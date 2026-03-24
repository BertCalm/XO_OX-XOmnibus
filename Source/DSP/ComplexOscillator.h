#pragma once
#include <cmath>
#include <algorithm>
#include "FastMath.h"

namespace xolokun {

//==============================================================================
// ComplexOscillator — Buchla 259-inspired bidirectional FM processor.
//
// Two oscillator signals modulate each other's frequency AND timbre
// simultaneously, creating the distinctive "alive" quality of the original
// Buchla Complex Waveform Generator.
//
// Unlike standard FM where A modulates B's pitch, here:
//   - Osc A's output modulates Osc B's frequency (pitch FM)
//   - Osc B's output modulates Osc A's timbre (waveshape modulation)
//   - Both modulations are simultaneous and bidirectional
//   - A "symmetry" control balances which direction dominates
//
// This is designed as a coupling processor — it sits between two engine
// audio outputs and applies the bidirectional modulation before they're mixed.
//
// Controls:
//   fmDepth:    0..1 — Depth of frequency modulation (A→B pitch)
//   timbreDepth: 0..1 — Depth of timbre modulation (B→A waveshape)
//   symmetry:   0..1 — Balance between A→B and B→A (0.5 = equal)
//   fmRatio:    0.5..8 — Frequency ratio for FM (harmonic intervals)
//   feedback:   0..0.5 — Self-feedback on each oscillator (Buchla-style)
//   mix:        0..1 — Wet/dry blend
//
// Inspired by: Buchla 259 (1966), Make Noise DPO, Verbos Complex Oscillator
//==============================================================================
class ComplexOscillator
{
public:
    ComplexOscillator() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    void setFMDepth (float d)       { fmDepth     = std::clamp (d, 0.0f, 1.0f); }
    void setTimbreDepth (float d)   { timbreDepth  = std::clamp (d, 0.0f, 1.0f); }
    void setSymmetry (float s)      { symmetry     = std::clamp (s, 0.0f, 1.0f); }
    void setFMRatio (float r)       { fmRatio      = std::clamp (r, 0.5f, 8.0f); }
    void setFeedback (float fb)     { feedback     = std::clamp (fb, 0.0f, 0.5f); }
    void setMix (float m)           { mix          = std::clamp (m, 0.0f, 1.0f); }

    /// Process two engine audio streams with bidirectional complex modulation.
    /// @param engineA   Audio from engine slot A (modified in place)
    /// @param engineB   Audio from engine slot B (modified in place)
    /// @param numSamples Block size
    void processBlock (float* engineA, float* engineB, int numSamples)
    {
        if (mix < 0.001f || (fmDepth < 0.001f && timbreDepth < 0.001f))
            return;

        const float aToBScale = symmetry;           // How much A modulates B
        const float bToAScale = 1.0f - symmetry;    // How much B modulates A

        for (int i = 0; i < numSamples; ++i)
        {
            float dryA = engineA[i];
            float dryB = engineB[i];

            float a = engineA[i];
            float b = engineB[i];

            // --- Bidirectional FM ---
            // A → B pitch modulation (through a phase accumulator concept)
            float fmModB = a * fmDepth * aToBScale * fmRatio;

            // Apply FM to B: phase-modulate B's signal
            // We approximate this by time-varying waveshaping
            // SRO: fastCos/fastSin replace std:: trig (per-sample coupling)
            float modulatedB = b * fastCos (fmModB * 3.14159265f)
                             + b * fastSin (fmModB * 3.14159265f) * 0.5f;

            // --- Bidirectional Timbre Mod ---
            // B → A waveshape modulation
            float timbreMod = b * timbreDepth * bToAScale;

            // Apply timbre mod to A: wavefold-like distortion controlled by B
            float foldAmount = std::abs (timbreMod);
            float modulatedA = a;
            if (foldAmount > 0.001f)
            {
                float driven = a * (1.0f + foldAmount * 4.0f);
                // Soft wavefold
                // SRO: fastSin replaces std::sin (per-sample wavefold)
                modulatedA = fastSin (driven) * (1.0f / (1.0f + foldAmount));
                modulatedA = a + foldAmount * (modulatedA - a);
            }

            // --- Self-feedback (Buchla-style) ---
            if (feedback > 0.001f)
            {
                modulatedA += feedback * prevA;
                modulatedB += feedback * prevB;

                // Soft limit to prevent runaway
                modulatedA = softLimit (modulatedA);
                modulatedB = softLimit (modulatedB);
            }

            prevA = modulatedA;
            prevB = modulatedB;

            // Denormal protection
            if (std::abs (prevA) < 1e-15f) prevA = 0.0f;
            if (std::abs (prevB) < 1e-15f) prevB = 0.0f;

            // Mix
            engineA[i] = dryA + mix * (modulatedA - dryA);
            engineB[i] = dryB + mix * (modulatedB - dryB);
        }
    }

    void reset()
    {
        prevA = prevB = 0.0f;
    }

private:
    double sr = 44100.0;
    float fmDepth     = 0.0f;
    float timbreDepth = 0.0f;
    float symmetry    = 0.5f;
    float fmRatio     = 2.0f;
    float feedback    = 0.0f;
    float mix         = 1.0f;

    float prevA = 0.0f, prevB = 0.0f;

    static float softLimit (float x)
    {
        if (x > 1.5f)  return 1.0f;
        if (x < -1.5f) return -1.0f;
        return x - (x * x * x) / 6.75f;
    }
};

} // namespace xolokun
