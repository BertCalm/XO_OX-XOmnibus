// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "PresetManager.h"
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

//==============================================================================
// PresetMorpher — Morphing, breeding, and smart randomization for XOceanus
//                 .xometa presets.
//
// Design contract:
//   - UI-thread only. No audio-thread calls.
//   - No std::random — uses a fast xorshift64 PRNG throughout.
//   - All operations return new PresetData; inputs are never mutated.
//   - Compatible with parametersByEngine using juce::var (DynamicObject) values,
//     exactly as written by PresetManager::parseJSON.
//
// Sections:
//   1. xorshift64 PRNG
//   2. Parameter classification helpers
//   3. DNA → parameter-weight mapping
//   4. Preset Interpolation (morph)
//   5. DNA-Based Breeding (breed)
//   6. Smart Randomization (randomize)
//   7. DNA Distance + Nearest Search (findNearest)
//   8. PresetMorpher class (public API)
//
// All free functions live inside namespace xoceanus::morpher to avoid
// polluting the top-level namespace. The public PresetMorpher class is
// in namespace xoceanus.
//==============================================================================

namespace xoceanus
{

//==============================================================================
// Section 1 — xorshift64 PRNG
//==============================================================================

namespace morpher
{

/// Minimal xorshift64 PRNG (Marsaglia 2003).
/// State must be non-zero; seed with any non-zero value.
struct Xorshift64
{
    uint64_t state;

    explicit Xorshift64(uint64_t seed = 0xDEADBEEFCAFEBABEull) : state(seed == 0 ? 0xDEADBEEFCAFEBABEull : seed) {}

    uint64_t next() noexcept
    {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return state;
    }

    /// Uniform float in [0, 1).
    float nextFloat() noexcept
    {
        // Use the top 23 mantissa bits so every ULP is reachable.
        uint32_t bits = static_cast<uint32_t>(next() >> 41) | 0x3F800000u;
        float f;
        std::memcpy(&f, &bits, sizeof(float));
        return f - 1.0f;
    }

    /// Uniform float in [lo, hi).
    float nextFloat(float lo, float hi) noexcept { return lo + nextFloat() * (hi - lo); }

    /// Uniform int in [0, n).  n must be > 0.
    int nextInt(int n) noexcept { return static_cast<int>(next() % static_cast<uint64_t>(n)); }

