# DSP Profiler Report — V1 Engine CPU Analysis
**Date:** 2026-03-24
**Analyst:** DSP Profiler (Claude Sonnet 4.6)
**Engines Analyzed:** OBRIX (flagship), ONSET (drums / first MPCe pack), OPERA (additive-vocal)
**Sources read:** `Source/Engines/Obrix/ObrixEngine.h`, `Source/Engines/Onset/OnsetEngine.h`, `Source/Engines/Opera/OperaEngine.h`, `Source/Engines/Opera/KuramotoField.h`, `Source/Engines/Opera/OperaPartialBank.h`, `Source/Engines/Opera/ReactiveStage.h`, `Source/Engines/Opera/OperaConstants.h`

---

## Summary Table

| Engine | Weight Class | Max Voices | Key Expensive Operations | Top Optimization Opportunity |
|--------|-------------|-----------|--------------------------|------------------------------|
| OBRIX  | Heavy       | 8 poly    | 3 SVF filters/voice per-sample with `setCoefficients()` inside sample loop; JI ratio search (`findNearestJIRatio`) per voice | Hoist `setCoefficients()` to block-rate for proc filters; cache JI lookup result between samples |
| ONSET  | Medium      | 8 fixed   | ModalResonator: 8 SVF bandpass per-sample; MetallicOsc: 6 oscillator + 3 SVF per-sample; per-sample `voiceFilter.setCoefficients()` in breathing LFO path | Move `voiceFilter.setCoefficients()` to block-rate (called every sample via breathingLFO) |
| OPERA  | Very Heavy  | 8 poly × 4 unison layers | 48 sine partials per voice per sample (×8 voices = 384 fastSin calls/sample); per-partial constant-power pan (fastCos + fastSin per partial per sample); Kuramoto O(2N) field update every 8 samples | Reduce per-partial pan to block-rate lookup; cap effective partial count on single-note patches |

---

## 1. OBRIX (ObrixEngine.h) — Heavy

### Architecture Summary
- 8 polyphonic voices (configurable via `obrix_polyphony`: Mono/Legato/Poly4/Poly8)
- Per voice: 2 PolyBLEP oscillators, 3 CytomicSVF filter slots (Proc1/Proc2/Proc3), 4 modulator slots (ADSR or LFO), 1 amp ADSR
- 3 global FX slots (Delay/Chorus/Reverb) — serial or parallel (Wave 4)
- Wave 3: global Drift Bus LFO ticked per sample
- Wave 4: Biophonic — turbidity noise RNG per sample, JI harmonic field per voice, stress/bleach state
- Wave 5: Reef Residency — coupling RMS computation over whole buffer pre-block

### DSP Operations Per Sample (8-voice worst case, all bricks active)
| Operation | Count | Cost Note |
|-----------|-------|-----------|
| PolyBLEP oscillator render | 2/voice × 8 = 16 | PolyBLEP adds conditional polyBLEP correction (cheap) |
| CytomicSVF `setCoefficients()` + `processSample()` | 3/voice × 8 = 24 | `setCoefficients` calls `2*sin(pi*f/sr)` — expensive per-sample |
| ObrixADSR/ObrixLFO process | (4+1)/voice × 8 = 40 | Lightweight one-pole math |
| fastTanh (feedback + wavefolder) | up to 4/voice × 8 = 32 | FastMath — acceptable |
| JI ratio search (`findNearestJIRatio`) | 2/voice × 8 = 16 | Table search per source per voice when `fieldStrength > 0` |
| Turbidity LCG RNG | 1/sample global | Trivial |
| Drift Bus fastSin | 1/sample global + per-voice offset | 1 fastSin per voice = 8 |
| Global FX (Delay+Chorus+Reverb in series) | 3 global slots | Reverb: 4 comb + 2 allpass filters per sample |
| Distance/Air 1-pole filters | 4 per sample | Trivial |

**Hottest path:** `voice.procFilters[i].setCoefficients(cut, res, sr)` called every sample for each active processor slot. `CytomicSVF::setCoefficients` computes `2.0f * std::sin(kPi * cutoff / sr)` — a transcendental per-sample. With 3 active procs × 8 voices = 24 `std::sin` calls per sample on top of the filter computation. This is the single most expensive operation per sample.

