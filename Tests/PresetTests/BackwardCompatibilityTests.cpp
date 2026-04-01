/*
    XOlokun Backward Compatibility Regression Tests
    =================================================
    Tests that verify backward compatibility guarantees are not silently
    broken. Covers three critical areas:

    1. ENGINE ALIAS RESOLUTION
       resolveEngineAlias() must correctly map every known legacy engine name
       to its canonical O-prefix form. Breaking an alias silently breaks all
       legacy presets that use the old name — ~17,250 presets at risk.

    2. FROZEN PARAMETER ID PREFIXES
       frozenPrefixForEngine() must return the exact documented prefix for
       every engine. These are "frozen forever" — changing a prefix breaks
       every preset that uses it. Test verifies the frozen table has not
       drifted from the CLAUDE.md source of truth.

    3. COUPLING TYPE ENUM STABILITY
       CouplingType enum values are persisted in preset files as integers.
       Reordering entries silently corrupts all preset coupling configs.
       Test verifies each enum has the exact integer value it was first
       assigned at.

    No test framework required — assert-based with descriptive console output.

    See: https://github.com/BertCalm/XO_OX-XOmnibus/issues/459
*/

#include "BackwardCompatibilityTests.h"

#include "Core/PresetManager.h"
#include "Core/SynthEngine.h"

#include <juce_core/juce_core.h>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace xolokun;

namespace backward_compat_tests {

//==============================================================================
// Test infrastructure
//==============================================================================

static int g_passed = 0;
static int g_failed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        ++g_passed;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        ++g_failed;
    }
}

//==============================================================================
// 1. Engine alias regression tests
//
// For each legacy name, we hardcode the expected canonical form.
// If the mapping ever changes, this test catches it immediately.
//==============================================================================

static void testEngineAliases()
{
    std::cout << "\n--- Engine Alias Regression Tests ---\n";

    // Format: { legacy_name, expected_canonical }
    static const std::pair<const char*, const char*> kAliases[] = {
        // Short names (original rename batch)
        { "Snap",        "OddfeliX" },
        { "Morph",       "OddOscar" },
        { "Dub",         "Overdub"  },
        { "Drift",       "Odyssey"  },
        { "Bob",         "Oblong"   },
        { "Fat",         "Obese"    },
        { "Bite",        "Overbite" },
        // XO-prefixed instrument names (from standalone app era)
        { "XOddCouple",  "OddfeliX" },
        { "XOverdub",    "Overdub"  },
        { "XOdyssey",    "Odyssey"  },
        { "XOblong",     "Oblong"   },
        { "XOblongBob",  "Oblong"   },
        { "XObese",      "Obese"    },
        { "XOnset",      "Onset"    },
        { "XOrbital",    "Orbital"  },
        { "XOrganon",    "Organon"  },
        { "XOuroboros",  "Ouroboros"},
        { "XOpal",       "Opal"     },
        { "XOpossum",    "Overbite" },
        { "XOverbite",   "Overbite" },
        { "XObsidian",   "Obsidian" },
        { "XOrigami",    "Origami"  },
        { "XOracle",     "Oracle"   },
        { "XObscura",    "Obscura"  },
        { "XOceanic",    "Oceanic"  },
        { "XOptic",      "Optic"    },
        { "XOblique",    "Oblique"  },
        { "XOverworld",  "Overworld"},
        { "XOrca",       "Orca"     },
        { "XOctopus",    "Octopus"  },
        // Phase 4 aliases
        { "XOverlap",    "Overlap"  },
        { "XOutwit",     "Outwit"   },
    };

    for (auto& [legacy, expected] : kAliases)
    {
        auto result = resolveEngineAlias(legacy);
        bool ok = (result == juce::String(expected));

        // Build readable test name
        juce::String testName = juce::String("alias ") + legacy + " → " + expected;
        reportTest(testName.toRawUTF8(), ok);

        if (!ok)
        {
            std::cout << "    Expected: " << expected
                      << "  Got: " << result.toRawUTF8() << "\n";
        }
    }

    // Canonical names should pass through unchanged
    {
        juce::String canonical = "Origami";
        bool ok = (resolveEngineAlias(canonical) == canonical);
        reportTest("canonical name 'Origami' passes through unchanged", ok);
    }
    {
        juce::String canonical = "OddfeliX";
        bool ok = (resolveEngineAlias(canonical) == canonical);
        reportTest("canonical name 'OddfeliX' passes through unchanged", ok);
    }

    // Unknown names should pass through unchanged
    {
        juce::String unknown = "UnknownEngine2099";
        bool ok = (resolveEngineAlias(unknown) == unknown);
        reportTest("unknown engine name passes through unchanged", ok);
    }
}

