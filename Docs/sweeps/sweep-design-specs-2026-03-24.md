# Design Spec Sweep — 2026-03-24

**Scope**: Validation of 4 design documents created in Sessions 8-9
**Documents checked**:
1. `Docs/design/xolokun-definitive-ui-spec.md` (1,773 lines)
2. `Docs/design/playsurface-design-spec.md` (971 lines)
3. `Docs/design/asset-registry.md` (1,005 lines)
4. `Docs/design/xolokun-ui-blessing-session.md` (507 lines)

**Status key**: CONFIRMED / WARNING / ERROR / NOTE

---

## DOCUMENT 1: Definitive UI Spec

### Color Hex Values

All hex values in the spec are valid 6-digit format. Spot-checked: `#F8F6F3`, `#1A1A1A`, `#E9C46A`, `#48CAE4`, `#0096C7`, `#7B2FBE`, `#150820`, `#D4AF37`, `#1E8B7E`, `#BF40FF`, `#00FF41`, `#00B4A0`, `#4ADE80`, `#F5C97A`, `#EF4444`, `#F0EDE8`, `#2A2A2A`, `#DDDAD5`.

**CONFIRMED** — No malformed hex values found.

### Font References

The spec uses three production fonts: **Space Grotesk**, **Inter**, **JetBrains Mono**. All three exist on Google Fonts (free, OFL licensed). Additionally references:
- **Nebulica** (premium — found at `~/Downloads/nebulica_.../OTF/`, confirmed on disk)
- **Kit-07 and Knob-Set-09** knob assets (found at `~/Downloads/Audio UI/KITS/kit-07/`, `~/Downloads/Audio UI/KNOBS-SET/knob-set-09/`, confirmed on disk)

**CONFIRMED** — All font references are valid.

**NOTE — Fonts not present in spec**: The task asked to verify references to "APERTURE, Monaspace, Michroma". None of these three fonts appear anywhere in the Definitive UI Spec or the PlaySurface Design Spec. "Aperture 3.2" appears only in the Asset Registry (§1.5 REFERENCE tier), marked for sci-fi contexts and not referenced in any active component spec. "Monaspace" and "Michroma" are entirely absent from all four documents. This may indicate the checklist was written against an older or proposed spec that was superseded.

### Animation Timings

Appendix E provides a complete animation timing bible. Every animation checked:
- All UI transitions: specified durations (150ms–600ms), explicit easing curves. No TBDs.
- All performance animations: specified rates (0.3Hz/0.2Hz Planchette drift, 150ms lock-on, 400ms release hold, etc.)
- Reduced motion alternatives specified for every entry.

A search for "TBD", "TODO", "placeholder", "NOT YET", "missing", and "undefined" across the spec returned zero matches (only one hit for the word "placeholder" in a color token comment for `textDim`, which is a correctly defined value `#BDBBB6`).

**CONFIRMED** — Animation timings are fully specified with no placeholders.

### 21-Phase Implementation Plan

The plan spans Phases 0–21 (22 total steps). Phase numbering is sequential with no gaps. Dependency review:

| Phase | Prerequisite | Status |
|-------|-------------|--------|
| Phase 0 (LookAndFeel) | None | Foundation — correct first |
| Phase 1 (Header + Status bar) | Phase 0 | Correct |
| Phase 2 (Engine panel L1) | Phase 0-1 | Correct |
| Phase 3 (Preset sidebar) | Phase 2 | Correct |
| Phase 4 (MPC Pad Grid) | Phase 0 | Correct |
| Phase 5 (XOuija + Planchette) | Phase 4 | Correct |
| Phase 6 (Expression controllers) | Phase 5 | Correct |
| Phase 7 (Dark Cockpit) | Phases 2, 5 | Correct |
| Phase 8 (Coupling tab + Visualizer) | Phase 3 | Correct |
| Phase 9 (Seaboard keyboard) | Phase 4 | Correct |
| Phase 10 (Tide Controller) | Phase 6 | Correct |
| Phase 11 (Sonic DNA hexagons) | Phase 2-3 | Correct |
| Phase 12 (Progressive disclosure L2/L3) | Phase 2 | Correct |
| Phase 13 (FX tab) | Phase 3 | Correct |
| Phase 14 (Sound on First Launch) | Phases 1-6 | Correct — requires prior phases |
| Phase 15 (Constellation View) | Phase 2 | Correct |
| Phases 16-19 (Innovations) | Phases 4, 5, 3, 5 respectively | Correct — correctly deferred |
| Phase 20 (iPad) | Phases 1-14 | Correct |
| Phase 21 (iPhone) | Phase 20 | Correct |

