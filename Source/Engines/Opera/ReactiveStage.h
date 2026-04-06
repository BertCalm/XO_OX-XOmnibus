// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "OperaConstants.h"

//==============================================================================
// ReactiveStage.h — opera::ReactiveStage
//
// Reactive reverb for the XOpera (OPERA) synthesizer. A 4-line Hadamard FDN
// whose parameters respond to the Kuramoto synchronicity field in real-time
// (Tomita's suggestion).
//
// Reactive behavior:
//   - RT60 scales with coupling K: intimate room at low K, cathedral at high K
//   - Early reflection density increases with drama
//   - Frequency-dependent damping simulates air absorption in larger spaces
//   - Pre-delay contracts at high sync (hall wraps around performer)
//
// Architecture:
//   input (mono sum) -> pre-delay -> 4 prime-length delay lines
//   -> Hadamard feedback matrix (orthogonal, energy-preserving)
//   -> one-pole absorption filters per line -> feedback injection
//   -> stereo output from alternating line pairs
//
// All code inline. No allocation on audio thread (buffers sized in prepare()).
// No framework dependencies.
//==============================================================================

#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>

namespace opera
{

//==============================================================================
/// ReactiveStage — Kuramoto-responsive FDN reverb.
///
/// Usage:
///   ReactiveStage stage;
///   stage.prepare (sampleRate, maxBlockSize);
///
///   // Per block:
///   stage.processBlock (leftChannel, rightChannel, numSamples,
///                       stageAmount, orderParam);
//==============================================================================
class ReactiveStage
{
public:
    //==========================================================================
    /// Prepare the stage. Allocates delay line buffers (call on audio thread
    /// init, not per-block). Must be called before processBlock.
    ///
    /// @param sampleRate     Host sample rate (Hz). Never hardcoded to 44100.
    /// @param maxBlockSize   Maximum samples per processBlock call.
    //==========================================================================
    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr = static_cast<float>(sampleRate);
        if (sr <= 0.0f)
            sr = 48000.0f;

        invSr = 1.0f / sr;

        // Compute prime delay lengths scaled to actual sample rate.
        // Base lengths chosen at 48kHz: 1423, 1637, 1879, 2089 samples.
        // These are primes, giving maximal diffusion and minimal comb artifacts.
        for (int i = 0; i < kNumLines; ++i)
        {
            float scaledLen = kBaseDelayLengths48k[i] * (sr / 48000.0f);
            delayLengths[i] = std::max(1, static_cast<int>(scaledLen + 0.5f));
        }

        // Pre-delay buffer: max 60ms
        maxPreDelaySamples = static_cast<int>(0.06f * sr) + 1;

        // Allocate buffers. Each delay line needs enough capacity for the
        // longest possible delay (base length at max sample rate).
        // Add margin for pre-delay modulation.
        int maxDelay = static_cast<int>(2200.0f * (sr / 48000.0f)) + 2;
        for (int i = 0; i < kNumLines; ++i)
        {
            bufferSizes[i] = maxDelay;
            delayBuffers[i].assign(static_cast<size_t>(maxDelay), 0.0f);
        }

        preDelayBuffer.assign(static_cast<size_t>(maxPreDelaySamples), 0.0f);

        reset();
    }

    //==========================================================================
    /// Clear all delay line state and filter memory.
    //==========================================================================
    void reset() noexcept
    {
        for (int i = 0; i < kNumLines; ++i)
        {
            std::fill(delayBuffers[i].begin(), delayBuffers[i].end(), 0.0f);
            writePos[i] = 0;
            dampState[i] = 0.0f;
        }

        std::fill(preDelayBuffer.begin(), preDelayBuffer.end(), 0.0f);
        preDelayWritePos = 0;

        // Smoothing state
        smoothedOrderParam = 0.0f;
    }

