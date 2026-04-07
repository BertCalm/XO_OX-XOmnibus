// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_core/juce_core.h>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include "EngineRegistry.h"
// Issue #899: embedded Init preset compiled into binary data by XOceanusInitPreset target.
// HEADER_NAME "InitPresetData.h" avoids collision with XOceanusFont's BinaryData.h.
#include "InitPresetData.h"

namespace xoceanus
{

//==============================================================================
// getValidEngineNames() — runtime accessor that derives the valid engine name
// list from EngineRegistry so it stays in sync automatically as engines are
// registered. Legacy aliases are added for backward preset compatibility.
// Prefer this over validEngineNames for runtime validation. (#683)
inline juce::StringArray getValidEngineNames()
{
    const auto& ids = EngineRegistry::instance().getRegisteredIds();
    juce::StringArray names;
    names.ensureStorageAllocated(static_cast<int>(ids.size()) + 32);
    for (const auto& id : ids)
        names.add(juce::String(id));

    // Legacy aliases resolved by resolveEngineAlias() before validation.
    static const char* const kLegacyAliases[] = {
        "Snap", "Morph", "Dub", "Drift", "Bob", "Fat", "Bite",
        "XOddCouple", "XOverdub", "XOdyssey", "XOblong", "XOblongBob",
        "XObese", "XOnset", "XOrbital", "XOrganon", "XOuroboros", "XOpal",
        "XOpossum", "XOverbite", "XObsidian", "XOrigami", "XOracle",
        "XObscura", "XOceanic", "XOptic", "XOblique", "XOverworld",
        "XOrca", "XOctopus", "XOverlap", "XOutwit",
    };
    for (const char* alias : kLegacyAliases)
        names.addIfNotAlreadyThere(alias);

    return names;
}

//==============================================================================
// validEngineNames — compile-time list kept for backward compatibility with
// call sites that need a const reference. Prefer getValidEngineNames() for
// any runtime engine name validation.
inline const juce::StringArray validEngineNames{
    // All engine IDs start with O (brand convention)
    "OddfeliX", "OddOscar", // Mascots: feliX the neon tetra, Oscar the axolotl
    "Overdub", "Odyssey", "Oblong", "Obese", "Onset", "Overworld", "Opal", "Orbital", "Organon", "Ouroboros",
    "Obsidian", "Overbite", "Origami", "Oracle", "Obscura", "Oceanic", "Ocelot", "Osprey", "Osteria", "Owlfish", "Ohm",
    "Orphica", "Obbligato", "Ottoni", "Ole", "Optic", "Oblique", "Ombre", "Orca", "Octopus",
    // Phase 4 engines
    "Overlap", "Outwit",
    // V1 concept engines
    "OpenSky", "Ostinato", "OceanDeep", "Ouie",
    // Flagship
    "Obrix",
    // V2 theorem engines
    "Orbweave", "Overtone", "Organism",
    // Singularity engines
    "Oxbow", "Oware",
    // Kuramoto vocal synthesis
    "Opera",
    // Psychology-driven boom bap drums
    "Offering",
    // Chef Quad Collection
    "Oto", "Octave", "Oleg", "Otis",
    // KITCHEN Quad Collection
    "Oven", "Ochre", "Obelisk", "Opaline",
    // CELLAR Quad Collection
    "Ogre", "Olate", "Oaken", "Omega",
    // GARDEN Quad Collection
    "Orchard", "Overgrow", "Osier", "Oxalis",
    // BROTH Quad Collection
    "Overwash", "Overworn", "Overflow", "Overcast",
    // FUSION Quad Collection
    "Okeanos", "Oddfellow", "Onkolo", "Opcode",
    // Membrane Collection
    "Osmosis",
    // Love Triangle Circuit Synth
    "Oxytocin",
    // Panoramic visionary synth
    "Outlook",
    // Dual Engine Integration
    "Oasis", "Outflow",
    // Cellular Automata Oscillator
    "Obiont",
    // Age-based corrosion synthesis
    "Oxidize",
    // Legacy aliases (kept for backward preset compatibility)
    "XOddCouple", "XOverdub", "XOdyssey", "XOblong", "XOblongBob", "XObese", "XOnset", "XOrbital", "XOrganon",
    "XOuroboros", "XOpal", "XOpossum", "XOverbite", "XObsidian", "XOrigami", "XOracle", "XObscura", "XOceanic",
    "XOptic", "XOblique", "XOverworld", "XOrca", "XOctopus", "Snap", "Morph", "Dub", "Drift", "Bob", "Fat", "Bite"};

// REMOVED ALIASES:
// "Oasis" → "Okeanos" was removed (2026-03-31).
// "Oasis" now refers to a new engine (FUSION EP tier).
// Old presets referencing the original Oasis concept should use "Okeanos" directly.

// Resolve legacy engine name aliases to current canonical IDs.
// Returns the input unchanged if it's already a current ID or unrecognized.
inline juce::String resolveEngineAlias(const juce::String& name)
{
    static const std::map<juce::String, juce::String> aliases{
        {"Snap", "OddfeliX"},
        {"Morph", "OddOscar"},
        {"Dub", "Overdub"},
        {"Drift", "Odyssey"},
        {"Bob", "Oblong"},
        {"Fat", "Obese"},
        {"Bite", "Overbite"},
        {"XOddCouple", "OddfeliX"}, // v0 name for Snap/feliX
        {"XOverdub", "Overdub"},
        {"XOdyssey", "Odyssey"},
        {"XOblong", "Oblong"},
        {"XOblongBob", "Oblong"},
        {"XObese", "Obese"},
        {"XOnset", "Onset"},
        {"XOrbital", "Orbital"},
        {"XOrganon", "Organon"},
        {"XOuroboros", "Ouroboros"},
        {"XOpal", "Opal"},
        {"XOpossum", "Overbite"},
        {"XOverbite", "Overbite"},
        {"XObsidian", "Obsidian"},
        {"XOrigami", "Origami"},
        {"XOracle", "Oracle"},
        {"XObscura", "Obscura"},
        {"XOceanic", "Oceanic"},
        {"XOptic", "Optic"},
        {"XOblique", "Oblique"},
        {"XOverworld", "Overworld"},
        {"XOrca", "Orca"},
        {"XOctopus", "Octopus"},
        // Phase 4 engine aliases
        {"XOverlap", "Overlap"},
        {"XOutwit", "Outwit"},
    };
    auto it = aliases.find(name);
    return (it != aliases.end()) ? it->second : name;
}

// Frozen parameter prefix for each canonical engine ID.
// These NEVER change — parameter IDs are stable across releases.
// All prefixes include the trailing underscore (e.g. "snap_", "oven_").
// Callers can concatenate directly: prefix + paramName → "snap_filterCutoff".
inline juce::String frozenPrefixForEngine(const juce::String& engineId)
{
    static const std::map<juce::String, juce::String> prefixes{
        {"OddfeliX", "snap_"},
        {"OddOscar", "morph_"},
        {"Overdub", "dub_"},
        {"Odyssey", "drift_"},
        {"Oblong", "bob_"},
        {"Obese", "fat_"},
        {"Overbite", "poss_"},
        {"Onset", "perc_"},
        {"Overworld", "ow_"},
        {"Opal", "opal_"},
        {"Orbital", "orb_"},
        {"Organon", "organon_"},
        {"Ouroboros", "ouro_"},
        {"Obsidian", "obsidian_"},
        {"Origami", "origami_"},
        {"Oracle", "oracle_"},
        {"Obscura", "obscura_"},
        {"Oceanic", "ocean_"},
        {"Optic", "optic_"},
        {"Oblique", "oblq_"},
        {"Ocelot", "ocelot_"},
        {"Osprey", "osprey_"},
        {"Osteria", "osteria_"},
        {"Owlfish", "owl_"},
        {"Ohm", "ohm_"},
        {"Orphica", "orph_"},
        {"Obbligato", "obbl_"},
        {"Ottoni", "otto_"},
        {"Ole", "ole_"},
        {"Ombre", "ombre_"},
        {"Orca", "orca_"},
        {"Octopus", "octo_"},
        {"Overlap", "olap_"},
        {"Outwit", "owit_"},
        // V1 Concept Engines
        {"OpenSky", "sky_"},
        {"Ostinato", "osti_"},
        {"OceanDeep", "deep_"},
        {"Ouie", "ouie_"},
        // Flagship
        {"Obrix", "obrix_"},
        // V2 Theorem Engines
        {"Orbweave", "weave_"},
        {"Overtone", "over_"},
        {"Organism", "org_"},
        // Singularity Engines
        {"Oxbow", "oxb_"},
        {"Oware", "owr_"},
        // Kuramoto Vocal Synthesis
        {"Opera", "opera_"},
        // Psychology-Driven Boom Bap Drums
        {"Offering", "ofr_"},
        // Chef Quad Collection
        {"Oto", "oto_"},
        {"Octave", "oct_"},
        {"Oleg", "oleg_"},
        {"Otis", "otis_"},
        // KITCHEN Quad Collection
        {"Oven", "oven_"},
        {"Ochre", "ochre_"},
        {"Obelisk", "obel_"},
        {"Opaline", "opal2_"},
        // CELLAR Quad Collection
        {"Ogre", "ogre_"},
        {"Olate", "olate_"},
        {"Oaken", "oaken_"},
        {"Omega", "omega_"},
        // GARDEN Quad Collection
        {"Orchard", "orch_"},
        {"Overgrow", "grow_"},
        {"Osier", "osier_"},
        {"Oxalis", "oxal_"},
        // BROTH Quad Collection
        {"Overwash", "wash_"},
        {"Overworn", "worn_"},
        {"Overflow", "flow_"},
        {"Overcast", "cast_"},
        // FUSION Quad Collection
        {"Okeanos", "okan_"},
        {"Oddfellow", "oddf_"},
        {"Onkolo", "onko_"},
        {"Opcode", "opco_"},
        // Membrane Collection
        {"Osmosis", "osmo_"},
        // Love Triangle Circuit Synth
        {"Oxytocin", "oxy_"},
        // Panoramic Visionary Synth
        {"Outlook", "look_"},
        // Dual Engine Integration
        {"Oasis", "oas_"},
        {"Outflow", "out_"},
        // Cellular Automata Oscillator
        {"Obiont", "obnt_"},
        // Age-based corrosion synthesis
        {"Oxidize", "oxidize_"},
    };
    auto it = prefixes.find(engineId);
    return (it != prefixes.end()) ? it->second : juce::String();
}

// Resolve legacy per-parameter aliases for OddfeliX (Snap) engine.
// These were renamed or removed when the engine became purely percussive
// (no ADSR sustain/release stage). Returns the canonical param ID, or an
// empty String if the param has been removed and should be dropped.
inline juce::String resolveSnapParamAlias(const juce::String& paramId)
{
    // Renamed params: old ID -> canonical ID
    static const std::map<juce::String, juce::String> renamed{
        {"snap_resonance", "snap_filterReso"},
        {"snap_filterEnvAmt", "snap_filterEnvDepth"},
        {"snap_filterEnv", "snap_filterEnvDepth"},
        {"snap_snapAmount", "snap_snap"},
    };
    // Removed params: had no equivalent in the percussive engine redesign
    static const std::set<juce::String> removed{
        "snap_attack",     "snap_sustain",    "snap_release",  "snap_ampAttack", "snap_ampDecay",
        "snap_ampRelease", "snap_ampSustain", "snap_oscShape", "snap_oscTune",   "snap_snapTone",
    };

    if (removed.count(paramId))
        return {}; // empty = drop this param

    auto it = renamed.find(paramId);
    return (it != renamed.end()) ? it->second : paramId;
}

// Resolve legacy per-parameter aliases for Overbite (Bite) engine.
// The engine accumulated 4 generations of parameter naming across its life as a
// standalone plugin (XOppossum) and XOceanus integration. Returns the canonical
// param ID (poss_-prefixed, matching BiteEngine.h frozen APVTS IDs), or an empty
// String if the param has no canonical equivalent and should be dropped silently.
inline juce::String resolveBiteParamAlias(const juce::String& paramId)
{
    // Gen 3 → Gen 4 renames: abbreviated names → full APVTS IDs
    static const std::map<juce::String, juce::String> renamed{
        {"poss_fur", "poss_furAmount"},
        {"poss_glide", "poss_glideTime"},
        {"poss_gnash", "poss_gnashAmount"},
        {"poss_gnashAmt", "poss_gnashAmount"},
        {"poss_chew", "poss_chewAmount"},
        {"poss_masterVolume", "poss_level"},
        {"poss_outputLevel", "poss_level"},
        {"poss_outputPan", "poss_pan"},
        {"poss_oscAWave", "poss_oscAWaveform"},
        {"poss_oscBWave", "poss_oscBWaveform"},
        {"poss_subOct", "poss_subOctave"},
        {"poss_filterEnvAmt", "poss_filterEnvAmount"},
        {"poss_filtEnvAttack", "poss_filterAttack"},
        {"poss_filtEnvDecay", "poss_filterDecay"},
        {"poss_filtEnvSustain", "poss_filterSustain"},
        {"poss_filtEnvRelease", "poss_filterRelease"},
        {"poss_filter_cutoff", "poss_filterCutoff"},
        {"poss_amp_sustain", "poss_ampSustain"},
        {"poss_resonance", "poss_filterReso"},
        {"poss_oscInteractionAmt", "poss_oscInteractAmount"},
        {"poss_oscInteractionMode", "poss_oscInteractMode"},
        {"poss_drive", "poss_driveAmount"},
        {"poss_weightAmt", "poss_weightLevel"},
        // Numbered macros → named macros (positional: 1=Belly, 2=Bite, 3=Scurry, 4=Trash)
        {"poss_macro1", "poss_macroBelly"},
        {"poss_macro2", "poss_macroBite"},
        {"poss_macro3", "poss_macroScurry"},
        {"poss_macro4", "poss_macroTrash"},
    };
    // Params with no canonical equivalent — removed or replaced by new architecture.
    // Gen 2 concept params, old per-engine coupling, ambiguous LFO/env params, etc.
    static const std::set<juce::String> removed{
        // Gen 2 concept params (standalone prototype era)
        "poss_aggression",
        "poss_brightness",
        "poss_warmth",
        "poss_couplingBus",
        "poss_couplingLevel",
        "poss_macroCoupling",
        "poss_attack",
        "poss_release",
        // Gen 2/3 params with no canonical equivalent
        "poss_biteDepth",
        "poss_fangAttack",
        "poss_snapRelease",
        "poss_couplingIn",
        "poss_couplingOut",
        "poss_coupling",
        "poss_character",
        "poss_bite",
        "poss_delayMix",
        "poss_reverbMix",
        "poss_space",
        "poss_movement",
        "poss_lfoRate",
        "poss_lfoDepth",
        "poss_envAmount",
        "poss_fmDepth",
        "poss_oscALevel",
        "poss_oscBLevel",
        "poss_oscBDetune",
        "poss_oscBAsymmetry",
        "poss_oscAPulseWidth",
        "poss_oscWave",
        "poss_noiseTransient",
        "poss_filtEnvVelScale",
        "poss_sub_osc",
        "poss_subShape",
    };

    if (removed.count(paramId))
        return {}; // empty = drop this param

    auto it = renamed.find(paramId);
    return (it != renamed.end()) ? it->second : paramId;
}

// Valid moods — 16 browsing categories plus User.
// Must match the 16 mood directories under Presets/XOceanus/ and CLAUDE.md.
inline const juce::StringArray validMoods{"Foundation", "Atmosphere", "Entangled", "Prism",       "Flux",   "Aether",
                                          "Family",     "Submerged",  "Coupling",  "Crystalline", "Deep",   "Ethereal",
                                          "Kinetic",    "Luminous",   "Organic",   "Shadow",      "User"};

// Valid coupling intensity levels.
inline const juce::StringArray validCouplingIntensities{"None", "Low", "Medium", "High",
                                                        // Legacy aliases (kept for backward preset compatibility)
                                                        "Subtle", "Moderate", "Deep"};

// Valid coupling pair types (string form as they appear in .xometa JSON).
// Must match the CouplingType enum in SynthEngine.h 1:1.
// Accepts both CamelCase (AmpToFilter) and arrow (Amp->Filter) formats.
inline const juce::StringArray validCouplingTypes{
    "AmpToFilter", "AmpToPitch", "LFOToPitch", "EnvToMorph", "AudioToFM", "AudioToRing", "FilterToFilter", "AmpToChoke",
    "RhythmToBlend", "EnvToDecay", "PitchToPitch", "AudioToWavetable", "AudioToBuffer", "KnotTopology",
    "TriangularCoupling",
    // Legacy arrow-notation aliases
    "Amp->Filter", "Amp->Pitch", "LFO->Pitch", "Env->Morph", "Audio->FM", "Audio->Ring", "Filter->Filter", "Amp->Choke",
    "Rhythm->Blend", "Env->Decay", "Pitch->Pitch", "Audio->Wavetable", "Audio->Buffer", "Knot->Topology",
    "Triangular->Coupling"};

//==============================================================================
// 6D Sonic DNA — fingerprint for similarity search, morphing, and breeding.
struct PresetDNA
{
    float brightness = 0.5f;
    float warmth = 0.5f;
    float movement = 0.5f;
    float density = 0.5f;
    float space = 0.5f;
    float aggression = 0.5f;
};

//==============================================================================
// A single coupling pair — directional cross-engine modulation route.
struct CouplingPair
{
    juce::String engineA; // Source engine
    juce::String engineB; // Destination engine
    juce::String type;    // e.g. "Amp->Filter"
    float amount = 0.0f;  // -1.0 to +1.0
};

//==============================================================================
// A single macro target — one parameter controlled by a global macro knob.
// Each preset stores up to 4 macro slots (CHARACTER/MOVEMENT/COUPLING/SPACE),
// and each slot can route to multiple engine parameters.
//
// NOTE: Named PresetMacroTarget to avoid collision with xoceanus::MacroTarget
// in MacroSystem.h, which is the live-runtime modulation struct.
struct PresetMacroTarget
{
    juce::String engineName; // Canonical engine ID (e.g. "Onset", "Obrix")
    juce::String paramId;    // Frozen parameter ID (e.g. "perc_noiseLevel")
    float depthMin = 0.0f;   // Parameter value when macro is at 0.0
    float depthMax = 1.0f;   // Parameter value when macro is at 1.0
};

//==============================================================================
// Complete preset data as loaded from a .xometa file.
struct PresetData
{
    int schemaVersion = 1;
    juce::String name;
    juce::String
        mood; // 16 moods: Foundation|Atmosphere|Entangled|Prism|Flux|Aether|Family|Submerged|Coupling|Crystalline|Deep|Ethereal|Kinetic|Luminous|Organic|Shadow|User
    juce::StringArray engines; // 1-5 engine names (MaxSlots = 5)
    juce::String author;
    juce::String version;
    juce::String description;
    juce::StringArray tags;
    juce::StringArray macroLabels;  // 4 labels: CHARACTER, MOVEMENT, COUPLING, SPACE
    juce::String couplingIntensity; // None|Subtle|Moderate|Deep
    float tempo = 0.0f;             // 0 if not tempo-dependent
    PresetDNA dna;
    std::map<juce::String, juce::var> parametersByEngine; // engine name -> params object
    std::vector<CouplingPair> couplingPairs;
    // Macro target routing: [0]=CHARACTER [1]=MOVEMENT [2]=COUPLING [3]=SPACE.
    // Each slot holds zero or more parameter targets that the macro sweeps.
    std::array<std::vector<PresetMacroTarget>, 4> macroTargets;
    juce::var sequencerData; // Raw JSON, undefined if no sequencer

