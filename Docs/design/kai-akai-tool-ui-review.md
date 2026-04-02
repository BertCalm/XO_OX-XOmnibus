# Kai Android Team — AKAI/XPN Tool UI Review
**Date:** 2026-03-23
**Subject:** Outshine, Originate, Oxport — UI Compatibility with XOceanus's Rebirth Aesthetic
**Mandate:** "The XPN suite Akai wishes they could have created."
**Androids deployed:** Rex, Vibe, Scout, Atlas, Sage

---

## Kai Opening Statement

We read three artifacts before writing a single opinion:
1. `Docs/mockups/outshine-prototype.html` — the living prototype
2. `DESIGN_SPECIFICATION_OUTSHINE_ORIGINATE.md` — the UIX Studio spec (Ulf/Issea/Xavier)
3. `Docs/xoceanus_master_specification.md` / `Docs/design/xoceanus_ui_master_spec_v2.md` — XOceanus's Rebirth design language

The UIX Studio did excellent foundational work. This review is not a rejection of that work. It is a calibration pass: aligning Outshine/Originate/Oxport to the full weight of XOceanus's Rebirth aesthetic, MPC format realities, and the MPCe 3D pad competitive window. Several findings are [CRITICAL] with production consequences; most are [IMPROVEMENT] or [STYLE].

---

## Android Reports

---

### [REX] — XPN Format & Golden Rules

**Domain:** Does the UI correctly represent XPN/XPM format constraints? Are the Golden Rules enforced?

#### R01 [CRITICAL] — KeyTrack=True Is Not Visible in the Zone Map

The Design Mode wireframe shows `Key Track: [●] On` as a single toggle buried in the Selected Zone inspector. This is wrong at the product level: **KeyTrack=True is not a user choice — it is the XPN Golden Rule.** The toggle should not exist. It should be a fixed read-only indicator (a small lock icon with "MPC Required" tooltip). If a user sets KeyTrack=False, the resulting XPN will produce samples that do not transpose across zones — the single most common cause of broken MPC keygroups in the field.

**Fix:** Remove the KeyTrack toggle. Replace with a non-interactive badge: `KEY TRACK  [LOCK] On — required by MPC`. Add a help popover explaining why. For expert users who genuinely need KeyTrack=False (e.g., one-shot drum kits mapped as single notes), expose it only under an "Advanced / Override Golden Rules" section with a prominent warning.

#### R02 [CRITICAL] — VelStart=0 Has No UI Representation

The XPM Golden Rule "empty layer VelStart=0 prevents ghost triggering" has no corresponding UI concept in any wireframe. Velocity layer assignment currently shows layer counts (1, 3, 6, 12) but has no visibility into the VelStart values. A user building a 3-layer kit can unknowingly produce a VelStart gap (e.g., 0/43/86 vs. the correct 0/43/85) that causes silent notes on MPC.

**Fix:** In the Velocity tab of Design Mode, show each layer's start/end values as a color-coded ladder. Auto-compute contiguous splits from the layer count. Display in JetBrains Mono. Add a "Validate Ranges" button that runs before Export and catches any gap.

#### R03 [IMPROVEMENT] — RootNote=0 Convention Is Undocumented

The export complete state (A5) shows metadata fields (Name, Description, Author, Tags) but no RootNote field. The XPM Golden Rule is RootNote=0 (MPC auto-detect convention). Auto-classifying the root note during sample analysis is correct (the Auto Mode panel shows `Root: C2 (detected)`), but the final export panel should confirm the XPM will use RootNote=0 regardless of the auto-detected root. The detected root is used for zone placement only; the XPM field is always 0.

**Fix:** In the Export panel metadata block, add a read-only line: `XPM Root:  0  (MPC convention — overrides detected root)` with an info icon.

#### R04 [IMPROVEMENT] — ZonePlay Mutual Exclusion Missing

The XPN spec makes `ZonePlay` velocity switching (1) and round-robin cycle (0) mutually exclusive per keygroup. The Velocity tab shows velocity layers, and the Auto Mapping Preview references "Round-robin: Off" — but there is no enforcement or visual warning when a user attempts to enable both simultaneously. On MPC, the result is silent: MPC ignores the conflicting attribute silently.

**Fix:** In the Velocity tab, when `Round-Robin` is toggled on, gray out the Velocity Layer count with a label: "Velocity layers disabled — round-robin active (MPC constraint)." And vice versa.

