#pragma once
#include <juce_core/juce_core.h>
#include <unordered_map>
#include <string>

//==============================================================================
// EngineVocabulary — per-engine parameter display label overrides.
//
// Overrides are SHORT (≤8 chars ideally, ≤9 chars max), ALL-CAPS, and
// mythologically appropriate for each engine's character identity.
//
// Falls back to `fallback` (or a cleaned-up version of paramId if fallback
// is empty: strip engine prefix, title-case last camelCase word, uppercase,
// truncate to 8 chars).
//
// DISPLAY ONLY — never stored or used in preset serialisation.
// Parameter IDs are frozen and must never be modified here.
//
// Usage:
//   juce::String label = EngineVocabulary::labelFor("Opera", "opera_drama", "DRAMA");
//==============================================================================

namespace xolokun {

struct EngineVocabulary
{
    // Returns a short display label for (engineId, paramId).
    // engineId  — engine's canonical ID (e.g. "Opera", "Organism")
    // paramId   — frozen APVTS parameter ID (e.g. "opera_drama")
    // fallback  — label to use when no override exists; if empty, a best-effort
    //             label is derived from paramId (strip prefix, uppercase last
    //             camelCase word, truncate to 8 chars).
    static juce::String labelFor (const juce::String& engineId,
                                  const juce::String& paramId,
                                  const juce::String& fallback = {})
    {
        // Check the override table — case-insensitive engine ID lookup.
        const auto& table = kOverrides();
        auto it = table.find (engineId.toLowerCase().toStdString());
        if (it != table.end())
        {
            auto jt = it->second.find (paramId.toStdString());
            if (jt != it->second.end())
                return juce::String (jt->second);
        }

        // No override found — use fallback or derive from paramId.
        if (fallback.isNotEmpty())
            return fallback;

        return derivedLabel (paramId);
    }

private:
    // Derive a best-effort label from a paramId when no override or fallback
    // is provided.  "opera_formantShift" → "FORMANT", "org_ruleSet" → "RULESET"
    //               "oxy_warmth_rate"   → "RATE"   (multi-underscore handled below)
    static juce::String derivedLabel (const juce::String& paramId)
    {
        // Strip prefix up to (and including) the first underscore.
        int under = paramId.indexOf ("_");
        juce::String inner = (under >= 0) ? paramId.substring (under + 1) : paramId;

        // Fix: if inner still contains an underscore (multi-segment IDs like
        // "warmth_rate"), take only the last segment so callers get "RATE"
        // rather than "WARMTH_RATE" (which would be truncated awkwardly).
        int lastUnder = inner.lastIndexOfChar ('_');
        if (lastUnder >= 0)
            inner = inner.substring (lastUnder + 1);

        // Extract last camelCase segment: find last uppercase transition.
        juce::String best = inner;
        for (int i = (int) inner.length() - 1; i >= 1; --i)
        {
            if (juce::CharacterFunctions::isUpperCase (inner[i]))
            {
                best = inner.substring (i);
                break;
            }
        }

        return best.toUpperCase().substring (0, 8);
    }

