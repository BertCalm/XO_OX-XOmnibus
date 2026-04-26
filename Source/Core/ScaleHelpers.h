// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// ScaleHelpers.h — Scale interval tables + degree-to-note helpers for ChordMachine.
//
// Used by ChordMachine::ScaleDegree input mode to:
//   1. Map an incoming MIDI note number to a scale degree (1-7 for heptatonic scales).
//   2. Derive the chord root for that degree in the current global key.
//
// Scale indices match NoteInputZone::scales[] order so the same index can be stored
// in a single APVTS param (cm_global_scale) and used by both the UI (PlaySurface) and
// the audio engine (ChordMachine), without coupling those two classes.
//
// Scale index table (must stay stable — frozen once persisted):
//   0  Chromatic      — all semitones (degree resolution: returns semitone class)
//   1  Major          — Ionian:     0 2 4 5 7 9 11
//   2  Minor          — Aeolian:    0 2 3 5 7 8 10
//   3  Dorian         — Dorian:     0 2 3 5 7 9 10
//   4  Mixolydian     — Mixolydian: 0 2 4 5 7 9 10
//   5  Pent Minor     — 0 3 5 7 10
//   6  Pent Major     — 0 2 4 7 9
//   7  Blues          — 0 3 5 6 7 10
//   8  Harm Minor     — 0 2 3 5 7 8 11
//
// Thread safety: all functions are pure (no side effects), safe on the audio thread.

#include <array>
#include <cstdint>
#include <algorithm>

namespace xoceanus
{

// Number of scale types (matches NoteInputZone::scales[] count)
static constexpr int kNumScaleTypes = 9;

// Maximum intervals in a single scale (chromatic has 12)
static constexpr int kMaxScaleIntervals = 12;

// Interval tables: semitone offsets from root, ascending, padded with -1.
// Row 0 = Chromatic, Row 1 = Major, ... Row 8 = Harm Minor
// Sizes: [0]=12, [1-4]=7, [5-6]=5, [7]=6, [8]=7
static constexpr int kScaleIntervals[kNumScaleTypes][kMaxScaleIntervals] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, // 0 Chromatic
    { 0, 2, 4, 5, 7, 9, 11,-1,-1,-1,-1,-1 },   // 1 Major
    { 0, 2, 3, 5, 7, 8, 10,-1,-1,-1,-1,-1 },   // 2 Minor
    { 0, 2, 3, 5, 7, 9, 10,-1,-1,-1,-1,-1 },   // 3 Dorian
    { 0, 2, 4, 5, 7, 9, 10,-1,-1,-1,-1,-1 },   // 4 Mixolydian
    { 0, 3, 5, 7, 10,-1,-1,-1,-1,-1,-1,-1 },   // 5 Pent Minor
    { 0, 2, 4, 7,  9,-1,-1,-1,-1,-1,-1,-1 },   // 6 Pent Major
    { 0, 3, 5, 6,  7, 10,-1,-1,-1,-1,-1,-1},   // 7 Blues
    { 0, 2, 3, 5,  7,  8, 11,-1,-1,-1,-1,-1},  // 8 Harm Minor
};

// Number of intervals per scale (degrees / unique notes)
static constexpr int kScaleIntervalCount[kNumScaleTypes] = {
    12, // Chromatic
     7, // Major
     7, // Minor
     7, // Dorian
     7, // Mixolydian
     5, // Pent Minor
     5, // Pent Major
     6, // Blues
     7, // Harm Minor
};

//==============================================================================
// scaleDegreeFromNote — returns the 0-based degree index into the scale's
// interval array for the given MIDI note number, or -1 if not in scale.
//
// For Chromatic (scaleIdx==0), every semitone is degree (semitone class 0-11).
//
// Example: key=0 (C), scaleIdx=1 (Major), note=62 (D4) → D is at semitone 2,
//   kScaleIntervals[1] = {0,2,4,5,7,9,11} → degree index 1 (ii).
//
// Audio-thread safe: pure, no allocation.
inline int scaleDegreeFromNote(int noteNumber, int rootKey, int scaleIdx) noexcept
{
    if (scaleIdx < 0 || scaleIdx >= kNumScaleTypes)
        scaleIdx = 0;
    rootKey = ((rootKey % 12) + 12) % 12;

    const int relSemi = ((noteNumber % 12) - rootKey + 12) % 12;
    const int count = kScaleIntervalCount[scaleIdx];
    for (int d = 0; d < count; ++d)
    {
        if (kScaleIntervals[scaleIdx][d] == relSemi)
            return d;
    }
    // Not in scale — snap to nearest degree below
    int best = 0;
    int bestDist = 12;
    for (int d = 0; d < count; ++d)
    {
        const int dist = (relSemi - kScaleIntervals[scaleIdx][d] + 12) % 12;
        if (dist < bestDist)
        {
            bestDist = dist;
            best = d;
        }
    }
    return best;
}

