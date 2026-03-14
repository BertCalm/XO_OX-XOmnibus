# AudioToBuffer Phase 3 — Generalization Specification

**Round**: 12D
**Date**: 2026-03-14
**Status**: SPEC — not yet implemented
**Depends on**: Phase 1 (`Docs/audio_to_buffer_implementation.md`), Phase 2 (`Docs/audio_to_buffer_phase2.md`)

---

## Executive Summary

Phase 1 added the ring buffer and the `AudioToBuffer` enum value. Phase 2 made OpalEngine the
first live receiver, using a `dynamic_cast<OpalEngine*>` inside `processAudioRoute()`. Phase 3
removes that engine-specific coupling from the routing core and generalizes every aspect of the
system so that:

- Any future engine can receive `AudioToBuffer` routes by implementing one interface.
- Feedback loops (A→B→A, A→B→C→A) are detected and rejected before they corrupt audio.
- The FREEZE gesture has a fully defined four-state lifecycle tied to `AudioRingBuffer`.
- One source engine can simultaneously stream into up to four destination engines.
- The performance ceiling is documented and tested.

No existing parameter IDs change. No preset format changes. `OpalEngine` continues to work
exactly as it does after Phase 2 — Phase 3 is a superset of Phase 2 behaviour.

---

## 1. `IAudioBufferSink` Interface

### 1.1 Purpose

`MegaCouplingMatrix::processAudioRoute()` currently contains:

```cpp
auto* opalDest = dynamic_cast<OpalEngine*>(dest);
if (opalDest == nullptr)
    return;
```

This makes `MegaCouplingMatrix.h` depend on `OpalEngine.h`, couples the routing core to a
single engine type, and prevents any other engine from ever receiving an `AudioToBuffer` route
without modifying the matrix. The `IAudioBufferSink` interface breaks this dependency.

### 1.2 Declaration

**New file**: `Source/Core/IAudioBufferSink.h`

```cpp
#pragma once
#include "AudioRingBuffer.h"

namespace xomnibus {

//==============================================================================
// IAudioBufferSink — implemented by any engine that can receive an
// AudioToBuffer coupling route.
//
// MegaCouplingMatrix::processAudioRoute() queries the destination engine
// for this interface via dynamic_cast<IAudioBufferSink*>(dest). If the cast
// returns nullptr, the route is silently skipped — the dest engine does not
// support streaming audio input.
//
// Design constraints (all must hold for a conforming implementation):
//   - getGrainBuffer() must be callable on the audio thread with no allocation.
//   - getNumInputSlots() must return a value ≥ 1 and ≤ MegaCouplingMatrix::MaxSlots.
//   - receiveAudioBuffer() must be real-time safe: no allocation, no I/O, no locks.
//
// Thread safety note: all three methods are called on the audio thread by
// MegaCouplingMatrix. The implementing class is responsible for ensuring
// that prepare() (which calls AudioRingBuffer::prepare()) has completed on
// the non-audio thread before processBlock() is first called.
//
class IAudioBufferSink
{
public:
    virtual ~IAudioBufferSink() = default;

    // Return a pointer to the ring buffer for the given source slot index.
    // Slot indices correspond to MegaCouplingMatrix source slot positions (0–3).
    // Returns nullptr if slot is out of range — callers must check.
    virtual AudioRingBuffer* getGrainBuffer (int slot) noexcept = 0;

    // Return the number of input slots this engine supports.
    // Used by MegaCouplingMatrix to validate slot assignments.
    // Must return a value in [1, MegaCouplingMatrix::MaxSlots].
    virtual int getNumInputSlots() const noexcept = 0;

    // Receive and blend a block of audio from a ring buffer source.
    // Called by OpalEngine (and future engines) during renderBlock().
    // `src`        — the ring buffer to read from
    // `numSamples` — number of samples in the current block
    // `mix`        — blend ratio (0.0 = internal only, 1.0 = external only)
    // `frozen`     — current FREEZE state; implementor may hold blend cache
    //               rather than advancing through the ring when true
    virtual void receiveAudioBuffer (const AudioRingBuffer& src,
                                     int numSamples,
                                     float mix,
                                     bool frozen) noexcept = 0;
};

} // namespace xomnibus
```

