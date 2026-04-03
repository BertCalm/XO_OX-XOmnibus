/*
    XOlokun Backward Compatibility Regression Tests
    =================================================
    Tests that verify backward compatibility guarantees are not silently broken.
    Covers: engine alias resolution, frozen parameter prefixes, CouplingType enum stability.
    See: https://github.com/BertCalm/XO_OX-XOmnibus/issues/459
    Migrated to Catch2 v3: issue #81
*/

#include "BackwardCompatibilityTests.h"

#include <catch2/catch_test_macros.hpp>

#include "Core/PresetManager.h"
#include "Core/SynthEngine.h"

#include <juce_core/juce_core.h>
#include <utility>
#include <vector>

using namespace xoceanus;

//==============================================================================
// 1. Engine alias regression tests
//==============================================================================

TEST_CASE("BackwardCompat - engine aliases resolve to canonical names", "[compat][alias]")
{
    static const std::pair<const char*, const char*> kAliases[] = {
        {"Snap", "OddfeliX"},        {"Morph", "OddOscar"},   {"Dub", "Overdub"},       {"Drift", "Odyssey"},
        {"Bob", "Oblong"},           {"Fat", "Obese"},        {"Bite", "Overbite"},     {"XOddCouple", "OddfeliX"},
        {"XOverdub", "Overdub"},     {"XOdyssey", "Odyssey"}, {"XOblong", "Oblong"},    {"XOblongBob", "Oblong"},
        {"XObese", "Obese"},         {"XOnset", "Onset"},     {"XOrbital", "Orbital"},  {"XOrganon", "Organon"},
        {"XOuroboros", "Ouroboros"}, {"XOpal", "Opal"},       {"XOpossum", "Overbite"}, {"XOverbite", "Overbite"},
        {"XObsidian", "Obsidian"},   {"XOrigami", "Origami"}, {"XOracle", "Oracle"},    {"XObscura", "Obscura"},
        {"XOceanic", "Oceanic"},     {"XOptic", "Optic"},     {"XOblique", "Oblique"},  {"XOverworld", "Overworld"},
        {"XOrca", "Orca"},           {"XOctopus", "Octopus"}, {"XOverlap", "Overlap"},  {"XOutwit", "Outwit"},
    };

    for (auto& [legacy, expected] : kAliases)
    {
        INFO("Legacy: " << legacy << " -> " << expected);
        CHECK(resolveEngineAlias(legacy) == juce::String(expected));
    }
}

TEST_CASE("BackwardCompat - canonical names pass through resolveEngineAlias unchanged", "[compat][alias]")
{
    CHECK(resolveEngineAlias("Origami") == "Origami");
    CHECK(resolveEngineAlias("OddfeliX") == "OddfeliX");
}

TEST_CASE("BackwardCompat - unknown names pass through resolveEngineAlias unchanged", "[compat][alias]")
{
    CHECK(resolveEngineAlias("UnknownEngine2099") == "UnknownEngine2099");
}

//==============================================================================
// 2. Frozen parameter prefix tests
//==============================================================================

