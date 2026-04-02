# Aquatic Master FX Suite — Concept Brief

**Date:** March 2026
**Phase:** 0 — Concept approved
**Status:** Design spec, pending DSP implementation

---

## Identity

- **Name:** The Aquarium (Aquatic Master FX Suite)
- **Thesis:** Six master effects themed around water phenomena — the sonic environment that every XOceanus engine passes through. The aquatic mythology that connects feliX (neon tetra) and Oscar (axolotl) to the entire platform becomes *audible* through the master bus.
- **Purpose:** Brand-defining master FX chain that makes all XOceanus output sound cohesive — "that sounds like XO_OX" regardless of which engine is active.

---

## The Six Effects

### 1. Fathom — Hydrostatic Pressure
- **Phenomenon:** The increasing pressure of deep water
- **DSP:** Multi-band compression + low-shelf warmth + high-shelf attenuation
- **Character:** The deeper you push the mix knob, the more "submerged" — highs attenuate, lows gain mass, transients soften. Like sinking.
- **Parameters:**
  - `aqua_fathomDepth` — 0.0 (surface) to 1.0 (abyssal) — controls compression ratio + shelf amounts
  - `aqua_fathomPressure` — transient softening amount (0.0–1.0)
  - `aqua_fathomMix` — dry/wet (0.0–1.0)
- **Oscar's FX:** Depth, patience, enclosed warmth

### 2. Drift — Ocean Currents
- **Phenomenon:** Unpredictable movement of water
- **DSP:** Stereo chorus/ensemble with Brownian (random walk) LFO, not sine
- **Character:** Sound drifts like kelp in a current. Never the same path twice. Subtle at low mix, swaying and hypnotic at high.
- **Parameters:**
  - `aqua_driftRate` — base speed of random walk (0.01–5.0 Hz equivalent)
  - `aqua_driftWidth` — stereo spread of the drift (0.0–1.0)
  - `aqua_driftDepth` — modulation depth / how far sound travels from center (0.0–1.0)
  - `aqua_driftMix` — dry/wet (0.0–1.0)
- **Oscar's FX:** Wandering, organic, never repeating
- **Note:** Distinct from Odyssey's "Voyager Drift" trait — this is a master bus effect, not per-engine

### 3. Tide — Tidal Rhythm
- **Phenomenon:** Lunar tidal cycles — the inhale/exhale of the ocean
- **DSP:** Tremolo/auto-filter with configurable LFO (tempo-sync or free-run)
- **Character:** Slow, breathing modulation. Can sync to BPM or free-run at geological speeds. The "inhale/exhale" of the mix.
- **Parameters:**
  - `aqua_tideRate` — LFO rate (free: 0.01–4.0 Hz, sync: 1/1–1/32)
  - `aqua_tideShape` — LFO shape: sine, triangle, or "lunar" (asymmetric rise/fall)
  - `aqua_tideTarget` — modulation target: amplitude, filter cutoff, or both
  - `aqua_tideMix` — dry/wet (0.0–1.0)
- **feliX's FX:** Rhythmic, precise, cyclical

### 4. Reef — Enclosed Underwater Space
- **Phenomenon:** Sound bouncing inside a coral structure
- **DSP:** Early-reflection reverb with LP-filtered diffusion (water absorbs highs)
- **Character:** Short, dense, colorful — sound bouncing inside a coral maze. Filtered tails simulate water's frequency absorption.
- **Parameters:**
  - `aqua_reefSize` — virtual space size (0.0 small crevice – 1.0 cathedral cave)
  - `aqua_reefDamping` — high-frequency absorption (0.0 bright – 1.0 deep water)
  - `aqua_reefDensity` — early reflection density (0.0 sparse – 1.0 thick)
  - `aqua_reefMix` — dry/wet (0.0–1.0)
- **Oscar's FX:** Enclosed, safe, warm resonance

### 5. Surface — The Air-Water Boundary
- **Phenomenon:** The transition between submerged and emerged
- **DSP:** Dynamic high-shelf filter + transient shaper. Single knob sweeps between two worlds.
- **Character:** Below = muffled warmth (LP-filtered, soft transients). Above = bright air (HP shelf boost, crispy transients). The tension between two worlds — feliX's electric surface and Oscar's deep patience.
- **Parameters:**
  - `aqua_surfaceLevel` — -1.0 (deep submerged) through 0.0 (at surface) to +1.0 (bright air)
  - `aqua_surfaceTension` — how sharp the transition is (0.0 gradual – 1.0 abrupt)
  - `aqua_surfaceMix` — dry/wet (0.0–1.0)
