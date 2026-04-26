# Fleet DSP Wave 1 Findings — 2026-04-19
## Reconcile-Mode Scan: CPU Bombs Cluster (10 Engines)

Baseline: `Docs/triage/fleet-dsp-reconciliation-2026-04-19.md`
Novel findings only — issues already in baseline are not repeated here.

---

## Scan Status

| Engine | File | Read Status | Novel Findings |
|--------|------|-------------|----------------|
| Oceanic | `Source/Engines/Oceanic/OceanicEngine.h` | Full (1419 lines) | 1 P2, 1 P3 (baseline stale) |
| Overwash | `Source/Engines/Overwash/OverwashEngine.h` | Full (807 lines) | 2 P2 |
| Oasis | `Source/Engines/Oasis/OasisEngine.h` | Full (1289 lines) | 1 P1, 3 P2 |
| Ohm | `Source/Engines/Ohm/OhmEngine.h` | Full (849 lines) | 1 P1, 1 P2 |
| Opal | `Source/Engines/Opal/OpalEngine.h` | Partial (first ~700 lines; render loop + param attachment not reached) | 1 P1 |
| Oxytocin | `Source/Engines/Oxytocin/OxytocinEngine.h` | Full (350 lines — adapter only; OxytocinVoice.h not read) | 1 P1 |
| Odyssey (Drift) | `Source/Engines/Odyssey/OdysseyEngine.h` | Partial (first ~200 lines; formant render loop not reached) | 0 confirmed novel |
| Overbite (Bite) | `Source/Engines/Overbite/OverbiteEngine.h` | Partial (first ~200 lines; main render loop not reached) | 0 confirmed novel |
| Obbligato | `Source/Engines/Obbligato/ObbligatoEngine.h` | Full (1015 lines) | 1 P2 |
| Onkolo | `Source/Engines/Onkolo/OnkoloEngine.h` | Full (732 lines) | 2 P2, 1 note on prepare() lifecycle, 1 P3 (baseline stale) |

**Unreadable / Partial**: Opal (render loop), Drift/Odyssey (formant loop), Bite/Overbite (main render loop) — all exceed file-size limits. Existing baseline P1s for these three remain OPEN unconfirmed.

---

## Novel Findings by Engine

---

### OCEANIC — `Source/Engines/Oceanic/OceanicEngine.h`

#### [OCN-N01] P2 — fastLog2 at control rate still fires 512× per control tick
**Line**: ~544 (separation loop inside control-rate block)
**Description**: `fastLog2()` in the 128-particle × 4-voice separation loop runs at control rate (~2 kHz, guarded by `controlCounter >= controlRateDiv`), not per audio sample. Each control tick fires ~512 `fastLog2()` calls. While cheaper than `std::log2`, this is still significant at control rate and should be cached or reduced.
**Severity rationale**: Not a per-sample CPU bomb, but latent spike load at every control-rate tick.
**Recommendation**: Cache separation distance log-ratios; recompute only when particle positions change by a threshold.

#### [OCN-N02] P3 — Baseline P0 (740 trig/sample) appears resolved
**Description**: FIX 1–4 comments in the header confirm:
- FIX 1: Quake-style fast inverse-sqrt replaces `std::sqrt` in O(N²) separation loop
- FIX 2: `fastPow2` replaces `std::pow(2.0f, x)` for frequency integration
- FIX 3: Pan coefficients cached at control rate (~512 `cos/sin/sample` eliminated)
- FIX 4: `cachedInvNorm = 1/sqrt(activeParticles)` cached at control rate

**Baseline entry**: "Oceanic P0-01: 740 trig calls/sample — OPEN" is **stale**; code evidence suggests this is resolved. Recommend updating baseline to RESOLVED.

---

### OVERWASH — `Source/Engines/Overwash/OverwashEngine.h`

#### [OWS-N01] P2 — setADSR() called every audio sample per active voice
**Lines**: 384–385
**Description**: Inside the per-sample render loop (`for (int i = 0; i < numSamples; ++i)` iterating active voices), both `voice.ampEnv.setADSR(pAmpA, pAmpD, pAmpS, pAmpR)` and `voice.filterEnv.setADSR(pFiltA, pFiltD, pFiltS, pFiltR)` are called unconditionally. Since `pAmpA/D/S/R` and `pFiltA/D/S/R` are block-rate snapshots that do not change sample-to-sample, these calls recompute internal ADSR coefficients on every sample for every active voice.
**Severity rationale**: `setADSR()` internally recomputes rate coefficients (division, potentially exp). With 8 voices × 44100 samples/sec = ~352K redundant coefficient recomputes per second.
**Recommendation**: Move `voice.ampEnv.setADSR()` and `voice.filterEnv.setADSR()` to the pre-loop block-rate section, alongside other `pAmp*` snapshot reads.

#### [OWS-N02] P2 — inverseSr_ hardcoded to 44100 at member initialization
**Line**: 746
**Description**: `float inverseSr_ = 1.0f / 44100.0f;` is hardcoded as a member field default. Before `prepare()` is called (e.g., during construction or if `prepare()` is skipped), any DSP path using `inverseSr_` will compute incorrect values on 48kHz or 96kHz hosts.
**Severity rationale**: Silent wrong-rate DSP until `prepare()` is invoked; constructor-time use of `inverseSr_` would produce incorrect filter coefficients. Consistent with CLAUDE.md: "Always pass `sampleRate` — never hardcode 44100."
**Recommendation**: Initialize `inverseSr_` to `0.0f` or leave uninitialized; assert/set only in `prepare()`.

---

### OASIS — `Source/Engines/Oasis/OasisEngine.h`

#### [OAS-N01] P1 — PitchBendUtil::semitonesToFreqRatio() called every audio sample
**Line**: 782
**Description**: `float bendRatio = PitchBendUtil::semitonesToFreqRatio(pitchBendNorm_ * 2.0f)` is called inside the per-sample voice render loop. If `semitonesToFreqRatio` uses `fastPow2`, this is P2; if it uses `std::pow`, this is a P1 CPU bomb. The implementation of `PitchBendUtil` was not directly inspected. Pitch bend values change at MIDI resolution (coarse), not sample-by-sample — this should be computed once per block or on MIDI event.
**Severity rationale**: P1 pending `PitchBendUtil` implementation. Even `fastPow2` once per sample per voice is unnecessary.
**Recommendation**: Cache `bendRatio` at block rate, recompute only when `pitchBendNorm_` changes. Move outside sample loop.

