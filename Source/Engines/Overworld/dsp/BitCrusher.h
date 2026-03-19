#pragma once
#include <cmath>
#include <algorithm>

namespace xoverworld {

//==============================================================================
// BitCrusher — Bit-depth reduction + sample-rate decimation
//
// Models the lo-fi degradation of classic chip hardware:
//   - Bit reduction:  quantise to N bits (1–16); default 16 = transparent
//   - Rate reduction: hold each sample for R samples (1 = transparent)
//   - Wet/dry mix:    blend crushed signal with clean input
//
// Parameters (set before calling process()):
//   bits        float [1, 16] — target bit depth (float for smooth macro control)
//   rateDivider int   [1, 32] — sample-hold divider (1 = off, 32 = extreme decimation)
//   mix         float [0, 1]  — 0 = dry, 1 = fully crushed
//
// Usage:
//   crusher.reset();
//   crusher.setBits(8.0f);
//   crusher.setRateDivider(2);
//   crusher.setMix(0.7f);
//   float out = crusher.process(in);
//==============================================================================
class BitCrusher
{
public:
    BitCrusher() = default;

    void reset()
    {
        held      = 0.0f;
        holdCount = 0;
    }

    void setBits(float b)
    {
        bits = std::max(1.0f, std::min(16.0f, b));
    }

    void setRateDivider(int r)
    {
        rateDivider = std::max(1, std::min(32, r));
    }

    void setMix(float m)
    {
        mix = std::max(0.0f, std::min(1.0f, m));
    }

    float process(float x)
    {
        // Sample-rate reduction: hold
        ++holdCount;
        if (holdCount >= rateDivider)
        {
            holdCount = 0;
            // Bit-depth reduction: quantise
            const float levels = std::exp2(bits) - 1.0f; // 2^bits - 1 steps
            held = std::round(x * levels) / levels;
        }

        return x + (held - x) * mix;
    }

private:
    float bits        = 16.0f;
    int   rateDivider = 1;
    float mix         = 0.0f;

    float held        = 0.0f;
    int   holdCount   = 0;
};

} // namespace xoverworld
