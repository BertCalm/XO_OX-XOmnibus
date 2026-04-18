# Oxport Tool Suite — Competitive Analysis & R&D Spec
**Date**: 2026-03-16 | **Status**: Draft | **Author**: XO_OX Designs

---

## Overview

This document compares the XO_OX Oxport tool suite against pack creation pipelines used by major
expansion studios. It identifies Oxport's unique capabilities, capability gaps vs. industry standard,
the critical fleet render bottleneck, a 12-month v2 vision, and the open source strategy question.

"Oxport" is the umbrella name for all tooling under `Tools/` — Python scripts, the `oxport.py`
orchestrator, and the `XPNExporter.h` C++ export module inside XOceanus.

---

## 1. How Competing Studios Build Packs

### Drum Broker
- **Workflow**: Manual curation on hardware (MPC, SP-404, vinyl rips) → Ableton bounce → manual
  ZIP packaging → Shopify/Gumroad upload
- **Automation**: None reported. No public tooling.
- **Strengths**: Curation taste, brand identity, community loyalty
- **Weaknesses**: No metadata standard, no velocity mapping tooling, no format targeting beyond
  loose WAV dumps. Pack quality is 100% human-dependent.

### MSXII Sound Design
- **Workflow**: Native MPC workflow on hardware → file export → minimal post-processing
- **Automation**: Minimal. Naming conventions are manual. No reported automated velocity layering.
- **Strengths**: Deep MPC platform knowledge, authentic performance workflow
- **Weaknesses**: No programmatic format validation, no batch processing, no analytics.
  Pack consistency is operator skill.

### Output
- **Workflow**: Proprietary NKS/Kontakt-first pipeline. Samples recorded/designed in-house,
  curated by sound designers, exported via custom Kontakt scripting or internal tools.
- **Automation**: Internal tooling for NKS tagging, Kontakt library packaging, and Arcade streaming
  metadata. Not public.
- **Strengths**: NKS deep integration, Arcade streaming model, polished installer UX
- **Weaknesses**: Locked to Native Access ecosystem. MPC/hardware export is not a focus.
  Their pipeline is proprietary and cannot be inspected or built upon.

### Splice
- **Workflow**: Web platform — contributors upload stems/one-shots; Splice runs auto-normalization,
  waveform analysis, BPM/key detection, and metadata tagging server-side. Users stream or download.
- **Automation**: Auto-normalizer, MP3 preview generation, waveform image render, metadata DB,
  similarity detection. All server-side.
- **Strengths**: Distribution scale, streaming/rental revenue model, auto-generated previews,
  community contributor network
- **Weaknesses**: No instrument-specific format targeting (MPC, Kontakt, Ableton). Samples are
  de-contextualized from instruments. No synthesis integration.

### Native Instruments / Maschine
- **Workflow**: Internal factory pipeline using custom Maschine expansion builder tools (not public).
  NKS spec defines the interchange format. Third-party publishers (e.g., Goldbaby, Sample Magic)
  use NKS Builder and Maschine Controller Editor for pad assignment.
- **Automation**: NKS Builder handles tag mapping and preview rendering. Maschine packs require
  custom pad/group XML that mirrors XPM in concept but is Maschine-native.
- **Strengths**: Deep hardware integration, NKS is a de facto standard for controller hardware
- **Weaknesses**: Closed ecosystem, Windows-first tooling. No open API for third-party format
  extensions. Community builders must reverse-engineer pack format edge cases.

### Akai / MPC Expansions (first-party)
- **Workflow**: Internal tooling, entirely opaque. The MPC XPN/XPM format is documented only
  by community reverse engineering. No official SDK for third-party expansion building.
- **Strengths**: Native hardware integration, format authority
- **Weaknesses**: No public toolchain. Community builders (including XO_OX) operate without
  official support, relying on format archaeology.

---

## 2. Oxport Unique Capabilities

The following capabilities do not exist in any publicly known competitor pipeline.

### 2.1 DNA-Based Pack Analytics and Fleet Gap Detection
**Tools**: `xpn_pack_analytics.py`, `audit_sonic_dna.py`, `compute_preset_dna.py`

Oxport encodes every preset and kit with 6D Sonic DNA (brightness, warmth, movement, density,
space, aggression). `xpn_pack_analytics.py` reports DNA coverage distribution across a pack and
flags under-represented zones (e.g., "no high-density / high-aggression content"). Fleet-level
gap detection compares a new pack's DNA centroid against the full 2,550-preset library to prevent
redundancy.

No competitor has anything equivalent. Drum Broker and MSXII produce packs intuitively; Output
and NI have internal A&R processes, but none expose a quantitative DNA map.

### 2.2 Adaptive Velocity Boundaries Per Instrument Classifier
**Tools**: `xpn_adaptive_velocity.py`, `xpn_classify_instrument.py`

