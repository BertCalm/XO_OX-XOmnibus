# AudioToBuffer Phase 2 — OpalEngine Integration

**Round**: 11F
**Date**: 2026-03-14
**Status**: COMPLETE

---

## Summary

Phase 2 makes OpalEngine the first live receiver of the `AudioToBuffer` coupling type.
Any engine in the XOmnibus slot grid can now stream its stereo output directly into
OPAL's internal grain buffer, allowing the granulator to continuously shred another
engine's audio in real time.

---

## What Was Implemented

### 1. MegaCouplingMatrix::processAudioRoute() — completed

**File**: `Source/Core/MegaCouplingMatrix.h`

The Phase 1 TODO stub was replaced with a full implementation:

1. **Cycle detection guard** — if `route.sourceSlot == route.destSlot`, the route is
   skipped. This prevents a single engine from writing into its own grain buffer (which
   would accumulate DC and saturate the granulator). Full graph-level cycle detection
   (e.g., A→B→A ping-pong chains) is deferred to Phase 3.

2. **Stereo scratch fill** — L and R channels are read sample-by-sample from the source
   engine's output cache via `getSampleForCoupling(0/1, i)`.

3. **OpalEngine downcast** — `dynamic_cast<OpalEngine*>(dest)` returns the receiver.
   Returns early for any non-OPAL destination. A future `IAudioBufferSink` interface
   will replace the downcast in Phase 3 to support multiple receiver engine types.

4. **Per-slot ring buffer lookup** — `OpalEngine::getGrainBuffer(route.sourceSlot)`
   returns a pointer to one of 4 dedicated `AudioRingBuffer` instances. Each source
   slot gets its own ring buffer, preventing collisions between simultaneous sources.

5. **AudioRingBuffer::pushBlock()** — lock-free, allocation-free stereo write. The
   `route.amount` scalar is passed as the level argument to scale input gain.

`dest->applyCouplingInput()` is intentionally NOT called — the ring buffer is the
exclusive sink for `AudioToBuffer` routes.

---

### 2. MegaCouplingMatrix.h — OpalEngine.h include added

`#include "../Engines/Opal/OpalEngine.h"` was added so the downcast compiles without
a forward declaration stub.

---

### 3. OpalEngine — 4 per-slot AudioRingBuffer members

**File**: `Source/Engines/Opal/OpalEngine.h`

```cpp
static constexpr int   kOpalInputSlots            = 4;
static constexpr float kOpalExternalBufferSeconds  = 8192.0f / 44100.0f; // ~186ms

std::array<AudioRingBuffer, kOpalInputSlots> inputBuffers;
```

Each buffer is initialized in `prepare()` via `AudioRingBuffer::prepare()`.
Slot 0–3 map directly to MegaCouplingMatrix source slot indices.

---

### 4. OpalEngine::getGrainBuffer(int slot)

Public accessor returning `&inputBuffers[slot]`, or `nullptr` for out-of-range indices.
Called by `MegaCouplingMatrix::processAudioRoute()` on every audio callback.

---

### 5. OpalEngine::receiveAudioBuffer(...)

```cpp
void receiveAudioBuffer(const AudioRingBuffer& src, int numSamples,
                        float mix, bool frozen) noexcept
```

Reads `numSamples` mono-averaged samples from `src` (most-recent samples first) and
accumulates them into `extAudioBufL` / `extAudioBufR` — the per-block blend cache.

The fractional offset mapping ensures `n=0` retrieves the most recent written sample
and `n=numSamples-1` retrieves the oldest sample of this block, preserving temporal
ordering through the ring.

---

### 6. OpalEngine — grain source blend in renderBlock()

Before the per-sample grain-source write loop, `renderBlock()` now:

1. Zeroes `extAudioBufL/R`.
2. Iterates all 4 input slots; for each with a non-zero capacity, calls
   `receiveAudioBuffer()` with level = `externalMix / kOpalInputSlots`.
3. Inside the per-sample loop, blends:

```
srcSample = srcSample * (1 - externalMix) + extAudioBufL[n]
```

