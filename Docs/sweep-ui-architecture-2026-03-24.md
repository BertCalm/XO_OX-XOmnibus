# XOlokun UI Architecture Sweep
**Date:** 2026-03-24
**Purpose:** "What exists" baseline for the Rebirth implementation.
**Source file audited:** `Source/UI/XOlokunEditor.h` (3,936 lines)
**Total UI source:** 11,649 lines across 19 header files in `Source/UI/`

---

## Part 1: Component Hierarchy

### Classes Defined in XOlokunEditor.h

All classes live inside the `namespace xolokun {}` block.

| Class | Type | Lines (approx) |
|-------|------|----------------|
| `GalleryLookAndFeel` | `juce::LookAndFeel_V4` subclass | 141 |
| `ParameterGrid` | `juce::Component` | 102 |
| `MacroHeroStrip` | `juce::Component` | 152 |
| `EngineDetailPanel` | `juce::Component` | 121 |
| `FieldMapPanel` | `juce::Component + juce::Timer` | 117 |
| `OverviewPanel` | `juce::Component` | 277 |
| `CompactEngineTile` | `juce::Component + juce::SettableTooltipClient + juce::Timer` | 326 |
| `MacroSection` | `juce::Component` | 108 |
| `AdvancedFXPanel` | `juce::Component` | 65 |
| `MasterFXSection` | `juce::Component` | 272 |
| `PresetBrowserPanel` | `juce::Component + juce::ListBoxModel` | 219 |
| `PresetBrowserStrip` | `juce::Component + PresetManager::Listener` | 135 |
| `ChordMachinePanel` | `juce::Component + juce::Timer` | 299 |
| `PerformanceViewPanel` | `juce::Component` | 526 |
| `CouplingArcOverlay` | `juce::Component + juce::Timer` | 145 |
| `XOlokunEditor` | `juce::AudioProcessorEditor + juce::Timer` | 484 |

**Total classes in this one file: 16**

---

### XOlokunEditor Member Variables (Component Instances)

```
std::array<std::unique_ptr<CompactEngineTile>, MaxSlots>   tiles         (4 instances)
FieldMapPanel          fieldMap
OverviewPanel          overview
EngineDetailPanel      detail
ChordMachinePanel      chordPanel
PerformanceViewPanel   performancePanel
MacroSection           macros
MasterFXSection        masterFXStrip
PresetBrowserStrip     presetBrowser
juce::TextButton       cmToggleBtn
juce::TextButton       perfToggleBtn
juce::TextButton       themeToggleBtn
CouplingArcOverlay     couplingArcs
std::unique_ptr<GalleryLookAndFeel>  laf
```

**Direct top-level component count: 14 (+ 4 tiles = 18 direct children of the editor)**

---

### addAndMakeVisible vs. Hidden at Startup

All 18 direct children are `addAndMakeVisible()` in the constructor. However three are immediately set invisible:

```cpp
// Constructor (lines 3489–3494):
detail.setVisible(false);
detail.setAlpha(0.0f);
chordPanel.setVisible(false);
chordPanel.setAlpha(0.0f);
performancePanel.setVisible(false);
performancePanel.setAlpha(0.0f);
```

At any given time only ONE of the four right-panel components is visible (overview, detail, chordPanel, or performancePanel). They all share identical bounds. Visibility is managed via 150ms fade animations using `juce::Desktop::getInstance().getAnimator()`.

**Runtime visibility state (startup):**
- VISIBLE: tiles[0–3], fieldMap, overview, macros, masterFXStrip, presetBrowser, cmToggleBtn, perfToggleBtn, themeToggleBtn, couplingArcs
- HIDDEN (alpha=0): detail, chordPanel, performancePanel

---

### EngineDetailPanel Internal Hierarchy

When a slot is selected, `EngineDetailPanel::loadSlot()` builds:
- `MacroHeroStrip` — 4 tall linear slider pillars for macro params (may auto-hide if no macros found)
- `juce::Viewport` — scrollable container
  - `ParameterGrid` — dynamically created per-engine, owned by Viewport (takeOwnership=true), previous instance is deleted on each slot change

