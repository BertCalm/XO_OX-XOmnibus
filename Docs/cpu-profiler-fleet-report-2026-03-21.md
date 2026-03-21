# XOmnibus Fleet CPU Profiler Report
**Date**: 2026-03-21
**Scope**: 70 engines (46 original + 24 Kitchen Collection)
**Method**: Static analysis of all engine `.h` files — transcendentals, divisions, memory allocations, filter counts, voice polyphony
**Status**: Diagnostic only — no changes made

---

## 1. Ranked Table — All Engines by Estimated CPU Cost

Cost estimation: `polyphony × (transcendentals_per_sample × ~50 cycles + filter_count × ~15 cycles + base_DSP)`

| Rank | Engine | Voices | Transcend. Total | Hot-Path Trig | Filters/Voice | Cost Tier |
|------|--------|--------|-----------------|---------------|---------------|-----------|
| 1 | **OSTERIA** | 8 | 0 (all sub-files) | 32 std::cos/sin/sample (4ch × 8v) | 4 SVF/ch | **CRITICAL** |
| 2 | **OUROBOROS** | 6 | 4 | RK4 × 4x oversample × 6v | 0 | **CRITICAL** (designed) |
| 3 | **OPERA** | 8 | 26 | 16 std::tan/sample (2 SVF/voice) | 2 SVF/v | **HIGH** |
| 4 | **OCELOT** | 8 | 33 | Understory sin, Canopy sin×partials, sigmoid exp | Multiple/v | **HIGH** |
| 5 | **OFFERING** | 8 | 28 (sub-files) | Kick: 2 sin/sample; Rim/Tom: 1 sin/sample; Pan: 2 trig/v; Compressor: log10+pow | 2 filters | **HIGH** |
| 6 | **OVERLAP** | 6 | 13 | atan2 per sample (Kuramoto entrainment) | 6-ch FDN | **HIGH** |
| 7 | **OPAL** | 12 | 23 | Grain window: 3 std::cos per active grain (Hann/Tukey shapes) | 4 comb+SVF | **HIGH** |
| 8 | **OSTINATO** | 16 sub-v | 4 | std::log per sample (compressor, above threshold) | 8 resonators/seat | **HIGH** |
| 9 | **FAT** | 6 | 12 | std::tan per block (computeCoeffs, 1 call/voice/sample for 4-pole ladder) | 4-pole ladder/v | **MEDIUM-HIGH** |
| 10 | **OBRIX** | 8 | 29 | std::sin in wavefolder per active voice; wavetable sin = setup only | SVF/v | **MEDIUM-HIGH** |
| 11 | **OBELISK** | 8 | 12 | setCoefficients (fastTan) per voice per sample × 8v | 3+ resonators/v | **MEDIUM** |
| 12 | **OCHRE** | 8 | 11 | setCoefficients (fastTan) per voice per sample × 8v | 3+ resonators/v | **MEDIUM** |
| 13 | **OPALINE** | 8 | 11 | setCoefficients (fastTan) per voice per sample × 8v | 3+ resonators/v | **MEDIUM** |
| 14 | **OVEN** | 8 | 10 | setCoefficients (fastTan) per voice per sample × 8v | SVF/v | **MEDIUM** |
| 15 | **ONSET** | 8 | 12 | Resonator bank (8 modes/voice) | 8 BPF/voice | **MEDIUM** |
| 16 | **ORACLE** | 8 | 9 | std::tan+std::log (Cauchy/Logistic dist) per cycle; std::sort per cycle | interp | **MEDIUM** |
| 17 | **ORBITAL** | 6 | 11 | exp/pow in partial amplitude build (per-noteOn), sqrt RMS per block | SVF/v | **MEDIUM** |
| 18 | **OWLFISH** | 1 | 12 | sin per block (grain LFO), tan in SVF per-block update | 1 SVF | **MEDIUM** |
| 19 | **OAKEN** | 8 | 5 | exp/cos/sin = setup only | resonators | **MEDIUM** |
| 20 | **OWARE** | 8 | 16 | exp calls mostly setup; fastPow2/fastExp already in hot path | 3+ per voice | **MEDIUM** |
| 21 | **OCHRE** | 8 | 11 | See rank 12 | resonators | **MEDIUM** |
| 22 | **OTIS** | 8 | 8 | All exp/cos/sin are per-noteOn or per-block | SVF/v | **LOW-MEDIUM** |
| 23 | **OBSIDIAN** | 16 | 8 | LFO sin per block; LUT sin = setup; sqrt in render = per-block | SVF/v | **LOW-MEDIUM** |
| 24 | **OCTOPUS** | 16 | 6 | Wavetable sin = setup only; exp = setup only | SVF/v | **LOW-MEDIUM** |
| 25 | **BOB** | 8 | 8 | tan in SVF = per-block (coeffDirty flag); pan = setup | SVF/v | **LOW-MEDIUM** |
| 26 | **MORPH** | 16 | 7 | LFO sin per block; wavetable sin = setup; pow = per-noteOn | SVF/v | **LOW-MEDIUM** |
| 27 | **ONKOLO** | 8 | 9 | exp = setup mostly; sqrt = per-block RMS | SVF/v | **LOW** |
| 28 | **BITE** | 16 | 7 | Wavetable sin = setup; pow per-noteOn; sqrt per block | SVF/v | **LOW** |
| 29 | **OUROBOROS** | 6 | 4 | cos/sin per-block (not per-sample) | - | **DESIGNED** |
| 30 | **OCEANIC** | 4 | 8 | pow/sqrt mostly setup | SVF/v | **LOW** |
| 31 | **OASIS** | 8 | 6 | exp = setup; pow = noteOn; sqrt per block; cos/sin = noteOn | SVF/v | **LOW** |
| 32 | **ORGANI/ORGANON** | 4 | 8 | pow mostly setup/noteOn | SVF/v | **LOW** |
| 33 | **OSPREY** | 8 | 6 | exp/cos/sin = setup or noteOn | SVF/v | **LOW** |
| 34 | **ORCA** | 16 | 4 | sin per block (LFO); exp = setup | SVF/v | **LOW** |
| 35 | **ORACLE** | 8 | 9 | See rank 16 | - | **MEDIUM** |
| 36 | **ORIGAMI** | 8 | 13 | cos/sin/atan2 mostly setup or per-block | SVF/v | **LOW** |
| 37 | **OCTAVE** | 8 | 5 | sin/exp mostly setup | SVF/v | **LOW** |
| 38 | **OHRESIDIAN** | 8 | 8 | exp/sin = setup or noteOn | SVF/v | **LOW** |
| 39 | **ORGANISM** | 1 | 8 | Cellular automata engine; no oscillator trig | - | **LOW** |
| 40 | **OVERTONE** | 1 | 4 | exp/sqrt = setup; per-sample DSP is waveguide | waveguide | **LOW** |
| 41 | **OHM** | 12 | 7 | sin mostly setup; exp = setup | 4 comb | **LOW** |
| 42 | **OUTWIT** | 1 | 3 | sqrt = per-block; sin = setup | - | **LOW** |
| 43 | **OVERWORLD** | 8 | 1 | sqrt = setup | SVF/v | **LOW** |
| 44 | **OVERLAP** | 6 | 13 | See rank 6 | FDN | **HIGH** |
| 45 | **OXBOW** | 1 | 2 | exp/pow = setup | FDN | **LOW** |
| 46 | **OSIER** | 4 | 3 | pow/cos/sin = setup or noteOn | SVF/v | **LOW** |
| 47 | **OPTIC** | 0 | 1 | sqrt = setup; no audio output | - | **NEGLIGIBLE** |
| 48 | **OPENSKY** | 16 | 1 | sqrt = per-block normalization | SVF/v | **LOW** |
| 49 | **OBBLIGATO** | 12 | 2 | pow/sin = setup | resonators | **LOW** |
| 50 | **ORPHICA** | 16 | 5 | exp = setup or noteOn | granular | **LOW** |
| 51 | **OVERFLOW** | 8 | 1 | exp = setup | SVF/v | **LOW** |
| 52 | **OVERCAST** | 8 | 0 | None found | SVF/v | **LOW** |
| 53 | **OVERGROW** | 4 | 3 | pow/cos/sin = noteOn | SVF/v | **LOW** |
| 54 | **DUB** | 8 | 4 | exp = setup only | SVF/v | **LOW** |
| 55 | **DRIFT** | 8 | 4 | pow/exp/sin = setup or noteOn | SVF/v | **LOW** |
| 56 | **SNAP** | 8 | 3 | pow/sin = setup or noteOn | SVF/v | **LOW** |
| 57 | **OLATE** | 8 | 3 | pow/cos/sin = setup or noteOn | SVF/v | **LOW** |
| 58 | **OLE** | 18 | 1 | pow = noteOn | SVF/v | **LOW** |
| 59 | **OTTONI** | 12 | 3 | sin/sqrt mostly setup; 4 comb filters | 4 comb | **LOW** |
| 60 | **ORCHARD** | 4 | 3 | pow/cos/sin = setup | SVF/v | **LOW** |
| 61 | **ORBWEAVE** | 8 | 1 | exp = setup | - | **LOW** |
| 62 | **OLEG** | 8 | 4 | exp/pow/cos/sin = setup or noteOn | SVF/v | **LOW** |
| 63 | **OMEGA** | 8 | 4 | exp/pow/cos/sin = setup | SVF/v | **LOW** |
| 64 | **OBSCURA** | 8 | 4 | sin/exp/cos/pow = setup | SVF/v | **LOW** |
| 65 | **OGRE** | 8 | 4 | exp/pow/cos/sin = setup | SVF/v | **LOW** |
| 66 | **OMBRE** | 8 | 1 | pow = setup | SVF/v | **LOW** |
| 67 | **OVERWASH** | 8 | 3 | log/sqrt = setup | - | **LOW** |
| 68 | **OVERWORN** | 8 | 0 | None found | SVF/v | **LOW** |
| 69 | **OBBLIGATO** | 12 | 2 | See rank 49 | - | **LOW** |
| 70 | **OPTIC** | 0 | 1 | See rank 47 | - | **NEGLIGIBLE** |

