# OBRIX Issue Triage — 2026-04-06

Systematic audit of the Obrix flagship engine. Issues logged for GitHub migration when tooling is available.

---

## P0 — Must Fix Before V1

### OBRIX-001: Voice orphaning on poly→mono mode switch
- **Category:** DSP / Voice Management
- **Location:** `Source/Engines/Obrix/ObrixEngine.h:703-705, 2024-2065`
- **Problem:** When `voiceModeIdx` changes from Poly (3) to Mono (0/1), `polyLimit_` drops from 8 to 1. But voices in slots 1–7 that are already active continue sounding — `noteOn()` respects the new `polyLimit_` for allocation, but nothing kills existing voices beyond the new limit.
- **Expected:** Voices beyond the new polyLimit should receive immediate `noteOff()` (or fast release) when the mode switches down.
- **Suggested Fix:** At the point where `polyLimit_` is set (line 705), compare to previous value; if shrinking, release voices in slots ≥ new limit.
- **Model:** Sonnet
- **Skill:** `/dsp-safety`
- **Rationale:** Context-aware voice lifecycle change, must preserve audio-thread safety.

### OBRIX-002: COUPLING macro does nothing when no partner engine loaded
- **Category:** DSP / Macro System
- **Location:** `Source/Engines/Obrix/ObrixEngine.h:933-934`
- **Problem:** `macroCoup` only scales existing `pitchMod` and `cutoffMod` via multiplication: `pitchMod *= (1.0f + macroCoup * 2.0f)`. When no coupling input exists (solo context), coupling mods are zero, so the macro is inaudible. The seance verdict (Kakehashi) specifically called for a self-routing fallback: internal shimmer between active bricks at low macro values.
- **Expected:** COUPLING macro produces audible effect even with no partner engine — e.g., subtle cross-brick chorus/shimmer at low values, increasing cross-modulation intensity at high values.
- **Suggested Fix:** Add additive self-routing term: when `macroCoup > 0` and coupling inputs are zero, inject subtle src1↔src2 cross-modulation (pitch micro-detune + cutoff offset between the two sources).
- **Model:** Opus
- **Skill:** `/coupling-debugger`
- **Rationale:** New DSP behavior with design implications; must sound musical, not just technically present.

---

## P1 — Fix Before V1 macOS Launch

### OBRIX-003: findNearestJICents called per-sample per-voice
- **Category:** DSP / CPU Optimization
- **Location:** `Source/Engines/Obrix/ObrixEngine.h:987`
- **Problem:** `findNearestJICents()` executes every sample for each active voice when `fieldStrength > 0`. The JI target ratio changes only when note pitch changes significantly. The function iterates a static table of 7–12 ratios with `std::fabs` comparisons per call.
- **Expected:** Cache JI cents result per-voice; recompute only on note change or ratio crossing threshold.
- **Suggested Fix:** Add `cachedJICents[2]` and `cachedJIRatio[2]` to `ObrixVoice`. Recompute only when `std::fabs(ratio - cachedJIRatio) > 0.01f`.
- **Model:** Sonnet
- **Skill:** `/dsp-safety`
- **Rationale:** Multi-file change (voice struct + render loop), needs delta-threshold design.

### OBRIX-004: 86% of presets have COUPLING macro at zero
- **Category:** Preset Quality
- **Location:** `Presets/XOceanus/*/Obrix/*.xometa`
- **Problem:** The COUPLING macro — OBRIX's flagship ecosystem feature — is unused in 86% of the 394-preset library. This makes the engine's unique coupling architecture invisible to users browsing presets.
- **Expected:** At least 40-50% of presets should have non-zero COUPLING macro values, especially Entangled, Flux, and Atmosphere moods.
- **Suggested Fix:** Batch update presets to add meaningful COUPLING values. Depends on OBRIX-002 (self-routing fallback) landing first so the macro is audible in solo context.
- **Model:** Haiku (batch update after OBRIX-002)
- **Skill:** `/preset-auditor`

### OBRIX-005: Only 12% of presets engage Wave 4 biophonic features
- **Category:** Preset Quality
- **Location:** `Presets/XOceanus/*/Obrix/*.xometa`
- **Problem:** Wave 4 (Harmonic Field, Environmental, Brick Ecology, Stateful Synthesis) represents 16 of 81 parameters and is OBRIX's most novel synthesis contribution — yet 88% of presets leave all Wave 4 params at defaults. The features that differentiate OBRIX from every other synth are invisible.
- **Expected:** 7–15 "signature" presets that foreground Wave 4 biophonics, plus broader Wave 4 engagement across existing moods.
- **Suggested Fix:** Create dedicated biophonic showcase presets. See preset campaign below.
- **Model:** Opus
- **Skill:** `/preset-architect`

### OBRIX-006: 55% of presets have Drift Bus at zero
- **Category:** Preset Quality  
- **Location:** `Presets/XOceanus/*/Obrix/*.xometa`
- **Problem:** The Drift Bus (Wave 3) is OBRIX's primary temporal identity — Schulze's ultra-slow ensemble LFO. Over half the library is static in time.
- **Expected:** Majority of non-Foundation presets should have non-zero driftDepth.
- **Suggested Fix:** Batch update applicable presets with subtle drift values (0.02–0.08).
- **Model:** Haiku
- **Skill:** `/preset-auditor`

---

## P2 — Post-V1

### OBRIX-007: Wavetable content limited to 4 banks with basic crossfade
- **Category:** DSP / Content
- **Location:** `Source/Engines/Obrix/ObrixEngine.h` (renderSourceSample wavetable path)
- **Problem:** Tomita's seance criticism: "a wavetable costume." Current banks (Analog/Vocal/Metallic/Organic) are functional but not distinctive. Seance recommended 8–16 ocean-sourced wavetable banks.
- **Expected:** Expanded wavetable library with unique content (recorded ocean acoustics, bioluminescent spectra).
- **Model:** Opus
- **Rationale:** Content creation + DSP integration; post-V1 roadmap item.

---

## Summary

| ID | Priority | Category | Title | Model | Status |
|----|----------|----------|-------|-------|--------|
| OBRIX-001 | P0 | DSP | Voice orphaning on poly→mono | Sonnet | Open |
| OBRIX-002 | P0 | DSP | COUPLING macro self-routing | Opus | Open |
| OBRIX-003 | P1 | CPU | JI cents per-sample lookup | Sonnet | Open |
| OBRIX-004 | P1 | Presets | COUPLING macro 86% zero | Haiku | Blocked on 002 |
| OBRIX-005 | P1 | Presets | Wave 4 only 12% engaged | Opus | Open |
| OBRIX-006 | P1 | Presets | Drift Bus 55% zero | Haiku | Open |
| OBRIX-007 | P2 | Content | Wavetable expansion | Opus | Deferred |

**Note:** `setCoefficients()` hoisting (originally flagged P0) has already been resolved — delta-threshold guards with `kFilterDeltaHz=1.0f` / `kFilterDeltaRes=0.001f` are in place at lines 1040–1046, 1062–1068, 1111–1117. Filter type guards (`proc1Type <= 3`) prevent trig calls for Wavefolder/RingMod modes.