### 1.3 OpalEngine conformance

`OpalEngine` already satisfies the interface contract informally. To make it a conforming
implementation, two changes are required:

1. `OpalEngine` inherits from `IAudioBufferSink` in addition to `SynthEngine`:
   ```cpp
   class OpalEngine : public SynthEngine, public IAudioBufferSink
   ```

2. `getGrainBuffer`, `getNumInputSlots`, and `receiveAudioBuffer` are marked `override`.
   The existing bodies are unchanged.

`getNumInputSlots()` returns `kOpalInputSlots` (currently `4`).

### 1.4 MegaCouplingMatrix — replace the downcast

In `processAudioRoute()`, replace:
```cpp
auto* opalDest = dynamic_cast<OpalEngine*>(dest);
if (opalDest == nullptr) return;
AudioRingBuffer* rb = opalDest->getGrainBuffer(route.sourceSlot);
```
with:
```cpp
auto* sink = dynamic_cast<IAudioBufferSink*>(dest);
if (sink == nullptr) return;
if (route.sourceSlot >= sink->getNumInputSlots()) return;
AudioRingBuffer* rb = sink->getGrainBuffer(route.sourceSlot);
```

Remove `#include "../Engines/Opal/OpalEngine.h"` from `MegaCouplingMatrix.h` and replace it
with `#include "IAudioBufferSink.h"`. `MegaCouplingMatrix.h` no longer knows about any
specific engine type.

---

## 2. Graph Cycle Detection

### 2.1 Why Phase 2's guard is insufficient

Phase 2 added a single-slot guard (`if (route.sourceSlot == route.destSlot) return`). This
catches only self-routes. It does not catch:
- Two-node cycles: Engine A (slot 0) → Engine B (slot 1) → Engine A (slot 0)
- Three-node cycles: A→B→C→A
- Any cycle where the path length exceeds one hop

An undetected cycle at the ring-buffer level is less catastrophic than a sample-level feedback
loop (there is no instantaneous feedback because the ring has latency), but it still produces
continuous DC accumulation and granulator saturation within ~1–2 seconds. The route must be
rejected before it is registered.

### 2.2 Algorithm — DFS on the AudioToBuffer adjacency list

Cycle detection runs on the **message thread**, during `addRoute()`, before the new route is
committed to the atomic route list. It is never called on the audio thread.

**Data structure**: A directed adjacency list maintained inside `MegaCouplingMatrix`.

```
// Keyed by source slot index (0–3).
// Value: set of destination slot indices that have an active AudioToBuffer route
//        from the source.
std::array<std::vector<int>, MaxSlots> audioBufferAdjacency;
```

This is a message-thread-only structure — no atomic access required.

**Algorithm**: Before committing a new `AudioToBuffer` route from `srcSlot` to `dstSlot`:

```
wouldCreateCycle(srcSlot, dstSlot):
    visited = {} (empty set)
    stack   = [dstSlot]

    while stack is not empty:
        node = stack.pop()
        if node == srcSlot:
            return true          // cycle found: adding this edge would close a loop
        if node in visited:
            continue             // already explored
        visited.add(node)
        for each neighbor in audioBufferAdjacency[node]:
            stack.push(neighbor)

    return false                 // no cycle
```

**Integration into `addRoute()`**:

```cpp
void addRoute (CouplingRoute route)
{
    if (route.type == CouplingType::AudioToBuffer)
    {
        if (route.sourceSlot == route.destSlot)
        {
            // Self-route: reject immediately.
            juce::Logger::writeToLog ("AudioToBuffer: rejected self-route on slot "
                                      + juce::String (route.sourceSlot));
            return;
        }

        if (wouldCreateCycle (route.sourceSlot, route.destSlot))
        {
            juce::Logger::writeToLog ("AudioToBuffer: cycle detected, route "
                + juce::String (route.sourceSlot) + " → "
                + juce::String (route.destSlot) + " rejected");
            return;
        }

        // Safe to commit — register in adjacency list.
        audioBufferAdjacency[static_cast<size_t>(route.sourceSlot)]
            .push_back(route.destSlot);
    }

    // Standard atomic route-list swap (existing code).
    auto current   = std::atomic_load (&routeList);
    auto newRoutes = std::make_shared<std::vector<CouplingRoute>> (*current);
    newRoutes->push_back (route);
    std::atomic_store (&routeList, newRoutes);
}
```

