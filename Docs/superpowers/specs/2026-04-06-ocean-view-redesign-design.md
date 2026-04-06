# XOceanus Ocean View — Full UI Redesign Spec

> **Date:** 2026-04-06
> **Author:** UIX Design Studio + Demolition Teardown
> **Status:** Design approved, pending implementation plan
> **Replaces:** Gallery Model (3-column layout)

---

## 1. Vision

XOceanus is a living ocean. The UI is not a control panel — it is the ocean itself. Engines are creatures in their habitat. Coupling is the water between them. Presets are ecological states. The user doesn't operate a synthesizer; they explore an ecosystem.

**One identity: Ocean.** No more gallery, cockpit, forge, or laboratory. Everything is ocean.

---

## 2. Spatial Layout: Radial Depth

The plugin window is a top-down view of the ocean water column.

### 2.1 Structure

```
┌──────────────────────────────────────────────┐
│              ambient edge glow                │
│  ┌────────────────────────────────────────┐  │
│  │                                        │  │
│  │    [engine]          [engine]          │  │
│  │         ╲               ╱              │  │
│  │          ╲   ┌───────┐ ╱               │  │
│  │           ╲  │ NEXUS │╱                │  │
│  │    ────────▶ │ DNA+  │◀────────        │  │
│  │           ╱  │ Name  │╲                │  │
│  │          ╱   └───────┘ ╲               │  │
│  │         ╱               ╲              │  │
│  │    [engine]          [engine]          │  │
│  │                                        │  │
│  │    ┌──┐ ┌──┐ ┌──┐ ┌──┐               │  │
│  │    CHAR MOVE COUP SPACE  (macros)     │  │
│  └────────────────────────────────────────┘  │
│              ambient edge glow                │
└──────────────────────────────────────────────┘
```

- **Center:** Preset Identity nexus (see §3)
- **Orbiting:** Up to 4 engine creatures at variable distances from center
- **Distance from center:** Depth zone (Sunlit = close, Midnight = far)
- **Coupling threads:** Luminescent lines connecting engines through/around the nexus
- **Background:** Radial gradient from ocean blue (center) to abyssal dark (edges)
- **Edge glow:** Ambient color shifts based on active depth zones (cyan top = Sunlit loaded, violet bottom = Midnight loaded)
- **Macros:** 4 gold knobs anchored below the nexus

### 2.2 Dimensions

- **Minimum:** 960 × 600px (matches current minimum)
- **Default:** 1100 × 750px
- **Maximum:** 1600 × 1000px
- **PlaySurface overlay:** Renders within existing window bounds (architectural change from current 264pt window expansion). Requires rewrite of PlaySurface show/hide to use overlay compositing instead of `setSize()`.

### 2.3 Engine Creature Positioning

Each loaded engine is positioned using polar coordinates from center:

```
angle = slot_index * (2π / num_loaded_engines) + rotation_offset
radius = base_radius + depth_zone_offset
```

- **Sunlit engines:** radius = 30% of min(width, height)
- **Twilight engines:** radius = 45%
- **Midnight engines:** radius = 60%

Up to **5** engine creatures orbit (4 primary slots + Ghost Slot when active). Creatures are `64–80px` diameter circles containing:
- Procedural creature sprite (or PNG when available)
- Engine name below in Overbit 12px
- Accent-colored border ring
- Breath animation (scale oscillation at voice activity rate)
- Coupling lean (directional tilt based on active coupling routes)

### 2.4 Coupling Visualization

Coupling routes render as the **ocean substrate** — always visible, not a sidebar tab.

- Each active route: a curved Bézier line between the two engine creatures
- **Stroke width:** Proportional to coupling amount (1px at 10%, 4px at 100%)
- **Color:** Coupling type color from `CouplingColors.h`
- **Animation:** Pulse opacity at the coupling rate (if applicable), particle flow along the line
- **Interaction:** Click a coupling thread → expand inline to show type + amount slider
- **Empty state:** No routes = faint concentric depth-zone rings visible as background texture

---

## 3. Center Nexus: Preset Identity

The center of the radial view is the patch itself.

### 3.1 Components