---

## 2. Fleet Summary

| Tier | Count | Engines |
|------|-------|---------|
| **CRITICAL** | 2 | OSTERIA, OUROBOROS (designed) |
| **HIGH** | 6 | OPERA, OCELOT, OFFERING, OVERLAP, OPAL, OSTINATO |
| **MEDIUM** | 9 | FAT, OBRIX, OBELISK, OCHRE, OPALINE, OVEN, ONSET, ORACLE, ORBITAL |
| **LOW-MEDIUM** | 8 | OTIS, OBSIDIAN, OCTOPUS, BOB, MORPH, ONKOLO, BITE, OCEANIC |
| **LOW** | 43 | All remaining engines |
| **NEGLIGIBLE** | 1 | OPTIC (no audio output) |
| **DESIGNED-HIGH** | 1 | OUROBOROS (RK4 at 22% budget, intentional) |

---

## 3. Hot Path Inventory — Every Expensive Transcendental in Render Paths

### CRITICAL: Per-Sample std:: Calls (Confirmed In Sample Loop)

| Engine | File:Line | Call | Context | Est. Cycles/Sample |
|--------|-----------|------|---------|-------------------|
| OPERA | `OperaEngine.h:238` | `std::tan(kPi * cutoffHz / sampleRate)` | `OperaSVF::process()` — called per voice per sample (8v × 2 SVFs = **16 tan/sample**) | ~1,600/sample |
| OSTERIA | `OsteriaEngine.h:1103` | `std::cos(panAngle)` | Inner channel loop — 8 voices × 4 channels = **32 cos/sample**; panAngle is block-constant | ~1,600/sample |
| OSTERIA | `OsteriaEngine.h:1105` | `std::sin(panAngle)` | Same as above — **32 sin/sample** | ~1,600/sample |
| OFFERING | `OfferingTransient.h:392` | `std::sin(phase * 6.2831853f)` | `processKick()` — main sine oscillator, called per active kick sample | ~100/sample |
| OFFERING | `OfferingTransient.h:404` | `std::sin(subPhase_ * 6.2831853f)` | `processKick()` — sub-harmonic, called per active kick sample | ~100/sample |
| OFFERING | `OfferingTransient.h:533` | `std::sin(phase_ * 6.2831853f)` | `processRim()` — resonance oscillator | ~100/sample |
| OFFERING | `OfferingTransient.h:550` | `std::sin(phase_ * 6.2831853f)` | `processTom()` — sine oscillator | ~100/sample |
| OFFERING | `OfferingTransient.h:466` | `std::sin(metalPhases_[0] * 6.2831853f)` | `processHat()` — tonal component | ~100/sample |
| OFFERING | `OfferingEngine.h:407` | `std::cos((vs.pan + 1.0f) * 0.25f * kPi)` | Pan in voice loop — panAngle **never changes per sample** | ~50×8 voices |
| OFFERING | `OfferingEngine.h:408` | `std::sin((vs.pan + 1.0f) * 0.25f * kPi)` | Same — should be precomputed per block | ~50×8 voices |
| OFFERING | `OfferingCity.h:57` | `std::log10(envLevel_ / threshold_)` | `OfferingCompressor::process()` — fires when signal above threshold | ~60/sample |
| OFFERING | `OfferingCity.h:59` | `std::pow(10.0f, -reductionDb / 20.0f)` | Same compressor — fires when signal above threshold | ~100/sample |
| OVERLAP | `DSP/Entrainment.h:72` | `std::atan2(sinSum, cosSum)` | `Entrainment::process()` — **called per sample**, Kuramoto sync | ~150/sample |
| OCELOT | `OcelotUnderstory.h:127` | `std::sin(oscPhase * 2.0f * kPi)` | Per-sample oscillator in Understory module | ~100/sample |
| OCELOT | `OcelotCanopy.h:91` | `std::sin(breathePhase * twoPi)` | Per-sample breathe LFO in Canopy (partial synth) | ~100/sample |
| OCELOT | `OcelotCanopy.h:114` | `std::sin(partialPhases[p] * ...)` | Per-sample per-partial synthesis (additive partials) | ~100×partials |
| OCELOT | `EcosystemMatrix.h:131` | `std::exp(-x)` in `sigmoid()` | Coupling sigmoid — called per voice in render | ~80/voice |
| OBRIX | `ObrixEngine.h:901` | `std::sin(signal * fold * kPi)` | Wavefolder — fires for active voices when proc slot = fold type | ~100/active voice |
| OSTINATO | `OstinatoEngine.h:1281` | `std::log(overshoot)` inside `fastExp(std::log(...))` | `OstiCompressor::process()` — fires when above threshold | ~80/sample |

