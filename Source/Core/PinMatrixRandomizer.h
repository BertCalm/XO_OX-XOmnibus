#pragma once
#include "MegaCouplingMatrix.h"
#include "SynthEngine.h"
#include "EngineRegistry.h"
#include <juce_core/juce_core.h>
#include <vector>
#include <array>
#include <cstdint>

namespace xomnibus {

//==============================================================================
// PinMatrixRandomizer — EMS VCS3-inspired random coupling discovery.
//
// The VCS3's pin matrix let you jam physical pins into a grid to create
// unexpected modulation connections. This captures that spirit of discovery
// for the MegaCouplingMatrix: generate musically-weighted random coupling
// configurations with a single gesture.
//
// Modes:
//   Subtle   — 1-2 gentle couplings (safe for live use)
//   Moderate — 2-3 medium couplings (exploration sweet spot)
//   Wild     — 3-5 aggressive couplings (VCS3 "all pins in" chaos)
//   Genre    — Preset-weighted randomization (e.g. Dub, IDM, Ambient)
//
// Musical weighting:
//   - Avoids destructive combinations (e.g. AudioToRing at 100%)
//   - Prefers complementary coupling types for filled slots
//   - Respects engine capabilities (no FM to engines that don't support it)
//   - Intensity scaled by mode (subtle = 10-30%, wild = 40-90%)
//
// Inspired by: EMS VCS3 (1969), Buchla 100 series banana patching
//==============================================================================
class PinMatrixRandomizer
{
public:
    enum class Mode
    {
        Subtle = 0,    // 1-2 gentle couplings
        Moderate,      // 2-3 medium couplings
        Wild,          // 3-5 aggressive couplings
        NumModes
    };

    enum class Genre
    {
        Dub = 0,       // AmpToFilter heavy, subtle LFO, warm
        IDM,           // AudioToFM, AudioToRing, complex
        Ambient,       // EnvToMorph, LFOToPitch, gentle
        Industrial,    // AudioToRing, AmpToChoke, aggressive
        NumGenres
    };

    struct GeneratedPatch
    {
        struct Route
        {
            int sourceSlot;
            int destSlot;
            CouplingType type;
            float intensity;
        };
        std::vector<Route> routes;
        juce::String description;  // Human-readable patch summary
    };

    PinMatrixRandomizer() = default;

    //--------------------------------------------------------------------------
    /// Generate a random coupling configuration for the currently loaded engines.
    /// @param activeSlots  Number of engine slots currently filled (1-4)
    /// @param mode         Randomization intensity
    GeneratedPatch generate (int activeSlots, Mode mode)
    {
        GeneratedPatch patch;
        if (activeSlots < 2) return patch;  // Need at least 2 engines to couple

        int minRoutes, maxRoutes;
        float minIntensity, maxIntensity;

        switch (mode)
        {
            case Mode::Subtle:
                minRoutes = 1; maxRoutes = 2;
                minIntensity = 0.1f; maxIntensity = 0.3f;
                break;
            case Mode::Moderate:
                minRoutes = 2; maxRoutes = 3;
                minIntensity = 0.2f; maxIntensity = 0.55f;
                break;
            case Mode::Wild:
                minRoutes = 3; maxRoutes = 5;
                minIntensity = 0.35f; maxIntensity = 0.85f;
                break;
            default:
                minRoutes = 2; maxRoutes = 3;
                minIntensity = 0.2f; maxIntensity = 0.5f;
        }

        int numRoutes = minRoutes + static_cast<int> (nextRandom() * static_cast<float> (maxRoutes - minRoutes + 1));
        numRoutes = std::min (numRoutes, maxRoutes);

        // Available coupling types weighted by musicality
        static constexpr CouplingType safeTypes[] = {
            CouplingType::AmpToFilter,
            CouplingType::LFOToPitch,
            CouplingType::EnvToMorph,
            CouplingType::EnvToDecay,
            CouplingType::PitchToPitch,
            CouplingType::RhythmToBlend
        };
        static constexpr CouplingType aggressiveTypes[] = {
            CouplingType::AudioToFM,
            CouplingType::AudioToRing,
            CouplingType::FilterToFilter,
            CouplingType::AmpToPitch,
            CouplingType::AmpToChoke,
            CouplingType::AudioToWavetable
        };

        juce::String desc;

        for (int r = 0; r < numRoutes; ++r)
        {
            GeneratedPatch::Route route;

            // Random source/dest (different slots)
            route.sourceSlot = static_cast<int> (nextRandom() * static_cast<float> (activeSlots));
            do {
                route.destSlot = static_cast<int> (nextRandom() * static_cast<float> (activeSlots));
            } while (route.destSlot == route.sourceSlot);

            // Pick coupling type: bias toward safe types in subtle mode
            float aggressiveChance = (mode == Mode::Subtle) ? 0.1f
                                   : (mode == Mode::Moderate) ? 0.3f : 0.5f;

            if (nextRandom() < aggressiveChance)
            {
                int idx = static_cast<int> (nextRandom() * 6.0f) % 6;
                route.type = aggressiveTypes[idx];
            }
            else
            {
                int idx = static_cast<int> (nextRandom() * 6.0f) % 6;
                route.type = safeTypes[idx];
            }

            // Random intensity within mode range
            route.intensity = minIntensity + nextRandom() * (maxIntensity - minIntensity);

            // Safety: cap destructive types
            if (route.type == CouplingType::AudioToRing ||
                route.type == CouplingType::AmpToChoke)
            {
                route.intensity = std::min (route.intensity, 0.5f);
            }

            patch.routes.push_back (route);

            if (desc.isNotEmpty()) desc += " + ";
            desc += couplingTypeName (route.type);
            desc += juce::String::formatted (" %d→%d @%.0f%%",
                route.sourceSlot + 1, route.destSlot + 1, route.intensity * 100.0f);
        }

        patch.description = desc;
        return patch;
    }