**CONFIRMED** — No circular dependencies. The plan is coherent. Total estimate is ~40 days.

### JUCE Class References

All JUCE classes referenced are valid JUCE 7/8 APIs:
- `juce::AudioProcessorEditor`, `juce::Component`, `juce::LookAndFeel_V4`, `juce::Graphics` — standard
- `juce::Slider` with `RotaryHorizontalVerticalDrag` — valid Slider style
- `juce::Drawable::createFromSVG()` — valid
- `juce::Path::addArc()`, `juce::PathStrokeType` — valid
- `juce::AbstractFifo` — valid for lock-free FIFO
- `juce::ImageCache::getFromFile()` — valid
- `juce::AccessibilityHandler`, `juce::AccessibilityRole`, `juce::AccessibilityActions` — valid (JUCE 6.1+)
- `juce::BubbleComponent` — valid
- `juce::MidiMessage::noteOn()`, `juce::MidiKeyboardState` — valid
- `juce::AudioProcessorValueTreeState::SliderAttachment` — valid
- `UIImpactFeedbackGenerator(.light)`, `UIImpactFeedbackGenerator(.medium)` — valid iOS APIs

**NOTE**: The spec references "Melatonin Blur JUCE library" (Section 1.1.1, Section 2.4.1) as an option for frosted glass effects. This is a real third-party JUCE library but is NOT in the JUCE standard library. Its use requires a dependency that is not tracked in any CMakeLists.txt or dependency manifest in the repo. The spec also provides a software fallback (`juce::Image::rescaled()` blur approximation), so this is a design-time dependency, not a hard runtime requirement. Recommend noting this external dependency in `Docs/design/xolokun-definitive-ui-spec.md` or in `CMakeLists.txt` if the frosted glass path is chosen.

**CONFIRMED** — All standard JUCE class references are valid. One untracked optional dependency (Melatonin Blur).

### Accessibility — WCAG 2.1 AA Contrast Ratios

The spec's accessibility section (Part 4) provides a contrast ratio matrix. All ratios were independently verified using the WCAG relative luminance formula. Results:

| Pair | Spec Claims | Calculated | Status |
|------|-------------|-----------|--------|
| `#E9C46A` XO Gold on `#1A1A1A` dark | 7.2:1 AAA | **10.4:1** | WARNING — Understated. Actual ratio is better; PASS still holds. AAA claim is correct. |
| `#E9C46A` XO Gold on `#F8F6F3` light | 2.4:1 FAIL | **1.5:1** | WARNING — The stated ratio (2.4:1) overstates the actual contrast (1.5:1). The FAIL verdict is still correct, but the number is wrong. |
| `#9E7C2E` XO Gold Text on `#F8F6F3` | 4.8:1 AA | **3.6:1** | **ERROR** — Spec claims 4.8:1 (AA pass). Actual ratio is 3.6:1, which is BELOW the 4.5:1 AA threshold for normal text. This is a **false AA claim**. Gold text on the light shell background does NOT pass WCAG AA for normal-sized text. |
| `#1A1A1A` text on `#F8F6F3` light | 14.7:1 AAA | 16.1:1 | OK — spec understates; actual is better. |
| `#F0EDE8` text on `#1A1A1A` dark | 13.9:1 AAA | 14.9:1 | OK |
| `#777570` secondary on `#F8F6F3` | 4.7:1 AA | **4.3:1** | WARNING — Spec claims 4.7:1 (barely AA). Actual is 4.3:1, which is below 4.5:1 AA. This is a **borderline false AA claim** for normal-weight text. Passes 3:1 for large text (18pt+). |
| `#A0A0A0` secondary on `#1A1A1A` dark | 6.0:1 AA | 6.7:1 | OK — actual is better. |
| `#A0A0A0` secondary on `#2E2E2E` card | 4.6:1 AA | 5.2:1 | OK |

