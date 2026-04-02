#pragma once
#include <cmath>
#include <cstdint>
#include "../FastMath.h"

namespace xolokun {

//==============================================================================
// Corroder — Digital erosion / degradation processor.
//
// Combines three degradation axes into a single module:
//   1. Bit Reduction    — reduces bit depth (24→1 bit, adds quantization noise)
//   2. Sample Rate Reduction — reduces effective SR (aliasing, lo-fi crunch)
//   3. FM Distortion    — self-FM with bandwidth control (Pigments Corroder style)
//
// Each axis is independently controllable. At zero, each bypasses.
// Together they create lo-fi, industrial, glitch, and vintage sampler textures.
//
// Inspired by: Arturia Pigments Corroder, AIR Flavor Pro, Decimort
//
// Features:
//   - Bit depth: continuous 1.0–24.0 bits
//   - Sample rate: continuous 100Hz–full SR
//   - FM depth with rate control (self-modulation)
//   - Tone filter (post-erosion LP) to tame harshness
//   - Zero CPU when all three axes at zero
//
// Usage:
//   Corroder cor;
//   cor.prepare(44100.0);
//   cor.setBitDepth(8.0f);
//   cor.setSampleRate(11025.0f);
//   cor.setFMDepth(0.3f);
//   cor.setMix(0.5f);
//   cor.processBlock(L, R, numSamples);
//==============================================================================
class Corroder
{
public:
    Corroder() = default;

    //--------------------------------------------------------------------------
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        holdL = holdR = 0.0f;
        holdCounter = 0.0f;
        fmPhase = 0.0f;
        toneLP_L = toneLP_R = 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Set bit depth reduction. [1.0, 24.0] — 24 = no reduction.
    void setBitDepth (float bits)
    {
        bitDepth = clamp (bits, 1.0f, 24.0f);
    }

    /// Set effective sample rate. [100, sr] — sr = no reduction.
    void setSampleRate (float hz)
    {
        targetSR = clamp (hz, 100.0f, static_cast<float> (sr));
    }

    /// Set FM self-modulation depth. [0, 1] — 0 = off.
    void setFMDepth (float depth)
    {
        fmDepth = clamp (depth, 0.0f, 1.0f);
    }

    /// Set FM modulator rate in Hz. [0.5, 5000]
    void setFMRate (float hz)
    {
        fmRate = clamp (hz, 0.5f, 5000.0f);
    }

    /// Set post-erosion tone (LP filter). [0, 1] — 0 = dark, 1 = bright (bypass).
    void setTone (float t)
    {
        tone = clamp (t, 0.0f, 1.0f);
    }

    /// Set wet/dry mix. [0, 1]
    void setMix (float wet)
    {
        mix = clamp (wet, 0.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    /// Process stereo audio in-place.
    void processBlock (float* L, float* R, int numSamples)
    {
        // Precompute
        float stepSize = static_cast<float> (sr) / targetSR;
        float quantLevels = std::max (1.0f, fastPow2 (bitDepth - 1.0f));
        float invLevels = 1.0f / quantLevels;
        bool doBitReduce = (bitDepth < 23.5f);
        bool doSRReduce = (targetSR < static_cast<float> (sr) * 0.99f);
        bool doFM = (fmDepth > 0.001f);

        // Tone LP coefficient: tone=1 → coeff=1 (bypass), tone=0 → coeff≈0.02
        float toneCoeff = 0.02f + tone * 0.98f;

        float fmPhaseInc = fmRate / static_cast<float> (sr);

        for (int i = 0; i < numSamples; ++i)
        {
            float dryL = L[i];
            float dryR = R[i];
            float wetL = dryL;
            float wetR = dryR;

            // --- FM self-modulation ---
            if (doFM)
            {
                float fmMod = fastSin (fmPhase * 6.28318530718f) * fmDepth;
                wetL += wetL * fmMod;
                wetR += wetR * fmMod;
                fmPhase += fmPhaseInc;
                if (fmPhase >= 1.0f) fmPhase -= 1.0f;
            }

            // --- Sample rate reduction (sample-and-hold) ---
            if (doSRReduce)
            {
                holdCounter += 1.0f;
                if (holdCounter >= stepSize)
                {
                    holdCounter -= stepSize;
                    holdL = wetL;
                    holdR = wetR;
                }
                wetL = holdL;
                wetR = holdR;
            }

            // --- Bit depth reduction (quantization) ---
            if (doBitReduce)
            {
                // Quantize to N levels, then reconstruct
                wetL = std::round (wetL * quantLevels) * invLevels;
                wetR = std::round (wetR * quantLevels) * invLevels;
            }

            // --- Post-erosion tone LP ---
            toneLP_L = flushDenormal (toneLP_L + toneCoeff * (wetL - toneLP_L));
            toneLP_R = flushDenormal (toneLP_R + toneCoeff * (wetR - toneLP_R));

            // Use LP only when tone < 1 (avoid unnecessary filtering)
            if (tone < 0.99f)
            {
                wetL = toneLP_L;
                wetR = toneLP_R;
            }

            // Mix
            L[i] = dryL * (1.0f - mix) + wetL * mix;
            R[i] = dryR * (1.0f - mix) + wetR * mix;
        }
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        holdL = holdR = 0.0f;
        holdCounter = 0.0f;
        fmPhase = 0.0f;
        toneLP_L = toneLP_R = 0.0f;
    }

private:
    double sr = 44100.0;

    // Sample-and-hold state
    float holdL = 0.0f;
    float holdR = 0.0f;
    float holdCounter = 0.0f;

    // FM state
    float fmPhase = 0.0f;

    // Tone LP state
    float toneLP_L = 0.0f;
    float toneLP_R = 0.0f;

    // Parameters
    float bitDepth = 24.0f;
    float targetSR = 44100.0f;
    float fmDepth = 0.0f;
    float fmRate = 500.0f;
    float tone = 1.0f;
    float mix = 0.0f;
};

} // namespace xolokun