    // Source file for navigation purposes (empty for programmatic presets)
    juce::File sourceFile;
};

//==============================================================================
// PresetManager — Loading, saving, browsing, and DNA-powered search for
// the XOceanus .xometa preset format.
//
// Design contract:
//   - UI-thread only. No audio-thread calls.
//   - Full library held in memory (519-1000 presets is small).
//   - Uses juce::JSON for all parsing/serialization.
//   - Handles malformed presets gracefully — never crashes on bad data.
//
class PresetManager
{
public:
    //--------------------------------------------------------------------------
    // Listener interface — notified when the active preset changes.
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void presetLoaded(const PresetData& preset) = 0;
    };

    //--------------------------------------------------------------------------
    PresetManager() = default;

    // Issue #712 — Destructor stops any in-flight scan thread.
    //
    // The ScanThread posts a callAsync that holds a WeakReference to this
    // PresetManager.  Clearing the weak master here (JUCE does this when the
    // owner is destroyed) means the callAsync lambda becomes a harmless no-op
    // if it fires after destruction.  We also signal the worker and wait so the
    // thread object is not leaked.
    ~PresetManager()
    {
        if (activeScanThread != nullptr)
        {
            activeScanThread->signalThreadShouldExit();
            // waitForThreadToExit blocks until run() returns.  run() completes
            // quickly once threadShouldExit() returns true (the file-scan loop
            // checks the flag every iteration).  The self-delete inside callAsync
            // is safe even after we return here because callAsync owns the thread
            // pointer exclusively at that point.
            activeScanThread->waitForThreadToExit(3000);
            // After wait returns, run() has posted the callAsync and exited.
            // The WeakReference guard in the lambda prevents it touching the
            // now-dead library.  We clear our pointer (it will self-delete in
            // the callAsync).
            activeScanThread = nullptr;
        }
    }

