#pragma once

/*
    HarmonicField.h
    ================
    Pure C++ header (zero JUCE dependency) implementing circle-of-fifths math,
    tension coloring, scale membership, and marker properties for the XOuija
    harmonic navigation system.

    Namespace: xolokun
    C++ standard: C++17
*/

#include <array>
#include <cmath>
#include <algorithm>
#include <tuple>
#include <utility>

namespace xolokun {

struct HarmonicField
{
    // =========================================================================
    // Data Tables
    // =========================================================================

    // 13 positions on the circle of fifths (Gb through F#, with C at center/idx 6).
    // Entry = semitone offset from C (0=C, 1=C#, 2=D, 3=D#, 4=E, 5=F,
    //         6=F#, 7=G, 8=G#, 9=A, 10=A#, 11=B).
    // Order: Gb, Db, Ab, Eb, Bb, F, C, G, D, A, E, B, F#
    static constexpr std::array<int, 13> kFifthsSemitones =
        { 6, 1, 8, 3, 10, 5, 0, 7, 2, 9, 4, 11, 6 };

    static constexpr std::array<const char*, 13> kNoteNames =
        { "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#" };

    // Major scale intervals (semitones from root): W W H W W W H
    static constexpr std::array<int, 7> kMajorScale =
        { 0, 2, 4, 5, 7, 9, 11 };

    // =========================================================================
    // Circle of Fifths Mapping
    // =========================================================================

    static int positionToKey(float x) noexcept
    {
        x = std::max(0.0f, std::min(1.0f, x));
        float fi = x * 12.0f;
        int idx = static_cast<int>(fi + 0.5f);
        idx = std::max(0, std::min(12, idx));
        return kFifthsSemitones[static_cast<std::size_t>(idx)];
    }

    static int positionToFifthsOffset(float x) noexcept
    {
        x = std::max(0.0f, std::min(1.0f, x));
        float fi = x * 12.0f;
        int idx = static_cast<int>(fi + 0.5f);
        idx = std::max(0, std::min(12, idx));
        return idx - 6;
    }

    static float fifthsOffsetToPosition(int offset) noexcept
    {
        offset = std::max(-6, std::min(6, offset));
        return static_cast<float>(offset + 6) / 12.0f;
    }

    static int fifthsDistance(int keyA, int keyB) noexcept
    {
        keyA = ((keyA % 12) + 12) % 12;
        keyB = ((keyB % 12) + 12) % 12;
        int fA = (keyA * 7) % 12;
        int fB = (keyB * 7) % 12;
        int diff = std::abs(fA - fB);
        return std::min(diff, 12 - diff);
    }

    // =========================================================================
    // Tension Coloring
    // =========================================================================

    static std::tuple<float, float, float> tensionColor(int fifthsDist) noexcept
    {
        fifthsDist = std::max(0, std::min(6, fifthsDist));

        constexpr float tealR = 0x2A / 255.0f, tealG = 0x9D / 255.0f, tealB = 0x8F / 255.0f;
        constexpr float goldR = 0xE9 / 255.0f, goldG = 0xC4 / 255.0f, goldB = 0x6A / 255.0f;
        constexpr float redR  = 0xE0 / 255.0f, redG  = 0x7A / 255.0f, redB  = 0x5F / 255.0f;

        float r, g, b;
        if (fifthsDist <= 3)
        {
            float t = static_cast<float>(fifthsDist) / 3.0f;
            r = tealR + t * (goldR - tealR);
            g = tealG + t * (goldG - tealG);
            b = tealB + t * (goldB - tealB);
        }
        else
        {
            float t = static_cast<float>(fifthsDist - 3) / 3.0f;
            r = goldR + t * (redR - goldR);
            g = goldG + t * (redG - goldG);
            b = goldB + t * (redB - goldB);
        }

        return { r, g, b };
    }

    // =========================================================================
    // Marker Properties
    // =========================================================================

    static std::pair<float, float> markerProperties(int fifthsDist) noexcept
    {
        fifthsDist = std::max(0, std::min(6, fifthsDist));
        if (fifthsDist == 0)    return { 1.00f, 1.00f };
        if (fifthsDist <= 2)    return { 0.85f, 0.75f };
        if (fifthsDist <= 4)    return { 0.70f, 0.50f };
        /* fifthsDist 5 or 6 */ return { 0.55f, 0.35f };
    }

    // =========================================================================
    // Marker Arc
    // =========================================================================

    static float markerArcY(int fifthsIdx, float amplitude = 8.0f) noexcept
    {
        fifthsIdx = std::max(0, std::min(12, fifthsIdx));
        float norm = static_cast<float>(fifthsIdx - 6) / 6.0f;
        return amplitude * (2.0f * norm * norm - 1.0f);
    }

    // =========================================================================
    // Scale Membership
    // =========================================================================

    static bool isInKey(int midiNote, int rootKey) noexcept
    {
        int semitone = ((midiNote % 12) + 12) % 12;
        int root     = ((rootKey  % 12) + 12) % 12;
        int interval = (semitone - root + 12) % 12;
        for (int s : kMajorScale)
            if (s == interval) return true;
        return false;
    }

    static bool isRoot(int midiNote, int rootKey) noexcept
    {
        int semitone = ((midiNote % 12) + 12) % 12;
        int root     = ((rootKey  % 12) + 12) % 12;
        return semitone == root;
    }

    static int quantizeToNearest(int midiNote, int rootKey) noexcept
    {
        if (isInKey(midiNote, rootKey)) return midiNote;

        for (int delta = 1; delta <= 6; ++delta)
        {
            if (isInKey(midiNote + delta, rootKey)) return midiNote + delta;
            if (isInKey(midiNote - delta, rootKey)) return midiNote - delta;
        }
        return midiNote;
    }
};

// Out-of-class constexpr definitions (required for C++14 linkage; C++17 inline)
constexpr std::array<int, 13>          HarmonicField::kFifthsSemitones;
constexpr std::array<const char*, 13>  HarmonicField::kNoteNames;
constexpr std::array<int, 7>           HarmonicField::kMajorScale;

} // namespace xolokun
