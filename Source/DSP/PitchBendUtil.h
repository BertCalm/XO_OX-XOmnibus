// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "FastMath.h"
#include <cmath>

namespace xoceanus
{

//==============================================================================
// PitchBendUtil — Shared pitch bend processing for the XOceanus fleet.
//
// Consolidates the MIDI pitch wheel → frequency ratio pipeline found across
// 10+ engines. Three functions cover the full chain:
//
//   1. parsePitchWheel()  — MIDI 14-bit → normalized bipolar [-1, +1]
//   2. bendToSemitones()  — normalized × range → semitones
//   3. bendToFreqRatio()  — semitones → frequency multiplier via fastPow2
//
// One-liner for common case:
//   float ratio = PitchBendUtil::freqRatio (msg.getPitchWheelValue(), 2.0f);
//   float freq  = baseFreq * ratio;
//
// All functions are constexpr/inline, noexcept, allocation-free.
//==============================================================================
struct PitchBendUtil
{
    /// Parse JUCE/MIDI 14-bit pitch wheel value [0, 16383] to bipolar [-1, +1].
    /// Center (8192) maps to 0.0. This is the formula used by Obrix, Orbweave,
    /// OceanDeep, and all engines that process isPitchWheel() messages.
    static inline float parsePitchWheel(int midiValue) noexcept
    {
        return static_cast<float>(midiValue - 8192) / 8192.0f;
    }

    /// Convert normalized bend [-1, +1] to semitones given a range.
    /// Default range ±2 semitones matches GM standard.
    static inline float bendToSemitones(float normalizedBend, float rangeSemitones = 2.0f) noexcept
    {
        return normalizedBend * rangeSemitones;
    }

    /// Convert semitones offset to frequency multiplier.
    /// Uses fastPow2 for real-time safety (matches OceanDeep's approach).
    /// Accurate to <0.03% for ±4 semitones. For wide ranges (±12+), prefer
    /// std::pow(2.0f, semitones / 12.0f) — fastPow2 drifts ~3% at ±12st.
    static inline float semitonesToFreqRatio(float semitones) noexcept { return fastPow2(semitones / 12.0f); }

    /// Full pipeline: MIDI pitch wheel value → frequency multiplier.
    /// One-liner replacement for the 3-step pattern found in most engines.
    static inline float freqRatio(int midiPitchWheelValue, float rangeSemitones = 2.0f) noexcept
    {
        float norm = parsePitchWheel(midiPitchWheelValue);
        float semi = bendToSemitones(norm, rangeSemitones);
        return semitonesToFreqRatio(semi);
    }

    /// Convert semitones to cents (for engines that work in cents, e.g. Orbweave).
    static inline float semitonesToCents(float semitones) noexcept { return semitones * 100.0f; }
};

} // namespace xoceanus
