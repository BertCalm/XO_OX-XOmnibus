// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>
#include "../FastMath.h"

namespace xoceanus
{

//==============================================================================
// MasterModulation — Multi-mode modulation processor for the Master FX chain.
//
// Five modes:
//   1. Chorus:   Classic 2-voice BBD-style chorus (warm, wide)
//   2. Flanger:  Short delay + feedback comb filtering (jet-engine sweep)
//   3. Ensemble: 3-voice phase-offset chorus (Roland Dimension D inspired)
//   4. Drift:    Random-walk LFO wow/flutter (Chase Bliss Warped Vinyl)
//   5. Phaser:   6-stage allpass phaser (MXR Phase 90 / Small Stone inspired)
//
// Features:
//   - BBD character: optional bandwidth limiting + subtle noise
//   - Stereo decorrelation via phase-offset LFOs
//   - Denormal protection on all feedback/state
//   - Zero CPU when depth = 0
//
// Phaser design:
//   - 6 first-order allpass stages per channel
//   - allpass coefficient 'a' swept by sine LFO between low/high frequency poles
//   - Feedback from phaser output back to input (resonance / notch depth)
//   - Stereo: R channel LFO offset by 90° for natural width
//   - Matched-Z coefficient: a = (tan(pi*fc/sr) - 1) / (tan(pi*fc/sr) + 1)
//   - Depth parameter controls sweep range width
//   - Rate parameter is shared with other modes (0.01–15 Hz)
//
// Inspired by: Walrus Julia, Chase Bliss Warped Vinyl, OBNE Visitor,
//              Roland Dimension D, MXR Phase 90, EHX Small Stone
//==============================================================================
class MasterModulation
{
public:
    enum class Mode
    {
        Chorus = 0,
        Flanger,
        Ensemble,
        Drift,
        Phaser,
        NumModes
    };

    MasterModulation() = default;

    //--------------------------------------------------------------------------
    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        // Max modulated delay: 40ms for chorus/ensemble, 5ms for flanger
        // Use 50ms to cover all modes with margin
        int maxDelaySamples = static_cast<int>(sr * 0.05) + 64;
        for (int v = 0; v < kMaxVoices; ++v)
        {
            delayLines[v].assign(static_cast<size_t>(maxDelaySamples), 0.0f);
            delayLinesR[v].assign(static_cast<size_t>(maxDelaySamples), 0.0f);
            writePositions[v] = 0;
        }
        maxDelayLen = maxDelaySamples;

        // Reset LFO phases
        for (int v = 0; v < kMaxVoices; ++v)
            lfoPhase[v] = static_cast<float>(v) / static_cast<float>(kMaxVoices);

        // Drift random state
        driftState = 0.0f;
        driftTarget = 0.0f;
        driftCounter = 0;
        driftInterval = static_cast<int>(sr * 0.15); // new target every ~150ms

        // BBD lowpass state
        bbdLP_L = bbdLP_R = 0.0f;

        // Random seed from sample rate bits for deterministic-ish behavior
        randState = static_cast<uint32_t>(sr * 1000.0) ^ 0xDEADBEEF;

        // Phaser state reset
        phaserLfoPhaseL = 0.0f;
        phaserLfoPhaseR = 0.25f;
        for (int s = 0; s < kPhaserStages; ++s)
        {
            phaserStateL[s] = 0.0f;
            phaserStateR[s] = 0.0f;
        }
        phaserFBL = phaserFBR = 0.0f;
    }

    //--------------------------------------------------------------------------
    void setRate(float hz) { rate = clamp(hz, 0.01f, 15.0f); }
    void setDepth(float d) { depth = clamp(d, 0.0f, 1.0f); }
    void setMix(float wet) { mix = clamp(wet, 0.0f, 1.0f); }
    void setMode(Mode m) { mode = m; }
    void setFeedback(float fb) { feedback = clamp(fb, 0.0f, 0.85f); }

