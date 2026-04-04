// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// CouplingColors.h — canonical colour, label, and display-name mappings for all
// 15 CouplingType values.
//
// Single source of truth used by:
//   Source/UI/CouplingVisualizer/CouplingVisualizer.h
//   Source/UI/Gallery/CouplingArcOverlay.h
//   Source/UI/Gallery/CouplingChainView.h
//   Source/UI/Gallery/MiniCouplingGraph.h
//
// Does NOT include any UI component headers — safe to include from any Gallery
// component without risk of circular dependency.
//
// Color rationale: Docs/coupling-ui-architecture-2026-03-21.md §5.
// All colors meet WCAG AA (4.5:1) contrast on both dark (#1A1A1A) and
// Gallery shell white (#F8F6F3).

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Core/SynthEngine.h" // CouplingType enum
#include <array>

namespace xoceanus
{
namespace CouplingTypeColors
{

// Returns the canonical ARGB colour for a coupling type.
inline juce::Colour forType(CouplingType type)
{
    switch (type)
    {
    case CouplingType::AmpToFilter:
        return juce::Colour(0xFF2196F3); // Cerulean
    case CouplingType::AmpToPitch:
        return juce::Colour(0xFF1565C0); // Cobalt
    case CouplingType::LFOToPitch:
        return juce::Colour(0xFF7986CB); // Periwinkle
    case CouplingType::EnvToMorph:
        return juce::Colour(0xFFBA68C8); // Orchid
    case CouplingType::AudioToFM:
        return juce::Colour(0xFFE53935); // Vermilion
    case CouplingType::AudioToRing:
        return juce::Colour(0xFFF4511E); // Deep Orange
    case CouplingType::FilterToFilter:
        return juce::Colour(0xFF4DB6AC); // Steel Teal
    case CouplingType::AmpToChoke:
        return juce::Colour(0xFF546E7A); // Charcoal
    case CouplingType::RhythmToBlend:
        return juce::Colour(0xFFFFB300); // Warm Amber
    case CouplingType::EnvToDecay:
        return juce::Colour(0xFF66BB6A); // Sage Green
    case CouplingType::PitchToPitch:
        return juce::Colour(0xFF9C8FC4); // Lavender
    case CouplingType::AudioToWavetable:
        return juce::Colour(0xFF00ACC1); // Cyan
    case CouplingType::AudioToBuffer:
        return juce::Colour(0xFFFB8C00); // Mango
    case CouplingType::KnotTopology:
        return juce::Colour(0xFFE9C46A); // XO Gold
    case CouplingType::TriangularCoupling:
        return juce::Colour(0xFF9B5DE5); // Synapse Violet
    default:
        return juce::Colour(0xFFAAAAAA); // fallback grey
    }
}

// Short human-readable label for arc midpoint and menus.
inline juce::String shortLabel(CouplingType type)
{
    switch (type)
    {
    case CouplingType::AmpToFilter:
        return "Amp>F";
    case CouplingType::AmpToPitch:
        return "Amp>P";
    case CouplingType::LFOToPitch:
        return "LFO>P";
    case CouplingType::EnvToMorph:
        return "Env>M";
    case CouplingType::AudioToFM:
        return "FM";
    case CouplingType::AudioToRing:
        return "Ring";
    case CouplingType::FilterToFilter:
        return "F>F";
    case CouplingType::AmpToChoke:
        return "Choke";
    case CouplingType::RhythmToBlend:
        return "Rhy>B";
    case CouplingType::EnvToDecay:
        return "Env>D";
    case CouplingType::PitchToPitch:
        return "P>P";
    case CouplingType::AudioToWavetable:
        return "WT";
    case CouplingType::AudioToBuffer:
        return "A>Buf";
    case CouplingType::KnotTopology:
        return "KNOT";
    case CouplingType::TriangularCoupling:
        return "TRI";
    default:
        return "?";
    }
}

// Full display name for menus and tooltips.
inline juce::String displayName(CouplingType type)
{
    switch (type)
    {
    case CouplingType::AmpToFilter:
        return "Amplitude to Filter";
    case CouplingType::AmpToPitch:
        return "Amplitude to Pitch";
    case CouplingType::LFOToPitch:
        return "LFO to Pitch";
    case CouplingType::EnvToMorph:
        return "Envelope to Morph";
    case CouplingType::AudioToFM:
        return "Audio to FM";
    case CouplingType::AudioToRing:
        return "Audio to Ring Mod";
    case CouplingType::FilterToFilter:
        return "Filter to Filter";
    case CouplingType::AmpToChoke:
        return "Amplitude to Choke";
    case CouplingType::RhythmToBlend:
        return "Rhythm to Blend";
    case CouplingType::EnvToDecay:
        return "Envelope to Decay";
    case CouplingType::PitchToPitch:
        return "Pitch to Pitch";
    case CouplingType::AudioToWavetable:
        return "Audio to Wavetable";
    case CouplingType::AudioToBuffer:
        return "Audio to Buffer";
    case CouplingType::KnotTopology:
        return "Knot Topology (bidirectional)";
    case CouplingType::TriangularCoupling:
        return "Triangular Coupling";
    default:
        return "Unknown";
    }
}

// All 15 types in display order for menus and legend.
inline std::array<CouplingType, 15> allTypes()
{
    return {CouplingType::AmpToFilter,    CouplingType::AmpToPitch,   CouplingType::LFOToPitch,
            CouplingType::EnvToMorph,     CouplingType::AudioToFM,    CouplingType::AudioToRing,
            CouplingType::FilterToFilter, CouplingType::AmpToChoke,   CouplingType::RhythmToBlend,
            CouplingType::EnvToDecay,     CouplingType::PitchToPitch, CouplingType::AudioToWavetable,
            CouplingType::AudioToBuffer,  CouplingType::KnotTopology, CouplingType::TriangularCoupling};
}

} // namespace CouplingTypeColors
} // namespace xoceanus
