# XOlokun Performance Mapping — Meta-Controller Design

**Scope:** XOlokun-level hardware control — engine selection, coupling routing, and per-engine macro pass-through
**Coupling System:** MegaCouplingMatrix (15 types incl. KnotTopology + TriangularCoupling)
**Engine Slots:** 4 simultaneous engines (A, B, C, D)

---

## Overview

XOlokun is a multi-engine synthesizer. The hardware performance mapping operates at two levels:

1. **Engine Level** — per-engine parameters (see individual engine mapping files)
2. **XOlokun Level** — engine selection, coupling amount, coupling type, per-engine volume/mute, macro pass-through

This document covers Level 2 exclusively. For Level 1, see:
- `obrix-hardware-mapping.md`
- `onset-hardware-mapping.md`
- `opera-hardware-mapping.md`

---

## Core Concepts

### 4 Engine Slots
XOlokun runs 4 engine slots simultaneously: **Slot A**, **B**, **C**, **D**.
Each slot can hold any of the 73 registered engines.

### Coupling Architecture
Coupling connects pairs of engine slots:
- **A→B route:** Engine A output modulates Engine B
- **B→A route:** Engine B output modulates Engine A
- Both routes are simultaneously active (asymmetric is possible)
- 15 coupling types available (see type table below)
- Each route has: **Amount** (0.0–1.0) + **Type** (0–14) + **Bypass** toggle

### Macro Pass-Through
Each engine exposes 4 macros (CHARACTER/MOVEMENT/COUPLING/SPACE).
XOlokun's performance layer can override or blend with per-engine macro values.
When a XOlokun-level macro is engaged, it broadcasts to the active engine's macro.

---

## Coupling Types Reference

| Type # | Name | Description |
|--------|------|-------------|
| 0 | Amplitude | Simple audio-level modulation |
| 1 | FM | Frequency modulation (A carrier → B oscillator) |
| 2 | Filter | A audio drives B filter cutoff |
| 3 | Envelope | A envelope gates/shapes B amplitude |
| 4 | Ring | Ring modulation between A and B |
| 5 | Sync | Hard sync B oscillator to A fundamental |
| 6 | Chord | A note triggers harmonically-related B note |
| 7 | Phase | A phase state modulates B Kuramoto field |
| 8 | Rhythm | A transient peaks trigger B envelope retriggering |
| 9 | Pitch | A pitch CV drives B pitch transposition |
| 10 | Spectral | A spectral centroid modulates B filter/timbre |
| 11 | AmpToChoke | A amplitude chokes B (muting) |
| 12 | PitchToFilter | A pitch modulates B cutoff in Hz |
| 13 | KnotTopology | Topological routing matrix (ORBWEAVE-style) |
| 14 | TriangularCoupling | Three-engine love triangle dynamics (OXYTOCIN-style) |

---

## Push 2/3 — XOlokun Meta Layer

### Philosophy
Push's User mode gives full 8×8 pad matrix + 8 knobs per row at XOlokun level. The top two rows of pads are the engine/coupling command surface. The lower 6 rows remain for note playing (auto-routed to the active engine's preferred mode).

### XOlokun Command Surface (Top 2 Pad Rows)

```
ROW 8 (top) — Engine Selection + Status
┌────────┬────────┬────────┬────────┬────────┬────────┬────────┬────────┐
│ ENG A  │ ENG B  │ ENG C  │ ENG D  │  MUT A │  MUT B │  MUT C │  MUT D │
│[select]│[select]│[select]│[select]│ [mute] │ [mute] │ [mute] │ [mute] │
│Gold lit│dim=idle│        │        │red=mute│        │        │        │
└────────┴────────┴────────┴────────┴────────┴────────┴────────┴────────┘

ROW 7 — Coupling Controls
┌────────┬────────┬────────┬────────┬────────┬────────┬────────┬────────┐
│ A→B    │ A→B    │  B→A   │ B→A    │ COUP   │ COUP   │ BYPASS │ BYPASS │
│  TYPE  │  AMT   │  TYPE  │  AMT   │ A→C    │ C→A    │ A→B    │ B→A    │
│ cycle  │ [hold] │ cycle  │ [hold] │ toggle │ toggle │ toggle │ toggle │
└────────┴────────┴────────┴────────┴────────┴────────┴────────┴────────┘
```

### XOlokun Knob Layer (8 knobs at top)

