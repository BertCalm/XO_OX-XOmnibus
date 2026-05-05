/*
    XOceanus MoodModulationBus Tests (Phase 0 smoke test)
    =====================================================
    Validates the Phase 0 wildcard infrastructure primitive that promotes
    a preset's 16-mood tag from static metadata to a live continuous
    modulation source. Tests cover:

      - Default state (one-hot Foundation)
      - setMood one-hot behaviour, all 16 moods
      - setMoodByName for every canonical mood string
      - Unknown / "User" mood names leave state unchanged
      - setBlend produces soft-blend continuous output
      - Smoothing converges over multiple blocks (D005-style breathing)
      - Drift LFO supports ≤ 0.01 Hz rates (D005)
      - getDominantMood reflects highest-weight mood
      - Out-of-range mood index handling

    Locked decisions exercised:
      D3 — bus is read-only; default-OFF lives at the consumer.
           This test only exercises the bus; consumer gating is tested
           where the consumer ships (Pack 8 — Mastering, Phase 2).
      D005 — drift LFO floor ≤ 0.01 Hz. Bus accepts 0.001 Hz.

    See: Docs/specs/2026-04-27-fx-engine-build-plan.md §3.1
*/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Source/Core/MoodModulationBus.h"

using xoceanus::MoodModulationBus;
using Mood = MoodModulationBus::Mood;
using Catch::Matchers::WithinAbs;

namespace
{
    void converge(MoodModulationBus& bus, int iterations = 1000)
    {
        for (int i = 0; i < iterations; ++i)
            bus.advanceSmoothing(256);
    }

    constexpr const char* kMoodNames[MoodModulationBus::NumMoods] = {
        "Foundation",  "Atmosphere",  "Entangled",   "Prism",
        "Flux",        "Aether",      "Family",      "Submerged",
        "Coupling",    "Crystalline", "Deep",        "Ethereal",
        "Kinetic",     "Luminous",    "Organic",     "Shadow"
    };
}

TEST_CASE("MoodModulationBus - default state is one-hot Foundation", "[mood][bus][defaults]")
{
    MoodModulationBus bus;

    REQUIRE(bus.getWeight(Mood::Foundation) == 1.0f);
    for (int i = 1; i < MoodModulationBus::NumMoods; ++i)
        REQUIRE(bus.getWeight(i) == 0.0f);

    REQUIRE(bus.getDominantMood() == Mood::Foundation);
}

TEST_CASE("MoodModulationBus - setMood one-hot for every mood", "[mood][bus][one-hot]")
{
    MoodModulationBus bus;

    for (int i = 0; i < MoodModulationBus::NumMoods; ++i)
    {
        const auto m = static_cast<Mood>(i);
        bus.setMood(m);
        converge(bus);

        REQUIRE_THAT(bus.getWeight(m), WithinAbs(1.0f, 1e-3f));

        for (int j = 0; j < MoodModulationBus::NumMoods; ++j)
            if (j != i)
                REQUIRE_THAT(bus.getWeight(j), WithinAbs(0.0f, 1e-3f));

        REQUIRE(bus.getDominantMood() == m);
    }
}

TEST_CASE("MoodModulationBus - setMoodByName accepts all 16 canonical names", "[mood][bus][names]")
{
    MoodModulationBus bus;

    for (int i = 0; i < MoodModulationBus::NumMoods; ++i)
    {
        REQUIRE(bus.setMoodByName(kMoodNames[i]));
        converge(bus);
        REQUIRE_THAT(bus.getWeight(static_cast<Mood>(i)), WithinAbs(1.0f, 1e-3f));
    }
}

TEST_CASE("MoodModulationBus - unknown / User / empty mood names leave state unchanged", "[mood][bus][names]")
{
    MoodModulationBus bus;
    bus.setMoodByName("Atmosphere");
    converge(bus);
    REQUIRE_THAT(bus.getWeight(Mood::Atmosphere), WithinAbs(1.0f, 1e-3f));

    REQUIRE_FALSE(bus.setMoodByName("User"));
    REQUIRE_FALSE(bus.setMoodByName(""));
    REQUIRE_FALSE(bus.setMoodByName("NotAMood"));

    converge(bus);
    // State unchanged — Atmosphere still dominant.
    REQUIRE_THAT(bus.getWeight(Mood::Atmosphere), WithinAbs(1.0f, 1e-3f));
}

TEST_CASE("MoodModulationBus - setBlend produces soft-blend continuous output", "[mood][bus][blend]")
{
    MoodModulationBus bus;

    SECTION("blend = 0 → pure a")
    {
        bus.setBlend(Mood::Foundation, Mood::Shadow, 0.0f);
        converge(bus);
        REQUIRE_THAT(bus.getWeight(Mood::Foundation), WithinAbs(1.0f, 1e-3f));
        REQUIRE_THAT(bus.getWeight(Mood::Shadow),     WithinAbs(0.0f, 1e-3f));
    }

    SECTION("blend = 1 → pure b")
    {
        bus.setBlend(Mood::Foundation, Mood::Shadow, 1.0f);
        converge(bus);
        REQUIRE_THAT(bus.getWeight(Mood::Foundation), WithinAbs(0.0f, 1e-3f));
        REQUIRE_THAT(bus.getWeight(Mood::Shadow),     WithinAbs(1.0f, 1e-3f));
    }

    SECTION("blend = 0.5 → equal weight, sum = 1")
    {
        bus.setBlend(Mood::Foundation, Mood::Shadow, 0.5f);
        converge(bus);
        REQUIRE_THAT(bus.getWeight(Mood::Foundation), WithinAbs(0.5f, 1e-3f));
        REQUIRE_THAT(bus.getWeight(Mood::Shadow),     WithinAbs(0.5f, 1e-3f));

        const auto all = bus.getAllWeights();
        float sum = 0.0f;
        for (auto w : all) sum += w;
        REQUIRE_THAT(sum, WithinAbs(1.0f, 1e-3f));
    }

    SECTION("blend with same mood twice collapses to one-hot")
    {
        bus.setBlend(Mood::Crystalline, Mood::Crystalline, 0.5f);
        converge(bus);
        REQUIRE_THAT(bus.getWeight(Mood::Crystalline), WithinAbs(1.0f, 1e-3f));
    }
}