#### R05 [STYLE] — "WAV files, folders, XPN packs" Drop Zone Text

The empty-state drop zone advertises "XPN packs" as an input source. This is correct behavior for re-editing existing instruments, but "XPN pack" is Akai's brand term. For XOceanus's positioning as "the XPN suite Akai wishes they could have built," we should own our vocabulary. Prefer: `WAV files, folders, or .xpn archives`.

---

### [VIBE] — Sound Design & Creative Inspiration

**Domain:** Does the UI inspire creative sound design? Is the forge metaphor alive?

#### V01 [CRITICAL] — The Forge Metaphor Stops at the Header

The current prototype is technically clean but emotionally quiet after the empty state. The ember-pulse animation on the forge icon is the one moment of life. Once samples are loaded, the UI becomes a form-filling exercise. The Outshine Seance gave this a 7.0/10 on Expressiveness — that score lives in the UI as much as the DSP.

**The forge metaphor should be ambient throughout:**
- The progress bar during analysis should pulse gold like coals breathing, not fill linearly.
- The Auto Classification panel should animate each classification line as it resolves — a brief spark-flash per detected property (Type, Root, Transient, Length).
- The Rendering state's "FORGING YOUR INSTRUMENT" text (already specified in the UIX doc — Issea's ritual moment) should be accompanied by a subtle particle field rising from the bottom of the panel, using the gold/amber palette. Not dramatic — ember drift, not fireworks.
- The Export Complete state should feel like the instrument emerging from the fire: a brief warm-glow entrance animation on the cover art preview.

**None of this requires new infrastructure** — CSS keyframe animations in the prototype, JUCE component repaint timers at 30fps in production. The UIX Studio spec lists `dur-normal: 200ms` and `dur-enter: 250ms cubic-bezier(0.34,1.56,0.64,1)`. Use them more aggressively in the success states.

#### V02 [IMPROVEMENT] — Sonic DNA Card Is Missing from Auto Classification

The Auto Mode panel (A2) shows classification properties (Type, Root, Transient, Length, Formant, Vel layers) but does not surface the sample's Sonic DNA fingerprint. Given that Sonic DNA (B039 — RATIFIED 8-0) is the crown jewel of the Outshine architecture, it should have presence in the UI. Even a minimal 6-axis mini-radar (32x32px) per sample in the source list would make DNA tangible and show users that Outshine is doing something intelligent beyond threshold detection.

**Fix:** Add a compact DNA mini-radar to each sample row in the source panel. 6 axes: brightness, warmth, movement, density, space, aggression. Gold fill. 32x32px. Tooltip on hover shows axis labels and values.

#### V03 [IMPROVEMENT] — Rebirth Mode Has No UI Presence

Seance Blessing B040 — Rebirth Mode (engines as transfer functions) is "genuine innovation." But Rebirth Mode is completely absent from all wireframes. This is the most differentiated capability: using XOceanus's 71-engine fleet as processing/synthesis tools to reshape samples. It should not be buried in settings.

**Fix:** Add a "Rebirth" toggle button in the header bar next to `Auto Mode / Design Mode`. When enabled, the Auto Classification panel gains a row: `Rebirth: OSPREY (Coastal Processing)` with a small engine accent color swatch. The Export button changes label to `Export Reborn XPN Pack`. This single UI addition communicates the product's uniqueness to every user who opens Outshine.

#### V04 [STYLE] — Velocity Layer Palette Is Not Using the Zone Color System

The Velocity tab (implied by the wireframe) presumably uses generic color for velocity layer visualization. The existing 12-color zone palette from the spec (`Reef Jade`, `Crate Yellow`, `Axolotl Pink`, etc.) should be applied to velocity layers, using the same hue-sequence per zone. This makes the instrument feel like it belongs to the XOceanus family rather than being a standalone utility.

#### V05 [STYLE] — "Contextual Preset Generation" (VIS-007) Deserves a UI Hook

The Seance recommended auto-generating Natural/Intimate/Cinematic/Ghost instrument variations from each forged instrument. The Export complete panel (A5) has space to show 4 variation thumbnails with names. Even as a placeholder, the UI should promise this behavior: 4 small cover art previews below the main one, labeled with variation names. Users will understand immediately that they are getting 4 instruments for the work of 1.

---

### [SCOUT] — MPCe + Community Intelligence

**Domain:** Quad-corner UI clarity, MPC forum readability, competitive positioning.

