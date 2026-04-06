// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <vector>
#include "../FastMath.h"

namespace xoceanus
{

//==============================================================================
// fXfXOneiric — "Dream State" boutique effect.
//
// A pitch-shifted feedback delay that creates infinitely ascending or
// descending spectral spirals. The frequency shifter lives inside the
// feedback loop — each echo shifts by a fixed Hz amount, so repeat 1
// is shifted +N Hz, repeat 2 is shifted +2N Hz, repeat 3 is shifted +3N Hz...
// creating a Shepard-tone-like staircase of ghostly echoes.
//
// Signal flow:
//   input ─┬─────────────────────────── dry ──────┐
//          │                                       │
//          └─→ delay line ←──── feedback path ─────┤
//                  │        ↑                      │
//                  └─→ HP ─→ freq shift ─→ LP ─┘   │
//                       (DC)   (Hilbert)  (damp)   │
//                                                  ↓
//                                            mix dry/wet
//
// Character: Hypnotic, otherworldly. At low shift (2-5 Hz): dreamy, subtle
//   spectral drift. At medium shift (10-30 Hz): metallic, alien. At high
//   shift (50-200 Hz): extreme spectral disintegration, like sound dissolving.
//   Boards of Canada, Brian Eno, Hainbach territory.
//
// CPU budget: ~1.1% @ 44.1kHz (1 delay + 1 Hilbert transform + 2 one-poles)
// Bricks: FastMath (fastSin/fastCos), flushDenormal, lerp
//
// Usage:
//   fXOneiric fx;
//   fx.prepare(44100.0, 512);
//   fx.setDelayTime(350.0f);    // ms
//   fx.setShift(5.0f);          // Hz — positive = up, negative = down
//   fx.setFeedback(0.65f);
//   fx.setDamping(0.4f);        // HF rolloff per repeat
//   fx.setMix(0.35f);
//   fx.processBlock(L, R, numSamples);
//==============================================================================
class fXOneiric
{
public:
    fXOneiric() = default;

    //--------------------------------------------------------------------------
    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        // Delay buffer: max 1500ms for long dream echoes
        int maxSamples = static_cast<int>(sr * 1.5) + 1;
        delayL.assign(static_cast<size_t>(maxSamples), 0.0f);
        delayR.assign(static_cast<size_t>(maxSamples), 0.0f);
        bufferSize = maxSamples;
        writePos = 0;

        // Hilbert transform: 3-stage allpass network (I and Q channels)
        // Coefficients from Olli Niemitalo's design (90° phase difference
        // over ~20 Hz to ~20 kHz)
        static constexpr float hilbertCoeffsI[kHilbertStages] = {0.6923878f, 0.9360654f, 0.9882295f};
        static constexpr float hilbertCoeffsQ[kHilbertStages] = {0.4021921f, 0.8561711f, 0.9722910f};
        for (int i = 0; i < kHilbertStages; ++i)
        {
            hCoeffI[i] = hilbertCoeffsI[i];
            hCoeffQ[i] = hilbertCoeffsQ[i];
        }

        // Quadrature oscillator: cos + sin pair for frequency shifting
        oscPhase = 0.0;

        // One-pole filter states
        dampStateL = dampStateR = 0.0f;
        dcStateL = dcStateR = 0.0f;
        dcPrevL = dcPrevR = 0.0f;

        resetHilbertState();
    }

    //--------------------------------------------------------------------------
    /// Delay time in ms (1-1500).
    void setDelayTime(float ms) { delayTimeMs = clamp(ms, 1.0f, 1500.0f); }

    /// Pitch shift per echo in Hz. Positive = ascending spiral, negative = descending.
    void setShift(float hz) { shiftHz = clamp(hz, -500.0f, 500.0f); }

    /// Feedback amount (0-0.92). Higher = more echoes = longer spiral.
    void setFeedback(float fb) { feedback = clamp(fb, 0.0f, 0.92f); }

