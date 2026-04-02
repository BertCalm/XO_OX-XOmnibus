#pragma once
#include "SynthEngine.h"
#include <array>
#include <atomic>

namespace xoceanus {

//==============================================================================
// CouplingCrossfader — Smooth transitions when switching coupling types in
// real time. Prevents clicks and discontinuities for audio-rate coupling types
// (FM, Ring, Wavetable, Buffer, Knot) by running dual evaluation with a linear
// crossfade. Control-rate types are hard-switchable and bypass the crossfade.
//
// Thread safety:
//   - No allocations, no locks. All state is fixed-size and audio-thread safe.
//   - Called from MegaCouplingMatrix::processBlock() on the audio thread.
//   - Type changes are detected by comparing previous vs. current type per route.
//
// Design ref: Docs/specs/coupling_performance_spec.md §3
//
class CouplingCrossfader {
public:
    static constexpr int MaxRouteSlots = 4;

    //-- Crossfade durations (in milliseconds) ---------------------------------

    static constexpr float kDefaultCrossfadeMs = 50.0f;   // AudioToFM, AudioToRing, AudioToWavetable
    static constexpr float kLongCrossfadeMs    = 100.0f;  // AudioToBuffer, KnotTopology

    //-- Lifecycle -------------------------------------------------------------

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        currentSampleRate.store(sampleRate, std::memory_order_relaxed);

        for (auto& slot : slots)
            slot.reset();
    }

    //-- Per-route type change detection and crossfade state management ---------

    // Call once per route per processBlock BEFORE evaluation.
    // Returns true if a crossfade is active for this route slot.
    // `routeIndex` is 0-3 (the performance route slot index).
    // `newType` is the coupling type the route should evaluate this block.
    bool updateRouteType(int routeIndex, CouplingType newType)
    {
        jassert(routeIndex >= 0 && routeIndex < MaxRouteSlots);
        if (routeIndex < 0 || routeIndex >= MaxRouteSlots)
            return false;

        auto& slot = slots[static_cast<size_t>(routeIndex)];

        // First call — no previous type to compare against.
        if (!slot.initialized)
        {
            slot.currentType = newType;
            slot.initialized = true;
            return false;
        }

        // No change — check if we're mid-crossfade from a prior switch.
        if (slot.currentType == newType && !slot.crossfadeActive)
            return false;

        // Type changed — start a new crossfade (if the new type or the old type
        // requires one). If both old and new types are control-rate safe, the
        // crossfade is skipped entirely.
        if (slot.currentType != newType)
        {
            if (needsCrossfade(slot.currentType) || needsCrossfade(newType))
            {
                slot.previousType = slot.currentType;
                slot.currentType = newType;
                slot.crossfadeActive = true;
                slot.crossfadeProgress = 0.0f;
                slot.crossfadeDuration = getCrossfadeDuration(slot.previousType, newType);
            }
            else
            {
                // Both types are control-rate safe — hard switch, no crossfade.
                slot.currentType = newType;
                slot.crossfadeActive = false;
                return false;
            }
        }

        return slot.crossfadeActive;
    }

    //-- Crossfade state queries -----------------------------------------------

    bool isCrossfading(int routeIndex) const
    {
        if (routeIndex < 0 || routeIndex >= MaxRouteSlots)
            return false;
        return slots[static_cast<size_t>(routeIndex)].crossfadeActive;
    }

    CouplingType getPreviousType(int routeIndex) const
    {
        if (routeIndex < 0 || routeIndex >= MaxRouteSlots)
            return CouplingType::AmpToFilter;
        return slots[static_cast<size_t>(routeIndex)].previousType;
    }

    CouplingType getCurrentType(int routeIndex) const
    {
        if (routeIndex < 0 || routeIndex >= MaxRouteSlots)
            return CouplingType::AmpToFilter;
        return slots[static_cast<size_t>(routeIndex)].currentType;
    }

    //-- Crossfade execution ---------------------------------------------------

