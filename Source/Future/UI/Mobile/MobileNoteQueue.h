// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// NOTE: This file is currently unused and not included in the build.
// Retained for potential future use. See GitHub issue #637.

#include <atomic>
#include <array>
#include <cstdint>

//==============================================================================
// MobileNoteQueue.h — Lock-free SPSC ring buffer for mobile note events.
//
// Replaces std::function NoteCallback (heap allocation on audio thread).
// Mandated by QDD LVL 3 audit: Smith: BLOCK, Buchla: BLOCK.
//
// Producer: message thread (MobilePlaySurface touch handlers)
// Consumer: audio thread (processBlock)
//
// Drop policy:
//   - NoteOn  : if full, drop incoming event and set overflow flag
//   - NoteOff : if full, FORCE push by evicting oldest slot (stuck-note prevention)
//
// NoteLifetimeWatchdog: fixed-size array of up to 32 active notes; no heap.
// Call checkZombies() periodically from the audio thread to auto-release notes
// that have been sustained longer than maxDurationMs (default 8000 ms).
//
// All methods are noexcept.
//==============================================================================

namespace xoceanus
{

//------------------------------------------------------------------------------
// MobileNoteEvent
//------------------------------------------------------------------------------

struct MobileNoteEvent
{
    int midiNote;        // 0-127
    float velocity;      // 0.0-1.0  (0.0 on noteOff)
    bool noteOn;         // true = note-on, false = note-off
    int touchId;         // correlates Began/Ended pairs
    int64_t timestampMs; // monotonic (from juce::Time::currentTimeMillis())
};

//------------------------------------------------------------------------------
// MobileNoteQueue — SPSC ring buffer, capacity 256
//------------------------------------------------------------------------------

class MobileNoteQueue
{
public:
    static constexpr int kCapacity = 256;

    MobileNoteQueue() noexcept = default;

    // Non-copyable, non-movable — atomics must not be relocated.
    MobileNoteQueue(const MobileNoteQueue&) = delete;
    MobileNoteQueue& operator=(const MobileNoteQueue&) = delete;
    MobileNoteQueue(MobileNoteQueue&&) = delete;
    MobileNoteQueue& operator=(MobileNoteQueue&&) = delete;

    //--------------------------------------------------------------------------
    // Producer side — message thread only
    //
    // Returns true if the event was stored.
    // Returns false if the buffer was full AND this was a noteOn that was
    // dropped (overflow flag is set in both cases where a drop occurs).
    //--------------------------------------------------------------------------
    bool pushNote(const MobileNoteEvent& event) noexcept
    {
        const int head = head_.load(std::memory_order_relaxed);
        const int next = advance(head);

        if (next == tail_.load(std::memory_order_acquire))
        {
            // Buffer is full.
            if (!event.noteOn)
            {
                // NoteOff MUST NOT be dropped per QDD mandate.
                // Force-push by evicting the oldest slot (advance tail by one).
                // The consumer may miss one event but will never get a stuck note.
                const int tail = tail_.load(std::memory_order_relaxed);
                buffer_[static_cast<size_t>(head)] = event;
                head_.store(next, std::memory_order_release);
                // Advance tail to reclaim the evicted slot, visible to consumer.
                tail_.store(advance(tail), std::memory_order_release);
                overflow_.store(true, std::memory_order_relaxed);
                return true; // NoteOff was delivered (with eviction)
            }
            else
            {
                // NoteOn: drop it; set overflow flag.
                overflow_.store(true, std::memory_order_relaxed);
                return false;
            }
        }

        buffer_[static_cast<size_t>(head)] = event;
        head_.store(next, std::memory_order_release);
        return true;
    }

    //--------------------------------------------------------------------------
    // Consumer side — audio thread only
    //
    // Pops the oldest event into `event`.
    // Returns true if an event was available, false if the queue was empty.
    //--------------------------------------------------------------------------
    bool popNote(MobileNoteEvent& event) noexcept
    {
        const int tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_.load(std::memory_order_acquire))
            return false; // Empty

        event = buffer_[static_cast<size_t>(tail)];
        tail_.store(advance(tail), std::memory_order_release);
        return true;
    }

    //--------------------------------------------------------------------------
    // Number of events waiting to be consumed.
    // Approximate — only safe to call from the consumer thread for scheduling;
    // the value may be stale by the time it is acted upon.
    //--------------------------------------------------------------------------
    int available() const noexcept
    {
        const int h = head_.load(std::memory_order_acquire);
        const int t = tail_.load(std::memory_order_acquire);
        if (h >= t)
            return h - t;
        return kCapacity - t + h;
    }

    //--------------------------------------------------------------------------
    // Diagnostics
    //--------------------------------------------------------------------------
    bool hadOverflow() const noexcept { return overflow_.load(std::memory_order_relaxed); }