**Note on ParameterGrid destruction:** The grid is rebuilt entirely on every slot selection. The `Viewport::setViewedComponent(..., takeOwnership=true)` call deletes the previous grid on the message thread. This is safe but means APVTS attachment destructors run frequently during normal use.

---

### PerformanceViewPanel Internal Hierarchy

Contains within itself (all `addAndMakeVisible`):
- `CouplingStripEditor couplingStrip` — 40px left column, node+arc visualization
- `juce::TextButton bakeBtn`
- `juce::TextButton clearBtn`
- `juce::ComboBox couplingPresetBox`
- 4x `RouteSection` struct, each containing:
  - `juce::Label label`
  - `juce::TextButton activeBtn`
  - `juce::ComboBox typeBox`
  - `juce::Slider depthSlider`
  - `juce::ComboBox sourceBox`
  - `juce::ComboBox targetBox`
- 4x `juce::Slider macroKnobs` + 4x `juce::Label macroLabels` (duplicate of MacroSection)

---

### ChordMachinePanel Internal Hierarchy

Contains:
- `juce::TextButton enableBtn, seqBtn, enoBtn`
- `juce::ComboBox paletteBox, voicingBox, patternBox, velCurveBox`
- `juce::Slider spreadKnob, bpmKnob, swingKnob, gateKnob, humanizeKnob, duckKnob`
- All drawn via `paint()` (the 4-card chord strip and 16-step grid are pure paint calls, not Components)

---

### MacroSection Internal Hierarchy

Contains:
- 4x `juce::Slider knobs` (rotary, XO Gold) + 4x `juce::Label lbls`
- 1x `juce::Slider master` (rotary, grey) + 1x `juce::Label masterLbl`
- Total: 10 child Components

**Macro duplication note:** MacroSection (in the persistent bottom strip) and PerformanceViewPanel both own 4 macro knobs wired to the same APVTS parameters (`macro1`–`macro4`). This is intentional (context-sensitive access) but means 8 SliderAttachments exist for these 4 parameters when PerformanceView is instantiated.

---

## Part 2: The Hidden Visualization Components

These three components exist as fully implemented headers but are **not mounted in XOlokunEditor at all**. They are not `#include`d by `XOlokunEditor.h` and do not appear in any `addAndMakeVisible()` call.

### PlaySurface (`Source/UI/PlaySurface/`)

| File | Lines |
|------|-------|
| `PlaySurface.h` | 1,091 |
| `ToucheExpression.h` | 212 |
| **Total** | **1,303** |

**Status: UNCONNECTED.** Not referenced anywhere in `XOlokunEditor.h`. The PlaySurface is the 4-zone unified playing interface described in CLAUDE.md (Zone 1: 4×4 Pad Grid / Fretless / Drum, Zone 2: Orbit, Zone 3: Expression Strip, Zone 4: Control Rail) and has its own coordinate system (`PS::kDesktopW = 1060, kDesktopH = 344`) — it's designed for a _different_ window size than the editor's 880×562. Blessings B041 (Dark Cockpit Attentional Design), B042 (Planchette as Autonomous Entity), and B043 (Gesture Trail as Modulation Source) live here.

### CouplingVisualizer (`Source/UI/CouplingVisualizer/`)

| File | Lines |
|------|-------|
| `CouplingVisualizer.h` | 1,104 |

**Status: UNCONNECTED.** Not referenced in `XOlokunEditor.h`. This is described as a "drop-in replacement for CouplingStripEditor inside PerformanceViewPanel" — but the actual PerformanceViewPanel uses `CouplingStripEditor` (191 lines, the simpler component). The CouplingVisualizer is the full diamond-layout graph with per-type coloring from `CouplingTypeColors` namespace, drag-to-add-route interaction, audio-thread RMS atomics for arc brightness, and a designed performance budget of <1ms/frame at 30Hz.

