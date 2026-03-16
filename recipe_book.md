# XO_OX Preset Recipe Book

> A practical guide to recreating signature XO_OX sounds. Each recipe specifies exact parameter values, macro directions, and sound design principles so you can learn the engines from the inside out.

*Generated: 2026-03-16 — 3 recipes*

---

## Table of Contents

- [OPAL](#opal) — 3 recipes

---

## OPAL

### Frozen Shimmer
**Engine:** OPAL  
**Mood:** Aether  
**Tags:** granular, freeze, shimmer, ambient, pad  
**Coupling:** Light

A granular freeze effect where incoming audio crystallizes into an endless shimmer. Grain position is static, spray is tight, and pitch randomization adds a delicate sparkle without destabilizing the pitch.

**Parameters — `Opal`**

| Parameter | Value |
|-----------|-------|
| `opal_grainSize` | `0.85` |
| `opal_grainDensity` | `0.92` |
| `opal_grainPosition` | `0.5` |
| `opal_grainSpray` | `0.08` |
| `opal_pitchRandom` | `0.12` |
| `opal_pitchShift` | `0.5` |
| `opal_freeze` | `1.0` |
| `opal_space` | `0.82` |
| `opal_masterVol` | `0.72` |

**Macro Guide**

| Macro | Direction | What It Does |
|-------|-----------|--------------|
| GRAIN | high | Large grains sustain the frozen texture — pull down for crunchier ice. |
| SCATTER | minimal | Keep low to maintain the frozen illusion; increase for shimmer movement. |
| SPACE | high | Wide reverb is the backbone of this sound — do not pull below 60%. |

**Sound Design Notes**

Set opal_freeze to 1.0 before anything else — this is the defining parameter. Then sculpt with grain size (shimmer vs. crystalline) and spray (tight vs. diffuse). Coupling OPAL → DUB adds a subtle tape warmth that prevents the freeze from feeling sterile.

---

### Granular Fog
**Engine:** OPAL  
**Mood:** Atmosphere  
**Tags:** granular, fog, texture, noise, atmospheric  
**Coupling:** None

Dense overlapping micro-grains with high spray create an amorphous, evolving fog of sound. Pitch tracking is loose — this is texture, not tone.

**Parameters — `Opal`**

| Parameter | Value |
|-----------|-------|
| `opal_grainSize` | `0.22` |
| `opal_grainDensity` | `0.98` |
| `opal_grainPosition` | `0.35` |
| `opal_grainSpray` | `0.78` |
| `opal_pitchRandom` | `0.55` |
| `opal_pitchShift` | `0.5` |
| `opal_freeze` | `0.0` |
| `opal_space` | `0.65` |
| `opal_masterVol` | `0.65` |

**Macro Guide**

| Macro | Direction | What It Does |
|-------|-----------|--------------|
| SCATTER | high | The core of the fog — maximum spray breaks melodic identity. |
| DRIFT | slow sweep | Slowly evolving grain position keeps the fog alive and non-repetitive. |
| GRAIN | down | Small grain sizes are essential — larger grains start forming recognizable phrases. |

**Sound Design Notes**

The fog lives in the collision between high density and high spray. Any reduction in density (below 0.80) will create audible gaps that break the illusion. Use automation on opal_grainPosition over 8–16 bars for organic movement.

---

### Glass Melt
**Engine:** OPAL  
**Mood:** Prism  
**Tags:** granular, glass, melt, pitched, ethereal  
**Coupling:** None

Pitched granular synthesis where medium-size grains overlap to create the sensation of glass slowly melting. Slight pitch randomization and a long, glassy reverb tail complete the effect.

**Parameters — `Opal`**

| Parameter | Value |
|-----------|-------|
| `opal_grainSize` | `0.55` |
| `opal_grainDensity` | `0.75` |
| `opal_grainPosition` | `0.62` |
| `opal_grainSpray` | `0.3` |
| `opal_pitchRandom` | `0.22` |
| `opal_pitchShift` | `0.5` |
| `opal_freeze` | `0.0` |
| `opal_space` | `0.78` |
| `opal_masterVol` | `0.68` |

**Macro Guide**

| Macro | Direction | What It Does |
|-------|-----------|--------------|
| GRAIN | mid | Medium grain size is critical — too small = noise, too large = chords. |
| SPACE | high | A long, bright reverb IS the glass surface. |
| DRIFT | gentle | Slow position drift makes the melt feel continuous. |

**Sound Design Notes**

The melt illusion depends on the overlap between grains — opal_grainDensity must stay above 0.65. Use opal_pitchRandom between 0.15–0.30: lower is glassy and pure, higher introduces a blurred, melting quality. Feed this into a plate reverb with a 4–6 second tail for full effect.

---
