/*
    XOceanus Preset Round-Trip Tests
    =================================
    Tests for PresetManager: parsing, serialization, validation, DNA search.
    Migrated to Catch2 v3: issue #81
*/

#include "PresetRoundTripTests.h"

#include <catch2/catch_test_macros.hpp>

#include "Core/PresetManager.h"

#include <juce_core/juce_core.h>
#include <cmath>

using namespace xoceanus;

// NOTE: JSON presets intentionally use legacy engine names to verify backward
// compatibility. resolveEngineAlias() must map them to canonical O-prefix names.
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

//==============================================================================
// Parsing tests
//==============================================================================

TEST_CASE("PresetManager - parse valid JSON populates all fields", "[preset][parse]")
{
    PresetManager pm;
    REQUIRE(pm.loadPresetFromJSON(juce::String(kValidPresetJSON)));

    const auto& p = pm.getCurrentPreset();
    CHECK(p.name == "Test Preset");
    CHECK(p.mood == "Foundation");
    CHECK(p.engines.size() == 1);
    CHECK(p.engines[0] == "OddfeliX"); // alias resolved
    CHECK(p.author == "TestAuthor");
    CHECK(p.version == "1.0.0");
    CHECK(p.description == "A test preset for unit testing.");
    CHECK(p.tags.size() == 2);
    CHECK(p.macroLabels.size() == 4);
    CHECK(p.couplingIntensity == "Moderate");
    CHECK(std::abs(p.tempo - 120.0f) < 0.01f);
    CHECK(std::abs(p.dna.brightness - 0.7f) < 0.01f);
    CHECK(std::abs(p.dna.warmth - 0.3f) < 0.01f);
    CHECK(std::abs(p.dna.movement - 0.5f) < 0.01f);
    CHECK(std::abs(p.dna.density - 0.8f) < 0.01f);
    CHECK(std::abs(p.dna.space - 0.2f) < 0.01f);
    CHECK(std::abs(p.dna.aggression - 0.1f) < 0.01f);
    CHECK(p.parametersByEngine.count("OddfeliX") > 0);
    REQUIRE(p.couplingPairs.size() == 1);
    CHECK(p.couplingPairs[0].engineA == "OddfeliX");
    CHECK(p.couplingPairs[0].engineB == "OddOscar");
    CHECK(p.couplingPairs[0].type == "Amp->Filter");
    CHECK(std::abs(p.couplingPairs[0].amount - 0.6f) < 0.01f);
}

//==============================================================================
// Round-trip tests
//==============================================================================

TEST_CASE("PresetManager - serialize then re-parse produces identical fields", "[preset][roundtrip]")
{
    PresetManager pm;
    pm.loadPresetFromJSON(juce::String(kValidPresetJSON));
    const auto& original = pm.getCurrentPreset();

    juce::String json = pm.serializeToJSON(original);
    CHECK(json.isNotEmpty());

    PresetManager pm2;
    REQUIRE(pm2.loadPresetFromJSON(json));

    const auto& p = pm2.getCurrentPreset();
    CHECK(p.name == original.name);
    CHECK(p.mood == original.mood);
    CHECK(p.engines.size() == original.engines.size());
    CHECK(p.engines[0] == original.engines[0]);
    CHECK(p.author == original.author);
    CHECK(p.version == original.version);
    CHECK(p.description == original.description);
    CHECK(p.tags.size() == original.tags.size());
    CHECK(p.macroLabels.size() == original.macroLabels.size());
    CHECK(p.macroLabels[0] == original.macroLabels[0]);
    CHECK(p.couplingIntensity == original.couplingIntensity);
    CHECK(std::abs(p.tempo - original.tempo) < 0.01f);
    CHECK(std::abs(p.dna.brightness - original.dna.brightness) < 0.01f);
    CHECK(std::abs(p.dna.warmth - original.dna.warmth) < 0.01f);
    CHECK(std::abs(p.dna.movement - original.dna.movement) < 0.01f);
    CHECK(std::abs(p.dna.density - original.dna.density) < 0.01f);
    CHECK(std::abs(p.dna.space - original.dna.space) < 0.01f);
    CHECK(std::abs(p.dna.aggression - original.dna.aggression) < 0.01f);
    CHECK(p.couplingPairs.size() == original.couplingPairs.size());
}

//==============================================================================
// Validation tests
//==============================================================================

TEST_CASE("PresetManager - missing name returns false", "[preset][validation]")
{
    const char* json = R"({"schema_version":1,"mood":"Foundation","engines":["Snap"],"parameters":{"Snap":{}}})";
    PresetManager pm;
    CHECK(!pm.loadPresetFromJSON(juce::String(json)));
}