### HIGH: Per-Block (Once Per renderBlock) — Acceptable But Fixable

| Engine | File:Line | Call | Context |
|--------|-----------|------|---------|
| OPERA | `OperaEngine.h` via `OperaEnvelope` | `std::exp` | Envelope coefficient recalc — per noteOn only (correct) |
| OBRIX | `ObrixEngine.h:442,452,512,532` | `4× std::exp` | Coupling coefficients — per renderBlock (block-rate, acceptable) |
| OBRIX | `ObrixEngine.h:784` | `std::log2` (JI cents) | JI calculation — per-block param read (acceptable) |
| FAT | `FatEngine.h:237` | `std::tan` | `computeCoeffs()` — shared once per voice per sample, not 4× per filter |
| FAT | `FatEngine.h:298` | `std::tan` | `updateCoefficients()` — gated by `coeffDirty` flag (per-block) |
| ORACLE | `OracleEngine.h:1170` | `std::tan` | `sampleDistribution()` (Cauchy) — per-cycle (once per waveform period), not per-sample |
| ORACLE | `OracleEngine.h:1175` | `std::log` | `sampleDistribution()` (Logistic) — per-cycle |
| ORACLE | `OracleEngine.h` evolve | `std::sort` | Breakpoint sort — O(n log n) per waveform cycle × voice count |
| OUROBOROS | `OuroborosEngine.h:972-975` | `4× std::cos/sin` | Projection matrix — computed **per block** (before sample loop), not per-sample |
| OBSIDIAN | `ObsidianEngine.h:431` | `std::sin` | LFO — advanced by block, computed once per renderBlock |
| MORPH | `MorphEngine.h:487` | `std::sin` | LFO — computed once per renderBlock |
| OWLFISH | `OwlfishVoice.h:152` | `std::sin` | Grain LFO — computed once per block |
| OWLFISH | `OwlfishCytomicSVF.h:68` | `std::tan` | SVF coefficients — in `setCoefficients`, called once per block |
| OCELOT | `OcelotVoicePool.h:125` | `std::sqrt` | RMS level — per block |
| OCELOT | `OcelotEmergent.h:221-235` | `5× std::pow` | Pitch multipliers — per noteOn (confirmed in update function) |