where `extAudioBufL[n]` is the pre-scaled sum from all active ring buffer slots.

This gives a smooth continuous cross-fade:
- `opal_externalMix = 0.0` — pure internal oscillator / coupling source
- `opal_externalMix = 1.0` — pure external AudioToBuffer content

---

### 7. opal_externalMix parameter (ID: `opal_externalMix`)

Added to `OpalParam` namespace, `addParametersImpl()`, and `attachParameters()`.

```
Range:   0.0 – 1.0
Default: 0.0 (no change to existing behaviour; parameter is additive)
Step:    0.01
```

This is parameter 87 in OpalEngine's layout (86 frozen + 1 new). Frozen IDs are
unaffected — only new IDs were added.

---

### 8. Per-block external audio cache (extAudioBufL / extAudioBufR)

```cpp
std::vector<float> extAudioBufL;
std::vector<float> extAudioBufR;
```

Pre-allocated in `prepare()` to `maxBlockSize`. Zeroed in `reset()`. No heap
allocation on the audio thread.

---

## What Remains for Phase 3

| Item | Priority | Notes |
|------|----------|-------|
| Full graph-level cycle detection | P1 | Detect A→B→A, A→B→C→A chains. Needs adjacency matrix or DFS in MegaCouplingMatrix. Phase 2 only guards same-slot self-routes. |
| FREEZE state machine | P1 | `opal_freeze` currently suspends OpalGrainBuffer writes but not AudioRingBuffer pushes. Phase 3 should gate `pushBlock()` or mirror the frozen flag into `AudioRingBuffer::frozen` from OpalEngine's freeze gesture handler. `AudioRingBuffer::resumeFromShadow()` is implemented but not yet called. |
| Multiple receiver engine types | P2 | Replace `dynamic_cast<OpalEngine*>` with an `IAudioBufferSink` interface so other future engines (e.g., XOpal-derived loopers) can receive AudioToBuffer routes without modifying MegaCouplingMatrix. |
| UI slot assignment | P2 | No UI yet exposes which OPAL input slot a source engine writes to. Needs coupling strip UX for slot selection. |
| OVERWORLD→OPAL and DRIFT→OPAL coupling presets | P3 | First-use preset demonstrations of the Time Telescope coupling type. |
| opal_externalMix mod matrix destination | P3 | Adding `opal_externalMix` as a mod matrix destination would allow LFO-driven source cross-fades — "source morphing" over time. |

---

## AudioToBuffer Data Flow (Phase 2)

```
Source engine renderBlock()
       │ getSampleForCoupling(0/1, n)
       ▼
MegaCouplingMatrix::processAudioRoute()
  [cycle guard: sourceSlot != destSlot]
  [fill couplingBuffer L/R from source cache]
  [dynamic_cast<OpalEngine*>(dest)]
  [getGrainBuffer(route.sourceSlot) → AudioRingBuffer*]
  [AudioRingBuffer::pushBlock(L, R, numSamples, amount)]
       │
       ▼
OpalEngine::inputBuffers[slot]  (AudioRingBuffer — ~186ms stereo ring)
       │
OpalEngine::renderBlock()
  [receiveAudioBuffer() for each slot]
  [blend into extAudioBufL/R]
  [per-sample: srcSample = internal*(1-mix) + external*mix]
  [grainBuffer.write(srcSample, frozen)]
       │
       ▼
OpalGrainPool::processAll()  →  12 cloud voices × 32 grains → stereo out
```

---

## Files Modified

| File | Change |
|------|--------|
| `Source/Core/MegaCouplingMatrix.h` | Completed `processAudioRoute()` stub; added `OpalEngine.h` include |
| `Source/Engines/Opal/OpalEngine.h` | Added `AudioRingBuffer.h` include; `opal_externalMix` param; 4 `inputBuffers`; `extAudioBufL/R`; `getGrainBuffer()`; `receiveAudioBuffer()`; prepare/reset/renderBlock updates |

## Files Created

| File | Purpose |
|------|---------|
| `Docs/audio_to_buffer_phase2.md` | This document |
