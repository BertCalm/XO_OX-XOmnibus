# TIDEsigns — Phase 2 Audit
**Author:** TIDEsigns Audit Team
**Date:** 2026-03-27
**Repo:** `~/Documents/GitHub/XO_OX-XOmnibus/`
**Reference:** `Docs/mockups/xolokun-v05-accurate.html`
**Method:** Direct source read of `XOlokunEditor.h`, `EngineDetailPanel.h`, `SidebarPanel.h`, `GalleryLookAndFeel.h`, `GalleryColors.h`, `ColumnLayoutManager.h`, `CompactEngineTile.h`, `MasterFXSection.h`, `StatusBar.h`, `CouplingInspectorPanel.h` — full survey of all `y=-100`/`y=-200` parking sites.

---

## Focus 1 — CORAL (Visual): Header & Tab Bar

### Critical

**C1-01 — Header right cluster is sparse: 5 buttons hidden, only PLAY + EXPORT + gear visible**
`XOlokunEditor.h` lines 743–752 park `cinematicToggleBtn` (CI), `cmToggleBtn` (CM), `perfToggleBtn` (P), `themeToggleBtn` (DK), and `midiIndicator` at `y = -100`. `abCompare`, `presetPrevBtn`, `presetNextBtn` also parked. The visible right cluster is: PLAY (48px) + EXPORT (56px) + gear (28px) + CPU meter (64px). The v05 mockup shows all 5 icon buttons active. The header right side is visually half-empty compared to the mockup.

**C1-02 — DepthZoneDial explicitly parked off-screen**
`depthDial.setBounds(0, -100, 0, 0)` at line 743. The dial is still constructed and owned; it participates in `selectSlot()` logic (line 862: `depthDial.setSlot(...)`) and drives `openEnginePickerPopup()` from the ENGINES button handler (line 153). It is logically active but visually absent. The v05 mockup places a 36x36 dial between ENGINES and the macro group. This is the most prominent structural gap in the header.

**C1-03 — Tab bar height 38px, mockup spec is 32px**
`SidebarPanel.h` line 428: `static constexpr int kTabBarH = 38;`. The v05 spec (`tidesigns-survey.md` section 2, Column C) is 32px. The comment at the top of `SidebarPanel.h` (line 7) correctly records `32pt` as the target but the constant was never updated. 6px discrepancy compresses every tab content panel by 6px and makes the tab bar visually heavier than intended.

### Important

**C1-04 — Active tab underline uses engine accent color — not explicitly XO Gold, but also not the survey's "Gold" finding**
`SidebarPanel.h` line 333: `g.setColour(engineAccent);` for the 2px underline. The header comment (line 9) says "Active: full-opacity text + 2px XO Gold underline" but the actual paint code uses the current engine accent. The v05 mockup uses `var(--accent)` (contextual teal) — so the *paint code is closer to the mockup than the comment suggests*. The discrepancy is between the comment and the code, not the code and the mockup. Update the comment to match actual behavior (engine accent = correct per v05), or intentionally freeze it to XO Gold if the dynamic-accent-per-engine approach is not desired. See Focus 4 / R06.

**C1-05 — GalleryLookAndFeel references v04, not v05**
`GalleryLookAndFeel.h` line 9: `// Matched to xolokun-v04-polished.html prototype exactly.` The canonical reference is now v05. No functional breakage identified in the audit pass, but any future pixel-matching against v04 will produce wrong results. Stale reference.

**C1-06 — Logo geometry: left circle dot uses stroke color, not fill**
`XOlokunEditor.h` lines 578–581: the left ring is drawn with `.drawEllipse()` (stroke), then `.fillEllipse()` for the center dot — using `0xFF1E8B7E.withAlpha(0.85f)` for both. The right ring does the same with gold. The v05 mockup SVG shows two filled circles with overlapping areas, not stroke rings with center dots. The painted logo is a structural approximation — acceptable but not pixel-accurate to the SVG spec in `tidesigns-survey.md` section 2.

### Minor

**C1-07 — Subtitle font size 8px is at the boundary of readability**
`XOlokunEditor.h` line 599: `GalleryFonts::body(8.0f)` for "XO_OX Designs". At 8px on a 52px header this may be sub-pixel on non-Retina displays. The v05 mockup does not specify a font size for the subtitle explicitly but the visual weight implies ~10px.

