# Fleet DSP Wave 2 Findings — 2026-04-19
## Reconcile-Mode Scan: Dead-Param/Click Cluster (9 engines)

Baseline: reconciliation + Wave 1 + recent commits 869143256, 8eb2d4685, e3ba32248, de54fc66b.
Novel findings only.

---

## Scan Status

| Engine | Lines Read | Coverage | Novel Findings |
|--------|-----------|----------|---------------|
| Omega | 750 / 750 | Full | 2 (OMG-W2-01, OMG-W2-02) |
| Offering | 995 / 995 | Full | 1 (OFR-W2-01) |
| Owlfish | 275 / 275 | Full | 1 (OWL-W2-01) |
| Overworn | 851 / 851 | Full | 2 (WRN-W2-01, WRN-W2-02) |
| Orca | 1380 / 1380 | Full | 1 (ORC-W2-01) |
| OpenSky | 600 / ~1400+ | **Partial** | 0 (render loop not reached; baseline RP-N03 open) |
| Ottoni | 626 / 626 | Full | 1 (OTT-W2-01) |
| Outlook | 841 / 841 | Full | 2 (OUT-W2-01, OUT-W2-02) |
| Ohm | 848 / 848 | Full | 1 (OHM-W2-01) |

**Total novel**: 0 P0 · 3 P1 · 6 P2 · 1 P3  
**Note**: OpenSky render loop, parameter attachment, and mod-matrix routing were not reached due to file size. Baseline RP-N03 (6 dead mod-matrix params) remains open.

---

## Novel Findings

### Omega

#### OMG-W2-01 — P1 — LFOToPitch/AmpToPitch coupling silently dead (zeroed before sample loop)

`couplingPitchMod` is set to `0.0f` at line 370, before the sample loop begins at line 419. Inside the loop, `PitchBendUtil::semitonesToFreqRatio(bendSemitones + couplingPitchMod)` at line 440 uses the zeroed value — the coupling offset never arrives. Any engine routing LFOToPitch or AmpToPitch into Omega produces no pitch movement.

```cpp
// Line 370-371: zeroed BEFORE sample loop
couplingFilterMod = 0.0f;
couplingPitchMod  = 0.0f;
// ...
for (int s = 0; s < numSamples; ++s)  // line 419
{
    // ...
    freq *= PitchBendUtil::semitonesToFreqRatio(bendSemitones + couplingPitchMod); // line 440 — always 0
```

**Fix**: Move the zeroing assignments inside the sample loop, after the coupling accumulation reads.

---

#### OMG-W2-02 — P2 — `inverseSr_` hardcoded to 48000.0 Hz at declaration

`inverseSr_` is initialized at line 704 as `1.0f / 48000.0f`. Before `prepare()` is called — e.g., if any code path uses `inverseSr_` during voice allocation or early modulation — the wrong sample rate is assumed. The prior fleet grep (Wave 1 FPG-P2) flagged the 44100.0f variant; this is the 48000.0f variant, novel for Omega.

```cpp
float inverseSr_ = 1.0f / 48000.0f;  // line 704 — wrong before prepare()
```

**Fix**: Initialize to `0.0f` and guard usage: `if (inverseSr_ == 0.0f) return;` or `assert(sr > 0)` in `prepare()`.

---

### Offering

#### OFR-W2-01 — P1 — `buffer.clear()` destroys accumulated multi-engine output

`buffer.clear(0, numSamples)` at line 355 is called unconditionally in `renderBlock()` before Offering writes its voices. In a multi-engine rack (e.g., Submarine signal chain), every engine before Offering in the render order has its output silenced. This is the same pattern as OXY-N01 (Oxytocin, Wave 1) — novel for Offering.

```cpp
buffer.clear(0, numSamples);  // line 355 — destroys all prior engine output
```

**Fix**: Remove `buffer.clear()`. Voices should accumulate with `+=` into the buffer, not overwrite it. If a silence path is needed, guard with `if (isActive())`.

---

### Owlfish

#### OWL-W2-01 — P2 — AudioToRing `couplingFMMod` has no Nyquist guard at high pitch

In the AudioToRing coupling path, `freqRatio *= (1.0f + couplingFMMod)` at line 152 applies the coupling multiplier directly. `couplingFMMod = amount * rms * 2.0f` (line 207) yields a maximum of ~2.0f at full coupling. Combined with the base `freqRatio`, a voice at MIDI note 100 (~2637 Hz) with `pitchBendRatio = 1.0f` receives `freqRatio *= 3.0f`, pushing the computed frequency to ~7911 Hz — beyond Nyquist at 16kHz, and severe aliasing at 44.1kHz. No clamp or fold-back guard exists.

```cpp
freqRatio *= (1.0f + couplingFMMod);  // line 152 — no Nyquist guard
// couplingFMMod = amount * rms * 2.0f  // line 207 — max ~2.0
```

