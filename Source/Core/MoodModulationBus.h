// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// MoodModulationBus — promotes a preset's 16-mood tag from static metadata to a
// live continuous modulation source readable by FX chains and engines.
//
// Phase 0 wildcard infrastructure (FX gap analysis, 2026-04-27):
//   See Docs/specs/2026-04-27-fx-engine-build-plan.md §3.1.
//   Pack 8 (Mastering) is the first consumer in Phase 2 — Phase 0 ships the
//   bus + smoke test only. No chain reads from it yet.
//
// Output model — 16 weights, one per mood:
//   Each Mood enum member maps to a normalized weight in [0, 1]. setMood() is
//   one-hot (target = 1.0 on selected, 0.0 elsewhere). setBlend() lets two
//   moods coexist for soft-blend continuous output (e.g. transitioning from
//   "Foundation" → "Atmosphere"). Sum of smoothed weights ≈ 1.0.
//
// Default behaviour:
//   Initialised to one-hot Foundation (index 0). Processor calls
//   setMoodByName(preset.mood) at preset load, so this default is only seen
//   in the brief window before the first preset loads.
//
// Opt-in at the consumer (locked decision D3, 2026-04-27):
//   The bus is a read-only mod source. Default-OFF behaviour lives on each
//   chain (e.g. Mastering's MASTER_MOOD_AWARE toggle). The bus does not gate
//   anything — chains decide whether to query it.
//
// Drift LFO (D005 — every engine must breathe):
//   setDriftRate() supports rates from 0.0001 Hz upward, fully covering the
//   ≤ 0.01 Hz "must breathe" floor. Output via getDriftValue() is a sine in
//   [-driftDepth, +driftDepth] that consumers can mix into their own modes
//   (e.g. mastering tilt EQ, mood-coupled reverb size).
//
// Audio-thread contract:
//   - get*() and advanceSmoothing() are O(1), lock-free.
//   - setMood / setBlend / setDrift* are message-thread; they update atomic
//     targets which the next advanceSmoothing() interpolates toward.
//   - prepare(sampleRate) must be called before drift LFO is meaningful.
//
class MoodModulationBus
{
public:
    static constexpr int NumMoods = 16;

    // Order MUST match the first 16 entries of validMoods in
    // Source/Core/PresetManager.h (excluding the "User" fallback bucket) and
    // CLAUDE.md's mood list.
    enum class Mood : int
    {
        Foundation  = 0,
        Atmosphere  = 1,
        Entangled   = 2,
        Prism       = 3,
        Flux        = 4,
        Aether      = 5,
        Family      = 6,
        Submerged   = 7,
        Coupling    = 8,
        Crystalline = 9,
        Deep        = 10,
        Ethereal    = 11,
        Kinetic     = 12,
        Luminous    = 13,
        Organic     = 14,
        Shadow      = 15
    };

    MoodModulationBus() noexcept
    {
        for (int i = 0; i < NumMoods; ++i)
        {
            targetWeights_[(size_t) i].store(0.0f, std::memory_order_relaxed);
            smoothedWeights_[(size_t) i] = 0.0f;
        }
        // Default to Foundation one-hot until a preset overrides via
        // setMoodByName(). Keeps consumers from seeing an all-zero state if
        // they read before the first preset load.
        targetWeights_[0].store(1.0f, std::memory_order_relaxed);
        smoothedWeights_[0] = 1.0f;
    }

    //-- Setup (message thread) ------------------------------------------------

    void prepare(double sampleRate, int /*blockSize*/) noexcept
    {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 48000.0;
    }

    //-- Configuration (message thread) ----------------------------------------

    // One-hot target on the selected mood.
    void setMood(Mood m) noexcept
    {
        const int idx = static_cast<int>(m);
        if (! validMood(idx)) return;
        for (int i = 0; i < NumMoods; ++i)
            targetWeights_[(size_t) i].store(i == idx ? 1.0f : 0.0f,
                                             std::memory_order_release);
    }

    // Set the target mood by canonical name (matches PresetData::mood
    // strings). Returns true on match. Unknown names ("User", empty, typos)
    // leave state unchanged.
    bool setMoodByName(const juce::String& moodName) noexcept
    {
        const int idx = moodIndexFromName(moodName);
        if (idx < 0) return false;
        setMood(static_cast<Mood>(idx));
        return true;
    }

    // Soft-blend between two moods. blend = 0 → pure a, blend = 1 → pure b.
    void setBlend(Mood a, Mood b, float blend) noexcept
    {
        const int ai = static_cast<int>(a);
        const int bi = static_cast<int>(b);
        if (! validMood(ai) || ! validMood(bi)) return;

        const float t  = juce::jlimit(0.0f, 1.0f, blend);
        const float wa = (ai == bi) ? 1.0f : (1.0f - t);
        const float wb = (ai == bi) ? 0.0f : t;

        for (int i = 0; i < NumMoods; ++i)
        {
            float w = 0.0f;
            if (i == ai) w += wa;
            if (i == bi) w += wb;
            targetWeights_[(size_t) i].store(w, std::memory_order_release);
        }
    }