**Why it's not mounted:** It was designed as a replacement but never swapped in. The PerformanceViewPanel constructor hardcodes `CouplingStripEditor`.

### OpticVisualizer (`Source/UI/OpticVisualizer/`)

| File | Lines |
|------|-------|
| `OpticVisualizer.h` | 475 |

**Status: UNCONNECTED.** Not referenced in `XOlokunEditor.h`. This is a 4-mode Winamp/Milkdrop-style visualizer (Scope / Spectrum / Milkdrop / Particles) designed to read from `OpticEngine::getModOutputs()`. It has WCAG-compliant reduced-motion support and CRT-style phosphor green theming. It's a standalone component — there is no UI surface in the current editor to mount it.

---

### Summary Table: Hidden Components

| Component | Lines | What it does | Why hidden |
|-----------|-------|-------------|------------|
| PlaySurface | 1,303 | 4-zone playing interface; pads, fretless, orbit, expression | Never wired to editor; different size model |
| CouplingVisualizer | 1,104 | Full diamond graph for coupling routes | CouplingStripEditor used instead |
| OpticVisualizer | 475 | Audio-reactive Winamp/Milkdrop viz | No mount point in current editor |
| **Total** | **2,882** | | |

---

### Other Unconnected Components

| Component | Lines | Status |
|-----------|-------|--------|
| `PresetBrowser/PresetBrowser.h` | 413 | Duplicate/predecessor of `PresetBrowserPanel` (inline in editor). Has additional "Find Similar" DNA feature not in the inline version. Not mounted. |
| `ExportDialog/ExportDialog.h` | 1,089 | Full XPN/MPC export dialog. Not wired to any button in the current editor. |
| `OddOscar/OscarRiveComponent.h` | 528 | Rive animation component for OddOscar character. Not mounted. |
| `OddOscar/OscarJuceRenderer.h` | 258 | JUCE fallback renderer for OddOscar. Not mounted. |
| `OddOscar/OscarAnimState.h` | 89 | Animation state machine. Not mounted. |
| `Mobile/MobilePlaySurface.h` | 727 | iOS touch-optimized playing surface. Not mounted (iOS build only). |
| `Mobile/MobileTouchHandler.h` | 304 | iOS multi-touch gesture recognizer. Not mounted. |
| `Mobile/ParameterDrawer.h` | 251 | Bottom-sheet parameter drawer for mobile. Not mounted. |
| `Mobile/CPUMonitor.h` | 220 | Real-time CPU usage display. Not mounted. |
| `Mobile/SensorManager.h` | 190 | Accelerometer/gyroscope sensor integration. Not mounted. |
| `Mobile/MobileLayoutManager.h` | 204 | Adaptive layout for different iOS form factors. Not mounted. |
| `Mobile/HapticEngine.h` | 158 | Haptic feedback integration. Not mounted. |

**Total unconnected UI code: ~7,715 lines** (PlaySurface 1,303 + CouplingVisualizer 1,104 + OpticVisualizer 475 + ExportDialog 1,089 + PresetBrowser 413 + OddOscar 875 + Mobile 2,054 = ~7,313 + EngineVocabulary 204 = ~7,517)

---

## Part 3: The ParameterGrid Problem

### How It Works

`ParameterGrid` constructor (lines 449–497) iterates `proc.getParameters()` — the full list of all `juce::RangedAudioParameter*` registered in the APVTS — and includes any parameter whose ID starts with `enginePrefix + "_"`.

```cpp
juce::String pfx = enginePrefix + "_";
for (auto* p : rawParams)
{
    if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
    {
        juce::String pid = rp->getParameterID();
        if (!pid.startsWith(pfx))
            continue;
        // ... creates slider + label
    }
}
```

### Why It Shows "All 111 Parameters"

There is **no grouping logic** and **no filtering by functional category**. Every parameter with the matching prefix gets a 82×90px knob cell. For an engine like ONSET with 111 parameters, this produces a grid of 111 knobs that requires scrolling through a juce::Viewport.

