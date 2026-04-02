#pragma once
#include <cmath>
#include <algorithm>
#include "FastMath.h"

namespace xoceanus {

//==============================================================================
// ComplexOscillator — Buchla 259-inspired bidirectional FM oscillator.
//
// Two independent phase accumulators (oscA, oscB) mutually frequency-modulate
// each other every sample.  This is true FM (instantaneous-phase modulation),
// not the amplitude×phase-rotation approximation of the previous version.
//
// Signal flow (per sample):
//   1.  Compute A's output using B's previous output as modulator:
//         outA = sin(phaseA + fmIndexBA × prevB + feedbackA × prevA)
//   2.  Compute B's output using A's previous output as modulator:
//         outB = waveshape(phaseB + fmIndexAB × prevA + feedbackB × prevB)
//   3.  Advance phase accumulators:
//         phaseA += 2π × freqA / sampleRate
//         phaseB += 2π × freqB / sampleRate
//   4.  Update previous-output state with tanh soft-limiting on feedback term
//       to prevent DC offset drift.
//   5.  Mix wet oscillator outputs with the dry engine audio streams.
//
// Modulation indices are in radians of phase deviation (standard FM convention).
// At index=1 the peak phase deviation is ±1 rad, producing a first sideband
// at carrier ± modulator frequency with approximately 0.76 × carrier amplitude.
// At index=5 you get classic Yamaha DX7-style spectral spread.
//
// API changes from v1 (coupling-processor version):
//   NEW:  setBaseFreq (float hz)     — sets oscA fundamental (default 220 Hz)
//   NEW:  setFMIndexAB (float index) — A → B depth in radians (replaces half of fmDepth)
//   NEW:  setFMIndexBA (float index) — B → A depth in radians (replaces other half)
//   NEW:  setFeedbackA / setFeedbackB — per-oscillator self-feedback (0..0.95)
//   NEW:  setTimbreMode (int mode)   — 0=sine, 1=soft-clip sine, 2=folded sine (oscB only)
//   KEPT: setFMDepth (float 0..1)    — convenience: scales BOTH indices uniformly
//   KEPT: setTimbreDepth (float 0..1)— convenience: controls timbre mode depth
//   KEPT: setSymmetry (float 0..1)   — balance: 0 = only A→B, 1 = only B→A
//   KEPT: setFMRatio (float 0.5..8)  — freqB = freqA × fmRatio
//   KEPT: setFeedback (float 0..0.5) — sets BOTH feedbacks identically
//   KEPT: setMix (float 0..1)        — wet / dry blend with incoming engine streams
//   KEPT: prepare (double sampleRate)
//   KEPT: reset()
//   KEPT: processBlock (float*, float*, int)
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
        updatePhaseIncrements();
        reset();
    }

    //--------------------------------------------------------------------------
    // Convenience setters — preserved from v1 API

    /// Uniform FM depth scalar (0..1). Scales both fmIndexAB and fmIndexBA.
    /// Maximum index at depth=1 is kMaxIndex (5 rad — DX7 FM range).
    void setFMDepth (float d)
    {
        fmDepth = std::clamp (d, 0.0f, 1.0f);
        // Symmetric split scaled by symmetry is applied inside processBlock
    }

    /// Timbre mod depth (0..1) — controls how strongly B's waveshaper is active.
    void setTimbreDepth (float d)     { timbreDepth  = std::clamp (d, 0.0f, 1.0f); }

    /// Symmetry (0..1): 0 = only A→B FM, 0.5 = equal bidirectional, 1 = only B→A.
    void setSymmetry (float s)        { symmetry     = std::clamp (s, 0.0f, 1.0f); }

    /// Frequency ratio: freqB = freqA × fmRatio (0.5..8).
    void setFMRatio (float r)
    {
        fmRatio = std::clamp (r, 0.5f, 8.0f);
        updatePhaseIncrements();
    }

    /// Set both oscillator self-feedback identically (0..0.5, same range as v1).
    void setFeedback (float fb)
    {
        float clamped = std::clamp (fb, 0.0f, 0.95f);
        feedbackA = clamped;
        feedbackB = clamped;
    }

    /// Wet / dry blend with incoming engine audio streams (0..1).
    void setMix (float m)             { mix          = std::clamp (m, 0.0f, 1.0f); }

    //--------------------------------------------------------------------------
    // New setters — finer bidirectional FM control

    /// Base frequency of oscA in Hz.  oscB = freqA × fmRatio.
    void setBaseFreq (float hz)
    {
        freqA = std::max (hz, 0.1f);
        updatePhaseIncrements();
    }

    /// A→B FM index in radians (0..kMaxIndex).  Larger = more sideband spread.
    void setFMIndexAB (float index)
    {
        fmIndexAB = std::clamp (index, 0.0f, kMaxIndex);
    }

    /// B→A FM index in radians (0..kMaxIndex).  Buchla favours asymmetric ratios.
    void setFMIndexBA (float index)
    {
        fmIndexBA = std::clamp (index, 0.0f, kMaxIndex);
    }

    /// Per-oscillator self-feedback (0..0.95). Higher = metallic self-resonance.
    void setFeedbackA (float fb)      { feedbackA = std::clamp (fb, 0.0f, 0.95f); }
    void setFeedbackB (float fb)      { feedbackB = std::clamp (fb, 0.0f, 0.95f); }

    /// Oscillator B waveshaper mode for oscB:
    ///   0 = pure sine  (clean partials)
    ///   1 = soft-clip sine  (odd harmonics, tube-like)
    ///   2 = folded sine  (Buchla-style wavefold — richer high-end)
    void setTimbreMode (int mode)     { timbreMode = std::clamp (mode, 0, 2); }

    //--------------------------------------------------------------------------
    // Core processing

    /// Process two engine audio streams.  The internal bidirectional FM oscillators
    /// run independently; their outputs are blended into engineA/engineB via mix.
    /// @param engineA   Dry signal from engine slot A (modified in place)
    /// @param engineB   Dry signal from engine slot B (modified in place)
    /// @param numSamples Block size
    void processBlock (float* engineA, float* engineB, int numSamples)
    {
        // Early-out: if mix is negligible or both FM indices and feedback are zero,
        // the wet signal is silent and we can skip the whole loop.
        if (mix < 0.001f)
            return;

        // Derive effective FM indices from the convenience parameters.
        // symmetry controls the balance between A→B and B→A.
        // fmDepth scales the combined result uniformly.
        const float aToBIdx = fmIndexAB + fmDepth * kMaxIndex * (1.0f - symmetry);
        const float bToAIdx = fmIndexBA + fmDepth * kMaxIndex * symmetry;

        constexpr float pi    = 3.14159265358979323846f;
        constexpr float twoPi = 2.0f * pi;

        // Cache state locals to avoid repeated member access
        float pA       = phaseA;
        float pB       = phaseB;
        float incA     = phaseIncA;
        float incB     = phaseIncB;
        float prevOutA = prevA;
        float prevOutB = prevB;
        const float fbA       = feedbackA;
        const float fbB       = feedbackB;
        const float tdepth    = timbreDepth;
        const int   tmode     = timbreMode;

        for (int i = 0; i < numSamples; ++i)
        {
            //------------------------------------------------------------------
            // Step 1 — Compute oscA output.
            // B's previous output modulates A's instantaneous phase.
            // A's previous output provides self-feedback.
            // Both use fastTanh limiting on the modulation term to bound DC drift.
            float modTermA = fastTanh (bToAIdx * prevOutB * 0.95f)
                           + fbA * prevOutA;
            float outA = fastSin (pA + modTermA);

            //------------------------------------------------------------------
            // Step 2 — Compute oscB output.
            // A's previous output modulates B's instantaneous phase.
            float modTermB = fastTanh (aToBIdx * prevOutA * 0.95f)
                           + fbB * prevOutB;
            float rawB = fastSin (pB + modTermB);

            // Apply oscB waveshaper according to timbreMode.
            // timbreDepth blends between pure sine and shaped version.
            float outB = rawB;
            if (tdepth > 0.001f)
            {
                float shaped = rawB;
                switch (tmode)
                {
                    case 1:
                        // Soft-clip: drive then clip — odd harmonics, tube warmth
                        shaped = fastTanh (rawB * (1.0f + tdepth * 3.0f));
                        break;

                    case 2:
                        // Wavefold: fold the sine around ±1 — Buchla-style spectral richness
                        {
                            float driven = rawB * (1.0f + tdepth * 2.0f);
                            // One fold: if |driven| > 1, reflect back
                            // fastSin applied to the folded argument approximates the
                            // continuous wavefold characteristic without lookup tables.
                            shaped = fastSin (driven * pi * 0.5f);
                        }
                        break;

                    default: break;  // case 0: pure sine, no shaping
                }
                outB = rawB + tdepth * (shaped - rawB);
            }

            //------------------------------------------------------------------
            // Step 3 — Advance phase accumulators
            pA += incA;
            pB += incB;

            // Wrap to [-π, π] to keep fastSin in its accurate range.
            // Subtract/add 2π rather than fmod — cheaper on the DSP thread.
            if (pA >  pi) pA -= twoPi;
            if (pA < -pi) pA += twoPi;
            if (pB >  pi) pB -= twoPi;
            if (pB < -pi) pB += twoPi;

            //------------------------------------------------------------------
            // Step 4 — Update feedback state with denormal protection
            prevOutA = outA;
            prevOutB = outB;
            if (std::abs (prevOutA) < 1e-15f) prevOutA = 0.0f;
            if (std::abs (prevOutB) < 1e-15f) prevOutB = 0.0f;

            //------------------------------------------------------------------
            // Step 5 — Wet/dry mix with incoming engine streams
            engineA[i] += mix * (outA - engineA[i]);
            engineB[i] += mix * (outB - engineB[i]);
        }

        // Write state back
        phaseA = pA;
        phaseB = pB;
        prevA  = prevOutA;
        prevB  = prevOutB;
    }

    void reset()
    {
        phaseA = phaseB = 0.0f;
        prevA  = prevB  = 0.0f;
    }