**`removeUserRoute()` must also update the adjacency list**:

```cpp
// After erasing from newRoutes, also remove from adjacency list:
auto& neighbors = audioBufferAdjacency[static_cast<size_t>(sourceSlot)];
neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), destSlot),
                neighbors.end());
```

**`clearRoutes()` must reset the adjacency list**:

```cpp
void clearRoutes()
{
    for (auto& v : audioBufferAdjacency) v.clear();
    auto newRoutes = std::make_shared<std::vector<CouplingRoute>>();
    std::atomic_store (&routeList, newRoutes);
}
```

### 2.3 Complexity

- DFS visits at most `MaxSlots` (4) nodes.
- The adjacency list holds at most `MaxSlots × MaxSlots` (16) edges.
- `wouldCreateCycle` runs in O(MaxSlots²) = O(16) — effectively O(1) for this graph size.
- Called only on route registration (message thread, not time-critical).

### 2.4 Normalled routes

Normalled routes (set via `addNormalledDefaults()`) bypass cycle detection by convention —
they are authored by the XOmnibus team and are guaranteed acyclic. The `isNormalled` flag can
be used as the guard:

```cpp
if (route.type == CouplingType::AudioToBuffer && !route.isNormalled)
{
    // run cycle check
}
```

---

## 3. FREEZE State Machine

### 3.1 States

The FREEZE gesture on an `IAudioBufferSink` engine follows a four-state lifecycle:

```
Playing ──(press)──▶ FreezeRequest ──(block boundary)──▶ Frozen
                                                              │
Playing ◀──(block boundary)── ResumeRequest ◀──(release)────┘
```

| State | AudioRingBuffer::frozen | pushBlock writes? | grainBuffer writes? | Notes |
|-------|------------------------|-------------------|---------------------|-------|
| `Playing` | `false` | Yes | Yes | Normal operation |
| `FreezeRequest` | set to `true` at next block start | No (from next block) | Yes (last block drains) | Transition state; lives ≤ 1 block |
| `Frozen` | `true` | Shadow head only | No — `write(sample, frozen=true)` nops | Ring holds the freeze snapshot |
| `ResumeRequest` | cleared to `false` at next block start after `resumeFromShadow()` | Yes | Yes | Transition state; lives ≤ 1 block |

### 3.2 Implementation in OpalEngine

**New enum** (add inside `OpalEngine`, before class member declarations):

```cpp
enum class FreezeState { Playing, FreezeRequest, Frozen, ResumeRequest };
```

**New member**:

```cpp
FreezeState freezeState { FreezeState::Playing };
```

**Existing `pFreeze` parameter** (`opal_freeze`, bool param 0.0/1.0) drives the transition.
The state machine is evaluated once at the top of `renderBlock()`, before per-sample processing.

**State machine tick** (call at the top of `renderBlock()`):

```cpp
void tickFreezeStateMachine() noexcept
{
    const bool freezeParamOn = (safeLoadF(pFreeze, 0.0f) > 0.5f);

    switch (freezeState)
    {
        case FreezeState::Playing:
            if (freezeParamOn)
                freezeState = FreezeState::FreezeRequest;
            break;

        case FreezeState::FreezeRequest:
            // Commit the freeze at the block boundary.
            // All inputBuffers share the same frozen flag because they are all
            // frozen together by a single gesture.
            for (auto& rb : inputBuffers)
                rb.frozen.store(true, std::memory_order_release);
            freezeState = FreezeState::Frozen;
            break;

        case FreezeState::Frozen:
            if (!freezeParamOn)
                freezeState = FreezeState::ResumeRequest;
            break;

        case FreezeState::ResumeRequest:
            // Advance main write heads to shadow positions before re-enabling writes.
            for (auto& rb : inputBuffers)
            {
                rb.resumeFromShadow();                           // main head → shadow
                rb.frozen.store(false, std::memory_order_release);
            }
            freezeState = FreezeState::Playing;
            break;
    }
}
```

