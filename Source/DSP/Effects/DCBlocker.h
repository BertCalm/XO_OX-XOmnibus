// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include "../FastMath.h"

namespace xoceanus
{

//==============================================================================
// DCBlocker — End-of-chain DC offset removal.
//
// A 1st-order high-pass filter at ~10 Hz that removes DC offset without
// audibly affecting bass content. Uses the standard DC blocking topology:
//
//   y[n] = x[n] - x[n-1] + R * y[n-1]
//
// where R = 1 - (2π × cutoff / sampleRate), typically ~0.9986 at 44.1kHz.
//
// This sits as the absolute final processing stage — after the limiter,
// before the output. Any upstream effect (saturation, comb filters,
// frequency shifting) can inject DC; this catches it all.
//
// CPU budget: ~0.01% (2 multiplies + 2 adds per sample, stereo)
//
// Usage:
//   DCBlocker dc;
//   dc.prepare(44100.0);
//   dc.processBlock(L, R, numSamples);
//==============================================================================
class DCBlocker
{
public:
    DCBlocker() = default;

    //--------------------------------------------------------------------------
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        updateCoefficient();
        reset();
    }

    //--------------------------------------------------------------------------
    void processBlock(float* L, float* R, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            // Left channel
            float xL = L[i];
            float yL = xL - prevInL + coeff * prevOutL;
            prevInL = xL;
            prevOutL = flushDenormal(yL);
            L[i] = yL;

            // Right channel
            float xR = R[i];
            float yR = xR - prevInR + coeff * prevOutR;
            prevInR = xR;
            prevOutR = flushDenormal(yR);
            R[i] = yR;
        }
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        prevInL = prevInR = 0.0f;
        prevOutL = prevOutR = 0.0f;
    }

private:
    void updateCoefficient()
    {
        // coeff = 1 - (2π × cutoffHz / sampleRate)
        // ~10 Hz cutoff: inaudible, catches all DC
        constexpr float cutoffHz = 10.0f;
        coeff = 1.0f - (6.2831853f * cutoffHz / static_cast<float>(sr));
        coeff = clamp(coeff, 0.9f, 0.9999f);
    }

    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float coeff = 0.9986f;

    // State
    float prevInL = 0.0f, prevInR = 0.0f;
    float prevOutL = 0.0f, prevOutR = 0.0f;
};

} // namespace xoceanus