**C1-08 — Brand contradiction: CLAUDE.md says "light mode is the default (brand rule)" but code defaults to dark**
`XOlokunEditor.h` line 90 comment: `// Light mode is the default (brand rule)`. But line 103 reads persisted preference with `getBoolValue("darkMode", true)` — default `true` means dark. `GalleryColors.h` line 33: `static bool dark = true`. Both the code and the v05 mockup are dark-first. CLAUDE.md is the lagging artifact. Resolve the policy and update the comment.

---

## Focus 2 — DRIFT (Interaction): Column B Flow

### Critical

**C2-01 — Signal flow strip is permanently paint-only with a hardcoded active section**
`XOlokunEditor.h` line 685: `const int activeSection = 0; // P0-12: default active = SRC1 (index 0)`. The six labels (SRC1 → SRC2 → FILTER → SHAPER → FX → OUT) are painted in `paint()` with no mouse handler, no hit regions, no state wiring. The v05 mockup describes clickable navigation blocks (OSC → ENV → FILTER → FX → COUPLING) that scroll or jump the ParameterGrid to the relevant section. This requires architecture work, not just visual change: state variable for active section, mouse hit-testing on the strip area, and a `ParameterGrid::scrollToSection()` callback. The label names also diverge: the code uses `SRC1/SRC2/FILTER/SHAPER/FX/OUT` (6 items, engine-DSP oriented) while the mockup uses `OSC/ENV/FILTER/FX/COUPLING` (5 items, more musical). The label set should be reconciled.

**C2-02 — MacroHeroStrip is zeroed out and hidden in EngineDetailPanel**
`EngineDetailPanel.h` lines 395–396: `macroHero.setBounds(0, 0, 0, 0); macroHero.setVisible(false);` — the strip is constructed but never shown. The `macroHero.loadEngine()` call at line 138 still runs, wasting string allocation and parameter lookup. Since the JUCE global macro knobs row (`macros` in the header) serves this function, the strip is double-loaded but only one is visible. The dead `loadEngine()` call on an invisible component is a minor resource waste, but also creates confusion: code reads as if MacroHeroStrip is functional in this context when it is not.

### Important

**C2-03 — MasterFX strip height is 96px in code, 68px in v05 spec**
`XOlokunEditor.h` line 1266: `static constexpr int kMasterFXH = 96;`. The v05 mockup specifies 68px. The comment says "extra height prevents ADV buttons from overlapping knob labels" — a legitimate constraint, but the 28px excess visually crowds the ParameterGrid above it. The code comment acknowledges this is a layout workaround. If the ADV buttons can be repositioned or the label layout tightened, this could recover 28px of scroll height in the ParameterGrid.

**C2-04 — Tile click → detail panel transition: no visual feedback during the 150ms fade**
`XOlokunEditor.h` line 1269: `kFadeMs = 150`. During the `callAfterDelay(kFadeMs, ...)` window, the tile is selected but the detail panel is not yet visible. There is no intermediate loading indicator or tile highlight state that confirms the click registered. On a slow repaint cycle the user may click again, triggering a second `selectSlot()` call. The CQ17 guard (line 885) cancels the second load if `slot != selectedSlot`, but the double-click can still cause two fade animations to queue.

**C2-05 — EngineDetailPanel header kHeaderH constant is 38px — matches old tab bar height, not a mockup spec**
`EngineDetailPanel.h` line 461: `static constexpr int kHeaderH = 38;`. No mockup spec was found for this specific value. It happens to match the old tab bar height (38px). The engine name + accent line section appears adequate but the value's origin is undocumented.

### Minor

**C2-06 — ADSR display uses proportional segment widths derived from param values, not actual time-linear mapping**
`EngineDetailPanel.h` lines 49–52: `aW = a * totalW * 0.3f + totalW * 0.05f` — a heuristic formula. The result is a rough envelope shape that may not accurately represent the actual ADSR timing. For engines with very short attack + very long release, the displayed shape will be misleading. This is a fidelity issue, not a crash.

**C2-07 — Hover feedback on knobs relies entirely on GalleryLookAndFeel, no component-level override**
No per-knob `mouseEnter`/`mouseExit` callbacks found in `EngineDetailPanel.h`, `ParameterGrid.h` is not read in full but GalleryKnob.h is the delegate. Hover state is implicit through JUCE's built-in `isMouseOver()` repaint. Whether the hover color change matches the mockup (subtle arc brightening) cannot be confirmed without runtime inspection, but there is no custom hover implementation visible.

