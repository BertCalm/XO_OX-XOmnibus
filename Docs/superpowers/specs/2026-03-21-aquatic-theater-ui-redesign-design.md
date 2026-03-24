# XOlokun UI Redesign — Aquatic Theater

**Date:** 2026-03-21
**Status:** Design
**Approach:** Code-First Incremental (6 phases, each buildable)

---

## Vision

Transform XOlokun from a developer prototype into a premium aquatic-themed instrument that feels unique in the market. Deep ocean gradients, floating engine orbs, glowing coupling arcs, and progressive disclosure that serves both the creator's workflow and a broad audience.

No other synth looks like this. The aquatic mythology IS the UI identity.

## Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Theme | Dark aquatic (deep ocean gradient) | Matches mythology, feels premium, unique identity |
| Layout | Compact Hybrid — engine strip + PERFORM mode | Quick editing AND immersive performance |
| Parameters | Progressive disclosure + smart groups | Simple for beginners, organized depth for power users |
| Empty state | Guided first-run → restore session | No confusion on first launch, continuity after |
| Fonts | Space Grotesk / Inter / JetBrains Mono (embedded) | Already implemented and building |
| Target user | Creator first, progressive disclosure for broad audience | D+C from brainstorm |

---

## Phase 1: Fix Broken Things

**Goal:** Stop shipping known bugs. No visual changes yet — just correctness.

### 1.1 Stale header tagline
- **Current:** "21 Engines · 12 Coupling Types · 1600+ Presets"
- **Fix:** Dynamic counts from EngineRegistry and preset scanner
- **Format:** "{N} Engines · 14 Coupling Types · {P}+ Presets"
- Engine count: `EngineRegistry::instance().getRegisteredIds().size()` (currently 46)
- Coupling types: hardcode 14 (from `CouplingType` enum in `SynthEngine.h`)
- Preset count: `presetManager.getLibrary().size()`. If 0 (not yet scanned), show "18000+" as static fallback
- **File:** `XOlokunEditor.h` ~line 2760

### 1.2 Missing mood filters
- **Current:** PresetBrowserPanel has 7 tabs (ALL + 6 moods), missing Family and Submerged
- **Fix:** Add "Family" and "Submerged" tabs. Update ALL parallel arrays:
  - `kNumMoods` → 9
  - `moodLabels[]` array (button text)
  - `moodNames[]` in `updateFilter()` (~line 1575)
  - `moodColors[]` in `paintListBoxItem` (~line 1487) — assign colors for Family and Submerged
  - `moodIds[]` in `paintListBoxItem` (~line 1496) — add matching IDs
- **File:** `XOlokunEditor.h` PresetBrowserPanel section

### 1.3 Right-click context menu
- **Current:** Tooltip says "right-click to swap" but mouseUp only handles left button
- **Fix:** Implement right-click PopupMenu on CompactEngineTile with: Swap Engine, Remove Engine, Move to Slot N
- **File:** `XOlokunEditor.h` CompactEngineTile::mouseUp

### 1.4 Engine removal
- **Current:** No way to remove an engine from a slot
- **Fix:** Add "Remove Engine" to the right-click menu + showLoadMenu. Calls `processor.loadEngine(slot, "")` or equivalent clear.
- **File:** `XOlokunEditor.h` CompactEngineTile::showLoadMenu

### 1.5 mouseUp button check bug
- **Current:** `if (!e.mods.isLeftButtonDown())` is vacuous on mouseUp (button already released)
- **Fix:** Remove the dead check, keep only `mouseWasDraggedSinceMouseDown()` guard
- **File:** `XOlokunEditor.h` CompactEngineTile::mouseUp ~line 830

---

## Phase 2: Dark Aquatic Theme

**Goal:** Transform the visual identity from bland light mode to the Aquatic Theater.

### 2.1 Color palette overhaul
Replace GalleryColors constants:

| Role | Current (Light) | New (Aquatic Dark) |
|------|-----------------|-------------------|
| Background | `#F8F6F3` (shell white) | `#0A1628` (deep ocean) |
| Panel/Card BG | `#FCFBF9` (slot bg) | `#0D2137` → `#132B4A` (gradient) |
| Text primary | `#1A1A1A` | `#EEEEEE` |
| Text secondary | `#777570` | `#6688AA` |
| Borders | `#DDDAD5` | `rgba(255,255,255,0.06)` |
| Empty slot | `#EAE8E4` | `#1A2A3D` with dashed border |
| XO Gold | `#E9C46A` (unchanged) | `#E9C46A` (unchanged — brand constant) |
| Gold text | `#9E7C2E` | `#E9C46A` (reads well on dark) |

### 2.2 Background gradient
- Main editor background: `linear-gradient(180deg, #0A1628, #0D2137)`
- Subtle radial glow behind loaded engine slots using their accent color at ~5% opacity
- Optional (Phase 6): static pre-rendered caustic texture as `juce::Image`, NOT animated repaint loop. Paint once at startup or on resize, display as background. Avoids 30-60Hz repaint cost.

### 2.3 Typography sizing
Increase key font sizes for readability on dark background:
- Engine names in tiles: 11pt → 13pt Space Grotesk Bold
- Parameter labels: 8pt → 9pt Inter Medium
- Parameter values: currently unlabeled → 8pt JetBrains Mono
- Section headers: 9pt → 11pt Inter Bold
- Preset name in browser strip: 9pt → 11pt Inter Medium

### 2.4 Fix missing accent colors
- `accentForEngine()` is missing entries for: OXBOW (`#1A6B5A`), OWARE (`#B5883E`), OPERA (`#D4AF37`), OFFERING (`#E5B80B`)
- Add all 4 entries. Cross-reference full color table in CLAUDE.md.
- Audit: ensure all 46 engines have an entry. The function currently falls back to grey for unknown IDs.

### 2.5 Dark mode as default
- Flip `GalleryColors::darkMode()` default to `true`
- The "Aquatic" palette IS the dark mode. Light mode becomes the toggle (legacy/accessibility).
- Update Dark namespace constants to the new aquatic palette

---

## Phase 3: Engine Strip Redesign

**Goal:** Replace the cramped sidebar with the Compact Hybrid engine strip.

