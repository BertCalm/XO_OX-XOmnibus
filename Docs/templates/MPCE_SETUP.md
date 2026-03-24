# XO_OX MPCe Percussion Vol. 1 — MPCe Setup Guide

**Designed for:** MPC Live III / MPC XL (firmware 3.7+)
**Standard version included** for MPC Live II, One, Key 61, and Force.

---

## What Makes This Pack Different

This pack was designed from the ground up for MPCe 3D pads. Every drum pad has **four articulation corners** — strike near a corner to get that articulation. Slide your finger across the pad surface to morph between them in real time.

Standard MPC pads: 1 sound per pad. This pack: 4 sounds per pad, continuously morphable.

---

## Pad Corner Layout

Each of the 16 pads uses the **Dynamic Expression** corner architecture:

```
    NW: Ghost Note          NE: Accent / Rimshot
      (quiet, muted,          (alternate strike,
       brush-adjacent)         bright transient)
    ┌────────────────────┬────────────────────┐
    │                    │                    │
    │   whisper          │          bite      │
    │                    │                    │
    ├────────────────────┼────────────────────┤
    │                    │                    │
    │   standard         │          texture   │
    │                    │                    │
    └────────────────────┴────────────────────┘
    SW: Standard Hit        SE: Effect / Flam
      (center strike,         (processed,
       the default voice)      textured variant)
```

**How to play:**
- **Strike near a corner** to get that articulation cleanly
- **Strike in the center** to blend all four corners
- **Slide from corner to corner** while holding to morph the sound
- The physical edge of the rubber pad IS the corner — aim for the edge

---

## XY Modulation Targets

While your finger is on the pad, its position continuously controls:

| Axis | Controls | Feel |
|------|----------|------|
| **X** (horizontal) | Reverb send | Left = dry, Right = spacious |
| **Y** (vertical) | Decay time | Bottom = tight/choked, Top = open/long |
| **Z** (pressure) | Filter cutoff | Light = dark/muffled, Heavy = bright/open |

---

## Pad Map

### Row A (Core Kit)
| Pad | Instrument | NW (Ghost) | NE (Accent) | SW (Standard) | SE (Effect) |
|-----|-----------|------------|-------------|---------------|-------------|
| A01 | Kick | Sub thump | Beater click | Full kick | Layered boom |
| A02 | Snare | Ghost tap | Rimshot | Center hit | Buzz roll |
| A03 | Closed Hat | Tick | Pedal splash | Standard close | Shaker |
| A04 | Open Hat | Half open | Crash bleed | Full open | Sizzle |

### Row A (Color + Texture)
| Pad | Instrument | NW (Ghost) | NE (Accent) | SW (Standard) | SE (Effect) |
|-----|-----------|------------|-------------|---------------|-------------|
| A05 | Clap | Finger snap | Double clap | Single clap | Reverb clap |
| A06 | Low Tom | Muted | Rim | Open tom | Floor sweep |
| A07 | Rimshot | Side stick | Cross stick | Full rim | Flam |
| A08 | Crash | Choke | Bell | Full crash | Reversed |
| A09 | Ride | Tip | Bell | Bow | Swell |
| A10 | Mid Tom | Muted | Rim | Open | Roll |
| A11 | High Tom | Muted | Rim | Open | Ruff |
| A12 | Floor Tom | Muted | Rim | Open | Thunder |

### Row A (Atmosphere)
| Pad | Instrument | NW (Ghost) | NE (Accent) | SW (Standard) | SE (Effect) |
|-----|-----------|------------|-------------|---------------|-------------|
| A13 | Tambourine | Single hit | Shake | Roll | Filtered |
| A14 | Cowbell | Muted | Open | Standard | Pitched |
| A15 | Clave | Soft | Hard | Standard | Delayed |
| A16 | Wood Block | Low | High | Standard | Resonant |

---

## Suggested Techniques

### Surface Sweep
Strike A02 (snare) at the SW corner, then slowly drag to NE. You'll hear the snare morph from a standard center hit into a bright rimshot. This is a single-take performance gesture that would normally require two separate pads.

### Diagonal Morph
Strike near NW (ghost), drag diagonally to SE (effect). Maximum timbral transformation in one gesture. Great for building intensity within a single fill.

### Pressure Build
Strike any pad lightly, then gradually increase pressure while holding. The filter opens progressively — dark to bright under your finger. Combined with the corner position, this gives you a 3D timbral space on every pad.

### Corner Snap
For clean, isolated articulations: strike firmly near the physical edge of the pad. The closer to the corner, the purer the articulation. Center strikes blend all four.

---

## Standard Version (Non-MPCe)

If you're on MPC Live II, One, Key 61, or Force, the **Standard** program is included:

- `XOOX_MPCe_Perc_v1_Standard.xpm` — one sample per pad, velocity-layered (4 layers)
- Same GM pad layout, same voice assignments
- Corner articulations are mapped as velocity layers instead:
  - Ghost (vel 1-20) | Light (vel 21-50) | Medium (vel 51-90) | Hard (vel 91-127)

---

## About XO_OX Designs

XO_OX Designs makes instruments that think differently about sound. Our XOlokun platform
houses 44 synthesis engines that can couple, collide, and mutate into sounds impossible
with any single synth.

This is the first pack in our **MPCe Exclusive** series — content designed from the ground
up for the spatial capabilities of MPC Live III and MPC XL.

More at [xo-ox.org](https://xo-ox.org)