    void clearOverflow() noexcept { overflow_.store(false, std::memory_order_relaxed); }

private:
    //--------------------------------------------------------------------------
    // Ring buffer storage.
    // Index arithmetic: indices run 0..kCapacity-1; one slot is always kept
    // empty so that head == tail unambiguously means "empty" (not "full").
    //--------------------------------------------------------------------------
    std::array<MobileNoteEvent, kCapacity> buffer_{};

    // head_ — next write position; owned by producer.
    // tail_ — next read position;  owned by consumer.
    std::atomic<int> head_{0};
    std::atomic<int> tail_{0};
    std::atomic<bool> overflow_{false};

    static int advance(int index) noexcept
    {
        // Branchless power-of-two wrap: kCapacity must be a power of 2.
        static_assert((kCapacity & (kCapacity - 1)) == 0, "MobileNoteQueue::kCapacity must be a power of 2");
        return (index + 1) & (kCapacity - 1);
    }
};

//------------------------------------------------------------------------------
// NoteLifetimeWatchdog
//
// Tracks up to kMaxActive concurrently sustained notes.
// Call noteOn() / noteOff() from the message thread when pushNote() is called.
// Call checkZombies() from the audio thread each processBlock() to detect and
// force-release notes that have been held longer than maxDurationMs_.
//
// No heap allocation. All storage is a fixed-size array.
//------------------------------------------------------------------------------

class NoteLifetimeWatchdog
{
public:
    static constexpr int kMaxActive = 32;
    static constexpr int64_t kDefaultMaxDurMs = 8000;

    NoteLifetimeWatchdog() noexcept
    {
        for (auto& n : active_)
            n = {};
    }

    NoteLifetimeWatchdog(const NoteLifetimeWatchdog&) = delete;
    NoteLifetimeWatchdog& operator=(const NoteLifetimeWatchdog&) = delete;

    //--------------------------------------------------------------------------
    // Call from message thread when a note-on is pushed.
    //--------------------------------------------------------------------------
    void noteOn(int touchId, int midiNote, int64_t timestampMs) noexcept
    {
        // Reuse an existing slot for this touchId (double-tap guard).
        for (auto& n : active_)
        {
            if (n.alive && n.touchId == touchId)
            {
                n.midiNote = midiNote;
                n.startTimeMs = timestampMs;
                return;
            }
        }
        // Find a free slot.
        for (auto& n : active_)
        {
            if (!n.alive)
            {
                n.touchId = touchId;
                n.midiNote = midiNote;
                n.startTimeMs = timestampMs;
                n.alive = true;
                return;
            }
        }
        // All slots occupied — evict oldest (prevents array from going stale).
        int64_t oldest = active_[0].startTimeMs;
        int oldestIdx = 0;
        for (int i = 1; i < kMaxActive; ++i)
        {
            if (active_[i].startTimeMs < oldest)
            {
                oldest = active_[i].startTimeMs;
                oldestIdx = i;
            }
        }
        active_[oldestIdx] = {touchId, midiNote, timestampMs, true};
    }

    //--------------------------------------------------------------------------
    // Call from message thread when a note-off is pushed.
    //--------------------------------------------------------------------------
    void noteOff(int touchId) noexcept
    {
        for (auto& n : active_)
        {
            if (n.alive && n.touchId == touchId)
            {
                n.alive = false;
                return;
            }
        }
    }

    //--------------------------------------------------------------------------
    // Call from audio thread (processBlock) — emits auto-off events for any
    // note that has been sustained longer than maxDurationMs_.
    //
    // Injects the synthetic note-off directly into the queue so the existing
    // consumer path handles it without any special-case logic.
    //--------------------------------------------------------------------------
    void checkZombies(int64_t currentTimeMs, MobileNoteQueue& queue) noexcept
    {
        for (auto& n : active_)
        {
            if (!n.alive)
                continue;

            if ((currentTimeMs - n.startTimeMs) >= maxDurationMs_)
            {
                MobileNoteEvent off{};
                off.midiNote = n.midiNote;
                off.velocity = 0.0f;
                off.noteOn = false;
                off.touchId = n.touchId;
                off.timestampMs = currentTimeMs;

                // pushNote() will force-push noteOff even if the queue is full
                // (QDD NoteOff-must-never-be-dropped mandate).
                queue.pushNote(off);

                // Mark the slot free — watchdog no longer owns this note.
                n.alive = false;
            }
        }
    }

    //--------------------------------------------------------------------------
    // Configuration
    //--------------------------------------------------------------------------
    void setMaxDuration(int64_t ms) noexcept { maxDurationMs_ = ms; }
    int64_t getMaxDuration() const noexcept { return maxDurationMs_; }

    // Number of currently tracked active notes (for diagnostics).
    int activeCount() const noexcept
    {
        int count = 0;
        for (const auto& n : active_)
            if (n.alive)
                ++count;
        return count;
    }

private:
    struct ActiveNote
    {
        int touchId = -1;
        int midiNote = -1;
        int64_t startTimeMs = 0;
        bool alive = false;
    };

    std::array<ActiveNote, kMaxActive> active_{};
    int64_t maxDurationMs_ = kDefaultMaxDurMs;
};

} // namespace xoceanus