The MacroHeroStrip provides partial relief by extracting `{prefix}_macro*` parameters into 4 tall pillars at the top, but these same macro params also appear again in the grid below (the grid includes all matching params including macros).

**Bug: macros appear twice.** The `ParameterGrid` does not exclude params already shown in `MacroHeroStrip`. A user sees macro parameters as both pillar sliders (top) and small rotary knobs (inside the viewport).

### Grouping Logic Status

There is no grouping. `EngineVocabulary::labelFor()` provides per-engine label overrides (204 lines of vocabulary), but no page/group/tab concept exists anywhere in the grid. The Rebirth must solve this.

---

## Part 4: Preset Browser State

### Two Preset Browsers Exist

**1. `PresetBrowserStrip`** (inline in editor, always visible in header):
- Width: 220px, height: ~30px (header strip)
- Contains: `<` prev button (22px), `>` next button (22px), `⊞` browse button (30px), name label (remaining width)
- Browse button opens `PresetBrowserPanel` in a `juce::CallOutBox`

**2. `PresetBrowserPanel`** (in CallOutBox, 380×340):
- Search field + count label
- 9 mood filter tabs (ALL + 8 moods: Foundation/Atmosphere/Entangled/Prism/Flux/Aether/Family/Submerged)
- Scrollable `juce::ListBox` (24px row height, mood dot + name + engine tag)
- Preset loading: single-click selects row, double-click loads (or `selectedRowsChanged` fires on click)

**3. `PresetBrowser/PresetBrowser.h`** (413 lines, NOT mounted):
- Separate, more capable implementation with DNA-based "Find Similar" feature
- Never wired to the editor

### Preset Loading Correctness

The preset loading path in `PresetBrowserStrip::openBrowser()` uses a `SafePointer` pattern to guard against editor destruction during the async CallOutBox. The `applyPreset()` call routes through the processor correctly.

**Known limitation:** The mood filter in `PresetBrowserPanel` only shows 9 moods (ALL + 8). The full preset system has 15 moods (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged, Coupling, Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic). The 7 new moods added after the browser was written are invisible to the filter UI.

---

## Part 5: LookAndFeel Assessment

### GalleryLookAndFeel

`GalleryLookAndFeel` extends `juce::LookAndFeel_V4`. It overrides two methods:

**`drawRotarySlider()`** — The "Ecological Instrument Paradigm" knob:
1. Focus ring (WCAG 2.4.7) — 2px blue ring when keyboard-focused
2. Three zone arcs (sunlit/twilight/midnight depth bands at 0.22 opacity): cyan, blue, violet
3. Full-sweep track ring (borderGray at 40% alpha, 1.5px stroke)
4. Value arc (engine accent color, 3px stroke, from start to current position)
5. Center disc (warm white shell, 44% of radius) with live value readout on drag
6. Setpoint triangle ▲ (XO Gold filled, pointing outward at current position)

**`drawButtonBackground()`** — Simple rounded rectangle with:
- Down state: XO Gold fill
- Highlighted: brightened 6%
- Normal: bg color
- Focus state: A11y blue focus ring (replaces border)
- Normal border: 1px borderGray rounded

**Color scheme:**
- Light mode default (`GalleryColors::darkMode() = false`)
- Shell white `#F8F6F3` background
- XO Gold `#E9C46A` brand constant (macros, coupling, active states)
- Text dark `#1A1A1A`, text mid `#777570`, border gray `#DDDA D5`
- Full dark mode support via `applyTheme()` called on toggle

**Typography system (`GalleryFonts`):**
- Space Grotesk Bold + Medium (display headings) — embedded as `FontData::SpaceGroteskBold_otf`
- Inter Regular + Medium + Bold (body/label) — embedded as `FontData::InterRegular_ttf` etc.
- JetBrains Mono Regular (numeric values) — embedded as `FontData::JetBrainsMonoRegular_ttf`
- All 6 typefaces loaded as static cached pointers on first access
- **All 3 fonts are properly embedded** via `juce_add_binary_data` in CMakeLists.txt

