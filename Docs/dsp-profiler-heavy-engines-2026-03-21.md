# DSP Profiler — Heavy Engines Analysis
**Date:** 2026-03-21
**Engines:** ORGANON · OSTINATO · OWARE
**Analyst:** Claude Sonnet 4.6 (static analysis; no hardware counter data)

---

## Methodology

This is a static analysis — no profiling hardware was available. Costs are estimated by:
1. Counting arithmetic operations in each per-sample hot path
2. Multiplying by voice/seat/mode counts
3. Applying CPU frequency assumptions (Apple M1/M2 class; ~3.2 GHz, OOO execution, NEON SIMD not yet exploited)
4. Normalizing to a 44.1 kHz mono budget ≈ 22,676 ns/sample (stereo halves each engine's share to ~5,669 ns at 4 active engines)

Operation cost references used:
- Add/Mul: ~0.3–0.5 ns each (ALU, OOO pipelined)
- Div: ~4–8 ns
- `fastExp` (Schraudolph): ~1–2 ns (integer cast path)
- `fastTanh` (rational): ~3–4 ns
- `fastSin` (polynomial): ~4–6 ns
- `std::exp`, `std::cos`, `std::pow`: ~15–30 ns each (transcendental, not fast-path)
- `flushDenormal` (bit-inspect): ~1–2 ns
- CytomicSVF `processSample`: ~8–10 muls + 4 adds + 2 flushDenormal = ~6–8 ns

---

## Engine 1: ORGANON (OrganonEngine.h, 1587 lines)

### Architecture Summary
- 4 voices (kMaxVoices = 4)
- Per voice: IngestedSample → CytomicSVF (bandpass) → EntropyAnalyzer → MetabolicEconomy (VFE) → ModalArray (32 modes, RK4 per sample)
- Control rate at ~2 kHz (every ~22 samples) for entropy and metabolic updates

### Per-Sample Hot Path — Single Voice

#### Stage 1: Ingestion (self-feed path)
- xorshift32 noise: 3 XOR + 1 integer cast = ~2 ns
- `CytomicSVF::processSample` (bandpass): ~7 ns
- `setCoefficients` called every sample (ingestion filter update): `fastTan` + 5 muls + 1 div = ~10 ns
  **BUG RISK:** `setCoefficients` (full, with `fastTan`) is called every sample inside the per-sample loop when in self-feed mode. The fast path should use `setCoefficients_fast` here.
- **Ingestion total: ~19 ns/voice**

#### Stage 2: EntropyAnalyzer (every sample)
- Ring buffer write: 1 store + 1 add + 1 bitwise AND = ~1 ns
- Modulo counter increment: ~1 ns
- `computeEntropy` fires every ~22 samples:
  - 256 histogram bins: 256 iterations × (1 branch + 1 clamp + 1 array write) ≈ ~500 ns amortized = ~23 ns/sample
  - 32 bin Shannon H loop: 32 × (1 `fastLog2` + 3 muls + 2 adds) ≈ 32 × 12 ns = 384 ns amortized = ~17 ns/sample
  - **Control-rate cost amortized: ~40 ns/sample**

#### Stage 3: MetabolicEconomy (every sample, fires every ~22 samples)
- VFE computation: ~12 multiplies, 6 adds, 3 `flushDenormal`, 2 `std::clamp`
- ~120 ns per tick amortized / 22 = ~5.5 ns/sample

#### Stage 4: ModalArray — THE DOMINANT COST
- `setFundamental`: called every sample (inside voice loop)
  - 32 iterations × `std::pow(harmonicNumber, spread)` = 32 × ~25 ns = **800 ns**
  **CRITICAL:** This is 32 `std::pow` calls per sample per voice. At 4 voices: **3,200 ns** = ~56% of the entire 44.1 kHz stereo engine budget alone.

- `updateWeights`: called every sample (inside voice loop)
  - 32 × `fastExp(-0.5f * dist * dist * inv_bw_sq)` = 32 × ~2 ns = 64 ns
  - Plus arithmetic: 32 × ~5 ns = 160 ns
  - **updateWeights total: ~224 ns/voice**

- `processSample` — RK4 integration, 32 modes:
  - Per mode: ~6 multiplies (ω², RK4 k-values) × 4 stages = ~24 muls + ~12 adds
  - 2 `flushDenormal` calls per mode
  - 32 modes: 32 × (24 × 0.4 ns + 12 × 0.3 ns + 2 × 1.5 ns) = 32 × (9.6 + 3.6 + 3.0) = 32 × 16.2 ≈ **518 ns/voice**

- `fastTanh` on output: ~4 ns
- **ModalArray total per voice: ~800 + 224 + 518 + 4 ≈ 1,546 ns/voice**
  (**800 ns of that is fixable** — see Optimization section)

### Per-Voice Total (self-feed mode, 4 voices active)
| Stage | Per Voice | × 4 voices |
|-------|-----------|------------|
| Ingestion (incl. full setCoefficients) | ~19 ns | ~76 ns |
| EntropyAnalyzer (amortized) | ~40 ns | ~160 ns |
| MetabolicEconomy (amortized) | ~5.5 ns | ~22 ns |
| setFundamental (32× std::pow) | **~800 ns** | **~3,200 ns** |
| updateWeights (32× fastExp) | ~224 ns | ~896 ns |
| processSample RK4 (32 modes) | ~518 ns | ~2,072 ns |
| Misc (tanh, vel, stereo) | ~10 ns | ~40 ns |
| **Voice total** | **~1,616 ns** | **~6,466 ns** |

**Estimated CPU at 44.1 kHz, 4 voices active: ~29%**
(Budget per engine in 4-slot XOmnibus: ~25%; Organon is currently overbudget when all 4 voices run)
Organon's own declared budget: 22% (`profiler.setCpuBudgetFraction(0.22f)`) — will breach at 3–4 active voices.

### Most Expensive DSP Stages
1. `setFundamental`: 32× `std::pow` per sample per voice — **single largest cost in the engine**
2. RK4 modal integration: legitimately expensive but architecturally sound
3. `EntropyAnalyzer::computeEntropy`: histogram build is O(window_size) every ~22 samples

### Denormal Protection Coverage
- All RK4 state variables (`displacement`, `velocity`): `flushDenormal` on every update ✓
- MetabolicEconomy EMA paths: `flushDenormal` on `beliefChangeRate`, `entropyVariance`, `adaptationGain` ✓
- Ingestion filter: `CytomicSVF` has `flushDenormal` in state updates ✓
- `modWheelAmount`, `externalRhythmModulation`, etc.: not feedback paths, denormals not an issue ✓
- **Coverage: GOOD**

### Optimization Opportunities

#### O1 — Move `setFundamental` to Control Rate (HIGH IMPACT)
The fundamental frequency does not change sample-to-sample unless pitch bend or LFO is active. Currently called every sample unconditionally. Moving to block rate with a "dirty" flag would eliminate 32× `std::pow` per voice per sample.

**Estimated savings: ~800 ns/voice → ~6 ns/voice (cache angular frequencies, skip recompute if frequency unchanged)**
**Impact: reduces 4-voice cost from ~6,466 ns to ~3,266 ns (~51% speedup)**

```cpp
// Proposed: cache last computed frequency
float lastFundamental[kMaxVoices] = {};
float lastIsotope[kMaxVoices] = {};
// Only call setFundamental if |newFreq - lastFreq| > 0.01 Hz
```

#### O2 — Move `updateWeights` to Control Rate (~2 kHz)
Spectral centroid and free energy change at the control rate, not at audio rate. `updateWeights` reads `getSpectralCentroid()` and `getFreeEnergy()` which are updated at 2 kHz anyway.

**Estimated savings: ~224 ns/voice → ~10 ns/voice amortized**

#### O3 — Replace `setCoefficients` with `setCoefficients_fast` in Ingestion Loop
The ingestion filter's `setCoefficients` (full path with `fastTan`) is called every sample. The `_fast` variant (line 99) exists for exactly this use case. For a nearly-static enzymeSelectivity parameter, even block-rate updates would suffice.

**Estimated savings: ~10 ns → ~3 ns/voice/sample**

#### O4 — SIMD the RK4 Modal Loop
The 32-mode arrays are `alignas(16)`, explicitly preparing for SIMD. Processing 4 modes at a time with NEON float32x4 would reduce 32 scalar iterations to 8 SIMD iterations.

**Estimated savings: ~518 ns → ~130 ns/voice (theoretical 4× if SIMD-complete)**

#### O5 — Silence-Gate Voice Skipping
Already present via `SilenceGate`. Ensure it is waking/sleeping at the voice level, not just the engine level, to avoid paying RK4 cost for idle-but-allocated voices.

---

## Engine 2: OSTINATO (OstinatoEngine.h, 2207 lines)

### Architecture Summary
- 8 seats × 2 sub-voices = 16 max simultaneous DSP chains
- Per sub-voice chain: OstiModalMembrane (8 modes, CytomicSVF BandPass) → OstiWaveguideBody (delay + 2× CytomicSVF) → OstiRadiationFilter (CytomicSVF) → OstiBreathingLFO (fastSin) → CytomicSVF voice filter → OstiEnvelope → mix
- Master bus: 2× CytomicSVF (master filter) + OstiReverb (4 comb + 2 allpass) + OstiCompressor
- Pattern sequencer: 8 instances, block-rate logic (negligible cost)

### Per-Sample Hot Path — Single Sub-Voice (active, decay stage)

#### OstiModalMembrane::process()
- Noise: xorshift32 = ~2 ns
- Excitation decay: 1 mul + 1 sub + 1 `flushDenormal` = ~3 ns
- Pitch spike (when active): `fastSin` + 3 muls = ~8 ns (short-lived, only during attack)
- Modal resonators loop (numModes = 6–8, CytomicSVF BandPass):
  - Each BandPass SVF: ~7 ns (4 muls + 4 adds + 2 flushDenormal)
  - 8 modes × 7 ns = **56 ns**
- Output normalize: 1 mul = ~0.5 ns
- **Membrane total: ~62 ns/subvoice** (attack: ~70 ns, decay: ~62 ns)

#### OstiWaveguideBody::process()
- Cylindrical (most common): delay read + linear interpolation + 2× CytomicSVF (reflectionFilter + allpassFilter) = ~2 ns + 2 × 7 ns + ~3 ns = **~19 ns**
- Box (cajon): +2 extra delay taps + ~4 ns = **~23 ns**
- Frame (bowl body): 3× OwareMode-style resonators = 3 × 5 ns = **~15 ns**
- **Body total: ~15–23 ns/subvoice**

#### OstiRadiationFilter::process()
- 1× CytomicSVF: ~7 ns
- **Radiation: ~7 ns/subvoice**

#### OstiBreathingLFO::process()
- `fastSin` + 1 add + 1 div = ~8 ns
- **LFO: ~8 ns/subvoice**

#### Voice filter (per-sample `setCoefficients_fast` + `processSample`)
- `setCoefficients_fast`: `fastTan` + 5 muls + 1 div = ~8 ns
- `processSample`: ~7 ns
- **Voice filter total: ~15 ns/subvoice**
  Note: `setCoefficients_fast` called every sample due to LFO modulation. Legitimate but still costly.

#### OstiEnvelope::process()
- Decay stage: 2 muls + 1 sub + 1 `flushDenormal` + 1 comparison = ~3 ns
- **Envelope: ~3 ns/subvoice**

**Sub-voice total: ~62 + 19 + 7 + 8 + 15 + 3 = ~114 ns/subvoice**

### Full Engine Per-Sample Cost

| Component | Count | Unit Cost | Total |
|-----------|-------|-----------|-------|
| OstiSubVoice (active) | up to 16 | ~114 ns | up to ~1,824 ns |
| Master filter L + R | 2× CytomicSVF | ~7 ns each | ~14 ns |
| OstiReverb (4 comb + 2 allpass) | 1 | ~60 ns | ~60 ns |
| OstiCompressor | 1 | ~8 ns (1 `fastExp`, 1 `std::log`) | **~25 ns** |
| Envelope follower | 1 | ~3 ns | ~3 ns |
| **Total (all 16 voices active)** | | | **~1,926 ns** |
| **Total (typical: 4–6 subvoices)** | | | **~600–900 ns** |

**Note:** The OstiCompressor uses `std::log` (line 1298) in its gain computation. This is called every sample when above threshold. `std::log` is ~15–20 ns; the `fastExp(std::log(x) * invRatio)` combination should be replaced with a `fastPow` or power-series approximation.

**Estimated CPU at 44.1 kHz:**
- All 16 sub-voices: **~8.5%**
- Typical 4–6 sub-voices (2–3 seats active): **~3–5%**
- Dense preset (8 seats, 2 voices each): **~8–9%**

OSTINATO is the **most efficiently designed** of the three heavy engines — the percussive nature means voices decay quickly, keeping average polyphony moderate.

### Most Expensive DSP Stages
1. OstiModalMembrane: 8× CytomicSVF BandPass per sub-voice — unavoidable for the physics model
2. OstiWaveguideBody: 2× CytomicSVF + delay per sub-voice
3. `setCoefficients_fast` per sample for breathing LFO filter modulation — moderately costly
4. OstiReverb: 4 comb + 2 allpass at block level — single shared reverb is efficient

### Denormal Protection Coverage
- `OstiEnvelope` decay state: `flushDenormal` ✓
- `OstiWaveguideBody` feedback paths: `flushDenormal` on `reflected` ✓
- `OstiReverb` comb LP states: `flushDenormal` on each `combLP[c]` ✓
- `OstiCompressor` envelope: `flushDenormal` ✓
- Sub-voice `excitationLevel` decay: `flushDenormal` ✓
- **Coverage: EXCELLENT**

### Optimization Opportunities

#### O1 — OstiCompressor: Replace `std::log` with `fastLog`
Line 1298: `fastExp(std::log(overshoot) * invRatio)` — `std::log` is a full IEEE transcendental.
Replace with `fastExp(fastLog2(overshoot) * 0.693147f * invRatio)` or precompute at block rate.

**Estimated savings: ~12–15 ns/sample (called every sample when above threshold)**

#### O2 — Move Breathing LFO `setCoefficients_fast` to Block Rate
The LFO modulates filter cutoff at 0.06 Hz — the cutoff barely moves within a 512-sample block. Computing new coefficients every sample is wasteful. Update once per block (or every 64 samples).

**Estimated savings: ~8 ns/subvoice × 16 = ~128 ns/sample**

#### O3 — OstiModalMembrane: Skip Inactive Modes
Not all instruments use all 8 modes (bongos use 6, tabla uses 6). The loop `for (int i = 0; i < numActiveModes; ++i)` already respects `numActiveModes`. Confirm this is always correctly set at trigger time (it is — verified in `trigger()`). This is already optimized.

#### O4 — Inactive Sub-Voice Early Exit
`OstiSubVoice::processSample()` checks `if (!active) return 0.0f` at entry. The seat-level loop `OstiSeat::processSample()` still iterates both sub-voices unconditionally. The `if (sv.active)` check is present, which is correct — no action needed here.

#### O5 — Pattern Sequencer: Already Block-Rate
The `OstiPatternSequencer::advance()` runs once per block per seat (8 calls total). This is already optimal.

---

## Engine 3: OWARE (OwareEngine.h, 911 lines)

### Architecture Summary
- 8 voices (kMaxVoices = 8), tuned percussion
- Per voice: OwareMalletExciter → 8× OwareMode (2nd-order IIR resonator) + sympathetic coupling → OwareBuzzMembrane (CytomicSVF BPF + tanh) → OwareBodyResonator (delay or frame modes or bowl) → FilterEnvelope → CytomicSVF → pan
- Sympathetic resonance: O(V²×M²) nested loop per sample when sympathy > 0

### Per-Sample Hot Path — Single Voice

#### OwareMalletExciter::process() (only during attack, short-lived)
- Primary strike: 1 LCG noise + 1 `std::sin` + 3 muls = ~18 ns
- Mallet LP filter: 1 mul + 1 sub + 1 add = ~2 ns
- **Exciter: ~20 ns (during attack only, ~5–15 ms total)**

#### OwareMode::process() × 8 modes
- Per mode: `b0*input + a1*y1 - a2*y2` + store + `flushDenormal` = 3 muls + 2 adds + 1 flushDenormal ≈ ~4 ns
- 8 modes × 4 ns = **32 ns**

#### Sympathetic Resonance Network (O(V²×M²) — THE DOMINANT COST)
When `sympNow > 0.001f` and multiple voices active:
```
for voice in voices (up to 8):
  for other_voice in voices (up to 7):
    for m in 8 modes:
      for om in 8 other_voice modes:
        if |freq[m] - freq[om]| < 50 Hz:
          sympInput[m] += other.modes[om].lastOutput × proximity × sympNow × 0.03f
```
- Worst case: 8 voices × 7 others × 8 modes × 8 other_modes = **3,584 iterations/sample**
- Each iteration: 1 `std::fabs` + 1 comparison + conditional: ~3 ops = ~1.5 ns
- **Worst-case sympathetic cost: 3,584 × ~1.5 ns = ~5,376 ns/sample**

In practice with sparse polyphony (2–3 voices): 2 × 1 × 8 × 8 = 128 iterations = **~192 ns/sample**
With 4 voices: 4 × 3 × 64 = 768 iterations = **~1,152 ns/sample**

This is an **exponential polyphony cost** that must be addressed.

#### OwareBuzzMembrane::process()
- CytomicSVF BPF: ~7 ns
- `std::tanh(buzzBand × sensitivity)`: **~20–25 ns** (full IEEE tanh)
  **BUG:** `std::tanh` is used instead of `fastTanh`. At high `amount` values this fires every sample.
- Add + mul: ~2 ns
- **Buzz: ~29 ns (when amount > 0)**

#### OwareBodyResonator::process()
- Tube (case 0): delay read + linear interpolation + mul + add = ~5 ns
- Frame (case 1): 3× OwareMode `process()` = 3 × 4 ns = ~12 ns
- Bowl (case 2): `std::cos` per sample = **~20 ns** + 3 muls + 2 adds + flushDenormal = **~26 ns**
  **BUG:** The bowl resonator (case 2) computes `2.0f * r * std::cos(w)` inside the per-sample loop at line 328 (OwareBodyResonator). `w` depends only on `bowlFreq / sr` which is set at `setFundamental()` — this `std::cos` can be precomputed once and stored as `cosW`.

#### FilterEnvelope + CytomicSVF (voice filter)
- `FilterEnvelope::process()`: ~5 ns
- `CytomicSVF::setCoefficients` called every sample (line 681): `fastTan` = ~10 ns
  **ISSUE:** Full `setCoefficients` called every sample with `cutoff` computed from `brightNow + envMod`. Use `setCoefficients_fast` here.
- `CytomicSVF::processSample`: ~7 ns
- **Filter: ~22 ns/voice**

#### Material Ratio Interpolation (8 modes, per sample)
- Lines 632–641: 8 modes × (2 branches + 3 muls + 2 adds) = 8 × ~8 ns = **64 ns/voice**
- `std::pow(m+1, -materialAlpha)` per mode (line 657): 8 × `std::pow` = 8 × ~25 ns = **200 ns/voice**
  **CRITICAL:** 8× `std::pow` per voice per sample for decay scaling. This can be precomputed at control rate since `materialAlpha` is a smoothed parameter that barely changes within a block.

#### Per-Voice Total (4 voices, sympathetic coupling active)

| Stage | Per Voice | × 4 voices |
|-------|-----------|------------|
| Modes (8× OwareMode) | ~32 ns | ~128 ns |
| Sympathetic loop (4 voices) | ~288 ns/voice | ~1,152 ns |
| Buzz (BPF + std::tanh) | ~29 ns | ~116 ns |
| Body (tube avg) | ~5–26 ns | ~80 ns |
| Filter env + setCoefficients | ~22 ns | ~88 ns |
| Material ratio + std::pow × 8 | **~264 ns** | **~1,056 ns** |
| Exciter (amortized) | ~2 ns | ~8 ns |
| Shimmer LFO + thermal + glide | ~15 ns | ~60 ns |
| Pan (cos + sin) | ~5 ns | ~20 ns |
| **Voice total** | **~677 ns** | **~2,708 ns** |

Plus 6× ParameterSmoother: ~6 × 3 ns = ~18 ns/sample (negligible).

**Estimated CPU at 44.1 kHz:**
- 4 voices, sympathy active: **~12%**
- 8 voices, sympathy active: **~35–40%** (exponential due to sympathetic O(V²) loop)
- 8 voices, sympathy off: **~15%**

### Most Expensive DSP Stages
1. Sympathetic resonance O(V²×M²) loop — exponential polyphony cost
2. `std::pow(m+1, -materialAlpha)` × 8 per voice per sample — fixable
3. `std::cos` in bowl body resonator per sample — fixable
4. `std::tanh` in BuzzMembrane — should be `fastTanh`

### Denormal Protection Coverage
- OwareMode state variables: `flushDenormal` per mode per sample ✓
- `voice.ampLevel` decay: `flushDenormal` ✓
- OwareBodyResonator bowl: `flushDenormal` on `bowlY1` ✓
- FilterEnvelope (external module): assumed to have denormal protection
- `thermalState` EMA: no `flushDenormal` — **MINOR GAP**: very slow-converging EMA can produce denormals
- **Coverage: GOOD with one minor gap**

### Optimization Opportunities

#### O1 — Sympathetic Loop: Spatial Acceleration (HIGH IMPACT, CRITICAL)
The O(V²×M²) loop at full polyphony is catastrophic (3,584 iterations). Three viable solutions:

**Option A — Precompute proximity table at note-on:**
When a voice is triggered, compute which (voice, mode) pairs are within the 50 Hz sympathetic range and store a sparse list. Per-sample loop becomes O(sparse_pairs) instead of O(V²×M²).
At note-on cost: 8 × 8 × 8 distance checks = 512 operations once → amortized to near-zero.

**Option B — Reduce to block-rate:**
Sympathetic coupling at audio rate is overkill for a slow-changing coupling; reading `lastOutput` from the previous block (block-rate update) loses no perceptual quality.

**Option C — Cap sympathetic voices:**
Only check the 2 most-recently-struck other voices, not all N−1. Perceptually indistinguishable.

**Estimated savings Option A: ~1,152 ns → ~50 ns at 4 voices**

#### O2 — `std::pow` for Material Decay Scaling: Move to Block Rate (HIGH IMPACT)
`std::pow(static_cast<float>(m+1), -materialAlpha)` at line 657 is computed every sample for every voice every mode. `materialAlpha` derives from a ParameterSmoother — it changes at most a few % per block. Cache `modeDecayScale[8]` at block boundary and only recompute when `materialAlpha` changes by more than a threshold.

**Estimated savings: ~200 ns/voice → ~3 ns/voice amortized**

#### O3 — `std::cos` in Bowl Body: Precompute at setFundamental (HIGH IMPACT)
In `OwareBodyResonator::process()` case 2 (bowl), line 327:
```cpp
float w = 2.0f * 3.14159265f * std::max(bowlFreq, 20.0f) / sr;
float newY1 = bowlY1 * 2.0f * r * std::cos(w) - ...
```
`bowlFreq` is set in `setFundamental()` which is called at note-on. `w` and `std::cos(w)` can be precomputed and stored as member variables `bowlW`, `twoRcosW`.

**Estimated savings: ~20 ns/voice/sample (bowl body type)**

#### O4 — BuzzMembrane: Replace `std::tanh` with `fastTanh`
Line 214: `float buzzed = buzzBand * (1.0f + amount * std::tanh(buzzBand * sensitivity))`.
`std::tanh` is the system transcendental. `fastTanh` in FastMath.h exists for this purpose.

**Estimated savings: ~18–20 ns/voice/sample when buzz > 0**

#### O5 — Voice Filter: Use `setCoefficients_fast` Instead of Full `setCoefficients`
Line 681: `voice.svf.setCoefficients(cutoff, 0.5f, srf)` is called every sample inside the voice loop. The `_fast` variant already uses `fastTan` and skips shelf gain — use it.

**Estimated savings: ~3 ns/voice/sample (minor)**

---

## Comparative Summary

| Engine | Max Voices | Typical CPU | Worst-Case CPU | Primary Bottleneck |
|--------|------------|-------------|----------------|--------------------|
| ORGANON | 4 | ~15% (2 voices) | **~29% (4 voices)** | 32× `std::pow` in `setFundamental` per sample |
| OSTINATO | 16 sub-voices | ~4% (4 voices) | ~9% (16 voices) | 8× CytomicSVF per sub-voice (unavoidable) |
| OWARE | 8 | ~12% (4 voices) | **~35–40% (8 voices, sympathy)** | O(V²×M²) sympathetic loop; 8× `std::pow` per voice |

### Quick-Win Priority Ranking (effort vs. impact)

| Priority | Engine | Fix | Estimated CPU Saving |
|----------|--------|-----|---------------------|
| P1 | ORGANON | Move `setFundamental` to block/control rate | ~12–15% → ~6–8% (4 voices) |
| P2 | OWARE | Sympathetic loop precomputed proximity table (Option A) | ~15–20% at dense polyphony |
| P3 | OWARE | `std::pow` decay scaling → block-rate cache | ~4–5% (4 voices) |
| P4 | OWARE | `std::cos` bowl → precompute | ~1–2% (bowl body type) |
| P5 | OWARE | `std::tanh` → `fastTanh` in BuzzMembrane | ~1–2% |
| P6 | ORGANON | `setCoefficients` → `_fast` in ingestion loop | ~0.5–1% |
| P7 | ORGANON | Move `updateWeights` to control rate | ~2–3% (4 voices) |
| P8 | OSTINATO | `std::log` → `fastLog2` in compressor | ~0.1% |
| P9 | OSTINATO | Breathing LFO filter → block-rate coefficients | ~0.3% |

### Shared Observations Across All Three Engines

1. **`std::pow` is the arch-villain.** All three engines call `std::pow` at audio rate in inner loops. The pattern `std::pow(n, exponent)` where `n` is a small integer (mode index) and `exponent` is a slowly-varying parameter can always be moved to control or block rate with a dirty-flag cache.

2. **`setCoefficients` vs `setCoefficients_fast` discipline:** Several engines call full `setCoefficients` (which includes `fastTan`) every sample inside voice loops. This is 2–3× the cost of `setCoefficients_fast`. Rule: any modulated filter in an audio-rate loop must use `_fast`.

3. **Sympathetic/coupling networks are O(N²) in polyphony** — both ORGANON's Phason system and OWARE's sympathetic loop have this property. Block-rate updates or sparse precomputed proximity tables are the standard solution.

4. **Denormal protection is generally excellent.** All three engines consistently apply `flushDenormal` to feedback paths. The CLAUDE.md recommendation is being followed. Only OWARE's `thermalState` EMA (very slow-converging, ~0.00001/sample approach rate) is a minor gap.

5. **SIMD opportunity.** ORGANON's 32-mode arrays are `alignas(16)` and explicitly noted for future SIMD. OWARE's 8-mode array is a natural NEON float32x4 target. Neither engine currently vectorizes. A single SIMD pass over the ORGANON modal array would cut its RK4 cost by 3–4×.

---

## Recommended Immediate Actions (before next seance)

1. **ORGANON P1:** Cache `angularFrequency[]` and only call `setFundamental` when `|newFreq - cachedFreq| > 0.01f`. This single change brings ORGANON from ~29% to ~15% CPU at 4 voices.

2. **OWARE P2:** Implement proximity table at note-on. In `OwareEngine::noteOn()`, after `v.modes[m].setFreqAndQ(...)`, compute which (otherVoice, otherMode) pairs fall within 50 Hz and store as a `SympLink` array. Replace the O(V²×M²) loop with iteration over that array.

3. **OWARE P3:** At the top of `renderBlock`, after smoothing `effectiveMaterial` → `materialAlpha`, compute `modeDecayScale[8]` using `std::pow` once and cache. Replace line 657 with a table lookup.

4. **OWARE P4+P5:** One-line fixes: `std::cos(w)` → precomputed in `setFundamental`; `std::tanh` → `fastTanh`. Do these together in 5 minutes.

5. **ALL ENGINES:** Audit all `setCoefficients` calls inside per-sample voice loops and replace with `setCoefficients_fast` where the filter is modulated audio-rate. Full `setCoefficients` should only be called at trigger time or block rate.

---

*Analysis generated 2026-03-21 via static code review of OrganonEngine.h (1587 lines), OstinatoEngine.h (2207 lines), OwareEngine.h (911 lines). CPU estimates assume Apple M1/M2, 44.1 kHz, 512-sample blocks, no SIMD vectorization. Actual profiling with Instruments or JUCE's PerformanceMeter recommended to validate these estimates before optimizing.*