    //==========================================================================
    /// Process a stereo block in-place.
    ///
    /// @param leftChannel   Left audio channel (read/write, numSamples float)
    /// @param rightChannel  Right audio channel (read/write, numSamples float)
    /// @param numSamples    Number of samples in this block
    /// @param stageAmount   opera_stage: 0.0 (dry) to 1.0 (full wet)
    /// @param orderParam    Kuramoto order parameter r(t): 0.0 to 1.0
    //==========================================================================
    void processBlock(float* leftChannel, float* rightChannel, int numSamples, float stageAmount,
                      float orderParam) noexcept
    {
        if (stageAmount <= 0.0001f || numSamples <= 0)
            return;

        float wetMix = clamp(stageAmount, 0.0f, 1.0f);
        float dryMix = 1.0f - wetMix;

        // ----------------------------------------------------------------
        // Per-block: smooth order parameter toward target, then compute
        // all derived coefficients once. This avoids std::pow / std::exp
        // inside the per-sample loop (critical for CPU budget).
        //
        // The smoothing uses a simple one-pole filter advanced by the
        // block size, giving natural ~20ms response.
        // ----------------------------------------------------------------
        float rTarget = clamp(orderParam, 0.0f, 1.0f);

        // One-pole smoothing coefficient (~20ms time constant)
        float smoothCoeff = 1.0f - std::exp(-kTwoPi * 50.0f * invSr);

        // Compute start-of-block and end-of-block order parameter
        float rStart = smoothedOrderParam;
        // Advance smoother by numSamples steps (geometric series closed form)
        float rEnd = rTarget + (rStart - rTarget) * std::pow(1.0f - smoothCoeff, static_cast<float>(numSamples));
        smoothedOrderParam = rEnd;

        // Use midpoint for block-rate coefficient computation
        float rSmooth = (rStart + rEnd) * 0.5f;

        // ----------------------------------------------------------------
        // Reactive RT60: scales with coupling K
        //   Base: 0.2s (dry room) to 5.0s (cathedral) from stageAmount
        //   Reactive: multiply by 1.0x (r=0) to 2.5x (r=1)
        // ----------------------------------------------------------------
        float baseRT60 = 0.2f + wetMix * 4.8f;
        float reactiveMultiplier = 1.0f + rSmooth * 1.5f;
        float effectiveRT60 = baseRT60 * reactiveMultiplier;

        // Convert RT60 to per-line feedback gain:
        // gain_i = 10^(-3 * delayTime_i / RT60)
        float feedbackGain[kNumLines];
        for (int i = 0; i < kNumLines; ++i)
        {
            float delayTimeSec = static_cast<float>(delayLengths[i]) * invSr;
            feedbackGain[i] = std::pow(10.0f, -3.0f * delayTimeSec / effectiveRT60);
            // Safety clamp: prevent runaway feedback
            feedbackGain[i] = std::min(feedbackGain[i], 0.9995f);
        }

        // ----------------------------------------------------------------
        // Frequency-dependent damping (air absorption)
        //   At low sync (r~0): 12000 Hz cutoff (bright, intimate)
        //   At high sync (r~1): 4000 Hz cutoff (dark, cathedral)
        // ----------------------------------------------------------------
        float dampingCutoff = 12000.0f - rSmooth * 8000.0f;
        float dampCoeff = std::exp(-kTwoPi * dampingCutoff * invSr);

        // ----------------------------------------------------------------
        // Pre-delay: contracts at high sync (hall wraps around)
        //   r=0: 50ms  (distant, sparse)
        //   r=1: 10ms  (close, dense)
        // ----------------------------------------------------------------
        float preDelayMs = 10.0f + (1.0f - rSmooth) * 40.0f;
        int preDelaySamples = static_cast<int>(preDelayMs * 0.001f * sr);
        preDelaySamples = std::max(1, std::min(preDelaySamples, maxPreDelaySamples - 1));

        // ----------------------------------------------------------------
        // Early reflection density: active line count
        //   r=0: 2 lines (sparse)
        //   r=1: 4 lines (dense)
        // ----------------------------------------------------------------
        int activeLines = 2 + static_cast<int>(rSmooth * 2.0f);
        activeLines = std::min(activeLines, kNumLines);

        // ----------------------------------------------------------------
        // Per-sample FDN processing
        // ----------------------------------------------------------------
        for (int n = 0; n < numSamples; ++n)
        {
            // Mono input sum
            float inputMono = (leftChannel[n] + rightChannel[n]) * 0.5f;

            // Write to pre-delay and read back
            preDelayBuffer[static_cast<size_t>(preDelayWritePos)] = inputMono;
            int preDelayReadPos = preDelayWritePos - preDelaySamples;
            if (preDelayReadPos < 0)
                preDelayReadPos += maxPreDelaySamples;
            float preDelayedInput = preDelayBuffer[static_cast<size_t>(preDelayReadPos)];
            preDelayWritePos = (preDelayWritePos + 1) % maxPreDelaySamples;

            // 1. Read from delay lines
            float read[kNumLines];
            for (int i = 0; i < kNumLines; ++i)
            {
                int rp = writePos[i] - delayLengths[i];
                if (rp < 0)
                    rp += bufferSizes[i];
                read[i] = delayBuffers[i][static_cast<size_t>(rp)];
            }

            // Zero out inactive lines (early reflection density control)
            for (int i = activeLines; i < kNumLines; ++i)
                read[i] = 0.0f;

            // 2. Hadamard mixing matrix (4x4, orthogonal, energy-preserving)
            //    H = 0.5 * [[1,1,1,1],[1,-1,1,-1],[1,1,-1,-1],[1,-1,-1,1]]
            float mixed[kNumLines];
            for (int i = 0; i < kNumLines; ++i)
            {
                mixed[i] = 0.0f;
                for (int j = 0; j < kNumLines; ++j)
                    mixed[i] += kHadamard[i][j] * read[j];
            }

            // 3. Absorption filtering + feedback + input injection
            for (int i = 0; i < kNumLines; ++i)
            {
                // One-pole lowpass damping in each feedback path
                dampState[i] = mixed[i] + (dampState[i] - mixed[i]) * dampCoeff;
                dampState[i] = flushDenormal(dampState[i]);

                // Apply RT60-derived feedback gain
                float fb = dampState[i] * feedbackGain[i];

                // FIX P0: gate input injection — inactive lines must not accumulate
                // input energy; they were zeroed for output (read[i]=0) but still
                // received preDelayedInput on every write, causing them to build up
                // a hidden reservoir that burst audibly when they became active again.
                float injection = (i < activeLines) ? preDelayedInput : 0.0f;

                // Write: feedback + (gated) pre-delayed input
                delayBuffers[i][static_cast<size_t>(writePos[i])] = flushDenormal(fb + injection);

                // Advance write pointer (wrap within buffer)
                writePos[i] = (writePos[i] + 1) % bufferSizes[i];
            }

            // 4. Stereo output from alternating delay line pairs
            //    Lines 0+2 -> left, lines 1+3 -> right
            float wetL = (read[0] + read[2]) * 0.5f;
            float wetR = (read[1] + read[3]) * 0.5f;

            // 5. Mix dry/wet
            leftChannel[n] = leftChannel[n] * dryMix + wetL * wetMix;
            rightChannel[n] = rightChannel[n] * dryMix + wetR * wetMix;
        }
    }

private:
    //==========================================================================
    static constexpr int kNumLines = 4;