    /// Seed from wall-clock time (nanosecond precision) — provides unique
    /// sequences per call without touching std::random_device.
    void seedFromTime() noexcept
    {
        state = static_cast<uint64_t>(juce::Time::getHighResolutionTicks());
        if (state == 0)
            state = 0xDEADBEEFCAFEBABEull;
        // Warm up to break temporal correlations.
        next();
        next();
        next();
    }
};

//==============================================================================
// Section 2 — Parameter classification helpers
//==============================================================================

/// Returns true if a parameter value should be treated as an integer
/// (enum / discrete choice) rather than a continuous float.
/// Heuristic: if the stored juce::var is an int, or if the parameter name
/// contains well-known integer suffixes / keywords.
inline bool isIntegerParam(const juce::String& paramId, const juce::var& value)
{
    // Explicit type check first — the most reliable signal.
    if (value.isInt())
        return true;

    // Name-based heuristics for parameters stored as floats but semantically
    // discrete (wave types, mode selectors, divider counts, etc.).
    // Convention: XOceanus engines use these suffixes/keywords for integers.
    static const juce::StringArray integerKeywords{
        "Mode",   "Wave",    "Type",  "Shape",  "Select", "Div",    "Oct",   "Octave", "Era",
        "Scale",  "Pattern", "Rule",  "Legato", "Glide",  "Bus",    "Chord", "Algo",   "Version",
        "Preset", "Slot",    "Index", "Step",   "Count",  "Number", "Set",
    };

    for (const auto& kw : integerKeywords)
        if (paramId.containsIgnoreCase(kw))
            return true;

    return false;
}

/// Clamp a float to [0, 1] — used for normalized parameter values.
inline float clamp01(float v) noexcept
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

/// Clamp a float to an arbitrary range.
inline float clampRange(float v, float lo, float hi) noexcept
{
    return v < lo ? lo : (v > hi ? hi : v);
}

//==============================================================================
// Section 3 — DNA dimension → parameter keyword mapping
//
// Each entry maps a DNA dimension to a list of parameter-name fragments.
// When a parameter name contains a listed fragment (case-insensitive) its
// value is used as a proxy for that DNA dimension when computing the
// "parameter-derived DNA" for breeding fitness scoring.
//
// This mapping also drives randomization bias (perturb parameters that
// contribute to a dimension proportionally to their DNA weight).
//==============================================================================

enum class DNADimension
{
    Brightness,
    Warmth,
    Movement,
    Density,
    Space,
    Aggression
};

struct DNAMapping
{
    DNADimension dim;
    float weight;          // Relative weight of this keyword vs others.
    juce::String fragment; // Case-insensitive substring of the param ID.
};

/// Returns the static mapping table.  All weights are positive; they are
/// normalised per-dimension when used for fitness scoring.
inline const std::vector<DNAMapping>& getDNAMappings()
{
    // Defined once; shared across all PresetMorpher instances.
    static const std::vector<DNAMapping> mappings = {
        //--- Brightness ---
        {DNADimension::Brightness, 1.0f, "filterCutoff"},
        {DNADimension::Brightness, 1.0f, "fltCutoff"},
        {DNADimension::Brightness, 0.8f, "cutoff"},
        {DNADimension::Brightness, 0.8f, "brightness"},
        {DNADimension::Brightness, 0.6f, "hipass"},
        {DNADimension::Brightness, 0.6f, "highpass"},
        {DNADimension::Brightness, 0.5f, "filterEnvAmt"},
        {DNADimension::Brightness, 0.5f, "filterEnvDepth"},
        {DNADimension::Brightness, 0.4f, "filterReso"},
        {DNADimension::Brightness, 0.4f, "resonance"},
        {DNADimension::Brightness, 0.4f, "filterTrack"},
        //--- Warmth ---
        {DNADimension::Warmth, 1.0f, "satDrive"},
        {DNADimension::Warmth, 1.0f, "drive"},
        {DNADimension::Warmth, 0.9f, "saturation"},
        {DNADimension::Warmth, 0.8f, "warmth"},
        {DNADimension::Warmth, 0.8f, "overdrive"},
        {DNADimension::Warmth, 0.7f, "distortion"},
        {DNADimension::Warmth, 0.6f, "bodyLevel"},
        {DNADimension::Warmth, 0.6f, "subMix"},
        {DNADimension::Warmth, 0.5f, "waveform"},
        {DNADimension::Warmth, 0.5f, "oscWave"},
        {DNADimension::Warmth, 0.5f, "fundWave"},
        {DNADimension::Warmth, 0.4f, "fold"},
        {DNADimension::Warmth, 0.4f, "crush"},
        //--- Movement ---
        {DNADimension::Movement, 1.0f, "lfoRate"},
        {DNADimension::Movement, 1.0f, "lfo1Rate"},
        {DNADimension::Movement, 1.0f, "lfo2Rate"},
        {DNADimension::Movement, 0.9f, "lfoDepth"},
        {DNADimension::Movement, 0.9f, "lfo1Depth"},
        {DNADimension::Movement, 0.9f, "lfo2Depth"},
        {DNADimension::Movement, 0.8f, "movement"},
        {DNADimension::Movement, 0.7f, "vibrato"},
        {DNADimension::Movement, 0.7f, "tremolo"},
        {DNADimension::Movement, 0.7f, "portamento"},
        {DNADimension::Movement, 0.6f, "modDepth"},
        {DNADimension::Movement, 0.6f, "modRate"},
        {DNADimension::Movement, 0.5f, "ampDecay"},
        {DNADimension::Movement, 0.5f, "glide"},
        //--- Density ---
        {DNADimension::Density, 1.0f, "grainDensity"},
        {DNADimension::Density, 0.9f, "density"},
        {DNADimension::Density, 0.8f, "unison"},
        {DNADimension::Density, 0.8f, "spread"},
        {DNADimension::Density, 0.7f, "voices"},
        {DNADimension::Density, 0.7f, "detune"},
        {DNADimension::Density, 0.6f, "chordDepth"},
        {DNADimension::Density, 0.6f, "harmonic"},
        {DNADimension::Density, 0.5f, "mixtur"},
        {DNADimension::Density, 0.5f, "chorus"},
        {DNADimension::Density, 0.4f, "stereoWidth"},
        //--- Space ---
        {DNADimension::Space, 1.0f, "reverbMix"},
        {DNADimension::Space, 1.0f, "reverbSize"},
        {DNADimension::Space, 0.9f, "space"},
        {DNADimension::Space, 0.9f, "delay"},
        {DNADimension::Space, 0.8f, "reverbDamp"},
        {DNADimension::Space, 0.8f, "roomSize"},
        {DNADimension::Space, 0.7f, "delayMix"},
        {DNADimension::Space, 0.7f, "outputPan"},
        {DNADimension::Space, 0.6f, "pan"},
        {DNADimension::Space, 0.6f, "spread"},
        {DNADimension::Space, 0.5f, "shimmer"},
        {DNADimension::Space, 0.5f, "entangle"},
        //--- Aggression ---
        {DNADimension::Aggression, 1.0f, "aggression"},
        {DNADimension::Aggression, 1.0f, "compRatio"},
        {DNADimension::Aggression, 0.9f, "compThreshold"},
        {DNADimension::Aggression, 0.8f, "bite"},
        {DNADimension::Aggression, 0.8f, "gnash"},
        {DNADimension::Aggression, 0.7f, "attack"},
        {DNADimension::Aggression, 0.7f, "transient"},
        {DNADimension::Aggression, 0.6f, "punch"},
        {DNADimension::Aggression, 0.6f, "pressure"},
        {DNADimension::Aggression, 0.5f, "ampAttack"},
        {DNADimension::Aggression, 0.5f, "noiseLevel"},
        {DNADimension::Aggression, 0.4f, "velocity"},
    };
    return mappings;
}

/// Compute the weighted contribution of a parameter to a given DNA dimension.
/// Returns 0 if the parameter has no mapping for that dimension.
inline float paramDNAWeight(const juce::String& paramId, DNADimension dim)
{
    float total = 0.0f;
    for (const auto& m : getDNAMappings())
        if (m.dim == dim && paramId.containsIgnoreCase(m.fragment))
            total += m.weight;
    return total;
}

/// Estimate a DNA value for a given dimension from a single engine's parameter
/// map.  The estimate is a weighted average of the relevant parameter values.
/// Returns 0.5 (neutral) if no relevant parameters are found.
inline float estimateDNAFromParams(const juce::var& engineParams, DNADimension dim)
{
    if (!engineParams.isObject())
        return 0.5f;

    auto* obj = engineParams.getDynamicObject();
    if (obj == nullptr)
        return 0.5f;

    float weightedSum = 0.0f;
    float totalWeight = 0.0f;

    for (const auto& prop : obj->getProperties())
    {
        juce::String paramId = prop.name.toString();
        float w = paramDNAWeight(paramId, dim);
        if (w <= 0.0f)
            continue;

        float val = clamp01(static_cast<float>(prop.value));
        weightedSum += w * val;
        totalWeight += w;
    }

    return (totalWeight > 0.0f) ? clamp01(weightedSum / totalWeight) : 0.5f;
}

//==============================================================================
// Section 4 — Preset Interpolation
//
// morph(a, b, t) blends two presets at position t ∈ [0, 1].
//
// Rules:
//   - Float params: linear interpolation between same-named parameters.
//   - Integer / enum params: step-switch at t = 0.5.
//   - Engine selection: each slot switches at t = 0.5 if engines differ.
//   - Cross-engine morphing: shared param names (matching suffix after prefix_)
//     are blended when engines differ; engine-specific params are kept from the
//     dominant side (t < 0.5 → A, t >= 0.5 → B).
//   - DNA: linearly interpolated.
//   - Metadata: taken from A when t < 0.5, from B otherwise, with an
//     auto-generated name like "A × B".
//   - CouplingPairs: taken from A when t < 0.5, from B otherwise (discrete).
//==============================================================================

/// Strip the engine prefix from a parameter ID so we can compare across engines.
/// e.g. "owl_filterCutoff" → "filterCutoff",  "snap_filterReso" → "filterReso"
inline juce::String stripEnginePrefix(const juce::String& paramId)
{
    int underscoreIdx = paramId.indexOfChar('_');
    if (underscoreIdx > 0)
        return paramId.substring(underscoreIdx + 1);
    return paramId;
}

/// Interpolate two juce::var parameter values.
/// For integers: step at t = 0.5. For floats: lerp.
inline juce::var lerpParam(const juce::String& paramId, const juce::var& a, const juce::var& b, float t)
{
    // Both values must be numeric for interpolation to be meaningful.
    bool aNum = a.isDouble() || a.isInt();
    bool bNum = b.isDouble() || b.isInt();

    if (!aNum && !bNum)
        return (t < 0.5f) ? a : b; // non-numeric: step switch
    if (!aNum)
        return b;
    if (!bNum)
        return a;

    // Integer (enum / discrete) parameters: step-switch at t = 0.5.
    if (isIntegerParam(paramId, a) || isIntegerParam(paramId, b))
        return (t < 0.5f) ? a : b;

    // Continuous float: linear interpolation.
    float fa = static_cast<float>(a);
    float fb = static_cast<float>(b);
    return juce::var(static_cast<double>(fa + t * (fb - fa)));
}

/// Build a merged parameter map from two engine parameter objects, given a
/// morph position t.
///
/// Strategy for same-engine morphing: iterate union of parameter keys, lerp.
/// Strategy for cross-engine morphing: find shared base names (suffix after
/// prefix_), blend those; keep engine-A-only params when t < 0.5,
/// engine-B-only params when t >= 0.5.
inline juce::var morphEngineParams(const juce::String& engineNameA, const juce::var& paramsA,
                                   const juce::String& engineNameB, const juce::var& paramsB, float t)
{
    auto* result = new juce::DynamicObject();

    bool sameEngine = (engineNameA == engineNameB);

    auto* objA = (paramsA.isObject()) ? paramsA.getDynamicObject() : nullptr;
    auto* objB = (paramsB.isObject()) ? paramsB.getDynamicObject() : nullptr;

    if (sameEngine)
    {
        // ---- Same engine: lerp every parameter in the union of both sets ----
        // First pass: all params from A.
        if (objA != nullptr)
        {
            for (const auto& propA : objA->getProperties())
            {
                juce::String id = propA.name.toString();
                juce::var valA = propA.value;
                juce::var valB = (objB != nullptr) ? objB->getProperty(propA.name) : juce::var();

                if (valB.isVoid() || valB.isUndefined())
                    result->setProperty(propA.name, valA); // only in A
                else
                    result->setProperty(propA.name, lerpParam(id, valA, valB, t));
            }
        }
        // Second pass: params only in B.
        if (objB != nullptr)
        {
            for (const auto& propB : objB->getProperties())
            {
                if (result->hasProperty(propB.name))
                    continue; // already set
                result->setProperty(propB.name, propB.value);
            }
        }
    }
    else
    {
        // ---- Cross-engine morphing ----
        // 1. Build a lookup map from base name → (idA, valueA) for engine A.
        std::map<juce::String, std::pair<juce::Identifier, juce::var>> baseToA;
        if (objA != nullptr)
            for (const auto& p : objA->getProperties())
                baseToA[stripEnginePrefix(p.name.toString())] = {p.name, p.value};

        // 2. Build a lookup map from base name → (idB, valueB) for engine B.
        std::map<juce::String, std::pair<juce::Identifier, juce::var>> baseToB;
        if (objB != nullptr)
            for (const auto& p : objB->getProperties())
                baseToB[stripEnginePrefix(p.name.toString())] = {p.name, p.value};

        // 3. Determine which engine's prefix to use in the output.
        // We use the dominant engine's prefix (t < 0.5 → A, t >= 0.5 → B).
        bool useAPrefix = (t < 0.5f);

        // 4. Shared base names: lerp the values, write under dominant prefix.
        for (const auto& [baseName, pairA] : baseToA)
        {
            auto it = baseToB.find(baseName);
            if (it != baseToB.end())
            {
                // Both engines have this logical parameter.
                float valA = static_cast<float>(pairA.second);
                float valB = static_cast<float>(it->second.second);
                float lerped = valA + t * (valB - valA);

                juce::Identifier outId = useAPrefix ? pairA.first : it->second.first;
                if (isIntegerParam(baseName, pairA.second) || isIntegerParam(baseName, it->second.second))
                    result->setProperty(outId, (t < 0.5f) ? pairA.second : it->second.second);
                else
                    result->setProperty(outId, static_cast<double>(lerped));
            }
            else
            {
                // Only in A — include only when A is dominant.
                if (useAPrefix)
                    result->setProperty(pairA.first, pairA.second);
            }
        }

        // 5. Params only in B — include only when B is dominant.
        if (!useAPrefix && objB != nullptr)
        {
            for (const auto& [baseName, pairB] : baseToB)
            {
                if (baseToA.find(baseName) == baseToA.end())
                    result->setProperty(pairB.first, pairB.second);
            }
        }
    }

    return juce::var(result);
}

/// Interpolate the 6D DNA linearly.
inline PresetDNA lerpDNA(const PresetDNA& a, const PresetDNA& b, float t) noexcept
{
    auto lerp = [t](float x, float y) { return x + t * (y - x); };
    PresetDNA d;
    d.brightness = lerp(a.brightness, b.brightness);
    d.warmth = lerp(a.warmth, b.warmth);
    d.movement = lerp(a.movement, b.movement);
    d.density = lerp(a.density, b.density);
    d.space = lerp(a.space, b.space);
    d.aggression = lerp(a.aggression, b.aggression);
    return d;
}

/// Morph two presets at position t ∈ [0, 1].
/// t = 0 → pure A,  t = 1 → pure B.
inline PresetData morph(const PresetData& a, const PresetData& b, float t)
{
    t = clamp01(t);

    PresetData result;
    result.schemaVersion = a.schemaVersion;
    result.author = "XO_OX Designs";
    result.version = "1.0.0";
    result.dna = lerpDNA(a.dna, b.dna, t);

    // ---- Metadata: take from the dominant side ----
    if (t < 0.5f)
    {
        result.mood = a.mood;
        result.couplingIntensity = a.couplingIntensity;
        result.couplingPairs = a.couplingPairs;
        result.tempo = a.tempo;
        result.sequencerData = a.sequencerData;
        result.macroLabels = a.macroLabels;
        result.description = "Morphed preset — " + a.name + " x " + b.name;
        result.name = a.name + " x " + b.name;
        // Union tags, deduplicated.
        result.tags = a.tags;
        for (const auto& tag : b.tags)
            if (!result.tags.contains(tag))
                result.tags.add(tag);
    }
    else
    {
        result.mood = b.mood;
        result.couplingIntensity = b.couplingIntensity;
        result.couplingPairs = b.couplingPairs;
        result.tempo = b.tempo;
        result.sequencerData = b.sequencerData;
        result.macroLabels = b.macroLabels;
        result.description = "Morphed preset — " + a.name + " x " + b.name;
        result.name = a.name + " x " + b.name;
        result.tags = b.tags;
        for (const auto& tag : a.tags)
            if (!result.tags.contains(tag))
                result.tags.add(tag);
    }

    // ---- Engine slots ----
    // Each slot switches at t = 0.5 if engines differ.
    // We handle up to 3 engine slots.
    int numSlots = std::max(a.engines.size(), b.engines.size());
    numSlots = std::min(numSlots, 3);

    for (int slot = 0; slot < numSlots; ++slot)
    {
        bool hasA = slot < a.engines.size();
        bool hasB = slot < b.engines.size();

        juce::String engA = hasA ? a.engines[slot] : juce::String();
        juce::String engB = hasB ? b.engines[slot] : juce::String();

        // Determine which engine name is in the output for this slot.
        juce::String outEngine;
        if (hasA && hasB)
            outEngine = (t < 0.5f) ? engA : engB;
        else if (hasA)
            outEngine = (t < 0.5f) ? engA : juce::String();
        else
            outEngine = (t >= 0.5f) ? engB : juce::String();

        if (outEngine.isEmpty())
            continue;

        result.engines.add(outEngine);

        // ---- Parameter blending ----
        juce::var paramsA =
            hasA ? a.parametersByEngine.count(engA) ? a.parametersByEngine.at(engA) : juce::var() : juce::var();
        juce::var paramsB =
            hasB ? b.parametersByEngine.count(engB) ? b.parametersByEngine.at(engB) : juce::var() : juce::var();

        juce::String morphEngA = hasA ? engA : juce::String();
        juce::String morphEngB = hasB ? engB : juce::String();

        if (hasA && hasB)
        {
            auto blended = morphEngineParams(engA, paramsA, engB, paramsB, t);
            result.parametersByEngine[outEngine] = blended;
        }
        else if (hasA)
        {
            if (!paramsA.isVoid() && !paramsA.isUndefined())
                result.parametersByEngine[outEngine] = paramsA;
        }
        else
        {
            if (!paramsB.isVoid() && !paramsB.isUndefined())
                result.parametersByEngine[outEngine] = paramsB;
        }
    }

    return result;
}

//==============================================================================
// Section 5 — DNA-Based Breeding
//
// breed(parent1, parent2, targetDNA, rng) produces an offspring preset.
//
// Algorithm:
//   For each engine slot and each parameter key, inherit the value from
//   the parent whose DNA is "closer to the target" in the dimension that
//   parameter maps to most strongly.
//
//   If a parameter maps equally to multiple dimensions, use the average
//   distance across all mapped dimensions.
//
//   If a parameter has no DNA mapping, inherit from a coin flip (50/50
//   per parameter, seeded by the PRNG).
//
// DNA inheritance for the offspring:
//   Computed from the inherited parameters via estimateDNAFromParams so
//   the child's DNA is grounded in its actual values.  If neither parent
//   has a parameter, the offspring skips it.
//
// Engine selection: the engine whose aggregate parameter set is "closer"
// (by sum of per-param DNA fitness scores) is selected per slot.
//==============================================================================

/// Compute the total "fitness" of a parameter set for a target DNA.
/// Fitness = sum over all params of (weight * (1 - |estimatedDim - targetDim|))
inline float computeParamSetFitness(const juce::var& params, const PresetDNA& target)
{
    if (!params.isObject())
        return 0.0f;

    auto* obj = params.getDynamicObject();
    if (obj == nullptr)
        return 0.0f;

    static const DNADimension dims[] = {DNADimension::Brightness, DNADimension::Warmth, DNADimension::Movement,
                                        DNADimension::Density,    DNADimension::Space,  DNADimension::Aggression};

    float totalFitness = 0.0f;
    for (auto dim : dims)
    {
        float estimated = estimateDNAFromParams(params, dim);
        float target_v = 0.5f;
        switch (dim)
        {
        case DNADimension::Brightness:
            target_v = target.brightness;
            break;
        case DNADimension::Warmth:
            target_v = target.warmth;
            break;
        case DNADimension::Movement:
            target_v = target.movement;
            break;
        case DNADimension::Density:
            target_v = target.density;
            break;
        case DNADimension::Space:
            target_v = target.space;
            break;
        case DNADimension::Aggression:
            target_v = target.aggression;
            break;
        }
        totalFitness += 1.0f - std::abs(estimated - target_v);
    }
    return totalFitness;
}

/// For a single parameter, determine which parent to inherit from.
/// Returns 0 for parent A, 1 for parent B.
inline int selectParentForParam(const juce::String& paramId, const juce::var& valA, const juce::var& valB,
                                const PresetDNA& dnaA, const PresetDNA& dnaB, const PresetDNA& target, Xorshift64& rng)
{
    // Gather how strongly this param maps to each DNA dimension.
    static const std::pair<DNADimension, float PresetDNA::*> dimFields[] = {
        {DNADimension::Brightness, &PresetDNA::brightness}, {DNADimension::Warmth, &PresetDNA::warmth},
        {DNADimension::Movement, &PresetDNA::movement},     {DNADimension::Density, &PresetDNA::density},
        {DNADimension::Space, &PresetDNA::space},           {DNADimension::Aggression, &PresetDNA::aggression},
    };

    float scoreA = 0.0f, scoreB = 0.0f, totalW = 0.0f;

    for (const auto& [dim, field] : dimFields)
    {
        float w = paramDNAWeight(paramId, dim);
        if (w <= 0.0f)
            continue;

        float targetDim = target.*field;
        float distA = std::abs((dnaA.*field) - targetDim);
        float distB = std::abs((dnaB.*field) - targetDim);

        // The parent closer to the target in this dimension wins this weighted vote.
        scoreA += w * (1.0f - distA);
        scoreB += w * (1.0f - distB);
        totalW += w;
    }

    if (totalW <= 0.0f)
        return (rng.nextFloat() < 0.5f) ? 0 : 1; // no mapping → coin flip

    return (scoreA >= scoreB) ? 0 : 1;
}

/// Build the offspring parameter map from two parent engine param objects.
inline juce::var breedEngineParams(const juce::String& engineName, const juce::var& paramsA, const juce::var& paramsB,
                                   const PresetDNA& dnaA, const PresetDNA& dnaB, const PresetDNA& targetDNA,
                                   Xorshift64& rng)
{
    auto* result = new juce::DynamicObject();

    auto* objA = paramsA.isObject() ? paramsA.getDynamicObject() : nullptr;
    auto* objB = paramsB.isObject() ? paramsB.getDynamicObject() : nullptr;

    // Build the union of parameter keys from both parents.
    std::map<juce::String, std::pair<juce::var, juce::var>> unionParams;

    if (objA != nullptr)
        for (const auto& p : objA->getProperties())
            unionParams[p.name.toString()].first = p.value;

    if (objB != nullptr)
        for (const auto& p : objB->getProperties())
            unionParams[p.name.toString()].second = p.value;

    for (const auto& [id, vals] : unionParams)
    {
        const juce::var& vA = vals.first;
        const juce::var& vB = vals.second;

        juce::var chosen;

        bool hasA = !(vA.isVoid() || vA.isUndefined());
        bool hasB = !(vB.isVoid() || vB.isUndefined());

        if (hasA && hasB)
        {
            int parent = selectParentForParam(id, vA, vB, dnaA, dnaB, targetDNA, rng);
            chosen = (parent == 0) ? vA : vB;
        }
        else if (hasA)
        {
            chosen = vA;
        }
        else
        {
            chosen = vB;
        }

        result->setProperty(id, chosen);
    }

    return juce::var(result);
}

/// Breed two presets toward a target DNA.
/// Both parents must use at least one engine; cross-engine breeding is
/// supported for the same slot when both parents have an engine there.
inline PresetData breed(const PresetData& parentA, const PresetData& parentB, const PresetDNA& targetDNA,
                        Xorshift64& rng)
{
    PresetData offspring;
    offspring.schemaVersion = parentA.schemaVersion;
    offspring.author = "XO_OX Designs";
    offspring.version = "1.0.0";
    offspring.mood = (rng.nextFloat() < 0.5f) ? parentA.mood : parentB.mood;
    offspring.couplingIntensity = (rng.nextFloat() < 0.5f) ? parentA.couplingIntensity : parentB.couplingIntensity;
    offspring.tempo = (rng.nextFloat() < 0.5f) ? parentA.tempo : parentB.tempo;

    offspring.description = "Bred from " + parentA.name + " + " + parentB.name;
    offspring.name = parentA.name.substring(0, 10) + "/" + parentB.name.substring(0, 10);

    // Tags: union.
    offspring.tags = parentA.tags;
    for (const auto& tag : parentB.tags)
        if (!offspring.tags.contains(tag))
            offspring.tags.add(tag);

    // Macros: per-slot coin flip from dominant parent.
    offspring.macroLabels = parentA.macroLabels; // start from A
    for (int m = 0; m < 4 && m < parentB.macroLabels.size(); ++m)
        if (rng.nextFloat() < 0.5f && m < offspring.macroLabels.size())
            offspring.macroLabels.set(m, parentB.macroLabels[m]);

    // Coupling: pick from parent with higher fitness score.
    float fitnessA = 0.0f, fitnessB = 0.0f;
    for (const auto& [eng, params] : parentA.parametersByEngine)
        fitnessA += computeParamSetFitness(params, targetDNA);
    for (const auto& [eng, params] : parentB.parametersByEngine)
        fitnessB += computeParamSetFitness(params, targetDNA);

    offspring.couplingPairs = (fitnessA >= fitnessB) ? parentA.couplingPairs : parentB.couplingPairs;

    // ---- Engine slots ----
    int numSlots = std::max(parentA.engines.size(), parentB.engines.size());
    numSlots = std::min(numSlots, 3);

    for (int slot = 0; slot < numSlots; ++slot)
    {
        bool hasA = slot < parentA.engines.size();
        bool hasB = slot < parentB.engines.size();

        juce::String engA = hasA ? parentA.engines[slot] : juce::String();
        juce::String engB = hasB ? parentB.engines[slot] : juce::String();

        if (!hasA && !hasB)
            continue;

        if (!hasA)
        {
            // Only B has an engine here.
            offspring.engines.add(engB);
            if (parentB.parametersByEngine.count(engB))
                offspring.parametersByEngine[engB] = parentB.parametersByEngine.at(engB);
            continue;
        }
        if (!hasB)
        {
            // Only A has an engine here.
            offspring.engines.add(engA);
            if (parentA.parametersByEngine.count(engA))
                offspring.parametersByEngine[engA] = parentA.parametersByEngine.at(engA);
            continue;
        }

        // Both slots are populated.  Choose the dominant engine by fitness.
        juce::var pA = parentA.parametersByEngine.count(engA) ? parentA.parametersByEngine.at(engA) : juce::var();
        juce::var pB = parentB.parametersByEngine.count(engB) ? parentB.parametersByEngine.at(engB) : juce::var();

        float fA = computeParamSetFitness(pA, targetDNA);
        float fB = computeParamSetFitness(pB, targetDNA);

        juce::String outEngine = (fA >= fB) ? engA : engB;
        offspring.engines.add(outEngine);

        // Breed parameters within the chosen engine (same name → use its params
        // from both parents if they happen to share the same engine type; otherwise
        // fall back to the dominant engine's params with per-param breeding).
        if (engA == engB)
        {
            offspring.parametersByEngine[outEngine] =
                breedEngineParams(outEngine, pA, pB, parentA.dna, parentB.dna, targetDNA, rng);
        }
        else
        {
            // Different engine types in this slot: the dominant engine wins but
            // we still run breedEngineParams to carry over semantically matching
            // params from the non-dominant parent (via stripEnginePrefix matching).
            // Re-key the non-dominant params under the dominant prefix.
            // Strategy: remap non-dominant params to dominant prefix where base names match.
            juce::var& domParams = (fA >= fB) ? pA : pB;
            juce::var& altParams = (fA >= fB) ? pB : pA;
            juce::String domEng = outEngine;
            juce::String altEng = (fA >= fB) ? engB : engA;
            PresetDNA domDNA = (fA >= fB) ? parentA.dna : parentB.dna;
            PresetDNA altDNA = (fA >= fB) ? parentB.dna : parentA.dna;

            // Build a remapped copy of altParams using dominant engine prefix.
            // frozenPrefixForEngine() returns prefix with trailing underscore for
            // all engines (normalized in #126), so direct use is safe.
            juce::String domPrefix = frozenPrefixForEngine(domEng);
            juce::String altPrefix = frozenPrefixForEngine(altEng);
            auto* remapped = new juce::DynamicObject();

            if (altParams.isObject())
            {
                auto* altObj = altParams.getDynamicObject();
                if (altObj != nullptr)
                {
                    for (const auto& prop : altObj->getProperties())
                    {
                        juce::String origId = prop.name.toString();
                        juce::String baseName = origId.startsWith(altPrefix) ? origId.substring(altPrefix.length())
                                                                             : stripEnginePrefix(origId);
                        juce::String newId = domPrefix + baseName;
                        remapped->setProperty(newId, prop.value);
                    }
                }
            }

            offspring.parametersByEngine[outEngine] =
                breedEngineParams(outEngine, domParams, juce::var(remapped), domDNA, altDNA, targetDNA, rng);
        }
    }

    // ---- Offspring DNA: estimated from inherited parameters ----
    // Use weighted average of estimateDNAFromParams across all engine param sets.
    {
        float bSum = 0, wSum = 0, mSum = 0, dSum = 0, sSum = 0, aSum = 0;
        int count = 0;
        for (const auto& [eng, params] : offspring.parametersByEngine)
        {
            bSum += estimateDNAFromParams(params, DNADimension::Brightness);
            wSum += estimateDNAFromParams(params, DNADimension::Warmth);
            mSum += estimateDNAFromParams(params, DNADimension::Movement);
            dSum += estimateDNAFromParams(params, DNADimension::Density);
            sSum += estimateDNAFromParams(params, DNADimension::Space);
            aSum += estimateDNAFromParams(params, DNADimension::Aggression);
            ++count;
        }
        float n = (count > 0) ? static_cast<float>(count) : 1.0f;
        offspring.dna.brightness = bSum / n;
        offspring.dna.warmth = wSum / n;
        offspring.dna.movement = mSum / n;
        offspring.dna.density = dSum / n;
        offspring.dna.space = sSum / n;
        offspring.dna.aggression = aSum / n;
    }

    return offspring;
}

//==============================================================================
// Section 6 — Smart Randomization
//
// randomize(source, maxDNAJump, rng) produces a variant of `source` that:
//   1. Keeps the same engine(s) — never changes engine identity.
//   2. Keeps the same mood.
//   3. Perturbs each float parameter by a Gaussian-like amount proportional
//      to the maxDNAJump budget scaled by the parameter's DNA mapping weight.
//      Parameters with no DNA mapping are perturbed with a smaller budget.
//   4. Does NOT perturb integer / enum parameters (mode selectors, etc.).
//   5. Clamps all values to [0, 1] for normalized params.  Params that look
//      like they operate in larger ranges (values > 2.0 in the source) are
//      perturbed ± (maxDNAJump * rangeEstimate) instead.
//   6. The DNA is recomputed from the perturbed parameters.
//
// maxDNAJump: max perturbation magnitude per DNA dimension, in [0, 1].
//   Typical: 0.1 (subtle), 0.25 (moderate), 0.5 (wild).
//==============================================================================

/// Estimate the natural range of a parameter from its value in the source
/// preset.  Values ≤ 2 are assumed to be normalized (range ~ 1).  Larger
/// values imply a wider range; we use the value itself as a rough scale.
inline float estimateParamRange(float val) noexcept
{
    if (val < 0.0f)
        val = -val;
    if (val <= 2.0f)
        return 1.0f;
    if (val <= 20.0f)
        return val;
    if (val <= 200.0f)
        return val * 0.5f;
    return val * 0.25f;
}

/// Gaussian-like sample using the sum of two uniform[-0.5, 0.5] variables
/// (triangular distribution — no tails, 0-mean, σ ≈ 0.408 for ±1 inputs).
/// Much cheaper than computing erfinv; good enough for parameter jitter.
inline float triangularSample(Xorshift64& rng) noexcept
{
    float u1 = rng.nextFloat() - 0.5f;
    float u2 = rng.nextFloat() - 0.5f;
    return u1 + u2; // range ±1, mean 0, peak at 0
}

/// Randomize a preset while keeping engine, mood, and integer params fixed.
/// maxDNAJump: perturbation budget per DNA dimension (0 = no change, 1 = anything).
inline PresetData randomize(const PresetData& source, float maxDNAJump, Xorshift64& rng)
{
    maxDNAJump = clamp01(maxDNAJump);

    PresetData result = source; // copy: preserves engine, mood, coupling, meta
    result.name = source.name + " (Randomized)";
    result.description = "Smart randomization of " + source.name;

    for (auto& [engineName, params] : result.parametersByEngine)
    {
        if (!params.isObject())
            continue;

        auto* obj = params.getDynamicObject();
        if (obj == nullptr)
            continue;

        // We need to modify properties of the DynamicObject in-place.
        // Collect all (id, val) pairs, perturb, then set.
        std::vector<std::pair<juce::String, juce::var>> newProps;
        for (const auto& prop : obj->getProperties())
        {
            juce::String id = prop.name.toString();
            juce::var val = prop.value;

            // Skip non-numeric and integer params.
            if (!val.isDouble() && !val.isInt())
            {
                newProps.emplace_back(id, val);
                continue;
            }
            if (isIntegerParam(id, val))
            {
                newProps.emplace_back(id, val);
                continue;
            }

            float fval = static_cast<float>(val);

            // Determine how much budget to spend on this param.
            // Sum weights across all DNA dimensions.
            float totalW = 0.0f;
            for (const auto& m : getDNAMappings())
                if (id.containsIgnoreCase(m.fragment))
                    totalW += m.weight;

            // Scale budget: params with strong DNA mapping get more perturbation.
            // Unmapped params get 10% of max budget.
            float budget = (totalW > 0.0f) ? maxDNAJump * std::min(totalW, 2.0f) / 2.0f : maxDNAJump * 0.1f;

            float range = estimateParamRange(fval);
            float perturbation = triangularSample(rng) * budget * range;
            float newVal = fval + perturbation;

            // Clamp to range based on whether it's a normalized param.
            if (range <= 1.0f)
                newVal = clamp01(newVal);
            else
                newVal = clampRange(newVal, 0.0f, range * 2.0f);

            newProps.emplace_back(id, static_cast<double>(newVal));
        }

        // Rebuild the DynamicObject with perturbed values.
        auto* newObj = new juce::DynamicObject();
        for (const auto& [id, val] : newProps)
            newObj->setProperty(id, val);

        params = juce::var(newObj);
    }

    // Recompute DNA from perturbed params.
    {
        float bSum = 0, wSum = 0, mSum = 0, dSum = 0, sSum = 0, aSum = 0;
        int count = 0;
        for (const auto& [eng, params] : result.parametersByEngine)
        {
            bSum += estimateDNAFromParams(params, DNADimension::Brightness);
            wSum += estimateDNAFromParams(params, DNADimension::Warmth);
            mSum += estimateDNAFromParams(params, DNADimension::Movement);
            dSum += estimateDNAFromParams(params, DNADimension::Density);
            sSum += estimateDNAFromParams(params, DNADimension::Space);
            aSum += estimateDNAFromParams(params, DNADimension::Aggression);
            ++count;
        }
        float n = (count > 0) ? static_cast<float>(count) : 1.0f;
        result.dna.brightness = bSum / n;
        result.dna.warmth = wSum / n;
        result.dna.movement = mSum / n;
        result.dna.density = dSum / n;
        result.dna.space = sSum / n;
        result.dna.aggression = aSum / n;
    }

    return result;
}

//==============================================================================
// Section 7 — DNA Distance + Nearest Search
//
// These are standalone functions that mirror PresetManager's DNA helpers but
// operate over an arbitrary collection rather than the live library.
//==============================================================================

/// Euclidean distance in 6D DNA space.
inline float dnaDistance(const PresetDNA& a, const PresetDNA& b) noexcept
{
    float db = a.brightness - b.brightness;
    float dw = a.warmth - b.warmth;
    float dm = a.movement - b.movement;
    float dd = a.density - b.density;
    float ds = a.space - b.space;
    float da = a.aggression - b.aggression;
    return std::sqrt(db * db + dw * dw + dm * dm + dd * dd + ds * ds + da * da);
}

/// Manhattan (L1) distance in 6D DNA space.
inline float dnaDistanceManhattan(const PresetDNA& a, const PresetDNA& b) noexcept
{
    return std::abs(a.brightness - b.brightness) + std::abs(a.warmth - b.warmth) + std::abs(a.movement - b.movement) +
           std::abs(a.density - b.density) + std::abs(a.space - b.space) + std::abs(a.aggression - b.aggression);
}

/// Find the N presets nearest to targetDNA from an arbitrary collection.
/// Returns at most `count` results, sorted ascending by distance.
inline std::vector<std::pair<float, const PresetData*>> findNearest(const std::vector<PresetData>& library,
                                                                    const PresetDNA& targetDNA, int count)
{
    struct Ranked
    {
        float dist;
        size_t idx;
    };

    std::vector<Ranked> ranked;
    ranked.reserve(library.size());

    for (size_t i = 0; i < library.size(); ++i)
        ranked.push_back({dnaDistance(library[i].dna, targetDNA), i});

    std::sort(ranked.begin(), ranked.end(), [](const Ranked& x, const Ranked& y) { return x.dist < y.dist; });

    std::vector<std::pair<float, const PresetData*>> results;
    int n = std::min(count, static_cast<int>(ranked.size()));
    for (int i = 0; i < n; ++i)
        results.emplace_back(ranked[static_cast<size_t>(i)].dist, &library[ranked[static_cast<size_t>(i)].idx]);

    return results;
}

/// Find the N presets from `library` whose engine list contains `engineFilter`.
/// Useful for "stay in the same engine" nearest search.
inline std::vector<std::pair<float, const PresetData*>> findNearestForEngine(const std::vector<PresetData>& library,
                                                                             const PresetDNA& targetDNA,
                                                                             const juce::String& engineFilter,
                                                                             int count)
{
    std::vector<PresetData> filtered;
    for (const auto& p : library)
        if (p.engines.contains(engineFilter))
            filtered.push_back(p);

    // Build nearest results pointing into `library` (not filtered copy).
    // We do a two-pass: filter, then rank.
    struct Ranked
    {
        float dist;
        const PresetData* ptr;
    };

    std::vector<Ranked> ranked;
    for (const auto& p : library)
    {
        if (!p.engines.contains(engineFilter))
            continue;
        ranked.push_back({dnaDistance(p.dna, targetDNA), &p});
    }

    std::sort(ranked.begin(), ranked.end(), [](const Ranked& x, const Ranked& y) { return x.dist < y.dist; });

    std::vector<std::pair<float, const PresetData*>> results;
    int n = std::min(count, static_cast<int>(ranked.size()));
    for (int i = 0; i < n; ++i)
        results.emplace_back(ranked[static_cast<size_t>(i)].dist, ranked[static_cast<size_t>(i)].ptr);
    return results;
}

} // namespace morpher

//==============================================================================
// Section 8 — PresetMorpher — Public API
//
// Thin façade that bundles a PRNG and exposes a clean interface.
// All operations are stateless except for the PRNG state — safe to call
// from any UI thread.  Not thread-safe for concurrent calls.
//==============================================================================

class PresetMorpher
{
public:
    //--------------------------------------------------------------------------
    // Construction
    //--------------------------------------------------------------------------

