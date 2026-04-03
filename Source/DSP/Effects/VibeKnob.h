// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>
#include "../FastMath.h"

namespace xoceanus
{

//==============================================================================
// VibeKnob — Bipolar character dial: GRIT ←→ SWEET.
//
// A single continuous control from -1.0 to +1.0:
//
//   SWEET side (negative):
//     - Gentle high-shelf rolloff (air absorption, "vinyl warmth")
//     - Subtle even-harmonic saturation (tube-like bloom)
//     - Transient softening (micro-attack rounding)
//     - Light stereo thickening via complementary phase offset
//
//   GRIT side (positive):
//     - Warm tape saturation (musical odd harmonics, not harsh)
//     - Gentle bus compression (2:1-4:1, smooth attack, program-dependent)
//     - High-frequency presence boost (exciter-like)
//     - Subtle stereo narrowing (mono-punchy focus)
//
// At center (0.0): complete bypass, zero CPU.
// The transitions are smooth — there's no discontinuity at the center.
//
// Design philosophy: This should sound like the difference between
// "raw demo" and "finished record" in a single gesture.
//
// CPU: ~3 biquads + 1 envelope follower + soft clip = trivial.
//==============================================================================
class VibeKnob
{
public:
    VibeKnob() = default;

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    /// vibe: -1.0 = full sweet, 0.0 = bypass, +1.0 = full grit
    void setVibe(float v) { vibe = std::clamp(v, -1.0f, 1.0f); }

    void processBlock(float* left, float* right, int numSamples)
    {
        if (std::abs(vibe) < 0.001f)
            return;

        const bool isGrit = vibe > 0.0f;
        const float amount = std::abs(vibe);

        // Precompute per-block coefficients
        if (isGrit)
            setupGrit(amount);
        else
            setupSweet(amount);

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = left[i];
            float inR = right[i];

            float outL, outR;

            if (isGrit)
                processGritSample(inL, inR, outL, outR, amount);
            else
                processSweetSample(inL, inR, outL, outR, amount);

            left[i] = outL;
            right[i] = outR;
        }

        flushDenormals();
    }

    void reset()
    {
        // Shelving filter states
        shelfL = {};
        shelfR = {};
        // Presence filter states
        presL = {};
        presR = {};
        // Compressor envelope
        compEnv = 0.0f;
        // Transient softening
        softL = softR = 0.0f;
        // Phase thickening
        phaseL = phaseR = 0.0f;
    }

private:
    double sr = 44100.0;
    float vibe = 0.0f;

