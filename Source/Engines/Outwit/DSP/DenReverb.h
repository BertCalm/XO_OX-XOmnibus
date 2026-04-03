// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// DenReverb.h — XOutwit "Octopus Den" reverb
//
// A Schroeder-style FDN (feedback delay network) tuned to sound like the
// resonant interior of an octopus den: short-to-medium diffuse space with
// a dark, organic character.
//
// Adapter usage:
//   denReverb.prepare(sampleRate)
//   denReverb.clear()
//   denReverb.setParams(size, decay, sampleRate)
//   float wet = denReverb.process(monoInput)
//
// All delay lines are statically allocated (no heap on audio thread).
// Denormal protection on all feedback nodes.
//==============================================================================

#include "FastMath.h"
#include <array>
#include <algorithm>
#include <cmath>

namespace xoutwit
{

class DenReverb
{
public:
    //--------------------------------------------------------------------------
    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        computeCoefficients();
        clear();
    }

    //--------------------------------------------------------------------------
    void clear() noexcept
    {
        for (auto& dl : delayBuffers)
            dl.fill(0.0f);
        for (auto& d : delayWritePos)
            d = 0;
        for (auto& s : allpassBuf)
            s = 0.0f;
        lpfStates.fill(0.0f);
    }

    //--------------------------------------------------------------------------
    // size [0,1]: scales delay line lengths (room size)
    // decay [0,1]: feedback gain (reverb time)
    // Pass current sampleRate to recompute delay lengths if rate changed.
    void setParams(float size, float decay, double currentSampleRate) noexcept
    {
        sr = static_cast<float>(currentSampleRate);
        roomSize = std::clamp(size, 0.0f, 1.0f);
        decayAmt = std::clamp(decay, 0.0f, 1.0f);
        computeCoefficients();
    }

    //--------------------------------------------------------------------------
    // Process one mono sample; returns wet mono output.
    float process(float input) noexcept
    {
        // Attenuate input to prevent buildup
        float x = input * 0.25f;

        // --- 4-comb filter network (Schroeder) ---
        // Each comb line has its own independent LP state (Seance P2 fix:
        // shared lpfState created serial dependency between parallel combs)
        float combOut = 0.0f;
        for (size_t i = 0; i < kNumCombs; ++i)
        {
            int len = combLengths[i];
            int rpos = (delayWritePos[i] - len + kMaxDelay) & kDelayMask;
            float y = delayBuffers[i][rpos];
            y = xoutwit::flushDenormal(y);

            float filtered = y + lpfCoeff * (y - lpfStates[i]); // per-comb shelf LPF
            lpfStates[i] = xoutwit::flushDenormal(filtered);

            delayBuffers[i][delayWritePos[i]] = x + filtered * feedbackGain;
            delayWritePos[i] = (delayWritePos[i] + 1) & kDelayMask;

            combOut += y;
        }
        combOut *= 0.25f;

        // --- 2-allpass diffusers ---
        float apOut = combOut;
        for (size_t i = 0; i < kNumAllpass; ++i)
        {
            float delayed = allpassBuf[i];
            float fwd = apOut + kAllpassGain * delayed;
            allpassBuf[i] = xoutwit::flushDenormal(fwd);
            apOut = delayed - kAllpassGain * fwd;
        }

        return xoutwit::flushDenormal(apOut);
    }

private:
    static constexpr int kMaxDelay = 8192; // must be power of 2
    static constexpr int kDelayMask = kMaxDelay - 1;
    static constexpr int kNumCombs = 4;
    static constexpr int kNumAllpass = 2;
    static constexpr float kAllpassGain = 0.7f;

    // Comb delay line lengths in samples (Schroeder primes scaled by room size)
    // Base lengths at 48kHz: 1557, 1617, 1491, 1422 (classic Schroeder ratios)
    static constexpr std::array<int, kNumCombs> kBaseCombMs = {32, 34, 31, 29}; // ms

    float sr = 48000.0f;
    float roomSize = 0.4f;
    float decayAmt = 0.4f;

    float feedbackGain = 0.7f;
    float lpfCoeff = 0.5f;
    std::array<float, kNumCombs> lpfStates{}; // independent LP state per comb line (Seance P2)

    std::array<int, kNumCombs> combLengths{};

    // Delay buffers — statically allocated, no heap
    std::array<std::array<float, kMaxDelay>, kNumCombs> delayBuffers{};
    std::array<int, kNumCombs> delayWritePos{};

    // Allpass state (single-sample, no delay line needed for short diffusers)
    std::array<float, kNumAllpass> allpassBuf{};

    void computeCoefficients() noexcept
    {
        // feedbackGain: 0.4 (dry/short) → 0.89 (large hall)
        feedbackGain = 0.4f + decayAmt * 0.49f;

        // LPF coefficient: higher roomSize → darker, more air absorption
        // lpfCoeff near 0 = heavy LPF, near 1 = bright
        lpfCoeff = 0.5f * (1.0f - roomSize * 0.6f);

        // Scale comb lengths by roomSize
        for (size_t i = 0; i < kNumCombs; ++i)
        {
            float scaledMs = kBaseCombMs[i] * (0.5f + roomSize * 1.5f);
            int lengthSamps = static_cast<int>(scaledMs * 0.001f * sr);
            // Clamp to [16, kMaxDelay-1]
            combLengths[i] = std::clamp(lengthSamps, 16, kMaxDelay - 1);
        }
    }
};

} // namespace xoutwit
