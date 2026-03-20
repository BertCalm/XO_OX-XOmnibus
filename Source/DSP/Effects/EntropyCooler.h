#pragma once
#include <cmath>
#include <array>
#include <algorithm>
#include "../FastMath.h"

namespace xomnibus {

//==============================================================================
// EntropyCooler — "Smart" lowpass filter governed by competing math systems.
//
// Math Mixture: Heat Equation + Shannon Entropy + Maxwell's Demon.
//
// The Heat Equation treats the audio spectrum as a temperature field,
// naturally "cooling" (smoothing) high frequencies. Shannon Entropy measures
// signal complexity. When entropy exceeds a threshold, Maxwell's Demon
// reverses the cooling — re-injecting high-frequency "heat" (grit/noise)
// to prevent the sound from becoming too simple.
//
// The result: an adaptive filter that smooths predictable signals but
// fights back against over-simplification with bursts of spectral energy.
//
// Parameters:
//   mfx_ecStability (CC 30) — how hard the Demon fights cooling (0–1)
//   mfx_ecCoolRate           — heat equation diffusion rate (0–1)
//   mfx_ecThreshold          — entropy level that triggers the Demon (0–1)
//   mfx_ecMix                — dry/wet (0–1)
//
// DSP: circular buffer for entropy window, 1-pole LP cascade for heat
// equation, RNG for demon heat injection, DC blocker on output.
//==============================================================================
class EntropyCooler
{
public:
    EntropyCooler() = default;

    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);

        // Heat equation state (2-stage 1-pole cascade for steeper roll-off)
        for (auto& s : heatStateL) s = 0.0f;
        for (auto& s : heatStateR) s = 0.0f;

        // Entropy analysis window (short — ~5ms)
        entropyWindowSize = std::max (32, static_cast<int> (sr * 0.005f));
        entropyBufL.assign (static_cast<size_t> (entropyWindowSize), 0.0f);
        entropyBufR.assign (static_cast<size_t> (entropyWindowSize), 0.0f);
        entropyWritePos = 0;

        // Demon state
        demonActiveL = 0.0f;
        demonActiveR = 0.0f;
        rng = 73856093u;

        // DC blocker
        dcStateL = 0.0f;
        dcStateR = 0.0f;
        dcPrevL = 0.0f;
        dcPrevR = 0.0f;