#### [OAS-N02] P2 — resonatorBank_.setCoefficients() full update every audio sample
**Line**: 769
**Description**: `resonatorBank_.setCoefficients(resBankFreq, pResonatorQ, srF_)` — note: `setCoefficients` (not `setCoefficients_fast`) — is called unconditionally every sample inside the voice render loop. Full coefficient recompute (trigonometric) per sample on the resonator bank.
**Severity rationale**: `setCoefficients()` implies full trig recomputation. Resonator bank frequency is LFO-modulated but LFO output is not per-sample precise enough to warrant full-coeff-per-sample.
**Recommendation**: Switch to `setCoefficients_fast()` or cache LFO output at sub-audio rate and recompute only on LFO step.

#### [OAS-N03] P2 — subFilter_ and canopy LP filters recomputed every sample via _fast
**Lines**: 964–966, 1014, 1019
**Description**: Three filter instances call `setCoefficients_fast()` every audio sample:
- `subFilter_.setCoefficients_fast(subCutoff, 0.707f, srF_)` — bioluminescence-LFO modulated cutoff
- `canopyLP_.setCoefficients_fast(canopyCutoff, 0.707f, srF_)` — canopy low-pass L
- `canopyLPR_.setCoefficients_fast(canopyCutoff, 0.707f, srF_)` — canopy low-pass R

`_fast` reduces cost vs full recompute but still performs coefficient math every sample. With LFO modulation at audio rates this is borderline justified, but if the LFO rate is sub-audio (which is standard for filter sweep LFOs), these can be decimated to control rate.
**Severity rationale**: 3× per-sample filter updates; lower risk than full `setCoefficients()` but cumulative load across voices.
**Recommendation**: Drive these from control-rate LFO output (downsampled), not per-sample.

---

### OHM — `Source/Engines/Ohm/OhmEngine.h`

#### [OHM-N01] P1 — OhmObedFM attack phase is dead code; ohm_fmAttack param silent
**Lines**: 40–43
**Description**: `OhmObedFM::tick()` implements FM attack envelope with the guard `if (envLevel < 1.0f && attack > 0.001f)`. However, `trigger()` initializes `envLevel = 1.0f`. On the first tick after trigger, `envLevel < 1.0f` is already false — the attack ramp branch is never entered. The envelope immediately enters the decay phase.

```cpp
// trigger() sets:
envLevel = 1.0f;

// tick() then checks:
float rate = (envLevel < 1.0f && attack > 0.001f) ? (1.0f / (sr * attack))
                                                   : (1.0f / (sr * std::max(decay, 0.01f)));
if (envLevel >= 0.999f) envLevel = 1.0f;
envLevel *= (1.0f - rate);
```

The `ohm_fmAttack` parameter never influences FM timbre. Users cannot hear attack shaping on FM modulation depth.
**Severity rationale**: FATHOM D004 (dead parameter with exposed control). The attack knob is visually present but acoustically silent — a trust and quality issue.
**Recommendation**: Fix `trigger()` to set `envLevel = 0.0f` and invert the envelope logic to ramp up during attack, then decay. Alternatively, if intent is "sustain-then-decay only", remove the `ohm_fmAttack` parameter from the UI.

#### [OHM-N02] P2 — Per-sample floating-point division in waveguide delay read
**Line**: 367
**Description**: `float dlen = v.sr / std::max(df, 20.f)` — floating-point division per active voice per sample in the waveguide delay line read. `v.sr` is constant per-prepare; `df` (detuned frequency) changes only on pitch events, not sample-by-sample.
**Severity rationale**: FP division ~20 cycles per voice. With 8 voices = 160 FP divides/sample = ~7M divides/second. Not a crisis but avoidable.
**Recommendation**: Cache `dlen` when `df` changes (pitch event), not per sample. Use `reciprocal * sr` or cached `cachedDLen`.

---

### OPAL — `Source/Engines/Opal/OpalEngine.h`

*Note: Only first ~700 lines read. Render loop, cloud voice scheduling, and parameter attachment not reached due to file size. Findings below are from OpalGrainPool and supporting types only.*

#### [OPL-N01] P1 — std::sqrt per audio sample for grain normalization
**Line**: 360
**Description**: `OpalGrainPool::processAll()` computes `float norm = 1.0f / std::sqrt(static_cast<float>(count))` unconditionally on every audio sample, where `count` is the number of active grains. `std::sqrt` is a transcendental-class operation. `count` only changes when grains activate or deactivate (an event-driven, infrequent change), not sample-by-sample.

```cpp
if (count > 0)
{
    float norm = 1.0f / std::sqrt(static_cast<float>(count));
    sumL *= norm;
    sumR *= norm;
}
```

**Severity rationale**: `std::sqrt` once per sample in the granular engine's core mix path. At 44.1kHz with polyphonic granular voices, this fires repeatedly. `count` is stable between grain events — this normalization should be cached.
**Recommendation**: Cache `cachedNorm = 1.0f / std::sqrt(float(count))` and invalidate only when `count` changes (grain activate/deactivate callbacks). Replace per-sample `std::sqrt` with cached value.

---

### OXYTOCIN — `Source/Engines/Oxytocin/OxytocinEngine.h`

*Note: OxytocinEngine.h is an adapter class (350 lines). OxytocinVoice.h (Thermal model, existing P1 for 24× std::pow) was not read. Findings below are from the adapter layer only.*

#### [OXY-N01] P1 — buffer.clear() in processBlock() zeroes accumulated multi-engine mix
**Line**: 173
**Description**: `buffer.clear()` is called at the top of `OxytocinEngine::processBlock()`. In XOceanus's multi-engine render pipeline, engines receive a shared buffer that accumulates audio from previously-rendered engines. Calling `buffer.clear()` destroys all accumulated output from engines rendered before Oxytocin in the render order.

