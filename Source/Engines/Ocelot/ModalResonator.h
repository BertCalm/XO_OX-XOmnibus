// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>

namespace xocelot
{

// ModalResonator — 2nd-order IIR bandpass resonator.
// Building block for physical percussion models (agogo, kalimba, pandeiro, log drum).
// Implements: y[n] = b0*x[n] + a1*y[n-1] - y[n-2]
// where a1 = 2*cos(2*pi*f/sr), b0 = 1/Q.

struct ModalResonator
{
    float y1 = 0.0f, y2 = 0.0f;
    float a1 = 0.0f, b0 = 0.0f;
    float gain = 1.0f;

    void setParams(float freq, float q, float g, float sr)
    {
        constexpr float kPi = 3.14159265358979323846f;
        freq = std::clamp(freq, 20.0f, sr * 0.45f);
        q = std::max(0.5f, q);
        a1 = 2.0f * std::cos(2.0f * kPi * freq / sr);
        b0 = 1.0f / q;
        gain = g;
    }

    float process(float x)
    {
        float y = b0 * x + a1 * y1 - y2;
        y2 = y1;
        y1 = y;
        // Denormal flush
        if (std::abs(y1) < 1.0e-10f)
            y1 = 0.0f;
        if (std::abs(y2) < 1.0e-10f)
            y2 = 0.0f;
        return y * gain;
    }

    void reset() { y1 = y2 = 0.0f; }
};

} // namespace xocelot