    // Prime-number delay lengths at 48kHz — chosen for maximal diffusion,
    // minimal sympathetic ringing. Each prime avoids integer relationships
    // that cause metallic coloration.
    static constexpr int kBaseDelayLengths48k[kNumLines] = {1423, 1637, 1879, 2089};

    // Hadamard feedback matrix (4x4, orthogonal, energy-preserving).
    // H4 = (1/2) * [[1,1,1,1],[1,-1,1,-1],[1,1,-1,-1],[1,-1,-1,1]]
    // Each row/column has unit magnitude after the 0.5 scaling.
    // Orthogonality guarantees energy preservation (no buildup/decay
    // from the matrix itself — all decay comes from the absorption filters).
    static constexpr float kHadamard[kNumLines][kNumLines] = {
        {0.5f, 0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f, 0.5f}};

    //==========================================================================
    float sr = 0.0f;  // Sentinel: must be set by prepare() before use (guarded with fallback)
    float invSr = 0.0f;  // Sentinel: computed from sr in prepare()

    // Delay line lengths (in samples, scaled to actual sample rate)
    int delayLengths[kNumLines] = {};

    // Delay line buffers (heap-allocated once in prepare, never on audio thread)
    std::vector<float> delayBuffers[kNumLines];
    int bufferSizes[kNumLines] = {};
    int writePos[kNumLines] = {};

    // Per-line absorption filter state (one-pole lowpass)
    float dampState[kNumLines] = {};

    // Pre-delay buffer
    std::vector<float> preDelayBuffer;
    int preDelayWritePos = 0;
    int maxPreDelaySamples = 1;

    // Smoothed order parameter (prevents sudden reverb parameter jumps)
    float smoothedOrderParam = 0.0f;
};

} // namespace opera
