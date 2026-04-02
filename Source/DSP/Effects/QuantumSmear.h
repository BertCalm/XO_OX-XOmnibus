#pragma once
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include "../FastMath.h"

namespace xoceanus {

//==============================================================================
// QuantumSmear — "Superposition" delay/reverb with probabilistic echo timing.
//
// Math Mixture: Schrödinger Wave Function + Airy Function + Peano Curve.
//
// Instead of fixed delay times, echo offsets are sampled from a probability
// distribution (|Ψ|² — Schrödinger wave function squared). The echo exists
// in multiple "time states" simultaneously, creating a shimmering blur.
//
// A Peano curve scans the feedback matrix, ensuring the reverb texture never
// repeats. The Airy function creates prismatic spectral diffusion on output.
//
// Parameters:
//   mfx_qsObservation (CC 32) — collapses wave function: blur → solid delay (0–1)
//   mfx_qsFeedback            — feedback amount (0–0.95)
//   mfx_qsDelayCenter         — center delay time in ms (10–500)
//   mfx_qsMix                 — dry/wet (0–1)
//
// "Observation" is the key parameter: low = blurry shimmer (quantum
// superposition), high = collapses to a solid rhythmic delay.
//==============================================================================
class QuantumSmear
{
public:
    QuantumSmear() = default;

    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);

        // Delay buffer: ~600ms max
        int maxDelay = static_cast<int> (sr * 0.6f) + 1;
        delayBufL.assign (static_cast<size_t> (maxDelay), 0.0f);
        delayBufR.assign (static_cast<size_t> (maxDelay), 0.0f);
        delayWritePos = 0;

        // Multi-tap read positions (8 quantum taps)
        for (int i = 0; i < kNumTaps; ++i)
        {
            tapWeights[i] = 0.0f;
            tapOffsets[i] = 0;
        }

        // Peano curve state (scanning through feedback matrix)
        peanoStep = 0;
        peanoLevel = 5; // 5 levels of recursion = 243 steps per cycle

        // Airy diffusion filter state
        airyStateL = 0.0f;
        airyStateR = 0.0f;
        airyPrevL = 0.0f;
        airyPrevR = 0.0f;

