/*
    XOmnibus Preset Round-Trip Tests
    =================================
    Tests for PresetManager: parsing, serialization, validation, DNA search.
    No test framework — assert-based with descriptive console output.
*/

#include "PresetRoundTripTests.h"

#include "Core/PresetManager.h"

#include <juce_core/juce_core.h>
#include <iostream>
#include <string>
#include <cmath>

using namespace xomnibus;

//==============================================================================
// Test infrastructure
//==============================================================================

static int g_presetTestsPassed = 0;
static int g_presetTestsFailed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        g_presetTestsPassed++;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        g_presetTestsFailed++;
    }
}

//==============================================================================
// Valid test JSON strings
//
// NOTE: These JSON presets intentionally use legacy engine names (Snap, Morph,
// Dub, etc.) to verify backward compatibility. The PresetManager's
// resolveEngineAlias() must resolve these to canonical O-prefix names
// (OddfeliX, OddOscar, Overdub, etc.) on load. Do NOT update the engine
// names inside these JSON strings.
//==============================================================================

static const char* kValidPresetJSON = R"({
    "schema_version": 1,
    "name": "Test Preset",
    "mood": "Foundation",
    "engines": ["Snap"],
    "author": "TestAuthor",
    "version": "1.0.0",
    "description": "A test preset for unit testing.",
    "tags": ["test", "unit"],
    "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "couplingIntensity": "Moderate",
    "tempo": 120.0,
    "dna": {
        "brightness": 0.7,
        "warmth": 0.3,
        "movement": 0.5,
        "density": 0.8,
        "space": 0.2,
        "aggression": 0.1
    },
    "parameters": {
        "Snap": {
            "snap_snap": 0.4,
            "snap_decay": 0.5,
            "snap_filterCutoff": 2000.0
        }
    },
    "coupling": {
        "pairs": [
            {
                "engineA": "Snap",
                "engineB": "Morph",
                "type": "Amp->Filter",
                "amount": 0.6
            }
        ]
    },
    "sequencer": null
})";

static const char* kMinimalValidJSON = R"({
    "schema_version": 1,
    "name": "Minimal",
    "mood": "Foundation",
    "engines": ["Snap"],
    "parameters": {
        "Snap": {}
    }
})";

//==============================================================================
// Parsing tests
//==============================================================================

static void testParsing()
{
    std::cout << "\n--- Preset Parsing ---\n";

    // Parse valid JSON: all fields populated
    {
        PresetManager pm;
        bool loaded = pm.loadPresetFromJSON(juce::String(kValidPresetJSON));
        reportTest("Parse valid JSON: returns true", loaded);

        if (loaded)
        {
            const auto& p = pm.getCurrentPreset();
            reportTest("Parse: name correct", p.name == "Test Preset");
            reportTest("Parse: mood correct", p.mood == "Foundation");
            reportTest("Parse: engines correct", p.engines.size() == 1 && p.engines[0] == "OddfeliX");
            reportTest("Parse: author correct", p.author == "TestAuthor");
            reportTest("Parse: version correct", p.version == "1.0.0");
            reportTest("Parse: description correct", p.description == "A test preset for unit testing.");
            reportTest("Parse: tags correct", p.tags.size() == 2);
            reportTest("Parse: macroLabels correct", p.macroLabels.size() == 4);
            reportTest("Parse: couplingIntensity correct", p.couplingIntensity == "Moderate");
            reportTest("Parse: tempo correct", std::abs(p.tempo - 120.0f) < 0.01f);
            reportTest("Parse: DNA brightness", std::abs(p.dna.brightness - 0.7f) < 0.01f);
            reportTest("Parse: DNA warmth", std::abs(p.dna.warmth - 0.3f) < 0.01f);
            reportTest("Parse: DNA movement", std::abs(p.dna.movement - 0.5f) < 0.01f);
            reportTest("Parse: DNA density", std::abs(p.dna.density - 0.8f) < 0.01f);
            reportTest("Parse: DNA space", std::abs(p.dna.space - 0.2f) < 0.01f);
            reportTest("Parse: DNA aggression", std::abs(p.dna.aggression - 0.1f) < 0.01f);
            reportTest("Parse: parameters has OddfeliX engine",
                       p.parametersByEngine.count("OddfeliX") > 0);
            reportTest("Parse: coupling pairs correct",
                       p.couplingPairs.size() == 1
                       && p.couplingPairs[0].engineA == "OddfeliX"
                       && p.couplingPairs[0].engineB == "OddOscar"
                       && p.couplingPairs[0].type == "Amp->Filter"
                       && std::abs(p.couplingPairs[0].amount - 0.6f) < 0.01f);
        }
    }
}

