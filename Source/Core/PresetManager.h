#pragma once
#include <juce_core/juce_core.h>
#include <cmath>
#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace xomnibus {

//==============================================================================
// Valid engine names — all registered XOmnibus engines.
inline const juce::StringArray validEngineNames {
    // All engine IDs start with O (brand convention)
    "OddfeliX", "OddOscar",  // Mascots: feliX the neon tetra, Oscar the axolotl
    "Overdub", "Odyssey", "Oblong", "Obese", "Onset",
    "Overworld", "Opal", "Orbital",
    "Organon", "Ouroboros",
    "Obsidian", "Overbite", "Origami", "Oracle", "Obscura", "Oceanic",
    "Ocelot", "Osprey", "Osteria", "Owlfish",
    "Ohm", "Orphica", "Obbligato", "Ottoni", "Ole",
    "Optic", "Oblique", "Ombre", "Orca", "Octopus",
    // Legacy aliases (kept for backward preset compatibility)
    "XOddCouple", "XOverdub", "XOdyssey", "XOblong", "XOblongBob",
    "XObese", "XOnset", "XOrbital", "XOrganon", "XOuroboros",
    "XOpal", "XOpossum", "XOverbite", "XObsidian", "XOrigami",
    "XOracle", "XObscura", "XOceanic", "XOptic", "XOblique",
    "XOverworld", "XOrca", "XOctopus",
    "Snap", "Morph", "Dub", "Drift", "Bob", "Fat", "Bite"
};

// Resolve legacy engine name aliases to current canonical IDs.
// Returns the input unchanged if it's already a current ID or unrecognized.
inline juce::String resolveEngineAlias(const juce::String& name)
{
    static const std::map<juce::String, juce::String> aliases {
        { "Snap",        "OddfeliX"  },
        { "Morph",       "OddOscar"  },
        { "Dub",         "Overdub"   },
        { "Drift",       "Odyssey"   },
        { "Bob",         "Oblong"    },
        { "Fat",         "Obese"     },
        { "Bite",        "Overbite"  },
        { "XOddCouple",  "OddfeliX" },  // v0 name for Snap/feliX
        { "XOverdub",    "Overdub"   },
        { "XOdyssey",    "Odyssey"   },
        { "XOblong",     "Oblong"    },
        { "XOblongBob",  "Oblong"    },
        { "XObese",      "Obese"     },
        { "XOnset",      "Onset"     },
        { "XOrbital",    "Orbital"   },
        { "XOrganon",    "Organon"   },
        { "XOuroboros",  "Ouroboros" },
        { "XOpal",       "Opal"      },
        { "XOpossum",    "Overbite"  },
        { "XOverbite",   "Overbite"  },
        { "XObsidian",   "Obsidian"  },
        { "XOrigami",    "Origami"   },
        { "XOracle",     "Oracle"    },
        { "XObscura",    "Obscura"   },
        { "XOceanic",    "Oceanic"   },
        { "XOptic",      "Optic"     },
        { "XOblique",    "Oblique"   },
        { "XOverworld",  "Overworld" },
        { "XOrca",       "Orca"      },
        { "XOctopus",    "Octopus"   },
    };
    auto it = aliases.find(name);
    return (it != aliases.end()) ? it->second : name;
}

// Frozen parameter prefix for each canonical engine ID.
// These NEVER change — parameter IDs are stable across releases.
// Returns the prefix WITHOUT trailing underscore (e.g. "snap", "morph").
inline juce::String frozenPrefixForEngine(const juce::String& engineId)
{
    static const std::map<juce::String, juce::String> prefixes {
        { "OddfeliX",   "snap"    },
        { "OddOscar",   "morph"   },
        { "Overdub",     "dub"     },
        { "Odyssey",     "drift"   },
        { "Oblong",      "bob"     },
        { "Obese",       "fat"     },
        { "Overbite",    "poss"    },
        { "Onset",       "perc"    },
        { "Overworld",   "ow"      },
        { "Opal",        "opal"    },
        { "Orbital",     "orb"     },
        { "Organon",     "organon" },
        { "Ouroboros",   "ouro"    },
        { "Obsidian",    "obsidian"},
        { "Origami",     "origami" },
        { "Oracle",      "oracle"  },
        { "Obscura",     "obscura" },
        { "Oceanic",     "ocean"   },
        { "Optic",       "optic"   },
        { "Oblique",     "oblq"    },
        { "Ocelot",      "ocelot"  },
        { "Osprey",      "osprey"  },
        { "Osteria",     "osteria" },
        { "Owlfish",     "owl"     },
        { "Ohm",         "ohm"     },
        { "Orphica",     "orph"    },
        { "Obbligato",   "obbl"    },
        { "Ottoni",      "otto"    },
        { "Ole",         "ole"     },
        { "Ombre",       "ombre"   },
        { "Orca",        "orca"    },
        { "Octopus",     "octo"    },
        { "XOverlap",    "olap"    },
        { "XOutwit",     "owit"    },
        // V1 Concept Engines
        { "OpenSky",     "sky"     },
        { "Ostinato",    "osti"    },
        { "Oceandeep",   "deep"    },
        { "Ouie",        "ouie"    },
    };
    auto it = prefixes.find(engineId);
    return (it != prefixes.end()) ? it->second : engineId.toLowerCase();
}