### 3.1 Layout restructure
```
┌──── HEADER (44px) ──────────────────────────────────────────────┐
│  XO_OX logo (gold)              [< Preset Name >]  [PERFORM]   │
├─────────────────────────────────────────────────────────────────┤
│  ENGINE STRIP (60px) — horizontal cards                         │
│  [●ON ONSET Perc·8v] [●OP OPAL Gran·4v] [+ Add] [+ Add]       │
├─────────────────────────────────────────────────────────────────┤
│  COUPLING BAR (28px, optional — only if routes active)          │
│  COUPLING ─── ON→OP AmpToFilter 0.45 ─── ON→OP FreqMod 0.2    │
├─────────────────────────────────────────────────────────────────┤
│  PARAMETER PANEL (flexible height, scrollable)                  │
│  [ONSET — Simple view: 4 macro knobs + key params]              │
│  or [ONSET — Advanced: smart-grouped sections]                  │
├─────────────────────────────────────────────────────────────────┤
│  MACRO BAR (50px) — 4 gold knobs + master volume                │
├─────────────────────────────────────────────────────────────────┤
│  MASTER FX BAR (50px) — SAT|DELAY|REVERB|MOD|COMP|SEQ          │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 Engine card design
Each loaded engine card (in the strip):
- Engine accent color orb (28px, radial gradient with glow)
- 2-letter abbreviation inside orb (e.g., "ON" for Onset)
- Engine name in accent color (13pt Space Grotesk Bold)
- Subtitle: engine type + voice count (7pt Inter, muted color)
- Background: engine accent at 8% opacity with 1px accent border at 20%
- Selected state: accent border brightens to 50%, underline accent bar below

Empty slot card:
- Dashed border (`rgba(255,255,255,0.1)`)
- "+" icon centered (16pt, `rgba(255,255,255,0.2)`)
- Click opens engine browser overlay (not a PopupMenu — a proper panel)

### 3.3 Engine browser overlay
When clicking "+" or first-run:
- Full-width overlay panel (replaces parameter panel area temporarily)
- Search field at top
- Grid of engine cards: accent orb + name + one-line description + category tag
- Categories: Percussion, Synthesis, Physical, Spectral, Generative, Modulation, World
- Click to load into slot, overlay closes, engine appears in strip

**Data source for descriptions and categories:**
Engine metadata is stored as a UI-side lookup table (NOT on `SynthEngine` interface — avoids breaking all 46 adapters). Add `EngineMetadata.h` to `Source/UI/`:
```cpp
struct EngineInfo { const char* id; const char* shortDesc; const char* category; const char* abbrev; };
static constexpr EngineInfo kEngineInfo[] = {
    {"Onset",     "Percussion synthesis, 8 voices",  "Percussion", "ON"},
    {"Opal",      "Granular synthesis",              "Spectral",   "OP"},
    // ... all 46 engines
};
```
This table also provides the 2-letter abbreviation (resolves S4 collision issue — hand-authored, not derived).

---

## Phase 4: Parameter Panel Redesign

**Goal:** Progressive disclosure with smart-grouped advanced view.

### 4.1 Simple view (default)
- Shows only the 4 macros + up to 4 "key parameters" per engine
- Key parameters stored in `EngineMetadata.h` (same UI-side table as descriptions/categories):
  ```cpp
  struct EngineInfo { ...; std::initializer_list<const char*> keyParams; };
  // e.g., {"Onset", ..., {"perc_drive", "perc_masterTone", "perc_circuitBlend", "perc_noiseLevel"}}
  ```
- If no key params defined for an engine, show macros only (graceful fallback)
- Toggle: "Show All ▾" at bottom-right of panel

### 4.2 Advanced view (smart-grouped)
When "Show All" is toggled:
- Parameters grouped by prefix family using a generic sub-prefix detector:
  1. Strip the engine prefix (e.g., `perc_` from `perc_fx_reverb_mix`)
  2. If the remainder contains `_`, the first segment is the group name (e.g., `fx` → "Effects")
  3. If the remainder has no `_`, it's a top-level param → goes in "Core"
  4. Group name mapping (UI-side table, not engine-specific):
     - `macro` → "Macros", `fx` → "Effects", `xvc` → "Cross-Voice", `char` → "Character"
     - `lfo` → "LFO", `env` → "Envelope", `mod` → "Modulation", `osc` → "Oscillator"
     - Unknown sub-prefixes: use the raw sub-prefix, Title-cased (e.g., `hunt` → "Hunt")
  5. This works across ALL 46 engines because it's derived from parameter ID structure, not hardcoded per engine
- For engines with flat naming (e.g., `opal_grainSize` has no sub-prefix → all params go to "Core")
  that's acceptable — a single "Core" group is no worse than the current flat grid, and these engines
  can adopt sub-prefixes later without UI changes
- Each group has a section header (11pt Inter Bold, engine accent color)
- Knobs within each group arranged in a flow grid
- Entire panel scrollable

### 4.3 Knob design
- Replace generic JUCE rotary with custom painted knob:
  - Outer ring: engine accent color (arc shows value)
  - Inner circle: dark background (`#0D1B2A`)
  - Value text: JetBrains Mono, centered in knob
  - Label below: Inter Medium, muted color
  - Size: 40px diameter (simple view), 34px (advanced view)

---

## Phase 5: Wire PlaySurface + PERFORM Mode

**Goal:** The fully-built PlaySurface finally becomes accessible.

### 5.1 PERFORM button
- In header, right side
- Toggles between Edit mode (compact hybrid) and Perform mode
- Keyboard shortcut: `P` only (Tab conflicts with JUCE focus traversal and DAW hosts)

### 5.2 Perform mode layout
```
┌──── HEADER (44px) ──────────────────────────────────────────────┐
│  XO_OX logo              [< Preset Name >]  [PERFORM ●]        │
├────────── SPATIAL FIELD (flexible, ~45% height) ────────────────┤
│  Floating engine orbs with coupling arcs                        │
│  Orb size = voice count (min 20px floor for 0-voice engines).   │
│  Draggable orbs (position is cosmetic, not functional)          │
├────────── PLAY SURFACE (flexible, ~45% height) ─────────────────┤
│  [NoteInputZone 4x4] [OrbitPath XY] [PerfStrip] [PerfPads]     │
├──── MACRO BAR + FX BAR (100px combined) ────────────────────────┤
│  Same as edit mode                                              │
└─────────────────────────────────────────────────────────────────┘
```

