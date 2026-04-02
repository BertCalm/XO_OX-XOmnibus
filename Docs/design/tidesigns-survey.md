# TIDEsigns — Situational Brief
**Author:** Marina, Creative Director, TIDEsigns
**Date:** 2026-03-27
**Subject:** XOceanus JUCE Plugin UI — Phase 1 Survey
**Repo:** `~/Documents/GitHub/XO_OX-XOceanus/`
**Reference mockup:** `Docs/mockups/xoceanus-v05-accurate.html`

---

## 1. What Exists Today

### Component Inventory

The Gallery Model UI is built as a 3-column layout managed by `ColumnLayoutManager.h`. All components live under `Source/UI/Gallery/`. The orchestrating file is `Source/UI/XOceanusEditor.h`.

| Component | File | Size | Status |
|-----------|------|------|--------|
| Main editor + layout | `XOceanusEditor.h` | ~850 lines | Active |
| Column layout math | `ColumnLayoutManager.h` | ~8 KB | Active |
| Engine tile (Col A) | `CompactEngineTile.h` | ~32 KB | Recently reworked |
| Engine detail (Col B) | `EngineDetailPanel.h` | ~21 KB | Active |
| Parameter knob grid | `ParameterGrid.h` | ~32 KB | Active |
| Macro hero strip | `MacroHeroStrip.h` | ~8 KB | Active |
| Master FX strip | `MasterFXSection.h` | ~14 KB | Active |
| Sidebar (Col C) | `SidebarPanel.h` | ~21 KB | Active |
| Status bar | `StatusBar.h` | ~19 KB | Active |
| FX inspector panel | `FXInspectorPanel.h` | ~24 KB | Reworked 2026-03-27 |
| Coupling inspector | `CouplingInspectorPanel.h` | ~38 KB | Reworked 2026-03-27 |
| Export tab panel | `ExportTabPanel.h` | ~18 KB | Tweaked 2026-03-28 |
| LookAndFeel overrides | `GalleryLookAndFeel.h` | ~22 KB | Active |
| Color + font system | `Source/UI/GalleryColors.h` | External to Gallery/ | Active |
| Mini coupling graph | `MiniCouplingGraph.h` | ~12 KB | Active (Col A bottom) |
| Waveform display | `WaveformDisplay.h` | ~11 KB | Active |
| Engine picker popup | `EnginePickerPopup.h` | ~32 KB | Active |
| Coupling arc overlay | `CouplingArcOverlay.h` | ~14 KB | Active |
| ADSR display | (inline in `EngineDetailPanel.h`) | — | Added 2026-03-28 |
| OBRIX detail panel | `ObrixDetailPanel.h` | ~55 KB | Active |
| PlaySurface | `PlaySurface/PlaySurface.h` | External | Floating popup |
| Preset browser | `PresetBrowser/PresetBrowser.h` | External | Active |

Notable: `GalleryColors.h` lives at `Source/UI/GalleryColors.h`, not inside `Gallery/` — it is a parent-level dependency. There is no separate `GalleryColors.h` inside `Gallery/`.

### What Has Changed Recently (since 2026-03-25)

The past two days have seen ~25 targeted UI commits, all under `fix(UI)` or `feat(UI)`. Key changes:

- **Dark mode made default** — `GalleryColors::darkMode()` now returns `true` by default. The CLAUDE.md and brand rules both state light mode is primary, but the code and v05 mockup both use dark mode. This is a brand alignment question that needs resolution.
- **Tile redesign** — porthole circles and creature renderer removed from `CompactEngineTile`. Replaced with a power-button style toggle, larger knob arcs, and a painted waveform. `CreatureState` struct remains as a scaffold.
- **Header decluttered** — macro row removed from Column B (was redundant with `MacroHeroStrip` inside `EngineDetailPanel`). DepthZoneDial and several icon buttons (`CI`, `CM`, `P`, `DK`) are parked off-screen at `y = -100`. Only `PLAY`, `EXPORT`, and gear remain in the right cluster.
- **Status bar trigger pads hidden** — FIRE/XOSEND/ECHO CUT/PANIC buttons exist in code but `setVisible(false)` — moved to PlaySurface popup. Keyboard shortcuts (Z/X/C/V) remain wired.
- **PLAY tab hidden** in SidebarPanel — deemed redundant with PlaySurface popup.
- **Coupling inspector polished** — dark theme fixes, miniViz diagram removed.
- **FX inspector improved** — knob labels added, LED brightness improved.
- **Export tab** — gold toned down, Outshine section hidden.
- **EngineDetailPanel + ParameterGrid** — multiple mockup-match passes.

