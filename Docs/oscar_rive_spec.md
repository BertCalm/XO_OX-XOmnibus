# Oscar — Rive State Machine Specification

*For the artist building `oscar.riv` in the Rive editor.*
*Version 1.0 | XO_OX Designs | March 2026*

---

## Overview

Oscar is an axolotl. Pink gills breathing slowly in a coral cave. He is the warm, living heart of the OddOscar synthesizer — always breathing, always watching, evolving across five stages of life.

The `.riv` file is the canonical Oscar. One file, one character, event-driven. The C++ code pushes values; the state machine decides what Oscar does.

---

## Artboard

| Property | Value |
|---|---|
| Name | `Oscar` |
| Size | 200 × 200 px |
| Background | Transparent |
| Origin | Centred |

The character is displayed at any size from **24 px** (header indicator) to **200 px** (main panel). At small sizes, hide detail layers. At large sizes, show full body.

---

## Character Shape Inventory

Oscar is exactly **15 vector shapes**. No more. This keeps the `.riv` file under 50 KB and the CPU cost trivial.

| # | Name | Description | Layer |
|---|---|---|---|
| 1 | `body` | Main pink blob, rounded rectangle with soft corners | Base |
| 2 | `belly` | Lighter pink ellipse, slightly off-centre, gives 3D volume | Base |
| 3 | `eye_left` | Circle, dark fill | Face |
| 4 | `eye_right` | Circle, dark fill | Face |
| 5 | `pupil_left` | Smaller circle, near-black | Face |
| 6 | `pupil_right` | Smaller circle, near-black | Face |
| 7 | `smile` | Subtle arc, neutral expression by default | Face |
| 8 | `gill_1` | Feathery bezier stalk, left cluster | Gills |
| 9 | `gill_2` | Feathery bezier stalk, left cluster | Gills |
| 10 | `gill_3` | Feathery bezier stalk, left cluster | Gills |
| 11 | `gill_4` | Feathery bezier stalk, right cluster | Gills |
| 12 | `gill_5` | Feathery bezier stalk, right cluster | Gills |
| 13 | `gill_6` | Feathery bezier stalk, right cluster | Gills |
| 14 | `leg_left` | Stubby rounded limb (hidden at < 64 px render size) | Body |
| 15 | `leg_right` | Stubby rounded limb (hidden at < 64 px render size) | Body |

### Scale-based visibility (use Rive constraints or a number input `renderSize`)
- `leg_left` and `leg_right`: visible when `renderSize > 64`
- `belly`: always visible
- All gill detail (feathery branching): always visible

---

## Color Palette

| Role | Hex | Usage |
|---|---|---|
| Axolotl Gill Pink | `#E8839B` | `body`, gill stalks |
| Soft Belly | `#F2B3C1` | `belly` |
| Deep Pink | `#C45E7A` | Outline/shadow strokes |
| Near-black | `#1A1A2E` | Pupils |
| Warm dark | `#3D2B3D` | Eyes |
| Coral highlight | `#FFD6E0` | Gill tips at high excitation |

Body color should shift subtly based on morph position (see state machine inputs below). At morph=0 (sine, at rest): pure `#E8839B`. At morph=3 (noise, dissolved): desaturate to `#C4A8B0` with 80% opacity.

---

## State Machine

**Name in file:** `OscarBehavior`

### Inputs

These are the exact names the C++ code uses. Any typo breaks the integration.

| Name | Type | Range | Description |
|---|---|---|---|
| `gillSpeed` | Number | 0.0 – 1.0 | Gill oscillation rate. 0.12 = resting breath. 1.0 = full excited flutter. |
| `morphPosition` | Number | 0.0 – 3.0 | Wavetable position. Drives body colour desaturation and slight transparency. |
| `evolutionLevel` | Number | 0 – 5 | Oscar's age/stage. Drives blend state morphing. |
| `levelPct` | Number | 0.0 – 1.0 | Output amplitude. Drives subtle body swell on loud chords. |
| `noteOn` | Trigger | — | Fires on every new note. Brief blink + gill flutter spike. |
| `celebration` | Trigger | — | Fires on level clear. Full 3-second celebration animation. |
| `bossMode` | Boolean | true/false | Held true during boss phase. Oscar goes alert, pupils dilate. |

### States

| State | Entry Condition | Description |
|---|---|---|
| `IDLE` | Default / no notes | Slow gill oscillation at `gillSpeed`. Eyes soft. Body slight sway. |
| `ACTIVE` | `noteOn` trigger | Gill flutter spikes for 0.5s then smooths to `gillSpeed`. Pupils widen. |
| `ALERT` | `bossMode = true` | Eyes widen. Gill speed locked to 0.8. Body tenses (slight scale-down). |
| `CELEBRATION` | `celebration` trigger | Full body wiggle. Gills fan outward. Eyes happy-squint. 3 seconds. Returns to IDLE. |
| `EVOLUTION` | `evolutionLevel` changes | 3-second morph between age stages. Oscar shifts shape, grows slightly. Returns to previous state. |

### State Transitions

