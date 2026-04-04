// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>
#include "../../DSP/FastMath.h"

namespace xocelot
{

// ModalResonator — 2nd-order IIR bandpass resonator.
// Building block for physical percussion models (agogo, kalimba, pandeiro, log drum).
// Implements: y[n] = b0*x[n] + a1*y[n-1] + a2*y[n-2]
// where a1 = 2*r*cos(2*pi*f/sr), a2 = -(r*r), b0 = 1/Q.
// Pole radius r = exp(-pi * bw / sr), bw = freq/Q — finite decay, never self-oscillates.

struct ModalResonator
{
    float y1 = 0.0f, y2 = 0.0f;
    float a1 = 0.0f, a2 = -1.0f, b0 = 0.0f;
    float gain = 1.0f;

    void setParams(float freq, float q, float g, float sr)
    {
        constexpr float kPi = 3.14159265358979323846f;
        freq = std::clamp(freq, 20.0f, sr * 0.45f);
        q = std::max(0.5f, q);
        // Compute pole radius for finite decay: r = exp(-pi * bw / sr), bw = freq / Q
        const float bw = freq / q;
        const float r  = std::exp(-kPi * bw / sr);
        a1 = 2.0f * r * xoceanus::fastCos(2.0f * kPi * freq / sr);
        a2 = -(r * r);  // was implicitly -1.0 (unit circle — marginally stable)
        b0 = 1.0f / q;
        gain = g;
    }

    float process(float x)
    {
        float y = b0 * x + a1 * y1 + a2 * y2;
        y2 = y1;
        y1 = y;
        // Denormal flush
        if (std::abs(y1) < 1.0e-37f)
            y1 = 0.0f;
        if (std::abs(y2) < 1.0e-37f)
            y2 = 0.0f;
        return y * gain;
    }

    void reset() { y1 = y2 = 0.0f; }
};

} // namespace xocelot