    /// HF damping per repeat (0-1). Higher = darker echoes. Each repeat
    /// gets progressively softer in the highs, like sound fading into fog.
    void setDamping(float d) { damping = clamp(d, 0.0f, 1.0f); }

    /// Stereo spread: offsets the R channel delay slightly for width (0-1).
    void setSpread(float s) { spread = clamp(s, 0.0f, 1.0f); }

    /// Dry/wet mix (0-1).
    void setMix(float m) { mix = clamp(m, 0.0f, 1.0f); }

    //--------------------------------------------------------------------------
    /// Process stereo audio in-place.
    void processBlock(float* L, float* R, int numSamples)
    {
        if (bufferSize <= 0 || mix < 0.001f)
            return;

        float delaySamples = static_cast<float>(delayTimeMs * 0.001 * sr);
        delaySamples = clamp(delaySamples, 1.0f, static_cast<float>(bufferSize - 2));

        // R channel offset for stereo spread (up to 15ms)
        float spreadOffset = spread * 0.015f * static_cast<float>(sr);
        float delaySamplesR = clamp(delaySamples + spreadOffset, 1.0f, static_cast<float>(bufferSize - 2));

        // Damping coefficient: one-pole LP in [0.15, 1.0] range
        float dampCoeff = 1.0f - damping * 0.85f;

        // Quadrature oscillator increment
        double phaseInc = shiftHz / sr;

        // DC blocker coefficient (~30 Hz)
        float dcCoeff = 1.0f - (188.5f / static_cast<float>(sr)); // 2*pi*30/sr

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = L[i];
            float inR = R[i];

            // Read from delay with linear interpolation
            float delL = readDelay(delayL, delaySamples);
            float delR = readDelay(delayR, delaySamplesR);

            // === Feedback path: frequency shift the delayed signal ===

            // Hilbert transform: produce I (in-phase) and Q (quadrature) signals
            float hI_L = delL, hQ_L = delL;
            float hI_R = delR, hQ_R = delR;

            for (int s = 0; s < kHilbertStages; ++s)
            {
                // I chain (left)
                float tmp = hI_L - hCoeffI[s] * hStateI_L[s];
                hI_L = flushDenormal(hStateI_L[s] + hCoeffI[s] * tmp);
                hStateI_L[s] = tmp;

                // Q chain (left)
                tmp = hQ_L - hCoeffQ[s] * hStateQ_L[s];
                hQ_L = flushDenormal(hStateQ_L[s] + hCoeffQ[s] * tmp);
                hStateQ_L[s] = tmp;

                // I chain (right)
                tmp = hI_R - hCoeffI[s] * hStateI_R[s];
                hI_R = flushDenormal(hStateI_R[s] + hCoeffI[s] * tmp);
                hStateI_R[s] = tmp;

                // Q chain (right)
                tmp = hQ_R - hCoeffQ[s] * hStateQ_R[s];
                hQ_R = flushDenormal(hStateQ_R[s] + hCoeffQ[s] * tmp);
                hStateQ_R[s] = tmp;
            }

            // Quadrature oscillator: complex rotation for frequency shift
            float cosOsc = fastCos(static_cast<float>(oscPhase * 6.28318530718));
            float sinOsc = fastSin(static_cast<float>(oscPhase * 6.28318530718));

            // Single-sideband modulation: output = I*cos - Q*sin (upper sideband)
            // For negative shift: output = I*cos + Q*sin (lower sideband)
            float shiftedL, shiftedR;
            if (shiftHz >= 0.0f)
            {
                shiftedL = hI_L * cosOsc - hQ_L * sinOsc;
                shiftedR = hI_R * cosOsc - hQ_R * sinOsc;
            }
            else
            {
                shiftedL = hI_L * cosOsc + hQ_L * sinOsc;
                shiftedR = hI_R * cosOsc + hQ_R * sinOsc;
            }

            // Advance oscillator phase
            oscPhase += std::fabs(phaseInc);
            if (oscPhase >= 1.0)
                oscPhase -= 1.0;

            // DC blocker (frequency shifting generates DC offset)
            float dcOutL = shiftedL - dcPrevL + dcCoeff * dcStateL;
            float dcOutR = shiftedR - dcPrevR + dcCoeff * dcStateR;
            dcPrevL = shiftedL;
            dcPrevR = shiftedR;
            dcStateL = flushDenormal(dcOutL);
            dcStateR = flushDenormal(dcOutR);

            // Damping LP in feedback path (each repeat gets darker)
            dampStateL = flushDenormal(dampStateL + dampCoeff * (dcOutL - dampStateL));
            dampStateR = flushDenormal(dampStateR + dampCoeff * (dcOutR - dampStateR));

            // Write to delay: input + shifted feedback
            float fbL = dampStateL * feedback;
            float fbR = dampStateR * feedback;

            delayL[static_cast<size_t>(writePos)] = flushDenormal(inL + fbL);
            delayR[static_cast<size_t>(writePos)] = flushDenormal(inR + fbR);
            writePos = (writePos + 1) % bufferSize;

            // Mix dry/wet
            L[i] = inL * (1.0f - mix) + delL * mix;
            R[i] = inR * (1.0f - mix) + delR * mix;
        }
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        std::fill(delayL.begin(), delayL.end(), 0.0f);
        std::fill(delayR.begin(), delayR.end(), 0.0f);
        writePos = 0;
        oscPhase = 0.0;
        dampStateL = dampStateR = 0.0f;
        dcStateL = dcStateR = 0.0f;
        dcPrevL = dcPrevR = 0.0f;
        resetHilbertState();
    }

