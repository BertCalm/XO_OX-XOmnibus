# XOceanus UI Wiring Manifest — Phase 1

> **Status:** V2 — Post Quantum Audit, Ready for Build Sprint
> **Date:** 2026-03-25
> **Purpose:** Single source of truth mapping every UI element in the V1 spatial architecture to its C++ APVTS parameter, callback, data source, or implementation need. A JUCE developer reads this document and knows exactly what to build, what to rewire, and what already exists.
> **Source files used:** `Source/UI/XOceanusEditor.h` (4,323 lines), `Docs/design/xoceanus-spatial-architecture-v1.md`, `Docs/design/xoceanus-definitive-ui-spec.md`

---

## Legend

| Symbol | Meaning |
|--------|---------|
| EXISTS | Code exists and is wired correctly |
| REWIRE | Code exists but needs restructuring, relocation, or dimension change |
| BUILD | No code exists; needs to be built from scratch |
| ISSUE | Code exists but has a known problem requiring fix |

**Thread safety notation:**
- `[UI]` — UI thread only (SliderAttachment, direct APVTS reads)
- `[AUDIO]` — Audio thread only (getRawParameterValue, processBlock)
- `[FIFO]` — Lock-free FIFO crossing audio→UI boundary
- `[ATOMIC]` — std::atomic, safe to read from UI

---

## 1. HEADER (64pt)

### Current Implementation
The editor currently has a 50pt header (`kHeaderH = 50`) painted directly in `XOceanusEditor::paint()`. It contains: "XOceanus" text (Space Grotesk 19pt), "XO_OX Designs" subtitle, active coupling route count label, MIDI Learn status badge. Right side: `PresetBrowserStrip` (220px wide), Export button, Dark mode toggle, PS toggle, Performance toggle, CM toggle.

**Target (V1):** 64pt header with Logo, Depth-Zone Dial engine selector, Preset Navigator (prev/name/next/heart/DNA), A/B comparison toggle, M1-M4 macro knobs, utility cluster.

**Audit fix (ULF-P0 + XAVIER-P0):** Header increased from 52pt to 64pt to accommodate 44pt touch targets (Apple HIG) and knob+label vertical clearance.

---

### 1.1 XO_OX Logo

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| XO_OX Logo (32x32pt SVG) | None — static asset | Text "XOceanus" painted in `XOceanusEditor::paint()`. No SVG logo component. | REWIRE | [UI] | Replace text with `juce::Drawable::createFromSVG()`. Asset path: `Resources/logo.svg`. Bridge color always `#E9C46A`. Symbol color `#1A1A1A` light / `#F0EDE8` dark. Click opens About panel (not implemented). |
| About panel (on logo click) | None — informational | Not implemented | BUILD | [UI] | Simple `juce::CallOutBox` with version, credits, patreon link. No APVTS involvement. |

---

### 1.2 Depth-Zone Dial (Engine Selector)

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Depth-Zone Dial (48x48pt circular dial) | `EngineRegistry::instance()` — reads registered engine list; calls `processor.loadEngine(slot, engineId)` on change | Not implemented. Engine selection currently done via `CompactEngineTile` right-click context menu only (`showLoadMenu()` in `CompactEngineTile::mouseDown`). | BUILD | [UI] | Custom `juce::Component`. Radial gradient: top arc `#48CAE4` (Sunlit), middle `#0096C7` (Twilight), bottom `#7B2FBE` (Midnight). Center: creature icon (engine-specific SVG, outline-only, 1.5px stroke). Drag CW/CCW scrolls through engines in water-column order. 50ms hot-swap crossfade on engine change (existing in processor). |
| Engine name label (below dial) | `processor.getEngine(selectedSlot)->getEngineId()` | `CompactEngineTile` shows engine name; no standalone engine name label in header | BUILD | [UI] | Space Grotesk Regular 11pt, engine accent color. Updates when Depth-Zone Dial changes selection. |
| Slot selector (which slot is the dial editing) | Internal dial state; maps to `selectedSlot` in editor | `XOceanusEditor::selectedSlot` (int, -1 to 3) — currently tile-click driven | REWIRE | [UI] | **Q1 RESOLVED (A):** Dial = engine picker for active slot. Tile clicks remain the primary slot selector. The two controls are complementary — tiles select the slot, dial browses engines within it. |

---

### 1.3 Preset Navigator

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Left arrow (prev preset) | `processor.getPresetManager().previousPreset()` + `processor.applyPreset()` | EXISTS — `PresetBrowserStrip::prevBtn` with `onClick` lambda | EXISTS | [UI] | Currently `juce::TextButton` with text "<". V1: replace with SVG arrow icon (HeroIcons). |
| Preset name display | `processor.getPresetManager().getCurrentPreset().name` | EXISTS — `PresetBrowserStrip::nameLabel` (juce::Label, Inter 10.5pt) | REWIRE | [UI] | V1: Inter Medium 14pt. Max 200pt width, truncate with ellipsis. Click opens preset browser (sidebar switch). |
| Right arrow (next preset) | `processor.getPresetManager().nextPreset()` + `processor.applyPreset()` | EXISTS — `PresetBrowserStrip::nextBtn` | EXISTS | [UI] | Same as left arrow note. |
| Favorite (heart) toggle | `PresetData::isFavorite` field | Not implemented — no favorite toggle in current UI | BUILD | [UI] | 20x20pt. Outline when off (`#DDDAD5`), filled `#E9C46A` when on. Calls `processor.getPresetManager().toggleFavorite(presetId)`. Needs `PresetManager` method if not present. |
| DNA hexagon mini (24x24pt) | `PresetData::dna` (6D Sonic DNA: brightness, warmth, movement, density, space, aggression) | Not implemented — PresetBrowser has DNA hex in full browser view but not header | BUILD | [UI] | Custom paint: 6-vertex hexagon, vertices displaced by DNA dimensions. High "Movement" adds animated ripple at 0.3Hz (Timer). Click opens DNA detail view (tooltip acceptable for V1). |
| A/B preset comparison toggle | Two `PresetData` slots stored in editor state | Not implemented | BUILD | [UI] | Toggle button. A = current preset, B = last preset before A was loaded. Click A: restore A. Click B: restore B. Data: `PresetData presetA, presetB` in editor. No APVTS param needed. |
| Preset browser (full browser, on name click) | `processor.getPresetManager()` — full library | EXISTS — `PresetBrowserStrip::openBrowser()` launches `PresetBrowser` in `juce::CallOutBox` | EXISTS | [UI] | Currently launched from browse button. V1: clicking name label also opens it. `PresetBrowser` supports search + DNA + mood filter — wired correctly. |
| `PresetManager::Listener` (auto-update name on load) | `PresetManager::Listener::presetLoaded()` | EXISTS — `PresetBrowserStrip` implements `PresetManager::Listener` | EXISTS | [UI] | Correct. No changes needed. |

---

### 1.4 Macro Knobs M1-M4 (Header Position)

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| M1 knob (CHARACTER) | `macro1` (0-1, default 0.5) | EXISTS — `MacroSection::knobs[0]` with `SliderAttachment("macro1")`. Currently in BOTTOM STRIP (105px), not header. | REWIRE | [UI] — SliderAttachment | **Q2 RESOLVED (A):** Full move of MacroSection to header. Restructure MacroSection so knobs live in header right side (44x44pt each, 8pt spacing). MIDI Learn remains wired via `MacroSection::setupMidiLearn()`. Bottom strip MacroSection is removed. **Audit fix:** Knobs enlarged to 44pt for iPad AUv3 touch compliance. |
| M1 label "CHAR" | Static — "CHARACTER" | EXISTS — `MacroSection::lbls[0]` with text "CHARACTER" | REWIRE | [UI] | V1: "CHAR" in XO Gold. 8pt Space Grotesk SemiBold ALL CAPS, letter-spacing +0.08em. |
| M2 knob (MOVEMENT) | `macro2` (0-1, default 0.5) | EXISTS — `MacroSection::knobs[1]` with `SliderAttachment("macro2")` | REWIRE | [UI] | Same as M1 — needs header relocation. |
| M2 label "MOVE" | Static | EXISTS | REWIRE | [UI] | V1: "MOVE" in `#00FF41` (Phosphor Green). |
| M3 knob (COUPLING) | `macro3` (0-1, default 0.5) | EXISTS — `MacroSection::knobs[2]` with `SliderAttachment("macro3")` | REWIRE | [UI] | Same. |
| M3 label "COUP" | Static | EXISTS | REWIRE | [UI] | V1: "COUP" in `#BF40FF` (Prism Violet). |
| M4 knob (SPACE) | `macro4` (0-1, default 0.5) | EXISTS — `MacroSection::knobs[3]` with `SliderAttachment("macro4")` | REWIRE | [UI] | Same. |
| M4 label "SPACE" | Static | EXISTS | REWIRE | [UI] | V1: "SPACE" in `#00B4A0` (Phosphorescent Teal). |
| Macro coupling-modulation pulse ring | `cp_r1_amount` through `cp_r4_amount` — live coupling energy | Not implemented | BUILD | [FIFO] | When coupling modulates a macro target, outer ring pulses at coupling LFO rate. `alpha = 0.6 + 0.4 * sin(t * couplingRate)`. Read coupling energy from lock-free FIFO (never poll audio thread). |
| M1-M4 value readout (0-100) | Same APVTS params | EXISTS — tooltip shows value; `GalleryLookAndFeel::drawRotarySlider` shows value in center disc on drag | REWIRE | [UI] | V1: JetBrains Mono Regular 9pt permanent display below label (not tooltip-only). |
| Master volume knob | `masterVolume` (0-1, default 0.8) | EXISTS — `MacroSection::master` with `SliderAttachment("masterVolume")`. In current bottom strip. | REWIRE | [UI] | **Q10 RESOLVED (A):** Master volume knob moves to header right edge, positioned after the M4 macro knob. Remains part of the header utility cluster alongside the macro knobs. |

---

### 1.5 Utility Controls

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Dark/Light mode toggle | `GalleryColors::darkMode()` — global bool, no APVTS param | EXISTS — `themeToggleBtn` (`juce::TextButton`) with `onClick` calling `GalleryColors::setDarkMode()` + `laf->applyTheme()` + `repaint()` | REWIRE | [UI] | V1: 20x20pt sun/moon SVG icon (HeroIcons). Currently text button. Behavior correct. |
| CPU meter | `processor.getMIDILearnManager()` — no, wrong. CPU from `juce::AudioProcessorGraph` or custom atomic | Not implemented as a displayed meter; header shows MIDI Learn badge only | BUILD | [UI] — atomic read | JetBrains Mono 10pt "CPU: 4.2%". Processor must expose `std::atomic<float> cpuPercent` updated in `processBlock`. Color: green < 30%, amber 30-70%, red > 70%. Update at 1Hz via existing timer. |
| MIDI activity indicator | `processor.getMIDILearnManager().isLearning()` | EXISTS — MIDI Learn badge in header (amber pulsing when active). No general MIDI note-on flash. | REWIRE | [UI] | V1: 8x8pt circle. Flashes engine accent on note-on (50ms flash, 200ms decay). Requires lock-free FIFO from audio thread signaling note-on events (already partially present: `XOceanusProcessor::NoteMapEvent` drained via `drainNoteEvents()`). Wire to indicator component. |
| Settings gear | Opens settings panel | Not implemented | BUILD | [UI] | 20x20pt HeroIcons cog. Opens settings panel (see Section 4.5 — C5 Settings tab). Can use `juce::CallOutBox` for V1. |
| Export button | Launches `ExportDialog` | EXISTS — `exportBtn` (`juce::TextButton` "EXPORT") with `onClick` launching `ExportDialog` in `juce::CallOutBox` | EXISTS | [UI] | Already correct. ExportDialog.h handles XPN export pipeline. |
| Performance Lock toggle | `performanceLock` bool — no APVTS param needed | Not implemented | BUILD | [UI] | When active: all param changes blocked except PlaySurface and FIRE/XOSEND/ECHO/PANIC. Visual: lock icon on affected controls at 40% opacity. Toggle: padlock icon in header utility cluster. Status bar indicator (see Section 6). |

---

## 2. COLUMN A — ENGINE RACK (260pt)

### Current Implementation
Left sidebar is 155px wide (`kSidebarW = 155`). Contains 4 `CompactEngineTile` instances stacked vertically. Each tile shows: slot number in porthole circle, engine name, voice activity dots (up to 4), empty-slot "+" affordance. No mini waveform, no 4 mini macro indicators, no coupling indicator dots, no mini coupling graph at bottom.

**Target (V1):** 260pt Engine Rack. Each slot tile: creature icon, engine name, mini waveform (32×16pt lock-free FIFO), 4 mini macro indicators, coupling indicator dots. Mini coupling graph at bottom of column.

---