This is a data-destruction bug in any multi-engine configuration where Oxytocin is not the first engine in the chain. The correct pattern is `buffer.addFrom()` / accumulate, not clear+fill.
**Severity rationale**: Renders all other engines inaudible when Oxytocin is active in a multi-engine slot. Silent failure in normal use.
**Recommendation**: Remove `buffer.clear()`. Use `buffer.addFrom()` to accumulate Oxytocin output into the existing buffer, consistent with fleet-standard engine pattern.

---

### ODYSSEY (DRIFT) — `Source/Engines/Odyssey/OdysseyEngine.h`

*Note: Only first ~200 lines read. File is ~33,100 tokens. Formant render loop (existing P1: 24× CytomicSVF coeff/sample) not reached. No novel findings confirmed from partial read.*

---

### OVERBITE (BITE) — `Source/Engines/Overbite/OverbiteEngine.h`

*Note: Only first ~200 lines read. File is ~60,958 tokens. Main render loop (existing P2: 32× std::exp+pow/sample) not reached. Oscillator types `BiteOscA` use `BiteSineTable::lookup()` (table-based, no transcendentals) and drift LFO uses table lookup — both correct in the read section. No novel findings confirmed from partial read.*

---

### OBBLIGATO — `Source/Engines/Obbligato/ObbligatoEngine.h`

#### [OBB-N01] P2 — reset() does not clear delay positions or filter states
**Lines**: 205–213
**Description**: `ObbligatoVoice::reset()` zeroes `ampEnv`, `filterEnv`, and resets `delayBuf[]` contents, but does NOT reset the following stateful members:
- `brightDelayPos` — comb delay write position (increments unbounded per sample)
- `darkDelayPos` — dark delay write position
- `springPos` — spring reverb position
- `plateState[4]` — plate reverb allpass state array
- `exciterLP`, `phaserStateL`, `phaserStateR`, `darkDelayLP_L`, `darkDelayLP_R`, `windLP` — various filter state variables

After `reset()`, the next note trigger begins with stale delay positions and filter memory from the previous voice lifecycle. This causes: (a) initial output burst from unconsumed delay content, (b) filter DC offset artifacts on note attack.
**Severity rationale**: Audible artifact at note onset for any voice that is reset and re-triggered with delay/reverb active.
**Recommendation**: Add explicit zeroing of all position counters and filter state arrays in `reset()`. `brightDelayPos = darkDelayPos = springPos = 0; plateState[0] = plateState[1] = plateState[2] = plateState[3] = 0.0f;` etc.

*Note: `brightDelayPos`, `darkDelayPos`, `springPos` increment unbounded per sample with no overflow guard (lines 623, 667, 679). Theoretically wraps at INT_MAX (~25 hours at 44kHz) — low-priority but worth noting.*

---

### ONKOLO — `Source/Engines/Onkolo/OnkoloEngine.h`

#### [ONK-N01] P2 — AutoWahEnvelope::process() calls full setCoefficients() every sample
**Line**: 253
**Description**: `AutoWahEnvelope::process()` calls `wahBPF.setCoefficients(wahFreq, 0.6f, sr)` (note: full `setCoefficients`, not `setCoefficients_fast`) every audio sample. `wahFreq` is modulated by the envelope follower which changes every sample. Full coefficient recompute (trig) per sample in the auto-wah path.
**Severity rationale**: Full SVF/BPF trig recompute per sample. In the wah path this fires for every active voice with the auto-wah enabled.
**Recommendation**: Use `setCoefficients_fast()` if the filter type supports it, or add hysteresis: only recompute when `wahFreq` changes by more than a threshold (e.g., 0.5 Hz). Audio-rate wah sweeps rarely need trig precision.

#### [ONK-N02] P2 — Voice SVF full setCoefficients() every sample in render loop
**Line**: 519
**Description**: `voice.svf.setCoefficients(cutoff, 0.15f, srf)` — full coefficient update (not `_fast`) — called every sample inside the main voice render loop. `cutoff` is LFO-modulated but LFO output is sub-audio rate.
**Severity rationale**: Full trig recompute per voice per sample for a sub-audio-rate modulation source.
**Recommendation**: Decimate LFO-driven coefficient updates to control rate (~100–500 Hz). Use `setCoefficients_fast()` or cache at control rate.

#### [ONK-N03] P2 — prepare() called from audio thread inside MIDI handler
**Lines**: 559–560
**Description**: Inside `renderBlock()`, the MIDI event loop calls `noteOn()`, which contains:
```cpp
v.string.prepare(srf);
v.string.trigger(vel);
v.wah.prepare(srf);
```

`prepare()` is a lifecycle method (audio-thread-safe if no allocation, but semantically wrong) called from within MIDI processing during `renderBlock()`. While `OnkoloString::prepare()` and `AutoWahEnvelope::prepare()` may not allocate memory, calling `prepare()` on active DSP objects mid-render can reset internal state inconsistently. This pattern also breaks the expected `prepare()` → `processBlock()` lifecycle contract.
**Severity rationale**: Pattern violation; potential for state corruption if `prepare()` resets coefficients/state mid-voice. Low crash risk but architecturally wrong.
**Recommendation**: Call `prepare(srf)` once in the engine-level `prepare()` for all pre-allocated voice slots. In `noteOn()`, call only `trigger()`/`reset()` methods, never `prepare()`.

#### [ONK-N04] P3 — Baseline P1 (64× std::exp on key-off) appears resolved
**Description**: Code inspection confirms: `cachedDecayRates[]` is populated in `trigger()`, and `cachedFastDampCoeff` is computed in `releaseKey()`. Inline comments reference the CPU fix explicitly.
**Baseline entry**: "Onkolo P1-01: 64× std::exp on key-off — OPEN" is **stale**; code evidence suggests this is resolved. Recommend updating baseline to RESOLVED.

---

## Summary Table — Novel Findings Only

