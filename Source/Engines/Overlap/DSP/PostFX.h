// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// PostFX.h — xoverlap::PostFX
//
// Post-FDN stereo effects chain:
//   1. Chorus — stereo widening via two independent modulated delay lines
//      (one per channel) — classic BBD-style mono pitch shimmer + stereo split.
//   2. Diffusion — all-pass based spatial diffusion that smears transients
//      across time, increasing the perceived "cloudiness" of the FDN tail.
//
// Chorus:
//   Delay range: 5–25ms modulated by a slow sine LFO.
//   Left and right modulators run 90° out of phase for stereo width.
//   chorusRate [0.01..0.5 Hz] controls LFO speed.
//   chorusMix  [0..1] controls wet/dry.
//
// Diffusion:
//   Three series all-pass sections per channel, with delay lengths derived
//   from a set of inharmonic primes (Moorer-style early reflection diffusor).
//   diffusion [0..1] controls all-pass coefficient.
//
// No heap allocation after prepare(). All buffers are fixed-size.
//==============================================================================

#include "FastMath.h"
#include <array>
#include <cmath>

namespace xoverlap
{

//==============================================================================
class PostFX
{
public:
    //==========================================================================
    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);

        // Clear all delay buffers
        chorusBufL.fill(0.0f);
        chorusBufR.fill(0.0f);
        chorusWriteL = 0;
        chorusWriteR = 0;
        chorusPhaseL = 0.0f;
        chorusPhaseR = 0.25f; // 90° offset for stereo image

        for (int i = 0; i < kDiffStages; ++i)
        {
            apBufL[i].fill(0.0f);
            apBufR[i].fill(0.0f);
            apWriteL[i] = 0;
            apWriteR[i] = 0;
        }
    }

    //==========================================================================
    // process() — apply chorus + diffusion in-place to L/R samples.
    //
    // left, right:   input/output samples (modified in-place)
    // chorusRate:    LFO rate in Hz [0.01, 0.5]
    // chorusMix:     wet gain [0, 1]
    // diffusion:     all-pass coefficient [0, 1]
    void process(float& left, float& right, float chorusRate, float chorusMix, float diffusion) noexcept
    {
        // ---- Chorus --------------------------------------------------------
        if (chorusMix > 0.001f)
        {
            // LFO phase advance
            float phaseInc = chorusRate / sr;
            chorusPhaseL += phaseInc;
            if (chorusPhaseL >= 1.0f)
                chorusPhaseL -= 1.0f;
            chorusPhaseR += phaseInc;
            if (chorusPhaseR >= 1.0f)
                chorusPhaseR -= 1.0f;

            float lfoL = 0.5f + 0.5f * fastSin(chorusPhaseL * 6.28318530f);
            float lfoR = 0.5f + 0.5f * fastSin(chorusPhaseR * 6.28318530f);

            // Delay range: 5ms..25ms
            float minDelay = 0.005f * sr;
            float maxDelay = 0.025f * sr;
            float delayL = minDelay + lfoL * (maxDelay - minDelay);
            float delayR = minDelay + lfoR * (maxDelay - minDelay);

            // Write into chorus buffers
            chorusBufL[static_cast<size_t>(chorusWriteL)] = left;
            chorusBufR[static_cast<size_t>(chorusWriteR)] = right;

            // Read with linear interpolation
            float wetL = readInterp(chorusBufL, chorusWriteL, delayL, kChorusBufLen);
            float wetR = readInterp(chorusBufR, chorusWriteR, delayR, kChorusBufLen);

            if (++chorusWriteL >= kChorusBufLen)
                chorusWriteL = 0;
            if (++chorusWriteR >= kChorusBufLen)
                chorusWriteR = 0;

            left = left * (1.0f - chorusMix) + wetL * chorusMix;
            right = right * (1.0f - chorusMix) + wetR * chorusMix;
        }

        // ---- Diffusion (all-pass series) -----------------------------------
        if (diffusion > 0.001f)
        {
            float g = diffusion * 0.7f; // all-pass coefficient [0, 0.7]
            left = processAllPassSeries(left, g, apBufL, apWriteL);
            right = processAllPassSeries(right, g, apBufR, apWriteR);
        }
    }

private:
    //==========================================================================
    // F11: chorus max delay is 25ms.  At 192kHz: 0.025 * 192000 = 4800 samples > 4096.
    // Extend to 8192 (next power-of-two above 4800) to cover 192kHz.
    static constexpr int kChorusBufLen = 8192; // covers 192kHz @ 25ms max delay
    static constexpr int kDiffStages = 3;
    // F03: all-pass delay constants are in absolute samples tuned for 44.1kHz.
    // At 96kHz, delay 257 → 561 samples and delay 397 → 866 samples, both > kApBufLen=512.
    // Extend kApBufLen to 2048 (next power-of-two above 866) to cover 96kHz.
    static constexpr int kApBufLen = 2048; // covers 96kHz for all three stage delays

    // All-pass delay lengths in samples (inharmonic primes for diffusion)
    // NOTE: these are calibrated at 44.1kHz; at higher sample rates the diffusion
    // tail is proportionally shorter, which is acceptable (no audible artefact).
    static constexpr int kApDelays[kDiffStages] = {113, 257, 397};

    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)

    // Chorus state
    std::array<float, kChorusBufLen> chorusBufL{};
    std::array<float, kChorusBufLen> chorusBufR{};
    int chorusWriteL = 0;
    int chorusWriteR = 0;
    float chorusPhaseL = 0.0f;
    float chorusPhaseR = 0.25f;

    // All-pass diffusion state
    std::array<std::array<float, kApBufLen>, kDiffStages> apBufL{};
    std::array<std::array<float, kApBufLen>, kDiffStages> apBufR{};
    std::array<int, kDiffStages> apWriteL{};
    std::array<int, kDiffStages> apWriteR{};

    //==========================================================================
    // Linear interpolated read from circular buffer
    float readInterp(const std::array<float, kChorusBufLen>& buf, int writePos, float delaySamples,
                     int bufLen) const noexcept
    {
        int d1 = static_cast<int>(delaySamples);
        float frac = delaySamples - static_cast<float>(d1);

        // F13-style: O(1) modulo avoids while-loop for large delay values
        int rp1 = ((writePos - d1) % bufLen + bufLen) % bufLen;
        int rp2 = (rp1 - 1 + bufLen) % bufLen;

        return buf[static_cast<size_t>(rp1)] * (1.0f - frac) + buf[static_cast<size_t>(rp2)] * frac;
    }

    //==========================================================================
    // Series all-pass: process through kDiffStages all-pass filters
    float processAllPassSeries(float x, float g, std::array<std::array<float, kApBufLen>, kDiffStages>& bufs,
                               std::array<int, kDiffStages>& writes) noexcept
    {
        float y = x;
        for (int s = 0; s < kDiffStages; ++s)
        {
            auto& buf = bufs[static_cast<size_t>(s)];
            int& wp = writes[static_cast<size_t>(s)];
            int d = kApDelays[s];

            // Read from delay — O(1) modulo, safe with extended kApBufLen
            int rp = ((wp - d) % kApBufLen + kApBufLen) % kApBufLen;
            float delayed = buf[static_cast<size_t>(rp)];

            // All-pass difference equation: w[n] = x[n] - g * w[n-D]
            //                               y[n] = g * w[n] + w[n-D]
            float w = y - g * delayed;
            float out = g * w + delayed;

            buf[static_cast<size_t>(wp)] = flushDenormal(w);
            if (++wp >= kApBufLen)
                wp = 0;

            y = out;
        }
        return flushDenormal(y);
    }
};

} // namespace xoverlap
