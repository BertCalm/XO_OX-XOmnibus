// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>

#include "OxytocinThermal.h"    // for fastTanh (defined there as shared header)
#include "../../DSP/FastMath.h" // for xoceanus::fastSin

/// OxytocinDrive — MS-20 Sallen-Key asymmetric saturation model.
///
/// Transfer function:
///   y = fastTanh(x * (1 + drive)) + 0.12 * fastTanh(3 * x * drive)
/// The second term emphasises the MS-20's characteristic odd harmonics.
///
/// Self-oscillation "scream" activates when passion > 0.9: a sine tone at
/// the filter cutoff frequency bleeds in, amplitude scaled by (passion-0.9)*10.
///
/// With circuit_age > 0: a small DC bias drifts in from aging components.
///
/// PERF-1: std::tanh replaced with fastTanh (Pade [3/3] approximation)
///         defined in OxytocinThermal.h.

class OxytocinDrive
{
public:
    OxytocinDrive() = default;

    void prepare(double sampleRate) noexcept
    {
        jassert(sampleRate > 0.0); // P1-7
        if (sampleRate <= 0.0) return;
        sr = sampleRate;
        screamPhase = 0.0f;
    }

    void reset() noexcept { screamPhase = 0.0f; }

    /// Process one sample.
    /// \param input        dry audio sample
    /// \param passion      0..1 passion level (determines drive amount)
    /// \param cutoffHz     filter cutoff frequency for self-oscillation
    /// \param circuitAge   0..1 aging amount
    float processSample(float input, float passion, float cutoffHz, float circuitAge) noexcept
    {
        const float driveAmount = passion * 4.0f; // 0 = clean, 4 = heavy

        // Main transfer function — PERF-1: fastTanh replaces std::tanh
        float y = fastTanh(input * (1.0f + driveAmount)) + 0.12f * fastTanh(3.0f * input * driveAmount);

        // Self-oscillation: "the scream"
        if (passion > 0.9f)
        {
            float screamAmp = (passion - 0.9f) * 10.0f;
            float screamFreq = std::max(20.0f, std::min(cutoffHz, 18000.0f));
            screamPhase += static_cast<float>(juce::MathConstants<double>::twoPi * screamFreq / sr);
            if (screamPhase > juce::MathConstants<float>::twoPi)
                screamPhase -= juce::MathConstants<float>::twoPi;
            y += screamAmp * xoceanus::fastSin(screamPhase) * 0.15f; // PERF: fastSin replaces std::sin
        }

        // Aging DC bias drift
        if (circuitAge > 0.0f)
            y += circuitAge * 0.02f;

        return y;
    }

private:
    double sr = 0.0; // P1-7: default 0
    float screamPhase = 0.0f;
};
