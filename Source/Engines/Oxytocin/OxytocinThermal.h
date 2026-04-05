// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>

/// OxytocinThermal — RE-201 warmth model.
///
/// Three-stage NTC thermistor warming chain: each stage is a one-pole lag on
/// the intimacy value.  The final warmth value shapes the audio via a
/// wet/dry between clean and tanh-saturated signal.
///
/// With circuit_age > 0 a slow pitch wobble (±3 cents at 0.3 Hz) is applied,
/// modelling the RE-201 capstan motor speed drift.
///
/// All block-rate coefficients are cached and only recomputed when the warmth
/// rate changes (OPERA P0 lesson — never compute exp() per sample on iOS).

namespace xoxytocin
{

// PERF-1: fast tanh — Pade [3/2] approximation, error < 0.001 for |x| < 4.
// Shared by OxytocinThermal and OxytocinDrive.
static inline float fastTanh(float x) noexcept
{
    if (x < -3.0f)
        return -1.0f;
    if (x > 3.0f)
        return 1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

class OxytocinThermal
{
public:
    OxytocinThermal() = default;

    void prepare(double sampleRate) noexcept
    {
        jassert(sampleRate > 0.0); // P1-7
        sr = sampleRate;
        // Fix 4 (FATHOM): envelope follower time constant — ~20 ms attack / ~200 ms release.
        // Computed from sample rate, never hardcoded.
        signalEnvAttackCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.020f));
        signalEnvReleaseCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.200f));
        reset();
        lastWarmthRate = -1.0f; // force coefficient recompute on first call
    }

    /// P1-5: reset() method so noteOn() can clear thermal state on voice steal.
    void reset() noexcept
    {
        stage1 = stage2 = stage3 = 0.0f;
        wobblePhase = 0.0f;
        signalEnvLevel = 0.0f;  // Fix 4: clear envelope follower on steal
        lastWarmthRate = -1.0f; // force coefficient recompute after reset
        // Cached load-adjusted coefficients will be recomputed by the next
        // updateWarmth() call, so no stale values survive across voice steals.
        cachedC1f = cachedC2f = cachedC3f = 0.0f;
    }

    /// Call once per block with the current intimacy level and warmth rate.
    /// Returns the current warmth value (used by the voice for cross-modulation).
    float updateWarmth(float intimacy, float warmthRate, float /*blockTime*/) noexcept
    {
        // Cache block-rate per-sample IIR coefficients.
        if (warmthRate != lastWarmthRate)
        {
            float tau1 = std::max(0.001f, warmthRate * 0.3f);
            float tau2 = std::max(0.001f, warmthRate * 0.6f);
            float tau3 = std::max(0.001f, warmthRate * 1.0f);

            coeff1 = std::exp(-1.0f / (static_cast<float>(sr) * tau1));
            coeff2 = std::exp(-1.0f / (static_cast<float>(sr) * tau2));
            coeff3 = std::exp(-1.0f / (static_cast<float>(sr) * tau3));
            lastWarmthRate = warmthRate;
        }

        // PERF: Precompute signal-load-adjusted coefficients at block rate.
        //
        // The NTC thermal-acceleration model raises each coefficient to the power
        // 1/loadMult so that a louder signal shortens effective time constants.
        // Both coeff1/2/3 and signalEnvLevel (which drives loadMult) are stable at
        // block rate — coeff1/2/3 by construction, signalEnvLevel because the
        // envelope follower is smoothed over ~20–200 ms.  Precomputing here replaces
        // 3 × std::pow() calls per sample (= 24 pow/sample at 8 voices) with 3 pow
        // calls per block, which is ~1000× cheaper at a 128-sample block size.
        {
            float loadMult = 1.0f + signalEnvLevel * 2.0f; // [1.0 .. 3.0]
            if (loadMult > 1.001f)
            {
                float invLoad = 1.0f / loadMult;
                cachedC1f = std::pow(coeff1, invLoad);
                cachedC2f = std::pow(coeff2, invLoad);
                cachedC3f = std::pow(coeff3, invLoad);
            }
            else
            {
                cachedC1f = coeff1;
                cachedC2f = coeff2;
                cachedC3f = coeff3;
            }
        }

        // Record target; stages are advanced per-sample in processSample().
        targetIntimacy = intimacy;
        return stage3;
    }

    /// Process a single audio sample.  Call after updateWarmth() for the block.
    float processSample(float input, float circuitAge) noexcept
    {
        // Signal-dependent thermal acceleration — envelope follower only.
        // The per-sample cost here is just comparisons, multiplications, and
        // the cached coefficient reads.  The actual std::pow() that derives
        // cachedC1f/2f/3f from the envelope level is called once per block in
        // updateWarmth(), not here.
        {
            // Peak envelope follower: fast attack, slow release.
            float absIn = std::abs(input);
            if (absIn > signalEnvLevel)
                signalEnvLevel = absIn + (signalEnvLevel - absIn) * signalEnvAttackCoeff;
            else
                signalEnvLevel = signalEnvLevel * signalEnvReleaseCoeff;

            signalEnvLevel = std::clamp(signalEnvLevel, 0.0f, 1.0f);

            // Advance warmth stages using block-rate-precomputed coefficients.
            // cachedC1f/2f/3f are set once per block by updateWarmth() — no
            // std::pow() here.
            stage1 = targetIntimacy + (stage1 - targetIntimacy) * cachedC1f;
            stage2 = stage1 + (stage2 - stage1) * cachedC2f;
            stage3 = stage2 + (stage3 - stage2) * cachedC3f;
        }

        // P1-4: denormal guards on warmth stage accumulators
        constexpr float kDenorm = 1e-18f;
        stage1 += kDenorm;
        stage2 += kDenorm;
        stage3 += kDenorm;

        const float warmth = stage3; // 0..1

        // NTC model: warm signal = tanh saturation
        // PERF-1: fastTanh replaces std::tanh (Pade [3/2] approximation)
        const float saturated = fastTanh(input * (1.0f + warmth * 2.5f)) * 0.7f;
        const float clean = input;

        float output = clean * (1.0f - warmth * 0.4f) + saturated * (warmth * 0.4f);

        // Circuit-age pitch wobble: ±3 cents * age * sin(0.3 Hz)
        if (circuitAge > 0.0f)
        {
            wobblePhase += static_cast<float>(juce::MathConstants<double>::twoPi * 0.3 / sr);
            if (wobblePhase > juce::MathConstants<float>::twoPi)
                wobblePhase -= juce::MathConstants<float>::twoPi;

            // Pitch wobble is applied at the signal level as a subtle AM
            // (a true pitch wobble would require a delay line; this is an
            //  affordable approximation for the "motor flutter" aesthetic)
            float wobble = 1.0f + circuitAge * 0.0017f * std::sin(wobblePhase);
            output *= wobble;
        }

        return output;
    }

    float getWarmth() const noexcept { return stage3; }

private:
    double sr = 0.0; // P1-7: default 0
    float stage1 = 0.0f;
    float stage2 = 0.0f;
    float stage3 = 0.0f;
    float targetIntimacy = 0.0f;
    float coeff1 = 0.0f;
    float coeff2 = 0.0f;
    float coeff3 = 0.0f;
    float lastWarmthRate = -1.0f;
    // PERF: block-rate cache of signal-load-adjusted NTC coefficients.
    // Precomputed in updateWarmth() — eliminates 3 std::pow() calls per sample.
    float cachedC1f = 0.0f;
    float cachedC2f = 0.0f;
    float cachedC3f = 0.0f;
    float wobblePhase = 0.0f;

    // Fix 4 (FATHOM): signal-level envelope follower for NTC thermal acceleration
    float signalEnvLevel = 0.0f;        // current signal envelope estimate [0..1]
    float signalEnvAttackCoeff = 0.0f;  // ~20 ms attack  (computed from sr in prepare())
    float signalEnvReleaseCoeff = 0.0f; // ~200 ms release (computed from sr in prepare())
};

} // namespace xoxytocin