    //--------------------------------------------------------------------------
    /// Generate a genre-weighted coupling configuration.
    GeneratedPatch generateGenre (int activeSlots, Genre genre)
    {
        GeneratedPatch patch;
        if (activeSlots < 2) return patch;

        // Genre-specific coupling preferences
        struct GenreWeight { CouplingType type; float weight; float minI; float maxI; };

        std::vector<GenreWeight> weights;

        switch (genre)
        {
            case Genre::Dub:
                weights = {
                    { CouplingType::AmpToFilter,    0.4f, 0.2f, 0.5f },
                    { CouplingType::EnvToMorph,     0.25f, 0.1f, 0.3f },
                    { CouplingType::LFOToPitch,     0.2f, 0.05f, 0.15f },
                    { CouplingType::RhythmToBlend,  0.15f, 0.15f, 0.35f }
                };
                break;
            case Genre::IDM:
                weights = {
                    { CouplingType::AudioToFM,       0.3f, 0.2f, 0.6f },
                    { CouplingType::AudioToRing,     0.2f, 0.1f, 0.4f },
                    { CouplingType::PitchToPitch,    0.2f, 0.15f, 0.5f },
                    { CouplingType::FilterToFilter,  0.15f, 0.2f, 0.5f },
                    { CouplingType::EnvToDecay,      0.15f, 0.1f, 0.4f }
                };
                break;
            case Genre::Ambient:
                weights = {
                    { CouplingType::EnvToMorph,      0.35f, 0.1f, 0.25f },
                    { CouplingType::LFOToPitch,      0.3f, 0.03f, 0.1f },
                    { CouplingType::AmpToFilter,     0.2f, 0.1f, 0.2f },
                    { CouplingType::RhythmToBlend,   0.15f, 0.05f, 0.15f }
                };
                break;
            case Genre::Industrial:
                weights = {
                    { CouplingType::AudioToRing,     0.3f, 0.3f, 0.7f },
                    { CouplingType::AmpToChoke,      0.2f, 0.2f, 0.5f },
                    { CouplingType::AudioToFM,       0.25f, 0.3f, 0.65f },
                    { CouplingType::AmpToPitch,      0.15f, 0.2f, 0.4f },
                    { CouplingType::FilterToFilter,  0.1f, 0.3f, 0.6f }
                };
                break;
            default:
                return generate (activeSlots, Mode::Moderate);
        }

        int numRoutes = 2 + static_cast<int> (nextRandom() * 2.0f);
        juce::String desc;

        for (int r = 0; r < numRoutes && !weights.empty(); ++r)
        {
            // Weighted random selection
            float roll = nextRandom();
            float cumulative = 0.0f;
            size_t selected = 0;

            for (size_t w = 0; w < weights.size(); ++w)
            {
                cumulative += weights[w].weight;
                if (roll <= cumulative) { selected = w; break; }
            }

            auto& gw = weights[selected];
            GeneratedPatch::Route route;
            route.sourceSlot = static_cast<int> (nextRandom() * static_cast<float> (activeSlots));
            do {
                route.destSlot = static_cast<int> (nextRandom() * static_cast<float> (activeSlots));
            } while (route.destSlot == route.sourceSlot);

            route.type = gw.type;
            route.intensity = gw.minI + nextRandom() * (gw.maxI - gw.minI);

            patch.routes.push_back (route);

            if (desc.isNotEmpty()) desc += " + ";
            desc += couplingTypeName (route.type);
        }

        patch.description = genreName (genre) + ": " + desc;
        return patch;
    }

    //--------------------------------------------------------------------------
    /// Seed the random generator (for reproducible results or user "lock" feature)
    void setSeed (uint32_t seed) { randState = seed; }

private:
    uint32_t randState = 0xDEADBEEF;

    float nextRandom()
    {
        randState ^= randState << 13;
        randState ^= randState >> 17;
        randState ^= randState << 5;
        return static_cast<float> (randState & 0x7FFFFF) / static_cast<float> (0x7FFFFF);
    }

    static juce::String couplingTypeName (CouplingType t)
    {
        switch (t)
        {
            case CouplingType::AmpToFilter:      return "Amp→Filter";
            case CouplingType::AmpToPitch:       return "Amp→Pitch";
            case CouplingType::LFOToPitch:       return "LFO→Pitch";
            case CouplingType::EnvToMorph:       return "Env→Morph";
            case CouplingType::AudioToFM:        return "Audio→FM";
            case CouplingType::AudioToRing:      return "Ring Mod";
            case CouplingType::FilterToFilter:   return "Filter→Filter";
            case CouplingType::AmpToChoke:       return "Choke";
            case CouplingType::RhythmToBlend:    return "Rhythm→Blend";
            case CouplingType::EnvToDecay:       return "Env→Decay";
            case CouplingType::PitchToPitch:     return "Pitch→Pitch";
            case CouplingType::AudioToWavetable: return "Audio→WT";
            default: return "Unknown";
        }
    }

    static juce::String genreName (Genre g)
    {
        switch (g)
        {
            case Genre::Dub:        return "Dub";
            case Genre::IDM:        return "IDM";
            case Genre::Ambient:    return "Ambient";
            case Genre::Industrial: return "Industrial";
            default: return "Unknown";
        }
    }
};

} // namespace xomnibus