TEST_CASE("MoodModulationBus - smoothing converges, does not jump", "[mood][bus][smoothing]")
{
    MoodModulationBus bus;
    bus.setMood(Mood::Foundation);
    converge(bus);

    // Switch target — first advance must be partway, not snapped.
    bus.setMood(Mood::Shadow);
    bus.advanceSmoothing(256);

    const float shadowAfterOne = bus.getWeight(Mood::Shadow);
    REQUIRE(shadowAfterOne > 0.0f);
    REQUIRE(shadowAfterOne < 1.0f);

    converge(bus);
    REQUIRE_THAT(bus.getWeight(Mood::Shadow), WithinAbs(1.0f, 1e-3f));
}

TEST_CASE("MoodModulationBus - drift LFO supports ≤ 0.01 Hz (D005 must-breathe)", "[mood][bus][drift][D005]")
{
    MoodModulationBus bus;
    bus.prepare(48000.0, 256);

    SECTION("default drift output is 0")
    {
        bus.advanceSmoothing(256);
        REQUIRE(bus.getDriftValue() == 0.0f);
    }

    SECTION("0.005 Hz rate (well below the D005 floor) is accepted and oscillates")
    {
        bus.setDriftRate(0.005f);
        bus.setDriftDepth(1.0f);

        // Advance enough blocks for the phase to move appreciably:
        // dt per block = 256 / 48000 = ~5.33 ms; phase increment per block
        // = 2π × 0.005 × dt = ~1.67e-4 rad. After 5000 blocks (≈26.7 s),
        // phase = ~0.838 rad → sin ≈ 0.74. Just sample-and-check non-zero.
        for (int i = 0; i < 5000; ++i)
            bus.advanceSmoothing(256);

        const float v = bus.getDriftValue();
        REQUIRE(std::abs(v) > 0.05f);
        REQUIRE(std::abs(v) <= 1.0f);
    }

    SECTION("rate = 0 disables drift cleanly")
    {
        bus.setDriftRate(1.0f);
        bus.setDriftDepth(1.0f);
        bus.advanceSmoothing(256);
        // Some non-zero value possible after one step.

        bus.setDriftRate(0.0f);
        bus.advanceSmoothing(256);
        REQUIRE(bus.getDriftValue() == 0.0f);
    }

    SECTION("depth scales output range")
    {
        bus.setDriftRate(2.0f);
        bus.setDriftDepth(0.25f);

        float maxAbs = 0.0f;
        // 2 Hz × 256/48000 s/block = 0.0107 rad/block → ~588 blocks for 2π.
        for (int i = 0; i < 1200; ++i)
        {
            bus.advanceSmoothing(256);
            maxAbs = std::max(maxAbs, std::abs(bus.getDriftValue()));
        }

        REQUIRE(maxAbs <= 0.25f + 1e-3f);
        REQUIRE(maxAbs >  0.20f); // covered most of a full cycle
    }
}

TEST_CASE("MoodModulationBus - getDominantMood reflects highest weight", "[mood][bus][dominant]")
{
    MoodModulationBus bus;

    bus.setBlend(Mood::Aether, Mood::Kinetic, 0.7f); // 0.3 Aether, 0.7 Kinetic
    converge(bus);
    REQUIRE(bus.getDominantMood() == Mood::Kinetic);

    bus.setBlend(Mood::Aether, Mood::Kinetic, 0.3f); // 0.7 Aether, 0.3 Kinetic
    converge(bus);
    REQUIRE(bus.getDominantMood() == Mood::Aether);
}

TEST_CASE("MoodModulationBus - out-of-range mood index returns 0", "[mood][bus][bounds]")
{
    MoodModulationBus bus;

    REQUIRE(bus.getWeight(-1) == 0.0f);
    REQUIRE(bus.getWeight(99) == 0.0f);
    REQUIRE(bus.getWeight(MoodModulationBus::NumMoods) == 0.0f);
}

TEST_CASE("MoodModulationBus - getAllWeights matches per-mood get", "[mood][bus][api]")
{
    MoodModulationBus bus;
    bus.setBlend(Mood::Prism, Mood::Luminous, 0.4f);
    converge(bus);

    const auto all = bus.getAllWeights();
    for (int i = 0; i < MoodModulationBus::NumMoods; ++i)
        REQUIRE_THAT(all[(size_t) i],
                     WithinAbs(bus.getWeight(static_cast<Mood>(i)), 1e-6f));
}