// Resolve legacy per-parameter aliases for OddfeliX (Snap) engine.
// These were renamed or removed when the engine became purely percussive
// (no ADSR sustain/release stage). Returns the canonical param ID, or an
// empty String if the param has been removed and should be dropped.
inline juce::String resolveSnapParamAlias(const juce::String& paramId)
{
    // Renamed params: old ID -> canonical ID
    static const std::map<juce::String, juce::String> renamed {
        { "snap_resonance",    "snap_filterReso"      },
        { "snap_filterEnvAmt", "snap_filterEnvDepth"  },
        { "snap_filterEnv",    "snap_filterEnvDepth"  },
        { "snap_snapAmount",   "snap_snap"             },
    };
    // Removed params: had no equivalent in the percussive engine redesign
    static const std::set<juce::String> removed {
        "snap_attack", "snap_sustain", "snap_release",
        "snap_ampAttack", "snap_ampDecay", "snap_ampRelease", "snap_ampSustain",
        "snap_oscShape", "snap_oscTune", "snap_snapTone",
    };

    if (removed.count(paramId))
        return {};  // empty = drop this param

    auto it = renamed.find(paramId);
    return (it != renamed.end()) ? it->second : paramId;
}

// Valid moods — the 8 browsing categories plus User.
inline const juce::StringArray validMoods {
    "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family", "Submerged", "User"
};

// Valid coupling intensity levels.
inline const juce::StringArray validCouplingIntensities {
    "None", "Low", "Medium", "High",
    // Legacy aliases (kept for backward preset compatibility)
    "Subtle", "Moderate", "Deep"
};

// Valid coupling pair types (string form as they appear in .xometa JSON).
// Must match the CouplingType enum in SynthEngine.h 1:1.
// Accepts both CamelCase (AmpToFilter) and arrow (Amp->Filter) formats.
inline const juce::StringArray validCouplingTypes {
    "AmpToFilter", "AmpToPitch", "LFOToPitch", "EnvToMorph",
    "AudioToFM", "AudioToRing", "FilterToFilter", "AmpToChoke",
    "RhythmToBlend", "EnvToDecay", "PitchToPitch", "AudioToWavetable",
    // Legacy arrow-notation aliases
    "Amp->Filter", "Amp->Pitch", "LFO->Pitch", "Env->Morph",
    "Audio->FM", "Audio->Ring", "Filter->Filter", "Amp->Choke",
    "Rhythm->Blend", "Env->Decay", "Pitch->Pitch", "Audio->Wavetable"
};

//==============================================================================
// 6D Sonic DNA — fingerprint for similarity search, morphing, and breeding.
struct PresetDNA {
    float brightness = 0.5f;
    float warmth     = 0.5f;
    float movement   = 0.5f;
    float density    = 0.5f;
    float space      = 0.5f;
    float aggression = 0.5f;
};

//==============================================================================
// A single coupling pair — directional cross-engine modulation route.
struct CouplingPair {
    juce::String engineA;  // Source engine
    juce::String engineB;  // Destination engine
    juce::String type;     // e.g. "Amp->Filter"
    float amount = 0.0f;   // -1.0 to +1.0
};