//==============================================================================
// Round-trip tests
//==============================================================================

static void testRoundTrip()
{
    std::cout << "\n--- Preset Round-Trip ---\n";

    // Serialize then parse back: all fields match
    {
        PresetManager pm;
        pm.loadPresetFromJSON(juce::String(kValidPresetJSON));
        const auto& original = pm.getCurrentPreset();

        // Serialize
        juce::String json = pm.serializeToJSON(original);
        reportTest("Round-trip: serialize produces non-empty JSON", json.isNotEmpty());

        // Parse back
        PresetManager pm2;
        bool reloaded = pm2.loadPresetFromJSON(json);
        reportTest("Round-trip: re-parse succeeds", reloaded);

        if (reloaded)
        {
            const auto& p = pm2.getCurrentPreset();
            reportTest("Round-trip: name matches", p.name == original.name);
            reportTest("Round-trip: mood matches", p.mood == original.mood);
            reportTest("Round-trip: engines match",
                       p.engines.size() == original.engines.size()
                       && p.engines[0] == original.engines[0]);
            reportTest("Round-trip: author matches", p.author == original.author);
            reportTest("Round-trip: version matches", p.version == original.version);
            reportTest("Round-trip: description matches",
                       p.description == original.description);
            reportTest("Round-trip: tags match",
                       p.tags.size() == original.tags.size());
            reportTest("Round-trip: macroLabels match",
                       p.macroLabels.size() == original.macroLabels.size()
                       && p.macroLabels[0] == original.macroLabels[0]);
            reportTest("Round-trip: couplingIntensity matches",
                       p.couplingIntensity == original.couplingIntensity);
            reportTest("Round-trip: tempo matches",
                       std::abs(p.tempo - original.tempo) < 0.01f);
            reportTest("Round-trip: DNA matches",
                       std::abs(p.dna.brightness - original.dna.brightness) < 0.01f
                       && std::abs(p.dna.warmth - original.dna.warmth) < 0.01f
                       && std::abs(p.dna.movement - original.dna.movement) < 0.01f
                       && std::abs(p.dna.density - original.dna.density) < 0.01f
                       && std::abs(p.dna.space - original.dna.space) < 0.01f
                       && std::abs(p.dna.aggression - original.dna.aggression) < 0.01f);
            reportTest("Round-trip: coupling pairs match",
                       p.couplingPairs.size() == original.couplingPairs.size());
        }
    }
}

//==============================================================================
// Validation tests
//==============================================================================

