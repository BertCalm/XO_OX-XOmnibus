// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "OwlfishFastMath.h"
#include <cmath>
#include <vector>

namespace xowlfish
{

//==============================================================================
// AbyssReverb -- Dark algorithmic FDN reverb (ABYSS REVERB).
//
// A 4-line feedback delay network tuned for the bathypelagic zone.
// Fibonacci-adjacent prime delay lengths for maximum decorrelation.
// Heavy one-pole lowpass damping in each feedback path -- at max damping
// only sub-bass survives the tail. Extended pre-delay (20-200ms).
//
// Hadamard-like mixing matrix for energy-preserving diffusion.
// Stereo output: lines 0+1 to left, 2+3 to right.
// Mix is external -- caller blends wet/dry.
//
// All DSP inline. Denormal-safe. Buffers allocated in prepare().
//==============================================================================

class AbyssReverb
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;

        // -- Compute delay line lengths scaled to sample rate --
        constexpr int baseLengths[kNumLines] = {1597, 1879, 2281, 2659};
        float scale = static_cast<float>(sr) / 44100.0f;

        for (int i = 0; i < kNumLines; ++i)
        {
            int len = static_cast<int>(static_cast<float>(baseLengths[i]) * scale);
            if (len < 1)
                len = 1;
            delayLengths[i] = len;
            delayBuffers[i].assign(static_cast<size_t>(len), 0.0f);
            delayPos[i] = 0;
            filterState[i] = 0.0f;
        }

        // -- Pre-delay buffer: max 200ms --
        preDelayMaxLen = static_cast<int>(0.2f * static_cast<float>(sr));
        if (preDelayMaxLen < 1)
            preDelayMaxLen = 1;
        preDelayBuf.assign(static_cast<size_t>(preDelayMaxLen), 0.0f);
        preDelayPos = 0;
        preDelaySamples = 0;
    }

    void reset()
    {
        for (int i = 0; i < kNumLines; ++i)
        {
            std::fill(delayBuffers[i].begin(), delayBuffers[i].end(), 0.0f);
            delayPos[i] = 0;
            filterState[i] = 0.0f;
        }
        std::fill(preDelayBuf.begin(), preDelayBuf.end(), 0.0f);
        preDelayPos = 0;
    }

    /// Set parameters from normalized 0-1 values (call once per block).
    /// size01:     0 -> short tail (0.7 feedback), 1 -> near-infinite (0.995)
    /// damp01:     0 -> 20 kHz (bright), 1 -> 800 Hz (only sub-bass survives)
    /// preDelay01: 0 -> 20 ms, 1 -> 200 ms
    void setParams(float size01, float damp01, float preDelay01)
    {
        // Feedback coefficient: 0.7 to 0.995
        feedback = 0.7f + size01 * 0.295f;

        // Damping: one-pole LP coefficient from cutoff frequency
        // Map 0-1 -> 20000 Hz to 800 Hz (inverted: more damp = lower cutoff)
        float cutoffHz = 20000.0f - damp01 * 19200.0f;
        if (cutoffHz < 20.0f)
            cutoffHz = 20.0f;
        // One-pole LP coefficient: c = 1 - e^(-2*pi*fc/sr)
        constexpr float twoPi = 6.28318530717958647692f;
        dampCoeff = 1.0f - fastExp(-twoPi * cutoffHz / static_cast<float>(sr));
        if (dampCoeff > 1.0f)
            dampCoeff = 1.0f;
        if (dampCoeff < 0.0f)
            dampCoeff = 0.0f;

        // Pre-delay: 20ms to 200ms in samples
        float preDelayMs = 20.0f + preDelay01 * 180.0f;
        preDelaySamples = static_cast<int>(preDelayMs * 0.001f * static_cast<float>(sr));
        if (preDelaySamples >= preDelayMaxLen)
            preDelaySamples = preDelayMaxLen - 1;
        if (preDelaySamples < 0)
            preDelaySamples = 0;
    }

    /// Process one stereo sample pair through the reverb.
    void processSample(float inputL, float inputR, float& outL, float& outR)
    {
        // -- Pre-delay --
        float mono = (inputL + inputR) * 0.5f;

        preDelayBuf[preDelayPos] = mono;
        int readPos = preDelayPos - preDelaySamples;
        if (readPos < 0)
            readPos += preDelayMaxLen;
        float preDelayed = preDelayBuf[readPos];
        preDelayed = flushDenormal(preDelayed);
        preDelayPos = (preDelayPos + 1) % preDelayMaxLen;

        // -- Read current outputs from all 4 delay lines --
        float lineOut[kNumLines];
        for (int i = 0; i < kNumLines; ++i)
        {
            lineOut[i] = delayBuffers[i][delayPos[i]];
            lineOut[i] = flushDenormal(lineOut[i]);
        }

        // -- Hadamard-like mixing matrix (4x4) --
        // [+,+,+,+]  [+,-,+,-]  [+,+,-,-]  [+,-,-,+]  / 2
        float mixed[kNumLines];
        mixed[0] = (lineOut[0] + lineOut[1] + lineOut[2] + lineOut[3]) * 0.5f;
        mixed[1] = (lineOut[0] - lineOut[1] + lineOut[2] - lineOut[3]) * 0.5f;
        mixed[2] = (lineOut[0] + lineOut[1] - lineOut[2] - lineOut[3]) * 0.5f;
        mixed[3] = (lineOut[0] - lineOut[1] - lineOut[2] + lineOut[3]) * 0.5f;

        // -- Write back into delay lines with LP damping --
        for (int i = 0; i < kNumLines; ++i)
        {
            // One-pole lowpass in feedback path (the key darkening character)
            float input = preDelayed + mixed[i] * feedback;
            filterState[i] += dampCoeff * (input - filterState[i]);
            filterState[i] = flushDenormal(filterState[i]);

            delayBuffers[i][delayPos[i]] = filterState[i];
            delayPos[i] = (delayPos[i] + 1) % delayLengths[i];
        }

        // -- Stereo output: lines 0+1 left, 2+3 right --
        outL = (lineOut[0] + lineOut[1]) * 0.5f;
        outR = (lineOut[2] + lineOut[3]) * 0.5f;
    }

private:
    static constexpr int kNumLines = 4;

    std::vector<float> delayBuffers[kNumLines];
    int delayLengths[kNumLines] = {};
    int delayPos[kNumLines] = {};
    float filterState[kNumLines] = {};

    std::vector<float> preDelayBuf;
    int preDelayPos = 0;
    int preDelaySamples = 0;
    int preDelayMaxLen = 0;

    float feedback = 0.85f;
    float dampCoeff = 0.3f;
    double sr = 44100.0;
};

} // namespace xowlfish