### 2.1 Column A Container

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Column A container | Layout | `kSidebarW = 155` in `XOceanusEditor` | REWIRE | [UI] | Widen to 260pt. Update `resized()` math accordingly. All right-panel widths shrink proportionally. |
| "Add Engine" button (`[+ ENGINE]`) | `EngineRegistry::instance()` — load engine into empty slot | Not implemented as a separate button; empty tiles show "+" and can be right-clicked | BUILD | [UI] | `juce::TextButton` below the 4 tiles. Opens engine picker (same popup as tile right-click `showLoadMenu()`). |

---

### 2.2 Slot Tile — Per-Slot Elements (4 tiles)

**Existing class:** `CompactEngineTile` in `XOceanusEditor.h` at line 1447.

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Tile background + border | `isSelected`, `hasEngine`, `isMouseOver()` | EXISTS — depth-zone gradient when selected, `slotBg()` otherwise, accent border on select/hover | EXISTS | [UI] | Correct. No change needed. |
| On/Off toggle (engine mute) | `{prefix}_enabled` or per-slot bypass param | Not implemented in tile | BUILD | [UI] | Small toggle (16x16pt, top-left of tile). `SliderAttachment` or `ButtonAttachment` to a per-slot bypass parameter. Needs new APVTS params: `slot1_enabled`, `slot2_enabled`, `slot3_enabled`, `slot4_enabled` (bool, default true). Add to `XOceanusProcessor::createParameters()`. |
| Creature icon (24x24pt outline SVG) | `eng->getEngineId()` → icon asset lookup | NOT implemented — porthole circle with slot number is current substitute | BUILD | [UI] | 76 SVG icons required (one per engine), outline-only, 1.5px stroke, engine accent color. Asset path convention: `Resources/creatures/{engineId}.svg`. For V1 fallback: continue using porthole+slot-number until all icons created. |
| Engine name label | `eng->getEngineId()` | EXISTS — `GalleryFonts::heading(11.0f)`, engine ID uppercase | REWIRE | [UI] | V1 spec: Space Grotesk SemiBold, engine accent color when selected (currently `get(textDark())` when unselected). Layout adjustment needed for wider tile. |
| Mini waveform (32×16pt) | Lock-free FIFO — 256 audio samples from audio thread | Not implemented | BUILD | [FIFO] | `juce::Path` from ring buffer of 256 samples. Processor must write samples to per-slot lock-free FIFO (e.g., `juce::AbstractFifo` + `std::array<float, 256>`). UI reads at 30Hz. Stroke 1.5px, engine accent color. No waveform when slot empty. |
| 4 mini macro indicators | `macro1` through `macro4` APVTS values | Not implemented | BUILD | [UI] — atomic APVTS read | 4 small dots or micro-bars below engine name. Each reads `processor.getAPVTS().getRawParameterValue("macroN")->load()` at 10Hz refresh. Color: M1=XO Gold, M2=Phosphor Green, M3=Prism Violet, M4=Teal. Size: 4x4pt circles or 20x3pt bars. |
| Coupling indicator dots | `MegaCouplingMatrix::getRoutes()` — active routes involving this slot | EXISTS (partially) — `CouplingArcOverlay` draws arcs over the sidebar, tile itself has no coupling dot | REWIRE | [UI] | V1: 1-4 small dots on tile (1 dot per active coupling route involving this slot). Color = arc type color (XO Gold, Twilight Blue, or Midnight Violet). `CompactEngineTile::refresh()` already calls `processor.getEngine(slot)` — extend to also query `processor.getCouplingMatrix().getRoutes()`. |
| Voice count dots (right edge) | `eng->getActiveVoiceCount()` — polled via 10Hz timer | EXISTS — up to 4 dots with alpha ramp | EXISTS | [ATOMIC] | Correct implementation. |
| Left accent strip (voice activity) | `eng->getActiveVoiceCount()` | EXISTS — 3px left strip, voice-density-driven alpha | EXISTS | [ATOMIC] | Correct. |
| Porthole circle | Slot number display / voice indicator | EXISTS — 30px circle, slot number, inner fill + ring responsive to voice count | REWIRE | [UI] | V1: replace porthole with creature icon once icons exist. For now, porthole remains valid. |
| Tile select callback | `selectSlot(int)` in editor | EXISTS — `onSelect` lambda, calls `selectSlot()` | EXISTS | [UI] | Correct. |
| Tile keyboard navigation (1-4 keys) | Keyboard shortcut → `selectSlot()` | EXISTS — `keyPressed()` in `XOceanusEditor`: '1'=slot 0, '2'=slot 1, '3'=slot 2, '4'=slot 3 | EXISTS | [UI] | Correct. |
| Engine load popup (right-click) | `EngineRegistry::instance().getRegisteredIds()` | EXISTS — `showLoadMenu()` in `CompactEngineTile::mouseDown` — scans registry, shows `juce::PopupMenu` | EXISTS | [UI] | Correct. Hot-swap handled by processor. |
| Engine remove (right-click → Remove) | `processor.removeEngine(slot)` | EXISTS — in `showLoadMenu()` popup | EXISTS | [UI] | Correct. |
| Focus ring (WCAG 2.4.7) | Keyboard focus | EXISTS — `A11y::drawFocusRing()` in `CompactEngineTile::paint()` | EXISTS | [UI] | Correct. |
| Loading state display | `isLoading` bool in tile | EXISTS — "LOADING..." text during hot-swap | EXISTS | [UI] | Correct. |
| Tooltip | `hasEngine ? "Click to edit..." : "Slot N: empty..."` | EXISTS — `setTooltip()` called in `refresh()` | EXISTS | [UI] | Correct. |

---

### 2.3 Mini Coupling Graph (Bottom of Column A)

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Mini coupling graph | `processor.getCouplingMatrix().getRoutes()` + per-slot accent colors | Not implemented — `OverviewPanel` (in Column B) shows a coupling graph; no Column A miniature | BUILD | [UI] | Small version of `OverviewPanel` coupling graph, ~260x80pt. 4 slot nodes arranged vertically (matching tile positions). Active arcs drawn with type-coded colors. Refreshed at 5Hz (not 30Hz — miniature, low visual priority). Can reuse `CouplingVisualizer.h` component scaled down. |

---

## 3. COLUMN B — MAIN CANVAS (520pt)

### Current Implementation
Column B is the "right panel" area (editor width minus 155px sidebar). Shows one of: `OverviewPanel`, `EngineDetailPanel`, `ChordMachinePanel`, `PerformanceViewPanel` — one visible at a time with 150ms fade transitions. Below the panel stack: `FieldMapPanel` (110px, always visible).

**Target (V1):** 520pt Main Canvas. Views: B1 Engine Detail (default), B2 Overview/Coupling Graph, B3 FX Chain, B4 Chord Machine, B5 Export Panel. Signal Flow Mini-Diagram strip (520×32pt).

---

### 3.1 Panel Switching System

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Panel stack (one visible at a time) | Internal editor state (`selectedSlot`, toggle buttons) | EXISTS — `OverviewPanel`, `EngineDetailPanel`, `ChordMachinePanel`, `PerformanceViewPanel` stacked with `setVisible()`/opacity fade | REWIRE | [UI] | V1 adds `FXChainPanel` (B3) as a new panel mode. Existing `showOverview()`, `showChordMachine()`, `showPerformanceView()`, `selectSlot()` pattern is correct. Add `showFXChain()`. |
| 150ms fade transitions | `juce::Desktop::getInstance().getAnimator()` | EXISTS — `fadeIn`/`fadeOut` + `callAfterDelay(kFadeMs)` throughout | EXISTS | [UI] | Correct. SafePointer pattern correctly handles editor destruction during animation. |
| Signal Flow Mini-Diagram (520×32pt) | `processor.getEngine(0-3)->getEngineId()`, coupling routes | Not implemented | BUILD | [UI] | Horizontal strip showing audio path left-to-right: [S1 box] → [coupling arc] → [S2 box] → [FX icon] → [OUT]. Painted as a custom `juce::Component`. Only visible when Overview or Engine Detail mode active. 32pt height. |

---

### 3.2 B1 — Engine Detail View (default)

**Existing class:** `EngineDetailPanel` at line 887. Contains `MacroHeroStrip` (4 pillar sliders for engine macros) + `juce::Viewport` with `ParameterGrid` (all params scrollable).

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Engine nameplate | `eng->getEngineId()` | EXISTS — gradient header, engine name in white Space Grotesk 16pt | REWIRE | [UI] | V1: left 4px vertical accent stripe + creature icon beside name. Add `engineIdentityBand` to `EngineDetailPanel::paint()`. |
| Ecological header gradient (accent → midnight) | `eng->getAccentColour()` | EXISTS — `juce::ColourGradient` in `EngineDetailPanel::paint()` | EXISTS | [UI] | Correct. |
| Zone depth bands (3 strips) | Static visual | EXISTS — 3 × 2px horizontal strips in `EngineDetailPanel::paint()` | EXISTS | [UI] | Correct. |
| Macro Hero Strip — 4 pillar sliders | `{prefix}_macro*` params discovered dynamically | EXISTS — `MacroHeroStrip::loadEngine()` iterates APVTS for `{prefix}_macro*` params, up to 4 | EXISTS | [UI] — SliderAttachment | Correct. MIDI Learn wired via `setMidiLearnManager()`. |
| Macro pillar labels (per-engine names) | `EngineVocabulary::labelFor()` — vocabulary overrides, else raw param name | EXISTS — in `MacroHeroStrip::loadEngine()` | EXISTS | [UI] | Correct. |
| Parameter Grid — OSC/FILTER/MOD/FX/MACRO/OTHER sections | All `{prefix}_*` params via `ParameterGrid` constructor | EXISTS — `ParameterGrid` iterates `processor.getParameters()`, classifies by keyword, renders in sections with color-coded headers | EXISTS | [UI] — SliderAttachment | Correct. Section colors, header rows, knob cells all implemented. |
| Progressive disclosure Level 1 (4 macro knobs visible) | `{prefix}_macro*` | EXISTS via `MacroHeroStrip` | REWIRE | [UI] | V1 spec calls for 48x48pt kit-07 illuminated style. Currently pillar sliders. Consider adding Level 1 mode that shows 4 large rotary knobs before expanding to pillar+grid view. Decision in Section 12. |
| Progressive disclosure Level 2 (group view, 12-16 controls) | Per-section APVTS params | PARTIAL — `ParameterGrid` sections exist but no collapse/expand toggle | REWIRE | [UI] | Add `[EXPAND]` / `[COLLAPSE]` per section header in `ParameterGrid`. Animate height with `juce::ComponentAnimator`. |
| Progressive disclosure Level 3 (full scrollable) | All `{prefix}_*` | EXISTS — `ParameterGrid` in viewport already scrollable | EXISTS | [UI] | Correct. This is the current default behavior. |
| Oscilloscope / waveform display (200x80pt) | Lock-free FIFO — 256 samples from audio thread | NOT in `EngineDetailPanel` — `FieldMapPanel` exists (note events, not waveform) | BUILD | [FIFO] | Custom `juce::Component` reading waveform FIFO. Draw `juce::Path` from 256-sample buffer. 30Hz update via `AsyncUpdater`. CRT aesthetic only for OPTIC engine. |
| ADSR envelope display (200x60pt interactive) | Attack, Decay, Sustain, Release params of active engine (`{prefix}_attack`, `{prefix}_decay`, etc.) | Not implemented | BUILD | [UI] — SliderAttachment proxy | Custom `juce::Component`. 4 draggable points (8pt circles). `juce::Path` line segments. Drag detection via `mouseDown()` proximity. Proxy approach: find `{prefix}_attack`, `{prefix}_decay`, `{prefix}_sustain`, `{prefix}_release` params (if they exist); degrade gracefully if engine has different envelope naming. |
| Section header section color dots | Static per-section color | EXISTS — `ParameterGrid::paint()` draws section headers with color | EXISTS | [UI] | Correct. |
| Knob cell left-border bars (3px, section color) | Static per-section color | EXISTS — `ParameterGrid::paint()` draws per-cell left-border bars | EXISTS | [UI] | Correct. |
| GalleryKnob double-click reset | `rp->getDefaultValue()` → `knob.setDefaultValue()` | EXISTS — `enableKnobReset()` helper called for every knob in `ParameterGrid` and `MacroSection` | EXISTS | [UI] | Correct. |
| GalleryKnob Cmd+click reset | Same as above | EXISTS — `GalleryKnob::mouseDown()` checks `isCommandDown()` | EXISTS | [UI] | Correct. |
| MIDI Learn right-click on any knob | `MIDILearnManager` — maps CC → param | EXISTS — `GalleryKnob::setupMidiLearn()`, amber pulse ring, green "ML" badge | EXISTS | [UI/AUDIO atomic] | Correct. Audio thread writes pending CC via atomic; UI drains via `checkPendingLearn()` in 1Hz/10Hz timer. |

---

### 3.3 B2 — Overview / Coupling Graph View

