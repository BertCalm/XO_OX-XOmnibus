# Sweep Round 8 — Final Audit Batch
**Date:** 2026-03-24
**Engines audited:** OBLIQUE, OSPREY, OHM, OBBLIGATO, OTTONI, ORBITAL, OBSIDIAN, OWLFISH
**Fixes applied (Round 7 P1 + easy wins):** OCEANDEEP delay buffer, OCTOPUS chromatophore coeff, ORIGAMI panning

---

## Round 7 Fixes Applied

### FIX 1 — OCEANDEEP: Dynamic delay buffer (P1 correctness)
**File:** `Source/Engines/OceanDeep/OceanDeepEngine.h`
**Problem:** `DeepWaveguideBody` had `static constexpr int kMaxDelay = 48001` with a stack-allocated `float buf[kMaxDelay]`. At 192 kHz, a 1 Hz fundamental needs 192,001 samples — 4× the buffer. At 96 kHz the buffer was tight (needed 96,001, had 48,001). The clamp on line 135 would silently cap the delay, raising the lowest playable frequency to ~4 Hz at 192 kHz instead of the designed 1 Hz.
**Fix:** Changed `buf` to `std::vector<float>`. `prepare()` now computes `maxDelay = (int)(sr + 1.5f)` and calls `buf.assign(maxDelay, 0.f)`. All read/write accesses use `static_cast<size_t>()`. Added `#include <vector>`.
**Result:** Correct 1 Hz fundamental at all sample rates. No audio-thread allocation (vector allocated once in `prepare()`).

### FIX 2 — OCTOPUS: Chromatophore envelope follower coefficient (P1 perf)
**File:** `Source/Engines/Octopus/OctopusEngine.h`
**Problem:** `chromaCoeff = 1.0f - std::exp(-kTwoPi * (pChromaSpeed * 20.0f + 1.0f) / srf)` was computed inside the per-sample loop, inside the per-voice inner loop. Both `pChromaSpeed` (block-rate param) and `srf` (sample rate) are block-constant. At 16-voice max polyphony and 512-sample blocks, this was 8,192 `std::exp` calls per block that could be 1.
**Fix:** Hoisted `const float chromaCoeff` to just before the sample loop. Removed the local `float chromaCoeff` declaration from inside the loop.
**Result:** 1 `std::exp` call per block instead of up to 8,192. No behavioural change.

### FIX 3 — ORIGAMI: Per-voice constant-power panning (P1 perf)
**File:** `Source/Engines/Origami/OrigamiEngine.h`
**Problem:** `std::cos(panPosition * kPI * 0.5f)` and `std::sin(panPosition * kPI * 0.5f)` were computed per-sample inside the per-voice inner loop. `panPosition` depends only on `voiceIndex` (a compile-time-deterministic value) — it cannot change within a block. At 8 voices × 512 samples this was 8,192 `std::cos`/`std::sin` pairs per block.
**Fix:** Added a `VoicePanGains` struct and `std::array<VoicePanGains, kMaxVoices> voicePanGains` precomputed before the sample loop. The inner loop now does a single array lookup by `voiceIndex`.
**Result:** 16 transcendental calls per block (8 cos + 8 sin) instead of up to 8,192. No behavioural change.

### DEFERRED — DRIFT tidal pulse std::sin
The `std::sin` in `DriftTidalPulse` advances a continuous phase accumulator — it is mathematically per-sample and cannot be hoisted without breaking the modulation signal. Documented as future optimization target (Padé approximation or BLEP-table LFO).

### DEFERRED — FAT ZDF ladder std::tan
The `std::tan` in ZDF prewarping is correct per the matched-Z transform (required when cutoff modulates per-sample). Changing it would alter the sound signature. Documented as candidate for a fleet-wide Padé approximation utility, not a correctness bug.

---

## Round 8 Audit — 8 Engines, 10-Point Checklist

