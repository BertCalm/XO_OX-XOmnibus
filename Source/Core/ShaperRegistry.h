// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "ShaperEngine.h"
#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace xoceanus {

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
// Thread safety model (fixes #557):
//   - loadInsert / loadBus: message thread ONLY (jassert enforced).
//     A SpinLock is acquired during the slot swap so that any concurrent
//     audio-thread read (getInsert / getBus / processInsert / processBus)
//     sees either the old or the new pointer — never a torn state.
//   - getInsert / getBus / processInsert / processBus: audio thread.
//     SpinLock is acquired for the minimum window (pointer copy only).
//     The ShaperEngine itself is then called lock-free.
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

    bool registerShaper (const std::string& id, ShaperFactory factory)
    {
        if (factories.count (id) > 0)
        {
            jassertfalse; // Duplicate shaper ID
            return false;
        }
        factories[id] = std::move (factory);
        return true;
    }

    std::unique_ptr<ShaperEngine> createShaper (const std::string& id) const
    {
        auto it = factories.find (id);
        if (it != factories.end())
            return it->second();
        return nullptr;
    }

    std::vector<std::string> getRegisteredShaperIds() const
    {
        std::vector<std::string> ids;
        ids.reserve (factories.size());
        for (const auto& [id, _] : factories)
            ids.push_back (id);
        return ids;
    }

    //-- Slot Management -------------------------------------------------------

    // Load a shaper into an insert slot (0-3). Pass empty string to clear.
    // MUST be called from the message thread (or prepareToPlay) — never from the audio thread.
    // The SpinLock guarantees the audio thread sees a consistent pointer after the swap.
    void loadInsert (int slot, const std::string& shaperId, double sampleRate, int maxBlockSize)
    {
        jassert (juce::MessageManager::existsAndIsCurrentThread()
                 ? juce::MessageManager::getInstance()->isThisTheMessageThread()
                 : true); // Allow prepareToPlay (non-audio, non-message) calls
        jassert (slot >= 0 && slot < MaxInserts);

        std::unique_ptr<ShaperEngine> shaper;
        if (! shaperId.empty())
        {
            shaper = createShaper (shaperId);
            if (shaper)
                shaper->prepare (sampleRate, maxBlockSize);
        }
        // Swap under lock — audio thread only ever reads a complete pointer.
        const juce::SpinLock::ScopedLockType scopedLock (slotsLock);
        inserts[slot] = std::move (shaper);
    }

    // Load a shaper into a bus slot (0-1). Pass empty string to clear.
    // MUST be called from the message thread (or prepareToPlay) — never from the audio thread.
    void loadBus (int slot, const std::string& shaperId, double sampleRate, int maxBlockSize)
    {
        jassert (juce::MessageManager::existsAndIsCurrentThread()
                 ? juce::MessageManager::getInstance()->isThisTheMessageThread()
                 : true);
        jassert (slot >= 0 && slot < MaxBus);

        std::unique_ptr<ShaperEngine> shaper;
        if (! shaperId.empty())
        {
            shaper = createShaper (shaperId);
            if (shaper)
                shaper->prepare (sampleRate, maxBlockSize);
        }
        const juce::SpinLock::ScopedLockType scopedLock (slotsLock);
        bus[slot] = std::move (shaper);
    }

    // Access active shapers for processing — audio-thread safe (SpinLock for pointer read).
    // Caller must NOT hold the pointer across a message-thread loadInsert/loadBus call.
    // In practice processInsert / processBus are the correct entry points.
    ShaperEngine* getInsert (int slot)
    {
        const juce::SpinLock::ScopedTryLockType tryLock (slotsLock);
        if (! tryLock.isLocked()) return nullptr; // message thread is swapping — skip this block
        return (slot >= 0 && slot < MaxInserts) ? inserts[slot].get() : nullptr;
    }
    ShaperEngine* getBus (int slot)
    {
        const juce::SpinLock::ScopedTryLockType tryLock (slotsLock);
        if (! tryLock.isLocked()) return nullptr;
        return (slot >= 0 && slot < MaxBus) ? bus[slot].get() : nullptr;
    }

    const ShaperEngine* getInsert (int slot) const
    {
        const juce::SpinLock::ScopedTryLockType tryLock (slotsLock);
        if (! tryLock.isLocked()) return nullptr;
        return (slot >= 0 && slot < MaxInserts) ? inserts[slot].get() : nullptr;
    }
    const ShaperEngine* getBus (int slot) const
    {
        const juce::SpinLock::ScopedTryLockType tryLock (slotsLock);
        if (! tryLock.isLocked()) return nullptr;
        return (slot >= 0 && slot < MaxBus) ? bus[slot].get() : nullptr;
    }

    // Process an engine's output through its insert shaper (audio thread)
    void processInsert (int slot, juce::AudioBuffer<float>& buffer,
                        juce::MidiBuffer& midi, int numSamples)
    {
        // Acquire lock only for the pointer copy, then release before DSP.
        ShaperEngine* shaper = nullptr;
        {
            const juce::SpinLock::ScopedTryLockType tryLock (slotsLock);
            if (! tryLock.isLocked()) return; // swap in progress — skip this block safely
            if (slot >= 0 && slot < MaxInserts)
                shaper = inserts[slot].get();
        }
        if (shaper && ! shaper->isBypassed())
        {
            if (shaper->silenceGate.isBypassed())
                return; // SRO: input is silent, skip processing

            shaper->processBlock (buffer, midi, numSamples);
            shaper->silenceGate.analyzeBlock (buffer.getReadPointer (0),
                buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr,
                numSamples);
        }
    }

    // Process the mixed bus through bus shapers in series (audio thread)
    void processBus (juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi, int numSamples)
    {
        for (int i = 0; i < MaxBus; ++i)
        {
            ShaperEngine* shaper = nullptr;
            {
                const juce::SpinLock::ScopedTryLockType tryLock (slotsLock);
                if (! tryLock.isLocked()) continue;
                shaper = bus[i].get();
            }
            if (shaper && ! shaper->isBypassed())
            {
                if (shaper->silenceGate.isBypassed())
                    continue;

                shaper->processBlock (buffer, midi, numSamples);
                shaper->silenceGate.analyzeBlock (buffer.getReadPointer (0),
                    buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr,
                    numSamples);
            }
        }
    }

    // Prepare all active shapers (called from prepareToPlay — message/prep thread only)
    void prepareAll (double sampleRate, int maxBlockSize)
    {
        const juce::SpinLock::ScopedLockType scopedLock (slotsLock);
        for (auto& s : inserts) if (s) s->prepare (sampleRate, maxBlockSize);
        for (auto& s : bus)     if (s) s->prepare (sampleRate, maxBlockSize);
    }

    // Reset all active shapers (message/prep thread only)
    void resetAll()
    {
        const juce::SpinLock::ScopedLockType scopedLock (slotsLock);
        for (auto& s : inserts) if (s) s->reset();
        for (auto& s : bus)     if (s) s->reset();
    }

    // Global bypass toggle (A/B comparison — message thread)
    void setGlobalBypass (bool bypassed)
    {
        globalBypassed.store (bypassed, std::memory_order_relaxed);
    }

    bool isGlobalBypassed() const
    {
        return globalBypassed.load (std::memory_order_relaxed);
    }

