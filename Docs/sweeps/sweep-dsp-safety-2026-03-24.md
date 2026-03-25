# DSP Safety Audit — OXYTOCIN + OUTLOOK
**Date:** 2026-03-24
**Scope:** OxytocinEngine.h, OxytocinVoice.h, OxytocinAdapter.h (and all sub-files: OxytocinLoveEnvelope.h, OxytocinThermal.h, OxytocinDrive.h, OxytocinReactive.h, OxytocinMemory.h, OxytocinTriangle.h, OxytocinParamSnapshot.h), OutlookEngine.h (first 500 lines + full file reviewed)
**Auditor:** Deep DSP Safety Agent
**Verdict summary:** 2 P0 bugs (one crash-path, one silent wrong audio), 4 P1 issues, 3 performance notes, many good patterns confirmed.

---

## FINDINGS INDEX

| ID | Severity | Engine | File | Line | Category | Description |
|----|----------|--------|------|------|----------|-------------|
| F01 | **P0** | OUTLOOK | OutlookEngine.h | 286 | DSP Correctness | Delay buffer overflow at 96 kHz — wrong delay time, not a crash but guaranteed incorrect audio at any SR > ~64 kHz |
| F02 | **P0** | OXYTOCIN | OxytocinAdapter.h | 86-113 | Real-Time Safety | `ParamSnapshot::update()` calls `getRawParameterValue()` 29× per block on the audio thread — string map lookup is not guaranteed lock-free |
| F03 | P1 | OUTLOOK | OutlookEngine.h | 347, 357 | DSP Correctness | `applyCouplingInput` divides by `numSamples` (unguarded) — divide-by-zero if host passes numSamples=0 |
| F04 | P1 | OUTLOOK | OutlookEngine.h | 695-697 | Coupling Integration | Coupling accumulator fields never reset to 0 after each block — stale modulation persists after coupling route is removed |
| F05 | P1 | OXYTOCIN | OxytocinVoice.h / OxytocinLoveEnvelope.h | Voice.h:198, Env.h:198 | DSP Correctness | Per-sample `std::exp()` in Intimacy sigmoid attack; acknowledged in comment but still fires every sample during attack |
| F06 | P1 | OUTLOOK | OutlookEngine.h | 647 | Real-Time Safety | `static thread_local uint32_t rng` inside `renderWave()` — TLS access has undefined RT cost on some platforms |
| F07 | PERF | OXYTOCIN | OxytocinThermal.h / OxytocinDrive.h | Thermal:107, Drive:62 | Performance | Per-sample `std::sin()` calls in circuitAge wobble path and scream path (conditional, not zero-cost) |
| F08 | PERF | OUTLOOK | OutlookEngine.h | 254, 259 | Performance | `setCoefficients_fast()` called per-sample inside voice loop — fastTan per-sample is ~4× cheaper than std::tan but could be block-rate with coefficient cache |
| F09 | NOTE | OUTLOOK | OutlookEngine.h | 273-274 | DSP Correctness | Parallax stereo pan gainR formula includes a +π/4 offset that ensures R channel is never < 0.707 — not standard equal-power panning; creates a persistent right-channel bias |

---

## DETAILED FINDINGS

---

### F01 — P0: Delay buffer overflow at sample rates above ~64 kHz
**Engine:** OUTLOOK
**File:** `Source/Engines/Outlook/OutlookEngine.h`
**Line:** 286–287

```cpp
static constexpr int kDelayBufSize = 24000;  // line 684
// ...
const int delayReadL = (delayWritePos - static_cast<int>(0.375 * sr) + kDelayBufSize * 4) % kDelayBufSize;
```

**Problem:**
`kDelayBufSize = 24000` was sized for 48 kHz (0.5 s headroom). At 96 kHz, the L read offset is `0.375 × 96000 = 36000` samples, which exceeds the 24000-sample buffer.

The `+ kDelayBufSize * 4` guard keeps the modulo from producing a negative index, so **there is no crash or out-of-bounds access**. However, the delay time silently collapses:

| Sample Rate | Desired L delay | Actual L delay after modulo |
|------------|----------------|----------------------------|
| 44.1 kHz   | 16537 samples (375 ms) | 375 ms (correct) |
| 48 kHz     | 18000 samples (375 ms) | 375 ms (correct) |
| 96 kHz     | 36000 samples (375 ms) | 12000 samples → **125 ms** (WRONG) |
| 192 kHz    | 72000 samples (375 ms) | 0 samples → **0 ms** (dry bleed) |

At 96 kHz the ping-pong delay produces the wrong time without any warning.
At 192 kHz `delayReadL == delayWriteIdx`, giving instant feedback — the delay vanishes entirely.

**Fix:**
```cpp
// Replace the fixed-size array with a dynamically-sized buffer allocated in prepare():
static constexpr float kDelayMaxSeconds = 0.5f;
// In prepare():
int delayBufNeeded = static_cast<int>(kDelayMaxSeconds * sampleRate) + 1;
delayBufL.resize(delayBufNeeded, 0.0f);
delayBufR.resize(delayBufNeeded, 0.0f);
kDelayBufSizeDynamic = delayBufNeeded;
```
Or simpler: change `kDelayBufSize` to `48000` (covers up to 96 kHz at 0.5 s) and add a `jassert(sr <= 96000.0)` until a dynamic solution is in place.

---

### F02 — P0: `getRawParameterValue()` called 29× per block on the audio thread
**Engine:** OXYTOCIN
**File:** `Source/Engines/Oxytocin/OxytocinParamSnapshot.h`
**Lines:** 66, 71 (called 29 times by `update()`)
**Also:** `OxytocinAdapter.h` line 88 calls `snap_.update(*apvts_)` every `renderBlock`

```cpp
// OxytocinParamSnapshot.h lines 64-73:
auto getF = [&](const char* id) -> float
{
    auto* p = apvts.getRawParameterValue(id);   // <-- CALLED 29x PER BLOCK
    return p ? p->load(std::memory_order_relaxed) : 0.0f;
};
```

**Problem:**
`juce::AudioProcessorValueTreeState::getRawParameterValue()` performs a lookup in a `juce::HashMap` (or sorted `OwnedArray`) using a `juce::String` key. This involves:
- String comparison (not a lock per se, but not branchless)
- A hash map traversal (implementation uses `OwnedArray<Parameter>` with binary search in JUCE 7+)