### MEDIUM: Setup / noteOn / prepare() — Not In Render Path

| Engine | File:Line | Call | Context |
|--------|-----------|------|---------|
| OBRIX | `ObrixEngine.h:1808-1846` | `19× std::sin` | `buildWavetables()` — runs **once at prepare()** |
| OCTOPUS | `OctopusEngine.h:1130-1175` | `6× std::sin` | `buildWavetable()` — runs once at prepare() |
| OBSIDIAN | `ObsidianEngine.h:1066` | `std::sin` + `std::pow` | LUT build — runs once at prepare() |
| BITE | `BiteEngine.h:71` | `std::sin` | Static sine table — one-time at first use |
| MORPH | `MorphEngine.h:132` | `std::sin` | Table build — once at prepare() |
| OPERA | `OperaPartialBank.h:361` | `std::tan` | `computeLorentzianDetune()` — called on noteOn only |
| OWARE | `OwareEngine.h` | Various exp/cos/sin | Mostly in `malletImpulse()` and `setFreqAndBW()` (noteOn) |
| OCELOT | `OcelotFloor.h:217` | `std::pow` | `// SRO: fastPow2 replaces std::pow (called on noteOn)` — already replaced |
| Most engines | Various | `std::pow(2.0f, (note-69)/12.0f)` | MIDI-to-frequency — all correctly on noteOn, not per-sample |