**Existing class:** `OverviewPanel` at line 1165.

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Overview coupling graph | `processor.getCouplingMatrix().getRoutes()` + per-slot accent colors | EXISTS — `OverviewPanel::paint()` draws engine nodes in diamond arrangement, Bézier arcs between coupled pairs, coupling type labels | REWIRE | [UI] | V1: add real-time arc pulse animations (Perlin/Simplex noise on control points at 0.5Hz). Currently static arcs. Add `juce::Timer` at 10Hz to `OverviewPanel`. |
| Engine nodes (24px circles) | `eng->getAccentColour()`, `eng->getEngineId()` | EXISTS — `OverviewPanel::cachedActiveEngines` (refreshed via `refresh()`) | REWIRE | [UI] | V1: 28px circles per spec. Active: 100% opacity + 2px glow. Inactive: 25% opacity. |
| Coupling arc labels (JetBrains Mono 9pt) | `couplingTypeLabel(route.type)` | EXISTS — `couplingTypeLabel()` helper + label drawn centered on arc midpoint | EXISTS | [UI] | Correct. |
| Inline coupling popover (click on arc) | `cp_r{N}_type`, `cp_r{N}_amount`, `cp_r{N}_active` | NOT in `OverviewPanel` — arcs in the overlay don't receive clicks (`setInterceptsMouseClicks(false, false)`) | BUILD | [UI] — APVTS | V1 mandates inline popover on arc click (type/amount/depth in-place). `CouplingArcOverlay` currently passes through all mouse events. Add hit-testing per arc in a new clickable overlay variant (or move arc drawing into a clickable component). Shows: type dropdown, amount slider (bipolar), active toggle. |
| Living Gold Corridor (coupling active gold band) | `cp_r{N}_amount` values — coupling energy RMS | Not implemented | BUILD | [UI] — FIFO | 24px horizontal gold band at top of overview area. `alpha = 0.7 + 0.3 * couplingRMS`. Gold glow 4px above/below. Coupling energy from lock-free FIFO. |
| Empty state (no engines) | Static | EXISTS — XO Gold logo mark + "Load engines to begin" instruction text | EXISTS | [UI] | Correct. |
| `OverviewPanel::refresh()` | Called from `XOceanusEditor::timerCallback()` | EXISTS — reads `getCouplingMatrix().getRoutes()` + per-slot engine info | EXISTS | [UI] | Correct. Called at 1Hz/10Hz depending on MIDI Learn state. |

---

### 3.4 B3 — FX Chain View

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| FX Chain Panel (6 slots) | `master_sat*`, `master_corr*`, `master_comb*`, `master_fshift*`, `master_ott*`, `master_reverb*`, `master_comp*`, `master_delay*`, `master_mod*`, `master_seq*`, `master_vibe*`, `master_tilt*` etc. | PARTIAL — `MasterFXSection` exists (bottom strip, 68px). `AdvancedFXPanel` exists as popup for expanded params. | REWIRE | [UI] — SliderAttachment | V1 places FX Chain as a full Column B panel mode. `MasterFXSection` in bottom strip needs to be supplemented (or replaced) by the B3 panel when FX button is toggled in header or Column C. The bottom strip can remain as a compact always-visible summary. |
| FX slot 1 — Saturation | `master_satDrive` (primary), `master_satMode` (choice) | EXISTS — `MasterFXSection` has DRIVE slider and MODE combo for sat | REWIRE | [UI] | Move/expose in B3 panel. Full list: `master_satDrive`, `master_satMode`. ADV: `AdvancedFXPanel` launched from ADV button. |
| FX slot 2 — Corruption | `master_corrMix`, `master_corrBits`, `master_corrSR`, `master_corrFM`, `master_corrTone` | EXISTS — partially in `MasterFXSection` | REWIRE | [UI] | Full B3 expansion needed. |
| FX slot 3 — Comb / Spectral | `master_combMix`, `master_combFreq`, `master_combFeedback`, `master_combDamping`, `master_combNoise`, `master_combSpread`, `master_combOffset2`, `master_combOffset3` | EXISTS — partially | REWIRE | [UI] | |
| FX slot 4 — Frequency Shift | `master_fshiftHz`, `master_fshiftMix`, `master_fshiftMode`, `master_fshiftFeedback` | EXISTS — partially | REWIRE | [UI] | |
| FX slot 5 — Reverb | `master_reverbSize`, `master_reverbMix` | EXISTS | REWIRE | [UI] | B3 full expansion. |
| FX slot 6 — Compressor | `master_compRatio`, `master_compAttack`, `master_compRelease`, `master_compMix` | EXISTS | REWIRE | [UI] | |
| Additional FX: Delay | `master_delayTime`, `master_delayFeedback`, `master_delayMix`, `master_delayPingPong`, `master_delayDamping`, `master_delayDiffusion`, `master_delaySync` | EXISTS in `MasterFXSection` | REWIRE | [UI] | |
| Additional FX: Mod | `master_modRate`, `master_modDepth`, `master_modMix`, `master_modMode`, `master_modFeedback` | EXISTS | REWIRE | [UI] | |
| Additional FX: Sequencer | `master_seqEnabled`, `master_seqRate`, `master_seqSteps`, `master_seqDepth`, `master_seqSmooth`, `master_seqTarget1`, `master_seqTarget2`, `master_seqPattern`, `master_seqEnvFollow`, `master_seqEnvAmount` | EXISTS | REWIRE | [UI] | |
| Additional FX: Vibe/Tilt | `master_vibeAmount`, `master_tiltAmount`, `master_tiltMix` | EXISTS | REWIRE | [UI] | |
| Additional FX: Transient Designer | `master_tdAttack`, `master_tdSustain`, `master_tdMix` | EXISTS | REWIRE | [UI] | |
| Additional FX: Doppler | `master_dopplerDist`, `master_dopplerSpeed`, `master_dopplerMix` | EXISTS | REWIRE | [UI] | |
| Additional FX: Smear | `master_smearAmount`, `master_smearGrainSize`, `master_smearDensity`, `master_smearMix` | EXISTS | REWIRE | [UI] | |
| Additional FX: Exciter | `master_excDrive`, `master_excFreq`, `master_excTone`, `master_excMix` | EXISTS | REWIRE | [UI] | |
| Additional FX: Sculpt (MS width) | `master_sculLowWidth`, `master_sculMidWidth`, `master_sculHighWidth`, `master_sculLowCross`, `master_sculHighCross`, `master_sculMix` | EXISTS | REWIRE | [UI] | |
| Additional FX: Width (stereo) | `master_pwidthAmount`, `master_pwidthHaas`, `master_pwidthComb`, `master_pwidthMono`, `master_pwidthMix` | EXISTS | REWIRE | [UI] | |
| Master Limiter | `master_limCeiling`, `master_limRelease` | EXISTS | REWIRE | [UI] | Always-on. |
| Osmosis FX (membrane modeling) | `master_osmMembrane`, `master_osmReactivity`, `master_osmResonance`, `master_osmSaturation`, `master_osmMix` | EXISTS | REWIRE | [UI] | |
| MFX Formant | `mfx_formantShift`, `mfx_formantVowel`, `mfx_formantQ`, `mfx_formantMix` | EXISTS | REWIRE | [UI] | |
| MFX Breath | `mfx_breathAmount`, `mfx_breathTilt`, `mfx_breathSens`, `mfx_breathMix` | EXISTS | REWIRE | [UI] | |
| On-delay | `master_onDelayTime`, `master_onDelayShiftHz`, `master_onDelayFeedback`, `master_onDelayDamping`, `master_onDelaySpread`, `master_onDelayMix` | EXISTS | REWIRE | [UI] | |
| FX slot bypass toggles | Per-FX `bypass` parameter or `isEnabled` bool | EXISTS — `AdvancedFXPanel` has bypass in some sections | REWIRE | [UI] | V1: 12x12pt circle bypass toggle per slot in B3. |
| FX slot wet/dry knob (24x24pt mini) | Per-FX `_mix` parameter | EXISTS | REWIRE | [UI] | |
| Drag-to-reorder FX slots | No APVTS param — internal routing order | Not implemented | BUILD | [UI] | Drag handle (3 horizontal dots, 12x8pt). Reordering changes DSP routing order in processor. |
| Serial/Parallel chain mode toggle | `master_fxMode` (if exists; OBRIX has `obrix_fxMode`) | EXISTS in OBRIX (`obrix_fxMode` — 0=Serial, 1=Parallel). Master-level chain mode not parameterized. | BUILD | [UI] | Add `master_fxChainMode` APVTS param (bool, default false=serial). Space Grotesk SemiBold 9pt ALL CAPS toggle. |
| FX chain mode selection button (in header) | Internal editor state → `showFXChain()` | Not implemented | BUILD | [UI] | Adds a new toggle to header controls cluster OR Column C FX tab triggers Column B to show B3. |

---

### 3.5 B4 — Chord Machine View

**Existing class:** `ChordMachinePanel` at line 2658.

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| ON/OFF toggle | `cm_enabled` | EXISTS — `enableBtn` with `TextButton`, `p->setValueNotifyingHost()` | EXISTS | [UI] | Correct. |
| Chord Strip (4 slot cards) | `ChordMachine::getCurrentAssignment()`, per-slot `eng->getAccentColour()` | EXISTS — 4 cards drawn in `paint()` with MIDI note name, engine name, accent bar at bottom | EXISTS | [UI] | Correct. |
| 16-step sequencer grid | `ChordMachine::getStep(s).active`, `getCurrentStep()`, `isSequencerRunning()` | EXISTS — 16 cells drawn with active/current/normal states, beat markers | EXISTS | [UI] | Step highlighting at 10Hz. Correct. |
| Sequencer SEQ play/stop button | `cm_seq_running` | EXISTS — `seqBtn` with `ButtonAttachment`-style param write | EXISTS | [UI] | Correct. |
| Palette selector | `cm_palette` | EXISTS — `paletteBox` ComboBox, 8 items, manual `setValueNotifyingHost` | REWIRE | [UI] | Should use `ComboBoxAttachment` for full APVTS integration and undo support. Currently sets raw value. |
| Voicing selector | `cm_voicing` | EXISTS — `voicingBox` ComboBox, 5 items | REWIRE | [UI] | Same — should use `ComboBoxAttachment`. |
| Pattern selector | `cm_seq_pattern` + `ChordMachine::applyPattern()` | EXISTS — `patternBox`, 8 items, also calls `applyPattern()` directly | EXISTS | [UI] | Correct — dual: APVTS + ChordMachine method. |
| Velocity curve selector | `cm_vel_curve` | EXISTS — `velCurveBox`, 4 items | REWIRE | [UI] | Same — should use `ComboBoxAttachment`. |
| Spread knob | `cm_spread` | EXISTS — `spreadKnob` with `SliderAttachment("cm_spread")` | EXISTS | [UI] | Correct. |
| BPM knob | `cm_seq_bpm` | EXISTS — `bpmKnob` with `SliderAttachment("cm_seq_bpm")` | EXISTS | [UI] | Correct. |
| Swing knob | `cm_seq_swing` | EXISTS — `swingKnob` with `SliderAttachment("cm_seq_swing")` | EXISTS | [UI] | Correct. |
| Gate knob | `cm_seq_gate` | EXISTS — `gateKnob` with `SliderAttachment("cm_seq_gate")` | EXISTS | [UI] | Correct. |
| Humanize knob | `cm_humanize` | EXISTS — `humanizeKnob` with `SliderAttachment("cm_humanize")` | EXISTS | [UI] | Correct. |
| Sidechain Duck knob | `cm_sidechain_duck` | EXISTS — `duckKnob` with `SliderAttachment("cm_sidechain_duck")` | EXISTS | [UI] | Correct. |
| ENO mode toggle | `cm_eno_mode` | EXISTS — `enoBtn` with `setValueNotifyingHost` | REWIRE | [UI] | Should use `ButtonAttachment`. |
| Dynamic spread label ("WIDE" etc.) | `ChordMachine::spreadLabel(curSpread)` | EXISTS — dynamic label drawn in `paint()` | EXISTS | [UI] | Correct. |
| Step cell click-to-toggle | `ChordMachine::toggleStep(s)` | NOT implemented — step grid is paint-only, no mouse listener | BUILD | [UI] | Add `mouseDown()` to `ChordMachinePanel`. Calculate which step was clicked from X position. Call `processor.getChordMachine().toggleStep(s)`. |
| Chord machine panel toggle (header "CM" button) | Internal editor state | EXISTS — `cmToggleBtn`, `showChordMachine()` / `showOverview()` | EXISTS | [UI] | Correct. |

---

### 3.6 B5 — Export Panel

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Export Panel | `ExportDialog` — full XPN export pipeline | EXISTS — `ExportDialog` launched in `juce::CallOutBox` from `exportBtn` click. Not a persistent panel. | REWIRE | [UI] | V1: Export Panel is a persistent Column B view (survives focus loss, unlike `CallOutBox`). Add `ExportPanel` class (port `ExportDialog` internals). Add export toggle button. |

---

## 4. COLUMN C — BROWSER/INSPECTOR (320pt)

### Current Implementation
Column C does not exist as a distinct component. Preset browsing: `PresetBrowserStrip` (220px in header) + `PresetBrowser` in `CallOutBox`. Coupling inspection: `PerformanceViewPanel` (full Column B). FX: `MasterFXSection` (bottom strip). No tabbed sidebar exists.

