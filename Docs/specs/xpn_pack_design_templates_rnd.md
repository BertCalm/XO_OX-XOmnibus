# XPN Pack Design Templates — R&D Spec
**Date**: 2026-03-16 | **Status**: Draft | **Author**: XO_OX Designs

---

## Overview

A **pack design template** is a reusable structural blueprint for an XPN pack. It defines the
architecture of a pack — program count, naming arc, velocity layer strategy, Q-Link assignments,
and feliX-Oscar distribution — without specifying sounds. The same template can be applied to
different engines with minimal adaptation.

Six templates cover the full range of XO_OX instrument families:

| # | Template Name | Primary Engines | Programs |
|---|---------------|----------------|---------|
| T1 | The Drum Foundation | ONSET, OBLONG (perc), OSTINATO | 12–16 |
| T2 | The Voice Portrait | OBLONG, OBBLIGATO, OHM, OTTONI, OLE | 8–12 |
| T3 | The Bass Architecture | OBESE, ORCA, OCEANIC (bass ranges) | 8–10 |
| T4 | The Atmospheric Journey | ORACLE, ORIGAMI, OBSCURA, OPAL | 6–8 |
| T5 | The Character Machine | OVERDUB, ODYSSEY, OUROBOROS, OVERBITE | 10–14 |
| T6 | The MPCe Quad Pack | All 34 engines | 4–8 |

Each template entry below includes: intent, program table, naming arc rules, Q-Link layout,
velocity strategy, feliX-Oscar distribution, and adaptation notes.

---

## T1 — The Drum Foundation

**Intent**: Percussion-focused packs that build from minimal kits to full rhythmic ecosystems.
Suited to engines with multiple independent voice channels (kick, snare, hats, etc.).

**Target engines**: ONSET, OBLONG (percussion mode), OSTINATO

### Program Table

| Slot | Zone | Character |
|------|------|-----------|
| 1–3 | Sparse | 3–4 voice kits — sub, snap, one texture |
| 4–6 | Building | 5–6 voice kits — adding hats, claps |
| 7–10 | Full | Complete 8-voice kits, maximum density |
| 11–13 | Hybrid | Mixed acoustic/electronic or pitched drum |
| 14–16 | FX/Close | Heavy processing, glitch, or abstraction |

Minimum 12 programs. Maximum 16. Do not ship fewer than 12.

### Naming Arc Rules

- Sparse zone: single evocative word — `Ember`, `Hollow`, `Spine`
- Building zone: two words, physical + texture — `Cracked Shell`, `Iron Root`
- Full zone: two words, room or place — `Basement Floor`, `Stone Garden`
- Hybrid zone: material collision — `Wire Cactus`, `Glass Soil`
- FX zone: process or state — `Signal Rot`, `Loop Ghost`, `Scatter`

### Q-Link Layout

| Q1 | Q2 | Q3 | Q4 |
|----|----|----|-----|
| ATTACK | DECAY | TUNE | MUTE |

TUNE maps to global pitch offset or per-voice root. MUTE should mute the most prominent
mid-frequency voice (snare or clap) to enable loop layering.

### Velocity Strategy

- **4 layers per pad**: soft / medium-soft / medium-hard / hard
- Layer boundaries: 0–30 / 31–70 / 71–100 / 101–127
- Timbre shift required between layers (not just amplitude) — D001 compliance
- Recommended: filter brightness opens with velocity; transient sharpness increases

### feliX-Oscar Distribution

| feliX (bright, cutting) | Center (balanced) | Oscar (dark, heavy) |
|------------------------|------------------|-------------------|
| 40% | 20% | 40% |

Even feliX/Oscar split reflects the dual nature of percussion — attack brightness (feliX)
and body weight (Oscar). Center programs serve as kit transitions.

### Adaptation Notes

- ONSET: use `perc_` parameter prefix; velocity layers map to built-in layer system
- OBLONG (perc): use `bob_` prefix; assign voice channels to pad layout manually
- OSTINATO (concept): when built, use `ost_` prefix; communal voice assignment TBD

---

## T2 — The Voice Portrait

**Intent**: Instrument character packs — single instruments rendered across processing stages.
Arc moves from raw/dry to fully transformed XO_OX character. Expression via CC, not velocity.

**Target engines**: OBLONG, OBBLIGATO, OHM, OTTONI, OLE