### Denormal Protection
- Feedback state: `flushDenormal()` on `procFbState` — present and correct
- Drift bus: `flushDenormal()` on drift phase states — present
- JI IIR: `flushDenormal()` on `jifiOffset` — present
- Stress/bleach integrators: `flushDenormal()` applied — present
- `juce::ScopedNoDenormals` at renderBlock entry — present
- **Status: Complete**

### Stewardship Canon Checks
- **Dead DSP stages:** Proc2 and Proc3 default to `Off`. The per-sample switch branch returns immediately (`if (proc2Type > 0 && proc2Type <= 3 && src2Type > 0)`). However `setCoefficients()` is still called every sample even when the filter is in Wavefolder or RingMod mode (proc types 4 and 5) — those calls compute sin unnecessarily and then the filter output is ignored.
- **Filter mode tax:** CytomicSVF is a 2-pole SVF (12 dB/oct). All three proc slots default to or use the same 2-pole structure regardless of mode. No LP24 (4-pole) overhead — appropriate.
- **Effect bypass:** FX slots check `fxType[fx] > 0` before calling `applyEffect` — zero-cost early return when Off. **Correct.**
- **Shared DSP usage:** Uses `StandardADSR` (aliased as `ObrixADSR`) and `StandardLFO` (aliased as `ObrixLFO`) from `Source/DSP/`. `CytomicSVF` from `Source/DSP/`. `PolyBLEP` from `Source/DSP/`. **Full shared library compliance.**

### Top 3 Optimization Opportunities

1. **Hoist `setCoefficients()` to block-rate for all proc filters.** Currently called every sample with per-sample modulated cutoff values. The proper fix: accumulate cutoff modulation to a block-level value (or at minimum compute once per sub-block of 16 samples), then call `setCoefficients` only when the value changes by more than a threshold (e.g., 1 Hz). The SVF state equations are stable between coefficient updates. Estimated saving: ~50% of OBRIX's compute budget at full polyphony.

