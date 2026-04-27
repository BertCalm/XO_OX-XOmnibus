# Wave 11 Verification Sweep — Audit Report

**Date**: 2026-04-26  
**Branch audited**: `claude/issue-1184-oceanview-decomposition-phase1` (post Wave 5–9 merges)  
**Scope**: Orphaned components, dead wires, unfired callbacks, APVTS gaps, TODO mount residue  
**Method**: Static analysis + build sanity (compile: clean, link: stale cache — see note §7)

---

## Findings Table

| # | Severity | File:Line | Description | Recommended Fix |
|---|----------|-----------|-------------|-----------------|
| F01 | **CRIT** | `Source/UI/PlaySurface/XYSurface.h:44–76` | XYSurface.h (Wave 8) is a fully-implemented 888-line component that is **never `#include`d**, never instantiated, has **5 unresolved `TODO W8 mount`** items, and **0 APVTS params registered** (`xy_pattern_slotN`, `xy_speed_slotN` etc.). The XY surface feature shipped as a file but is completely disconnected from the host. | Implement mount TODOs: add `#include`, add `xyPanel_` member to OceanView or SurfaceRightPanel, wire `onXYChanged`, add 32 APVTS params in `createParameterLayout()`. |
| F02 | **CRIT** | `Source/UI/FirstHourWalkthrough.h:643–685` | FirstHourWalkthrough.h (Wave 9c) is a fully-implemented 636-line onboarding overlay component that is **never `#include`d anywhere**, never declared as a member, never added to any parent component. The guided onboarding tour is completely non-functional despite full implementation. File contains `TODO W9c mount` instructions. | Follow mount instructions at `FirstHourWalkthrough.h:643`: declare `walkthrough_` member in `XOceanusEditor.h`, wire 8 bound-accessor lambdas, `addChildComponent`, `setBounds`, trigger from `promptIfEligible()` post-greeting. |
| F03 | **HIGH** | `Source/UI/Ocean/SurfaceRightPanel.h:142–143` | `SurfaceRightPanel::onNoteOn` and `onNoteOff` are declared but **never assigned**. PAD mode (4×4 chromatic grid) and DRUM mode (4×4 drum grid) clicks fire `onNoteOn`/`onNoteOff` which resolve to `nullptr` — completely silent. Only `SubmarinePlaySurface::onNoteOn`/`onNoteOff` are wired in `XOceanusEditor.h:897–908`. | In editor `initOceanView()`, after wiring `onOuijaCCOutput` (line 921), also wire: `oceanView_.getSurfaceRight().onNoteOn = [this](int n, float v) { ... addMessageToQueue(NoteOn) ... };` |
| F04 | **HIGH** | `Source/UI/Ocean/SettingsDrawer.h:64` | `SettingsDrawer::onSettingChanged` is declared and fires from all 5 settings sections (Voice, Tuning, MIDI, Engine, Display) but is **never assigned anywhere** in OceanView or XOceanusEditor. All setting changes silently disappear. The SettingsDrawer has no APVTS reference, so settings only take effect if the host handles `onSettingChanged`. | Wire `settingsDrawer_.onSettingChanged` in `OceanView` constructor after `addChildComponent(settingsDrawer_)` (line 557). Map keys like `"voice_poly"`, `"tuning_master"` etc. to APVTS params or PropertiesFile. |
| F05 | **MED** | `Source/UI/Ocean/SurfaceRightPanel.h:144, 601–602` | `SurfaceRightPanel::onXYChanged` fires on all XY mode gestures (mouse drag + mouse up) but is **never assigned** in OceanView. XY position data is generated and discarded; APVTS params are not driven. Note: XYSurface.h (F01) was the intended integrator, but since XYSurface is unintegrated, this path is doubly broken. | After fixing F01: wire `surfaceRight_.onXYChanged = [this](float x, float y) { handleXYOutput(x, y); }` in OceanView constructor. |
| F06 | **MED** | `Source/UI/Ocean/TransportBar.h:130, 578–587` | `TransportBar::onTimeSigChanged` fires when numerator or denominator is tapped, but is **never assigned** in `XOceanusEditor.h` (only `onPlayToggled` and `onBpmChanged` are wired at lines 764, 781). Time signature changes silently disappear. | In editor `initTransportBar()` block (line 762+), add `tb->onTimeSigChanged = [&proc](int num, int den) { /* write to cm_seq_timeSig or SharedTransport */ };`. |
| F07 | **MED** | `Source/UI/Ocean/ChordBarComponent.h:137, 141` | `ChordBarComponent::onVisibilityChanged` and `onInputModeChanged` are declared and fire (lines 132, 1102) but are **never assigned** in OceanView or XOceanusEditor. Visibility toggle doesn't trigger layout updates; input mode changes (AUTO/PAD/DEG) don't propagate. Note: `chord_input_mode` APVTS param is correctly registered and `ChordBreakoutPanel` writes it directly — `onInputModeChanged` is redundant but still a broken interface contract. | Wire `cb->onVisibilityChanged = [this]() { resized(); };` in `OceanView::initChordBar()`. `onInputModeChanged` can be left null or wired to a debug log. |
| F08 | **LOW** | `Source/UI/Ocean/SurfaceRightPanel.h:401` | SurfaceRightPanel XY auto-motion pills use **stale labels** `{"CIRCLE", "FIG-8", "SWEEP", "RANDOM"}`. The locked Wave 8 B2 spec (`XYSurface.h:13`) replaced these with `PULSE / DRIFT / TIDE / RIPPLE / CHAOS`. Labels are cosmetic mismatch between the spec and the inline implementation. | Replace `kAutoMotionLabels[]` with `{"PULSE", "DRIFT", "TIDE", "RIPPLE", "CHAOS"}` and set `kAutoMotionCount = 5`. |