### Program Table

| Slot | Zone | Character |
|------|------|-----------|
| 1–2 | Dry | Instrument as-is, no processing |
| 3–4 | Touched | Light EQ, subtle room, character emerging |
| 5–7 | Processed | Filter color, modulation, identity forming |
| 8–10 | Transformed | Full XO_OX treatment, far from source |
| 11–12 | Extreme | Edge state, near abstraction or dual coupling |

Minimum 8 programs (drop slots 11–12 for lean packs). Maximum 12.

### Naming Arc Rules

- Dry zone: instrument name + material — `Reed Air`, `Brass Open`
- Touched zone: quality + instrument — `Warm Reed`, `Bright Brass`
- Processed zone: character descriptor — `Swollen Reed`, `Ancient Brass`
- Transformed zone: metaphor or creature — `Reed Tide`, `Brass Becoming`
- Extreme zone: single abstract word — `Molt`, `Breach`, `Dissolve`

### Q-Link Layout

| Q1 | Q2 | Q3 | Q4 |
|----|----|----|-----|
| CUTOFF | RES | ATTACK | SPACE |

SPACE maps to reverb send or hall size. Aftertouch should be wired to CUTOFF modulation
depth on all programs (D006 compliance).

### Velocity Strategy

- **Single velocity layer** — one sample or synth voice across full velocity range
- Velocity → amplitude only (not timbre) at the XPN layer
- Timbre expression delivered via aftertouch CC (D006) and mod wheel (D002)
- This keeps sample budgets lean and pack size manageable for tonal instruments

### feliX-Oscar Distribution

| feliX (bright, cutting) | Center (balanced) | Oscar (dark, heavy) |
|------------------------|------------------|-------------------|
| 30% | 40% | 30% |

Center majority reflects instrumental neutrality — voice packs serve producers who bring
their own tonal context. feliX/Oscar minority programs provide pre-committed character options.

### Adaptation Notes

- OBBLIGATO: `obbl_breathA` / `obbl_breathB` route naturally to Q1/Q2; BOND macro
  captures the two-voice coupling unique to this engine
- OHM: `ohm_macroMeddling` maps to Q4 (SPACE equivalent); COMMUNE axis adds
  a community-warmth dimension not present in other voice engines
- OTTONI: `otto_macroGrow` creates natural arc from muted to full brass — sequence
  programs along GROW macro position
- OLE: `ole_macroDrama` defines the arc; DRAMA low = introspective, DRAMA high = performance

---

## T3 — The Bass Architecture

**Intent**: Bass-forward packs built in three floors — foundation, character, dirt. Sub integrity
is the primary value; distortion and color are additive layers.

**Target engines**: OBESE, ORCA, OCEANIC (bass frequency range)

### Program Table

| Slot | Zone | Character |
|------|------|-----------|
| 1–3 | Sub Foundation | Clean sub, maximum low-end integrity |
| 4–6 | Mid Character | Body and presence, filter color engaged |
| 7–8 | Dirty | Saturation, drive, grit — controlled |
| 9–10 | Distorted | Pushed hard, industrial or aggressive |

Minimum 8 programs. Maximum 10.

### Naming Arc Rules

- Sub zone: material weight — `Iron Sub`, `Black Mud`, `Deep Concrete`
- Mid zone: texture + source — `Resin Growl`, `Cork Drive`, `Rubber Mouth`
- Dirty zone: process collision — `Tar Resonance`, `Acid Brick`
- Distorted zone: destruction metaphor — `Split Column`, `Sludge Break`

### Q-Link Layout

| Q1 | Q2 | Q3 | Q4 |
|----|----|----|-----|
| DRIVE | CUTOFF | ATTACK | WIDTH |

WIDTH maps to stereo spread or chorus width. Bass programs should ship with WIDTH at
a conservative default (narrow) — producers widen to taste.

### Velocity Strategy

- **2 velocity layers**: soft (0–63) / hard (64–127)
- Hard layer: more transient attack, brighter filter opening
- Soft layer: pure sub, rounded, no transient edge
- Keep layer count to 2 — bass instruments benefit from simplicity; extra layers add
  CPU without proportional sonic gain in low registers

### feliX-Oscar Distribution