**CRITICAL ERROR**: `#9E7C2E` (XO Gold Text) on `#F8F6F3` (light shell) calculates to 3.6:1, NOT 4.8:1. This means:
- All uses of "XO Gold Text" for normal-weight body copy on the light background fail WCAG AA.
- The spec currently recommends this color specifically for AA compliance on light backgrounds. That recommendation is incorrect.
- **Recommended fix**: Use `#8A6B22` or darker. At `#7A5C1E` the ratio reaches ~5.0:1 (AA). Alternatively, restrict `#9E7C2E` to large text (18pt+ or 14pt bold) where the 3:1 threshold applies.

**SECONDARY WARNING**: `#777570` secondary text on light shell (4.3:1) also fails AA for normal text. Darkening to `#706E6B` or `#6A6866` would bring it above 4.5:1.

### Engine Count Consistency

The spec refers to "73 engines" throughout (Section 1.1.1 "73 Colors Are 73 Languages", etc.). CLAUDE.md confirms 73 registered engines. **CONFIRMED**.

---

## DOCUMENT 2: PlaySurface Design Spec

### MPC Pad Layout

**CONFIRMED** — Pad 1 = bottom-left, Pad 16 = top-right (standard MPC layout). The spec explicitly states this and the pad diagram correctly shows:
```
Pads 13-16 (top row)
Pads  9-12
Pads  5-8
Pads  1-4  (bottom row)
```

