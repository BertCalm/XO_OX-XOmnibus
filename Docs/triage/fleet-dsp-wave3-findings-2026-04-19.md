# Fleet DSP Wave 3 Findings — 2026-04-19
## Reconcile-Mode Scan: New Engines (5 engines, all post-FATHOM-L5)

Baseline: `Docs/triage/fleet-dsp-reconciliation-2026-04-19.md` (103 known) + Wave 1 findings + recent fix commits (869143256, 8eb2d4685, e3ba32248, de54fc66b).
Novel findings only.

---

## Scan Status

| Engine | File | Lines | Read Status | Novel Findings |
|--------|------|-------|-------------|----------------|
| Ostracon | `Source/Engines/Ostracon/OstraconEngine.h` | 1336 | Full | 3 (P2, P2, P3) |
| Ogive | `Source/Engines/Ogive/OgiveEngine.h` | 1533 | Full (in 2 passes) | 3 (P1, P2, P3) |
| Olvido | `Source/Engines/Olvido/OlvidoEngine.h` | 1527 | Full (in 2 passes) | 4 (P1, P1, P2, P3) |
| Oobleck | `Source/Engines/Oobleck/OobleckEngine.h` | 1446 | Full (render core + GS update) | 1 (P3) |
| Ooze | `Source/Engines/Ooze/OozeEngine.h` | 1368 | Full (render core) | 2 (P2, P3) |

---

## Novel Findings by Engine

---

### OSTRACON — `Source/Engines/Ostracon/OstraconEngine.h`

#### [OSTR-N01] P2 — Per-sample per-head `setCoefficients_fast` with input that changes every sample
**Line**: 659 — `voice.oxideFilter[h].setCoefficients_fast(oxideCutoff, 0.3f, currentSampleRate);`
**Description**: Inside PASS 3 per-sample voice loop (line 425 `for (int sampleIdx…)`), for every active voice and every read head (up to 4), `setCoefficients_fast()` is called once per sample because `oxideCutoff` depends on `normDist` which is derived from the per-sample-advancing `voice.readPos[h]`. At 8 voices × 4 heads = up to 32 `setCoefficients_fast` calls per sample, plus `fastExp` per call. This is correct-but-expensive: the cutoff DOES change each sample because head positions advance. If `setCoefficients_fast` is non-trivial (involves exp or trig), this is a latent CPU concern at full polyphony.
**Recommendation**: Decimate oxide cutoff updates to every 4–8 samples (inaudible at LFO rates); hold `normDist` constant between updates using a low-pass follower on the distance value.

#### [OSTR-N02] P2 — D001 documentation claim does not match code
**Lines**: 502–504, 724–726
**Description**: `// D001: velocity shapes brightness via velFilterMod (already baked into write gain in PASS 2)` at line 724 is misleading. `velGain = voice.velocity²` at line 503 scales the **amplitude** of the signal written to tape — this is a loudness effect, not a timbre/brightness effect. No velocity-dependent filter cutoff offset or harmonic content modulation exists in Ostracon. D001 (velocity → timbre, not just amplitude) is technically incomplete: the output filter cutoff at lines 715–716 does not vary with velocity.
**Recommendation**: Add `velFilterMod = voice.velocity * voice.velocity * 3000.0f` to `effectiveCutoff` for the output filter (consistent with Oobleck/Ooze pattern), or update doctrine comment to accurately describe the write-gain mechanism as a timbre proxy.

#### [OSTR-N03] P3 — `midiNoteToHz` uses `std::pow` — noteOn path only, not per-sample
**Line**: 1054 — `return 440.0f * std::pow(2.0f, (static_cast<float>(noteNum) - 69.0f) / 12.0f);`
**Description**: Called only in `doNoteOn()`, not in the render path. Not a CPU concern. However, the project convention uses `fastPow2` for this conversion in other engines (Oceanic, OceanDeep). Minor consistency issue only.
**Recommendation**: Cosmetic — replace with `440.0f * fastPow2((noteNum - 69) / 12.0f)` for consistency with fleet convention.

---

### OGIVE — `Source/Engines/Ogive/OgiveEngine.h`