| feliX (bright, cutting) | Center (balanced) | Oscar (dark, heavy) |
|------------------------|------------------|-------------------|
| 20% | 20% | 60% |

Oscar dominant — bass identity is gravitational. feliX programs provide contrast and
brightness for genres that need cutting bass (electro, grime, UK bass).

### Adaptation Notes

- OBESE: `fat_satDrive` is the natural Q1 DRIVE target; Mojo Control (B015) adds
  analog/digital axis that creates program variation beyond filter color alone
- ORCA: `orca_huntMacro` as DRIVE; echolocation system creates mid-bass transient
  artifacts that differentiate ORCA bass packs from standard sub packs
- OCEANIC: phosphorescent teal character suits mid-bass zone programs; `ocean_separation`
  expands stereo field naturally for WIDTH assignment

---

## T4 — The Atmospheric Journey

**Intent**: Textural and ambient packs with long sample times and slow development.
Arc structure mirrors a narrative: opening, expanding, peak, and closing.

**Target engines**: ORACLE, ORIGAMI, OBSCURA, OPAL

### Program Table

| Slot | Zone | Character |
|------|------|-----------|
| 1 | Opener | Sparse, brief — single texture, quiet |
| 2–3 | Expanding | More motion, second layer introduced |
| 4–5 | Building | Density increasing, modulation visible |
| 6 | Peak | Densest point — maximum texture, full motion |
| 7 | Returning | Density falling, motion slowing |
| 8 | Closer | Sparse again — echo of opener, not identical |

Minimum 6 programs (drop Returning). Maximum 8. Do not ship more than 8 —
atmospheric packs lose identity with too many programs.

### Naming Arc Rules

- Opener: time of day or climate state — `Pre-Dawn`, `Still Water`, `First Fog`
- Expanding: natural movement — `Tide Rising`, `Wind Finding`, `Spore Drift`
- Building: ecological or geological — `Canopy Pressure`, `Plate Shift`
- Peak: event word — `Confluence`, `Eruption`, `Full Bloom`
- Returning: post-event — `After Rain`, `Low Tide`, `Cooling`
- Closer: resolution without repetition — `Residue`, `Salt Flat`, `Long Echo`

### Q-Link Layout

| Q1 | Q2 | Q3 | Q4 |
|----|----|----|-----|
| MOVEMENT | SPACE | DEPTH | FEEDBACK |

MOVEMENT maps to LFO rate or grain speed. DEPTH maps to reverb pre-delay or wet depth.
FEEDBACK maps to delay feedback or loop density. All four should produce audible change
from minimum to maximum — D004 compliance critical here.

### Velocity Strategy

- **Single layer** — long atmospheric samples do not benefit from velocity layering
- Velocity → amplitude only
- Samples should be 4–30 seconds in length; loop points required for held notes
- Crossfade loop points mandatory (no audible seam)

### feliX-Oscar Distribution

| feliX (bright, cutting) | Center (balanced) | Oscar (dark, heavy) |
|------------------------|------------------|-------------------|
| 25% | 50% | 25% |

Center majority — atmospheric packs serve as tonal environments for other instruments.
Heavy feliX or Oscar commitment narrows producer use cases. Balanced center is the correct
default for ambient and scoring work.

### Adaptation Notes

- ORACLE: GENDY stochastic synthesis (B010) means MOVEMENT control is particularly
  expressive; breakpoint system creates program variation without additional samples
- ORIGAMI: `origami_foldPoint` naturally maps to DEPTH; fold geometry creates
  textural variation distinct from other atmospheric engines
- OBSCURA: daguerreotype character suits Opener and Closer zones naturally;
  `obscura_stiffness` maps well to FEEDBACK for decaying texture programs
- OPAL: grain-based engine — loop points not required; grain size and position replace
  sample start/end considerations; `opal_grainSize` maps to MOVEMENT

---

## T5 — The Character Machine

**Intent**: Personality-forward packs for engines with strong DSP identity. Arc moves from
character introduction through full expression to extreme states and resolution.

**Target engines**: OVERDUB, ODYSSEY, OUROBOROS, OVERBITE

### Program Table

| Slot | Zone | Character |
|------|------|-----------|
| 1–2 | Introduction | Engine character emerging, restrained |
| 3–5 | Establishing | Character confirmed, macros engaged |
| 6–8 | Full Character | Peak personality, all engine features active |
| 9–11 | Edge | Pushed to extremes, near instability |
| 12–13 | Resolution | Character returning, quieter, transformed |
| 14 | Outro | Single summary program — character distilled |