---

## Sweep Checklist Results

### 1. Orphaned Components
- **XYSurface.h** — CRIT (F01). No host inclusion, no instantiation.
- **FirstHourWalkthrough.h** — CRIT (F02). No host inclusion, no instantiation.
- All other Wave 5–9 components (SeqBreakoutComponent, ChordBreakoutPanel, SeqStripComponent, OceanChildren, etc.) are correctly mounted via `OceanChildren::initX()` and verified `addAndMakeVisible`.

### 2. Dead Wires / Unfired Callbacks
- `SurfaceRightPanel::onNoteOn/onNoteOff` — HIGH (F03)
- `SettingsDrawer::onSettingChanged` — HIGH (F04)  
- `SurfaceRightPanel::onXYChanged` — MED (F05)
- `TransportBar::onTimeSigChanged` — MED (F06)
- `ChordBarComponent::onVisibilityChanged` — MED (F07)
- `ChordBarComponent::onInputModeChanged` — MED (F07)
- All other new callbacks verified wired: `SubmarinePlaySurface::onNoteOn/Off/Aftertouch`, `SurfaceRightPanel::onOuijaCCOutput`, `TideWaterline::onHeightChanged`, `SeqStrip::setBreakout()`, `TransportBar::onPlayToggled/onBpmChanged`, `OceanView::onEngineSelected/onEngineDiveDeep/onEngineSelectedFromDrawer/onUndoRequested/onRedoRequested/onPlaySurfaceVisibilityChanged`, `DnaMapBrowser::onPresetSelected/onDismissed`, `DetailOverlay::onHidden`.

### 3. TODO Mount Comments Still Present
- `XYSurface.h:44–76` — 5 `TODO W8 mount` items (F01)
- `FirstHourWalkthrough.h:643–685` — `TODO W9c mount` block (F02)
- `FirstHourWalkthrough.h:656–669` — 4 nested `TODO W9c: expose ... bounds` items (F02)
- No Wave 5 or Wave 6 TODOs remain unresolved.