**Target (V1):** 320pt tabbed sidebar: PRESET | COUPLE | FX | PLAY | EXPORT | SETTINGS

---

### 4.1 Column C Container + Tab Bar

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Column C container (320pt) | Layout | Not implemented | BUILD | [UI] | New `juce::Component` class `SidebarPanel`. Contains tab bar + content area. Added to `XOceanusEditor::resized()`. |
| Tab bar (36pt, 6 tabs) | Internal state | Not implemented | BUILD | [UI] | Tabs: PRESET, COUPLE, FX, PLAY, EXPORT, SETTINGS. Space Grotesk SemiBold 10pt ALL CAPS. Active: engine accent color text + 2px underline. Inactive: `rgba(240,237,232,0.45)`. |

---

### 4.2 C1 — Preset Tab

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Mood filter row (15 pill buttons) | `processor.getPresetManager()` — filter by mood string | EXISTS — inside `PresetBrowser` (launched in `CallOutBox`) | REWIRE | [UI] | Move mood filter row into the Column C Preset tab. `PresetBrowser` component can be embedded directly. |
| Search field | `PresetManager` — filter by text query | EXISTS — inside `PresetBrowser` | REWIRE | [UI] | Same — embed `PresetBrowser` in C1. |
| Preset list (scrollable, 72pt cards) | `PresetManager::getLibrary()` filtered by mood + search | EXISTS — inside `PresetBrowser` | REWIRE | [UI] | Each card: DNA hex (32x32), preset name (Inter Med 13pt), engine badge (8pt caps), mood dot, tags, heart toggle. |
| DNA hexagon per card | `PresetData::dna` 6D struct | EXISTS — `PresetBrowser` draws DNA hex in cards | EXISTS | [UI] | Correct. |
| Preset card hover/selected states | Internal state | EXISTS — in `PresetBrowser` | EXISTS | [UI] | Correct. |
| Preset card click → apply | `processor.applyPreset(preset)` | EXISTS — `PresetBrowser::onPresetSelected` callback | EXISTS | [UI] | Correct. |
| Type filter (engine category) | Preset category metadata | EXISTS — mood filter in PresetBrowser | REWIRE | [UI] | V1 adds engine-type filter. Needs `PresetData` to carry engine category field, or derive from engine list. |

---

### 4.3 C2 — Coupling Inspector Tab

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| CouplingVisualizer (380x240pt) | `processor.getCouplingMatrix()`, per-slot accent colors | EXISTS — `CouplingVisualizer.h`, used in `PerformanceViewPanel` | REWIRE | [UI] | Embed `CouplingVisualizer` in C2 tab. Remove it from being Column B only. Engine nodes: 28pt circles, diamond arrangement. |
| Coupling crossfader | `cp_r{N}_amount` | EXISTS — `PerformanceViewPanel` has per-route depth slider | REWIRE | [UI] | V1 wants a crossfader (linear, bidirectional). Current implementation: 4 separate bipolar sliders. Consider a single global coupling intensity crossfader + individual route overrides. |
| Coupling type selector dropdown | `cp_r{N}_type` | EXISTS — `PerformanceViewPanel::RouteSection::typeBox` with `ComboBoxAttachment` | REWIRE | [UI] | Embed in C2. 15 coupling types (currently 14 in `ChordMachinePanel` — missing TriangularCoupling). |
| Coupling amount knob (48x48pt) | `cp_r{N}_amount` | EXISTS — per-route depth slider | REWIRE | [UI] | V1: 48x48pt rotary knob per route. `SliderAttachment` to `cp_r{N}_amount`. |
| Route active toggles (4 routes) | `cp_r1_active` through `cp_r4_active` | EXISTS — `RouteSection::activeBtn` with `ButtonAttachment` | REWIRE | [UI] | Move from `PerformanceViewPanel` to C2. |
| Source/Target slot selectors | `cp_r{N}_source`, `cp_r{N}_target` | EXISTS — `RouteSection::sourceBox`/`targetBox` with `ComboBoxAttachment` | REWIRE | [UI] | Move from `PerformanceViewPanel` to C2. |
| BAKE button | `CouplingPresetManager::bakeCurrent()` | EXISTS — in `PerformanceViewPanel` | REWIRE | [UI] | Move to C2 Coupling tab. |
| CLR (clear all routes) button | `CouplingPresetManager::clearOverlay()` | EXISTS — in `PerformanceViewPanel` | REWIRE | [UI] | Move to C2. |
| Coupling preset dropdown | `CouplingPresetManager` library | EXISTS — `couplingPresetBox` in `PerformanceViewPanel` | REWIRE | [UI] | Move to C2. |

---

### 4.4 C3 — FX Inspector Tab

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| FX chain slot list | Master FX params | EXISTS — `MasterFXSection` in bottom strip | REWIRE | [UI] | C3 shows the FX chain in inspector mode (compact). Clicking a slot expands inline params. Bottom strip (`MasterFXSection`) remains as compact always-visible summary. |
| Per-FX wet/dry | `master_*Mix` params | EXISTS — in `MasterFXSection` per section | REWIRE | [UI] | Expose in C3 with mini knobs. |
| Per-FX bypass | Per-FX bypass state | EXISTS partially | REWIRE | [UI] | 12x12pt circle toggle per slot. |
| Per-FX inline expansion (click → expand) | Per-FX detailed params | EXISTS — `AdvancedFXPanel` launched in `CallOutBox` | REWIRE | [UI] | V1: inline expansion in C3 (not `CallOutBox`). Click on FX slot → sub-panel expands below with all params. Use `juce::ComponentAnimator` to animate height. |
| Drag to reorder | Internal routing order | Not implemented | BUILD | [UI] | Same as B3 note. |

---

### 4.5 C4 — Play Tab

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Mod Wheel display | CC1 via `ToucheExpression` or MIDI input | EXISTS — `PlaySurface` has `ToucheExpression.h` for expression controllers | REWIRE | [UI] | C4 shows these when sidebar is in Play mode (narrow window, PlaySurface collapsed). Vertical strip, 28x160pt. |
| Pitch Bend display | Pitch Bend CC | Similar | REWIRE | [UI] | |
| XY Pad (160x160pt) | Assignable — default X=filter cutoff, Y=resonance | Not implemented standalone | BUILD | [UI] | Custom `juce::Component`. `mouseDown`/`mouseDrag` → `setValueNotifyingHost()` on two params. |
| Macro Strips (4 vertical, 20pt wide) | `macro1` through `macro4` | EXISTS — `MacroSection` exists but not in C4 | REWIRE | [UI] | Cannot have two `SliderAttachment`s to same param. Solution: vertical visual-only meters driven by `getRawParameterValue()`, with click-drag updating via `setValueNotifyingHost()`. |
| Tide Controller (120pt diameter) | Assignable — default filter cutoff | Not implemented | BUILD | [UI] | `TideController : public juce::Component, private juce::Timer`. 16x16 `float` arrays. `paint()` as filled rects with color interpolation. Output: `h[8][8]` normalized 0-1, calls `setValueNotifyingHost()` on assigned param. |
| Breath Display | `mfx_breathAmount` | Not implemented | BUILD | [UI] | |
| Scale/quantize selector | Internal PlaySurface state — no APVTS | EXISTS — in `PlaySurface.h` | REWIRE | [UI] | Expose in C4 for quick access when PlaySurface is active. |

---

### 4.6 C5 — Settings Tab

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| MPE enable toggle | `mpe_enabled` | Not exposed in UI | BUILD | [UI] — ButtonAttachment | `juce::ToggleButton` with `ButtonAttachment("mpe_enabled")`. |
| MPE zone selector | `mpe_zone` | Not exposed | BUILD | [UI] — ComboBoxAttachment | `juce::ComboBox` with `ComboBoxAttachment("mpe_zone")`. |
| MPE pitch bend range | `mpe_pitchBendRange` | Not exposed | BUILD | [UI] — SliderAttachment | `juce::Slider` (48 semitones max). |
| MPE pressure target | `mpe_pressureTarget` | Not exposed | BUILD | [UI] — ComboBoxAttachment | Param target selector. |
| MPE slide target | `mpe_slideTarget` | Not exposed | BUILD | [UI] — ComboBoxAttachment | |
| Dark mode toggle (duplicate from header) | `GalleryColors::setDarkMode()` | EXISTS — `themeToggleBtn` in header | REWIRE | [UI] | Add toggle in Settings tab too for discoverability. Calls same function. |
| Interactive hover toggle (first launch mode) | `isFirstLaunch` flag — persisted in user data | Not implemented | BUILD | [UI] | Toggle: "Interactive Hover" — enables/disables hover-modulation feature from first-launch experience. Store in `juce::PropertiesFile`. |
| Reduced motion toggle | `A11y::reducedMotion` global | Not implemented | BUILD | [UI] | Affects animation update rates (10Hz vs 30Hz). WCAG 2.3.3 (Motion). |
| Performance Lock toggle (also in status bar) | Internal bool — no APVTS | Not implemented | BUILD | [UI] | See Section 1.5. Duplicate control in Settings for discoverability. |
| Audio device settings | `juce::AudioDeviceManager` | Not implemented in plugin context | BUILD | [UI] | For standalone build only. Show `juce::AudioDeviceSelectorComponent`. Hide in AU/VST3 context. |
| MIDI mappings list | `MIDILearnManager::getMappings()` | Not implemented as UI | BUILD | [UI] | Table listing all CC→param mappings. Clear individual or all. JetBrains Mono 9pt. |
| About / version | Static | Not implemented | BUILD | [UI] | Version string from `ProjectInfo::versionString` (or CMake define). Link to patreon. |

---

## 5. PLAYSURFACE (220pt)

**Existing class:** `PlaySurface.h` in `Source/UI/PlaySurface/`.

**Current Implementation:** PlaySurface exists. Wired via `playSurface.setMidiCollector(&proc.getMidiCollector(), 1)`. Toggle via `surfaceToggleBtn`. When visible: occupies bottom `40%` of content height (dynamic). 150ms fade in/out. Tab bar (XOUIJA|MPC|KEYS) presumably in `PlaySurface.h`.

---

### 5.1 PlaySurface Container

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| PlaySurface toggle (PS button) | Internal editor state | EXISTS — `surfaceToggleBtn` with `showPlaySurface()` / `hidePlaySurface()` | EXISTS | [UI] | Correct. 150ms fade. `resized()` recalculates on show/hide. |
| PlaySurface height (220pt target, currently 40% of content) | Layout | PARTIAL — 40% of content height (`surfaceH = contentH * 0.40f`) | REWIRE | [UI] | V1 spec: fixed 220pt. Not percentage-based. Recalculate `resized()` to use `min(220, contentH * 0.40f)` or simply 220pt fixed. |
| Auto-expand for drum engines | `eng->getEngineId()` — check if drum engine (ONSET, OFFERING) | Not implemented | BUILD | [UI] | On `loadSlot()` or engine hot-swap, check if engine has `engineType == "drums"`. If yes: auto-set `surfaceToggleBtn` state to true and call `showPlaySurface()`. |

---

### 5.2 Tab Bar (28pt)

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| XOUIJA tab | Internal PlaySurface tab state | EXISTS in PlaySurface.h (presumed — not read above) | EXISTS | [UI] | Dark background `#1A1A1A`. Space Grotesk SemiBold 10pt ALL CAPS. Active: `#F0EDE8` text, 2px XO Gold underline. |
| MPC PADS tab | Same | EXISTS | EXISTS | [UI] | |
| KEYS tab | Same | EXISTS | EXISTS | [UI] | |
| Surface switching (instant, preloaded) | Internal | EXISTS — visibility toggle, no rebuild | EXISTS | [UI] | Correct. Surfaces are preloaded at construction. |

---

