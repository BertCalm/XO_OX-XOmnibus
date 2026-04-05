# XOceanus Threading Model

> **Status**: Canonical reference. Written 2026-04-04 per Ghost Council mandate (QDD Level 5).
> **Scope**: All audio-thread / message-thread interactions in the XOceanus runtime.

## 1. Thread Roles

| Thread | JUCE Name | Responsibilities | Real-time? |
|--------|-----------|-----------------|------------|
| **Audio** | `processBlock()` callback | DSP, voice allocation, coupling matrix, shaper processing, waveform FIFO push | YES — no allocation, no blocking, no I/O |
| **Message** | Message thread / Timer callbacks | UI rendering, parameter changes, engine load/unload, preset management, shaper swap | NO — may allocate, may block briefly |
| **Background** | `juce::ThreadPool` / `std::async` | Preset scanning, export rendering, AI subsystem | NO — fully concurrent |

### The Cardinal Rule

> **The audio thread must never wait for the message thread.** All synchronization primitives used on the audio thread must be non-blocking (`try_lock`, `atomic`, lock-free structures). The message thread may briefly wait for the audio thread (SpinLock) but never the reverse.

## 2. Object Ownership Model

### 2.1 Exclusive Ownership (Current)

Objects owned by exactly one entity. Transfer is explicit.

| Object | Owner | Created By | Destroyed By |
|--------|-------|-----------|-------------|
| `SynthEngine` instances | `engines[]` atomic shared_ptr array | Message thread (loadEngine) | Crossfade struct (50ms grace, message thread cleanup) |
| `ShaperEngine` instances | `ShaperRegistry` slot unique_ptr | Message thread (loadInsert/loadBus) | **HAZARD** — see §4.1 |
| `CouplingRoute` configurations | `MegaCouplingMatrix` route vector | Message thread (resolveRoutes) | Message thread (route rebuild) |
| Parameter values | `AudioProcessorValueTreeState` | Plugin initialization | Plugin destruction |
| `WaveformFifo` | `XOceanusProcessor` per-slot array | Processor constructor | Processor destructor |

### 2.2 Shared Access Pattern

When both threads need access to the same object:

```
┌─────────────────────────────────────────────────────────┐
│                   CORRECT PATTERN                        │
│                                                          │
│  Message thread:                                         │
│    1. Create new object                                  │
│    2. Prepare/initialize it                              │
│    3. Acquire lock                                       │
│    4. Swap shared_ptr (old → staging, new → slot)        │
│    5. Release lock                                       │
│    6. Queue old object for deferred deletion              │
│                                                          │
│  Audio thread:                                           │
│    1. Acquire try_lock (skip block if contended)         │
│    2. Copy shared_ptr (ref count increment, NOT raw ptr) │
│    3. Release lock                                       │
│    4. Process through local shared_ptr copy              │
│    5. Local copy destroyed at block end (ref count dec)  │
│                                                          │
│  Deferred deletion thread/callback:                      │
│    - Drains a lock-free FIFO of expired shared_ptrs      │
│    - Destructor runs here, NEVER on the audio thread     │
└─────────────────────────────────────────────────────────┘
```

### 2.3 The Deferred Deletion Queue

```cpp
// Lock-free SPSC FIFO for deferred destruction.
// Audio thread pushes expired shared_ptrs.
// Message thread drains on timer (~100ms interval).
template <typename T, size_t Capacity = 32>
struct DeferredDeletionQueue {
    std::array<std::shared_ptr<T>, Capacity> ring;
    std::atomic<size_t> writeHead{0};
    std::atomic<size_t> readHead{0};

    // Audio thread: push an expired shared_ptr for later destruction
    bool push(std::shared_ptr<T> expiring) noexcept { ... }

    // Message thread: drain all expired objects (destructors run here)
    void drain() { ... }
};
```

## 3. Invariants

### 3.1 Block-Boundary Atomicity

> **All configuration changes (engine swap, coupling route rebuild, shaper swap) must be atomic at the audio block boundary.** The audio thread processes an entire block with either the old or new configuration — never a partial state.

Implementation: Double-buffer or atomic pointer swap. The audio thread reads the configuration pointer once at block start and uses it for the entire block.

### 3.2 No Raw Pointers Across Lock Boundaries