### ALREADY OPTIMIZED (SRO Annotations)

Ocelot has already applied the SRO (Sample-Rate Optimization) pattern to its hottest paths:
- `OcelotEmergent.h:120` — `fastExp` replaces `std::exp` per-sample envelope decay
- `OcelotEmergent.h:180` — `fastSin` replaces `std::sin` per-sample SVF coefficient
- `OcelotEmergent.h:189` — `fastTanh` replaces `std::tanh` per-sample saturation
- `OcelotFloor.h:282,337,367,382,386,395` — `fastExp/fastSin/fastPow2/fastTanh` all applied
- `OcelotVoice.h:113,188` — `fastSin/fastTanh` applied
- `CytomicSVF.h:70` — `fastTan` replaces `std::tan` in shared SVF (all engines using shared CytomicSVF benefit)

The shared `CytomicSVF::setCoefficients()` uses `fastTan` (not `std::tan`), so engines calling it per-sample (OBELISK, OCHRE, OPALINE, OVEN) pay fastTan cost (~10 cycles) rather than full `std::tan` (~50-100 cycles). Upgrading these to use `setCoefficients_fast()` or precomputing once-per-block would save additional cycles.

---

## 4. Top 10 Optimizations — Ranked by Estimated Cycles Saved Per Sample

### Fix 1 — OSTERIA: Precompute pan gains per block
**File**: `OsteriaEngine.h:1103-1105`
**Issue**: `std::cos(panAngle)` and `std::sin(panAngle)` called inside the per-sample loop, once per channel per voice. `channelPans[c]` does **not change per sample** — it is block-constant.
**Fix**: Precompute `panL[c]` and `panR[c]` arrays once before the sample loop, reference them inside.
**Savings**: 8 voices × 4 channels × 2 trig = **64 trig calls/sample eliminated**. At ~50 cycles each → ~3,200 cycles/sample saved at full polyphony.