**XOlokun Knob Row — Meta Performance**
```
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│  Eng A   │  Eng B   │  Eng C   │  Eng D   │ Coup A→B │ Coup B→A │ Coup A→C │ Master   │
│  Volume  │  Volume  │  Volume  │  Volume  │  Amount  │  Amount  │  Amount  │  Volume  │
│  0→1     │  0→1     │  0→1     │  0→1     │  0→1     │  0→1     │  0→1     │  0→1     │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
```

**Secondary XOlokun Knob Row** (accessed via XOlokun Page 2):
```
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│  Eng A   │  Eng A   │  Eng A   │  Eng A   │  Eng B   │  Eng B   │  Eng B   │  Eng B   │
│CHARACTER │MOVEMENT  │ COUPLING │  SPACE   │CHARACTER │MOVEMENT  │ COUPLING │  SPACE   │
│  macro   │  macro   │  macro   │  macro   │  macro   │  macro   │  macro   │  macro   │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
```

(Page 3 would show Engines C and D macros similarly)

### Playing Surface (Rows 1–6, 6×8 = 48 pads)

In XOlokun multi-engine mode, the pad surface routes to the **active engine** (selected via Row 8 pad):
- If Slot A is active (lit): chromatic or drum mode of Engine A
- If multiple engines are selected: note-on sends to all selected slots simultaneously

**Engine-specific pad modes (auto-set on engine selection):**
- OBRIX: Chromatic (4ths layout)
- ONSET: Drum Rack (8 voices mapped to 8 pads)
- OPERA: Chromatic (4ths layout)

### Coupling Type Scrolling on Push

Coupling type cycling (Pad Row 7, pads 1 and 3) shows type name on Push display:
```
A→B TYPE: [← PREV]  [Ring Mod]  [NEXT →]
```
Types cycle through: Amplitude → FM → Filter → Envelope → Ring → Sync → Chord → Phase → Rhythm → Pitch → Spectral → AmpToChoke → PitchToFilter → KnotTopology → TriangularCoupling → (loop)

---

## Maschine — XOlokun Meta Layer

### Philosophy
Maschine's 4 Group buttons (A–D) map directly to XOlokun's 4 engine slots. This is an intuitive 1:1 mapping — Group A = Engine Slot A, etc. The 16 pads beneath each active Group play notes in the active engine's mode. Global Smart Strips handle coupling.

### Group → Engine Slot Mapping

| Maschine Group | XOlokun Slot | Default Engine |
|---------------|-------------|----------------|
| Group A | Slot A | OBRIX |
| Group B | Slot B | ONSET |
| Group C | Slot C | OPERA |
| Group D | Slot D | (any 4th engine) |

### Smart Strips — XOlokun Level

| Strip | Assignment | Notes |
|-------|------------|-------|
| Strip 1 | Coupling A→B Amount | Primary coupling; most-used |
| Strip 2 | Coupling B→A Amount | Reverse coupling |
| Strip 3 | Engine A Volume | Slot A level |
| Strip 4 | Engine B Volume | Slot B level |
| Strip 5 | Engine C Volume | Slot C level |
| Strip 6 | Engine D Volume | Slot D level |
| Strip 7 | Coupling A→B Type | Step through 15 types |
| Strip 8 | Master Volume | Global output |

### Knob Pages — XOlokun Level

**XOlokun Page 1 — Coupling Matrix**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| A→B Amt | A→B Type | B→A Amt | B→A Type | A→C Amt | C→A Amt | B→C Amt | C→B Amt |

**XOlokun Page 2 — Engine Volumes + Mutes**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| Eng A Vol | Eng B Vol | Eng C Vol | Eng D Vol | Mute A | Mute B | Mute C | Mute D |

**XOlokun Page 3 — Engine A Macros**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| A: CHAR | A: MOVE | A: COUP | A: SPACE | B: CHAR | B: MOVE | B: COUP | B: SPACE |

**XOlokun Page 4 — Engine C + D Macros**
| K1 | K2 | K3 | K4 | K5 | K6 | K7 | K8 |
|----|----|----|----|----|----|----|-----|
| C: CHAR | C: MOVE | C: COUP | C: SPACE | D: CHAR | D: MOVE | D: COUP | D: SPACE |

### Pad Surface — Per-Group Engine Interaction

When Group A is selected: 16 pads play Engine A (OBRIX) in chromatic mode
When Group B is selected: 16 pads play Engine B (ONSET) as drum kit (8 voices)
When Group C is selected: 16 pads play Engine C (OPERA) in chromatic mode

**Hold a Group button** to access that engine's settings in the Knob Pages.

