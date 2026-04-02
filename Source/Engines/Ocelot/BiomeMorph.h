// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include <cmath>
#include <algorithm>

namespace xocelot {

// BiomeProfile — compile-time timbral constants for each biome.
// Transition between biomes by interpolating two BiomeProfiles over 200ms.
struct BiomeProfile
{
    float floorDampingBase      = 0.0f;  // additive offset to floor damping
    float floorBrightnessBase   = 0.0f;  // EQ tilt on Floor output (+1 = bright, -1 = dark)
    float understoryWobbleBase  = 0.0f;  // additive tape wobble
    float understoryBitShift    = 0.0f;  // bit depth additive offset (negative = crunchier)
    float canopyPartialTilt     = 0.0f;  // high vs low partial balance
    float canopyBreathePeriod   = 1.0f;  // breathe LFO period multiplier
    float emergentPitchRange    = 0.5f;  // creature pitch sweep range
    float emergentDecayBase     = 0.0f;  // additive to creature tail length
    float humidityCharacter     = 0.0f;  // 0=warm tape, 0.5=depth, 1=frost
    float reverbPreDelayMs      = 20.0f; // pre-delay in ms
    float reverbDiffusion       = 0.7f;  // reverb density
};

// Factory: get the canonical profile for each biome
inline BiomeProfile getJungleProfile()
{
    return {
        .floorDampingBase      = 0.0f,
        .floorBrightnessBase   = 0.1f,
        .understoryWobbleBase  = 0.0f,
        .understoryBitShift    = 0.0f,
        .canopyPartialTilt     = 0.0f,
        .canopyBreathePeriod   = 1.0f,
        .emergentPitchRange    = 0.5f,
        .emergentDecayBase     = 0.0f,
        .humidityCharacter     = 0.0f,  // warm tape
        .reverbPreDelayMs      = 20.0f,
        .reverbDiffusion       = 0.75f,
    };
}

inline BiomeProfile getUnderwaterProfile()
{
    return {
        .floorDampingBase      = 0.3f,   // submerged resonance longer
        .floorBrightnessBase   = -0.5f,  // low-pass dark
        .understoryWobbleBase  = 0.4f,   // slow viscous warp
        .understoryBitShift    = 2.0f,   // softer (higher bit depth effective)
        .canopyPartialTilt     = -0.4f,  // low partials dominant (oceanic depth)
        .canopyBreathePeriod   = 3.0f,   // slow tidal breathe
        .emergentPitchRange    = 0.8f,   // whale sweep is wide
        .emergentDecayBase     = 0.4f,   // long tails (sound carries in water)
        .humidityCharacter     = 0.5f,   // depth character
        .reverbPreDelayMs      = 60.0f,  // sound travels slower
        .reverbDiffusion       = 0.92f,  // ocean cavern
    };
}

inline BiomeProfile getWinterProfile()
{
    return {
        .floorDampingBase      = -0.2f,  // brittle — shorter resonance
        .floorBrightnessBase   = 0.4f,   // crystalline high end
        .understoryWobbleBase  = 0.15f,  // cold warps the mechanism
        .understoryBitShift    = -1.5f,  // colder artifact (more crunch)
        .canopyPartialTilt     = 0.3f,   // high partials dominant (ice)
        .canopyBreathePeriod   = 0.5f,   // wind gusts — faster irregular
        .emergentPitchRange    = 0.35f,  // wolf howl narrow
        .emergentDecayBase     = 0.2f,   // sound carries far in cold
        .humidityCharacter     = 1.0f,   // frost character
        .reverbPreDelayMs      = 10.0f,  // close reflections absorbed by snow
        .reverbDiffusion       = 0.55f,  // sparse winter space
    };
}

// BiomeMorph — interpolates between two BiomeProfiles over a fixed crossfade time.
// Update on audio thread (just linear interp — very cheap).
// Reset / retarget on message thread when ocelot_biome changes.
class BiomeMorph
{
public:
    void prepare(double sampleRate, float crossfadeMs = 200.0f)
    {
        sr = sampleRate;
        crossfadeSamples = static_cast<int>(sampleRate * crossfadeMs * 0.001);
        setCurrent(0); // Jungle
    }

    // Call from message thread when biome parameter changes
    void setTargetBiome(int biomeIndex)
    {
        fromProfile = current;
        toProfile   = profileForIndex(biomeIndex);
        crossfadePos = 0;
        active = true;
    }

    // Call from audio thread once per block — advances crossfade
    void advance(int numSamples)
    {
        if (!active) return;
        crossfadePos += numSamples;
        if (crossfadePos >= crossfadeSamples)
        {
            current = toProfile;
            active  = false;
        }
        else
        {
            float t = static_cast<float>(crossfadePos) / static_cast<float>(crossfadeSamples);
            current = lerp(fromProfile, toProfile, t);
        }
    }

    const BiomeProfile& get() const { return current; }
    bool isTransitioning() const    { return active; }

private:
    double sr = 0.0;
    int crossfadeSamples = 8820; // 200ms at 44.1k — updated in prepare()
    int crossfadePos     = 0;
    bool active          = false;

    BiomeProfile current;
    BiomeProfile fromProfile;
    BiomeProfile toProfile;

    void setCurrent(int index)
    {
        current = profileForIndex(index);
        fromProfile = current;
        toProfile   = current;
        active = false;
    }

    static BiomeProfile profileForIndex(int index)
    {
        switch (index)
        {
            case 1:  return getUnderwaterProfile();
            case 2:  return getWinterProfile();
            default: return getJungleProfile();
        }
    }

    static BiomeProfile lerp(const BiomeProfile& a, const BiomeProfile& b, float t)
    {
        auto mix = [&](float x, float y) { return x + (y - x) * t; };
        return {
            mix(a.floorDampingBase,     b.floorDampingBase),
            mix(a.floorBrightnessBase,  b.floorBrightnessBase),
            mix(a.understoryWobbleBase, b.understoryWobbleBase),
            mix(a.understoryBitShift,   b.understoryBitShift),
            mix(a.canopyPartialTilt,    b.canopyPartialTilt),
            mix(a.canopyBreathePeriod,  b.canopyBreathePeriod),
            mix(a.emergentPitchRange,   b.emergentPitchRange),
            mix(a.emergentDecayBase,    b.emergentDecayBase),
            mix(a.humidityCharacter,    b.humidityCharacter),
            mix(a.reverbPreDelayMs,     b.reverbPreDelayMs),
            mix(a.reverbDiffusion,      b.reverbDiffusion),
        };
    }
};

} // namespace xocelot