**Fix**: Clamp `freqRatio` after coupling: `freqRatio = std::min(freqRatio, 0.45f * sampleRate / baseFreq)` or equivalent Nyquist fraction guard.

---

### Overworn

#### WRN-W2-01 — P1 — `setADSR()` called per-sample for both amp and filter envelopes

`voice.ampEnv.setADSR(pAmpA, pAmpD, pAmpS, pAmpR)` and `voice.filterEnv.setADSR(pFiltA, pFiltD, pFiltS, pFiltR)` are called at lines 457–458 inside `for (int i = 0; i < numSamples; ++i)` (line 384). All four ADSR parameters are block-constant (read once from params before the loop). This recomputes exponential envelope coefficients 44100+ times/second for values that never change within the block. Overworn was not in the Wave 1 `setADSR` grep list (FPG-P1-01 through FPG-P1-06) — novel for this engine.

```cpp
for (int i = 0; i < numSamples; ++i)  // line 384
{
    // ...
    voice.ampEnv.setADSR(pAmpA, pAmpD, pAmpS, pAmpR);       // line 457 — per-sample!
    voice.filterEnv.setADSR(pFiltA, pFiltD, pFiltS, pFiltR); // line 458 — per-sample!
```

**Fix**: Hoist both `setADSR()` calls to the pre-loop block, after param snapshot but before `for (int i = 0; ...)`.

---

#### WRN-W2-02 — P2 — 8-band spectral state update runs per audio sample

The BROTH state update (session age advancement + all 8 `WornBand` updates) runs inside `for (int i = 0; i < numSamples; ++i)` at lines 404–415. Session aging and spectral band coefficients are driven by block-constant parameters (`pSessionAge`, `pMaillard`, `pConcentrate`, `pSpectralMass`). Updating 8 IIR band states 44100 times/second when the parameters don't change within a block wastes ~8 filter coefficient recomputations per sample per voice.

```cpp
for (int i = 0; i < numSamples; ++i)  // line 384
{
    // ...
    reduction.sessionAge = ...;           // line 404 — per-sample
    for (int b = 0; b < 8; ++b)          // line 406
        reduction.bands[b].update(...);   // line 415 — 8× per-sample
```

**Fix**: Move session state update and band coefficient calculations to the pre-loop block. Only the per-sample audio accumulation through those coefficients needs to remain inside the loop.

**Note on baseline RP-N02** (`worn_concentrate` claimed dead): `pConcentrate` IS used at line 419 (`reduction.concentrateDark = clamp(reduction.sessionAge * pMaillard * pConcentrate * 1.5f, 0.0f, 1.0f)`). RP-N02 appears to be a **false positive** in the reconciliation doc. Recommend marking RP-N02 as INVALID/FP.

---

### Orca

#### ORC-W2-01 — P2 — Block-rate `mainFilter.setCoefficients()` immediately overridden per-sample

`voice.mainFilter.setCoefficients(velCutoff, effectiveReso, srf)` is called at line 502 in the pre-loop block. Then inside the sample loop at line 673, `voice.mainFilter.setCoefficients(lfo2Cutoff, effectiveReso, srf)` overrides it per-sample to apply LFO2 modulation. The block-rate call is dead work — its result is overwritten on sample 0 of the loop. The per-sample call is valid (LFO2 audio-rate sweep is intentional), but the preceding block-rate call serves no purpose.

```cpp
// Pre-loop block (line 502):
voice.mainFilter.setCoefficients(velCutoff, effectiveReso, srf);  // overridden on sample 0

// Per-sample loop (line 673):
voice.mainFilter.setCoefficients(lfo2Cutoff, effectiveReso, srf); // this wins every sample
```

**Fix**: Remove the block-rate call at line 502. The per-sample call at line 673 is the correct and sufficient path.

---

### OpenSky

**Coverage: Partial (lines 1–600 of ~1400+). Render loop and mod-matrix routing not reached.**

No novel findings confirmed. Baseline RP-N03 (6 dead mod-matrix params: `sky_modSlot1-2 Src/Dst/Amt`) remains OPEN — these appear to be a V2 feature stub rather than a wiring oversight (slots are declared and cached but routing implementation is entirely absent from all read sections).

---

### Ottoni

#### OTT-W2-01 — P1 — `std::exp()` per-sample in release path

`std::exp(-1.0f / (v.sr * 0.3f))` is computed at line 313 inside `for (int i = 0; i < ns; ++i)`. Both `v.sr` and `0.3f` are constants for the lifetime of a voice — this coefficient is identical on every sample of the release tail. `std::exp` is a transcendental function; per-sample cost at 96kHz with 8 voices = 768 `std::exp` calls/block on release voices alone.