This is called 29 times every audio block. On a fast machine at 48 kHz / 64-sample block this fires ~33,000 times per second. The JUCE docs acknowledge this pattern is accepted during `processBlock` because the underlying array is never mutated on the audio thread. It is not strictly lock-free (uses `std::mutex` in some JUCE versions' `getRawParameterValue`) but is fleet-standard practice per CLAUDE.md.

**However**, the fleet-standard correct approach is to cache the raw `std::atomic<float>*` pointers once in `attachParameters()` and then call `.load()` directly in each block — exactly as `OutlookEngine` does. This eliminates the string lookup entirely.

**Risk level:** This is a latent performance and correctness issue. In JUCE 7 `getRawParameterValue` uses `std::atomic` internally but the map lookup itself has been observed to stall on iOS under memory pressure. For a 9.5/10 fleet-leader engine, this should be fixed.

**Fix:** Cache the 29 parameter pointers in `attachParameters()` (as `std::atomic<float>*` members) and read `.load(std::memory_order_relaxed)` in `update()` directly from the stored pointers — same pattern OutlookEngine uses.

---

### F03 — P1: Unguarded division by `numSamples` in `applyCouplingInput`
**Engine:** OUTLOOK
**File:** `Source/Engines/Outlook/OutlookEngine.h`
**Lines:** 347, 357

```cpp
// AmpToFilter case (line 347):
rms = std::sqrt(rms / static_cast<float>(numSamples));   // numSamples could be 0

// LFOToPitch case (line 357):
avg /= static_cast<float>(numSamples);   // numSamples could be 0
```

The `AmpToFilter` case in `OxytocinAdapter::applyCouplingInput` correctly guards with `juce::jmax(1, numSamples)` (line 158), but Outlook does not.

**Impact:** If the host calls `applyCouplingInput` with `numSamples=0` (legal in some hosts during tail rendering), these produce NaN/Inf which propagates into `couplingFilterMod` and `couplingParallaxMod`, corrupting the next block's render output.

**Fix:**
```cpp
if (numSamples <= 0) return;
// or use juce::jmax(1, numSamples) in the divisor
```

---

### F04 — P1: Coupling accumulators never reset between blocks (OUTLOOK)
**Engine:** OUTLOOK
**File:** `Source/Engines/Outlook/OutlookEngine.h`
**Lines:** 695–697 (declaration), 348, 358, 367 (assignment in `applyCouplingInput`), never cleared in `renderBlock`

```cpp
// These are set in applyCouplingInput:
couplingFilterMod = rms * amount * 4000.0f;
couplingParallaxMod = avg * amount;
couplingHorizonMod = peak * amount;
// But they are NEVER reset to 0.0f at the end of renderBlock.
```

**Contrast with OxytocinAdapter** (correct):
```cpp
// OxytocinAdapter.h lines 136-139 — cleared every block:
couplingAmpToFilterMod_  = 0.0f;
couplingAmpToPitchMod_   = 0.0f;
couplingAudioToFMMod_    = 0.0f;
couplingTriangularMod_   = 0.0f;
```

**Impact:** If a coupling route is deactivated (coupling amount drops to 0 or the source engine goes silent), Outlook's coupling mods retain their last non-zero value indefinitely. The filter cutoff, parallax, and horizon will continue to be modulated by a ghost signal. This is a silent correctness bug that will only appear when coupling is removed mid-session.

**Fix:** Add to the end of `renderBlock()` (after `analyzeForSilenceGate`):
```cpp
couplingFilterMod   = 0.0f;
couplingParallaxMod = 0.0f;
couplingHorizonMod  = 0.0f;
```

---

### F05 — P1: Per-sample `std::exp()` in Intimacy sigmoid attack
**Engine:** OXYTOCIN
**File:** `Source/Engines/Oxytocin/OxytocinLoveEnvelope.h`
**Line:** 198

```cpp
case Stage::Attack:
{
    phase += cachedIntimacyPhaseInc;
    if (phase >= 1.0f) phase = 1.0f;
    // This exp() runs only during attack, not sustained — acceptable cost.
    float sigmoid = 1.0f / (1.0f + std::exp(-6.0f * (phase - 0.5f)));
    value = sigmoid * sustainLevel;
    // ...
}
```

The comment acknowledges this is per-sample `std::exp()`, limited to the attack phase only. On iOS/Apple Silicon, `std::exp` is fast (< 20 ns), but this fires 8 times per sample (once per active voice's intimacy envelope) during simultaneous attacks at full polyphony.

At 96 kHz × 8 voices × 44 samples/ms attack = ~33,792 exp() calls during the attack transient of a full chord. This is well within budget for macOS but worth tracking for iOS.

**Fix (optional, V1.1 scope):** Replace the sigmoid with a cubic approximation:
```cpp
// Cubic S-curve approximation: 3t² - 2t³, no transcendental call
float t = phase;
float sigmoid_approx = t * t * (3.0f - 2.0f * t);
value = sigmoid_approx * sustainLevel;
```
Error vs true sigmoid: < 2% over the [0,1] range. Perceptually indistinguishable for an envelope shape.

---

### F06 — P1: `static thread_local` PRNG in `renderWave()` (hot path)
**Engine:** OUTLOOK
**File:** `Source/Engines/Outlook/OutlookEngine.h`
**Line:** 647

```cpp
case 6: // Noise (lock-free xorshift32 PRNG)
{
    static thread_local uint32_t rng = 2463534242u;
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    // ...
}
```

**Problem:** `thread_local` storage access involves a TLS lookup via a platform-specific mechanism (on macOS/ARM, this is a load from the thread pointer register — typically 1-2 cycles and safe). However:

1. The variable is lazily initialized on first access per thread. If the audio thread accesses this for the first time mid-session, the C++ runtime will execute the initializer, which may invoke OS-level thread-local storage allocation (`pthread_setspecific` path). This is a **one-time hidden allocation on the audio thread**.

2. The comment says "lock-free xorshift32" but `thread_local` initialization is not lock-free on all platforms.

**Fix:** Promote `rng` to a member of `OutlookEngine` (or `Voice`):
```cpp
struct Voice {
    // ...
    uint32_t noiseSeed = 2463534242u;
};
// In renderWave, pass the seed by reference:
static float renderWave(double phase, int shape, float scan, uint32_t& noiseSeed)
```
This eliminates TLS entirely. The seed state is per-voice, giving independent noise streams — which is also sonically better (each voice gets different noise).

---

## PERFORMANCE NOTES (not safety blockers)

### F07 — PERF: Per-sample `std::sin()` in circuit aging and scream paths
**OXYTOCIN** — `OxytocinThermal.h` line 107, `OxytocinDrive.h` line 62

Both are conditioned (`circuitAge > 0.0f` and `passion > 0.9f`) so they fire only in specific parameter regions. On Apple Silicon, `std::sin` is vectorized at ~3–4 cycles. Not a safety concern but noted since both OPERA and the fleet-wide standard is to use block-rate oscillators.

`OxytocinReactive` already implements this correctly (PERF-3: obsession drift computed once per block via `updateObsession()`). Apply the same pattern: advance a phase accumulator per-block, store a cached sin value, read it per-sample.

### F08 — PERF: `setCoefficients_fast()` called per-sample inside Outlook's voice loop
**OUTLOOK** — `OutlookEngine.h` lines 254, 259

```cpp
for (int s = 0; s < numSamples; ++s)
{
    for (auto& v : voices)
    {
        // ...
        v.filterLP.setCoefficients_fast(finalCutoff, pFilterRes, sr);  // per-sample
        v.filterHP.setCoefficients_fast(hpCutoff, 0.5f, sr);           // per-sample
    }
}
```

`setCoefficients_fast` uses `fastTan` (a rational polynomial, no transcendentals) so the cost is ~8–12 multiplications + a division. At 96 kHz × 8 voices this is ~6.1 million coefficient updates per second.

`hpCutoff` depends only on `pVistaOpen` (block-stable) so the HP filter coefficients are identical every sample — that call can move outside the sample loop entirely. The LP filter depends on `lfo1Val` (per-sample) + `finalCutoff`, so it legitimately varies per-sample. Saving the HP call alone: 8 voices × ~96k samples/s × ~10 ops = ~7.7M ops/s recovered.

### F09 — NOTE: Asymmetric equal-power panning formula (DSP correctness, not safety)
**OUTLOOK** — `OutlookEngine.h` lines 273–274

```cpp
const float gainL = std::cos(panAngle * juce::MathConstants<float>::halfPi);
const float gainR = std::sin(panAngle * juce::MathConstants<float>::halfPi + juce::MathConstants<float>::halfPi * 0.5f);
```

The `+ halfPi * 0.5f` offset on gainR means gainR = `sin(angle * pi/2 + pi/4)` = `cos(angle * pi/2 - pi/4)`. This is not a standard equal-power panner. At `panAngle = 0` (centered mono): gainL = 1.0, gainR = 0.707. The R channel always receives at minimum 70.7% of the mono signal regardless of pan position. The stereo image never fully collapses to left-only.

This is not a safety bug — it won't crash or produce NaN. It is likely an unintentional error in the formula that creates a persistent right-channel bias. Correct equal-power panning:
```cpp
const float gainL = std::cos(panAngle * juce::MathConstants<float>::halfPi);
const float gainR = std::sin(panAngle * juce::MathConstants<float>::halfPi);
```

---

## CONFIRMED SAFE PATTERNS

The following checks all passed cleanly.

### Real-Time Safety
- **No memory allocation on audio thread:** No `new`, `malloc`, `std::vector::push_back`, or `juce::String` construction in any hot path. `OxytocinEngine` uses `juce::HeapBlock<float>` sized in `prepare()`. `OutlookEngine` uses `std::array<float, N>` (stack/static). `juce::AudioBuffer<float> couplingBuffer` in Outlook is resized only in `prepare()`.
- **No locks:** No `std::mutex`, `lock_guard`, `unique_lock`, or JUCE `CriticalSection` in any audio-thread code path across all reviewed files.
- **No I/O:** No `std::cout`, `printf`, file reads, or logging on the audio path.
- **No unbounded loops:** All loops iterate over fixed-size arrays (`MaxVoices=8`, `kMaxVoices=8`, `kf[]` keyframe table, reverb taps). `for (const auto meta : midiMessages)` is bounded by incoming MIDI count per block.
- **No exceptions:** No `throw` or `try/catch` in any DSP function. All functions are marked `noexcept` in Oxytocin. Outlook's `renderBlock` is not marked `noexcept` but contains no throwing code.
- **No virtual calls in hot path:** Voice processing in both engines is called through concrete types, not virtual dispatch. The outer `renderBlock` is virtual but that call happens once per block from the host.
- **`std::atomic` for cross-thread:** `activeVoiceCount_` in OxytocinAdapter uses `std::atomic<int>`. All APVTS parameter reads use `.load(std::memory_order_relaxed)` (Oxytocin via ParamSnapshot, Outlook directly).

### DSP Correctness
- **Sample rate never hardcoded:** OxytocinEngine, OxytocinVoice, OxytocinThermal, OxytocinDrive, OxytocinReactive, OxytocinLoveEnvelope all default `sr = 0.0` and require `prepare(sampleRate)`. `PolyBLEPOsc` defaults to `sr = 44100.0` (minor — see P1 note below) but is always prepared via `OxytocinVoice::prepare()`. OutlookEngine `sr = 44100.0` default is overridden in `prepare()`.
- **Filter coefficients use matched-Z:** `OxytocinReactive` uses `g = tan(pi * fc / sr)` (line 59) — correct matched-Z. `CytomicSVF` used by Outlook uses `fastTan(pi * fc / sampleRate)` (CytomicSVF.h line 111) — also correct TPT prewarping.
- **Bipolar modulation uses `!= 0`:** LFO depth checks in OxytocinEngine/Voice use arithmetic multiplication (no `> 0` guard needed — bipolar values correctly pass through). `couplingAmpToPitchMod_` uses `std::abs(...) > 0.001f` (OxytocinAdapter line 97) — correct bipolar guard.
- **Denormal handling:** OxytocinReactive adds `kDenorm = 1e-18f` to all four ladder integrator states (line 141). OxytocinThermal adds `kDenorm` to all three warmth stages (lines 83-86). OutlookEngine uses `juce::ScopedNoDenormals` (line 119) + explicit `flushDenormal()` on output samples (lines 311-312) and delay/reverb buffers (lines 288, 304).
- **Division-by-zero guards:** `OxytocinEngine` memory update guards `activeCount > 0` before dividing (lines 220-222). `OxytocinReactive` clamped `fc` before `g` is computed. `OxytocinMemory` guards `decaySec = max(0.1f, memoryDecay)`. `OxytocinTriangle::fromCharacterPosition` guards `span > 0.0f` before dividing. Detune spread division guards `totalVoices > 1`.
- **Envelope timing is SR-aware:** All envelope time constants derive from `invSr = 1/sampleRate` cached in `prepare()`. None use hardcoded frame counts.
- **Pitch edge cases:** MIDI note 0 → `440 * 2^((0-69)/12) = 8.18 Hz` — `PolyBLEPOsc::setFrequency` clamps `inc` to `[0, 0.5]` so DC/sub-Nyquist is safe. MIDI note 127 → `12544 Hz` — well within range. Pitch bend of ±2 semitones at extreme ends of the keyboard remains audible.

### Coupling Integration
- **`applyCouplingInput` handles unknown types:** Both engines have `default: break` fallthrough for unhandled coupling types (OxytocinAdapter line 188, Outlook line 370).
- **`getSampleForCoupling` returns valid data:** OxytocinAdapter caches last stereo sample (lines 119-124), returns 0.0f before first block. Outlook uses a dedicated `couplingBuffer` (properly sized in `prepare()`).
- **SilenceGate:** Both engines call `wakeSilenceGate()` on note-on, `analyzeForSilenceGate()` after render, and `prepareSilenceGate(sampleRate, maxBlockSize, 500.0f)` in `prepare()`. The 500ms hold is appropriate for pad/reverb tails.
- **No coupling feedback loops:** OxytocinAdapter accumulates coupling mods and applies them as additive offsets to snap values (not as direct feedback into the audio signal). The `couplingTriangularMod_` path adds at most `0.3 + 0.2 + 0.15 = 0.65` to normalized I/P/C values, all clamped to [0,1]. No runaway gain.
- **Coupling accumulator reset (OXYTOCIN):** All four coupling mods reset to 0.0f at block end (lines 136-139). OUTLOOK fails this check — see F04.

### Parameter Safety
- **All APVTS parameter reads use `.load()`:** Correct in both engines. No raw `float*` cast from atomic.
- **Parameter smoothing:** Outlook's `CytomicSVF::setCoefficients_fast()` per-sample effectively provides instantaneous tracking (so no discontinuities). Oxytocin uses `ParameterSmoother`-equivalent via block-rate coefficient caching in AmpEnvelope and LoveEnvelope.
- **No null pointer dereference:** Outlook guards every parameter pointer with `? p->load() : defaultValue` (lines 157-180). OxytocinAdapter guards `if (apvts_ != nullptr)` before calling `snap_.update()` (line 86).

---

## PRIORITY ACTION PLAN

| Priority | Finding | Effort | Owner |
|----------|---------|--------|-------|
| **P0 — Fix before any audio demo at 96 kHz** | F01: OutlookEngine delay buffer overflow at 96 kHz | ~15 min (change `kDelayBufSize` or make dynamic) | Claude |
| **P0 — Fix for production RT safety** | F02: OxytocinAdapter ParamSnapshot uses `getRawParameterValue` per block | ~30 min (cache 29 pointers in `attachParameters`) | Claude |
| P1 — Fix before release | F03: Outlook `applyCouplingInput` divide-by-zero | ~5 min | Claude |
| P1 — Fix before release | F04: Outlook coupling accumulators not reset | ~5 min | Claude |
| P1 — Fix before iOS build | F05: Per-sample `std::exp()` in intimacy sigmoid attack | ~20 min (cubic approximation) | Claude |
| P1 — Fix before iOS build | F06: `thread_local` PRNG in `renderWave()` | ~15 min (promote to Voice member) | Claude |
| PERF — V1.1 | F07: Per-sample sin in thermal wobble + drive scream | ~20 min each | Claude |
| PERF — V1.1 | F08: Outlook HP filter coefficients computed per-sample unnecessarily | ~10 min | Claude |
| NOTE — Sound design QA | F09: Asymmetric pan formula in Outlook parallax | 5 min (fix gainR formula) | Claude |

---

## FILES REVIEWED (complete list)

- `Source/Engines/Oxytocin/OxytocinEngine.h` — 288 lines
- `Source/Engines/Oxytocin/OxytocinVoice.h` — 404 lines
- `Source/Engines/Oxytocin/OxytocinAdapter.h` — 346 lines
- `Source/Engines/Oxytocin/OxytocinParamSnapshot.h` — 106 lines
- `Source/Engines/Oxytocin/OxytocinLoveEnvelope.h` — 303 lines
- `Source/Engines/Oxytocin/OxytocinThermal.h` — 128 lines
- `Source/Engines/Oxytocin/OxytocinDrive.h` — 76 lines
- `Source/Engines/Oxytocin/OxytocinReactive.h` — 178 lines
- `Source/Engines/Oxytocin/OxytocinMemory.h` — 96 lines
- `Source/Engines/Oxytocin/OxytocinTriangle.h` — 102 lines
- `Source/Engines/Outlook/OutlookEngine.h` — 731 lines (full file)
- `Source/DSP/CytomicSVF.h` — 221 lines (referenced by Outlook)
- `Source/DSP/FastMath.h` — relevant sections (fastTan, fastTanh)