### 5.3 XOuija Surface (888×236pt)

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Ocean depth background gradient | Static visual | EXISTS in PlaySurface.h | EXISTS | [UI] | Cached as `juce::Image` in `resized()`. Correct. |
| Scale fret lines | Internal scale state | EXISTS in PlaySurface.h | EXISTS | [UI] | |
| Parameter sensitivity map (64x64 texture, 8% opacity) | `PresetData` — precomputed per preset load | Not implemented | BUILD | [UI] | 64x64 `juce::Image`. Precomputed on preset load (`processor.applyPreset()` callback). White at 8% opacity where sensitivity is high. `drawImageWithin()` overlay. |
| Engine state visualization (LFO phase → surface displacement) | Lock-free FIFO from audio thread (LFO phase values) | Not implemented | BUILD | [FIFO] | Audio thread writes LFO phase values to lock-free FIFO. UI reads at 30Hz. Renders as sine-wave displacement on gradient. Default 5% opacity, adjustable. Toggleable off. |
| XOuija Planchette (80x60pt floating lens) | Note pitch (MIDI note), Hz, velocity, expression; `eng->getEngineId()`, `eng->getAccentColour()` | Not implemented | BUILD | [UI + FIFO] | `PlanchetteComponent : public juce::Component, private juce::Timer`. Lissajous idle drift: `a=0.3Hz, b=0.2Hz`. Lock-on to touch: 150ms cubic-bezier. Note name from FIFO. Engine accent border at 70%. Inner glow at 8% (15% when active). |
| Planchette: Idle drift (Lissajous) | Internal timer state | Not implemented | BUILD | [UI] | `x = cx + A*sin(0.3*t + PI/2)`, `y = cy + B*sin(0.2*t)`. Amplitude 15% of surface. Opacity 40% during drift. `juce::Timer` at 60Hz for smooth animation. |
| Planchette: Lock-on snap | Touch position | Not implemented | BUILD | [UI] | 150ms cubic-bezier(0.34, 1.56, 0.64, 1). Opacity jumps to 100%. Drift ceases immediately. |
| Planchette: Warm memory hold | 400ms hold after touch release | Not implemented | BUILD | [UI] | Note name fades to 50% over 400ms. Border to 40%. Resumes drift after. |
| Planchette: Bioluminescent trail | Touch x/y/age/velocity ring buffer | Not implemented | BUILD | [UI] | Ring buffer: 12 `TrailPoint {float x, y, age, velocity}`. `fillEllipse(x, y, r, r)` where `r = 3 + velocity * 4`. Color: engine accent at `alpha = 0.6 * (1.0 - age / 1.5)`. |
| Planchette: Ripple on touch | Note-on event | Not implemented | BUILD | [UI] | 3 concentric rings from touch point. Max radius 40pt. 300ms. Fixed pool of 4 `RippleState` structs. |
| Note-on MIDI generation | `MidiMessageCollector` | EXISTS — `setMidiCollector()` wired | EXISTS | [UI→MIDI] | PlaySurface generates MIDI via `processor.getMidiCollector()`. Correct. |

---

### 5.4 MPC 16-Pad Grid

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| 4x4 pad grid | MIDI note mapping — engine-specific | EXISTS in PlaySurface.h | EXISTS | [UI→MIDI] | |
| Pad idle state (depth-zone tinting) | Row position → zone color | EXISTS in PlaySurface.h | EXISTS | [UI] | Row 0=Midnight violet, Row 3=Sunlit cyan. |
| Pad hit state (engine accent fill at velocity opacity) | Velocity from `mouseDown` timing | EXISTS | EXISTS | [UI] | |
| Warm memory ghost ring (1.5s decay) | Timer-driven age tracking | EXISTS | EXISTS | [UI] | |
| Velocity heatmap mode | Toggle | EXISTS | EXISTS | [UI] | |
| Keyboard mappings (Q/W/E/R etc.) | Keyboard listener in PlaySurface | EXISTS | EXISTS | [UI→MIDI] | |
| DrumPadGrid shared component (see Section 9.1) | For ONSET (8 voices), OFFERING (8 voices × 5 cities) | Not yet factored as shared component — lives in PlaySurface only | BUILD | [UI] | When ONSET or OFFERING is active slot, MPC pads should reflect engine-specific voice mapping. |

---

### 5.5 Seaboard-Style Continuous Keyboard

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Continuous keyboard (2 octaves, scrollable) | MIDI note mapping | EXISTS in PlaySurface.h | EXISTS | [UI→MIDI] | |
| Natural/sharp keys (Seaboard style) | Static layout | EXISTS | EXISTS | [UI] | |
| Touch active (engine accent fill at pressure) | Pressure from touch input | EXISTS | EXISTS | [UI→MIDI] | |
| Pitch bend zones (left/right 15% per key) | Pitch Bend CC | EXISTS | EXISTS | [UI→MIDI] | |
| Glide trail (engine accent, 200ms fade) | Touch position history | EXISTS | EXISTS | [UI] | |
| Octave scroll strip | Internal octave offset state | EXISTS | EXISTS | [UI] | |
| MPE generation (per-note Pitch Bend, CC74, Channel Pressure) | `mpe_enabled`, `mpe_pitchBendRange`, `mpe_pressureTarget`, `mpe_slideTarget` | EXISTS — wired to processor MIDI collector | REWIRE | [UI→MIDI] | MPE params need to be read by PlaySurface when generating MIDI. Connect `mpe_*` APVTS params. |

---

### 5.6 Expression Controller Panel (172×236pt)

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Mod Wheel (28x160pt) | CC1 via `MidiMessageCollector` | EXISTS — `ToucheExpression.h` | REWIRE | [UI→MIDI] | V1: ridged texture (4 horizontal ridges). Fill rises from bottom: engine accent color. Spring-return off by default. |
| Pitch Bend (28x160pt, spring-return) | Pitch Bend message | EXISTS | REWIRE | [UI→MIDI] | Center line 1px. Current position: 8pt bright dot in engine accent. Spring physics: `pos += (0.5 - pos) * 0.15` per tick. |
| Macro Strip M1 (20pt wide, 120pt tall) | `macro1` — visual only (no second SliderAttachment) | Not implemented in expression panel | BUILD | [UI] | Visual-only meter reading `getRawParameterValue("macro1")->load()`. Click-drag sends `setValueNotifyingHost()`. Color `#E9C46A` XO Gold. |
| Macro Strip M2 | `macro2` | Not implemented | BUILD | [UI] | Color `#00FF41`. |
| Macro Strip M3 | `macro3` | Not implemented | BUILD | [UI] | Color `#BF40FF`. |
| Macro Strip M4 | `macro4` | Not implemented | BUILD | [UI] | Color `#00B4A0`. |
| Tide Controller (100pt diameter) | Assignable param — default filter cutoff | Not implemented | BUILD | [UI] | See Section 4.5 (C4). Same `TideController` component reused here. Assignable target via right-click context menu → "Assign to..." → shows param list. |

---

## 6. STATUS BAR (28pt)

### Current Implementation
No dedicated status bar component. No FIRE/XOSEND/ECHO/PANIC triggers. BPM, voice count, CPU not displayed. Engine slot indicators not present. The `MacroSection` and `MasterFXSection` occupy the bottom of the editor.

---

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| Status bar container (28pt, full width) | Layout | Not implemented | BUILD | [UI] | New component `StatusBar : public juce::Component`. Add to `XOceanusEditor::resized()` at bottom. Adjust other heights. |
| FIRE trigger pad (48x48pt, green, key Z) | `ChordMachine` or trigger system | Not implemented | BUILD | [UI] | `juce::TextButton`. `#4ADE80` fill. Click triggers preset-defined action (e.g., start chord machine, trigger one-shot). Spring physics: 2px depth shift 50ms, bounce back 150ms. Keyboard shortcut Z. |
| XOSEND trigger pad (key X) | Export / send system | Not implemented | BUILD | [UI] | `#F5C97A` fill. Trigger XPN send or DAW clip export. Key X. |
| ECHO CUT trigger pad (key C) | Delay feedback cut | Not implemented | BUILD | [UI] | `#F5C97A` fill. Cuts delay feedback to 0 (one-shot, spring return). Sets `master_delayFeedback` to 0 temporarily. Key C. |
| PANIC trigger pad (key V) | MIDI All Notes Off | Not implemented | BUILD | [UI] | `#EF4444` fill. ALWAYS visible, NEVER dimmed by Dark Cockpit. Sends MIDI All Notes Off to `MidiMessageCollector`. Key V. |
| BPM display | `juce::AudioPlayHead::PositionInfo::bpm` — from host; or internal ChordMachine BPM | Not implemented | BUILD | [ATOMIC] | Processor reads host BPM in `processBlock`, stores in `std::atomic<double>`. UI reads at 1Hz. JetBrains Mono 12pt. |
| Voice count ("Voices: 4/8") | Sum of `eng->getActiveVoiceCount()` across all slots | PARTIAL — `CompactEngineTile` shows per-tile voice dots but no total | BUILD | [ATOMIC] | Sum voice counts from all active engines. `std::atomic<int> totalVoiceCount` in processor updated per block. |
| CPU meter (JetBrains Mono 10pt) | `std::atomic<float> cpuPercent` in processor | Not implemented | BUILD | [ATOMIC] | See Section 1.5. Duplicate here for always-visible status. |
| Engine slot indicator dots (4 × 12pt circles) | Per-slot: `eng->getAccentColour()` if active, `#EAE8E4` if empty | PARTIAL — `CompactEngineTile` shows accent color; no standalone dot row | BUILD | [UI] | Click selects slot (calls `selectSlot()`). |
| Performance Lock indicator | Internal bool | Not implemented | BUILD | [UI] | Padlock icon when Performance Lock active. |

---

## 7. SPECIAL FEATURES

### 7.1 Sound on First Launch

| Feature | Data Source | Current Implementation | Status | Notes |
|---------|------------|----------------------|--------|-------|
| First launch detection | `juce::PropertiesFile` — `isFirstLaunch` key | Not implemented | BUILD | Check on `XOceanusEditor` construction. If first launch: load OXBOW "First Breath" preset into slot 1, trigger auto-play at velocity 0.4. |
| OXBOW "First Breath" auto-play | `processor.applyPreset(firstBreathPreset)` + `processor.getMidiCollector().addMessageToQueue(noteOn)` | Not implemented | BUILD | Preset must exist at `Presets/XOceanus/Foundation/OXBOW_First_Breath.xometa`. |
| Hover modulation (any knob moves 5%) | `juce::MouseListener` on root component — `mouseMove()` → `setValueNotifyingHost()` | Not implemented | BUILD | Add `FirstLaunchHoverListener` to editor. Active for 30s then self-disables. Subtle: knob shifts ±5% toward cursor direction. |
| Welcome toast ("Welcome to XOceanus. Touch anything.") | `juce::Timer::callAfterDelay(30000, ...)` | Not implemented | BUILD | Need toast notification system. See `Docs/design/toast-notification-system.md`. |
| Flag first launch complete | `juce::PropertiesFile` write | Not implemented | BUILD | Set `isFirstLaunch = false` after first note is played by user. |

---

### 7.2 Dark Cockpit (B041 — Ratified 8-0)

| Feature | Data Source | Current Implementation | Status | Notes |
|---------|------------|----------------------|--------|-------|
| Dark Cockpit opacity system (5 levels: 100/80/45/20/0%) | PlaySurface input state, coupling amounts, mouse position in UI regions | Not implemented | BUILD | `DarkCockpitManager` singleton or editor member. Tracks current focus zone. Applies alpha to component groups via `setAlpha()`. |
| Trigger: PlaySurface input → dim engine params | PlaySurface `mouseDown` callback | Not implemented | BUILD | 400ms ease-in to 20% alpha on `EngineDetailPanel` Level 2/3 params. Macros remain 100%. |
| Trigger: 3s idle → restore | `juce::Timer` | Not implemented | BUILD | 600ms ease-out restore to 100%. |
| Trigger: coupling > 0.1 → dim non-coupled engines | `cp_r{N}_amount` | Not implemented | BUILD | 300ms ease-in. Non-coupled engine controls → 20%. Coupled engine controls → 100%. |
| Never dim: Logo, PANIC, preset name, CPU | Hard-coded exceptions | Not implemented | BUILD | Maintain `juce::Component*` list of never-dim components. Skip in alpha sweep. |
| Performance mode visual (PlaySurface active) | PlaySurface state | Not implemented | BUILD | Column A: 45% alpha. Column B: 20% alpha. Column C: 20% alpha. Header macros: 80% alpha (still reactive). |

**Audit fix (ULF-P0-2):** Text labels on interactive controls must maintain minimum 35% alpha regardless of cockpit state. The 20% level applies only to background fills and decorative paint, never to text. Exception list expanded: Logo, PANIC, preset name, CPU, FIRE, XOSEND, ECHO CUT, and all interactive control labels.

---

### 7.3 XOuija Planchette (B042 — Ratified 7-1)

(Detailed in Section 5.3. Summary here for feature tracking.)

| Feature | Status | Notes |
|---------|--------|-------|
| Lissajous idle drift | BUILD | 0.3Hz/0.2Hz, amplitude 15% of surface, 40% opacity during drift |
| Spring lock-on | BUILD | 150ms cubic-bezier, drift ceases, opacity 100% |
| Bioluminescent velocity-scaled trail | BUILD | Ring buffer 12 points, engine accent color |
| Warm memory hold (400ms) | BUILD | Note fades, border fades, then resumes drift |
| Gesture trail as modulation source (B043, ratified 6-2) | BUILD (post-V1) | 256-tuple ring buffer promoted to replayable DSP modulation signal. Two performers' trails create interference-pattern modulation. Design spec required. |

---

### 7.4 Tide Controller

(Detailed in Section 4.5 and 5.6. Summary here.)

| Feature | Status | Notes |
|---------|--------|-------|
| 16x16 wave equation grid | BUILD | `h[x][y]` height field + `velocity[x][y]`. Propagation coefficient, damping 0.98. |
| Visual render (bilinear interpolated, 30fps) | BUILD | Filled rectangles with engine accent color. Peaks bright, troughs dark. |
| Tap → single ripple | BUILD | Single impulse at `h[8][8]`. |
| Drag → continuous wave | BUILD | Continuous height injection at drag point. |
| Output: `h[8][8]` normalized 0-1 → assigned param | BUILD | `getRawParameterValue()` read + `setValueNotifyingHost()` write. |
| iPad tilt via `CMMotionManager` | BUILD (iOS only) | `CoreMotion` framework. Biases height field with device orientation. |

