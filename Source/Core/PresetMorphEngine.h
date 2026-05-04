// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
//
// PresetMorphEngine.h — parameter interpolation primitives between two PresetData
// snapshots. Header-only utility consumed by:
//   - Issue #9  (Preset Gradient Morphing / Sonic DNA Drift)
//   - Issue #2  (Coupling Mutation Live Breeding)
//
// Uses DNAProximity.h for 6D Sonic DNA distance, interpolation, and utility functions.
#pragma once

#include "PresetManager.h"  // PresetData, PresetDNA, CouplingPair
#include "DNAProximity.h"

#include <juce_core/juce_core.h>
#include <cmath>

namespace xoceanus
{

//==============================================================================
namespace PresetMorphEngine
{

//------------------------------------------------------------------------------
// Internal helpers (not part of the public API).
namespace detail
{
inline float clamp01(float t) noexcept { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); }
inline bool endsWith(const juce::String& s, const char* x) noexcept { return s.endsWith(x); }
inline bool contains(const juce::String& s, const char* x) noexcept { return s.contains(x); }

/** Coupling-type to name modifier lookup for breedFromCoupling auto-naming. */
inline juce::String couplingTypeName(const juce::String& type) noexcept
{
    if (type == "AmpToFilter"   || type == "Amp->Filter")   return "Amp Mesh";
    if (type == "KnotTopology"  || type == "Knot")          return "Knot Hybrid";
    if (type == "PhaseModulation" || type == "PhaseMod")    return "Phase Cross";
    if (type == "FrequencySync" || type == "FreqSync")      return "Freq Weave";
    if (type == "RingMod"       || type == "Ring")          return "Ring Fuse";
    if (type == "WaveshapeShare"|| type == "Waveshape")     return "Wave Bridge";
    if (type == "TriangularCoupling" || type == "Triangle") return "Tri Mesh";
    if (type == "EnvelopeFollow"|| type == "EnvFollow")     return "Env Cross";
    if (type == "PitchLock"     || type == "Pitch")         return "Pitch Bond";
    if (type == "GainDuck"      || type == "Duck")          return "Duck Hybrid";
    return {};  // empty — caller falls back to "<a> <-> <b>"
}

} // namespace detail

//==============================================================================
// Public API
//==============================================================================

/** Decide whether a parameter ID is "structural" — meaning crossfading it
    smoothly would produce nonsensical sound (e.g. oscillator count, routing
    topology, wavetable file path). Structural params crossfade at the midpoint.
    Returns true for known structural parameter ID patterns.

    Structural patterns detected:
      - ends with "Mode", "Type", "Wave", "OscMode", "OscWave", "WaveType"
      - ends with "Topology", "RuleSet", "RoutingMode", "WavetablePath"
      - contains "routing" (case-sensitive)
      - contains "ruleSet", "topology"
      - ends with "Count" (voice count, osc count — integer-stepped)
      - ends with "Bank" (sample bank index — discrete) */
inline bool isStructuralParam(const juce::String& paramId) noexcept
{
    // Exact suffix matches (most common patterns first)
    if (detail::endsWith(paramId, "Mode"))          return true;
    if (detail::endsWith(paramId, "Type"))          return true;
    if (detail::endsWith(paramId, "Wave"))          return true;
    if (detail::endsWith(paramId, "OscMode"))       return true;
    if (detail::endsWith(paramId, "OscWave"))       return true;
    if (detail::endsWith(paramId, "WaveType"))      return true;
    if (detail::endsWith(paramId, "Topology"))      return true;
    if (detail::endsWith(paramId, "RuleSet"))       return true;
    if (detail::endsWith(paramId, "RoutingMode"))   return true;
    if (detail::endsWith(paramId, "WavetablePath")) return true;
    if (detail::endsWith(paramId, "Count"))         return true;
    if (detail::endsWith(paramId, "Bank"))          return true;

    // Substring matches for mid-string structural tokens
    if (detail::contains(paramId, "routing"))       return true;
    if (detail::contains(paramId, "ruleSet"))       return true;
    if (detail::contains(paramId, "topology"))      return true;
    if (detail::contains(paramId, "Routing"))       return true;

    return false;
}

//------------------------------------------------------------------------------
/** Linear interpolate a single parameter value.
    For structural params, returns valueA when t < 0.5 and valueB when t >= 0.5
    (hard-switch at midpoint). For continuous params, simple lerp. */
inline float interpolateParam(const juce::String& paramId,
                               float valueA, float valueB, float t) noexcept
{
    const float tc = detail::clamp01(t);
    if (isStructuralParam(paramId))
        return tc < 0.5f ? valueA : valueB;

    return valueA + (valueB - valueA) * tc;
}

//------------------------------------------------------------------------------
/** Interpolate every parameter in `dst` between snapshots `a` and `b` at t in [0,1].
    DynamicObject params lerped per-key. Structural params hard-switch at midpoint.
    DNA interpolated via DNAProximity::interpolate.
    Tags: union. Coupling, engines, macros: dominant side (t<0.5→a, else b). */
inline void interpolatePreset(const PresetData& a,
                               const PresetData& b,
                               float t,
                               PresetData& dst) noexcept
{
    const float tc      = detail::clamp01(t);
    const bool  aDom    = tc < 0.5f;

    // --- Scalar / string fields: dominant side ---
    const PresetData& dom = aDom ? a : b;
    dst.schemaVersion    = dom.schemaVersion;
    dst.mood             = dom.mood;
    dst.category         = dom.category;
    dst.timbre           = dom.timbre;
    dst.tier             = dom.tier;
    dst.engines          = dom.engines;
    dst.author           = dom.author;
    dst.version          = dom.version;
    dst.description      = dom.description;
    dst.macroLabels      = dom.macroLabels;
    dst.couplingIntensity= dom.couplingIntensity;
    dst.tempo            = dom.tempo; // dominant side tempo
    dst.couplingPairs    = dom.couplingPairs;
    dst.macroTargets     = dom.macroTargets;
    dst.sequencerData    = dom.sequencerData;

    // --- Tags: union ---
    dst.tags = a.tags;
    for (int i = 0; i < b.tags.size(); ++i)
        dst.tags.addIfNotAlreadyThere(b.tags[i]);

    // --- DNA: interpolate ---
    dst.dna = DNAProximity::interpolate(a.dna, b.dna, tc);

    // --- Parameters: walk all engine keys ---
    dst.parametersByEngine.clear();

    // Collect all engine keys from both snapshots
    juce::StringArray engineKeys;
    for (const auto& kv : a.parametersByEngine)
        engineKeys.addIfNotAlreadyThere(kv.first);
    for (const auto& kv : b.parametersByEngine)
        engineKeys.addIfNotAlreadyThere(kv.first);

    for (const auto& engineKey : engineKeys)
    {
        const auto itA = a.parametersByEngine.find(engineKey);
        const auto itB = b.parametersByEngine.find(engineKey);

        if (itA == a.parametersByEngine.end())
        {
            // Only in b — copy directly
            dst.parametersByEngine[engineKey] = itB->second;
            continue;
        }
        if (itB == b.parametersByEngine.end())
        {
            // Only in a — copy directly
            dst.parametersByEngine[engineKey] = itA->second;
            continue;
        }

        // Both snapshots have this engine — interpolate per parameter
        const juce::var& varA = itA->second;
        const juce::var& varB = itB->second;

        auto* objA = varA.getDynamicObject();
        auto* objB = varB.getDynamicObject();

        if (objA == nullptr || objB == nullptr)
        {
            // Not DynamicObjects — take dominant side
            dst.parametersByEngine[engineKey] = aDom ? varA : varB;
            continue;
        }

        auto* dstObj = new juce::DynamicObject();

        // Walk A's properties
        const auto& propsA = objA->getProperties();
        for (int i = 0; i < propsA.size(); ++i)
        {
            const juce::Identifier& id = propsA.getName(i);
            const float vA = static_cast<float>(static_cast<double>(propsA.getValueAt(i)));
            const juce::var& rawB = objB->getProperty(id);
            const float vB = rawB.isUndefined() ? vA
                                                 : static_cast<float>(static_cast<double>(rawB));
            dstObj->setProperty(id, interpolateParam(id.toString(), vA, vB, tc));
        }

        // Walk B's properties not already in A
        const auto& propsB = objB->getProperties();
        for (int i = 0; i < propsB.size(); ++i)
        {
            const juce::Identifier& id = propsB.getName(i);
            if (!propsA.contains(id))
            {
                const float vB = static_cast<float>(static_cast<double>(propsB.getValueAt(i)));
                dstObj->setProperty(id, vB);
            }
        }

        dst.parametersByEngine[engineKey] = juce::var(dstObj);
    }
}

//------------------------------------------------------------------------------
/** Snapshot the current state of a morph: returns a fresh PresetData that
    captures the morph result at position t with auto-name
    "Morph <percent>%: <a.name> <-> <b.name>". */
inline PresetData snapshotMorph(const PresetData& a,
                                 const PresetData& b,
                                 float t) noexcept
{
    PresetData dst;
    interpolatePreset(a, b, t, dst);

    const int pct = static_cast<int>(std::round(detail::clamp01(t) * 100.0f));
    dst.name = juce::String("Morph ") + juce::String(pct)
               + juce::String("%: ") + a.name
               + juce::String(" <-> ") + b.name;

    return dst;
}

//------------------------------------------------------------------------------
/** DNA-weighted merge for the Live Breeding feature (issue #2).
    Weight: wA = (1-dist(a.dna, centroid)) / sum; wB = 1-wA. Falls back to 0.5
    for identical-DNA parents. couplingDepth >= 0.5 stamps a coupling-type name;
    < 0.5 produces "<a.name> <-> <b.name>". Returns a newly allocated PresetData. */
inline PresetData breedFromCoupling(const PresetData& engineA,
                                     const PresetData& engineB,
                                     const juce::String& couplingType,
                                     float couplingDepth) noexcept
{
    // --- Compute DNA-derived blend weight ---
    const std::vector<PresetDNA> dnaVec = { engineA.dna, engineB.dna };
    const PresetDNA cen = DNAProximity::centroid(dnaVec);
    const float distA   = DNAProximity::distance(engineA.dna, cen);
    const float distB   = DNAProximity::distance(engineB.dna, cen);

    const float rawA    = 1.0f - distA;
    const float rawB    = 1.0f - distB;
    const float sum     = rawA + rawB;

    float wA = 0.5f;
    if (sum > 1e-6f)
        wA = rawA / sum;

    // --- Blend presets at DNA-weighted position ---
    PresetData result;
    interpolatePreset(engineA, engineB, 1.0f - wA, result);

    // --- Auto-name ---
    const juce::String typeName = detail::couplingTypeName(couplingType);
    const float        depth    = detail::clamp01(couplingDepth);

    if (depth >= 0.5f && typeName.isNotEmpty())
    {
        result.name = typeName;
    }
    else
    {
        result.name = engineA.name + juce::String(" <-> ") + engineB.name;
    }

    return result;
}

} // namespace PresetMorphEngine
} // namespace xoceanus