    //==========================================================================
    // Loading
    //==========================================================================

    // Load a preset from a .xometa file on disk.
    // Returns true on success, false if the file can't be read or is invalid.
    // Maximum .xometa file size (1 MB) — prevents OOM on corrupted/malicious files.
    static constexpr int64_t kMaxPresetFileSize = 1024 * 1024;

    // Maximum number of presets loaded into the in-memory library (issue #424).
    // Prevents unbounded memory growth when a deeply-nested or very large preset
    // directory is scanned. O(n) DNA search operations also degrade linearly beyond
    // this threshold. 10 000 entries at ~4 KB average = ~40 MB RAM.
    static constexpr size_t kMaxLibrarySize = 10000;

    bool loadPresetFromFile(const juce::File& file)
    {
        if (!file.existsAsFile())
            return false;

        if (file.getSize() > kMaxPresetFileSize)
            return false;

        auto jsonString = file.loadFileAsString();
        if (jsonString.isEmpty())
            return false;

        PresetData preset;
        if (!parseJSON(jsonString, preset))
            return false;

        preset.sourceFile = file;
        setCurrentPreset(preset);
        return true;
    }

    // Load a preset from a raw JSON string.
    // Returns true on success, false if parsing or validation fails.
    bool loadPresetFromJSON(const juce::String& jsonString)
    {
        PresetData preset;
        if (!parseJSON(jsonString, preset))
            return false;

        setCurrentPreset(preset);
        return true;
    }