#### [OGV-N01] P1 — `ogiveCurve()` calls `std::pow` per sample inside per-voice render loop
**Line**: 527 (call site), 1042 (implementation): `float t = std::pow(phase, shape);`
**Context**: `ogiveCurve(voice.scanPhase, effectiveLancet)` is called at line 527 inside the outer per-sample loop (`for (int sampleIdx…)` at line 416) inside the per-voice loop (line 439). `effectiveLancet` (= `paramLancetShape + macroMove * 1.5f`) is a block-rate constant, and `voice.scanPhase` advances linearly by `scanPhaseInc` per sample. Both inputs change at audio rate (phase) or block rate (shape). The `std::pow(phase, shape)` executes once per sample per active voice.
**Severity rationale**: `std::pow` on per-sample path × 8 voices = up to 8 `std::pow` calls/sample — comparable to the P1 CPU bombs found in Wave 1. At 96kHz this doubles.
**Fix sketch**: Move `ogiveCurve` result to control rate: compute once per control-rate interval (e.g., every 16 samples), interpolate linearly between updates. `scanPhase` advances smoothly so linear interpolation of the ogive position is perceptually transparent.

#### [OGV-N02] P2 — CC11 expression written to `aftertouchValue` — clobbers aftertouch (D006 collision)
**Lines**: 383–386
```cpp
else if (msg.getControllerNumber() == 11)
{
    // CC11 expression → neon drive (D006)
    aftertouchValue = msg.getControllerValue() / 127.0f;
}
```
**Lines**: 389–392
```cpp
else if (msg.isChannelPressure())
{
    aftertouchValue = msg.getChannelPressureValue() / 127.0f;
}
```
**Description**: Both CC11 expression and channel pressure (aftertouch) write to the same `aftertouchValue` member. Sending CC11 overwrites any live aftertouch state and vice versa. Real-time performance with simultaneous aftertouch + expression pedal will produce unpredictable results (they fight each other). The comment labels CC11 as "neon drive" but the actual use of `aftertouchValue` at line 413 is scan position offset — the comment is also incorrect.
**Fix sketch**: Introduce separate `expressionValue` and `aftertouchValue` fields. In `effectiveScanPos` computation blend both: `scanPos + aftertouchValue * 0.2f + expressionValue * 0.1f`.

#### [OGV-N03] P3 — `getSampleForCoupling` returns same scalar for both channels (no stereo)
**Lines**: 651–658
```cpp
float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
{
    if (channel == 0) return cachedCouplingOutput;
    if (channel == 1) return cachedCouplingOutput;
    return 0.0f;
}
```
**Description**: Both channels return `cachedCouplingOutput`, a single mono scalar. Ogive produces true stereo output (independent neon filters L+R, chorus, reverb) but coupling consumers receive the same mono average for both channels. This isn't a crash or distortion issue but means Ogive cannot feed stereo coupling routes even though its output is stereo. Minor — consistent with several other engines that also use mono coupling output.
**Recommendation**: Store `cachedCouplingOutputL` and `cachedCouplingOutputR` separately from the per-voice `lastCouplingOut` average; return per-channel from `getSampleForCoupling`.

---

### OLVIDO — `Source/Engines/Olvido/OlvidoEngine.h`

#### [OLV-N01] P1 — 10 `setCoefficients_fast` calls per sample per voice for STATIC crossover frequencies
**Lines**: 573, 578 (inside per-sample loop at line 433, per-voice loop at line 449)
```cpp
voice.crossoverLP[ci].setCoefficients_fast(actualCrossover[ci], 0.7071f, sampleRateFloat);
// …
voice.crossoverHP[ci].setCoefficients_fast(actualCrossover[ci], 0.7071f, sampleRateFloat);
```
**Description**: For each of 5 crossover points, both LP and HP `setCoefficients_fast` are called **every audio sample** for every active voice. However `actualCrossover[ci]` is derived from the `shoreline` parameter shifted at block rate — it is a block-rate constant. At 8 voices × 5 crossover points × 2 (LP+HP) = 80 `setCoefficients_fast` calls per sample. Since crossover frequencies do not change between samples (shoreline is not LFO-modulated in the per-sample path), this is 80 redundant coefficient recomputations per sample.
**Severity rationale**: If `setCoefficients_fast` involves any math beyond a multiply-add (even an `exp` approximation), this is a P1 CPU bomb at full polyphony.
**Fix sketch**: Compute crossover coefficients once at block rate (before the per-sample loop) and store them. Remove `setCoefficients_fast` calls from per-sample path entirely; coefficients are already set at noteOn (line 1333).