Rather than applying flat velocity zones (0–42 / 43–84 / 85–127), Oxport classifies each sample
by instrument type (kick, snare, hat, clap, tom, perc, fx, tonal) and applies instrument-specific
velocity curves. Kicks use a bottom-heavy curve that emphasizes sub impact at low velocity;
hi-hats use a compressed curve reflecting natural dynamic range. This is not available in any
public drum pack toolchain.

### 2.3 Setlist Builder with Arc Scoring
**Tool**: `xpn_setlist_builder.py`

Generates ordered performance setlists from a fleet of XPN packs, scoring each ordering for
dynamic arc (tension/release, energy progression) using DNA values. No competitor ships tooling
for pack arrangement as a performance narrative. Splice's model is a-la-carte by design;
Maschine's set management is a DAW-level concern.

### 2.4 feliX-Oscar Instrument Bias Detection
**Tool**: `xpn_pack_analytics.py` (feliX-Oscar axis), `xpn_optic_fingerprint.py`

Every XO_OX pack is evaluated against the feliX-Oscar polarity axis — feliX (bright, playful,
treble-forward) vs. Oscar (dark, heavy, sub-forward). The tool detects imbalanced packs and
flags over-concentration on one pole. This is a brand-specific innovation tied to XO_OX's
aquatic mythology and has no equivalent in any competitor.

### 2.5 Evolution Builder (XPM Morphing)
**Tool**: `xpn_evolution_builder.py`

Generates intermediate XPM states between a source and target program — effectively morphing
between two kit configurations across N steps. The output is a sequence of XPMs suitable for
live progressive kit replacement on MPC. No competitor produces this kind of temporal morphing
in the pack creation phase.

### 2.6 Automated Choke Group Assignment
**Tool**: `xpn_choke_group_assigner.py`

Analyzes sample names and instrument classification to automatically assign MPC choke groups
(open hi-hat choked by closed, etc.). Competitors either require manual pad assignment or
provide hardware-level UI for this. Oxport is the only known tool that automates choke assignment
from sample metadata.

### 2.7 MPCe Quad-Corner Stems Checker
**Tool**: `xpn_stems_checker.py`

Validates that stems are organized into the four MPCe performance quadrants (Groove, Texture,
Melodic, Harmonic) with correct balance across corners. This is XO_OX-specific but mirrors a
real MPCe use case that has no automated validation elsewhere.

### 2.8 Curiosity Engine and Speculative Kits
**Tools**: `xpn_curiosity_engine.py`, `xpn_ca_presets_kit.py`, `xpn_attractor_kit.py`,
`xpn_entropy_kit.py`, `xpn_lsystem_kit.py`, `xpn_pendulum_kit.py`, `xpn_turbulence_kit.py`

Generative kit builders seeded from mathematical/physical systems (cellular automata, strange
attractors, L-systems, gravitational waves, etc.). No competitor ships generative pack tooling
at this conceptual level.

---

## 3. Capability Gaps vs. Industry Standard

### 3.1 No Audio Preview Generation — P1
**What Splice does**: Auto-generates an MP3 waveform preview for every uploaded sample, available
for in-browser streaming before purchase.

**What Oxport does**: `xpn_preview_generator.py` exists in the tool suite but generates preview
metadata (suggested clip points, waveform envelope data) — not an actual rendered MP3 or WAV.
The actual audio render requires a manual step through XOceanus standalone or a DAW.

**Gap**: Buyers on Gumroad or the future XO-OX.org store cannot hear a pack before purchase
without the user manually recording and uploading audio clips. The 20 hero preset audio clips
noted in `site-sample-recordings.md` are a manual workaround, not a systematic solution.

### 3.2 No Web Upload Integration — P1
**What Splice does**: CLI or web uploader with API authentication, batch metadata push, asset
validation, and publication workflow in one command.

**What Oxport does**: `xpn_packager.py` and `xpn_submission_packager.py` produce ZIP bundles and
manifests but terminate at the filesystem. Upload to Gumroad, Bandcamp, or XO-OX.org is manual.

**Gap**: Every pack release requires a manual upload step. Automatable with a Gumroad API
integration or a simple S3/CDN push script.

### 3.3 No Streaming / Rental Model — P2
**What Splice does**: Monthly subscription gives access to a rolling catalog. Samples are rented,
not owned. Revenue model is per-download triggered by credit spend.

**What Oxport targets**: One-time download (Gumroad/direct). No streaming infrastructure.

**Gap**: Not necessarily a gap to close — the XO_OX model is hardware-first (MPC XPN packs),
which is inherently a download model. Streaming adds significant infrastructure cost for modest
benefit given the platform. Revisit if XO-OX.org adds a web player.

### 3.4 No Hardware Render Integration — P0 CRITICAL (see Section 4)