- **DNA Hexagon:** 64px, always visible, with 6 labeled axes (brightness, warmth, movement, density, space, aggression). Filled shape shows the current preset's DNA profile. Guide hexagon at 15% opacity.
- **Preset Name:** Below the hexagon, 18px Space Grotesk Bold, XO Gold color, with text shadow glow.
- **Mood Badge:** Inline pill below the name. Mood color background at 12% alpha, mood name in mood color.
- **Click behavior:** Click preset name → opens DNA Ocean Floor browser (§5). Click DNA hexagon → cycles through axis pair projections.

### 3.2 Macros

4 macro knobs (CHARACTER, MOVEMENT, COUPLING, SPACE) positioned in a horizontal row below the nexus.

- **Size:** 44px diameter (WCAG compliant)
- **Color:** XO Gold arcs and labels
- **Modulation arcs:** Show coupling-driven modulation depth as colored arc segments
- **Master Volume:** 5th knob, slightly smaller (36px), to the right of SPACE

---

## 4. Engine Interaction: Two-Level Dive

### 4.1 Level 1 — Zoom-In (Snorkeling)

**Trigger:** Single click on an engine creature.

**Behavior:**
- Selected engine smoothly scales to ~120px and moves to center-ish position
- 4 macros + 4–8 "hero" parameters orbit it as a ring (radius = 100px from engine center)
- Other engines shrink to 32px and migrate to edges (but remain visible and clickable)
- Coupling threads stretch but stay visible
- Background tints with the selected engine's accent color at 3% alpha
- The nexus (preset name + DNA) shifts upward above the selected engine

**Hero parameter selection:** No engine currently defines hero parameters — this is a new API. Options: (a) add `getHeroParams()` virtual method to `SynthEngine` interface, (b) create a static lookup table in `EngineVocabulary.h`, (c) use the first 4 parameters from each signal-flow section (SRC1, FILTER, SHAPER, FX = 4 sections × 1 param = 4 hero params minimum). Fallback: first 8 parameters from the parameter layout.

> **Implementation note:** Option (b) is lowest-risk. A `heroParamsFor(engineId)` function returning `std::vector<juce::String>` of parameter IDs, defaulting to first 8 if not in the table.

**Exit:** Click empty background, press Escape, or click another engine.

### 4.2 Level 2 — Split Transform (Scuba Dive)

**Trigger:** Double-click an engine creature, or click "dive deeper" affordance.

**Behavior:**
- The radial view compresses to a vertical strip on the left (20% width)
  - Other engines shown as a mini orbital stack
  - Coupling threads cross the divide into the parameter area
- The remaining 80% becomes the parameter editing area:
  - Engine identity header (creature icon 28px + name + archetype tagline)
  - Signal flow breadcrumb: `SRC1 → SRC2 → FILTER → SHAPER → FX → OUT` (clickable sections)
  - Parameter grid organized by signal flow section
  - ADSR display (live, 30Hz) + Waveform display side by side at bottom
- Background: engine accent gradient at 5% alpha

**Exit:** Click the mini orbital strip, press Escape, or click another engine in the strip.

---

## 5. Preset Browser: DNA Ocean Floor

**Trigger:** Click the preset name in the nexus, press `P`, or use Cmd+F.

**Behavior:** Replaces the radial view with a full-window 2D scatter map.

### 5.1 Map Structure

- **X axis:** Brightness (left = dark, right = bright)
- **Y axis:** Warmth (top = cool, bottom = warm)
- These two axes are the default projection. Click DNA hexagon axes to switch projection (e.g., movement × density).
- **Each preset:** A colored dot (3–5px radius), colored by mood category
- **Mood territories:** Large, faint mood names rendered as region labels where clusters naturally form
- **Active preset:** Glows gold, larger (10px), with name label
- **Nearby presets:** Shimmer brighter based on DNA distance from active

### 5.2 Interaction

- **Pan:** Click and drag the map
- **Zoom:** Scroll wheel or pinch. Zooming in reveals preset names next to dots.
- **Hover:** Shows preset name + engine tags + mini DNA hex in a tooltip
- **Click:** Loads the preset. Engine creatures in the radial view update. Smooth transition back to radial view.
- **Search:** Floating search bar at top. Typing dims all non-matching presets. Matches brighten.
- **Mood filter:** Floating mood pills (compact). Toggling a mood dims all other moods' dots.
- **Favorites:** Gold star dots are always slightly larger. Filter toggle: "★ only"

