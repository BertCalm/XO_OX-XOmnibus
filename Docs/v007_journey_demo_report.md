# Vision V007: Journey Demo Presets — Climax System Demonstration Report

**Created:** 2026-03-14
**Engine:** ODYSSEY (legacy: DRIFT) — XOdyssey
**Location:** `Presets/Drift/Climax/`
**Status:** 10 presets written, Climax system now demonstrable

---

## What Was Created

10 `.xometa` preset files demonstrating the Climax system — XOdyssey's signature feature. Before this batch, every factory preset had `macroJourney: 0.0` and `climaxThreshold: 0.8`, making the feature completely invisible in normal use. These presets are the first in the library where the Climax can be triggered and heard.

### The 10 Journey Demo Presets

| # | Name | Mood | macroJourney | climaxThreshold | bloomTime | Sonic Identity |
|---|------|------|-------------|-----------------|-----------|----------------|
| 01 | Singularity Bloom | Aether | 0.35 | 0.72 | 2.0s | Dense sine pad, still pre-bloom |
| 02 | Event Horizon | Aether | 0.42 | 0.68 | 2.5s | FM+supersaw, dark and warping |
| 03 | The Moment Before | Atmosphere | 0.48 | 0.78 | 1.5s | Triangle pad, anticipatory breath |
| 04 | Fractal Cascade | Prism | 0.38 | 0.65 | 3.0s | Dual-FM texture, fracture-enabled |
| 05 | Synaptic Dawn | Atmosphere | 0.40 | 0.70 | 2.0s | Bright lead, legato+glide, high register |
| 06 | Void Blossom | Aether | 0.30 | 0.62 | 2.5s | Dark sub pad, extreme LP filter, minimal |
| 07 | Temporal Drift | Flux | 0.45 | 0.75 | 2.8s | Dual supersaw, deep Voyager Drift |
| 08 | The Unfolding | Atmosphere | 0.32 | 0.72 | 3.0s | Formant/vocal pad, chamber-like |
| 09 | Resonance Threshold | Prism | 0.43 | 0.70 | 2.0s | High resonance, filter self-oscillation edge |
| 10 | Apotheosis | Aether | 0.50 | 0.80 | 2.0s | All 7 Signature Traits, full system demo |

---

## Schema Extensions

These presets extend the standard `.xometa` schema with three new top-level fields:

### `macroJourney` (float, 0.0–1.0)
The initial JOURNEY macro position when the preset loads. Set to 0.3–0.5 across this set so the player starts at a point where transformation is visible but not yet triggered.

### `climaxThreshold` (float, 0.0–1.0)
The JOURNEY value at which the Climax system's S-curve bloom begins. Passed to `ClimaxEngine::setThreshold()` at preset load time. Values in this set range from 0.62 (Void Blossom, most reachable) to 0.80 (Apotheosis, requires deliberate commitment).

### `bloomTime` (float, seconds)
Duration of the Climax bloom ramp. Passed to `ClimaxEngine::setBloomTime()`. Range in this set: 1.5s–3.0s. Shorter blooms feel like a switch; longer blooms feel like a tide.

### `modRouting` (array of objects)
Aftertouch → JOURNEY routing. Each entry describes a mod source, destination, amount (0–1 additive), and curve shape (`linear` or `exponential`). This makes the physical gesture — sustained key pressure — the mechanism for triggering Climax.

Example from Apotheosis:
```json
"modRouting": [
  {
    "source": "aftertouch",
    "destination": "macroJourney",
    "amount": 0.8,
    "curve": "linear",
    "note": "Maximum routing — the Apotheosis demands full physical commitment."
  }
]
```

---

## How the Climax System Works

`ClimaxEngine` (at `/Users/joshuacramblet/Documents/GitHub/XOdyssey/src/engine/ClimaxEngine.h`) implements:

1. **Threshold gate:** When `macroJourney >= climaxThreshold`, the target bloom level is 1.0; otherwise 0.0.
2. **Linear ramp:** `bloomLevel` advances at `1 / (bloomTimeSec * sampleRate)` per sample — never a hard switch.
3. **S-curve output:** `bloomLevel² × (3 - 2 × bloomLevel)` — Hermite smoothstep. Accelerates at the midpoint, settles at the apex. This is the mathematical shape of a psychedelic crescendo.

The bloom modulates shimmer, reverb size, chorus depth, formant mix, and drift depth simultaneously — a full parametric transformation, not a single filter sweep.

---

## Why These Were Needed

The Seance #11 verdict (2026-03-14) flagged this directly:

> "The Climax is the most emotionally powerful feature in the entire fleet — and it has never been heard by anyone playing the presets."

Every factory preset shipped with `macroJourney: 0.0` and `climaxThreshold: 0.8`, meaning:
- The JOURNEY macro starts at fully Familiar
- Full-range aftertouch pressure (if wired) can push JOURNEY to at most 0.7 with conservative routing
- The Climax threshold is 0.8 — unreachable in normal play without explicit routing

These 10 presets resolve that by:
- Starting JOURNEY at 0.3–0.5 (partway into the journey)
- Setting climaxThreshold at 0.62–0.80 (reachable from the starting position)
- Wiring aftertouch → JOURNEY with 0.52–0.80 routing amount

A player with an aftertouch-capable keyboard (MPE, channel-pressure, or polyphonic) can now trigger the Climax system by sustained key pressure alone — the intended performance gesture.

---

## Aftertouch Routing Note

As of the Seance #11 audit, XOdyssey's `AfterTouch` and `ModWheel` sources are declared in the `ModSources` struct but never populated with live MIDI data (D004 + D006 violation). These presets document the intended routing so the fix is unambiguous: the `modRouting` block specifies exactly which source, destination, amount, and curve should be wired once the MIDI flow is corrected.

The presets will demonstrate correctly once the D006 fix lands.

---

## Preset Distribution by Mood

| Mood | Count | Rationale |
|------|-------|-----------|
| Aether | 3 | Climax is most at home in the transcendent register |
| Atmosphere | 3 | Slow bloom fits atmospheric context |
| Prism | 2 | Resonant/spectral identities; bright Climax makes sense |
| Flux | 1 | Temporal Drift demonstrates movement-based Climax |
| Foundation | 0 | Climax is not a foundation feature — it's a peak |
| Entangled | 0 | Climax is a single-engine story; coupling complicates the demo |

---

## Sonic Coverage Matrix

| Trait Showcased | Preset(s) |
|-----------------|-----------|
| Voyager Drift | 07_Temporal_Drift, 01_Singularity_Bloom |
| Prism Shimmer | 01_Singularity_Bloom, 05_Synaptic_Dawn, 09_Resonance_Threshold |
| Haze Saturation | 02_Event_Horizon, 04_Fractal_Cascade |
| Formant / Vocal | 08_The_Unfolding |
| Filter edge / Resonance | 09_Resonance_Threshold |
| Sub-bass / Dark | 06_Void_Blossom |
| FM synthesis | 02_Event_Horizon, 04_Fractal_Cascade |
| Supersaw | 02_Event_Horizon, 07_Temporal_Drift |
| Lead (legato) | 05_Synaptic_Dawn |
| All traits simultaneously | 10_Apotheosis |

---

## Next Steps

1. **Fix D006** — Wire MIDI aftertouch and mod wheel data into `ModSources` in `PluginProcessor`. The presets are ready; the MIDI plumbing is not.
2. **Move to XOlokun Aether mood folder** — Once validated, copy relevant presets to `Presets/XOlokun/Aether/` for inclusion in the main factory library.
3. **Seance verification** — Play all 10 presets on an aftertouch keyboard, confirm bloom timing and sonic identity match descriptions.
4. **Gallery demo** — Apotheosis should be the default-loaded preset for ODYSSEY in any public demo of XOlokun V007+.
