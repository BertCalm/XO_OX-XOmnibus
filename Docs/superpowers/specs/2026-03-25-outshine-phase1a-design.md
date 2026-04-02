# Outshine Phase 1A — The Core Loop
### "Drop a grain. The Oyster does the rest."

**Date:** 2026-03-25
**Revision:** 2 — incorporates DA review, Kai's 7-android audit, and Synth Seance ghost council findings
**Status:** Approved
**Authors:** XO_OX Design Council
**Metaphor:** The Oyster (tool) takes Grains (raw samples) and produces Pearls (finished instruments)

---

## Table of Contents

1. [Overview](#1-overview)
2. [Phase 1A Split: Alpha and Beta](#2-phase-1a-split-alpha-and-beta)
3. [The Oyster Metaphor](#3-the-oyster-metaphor)
4. [Window Architecture](#4-window-architecture)
5. [Responsive Input Panel](#5-responsive-input-panel)
6. [Data Flow Pipeline](#6-data-flow-pipeline)
7. [MPE Expression Mapping](#7-mpe-expression-mapping)
8. [XPN Output Specification](#8-xpn-output-specification)
9. [Pearl Preview Panel](#9-pearl-preview-panel)
10. [Shell State (Empty State)](#10-shell-state-empty-state)
11. [Sidebar Mini-Panel](#11-sidebar-mini-panel)
12. [Component Tree](#12-component-tree)
13. [Design Token Usage](#13-design-token-usage)
14. [Keyboard Navigation](#14-keyboard-navigation)
15. [State Persistence](#15-state-persistence)
16. [Error Handling](#16-error-handling)
17. [Phase Boundaries](#17-phase-boundaries)
18. [Backend Wiring](#18-backend-wiring)
19. [Known Limitations (Phase 1A)](#19-known-limitations-phase-1a)
20. [Hardware Verification Checklist](#20-hardware-verification-checklist)

---

## 1. Overview

Outshine is a sample instrument forge integrated into XOceanus. It converts raw audio samples (grains) into fully mapped, MPE-ready instrument programs (pearls) — keygroup or drum — packaged as `.xpn` archives compatible with MPC hardware.

**Phase 1A scope:** Auto-classification, melodic keygroup builder, MPE expression routing, and XPN export. This is the functional daily-driver: drop samples, get instruments.

### Primary User

MPC power user operating MPC Live II, MPC III, MPC One+, or MPC Control. Likely paired with a Roli Seaboard or other MPE controller. Workflow centers on dropping melodic samples into a keygroup and getting an immediately playable, expression-routed instrument without manual zone mapping.

### Primary Workflows

1. **Melodic samples → keygroup instrument:** Drop a folder of Rhodes multi-samples; receive a fully zoned, velocity-layered, MPE-wired keygroup program.
2. **XPN upgrade:** Drop an existing XPN; re-analyze, re-map, and re-export with improved zone boundaries, MPE expression routes, and LUFS normalization.

### What Exists

The backend pipeline is already implemented:

- `Source/Export/XOutshine.h` — 1,018 lines of C++. The canonical implementation.
- `Tools/xoutshine.py` — 2,042 lines of Python CLI. The prototype. C++ is authoritative when they diverge.

**This spec covers only the JUCE UI window and its wiring to the existing backend.** Backend hardening items identified by triple review (Phase 1A-alpha) must be complete before UI work (Phase 1A-beta) begins.

---

## 2. Phase 1A Split: Alpha and Beta

Phase 1A is divided into two sequential sub-phases based on findings from the DA review, Kai's 7-android audit, and the Synth Seance ghost council. All alpha items must ship before any beta UI work begins.

---

### Phase 1A-alpha: Backend Hardening

Fix all 9 P0 backend defects before touching the UI layer.

#### P0 Backend Fixes (Required — blocks all UI work)

| # | Fix | Description |
|---|-----|-------------|
| 1 | Multi-zone keygroup builder | Implement zone-boundary algorithm with midpoint rule: each zone's low boundary = midpoint between its root note and the previous zone's root note. Zone boundaries are inclusive on the lower end, exclusive on the upper end. |
| 2 | YIN pitch detection | Replace raw autocorrelation with the YIN algorithm (de Cheveigne & Kawahara 2002). YIN is more robust on inharmonic content (bells, metallic hits, prepared piano). Autocorrelation fails silently on these — YIN at least degrades gracefully. Falls back to C4 on detection failure; logs warning. |
| 3 | Bit depth clamping | Force 24-bit integer output on all exported WAVs. Convert 32-bit float sources to 24-bit during Stage 5 (NORMALIZE). Do not pass 32-bit float WAVs to the packager — MPC hardware rejects them silently. |
| 4 | Sample rate guard | Downsample 96 kHz sources to 48 kHz before packaging. Reject sources above 96 kHz with a user-facing error: "Sample rate {N} kHz is not supported. Maximum is 96 kHz." Never upsample. |
| 5 | KeyTrack=False on drum programs | Drum programs must have `KeyTrack=False` on all layers. The existing pipeline writes `KeyTrack=True` universally — this causes drum samples to detune on MPC when played at non-root notes. |
| 6 | RootNote: write detected pitch | Write the YIN-detected pitch as RootNote. Do not hardcode 0 or 60. `RootNote=0` triggers MPC auto-detection which is unreliable on multi-zone keygroups. See Section 8 for the full rationale. |
| 7 | Channel validation | All layers within a keygroup must have matching channel counts (all mono or all stereo). Mixed mono/stereo layers cause MPC to reject the program. Validate at Stage 1 (INGEST); flag mismatches as errors before the pipeline runs. |
| 8 | Streaming ZipFile output | Rewrite the Stage 7 packager to use streaming ZIP construction — write one file at a time rather than loading all WAV buffers into memory simultaneously. A 50-sample kit at 24-bit/48 kHz can reach 500 MB in memory; streaming keeps the peak footprint to a single WAV buffer. |
| 9 | Staged public API (Option B required) | `analyzeGrains()` and `exportPearl()` are REQUIRED public methods, not a recommended option. The Pearl Preview Panel (Section 9) cannot populate without intermediate state from `analyzeGrains()`. Option A (monolithic `run()`) is insufficient for the UI design. See Section 18 (Backend Wiring) for full API specification. |

#### P1 Backend Fixes (Required before Phase 1A-beta ships)

| # | Fix | Description |
|---|-----|-------------|
| 10 | Expand SampleCategory enum | Add `Woodwind`, `Brass`, `Vocal` to `SampleCategory`. Total: 18 categories (was 15). Update `classifyByName()` with keyword patterns for each. |
| 11 | Per-category LUFS targets | Replace the single `lufsTarget` setting with per-category targets (see table in Section 6, Stage 5). |
| 12 | Per-category true-peak ceilings | Replace the single `−0.3 dBTP` ceiling with per-category values (see table in Section 6, Stage 5). |
| 13 | Round-robin pitch variation | Fix: ±3 cents (not ±5). ±5 cents is audible as intentional detuning on sustained melodic content; ±3 is below the threshold of perception while still preventing machine-gun. |
| 14 | Micro-gain variation | Fix: ±0.5 dB (not ±0.25 dB). ±0.25 dB is inaudible on most material and fails to prevent machine-gun on identical consecutive hits. |
| 15 | Velocity filter taper | Fix: smooth taper replacing the hard cutoff at `t=0.7`. The hard cutoff creates an audible brightness jump at velocity 89/127. Use a raised cosine or similar smooth transition window over the 0.6–0.8 range. |
| 16 | Choke group implementation | Fix: use `ChokeSend` / `ChokeReceive` attributes instead of a single `MuteGroup`. MPC interprets these differently — `MuteGroup` applies mutual choking; `ChokeSend` / `ChokeReceive` allow directional choking (closed hat chokes open hat, not the reverse). |
| 17 | XML-escape ProgramName | Run `juce::XmlElement::createTextElement()` or equivalent escaping on the program name before inserting into XPM XML. Unescaped `&` / `<` / `>` in user-typed names will produce malformed XML that MPC rejects. |
| 18 | Fix XOriginate.h include | Resolve the circular include dependency in `XOriginate.h` before integrating Outshine UI. This is a build-time blocker on some compilers. |
| 19 | FAT32 filename sanitization | Strip characters illegal on FAT32 from all output filenames: `? < > : " / \ | *`. Replace with underscore. MPC internal storage uses FAT32. |
| 20 | Add Loop category | Add `Loop` to `SampleCategory` enum and `classifyByName()`. Loops are a distinct use case: they do not benefit from one-shot treatment and need loop point detection rather than tail-length analysis. |

---

### Phase 1A-beta: UI Build

Build the JUCE UI on the hardened backend. All P0 and P1 backend fixes must be complete before this phase begins.

**UI deliverables:**

- `DocumentWindow` (NOT `CallOutBox`) — resizable, 900×660 default, 700×500 minimum. See Section 4 for full specification.
- Responsive input panel with Mode A/B breakpoint at 700px width
- All 10 UI components as specified in Section 12
- Audio preview: "Play C4" button that triggers the current zone mapping through JUCE audio output
- Live MPE expression animation: MIDI callback updates the MPE panel expression bars in real time during playback
- Pipeline time estimate on the progress bar: "Analyzing 12 samples, ~8 sec"
- Dual progress strings: metaphor headline + technical stage shown on separate lines
  - Line 1 (metaphor): "Pearl forming..."
  - Line 2 (technical): "Stage 6/8: Building XPM programs"
- Zone boundary rule displayed in tooltip: "Inclusive lower, exclusive upper"
- Velocity layers: hard steps only (no crossfade — MPC hardware limitation, documented in Section 19)
- Rebirth Mode teaser: dimmed "Rebirth Mode (Phase 1B)" section in the Pearl Preview Panel, not interactive
- "Browse Files" toggle reserved space in spectrum view area (Phase 1B placeholder — visible but disabled)
- XPN upgrade warning toast: "Original program data will be rebuilt from scratch. Zone map, expression routes, and LUFS normalization will be regenerated."
- Unknown classification UX: contextual gold highlight + auto-scroll to first unresolved row
- Sidebar drop auto-triggers CLASSIFY + ANALYZE pipeline stages immediately (not just opens the window)
- Per-run time estimate displayed before pipeline starts

---

## 3. The Oyster Metaphor

The metaphor is structural. Every UI string, status label, and community touchpoint derives from it.

| Concept | Meaning |
|---|---|
| **The Oyster** | Outshine — the tool that transforms |
| **The Grain** | Raw sample input (WAV file, folder, or XPN archive) |
| **Nacre Layering** | Velocity layers + engine processing applied to the grain |
| **The Pearl** | Finished instrument output (XPN or XPM program) |

The name "Outshine" carries the payload: the pearl outshines the grain it started as.

### UI Language (Pipeline Status Strings)

These strings appear in the progress area and sidebar status:

| Stage | Display String |
|---|---|
| Empty / waiting | "The shell is open." |
| Processing | "Layering nacre..." |
| Mapping zones | "Pearl forming..." |
| Complete | "Pearl complete." |
| Exported | "Pearl & Export" (button label) |

### Community Language

These are the phrases that emerge organically when users share their work:

- "I pearled that Rhodes through OPERA"
- "Drop your best grain in #pearl-sharing"
- "This bass pearl hits different"

Do not force these into the UI. They belong in onboarding copy, release notes, and community channels.

---

## 4. Window Architecture

### Entry Point

Outshine is launched from the **EXPORT sidebar tab** (Column C, tab index 5) via an "Open the Oyster" button inside `OutshineSidebarPanel`.

The full window is a `juce::DocumentWindow` with a custom XO Gold title bar (no native chrome). It is launched with:

```cpp
// In OutshineSidebarPanel — create once, show/raise on subsequent clicks
if (outshineWindow == nullptr)
{
    outshineWindow = std::make_unique<OutshineDocumentWindow>(processor);
    outshineWindow->centreWithSize(900, 660);
}
outshineWindow->setVisible(true);
outshineWindow->toFront(true);
```

The window uses `juce::DocumentWindow` so it can be resized, minimized, and closed independently of the plugin editor. It is NOT anchored to the button (no arrow/callout chrome). It positions centered on screen.

**Why not CallOutBox:** `CallOutBox` is not resizable, has fixed dimensions, and is anchored to a UI element — all three of which conflict with Phase 1A requirements (resizable, 900×660 default, free-floating). `DocumentWindow` is the correct primitive.

**Double-Launch Guard:** Track the window's existence via a `juce::Component::SafePointer<OutshineDocumentWindow>` member in `OutshineSidebarPanel`. When the SafePointer becomes null (window closed), the guard resets. If the window is already open, `setVisible(true)` + `toFront(true)` brings it to front without creating a duplicate.

```cpp
juce::Component::SafePointer<OutshineDocumentWindow> outshineWindow;

void launchOutshine()
{
    if (outshineWindow != nullptr)
    {
        outshineWindow->toFront(true);  // already open — raise it
        return;
    }
    outshineWindow = new OutshineDocumentWindow(processor);
    outshineWindow->centreWithSize(900, 660);
    outshineWindow->setVisible(true);
}
```

### Window Dimensions

| Property | Value |
|---|---|
| Default size | 900 × 660 px |
| Minimum size | 700 × 500 px |
| Maximum size | 1600 × 1000 px |
| Resizable | Yes — `setResizable(true, true)` + `setResizeLimits(700, 500, 1600, 1000)` |
| Responsive breakpoint | 700px width (triggers Mode A ↔ Mode B switch) |
| Position | Centered on screen on first open; last position restored from state file on subsequent opens |

### Layout Regions (900×660 default)

```
┌─────────────────────────────────────────────────────────────────┐
│  HEADER: "OUTSHINE" + subtitle                         [×] close │
├──────────────────────────┬──────────────────────────────────────┤
│                          │                                       │
│   INPUT PANEL            │   PEARL PREVIEW PANEL                 │
│   (Left ~50%)            │   (Right ~50%)                        │
│                          │                                       │
│   [Drop Zone | Browse]   │   Classification summary              │
│                          │   Zone map visualization              │
│                          │   MPE expression panel                │
│                          │   Pearl name field                    │
│                          │   Export format selector              │
│                          │                                       │
├──────────────────────────┴──────────────────────────────────────┤
│  GRAIN STRIP: [Rhodes_C3 ×] [Rhodes_F3 ×] [Rhodes_A3 ×]   3    │
├─────────────────────────────────────────────────────────────────┤
│  [████████████████░░░░░░░░░░░░] 62%  Analyzing root notes...    │
└─────────────────────────────────────────────────────────────────┘
```

### Dependencies

| Dependency | File | Purpose |
|---|---|---|
| Design tokens | `GalleryColors.h` | Colors, spacing, typography |
| Look and feel | `GalleryLookAndFeel.h` | Knob/slider/button rendering |
| DSP pipeline | `Source/Export/XOutshine.h` | Classification, mapping, export |
| XPN packaging | `XPNExporter.h` | Archive assembly |

### Include Pattern

```cpp
// CORRECT — direct include, avoids circular dependency
#include "../GalleryColors.h"
#include "../GalleryLookAndFeel.h"
#include "../../Export/XOutshine.h"

// WRONG — do not include the editor header
// #include "../XOceanusEditor.h"  ← circular dependency
```

### Constructor Signature

```cpp
explicit OutshineWindow(XOceanusProcessor& processorRef);
```

The processor reference provides engine access for MPE routing and export pipeline integration.

---

## 5. Responsive Input Panel

The left half of the main window is the input panel. Its layout responds to the window width.

### Mode A — Vertical Split (width >= 700px)

The input panel is split vertically into two equal columns:

**Left half: Drop Zone**
- Dashed border (1px, `borderGray()`, 8px radius)
- Center: oyster icon (user-sourced asset — use placeholder until provided)
- Primary label: "Drop grains here" (`GalleryFonts::body()`, `textDark()`)
- Supporting label: "WAV • Folders • XPN" (`GalleryFonts::body()`, `textMid()`)
- Accepts: `.wav` files, directories, `.xpn` archives via OS drag-and-drop
- On drag enter: border glow gold (`xoGold`), icon pulse (200ms ease-in-out)
- On drop: immediate transition to analysis state

**Right half: Folder Browser**
- Path bar at top with a back-arrow `←` button (left of path label)
- File list below path bar:
  - Folders: folder icon + name
  - WAV files: waveform icon + name + duration (right-aligned, `value()` font)
- Multi-select supported (Shift+click for range, Cmd/Ctrl+click for individual)
- Buttons at bottom of browser pane:
  - "Select All WAVs" — selects all `.wav` files currently visible
  - "Add Selected →" — moves selected files into the grain strip
  - "Recent" — opens a dropdown overlay showing the 20 most recent grain paths. This provides Recent Grains access in Mode A without requiring a tab switch.

Files from either the Drop Zone or the Folder Browser feed into the grain strip at the bottom.

### Mode B — Tabbed (width < 700px)

Three tabs replace the split layout:

| Tab Index | Label | Content |
|---|---|---|
| 0 | Drop Zone | Full-width drop zone with same behavior as Mode A left half |
| 1 | Browse Files | Full-width folder browser with same behavior as Mode A right half |
| 2 | Recent Grains | List of last 20 imported file paths (from persisted JSON) |

Selected files from any tab accumulate in the grain strip at the bottom. The grain strip is always visible regardless of active tab.

"Recent Grains" tab shows file paths as rows. Clicking a row re-adds that grain to the strip (if it still exists on disk). Files that no longer exist are shown dimmed with a "missing" indicator but remain in the list.

### Grain Strip (always visible, both modes)

A horizontal strip spanning the full window width, pinned above the progress bar.

- Shows selected grains as removable chips: `[Rhodes_C3 ×]`
- Chip font: `GalleryFonts::value()` (monospace)
- Chip background: `borderGray()` with 4px radius
- `×` button on each chip removes that grain from the selection
- Grain count displayed on the right side: e.g., `3 grains`
- Horizontal scrolling if chips overflow width
- Strip persists across resize/mode switch — grains are never lost on layout change

**Grain Removal Behavior:** Removing a grain from the strip triggers re-analysis of the remaining grains. If all grains are removed, the UI reverts to Shell State. Zone boundaries are recalculated based on the updated sample set.

---

## 6. Data Flow Pipeline

The pipeline is implemented in `XOutshine.h`. The UI's role is to invoke each stage, display progress, and surface any manual interventions needed. All 8 stages run sequentially on a background thread via `juce::Thread`.

### Stage 1: INGEST

**Input:** File paths from grain strip (WAVs, folders, XPN archives)
**Operation:** Unpack XPN archives (extract embedded WAVs), recursively scan folders for `.wav` files, catalog all samples into an `AnalyzedSample` list.
**Output:** Flat list of WAV file paths + basic metadata (filename, file size, format)
**UI status:** "The shell is open." → "Opening the grain..."

**XPN Upgrade Behavior:** When ingesting an existing `.xpn`, Outshine extracts WAV samples and **DISCARDS** the original XPM program XML. The pipeline re-analyzes from scratch — classification, root note detection, zone mapping, and expression mapping are all regenerated. This is intentional: the purpose of XPN upgrade is to rebuild instruments to Outshine's quality standard, not to preserve the original mapping.

### Stage 2: CLASSIFY

**Input:** WAV file paths
**Operation:** 3-pass detection:
1. Filename pattern matching (e.g., "kick", "snr", "hat", "Rhodes", "woodwind", "brass", "vocal")
2. Spectral feature analysis (centroid, rolloff, energy distribution)
3. YIN pitch detection (confirms melodic vs. percussive — see Stage 3 note)

**Output categories (18 total):**

| Category | Description |
|---|---|
| `Kick` | Kick drum |
| `Snare` | Snare drum |
| `HiHatClosed` | Closed hi-hat |
| `HiHatOpen` | Open hi-hat |
| `Clap` | Clap / snap |
| `Tom` | Tom drum |
| `Percussion` | Non-kit percussion |
| `FX` | Sound effects, risers, impacts |
| `Loop` | Audio loops — receive loop point detection, not one-shot treatment |
| `Bass` | Bass instruments |
| `Pad` | Pad / atmospheric |
| `Lead` | Lead synth / melody |
| `Keys` | Piano, Rhodes, organ-type |
| `Pluck` | Plucked string, marimba, harp |
| `String` | Bowed string |
| `Woodwind` | Flute, clarinet, saxophone, oboe, etc. |
| `Brass` | Trumpet, trombone, French horn, etc. |
| `Vocal` | Sung or spoken vocal samples |
| `Unknown` | Classification failed — requires manual override |

`Unknown` items are displayed with a gold highlight in the preview panel, auto-scrolled to, and a dropdown prompts manual selection before export is allowed. See Section 16 (Error Handling) for the full Unknown UX.

**Canonical keyword patterns for new categories (used in `classifyByName()` pass 1):**
- Woodwind: flute, clarinet, saxophone, sax, oboe, bassoon, recorder, piccolo, fife
- Brass: trumpet, trombone, french horn, tuba, cornet, flugelhorn, horn
- Vocal: vocal, voice, choir, singing, vox, acapella, spoken

**UI status:** "Layering nacre..."

### Stage 3: ANALYZE

**Input:** Classified samples
**Operation:**
- Root note detection via **YIN algorithm** (de Cheveigne & Kawahara 2002). YIN is preferred over raw autocorrelation because it is more robust on inharmonic content (bells, metallic hits, prepared piano, brass transients). YIN threshold: 0.15. Returns MIDI note number 0–127. When YIN confidence is below threshold (0.15), the sample is flagged as 'Root Unverified' in the AnalyzedSample struct. The zone map displays these with an amber warning icon. The user can manually set the root note via a dropdown on the flagged sample in the source panel. The pipeline proceeds with C4 as a provisional root, but the 'Pearl & Export' button shows a badge: '2 unverified roots' — clickable to scroll to the flagged samples. This prevents silent wrong-zone-map failures.
- RMS and peak amplitude measurement
- Tail length detection (time to −60 dBFS from peak)
- Loop point detection (zero-crossing candidates; only for `Loop` category samples)
- Formant bounds estimation (for safe transposition range)
- Channel validation: confirm all layers in each keygroup candidate have matching channel counts. Flag mono/stereo mismatches as errors at this stage (do not defer to packaging).
- Sample rate validation: flag sources above 96 kHz as errors. Sources at 96 kHz are tagged for downsampling in Stage 5.

**Output:** `AnalyzedSample` structs populated with pitch, amplitude, timing, channel count, and sample rate data
**UI status:** "Analyzing root notes..."

**Implementation note:** The current `XOutshine.h` `classifyByAudio()` uses zero-crossing rate only — no pitch detection. Implementing YIN root note detection is a **P0 backend fix** (Phase 1A-alpha item #2). Without it, all melodic samples fall back to C4 root and zone mapping will be incorrect.

### Stage 4: ENHANCE

**Input:** Analyzed samples
**Operation:**
- Round-robin micro-variation: pitch **±3 cents** (not ±5 — ±5 is audible as intentional detuning on sustained melodic content), saturation ±0.5 dB (percussive categories only — see note below), micro-gain **±0.5 dB** (not ±0.25 — ±0.25 dB is inaudible on most material and fails to prevent machine-gun on consecutive identical hits). **Saturation variation scope:** Saturation variation (±0.5 dB) applies to percussive categories only (Kick, Snare, HiHatClosed, HiHatOpen, Clap, Tom, Percussion). For melodic categories (Keys, Lead, Bass, Pad, String, Pluck, Woodwind, Brass, Vocal), saturation variation is disabled to preserve timbral integrity across round-robin variants.
- Fade guards: applied to all samples
- DC removal: high-pass at 10 Hz
- Velocity layer shaping: amplitude + smooth taper one-pole low-pass for soft layers. Use a raised cosine window over the 0.6–0.8 velocity range to avoid the brightness jump that occurs with a hard cutoff at t=0.7.

**Output:** Modified WAV buffers written to temp directory
**UI status:** "Pearl forming..."

### Stage 5: NORMALIZE

**Input:** Enhanced WAV buffers
**Operation:** Per-category LUFS normalization, per-category true-peak limiting, bit depth conversion, sample rate downsampling (96 kHz → 48 kHz), optional dither.

**Per-category LUFS targets:**

| Category | Target LUFS |
|---|---|
| `Kick`, `Snare`, `HiHatClosed`, `HiHatOpen`, `Clap`, `Tom`, `Percussion` | −14 LUFS |
| `Bass` | −16 LUFS |
| `Pad`, `String` | −18 LUFS |
| `Keys`, `Lead`, `Woodwind`, `Brass`, `Pluck`, `FX`, `Loop` | −14 LUFS |
| `Vocal` | −16 LUFS |

**Per-category true-peak ceilings:**

| Category | True-Peak Ceiling |
|---|---|
| `Kick`, `Snare`, `HiHatClosed`, `HiHatOpen`, `Clap`, `Tom`, `Percussion`, `Keys` | −0.5 dBTP |
| `Lead`, `Pluck`, `Woodwind`, `Brass`, `FX`, `Loop` | −0.4 dBTP |
| `Pad`, `String`, `Vocal`, `Bass` | −0.3 dBTP |

**Bit depth conversion:**
- Output is always 24-bit integer. Convert 32-bit float sources to 24-bit during this stage.
- Do NOT pass 32-bit float WAVs to the packager — MPC hardware rejects them silently.
- 16-bit sources are upsampled to 24-bit (no dither needed on upsample).
- 24-bit output uses TPDF dither only when downconverting from 32-bit float.

**Sample rate handling:**
- Sources at 44.1 kHz or 48 kHz: pass through unchanged.
- Sources at 96 kHz: downsample to 48 kHz using a high-quality sinc resampler.
- Sources above 96 kHz: pipeline aborts with error toast: "Sample rate {N} kHz is not supported. Maximum is 96 kHz."
- Never upsample — if a 44.1 kHz source and a 48 kHz source are in the same kit, do not upsample the 44.1 kHz file. Output matching files at their native rate; note the mismatch in the Validate stage log.

**Output:** Normalized, 24-bit, rate-compliant WAV files ready for packaging
**UI status:** "Normalizing levels..."

### Stage 6: MAP (`buildPrograms`)

**Input:** Normalized programs (velocity layers + round-robin variants)
**Operation:**
- Generate XPM program XML from `UpgradedProgram` structs (keygroup or drum format — see Section 8)
- Zone boundary computation: midpoint rule between adjacent root notes. **Boundary convention: inclusive lower, exclusive upper.** Zone from C3 to D#3 means the zone triggers on C3 and all notes up to and including D3, but D#3 belongs to the next zone. Implement consistently or zone overlap/gaps occur on hardware. **Single-sample edge case (n=1):** For a single-sample input, the zone map spans the full MIDI range (0–127) with the detected root note centered. The zone map visualization shows a single full-width zone with the root note labeled. No midpoint calculation is needed.
- Velocity layer assignment via `getVelocitySplits()` with configured `XPNVelocityCurve`. **Hard steps only — no velocity crossfade.** MPC hardware does not support velocity crossfade at the XPM level. Do not attempt to implement crossfade in the program XML; it will be silently ignored. Document this clearly to users (see Section 19, Known Limitations).
- MPE expression route assignment (category-aware defaults — see Section 7)
- Choke group assignment: use `ChokeSend` / `ChokeReceive` attributes (directional). `HiHatClosed` sends a choke signal to `HiHatOpen`. Do not use symmetrical `MuteGroup` — it chokes in both directions.
- `KeyTrack=True` for all melodic categories; `KeyTrack=False` for all drum/percussion categories.

**Output:** `.xpm` XML files written to temp directory
**UI status:** "Pearl forming..."

### Stage 7: PACKAGE

**Input:** Normalized WAVs + XPM files + pearl name
**Operation:**
- Sanitize all output filenames for FAT32: strip `? < > : " / \ | *` and replace with underscore. MPC internal storage uses FAT32 and will silently reject files with illegal characters.
- Write normalized WAV files with naming convention: `{SampleName}__v{velLayer}__c{rrIndex+1}.wav` (double underscore separators, 1-based round-robin)
- Generate `Manifest.xml` with program metadata. XML-escape all user-provided strings (ProgramName, pearl name) before insertion.
- Assemble `.xpn` archive using **streaming ZIP construction** — write one file at a time into the ZIP stream rather than loading all WAV buffers simultaneously. A 50-sample kit at 24-bit/48 kHz can exceed 500 MB in memory; streaming keeps peak footprint to a single WAV buffer.

**Output:** `.xpn` file (or folder) at user-selected export path
**UI status:** "Packaging..."

### Stage 8: VALIDATE

**Input:** Packaged programs
**Operation:**
- Verify no clipping (peak < 1.0) on any layer
- Verify DC offset removed (mean amplitude < 0.005)
- Verify fade guards applied (first sample amplitude < 0.01)

**Output:** Issue count (0 = clean export)
**UI status:** "Pearl complete." → progress bar 100% (on success)

---

## 7. MPE Expression Mapping

Every pearl ships with MPE expression pre-wired. Expression defaults are category-aware and assigned automatically during Stage 6 (MAP / buildPrograms). Manual override is deferred to Phase 1C Design Mode.

### What XPM Format Actually Supports

XPM program files support the following expression destinations natively:

| Expression | XPM Attribute | Notes |
|---|---|---|
| AfterTouch | `<AfterTouch><Destination>` + `<Amount>` | Single destination confirmed. Dual destinations need hardware verification (see Section 20). |
| ModWheel (CC1) | `<ModWheel><Destination>` + `<Amount>` | Supported. |
| PitchBendRange | `<PitchBendRange>` | Integer semitones. Default: 24. |

**CC74 (Slide) is NOT embeddable in XPM format.** CC74 routing is configured at the MPC track level, not in the program file. Outshine cannot write CC74 expression into the XPM XML. On MPC hardware, Slide expression requires manual MIDI routing configuration at the track level after loading the pearl.

This means the UI's "Slide (CC74)" expression row in the Pearl Preview Panel (Section 9) is informational — it shows Outshine's intent for the user to configure manually, not a value that is written to the file.

### Melodic Defaults (Keys, Lead, Pad, Pluck, String, Bass, Woodwind, Brass, Vocal)

| MPE Dimension | XPM Embedded? | Destination | Amount |
|---|---|---|---|
| Slide (CC74) | **No** — MPC track-level only | Filter cutoff (user configures manually) | 0.8 (advisory) |
| Pressure (AfterTouch) | Yes | FilterResonance | 0.4 |
| Pitch bend | Yes (PitchBendRange) | ±24 semitones | — |
| Velocity | Yes (amplitude scaling is built-in) | Amplitude + filter cutoff | — |
| ModWheel | Yes | Filter cutoff | 0.5 |

**PitchBendRange:** 24 semitones (±2 octaves). This is the standard MPE melodic pitch bend range. ±48 is available as a Design Mode option in Phase 1C.

**Note on dual AfterTouch destinations:** XPM may support writing multiple `<AfterTouch>` blocks. This requires hardware verification before shipping. Default to single destination (FilterResonance) until verified. See Section 20 (Hardware Verification Checklist).

### Drum Defaults (Kick, Snare, HiHat*, Clap, Tom, Percussion, FX, Loop)

| MPE Dimension | XPM Embedded? | Destination | Amount |
|---|---|---|---|
| Slide (CC74) | **No** — MPC track-level only | Pitch offset ±2 semi (advisory) | — |
| Pressure (AfterTouch) | Yes | ChokeSpeed | 0.4 |
| Pitch bend | Omitted (one-shots) | — | — |
| Velocity | Yes (built-in) | Amplitude + saturation drive | — |

### XPM Expression XML

The following is the format that Outshine writes into the XPM for melodic programs. Only AfterTouch, ModWheel, and PitchBendRange are written — CC74 is NOT in this block.

```xml
<Instrument>
  <PitchBendRange>24</PitchBendRange>
  <AfterTouch>
    <Destination>FilterResonance</Destination>
    <Amount>0.4</Amount>
  </AfterTouch>
  <ModWheel>
    <Destination>FilterCutoff</Destination>
    <Amount>0.5</Amount>
  </ModWheel>
</Instrument>
```

For drum programs, AfterTouch maps to ChokeSpeed. PitchBendRange is omitted (one-shots do not use pitch bend). CC74 is never written into XPM — instruct users to configure it at the MPC track level.

**Important:** All XML attribute names and structure in this block need hardware verification against a physical MPC before shipping. The exact attribute names may differ from the above. See Section 20 (Hardware Verification Checklist).

**AfterTouch and ModWheel `<Amount>` encoding:** `<Amount>` values use integer encoding 0–100 (not float 0.0–1.0). This matches the MPC XPM 1.7 schema. Spec values expressed as floats (e.g., 0.4) must be converted: multiply by 100 and round to nearest integer (e.g., 0.4 → 40).

---

## 8. XPN Output Specification

### Keygroup Programs (Melodic Categories)

| Property | Value |
|---|---|
| `KeyTrack` | `True` — always enabled for melodic |
| `RootNote` | From YIN pitch detection result (NOT hardcoded 60, NOT `RootNote=0` auto-detect — unreliable on MPC for multi-zone keygroups). Write the detected MIDI note number explicitly. |
| `VelStart` | `0` on empty velocity layers — **MPC hard requirement, not optional** |
| Velocity layers | Up to 4 per zone |
| Zone boundaries | Midpoint rule between adjacent root notes. Inclusive lower, exclusive upper. |
| Velocity crossfade | Hard steps only. No crossfade — MPC hardware limitation. See Section 19. |
| Transposition range | Category default: Keys ±12 semi, Vocal ±5 semi, Woodwind/Brass ±5 semi, Lead/Synth ±24 semi |

**RootNote rationale:** CLAUDE.md specifies `RootNote=0` (MPC auto-detect). Outshine overrides this for multi-zone keygroups. YIN-detected root notes are more reliable than MPC auto-detect when zone boundaries depend on accurate root positions. The `RootNote=0` convention in CLAUDE.md applies to simple single-sample programs. For Outshine's multi-zone instruments, explicit root notes prevent zone boundary errors on hardware.

**Critical:** `VelStart=0` on empty layers is a hardware requirement — missing this causes silent layers on MPC.

### Drum Programs

| Property | Value |
|---|---|
| `KeyTrack` | `False` — **required** for drum programs. Writes the correct value; existing pipeline bug defaults to True. |
| `OneShot` | `True` for all drum hits |
| Note mapping | GM standard (see table below) |
| Choke groups | `ChokeSend` on `HiHatClosed` → `ChokeReceive` on `HiHatOpen`. Directional only. |

**GM Note Mapping:**

| Category | MIDI Note | Note Name |
|---|---|---|
| `Kick` | 36 | C2 |
| `Snare` | 38 | D2 |
| `HiHatClosed` | 42 | F#2 |
| `HiHatOpen` | 46 | A#2 |
| `Clap` | 39 | D#2 |
| `Tom` | 41 | F2 |
| `Percussion` | 43 | G2 |
| `FX` | 49 | C#3 |

### WAV Specifications

| Property | Value |
|---|---|
| Sample rate | 44.1 kHz or 48 kHz. 96 kHz sources downsampled to 48 kHz. Sources above 96 kHz rejected. **Never upsample.** |
| Bit depth | **24-bit integer only.** 32-bit float sources converted to 24-bit. 16-bit sources promoted to 24-bit. |
| LUFS | Per-category targets (see Section 6, Stage 5) |
| True-peak ceiling | Per-category (see Section 6, Stage 5 table) |
| Naming convention | `{SampleName}__v{velLayer}__c{rrIndex+1}.wav` (double underscores, 1-based round-robin, FAT32-sanitized) |

Example: `RhodesC3__v2__c1.wav` = Rhodes C3, velocity layer 2, round-robin variant 1 (first variant).

### Tested Hardware Targets

- MPC Live II
- MPC III
- MPC One+
- MPC Control

---

## 9. Pearl Preview Panel

The right half of the main window. Populates after grains are ingested and classified. In the shell state (no grains), this area shows a dimmed placeholder.

### Classification Summary (top)

Displayed as a compact data grid:

| Label | Value example |
|---|---|
| Type | "Keygroup" / "Drum" |
| Zones | "12" |
| Velocity Layers | "4" |
| Round-Robin | "2" |

Font: `GalleryFonts::value()` for values, `GalleryFonts::body()` for labels.

### Zone Map Visualization

A horizontal keyboard-style strip showing:
- White and black keys rendered proportionally (88-key range)
- Zone boundaries marked with vertical lines (`borderGray()`)
- Active zones shaded gold (`xoGold` at 30% opacity)
- Root notes labeled below each zone with note name (e.g., "C3") using `GalleryFonts::value()`
- Minimum height: 48px
- Tooltip on hover: zone range (e.g., "A#2 – D#3"), root note, velocity layer count

### Audio Preview

A "Play C4" button in the Pearl Preview Panel triggers the current zone mapping through JUCE's audio output. Pressing the button sends a MIDI note-on at C4 (MIDI 60) with velocity 100 to the keygroup/drum program, using a preview audio player that renders the sample at the appropriate velocity layer and zone.

- Button label: "Play C4"
- Keyboard shortcut: Space (when preview panel has focus)
- Playback stops automatically after the sample tail or after 4 seconds, whichever is shorter
- Live MPE expression animation: MIDI callback updates the expression bars in real time during preview playback (see MPE Expression Panel below)
- **Playback implementation:** Playback uses a dedicated `juce::AudioTransportSource` fed from the analyzed WAV buffer, routed through a private `juce::AudioDeviceManager` instance isolated from the plugin's audio graph. This prevents preview playback from conflicting with the DAW host's audio processing.

### MPE Expression Panel

Shows 4 expression route rows:

```
[ Slide (CC74) ]     → Filter Cutoff      [████░░░░] 80% (manual config)
[ Pressure    ]     → Filter Resonance   [██████░░] 40%
[ Pitch Bend  ]     → ±24 semitones      [active]
[ Velocity    ]     → Amplitude + Bright [active]
[ ModWheel    ]     → Filter Cutoff      [████░░░░] 50%
```

- Active routes highlighted with gold left border (`xoGold`, 2px)
- Amount displayed as a mini progress bar (fill proportion = amount value)
- "Inactive" routes (e.g., pitch bend on drums) shown dimmed
- **CC74 row shows "(manual config)" annotation** — this route cannot be embedded in XPM. The annotation makes clear to the user that this expression must be configured at the MPC track level.
- During "Play C4" preview playback: expression bars animate in real time from MIDI callbacks (Pressure bar pulses with aftertouch simulation; ModWheel bar responds to mod wheel input if a controller is connected)

### Rebirth Mode Teaser (Phase 1B)

A dimmed section at the bottom of the Pearl Preview Panel:

```
┌─────────────────────────────────────────────────────┐
│  Rebirth Mode (Phase 1B)                     [lock] │
│  Route grains through OPERA · OBRIX · ONSET          │
│  for engine-processed exports.                       │
└─────────────────────────────────────────────────────┘
```

- Entire section rendered at 35% opacity
- Not interactive — clicking shows a toast: "Rebirth Mode arrives in Phase 1B"
- A "Browse Files" reserved area (for spectrum view, Phase 1B) is shown as a placeholder region with dashed border at 20% opacity below the zone map

### Pearl Naming

- Editable text field, single-line
- Auto-generated value: `{SourceFolderName} Pearl` (e.g., "Rhodes Pearl", "808 Kit Pearl")
- If source is a single file: `{FileNameWithoutExtension} Pearl`
- Font: `GalleryFonts::body()`
- Max length: 64 characters
- Validates: no path separators, no leading/trailing whitespace (trimmed automatically)

### Export Format Selector

A segmented control or radio group:

- **XPN Pack** (default, recommended) — full `.xpn` archive with WAVs + XPM + Manifest
- **WAV Folder** — normalized WAVs only, no program file
- **XPM Only** — program XML only, assumes WAVs are already on device

### "Pearl & Export" Button

- Label: "Pearl & Export"
- Background: `xoGold` (#E9C46A)
- Text: `textDark()` (dark label on gold background)
- Full width of the right panel
- Disabled state (dimmed, `xoGold` at 40%) when: no grains added, or any `Unknown` classifications remain unresolved
- On click: launches pipeline stages 1–7 on background thread, populates progress bar

### Progress Bar

- 3px gold bar (`xoGold`) at the absolute bottom of the window, spanning full width
- Animates left to right as pipeline progresses
- Stage percentages (matching `XOutshine::run()` progress reports):

| Stage | Progress |
|---|---|
| INGEST | 0% |
| CLASSIFY | 15% |
| ANALYZE | 25% |
| ENHANCE | 35% |
| NORMALIZE | 60% |
| MAP (buildPrograms) | 75% |
| PACKAGE | 85% |
| VALIDATE | 95–100% |

**Dual progress strings** — two lines displayed above or beside the bar:
- Line 1 (metaphor headline): e.g., "Pearl forming..."
- Line 2 (technical stage): e.g., "Stage 6/8: Building XPM programs"

**Pipeline time estimate:** Before the pipeline runs (when user clicks "Pearl & Export"), display an estimate based on the sample count: "Analyzing {N} samples, ~{T} sec". Compute T from a rough heuristic (e.g., 0.5 sec per sample for analysis + 1 sec per sample for normalize/enhance + 2 sec for packaging). Display this in the progress area immediately on button click, before Stage 1 begins.

- On success: bar stays gold, both strings update — metaphor: "Pearl complete." / technical: "8/8 stages complete."
- On failure: bar turns red (`#E05252`), metaphor line shows error category, technical line shows specific error from `XOutshine`

---

## 10. Shell State (Empty State)

Displayed in the Pearl Preview Panel (right half) when no grains have been added.

### Visual Layout

```
           [oyster shell icon]

      The shell is open.
      Drop a grain to begin.
      WAV files, folders, or XPN archives.
```

| Element | Style |
|---|---|
| Icon | Oyster shell — user-sourced pixel art or SVG (placeholder until provided). Size: 64×64px. |
| Title | "The shell is open." — `GalleryFonts::display()` or `heading()`, 14px, `textDark()` |
| Subtitle | "Drop a grain to begin." — `GalleryFonts::body()`, 13px, `textMid()` |
| Supporting | "WAV files, folders, or XPN archives." — `GalleryFonts::body()`, 11px, #666666 |

All three lines centered horizontally and vertically in the right panel.

### Drag Interaction States

| State | Visual Change |
|---|---|
| Drag enter (file over window) | Window border glow gold (`xoGold`, 2px, 8px blur), icon pulse animation (scale 1.0 → 1.1 → 1.0, 300ms) |
| Drop | Icon transitions to classification spinner; shell state replaces with preview panel |
| Drag leave | Border glow fades (200ms ease-out) |

### Reduced Motion

If `juce::Desktop::getInstance().isScreenReaderEnabled()` or a JUCE accessibility flag for reduced motion is available, skip all animations. Use instant transitions. The pulse and glow animations are purely cosmetic — the functionality must work without them.

---

## 11. Sidebar Mini-Panel (OutshineSidebarPanel)

Lives in **Column C, EXPORT tab** (tab index 5). Must fit within the sidebar width (~320px) and available height.

### Layout

```
┌─────────────────────────────────────────┐
│  OUTSHINE                               │  ← XO Gold header, `GalleryFonts::heading()`
├─────────────────────────────────────────┤
│                                         │
│  [  Drop a grain here  ]                │  ← Mini drop zone, dashed border
│  [   WAV • Folder • XPN ]               │
│                                         │
├─────────────────────────────────────────┤
│  Status: No pearls forged yet           │  ← `textMid()`, 12px
│  (or: Last pearl: Rhodes Pearl          │
│        3 zones, MPE)                    │
├─────────────────────────────────────────┤
│  [     Open the Oyster     ]            │  ← Button launches full window
└─────────────────────────────────────────┘
```

### Behavior

- The mini drop zone accepts the same file types as the main window drop zone (WAV, folders, XPN)
- Dropping a file into the mini drop zone opens the full `OutshineDocumentWindow` AND pre-populates the grain strip with the dropped file AND immediately auto-triggers the CLASSIFY + ANALYZE stages. The user arrives in the full window with the preview panel already populated — they do not need to manually trigger analysis.
- "Open the Oyster" button launches the full `OutshineDocumentWindow` (via `launchOutshine()` — see Section 4). If the window is already open, it is brought to front.
- Status line updates after each successful pearl:
  - Empty: "No pearls forged yet"
  - After export: "Last pearl: {PearlName} ({N} zones, MPE)"
- Pearl name in status is truncated with ellipsis if over 24 characters
- XPN upgrade warning: if the dropped file is an `.xpn`, show a toast immediately on drop: "Original program data will be rebuilt from scratch. Zone map, expression routes, and LUFS normalization will be regenerated."

---

## 12. Component Tree

All Outshine UI components live under `Source/UI/Outshine/`:

```
Source/UI/Outshine/
├── OutshineSidebarPanel.h     ← Column C EXPORT tab content; mini drop zone + launch button
├── OutshineWindow.h           ← Main DocumentWindow (default 900×660, min 700×500)
├── OutshineShellState.h       ← Oyster shell empty state + drag-drop visual handling
├── OutshineInputPanel.h       ← Responsive container: Mode A (split) or Mode B (tabbed)
├── OutshineFolderBrowser.h    ← File system navigation; path bar + file list + selection
├── OutshineGrainStrip.h       ← Horizontal chip strip showing selected grains
├── OutshineAutoMode.h         ← Right panel container: classification + zone map + MPE
├── OutshineZoneMap.h          ← Keyboard zone visualization with root note labels
├── OutshineMPEPanel.h         ← MPE expression route display (4 rows)
└── OutshineExportBar.h        ← Pearl name field + format selector + button + progress bar
```

### Component Responsibilities

| Component | Owns | Communicates With |
|---|---|---|
| `OutshineWindow` | Layout, resize logic, mode switching | All child components |
| `OutshineInputPanel` | Mode A/B switch at 700px breakpoint | `OutshineFolderBrowser`, `OutshineGrainStrip` |
| `OutshineFolderBrowser` | File system state, selection state | `OutshineGrainStrip` (via callback) |
| `OutshineGrainStrip` | Grain list, chip render, removal | `OutshineWindow` (grain count), `OutshineAutoMode` (triggers classify) |
| `OutshineAutoMode` | Classification results, preview state | `OutshineZoneMap`, `OutshineMPEPanel`, `OutshineExportBar` |
| `OutshineExportBar` | Pearl name, format selection, pipeline trigger | `XOutshine` backend (via `OutshineWindow`) |

### Data Flow Between Components

```
OutshineGrainStrip
    → grain list changes
    → OutshineAutoMode::onGrainsChanged(grainPaths)
    → triggers XOutshine::analyzeGrains()  [INGEST + CLASSIFY + ANALYZE]
    → populates OutshineAutoMode state via getAnalyzedSamples()
    → OutshineZoneMap::setZones(programs)
    → OutshineMPEPanel::setExpressionRoutes(routes)
    → OutshineExportBar::setReadyToExport(true)

OutshineExportBar::onExportClicked()
    → reads pearl name + format from UI
    → calls OutshineWindow::runPipeline()
    → XOutshine::exportPearl()  [ENHANCE + NORMALIZE + MAP + PACKAGE + VALIDATE]
    → progress callback → OutshineExportBar::setProgress(float, string)
```

---

## 13. Design Token Usage

All tokens come from `GalleryColors.h` and `GalleryFonts` (part of `GalleryLookAndFeel.h`). Do not hardcode hex values except where noted.

### Colors

| Role | Token | Fallback Hex |
|---|---|---|
| Background | `GalleryColors::shellWhite()` | Theme-aware |
| Header accent | `GalleryColors::xoGold` | #E9C46A |
| Primary text | `GalleryColors::textDark()` | Theme-aware |
| Secondary text | `GalleryColors::textMid()` | Theme-aware |
| Disabled text | Direct hex | #666666 |
| Borders | `GalleryColors::borderGray()` | Theme-aware |
| Gold accent (active) | `GalleryColors::xoGold` | #E9C46A |
| Error state | Direct hex | #E05252 |

### Typography

| Role | Font Token |
|---|---|
| Window title | `GalleryFonts::display()` |
| Section headers | `GalleryFonts::heading()` |
| Body text / labels | `GalleryFonts::body()` |
| Data values (MIDI notes, durations, sample rates) | `GalleryFonts::value()` (monospace) |

### Spacing

| Level | Value |
|---|---|
| Base unit | 4px |
| Outer window padding | 16px |
| Inner component spacing | 8px |
| Chip gap (grain strip) | 4px |

### Border Radius

| Size | Value |
|---|---|
| Small (chips, badges) | 4px |
| Medium (panels, inputs) | 6px |
| Large (main sections) | 8px |

### Accessibility

- Call `A11y::setup(component, accessibleName, description)` on every interactive component
- Call `drawFocusRing(g, bounds)` in `paint()` when `hasKeyboardFocus(false)`
- Minimum touch/click target: 32×32px
- Color contrast: all text on background must pass WCAG AA (4.5:1 for normal text, 3:1 for large text)

---

## 14. Keyboard Navigation

All interactive elements must be reachable via Tab key. Tab order is linear, top-to-bottom, left-to-right.

### Tab Order

| Order | Element | Key behavior |
|---|---|---|
| 1 | Drop Zone | Enter = opens OS file picker; Space = same |
| 2 | Folder browser path bar | Enter = navigate to typed path |
| 3 | Folder browser file list | Arrow keys = navigate list; Space = toggle selection; Enter = navigate into folder |
| 4 | "Select All WAVs" button | Enter/Space = activate |
| 5 | "Add Selected →" button | Enter/Space = activate |
| 6 | Grain strip chips | Arrow keys = move between chips; Delete/Backspace = remove focused chip |
| 7 | Classification summary | Read-only; skipped if no grains present |
| 8 | Pearl name field | Standard text input behavior |
| 9 | Export format selector | Arrow keys = change selection |
| 10 | "Pearl & Export" button | Enter/Space = activate; disabled when no grains or unresolved Unknown classifications |

### Global Shortcuts

| Key | Action |
|---|---|
| Escape | Closes the DocumentWindow |
| Cmd+A / Ctrl+A | Select all WAVs in folder browser (when browser has focus) |
| Space | Play C4 preview (when Pearl Preview Panel has focus) |

---

## 15. State Persistence

State is persisted to JSON files in the user's application data directory. Do not use `Tools/social/output/` — use the app-appropriate path resolved via JUCE's `juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)`.

### Persisted State

| Key | Value | File |
|---|---|---|
| `recentGrains` | Array of last 20 file paths (strings), most recent first | `.outshine-state.json` |
| `lastExportFormat` | `"xpn"` / `"wav"` / `"xpm"` | `.outshine-state.json` |
| `lastBrowsePath` | Absolute directory path string | `.outshine-state.json` |
| `windowWidth` | Integer px | `.outshine-state.json` |
| `windowHeight` | Integer px | `.outshine-state.json` |

### File Location

```cpp
juce::File stateDir = juce::File::getSpecialLocation(
    juce::File::userApplicationDataDirectory
).getChildFile("XO_OX").getChildFile("Outshine");

juce::File stateFile = stateDir.getChildFile(".outshine-state.json");
```

### Recent Grains Behavior

- Maximum 20 entries; oldest entry is dropped when the 21st is added
- De-duplication: if a path is already in the list, it moves to position 0 (most recent) instead of duplicating
- Files that no longer exist on disk are retained in the list but displayed dimmed in the Recent Grains tab
- State file is written synchronously on the main thread after each successful grain add or pearl export (list is small; no async needed)

---

## 16. Error Handling

All errors surface as toast notifications using the FLOW V.4.0 notification pattern. Toasts use colored left borders:

| Border color | Meaning |
|---|---|
| `xoGold` (#E9C46A) | Info |
| Green (#4CAF50) | Success |
| Red (#E05252) | Error / failure |

### Error Cases

| Condition | Behavior |
|---|---|
| Sample classified as `Unknown` | Gold highlight on classification row; manual override dropdown (all `SampleCategory` values) shown with `Unknown` pre-selected; "Pearl & Export" button disabled until resolved |
| Corrupt or unreadable WAV | Skip file; toast: "Skipped: {filename} (unreadable)" (gold border, info) |
| XPN archive contains no WAVs | Toast: "This XPN contains no samples" (gold border, info) |
| Export path not writable | Toast: "Export failed: cannot write to {path}" (red border, error); progress bar turns red |
| Pipeline exception | Toast with specific error message from `XOutshine` (red border, error); progress bar turns red; window remains open |
| File no longer exists (from recent grains) | Skip silently when running pipeline; if user explicitly adds from recent list, show toast: "File not found: {filename}" |
| YIN pitch detection failure | Fall back to `C4` (MIDI 60) as root note; log warning; do not block export. See Section 19 for known failure modes. |

### Cancel Behavior

Pressing Escape or clicking a Cancel button during pipeline execution sets `OutshineProgress::cancelled = true`. The backend checks this flag between stages and aborts cleanly. The temp working directory is cleaned up. The UI reverts to the pre-pipeline state with grains still loaded. A toast shows "Pearl cancelled — grains preserved."

### Manual Override for Unknown Classifications

When a grain cannot be classified:

1. Classification summary row for that grain shows a dropdown instead of a static label, with a gold left border (`xoGold`, 2px) and gold background at 12% opacity
2. Dropdown is pre-selected to `Unknown`
3. The preview panel **auto-scrolls** to the first unresolved row immediately after analysis completes
4. A warning badge appears on the "Pearl & Export" button showing the count: "2 unresolved"
5. Once all `Unknown` items are resolved (user selects any non-Unknown category), the button becomes active
6. The user-selected category is used for all downstream decisions (zone mapping, MPE defaults, LUFS target, KeyTrack)

---

## 17. Phase Boundaries

### Phase 1A (This Spec) — The Core Loop

Delivers a functional daily-driver:

- Responsive input panel (drop zone + folder browser + grain strip)
- Full 8-stage pipeline: ingest → classify → analyze → enhance → normalize → map → package → validate
- MPE expression pre-wiring (category-aware defaults, all 4 dimensions)
- Zone map visualization
- XPN + XPM + WAV output
- Pearl naming + export format selection
- Shell state (empty state) with oyster metaphor
- Sidebar mini-panel with mini drop zone
- Keyboard navigation, accessibility, error handling
- State persistence (recent grains, window size, last format, last path)

### Phase 1B — Rebirth Mode

Adds engine-processed exports. User can route grains through 5 engine profiles before packaging:

- OVERWASH — pad diffusion and time-blurring
- ONSET — drum synthesis processing
- OWARE — tuned percussion resonance
- OPERA — additive vocal processing
- OBRIX — flagship character processing

Also adds:
- XPN upgrade pipeline (import existing XPN → re-analyze → re-map → re-export)
- Velocity-as-spectral-shaping (velocity layers receive different spectral processing, not just amplitude scaling)

### Phase 1C — Design Mode

Adds manual control over every automated decision:

- Manual zone editing tabs (drag zone boundaries on the keyboard strip)
- Full expression routing UI (drag-and-drop MPE assignments, amount knobs)
- Contextual preset modes: Natural / Intimate / Cinematic / Ghost
- Composite multi-source keygroups (combine samples from different sources into one instrument)
- Formant detection via LPC (Linear Predictive Coding) for more accurate transposition bounds

---

## 18. Backend Wiring

The UI drives the existing `XOutshine` class. All core pipeline DSP is already implemented. However, the current API is a single monolithic `run()` call, which must be extended with public stage methods for Phase 1A.

### Actual Public API (Current XOutshine.h)

The existing class exposes only:

```cpp
class XOutshine {
public:
    // Run the full upgrade pipeline.
    // inputPath: .xpn file or folder of WAVs
    // outputPath: .xpn file or folder
    // Returns true on success.
    bool run(const juce::File& inputPath,
             const juce::File& outputPath,
             const OutshineSettings& settings,
             ProgressCallback progress = nullptr);

    // Access results after run()
    int getNumPrograms() const;
    const juce::StringArray& getErrors() const;
};
```

All pipeline stages (ingest, classify, analyze, enhance, normalize, buildPrograms, packageXPN/packageFolder, validate) are currently **private** methods.

### UI Integration: Option B is Required

**Option A — Monolithic (insufficient):** Call `run()` as a single blocking operation on a background thread. This approach cannot support the Pearl Preview Panel because intermediate state (classified samples, zone map, expression routes) is never exposed. Option A is preserved for CLI/tool use only.

**Option B — Staged Public Methods (REQUIRED):** Add public stage methods to `XOutshine.h`. The UI runs INGEST → CLASSIFY → ANALYZE immediately when grains are added, populates the Pearl Preview Panel (Section 9) with live intermediate state, and defers ENHANCE → NORMALIZE → MAP → PACKAGE → VALIDATE to the "Pearl & Export" button click.

Option B is not optional. The Pearl Preview Panel (Section 9) requires intermediate state: classified sample list, zone map, and expression routes must be visible and interactive before export. This is a P0 backend requirement (Phase 1A-alpha item #9).

### Backend Modifications Required

The following public methods must be added to `XOutshine.h` as part of Phase 1A implementation:

```cpp
// --- New public stage methods ---

// Stage 1-3: Run analysis pass (ingest + classify + analyze).
// Call this when grains are added to the strip to populate preview panel.
// filePaths: flat list of WAV file paths (already extracted from folders/XPNs by UI)
bool analyzeGrains(const juce::StringArray& filePaths,
                   const OutshineSettings& settings,
                   ProgressCallback progress = nullptr);

// Access analysis results for preview panel (populated after analyzeGrains())
const std::vector<AnalyzedSample>& getAnalyzedSamples() const;

// Stage 4-8: Run the export pass (enhance + normalize + map + package + validate).
// Call this when user clicks "Pearl & Export".
// outputPath: .xpn file or folder
bool exportPearl(const juce::File& outputPath,
                 const OutshineSettings& settings,
                 ProgressCallback progress = nullptr);
```

The existing `run()` method is preserved for CLI/tool use and runs all 8 stages in one call. The new `analyzeGrains()` + `exportPearl()` split matches the UI's two-phase interaction model.

**State contract:** `XOutshine` retains the last `analyzeGrains()` result as internal state. `exportPearl()` operates on that retained state without re-scanning the file list. Calling `analyzeGrains()` again replaces the retained state entirely.

**Concurrent analysis cancellation:** If `analyzeGrains()` is already running when new grains are added, cancel the current run via `std::atomic<bool> cancelFlag` and restart with the full updated grain list. The cancellation flag is checked between pipeline stages. Partial results from the cancelled run are discarded.

**workDir lifecycle:** `analyzeGrains()` creates a workDir in the system temp directory and retains it as internal state. `exportPearl()` reuses the same workDir. The workDir is cleaned up when `exportPearl()` completes, when `analyzeGrains()` is called again (replacing the previous workDir), or when the `XOutshine` instance is destroyed (RAII cleanup via destructor).

### Progress Callback

The `ProgressCallback` type is already defined in `XOutshine.h`:

```cpp
struct OutshineProgress {
    int          currentSample = 0;
    int          totalSamples  = 0;
    juce::String stage;
    float        overallProgress = 0.0f;  // 0-1
    bool         cancelled = false;       // set true to abort pipeline
};

using ProgressCallback = std::function<void(OutshineProgress&)>;
```

The UI must post callbacks to the message thread before touching UI components:

```cpp
auto callback = [safeThis = juce::Component::SafePointer<OutshineWindow>(this)]
    (OutshineProgress& p)
{
    float prog = p.overallProgress;
    juce::String stg = p.stage;
    juce::MessageManager::callAsync([safeThis, prog, stg]() {
        if (auto* w = safeThis.getComponent())
            w->getExportBar().setProgress(prog, stg);
    });
};
```

Note: SafePointer prevents use-after-free if window closes during pipeline. Values copied on background thread before posting.

The "Pearl & Export" button disables itself on click and re-enables on pipeline completion (success or failure).

### Cancel Behavior

Pressing Escape or clicking a Cancel button during pipeline execution sets `OutshineProgress::cancelled = true`. The backend checks this flag between stages and aborts cleanly. The temp working directory is cleaned up. The UI reverts to the pre-pipeline state with grains still loaded. A toast shows "Pearl cancelled — grains preserved."

### Python CLI vs. C++ Canonical

`Tools/xoutshine.py` is the prototype implementation used for rapid iteration. `Source/Export/XOutshine.h` is canonical. When behavior diverges between them, the C++ version is authoritative. If the Python CLI has a behavior not yet in C++, file a task to port it — do not match the Python behavior in the UI layer.

### 17.1 Type Definitions

The following types are referenced in this spec. Their status against the current `XOutshine.h`:

| Type | Status | Location |
|---|---|---|
| `SampleCategory` (enum) | **EXISTS but INCOMPLETE** — add Loop, Woodwind, Brass, Vocal (P1 fix #10) | Categories: Kick, Snare, HiHatClosed, HiHatOpen, Clap, Tom, Percussion, FX, **Loop**, Bass, Pad, Lead, Keys, Pluck, String, **Woodwind**, **Brass**, **Vocal**, Unknown |
| `AnalyzedSample` (struct) | **EXISTS** in `XOutshine.h` | sourceFile, name, category, sampleRate, bitDepth, numChannels, numSamples, durationS, rmsDb, peakDb, dcOffset, tailLengthS, isLoopable, loopStart, loopEnd, detectedMidiNote, pitchConfidence |
| `EnhancedLayer` (struct) | **EXISTS** in `XOutshine.h` | file, filename, velLayer, rrIndex |
| `UpgradedProgram` (struct) | **EXISTS** in `XOutshine.h` — use this as the zone map type | name, category, sourceInfo, layers, numVelocityLayers, numRoundRobin |
| `OutshineSettings` (struct) | **EXISTS** in `XOutshine.h` | velocityLayers, roundRobin, velocityCurve, lufsTarget, applyFadeGuards, removeDC, applyDither, detectLoops, packName |
| `OutshineProgress` (struct) | **EXISTS** in `XOutshine.h` | currentSample, totalSamples, stage, overallProgress, cancelled |
| `ProgressCallback` | **EXISTS** in `XOutshine.h` | `std::function<void(OutshineProgress&)>` |
| `ExportFormat` (enum) | **DOES NOT EXIST** — must be added | XPNPack, WAVFolder, XPMOnly |
| `ZoneMap` | **DOES NOT EXIST** as a named type — use `std::vector<UpgradedProgram>` | Zone boundaries are implicit in the root note + layer structure of `UpgradedProgram` |
| `MPEExpressionRoutes` | **DOES NOT EXIST** — must be added or represented as a struct in the UI layer | Slide destination/amount, Aftertouch destination/amount, PitchBendRange, Velocity behavior |

**Action items for Phase 1A-alpha implementation:**
- Add `ExportFormat` enum to `XOutshine.h` (or a shared export types header)
- Add `MPEExpressionRoutes` struct to `XOutshine.h` or UI-side header
- Add `analyzeGrains()` and `exportPearl()` public methods (P0 — required)
- Add `getAnalyzedSamples()` accessor
- Add `Loop`, `Woodwind`, `Brass`, `Vocal` to `SampleCategory` enum
- Add `classifyByName()` patterns for new categories
- Replace autocorrelation pitch detection with YIN in `classifyByAudio()`
- Fix `KeyTrack` generation for drum programs (currently always writes True)
- Fix choke group generation to use `ChokeSend`/`ChokeReceive`
- Add channel validation in INGEST stage
- Add sample rate guard in INGEST stage (reject >96 kHz, flag 96 kHz for downsampling)
- Add 96 kHz → 48 kHz downsampling in NORMALIZE stage
- Force 24-bit output, add 32-bit float conversion in NORMALIZE stage
- Replace bulk ZIP assembly with streaming ZIP output
- Add FAT32 filename sanitization in PACKAGE stage
- Add XML escaping for ProgramName and all user-provided strings
- Fix round-robin pitch variation to ±3 cents (was ±5)
- Fix micro-gain variation to ±0.5 dB (was ±0.25)
- Fix velocity filter to smooth taper (was hard cutoff at t=0.7)
- Resolve `XOriginate.h` include dependency

### 18.2 OutshineSettings Struct Definition

The `OutshineSettings` struct must be expanded to replace the single `lufsTarget` and `truePeakCeiling` fields with per-category values (P1 fixes #11–#12). Target definition:

```cpp
struct OutshineSettings {
    int velocityLayers = 4;
    int roundRobin = 4;
    bool normalize = true;
    // Per-category LUFS targets (ITU-R BS.1770 approximate)
    float lufsTargetDrum = -14.0f;
    float lufsTargetBass = -16.0f;
    float lufsTargetKeysLead = -14.0f;
    float lufsTargetPadString = -18.0f;
    float lufsTargetVocal = -16.0f;
    float lufsTargetWoodwindBrass = -16.0f;
    // Per-category true-peak ceilings
    float truePeakDrumKeys = -0.5f;     // preserve transients
    float truePeakLeadPluck = -0.4f;
    float truePeakPadStringBass = -0.3f; // tighter for sustained content
};
```

---

## 19. Known Limitations (Phase 1A)

These limitations are intentional, hardware-constrained, or deferred. They must be communicated clearly to users — either in the UI or in release notes.

| # | Limitation | Notes |
|---|-----------|-------|
| 1 | CC74 (Slide) not embeddable in XPM | Slide expression requires manual MIDI routing configuration at the MPC track level. Outshine cannot write CC74 into the XPM program file. The CC74 row in the MPE Expression Panel is advisory only. |
| 2 | Velocity crossfade is hard-stepped | MPC hardware does not support velocity crossfade at the XPM level. Velocity layers trigger at hard boundaries. There is no workaround within the XPM format. |
| 3 | No session save/load | Outshine opens with a fresh state each time. Grain selection, classification overrides, and naming are not persisted across sessions. Session save/load is deferred to Phase 1C. |
| 4 | YIN pitch detection failure modes | YIN has known failure cases on highly inharmonic content (extreme metallic percussion, some granular textures). On failure, Outshine falls back to C4 (MIDI 60) and logs a warning. Users should verify root notes on complex timbres. |
| 5 | LUFS measurement is approximate | The LUFS measurement is a simplified integrated loudness approximation, not ITU-R BS.1770-4 K-weighted. Phase 1B will add proper K-weighting. Current targets are close enough for practical use on MPC. |
| 6 | Orchestral articulation switching not supported | Keyswitch-based articulation sets (e.g., separate notes for legato/staccato) are not supported in Phase 1A. Each program represents a single articulation. Multi-articulation programs are a Phase 1C feature. |
| 7 | Piano view only for zone map | The zone map uses a piano keyboard visualization. The spectrum view (showing frequency analysis of zones) is reserved for Phase 1B. The reserved area is shown dimmed in the Pearl Preview Panel. |
| 8 | Dual AfterTouch destinations unverified | XPM format may support multiple `<AfterTouch>` blocks. Phase 1A defaults to single destination (FilterResonance) until hardware verification confirms dual-destination support. See Section 20. |
| 9 | Mixed sample rate kits produce a warning | If a kit contains both 44.1 kHz and 48 kHz sources, Outshine outputs them at their native rates and logs a warning at VALIDATE. MPC handles mixed-rate kits, but the behavior should be verified on hardware (see Section 20). |
| 10 | Filename separator fallback | If double-underscore (`__`) filename separator fails MPC hardware verification, fall back to single underscore with tag prefix: `{SampleName}_v{velLayer}_c{rrIndex+1}.wav`. |

---

## 20. Hardware Verification Checklist

Before shipping Phase 1A to users, verify all items on physical MPC hardware (MPC Live II, MPC III, and MPC One+ are the primary targets). Testing in MPC Software (Mac) alone is insufficient — software and hardware can diverge on XPM parsing.

### Required Hardware Tests

- [ ] Keygroup program with 3 or more zones loads correctly on MPC Live II
- [ ] Root notes match expected pitch per zone (play each zone boundary note and verify no tuning error)
- [ ] Velocity layers trigger at correct velocity ranges (tap at various velocities, verify layer switching)
- [ ] `KeyTrack=True` on melodic programs: samples transpose correctly across zone boundaries
- [ ] `KeyTrack=False` on drum programs: samples do not detune when triggered at non-root notes
- [ ] `PitchBendRange=24` is respected on hardware (apply pitch bend, verify ±2 octave range)
- [ ] AfterTouch routing fires: single destination (FilterResonance) responds to channel aftertouch
- [ ] Dual AfterTouch destinations: test if a second `<AfterTouch>` block is respected or silently ignored
- [ ] Choke groups: `HiHatClosed` chokes `HiHatOpen` (trigger open hat, then trigger closed hat — open hat must stop)
- [ ] Reverse choke does not occur: `HiHatOpen` does not choke `HiHatClosed`
- [ ] 44.1 kHz WAVs load and play correctly
- [ ] 48 kHz WAVs load and play correctly
- [ ] 96 kHz → 48 kHz downsampled WAVs play correctly (no pitch shift, no artifacts)
- [ ] WAV filenames with double underscores (`__`) parse correctly (not misinterpreted as separators by MPC)
- [ ] WAV filenames with FAT32-sanitized characters (underscores replacing `?`, `<`, etc.) load correctly
- [ ] XPN archive loads in MPC Software (Mac) without errors
- [ ] XPN archive loads in MPC hardware expansion browser with correct metadata and program name
- [ ] XPM program name with special characters (apostrophes, ampersands) displays correctly after XML escaping
- [ ] Mixed-rate kit (44.1 + 48 kHz samples): verify each sample plays at its correct pitch on hardware, not just that the kit loads without error. If pitch accuracy fails, escalate mixed-rate kits from a warning to an error in the VALIDATE stage.
- [ ] Drum program with 8 samples at GM positions all trigger on correct pads
- [ ] `VelStart=0` on empty velocity layers: no ghost triggering on any layer

### Verification Sign-off

Each checkbox must be tested and signed off by a human on physical hardware before Phase 1A ships. Log the hardware model, firmware version, and MPC Software version for each test.

---

## Appendix A: File Naming Examples

| Source | Generated Pearl Name | Example WAV Name |
|---|---|---|
| Folder: `Rhodes MK2 Samples/` | "Rhodes MK2 Samples Pearl" | `RhodesC3__v1__c1.wav` |
| Single file: `808_Kick_Hard.wav` | "808 Kick Hard Pearl" | `808KickHard__v1__c1.wav` |
| XPN: `VintageKeys.xpn` | "VintageKeys Pearl" | `VintageKeysA3__v2__c1.wav` |

Naming rules:
- Underscores in source names converted to spaces for pearl name display
- Velocity layer index is 1-based (`v1`, `v2`, `v3`, `v4`)
- Round-robin index is **1-based** (`c1`, `c2`, `c3`, `c4`) — matches backend (`rr + 1` in filename generation)
- Separator is **double underscore** (`__`) — matches backend: `s.name + "__v" + vel + "__c" + (rr+1)`
- WAV filename has spaces removed (no path separator safety issues)

---

## Appendix B: MPC Compatibility Checklist

Before any pearl is exported, the pipeline validates:

- [ ] `KeyTrack=True` present on all keygroup layers (melodic)
- [ ] `KeyTrack=False` present on all drum program layers
- [ ] `RootNote` is a value between 0 and 127 (never absent, never "auto")
- [ ] `VelStart=0` on all empty velocity layers
- [ ] All WAV files referenced in XPM exist in the archive
- [ ] No WAV file exceeds 2GB (MPC limit)
- [ ] XPM program name matches archive folder name
- [ ] All sample paths in XPM use relative paths (not absolute)
- [ ] Drum programs have `OneShot=True` on all layers
- [ ] Choke groups use `ChokeSend`/`ChokeReceive` (directional, not symmetrical `MuteGroup`)
- [ ] All output WAVs are 24-bit integer (no 32-bit float in archive)
- [ ] All output WAVs are 44.1 kHz or 48 kHz (no 96 kHz or above in archive)
- [ ] All output filenames are FAT32-safe (no `? < > : " / \ | *` characters)
- [ ] ProgramName and all user-provided strings are XML-escaped in XPM

If validation fails, export is blocked and a specific error toast is shown listing the failed checks.

---

*Revision 2 — Updated 2026-03-25. Incorporates findings from DA review, Kai's 7-android audit, and Synth Seance ghost council. Phase 1A is split into alpha (backend hardening, 20 fixes) and beta (UI build). CC74 limitation documented honestly. Window architecture changed from CallOutBox to DocumentWindow. YIN pitch detection replaces autocorrelation. All known limitations and hardware verification requirements are explicit. A developer should be able to build Phase 1A-alpha from Section 2 and Phase 1A-beta from the remaining sections, referencing `XOutshine.h` for backend API details.*
