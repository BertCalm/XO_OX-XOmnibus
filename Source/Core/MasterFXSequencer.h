// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <cstdint>
#include <atomic>
#include "../DSP/FastMath.h"

namespace xoceanus {

//==============================================================================
// MasterFXSequencer — Step-sequenced parameter modulation for the Master FX chain.
//
// A non-audio modulation engine that rhythmically animates Master FX parameters.
// Runs at block rate (not per-sample) for minimal CPU overhead.
//
// Features:
//   - 8 algorithmic patterns (Pulse, Ramp, Triangle, Euclidean, Random, Scatter, etc.)
//   - 1-16 step lengths
//   - BPM-synced via host transport (PPQ position for sample-accurate sync)
//   - Two simultaneous modulation targets
//   - Smooth/glide interpolation between steps
//   - Envelope follower mode (input dynamics drive modulation)
//   - Cross-modulation: envelope follower can modulate sequencer depth
//   - Zero CPU when disabled
//
// Inspired by: Torso S-4, Chase Bliss Automatone, Meris internal mod bus
//
// Usage:
//   MasterFXSequencer seq;
//   seq.prepare(44100.0);
//   seq.setEnabled(true);
//   seq.setPattern(Pattern::Triangle);
//   seq.setSteps(8);
//   seq.setTarget1(Target::ReverbMix);
//   seq.setTarget2(Target::DelayFeedback);
//   seq.updateBlock(ppqPosition, bpm, numSamples, inputRMS);
//   float mod1 = seq.getModValue1();  // → apply as offset to target param
//   float mod2 = seq.getModValue2();
//==============================================================================
class MasterFXSequencer
{
public:
    /// Sequencer patterns
    enum class Pattern
    {
        Pulse = 0,       // Alternating high/low (sidechain pump)
        RampUp,          // Linear ramp 0→1 over step count
        RampDown,        // Linear ramp 1→0 over step count
        Triangle,        // Up then down (sweeping filter feel)
        Euclidean3,      // 3 hits distributed across N steps
        Euclidean5,      // 5 hits distributed across N steps
        RandomWalk,      // Brownian motion (new random offset per step)
        Scatter,         // Probability-based triggers (sparse, musical)
        NumPatterns
    };

    /// Modulation targets
    enum class Target
    {
        None = 0,
        SatDrive,
        DelayFeedback,
        DelayMix,
        ReverbMix,
        ModDepth,
        ModRate,
        CompMix,
        SpectralTilt,      // Spectral tilt amount
        TransientAttack,   // Transient designer attack
        DopplerDistance,    // Doppler distance
        GranularSmear,     // Granular smear amount
        ExciterDrive,      // Harmonic exciter drive
        StereoWidth,       // Stereo sculptor mid width
        PsychoWidth,       // Psychoacoustic widener width
        VibeKnob,          // Vibe knob (bipolar grit/sweet)
        OsmosisMix,        // fXOsmosis membrane mix
        OneiricMix,        // fXOneiric dream state mix
        NumTargets
    };

    /// Clock divisions
    enum class ClockDiv
    {
        Whole = 0,       // 1/1 = 4 beats per step
        Half,            // 1/2 = 2 beats per step
        Quarter,         // 1/4 = 1 beat per step
        Eighth,          // 1/8
        Sixteenth,       // 1/16
        ThirtySecond,    // 1/32
        DottedEighth,
        TripletQuarter,
        NumDivisions
    };

    MasterFXSequencer() = default;

    //--------------------------------------------------------------------------
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        currentStep = 0;
        modValue1 = 0.0f;
        modValue2 = 0.0f;
        smoothedMod1 = 0.0f;
        smoothedMod2 = 0.0f;
        prevPPQ = -1.0;
        randomWalkState = 0.5f;
        envFollowerState = 0.0f;
        randState = 0x12345678;
    }

    //--------------------------------------------------------------------------
    void setEnabled (bool on)         { enabled = on; }
    void setPattern (Pattern p)       { pattern = p; }
    void setSteps (int n)             { steps = clamp (n, 1, kMaxSteps); }
    void setDepth (float d)           { depth = clamp (d, 0.0f, 1.0f); }
    void setSmooth (float s)          { smooth = clamp (s, 0.0f, 1.0f); }
    void setRate (ClockDiv div)       { clockDiv = div; }
    void setTarget1 (Target t)        { target1 = t; }
    void setTarget2 (Target t)        { target2 = t; }
    void setEnvFollowEnabled (bool on) { envFollowEnabled = on; }
    void setEnvFollowAmount (float a) { envFollowAmount = clamp (a, 0.0f, 1.0f); }

