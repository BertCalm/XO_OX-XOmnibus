# OBRIX Issue Triage — 2026-04-06

Systematic audit of the Obrix flagship engine. Issues logged for GitHub migration when tooling is available.

> **Status update 2026-04-18:** All three flagged DSP issues (OBRIX-001/002/003) have been resolved in-code. Remaining open items are preset-content work (OBRIX-004/005/006) and the P2 wavetable expansion. See individual items below for code refs.

---

## P0 — Must Fix Before Release

### OBRIX-001: Voice orphaning on poly→mono mode switch — ✅ RESOLVED (2026-04-18 audit)
- **Category:** DSP / Voice Management
- **Location:** `Source/Engines/Obrix/ObrixEngine.h:783-791` (was flagged at 703-705, 2024-2065)
- **Resolution:** Fix is in place — when `polyLimit` shrinks, voices in slots ≥ new limit receive `ampEnv.noteOff()` (gentle release, not hard kill). Commented with `OBRIX-001` tag.
- **Previously documented problem:** When `voiceModeIdx` changed from Poly (3) to Mono (0/1), `polyLimit_` dropped from 8 to 1, but voices in slots 1–7 continued sounding.

### OBRIX-002: COUPLING macro does nothing when no partner engine loaded — ✅ RESOLVED (2026-04-18 audit)
- **Category:** DSP / Macro System
- **Location:** `Source/Engines/Obrix/ObrixEngine.h:1082-1088` (was flagged at 933-934)
- **Resolution:** Self-routing shimmer fallback is in place — when `macroCoup > 0.001f` and no coupling input detected (`hasCouplingInput` false), injects ±12¢ micro-detune on pitch and ±600Hz filter breathing via an independent-rate shimmer LFO (driftPhase × 60 + voice offset × 0.37). Feels like the bricks breathing together.
- **Previously documented problem:** `macroCoup` only scaled existing coupling mods; in solo context (no partner), the macro was inaudible. Kakehashi's seance verdict called for self-routing fallback.

---

## P1 — Fix Before macOS Launch

### OBRIX-003: findNearestJICents called per-sample per-voice — ✅ RESOLVED (2026-04-18 audit)
- **Category:** DSP / CPU Optimization
- **Location:** `Source/Engines/Obrix/ObrixEngine.h:320-323, 1147-1152` (was flagged at 987)
- **Resolution:** Per-voice `cachedJICents[2]` and `cachedJIRatio[2]` added to `ObrixVoice`. Recomputation gated by `kJICacheThreshold` delta on ratio. Reset on voice release. Commented with `OBRIX-003` tag.
- **Previously documented problem:** `findNearestJICents()` executed every sample for each active voice when `fieldStrength > 0`.

### OBRIX-004: 86% of presets have COUPLING macro at zero — OPEN
- **Category:** Preset Quality
- **Location:** `Presets/XOceanus/*/Obrix/*.xometa`
- **Problem:** The COUPLING macro — OBRIX's flagship ecosystem feature — is unused in 86% of the 394-preset library. This makes the engine's unique coupling architecture invisible to users browsing presets.
- **Expected:** At least 40-50% of presets should have non-zero COUPLING macro values, especially Entangled, Flux, and Atmosphere moods.
- **Suggested Fix:** Batch update presets to add meaningful COUPLING values. Now unblocked: OBRIX-002 self-routing fallback is live, so macro is audible in solo context.
- **Model:** Haiku (batch update)
- **Skill:** `/preset-auditor`

### OBRIX-005: Only 12% of presets engage Wave 4 biophonic features — OPEN
- **Category:** Preset Quality
- **Location:** `Presets/XOceanus/*/Obrix/*.xometa`
- **Problem:** Wave 4 (Harmonic Field, Environmental, Brick Ecology, Stateful Synthesis) represents 16 of 81 parameters and is OBRIX's most novel synthesis contribution — yet 88% of presets leave all Wave 4 params at defaults. The features that differentiate OBRIX from every other synth are invisible.
- **Expected:** 7–15 "signature" presets that foreground Wave 4 biophonics, plus broader Wave 4 engagement across existing moods.
- **Suggested Fix:** Create dedicated biophonic showcase presets. See preset campaign below.
- **Model:** Opus
- **Skill:** `/preset-architect`

### OBRIX-006: 55% of presets have Drift Bus at zero — OPEN
- **Category:** Preset Quality
- **Location:** `Presets/XOceanus/*/Obrix/*.xometa`
- **Problem:** The Drift Bus (Wave 3) is OBRIX's primary temporal identity — Schulze's ultra-slow ensemble LFO. Over half the library is static in time.
- **Expected:** Majority of non-Foundation presets should have non-zero driftDepth.
- **Suggested Fix:** Batch update applicable presets with subtle drift values (0.02–0.08).
- **Model:** Haiku
- **Skill:** `/preset-auditor`

---

## P2 — Post-Launch

### OBRIX-007: Wavetable content limited to 4 banks with basic crossfade — OPEN
- **Category:** DSP / Content
- **Location:** `Source/Engines/Obrix/ObrixEngine.h` (renderSourceSample wavetable path)
- **Problem:** Tomita's seance criticism: "a wavetable costume." Current banks (Analog/Vocal/Metallic/Organic) are functional but not distinctive. Seance recommended 8–16 ocean-sourced wavetable banks.
- **Expected:** Expanded wavetable library with unique content (recorded ocean acoustics, bioluminescent spectra).
- **Model:** Opus
- **Rationale:** Content creation + DSP integration; post-launch roadmap item.

---

## Summary

| ID | Priority | Category | Title | Model | Status |
|----|----------|----------|-------|-------|--------|
| OBRIX-001 | P0 | DSP | Voice orphaning on poly→mono | Sonnet | ✅ Resolved |
| OBRIX-002 | P0 | DSP | COUPLING macro self-routing | Opus | ✅ Resolved |
| OBRIX-003 | P1 | CPU | JI cents per-sample lookup | Sonnet | ✅ Resolved |
| OBRIX-004 | P1 | Presets | COUPLING macro 86% zero | Haiku | Open (now unblocked) |
| OBRIX-005 | P1 | Presets | Wave 4 only 12% engaged | Opus | Open |
| OBRIX-006 | P1 | Presets | Drift Bus 55% zero | Haiku | Open |
| OBRIX-007 | P2 | Content | Wavetable expansion | Opus | Deferred |

**Note:** `setCoefficients()` hoisting (originally flagged P0) has already been resolved — delta-threshold guards with `kFilterDeltaHz=1.0f` / `kFilterDeltaRes=0.001f` are in place at lines 1040–1046, 1062–1068, 1111–1117. Filter type guards (`proc1Type <= 3`) prevent trig calls for Wavefolder/RingMod modes.