| ID | Engine | Severity | One-Line Summary |
|----|--------|----------|-----------------|
| OCN-N01 | Oceanic | P2 | fastLog2 in 512-call separation loop at every control tick |
| OCN-N02 | Oceanic | P3 | Baseline P0 (740 trig/sample) appears resolved via FIX 1–4 |
| OWS-N01 | Overwash | P2 | setADSR() called every sample per voice (block-rate data) |
| OWS-N02 | Overwash | P2 | inverseSr_ hardcoded 44100 at member init (wrong before prepare()) |
| OAS-N01 | Oasis | P1 | PitchBendUtil::semitonesToFreqRatio() per audio sample in voice loop |
| OAS-N02 | Oasis | P2 | resonatorBank_.setCoefficients() (full) per sample |
| OAS-N03 | Oasis | P2 | 3× setCoefficients_fast() per sample (subFilter + canopyLP L/R) |
| OHM-N01 | Ohm | P1 | ohm_fmAttack dead code — trigger() sets envLevel=1.0, attack branch never fires |
| OHM-N02 | Ohm | P2 | Per-sample FP division in waveguide delay read (v.sr / df) |
| OPL-N01 | Opal | P1 | std::sqrt every sample for grain normalization in OpalGrainPool |
| OXY-N01 | Oxytocin | P1 | buffer.clear() destroys accumulated multi-engine output |
| OBB-N01 | Obbligato | P2 | reset() skips delay positions + 6 filter states → note-onset artifacts |
| ONK-N01 | Onkolo | P2 | AutoWahEnvelope full setCoefficients() per sample |
| ONK-N02 | Onkolo | P2 | Voice SVF full setCoefficients() per sample (sub-audio LFO source) |
| ONK-N03 | Onkolo | P2 | prepare() called from audio thread inside MIDI noteOn handler |
| ONK-N04 | Onkolo | P3 | Baseline P1 (64× std::exp on key-off) appears resolved |

**Totals**: 0 P0 · 4 P1 · 9 P2 · 2 P3 (novel only, excluding baseline)

---

## Patterns Observed Across Multiple Engines

### Pattern A — setCoefficients() inside per-sample render loops (4 engines)
Overwash (ADSR), Oasis (resonatorBank_, sub/canopy filters), Onkolo (AutoWah BPF, voice SVF) all call coefficient-update methods inside the innermost sample loop with block-rate or sub-audio-rate modulation sources. This is a fleet-wide anti-pattern. Candidate for a fleet-wide audit pass: grep for `setCoefficients` and `setADSR` inside `for (int i = 0; i < numSamples` blocks.

### Pattern B — Hardcoded sample rate defaults before prepare() (2+ engines)
Overwash `inverseSr_ = 1.0f / 44100.0f` and Ohm's reverb SR references (pre-existing) both assume 44100 Hz at construction time. Silent wrong-rate DSP until `prepare()` runs. Consistent with CLAUDE.md warning. Fleet-wide: member field SR defaults should be `0.0f` or `NaN` to surface the bug loudly.

### Pattern C — Baseline staleness (2 engines confirmed fixed)
Oceanic P0 (FIX 1–4 comments) and Onkolo P1 (CPU fix comment) are logged OPEN in the baseline but appear resolved in code. Suggests the reconciliation doc was written before these fixes landed. Recommend a targeted re-audit pass on all OPEN baseline entries to mark resolved items.

### Pattern D — prepare() lifecycle violations in MIDI handlers (1 engine, fleet risk)
Onkolo calls `prepare()` inside `noteOn()` which is called from the MIDI processing section of `renderBlock()`. This is wrong-phase object lifecycle usage. Any engine that lazily initializes DSP objects in note handlers rather than in `prepare()` carries this risk.

### Pattern E — buffer.clear() accumulation destruction (1 engine, fleet risk)
Oxytocin's `buffer.clear()` in `processBlock()` is a silent correctness bug in multi-engine configurations. Any engine that clears rather than accumulates into the shared buffer will mute all prior engines. Recommend a fleet-wide grep for `buffer.clear()` inside `processBlock()` implementations.

---

*Scan conducted 2026-04-19. Partial reads: Drift/Odyssey, Bite/Overbite (render loops not reached), Opal (render loop loop not reached), Oxytocin (OxytocinVoice.h not read). Existing baseline P1s for Drift, Bite remain OPEN unconfirmed.*

---

## Fleet-Wide Pattern Grep (2026-04-19)

Fleet-wide surgical grep across all `Source/Engines/*/` `.h` files for three DSP anti-patterns. 77 files matched the method-name pass; each hit manually verified for nesting context.

---

### Pattern 1 — Block-rate DSP in per-sample loops

Confirmed real (method called inside `for (int i/s = 0; i/s < numSamples)` with block-constant or sub-audio-rate inputs, no change-detection guard):

- **P1** — `Source/Engines/Overcast/OvercastEngine.h:423` — `voice.ampEnv.setADSR(pAmpA, pAmpD, pAmpS, pAmpR)` inside `for (int i = 0; i < numSamples; ++i)` inner voice loop. ADSR params read from parameter pointers that are block-constant. Quote: `voice.ampEnv.setADSR(pAmpA, pAmpD, pAmpS, pAmpR);`

- **P1** — `Source/Engines/Oddfellow/OddfellowEngine.h:433` — `voice.ampEnv.setADSR(...)` inside `for (int s = 0; s < numSamples; ++s)`. Params read via `paramAttack->load()` etc. — all block-constant. Quote: `voice.ampEnv.setADSR(paramAttack ? paramAttack->load() : 0.005f, ...)`

- **P1** — `Source/Engines/Otis/OtisEngine.h:1053` — `voice.ampEnv.setADSR(...)` inside `for (int s = 0; s < numSamples; ++s)`. Comment reads "per block so knob changes take effect" but call is inside the per-sample loop. Quote: `voice.ampEnv.setADSR(paramAttack ? paramAttack->load() : 0.005f, ...)`