**Simultaneous triggering:** With Maschine's Chord or Multi-Group mode, you can trigger notes in multiple engines simultaneously, letting coupling generate cross-engine interactions in real time.

---

## Generic MIDI CC Map — XOlokun Level

### Design Notes
- These CCs operate at the XOlokun layer — above individual engine CCs
- Engine-level CCs (see per-engine maps) use CC01–CC31 for each engine
- XOlokun-level CCs use CC80–CC119 to avoid conflicts
- If your controller is limited to CC01–CC127, use channels 1–4 for engines A–D respectively, and reserve channel 16 for XOlokun-level control

| CC# | Assignment | Name | Range Notes |
|-----|-----------|------|-------------|
| **Engine Volume + Mute** | | | |
| CC80 | Engine A Volume | Slot A Level | 0=silent, 127=full |
| CC81 | Engine B Volume | Slot B Level | 0=silent, 127=full |
| CC82 | Engine C Volume | Slot C Level | 0=silent, 127=full |
| CC83 | Engine D Volume | Slot D Level | 0=silent, 127=full |
| CC84 | Engine A Mute | Slot A Mute | 0=active, 127=muted |
| CC85 | Engine B Mute | Slot B Mute | 0=active, 127=muted |
| CC86 | Engine C Mute | Slot C Mute | 0=active, 127=muted |
| CC87 | Engine D Mute | Slot D Mute | 0=active, 127=muted |
| **Coupling Amounts** | | | |
| CC88 | Coupling A→B Amount | A sends to B | 0=off, 127=full |
| CC89 | Coupling B→A Amount | B sends to A | 0=off, 127=full |
| CC90 | Coupling A→C Amount | A sends to C | 0=off, 127=full |
| CC91 | Coupling C→A Amount | C sends to A | 0=off, 127=full |
| CC92 | Coupling B→C Amount | B sends to C | 0=off, 127=full |
| CC93 | Coupling C→B Amount | C sends to B | 0=off, 127=full |
| CC94 | Coupling A→D Amount | A sends to D | 0=off, 127=full |
| CC95 | Coupling D→A Amount | D sends to A | 0=off, 127=full |
| **Coupling Types** | | | |
| CC96 | Coupling A→B Type | Route A→B type | 0–8=types 0–8, 9–14=mapped |
| CC97 | Coupling B→A Type | Route B→A type | 0–14 step |
| CC98 | Coupling A→C Type | Route A→C type | 0–14 step |
| CC99 | Coupling B→C Type | Route B→C type | 0–14 step |
| **Coupling Bypass** | | | |
| CC100 | Bypass A→B | Toggle coupling | 0=active, 127=bypass |
| CC101 | Bypass B→A | Toggle coupling | 0=active, 127=bypass |
| CC102 | Bypass A→C | Toggle coupling | 0=active, 127=bypass |
| CC103 | Bypass B→C | Toggle coupling | 0=active, 127=bypass |
| **Engine Selection (Program Change recommended instead)** | | | |
| CC104 | Active Engine Select | Front engine for knob focus | 0–31=A, 32–63=B, 64–95=C, 96–127=D |
| **Macro Pass-Through (broadcasts to active engine)** | | | |
| CC105 | Active Engine CHARACTER | Macro 1 broadcast | 0–127 |
| CC106 | Active Engine MOVEMENT | Macro 2 broadcast | 0–127 |
| CC107 | Active Engine COUPLING | Macro 3 broadcast | 0–127 |
| CC108 | Active Engine SPACE | Macro 4 broadcast | 0–127 |
| **Master** | | | |
| CC119 | Master Volume | Global output | 0=silent, 127=full |

### Program Change → Engine Selection

Use Program Change (Ch16) to hot-swap engines into slots:
- PC 0–72: Load engine by index (see engine table in CLAUDE.md)
- Slot targeted by CC104 selection prior to PC message
- 50ms crossfade prevents clicks on hot-swap (see architecture rules)

---

## Recommended Physical Controller Setups

### Setup 1: Push 2/3 (Primary Controller)

```
Push surface:
├── Row 8 pads: Engine slot selection + mute
├── Row 7 pads: Coupling type cycle + bypass
├── Top 8 knobs: Engine volumes + coupling amounts
├── Bottom 8 knobs: Active engine Tier 1 parameters
└── Rows 1–6: Note playing surface (chromatic or drum rack)

Physical layout suggestion:
- Hold Row 8 pad = open that engine's param page on display
- Long-press Row 7 pad = open coupling settings display
```

### Setup 2: Maschine + Push (Dual Controller)