Minimum 10 programs (drop Edge to 2 programs, Outro optional). Maximum 14.

### Naming Arc Rules

- Introduction: character trait — `First Instinct`, `Quiet Arrival`
- Establishing: character in action — `Settling Into`, `Pattern Found`
- Full Character: declarative — `Full Possession`, `Peak Identity`, `The Machine Wakes`
- Edge: pressure language — `Under Strain`, `Held Too Long`, `At The Limit`
- Resolution: transformation — `After The Edge`, `New Pattern`, `Changed`
- Outro: archetype — `The Machine`, `The Character`, `The Instrument`

### Q-Link Layout

| Q1 | Q2 | Q3 | Q4 |
|----|----|----|-----|
| CHARACTER | MOVEMENT | COUPLING | SPACE |

This is the standard XOmnibus 4-macro layout. All four must be wired to meaningful DSP
per-engine — verified by doctrine check before shipping. CHARACTER should be the most
distinctive parameter for that engine's identity.

### Velocity Strategy

- **4 velocity layers**: soft / medium-soft / medium-hard / hard
- Layer boundaries: 0–30 / 31–70 / 71–100 / 101–127
- CHARACTER macro value should increase with velocity across layers — louder playing
  opens the engine's distinctive feature further
- This creates performative depth: quiet playing = restrained character, loud = full

### feliX-Oscar Distribution

| feliX (bright, cutting) | Center (balanced) | Oscar (dark, heavy) |
|------------------------|------------------|-------------------|
| 30% | 40% | 30% |

Balanced with center majority — character engines serve diverse production contexts.
Edge zone programs can skew feliX (bright distortion) or Oscar (dark weight) to
create contrast without forcing the full pack to commit.

### Adaptation Notes

- OVERDUB: Spring Reverb (B004) is the signature feature; Tape Delay creates natural
  MOVEMENT expression; `dub_sendAmount` maps to COUPLING for send-return architecture
- ODYSSEY: `drift_oscA_mode` wavetable position maps to CHARACTER; era-crossfade
  creates MOVEMENT dimension unique to this engine
- OUROBOROS: Leash Mechanism (B003) maps to CHARACTER — tight leash = Introduction,
  loose = Edge; Velocity Coupling Outputs (B007) add live-performance depth to COUPLING
- OVERBITE: Five-Macro System (B008) requires mapping BELLY/BITE/SCURRY/TRASH/PLAY DEAD
  to the 4 Q-Links — BITE maps to CHARACTER, SCURRY to MOVEMENT; PLAY DEAD is the
  resolution zone's signature state

---

## T6 — The MPCe Quad Pack

**Intent**: Native MPCe format — each program is a spatial quad. Four corner pads define
a feliX/Oscar × dry/wet matrix. XY touch surface morphs between corners. Applicable to
all 34 engines; no engine-specific adaptation required.

**Target engines**: All 34 registered engines

### Program Table

| Slot | Zone | Character |
|------|------|-----------|
| 1–2 | feliX Quads | NW/NE corner pair dominates — bright, cutting |
| 3–4 | Balanced Quads | All four corners distinct, equal weight |
| 5–6 | Oscar Quads | SW/SE corner pair dominates — dark, heavy |
| 7–8 | Fusion Quads | Corners reference coupling pairs across engines |

Minimum 4 programs. Maximum 8.

### Corner Assignment (Fixed Convention)

| Corner | Character |
|--------|-----------|
| NW | feliX / dry |
| NE | feliX / wet |
| SW | Oscar / dry |
| SE | Oscar / wet |

This assignment is non-negotiable across all T6 packs. Consistency is the product value —
producers learn one corner language and apply it across every T6 pack in the catalog.

### Naming Arc Rules

- Each program named with the engine identity word + quad type:
  - `[Engine Short Name] feliX Quad`, `[Engine] Balance Quad`, `[Engine] Oscar Quad`
  - For Fusion quads: `[EngineA] × [EngineB]` — e.g., `OHM × OVERDUB Fusion`

### Q-Link Layout

| Q1 | Q2 | Q3 | Q4 |
|----|----|----|-----|
| X-MORPH | Y-MORPH | DEPTH | SPACE |

