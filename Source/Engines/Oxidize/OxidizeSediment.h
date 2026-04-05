// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include <cmath>
#include <vector>
#include "../../DSP/FastMath.h"

namespace xoceanus
{

//==============================================================================
// OxidizeSediment — Shared 4-delay-line FDN reverb for OXIDIZE.
//
// One instance shared across all voices (Guru Bin: architecturally correct
// and CPU-efficient). Voices write sediment sends into the FDN input; the FDN
// accumulates decay across notes over a T60 range of 2s–300s.
//
// Design:
//   - 4 delay lines, coprime base lengths: 1117, 1571, 1949, 2311 samples
//     @ 44100 Hz (avoids metallic modes / Schroeder comb artifacts).
//     In prepare(), each length is scaled by sampleRate/44100 so the absolute
//     delay times (25ms/35ms/44ms/52ms) are preserved at any sample rate.
//     Approximate coprimality after rounding is sufficient to avoid comb
//     artifacts — no search for new coprime numbers is needed.
//   - Hadamard feedback matrix (orthogonal, energy-preserving)
//   - Per-delay one-pole low-pass filter (sedimentTone controls the cutoff)
//   - T60: 2s (sedimentTail=0) to 300s (sedimentTail=1)  — never infinite
//   - Output decorrelation: L = delay[0] + delay[2], R = delay[1] + delay[3]
//
// Hadamard H = 0.5 * [[ 1,  1,  1,  1],
//                      [ 1, -1,  1, -1],
//                      [ 1,  1, -1, -1],
//                      [ 1, -1, -1,  1]]
//
// Usage:
//   sediment_.prepare(sampleRate, maxBlockSize);
//   sediment_.setParameters(tail, tone, sampleRate);
//   sediment_.processBlock(inL, inR, outL, outR, numSamples);
//==============================================================================

class OxidizeSediment
{
public:
    //--------------------------------------------------------------------------
    // Prepare — allocate delay lines. Call before any audio processing.
    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sampleRate_ = sampleRate;

        // Scale base delay lengths (defined at 44100 Hz) to the actual sample
        // rate so that absolute delay times are preserved at any rate.
        const double srRatio = sampleRate / kReferenceSampleRate;
        for (int i = 0; i < kNumDelays; ++i)
            scaledDelayLengths_[i] = static_cast<int>(kDelayLengths[i] * srRatio);

        for (int i = 0; i < kNumDelays; ++i)
        {
            const int len = scaledDelayLengths_[i] + 1;  // +1 for write-head safety
            delayLines_[i].assign(static_cast<size_t>(len), 0.0f);
            writeHeads_[i] = 0;
            lpStates_[i]   = 0.0f;
        }

        // Default parameters (sedimentTail=0.5, sedimentTone=0.4)
        setParameters(0.5f, 0.4f, sampleRate);
    }

    //--------------------------------------------------------------------------
    // Reset — clear all delay-line and filter state.
    void reset()
    {
        for (int i = 0; i < kNumDelays; ++i)
        {
            std::fill(delayLines_[i].begin(), delayLines_[i].end(), 0.0f);
            writeHeads_[i] = 0;
            lpStates_[i]   = 0.0f;
        }
    }

    //--------------------------------------------------------------------------
    // setParameters — update feedback gain and LP-filter coefficients.
    // Call whenever sedimentTail or sedimentTone change.
    //
    //   tail     0..1 → T60 2s (0) to 300s (1)
    //   tone     0..1 → LP cutoff 200 Hz (0) to 12 000 Hz (1)
    //   sampleRate — must match the rate passed to prepare()
    void setParameters(float tail, float tone, double sampleRate)
    {
        sampleRate_ = sampleRate;

        // T60 from tail (seconds)
        const float t60 = 2.0f + tail * 298.0f;

        // Per-delay feedback gain derived from T60 of the longest delay line.
        // formula: feedback = 10^(-3 * delayTime / (T60 * sampleRate))
        // We use the longest scaled delay so the formula operates in samples
        // at the actual sample rate, giving the correct absolute decay time.
        const float longestDelaySamples = static_cast<float>(scaledDelayLengths_[kNumDelays - 1]);
        feedbackGain_ = std::pow(10.0f,
            -3.0f * longestDelaySamples / (t60 * static_cast<float>(sampleRate)));

        // One-pole LP coefficient for sedimentTone.
        // Map tone 0..1 to cutoff 200..12000 Hz using a log ramp.
        // coeff = 1 - exp(-2π * fc / sr)  (standard one-pole LP)
        const float fcLow  = 200.0f;
        const float fcHigh = 12000.0f;
        // Exponential ramp so low-tone values stay warm/dark
        const float fc = fcLow * std::pow(fcHigh / fcLow, tone);
        const float twoPiOverSr = static_cast<float>(6.28318530718 / sampleRate);
        lpCoeff_ = 1.0f - std::exp(-twoPiOverSr * fc);
    }