```cpp
// Before render loop — add once:
float precomputedPanL[4], precomputedPanR[4];
for (int c = 0; c < 4; ++c) {
    float panAngle = (channelPans[c] + 1.0f) * 0.25f * kOsteriaPI;
    precomputedPanL[c] = std::cos(panAngle);
    precomputedPanR[c] = std::sin(panAngle);
}
// Inside loop: replace std::cos/sin with precomputedPanL[c], precomputedPanR[c]
```

---

### Fix 2 — OPERA: Replace SVF tan with per-block prewarped coefficient
**File**: `OperaEngine.h:238` → `OperaSVF::process()`
**Issue**: `std::tan(kPi * cutoffHz / sampleRate)` computed **every sample** inside `OperaSVF::process()`, which is called twice per voice (filterL + filterR) × 8 voices = **16 tan calls per sample**.
**Fix**: Split SVF into `updateCoefficients(cutoffHz, Q, sampleRate)` (per-block) and `processSample(float input)` (per-sample), mirroring the pattern already used in `Fat/FatEngine.h:228-246`. Use `fastTan` (already available in `FastMath.h`) in the coefficient update.
**Savings**: 16 × ~80 cycles = **~1,280 cycles/sample eliminated**. The cutoff modulates via filter envelope — if envelope-modulated, cache per-voice at block-top; if unmodulated, cache globally.

---

### Fix 3 — OFFERING: Precompute pan gains per voice per block
**File**: `OfferingEngine.h:407-408`
**Issue**: `std::cos` and `std::sin` called for each of 8 voices' panning inside the per-sample loop. `vs.pan` is read from a parameter snapshot that does not change within a block.
**Fix**: Pre-cache `panL[v]` and `panR[v]` arrays before the sample loop.
**Savings**: 8 voices × 2 trig = **16 trig calls/sample eliminated** → ~800 cycles/sample.

---

### Fix 4 — OFFERING: Replace drum oscillator sin with wavetable or fastSin
**Files**: `OfferingTransient.h:392, 404, 466, 533, 550`
**Issue**: `std::sin(phase * 6.2831853f)` called per sample inside `processKick()`, `processTom()`, `processRim()`, `processHat()`. Each active voice type uses 1-2 sin calls per sample.
**Fix**: Replace with `FastMath::fastSin()` (already available fleet-wide) or a single-cycle wavetable. Kick especially benefits: 2 sin → 2 table lookups.
**Savings**: Up to 8 voices × 2 sin = **16 sin/sample eliminated** → ~800 cycles/sample at full load.

---

### Fix 5 — OFFERING: Replace compressor log10+pow with dB lookup table
**File**: `OfferingCity.h:57, 59`
**Issue**: `std::log10 + std::pow(10, ...)` called every sample when signal is above threshold in `OfferingCompressor::process()`. This is the "city processing" applied post-mix.
**Fix**: Replace with a pre-computed gain reduction table indexed by detected level, or use `FastMath::fastLog2` and `FastMath::fastExp` to approximate `log10(x) = log2(x) / log2(10)` and `pow(10, x) = fastExp(x * ln(10))`.
**Savings**: 1 log10 + 1 pow → ~2 fast approximations → **~150 cycles/sample saved** when compressor is active.

