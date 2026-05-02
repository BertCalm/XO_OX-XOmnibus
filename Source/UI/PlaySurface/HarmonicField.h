// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

/*
    HarmonicField.h
    ================
    Pure C++ header (zero JUCE dependency) implementing circle-of-fifths math,
    tension coloring, scale membership, and marker properties for the
    harmonic navigation system.

    Namespace: xoceanus
    C++ standard: C++17
*/

#include <array>
#include <cmath>
#include <algorithm>
#include <tuple>
#include <utility>

namespace xoceanus
{

struct HarmonicField
{
    // =========================================================================
    // Data Tables
    // =========================================================================

    // 13 positions on the circle of fifths (Gb through F#, with C at center/idx 6).
    // Entry = semitone offset from C (0=C, 1=C#, 2=D, 3=D#, 4=E, 5=F,
    //         6=F#, 7=G, 8=G#, 9=A, 10=A#, 11=B).
    // Order: Gb, Db, Ab, Eb, Bb, F, C, G, D, A, E, B, F#
    static constexpr std::array<int, 13> kFifthsSemitones = {6, 1, 8, 3, 10, 5, 0, 7, 2, 9, 4, 11, 6};

    static constexpr std::array<const char*, 13> kNoteNames = {"Gb", "Db", "Ab", "Eb", "Bb", "F", "C",
                                                               "G",  "D",  "A",  "E",  "B",  "F#"};

    // Major scale intervals (semitones from root): W W H W W W H
    static constexpr std::array<int, 7> kMajorScale = {0, 2, 4, 5, 7, 9, 11};

    // =========================================================================
    // Circle of Fifths Mapping
    // =========================================================================

    // Convert a normalized horizontal position (0.0=Gb, 0.5=C, 1.0=F#) to
    // the semitone value of the nearest fifths position (0=C, 1=C#, ..., 11=B).
    // The 13 positions are evenly spaced in [0, 1]; we clamp + round to nearest.
    static int positionToKey(float x) noexcept
    {
        // Clamp to [0, 1]
        x = std::max(0.0f, std::min(1.0f, x));
        // Map to [0, 12] and round to nearest index
        float fi = x * 12.0f;
        int idx = static_cast<int>(fi + 0.5f);
        idx = std::max(0, std::min(12, idx));
        return kFifthsSemitones[static_cast<std::size_t>(idx)];
    }

    // Return the fifths offset for a normalized position (-6 at Gb, 0 at C, +6 at F#).
    static int positionToFifthsOffset(float x) noexcept
    {
        x = std::max(0.0f, std::min(1.0f, x));
        float fi = x * 12.0f;
        int idx = static_cast<int>(fi + 0.5f);
        idx = std::max(0, std::min(12, idx));
        // Idx 6 → offset 0 (C), idx 0 → offset -6 (Gb), idx 12 → offset +6 (F#)
        return idx - 6;
    }

    // Convert a fifths offset (-6 to +6) to a normalized position in [0, 1].
    static float fifthsOffsetToPosition(int offset) noexcept
    {
        offset = std::max(-6, std::min(6, offset));
        // idx = offset + 6; position = idx / 12
        return static_cast<float>(offset + 6) / 12.0f;
    }

    // Shortest path on the circle of fifths between two semitone keys.
    // Returns an integer in [0, 6].
    // keyA and keyB are semitone values (0=C, 1=C#, ..., 11=B).
    static int fifthsDistance(int keyA, int keyB) noexcept
    {
        // Reduce semitone keys to their position index on the fifths circle.
        // The circle (without duplicate F#/Gb) has 12 unique positions [0..11].
        // We need the shortest distance in fifths steps (wrapping at 6).
        keyA = ((keyA % 12) + 12) % 12;
        keyB = ((keyB % 12) + 12) % 12;

        // Convert semitone to fifths index (position in C G D A E B F# Gb Db Ab Eb Bb)
        // Going up a fifth = +7 semitones; the fifths circle maps semitone → step.
        // To find fifths distance, compute steps on the circle: multiply by 5 mod 12
        // (5 is the inverse of 7 mod 12, since 7*5=35≡11≡-1 ... actually 7*7=49≡1 mod 12,
        // so the inverse of 7 mod 12 is 7).  One step up the circle of fifths = +7 semitones.
        // Fifths index of a semitone value s: fIdx(s) = (s * 7) % 12
        // (C=0→0, G=7→49%12=1, D=2→14%12=2, A=9→63%12=3, E=4→28%12=4, B=11→77%12=5,
        //  F#=6→42%12=6, Db=1→7%12=7, Ab=8→56%12=8, Eb=3→21%12=9, Bb=10→70%12=10, F=5→35%12=11)
        int fA = (keyA * 7) % 12;
        int fB = (keyB * 7) % 12;
        int diff = std::abs(fA - fB);
        return std::min(diff, 12 - diff);
    }

    // =========================================================================
    // Tension Coloring
    // =========================================================================

