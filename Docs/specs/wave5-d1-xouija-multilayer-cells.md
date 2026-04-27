<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 XO_OX Designs -->

# Wave 5 D1 — XOuija Multi-Layer Cells

**Status:** SPEC — not yet implemented
**Decision source:** D8 A4 (FULLY LOCKED 2026-04-25, `xoceanus-ui-deployment-2026-04-25.md`)
**Tracks:** Wave 5 DSP/Wiring + Wave 6 Layout Overhauls
**Depends on:** #1172 (SubmarineOuijaPanel CC wiring), D6 sequencer, D7 chord machine, D9 mod matrix

---

## 1. Why this spec exists

The current XOuija (`Source/UI/PlaySurface/XOuijaPanel.h`, 2,407 lines) is a 2D position surface. The planchette outputs two scalars — `circleX` (0-1, horizontal fifths position) and `influenceY` (0-1, depth). The 13 circle-of-fifths markers are single-value (semitone root note). There are no cells. There is no grid. There is no chord, rhythm, or texture concept.

D8 A4 mandates a fundamentally different architecture: **a grid of cells, each carrying three layers** (chord, rhythm-density, texture-vector), with a tempo-synced planchette that autonomously walks the grid. This is not a field addition — it is a replacement of the XOuija data model and movement engine.

This spec locks the design before implementation begins. Implementation is estimated 3-5 dev days.

---

## 2. Locked decisions (from D8 — do not re-litigate)

| ID | Decision |
|----|----------|
| D8-A4 | Grid cells, each holds chord + rhythm-density + texture-vector. Planchette emits all three layers. |
| D8-B4 | Movement: tempo-synced + tendency + mood sliders (calm/wild, consonant/dissonant). |
| D8-C3 | Curation: pin cells, capture moments (snapshot to slot), heatmap (brightness = recency). NO rewind. |
| D8-D5 | Output routing: per-engine selectable (notes / sequencer params / chord machine / mod source). |
| D8-E3 | UI: self-contained on OUIJA tab. Grid + planchette + mood sliders + capture + heatmap + pinned cells on one surface. Edit mode toggle (no separate breakout). |

---

## 3. Data model

### 3.1 XOuijaCellLayer (the three layers per cell)

```cpp
// Source/DSP/XOuijaCell.h  (new file)

namespace xoceanus {

//==============================================================================
// ChordLayer — what harmonic material the cell emits.
//
// ChordType matches D7's voicing palette: Tertian + Quartal/Quintal + Modal-world.
// Root is a semitone offset from the global key root (0 = root, 7 = fifth, etc.)
// 12-TET only; microtonal deferred to v2 per D7.
//
enum class ChordType : uint8_t
{
    // Tertian (Western)
    Maj = 0, Min, Dom7, Maj7, Min7, Dim, Aug, Sus2, Sus4,
    Add9, Min9, Maj9, Dom9, HalfDim,
    // Quartal / Quintal
    Quartal3, Quartal4, Quintal3,
    // Modal-world (root + characteristic intervals)
    Hijaz,      // Phrygian dominant (b2, M3, P5)
    Bhairavi,   // Minor with b2 (b2, m3, P5)
    YoScale,    // Pentatonic major (Japanese)
    InScale,    // Pentatonic minor (Japanese)
    // Special
    Unison,     // Single note (no chord, just root)
    Open5,      // Power chord (root + 5th, no 3rd)
    Count
};

struct ChordLayer
{
    ChordType type    = ChordType::Maj;
    int8_t    root    = 0;   // semitone offset from global key root, [-6, 6]
    uint8_t   octave  = 4;   // MIDI octave for voicing, [2, 7]
};

//==============================================================================
// RhythmDensity — how many events per bar this cell emits.
//
// 0.0 = rest (no events).  0.5 = moderate (quarter notes).  1.0 = dense (16th notes).
// The sequencer interprets this as a probability gate density or a fixed subdivision.
//
struct RhythmDensity
{
    float value = 0.5f;   // [0.0, 1.0]
};

//==============================================================================
// TextureVector — 2D timbral signature emitted as mod sources.
//
// brightness: 0.0 = dark/filtered / 1.0 = bright/open.
//             Mapped to filter cutoff shift (or engine-specific param per D5 routing).
// motion:     0.0 = static/sustained / 1.0 = animated/tremolo.
//             Mapped to LFO depth or mod rate (engine-specific per D5 routing).
//
struct TextureVector
{
    float brightness = 0.5f;   // [0.0, 1.0]
    float motion     = 0.5f;   // [0.0, 1.0]
};

//==============================================================================
// XOuijaCell — one grid cell.
//
// Default values produce "neutral" output: C major, moderate density, mid brightness/motion.
//
struct XOuijaCell
{
    ChordLayer     chord   {};
    RhythmDensity  rhythm  {};
    TextureVector  texture {};

    bool pinned = false;   // pinned cells block planchette movement through them (D8-C3)

    // Persistence
    [[nodiscard]] juce::ValueTree toValueTree() const;
    bool fromValueTree(const juce::ValueTree& t);
};

} // namespace xoceanus
```

