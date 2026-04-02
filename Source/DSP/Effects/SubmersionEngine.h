// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include "../FastMath.h"

namespace xoceanus {

//==============================================================================
// SubmersionEngine — Variable-stage APF cascade + micro-looper + master clock.
//
// Mashup: Chase Bliss MOOD MKII × OBNE Bathing
//
// A. Liminal Wet Channel: 2-12 all-pass filters in series, LFO-modulated.
// B. Phase-Warped Micro-Looper: 2-5 second buffer in the feedback path.
//    Every repeat becomes more phase-smeared.
// C. Master Submersion Clock: Global resampling that slows/speeds everything.
//    Lowering the clock pitches audio down + scales APF LFO frequency.
//
// CPU-optimized: max 8 APF stages (not 12), 3s looper (not 5s).
// CC46 = APF Stages, CC47 = Master Clock.
//==============================================================================
class SubmersionEngine
{
public:
    SubmersionEngine() = default;

    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);

        // APF cascade (max 8 stages)
        for (int i = 0; i < kMaxAPFStages; ++i)
        {
            apfState[i] = 0.0f;
            apfDelay[i] = 0.0f;
        }
        apfLFOPhase = 0.0f;

        // Micro-looper: 3 seconds
        int loopLen = static_cast<int> (sr * 3.0f) + 1;
        loopBufL.assign (static_cast<size_t> (loopLen), 0.0f);
        loopBufR.assign (static_cast<size_t> (loopLen), 0.0f);
        loopWritePos = 0;
        loopReadPhase = 0.0f;
        loopLength = loopLen;

        // Master clock resampling state
        clockPhase = 0.0f;
        prevSampleL = 0.0f;
        prevSampleR = 0.0f;
    }

    void reset()
    {
        for (int i = 0; i < kMaxAPFStages; ++i)
        {
            apfState[i] = 0.0f;
            apfDelay[i] = 0.0f;
        }
        apfLFOPhase = 0.0f;
        std::fill (loopBufL.begin(), loopBufL.end(), 0.0f);
        std::fill (loopBufR.begin(), loopBufR.end(), 0.0f);
        loopWritePos = 0;
        loopReadPhase = 0.0f;
        clockPhase = 0.0f;
        prevSampleL = prevSampleR = 0.0f;
    }

    void processBlock (float* L, float* R, int numSamples,
                       int numStages, float lfoRate, float lfoDepth,
                       float loopFeedback, float masterClock, float mix)
    {
        if (mix < 0.001f) return;

        // Clamp stages: 2, 4, or 8
        int stages = (numStages <= 2) ? 2 : (numStages <= 4) ? 4 : 8;

        // Master clock scales everything: 0.25 to 2.0
        float clockRate = 0.25f + masterClock * 1.75f;

        // Scaled LFO rate
        float effectiveLFORate = lfoRate * clockRate;
        float lfoInc = effectiveLFORate / sr;

        int loopBufSize = static_cast<int> (loopBufL.size());

        for (int s = 0; s < numSamples; ++s)
        {
            float dryL = L[s], dryR = R[s];

            // === Master Clock: resampling ===
            // At clockRate < 1.0, we read fewer samples → pitch down + aliasing
            // At clockRate > 1.0, we read more → pitch up
            clockPhase += clockRate;
            bool processThisSample = (clockPhase >= 1.0f);
            if (clockPhase >= 1.0f) clockPhase -= 1.0f;

            float wetL, wetR;
            if (processThisSample)
            {
                float inputL = dryL;

                // === A. APF Cascade ===
                float lfoVal = fastSin (apfLFOPhase * 6.28318f) * lfoDepth;
                apfLFOPhase += lfoInc;
                if (apfLFOPhase >= 1.0f) apfLFOPhase -= 1.0f;

                // Process through N all-pass filters
                float apfOutL = inputL;
                for (int i = 0; i < stages && i < kMaxAPFStages; ++i)
                {
                    // All-pass coefficient modulated by LFO
                    // Each stage gets a slightly different mod phase
                    float stagePhase = lfoVal * (1.0f + static_cast<float> (i) * 0.15f);
                    float coeff = 0.5f + stagePhase * 0.3f;
                    coeff = std::max (-0.95f, std::min (0.95f, coeff));

                    // First-order all-pass: y = coeff * (x - y_prev) + x_prev
                    float x = apfOutL;
                    float y = coeff * (x - apfState[i]) + apfDelay[i];
                    apfDelay[i] = x;
                    apfState[i] = flushDenormal (y);
                    apfOutL = y;
                }

                // === B. Micro-Looper (in feedback path) ===
                if (loopBufSize > 1 && loopFeedback > 0.001f)
                {
                    // Write APF output + looper feedback into the loop buffer
                    int readPos = (loopWritePos - loopBufSize + 1 + loopBufSize) % loopBufSize;
                    float loopRead = loopBufL[static_cast<size_t> (readPos)];

                    // Feed looper output back into APF input (phase smearing)
                    float loopWrite = apfOutL + loopRead * loopFeedback;
                    loopWrite = fastTanh (loopWrite); // prevent runaway
                    loopBufL[static_cast<size_t> (loopWritePos)] = flushDenormal (loopWrite);
                    loopWritePos = (loopWritePos + 1) % loopBufSize;

                    apfOutL = apfOutL * 0.7f + loopRead * loopFeedback * 0.3f;
                }

                wetL = apfOutL;
                wetR = apfOutL * 0.8f; // slight stereo offset

                prevSampleL = wetL;
                prevSampleR = wetR;
            }
            else
            {
                // ZOH: hold previous sample when clock doesn't tick
                wetL = prevSampleL;
                wetR = prevSampleR;
            }

            L[s] = dryL * (1.0f - mix) + wetL * mix;
            R[s] = dryR * (1.0f - mix) + wetR * mix;
        }
    }

private:
    float sr = 44100.0f;

    // APF cascade
    static constexpr int kMaxAPFStages = 8;
    float apfState[kMaxAPFStages] {};
    float apfDelay[kMaxAPFStages] {};
    float apfLFOPhase = 0.0f;

    // Micro-looper
    std::vector<float> loopBufL, loopBufR;
    int loopWritePos = 0;
    float loopReadPhase = 0.0f;
    int loopLength = 0;

    // Master clock
    float clockPhase = 0.0f;
    float prevSampleL = 0.0f;
    float prevSampleR = 0.0f;
};

} // namespace xoceanus
