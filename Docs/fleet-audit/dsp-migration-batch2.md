# DSP Migration — Batch 2 (O–Z engines)
**Date**: 2026-03-21
**Agent**: Batch 2 (engines O–Z, excluding skipped)

---

## Scope

Engines surveyed: all O-prefix directories under `Source/Engines/`.

### Skipped (pre-migrated or in-flight)
| Engine | Reason |
|--------|--------|
| Origami | Already migrated (proof-of-concept, -101 lines) |
| Oxbow | Uses shared DSP natively |
| Oware | Uses shared DSP natively |
| Oblique | Other agent (push-to-8.0+) |
| Ocelot | Other agent (push-to-8.0+) |
| Orphica | Other agent (push-to-8.0+) |

### Already using shared utilities (no migration needed)
| Engine | Utilities in use |
|--------|-----------------|
| Obbligato | PitchBendUtil |
| Ohm | PitchBendUtil |
| Ole | PitchBendUtil |
| Obscura | PitchBendUtil, StandardLFO, ParameterSmoother, VoiceAllocator |
| Obsidian | PitchBendUtil, StandardLFO, VoiceAllocator |
| Oceanic | PitchBendUtil, StandardLFO, ParameterSmoother, VoiceAllocator |
| Octopus | PitchBendUtil, StandardLFO, VoiceAllocator |
| Ombre | PitchBendUtil |
| Onset | PitchBendUtil |
| Opal | PitchBendUtil, StandardLFO, VoiceAllocator |
| OpenSky | PitchBendUtil, StandardLFO (via `using SkyLFO = StandardLFO`), VoiceAllocator |
| Oracle | PitchBendUtil, StandardLFO, GlideProcessor, VoiceAllocator |
| Orbital | PitchBendUtil |
| Orbweave | StandardLFO (via `using OrbweaveLFO = StandardLFO`) |
| Orca | PitchBendUtil, StandardLFO, GlideProcessor, ParameterSmoother, VoiceAllocator |
| Organon | PitchBendUtil |
| Osprey | PitchBendUtil, StandardLFO |
| Osteria | PitchBendUtil, StandardLFO, VoiceAllocator |
| Ostinato | PitchBendUtil, StandardLFO |
| Ottoni | PitchBendUtil |
| Ouie | PitchBendUtil, StandardLFO, VoiceAllocator |
| Ouroboros | PitchBendUtil |
| Outwit | PitchBendUtil |
| Overlap | PitchBendUtil |
| Overworld | PitchBendUtil |
| Owlfish | PitchBendUtil |

---

## Migrations Performed

### 1. OceanDeep (`Source/Engines/OceanDeep/OceanDeepEngine.h`)

**What was migrated:**
- **PitchBendUtil** — inline pitch bend parsing replaced
- **StandardLFO** — two engine-level sine LFOs (lfo1, lfo2) replaced

**Details:**

*PitchBendUtil:*
Replaced inline `pitchBendVal = (msg.getPitchWheelValue() - 8192) / 8192.f` with
`PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue())`.
Added `#include "../../DSP/PitchBendUtil.h"`.

*StandardLFO:*
Replaced two member variables `float lfo1Phase`, `float lfo2Phase` with
`StandardLFO lfo1`, `StandardLFO lfo2`.
Added `#include "../../DSP/StandardLFO.h"`.
Per-block: `lfo1.setRate(lfo1Rate, sr)` + `lfo1.setShape(StandardLFO::Sine)` (same for lfo2).
Per-sample: `lfo1.process()` replaces the inline `phaseInc += rate/sr; fastSin(phase * 2π)` loop.
Reset calls: `lfo1.reset()` / `lfo2.reset()` in both `prepare()` and `reset()`.

**Note:** `DeepBioExciter::lfoPhase` was intentionally NOT migrated. That struct uses a custom edge-detecting trigger LFO that fires burst envelopes on positive zero-crossings — fundamentally different from StandardLFO, which has no trigger logic.

**Lines removed:** ~8 lines (2 member decls + 6 inline phase accumulator lines across prepare/reset/loop)

---

### 2. Overtone (`Source/Engines/Overtone/OvertoneEngine.h`)