### 3.2 XOuijaCellGrid

```cpp
// Grid dimensions.  8×8 = 64 cells.
// Not defined by APVTS (too many params) — stored as a packed ValueTree child
// in the processor state tree under "XOuijaGrid".
//
// Rationale for 8×8 (not 4×4 or 16×16):
//   - 4×4 (16 cells) = too few to enable interesting probabilistic walks
//   - 16×16 (256 cells) = overwhelming to edit, heatmap illegible at UI scale
//   - 8×8 (64 cells) = sweet spot; resembles MPC pad grid (muscle memory benefit)
//
struct XOuijaCellGrid
{
    static constexpr int kCols = 8;
    static constexpr int kRows = 8;
    static constexpr int kSize = kCols * kRows;  // 64

    std::array<XOuijaCell, kSize> cells {};

    XOuijaCell&       at(int col, int row)       { return cells[row * kCols + col]; }
    const XOuijaCell& at(int col, int row) const { return cells[row * kCols + col]; }

    // heatmap[i] = normalized visit recency for cell i, [0.0, 1.0].
    // Updated by XOuijaWalkEngine after each planchette step.
    // 0.0 = never visited or cold.  1.0 = just visited.
    // Decays at ~0.01/sec (fully cold after ~100 seconds without visit).
    std::array<float, kSize> heatmap {};

    [[nodiscard]] juce::ValueTree toValueTree() const;
    bool fromValueTree(const juce::ValueTree& t);
};
```

---

## 4. Walk engine

```cpp
// Source/DSP/XOuijaWalkEngine.h  (new file)
//
// XOuijaWalkEngine — tempo-synced autonomous planchette.
//
// The walk engine is the core of D8-B4.  It runs on the audio thread
// (called from processBlock).  All state mutations happen on the audio thread;
// UI reads via atomic snapshots.
//
// Movement model:
//   Each beat (tempo-synced), the engine selects a candidate cell using:
//     1. Tendency vector (normalized direction bias in col/row space, [-1, 1] each axis)
//     2. Mood scalars:
//        - calm_wild [0, 1]:  0 = step stays adjacent (distance 1), 1 = can leap anywhere
//        - consonant_dissonant [0, 1]: 0 = prefer cells whose ChordType tension is low
//          (Maj/Min/Sus), 1 = prefer chromatic/dissonant chords (Dim/HalfDim/Hijaz)
//     3. Pinned cell avoidance: pinned cells are excluded from the candidate set
//     4. Heatmap cooling: recently visited cells have lower selection weight
//        (prevents the planchette from oscillating between two cells)
//
// RT-safe parameter updates use atomics for scalars and a lock-free SPSC queue
// for cell edits (cell index + new data).
//
class XOuijaWalkEngine
{
public:
    void prepareToPlay(double sampleRate, double bpm);
    void processBlock(int numSamples, double bpm, double ppqPosition, bool isPlaying);

    // Called by UI thread (via SPSC queue) when user edits a cell.
    void enqueueEdit(int cellIndex, XOuijaCell cell);

    // Mood sliders (RT-safe atomics)
    void setCalmWild(float v);          // [0, 1]
    void setConsonantDissonant(float v); // [0, 1]

    // Tendency vector (RT-safe atomics)
    void setTendencyCol(float v);  // [-1, 1]  negative = drift left, positive = drift right
    void setTendencyRow(float v);  // [-1, 1]  negative = drift down, positive = drift up

    // Snapshot for UI thread (lock-free read).
    // Returns current cell index + heatmap state.
    struct Snapshot
    {
        int   cellIndex    = 0;
        float heatmap[XOuijaCellGrid::kSize] = {};
    };
    [[nodiscard]] Snapshot getSnapshot() const;  // safe to call from UI thread

    // Output (polled by output router after each processBlock).
    // Reflects the currently active cell.
    [[nodiscard]] const XOuijaCell& currentCell() const noexcept;

private:
    XOuijaCellGrid grid_;
    int   currentIndex_  = 0;
    float phaseSamples_  = 0.0f;
    float stepSizeSamples_ = 0.0f;  // recomputed from bpm each block

    std::atomic<float> calmWild_       { 0.3f };
    std::atomic<float> consonantDissonant_ { 0.2f };
    std::atomic<float> tendencyCol_   { 0.0f };
    std::atomic<float> tendencyRow_   { 0.0f };

    // SPSC queue for cell edits from UI thread
    // capacity: 64 (one per cell, safe for any burst)
    struct CellEdit { int index; XOuijaCell cell; };
    juce::AbstractFifo editFifo_ { 64 };
    std::array<CellEdit, 64> editBuf_ {};

    // Heatmap (audio thread owns, UI thread reads via atomic snapshot)
    std::array<std::atomic<float>, XOuijaCellGrid::kSize> heatmapAtomics_ {};

    void applyPendingEdits();
    int  selectNextCell();
    void advanceHeatmap();
};
```

