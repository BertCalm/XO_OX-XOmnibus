// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>
#include "../../DSP/FastMath.h"

namespace xocelot
{

// BitCrusher — bit depth quantization + sample rate decimation.
// SP-1200 style: quantize to 2^bits levels, hold every Nth sample.

struct BitCrusher
{
    float holdSample = 0.0f;
    float holdCounter = 0.0f;
    float cachedBitDepth = -1.0f; // forces recompute on first call
    float cachedLevels = 1.0f;
    float cachedInvLevels = 1.0f;

    // bitDepth: 4.0–16.0 (float for smooth automation)
    // targetRate: target sample rate in Hz (4000–44100)
    // sr: actual sample rate
    float process(float x, float bitDepth, float targetRate, float sr)
    {
        // Sample rate decimation: hold-and-skip
        float ratio = sr / std::max(targetRate, 100.0f);
        holdCounter += 1.0f;
        if (holdCounter >= ratio)
        {
            holdCounter -= ratio;
            // Bit depth quantization — cache 2^bitDepth across calls.
            // std::pow on the audio thread was ~50× slower than fastPow2.
            if (bitDepth != cachedBitDepth)
            {
                cachedBitDepth = bitDepth;
                cachedLevels = xoceanus::fastPow2(std::clamp(bitDepth, 1.0f, 24.0f));
                cachedInvLevels = 1.0f / cachedLevels;
            }
            holdSample = std::round(x * cachedLevels) * cachedInvLevels;
        }
        return holdSample;
    }

    void reset()
    {
        holdSample = 0.0f;
        holdCounter = 0.0f;
        cachedBitDepth = -1.0f;
        cachedLevels = 1.0f;
        cachedInvLevels = 1.0f;
    }
};

} // namespace xocelot
