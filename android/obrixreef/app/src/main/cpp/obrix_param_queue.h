// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
//
// Lock-free single-producer single-consumer (SPSC) ring buffer.
// Mirrors the ObrixPocket iOS bridge pattern: UI thread writes,
// audio thread reads. Zero allocation, zero locking.
//
// Usage:
//   Producer (UI thread):  queue.push({paramIndex, value});
//   Consumer (audio thread): Event e; while (queue.pop(e)) { apply(e); }
#pragma once

#include <atomic>
#include <cstdint>

namespace obrix
{

template <typename T, int Capacity = 256>
class SPSCQueue
{
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

public:
    /// Push an event from the producer thread. Returns false if full (drop policy).
    bool push(const T& event) noexcept
    {
        const uint32_t w = write_.load(std::memory_order_relaxed);
        const uint32_t nextW = (w + 1) & kMask;

        if (nextW == read_.load(std::memory_order_acquire))
            return false; // Full — drop

        buffer_[w] = event;
        write_.store(nextW, std::memory_order_release);
        return true;
    }

    /// Pop an event from the consumer thread. Returns false if empty.
    bool pop(T& event) noexcept
    {
        const uint32_t r = read_.load(std::memory_order_relaxed);

        if (r == write_.load(std::memory_order_acquire))
            return false; // Empty

        event = buffer_[r];
        read_.store((r + 1) & kMask, std::memory_order_release);
        return true;
    }

    /// Check if empty (consumer side).
    bool empty() const noexcept
    {
        return read_.load(std::memory_order_acquire) ==
               write_.load(std::memory_order_acquire);
    }

    /// Clear all events (consumer side only).
    void clear() noexcept
    {
        read_.store(write_.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

private:
    static constexpr uint32_t kMask = Capacity - 1;

    T buffer_[Capacity] = {};
    alignas(64) std::atomic<uint32_t> read_{0};
    alignas(64) std::atomic<uint32_t> write_{0};
};

// Event types for the queues

struct NoteEvent
{
    int note;
    float velocity; // 0 = note off, >0 = note on
};

struct ParamEvent
{
    int paramIndex;
    float value;
};

} // namespace obrix