**`frozen` variable in `renderBlock()`**: Replace the current direct `pFreeze` read with
a derived value from the state machine:

```cpp
const bool frozen = (freezeState == FreezeState::Frozen
                  || freezeState == FreezeState::FreezeRequest);
```

This ensures that `OpalGrainBuffer::write(sample, frozen)` stops advancing the grain buffer
write head exactly when the ring buffer also stops accepting new pushes — the two buffers
stay in lock-step.

### 3.3 MIDI CC trigger

OPAL should respond to **CC#64 (Sustain Pedal)** as a hardware-accessible FREEZE toggle.
CC ≥ 64 = FREEZE ON (same convention as the sustain pedal standard).

**Implementation**: In `OpalEngine::handleMidi()` (or equivalent MIDI callback):

```cpp
if (message.isController() && message.getControllerNumber() == 64)
{
    const bool pedalDown = (message.getControllerValue() >= 64);
    // Mirror into the opal_freeze parameter so the state machine picks it up.
    if (auto* param = apvts.getParameter(OpalParam::FREEZE))
        param->setValueNotifyingHost(pedalDown ? 1.0f : 0.0f);
}
```

This routes through the existing parameter so that DAW automation, preset state, and the UI
toggle all remain consistent — the CC is not a separate code path.

**Rationale for CC#64**: The sustain pedal is universally available on MIDI keyboards, maps
intuitively to "hold this moment", and is distinct from any parameter CC assignments already
in use across the XOmnibus fleet.

### 3.4 FREEZE and `receiveAudioBuffer()`

The `frozen` parameter in `receiveAudioBuffer()` was reserved in Phase 2 with a `(void)frozen`
suppression. In Phase 3, it is used:

```cpp
void receiveAudioBuffer (const AudioRingBuffer& src, int numSamples,
                         float mix, bool frozen) noexcept
{
    if (frozen)
    {
        // Hold the blend cache at its last valid content.
        // Do not zero extAudioBufL/R — keep the frozen moment audible.
        return;
    }
    // ... existing read-and-accumulate logic ...
}
```

The call site in `renderBlock()` passes the derived `frozen` value (from the state machine),
not a raw parameter read.

---

## 4. Multi-Receiver Architecture

### 4.1 Model

In Phase 2, exactly one engine can receive from any one source — the `OpalEngine` downcast is
the only path. Phase 3 allows up to `MaxSlots` (4) engines to simultaneously receive the same
source engine's audio.

**Example**: Engine in slot 0 (e.g. OVERWORLD) streams into:
- Slot 1 (OPAL) via `AudioToBuffer` route (slot 0 → slot 1)
- Slot 2 (a future LOOPER engine) via `AudioToBuffer` route (slot 0 → slot 2)
- Slot 3 (ODYSSEY, if it implements `IAudioBufferSink`) via `AudioToBuffer` route (slot 0 → slot 3)

Each is a distinct `CouplingRoute` entry in the route list. `processAudioRoute()` is called
once per route per block — three separate `pushBlock()` calls for the example above.

### 4.2 Per-source recipient list

`MegaCouplingMatrix` does not need an additional data structure to support this. The existing
route list already supports multiple routes with the same `sourceSlot`. `processBlock()` iterates
all active routes; multiple `AudioToBuffer` routes from the same source are handled by separate
`processAudioRoute()` calls, each pushing into a different `IAudioBufferSink`.

The only constraint is that two routes from the same source to the same destination are not
meaningful — the second push would overwrite the ring buffer written by the first within the same
block. `addRoute()` should guard against duplicate (sourceSlot, destSlot, AudioToBuffer) pairs:

```cpp
// In addRoute(), for AudioToBuffer routes, before cycle check:
for (const auto& existing : *current)
{
    if (existing.type == CouplingType::AudioToBuffer
        && existing.sourceSlot == route.sourceSlot
        && existing.destSlot   == route.destSlot)
    {
        juce::Logger::writeToLog ("AudioToBuffer: duplicate route rejected");
        return;
    }
}
```