    // Issue #899 — Load the embedded Init preset from BinaryData.
    // Called on first launch (empty library or no factory preset directory) so
    // the user always has a named, playable preset in the browser strip rather
    // than an anonymous blank state.  Returns the parsed PresetData on success,
    // or a default-constructed (empty name) preset on failure.
    //
    // The embedded JSON is compiled into InitPresetData::Init_xometa via the
    // XOceanusInitPreset juce_add_binary_data target.  If BinaryData lookup
    // fails (should never happen in a correctly linked build) this returns a
    // graceful no-op preset rather than crashing.
    PresetData loadEmbeddedInitPreset()
    {
        // BinaryData symbol generated by JUCE from Resources/Init.xometa.
        // Symbol name is derived from the filename: Init.xometa → Init_xometa.
        const char* data = reinterpret_cast<const char*>(InitPresetData::Init_xometa);
        const int   size = InitPresetData::Init_xometaSize;

        if (data == nullptr || size <= 0)
        {
            DBG("PresetManager::loadEmbeddedInitPreset: BinaryData not found");
            return {};
        }

        juce::String jsonString = juce::String::createStringFromData(data, size);
        PresetData preset;
        if (!parseJSON(jsonString, preset))
        {
            DBG("PresetManager::loadEmbeddedInitPreset: failed to parse embedded Init.xometa");
            return {};
        }

        setCurrentPreset(preset);
        return preset;
    }

    //==========================================================================
    // Saving
    //==========================================================================

    // Save a preset to a .xometa file on disk.
    // Returns true on success, false if the file can't be written.
    bool savePresetToFile(const juce::File& file, const PresetData& preset)
    {
        auto json = serializeToJSON(preset);
        if (json.isEmpty())
            return false;

        return file.replaceWithText(json);
    }

