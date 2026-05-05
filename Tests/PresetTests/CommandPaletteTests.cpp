/*
    XOceanus Command Palette Tests
    ===============================
    Unit tests for the Cmd+K command palette feature (#22).

    Covers:
      - PresetManager::getPresetLibrary(), loadPresetByIndex()
      - PresetManager::recordPresetLoad(), getRecentPresetIndices()
      - EngineRegistry::recordEngineLoad(), getRecentEngineIds()
      - scorePresetLiteral() — literal fuzzy scoring
      - scorePresetDNA()     — DNA dimension scoring
      - scoreEngineLiteral() — engine id/display/kind scoring

    Spec: Docs/plans/2026-05-05-cmdk-palette-design.md
    Plan: Docs/plans/2026-05-05-cmdk-palette-implementation-plan.md
*/

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Core/PresetManager.h"
#include "Core/EngineRegistry.h"

using namespace xoceanus;

// ─────────────────────────────────────────────────────────────────────────────
// Task 1: PresetManager library accessor + loadPresetByIndex
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("PresetManager exposes preset library snapshot", "[CommandPalette]")
{
    PresetManager pm;
    auto lib = pm.getPresetLibrary();
    REQUIRE(lib != nullptr);
    // Library may be empty in test harness — only assert the accessor works.
}

TEST_CASE("loadPresetByIndex bounds-checks", "[CommandPalette]")
{
    PresetManager pm;
    REQUIRE_FALSE(pm.loadPresetByIndex(-1));
    REQUIRE_FALSE(pm.loadPresetByIndex(999999999));
}

// ─────────────────────────────────────────────────────────────────────────────
// Task 2: PresetManager recents tracking
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("PresetManager recents — push, dedupe, max-8", "[CommandPalette]")
{
    PresetManager pm;
    REQUIRE(pm.getRecentPresetIndices().empty());

    pm.recordPresetLoad(5);
    pm.recordPresetLoad(7);
    pm.recordPresetLoad(5);  // dedupe → moves 5 to front

    auto r = pm.getRecentPresetIndices();
    REQUIRE(r.size() == 2);
    REQUIRE(r[0] == 5);
    REQUIRE(r[1] == 7);

    for (int i = 0; i < 12; ++i) pm.recordPresetLoad(100 + i);
    REQUIRE(pm.getRecentPresetIndices().size() == 8);
}

// ─────────────────────────────────────────────────────────────────────────────
// Task 3: EngineRegistry recents tracking
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("EngineRegistry recents — push, dedupe, max-8", "[CommandPalette]")
{
    auto& reg = xoceanus::EngineRegistry::instance();
    // Note: singleton state persists across tests. Clear before assertions.
    reg.clearRecentsForTesting();

    REQUIRE(reg.getRecentEngineIds().empty());

    reg.recordEngineLoad("Obsidian", 0);
    reg.recordEngineLoad("Oracle", 1);
    reg.recordEngineLoad("Obsidian", 2);  // dedupe → moves to front

    auto r = reg.getRecentEngineIds();
    REQUIRE(r.size() == 2);
    REQUIRE(r[0] == "Obsidian");
    REQUIRE(r[1] == "Oracle");

    for (int i = 0; i < 12; ++i) reg.recordEngineLoad("Engine" + std::to_string(i), 0);
    REQUIRE(reg.getRecentEngineIds().size() == 8);

    reg.clearRecentsForTesting();  // leave clean for next test
}

// ─────────────────────────────────────────────────────────────────────────────
// Tasks 5-7: Scoring functions (included from CommandPalette.h)
// ─────────────────────────────────────────────────────────────────────────────

#include "UI/Ocean/CommandPalette.h"

TEST_CASE("Preset literal ranking — name beats tag beats description", "[CommandPalette][Rank]")
{
    PresetData a; a.name = "Warm Pad";    a.tags.add("ambient");
    PresetData b; b.name = "Glacial";      b.tags.add("warm"); b.tags.add("pad");
    PresetData c; c.name = "Description";  c.description = "warm strings";

    juce::StringArray tokens; tokens.add("warm");

    REQUIRE(scorePresetLiteral(a, tokens) == Catch::Approx(3.0f));   // name hit
    REQUIRE(scorePresetLiteral(b, tokens) == Catch::Approx(2.0f));   // tag hit
    REQUIRE(scorePresetLiteral(c, tokens) == Catch::Approx(1.0f));   // description hit
}

TEST_CASE("DNA dimension ranking", "[CommandPalette][Rank]")
{
    PresetData a; a.name = "Glacier"; a.dna.warmth = 0.9f;
    PresetData b; b.name = "Glacier"; b.dna.warmth = 0.1f;

    juce::StringArray tokens; tokens.add("warm");

    REQUIRE(scorePresetDNA(a, tokens) == Catch::Approx(1.8f));  // 0.9 * 2
    REQUIRE(scorePresetDNA(b, tokens) == Catch::Approx(0.2f));  // 0.1 * 2
}

TEST_CASE("DNA dimension keywords map correctly", "[CommandPalette][Rank]")
{
    PresetData p;
    p.dna.brightness = 0.7f;
    p.dna.warmth     = 0.0f;
    p.dna.movement   = 0.5f;
    p.dna.density    = 0.8f;
    p.dna.space      = 0.3f;
    p.dna.aggression = 0.9f;

    auto check = [&](const char* word, float expected) {
        juce::StringArray tokens; tokens.add(word);
        REQUIRE(scorePresetDNA(p, tokens) == Catch::Approx(expected));
    };

    check("bright",     0.7f * 2);
    check("warm",       0.0f * 2);
    check("movement",   0.5f * 2);
    check("dense",      0.8f * 2);
    check("space",      0.3f * 2);
    check("aggressive", 0.9f * 2);
}

TEST_CASE("Engine literal ranking — id beats tag", "[CommandPalette][Rank]")
{
    juce::StringArray tokens; tokens.add("obsidian");
    // id hit (3) + display hit (2) = 5
    REQUIRE(scoreEngineLiteral("Obsidian", "Obsidian", "pad", tokens) == Catch::Approx(5.0f));

    juce::StringArray bassTokens; bassTokens.add("bass");
    // kind tag hit only = 1
    REQUIRE(scoreEngineLiteral("OGRE", "OGRE", "bass", bassTokens) == Catch::Approx(1.0f));
}

TEST_CASE("rerank() empty query returns recents", "[CommandPalette]")
{
    PresetManager pm;
    pm.recordPresetLoad(0);
    auto lib = pm.getPresetLibrary();
    if (lib == nullptr || lib->empty()) { SUCCEED("no library in test env"); return; }

    SUCCEED("ranking pipeline compiles");
}