    /// Default constructor — seeds PRNG from the system high-resolution clock.
    PresetMorpher() { rng.seedFromTime(); }

    /// Constructor with explicit seed — for reproducible results in tests.
    explicit PresetMorpher(uint64_t seed) : rng(seed) {}

    //--------------------------------------------------------------------------
    // 1 — Interpolation
    //--------------------------------------------------------------------------

    /// Morph between two presets.
    /// t = 0 → pure a,  t = 1 → pure b.
    /// Float params are linearly interpolated.
    /// Integer / enum params switch at t = 0.5.
    /// Engine identity switches at t = 0.5 when engines differ.
    PresetData morph(const PresetData& a, const PresetData& b, float t) const { return morpher::morph(a, b, t); }

    /// Produce a sequence of `steps` evenly spaced morphs between a and b.
    /// Includes both endpoints.  steps must be ≥ 2.
    std::vector<PresetData> morphSequence(const PresetData& a, const PresetData& b, int steps) const
    {
        steps = std::max(steps, 2);
        std::vector<PresetData> seq;
        seq.reserve(static_cast<size_t>(steps));

        for (int i = 0; i < steps; ++i)
        {
            float t = static_cast<float>(i) / static_cast<float>(steps - 1);
            seq.push_back(morpher::morph(a, b, t));
        }
        return seq;
    }