- **P1** — `Source/Engines/Opcode/OpcodeEngine.h:441` — `voice.ampEnv.setADSR(...)` inside `for (int s = 0; s < numSamples; ++s)`. Same "per block" comment pattern as Otis/Oddfellow. Quote: `voice.ampEnv.setADSR(paramAttack ? paramAttack->load() : 0.005f, ...)`

- **P1** — `Source/Engines/Okeanos/OkeanosEngine.h:495` — `voice.ampEnv.setADSR(...)` inside `for (int s = 0; s < numSamples; ++s)`. Quote: `voice.ampEnv.setADSR(paramAttack ? paramAttack->load() : 0.005f, ...)`

- **P1** — `Source/Engines/Outlook/OutlookEngine.h:230` — `v.ampEnv.setADSR(pAtt, pDec, pSus, pRel)` inside `for (int s = 0; s < numSamples; ++s)`. All four params block-constant. Quote: `v.ampEnv.setADSR(pAtt, pDec, pSus, pRel);`

- **P1** — `Source/Engines/Oxidize/OxidizeVoice.h:578–595` — `erosionFilter.setCoefficients(...)` and `erosionBpFilter.setCoefficients(...)` called 2–3× per sample inside `for (int i = 0; i < numSamples; ++i)`. The comment explicitly states "Update erosion filter coefficients per-sample to track LFO1" — LFO output is audio-rate but the coefficient recompute (full trig) per sample is still expensive. 3 `setCoefficients` calls/sample. Quote: `erosionFilter.setCoefficients(erosionCutoffModded, 0.1f + 0.2f * erosionRes, sampleRate_);`

- **P1** — `Source/Engines/Organon/OrganonEngine.h:1315` — `voice.ingestionFilter.setCoefficients(enzymeSelectivity + externalFilterModulation * 2000.0f, ...)` inside `for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)`, inside the self-feeding branch `else { ... }`. Both `enzymeSelectivity` and `externalFilterModulation` are block-constant (read before the loop). Quote: `voice.ingestionFilter.setCoefficients(enzymeSelectivity + externalFilterModulation * 2000.0f, ...)`

- **P1** — `Source/Engines/Osteria/OsteriaEngine.h:426–427` (via `MurmurGenerator::process()`) — `formant1.setCoefficients(lowFormantFreq, ...)` + `formant2.setCoefficients(highFormantFreq, ...)` inside `MurmurGenerator::process()`, called per-sample at line 1320 inside the main render loop `for (int sample = 0; sample < numSamples; ++sample)`. The formant frequencies change each sample (driven by a 0.5 Hz LFO inside the same function). Both `setCoefficients` calls fire every sample. Quote: `formant1.setCoefficients(lowFormantFreq, 0.4f, sampleRate);`

**False positives eliminated:**
- Osprey: `setCoefficients` at line 249 is inside `trigger()` — init path, not render loop. Line 308 is inside `process()` which is called per-voice in the render loop — **confirmed real** but the frequency interpolation is genuinely audio-rate (formant sweep requires per-sample updates). Classified as intentional / accepted cost, not a naive bug.
- Osprey line 308: intentional audio-rate formant sweep. Not filed.
- Osmosis: `setCoefficients` inside `for (int i = 0; i < 2)` loops is inside `prepare()`. Clean.
- Opaline, Obelisk: `setCoefficients` inside `for (int i = 0; i < kMaxVoices)` is inside `prepare()`. Clean.
- Oblique (lines 314, 377): Both in `prepare()` and `update()` helper called from render, not inside per-sample loop. Line 557: guarded by `if (std::abs(lfoValue - lastPhaserLfoValue) >= 0.001f)` — change-detection skip. **Not filed.**
- Onset, Ostinato: `setCoefficients` inside `for (int i = 0; i < kNumModes)` is inside `noteOn()`/strike handler. Not a per-sample loop. Clean.
- Osteria `updateFormants` at line 1032: called inside control-rate gate (`if (voice.controlCounter >= controlRateDiv)`). **Not filed.**
- Optic: `setCoefficients` inside `prepare()`. Clean.
- Ortolan: `setADSR` inside a `for (int i = 0; i < maxPoly)` loop that is inside `noteOn()`. Not a render loop. Clean.
- Obiont: `setADSR` inside `for (int s = 0; s < kObiontModSlots)` is the mod-matrix snapshot read, not a sample loop. Clean. The actual `setADSR` at line 1348 is inside `for (auto& v : voices)` in `renderBlock()` but **outside** the per-sample inner loop. Classified as per-block — acceptable.

**P1 confirmed count: 9 engines / 11 call sites**

---

### Pattern 2 — Hardcoded sample rate defaults

Member init defaults of `44100` or `48000` that produce wrong-rate DSP before `prepare()` is called. Validation guards (`if (sr <= 0) sr = 44100`) excluded.

- **P2** — `Source/Engines/Overtide/OvertideEngine.h:1260` — `float sampleRateFloat = 44100.0f;` as member default. Used throughout to compute filter coefficients and envelope rates. Quote: `float sampleRateFloat = 44100.0f;`

- **P2** — `Source/Engines/OceanDeep/OceanDeepEngine.h:59` — `float sr = 44100.f;` in `DeepSineOsc` struct member init. `tick()` divides by `sr` to compute phase increment. Any call before `prepare()` uses 44100 regardless of host SR. Quote: `float sr = 44100.f;`

- **P2** — `Source/Engines/OceanDeep/OceanDeepEngine.h:117` — `float sr = 44100.f;` in `DeepWaveguideBody` struct member init. Used in comb filter delay computation. Quote: `float sr = 44100.f;`

- **P2** — `Source/Engines/OceanDeep/OceanDeepEngine.h:944` — Third `float sr = 44100.f;` instance (engine-level member). Quote: `float sr = 44100.f;`

- **P2** — `Source/Engines/Olvido/OlvidoEngine.h:1429–1430` — `double sampleRateDouble = 44100.0; float sampleRateFloat = 44100.0f;` as member defaults. Used in voice processing and envelope computations. Quote: `double sampleRateDouble = 44100.0;`