---

## 2. What the Vision Says (v05 Mockup)

The v05 HTML mockup (`xoceanus-v05-accurate.html`) defines a dark-mode-first plugin at **1100×700 px** with these precise specifications:

### Color System (Dark Mode Canonical)
- Shell: `#0E0E10` (bg)
- Surface layer: `#1A1A1C` (header, status bar bg)
- Elevated cards: `#242426`
- Borders: `rgba(255,255,255,0.07)` (very subtle)
- T1 text: `#F0EDE8`, T2: `#9E9B97`, T3: `#5E5C5A`, T4: `#3A3938`
- XO Gold: `#E9C46A`, Gold dim: `rgba(233,196,106,0.14)`
- Accent (OBRIX Reef Jade): `#1E8B7E` — used as the *demo* accent throughout

### Header (52px)
Left to right: logo SVG (two overlapping circles, teal + gold) → brand text ("XOceanus" / "XO_OX Designs") → ENGINES button → **DepthZoneDial (36×36)** → **5 macro knobs** (CHAR/MOVE/COUP/SPACE/VOL in a grouped pill container, centered, auto-expanding) → right cluster: CI/CM/P/PS/DK icon buttons (24×24) → preset nav (◀ name ▶) → EXPORT button (gold fill, dark text) → A/B toggle → CPU meter → MIDI dot.

The mockup explicitly shows **all 5 icon buttons active** in the header right cluster (CI, CM, P, PS, DK). This conflicts with the current code which parks most of them off-screen.

### Column A (260px) — Engine Rack
Each slot tile has:
- 3px left accent bar (engine color)
- Header: slot number | engine name (accent color) | power button (16×16 circle)
- 4 mini macro knobs (per-engine: e.g., SRC1/FILT/ENV/FX)
- 22px waveform SVG (accent color polyline)
- Footer row: mood dots (left) + FX indicator (right: 3 dots + "FX" label)
- 3px CPU bar

At the bottom of Col A: **coupling-graph** (80px fixed), showing coupling routes as a labeled area.

### Column B (520px) — Main Canvas
Top to bottom:
1. **Signal flow strip** (28px) — clickable blocks: OSC → ENV → FILTER → FX → COUPLING with accent-colored active state
2. **Macro knobs row** (64px) — 4 large macros (CHARACTER/MOVEMENT/COUPLING/SPACE), vertical slider bars + labels
3. **Engine detail area** (flex):
   - Engine nameplate (14px bold, accent color, text-shadow glow, 22-letter spacing)
   - 2px accent gradient bar
   - MacroHeroStrip (4 vertical bars, 60px)
   - WaveformDisplay (72px height, with OSCOPE label)
   - Scrollable ParameterGrid (section headers with color dots, knobs, lazy loading)
4. **MasterFX strip** (68px bottom) — 5-6 FX chips (SAT/DELAY/REVERB/MOD/COMP), each a pill chip with dot + name + wet%

### Column C (320px) — Tabbed Sidebar
- Tab bar: **32px** (mockup), current JUCE code says `kTabBarH = 38px`
- Tab label underline: **accent color** (teal `#1E8B7E`) on active — not XO Gold
- Tabs: PRESET / COUPLE / FX / PLAY / EXPORT / SETTINGS
- PRESET tab: search field → mood pills → preset list (with accent dot, title, mood tag, fave star)
- COUPLE tab: scrollable coupling route cards (elevated bg, from/to labels, type badge in gold, knobs)
- FX tab: scrollable FX slot rows (accordion)
- PLAY tab: centered launch button for PlaySurface
- EXPORT tab: format selector, path, big gold EXPORT button
- SETTINGS tab: toggle rows for theme, MIDI, performance settings