---

### 7.5 A/B Preset Comparison

| Feature | Data Source | Current Implementation | Status | Notes |
|---------|------------|----------------------|--------|-------|
| Store preset A | `PresetData presetA` in editor | Not implemented | BUILD | Current preset when A/B mode is entered. |
| Store preset B | `PresetData presetB` in editor | Not implemented | BUILD | Last preset before A was loaded. |
| A toggle | Click A → `processor.applyPreset(presetA)` | Not implemented | BUILD | In header preset navigator. |
| B toggle | Click B → `processor.applyPreset(presetB)` | Not implemented | BUILD | In header preset navigator. |
| Visual indication (A vs B) | Button state | Not implemented | BUILD | A button: XO Gold fill when A is active. B button: silver fill. |

---

### 7.6 Performance Lock

| Feature | Data Source | Current Implementation | Status | Notes |
|---------|------------|----------------------|--------|-------|
| Performance Lock state | Internal bool — no APVTS | Not implemented | BUILD | When active: intercept all `setValueNotifyingHost()` calls and block (return without calling). Exceptions: PlaySurface MIDI, FIRE/XOSEND/ECHO/PANIC, macro knobs. |
| Lock icon in header + status bar | Lock state | Not implemented | BUILD | Padlock SVG icon (HeroIcons). When locked: all knobs show lock overlay at 40% opacity. |
| Escape to unlock | Key shortcut | Not implemented | BUILD | OR keyboard Escape (currently: Escape returns to overview — need to check priority). |

---

### 7.7 73 Colors Are 73 Languages (Engine Accent System)

| Feature | Data Source | Current Implementation | Status | Notes |
|---------|------------|----------------------|--------|-------|
| Engine accent color propagation (macros, coupling, PlaySurface, preset browser, knobs) | `eng->getAccentColour()` | PARTIAL — `EngineDetailPanel`, `CompactEngineTile`, `MacroHeroStrip`, `ParameterGrid` all use engine accent. PlaySurface and global macro knobs do NOT update accent. | REWIRE | When active engine changes: cascade accent color update to: header macro knobs (currently always XO Gold), PlaySurface trail color, coupling strip gold corridor, preset browser highlights. Requires `onAccentColorChanged(juce::Colour)` callback or observer pattern. |
| Macro knob arcs (currently XO Gold always) | Active engine accent | `MacroSection` sets `rotarySliderFillColourId` to XO Gold at construction | REWIRE | Change to engine accent. MacroSection needs `setAccentColor(juce::Colour)` method called when slot changes. |
| PlaySurface trail color | Active engine accent | EXISTS — `PlaySurface` presumably uses engine accent; verify in `PlaySurface.h` | REWIRE | Pass accent color to PlaySurface on engine change. |

---

### 7.8 Easter Eggs

| Feature | Current Implementation | Status | Notes |
|---------|----------------------|--------|-------|
| "73 Engines" display in header (when no coupling active) | EXISTS — header shows "N Engines · 14 Coupling Types · 18000+ Presets" when no coupling active | EXISTS | Correct. |
| FieldMapPanel (sonic cartography) | EXISTS — `FieldMapPanel` at line 1026. Note events → glowing dots on time×pitch canvas. 4-minute window. | EXISTS | Correct. Drained from `drainNoteEvents()` in timer callback. |
| MIDI Learn visual feedback (amber pulse → green badge) | EXISTS — `GalleryKnob::drawMidiLearnOverlay()` | EXISTS | Correct. |

---

## 8. OBRIX SPECIAL UI

OBRIX (`Source/Engines/Obrix/ObrixEngine.h`) is the V1 flagship. It requires a dedicated non-grid UI panel in place of (or supplementing) the standard `ParameterGrid`. 81 params total.

### 8.1 Brick Stack View

| UI Element | APVTS Param / Data Source | Current Implementation | Status | Thread Safety | Notes |
|-----------|--------------------------|----------------------|--------|---------------|-------|
| OBRIX panel detection | `eng->getEngineId() == "Obrix"` | Not implemented — `EngineDetailPanel::loadSlot()` uses generic `ParameterGrid` for all engines | BUILD | [UI] | Add engine-type check in `loadSlot()`. If OBRIX: swap `ParameterGrid` for `ObrixDetailPanel`. Pattern already supports this — `viewport.setViewedComponent(newGrid, true)` can set any Component. |
| Brick Stack View (visual signal flow) | `obrix_src1Type`, `obrix_src2Type`, `obrix_fxMode` | Not implemented | BUILD | [UI] | Custom `juce::Component`. Vertical stack of typed brick slots (Sine/Noise/FM/Sample etc.). Click brick → expand params below. Drag to reorder. |
| Per-Brick Type Selector | `obrix_src1Type`, `obrix_src2Type` (choice params) | Exists as APVTS choice param | BUILD (UI) | [UI] — ComboBoxAttachment | ComboBox per brick showing type list. `ComboBoxAttachment`. |
| Per-Brick Parameters (hierarchical reveal) | All `obrix_src1*`, `obrix_src2*` params | Exist as APVTS params | BUILD (UI) | [UI] — SliderAttachment | Expand/collapse per brick. Show relevant params for current type. |
| Reef Ecology Display | `obrix_reefResident`, `obrix_residentStrength`, `obrix_stressLevel_` (internal), `obrix_bleachLevel_` (internal) | Not implemented | BUILD | [ATOMIC or FIFO] | Shows current ecological state: Competition/Symbiosis/Parasitism/Stress/Bleach. `stressLevel_` and `bleachLevel_` are internal DSP accumulators — need lock-free FIFO or atomic reads for UI. State visualization: color-coded indicator (healthy=Reef Jade, stressed=amber, bleached=white). |
| Wave System Indicator (which Waves 1-5 active) | OBRIX param groups detect which wave features are enabled | Not implemented | BUILD | [UI] | 5 small indicators (dots or numbered chips). Wave 1-5 label shows which feature set is active based on param values. |
| Harmonic Field strength indicator | `obrix_fieldStrength`, `obrix_fieldPolarity`, `obrix_fieldPrimeLimit` | Not implemented | BUILD | [UI] — SliderAttachment | Knob + polarity toggle. JI ratio display showing current convergence target (derived from DSP output — needs FIFO). |
| Environmental params display | `obrix_envTemp`, `obrix_envPressure`, `obrix_envCurrent`, `obrix_envTurbidity` | Not implemented | BUILD | [UI] — SliderAttachment | 4 knobs in a grouped "Environment" panel within OBRIX UI. |
| FX Mode toggle (Serial/Parallel) | `obrix_fxMode` | Not implemented in UI | BUILD | [UI] — ButtonAttachment | Toggle button: SERIAL / PARALLEL. `ButtonAttachment("obrix_fxMode")`. |
| Competition/Symbiosis strength sliders | `obrix_competitionStrength`, `obrix_symbiosisStrength` | Not implemented in UI | BUILD | [UI] — SliderAttachment | Two sliders in Reef Ecology section. |
| Reef Resident selector | `obrix_reefResident` (choice: Off/Competitor/Symbiote/Parasite) | Not implemented in UI | BUILD | [UI] — ComboBoxAttachment | ComboBox with 4 items. |
| Reef Resident strength | `obrix_residentStrength` | Not implemented in UI | BUILD | [UI] — SliderAttachment | Knob, default 0.3. |
| State Reset button | `obrix_stateReset` (trigger param) | Not implemented | BUILD | [UI] | TextButton → `setValueNotifyingHost(1.0f)` then `setValueNotifyingHost(0.0f)` (momentary). Clears `stressLevel_` and `bleachLevel_`. |

---

## 9. SHARED COMPONENT BUILD LIST

These components appear in multiple engines' detail views and should be built once, shared everywhere.

---

### 9.1 DrumPadGrid — V1-REQUIRED

**What it is:** 8-voice or 16-voice pad grid embedded in the Engine Detail Panel for percussion engines. Different from PlaySurface MPC grid (which is a performance surface). Engine Detail DrumPadGrid shows parameter-per-pad layout (tune, decay, level per voice).

**Engines that need it:** ONSET (8 voices), OFFERING (8 voices × 5 city chains)

**Estimated complexity:** Medium (2-3 days)

**Implementation notes:**
- `DrumPadGrid : public juce::Component`
- `numVoices` (8 or 16) configurable
- Per-pad: mini knobs for Tune, Decay, Level (or engine-specific params)
- Param prefix: `{prefix}_voice{N}_tune`, `{prefix}_voice{N}_decay`, `{prefix}_voice{N}_level`
- ONSET: `perc_voice{N}_*`. OFFERING: `ofr_voice{N}_*`
- Velocity heatmap per pad — `std::atomic<float> lastVelocity[8]` read from FIFO
- For OFFERING: city selector tabs (NY/Detroit/LA/Toronto/Bay Area) above grid. Each city = different processing chain visualization.

---

### 9.2 NamedModeSelector — V1-REQUIRED

**What it is:** A custom selector for engines where mode has a meaningful name (biome, ERA vertex, maqam, knot type, etc.). Visually richer than a ComboBox — shows icon or gradient swatch alongside name.

**Engines that need it:** OCELOT (biomes), OVERWORLD (ERA triangle — see also TriangleXYPad), ORACLE (maqam + stochastic modes), ORBWEAVE (knot types: Trefoil/Figure-Eight/Torus/Solomon), OUROBOROS (topology), OCEAN-DEEP, OCTOPUS, ORGANISM (rule sets), OSTINATO (seat ring), OHM, many others (~20 engines)

**Estimated complexity:** Low-Medium (1-2 days)

**Implementation notes:**
- `NamedModeSelector : public juce::Component`
- `juce::ComboBox` with custom `LookAndFeel` rendering icon + color swatch per item
- APVTS: `ComboBoxAttachment`
- Swatch color derived from engine accent color or mode-specific color
- Engine vocabulary overrides via `EngineVocabulary::labelFor()`

---

### 9.3 AccumulationMeter — POST-V1

**What it is:** A visual meter showing an accumulated (leaky integrator) value — used for engines with "accumulation" mechanics where parameter values build up over time.

**Engines that need it:** OVERWORN, OVERFLOW, OVERCAST, OMBRE, ORGANON

**Estimated complexity:** Low (0.5 days)

**Implementation notes:**
- `AccumulationMeter : public juce::Component, private juce::Timer`
- Reads accumulator value via lock-free FIFO or `std::atomic<float>`
- Vertical bar, engine accent color fill, decay rate visualized as opacity trail
- 30Hz update. Height = accumulator value (0-1).

---

### 9.4 SpectralFingerprintIndicator — POST-V1

**What it is:** Displays the Spectral Fingerprint Cache — a visual "sonic identity" card for FUSION quad engines. Shows spectral content as a fingerprint-style radial visualization.

**Engines that need it:** OASIS, ODDFELLOW, ONKOLO, OPCODE (the 4 FUSION engines)

**Estimated complexity:** Medium (2 days)

**Implementation notes:**
- `SpectralFingerprintIndicator : public juce::Component, private juce::Timer`
- Reads from Spectral Fingerprint Cache (lock-free FIFO or shared atomic array)
- Radial visualization: 32 frequency bands as spokes from center
- Engine accent color radial gradient
- 10Hz update rate

---

### 9.5 AnalysisEngineMeter — V1-REQUIRED (OPTIC only)

**What it is:** Specialized display for analysis-type engines that expose spectrum + multiple outputs.

**Engines that need it:** OPTIC (spectrum + 8 modulation outputs), OSMOSIS (4-band analyzer + pitch detect)

**Estimated complexity:** Medium-High (3 days)

**Implementation notes:**
- OPTIC: Winamp-style audio-reactive visualizer (`OpticVisualizer.h` ALREADY EXISTS in `Source/UI/OpticVisualizer/`). **REWIRE** — embed in Engine Detail Panel when OPTIC is active slot.
- OSMOSIS: 4-band meter + pitch readout. Custom component. Reads from `osmo_*` APVTS params + pitch FIFO.

---

### 9.6 TopologySelector — POST-V1

**What it is:** Visual selector for topological structures (knot types, network topologies).

**Engines that need it:** OVERLAP, ORBWEAVE, OUROBOROS

**Estimated complexity:** Medium (1.5 days)

**Implementation notes:**
- `TopologySelector : public juce::Component`
- Grid of topology icons (small SVG-like paths for Trefoil, Figure-Eight, Torus, Solomon links, Borromean rings)
- Click selects. Selected item: engine accent border + fill at 20%
- APVTS: `ComboBoxAttachment` to topology choice param

---

### 9.7 TemporalDriftIndicator — POST-V1

**What it is:** Visual indicator showing temporal drift state — how far the engine has drifted from its nominal timing.

