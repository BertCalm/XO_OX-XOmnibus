// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>
#include "../FastMath.h"

namespace xoceanus {

//==============================================================================
// SpectralTilt — Cascaded shelving filter that shifts spectral energy up or down.
//
// Single "tilt" knob: negative = dark (energy falls toward bass),
//                     positive = bright (energy rises toward treble).
// Uses 4 cascaded first-order shelving filters at staggered frequencies
// for a smooth, natural tilt curve. CPU: ~4 biquads = nearly free.
//
// Mix control for parallel blending with dry signal.
//==============================================================================
class SpectralTilt
{
public:
    SpectralTilt() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    /// tiltAmount: -1.0 (full dark) to +1.0 (full bright)
    // SRO: Dirty-flag coefficient caching — skip recalc if value unchanged
    void setTilt (float tiltAmount)
    {
        float clamped = std::clamp (tiltAmount, -1.0f, 1.0f);
        if (std::abs (clamped - tilt) > 0.0001f)
        {
            tilt = clamped;
            recalcCoeffs();
        }
    }

    /// mix: 0.0 = fully dry, 1.0 = fully wet
    void setMix (float m) { mix = std::clamp (m, 0.0f, 1.0f); }

    void processBlock (float* left, float* right, int numSamples)
    {
        if (std::abs (tilt) < 0.001f || mix < 0.001f)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            float dryL = left[i];
            float dryR = right[i];

            float wetL = left[i];
            float wetR = right[i];

            // Cascade through 4 shelving stages
            for (int s = 0; s < kNumStages; ++s)
            {
                // Left channel
                float inL = wetL;
                float outL = stages[s].b0 * inL + stages[s].b1 * stages[s].x1L
                           - stages[s].a1 * stages[s].y1L;
                stages[s].x1L = inL;
                stages[s].y1L = outL;
                wetL = outL;

                // Right channel
                float inR = wetR;
                float outR = stages[s].b0 * inR + stages[s].b1 * stages[s].x1R
                           - stages[s].a1 * stages[s].y1R;
                stages[s].x1R = inR;
                stages[s].y1R = outR;
                wetR = outR;
            }

            // Denormal protection
            wetL = flushDenormal (wetL);
            wetR = flushDenormal (wetR);

            left[i]  = dryL + mix * (wetL - dryL);
            right[i] = dryR + mix * (wetR - dryR);
        }
    }

    void reset()
    {
        for (auto& s : stages)
        {
            s.x1L = s.y1L = 0.0f;
            s.x1R = s.y1R = 0.0f;
        }
    }

private:
    static constexpr int kNumStages = 4;

    // Staggered center frequencies for smooth tilt
    static constexpr float kFreqs[kNumStages] = { 120.0f, 500.0f, 2000.0f, 8000.0f };

    struct ShelfStage
    {
        float b0 = 1.0f, b1 = 0.0f, a1 = 0.0f;
        float x1L = 0.0f, y1L = 0.0f;
        float x1R = 0.0f, y1R = 0.0f;
    };

    ShelfStage stages[kNumStages];
    float tilt = 0.0f;
    float mix  = 1.0f;
    double sr  = 44100.0;

    void recalcCoeffs()
    {
        // Each stage applies a first-order shelf.
        // Lower frequencies get negative gain when tilt > 0 (bright),
        // higher frequencies get positive gain.
        // The gain per stage is distributed so the overall curve is smooth.

        for (int s = 0; s < kNumStages; ++s)
        {
            // Map stage index to gain: stage 0 (low) gets -tilt, stage 3 (high) gets +tilt
            // Linearly interpolated across stages
            float normalized = static_cast<float> (s) / static_cast<float> (kNumStages - 1);
            float gainDb = tilt * 6.0f * (2.0f * normalized - 1.0f); // ±6dB per stage at extremes

            // SRO: dbToGain + fastTan replace std::pow/tan (per-block coeff calc)
            float A  = dbToGain (gainDb);
            float w0 = 2.0f * 3.14159265f * kFreqs[s] / static_cast<float> (sr);
            float t  = fastTan (w0 * 0.5f);

            // First-order shelving filter coefficients
            // High shelf when gain > 0 at high freq, low shelf when gain > 0 at low freq
            if (s < kNumStages / 2)
            {
                // Low shelf
                float alpha = (t - 1.0f) / (t + 1.0f);
                float beta  = (A * t - 1.0f) / (A * t + 1.0f);
                stages[s].b0 = (1.0f + beta) * 0.5f * A + (1.0f + alpha) * 0.5f;
                stages[s].b1 = (1.0f + beta) * 0.5f * A - (1.0f + alpha) * 0.5f;
                // Normalize
                float a0 = 1.0f;
                stages[s].a1 = alpha;
                stages[s].b0 /= a0;
                stages[s].b1 /= a0;
            }
            else
            {
                // High shelf
                float alpha = (t - 1.0f) / (t + 1.0f);
                stages[s].b0 = A * (1.0f + alpha) * 0.5f + (1.0f - alpha) * 0.5f;
                stages[s].b1 = A * (1.0f + alpha) * 0.5f - (1.0f - alpha) * 0.5f;
                stages[s].a1 = alpha;
            }
        }
    }

    // SRO: uses shared flushDenormal from FastMath.h
};

} // namespace xoceanus