private:
    float readDelay(const std::vector<float>& buf, float delaySamples) const
    {
        int d = static_cast<int>(delaySamples);
        float frac = delaySamples - static_cast<float>(d);
        if (d < 1)
            d = 1;
        if (d >= bufferSize - 1)
            d = bufferSize - 2;

        int r0 = (writePos - d + bufferSize) % bufferSize;
        int r1 = (r0 - 1 + bufferSize) % bufferSize;

        return flushDenormal(lerp(buf[static_cast<size_t>(r0)], buf[static_cast<size_t>(r1)], frac));
    }

    void resetHilbertState()
    {
        for (int s = 0; s < kHilbertStages; ++s)
        {
            hStateI_L[s] = hStateQ_L[s] = 0.0f;
            hStateI_R[s] = hStateQ_R[s] = 0.0f;
        }
    }

    //--------------------------------------------------------------------------
    static constexpr int kHilbertStages = 3;

    double sr = 0.0;  // Sentinel: must be set by prepare() before use

    // Delay buffers
    std::vector<float> delayL;
    std::vector<float> delayR;
    int bufferSize = 0;
    int writePos = 0;

    // Hilbert transform allpass coefficients and state
    float hCoeffI[kHilbertStages]{};
    float hCoeffQ[kHilbertStages]{};
    float hStateI_L[kHilbertStages]{};
    float hStateQ_L[kHilbertStages]{};
    float hStateI_R[kHilbertStages]{};
    float hStateQ_R[kHilbertStages]{};

    // Quadrature oscillator
    double oscPhase = 0.0;

    // Damping LP state
    float dampStateL = 0.0f;
    float dampStateR = 0.0f;

    // DC blocker state
    float dcStateL = 0.0f, dcStateR = 0.0f;
    float dcPrevL = 0.0f, dcPrevR = 0.0f;

    // Parameters
    float delayTimeMs = 350.0f;
    float shiftHz = 5.0f;
    float feedback = 0.6f;
    float damping = 0.3f;
    float spread = 0.3f;
    float mix = 0.0f;
};

} // namespace xoceanus