//==============================================================================
// chordRootForDegree — returns the MIDI note number (relative to the same
// octave range as the incoming note) for the chord root at `degree` in the
// given scale.
//
// Preserves octave: the returned note is in the same octave as `noteNumber`
// (within ±6 semitones — nearest octave reachable).
//
// Audio-thread safe: pure, no allocation.
inline int chordRootForDegree(int degree, int rootKey, int scaleIdx, int referenceNote) noexcept
{
    if (scaleIdx < 0 || scaleIdx >= kNumScaleTypes)
        scaleIdx = 0;
    rootKey = ((rootKey % 12) + 12) % 12;
    degree = std::max(0, std::min(degree, kScaleIntervalCount[scaleIdx] - 1));

    const int interval = kScaleIntervals[scaleIdx][degree];
    // Compute the absolute root in the same octave block as referenceNote
    const int octaveBase = (referenceNote / 12) * 12;
    int candidate = octaveBase + rootKey + interval;
    // Clamp to valid MIDI range
    if (candidate > 127) candidate -= 12;
    if (candidate < 0)   candidate += 12;
    return candidate;
}

//==============================================================================
// isNoteInScale — returns true if noteNumber is in the given scale/key.
// Chromatic always returns true.
inline bool isNoteInScale(int noteNumber, int rootKey, int scaleIdx) noexcept
{
    if (scaleIdx < 0 || scaleIdx >= kNumScaleTypes)
        return true;
    if (scaleIdx == 0)
        return true; // Chromatic
    rootKey = ((rootKey % 12) + 12) % 12;
    const int relSemi = ((noteNumber % 12) - rootKey + 12) % 12;
    const int count = kScaleIntervalCount[scaleIdx];
    for (int d = 0; d < count; ++d)
        if (kScaleIntervals[scaleIdx][d] == relSemi)
            return true;
    return false;
}

// Scale degree quality hint: is the degree a "major" or "minor" context?
// Returns true if the degree is considered major-quality in the scale.
// Used by ScaleDegree mode to pick chord voicings that match scale function.
// Simplified: even-numbered intervals from root are typically major-quality.
inline bool isDegreeMinorQuality(int degree, int scaleIdx) noexcept
{
    if (scaleIdx <= 0 || scaleIdx >= kNumScaleTypes) return false;
    if (degree < 0 || degree >= kScaleIntervalCount[scaleIdx]) return false;
    const int interval = kScaleIntervals[scaleIdx][degree];
    // A minor third above the previous degree indicates minor quality chord.
    // Check: is the interval between degree and degree+1 a minor 3rd (3 semitones)?
    // Simpler heuristic: degrees with flat 3rd (interval mod 12 in {3,8,10}) are minor.
    // Major chord degrees have pure major third = 4 semitones above.
    // We detect this from the scale's step pattern.
    if (degree + 1 < kScaleIntervalCount[scaleIdx])
    {
        const int nextInterval = kScaleIntervals[scaleIdx][degree + 1];
        const int step = nextInterval - interval;
        return (step == 1); // half-step = minor quality leading tone (rare heuristic)
    }
    // Fallback: use interval from root: 0 (I) and 4 (V) are major, 2 (iii) and 5 (vi) are minor
    static constexpr bool kMajorMinorFallback[7] = { false, true, true, false, false, true, true };
    if (degree < 7) return kMajorMinorFallback[degree];
    return false;
}

// Scale name lookup (matches scale index order)
inline const char* scaleName(int scaleIdx) noexcept
{
    static constexpr const char* kNames[kNumScaleTypes] = {
        "Chromatic", "Major", "Minor", "Dorian", "Mixolydian",
        "Pent Min", "Pent Maj", "Blues", "Harm Min"
    };
    if (scaleIdx >= 0 && scaleIdx < kNumScaleTypes)
        return kNames[scaleIdx];
    return "?";
}

// Roman numeral labels for scale degrees (7-degree max)
inline const char* degreeRomanNumeral(int degree, int scaleIdx, bool minorQuality) noexcept
{
    static constexpr const char* kMajorRoman[7] = { "I","ii","iii","IV","V","vi","vii\xC2\xB0" };
    static constexpr const char* kMinorRoman[7] = { "i","ii\xC2\xB0","III","iv","v","VI","VII" };
    (void)scaleIdx;
    if (degree < 0 || degree > 6) return "?";
    return minorQuality ? kMinorRoman[degree] : kMajorRoman[degree];
}

} // namespace xoceanus