static void testValidation()
{
    std::cout << "\n--- Preset Validation ---\n";

    // Missing required field: name
    {
        const char* json = R"({
            "schema_version": 1,
            "mood": "Foundation",
            "engines": ["Snap"],
            "parameters": { "Snap": {} }
        })";
        PresetManager pm;
        bool loaded = pm.loadPresetFromJSON(juce::String(json));
        reportTest("Missing name: returns false", !loaded);
    }

    // Missing required field: engines
    {
        const char* json = R"({
            "schema_version": 1,
            "name": "Test",
            "mood": "Foundation",
            "parameters": { "Snap": {} }
        })";
        PresetManager pm;
        bool loaded = pm.loadPresetFromJSON(juce::String(json));
        reportTest("Missing engines: returns false", !loaded);
    }

    // Missing required field: parameters
    {
        const char* json = R"({
            "schema_version": 1,
            "name": "Test",
            "mood": "Foundation",
            "engines": ["Snap"]
        })";
        PresetManager pm;
        bool loaded = pm.loadPresetFromJSON(juce::String(json));
        reportTest("Missing parameters: returns false", !loaded);
    }

    // Invalid mood defaults to "User"
    {
        const char* json = R"({
            "schema_version": 1,
            "name": "MoodTest",
            "mood": "InvalidMood",
            "engines": ["Snap"],
            "parameters": { "Snap": {} }
        })";
        PresetManager pm;
        bool loaded = pm.loadPresetFromJSON(juce::String(json));
        reportTest("Invalid mood: still loads", loaded);
        if (loaded)
            reportTest("Invalid mood: defaults to User",
                       pm.getCurrentPreset().mood == "User");
    }

    // Invalid engine names are filtered out
    {
        const char* json = R"({
            "schema_version": 1,
            "name": "EngineFilter",
            "mood": "Foundation",
            "engines": ["Snap", "NotAnEngine", "Morph"],
            "parameters": { "Snap": {}, "Morph": {} }
        })";
        PresetManager pm;
        bool loaded = pm.loadPresetFromJSON(juce::String(json));
        reportTest("Invalid engine names: still loads", loaded);
        if (loaded)
        {
            const auto& engines = pm.getCurrentPreset().engines;
            bool noInvalid = !engines.contains("NotAnEngine");
            bool hasValid = engines.contains("OddfeliX") && engines.contains("OddOscar");
            reportTest("Invalid engine names: filtered out", noInvalid && hasValid);
        }
    }

    // All engines invalid: returns false
    {
        const char* json = R"({
            "schema_version": 1,
            "name": "AllInvalid",
            "mood": "Foundation",
            "engines": ["FakeEngine1", "FakeEngine2"],
            "parameters": {}
        })";
        PresetManager pm;
        bool loaded = pm.loadPresetFromJSON(juce::String(json));
        reportTest("All invalid engines: returns false", !loaded);
    }

    // Coupling pairs with invalid types are filtered out
    {
        const char* json = R"({
            "schema_version": 1,
            "name": "CouplingFilter",
            "mood": "Foundation",
            "engines": ["Snap"],
            "parameters": { "Snap": {} },
            "coupling": {
                "pairs": [
                    {
                        "engineA": "Snap",
                        "engineB": "Morph",
                        "type": "InvalidType",
                        "amount": 0.5
                    },
                    {
                        "engineA": "Snap",
                        "engineB": "Morph",
                        "type": "Amp->Filter",
                        "amount": 0.6
                    }
                ]
            }
        })";
        PresetManager pm;
        bool loaded = pm.loadPresetFromJSON(juce::String(json));
        reportTest("Invalid coupling type: preset loads", loaded);
        if (loaded)
        {
            const auto& pairs = pm.getCurrentPreset().couplingPairs;
            reportTest("Invalid coupling type: filtered out (1 remains)",
                       pairs.size() == 1 && pairs[0].type == "Amp->Filter");
        }
    }

    // macroLabels padded to exactly 4
    {
        const char* json = R"({
            "schema_version": 1,
            "name": "MacroPad",
            "mood": "Foundation",
            "engines": ["Snap"],
            "parameters": { "Snap": {} },
            "macroLabels": ["CUSTOM1"]
        })";
        PresetManager pm;
        bool loaded = pm.loadPresetFromJSON(juce::String(json));
        reportTest("macroLabels: preset loads with 1 label", loaded);
        if (loaded)
        {
            const auto& labels = pm.getCurrentPreset().macroLabels;
            reportTest("macroLabels: padded to exactly 4", labels.size() == 4);
            reportTest("macroLabels: first label preserved", labels[0] == "CUSTOM1");
            reportTest("macroLabels: defaults for rest",
                       labels[1] == "MOVEMENT" && labels[2] == "COUPLING" && labels[3] == "SPACE");
        }
    }

    // macroLabels trimmed to exactly 4
    {
        const char* json = R"({
            "schema_version": 1,
            "name": "MacroTrim",
            "mood": "Foundation",
            "engines": ["Snap"],
            "parameters": { "Snap": {} },
            "macroLabels": ["A", "B", "C", "D", "E", "F"]
        })";
        PresetManager pm;
        bool loaded = pm.loadPresetFromJSON(juce::String(json));
        reportTest("macroLabels: preset loads with 6 labels", loaded);
        if (loaded)
        {
            reportTest("macroLabels: trimmed to exactly 4",
                       pm.getCurrentPreset().macroLabels.size() == 4);
        }
    }

    // Empty JSON string
    {
        PresetManager pm;
        bool loaded = pm.loadPresetFromJSON("");
        reportTest("Empty JSON string: returns false", !loaded);
    }

    // Garbage JSON
    {
        PresetManager pm;
        bool loaded = pm.loadPresetFromJSON("not valid json at all");
        reportTest("Garbage JSON: returns false", !loaded);
    }

    // schema_version < 1
    {
        const char* json = R"({
            "schema_version": 0,
            "name": "BadVersion",
            "mood": "Foundation",
            "engines": ["Snap"],
            "parameters": { "Snap": {} }
        })";
        PresetManager pm;
        bool loaded = pm.loadPresetFromJSON(juce::String(json));
        reportTest("schema_version < 1: returns false", !loaded);
    }
}

