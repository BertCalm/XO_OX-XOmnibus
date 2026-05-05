/*
    XOceanus Save As Tests
    ======================
    Unit tests for:
      - PresetManager::getNextAvailableName() — collision-avoidance helper
      - PresetManager::savePresetToFile(file, data, confirmOverwrite) — overwrite callback

    Issue: #1354 / #1405
*/

#include <catch2/catch_test_macros.hpp>

#include "Core/PresetManager.h"

#include <juce_core/juce_core.h>

using namespace xoceanus;

//==============================================================================
// getNextAvailableName tests
//==============================================================================

TEST_CASE("PresetManager::getNextAvailableName - returns base name when no collision", "[preset][save-as]")
{
    auto tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("xoceanus_saveas_test_" + juce::String (juce::Random::getSystemRandom().nextInt()));
    tempDir.createDirectory();

    auto result = PresetManager::getNextAvailableName ("My Preset", tempDir);
    CHECK (result == "My Preset");

    tempDir.deleteRecursively();
}

TEST_CASE("PresetManager::getNextAvailableName - appends (2) when base exists", "[preset][save-as]")
{
    auto tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("xoceanus_saveas_test_" + juce::String (juce::Random::getSystemRandom().nextInt()));
    tempDir.createDirectory();
    tempDir.getChildFile ("My Preset.xometa").create();

    auto result = PresetManager::getNextAvailableName ("My Preset", tempDir);
    CHECK (result == "My Preset (2)");

    tempDir.deleteRecursively();
}

TEST_CASE("PresetManager::getNextAvailableName - skips to (3) when (2) also exists", "[preset][save-as]")
{
    auto tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("xoceanus_saveas_test_" + juce::String (juce::Random::getSystemRandom().nextInt()));
    tempDir.createDirectory();
    tempDir.getChildFile ("My Preset.xometa").create();
    tempDir.getChildFile ("My Preset (2).xometa").create();

    auto result = PresetManager::getNextAvailableName ("My Preset", tempDir);
    CHECK (result == "My Preset (3)");

    tempDir.deleteRecursively();
}

//==============================================================================
// savePresetToFile overwrite-confirm overload tests
//==============================================================================

TEST_CASE("PresetManager::savePresetToFile overwrite-confirm - calls confirm only on collision", "[preset][save-as]")
{
    auto tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("xoceanus_saveas_test_" + juce::String (juce::Random::getSystemRandom().nextInt()));
    tempDir.createDirectory();
    auto target = tempDir.getChildFile ("test.xometa");

    PresetData data;
    data.name = "test";

    bool confirmCalled = false;
    PresetManager pm;
    auto result = pm.savePresetToFile (target, data,
        [&] (juce::File) { confirmCalled = true; return true; });

    CHECK (result);
    CHECK (! confirmCalled); // no collision → callback not invoked
    CHECK (target.existsAsFile());

    tempDir.deleteRecursively();
}

TEST_CASE("PresetManager::savePresetToFile overwrite-confirm - aborts save when confirm returns false", "[preset][save-as]")
{
    auto tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                      .getChildFile ("xoceanus_saveas_test_" + juce::String (juce::Random::getSystemRandom().nextInt()));
    tempDir.createDirectory();
    auto target = tempDir.getChildFile ("test.xometa");

    // Write a known sentinel into the file to prove it is NOT overwritten.
    target.replaceWithText ("ORIGINAL_CONTENT");

    PresetData data;
    data.name   = "test";
    data.author = "agent-test"; // sentinel that must NOT appear after a cancelled save

    PresetManager pm;
    auto result = pm.savePresetToFile (target, data,
        [] (juce::File) { return false; }); // cancel

    CHECK (! result);
    CHECK (target.loadFileAsString() == "ORIGINAL_CONTENT"); // unchanged

    tempDir.deleteRecursively();
}
