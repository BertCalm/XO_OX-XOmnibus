# XOmnibus DSP Sweep Report — 2026-03-20

**Scope:** 5 targeted areas — Aquatic FX, Mathematical FX, Boutique FX, OBRIX DSP, SDK
**Auditor:** Claude Code (Sonnet 4.6)
**Overall fleet health:** Good structure, denormal protection consistent, several medium-severity findings, two High findings, zero Criticals.

---

## Area 1 — Aquatic FX (`Source/DSP/Effects/AquaticFXSuite.h`)

**Status: WARN**

### Findings

**[High] Reef FDN read-position formula uses a bare multiply that can overflow the delay buffer**
File: `AquaticFXSuite.h` ~line 686
```cpp
int rp = (delays[ch].writePos - static_cast<int> (kPrimeLengths48k[ch] * srScale * size * 2.7f + 1)
           + len * 8) % len;
rp = std::max (0, rp % len);
```
When `size` is near 1.0 and `srScale` is ~2.0 (96 kHz), the subtracted value can exceed `len * 8`, making the modulo result negative before the guard on `rp`. The `std::max(0, rp % len)` guard only catches negative final values after the outer modulo, but `len * 8` may not be enough padding at 192 kHz sample rates. The guard also applies `% len` twice — `(... + len * 8) % len` is already zero-referenced, so the second `rp % len` is redundant. More critically, `len` itself is hard-capped at 5000 samples but at 192 kHz `kPrimeLengths48k[7] * srScale = 4799 * 4 = 19196`, which would be truncated to 5000. At maximum size=1.0 the computed offset (19196 * 1.0 * 2.7 ≈ 51829) would outright exceed `len * 8 = 40000`, yielding a negative `rp` after modulo and breaking buffer reads.
**Fix:** Replace the magic `len * 8` padding with a proper clamp: `rp = ((delays[ch].writePos - offset % len) + len) % len;` where `offset` is pre-clamped to `[1, len - 1]`.

**[Medium] Tide SVF updated once per block in AutoFilter mode but per-sample in tremolo+autofilter combined mode**
File: `AquaticFXSuite.h` ~lines 442–448 vs 490–491
In `Mode::AutoFilter`, the SVF coefficients are computed once per block (correct). In `Mode::Both`, `svfL/R.updateCoeffs()` is called every sample because the cutoff modulated by the LFO changes every sample. This is correct for accuracy but the SVF coefficient computation inside `updateCoeffs` calls `fastTan()` every sample. At 96 kHz with large block sizes this is noticeable CPU. Consider subsampling the coefficient update every 4 samples with interpolation since the LFO rate is much slower than audio rate.
**Fix (Medium):** Subsample `updateCoeffs` every 4 samples in Both mode.

**[Medium] `stereoPhaseRad` parameter accepted but silently ignored**
File: `AquaticFXSuite.h` line 503
```cpp
(void)stereoPhaseRad;
```
The parameter `stereoPhaseRad` is passed by callers but the stereo phase offset for the Tide LFO's R channel is never initialized from it. `lfoPhaseR` starts at 0 and is never set to `lfoPhase + stereoPhaseRad` on the first block. The comment says "re-sync the initial offset on prepare only" but `prepare()` also does not read `stereoPhaseRad`. The parameter is exposed to users but has zero effect.
**Fix:** On first block (or in `prepare()`), initialize `lfoPhaseR = lfoPhase + stereoPhaseRad` and update it each block from the current param value.

**[Medium] Drift stage has a stale default `sr = 44100.0` in the struct member**
File: `AquaticFXSuite.h` ~line 234
```cpp
double sr = 44100.0;
```
The `prepare(double sampleRate)` sets `sr = sampleRate` correctly, but if `processBlock()` is called before `prepare()` (e.g., a DAW plugin being loaded in an unusual order), the 44100 default leads to incorrect OU variance scaling and delay size mismatches. Other structs (TideStage, ReefStage, SurfaceStage, BiolumeStage) all share this pattern.
**Fix (Low):** This is consistent across all stages. A defensive assert or JUCE_ASSERT in processBlock that `sr > 0.0 && sr != 44100.0` at debug time would catch this. Alternatively, initialize to 0 and guard with `if (sr <= 0.0) return;`.