//==============================================================================
// DNA distance and search tests
//==============================================================================

static void testDNASearch()
{
    std::cout << "\n--- DNA Distance & Search ---\n";

    PresetManager pm;

    // distance(a, a) = 0
    {
        PresetDNA a;
        a.brightness = 0.5f;
        a.warmth = 0.3f;
        a.movement = 0.7f;
        a.density = 0.1f;
        a.space = 0.9f;
        a.aggression = 0.4f;

        float d = pm.dnaDistance(a, a);
        reportTest("DNA distance(a, a) = 0", d == 0.0f);
    }

    // distance(a, b) > 0 when a != b
    {
        PresetDNA a, b;
        a.brightness = 0.1f; a.warmth = 0.2f; a.movement = 0.3f;
        a.density = 0.4f; a.space = 0.5f; a.aggression = 0.6f;
        b.brightness = 0.9f; b.warmth = 0.8f; b.movement = 0.7f;
        b.density = 0.6f; b.space = 0.5f; b.aggression = 0.4f;

        float d = pm.dnaDistance(a, b);
        reportTest("DNA distance(a, b) > 0 when a != b", d > 0.0f);
    }

    // findSimilar returns presets sorted by distance
    {
        PresetManager pm2;

        // Build a small library with known DNA values
        auto makePreset = [](const char* name, float brightness) -> PresetData {
            PresetData p;
            p.schemaVersion = 1;
            p.name = name;
            p.mood = "Foundation";
            p.engines.add("Snap");
            p.dna.brightness = brightness;
            p.dna.warmth = 0.5f;
            p.dna.movement = 0.5f;
            p.dna.density = 0.5f;
            p.dna.space = 0.5f;
            p.dna.aggression = 0.5f;
            return p;
        };

        pm2.addPreset(makePreset("Far", 0.0f));
        pm2.addPreset(makePreset("Close", 0.8f));
        pm2.addPreset(makePreset("Closest", 0.9f));
        pm2.addPreset(makePreset("Medium", 0.5f));

        PresetDNA target;
        target.brightness = 1.0f;
        target.warmth = 0.5f;
        target.movement = 0.5f;
        target.density = 0.5f;
        target.space = 0.5f;
        target.aggression = 0.5f;

        auto similar = pm2.findSimilar(target, 4);
        reportTest("findSimilar: returns correct count", similar.size() == 4);
        if (similar.size() == 4)
        {
            reportTest("findSimilar: sorted by distance (closest first)",
                       similar[0].name == "Closest"
                       && similar[1].name == "Close");
        }
    }

    // findOpposite returns presets near the inverted DNA
    {
        PresetManager pm2;

        auto makePreset = [](const char* name, float brightness) -> PresetData {
            PresetData p;
            p.schemaVersion = 1;
            p.name = name;
            p.mood = "Foundation";
            p.engines.add("Snap");
            p.dna.brightness = brightness;
            p.dna.warmth = 0.5f;
            p.dna.movement = 0.5f;
            p.dna.density = 0.5f;
            p.dna.space = 0.5f;
            p.dna.aggression = 0.5f;
            return p;
        };

        pm2.addPreset(makePreset("Bright", 0.9f));
        pm2.addPreset(makePreset("Dark", 0.1f));
        pm2.addPreset(makePreset("Mid", 0.5f));

        // Target is bright -> opposite should be dark
        PresetDNA target;
        target.brightness = 0.9f;
        target.warmth = 0.5f;
        target.movement = 0.5f;
        target.density = 0.5f;
        target.space = 0.5f;
        target.aggression = 0.5f;

        auto opposite = pm2.findOpposite(target, 3);
        reportTest("findOpposite: returns results", !opposite.empty());
        if (!opposite.empty())
        {
            // Inverted brightness = 0.1, so "Dark" (0.1) should be closest
            reportTest("findOpposite: dark is closest to inverted bright",
                       opposite[0].name == "Dark");
        }
    }

    // Empty library searches return empty
    {
        PresetManager pm3;
        PresetDNA target;
        auto similar = pm3.findSimilar(target, 5);
        auto opposite = pm3.findOpposite(target, 5);
        reportTest("Empty library: findSimilar returns empty", similar.empty());
        reportTest("Empty library: findOpposite returns empty", opposite.empty());
    }
}

