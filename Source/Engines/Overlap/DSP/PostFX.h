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

namespace xoverlap {

//==============================================================================
class PostFX
{
public:
    //==========================================================================
    void prepare (double sampleRate) noexcept
    {
        sr = static_cast<float> (sampleRate);

        // Clear all delay buffers
        chorusBufL.fill (0.0f);
        chorusBufR.fill (0.0f);
        chorusWriteL = 0;
        chorusWriteR = 0;
        chorusPhaseL = 0.0f;
        chorusPhaseR = 0.25f;  // 90° offset for stereo image

        for (int i = 0; i < kDiffStages; ++i)
        {
            apBufL[i].fill (0.0f);
            apBufR[i].fill (0.0f);
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
    void process (float& left, float& right,
                  float chorusRate, float chorusMix, float diffusion) noexcept
    {
        // ---- Chorus --------------------------------------------------------
        if (chorusMix > 0.001f)
        {
            // LFO phase advance
            float phaseInc = chorusRate / sr;
            chorusPhaseL += phaseInc;
            if (chorusPhaseL >= 1.0f) chorusPhaseL -= 1.0f;
            chorusPhaseR += phaseInc;
            if (chorusPhaseR >= 1.0f) chorusPhaseR -= 1.0f;

            float lfoL = 0.5f + 0.5f * fastSin (chorusPhaseL * 6.28318530f);
            float lfoR = 0.5f + 0.5f * fastSin (chorusPhaseR * 6.28318530f);

            // Delay range: 5ms..25ms
            float minDelay = 0.005f * sr;
            float maxDelay = 0.025f * sr;
            float delayL   = minDelay + lfoL * (maxDelay - minDelay);
            float delayR   = minDelay + lfoR * (maxDelay - minDelay);

            // Write into chorus buffers
            chorusBufL[static_cast<size_t> (chorusWriteL)] = left;
            chorusBufR[static_cast<size_t> (chorusWriteR)] = right;

            // Read with linear interpolation
            float wetL = readInterp (chorusBufL, chorusWriteL, delayL, kChorusBufLen);
            float wetR = readInterp (chorusBufR, chorusWriteR, delayR, kChorusBufLen);

            if (++chorusWriteL >= kChorusBufLen) chorusWriteL = 0;
            if (++chorusWriteR >= kChorusBufLen) chorusWriteR = 0;

            left  = left  * (1.0f - chorusMix) + wetL * chorusMix;
            right = right * (1.0f - chorusMix) + wetR * chorusMix;
        }

        // ---- Diffusion (all-pass series) -----------------------------------
        if (diffusion > 0.001f)
        {
            float g = diffusion * 0.7f;  // all-pass coefficient [0, 0.7]
            left  = processAllPassSeries (left,  g, apBufL, apWriteL);
            right = processAllPassSeries (right, g, apBufR, apWriteR);
        }
    }

private:
    //==========================================================================
    static constexpr int kChorusBufLen = 4096;   // ~93ms at 44.1kHz
    static constexpr int kDiffStages   = 3;
    static constexpr int kApBufLen     = 512;    // ~11.6ms max per stage

    // All-pass delay lengths in samples (inharmonic primes for diffusion)
    static constexpr int kApDelays[kDiffStages] = { 113, 257, 397 };

    float sr = 44100.0f;

    // Chorus state
    std::array<float, kChorusBufLen>  chorusBufL {};
    std::array<float, kChorusBufLen>  chorusBufR {};
    int   chorusWriteL = 0;
    int   chorusWriteR = 0;
    float chorusPhaseL = 0.0f;
    float chorusPhaseR = 0.25f;

    // All-pass diffusion state
    std::array<std::array<float, kApBufLen>, kDiffStages> apBufL {};
    std::array<std::array<float, kApBufLen>, kDiffStages> apBufR {};
    std::array<int, kDiffStages> apWriteL {};
    std::array<int, kDiffStages> apWriteR {};

    //==========================================================================
    // Linear interpolated read from circular buffer
    float readInterp (const std::array<float, kChorusBufLen>& buf,
                      int writePos, float delaySamples, int bufLen) const noexcept
    {
        int   d1   = static_cast<int> (delaySamples);
        float frac = delaySamples - static_cast<float> (d1);

        int rp1 = writePos - d1;
        int rp2 = rp1 - 1;
        while (rp1 < 0) rp1 += bufLen;
        while (rp2 < 0) rp2 += bufLen;
        rp1 %= bufLen;
        rp2 %= bufLen;

        return buf[static_cast<size_t> (rp1)] * (1.0f - frac)
             + buf[static_cast<size_t> (rp2)] * frac;
    }

    //==========================================================================
    // Series all-pass: process through kDiffStages all-pass filters
    float processAllPassSeries (float x, float g,
                                 std::array<std::array<float, kApBufLen>, kDiffStages>& bufs,
                                 std::array<int, kDiffStages>& writes) noexcept
    {
        float y = x;
        for (int s = 0; s < kDiffStages; ++s)
        {
            auto& buf = bufs[static_cast<size_t>(s)];
            int&  wp  = writes[static_cast<size_t>(s)];
            int   d   = kApDelays[s];

            // Read from delay
            int rp = wp - d;
            if (rp < 0) rp += kApBufLen;
            rp %= kApBufLen;
            float delayed = buf[static_cast<size_t> (rp)];

            // All-pass difference equation: w[n] = x[n] - g * w[n-D]
            //                               y[n] = g * w[n] + w[n-D]
            float w = y - g * delayed;
            float out = g * w + delayed;

            buf[static_cast<size_t> (wp)] = flushDenormal (w);
            if (++wp >= kApBufLen) wp = 0;

            y = out;
        }
        return flushDenormal (y);
    }
};

} // namespace xoverlap
