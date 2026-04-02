#pragma once
// XOverworld BitCrusher — Sample-rate reduction + bit-depth quantization.
//
// Two independent lo-fi stages:
//   1. Sample-and-hold (rate reduction): holds the input sample for N frames,
//      where N = rateDivider. Reduces the effective sample rate by a factor of N.
//      N=1 is bypass (no hold). This simulates DAC aliasing of old chip hardware.
//
//   2. Bit depth quantization: maps the sample to the nearest level in a
//      2^bits-step uniform grid. bits=16 is transparent; bits=1 gives ±1 hard clip.
//
//   3. Wet/dry mix: preserves the original signal for blend control.
//
// No std::pow in the hot path — bit depth steps computed via fastPow2.

#include "../../../DSP/FastMath.h"

namespace xoverworld {

using namespace xoceanus;

class BitCrusher {
public:

    void reset() {
        holdCounter = 0;
        heldSample  = 0.0f;
    }

    // bits: 1.0–16.0 (16 = no quantization, 1 = 1-bit hard clip)
    void setBits(float b) {
        bits = b < 1.0f ? 1.0f : b > 16.0f ? 16.0f : b;
        // Compute quantization step: levels = 2^bits, step = 2 / levels
        // 2^bits via fastPow2; we store half the step for rounding
        float levels = fastPow2(bits);
        quantStep    = 2.0f / levels;       // step size in [-1, 1] range
        quantInv     = 1.0f / quantStep;    // for quantize: multiply, round, multiply-back
    }

    // rateDivider: 1 (bypass) to 32 (hold for 32 samples)
    void setRateDivider(int n) {
        rateDivider = n < 1 ? 1 : n > 32 ? 32 : n;
    }

    // mix: 0 = dry only, 1 = fully crushed
    void setMix(float m) {
        mix = m < 0.0f ? 0.0f : m > 1.0f ? 1.0f : m;
    }

    float process(float x) {
        // 1. Sample-and-hold (rate reduction)
        if (rateDivider > 1) {
            if (holdCounter <= 0) {
                heldSample  = x;
                holdCounter = rateDivider;
            }
            --holdCounter;
        } else {
            heldSample = x;
        }

        // 2. Bit-depth quantization
        // Map to grid: floor(x / step + 0.5) * step
        // Using multiplication by quantInv (= 1/step) to avoid division per sample
        float crushed;
        if (bits < 15.9f) {
            // Clamp input to [-1, 1] before quantizing to avoid grid overflow
            float clamped = heldSample < -1.0f ? -1.0f : heldSample > 1.0f ? 1.0f : heldSample;
            float scaled  = clamped * quantInv;
            float rounded = (float)(int)(scaled + (scaled >= 0.0f ? 0.5f : -0.5f));
            crushed = rounded * quantStep;
            // Reclamp after quantization (rounding can push exactly ±1 step over)
            crushed = crushed < -1.0f ? -1.0f : crushed > 1.0f ? 1.0f : crushed;
        } else {
            // bits close to 16 — quantization inaudible, skip for efficiency
            crushed = heldSample;
        }

        // 3. Mix (wet = crushed, dry = original input x)
        return x + mix * (crushed - x);
    }

private:
    float bits       = 16.0f;
    int   rateDivider = 1;
    float mix        = 0.0f;

    float quantStep  = 2.0f / 65536.0f; // 16-bit default
    float quantInv   = 65536.0f / 2.0f;

    int   holdCounter = 0;
    float heldSample  = 0.0f;
};

} // namespace xoverworld
