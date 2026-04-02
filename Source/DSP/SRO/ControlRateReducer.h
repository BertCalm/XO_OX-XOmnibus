// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../FastMath.h"

namespace xoceanus {

//==============================================================================
// ControlRateReducer — Decimates modulation signals to control rate with
// linear interpolation back to audio rate.
//
// Coupling and modulation signals (LFO→param, envelope→cutoff, etc.) rarely
// need audio-rate precision. Running them at 1/32 or 1/64 of the sample rate
// saves significant CPU while linear interpolation prevents zipper artifacts.
//
// Usage:
//     ControlRateReducer<64> reducer;  // update every 64 samples
//     reducer.prepare(sampleRate);
//
//     // In renderBlock():
//     for (int i = 0; i < numSamples; ++i)
//     {
//         if (reducer.shouldUpdate(i))
//         {
//             float newValue = expensiveModulationCalc();
//             reducer.pushValue(newValue);
//         }
//         float smoothed = reducer.getInterpolated(i);
//         applyModulation(smoothed);
//     }
//
// Design:
//   - Template parameter sets the decimation ratio (must be power of 2)
//   - Zero allocation — all state is inline
//   - Linear interpolation between control points prevents zipper noise
//   - Block-aligned variant for processing entire coupling blocks at once
//
// Topology note: At ratio N with linear interpolation, this is functionally
// a sample-and-hold with slew limiting — the topology Buchla called "source
// of uncertainty" (Model 266). Analog synths achieved this naturally through
// CV line capacitance. The smoothing is not a compromise; it is a modulation
// character that mirrors the organic feel of voltage-controlled systems.
//==============================================================================
template <int Ratio = 32>
class ControlRateReducer
{
    static_assert ((Ratio & (Ratio - 1)) == 0, "Ratio must be a power of 2");
    static_assert (Ratio >= 2 && Ratio <= 256, "Ratio must be in [2, 256]");

public:
    void prepare (double /*sampleRate*/) noexcept
    {
        reset();
    }

    void reset() noexcept
    {
        currentValue = 0.0f;
        targetValue  = 0.0f;
        increment    = 0.0f;
        counter      = 0;
    }

    //--------------------------------------------------------------------------
    /// Returns true if the modulation source should be evaluated at this sample.
    bool shouldUpdate (int sampleIndex) const noexcept
    {
        return (sampleIndex & (Ratio - 1)) == 0;
    }

    /// Push a new control-rate value. Call when shouldUpdate() returns true.
    void pushValue (float newValue) noexcept
    {
        currentValue = targetValue;
        targetValue  = newValue;
        increment    = (targetValue - currentValue) * kInvRatio;
        counter      = 0;
    }

    /// Get the linearly interpolated value at the current sample position.
    float getInterpolated (int /*sampleIndex*/) noexcept
    {
        float result = currentValue + increment * static_cast<float> (counter);
        counter++;
        return result;
    }

    //--------------------------------------------------------------------------
    /// Block-level convenience: decimates an input modulation buffer in-place.
    /// Reads every Ratio-th sample, linearly interpolates the rest.
    /// @param buffer  Modulation signal buffer (numSamples long). Modified in-place.
    /// @param numSamples  Number of samples in the buffer.
    void processBlock (float* buffer, int numSamples) noexcept
    {
        if (numSamples <= 0) return;

        // Capture first sample before any writes to prevent aliasing if
        // the compiler reorders loads/stores during SIMD vectorization.
        const float firstSample = buffer[0];
        float prev = firstSample;
        for (int i = 0; i < numSamples; i += Ratio)
        {
            float next = buffer[juce_min (i + Ratio, numSamples - 1)];
            float step = (next - prev) * kInvRatio;
            int end = juce_min (i + Ratio, numSamples);
            for (int j = i; j < end; ++j)
                buffer[j] = prev + step * static_cast<float> (j - i);
            prev = next;
        }
    }

    /// Block-level: reduce a source signal into a destination buffer.
    /// Source is read at control rate, destination is filled with interpolated values.
    /// @param source     Full-rate modulation source (numSamples).
    /// @param dest       Output buffer for reduced/interpolated signal (numSamples).
    /// @param numSamples Number of samples.
    void processBlock (const float* source, float* dest, int numSamples) noexcept
    {
        if (numSamples <= 0) return;

        float prev = source[0];
        for (int i = 0; i < numSamples; i += Ratio)
        {
            int nextIdx = i + Ratio;
            if (nextIdx >= numSamples) nextIdx = numSamples - 1;
            float next = source[nextIdx];
            float step = (next - prev) * kInvRatio;
            int end = (i + Ratio < numSamples) ? i + Ratio : numSamples;
            for (int j = i; j < end; ++j)
                dest[j] = prev + step * static_cast<float> (j - i);
            prev = next;
        }
    }

    static constexpr int getRatio() noexcept { return Ratio; }

private:
    static constexpr float kInvRatio = 1.0f / static_cast<float> (Ratio);

    float currentValue = 0.0f;
    float targetValue  = 0.0f;
    float increment    = 0.0f;
    int counter        = 0;

    // Minimal helper to avoid juce dependency in this header
    static constexpr int juce_min (int a, int b) noexcept { return a < b ? a : b; }
};

} // namespace xoceanus