#### [OLV-N02] P1 — `std::pow` called 3× per sample per voice in erosion/emergence paths
**Lines**: 608, 632, 644 (inside per-sample loop at line 433)
```cpp
float decayScale = std::pow(static_cast<float>(bi + 1), 2.0f * effectiveDepth);        // line 608
float emergenceScale = std::pow(invertBandIdx + 1.0f, 2.0f * effectiveDepth);          // line 632
voice.bandDecayRate[bi] = baseRate * std::pow(static_cast<float>(bi + 1), 2.0f * effectiveDepth);  // line 644
```
**Description**: Lines 608 and 632 are in the per-band decay/emergence branches (run when `effectiveCurrentNow != 0`). Line 644 is unconditional and runs for every band every sample. `effectiveDepth` is a block-rate constant (`paramDepth + macroMove * 0.7f`). For 6 bands × 3 calls = up to 18 `std::pow` calls per voice per sample at full erosion/emergence. Additionally, `bandDecayRate[bi]` at line 644 is set per-sample but **never read** after assignment in the render loop (see below) — making line 644's `std::pow` entirely wasted work.
**Severity rationale**: 18 `std::pow` calls/sample × 8 voices = 144 `std::pow`/sample. This is a P1 CPU bomb.
**Fix sketch**: Pre-compute all `decayScale[bi]` and `emergenceScale[bi]` arrays at block rate (one `std::pow` per band, 12 total, when `effectiveDepth` changes). Replace per-sample calls with the cached values.

#### [OLV-N03] P2 — `bandDecayRate[]` written unconditionally per sample but never read in render loop (D004)
**Lines**: 643–645
```cpp
voice.bandDecayRate[bi] = baseRate *
    std::pow(static_cast<float>(bi + 1), 2.0f * effectiveDepth);
```
**Description**: `bandDecayRate` is assigned every sample at line 643 but the render loop at lines 595–645 does not read it back — `perSampleDecay` (line 609) is computed inline without using `bandDecayRate`. The field exists as a member of `OlvidoVoice` (line 109) and is also assigned in `initVoice` (line 1325) but neither location is consumed in the live render path. This is dead computation (D004 violation: a stored value that affects nothing) plus it wastes one `std::pow` call per band per sample (already counted in OLV-N02).
**Recommendation**: Remove line 643 assignment from render loop entirely. Decide whether `bandDecayRate` is intended to be a UI-readable cache (if so, document it and move assignment to block rate) or remove the member.

#### [OLV-N04] P3 — D001 velocity shapes initial band amplitudes only (noteOn, not continuous)
**Lines**: 1282–1292 (in `initVoice`)
**Description**: Velocity modulates `bandAmplitude[]` initialization: higher velocity = more high-band content. This is D001-compliant at noteOn. However, after the note starts, velocity does not continuously modulate timbre — the output gain at line 679 is `ampLevel * voice.velocity` (amplitude only, no filter brightness). Other new engines (Oobleck, Ooze) add `velFilterMod = vel² * 4000Hz` to filter cutoff on every block for continuous velocity→brightness. Olvido's approach is not wrong but the timbre shaping only happens at attack, not through the sustain phase.
**Recommendation**: Add `velFilterMod = voice.velocity * voice.velocity * 3000.0f` to `voiceCutoff` in the per-sample filter step (line 659) to maintain D001 throughout the note.

---

### OOBLECK — `Source/Engines/Oobleck/OobleckEngine.h`

#### [OOBL-N01] P3 — `lastCouplingOut` member repurposed as LFO filter cache — naming hazard
**Line**: 894
```cpp
voice.lastCouplingOut = lfoFilterMod; // repurpose as lfo-filter cache
```
**Line**: 934
```cpp
const float activeCutoff = (snap_.lfoTarget == 3 && voice.lastCouplingOut > 0.0f)
    ? voice.lastCouplingOut : voiceFilterCutoff;
```
**Description**: `OobleckVoice::lastCouplingOut` is declared as a coupling output cache but is conditionally repurposed as a per-voice LFO-modulated filter cutoff store. If `CouplingType::AudioToBuffer` is active and another engine reads `getSampleForCoupling()`, the coupling consumer receives the LFO filter cutoff value instead of the actual audio output. The `getSampleForCoupling` implementation reads `outputCacheL_`/`outputCacheR_` (line 1023–1027), not `lastCouplingOut` directly, so the bug is contained. However, reading `lastCouplingOut` at line 934 for filter cutoff when `lfoTarget != 3` would produce `0.0f` (no filter modulation) — the guard `voice.lastCouplingOut > 0.0f` avoids this. The member serves two unrelated roles, making the code fragile.
**Recommendation**: Add `float lfoFilterCache = 0.0f;` as a separate member and use it at lines 894 and 934. Restore `lastCouplingOut` to audio-only semantics.

---

### OOZE — `Source/Engines/Ooze/OozeEngine.h`

