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

namespace xomnibus {

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
    static juce::String derivedLabel (const juce::String& paramId)
    {
        // Strip prefix up to (and including) the first underscore.
        int under = paramId.indexOf ("_");
        juce::String inner = (under >= 0) ? paramId.substring (under + 1) : paramId;

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
                { "opera_conductorMode",     "CONDUCT" },
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
            // OVERWORLD (prefix: ow_)  — Neon Green #39FF14
            // Chip synth (NES/Genesis/SNES), ERA Triangle
            //------------------------------------------------------------------
            { "overworld", {
                { "ow_era",              "ERA"     },
                // Macros
                { "ow_macroCharacter",   "ERA"     },
                { "ow_macroMovement",    "PULSE"   },
                { "ow_macroCoupling",    "SYNC"    },
                { "ow_macroSpace",       "SPACE"   },
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

} // namespace xomnibus