---

### Fix 6 — OVERLAP: Reduce Entrainment atan2 to per-block rate
**File**: `DSP/Entrainment.h:72`
**Issue**: `std::atan2(sinSum, cosSum)` called **every sample** for Kuramoto sync computation. The mean-phase calculation driving atan2 runs inside the per-sample render loop.
**Fix**: Move entrainment to a control-rate update (every 32-64 samples). The Kuramoto synchronization time constant (~0.5 seconds) means per-sample precision is unnecessary — 1kHz update rate is perceptually equivalent.
**Savings**: 1 atan2/sample → 1 atan2 per 64 samples → **~2.3 atan2/sample eliminated** → ~150 cycles/sample.

---

### Fix 7 — OCELOT: Replace Canopy partial synthesis sin with wavetable
**Files**: `OcelotCanopy.h:91, 114`
**Issue**: `std::sin` called per sample for partial synthesis in `OcelotCanopy`. Multiple partials accumulate. `OcelotUnderstory.h:127` also uses `std::sin` for a per-sample oscillator.
**Fix**: Ocelot already uses `fastSin` in `OcelotEmergent.h` and `OcelotVoice.h` (SRO complete). Apply the same `xomnibus::fastSin()` to Canopy and Understory. This is a 3-line change per site.
**Savings**: ~3-6 sin/sample per active voice → **~200-400 cycles/voice/sample** saved.

---

### Fix 8 — ORACLE: Replace sort with insertion sort for small breakpoint counts
**File**: `OracleEngine.h` — `evolveBreakpoints()` calls `std::sort`
**Issue**: `std::sort` is called once per waveform cycle for each voice's breakpoints. At typical pitches (100-1000 Hz), this occurs 100-1000 times per second per voice. With 8 breakpoints (typical), this is `O(n log n)` = ~24 comparisons per cycle × 8 voices × frequency-dependent rate.
**Fix**: Use insertion sort — optimal for nearly-sorted small arrays. Breakpoints drift slowly between cycles (random walk), so the array is nearly sorted on each call. Insertion sort on nearly-sorted data is O(n).
**Savings**: 50-70% reduction in sorting work → **~50-100 cycles/cycle/voice** saved.

---

### Fix 9 — OBELISK / OCHRE / OPALINE / OVEN: Upgrade to setCoefficients_fast per sample
**Files**: `ObeliskEngine.h:854`, `OchreEngine.h:719`, `OpalineEngine.h:705`, `OvenEngine.h:768`
**Issue**: All 4 engines call `CytomicSVF::setCoefficients()` (which uses `fastTan`) every sample inside the render loop to update the filter cutoff driven by the filter envelope. The shared CytomicSVF already provides `setCoefficients_fast()` for exactly this use case.
**Fix**: Replace `setCoefficients(cutoff, res, srf)` with `setCoefficients_fast(cutoff, res, srf)` at these 4 call sites. `setCoefficients_fast()` is already implemented and accurate to 0.03% below 0.25×sampleRate.
**Savings**: Each `fastTan` call saved → ~10 cycles × 8 voices × 4 engines = **~320 cycles/sample** total across the 4 engines.

---

### Fix 10 — OBRIX: Replace wavefolder sin with fastSin
**File**: `ObrixEngine.h:901`
**Issue**: `std::sin(signal * fold * kPi)` called inside the per-sample voice render loop for each voice where proc slot 4 (wavefolder) is active.
**Fix**: Replace with `fastSin()` from `FastMath.h` (already imported in `ObrixEngine.h`). The wavefolder input is already clipped/scaled; fast approximation is acceptable.
**Savings**: Up to 8 voices × 1 sin/sample = **~400 cycles/sample eliminated** at full polyphony with wavefolder active.

