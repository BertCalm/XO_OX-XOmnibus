/*
    XOceanus Schema v2 Reader + Taxonomy Validator Tests
    =====================================================
    Verifies that PresetManager::parseJSON correctly handles schema v1 and v2
    presets, that invalid category/timbre values are dropped gracefully, and that
    the PresetTaxonomy validators enforce exact-match case-sensitive lookup.

    Task 7 of the preset-picker-phase-1 spec.
*/

#include "SchemaV2Tests.h"

#include <catch2/catch_test_macros.hpp>

#include "Core/PresetManager.h"
#include "Core/PresetTaxonomy.h"

#include <juce_core/juce_core.h>

using namespace xoceanus;

namespace
{
    juce::File fixturesDir()
    {
        return juce::File (__FILE__).getParentDirectory().getChildFile ("fixtures");
    }

    juce::String readFixture (const juce::String& filename)
    {
        return fixturesDir().getChildFile (filename).loadFileAsString();
    }
} // namespace

//==============================================================================
// Schema v1 backward-compatibility
//==============================================================================

TEST_CASE ("Schema v1 preset loads with null category and timbre", "[preset][schema_v2]")
{
    PresetManager pm;
    PresetData data;
    const auto json = readFixture ("v1-sample.xometa");

    REQUIRE (pm.parseJSON (json, data));
    CHECK (data.schemaVersion == 1);
    CHECK (! data.category.has_value());
    CHECK (! data.timbre.has_value());
    CHECK (data.tier.isEmpty());
    CHECK (data.name == "Legacy Hymn");
    CHECK (data.mood == "Deep");
}

//==============================================================================
// Schema v2 — full preset (category + timbre + tier)
//==============================================================================

TEST_CASE ("Schema v2 full preset loads with category, timbre, tier", "[preset][schema_v2]")
{
    PresetManager pm;
    PresetData data;
    const auto json = readFixture ("v2-full.xometa");

    REQUIRE (pm.parseJSON (json, data));
    CHECK (data.schemaVersion == 2);
    REQUIRE (data.category.has_value());
    CHECK (*data.category == "pads");
    REQUIRE (data.timbre.has_value());
    CHECK (*data.timbre == "organ");
    CHECK (data.tier == "awakening");
}

//==============================================================================
// Schema v2 — minimal preset (category only, timbre absent/null)
//==============================================================================

TEST_CASE ("Schema v2 minimal preset loads with category only (timbre null)", "[preset][schema_v2]")
{
    PresetManager pm;
    PresetData data;
    const auto json = readFixture ("v2-minimal.xometa");

    REQUIRE (pm.parseJSON (json, data));
    CHECK (data.schemaVersion == 2);
    REQUIRE (data.category.has_value());
    CHECK (*data.category == "pads");
    CHECK (! data.timbre.has_value());
    CHECK (data.tier.isEmpty());
}

//==============================================================================
// Schema v2 — malformed category is silently dropped
//==============================================================================

TEST_CASE ("Schema v2 malformed category is rejected (category stays nullopt)", "[preset][schema_v2]")
{
    PresetManager pm;
    PresetData data;
    const auto json = readFixture ("v2-malformed-category.xometa");

    // Parse succeeds (preset is otherwise well-formed) but invalid category
    // value is dropped — safest behavior for library-import resilience.
    REQUIRE (pm.parseJSON (json, data));
    CHECK (data.schemaVersion == 2);
    CHECK (! data.category.has_value()); // "xylophones" dropped
    CHECK (data.name == "Bad Category");
}

//==============================================================================
// PresetTaxonomy validators
//==============================================================================

TEST_CASE ("PresetTaxonomy validators", "[preset][taxonomy]")
{
    SECTION ("all 10 categories validate true")
    {
        for (auto* c : xoceanus::kPresetCategories)
            CHECK (xoceanus::isValidPresetCategory (juce::String (c)));
    }

    SECTION ("known bad category returns false")
    {
        CHECK_FALSE (xoceanus::isValidPresetCategory ("xylophones"));
        CHECK_FALSE (xoceanus::isValidPresetCategory (""));
        CHECK_FALSE (xoceanus::isValidPresetCategory ("Pads")); // case-sensitive
    }

    SECTION ("all 8 timbres validate true")
    {
        for (auto* t : xoceanus::kPresetTimbres)
            CHECK (xoceanus::isValidPresetTimbre (juce::String (t)));
    }

    SECTION ("known bad timbre returns false")
    {
        CHECK_FALSE (xoceanus::isValidPresetTimbre ("electric"));
        CHECK_FALSE (xoceanus::isValidPresetTimbre (""));
        CHECK_FALSE (xoceanus::isValidPresetTimbre ("STRINGS")); // case-sensitive
    }
}

TEST_CASE ("Writer round-trip: v2 PresetData -> JSON -> PresetData is identity", "[preset][schema_v2][writer]")
{
    PresetManager pm;

    PresetData original;
    original.schemaVersion = 2;
    original.name = "Roundtrip Test";
    original.mood = "Deep";
    original.category = juce::String ("pads");
    original.timbre = juce::String ("organ");
    original.tier = "awakening";
    original.engines.add ("Organon");
    original.author = "Test";
    original.version = "1.0";
    original.description = "Round trip.";
    original.tags.add ("test");
    original.macroLabels = juce::StringArray ({ "CHARACTER","MOVEMENT","COUPLING","SPACE" });
    original.couplingIntensity = "None";
    original.tempo = 0.0f;
    original.dna.brightness = 0.2f;
    original.dna.warmth = 0.8f;
    original.dna.movement = 0.3f;
    original.dna.density = 0.7f;
    original.dna.space = 0.9f;
    original.dna.aggression = 0.1f;

    const auto json = pm.serializeToJSON (original);

    PresetData reloaded;
    REQUIRE (pm.parseJSON (json, reloaded));

    CHECK (reloaded.schemaVersion == 2);
    CHECK (reloaded.name == original.name);
    CHECK (reloaded.mood == original.mood);
    REQUIRE (reloaded.category.has_value());
    CHECK (*reloaded.category == *original.category);
    REQUIRE (reloaded.timbre.has_value());
    CHECK (*reloaded.timbre == *original.timbre);
    CHECK (reloaded.tier == original.tier);
}

TEST_CASE ("Writer omits timbre when nullopt", "[preset][schema_v2][writer]")
{
    PresetManager pm;

    PresetData data;
    data.schemaVersion = 2;
    data.name = "No Timbre";
    data.mood = "Atmosphere";
    data.category = juce::String ("fx");
    data.timbre = std::nullopt;
    data.tier = "";
    data.engines.add ("Organon");
    data.macroLabels = juce::StringArray ({ "CHARACTER","MOVEMENT","COUPLING","SPACE" });

    const auto json = pm.serializeToJSON (data);

    // Reload and confirm timbre stays nullopt
    PresetData reloaded;
    REQUIRE (pm.parseJSON (json, reloaded));
    CHECK (! reloaded.timbre.has_value());

    // Verify the JSON string does not contain a "timbre" key
    CHECK_FALSE (json.contains ("\"timbre\""));
}