    //-- Drift LFO (D005 must-breathe — supports ≤ 0.01 Hz) -------------------

    // 0 disables. Otherwise clamped to [0.0001, 4.0] Hz so the spec's
    // ≤ 0.01 Hz drift floor is comfortably reachable.
    void setDriftRate(float rateHz) noexcept
    {
        const float r = (rateHz <= 0.0f) ? 0.0f
                                         : juce::jlimit(0.0001f, 4.0f, rateHz);
        driftRateHz_.store(r, std::memory_order_release);
    }

    void setDriftDepth(float depth) noexcept
    {
        driftDepth_.store(juce::jlimit(0.0f, 1.0f, depth),
                          std::memory_order_release);
    }

    //-- Audio thread ----------------------------------------------------------

    // Smoothed weight for one mood. Sum across all moods ≈ 1.0.
    float getWeight(Mood m) const noexcept
    {
        const int idx = static_cast<int>(m);
        return validMood(idx) ? smoothedWeights_[(size_t) idx] : 0.0f;
    }

    float getWeight(int idx) const noexcept
    {
        return validMood(idx) ? smoothedWeights_[(size_t) idx] : 0.0f;
    }

    std::array<float, NumMoods> getAllWeights() const noexcept
    {
        std::array<float, NumMoods> out{};
        for (int i = 0; i < NumMoods; ++i)
            out[(size_t) i] = smoothedWeights_[(size_t) i];
        return out;
    }

    // Currently dominant mood (highest smoothed weight). Ties resolve to the
    // lower index.
    Mood getDominantMood() const noexcept
    {
        int   best  = 0;
        float bestW = smoothedWeights_[0];
        for (int i = 1; i < NumMoods; ++i)
        {
            if (smoothedWeights_[(size_t) i] > bestW)
            {
                best  = i;
                bestW = smoothedWeights_[(size_t) i];
            }
        }
        return static_cast<Mood>(best);
    }

    // Drift LFO sample, depth-scaled, in [-driftDepth, +driftDepth].
    float getDriftValue() const noexcept { return driftValue_; }

    // Advance per-mood smoothing and the drift LFO by one block. Must be
    // called once per processBlock from the audio thread.
    void advanceSmoothing(int blockSize) noexcept
    {
        constexpr float kSmoothingCoeff = 0.05f; // ~20 ms at 48 kHz / 256-spb

        for (int i = 0; i < NumMoods; ++i)
        {
            const float target = targetWeights_[(size_t) i].load(std::memory_order_acquire);
            smoothedWeights_[(size_t) i] += kSmoothingCoeff * (target - smoothedWeights_[(size_t) i]);
        }

        const float rate  = driftRateHz_.load(std::memory_order_acquire);
        const float depth = driftDepth_.load(std::memory_order_acquire);

        if (rate > 0.0f && depth > 0.0f && blockSize > 0)
        {
            const float dt    = static_cast<float>(blockSize) / static_cast<float>(sampleRate_);
            const float twoPi = juce::MathConstants<float>::twoPi;
            driftPhase_ += twoPi * rate * dt;
            while (driftPhase_ >  twoPi) driftPhase_ -= twoPi;
            while (driftPhase_ < -twoPi) driftPhase_ += twoPi;
            driftValue_ = depth * std::sin(driftPhase_);
        }
        else
        {
            driftValue_ = 0.0f;
        }
    }

private:
    static bool validMood(int idx) noexcept
    {
        return idx >= 0 && idx < NumMoods;
    }

    static int moodIndexFromName(const juce::String& name) noexcept
    {
        // Matches the first 16 entries of validMoods in PresetManager.h.
        // "User" and unknown strings return -1 (no-op on the bus).
        if (name == "Foundation")  return 0;
        if (name == "Atmosphere")  return 1;
        if (name == "Entangled")   return 2;
        if (name == "Prism")       return 3;
        if (name == "Flux")        return 4;
        if (name == "Aether")      return 5;
        if (name == "Family")      return 6;
        if (name == "Submerged")   return 7;
        if (name == "Coupling")    return 8;
        if (name == "Crystalline") return 9;
        if (name == "Deep")        return 10;
        if (name == "Ethereal")    return 11;
        if (name == "Kinetic")     return 12;
        if (name == "Luminous")    return 13;
        if (name == "Organic")     return 14;
        if (name == "Shadow")      return 15;
        return -1;
    }

    std::array<std::atomic<float>, NumMoods> targetWeights_;
    std::array<float, NumMoods>              smoothedWeights_;

    std::atomic<float> driftRateHz_ { 0.0f };
    std::atomic<float> driftDepth_  { 0.0f };
    float              driftPhase_  { 0.0f };
    float              driftValue_  { 0.0f };
    double             sampleRate_  { 48000.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoodModulationBus)
};

} // namespace xoceanus