**[Low] Drift 1× HP filter on feedback uses a 1-pole LP to derive HP but the LP state is stored as `hpStateL/R`**
File: `AquaticFXSuite.h` ~lines 349–351
The variable name `hpStateL` stores the LP state of the feedback highpass filter (HP = input - LP). The naming is inverted. No logic bug, but misleading for maintenance.
**Fix (Low):** Rename to `hpLpStateL`, `hpLpStateR`.

**DSP correctness (PASS):**
- Cytomic TPT SVF implementation is correct per Andy Simper's 2012 formulation. Denormal protection applied to all integrator states via `flushDenormal()`.
- Householder FDN mixing matrix is correct: `out[i] = in[i] - (2/N) * sum(in)`.
- Allpass diffusers use the correct Schroeder form.
- Biolume shimmer pitch-shifter saturates at 0.7 (`shimmerBlend` clamped) preventing runaway.
- All stages check `mix < threshold` and early-return, preventing CPU cost when bypassed.

**Sample rate independence (PARTIAL PASS):**
- All buffer sizes computed relative to `sampleRate`.
- FDN delay hard-cap at 5000 samples breaks at 96 kHz+ (see High finding above).
- Drift `sqrtDt` computed as `sqrt(1 / srF)` — correct.

**Parameter smoothing (PASS):**
- Surface stage smooths both frequency and gain per-sample using a 20 ms time constant.
- All other stages read params directly from parent (no smoothing at FX stage level) — acceptable because the calling code (MasterFXChain) is expected to handle block-rate reads.

---

## Area 2 — Mathematical FX (`Source/DSP/Effects/MathFXChain.h` + sub-processors)

**Status: PASS with notes**

### Findings

**[High] `AttractorDrive` hardcodes `44100.0f` in the ODE integration step**
File: `Source/DSP/Effects/AttractorDrive.h` line 84
```cpp
float basedt = (0.0001f + speed * 0.002f) * (44100.0f / sr);
```
This intentionally normalizes to 44100 Hz to keep the attractor's orbit speed consistent across sample rates. However the normalization is in the wrong direction: at 96 kHz, `44100/96000 ≈ 0.46`, making the step size too small and the attractor orbit half as fast as at 44100, changing the sonic character. The correct normalization if "orbit speed in perceptual terms" is desired would be `(sr / 44100.0f)` (inverse). However, if the intent is purely numerical stability (ODE step size must be small enough for the given integration rate), this is an arguably intentional tradeoff. The comment says "sufficient at audio sub-rate" which is accurate — Euler integration at 1/4 subsampling with this step size is stable. This finding should be reviewed for intent.
**Fix (design decision):** If orbit speed should be sample-rate-independent, flip the ratio to `sr / 44100.0f`. If the intent is matching the Lorenz attractor's published behavior at 44100 Hz, document that the 96 kHz behavior is intentionally slower.

**[Medium] `EntropyCooler.computeEntropy()` called once per sample (O(entropyWindowSize) per sample)**
File: `Source/DSP/Effects/EntropyCooler.h` lines 110–111
```cpp
float entropy = computeEntropy();
smoothedEntropy += (entropy - smoothedEntropy) * entSmooth;
```
`computeEntropy()` iterates over `entropyWindowSize` samples (~5 ms at 44100 = 220 samples) on every single audio sample. At 44100 Hz this is 44100 × 220 = 9.7 million iterations per second when `ecMix > 0`. At 96 kHz it doubles. This is a significant hidden CPU cost.
**Fix:** Compute entropy once per block, or at a fixed sub-rate (every 32 or 64 samples) and use the cached value. The entropy smoothing coefficient is already ~15 Hz, so per-sample computation is unnecessary precision.

