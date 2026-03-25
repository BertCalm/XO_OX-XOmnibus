# DSP Audit — Round 7 (2026-03-24)

**Engines audited**: Origami, Oracle, Obscura, OceanDeep, Octopus, Fat (OBESE), Drift, Onset
**Checklist** (10 items per engine):
1. Coupling accumulators reset per block
2. Buffer sizes safe at 192kHz
3. No `thread_local` in voice loops
4. No per-sample transcendentals that should be block-rate
5. SilenceGate present
6. `ScopedNoDenormals` present
7. No division-by-zero risk
8. No allocation on audio thread
9. Filter coefficients correct method
10. Coupling feedback loop protection

---

## 1. ORIGAMI — `Source/Engines/Origami/OrigamiEngine.h`

| # | Check | Result |
|---|-------|--------|
| 1 | Coupling accumulators reset per block | ✅ Lines 568-571 |
| 2 | Buffer sizes safe at 192kHz | ✅ FFT buffer 2048 samples; STFT hop 512 — fixed-size, SR-independent |
| 3 | No `thread_local` in voice loops | ✅ |
| 4 | No per-sample transcendentals | ❌ **P1 + P2** — see below |
| 5 | SilenceGate present | ✅ |
| 6 | `ScopedNoDenormals` present | ❌ Missing (fleet-wide gap) |
| 7 | No division-by-zero risk | ✅ |
| 8 | No allocation on audio thread | ✅ |
| 9 | Filter coefficients correct method | ❌ **P2** — per-sample call |
| 10 | Coupling feedback loop protection | ✅ |

**Verdict**: ISSUES

### P1 — Per-sample `std::cos`/`std::sin` for panning (lines 761-762)
Inside the per-sample, per-voice render loop:
```cpp
float panGainLeft  = std::cos(panPosition * kPI * 0.5f);
float panGainRight = std::sin(panPosition * kPI * 0.5f);
```
`panPosition` is derived from the voice index and a block-constant parameter — it never changes mid-block. These trig calls should be precomputed in `prepare()` or at block-start into a `panGainL[v]` / `panGainR[v]` array and read back each sample.

**Fix (block-start precompute, ≤8 lines):**
```cpp
// At block start, per voice:
for (int v = 0; v < kNumVoices; ++v) {
    float pan = computePanPosition(v, params);
    panGainL[v] = std::cos(pan * kPI * 0.5f);
    panGainR[v] = std::sin(pan * kPI * 0.5f);
}
// In per-sample loop: use panGainL[v], panGainR[v]
```

### P2 — Per-sample `postFilter.setCoefficients()` (line ~744)
`velBrightness` is constant per voice per block. `setCoefficients()` computes filter coefficients (likely involving `std::sin`/`std::cos`) every sample. Should be guarded by a `dirty` flag or computed once at block start.

**Fix**: Hoist to block start; set only when `velBrightness` changes.

---

## 2. ORACLE — `Source/Engines/Oracle/OracleEngine.h`

| # | Check | Result |
|---|-------|--------|
| 1 | Coupling accumulators reset per block | ✅ Lines 593-595 |
| 2 | Buffer sizes safe at 192kHz | ✅ Fixed-size internal buffers |
| 3 | No `thread_local` in voice loops | ✅ |
| 4 | No per-sample transcendentals | ✅ `evolveBreakpoints()` only at cycle boundary; `fastTanh` for limiting |
| 5 | SilenceGate present | ✅ |
| 6 | `ScopedNoDenormals` present | ❌ Missing |
| 7 | No division-by-zero risk | ✅ |
| 8 | No allocation on audio thread | ✅ |
| 9 | Filter coefficients correct method | ✅ |
| 10 | Coupling feedback loop protection | ✅ |

**Verdict**: CLEAN

No actionable bugs. `fastTanh` used correctly for the soft limiter. GENDY breakpoint evolution is rate-limited to cycle boundary events.

---

## 3. OBSCURA — `Source/Engines/Obscura/ObscuraEngine.h`

| # | Check | Result |
|---|-------|--------|
| 1 | Coupling accumulators reset per block | ✅ Snapshot-then-zero at lines 657-661 (best-practice pattern) |
| 2 | Buffer sizes safe at 192kHz | ⚠️ Minor behavioral change — see note |
| 3 | No `thread_local` in voice loops | ✅ |
| 4 | No per-sample transcendentals | ✅ Verlet integration uses only multiply/add |
| 5 | SilenceGate present | ✅ |
| 6 | `ScopedNoDenormals` present | ❌ Missing |
| 7 | No division-by-zero risk | ✅ `controlStepSamples` guarded: `if (controlStepSamples < 1.0f) controlStepSamples = 1.0f` |
| 8 | No allocation on audio thread | ✅ 128-mass chain pre-allocated |
| 9 | Filter coefficients correct method | ✅ |
| 10 | Coupling feedback loop protection | ✅ |

