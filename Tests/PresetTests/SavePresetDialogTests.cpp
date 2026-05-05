/*
    SavePresetDialog + PresetManager Save-As Tests
    =================================================
    Tests for:
    - PresetManager::getNextAvailableName() - base / (2) / (3) cases
    - PresetManager::savePresetToFile() collision-check overload
*/

#include <catch2/catch_test_macros.hpp>

#include "Core/PresetManager.h"

#include <juce_core/juce_core.h>

using namespace xoceanus;

//==============================================================================
// getNextAvailableName tests
//==============================================================================

TEST_CASE("PresetManager::getNextAvailableName - returns base name when no collision", "[preset][saveas]")
{
    auto tempDir = juce::File::createTempFile("xoceanus_test_dir");
    tempDir.deleteFile();
    tempDir.createDirectory();

    auto result = PresetManager::getNextAvailableName("My Preset", tempDir);
    CHECK(result == "My Preset");

    tempDir.deleteRecursively();
}

TEST_CASE("PresetManager::getNextAvailableName - appends (2) when base exists", "[preset][saveas]")
{
    auto tempDir = juce::File::createTempFile("xoceanus_test_dir");
    tempDir.deleteFile();
    tempDir.createDirectory();

    tempDir.getChildFile("My Preset.xometa").create();
    auto result = PresetManager::getNextAvailableName("My Preset", tempDir);
    CHECK(result == "My Preset (2)");

    tempDir.deleteRecursively();
}

TEST_CASE("PresetManager::getNextAvailableName - skips to (3) when (2) also exists", "[preset][saveas]")
{
    auto tempDir = juce::File::createTempFile("xoceanus_test_dir");
    tempDir.deleteFile();
    tempDir.createDirectory();

    tempDir.getChildFile("My Preset.xometa").create();
    tempDir.getChildFile("My Preset (2).xometa").create();
    auto result = PresetManager::getNextAvailableName("My Preset", tempDir);
    CHECK(result == "My Preset (3)");

    tempDir.deleteRecursively();
}

//==============================================================================
// savePresetToFile overwrite-confirm overload tests
//==============================================================================

TEST_CASE("PresetManager::savePresetToFile - confirm callback not invoked when no collision", "[preset][saveas]")
{
    auto tempDir = juce::File::createTempFile("xoceanus_test_dir");
    tempDir.deleteFile();
    tempDir.createDirectory();

    auto target = tempDir.getChildFile("test.xometa");
    PresetData data;
    data.name = "test";
    data.schemaVersion = 2;
    data.mood = "Foundation";
    data.engines.add("OddfeliX");
    data.macroLabels.add("CHARACTER");
    data.macroLabels.add("MOVEMENT");
    data.macroLabels.add("COUPLING");
    data.macroLabels.add("SPACE");
    data.parametersByEngine["OddfeliX"] = juce::var(new juce::DynamicObject());

    bool confirmCalled = false;
    PresetManager pm;
    auto result = pm.savePresetToFile(target, data,
        [&](juce::File) { confirmCalled = true; return true; });

    CHECK(result == true);
    CHECK(confirmCalled == false); // no collision -> callback not invoked
    CHECK(target.existsAsFile());

    tempDir.deleteRecursively();
}

TEST_CASE("PresetManager::savePresetToFile - aborts save when confirm returns false", "[preset][saveas]")
{
    auto tempDir = juce::File::createTempFile("xoceanus_test_dir");
    tempDir.deleteFile();
    tempDir.createDirectory();

    auto target = tempDir.getChildFile("test.xometa");
    // Create a file that will be the "collision"
    target.replaceWithText("{\"schema_version\":1,\"name\":\"original\"}");

    PresetData data;
    data.name = "test";
    data.schemaVersion = 2;
    data.mood = "Foundation";
    data.author = "agent-test"; // marker we can verify is NOT written
    data.engines.add("OddfeliX");
    data.macroLabels.add("CHARACTER");
    data.macroLabels.add("MOVEMENT");
    data.macroLabels.add("COUPLING");
    data.macroLabels.add("SPACE");
    data.parametersByEngine["OddfeliX"] = juce::var(new juce::DynamicObject());

    PresetManager pm;
    auto result = pm.savePresetToFile(target, data,
        [](juce::File) { return false; });

    CHECK(result == false);
    // File should be unchanged (still has "original", not "agent-test")
    CHECK(!target.loadFileAsString().contains("agent-test"));

    tempDir.deleteRecursively();
}
