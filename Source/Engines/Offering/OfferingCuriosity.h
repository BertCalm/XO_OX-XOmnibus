#pragma once
//==============================================================================
//
//  OfferingCuriosity.h — Psychology-Driven Timbral Variation Engine
//  XO_OX Designs | XOmnibus Multi-Engine Synthesizer
//
//  Translates published psychological research into DSP parameter variation:
//
//    Berlyne (1960) — Aesthetic curiosity: inverted-U hedonic curve.
//      Moderate novelty = maximum pleasure. Maps to timbral variation range.
//
//    Wundt (1874) — Hedonic intensity curve: asymmetric peak at 0.6.
//      Overstimulation drops pleasure faster than understimulation.
//      Maps to number of parameters varied per trigger.
//
//    Csikszentmihalyi (1975) — Flow state: challenge matches skill.
//      In drumming: pattern complexity vs. groove predictability.
//      Maps to probability of reusing previous variation set.
//
//  V1 = timbral modulation only (no autonomous pattern generation).
//  DIG macro drives curiosity. Mod wheel deepens the dig.
//
//  Accent: Crate Wax Yellow #E5B80B | Prefix: ofr_
//
//==============================================================================

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace xomnibus {

//==============================================================================
// OfferingVariationSet — Cached parameter offsets for one trigger event.
// Stores delta values applied to voice parameters on each hit.
//==============================================================================
struct OfferingVariationSet
{
    float tuneDelta = 0.0f;     // Semitone offset
    float decayDelta = 0.0f;    // Decay time multiplier offset
    float bodyDelta = 0.0f;     // Body balance offset
    float snapDelta = 0.0f;     // Snap sharpness offset
    float pitchEnvDelta = 0.0f; // Pitch envelope depth offset
    float satDelta = 0.0f;      // Saturation offset
};

//==============================================================================
// OfferingCuriosity — The curiosity engine: psychology → DSP.
//
// Called per-voice per-trigger to generate parameter variations.
// State is maintained per-voice (each voice has its own variation cache).
//==============================================================================
class OfferingCuriosity
{
public:
    //--------------------------------------------------------------------------
    // Berlyne Hedonic Curve → ofr_digCuriosity
    // Inverted-U: peak at x=0.5, minimum at 0 and 1.
    // Scaled to [0.2, 1.0] — even at extremes there's some variation.
    //--------------------------------------------------------------------------
    static float berlyneCurve (float x) noexcept
    {
        return 0.2f + 0.8f * (4.0f * x * (1.0f - x));
    }

    //--------------------------------------------------------------------------
    // Wundt Hedonic Curve → ofr_digComplexity
    // Asymmetric: slow ramp to peak at 0.6, sharper fall after.
    // Models Wundt's finding that overstimulation hurts more than understimulation.
    //--------------------------------------------------------------------------
    static float wundtDensity (float x) noexcept
    {
        if (x <= 0.6f)
            return x / 0.6f;
        else
            return 1.0f - ((x - 0.6f) / 0.4f) * 0.7f; // drops to 0.3 at x=1.0
    }

    //--------------------------------------------------------------------------
    // Csikszentmihalyi Flow Balance → ofr_digFlow
    // Probability that next hit reuses previous variation set.
    // 0 = every hit different (chaos), 1 = every hit identical (hypnotic lock).
    //--------------------------------------------------------------------------
    static float flowBalance (float x) noexcept
    {
        return x; // Linear — simplest is correct here
    }

    //--------------------------------------------------------------------------
    // Generate a variation set for a new trigger event.
    //
    // curiosity: Berlyne parameter [0, 1]
    // complexity: Wundt parameter [0, 1]
    // flow: Csikszentmihalyi parameter [0, 1]
    // voiceIndex: voice slot [0-7] for per-voice state
    //
    // Returns: variation offsets to apply to the voice's transient parameters.
    //--------------------------------------------------------------------------
    OfferingVariationSet generateVariation (float curiosity, float complexity,
                                            float flow, int voiceIndex) noexcept
    {
        voiceIndex = std::max (0, std::min (voiceIndex, 7));

        // Flow: decide whether to reuse last variation or generate new
        float rng = nextRandom (voiceIndex);
        if (rng < flowBalance (flow) && hasLastVariation_[voiceIndex])
        {
            // Reuse cached variation — creates the "locked groove" feel
            return lastVariations_[voiceIndex];
        }

        // Generate new variation using Berlyne + Wundt
        float variationRange = berlyneCurve (curiosity);
        float density = wundtDensity (complexity);

        // How many of the 6 voice params to vary
        int paramsToVary = static_cast<int> (density * 6.0f + 0.5f);
        paramsToVary = std::max (0, std::min (paramsToVary, 6));

        // High complexity = fewer params but wilder swings
        float variationIntensity = 1.0f;
        if (complexity > 0.6f)
            variationIntensity = 1.0f + (complexity - 0.6f) / 0.4f * 3.0f;

        OfferingVariationSet var;

        // Alien shift: above curiosity 0.7, base parameters shift toward unusual values
        float alienShift = std::max (0.0f, (curiosity - 0.7f) / 0.3f);

        // Randomly select which params to vary
        if (paramsToVary > 0) var.tuneDelta = randomDelta (voiceIndex) * variationRange * variationIntensity * 2.0f;
        if (paramsToVary > 1) var.decayDelta = randomDelta (voiceIndex) * variationRange * variationIntensity * 0.3f;
        if (paramsToVary > 2) var.bodyDelta = randomDelta (voiceIndex) * variationRange * variationIntensity * 0.2f
                                             - alienShift * 0.6f; // more noise at high curiosity
        if (paramsToVary > 3) var.snapDelta = randomDelta (voiceIndex) * variationRange * variationIntensity * 0.3f;
        if (paramsToVary > 4) var.pitchEnvDelta = randomDelta (voiceIndex) * variationRange * variationIntensity * 0.2f
                                                 + alienShift * 0.4f; // wilder pitch at high curiosity
        if (paramsToVary > 5) var.satDelta = randomDelta (voiceIndex) * variationRange * variationIntensity * 0.15f;

        // Cache for flow reuse
        lastVariations_[voiceIndex] = var;
        hasLastVariation_[voiceIndex] = true;

        return var;
    }

    void reset() noexcept
    {
        for (int i = 0; i < 8; ++i)
        {
            rngState_[i] = static_cast<uint32_t> (i * 7919 + 1);
            hasLastVariation_[i] = false;
            lastVariations_[i] = {};
        }
    }

private:
    //--------------------------------------------------------------------------
    // Per-voice PRNG (xorshift32) for deterministic variation.
    //--------------------------------------------------------------------------
    float nextRandom (int voice) noexcept
    {
        uint32_t& s = rngState_[voice];
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return static_cast<float> (s) / 4294967296.0f; // [0, 1)
    }

    // Returns random value in [-1, 1]
    float randomDelta (int voice) noexcept
    {
        return nextRandom (voice) * 2.0f - 1.0f;
    }

    //--------------------------------------------------------------------------
    // Per-voice state
    //--------------------------------------------------------------------------
    uint32_t rngState_[8] = { 1, 7920, 15839, 23758, 31677, 39596, 47515, 55434 };
    OfferingVariationSet lastVariations_[8] = {};
    bool hasLastVariation_[8] = {};
};

} // namespace xomnibus