//==============================================================================
// File size tests
//==============================================================================

static void testFileSize()
{
    std::cout << "\n--- File Size Limits ---\n";

    // Verify the constant is 1MB
    {
        reportTest("kMaxPresetFileSize = 1MB",
                   PresetManager::kMaxPresetFileSize == 1024 * 1024);
    }

    // Create a temp file larger than 1MB and try to load it
    {
        auto tempDir = juce::File::getSpecialLocation(
            juce::File::tempDirectory);
        auto bigFile = tempDir.getChildFile("xomnibus_test_big.xometa");

        // Write >1MB of data
        juce::String bigContent;
        bigContent.preallocateBytes(1024 * 1024 + 100);
        bigContent = "{\"schema_version\":1,\"name\":\"Big\",\"mood\":\"Foundation\","
                     "\"engines\":[\"Snap\"],\"parameters\":{\"Snap\":{\"data\":\"";
        // Pad with enough characters to exceed 1MB
        for (int i = 0; i < 1024 * 1024; ++i)
            bigContent += "x";
        bigContent += "\"}}}";

        bigFile.replaceWithText(bigContent);

        PresetManager pm;
        bool loaded = pm.loadPresetFromFile(bigFile);
        reportTest("File > 1MB is rejected", !loaded);

        // Clean up
        bigFile.deleteFile();
    }
}

//==============================================================================
// Public entry point
//==============================================================================

namespace preset_tests {

int runAll()
{
    g_presetTestsPassed = 0;
    g_presetTestsFailed = 0;

    std::cout << "\n========================================\n";
    std::cout << "  Preset Round-Trip Tests\n";
    std::cout << "========================================\n";

    testParsing();
    testRoundTrip();
    testValidation();
    testDNASearch();
    testFileSize();

    std::cout << "\n  Preset Tests: " << g_presetTestsPassed << " passed, "
              << g_presetTestsFailed << " failed\n";

    return g_presetTestsFailed;
}

} // namespace preset_tests
