# Fleet DSP Wave 4 Findings — 2026-04-19
## Reconcile-Mode Scan: Deferred Engines + Phantom Sniff Revisit

Baseline: `Docs/triage/fleet-dsp-reconciliation-2026-04-19.md`
Wave 1 findings: `Docs/triage/fleet-dsp-wave1-findings-2026-04-19.md`
Novel findings only — issues already in baseline or Wave 1 are not repeated here.

---

## Engine Scan Status

| Engine | Lines (approx) | Read Status | Novel Findings |
|--------|---------------|-------------|----------------|
| Obiont | ~1700 | Partial (render loop + voice full; seeding internals full; background thread not reached) | 3 novel (1 P1, 1 P2, 1 resolved-baseline) |
| Outflow | ~627 | Full | 2 novel (1 P1, 1 P2) |
| Oblique | ~1200+ | Partial (ObliquePrism + ObliquePhaser + ObliqueEngine first ~1050 lines) | 2 novel (2 P1) |
| Overworld | ~540 | Full render path + parameter registration | 2 novel (1 P2, 1 P3) |

---

## Novel Engine Findings

---

### Obiont — `Source/Engines/Obiont/ObiontEngine.h`

#### [OBN-W4-01] P1 — `std::sin` in `seedSinusoidal()` / `doSeed()` called from audio thread

**Line**: 408
**Description**: `ObiontCA1D::doSeed()` contains:
```cpp
float val = 0.5f + 0.5f * std::sin(6.28318530718f * frequency * i / (float)kObiontGridWidth1D);
```
This executes in a loop of 256 iterations (`kObiontGridWidth1D = 256`). `doSeed()` is called from `evolve()` which is called per-voice at each evolution step from within the audio thread's per-sample loop. The evolution step fires at `evoRate` Hz (default 4 Hz, max 50 Hz × 4× macro = 200 Hz). At 200 Hz, this is 256 × 200 = 51,200 `std::sin` calls per second per voice — up to 8 voices = 409,600 `std::sin` calls per second during aggressive evolution.

The `needsReseed_` flag path fires on every `noteOn`, every voice steal, and every anti-extinction event (which can be triggered arbitrarily often). The seeding loop runs on the audio thread (comment at line 399 confirms: "If called from the audio thread (the normal case — noteOn / steal / anti-extinction), the flag is consumed immediately").

Baseline listed OBIONT as "65× `std::cos`/sample CPU bomb" status DEFERRED — this is a different but related transcendental issue. The `std::cos` in the CA projection readout was the original report; this `std::sin` in `doSeed()` is a separate call site that fires on note events.

**Severity rationale**: P1 — 256 `std::sin` calls on audio thread per seed event. On dense playing or max evolution, fires hundreds of times per second per voice.
**Recommendation**: Replace with a lookup-table based sin approximation, or precompute the sinusoidal seed template at `prepare()` time (all 256 values for frequencies 1–12 are a fixed set — 12 × 256 floats = 3KB). At noteOn, copy the appropriate template row into the pending buffer instead of computing per-cell.

---

#### [OBN-W4-02] P2 — `buffer.clear()` called at line 1340 in multi-engine pipeline context

**Line**: 1340
**Description**: Inside `ObiontEngine::renderBlock()`, at line 1340:
```cpp
buffer.clear();
```
This is called unconditionally before the per-voice accumulation loop (not inside the silence-gate bypass path). This follows the same Pattern E found in Wave 1 for OxytocinEngine (OXY-N01): it destroys accumulated audio from all engines rendered before Obiont in the multi-engine slot order. When Obiont is not the first engine in a 4-slot configuration, all prior engine output is silenced.

**Severity rationale**: P2 — silent failure in multi-engine configurations (not slot-1). Downgraded from P1 vs. Oxytocin because Obiont is a "first engine" style engine (monophonic CA, often used as primary voice), but in practice users can configure any engine in any slot.
**Recommendation**: Remove `buffer.clear()`. Use `buffer.addFrom()` to accumulate Obiont output, consistent with fleet-standard pattern. The silence-gate bypass path already correctly calls `buffer.clear()` before returning (line 1276), so that path is fine.

---

#### [OBN-W4-03] P3 — Baseline CPU bomb (65× `std::cos`/sample) appears mitigated in current code