### 3.5 No Cross-DAW Format Targeting — P2
**What NKS Builder does**: Single source library exports to Maschine, Komplete Kontrol, NKS.

**What Oxport does**: `xpn_to_sfz.py` exists (SFZ format), and XPN is MPC-native. No Ableton
Rack, Logic EXS, or Maschine format output.

**Gap**: Real but low priority for the current MPC-first audience. SFZ coverage provides a
DAW-agnostic fallback. P2 or P3 depending on audience expansion goals.

### 3.6 No Metadata Database — P2
**What Splice does**: Central DB for all uploaded content with BPM, key, instrument tags, mood
tags, genre tags, waveform data, similar-sample links.

**What Oxport does**: Per-pack `manifest.json` files generated by `xpn_manifest_generator.py`
plus DNA fields in `.xometa`. No central queryable DB spanning the full fleet.

**Gap**: A simple SQLite or JSON fleet index, updated by CI on each pack release, would enable
fleet-wide search and cross-pack deduplication checks. Relatively low-effort, meaningful gain.

---

## 4. The Fleet Render Gap (P0 Critical)

**This is the most consequential missing piece in the Oxport pipeline.**

### The Problem

Every Oxport tool operates on XPM/XPN metadata and sample files. None of them can produce the
sample audio itself. When a new pack requires rendered audio — a new drum hit, a synthesized
one-shot, a multi-sampled keygroup — the workflow is:

1. Open XOceanus standalone
2. Navigate to the target engine and preset
3. Trigger the note manually (or via MIDI)
4. Record the output in a DAW (Logic, Ableton)
5. Trim, name, and move the WAV to the correct `Tools/samples/` subdirectory
6. Re-run the relevant Oxport tool

Steps 2–5 are manual, slow, and do not scale. For an engine with 150 presets and 4 sample
zones per preset, that is 600 individual render operations. This is the bottleneck blocking
XPN fleet scale.

### The Solution: `renderNoteToWav()` in XPNExporter.h

`Source/Export/XPNExporter.h` already contains the export pipeline skeleton. The missing method:

```cpp
// OPUS-DEFERRED — not yet implemented
void renderNoteToWav(
    int engineIndex,
    const String& presetPath,
    int midiNote,
    float velocity,
    float durationSeconds,
    const File& outputWav
);
```

This method would:
1. Load the target preset into an engine slot (non-audio thread, via `EngineRegistry`)
2. Construct an `OfflineAudioContext`-equivalent using JUCE's `AudioBuffer` + engine `processBlock()`
3. Synthesize N seconds of audio triggered at the specified note/velocity
4. Write a 24-bit WAV to `outputWav`