### Status Bar (28px)
Left to right: status dividers and labels (BPM · Voices · CPU) — **no trigger pads**. Right: slot status dots (5×, engine accent color when active) + LK (lock) button.

---

## 3. The Gap — Current vs. Target

### Critical Gaps (visual fidelity blockers)

**G01 — Header: Most icon buttons parked off-screen**
The v05 mockup shows CI/CM/P/PS/DK all present in the header right cluster. Current code parks `cinematicToggleBtn`, `cmToggleBtn`, `perfToggleBtn`, `midiIndicator` at `y = -100`. Only `surfaceToggleBtn` (relabeled "PLAY") and `settingsBtn` (gear) remain visible. The preset nav (◀ name ▶) and A/B toggle also appear hidden. Result: the header right cluster is visually sparse compared to the mockup.

**G02 — Header: DepthZoneDial hidden**
The DepthZoneDial (36×36) appears in the v05 mockup between the ENGINES button and the macro knobs. In `resized()`, it is explicitly parked off-screen: `depthDial.setBounds(0, -100, 0, 0)`. This is a deliberate removal but it was a visible feature in the mockup.

**G03 — Header: Macro knobs group vs. individual macros**
The mockup shows the 4 global macros + VOL grouped inside a single pill container with background `rgba(255,255,255,0.018)` and a border. The JUCE code uses a `MacroSection` component (`macros.setBounds(header.reduced(8, 4))`). Whether the pill container grouping visual is faithfully rendered is unknown without runtime inspection — the JUCE component has no explicit pill container background in the visible code.

**G04 — Column B: Signal flow strip absent**
The v05 mockup shows a 28px signal flow strip at the top of Column B with clickable blocks: OSC → ENV → FILTER → FX → COUPLING. In the JUCE `resized()`, `colBPanel.removeFromTop(kSignalFlowStripH)` allocates this space, but the comment says "Signal flow strip (28px) at top of Column B — painted in paint()." It is a paint-only decoration with no interactive block buttons. The mockup intends this to be clickable navigation. This is a passive decoration vs. active navigation gap.

**G05 — Column B: Macro knobs row removed**
The mockup's Column B shows a 64px macros row (4 vertical slider bars) below the signal flow strip, before the engine detail. The JUCE code explicitly removed this row: `// Macro knobs row removed — redundant with EngineDetailPanel's MacroHeroStrip.` This is a deliberate deviation. The MacroHeroStrip inside EngineDetailPanel provides a similar function, but the visual weight and prominent placement in the mockup differs from a nested strip inside the detail panel.

**G06 — Column C tab bar height mismatch**
The v05 mockup specifies the tab bar at **32px** (`height: 32px`). The JUCE `SidebarPanel.h` header comment says `kTabBarH = 38px`. This is a 6px discrepancy that compresses the content area and makes the tab bar look heavier than intended.

**G07 — Column C tab underline color: Gold vs. Accent**
The v05 mockup uses `border-bottom-color: var(--accent)` (teal `#1E8B7E`) for the active tab underline. The JUCE `SidebarPanel.h` header comment says "Active: full-opacity text + 2px XO Gold underline." XO Gold (`#E9C46A`) differs from the accent teal. A small detail, but inconsistent with the mockup which uses the contextual accent color.

**G08 — MasterFX strip height: 68px vs. actual**
The v05 mockup specifies 68px for the master FX section. `resized()` uses `kMasterFXH` — the constant is not immediately visible in the snippet read, but needs verification against the 68px spec.

**G09 — Preset card: missing secondary metadata row**
The v05 mockup shows each preset card having a `preset-mood-tag` secondary row (mood name in T4 mono below the title). The JUCE `PresetBrowserPanel.h` shows an "engine badge" right-aligned but mood tag below the title is unclear. The accent dot on the left side of the card (7×7) uses the engine accent color in the mockup but is described as mood-color-coded in the JUCE component. Alignment to verify.