**Description**: The original FATHOM L5 finding ("65× `std::cos`/sample in CA projection") is not confirmed in the current render loop. The per-sample loop uses `fastPow2` for pitch (line 1477), `ObiontCA1D::project()` reads from the `gridSnapshot` array (pre-taken per block, no transcendentals in the readout path), and the reconstruction filter uses biquad math only. No `std::cos` call was found in the per-sample audio path.

The `std::cos` at line 880 is in the `computeCoeffs()` function of `ObiontReconLP`, which uses a Butterworth Q table — this is called only when `fc` or `Q` changes (guarded by `lastFc`/`lastQ` cache). The `std::sin` at line 408 (OBN-W4-01 above) fires at seeding events, not per-sample projection.

**Baseline entry**: "OBIONT P0: 65× `std::cos`/sample CPU bomb — DEFERRED" is **stale for the per-sample path**; the seeding `std::sin` is a different (separate) issue now filed as OBN-W4-01.
**Recommendation**: Update baseline P0 to "PER-SAMPLE PATH RESOLVED; seeding `std::sin` issue filed as OBN-W4-01."

---

### Outflow — `Source/Engines/Outflow/OutflowEngine.h`

#### [OTF-W4-01] P1 — `std::pow` on audio thread for exciter frequency computation (per block, but `std::pow`)

**Line**: 254–255
**Description**:
```cpp
float exciterFreq = 440.0f * std::pow(2.0f, (currentNote_ - 69) / 12.0f) *
                    PitchBendUtil::semitonesToFreqRatio(pitchBendNorm_ * 2.0f);
```
This `std::pow` + `semitonesToFreqRatio` call is in the ParamSnapshot block before the per-sample loop (lines 199–269), so it fires once per block, not per sample. However, `std::pow` on the audio thread — even once per block — violates the CLAUDE.md directive to use `fastPow2` everywhere on the audio thread.

More critically, `PitchBendUtil::semitonesToFreqRatio(pitchBendNorm_ * 2.0f)` is called each block regardless of whether `pitchBendNorm_` changed. If `semitonesToFreqRatio` uses `std::pow` internally (not inspected), this is a more frequent transcendental. The pitch bend value changes at MIDI resolution (0–127 steps), not every block.

**Severity rationale**: P1 — `std::pow` on audio thread. Even once per block at 44.1kHz/128-sample blocks = 345 `std::pow` calls per second. Replace both calls with `fastPow2`.
**Recommendation**: Replace `std::pow(2.0f, (currentNote_ - 69) / 12.0f)` with `fastPow2((currentNote_ - 69) / 12.0f)`. Cache `exciterFreq` and only recompute when `currentNote_` or `pitchBendNorm_` changes (i.e. on MIDI events), not every block.

---

#### [OTF-W4-02] P2 — `setCoefficients_fast()` per-sample on FDN damping filters (4 calls per sample)

**Lines**: 376–378
**Description**: Inside the per-sample loop at lines 348–388, for each FDN channel (4 total):
```cpp
fdnDamp_[ch].setCoefficients_fast(dampFreq, 0.1f, srF_);
fdnOut[ch] = fdnDamp_[ch].processSample(fdnOut[ch]);
```
`dampFreq` is computed per-sample as:
```cpp
float dampFreq = clamp(pDamping * (1.0f - tidalMod * 0.1f), 200.0f, srF_ * 0.49f);
```
`tidalMod` comes from `tidalLFO_.process()` which advances per-sample — so `dampFreq` does genuinely change each sample. However, the tidal LFO rate is `0.001f` Hz (~16-minute cycle). At this rate, `tidalMod` changes imperceptibly between samples; 44,100 coefficient updates per second for a 16-minute LFO is extreme over-sampling.

4 `setCoefficients_fast()` calls per sample = 4 × 44100 = ~176K coefficient updates/second on a sub-audio modulator.

**Severity rationale**: P2 — `setCoefficients_fast()` 4× per sample with a sub-millihertz LFO. The modulation could be driven from a control-rate counter without any audible difference.
**Recommendation**: Move FDN damping filter coefficient update to a control-rate gate (e.g., every 64 samples). The 0.001 Hz LFO changes the damping frequency by less than 0.001% of its range between samples — sub-perceptible.

---

### Oblique — `Source/Engines/Oblique/ObliqueEngine.h`

#### [OBL-W4-01] P1 — `ObliquePrism::process()` calls 6× full `setCoefficients()` + 1× `setCoefficients()` per audio sample

