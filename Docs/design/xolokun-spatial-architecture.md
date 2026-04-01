# XOlokun Spatial Architecture — Where Every Feature Lives

> **Status:** SUPERSEDED — This is the Phase 1 draft. Canonical version: `xolokun-spatial-architecture-v1.md` (post-10-panel review).
> **Date:** 2026-03-24
> **Authors:** Ringleader + UIX Design Studio
> **Purpose:** Prove that every planned feature (326 cataloged) has a specific home in the UI before any visual polish is applied.
> **Principle:** Architecture before aesthetics. Wireframe before gradients. Every feature gets an address.

---

## TABLE OF CONTENTS

1. [Design Constraints](#1-design-constraints)
2. [Layout Paradigm Selection](#2-layout-paradigm-selection)
3. [Primary View Architecture](#3-primary-view-architecture)
4. [Feature-to-Location Master Map](#4-feature-to-location-master-map)
5. [Component Taxonomy (Reusable Library)](#5-component-taxonomy)
6. [Engine-Specific UI Requirements](#6-engine-specific-ui-requirements)
7. [OBRIX Special Treatment](#7-obrix-special-treatment)
8. [Extensibility Schema](#8-extensibility-schema)
9. [Interaction Flow Maps](#9-interaction-flow-maps)
10. [What We Learn from Vital/Serum/Pigments](#10-competitor-lessons)
11. [Open Questions for User Review](#11-open-questions)

---

## 1. Design Constraints

### Hard Constraints (non-negotiable)
- **Window:** 1100×700pt default, min 960×600, max 1920×1080
- **4 engine slots** simultaneously active
- **73 swappable engines** with 24-111 parameters each
- **15 coupling types** between any pair of loaded engines
- **FX chain** per engine (6 FX slots: SAT/DELAY/REVERB/MOD/COMP/SEQ)
- **PlaySurface** with 3 modes (XOuija fretless, MPC 16-pad, Seaboard keyboard)
- **Expression controllers** (mod wheel, pitch bend, 4 macro strips, tide controller, XY pad)
- **Preset browser** with ~17,250 presets, 15 moods, DNA similarity search
- **Must run as plugin** inside DAW (host window constraints, no floating windows in AU)
- **Must work on iPad** (AUv3 constraints, touch-only, safe areas)

### Soft Constraints (strong preferences)
- **No more than 2 clicks** to reach any frequently-used feature
- **Progressive disclosure** — default view shows ≤12 controls
- **Symmetrical, patterned layout** — logically clustered control groups (user feedback)
- **Future-proof** — unknown features from 2027+ must have a path into the UI
- **Each engine can look different** where it matters, but shares the same component library
- **Coupling must be self-explanatory** — a new user should understand what it does visually

### Pixel Budget
At 1100×700pt, our total area is **770,000 px²**. After chrome (header 52pt + status bar 28pt = 80pt of vertical space), we have **1100 × 620 = 682,000 px²** of working space. Every feature competes for this space.

---

## 2. Layout Paradigm Selection

### The Problem No Synth Has Solved

Most synths have 2-3 sound sources (oscillators). XOlokun has **4 simultaneous engine slots** drawn from a pool of **73 engines**, each with different parameter structures. This is closer to a **DAW mixer** than a traditional synth. No existing synth UI paradigm handles this cleanly.

### What We Can Learn from Competitors

| Synth | Layout Pattern | Engines/Oscs | What Works | What Doesn't (for us) |
|-------|---------------|-------------|------------|----------------------|
| **Vital** | 3-column (OSC1 / FILTER+ENV / OSC2+3), mod matrix bottom | 3 osc + sampler | Everything visible, logical flow L→R, wavetable editor dominates | Fixed 3-osc layout, doesn't scale to 73 swappable engines |
| **Serum** | 2 oscs top, filter mid, FX tabs bottom, matrix sidebar | 2 osc | Clean wavetable view, tab system for FX depth | Only 2 sources, tabs feel hidden |
| **Pigments** | Top tabs (ENGINE / SEQ / FX / MATRIX), full-width content per tab | 2 engines + seq | Full width per view, generous knob spacing | You see ONE thing at a time — lose context of the whole patch |
| **Phase Plant** | Vertical generator stack (add/remove/reorder), FX chain right | N generators | Scalable to any number of sources | Very tall, lots of scrolling, no overview |
| **Omnisphere** | Layer A/B tabs, massive browser, FX page | 2 layers | Deep browser, rich FX | Tabbed = you forget what the other layer is doing |
| **OP-1 (TE)** | Single screen, mode-switching, same 4 knobs control different params | 1 engine | Radical simplicity, same physical gestures for everything | Too constrained for 73 engines with different structures |

### The XOlokun Answer: Hybrid Stack + Context Strip

**Core insight:** We need to see ALL 4 engine slots simultaneously (like a mixer), but we can't show all params for all 4 engines at once (that's 200-400 knobs). The solution:

1. **Engine Stack** — 4 engine slots always visible as compact tiles (like mixer channels)
2. **Detail Panel** — ONE engine expanded at a time for deep editing (like clicking a mixer channel to open its channel strip)
3. **Context Strip** — A persistent horizontal strip showing coupling state + FX chain (like a mixer's master bus)
4. **PlaySurface** — Bottom zone for performance input (like a mixer's keyboard/pad area)

This gives us:
- **Always visible:** All 4 engines (compact), coupling state, macros, preset name
- **One click:** Expand any engine to full detail, open preset browser, switch FX
- **Two clicks:** Access any parameter in any engine, configure coupling, export

---

## 3. Primary View Architecture

### The 4-Zone Layout

```
+═══════════════════════════════════════════════════════════════+
| HEADER (52pt)                                                 |
| [Logo] [Engine Selector] [Preset Nav] ←→ [Macros M1-M4] [⚙] |
+═══════════════════════════════════════════════════════════════+
|                                                               |
| ZONE A: ENGINE RACK (left, 280pt wide)                       |
| ┌──────────────┐  ZONE B: DETAIL/CONTEXT (center, 540pt)    |
| │ SLOT 1 ████  │  ┌─────────────────────────────────────┐   |
| │ [name] [4mac]│  │                                     │   |
| │ [mini wave]  │  │  When no slot selected:             │   |
| ├──────────────┤  │  OVERVIEW (coupling graph +         │   |
| │ SLOT 2 ████  │  │  preset info + engine chain)        │   |
| │ [name] [4mac]│  │                                     │   |
| │ [mini wave]  │  │  When slot clicked:                 │   |
| ├──────────────┤  │  ENGINE DETAIL (params grouped      │   |
| │ SLOT 3 ████  │  │  by OSC/FILTER/MOD/FX + ADSR      │   |
| │ [name] [4mac]│  │  + oscilloscope)                    │   |
| ├──────────────┤  │                                     │   |
| │ SLOT 4 ████  │  │  When coupling tab:                 │   |
| │ [name] [4mac]│  │  COUPLING EDITOR (visualizer +      │   |
| │              │  │  route config + type selector)       │   |
| └──────────────┘  │                                     │   |
|                    │  When FX tab:                       │   |
| ZONE C: SIDEBAR    │  FX CHAIN (4 slots + wet/dry +     │   |
| TABS (280pt wide)  │  serial/parallel)                   │   |
| ┌──────────────┐  │                                     │   |
| │ [PRESET]     │  │  When preset tab:                   │   |
| │ [COUPLE]     │  │  PRESET BROWSER (search + mood      │   |
| │ [FX]         │  │  + DNA + list)                      │   |
| │ [PLAY]       │  └─────────────────────────────────────┘   |
| │              │                                             |
| │ (tab content │                                             |
| │  fills this  │                                             |
| │  column)     │                                             |
| └──────────────┘                                             |
+═══════════════════════════════════════════════════════════════+
| ZONE D: PLAYSURFACE (full width, 220pt tall)                 |
| [Tab: XOUIJA | MPC | KEYS]     [Expression Controllers 160pt]|
| [Active Surface 940pt wide]     [Mod|Bend|Macros|Tide|XY]    |
+═══════════════════════════════════════════════════════════════+
| STATUS BAR (28pt)                                             |
| [FIRE][XOSEND][ECHO][PANIC] | BPM | Voices | CPU | [slots]  |
+═══════════════════════════════════════════════════════════════+
```

### Wait — This Doesn't Work Yet

The layout above has a problem: **Zone B and Zone C overlap**. We can't have a center detail panel AND a right sidebar that both want the same horizontal space. Let me resolve this.

### Revised: 3-Column with Contextual Center

```
+═══════════════════════════════════════════════════════════════════+
| HEADER (52pt)                                                     |
| [Logo] [EngSel] [←Preset→] [♡] [DNA]    [M1][M2][M3][M4] [⚙☀] |
+═══════════════════════════════════════════════════════════════════+
|          |                              |                         |
| COL A    |         COL B               |        COL C            |
| ENGINE   |     MAIN CANVAS             |      BROWSER/           |
| RACK     |    (contextual)             |      INSPECTOR          |
| 220pt    |      ~580pt                 |       300pt             |
|          |                              |                         |
| ┌──────┐ | Default: OVERVIEW           | Default: PRESET         |
| │SLOT 1│ | (coupling graph + chain     | BROWSER                 |
| │ ████ │ |  overview + engine info)    | [Search...]             |
| │ name │ |                              | [Mood pills]            |
| │ wave │ | On slot click: ENGINE       | [Preset list]           |
| │m1m2  │ | DETAIL (grouped params,     |                         |
| │m3m4  │ | ADSR, oscilloscope,         | On COUPLE click:        |
| ├──────┤ | progressive disclosure)     | COUPLING INSPECTOR      |
| │SLOT 2│ |                              | (type, amount, depth,   |
| │ ████ │ | On FX click: FX CHAIN       | route config)           |
| │ name │ | (4 slots, reorderable,      |                         |
| │ wave │ | wet/dry per slot,           | On FX click:            |
| │m1m2  │ | serial/parallel)            | FX INSPECTOR            |
| │m3m4  │ |                              | (per-slot deep params)  |
| ├──────┤ |                              |                         |
| │SLOT 3│ |                              | On settings:            |
| │ ████ │ |                              | SETTINGS PANEL          |
| │......│ |                              |                         |
| ├──────┤ |                              |                         |
| │SLOT 4│ |                              |                         |
| │ ████ │ |                              |                         |
| │......│ |                              |                         |
| └──────┘ |                              |                         |
+═══════════════════════════════════════════════════════════════════+
| PLAYSURFACE (full width, 220pt)                                   |
| [XOUIJA | MPC | KEYS]  [active surface]  [expression controllers] |
+═══════════════════════════════════════════════════════════════════+
| STATUS (28pt) [FIRE][XOSEND][ECHO][PANIC] BPM Voices CPU [●●●●] |
+═══════════════════════════════════════════════════════════════════+
```

### Column Responsibilities

| Column | Width | Always Shows | Contextual Content | User Action to Switch |
|--------|-------|-------------|-------------------|----------------------|
| **A: Engine Rack** | 220pt | 4 engine slot tiles (compact) | — | Click slot to select for detail |
| **B: Main Canvas** | ~580pt | Overview (default) | Engine Detail / FX Chain | Click slot (detail), click FX tab |
| **C: Browser/Inspector** | 300pt | Preset Browser (default) | Coupling Inspector / FX Inspector / Settings | Click sidebar tabs or coupling arc |

### Why This Works

1. **All 4 engines always visible** — Column A is persistent. You never lose sight of what's loaded.
2. **Detail on demand** — Click any slot in Column A → Column B shows that engine's full parameter view.
3. **Context preserved** — While editing Engine 1 in Column B, you can still see Engines 2-4 in Column A.
4. **FX has a home** — Column B switches to FX Chain view. Column C shows per-FX deep params.
5. **Coupling is discoverable** — The coupling arcs are visible in the Overview (Column B default). Click them → Column C shows the Coupling Inspector with type/amount/depth controls.
6. **Browser always accessible** — Column C defaults to Preset Browser. One click on the PRESET tab returns you there from any inspector.
7. **PlaySurface is persistent** — Bottom zone, always available for performance input.
8. **Symmetrical layout** — Left rack, center canvas, right inspector. Standard 3-column pattern used by every professional creative tool (Ableton, Logic, Final Cut, Figma).

### Pixel Math Verification

```
Header:      52pt
Columns:     1100 - 0 = 1100pt wide
  Col A:     220pt
  Col B:     580pt
  Col C:     300pt
  TOTAL:     1100pt ✓
Column H:    700 - 52 (header) - 220 (playsurface) - 28 (status) = 400pt
PlaySurface: 220pt
Status:      28pt
TOTAL:       52 + 400 + 220 + 28 = 700pt ✓
```

---

## 4. Feature-to-Location Master Map

Every one of the 326 cataloged features assigned to a specific zone.

### HEADER (52pt × 1100pt = 57,200 px²)

| Feature | Location in Header | Size |
|---------|-------------------|------|
| XO_OX Logo | Far left, 16pt from edge | 32×32pt |
| Engine Selector (Depth-Zone Dial) | Left of center | 48×48pt |
| Preset Navigator (←name→♡DNA) | Center-left | ~250pt wide |
| Macro Knobs M1-M4 | Right of center | 4 × 36pt + 24pt spacing = 168pt |
| Dark/Light toggle | Far right group | 20×20pt |
| CPU meter | Far right group | ~40pt |
| MIDI indicator | Far right group | 8pt |
| Settings gear | Far right group | 20×20pt |
| XPN Export button | Far right group | 46pt |

**Header budget:** Logo(48) + EngSel(64) + PresetNav(250) + gap(32) + Macros(168) + gap(32) + Indicators(134) = **728pt used of 1100pt = 372pt breathing room.** That's generous — the V0.2 prototype was cramped because it didn't account for this spacing.

**IMPORTANT FIX from user feedback:** The 4 macro knobs at 36pt with only 6pt gaps was too tight. At the spacing above (36pt knobs + 8pt gaps), the total macro cluster is 168pt — readable and not cramped. We could even go to 40pt knobs here.

### COLUMN A: ENGINE RACK (220pt × 400pt = 88,000 px²)

Each slot tile: 220pt × ~95pt (4 tiles + 3 gaps in 400pt = 95pt each + 5pt gaps)

| Feature | Location in Slot Tile | Size |
|---------|---------------------|------|
| Engine accent stripe | Left edge, 4pt wide | 4×95pt |
| Creature icon | Top-left, inside stripe | 20×20pt |
| Engine name | Top, next to icon | ~120pt wide |
| Param count badge | Top-right | ~40pt |
| Mini waveform | Middle row | ~200×30pt |
| 4 mini macro indicators | Bottom row | 4 × 28pt knobs |
| Active/selected highlight | Border glow | Full tile border |
| Coupling indicator dot | Between tiles | 8pt dot when coupling active |

**This solves the "only 2 engines visible" problem.** All 4 slots are always here, compact but readable.

### COLUMN B: MAIN CANVAS (580pt × 400pt = 232,000 px²)

This is the largest zone — it contextually shows different content.

#### B1: Overview (default, no slot selected)

| Feature | Location | Size |
|---------|----------|------|
| Coupling Visualizer graph | Center, dominant | ~400×280pt |
| Engine chain pills (slot 1→2→3→4) | Top | 560×40pt |
| Archetype tagline | Below chain | ~200pt |
| "Click a slot to edit, or click arcs to configure coupling" | Bottom hint | Text |

**This solves "coupling doesn't make sense."** The Overview IS the coupling visualizer. It's the first thing you see. The 4 engine nodes correspond to the 4 slots in Column A. Arcs show connections. It's a visual map of your patch.

#### B2: Engine Detail (slot selected)

| Feature | Location | Size |
|---------|----------|------|
| Engine nameplate + creature | Top bar | 560×40pt |
| **Level 1: 4 Macro knobs (large)** | Top section | 4 × 64pt knobs = 280pt + labels |
| **Level 2: Grouped sections** | Scrollable middle | 560×~250pt |
| → OSC section (collapsible) | Section | 3-4 knobs per section |
| → FILTER section (collapsible) | Section | 3-4 knobs |
| → MOD section (collapsible) | Section | 3-4 knobs |
| → FX section (collapsible) | Section | 3-4 knobs |
| → SPECIAL section (engine-specific) | Section | Engine's unique params |
| **Level 3: Full grid** | "Show All" button | Scrollable grid of all params |
| Oscilloscope / Waveform | Top-right corner | 180×60pt |
| ADSR interactive display | Below osc | 180×50pt |

**This solves progressive disclosure.** Default shows 4 macros. Scroll or expand sections for grouped params. "Show All" for the full grid. 3 levels of depth in one scrollable panel.

#### B3: FX Chain (FX mode)

| Feature | Location | Size |
|---------|----------|------|
| 4 FX slot cards (vertical stack) | Left 60% | 4 × ~90pt tall |
| Per-slot: name dropdown, bypass, wet/dry, drag handle | Inside each card | ~340×80pt |
| Serial/Parallel toggle | Top | 100×28pt |
| Master output meter | Bottom | 340×20pt |

#### B4: Chord Machine (if toggled)

| Feature | Location | Size |
|---------|----------|------|
| Chord voicing selector | Top | ~300pt |
| Pattern selector | Middle | ~300pt |
| Note visualization | Bottom | ~300pt |

### COLUMN C: BROWSER/INSPECTOR (300pt × 400pt = 120,000 px²)

Tabs at top: [PRESET] [COUPLE] [FX] [PLAY] [⚙]

#### C1: Preset Browser (default)

| Feature | Location | Size |
|---------|----------|------|
| Search field | Top | 280×36pt |
| Mood filter pills (scrollable) | Below search | 280×28pt |
| Preset list (scrollable) | Main area | 280×~280pt |
| Per-preset: name, mood dot, engine badge, DNA mini hex, ♡ | Per row | 280×48pt |
| SAVE button | Bottom | 280×32pt |
| "Find Similar" DNA button | Below search | 60pt button |

#### C2: Coupling Inspector

| Feature | Location | Size |
|---------|----------|------|
| Gold corridor header | Top | 280×24pt |
| Route list (source→target) | Main | 280×~200pt |
| Per-route: type dropdown, amount knob, depth knob | Per row | 280×60pt |
| Coupling crossfader (A↔B) | Middle | 280×32pt |
| [+] Add route / [CLR] Clear | Bottom | 280×28pt |

#### C3: FX Inspector (per-slot deep params)

| Feature | Location | Size |
|---------|----------|------|
| FX name header | Top | 280×28pt |
| Per-FX parameter knobs | Main | 280×~300pt |

#### C4: Play/Expression (when sidebar-only mode)

| Feature | Location | Size |
|---------|----------|------|
| Expression controllers (vertical stack) | Main | 280×~360pt |
| Used when PlaySurface is hidden (small window mode) | | |

#### C5: Settings

| Feature | Location | Size |
|---------|----------|------|
| Audio device | Section | |
| MIDI config | Section | |
| Buffer size | Section | |
| Theme toggle | Section | |
| About / Cultural acknowledgment | Section | |

### ZONE D: PLAYSURFACE (1100pt × 220pt = 242,000 px²)

| Feature | Location | Size |
|---------|----------|------|
| Surface tab bar (XOUIJA/MPC/KEYS) | Top | 1100×28pt |
| Active surface | Left ~880pt | 880×192pt |
| Expression controllers | Right ~220pt | 220×192pt |
| → Mod Wheel | Expression panel | 28×120pt |
| → Pitch Bend | Expression panel | 28×120pt |
| → 4 Macro Strips | Expression panel | 4×20pt × 120pt |
| → Tide Controller | Expression panel | 100×100pt |

### STATUS BAR (1100pt × 28pt = 30,800 px²)

| Feature | Location | Size |
|---------|----------|------|
| FIRE/XOSEND/ECHO/PANIC triggers | Left | 4 × 28pt buttons |
| BPM display | Center-left | 60pt |
| Voice count | Center | 60pt |
| CPU meter | Center-right | 40pt |
| Engine slot indicator dots (4) | Right | 4 × 12pt dots |
| Performance Lock toggle | Far right | 28pt button |

---

## 5. Component Taxonomy (Reusable Library)

### Engine Classification (73 engines audited)

| Tier | Count | Description |
|------|-------|-------------|
| **STANDARD** | 30 | Generic knob grid sufficient. No custom UI needed. |
| **SEMI-CUSTOM** | 26 | Standard grid + 1-2 added controls (mode selector, indicator) |
| **CUSTOM** | 17 | Requires dedicated non-grid UI panel or specialized component |

### 7 Shared Component Types (Build Once, Reuse Across Fleet)

| # | Component | Engines That Need It | Customization Points |
|---|-----------|---------------------|---------------------|
| 1 | **DrumPadGrid** (8-voice, GM positions, per-voice algo/mode) | ONSET (8 voices × Circuit+Algorithm), OFFERING (8 voices × 5 cities) | Voice names, mode options per voice, color per voice |
| 2 | **NamedModeSelector** (pill strip or card selector for named modes) | ~20 engines: OCELOT (biomes), OVERWORLD (ERA vertices), ORACLE (maqam), OSPREY/OSTERIA (shore), OUROBOROS (topology), ORPHICA (grain mode), ORIGAMI (spectral ops), OVERLAP/ORBWEAVE (knot), OTO/OCTAVE/OLEG/OCHRE/OBELISK/OPALINE/OLATE/OAKEN (instrument models), OFFERING (cities), OVERCAST (transition modes) | Label text, icon per option, option count (2-70+), searchable vs static |
| 3 | **AccumulationMeter** (session-age / pressure / state meter with optional reset) | OVERWORN (irreversible 30min+), OVERFLOW (pressure gauge), OVERCAST (crystallization), OMBRE (memory decay), ORGANON (metabolic energy) | Direction (building vs decaying), danger zone threshold, reset button yes/no, time scale |
| 4 | **SpectralFingerprintIndicator** (5th-slot coupling status) | OASIS, ODDFELLOW, ONKOLO, OPCODE (all 4 FUSION quad engines) | Engine accent color, fingerprint active/inactive badge |
| 5 | **AnalysisEngineMeter** (spectrum/band display + output meters) | OPTIC (spectrum + 8 mod outputs), OSMOSIS (4-band RMS + pitch detect) | Band count, output channel count, visualization mode |
| 6 | **TopologySelector** (4-node graph with named topology options) | OVERLAP (Unknot/Trefoil/Figure-Eight/Torus), ORBWEAVE (same 4), OUROBOROS (Lorenz/Rossler/Chua/Aizawa) | Topology names, visual preview per topology, morphing position indicator |
| 7 | **TemporalDriftIndicator** (time-domain timbral evolution display) | ORCHARD (growth mode), OVERGROW (harmonic evolution), OLATE (fermentation), OMEGA (distillation), OAKEN (curing), OVERWORN (session reduction), OVERCAST (crystallization) | Time scale (seconds vs minutes), direction (growing vs decaying), visual metaphor |

### Additional Specialized Components (1-2 engines each)

| Component | Engine(s) | Description |
|-----------|----------|-------------|
| **TriangleXYPad** | OVERWORLD (ERA), OXYTOCIN (I/P/C love triangle) | Triangular/barycentric 2D pad with labeled vertices |
| **BipolarAxisBar** | OBESE (Mojo analog↔digital), OUIE (HAMMER STRIFE↔LOVE), OWARE (material continuum) | Horizontal bar with labeled endpoints and center indicator |
| **ConductorArcDisplay** | OPERA | Arc shape preview + progress bar + mode toggle |
| **CellularAutomataGrid** | ORGANISM (16-cell CA), OUTWIT (8-arm Wolfram rules) | Cell state display + rule selector + generation history |
| **DrawbarFaderBank** | OTIS | 9 vertical drawbar faders (Hammond paradigm) |
| **ModularSlotFlow** | OBRIX | Signal-flow diagram with typed source/filter/shaper slots |
| **DualVoiceAlgoPanel** | OUIE | Side-by-side 8-algorithm selectors with central HAMMER axis |
| **SeatRingSequencer** | OSTINATO | 8-seat circular layout with 16-step pattern per seat |
| **CrossFeedMatrix** | OCELOT | 4×3 visual grid (strata × route type) with per-cell amount |
| **5MacroDisplay** | OVERBITE | 5-macro layout (BELLY/BITE/SCURRY/TRASH/PLAY DEAD) instead of standard 4 |

### How the SPECIAL Section Works

Every engine's Column B detail view follows this template:
```
[Engine Nameplate + Creature]
[MacroKnobCluster — always 4 macros (or 5 for OVERBITE)]
[CollapsibleSection: OSC]
[CollapsibleSection: FILTER]
[CollapsibleSection: MOD]
[CollapsibleSection: FX]
[CollapsibleSection: SPECIAL ← engine-specific components go HERE]
[WaveformView + ADSRDisplay]
```

For STANDARD engines (30): SPECIAL section has additional knobs only.
For SEMI-CUSTOM engines (26): SPECIAL section contains one shared component (NamedModeSelector, BipolarAxisBar, etc.).
For CUSTOM engines (17): SPECIAL section is larger and may contain multiple specialized components or replace the standard layout entirely (OBRIX, OSTINATO, OTIS).

### Preliminary Component Types (from feature analysis)

| Component Type | Description | Estimated Engine Count | Customization Points |
|---------------|-------------|----------------------|---------------------|
| **RotaryKnob** | Standard filmstrip rotary with arc overlay | ALL 73 | Size (36/48/64pt), accent color, filmstrip set, label |
| **MiniKnob** | Compact knob for grids | ALL 73 (L3 view) | Size (24pt), accent color |
| **CollapsibleSection** | Header + expandable knob group | ALL 73 | Section name, color, knob count, hero params |
| **ModeSelector** | Grid or dropdown of named modes | ~15-20 engines | Mode count, labels, icons, grid dimensions |
| **PatternGrid** | Step sequencer / rhythm pattern display | ~3-5 engines | Step count, voice count, pattern bank size |
| **XYPad** | 2D controller with physics | ~5-8 engines | Axis labels, physics mode, trail rendering |
| **TopologyDisplay** | Node + arc graph visualization | ~3-5 engines | Node count, arc types, layout algorithm |
| **WaveformView** | Oscilloscope / spectrum display | ALL 73 | Render mode (scope/spectrum/lissajous), color |
| **ADSRDisplay** | Interactive envelope editor | ~50+ engines | Point count (ADSR vs AD vs AHDSR), curve type |
| **VoiceModeSelector** | Per-voice configuration | ~10-15 engines | Voice count, mode names |
| **MacroKnobCluster** | 4 large knobs for CHARACTER/MOVEMENT/COUPLING/SPACE | ALL 73 | Always the same layout, only accent color changes |
| **CreaturePortrait** | Engine identity icon (pixel art or 3D) | ALL 73 | Asset path, accent tint |
| **CouplingArc** | Bezier curve between two engine nodes | Coupling system | Source/target colors, type stroke pattern, RMS animation |
| **PresetCard** | Name + mood + engine + DNA hex + favorite | Preset browser | 48pt or 72pt row height |
| **FXSlotCard** | Name + bypass + wet/dry + drag handle | FX chain | FX-specific deep param set |

### Build Once, Customize Per Engine

Every engine's detail view (Column B2) is composed from these shared components:
```
EngineDetailView = {
  EngineNameplate (name + creature + accent stripe)
  MacroKnobCluster (always 4 macros)
  CollapsibleSection[OSC] (RotaryKnob × N)
  CollapsibleSection[FILTER] (RotaryKnob × N)
  CollapsibleSection[MOD] (RotaryKnob × N)
  CollapsibleSection[FX] (RotaryKnob × N)
  CollapsibleSection[SPECIAL] (mixed components — engine-specific)
  WaveformView
  ADSRDisplay
}
```

The SPECIAL section is where engine personality lives. Standard engines: just more knobs. OBRIX: brick ecology display. OPERA: conductor arc. OFFERING: city mode selector. Same container, different content.

---

## 6. Engine-Specific UI Requirements

### Full Audit Complete (73 engines read)

**17 CUSTOM engines** — each needs at least one bespoke panel in the SPECIAL section:

| Engine | Param Count | Key Custom UI Need |
|--------|------------|-------------------|
| OBRIX | 81 | Modular slot-flow diagram + ecology state + environmental panel |
| ONSET | 111 | 8-voice drum grid + per-voice algo selector + XVC matrix |
| OSTINATO | ~55 | 8-seat ring sequencer + 16-step pattern grid + instrument/pattern selectors |
| OPERA | 45 | Conductor arc display + Kuramoto R meter + 3-timescale indicator |
| OFFERING | 84 | 8-voice drum grid + 5-city selector + curiosity curve display |
| OXYTOCIN | 29 | I/P/C love triangle + circuit topology selector + bond depth meter |
| OPAL | 87 | FREEZE toggle + 6-slot mod matrix table + grain cloud display |
| OCELOT | ~40 | 4-stratum ecosystem + 12-route cross-feed matrix + biome selector |
| OUTWIT | ~45 | 8-arm Wolfram rule selector grid + CA state display |
| OUROBOROS | ~30 | Attractor topology selector + leash slider + phase portrait |
| ORBITAL | 32 | Spectral partial display + vowel formant selector |
| ORGANON | 10 | Metabolic state display + energy pool meter |
| OPTIC | ~18 | Spectrum analyzer + 8 modulation output meters (zero-audio engine) |
| ORACLE | 29 | Maqam scale selector (70+ modes) + GRAVITY bipolar + GENDY display |
| OSMOSIS | ~20 | External audio routing display + 4-band energy meters |
| OUIE | ~28 | Dual-voice algo selector × 2 + HAMMER axis + interval display |
| OTIS | 36 | 9 vertical drawbar faders + Leslie toggle + percussion toggle |
| OVERWORN | ~28 | Session age timeline (30+ min) + reduction state + "Start Fresh" reset |
| OVERFLOW | ~28 | Pressure gauge + valve release mode selector + over-pressure indicator |
| ORGANISM | ~30 | 16-cell CA state display + rule selector + generation history |
| OVERWORLD | ~38 | ERA triangle 2D pad + chip vertex selector |

**26 SEMI-CUSTOM engines** — standard grid + 1-2 shared components from the taxonomy:
ODDOSCAR, OBESE, OVERBITE, ORIGAMI, OSPREY, OSTERIA, OHM, ORPHICA, OVERLAP, ORBWEAVE, OVERTONE, OMBRE, ORCA, OCTOPUS, OPENSKY, OWARE, OTO, OCTAVE, OLEG, OCHRE, OBELISK, OPALINE, OLATE, OAKEN, ORCHARD, OSIER, OVERWASH, OVERCAST, OASIS, ODDFELLOW, ONKOLO, OPCODE

**30 STANDARD engines** — generic knob grid with color-coded sections is sufficient:
ODDFELIX, OVERDUB, ODYSSEY, OBLONG, OBSIDIAN, OBSCURA, OCEANIC, OBLIQUE, OWLFISH, OBBLIGATO, OTTONI, OLE, OXBOW, OGRE, OMEGA, OVERGROW, OXALIS, OCEANDEEP, OVEN, OUTLOOK, and others

### Critical Architectural Insight

Three cross-cutting patterns emerged that require new UI primitives:

1. **Accumulator engines** (OVERWORN, OVERFLOW, ORGANISM, OUROBOROS, OPTIC, OSMOSIS) need live readout of internal state machines beyond APVTS params → engine interface needs a state-publishing contract
2. **Mode-typed slot engines** (OBRIX, ONSET voice configs, OPAL mod matrix) have deep param counts because each slot multiplies params → need hierarchical reveal (show per-slot controls only for active slot type)
3. **Temporal drift engines** (ORCHARD, OVERGROW, OLATE, OMEGA, OAKEN, OVERWORN, OVERCAST) where sound changes over 5-30+ seconds → all need the same TemporalDriftIndicator primitive

---

## 7. OBRIX Special Treatment

OBRIX is fundamentally different from every other engine. It's a **modular construction system** — you build sounds by assembling "bricks" (sources, filters, shapers). Other engines are instruments you play. OBRIX is an instrument you *build* then play.

### What OBRIX Needs That Others Don't

1. **Brick Palette** — a selector showing available brick types (src1-src2 types, filter types, shaper types)
2. **Brick Stack View** — a visual representation of the current brick arrangement (which sources feed which filters feed which output)
3. **Per-Brick Parameters** — each brick has its own params (unlike other engines where params are flat)
4. **Reef Ecology Display** — shows Competition/Symbiosis/Parasitism/Stress/Bleach state
5. **Wave system indicator** — shows which Wave (1-5) features are active

### OBRIX in Column B (Engine Detail)

When OBRIX is selected, Column B transforms:
```
┌─────────────────────────────────────────┐
│ OBRIX — Reef Architect                  │
│ [CHAR] [MOVE] [COUP] [SPACE] macros    │
├─────────────────────────────────────────┤
│ BRICK STACK                             │
│ ┌────────┐  ┌────────┐                 │
│ │ SRC 1  │→ │ FILTER │→ [OUTPUT]       │
│ │ SAW    │  │ LP 24  │                 │
│ └────────┘  └────────┘                 │
│ ┌────────┐       ↑                     │
│ │ SRC 2  │───────┘                     │
│ │ NOISE  │  [+ ADD BRICK]             │
│ └────────┘                             │
├─────────────────────────────────────────┤
│ ▸ OSCILLATOR (src1Type, src2Type, tune)│
│ ▸ FILTER (cutoff, reso, envAmt)       │
│ ▸ ECOLOGY (field, temp, stress, bleach)│
│ ▸ REEF RESIDENCY (mode, strength)     │
├─────────────────────────────────────────┤
│ [Waveform]                    [ADSR]   │
└─────────────────────────────────────────┘
```

### OBRIX Pocket (Standalone App)

The TE Pocket Operator-inspired OBRIX Pocket is a **separate product** — a standalone iPhone/iPad app. It doesn't need to fit inside the XOlokun plugin window. It gets its own layout:

- **iPhone portrait:** 4 large knobs (TE PO-style) + brick palette at top + minimal visualization
- **iPhone landscape:** Full brick stack view + 8 knobs
- **iPad:** Full OBRIX experience with brick drag-and-drop

This is a separate design project. The XOlokun plugin needs to support OBRIX well, but the Pocket is an independent product with its own constraints.

---

## 8. Extensibility Schema

### How Unknown Future Features Get Incorporated

Every feature in XOlokun can be classified into one of 6 **Feature Shapes**:

| Shape | Description | UI Home | Example |
|-------|------------|---------|---------|
| **Parameter** | A single controllable value (0-1 or enum) | Knob/toggle in engine detail section | Any new engine param |
| **Panel** | A complex interactive surface | Column B canvas (new tab/view) | Constellation View, Spatial Navigation |
| **Inspector** | A configuration/browsing interface | Column C (new tab) | New preset browser mode, MIDI learn panel |
| **Strip** | A persistent thin display | Between zones (collapsible) | Coupling arc strip, performance strip |
| **Overlay** | A temporary full-window modal | Over entire window, 300ms fade | Constellation View, About, first-launch |
| **Dialog** | A popup for a specific task | CallOutBox from trigger button | Export dialog, Save As, Settings sub-page |

### Adding a New Feature

When a new feature is proposed, classify its Shape, then it immediately has a home:

1. **New Parameter:** Goes into the relevant CollapsibleSection in Column B2. If it doesn't fit existing sections, it goes in SPECIAL.
2. **New Panel:** Gets a new view mode for Column B. Add a tab/button to access it.
3. **New Inspector:** Gets a new tab in Column C's tab bar.
4. **New Strip:** Gets a collapsible zone between existing zones (like the coupling arc strip).
5. **New Overlay:** Triggered by a button/shortcut, appears over the entire window.
6. **New Dialog:** Launched as CallOutBox from a button, self-contained.

### Future-Proofing Rules

1. **Column B is infinitely extensible** — it's contextual. Any new view mode is just another B-mode.
2. **Column C tabs are extensible** — add new inspector tabs without affecting existing ones.
3. **PlaySurface tabs are extensible** — new playing surfaces (MPE, drum grid variants) = new tabs.
4. **Status bar has ~300pt of free space** on the right for new indicators/buttons.
5. **Engine SPECIAL sections are per-engine** — any engine can declare unique components.
6. **FX chain slots can host new FX types** — FX selector dropdown is extensible.
7. **The Overlay system** handles anything that needs temporary full-screen access.

### What This Means for 2027+ Features

| Potential Future Feature | Shape | Home |
|--------------------------|-------|------|
| Wavetable editor | Panel | Column B (new view mode) |
| Sample editor | Panel | Column B (new view mode) |
| Modulation matrix | Panel or Inspector | Column B or C tab |
| MIDI learn interface | Inspector | Column C tab |
| Hardware controller mapping | Dialog | CallOutBox from settings |
| Community preset sharing | Inspector | Column C tab |
| AI preset generation | Inspector or Dialog | Column C or CallOutBox |
| Visual coding / patch editor | Panel | Column B (new view mode) |
| Live set list manager | Panel or Inspector | Column B or C |
| Recording/bounce | Dialog | CallOutBox from status bar |

None of these require restructuring the layout. They all fit the existing 3-column architecture.

---

## 9. Interaction Flow Maps

### Flow 1: New User First Experience
```
OPEN PLUGIN
  → See Overview (coupling graph) + 4 engine slots + Sound on First Launch
  → Hover controls → hear response
  → Click SLOT 1 → Column B shows Engine Detail (4 macro knobs)
  → Turn a macro → hear change
  → Click ← preset arrow → new sound loads
  → Click SLOT 2 → Column B shows second engine
  → Notice coupling arc in overview → click COUPLE tab → see how they connect
  → Click MPC tab in PlaySurface → play pads
```
**0 clicks to hear sound. 1 click to see params. 1 click to browse presets.**

### Flow 2: Sound Designer Session
```
CLICK SLOT 1 → Engine Detail in Column B
  → Expand FILTER section → tweak cutoff, resonance
  → Expand MOD section → adjust LFO rate
  → Click SLOT 2 → detail switches to engine 2
  → Configure coupling: click COUPLE tab in Column C
  → Select coupling type, set amount
  → Click PRESET tab → save preset
  → Click FX in Column C → adjust per-engine FX
```

### Flow 3: Live Performance
```
CLICK PlaySurface XOUIJA tab
  → Dark Cockpit activates: everything dims except active surface
  → Play notes on XOuija → Planchette tracks, trails glow
  → Grab macro strip → timbral shift
  → Coupling crossfader → morph between engines
  → Hit FIRE/PANIC pads in status bar
```

### Flow 4: OBRIX Brick Building
```
CLICK SLOT with OBRIX loaded → Column B shows Brick Stack View
  → See current brick arrangement
  → Click SRC 1 brick → expand inline to show src1Type, tune, etc.
  → Click [+ ADD BRICK] → brick palette appears
  → Drag brick into stack → new source/filter/shaper added
  → Expand ECOLOGY section → adjust environmental params
  → Switch to L2/L3 for full parameter access
```

---

## 10. What We Learn from Vital/Serum/Pigments

### Vital's Strengths (that we should adopt)
- **Generous knob spacing** — ~60pt center-to-center for main knobs. Our V0.2 had 42pt = too tight.
- **Visual feedback on every control** — arcs, fills, modulation indicators. Never a mystery what a knob does.
- **Persistent wavetable display** — the oscillator visualization is always visible, not hidden behind a tab.
- **Modulation drag-to-assign** — dragging from a mod source to any param is intuitive.

### Serum's Strengths
- **Clear separation between sections** — OSC A / OSC B / Filter / FX are visually distinct zones with borders.
- **Tab system for deep features** — the matrix tab doesn't clutter the main view.
- **Wavetable as hero element** — the waveform IS the identity of the sound.

### Pigments's Strengths
- **Full-width views per mode** — when you're in the sequencer, you get the WHOLE screen.
- **Macro page as landing page** — 8 macros up front, everything else behind tabs.

### What None of Them Do (our advantage)
- No synth shows 4 sources simultaneously in a persistent rack
- No synth has coupling between sources as a first-class visual element
- No synth changes its entire color identity based on which engine is active
- No synth has an autonomous UI element (Planchette) that moves on its own

### Lessons Applied to Our Layout
1. **Knob spacing:** Minimum 56pt center-to-center for main knobs, 40pt for section knobs, 28pt for detail grid
2. **Section borders:** Each CollapsibleSection has a subtle top border + color accent dot (already implemented in code)
3. **Hero visualization:** The Coupling Visualizer in the Overview IS our "wavetable display" — it's the hero visual that defines XOlokun's identity
4. **Persistent overview:** The engine rack (Column A) is always visible, like Vital's oscillator display

---

## 11. Decisions (Resolved 2026-03-24)

All 7 questions resolved by user. These are now locked design decisions.

| Q | Decision | Rationale |
|---|----------|-----------|
| **Q1** | **B: 260/520/320** | Wider rack gives engine tiles room to breathe. Wider browser/inspector gives preset list + coupling config adequate space. Ableton-style balanced proportions. |
| **Q2** | **C: Collapsible, default hidden. Toggle to lock open at 220pt** | Maximizes column height (400pt → 620pt) when not performing. Toggle button locks PlaySurface open for performance sessions. |
| **Q3** | **Study competitors** | Research Vital/Serum/Pigments slot tiles and pick the best pattern. Avoid guessing — bring evidence. |
| **Q4** | **User-selectable default** | Column B landing page is customizable: coupling graph, engine chain, preset DNA, or blank. Saved in preferences. Different users want different defaults. |
| **Q5** | **C: Both graph + list, with list (Column C) as primary** | Graph in Column B is visual context. List/inspector in Column C is where you actually configure coupling (type dropdown, amount knob, depth knob per route). List is more important than graph for interaction. |
| **Q6** | **A: Include in this document** | OBRIX Pocket shares the Component Taxonomy. Designing it now ensures reusable components and avoids reinventing the wheel. |
| **Q7** | **Study competitors** | Same as Q3 — research how Vital/Serum/Pigments handle FX chains and pick the best pattern. |

### Revised Pixel Math (with decisions applied)

**PlaySurface collapsed (default — editing/browsing mode):**
```
Header:    52pt
Columns:   1100pt wide × 620pt tall (700 - 52 header - 28 status)
  Col A:   260pt (Engine Rack)
  Col B:   520pt (Main Canvas)
  Col C:   320pt (Browser/Inspector)
Status:    28pt
TOTAL:     52 + 620 + 28 = 700pt ✓
```

**PlaySurface expanded (performance mode):**
```
Header:    52pt
Columns:   1100pt wide × 400pt tall (700 - 52 - 220 - 28)
  Col A:   260pt
  Col B:   520pt
  Col C:   320pt
Surface:   220pt
Status:    28pt
TOTAL:     52 + 400 + 220 + 28 = 700pt ✓
```

**Key improvement:** When PlaySurface is collapsed (most of the time during sound design), columns get **620pt of height** instead of 400pt. That's 55% more vertical space for engine parameters, preset browsing, and coupling configuration. The PlaySurface toggles in for performance.

### Remaining Research Tasks

- [ ] **Q3 deep dive:** Study Vital/Serum/Pigments engine slot tile patterns. What does each show in the compact view? How much space per oscillator? Bring recommendation with screenshots/references.
- [ ] **Q7 deep dive:** Study how Vital/Serum/Pigments present FX chains. Inline? Separate page? Tab? How much space? Bring recommendation.
- [ ] **Multi-platform layouts:** Extend this architecture to Standalone Desktop, iPad, and iPhone form factors.
- [ ] **OBRIX Pocket section:** Design the TE PO-inspired standalone app layout using shared Component Taxonomy.

---

## 12. Form Factors (Multi-Platform Architecture)

The 3-column layout (§3) is the **Plugin AU/VST + Standalone Desktop** paradigm. Other form factors adapt the same Component Taxonomy to different spatial constraints.

### 12.1 Plugin AU/VST (PRIMARY — designed above)
- 1100×700pt default, resizable 960×600 → 1920×1080
- 3-column + collapsible PlaySurface + header + status
- Host provides window — no floating windows, no menu bar
- Must respect host resize constraints

### 12.2 Standalone Desktop
- Same 3-column layout as plugin, BUT:
- Own window with native menu bar (File/Edit/View/Help)
- File > Save/Save As/Open for preset management
- View > Toggle PlaySurface / Toggle Column C / Full Screen
- Can be larger than plugin (up to full screen)
- Multiple windows possible (e.g., separate OBRIX Pocket window)
- Drag-and-drop file import for samples (OSMOSIS, Outshine)

### 12.3 iPad (Landscape — primary iOS layout)
- Full screen or Split View (50% screen = ~512pt wide)
- **Single-column with drawers** replacing the 3-column layout:
  - Main: Column B (engine detail or overview) — full screen
  - Left drawer (swipe from left edge): Column A (engine rack) + engine browser
  - Right drawer (swipe from right edge): Column C (presets/coupling/FX/settings)
  - Bottom sheet (swipe up): PlaySurface
- Nav bar (44pt): logo, engine name, preset nav, macros
- Bottom bar (64pt + safe area): 4 macro knobs + trigger pads
- 44pt minimum touch targets everywhere
- Haptic feedback on all interactions

### 12.4 iPad (Portrait)
- Horizontal macro strip (full width, 48pt)
- Bottom tab bar: [Play] [Engine] [Presets] [FX]
- Each tab is a full-screen view of one column's content
- Play tab = PlaySurface (full screen, touch-optimized)

### 12.5 iPhone (Landscape)
- Full-screen PlaySurface only — pure performance mode
- Minimal header overlay: engine name + preset name
- Swipe down: macro strip (4 knobs)

### 12.6 iPhone (Portrait)
- Tab bar at bottom: [Play] [Sound] [Browse] [FX]
- Play: PlaySurface (full screen)
- Sound: 4 large macro knobs (80×80pt) — TE PO-inspired
- Browse: Preset browser (full screen)
- FX: FX chain (full screen)
- **No engine detail editing on iPhone** — too small for 81+ parameters. Sound design happens on desktop/iPad. iPhone is for performance and preset browsing only.

### 12.7 OBRIX Pocket (iPhone standalone app)
- Separate product, TE Pocket Operator-inspired
- **iPhone portrait:** Top: brick palette (scrollable). Center: 4 large TE-style knobs with mode-switching. Bottom: mini pad grid (2×2 or 4×4). Knobs remap per selected brick — same physical gestures, different parameters per mode (TE OP-1 pattern).
- **iPhone landscape:** Full brick stack view + 8 knobs
- **iPad:** Full OBRIX experience with brick drag-and-drop, full parameter view
- Shares Component Taxonomy: RotaryKnob, NamedModeSelector, ModularSlotFlow, CollapsibleSection, CreaturePortrait (OBRIX coral)
- Outputs: audio, MIDI, AUv3, standalone, AirDrop preset sharing

### Cross-Platform Component Sharing

| Component | Plugin/Desktop | iPad | iPhone | OBRIX Pocket |
|-----------|---------------|------|--------|-------------|
| RotaryKnob | 36-64pt | 48-80pt (touch) | 80pt (Sound tab) | 80pt (TE-style) |
| CollapsibleSection | Full | Full (in drawers) | N/A (tabs instead) | N/A |
| NamedModeSelector | Pill strip | Pill strip (larger) | Dropdown | Dropdown |
| WaveformView | 180×60pt | 240×80pt | Hidden | Hidden |
| PresetCard | 48pt rows | 56pt rows | 64pt rows | N/A |
| MacroKnobCluster | 4 × 36pt (header) | 4 × 48pt (bottom) | 4 × 80pt (Sound) | 4 × 80pt |
| PlaySurface | 1100×220pt | Full screen | Full screen (landscape) | Mini 2×2 pad |
| CouplingVisualizer | 520×400pt | Full screen (drawer) | N/A | N/A |

---

*This document is a living architecture blueprint. It evolves until every feature has a verified home across all 4 form factors. Next steps: Q3/Q7 competitor research, then V0.3 wireframe prototype built from this architecture.*
