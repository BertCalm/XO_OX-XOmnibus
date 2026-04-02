// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include "../FastMath.h"
#include "../CytomicSVF.h"
#include "Saturator.h"

namespace xoceanus {

//==============================================================================
// fXfXOsmosis — "Membrane Transfer" boutique effect.
//
// Audio passes through a resonant 3-band filter membrane that breathes with
// input dynamics. Quiet signals pass muted; loud signals bloom through the
// membrane pores. The envelope follower opens the filter resonance and
// brightness proportional to input energy. Tape saturation adds membrane
// nonlinearity — the material isn't perfectly transparent.
//
// Signal flow:
//   input → envelope follower → modulates 3 parallel bandpass filters
//         → each band through tape saturator (light)
//         → sum bands → mix with dry
//
// Character: Organic, alive, responsive. Pads breathe, leads bloom.
//            Think: sound seeping through kelp, coral, living tissue.
//
// CPU budget: ~0.5% @ 44.1kHz (3 SVFs + 1 envelope + 1 saturator)
// Bricks: CytomicSVF ×3, Saturator ×1, FastMath (envelope coefficients)
//
// Usage:
//   fXOsmosis fx;
//   fx.prepare(44100.0);
//   fx.setMembraneTone(0.5f);   // 0 = dark/submerged, 1 = bright/open
//   fx.setReactivity(0.6f);     // how much input dynamics open the pores
//   fx.setSaturation(0.3f);     // membrane nonlinearity
//   fx.setResonance(0.4f);      // base resonance of the membrane
//   fx.setMix(0.5f);
//   fx.processBlock(L, R, numSamples);
//==============================================================================
class fXOsmosis
{
public:
    fXOsmosis() = default;

    //--------------------------------------------------------------------------
    void prepare (double sampleRate)
    {
        sr = sampleRate;

        // Three membrane bands: low, mid, high
        // Base frequencies — shifted by membraneTone
        updateBandFrequencies();

        for (int b = 0; b < kNumBands; ++b)
        {
            bandsL[b].setMode (CytomicSVF::Mode::BandPass);
            bandsR[b].setMode (CytomicSVF::Mode::BandPass);
            bandsL[b].reset();
            bandsR[b].reset();
        }

        // Tape saturation: light drive for membrane nonlinearity
        saturator.setMode (Saturator::SaturationMode::Tape);
        saturator.setDrive (saturation);
        saturator.setMix (1.0f);
        saturator.setOutputGain (1.0f);
        saturator.reset();

        // Envelope follower state
        envState = 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Membrane tone: 0 = dark/submerged, 1 = bright/open.
    /// Shifts the center frequencies of the 3 filter bands upward.
    void setMembraneTone (float tone)
    {
        membraneTone = clamp (tone, 0.0f, 1.0f);
        updateBandFrequencies();
    }

    /// Reactivity: how much input dynamics open the pores (0-1).
    /// At 0, the membrane is static. At 1, loud signals fully open the filters.
    void setReactivity (float r) { reactivity = clamp (r, 0.0f, 1.0f); }

    /// Base resonance of the membrane filters (0-1).
    void setResonance (float r) { baseResonance = clamp (r, 0.0f, 0.85f); }

    /// Membrane nonlinearity / tape saturation amount (0-1).
    void setSaturation (float s)
    {
        saturation = clamp (s, 0.0f, 1.0f);
        saturator.setDrive (saturation);
    }

    /// Dry/wet mix (0-1).
    void setMix (float m) { mix = clamp (m, 0.0f, 1.0f); }

    //--------------------------------------------------------------------------
    /// Process stereo audio in-place.
    void processBlock (float* L, float* R, int numSamples)
    {
        if (mix < 0.001f) return;

        // Envelope follower coefficients (per-block, not per-sample)
        // Attack ~5ms, release ~80ms — fast enough to track transients
        float attackCoeff  = smoothCoeffFromTime (0.005f, static_cast<float> (sr));
        float releaseCoeff = smoothCoeffFromTime (0.08f, static_cast<float> (sr));

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = L[i];
            float inR = R[i];

            // Envelope follower: peak detection on mono sum
            float inputLevel = (std::fabs (inL) + std::fabs (inR)) * 0.5f;
            float envCoeff = (inputLevel > envState) ? attackCoeff : releaseCoeff;
            envState = flushDenormal (envState + envCoeff * (inputLevel - envState));

            // Modulate resonance and brightness based on envelope
            float envScaled = clamp (envState * 8.0f, 0.0f, 1.0f);  // normalize to usable range
            float dynResonance = baseResonance + reactivity * envScaled * (0.92f - baseResonance);
            float dynBrightness = envScaled * reactivity;

            // Update filter coefficients with dynamic resonance
            // Band frequencies shift up slightly with dynamics (membrane opens)
            for (int b = 0; b < kNumBands; ++b)
            {
                float freq = bandFreqs[b] * (1.0f + dynBrightness * 0.5f);
                freq = clamp (freq, 20.0f, static_cast<float> (sr) * 0.49f);
                bandsL[b].setCoefficients_fast (freq, dynResonance, static_cast<float> (sr));
                bandsR[b].setCoefficients_fast (freq, dynResonance, static_cast<float> (sr));
            }

            // Process through 3 parallel bandpass filters
            float wetL = 0.0f;
            float wetR = 0.0f;

            for (int b = 0; b < kNumBands; ++b)
            {
                wetL += bandsL[b].processSample (inL) * bandGains[b];
                wetR += bandsR[b].processSample (inR) * bandGains[b];
            }

            // Tape saturation on the wet signal (membrane nonlinearity)
            if (saturation > 0.01f)
            {
                wetL = saturator.processSample (wetL);
                wetR = saturator.processSample (wetR);
            }

            // Compensate level — bandpass sum is naturally quieter
            wetL *= 2.2f;
            wetR *= 2.2f;

            // Mix
            L[i] = inL * (1.0f - mix) + wetL * mix;
            R[i] = inR * (1.0f - mix) + wetR * mix;
        }
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        for (int b = 0; b < kNumBands; ++b)
        {
            bandsL[b].reset();
            bandsR[b].reset();
        }
        saturator.reset();
        envState = 0.0f;
    }

private:
    void updateBandFrequencies()
    {
        // Three pores: low (200-600), mid (800-2400), high (3000-8000)
        // membraneTone shifts the whole range upward
        float toneShift = 1.0f + membraneTone * 1.5f;  // 1.0× to 2.5×
        bandFreqs[0] = 300.0f * toneShift;
        bandFreqs[1] = 1200.0f * toneShift;
        bandFreqs[2] = 4500.0f * toneShift;

        // Gains: mid-focused with brighter top at higher tone
        bandGains[0] = 0.8f;
        bandGains[1] = 1.0f;
        bandGains[2] = 0.5f + membraneTone * 0.4f;
    }

    //--------------------------------------------------------------------------
    static constexpr int kNumBands = 3;

    double sr = 44100.0;

    // 3 stereo bandpass filter bands (the "membrane pores")
    CytomicSVF bandsL[kNumBands];
    CytomicSVF bandsR[kNumBands];
    float bandFreqs[kNumBands] = { 300.0f, 1200.0f, 4500.0f };
    float bandGains[kNumBands] = { 0.8f, 1.0f, 0.7f };

    // Tape saturator (membrane nonlinearity)
    Saturator saturator;

    // Envelope follower
    float envState = 0.0f;

    // Parameters
    float membraneTone  = 0.5f;
    float reactivity    = 0.5f;
    float baseResonance = 0.4f;
    float saturation    = 0.3f;
    float mix           = 0.0f;
};

} // namespace xoceanus