### 5.3 Performance

- 19,574 dots rendered via a `juce::OpenGLContext`-backed canvas or a pre-rendered `juce::Image` that updates on filter/zoom changes
- Spatial index (quadtree or grid) for hover hit-testing
- At maximum zoom-out, dots merge into density heatmap blobs per mood
- At close zoom, individual dots with names are visible

### 5.4 Exit

Click background, press Escape, or press `P` again → smooth transition back to radial orbital view.

---

## 6. PlaySurface: On-Demand Overlay

**Trigger:** Press any MIDI key (auto-appears), click KEYS button, or press `K`.

### 6.1 Behavior

- Slides up from bottom with 200ms ease-out animation (respects reduced motion: instant)
- Frosted glass panel: `backdrop-filter: blur(40px)`, 92% opacity dark background
- Rounded top corners (16px radius)
- Drag handle at top center (40×4px pill)
- Ocean view dims to 35% opacity behind

### 6.2 Content

- **Mode tabs:** KEYS | PADS | DRUM | FRETLESS (top-left)
- **XOuija mini controller:** 36px circle (top-right), click to expand to full XOuija panel
- **Keyboard (KEYS mode):** Full-width, 140px tall keys with:
  - Y-gradient velocity hint (top = loud, bottom = soft)
  - In-key highlighting from XOuija harmonic field (gold overlay)
  - Root note indicator
  - Pitch bend hint strips between octave groups
- **Pads (PAD/DRUM mode):** 4×4 grid, each pad 60×60px minimum
  - PAD mode: scale-degree labels, harmonic glow from XOuija
  - DRUM mode: MPC drum names, category-colored borders (kick=red, hat=cyan)
- **XOuija expanded:** When clicked, XOuija panel expands to fill left 30% of overlay
  - "← KEY SELECTION →" and "HARMONIC DEPTH ↕" axis labels
  - "DEEP" / "FREE" replacing YES/NO
  - Gesture trail visualization

### 6.3 Auto-Show / Persist

- **First launch:** Auto-shows in KEYS mode
- **Subsequent launches:** Respects persisted visible/hidden state
- **Dismiss:** Drag down, click outside, press `K`, or press Escape
- **MIDI auto-show:** If a MIDI note-on arrives while PlaySurface is hidden, auto-show with 150ms fade

---

## 7. Visual Language

### 7.1 Color System

The dark palette deepens to support the ocean metaphor:

| Token | Value | Usage |
|-------|-------|-------|
| `abyss` | `#04040A` | Deepest background, plugin edges |
| `deep` | `#0A0E18` | Radial view outer edge |
| `twilight` | `#0E1428` | Mid-depth surfaces |
| `shallow` | `#142040` | Near-center surfaces, overlays |
| `surface` | `#1A2848` | Cards, panels, elevated content |
| `foam` | `#E8E4DF` | Primary text (warm white) |
| `salt` | `#9E9B97` | Secondary text |
| `plankton` | `#5E6878` | Tertiary text, disabled |
| `xoGold` | `#E9C46A` | Brand accent (macros, active states, preset name) |
| `biolum` | Per-engine | Engine accent colors (77 implemented, 101 fleet design) |

> **Note:** 8 of 9 ocean tokens (`abyss` through `plankton`) must be added to `GalleryColors.h`. The existing `surface` token value `#1A1A1C` conflicts with the ocean `surface` value `#1A2848` — the ocean version replaces it.

### 7.2 Depth-Zone Gradients

- **Sunlit zone:** Engines at this depth get warm cyan tinting (`#48CAE4` at 4%)
- **Twilight zone:** Blue tinting (`#0096C7` at 3%)
- **Midnight zone:** Violet tinting (`#7B2FBE` at 4%)

The radial background gradient transitions through these zones concentrically.

### 7.3 Typography

| Role | Typeface | Size Range |
|------|----------|------------|
| Engine names | Overbit Regular | 12–20px |
| Display / headers | Space Grotesk Bold | 14–22px |
| Body / labels | Inter Regular/Medium | 9–14px (floor: 9px) |
| Values / mono | JetBrains Mono | 9–12px |

Minimum font size: **9px** (body), **12px** (Overbit — below 12 falls back to Space Grotesk).

### 7.4 Knobs