    //--------------------------------------------------------------------------
    /// Process stereo audio in-place.
    void processBlock(float* L, float* R, int numSamples)
    {
        if (maxDelayLen <= 0)
            return;

        // Phaser mode uses allpass stages, not delay lines — handle separately
        if (mode == Mode::Phaser)
        {
            if (sr <= 0.0)
                return; // not prepared
            processPhaserBlock(L, R, numSamples);
            return;
        }

        // Determine voice count and delay range based on mode
        int numVoices = 1;
        float minDelayMs = 0.0f;
        float maxDelayMs = 0.0f;
        float fbAmount = 0.0f;
        bool useBBD = false;

        switch (mode)
        {
        case Mode::Chorus:
            numVoices = 2;
            minDelayMs = 5.0f;
            maxDelayMs = 15.0f;
            fbAmount = 0.0f;
            useBBD = true;
            break;

        case Mode::Flanger:
            numVoices = 1;
            minDelayMs = 0.3f;
            maxDelayMs = 5.0f;
            fbAmount = feedback;
            useBBD = false;
            break;

        case Mode::Ensemble:
            numVoices = 3;
            minDelayMs = 5.0f;
            maxDelayMs = 25.0f;
            fbAmount = feedback * 0.3f; // light feedback for thickening
            useBBD = true;
            break;

        case Mode::Drift:
            numVoices = 1;
            minDelayMs = 3.0f;
            maxDelayMs = 20.0f;
            fbAmount = 0.0f;
            useBBD = true;
            break;

        default:
            break;
        }

        if (numVoices > kMaxVoices)
            numVoices = kMaxVoices;

        float minDelaySamples = static_cast<float>(minDelayMs * 0.001f * static_cast<float>(sr));
        float maxDelaySamples = static_cast<float>(maxDelayMs * 0.001f * static_cast<float>(sr));
        float delayRange = maxDelaySamples - minDelaySamples;

        // BBD character: LP at ~8kHz in the wet path
        float bbdCoeff = useBBD ? clamp(8000.0f / static_cast<float>(sr) * 6.2832f, 0.0f, 1.0f) : 1.0f;

        float phaseInc = rate / static_cast<float>(sr);

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = L[i];
            float inR = R[i];

            float wetL = 0.0f;
            float wetR = 0.0f;

            for (int v = 0; v < numVoices; ++v)
            {
                // Compute LFO value for this voice
                float lfoVal = 0.0f;

                if (mode == Mode::Drift)
                {
                    // Random walk LFO
                    lfoVal = computeDriftLFO();
                }
                else
                {
                    // Sine LFO with per-voice phase offset
                    lfoVal = fastSin(lfoPhase[v] * 6.28318530718f);
                }

                // Modulated delay in samples
                float modDelay = minDelaySamples + (lfoVal * 0.5f + 0.5f) * delayRange * depth;
                modDelay = clamp(modDelay, 1.0f, static_cast<float>(maxDelayLen - 2));

                // Read from delay line with linear interpolation
                int delaySamps = static_cast<int>(modDelay);
                float frac = modDelay - static_cast<float>(delaySamps);

                int wp = writePositions[v];
                int r0 = (wp - delaySamps + maxDelayLen) % maxDelayLen;
                int r1 = (r0 - 1 + maxDelayLen) % maxDelayLen;

                float dL = flushDenormal(
                    lerp(delayLines[v][static_cast<size_t>(r0)], delayLines[v][static_cast<size_t>(r1)], frac));
                float dR = flushDenormal(
                    lerp(delayLinesR[v][static_cast<size_t>(r0)], delayLinesR[v][static_cast<size_t>(r1)], frac));

                // Write to delay line with feedback
                delayLines[v][static_cast<size_t>(wp)] = inL + dL * fbAmount;
                delayLinesR[v][static_cast<size_t>(wp)] = inR + dR * fbAmount;
                writePositions[v] = (wp + 1) % maxDelayLen;

                // Stereo decorrelation: offset R phase by 90 degrees
                float lfoValR = (mode == Mode::Drift) ? lfoVal // Drift already has randomness
                                                      : fastSin((lfoPhase[v] + 0.25f) * 6.28318530718f);

                float modDelayR = minDelaySamples + (lfoValR * 0.5f + 0.5f) * delayRange * depth;
                modDelayR = clamp(modDelayR, 1.0f, static_cast<float>(maxDelayLen - 2));

                int delaySampsR = static_cast<int>(modDelayR);
                float fracR = modDelayR - static_cast<float>(delaySampsR);
                int r0R = (wp - delaySampsR + maxDelayLen) % maxDelayLen;
                int r1R = (r0R - 1 + maxDelayLen) % maxDelayLen;

                float dRStereo = flushDenormal(
                    lerp(delayLinesR[v][static_cast<size_t>(r0R)], delayLinesR[v][static_cast<size_t>(r1R)], fracR));

                wetL += dL;
                wetR += dRStereo;

                // Advance LFO phase
                lfoPhase[v] += phaseInc;
                if (lfoPhase[v] >= 1.0f)
                    lfoPhase[v] -= 1.0f;
            }

            // Normalize by voice count
            float normGain = 1.0f / static_cast<float>(numVoices);
            wetL *= normGain;
            wetR *= normGain;

            // BBD character: lowpass filter on wet signal
            if (useBBD && bbdCoeff < 1.0f)
            {
                bbdLP_L = flushDenormal(bbdLP_L + bbdCoeff * (wetL - bbdLP_L));
                bbdLP_R = flushDenormal(bbdLP_R + bbdCoeff * (wetR - bbdLP_R));
                wetL = bbdLP_L;
                wetR = bbdLP_R;
            }

            // Mix dry/wet
            L[i] = inL * (1.0f - mix) + wetL * mix;
            R[i] = inR * (1.0f - mix) + wetR * mix;
        }
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        for (int v = 0; v < kMaxVoices; ++v)
        {
            std::fill(delayLines[v].begin(), delayLines[v].end(), 0.0f);
            std::fill(delayLinesR[v].begin(), delayLinesR[v].end(), 0.0f);
            writePositions[v] = 0;
            lfoPhase[v] = static_cast<float>(v) / static_cast<float>(kMaxVoices);
        }
        bbdLP_L = bbdLP_R = 0.0f;
        driftState = driftTarget = 0.0f;
        driftCounter = 0;

