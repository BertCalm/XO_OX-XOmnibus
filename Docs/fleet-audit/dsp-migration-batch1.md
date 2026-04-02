# DSP Migration — Batch 1 (A–N Engines)
**Date**: 2026-03-21
**Agent**: Batch 1 (engines A–N alphabetically by directory name)
**Scope**: Engines Bite, Bob, Drift, Dub, Fat, Morph

---

## Summary

| Metric | Value |
|--------|-------|
| Engines audited | 6 |
| Migrations performed | 1 |
| Engines already fully migrated for their patterns | 3 (Bite, Bob, Drift) |
| Engines with custom/non-migratable patterns | 2 (Fat LFO2, Morph LFOs) |
| Lines of inline DSP removed | **~59** (DubLFO class + call-site cleanup) |

---

## Engine-by-Engine Analysis

### Bite (BiteEngine.h) — `poss_` prefix
**Status**: SKIPPED — custom implementations, no new migrations applicable

- **LFO**: `BiteLFO` class has 7 shapes (Sine, Triangle, Saw, Square, S&H, Random, Stepped) — StandardLFO has only 5. Random and Stepped are engine-specific. **Cannot migrate.**
- **Glide**: `glideCoeff = std::exp(-1.0f / (srf * glideTime))` (feedback coefficient form, not IIR ramp). Different coefficient convention from GlideProcessor's `1.0f - exp(...)`. Would require changing audio behaviour. **Skip.**
- **PitchBendUtil**: Already using (`#include "../../DSP/PitchBendUtil.h"`, line 3).
- **VoiceAllocator**: Already using (line 10).
- **FilterEnvelope**: Uses `StandardADSR` via `BiteAdsrEnvelope = StandardADSR` alias for all three envelopes (amp, filter, mod). No separate FilterEnvelope needed.
- **ParameterSmoother**: No one-pole parameter smoothing patterns found.
- **GlideProcessor**: Per-voice `glideFreq`/`glideTarget` with incompatible coefficient convention — skip.

**Net lines removed**: 0

---

### Bob (BobEngine.h) — `bob_` prefix
**Status**: SKIPPED — already using StandardLFO, PitchBendUtil, VoiceAllocator; custom glide

- **StandardLFO**: Already using (line 9). `BobCuriosityLFO` wraps StandardLFO for Sine/Triangle shapes and adds custom S&H/SmoothRand on top — intentional hybrid, do not change.
- **PitchBendUtil**: Already using (line 3).
- **VoiceAllocator**: Already using (line 10).
- **Glide**: `glideCoeff = 1.0f - std::exp(-1.0f / (srf * glideTimeSec))` — coefficient matches GlideProcessor, but glide state lives in `BobVoice.glideSourceFreq`/`glideActive` pair rather than a GlideProcessor struct. Migrating would require adding `GlideProcessor` to the voice struct, altering noteOn/noteOff control flow. Conservative pass — **skip** (per-voice structural refactor, not a simple swap).
- **FilterEnvelope**: Uses `StandardADSR` for all envelopes.
- **ParameterSmoother**: No one-pole smoothing patterns found.

**Net lines removed**: 0

---

### Drift (DriftEngine.h) — `drift_` prefix
**Status**: SKIPPED — already using StandardLFO, PitchBendUtil, VoiceAllocator

- **StandardLFO**: Already using (line 9). File header comment confirms: "DriftLFO replaced by StandardLFO."
- **PitchBendUtil**: Already using (line 3).
- **VoiceAllocator**: Already using (line 11). File header comment confirms: "findFreeVoice replaced by VoiceAllocator::findFreeVoice."
- **Glide**: Uses same per-voice `glideActive`/`glideSourceFreq` pattern as Bob — custom float coefficient `fastExp(-1.0f / ...)`. Same structural reason to skip as Bob.
- **FilterEnvelope**: Uses StandardADSR for amp; filter cutoff is modulated by amp envelope value scaled by `filterEnv` amount param (no separate filter ADSR). No FilterEnvelope needed.
- **ParameterSmoother**: No one-pole smoothing patterns found.