    //--------------------------------------------------------------------------
    // processSample — single-sample FDN tick.
    // inputL and inputR are the mono sediment send from all voice tap points
    // (caller sums the three tap points and passes the result here).
    void processSample(float inputL, float inputR, float& outL, float& outR)
    {
        // --- Read delayed outputs (before write) ---
        float delayed[kNumDelays];
        for (int i = 0; i < kNumDelays; ++i)
        {
            const int readHead = (writeHeads_[i] - scaledDelayLengths_[i] + static_cast<int>(delayLines_[i].size()))
                                 % static_cast<int>(delayLines_[i].size());
            delayed[i] = flushDenormal(delayLines_[i][static_cast<size_t>(readHead)]);
        }

        // --- Per-delay one-pole LP filter (tone control) ---
        for (int i = 0; i < kNumDelays; ++i)
        {
            lpStates_[i] = flushDenormal(lpStates_[i] + lpCoeff_ * (delayed[i] - lpStates_[i]));
            delayed[i]   = lpStates_[i];
        }

        // --- Hadamard feedback matrix ---
        // H = 0.5 * [[ 1,  1,  1,  1],
        //            [ 1, -1,  1, -1],
        //            [ 1,  1, -1, -1],
        //            [ 1, -1, -1,  1]]
        const float h0 = 0.5f * ( delayed[0] + delayed[1] + delayed[2] + delayed[3]);
        const float h1 = 0.5f * ( delayed[0] - delayed[1] + delayed[2] - delayed[3]);
        const float h2 = 0.5f * ( delayed[0] + delayed[1] - delayed[2] - delayed[3]);
        const float h3 = 0.5f * ( delayed[0] - delayed[1] - delayed[2] + delayed[3]);

        // --- Mono input: average L+R so both channels feed the shared FDN ---
        const float monoIn = 0.5f * (inputL + inputR);

        // --- Write: input + scaled feedback into each delay line ---
        const float hadamard[kNumDelays] = { h0, h1, h2, h3 };
        for (int i = 0; i < kNumDelays; ++i)
        {
            const float writeVal = flushDenormal(monoIn + feedbackGain_ * hadamard[i]);
            delayLines_[i][static_cast<size_t>(writeHeads_[i])] = writeVal;
            writeHeads_[i] = (writeHeads_[i] + 1) % static_cast<int>(delayLines_[i].size());
        }

        // --- Stereo decorrelation output ---
        // L = delay[0] + delay[2],  R = delay[1] + delay[3]
        // Scale by 0.5 to keep unity gain at the sum stage
        outL = 0.5f * (delayed[0] + delayed[2]);
        outR = 0.5f * (delayed[1] + delayed[3]);
    }

    //--------------------------------------------------------------------------
    // processBlock — block-level process (delegates to processSample).
    void processBlock(const float* inL, const float* inR,
                      float* outL, float* outR, int numSamples)
    {
        for (int s = 0; s < numSamples; ++s)
            processSample(inL[s], inR[s], outL[s], outR[s]);
    }

private:
    //--------------------------------------------------------------------------
    static constexpr int    kNumDelays           = 4;
    static constexpr double kReferenceSampleRate = 44100.0;
    // Coprime base delay lengths defined at 44100 Hz.
    // At 44100 Hz: 1117=25ms, 1571=35ms, 1949=44ms, 2311=52ms.
    // prepare() scales these to the actual sample rate so that absolute
    // delay times are preserved regardless of host sample rate.
    static constexpr int kDelayLengths[kNumDelays] = { 1117, 1571, 1949, 2311 };

    double sampleRate_           = 44100.0;
    int    scaledDelayLengths_[kNumDelays] = { 1117, 1571, 1949, 2311 };  // populated in prepare()

    std::vector<float> delayLines_[kNumDelays];
    int                writeHeads_[kNumDelays] = {};
    float              lpStates_[kNumDelays]   = {};

    float feedbackGain_ = 0.0f;  // derived from sedimentTail + sampleRate
    float lpCoeff_      = 1.0f;  // one-pole LP coefficient from sedimentTone
};

} // namespace xoceanus
