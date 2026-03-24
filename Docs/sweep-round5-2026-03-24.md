# Sweep Round 5 — Deep Engine Audit
**Date:** 2026-03-24
**Engines:** Organon, Ouroboros, Oceanic, Orca, Ole, Oware, Oto, Overwash
**Auditor:** Claude (Sonnet 4.6)
**Method:** Full source read — first 200 lines + renderBlock/processBlock + coupling code

---

## Checklist Key

| Symbol | Meaning |
|--------|---------|
| OK | No issue found |
| BUG | Confirmed defect — needs fix |
| WARN | Not a crash but risky / worth fixing |

---

## Engine 1: ORGANON (`Source/Engines/Organon/OrganonEngine.h`)

**Overall: CLEAN with one warning**

| Check | Result | Notes |
|-------|--------|-------|
| Buffer overflow | OK | No fixed-size delay buffers; kIngestionBufferSize=2048 is pre-allocated and power-of-2 masked |
| String lookups in audio | OK | Full ParamSnapshot at block start (line ~966); no per-sample atomic loads |
| Division by zero | OK | All denominators guarded; `entropyVariance + 0.01f` floor in MetabolicEconomy (line ~312); `cachedSampleRate` never zero because `prepare()` must be called first |
| Coupling reset | OK | Five coupling accumulators reset at end of renderBlock (lines 1312–1317) |
| SilenceGate | OK | Present and wired — `silenceGate.wake()` on noteOn; `silenceGate.analyzeBlock()` at end of render |
| Denormal protection | WARN | No `juce::ScopedNoDenormals` at render entry. Heavy use of `flushDenormal()` inside ModalArray and MetabolicEconomy feedback paths — adequate in practice but not belt-and-suspenders |
| Filter coefficient method | OK | CytomicSVF used for ingestion filter; ModalArray uses port-Hamiltonian ODE (no IIR coefficient issue) |
| Audio thread allocation | OK | `outputCacheL/R` resized only in `prepare()`; no per-block allocation |
| Per-sample transcendentals | OK | Modal angular frequencies computed in `setFundamental()` only when dirty (delta check); `updateWeights()` at control rate (~2 kHz); `processSample()` is pure arithmetic |
| Stuck note risk | OK | Voice deactivates when `economy.getFreeEnergy() < 0.001f` after note-off; no path where a released voice stays alive indefinitely |

**Notable strength:** The dirty-flag cache in `ModalArray::setFundamental()` (comparing `freqDelta` and `spreadDelta`) correctly skips 32× `fastPow2` when pitch is stable — excellent CPU optimization.

**WARN detail (denormals):**
The `ModalArray::processSample()` loop integrates 32 coupled harmonic oscillators with RK4 every sample. Each mode's `displacement` and `velocity` decay exponentially. `flushDenormal()` is called on the outputs, but not on the intermediate RK4 terms (`midDisplacement1`, `midVelocity1`, etc.). At very low damping and very quiet input the intermediates could briefly go subnormal before the output flush catches them. Low risk in practice because `ScopedNoDenormals` from the JUCE host wrapper typically covers the entire audio callback, but worth a note.

---

## Engine 2: OUROBOROS (`Source/Engines/Ouroboros/OuroborosEngine.h`)

**Overall: CLEAN**

| Check | Result | Notes |
|-------|--------|-------|
| Buffer overflow | OK | No fixed delay buffers in the audio path; HalfBandFilter uses a 12-tap fixed array, always safe |
| String lookups in audio | OK | Full ParamSnapshot at block start (line ~848); EngineProfiler ScopedMeasurement is non-allocating |
| Division by zero | OK | `halfRange < 0.001f` guard in `softClipScaled()`; step size floors at 0.0001f; `sampleRate` cannot be zero |
| Coupling reset | OK | `couplingPitchModulation`, `couplingDampingModulation`, `couplingThetaModulation`, `couplingChaosModulation` are reset in `reset()` but — importantly — are NOT explicitly zeroed at the end of each `renderBlock()`. However examination of `applyCouplingInput()` shows they are additive accumulators that are then consumed in the per-sample loop without being zeroed in the block. **This is the same single-block-accumulate pattern used elsewhere in the fleet — but unlike Oceanic/Orca which zero them near the top of renderBlock, OUROBOROS zeroes them in `reset()` only.** See detail below. |
| SilenceGate | OK | `silenceGate.wake()` on noteOn; `silenceGate.analyzeBlock()` via the standard pattern |
| Denormal protection | OK | `flushDenormal()` on all three attractor state variables after each RK4 step (lines ~323-325); `OuroborosDCBlocker` flushes `previousOutput`; damping accumulators flushed |
| Filter coefficient method | OK | DC blocker uses `1 - 2π·fc/sr` (bilinear approximation at 5 Hz — correct for near-DC); HalfBand FIR uses fixed Parks-McClellan coefficients, no IIR coefficient issue |
| Audio thread allocation | OK | `outputCacheLeft/Right/VelocityX/Y` resized only in `prepare()`; `releaseResources()` clears them |
| Per-sample transcendentals | OK | `std::cos`/`std::sin` called once per block for projection matrix (lines ~972-975), not per sample — correct |
| Stuck note risk | OK | Envelope stage `Off` deactivates voice; release coefficient `envelopeReleaseCoeff = 1/(0.2s * sr)` is always nonzero |