```
IDLE ──(noteOn)──────────────► ACTIVE
IDLE ──(bossMode=true)────────► ALERT
IDLE ──(celebration)──────────► CELEBRATION ──(3s)──► IDLE
IDLE ──(evolutionLevel change)► EVOLUTION ───(3s)──► IDLE

ACTIVE ──(no voice, 2s)───────► IDLE
ACTIVE ──(bossMode=true)──────► ALERT
ACTIVE ──(celebration)────────► CELEBRATION ──(3s)──► IDLE

ALERT ──(bossMode=false)──────► IDLE
ALERT ──(celebration)─────────► CELEBRATION ──(3s)──► IDLE

EVOLUTION can be interrupted by CELEBRATION.
EVOLUTION cannot be interrupted by noteOn (too noisy).
```

---

## Gill Animation — The Soul

The gills are the single most expressive element. They must feel alive at all times.

**Anatomy:** 6 stalks (`gill_1` through `gill_6`), arranged in clusters of 3 on each side of the head. Each stalk has a feathery tip — 3-4 small branches at the top of each bezier stalk.

**Base oscillation:**
Each gill oscillates independently with a slightly different phase offset and rate multiplier:

| Gill | Phase offset | Rate multiplier |
|---|---|---|
| gill_1 | 0.0 | 1.00 |
| gill_2 | 0.4 rad | 0.87 |
| gill_3 | 1.1 rad | 1.13 |
| gill_4 | 0.2 rad | 0.95 |
| gill_5 | 0.8 rad | 1.07 |
| gill_6 | 1.5 rad | 0.91 |

The multiplier and phase offsets ensure no two gills move in sync — this is what makes Oscar feel organic rather than mechanical.

**Oscillation motion:** Each gill stalk rotates ±12° at idle (`gillSpeed=0.12`). At `gillSpeed=1.0`, rotation increases to ±28° with faster frequency. The gill tips trail the stalk with a 2-frame lag (gives fluid, jellyfish-like softness).

**On `noteOn`:** Gill tips briefly fan outward (extra ±15°) for 4 frames, then settle back. Like a startled but not frightened creature.

**On `celebration`:** All 6 gills fan full outward simultaneously, hold for 8 frames, then oscillate in a wider range (±35°) for the celebration duration.

**On `bossMode`:** Gill rate locked to `0.8` regardless of `gillSpeed` input. Stalks hold slightly tighter (±20° max). Oscar is wary.

---

## Evolution Blend States

Oscar evolves across 6 stages. Each stage has distinct visual characteristics. The transition between stages is a 3-second path interpolation (Rive blend state).

| Level | Stage | Size | Key Visual |
|---|---|---|---|
| 0 | Hatchling | 80% | Small, large eyes relative to body, 4 tiny gill stalks |
| 1 | Juvenile | 88% | Eyes proportionate, 6 gill stalks but short |
| 2 | Adolescent | 94% | Full gill length, legs appear |
| 3 | Adult | 100% | Canonical Oscar, full presence |
| 4 | Ancient | 103% | Gills longer, slightly translucent body, deep pink |
| 5 | Elder | 106% | Gills very long and fan-like, body near-translucent, luminescent gill tips |

**Transition:** Use Rive's blend state with `evolutionLevel` as the blend input. Set the animation to interpolate vertex positions (not just scale) so the gills grow organically — stalks lengthen, not just scale up.

Elder gill tips should emit a soft glow (`#FFD6E0`, Additive blend, 40% opacity pulsing at 0.5 Hz).

---

## Body Swell (`levelPct`)

On loud chords, Oscar's body expands slightly:
- `body` scale: `1.0 + levelPct * 0.04` (max 4% expansion)
- `belly` scale: `1.0 + levelPct * 0.06` (slightly more, gives breathing feel)
- Smoothed with a 60ms attack, 200ms release to avoid jitter

This is subtle. It should feel like a breath, not a pump.

---

## Morph Body Tint (`morphPosition`)

As `morphPosition` increases from 0 to 3, Oscar's body gradually desaturates and becomes slightly translucent:

| morphPosition | Body opacity | Saturation |
|---|---|---|
| 0.0 | 100% | 100% — vivid coral pink |
| 1.0 | 97% | 85% — slightly muted |
| 2.0 | 90% | 65% — washed, reef-like |
| 3.0 | 80% | 40% — near-transparent, dissolved into reef |

At morph=3.0 Oscar is still visible but ghostly — he has "become the reef."

---

## Reduced Motion Mode

When the host sets reduced motion (WCAG 2.3.3), Oscar should:
- Stop all gill oscillation
- Hold a gentle static pose (gills slightly fanned, neutral expression)
- Still respond to state changes (EVOLUTION still happens, but instantly — no 3s transition)
- Still show `celebration` (but as a 0.5s brief glow, not full animation)

The C++ timer drops from 60 Hz to 10 Hz in reduced motion mode. The Rive advance call still happens; just less frequently.

---

## File Requirements

- Format: `.riv` (Rive runtime format, not `.rev`)
- Target runtime: rive-cpp 2.x (C++ runtime, MIT licensed)
- Max file size: 50 KB
- Artboard name: `Oscar` (exact)
- State machine name: `OscarBehavior` (exact)
- All input names must match the table above exactly (case-sensitive)

---

## What NOT to include

- No background (transparent artboard)
- No shadow/glow effects baked in (the JUCE component handles any drop shadow)
- No text or labels
- No more than 15 shapes (keep the file tiny and the renderer fast)
- No bitmap assets — all vector

---

*XO_OX Designs | oscar.riv | The canonical axolotl*