//==============================================================================
// Complete preset data as loaded from a .xometa file.
struct PresetData {
    int schemaVersion = 1;
    juce::String name;
    juce::String mood;                     // Foundation|Atmosphere|Entangled|Prism|Flux|Aether|Family|Submerged|User
    juce::StringArray engines;             // 1-3 engine names
    juce::String author;
    juce::String version;
    juce::String description;
    juce::StringArray tags;
    juce::StringArray macroLabels;         // 4 labels
    juce::String couplingIntensity;        // None|Subtle|Moderate|Deep
    float tempo = 0.0f;                    // 0 if not tempo-dependent
    PresetDNA dna;
    std::map<juce::String, juce::var> parametersByEngine;  // engine name -> params object
    std::vector<CouplingPair> couplingPairs;
    juce::var sequencerData;               // Raw JSON, undefined if no sequencer

    // Source file for navigation purposes (empty for programmatic presets)
    juce::File sourceFile;
};

//==============================================================================
// PresetManager — Loading, saving, browsing, and DNA-powered search for
// the XOmnibus .xometa preset format.
//
// Design contract:
//   - UI-thread only. No audio-thread calls.
//   - Full library held in memory (519-1000 presets is small).
//   - Uses juce::JSON for all parsing/serialization.
//   - Handles malformed presets gracefully — never crashes on bad data.
//
class PresetManager {
public:
    //--------------------------------------------------------------------------
    // Listener interface — notified when the active preset changes.
    struct Listener {
        virtual ~Listener() = default;
        virtual void presetLoaded(const PresetData& preset) = 0;
    };

    //--------------------------------------------------------------------------
    PresetManager() = default;

    //==========================================================================
    // Loading
    //==========================================================================

    // Load a preset from a .xometa file on disk.
    // Returns true on success, false if the file can't be read or is invalid.
    // Maximum .xometa file size (1 MB) — prevents OOM on corrupted/malicious files.
    static constexpr int64_t kMaxPresetFileSize = 1024 * 1024;

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
        dnaObj->setProperty("warmth",     static_cast<double>(preset.dna.warmth));
        dnaObj->setProperty("movement",   static_cast<double>(preset.dna.movement));
        dnaObj->setProperty("density",    static_cast<double>(preset.dna.density));
        dnaObj->setProperty("space",      static_cast<double>(preset.dna.space));
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