**What was migrated:**
- **StandardLFO** — two engine-level sine LFOs (lfo1, lfo2) replaced

**Details:**

Added `#include "../../DSP/StandardLFO.h"`.
Replaced `float lfo1Phase = 0.f`, `float lfo2Phase = 0.f` member variables with
`StandardLFO lfo1`, `StandardLFO lfo2`.
Per-block: `lfo1.setRate(lfo1Rate, sr)` + `lfo1.setShape(StandardLFO::Sine)` (same for lfo2).
Per-sample: `lfo1.process()` replaces inline phase accumulator + `fastSin(phase * 2π)`.
Reset calls updated in `prepare()` and `reset()`.

**Special case:** Line 803 used `lfo1Phase * 6.2831853f` directly to compute a shimmer phase offset for partial amplitude modulation. Migrated to `lfo1.phase * 6.2831853f` — `StandardLFO::phase` is the same 0–1 accumulator, so behavior is preserved exactly.

**PitchBendUtil:** Already in use — NOT a new addition.

**Lines removed:** ~7 lines (2 member decls + 5 inline phase accumulator lines)

---

### 3. Organism (`Source/Engines/Organism/OrganismEngine.h`)

**What was migrated:**
- **StandardLFO** — two engine-level sine LFOs (lfo1, lfo2) replaced

**Details:**

Added `#include "../../DSP/StandardLFO.h"`.
Replaced `float lfo1Phase = 0.f`, `float lfo2Phase = 0.f` member variables with
`StandardLFO lfo1`, `StandardLFO lfo2`.
Per-block: `lfo1.setRate(lfo1Rate, sr)` + `lfo1.setShape(StandardLFO::Sine)` (same for lfo2).
Per-sample: `lfo1.process()` replaces inline phase accumulator + `fastSin(phase * 2π)`.
Reset calls updated in `prepare()` and `reset()`.

**PitchBendUtil:** Already in use — NOT a new addition.

**Skipped — custom implementation:**
Organism has per-sample smoothers using `constexpr float kSmoothCoeff = 0.005f` (a hardcoded coefficient, not derived from sample rate). This is intentionally different from `ParameterSmoother`, which uses `exp(-2π * freq / sr)`. The behavior would change if ParameterSmoother were substituted at a different sample rate. Left as-is.

**Lines removed:** ~7 lines (2 member decls + 5 inline phase accumulator lines)

---

## Engines Surveyed — Not Migrated (Custom Implementations)

| Engine | Pattern | Reason not migrated |
|--------|---------|---------------------|
| Obbligato | FamilyWaveguide primitives + PitchBendUtil | No matching utility gaps |
| Ohm | Per-voice FamilyWaveguide + 2-op FM + Theremin | Custom DSP throughout; inline chorus LFO integral to FX chain |
| Ole | FamilyWaveguide family engine | No utility gaps |
| Ombre | No inline LFO/glide found | No migration candidates |
| Onset | Per-voice voice state, no inline LFO accumulator at engine level | No migration candidates |
| Optic | Visual modulation engine — no standard MIDI voice DSP | No migration candidates |
| Organon | PitchBendUtil already in use; no LFO accumulator visible | No migration candidates |
| Ouroboros | PitchBendUtil already in use; no LFO accumulator | No migration candidates |
| Outwit | PitchBendUtil already in use | No migration candidates |
| Overlap | PitchBendUtil already in use | No migration candidates |
| Overworld | PitchBendUtil already in use; inline `currentFreq += 0.f` is a 1-line guard, not glide | No migration candidates |
| Owlfish | PitchBendUtil already in use; Mixtur-Trautonium custom oscillator architecture | No migration candidates |
| Ottoni | PitchBendUtil already in use | No migration candidates |

---

## Summary

| Engine | Utilities Added | Lines Removed |
|--------|----------------|---------------|
| OceanDeep | PitchBendUtil, StandardLFO | ~8 |
| Overtone | StandardLFO | ~7 |
| Organism | StandardLFO | ~7 |
| **Total** | | **~22** |

All migrations preserve existing audio behavior exactly. No approximation changes. No parameter ID changes. No preset compatibility impact.