//==============================================================================
// 2. Frozen parameter prefix tests
//
// These prefixes are frozen forever. The CLAUDE.md parameter prefix table
// is the authoritative source. Any discrepancy corrupts presets.
//==============================================================================

static void testFrozenParameterPrefixes()
{
    std::cout << "\n--- Frozen Parameter Prefix Regression Tests ---\n";

    // Format: { engine_id, expected_prefix }
    // The prefix may or may not have a trailing underscore — test matches exactly.
    static const std::pair<const char*, const char*> kPrefixes[] = {
        // Original fleet (no trailing underscore)
        { "OddfeliX",  "snap"     },
        { "OddOscar",  "morph"    },
        { "Overdub",   "dub"      },
        { "Odyssey",   "drift"    },
        { "Oblong",    "bob"      },
        { "Obese",     "fat"      },
        { "Overbite",  "poss"     },
        { "Onset",     "perc"     },
        { "Overworld", "ow"       },
        { "Opal",      "opal"     },
        { "Orbital",   "orb"      },
        { "Organon",   "organon"  },
        { "Ouroboros", "ouro"     },
        { "Obsidian",  "obsidian" },
        { "Origami",   "origami"  },
        { "Oracle",    "oracle"   },
        { "Obscura",   "obscura"  },
        { "Oceanic",   "ocean"    },
        { "Optic",     "optic"    },
        { "Oblique",   "oblq"     },
        { "Ocelot",    "ocelot"   },
        { "Osprey",    "osprey"   },
        { "Osteria",   "osteria"  },
        { "Owlfish",   "owl"      },
        { "Ohm",       "ohm"      },
        { "Orphica",   "orph"     },
        { "Obbligato", "obbl"     },
        { "Ottoni",    "otto"     },
        { "Ole",       "ole"      },
        { "Ombre",     "ombre"    },
        { "Orca",      "orca"     },
        { "Octopus",   "octo"     },
        { "Overlap",   "olap"     },
        { "Outwit",    "owit"     },
        // Concept engines
        { "OpenSky",   "sky"      },
        { "Ostinato",  "osti"     },
        { "OceanDeep", "deep"     },
        { "Ouie",      "ouie"     },
        // Flagship
        { "Obrix",     "obrix"    },
        // Theorem engines
        { "Orbweave",  "weave"    },
        { "Overtone",  "over"     },
        { "Organism",  "org"      },
        // Singularity
        { "Oxbow",     "oxb"      },
        { "Oware",     "owr"      },
        // Special collections
        { "Opera",     "opera"    },
        { "Offering",  "ofr"      },
        { "Osmosis",   "osmo_"    },
        { "Oxytocin",  "oxy_"     },
        { "Outlook",   "look_"    },
        // Chef Quad
        { "Oto",       "oto"      },
        { "Octave",    "oct"      },
        { "Oleg",      "oleg"     },
        { "Otis",      "otis"     },
        // Kitchen Quad (include trailing underscore)
        { "Oven",      "oven_"    },
        { "Ochre",     "ochre_"   },
        { "Obelisk",   "obel_"    },
        { "Opaline",   "opal2_"   },
        // Cellar Quad
        { "Ogre",      "ogre_"    },
        { "Olate",     "olate_"   },
        { "Oaken",     "oaken_"   },
        { "Omega",     "omega_"   },
        // Garden Quad
        { "Orchard",   "orch_"    },
        { "Overgrow",  "grow_"    },
        { "Osier",     "osier_"   },
        { "Oxalis",    "oxal_"    },
        // Broth Quad
        { "Overwash",  "wash_"    },
        { "Overworn",  "worn_"    },
        { "Overflow",  "flow_"    },
        { "Overcast",  "cast_"    },
        // Fusion Quad
        { "Oddfellow", "oddf_"    },
        { "Onkolo",    "onko_"    },
        { "Opcode",    "opco_"    },
        // Cellular automata
        { "Obiont",    "obnt_"    },
    };

    for (auto& [engine, expectedPrefix] : kPrefixes)
    {
        auto result = frozenPrefixForEngine(juce::String(engine));
        bool ok = (result == juce::String(expectedPrefix));

        juce::String testName = juce::String("prefix ") + engine + " = \"" + expectedPrefix + "\"";
        reportTest(testName.toRawUTF8(), ok);

        if (!ok)
        {
            std::cout << "    Expected: \"" << expectedPrefix
                      << "\"  Got: \"" << result.toRawUTF8() << "\"\n";
        }
    }

    // Unknown engine returns empty string (not a crash)
    {
        auto result = frozenPrefixForEngine("UnknownEngine2099");
        bool ok = result.isEmpty();
        reportTest("unknown engine prefix returns empty string", ok);
    }
}