**Net lines removed**: 0

---

### Dub (DubEngine.h) — `dub_` prefix
**Status**: MIGRATED — DubLFO → StandardLFO

#### What changed
1. Added `#include "../../DSP/StandardLFO.h"` (line 10)
2. Removed inline `DubLFO` class body (~62 lines: class declaration + enum + prepare + reset + setRate + setShape + process + private fields)
3. Added migration tombstone comment explaining the substitution
4. Changed member declarations: `DubLFO lfo` → `StandardLFO lfo`; `DubLFO lfo2` → `StandardLFO lfo2`
5. Removed `lfo.prepare(sr)` and `lfo2.prepare(sr)` calls from `prepare()` (StandardLFO has no `prepare()`)
6. Updated `lfo.setRate(lfoRate)` → `lfo.setRate(lfoRate, srf)` (SR now passed at call site)
7. Updated `lfo2.setRate(std::max(...))` → `lfo2.setRate(std::max(...), srf)`

#### Why this is safe
- DubLFO waveforms: Sine=0, Triangle=1, Saw=2, Square=3, S&H=4 — identical numeric values and implementations to StandardLFO
- DubLFO S&H uses `sampleCounter * 1664525u + 1013904223u` (Knuth TAOCP) — same LCG as StandardLFO's `rngState`
- DubLFO default shape is `Shape::Sine` (shape index 0) — same as StandardLFO default (shape=0)
- `lfo` has no setShape call in the render path (always sine), `lfo2.setShape(1)` = Triangle (same index in StandardLFO)
- Float vs double phase: DubLFO used `double phase`; StandardLFO uses `float phase`. At Dub's LFO rate range (0.1–20 Hz at 44.1–96 kHz), float precision is fully adequate (32-bit float has ~7 decimal digits; LFO cycle at 20 Hz/48kHz = 2400 samples → phase increment ~4.16e-4, far above float's 1.2e-7 floor). No audible difference.
- `reset()` signature is compatible (`lfo.reset()` clears phase and holdValue in both).

#### Patterns NOT migrated in Dub
- **Glide**: Per-voice `glideActive`/`glideSourceFreq` pair with coefficient `1.0f - fastExp(-1.0f / (srf * ...))`. Matches GlideProcessor semantics but requires structural voice-struct change. Skipped per conservative policy.
- **PitchBendUtil**: `PitchBendUtil::parsePitchWheel` already in use (line 3).
- **VoiceAllocator**: Already in use (line 10).
- **FilterEnvelope**: Amp envelope modulates filter cutoff via scalar `filterEnv` amount (no separate ADSR for filter). No FilterEnvelope needed.
- **ParameterSmoother**: No one-pole smoothing patterns found.

**Net lines removed**: ~59 (62-line DubLFO class → 4-line tombstone comment; 2 prepare calls → 1 comment line; 2 setRate call-sites: minor edits)

---

### Fat (FatEngine.h) — `fat_` prefix
**Status**: SKIPPED — already using most utilities; inline LFO2 is block-advanced

- **StandardLFO**: Already using (line 18). Per-voice `BreathingLFO` from StandardLFO.h.
- **VoiceAllocator**: Already using (line 20).
- **PitchBendUtil**: Already using (line 17).
- **LFO2**: Inline `lfo2Phase`/`lfo2Output` using block-advance (phase steps by `numSamples` per block, not per-sample). StandardLFO advances per-sample only. Block-advanced LFOs are intentional — they avoid per-sample math and advance in sync with the buffer boundary. **Cannot migrate without changing audio behaviour.**
- **Glide**: Per-voice `glideActive`/`glideSourceFreq` — same structural reason as Bob/Drift. Skip.
- **FilterEnvelope**: Has per-voice `filterEnv` (StandardADSR) alongside `ampEnv`. Uses StandardADSR directly — already correct.
- **ParameterSmoother**: No one-pole smoothing patterns found.