---

## Focus 3 — LATTICE (Systems): Dead Code & Consistency

### Critical

**C3-01 — 10 components parked at y=-100/y=-200 remain live in the component tree**
`XOlokunEditor.h` lines 743–752 + 825:
- `depthDial` — at `(0, -100, 0, 0)`, still wired for `setSlot()` + `openEnginePickerPopup()`
- `abCompare` — at `(0, -100, 0, 0)`, permanently removed feature
- `presetBrowser` — at `(0, -200, 0, 0)`, superseded by sidebar `PresetBrowser`
- `presetPrevBtn` / `presetNextBtn` — at `(0, -100, 0, 0)`, handlers are placeholder `repaint()` only
- `cinematicToggleBtn` — at `(0, -100, 0, 0)`, fully functional but hidden
- `cmToggleBtn` — at `(0, -100, 0, 0)`, fully functional but hidden
- `perfToggleBtn` — at `(0, -100, 0, 0)`, fully functional but hidden
- `themeToggleBtn` — at `(0, -100, 0, 0)`, fully functional but hidden
- `midiIndicator` — at `(0, -100, 0, 0)`
- `fieldMap` — at `(0, -200, 0, 0)`

Each parked component still: (a) receives `resized()` calls, (b) is a child in the component tree, (c) participates in z-order and mouse hit-testing with zero-size bounds. Zero-size bounds prevent mouse hits, but these components are not `setVisible(false)` — they are merely off-screen. This is resource waste and cognitive overhead. Components that are permanently removed should use `setVisible(false)` or be removed from the child list. Components that are intentionally hidden (CI, CM, P, DK, PLAY) but may return should at minimum be `setVisible(false)` so JUCE excludes them from accessibility trees.

**C3-02 — `presetBrowser` (standalone) and sidebar `PresetBrowser` are both live — double construction**
`XOlokunEditor.h` line 126: `addAndMakeVisible(presetBrowser)`. This is the old standalone `PresetBrowser` member. Line 745 then parks it at `(0, -200, 0, 0)`. The sidebar's `SidebarPanel::setPresetManager()` constructs a second `PresetBrowser` inside the `ContentArea`. Both are wired to the same `PresetManager`. Double construction means double RAM usage, double preset list allocation, and potential double state divergence if either panel fires a selection callback.

### Important

**C3-03 — CouplingInspectorPanel miniViz parked at y=-200 but still calls setVisible(false) — redundant**
`CouplingInspectorPanel.h` line 417: `miniViz.setBounds(0, -200, 0, 0); miniViz.setVisible(false);`. Both off-screen parking AND setVisible(false) are applied. Redundant — pick one. The correct approach is `setVisible(false)` only; `setBounds(0, -200, ...)` adds no benefit and is confusing.

**C3-04 — OutshineSidebarPanel parked at (-4000, -4000) — non-standard parking coordinate**
`SidebarPanel.h` line 399: `outshineSidebar->setBounds(-4000, -4000, 320, 180)`. This is a non-zero size at a far-off coordinate. The component is 320x180 and receives layout calls. The extreme negative coordinate is unusual — `-4000` puts it well outside the monitor space on any screen. `setVisible(false)` is the correct approach.

**C3-05 — CreatureState scaffold struct retained in CompactEngineTile.h**
`CompactEngineTile.h` lines 17–23: `struct CreatureState` with four float members. The porthole and creature renderer were removed 2026-03-27 per the comment. The struct has no current usage site in `CompactEngineTile` (it is a member but none of its fields are read or written in the current paint flow). If creature sprites are not planned for v1, remove the struct. If they are planned, document the target date.

**C3-06 — `kMacroKnobsRowH = 64` constant defined but the macro knobs row was deliberately removed**
`XOlokunEditor.h` line 1268: `static constexpr int kMacroKnobsRowH = 64; // P0-13: macro knobs row placeholder`. The row was removed as "redundant with EngineDetailPanel's MacroHeroStrip" (which itself is now zeroed out — see C2-02). The constant is not used in `resized()`. Dead constant with a stale comment.

### Minor

**C3-07 — GalleryLookAndFeel reference comment is stale (v04, not v05)**
Already flagged in C1-05. Repeated here as a systems consistency issue: any automated test or future contributor matching the JLAF to the mockup would reach for v04 and get incorrect results.

