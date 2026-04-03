// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "OwlfishFastMath.h"
#include <cmath>

namespace xowlfish
{

//==============================================================================
// SubharmonicOsc -- Mixtur-Trautonium subharmonic oscillator (ABYSS HABITAT).
//
// Generates frequencies BELOW the fundamental via fractional frequency division.
// 4 subharmonic generators with selectable 1:2 through 1:8 division ratios.
// Mixtur waveshaping blends clean layering into soft-clip inter-modulation growl.
// A separate non-MIDI-tracked body sine provides sub-bass habitat pressure.
//
// All DSP inline. No allocations. Denormal-safe.
//==============================================================================

class SubharmonicOsc
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    void reset()
    {
        fundPhase = 0.0f;
        for (int i = 0; i < 4; ++i)
            subPhase[i] = 0.0f;
        bodyPhase = 0.0f;
    }

    /// Set parameters from snapshot (call once per block).
    void setParams(int div1, int div2, int div3, int div4, float level1, float level2, float level3, float level4,
                   float subMix, float mixtur, float fundWave, float subWave, float bodyFreq, float bodyLevel)
    {
        // Map division choice index -> actual divisor (0=off, 1->2, 2->3, ... 7->8)
        divisions[0] = divIndexToDivisor(div1);
        divisions[1] = divIndexToDivisor(div2);
        divisions[2] = divIndexToDivisor(div3);
        divisions[3] = divIndexToDivisor(div4);

        levels[0] = level1;
        levels[1] = level2;
        levels[2] = level3;
        levels[3] = level4;

        subMixAmt = subMix;
        mixturAmt = mixtur;
        fundWaveBlend = fundWave;
        subWaveBlend = subWave;
        bodyFreqHz = bodyFreq;
        bodyLevelAmt = bodyLevel;
    }

    /// Process one sample. freq = current MIDI frequency after portamento.
    float processSample(float freq)
    {
        constexpr float twoPi = 6.28318530717958647692f;
        float invSr = static_cast<float>(1.0 / sr);

        // -- Fundamental oscillator --
        float fundInc = freq * invSr;
        fundPhase += fundInc;
        if (fundPhase >= 1.0f)
            fundPhase -= 1.0f;
        fundPhase = flushDenormal(fundPhase);

        // Fundamental waveform: sine <-> triangle blend
        float fundSine = fastSin(fundPhase * twoPi);
        float fundTri = 4.0f * std::fabs(fundPhase - 0.5f) - 1.0f;
        float fundOut = lerp(fundSine, fundTri, fundWaveBlend);

        // -- 4 Subharmonic generators --
        float subSum = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            if (divisions[i] == 0)
                continue; // off

            float subInc = freq / static_cast<float>(divisions[i]) * invSr;
            subPhase[i] += subInc;
            if (subPhase[i] >= 1.0f)
                subPhase[i] -= 1.0f;
            subPhase[i] = flushDenormal(subPhase[i]);

            // Sub waveform: triangle <-> saw blend
            float subTri = 4.0f * std::fabs(subPhase[i] - 0.5f) - 1.0f;
            float subSaw = 2.0f * subPhase[i] - 1.0f;
            float subOut = lerp(subTri, subSaw, subWaveBlend);

            subSum += subOut * levels[i];
        }

        // -- Mixtur waveshaping --
        // Low mixtur: clean subharmonic layering
        // High mixtur: soft-clip inter-modulation growl
        float clean = subSum;
        float shaped = fastTanh(clean * 2.0f);
        float subFinal = lerp(clean, shaped, mixturAmt);

        // -- Sub-bass body --
        // Fixed-frequency sine (NOT MIDI-tracked). Habitat pressure.
        float bodyInc = bodyFreqHz * invSr;
        bodyPhase += bodyInc;
        if (bodyPhase >= 1.0f)
            bodyPhase -= 1.0f;
        bodyPhase = flushDenormal(bodyPhase);

        float bodyOut = fastSin(bodyPhase * twoPi) * bodyLevelAmt;

        // -- Mix fundamental and subharmonics --
        // subMix: 0 = fundamental only, 1 = subharmonics only
        float mixed = lerp(fundOut, subFinal, subMixAmt);

        // Add body on top (always present when bodyLevel > 0)
        return mixed + bodyOut;
    }

private:
    // Phase accumulators
    float fundPhase = 0.0f;
    float subPhase[4] = {};
    float bodyPhase = 0.0f;
    double sr = 44100.0;

    // Cached params
    int divisions[4] = {0, 0, 0, 0}; // 0=off, 2-8 = actual divisor
    float levels[4] = {};
    float subMixAmt = 0.5f;
    float mixturAmt = 0.0f;
    float fundWaveBlend = 0.0f;
    float subWaveBlend = 0.0f;
    float bodyFreqHz = 40.0f;
    float bodyLevelAmt = 0.3f;

    /// Map division choice index to actual divisor.
    /// 0 = Off, 1 = divide by 2, 2 = divide by 3, ... 7 = divide by 8.
    static int divIndexToDivisor(int index)
    {
        if (index <= 0)
            return 0; // off
        if (index > 7)
            return 8;     // clamp
        return index + 1; // 1->2, 2->3, ... 7->8
    }
};

} // namespace xowlfish