#### [OOZE-N01] P2 — `std::sin`, `std::cos`, `std::pow` in per-sample voice render loop
**Lines**: 872 (`std::sin`), 897 (`std::pow`), 918 (`std::cos`) inside per-sample loop (line 806)
```cpp
const float jetSine = std::sin(voice.jetPhase) * toneAmount;           // line 872
const float bubbleFreq = currentFreq * std::pow(2.0f, snap_.bubbleTrack / 12.0f) // line 897
const float bubbleSample = voice.bubbleAmplitude * std::cos(voice.bubblePhase * kTwoPi); // line 918
```
**Description**: Three stdlib transcendental calls per sample per active voice:
1. `std::sin(voice.jetPhase)` — jet oscillator. `jetPhase` advances at `edgeFreq/SR` per sample; this is inherently per-sample, but `fastSin` would suffice here.
2. `std::pow(2.0f, snap_.bubbleTrack / 12.0f)` — bubble semitone offset. `snap_.bubbleTrack` is a block-rate constant (from `OozeParamSnapshot::updateFrom()`). This should be computed once per block, not per sample.
3. `std::cos(voice.bubblePhase * kTwoPi)` — Minnaert bubble oscillator. Phase advances per sample; `fastCos` would suffice.
**Severity rationale**: `std::pow` call for bubble pitch at line 897 is unambiguously a block-rate constant computed per sample — P2 wasted work. `std::sin`/`std::cos` could be replaced with `fastSin`/`fastCos` (already available in FastMath.h, used elsewhere in the fleet) for P2 improvement.
**Fix sketch**: Move `bubbleFreq` computation (including `std::pow`) above the per-sample loop. Replace `std::sin` → `fastSin` and `std::cos` → `fastCos`.

#### [OOZE-N02] P3 — Waveguide model only implements one reflection path — D003 physical model incomplete
**Lines**: 930–991 (waveguide implementation)
**Description**: The waveguide writes excitation to `delayLine[writePos]` (line 940), reads end-A reflection via allpass (lines 943–952), applies reflection filter A and writes back at `writeBackPos = writePos + 1` (lines 956–991). End-B is read from `halfDelay` (line 994–995) and applied, but the output sample at line 1022 is `apOut + reflectedB + excitation * 0.05f`. This is not a closed-loop bidirectional waveguide — `reflectedB` is computed from the delay line but is NOT fed back into the delay line, breaking the physical standing-wave model. A correct Karplus-Strong / digital waveguide needs reflected energy to re-enter the delay line and sustain the resonance. As implemented, the "resonance" decays in a single pass rather than building a sustained tone. D003 (physics IS the synthesis) may not be fully honoured.
**Recommendation**: Flag for deeper physics review. `reflectedB` should be added into the write-back path at `writeBackPos` to complete the bidirectional waveguide loop. Without this, Ooze may not deliver the sustained pipe resonance its design intent claims.

---

## Cross-Engine Patterns

| Pattern | Engines | Severity |
|---------|---------|---------|
| `std::pow` per-sample in physics/scan loop | Ogive (ogiveCurve), Olvido (erosion scale), Ooze (bubbleFreq) | P1–P2 |
| `setCoefficients_fast` with block-rate-constant inputs called per-sample | Olvido (10×/sample), Ostracon (dynamic but expensive) | P1–P2 |
| `std::sin`/`std::cos` where `fastSin`/`fastCos` already available | Ooze (jet + bubble) | P2 |
| Dead computation written per-sample but never read | Olvido `bandDecayRate` (with `std::pow` cost!) | P2 |
| CC11 expression and aftertouch clobber same variable | Ogive | P2 |
| D001 velocity → timbre only at noteOn, not continuous | Olvido | P3 |

**Template candidate**: New engines with per-band physics (Olvido) need a "block-rate physics coefficient cache" pattern — pre-compute all `pow(band_index, depth)` values at block rate, cache them, and read in the per-sample loop. This same pattern applies to Ogive's scan trajectory shape coefficient.

---

## Engines Flagged for Deeper Physics Review

- **Ooze** [OOZE-N02]: The waveguide end-B reflection is not fed back into the delay loop. A pipe model without bidirectional feedback will not sustain like a real pipe — tones will decay in one delay-line pass. Needs a physics review to confirm whether sustained resonance is actually achieved or whether the output is solely from direct excitation pass-through.

- **Olvido** [OLV-N01 + N02]: Combined P1 CPU load from static crossover setCoeff + `std::pow` erosion scale may produce audible CPU spikes at full polyphony. Recommend profiling before ship.