**C3-08 — Inactive tab color in SidebarPanel constructor uses `t3()` (theme-aware), but header comment hardcodes `rgba(80,76,70,0.75)` (light-mode value)**
`SidebarPanel.h` line 62: `btn->setColour(textColourOffId, GalleryColors::get(GalleryColors::t3()))`. The constructor is correct — it uses the theme-aware `t3()` accessor. But the file header (line 10) hardcodes `rgba(80,76,70,0.75)` which is a light-mode value. In dark mode, `t3()` returns `#5E5C5A` which is noticeably different from `rgba(80,76,70,0.75)`. The comment is wrong. This is documentation debt only — the code is correct.

---

## Focus 4 — SHEEN (Polish): First Impression

### Critical

**C4-01 — On first open, Column B shows OverviewPanel (no engine detail), but the user has no cue that clicking a tile will switch views**
The overview-to-detail transition is a click-to-reveal pattern with a 150ms cross-fade. A new user sees 4 engine tiles on the left and an overview on the right. Nothing labels the tiles as clickable or explains the tile→detail reveal. No tooltip appears on hover because tile tooltips say "Click to select engine, right-click for options" — this wording ("select") does not communicate that a full detail panel will open. First-use confusion is high.

**C4-02 — PLAY tab exists in the SidebarPanel tab list but is hidden without explanation**
`SidebarPanel.h` line 73–75: `if (i == Play) btn->setVisible(false)`. The PLAY tab is removed from the visible set. Users who expect a PLAY tab (from any prior documentation or marketing) will not find it. The sidebar shows 5 tabs (PRESET / COUPLE / FX / EXPORT / SETTINGS) with no indication that PLAY is accessible elsewhere (PlaySurface popup via the PLAY button in the header). A tooltip on the PLAY header button, or a small mention in the SETTINGS tab, would bridge this.

### Important

**C4-03 — No encoding issues found**
The grep for mojibake sequences (`â€`, `Ã`, etc.) across the entire `Source/UI/` directory returned no matches. All UTF-8 special characters (arrows `→`, gear `⚙`, triangle `▾`) use explicit `juce::CharPointer_UTF8("\xe2...")` encoding. Clean.

**C4-04 — Information hierarchy in the header is unclear at small sizes**
The header progression (left to right): logo rings → "XOlokun" title + "XO_OX Designs" subtitle → ENGINES button → [5 macro knobs filling ~420px] → CPU meter → PLAY button → EXPORT button → gear. The macro section dominates the header width. At the reference 1100px width, `macros.setBounds(header.reduced(8, 4))` gives the macro section approximately 300px after the right cluster consumes ~200px and the left cluster ~180px. The brand identity (logo + text) at 48–148px occupies only ~100px of the 1100px total. The title is visually subordinate to the macro knobs, which is unconventional for a DAW plugin header. This is a deliberate design choice but may confuse users expecting a centered product name or version indicator.

**C4-05 — MasterFX strip knobs have no section separators visible between SAT / DELAY / REVERB / MOD / COMP**
`MasterFXSection.h` shows 6 knobs (`satDrive`, `delayMix`, `delayFeedback`, `reverbMix`, `modDepth`, `compMix`) plus section labels. The v05 mockup describes pill-chip containers per FX slot. Whether the paint code renders visual pill chips was not confirmed (full MasterFXSection.h paint() not read), but the knob color is set to `GalleryColors::get(GalleryColors::textMid()).withAlpha(0.7f)` — a neutral grey, not the accent-colored chips shown in the mockup. This is the strip's primary visual distinguisher from the mockup.

### Minor

**C4-06 — Ghost tile (slot 5) materializes without any introduction or user education**
`checkCollectionUnlock()` will fade in a 5th tile when the right 4-engine combination is loaded. A new user who happens to load a collection set will see a tile appear with no tooltip, no badge, and no explanation. The `ghostTile` has the standard `A11y::setup()` text "Engine Slot 5" but no "collection detected" contextual message.

**C4-07 — StatusBar trigger pad buttons are created, fully wired, then hidden — creating silent keyboard shortcut behavior**
`StatusBar.h` lines 57–60: FIRE/XOSEND/ECHO CUT/PANIC are `setVisible(false)`. But the file header states keyboard shortcuts Z/X/C/V remain active. A user pressing Z in a text field (which is guarded by KAI-P2-03) would be fine, but pressing Z while a knob is focused would silently fire a chord machine action with no visual feedback confirming it happened. The Z/X/C/V shortcuts should either have a visible indicator in the status bar when triggered, or a toast notification.

