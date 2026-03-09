#pragma once
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include "../FastMath.h"

namespace xomnibus {

//==============================================================================
// LushReverb — Algorithmic stereo reverb (Schroeder-Moorer architecture).
//
// 8 parallel comb filters feed into 4 series allpass filters, producing
// dense, smooth tails with controllable room size and damping.
//
// Features:
//   - Room size, damping, stereo width, wet/dry mix
//   - Pre-delay line (0–100ms)
//   - 8 comb filters with prime-number-based delay lengths
//   - 4 series allpass diffusers
//   - Denormal protection on all comb/allpass state
//
// Usage:
//   LushReverb reverb;
//   reverb.prepare(44100.0);
//   reverb.setRoomSize(0.7f);
//   reverb.setDamping(0.3f);
//   reverb.setWidth(1.0f);
//   reverb.setMix(0.25f);
//   reverb.processBlock(inL, inR, outL, outR, numSamples);
//==============================================================================
class LushReverb
{
public:
    LushReverb() = default;

    //--------------------------------------------------------------------------
    /// Prepare the reverb for playback. Allocates all internal buffers.
    /// @param sampleRate  Current sample rate in Hz.
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        float srScale = static_cast<float> (sr / 44100.0);

        // Comb filter delay lengths (prime numbers, scaled to sample rate)
        // These primes are chosen to minimize common harmonics and produce
        // a smooth, dense reverb tail.
        static constexpr int kCombBaseLengths[kNumCombs] = {
            1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617
        };

        for (int i = 0; i < kNumCombs; ++i)
        {
            int len = static_cast<int> (static_cast<float> (kCombBaseLengths[i]) * srScale) + 1;
            combBuffers[i].assign (static_cast<size_t> (len), 0.0f);
            combPos[i] = 0;
            combFilterState[i] = 0.0f;
        }

        // Allpass filter delay lengths (also prime-based)
        static constexpr int kAllpassBaseLengths[kNumAllpasses] = {
            556, 441, 341, 225
        };

        for (int i = 0; i < kNumAllpasses; ++i)
        {
            int len = static_cast<int> (static_cast<float> (kAllpassBaseLengths[i]) * srScale) + 1;
            allpassBuffers[i].assign (static_cast<size_t> (len), 0.0f);
            allpassPos[i] = 0;

            // Right channel allpass with slightly different lengths for decorrelation
            int lenR = len + static_cast<int> (stereoSpread * 0.7f);
            allpassBuffersR[i].assign (static_cast<size_t> (lenR), 0.0f);
            allpassPosR[i] = 0;
        }

        // Pre-delay buffer: max 100ms
        int preDelayLen = static_cast<int> (sr * 0.1) + 1;
        preDelayBuffer.assign (static_cast<size_t> (preDelayLen), 0.0f);
        preDelayPos = 0;
        preDelayMaxLen = preDelayLen;

        // Stereo offset for right channel comb reads (23 samples @ 44.1k gives decorrelation)
        stereoSpread = static_cast<int> (23.0f * srScale);
    }

    //--------------------------------------------------------------------------
    /// Set room size. 0.0 = small room, 1.0 = large hall.
    void setRoomSize (float size)
    {
        roomSize = clamp (size, 0.0f, 1.0f);
        // Map room size to feedback gain: [0.0, 1.0] -> [0.5, 0.98]
        combFeedback = 0.5f + roomSize * 0.48f;
    }

    /// Set high-frequency damping. 0.0 = bright, 1.0 = very dark.
    void setDamping (float damp)
    {
        damping = clamp (damp, 0.0f, 1.0f);
        // Damping coefficient for 1-pole LP in comb feedback
        dampCoeff = damping * 0.4f;
    }

    /// Set stereo width. 0.0 = mono, 1.0 = full stereo.
    void setWidth (float w)
    {
        width = clamp (w, 0.0f, 1.0f);
        wet1 = width * 0.5f + 0.5f;
        wet2 = (1.0f - width) * 0.5f;
    }

    /// Set wet/dry mix. 0.0 = fully dry, 1.0 = fully wet.
    void setMix (float wet)
    {
        mix = clamp (wet, 0.0f, 1.0f);
    }

    /// Set pre-delay time in milliseconds. Clamped to [0, 100]ms.
    void setPreDelay (float ms)
    {
        preDelayMs = clamp (ms, 0.0f, 100.0f);
        preDelaySamples = static_cast<int> (static_cast<double> (preDelayMs) * 0.001 * sr);
        if (preDelaySamples >= preDelayMaxLen)
            preDelaySamples = preDelayMaxLen - 1;
        if (preDelaySamples < 0)
            preDelaySamples = 0;
    }