**Verdict**: CLEAN (with note)

**Note — 192kHz physics rate**: Physics runs at `kPhysicsControlRate = 4000 Hz`. At 44.1kHz, `controlStepSamples ≈ 11`; at 192kHz, `controlStepSamples = 48`. Physics updates become less frequent relative to audio output, changing the stiffness/damping character of the simulation. This is a **behavioral change** (softer, slower physics response at high SR), not a crash or correctness bug. Acceptable as-is; document in release notes.

---

## 4. OCEANDEEP — `Source/Engines/OceanDeep/OceandeepEngine.h`

| # | Check | Result |
|---|-------|--------|
| 1 | Coupling accumulators reset per block | ✅ Lines 746-747 |
| 2 | Buffer sizes safe at 192kHz | ❌ **P1** — see below |
| 3 | No `thread_local` in voice loops | ✅ |
| 4 | No per-sample transcendentals | ❌ **P2** — per-sample trig in DarkFilter |
| 5 | SilenceGate present | ✅ `prepareSilenceGate()` / `isSilenceGateBypassed()` |
| 6 | `ScopedNoDenormals` present | ❌ Missing |
| 7 | No division-by-zero risk | ✅ Delay index clamped |
| 8 | No allocation on audio thread | ✅ |
| 9 | Filter coefficients correct method | ⚠️ `fastCos`/`fastSin` — see P2 |
| 10 | Coupling feedback loop protection | ✅ |

**Verdict**: ISSUES

### P1 — `DeepWaveguideBody::kMaxDelay = 48001` too small for >48kHz (tuning error)
Comment reads "covers 1 Hz fundamental at up to 48 kHz" — correct only for 44.1/48kHz. At 88.2kHz, a 1Hz tone requires 88,200 delay samples; at 96kHz, 96,000 samples. The existing guard:
```cpp
if (delaySamples >= kMaxDelay) delaySamples = kMaxDelay - 1;
```
prevents a buffer overrun (no crash), but clamps the delay, causing wrong pitch for very low-frequency fundamentals at high sample rates.

**Fix**: Scale `kMaxDelay` at `prepare()` time:
```cpp
static constexpr int kMaxDelayBase = 48001;
int kMaxDelay = kMaxDelayBase; // updated in prepare()
// In prepare():
kMaxDelay = static_cast<int>(std::ceil(sampleRate)) + 1;
delayBuffer.resize(kMaxDelay, 0.0f);
```
This is heap allocation inside `prepare()` (correct — not on the audio thread per block).

### P2 — Per-sample trig in `DeepDarknessFilter::computeCoeffs()` when LFO modulates cutoff
The filter has a float-equality coefficient cache:
```cpp
if (fc == lastFc && Q == lastQ) return;
```
When `dynCutoff` is driven by an LFO it changes every sample, meaning cache misses on every call. Each miss invokes `fastCos`/`fastSin`. Consider a change-threshold guard:
```cpp
if (std::abs(fc - lastFc) < 0.5f && std::abs(Q - lastQ) < 0.001f) return;
```
Or compute at block-rate by smoothing `dynCutoff` and accepting one coefficient update per block.

**Note — `DeepAbyssalReverb` comb lengths**: Fixed comb delay times (max 701 samples) encode character at 44.1kHz (~15.9ms). At 192kHz the same 701-sample buffer is only ~3.6ms, yielding a very different room. Not a crash — but the reverb character changes significantly at pro-audio sample rates. A SR-normalized comb would require dynamic allocation in `prepare()`.

---

## 5. OCTOPUS — `Source/Engines/Octopus/OctopusEngine.h`

| # | Check | Result |
|---|-------|--------|
| 1 | Coupling accumulators reset per block | ✅ Lines 452-456 |
| 2 | Buffer sizes safe at 192kHz | ✅ |
| 3 | No `thread_local` in voice loops | ✅ |
| 4 | No per-sample transcendentals | ❌ **P1** — see below |
| 5 | SilenceGate present | ✅ |
| 6 | `ScopedNoDenormals` present | ❌ Missing |
| 7 | No division-by-zero risk | ✅ |
| 8 | No allocation on audio thread | ✅ 16 voices × 8 arms pre-allocated |
| 9 | Filter coefficients correct method | ✅ |
| 10 | Coupling feedback loop protection | ✅ |

**Verdict**: ISSUES