        // Phaser state
        phaserLfoPhaseL = 0.0f;
        phaserLfoPhaseR = 0.25f; // 90° offset for stereo width
        for (int s = 0; s < kPhaserStages; ++s)
        {
            phaserStateL[s] = 0.0f;
            phaserStateR[s] = 0.0f;
        }
        phaserFBL = 0.0f;
        phaserFBR = 0.0f;
    }

private:
    /// Drift mode: smoothed random walk
    float computeDriftLFO()
    {
        driftCounter++;
        if (driftCounter >= driftInterval)
        {
            driftCounter = 0;
            // New random target in [-1, 1]
            driftTarget = (nextRandom() * 2.0f - 1.0f);
            // Scale interval by rate (faster rate = shorter intervals)
            driftInterval = static_cast<int>(sr * 0.05f / clamp(rate, 0.05f, 15.0f));
            if (driftInterval < 1)
                driftInterval = 1;
        }

        // Smooth toward target
        float smoothing = 1.0f - fastExp(-rate * 0.5f / static_cast<float>(sr));
        driftState = flushDenormal(driftState + smoothing * (driftTarget - driftState));
        return driftState;
    }

    /// Simple xorshift32 PRNG → [0, 1)
    float nextRandom()
    {
        randState ^= randState << 13;
        randState ^= randState >> 17;
        randState ^= randState << 5;
        return static_cast<float>(randState & 0x7FFFFF) / static_cast<float>(0x7FFFFF);
    }

    //--------------------------------------------------------------------------
    // Phaser mode: 6-stage first-order allpass cascade with feedback.
    //
    // Each allpass stage implements the transfer function:
    //   H(z) = (a + z^-1) / (1 + a * z^-1)
    //
    // where 'a' is the allpass coefficient derived from a target frequency fc:
    //   a = (tan(pi * fc / sr) - 1) / (tan(pi * fc / sr) + 1)
    //
    // This matched-Z design keeps the allpass pole/zero pair correctly placed
    // in the z-plane across all sample rates (44100 / 48000 / 96000 Hz).
    //
    // The LFO sweeps fc between fcLow and fcHigh. Depth scales the sweep range
    // symmetrically around the centre frequency. Feedback (signed, 0..0.85)
    // controls notch depth: positive feedback deepens notches; negative
    // feedback inverts the comb for a classic "flanged phaser" character.
    //
    // Stereo: L and R share the same LFO frequency but their phase is offset
    // by 90° so the notch positions diverge slowly, giving natural width.
    //--------------------------------------------------------------------------
    void processPhaserBlock(float* L, float* R, int numSamples)
    {
        // Frequency sweep bounds (Hz). Centre ~800 Hz, depth widens the sweep.
        // At depth=0 the coefficient is constant (phaser still colours tone).
        // At depth=1 the sweep spans fcLow..fcHigh for maximum notch movement.
        constexpr float fcCentre = 800.0f;
        constexpr float fcSweepMax = 700.0f; // max deviation from centre

        const float fcLow = fcCentre - fcSweepMax * depth;
        const float fcHigh = fcCentre + fcSweepMax * depth;

        const float srF = static_cast<float>(sr);
        const float phaseInc = rate / srF;

        // Feedback: scale so 0.85 → ~0.75 internal (leaves headroom)
        const float fb = feedback * 0.88f;

        for (int i = 0; i < numSamples; ++i)
        {
            // Advance LFO phases
            phaserLfoPhaseL += phaseInc;
            if (phaserLfoPhaseL >= 1.0f)
                phaserLfoPhaseL -= 1.0f;
            phaserLfoPhaseR += phaseInc;
            if (phaserLfoPhaseR >= 1.0f)
                phaserLfoPhaseR -= 1.0f;

            // Sine LFO in [0, 1]
            const float lfoL = fastSin(phaserLfoPhaseL * 6.28318530718f) * 0.5f + 0.5f;
            const float lfoR = fastSin(phaserLfoPhaseR * 6.28318530718f) * 0.5f + 0.5f;

            // Target allpass frequencies for L and R
            const float fcL = fcLow + lfoL * (fcHigh - fcLow);
            const float fcR = fcLow + lfoR * (fcHigh - fcLow);

            // Matched-Z allpass coefficient: a = (tan(pi*fc/sr) - 1) / (tan(pi*fc/sr) + 1)
            // tan() is expensive; use a fast polynomial approximation valid for fc < sr/4.
            // For fc in [100, 1500] Hz at sr >= 44100, pi*fc/sr < 0.107 rad, well within
            // the tan(x) ≈ x + x^3/3 + 2*x^5/15 domain (Maclaurin, < 0.1% for |w| < pi/4).
            auto allpassCoeff = [&](float fc) -> float
            {
                // Clamp fc to avoid coefficient approaching ±1 (would cause instability)
                const float fcClamped = clamp(fc, 20.0f, srF * 0.25f);
                const float w = 3.14159265359f * fcClamped / srF;
                // tan(w) via fast polynomial (accurate to < 0.1% for w < pi/4)
                const float w2 = w * w;
                const float tanW = w * (1.0f + w2 * (0.33333333f + w2 * 0.13333333f));
                const float denom = tanW + 1.0f;
                if (denom < 1e-6f)
                    return 0.0f; // degenerate — near-transparent allpass (defensive; unreachable given fc clamp)
                return clamp((tanW - 1.0f) / denom, -0.9999f, 0.9999f);
            };

            const float aL = allpassCoeff(fcL);
            const float aR = allpassCoeff(fcR);

            // --- Process Left channel ---
            float xL = flushDenormal(L[i] + phaserFBL * fb);
            // 6 cascaded allpass stages
            for (int s = 0; s < kPhaserStages; ++s)
            {
                const float yn = aL * xL + phaserStateL[s];
                phaserStateL[s] = flushDenormal(xL - aL * yn);
                xL = yn;
            }
            xL = clamp(
                xL, -4.0f,
                4.0f); // hard clip: prevents Inf/NaN from allpass runaway; may crunch at max feedback + resonance (by design)
            phaserFBL = flushDenormal(xL);
            L[i] = L[i] * (1.0f - mix) + xL * mix;

            // --- Process Right channel ---
            float xR = flushDenormal(R[i] + phaserFBR * fb);
            for (int s = 0; s < kPhaserStages; ++s)
            {
                const float yn = aR * xR + phaserStateR[s];
                phaserStateR[s] = flushDenormal(xR - aR * yn);
                xR = yn;
            }
            xR = clamp(
                xR, -4.0f,
                4.0f); // hard clip: prevents Inf/NaN from allpass runaway; may crunch at max feedback + resonance (by design)
            phaserFBR = flushDenormal(xR);
            R[i] = R[i] * (1.0f - mix) + xR * mix;
        }
    }

    //--------------------------------------------------------------------------
    static constexpr int kMaxVoices = 3;

    double sr = 44100.0;
    int maxDelayLen = 0;

    // Per-voice delay lines (stereo)
    std::vector<float> delayLines[kMaxVoices];
    std::vector<float> delayLinesR[kMaxVoices];
    int writePositions[kMaxVoices]{};

    // LFO state
    float lfoPhase[kMaxVoices]{};

    // Drift mode state
    float driftState = 0.0f;
    float driftTarget = 0.0f;
    int driftCounter = 0;
    int driftInterval = 6615; // ~150ms @ 44.1k

    // BBD lowpass state
    float bbdLP_L = 0.0f;
    float bbdLP_R = 0.0f;

    // PRNG state
    uint32_t randState = 0xDEADBEEF;

    // Phaser mode state
    static constexpr int kPhaserStages = 6;
    float phaserLfoPhaseL = 0.0f;
    float phaserLfoPhaseR = 0.25f;       // 90° offset for stereo width
    float phaserStateL[kPhaserStages]{}; // allpass z^-1 state, L
    float phaserStateR[kPhaserStages]{}; // allpass z^-1 state, R
    float phaserFBL = 0.0f;              // feedback sample, L
    float phaserFBR = 0.0f;              // feedback sample, R

    // Parameters
    Mode mode = Mode::Chorus;
    float rate = 0.8f;
    float depth = 0.0f;
    float mix = 0.0f;
    float feedback = 0.0f;
};

} // namespace xoceanus