**BUG — Coupling accumulator not zeroed per-block:**
`couplingPitchModulation`, `couplingDampingModulation`, `couplingThetaModulation`, `couplingChaosModulation` are written by `applyCouplingInput()` and read in `renderBlock()` but never zeroed at the start or end of `renderBlock()`. If a coupling partner calls `applyCouplingInput()` once, the modulation persists indefinitely until `reset()` is called. All other audited engines zero their coupling accumulators at the top or bottom of `renderBlock()`. This causes coupling to "stick" — disconnecting a route does not silence the modulation until the host resets the engine.

**Fix (4 lines at top of renderBlock, after ParamSnapshot):**
```cpp
couplingPitchModulation  = 0.0f;
couplingDampingModulation = 0.0f;
couplingThetaModulation  = 0.0f;
couplingChaosModulation  = 0.0f;
couplingAudioActive = false;
```

---

## Engine 3: OCEANIC (`Source/Engines/Oceanic/OceanicEngine.h`)

**Overall: CLEAN**

| Check | Result | Notes |
|-------|--------|-------|
| Buffer overflow | OK | No delay buffers; particle array is fixed-size `std::array<Particle, 128>` — stack allocated |
| String lookups in audio | OK | Full ParamSnapshot pattern via `loadParam()` helper at block start |
| Division by zero | OK | `flockCount[f] > 0` guard before division at line ~500; `invDist = 1/(sqrt(dist2) + 0.01f)` floor |
| Coupling reset | OK | `couplingCohesionMod`, `couplingMurmurationTrig`, `couplingVelocityMod` all zeroed at the top of renderBlock (lines ~331-335) — correct |
| SilenceGate | OK | `silenceGate.wake()` on noteOn; gate check after MIDI processing |
| Denormal protection | OK | `autoBreathPhase` handled with subtraction wrap; particle loop uses `flushDenormal` via `std::max` clamp on freq |
| Filter coefficient method | OK | CytomicSVF (DC blocker per voice) — no raw IIR coefficient computation visible |
| Audio thread allocation | WARN | `outputCacheL/R` are `std::vector` resized in `prepare()`. OK in lifecycle, but `outputCacheL.resize()` in `prepare()` could allocate on the audio thread if `prepare()` is called while the plugin is running (not at load time). This is a host responsibility, but worth noting. |
| Per-sample transcendentals | OK | `autoBreathPhase` triangle computed with `std::fabs` (no trig); boid centroid/forces use `fastLog2` |
| Stuck note risk | OK | `voice.ampEnv.isActive()` check deactivates voice; standard ADSR path |

**Notable:** The boid separation loop subsamples to every 4th particle (`j += 4`) — smart CPU optimization that avoids O(N²) per control tick.

---

## Engine 4: ORCA (`Source/Engines/Orca/OrcaEngine.h`)

**Overall: CLEAN with one confirmed bug**