### P1 — Per-sample `std::exp` in chromatophore envelope coefficient
Inside the per-sample, per-voice loop (line ~642):
```cpp
float chromaCoeff = 1.0f - std::exp(-kTwoPi * (pChromaSpeed * 20.0f + 1.0f) / srf);
```
Both `pChromaSpeed` (block-constant parameter) and `srf` (block-constant sample rate float) are invariant within the block. This computes 16 voices × buffer-size `std::exp()` calls per block.

**Fix (block-start precompute, ≤6 lines):**
```cpp
// Once per block:
float chromaCoeff = 1.0f - std::exp(-kTwoPi * (pChromaSpeed * 20.0f + 1.0f) / srf);
// Pass to per-voice, per-sample loop as a constant.
```
Estimated saving: ~(blockSize × numActiveVoices) `std::exp()` calls → 0 per block.

---

## 6. FAT (OBESE) — `Source/Engines/Fat/FatEngine.h`

| # | Check | Result |
|---|-------|--------|
| 1 | Coupling accumulators reset per block | ✅ Lines 939-942 (`externalPitchMod`, `externalFilterMod`) |
| 2 | Buffer sizes safe at 192kHz | ✅ |
| 3 | No `thread_local` in voice loops | ✅ |
| 4 | No per-sample transcendentals | ❌ **P1** — see below |
| 5 | SilenceGate present | ✅ |
| 6 | `ScopedNoDenormals` present | ❌ Missing |
| 7 | No division-by-zero risk | ✅ |
| 8 | No allocation on audio thread | ✅ |
| 9 | Filter coefficients correct method | ⚠️ ZDF `std::tan()` prewarping — see P1 |
| 10 | Coupling feedback loop protection | ✅ |

**Verdict**: ISSUES

### P1 — `FatLadderFilter::computeCoeffs()` calls `std::tan()` per sample
The ZDF Moog ladder requires `std::tan(π × fc / sr)` for the prewarping coefficient. This is called once per voice per sample (hoisted from 4 calls to 1 — good partial optimization). With 6 active voices, this is 6× `std::tan()` per sample, driven by filter envelope + LFO changing cutoff each sample.

**Options (trade-offs)**:
- **Block-rate**: Smooth the filter cutoff at block-rate and compute `std::tan()` once per voice per block. This is ~standard in production synths. Slight envelope tracking lag (one block ≈ 1-5ms).
- **Fast approximation**: Replace `std::tan(x)` with Padé rational approximation for `0 < x < π/2` (error < 0.1% for `fc < 0.45 × sr`). No lag, ~5× faster than `std::tan()`.
- **Accepted risk**: Leave as-is. At typical buffer sizes (128 samples, 6 voices), this is 768 `std::tan()` calls per buffer — measurable but may be acceptable given overall CPU budget.

**Partial hoisting already present (✅ noted):**
```cpp
// subRatio, octUpRatio, octDnRatio via fastExp — hoisted to block-rate ✓
// crushLevels = std::pow(2.0f, std::floor(crushDepth)) — hoisted to block-rate ✓
// lfo2Phase is a block-rate double accumulator ✓
```

---

## 7. DRIFT — `Source/Engines/Drift/DriftEngine.h`

| # | Check | Result |
|---|-------|--------|
| 1 | Coupling accumulators reset per block | ✅ Lines 921-927 (`externalPitchMod`, `externalFilterMod`, `externalMorphMod`) |
| 2 | Buffer sizes safe at 192kHz | ✅ |
| 3 | No `thread_local` in voice loops | ✅ |
| 4 | No per-sample transcendentals | ❌ **P1** — see below |
| 5 | SilenceGate present | ✅ |
| 6 | `ScopedNoDenormals` present | ❌ Missing |
| 7 | No division-by-zero risk | ✅ |
| 8 | No allocation on audio thread | ✅ |
| 9 | Filter coefficients correct method | ✅ `DriftVoyagerDrift` caches `smoothCoeff` ✓ |
| 10 | Coupling feedback loop protection | ✅ |

**Verdict**: ISSUES

### P1 — Per-sample `std::sin` in `DriftTidalPulse::process()`
Inside the voice render loop, each sample:
```cpp
float s = static_cast<float>(std::sin(phase * twoPi));
float breathCycle = s * s;
```
`TidalPulse` is architecturally a per-sample running oscillator (phase accumulation pattern). The `std::sin` here is fundamental to its function, not an incidental coefficient.