TEST_CASE("PresetManager - missing engines returns false", "[preset][validation]")
{
    const char* json = R"({"schema_version":1,"name":"T","mood":"Foundation","parameters":{"Snap":{}}})";
    PresetManager pm;
    CHECK(!pm.loadPresetFromJSON(juce::String(json)));
}

TEST_CASE("PresetManager - missing parameters returns false", "[preset][validation]")
{
    const char* json = R"({"schema_version":1,"name":"T","mood":"Foundation","engines":["Snap"]})";
    PresetManager pm;
    CHECK(!pm.loadPresetFromJSON(juce::String(json)));
}

TEST_CASE("PresetManager - invalid mood defaults to User", "[preset][validation]")
{
    const char* json =
        R"({"schema_version":1,"name":"M","mood":"InvalidMood","engines":["Snap"],"parameters":{"Snap":{}}})";
    PresetManager pm;
    REQUIRE(pm.loadPresetFromJSON(juce::String(json)));
    CHECK(pm.getCurrentPreset().mood == "User");
}

TEST_CASE("PresetManager - invalid engine names filtered out, valid ones kept", "[preset][validation]")
{
    const char* json = R"({
        "schema_version":1,"name":"E","mood":"Foundation",
        "engines":["Snap","NotAnEngine","Morph"],
        "parameters":{"Snap":{},"Morph":{}}
    })";
    PresetManager pm;
    REQUIRE(pm.loadPresetFromJSON(juce::String(json)));
    const auto& engines = pm.getCurrentPreset().engines;
    CHECK(!engines.contains("NotAnEngine"));
    CHECK(engines.contains("OddfeliX"));
    CHECK(engines.contains("OddOscar"));
}

TEST_CASE("PresetManager - all invalid engines returns false", "[preset][validation]")
{
    const char* json =
        R"({"schema_version":1,"name":"A","mood":"Foundation","engines":["Fake1","Fake2"],"parameters":{}})";
    PresetManager pm;
    CHECK(!pm.loadPresetFromJSON(juce::String(json)));
}

TEST_CASE("PresetManager - invalid coupling type filtered out, valid kept", "[preset][validation]")
{
    const char* json = R"({
        "schema_version":1,"name":"C","mood":"Foundation","engines":["Snap"],
        "parameters":{"Snap":{}},
        "coupling":{"pairs":[
            {"engineA":"Snap","engineB":"Morph","type":"InvalidType","amount":0.5},
            {"engineA":"Snap","engineB":"Morph","type":"Amp->Filter","amount":0.6}
        ]}
    })";
    PresetManager pm;
    REQUIRE(pm.loadPresetFromJSON(juce::String(json)));
    const auto& pairs = pm.getCurrentPreset().couplingPairs;
    REQUIRE(pairs.size() == 1);
    CHECK(pairs[0].type == "Amp->Filter");
}

TEST_CASE("PresetManager - macroLabels padded to exactly 4", "[preset][validation]")
{
    const char* json = R"({
        "schema_version":1,"name":"X","mood":"Foundation","engines":["Snap"],
        "parameters":{"Snap":{}},"macroLabels":["CUSTOM1"]
    })";
    PresetManager pm;
    REQUIRE(pm.loadPresetFromJSON(juce::String(json)));
    const auto& labels = pm.getCurrentPreset().macroLabels;
    REQUIRE(labels.size() == 4);
    CHECK(labels[0] == "CUSTOM1");
    CHECK(labels[1] == "MOVEMENT");
    CHECK(labels[2] == "COUPLING");
    CHECK(labels[3] == "SPACE");
}

TEST_CASE("PresetManager - macroLabels trimmed to exactly 4", "[preset][validation]")
{
    const char* json = R"({
        "schema_version":1,"name":"Y","mood":"Foundation","engines":["Snap"],
        "parameters":{"Snap":{}},"macroLabels":["A","B","C","D","E","F"]
    })";
    PresetManager pm;
    REQUIRE(pm.loadPresetFromJSON(juce::String(json)));
    CHECK(pm.getCurrentPreset().macroLabels.size() == 4);
}

TEST_CASE("PresetManager - empty JSON returns false", "[preset][validation]")
{
    PresetManager pm;
    CHECK(!pm.loadPresetFromJSON(""));
}

TEST_CASE("PresetManager - garbage JSON returns false", "[preset][validation]")
{
    PresetManager pm;
    CHECK(!pm.loadPresetFromJSON("not valid json at all"));
}

TEST_CASE("PresetManager - schema_version < 1 returns false", "[preset][validation]")
{
    const char* json =
        R"({"schema_version":0,"name":"B","mood":"Foundation","engines":["Snap"],"parameters":{"Snap":{}}})";
    PresetManager pm;
    CHECK(!pm.loadPresetFromJSON(juce::String(json)));
}