    //--------------------------------------------------------------------------
    // 2 — Breeding
    //--------------------------------------------------------------------------

    /// Breed two presets toward a target DNA.
    /// Each parameter inherits from the parent whose DNA is closer to the target
    /// in the dimension that parameter most strongly affects.
    PresetData breed(const PresetData& parentA, const PresetData& parentB, const PresetDNA& targetDNA)
    {
        return morpher::breed(parentA, parentB, targetDNA, rng);
    }

    /// Convenience overload: breed toward the midpoint of both parents' DNAs.
    PresetData breed(const PresetData& parentA, const PresetData& parentB)
    {
        PresetDNA midpoint = morpher::lerpDNA(parentA.dna, parentB.dna, 0.5f);
        return morpher::breed(parentA, parentB, midpoint, rng);
    }

    /// Produce multiple offspring from two parents toward the same target.
    std::vector<PresetData> breedPopulation(const PresetData& parentA, const PresetData& parentB,
                                            const PresetDNA& targetDNA, int count)
    {
        count = std::max(count, 1);
        std::vector<PresetData> population;
        population.reserve(static_cast<size_t>(count));

        for (int i = 0; i < count; ++i)
            population.push_back(morpher::breed(parentA, parentB, targetDNA, rng));

        return population;
    }

    //--------------------------------------------------------------------------
    // 3 — Smart Randomization
    //--------------------------------------------------------------------------

