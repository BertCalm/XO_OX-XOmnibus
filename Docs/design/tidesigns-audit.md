# TIDEsigns QDD Full-Fleet UI/UX Audit — 2026-03-29

## Overview
- **Target:** XOceanus Desktop Plugin — 86 UI files, 86 engines, ~1.5MB source
- **Method:** RAC (Ringleader + Architect + Consultant) × TIDEsigns × QDD
- **Phases:** 5 (Foundation → Core Components → Subsystems → Engine-Specific → Cross-Cutting)
- **Subagent dispatches:** 34 parallel audits
- **Total findings:** 100 Critical, 164 Important, 186 Minor = 450 total
- **Fixed in-session:** 34 (Commits: 7196fdbf1, c01948faa)

## Composite Score: 5.4/10

| Dimension | Score | Key Issue |
|-----------|-------|-----------|
| Color System | 7/10 | Luminance floor landed. Gold cluster + near-white cluster still confusable |
| Typography | 5/10 | Overbit never used. 50% fallback labels broken |
| Layout | 8/10 | Three-column correct. MPC dead code. Ghost tile fixed |
| Interaction Feel | 6/10 | 300ms transitions. No knob hover. Focus ring added |
| Accessibility | 5/10 | 7 components missing A11y. Color-only encoding |
| Coupling UX | 3/10 | Drag-to-create broken. Right-click dead. 3 color systems |
| Export | 5/10 | Thread safety fixed. Dead sidebar controls. No MIDI export |
| PlaySurface | 7/10 | Key forwarding fixed. Georgia dependency. NoteOff fixed |
| Engine Identity | 4/10 | Creature removed. 7/76 vocabulary coverage |
| Dark/Light Mode | 6/10 | 13 hardcoded Light:: in Outshine. 6 white-on-themed files |
| Performance | 6/10 | 569 timer callbacks/sec peak. Unconditional repaints |
| Spec Fidelity | 4/10 | DNA hex, Tide Controller, sensitivity map all MISSING |
| Dead Code | 4/10 | paintMacroBars dead. CreatureRenderer 0 callers. 10 parked components |

---

## Fixes Applied (34 total, 2 commits)

### Commit 1: 7196fdbf1 — Phase 1+2 (24 fixes, 13 files)

| Fix | File | Description |
|-----|------|-------------|
| DPI inversion | GalleryLookAndFeel.h | Removed / dpiScale on arc strokes |
| Invisible accents | GalleryColors.h | ensureMinContrast() luminance floor |
| OBIONT missing | GalleryColors.h | Added 0xFFE8A030 |
| Fallback color | GalleryColors.h | borderGray → t2 |
| PlaySurface fonts | PlaySurface.h | Removed shadow GalleryFonts namespace |
| CouplingViz fonts | CouplingVisualizer.h | 3 raw juce::Font → GalleryFonts |
| OpticViz font | OpticVisualizer.h | Raw 14px → GalleryFonts::display |
| Disabled states | GalleryLookAndFeel.h | isEnabled() on all 4 draw methods |
| getCurrentOpacity | GalleryLookAndFeel.h | → g.setOpacity(0.35f) |
| Tile arcs | CompactEngineTile.h | Hardcoded → macroValues[k] |
| Coupling dots | CompactEngineTile.h | paintCouplingDots() now called |
| Knob focus ring | GalleryKnob.h | focusGained/focusLost added |
| Preset arrows | XOceanusEditor.h | Stubs → PresetManager nav |
| Coupling labels | OverviewPanel.h + CouplingChainView.h | KnotTopology + 2 siblings |
| q classifier | ParameterGrid.h | contains("q") → exact match |
| MACRO keywords | ParameterGrid.h | space/character/movement exact match |
| cy offset | ParameterGrid.h | cy → cy+2 consistency |
| Picker commit | EnginePickerPopup.h | Single-click now commits |
| Archetype text | EnginePickerPopup.h | Subtitle rendered in rows |
| Engine count | EnginePickerPopup.h | Dynamic placeholder |
| Empty state | EnginePickerPopup.h | "No engines match" label |
| Pill labels | EnginePickerPopup.h | SYN→Synth etc + tooltips |
| AB Compare | ABCompare.h | New loads → B slot |
| sfHitRects | XOceanusEditor.h | paint() → resized() |

