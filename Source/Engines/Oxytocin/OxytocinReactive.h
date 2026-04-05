// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>

namespace xoxytocin
{

/// OxytocinReactive — Huovilainen improved Moog 4-pole ladder filter.
///
/// Commitment is expressed through the filter's memory:
///   - high commitment → very long filter release (capacitive hold)
///   - obsession mode (C>0.85, I<0.2, P<0.2) → resonance pushed toward
///     self-oscillation with a slow random-ish pitch drift.
///
/// coefficients: matched-Z  g = tan(pi*fc/sr)  with thermal pre-warping.
/// All block-rate coefficients are cached (OPERA P0 lesson).
///
/// P0-2: Huovilainen normalisation factor 'a' is now applied to the
///       feedback path, preventing divergence at high commitment levels.
///
/// PERF-3: obsession drift sine is computed once per block (not per sample).

class OxytocinReactive
{
public:
    OxytocinReactive() = default;

    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        s1 = s2 = s3 = s4 = 0.0f;
        prevS1 = prevS2 = prevS3 = prevS4 = 0.0f;
        obsessionDrift = 0.0f;
        obsessionDriftPhase = 0.0f;
        cachedObsessionDrift = 0.0f;
        lastCutoff = -1.0f;
        lastResonance = -1.0f;
    }

    void reset() noexcept
    {
        s1 = s2 = s3 = s4 = 0.0f;
        prevS1 = prevS2 = prevS3 = prevS4 = 0.0f;
        // P1-8: also reset obsession drift phase so retriggered voices start clean
        obsessionDrift = 0.0f;
        obsessionDriftPhase = 0.0f;
        cachedObsessionDrift = 0.0f;
    }

    /// Update filter coefficients from the current commitment level.
    ///
    /// Fix 1 (FATHOM): now called once per SAMPLE inside OxytocinVoice's sample
    /// loop, passing boostedC (love envelope output) rather than snap.commitment
    /// (static knob).  This makes resonance breathe with the love envelope.
    ///
    /// The change-guard (lastResonance / lastCutoff) still avoids redundant
    /// tan() recomputes when boostedC is stable between consecutive samples.
    void updateCoefficients(float cutoffHz, float commitmentLevel) noexcept
    {
        float newRes = commitmentLevel * 3.8f;

        // FIX 5: quantize cutoff to 0.01 Hz grid before the change guard.
        // During commitment envelope sweeps, boostedC changes every sample, causing
        // ~44,100 tan() calls/sec/voice.  The guard below now only trips when the
        // cutoff moves by at least 0.01 Hz — limiting recomputes to ~100/sec/voice
        // during smooth envelope movement without any audible resolution loss.
        float quantizedCutoff = std::round(cutoffHz * 100.0f) / 100.0f;

        if (newRes != lastResonance || quantizedCutoff != lastCutoff)
        {
            float fc = std::max(20.0f, std::min(quantizedCutoff, static_cast<float>(sr) * 0.49f));

            // Matched-Z: g = tan(pi * fc / sr)
            g = std::tan(juce::MathConstants<float>::pi * fc / static_cast<float>(sr));

            // FIX 1: Correct TPT Moog normalisation factor.
            // The feedback path must use the normalised one-pole gain G = g/(1+g)
            // raised to the 4th power — NOT the binomial expansion (1+g)^4.
            // Old formula: a = 1/(1 + 4*res*g + 6*g^2 + 4*g^3 + g^4) == 1/(1+g)^4 at res=0
            //   → resonance ~10% of intended at typical operating points (e.g. 8kHz, res=3.8).
            // Correct TPT Moog: a = 1 / (1 + 4 * res * G^4)  where G = g/(1+g)
            float G = g / (1.0f + g); // normalised one-pole gain
            float G4 = G * G * G * G; // G^4
            a = 1.0f / (1.0f + 4.0f * newRes * G4);

            res = newRes;
            lastCutoff = quantizedCutoff;
            lastResonance = newRes;
        }
    }

    /// PERF-3: compute obsession drift once per block (called by OxytocinVoice
    /// before the sample loop).
    void updateObsession(float blockTimeSec) noexcept
    {
        // Advance phase by block time at ~0.286 Hz (1/3.5 Hz)
        obsessionDriftPhase += static_cast<float>(juce::MathConstants<double>::twoPi * (1.0 / 3.5) * blockTimeSec);
        if (obsessionDriftPhase > juce::MathConstants<float>::twoPi)
            obsessionDriftPhase -= juce::MathConstants<float>::twoPi;

        // Lissajous combination — irrational ratio prevents periodicity
        cachedObsessionDrift = std::sin(obsessionDriftPhase) * 0.35f + std::sin(obsessionDriftPhase * 1.618f) * 0.15f;
    }