X-MORPH maps to the horizontal axis (feliX ↔ Oscar). Y-MORPH maps to the vertical axis
(dry ↔ wet). DEPTH and SPACE are universal atmospheric controls applicable to any engine.

The MPCe's native XY pad surface should be the primary performance interface.
Q-Links serve secondary precision control.

### Velocity Strategy

- **No velocity layers** — Z-axis (pad pressure on MPCe) handles dynamics
- Velocity → amplitude only at the XPN layer
- All dynamic expression is reserved for Z-axis and XY position
- This keeps quad programs focused on spatial morphing, not layer management

### feliX-Oscar Distribution

| feliX (bright, cutting) | Center (balanced) | Oscar (dark, heavy) |
|------------------------|------------------|-------------------|
| Defined by corner, not program | — | — |

Distribution is spatial rather than per-program. Within each program, all four corners
must be genuinely distinct — no two corners should sound similar. If the NW and NE
corners are sonically indistinguishable, the wet signal is not wet enough.

### Fusion Quad Notes

Fusion quads (slots 7–8) source corners from two different engines:
- NW = Engine A feliX dry
- NE = Engine B feliX wet
- SW = Engine A Oscar dry
- SE = Engine B Oscar wet

Use established coupling pairs where possible — reference `Skills/coupling-interaction-cookbook/`
for compatible engine pairs. Fusion quads are the highest-complexity T6 programs and should
be produced last after single-engine quads are approved.

### Adaptation Notes

- OPTIC: visual engine — OPTIC does not produce audio. T6 is not applicable.
- ONSET: drum engine — corner assignment maps to drum bus character, not pitched notes.
  NW = feliX kit (snappy, bright), NE = feliX FX kit, SW = Oscar kit (heavy, deep),
  SE = Oscar FX kit.
- All other 32 engines: standard corner assignment applies without modification.

---

## Cross-Template Rules

These rules apply to all six templates without exception.

### Sample Depth

| Template | Sample Depth |
|----------|-------------|
| T1 Drum Foundation | 4 velocity layers × up to 4 round-robin samples per layer = 16 samples per pad |
| T2 Voice Portrait | 1 layer × 1–2 round-robins = 1–2 samples per pad |
| T3 Bass Architecture | 2 velocity layers × 1 round-robin = 2 samples per pad |
| T4 Atmospheric Journey | 1 layer × 1 loop = 1 sample per pad |
| T5 Character Machine | 4 velocity layers × 1 sample = 4 samples per pad |
| T6 MPCe Quad | 4 corners × 1 sample = 4 samples per program |

### Naming Constraints (All Templates)

- Maximum 30 characters per program name
- No jargon, no parameter names, no engine technical terms
- Names must evoke sensation, place, material, or event
- No duplicate names within a single pack
- No duplicate names across packs in the same engine family

### Q-Link Consistency Rule

Q-Link assignments within a single pack must be identical across all programs. A producer
who learns Q1 = ATTACK on program 3 must find Q1 = ATTACK on program 11.
Variation in Q-Link targets between programs is a quality failure.

### feliX-Oscar Balance Check

Before shipping, tally the feliX/Oscar/center distribution and verify it matches the
template target within ±5%. A pack that drifts more than 5% from its template target
should be reviewed — either justify the drift or rebalance programs.

### Export Validation

All templates must pass XPN export validation before release:
- `KeyTrack = True` on all samples
- `RootNote = 0` on all samples
- Empty layer `VelStart = 0` on all unfilled velocity slots
- Reference: `Tools/` XPN export pipeline; `Skills/xpn-export-specialist/`

---

## Template Selection Guide

Use this table to select the right template for a new pack:

| If the pack features... | Use template |
|------------------------|-------------|
| Drums, percussion, rhythmic kits | T1 |
| A single acoustic instrument rendered across processing stages | T2 |
| Bass — sub, mid, or dirty | T3 |
| Textures, atmospheres, ambient soundscapes | T4 |
| A strong-personality engine at full expression | T5 |
| Any engine in MPCe spatial format | T6 |
| Multiple engines in one pack | T5 or T6 (prefer T6 for cross-engine packs) |

---

*End of spec. See `Tools/` for export pipeline. See `Skills/xpn-export-specialist/` for validation.*
