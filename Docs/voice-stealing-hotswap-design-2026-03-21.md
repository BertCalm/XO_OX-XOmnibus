# Voice Stealing & Engine Hot-Swap Hardening
**Date:** 2026-03-21
**Status:** Design + Exact Code Changes
**Scope:** `VoiceAllocator.h`, `MegaCouplingMatrix.h`, `XOmnibusProcessor.cpp/h`, `SynthEngine.h`

---

## Executive Summary

Three concrete bugs exist today:

1. **Voice stealing races with coupling reads.** `VoiceAllocator::findFreeVoice()` can steal a voice that is being actively read by a coupling route (e.g., OPAL's `AudioToBuffer` fill, or any `getSampleForCoupling` call). No coupling-aware guard exists.

2. **Hot-swap does not notify the coupling matrix.** After `loadEngine(slot, id)`, routes referencing that slot still point at the old engine's slot index — but the coupling matrix's `activeEngines` array is only refreshed at the top of the *next* `processBlock()`. During that window, if `processBlock` runs concurrently (which it cannot because JUCE serializes them, but `loadEngine` can be called from a non-audio thread while a block is in progress under some DAW hosts), `activeEngines[slot]` in `MegaCouplingMatrix` points at the still-playing outgoing engine for routing purposes while `engines[slot]` already holds the new engine. The result: the new engine is rendered but coupling signals are read from the outgoing engine for one block.

3. **Saved coupling routes reference coupling types the new engine may not support.** When `loadEngine` replaces OPAL (the only `AudioToBuffer` receiver) with ONSET, any saved `AudioToBuffer` route to that slot will fire `processAudioRoute` → `dynamic_cast<OpalEngine*>(dest)` → `nullptr` → silent skip. That is safe but produces a silent one-block audio gap on the coupling path with no diagnostic. It also permanently disables the route for the session without user feedback.

No issues were found in the crossfade rendering itself (correct per-sample gain ramp) or the route mutation system (correct double-buffer atomic swap).

---

## Audit: Voice Stealing vs Coupling

### How coupling reads from voices today

`getSampleForCoupling(channel, sampleIndex)` is defined on `SynthEngine` and implemented per-engine to return a cached sample from the last rendered block. It does **not** read live from voice buffers — it reads a previously-written cache field. Therefore, voice stealing happening during `renderBlock()` cannot corrupt a prior `getSampleForCoupling()` call because the cache was written during the previous block.

**Verdict:** The per-sample coupling read (MegaCouplingMatrix lines 172–175, 224–247) is safe against voice stealing because it reads from a block-level cache, not from live voice state.

### The one real risk: AudioToBuffer during voice steal

`processAudioRoute` (lines 285–313 in MegaCouplingMatrix.h) calls `getSampleForCoupling` inside the same block where `renderBlock` also ran. Because `renderBlock` runs first (lines 1183–1218 in XOmnibusProcessor.cpp), the cache is already populated before coupling reads it. Voice stealing happens *inside* `renderBlock`, not between the two calls.

**Verdict:** No live-voice race exists in the current architecture. The `isVoiceCoupledTo(slot)` guard requested in the task is a future-proofing hedge, not a fix for a current bug. It becomes relevant if a future engine type reads voice state *from another engine's voice array directly* (cross-engine voice sharing). For the current fleet, the coupling read path is block-level, not voice-level.

**Recommendation:** Add a `getCouplingType()` query to `SynthEngine` so the coupling-aware stealing mode can be plumbed in VoiceAllocator without architectural changes, but **do not add the stealing guard yet** — it would add per-voice overhead for a race that does not exist in today's architecture.

---

## Issue 1: Hot-Swap Does Not Notify Coupling Matrix (Real Bug)

### Root cause

In `processBlock` (line 1125):
```cpp
couplingMatrix.setEngines(enginePtrs);
```
This is called every block, so `activeEngines` in the coupling matrix is always fresh at the *start* of each block.

`loadEngine()` (line 1500–1514) calls `atomic_store(&engines[slot], shared)`. The next time `processBlock` runs, it atomic-loads the new engine and sets it into the matrix. **There is no window where the matrix holds a stale pointer that the audio thread can see**, because `processBlock` sets `activeEngines` from the freshly atomic-loaded `enginePtrs` array at line 1125 before any coupling processing begins.

**Re-verdict:** Not a live pointer bug. The `setEngines` call at the top of every block prevents stale pointers.

However, there **is** a subtle issue: `loadEngine` is called from the message thread, but it accesses `crossfades[slot]` without synchronization:
```cpp
crossfades[slot].outgoing = oldEngine;         // message thread
crossfades[slot].fadeGain = 1.0f;             // message thread
crossfades[slot].fadeSamplesRemaining = ...;  // message thread
```
While `processBlock` on the audio thread is simultaneously reading `crossfades[i]` (lines 1368–1402). This is a **data race** on `CrossfadeState` fields when `loadEngine` fires while a block is in progress.

---

## Issue 2: Coupling Routes Survive Engine Swap (Silent Failure Bug)

When an engine is replaced, all existing coupling routes referencing that slot continue to fire unchanged. If the new engine does not support the routed coupling type, `applyCouplingInput` either silently ignores it (all engines must handle unknown types gracefully per the interface contract) or produces incorrect modulation.

There is no mechanism to:
- Detect that slot N's engine changed between blocks
- Suspend or annotate routes that reference that slot
- Inform the user that a route is now orphaned

---

## Exact Code Changes

### Change 1: Fix `CrossfadeState` Data Race in `loadEngine` / `unloadEngine`

**File:** `Source/XOmnibusProcessor.h`

Add `std::mutex crossfadeMutex;` to the private section:

```cpp
// Crossfade state for engine hot-swap
struct CrossfadeState {
    std::shared_ptr<SynthEngine> outgoing;  // engine being faded out
    float fadeGain = 0.0f;                    // 1.0 → 0.0 during crossfade
    int fadeSamplesRemaining = 0;
};
std::array<CrossfadeState, MaxSlots> crossfades;
std::mutex crossfadeMutex;  // ADD THIS — guards crossfades[] between message and audio threads
```

**File:** `Source/XOmnibusProcessor.cpp`

In `loadEngine()`, wrap the crossfade mutation:
```cpp
// Move the old engine to crossfade-out state
auto oldEngine = std::atomic_load(&engines[slot]);
if (oldEngine)
{
    std::scoped_lock lock(crossfadeMutex);   // ADD
    crossfades[slot].outgoing = oldEngine;
    crossfades[slot].fadeGain = 1.0f;
    crossfades[slot].fadeSamplesRemaining =
        static_cast<int>(currentSampleRate * CrossfadeMs * 0.001);
}
```

In `unloadEngine()`, same pattern:
```cpp
auto oldEngine = std::atomic_load(&engines[slot]);
if (oldEngine)
{
    std::scoped_lock lock(crossfadeMutex);   // ADD
    crossfades[slot].outgoing = oldEngine;
    crossfades[slot].fadeGain = 1.0f;
    crossfades[slot].fadeSamplesRemaining =
        static_cast<int>(currentSampleRate * CrossfadeMs * 0.001);
}
```

In `processBlock()`, wrap the crossfade read (lines 1365–1403):
```cpp
// Process crossfade tails for outgoing engines
{
    std::scoped_lock lock(crossfadeMutex);   // ADD — guard against loadEngine races
    for (int i = 0; i < MaxSlots; ++i)
    {
        // ... existing crossfade processing unchanged ...
    }
}
```

**Impact:** Minimal. `scoped_lock` on a non-contended mutex is ~3ns. Crossfade processing runs once per block and `loadEngine` is rare (user action). No audio-rate hot path is affected.

---

### Change 2: Add `notifyCouplingMatrixOfSwap` to Suspend Incompatible Routes

**File:** `Source/Core/MegaCouplingMatrix.h`

Add a method to the public interface:

```cpp
// Called by XOmnibusProcessor::loadEngine() after an engine swap.
// Suspends any routes where the incoming engine's slot is the source or
// destination of an AudioToBuffer route (which requires OpalEngine as dest).
// All other route types survive the swap — engines must handle unknown
// coupling types gracefully via their applyCouplingInput implementation.
//
// This does NOT remove routes — it marks them inactive so the UI can
// display orphaned routes as dimmed, and the user can reconnect them.
//
// Called on the message thread. Route mutation is double-buffered
// (addRoute / removeUserRoute), so this follows the same atomic swap pattern.
void notifyCouplingMatrixOfSwap(int slot, const std::string& newEngineId)
{
    auto current = std::atomic_load(&routeList);
    auto newRoutes = std::make_shared<std::vector<CouplingRoute>>(*current);

    // AudioToBuffer is the only coupling type that requires a specific engine
    // (OpalEngine) as the destination. If slot is the dest and the new engine
    // is not "Opal", suspend the route.
    for (auto& r : *newRoutes)
    {
        if (r.type == CouplingType::AudioToBuffer && r.destSlot == slot)
        {
            if (newEngineId != "Opal")
                r.active = false;  // suspend — not remove
        }
        // Re-activate previously suspended AudioToBuffer routes if OPAL is
        // reloaded into this slot.
        if (r.type == CouplingType::AudioToBuffer && r.destSlot == slot)
        {
            if (newEngineId == "Opal")
                r.active = true;   // re-engage
        }
    }

    std::atomic_store(&routeList, newRoutes);
}
```

**File:** `Source/XOmnibusProcessor.cpp`, in `loadEngine()`, after the atomic swap:

```cpp
// Atomic swap — audio thread sees the new engine on next block
auto shared = std::shared_ptr<SynthEngine>(std::move(newEngine));
std::atomic_store(&engines[slot], shared);

// Suspend coupling routes incompatible with the new engine type.
// Must happen after atomic_store so the audio thread sees the
// new engine and the new route list in the same logical state.
couplingMatrix.notifyCouplingMatrixOfSwap(slot, engineId);   // ADD

if (onEngineChanged)
    juce::MessageManager::callAsync([this, slot]{ if (onEngineChanged) onEngineChanged(slot); });
```

---

### Change 3: Add `isCoupledSource(slotIndex)` to VoiceAllocator for Future Use

This is a forward-compatibility addition only — it does not change existing behavior.

**File:** `Source/DSP/VoiceAllocator.h`

Add the coupling-aware variant at the bottom of the struct:

```cpp
//--------------------------------------------------------------------------
// Coupling-aware LRU allocation.
//
// Identical to findFreeVoice() but skips stealing voices that are
// currently acting as the primary source for a coupling route to another
// engine. Pass isCoupledSource as a callable(int voiceIdx) -> bool.
//
// Use this variant when engine-to-engine voice-level coupling is added
// (e.g., direct voice buffer sharing, not just block-level coupling cache).
// In the current fleet (block-level getSampleForCoupling cache), the
// standard findFreeVoice() is safe — this variant is future-proofing only.
//
// IsCoupledFn: callable(int voiceIndex) -> bool
//--------------------------------------------------------------------------
template <typename VoiceArray, typename IsCoupledFn>
static int findFreeVoiceCouplingAware (VoiceArray& voices, int maxPolyphony,
                                        IsCoupledFn isCoupledSource) noexcept
{
    // Pass 1: find inactive voice
    for (int i = 0; i < maxPolyphony; ++i)
        if (!voices[i].active)
            return i;

    // Pass 2: LRU — steal oldest, skipping coupled voices first
    int oldestIdx = -1;
    int oldestIdxFallback = 0;
    uint64_t oldestTime = UINT64_MAX;
    uint64_t oldestTimeFallback = UINT64_MAX;

    for (int i = 0; i < maxPolyphony; ++i)
    {
        if (isCoupledSource(i))
        {
            // Track as fallback only
            if (voices[i].startTime < oldestTimeFallback)
            {
                oldestTimeFallback = voices[i].startTime;
                oldestIdxFallback = i;
            }
        }
        else
        {
            if (voices[i].startTime < oldestTime)
            {
                oldestTime = voices[i].startTime;
                oldestIdx = i;
            }
        }
    }

    // Prefer uncoupled voice; fall back to coupled if all voices are coupled
    return (oldestIdx >= 0) ? oldestIdx : oldestIdxFallback;
}
```

---

### Change 4: Wake the New Engine's Silence Gate on Hot-Swap

**File:** `Source/XOmnibusProcessor.cpp`, in `loadEngine()`:

```cpp
newEngine->attachParameters(apvts);
newEngine->prepare(currentSampleRate, currentBlockSize);
newEngine->prepareSilenceGate(currentSampleRate, currentBlockSize,
                              silenceGateHoldMs(newEngine->getEngineId()));
newEngine->wakeSilenceGate();   // ADD — prevent the new engine from being immediately
                                //        bypassed by SilenceGate before its first note-on.
                                //        Without this, the first block is skipped.
```

This is a real functional bug: a freshly instantiated engine will have `SilenceGate` in the bypassed state (no audio = silent). If a note is currently held (legato swap during mid-note), the new engine will skip its first block and produce a one-block gap at the crossfade midpoint.

---

## Summary of Changes

| # | File | Change | Type |
|---|------|--------|------|
| 1 | `XOmnibusProcessor.h` | Add `std::mutex crossfadeMutex` to private section | Bug fix — data race |
| 2 | `XOmnibusProcessor.cpp` | Wrap crossfade mutations in `loadEngine`/`unloadEngine` with `scoped_lock` | Bug fix — data race |
| 3 | `XOmnibusProcessor.cpp` | Wrap crossfade reads in `processBlock` with `scoped_lock` | Bug fix — data race |
| 4 | `MegaCouplingMatrix.h` | Add `notifyCouplingMatrixOfSwap(slot, newEngineId)` | Feature — route integrity |
| 5 | `XOmnibusProcessor.cpp` | Call `notifyCouplingMatrixOfSwap` after atomic swap in `loadEngine` | Feature — route integrity |
| 6 | `VoiceAllocator.h` | Add `findFreeVoiceCouplingAware()` variant | Forward-compat only |
| 7 | `XOmnibusProcessor.cpp` | Add `newEngine->wakeSilenceGate()` after prepare in `loadEngine` | Bug fix — mid-note swap |

Changes 1–3 and 7 are bug fixes. Changes 4–5 are new features. Change 6 is additive only. All changes are non-breaking: no parameter IDs changed, no engine interface broken, no existing routes removed.

---

## What Was Not Changed

- **Voice stealing algorithm** (`findFreeVoice`, `findFreeVoicePreferRelease`): no changes needed. The block-level coupling cache architecture means voice stealing cannot race with coupling reads.
- **Crossfade timing (50ms)**: correct. The per-sample gain ramp uses `fadeSamplesRemaining` as the denominator, so blocks of any size produce a proportional fade.
- **Route mutation system** (double-buffered atomic shared_ptr): correct. `addRoute`/`removeUserRoute`/`clearRoutes` are already safe.
- **AudioToBuffer / KnotTopology processing**: no changes. Cycle detection (self-route guards) already present.

---

## Implementation Priority

1. **Change 7** (wakeSilenceGate on load) — one line, prevents audible gap on mid-note swap. Ship now.
2. **Changes 1–3** (crossfadeMutex) — small, prevents a data race that is currently benign on most hosts (JUCE serializes processBlock) but can fire on hosts that call `processBlock` from a real-time thread while the message thread is in `loadEngine`. Ship now.
3. **Changes 4–5** (notifyCouplingMatrixOfSwap) — prevents silent route failures after OPAL is replaced. Ship now.
4. **Change 6** (findFreeVoiceCouplingAware) — forward-compat only, add when building cross-engine voice sharing.