//==============================================================================
// DNA distance and search tests
//==============================================================================

TEST_CASE("PresetManager - dnaDistance(a,a) = 0", "[preset][dna]")
{
    PresetManager pm;
    PresetDNA a;
    a.brightness = 0.5f;
    a.warmth = 0.3f;
    a.movement = 0.7f;
    a.density = 0.1f;
    a.space = 0.9f;
    a.aggression = 0.4f;
    CHECK(pm.dnaDistance(a, a) == 0.0f);
}

TEST_CASE("PresetManager - dnaDistance(a,b) > 0 when a != b", "[preset][dna]")
{
    PresetManager pm;
    PresetDNA a, b;
    a.brightness = 0.1f;
    a.warmth = 0.2f;
    a.movement = 0.3f;
    a.density = 0.4f;
    a.space = 0.5f;
    a.aggression = 0.6f;
    b.brightness = 0.9f;
    b.warmth = 0.8f;
    b.movement = 0.7f;
    b.density = 0.6f;
    b.space = 0.5f;
    b.aggression = 0.4f;
    CHECK(pm.dnaDistance(a, b) > 0.0f);
}

TEST_CASE("PresetManager - findSimilar returns presets sorted by distance", "[preset][dna]")
{
    auto makePreset = [](const char* name, float brightness)
    {
        PresetData p;
        p.schemaVersion = 1;
        p.name = name;
        p.mood = "Foundation";
        p.engines.add("Snap");
        p.dna.brightness = brightness;
        p.dna.warmth = p.dna.movement = p.dna.density = p.dna.space = p.dna.aggression = 0.5f;
        return p;
    };

    PresetManager pm;
    pm.addPreset(makePreset("Far", 0.0f));
    pm.addPreset(makePreset("Close", 0.8f));
    pm.addPreset(makePreset("Closest", 0.9f));
    pm.addPreset(makePreset("Medium", 0.5f));

    PresetDNA target;
    target.brightness = 1.0f;
    target.warmth = target.movement = target.density = target.space = target.aggression = 0.5f;

    auto similar = pm.findSimilar(target, 4);
    CHECK(similar.size() == 4);
    if (similar.size() >= 2)
    {
        CHECK(similar[0].name == "Closest");
        CHECK(similar[1].name == "Close");
    }
}

TEST_CASE("PresetManager - findOpposite returns presets near inverted DNA", "[preset][dna]")
{
    auto makePreset = [](const char* name, float brightness)
    {
        PresetData p;
        p.schemaVersion = 1;
        p.name = name;
        p.mood = "Foundation";
        p.engines.add("Snap");
        p.dna.brightness = brightness;
        p.dna.warmth = p.dna.movement = p.dna.density = p.dna.space = p.dna.aggression = 0.5f;
        return p;
    };

    PresetManager pm;
    pm.addPreset(makePreset("Bright", 0.9f));
    pm.addPreset(makePreset("Dark", 0.1f));
    pm.addPreset(makePreset("Mid", 0.5f));

    PresetDNA target;
    target.brightness = 0.9f;
    target.warmth = target.movement = target.density = target.space = target.aggression = 0.5f;

    auto opposite = pm.findOpposite(target, 3);
    CHECK(!opposite.empty());
    if (!opposite.empty())
        CHECK(opposite[0].name == "Dark");
}

TEST_CASE("PresetManager - empty library returns empty results from findSimilar/findOpposite", "[preset][dna]")
{
    PresetManager pm;
    PresetDNA target;
    CHECK(pm.findSimilar(target, 5).empty());
    CHECK(pm.findOpposite(target, 5).empty());
}

//==============================================================================
// File size tests
//==============================================================================

TEST_CASE("PresetManager - kMaxPresetFileSize is 1MB", "[preset][filesize]")
{
    CHECK(PresetManager::kMaxPresetFileSize == 1024 * 1024);
}

TEST_CASE("PresetManager - file larger than 1MB is rejected", "[preset][filesize]")
{
    auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto bigFile = tempDir.getChildFile("xoceanus_test_big.xometa");

    juce::String bigContent;
    bigContent.preallocateBytes(1024 * 1024 + 100);
    bigContent = "{\"schema_version\":1,\"name\":\"Big\",\"mood\":\"Foundation\","
                 "\"engines\":[\"Snap\"],\"parameters\":{\"Snap\":{\"data\":\"";
    for (int i = 0; i < 1024 * 1024; ++i)
        bigContent += "x";
    bigContent += "\"}}}";

    bigFile.replaceWithText(bigContent);

    PresetManager pm;
    bool loaded = pm.loadPresetFromFile(bigFile);
    bigFile.deleteFile();
    CHECK(!loaded);
}

