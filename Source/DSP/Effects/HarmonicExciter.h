// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>
#include "../FastMath.h"

namespace xoceanus {

//==============================================================================
// HarmonicExciter — Parallel saturation on isolated high-frequency content.
//
// Extracts high-frequency content via HPF, applies soft-clip saturation to
// generate harmonics, then blends back into the signal. Adds "air" and
// presence that EQ cannot achieve.
//
// Controls:
//   drive:  0..1 — saturation amount on HF content
//   freq:   1000..12000 — HPF cutoff for extraction
//   tone:   0..1 — post-saturation brightness (LPF on harmonics)
//   mix:    0..1 — blend of excited harmonics back into signal
//
// CPU: 2 biquads (HPF + tone LPF) + soft clip = trivial.
//==============================================================================
class HarmonicExciter
{
public:
    HarmonicExciter() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    void setDrive (float d) { drive = std::clamp (d, 0.0f, 1.0f); }
    // SRO: Dirty-flag coefficient caching — skip recalc if value unchanged
    void setFreq (float f)
    {
        float clamped = std::clamp (f, 1000.0f, 12000.0f);
        if (std::abs (clamped - hpfFreq) > 0.01f)
        {
            hpfFreq = clamped;
            recalcHPF();
        }
    }
    void setTone (float t)
    {
        float clamped = std::clamp (t, 0.0f, 1.0f);
        if (std::abs (clamped - tone) > 0.0001f)
        {
            tone = clamped;
            recalcToneLPF();
        }
    }
    void setMix (float m)   { mix = std::clamp (m, 0.0f, 1.0f); }

    void processBlock (float* left, float* right, int numSamples)
    {
        if (mix < 0.001f || drive < 0.001f)
            return;

        const float driveGain = 1.0f + drive * 15.0f; // 1x to 16x gain into clipper

        for (int i = 0; i < numSamples; ++i)
        {
            // Extract HF via 2nd-order HPF (left)
            float hfL = processHPF (left[i], hpfStateL);
            float hfR = processHPF (right[i], hpfStateR);

            // Drive into soft clipper
            hfL = softClip (hfL * driveGain);
            hfR = softClip (hfR * driveGain);

            // Tone control: LPF on the harmonics
            hfL = processToneLPF (hfL, toneStateL);
            hfR = processToneLPF (hfR, toneStateR);

            // Blend harmonics back (additive, not replacing)
            left[i]  += mix * hfL;
            right[i] += mix * hfR;
        }

        flushStates();
    }

    void reset()
    {
        hpfStateL = {};
        hpfStateR = {};
        toneStateL = {};
        toneStateR = {};
        recalcHPF();
        recalcToneLPF();
    }

private:
    double sr = 44100.0;
    float drive   = 0.3f;
    float hpfFreq = 3500.0f;
    float tone    = 0.7f;
    float mix     = 1.0f;

    // 2nd-order HPF state
    struct BiquadState
    {
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    };

    struct BiquadCoeffs
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    };

    BiquadState hpfStateL, hpfStateR;
    BiquadCoeffs hpfCoeffs;

    BiquadState toneStateL, toneStateR;
    BiquadCoeffs toneCoeffs;

    void recalcHPF()
    {
        // SRO: fastSin/fastCos replace std:: trig (per-setter coefficient calc)
        float w0 = 2.0f * 3.14159265f * hpfFreq / static_cast<float> (sr);
        float cosW0 = fastCos (w0);
        float sinW0 = fastSin (w0);
        float alpha = sinW0 / (2.0f * 0.707f); // Q = 0.707

        float a0 = 1.0f + alpha;
        hpfCoeffs.b0 = ((1.0f + cosW0) / 2.0f) / a0;
        hpfCoeffs.b1 = (-(1.0f + cosW0)) / a0;
        hpfCoeffs.b2 = ((1.0f + cosW0) / 2.0f) / a0;
        hpfCoeffs.a1 = (-2.0f * cosW0) / a0;
        hpfCoeffs.a2 = (1.0f - alpha) / a0;
    }

    void recalcToneLPF()
    {
        // Tone maps 0..1 to 2kHz..16kHz
        float freq = 2000.0f + tone * 14000.0f;
        // SRO: fastSin/fastCos replace std:: trig (per-setter coefficient calc)
        float w0 = 2.0f * 3.14159265f * freq / static_cast<float> (sr);
        float cosW0 = fastCos (w0);
        float sinW0 = fastSin (w0);
        float alpha = sinW0 / (2.0f * 0.707f);

        float a0 = 1.0f + alpha;
        toneCoeffs.b0 = ((1.0f - cosW0) / 2.0f) / a0;
        toneCoeffs.b1 = (1.0f - cosW0) / a0;
        toneCoeffs.b2 = ((1.0f - cosW0) / 2.0f) / a0;
        toneCoeffs.a1 = (-2.0f * cosW0) / a0;
        toneCoeffs.a2 = (1.0f - alpha) / a0;
    }

    float processHPF (float in, BiquadState& s)
    {
        float out = hpfCoeffs.b0 * in + hpfCoeffs.b1 * s.x1 + hpfCoeffs.b2 * s.x2
                  - hpfCoeffs.a1 * s.y1 - hpfCoeffs.a2 * s.y2;
        s.x2 = s.x1; s.x1 = in;
        s.y2 = s.y1; s.y1 = out;
        return out;
    }

    float processToneLPF (float in, BiquadState& s)
    {
        float out = toneCoeffs.b0 * in + toneCoeffs.b1 * s.x1 + toneCoeffs.b2 * s.x2
                  - toneCoeffs.a1 * s.y1 - toneCoeffs.a2 * s.y2;
        s.x2 = s.x1; s.x1 = in;
        s.y2 = s.y1; s.y1 = out;
        return out;
    }

    static float softClip (float x)
    {
        if (x > 1.0f) return 2.0f / 3.0f;
        if (x < -1.0f) return -2.0f / 3.0f;
        return x - (x * x * x) / 3.0f;
    }

    // SRO: Use shared flushDenormal from FastMath.h
    void flushStates()
    {
        auto flush = [] (BiquadState& s) {
            s.x1 = flushDenormal (s.x1); s.x2 = flushDenormal (s.x2);
            s.y1 = flushDenormal (s.y1); s.y2 = flushDenormal (s.y2);
        };
        flush (hpfStateL); flush (hpfStateR);
        flush (toneStateL); flush (toneStateR);
    }
};

} // namespace xoceanus
