/*
    XOceanus DNAModulationBus Tests
    ===============================
    Validates the Phase 0 wildcard infrastructure primitive that promotes
    the 6D Sonic DNA from preset metadata to a live continuous modulation
    source. Tests cover:

      - Default state (neutral 0.5 across all axes)
      - setBaseDNA bounds clamping and per-axis assignment
      - Uniform applyMacroWarp behaviour (positive/negative/zero macro)
      - Per-axis weighted applyMacroWarp (decision A1)
      - Smoothing convergence over multiple blocks
      - Out-of-range engine slot graceful handling
      - getAll() consistency with per-axis get()

    Locked decisions exercised:
      D1 — per-engine warp at block-rate, smoothed
      A1 — applyMacroWarp two overloads (uniform + per-axis weights)
      D5 — defaults: AGE-style "factory-fresh" predictability (here: 0.5
            neutral DNA when nothing is set)

    See: Docs/specs/2026-04-27-fx-engine-build-plan.md §3.1
         Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md §5.1
*/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Source/Core/DNAModulationBus.h"

using xoceanus::DNAModulationBus;
using Axis = DNAModulationBus::Axis;
using Catch::Matchers::WithinAbs;

namespace
{
    // Run smoothing to convergence — far more blocks than needed, but cheap.
    void converge(DNAModulationBus& bus, int iterations = 1000)
    {
        for (int i = 0; i < iterations; ++i)
            bus.advanceSmoothing(256);
    }
}

TEST_CASE("DNAModulationBus - default state is neutral 0.5 on all axes", "[dna][bus][defaults]")
{
    DNAModulationBus bus;

    for (int slot = 0; slot < DNAModulationBus::MaxEngineSlots; ++slot)
    {
        REQUIRE(bus.get(slot, Axis::Brightness) == 0.5f);
        REQUIRE(bus.get(slot, Axis::Warmth)     == 0.5f);
        REQUIRE(bus.get(slot, Axis::Movement)   == 0.5f);
        REQUIRE(bus.get(slot, Axis::Density)    == 0.5f);
        REQUIRE(bus.get(slot, Axis::Space)      == 0.5f);
        REQUIRE(bus.get(slot, Axis::Aggression) == 0.5f);
    }
}

TEST_CASE("DNAModulationBus - setBaseDNA stores per-slot per-axis values", "[dna][bus][base]")
{
    DNAModulationBus bus;
    bus.setBaseDNA(0, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f});
    converge(bus);

    REQUIRE_THAT(bus.get(0, Axis::Brightness), WithinAbs(0.1f, 1e-3f));
    REQUIRE_THAT(bus.get(0, Axis::Warmth),     WithinAbs(0.2f, 1e-3f));
    REQUIRE_THAT(bus.get(0, Axis::Movement),   WithinAbs(0.3f, 1e-3f));
    REQUIRE_THAT(bus.get(0, Axis::Density),    WithinAbs(0.4f, 1e-3f));
    REQUIRE_THAT(bus.get(0, Axis::Space),      WithinAbs(0.5f, 1e-3f));
    REQUIRE_THAT(bus.get(0, Axis::Aggression), WithinAbs(0.6f, 1e-3f));

    // Other slots untouched
    REQUIRE(bus.get(1, Axis::Brightness) == 0.5f);
}

TEST_CASE("DNAModulationBus - setBaseDNA clamps out-of-range to [0,1]", "[dna][bus][bounds]")
{
    DNAModulationBus bus;
    bus.setBaseDNA(0, {-0.5f, 1.5f, -1.0f, 2.0f, 0.5f, 0.5f});
    converge(bus);

    REQUIRE(bus.get(0, Axis::Brightness) == 0.0f); // -0.5 clamped to 0
    REQUIRE(bus.get(0, Axis::Warmth)     == 1.0f); //  1.5 clamped to 1
    REQUIRE(bus.get(0, Axis::Movement)   == 0.0f);
    REQUIRE(bus.get(0, Axis::Density)    == 1.0f);
}

TEST_CASE("DNAModulationBus - uniform macro warp pushes all axes toward extreme", "[dna][bus][warp][uniform]")
{
    DNAModulationBus bus;
    bus.setBaseDNA(0, {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f});

    SECTION("Positive macro pushes all axes toward 1.0")
    {
        bus.applyMacroWarp(0, 1.0f);
        converge(bus);

        for (int a = 0; a < DNAModulationBus::NumAxes; ++a)
            REQUIRE_THAT(bus.get(0, static_cast<Axis>(a)), WithinAbs(1.0f, 1e-3f));
    }

    SECTION("Negative macro pushes all axes toward 0.0")
    {
        bus.applyMacroWarp(0, -1.0f);
        converge(bus);

        for (int a = 0; a < DNAModulationBus::NumAxes; ++a)
            REQUIRE_THAT(bus.get(0, static_cast<Axis>(a)), WithinAbs(0.0f, 1e-3f));
    }

    SECTION("Zero macro leaves base DNA unchanged")
    {
        bus.applyMacroWarp(0, 0.0f);
        converge(bus);

        for (int a = 0; a < DNAModulationBus::NumAxes; ++a)
            REQUIRE_THAT(bus.get(0, static_cast<Axis>(a)), WithinAbs(0.5f, 1e-3f));
    }
}