    // Serialize a PresetData to a JSON string.
    juce::String serializeToJSON(const PresetData& preset)
    {
        auto* root = new juce::DynamicObject();

        root->setProperty("schema_version", preset.schemaVersion);
        root->setProperty("name", preset.name);
        root->setProperty("mood", preset.mood);

        // engines array
        juce::var enginesArray;
        for (const auto& e : preset.engines)
            enginesArray.append(e);
        root->setProperty("engines", enginesArray);

        root->setProperty("author", preset.author);
        root->setProperty("version", preset.version);
        root->setProperty("description", preset.description);

        // tags
        juce::var tagsArray;
        for (const auto& t : preset.tags)
            tagsArray.append(t);
        root->setProperty("tags", tagsArray);

        // macroLabels
        juce::var macroArray;
        for (const auto& m : preset.macroLabels)
            macroArray.append(m);
        root->setProperty("macroLabels", macroArray);

        root->setProperty("couplingIntensity", preset.couplingIntensity);

        // tempo — null if 0
        if (preset.tempo > 0.0f)
            root->setProperty("tempo", static_cast<double>(preset.tempo));
        else
            root->setProperty("tempo", juce::var());

        // dna
        auto* dnaObj = new juce::DynamicObject();
        dnaObj->setProperty("brightness", static_cast<double>(preset.dna.brightness));
        dnaObj->setProperty("warmth", static_cast<double>(preset.dna.warmth));
        dnaObj->setProperty("movement", static_cast<double>(preset.dna.movement));
        dnaObj->setProperty("density", static_cast<double>(preset.dna.density));
        dnaObj->setProperty("space", static_cast<double>(preset.dna.space));
        dnaObj->setProperty("aggression", static_cast<double>(preset.dna.aggression));
        root->setProperty("dna", juce::var(dnaObj));

        // parameters — engine-keyed objects
        auto* paramsObj = new juce::DynamicObject();
        for (const auto& [engineName, params] : preset.parametersByEngine)
            paramsObj->setProperty(engineName, params);
        root->setProperty("parameters", juce::var(paramsObj));

        // coupling
        if (!preset.couplingPairs.empty())
        {
            auto* couplingObj = new juce::DynamicObject();
            juce::var pairsArray;
            for (const auto& cp : preset.couplingPairs)
            {
                auto* pairObj = new juce::DynamicObject();
                pairObj->setProperty("engineA", cp.engineA);
                pairObj->setProperty("engineB", cp.engineB);
                pairObj->setProperty("type", cp.type);
                pairObj->setProperty("amount", static_cast<double>(cp.amount));
                pairsArray.append(juce::var(pairObj));
            }
            couplingObj->setProperty("pairs", pairsArray);
            root->setProperty("coupling", juce::var(couplingObj));
        }
        else
        {
            root->setProperty("coupling", juce::var());
        }

        // sequencer
        if (!preset.sequencerData.isVoid() && !preset.sequencerData.isUndefined())
            root->setProperty("sequencer", preset.sequencerData);
        else
            root->setProperty("sequencer", juce::var());

        // macroTargets — 4-element array, one entry per macro slot.
        // Each element is an array of { engineName, paramId, depthMin, depthMax } objects.
        // Always written (even when all slots are empty) so the field is present in every
        // saved preset and loaders never need to fabricate a fallback value.
        {
            static const juce::StringArray macroSlotNames{"CHARACTER", "MOVEMENT", "COUPLING", "SPACE"};
            juce::var macroTargetsArray;
            for (int slot = 0; slot < 4; ++slot)
            {
                juce::var slotArray;
                for (const auto& mt : preset.macroTargets[static_cast<size_t>(slot)])
                {
                    auto* mtObj = new juce::DynamicObject();
                    mtObj->setProperty("engineName", mt.engineName);
                    mtObj->setProperty("paramId", mt.paramId);
                    mtObj->setProperty("depthMin", static_cast<double>(mt.depthMin));
                    mtObj->setProperty("depthMax", static_cast<double>(mt.depthMax));
                    slotArray.append(juce::var(mtObj));
                }
                macroTargetsArray.append(slotArray);
            }
            root->setProperty("macroTargets", macroTargetsArray);
        }

        return juce::JSON::toString(juce::var(root), false);
    }

    //==========================================================================
    // Library management
    //==========================================================================

    // Scan a directory (recursively) for .xometa files and add them to the library.
    // UI-thread only.  For non-blocking startup, prefer scanPresetDirectoryAsync().
    void scanPresetDirectory(const juce::File& directory)
    {
        if (!directory.isDirectory())
            return;

        // Build into a local mutable vector, then swap-publish atomically.
        auto newLib = std::make_shared<std::vector<PresetData>>();

        for (const auto& file : directory.findChildFiles(juce::File::findFiles, true, "*.xometa"))
        {
            // Issue #424: cap library size to prevent unbounded memory growth.
            if (newLib->size() >= kMaxLibrarySize)
            {
                DBG("PresetManager: library limit (" + juce::String((int)kMaxLibrarySize) +
                    ") reached — stopping scan of " + directory.getFullPathName());
                break;
            }

            if (file.getSize() > kMaxPresetFileSize)
                continue;

            auto jsonString = file.loadFileAsString();
            if (jsonString.isEmpty())
                continue;

            PresetData preset;
            if (parseJSON(jsonString, preset))
            {
                preset.sourceFile = file;
                newLib->push_back(std::move(preset));
            }
        }

        // Publish: make the vector const, then atomic-store the new shared_ptr.
        std::atomic_store(&library_,
            std::static_pointer_cast<const std::vector<PresetData>>(std::move(newLib)));
    }