### Commit 2: c01948faa — Phase 3 (10 fixes, 8 files)

| Fix | File | Description |
|-----|------|-------------|
| Key forwarding | PlaySurface.h | Forward to XOuijaPanel |
| ToucheExpression bg | ToucheExpression.h | White → surface() |
| TriangularCoupling | CouplingVisualizer.h | Added to color/label/allTypes |
| previewPlaybackPos | ExportDialog.h | int → atomic<int> |
| exportFinished | ExportDialog.h | release/acquire ordering |
| Library snapshot | ExportDialog.h | Copy before thread launch |
| DrumPadGrid noteOff | DrumPadGrid.h | mouseUp sends noteOff |
| TriangleXYPad keyboard | SpecializedDisplays.h | Arrow key navigation |
| BipolarAxisBar async | SpecializedWidgets.h | async → sync notification |
| ObrixDetailPanel | ObrixDetailPanel.h | compactMode uses viewport height |

---

## Open Criticals (66 remaining)

### COUPLING (6 criticals)
- C1: Drag-to-create fires with no type selector popup
- C2: Right-click context menu built but juce::ignoreUnused(menu)
- C3: CouplingPopover only lists slots 1-4 (missing Ghost Slot)
- C4: CouplingStripEditor mouseDown does nothing
- C5: miniViz hidden unconditionally — 96pt dead zone
- C6: Three incompatible color systems across coupling components

### THREAD SAFETY (5 criticals)
- Outshine preview creates 2nd AudioDeviceManager (can silence plugin)
- Outshine background threads hold raw this without SafePointer
- NoteLifetimeWatchdog audio/message thread race
- MobileNoteQueue force-eviction breaks SPSC invariant
- TouchForce_iOS receives wrong index (JUCE source vs UITouch hash)