**Accessibility (`A11y` namespace):**
- WCAG 2.4.7: focus rings on all interactive components
- WCAG 4.1.2: `setTitle()`/`setDescription()` on all components
- WCAG 2.1.1: keyboard activation on CompactEngineTile (Return/Space)
- WCAG 2.5.8: touch target minimum check utility (not enforced — utility only)
- WCAG 2.3.3: `prefersReducedMotion()` (macOS approximation via screen saver API — imprecise)

---

## Part 6: Font Integration

All three typefaces are fully embedded:

```cpp
// GalleryFonts namespace, lines 130–209
static auto tf = loadTypeface(FontData::SpaceGroteskBold_otf,
                               FontData::SpaceGroteskBold_otfSize);  // ✓
static auto tf = loadTypeface(FontData::SpaceGroteskMedium_otf, ...); // ✓
static auto tf = loadTypeface(FontData::InterRegular_ttf, ...);        // ✓
static auto tf = loadTypeface(FontData::InterMedium_ttf, ...);         // ✓
static auto tf = loadTypeface(FontData::InterBold_ttf, ...);           // ✓
static auto tf = loadTypeface(FontData::JetBrainsMonoRegular_ttf, ...);// ✓
```

Font access is through semantic helpers:
- `GalleryFonts::display(size)` — Space Grotesk Bold
- `GalleryFonts::heading(size)` — Inter Bold
- `GalleryFonts::body(size)` — Inter Regular
- `GalleryFonts::label(size)` — Inter Medium
- `GalleryFonts::value(size)` — JetBrains Mono

Usage is consistent throughout the file. No hardcoded JUCE default font calls observed in the main rendering paths.

---

## Part 7: Performance / Timer Analysis

### Active Timers (at steady state with all engines loaded)

| Component | Frequency | Purpose | Repaint cost |
|-----------|-----------|---------|--------------|
| `FieldMapPanel` | 30 Hz | Unconditional repaint | Full panel repaint every frame |
| `CouplingArcOverlay` | 30 Hz | Pulse animation for active arcs | Full editor-sized overlay repaint |
| `CompactEngineTile` × 4 | 10 Hz each | Voice count polling per slot | 4 tile repaints at 10 Hz |
| `ChordMachinePanel` | 10 Hz | Step highlight (only when step changes) | Conditional — good |
| `XOlokunEditor` | 1 Hz | Fallback tile refresh + fieldMap drain | 4 tiles + overview |

**Total active timer callbacks (all panels loaded, performance view hidden):**
- 30 Hz: FieldMapPanel + CouplingArcOverlay = 2 timers = 60 callbacks/sec
- 10 Hz: 4 × CompactEngineTile = 4 timers = 40 callbacks/sec
- 1 Hz: XOlokunEditor = 1 callback/sec
- **Total: ~101 timer callbacks/second at steady state**

When ChordMachinePanel is visible: +10 Hz = 111 callbacks/sec.

### repaint() Call Analysis

`repaint()` is called unconditionally in `FieldMapPanel::timerCallback()` and `CouplingArcOverlay::timerCallback()` — both at 30 Hz — regardless of whether anything has changed visually. This is the primary performance risk.

`CouplingArcOverlay::paint()` calls `processor.getCouplingMatrix().getRoutes()` every frame (presumably an `atomic_load` of a shared_ptr). With no active routes it returns early, but the atomic load still happens 30 times/second.

`XOlokunEditor::timerCallback()` at 1 Hz:
- Calls `tiles[i]->refresh()` × 4 (which calls `processor.getEngine(i)` × 4)
- Calls `overview.refresh()` if detail is not visible (which calls `getRoutes()` for the coupling route count)
- Drains note events via `processor.drainNoteEvents()`

### Dark Mode Toggle Performance Problem