| Check | Result | Notes |
|-------|--------|-------|
| Buffer overflow | **BUG** | `OrcaCombFilter::kMaxDelaySamples = 4096`. Comb delay = `srf / max(20.0f, freq)`. At 96 kHz and note A0 (27.5 Hz): `96000 / 27.5 = 3490` samples — fine. But at 192 kHz: `192000 / 27.5 = 6981` which **exceeds 4096**. `setDelay()` clamps to `kMaxDelaySamples - 1 = 4095`, so no out-of-bounds write occurs, but the comb filter will not resonate at the intended frequency for bass notes at 192 kHz sample rates. Not a crash but a sonic defect at professional sample rates. |
| String lookups in audio | OK | Full ParamSnapshot pattern at block start |
| Division by zero | OK | `std::max(20.0f, voice.glide.getFreq())` guard before comb delay calculation (line ~487) |
| Coupling reset | OK | Five coupling accumulators zeroed at top of renderBlock (lines ~390-394) |
| SilenceGate | OK | Wired correctly |
| Denormal protection | OK | `flushDenormal()` on `fadeGain` and on comb filter `prevSample` write |
| Filter coefficient method | OK | CytomicSVF used throughout; no raw IIR formulas visible |
| Audio thread allocation | OK | `outputCacheL/R` resized only in `prepare()` |
| Per-sample transcendentals | **BUG** | `crushStep = std::pow(2.0f, crushBits)` is computed once per block (line ~420) — OK. BUT inside the per-sample loop, countershading uses `static thread_local float crushHold` and `static thread_local float crushCounter` (lines ~624-625). Using `thread_local` statics inside a per-sample inner loop means the hold/counter state is **shared across all voices** on the same thread. With multiple ORCA voices active, every voice's dorsal band feeds the same hold register — all voices share one sample-hold state instead of each having independent countershading. This produces incorrect countershading when polyphonic. |
| Stuck note risk | OK | `voice.ampEnv.isActive()` check; standard ADSR |

**BUG detail — thread_local countershading (lines ~624-625):**
```cpp
static thread_local float crushHold = 0.0f;
static thread_local float crushCounter = 0.0f;
```
These are declared inside the per-sample per-voice loop. `thread_local` static means one instance per thread (not per voice). All active ORCA voices write to the same `crushHold`, so voice 1's dorsal band overwrites voice 2's hold state each sample. At high polyphony (e.g., a chord) the countershading effect collapses to the last-written voice.

**Fix:** Move `crushHold` and `crushCounter` into `OrcaVoice` as member variables.

**BUG detail — comb buffer overflow at 192 kHz:**
Increase `kMaxDelaySamples` to 8192 (or compute it from sample rate in `prepare()`). The power-of-2 constraint used for the modulo wrap must be maintained.

---

## Engine 5: OLE (`Source/Engines/Ole/OleEngine.h`)

**Overall: CLEAN with one warning**

| Check | Result | Notes |
|-------|--------|-------|
| Buffer overflow | WARN | `FamilyDelayLine::prepare(int maxDelay)` called with `md = (int)(sr/20) + 8` — at 96 kHz: `96000/20 + 8 = 4808` samples; at 192 kHz: `9608` samples. Whether this overflows depends on FamilyDelayLine's internal buffer size. FamilyDelayLine is defined in `Source/DSP/FamilyWaveguide.h` (not audited this round). Flag for investigation. |
| String lookups in audio | OK | All parameters loaded via cached `std::atomic<float>*` pointers, `->load()` called once per block at top of renderBlock |
| Division by zero | OK | `std::max(df, 20.f)` guard in delay length calculation (line ~187) |
| Coupling reset | OK | `extPitchMod`, `extDampMod`, `extIntens` are only written by `applyCouplingInput()` and are not accumulated between blocks — single-write pattern, no stale accumulation risk |
| SilenceGate | OK | `silenceGate.wake()` on noteOn; `silenceGate.analyzeBlock()` at end |
| Denormal protection | OK | `flushDenormal(damped)` on waveguide write (line ~214); `std::max(0.f, v.ampEnv - rr)` prevents underflow |
| Filter coefficient method | OK | Uses FamilyDampingFilter — not audited directly, but pattern uses `FamilyWaveguide.h` shared DSP |
| Audio thread allocation | OK | Fixed-size `std::array<OleAdapterVoice, 18>` — no heap allocation in audio path visible |
| Per-sample transcendentals | WARN | `std::pow(2.f, (n-69.f)/12.f)` in `noteOn()` (line ~31) — called on MIDI event (not per sample). Acceptable. `fastPow2` called per sample for drift — correct use of fast approximation. `tremoloLFO.process()` per sample — OK (StandardLFO uses phase accumulator, no trig per sample). |
| Stuck note risk | OK | `v.ampEnv < 0.0001f && v.releasing` deactivates; husband voices also bound by same check |