TEST_CASE("DNAModulationBus - per-axis weights selectively warp axes (A1)", "[dna][bus][warp][per-axis]")
{
    DNAModulationBus bus;
    bus.setBaseDNA(0, {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f});

    // Warp only brightness (axis 0) and movement (axis 2) at full strength;
    // leave warmth/density/space/aggression untouched.
    std::array<float, 6> weights = {1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    bus.applyMacroWarp(0, 1.0f, weights);
    converge(bus);

    REQUIRE_THAT(bus.get(0, Axis::Brightness), WithinAbs(1.0f, 1e-3f)); // warped
    REQUIRE_THAT(bus.get(0, Axis::Warmth),     WithinAbs(0.5f, 1e-3f)); // weight 0
    REQUIRE_THAT(bus.get(0, Axis::Movement),   WithinAbs(1.0f, 1e-3f)); // warped
    REQUIRE_THAT(bus.get(0, Axis::Density),    WithinAbs(0.5f, 1e-3f)); // weight 0
    REQUIRE_THAT(bus.get(0, Axis::Space),      WithinAbs(0.5f, 1e-3f)); // weight 0
    REQUIRE_THAT(bus.get(0, Axis::Aggression), WithinAbs(0.5f, 1e-3f)); // weight 0
}

TEST_CASE("DNAModulationBus - partial weights produce proportional warp", "[dna][bus][warp][per-axis]")
{
    DNAModulationBus bus;
    bus.setBaseDNA(0, {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f});

    // 50% weight, +1.0 macro → expect 0.75 (half-way between 0.5 and 1.0)
    std::array<float, 6> weights = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
    bus.applyMacroWarp(0, 1.0f, weights);
    converge(bus);

    for (int a = 0; a < DNAModulationBus::NumAxes; ++a)
        REQUIRE_THAT(bus.get(0, static_cast<Axis>(a)), WithinAbs(0.75f, 1e-3f));
}

TEST_CASE("DNAModulationBus - smoothing converges, does not jump", "[dna][bus][smoothing]")
{
    DNAModulationBus bus;
    bus.setBaseDNA(0, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f});
    converge(bus);
    REQUIRE_THAT(bus.get(0, Axis::Brightness), WithinAbs(0.0f, 1e-3f));

    // Now jump base to 1.0 and check first few advances are below target
    bus.setBaseDNA(0, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f});

    bus.advanceSmoothing(256);
    const float afterOne = bus.get(0, Axis::Brightness);
    REQUIRE(afterOne > 0.0f);
    REQUIRE(afterOne < 1.0f); // not snapped — smoothing is in effect

    converge(bus);
    REQUIRE_THAT(bus.get(0, Axis::Brightness), WithinAbs(1.0f, 1e-3f));
}

TEST_CASE("DNAModulationBus - out-of-range engine slot returns neutral", "[dna][bus][bounds]")
{
    DNAModulationBus bus;

    REQUIRE(bus.get(-1,  Axis::Brightness) == 0.5f);
    REQUIRE(bus.get(99,  Axis::Brightness) == 0.5f);

    auto all = bus.getAll(99);
    for (auto v : all)
        REQUIRE(v == 0.5f);

    // setBaseDNA on bad slot must not crash and must not leak into valid slots
    bus.setBaseDNA(-1, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f});
    converge(bus);
    REQUIRE(bus.get(0, Axis::Brightness) == 0.5f);
}

TEST_CASE("DNAModulationBus - getAll matches per-axis get for all slots", "[dna][bus][api]")
{
    DNAModulationBus bus;
    bus.setBaseDNA(2, {0.11f, 0.22f, 0.33f, 0.44f, 0.55f, 0.66f});
    converge(bus);

    auto all = bus.getAll(2);
    REQUIRE_THAT(all[0], WithinAbs(bus.get(2, Axis::Brightness), 1e-6f));
    REQUIRE_THAT(all[1], WithinAbs(bus.get(2, Axis::Warmth),     1e-6f));
    REQUIRE_THAT(all[2], WithinAbs(bus.get(2, Axis::Movement),   1e-6f));
    REQUIRE_THAT(all[3], WithinAbs(bus.get(2, Axis::Density),    1e-6f));
    REQUIRE_THAT(all[4], WithinAbs(bus.get(2, Axis::Space),      1e-6f));
    REQUIRE_THAT(all[5], WithinAbs(bus.get(2, Axis::Aggression), 1e-6f));
}

TEST_CASE("DNAModulationBus - per-axis warp followed by uniform warp resets weights", "[dna][bus][warp][reset]")
{
    DNAModulationBus bus;
    bus.setBaseDNA(0, {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f});

    // First: per-axis warp with brightness-only
    std::array<float, 6> weights = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    bus.applyMacroWarp(0, 1.0f, weights);
    converge(bus);
    REQUIRE_THAT(bus.get(0, Axis::Brightness), WithinAbs(1.0f, 1e-3f));
    REQUIRE_THAT(bus.get(0, Axis::Warmth),     WithinAbs(0.5f, 1e-3f));

    // Now uniform warp — should reset weights to 1.0 on all axes
    bus.applyMacroWarp(0, 1.0f);
    converge(bus);
    for (int a = 0; a < DNAModulationBus::NumAxes; ++a)
        REQUIRE_THAT(bus.get(0, static_cast<Axis>(a)), WithinAbs(1.0f, 1e-3f));
}