The `themeToggleBtn` onClick handler (lines 3537–3550) calls `repaint()` on **every single component** in the hierarchy manually, including tile[0–3], overview, detail, chordPanel, performancePanel, macros, masterFXStrip, presetBrowser. This is a full-tree manual invalidation. A single `getTopLevelComponent()->repaint()` would suffice.

### FieldMapPanel: Always-On 30Hz

`FieldMapPanel` runs at 30 Hz unconditionally even when the user never plays a note. When no notes have been played, `paint()` still runs the full background fill + 3 zone bands + 2 boundary lines + time cursor draw. There is no early-exit for the empty state.

---

## Part 8: Overall Size Accounting

### Source/UI/ File Inventory

| File | Lines | Status |
|------|-------|--------|
| `XOlokunEditor.h` | 3,936 | Active — the entire current UI |
| `CouplingVisualizer/CouplingVisualizer.h` | 1,104 | **Unconnected** |
| `PlaySurface/PlaySurface.h` | 1,091 | **Unconnected** |
| `ExportDialog/ExportDialog.h` | 1,089 | **Unconnected** |
| `Mobile/MobilePlaySurface.h` | 727 | **Unconnected (iOS only)** |
| `OddOscar/OscarRiveComponent.h` | 528 | **Unconnected** |
| `OpticVisualizer/OpticVisualizer.h` | 475 | **Unconnected** |
| `PresetBrowser/PresetBrowser.h` | 413 | **Unconnected (superseded inline)** |
| `Mobile/MobileTouchHandler.h` | 304 | **Unconnected (iOS only)** |
| `OddOscar/OscarJuceRenderer.h` | 258 | **Unconnected** |
| `Mobile/ParameterDrawer.h` | 251 | **Unconnected (iOS only)** |
| `Mobile/CPUMonitor.h` | 220 | **Unconnected** |
| `PlaySurface/ToucheExpression.h` | 212 | **Unconnected** |
| `Mobile/MobileLayoutManager.h` | 204 | **Unconnected (iOS only)** |
| `EngineVocabulary.h` | 204 | Active — used by ParameterGrid |
| `Mobile/SensorManager.h` | 190 | **Unconnected (iOS only)** |
| `CouplingStrip/CouplingStripEditor.h` | 191 | Active — used by PerformanceViewPanel |
| `Mobile/HapticEngine.h` | 158 | **Unconnected (iOS only)** |
| `OddOscar/OscarAnimState.h` | 89 | **Unconnected** |
| `PerformanceViewPanel.h` | 5 | Active — forwarding header only |
| **Total** | **11,649** | |

**Active code: ~4,336 lines** (XOlokunEditor.h + EngineVocabulary.h + CouplingStripEditor.h + PerformanceViewPanel.h)
**Unconnected code: ~7,313 lines** (66% of UI source is written but not mounted)

---

## Part 9: Layout Constants

From `XOlokunEditor` (lines 3907–3912):

```cpp
static constexpr int kHeaderH   = 50;    // Top header bar
static constexpr int kMacroH    = 105;   // MacroSection bottom strip
static constexpr int kMasterFXH = 68;    // MasterFX strip (above macros)
static constexpr int kSidebarW  = 155;   // Left tile column
static constexpr int kFadeMs    = 150;   // Cross-fade duration (ms)
static constexpr int kFieldMapH = 110;   // FieldMap panel (above macros)
```

**Default window size:** 880 × 562 px
**Resize range:** 720×460 min → 1400×900 max
**Right panel available area:** (880 - 155) × (562 - 50 - 105 - 68 - 110) = 725 × 229 px

The right panel area (229px tall after subtracting fixed strips) is quite constrained for detailed engine parameter editing.

---

## Part 10: Known Issues and Architecture Deficits

### Confirmed Bugs

1. **Macro params appear twice** — `MacroHeroStrip` shows macros as pillars AND `ParameterGrid` includes those same params again as rotary knobs (no exclusion logic between them).