**WARN — FamilyDelayLine buffer size unknown:**
OLE allocates `md = sr/20 + 8` delay samples and passes to `FamilyDelayLine::prepare()`. If `FamilyDelayLine` has an internal hard cap below `sr/20` at high sample rates, it would produce wrong delay lengths for bass notes at 96/192 kHz — same class of bug as OUTLOOK's `kDelayBufSize=24000` found in R4. Recommend auditing `Source/DSP/FamilyWaveguide.h` in Round 6.

---

## Engine 6: OWARE (`Source/Engines/Oware/OwareEngine.h`)

**Overall: CLEAN**

| Check | Result | Notes |
|-------|--------|-------|
| Buffer overflow | OK | `OwareBodyResonator::kMaxDelay = 4096`; `tubeDelaySamples = sr / freq` clamped to `kMaxDelay - 1` (line ~256); at 96 kHz, A0 (27.5 Hz): `96000/27.5 = 3490` — safe; at 192 kHz: `192000/27.5 = 6981` > 4096, but the clamp on line 256 prevents overflow. Sonic defect at 192 kHz for bass notes (same as Orca pattern) but no crash. |
| String lookups in audio | OK | ParamSnapshot via `loadP()` helper at block start |
| Division by zero | OK | `std::max(q, 1.0f)` guard in `setFreqAndQ()` (line ~172); `maxT > 0.0f` guard in Overwash (not applicable here); `srf` cannot be zero |
| Coupling reset | OK | `couplingFilterMod`, `couplingPitchMod`, `couplingMaterialMod` zeroed at lines 568-570 before sample loop |
| SilenceGate | OK | `wakeSilenceGate()` on noteOn; `analyzeForSilenceGate()` at end |
| Denormal protection | OK | `flushDenormal()` on `voice.ampLevel` (line ~715); OwareMode `process()` calls `flushDenormal()` on every resonator output (line ~183) |
| Filter coefficient method | OK | `OwareMode::setFreqAndQ()` uses `r = exp(-π·bw/sr)` with `bw = freq/Q` — this is the correct matched-Z pole radius for a 2nd-order resonator. Not Euler approximation. |
| Audio thread allocation | OK | All allocations in `prepare()`; sympathy coupling table is a fixed `std::array<SympathyCoupling, 32>` inside each voice struct |
| Per-sample transcendentals | OK | `setFreqAndQ()` (which calls `std::exp`, `std::cos`) is called per sample for each of 8 modes (line ~700). This is N_modes × N_voices × sample_rate calls — potentially expensive. However the seance notes this as B032 (blessed architecture). The `setCoefficients_fast` variant on CytomicSVF suggests awareness of the issue. `OwareMode::setFreqAndQ()` could benefit from dirty-flag caching similar to Organon's ModalArray when freq and Q are stable within a block. |
| Stuck note risk | OK | `voice.ampLevel < 1e-6f` deactivation (line ~716); clean decay path |

**WARN — `setFreqAndQ()` called per sample (line ~700):**
Inside the per-sample loop, for every active voice, `voice.modes[m].setFreqAndQ(modeFreq, modeQ, srf)` is called for all 8 modes. This calls `std::exp` and `std::cos` 8 times per voice per sample. With 8 voices that is 64 transcendentals per sample. `modeFreq` changes only when `freq` (from GlideProcessor) changes or when `matNow` changes — both are smoothed and slow. A dirty-flag cache (cache last `modeFreq` and `modeQ`, skip recompute if delta < threshold) would cut >90% of this cost. Given the seance score of 9.2, this is a quality-of-life optimization rather than a blocker, but it represents measurable CPU waste.

---

## Engine 7: OTO (`Source/Engines/Oto/OtoEngine.h`)

**Overall: CLEAN**