2. **Gate the JI `findNearestJIRatio` lookup.** Currently executed every sample per active voice when `fieldStrength > 0`. The JI target changes slowly (it is driven by the note's fundamental ratio). Cache the `nearestJI` result and recompute only when the voice note changes or when `fieldStrength` crosses a threshold. The IIR convergence (`jifiOffset`) can run every sample with the cached result.

3. **Hoist Reef Residency Parasite HF filter loop.** The Wave 5 Parasite mode iterates the entire coupling buffer with a per-sample 1-pole HP filter inside a `for(i..reefCouplingBufLen_)` loop — this runs before the main sample loop and adds another O(blockSize) pass. Use a running leaky integrator updated once per block instead. The RMS accuracy loss is negligible for ecological purposes.

---

## 2. ONSET (OnsetEngine.h) — Medium

### Architecture Summary
- 8 fixed-role voices (Kick, Snare, HH-C, HH-O, Clap, Tom, Perc A, Perc B)
- Each voice: Layer X (one of BridgedTOsc / NoiseBurstCircuit / MetallicOsc) + Layer O (one of FMPercussion / ModalResonator / KarplusStrongPerc / PhaseDistPerc)
- Equal-power Layer X/O blend — **blend gains hoisted to block-rate** (QA C1, correctly implemented)
- Per voice: 1 CytomicSVF voice filter (cutoff modulated per-sample by breathingLFO), 1 OnsetEnvelope, 1 OnsetTransient
- Global: CharacterStage (tanh sat + warmth LP), OnsetDelay (LP in feedback), OnsetReverb (4-comb + 2-allpass Schroeder), OnsetLoFi
- Cross-Voice Coupling (XVC): previous-block peak tracking, 4 routing rules
- SilenceGate: zero-cost bypass when all voices silent

### DSP Operations Per Sample (8 voices all active, worst-case blend)
| Operation | Count | Cost Note |
|-----------|-------|-----------|
| Layer X render (BridgedT/NoiseBurst/Metallic) | 1/voice × 8 = 8 | BridgedTOsc: 1 fastSin + 1 fastExp + 1 fastTanh. NoiseBurst: 2 fastSin + 1 CytomicSVF. MetallicOsc: 6 phase accumulate/compare + 3 CytomicSVF |
| Layer O render | 1/voice × 8 = 8 | FM: 2 fastSin. Modal: 8 CytomicSVF bandpass. KarplusStrong: 1 delay read/write. PhaseDist: 1 fastSin |
| OnsetEnvelope process | 8 | Trivial one-pole |
| OnsetTransient process | 8 (only during attack ms) | Very brief — negligible |
| voiceFilter setCoefficients + processSample | 8 | `setCoefficients` called every sample due to breathingLFO modulation — 8 `std::sin` calls |
| breathingLFO process | 8 | Lightweight LFO (uses StandardLFO internally) |
| Global CharacterStage | 1 stereo | fastTanh × 2 + 2 SVF filter samples |
| OnsetDelay | 1 stereo | Delay read + LP filter in feedback × 2 |
| OnsetReverb | 1 stereo | 4 comb + 2 allpass per sample |
| Master filter | 2 SVF samples | Trivial |

**Hottest path (per voice):** ModalResonator when active — 8 CytomicSVF bandpass filters executed in parallel every sample. At 8 voices if all were ModalResonator, this would be 64 SVF evaluations per sample. In practice only V6 (Tom) and V8 (Perc B) default to Modal; V8 uses Metallic (3 SVF). Worst realistic active count: ~20 SVF ops/sample for voice DSP.

**Second hottest:** `voiceFilter.setCoefficients()` called inside `processSample()` every sample for all 8 voices, because breathingLFO modulates cutoff. This is 8 `std::sin` calls per sample unconditionally when any voice is active. Flagged in the code as a known inefficiency — comment at line 1057 says "Coefficients are block-constant — caller should call setWarmth() once per block" (CharacterStage), but the voice filter does NOT apply this same discipline.

### Denormal Protection
- `OnsetEnvelope::flushDenormal` on decay/release — present for all decay stages
- Feedback in `OnsetDelay` (LP filter feedback path) — `flushDenormal` present
- Reverb comb filter LP states — `flushDenormal` present
- KarplusStrong feedback loop — `flushDenormal` present
- `juce::ScopedNoDenormals` at renderBlock entry — present
- **Status: Complete**

### Stewardship Canon Checks
- **Dead DSP stages:** Each OnsetVoice instantiates all 3 Layer X types (BridgedTOsc + NoiseBurstCircuit + MetallicOsc) and all 4 Layer O types (FM + Modal + KarplusStrong + PhaseDist), but only one of each is rendered per sample (`switch(circuitType)`, `switch(algoMode)`). The inactive objects exist in memory but consume zero CPU — correct.
- **Filter mode tax:** ModalResonator uses 8 CytomicSVF bandpass filters. For a voice like Kick (V1, defaults to Modal as Layer O) this is appropriate — the 8-mode resonator bank is the entire sonic identity. However V3 and V4 (Hi-Hats) default to FM as Layer O. FM is cheaper than Modal, which is appropriate given hats are often at high blend toward Layer X (Metallic). No unnecessary LP24-equivalent overhead found.
- **Effect bypass:** `if (mix < 0.001f) return;` at the top of `OnsetDelay::process`, `OnsetReverb::process`, `OnsetLoFi::process` — zero-cost when FX mix is zero. **Correct.**
- **Shared DSP usage:** Uses `CytomicSVF` from `Source/DSP/`. Uses `StandardLFO` for `BreathingLFO`. Uses `SilenceGate` from `Source/DSP/SRO/`. Uses `PitchBendUtil`. **Good compliance.** Layer X/O modules (BridgedTOsc, FMPercussion, etc.) are ONSET-private implementations — appropriate since drum voice primitives are not fleet-general.

### Top 3 Optimization Opportunities

1. **Move `voiceFilter.setCoefficients()` to block-rate.** The breathing LFO (`0.08 Hz`) changes so slowly that per-sample coefficient recomputation is wasteful. Compute the modulated cutoff once per block (or at sub-block rate of 16–32 samples) and call `setCoefficients` only then. This eliminates 8 `std::sin` calls per sample — the single largest win available in ONSET. Estimated saving: 15–20% total ONSET CPU.

2. **Consider SilenceGate voice-level gating.** The current SilenceGate operates at the engine output level (100ms hold). Individual voices that have decayed to silence still enter the sample loop and execute the early-exit `if (!voices[v].active) continue`. Marking individual voices inactive sooner (already done via `ampEnv.isActive()`) is correct, but the MetallicOsc and KarplusStrongPerc have their own `isActive()` checks separate from the amp envelope. Verify that all Layer X/O oscillators auto-deactivate when the amp envelope reaches `Stage::Idle` — KarplusStrong's `1e-7` silence floor is correct, but BridgedTOsc has `bool active = false` set at trigger — it never self-deactivates, relying on the amp envelope. This is correct but means the oscillator keeps computing (cheaply, but still) until the envelope gates it. Low priority for BridgedTOsc; no action needed.

3. **ModalResonator: cache `setCoefficients` per trigger, not per sample.** The ModalResonator calls `resonators[i].setCoefficients()` only in `trigger()` (not in `process()`) — this is already correctly block-rate. However the Q values and mode frequencies never change mid-note. This is already optimal. The optimization opportunity is actually to **skip inactive ModalResonators** (those whose mode amplitude is < 1e-4) rather than calling `processSample` on all 8. A small amplitude check before each bandpass call could save ~3–4 SVF evaluations per active ModalResonator voice.

---

## 3. OPERA (OperaEngine.h + support files) — Very Heavy

### Architecture Summary
- 8 polyphonic voices × up to 4 unison layers = up to 32 logical synthesis instances
- Per voice per sample: up to 48 additive partials (4–48, default 32) rendered as sine oscillators
- Each partial: amplitude computation (formant weight × spectral tilt × Nyquist gain × cluster boost), per-partial constant-power stereo panning (fastCos + fastSin per partial), phase advance
- Kuramoto coupled-oscillator field (`KuramotoField`): O(2N) mean-field update every 8 samples, writes phase corrections back to all partials
- OperaBreathEngine: noise-filtered formant breath component — additional per-voice per-sample cost
- 2 OperaSVF filters per voice (L+R) with block-rate coefficient cache (already optimized)
- OperaConductor: autonomous dramatic arc, sampled every block — negligible
- ReactiveStage: Hadamard FDN reverb (4 delay lines + Hadamard matrix), runs once per block post-voice
- 3 global LFOs + vibrato LFO ticked per sample
- Block-rate: formant weight update, Nyquist gain update, active voice envelope configuration

### DSP Operations Per Sample (8 voices, 32 partials each, unison=1)
| Operation | Count | Cost Note |
|-----------|-------|-----------|
| Partial phase advance | 32/voice × 8 = 256 | `p.theta += p.omega * invSr_` — trivial |
| fastSin (sine table lookup) per partial | 32/voice × 8 = 256 | 1024-entry table with linear interp — fast but 256 calls/sample |
| Per-partial pan (fastCos + fastSin) | 32/voice × 8 = 256 × 2 = 512 | Two trigonometric calls per partial per sample — **most expensive single operation** |
| formantWeights/nyquistGains/clusterBoost lookup | 32/voice × 8 = 256 | Array reads — cache-friendly |
| spectralTilt compute | 32/voice × 8 = 256 | Involves `fastPow` or multiply chain |
| KuramotoField update (every 8 samples) | O(2N) per voice = O(64) per voice × 8 = O(512) per update | Mean-field: 2 × N cos/sin operations + phase correction injection, amortized to ~64 trig calls/sample |
| OperaEnvelope process (amp + filter) | 2/voice × 8 = 16 | Trivial one-pole |
| OperaSVF (L+R, coefficient cached) | 2/voice × 8 = 16 | SVF at block-rate coefficients — cheap |
| OperaBreathEngine | 1/voice × 8 = 8 | Contains noise + formant filters |
| vibratoLFO + LFO1 + LFO2 | 3 global | Trivial |
| std::pow (vibrato pitch multiplier) | 1/sample | `std::pow(2.0f, ...)` — expensive! Uncached |
| ReactiveStage (FDN reverb, block) | 4 delay lines + Hadamard | Amortized per sample: ~12 multiply-adds |

**Hottest path: Per-partial constant-power panning.**
For each partial, `renderBlock` computes:
```cpp
float angle = (pan + 1.0f) * 0.25f * kPi;
float panL  = FastMath::fastCos(angle);
float panR  = FastMath::fastSin(angle);
```
This fires 2 trig calls per partial per sample. At default 32 partials × 8 voices = 512 trig calls per sample, just for panning. Since panning is driven by Kuramoto order parameter R (which changes every 8 samples), these could be computed at 8-sample intervals and linearly interpolated. This is the single highest-value optimization available.

**Second hottest: `std::pow(2.0f, vibSample * ...)` for vibrato pitch multiplier.**
Called once per sample — `std::pow` is expensive. Should use `fastPow2` or a Taylor expansion approximation. Replace with `FastMath::fastExp2` or the fleet-standard `fastPow2` from the OBRIX path.

**Third hottest: Kuramoto update (every 8 samples per voice).**
The O(2N) mean-field reduction computes sum of N cosines and sines (the complex mean-field R × e^(i×psi)), then applies phase corrections. At 32 partials per voice, this is ~64 trig calls per voice per update cycle, amortized to ~8 trig calls per sample per voice × 8 voices = 64/sample. The 8-sample decimation (`kKuraBlock = 8`) is already a significant optimization — halving this to kKuraBlock=4 would double cost, so maintain at 8.

**Unison note:** At `unison=4` (MACRO CHORUS fully open), the partial rendering loop nests 4 unison layers over the partial loop. Cost scales linearly: 32 partials × 4 layers = 128 fastSin calls per voice per sample. At 8 voices: 1024 fastSin calls/sample plus 1024 pan trig pairs = 3072 trig calls per sample. This is the absolute worst case. At 48 partials × 4 unison × 8 voices = 1536 fastSin + 3072 pan trig = 4608 trig operations per sample. Consider imposing an effective-voice count cap: `clamp(unison * activeVoiceCount, 1, 16)`.

### Denormal Protection
- OperaEnvelope: `flushDen()` on decay/release — present
- OperaSVF: bitwise denormal flush on `ic1eq`/`ic2eq` — present and efficient
- KuramotoField: `flushDenormal` on `kSmoother_` state — present
- ReactiveStage FDN: needs verification (not fully read, but FDN delay lines with feedback are prime denormal sites)
- `juce::ScopedNoDenormals` at renderBlock entry — present
- **Status: Mostly complete. Verify ReactiveStage feedback paths.**

### Stewardship Canon Checks
- **Dead DSP stages:** When `breath_eff < 0.001f`, the BreathEngine still runs `processSample()` — no early-exit guard on the breath component. Low risk (breath engine is lightweight) but a guard could save some CPU.
- **Filter mode tax:** OperaSVF is a 2-pole SVF per channel. With block-rate coefficient caching (`lastCutoff_`/`lastRes_` delta check > 0.001f), the expensive `fastTan` runs only when cutoff changes. **Well optimized (P0 fix from 2026-03-22 is in place).**
- **Effect bypass:** ReactiveStage processes only `if (snap_.stage > 0.0001f)` — correctly gated. Soft clip pass always runs (cheap).
- **Shared DSP usage:** OPERA does not use StandardLFO or StandardADSR — it has self-contained `OperaLFO` and `OperaEnvelope` structs. These are functionally equivalent but represent code duplication. Not a bug, but a maintenance debt. More critically, OPERA uses its own `OperaPartialBank::FastMath` (1024-entry sine table) which is separate from fleet-wide `FastMath.h`. Both are correct implementations; the duplication is benign.
- **Portamento `std::exp` per sample:** Line ~963: `float glideCoeff = 1.0f - std::exp(-kTwoPi / (sr_ * portTime))` computed per sample per active voice. Should be hoisted to block-rate since `portamento` param doesn't change mid-sample.

### Top 3 Optimization Opportunities

1. **[P0-class] Hoist per-partial panning to 8-sample rate.** The pan values (`OperaPartialBank::computePartialPan` + `fastCos`/`fastSin`) depend on Kuramoto R and psi, which update every `kKuraBlock = 8` samples. Therefore pan values are provably constant between Kuramoto updates. Cache per-partial `panL`/`panR` in the partial struct and refresh only when `kuramotoField.shouldUpdate()` returns true (every 8 samples). Eliminates ~512 trig calls/sample at 32 partials × 8 voices. Estimated saving: 30–40% of OPERA's total CPU budget.

2. **Replace `std::pow(2.0f, vibSample * ...)` with `FastMath::fastPow2`.** A one-line change. The vibrato pitch multiplier `std::pow(2.0f, vibSample * (100.0f/1200.0f))` fires `std::pow` once per sample. Use `fastExp(vibSample * (100.0f/1200.0f) * kLn2)` or the fleet's `fastPow2`. Estimated saving: minor (~1%) but free.

3. **Hoist portamento glide coefficient to block-rate.** `std::exp(-kTwoPi / (sr_ * portTime))` is computed every sample per active voice when `portamento > 0`. Since `portamento` is read from a snapshot and doesn't change within a block, this can be computed once before the sample loop. Additionally, this is inside the per-voice, per-sample inner loop — at 8 voices, 8 `std::exp` calls per sample are eliminated. Estimated saving: ~5% of OPERA CPU when portamento is non-zero.

---

## CPU Weight Analysis

### Estimated Relative Load (single instance, full polyphony, typical preset)

| Engine | Estimated % CPU (single core, 48kHz, 512-sample block) | Notes |
|--------|--------------------------------------------------------|-------|
| ONSET  | 8–14% | 8 fixed voices but percussion — short decay, SilenceGate helps significantly |
| OBRIX  | 18–28% | 8 voices, 3 filters per voice per sample with `setCoefficients`; Wave 4 biophonics adds overhead |
| OPERA  | 28–45% | Partial count and per-partial panning dominate; unison=4 pushes toward 45% |

**P0 Risk Assessment:**
- ONSET at default settings: no CPU P0 risk. SilenceGate eliminates cost between hits.
- OBRIX at full polyphony with Wave 4 biophonics fully active: approaches 30% CPU — at the threshold for concern on slower iOS hardware. On macOS M-series: safe.
- OPERA at 8-voice polyphony with `partials=48` and `unison=4`: may exceed 40% CPU on iOS (A-series chip at 48kHz). With default `partials=32` and `unison=1`: approximately 18–22%, acceptable. **The per-partial panning fix (Optimization #1) must be implemented before V1 iOS launch.**

### Coupling Cost Note
When OBRIX and OPERA are both active and coupled (e.g., KnotTopology coupling), each engine's `applyCouplingInput()` adds an additional O(blockSize) buffer accumulation pass, and OPERA's `couplingKBuffer_` is read inside the per-sample loop. No P0 issue — the coupling buffer reads are array accesses, not transcendentals.

---

## Stewardship Summary

| Canon | OBRIX | ONSET | OPERA |
|-------|-------|-------|-------|
| Denormal protection | Complete | Complete | Mostly complete (verify ReactiveStage) |
| Block-rate param reads (ParamSnapshot) | Complete | Complete | Complete |
| FX bypass (zero cost when Off) | Complete | Complete | Complete (stage guard) |
| Shared DSP library usage | Full compliance | Good compliance | Partial — self-contained LFO/ADSR |
| Dead DSP stages | Minor: `setCoefficients` called for inactive Wavefolder/RingMod proc slots | None found | Minor: BreathEngine lacks early-exit at breath=0 |
| SilenceGate | Yes (500ms hold) | Yes (100ms percussive hold) | Partial (stage bypass only) |

---

## Priority Action List

### Before V1 iOS Launch (P0)
1. **[OPERA]** Cache per-partial pan (L/R) at 8-sample Kuramoto rate — eliminates ~512 trig calls/sample
2. **[OPERA]** Cap effective `unison × activeVoices` product at 16 to prevent 4608-trig/sample worst case
3. **[OBRIX]** Hoist `setCoefficients()` to block-rate for all proc filter slots (check cutoff delta threshold)

### Before V1 macOS Launch (P1)
4. **[ONSET]** Move `voiceFilter.setCoefficients()` from per-sample to block-rate (breathing LFO is 0.08 Hz — imperceptible to update at 47 Hz = every ~1024 samples)
5. **[OPERA]** Replace `std::pow` with `fastPow2` for vibrato pitch multiplier
6. **[OPERA]** Hoist portamento glide coefficient to block-rate pre-loop computation

### Post-V1 Polish (P2)
7. **[OBRIX]** Cache JI `findNearestJIRatio` result per voice per note — recompute only on note change
8. **[OBRIX]** Gate `setCoefficients` call for Wavefolder/RingMod proc slots (proc types 4/5 do not use the SVF output — avoid the `std::sin` computation)
9. **[OPERA]** Verify ReactiveStage FDN feedback paths have denormal protection on all 4 delay line filters
10. **[ONSET]** Add `modeAmplitude < 1e-4f` early-exit guard inside ModalResonator per-mode bandpass loop

---

*Report generated by automated source analysis. All findings based on static code review of `Source/Engines/` headers. Runtime profiling with Instruments or perf is recommended to validate estimates, particularly for OPERA at full unison.*