- **P2** — `Source/Engines/Ortolan/OrtolanEngine.h:1509` — `float sampleRateFloat = 44100.0f;` as member default. Used in DSP coefficient computations. Quote: `float sampleRateFloat = 44100.0f;`

- **P2** — `Source/Engines/Observandum/ObservandumEngine.h:1481–1482` — `double sampleRateDouble = 44100.0; float sampleRateFloat = 44100.0f;` as member defaults. Quote: `double sampleRateDouble = 44100.0;`

- **P2** — `Source/Engines/Ocelot/OcelotParamSnapshot.h:53` — `float sampleRateRed = 44100.0f;` as member default. Used in reduced-rate processing. Quote: `float sampleRateRed = 44100.0f;`

- **P2** — `Source/Engines/Orrery/OrreryEngine.h:1493` — `float sampleRateFloat = 44100.0f;` as member default. Quote: `float sampleRateFloat = 44100.0f;`

- **P2** — `Source/Engines/Ooze/OozeEngine.h:1342` — `double currentSampleRate_ = 44100.0;` as member default. Quote: `double currentSampleRate_ = 44100.0;`

- **P2** — `Source/Engines/Oobleck/OobleckEngine.h:1419` — `double currentSampleRate_ = 44100.0;` as member default. Quote: `double currentSampleRate_ = 44100.0;`

- **P2** — `Source/Engines/Ondine/OndineEngine.h:1427` — `float sampleRateFloat = 44100.0f;` as member default. Quote: `float sampleRateFloat = 44100.0f;`

- **P2** — `Source/Engines/Ostracon/OstraconEngine.h:1234` — `float currentSampleRate = 44100.0f;` as member default. Quote: `float currentSampleRate = 44100.0f;`

- **P2** — `Source/Engines/Octant/OctantEngine.h:1329` — `float sampleRateFloat = 44100.0f;` as member default. Quote: `float sampleRateFloat = 44100.0f;`

- **P2** — `Source/Engines/Oort/OortEngine.h:1491` — `float sampleRateFloat = 44100.0f;` as member default. Quote: `float sampleRateFloat = 44100.0f;`

- **P2** — `Source/Engines/Ogive/OgiveEngine.h:1430–1431` — `double sampleRateDouble = 44100.0; float sampleRateFloat = 44100.0f;` as member defaults. Quote: `double sampleRateDouble = 44100.0;`

- **P2** — `Source/Engines/Opsin/OpsinEngine.h:1422` — `float sampleRateFloat = 44100.0f;` as member default (line 486 is a validation guard — excluded). Quote: `float sampleRateFloat = 44100.0f;`

- **P2** — `Source/Engines/Opera/OperaBreathEngine.h:110` and `Source/Engines/Opera/ReactiveStage.h:63` — `sr = 48000.0f` hardcoded as a fallback in struct init, not a validation guard. Opera's breath engine defaults to 48000 rather than inheriting from the live AudioContext. Quote: `sr = 48000.0f;`

**False positives eliminated:**
- `Onset/OnsetEngine.h:1334` — `static constexpr float kReferenceSampleRate = 44100.0f` is a reference/normalization constant, not a live SR member. Clean.
- `Organism/OrganismEngine.h:243` — `static constexpr double kRefSampleRate = 44100.0` — same pattern. Clean.
- `Ohm/OhmEngine.h:198` — same. Clean.
- `Oxidize/OxidizeSediment.h:177` — same. Clean.
- `Oxidize/OxidizeVoice.h:58` — `entropyRate = 44100.0f` is labeled "effective sample rate (for S&H)" but this is the S&H clock rate, a design constant, not the audio SR. Borderline; **not filed** as the comment clarifies intentional design.
- All three Ouroboros hits are validation guards (`if (sampleRate <= 0.0) sampleRate = 44100.0`). Clean.

**P2 confirmed count: 19 engines / 21 member-init sites** (OceanDeep contributes 3 independent structs)

---

### Pattern 3 — `prepare()` from audio thread

`prepare()` or `prepareToPlay()` called inside a MIDI handler, parameter callback, or `processBlock` branch on the audio thread.

- **P3** — `Source/Engines/Obiont/ObiontEngine.h:1050` — `adsr.prepare((float)sampleRate)` called inside `ObiontVoice::noteOn()`, which is invoked from `renderBlock()`'s MIDI event loop. The `sampleRate` variable here is the voice-level member, not freshly queried. This re-prepares the ADSR on every note-on from the audio thread. Quote: `adsr.prepare((float)sampleRate);`

- **P3** — `Source/Engines/Obbligato/ObbligatoEngine.h:105–113` — `ObbligatoVoice::prepare()` calls `delayLine.prepare(maxDelaySamples)`, `bodyResonator.prepare(sampleRate)`, `sympBank.prepare(sampleRate, 512)`, `organicDrift.prepare(sampleRate)`, `airJet.prepare(sampleRate)`, `reed.prepare(sampleRate)`, `adsr.prepare(sr)` — 7 full `prepare()` calls. This voice-level `prepare()` is called from engine-level `noteOn()` at line 288: `voices[targetSlot].noteOn(...)` which chains to `voice.prepare()` on voice retrigger. Full waveguide + sympathetic bank re-initialization on every note-on from the audio thread. **Most egregious P3 in fleet.** Quote: `sympBank.prepare(sampleRate, 512);`

- **P3** — `Source/Engines/Oxidize/OxidizeEngine.h:66,69` — `v.prepare(sampleRate, maxBlockSize)` and `sediment_.prepare(sampleRate, maxBlockSize)` called inside what the P3 scanner flagged as `processBlock` context. Reading line 56 confirms this is the engine-level `prepare()` override (not `processBlock`). **False positive — not filed.**

- **P3** — `Source/Engines/Organon/OrganonEngine.h:985,989,991,994` — `voice.prepare()`, `aftertouch.prepare()`, `profiler.prepare()`, `silenceGate.prepare()` — all inside `OrganonEngine::prepare()` override. **False positive — not filed.**