#### SC01 [CRITICAL] — No MPCe 3D Pad Quad-Corner Assignment UI Exists

This is the single largest gap relative to the competitive window. As of March 2026, zero competitors are designing packs specifically for MPCe quad-corner + XY morphing. Outshine's UI has no concept of MPCe pad assignment. The existing Zone Map is a linear keyboard strip — correct for keygroups, completely absent for drum program + MPCe mapping.

**The missing panel:** A dedicated "MPCe Layout" tab (5th tab in Design Mode, appearing only when export type is set to Drum Program) showing a 4x4 grid of pads visually identical to the MPC hardware layout. Each pad has four corners. Users drag samples from the source panel to corners. The XY axis per pad is shown as a small gradient square in the pad center. Color coding follows the zone palette.

This panel should be the **first screenshot in any marketing material for Outshine**. It is the feature that no competing tool has. Building it into the UI from day one — even as a locked/coming-soon state — establishes positioning.

**Implementation priority:** High. The competitive window is 6-12 months from March 2026. Month 5.

#### SC02 [IMPROVEMENT] — MPC Forum Users Will Not Know What "Rebirth Mode" Means

The MPC community forum vocabulary is concrete: "drum program," "keygroup program," "velocity layer," "round robin," "chromatic." Abstract terms like "Rebirth Mode" or "Sonic DNA" will confuse forum users who are comparing tools.

**Fix:** Add a subtitle to every abstract concept. `Rebirth: OSPREY` → `Rebirth: OSPREY (processes samples through the Osprey synth engine)`. `Sonic DNA` → `Sonic DNA (tonal fingerprint — used to match samples and drive intelligent mapping)`. MPC forum users are sophisticated about their hardware; they will respect the depth once they understand the vocabulary.

#### SC03 [IMPROVEMENT] — The Originate Preset List Needs MPC-Readiness Indicators

The Originate preset selection view (A6) shows engine name and mood but no MPC-readiness signal. Forum users want to know: "Will this preset sound good on my MPC?" Key indicators:
- Sound Shape (Transient / Sustained / Evolving / Bass / Texture / Rhythmic) — directly affects how many velocity layers are needed
- Estimated WAV count per preset (so users can plan pack size)
- A green/yellow/red MPC-readiness dot based on DNA parameters (evolving pads with long tails are yellow; one-shot transients are green)

**Fix:** Add 3 small columns to the preset list: `Shape` (2-letter code: TR/SU/EV/BA/TE/RH), `WAVs` (estimated count), `MPC` (green/yellow/red dot). These are computable from `.xometa` data — no new infrastructure required.

#### SC04 [STYLE] — Export Success Language Should Match MPC Hardware Vocabulary

The Export Complete state says "INSTRUMENT READY." MPC users call these "programs." Consider: "PROGRAM READY" or "KEYGROUP PROGRAM READY" or "DRUM PROGRAM READY" — dynamically reflecting the export type chosen. This one-word change makes the tool feel native to the MPC ecosystem rather than like a DAW plugin with an export function bolted on.

---

### [ATLAS] — XOceanus Design Language Integration

**Domain:** Does everything match XOceanus's Rebirth aesthetic (dark mode, XO Gold, depth-zone gradients, APERTURE/Space Grotesk typography)? Does it feel like a sibling?

#### A01 [CRITICAL] — The Current Prototype Is Dark Mode Only; XOceanus Is Light Mode Default

The prototype (`outshine-prototype.html`) is built entirely in dark mode: `--xo-bg-base: #1A1A1A`, `--xo-bg-panel: #242424`. XOceanus's Rebirth design language specifies **light mode as the default** with dark mode as a toggle. The Gallery Model is a "warm white shell `#F8F6F3` framing engine accent colors."

This is not a preference conflict — it is a product decision. When Outshine launches from XOceanus's menu bar, it should inherit XOceanus's current mode (light or dark). Opening a dark Outshine from a light XOceanus window creates a jarring ownership break.

**Fix:**
- Implement both light and dark themes using the design token system from the UIX Studio spec (the spec correctly defined both — the prototype only implemented dark).
- Light mode tokens: `bg-window: #F8F6F3`, `bg-panel: #EFEDE9`, `bg-card: #FFFFFF`.
- The XO Gold header (`#E9C46A`) is a brand constant — identical in both modes.
- Mode should be read from XOceanus's `AppearancePreference` on launch and respond to system dark mode changes via `NSAppearance` / JUCE's appearance observer.

