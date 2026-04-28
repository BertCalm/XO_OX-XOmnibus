// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

namespace xoceanus { namespace aging {

//==============================================================================
// Aging convention — STUB
//
// Phase 0 scaffolds this header to lock the namespace and the AGE convention.
// Concrete helper functions are deferred until Pack 2 (Analog Warmth) becomes
// the first consumer; the warmth pack will use these to age tape oxide,
// spring tension, and tube bias.
//
// AGE convention (locked decision D5, 2026-04-27):
//   - AGE is a normalised scalar in [0, 1].
//   - 0 = factory-fresh; 1 = maximally aged (oxide stripped, springs sagged,
//     tube bias drifted to spec edge).
//   - AGE defaults to 0 when nothing is coupled to it. Auto-driving by
//     wall-clock is forbidden (would make presets non-deterministic across
//     sessions).
//   - To unlock the wildcard, the user explicitly couples AGE to:
//       * a slow ≤0.01 Hz LFO (preset-deterministic)
//       * a partner engine's MOVEMENT macro (musically driven)
//       * the host transport time (project-deterministic)
//
// Cross-cutting principle: wildcards are opt-in, not opt-out.
//
struct AgeContext
{
    float age = 0.0f;          // [0, 1], smoothed
    bool  isCoupled = false;   // true if any input drives age
};

//-- Stub helpers (Pack 2 will provide concrete curves) ------------------------

// Apply standard aging curve to a filter cutoff. Higher AGE darkens cutoff.
// Stub implementation — Pack 2 will tune the curve from real measurements.
inline float applyToFilterCutoff(float baseCutoff, float age) noexcept
{
    const float clamped = juce::jlimit(0.0f, 1.0f, age);
    return baseCutoff * (1.0f - 0.3f * clamped);
}

// Apply standard aging curve to saturation amount. Higher AGE adds asymmetry.
inline float applyToSaturation(float baseSat, float age) noexcept
{
    const float clamped = juce::jlimit(0.0f, 1.0f, age);
    return baseSat + 0.2f * clamped;
}

// Apply standard aging curve to wow/flutter depth. Higher AGE increases warble.
inline float applyToWowFlutter(float baseDepth, float age) noexcept
{
    const float clamped = juce::jlimit(0.0f, 1.0f, age);
    return baseDepth * (1.0f + 2.0f * clamped);
}

}} // namespace xoceanus::aging