        return juce::JSON::toString(juce::var(root), false);
    }

    //==========================================================================
    // Library management
    //==========================================================================

    // Scan a directory (recursively) for .xometa files and add them to the library.
    void scanPresetDirectory(const juce::File& directory)
    {
        if (!directory.isDirectory())
            return;

        for (const auto& file :
             directory.findChildFiles(juce::File::findFiles, true, "*.xometa"))
        {
            if (file.getSize() > kMaxPresetFileSize)
                continue;

            auto jsonString = file.loadFileAsString();
            if (jsonString.isEmpty())
                continue;

            PresetData preset;
            if (parseJSON(jsonString, preset))
            {
                preset.sourceFile = file;
                library.push_back(std::move(preset));
            }
        }
    }

    // Return all presets matching a specific mood.
    std::vector<PresetData> getPresetsForMood(const juce::String& mood) const
    {
        std::vector<PresetData> results;
        for (const auto& p : library)
            if (p.mood == mood)
                results.push_back(p);
        return results;
    }

    // Return all presets containing a specific tag (case-insensitive).
    std::vector<PresetData> searchByTag(const juce::String& tag) const
    {
        std::vector<PresetData> results;
        auto tagLower = tag.toLowerCase();
        for (const auto& p : library)
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
        std::vector<PresetData> results;
        auto queryLower = query.toLowerCase();
        for (const auto& p : library)
            if (p.name.toLowerCase().contains(queryLower))
                results.push_back(p);
        return results;
    }

    // Return the full in-memory library.
    const std::vector<PresetData>& getLibrary() const { return library; }

    // Return the number of presets in the library.
    int getLibrarySize() const { return static_cast<int>(library.size()); }

    // Add a preset to the library (for testing and programmatic use).
    void addPreset(PresetData preset) { library.push_back(std::move(preset)); }

    //==========================================================================
    // DNA-powered features
    //==========================================================================

    // Euclidean distance in 6D DNA space.
    float dnaDistance(const PresetDNA& a, const PresetDNA& b) const
    {
        float db = a.brightness - b.brightness;
        float dw = a.warmth    - b.warmth;
        float dm = a.movement  - b.movement;
        float dd = a.density   - b.density;
        float ds = a.space     - b.space;
        float da = a.aggression - b.aggression;
        return std::sqrt(db*db + dw*dw + dm*dm + dd*dd + ds*ds + da*da);
    }

    // Find the `count` most similar presets by DNA distance.
    std::vector<PresetData> findSimilar(const PresetDNA& dna, int count = 5) const
    {
        return findNearest(dna, count);
    }

    // Find the `count` most opposite presets: invert the DNA vector
    // (1.0 - each dimension), then find nearest matches to the inverted point.
    std::vector<PresetData> findOpposite(const PresetDNA& dna, int count = 5) const
    {
        PresetDNA inverted;
        inverted.brightness = 1.0f - dna.brightness;
        inverted.warmth     = 1.0f - dna.warmth;
        inverted.movement   = 1.0f - dna.movement;
        inverted.density    = 1.0f - dna.density;
        inverted.space      = 1.0f - dna.space;
        inverted.aggression = 1.0f - dna.aggression;
        return findNearest(inverted, count);
    }

    //==========================================================================
    // Current preset
    //==========================================================================

    const PresetData& getCurrentPreset() const { return currentPreset; }

    void setCurrentPreset(const PresetData& preset)
    {
        currentPreset = preset;

        // Update navigation index if this preset exists in the library
        currentIndex = -1;
        for (int i = 0; i < static_cast<int>(library.size()); ++i)
        {
            if (library[static_cast<size_t>(i)].name == preset.name)
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
        if (library.empty())
            return;

        currentIndex = (currentIndex + 1) % static_cast<int>(library.size());
        currentPreset = library[static_cast<size_t>(currentIndex)];
        notifyListeners();
    }

    void previousPreset()
    {
        if (library.empty())
            return;

        currentIndex--;
        if (currentIndex < 0)
            currentIndex = static_cast<int>(library.size()) - 1;

        currentPreset = library[static_cast<size_t>(currentIndex)];
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
        listeners.erase(
            std::remove(listeners.begin(), listeners.end(), l),
            listeners.end());
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

        // name
        if (!obj->hasProperty("name"))
            return false;
        out.name = obj->getProperty("name").toString();
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
            if (validEngineNames.contains(engineName))
                out.engines.add(engineName);
        }
        if (out.engines.isEmpty())
            return false;
        // Cap at 3 engines
        while (out.engines.size() > 3)
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
                    if (validEngineNames.contains(engineName))
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
                    out.macroLabels.add(m.toString());
        }
        // Pad to 4 with defaults if needed
        static const juce::StringArray defaultMacros {
            "CHARACTER", "MOVEMENT", "COUPLING", "SPACE"
        };
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
                    out.dna.warmth     = clampDNA(dnaObj->getProperty("warmth"));
                    out.dna.movement   = clampDNA(dnaObj->getProperty("movement"));
                    out.dna.density    = clampDNA(dnaObj->getProperty("density"));
                    out.dna.space      = clampDNA(dnaObj->getProperty("space"));
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
                            cp.type    = pairObj->getProperty("type").toString();
                            cp.amount  = static_cast<float>(pairObj->getProperty("amount"));

                            // Validate: both engines must be known, type must be valid
                            if (validEngineNames.contains(cp.engineA)
                                && validEngineNames.contains(cp.engineB)
                                && validCouplingTypes.contains(cp.type))
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
        struct Ranked {
            float distance;
            size_t index;
        };

        std::vector<Ranked> ranked;
        ranked.reserve(library.size());

        for (size_t i = 0; i < library.size(); ++i)
            ranked.push_back({ dnaDistance(library[i].dna, target), i });

        std::sort(ranked.begin(), ranked.end(),
                  [](const Ranked& a, const Ranked& b) {
                      return a.distance < b.distance;
                  });

        std::vector<PresetData> results;
        int n = std::min(count, static_cast<int>(ranked.size()));
        for (int i = 0; i < n; ++i)
            results.push_back(library[ranked[static_cast<size_t>(i)].index]);

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
    std::vector<PresetData> library;
    PresetData currentPreset;
    int currentIndex = -1;
    std::vector<Listener*> listeners;
};

} // namespace xomnibus
