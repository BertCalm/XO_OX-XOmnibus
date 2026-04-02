// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// FDN.h — xoverlap::FeedbackDelayNetwork
//
// 6-channel Feedback Delay Network at the heart of the OVERLAP engine.
// The feedback matrix is set to a KnotMatrix-derived 6×6 routing matrix,
// producing topologically-shaped signal tangling that varies from simple
// parallel reverb (Unknot) to complex braided diffusion (Torus knot).
//
// Architecture:
//   input[i]  →  delay_line[i]  →  matrix multiply  →  onepole_damp  →  output[i]
//               ↑_______________________________________________________↑
//
// Delay lengths:  delayBase (ms) × delayRatios[i], converted to samples.
//                 Prime-ish lengths chosen at prepare() via Schroeder primes.
// Dampening:      One-pole lowpass per channel — coefficient from dampeningCoeff.
// Feedback:       Scalar gain applied to all matrix outputs before re-injection.
//
// All state is float[]. No heap allocation after prepare().
//==============================================================================

#include "KnotMatrix.h"
#include "FastMath.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace xoverlap {

//==============================================================================
class FeedbackDelayNetwork
{
public:
    //==========================================================================
    // Public members written directly by the adapter
    float feedback      = 0.7f;   // feedback scalar [0, 0.99]
    float dampeningCoeff = 0.5f;  // lowpass dampening amount [0, 1]

    //==========================================================================
    void prepare (double sampleRate) noexcept
    {
        sr = static_cast<float> (sampleRate);

        // Schroeder prime-ish delay lengths (in samples at 44.1 kHz; scaled for sr)
        static const float kBaseLengthsMs[6] = { 29.7f, 37.1f, 41.1f, 43.3f, 47.3f, 53.1f };
        for (int i = 0; i < kChannels; ++i)
        {
            baseLengthsSamples[i] = kBaseLengthsMs[i] * 0.001f * sr;
            currentLengths[i]     = static_cast<int> (baseLengthsSamples[i]);
        }

        // Allocate delay buffers — max 200ms per channel
        int maxLen = static_cast<int> (0.2f * sr) + 2;
        for (int i = 0; i < kChannels; ++i)
        {
            delayBuffers[i].assign (static_cast<size_t> (maxLen), 0.0f);
            writePos[i] = 0;
        }

        // Init dampening state and output
        dampState.fill (0.0f);
        outputs.fill (0.0f);

        // Identity matrix as default
        matrix = KnotMatrix::unknot();
    }

    //==========================================================================
    // setMatrix() — called once per block after knot topology changes
    void setMatrix (const KnotMatrix::Matrix& m) noexcept
    {
        matrix = m;
    }

    //==========================================================================
    // setDelayBase() — recalculate delay lengths from base (ms) × ratios
    void setDelayBase (float baseMs, double /*sampleRate*/,
                       const std::array<float, 6>& ratios) noexcept
    {
        for (int i = 0; i < kChannels; ++i)
        {
            float lenSamples = baseMs * 0.001f * sr * ratios[static_cast<size_t>(i)];
            // Clamp to buffer capacity
            int maxLen = static_cast<int> (delayBuffers[static_cast<size_t>(i)].size()) - 1;
            currentLengths[i] = std::max (1, std::min (static_cast<int> (lenSamples), maxLen));
        }
    }

    //==========================================================================
    // process() — advance one sample.
    // fdnInputs: external excitation from voice outputs.
    void process (const std::array<float, 6>& fdnInputs) noexcept
    {
        // 1. Read delayed samples from each channel
        std::array<float, kChannels> delayed{};
        for (int i = 0; i < kChannels; ++i)
        {
            auto& buf = delayBuffers[static_cast<size_t>(i)];
            int   len = currentLengths[i];
            int   rp  = writePos[i] - len;
            if (rp < 0) rp += static_cast<int> (buf.size());
            delayed[static_cast<size_t>(i)] = buf[static_cast<size_t>(rp)];
        }

        // 2. Apply dampening (one-pole lowpass per channel)
        //    cutoff increases as dampeningCoeff → 0 (less damping)
        float dampAlpha = 0.5f + dampeningCoeff * 0.45f;  // 0.5..0.95
        for (int i = 0; i < kChannels; ++i)
        {
            dampState[static_cast<size_t>(i)] = dampAlpha * dampState[static_cast<size_t>(i)]
                + (1.0f - dampAlpha) * delayed[static_cast<size_t>(i)];
            delayed[static_cast<size_t>(i)] = flushDenormal (dampState[static_cast<size_t>(i)]);
        }

        // 3. Matrix multiply: mixed[i] = Σ_j matrix[i][j] * delayed[j]
        std::array<float, kChannels> mixed{};
        for (int i = 0; i < kChannels; ++i)
        {
            float sum = 0.0f;
            for (int j = 0; j < kChannels; ++j)
                sum += matrix[static_cast<size_t>(i)][static_cast<size_t>(j)]
                     * delayed[static_cast<size_t>(j)];
            mixed[static_cast<size_t>(i)] = sum;
        }

        // 4. Write back: input + feedback * mixed
        float fb = std::max (0.0f, std::min (0.99f, feedback));
        for (int i = 0; i < kChannels; ++i)
        {
            auto& buf = delayBuffers[static_cast<size_t>(i)];
            float writeVal = fdnInputs[static_cast<size_t>(i)] + fb * mixed[static_cast<size_t>(i)];
            buf[static_cast<size_t>(writePos[i])] = flushDenormal (writeVal);
            if (++writePos[i] >= static_cast<int> (buf.size())) writePos[i] = 0;
        }

        // 5. Outputs are the matrix-mixed (pre-feedback) signals
        outputs = mixed;
    }

    //==========================================================================
    // getOutput() — retrieve per-channel output for stereo panning
    float getOutput (int channel) const noexcept
    {
        return outputs[static_cast<size_t>(channel)];
    }

private:
    //==========================================================================
    static constexpr int kChannels = 6;

    float sr = 44100.0f;

    KnotMatrix::Matrix matrix;

    // Per-channel delay buffers (heap-allocated once in prepare)
    std::array<std::vector<float>, kChannels> delayBuffers;
    std::array<int,   kChannels> writePos           {};
    std::array<float, kChannels> baseLengthsSamples {};
    std::array<int,   kChannels> currentLengths      {};

    // Dampening one-pole state
    std::array<float, kChannels> dampState {};

    // Per-channel outputs (updated each process() call)
    std::array<float, kChannels> outputs {};
};

} // namespace xoverlap
