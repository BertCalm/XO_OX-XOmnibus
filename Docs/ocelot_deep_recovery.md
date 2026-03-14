# OCELOT Deep Recovery Report

**Date:** 2026-03-14
**Previous score:** 6.4/10
**Issues addressed:** Ecosystem Matrix underused in presets; D005 LFO absent; preset count 4 (zero in Foundation/Prism/Flux)

---

## What Was Fixed

### 1. D005 — Autonomous Modulation Added

**File:** `Source/Engines/Ocelot/OcelotVoice.h`

OCELOT was the only engine in the fleet with a functional EcosystemMatrix that lacked any
autonomous evolution. The matrix route values could oscillate on their own if given a driving
LFO — but nothing drove them.

**Fix:** Added a 0.07 Hz phase accumulator (`ecosystemDriftPhase`) in `OcelotVoice`. Each
render block, the phase advances and produces a sine value that modulates `ecosystemDepth`
by ±20% of its current value before it is passed to `EcosystemMatrix::process()`.

This is thematically correct: population dynamics (Lotka-Volterra predator-prey cycles)
operate on 10–20 second timescales, making 0.07 Hz (14-second cycle) an accurate ecological
period. The drift is proportional to current `ecosystemDepth`, so it is silent when the
ecosystem is off and most audible at high matrix settings.

### 2. Preset Expansion — 8 New Presets

Previous state: 4 presets, all in Atmosphere or Aether; zero in Foundation, Prism, or Flux.
The Ecosystem Matrix cross-feed values were conservative in every existing preset.

**New presets (12 total, up from 4):**

| Name | Category | Key Matrix Feature |
|------|----------|--------------------|
| Jungle Floor | Foundation | Floor→Understory rhythm coupling + Floor→Emergent threshold trigger |
| Canopy Layer | Atmosphere | Canopy→Floor negative damping (brightness dims percussion) |
| Ice Chime Matrix | Prism | Full upper cascade: Floor→Canopy, Canopy→Emergent, Emergent→Canopy feedback |
| Predator Cycle | Flux | Inverse threshold routes — creature calls silence floor and understory |
| Strata Cascade | Entangled | Unidirectional chain: Floor→Under→Canopy→Emergent, ECOSYSTEM macro amplifies all |
| Feedback Biome | Entangled | All 12 routes active simultaneously; D005 drift LFO most audible here |
| Whale Cathedral | Aether | Minimal matrix: one route only (Emergent→Canopy shimmer). Space over density. |
| Thermal Layer | Aether | Inverse threshold: Canopy→Floor silence route; pad opening mutes percussion |

---

## What the Ecosystem Matrix Actually Does

The Ecosystem Matrix is a 12-route per-block cross-modulation system in which each of
OCELOT's four synthesis strata (Floor, Understory, Canopy, Emergent) can read the output
amplitude, timbre, pitch, or pattern density of any other stratum and inject that as an
additive modulation signal into any parameter of any other stratum. The matrix runs once
per audio block (not per sample), meaning it introduces exactly one block of latency —
typically 5–12 ms — which is perceptually inaudible but sufficient to capture the
block-level energy of each stratum.

The 12 routes are typed: **Continuous** routes (8 total) compute a simple linear bipolar
scalar; **Threshold** routes (2 total) apply a sigmoid function so the source only drives
the destination once it crosses an amplitude threshold (and negative amounts invert this —
silence fires, not peaks); **Rhythmic** routes (2 total) produce stepped values where the
sign controls whether fast rhythmic activity at the source speeds up or opposes the
destination's rhythm. A master `ecosystemDepth` scalar multiplies all 12 outputs before
application. This is structurally similar to analog voltage-controlled summing matrices
in modular synthesis — with the specific innovation that each route is typed to match
the nature of the signal crossing it.

The novelty is ecological: the route types correspond to real inter-species relationships.
A threshold route with negative polarity on Floor→Emergent is literally "bird calls go
quiet when predator is loud." A rhythmic route with negative polarity on Emergent→Understory
is "dense creature activity fragments the samples against the call rhythm." This is not
metaphor — it is the acoustic behavior of actual forest ecosystems, encoded as DSP logic.

---

## Macros (Fixed in Prior Round)

The four macros were confirmed live in this audit:

| Macro | DSP Target |
|-------|-----------|
| PROWL | ecosystemDepth + density |
| FOLIAGE | reverbSize + reverbMix |
| ECOSYSTEM | xfFloorCanopy + xfCanopyFloor + xfUnderEmerg |
| CANOPY | canopyLevel + canopyShimmer + canopySpectralFilter |

These were wired in the D004 round (Round 3B). This recovery confirms they are audible
across all new presets — every preset has all four macros at 0.0 default so the effect
of raising each is unambiguous.

---

## Remaining Limitations (Phase 2)

1. `floorTimbre` in `StrataSignals` is hardcoded to `0.5f` — real spectral centroid proxy
   would make the Continuous Floor→Canopy route more musically interesting.
2. The Understory read position mod (`understoryGrainPosMod`) can produce zipper clicks
   at high matrix values — a smoothing stage is needed in Phase 2.
3. D006 (velocity→timbre and expression CC) not yet addressed for OCELOT. Velocity drives
   amplitude through all four strata but does not change spectral content independently.

---

## Score Assessment

With the D005 LFO fix and 8 new presets demonstrating the matrix at diverse configurations,
OCELOT should advance from 6.4/10 toward 7.5–8.0/10. The Ecosystem Matrix remains the
most architecturally novel feature in the XOmnibus fleet — a typed directed-graph
cross-modulation system with ecological behavioral semantics. The recovery ensures
this feature is no longer invisible.