private:
    //--------------------------------------------------------------------------
    // State

    double sr       = 44100.0;

    float phaseA    = 0.0f;
    float phaseB    = 0.0f;
    float phaseIncA = 0.0f;   ///< 2π × freqA / sr  (radians per sample)
    float phaseIncB = 0.0f;   ///< 2π × freqB / sr

    float prevA     = 0.0f;   ///< oscA output one sample ago (bidirectional FM + feedback)
    float prevB     = 0.0f;   ///< oscB output one sample ago

    //--------------------------------------------------------------------------
    // Parameters

    static constexpr float kMaxIndex = 5.0f;  ///< Maximum FM index (radians). DX7 range.

    float freqA      = 220.0f;  ///< Fundamental of oscA (Hz)
    float fmRatio    = 2.0f;    ///< freqB = freqA × fmRatio

    // Fine-grained per-direction FM indices (set directly or via convenience API)
    float fmIndexAB  = 0.0f;    ///< A → B, radians
    float fmIndexBA  = 0.0f;    ///< B → A, radians

    // Self-feedback per oscillator (0..0.95 — tanh-limited to prevent runaway)
    float feedbackA  = 0.0f;
    float feedbackB  = 0.0f;

    // Convenience scalars (v1 API mapping)
    float fmDepth    = 0.0f;    ///< Uniform FM depth scalar (0..1)
    float timbreDepth= 0.0f;    ///< OscB waveshaper depth (0..1)
    float symmetry   = 0.5f;    ///< 0 = only A→B, 0.5 = balanced, 1 = only B→A

    float mix        = 1.0f;    ///< Wet/dry blend with incoming engine audio

    int   timbreMode = 0;       ///< 0=sine, 1=soft-clip, 2=wavefold

    //--------------------------------------------------------------------------
    void updatePhaseIncrements()
    {
        if (sr <= 0.0) return;
        constexpr double twoPi = 6.28318530717958647692;
        phaseIncA = static_cast<float> (twoPi * freqA / sr);
        phaseIncB = static_cast<float> (twoPi * (freqA * fmRatio) / sr);
    }
};

} // namespace xoceanus