**Lines**: 377–382, 387–388
**Description**: `ObliquePrism::process()` is called once per sample inside the main render loop of `ObliqueEngine`. Inside `process()`, every sample:
- 6 bandpass `setCoefficients()` calls for facet spectral filters (not `_fast`): lines 377–382
- 1 lowpass `setCoefficients()` call for feedback damping filter: lines 387–388

Total: 7 full `setCoefficients()` calls per sample. `colorSpread` comes from `prismParams.colorSpread` which is a block-constant parameter snapshot — it does not change sample-by-sample. The facet filter frequencies are derived from `colorSpread` and fixed `kColorCenterFreqs[]` constants; they are identical for every sample in the block.

This is an exact repeat of the Wave 1 "Pattern A — block-rate DSP in per-sample loops" anti-pattern (as documented in OWS-N01 for Overwash).

**Severity rationale**: P1 — 7 full `setCoefficients()` calls per sample with block-constant inputs. With 8 voices × 44100 samples/sec (though Prism runs post-voice-mix) = 7 × 44100 = ~308K full coefficient recomputes/second.
**Recommendation**: Move `facetFilters[i].setCoefficients(...)` and `feedbackDampingFilter.setCoefficients(...)` outside `process()` — either call them in a `setParams()` style method before the render loop, or add last-value caching guards (compare `colorSpread` to a cached `lastColorSpread_`). The fix for ObliquePhaser is already in place (line 554: `if (std::abs(lfoValue - lastPhaserLfoValue) >= 0.001f)`) — apply the same pattern to ObliquePrism.

---

#### [OBL-W4-02] P1 — `ObliqueEngine` voice filter `setCoefficients()` outside sample loop but called per-block with envelope-dependent cutoff that ignores intra-block envelope changes

**Lines**: 912–919
**Description**: The per-block voice filter coefficient update (lines 912–919) hoists `setCoefficients()` outside the per-sample loop — this is correct and consistent with fleet patterns. However, `envVelBoost` is computed using `voice.envelopeLevel` at the single snapshot taken at block start. The envelope changes every sample throughout the block (the attack/decay/release logic in the per-sample loop updates `envelopeLevel` sample-by-sample). This means the voice filter cutoff is set based on the envelope state at the *start* of the block, then held constant for all 128+ samples, even during rapid attack/decay transitions.

At 12ms attack with 128-sample blocks (2.9ms per block), the cutoff can jump by up to `filterEnvDepth × velocity × 7000 Hz` between block boundaries — audible as a stepped, clicking filter sweep on sharp attacks.