    /// Randomize a preset with a given perturbation budget.
    /// maxDNAJump: 0 (no change) → 1 (maximum jump).
    ///   Recommended: 0.1 (subtle), 0.25 (moderate), 0.5 (wild).
    /// Engine identity and integer (mode/type/waveform) params are preserved.
    PresetData randomize(const PresetData& source, float maxDNAJump = 0.15f)
    {
        return morpher::randomize(source, maxDNAJump, rng);
    }

    /// Produce `count` unique randomized variants of `source`.
    std::vector<PresetData> randomizeMultiple(const PresetData& source, float maxDNAJump, int count)
    {
        count = std::max(count, 1);
        std::vector<PresetData> variants;
        variants.reserve(static_cast<size_t>(count));

        for (int i = 0; i < count; ++i)
        {
            auto v = morpher::randomize(source, maxDNAJump, rng);
            v.name = source.name + " (Variant " + juce::String(i + 1) + ")";
            variants.push_back(std::move(v));
        }

        return variants;
    }

    //--------------------------------------------------------------------------
    // 4 — DNA Distance + Nearest Search
    //--------------------------------------------------------------------------

    /// Euclidean distance in 6D DNA space.  Range: [0, √6] ≈ [0, 2.449].
    static float dnaDistance(const PresetDNA& a, const PresetDNA& b) { return morpher::dnaDistance(a, b); }