    // Return an RGB color tuple for a given fifths distance (0–6).
    // 0 (home/tonic) → Teal #2A9D8F
    // 3 (mid tension) → XO Gold #E9C46A
    // 6 (max tension) → Warm Red #E07A5F
    // Linear interpolation between the three anchor colors.
    static std::tuple<float, float, float> tensionColor(int fifthsDist) noexcept
    {
        fifthsDist = std::max(0, std::min(6, fifthsDist));

        // Anchor colors (normalized 0–1)
        constexpr float tealR = 0x2A / 255.0f, tealG = 0x9D / 255.0f, tealB = 0x8F / 255.0f;
        constexpr float goldR = 0xE9 / 255.0f, goldG = 0xC4 / 255.0f, goldB = 0x6A / 255.0f;
        constexpr float redR = 0xE0 / 255.0f, redG = 0x7A / 255.0f, redB = 0x5F / 255.0f;

        float r, g, b;
        if (fifthsDist <= 3)
        {
            // Teal → Gold  (t goes from 0.0 at dist=0 to 1.0 at dist=3)
            float t = static_cast<float>(fifthsDist) / 3.0f;
            r = tealR + t * (goldR - tealR);
            g = tealG + t * (goldG - tealG);
            b = tealB + t * (goldB - tealB);
        }
        else
        {
            // Gold → Red  (t goes from 0.0 at dist=3 to 1.0 at dist=6)
            float t = static_cast<float>(fifthsDist - 3) / 3.0f;
            r = goldR + t * (redR - goldR);
            g = goldG + t * (redG - goldG);
            b = goldB + t * (redB - goldB);
        }

        return {r, g, b};
    }

    // =========================================================================
    // Marker Properties
    // =========================================================================

    // Return {size, opacity} for a marker at a given fifths distance.
    //   dist 0:   {1.00, 1.00}  (home key — full size and full opacity)
    //   dist 1-2: {0.85, 0.75}
    //   dist 3-4: {0.70, 0.50}
    //   dist 5-6: {0.55, 0.35}
    static std::pair<float, float> markerProperties(int fifthsDist) noexcept
    {
        fifthsDist = std::max(0, std::min(6, fifthsDist));
        if (fifthsDist == 0)
            return {1.00f, 1.00f};
        if (fifthsDist <= 2)
            return {0.85f, 0.75f};
        if (fifthsDist <= 4)
            return {0.70f, 0.50f};
        /* fifthsDist 5 or 6 */ return {
            0.65f, 0.50f}; // WCAG Fix 4: raised from {0.55, 0.35} to meet WCAG 1.4.11 non-text contrast 3:1
    }

    // =========================================================================
    // Marker Arc
    // =========================================================================

    // Vertical offset for a parabolic arc across all 13 markers.
    // idx 0 and idx 12 (edges) are at +amplitude (high).
    // idx 6 (center, C) is at -amplitude (low).
    // Formula: amplitude * (2 * norm^2 - 1)  where norm = (idx - 6) / 6.0
    static float markerArcY(int fifthsIdx, float amplitude = 8.0f) noexcept
    {
        fifthsIdx = std::max(0, std::min(12, fifthsIdx));
        float norm = static_cast<float>(fifthsIdx - 6) / 6.0f;
        return amplitude * (2.0f * norm * norm - 1.0f);
    }

    // =========================================================================
    // Scale Membership
    // =========================================================================

    // Return true if midiNote is a member of the major scale rooted at rootKey.
    // rootKey is a semitone value (0=C, 1=C#, ..., 11=B).
    static bool isInKey(int midiNote, int rootKey) noexcept
    {
        int semitone = ((midiNote % 12) + 12) % 12;
        int root = ((rootKey % 12) + 12) % 12;
        int interval = (semitone - root + 12) % 12;
        for (int s : kMajorScale)
            if (s == interval)
                return true;
        return false;
    }

    // Return true if midiNote is the root of the given key (octave-independent).
    static bool isRoot(int midiNote, int rootKey) noexcept
    {
        int semitone = ((midiNote % 12) + 12) % 12;
        int root = ((rootKey % 12) + 12) % 12;
        return semitone == root;
    }

    // Return the nearest diatonic note in the given major key.
    // If midiNote is already in key, it is returned unchanged.
    // Otherwise, checks +1 and -1 semitone alternately, expanding outward.
    static int quantizeToNearest(int midiNote, int rootKey) noexcept
    {
        if (isInKey(midiNote, rootKey))
            return midiNote;

        // Search outward symmetrically: ±1, ±2, ...
        for (int delta = 1; delta <= 6; ++delta)
        {
            if (isInKey(midiNote + delta, rootKey))
                return midiNote + delta;
            if (isInKey(midiNote - delta, rootKey))
                return midiNote - delta;
        }
        // Should never reach here for a valid 7-note scale
        return midiNote;
    }
};

// Note: static constexpr members are implicitly inline in C++17 and do NOT
// require out-of-class definitions. Providing them causes ODR violations when
// this header is included in multiple translation units. They have been removed.

} // namespace xoceanus
