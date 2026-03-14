# AudioToBuffer — Implementation Summary

**Date:** 2026-03-14
**Spec:** `Docs/xopal_phase1_architecture.md` §15 (V008 revision)
**Agent:** 6C spec → implementation pass

---

## What Was Implemented

### 1. CouplingType enum — `AudioToBuffer` added

**File:** `Source/Core/SynthEngine.h`

`AudioToBuffer` appended to the `CouplingType` enum with an inline doc comment explaining
its purpose (continuous stereo streaming, freeze support) and pointing to the spec section.
`AudioToWavetable` is the prior art; `AudioToBuffer` is the new V008 type.

### 2. MegaCouplingMatrix — two changes

**File:** `Source/Core/MegaCouplingMatrix.h`

**a. Second scratch buffer (`couplingBufferR`)**
`prepare()` now allocates `couplingBufferR` alongside `couplingBuffer`. The R buffer is used
exclusively by `processAudioRoute()` to collect the right-channel samples for stereo push.
Memory cost: `maxBlockSize × 4 bytes` (~4KB at 1024 block size).

**b. `processAudioRoute()` stub + dispatch**
In `processBlock()`, `AudioToBuffer` routes are intercepted before the mono-mixdown branch
and forwarded to `processAudioRoute()` with a `continue` after the call so they skip
`applyCouplingInput()` entirely.

`processAudioRoute()` is a private method that:
- Fills `couplingBuffer` (L) and `couplingBufferR` (R) from `source->getSampleForCoupling()`
- Has a complete TODO comment showing the exact downcast + `pushBlock()` call for Phase 2
- Suppresses unused-variable warnings for `dest` and `route` with `(void)` casts
- Compiles and links today with no OpalEngine dependency

**c. `isAudioRoute` guard updated**
`AudioToBuffer` is included in the `isAudioRoute` predicate so that if for any reason the
route reaches the general path, it is treated as stereo (consistent with the spec intent).

### 3. AudioRingBuffer struct — new file

**File:** `Source/Core/AudioRingBuffer.h`

Full implementation per spec §15.3, with additions:

| Feature | Status |
|---|---|
| `data[2]` planar stereo float storage | Implemented |
| `capacity` set in `prepare()` | Implemented |
| `writeHead` (atomic int) | Implemented |
| `shadowWriteHead` (atomic int) | Implemented |
| `frozen` (atomic bool) | Implemented |
| `prepare(sampleRate, durationSeconds)` | Implemented |
| `pushBlock(srcL, srcR, numSamples, level)` | Implemented |
| `readAt(channel, fractionalOffset)` with linear interpolation | Implemented |
| `resumeFromShadow()` — advance main head to shadow on unfreeze | Added (not in spec, required for correctness) |
| No heap allocation after `prepare()` | Confirmed |
| No locks | Confirmed |
| `memory_order_acquire/release` pairs | Correct on all atomic accesses |

**`resumeFromShadow()` addition:** The spec describes the freeze-resume behaviour
(§15.7: "the main write head jumps to the shadow position") but does not specify where
this jump call lives. `resumeFromShadow()` encapsulates it so `OpalEngine::handleFreezeRelease()`
can call it cleanly before clearing the `frozen` flag. This is a pure correctness addition
consistent with the spec's intent.

---

## What Is Still Stubbed

### `processAudioRoute()` — OpalEngine downcast

The stub fills both scratch buffers but does not call `pushBlock()`. The TODO comment
inside the stub shows the exact code needed:

```cpp
// TODO (Phase 2): Replace this block with the real push:
//
//   auto* opalDest = dynamic_cast<OpalEngine*>(dest);
//   if (!opalDest) return;
//   AudioRingBuffer* rb = opalDest->getGrainBuffer(route.sourceSlot);
//   if (!rb) return;
//   rb->pushBlock(couplingBuffer.data(), couplingBufferR.data(),
//                 numSamples, route.amount);
```

This is blocked on `OpalEngine` existing as a concrete type that `MegaCouplingMatrix.h`
can see. Options for Phase 2:

1. **Forward-declare + downcast** (spec approach) — include `OpalEngine.h` in
   `MegaCouplingMatrix.h` and `dynamic_cast<OpalEngine*>`.
2. **IAudioBufferSink interface** (cleaner) — add a `getGrainBuffer(int slot)` virtual
   method to a new interface; `OpalEngine` implements it; `MegaCouplingMatrix` casts to
   the interface. Avoids the circular header dependency.

Option 2 is recommended for Phase 2 because it keeps `MegaCouplingMatrix.h` engine-agnostic.

### OpalEngine — not implemented

`OpalEngine.h` does not exist yet. Phase 2 is the scaffold phase per the spec's Phase Map.
`AudioRingBuffer` is ready to be embedded as:

```cpp
// OpalEngine.h
#include "../../Core/AudioRingBuffer.h"

class OpalEngine : public SynthEngine {
    ...
    AudioRingBuffer* getGrainBuffer(int sourceSlot) {
        if (sourceSlot < 0 || sourceSlot >= MegaCouplingMatrix::MaxSlots) return nullptr;
        return &coupledBuffers[static_cast<size_t>(sourceSlot)];
    }
private:
    std::array<AudioRingBuffer, MegaCouplingMatrix::MaxSlots> coupledBuffers;
```

### Cycle detection (`wouldCreateCycle`)

Spec §15.5 specifies a DFS cycle check in `EngineRegistry`. Not implemented in this pass —
deferred to Phase 2 when `OpalEngine` routes are actually registered.

### FREEZE gesture state machine

Spec §15.7 defines `FreezeState` enum and `handleFreezePress/Release()` methods.
These live in `OpalEngine` — deferred to Phase 2.

---

## File Manifest

| File | Change |
|---|---|
| `Source/Core/SynthEngine.h` | `AudioToBuffer` added to `CouplingType` enum |
| `Source/Core/MegaCouplingMatrix.h` | `couplingBufferR` added; `processAudioRoute()` stub added; `isAudioRoute` guard updated; dispatch logic added |
| `Source/Core/AudioRingBuffer.h` | New file — complete lock-free ring buffer implementation |
| `Docs/audio_to_buffer_implementation.md` | This file |

---

## Phase 2 Gate Checklist

Before marking `AudioToBuffer` fully operational:

- [ ] `OpalEngine.h` scaffolded with `coupledBuffers` array and `getGrainBuffer()`
- [ ] `processAudioRoute()` TODO replaced with real downcast + `pushBlock()` call
      (or `IAudioBufferSink` interface route)
- [ ] Cycle detection (`wouldCreateCycle`) added to `EngineRegistry`
- [ ] FREEZE gesture state machine implemented in `OpalEngine`
- [ ] `AudioRingBuffer::resumeFromShadow()` called from `handleFreezeRelease()`
- [ ] At least one `AudioToBuffer` route registered in normalled defaults
- [ ] Audible: external engine audio granulated by OPAL grain scheduler