**Options**:
- **Sine table LUT**: Replace `std::sin(phase * twoPi)` with a 4096-entry wavetable lookup + linear interpolation. Used by ~80% of the fleet's oscillator paths. Near-zero CPU cost.
- **Polynomial approximation**: Bhaskara I or minimax 5th-order (`fastSin`). Already present in several engines — import and use.
- **Accepted as architectural**: If TidalPulse represents a small fraction of the voice budget and is only active in certain modes, this may be acceptable. Characterize CPU contribution first.

**Good pattern already present (✅ noted):**
```cpp
// DriftVoyagerDrift: caches smoothCoeff when rate changes — avoids redundant std::exp ✓
// DriftPrismShimmer::updateTone(): coefficient cached ✓
// ADSR setParams() called once at sample==0 per block ✓
```

---

## 8. ONSET — `Source/Engines/Onset/OnsetEngine.h`

| # | Check | Result |
|---|-------|--------|
| 1 | Coupling accumulators reset per block | ✅ Lines 1842-1844 |
| 2 | Buffer sizes safe at 192kHz | ✅ |
| 3 | No `thread_local` in voice loops | ✅ |
| 4 | No per-sample transcendentals | ✅ All correctly hoisted |
| 5 | SilenceGate present | ✅ |
| 6 | `ScopedNoDenormals` present | ❌ Missing |
| 7 | No division-by-zero risk | ✅ |
| 8 | No allocation on audio thread | ✅ |
| 9 | Filter coefficients correct method | ✅ |
| 10 | Coupling feedback loop protection | ✅ `couplingBuffer` cleared at block start (line 1721) |

**Verdict**: CLEAN

**Exemplary hoisting pattern — reference for fixing other engines:**
```cpp
// All at block-start:
blendGainX[v] = std::cos(vBlend[v] * halfPi);   // block-rate ✓
blendGainO[v] = std::sin(vBlend[v] * halfPi);   // block-rate ✓
panGainL[v]   = std::sqrt(0.5f * (1.0f - vPan[v]));  // block-rate ✓
lofiSteps     = std::pow(2.0f, pLofiBits);       // block-rate ✓
```
Onset's XVC (Cross-Voice Coupling) uses one-block-delayed peaks — intentional design for feedback-free coupling, not a bug.

---

## Fleet-Wide Issue: `ScopedNoDenormals` Missing

All 8 audited engines lack `juce::ScopedNoDenormals` at the top of `processBlock()`. Fleet-wide grep shows only ~14 of 73 engines have it. This is a **low-priority** fleet gap because:
- Individual filter states call `flushDenormal()` on their state variables
- Denormal-prone operations (filters, envelopes) are individually protected
- Most DSP paths are multiplied by the SilenceGate before they can sustain denormals

**Recommended fix (one line per engine, trivial)**:
```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override {
    juce::ScopedNoDenormals noDenormals; // ADD THIS
    // ...
}
```
Priority: low. Batch-apply to all 73 engines in one pass.

---

## Priority Summary

| Engine | Severity | Issue |
|--------|----------|-------|
| ORIGAMI | P1 | Per-sample `std::cos`/`std::sin` for voice panning |
| ORIGAMI | P2 | Per-sample `postFilter.setCoefficients()` |
| OCEANDEEP | P1 | `kMaxDelay=48001` → wrong pitch at >48kHz SR for sub-bass |
| OCEANDEEP | P2 | Per-sample trig in DarkFilter when LFO active |
| OCTOPUS | P1 | Per-sample `std::exp` for chromatophore coefficient |
| FAT | P1 | `std::tan()` per voice per sample in ZDF ladder |
| DRIFT | P1 | Per-sample `std::sin` in `DriftTidalPulse` |
| ORACLE | — | CLEAN |
| OBSCURA | — | CLEAN (192kHz physics behavioral note) |
| ONSET | — | CLEAN (exemplary hoisting pattern) |
| Fleet | low | `ScopedNoDenormals` missing from ~59/73 engines |

### Recommended Fix Order
1. **OCTOPUS P1** — trivial 1-line precompute, highest payoff per effort
2. **ORIGAMI P1** — 8-line array precompute at block start
3. **ORIGAMI P2** — hoist `postFilter.setCoefficients()` to block start
4. **OCEANDEEP P1** — scale `kMaxDelay` in `prepare()` (required for accurate >48kHz bass)
5. **DRIFT P1** — replace `std::sin` with `fastSin` or 4096-entry LUT
6. **OCEANDEEP P2** — add threshold guard to DarkFilter cache check
7. **FAT P1** — evaluate Padé approximation or block-rate smoothed cutoff
8. **Fleet** — batch-add `ScopedNoDenormals` to all 73 engines

---

*Round 7 complete. Engines audited this session: Origami, Oracle, Obscura, OceanDeep, Octopus, Fat, Drift, Onset.*