### DEAD UI / PARKED COMPONENTS (10 criticals)
- CM/CI/P/DK buttons parked off-screen with no UI access
- presetPrevBtn/presetNextBtn parked (wired but invisible)
- midiIndicator parked (live callbacks, invisible)
- fieldMapPanel receiving data invisibly
- depthDial parked (spec's primary engine selector)
- 'P' key conflict (PlaySurface vs PerformanceView)

### TYPOGRAPHY (5 criticals)
- Overbit (engine nameplate font) loaded but never used
- XOuijaPanel depends on Georgia system font (no fallback)
- 50% of fallback parameter labels are poor/broken
- Oxytocin 12 multi-underscore params produce broken labels
- Ocelot cross-feed matrix (12 params) labels lose routing direction

### ACCESSIBILITY (5 criticals)
- 7 interactive components missing A11y::setup entirely
- Color-only encoding in coupling dots + CPU meter
- 8 components accept focus but don't draw focus ring
- No live screen reader announcements for preset load/coupling changes
- Light mode: textMid + xoGoldText fail WCAG AA

### RENDERING (3 criticals)
- OscarJuceRenderer makeRenderPath/makeRenderPaint leak memory
- CouplingVisualizer unconditional 30Hz repaint
- PlayControlPanel 30Hz timer with no visibility gate

### SPEC FEATURES MISSING (5 criticals)
- Sonic DNA Hexagons — zero implementation
- Tide Controller — zero implementation
- Parameter Sensitivity Map — zero implementation
- Sound on First Launch — zero implementation
- PlaySurface embedded → floating (architectural divergence)

### OTHER (7 criticals)
- ExportTabPanel settings are dead controls (silently ignored)
- No undo integration for ModRouting changes
- GestureTrailBuffer no thread safety
- Fretless pitch bend assumes ±2 semitones
- ConductorArcDisplay readChoiceParam incorrect normalization
- KeysMode MIDI double-fire on noteOn
- MobilePlaySurface shellWhite background in dark mode

---

## Six Tides Verdict

| Tide | Verdict | Key Issue |
|------|---------|-----------|
| T1: Pad Player | UNDERSERVED | No auto-loaded sound on first launch |
| T2: Deep Diver | SERVED (gaps) | Coupling labels cryptic; KnotTopology fixed |
| T3: First Touch | UNDERSERVED | Zero onboarding; jargon everywhere |
| T4: Session Pro | SERVED | Preset arrows now wired |
| T5: Accessibility | PARTIAL | Color-only encoding; incomplete focus rings |
| T6: Reef Explorer | EXCLUDED | Creature renderer removed; mythology invisible |

---

## Phase 4: EngineVocabulary Coverage

- 7/90 engines have override tables (9.5%)
- 50% of fallback-derived labels are poor or broken
- Root causes: multi-underscore IDs, LFO identity erasure, parallel envelope collision, multi-layer LEVEL/RATE/DEPTH saturation
- Priority engines needing overrides: Oxytocin, Ocelot, Owlfish, Obscura, Fat

## Phase 5: Performance

- 25 simultaneous timers, 569 callbacks/sec at peak
- CouplingVisualizer: unconditional 30Hz repaint (highest waste)
- DrumPadGrid: 32 Font constructions per frame
- XOceanusEditor: 42 string-width measurements per repaint
- Cinematic mode: colA=0 bypasses minimum guards

## Phase 5: Dead Code

- 10 parked-but-wired components in XOceanusEditor
- paintMacroBars: 35 lines, zero callers
- CreatureRenderer.h: 311 lines, zero callers
- cachedDashedPath: computed in resized(), unused in paint()
- FieldMapPanel: receiving data invisibly

## Phase 5: Spec Divergence

- PlaySurface: embedded (spec) → floating popup (reality)
- Depth Zone Dial: prominent (spec) → parked off-screen
- Button System: 3-tier (spec) → single-tier (reality)
- Transitions: 400ms (spec) → 150ms (reality)
- Sonic DNA, Tide Controller, Sensitivity Map: MISSING entirely

---

## Recommended Fix Sequence

### Wave 1: Quick Wins (1-2 lines each)
- Delete paintMacroBars dead code
- Gate CouplingVisualizer repaint on !cachedRoutes.empty()
- Add visibilityChanged() to PlayControlPanel, ConductorArcDisplay, WaveformDisplay
- Fix ConductorArcDisplay readChoiceParam normalization
- Fix KeysMode MIDI double-fire (if → else if)

### Wave 2: Parked Component Restoration
- Restore presetPrevBtn/presetNextBtn to header
- Restore midiIndicator to header
- Restore cinematicToggleBtn to header
- Restore cmToggleBtn and perfToggleBtn
- Resolve 'P' key conflict

### Wave 3: Vocabulary + Labels
- Add EngineVocabulary overrides for Oxytocin, Ocelot, Owlfish, Obscura, Fat
- Patch derivedLabel() for multi-underscore IDs
- Fix LFO disambiguation

### Wave 4: Coupling UX
- Implement showAddRoutePopup with type selector
- Wire showArcContextMenu (remove ignoreUnused)
- Add Ghost Slot to CouplingPopover
- Unify coupling color system

### Wave 5: Accessibility
- Add A11y::setup to 7 missing components
- Add shape differentiation to coupling dots
- Add focusGained/drawFocusRing to 8 components
- Fix Light mode contrast failures

### Wave 6: Thread Safety
- Fix Outshine AudioDeviceManager (route through processor)
- Fix Outshine thread lifetime (SafePointer on inner lambdas)
- Fix NoteLifetimeWatchdog race
- Fix MobileNoteQueue SPSC break
- Fix TouchForce_iOS index

### Wave 7: Performance
- Cache Fonts in DrumPadGrid, ObrixDetailPanel paint
- Cache string widths in XOceanusEditor paint
- Gate DepthZoneDial timer on visibility
- Reduce CouplingVisualizer repaint frequency

### Wave 8: Spec Features (Future Sessions)
- Sonic DNA Hexagons
- Depth Zone Dial restoration
- PlaySurface embed decision
- Button tier system
- Creature icon restoration