> **Never extract a raw pointer under a lock and use it after the lock is released.** This is the ShaperRegistry UAF pattern. The audio thread must hold a `shared_ptr` copy (not `T*`) for the duration of processing.

### 3.3 No Allocation on the Audio Thread

> **The audio thread must never call `new`, `delete`, `malloc`, `free`, or any function that may allocate.** This includes:
> - `std::string` construction or comparison
> - `std::vector::push_back` (may reallocate)
> - `std::shared_ptr` destructor when ref count reaches zero (runs `delete`)
> - `juce::String` construction
> - `getRawParameterValue()` by string key (allocates juce::String for comparison)

Mitigation for shared_ptr: Use the Deferred Deletion Queue (§2.3). The audio thread's local shared_ptr copy must never be the last reference.

### 3.4 Parameter Access Pattern

> **Parameter pointers (`std::atomic<float>*`) must be cached once during `attachParameters()` or `prepare()`.** Per-block access reads from cached pointers via `atomic::load(memory_order_relaxed)`. String-based lookup (`getRawParameterValue("id")`) is forbidden on the audio thread.

Correct:
```cpp
// In attachParameters() — called once at setup
pCutoff = apvts.getRawParameterValue("snap_filterCutoff");

// In renderBlock() — called every block
float cutoff = pCutoff->load(std::memory_order_relaxed);
```

Incorrect:
```cpp
// In renderBlock() — allocates juce::String every block!
float cutoff = *apvts.getRawParameterValue("snap_filterCutoff");
```

### 3.5 Crossfade Protocol for Hot-Swap

When replacing an engine or shaper:

1. **Message thread**: Create new object, prepare it, swap into slot (atomic)
2. **Audio thread**: Detects swap at next block start. Crossfades old→new over 50ms.
3. **Message thread**: After crossfade completes, old object enters deferred deletion.
4. **Invariant**: Old object is alive for at least 50ms after swap. Audio thread never holds the last reference.

## 4. Known Violations (to be fixed)

### 4.1 ShaperRegistry UAF (#751 — CRITICAL)

**Current code** (ShaperRegistry.h:168-184):
```cpp
ShaperEngine* shaper = nullptr;
{
    SpinLock::ScopedTryLockType tryLock(slotsLock);
    shaper = inserts[slot].get();  // raw pointer extracted
}
// Lock released — message thread can destroy inserts[slot] here
shaper->processBlock(buffer, midi, numSamples);  // UAF
```

**Required fix**: Replace `unique_ptr<ShaperEngine>` slots with `shared_ptr<ShaperEngine>`. Audio thread copies `shared_ptr` under lock, processes through copy. Add `DeferredDeletionQueue` to prevent audio-thread destruction.

### 4.2 MegaCouplingMatrix Swap (#755 — HIGH)

**Current**: Route configuration is modified on message thread. Audio thread may read a partially-updated route list.

**Required fix**: Double-buffer the route configuration. Audio thread reads from a `std::atomic<RouteConfig*>` that is swapped atomically at block boundary.

### 4.3 Overlap ParamSnapshot (#754 — HIGH)

**Current**: 41 `getRawParameterValue()` string lookups per audio block.

**Required fix**: Cache `std::atomic<float>*` pointers in `attachParameters()`, read via `atomic::load()` in `update()`.

### 4.4 firstBreathEnginePtr_ (#756 — HIGH)

**Current**: Raw pointer comparison across blocks. UB after crossfade frees old engine.

**Required fix**: Generation counter pattern. Increment counter on engine swap, compare counters instead of pointers.

## 5. Verification Checklist

When reviewing new code for thread safety:

- [ ] Does any audio-thread code extract a raw pointer from a shared/unique_ptr? → Use shared_ptr copy
- [ ] Does any audio-thread code construct a `std::string` or `juce::String`? → Cache at prep time
- [ ] Does any audio-thread code call `new`/`delete`/`malloc`/`free`? → Use pre-allocated buffers
- [ ] Does any lock-holding code call DSP processing? → Extract shared_ptr, release lock, then process
- [ ] Does any configuration change happen outside block boundaries? → Double-buffer
- [ ] Does any shared_ptr destructor run on the audio thread? → Add to DeferredDeletionQueue
- [ ] Are parameter pointers cached at setup time? → Verify no string lookups in renderBlock