    /// Process one sample with given effective commitment, intimacy, passion.
    /// commitRate and baseRelease are reserved for a planned future feature (per-sample
    /// release modulation driven by commitment level — V1.1 scope). They are passed
    /// now to keep the call-site API stable and avoid parameter-ID freeze issues later.
    float processSample(float input, float effectiveCommitment, float effectiveIntimacy, float effectivePassion,
                        [[maybe_unused]] float commitRate, [[maybe_unused]] float baseRelease) noexcept
    {
        // Obsession mode — smoothstep crossfade over the threshold region.
        // Replaces the hard binary switch: prevents a resonance step when
        // automating commitment slowly across the 0.85 boundary (P3 fix).
        //   commitment: onset at 0.70, full at 0.90  (0.2 ramp width)
        //   intimacy:   full below 0.25, zero at 0.10  (inverted)
        //   passion:    full below 0.25, zero at 0.10  (inverted)
        float obsessionAmount = std::clamp((effectiveCommitment - 0.7f) / 0.2f, 0.0f, 1.0f) *
                                std::clamp((0.25f - effectiveIntimacy) / 0.15f, 0.0f, 1.0f) *
                                std::clamp((0.25f - effectivePassion) / 0.15f, 0.0f, 1.0f);

        bool obsessing = (obsessionAmount > 0.0f);

        float effectiveRes = res;
        effectiveRes += 0.5f * obsessionAmount; // smoothly scaled boost

        // Commitment hold: filter state vectors resist change proportional to
        // commitment.  This is implemented by softly blending each stage toward
        // its previous value at a "hold" rate.
        float holdCoeff = effectiveCommitment * 0.35f; // 0 = free, ~0.33 = very sticky

        // 4-pole Moog ladder (Huovilainen model)
        // P0-2: apply normalisation factor 'a' to the feedback path.
        // Without 'a', fb = res * s4 is unnormalised and will diverge at
        // high commitment (res → 3.8) producing NaN/Inf.
        float fb = effectiveRes * s4 * a; // P0-2: 'a' normalises the feedback
        float inp = input - fb;

        // Each stage: y = (g * x + s) / (1 + g)  where s is the integrator state
        float y1 = (g * inp + s1) / (1.0f + g);
        s1 = 2.0f * y1 - s1;

        float y2 = (g * y1 + s2) / (1.0f + g);
        s2 = 2.0f * y2 - s2;

        float y3 = (g * y2 + s3) / (1.0f + g);
        s3 = 2.0f * y3 - s3;

        float y4 = (g * y3 + s4) / (1.0f + g);
        s4 = 2.0f * y4 - s4;

        // P1-3: denormal guards on ladder integrator states
        auto fd = [](float v) { return (std::abs(v) < 1e-18f) ? 0.0f : v; };
        s1 = fd(s1);
        s2 = fd(s2);
        s3 = fd(s3);
        s4 = fd(s4);

        // Commitment hold: resist state change
        if (holdCoeff > 0.0f)
        {
            s1 = s1 * (1.0f - holdCoeff) + prevS1 * holdCoeff;
            s2 = s2 * (1.0f - holdCoeff) + prevS2 * holdCoeff;
            s3 = s3 * (1.0f - holdCoeff) + prevS3 * holdCoeff;
            s4 = s4 * (1.0f - holdCoeff) + prevS4 * holdCoeff;
        }

        prevS1 = s1;
        prevS2 = s2;
        prevS3 = s3;
        prevS4 = s4;

        // PERF-3: use block-rate cached drift value (not per-sample sin calls).
        // Lissajous drift is also blended by obsessionAmount (smooth onset).
        float output = y4;
        if (obsessing)
            output *= (1.0f + cachedObsessionDrift * 0.05f * obsessionAmount);

        return output;
    }

    float getLastOutput() const noexcept { return prevS4 * 0.5f; }

private:
    double sr = 0.0; // P1-7: default 0
    float g = 0.1f;
    float res = 0.0f;
    float a = 1.0f; // P0-2: Huovilainen normalisation factor
    float s1 = 0, s2 = 0, s3 = 0, s4 = 0;
    float prevS1 = 0, prevS2 = 0, prevS3 = 0, prevS4 = 0;
    float lastCutoff = -1.0f;
    float lastResonance = -1.0f;
    float obsessionDrift = 0.0f;
    float obsessionDriftPhase = 0.0f;
    float cachedObsessionDrift = 0.0f; // PERF-3: block-rate cached value
};

} // namespace xoxytocin