**Engines that need it:** ORCHARD, OVERGROW, OLATE, OMEGA, OAKEN, OVERWORN, OVERCAST

**Estimated complexity:** Low (0.5 days)

**Implementation notes:**
- `TemporalDriftIndicator : public juce::Component, private juce::Timer`
- Animated line/wave drifting from center
- Amplitude = drift amount (from lock-free FIFO)
- Engine accent color stroke, 1.5px

---

### 9.8 TriangleXYPad — V1-REQUIRED (OVERWORLD)

**What it is:** 2D XY pad shaped as an equilateral triangle. Position = blend of 3 timbral dimensions.

**Engines that need it:** OVERWORLD (ERA triangle: Buchla/Schulze/Vangelis corners), OXYTOCIN (bond triangle)

**Estimated complexity:** Medium (2 days)

**Implementation notes:**
- `TriangleXYPad : public juce::Component`
- Equilateral triangle drawn via `juce::Path`
- Touch position = barycentric coordinates → 3 float outputs (summing to 1.0)
- Vertex labels from engine vocabulary
- APVTS: 3 params — `{prefix}_eraA`, `{prefix}_eraB`, `{prefix}_eraC` (or similar). Use `setValueNotifyingHost()` in drag handler.
- OVERWORLD: `ow_eraBuchla`, `ow_eraSchulze`, `ow_eraVangelis`
- OXYTOCIN: bond triangle axes per `oxy_*` params

---

### 9.9 BipolarAxisBar — V1-REQUIRED

**What it is:** A horizontal bar with center = 0, left = negative, right = positive. For engines with bipolar interaction axes.

**Engines that need it:** OBESE (Analog/Digital Mojo axis), OUIE (STRIFE/LOVE HAMMER axis), OWARE (material continuum)

**Estimated complexity:** Low (0.5 days)

**Implementation notes:**
- `BipolarAxisBar : public juce::Component`
- `juce::Slider` with `LinearHorizontal` style and custom `LookAndFeel`
- Center tick at 0. Fill from center to current position (engine accent if positive, complement if negative).
- Labels on left/right endpoints (from `EngineVocabulary`)
- APVTS: `SliderAttachment` to bipolar (-1 to 1) param

---

### 9.10 ConductorArcDisplay — V1-REQUIRED

**What it is:** Visualization of OPERA's autonomous dramatic arc — shows current arc shape (4 options), progress position, conductor state.

**Engine:** OPERA only

**Estimated complexity:** Medium-High (2 days)

**Implementation notes:**
- `ConductorArcDisplay : public juce::Component, private juce::Timer`
- Draws current arc shape (rising/falling/arch/valley) as a `juce::Path`
- Current position marker: vertical line at `conductorTime / arcDuration`
- Arc color: Aria Gold `#D4AF37`
- Mode display: Conductor/Manual/Both/Sync — from `opera_conductorMode` APVTS
- 10Hz update. Arc shape from `opera_arcShape` param.

---

### 9.11 CellularAutomataGrid — POST-V1

**What it is:** Live view of the cellular automaton state for emergent engines.

**Engines that need it:** ORGANISM (Coral Colony, Conway-like rules), OUTWIT (chromatophore CA)

**Estimated complexity:** High (3 days)

**Implementation notes:**
- `CellularAutomataGrid : public juce::Component, private juce::Timer`
- Grid display: 16x16 or 32x32 cells
- Cell state from lock-free FIFO (audio thread computes CA)
- Engine accent color for "alive" cells, panel background for "dead"
- Click to seed/toggle cells (sends via FIFO to audio thread — inverse direction)
- 10Hz update

---

### 9.12 DrawbarFaderBank — POST-V1

**What it is:** Hammond-style drawbar faders for additive organ engines.

**Engine:** OTIS

**Estimated complexity:** Low (1 day)