    // Issue #712 — Async variant of scanPresetDirectory().
    //
    // Builds the preset library on a background thread so the message thread is
    // never blocked by disk I/O.  The `library` vector is ONLY mutated on the
    // message thread (via callAsync) — the worker thread operates on a completely
    // separate local vector and never touches `library` directly.
    //
    // `onComplete` is called on the message thread once the swap is done.
    // It may be nullptr.
    //
    // Safety rules:
    //   - Never call `library.push_back()` from the worker thread.
    //   - The atomic swap and the `onComplete` call both happen inside callAsync,
    //     which guarantees message-thread execution.
    //   - Only one scan should be in flight at a time; starting a second scan
    //     while one is running is safe but may produce interleaved results — the
    //     caller is expected to guard against this (XOceanusEditor calls it once
    //     at construction time).
    void scanPresetDirectoryAsync(const juce::File& directory,
                                  std::function<void()> onComplete)
    {
        // Inner Thread subclass — self-deletes after the callAsync fires.
        // Issue #712: uses WeakReference<PresetManager> so the callAsync lambda
        // is a no-op if the PresetManager is destroyed before the message-thread
        // callback fires (e.g., plugin window closed milliseconds after open).
        struct ScanThread : public juce::Thread
        {
            ScanThread(PresetManager& mgr,
                       juce::File dir,
                       std::function<void()> done)
                : juce::Thread("PresetScan"),
                  weakManager(&mgr),
                  scanDir(std::move(dir)),
                  onComplete(std::move(done))
            {
            }

            void run() override
            {
                // Build the library into a *local* vector — never touch
                // manager.library from here.  All JSON parsing is done with
                // a temporary local PresetManager so parseJSON (private) is
                // accessible without touching the shared instance on the
                // wrong thread.
                std::vector<PresetData> localLib;

                if (scanDir.isDirectory())
                {
                    // Temporary manager used only for its parseJSON helper.
                    // This avoids any cross-thread access to the shared manager.
                    PresetManager localParser;

                    for (const auto& file :
                         scanDir.findChildFiles(juce::File::findFiles, true, "*.xometa"))
                    {
                        if (threadShouldExit())
                            break;

                        if (localLib.size() >= PresetManager::kMaxLibrarySize)
                        {
                            DBG("PresetManager: library limit (" +
                                juce::String((int)PresetManager::kMaxLibrarySize) +
                                ") reached — stopping async scan of " +
                                scanDir.getFullPathName());
                            break;
                        }

                        if (file.getSize() > PresetManager::kMaxPresetFileSize)
                            continue;

                        auto jsonString = file.loadFileAsString();
                        if (jsonString.isEmpty())
                            continue;

                        PresetData preset;
                        if (localParser.parseJSON(jsonString, preset))
                        {
                            preset.sourceFile = file;
                            localLib.push_back(std::move(preset));
                        }
                    }
                }

                // Record whether the scan was cancelled mid-flight.  If it was,
                // we still post callAsync (so self-delete happens on the message
                // thread) but we skip updating the library and calling onComplete.
                const bool wasCancelled = threadShouldExit();

                // Swap onto the message thread.  The WeakReference guard ensures
                // this is a no-op if PresetManager was destroyed while we were
                // scanning (e.g., plugin window closed before scan finished).
                juce::MessageManager::callAsync(
                    [weakRef    = weakManager,
                     newLib     = std::move(localLib),
                     done       = std::move(onComplete),
                     cancelled  = wasCancelled,
                     self       = this]() mutable
                    {
                        // Issue #712: guard against UAF — manager may be dead.
                        if (!cancelled)
                        {
                            if (auto* mgr = weakRef.get())
                            {
                                // Publish via atomic swap (O(1), no vector copy).
                                std::atomic_store(&mgr->library_,
                                    std::make_shared<const std::vector<PresetData>>(std::move(newLib)));
                                mgr->activeScanThread = nullptr; // clear tracker
                                if (done)
                                    done();
                            }
                        }
                        // Thread has finished its run() by now; delete self.
                        delete self;
                    });
            }

            juce::WeakReference<PresetManager> weakManager;
            juce::File                          scanDir;
            std::function<void()>               onComplete;
        };

        // Cancel and wait for any previous scan before starting a new one.
        // This prevents two scans racing to overwrite `library`.
        if (activeScanThread != nullptr)
        {
            activeScanThread->signalThreadShouldExit();
            activeScanThread->waitForThreadToExit(3000);
            // activeScanThread will self-delete inside callAsync; null it here
            // so we don't double-signal if the destructor also runs.
            activeScanThread = nullptr;
        }

        auto* thread = new ScanThread(*this, directory, std::move(onComplete));
        activeScanThread = thread;
        thread->startThread(juce::Thread::Priority::background);
    }

    // Return all presets matching a specific mood.
    std::vector<PresetData> getPresetsForMood(const juce::String& mood) const
    {
        auto lib = std::atomic_load(&library_);
        std::vector<PresetData> results;
        for (const auto& p : *lib)
            if (p.mood == mood)
                results.push_back(p);
        return results;
    }

    // Return all presets containing a specific tag (case-insensitive).
    std::vector<PresetData> searchByTag(const juce::String& tag) const
    {
        auto lib = std::atomic_load(&library_);
        std::vector<PresetData> results;
        auto tagLower = tag.toLowerCase();
        for (const auto& p : *lib)
            for (const auto& t : p.tags)
                if (t.toLowerCase() == tagLower)
                {
                    results.push_back(p);
                    break;
                }
        return results;
    }

    // Return all presets whose name contains the query (case-insensitive).
    std::vector<PresetData> searchByName(const juce::String& query) const
    {
        auto lib = std::atomic_load(&library_);
        std::vector<PresetData> results;
        auto queryLower = query.toLowerCase();
        for (const auto& p : *lib)
            if (p.name.toLowerCase().contains(queryLower))
                results.push_back(p);
        return results;
    }

    // Return a shared_ptr to the full in-memory library (O(1) — no vector copy).
    // Callers on background threads hold the shared_ptr for the duration of their
    // work; a concurrent scanPresetDirectoryAsync() swap never invalidates them.
    std::shared_ptr<const std::vector<PresetData>> getLibrary() const
    {
        return std::atomic_load(&library_);
    }

    // Return the number of presets in the library.
    int getLibrarySize() const { return static_cast<int>(std::atomic_load(&library_)->size()); }

    // Add a preset to the library (for testing and programmatic use).
    // Builds a new vector from the current contents, appends, then publishes.
    void addPreset(PresetData preset)
    {
        auto current = std::atomic_load(&library_);
        auto newLib  = std::make_shared<std::vector<PresetData>>(*current);
        newLib->push_back(std::move(preset));
        std::atomic_store(&library_,
            std::static_pointer_cast<const std::vector<PresetData>>(std::move(newLib)));
    }

    //==========================================================================
    // DNA-powered features
    //==========================================================================

    // Euclidean distance in 6D DNA space.
    float dnaDistance(const PresetDNA& a, const PresetDNA& b) const
    {
        float db = a.brightness - b.brightness;
        float dw = a.warmth - b.warmth;
        float dm = a.movement - b.movement;
        float dd = a.density - b.density;
        float ds = a.space - b.space;
        float da = a.aggression - b.aggression;
        return std::sqrt(db * db + dw * dw + dm * dm + dd * dd + ds * ds + da * da);
    }

    // Find the `count` most similar presets by DNA distance.
    std::vector<PresetData> findSimilar(const PresetDNA& dna, int count = 5) const { return findNearest(dna, count); }

    // Find the `count` most opposite presets: invert the DNA vector
    // (1.0 - each dimension), then find nearest matches to the inverted point.
    std::vector<PresetData> findOpposite(const PresetDNA& dna, int count = 5) const
    {
        PresetDNA inverted;
        inverted.brightness = 1.0f - dna.brightness;
        inverted.warmth = 1.0f - dna.warmth;
        inverted.movement = 1.0f - dna.movement;
        inverted.density = 1.0f - dna.density;
        inverted.space = 1.0f - dna.space;
        inverted.aggression = 1.0f - dna.aggression;
        return findNearest(inverted, count);
    }

    //==========================================================================
    // Current preset
    //==========================================================================

    const PresetData& getCurrentPreset() const { return currentPreset; }
    /** Returns the 0-based index of the current preset in the library, or -1 if unknown. */
    int getCurrentPresetIndex() const { return currentIndex; }

    void setCurrentPreset(const PresetData& preset)
    {
        currentPreset = preset;

        // Update navigation index if this preset exists in the library.
        // Load once to keep a stable snapshot for the duration of this scan.
        auto lib = std::atomic_load(&library_);
        currentIndex = -1;
        for (int i = 0; i < static_cast<int>(lib->size()); ++i)
        {
            if ((*lib)[static_cast<size_t>(i)].name == preset.name)
            {
                currentIndex = i;
                break;
            }
        }

        notifyListeners();
    }

