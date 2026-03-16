# JUCE to MPC Workflow: XOmnibus to XPN Pack

**Status:** Internal reference guide | March 2026
**Scope:** Complete pipeline from sound design in XOmnibus to production-ready XPN pack on MPC pads

---

## Overview

This is the end-to-end process for turning an XOmnibus preset into a deployable MPC expansion pack.
The pipeline has four phases. Without Fleet Render Automation, the bottleneck is Phase 1 — manual
recording. With Fleet Render, Phase 1 drops from several hours to minutes and the entire 20-preset
pack closes in ~30 minutes.

**Time estimates (20-preset melodic pack):**

| Phase | Manual render | Fleet Render (future) |
|-------|-------------|----------------------|
| Phase 1 — Sound design + render | 3–5 hours | 10–15 min |
| Phase 2 — Post-processing | 20–30 min | 5–10 min |
| Phase 3 — XPM assembly | 15–20 min | 5 min |
| Phase 4 — Pack finalization | 20–30 min | 10 min |
| **Total** | **~4–6 hours** | **~30 min** |

The current bottleneck is clear: manual recording of every note at every velocity layer is the
overwhelming majority of the work. Fleet Render Automation (`Source/Export/XPNExporter.h`) has
~80% of the scaffolding complete; the blocking item is `renderNoteToWav()` in lines 602–616 of
that file, which currently generates silence. Once that method routes audio through the engine,
the entire pipeline becomes viable at scale for the full 2,550-preset fleet.

---

## Phase 1: Sound Design in XOmnibus

**Estimated time: 3–5 hours (manual) | 10–15 min (Fleet Render)**

### Setting Up for Pack Export

Design with the MPC playback context in mind from the start. The MPC is a performance-first
instrument — presets that read as interesting in a DAW can feel static on pads. Key principles:

- **Velocity drives timbre, not just volume** (Doctrine D001). Every sound should be noticeably
  brighter, denser, or more aggressive at high velocity. The MPC pad response clusters in the
  50–110 MIDI range; that zone must feel expressive.
- **Short releases for drums and stabs** — pads need sounds that cut cleanly when released.
  Tails should be designed into the sample, not left to the synth envelope during playback.
- **Limit modulation complexity** — all LFOs and envelopes will be baked into the sample. The
  snapshot captures a moment in time. For keygroup sounds, either render a "settled" state or
  render with modulation synced so the loop point is clean.
- **Pitch center** — for melodic keygroups, tune to C3 (MIDI 60) unless the character of the
  sound demands a different register. Deviation from C3 is handled by `xpn_auto_root_detect.py`
  but obvious detuning wastes render time.

### The 4 Macros and Their XPN Translation

XOmnibus presets expose 4 macros: CHARACTER, MOVEMENT, COUPLING, SPACE. These do not survive
into an XPN keygroup as interactive parameters — they are baked into which preset state you
render. Before rendering:

- **CHARACTER** — Choose a single macro position that represents the pack's identity. Do not
  render multiple CHARACTER positions as separate samples; that is a separate preset/kit decision.
- **MOVEMENT** — Set to the intended performance feel. A pad pack centered on slow evolving textures
  should render with MOVEMENT at a comfortable mid position, not maxed.
- **COUPLING** — Set COUPLING to 0 unless the coupled behavior is the defining character of the
  sound. Rendering cross-engine coupling introduces timbral instability that can vary between takes.
- **SPACE** — Set reverb/delay tail to taste, then render. The reverb IS the sound for atmospheric
  pads; for drums, render dry and let the MPC program handle space.

### Recording Renders

**DAW recording:** Highest quality control. Automate MIDI notes, render tracks offline. Drawback:
requires loading XOmnibus as a plugin, playing each note, naming files manually. For a 20-preset
pack at 4-layer velocity × 13 notes per octave, that is 1,040 individual recordings.

**Standalone rendering:** Load XOmnibus standalone, use system audio recording (e.g., BlackHole or
Loopback). Same bottleneck as DAW recording — manual, laborious.

**Fleet Render Automation (future):** Once `renderNoteToWav()` is implemented in `XPNExporter.h`,
the CLI target will accept a preset name, note range, and velocity spec and produce all WAVs
automatically. This is the only economically viable path for the full fleet.

### Note Coverage: How Many Notes to Render

