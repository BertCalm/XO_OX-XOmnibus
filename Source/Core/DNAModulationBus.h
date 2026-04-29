// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>

namespace xoceanus
{

//==============================================================================
// DNAModulationBus — promotes the 6D Sonic DNA from preset metadata to a live
// continuous modulation source readable by FX chains and engines.
//
// Per-engine warp (locked decision D1, 2026-04-27):
//   M1 CHARACTER macro warps each engine's DNA at the engine level (not per-voice,
//   not preset-load only). Update rate is block, smoothed to eliminate zipper noise.
//   CPU cost: 6 floats × N engines × block-rate (trivial).
//
// Why a side-channel bus (not a coupling source):
//   The MegaCouplingMatrix routes engine-to-engine signals. DNA is metadata that
//   characterises an engine, not a signal it produces. Consumers (FX chains, other
//   engines) query the bus directly by (engineSlot, axis). This keeps Phase 0
//   decoupled from matrix changes.
//
// Audio-thread contract:
//   - get() is O(1), lock-free, returns the smoothed value.
//   - setBaseDNA() and applyMacroWarp() are message-thread; they update an atomic
//     target which the next call to advanceSmoothing() interpolates toward.
//   - advanceSmoothing() must be called once per block from the audio thread.
//
// Per-axis weights (locked decision A1, 2026-04-27):
//   applyMacroWarp() has two overloads. The uniform overload warps all 6 axes
//   equally (sensible default). The per-axis overload accepts a weight vector so
//   preset designers can express "M1 mostly warps brightness/movement, leaves
//   warmth/aggression alone" — the kind of expressive design the 6D structure
//   exists for.
//
class DNAModulationBus
{
public:
    static constexpr int MaxEngineSlots = 4;
    static constexpr int NumAxes = 6;

    enum class Axis : int
    {
        Brightness = 0,
        Warmth     = 1,
        Movement   = 2,
        Density    = 3,
        Space      = 4,
        Aggression = 5
    };

    DNAModulationBus() noexcept
    {
        for (auto& slot : slots_)
        {
            for (int a = 0; a < NumAxes; ++a)
            {
                slot.base[a].store(0.5f, std::memory_order_relaxed);
                slot.target[a].store(0.5f, std::memory_order_relaxed);
                slot.smoothed[a] = 0.5f;
                slot.weights[a].store(1.0f, std::memory_order_relaxed);
            }
            slot.macroValue.store(0.0f, std::memory_order_relaxed);
        }
    }

    //-- Configuration (message thread) ----------------------------------------

    // Set the base DNA for an engine slot, typically called at preset load.
    // Values are clamped to [0, 1].
    void setBaseDNA(int engineSlot, std::array<float, NumAxes> dna) noexcept
    {
        if (! validSlot(engineSlot)) return;
        auto& slot = slots_[(size_t) engineSlot];
        for (int a = 0; a < NumAxes; ++a)
            slot.base[a].store(juce::jlimit(0.0f, 1.0f, dna[(size_t) a]),
                               std::memory_order_release);
        recomputeTarget(engineSlot);
    }

    // Apply the M1 CHARACTER macro warp uniformly across all 6 axes.
    // characterMacro is in [-1, 1]; positive shifts toward 1, negative toward 0.
    void applyMacroWarp(int engineSlot, float characterMacro) noexcept
    {
        if (! validSlot(engineSlot)) return;
        auto& slot = slots_[(size_t) engineSlot];
        slot.macroValue.store(juce::jlimit(-1.0f, 1.0f, characterMacro),
                              std::memory_order_release);
        // Reset weights to uniform if previously customised.
        for (int a = 0; a < NumAxes; ++a)
            slot.weights[a].store(1.0f, std::memory_order_release);
        recomputeTarget(engineSlot);
    }

    // Per-axis variant: weights[a] in [0, 1] scales how strongly the macro
    // warps axis `a`. Locked by decision A1 — enables expressive presets where
    // M1 mostly affects brightness/movement and leaves warmth alone.
    void applyMacroWarp(int engineSlot,
                        float characterMacro,
                        std::array<float, NumAxes> weights) noexcept
    {
        if (! validSlot(engineSlot)) return;
        auto& slot = slots_[(size_t) engineSlot];
        slot.macroValue.store(juce::jlimit(-1.0f, 1.0f, characterMacro),
                              std::memory_order_release);
        for (int a = 0; a < NumAxes; ++a)
            slot.weights[a].store(juce::jlimit(0.0f, 1.0f, weights[(size_t) a]),
                                  std::memory_order_release);
        recomputeTarget(engineSlot);
    }

    //-- Audio thread ----------------------------------------------------------

    // Read the smoothed DNA value for (engineSlot, axis). O(1), lock-free.
    // Returns 0.5 if engineSlot is out of range (neutral DNA).
    float get(int engineSlot, Axis axis) const noexcept
    {
        if (! validSlot(engineSlot)) return 0.5f;
        return slots_[(size_t) engineSlot].smoothed[(int) axis];
    }

    // Convenience: read all 6 axes at once for one engine.
    std::array<float, NumAxes> getAll(int engineSlot) const noexcept
    {
        std::array<float, NumAxes> out;
        if (! validSlot(engineSlot))
        {
            out.fill(0.5f);
            return out;
        }
        for (int a = 0; a < NumAxes; ++a)
            out[(size_t) a] = slots_[(size_t) engineSlot].smoothed[a];
        return out;
    }

    // Advance smoothing by one block. Must be called once per processBlock from
    // the audio thread. blockSize is informational; the smoothing constant is
    // pre-baked for typical block sizes (32–2048).
    void advanceSmoothing(int /*blockSize*/) noexcept
    {
        constexpr float kSmoothingCoeff = 0.05f; // ~20 ms at 48 kHz, 256-sample blocks
        for (auto& slot : slots_)
        {
            for (int a = 0; a < NumAxes; ++a)
            {
                const float target = slot.target[a].load(std::memory_order_acquire);
                slot.smoothed[a] += kSmoothingCoeff * (target - slot.smoothed[a]);
            }
        }
    }

private:
    struct Slot
    {
        std::array<std::atomic<float>, NumAxes> base;     // raw preset DNA, [0,1]
        std::array<std::atomic<float>, NumAxes> weights;  // per-axis macro weights, [0,1]
        std::atomic<float>                      macroValue { 0.0f }; // M1 CHARACTER, [-1,1]
        std::array<std::atomic<float>, NumAxes> target;   // computed target after warp
        std::array<float, NumAxes>              smoothed; // audio-thread state
    };

    static bool validSlot(int slot) noexcept
    {
        return slot >= 0 && slot < MaxEngineSlots;
    }

    // Compute target = base + macroValue * weight * (direction-toward-extreme).
    // Called from message thread whenever base, macroValue, or weights change.
    void recomputeTarget(int engineSlot) noexcept
    {
        auto& slot = slots_[(size_t) engineSlot];
        const float macro = slot.macroValue.load(std::memory_order_acquire);
        for (int a = 0; a < NumAxes; ++a)
        {
            const float base = slot.base[a].load(std::memory_order_acquire);
            const float w    = slot.weights[a].load(std::memory_order_acquire);
            // Positive macro pushes toward 1; negative toward 0.
            // Magnitude is scaled by axis weight.
            const float t = (macro >= 0.0f)
                          ? base + w * macro * (1.0f - base)
                          : base + w * macro * base;
            slot.target[a].store(juce::jlimit(0.0f, 1.0f, t),
                                 std::memory_order_release);
        }
    }

    std::array<Slot, MaxEngineSlots> slots_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DNAModulationBus)
};

} // namespace xoceanus
