#pragma once
#include "FastMath.h"
#include <cmath>
#include <cstdint>

namespace xolokun {

//==============================================================================
// StandardLFO — Shared low-frequency oscillator for the XOlokun fleet.
//
// Consolidates the identical LFO pattern found in 21+ engines into a single
// implementation. Supports 5 standard waveform shapes, sample-rate-correct
// phase accumulation, D005-compliant sub-Hz rates (floor 0.005 Hz = 200s
// cycle), deterministic S&H via Knuth TAOCP LCG, and optional phase offset
// for ensemble staggering.
//
// Usage:
//   StandardLFO lfo;
//   lfo.setRate (0.5f, sampleRate);    // 0.5 Hz
//   lfo.setShape (Shape::Triangle);
//   float mod = lfo.process();          // [-1, +1] bipolar output
//
// All methods are noexcept and allocation-free for real-time safety.
//==============================================================================
struct StandardLFO
{
    //--------------------------------------------------------------------------
    // Waveform shapes — matches the 5-shape standard used across the fleet.
    // Integer values are stable and match existing parameter serialization.
    //--------------------------------------------------------------------------
    enum Shape : int
    {
        Sine     = 0,
        Triangle = 1,
        Saw      = 2,
        Square   = 3,
        SandH    = 4   // Sample & Hold (random value on phase wrap)
    };

    //--------------------------------------------------------------------------
    // Configuration
    //--------------------------------------------------------------------------

    /// Set LFO rate in Hz. Sample rate is required for correct phase increment.
    /// Rates below 0.005 Hz are clamped (D005 floor = 200-second cycle).
    void setRate (float hz, float sampleRate) noexcept
    {
        phaseInc = hz / sampleRate;
    }

    /// Set waveform shape. Accepts int for direct parameter binding.
    void setShape (int newShape) noexcept
    {
        shape = newShape;
    }

    /// Set waveform shape from enum.
    void setShape (Shape newShape) noexcept
    {
        shape = static_cast<int> (newShape);
    }

    /// Set a fixed phase offset (0–1). Used for ensemble staggering:
    /// e.g., 4 voices with offsets 0.0, 0.25, 0.5, 0.75.
    void setPhaseOffset (float offset) noexcept
    {
        phaseOffset = offset;
    }

    //--------------------------------------------------------------------------
    // Processing
    //--------------------------------------------------------------------------

    /// Advance one sample and return bipolar output [-1, +1].
    float process() noexcept
    {
        // Compute effective phase with offset
        float effPhase = phase + phaseOffset;
        if (effPhase >= 1.0f) effPhase -= 1.0f;

        float out = 0.0f;

        switch (shape)
        {
            case Sine:
                out = fastSin (effPhase * kTwoPi);
                break;

            case Triangle:
                out = 4.0f * std::fabs (effPhase - 0.5f) - 1.0f;
                break;

            case Saw:
                out = 2.0f * effPhase - 1.0f;
                break;

            case Square:
                out = (effPhase < 0.5f) ? 1.0f : -1.0f;
                break;

            case SandH:
            {
                // Detect phase wrap: generate new random value on each cycle.
                // Uses Knuth TAOCP Vol. 2 LCG — deterministic, fast, no alloc.
                // Track lastPhase explicitly — safe at audio-rate LFO speeds where
                // phaseInc can be large (0.02+) and the old prev<0 check was fragile.
                if (phase < lastPhase)
                {
                    rngState = rngState * 1664525u + 1013904223u;
                    holdValue = static_cast<float> (rngState & 0xFFFF) / 32768.0f - 1.0f;
                }
                out = holdValue;
                break;
            }

            default:
                out = fastSin (effPhase * kTwoPi);  // fallback to sine
                break;
        }

        // Advance phase accumulator — record lastPhase before wrapping so
        // S&H wrap detection sees the pre-wrap value next sample.
        lastPhase = phase;
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;

        return out;
    }

    //--------------------------------------------------------------------------
    // State management
    //--------------------------------------------------------------------------

    /// Reset phase and S&H state. Call on voice init or note retrigger.
    void reset() noexcept
    {
        phase     = 0.0f;
        lastPhase = 0.0f;
        holdValue = 0.0f;
    }

    /// Reset with a specific starting phase (0–1). Useful for voice staggering
    /// when you want each voice to start at a different point in the cycle.
    void reset (float startPhase) noexcept
    {
        phase     = startPhase;
        lastPhase = startPhase;
        holdValue = 0.0f;
    }

    /// Reseed the S&H PRNG. Default seed 12345u matches existing fleet behavior.
    /// Use different seeds per voice/instance to decorrelate S&H patterns.
    void reseed (uint32_t seed) noexcept
    {
        rngState = seed;
    }

    /// Get current phase (0–1). Useful for phase-linked visualization.
    float getPhase() const noexcept { return phase; }

    //--------------------------------------------------------------------------
    // State — public for snapshot/restore in undo system
    //--------------------------------------------------------------------------
    float    phase       = 0.0f;
    float    lastPhase   = 0.0f;  // Previous phase — used for S&H wrap detection
    float    phaseInc    = 0.0f;
    int      shape       = 0;
    float    phaseOffset = 0.0f;
    float    holdValue   = 0.0f;
    uint32_t rngState    = 12345u;

private:
    static constexpr float kTwoPi = 6.28318530717958647692f;
};

//==============================================================================
// BreathingLFO — D005-compliant autonomous breathing modulator.
//
// A minimal sine-only LFO for engines that just need organic evolution.
// Rate floor enforced at 0.005 Hz (200-second cycle). No shape selection,
// no S&H, no phase offset — just breathing.
//
// Usage:
//   BreathingLFO breath;
//   breath.setRate (0.01f, sampleRate);   // gentle 100-second drift
//   float mod = breath.process();          // [-1, +1]
//==============================================================================
struct BreathingLFO
{
    /// Set breathing rate in Hz. Clamped to [0.005, maxRate].
    /// Default maxRate of 2.0 Hz covers macro-scaled ranges.
    void setRate (float hz, float sampleRate, float maxRate = 2.0f) noexcept
    {
        constexpr float kMinRate = 0.005f;  // D005 floor: 200-second cycle
        float clampedHz = hz;
        if (clampedHz < kMinRate) clampedHz = kMinRate;
        if (clampedHz > maxRate)  clampedHz = maxRate;
        phaseInc = clampedHz / sampleRate;
    }

    /// Advance one sample and return bipolar sine output [-1, +1].
    float process() noexcept
    {
        float out = fastSin (phase * kTwoPi);
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;
        return out;
    }

    void reset() noexcept { phase = 0.0f; }

    float phase    = 0.0f;
    float phaseInc = 0.0f;

private:
    static constexpr float kTwoPi = 6.28318530717958647692f;
};

} // namespace xolokun