**G10 — FieldMap: allocated but sent off-screen**
The FieldMap (80px, bottom of Col B) is in the mockup and allocated in `ColumnLayoutManager`. In `resized()`: `fieldMap.setBounds(0, -200, 0, 0)` — explicitly hidden with the comment "80px reclaimed for parameter sections." This is a deliberate removal with no mockup equivalent.

**G11 — Status bar: trigger pad buttons exist but are hidden**
Current code has FIRE/XOSEND/ECHO CUT/PANIC buttons that are `setVisible(false)`. The v05 mockup's status bar shows **no trigger pads** — instead just BPM/Voices/CPU text and slot dots. This aligns. However the code still creates and hides these widgets — dead weight in the component tree.

**G12 — Tile: CreatureState scaffold retained**
The tile comment says "Porthole and creature painting removed 2026-03-27. Member kept so that any future re-introduction of animated sprites has a ready drop-in home." The `CreatureState` struct remains. The v05 mockup has no porthole or creature. This is code hygiene, not a visual gap.

**G13 — GalleryLookAndFeel targets v04 mockup, not v05**
Line 9 of `GalleryLookAndFeel.h` reads: `// Matched to xoceanus-v04-polished.html prototype exactly.` The canonical reference is now v05. The LookAndFeel reference comment is stale. Whether the v04→v05 delta affects JLAF overrides needs audit.

### Minor Gaps (polish level)

**G14 — Logo SVG: XOceanus circles vs. XO-OX rings**
The v05 mockup shows a specific SVG: two overlapping circles (teal + gold fill + outline) at `cx=9,cy=9,r=7` and `cx=17,cy=17,r=7`. The JUCE editor paints the logo procedurally. Whether it matches this exact SVG geometry needs visual verification.

**G15 — Export button: gold fill with dark text**
The mockup shows the EXPORT button as a gold-filled (`#E9C46A`) button with dark text (`#1A1A1C`). Current code uses `exportBtn` as a `TextButton`. Whether GalleryLookAndFeel renders this correctly (gold fill, not gold outline) needs verification.

**G16 — ParameterGrid lazy-load viewport margin**
The mockup implies a smooth scrolling experience. The JUCE ParameterGrid uses 10Hz timer polling and a 100px pre-load margin. This should be adequate but the jitter between render cycles may cause visible pop-in on fast scrolls.

---

## 4. Surfaces Involved

| Surface | Status | Notes |
|---------|--------|-------|
| **Desktop JUCE AU/VST3** | Primary. Active development. | `Source/UI/` — 40+ components |
| **iOS AUv3** | Secondary. Design deferred. | Full iOS port cancelled. OBRIX Pocket (Swift + C++ core) is separate. |
| **Web (XO-OX.org)** | Reference only. | HTML mockups in `Docs/mockups/` are the design reference, not a deployed web UI. |

The iOS surface is not a concern for this engagement. OBRIX Pocket is a separate Swift project with SpriteKit + JUCE hybrid — its UI is addressed by the ios-optimizer skill, not TIDEsigns.

Web mockups are the *source of truth for the desktop JUCE UI*, not a separate deliverable.

---

## 5. Initial Risk Flags

**R01 — Light vs. Dark Mode Brand Contradiction**
`CLAUDE.md` and `xoceanus_master_specification.md` both declare "light mode is the default." `GalleryColors.h` hardcodes `static bool dark = true`. The v05 mockup is dark-mode-only. This is an unresolved brand contradiction. Resolving it requires a policy decision: is v05 the new canonical brand (dark-first), or must the JUCE default be flipped back to light with dark as a toggle? **Any design work that doesn't resolve this first will be revisited.**

**R02 — GalleryLookAndFeel references v04, not v05**
The JLAF comment says it matches v04. Delta between v04 and v05 is undocumented. Any LookAndFeel override that was tuned to v04 pixel-perfect may have silent regressions against v05. A systematic v04→v05 diff of the HTML mockups should precede any JLAF work.

**R03 — Header right cluster sparseness will be visible**
With CI/CM/P/DK/DepthDial all hidden, the right side of the header will feel empty compared to the mockup. This is the most immediately visible gap. Any screenshot or demo will show this discrepancy.