**Net lines removed**: 0

---

### Morph (MorphEngine.h) — `morph_` prefix
**Status**: SKIPPED — block-advanced LFOs are intentional; glide is structural

- **LFOs**: Two inline block-advanced LFOs:
  - `lfoPhase` — hardcoded 0.3 Hz sine, advances by `numSamples` per block. Used as coupling signal output (`lfoOutput`). Block-advance is intentional for coupling accuracy.
  - `lfo2Phase` — user-rate triangle LFO, also block-advanced. Rate range 0.005–2 Hz.
  - Both are fundamentally different from StandardLFO's per-sample model. **Cannot migrate.**
- **VoiceAllocator**: Already using (line 9, `VoiceAllocator::findFreeVoice` on line 1095).
- **PitchBendUtil**: Already using (line 7).
- **Glide**: `glideCoefficient` stored per-voice. Semantics match GlideProcessor but glide state is embedded in `MorphVoice` struct. Structural refactor — skip.
- **FilterEnvelope**: Uses `StandardADSR` for amp; no separate filter ADSR (Moog ladder is swept by LFO2, not an ADSR). FilterEnvelope not needed.
- **ParameterSmoother**: No inline `target - current * coeff` smoothing patterns found.

**Net lines removed**: 0

---

## Files Modified

| File | Change | Lines Removed |
|------|--------|--------------|
| `Source/Engines/Dub/DubEngine.h` | DubLFO → StandardLFO | ~59 |

---

## Updated Migration Matrix (A–N engines after Batch 1)

| Engine | LFO | ADSR | FiltEnv | VoiceAlloc | Glide | PSmooth | PBend | Score |
|--------|-----|------|---------|------------|-------|---------|-------|-------|
| Bite | N† | Y | N | Y | N | N | Y | 3/7 |
| Bob | Y | Y | N | Y | N | N | Y | 4/7 |
| Drift | Y | Y | N | Y | N | N | Y | 4/7 |
| **Dub** | **Y** | Y | N | Y | N | N | Y | **4/7** |
| Fat | Y | Y | N | Y | N | N | Y | 4/7 |
| Morph | N† | Y | N | Y | N | N | Y | 3/7 |

†LFO: Bite has a 7-shape custom BiteLFO (not migratable). Morph has block-advanced LFOs (not migratable).

Note: Bite's score updated to 3/7 (was 2/7 in prior doc) — PitchBendUtil was already present, updating matrix accordingly.

---

## Remaining Opportunities (A–N batch)

### GlideProcessor (would require per-voice struct changes)
All six engines have inline glide implementations using the correct `1.0f - exp(...)` coefficient form that matches GlideProcessor. However, all require adding `GlideProcessor` as a member of the voice struct and refactoring noteOn/noteOff control flow — this is a structural change, not a simple swap. Deferred to a future structural migration pass.

| Engine | Inline Glide Lines | Structural Effort |
|--------|-------------------|-------------------|
| Bite | ~18L | Medium (per-voice struct change + 3 call sites) |
| Bob | ~4L | Low (per-voice struct change + 2 call sites) |
| Drift | ~3L | Low (per-voice struct change + 2 call sites) |
| Dub | ~4L | Low (per-voice struct change + 2 call sites) |
| Fat | ~4L | Low (per-voice struct change + 2 call sites) |
| Morph | ~16L | High (voiceMode/legato logic intertwined) |

### ParameterSmoother
None of the A–N engines have standalone one-pole parameter smoothing patterns. Their smoothing (where present) is either inside DSP helper classes or via JUCE's built-in param smoothing.

### FilterEnvelope
None of the A–N engines have a separate filter ADSR that could be swapped for FilterEnvelope without restructuring. Bite/Bob/Fat use StandardADSR for their filter envelopes already. Dub/Drift/Morph drive filter from the amp envelope via a scalar amount.