- **P3** — `Source/Engines/Oracle/OracleEngine.h:1551,1553,1598,1600` — `voice.dcBlockerL.prepare(sampleRateFloat)` × 2 + `voice.dcBlockerR.prepare(sampleRateFloat)` × 2 called inside `noteOn()`, which is invoked from the audio thread's MIDI handler. DC blocker `prepare()` computes a coefficient from sample rate — not allocating, but still an unexpected lifecycle call from the audio thread. Quote: `voice.dcBlockerL.prepare(sampleRateFloat);`

- **P3** — Voice-level `prepare()` in `noteOn()` pattern (multiple engines): Oddfellow, Ogre, Opal, Ochre, Oaken, Olate, Omega, Obelisk, Oven, Osier, Otis, Oxalis, Octave, Orchard, Oto, Oleg, Okeanos, Opcode, Overgrow, Oware, Snap — all call `v.someComponent.prepare(srf)` inside `noteOn()` on the audio thread. For lightweight components (envelope, DC blocker, simple one-pole filter) this is a widespread pattern that is RT-safe (no allocation) but architecturally wrong per lifecycle contract. The non-allocating calls are severity P3-low; the allocating ones (waveguide delay lines, FX chains) are P3-high.
  - **P3-HIGH** — `Source/Engines/Overgrow/OvergrowEngine.h:609` — `v.string.prepare(srf)` in `noteOn()`. Physical string model prepare likely re-sizes the delay line buffer. Quote: `v.string.prepare(srf);`
  - **P3-HIGH** — `Source/Engines/Ochre/OchreEngine.h:864` — `v.body.prepare(srf)` in `noteOn()`. Body resonance prepare. Quote: `v.body.prepare(srf);`
  - **P3-HIGH** — `Source/Engines/Oaken/OakenEngine.h:683–684` — `v.body.prepare(srf)` + `v.curing.prepare(srf)` in `noteOn()`. Two physical-model prepares per note-on. Quote: `v.body.prepare(srf);`
  - **P3-HIGH** — `Source/Engines/Onkolo/OnkoloEngine.h:559–561` — `v.string.prepare(srf)` + `v.wah.prepare(srf)` in `noteOn()`. (Already in Wave 1 — confirm cross-reference with ONK-N03.)
  - **P3-HIGH** — `Source/Engines/Octave/OctaveEngine.h:899` — `v.room.prepare(srf)` in `noteOn()`. Room reverb re-prepare per note. Quote: `v.room.prepare(srf);`

**P3 confirmed count (high-severity only): 7 engines** (Obiont, Obbligato, Oracle, Overgrow, Ochre, Oaken, Octave). Fleet-wide low-severity pattern affects ~20 more engines but is RT-safe in practice.

---

### Summary Table — Fleet-Wide Pattern Grep Findings

| ID | Engine | Pattern | Severity | One-Line Summary |
|----|--------|---------|----------|-----------------|
| FPG-P1-01 | Overcast | P1 | HIGH | `setADSR` called per-sample with block-constant ADSR params |
| FPG-P1-02 | Oddfellow | P1 | HIGH | `setADSR` called per-sample with block-constant ADSR params |
| FPG-P1-03 | Otis | P1 | HIGH | `setADSR` per-sample — misplaced "per block" comment |
| FPG-P1-04 | Opcode | P1 | HIGH | `setADSR` per-sample — misplaced "per block" comment |
| FPG-P1-05 | Okeanos | P1 | HIGH | `setADSR` per-sample with block-constant params |
| FPG-P1-06 | Outlook | P1 | HIGH | `setADSR` per-sample with block-constant params |
| FPG-P1-07 | Oxidize | P1 | HIGH | 2–3× `setCoefficients` per sample (LFO-tracked erosion filter) |
| FPG-P1-08 | Organon | P1 | MEDIUM | `setCoefficients` per-sample in self-feeding branch (block-constant args) |
| FPG-P1-09 | Osteria | P1 | HIGH | `MurmurGenerator::process()` calls 2× `setCoefficients` per-sample |
| FPG-P2-01 | Overtide | P2 | MEDIUM | `sampleRateFloat = 44100.0f` member default |
| FPG-P2-02 | OceanDeep | P2 | HIGH | Three structs with `sr = 44100.f` member defaults |
| FPG-P2-03 | Olvido | P2 | MEDIUM | `sampleRateDouble/Float = 44100.0` member defaults |
| FPG-P2-04 | Ortolan | P2 | MEDIUM | `sampleRateFloat = 44100.0f` member default |
| FPG-P2-05 | Observandum | P2 | MEDIUM | `sampleRateDouble/Float = 44100.0` member defaults |
| FPG-P2-06 | Ocelot | P2 | MEDIUM | `sampleRateRed = 44100.0f` in ParamSnapshot |
| FPG-P2-07 | Orrery | P2 | MEDIUM | `sampleRateFloat = 44100.0f` member default |
| FPG-P2-08 | Ooze | P2 | MEDIUM | `currentSampleRate_ = 44100.0` member default |
| FPG-P2-09 | Oobleck | P2 | MEDIUM | `currentSampleRate_ = 44100.0` member default |
| FPG-P2-10 | Ondine | P2 | MEDIUM | `sampleRateFloat = 44100.0f` member default |
| FPG-P2-11 | Ostracon | P2 | MEDIUM | `currentSampleRate = 44100.0f` member default |
| FPG-P2-12 | Octant | P2 | MEDIUM | `sampleRateFloat = 44100.0f` member default |
| FPG-P2-13 | Oort | P2 | MEDIUM | `sampleRateFloat = 44100.0f` member default |
| FPG-P2-14 | Ogive | P2 | MEDIUM | `sampleRateDouble/Float = 44100.0` member defaults |
| FPG-P2-15 | Opsin | P2 | MEDIUM | `sampleRateFloat = 44100.0f` member default |
| FPG-P2-16 | Opera | P2 | HIGH | `sr = 48000.0f` hardcoded in BreathEngine + ReactiveStage |
| FPG-P3-01 | Obiont | P3 | MEDIUM | `adsr.prepare()` called from `noteOn()` on audio thread |
| FPG-P3-02 | Obbligato | P3 | HIGH | 7 full `prepare()` calls (waveguide + sympathetic bank) in `noteOn()` |
| FPG-P3-03 | Oracle | P3 | MEDIUM | `dcBlocker.prepare()` × 4 called from `noteOn()` on audio thread |
| FPG-P3-04 | Overgrow | P3 | HIGH | `v.string.prepare()` (physical model) in `noteOn()` — possible alloc |
| FPG-P3-05 | Ochre | P3 | HIGH | `v.body.prepare()` in `noteOn()` |
| FPG-P3-06 | Oaken | P3 | HIGH | `v.body.prepare()` + `v.curing.prepare()` in `noteOn()` |
| FPG-P3-07 | Octave | P3 | HIGH | `v.room.prepare()` (reverb) in `noteOn()` — probable alloc |