    // Lazy-initialised override table: engineId (lowercase) → paramId → label.
    // Add new engine blocks here as vocabulary is defined.
    static const std::unordered_map<std::string,
                                    std::unordered_map<std::string, std::string>>&
    kOverrides()
    {
        static const std::unordered_map<
            std::string, std::unordered_map<std::string, std::string>> table =
        {
            //------------------------------------------------------------------
            // OPERA (prefix: opera_)  — Aria Gold #D4AF37
            // Additive-vocal Kuramoto synchronicity engine
            //------------------------------------------------------------------
            { "opera", {
                { "opera_drama",             "DRAMA"   },
                { "opera_voiceCount",        "VOICES"  },
                { "opera_chorusSync",        "CHORUS"  },
                { "opera_stageWidth",        "STAGE"   },
                { "opera_arcMode",           "CONDUCT" },
                { "opera_conductorTimescale","ARC"     },
                { "opera_conductorPeak",     "PEAK"    },
                { "opera_conductorJitter",   "JITTER"  },
                { "opera_formantShift",      "FORMANT" },
                { "opera_vibrato",           "VIBRATO" },
                // Macros
                { "opera_macroCharacter",    "DRAMA"   },
                { "opera_macroMovement",     "VOICE"   },
                { "opera_macroCoupling",     "CHORUS"  },
                { "opera_macroSpace",        "STAGE"   },
            }},

            //------------------------------------------------------------------
            // ORGANISM (prefix: org_)  — Emergence Lime #C6E377
            // Cellular automata generative engine (Coral Colony)
            //------------------------------------------------------------------
            { "organism", {
                { "org_ruleSet",         "GENOME"  },
                { "org_cellSize",        "CELL"    },
                { "org_mutationRate",    "MUTATE"  },
                { "org_density",         "COLONY"  },
                { "org_generation",      "GEN"     },
                // Macros
                { "org_macroCharacter",  "RULES"   },
                { "org_macroMovement",   "GROW"    },
                { "org_macroCoupling",   "COLONY"  },
                { "org_macroSpace",      "SPACE"   },
            }},

            //------------------------------------------------------------------
            // OWARE (prefix: owr_)  — Akan Goldweight #B5883E
            // Tuned percussion (material continuum + mallet physics)
            //------------------------------------------------------------------
            { "oware", {
                { "owr_material",        "MATTER"  },
                { "owr_mallet",          "STRIKE"  },
                { "owr_sympathetic",     "RESONATE"},
                { "owr_tuningMode",      "TUNING"  },
                { "owr_buzzAmount",      "BUZZ"    },
                // Macros
                { "owr_macroCharacter",  "MATTER"  },
                { "owr_macroMovement",   "STRIKE"  },
                { "owr_macroCoupling",   "SYMPATH" },
                { "owr_macroSpace",      "SPACE"   },
            }},

            //------------------------------------------------------------------
            // OWLFISH (prefix: owl_)  — Abyssal Gold #B8860B
            // Mixtur-Trautonium + bioluminescent sub-harmonic engine
            // (triple MIX params, double THRESHOLD — all disambiguated)
            //------------------------------------------------------------------
            { "owlfish", {
                { "owl_subMix",           "SUBMIX"  },
                { "owl_fundWave",         "FUNDWAV" },
                { "owl_subWave",          "SUBWAVE" },
                { "owl_bodyFreq",         "BODYFRQ" },
                { "owl_bodyLevel",        "BODYLVL" },
                { "owl_compRatio",        "CMPRAT"  },
                { "owl_compThreshold",    "CMPTHRS" },
                { "owl_compAttack",       "CMPATTK" },
                { "owl_compRelease",      "CMPRLS"  },
                { "owl_filterEnvDepth",   "ENVDPTH" },
                { "owl_grainSize",        "GRNSZ"   },
                { "owl_grainMix",         "GRNMIX"  },
                { "owl_feedRate",         "FEEDRT"  },
                { "owl_armorThreshold",   "ARMRTHS" },
                { "owl_armorDecay",       "ARMRDCY" },
                { "owl_reverbSize",       "VERBSZ"  },
                { "owl_reverbPreDelay",   "PREDLY"  },
                { "owl_reverbMix",        "VERBMIX" },
                { "owl_legatoMode",       "LEGATO"  },
            }},

            //------------------------------------------------------------------
            // OXBOW (prefix: oxb_)  — Oxbow Teal #1A6B5A
            // Entangled reverb synth (Chiasmus FDN + phase erosion)
            //------------------------------------------------------------------
            { "oxbow", {
                { "oxb_entangle",        "ENTANGL" },
                { "oxb_chiasmus",        "CHIASM"  },
                { "oxb_erosion",         "ERODE"   },
                { "oxb_resonance",       "GOLDEN"  },
                // Macros
                { "oxb_macroCharacter",  "ENTANGL" },
                { "oxb_macroMovement",   "ERODE"   },
                { "oxb_macroCoupling",   "CHIASM"  },
                { "oxb_macroSpace",      "SPACE"   },
            }},

            //------------------------------------------------------------------
            // OXYTOCIN (prefix: oxy_)  — Synapse Violet #9B5DE5
            // Circuit-modeling love-triangle synth (fleet leader, 9.5/10)
            // Note Duration as Synthesis Parameter (B040)
            //------------------------------------------------------------------
            { "oxytocin", {
                { "oxy_intimacy",          "INTIMCY" },
                { "oxy_passion",           "PASSION" },
                { "oxy_commitment",        "COMMIT"  },
                { "oxy_warmth_rate",       "WARMTH"  },
                { "oxy_passion_rate",      "FERVOR"  },
                { "oxy_commit_rate",       "BONDING" },
                { "oxy_entanglement",      "ENTANGL" },
                { "oxy_circuit_age",       "VINTAGE" },
                { "oxy_circuit_noise",     "HISS"    },
                { "oxy_memory_depth",      "RECALL"  },
                { "oxy_memory_decay",      "FORGET"  },
                { "oxy_topology",          "TOPOLOGY"},
                { "oxy_topology_lock",     "LOCK"    },
                { "oxy_lfo_rate",          "LFO1 RT" },
                { "oxy_lfo_depth",         "LFO1 DP" },
                { "oxy_lfo_shape",         "LFO1 SH" },
                { "oxy_lfo2_rate",         "LFO2 RT" },
                { "oxy_lfo2_depth",        "LFO2 DP" },
                // Macros: Oxytocin has no oxy_macro* params (fixes #306).
                // The four expressive controls above ARE the M1-M4 macros:
                //   M1 CHARACTER = oxy_intimacy   (already mapped -> "INTIMCY")
                //   M2 MOVEMENT  = oxy_passion    (already mapped -> "PASSION")
                //   M3 COUPLING  = oxy_commitment (already mapped -> "COMMIT")
                //   M4 SPACE     = oxy_entanglement (already mapped -> "ENTANGL")
            }},

            //------------------------------------------------------------------
            // OVERWORLD (prefix: ow_)  — Neon Green #39FF14
            // Chip synth (NES/Genesis/SNES), ERA Triangle
            //------------------------------------------------------------------
            { "overworld", {
                { "ow_era",              "ERA"     },
                // Macros -- keyed on actual registered param IDs (fixes #305).
                // Overworld registers ow_macroEra/Crush/Glitch/Space, not the
                // generic macroCharacter/Movement/Coupling/Space names.
                { "ow_macroEra",         "ERA"     },  // M1 CHARACTER
                { "ow_macroCrush",       "CRUSH"   },  // M2 MOVEMENT
                { "ow_macroGlitch",      "GLITCH"  },  // M3 COUPLING
                { "ow_macroSpace",       "SPACE"   },  // M4 SPACE
            }},

            //------------------------------------------------------------------
            // OBESE (prefix: fat_)  — Hot Pink #FF1493
            // Saturation / lo-fi bass synth (Mojo analog-digital axis)
            //------------------------------------------------------------------
            { "obese", {
                { "fat_subLevel",        "SUBLVL"  },
                { "fat_groupMix",        "GRPMIX"  },
                { "fat_fltKeyTrack",     "KEYTRCK" },
                { "fat_fltEnvAmt",       "FLTENV"  },
                { "fat_fltEnvAttack",    "FENVATT" },
                { "fat_fltEnvDecay",     "FENVDCY" },
                { "fat_satDrive",        "SATDRV"  },
                { "fat_crushDepth",      "CRSHBIT" },
                { "fat_crushRate",       "CRSHRT"  },
                { "fat_lfo1Rate",        "LFO1 RT" },
                { "fat_lfo1Depth",       "LFO1 DP" },
                { "fat_lfo2Rate",        "LFO2 RT" },
                { "fat_lfo2Depth",       "LFO2 DP" },
                { "fat_arpRate",         "ARPRATE" },
            }},

            //------------------------------------------------------------------
            // OBSCURA (prefix: obscura_)  — Daguerreotype Silver #8A9BA8
            // Bowed/resonant physical model (misleading SUSTAIN = bow force)
            //------------------------------------------------------------------
            { "obscura", {
                { "obscura_sustain",         "BOWFRC"  },
                { "obscura_excitePos",        "EXCPOS"  },
                { "obscura_exciteWidth",      "EXCWDTH" },
                { "obscura_scanWidth",        "SCANWDT" },
                { "obscura_physEnvAttack",    "PHYSATT" },
                { "obscura_physEnvDecay",     "PHYSDCY" },
                { "obscura_physEnvSustain",   "PHYSSUS" },
                { "obscura_physEnvRelease",   "PHYSREL" },
                { "obscura_lfo1Rate",         "LFO1 RT" },
                { "obscura_lfo1Depth",        "LFO1 DP" },
                { "obscura_lfo2Rate",         "LFO2 RT" },
                { "obscura_lfo2Depth",        "LFO2 DP" },
                { "obscura_lfo2Shape",        "LFO2 SH" },
                { "obscura_initShape",        "INITSHP" },
                // Macros
                { "obscura_macroCharacter",   "SILVER"  },
                { "obscura_macroMovement",    "GRAIN"   },
                { "obscura_macroCoupling",    "STIFF"   },
                { "obscura_macroSpace",       "ABYSS"   },
            }},

            //------------------------------------------------------------------
            // OCELOT (prefix: ocelot_)  — Ocelot Tawny #C5832B
            // Cross-feed matrix engine (forest-layer biome routing)
            //------------------------------------------------------------------
            { "ocelot", {
                { "ocelot_xf_floorUnder",         "F>UNDER" },
                { "ocelot_xf_floorCanopy",         "F>CANOP" },
                { "ocelot_xf_floorEmerg",          "F>EMERG" },
                { "ocelot_xf_underFloor",          "U>FLOOR" },
                { "ocelot_xf_underCanopy",         "U>CANOP" },
                { "ocelot_xf_underEmerg",          "U>EMERG" },
                { "ocelot_xf_canopyFloor",         "C>FLOOR" },
                { "ocelot_xf_canopyUnder",         "C>UNDER" },
                { "ocelot_xf_canopyEmerg",         "C>EMERG" },
                { "ocelot_xf_emergFloor",          "E>FLOOR" },
                { "ocelot_xf_emergUnder",          "E>UNDER" },
                { "ocelot_xf_emergCanopy",         "E>CANOP" },
                { "ocelot_floorLevel",             "FLRLVL"  },
                { "ocelot_chopRate",               "CHOPRT"  },
                { "ocelot_bitDepth",               "BITDPTH" },
                { "ocelot_sampleRate",             "SMPRATE" },
                { "ocelot_dustLevel",              "DUSTLVL" },
                { "ocelot_understoryLevel",        "UNDRLVL" },
                { "ocelot_canopyLevel",            "CNPYLVL" },
                { "ocelot_canopySpectralFilter",   "SPECTRL" },
                { "ocelot_creatureType",           "CRTYPE"  },
                { "ocelot_creatureRate",           "CRRATE"  },
                { "ocelot_creatureAttack",         "CRATTK"  },
                { "ocelot_creatureDecay",          "CRDECAY" },
                { "ocelot_filterEnvDepth",         "FLTENVD" },
                { "ocelot_ecosystemDepth",         "ECODPTH" },
            }},

            //------------------------------------------------------------------
            // OCEANDEEP (prefix: deep_)  — Trench Violet #2D0A4E
            // Deep-ocean pressure synthesis (Hydrostatic + Bioluminescent)
            //------------------------------------------------------------------
            { "oceandeep", {
                { "deep_macroPressure",  "DEPTH"   },
                // Macros
                { "deep_macroCharacter", "BIOLUM"  },
                { "deep_macroMovement",  "DRIFT"   },
                { "deep_macroCoupling",  "PRESSURE"},
                { "deep_macroSpace",     "ABYSS"   },
            }},

            //------------------------------------------------------------------
            // OFFERING (prefix: ofr_)  — Crate Wax Yellow #E5B80B
            // Psychology-driven boom bap drum synthesis (Berlyne curiosity)
            //------------------------------------------------------------------
            { "offering", {
                { "ofr_digCuriosity",    "CURIOSITY"},
                { "ofr_cityMode",        "CITY"    },
                // Macros
                { "ofr_macroCharacter",  "CURIOUS" },
                { "ofr_macroMovement",   "CITY"    },
                { "ofr_macroCoupling",   "GROOVE"  },
                { "ofr_macroSpace",      "SPACE"   },
            }},
        };
        return table;
    }

    JUCE_DECLARE_NON_COPYABLE (EngineVocabulary)
};

} // namespace xolokun
