// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

/*
    XOuijaCell.h
    ============
    Data model for the XOuija multi-layer cell grid (Wave 5, D1 / issue #1287).

    Decision source: D8 A4 (FULLY LOCKED 2026-04-25).

    Each cell in the 8×8 XOuija grid carries three independent layers:

        ChordLayer     — harmonic material (chord type + root offset + octave)
        RhythmDensity  — gate-event density [0, 1]  (sparse → dense)
        TextureVector  — timbral signature, 2D brightness × motion [0, 1]

    When the XOuijaWalkEngine advances the planchette to a new cell, all three
    layers are emitted simultaneously to their configured output systems
    (see XOuijaOutputRouter and XOuijaOutputMode).

    Persistence: ValueTree, NOT APVTS.  64 cells × 3 layers ≈ 192+ values is
    too many APVTS parameters.  Grid contents are editorial state, not automatable.
    The "XOuijaGrid" ValueTree child lives under "XOuijaPanel" in apvts.state.
    (See spec section 6 and XOceanusProcessor getStateInformation / setStateInformation.)

    Thread safety: XOuijaCell instances are owned by XOuijaWalkEngine (audio thread).
    ValueTree serialization is message-thread-only (called only from
    getStateInformation / setStateInformation).

    JUCE 8, C++17 — no STL types with heap allocations on the audio path.
    Spec: Docs/specs/wave5-d1-xouija-multilayer-cells.md
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cstdint>
#include <algorithm>

namespace xoceanus {

//==============================================================================
// ChordType — harmonic vocabulary for XOuija cells.
//
// Matches D7 ChordMachine voicing palette: Tertian + Quartal/Quintal + Modal-world.
// 12-TET only; microtonal support deferred to v2 per locked D7 spec.
//
// COUNT must remain the last entry — it drives array sizing and validation.
//
enum class ChordType : uint8_t
{
    // Tertian (Western common-practice)
    Maj = 0,    // Major triad
    Min,        // Minor triad
    Dom7,       // Dominant seventh
    Maj7,       // Major seventh
    Min7,       // Minor seventh
    Dim,        // Diminished triad
    Aug,        // Augmented triad
    Sus2,       // Suspended 2nd
    Sus4,       // Suspended 4th
    Add9,       // Add-9 (no 7th)
    Min9,       // Minor ninth
    Maj9,       // Major ninth
    Dom9,       // Dominant ninth
    HalfDim,    // Half-diminished (minor 7 flat 5)

    // Quartal / Quintal
    Quartal3,   // 3 stacked perfect fourths
    Quartal4,   // 4 stacked perfect fourths
    Quintal3,   // 3 stacked perfect fifths

    // Modal-world (root + characteristic intervals, 12-TET)
    Hijaz,      // Phrygian dominant: b2 + M3 + P5 (flamenco / Maqam Hijaz)
    Bhairavi,   // Carnatic: b2 + m3 + P5 (similar to Phrygian)
    YoScale,    // Japanese pentatonic major
    InScale,    // Japanese pentatonic minor

    // Special-case
    Unison,     // Single root note only (no chord stacking)
    Open5,      // Power chord: root + P5, no third

    Count       // Sentinel — keep last
};

//==============================================================================
// ChordLayer — harmonic identity of a cell.
//
// root:   semitone offset from the global key root.  Range [-6, 6].
//         0 = root of the key.  7 = perfect fifth above.  -5 = perfect fourth above.
// octave: MIDI octave for chord voicing.  Range [2, 7].  Default = 4 (middle octave).
//
struct ChordLayer
{
    ChordType type   = ChordType::Maj;
    int8_t    root   = 0;    // semitone offset from global key root, [-6, 6]
    uint8_t   octave = 4;    // MIDI octave for voicing, [2, 7]

    [[nodiscard]] juce::ValueTree toValueTree() const
    {
        juce::ValueTree t { "Chord" };
        t.setProperty("type",   static_cast<int>(type),  nullptr);
        t.setProperty("root",   static_cast<int>(root),  nullptr);
        t.setProperty("octave", static_cast<int>(octave), nullptr);
        return t;
    }

    bool fromValueTree(const juce::ValueTree& t)
    {
        if (!t.isValid()) return false;
        const int rawType = t.getProperty("type", static_cast<int>(ChordType::Maj));
        type   = static_cast<ChordType>(
                     juce::jlimit(0, static_cast<int>(ChordType::Count) - 1, rawType));
        root   = static_cast<int8_t>(
                     juce::jlimit(-6, 6, static_cast<int>(t.getProperty("root",   0))));
        octave = static_cast<uint8_t>(
                     juce::jlimit(2,  7, static_cast<int>(t.getProperty("octave", 4))));
        return true;
    }

    bool operator==(const ChordLayer& o) const noexcept
    {
        return type == o.type && root == o.root && octave == o.octave;
    }
    bool operator!=(const ChordLayer& o) const noexcept { return !(*this == o); }
};

//==============================================================================
// RhythmDensity — gate-event density for a cell.
//
// value:  normalized [0.0, 1.0].
//   0.0 = complete rest (no gate events emitted from this cell).
//   0.5 = moderate (interpreted as quarter-note probability or fixed quarter subdivisions).
//   1.0 = maximum density (16th-note events or full gate probability).
//
// The PerEnginePatternSequencer (Wave5-C1) consumes this when XOuijaOutputMode
// is DriveSeq — it scales the sequencer's gate probability per cell visit.
//
struct RhythmDensity
{
    float value = 0.5f;   // [0.0, 1.0]

    [[nodiscard]] juce::ValueTree toValueTree() const
    {
        juce::ValueTree t { "Rhythm" };
        t.setProperty("density", value, nullptr);
        return t;
    }

    bool fromValueTree(const juce::ValueTree& t)
    {
        if (!t.isValid()) return false;
        value = juce::jlimit(0.0f, 1.0f, static_cast<float>(t.getProperty("density", 0.5f)));
        return true;
    }

    bool operator==(const RhythmDensity& o) const noexcept { return value == o.value; }
    bool operator!=(const RhythmDensity& o) const noexcept { return !(*this == o); }
};

//==============================================================================
// TextureVector — 2D timbral signature emitted as mod sources.
//
// brightness:  0.0 = dark / filtered.  1.0 = bright / open.
//              Mapped to filter cutoff shift (or engine-specific param per D5 routing).
//
// motion:      0.0 = static / sustained.  1.0 = animated / tremolo.
//              Mapped to LFO depth or mod rate (engine-specific per D5 routing).
//
// When XOuijaOutputMode is ModSource, both values are dispatched as mod matrix
// sources by XOuijaOutputRouter (XOuijaModSource::Brightness / ::Motion).
//
struct TextureVector
{
    float brightness = 0.5f;   // [0.0, 1.0]
    float motion     = 0.5f;   // [0.0, 1.0]

    [[nodiscard]] juce::ValueTree toValueTree() const
    {
        juce::ValueTree t { "Texture" };
        t.setProperty("brightness", brightness, nullptr);
        t.setProperty("motion",     motion,     nullptr);
        return t;
    }

    bool fromValueTree(const juce::ValueTree& t)
    {
        if (!t.isValid()) return false;
        brightness = juce::jlimit(0.0f, 1.0f,
            static_cast<float>(t.getProperty("brightness", 0.5f)));
        motion     = juce::jlimit(0.0f, 1.0f,
            static_cast<float>(t.getProperty("motion", 0.5f)));
        return true;
    }

    bool operator==(const TextureVector& o) const noexcept
    {
        return brightness == o.brightness && motion == o.motion;
    }
    bool operator!=(const TextureVector& o) const noexcept { return !(*this == o); }
};

//==============================================================================
// XOuijaOutputMode — per-engine-slot routing choice.
//
// Stored as 4 int APVTS params: "ouija_route_0" .. "ouija_route_3".
// Values match the spec (section 5) exactly — do not reorder.
//
enum class XOuijaOutputMode : uint8_t
{
    Off        = 0,   // XOuija does not affect this engine slot
    DriveNotes = 1,   // chord layer → MIDI notes through ChordMachine (D7)
    DriveSeq   = 2,   // rhythm-density layer → sequencer gate probability (D6)
    DriveChord = 3,   // chord type + root → ChordMachine root/voicing override (D7)
    ModSource  = 4,   // texture-vector → 2 mod matrix sources (brightness, motion) (D9)
};

//==============================================================================
// XOuijaCell — one cell in the 8×8 XOuija grid.
//
// Default construction produces a "neutral" cell:
//   C major chord | moderate rhythm density | mid brightness/motion.
//
// pinned: when true, the XOuijaWalkEngine excludes this cell from random selection.
//         Pinned cells may only be reached by a DirectWalk or forced-step.
//         (D8-C3: pinned cells block planchette movement through them.)
//
struct XOuijaCell
{
    ChordLayer    chord   {};
    RhythmDensity rhythm  {};
    TextureVector texture {};
    bool          pinned  = false;

    [[nodiscard]] juce::ValueTree toValueTree(int index) const
    {
        juce::ValueTree t { "Cell" };
        t.setProperty("index",  index,              nullptr);
        t.setProperty("pinned", pinned ? 1 : 0,     nullptr);
        t.appendChild(chord.toValueTree(),           nullptr);
        t.appendChild(rhythm.toValueTree(),          nullptr);
        t.appendChild(texture.toValueTree(),         nullptr);
        return t;
    }

    bool fromValueTree(const juce::ValueTree& t)
    {
        if (!t.isValid()) return false;
        pinned = (static_cast<int>(t.getProperty("pinned", 0)) != 0);
        chord.fromValueTree(t.getChildWithName("Chord"));
        rhythm.fromValueTree(t.getChildWithName("Rhythm"));
        texture.fromValueTree(t.getChildWithName("Texture"));
        return true;
    }

    bool operator==(const XOuijaCell& o) const noexcept
    {
        return chord == o.chord && rhythm == o.rhythm &&
               texture == o.texture && pinned == o.pinned;
    }
    bool operator!=(const XOuijaCell& o) const noexcept { return !(*this == o); }
};

//==============================================================================
// XOuijaCellGrid — the 8×8 grid of cells plus the ephemeral heatmap.
//
// Rationale for 8×8 (64 cells):
//   - 4×4 (16 cells) = too few for interesting probabilistic walks.
//   - 16×16 (256 cells) = overwhelming to edit; heatmap illegible at UI scale.
//   - 8×8 = sweet spot; resembles MPC pad grid (muscle-memory benefit).
//
// Heatmap:
//   heatmap[i] = normalized visit recency, [0.0, 1.0].
//   0.0 = never visited (or cold).  1.0 = just visited this cell.
//   Decays at ~0.01/sec (fully cold after ~100 seconds without a visit).
//   Heatmap is intentionally NOT persisted — it is ephemeral session state
//   and resets to zero on every load.  (Spec section 6.)
//
struct XOuijaCellGrid
{
    static constexpr int kCols = 8;
    static constexpr int kRows = 8;
    static constexpr int kSize = kCols * kRows;   // 64

    std::array<XOuijaCell, kSize> cells {};

    //--------------------------------------------------------------------------
    // Heatmap — audio thread writes (XOuijaWalkEngine), UI thread reads via
    // atomic snapshot.  Declared here for layout clarity; the walk engine stores
    // the live copy as atomics separately to avoid data races.
    // This non-atomic array is only used during ValueTree save (message thread).
    std::array<float, kSize> heatmap {};

    XOuijaCell&       at(int col, int row)       noexcept
    {
        return cells[static_cast<size_t>(row * kCols + col)];
    }
    const XOuijaCell& at(int col, int row) const noexcept
    {
        return cells[static_cast<size_t>(row * kCols + col)];
    }

    XOuijaCell&       at(int index)       noexcept
    {
        return cells[static_cast<size_t>(juce::jlimit(0, kSize - 1, index))];
    }
    const XOuijaCell& at(int index) const noexcept
    {
        return cells[static_cast<size_t>(juce::jlimit(0, kSize - 1, index))];
    }

    //--------------------------------------------------------------------------
    // ValueTree persistence.
    // Heatmap is explicitly NOT persisted — it resets on load (spec section 6).
    //
    [[nodiscard]] juce::ValueTree toValueTree() const
    {
        juce::ValueTree root { "XOuijaGrid" };
        for (int i = 0; i < kSize; ++i)
            root.appendChild(cells[static_cast<size_t>(i)].toValueTree(i), nullptr);
        return root;
    }

    // Returns true if at least one cell was successfully parsed.
    // Unknown cell indices are ignored (forward-compat: larger grids in future saves).
    // Missing cells keep their default construction value.
    bool fromValueTree(const juce::ValueTree& root)
    {
        if (!root.isValid()) return false;
        bool anyOk = false;
        for (int i = 0; i < root.getNumChildren(); ++i)
        {
            const auto child = root.getChild(i);
            const int idx = static_cast<int>(child.getProperty("index", -1));
            if (idx >= 0 && idx < kSize)
            {
                cells[static_cast<size_t>(idx)].fromValueTree(child);
                anyOk = true;
            }
        }
        return anyOk;
    }
};

} // namespace xoceanus