### 4.3 Slot indices: source slot vs. receiver slot

`route.sourceSlot` identifies which engine slot in the XOmnibus grid is the source. It is also
the index passed to `IAudioBufferSink::getGrainBuffer(slot)`. This means that on the receiver,
slot N holds audio from the source engine that occupies grid slot N — not from the Nth route
registered.

**Implication**: If two sources (slots 0 and 2) both route to the same receiver (slot 1), the
receiver's `inputBuffers[0]` holds slot-0 audio and `inputBuffers[2]` holds slot-2 audio.
`inputBuffers[1]` (self-route) and `inputBuffers[3]` (no route from slot 3) remain silent/zeroed.

This is exactly the current OPAL behaviour — Phase 3 codifies it as the spec-level contract for
all `IAudioBufferSink` implementations.

### 4.4 `getNumInputSlots()` validation

`processAudioRoute()` now calls `sink->getNumInputSlots()` before accessing a slot:

```cpp
if (route.sourceSlot >= sink->getNumInputSlots())
{
    juce::Logger::writeToLog ("AudioToBuffer: sourceSlot "
        + juce::String(route.sourceSlot)
        + " exceeds sink capacity "
        + juce::String(sink->getNumInputSlots()));
    return;
}
```

An engine with fewer than 4 input slots (e.g., a looper with only 2) is safe to use as a
receiver — routes to out-of-range slots are silently dropped.

---

## 5. Performance Budget

### 5.1 Operation costs

`AudioRingBuffer::pushBlock()` iterates `numSamples` in a simple loop:
- Per-sample cost: 1 conditional branch, 2 float stores (L + R), 2 integer increments.
- Two atomic store operations per call (outside the loop): `writeHead` and `shadowWriteHead`.
- Classification: O(numSamples) with a very small constant.

`AudioRingBuffer::readAt()` is O(1):
- 1 atomic load, 3 float multiplies, 2 float adds, 2 modular indexing operations.
- Called from `receiveAudioBuffer()` once per sample in the current block → O(numSamples) per slot.

### 5.2 Worst-case accounting

| Scenario | Operations per block |
|----------|---------------------|
| 1 source → 1 receiver, 128 samples | 128 float writes (pushBlock) + 128 float reads (receiveAudioBuffer) = 256 |
| 1 source → 4 receivers, 128 samples | 4 × 128 pushBlock writes + 4 × 128 receive reads = 1,024 |
| 4 sources → 4 receivers (fully saturated grid), 128 samples | 4 × 4 × 128 = 2,048 float ops + 16 atomic stores (writeHead pairs) |

At 44,100 Hz with a 128-sample block size, one full block runs every ~2.9 ms. The worst-case
scenario (4 sources × 4 receivers × 128 samples = 2,048 scalar float ops) executes in well under
1 µs on any modern core — this is dominated by cache misses, not arithmetic.

**Acceptable ceiling**: The `AudioToBuffer` system's performance overhead is bounded by:
- At most 4 × `MaxSlots` = 16 `pushBlock()` calls per block
- At most 16 × `numSamples` = 2,048 scalar float reads in `receiveAudioBuffer()` across all receivers
- 32 atomic operations per block (2 per pushBlock call × 16 maximum pushBlock calls)

This is smaller than a single reverb tail convolution and should not appear in profiler output.

**Rule**: If `processAudioRoute()` ever shows up above the noise floor in CPU profiling, the
fix is vectorisation of the pushBlock loop (SIMD memcpy of the scaled block), not architectural
changes.

### 5.3 Cache behaviour

`AudioRingBuffer::data[0]` and `data[1]` are `std::vector<float>` with ~186ms of samples at
44.1 kHz = 8,192 floats = 32,768 bytes per channel = ~64 KB per ring buffer. Four ring buffers
per engine = ~256 KB — larger than L1 cache on most cores.

This means `pushBlock()` and `readAt()` will incur L2/L3 cache misses on each call unless
the ring buffer is accessed sequentially. The write path (`pushBlock`) is sequential (ascending
address order), which is cache-friendly. The read path (`readAt`) in `receiveAudioBuffer()` reads
backward from `head` by up to `numSamples` positions — also sequential (descending), which is
cache-friendly for prefetch.

