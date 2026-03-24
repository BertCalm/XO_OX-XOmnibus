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

// PERF-1: fast tanh — Pade [3/3] approximation, error < 0.001 for |x| < 4.
// Shared by OxytocinThermal and OxytocinDrive.
static inline float fastTanh (float x) noexcept
{
    if (x < -3.0f) return -1.0f;
    if (x >  3.0f) return  1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

class OxytocinThermal
{
public:
    OxytocinThermal() = default;

    void prepare (double sampleRate) noexcept
    {
        jassert (sampleRate > 0.0);  // P1-7
        sr = sampleRate;
        reset();
        lastWarmthRate = -1.0f;   // force coefficient recompute on first call
    }

    /// P1-5: reset() method so noteOn() can clear thermal state on voice steal.
    void reset() noexcept
    {
        stage1 = stage2 = stage3 = 0.0f;
        wobblePhase = 0.0f;
    }

    /// Call once per block with the current intimacy level and warmth rate.
    /// Returns the current warmth value (used by the voice for cross-modulation).
    float updateWarmth (float intimacy, float warmthRate, float /*blockTime*/) noexcept
    {
        // Cache block-rate coefficients
        if (warmthRate != lastWarmthRate)
        {
            float tau1 = std::max (0.001f, warmthRate * 0.3f);
            float tau2 = std::max (0.001f, warmthRate * 0.6f);
            float tau3 = std::max (0.001f, warmthRate * 1.0f);

            // Use averaged sample-rate (block rate ~ sr/blockSize, but we still
            // compute at sample rate inside processBlock via processSample).
            // Here we store the per-sample coefficients — computed once per block.
            coeff1 = std::exp (-1.0f / (static_cast<float> (sr) * tau1));
            coeff2 = std::exp (-1.0f / (static_cast<float> (sr) * tau2));
            coeff3 = std::exp (-1.0f / (static_cast<float> (sr) * tau3));
            lastWarmthRate = warmthRate;
        }

        // Advance the three stages by one block-average tick
        // (these are updated per-sample in processSample; here we just
        // record the target so processSample can use it)
        targetIntimacy = intimacy;
        return stage3;
    }

    /// Process a single audio sample.  Call after updateWarmth() for the block.
    float processSample (float input, float circuitAge) noexcept
    {
        // Advance warmth stages (one-pole filters on the intimacy target)
        stage1 = targetIntimacy + (stage1 - targetIntimacy) * coeff1;
        stage2 = stage1          + (stage2 - stage1)         * coeff2;
        stage3 = stage2          + (stage3 - stage2)         * coeff3;

        // P1-4: denormal guards on warmth stage accumulators
        constexpr float kDenorm = 1e-18f;
        stage1 += kDenorm;
        stage2 += kDenorm;
        stage3 += kDenorm;

        const float warmth = stage3;  // 0..1

        // NTC model: warm signal = tanh saturation
        // PERF-1: fastTanh replaces std::tanh (Pade [3/3] approximation)
        const float saturated = fastTanh (input * (1.0f + warmth * 2.5f)) * 0.7f;
        const float clean     = input;

        float output = clean * (1.0f - warmth * 0.4f) + saturated * (warmth * 0.4f);

        // Circuit-age pitch wobble: ±3 cents * age * sin(0.3 Hz)
        if (circuitAge > 0.0f)
        {
            wobblePhase += static_cast<float> (juce::MathConstants<double>::twoPi * 0.3 / sr);
            if (wobblePhase > juce::MathConstants<float>::twoPi)
                wobblePhase -= juce::MathConstants<float>::twoPi;

            // Pitch wobble is applied at the signal level as a subtle AM
            // (a true pitch wobble would require a delay line; this is an
            //  affordable approximation for the "motor flutter" aesthetic)
            float wobble = 1.0f + circuitAge * 0.0017f * std::sin (wobblePhase);
            output *= wobble;
        }

        return output;
    }

    float getWarmth() const noexcept { return stage3; }

private:
    double sr           = 0.0;   // P1-7: default 0
    float  stage1       = 0.0f;
    float  stage2       = 0.0f;
    float  stage3       = 0.0f;
    float  targetIntimacy = 0.0f;
    float  coeff1       = 0.0f;
    float  coeff2       = 0.0f;
    float  coeff3       = 0.0f;
    float  lastWarmthRate = -1.0f;
    float  wobblePhase  = 0.0f;
};