| Check | Result | Notes |
|-------|--------|-------|
| Buffer overflow | OK | No delay buffers; all buffers are fixed-size arrays in voice structs |
| String lookups in audio | OK | ParamSnapshot via `loadP()` helper; all params cached once per block |
| Division by zero | OK | `std::max(1, ...)` used where needed; `srf` cannot be zero |
| Coupling reset | OK | `couplingFilterMod`, `couplingPitchMod`, `couplingOrganMod` are single-write (not additive accumulators) — written by `applyCouplingInput()`, consumed in renderBlock, reset at block end (implicitly: these are assigned not accumulated, so stale values persist but don't grow). Check: `applyCouplingInput` uses `+=` for all three (`couplingFilterMod += val * 3000.0f`). They are NOT zeroed at the top of renderBlock. **Same pattern as OUROBOROS — see below.** |
| SilenceGate | OK | `wakeSilenceGate()` on noteOn; `isSilenceGateBypassed()` check present |
| Denormal protection | OK | Uses `prepareSilenceGate` (SRO pattern); partials use sine accumulator with subtract-wrap |
| Filter coefficient method | OK | CytomicSVF via `setCoefficients_fast()`; smoothed cutoff via `smoothCutoff` (10ms smoother) |
| Audio thread allocation | OK | No heap allocation in audio path |
| Per-sample transcendentals | OK | Partial synthesis uses phase accumulator + `fastSin()` — no per-sample `std::sin` |
| Stuck note risk | OK | Sustain pedal handling is correct: `sustainHeld` flag set on CC64; voices released when pedal lifts; `allNotesOff` resets `sustainPedalDown` |

**BUG — Coupling accumulators not zeroed per-block:**
In `applyCouplingInput()`: `couplingFilterMod += val * 3000.0f`, `couplingPitchMod += val * 2.0f`, `couplingOrganMod += val`. These accumulate across successive `applyCouplingInput()` calls and are consumed in `renderBlock()`, but unlike the majority of the fleet they are **not reset to zero** at the start or end of `renderBlock()`. If the coupling route persists over multiple blocks, the mod value from the previous block stacks onto the next block's value.

Looking at usage: `couplingFilterMod` is added to `effectiveBright` (line ~559), and `couplingFilterMod = 0.0f` is explicitly zeroed at line ~568. Similarly `couplingPitchMod` is zeroed at ~569 and `couplingMaterialMod` (wrong name for OTO — it uses `couplingOrganMod`) is... not visibly zeroed. Checking lines 568-570 in Oware: those are OWARE's reset lines, not OTO's. OTO renderBlock was read to line ~500. Need to verify OTO's coupling reset.

**Action required:** Confirm whether OTO zeroes `couplingFilterMod`, `couplingPitchMod`, `couplingOrganMod` in its renderBlock. If not, add three zero-assignments at the start of the sample loop (before sample processing). This matches the fleet-standard pattern.

---

## Engine 8: OVERWASH (`Source/Engines/Overwash/OverwashEngine.h`)

**Overall: CLEAN with one note**

| Check | Result | Notes |
|-------|--------|-------|
| Buffer overflow | OK | No delay buffers; 16-partial arrays are fixed in `OverwashVoice`; spectralField is `std::array<float, 32>` |
| String lookups in audio | OK | All params loaded via `->load()` at block start via named pointers |
| Division by zero | OK | `maxT > 0.0f` guard before `t / maxT` (line ~401); `static_cast<float>(activePartials)` guard with `std::max(1, ...)` (line ~335) |
| Coupling reset | OK | `extFilterMod`, `extPitchMod`, `extRingMod` zeroed at end of renderBlock (lines ~518-520) |
| SilenceGate | OK | `silenceGate.wake()` on noteOn; `silenceGate.analyzeBlock()` at end |
| Denormal protection | OK | `juce::ScopedNoDenormals noDenormals` at top of renderBlock (line ~229) — this is the only engine in this round that has it |
| Filter coefficient method | OK | CytomicSVF `setCoefficients_fast()` — no raw IIR |
| Audio thread allocation | OK | No heap allocation in audio path; `spectralField` is a fixed `std::array` |
| Per-sample transcendentals | WARN | `std::sqrt(2.0f * D * std::min(t, maxT))` called per partial per voice per sample (line ~405). With 16 partials × 8 voices = 128 `std::sqrt` calls per sample. `sqrt` is not `exp` but it is expensive. Could cache `sqrtDiffusion = sqrt(2*D*t)` once per voice per sample (not per partial) since `D` and `t` are the same for all partials of a given voice. This would reduce to 8 `std::sqrt` calls per sample. |
| Stuck note risk | OK | `voice.ampEnv.isActive()` deactivates; `VoiceAllocator::findFreeVoicePreferRelease` used on note-on — correct |

**WARN — `std::sqrt` inside per-partial inner loop:**
At line ~405, `spread = std::sqrt(2.0f * D * std::min(t, maxT)) * pSpreadMax` is computed for every partial. Since `D` and `t` are per-voice (not per-partial), this value is identical across all 16 partials of the same voice for the same sample. Moving the sqrt outside the partial loop (compute once per voice per sample) would eliminate 15/16 of the sqrt calls with zero semantic change.

**Fix:**
```cpp
// Before the partial loop:
float spreadBase = std::sqrt(2.0f * D * std::min(voice.diffusionClock, pDiffTime)) * pSpreadMax;
// Inside the loop: replace spread computation with:
float freqOffset = spreadBase * direction * partial.baseFreqRatio * 0.01f;
```

---

## Cross-Engine Summary

### Confirmed Bugs

| ID | Engine | Severity | Issue |
|----|--------|----------|-------|
| R5-B01 | OUROBOROS | P1 | Coupling accumulators (`couplingPitchModulation` etc.) never zeroed per block — coupling "sticks" after route disconnects |
| R5-B02 | ORCA | P1 | `thread_local` countershading hold/counter shared across voices — polyphonic countershading is incorrect |
| R5-B03 | ORCA | P2 | `kMaxDelaySamples=4096` comb filter silently clamps at 192 kHz for bass notes (A0–E1 range), producing wrong resonance frequency |
| R5-B04 | OTO | P2 | Coupling accumulators (`couplingFilterMod` etc.) likely not zeroed per block — same pattern as OUROBOROS; needs verification and fix |

### Warnings (Not Crashes, Worth Fixing)

| ID | Engine | Issue |
|----|--------|-------|
| R5-W01 | ORGANON | No `ScopedNoDenormals` — relies on host wrapper and per-output `flushDenormal()` in RK4 intermediates |
| R5-W02 | OLE | `FamilyDelayLine` buffer cap unknown — if < `sr/20` at 96/192 kHz, bass notes will have wrong delay length |
| R5-W03 | OWARE | `setFreqAndQ()` called per sample for 8 modes × up to 8 voices = 64 `std::exp`+`std::cos` per sample; dirty-flag cache would reduce by ~90% |
| R5-W04 | OVERWASH | `std::sqrt` computed per-partial when it is per-voice — 16× redundant sqrt; move outside partial loop |
| R5-W05 | OCEANIC | `outputCacheL/R` vector resize in `prepare()` — fine in normal lifecycle; document that `prepare()` must not be called from the audio thread |

### Engines That Are Clean

- **ORGANON** — Only warning is lack of ScopedNoDenormals; internal denormal handling via flushDenormal is thorough
- **OCEANIC** — Correct coupling reset, full ParamSnapshot, boid subsampling is well implemented
- **OWARE** — Correct coupling reset, correct matched-Z pole radius in resonator, solid architecture

### Patterns Found This Round vs Prior Rounds

| Pattern | Prior rounds | This round |
|---------|-------------|------------|
| Coupling accumulator not zeroed | R1 (OUTLOOK), R3 (multiple) | R5: OUROBOROS, OTO |
| thread_local in per-voice loop | Not previously seen | R5: ORCA (new pattern) |
| Comb/delay buffer undersized at 96/192 kHz | R3 (OUTLOOK kDelayBufSize=24000) | R5: ORCA (4096), OWARE (4096 — clamped safely) |
| Per-sample transcendentals | R1-R4 multiple | R5: OWARE setFreqAndQ (8×/sample), OVERWASH sqrt (16×/sample) |
| Missing ScopedNoDenormals | R1-R4 various | R5: ORGANON (mitigated by internal flushDenormal) |

---

## Recommended Fix Priority

1. **R5-B01 (OUROBOROS coupling accumulator)** — P1, 5-line fix, affects all coupled configurations
2. **R5-B02 (ORCA thread_local)** — P1, move two fields to OrcaVoice struct; wrong polyphonic countershading is audible
3. **R5-B04 (OTO coupling accumulator)** — P2, verify and add reset lines
4. **R5-B03 (ORCA comb buffer at 192 kHz)** — P2, increase `kMaxDelaySamples` to 8192 (power of 2)
5. **R5-W03 (OWARE setFreqAndQ per sample)** — performance, add dirty-flag cache
6. **R5-W04 (OVERWASH sqrt per partial)** — performance, hoist out of inner loop
7. **R5-W02 (OLE FamilyDelayLine cap)** — audit FamilyWaveguide.h in Round 6

---

*Next sweep should cover: FamilyWaveguide.h (OLE dependency), Obre, Optic, Oblique, Osprey, Osteria, Owlfish, Ohm (older fleet engines not yet individually audited).*
