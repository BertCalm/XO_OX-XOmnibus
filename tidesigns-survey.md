# TIDEsigns Situational Brief: XOceanus (Desktop JUCE Plugin)
**Date:** 2026-03-28
**Surface(s):** JUCE (macOS/Windows AU/VST3)
**Tides most affected:** T1 (Pad Player), T2 (Deep Diver), T3 (First Touch), T4 (Session Pro)

## What Exists Today

### Codebase Scale
- **403 source files** across 6 major directories
- **78 registered synth engines** (74 unique + Kitchen Collection quads)
- **~31,000 lines of UI code** across 81 header files
- **44 Gallery components** (primary desktop UI system)
- **7 embedded fonts** (Space Grotesk, Inter, JetBrains Mono, Overbit)
- **CMake build system**, JUCE 8, AU/VST3 targets, auval PASS

### Active UI Architecture
- **XOlokunEditor** — 3,936-line monolith root component (484 lines core + 14 children)
- **GalleryLookAndFeel** — Custom LookAndFeel_V4 subclass (rotary knobs, buttons, WCAG focus rings)
- **GalleryColors** — Full dark-mode-primary color system (bg #0E0E10, surface #1A1A1C, XO Gold #E9C46A)
- **ColumnLayoutManager** — 3-column responsive layout + Cinematic Mode
- **CompactEngineTile** — 4 primary slots + 1 ghost slot (porthole design)
- **ParameterGrid** — Flat 8xN scrollable grid (NO parameter grouping)
- **MacroHeroStrip + MacroSection** — Dual macro display (known duplicate bug)
- **MasterFXSection** — FX strips (272 lines)
- **PerformanceViewPanel** — Coupling/bake UI (526 lines)
- **PresetBrowserStrip/Panel** — Inline preset selection (9/15 moods active)
- **ChordMachinePanel** — Generative chord sequencer (299 lines)
- **FieldMapPanel** — Note history timeline (117 lines, always repainting at 30Hz)
- **CouplingArcOverlay** — Animated arc viz (145 lines, always repainting at 30Hz)

### Orphaned UI Code (~7,313 lines built but unmounted)
| Component | Lines | Why Orphaned |
|-----------|-------|-------------|
| PlaySurface | 1,303 | 4-zone interface; no mount point in layout |
| CouplingVisualizer | 1,104 | Full diamond graph; never swapped in for basic strip |
| ExportDialog | 1,089 | XPN/MPC export; no button to open |
| OpticVisualizer | 475 | Audio-reactive viz; no visual output surface |
| OddOscar suite | 875 | Rive animation + JUCE renderer; not integrated |
| Mobile components | 2,054 | iOS-only (touch, haptics, sensors) |
| PresetBrowser (old) | 413 | Superseded but has DNA "Find Similar" feature |

### Layout Constants
```
Default window:   880 x 562 px
Resize range:     720x460 (min) → 1400x900 (max)
kHeaderH:         50px   (header + preset browser)
kSidebarW:        155px  (engine tile column)
kMasterFXH:       68px   (FX strips)
kFieldMapH:       110px  (note timeline)
kMacroH:          105px  (bottom macro knobs)
Right panel area: 725 x 229px (CRITICAL CONSTRAINT)
```

### Design System Infrastructure
- **B041 — Dark Cockpit**: 5-level opacity hierarchy (100/80/45/20/0%)
- **B042 — The Planchette**: Autonomous cursor with Lissajous drift, spring lock-on
- **B043 — Gesture Trail**: Ring buffer (256 tuples) as replayable modulation source
- **73-Color System**: Each engine has a dedicated accent color in EngineVocabulary.h
- **WCAG Compliance**: Focus rings (2.4.7), keyboard nav (2.1.1), semantic labels (4.1.2)

## What the Vision Says

### Canonical Specs (3 documents)
1. **xolokun-definitive-ui-spec.md** (1,774 lines) — 5 principles: Planchette, Dark Cockpit, PlaySurface visualization, Sound on Launch, 73-color system
2. **xolokun-spatial-architecture-v1.md** (639 lines) — Hybrid Stack + Context Strip paradigm, all 326 features mapped to UI homes
3. **xolokun-wiring-manifest.md** (1,062 lines) — Line-by-line EXISTS/REWIRE/BUILD/ISSUE status with thread safety annotations

### HTML Prototypes (10 mockups)
- **xolokun-v05-accurate.html** (81KB) — Latest design iteration (2026-03-27)
- **xolokun-v04-polished.html** (80KB) — Primary visual target per MEMORY mandate
- **outshine-prototype.html** (44KB) — Sample forge companion window
- **playsurface-width-comparison.html** (27KB) — PlaySurface sizing studies
- Plus: v03 wireframe, v02 prototype, iPad landscape, iPhone, MPC hardware, main UI

### Upstream Reviews
- **Synth Seance V1**: 7/8 "Almost Ready", 1/8 "Ready to Ship". Top priority: audio demos, scope clarity, community infra
- **UI Blessing Session**: Dark Cockpit approved, Planchette blessed, Gesture Trail as modulation blessed
- **TIDEsigns Phases 1-2**: COMPLETE. 8 Critical bugs fixed. Phase 3 IN PROGRESS (signal flow + coupling tab)
- **Architect**: Outshine approved with conditions (both resolved 2026-03-25)

### Key Design Mandates
- Dark mode IS primary (user mandate, overrides any light-mode assumptions)
- Gallery Model: engine accent color framing, light shell aesthetic ONLY in light mode variant
- PlaySurface: must ship as unified 4-zone interface (Pad/Fretless/Drum/Expression)
- Signal Flow Strip: must become interactive (click section → scroll to parameters)
- Coupling Tab: must show engine names and accent colors, not "Slot N" placeholders

## The Gap

### Critical Visual Gap (MEMORY PRIORITY)
> "Build must match `Docs/mockups/xolokun-v04-polished.html`. NO new features until visual gap closed."

The current build does NOT match the polished prototype. 13 components + 5th slot + Column C + 15 audit fixes were built in 2026-03-25, but visual quality remains below prototype fidelity.

### Architectural Gaps
1. **Right panel constraint**: 229px usable height — engines with 84-111 parameters become an unnavigable scroll pit
2. **No parameter grouping**: Flat grid dumps all params equally; no semantic sections, no collapsing
3. **7,313 lines of orphaned UI**: Major features (PlaySurface, CouplingVisualizer, ExportDialog, OpticVisualizer) built but unreachable
4. **101 callbacks/second at idle**: FieldMapPanel (30Hz) + CouplingArcOverlay (30Hz) + 4xEngineTile (10Hz each) + Editor (1Hz)
5. **Signal flow strip static**: Designed as interactive breadcrumb, hard-coded to `activeSection = 0`
6. **Macros displayed twice**: MacroHeroStrip AND ParameterGrid show same parameters
7. **Mood filter incomplete**: 9/15 moods active in preset browser
8. **Dark mode cascade manual**: Theme toggle repaints 8 components individually instead of top-level invalidation

### UX Gaps
- **T3 (First Touch)**: Can a beginner find something beautiful in 10 minutes? Seance says unclear.
- **T1 (Pad Player)**: PlaySurface is the primary instrument — it's built but has zero mount point
- **T2 (Deep Diver)**: 111 parameters in a flat grid with no grouping is hostile
- **5 stub engines** (Obbligato, Ohm, Ole, Orphica, Ottoni) are silent — no "Coming Soon" UI

## Surfaces Involved
- **Primary**: JUCE desktop plugin (macOS AU, Windows VST3)
- **Secondary reference**: HTML prototypes (design targets, not deployable)
- **Out of scope for this run**: OBRIX Pocket iOS, MPC expansion UX, web

## Initial Risk Flags
1. **Scope vs. polish tension**: 78 engines + 7K orphaned lines + visual gap mandate. Risk of spreading thin.
2. **Right panel height**: 229px is a hard constraint from the current layout math. Expanding it means shrinking something else.
3. **Performance at idle**: 101 callbacks/sec before the user touches anything. Adding PlaySurface (which needs its own render loop) could make this worse.
4. **Phase 3 in progress**: TIDEsigns already has active Phase 3 work (signal flow + coupling tab). This full pipeline must integrate with, not overwrite, that work.
5. **Dark mode fidelity**: Prototype is the target, but the prototype was built in HTML/CSS. Translating to JUCE's rendering model has inherent fidelity losses (no CSS box-shadow, no backdrop-blur, limited gradient support).

## Recommended Phase 2 Focus

| Specialist | Priority Focus |
|-----------|---------------|
| **Marina** | Vision-to-reality alignment: prototype vs. build fidelity. What specifically doesn't match? |
| **Drift** | The 7,313-line orphan problem: how do PlaySurface, CouplingVisualizer, ExportDialog get mounted? What flows break? |
| **Coral** | Dark mode pixel audit: compare prototype screenshots to live build. Identify every visual deviation. |
| **Haven** | T3 (First Touch) walkthrough: can a new user accomplish anything meaningful in the first session? |
| **Lattice** | Right panel constraint: can the 229px height be reclaimed? What's the component architecture for parameter grouping? |
| **Forge** | Performance audit: 101 callbacks/sec + orphaned render loops. What's the real cost? What can be lazy? |
| **Sheen** | Phase 3 integration: what's already been fixed in Phases 1-2? What's the exact state of the in-progress Phase 3 work? |