**[Medium] `EntropyCooler` demon attack/release coefficients recomputed per sample**
File: `Source/DSP/Effects/EntropyCooler.h` lines 129–130
```cpp
float demonAttack = 1.0f - std::exp (-1.0f / (0.002f * sr));  // 2ms
float demonRelease = 1.0f - std::exp (-1.0f / (0.05f * sr));  // 50ms
```
These are constant within a block (they don't depend on any per-sample variable) but are computed inside the per-sample loop. Should be hoisted before the loop.
**Fix:** Move both coefficient computations before the `for (int s = 0; s < numSamples; ++s)` loop.

**[Low] `VoronoiShatter` — `Seed::grainPhase` field declared but never read or written during processing**
File: `Source/DSP/Effects/VoronoiShatter.h` line 193
```cpp
float grainPhase = 0.0f;
```
The field exists in the struct definition and is initialized in `prepare()` (implicitly via `seeds{}`) but `processBlock()` never reads or writes it. It appears to be a placeholder for future per-seed grain envelope tracking.
**Fix (Low):** Either implement it or remove it to avoid confusion. If placeholder: add a `// reserved — future per-seed grain envelope` comment.

**[Low] `QuantumSmear` — tap distribution and weights are recomputed at the start of every block even when params haven't changed**
File: `Source/DSP/Effects/QuantumSmear.h` lines 96–119
All 8 tap weights and offsets are recomputed each `processBlock()` call. These are expensive (8 `std::exp()` calls). Since `observation` and `delayCenterMs` are smoothed by APVTS, they won't change rapidly enough to require per-block recomputation in most contexts.
**Fix (Low):** Cache the last values of `observation` and `delayCenterMs` and skip recomputation when they haven't changed by more than a small epsilon.

**DSP correctness (PASS):**
- `QuantumSmear` feedback clamped at 0.95 and soft-clipped via `fastTanh()` before writing to delay buffer. No runaway risk.
- `AttractorDrive` Lorenz state bounded to `[-50, 50]` / `[0, 60]` with hard clamp plus a DC blocker and hard limiter. Numerically stable.
- `VoronoiShatter` crossfade logic handles the `minDist1 + minDist2 = 0` edge case with `std::max(0.001f, ...)`.
- All four processors: bypass at mix < threshold correctly.

**Numerical stability (PASS):** No division by zero found. All denominators guarded.

---

## Area 3 — Boutique FX (`Source/DSP/Effects/BoutiqueFXChain.h` + sub-processors)

**Status: PASS with notes**

### Findings

**[Medium] `AnomalyEngine` time-slip `slipFadeGain` increments/decrements are not sample-rate-independent**
File: `Source/DSP/Effects/AnomalyEngine.h` lines 192, 209
```cpp
slipFadeGain = std::min (1.0f, slipFadeGain + 0.001f);   // ~1000 samples fade-in
slipFadeGain -= 0.002f;                                   // ~500 samples fade-out
```
These are fixed per-sample increments regardless of sample rate. At 44100 Hz, fade-in takes ~1000 samples = 22.7 ms (reasonable). At 96000 Hz, fade-in takes ~1000 samples = 10.4 ms — still acceptable perceptually. At 192000 Hz it drops to 5.2 ms, which may cause audible clicks on fast fade-outs. This is a minor practical concern given the current 44.1/48/96 kHz usage.
**Fix (Low-Medium):** Normalize: `slipFadeInRate = 1.0f / (0.023f * sr)` and `slipFadeOutRate = 1.0f / (0.011f * sr)`, computed in `prepare()`.

**[Medium] `AnomalyEngine` stereo output uses an asymmetric formula that leaks time-slip signal inconsistently**
File: `Source/DSP/Effects/AnomalyEngine.h` line 224
```cpp
L[s] = dryL * (1.0f - mix) + finalOut * mix;
R[s] = dryR * (1.0f - mix) + finalOut * mix * 0.8f + slipOut * 0.2f;
```
The right channel gets `finalOut * mix * 0.8` plus a separate `slipOut * 0.2` additive term, while the left channel gets `finalOut * mix` (which already contains `slipOut` inside `finalOut`). This means the right channel's slip contribution is doubled: once inside `finalOut` (at 80% mix) and once additively (0.2). This is either intentional stereo widening of the slip or a bug. Given there's no comment explaining it, it looks unintentional.
**Fix:** Decide if the asymmetry is intentional. If not:
```cpp
L[s] = dryL * (1.0f - mix) + finalOut * mix;
R[s] = dryR * (1.0f - mix) + finalOut * mix;
```

**[Medium] `AnomalyEngine` Karplus-Strong is effectively mono — only `ksBufL` is used**
File: `Source/DSP/Effects/AnomalyEngine.h` lines 118–126
The KS buffer `ksBufL` is used for both the delay read and `ksOut`. `ksBufR` is allocated in `prepare()` and cleared in `reset()` but never read or written during `processBlock()`. The `ksOut` is a mono signal fed into both channels. This is a mild waste (one unused vector) and may be intentional (mono resonator is fine), but the allocated-but-unused `ksBufR` is dead memory.
**Fix (Low):** Remove `ksBufR` allocation if mono KS is the intent, or implement true stereo KS with slightly different tunings for mild stereo widening.

**[Low] `BoutiqueFXChain::addParameters` takes `std::vector<unique_ptr<RangedAudioParameter>>&` but `AquaticFXSuite::addParameters` takes `ParameterLayout&`**
File: `BoutiqueFXChain.h` line 74, `MathFXChain.h` line 63, vs `AquaticFXSuite.h` line 70
The three FX chains have inconsistent parameter registration APIs. AquaticFXSuite uses the newer JUCE `ParameterLayout&` API; MathFX and BoutiqueFX use the older `vector<unique_ptr<>>` pattern. Both work, but the inconsistency complicates the calling code in `XOmnibusProcessor`.
**Fix (Low):** Standardize all three to use `ParameterLayout&` for consistency.

**DSP correctness (PASS):**
- `SubmersionEngine`: feedback capped at 0.95, consistent with MathFX.
- `ArtifactCathedral`: bit-crush and data-rot operations are bounded.
- `DissolvingArchive`: grain mix parameters prevent clipping at unity settings.
- All stages: `mix < threshold` early-return bypass present.

**FX chain ordering (PASS):** Anomaly → Archive → Cathedral → Submersion is a valid signal flow (texture generator → memory/grains → reverb/spectral → immersive submersion). No ordering concern.

---

## Area 4 — OBRIX DSP (`Source/Engines/Obrix/ObrixEngine.h`)

**Status: WARN**

### Findings

**[High] `ObrixEngine::renderBlock` signature does not match the `SynthEngine` interface**
File: `Source/Engines/Obrix/ObrixEngine.h` line 326; `Source/Core/SynthEngine.h` line 73
OBRIX declares:
```cpp
void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
```
The internal `SynthEngine` interface declares:
```cpp
virtual void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) = 0;
```
These match. However the **SDK** `SynthEngine` (in `SDK/include/xomnibus/SynthEngine.h`) declares a different, JUCE-free signature:
```cpp
virtual void renderBlock (StereoBuffer& buffer, const MidiEventList& midi) = 0;
```
OBRIX is written against the internal JUCE-based interface (correct for in-tree use), but the SDK `MinimalEngine` template correctly uses the JUCE-free SDK interface. This is not a runtime bug but is a documentation gap: the `CLAUDE.md` states "All DSP lives inline in .h headers" and implies engines are SDK-compatible. OBRIX is not portable to the SDK without an adapter layer.
**Severity:** High for documentation/portability, not for runtime stability.
**Fix:** Explicitly document in OBRIX's header that it is an internal engine (not SDK-portable). The SDK `EngineModule.h` macro calls the SDK `renderBlock`, and any future attempt to compile OBRIX against the SDK will fail at compile time — which is the correct failure mode. Consider adding a comment: `// Internal engine — uses JUCE SynthEngine interface, not SDK StereoBuffer interface`.

**[Medium] Per-sample CytomicSVF coefficient recalculation inside the voice loop**
File: `ObrixEngine.h` ~lines 619–622, 634–636, 652–655
```cpp
voice.procFilters[0].setCoefficients (cut, res, sr);
float fbIn = sig1 + fastTanh (voice.procFbState[0] * proc1Fb * 4.0f);
float filtOut = voice.procFilters[0].processSample (fbIn);
```
`setCoefficients()` is called per sample, per voice, per active processor — up to 24 calls per sample at 8 voices × 3 processors. At 48 kHz with 8 voices, this is 24 × 48000 = 1.15M coefficient sets per second. CytomicSVF coefficient computation involves `std::tan()` (or `fastTan()`). Even with FastMath, this is significant.
**Mitigation exists:** `cutoffMod` is computed once per voice per sample (not per processor), and the coefficients only change when modulation changes. However since modulation (including LFOs) can change every sample, coefficients genuinely need per-sample updates when mod is active.
**Fix (optimization):** Add a dirty flag per voice: only call `setCoefficients()` when `cut` or `res` has changed by more than 0.1% from the last set value. This eliminates most redundant recalculations during held notes with slow/no modulation.

**[Medium] Unison voice count not validated against `kMaxVoices`**
File: `ObrixEngine.h` ~lines 452–461
```cpp
const int unisonSize = calcUnisonSize (voiceModeIdx, unisonDetune);
for (int u = 1; u < unisonSize; ++u)
    noteOn (msg.getNoteNumber(), ...);
```
If `calcUnisonSize()` returns a value larger than `kMaxVoices` (8) and voices are already partially active, the `noteOn()` calls will silently fail to find a free voice slot but not clip `unisonSize`. This is harmless (voice stealing handles it) but can leave fewer actual unison voices than expected with no feedback to the user.
**Fix (Low):** Assert or cap: `unisonSize = std::min(unisonSize, kMaxVoices)`.

**[Low] `ObrixADSR` linear attack is inconsistent with the architecture notes**
File: `ObrixEngine.h` lines 118–119
```cpp
case Stage::Attack:
    level += aRate;
    if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
```
Attack is linear. The CLAUDE.md architecture notes warn: "Offline render envelopes must match live engine (exponential vs linear) or exports will sound different." OBRIX's attack is linear (consistent in both live and offline contexts since there is no separate offline render for OBRIX), but it means velocity-scaled filter envelopes will have a linear ramp to full brightness, which can sound abrupt on long attack times. This is a sound design preference, not a DSP bug.
**Fix (optional):** Consider exponential attack with `aRate = exp(-log(9999) / (attackTime * sr))` for a more natural feel, matching the decay/release exponential shape.

**PolyBLEP anti-aliasing (PASS):**
The `PolyBLEP` implementation is correct. The 2nd-order polynomial correction is applied at both transition edges for Square and Pulse. Triangle is derived from integrated Square with the DC-blocking leaky integrator (0.999 factor). The frequency clamp to Nyquist/2 is correctly applied.

**FM feedback stability (PASS):**
FM depth is bipolar `[-1, 1]` → `± 24 semitones`. The resulting `fmSemitones` goes through `fastPow2(fmSemitones / 12.0f)` which is bounded. No self-oscillation risk. Filter feedback is limited by `fastTanh()` on both the feedback amount and the filter output.

**Drift Bus correctness (PASS):**
The irrational voice offset (0.23) correctly prevents voice slot phase-locking. The Drift Bus phase advances at `driftRate / sr` each sample, giving correct time-domain LFO behavior regardless of sample rate.

**Journey Mode (PASS):**
Note-off suppression is clean: `if (!journeyMode_) noteOff(...)` is evaluated before `reset()` (which responds to AllNotesOff/AllSoundOff and correctly clears voices regardless of Journey mode).

**Memory management in unison (PASS):**
Unison voices share the same `fxSlots[3]` array (not per-voice). This is an intentional design: the FX are post-mix. No per-voice allocation occurs in `noteOn()`.

**All 65 parameters attached (PASS):**
The `attachParameters()` call covers all Wave 1 (44), Wave 2 (5), and Wave 3 (5) = 54 float parameters + 11 choice parameters = 65 total. All `pXxx` pointers are populated.

---

## Area 5 — SDK (`SDK/include/xomnibus/`)

**Status: PASS**

### Findings

**[Medium] `CouplingType` enum is defined in two places with different values**
File: `SDK/include/xomnibus/CouplingTypes.h` vs `Source/Core/SynthEngine.h`
The SDK `CouplingTypes.h` defines 13 types (`AmpToFilter` through `AudioToBuffer`). The internal `Source/Core/SynthEngine.h` defines 14 types — it includes `KnotTopology` as a 14th type. The SDK is missing `KnotTopology`.
This means community engines built against the SDK cannot declare support for KnotTopology coupling. If XOmnibus sends a `KnotTopology` coupling signal to an SDK engine, the `applyCouplingInput()` default handler will simply ignore it (the default implementation returns without action). This is safe but limits SDK engines from opting into the Knot coupling system.
**Fix:** Add `KnotTopology` to `SDK/include/xomnibus/CouplingTypes.h` and update `kNumCouplingTypes` from 13 to 14, with a comment noting it is a V2 feature. Alternatively, leave it out until V2 and document the gap.

**[Low] `kNumCouplingTypes = 13` is a magic constant that can silently go stale**
File: `SDK/include/xomnibus/CouplingTypes.h` line 27
```cpp
constexpr int kNumCouplingTypes = 13;
```
If a new type is added to the enum without updating this constant, iterating code will silently miss the new type.
**Fix:** Use `static_cast<int>(CouplingType::AudioToBuffer) + 1` or add a `kCount` sentinel at the end of the enum.

**[Low] `EngineMetadata::sdkVersion` is hardcoded to 1 in the export macro and never externally configurable**
File: `SDK/include/xomnibus/EngineModule.h` line 72
```cpp
out->sdkVersion = 1;
```
The macro always writes version 1. When the SDK version increments, community engines built against old headers will still report version 1, which could confuse the version-checking loader. The version should come from a header-defined constant.
**Fix:** Define `constexpr int kSDKVersion = 1;` in `SynthEngine.h` and use it in the macro: `out->sdkVersion = kSDKVersion;`. Bump the constant when breaking changes are made.

**[Low] `MinimalEngine` template uses `std::sin()` instead of `fastSin()`**
File: `SDK/templates/MinimalEngine/MinimalEngine.h` line 57
```cpp
float sample = std::sin (phase * 6.28318530718f) * velocity * 0.5f;
```
Community developers will take this as a reference implementation. The SDK's `SynthEngine.h` says "no JUCE dependency" but doesn't bundle `FastMath.h`. If performance is important, the template should either use `std::sin` (as it does, fine for a minimal example) and note that it can be replaced with a fast approximation, or the SDK should include a standalone `FastMath.h`.
**Fix (Low):** Add a comment: `// Replace std::sin with a fast approximation in production code`. Optionally include a header-only `fastSin` in the SDK.

**JUCE-free guarantee (PASS):**
`SDK/include/xomnibus/SynthEngine.h`, `CouplingTypes.h`, and `EngineModule.h` contain zero JUCE includes. The only standard library headers used are `<string>`, `<cstdint>`, `<vector>`, `<memory>`, `<cstring>`. The SDK is clean.

**API correctness (PASS):**
The `SynthEngine` pure virtuals (`prepare`, `reset`, `renderBlock`, `getParameterDefs`, `getEngineId`) are the minimal correct set. Default no-op implementations for optional methods (`releaseResources`, `setParameter`, `getParameter`, `getSampleForCoupling`, `applyCouplingInput`) are appropriate and safe.

**Export macro (PASS):**
`XOMNIBUS_EXPORT_ENGINE` uses `std::memset` + `std::strncpy` with `sizeof(field) - 1` guards. The null-pointer guard `if (!out) return;` is present. Cross-platform `__declspec(dllexport)` / `__attribute__((visibility("default")))` handled correctly.

**MinimalEngine template (PASS):**
The template compiles against the SDK interface (uses `StereoBuffer`, `MidiEventList`), demonstrates note-on/off MIDI handling, additive audio output (correct for XOmnibus's summing model), and correct parameter definition format.

---

## Summary Table

| Area | Status | Critical | High | Medium | Low |
|------|--------|----------|------|--------|-----|
| Aquatic FX | WARN | 0 | 1 | 2 | 2 |
| Mathematical FX | PASS | 0 | 1* | 3 | 1 |
| Boutique FX | PASS | 0 | 0 | 2 | 2 |
| OBRIX DSP | WARN | 0 | 1** | 2 | 2 |
| SDK | PASS | 0 | 0 | 1 | 3 |
| **TOTAL** | | **0** | **3** | **10** | **10** |

\* AttractorDrive 44100 hardcode may be intentional — marked High pending design review
\*\* OBRIX renderBlock interface mismatch is documentation/portability, not runtime

---

## Priority Fix Order

1. **AquaticFXSuite.h ~line 686** — Reef FDN read-position buffer overflow risk at high sample rates
2. **EntropyCooler.h ~line 110** — O(N) entropy computation called once per sample — move to sub-rate
3. **EntropyCooler.h ~lines 129–130** — hoist constant demon coefficients out of sample loop
4. **AnomalyEngine.h line 224** — stereo slip signal double-mixed in R channel (likely bug)
5. **AttractorDrive.h line 84** — review 44100 normalization direction for 96 kHz behavior
6. **AquaticFXSuite.h line 503** — `stereoPhaseRad` silently ignored (D004 violation potential)
7. **ObrixEngine.h lines 619–636** — SVF coefficient dirty-flag optimization
8. **CouplingTypes.h** — add KnotTopology to SDK + update count constant
9. **EngineModule.h** — extract `sdkVersion` to named constant

---

*Generated by Claude Code sweep on 2026-03-20. Re-run after fixes to clear findings.*