### 5.3 PlaySurface wiring
- Instantiate `PlaySurface` in `XOlokunEditor`
- **MIDI injection (thread-safe):** Use `juce::MidiMessageCollector` owned by the processor.
  - Editor calls `collector.addMessageToQueue(msg)` from the UI thread (lock-free)
  - Processor calls `collector.removeNextBlockOfMessages(midiBuffer, numSamples)` in `processBlock`
  - This is the standard JUCE pattern for UI→audio MIDI in AU/AUv3 contexts
- Connect `NoteInputZone` `onNoteOn`/`onNoteOff` → `collector.addMessageToQueue()`
- Connect `OrbitPathZone` → send CC messages via collector (CC74 for cutoff, CC71 for resonance)
- Connect `PerformanceStrip` → send CC messages per mode
- Connect `PerformancePads` (FIRE/XOSEND/ECHO CUT/PANIC) → processor action flags (atomic bools)

---

## Phase 6: Polish Pass

**Goal:** Menus, tooltips, animations, and the details that make it feel premium.

### 6.1 Tooltips
- Every knob: "{Engine} {Parameter Name}: {value}" on hover
- Delay: 400ms before showing
- Style: dark tooltip with accent-colored parameter name
- Include range: "0.0 – 1.0" or "20 Hz – 20 kHz" where applicable

### 6.2 Menus
- Replace all `juce::PopupMenu` with custom-styled menus matching aquatic theme
- Dark background, accent-colored items, rounded corners
- Engine browser: proper overlay with search, not a popup

### 6.3 Animations
- Engine load: orb fades in with a pulse glow (300ms)
- Panel transitions: 150ms opacity crossfade (already exists, keep)
- Coupling arc: subtle pulse animation along the arc (CSS-style dash offset)
- Knob turns: smooth interpolation (already APVTS-driven)
- Mode switch (Edit↔Perform): slide transition (200ms)

### 6.4 Preset browser upgrade
- Add missing mood tabs (Family, Submerged)
- Add engine filter dropdown (currently shows all presets regardless)
- Use the standalone PresetBrowser.h's "Find Similar" DNA search feature
- Dark-themed callout box matching aquatic palette

### 6.5 Settings / About
- Accessible via gear icon in header
- Theme toggle (Aquatic Dark / Legacy Light)
- Audio/MIDI settings shortcut
- Version info + engine count
- Link to XO-OX.org

### 6.6 Window size
- Default: 1000×700 (up from 880×562)
- Min: 800×550
- Max: 1600×1000
- Resizable (already supported)

---

## Out of Scope (V2)

- Figma design system (can add later for consistency)
- Per-engine custom UI panels (e.g., OBRIX ecology visualizer, ONSET circuit selector)
- Mobile-specific layout changes (iOS AUv3)
- VST3 format
- Rive animations (Oscar character)

---

## Implementation Order

| Phase | Effort | Dependencies |
|-------|--------|-------------|
| 1. Fix broken things | Low (1-2 sessions) | None |
| 2. Dark aquatic theme | Medium (1-2 sessions) | Phase 1 |
| 3. Engine strip redesign | High (2-3 sessions) | Phase 2 |
| 4. Parameter panel | Medium (1-2 sessions) | Phase 3 |
| 5. PlaySurface + PERFORM | High (2-3 sessions) | Phase 3 |
| 6. Polish pass | Medium (2-3 sessions) | Phase 4+5 |

**Total estimate: 9-15 sessions** (phases 4+5 can run in parallel after phase 3)

---

## Success Criteria

1. A new user can load an engine and hear sound within 30 seconds of opening XOlokun
2. The UI feels visually distinct — no one mistakes it for another synth
3. Parameters are discoverable without reading documentation
4. The PlaySurface is accessible and connected to the audio engine
5. All 9 bugs identified in the audit are fixed
6. Fonts render as Space Grotesk / Inter / JetBrains Mono (already done)