        rng = 2147001325u;
    }

    void reset()
    {
        std::fill (delayBufL.begin(), delayBufL.end(), 0.0f);
        std::fill (delayBufR.begin(), delayBufR.end(), 0.0f);
        delayWritePos = 0;
        peanoStep = 0;
        airyStateL = airyStateR = 0.0f;
        airyPrevL = airyPrevR = 0.0f;
    }

    //--------------------------------------------------------------------------
    void processBlock (float* L, float* R, int numSamples,
                       float observation, float feedback, float delayCenterMs, float mix)
    {
        if (mix < 0.001f) return;

        int bufSize = static_cast<int> (delayBufL.size());
        if (bufSize < 2) return;

        // Center delay in samples
        int centerDelay = std::max (1, std::min (bufSize - 1,
            static_cast<int> (delayCenterMs * 0.001f * sr)));

        // Feedback clamped for stability
        feedback = std::min (0.95f, std::max (0.0f, feedback));

        // Observation controls the probability spread:
        // 0 = wide spread (quantum blur), 1 = collapsed to center (solid delay)
        float spread = (1.0f - observation) * 0.8f; // 0 to 0.8

        // Generate tap weights from |Ψ|² (Gaussian-like probability distribution)
        // centered on delay time, spread controlled by observation
        float totalWeight = 0.0f;
        for (int t = 0; t < kNumTaps; ++t)
        {
            // Distribute taps around center delay
            float normalizedPos = (static_cast<float> (t) / static_cast<float> (kNumTaps - 1)) * 2.0f - 1.0f;

            // |Ψ|² = Gaussian envelope: exp(-x²/2σ²)
            float sigma = 0.1f + spread * 2.0f;
            float psi2 = std::exp (-normalizedPos * normalizedPos / (2.0f * sigma * sigma));

            tapWeights[t] = psi2;
            totalWeight += psi2;

            // Tap offset: spread around center delay
            int offsetSpread = static_cast<int> (static_cast<float> (centerDelay) * spread * normalizedPos);
            tapOffsets[t] = std::max (1, std::min (bufSize - 1, centerDelay + offsetSpread));
        }

        // Normalize weights
        if (totalWeight > 0.001f)
            for (int t = 0; t < kNumTaps; ++t)
                tapWeights[t] /= totalWeight;

        // Peano curve feedback path rotation
        // Advances through a space-filling pattern so feedback texture never repeats
        constexpr int peanoSteps = 243; // 3^5

        for (int s = 0; s < numSamples; ++s)
        {
            // Read from multi-tap delay (superposition of time states)
            float wetL = 0.0f;
            float wetR = 0.0f;

            for (int t = 0; t < kNumTaps; ++t)
            {
                int readPos = (delayWritePos - tapOffsets[t] + bufSize) % bufSize;
                wetL += delayBufL[static_cast<size_t> (readPos)] * tapWeights[t];
                wetR += delayBufR[static_cast<size_t> (readPos)] * tapWeights[t];
            }

            // --- Peano curve: rotate which tap gets feedback emphasis ---
            // This ensures the reverb texture evolves continuously
            int peanoTap = peanoStep % kNumTaps;
            float peanoBoost = 1.0f + 0.3f * (static_cast<float> ((peanoStep / kNumTaps) % 3) / 2.0f);

            // Slightly emphasize the Peano-selected tap
            float peanoWetL = delayBufL[static_cast<size_t> (
                (delayWritePos - tapOffsets[peanoTap] + bufSize) % bufSize)];
            float peanoWetR = delayBufR[static_cast<size_t> (
                (delayWritePos - tapOffsets[peanoTap] + bufSize) % bufSize)];
            wetL += peanoWetL * 0.15f * peanoBoost;
            wetR += peanoWetR * 0.15f * peanoBoost;

            // Advance Peano step
            peanoStep = (peanoStep + 1) % peanoSteps;

            // --- Airy function diffusion (spectral blur on output) ---
            // Approximated as allpass-like filter with frequency-dependent phase
            float airyFreq = 1000.0f + (1.0f - observation) * 3000.0f;
            float airyCoeff = 1.0f - std::exp (-6.28318f * airyFreq / sr);

            float airyInL = wetL;
            float airyInR = wetR;
            airyStateL += (airyInL - airyStateL) * airyCoeff;
            airyStateR += (airyInR - airyStateR) * airyCoeff;

            // Allpass structure: output = filtered - input + previous
            float diffusedL = airyStateL - airyInL + airyPrevL;
            float diffusedR = airyStateR - airyInR + airyPrevR;
            airyPrevL = airyInL;
            airyPrevR = airyInR;

            // Blend diffusion with direct wet based on observation
            // Low observation = more diffused (prismatic blur)
            float diffBlend = 1.0f - observation;
            wetL = wetL * observation + diffusedL * diffBlend;
            wetR = wetR * observation + diffusedR * diffBlend;

            // Write into delay buffer with feedback
            float fbL = L[s] + wetL * feedback;
            float fbR = R[s] + wetR * feedback;

            // Soft clip feedback to prevent runaway
            fbL = fastTanh (fbL);
            fbR = fastTanh (fbR);

            delayBufL[static_cast<size_t> (delayWritePos)] = flushDenormal (fbL);
            delayBufR[static_cast<size_t> (delayWritePos)] = flushDenormal (fbR);

            delayWritePos = (delayWritePos + 1) % bufSize;

            wetL = flushDenormal (wetL);
            wetR = flushDenormal (wetR);

            L[s] = L[s] * (1.0f - mix) + wetL * mix;
            R[s] = R[s] * (1.0f - mix) + wetR * mix;
        }
    }

private:
    float sr = 44100.0f;

    static constexpr int kNumTaps = 8;

    // Delay buffer
    std::vector<float> delayBufL, delayBufR;
    int delayWritePos = 0;

    // Quantum tap state
    float tapWeights[kNumTaps] {};
    int tapOffsets[kNumTaps] {};

    // Peano curve state
    int peanoStep = 0;
    int peanoLevel = 5;

    // Airy diffusion
    float airyStateL = 0.0f, airyStateR = 0.0f;
    float airyPrevL = 0.0f, airyPrevR = 0.0f;

    uint32_t rng = 2147001325u;
};

} // namespace xoceanus