Redesigned from the Gallery Model:

- **Body:** Engine accent color at 8% saturation (not neutral gray) — each engine's knobs have a warm tint
- **Arc track:** 4px width, `plankton` color
- **Arc fill:** 4px width, engine accent color
- **Pointer:** Visible center-to-arc line (1.5px, accent color at 80%) — the primary value indicator
- **Modulation arc:** 3px, coupling type color at 50% alpha
- **Hover:** Accent color glow ring at 15% alpha
- **Size:** 44px standard, 36px compact, 28px minimum (no knobs below 28px)

### 7.5 Animations

All animations respect `A11y::prefersReducedMotion()`:

| Animation | Duration | Easing | Reduced Motion |
|-----------|----------|--------|---------------|
| Engine zoom-in | 250ms | ease-out | instant |
| Split transform | 300ms | ease-in-out | instant |
| PlaySurface slide | 200ms | ease-out | instant |
| Browser open/close | 250ms | ease-out | instant |
| Coupling pulse | continuous, 2-4s period | sine | static at 50% |
| Creature breath | continuous, 3-6s period | sine | static at 1.0 |
| Engine switch | 150ms crossfade | linear | instant |

---

## 8. Creatures

### 8.1 Sprite System

Each engine gets a unique creature at 64–80px. The current `CreatureRenderer` procedural system (4 real creatures + 97 blobs) is replaced with:

**Phase 1 (immediate):** Expand `CreatureRenderer` to 20 unique procedural archetypes covering the most important engines:
- OBRIX: coral branch (exists)
- OPERA: singing fish (exists)
- OXYTOCIN: jellyfish (exists)
- ONSET: pistol shrimp (exists)
- OUROBOROS: ouroboros serpent eating its tail
- ORGANISM: cellular automata grid creature
- OWARE: mancala seed pod with tendrils
- ORBITAL: rotating ring system
- OBSIDIAN: dark crystal formation
- ODYSSEY: explorer fish with compass fin
- OBLONG: elongated blob with curious tentacles
- OPAL: iridescent shell creature
- OVERWORLD: pixelated retro fish
- OXBOW: river loop serpent
- OCELOT: spotted predator fish
- OWLFISH: deep-sea owl-faced fish
- ORCA: stylized killer whale
- OSTINATO: rhythmic pulsing anemone
- OBESE: round pufferfish
- ODDFELIX: neon tetra with glowing stripe

**Phase 2 (later):** Full 77-engine pixel art sprite sheet via the existing `render.py --tiles` pipeline. 32×32px source, rendered at 2x for 64px display. `.xogenome` format for variant cosmetics.

### 8.2 Creature Reactivity

At 64–80px, creatures can communicate engine state:

- **Breath scale:** Oscillates 0.95–1.05 at 0.2Hz idle, faster (0.5Hz) when voices active
- **Coupling lean:** -1 to +1 based on net coupling direction (more sending vs receiving)
- **Eye dilation:** Maps to the loudest voice's velocity
- **Bioluminescent spots:** Pulse at the engine's dominant LFO rate
- **Color intensity:** Maps to the engine's output level (brighter when louder)

---

## 9. Header: Ambient Edge

The traditional toolbar header is abolished. In its place:

### 9.1 Ambient Edge Glow

The plugin window border emits a soft glow that reflects the active depth zones:
- Sunlit engines loaded → top edge glows cyan
- Midnight engines loaded → bottom edge glows violet
- Both → gradient from top to bottom
- No engines → faint gold (XO Gold at 5%)

### 9.2 Persistent Minimal Controls

Floating in the top-left corner (not a bar — positioned over the ocean):
- **Preset name** (clickable → opens DNA browser)
- **< >** preset arrows (flanking the name)
- **★** favorite toggle

Floating in the top-right corner:
- **⚙** settings gear
- **KEYS** button (PlaySurface toggle)

That's it. **7 controls total.** Everything else (engine selection, coupling, FX, export) is accessible through the radial view interactions or contextual menus.

### 9.3 Contextual Appearance

- Hover near top edge → preset controls fade in (normally at 60% opacity)
- Hover near an engine → engine name tooltip appears
- Right-click anywhere → context menu with full feature access

---

## 10. Accessibility

### 10.1 Keyboard Navigation