No additional optimization is required for Phase 3.

---

## 6. Phase 3 Implementation Roadmap

### 6.1 Ordering constraint

The `IAudioBufferSink` interface must exist before `OpalEngine` can declare conformance and
before `MegaCouplingMatrix` can remove the `OpalEngine.h` include. All other items are
independent once the interface is in place.

### 6.2 File manifest

**New files**

| File | Purpose |
|------|---------|
| `Source/Core/IAudioBufferSink.h` | The pure virtual interface. No external dependencies beyond `AudioRingBuffer.h`. |

**Modified files**

| File | Change |
|------|--------|
| `Source/Core/MegaCouplingMatrix.h` | Replace `#include "../Engines/Opal/OpalEngine.h"` with `#include "IAudioBufferSink.h"`. In `processAudioRoute()`: replace `dynamic_cast<OpalEngine*>` with `dynamic_cast<IAudioBufferSink*>` and add `getNumInputSlots()` guard. In `addRoute()`: add duplicate-route guard and cycle detection. In `removeUserRoute()` and `clearRoutes()`: maintain `audioBufferAdjacency`. Add `audioBufferAdjacency` member. Add `wouldCreateCycle()` private method. |
| `Source/Engines/Opal/OpalEngine.h` | Inherit from `IAudioBufferSink`. Mark `getGrainBuffer`, `getNumInputSlots`, `receiveAudioBuffer` as `override`. Add `FreezeState` enum and `freezeState` member. Replace direct `pFreeze` boolean read in `renderBlock()` with `tickFreezeStateMachine()` call. Update `receiveAudioBuffer()` to honour the `frozen` parameter (remove `(void)frozen`). Add CC#64 handling in MIDI callback. |
| `Docs/audio_to_buffer_phase3_spec.md` | This file — created in Round 12D. |

### 6.3 Step-by-step implementation order

**Step 1** — Create `Source/Core/IAudioBufferSink.h` with the three pure virtual methods.
Verify it compiles standalone (no JUCE headers needed, only `AudioRingBuffer.h`).

**Step 2** — Update `OpalEngine.h`:
  - Add `: public IAudioBufferSink` to the class declaration.
  - Mark the three methods `override`.
  - Confirm the build still passes (`cmake --build build`).

**Step 3** — Update `MegaCouplingMatrix.h`:
  - Swap the include.
  - Replace the downcast.
  - Add the `getNumInputSlots()` guard.
  - Confirm the build still passes.

**Step 4** — Add cycle detection to `MegaCouplingMatrix.h`:
  - Add `audioBufferAdjacency` member.
  - Add `wouldCreateCycle()` private method.
  - Update `addRoute()`, `removeUserRoute()`, `clearRoutes()`.
  - Write a minimal non-audio-thread test: register A→B, confirm B→A is rejected.

**Step 5** — Implement FREEZE state machine in `OpalEngine.h`:
  - Add `FreezeState` enum and `freezeState` member.
  - Add `tickFreezeStateMachine()` method.
  - Call it at the top of `renderBlock()`.
  - Update `frozen` derivation.
  - Update `receiveAudioBuffer()` to use the `frozen` parameter.
  - Test: toggle `opal_freeze` parameter, verify grain buffer stops advancing.

**Step 6** — Add CC#64 MIDI trigger:
  - Wire in the MIDI callback.
  - Test: sustain pedal on/off should mirror `opal_freeze` toggle.

**Step 7** — Write an integration test (or manually test in standalone):
  - Load OVERWORLD in slot 0, OPAL in slot 1.
  - Register `AudioToBuffer` route 0→1.
  - Attempt to register 1→0 — verify rejection logged.
  - Verify OVERWORLD audio appears in OPAL grain buffer (listen: `opal_externalMix` at 1.0).
  - Freeze: verify grain buffer holds snapshot.
  - Resume: verify seamless join at shadow position.

**Step 8** — Update `Docs/prism_sweep_index.md` with Round 12D completion note.

### 6.4 Future receiver engines