The MIDI note table (Section 3.8) shows Pad 1 = MIDI note 37 (C#2) as "Kick A" in MPC Default. The inline text says "Pad 1 = MIDI note 37 (C#2, MPC convention for kick)". This is consistent with Akai's actual MPC Bank A layout.

**CONFIRMED** — MPC pad numbering and MIDI mapping are consistent and correct.

### MPE Specification

**WARNING — Channel count**: The spec states "Lower Zone (channels 2-8, 7 voices)" as the default MPE configuration. The MPE specification (MIDI Polyphonic Expression, ratified by the MIDI Association) defines the Lower Zone as channels 2-16 (15 member channels), with Manager channel = 1. A configuration of channels 2-8 is a valid *constrained* MPE zone for 7-voice polyphony, but it is NOT the standard default. The standard default Lower Zone would support 15 voices (channels 2-16).

This is a deliberate design choice (limiting to 7 voices for performance headroom), but the spec should clarify it is a constrained zone, not the "standard" MPE default. Many MPE controllers (Seaboard, Linnstrument) send on all 15 channels by default and would need to be reconfigured to work within a 7-voice zone. The spec's configurable "Upper Zone or Full MPE" options do not address 15-voice Lower Zone.

**Recommended fix**: Change the description to "Lower Zone (channels 2-8, 7 voices — constrained for CPU headroom; standard Lower Zone is channels 2-16, 15 voices)."

**CONFIRMED** — Pitch bend range of +/-48 semitones (configurable) is consistent with the MPE spec (which allows any range). The default of +/-2 is standard.

**CONFIRMED** — CC 74 (Brightness) for Slide, and Channel Pressure for per-note pressure, are the correct MPE mappings per the spec.

### JUCE Component Count and Performance Budget

The spec claims <4% total CPU for the PlaySurface. Individual component budget:
- XOuija background: 0% (cached image)
- XOuija Planchette + trails + ripples: <1%
- MPC pad grid: <0.3% per active pad
- Seaboard keyboard: <1%
- Expression controllers: <1%
- Tide Controller: <0.2%

**CONFIRMED** — Total budget of <4% CPU for the PlaySurface is realistic given the strategies specified (cached gradient images, no per-key child components for Seaboard, `fillEllipse` instead of `Path` for trails). These are proven JUCE optimization patterns.

**NOTE**: The spec separately claims XOuija at "<3% CPU on repaint" in Section 2.5 (Lucy's assessment) and "<2ms at 60fps" (equivalent to ~<12% of a 16.7ms frame budget at 60fps) in Section 2.6.2. The "<3% CPU" and "<2ms" numbers are not in direct conflict but are measuring different things (CPU percentage vs. paint time). The broader JUCE Implementation Guide (Appendix D.5) caps XOuija at "<2ms" as part of a total-per-frame budget of "<8ms." This is internally consistent.

### Platform Coverage

All three surfaces (XOuija, MPC Pads, Seaboard) have platform-specific notes for:
- Desktop AU: CONFIRMED (Section 6.1, keyboard shortcuts, mouse interactions)
- iPad AUv3/Standalone: CONFIRMED (Section 6.2, multi-touch, haptics, edge swipe drawers, safe areas)
- iPhone: CONFIRMED in the main UI spec Section 3.3 (simplified layout, tab-based navigation)
- MPCe (MPC Live III / MPC Key 61): CONFIRMED (Section 6.3, Q-Link mapping, touch strip, 7" screen notes)

**CONFIRMED** — All platforms have notes. iPhone is specified in the main UI spec (Section 3.3) rather than the PlaySurface spec itself; the PlaySurface spec focuses on the three primary surfaces only.

### MPC Velocity Curve Discrepancy

**WARNING**: Section 3.8 states "The MPC's logarithmic curve (called 'Curve 2' in MPC software) uses `v = 127 * pow(input/127, 0.6)`." However, Section 3.4 defines XOlokun's "Logarithmic" curve as `v = log(1 + input*9) / log(10)`.

These are different mathematical functions and will produce different output values. The spec states both must match for authentic MPC feel, but only provides the MPC formula in Section 3.8 — the "Curve 2 equivalence" requirement contradicts defining a different logarithmic formula in Section 3.4.

**Recommended fix**: In Section 3.4, replace the logarithmic curve formula with the MPC-equivalent `v = pow(input, 0.6)` (normalized form of the Kai-specified formula), and rename it "MPC Curve 2 (Logarithmic)" for clarity.

---

## DOCUMENT 3: Asset Registry

### File Path Verification (10 random paths)

All 10 paths verified as existing on disk:

| Path | Status |
|------|--------|
| `~/Downloads/Audio UI/KNOBS-SET/knob-set-09/` | EXISTS (contains Knob-01/02/03-Filmstrip.png) |
| `~/Downloads/Audio UI/KNOBS-SET/knob-set-08/` | EXISTS (contains Knob-01/02/03-Filmstrip.png) |
| `~/Downloads/Audio UI/KITS/kit-07/` | EXISTS (contains big-knob, light-knob, small-knob) |
| `~/Downloads/Audio UI/KITS/kit-28/` | EXISTS (contains Wheel, knobs, sliders) |
| `~/Downloads/Galactic Gradients/` | EXISTS (contains Galactic-Upscale folder) |
| `~/Downloads/Cubic Glass Gradient/` | EXISTS (contains 20 JPG reference images) |
| `~/Downloads/Audio UI/BUTTONS/Button-01/` | EXISTS (Nickel + Silver subdirs) |
| `~/Downloads/Audio UI/SLIDERS/metal-slider-set-02/` | EXISTS (Metal Fader-01 through 05) |
| `~/Downloads/Audio UI/VU-METER/vu-meter-03/` | EXISTS (Vu Meter-Filmstrip.png) |
| `~/Downloads/LEDboard/` | EXISTS (full A-Z, 0-9, symbol set, on/off pairs) |

**CONFIRMED** — All 10 verified paths exist on disk.

### Additional Path Check (Fonts)

| Path | Status |
|------|--------|
| `~/Downloads/nebulica_.../OTF/` | EXISTS (9 weights: Thin through ExtraBold) |
| `~/Downloads/chrys_sans_serif.../OTF/` | EXISTS (Chrys-Distorted, Light, Regular, Thin) |
| `~/Downloads/Audio UI/FIGMA-KITS/Mobile_Application_Audio_GUI/` | EXISTS |
| `~/Downloads/Hero Gradients v1/` | EXISTS (36 images) |
| `~/Downloads/Audio UI/LIGHT/Light-Set-01/` | EXISTS |

**CONFIRMED** — All font paths verified.

### Knob Count Accuracy

The spec claims "93 styles" of individual knobs. A count of all entries in the knob catalogue table yields exactly 93 entries. The knob numbers have gaps (4, 8, 13, 22, 36-40, 68, 84, 89-90 are absent), confirming these are vendor product numbers, not a sequential count. The 93-count claim is accurate.

**CONFIRMED** — 93 knobs are catalogued. The count is correct.

### Path Naming Inconsistency — XOlokun vs XOmnibus

**ERROR**: Multiple paths in the asset registry use the repository name `XO_OX-XOlokun` (the renamed version), but the actual repository on disk is `XO_OX-XOmnibus`:

- `~/Documents/GitHub/XO_OX-XOlokun/Site/design-tokens.css` → should be `XO_OX-XOmnibus/Site/design-tokens.css`
- `~/Documents/GitHub/XO_OX-XOlokun/Site/engine-creature-map.json` → same issue
- `~/Documents/GitHub/XO_OX-XOlokun/Site/fonts/HINOAloraglyphs.otf` → same issue
- `~/Documents/GitHub/XO_OX-XOlokun/Docs/mockups/xolokun-main-ui.html` → same issue
- `~/Documents/GitHub/XO_OX-XOlokun/Site/img/led/` → same issue

All five paths use `XO_OX-XOlokun` which does not match the actual repo directory `XO_OX-XOmnibus`. The CLAUDE.md notes the product was renamed from XOmnibus to XOlokun on 2026-03-24, but the repo folder itself has not been renamed (still at `~/Documents/GitHub/XO_OX-XOmnibus/`). These paths would fail at implementation time.

**Recommended fix**: Update all 5 instances in asset-registry.md §12 and §13 from `XO_OX-XOlokun` to `XO_OX-XOmnibus`.

### Engine Color Count Discrepancy

**WARNING**: The Asset Registry §12.1 describes `design-tokens.css` as having "71 Engine Colors" (and the Quick-Access table at §Quick-Access calls out "71 engine colors"). However, CLAUDE.md and the Definitive UI Spec both reference **73 registered engines**. The 2-engine discrepancy corresponds to OXYTOCIN (added 2026-03-23) and OUTLOOK (added 2026-03-23) — the two most recently added engines, which were added after the asset registry was written.

The asset registry predates those two engine additions. The `design-tokens.css` file (if it has not been updated) would be missing OXYTOCIN (`#9B5DE5` Synapse Violet) and OUTLOOK (`#4169E1` Horizon Indigo).

**Recommended fix**: Update the asset registry description of `design-tokens.css` to "73 Engine Colors" and verify the actual CSS file has been updated with the two new engine accent colors.

### JUCE Translation Guide

The JUCE Translation Notes section (Lucy's Reference) references valid APIs throughout:
- `juce::ImageCache::getFromMemory()` with BinaryData — valid
- `drawRotarySlider` override on `LookAndFeel_V4` — valid
- `juce::Drawable::createFromSVG()` — valid
- `juce::AccessibilityHandler` with role + value interface — valid

**CONFIRMED** — JUCE Translation Guide references valid APIs.

---

## DOCUMENT 4: Blessing Session

### Ghost Verdicts Consistency

Checked all 8 ghost verdicts against the seance doctrines established in CLAUDE.md:

| Ghost | Verdict | Consistency |
|-------|---------|-------------|
| Moog | YES (unconditional) | Consistent with UI-DOC-001 (macro primacy). His concerns about full label text and MIDI learn are valid but not doctrine violations. |
| Buchla | CONDITIONAL YES | Condition (persistent coupling arc strip) is novel UI requirement not in existing doctrines; consistent with his "arcs visible" concern. |
| Smith | YES (provisional on V1.1) | Consistent with preset navigation doctrine. |
| Kakehashi | YES | Consistent with "30-second test" and "Sound on First Launch" doctrine. |
| Ciani | YES | Consistent with performance mode and expression controller requirements. |
| Neve | YES | Consistent with knob hierarchy and value readout requirements. |
| Tomita | YES | Consistent with 73-color system and organism quality. |
| Rams | CONDITIONAL YES | Condition (V1 feature priority matrix) is design governance, not a doctrine change. |

**CONFIRMED** — All 8 ghost verdicts are internally consistent with their established positions in prior seance history.

### Producer Guild Top 5 Features vs ui-producer-needs.md

**WARNING — Source file missing**: The task asks to verify Producer Guild top 5 features against `ui-producer-needs.md`. That file does **not exist** in `Docs/design/`:

```
Docs/design/
  accessibility-audit.md
  asset-registry.md
  button-system-spec.md
  input-state-matrix.md
  kai-akai-tool-ui-review.md
  outshine-empty-state.md
  playsurface-design-spec.md
  toast-notification-system.md
  xolokun-definitive-ui-spec.md
  xolokun-ui-blessing-session.md
  xomnibus_design_guidelines.md
  xomnibus_ui_master_spec_v2.md
```

The `ui-producer-needs.md` referenced in the checklist is absent. The blessing session itself (Part II) defines the 5 top features inline: (1) Acoustic Preset Browser with DNA — 24/25, (2) 2D Sonic Space Engine Navigator — 21/25, (3) Performance Lock Mode — 19/25, (4) Live Modulation Visibility — 18/25, (5) Sound Character Search — 17/25. These can only be validated against the blessing session itself (they are self-referential). The source document that originally produced these priority scores is not on disk.

**Recommended action**: If `ui-producer-needs.md` existed in a prior session and was used to generate the Producers Guild section, it should be recreated or the blessing session should document its source explicitly.

### New Blessings B041-B043 Conflict Check

Checked B041, B042, B043 against the full blessing history (B001-B040) in CLAUDE.md:

**B041 — Dark Cockpit Attentional Design**:
- No conflict with any prior blessing. No prior blessing addresses UI attentional management. Ratified 8-0 in the blessing session matches the CLAUDE.md record.
- The 5 opacity levels (100%/80%/45%/20%/0%) do not conflict with any engine DSP blessing.
- **CONFIRMED**

**B042 — The Planchette as Autonomous Entity**:
- No conflict with any prior blessing. No prior blessing addresses play-surface cursor behavior.
- Vote 7-1 (Rams dissenting) matches the CLAUDE.md record and the blessing session text.
- **CONFIRMED**

**B043 — Gesture Trail as First-Class Modulation Source**:
- Vote 6-2 (Moog and Rams dissenting) matches CLAUDE.md record and blessing session text.
- No conflict with prior blessings. The gesture-trail-as-mod-source concept is novel and not addressed by any engine-level blessing.
- **NOTE**: B043 is assigned to "UI" in CLAUDE.md. The blessing session (Part VI) correctly states the trail ring buffer of "256 (x, y, velocity, time) tuples." The Definitive UI Spec (Section 5.1.1) specifies "256 `(float x, float y, float vel, double time)` tuples" — these match. However, Appendix D.5 (performance budget) does not include the gesture trail modulation source as a separate component. If the trail is promoted from visual-only to a DSP modulation output, it should appear in the performance budget table as a separate line item (estimated cost: negligible, as stated in the spec).
- **CONFIRMED** — No conflicts with existing blessings.

### Path to 10.0 Specificity and Actionability

P1 through P8 evaluated for specificity:

| Item | Actionable | Notes |
|------|-----------|-------|
| P1: Feature Priority Matrix (Appendix F) | YES | Proposed V1/V1.1/V1.2/V2 breakdown is specific and immediately writeable |
| P2: Performance Lock Mode (Section 2.10) | YES | Toggle location (Status Bar), state behavior, Cmd+L shortcut all specified |
| P3: Persistent Coupling Arc Strip | YES | 80pt height, collapsible to 8pt, trigger condition (>0.1) specified |
| P4: Full Macro Names (CHAR→CHARACTER etc.) | YES | Location (header L1, engine panel L1) specified; abbreviations reserved for Macro Strips only |
| P5: Floating Value Bubble During Drag | YES | Dimensions (48x20pt), timing (disappear 1.5s after drag), font, placement all specified |
| P6: Preset Save Workflow | YES | Button location (PRESET tab bottom), naming dialog, overwrite protection all specified |
| P7: DNA Character Search Keywords | YES | 12 keywords, per-keyword DNA threshold mappings, compound query support all specified |
| P8: Spectral Silhouette in Core Spec | YES | Implementation (128-float array, 5% opacity path, V1 promotion) fully specified |

**CONFIRMED** — All 8 Path to 10.0 items are specific and actionable as written.

---

## CROSS-DOCUMENT CONSISTENCY

### OPERA Accent Color

- UI Spec Section 1.1.1: `#D4AF37` (Aria Gold)
- PlaySurface Spec Section 2.2: `#D4AF37` (Aria Gold)
- CLAUDE.md engine table: `#D4AF37` (Aria Gold)

**CONFIRMED** — OPERA color is consistent across all documents.

### Planchette Specifications

The Planchette is described in both the UI Spec (Section 1.2) and the PlaySurface Spec (Section 2.2). Key parameters cross-checked:

| Parameter | UI Spec | PlaySurface Spec | Match? |
|-----------|---------|-----------------|--------|
| Size | 80x60pt | "approximately 80x60pt" | CONFIRMED |
| Idle drift frequency | a=0.3Hz, b=0.2Hz | "~0.3 Hz cycle" (simplified) | CONFIRMED |
| Lock-on timing | 150ms cubic-bezier(0.34, 1.56, 0.64, 1) | "150ms ease-in (cubic bezier)" | CONFIRMED |
| Release hold | 400ms | 400ms | CONFIRMED |
| Drift amplitude | 15% of surface | 15% of surface width | CONFIRMED |
| Trail points | 12 TrailPoint structs | "8-12 points" | MINOR DISCREPANCY — UI spec specifies exactly 12; PlaySurface spec says 8-12. The ring buffer implementation in both uses 12 points (verified in Section 2.6.2 of UI spec and Section 2.5 implementation details). |
| Ripple rings | 3 rings, 300ms | 3 rings, 300ms | CONFIRMED |
| Trail fade | 1.5s decay | 1.5s | CONFIRMED |

**MINOR NOTE**: Trail point count described as "8-12 points" in PlaySurface spec vs. "12 TrailPoint structs" in UI spec. Suggest standardizing to 12 in the PlaySurface spec for precision.

### Knob Sizes

| Context | UI Spec | PlaySurface Spec | Match? |
|---------|---------|-----------------|--------|
| Macro knobs (L1) | 36-48pt (header: 36pt, engine panel: 48pt) | 36x36 in bottom bar | CONFIRMED |
| Mod Wheel | 28x120pt | 28pt wide, 160pt tall | DISCREPANCY — UI Spec Appendix D layout says 28x120pt; PlaySurface Spec Section 5.1 says "28pt wide, 160pt tall." 40pt difference in height. |

**WARNING — Mod Wheel height inconsistency**: The expression controller panel layout in UI Spec Section 2.6.5 lists Mod Wheel as "28x120pt" but the PlaySurface Spec Section 5.1 specifies the Mod Wheel as "28pt wide, 160pt tall." One of these is wrong. The expression controller panel height is 236pt total (from Section 2.6.5), which must fit: Mod Wheel + Pitch Bend (at same height) + 4 Macro Strips + Tide Controller (100pt dia). 160pt for the wheels would leave only 76pt for 4 strips (minimum 80pt needed at 20pt per strip with spacing) plus 100pt for Tide Controller — this would overflow. The 120pt figure is more likely correct for the desktop layout.

### Coupling Tab — Coupling Types Count

UI Spec Section 2.5.3 lists 15 coupling types: "Amplitude, Filter, Pitch, Ring, FM, Sync, Waveshape, Granular, Spectral, Formant, Reverb, Delay, Knot, Triangular, Adversarial." CLAUDE.md confirms "15 coupling types incl. KnotTopology + TriangularCoupling." **CONFIRMED**.

### Tone — `SensorManager` Reference

**ERROR — Wrong API**: UI Spec Section 1.3 (Tide Controller) states "Tilt (iPad gyroscope via `SensorManager`)". `SensorManager` is an Android API. The correct iOS API for gyroscope/accelerometer access is `CMMotionManager` from the Core Motion framework (`import CoreMotion`). This is an incorrect API reference that would cause a build failure on iOS.

**Recommended fix**: Replace `SensorManager` with `CMMotionManager` (Core Motion) in Section 1.3 of the UI spec.

---

## SUMMARY OF FINDINGS

### Critical Errors (must fix before implementation)

| # | Document | Section | Issue |
|---|----------|---------|-------|
| C1 | UI Spec | §4.1.1 | `#9E7C2E` on `#F8F6F3` calculates to 3.6:1, NOT 4.8:1 — false WCAG AA claim for gold text on light background |
| C2 | UI Spec | §1.3 | `SensorManager` is an Android API. iOS gyroscope uses `CMMotionManager` (Core Motion) |
| C3 | Asset Registry | §12, §13 | 5 file paths use `XO_OX-XOlokun` repo name — actual disk directory is `XO_OX-XOmnibus` |

### Warnings (should fix before shipping)

| # | Document | Section | Issue |
|---|----------|---------|-------|
| W1 | UI Spec | §4.1.1 | `#777570` secondary text on light shell calculates to 4.3:1, not 4.7:1 — fails AA for normal text |
| W2 | UI Spec | §4.1.1 | Several other contrast ratios are slightly off (10.4:1 stated as 7.2:1, 1.5:1 stated as 2.4:1) — errors are conservative (actual contrast is equal or better) except for C1 and W1 |
| W3 | PlaySurface | §4.3 | MPE Lower Zone specified as "channels 2-8, 7 voices" — needs clarification that this is a constrained zone, not the MPE standard default (which is channels 2-16, 15 voices) |
| W4 | PlaySurface | §3.4 + §3.8 | Two different logarithmic velocity curve formulas given; the "XOlokun Logarithmic" curve must match the MPC Curve 2 formula for authentic feel |
| W5 | Asset Registry | §12.1 + Quick-Access | Engine color count says 71; CLAUDE.md and UI Spec say 73 — OXYTOCIN and OUTLOOK colors may be missing from `design-tokens.css` |
| W6 | UI Spec | §2.4.1 | Melatonin Blur JUCE library used for frosted glass — untracked external dependency |
| W7 | PlaySurface ↔ UI Spec | §2.6.5 vs §5.1 | Mod Wheel height: UI Spec says 120pt, PlaySurface Spec says 160pt |

### Notes (informational, no action required)

| # | Document | Section | Note |
|---|----------|---------|------|
| N1 | UI Spec | Appendix B | Fonts "Monaspace", "Michroma", and "APERTURE" (as listed in the validation checklist) do not appear in any active component spec. Aperture 3.2 exists in the Asset Registry as REFERENCE tier only. |
| N2 | Blessing Session | Part II | `ui-producer-needs.md` (referenced source for Producer Guild priority scores) does not exist in `Docs/design/`. The scores are documented inline but their derivation is opaque. |
| N3 | PlaySurface | §2.2 | Trail described as "8-12 points" — standardize to exactly 12 to match the UI Spec and ring buffer implementation |
| N4 | UI Spec | §5.1.1 | B043 gesture trail DSP cost not reflected in Appendix D.5 performance budget table (estimated cost: negligible; not a problem, just missing) |
| N5 | Asset Registry | §4.2 | Knob-Set-09 only contains 3 filmstrips (Knob-01/02/03), not "5 knobs per set" as the intro claims. Set-09 is described in the intro as a "5-knob family" but the actual files on disk show 3 filmstrips. This may mean 2 sizes are missing or the count includes the OBJ 3D source files. Verify before implementation. |

---

*Sweep conducted: 2026-03-24*
*All 4 documents read in full. 10 asset paths verified on disk. Contrast ratios independently computed.*
