#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include "FastMath.h"

namespace xomnibus {

//==============================================================================
// FDNReverb — 4-tap Feedback Delay Network with Householder matrix.
//
// A minimal FDN suitable for in-engine and FX-chain reverb. Delay lengths are
// scaled to the actual sample rate at prepare() time. Feedback is LP-filtered
// per tap (water absorbs high frequencies). Householder 4×4 matrix guarantees
// unit energy reflection with decorrelation.
//
// Delay constants (prime-number based, ≈ room sizes 24–40 ms @ 44.1 kHz):
//   { 1087, 1283, 1511, 1789 } samples @ 44100 Hz
//
// Usage:
//   FDNReverb rev;
//   rev.prepare(sampleRate);
//   // per-sample:
//   float reverbL, reverbR;
//   rev.processSample(inputMono, size, damping, feedback, reverbL, reverbR);
//==============================================================================
class FDNReverb
{
public:
    FDNReverb() = default;

    //--------------------------------------------------------------------------
    void prepare (double sampleRate) noexcept
    {
        sr = static_cast<float> (sampleRate);
        const float srScale = sr / 44100.0f;
        static constexpr int kBaseLengths[4] = { 1087, 1283, 1511, 1789 };
        for (int i = 0; i < 4; ++i)
        {
            int len = static_cast<int> (static_cast<float> (kBaseLengths[i]) * srScale) + 1;
            buf[i].assign (static_cast<size_t> (len), 0.0f);
            pos[i] = 0;
            filtState[i] = 0.0f;
        }
    }

    //--------------------------------------------------------------------------
    void reset() noexcept
    {
        for (int i = 0; i < 4; ++i)
        {
            std::fill (buf[i].begin(), buf[i].end(), 0.0f);
            pos[i] = 0;
            filtState[i] = 0.0f;
        }
    }

    //--------------------------------------------------------------------------
    /// Process one sample through the FDN.
    /// @param input     Mono input signal to inject (typically (L+R)*0.5*density).
    /// @param size      Room size in [0, 1] — scales the effective delay length.
    /// @param damping   High-frequency damping in [0, 1].
    /// @param feedback  Feedback amount in [0, 1] — clamped to [0, 0.95] internally.
    /// @param outL/outR Decorrelated stereo outputs (not mixed with dry).
    void processSample (float input, float size, float damping, float feedback,
                        float& outL, float& outR) noexcept
    {
        const float fb = 0.3f + std::min (feedback, 0.95f) * 0.65f;

        // Read taps with LP damping
        float tap[4];
        for (int i = 0; i < 4; ++i)
        {
            const int len = static_cast<int> (buf[i].size());
            if (len == 0) { tap[i] = 0.0f; continue; }

            int readOffset = static_cast<int> ((0.3f + size * 0.7f) * static_cast<float> (len));
            if (readOffset < 1) readOffset = 1;
            if (readOffset > len - 1) readOffset = len - 1;
            const int readPos = (pos[i] - readOffset + len) % len;
            tap[i] = buf[i][static_cast<size_t> (readPos)];

            // One-pole LP in feedback path
            const float damp = damping * 0.85f;
            filtState[i] += (tap[i] - filtState[i]) * (1.0f - damp);
            tap[i] = filtState[i];
        }

        // Householder 4×4 feedback matrix: output_i = tap_i - 0.5 * sum(all taps)
        const float tapSum = tap[0] + tap[1] + tap[2] + tap[3];
        for (int i = 0; i < 4; ++i)
        {
            float fbSample = (tap[i] - 0.5f * tapSum) * fb + input;
            fbSample = flushDenormal (fbSample);
            fbSample = fastTanh (fbSample); // soft clip prevents runaway

            const int len = static_cast<int> (buf[i].size());
            if (len > 0)
                buf[i][static_cast<size_t> (pos[i])] = fbSample;
            pos[i] = (pos[i] + 1) % std::max (1, len);
        }

        // Decorrelated stereo mix from tap pairs (opposite signs for width)
        outL = tap[0] * 0.6f + tap[1] * 0.4f - tap[2] * 0.3f + tap[3] * 0.1f;
        outR = tap[0] * 0.1f - tap[1] * 0.3f + tap[2] * 0.4f + tap[3] * 0.6f;
    }

private:
    float sr = 44100.0f;
    std::vector<float> buf[4];
    int pos[4] {};
    float filtState[4] {};
};

} // namespace xomnibus