    //--------------------------------------------------------------------------
    /// Call once per audio block. Advances the sequencer and computes mod values.
    /// @param ppqPosition  Current PPQ position from AudioPlayHead
    /// @param bpm          Current BPM from host transport
    /// @param numSamples   Block size (for envelope follower smoothing)
    /// @param inputRMS     RMS level of the input signal (for envelope follower)
    void updateBlock (double ppqPosition, double bpm, int numSamples, float inputRMS)
    {
        if (sr <= 0.0)
        {
            modValue1 = modValue2 = smoothedMod1 = smoothedMod2 = 0.0f;
            return;
        }
        if (!enabled)
        {
            modValue1 = 0.0f;
            modValue2 = 0.0f;
            smoothedMod1 = 0.0f;
            smoothedMod2 = 0.0f;
            return;
        }

        // --- Envelope follower ---
        float envMod = 0.0f;
        if (envFollowEnabled)
        {
            // One-pole follower with ~50ms attack, ~200ms release
            float attackCoeff = 1.0f - fastExp (-1.0f / (0.05f * static_cast<float> (sr)));
            float releaseCoeff = 1.0f - fastExp (-1.0f / (0.2f * static_cast<float> (sr)));

            float target = inputRMS;
            float coeff = (target > envFollowerState) ? attackCoeff : releaseCoeff;

            // Approximate block-rate update
            for (int i = 0; i < numSamples; ++i)
                envFollowerState = flushDenormal (envFollowerState + coeff * (target - envFollowerState));

            envMod = envFollowerState * envFollowAmount;
        }

        // --- Step advancement via PPQ ---
        float beatsPerStep = computeBeatsPerStep();
        int newStep = currentStep;

        if (bpm > 0.0 && ppqPosition >= 0.0)
        {
            // Absolute step from PPQ position — no cumulative drift
            double stepsFromPPQ = ppqPosition / static_cast<double> (beatsPerStep);
            newStep = static_cast<int> (std::fmod (stepsFromPPQ, static_cast<double> (steps)));
            if (newStep < 0) newStep = 0;
        }
        else
        {
            // Free-running fallback: advance by block time
            freeRunAccumulator += static_cast<float> (numSamples) / static_cast<float> (sr);
            float stepDuration = beatsPerStep * 60.0f / 120.0f;  // assume 120 BPM fallback
            if (freeRunAccumulator >= stepDuration)
            {
                freeRunAccumulator -= stepDuration;
                newStep = (currentStep + 1) % steps;
            }
        }

        // Generate step on change
        if (newStep != currentStep)
        {
            currentStep = newStep;
            if (pattern == Pattern::RandomWalk)
                advanceRandomWalk();
        }

        // --- Compute raw pattern value for current step ---
        float raw = computePatternValue (currentStep);

        // Apply envelope follower cross-modulation to depth
        float effectiveDepth = depth;
        if (envFollowEnabled)
            effectiveDepth = clamp (depth + envMod, 0.0f, 1.0f);

        float targetMod1 = raw * effectiveDepth;
        float targetMod2 = raw * effectiveDepth;

        // --- Smooth interpolation ---
        // Smooth 0 = snap (instant), smooth 1 = slow glide
        float smoothCoeff = (smooth < 0.01f)
            ? 1.0f
            : clamp (1.0f - fastExp (-static_cast<float> (numSamples) / (smooth * 0.5f * static_cast<float> (sr))),
                     0.001f, 1.0f);

        smoothedMod1 = flushDenormal (smoothedMod1 + smoothCoeff * (targetMod1 - smoothedMod1));
        smoothedMod2 = flushDenormal (smoothedMod2 + smoothCoeff * (targetMod2 - smoothedMod2));

        modValue1 = (target1 != Target::None) ? smoothedMod1 : 0.0f;
        modValue2 = (target2 != Target::None) ? smoothedMod2 : 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Get current modulation output for target 1. Range: [0, 1]
    float getModValue1() const { return modValue1; }

    /// Get current modulation output for target 2. Range: [0, 1]
    float getModValue2() const { return modValue2; }

    /// Get which parameter target 1 modulates
    Target getTarget1() const { return target1; }

    /// Get which parameter target 2 modulates
    Target getTarget2() const { return target2; }

    /// Get current step (for UI display)
    int getCurrentStep() const { return currentStep; }

    /// Is the sequencer enabled?
    bool isEnabled() const { return enabled; }

    //--------------------------------------------------------------------------
    void reset()
    {
        currentStep = 0;
        modValue1 = modValue2 = 0.0f;
        smoothedMod1 = smoothedMod2 = 0.0f;
        randomWalkState = 0.5f;
        envFollowerState = 0.0f;
        freeRunAccumulator = 0.0f;
        prevPPQ = -1.0;
    }

private:
    //--------------------------------------------------------------------------
    float computeBeatsPerStep() const
    {
        switch (clockDiv)
        {
            case ClockDiv::Whole:            return 4.0f;
            case ClockDiv::Half:             return 2.0f;
            case ClockDiv::Quarter:          return 1.0f;
            case ClockDiv::Eighth:           return 0.5f;
            case ClockDiv::Sixteenth:        return 0.25f;
            case ClockDiv::ThirtySecond:     return 0.125f;
            case ClockDiv::DottedEighth:     return 0.75f;
            case ClockDiv::TripletQuarter:   return 2.0f / 3.0f;
            default:                         return 1.0f;
        }
    }

    //--------------------------------------------------------------------------
    float computePatternValue (int step) const
    {
        float pos = static_cast<float> (step) / static_cast<float> (steps);

        switch (pattern)
        {
            case Pattern::Pulse:
                return (step % 2 == 0) ? 1.0f : 0.0f;

            case Pattern::RampUp:
                return pos;

            case Pattern::RampDown:
                return 1.0f - pos;

            case Pattern::Triangle:
            {
                float t = pos * 2.0f;
                return (t <= 1.0f) ? t : (2.0f - t);
            }

            case Pattern::Euclidean3:
                return euclideanHit (step, 3, steps) ? 1.0f : 0.0f;

            case Pattern::Euclidean5:
                return euclideanHit (step, 5, steps) ? 1.0f : 0.0f;

            case Pattern::RandomWalk:
                return randomWalkState;

            case Pattern::Scatter:
            {
                // Deterministic scatter based on step index
                // Uses a hash to decide if step triggers
                uint32_t hash = static_cast<uint32_t> (step) * 2654435761u;
                float threshold = static_cast<float> (hash & 0xFF) / 255.0f;
                return (threshold < 0.35f) ? 1.0f : 0.0f;  // ~35% density
            }

            default:
                return 0.0f;
        }
    }

    //--------------------------------------------------------------------------
    /// Bjorklund's Euclidean rhythm algorithm (O(1) per step)
    static bool euclideanHit (int step, int hits, int totalSteps)
    {
        if (totalSteps <= 0 || hits <= 0) return false;
        if (hits >= totalSteps) return true;

        // The Euclidean pattern: step is a hit if
        // floor(step * hits / totalSteps) != floor((step-1) * hits / totalSteps)
        // This distributes 'hits' as evenly as possible across 'totalSteps'
        int current = (step * hits) / totalSteps;
        int previous = ((step > 0 ? step - 1 : totalSteps - 1) * hits) / totalSteps;
        return current != previous;
    }

    //--------------------------------------------------------------------------
    void advanceRandomWalk()
    {
        // Brownian motion: small random step from current position
        float step = (nextRandom() - 0.5f) * 0.3f;
        randomWalkState = clamp (randomWalkState + step, 0.0f, 1.0f);
    }

    float nextRandom()
    {
        randState ^= randState << 13;
        randState ^= randState >> 17;
        randState ^= randState << 5;
        return static_cast<float> (randState & 0x7FFFFF) / static_cast<float> (0x7FFFFF);
    }

    //--------------------------------------------------------------------------
    static constexpr int kMaxSteps = 16;

    double sr = 0.0;

    // State
    int currentStep = 0;
    float modValue1 = 0.0f;
    float modValue2 = 0.0f;
    float smoothedMod1 = 0.0f;
    float smoothedMod2 = 0.0f;
    double prevPPQ = -1.0;
    float freeRunAccumulator = 0.0f;
    float randomWalkState = 0.5f;
    float envFollowerState = 0.0f;
    uint32_t randState = 0x12345678;

    // Parameters
    bool enabled = false;
    Pattern pattern = Pattern::Pulse;
    int steps = 8;
    float depth = 0.5f;
    float smooth = 0.3f;
    ClockDiv clockDiv = ClockDiv::Quarter;
    Target target1 = Target::None;
    Target target2 = Target::None;
    bool envFollowEnabled = false;
    float envFollowAmount = 0.5f;

    // Clamp helpers — separate int and float overloads to prevent implicit float→int
    // truncation (e.g. clamp(0.5f, 0.0f, 1.0f) must not truncate to 0).
    static int clamp (int x, int lo, int hi)
    {
        return (x < lo) ? lo : ((x > hi) ? hi : x);
    }
    static float clamp (float x, float lo, float hi)
    {
        return (x < lo) ? lo : ((x > hi) ? hi : x);
    }
};

} // namespace xoceanus