**Implementation notes:**
- `DrawbarFaderBank : public juce::Component`
- 9 vertical faders (Hammond drawbar convention: 16', 8', 5⅓', 4', 2⅔', 2', 1⅗', 1⅓', 1')
- `SliderAttachment` to `otis_drawbar{N}` params (9 params)
- Traditional brown/white coloring per Hammond spec. Gospel Gold accent on active.

---

### 9.13 SeatRingSequencer — POST-V1

**What it is:** Circular sequencer visualization for OSTINATO's seat-ring pattern system.

**Engine:** OSTINATO

**Estimated complexity:** High (3 days)

**Implementation notes:**
- `SeatRingSequencer : public juce::Component, private juce::Timer`
- Circle with 16 seats (step positions). Each seat = `osti_seat{N}_active`, `osti_seat{N}_note`, `osti_seat{N}_velocity`
- Current step highlighted (engine accent glow). Playhead rotates clockwise.
- Click seat to toggle active state.
- Inner label: note name. Outer ring: velocity arc.
- 15Hz update.

---

### 9.14 CrossFeedMatrix — POST-V1

**What it is:** 4x4 matrix display showing cross-feed routing between OCELOT's biome oscillators.

**Engine:** OCELOT

**Estimated complexity:** Medium (1.5 days)

**Implementation notes:**
- `CrossFeedMatrix : public juce::Component`
- 4x4 grid of knobs. Row = source, Col = destination.
- `SliderAttachment` to `ocelot_xfeed{src}{dst}` params
- Diagonal = self-feedback. Off-diagonal = cross-biome feed.

---

### 9.15 FiveMacroDisplay (OVERBITE) — V1-REQUIRED

**What it is:** Special macro display for OVERBITE's 5-macro system (BELLY/BITE/SCURRY/TRASH/PLAY DEAD).

**Engine:** OVERBITE

**Estimated complexity:** Low (0.5 days)

**Implementation notes:**
- Uses existing `MacroHeroStrip` pattern but extended to 5 macros.
- `poss_macroBelly`, `poss_macroBite`, `poss_macroScurry`, `poss_macroTrash`, `poss_macroPlayDead`
- `MacroHeroStrip::loadEngine()` caps at 4 macros — extend to support 5-macro override for OVERBITE.

---

## 10. STRUCTURAL MIGRATION PLAN

### What Moves Where

| Current Location | Current Component | Move To | Action |
|-----------------|------------------|---------|--------|
| Bottom strip, 105px | `MacroSection` (M1-M4 + Master Vol knobs) | Header (M1-M4 + Master Vol) | **Q2 RESOLVED (A):** Full move to header. Restructure MacroSection — M1-M4 knobs + Master Vol knob all relocate to header right side. Retain MIDI Learn via `MacroSection::setupMidiLearn()`. Bottom strip MacroSection is removed entirely. |
| Bottom strip, 68px | `MasterFXSection` (6 FX sections) | Stay as compact strip; ALSO build B3 FX Chain Panel as full view | REWIRE — keep strip for always-visible access; add B3 panel for editing |
| Column B (full area) | `PerformanceViewPanel` | C2 Coupling Tab | REWIRE — move coupling inspector to Column C. Keep `PerformanceViewPanel` for coupling visualization. |
| Header (right), 220px | `PresetBrowserStrip` | Header preset navigator + C1 Preset Tab | REWIRE — header retains prev/name/next/heart/DNA. Full browser moves to C1. |
| Column B (conditionally) | `ChordMachinePanel` | Stay in Column B (B4) | STAYS — correct location per V1 |
| Full editor overlay | `CouplingArcOverlay` | Stay as overlay | STAYS — correct |
| Column B (110px strip) | `FieldMapPanel` | Column B — reduced height strip below panel stack | **Q3 RESOLVED (A):** Keep FieldMapPanel in Column B below the panel stack at reduced height (~60-70px instead of 110px). This frees ~40-50px for the Status Bar without removing the sonic cartography feature. Update `resized()` to allocate 65px to FieldMapPanel and 28px to StatusBar. |

### New Components to Add to `XOceanusEditor`

```
// New members to add:
SidebarPanel          columnC;          // new — tabbed C1-C5
StatusBar             statusBar;        // new — 28pt bottom strip
SignalFlowDiagram     signalFlow;       // new — 520×32pt strip in Column B
PlanchetteComponent   planchette;       // new — XOuija overlay
TideController        tideCtrl;         // new — expression panel
ObrixDetailPanel      obrixDetail;      // new — OBRIX special UI
```

### New `resized()` layout (target)

```
Header:          64pt   (was 52pt — Audit fix ULF-P0 + XAVIER-P0)
Body:            remaining height - 28pt (status)
  Column A:      260pt  (Engine Rack — 4 tiles + mini coupling graph + add button)
  Column B:      520pt  (Main Canvas — panel stack + signal flow strip)
  Column C:      320pt  (Sidebar — tabbed C1-C5)
PlaySurface:     220pt  (collapsible, below body when expanded)
Status Bar:      28pt   (FIRE/XOSEND/ECHO/PANIC + BPM + Voices + CPU + slot dots)
```

**Critical:** Total column widths must equal window width: 260 + 520 + 320 = 1100 (default). At 960px minimum: 240 + 440 + 280. Proportional scaling.

---

## 11. THREAD SAFETY MATRIX

| Data Type | Producer | Consumer | Mechanism | Notes |
|-----------|----------|----------|-----------|-------|
| APVTS parameter values (macros, coupling routes, FX) | UI thread (via SliderAttachment / `setValueNotifyingHost`) | Audio thread (via `getRawParameterValue()->load()`) | JUCE APVTS internal atomics | Safe. Never write from audio thread. |
| Engine voice count | Audio thread (`processBlock`) | UI thread (tile timer, status bar) | `std::atomic<int>` per slot | `eng->getActiveVoiceCount()` — ensure it returns atomic load |
| CPU usage | Audio thread | UI thread | `std::atomic<float>` in processor | Must be added. Update once per `processBlock`. |
| BPM from host | Audio thread (`processBlock` via `getPlayHead()`) | UI thread (status bar) | `std::atomic<double>` in processor | Must be added. |
| Waveform data (oscilloscope) | Audio thread (per-block sample copy) | UI thread (30Hz repaint) | `juce::AbstractFifo` + `std::array<float, 256>` per slot | Must be built. Never allocate in audio thread. |
| Note-on events (FieldMapPanel, MIDI indicator) | Audio thread (`processBlock`) | UI thread (1Hz drain) | Lock-free FIFO (`XOceanusProcessor::NoteMapEvent`) | EXISTS — `drainNoteEvents()` called in timer. Add MIDI indicator flash. |
| Coupling energy / RMS (arcs, corridor pulse) | Audio thread | UI thread | Lock-free FIFO (one float per route per frame) | Must be built. Used by Living Gold Corridor and arc thickness animation. |
| LFO phase values (PlaySurface engine state viz) | Audio thread | UI thread | Lock-free FIFO (one float per slot per block) | Must be built. |
| Cellular automaton state (ORGANISM, OUTWIT) | Audio thread | UI thread | Lock-free FIFO (grid snapshot) | Must be built per engine. |
| Spectral fingerprint (FUSION engines) | Audio thread (FFT results) | UI thread | Lock-free FIFO (32-band array) | Must be built. |
| Stress/bleach levels (OBRIX ecology) | Audio thread (leaky integrators) | UI thread | `std::atomic<float>` pair | Must be exposed from OBRIX engine. |
| MIDI Learn pending CC capture | Audio thread (processMidi) | UI thread (`checkPendingLearn`) | `std::atomic<int>` (CC number) | EXISTS — correct. |
| Engine hot-swap crossfade | Audio thread | UI thread (loading indicator) | `std::atomic<bool>` isLoading | EXISTS — `CompactEngineTile::isLoading` flag, partial. |
| Gesture trail as modulation (B043) | UI thread (Planchette) | Audio thread (modulation target) | Lock-free FIFO (256-tuple ring) | Must be built. UI→Audio direction (unusual — ensure no blocking). |

**Rules:**
1. NEVER call allocation functions on audio thread (`new`, `delete`, `std::vector::push_back` unless pre-allocated)
2. NEVER call JUCE UI functions from audio thread (`repaint()`, `setVisible()`, `setBounds()`)
3. NEVER use `std::mutex` on audio thread — use atomics or `juce::AbstractFifo`
4. ALL `getRawParameterValue()` reads return atomic — safe from any thread
5. `processor.getEngine(slot)` — ensure this is message-thread safe (engine pointer stability during hot-swap)

---

## 12. DECISIONS LOG

All questions resolved. Implementation may proceed.

| # | Question | **Decision** | Impact |
|---|----------|-------------|--------|
| Q1 | **Depth-Zone Dial scope**: Does the dial in the header select which *engine* to load into the *active slot*, or does it select which *slot* is active? | **RESOLVED: (A)** — Dial = engine picker for active slot. Tiles remain slot selectors. The two controls are complementary — tiles select which slot is active, dial browses engines within that slot. | Column A tile interaction is unchanged. Tile clicks are primary slot selectors. Dial is a secondary engine-browser scoped to the active slot. |
| Q2 | **Macro knobs location**: V1 spec puts M1-M4 in header. Current code has them in bottom strip with MIDI Learn. Move to header means MacroSection needs full restructuring. | **RESOLVED: (A)** — Full move of MacroSection to header. Restructure MacroSection so all knobs (M1-M4 + Master Vol) live in header. Bottom strip MacroSection is removed entirely. MIDI Learn rewired at new header attachment points. | MacroSection class restructured. `MacroSection::setupMidiLearn()` retained. Header `resized()` updated to accommodate 44x44pt knobs + 8pt spacing (Audit fix XAVIER-P0). |
| Q3 | **FieldMapPanel fate**: Currently 110px strip in Column B below panel stack. V1 Status Bar is 28pt. FieldMapPanel + Status Bar = 138px of bottom real estate. | **RESOLVED: (A)** — Keep FieldMapPanel in Column B at reduced height (~60-70px instead of 110px). Status Bar gets its 28pt below. `resized()` allocates 65px to FieldMapPanel and 28px to StatusBar. Sonic cartography feature preserved. | ~45px savings vs. current. Update `FieldMapPanel` paint/layout for reduced height. |
| Q4 | **Column C width vs Column B**: Should Column C be collapsible? | **RESOLVED: (B)** — Column C collapsible to icon strip when toggled. A toggle button collapses Column C to ~40pt icon strip (showing tab icons only), expanding Column B to fill the reclaimed space. | `resized()` has two layout modes. Toggle button in header or Column C tab bar. Column B width: 520pt (expanded) or ~800pt (C collapsed). |
| Q5 | **OBRIX dedicated panel vs augmented grid**: OBRIX has 81 params. Full custom `ObrixDetailPanel` or augmented `ParameterGrid`? | **RESOLVED: (A)** — Full custom `ObrixDetailPanel` for OBRIX flagship. Brick Stack View, Reef Ecology Display, Harmonic Field indicator, Environmental params panel. Supports B016 brick independence correctly. | `ObrixDetailPanel` class built from scratch. `EngineDetailPanel::loadSlot()` checks `eng->getEngineId() == "Obrix"` and swaps component accordingly. |
| Q6 | **FX location strategy**: Three FX entry points — intentional? | **RESOLVED: (A)** — Three FX entry points coexist: bottom strip `MasterFXSection` (compact always-visible summary), B3 panel (full edit view), C3 inspector (deep params + reorder). Layered progressive disclosure matches design philosophy. | All three views reference same APVTS params. No duplication of attachments — C3 and B3 use visual-only meters where bottom strip holds the `SliderAttachment`s, or each section is scoped to non-overlapping params. |
| Q7 | **Performance View toggle button (`perfToggleBtn`)**: Remove or repurpose after coupling moves to C2? | **RESOLVED: (B)** — Keep `perfToggleBtn` as "Cinematic Mode" trigger. Cinematic Mode: Column A + Column C collapse, Column B expands to full 1100pt coupling visualization. Button label updated to reflect new purpose. | `perfToggleBtn` renamed / relabeled. Cinematic Mode `resized()` path added. `PerformanceViewPanel` remains in Column B for Cinematic Mode; coupling inspector in C2 for normal mode. |
| Q8 | **A/B preset comparison data persistence**: Session-only or persisted in plugin state? | **RESOLVED: (B)** — A/B preset slots persisted in plugin state via `setStateInformation`/`getStateInformation`. Preferred for professional use. `PresetData presetA, presetB` serialized to/from plugin state XML. | `XOceanusProcessor::getStateInformation()` updated to include `presetA` and `presetB` blobs. Increases state size by ~2× preset size. |
| Q9 | **TriangularCoupling (#15) in UI**: Currently 14 types shown in coupling type dropdown. | **RESOLVED: (A)** — TriangularCoupling added as 15th item in the coupling type dropdown (`cp_r{N}_type` ComboBox). Accessible for any route, not OXYTOCIN-exclusive. OXYTOCIN seance (B040) identifies it as OXYTOCIN's defining coupling type — the dropdown makes it discoverable to all. | `RouteSection::typeBox` updated to 15 items. `couplingTypeLabel()` and `CLAUDE.md` coupling type list updated to include TriangularCoupling. |
| Q10 | **Master volume knob relocation**: Moves with macros out of bottom strip. New home? | **RESOLVED: (A)** — Master volume knob placed in header right edge, after the M4 macro knob. Part of the header utility cluster alongside macro knobs. `SliderAttachment("masterVolume")` in header. | `MacroSection` restructured to include master vol in the header layout. `resized()` adds ~44pt for master vol knob after M4. |

---

## 13. QUANTUM AUDIT — P0 RESOLUTION LOG

> 10 panels reviewed. 87 findings total (22 P0, 38 P1, 27 P2). This section tracks P0 resolution status.

### Structural P0s (must resolve before any layout code)

| ID | Finding | Resolution | Status |
|----|---------|------------|--------|
| BS-P0-1 | XOceanusEditor.h (4,323 lines) must be split before adding V1 components | Extract 9 classes into separate files. See Prep Sprint Step 1. | PENDING |
| BS-P0-2 | resized() proportional 3-column math undefined | Write ColumnLayoutManager (80-line struct). See Prep Sprint Step 6. | PENDING |
| GC-P0-1 | crossfadeMutex on audio thread — std::scoped_lock in processBlock | Replace with lock-free command queue or atomic CAS. | PENDING |
| GC-P0-2 | 11 engines return non-atomic getActiveVoiceCount() | Add std::atomic<int> to SynthEngine base class. | PENDING |
| LUCY-P0-1 | 3-column layout cannot use raw resized() | Write ColumnLayoutManager struct. | PENDING |
| LUCY-P0-2 | Frosted glass Planchette requires Melatonin Blur | Add sudara/melatonin_blur via FetchContent in CMakeLists.txt. | PENDING (post-V1 for Planchette) |
| LUCY-P0-3 | Viewport cannot do sticky header + sticky footer | Place MacroHeroStrip and bottomSticky outside Viewport as siblings. Pattern documented. | RESOLVED (design pattern identified) |
| LUCY-P0-4 | CouplingArcOverlay assumes 2-column geometry | Query live tile positions at paint time. | PENDING |

### Ergonomic P0s (must resolve before UI is usable)

| ID | Finding | Resolution | Status |
|----|---------|------------|--------|
| ULF-P0 | Header 52pt too short | Changed to 64pt. | RESOLVED |
| XAVIER-P0 | 36pt touch targets fail Apple HIG 44pt | Changed to 44pt knobs. | RESOLVED |
| A-01 | Inactive tab text fails WCAG 1.4.3 | Use rgba(80,76,70,0.75) for inactive tabs. | RESOLVED |
| CBG-P0-02 | 960×600 + PlaySurface = 83pt viewport | Auto-collapse PlaySurface when bodyHeight < 320pt. | PENDING |
| KAI-P0-01 | MPC 800×480 viewport breaks layout | Define 2-column fallback in ColumnLayoutManager. | PENDING |

### Pipeline P0s (must resolve to avoid data loss/corruption)

| ID | Finding | Resolution | Status |
|----|---------|------------|--------|
| HX-P0-01 | slot_enabled params not in createParameters() | Add as AudioParameterBool at instantiation. | PENDING |
| HX-P0-02 | MIDILearnManager not serialized | Add toValueTree/fromValueTree to state save/restore. | PENDING |
| HX-P0-03 | Missing beginChangeGesture/endChangeGesture | Add to all custom drag handlers. | PENDING |
| KAI-P0-02 | ExportDialog migration creates dual pipeline | Rewire header exportBtn to trigger showExportPanel(). | PENDING |
| KAI-P0-03 | DrumPadGrid must match MPC pad MIDI note map | Voice 0 = MIDI note 37 (MPC pad A01). | PENDING |

### V1-REQUIRED vs POST-V1 Component Tags

| Component | V1 Status | Engines |
|-----------|-----------|---------|
| DrumPadGrid | V1-REQUIRED | ONSET, OFFERING |
| NamedModeSelector | V1-REQUIRED | ~20 engines including V1 curated |
| AccumulationMeter | POST-V1 | OVERWORN, OVERFLOW, OVERCAST, OMBRE, ORGANON |
| SpectralFingerprintIndicator | POST-V1 | OASIS, ODDFELLOW, ONKOLO, OPCODE (Fusion quad) |
| AnalysisEngineMeter | V1-REQUIRED (OPTIC only) | OPTIC, OSMOSIS |
| TopologySelector | POST-V1 | OVERLAP, ORBWEAVE, OUROBOROS |
| TemporalDriftIndicator | POST-V1 | ORCHARD, OVERGROW, OLATE, OMEGA, OAKEN, OVERWORN, OVERCAST |
| TriangleXYPad | V1-REQUIRED (OVERWORLD) | OVERWORLD, OXYTOCIN |
| BipolarAxisBar | V1-REQUIRED | OBESE, OUIE, OWARE |
| ConductorArcDisplay | V1-REQUIRED | OPERA |
| CellularAutomataGrid | POST-V1 | ORGANISM, OUTWIT |
| DrawbarFaderBank | POST-V1 | OTIS (Kitchen Collection) |
| ModularSlotFlow | V1-REQUIRED | OBRIX (flagship) |
| SeatRingSequencer | POST-V1 | OSTINATO |
| CrossFeedMatrix | POST-V1 | OCELOT |
| FiveMacroDisplay | V1-REQUIRED | OVERBITE |
| DualVoiceAlgoPanel | POST-V1 | OUIE |
| 5th Slot (Ghost Slot) | V1-REQUIRED | All collections (Kitchen Fusion first) |
| CouplingChainView | V1-REQUIRED | All engines (OverviewPanel enhancement) |

---

## 8B. 5TH SLOT (Ghost Slot Architecture)

### Unlock Mechanic
When all 4 engine slots contain engines from the same collection (e.g., Kitchen quad: OVEN, OCHRE, OBELISK, OPALINE), a 5th slot materializes with a collection-specific animation.

### Implementation Requirements

| Element | Data Source | Status | Thread Safety | Notes |
|---------|-----------|--------|---------------|-------|
| `MaxSlots` constant change (4 → 5) | `EngineRegistry.h`, `MegaCouplingMatrix.h`, `XOceanusProcessor.h` | BUILD | [UI + AUDIO] | Compile-time constant. All slot arrays, FIFOs, coupling matrices expand automatically. |
| Collection detection | `EngineRegistry` — check if slots 0-3 contain engines from same collection | BUILD | [UI] | New method: `EngineRegistry::detectCollection(engineIds[4])` returns collection name or empty. |
| 5th slot CompactEngineTile | Conditional visibility based on collection detection | BUILD | [UI] | Hidden by default (`setVisible(false)`). Materializes with collection-specific animation on detection. |
| Ghost Slot coupling | Spectral Fingerprint Cache — metadata coupling, NOT audio routing | BUILD | [UI] | MegaCouplingMatrix stays 4×4 for audio. 5th slot uses fingerprint-based modulation only. |
| Collection-specific unlock animation | Per-collection animation spec | BUILD | [UI] | Kitchen: 500ms XO Gold shimmer. Creature: 1000ms rise-from-below. Each collection unique. |
| WaveformFifo for slot 5 | Already handled — `MaxSlots` change expands `waveformFifos` array | EXISTS | [FIFO] | Array-based; automatic. |
| Removal animation | Any Kitchen engine removed → 5th slot fades out (100ms) | BUILD | [UI] | Graceful teardown: fade opacity, then `setVisible(false)`. |

### CPU Budget
- Ghost Slot uses shared voice budget: 32 voices across 5 engines
- 5th slot engine ≤ 10% CPU
- Spectral Fingerprint Cache eliminates need for 4 additional audio coupling streams

---

## 8C. COUPLING CHAIN VIEW (OverviewPanel Enhancement)

### Purpose
Visual signal-flow representation showing active coupling routes as a readable chain: `[ENGINE A] → [coupling type] → [ENGINE B] → [master FX] → [OUT]`

| Element | Data Source | Status | Thread Safety | Notes |
|---------|-----------|--------|---------------|-------|
| Chain diagram component | `MegaCouplingMatrix::getRoutes()` + engine IDs | BUILD | [UI] | Horizontal strip (520×48pt) below the coupling node graph in OverviewPanel. Shows audio path left-to-right. |
| Engine boxes | Per-slot accent color + engine name | BUILD | [UI] | Rounded rect, accent color border, 8pt engine name. Active: 100% opacity. |
| Coupling type arrows | Route type → arrow label | BUILD | [UI] | Thin arrow with coupling type abbreviated label (reuse `couplingTypeLabel()`). |
| OBRIX insert indicator | When OBRIX receives coupling input | BUILD | [UI] | Special icon showing "programmable processor" role. |
| Master FX → OUT | Static | BUILD | [UI] | Final arrow to speaker icon. |

---

*End of XOceanus UI Wiring Manifest — Phase 1*
*Built from: `Source/UI/XOceanusEditor.h` (4,323 lines), `Docs/design/xoceanus-spatial-architecture-v1.md`, `Docs/design/xoceanus-definitive-ui-spec.md`, CLAUDE.md, and all 76 engine APVTS parameter specifications.*