    //--------------------------------------------------------------------------
    /// Process a block of stereo audio through the reverb.
    /// Input and output buffers may alias (in-place processing is safe).
    void processBlock (float* leftIn, float* rightIn,
                       float* leftOut, float* rightOut, int numSamples)
    {
        // Guard: if prepare() hasn't been called, pass audio through unchanged
        if (combBuffers[0].empty())
        {
            if (leftIn != leftOut)
                std::copy(leftIn, leftIn + numSamples, leftOut);
            if (rightIn != rightOut)
                std::copy(rightIn, rightIn + numSamples, rightOut);
            return;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = leftIn[i];
            float inR = rightIn[i];

            // Sum to mono for reverb input, with input scaling
            float monoIn = (inL + inR) * 0.5f;

            // Pre-delay
            float preDelayed = monoIn;
            if (preDelaySamples > 0)
            {
                int readPos = (preDelayPos - preDelaySamples + preDelayMaxLen) % preDelayMaxLen;
                preDelayed = preDelayBuffer[static_cast<size_t> (readPos)];
                preDelayBuffer[static_cast<size_t> (preDelayPos)] = monoIn;
                preDelayPos = (preDelayPos + 1) % preDelayMaxLen;
            }

            // Accumulate comb filter outputs (8 parallel combs)
            float combOutL = 0.0f;
            float combOutR = 0.0f;

            for (int c = 0; c < kNumCombs; ++c)
            {
                int len = static_cast<int> (combBuffers[c].size());
                int pos = combPos[c];

                // Read from comb delay line
                float readVal = flushDenormal (combBuffers[c][static_cast<size_t> (pos)]);

                // One-pole lowpass in feedback path for damping
                combFilterState[c] = flushDenormal (
                    readVal * (1.0f - dampCoeff) + combFilterState[c] * dampCoeff
                );

                // Write back with input and feedback
                combBuffers[c][static_cast<size_t> (pos)] =
                    preDelayed + combFilterState[c] * combFeedback;

                // Accumulate to L/R using stereo spread offset
                combOutL += readVal;

                // Right channel reads from an offset position for decorrelation
                int rightPos = (pos + stereoSpread) % len;
                combOutR += flushDenormal (combBuffers[c][static_cast<size_t> (rightPos)]);

                combPos[c] = (pos + 1) % len;
            }

            // Scale comb outputs
            combOutL /= static_cast<float> (kNumCombs);
            combOutR /= static_cast<float> (kNumCombs);

            // 4 series allpass filters for diffusion.
            // Left and right channels are processed through separate allpass
            // buffer sets to produce a symmetric, diffused reverb tail.
            float allpassOutL = combOutL;
            float allpassOutR = combOutR;

            for (int a = 0; a < kNumAllpasses; ++a)
            {
                int len = static_cast<int> (allpassBuffers[a].size());
                int pos = allpassPos[a];

                // Process left through allpass
                float bufValL = flushDenormal (allpassBuffers[a][static_cast<size_t> (pos)]);
                float apOutL = -allpassOutL + bufValL;
                allpassBuffers[a][static_cast<size_t> (pos)] =
                    allpassOutL + bufValL * kAllpassFeedback;
                allpassOutL = apOutL;

                // Process right through separate allpass buffer set
                int lenR = static_cast<int> (allpassBuffersR[a].size());
                int posR = allpassPosR[a];

                float bufValR = flushDenormal (allpassBuffersR[a][static_cast<size_t> (posR)]);
                float apOutR = -allpassOutR + bufValR;
                allpassBuffersR[a][static_cast<size_t> (posR)] =
                    allpassOutR + bufValR * kAllpassFeedback;
                allpassOutR = apOutR;

                allpassPos[a] = (pos + 1) % len;
                allpassPosR[a] = (posR + 1) % lenR;
            }

            // Apply stereo width matrix
            float wetL = allpassOutL * wet1 + allpassOutR * wet2;
            float wetR = allpassOutR * wet1 + allpassOutL * wet2;

            // Mix dry/wet
            leftOut[i]  = inL * (1.0f - mix) + wetL * mix;
            rightOut[i] = inR * (1.0f - mix) + wetR * mix;
        }
    }

    //--------------------------------------------------------------------------
    /// Reset all reverb state without reallocating buffers.
    void reset()
    {
        for (int i = 0; i < kNumCombs; ++i)
        {
            std::fill (combBuffers[i].begin(), combBuffers[i].end(), 0.0f);
            combPos[i] = 0;
            combFilterState[i] = 0.0f;
        }

        for (int i = 0; i < kNumAllpasses; ++i)
        {
            std::fill (allpassBuffers[i].begin(), allpassBuffers[i].end(), 0.0f);
            allpassPos[i] = 0;
            std::fill (allpassBuffersR[i].begin(), allpassBuffersR[i].end(), 0.0f);
            allpassPosR[i] = 0;
        }

        std::fill (preDelayBuffer.begin(), preDelayBuffer.end(), 0.0f);
        preDelayPos = 0;
    }

private:
    static constexpr int kNumCombs = 8;
    static constexpr int kNumAllpasses = 4;
    static constexpr float kAllpassFeedback = 0.5f;

    double sr = 44100.0;

    // Comb filters
    std::vector<float> combBuffers[kNumCombs];
    int combPos[kNumCombs] {};
    float combFilterState[kNumCombs] {};
    float combFeedback = 0.84f;

    // Allpass diffusers (separate L/R buffer sets for symmetric stereo)
    std::vector<float> allpassBuffers[kNumAllpasses];
    int allpassPos[kNumAllpasses] {};
    std::vector<float> allpassBuffersR[kNumAllpasses];
    int allpassPosR[kNumAllpasses] {};

    // Pre-delay
    std::vector<float> preDelayBuffer;
    int preDelayPos = 0;
    int preDelayMaxLen = 0;
    int preDelaySamples = 0;
    float preDelayMs = 0.0f;

    // Parameters
    float roomSize = 0.7f;
    float damping = 0.3f;
    float dampCoeff = 0.12f;
    float width = 1.0f;
    float mix = 0.25f;

    // Stereo width coefficients
    float wet1 = 1.0f;
    float wet2 = 0.0f;

    // Stereo decorrelation offset
    int stereoSpread = 23;
};

} // namespace xomnibus