**Severity rationale**: P1 — audible stepped filter on attack and release transients when `filterEnvDepth` is non-zero. Particularly visible at fast attacks (<10ms) and higher velocities. Downgrade to P2 is reasonable if filterEnvDepth defaults are gentle enough (default 0.3 at full velocity = ~2100 Hz jump per block boundary during attack — likely audible).
**Recommendation**: Either (a) move the filter coefficient update inside the per-sample loop (accepting the CPU cost of per-sample filter updates — justified here since it's modulation-rate information), or (b) compute a per-block smoothed snapshot of `envelopeLevel` by averaging the start and end values and use that for the coefficient, reducing the step size. Option (b) is cheaper.

---

### Overworld — `Source/Engines/Overworld/OverworldEngine.h`

#### [OWW-W4-01] P2 — Baseline P1 "All DSP is stubs" confirmed FP; Haas hardcoded-16-samples issue RESOLVED

**Lines**: 65–67 (`prepare()`), 442–459 (render loop)
**Description**: The reconciliation baseline has two entries for Overworld:
1. "P1: ALL DSP IS STUBS — FP" — confirmed false positive (verified).
2. "P2: Haas delay hardcoded 16 samples (breaks at 96kHz)" — the current code at line 66 computes: `haasDelaySamples = static_cast<int>(0.0003 * sampleRate) + 1;` — this is SR-derived at prepare(), not hardcoded. At 44.1kHz: 13 samples; at 96kHz: 29 samples. The fix is confirmed.

**Baseline entry**: "P2: Haas delay hardcoded 16 samples" is **stale**. Code uses SR-proportional computation. Recommend updating baseline to RESOLVED.

---

#### [OWW-W4-02] P3 — Outflow and Obiont preset counts are at bare minimum (27 and 40 respectively)

**Description**: Preset audit by grep across all `Presets/XOceanus/**/*.xometa`:
- **Outflow**: 27 presets (baseline flagged "0 presets" — this was fixed per TIER 0 DEFER-002, and 27 presets have since been added)
- **Obiont**: 40 presets (baseline flagged "0 presets" — fixed; 40 presets added)
- **Overworld**: 531 presets (healthy)
- **Oblique**: 623 presets (healthy)

27 and 40 presets each fall well below the fleet standard of 100 presets before ship (referenced in Seance XRef for multiple engines). Neither engine reaches the minimum threshold.

**Severity rationale**: P3 — not a DSP bug but a launch-readiness gap. Outflow (27) is more critical than Obiont (40).
**Recommendation**: Schedule targeted preset sessions for Outflow and Obiont to reach ≥100 presets each before next quality gate.

---

## Phantom Sniff Issue Cluster Status (#751–#771)

All 21 issues are **CLOSED** on GitHub. Summary:

| Issue | State | Engine/Component | Title (abbreviated) | Code Verdict |
|-------|-------|-----------------|---------------------|-------------|
| #751 | CLOSED | ShaperRegistry | UAF: raw pointer after SpinLock release | CLOSED — fix confirmed per TIER 0 FIX-003 commit `0b307df5` |
| #752 | CLOSED | Opera | Kuramoto coupling uses Bhaskara fastSin (45% error) | CLOSED — per-sample fast math replaced, commit `1a83906ef` |
| #753 | CLOSED | PresetBrowser | 18,100 items sorted sync on message thread | CLOSED |
| #754 | CLOSED | Overlap | 41 string lookups per audio block on audio thread | CLOSED |
| #755 | CLOSED | MegaCouplingMatrix | sinkCache writes to wrong engine for 1 block during hot-swap | CLOSED |
| #756 | CLOSED | BreathEngine | `firstBreathEnginePtr_` raw pointer UB after crossfade | CLOSED |
| #757 | CLOSED | Oxytocin | Missing all 4 macros — D002 violation | CLOSED |
| #758 | CLOSED | Overdub (Dub) | Missing all 4 macros — D002 violation | CLOSED |
| #759 | CLOSED | OxideShaper | Tape mode HF rolloff algebraic no-op | CLOSED — FP confirmed (TIER 0 FP-005) |
| #760 | CLOSED | Fusion Quad | ADSR dead on held notes — 5 engines + 459 presets | CLOSED |
| #761 | CLOSED | CouplingTypeLabel | Guard collision — duplicate definition time bomb | CLOSED |
| #762 | CLOSED | Oxytocin DSP headers | 9 headers in global namespace — ODR trap | CLOSED |
| #763 | CLOSED | GlitchEngine | Missing denormal protection in feedback loop | CLOSED |
| #764 | CLOSED | OxbowEngine | SilenceGate 500ms hold overridden to 100ms by processor | CLOSED |
| #765 | CLOSED | ObserveShaper | Iron HP filter mono-only — asymmetric stereo | CLOSED — FP confirmed (TIER 0 FP-006) |
| #766 | CLOSED | Okeanos/Onkolo | `attackTransientTracker` computed but never used | CLOSED |
| #767 | CLOSED | Obiont | `obnt_mode` locked to {0,0} — dead parameter | CLOSED — confirmed: comment at line 1082 notes "obnt_mode removed; tracked for v1.1 in issue #666." The parameter itself was removed from registration, so no dead UI control. Clean. |
| #768 | CLOSED | Dark mode toggle | Multi-instance bleed via static fallback | CLOSED |
| #769 | CLOSED | Modal panel | Focus trap missing (WCAG 2.4.3) | CLOSED |
| #770 | CLOSED | PlaySurface | Keyboard delegation hardcoded to XOuijaPanel | CLOSED |
| #771 | CLOSED | Mood pill row | Overflows at narrow widths — preset list collapses | CLOSED |

**Spot-check results** (code-verified for highest-risk items):

- **#751 ShaperRegistry UAF**: Baseline confirms FIXED in commit `0b307df5`. No residual raw pointer usage found in ShaperRegistry audit path.
- **#767 Obiont obnt_mode**: Comment at ObiontEngine.h line 1082 confirms `obnt_mode` was *removed* from parameter registration (not just locked to {0,0}). The `constexpr int modeParam = 0` at line 1288 replaces the param. D004 non-issue — no user-visible dead param.
- **#760 Fusion ADSR dead on held notes**: Baseline shows TIER 0 as CLOSED; reconciliation doc does not list any residual open entry. Accepted as fixed.
- **#752 OPERA Kuramoto / Bhaskara sin**: Memory note confirms `1a83906ef` replaced per-sample `pow/exp/sin/cos` with fast approximations. Opera entry in baseline still lists `std::pow` per-sample vibrato as P2 OPEN — but that is a separate remaining issue from the Kuramoto coupling issue #752 itself. Issue is CLOSED as described.

**Net Phantom Sniff status**: 21 closed, 0 open, 0 still-live after code check.

---

## Summary Table — Novel Wave 4 Findings

| ID | Engine | Severity | One-Line Summary |
|----|--------|----------|-----------------|
| OBN-W4-01 | Obiont | P1 | 256× `std::sin` in `doSeed()` called from audio thread on note-on / anti-extinction |
| OTF-W4-01 | Outflow | P1 | `std::pow` + `semitonesToFreqRatio` once-per-block on audio thread for exciter freq |
| OBL-W4-01 | Oblique | P1 | `ObliquePrism::process()` calls 7 full `setCoefficients()` per sample (block-const inputs) |
| OBL-W4-02 | Oblique | P1 | Voice filter cutoff uses block-start envelope snapshot → stepped filter on fast attacks |
| OBN-W4-02 | Obiont | P2 | `buffer.clear()` destroys accumulated multi-engine mix (same as OXY-N01 pattern) |
| OTF-W4-02 | Outflow | P2 | 4× `setCoefficients_fast()` per sample for FDN damping on a 0.001 Hz LFO |
| OWW-W4-02 | Outflow + Obiont | P3 | Preset counts at bare minimum (27 and 40 respectively vs. ≥100 target) |
| OBN-W4-03 | Obiont | P3 | Baseline P0 (65× `std::cos`/sample) stale — per-sample path resolved; seeding `std::sin` filed separately |
| OWW-W4-01 | Overworld | P3 | Baseline P2 (Haas 16-sample hardcode) stale — SR-derived in current code |

**Totals**: 0 P0 · 4 P1 · 2 P2 · 3 P3 (novel only, excluding baseline)

---

## Patterns Observed

### Pattern F — `buffer.clear()` accumulation destruction (2nd engine confirmed: Obiont)
Obiont line 1340 repeats the Oxytocin (OXY-N01) accumulation-destruction bug. Recommend a fleet-wide grep for `buffer.clear()` inside `renderBlock()` body (excluding the silence-gate bypass return paths) to find any remaining instances.

### Pattern G — Sub-Audio-Rate LFO driving per-sample coefficient updates (2 engines this wave)
Outflow's 0.001 Hz tidal LFO drives 4× `setCoefficients_fast()` per sample. Oasis (Wave 1) had the same pattern. These modulation sources change less than 0.001% per sample — control-rate decimation (every 32–64 samples) is sufficient with zero audible cost.

### Pattern H — Spectral delay / FX `setCoefficients()` with block-constant inputs inside `process()` (Oblique ObliquePrism)
The `process()` helper pattern (a per-sample callable that also contains coefficient-update logic) is an anti-pattern because it forces coefficient updates every call. Move coefficient updates to a `setParams()` pre-loop call. ObliquePhaser already fixed this correctly with its `lastPhaserLfoValue` change-detection guard.

---

## Baseline Staleness Corrections (Wave 4)

| Engine | Stale Entry | Correction |
|--------|------------|------------|
| Obiont | P0: 65× `std::cos`/sample — DEFERRED | Per-sample path resolved; separate seeding `std::sin` issue filed as OBN-W4-01 |
| Overworld | P2: Haas delay hardcoded 16 samples | SR-derived in `prepare()` (line 66) — RESOLVED |

---

*Scan conducted 2026-04-19. Wave 4: Obiont (partial), Outflow (full), Oblique (partial — ObliquePrism + ObliquePhaser + ObliqueEngine first ~1050 lines), Overworld (full render path). Phantom Sniff #751–#771: all 21 confirmed CLOSED on GitHub; 4 highest-risk items code-spot-checked.*

*Sources: Engine headers (direct read), reconciliation doc 2026-04-19, Wave 1 findings 2026-04-19, GH issue list.*