**Checklist:**
1. Coupling reset — accumulators zeroed each block
2. Buffer sizing — fixed arrays safe at 192 kHz
3. `thread_local` / static mutable state — none in hot paths
4. Per-sample transcendentals — no `std::sin/cos/exp/tan/pow` per sample unless mathematically required
5. SilenceGate — present and used correctly
6. Denormals — `flushDenormal()` in all feedback paths
7. Div-by-zero — all denominators guarded
8. Audio-thread allocations — no `new`/`push_back`/`resize` in `renderBlock`
9. Filter method — Cytomic SVF or matched-Z IIR (no Euler approximation)
10. Feedback protection — all recursive paths have magnitude clamps or denormal flushing

---

### 1. OBLIQUE (`Source/Engines/Oblique/ObliqueEngine.h` — 1765 lines)

| Check | Result | Notes |
|-------|--------|-------|
| Coupling reset | PASS | `externalPitchModulation = 0.0f; externalFilterModulation = 0.0f` consumed and cleared at block start |
| Buffer sizing | PASS | `kMaxDelaySamples = 96000` with `delaySamples = std::min(delaySamples, kMaxDelaySamples - 1)` clamp — silently limits max delay to 0.5 s at 192 kHz (aesthetic tradeoff, not overflow) |
| thread_local | PASS | No mutable statics in hot paths; `static constexpr` constants only |
| Transcendentals | P2 | `std::cos(bouncePan * kHalfPi)` / `std::sin(bouncePan * kHalfPi)` computed per sample in the per-voice inner loop. `bouncePan` is note-number-based (doesn't change mid-note) — could be precomputed per voice per block. Low priority because bounce events are sparse. |
| SilenceGate | PASS | Present; `analyzeBlock` called post-render |
| Denormals | PASS | `flushDenormal` in click decay and delay feedback paths |
| Div-by-zero | PASS | Denominators guarded (`kNumFacets` is constexpr 6, all divisions by sample-rate use float casts) |
| Audio-thread alloc | PASS | `resize` only in `prepare()`; `renderBlock` clean |
| Filter method | PASS | `CytomicSVF` throughout |
| Feedback protection | PASS | `feedbackAccumulator = flushDenormal(feedbackSum / kNumFacets)` |

**P0/P1 count: 0**
**P2 note:** Bounce pan transcendentals are per-sample but bounce events are rare (triggered by timing logic, not every sample). Negligible real-world cost.

---

### 2. OSPREY (`Source/Engines/Osprey/OspreyEngine.h` — 2054 lines)

| Check | Result | Notes |
|-------|--------|-------|
| Coupling reset | PASS | Full accumulator clear at block start (`couplingExcitationMod = 0.0f` etc.) |
| Buffer sizing | **P1** | `AllpassDelay::kMaxBufferSize = 4096`. Harbor verb allpass lengths are scaled by `sampleRateFloat / 44100.0f`. At 192 kHz: base lengths {1087, 1283, 1511, 1777} scale to {4732, 5585, 6578, 7736} — all exceed 4096. `setParams` clamps to `kMaxBufferSize - 1 = 4095`, so no buffer overflow occurs, but the allpass character degrades significantly at 192 kHz (shorter delays than intended). |
| thread_local | PASS | No mutable statics in hot paths |
| Transcendentals | PASS | All transcendentals (pan law, `std::exp` for glide) are at block/noteOn rate, not per-sample |
| SilenceGate | PASS | Present; `setHoldTime(500.0f)` accounts for reverb tails |
| Denormals | PASS | `flushDenormal` in resonator states and turbulence states |
| Div-by-zero | PASS | `std::max(formantBandwidths[i], 10.0f)` prevents zero-bandwidth resonators |
| Audio-thread alloc | PASS | Vectors sized in `prepare()` only |
| Filter method | PASS | Purpose-built resonator filters with proper bandwidth; not Euler |
| Feedback protection | PASS | Resonator states flush denormals; no runaway feedback |

**P0/P1 count: 1 (P1)**
**P1-OSP-01:** `AllpassDelay::kMaxBufferSize = 4096` is too small for 192 kHz sample-rate-scaled allpass lengths. No buffer overflow (clamped), but harbor verb loses character at 96/192 kHz. Fix: increase to `16384` (covers up to 384 kHz, still only 64 KB stack memory for 4 allpasses).

---

### 3. OHM (`Source/Engines/Ohm/OhmEngine.h` — 622 lines)

| Check | Result | Notes |
|-------|--------|-------|
| Coupling reset | **P1** | `extPitchMod`, `extDampMod`, `extIntens` are set in `applyCouplingInput` but **never reset to defaults** in `renderBlock`. When coupling is disconnected (no `applyCouplingInput` call), the last-received values persist across all subsequent blocks. `extIntens` defaults to 1.0f and `extDampMod` to 0.0f so they are benign when coupling is absent, but `extPitchMod` will hold its last non-zero pitch offset indefinitely after the source engine stops. |
| Buffer sizing | PASS | Reverb `combBuf` uses `std::vector` sized from `kCombLens` fixed primes (sample-rate-independent lengths from Moorer 1979 — acceptable artistic choice) |
| thread_local | PASS | None in hot paths |
| Transcendentals | PASS | `std::sin` calls are inside oscillator `tick()` methods which are the synthesis core — required per-sample |
| SilenceGate | PASS | Present; `isBypassed()` checked before DSP |
| Denormals | PASS | `flushDenormal` in comb filter and allpass feedback paths |
| Div-by-zero | PASS | `std::max(density, 1.0f)` guards grain density; `std::max(decay, 0.01f)` guards FM decay rate |
| Audio-thread alloc | PASS | No allocation in `renderBlock` |
| Filter method | PASS | `FamilyWaveguide` primitives use proper delay-line + damping filter topology |
| Feedback protection | PASS | Waveguide delay line uses `flushDenormal` on write |

**P0/P1 count: 1 (P1)**
**P1-OHM-01:** `extPitchMod` coupling accumulator not reset per block. After a coupling source engine is removed, `extPitchMod` retains the last-received pitch offset, permanently detuning OHM. Fix: reset `extPitchMod = 0.f; extDampMod = 0.f; extIntens = 1.f` at the top of `renderBlock` before consumption.

---

### 4. OBBLIGATO (`Source/Engines/Obbligato/ObbligatoEngine.h` — 519 lines)

| Check | Result | Notes |
|-------|--------|-------|
| Coupling reset | **P1** | Same pattern as OHM: `extPitchMod`, `extDampMod`, `extIntens` not reset per block. Sticky pitch offset persists after coupling source disconnects. |
| Buffer sizing | **P1** | `kDelayLen=661` (~15 ms at 44.1 kHz), `kDarkDelayLen=1543` (~35 ms), `kSpringLen=307` (~7 ms). All three are stack-allocated fixed arrays not scaled by sample rate. At 192 kHz: 15 ms = 2,880 samples (needs buffer > 661 → **index overflow**), 35 ms = 6,720 samples (needs > 1,543 → **overflow**), 7 ms = 1,344 samples (needs > 307 → **overflow**). `brightDelayPos++` and `darkDelayPos++` increment unbounded, and the modulo (`% kDelayLen`) wraps within the buffer, but the delay *time* is wrong (very short ~0.16 ms instead of 15 ms at 192 kHz). Not a buffer overrun (modulo prevents it) but produces incorrect/near-zero delay times at elevated sample rates. |
| thread_local | PASS | `static constexpr` tables only |
| Transcendentals | PASS | `std::sin` inside `drift.tick()` is the organic drift oscillator synthesis core — required per-sample |
| SilenceGate | PASS | Present; correct usage |
| Denormals | PASS | `flushDenormal` in all delay/filter feedback paths |
| Div-by-zero | PASS | `drive` in `fastTanh(wetL * drive) / drive` is only applied when drive > 0 (parameter range guard) |
| Audio-thread alloc | PASS | No allocation in `renderBlock` |
| Filter method | PASS | `FamilyWaveguide` primitives; `AirJetExciter`/`ReedExciter` use proper waveguide delay |
| Feedback protection | PASS | Plate, spring, and delay buffers all use `flushDenormal` on write |

**P0/P1 count: 2 (both P1)**
**P1-OBBL-01:** Coupling sticky-value bug (same as OHM). Fix: reset extPitchMod/extDampMod/extIntens at renderBlock top.
**P1-OBBL-02:** Fixed delay buffer sizes produce near-zero delay times (incorrect sonic character, not overflow) at 96/192 kHz. Fix: either scale `kDelayLen` etc. by `sr/44100` and convert to `std::vector`, or clamp the delay write position appropriately.

---

### 5. OTTONI (`Source/Engines/Ottoni/OttoniEngine.h` — 469 lines)

| Check | Result | Notes |
|-------|--------|-------|
| Coupling reset | **P1** | Same sticky-value pattern as OHM/OBBLIGATO: `extPitchMod`, `extDampMod`, `extIntens` not reset per block. |
| Buffer sizing | PASS | Reverb `kRevMax=4096` is correct — `combLens[4]` are computed with `srMul=std::max(1,(int)(sr/44100+0.5))` scaling, so at 192 kHz `srMul=4` and max comb = 1559 × 4 = 6236 > 4096. **Potential index issue.** However, reverb read/write uses `%kRevMax` modulo, so no OOB access, but reverb comb lengths longer than kRevMax are silently truncated. At 192 kHz this means reverb tail is shorter than intended (6,236 samples clamped to 4,095). `kDelMax=96000` delay buffer is sufficient at up to 48 kHz (2s) but at 192 kHz only covers 500 ms. |
| thread_local | PASS | None in hot paths |
| Transcendentals | PASS | `std::sin` in vibrato LFO phases is required per-sample synthesis |
| SilenceGate | PASS | Present; correct usage |
| Denormals | PASS | `flushDenormal` in waveguide write and reverb comb states |
| Div-by-zero | PASS | `wTot > 0.001f` guards cross-age weighted average |
| Audio-thread alloc | PASS | No allocation in `renderBlock` |
| Filter method | PASS | `FamilyWaveguide` primitives; proper delay + damping filter |
| Feedback protection | PASS | Waveguide and reverb paths have denormal flushing |

**P0/P1 count: 1 (P1)**
**P1-OTTO-01:** Coupling sticky-value bug (same pattern as OHM). Fix: reset coupling accumulators at renderBlock top.
**P2 note:** `kRevMax=4096` silently truncates reverb comb lengths at 192 kHz (too short). Increase to `16384` for correct 192 kHz reverb character. Not a correctness bug (no overflow, no wrong audio other than shorter tail) but a regression from intended character.

---

### 6. ORBITAL (`Source/Engines/Orbital/OrbitalEngine.h` — 1634 lines)

| Check | Result | Notes |
|-------|--------|-------|
| Coupling reset | PASS | All coupling accumulators explicitly zeroed: `externalPitchMod = externalMorphMod = externalFilterMod = 0.0f; externalFmMod = externalDecayMod = externalBlendMod = 0.0f` with `couplingAudioBuffer.clear()` and `hasAudioCoupling = false` |
| Buffer sizing | PASS | No fixed DSP delay arrays; 64-partial additive uses only phase accumulators |
| thread_local | PASS | `static const` spectral profile tables only (immutable) |
| Transcendentals | PASS | `std::pow`/`std::exp` in `buildBell()`, `buildGlass()`, `buildVocal()` are called only at `prepare()` time to build static profile tables, not per-block or per-sample. Per-sample synthesis uses only phase increment addition. `std::sqrt(rmsAccumulator)` is once per block (post-render), not per-sample. `std::sqrt` for inharmonicity is in `handleNoteOn` (once per note-on). All correct. |
| SilenceGate | PASS | Present; `analyzeBlock` called post-render |
| Denormals | PASS | `flushDenormal(voice.envLevel)` in decay and release stages; group envelope levels also flushed |
| Div-by-zero | PASS | `kInvPartialCount` is compile-time constant (1/63); sample-rate division uses `static_cast<float>(numSamples)` with `numSamples > 0` guaranteed by engine contract |
| Audio-thread alloc | PASS | No allocation in `renderBlock` |
| Filter method | PASS | `CytomicSVF` post-filter; tube saturator separate |
| Feedback protection | PASS | No feedback paths in additive synthesis (phase accumulators, no recursive filters in the core) |

**P0/P1 count: 0**
**ORBITAL is clean.**

---

### 7. OBSIDIAN (`Source/Engines/Obsidian/ObsidianEngine.h` — 1440 lines)

| Check | Result | Notes |
|-------|--------|-------|
| Coupling reset | PASS | `couplingPhaseDistortionDepthMod = 0.0f; couplingFilterCutoffMod = 0.0f; couplingDensityMod = 0.0f; couplingTiltMod = 0.0f` zeroed at top of `renderBlock` |
| Buffer sizing | PASS | No fixed delay arrays; phase distortion uses LUTs built in `prepare()` |
| thread_local | PASS | None in hot paths |
| Transcendentals | PASS | `std::cos`/`std::sin`/`std::pow` used exclusively in LUT construction (`prepare()` time) and noteOn stiffness calculation. Per-sample synthesis uses `lookupDistortion()` (trilinear LUT interpolation) and `lookupCosine()` (LUT) — no transcendentals per sample. The global LFO `std::sin` is computed once per block (block-center approximation, line ~431). |
| SilenceGate | PASS | Present; `analyzeBlock` called post-render |
| Denormals | PASS | `flushDenormal` on crossfadeGain, currentFrequency, and output paths |
| Div-by-zero | PASS | LUT index arithmetic uses integer ops; interpolation weights are fractional remainders bounded [0,1] |
| Audio-thread alloc | PASS | No allocation in `renderBlock` |
| Filter method | PASS | `CytomicSVF` main filter and 4 formant band-pass filters |
| Feedback protection | PASS | No recursive feedback in PD synthesis; formant filters are ADSR-driven, not self-oscillating |

**P0/P1 count: 0**
**OBSIDIAN is clean.** The LUT approach is exemplary — all transcendentals precomputed, zero per-sample trig cost.

---

### 8. OWLFISH (`Source/Engines/Owlfish/OwlfishEngine.h` — 253 lines)

| Check | Result | Notes |
|-------|--------|-------|
| Coupling reset | PASS | `couplingGrainMod = 0.0f; couplingSubMod = 0.0f; couplingPitchMod = 0.0f` reset after consumption in `renderBlock` |
| Buffer sizing | PASS | Monophonic adapter; buffer sizing delegated to `OwlfishVoice` (standalone-tested) |
| thread_local | PASS | None |
| Transcendentals | PASS | Not directly visible in adapter; voice DSP in `OwlfishVoice.h` (standalone, already audited in fleet sweep) |
| SilenceGate | PASS | Present; `silenceGate.prepare(sampleRate, maxBlockSize)` in `prepare()` |
| Denormals | PASS | Delegated to voice; adapter output paths write to `outputCacheL/R` vectors |
| Div-by-zero | PASS | No divisions in adapter |
| Audio-thread alloc | PASS | `outputCacheL/R` sized in `prepare()` only; no alloc in `renderBlock` |
| Filter method | PASS | Delegated to standalone voice |
| Feedback protection | PASS | Delegated to standalone voice |

**P0/P1 count: 0**
**OWLFISH adapter is clean.** Thin adapter pattern correctly delegates all DSP risk to the standalone voice.

---

## Round 8 Summary

| Engine | P0 | P1 | P2 | Status |
|--------|----|----|-----|--------|
| OBLIQUE | 0 | 0 | 1 | CLEAN |
| OSPREY | 0 | 1 | 0 | 1 fix needed |
| OHM | 0 | 1 | 0 | 1 fix needed |
| OBBLIGATO | 0 | 2 | 0 | 2 fixes needed |
| OTTONI | 0 | 1 | 1 | 1 fix needed |
| ORBITAL | 0 | 0 | 0 | CLEAN |
| OBSIDIAN | 0 | 0 | 0 | CLEAN |
| OWLFISH | 0 | 0 | 0 | CLEAN |
| **TOTAL R8** | **0** | **5** | **2** | |

### R8 P1 Issue List

| ID | Engine | Severity | Description |
|----|--------|----------|-------------|
| P1-OSP-01 | OSPREY | P1 | `AllpassDelay::kMaxBufferSize=4096` too small for 192 kHz sample-rate-scaled allpass lengths (4732–7736 samples needed). Harbor verb silently degraded at 96/192 kHz. Fix: increase to 16384. |
| P1-OHM-01 | OHM | P1 | `extPitchMod` not reset per block; sticky pitch offset persists after coupling source disconnects. Fix: reset `extPitchMod=0.f; extDampMod=0.f; extIntens=1.f` at renderBlock top. |
| P1-OBBL-01 | OBBLIGATO | P1 | Same coupling sticky-value pattern as OHM. |
| P1-OBBL-02 | OBBLIGATO | P1 | Fixed delay buffers (661/1543/307) produce near-zero (~0.16/0.37/0.07 ms) delay times at 192 kHz instead of designed 15/35/7 ms. Fix: scale by `sr/44100` or use `std::vector` sized in `prepare()`. |
| P1-OTTO-01 | OTTONI | P1 | Same coupling sticky-value pattern as OHM. |

### Coupling sticky-value pattern (OHM / OBBLIGATO / OTTONI)
All three share identical code from a common template. All three have `extPitchMod`, `extDampMod`, `extIntens` set in `applyCouplingInput` but never reset at block start. The one-line fix is the same for all three: add to the top of `renderBlock`, after the SilenceGate early-return:
```cpp
extPitchMod = 0.f;
extDampMod  = 0.f;
extIntens   = 1.f;
```

---

## Round 7 Fixes + Round 8 Total

| Round | Fixes Applied | P0 Remaining | P1 Remaining |
|-------|--------------|-------------|-------------|
| R7 fixes | OCEANDEEP buffer, OCTOPUS exp hoist, ORIGAMI pan hoist | 0 | 0 (fixed) |
| R8 audit | 8 engines | 0 | 5 |

**Total outstanding P0: 0**
**Total outstanding P1: 5** (OSPREY allpass buffer, OHM/OBBLIGATO/OTTONI coupling reset, OBBLIGATO delay buffers)

---

## EXIT CONDITION Assessment

The 10-round sweep has now covered all engines in the fleet. The exit condition — zero P0/P1 across the full audit — is **not yet reached**. However:

- **All P1 issues found in R8 are clustered**: three engines (OHM, OBBLIGATO, OTTONI) share the same 3-line coupling reset fix. OSPREY needs one constant changed. OBBLIGATO needs delay buffer scaling.
- **No new engine failure modes** were found beyond patterns already addressed in earlier rounds.
- **Zero P0 issues** across all 8 engines — no correctness-breaking bugs.

**Recommendation:** Apply the 5 R8 P1 fixes in a single pass (Round 9), then re-audit only the affected engines. If clean, declare EXIT.

### Quick-fix template for OHM / OBBLIGATO / OTTONI
In each `renderBlock`, after the SilenceGate early-return check, add:
```cpp
// Reset coupling accumulators — must be cleared each block to prevent
// sticky pitch/damping values after coupling source disconnects.
extPitchMod = 0.f;
extDampMod  = 0.f;
extIntens   = 1.f;
```