```
Maschine:
├── Groups A–D: Engine slot focus
├── Smart Strips: Coupling amounts (always accessible)
├── 16 pads: Note playing for focused engine
└── Knob pages: Deep parameter editing

Push (slaved as MIDI controller):
├── 8 top knobs: XOlokun-level meta controls
└── Rows 1–8: Performance note surface
```

### Setup 3: Generic Controller (Endless Encoders + Pads)

Minimum recommended: 8 encoders + 16 pads

```
Encoders 1–4: Engine A–D volumes (CC80–83)
Encoders 5–6: Coupling A→B + B→A amounts (CC88–89)
Encoder 7: Coupling type A→B (CC96, step)
Encoder 8: Master volume (CC119)

Pads 1–4: Engine A–D select (CC104)
Pads 5–8: Engine A–D mute (CC84–87)
Pads 9–12: Coupling bypass A→B, B→A, A→C, B→C (CC100–103)
Pads 13–16: Coupling type preset recall (PC 0–3 quick slots)
```

---

## Coupling Recipe Guide

### OBRIX + ONSET (Flagship V1 Pairing)

**Setup:** Slot A = OBRIX, Slot B = ONSET
**Coupling A→B:** Type = Rhythm (8) — OBRIX's long sustained notes create envelopes for ONSET drums
**Coupling B→A:** Type = Filter (2) — ONSET kick peaks drive OBRIX Proc1 Cutoff

**OBRIX settings for best coupling:**
- Set `obrix_reefResident` = Competitor or Symbiote
- Keep `obrix_macroCoupling` at 0.4–0.7

**ONSET settings:**
- Coupling modifies `couplingBlendMod` (blend) and `couplingFilterMod` (tone)
- Raise `perc_xvc_global_amount` to hear XVC interaction alongside coupling

**Ergonomic note:** With this pairing, the left hand (sustained notes on OBRIX) shapes what the right hand (drum triggers on ONSET) sounds like in real time.

### OPERA + OBRIX (Melodic Duo)

**Setup:** Slot A = OPERA, Slot B = OBRIX
**Coupling A→B:** Type = Phase (7) — OPERA Kuramoto order parameter drives OBRIX Harmonic Field
**Coupling B→A:** Type = FM (1) — OBRIX Saw source modulates OPERA partial frequencies

**Best for:** Vocal-textured leads over an evolving harmonic backdrop.

When OPERA Drama is high (locked partials), the coupling pushes OBRIX toward JI harmony. When OPERA is chaotic (low Drama), OBRIX gets detuned by the FM coupling.

### ONSET + OPERA (Rhythm + Voice)

**Setup:** Slot A = ONSET, Slot B = OPERA
**Coupling A→B:** Type = Amplitude (0) — drum hits gate OPERA amplitude (rhythmic vocal stabs)
**Coupling B→A:** Type = Spectral (10) — OPERA spectral centroid modulates ONSET master tone

**Amount suggestion:** A→B at 0.6, B→A at 0.3 (voice decorates drums more subtly)

**Interesting effect:** When OPERA is in high-Drama (locked), the coupling makes ONSET hit harder with each rhythmic trigger. As Drama falls (chaos), the coupling weakens and the drums return to their natural character.

### All Three + TriangularCoupling (Type 14)

**Setup:** Slots A/B/C = OBRIX/ONSET/OPERA
**Coupling:** Enable Type 14 (TriangularCoupling) on route A→B
- This activates the three-engine love triangle dynamics (OXYTOCIN-style)
- Each engine influences the other two asymmetrically based on output level
- The engine with the highest output "dominates" and suppresses the others slightly

**Warning:** TriangularCoupling is CPU-intensive. Monitor CPU meter.

---

## XOlokun Performance Philosophy

The goal of the XOlokun performance layer is to make complex cross-engine interactions feel like one instrument, not three separate synths routed through a mixer. When the coupling amounts are balanced and the types are well-chosen, moving a single coupling knob can fundamentally transform the relationship between engines — from independent voices to deeply entangled organisms.

The best XOlokun performances treat the coupling layer as a musical instrument in itself:
- Low coupling = independent engines, each with their own character
- Medium coupling = symbiotic — each engine flavors the others
- High coupling = entangled — the engines lose individual identity and become a single, breathing system

The "sweet spot" for live performance is usually coupling amounts between 0.3 and 0.6. Below 0.3, coupling is subtle (use for decoration). Above 0.7, the interaction becomes dominant and can override preset character (use for climax moments).
