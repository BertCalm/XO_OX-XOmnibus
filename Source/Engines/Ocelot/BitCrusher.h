#pragma once
#include <cmath>
#include <algorithm>
#include "../../DSP/FastMath.h"

namespace xocelot {

// BitCrusher — bit depth quantization + sample rate decimation.
// SP-1200 style: quantize to 2^bits levels, hold every Nth sample.

struct BitCrusher
{
    float holdSample = 0.0f;
    float holdCounter = 0.0f;

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
            // Bit depth quantization
            float levels = xomnibus::fastPow2(std::clamp(bitDepth, 1.0f, 24.0f));
            holdSample = std::round(x * levels) / levels;
        }
        return holdSample;
    }

    void reset() { holdSample = 0.0f; holdCounter = 0.0f; }
};

} // namespace xocelot
