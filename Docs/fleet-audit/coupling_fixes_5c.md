# Coupling Fixes 5c

**Date:** 2026-03-14
**Files modified:** `Source/Engines/Snap/SnapEngine.h`, `Source/Engines/Opal/OpalEngine.h`

---

## Bug 1 — SNAP: Missing AmpToFilter coupling handler

**Root cause:** `applyCouplingInput` in `SnapEngine.h` handled `AmpToPitch`, `LFOToPitch`, and `PitchToPitch` but had no case for `CouplingType::AmpToFilter`. When a partner engine (e.g. ONSET's ch2 envelope level) routed amplitude to SNAP's filter via the MegaCouplingMatrix, the `default: break` branch silently discarded the value.

**Fix:**
- Added `float couplingCutoffMod = 1.0f` member (unity = no change).
- Added `case CouplingType::AmpToFilter` in `applyCouplingInput` that stores `amount` as a BPF center multiplier, clamped to `[0.1, 2.0]` to prevent full filter closure or aliasing.
- Consumed and reset `couplingCutoffMod` at the start of `renderBlock` (same pattern as `externalPitchModulation`), then applied it as a multiplier to `effectiveBpfCenter`:
  ```
  effectiveBpfCenter = effectiveCutoff * (1.0f + 0.08f * sin(lfoPhase)) * cutoffMod;
  ```
- Updated the engine header doc comment to list `AmpToFilter` as a supported input coupling type.

**Effect:** A partner engine routing amplitude (e.g. kick transient) to SNAP's filter now opens/closes feliX's BPF in real-time. Amount 0 → minimum cutoff (0.1×), amount 1 → no change, amount > 1 → brightened cutoff.

---

## Bug 2 — OPAL: `getSampleForCoupling` ignores `sampleIndex`

**Root cause:** `getSampleForCoupling` returned `lastSampleL` or `lastSampleR` for every `sampleIndex`, returning the same end-of-block scalar for all positions. Tight coupling types (`AudioToFM`, `AudioToRing`) that call `getSampleForCoupling` per sample index received stale data from the previous block's last output, not the current block's per-position value.

**Fix:**
- Added `std::vector<float> outputCacheLeft` and `outputCacheRight` member vectors.
- Resized them to `maxBlockSize` in `prepare()` alongside the existing `workBufL/R` and `couplingBufL/R` vectors.
- At the end of `renderBlock()`, after the FX chain and master level loop, a new loop writes `outL[n]` / `outR[n]` into `outputCacheLeft[n]` / `outputCacheRight[n]` for every sample in the block. The existing `lastSampleL/R` scalar is preserved as a fallback.
- Updated `getSampleForCoupling` to do a bounds-checked index into `outputCacheLeft/Right`, with `lastSampleL/R` as a fallback for out-of-range indices (block-level coupling callers).

**Effect:** Per-sample coupling reads on OPAL now return the exact rendered output value at each sample position. Sub-block audio accuracy is restored for tight coupling routes. No memory allocation on the audio thread — buffers are pre-allocated in `prepare()`.

---

## Verification checklist

- [ ] Build passes (no new compiler errors in either engine)
- [ ] `auval -v aumu XOmn Xoox` passes
- [ ] ONSET → SNAP AmpToFilter coupling: kick transient visibly modulates SNAP BPF center in preset browser
- [ ] OPAL → any engine tight coupling (AudioToFM): waveform modulation is time-accurate (no 1-block latency offset)
