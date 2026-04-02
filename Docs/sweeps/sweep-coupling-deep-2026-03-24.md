# Deep Audit: MegaCouplingMatrix + CouplingPresetManager
**Date:** 2026-03-24
**Files audited:**
- `Source/Core/MegaCouplingMatrix.h` (438 lines)
- `Source/Core/CouplingPresetManager.h` (589 lines)
- `Source/Core/SynthEngine.h` (enum + interface)
- `Source/Core/PresetManager.h` (`validCouplingTypes`, `resolveEngineAlias`)
- `Source/Core/CouplingCrossfader.h` (`needsCrossfade`, `getCrossfadeDuration`)
- `Source/Engines/Oxytocin/OxytocinAdapter.h` (TriangularCoupling implementation)

---

## 1. Logic Correctness

### 1.1 CouplingType enum — 15 values, coverage audit

The enum in `SynthEngine.h` lines 11–47 defines these 15 values (0-indexed):

| Index | Name |
|-------|------|
| 0 | AmpToFilter |
| 1 | AmpToPitch |
| 2 | LFOToPitch |
| 3 | EnvToMorph |
| 4 | AudioToFM |
| 5 | AudioToRing |
| 6 | FilterToFilter |
| 7 | AmpToChoke |
| 8 | RhythmToBlend |
| 9 | EnvToDecay |
| 10 | PitchToPitch |
| 11 | AudioToWavetable |
| 12 | AudioToBuffer |
| 13 | KnotTopology |
| 14 | TriangularCoupling |

**Coverage in `processBlock()` (MegaCouplingMatrix.h lines 118–191):**

The dispatch tree is:
1. KnotTopology → `processKnotRoute()` (line 149)
2. AudioToBuffer → `processAudioRoute()` (line 164)
3. AudioToFM / AudioToRing / AudioToWavetable → mono mixdown loop (line 170)
4. Everything else → `fillControlRateBuffer()` (line 185)