| Pack type | Notes per octave | Reasoning |
|-----------|-----------------|-----------|
| Drum / one-shot | 1 per pad (no pitch range) | Fixed pitch; keygroup spans full range from one root |
| Simple melodic (pads, strings) | Every 6th semitone (C, F#) | Pitch-shift artifacts are minor; 2 roots per octave sufficient |
| Harmonically sensitive (piano, plucked strings) | Every 3rd semitone (C, Eb, F#, A) | Reduces stretch artifacts across the octave |
| Hero instruments requiring maximum realism | Every semitone | Only justified for flagship instruments; adds 12× render time |

The standard for XO_OX melodic packs is **every 6th semitone** across a 4-octave range
(C1–C5 = 9 render points). For character instruments where pitch-tracking is secondary to timbre,
a single root note with wide keygroup stretch zones is acceptable.

### Velocity Layers: Vibe Musical Curve Boundaries

The **Vibe Musical Curve** redistributes velocity boundaries toward the 50–110 performance zone
where real MPC pad strikes cluster. Equal-division layers (0–31, 32–63, 64–95, 96–127) waste
resolution at the extremes.

| Layer count | Vibe boundaries (upper limit per layer) |
|-------------|----------------------------------------|
| 2 layers | 80 / 127 |
| 4 layers (standard for melodic) | 40 / 72 / 100 / 127 |
| 8 layers (standard for drums) | 20 / 40 / 58 / 72 / 84 / 96 / 108 / 127 |

Render samples at the **midpoint** of each boundary zone, not at the boundary itself:
- 4-layer: velocities 20, 56, 86, 114
- 8-layer: velocities 10, 30, 49, 65, 78, 90, 102, 118

This ensures the rendered sample is representative of the full zone's timbral character, not an
edge case at a transition boundary.

---

## Phase 2: Post-Processing

**Estimated time: 20–30 min manual | 5–10 min automated**

Run these tools in sequence on the raw WAV exports:

1. **`xpn_normalize.py`** — Level normalization. Applies -0.3 dBFS peak ceiling across all samples
   in the batch. Run this before trimming; normalizing after trim can introduce inter-sample level
   inconsistency if the trim points differ in loudness.

2. **`xpn_smart_trim.py`** — Detects and removes pre-attack silence and post-tail silence using
   threshold-based onset detection. Preserves natural attack transients. Drum samples get hard
   zero-crossing trims; melodic pads get a 5ms fade-in to prevent clicks.

3. **`xpn_auto_root_detect.py`** — Analyzes pitch of each sample via autocorrelation and assigns
   a MIDI root note. Cross-check the output on any non-pitched or heavily detuned sounds — the
   detector can miscode noise-forward transients.

4. **`xpn_classify_instrument.py`** — Assigns instrument category metadata (Drum / Bass / Melodic /
   Texture / SFX). This classification gates which XPM template `oxport.py` uses downstream and
   which velocity response mode is applied.

---

## Phase 3: XPM Assembly

**Estimated time: 15–20 min | 5 min automated**

1. **`oxport.py`** — Core XPM generator. Reads the classified WAV folder, root note metadata, and
   velocity layer map, and produces `.xpm` program files. Enforces the 3 non-negotiable MPC rules:
   `KeyTrack=True`, `RootNote=0` (MPC convention for auto-detect), and empty layer `VelStart=0`
   (prevents ghost triggering).

2. **`xpn_adaptive_velocity.py`** — Applies the Vibe Musical Curve velocity response to each
   keygroup layer. Also sets the velocity-to-volume curve (square-root shape for melodic instruments;
   flat/pre-baked for drums) and optionally adds a velocity-to-filter-cutoff modulation for
   instruments where timbral brightness should scale with force.

3. **`xpn_coupling_docs_generator.py`** — Writes CouplingDNA comment blocks into the XPM files.
   These comments are human-readable metadata (not parsed by MPC OS) that document which
   XOmnibus coupling configurations were active when the samples were rendered. Preserves the
   sonic genealogy for future re-renders or pack updates.

4. **`xpn_pack_score.py`** — Scores the assembled pack against a rubric: note coverage breadth,
   velocity layer count, naming clarity, DNA completeness, sample length consistency. A score
   below 70/100 should block release; most well-designed packs land 80–90.

---

## Phase 4: Pack Finalization

**Estimated time: 20–30 min**

1. **`xpn_cover_art.py`** — Generates 600×600px cover art using the engine's accent color,
   engine name, and pack title. Output is a JPEG placed in the pack root alongside the XPM files.
   For batch art generation across multiple packs, use `xpn_cover_art_batch.py`.

2. **`xpn_manifest_generator.py`** (via `xpn_packager.py`) — Produces `manifest.json` with pack
   metadata: name, version, engine source, preset count, mood category, Sonic DNA summary, and
   tool version fingerprint. This manifest drives the XO-OX.org download page and future storefront.

3. **`xpn_validator.py`** — Hard validation pass. Checks all XPM XML is well-formed, all referenced
   WAV files exist at the expected relative paths, all required metadata fields are populated, and
   no duplicate sample names exist within the pack. Failures here block packaging; warnings are
   documented but do not block.

4. **`xpn_community_qa.py`** — Soft validation: checks naming conventions, flags any preset names
   that appear in other published packs (dedup), verifies sample lengths are within MPC OS limits
   (max 2GB per program, practical limit ~500MB for responsiveness), and generates a human-readable
   QA report.

5. **`xpn_bundle_builder.py`** — Final step. Zips the validated pack directory into a `.xpn`
   bundle with the correct internal structure: `Programs/`, `Samples/`, `manifest.json`, and
   cover art. The resulting `.xpn` file is ready for MPC OS import or XO-OX.org distribution.

---

## Current Bottleneck Summary

The entire pipeline from design to bundle can complete in ~30 minutes once Fleet Render Automation
is functional. Until then, the render step (Phase 1) is a multi-hour manual process that limits
pack throughput to approximately 1–2 packs per work session regardless of how fast the post-
processing and assembly steps run.

**The single implementation task that unlocks scale:** Complete `renderNoteToWav()` in
`Source/Export/XPNExporter.h` lines 602–616. All other infrastructure is built and tested.

---

*See also: `fleet_render_automation_spec.md` | `velocity_science_rnd.md` | `xpn-tools.md` (memory)*