- `Tab` cycles: nexus → engine 1 → engine 2 → engine 3 → engine 4 → macros → back to nexus
- `Enter` on engine: zoom-in (Level 1)
- `Enter` twice: split transform (Level 2)
- `Escape`: back up one level
- `P`: toggle DNA browser
- `K`: toggle PlaySurface
- `Cmd+Z` / `Cmd+Shift+Z`: undo/redo
- `1-4`: select engine slot directly
- Arrow keys on engine: navigate to adjacent engine

### 10.2 Screen Reader

- All engine creatures: `A11y::setup()` with engine name + archetype + depth zone
- Nexus: "Preset: [name], Mood: [mood], DNA: brightness [X], warmth [Y], ..."
- Coupling threads: "Coupling route: [engine A] to [engine B], type [type], amount [X]%"
- Macro knobs: existing A11y labels preserved

### 10.3 Contrast

- All text meets WCAG AA (4.5:1 normal, 3:1 large)
- `ensureMinContrast()` applied at every text draw site
- Engine accent colors validated against ocean background
- Focus rings: 2px, high-contrast blue, on every interactive element

### 10.4 Reduced Motion

- All continuous animations (creature breath, coupling pulse, bioluminescence) freeze at static midpoint
- All transitions (zoom, split, slide) become instant
- Preference persisted via `A11y::prefersReducedMotion()`

---

## 11. Migration Path

### 11.1 What Gets Deleted

- `ColumnLayoutManager.h` — replaced by radial positioning
- `OverviewPanel.h` — replaced by the nexus + orbital view
- `CompactEngineTile.h` — replaced by orbital creatures
- `SidebarPanel.h` — replaced by contextual interactions
- `HeaderIndicators.h` — replaced by ambient edge glow
- The 3-column resized() logic in `XOceanusEditor.h`

### 11.2 What Gets Kept

- `GalleryLookAndFeel.h` — knob rendering (with redesigned body)
- `GalleryColors.h` — color system (extended with ocean tokens)
- `GalleryKnob.h` — slider + modulation arc system
- `EngineDetailPanel.h` — parameter grid (used in Level 2 split transform)
- `WaveformDisplay.h` — oscilloscope (promoted to visible in creatures + Level 2)
- `ADSRDisplay.h` — now live, used in Level 2
- `PresetBrowserPanel.h` — gutted and rebuilt as DNA map, but preset data structures remain
- `PlaySurface/*.h` — all PlaySurface code (overlay wrapper is new, internals preserved)
- `XOuijaPanel.h` — preserved with new labels
- `DepthZoneDial.h` — may become the "engine browser" affordance in the orbital view
- `CreatureRenderer.h` — expanded from 4 to 20 archetypes
- `DnaHexagon.h` — promoted from 24px header to 64px nexus center
- All `Source/Core/`, `Source/DSP/`, `Source/Engines/` — untouched

### 11.3 New Files

- `Source/UI/Ocean/OceanView.h` — main radial layout + orbital positioning
- `Source/UI/Ocean/OceanBackground.h` — depth gradient + coupling substrate rendering
- `Source/UI/Ocean/EngineOrbit.h` — creature positioning, zoom-in, split-transform transitions
- `Source/UI/Ocean/NexusDisplay.h` — preset identity center (DNA hex + name + mood)
- `Source/UI/Ocean/CouplingSubstrate.h` — always-visible coupling thread rendering
- `Source/UI/Ocean/DnaMapBrowser.h` — 2D scatter plot preset browser
- `Source/UI/Ocean/AmbientEdge.h` — edge glow system
- `Source/UI/Ocean/PlaySurfaceOverlay.h` — frosted glass overlay wrapper

---

## 12. Success Criteria

The redesign is complete when:

1. A first-time user can make sound within 3 seconds of opening the plugin
2. No user needs to read documentation to understand the basic workflow
3. Each engine is visually distinguishable from every other engine at a glance
4. The coupling system is visible without navigating to a secondary panel
5. A YouTube reviewer opens the plugin and says something equivalent to "I've never seen anything like this"
6. The UI passes WCAG AA accessibility audit
7. The plugin runs at 60fps on a 2020 MacBook Air (M1, 8GB)
8. All 19,574 presets are navigable without scrolling a list
9. The emotional response on first open is: **wonder**