2. **Mood filter shows 9 of 15 moods** — `PresetBrowserPanel` hardcodes 9 mood tabs (ALL + 8). The 7 moods added in 2026-03-20 (Coupling, Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic) are invisible in the browser.

3. **Dark mode manual repaint cascade** — `themeToggleBtn.onClick` manually calls `repaint()` on 8 named components instead of invalidating the top-level component once.

4. **reduceMotion approximation** — `A11y::prefersReducedMotion()` on macOS uses the screen saver API as an approximation for `prefers-reduced-motion`. This is wrong; on JUCE 7+ there is no clean cross-platform API, but the current implementation can return `true` when the screen saver is disabled for unrelated reasons.

### Architecture Deficits the Rebirth Must Address

1. **No parameter grouping** — All N parameters of an engine dump into one flat scrollable grid. Engines like OFFERING (84 params) and OBRIX (81 params) produce unusably long grids.

2. **PlaySurface is orphaned** — 1,303 lines of Blessing B041/B042/B043 code has no mount point. The entire "playing the space between" philosophy lives here but is invisible to users.

3. **CouplingVisualizer is orphaned** — The fully-spec'd replacement for CouplingStripEditor (1,104 lines, per-type colors, drag-to-add, RMS brightness) was never swapped in.

4. **ExportDialog is orphaned** — 1,089 lines of export tooling with no button to open it in the current UI.

5. **OpticVisualizer is orphaned** — The only visual engine (OPTIC) has no visual output surface in the current UI.

6. **Panel height is too constrained** — The right panel has ~229px of usable height for engine detail. At 82×90px per knob cell (ParameterGrid constants), only 2–3 knob rows fit before scrolling begins.

7. **FieldMapPanel always repaints at 30Hz** — Even when no notes are playing, this full-panel repaint runs. Combined with CouplingArcOverlay's unconditional 30Hz repaint, the idle GPU load is non-trivial.

8. **Duplicate macro knobs** — MacroSection (always visible) and PerformanceViewPanel both render the same 4 macro knobs. On screen these are never simultaneously visible, but 8 SliderAttachments exist for 4 parameters.

9. **No gesture modulation surface** — Blessing B043 (Gesture Trail as First-Class Modulation Source) is fully designed but requires PlaySurface to be mounted.

10. **CouplingStripEditor used instead of CouplingVisualizer** — The stripped-down 191-line CouplingStripEditor (dark background, basic node circles) is used in PerformanceViewPanel instead of the 1,104-line CouplingVisualizer with its full per-type color palette and drag interaction.

---

## Summary for Rebirth Planning

The current UI is a **functional but cramped single-file monolith** (3,936 lines) that shows one of four overlapping panels in a ~229px tall right pane. The real assets — PlaySurface, CouplingVisualizer, OpticVisualizer, ExportDialog — are all built, tested (visually), and waiting to be connected.

The Rebirth does not need to write most of the DSP-adjacent visualization logic from scratch. It needs a **layout architecture** that can give those existing components the space they deserve.

**What the Rebirth replaces:** the layout constants, the single-panel right-pane model, the flat ParameterGrid, and the show/hide animation logic.

**What the Rebirth should keep intact:**
- `GalleryLookAndFeel` (solid, WCAG-compliant, well-designed knobs)
- `GalleryColors` namespace (complete light/dark theme system)
- `GalleryFonts` namespace (all 6 typefaces properly embedded)
- `A11y` namespace (accessibility utilities)
- `CompactEngineTile` (porthole design, voice activity, keyboard nav — fully correct)
- `MacroSection` (clean, functional)
- `MasterFXSection` (clean, functional)
- `PresetBrowserStrip` (correct, safe pointer patterns)
- `CouplingArcOverlay` (animated arcs, good pulse math)
- `PerformanceViewPanel` (BAKE workflow, coupling preset management — complete)
- `ChordMachinePanel` (complete Chord Machine UI)
- `FieldMapPanel` (good concept; just needs conditional repaint gating)