    //==========================================================================
    // Navigation — step through library sequentially.
    //==========================================================================

    void nextPreset()
    {
        auto lib = std::atomic_load(&library_);
        if (lib->empty())
            return;

        currentIndex = (currentIndex + 1) % static_cast<int>(lib->size());
        currentPreset = (*lib)[static_cast<size_t>(currentIndex)];
        notifyListeners();
    }

    void previousPreset()
    {
        auto lib = std::atomic_load(&library_);
        if (lib->empty())
            return;

        currentIndex--;
        if (currentIndex < 0)
            currentIndex = static_cast<int>(lib->size()) - 1;

        currentPreset = (*lib)[static_cast<size_t>(currentIndex)];
        notifyListeners();
    }

    //==========================================================================
    // Listeners
    //==========================================================================

    void addListener(Listener* l)
    {
        if (l != nullptr)
            listeners.push_back(l);
    }

    void removeListener(Listener* l)
    {
        listeners.erase(std::remove(listeners.begin(), listeners.end(), l), listeners.end());
    }

private:
    //==========================================================================
    // JSON parsing — extract a PresetData from a JSON string.
    // Returns false on any fatal parse error; tolerates missing optional fields.
    //==========================================================================
    bool parseJSON(const juce::String& jsonString, PresetData& out)
    {
        auto parsed = juce::JSON::parse(jsonString);
        if (!parsed.isObject())
            return false;

        auto* obj = parsed.getDynamicObject();
        if (obj == nullptr)
            return false;

        // --- Required fields ---

        // schema_version
        if (!obj->hasProperty("schema_version"))
            return false;
        out.schemaVersion = static_cast<int>(obj->getProperty("schema_version"));
        if (out.schemaVersion < 1)
            return false;
        static constexpr int kCurrentSchemaVersion = 1;
        if (out.schemaVersion > kCurrentSchemaVersion)
        {
            DBG("Preset schema version " + juce::String(out.schemaVersion) + " is newer than supported version " + juce::String(kCurrentSchemaVersion));
            return false;
        }

        // name
        if (!obj->hasProperty("name"))
            return false;
        out.name = obj->getProperty("name").toString().substring(0, 128);
        if (out.name.isEmpty())
            return false;

        // mood
        if (!obj->hasProperty("mood"))
            return false;
        out.mood = obj->getProperty("mood").toString();
        if (!validMoods.contains(out.mood))
            out.mood = "User"; // fallback for unknown moods

        // engines
        if (!obj->hasProperty("engines"))
            return false;
        auto enginesVar = obj->getProperty("engines");
        if (!enginesVar.isArray() || enginesVar.getArray()->isEmpty())
            return false;
        out.engines.clear();
        for (const auto& e : *enginesVar.getArray())
        {
            auto engineName = resolveEngineAlias(e.toString());
            if (getValidEngineNames().contains(engineName))
                out.engines.add(engineName);
        }
        if (out.engines.isEmpty())
            return false;
        // Cap at MaxSlots engines — matches EngineRegistry::MaxSlots and MegaCouplingMatrix::MaxSlots (both = 5).
        while (out.engines.size() > 5)
            out.engines.remove(out.engines.size() - 1);

        // author
        out.author = obj->getProperty("author").toString();
        if (out.author.isEmpty())
            out.author = "Unknown";

        // version
        out.version = obj->getProperty("version").toString();
        if (out.version.isEmpty())
            out.version = "1.0.0";

        // parameters (required by schema)
        if (!obj->hasProperty("parameters"))
            return false;
        auto paramsVar = obj->getProperty("parameters");
        if (paramsVar.isObject())
        {
            auto* paramsObj = paramsVar.getDynamicObject();
            if (paramsObj != nullptr)
            {
                for (const auto& prop : paramsObj->getProperties())
                {
                    auto engineName = resolveEngineAlias(prop.name.toString());
                    if (getValidEngineNames().contains(engineName))
                        out.parametersByEngine[engineName] = prop.value;
                }
            }
        }

        // --- Optional fields ---

        // description
        out.description = obj->getProperty("description").toString();

        // tags
        out.tags.clear();
        if (obj->hasProperty("tags"))
        {
            auto tagsVar = obj->getProperty("tags");
            if (tagsVar.isArray())
                for (const auto& t : *tagsVar.getArray())
                    out.tags.add(t.toString());
        }

        // macroLabels — default to the 4 standard labels
        out.macroLabels.clear();
        if (obj->hasProperty("macroLabels"))
        {
            auto macroVar = obj->getProperty("macroLabels");
            if (macroVar.isArray())
                for (const auto& m : *macroVar.getArray())
                    out.macroLabels.add(m.toString().substring(0, 20));
        }
        // Pad to 4 with defaults if needed
        static const juce::StringArray defaultMacros{"CHARACTER", "MOVEMENT", "COUPLING", "SPACE"};
        while (out.macroLabels.size() < 4)
            out.macroLabels.add(defaultMacros[out.macroLabels.size()]);
        // Trim to exactly 4
        while (out.macroLabels.size() > 4)
            out.macroLabels.remove(out.macroLabels.size() - 1);

        // couplingIntensity
        out.couplingIntensity = obj->getProperty("couplingIntensity").toString();
        if (!validCouplingIntensities.contains(out.couplingIntensity))
            out.couplingIntensity = "None";

        // tempo — 0 means not tempo-dependent
        auto tempoVar = obj->getProperty("tempo");
        if (tempoVar.isDouble() || tempoVar.isInt())
            out.tempo = static_cast<float>(tempoVar);
        else
            out.tempo = 0.0f;

        // dna
        if (obj->hasProperty("dna"))
        {
            auto dnaVar = obj->getProperty("dna");
            if (dnaVar.isObject())
            {
                auto* dnaObj = dnaVar.getDynamicObject();
                if (dnaObj != nullptr)
                {
                    out.dna.brightness = clampDNA(dnaObj->getProperty("brightness"));
                    out.dna.warmth = clampDNA(dnaObj->getProperty("warmth"));
                    out.dna.movement = clampDNA(dnaObj->getProperty("movement"));
                    out.dna.density = clampDNA(dnaObj->getProperty("density"));
                    out.dna.space = clampDNA(dnaObj->getProperty("space"));
                    out.dna.aggression = clampDNA(dnaObj->getProperty("aggression"));
                }
            }
        }
        // If no DNA was present, the defaults (0.5 each) mark it as needing computation.

        // coupling pairs
        out.couplingPairs.clear();
        if (obj->hasProperty("coupling"))
        {
            auto couplingVar = obj->getProperty("coupling");
            if (couplingVar.isObject())
            {
                auto* couplingObj = couplingVar.getDynamicObject();
                if (couplingObj != nullptr && couplingObj->hasProperty("pairs"))
                {
                    auto pairsVar = couplingObj->getProperty("pairs");
                    if (pairsVar.isArray())
                    {
                        for (const auto& pairVar : *pairsVar.getArray())
                        {
                            if (!pairVar.isObject())
                                continue;
                            auto* pairObj = pairVar.getDynamicObject();
                            if (pairObj == nullptr)
                                continue;

                            CouplingPair cp;
                            cp.engineA = resolveEngineAlias(pairObj->getProperty("engineA").toString());
                            cp.engineB = resolveEngineAlias(pairObj->getProperty("engineB").toString());
                            cp.type = pairObj->getProperty("type").toString();
                            cp.amount = static_cast<float>(pairObj->getProperty("amount"));

                            // Validate: both engines must be known, type must be valid
                            if (getValidEngineNames().contains(cp.engineA) && getValidEngineNames().contains(cp.engineB) &&
                                validCouplingTypes.contains(cp.type))
                            {
                                cp.amount = juce::jlimit(-1.0f, 1.0f, cp.amount);
                                out.couplingPairs.push_back(cp);
                            }
                        }
                    }
                }
            }
        }

        // sequencer — store raw JSON, keep undefined if absent/null
        if (obj->hasProperty("sequencer"))
        {
            auto seqVar = obj->getProperty("sequencer");
            if (seqVar.isObject())
                out.sequencerData = seqVar;
            else
                out.sequencerData = juce::var(); // null/void
        }
        else
        {
            out.sequencerData = juce::var();
        }

        // macroTargets — optional; old presets without this field load with empty targets.
        // Format: 4-element array of arrays, each inner array holds target objects:
        //   { "engineName": "Onset", "paramId": "perc_noiseLevel", "depthMin": 0.0, "depthMax": 1.0 }
        // Slot order: [0]=CHARACTER [1]=MOVEMENT [2]=COUPLING [3]=SPACE.
        for (auto& slot : out.macroTargets)
            slot.clear();

        if (obj->hasProperty("macroTargets"))
        {
            auto macroTargetsVar = obj->getProperty("macroTargets");
            if (macroTargetsVar.isArray())
            {
                auto* outerArr = macroTargetsVar.getArray();
                int numSlots = std::min(4, static_cast<int>(outerArr->size()));
                for (int slot = 0; slot < numSlots; ++slot)
                {
                    const auto& slotVar = (*outerArr)[static_cast<size_t>(slot)];
                    if (!slotVar.isArray())
                        continue;

                    for (const auto& targetVar : *slotVar.getArray())
                    {
                        if (!targetVar.isObject())
                            continue;
                        auto* targetObj = targetVar.getDynamicObject();
                        if (targetObj == nullptr)
                            continue;

                        PresetMacroTarget mt;
                        mt.engineName = resolveEngineAlias(targetObj->getProperty("engineName").toString());
                        mt.paramId = targetObj->getProperty("paramId").toString();

                        // depthMin/depthMax: default to 0/1 if absent or malformed
                        auto depthMinVar = targetObj->getProperty("depthMin");
                        auto depthMaxVar = targetObj->getProperty("depthMax");
                        mt.depthMin =
                            (depthMinVar.isDouble() || depthMinVar.isInt()) ? static_cast<float>(depthMinVar) : 0.0f;
                        mt.depthMax =
                            (depthMaxVar.isDouble() || depthMaxVar.isInt()) ? static_cast<float>(depthMaxVar) : 1.0f;

                        // Only store targets whose engine and param IDs are non-empty.
                        // We intentionally do NOT validate engineName against validEngineNames
                        // here: third-party or future engines may define targets that load
                        // before their engine is registered. MacroSystem is responsible for
                        // silently skipping targets it cannot resolve at apply-time.
                        if (mt.engineName.isNotEmpty() && mt.paramId.isNotEmpty())
                            out.macroTargets[static_cast<size_t>(slot)].push_back(std::move(mt));
                    }
                }
            }
        }

        return true;
    }