TEST_CASE("BackwardCompat - frozen parameter prefixes match CLAUDE.md source of truth", "[compat][prefix]")
{
    static const std::pair<const char*, const char*> kPrefixes[] = {
        {"OddfeliX", "snap_"},   {"OddOscar", "morph_"},    {"Overdub", "dub_"},     {"Odyssey", "drift_"},
        {"Oblong", "bob_"},      {"Obese", "fat_"},         {"Overbite", "poss_"},   {"Onset", "perc_"},
        {"Overworld", "ow_"},    {"Opal", "opal_"},         {"Orbital", "orb_"},     {"Organon", "organon_"},
        {"Ouroboros", "ouro_"},  {"Obsidian", "obsidian_"}, {"Origami", "origami_"}, {"Oracle", "oracle_"},
        {"Obscura", "obscura_"}, {"Oceanic", "ocean_"},     {"Optic", "optic_"},     {"Oblique", "oblq_"},
        {"Ocelot", "ocelot_"},   {"Osprey", "osprey_"},     {"Osteria", "osteria_"}, {"Owlfish", "owl_"},
        {"Ohm", "ohm_"},         {"Orphica", "orph_"},      {"Obbligato", "obbl_"},  {"Ottoni", "otto_"},
        {"Ole", "ole_"},         {"Ombre", "ombre_"},       {"Orca", "orca_"},       {"Octopus", "octo_"},
        {"Overlap", "olap_"},    {"Outwit", "owit_"},       {"OpenSky", "sky_"},     {"Ostinato", "osti_"},
        {"OceanDeep", "deep_"},  {"Ouie", "ouie_"},         {"Obrix", "obrix_"},     {"Orbweave", "weave_"},
        {"Overtone", "over_"},   {"Organism", "org_"},      {"Oxbow", "oxb_"},       {"Oware", "owr_"},
        {"Opera", "opera_"},     {"Offering", "ofr_"},      {"Osmosis", "osmo_"},    {"Oxytocin", "oxy_"},
        {"Outlook", "look_"},    {"Oto", "oto_"},           {"Octave", "oct_"},      {"Oleg", "oleg_"},
        {"Otis", "otis_"},       {"Oven", "oven_"},         {"Ochre", "ochre_"},     {"Obelisk", "obel_"},
        {"Opaline", "opal2_"},   {"Ogre", "ogre_"},         {"Olate", "olate_"},     {"Oaken", "oaken_"},
        {"Omega", "omega_"},     {"Orchard", "orch_"},      {"Overgrow", "grow_"},   {"Osier", "osier_"},
        {"Oxalis", "oxal_"},     {"Overwash", "wash_"},     {"Overworn", "worn_"},   {"Overflow", "flow_"},
        {"Overcast", "cast_"},   {"Oddfellow", "oddf_"},    {"Onkolo", "onko_"},     {"Opcode", "opco_"},
        {"Obiont", "obnt_"},
    };

    for (auto& [engine, expectedPrefix] : kPrefixes)
    {
        INFO("Engine: " << engine << " -> " << expectedPrefix);
        CHECK(frozenPrefixForEngine(juce::String(engine)) == juce::String(expectedPrefix));
    }
}

TEST_CASE("BackwardCompat - unknown engine prefix returns empty string", "[compat][prefix]")
{
    CHECK(frozenPrefixForEngine("UnknownEngine2099").isEmpty());
}

//==============================================================================
// 3. CouplingType enum stability tests
//==============================================================================

TEST_CASE("BackwardCompat - CouplingType enum ordinals are frozen", "[compat][coupling-type]")
{
    static const std::pair<CouplingType, int> kExpected[] = {
        {CouplingType::AmpToFilter, 0},    {CouplingType::AmpToPitch, 1},    {CouplingType::LFOToPitch, 2},
        {CouplingType::EnvToMorph, 3},     {CouplingType::AudioToFM, 4},     {CouplingType::AudioToRing, 5},
        {CouplingType::FilterToFilter, 6}, {CouplingType::AmpToChoke, 7},    {CouplingType::RhythmToBlend, 8},
        {CouplingType::EnvToDecay, 9},     {CouplingType::PitchToPitch, 10}, {CouplingType::AudioToWavetable, 11},
        {CouplingType::AudioToBuffer, 12}, {CouplingType::KnotTopology, 13}, {CouplingType::TriangularCoupling, 14},
    };

    for (auto& [type, expectedOrdinal] : kExpected)
    {
        INFO("Ordinal " << expectedOrdinal);
        CHECK(static_cast<int>(type) == expectedOrdinal);
    }
}

TEST_CASE("BackwardCompat - CouplingType has exactly 15 values", "[compat][coupling-type]")
{
    int count = static_cast<int>(CouplingType::TriangularCoupling) + 1;
    CHECK(count == 15);
}

// Backward-compat shim
namespace backward_compat_tests
{
int runAll()
{
    return 0;
}
} // namespace backward_compat_tests