- **feliX's FX:** The boundary, the flash of neon breaking the surface
- **Key coupling concept:** feliX lives near the surface (bright, darting). Oscar lives in the depths (patient, regenerative). Surface is where they meet.

### 6. Biolume — Bioluminescence
- **Phenomenon:** Deep-sea creatures generating their own light
- **DSP:** Spectral exciter + harmonic shimmer (even or odd harmonic emphasis + high-shelf saturation)
- **Character:** Adds luminous overtones that weren't in the original signal — like deep-sea creatures generating their own light. Subtle at low mix, phosphorescent at high.
- **Parameters:**
  - `aqua_biolumeGlow` — exciter intensity (0.0–1.0)
  - `aqua_biolumeSpectrum` — harmonic emphasis: even (warm glow) to odd (cold shimmer) (0.0–1.0)
  - `aqua_biolumeDecay` — how long the shimmer rings after transients (0.0–1.0)
  - `aqua_biolumeMix` — dry/wet (0.0–1.0)
- **feliX's FX:** Neon brightness, the tetra's luminous stripe

---

## Architecture

### Signal Flow
```
Engine Output → [Fathom] → [Drift] → [Tide] → [Reef] → [Surface] → [Biolume] → Master Out
```

Each FX is independently bypassable. Order is fixed by design:
1. **Fathom** first — compression/depth shapes the dynamic foundation
2. **Drift** — chorus/modulation operates on the compressed signal
3. **Tide** — rhythmic modulation rides on top of the drift
4. **Reef** — reverb captures the modulated signal
5. **Surface** — frequency shaping determines the final tonal character
6. **Biolume** — exciter adds luminance as the final touch

### Pairing Logic

The 6 FX form 3 conceptual pairs:

| Pair | Axis | FX A | FX B |
|------|------|------|------|
| **Depth** | Pressure ↔ Air | Fathom (sinks) | Surface (emerges) |
| **Motion** | Chaos ↔ Rhythm | Drift (random) | Tide (cyclical) |
| **Color** | Space ↔ Light | Reef (shapes space) | Biolume (adds glow) |

### feliX vs Oscar Mapping

| FX | Character | Why |
|----|-----------|-----|
| Fathom | Oscar | Depth, pressure, patience — axolotl territory |
| Drift | Oscar | Organic wandering, never the same — regenerative |
| Tide | feliX | Rhythmic precision, cyclical — schooling behavior |
| Reef | Oscar | Enclosed warmth, safe resonance — the cave |
| Surface | feliX | The boundary, brightness, electric emergence |
| Biolume | feliX | Neon glow, luminous harmonics — the tetra's stripe |

---

## Macro Integration

The Aquatic FX should respond to XOceanus macros:

- **CHARACTER macro** → Modulates Fathom depth + Biolume glow (more character = deeper + brighter)
- **SPACE macro** → Modulates Reef size + Drift width (more space = larger reverb + wider chorus)
- **MOVEMENT macro** → Modulates Tide rate + Drift rate (more movement = faster modulation)
- **COUPLING macro** → Modulates Surface level (coupling pushes toward the boundary where engines meet)

---

## Implementation Notes

- All 6 FX share the `aqua_` parameter prefix
- Total parameter count: ~22 (3-4 per FX)
- DSP complexity is moderate — no convolution, all algorithmic
- Fathom uses multi-band split (3 bands: sub, mid, presence)
- Drift's Brownian LFO: `next = current + randn() * step_size`, clamped to ±1
- Reef's early reflections: Householder feedback delay network (FDN), 4-8 taps
- Surface's filter: Cytomic SVF high-shelf, sweepable
- Biolume's exciter: half-wave rectification + HP filter + saturation + blend

---

## Brand Site Integration

The Aquatic FX suite becomes a section on XO-OX.org between "The Platform" and "Open Source":
- Interactive visualization: 6 FX as depth layers in a vertical cross-section of water
- Each FX card shows its phenomenon, character, and which XO character (feliX/Oscar) it belongs to
- Hover reveals parameter names and descriptions
- Optional: interactive audio demo where visitors can toggle FX on/off over a playing engine

---

## Blog / Field Guide Post

**"The Aquarium — Six Ways to Drown Your Sound"**
- History of water as metaphor in electronic music
- Drexciya's Afrofuturist underwater mythology
- Brian Eno's ambient water recordings
- Aphex Twin's *Selected Ambient Works Volume II*
- Walkthrough of each FX with sound design recipes
- The feliX/Oscar mythology made audible
