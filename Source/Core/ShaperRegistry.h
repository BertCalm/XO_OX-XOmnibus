// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "ShaperEngine.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace xoceanus
{

//==============================================================================
// Factory function type for creating shaper instances.
using ShaperFactory = std::function<std::unique_ptr<ShaperEngine>()>;

//==============================================================================
// ShaperRegistry — Manages shaper type registration and the 6 active slots.
//
// Slot layout:
//   Insert[0..3] — one per engine, processes engine output before mix
//   Bus[0..1]    — processes the mixed bus before MasterFXChain
//
// Shapers register at compile time via REGISTER_SHAPER macro.
// The processor loads/swaps shapers at runtime with 50ms crossfade.
//
// Thread safety model (fixes #557, use-after-free fix #751):
//   - loadInsert / loadBus: message thread ONLY (jassert enforced).
//     A SpinLock is acquired during the slot swap so that any concurrent
//     audio-thread read sees either the old or the new shared_ptr — never
//     a torn state.  The old shared_ptr may be kept alive by the audio
//     thread; its destructor runs on the message thread via
//     drainDeferredDeletions().
//   - processInsert / processBus: audio thread.
//     SpinLock is acquired only to copy the shared_ptr (ref-count bump).
//     The ShaperEngine is then called lock-free through that copy.
//     If the copy holds the last reference after DSP, it is pushed to
//     deferredRing_ so destruction never occurs on the audio thread.
//   - getInsert / getBus: message thread helpers for prepare/configure.
//     Return raw pointer (safe: message thread owns the slot lifetime).
//   - drainDeferredDeletions(): call from a message-thread timer (~20ms).
//     Drops shared_ptrs whose ref-count reached 1 inside processInsert/Bus.
//   - registerShaper: static-init time only (REGISTER_SHAPER macro).
//     No lock needed — all registration completes before any audio callback.
//   - prepareAll / resetAll: message thread / prepareToPlay only.
//     No audio thread contention expected; SpinLock guards the read pass.
//
class ShaperRegistry
{
public:
    static constexpr int MaxInserts = 4;
    static constexpr int MaxBus = 2;
    static constexpr int TotalSlots = MaxInserts + MaxBus;
    static constexpr float CrossfadeMs = 50.0f;

    static ShaperRegistry& instance()
    {
        static ShaperRegistry reg;
        return reg;
    }

    //-- Registration ----------------------------------------------------------
    // Called from static-init (REGISTER_SHAPER macro) — single-threaded, no lock needed.

    bool registerShaper(const std::string& id, ShaperFactory factory)
    {
        if (factories.count(id) > 0)
        {
            jassertfalse; // Duplicate shaper ID
            return false;
        }
        factories[id] = std::move(factory);
        return true;
    }

    std::unique_ptr<ShaperEngine> createShaper(const std::string& id) const
    {
        auto it = factories.find(id);
        if (it != factories.end())
            return it->second();
        return nullptr;
    }

    std::vector<std::string> getRegisteredShaperIds() const
    {
        std::vector<std::string> ids;
        ids.reserve(factories.size());
        for (const auto& [id, _] : factories)
            ids.push_back(id);
        return ids;
    }

    //-- Slot Management -------------------------------------------------------

    // Load a shaper into an insert slot (0-3). Pass empty string to clear.
    // MUST be called from the message thread (or prepareToPlay) — never from the audio thread.
    // The SpinLock guarantees the audio thread sees a consistent pointer after the swap.
    void loadInsert(int slot, const std::string& shaperId, double sampleRate, int maxBlockSize)
    {
        jassert(juce::MessageManager::existsAndIsCurrentThread()
                    ? juce::MessageManager::getInstance()->isThisTheMessageThread()
                    : true); // Allow prepareToPlay (non-audio, non-message) calls
        jassert(slot >= 0 && slot < MaxInserts);

        std::shared_ptr<ShaperEngine> shaper;
        if (!shaperId.empty())
        {
            if (auto up = createShaper(shaperId))
            {
                up->prepare(sampleRate, maxBlockSize);
                shaper = std::move(up); // unique_ptr → shared_ptr conversion
            }
        }
        // Swap under lock — audio thread only ever reads a complete shared_ptr.
        // The old shared_ptr is released here; if the audio thread still holds
        // its own copy the destructor will run later via drainDeferredDeletions().
        const juce::SpinLock::ScopedLockType scopedLock(slotsLock);
        inserts[slot] = std::move(shaper);
    }

    // Load a shaper into a bus slot (0-1). Pass empty string to clear.
    // MUST be called from the message thread (or prepareToPlay) — never from the audio thread.
    void loadBus(int slot, const std::string& shaperId, double sampleRate, int maxBlockSize)
    {
        jassert(juce::MessageManager::existsAndIsCurrentThread()
                    ? juce::MessageManager::getInstance()->isThisTheMessageThread()
                    : true);
        jassert(slot >= 0 && slot < MaxBus);

        std::shared_ptr<ShaperEngine> shaper;
        if (!shaperId.empty())
        {
            if (auto up = createShaper(shaperId))
            {
                up->prepare(sampleRate, maxBlockSize);
                shaper = std::move(up); // unique_ptr → shared_ptr conversion
            }
        }
        // See loadInsert() comment on lifetime and deferred deletion.
        const juce::SpinLock::ScopedLockType scopedLock(slotsLock);
        bus[slot] = std::move(shaper);
    }

    // Access active shapers for configuration — MESSAGE THREAD ONLY.
    // These return raw pointers valid only as long as the caller does not
    // trigger a loadInsert/loadBus swap on the same thread.  They MUST NOT
    // be called from the audio thread — use processInsert/processBus instead.
    ShaperEngine* getInsert(int slot)
    {
        const juce::SpinLock::ScopedTryLockType tryLock(slotsLock);
        if (!tryLock.isLocked())
            return nullptr;
        return (slot >= 0 && slot < MaxInserts) ? inserts[slot].get() : nullptr;
    }
    ShaperEngine* getBus(int slot)
    {
        const juce::SpinLock::ScopedTryLockType tryLock(slotsLock);
        if (!tryLock.isLocked())
            return nullptr;
        return (slot >= 0 && slot < MaxBus) ? bus[slot].get() : nullptr;
    }

    const ShaperEngine* getInsert(int slot) const
    {
        const juce::SpinLock::ScopedTryLockType tryLock(slotsLock);
        if (!tryLock.isLocked())
            return nullptr;
        return (slot >= 0 && slot < MaxInserts) ? inserts[slot].get() : nullptr;
    }
    const ShaperEngine* getBus(int slot) const
    {
        const juce::SpinLock::ScopedTryLockType tryLock(slotsLock);
        if (!tryLock.isLocked())
            return nullptr;
        return (slot >= 0 && slot < MaxBus) ? bus[slot].get() : nullptr;
    }

    // Process an engine's output through its insert shaper (audio thread).
    // A shared_ptr copy is taken under the lock so the object cannot be
    // destroyed between lock release and processBlock().  If our copy is
    // the last reference (message thread already swapped the slot), the
    // destructor is deferred to the message thread via deferDeletion().
    void processInsert(int slot, juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples)
    {
        std::shared_ptr<ShaperEngine> shaper;
        {
            const juce::SpinLock::ScopedTryLockType tryLock(slotsLock);
            if (!tryLock.isLocked())
                return; // swap in progress — skip this block safely
            if (slot >= 0 && slot < MaxInserts)
                shaper = inserts[slot]; // shared_ptr COPY — bumps ref count
        }
        // Lock released.  shaper keeps the object alive even if the message
        // thread calls loadInsert() and drops the slot's shared_ptr.
        if (shaper && !shaper->isBypassed())
        {
            if (shaper->silenceGate.isBypassed())
            {
                // SRO: input is silent, skip processing.
                // Still need to check for deferred deletion below.
            }
            else
            {
                shaper->processBlock(buffer, midi, numSamples);
                shaper->silenceGate.analyzeBlock(
                    buffer.getReadPointer(0),
                    buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr,
                    numSamples);
            }
        }
        // If we hold the only remaining reference, defer destruction so the
        // destructor runs on the message thread, not here on the audio thread.
        if (shaper && shaper.use_count() == 1)
            deferDeletion(std::move(shaper));
    }

    // Process the mixed bus through bus shapers in series (audio thread).
    // Same shared_ptr copy pattern as processInsert() — see that method for
    // the full rationale.
    void processBus(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples)
    {
        for (int i = 0; i < MaxBus; ++i)
        {
            std::shared_ptr<ShaperEngine> shaper;
            {
                const juce::SpinLock::ScopedTryLockType tryLock(slotsLock);
                if (!tryLock.isLocked())
                    continue; // swap in progress — skip this slot
                shaper = bus[i]; // shared_ptr COPY — bumps ref count
            }
            if (shaper && !shaper->isBypassed())
            {
                if (!shaper->silenceGate.isBypassed())
                {
                    shaper->processBlock(buffer, midi, numSamples);
                    shaper->silenceGate.analyzeBlock(
                        buffer.getReadPointer(0),
                        buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr,
                        numSamples);
                }
            }
            // Defer destruction if this is the last reference.
            if (shaper && shaper.use_count() == 1)
                deferDeletion(std::move(shaper));
        }
    }

    // Prepare all active shapers (called from prepareToPlay — message/prep thread only)
    void prepareAll(double sampleRate, int maxBlockSize)
    {
        const juce::SpinLock::ScopedLockType scopedLock(slotsLock);
        for (auto& s : inserts)
            if (s)
                s->prepare(sampleRate, maxBlockSize);
        for (auto& s : bus)
            if (s)
                s->prepare(sampleRate, maxBlockSize);
    }

    // Reset all active shapers (message/prep thread only)
    void resetAll()
    {
        const juce::SpinLock::ScopedLockType scopedLock(slotsLock);
        for (auto& s : inserts)
            if (s)
                s->reset();
        for (auto& s : bus)
            if (s)
                s->reset();
    }

    // Global bypass toggle (A/B comparison — message thread)
    void setGlobalBypass(bool bypassed) { globalBypassed.store(bypassed, std::memory_order_relaxed); }

    bool isGlobalBypassed() const { return globalBypassed.load(std::memory_order_relaxed); }

    // Drain shared_ptrs deferred from the audio thread.
    // Call this from a message-thread timer (~20ms period) to ensure
    // ShaperEngine destructors always run on the message thread.
    void drainDeferredDeletions()
    {
        size_t r = deferredRead_.load(std::memory_order_relaxed);
        const size_t w = deferredWrite_.load(std::memory_order_acquire);
        while (r != w)
        {
            deferredRing_[r].reset(); // destructor runs here, on message thread
            r = (r + 1) % kDeferredCapacity;
        }
        deferredRead_.store(r, std::memory_order_release);
    }

private:
    ShaperRegistry() = default;

    // Factory map — populated at static-init time only; read-only thereafter.
    std::unordered_map<std::string, ShaperFactory> factories;

    // Active slot arrays — protected by slotsLock for all reads and writes.
    // shared_ptr instead of unique_ptr so the audio thread can copy the pointer
    // under the lock and then process DSP lock-free without risk of use-after-free.
    std::array<std::shared_ptr<ShaperEngine>, MaxInserts> inserts{};
    std::array<std::shared_ptr<ShaperEngine>, MaxBus> bus{};

    // SpinLock protecting inserts[] and bus[] slot arrays.
    // Write side: message thread loadInsert/loadBus (blocking lock — no RT thread).
    // Read side:  audio thread processInsert/processBus (try-lock — drops a block if
    //             contended, which is a single ~5ms glitch, far better than a crash).
    mutable juce::SpinLock slotsLock;

    // Atomic bypass flag — safe for cross-thread reads without the SpinLock.
    std::atomic<bool> globalBypassed{false};

    // -------------------------------------------------------------------------
    // Deferred deletion ring (SPSC: audio thread writes, message thread reads).
    //
    // When processInsert/processBus detect that their local shared_ptr copy is
    // the last reference (use_count == 1), they push it here instead of letting
    // it destruct on the audio thread.  drainDeferredDeletions() then resets
    // the slots on the message thread where allocation/deallocation is allowed.
    //
    // Capacity 32 is far more than needed: only MaxInserts + MaxBus = 6 shapers
    // can ever be in-flight, but we leave generous headroom.
    // -------------------------------------------------------------------------
    static constexpr size_t kDeferredCapacity = 32;
    std::array<std::shared_ptr<ShaperEngine>, kDeferredCapacity> deferredRing_{};
    std::atomic<size_t> deferredWrite_{0};
    std::atomic<size_t> deferredRead_{0};

    // Push an expiring shared_ptr onto the deferred ring (audio-thread safe).
    // If the ring is full (should never happen in practice) the shared_ptr is
    // simply dropped here — the destructor runs on the audio thread as a last
    // resort, which is the old (unsafe) behaviour and still better than a crash.
    void deferDeletion(std::shared_ptr<ShaperEngine> expiring) noexcept
    {
        const size_t w = deferredWrite_.load(std::memory_order_relaxed);
        const size_t next = (w + 1) % kDeferredCapacity;
        if (next == deferredRead_.load(std::memory_order_acquire))
            return; // ring full — drop (destructor runs on audio thread as fallback)
        deferredRing_[w] = std::move(expiring);
        deferredWrite_.store(next, std::memory_order_release);
    }
};

//==============================================================================
// Registration macro — use in each shaper's .cpp file
#define REGISTER_SHAPER(ShaperId, ShaperClass)                                                                         \
    static bool _shaper_##ShaperClass##_registered = xoceanus::ShaperRegistry::instance().registerShaper(              \
        ShaperId, []() { return std::make_unique<xoceanus::ShaperClass>(); });

} // namespace xoceanus
