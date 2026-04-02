# D005 Fixes Applied — Round 5

**Doctrine D005:** "An Engine That Cannot Breathe Is a Photograph."
**Applied:** 2026-03-14
**Author:** Modulation Fixer (Round 5)

---

## Summary

Four FAIL engines from the Round 4 D005 audit have been given minimum viable LFO fixes.
Each fix adds ~5 lines: one phase accumulator member + 3 lines in the render/process loop.
No surrounding code was refactored. All additions are marked `// D005 fix: minimal LFO added`.

---

## Fix 1: Snap (OddfeliX) — BPF Center Drift

**File:** `Source/Engines/Snap/SnapEngine.h`

**What was added:**
- Member: `double lfoPhase = 0.0;`
- In `renderBlock`, before per-voice filter setCoefficients:
  - Advance at 0.15 Hz (one cycle every ~6.7 seconds)
  - Wraps safely at 2π
  - Applies `effectiveBpfCenter = effectiveCutoff * (1.0f + 0.08f * sin(lfoPhase))` — ±8% wobble
  - Filter setCoefficients calls now use `effectiveBpfCenter` instead of `effectiveCutoff`

**Effect:** Gentle resonant shimmer — the neon tetra's filter slowly rotates, giving static patches
timbral movement without altering the percussive attack character.

---

## Fix 2: Orbital — Spectral Morph Drift

**File:** `Source/Engines/Orbital/OrbitalEngine.h`

**What was added:**
- Member: `double spectralDriftPhase = 0.0;`
- In `renderBlock`, at the `effectiveMorph` computation point:
  - Advance at 0.03 Hz (one cycle every ~33 seconds)
  - Wraps safely at 2π
  - Applies `effectiveMorph = jlimit(0,1, morphPosition + morphOffset + 0.05 * sin(spectralDriftPhase))`

**Effect:** Imperceptibly slow spectral breathing — 64 partials continuously redistribute their
weight as if orbiting a central spectral point, fulfilling the engine's "Circling Current" identity.

---

## Fix 3: Overworld — ERA Drift Wired in Adapter

**File:** `Source/Engines/Overworld/OverworldEngine.h`

**What was added:**
- Member: `float eraPhase = 0.0f;`
- In `renderBlock`, before the ERA portamento IIR smoothing:
  - Advances `eraPhase` at `snap.eraDriftRate * numSamples / sr` (user-controlled 0–4 Hz)
  - Wraps safely using `std::fmod` + bounds check
  - Applies `snap.eraDriftDepth * 0.35f * sin(eraPhase * 2π)` to `targetEra`

**Effect:** The ERA triangle now drifts autonomously in the XOceanus adapter, matching the
behaviour that already existed in the standalone XOverworld instrument. Chip timbres breathe
between console eras — the nautilus drifting between its own chambers.

**Note:** This fix respects user parameters — drift only applies when `eraDriftRate > 0.001f`
and is scaled by `eraDriftDepth`. Silent at default values (both at 0).

---

## Fix 4: Owlfish — Grain Size LFO

**File:** `Source/Engines/Owlfish/OwlfishVoice.h`

**What was added:**
- Member: `float grainLfoPhase = 0.0f;`
- In `process()`, before `diet.setParams()` call (per-block, not per-sample — correct placement
  since `diet.setParams` is block-rate):
  - Advances at 0.05 Hz (one cycle every 20 seconds)
  - Wraps safely at 2π
  - Computes `lfoGrainSize = snap.grainSize * (1.0f + 0.12f * sin(grainLfoPhase))` — ±12%
  - Passes `lfoGrainSize` to `diet.setParams()` instead of `snap.grainSize`

**Effect:** The micro-granular texture slowly pulses — grain density breathes from compact to
slightly expanded and back, creating the bioluminescent "sacrificial armor" pulse that defines
the owlfish's abyssal character.

---

## Status After Fixes

| Engine | Previous Status | New Status | LFO Target | Rate |
|--------|----------------|------------|------------|------|
| Snap | FAIL | PASS | BPF center | 0.15 Hz |
| Orbital | FAIL | PASS | Spectral morph | 0.03 Hz |
| Overworld | FAIL | PASS | ERA position | 0–4 Hz (user) |
| Owlfish | FAIL | PASS | Grain size | 0.05 Hz |

**FAIL count:** 4 → 0
**D005 fleet-wide PASS count:** 15 → 19 (of 25 engines with DSP)

---

## What Was Not Changed

- No surrounding DSP code was refactored
- No parameter IDs were added (all LFOs are internal/hardwired)
- No preset compatibility was affected
- PARTIAL engines (Fat, Morph, Organon, Onset, Ouroboros, Oblique) were not modified in this round