//==============================================================================
// 3. CouplingType enum stability tests
//
// The integer values of CouplingType are persisted in preset files.
// They were assigned in this exact order and MUST NOT change.
// Any reordering silently corrupts all saved coupling configurations.
//==============================================================================

static void testCouplingTypeStability()
{
    std::cout << "\n--- CouplingType Enum Stability Tests ---\n";

    // These are the original integer ordinals (0-based, in declaration order).
    // If the enum is reordered or a value is inserted/removed before any of
    // these, the test will fail, alerting the developer before presets corrupt.
    static const std::pair<CouplingType, int> kExpectedOrdinals[] = {
        { CouplingType::AmpToFilter,      0  },
        { CouplingType::AmpToPitch,       1  },
        { CouplingType::LFOToPitch,       2  },
        { CouplingType::EnvToMorph,       3  },
        { CouplingType::AudioToFM,        4  },
        { CouplingType::AudioToRing,      5  },
        { CouplingType::FilterToFilter,   6  },
        { CouplingType::AmpToChoke,       7  },
        { CouplingType::RhythmToBlend,    8  },
        { CouplingType::EnvToDecay,       9  },
        { CouplingType::PitchToPitch,     10 },
        { CouplingType::AudioToWavetable, 11 },
        { CouplingType::AudioToBuffer,    12 },
        { CouplingType::KnotTopology,     13 },
        { CouplingType::TriangularCoupling, 14 },
    };

    static const char* kNames[] = {
        "AmpToFilter",
        "AmpToPitch",
        "LFOToPitch",
        "EnvToMorph",
        "AudioToFM",
        "AudioToRing",
        "FilterToFilter",
        "AmpToChoke",
        "RhythmToBlend",
        "EnvToDecay",
        "PitchToPitch",
        "AudioToWavetable",
        "AudioToBuffer",
        "KnotTopology",
        "TriangularCoupling",
    };

    int n = static_cast<int>(std::size(kExpectedOrdinals));
    for (int i = 0; i < n; ++i)
    {
        auto [type, expectedOrdinal] = kExpectedOrdinals[i];
        int actualOrdinal = static_cast<int>(type);
        bool ok = (actualOrdinal == expectedOrdinal);

        juce::String testName = juce::String("CouplingType::") + kNames[i]
                                + " == " + juce::String(expectedOrdinal);
        reportTest(testName.toRawUTF8(), ok);

        if (!ok)
        {
            std::cout << "    Expected ordinal: " << expectedOrdinal
                      << "  Actual: " << actualOrdinal << "\n";
        }
    }

    // Verify total coupling type count is exactly 15
    {
        // Cast the last enum + 1 to get the count via the enum size
        // (only works if enum values are 0-based sequential — which they are)
        int count = static_cast<int>(CouplingType::TriangularCoupling) + 1;
        bool ok = (count == 15);
        reportTest("CouplingType has exactly 15 values", ok);
    }
}

//==============================================================================
// Entry point
//==============================================================================

int runAll()
{
    g_passed = 0;
    g_failed = 0;

    std::cout << "========================================\n";
    std::cout << "  Backward Compatibility Regression Tests\n";
    std::cout << "========================================\n";

    testEngineAliases();
    testFrozenParameterPrefixes();
    testCouplingTypeStability();

    std::cout << "\n  Backward Compat Tests: " << g_passed << " passed, "
              << g_failed << " failed\n";

    return g_failed;
}

} // namespace backward_compat_tests