    // Apply per-sample crossfade between two evaluated coupling outputs.
    // `prevBuffer` contains the result of evaluating the previous coupling type.
    // `nextBuffer` contains the result of evaluating the new coupling type.
    // `outputBuffer` receives the blended result.
    // `numSamples` is the block size.
    // `routeIndex` is 0-3.
    //
    // Call this AFTER evaluating both coupling types for the block.
    // The crossfade progresses sample-by-sample for click-free transitions.
    //
    void applyCrossfade(int routeIndex,
                        const float* prevBuffer,
                        const float* nextBuffer,
                        float* outputBuffer,
                        int numSamples)
    {
        if (currentSampleRate.load(std::memory_order_relaxed) <= 0.0) return;

        if (prevBuffer == nullptr || nextBuffer == nullptr || outputBuffer == nullptr || numSamples <= 0)
            return;

        if (routeIndex < 0 || routeIndex >= MaxRouteSlots)
            return;

        auto& slot = slots[static_cast<size_t>(routeIndex)];

        if (!slot.crossfadeActive)
        {
            // No crossfade — copy nextBuffer straight through.
            for (int i = 0; i < numSamples; ++i)
                outputBuffer[i] = nextBuffer[i];
            return;
        }

        // Convert duration from ms to samples.
        const float durationSamples = (slot.crossfadeDuration * 0.001f)
                                    * static_cast<float>(currentSampleRate.load(std::memory_order_relaxed));
        const float invDuration = (durationSamples > 0.0f)
                                ? 1.0f / durationSamples
                                : 1.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float fade = slot.crossfadeProgress * invDuration;

            if (fade >= 1.0f)
            {
                // Crossfade complete — copy remaining samples from nextBuffer.
                for (int j = i; j < numSamples; ++j)
                    outputBuffer[j] = nextBuffer[j];

                slot.crossfadeActive = false;
                return;
            }

            // Linear crossfade: prev * (1 - fade) + next * fade
            outputBuffer[i] = prevBuffer[i] * (1.0f - fade)
                            + nextBuffer[i] * fade;
            slot.crossfadeProgress += 1.0f;
        }
    }

    // Convenience: in-place crossfade that blends prevBuffer into nextBuffer.
    // After this call, nextBuffer contains the crossfaded result.
    void applyCrossfadeInPlace(int routeIndex,
                               const float* prevBuffer,
                               float* nextBuffer,
                               int numSamples)
    {
        applyCrossfade(routeIndex, prevBuffer, nextBuffer, nextBuffer, numSamples);
    }

    //-- Block-level crossfade advance (for non-sample-accurate callers) -------

    // Advance the crossfade by a block's worth of time without producing audio.
    // Useful when the caller handles the blend externally (e.g. KnotTopology
    // bidirectional path where both directions need independent evaluation).
    void advanceCrossfade(int routeIndex, int numSamples)
    {
        if (routeIndex < 0 || routeIndex >= MaxRouteSlots)
            return;

        auto& slot = slots[static_cast<size_t>(routeIndex)];
        if (!slot.crossfadeActive)
            return;

        const float durationSamples = (slot.crossfadeDuration * 0.001f)
                                    * static_cast<float>(currentSampleRate.load(std::memory_order_relaxed));
        slot.crossfadeProgress += static_cast<float>(numSamples);

        if (slot.crossfadeProgress >= durationSamples)
            slot.crossfadeActive = false;
    }

    // Get the current crossfade mix amount (0.0 = fully previous, 1.0 = fully current).
    // Returns 1.0 if no crossfade is active (fully transitioned to current type).
    float getCrossfadeMix(int routeIndex) const
    {
        if (routeIndex < 0 || routeIndex >= MaxRouteSlots)
            return 1.0f;

        const auto& slot = slots[static_cast<size_t>(routeIndex)];
        if (!slot.crossfadeActive)
            return 1.0f;

        const float durationSamples = (slot.crossfadeDuration * 0.001f)
                                    * static_cast<float>(currentSampleRate.load(std::memory_order_relaxed));
        if (durationSamples <= 0.0f)
            return 1.0f;

        return juce::jmin(slot.crossfadeProgress / durationSamples, 1.0f);
    }

    //-- Reset -----------------------------------------------------------------

    void resetSlot(int routeIndex)
    {
        if (routeIndex >= 0 && routeIndex < MaxRouteSlots)
            slots[static_cast<size_t>(routeIndex)].reset();
    }

    void resetAll()
    {
        for (auto& slot : slots)
            slot.reset();
    }

private:
    //-- Type classification ---------------------------------------------------

    // Audio-rate coupling types that produce discontinuities when switched.
    // Control-rate types (all others) use SRO decimation and are inherently
    // smooth — hard-switching is safe.
    static bool needsCrossfade(CouplingType type)
    {
        switch (type)
        {
            case CouplingType::AudioToFM:
            case CouplingType::AudioToRing:
            case CouplingType::AudioToWavetable:
            case CouplingType::AudioToBuffer:
            case CouplingType::KnotTopology:
                return true;

            default:
                return false;
        }
    }

    // Choose crossfade duration based on the more demanding type.
    // AudioToBuffer and KnotTopology need longer fades (100ms) due to
    // buffer swap latency and bidirectional coupling respectively.
    static float getCrossfadeDuration(CouplingType prevType, CouplingType newType)
    {
        if (prevType == CouplingType::AudioToBuffer
         || newType == CouplingType::AudioToBuffer
         || prevType == CouplingType::KnotTopology
         || newType == CouplingType::KnotTopology)
            return kLongCrossfadeMs;

        return kDefaultCrossfadeMs;
    }

    //-- Per-route crossfade state ---------------------------------------------

    struct RouteSlot {
        CouplingType currentType  = CouplingType::AmpToFilter;
        CouplingType previousType = CouplingType::AmpToFilter;
        float crossfadeProgress   = 0.0f;   // in samples
        float crossfadeDuration   = 0.0f;   // in ms (converted to samples at use site)
        bool  crossfadeActive     = false;
        bool  initialized         = false;

        void reset()
        {
            currentType      = CouplingType::AmpToFilter;
            previousType     = CouplingType::AmpToFilter;
            crossfadeProgress = 0.0f;
            crossfadeDuration = 0.0f;
            crossfadeActive  = false;
            initialized      = false;
        }
    };

    std::array<RouteSlot, MaxRouteSlots> slots {};
    std::atomic<double> currentSampleRate { 0.0 };
};

} // namespace xoceanus