---

## 5. Output routing (D8-D5)

Per-engine selectable. Stored as 4 enums (one per engine slot) in the processor state.

```cpp
enum class XOuijaOutputMode : uint8_t
{
    Off          = 0,  // XOuija does not affect this engine slot
    DriveNotes   = 1,  // chord layer → MIDI notes through chord machine (D7)
    DriveSeq     = 2,  // rhythm-density layer → sequencer gate probability (D6)
    DriveChord   = 3,  // chord type + root → chord machine root/voicing override (D7)
    ModSource    = 4,  // texture-vector (brightness, motion) → 2 mod matrix sources (D9)
};

// Stored in processor APVTS:
//   "ouija_route_0" .. "ouija_route_3"  (one per engine slot, int param 0-4)
```

The output router lives in `XOceanusProcessor::processBlock` and dispatches each layer to its target system each time `XOuijaWalkEngine` advances to a new cell.

---

## 6. Persistence

XOuija state uses ValueTree (NOT APVTS parameters) because:
- 64 cells × 3 layers = 192+ values is too many APVTS params (downstream cost to DAW)
- Cell content is editorial state (composer's grid), not real-time automatable
- Mood sliders and tendency ARE APVTS params (user should be able to automate them)

ValueTree node layout inside `apvts.state`:

```
XOuijaPanel (existing node — already persisted in XOceanusProcessor)
  XOuijaGrid
    Cell index="0"  chord_type="0" chord_root="0" chord_octave="4"
                    rhythm="0.5"
                    texture_b="0.5" texture_m="0.5"
                    pinned="0"
    Cell index="1"  ...
    ...
    Cell index="63" ...
  XOuijaHeatmap
    (omit from persistence — heatmap is ephemeral session state, reset on load)
```

APVTS params (automatable mood + tendency):
- `ouija_calm_wild`           float [0, 1]  default 0.3
- `ouija_consonant_dissonant` float [0, 1]  default 0.2
- `ouija_tendency_col`        float [-1, 1] default 0.0
- `ouija_tendency_row`        float [-1, 1] default 0.0
- `ouija_route_0` .. `ouija_route_3`  int [0, 4] default 0 (Off)

Total new APVTS params: 8. New ValueTree children: 1 (`XOuijaGrid`).

---

## 7. UI changes

The existing `XOuijaPanel.h` and `SubmarineOuijaPanel.h` are both 2D-position panels. Under this design they are replaced by a new grid surface. Key UI deltas:

**New components (header-only, `Source/UI/Ocean/` or `Source/UI/PlaySurface/`):**

1. `XOuijaCellGridView.h` — 8×8 grid renderer.
   - Each cell: 44px min tap target (WCAG), colored by chord type (tension-to-color from `HarmonicField::tensionColor`).
   - Heatmap overlay: cell brightness proportional to `heatmap[i]`.
   - Pinned cells: rendered with a pin icon overlay.
   - Planchette: a 16×16 circle that animates between cell centres (tempo-synced, eased).
   - Edit mode toggle: shows per-cell chord/density/texture popover in-place (no breakout panel).

2. `XOuijaMoodStrips.h` — two horizontal sliders: CALM ↔ WILD, CONSONANT ↔ DISSONANT.
   - Styled as atmospheric temperature gauges (matches submarine.html aesthetic).

3. `XOuijaCaptureBar.h` — row of 4 capture slots. "Snapshot current cell state to slot N" buttons.
   - Each slot shows a miniature cell preview (chord name + rhythm dot density + texture bar).

**Existing components to retire (after this wave):**
- `SubmarineOuijaPanel.h` — replace with `XOuijaCellGridView` in OceanView.
- `XOuijaPanel.h` (in PlaySurface) — the new grid view IS the OUIJA tab; the circle-of-fifths planchette becomes the chord machine's internal rendering, not XOuija's surface.

**Backward compatibility:** `circleX_` and `influenceY_` outputs are preserved as computed values (current planchette column/row normalized to [0,1]) so any existing CC mappings (CC 85, CC 86) keep working.

---

## 8. Migration path

The existing `XOuijaPanel.h` has 2,407 lines of production code (gesture banks, trail modulator, MIDI learn, planchette animation, etc.). Some of it transfers:

| Existing component | Fate |
|--------------------|------|
| `Planchette` class | Reuse — adapts to grid-cell centre positions instead of continuous (x,y) |
| `GestureButtonBar` | Reuse — FREEZE/HOME/DRIFT still meaningful (freeze walk / spring to center / resume) |
| `GoodbyeButton`    | Reuse — emits chord-off + resets walk to center cell |
| `TrailModulator`   | Keep — trail velocity remains a valid mod source even in grid mode |
| `GestureButtonMidiLearnManager` | Keep — maps CCs to gesture buttons |
| Circle-of-fifths marker paint | Retire — replaced by grid cell renderer |
| `circleX_` / `influenceY_` | Convert to derived (planchette col/row → normalized [0,1]) |

---

## 9. Implementation sequence

Phase A — Data + engine (no UI, audio thread only):
1. Write `Source/DSP/XOuijaCell.h` (structs, ValueTree serialization)
2. Write `Source/DSP/XOuijaWalkEngine.h` (walk + heatmap, no MIDI output yet)
3. Add `XOuijaWalkEngine` member to `XOceanusProcessor`, wire `prepareToPlay` + `processBlock`
4. Add 8 new APVTS params (`ouija_calm_wild`, etc.)
5. Wire ValueTree save/restore in `XOceanusProcessor::getStateInformation` / `setStateInformation`

Phase B — Output router (audio thread):
6. Write `XOuijaOutputRouter` (dispatches cell layers to D6/D7/D9 systems by slot)
7. Wire router to `XOceanusProcessor::processBlock`

Phase C — UI (message thread):
8. Write `XOuijaCellGridView.h` (grid renderer + edit mode)
9. Write `XOuijaMoodStrips.h`
10. Write `XOuijaCaptureBar.h`
11. Replace `SubmarineOuijaPanel` in `OceanView.h`
12. Replace circle-of-fifths section in `XOuijaPanel.h` (PlaySurface OUIJA tab)

Phase D — Verification:
13. Manual smoke test: planchette walks, chord output reaches engine, heatmap brightens
14. Save/restore round-trip (grid contents survive close/open)
15. CC 85/86 backward compat (check in DAW with automation)

---

## 10. Open questions (require user decision before Phase C)

1. **Grid size:** 8×8 locked here. User may prefer 4×8 (portrait) or 6×6. Confirm before implementing `XOuijaCellGridView`.

2. **Walk tempo:** D8-B4 says "tempo-synced." What subdivision? Options: whole note, half note, quarter note, or user-selectable. Recommendation: default = 1 bar (whole note), selectable 1/2 / 1 / 2 / 4 bars.

3. **Edit mode UI:** D8-E3 says in-place popover. Define popover contents for a cell (chord type picker + root offset + rhythm density slider + texture XY pad + pin toggle). This is ~8 controls per cell. Confirm layout approach before implementation.

4. **Capture slots:** D8-C3 says "snapshot current planchette state to a slot." Does "state" mean (a) just the current cell's layers, or (b) the full grid contents + planchette position? (a) is simpler and more composable; (b) is more like scene recall.

---

## 11. Reference files

- Decision source: `~/.claude/projects/-Users-joshuacramblet/memory/xoceanus-ui-deployment-2026-04-25.md` D8 section
- Existing XOuija: `Source/UI/PlaySurface/XOuijaPanel.h` (2,407 lines)
- Submarine variant: `Source/UI/Ocean/SubmarineOuijaPanel.h` (692 lines)
- Related CC wiring issue: #1172 (SubmarineOuijaPanel currently decorative)
- Chord machine spec: locked in D7 (same memory file)
- Mod matrix spec: locked in D9 (same memory file)
- Sequencer spec: `~/.claude/projects/-Users-joshuacramblet/memory/wave5-c1-sequencer-design-2026-04-26.md`