**Why it is deferred**: The implementation requires careful offline audio threading (cannot touch
the live `AudioContext`), accurate envelope completion detection (don't truncate release tails),
and sampleRate parity with the live engine. This is Opus-level DSP work. It was deferred in the
Prism Sweep to avoid destabilizing the live audio thread during the quality pass.

### What Closing This Gap Enables

| Currently Manual | After `renderNoteToWav()` |
|-----------------|--------------------------|
| Record 20 site hero clips | `oxport render-fleet --mode hero` |
| Render XObese XPN bundle samples | `oxport render-pack XObese` |
| Build multi-velocity keygroup layers | `oxport render-keygroups --velocities 32,64,96,127` |
| Generate audio previews for Gumroad | `oxport render-previews --format mp3` |
| Create evolution builder audio demos | `oxport render-evolution --steps 8` |

This is the highest-leverage engineering investment in the Oxport roadmap. It transforms
Oxport from a metadata pipeline into a complete end-to-end production system.

**Estimated scope**: 3–5 Opus sessions. Prerequisite: offline render architecture design doc.

---

## 5. Oxport v2 Vision — 12-Month Roadmap

### P0 (Ship before launch)
| Item | Description |
|------|-------------|
| `renderNoteToWav()` | Fleet render integration — closes the manual render bottleneck |
| Gumroad upload integration | `oxport publish --platform gumroad` — automated ZIP + asset upload |
| Fleet DNA index | SQLite fleet DB updated on each pack build; queryable by all tools |

### P1 (within 6 months of launch)
| Item | Description |
|------|-------------|
| Audio preview generation | Post-render: auto-generate 15s MP3 previews from rendered WAVs |
| Waveform image render | SVG/PNG waveform thumbnails for web store product pages |
| Maschine format output | XPN → Maschine expansion XML via `xpn_to_maschine.py` |
| CI pack validation | GitHub Actions gate on `xpn_validator.py` + `xpn_qa_checker.py` |

### P2 (6–12 months post launch)
| Item | Description |
|------|-------------|
| XO-OX.org upload API | Direct publish to site store from `oxport publish` |
| Ableton Rack export | `xpn_to_alc.py` — one-shot → Rack Device with auto-mapped pads |
| Community submission pipeline | `xpn_community_qa.py` + `xpn_submission_packager.py` → full review queue |
| DAW session generator | Export a Logic/Ableton session pre-loaded with a pack's stems in quad-corner layout |

### P3 (Exploratory / longer-term)
| Item | Description |
|------|-------------|
| Streaming infrastructure | If XO-OX.org adds a web player: server-side sample streaming |
| Similarity detection | Cross-pack deduplication using spectral fingerprint comparison |
| AI-assisted curation | DNA clustering to suggest pack names, mood tags, arc ordering |
| Physical hardware export | Direct MPC WiFi push via MPC API (if Akai exposes one) |

---

## 6. Open Source Strategy

### The Question

Should Oxport tools (`Tools/*.py` and `Source/Export/XPNExporter.h`) be released as open source?
The XOceanus plugin itself is already open source (the CLAUDE.md describes it as
"free, open-source"). The tools are less clearly defined.

### Arguments For Open Source

**Community benefit is high.** The MPC XPN/XPM format has no official SDK. Every independent
expansion creator works from community reverse engineering. Releasing Oxport's format handling
code — especially `xpn_validator.py`, `xpn_drum_export.py`, and `xpn_keygroup_export.py` —
would be a significant contribution to the MPC creator community.

**Network effects.** An open-source Oxport becomes a reference implementation. Third-party
creators build on it, contribute improvements, and expand MPC format compatibility. XO_OX
benefits from defect reports and format edge-case discoveries at community scale.

**Brand signal.** Open tooling reinforces the XO_OX brand identity as a maker/community platform
rather than a closed commercial operation. Consistent with the "free, open-source" plugin.

**No real competitive risk on the format tools.** Drum Broker, MSXII, and Splice are not MPC-XPN
shops. NI is not a competitor in this space. The XPN format tools have no meaningful value to
any current competitor.

### Arguments Against (or For Partial Release)

**Oxport's DNA tools are the moat.** `xpn_pack_analytics.py`, `xpn_pack_score.py`,
`xpn_setlist_builder.py`, `xpn_evolution_builder.py`, and the DNA/feliX-Oscar analytics are
not format tooling — they are XO_OX's brand intelligence. Releasing these would allow a
well-resourced competitor to clone the curatorial framework without the years of conceptual
development behind it.

**`renderNoteToWav()` has real competitive value once built.** A working headless XOceanus
renderer would be a meaningful engineering asset. It should not be open-sourced until the
competitive landscape is better understood.

### Recommendation: Tiered Release

| Tier | Tools | Release Status |
|------|-------|---------------|
| **Open** | Format tools: `xpn_validator.py`, `xpn_drum_export.py`, `xpn_keygroup_export.py`, `xpn_kit_expander.py`, `xpn_bundle_builder.py`, `xpn_packager.py`, `xpn_normalize.py`, `xpn_smart_trim.py`, `xpn_choke_group_assigner.py` | Release at launch |
| **Open** | Utility tools: `xpn_manifest_generator.py`, `xpn_cover_art.py`, `xpn_liner_notes.py`, `xpn_pack_readme_generator.py` | Release at launch |
| **Hold** | Analytics/DNA tools: `xpn_pack_analytics.py`, `xpn_pack_score.py`, `xpn_setlist_builder.py`, `xpn_evolution_builder.py`, `xpn_auto_dna.py`, `xpn_optic_fingerprint.py` | Hold for later review |
| **Hold** | Render pipeline: `XPNExporter.h` render methods, `renderNoteToWav()` | Hold for later |

A dedicated `oxport-tools` GitHub repository can host the Open tier with MIT license.
The Hold-tier tools remain in the XOceanus monorepo.

---

## Summary

| Dimension | XO_OX Oxport | Industry |
|-----------|-------------|---------|
| MPC XPN format depth | Best known public implementation | No public competition |
| DNA analytics | Unique — no equivalent exists | Not attempted |
| Audio preview generation | Gap — metadata only, no render | Splice: full auto-generation |
| Web upload integration | Gap — manual step | Splice: API-driven |
| Streaming model | Not applicable (hardware-first) | Splice: core model |
| Fleet render (P0 critical) | Gap — manual DAW step required | NI/Output: internal tools |
| Generative kit tooling | Unique — mathematical system kits | Not attempted publicly |
| Format breadth | XPN + SFZ | NI: NKS + Maschine; Output: Kontakt |

The Oxport suite is ahead of any publicly known toolchain in MPC format fidelity, pack analytics,
and generative content. The single highest-leverage gap to close before launch is `renderNoteToWav()`
in `XPNExporter.h` — without it, every pack still requires manual DAW recording sessions that
do not scale to the 34+ engine fleet.

---

*Next actions: Opus session for offline render architecture → `renderNoteToWav()` implementation
plan → Gumroad API integration spike.*