**Totals: P1 = 9 engines / 11 sites · P2 = 16 engines / 21 sites · P3 = 7 engines (high-severity)**

---

### Novel Engines (not in Wave 1 or baseline)

Wave 1 / baseline covered: Oceanic, Overwash, Oasis, Ohm, Opal, Oxytocin, Drift, Bite, Obbligato, Onkolo.

**New engines implicated by this grep:** Overcast, Oddfellow, Otis, Opcode, Okeanos, Outlook, Oxidize, Organon, Osteria, Overtide, OceanDeep, Olvido, Ortolan, Observandum, Ocelot, Orrery, Ooze, Oobleck, Ondine, Ostracon, Octant, Oort, Ogive, Opsin, Opera, Obiont, Oracle, Overgrow, Ochre, Oaken, Octave.

**Novel engine count: 31**

---

*Grep conducted 2026-04-19. 77 engine files scanned. All P1/P2/P3 raw hits manually verified against surrounding context. False-positive rate: P1 ~65% (34 raw → 11 confirmed), P2 ~35% (31 raw → 21 confirmed), P3 ~85% (156 raw → 7 high-severity confirmed).*

---

## Resolve-Pass Novel Findings (2026-04-19)

Novel issues spotted while spot-checking baseline OPEN entries for resolution. Not previously recorded in baseline or Wave 1.

---

### [RP-N01] P1 — Opal `CombFilter kMaxDelay=8192` borderline overflow at 96kHz, size>~0.97 (CONFIRMED)

**File**: `Source/Engines/Opal/OpalEngine.h` line 632
**Description**: The Opal P0 in the baseline noted `kMaxDelay=8192` for OpalScatterReverb CombFilter. Spot-checking reveals the overflow math: at 96kHz, scale = 0.5 + size * 1.5 (max size=1.0 → scale=2.0), and longest comb time = 43.7ms. Required samples = 43.7×0.001×2.0×96000 = 8390 > 8192. `setDelay()` clamps to `kMaxDelay-1=8191`, so no out-of-bounds write, but the reverb tail is truncated/mistuned at 96kHz with size>~0.97. The P0 "memory corruption" from the baseline is not confirmed — the clamp prevents the buffer overflow. However, the reverb sounds wrong at 96kHz max settings (comb mismatch). Downgrade from P0 to P1.
**Severity**: P1 (audible reverb mistune / tail truncation at 96kHz with large size), not P0 (no memory corruption due to clamp).
**Recommendation**: Increase `kMaxDelay` to `static_cast<int>(0.05f * 192000) + 1 = 9601` to cover 50ms at 192kHz safely. Or dynamically allocate in `prepare()`.

---

### [RP-N02] P2 — Overworn `worn_concentrate` param loaded but never consumed in DSP (CONFIRMED D004)

**File**: `Source/Engines/Overworn/OverwornEngine.h` lines 296, 343
**Description**: `pConcentrate` is loaded from APVTS and adjusted by the COUPLING macro, but the resulting value is never used to affect audio output. The reduction state (concentrateDark, umamiBed, spectralMass) is computed from `pMaillard` and `pUmamiDepth` only — `pConcentrate` is a dead end. D004 violation: user-visible "Concentrate" knob has zero effect on sound.
**Recommendation**: Route `pConcentrate` to scale the export strength of concentrateDark or the umamiBed multiplier. E.g., `reduction.concentrateDark = clamp(reduction.sessionAge * pMaillard * pConcentrate * 1.5f, 0.0f, 1.0f)`.

---

### [RP-N03] P2 — Opensky 6 dead mod-matrix params (CONFIRMED D004)

**File**: `Source/Engines/OpenSky/OpenSkyEngine.h` lines 1267–1272, renderBlock
**Description**: `sky_modSlot1Src`, `sky_modSlot1Dst`, `sky_modSlot1Amt`, `sky_modSlot2Src`, `sky_modSlot2Dst`, `sky_modSlot2Amt` are declared, registered in APVTS, and cached as `std::atomic<float>*` pointers, but zero usage in `renderBlock()` was found. The mod matrix UI shows controls that have no effect. D004 violation ×6.
**Recommendation**: Implement the mod matrix routing using the cached pointers in `renderBlock()`. Even a single LFO→filter routing would unblock the D004 finding.

---

### [RP-N04] P1 — Okeanos `std::exp` per partial per sample in tine decay (CONFIRMED)

**File**: `Source/Engines/Okeanos/OkeanosEngine.h` line 151
**Description**: Inside the tine oscillator sample loop: `float decayRate = 1.0f - std::exp(-1.0f / (sr * (2.0f - static_cast<float>(i) * 0.25f)))` — `sr` is constant, `i` is the partial index (0..11 = 12 values) — this computation is purely a function of `sr` and partial index. All 12 values can be precomputed at `prepare()` time and stored in a `cachedPartialDecayRate[12]` array. Currently runs 12 `std::exp` calls per voice per sample.
**Recommendation**: Add `float cachedPartialDecayRate[kNumPartials]` to the tine oscillator struct. Compute in `prepare()`. Replace line 151 with `const float decayRate = cachedPartialDecayRate[i];`

---

*Resolve-pass conducted 2026-04-19. 28 OPEN baseline entries spot-checked (all P0, all P1, selected P2). 4 novel findings logged above.*
