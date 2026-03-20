#pragma once
// XOverlap FeedbackDelayNetwork — 6-line FDN with knot-topology routing matrix.
//
// Architecture:
//   Each delay line is a circular buffer. Per sample:
//     1. Read delayed output from each line.
//     2. Apply 1-pole lowpass dampening to each read value.
//     3. Multiply vector of dampened reads by the routing matrix (matrix multiply).
//     4. Multiply result by feedback factor.
//     5. Add the external input signal for each line.
//     6. Write combined value into each delay buffer.
//     7. getOutput(i) returns the read value (= delayed signal before matrix mix).
//
// The routing matrix (set by setMatrix) determines the topological coupling.
// Tangle depth interpolates identity → full knot routing.
// Dampening coefficient controls 1-pole LP cutoff (0=none, ~1=heavy dark).
// Feedback coefficient (0–0.99) controls self-resonance intensity.
//
// Max delay per line: ~85ms at 96kHz (kMaxDelay=8192 samples).
// Denormal protection: flushDenormal() applied per write.

#include "KnotMatrix.h"
#include "FastMath.h"   // provides xoverlap::flushDenormal via alias from xomnibus
#include <array>
#include <cmath>
#include <algorithm>

namespace xoverlap {

//==============================================================================
class FeedbackDelayNetwork
{
public:
    // Public members set by the adapter each block
    float feedback      = 0.7f;   // overall FDN feedback gain (0–0.99)
    float dampeningCoeff = 0.5f;  // 1-pole LP coeff (0=none, approaches 1=heavy)

    //==========================================================================
    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < kMaxDelay; ++j)
                delayBuf[i][j] = 0.0f;
        for (int i = 0; i < 6; ++i) { lpState[i] = 0.0f; state[i] = 0.0f; }
        for (int i = 0; i < 6; ++i) { writePos[i] = 0; delayLen[i] = 512; }
        matrix = KnotMatrix::unknot();
    }

    //==========================================================================
    void setMatrix(const KnotMatrix::Matrix& m) noexcept
    {
        matrix = m;
    }

    //==========================================================================
    // Set delay lengths from base time (ms) + per-voice ratio array.
    // delayMs range: 1–50ms. Ratios should be in roughly 0.3–2.0 range.
    void setDelayBase(float delayMs, float sampleRate,
                      const std::array<float, 6>& ratios) noexcept
    {
        int base = static_cast<int>(delayMs * sampleRate / 1000.0f);
        base     = std::max(4, std::min(base, kMaxDelay / 2));

        for (int i = 0; i < 6; ++i)
        {
            int len = static_cast<int>(static_cast<float>(base) * ratios[static_cast<size_t>(i)]);
            delayLen[static_cast<size_t>(i)] = std::max(4, std::min(len, kMaxDelay - 2));
        }
    }

    //==========================================================================
    // Process one sample block-element: read, matrix-mix, dampening, write.
    // inputs: per-voice signal from bell oscillators.
    void process(const std::array<float, 6>& inputs) noexcept
    {
        // Step 1: Read delayed outputs from each line
        for (int i = 0; i < 6; ++i)
        {
            int rdPos = (writePos[static_cast<size_t>(i)]
                         - delayLen[static_cast<size_t>(i)] + kMaxDelay) % kMaxDelay;
            state[static_cast<size_t>(i)] = delayBuf[static_cast<size_t>(i)][rdPos];
        }

        // Step 2: Matrix multiply: fbMix[i] = sum_j( matrix[i][j] * state[j] )
        float fbMix[6] = {};
        for (int row = 0; row < 6; ++row)
            for (int col = 0; col < 6; ++col)
                fbMix[row] += matrix[static_cast<size_t>(row)][static_cast<size_t>(col)]
                              * state[static_cast<size_t>(col)];

        // Step 3–6: Dampening LP, add input, write
        float lpCoeff = dampeningCoeff * 0.97f;  // max coefficient 0.97 for stability

        for (int i = 0; i < 6; ++i)
        {
            // Combine: external input + matrix-routed feedback
            float newVal = inputs[static_cast<size_t>(i)]
                           + fbMix[i] * feedback;

            // 1-pole lowpass: y = y_prev * c + x * (1-c)
            lpState[static_cast<size_t>(i)] =
                lpState[static_cast<size_t>(i)] * lpCoeff
                + newVal * (1.0f - lpCoeff);

            // Denormal flush
            lpState[static_cast<size_t>(i)] =
                flushDenormal(lpState[static_cast<size_t>(i)]);

            // Write to delay buffer
            delayBuf[static_cast<size_t>(i)][writePos[static_cast<size_t>(i)]] =
                lpState[static_cast<size_t>(i)];

            writePos[static_cast<size_t>(i)] =
                (writePos[static_cast<size_t>(i)] + 1) % kMaxDelay;
        }
    }

    //==========================================================================
    // Returns the delayed output of delay line i (set during process()).
    float getOutput(int i) const noexcept
    {
        return state[static_cast<size_t>(i)];
    }

private:
    static constexpr int kMaxDelay = 8192;  // max ~85ms at 96kHz

    KnotMatrix::Matrix          matrix{};
    std::array<float, 6>        state{};
    std::array<float, 6>        lpState{};
    std::array<int, 6>          writePos{};
    std::array<int, 6>          delayLen{};
    float delayBuf[6][kMaxDelay]{};

    float sr = 44100.0f;
};

} // namespace xoverlap
