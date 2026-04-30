// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_core/juce_core.h>
#include <map>

//==============================================================================
// EnginePrefix.h — Frozen parameter-prefix lookup for the XOceanus engine fleet.
//
// This header is intentionally minimal so it can be included by both
// SynthEngine.h (the base class) and PresetManager.h (the preset system)
// without creating circular dependencies.
//
// Rule: These prefixes are FROZEN. They cannot change after first release
// because they are baked into preset files (.xometa). Adding new engines is
// fine; renaming existing prefixes is NEVER allowed.
//
// Usage:
//   juce::String prefix = frozenPrefixForEngine("Oasis");  // → "oas_"
//   auto paramId = prefix + "filterCutoff";                 // → "oas_filterCutoff"
//==============================================================================

namespace xoceanus
{

// Returns the frozen parameter prefix for a canonical engine ID.
// Includes the trailing underscore (e.g. "snap_", "oas_").
// Returns an empty string if the engine ID is not recognised.
// RT-safe: the map is a static local — constructed once, never reallocated.
inline juce::String frozenPrefixForEngine(const juce::String& engineId)
{
    static const std::map<juce::String, juce::String> prefixes{
        {"OddfeliX", "snap_"},
        {"OddOscar", "morph_"},
        {"Overdub", "dub_"},
        {"Odyssey", "drift_"},
        {"Oblong", "bob_"},
        {"Obese", "fat_"},
        {"Overbite", "poss_"},
        {"Onset", "perc_"},
        {"Overworld", "ow_"},
        {"Opal", "opal_"},
        {"Orbital", "orb_"},
        {"Organon", "organon_"},
        {"Ouroboros", "ouro_"},
        {"Obsidian", "obsidian_"},
        {"Origami", "origami_"},
        {"Oracle", "oracle_"},
        {"Obscura", "obscura_"},
        {"Oceanic", "ocean_"},
        {"Optic", "optic_"},
        {"Oblique", "oblq_"},
        {"Ocelot", "ocelot_"},
        {"Osprey", "osprey_"},
        {"Osteria", "osteria_"},
        {"Owlfish", "owl_"},
        {"Ohm", "ohm_"},
        {"Orphica", "orph_"},
        {"Obbligato", "obbl_"},
        {"Ottoni", "otto_"},
        {"Ole", "ole_"},
        {"Ombre", "ombre_"},
        {"Orca", "orca_"},
        {"Octopus", "octo_"},
        {"Overlap", "olap_"},
        {"Outwit", "owit_"},
        // V1 Concept Engines
        {"OpenSky", "sky_"},
        {"Ostinato", "osti_"},
        {"OceanDeep", "deep_"},
        {"Ouie", "ouie_"},
        // Flagship
        {"Obrix", "obrix_"},
        // V2 Theorem Engines
        {"Orbweave", "weave_"},
        {"Overtone", "over_"},
        {"Organism", "org_"},
        // Singularity Engines
        {"Oxbow", "oxb_"},
        {"Oware", "owr_"},
        // Kuramoto Vocal Synthesis
        {"Opera", "opera_"},
        // Psychology-Driven Boom Bap Drums
        {"Offering", "ofr_"},
        // Chef Quad Collection
        {"Oto", "oto_"},
        {"Octave", "oct_"},
        {"Oleg", "oleg_"},
        {"Otis", "otis_"},
        // KITCHEN Quad Collection
        {"Oven", "oven_"},
        {"Ochre", "ochre_"},
        {"Obelisk", "obel_"},
        {"Opaline", "opal2_"},
        // CELLAR Quad Collection
        {"Ogive", "ogv_"},
        {"Olvido", "olv_"},
        {"Ostracon", "ostr_"},
        {"Ogre", "ogre_"},
        {"Olate", "olate_"},
        {"Oaken", "oaken_"},
        {"Omega", "omega_"},
        // GARDEN Quad Collection
        {"Orchard", "orch_"},
        {"Overgrow", "grow_"},
        {"Osier", "osier_"},
        {"Oxalis", "oxal_"},
        // BROTH Quad Collection
        {"Overwash", "wash_"},
        {"Overworn", "worn_"},
        {"Overflow", "flow_"},
        {"Overcast", "cast_"},
        // FUSION Quad Collection
        {"Okeanos", "okan_"},
        {"Oddfellow", "oddf_"},
        {"Onkolo", "onko_"},
        {"Opcode", "opco_"},
        // Membrane Collection
        {"Osmosis", "osmo_"},
        // Love Triangle Circuit Synth
        {"Oxytocin", "oxy_"},
        // Panoramic Visionary Synth
        {"Outlook", "look_"},
        // Dual Engine Integration
        {"Oasis", "oas_"},
        {"Outflow", "out_"},
        // Cellular Automata Oscillator
        {"Obiont", "obnt_"},
        // Age-based corrosion synthesis
        {"Oxidize", "oxidize_"},
        // Crystalline Phase Distortion
        {"Observandum", "observ_"},
        // Fleet Navigation Vector Synthesis
        {"Orrery", "orry_"},
        // Bioluminescent Neural Feedback
        {"Opsin", "ops_"},
        // Stochastic Cloud Synthesis
        {"Oort", "oort_"},
        // Formant Vocal Tract Synthesis
        {"Ondine", "ond_"},
        // VOSIM Hierarchical Pulse Synthesis
        {"Ortolan", "ort_"},
        // Tensor Spectral Synthesis
        {"Octant", "octn_"},
        // Wavelet Multi-Resolution Synthesis
        {"Overtide", "ovt_"},
        // Reaction-Diffusion Wavetable Synthesis
        {"Oobleck", "oobl_"},
        // Fluid Dynamics Synthesis
        {"Ooze", "ooze_"},
        // Tape-chamber keyboard synthesis
        {"Ollotron", "ollo_"},
        // NLS Soliton Synthesis (engine #90; renamed from Oneiric 2026-04-22)
        // Prefix stays oner_ — frozen for preset compatibility.
        {"Onda", "oner_"},
        // Wave-Terrain Synthesis
        {"Outcrop", "outc_"},
    };
    auto it = prefixes.find(engineId);
    return (it != prefixes.end()) ? it->second : juce::String();
}

} // namespace xoceanus