**TriangularCoupling (#14) is NOT explicitly dispatched.** It falls through to the `else` branch at line 185 and goes into `fillControlRateBuffer()`, which is correct behaviorally — it will be passed to `dest->applyCouplingInput()` with control-rate decimation. This is the right treatment: TriangularCoupling carries I/P/C state values, not raw audio, so control-rate is appropriate. The behavior is correct but undocumented in the dispatch block — a reader has to infer it.

**Recommendation (non-critical):** Add a brief comment above the `else` branch noting that TriangularCoupling and any future non-audio modulation types are handled here by design.

---

### 1.2 TriangularCoupling — stub or full implementation?

**Full implementation, not a stub.**

- `OxytocinAdapter.h` lines 182–186 handle `CouplingType::TriangularCoupling` in its `applyCouplingInput()` switch, accumulating into `couplingTriangularMod_`.
- `renderBlock()` (lines 107–112) consumes `couplingTriangularMod_` to modulate `snap_.intimacy`, `snap_.passion`, and `snap_.commitment`.
- The bandA/B/C/D encoding (I/P/C + memory average) is documented in the adapter header but only `bandA` (via RMS) is transmitted — the source buffer carries the RMS of the source block, not per-band values. This means all four conceptual bands reduce to a single scalar. This is a design simplification, not a bug, but it means TriangularCoupling as currently implemented does not differentiate between intimacy, passion, and commitment from the source — it uses combined RMS and distributes it across the destination's three components at fixed ratios (0.3 / 0.2 / 0.15). The comment on line 10–14 of `OxytocinAdapter.h` describing bandA–bandD is aspirational documentation, not the current behavior.

**Finding (LOW — design discrepancy):** The bandA/B/C/D encoding described in the header (`SynthEngine.h` lines 39–44 and `OxytocinAdapter.h` lines 11–14) is not implemented. The `sourceBuffer` carries a single time-domain signal; there is no mechanism to pack per-band values. The behavior is musically reasonable but the documentation is misleading. Either remove the bandA-D spec from the comments or implement structured multi-band packing.

---

### 1.3 Coupling strength clamping — can amounts go negative or above 1?

**In MegaCouplingMatrix itself:** No explicit clamp. The `CouplingRoute.amount` field is typed `float` with a comment `// 0.0 to 1.0` but is never clamped by `addRoute()` or `processBlock()`. The only guard is the zero-skip at line 126: `route.amount < 0.001f`.

**Consequence if amount > 1.0:** The scaled modulation delivered to `dest->applyCouplingInput()` will be > 1.0. Each engine's `applyCouplingInput()` is responsible for its own clamping. OxytocinAdapter accumulates `rms * amount` into e.g. `couplingAmpToFilterMod_` and clamps it in `renderBlock()` (lines 93–94: `std::clamp(snap_.cutoff + mod * 4000.0f, 20.0f, 20000.0f)`). This is the correct defense-in-depth pattern.

**Consequence if amount < 0 (negative):** `processBlock()` line 126 skips routes where `route.amount < 0.001f`, which means **negative amounts are silently swallowed**. The APVTS cp_* `amount` parameter is bipolar (-1.0 to 1.0 per `BakedCouplingState::Route.amount`), but by the time a route reaches `MegaCouplingMatrix`, negative amounts are no-ops.

**Finding (MEDIUM — silent failure of bipolar coupling):** The coupling performance overlay stores bipolar amounts (-1.0 to 1.0) but `processBlock()` line 126 short-circuits all negative amounts before they reach any engine. This means downward modulation (e.g. source amplitude *lowering* destination cutoff) is impossible through the performance overlay path. The CLAUDE.md note about `!= 0` vs `> 0` for bipolar modulation applies here. The fix is to change line 126 from `route.amount < 0.001f` to `std::abs(route.amount) < 0.001f`. All engines that handle bipolar coupling (AmpToFilter, AmpToPitch, etc.) need to accept and apply negative amounts.

**File:** `MegaCouplingMatrix.h` line 126
**Current:** `if (!route.active || route.amount < 0.001f)`
**Fix:** `if (!route.active || std::abs(route.amount) < 0.001f)`

---

### 1.4 What happens with 4 engines all coupled to each other (6 bidirectional pairs)?

With 4 engines and routes for all 6 pairs, the route list could have up to 12 entries (6 pairs × 2 directions, or 6 KnotTopology routes). `MaxRoutes = 64` so no overflow concern.

**Performance with 6 KnotTopology routes:** Each KnotTopology route calls `fillControlRateBuffer()` twice (once for each direction). Each `fillControlRateBuffer()` call makes one `getSampleForCoupling()` call per control-rate block (every 32 samples), plus writes `numSamples` floats into `couplingBuffer`. For a 128-sample block, that is 4 calls to `getSampleForCoupling()` and 256 float writes per route. Six Knot routes = 48 source reads and ~1536 float writes per block. This is negligible.

**No stack allocation, no allocation on audio thread.** `couplingBuffer` and `couplingBufferR` are pre-allocated in `prepare()`. No concern.

**Mutual coupling convergence / feedback:** With 4 engines each modulating the others simultaneously via KnotTopology, the coupling inputs from the *current block* are based on outputs from the *previous block* (since `getSampleForCoupling()` returns cached values from the last `renderBlock()`). This is a one-block latency which prevents true per-sample feedback loops. No risk of unbounded runaway within a single block.

---

### 1.5 Self-coupling (engine A coupled to itself)

- `processAudioRoute()` line 339: explicit `if (route.sourceSlot == route.destSlot) return;` — correct.
- `processKnotRoute()` line 411: same guard — correct.
- Standard routes (non-Knot, non-AudioToBuffer): **no self-route guard**. If sourceSlot == destSlot, the source's output is read into `couplingBuffer` and then `applyCouplingInput()` is called on the same engine. This creates self-modulation. Whether this is intentional or a bug depends on the use case. Since `notifyCouplingMatrixOfSwap()` comment (line 207) says "every SynthEngine must handle unknown coupling types gracefully," self-modulation of control-rate types (AmpToFilter, LFOToPitch) is presumably safe — it just creates an interesting self-feedback loop. No crash risk.

**Finding (LOW — undocumented behavior):** Self-routes for non-audio, non-Knot coupling types are silently permitted and execute. This may be intentional but is not documented. Consider adding a self-route guard or a comment explaining why self-modulation is allowed for control-rate types.

---

## 2. Thread Safety

### 2.1 Route mutation vs. audio rendering

The double-buffer pattern using `std::shared_ptr<std::vector<CouplingRoute>>` with `std::atomic_store` / `std::atomic_load` is correct for a single-writer (message thread) / single-reader (audio thread) scenario.

**Correctness of `atomic_load` / `atomic_store` with shared_ptr:** In C++11/14, `std::atomic_load` and `std::atomic_store` for `shared_ptr` are defined as specializations. They use an internal lock (not lock-free) but this lock is in the shared_ptr runtime, not on the audio thread's critical path — the audio thread only calls `std::atomic_load` once per callback (via `loadRoutes()` at line 110) and then holds the result as a local reference. The lock is only held for the duration of the atomic load, which is nanoseconds. This is acceptable practice.

**IMPORTANT — C++20 deprecation:** `std::atomic_load(shared_ptr*)` and `std::atomic_store(shared_ptr*)` are **deprecated in C++20** and removed in C++23. If the project ever targets C++20 or later, these need to be replaced with `std::atomic<std::shared_ptr<T>>`. This is not a current bug but is a technical debt item.

**Finding (MEDIUM — forward compatibility):** `std::atomic_load` / `std::atomic_store` for `shared_ptr` are used in lines 70, 75, 78, 83, 102, 112, 195, 221, 251. These are deprecated in C++20. Migrate to `std::atomic<std::shared_ptr<std::vector<CouplingRoute>>>` declared as a member when C++20 is adopted.

### 2.2 Race conditions between coupling parameter changes and audio rendering

`setEngines()` (line 54) assigns `activeEngines` directly without any synchronization. This is called from the message thread while `processBlock()` reads `activeEngines` on the audio thread. Since `activeEngines` is a `std::array<SynthEngine*, 4>` (not atomic), concurrent writes and reads are a data race.

**Finding (HIGH — data race on `activeEngines`):** `setEngines()` at line 54 is called from the message thread but `activeEngines` is read from the audio thread in `processBlock()`. There is no synchronization. This is undefined behavior per the C++ memory model. The fix is either:
  - Make `activeEngines` an `std::array<std::atomic<SynthEngine*>, MaxSlots>`, or
  - Use the same double-buffer / atomic-swap pattern as `routeList`, or
  - Ensure `setEngines()` is only called during `prepareToPlay()` before audio starts.

The last option is the simplest if the contract is that engines are always swapped during a non-running state. But if `loadEngine()` (hot-swap) can be called while audio is running, this is a live data race.

### 2.3 kControlRateRatio downsampling

`kControlRateRatio = 32` (line 259). For a 128-sample block at 48kHz, this produces 4 control points per block = 1.5kHz effective control rate. Adequate for slowly-varying modulation sources (LFO, envelope, amplitude). The linear interpolation in `fillControlRateBuffer()` (lines 296–299) ensures no audible stepping artifacts.

No correctness issues found in the decimation logic.

---

## 3. Edge Cases

### 3.1 Zero engines loaded

`processBlock()` checks `source` and `dest` for null at line 136. If all engine slots are null, every route skips. Safe, no crash.

### 3.2 One engine loaded

Same null check covers this. Routes involving the null slot are skipped. The one active engine processes normally. Safe.

### 3.3 All 4 engines loaded

Normal operation. No special cases needed.

### 3.4 Engine unloaded while coupled

`notifyCouplingMatrixOfSwap()` (line 219) handles this for `AudioToBuffer` routes only, marking them inactive when the destination is not OpalEngine. For all other coupling types, routes remain active and will resolve at runtime via the null check at line 136 (if the engine pointer is null) or via `applyCouplingInput()` (if the new engine doesn't recognize the coupling type and falls to `default: break`).

**Finding (LOW — inconsistent hot-swap handling):** The orphan-route logic only applies to `AudioToBuffer` → OPAL combinations. If a non-OPAL engine is unloaded and its slot is either null or replaced, routes pointing at it either silently no-op (null check) or deliver coupling to the replacement engine unintentionally. This is documented as intentional ("different but never crash-inducing") but worth reviewing for the performance system, where users may have carefully constructed coupling presets.

### 3.5 Coupling strength = 0

`route.amount < 0.001f` check at line 126 skips processing entirely. This is the intended optimization — no processing, no buffer fills. Correct.

### 3.6 prepare() never called

If `prepare()` is never called, `couplingBuffer` and `couplingBufferR` remain empty vectors (size 0). `fillControlRateBuffer()` will pass `numSamples` but `couplingBuffer.size()` will be 0, so `limit` at line 144 will be 0. The fill loops won't execute. `dest->applyCouplingInput()` will receive a null-like 0-length buffer and `numSamples = 0`. This is safe for well-written engines (most check `numSamples > 0`).

**However:** `applyCouplingInput()` is called with `couplingBuffer.data()` (which is a valid but zero-size allocation pointer) and `numSamples` (the *original* unclamped count, not `limit`). This means engines receive a stale `numSamples` that doesn't match the actual data in the buffer. See Bug 3.7 below.

### 3.7 `limit` vs `numSamples` mismatch in `applyCouplingInput()` call

**Finding (MEDIUM — buffer size / numSamples mismatch):** In `processBlock()` line 144, `limit = juce::jmin(numSamples, static_cast<int>(couplingBuffer.size()))`. But at line 188–189, the call is:

```cpp
dest->applyCouplingInput(route.type, route.amount,
                         couplingBuffer.data(), numSamples);  // ← numSamples, not limit
```

If `limit < numSamples` (which happens when `couplingBuffer.size() < numSamples`, i.e., `prepare()` was called with a `maxBlockSize` smaller than the current block), the engine receives a `numSamples` count that is larger than the actual populated region of `couplingBuffer`. The engine will read uninitialized or zeroed memory for samples beyond index `limit-1`.

The same mismatch applies inside `fillControlRateBuffer()` and the audio-rate loop (lines 173–176). Those write up to `limit` samples but `applyCouplingInput()` is told there are `numSamples` samples.

**Fix:** Change line 189 from `numSamples` to `limit`. Also verify `processKnotRoute()` — it passes `numSamples` as `limit` parameter to `processKnotRoute()` already (line 151), so the outer limit is already applied there.

---

## 4. Performance

### 4.1 Unnecessary work when coupling is disabled

The `!route.active` check at line 126 exits early. If no routes are active, `processBlock()` iterates the route list but does zero DSP work. With an empty route list, the early-exit `routes->empty()` check at line 121 prevents even that iteration. Good.

### 4.2 Per-sample operations that should be per-block

The `isAudioRoute` audio mixdown loop (lines 173–176) is per-sample for AudioToFM, AudioToRing, AudioToWavetable. This is correct and intentional — these carry audio-rate content. The comment at line 182 explicitly justifies the SRO decimation for control-rate types. No issue.

### 4.3 `invRatio` variable — is it dead code?

**File:** `MegaCouplingMatrix.h` line 281
**Code:** `const float invRatio = 1.0f / static_cast<float>(kControlRateRatio);`

This variable is declared in `fillControlRateBuffer()` but never used. The per-block interpolation uses `invLen = 1.0f / static_cast<float>(blockLen)` (line 294) instead.

**Finding (LOW — dead code / compiler warning):** `invRatio` at line 281 is computed but never read. The build warning about unused variables mentioned in the audit prompt is confirmed. This is the variable. It is harmless but should be removed to eliminate the warning and reduce reader confusion.

**Fix:** Delete line 281 from `fillControlRateBuffer()`.

### 4.4 `fillControlRateBuffer` — edge case with `numSamples == 1`

When `numSamples == 1`, `blockLen = 1`, `invLen = 1.0f`, `t = 0.0f`. The interpolation writes `currentVal + 0.0f * (nextVal - currentVal) = currentVal`. This is correct. No issue.

### 4.5 `getSampleForCoupling()` called with `sampleIndex` during control-rate fill

In `fillControlRateBuffer()` line 289–290, `source->getSampleForCoupling(0, std::min(blockEnd, numSamples - 1))` is called. The `SynthEngine` contract (line 91–92 of SynthEngine.h) says this must be O(1) and return from a cached member. If any engine uses `sampleIndex` to index into a live buffer rather than returning a cached value, this would be incorrect at the call site (since the source hasn't rendered new audio for `blockEnd` yet — it's a prior-block cache). All known engines correctly ignore `sampleIndex` and return cached values. This is a contract enforcement issue to check when reviewing new engines.

---

## 5. CouplingPresetManager — Serial/Deserialize Correctness

### 5.1 TriangularCoupling in `validCouplingTypes` (PresetManager.h)

**Finding (HIGH — TriangularCoupling and AudioToBuffer/KnotTopology missing from preset validation whitelist):**

`PresetManager.h` lines 319–327 define `validCouplingTypes`:

```
"AmpToFilter", "AmpToPitch", "LFOToPitch", "EnvToMorph",
"AudioToFM", "AudioToRing", "FilterToFilter", "AmpToChoke",
"RhythmToBlend", "EnvToDecay", "PitchToPitch", "AudioToWavetable",
```

The list contains **12 entries**. The enum has **15 values**. Missing from the validation whitelist:
- `"AudioToBuffer"` (index 12)
- `"KnotTopology"` (index 13)
- `"TriangularCoupling"` (index 14)

The comment on line 317 says "Must match the CouplingType enum in SynthEngine.h 1:1" — it does not.

**Consequence:** Any `.xometa` preset with coupling pairs using `AudioToBuffer`, `KnotTopology`, or `TriangularCoupling` will fail the validation check at `PresetManager.h` line 896 (`validCouplingTypes.contains(cp.type)`) and those coupling pairs will be **silently dropped on load**. Factory presets and user presets using these coupling types will load with those routes missing, producing no error and no indication to the user.

The DEMO coupling presets added in Phase D (Coupling Performance System) that use KnotTopology in preset files would be silently discarded.

**Fix:** Add to `validCouplingTypes` in `PresetManager.h`:
```
"AudioToBuffer", "KnotTopology", "TriangularCoupling",
"Audio->Buffer", "Knot->Topology", "Triangular->Coupling"  // legacy alias form if desired
```

### 5.2 `overlayToCouplingPairs()` — typeNames array length mismatch

**Finding (HIGH — off-by-one in typeNames, TriangularCoupling unreachable):**

In `CouplingPresetManager.h` lines 319–324:

```cpp
static const char* typeNames[] = {
    "AmpToFilter", "AmpToPitch", "LFOToPitch", "EnvToMorph",
    "AudioToFM", "AudioToRing", "FilterToFilter", "AmpToChoke",
    "RhythmToBlend", "EnvToDecay", "PitchToPitch", "AudioToWavetable",
    "AudioToBuffer", "KnotTopology"
};
```

This array has **14 entries** (indices 0–13). `TriangularCoupling` (index 14) is absent.

Line 348: `int typeIdx = juce::jlimit(0, 13, juce::roundToInt(typeParam->load()));`

The `jlimit(0, 13, ...)` cap means TriangularCoupling routes (which have `couplingTypeIndex = 14`) are **clamped to 13 (KnotTopology)** in `overlayToCouplingPairs()`. A baked TriangularCoupling overlay will serialize as KnotTopology in the merged `.xometa` preset. This is silent corruption.

**Fix:**
1. Add `"TriangularCoupling"` as the 15th entry in `typeNames[]`.
2. Change the `jlimit` upper bound from 13 to `static_cast<int>(CouplingType::TriangularCoupling)` (which equals 14) to make it self-maintaining.

### 5.3 `fromJSON()` — KnotTopology upper-bound clamp

In `CouplingPresetManager.h` line 125–126:

```cpp
r.couplingTypeIndex = juce::jlimit(0, static_cast<int>(CouplingType::KnotTopology),
                                   static_cast<int>(routeObj->getProperty("type")));
```

`CouplingType::KnotTopology` has integer value 13. This clamps any type index to [0, 13], which means a serialized TriangularCoupling (index 14) loaded from a `.xocoupling` file will be clamped to KnotTopology.

Same issue exists in `bakeCurrent()` line 228:
```cpp
route.couplingTypeIndex = juce::jlimit(0,
    static_cast<int>(CouplingType::KnotTopology),
    juce::roundToInt(p->load()));
```

**Finding (HIGH — TriangularCoupling silently degrades to KnotTopology on save/load cycle):**

A complete roundtrip of a TriangularCoupling route through BAKE → save `.xocoupling` → load → `loadBakedCoupling()` produces a KnotTopology route, not a TriangularCoupling route. The user's intent is silently corrupted.

**Fix:** In both `fromJSON()` and `bakeCurrent()`, change the upper bound from `static_cast<int>(CouplingType::KnotTopology)` to `static_cast<int>(CouplingType::TriangularCoupling)`.

### 5.4 Macro amounts clamped to [0, 1] on load, but overlay macros are [0, 1] by design

`BakedCouplingState::fromJSON()` line 144: `juce::jlimit(0.0f, 1.0f, ...)` — correct, macros are unipolar.

`BakedCouplingState::Route.amount` is bipolar (-1.0 to 1.0) per the field comment. The load path at line 127–128 uses `juce::jlimit(-1.0f, 1.0f, ...)` — correct.

No issue here.

### 5.5 `loadBakedCoupling()` writes `amount` in raw domain to APVTS

`setParam()` at line 557–559 calls `p->convertTo0to1(rawValue)` before `setValueNotifyingHost()`. This is correct — APVTS params store normalized [0,1] values internally.

The `cp_*_amount` parameter is bipolar (range -1.0 to 1.0). `convertTo0to1(-1.0)` should return 0.0 and `convertTo0to1(1.0)` should return 1.0, assuming the APVTS range is [-1, 1]. This is correct.

No issue here, but depends on the `cp_*_amount` parameter being declared with range [-1.0, 1.0] in the APVTS layout. If it was accidentally declared as [0.0, 1.0], negative amounts would be clamped to 0. This should be verified in `XOceanusProcessor.cpp` where `cp_*_amount` parameters are declared.

### 5.6 `deletePreset()` — iterator invalidation after erase

`CouplingPresetManager.h` line 485: `library.erase(library.begin() + index)`. This is correct — `erase` returns a valid iterator to the element after the erased one, and we don't use the return value. No issue.

### 5.7 `scanDirectory()` — no deduplication

If `scanDirectory()` is called twice on the same directory (e.g. on a rescan), `library.clear()` at line 411 resets the library before populating. No duplicate accumulation. Correct.

### 5.8 `notifyListeners()` — listener modification during notification

`notifyListeners()` at lines 581–586 iterates `listeners` while calling callbacks. If a listener calls `removeListener()` or `addListener()` from within the callback, the iterator is invalidated (use-after-free or infinite loop). This is a standard listener safety issue.

**Finding (LOW — unsafe listener iteration):** `notifyListeners()` does not protect against listener list modification during dispatch. This is a common C++ pitfall. Fix by taking a snapshot copy before iterating: `auto listenersCopy = listeners; for (auto* l : listenersCopy) if (l) l->couplingPresetLibraryChanged();`

---

## 6. CouplingCrossfader — TriangularCoupling coverage

`CouplingCrossfader.h` `needsCrossfade()` (line 248) switch statement handles:
- AudioToFM, AudioToRing, AudioToWavetable, AudioToBuffer, KnotTopology → `return true`
- Everything else (including TriangularCoupling) → `return false`

TriangularCoupling carries control-rate state values (I/P/C scalars), not audio-rate signal. Classifying it as a hard-switchable control-rate type is correct — no crossfade needed.

`getCrossfadeDuration()` only checks AudioToBuffer and KnotTopology for long crossfade. Correct for TriangularCoupling.

No issues in CouplingCrossfader for TriangularCoupling.

---

## 7. Summary of Findings

### Priority: HIGH (fix before release)

| ID | Location | Description |
|----|----------|-------------|
| H01 | `PresetManager.h` line 319–327 | `validCouplingTypes` missing `AudioToBuffer`, `KnotTopology`, `TriangularCoupling` — these coupling pairs are silently dropped on preset load |
| H02 | `CouplingPresetManager.h` line 323–324 | `typeNames[]` in `overlayToCouplingPairs()` missing `TriangularCoupling` — baked TC routes saved as KnotTopology |
| H03 | `CouplingPresetManager.h` lines 125–126, 228 | `jlimit` upper bound is `KnotTopology` (13) not `TriangularCoupling` (14) — TC index clamped to Knot on every save/load roundtrip |
| H04 | `MegaCouplingMatrix.h` line 54 | `setEngines()` writes `activeEngines` on message thread with no synchronization — data race with audio thread reading in `processBlock()` |

### Priority: MEDIUM (fix in next sprint)

| ID | Location | Description |
|----|----------|-------------|
| M01 | `MegaCouplingMatrix.h` line 126 | `route.amount < 0.001f` silently drops all negative amounts — bipolar downward modulation is never delivered |
| M02 | `MegaCouplingMatrix.h` line 188–189 | `applyCouplingInput()` called with `numSamples` instead of `limit` — engines receive stale count when buffer is smaller than block |
| M03 | `MegaCouplingMatrix.h` lines 70, 75, 78, 83, 102, 112, 195, 221, 251 | `std::atomic_load/store` for `shared_ptr` deprecated in C++20 — migrate to `std::atomic<std::shared_ptr<T>>` |

### Priority: LOW (cleanup / documentation)

| ID | Location | Description |
|----|----------|-------------|
| L01 | `MegaCouplingMatrix.h` line 281 | `invRatio` declared but never used — dead code / compiler warning |
| L02 | `MegaCouplingMatrix.h` processBlock else-branch | TriangularCoupling falls through to control-rate path correctly but undocumented — add comment |
| L03 | `MegaCouplingMatrix.h` processBlock line 136 | No self-route guard for control-rate coupling types — undocumented but safe; add comment or guard |
| L04 | `OxytocinAdapter.h` lines 10–14 | bandA/B/C/D encoding is aspirational documentation — current behavior uses single RMS scalar; update comment to reflect reality |
| L05 | `CouplingPresetManager.h` lines 581–586 | `notifyListeners()` unsafe if listener modifies listener list during callback — take snapshot copy |

---

## 8. Recommended Fix Order

1. **H01** — Add `AudioToBuffer`, `KnotTopology`, `TriangularCoupling` to `validCouplingTypes` in `PresetManager.h`. One-line change.
2. **H02 + H03** — Fix the `typeNames[]` array in `overlayToCouplingPairs()` and the `jlimit` upper bounds in `fromJSON()` and `bakeCurrent()`. Three targeted edits in `CouplingPresetManager.h`.
3. **M01** — Change `route.amount < 0.001f` to `std::abs(route.amount) < 0.001f` in `processBlock()`. One character change.
4. **M02** — Change `numSamples` to `limit` in the `applyCouplingInput()` call at line 189. One word change.
5. **L01** — Delete the `invRatio` declaration at line 281.
6. **H04** — Review the `setEngines()` call site in `XOceanusProcessor.cpp`. If hot-swap can occur during audio rendering, add synchronization.

Items H01–H03 are the most impactful: they cause silent data loss in preset files for three coupling types, including KnotTopology which is used in factory Entangled mood presets.