    /// Manhattan distance in 6D DNA space.  Range: [0, 6].
    static float dnaDistanceManhattan(const PresetDNA& a, const PresetDNA& b)
    {
        return morpher::dnaDistanceManhattan(a, b);
    }

    /// Find the N closest presets to targetDNA in the provided library.
    /// Returns (distance, pointer) pairs sorted ascending by distance.
    /// Pointers remain valid only as long as `library` is not modified.
    static std::vector<std::pair<float, const PresetData*>> findNearest(const std::vector<PresetData>& library,
                                                                        const PresetDNA& targetDNA, int count = 5)
    {
        return morpher::findNearest(library, targetDNA, count);
    }

    /// Find the N closest presets that use a specific engine.
    static std::vector<std::pair<float, const PresetData*>> findNearestForEngine(const std::vector<PresetData>& library,
                                                                                 const PresetDNA& targetDNA,
                                                                                 const juce::String& engineName,
                                                                                 int count = 5)
    {
        return morpher::findNearestForEngine(library, targetDNA, engineName, count);
    }

    /// Estimate a preset's 6D DNA from its parameter values alone.
    /// Useful for presets that were programmatically built and don't yet have
    /// a hand-authored DNA block.
    static PresetDNA estimateDNA(const PresetData& preset)
    {
        PresetDNA dna;
        float bSum = 0, wSum = 0, mSum = 0, dSum = 0, sSum = 0, aSum = 0;
        int count = 0;

        for (const auto& [eng, params] : preset.parametersByEngine)
        {
            bSum += morpher::estimateDNAFromParams(params, morpher::DNADimension::Brightness);
            wSum += morpher::estimateDNAFromParams(params, morpher::DNADimension::Warmth);
            mSum += morpher::estimateDNAFromParams(params, morpher::DNADimension::Movement);
            dSum += morpher::estimateDNAFromParams(params, morpher::DNADimension::Density);
            sSum += morpher::estimateDNAFromParams(params, morpher::DNADimension::Space);
            aSum += morpher::estimateDNAFromParams(params, morpher::DNADimension::Aggression);
            ++count;
        }

        float n = (count > 0) ? static_cast<float>(count) : 1.0f;
        dna.brightness = bSum / n;
        dna.warmth = wSum / n;
        dna.movement = mSum / n;
        dna.density = dSum / n;
        dna.space = sSum / n;
        dna.aggression = aSum / n;

        return dna;
    }

    //--------------------------------------------------------------------------
    // Utility: reseed the PRNG (useful after loading a saved project state).
    //--------------------------------------------------------------------------

    void reseedFromTime() { rng.seedFromTime(); }
    void reseed(uint64_t seed) { rng = morpher::Xorshift64(seed); }

private:
    morpher::Xorshift64 rng;
};

} // namespace xoceanus