    //==========================================================================
    // DNA helpers
    //==========================================================================

    // Clamp a juce::var to a valid DNA range [0, 1], defaulting to 0.5.
    static float clampDNA(const juce::var& v)
    {
        if (v.isDouble() || v.isInt())
            return juce::jlimit(0.0f, 1.0f, static_cast<float>(v));
        return 0.5f;
    }

    // Find the `count` nearest presets to `target` in DNA space.
    std::vector<PresetData> findNearest(const PresetDNA& target, int count) const
    {
        struct Ranked
        {
            float distance;
            size_t index;
        };

        auto lib = std::atomic_load(&library_);
        std::vector<Ranked> ranked;
        ranked.reserve(lib->size());

        for (size_t i = 0; i < lib->size(); ++i)
            ranked.push_back({dnaDistance((*lib)[i].dna, target), i});

        std::sort(ranked.begin(), ranked.end(),
                  [](const Ranked& a, const Ranked& b) { return a.distance < b.distance; });

        std::vector<PresetData> results;
        int n = std::min(count, static_cast<int>(ranked.size()));
        for (int i = 0; i < n; ++i)
            results.push_back((*lib)[ranked[static_cast<size_t>(i)].index]);

        return results;
    }

    //==========================================================================
    // Listener notification
    //==========================================================================
    void notifyListeners()
    {
        for (auto* l : listeners)
            if (l != nullptr)
                l->presetLoaded(currentPreset);
    }

    //==========================================================================
    // State
    //==========================================================================
    // shared_ptr<const vector> enables O(1) snapshot hand-off to filter/export
    // background threads.  Use std::atomic_load / std::atomic_store for all
    // access — never read or write library_ directly.
    mutable std::shared_ptr<const std::vector<PresetData>> library_
        = std::make_shared<const std::vector<PresetData>>();
    PresetData currentPreset;
    int currentIndex = -1;
    std::vector<Listener*> listeners;

    // Issue #712: raw pointer to the in-flight scan thread (if any).
    // Owned by the callAsync self-delete; we only hold a non-owning observer
    // pointer here so the destructor can signal it.  Must only be read/written
    // on the message thread.
    juce::Thread* activeScanThread = nullptr;

    // Enables juce::WeakReference<PresetManager> so the callAsync lambda in
    // scanPresetDirectoryAsync() can safely detect destruction without UAF.
    JUCE_DECLARE_WEAK_REFERENCEABLE(PresetManager)
};

} // namespace xoceanus