#### A02 [CRITICAL] — Typography: Space Grotesk vs. APERTURE

The UIX Studio spec and prototype correctly use Space Grotesk (XOceanus's display typeface). The CLAUDE.md mentions "APERTURE font" in relation to XOceanus's new design language. Clarification needed: if APERTURE is the Rebirth-era typographic direction, then the tool headers (`OUTSHINE`, `ORIGINATE`) should use APERTURE for the title word, with Space Grotesk for all body labels. If APERTURE is not yet finalized or licensed, Space Grotesk remains correct for now.

**Current state:** The prototype uses Space Grotesk at 13px SemiBold for the window title — visually correct for the XOceanus family. No action required unless APERTURE is confirmed and licensed.

#### A03 [IMPROVEMENT] — Depth-Zone Gradients Are Missing from Zone Colors

XOceanus's aquatic mythology assigns visual materials based on depth (surface/abyss). The Outshine zone color palette (12 colors: Reef Jade, Crate Yellow, Axolotl Pink, etc.) is the correct XOceanus color language. But the current prototype uses these colors as flat fills. XOceanus's Tactile Mandate states: "Nothing is truly flat. Use subtle gradients, inner shadows, and easing curves." Zone blocks in the keyboard map should have a 1px inner shadow on the top edge (Issea's recommendation from the UIX spec — already documented; needs implementing) and a subtle 2% lightness gradient top-to-bottom.

#### A04 [IMPROVEMENT] — The Coupling Blending UI Needs XOceanus's Living Gold Corridor

Blessing B041 — Coupling-Inspired Sample Blending — is the most profound differentiator in Outshine. When two samples are coupled-blended, the visual language should reference XOceanus's Living Gold Corridor: a gold arc or bridge between the two source panel entries, pulsing at the blend rate. This is a single JUCE OpenGL or CoreGraphics arc drawn between two list rows. It communicates immediately that Outshine's blending is not a crossfader — it is the same coupling technology that powers XOceanus's engine interactions.

#### A05 [IMPROVEMENT] — The Header Bar Should Show Engine Accent Color When Rebirth Is Active

When Rebirth Mode is engaged with a specific engine (e.g., OSPREY — Azulejo Blue `#1B4F8A`), the XO Gold header bar should subtly blend the engine's accent color into the gradient: `linear-gradient(90deg, #EDD08A 0%, blend(#E9C46A, #1B4F8A, 0.15) 100%)`. This is a 1-line CSS change in the prototype and a trivial JUCE gradient override. It signals "this instrument was forged through OSPREY" at a glance.

#### A06 [STYLE] — Outshine's Window Should Be Launchable from XOceanus's File Menu

The integration path requires a `Tools > Open Outshine...` menu item in XOceanus's menu bar (JUCE `MenuBarModel`). The window should open as a floating panel, not a separate application. On macOS, this means `juce::DialogWindow` with `NSWindowStyleMaskFullSizeContentView` (as specified by Xavier in the UIX doc). The Outshine window should respond to XOceanus's window focus changes: dim to 90% opacity when XOceanus is not frontmost.

---

### [SAGE] — Hardware Compatibility

**Domain:** Will these tools produce packs that look and function correctly on MPC Live III, MPC X, MPC One+?

#### SG01 [CRITICAL] — Cover Art Dimensions Need Both Resolutions

The Export Complete panel mentions "auto-generated cover art" with a 120x120 preview thumbnail. The actual MPC requirements are:
- Primary: 2000x2000px PNG (displayed on MPC screen and in MPC software)
- Thumbnail: 1000x1000px PNG (displayed in expansion browser)

Both must be generated. The XPN packager (`xpn_cover_art.py`) appears to handle this — the Export panel UI should make both sizes explicit with file size estimates: `Artwork: 2000×2000 (~800 KB) + 1000×1000 (~220 KB)`.

#### SG02 [IMPROVEMENT] — MPC OS Version Guard on XY/MPCe Features

The MPCe quad-corner (XY per pad) feature requires MPC OS 3.3 or later (Live III / MPC XL only). If Outshine exports an MPCe-optimized pack, the completion panel should show: `MPCe Pad Layout:  [ACTIVE]  Requires MPC OS 3.3+ (Live III / MPC XL)`. Users with MPC One+ or earlier firmware will get a standard drum program — it will still work, just without the XY per-pad dimension.

**Fix:** Add an OS compatibility tag to any pack exported with MPCe features active. The `xpn_intent.json` already captures this metadata (from OXPORT_WIRING_TODO.md intent stage). Surface it in the Export Complete summary panel.

#### SG03 [IMPROVEMENT] — Sample Rate Warning for MPC Hardware

MPC hardware runs at 44.1kHz. The Originate render settings panel offers 48000Hz as the default (`Sample rate: [48000 Hz ▾]`). MPC will accept 48kHz WAVs, but will perform sample rate conversion internally — this costs quality and introduces subtle pitch artifacts on some firmware versions. For drum kits and percussive one-shots, 44100Hz is measurably better on hardware.

**Fix:** Change the Originate default to `44100 Hz` for the "MPC One/Live (recommended)" profile. Add a tooltip: "MPC hardware natively runs at 44.1 kHz. 48 kHz files are resampled internally." Keep 48000Hz available as an option — it is correct for studio bounce destinations.

#### SG04 [STYLE] — File Size Estimate Should Include MPC Program Slot Budget

MPC Live III has 16 programs per project by default (extendable to 64). A single Originate export (2 presets × 720 WAVs) already touches the file system heavily. The size estimate line should include a slot advisory: `Size: ~86 MB  •  ~4.2 min render  •  2 MPC program slots`. This helps producers plan their session before rendering.

---

## Kai's Unified Recommendations

### 1. Outshine UI — Updates Required for XOceanus Rebirth Aesthetic

**Tier 1 — Must Fix Before Ship (Blocking)**

| # | Finding | Action |
|---|---------|--------|
| R01 | KeyTrack toggle must not exist | Replace with read-only lock badge |
| R02 | VelStart gap UI missing | Add velocity ladder with split validation |
| A01 | Dark mode only; XOceanus is light mode default | Implement full light/dark theme switching |
| SC01 | No MPCe quad-corner assignment panel | Add 5th tab "MPCe Layout" with 4×4 grid |
| SG01 | Cover art only shows 120×120 preview | Confirm both 2000×2000 and 1000×1000 generated |

**Tier 2 — Ship + 1 (Strong Improvements)**

| # | Finding | Action |
|---|---------|--------|
| V01 | Forge metaphor dies after empty state | Ambient ember animations in progress/success states |
| V02 | Sonic DNA absent from Auto Mode | Add 32×32 DNA mini-radar per sample row |
| V03 | Rebirth Mode has no UI presence | Add Rebirth toggle in header, change Export label |
| R04 | ZonePlay mutual exclusion not enforced | Gray out velocity layers when round-robin on |
| SC03 | Originate preset list lacks MPC-readiness | Add Shape, WAVs, MPC columns |
| SG03 | 48kHz default wrong for MPC hardware | Change "MPC recommended" profile to 44100Hz |
| A04 | Coupling blend has no visual language | Gold arc bridge between coupled samples |

**Tier 3 — Nice-to-Have (Next Quarter)**

| # | Finding | Action |
|---|---------|--------|
| V03 | Contextual preset variation previews | 4 variant thumbnails in Export Complete |
| A05 | Header doesn't reflect active Rebirth engine | Subtle accent blend in gold gradient |
| SG02 | MPCe features need OS version guard | OS compatibility tag in pack + export panel |
| SC04 | "INSTRUMENT READY" should be "PROGRAM READY" | Dynamic label based on export type |
| SG04 | File size estimate missing program slot count | Add `N MPC program slots` to estimate line |

---

### 2. Originate Export Dialog — Target Design

The Originate dialog is well-conceived (UIX Studio's linear 5-step disclosure is correct). The specific changes Kai recommends:

**Step 1 (Preset Selection):**
- Add 3 columns: Sound Shape (2-char code), WAV estimate, MPC dot
- Add a filter: `[All Shapes ▾]` alongside existing engine/mood filters
- Presets with Sound Shape `Evolving` or `Texture` should show a yellow MPC dot by default (long renders, large files)

**Step 2 (Render Settings):**
- Change default sample rate from 48000 to 44100 Hz in "MPC One/Live (recommended)" profile
- Add "MPCe Mode" toggle (separate from Entangled Mode): when on, enables per-pad XY encoding in the output XPM
- Change `Velocity Layers [1 ▾][3 ▾][6 ▾][●12]` default selection: 12 is too heavy for general use; default should be 3 (matches "MPC Standard" profile from the export spec)
- Add estimated program slot count to the size estimate line

**Step 3 (Bundle Config):** No structural changes. Add cover art preview (already recommended in `xpn_export_enhancement_plan.md` as item 3c).

**Step 5 (Complete):**
- Replace "INSTRUMENT READY" with `DRUM PROGRAM READY` or `KEYGROUP PROGRAM READY` based on export type
- Show both cover art sizes with checksums
- Add MPC OS compatibility note when MPCe features are active

---

### 3. Oxport CLI — Visual Dashboard Companion Design

The 10-stage Oxport pipeline (`oxport.py`) is a sophisticated production tool. Its current form is purely CLI. For a product positioning of "the XPN suite Akai wishes they could have created," the CLI is an internal tool — it needs a dashboard companion for producers who are not comfortable in a terminal.

**The Oxport Dashboard — Concept**

A dedicated panel accessible from XOceanus's menu: `Tools > Oxport Dashboard`. It is a single-window monitoring and launch interface, not a configuration editor. Configuration remains in `.oxbuild` files (correct architecture — producers can edit YAML, engineers configure via CLI).

**Dashboard Layout (900×600px, XOceanus design language):**

```
╔══════════════════════════════════════════════════════════════════════╗
║  [XO Gold header, 40px — traffic lights float left]                  ║
║  OXPORT                                  Pack Pipeline    [?][×]     ║
╠═══════════════════╦══════════════════════════════════════════════════╣
║  PACKS            ║  mpce-perc-001.oxbuild                           ║
║  ─────────────    ║  ─────────────────────────────────────────────   ║
║  mpce-perc-001 ●  ║  ENGINE    ONSET  •  633 presets selected        ║
║  [+ New Pack…]    ║  PROFILE   mpce-perc-001  •  3 vel layers        ║
║                   ║  SAMPLES   256 WAVs  •  Stage 6–10 pending       ║
║                   ║                                                   ║
║                   ║  PIPELINE STAGES                                  ║
║                   ║  [1] PARSE      ✓   [6] ASSEMBLE    ✓            ║
║                   ║  [2] SELECT     ✓   [7] INTENT      ✓            ║
║                   ║  [3] RENDER     ✓   [8] DOCS        ✓            ║
║                   ║  [4] VALIDATE   ✓   [9] ART         ○            ║
║                   ║  [5] FALLBACK   ✓   [10] PACKAGE    ○            ║
║                   ║                                                   ║
║                   ║  [▸ Continue from Stage 9]   [Dry Run]           ║
╠═══════════════════╩══════════════════════════════════════════════════╣
║  LOG  ──────────────────────────────────────────────────────────     ║
║  [2026-03-22 08:14]  SELECT: 633 presets (farthest-point sampling)   ║
║  [2026-03-22 08:16]  RENDER: 256 WAVs complete  •  0 errors          ║
║  [Last entry]        VALIDATE: Stages 6–10 not run                   ║
║                                                      [Clear] [Copy]  ║
╚══════════════════════════════════════════════════════════════════════╝
```

**Key behaviors:**
- Stage indicators: green checkmark (complete), gold spinner (active), gray circle (pending), red X (failed with clickable error detail)
- "Continue from Stage N" button auto-computes the next unrun stage from the build manifest — producers never need to know stage numbers
- Log panel streams stdout from the subprocess in real-time (NSTask/Process piped to a juce::TextEditor)
- Pack list sidebar reads `packs/*.oxbuild` files from the repo — no separate configuration needed
- The `.xpn` output file, when complete, shows a "Reveal in Finder" button and a "Load into MPC" button (copies to the MPC's USB-mounted expansion folder if detected)

**"Load into MPC" Feature:** Scout flags this as the single most valuable UX improvement over any competing XPN tool. When the MPC is connected via USB and mounted as a drive, a "Send to MPC" button copies the `.xpn` to `MPC/Expansions/` and triggers a filesystem refresh. No other XPN tool offers this in a GUI. This is a first-mover differentiator.

---

### 4. New Tool Ideas from the Team

#### [ATLAS] — Oxbuild Wizard
Producers unfamiliar with YAML should not have to hand-edit `.oxbuild` files. A 4-step wizard (accessible from the Oxport Dashboard "New Pack..." button) walks through: engine selection, preset count target, velocity layer count, profile selection, and output path. Generates a valid `.oxbuild` file. No DSP changes required — pure UI.

#### [SCOUT] — Pack Preview Player
Before exporting a full pack, producers need to hear representative samples. A "Preview Mode" in Oxport Dashboard: plays 8 auto-selected presets (one per mood) through XOceanus's engine via BlackHole loopback, without writing any files. Confirms the pack sounds correct before a 4-minute render. Critical for the competitive window — saves iteration time.

#### [REX] — XPN Diff Tool
When updating an existing pack (v1.0 → v1.1), there is no way to see what changed in the XPM structure. A diff view in the Oxport Dashboard comparing two `.xpn` archives (side-by-side tree with highlighted differences in zone ranges, velocity splits, sample names) would be standard tooling for any professional pack publisher. Native Instruments' tools have this; Akai's don't. Another first-mover opportunity.

#### [VIBE] — Sonic DNA Pack Visualizer
After SELECT runs, visualize the DNA spread of chosen presets as a 2D scatter plot (brightness × warmth axes, color-coded by sound shape). If the pack is unbalanced (all presets clustered in one corner), the visualizer makes the problem visible before rendering. A well-balanced pack (DNA spread across the space) produces a more versatile, more marketable expansion.

#### [HEX] — Note: MPC Standalone Plugin Integration Status
Hex reports: no path to running Outshine or Originate as standalone plugins on MPC Live III (MPC's embedded Linux does not support AUv3 or JUCE plugins natively). The correct architecture is exactly what is designed: desktop tool producing `.xpn` output, loaded via USB onto hardware. Hex is watching for any MPC OS update that adds third-party plugin support — none detected as of March 2026.

---

## Priority Action List

| Priority | Item | Owner | Blocking? |
|----------|------|-------|-----------|
| 1 | Remove KeyTrack toggle → read-only badge | Dev | Yes — XPN correctness |
| 2 | Add VelStart validation UI to Velocity tab | Dev | Yes — XPN correctness |
| 3 | Implement light mode theme (Outshine prototype) | Dev | Yes — XOceanus sibling |
| 4 | Change Originate default sample rate to 44100 | Dev | Yes — MPC hardware |
| 5 | Add MPCe Layout tab (4×4 pad grid) to Outshine | Dev | Yes — competitive window |
| 6 | Confirm cover art generates both 2000×2000 + 1000×1000 | Dev | Yes — MPC display |
| 7 | Add DNA mini-radar to Outshine source panel rows | Dev | No |
| 8 | Add Rebirth toggle to Outshine header | Dev | No |
| 9 | Add Sound Shape + MPC dot columns to Originate preset list | Dev | No |
| 10 | Build Oxport Dashboard UI | Dev | No |
| 11 | Implement ZonePlay mutual exclusion warning | Dev | No |
| 12 | Design Oxbuild Wizard (4-step) | Design first | No |
| 13 | Implement "Send to MPC" button in Oxport Dashboard | Dev | No |
| 14 | Build Pack Preview Player (8-preset audition) | Dev | No |

---

## Closing Note from Kai

The UIX Studio (Ulf, Issea, Xavier) built a strong foundation. The bones are correct: the forge metaphor, the progressive disclosure architecture, the XO Gold brand language, the keyboard zone map as the product's center of gravity. What the Studio built is an excellent sample editor. What the MPC community needs is the best XPN authoring tool that has ever existed — including anything Akai or Native Instruments have shipped internally.

The delta between those two things is not a redesign. It is:
- Golden Rule enforcement (Rex's domain — 3 changes)
- MPCe tab (Scout's competitive mandate — 1 new tab)
- Light mode implementation (Atlas's XOceanus fidelity mandate — theme switch already specified, not yet built)
- 44.1kHz default correction (Sage's hardware truth — 1 settings change)
- The Oxport Dashboard (a new tool that wraps an existing tool in a face that producers can use)

These are achievable in 2-3 development sessions. The competitive window is month 5 of 6-12. Build the MPCe tab first. It is the screenshot that defines the category.

---

*Kai + Android Team — Rex, Vibe, Scout, Atlas, Sage, Hex*
*Reference files: `DESIGN_SPECIFICATION_OUTSHINE_ORIGINATE.md`, `Docs/mockups/outshine-prototype.html`, `Docs/seance-outshine-2026-03-22.md`, `Tools/OXPORT_USER_GUIDE.md`, `Tools/OXPORT_WIRING_TODO.md`, `Docs/plans/xpn_export_enhancement_plan.md`*