Any engine that wants to receive `AudioToBuffer` routes in a future session:
1. Include `../../Core/IAudioBufferSink.h`.
2. Inherit from `IAudioBufferSink` in addition to `SynthEngine`.
3. Declare `std::array<AudioRingBuffer, N> inputBuffers` (N = number of desired slots, ≤ 4).
4. Implement `getGrainBuffer(slot)`, `getNumInputSlots()`, and `receiveAudioBuffer(...)`.
5. Call `inputBuffers[i].prepare(sampleRate, durationSeconds)` from `prepareToPlay()`.
6. No changes to `MegaCouplingMatrix.h` are required.

---

## 7. Open Questions for Phase 4

These items are acknowledged but explicitly deferred beyond Phase 3:

| Item | Notes |
|------|-------|
| UI slot assignment | No UI currently exposes which OPAL input slot a source writes to. Requires coupling strip UX for slot selection (drag-to-slot or dropdown). |
| `opal_externalMix` as mod destination | Would allow LFO-driven source cross-fading ("source morphing"). Low priority until Phase 3 FREEZE is validated. |
| Per-slot `opal_externalMix` values | Currently all 4 slots share one mix ratio. A 4-slot fader panel would require 4 new parameters (`opal_externalMix0`–`opal_externalMix3`). |
| AudioToBuffer normalled presets | OVERWORLD→OPAL and DRIFT→OPAL coupling presets demonstrating the "Time Telescope" coupling type. Blocked on Phase 3 FREEZE working end-to-end. |
| Alternative ring buffer durations | `kOpalExternalBufferSeconds` is hardcoded at ~186ms. A user-facing "buffer length" knob would allow longer freeze windows for slow, ambient work. |

---

## Appendix A — Data Flow After Phase 3

```
Source engine renderBlock()
       │ getSampleForCoupling(0/1, n)
       ▼
MegaCouplingMatrix::processAudioRoute()
  [cycle guard: wouldCreateCycle() on addRoute() (message thread, not here)]
  [self-route guard: sourceSlot == destSlot → skip]
  [fill couplingBuffer L/R from source coupling cache]
  [dynamic_cast<IAudioBufferSink*>(dest)]  ← Phase 3: interface, not OpalEngine
  [getNumInputSlots() guard]
  [getGrainBuffer(route.sourceSlot) → AudioRingBuffer*]
  [AudioRingBuffer::pushBlock(L, R, numSamples, amount)]
       │                                     ↑
       │                              (frozen=true → shadow only)
       │
       ▼
IAudioBufferSink::inputBuffers[slot]  (AudioRingBuffer — ~186ms stereo ring)
       │
Receiver engine renderBlock()
  [tickFreezeStateMachine()]          ← Phase 3: state machine, not raw pFreeze
  [receiveAudioBuffer() for each slot, passing frozen state]
  [per-sample: srcSample = internal*(1-mix) + external*mix]
  [grain/looper/other write path (receiver-specific)]
       │
       ▼
Receiver engine stereo output
```

---

## Appendix B — Cycle Detection Example

```
Slots: 0=OVERWORLD  1=OPAL  2=DRIFT  3=empty

Route 1: OVERWORLD(0) → OPAL(1)         adjacency: {0:[1]}    — ACCEPTED
Route 2: OPAL(1) → DRIFT(2)             adjacency: {0:[1], 1:[2]} — ACCEPTED
Route 3: DRIFT(2) → OVERWORLD(0)        DFS from dstSlot=0:
                                           stack=[0], visited={}
                                           pop 0 → 0 == srcSlot(2)? No.
                                           neighbors of 0 = [1] → stack=[1]
                                           pop 1 → 1 == srcSlot(2)? No.
                                           neighbors of 1 = [2] → stack=[2]
                                           pop 2 → 2 == srcSlot(2)? YES → CYCLE
                                         — REJECTED. Log emitted. Route not registered.

Route 3 (retry): DRIFT(2) → slot 3     adjacency: {0:[1], 1:[2], 2:[3]} — ACCEPTED
```

---

*End of spec — Phase 3 implementation may proceed after this document is reviewed.*