**R04 — Signal flow strip is paint-only (no interaction)**
The mockup intends OSC → ENV → FILTER → FX → COUPLING as clickable navigation. The current implementation is a painted decoration. If this becomes interactive, it requires architecture (state wiring, callbacks, repaint logic) not just visual work.

**R05 — Dead component weight accumulating**
Multiple widgets are parked off-screen (`y = -100`) rather than conditionally created. `CreatureState` struct retained. Hidden trigger pad buttons in StatusBar. This is manageable now but will slow build times and increase cognitive load as the file grows.

**R06 — 73 engine accent colors, but only one accent shown in mockup**
The v05 mockup uses OBRIX teal (`#1E8B7E`) as the demo accent throughout. The real plugin must dynamically set the accent based on which engine is selected. The "active tab underline uses accent color" in v05 means the tab bar underline should change color with engine selection. Whether this is desirable UX (potentially jarring when switching engines) or should be fixed to XO Gold needs a design decision.

**R07 — ObrixDetailPanel at 55KB is an anomaly**
`ObrixDetailPanel.h` is 55KB — 2.5x larger than the next largest component. This suggests OBRIX's UI is substantially custom. Any design system work must account for this one-off surface. It may not inherit standard ParameterGrid patterns.

---

## 6. Recommended Phase 2 Focus

### Specialist 1 — Header & Navigation Fidelity
**Target:** Close G01, G02, G03, G13, G14, G15
Audit the header zone against the v05 mockup pixel-for-pixel. Determine policy on which icon buttons are visible. Document what CI/CM/P/DK/PS map to in terms of behavior. Restore or formally remove the DepthZoneDial. Update GalleryLookAndFeel comment to reference v05. Audit the EXPORT button fill color.

### Specialist 2 — Column C Design System
**Target:** Close G06, G07, G09, R06
Fix the tab bar height (38px → 32px). Resolve tab underline color policy (Gold vs. Accent). Audit all 6 tab panels against the mockup. Standardize preset card secondary row. Decide on dynamic-accent vs. fixed-gold underline and document the rule.

### Specialist 3 — Column B Layout & Interactivity
**Target:** Close G04, G05, G08, G16
Determine if signal flow strip should become interactive navigation or remain decorative. Assess whether the macro knobs row removal (G05) is permanent or should be restored as the mockup shows. Verify MasterFX strip height (68px). Audit ParameterGrid scroll performance and lazy-load behavior.

### Specialist 4 — Brand Policy & Dead Code Sweep
**Target:** Close R01, R05, R02
Convene with project lead to resolve Light vs. Dark mode default. Document the v04→v05 delta. Remove or formally comment out all `y=-100` parked widgets that are permanently removed. Clear `CreatureState` scaffold if creature renderer is not planned for v1. Update all component header comments that reference v04.

---

## Appendix: File Quick Reference

| Path | Role |
|------|------|
| `Docs/mockups/xoceanus-v05-accurate.html` | Canonical design target |
| `Docs/mockups/xoceanus-v04-polished.html` | Prior reference (partially stale) |
| `Docs/ui-components-dimensions-2026-03-26.md` | Dimension spec sheet (mostly current, some v04 residue) |
| `Source/UI/XOceanusEditor.h` | Main window + resized() layout orchestration |
| `Source/UI/GalleryColors.h` | Color system (at UI/, not Gallery/) |
| `Source/UI/Gallery/GalleryLookAndFeel.h` | JUCE widget overrides (references v04) |
| `Source/UI/Gallery/ColumnLayoutManager.h` | Master layout constants |
| `Source/UI/Gallery/CompactEngineTile.h` | Col A tiles (recently reworked) |
| `Source/UI/Gallery/SidebarPanel.h` | Col C tabs (38px bar, needs 32px) |
| `Source/UI/Gallery/CouplingInspectorPanel.h` | COUPLE tab (largest sidebar component) |
| `Source/UI/Gallery/ObrixDetailPanel.h` | Custom OBRIX detail (55KB anomaly) |

---

*TIDEsigns Phase 1 Survey — Ready for Phase 2 specialist dispatch.*