private:
    ShaperRegistry() = default;

    // Factory map — populated at static-init time only; read-only thereafter.
    std::unordered_map<std::string, ShaperFactory> factories;

    // Active slot arrays — protected by slotsLock for all reads and writes.
    std::array<std::unique_ptr<ShaperEngine>, MaxInserts> inserts {};
    std::array<std::unique_ptr<ShaperEngine>, MaxBus> bus {};

    // SpinLock protecting inserts[] and bus[] slot arrays.
    // Write side: message thread loadInsert/loadBus (blocking lock — no RT thread).
    // Read side:  audio thread processInsert/processBus (try-lock — drops a block if
    //             contended, which is a single ~5ms glitch, far better than a crash).
    mutable juce::SpinLock slotsLock;

    // Atomic bypass flag — safe for cross-thread reads without the SpinLock.
    std::atomic<bool> globalBypassed { false };
};

//==============================================================================
// Registration macro — use in each shaper's .cpp file
#define REGISTER_SHAPER(ShaperId, ShaperClass)                               \
    static bool _shaper_##ShaperClass##_registered =                         \
        xoceanus::ShaperRegistry::instance().registerShaper(                 \
            ShaperId, []() { return std::make_unique<xoceanus::ShaperClass>(); });

} // namespace xoceanus