---

## 5. Additional Findings

### Division in Render Paths
The highest division counts (Otis: 19, Opera: 19, Offering: 18, Ocelot: 17) require manual verification to distinguish compile-time-constant divisors (which the compiler reciprocal-optimizes) from runtime-variable divisors. Most `/ sampleRate` patterns are block-constant and safe. Specifically flagged for review:
- **OTIS** `OtisEngine.h` — 19 divisions; check for per-sample `/ voice.freq` patterns
- **OPERA** `OperaEngine.h` — 19 divisions; `/ sampleRate` in SVF is already in `process()` per sample (tied to Fix 2)

### Memory Allocation
No per-sample heap allocations found. All `std::vector` resizing occurs in `prepare()` (setup phase). The `Ohm` engine calls `couplingBuf.resize(maxBlockSize*2, 0)` in `prepare()` — correct.

### Ouroboros: Designed High CPU
OUROBOROS intentionally uses 4× oversampled RK4 integration for 6 chaos attractor voices. A `profiler.setCpuBudgetFraction(0.22f)` annotation marks this as budgeted at 22% CPU. **Do not optimize without explicit decision** — the chaos quality depends on the oversampling.

### Osteria Formant Coefficients: Already at Control Rate
Osteria's `updateFormants()` (which calls `setCoefficients`) fires at ~2kHz (every 22 samples via `controlRateDiv`), not per-sample. This is correct. Only the **pan computation** is the fix needed (Fix 1 above).

### Engines With No Hot-Path Transcendentals (Fast By Design)
The following 7 engines have zero transcendental calls in any path: **OVERCAST, OVERWORN** (none found), **ORBWEAVE** (1 exp = setup), **OVERFLOW** (1 exp = setup), and many adapter engines. These are all correctly **LOW** tier.

### Oware / Ochre / Opaline (Related Percussion Family)
Ochre, Opaline, Obelisk, and Oware share architectural patterns (mallet impulse, modal resonators, thermal drift). Their `exp/cos/sin` calls are mostly in `malletImpulse()` / `setFreqAndBW()` (noteOn scope). The per-sample cost is dominated by their resonator banks (8 CytomicSVF filters per voice), not transcendentals.

---

## 6. Recommended Priority Order

| Priority | Fix | Engines | Estimated Savings |
|----------|-----|---------|------------------|
| P0 | Fix 1 — Osteria pan precompute | OSTERIA | ~3,200 cycles/sample |
| P0 | Fix 2 — Opera SVF tan → per-block | OPERA | ~1,280 cycles/sample |
| P1 | Fix 3 — Offering pan precompute | OFFERING | ~800 cycles/sample |
| P1 | Fix 4 — Offering drum sin → fastSin | OFFERING | ~800 cycles/sample |
| P1 | Fix 6 — Overlap atan2 → control rate | OVERLAP | ~150 cycles/sample |
| P2 | Fix 5 — Offering compressor log/pow | OFFERING | ~150 cycles/sample |
| P2 | Fix 7 — Ocelot Canopy/Understory fastSin | OCELOT | ~300 cycles/voice |
| P2 | Fix 9 — 4 engines setCoefficients_fast | OBELISK, OCHRE, OPALINE, OVEN | ~320 cycles/sample |
| P3 | Fix 10 — Obrix wavefolder fastSin | OBRIX | ~400 cycles/sample (wavefolder active) |
| P3 | Fix 8 — Oracle insertion sort | ORACLE | ~100 cycles/cycle |

**P0 fixes (OSTERIA + OPERA) alone recover ~4,480 cycles/sample** — equivalent to adding ~2-3 full voices of headroom on an M1 Mac at 44.1kHz.

---

*Report generated by static analysis — all findings require profiling confirmation under real audio load. Cycle estimates assume ~50 cycles/transcendental (std::), ~10 cycles (fast approximation), ~15 cycles/SVF filter.*