---

## Summary Table

| ID | Focus | Severity | File | Action |
|----|-------|----------|------|--------|
| C1-01 | Coral | Critical | `XOlokunEditor.h:743` | Decide: restore hidden buttons or formally remove them |
| C1-02 | Coral | Critical | `XOlokunEditor.h:743` | Restore DepthZoneDial to header or cut the component entirely |
| C1-03 | Coral | Critical | `SidebarPanel.h:428` | Change `kTabBarH` from 38 to 32 |
| C1-04 | Coral | Important | `SidebarPanel.h:9,333` | Fix header comment to match actual engine-accent behavior |
| C1-05 | Coral | Important | `GalleryLookAndFeel.h:9` | Update reference comment from v04 to v05 |
| C1-06 | Coral | Important | `XOlokunEditor.h:566-588` | Logo rings: verify geometry against v05 SVG spec |
| C1-07 | Coral | Minor | `XOlokunEditor.h:599` | Raise subtitle from 8px to 10px |
| C1-08 | Coral | Minor | `XOlokunEditor.h:90, GalleryColors.h:33` | Resolve brand/comment contradiction; update CLAUDE.md |
| C2-01 | Drift | Critical | `XOlokunEditor.h:683-723` | Architect signal flow strip interactivity; reconcile label names with mockup |
| C2-02 | Drift | Critical | `EngineDetailPanel.h:395-396` | Remove dead `macroHero.loadEngine()` call on hidden strip |
| C2-03 | Drift | Important | `XOlokunEditor.h:1266` | Reduce `kMasterFXH` from 96 to 68 if ADV layout permits |
| C2-04 | Drift | Important | `XOlokunEditor.h:881` | Add tile selected state or loading indicator during 150ms fade |
| C2-05 | Drift | Important | `EngineDetailPanel.h:461` | Document origin of `kHeaderH = 38` or adjust to v05 spec |
| C2-06 | Drift | Minor | `EngineDetailPanel.h:49-52` | ADSR proportional mapping is heuristic — document as intentional approximation |
| C2-07 | Drift | Minor | (runtime only) | Verify knob hover color matches v05 at runtime |
| C3-01 | Lattice | Critical | `XOlokunEditor.h:743-752,825` | `setVisible(false)` on all permanently removed components |
| C3-02 | Lattice | Critical | `XOlokunEditor.h:126,745` | Remove standalone `presetBrowser` member; sidebar's PresetBrowser is canonical |
| C3-03 | Lattice | Important | `CouplingInspectorPanel.h:417` | Remove redundant `setBounds(0,-200,...)` from miniViz; keep `setVisible(false)` |
| C3-04 | Lattice | Important | `SidebarPanel.h:399` | Replace `(-4000,-4000,...)` parking with `setVisible(false)` |
| C3-05 | Lattice | Important | `CompactEngineTile.h:17-23` | Remove `CreatureState` if not planned for v1; document if planned |
| C3-06 | Lattice | Important | `XOlokunEditor.h:1268` | Remove dead `kMacroKnobsRowH` constant |
| C3-07 | Lattice | Minor | `GalleryLookAndFeel.h:9` | (same as C1-05) |
| C3-08 | Lattice | Minor | `SidebarPanel.h:10` | Fix header comment: `rgba(80,76,70,0.75)` is light-mode only |
| C4-01 | Sheen | Critical | `CompactEngineTile.h:47` | Add hover tooltip copy explaining tile→detail reveal |
| C4-02 | Sheen | Critical | `SidebarPanel.h:73` | Add PLAY button tooltip or SETTINGS note pointing to PlaySurface popup |
| C4-03 | Sheen | — | (all files) | No encoding issues found |
| C4-04 | Sheen | Important | `XOlokunEditor.h:770` | Consider visual weight of macro section vs. brand identity in header |
| C4-05 | Sheen | Important | `MasterFXSection.h` | Verify FX chip pill rendering matches v05 accent-colored chips |
| C4-06 | Sheen | Minor | `XOlokunEditor.h:checkCollectionUnlock` | Add "collection detected" contextual text when ghost tile appears |
| C4-07 | Sheen | Minor | `StatusBar.h:57-60` | Add visual feedback (toast or status text) when Z/X/C/V shortcuts fire |

---

*TIDEsigns Phase 2 Audit — 2026-03-27. Ready for triage and fix dispatch.*
