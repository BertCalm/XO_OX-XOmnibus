# XPN MPCe-Native Expansion Pack Design Guide
**R&D Spec — XO_OX Internal**
Date: 2026-03-16
Status: Active Reference

---

## Purpose

This document defines how XO_OX designs expansion packs specifically for the MPC Live III's 3D pad surface (MPCe). Standard XPN packs treat pads as velocity-sensitive buttons. MPCe-native packs treat each pad as a full performance surface — four sonic positions, continuous XY morphing, and pressure-driven modulation. The result is a fundamentally different instrument per pad.

---

## 1. MPCe Architecture Review

The MPC Live III's 3D pads track three continuous axes simultaneously while a pad is held:

- **Z-axis (pressure)**: Aftertouch-style depth measurement from initial strike through sustained hold. Ranges 0–127. Updates continuously.
- **X-axis (left-right)**: Horizontal position across the pad surface. Center = 64, left edge = 0, right edge = 127.
- **Y-axis (bottom-top)**: Vertical position. Bottom = 0, top = 127. Center = 64.

Each pad also carries **four corner samples** assigned to the cardinal positions: NW, NE, SW, SE. The MPC engine interpolates between all four corners based on the live XY position, blending continuously as a finger moves across the surface. Z-axis pressure is routed separately as a modulation source (volume, pitch, filter, or combinations).

This creates a 5-dimensional performance surface per pad: strike velocity, XY position at onset, XY position during hold, and sustained Z pressure. A single pad can contain an entire sonic spectrum if the corners are designed to carry it.

---

## 2. The XO_OX Quad-Corner Strategy (Canonical)

Every MPCe-native pack defaults to this corner architecture unless a specific alternate is chosen (see Section 3). The layout maps directly to the feliX-Oscar polarity axis:

| Corner | Identity | Character |
|--------|----------|-----------|
| **NW** | feliX / dry | Bright, clinical, unprocessed signal |
| **NE** | feliX / wet | Bright, clinical, with reverb and/or delay |
| **SW** | Oscar / dry | Warm, organic, unprocessed signal |
| **SE** | Oscar / wet | Warm, organic, with full effects chain |

Playing diagonally from NW to SE sweeps through the complete feliX-Oscar spectrum while simultaneously moving from dry to wet. Horizontal movement (NW→NE or SW→SE) controls wet/dry without changing character. Vertical movement (NW→SW or NE→SE) morphs feliX to Oscar without changing effects depth.

This architecture is canonical because it maps directly to the XO_OX brand polarity and teaches the listener what feliX and Oscar mean through physical gesture — no manual required.

---

## 3. Alternative Corner Architectures

Use these for variety across packs, or when the sound design concept demands it. One alternative architecture per pack is sufficient — mixing architectures within a pack creates orientation confusion.

**Era Corners** — for drum machine character packs:
- NW = 808 character, NE = 909 character, SW = LinnDrum character, SE = TR-8S character
- Horizontal axis sweeps era, vertical axis sweeps generation (analog → digital)

**Dynamic Corners** — for expressive melodic instruments:
- NW = staccato, NE = accent, SW = legato, SE = tremolo
- Pressure (Z) adds a fifth dimension: breath or bow intensity

**Coupling Corners** — for XOlokun engine-derived packs:
- NW = solo/isolated, NE = light coupling (two engines), SW = heavy coupling, SE = full coupling
- Horizontal axis controls isolation-to-entanglement; vertical controls coupling density

**Pitch Corners** — for harmonic instruments where interval content is the design:
- NW = root, NE = perfect fifth, SW = octave below, SE = octave above
- Each pad becomes a chordal or intervallic gesture tool; pressure adds vibrato or bite

---

## 4. Z-Axis (Pressure) Assignments by Program Type

Z-axis behavior must be intentional and consistent within a program type. Never leave pressure unassigned.

- **Drum kits**: Pressure maps to filter brightness. Low pressure (light touch, sustained) keeps the transient dark and controlled. High pressure (pressing in after strike) opens the filter, adding aggression and presence. Range: filter cutoff ±30%, no pitch modulation.

- **Bass programs**: Pressure drives pitch vibrato amount. No vibrato at rest; a deliberate press introduces natural-sounding pitch shimmer. Vibrato rate: 4–6 Hz fixed. Range: ±20 cents maximum.

- **Pad / atmosphere programs**: Pressure controls reverb send or sustained feedback level. Light touch keeps the sound intimate. Sustained pressure opens the tail, allowing the sound to bloom into its wet corner state organically.

- **Lead / melodic programs**: Pressure adds distortion or overdrive amount. This matches physical intuition — pressing harder makes the instrument louder and dirtier, like bowing or blowing harder. Range: 0–40% drive increase. No pitch modulation on leads; Z and pitch bend should not compete.

---

## 5. MPCE_SETUP.md Template

Every MPCe-native pack must include an `MPCE_SETUP.md` file at the pack root. This file is the player's orientation document.

```
# [Pack Name] — MPCe Setup Guide

## Pad Corner Layout

Each pad on this pack is arranged as follows:

    NW (feliX/dry) -------- NE (feliX/wet)
         |                       |
         |     XY morphs here    |
         |                       |
    SW (Oscar/dry) -------- SE (Oscar/wet)

Move your finger across the surface to blend between corners.
Pressure (pressing in after striking) controls [Z-axis behavior for this pack].

## Suggested Techniques

- **Surface sweep**: Strike and drag slowly across the full pad surface.
- **Diagonal drag**: Strike NW, drag to SE for maximum feliX-to-Oscar wet transformation.
- **Pressure build**: Hold a note and gradually increase pressure to [describe effect].
- **Corner snap**: Strike close to a corner for clean isolated sounds without morphing.

## Compatibility

Designed for MPC Live III (firmware 3.x+). On MPC X, Z-axis pressure is available
but XY morphing behavior may differ — test before performance.
```

Each program in the pack should reference its specific Z-axis behavior in the setup file. If all programs share the same behavior, one section at the top is sufficient.

---

## 6. Design Constraints

- **Corner distinctness**: DNA distance of ≥ 0.3 between any two corners (use the XO_OX sonic DNA scoring rubric). Adjacent corners that blend too smoothly undermine the architecture.
- **Corner standalone viability**: Every corner sample must function as a complete, usable sound in isolation — not only as a blend ingredient. Test each corner solo before finalizing.
- **Z-axis modulation ceiling**: Never exceed ±50 cents pitch deviation. Never exceed ±40% filter cutoff shift. These limits prevent the Z-axis from producing sounds that contradict the XY corner design.
- **MPCE_SETUP.md is mandatory**: One file per pack minimum. Programs with unique Z-axis behaviors get their own section.

---

## 7. Pack Naming Convention

- **Program name**: Use the sound name only — no corner suffixes. The corners are invisible infrastructure, not exposed labels. Example: `Deep Brass`, not `Deep Brass NW-feliX`.
- **Pad labels**: Use the 6-character pad label field to orient the player. Label format: `NW/DRY`, `NE/WET`, `SW/ORG`, `SE/FUL`. These appear on the MPC screen during performance and serve as a quick orientation map.
- **Pack filename suffix**: Append `_mpce` to the XPN bundle filename. Example: `ONSET_Ensemble_Kit_mpce.xpn`. This distinguishes MPCe-native packs from standard velocity-layer packs in the library browser.

---

*End of spec. Questions → Joshua or the XPN Tools channel.*