        smoothedEntropy = 0.0f;
    }

    void reset()
    {
        for (auto& s : heatStateL) s = 0.0f;
        for (auto& s : heatStateR) s = 0.0f;
        std::fill (entropyBufL.begin(), entropyBufL.end(), 0.0f);
        std::fill (entropyBufR.begin(), entropyBufR.end(), 0.0f);
        entropyWritePos = 0;
        demonActiveL = 0.0f;
        demonActiveR = 0.0f;
        dcStateL = dcStateR = dcPrevL = dcPrevR = 0.0f;
        smoothedEntropy = 0.0f;
    }

    //--------------------------------------------------------------------------
    // Process stereo block in-place.
    void processBlock (float* L, float* R, int numSamples,
                       float stability, float coolRate, float threshold, float mix)
    {
        if (mix < 0.001f) return;

        // Heat equation diffusion coefficient: higher coolRate = more smoothing
        // α∇²u approximated as 1-pole LP with coefficient proportional to coolRate
        float heatCoeff = 1.0f - std::exp (-6.28318f * (200.0f + coolRate * 6000.0f) / sr);

        // Demon heat injection intensity
        float demonPower = stability * 0.6f;

        // Entropy smoothing coefficient
        float entSmooth = 1.0f - std::exp (-6.28318f * 15.0f / sr);

        for (int s = 0; s < numSamples; ++s)
        {
            float dryL = L[s];
            float dryR = R[s];

            // --- Record into entropy analysis buffer ---
            if (!entropyBufL.empty())
            {
                entropyBufL[static_cast<size_t> (entropyWritePos)] = dryL;
                entropyBufR[static_cast<size_t> (entropyWritePos)] = dryR;
                entropyWritePos = (entropyWritePos + 1) % entropyWindowSize;
            }

            // --- Compute Shannon Entropy (approximated via amplitude histogram) ---
            // Recompute every 32 samples to avoid O(N) histogram per sample.
            // Smoothing already low-passes the result, so 32-sample granularity is inaudible.
            constexpr int kEntropyInterval = 32;
            if (entropySampleCounter == 0)
                cachedEntropy = computeEntropy();
            entropySampleCounter = (entropySampleCounter + 1) % kEntropyInterval;
            smoothedEntropy += (cachedEntropy - smoothedEntropy) * entSmooth;

            // --- Heat Equation: 2-stage 1-pole lowpass cascade ---
            // Each stage: y[n] = y[n-1] + coeff * (x[n] - y[n-1])
            heatStateL[0] += (dryL - heatStateL[0]) * heatCoeff;
            heatStateL[1] += (heatStateL[0] - heatStateL[1]) * heatCoeff;
            heatStateR[0] += (dryR - heatStateR[0]) * heatCoeff;
            heatStateR[1] += (heatStateR[0] - heatStateR[1]) * heatCoeff;

            float cooledL = heatStateL[1];
            float cooledR = heatStateR[1];

            // --- Maxwell's Demon: entropy-triggered heat injection ---
            // When entropy exceeds threshold, re-inject high-frequency energy
            bool demonTriggered = smoothedEntropy > (0.2f + threshold * 0.7f);

            // Demon attack/release envelope
            float demonTarget = demonTriggered ? 1.0f : 0.0f;
            float demonAttack = 1.0f - std::exp (-1.0f / (0.002f * sr));  // 2ms
            float demonRelease = 1.0f - std::exp (-1.0f / (0.05f * sr));  // 50ms
            float demonCoeff = (demonTarget > demonActiveL) ? demonAttack : demonRelease;
            demonActiveL += (demonTarget - demonActiveL) * demonCoeff;
            demonActiveR = demonActiveL; // mono demon for coherent response

            float wetL = cooledL;
            float wetR = cooledR;

            if (demonActiveL > 0.01f)
            {
                // Re-inject "heat": the difference between dry and cooled (= HF content)
                // plus a dash of shaped noise for spectral grit
                float hfL = dryL - cooledL;
                float hfR = dryR - cooledR;

                rng = rng * 1664525u + 1013904223u;
                float noiseL = static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f;
                rng = rng * 1664525u + 1013904223u;
                float noiseR = static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f;

                float heatL = hfL + noiseL * std::fabs (dryL) * 0.15f;
                float heatR = hfR + noiseR * std::fabs (dryR) * 0.15f;

                wetL += heatL * demonActiveL * demonPower;
                wetR += heatR * demonActiveR * demonPower;
            }

            // --- DC blocker (high-pass at ~5 Hz) ---
            float dcCoeff = 1.0f - (6.28318f * 5.0f / sr);
            float outL = wetL - dcPrevL + dcStateL * dcCoeff;
            float outR = wetR - dcPrevR + dcStateR * dcCoeff;
            dcPrevL = wetL;
            dcPrevR = wetR;
            dcStateL = outL;
            dcStateR = outR;

            outL = flushDenormal (outL);
            outR = flushDenormal (outR);

            L[s] = dryL * (1.0f - mix) + outL * mix;
            R[s] = dryR * (1.0f - mix) + outR * mix;
        }
    }

private:
    float sr = 44100.0f;

    // Heat equation filter state (2-stage cascade, stereo)
    float heatStateL[2] {};
    float heatStateR[2] {};

    // Entropy analysis
    std::vector<float> entropyBufL, entropyBufR;
    int entropyWindowSize = 256;
    int entropyWritePos = 0;
    float smoothedEntropy = 0.0f;
    float cachedEntropy = 0.0f;      // Last computed entropy value (updated every kEntropyInterval samples)
    int entropySampleCounter = 0;    // Counts samples since last entropy recompute

    // Demon state
    float demonActiveL = 0.0f;
    float demonActiveR = 0.0f;
    uint32_t rng = 73856093u;

    // DC blocker
    float dcStateL = 0.0f, dcStateR = 0.0f;
    float dcPrevL = 0.0f, dcPrevR = 0.0f;

    //--------------------------------------------------------------------------
    // Shannon entropy approximation over amplitude histogram (16 bins).
    // Returns 0.0 (silence/DC) to ~4.0 (maximum complexity/noise).
    float computeEntropy() const
    {
        if (entropyBufL.empty()) return 0.0f;

        constexpr int kBins = 16;
        int counts[kBins] {};
        int total = entropyWindowSize;

        for (int i = 0; i < total; ++i)
        {
            // Map sample to bin: clamp to ±1, quantize to 0..15
            float sample = (entropyBufL[static_cast<size_t> (i)]
                          + entropyBufR[static_cast<size_t> (i)]) * 0.5f;
            sample = std::max (-1.0f, std::min (1.0f, sample));
            int bin = static_cast<int> ((sample + 1.0f) * 0.5f * static_cast<float> (kBins - 1));
            bin = std::max (0, std::min (kBins - 1, bin));
            counts[bin]++;
        }

        // H = -Σ p(x) * log2(p(x))
        float H = 0.0f;
        float invTotal = 1.0f / static_cast<float> (total);
        for (int b = 0; b < kBins; ++b)
        {
            if (counts[b] == 0) continue;
            float p = static_cast<float> (counts[b]) * invTotal;
            H -= p * std::log2 (p);
        }
        return H;
    }
};

} // namespace xomnibus