    //--------------------------------------------------------------------------
    // Filter state (shared between grit/sweet paths)
    struct FilterState
    {
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    };
    struct FilterCoeffs
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    };

    FilterState shelfL, shelfR;
    FilterCoeffs shelfC;

    FilterState presL, presR;
    FilterCoeffs presC;

    // Compressor envelope
    float compEnv = 0.0f;
    float compAttackCoeff = 0.0f, compReleaseCoeff = 0.0f;
    float compThresh = 0.0f, compRatio = 1.0f;

    // Transient softener (one-pole LPF on signal)
    float softL = 0.0f, softR = 0.0f;
    float softCoeff = 0.0f;

    // Phase thickener
    float phaseL = 0.0f, phaseR = 0.0f;
    float phaseCoeff = 0.0f;

    //--------------------------------------------------------------------------
    // SWEET path setup
    void setupSweet(float amount)
    {
        // High shelf rolloff: cut HF proportional to amount
        // At full sweet: -6dB shelf at 4kHz
        float shelfDb = -6.0f * amount;
        calcHighShelf(shelfC, 4000.0f, shelfDb);

        // Transient softener: one-pole LPF with amount-dependent cutoff
        // Full sweet = ~4kHz smoothing (gentle), less sweet = more transparent
        float softFreq = 20000.0f - amount * 16000.0f; // 20k → 4k
        softCoeff = 1.0f - fastExp(-6.28318530718f * softFreq / static_cast<float>(sr));

        // Phase thickening: slight allpass-like smear for stereo bloom
        phaseCoeff = 0.1f * amount;
    }

    // SWEET per-sample
    void processSweetSample(float inL, float inR, float& outL, float& outR, float amount)
    {
        // 1. Gentle even-harmonic saturation (soft tube warmth)
        float satL = tubeWarmth(inL, amount * 0.4f);
        float satR = tubeWarmth(inR, amount * 0.4f);

        // 2. Transient softening
        softL += softCoeff * (satL - softL);
        softR += softCoeff * (satR - softR);
        float blendL = satL + amount * 0.3f * (softL - satL);
        float blendR = satR + amount * 0.3f * (softR - satR);

        // 3. High shelf rolloff (warmth)
        blendL = applyBiquad(blendL, shelfL, shelfC);
        blendR = applyBiquad(blendR, shelfR, shelfC);

        // 4. Subtle stereo thickening (complementary phase nudge)
        float prevPhaseL = phaseL;
        float prevPhaseR = phaseR;
        phaseL = blendL;
        phaseR = blendR;
        outL = blendL + phaseCoeff * prevPhaseR; // Cross-feed tiny amount
        outR = blendR + phaseCoeff * prevPhaseL;

        // Compensate for any gain increase
        float gain = 1.0f / (1.0f + phaseCoeff * 0.5f);
        outL *= gain;
        outR *= gain;
    }

    //--------------------------------------------------------------------------
    // GRIT path setup
    void setupGrit(float amount)
    {
        // Presence boost: +3-6dB high shelf at 5kHz
        float presDb = 3.0f + amount * 3.0f; // +3 to +6dB
        calcHighShelf(presC, 5000.0f, presDb);

        // Bus compressor coefficients
        // Attack: 15-30ms (program-dependent, smooth)
        // Release: 80-200ms
        float attackMs = 30.0f - amount * 15.0f;    // 30ms → 15ms
        float releaseMs = 200.0f - amount * 120.0f; // 200ms → 80ms
        // SRO: fastExp replaces std::exp (per-block coefficient computation)
        compAttackCoeff = 1.0f - fastExp(-1.0f / (attackMs * 0.001f * static_cast<float>(sr)));
        compReleaseCoeff = 1.0f - fastExp(-1.0f / (releaseMs * 0.001f * static_cast<float>(sr)));

        // Threshold and ratio scale with amount
        compThresh = 0.5f - amount * 0.25f; // -6dB → -12dB
        compRatio = 2.0f + amount * 2.0f;   // 2:1 → 4:1
    }

    // GRIT per-sample
    void processGritSample(float inL, float inR, float& outL, float& outR, float amount)
    {
        // 1. Warm tape saturation (musical, not extreme)
        float satL = tapeSaturate(inL, amount);
        float satR = tapeSaturate(inR, amount);

        // 2. Presence boost
        satL = applyBiquad(satL, presL, presC);
        satR = applyBiquad(satR, presR, presC);

        // 3. Gentle bus compression
        float peak = std::max(std::abs(satL), std::abs(satR));
        float coeff = (peak > compEnv) ? compAttackCoeff : compReleaseCoeff;
        compEnv += coeff * (peak - compEnv);

        float compGain = 1.0f;
        if (compEnv > compThresh && compThresh > 0.001f)
        {
            // SRO: gainToDb/dbToGain replace std::log10/std::pow (per-sample hot path)
            float overDb = gainToDb(compEnv / compThresh);
            float reducedDb = overDb * (1.0f - 1.0f / compRatio);
            compGain = dbToGain(-reducedDb);
        }

        // Makeup gain: gentle, proportional to compression amount
        float makeupGain = 1.0f + amount * 0.15f;

        outL = satL * compGain * makeupGain;
        outR = satR * compGain * makeupGain;

        // 4. Subtle stereo narrowing for punch focus
        float mid = (outL + outR) * 0.5f;
        float side = (outL - outR) * 0.5f;
        side *= (1.0f - amount * 0.2f); // Max 20% narrowing
        outL = mid + side;
        outR = mid - side;
    }

    //--------------------------------------------------------------------------
    // Saturation curves

    /// Even-harmonic tube warmth (sweet side) — subtle bloom
    static float tubeWarmth(float x, float amount)
    {
        if (amount < 0.001f)
            return x;
        // Asymmetric soft clip: positive half gets slightly more gain
        // This creates even harmonics (tube character)
        float drive = 1.0f + amount * 3.0f;
        float driven = x * drive;
        float pos = driven > 0.0f ? driven / (1.0f + driven) : driven / (1.0f - driven * 0.7f);
        // Blend with dry to control intensity
        return x + amount * (pos / drive - x);
    }

    /// Tape saturation (grit side) — warm, musical
    static float tapeSaturate(float x, float amount)
    {
        // Tape-style: smooth soft clip with hysteresis approximation
        float drive = 1.0f + amount * 4.0f;
        float driven = x * drive;

        // Tape formula: tanh-like but warmer
        float sat;
        if (std::abs(driven) < 1.5f)
            sat = driven - (driven * driven * driven) / 6.75f; // Taylor approx
        else
            sat = (driven > 0.0f) ? 0.889f : -0.889f; // Soft limit

        // Mix: even at full grit, keep some dry signal for musicality
        float wetBlend = 0.5f + amount * 0.35f; // 50-85% wet
        return x * (1.0f - wetBlend) + (sat / drive) * wetBlend;
    }

    //--------------------------------------------------------------------------
    // Biquad helpers

    float applyBiquad(float in, FilterState& s, const FilterCoeffs& c)
    {
        float out = c.b0 * in + c.b1 * s.x1 + c.b2 * s.x2 - c.a1 * s.y1 - c.a2 * s.y2;
        s.x2 = s.x1;
        s.x1 = in;
        s.y2 = s.y1;
        s.y1 = out;
        return out;
    }

    void calcHighShelf(FilterCoeffs& c, float freq, float gainDb)
    {
        // SRO: fastExp + fastSin/fastCos replace std:: trig (per-block coefficient calc)
        float A = dbToGain(gainDb * 0.5f); // 10^(dB/40) = dbToGain(dB/2)
        float w0 = 2.0f * kPi * freq / static_cast<float>(sr);
        float cosW0 = fastCos(w0);
        float sinW0 = fastSin(w0);
        float alpha = sinW0 / (2.0f * 0.707f);
        // SRO: fast sqrt via fastPow2/fastLog2 (per-setter shelf calc)
        float sqrtA = (A > 0.0f) ? fastPow2(0.5f * fastLog2(A)) : 0.0f;

        float a0 = (A + 1.0f) - (A - 1.0f) * cosW0 + 2.0f * sqrtA * alpha;
        c.b0 = (A * ((A + 1.0f) + (A - 1.0f) * cosW0 + 2.0f * sqrtA * alpha)) / a0;
        c.b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosW0)) / a0;
        c.b2 = (A * ((A + 1.0f) + (A - 1.0f) * cosW0 - 2.0f * sqrtA * alpha)) / a0;
        c.a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cosW0)) / a0;
        c.a2 = ((A + 1.0f) - (A - 1.0f) * cosW0 - 2.0f * sqrtA * alpha) / a0;
    }

    // SRO: Use shared flushDenormal from FastMath.h
    void flushDenormals()
    {
        auto fd = [](float& v) { v = flushDenormal(v); };
        fd(shelfL.x1);
        fd(shelfL.x2);
        fd(shelfL.y1);
        fd(shelfL.y2);
        fd(shelfR.x1);
        fd(shelfR.x2);
        fd(shelfR.y1);
        fd(shelfR.y2);
        fd(presL.x1);
        fd(presL.x2);
        fd(presL.y1);
        fd(presL.y2);
        fd(presR.x1);
        fd(presR.x2);
        fd(presR.y1);
        fd(presR.y2);
        fd(compEnv);
        fd(softL);
        fd(softR);
        fd(phaseL);
        fd(phaseR);
    }

    static constexpr float kPi = 3.14159265358979f;
};

} // namespace xoceanus