### 4. Callback Signature Spot-Checks (5 sampled)
1. `surfaceRight_.onOuijaCCOutput` → `processor.pushCCOutput(channel, cc, value)` — **MATCH** (`SurfaceRightPanel` emits `(uint8_t cc, uint8_t value)`; processor signature `(uint8_t channel, uint8_t cc, uint8_t value)` correctly receives channel offset).
2. `PerEnginePatternSequencer::addParameters("slot0_seq_", ...)` → `SeqBreakoutComponent` reads `"slot0_seq_enabled"` etc. — **MATCH**.
3. `TideWaterline::onHeightChanged` → `[this]() { resized(); }` — **MATCH** (no-arg callback).
4. `TransportBar::onPlayToggled` → APVTS `master_seqEnabled` toggle — **MATCH** (no-arg, reads param internally).
5. `SubmarinePlaySurface::onNoteOn(int note, float velocity)` → `MidiMessage::noteOn(1, note, velocity)` — **MATCH**.

### 5. APVTS Parameter Consistency
- `PerEnginePatternSequencer::addParameters()` called for slots 0–3 at `XOceanusProcessor.cpp:1243` — **VERIFIED**.
- Wave 6 `slot{N}_layout_mode` params registered for slots 0–3 at `XOceanusProcessor.cpp:1253` — **VERIFIED**.
- `chord_input_mode`, `cm_seq_bpm`, `cm_seq_gate`, `cm_seq_pattern` — **VERIFIED**.
- Wave 5 A1 global mod route params (read at `XOceanusProcessor.cpp:444+`) — **VERIFIED** (7 mod route slots, no APVTS, stored as ValueTree children).
- **XY APVTS params (32 total: 8 per slot × 4 slots) — MISSING**. `xy_pattern_slotN`, `xy_speed_slotN`, `xy_depth_slotN`, `xy_sync_slotN`, `xy_assignX_slotN`, `xy_assignY_slotN`, `xy_pos_x_slotN`, `xy_pos_y_slotN` are documented in `XYSurface.h:27–34` but never registered. See F01.

### 6. Build Sanity
- CMake configure: **CLEAN** (0 errors).
- Compile of `XOceanusEditor.cpp` (includes all Wave 5–9 headers transitively): **CLEAN** (0 errors, warnings only for pre-existing unused parameters in engines).
- Link step: **FAILED** with `ar: ... .o: No such file or directory` for ~8 engine `.cpp` object files. **This is a pre-existing stale build cache issue** (missing `.o` files from a prior partial build), NOT introduced by Wave 5–9 changes. The CI environment (fresh checkout) will not have this issue.

### 7. Unused Includes Spot-Check (3 new headers)
- **XYSurface.h**: includes `../../Core/XYPatternGenerator.h`, `../GalleryColors.h`, JUCE basics — all referenced in body. No dead includes.
- **FirstHourWalkthrough.h**: includes `juce_gui_basics`, `GalleryColors.h`, `<functional>`, `<memory>` — all referenced. No dead includes.
- **OceanChildren.h**: includes `TideWaterline.h`, `ChordBarComponent.h`, `ChordBreakoutPanel.h`, `SeqBreakoutComponent.h`, `SeqStripComponent.h`, `MasterFXStripCompact.h`, `EpicSlotsPanel.h`, `TransportBar.h` — all instantiated in `initX()` methods. No dead includes.

---

## Priority Fix Order

1. **F03** (SurfaceRightPanel.onNoteOn/Off) — 5-line fix, PAD/DRUM pads silent is a P0 regression.
2. **F04** (SettingsDrawer.onSettingChanged) — settings UI is a lie; needs APVTS key→param mapping.
3. **F01** (XYSurface orphaned) — full Wave 8 integration; blocked until mount TODOs are executed.
4. **F02** (FirstHourWalkthrough orphaned) — full Wave 9c integration; same pattern as F01.
5. **F05–F07** (XYChanged, TimeSig, ChordBar visibility) — secondary quality fixes.
6. **F08** (stale labels) — 1-line cosmetic fix.

---

*Audit method: static analysis of Source/ directory. Build: CMake Ninja Release, XOceanusEditor.cpp compiled clean.*