```cpp
if (v.releasing)
{
    float releaseCoeff = std::exp(-1.0f / (v.sr * 0.3f));  // line 313 — per-sample!
    v.ampEnv *= releaseCoeff;
}
```

**Fix**: Precompute `releaseCoeff_` once in `prepare()` (or `noteOff()`) and store as a voice member. Replace the per-sample computation with the cached value.

---

### Outlook

#### OUT-W2-01 — P2 — `crossfadeRate` recomputed per-sample during voice-steal

`const float crossfadeRate = 1.0f / (0.005f * static_cast<float>(sr))` is computed at line 252 inside the `if (v.isBeingStolen)` branch of the render loop. `sr` is block-constant. During a 5ms voice-steal crossfade this is ~220 divisions per block, for a result that never changes.

```cpp
if (v.isBeingStolen)
{
    const float crossfadeRate = 1.0f / (0.005f * static_cast<float>(sr));  // line 252 — per-sample
    v.stealFadeGain -= crossfadeRate;
```

**Fix**: Precompute `stealCrossfadeRate_` in `prepare()` and store as a member. Replace line 252 with the cached value.

---

#### OUT-W2-02 — P2 — `superOsc[0]` aliased between Triangle-morph (case 1) and Super stack (case 5)

`superOsc[0]` is borrowed as a scratch Saw oscillator for the Triangle→Saw morph (case 1, lines 700–706). In case 5 (Super unison stack), `superOsc[0]` is used as the first of three detuned saws (lines 745–751). If a note plays in case 1 and the user switches waveform to case 5 mid-note (or if polyphony causes two voices to play in different cases), `superOsc[0]` carries stale phase/waveform state from case 1 into case 5, producing an audible click or phase artifact on the transition.

```cpp
case 1: // Triangle→Saw morph
    v.superOsc[0].setWaveform(PolyBLEP::Waveform::Saw);   // borrows superOsc[0]
    const float saw = v.superOsc[0].processSample();        // line 703-704

// ...
case 5: // Super — 3 detuned saws
    for (int d = 0; d < 3; ++d)
        sum += v.superOsc[d].processSample();               // superOsc[0] reused here
```

**Fix**: Allocate a dedicated scratch oscillator for case 1 (e.g., `morphOsc`), separate from the `superOsc[]` array used by case 5. Alternatively, reset `superOsc[0]` phase on waveform case transition.

---

### Ohm

#### OHM-W2-01 — P3 — Release coefficient uses Euler approximation instead of matched-Z

The release envelope coefficient at line 570 is computed as `1.0f - 1.0f / (v.sr * kRelTime)` — the Euler (`w/(w+1)`) approximation. CLAUDE.md explicitly requires `exp(-1/(sr*T))` (matched-Z). The error is small at 44.1kHz (`~0.003%`) but increases at lower sample rates. Per doctrine violation rather than audible impact at standard rates.

```cpp
static constexpr float kRelTime = 0.3f;
float relCoeff = 1.0f - 1.0f / (v.sr * kRelTime);  // line 570 — Euler, not exp(-1/(sr*T))
```

**Fix**: Replace with `std::exp(-1.0f / (v.sr * kRelTime))`.

---

## Cross-Engine Patterns (Novel — Wave 2)

### Pattern F — `buffer.clear()` before voice accumulation (multi-engine mix destruction)
- **Instances**: Offering (OFR-W2-01) + Oxytocin (OXY-N01 from Wave 1)
- **Risk**: Silent mix destruction in any multi-engine rack arrangement
- **Action**: Fleet-wide grep for `buffer.clear` inside `renderBlock()` or equivalent render method

### Pattern G — `std::exp()` inside per-sample release path
- **Instances**: Ottoni (OTT-W2-01)
- **Pattern**: Transcendental function with block/voice-constant inputs inside `for (int i = 0; ...)` release branch
- **Action**: Grep for `std::exp` inside loops with `releasing` / `noteOff` branch guards across full fleet

---

## Dead Param → Missing Feature Signal

| Engine | Params | Assessment |
|--------|--------|-----------|
| OpenSky | `sky_modSlot1Src`, `sky_modSlot1Dst`, `sky_modSlot1Amt`, `sky_modSlot2Src`, `sky_modSlot2Dst`, `sky_modSlot2Amt` | V2 mod-matrix feature stub. Slots are declared, cached, and exposed in UI but routing implementation is entirely absent. Not a wiring oversight — this is a deliberately deferred modulation routing system. |

---

## Reconciliation Doc Correction

**RP-N02 (`worn_concentrate`) should be marked INVALID/FP**: `pConcentrate` is actively used at Overworn line 419. The prior "dead" classification was incorrect.

---

*Wave 2 complete. 8 of 9 engines fully covered. OpenSky partial (render loop not reached).*
*Novel: 0 P0 · 3 P1 · 6 P2 · 1 P3*
