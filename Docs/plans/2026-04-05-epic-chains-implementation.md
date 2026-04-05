# Epic Chains Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement 4 Epic Chain FX engines (ONRUSH, OMNISTEREO, OBLITERATE, OBSCURITY) and a unified 3-slot FX assignment system replacing fixed MasterFXChain positions.

**Architecture:** EpicChainSlotController manages 3 FXSlot instances, each pre-allocating all 10 chain variants. Only the active chain per slot runs processBlock(). Chain swaps use 50ms crossfade. Parameter IDs use slot{N}_{prefix}_{param} convention with hidden aliases for backward compatibility.

**Tech Stack:** C++17, JUCE 8, CMake/Ninja, inline header-only DSP

**Spec:** `Docs/specs/2026-04-05-epic-chains-fx-design.md`

---

## Codebase Reference (read before implementing)

### Gold Standard Pattern: AquaticFXSuite
`Source/DSP/Effects/AquaticFXSuite.h` is the canonical model for all chain implementations:
- `static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)` — registers params using `std::make_unique<juce::AudioParameterFloat>()` with `juce::NormalisableRange<float>`
- `void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts)` — caches `std::atomic<float>*` pointers from `apvts.getRawParameterValue()`
- `void processBlock(float* L, float* R, int numSamples, ...)` — reads all params once at block start (ParamSnapshot), then processes stages with `if (mix < 0.0001f) return;` early-exit per stage
- `void prepare(double sampleRate, int samplesPerBlock)` — pre-allocates all buffers, sets `sr`
- All DSP inline in `.h`. Corresponding `.cpp` is always just `#include "ThisFile.h"`

### MasterFXChain structure (Source/Core/MasterFXChain.h)
Current chain layout, stages 19.6–19.8:
- Stage 19.6: `AquaticFXSuite aquaticFX` — `aquaticFX.processBlock(L, R, numSamples, bpm)`
- Stage 19.7: `MathFXChain mathFX` — inline processBlock call with all `mfx_` params
- Stage 19.8: `BoutiqueFXChain boutiqueFX` — inline processBlock call with all `bfx_` params
- Singularity FX: `fXOnslaught onslaught` (stage 14.5), `fXObscura obscura` (stage 5.5), `fXOratory oratory` (stage 8.5) — scattered throughout the chain, NOT in a dedicated block

`cacheParameterPointers()` currently caches pointers for:
- `master_onslFlowRate`, `master_onslFlowDepth`, `master_onslThreshold`, `master_onslModDepth`, `master_onslModRate`, `master_onslDecay`, `master_onslSCHP`, `master_onslMix` (Onslaught — 8 params)
- `master_obsThreshold`, `master_obsHold`, `master_obsRelease`, `master_obsErosion`, `master_obsSubHarm`, `master_obsSaturation`, `master_obsDecimate`, `master_obsResonance`, `master_obsTone`, `master_obsPatina`, `master_obsMix` (Obscura — 11 params)
- `master_oraPattern`, `master_oraSyllable`, `master_oraAccent`, `master_oraSpread`, `master_oraFeedback`, `master_oraDamping`, `master_oraDampRes`, `master_oraDrift`, `master_oraMix` (Oratory — 9 params)
- `mfx_*` (16 MathFX params) — cached at stage 19.7 block
- `bfx_*` (25 BoutiqueFX params) — cached at stage 19.8 block

### CMakeLists.txt structure
- Header-only files (`.h`) are listed as source headers at lines ~141–355
- Engine `.cpp` stubs are listed at lines ~176–494
- `Source/Core/` files: listed as headers only (lines 141–148) — no `.cpp` files in Core currently
- `Source/DSP/Effects/` files: listed as headers only (lines 159–174, 351–353)
- New `.cpp` stubs must be added to the sources list (find the nearest existing `.cpp` entry and add after it)

### Namespace
All code lives in `namespace xoceanus { ... }`

---

## Phase 1: Infrastructure

### Task 1: EpicChainSlotController skeleton

**Files to create:**
- `Source/Core/EpicChainSlotController.h`
- `Source/Core/EpicChainSlotController.cpp`

**Goal:** Compile-verified skeleton with crossfade logic and passthrough. No chain instances yet — those come in Task 3. Uses a dispatch switch that returns early (passthrough) for all non-Off ChainID values.

- [ ] **Step 1.1** — Create `Source/Core/EpicChainSlotController.h` with the following complete code:

```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cstring>

namespace xoceanus
{

//==============================================================================
// EpicChainSlotController — 3 fully-assignable FX chain slots.
//
// Replaces the fixed AquaticFXSuite / MathFXChain / BoutiqueFXChain /
// Singularity FX positions in MasterFXChain with a unified assignable system.
//
// Each slot holds all 10 chain instances (pre-allocated in prepare()).
// Only the active chain per slot runs processBlock() per audio block.
// Chain swaps trigger a 50ms linear crossfade.
//
// Parameter convention: slot{N}_{chainPrefix}_{paramName}
//   Control:  slot1_chain, slot1_mix, slot1_bypass  (× 3 slots = 9 params)
//   Chains:   slot1_aqua_fathom_mix, slot2_onr_swellThresh, etc.
//
// See Docs/specs/2026-04-05-epic-chains-fx-design.md for full spec.
//==============================================================================
class EpicChainSlotController
{
public:
    static constexpr int kNumSlots  = 3;
    static constexpr int kNumChains = 10;

    enum ChainID : int
    {
        Off        = 0,
        Aquatic    = 1,
        Math       = 2,
        Boutique   = 3,
        Onslaught  = 4,
        Obscura    = 5,
        Oratory    = 6,
        Onrush     = 7,
        Omnistereo = 8,
        Obliterate = 9,
        Obscurity  = 10
    };

    EpicChainSlotController() = default;

    //--------------------------------------------------------------------------
    /// Prepare all slots and all chain instances.
    /// Must be called before processBlock(). No heap allocation in processBlock.
    void prepare(double sampleRate, int maxBlockSize);

    //--------------------------------------------------------------------------
    /// Process 3 slots in series. buffer must be stereo (2 channels).
    /// Reads slot{N}_chain, slot{N}_mix, slot{N}_bypass from cached param pointers.
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples,
                      double bpm = 0.0, double ppqPosition = -1.0);

    //--------------------------------------------------------------------------
    /// Register all slot control parameters + all chain parameters (all 3 slots).
    /// Called from XOceanusProcessor::createParameterLayout().
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout);

    //--------------------------------------------------------------------------
    /// Cache all parameter pointers once after prepareToPlay().
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts);

    //--------------------------------------------------------------------------
    /// Reset all chain states (called from MasterFXChain::reset()).
    void reset();

private:
    double sr_         = 44100.0;
    int    blockSize_  = 512;

    //==========================================================================
    // FXSlot — one assignable slot holding all 10 chain instances.
    //==========================================================================
    struct FXSlot
    {
        ChainID activeChain   = Off;
        ChainID pendingChain  = Off;
        float crossfadeProgress = 1.0f; // 1.0 = stable; 0.0 = crossfade just started

        // Crossfade scratch buffer (allocated in prepare)
        juce::AudioBuffer<float> crossfadeBuf;

        // ---- Chain instances will be added in Task 3 ----
        // (Forward-declared as forward-allocated placeholders for now)
        // AquaticFXSuite  aquatic;
        // MathFXChain     math;
        // BoutiqueFXChain boutique;
        // fXOnslaught     onslaught;
        // fXObscura       obscura;
        // fXOratory       oratory;
        // OnrushChain     onrush;
        // OmnistereoChain omnistereo;
        // ObliterateChain obliterate;
        // ObscurityChain  obscurity;

        // ---- Cached slot control param pointers (added per slot in cacheParameterPointers) ----
        std::atomic<float>* pChain  = nullptr;
        std::atomic<float>* pMix    = nullptr;
        std::atomic<float>* pBypass = nullptr;
    };

    std::array<FXSlot, kNumSlots> slots_;

    //--------------------------------------------------------------------------
    // Helper: dispatch processBlock to the correct chain instance.
    // Task 3 fills in the chain cases; for now all cases are passthrough.
    void dispatchChain(FXSlot& slot, ChainID chain,
                       float* L, float* R, int numSamples,
                       double bpm, double ppqPosition);

    //--------------------------------------------------------------------------
    // Helper: apply per-slot wet/dry mix.
    static void applyMix(float* dryL, float* dryR,
                         float* wetL, float* wetR,
                         int numSamples, float mix);

    //--------------------------------------------------------------------------
    // Compute crossfade increment per sample (50ms ramp).
    float crossfadeIncrement() const
    {
        // Clamp to avoid division by zero
        float xfSamples = static_cast<float>(sr_) * 0.05f; // 50ms
        return 1.0f / std::max(xfSamples, 1.0f);
    }
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void EpicChainSlotController::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;

    for (auto& slot : slots_)
    {
        slot.crossfadeBuf.setSize(2, maxBlockSize, false, true, true);
        slot.crossfadeProgress = 1.0f;
        slot.activeChain       = Off;
        slot.pendingChain      = Off;

        // ---- Chain prepare calls go here in Task 3 ----
    }
}

inline void EpicChainSlotController::reset()
{
    for (auto& slot : slots_)
    {
        slot.crossfadeProgress = 1.0f;
        // ---- Chain reset calls go here in Task 3 ----
    }
}

inline void EpicChainSlotController::processBlock(juce::AudioBuffer<float>& buffer,
                                                   int numSamples,
                                                   double bpm,
                                                   double ppqPosition)
{
    if (buffer.getNumChannels() < 2 || numSamples <= 0)
        return;

    const float xfInc = crossfadeIncrement();

    for (int slotIdx = 0; slotIdx < kNumSlots; ++slotIdx)
    {
        FXSlot& slot = slots_[slotIdx];

        // Read control params
        const float chainVal  = slot.pChain  ? slot.pChain->load(std::memory_order_relaxed)  : 0.0f;
        const float slotMix   = slot.pMix    ? slot.pMix->load(std::memory_order_relaxed)    : 1.0f;
        const float bypass    = slot.pBypass ? slot.pBypass->load(std::memory_order_relaxed) : 0.0f;

        if (bypass > 0.5f)
            continue; // slot bypassed — passthrough

        // Detect chain change
        ChainID requestedChain = static_cast<ChainID>(static_cast<int>(chainVal + 0.5f));
        requestedChain = static_cast<ChainID>(
            juce::jlimit(static_cast<int>(Off), static_cast<int>(Obscurity),
                         static_cast<int>(requestedChain)));

        if (requestedChain != slot.pendingChain && slot.crossfadeProgress >= 1.0f)
        {
            // New chain requested — initiate crossfade
            slot.pendingChain      = requestedChain;
            slot.crossfadeProgress = 0.0f;
        }

        if (slot.activeChain == Off && slot.pendingChain == Off)
            continue; // both Off — zero-cost passthrough

        float* L = buffer.getWritePointer(0);
        float* R = buffer.getWritePointer(1);

        if (slot.crossfadeProgress >= 1.0f)
        {
            // ---- Stable: process active chain ----
            if (slot.activeChain == Off)
                continue;

            // Save dry copy for wet/dry mix
            if (slotMix < 0.999f)
            {
                const int safeSamples = std::min(numSamples, slot.crossfadeBuf.getNumSamples());
                slot.crossfadeBuf.copyFrom(0, 0, buffer, 0, 0, safeSamples);
                slot.crossfadeBuf.copyFrom(1, 0, buffer, 1, 0, safeSamples);
            }

            dispatchChain(slot, slot.activeChain, L, R, numSamples, bpm, ppqPosition);

            if (slotMix < 0.999f)
            {
                applyMix(slot.crossfadeBuf.getWritePointer(0),
                         slot.crossfadeBuf.getWritePointer(1),
                         L, R, numSamples, slotMix);
            }
        }
        else
        {
            // ---- Crossfade active ----
            // outgoing = activeChain, incoming = pendingChain
            // crossfadeBuf = outgoing wet result
            // buffer = incoming wet result

            const int safeSamples = std::min(numSamples, slot.crossfadeBuf.getNumSamples());

            // Copy input into crossfadeBuf for outgoing chain
            slot.crossfadeBuf.copyFrom(0, 0, buffer, 0, 0, safeSamples);
            slot.crossfadeBuf.copyFrom(1, 0, buffer, 1, 0, safeSamples);

            // Outgoing chain processes crossfadeBuf
            if (slot.activeChain != Off)
                dispatchChain(slot, slot.activeChain,
                              slot.crossfadeBuf.getWritePointer(0),
                              slot.crossfadeBuf.getWritePointer(1),
                              safeSamples, bpm, ppqPosition);

            // Incoming chain processes buffer
            if (slot.pendingChain != Off)
                dispatchChain(slot, slot.pendingChain, L, R, safeSamples, bpm, ppqPosition);
            // else buffer stays as dry input (crossfading out to dry)

            // Blend per sample: buffer = incoming * progress + outgoing * (1 - progress)
            float* xfL = slot.crossfadeBuf.getWritePointer(0);
            float* xfR = slot.crossfadeBuf.getWritePointer(1);
            float progress = slot.crossfadeProgress;
            for (int i = 0; i < safeSamples; ++i)
            {
                L[i] = L[i] * progress + xfL[i] * (1.0f - progress);
                R[i] = R[i] * progress + xfR[i] * (1.0f - progress);
                progress = std::min(1.0f, progress + xfInc);
            }
            slot.crossfadeProgress = progress;

            if (slot.crossfadeProgress >= 1.0f)
            {
                slot.activeChain      = slot.pendingChain;
                slot.crossfadeProgress = 1.0f;
            }
        }
    }
}

inline void EpicChainSlotController::dispatchChain(FXSlot& /*slot*/, ChainID chain,
                                                    float* /*L*/, float* /*R*/,
                                                    int /*numSamples*/,
                                                    double /*bpm*/,
                                                    double /*ppqPosition*/)
{
    // Task 3 fills this in.
    // For now: all chains are passthrough (input unchanged).
    (void)chain;
}

inline void EpicChainSlotController::applyMix(float* dryL, float* dryR,
                                               float* wetL, float* wetR,
                                               int numSamples, float mix)
{
    const float dry = 1.0f - mix;
    for (int i = 0; i < numSamples; ++i)
    {
        wetL[i] = wetL[i] * mix + dryL[i] * dry;
        wetR[i] = wetR[i] * mix + dryR[i] * dry;
    }
}

inline void EpicChainSlotController::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using AP  = juce::AudioParameterFloat;
    using API = juce::AudioParameterInt;
    using APB = juce::AudioParameterBool;
    using NR  = juce::NormalisableRange<float>;

    // ---- Slot control parameters (9 total) ----
    for (int n = 1; n <= kNumSlots; ++n)
    {
        juce::String prefix = "slot" + juce::String(n) + "_";
        layout.add(std::make_unique<AP>(
            juce::ParameterID{prefix + "chain", 1},
            prefix + "Chain",
            NR(0.0f, static_cast<float>(kNumChains), 1.0f),
            0.0f));
        layout.add(std::make_unique<AP>(
            juce::ParameterID{prefix + "mix", 1},
            prefix + "Mix",
            NR(0.0f, 1.0f, 0.001f),
            1.0f));
        layout.add(std::make_unique<AP>(
            juce::ParameterID{prefix + "bypass", 1},
            prefix + "Bypass",
            NR(0.0f, 1.0f, 1.0f),
            0.0f));
    }

    // ---- Chain parameters: Task 3 adds all existing chain params here ----
    // ---- Task 6-9 add all new epic chain params here ----
    // Placeholder comment — see Tasks 3 and 6-9.
}

inline void EpicChainSlotController::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts)
{
    for (int n = 0; n < kNumSlots; ++n)
    {
        juce::String prefix = "slot" + juce::String(n + 1) + "_";
        slots_[n].pChain  = apvts.getRawParameterValue(prefix + "chain");
        slots_[n].pMix    = apvts.getRawParameterValue(prefix + "mix");
        slots_[n].pBypass = apvts.getRawParameterValue(prefix + "bypass");
        // Task 3 adds chain cacheParameterPointers calls here
    }
}

} // namespace xoceanus
```

- [ ] **Step 1.2** — Create `Source/Core/EpicChainSlotController.cpp`:

```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#include "EpicChainSlotController.h"
```

- [ ] **Step 1.3** — Build to verify compilation:
```bash
eval "$(fnm env)" && fnm use 20
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -5
cmake --build build 2>&1 | tail -20
```
Fix any compilation errors before proceeding.

---

### Task 2: Integrate EpicChainSlotController into MasterFXChain

**Files to modify:** `Source/Core/MasterFXChain.h`

**Goal:** Add the slot controller as a member, call prepare/reset, and route stages 19.6–19.8 through it. The old chain members remain in place during this task — they will be moved inside the slot controller in Task 3.

- [ ] **Step 2.1** — Read `Source/Core/MasterFXChain.h` to confirm current state before editing.

- [ ] **Step 2.2** — Add the include near the top of `MasterFXChain.h`, after the BoutiqueFXChain include (line ~33):

```cpp
#include "EpicChainSlotController.h"
```

- [ ] **Step 2.3** — Add the member variable in the `private:` section of `MasterFXChain`, before the existing chain members (search for `AquaticFXSuite aquaticFX`):

```cpp
    EpicChainSlotController epicSlots_;
```

- [ ] **Step 2.4** — In `prepare()`, add after `boutiqueFX.prepare(sampleRate)` (stage 19.8 prepare):

```cpp
        // Epic Chain Slot Controller (replaces fixed chain positions in stages 19.6-19.8)
        epicSlots_.prepare(sampleRate, samplesPerBlock);
        epicSlots_.cacheParameterPointers(apvts);
```

- [ ] **Step 2.5** — In `reset()`, add `epicSlots_.reset();` after `boutiqueFX.reset();`.

- [ ] **Step 2.6** — In `processBlock()`, after stage 19.8 BoutiqueFXChain block and before stage 20 (Brickwall Limiter), add:

```cpp
        // ====================================================================
        // Stages 19.6-19.8 (Epic Slot System — replaces fixed chain routing)
        // Note: In Task 3 the old fixed chain calls above will be removed and
        // the chains will be dispatched from within epicSlots_.processBlock().
        // For now, epicSlots_ runs as an additional passthrough stage.
        // ====================================================================
        epicSlots_.processBlock(buffer, numSamples, bpm, ppqPosition);
```

- [ ] **Step 2.7** — In `addParameters()` / the static method that registers parameters (this is typically in `XOceanusProcessor.cpp` — find where `AquaticFXSuite::addParameters(layout)` is called and add next to it):

```cpp
        EpicChainSlotController::addParameters(layout);
```

Note: Do NOT remove the existing `AquaticFXSuite::addParameters(layout)` call yet — that happens in Task 3.

- [ ] **Step 2.8** — Build to verify:
```bash
cmake --build build 2>&1 | tail -20
```

---

### Task 3: Move existing chains into slot controller

**Files to modify:** `Source/Core/EpicChainSlotController.h`, `Source/Core/MasterFXChain.h`

**Goal:** All 6 existing chain instances live inside FXSlot, dispatched by the switch. MasterFXChain stages 19.6–19.8 (fixed calls) and the 3 Singularity FX stages are removed.

> **CRITICAL:** The Singularity FX (fXOnslaught, fXObscura, fXOratory) are currently scattered at stages 5.5, 8.5, and 14.5 in MasterFXChain. They move into the slot system here. All their `master_obs*`, `master_onsl*`, and `master_ora*` parameter IDs must be aliased to `slot{N}_obs_*`, `slot{N}_onsl_*`, and `slot{N}_ora_*`. The Preset Migration task (Task 5) handles the preset-side aliasing.

- [ ] **Step 3.1** — Add includes to `EpicChainSlotController.h`:

```cpp
#include "../DSP/Effects/AquaticFXSuite.h"
#include "../DSP/Effects/MathFXChain.h"
#include "../DSP/Effects/BoutiqueFXChain.h"
#include "../DSP/Effects/fXOnslaught.h"
#include "../DSP/Effects/fXObscura.h"
#include "../DSP/Effects/fXOratory.h"
```

Add these at the top of the file, after `<array>`.

- [ ] **Step 3.2** — Add chain instances to the `FXSlot` struct (uncomment the placeholder block and add the actual members):

```cpp
    struct FXSlot
    {
        ChainID activeChain   = Off;
        ChainID pendingChain  = Off;
        float crossfadeProgress = 1.0f;
        juce::AudioBuffer<float> crossfadeBuf;

        // All 6 existing chain instances (pre-allocated, never on audio-thread heap)
        AquaticFXSuite    aquatic;
        MathFXChain       math;
        BoutiqueFXChain   boutique;
        fXOnslaught       onslaught;
        fXObscura         obscura;
        fXOratory         oratory;

        // Epic chains — added in Tasks 6-9
        // OnrushChain     onrush;
        // OmnistereoChain omnistereo;
        // ObliterateChain obliterate;
        // ObscurityChain  obscurity;

        std::atomic<float>* pChain  = nullptr;
        std::atomic<float>* pMix    = nullptr;
        std::atomic<float>* pBypass = nullptr;
    };
```

- [ ] **Step 3.3** — In `prepare()`, add chain prepare calls inside the slot loop:

```cpp
inline void EpicChainSlotController::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;

    for (auto& slot : slots_)
    {
        slot.crossfadeBuf.setSize(2, maxBlockSize, false, true, true);
        slot.crossfadeProgress = 1.0f;
        slot.activeChain       = Off;
        slot.pendingChain      = Off;

        slot.aquatic.prepare(sampleRate, maxBlockSize);
        slot.math.prepare(sampleRate);
        slot.boutique.prepare(sampleRate);
        slot.onslaught.prepare(sampleRate, maxBlockSize);
        slot.obscura.prepare(sampleRate);
        slot.oratory.prepare(sampleRate, maxBlockSize);
        // Epic chains — Tasks 6-9
    }
}
```

- [ ] **Step 3.4** — In `reset()`, add chain reset calls:

```cpp
inline void EpicChainSlotController::reset()
{
    for (auto& slot : slots_)
    {
        slot.crossfadeProgress = 1.0f;
        slot.aquatic.reset();
        slot.math.reset();
        slot.boutique.reset();
        slot.onslaught.reset();
        slot.obscura.reset();
        slot.oratory.reset();
        // Epic chains — Tasks 6-9
    }
}
```

- [ ] **Step 3.5** — Fill in `dispatchChain()` with real chain dispatch for the 6 existing chains. The bpm / ppqPosition are passed for chains that need them (AquaticFXSuite needs bpm; fXOratory needs ppqPosition):

```cpp
inline void EpicChainSlotController::dispatchChain(FXSlot& slot, ChainID chain,
                                                    float* L, float* R,
                                                    int numSamples,
                                                    double bpm,
                                                    double ppqPosition)
{
    switch (chain)
    {
        case Off:
            break; // passthrough

        case Aquatic:
            slot.aquatic.processBlock(L, R, numSamples, bpm);
            break;

        case Math:
            slot.math.processBlockFromSlot(L, R, numSamples);
            break;

        case Boutique:
            slot.boutique.processBlockFromSlot(L, R, numSamples);
            break;

        case Onslaught:
            slot.onslaught.processBlockFromSlot(L, R, numSamples);
            break;

        case Obscura:
            slot.obscura.processBlockFromSlot(L, R, numSamples);
            break;

        case Oratory:
            slot.oratory.processBlock(L, R, numSamples, ppqPosition);
            break;

        // Epic chains (Tasks 6-9):
        case Onrush:     break; // passthrough until Task 6
        case Omnistereo: break; // passthrough until Task 7
        case Obliterate: break; // passthrough until Task 8
        case Obscurity:  break; // passthrough until Task 9

        default: break;
    }
}
```

> **Note on processBlockFromSlot():** MathFXChain, BoutiqueFXChain, fXOnslaught, and fXObscura currently have their parameters read inline inside MasterFXChain::processBlock() using local variables from cached pointers. When these chains move into the slot controller, they need their own `cacheParameterPointers()` to be called per-slot, or we add a `processBlockFromSlot()` variant that reads directly from internally-cached pointers. The cleanest approach: call `cacheParameterPointers(apvts)` on each chain instance during the slot controller's `cacheParameterPointers()`, so each chain reads its own params. Confirm that MathFXChain, BoutiqueFXChain, fXOnslaught, fXObscura, and fXOratory each have a `cacheParameterPointers(apvts)` method (they should — check the headers). If they do, call them in step 3.6. If any chain still uses a parameter-passing processBlock signature, add the internal cache version.

- [ ] **Step 3.6** — In `cacheParameterPointers()`, add per-slot chain cache calls:

```cpp
inline void EpicChainSlotController::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts)
{
    for (int n = 0; n < kNumSlots; ++n)
    {
        juce::String prefix = "slot" + juce::String(n + 1) + "_";
        slots_[n].pChain  = apvts.getRawParameterValue(prefix + "chain");
        slots_[n].pMix    = apvts.getRawParameterValue(prefix + "mix");
        slots_[n].pBypass = apvts.getRawParameterValue(prefix + "bypass");

        // Cache param pointers for each chain in this slot
        // Each chain's cacheParameterPointers looks up slot{N}-prefixed IDs
        slots_[n].aquatic.cacheParameterPointers(apvts);
        slots_[n].math.cacheParameterPointers(apvts);
        slots_[n].boutique.cacheParameterPointers(apvts);
        slots_[n].onslaught.cacheParameterPointers(apvts);
        slots_[n].obscura.cacheParameterPointers(apvts);
        slots_[n].oratory.cacheParameterPointers(apvts);
        // Epic chains — Tasks 6-9
    }
}
```

> **Implementation note on slot-prefixed param IDs:** The existing chain `addParameters()` calls use bare IDs like `aqua_fathom_mix`. For the slot system, each slot's instance needs its own parameter set. The cleanest approach: add an `addParameters(layout, const juce::String& slotPrefix)` overload to each chain, which prepends `slotPrefix` to every param ID. Similarly, `cacheParameterPointers(apvts, const juce::String& slotPrefix)` uses the prefix. Since the existing chains already have static `addParameters()` and instance `cacheParameterPointers()`, the migration is: (a) update each chain to accept an optional prefix, (b) call with `"slot1_"`, `"slot2_"`, `"slot3_"` for the 3 instances. Alternatively, make the prefix a constructor argument stored as a member. Choose whichever requires fewer changes to the existing chain headers.

- [ ] **Step 3.7** — In `addParameters()`, add calls to all chain `addParameters()` for all 3 slots:

```cpp
inline void EpicChainSlotController::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using AP = juce::AudioParameterFloat;
    using NR = juce::NormalisableRange<float>;

    // ---- Slot control parameters ----
    for (int n = 1; n <= kNumSlots; ++n)
    {
        juce::String prefix = "slot" + juce::String(n) + "_";
        layout.add(std::make_unique<AP>(
            juce::ParameterID{prefix + "chain", 1}, prefix + "Chain",
            NR(0.0f, static_cast<float>(kNumChains), 1.0f), 0.0f));
        layout.add(std::make_unique<AP>(
            juce::ParameterID{prefix + "mix", 1}, prefix + "Mix",
            NR(0.0f, 1.0f, 0.001f), 1.0f));
        layout.add(std::make_unique<AP>(
            juce::ParameterID{prefix + "bypass", 1}, prefix + "Bypass",
            NR(0.0f, 1.0f, 1.0f), 0.0f));
    }

    // ---- Chain parameters (all 3 instances of each chain) ----
    for (int n = 1; n <= kNumSlots; ++n)
    {
        juce::String slotPrefix = "slot" + juce::String(n) + "_";
        AquaticFXSuite::addParameters(layout, slotPrefix);
        MathFXChain::addParameters(layout, slotPrefix);
        BoutiqueFXChain::addParameters(layout, slotPrefix);
        fXOnslaught::addParameters(layout, slotPrefix);
        fXObscura::addParameters(layout, slotPrefix);
        fXOratory::addParameters(layout, slotPrefix);
        // Epic chains — Tasks 6-9
    }
}
```

- [ ] **Step 3.8** — In `MasterFXChain.h`:
  - Remove the fixed stage 19.6 `aquaticFX.processBlock(L, R, numSamples, bpm)` call
  - Remove the fixed stage 19.7 `mathFX.processBlock(...)` block
  - Remove the fixed stage 19.8 `boutiqueFX.processBlock(...)` block
  - Remove the scattered Singularity FX stage calls (5.5 fXObscura, 8.5 fXOratory, 14.5 fXOnslaught)
  - Remove the corresponding `cacheParameterPointers()` lookups for `master_obs*`, `master_onsl*`, `master_ora*`, `mfx_*`, `bfx_*`
  - Remove the member variables: `AquaticFXSuite aquaticFX`, `MathFXChain mathFX`, `BoutiqueFXChain boutiqueFX` (the `fXOnslaught onslaught`, `fXObscura obscura`, `fXOratory oratory` members also removed)
  - Remove the corresponding `pObs*`, `pOnsl*`, `pOra*`, `pMfx*`, `pBfx*` pointer members
  - Keep all the non-FX-chain stages (1–19, 20–22) intact
  - Remove `AquaticFXSuite::addParameters(layout)`, `MathFXChain::addParameters(layout)`, `BoutiqueFXChain::addParameters(layout)` calls from wherever `addParameters` is registered (likely in `XOceanusProcessor.cpp`)

- [ ] **Step 3.9** — Build to verify:
```bash
cmake --build build 2>&1 | tail -30
```
This is the riskiest build step — expect to iterate. Common errors:
- Missing `reset()` methods on chain types → add them
- `processBlock` signature mismatch → check each chain's exact processBlock signature and adapt the dispatch accordingly
- Missing `addParameters(layout, prefix)` overloads → add overloads

---

### Task 4: Parameter alias system

**Files to modify:** `Source/Core/PresetManager.h`

**Goal:** All pre-slot parameter IDs (e.g., `aqua_fathom_mix`, `master_onslFlowRate`) resolve to their slot-prefixed equivalents. This ensures DAW automation recorded with old IDs continues to function.

- [ ] **Step 4.1** — Read `Source/Core/PresetManager.h` to understand current structure and find the `resolveEngineAlias()` pattern.

- [ ] **Step 4.2** — Add `std::unordered_map` to includes:

```cpp
#include <unordered_map>
```

- [ ] **Step 4.3** — Add the following inline function after `resolveEngineAlias()`:

```cpp
//==============================================================================
// resolveFXParamAlias() — map pre-slot FX parameter IDs to their slot-prefixed
// equivalents. Called during APVTS parameter lookup and preset loading.
//
// Default slot assignment:
//   aqua_*           → slot1_aqua_*   (Aquatic defaults to slot 1)
//   mfx_*            → slot2_mfx_*    (Math defaults to slot 2)
//   bfx_*            → slot3_bfx_*    (Boutique defaults to slot 3)
//   master_onsl*     → slot1_onsl_*   (Onslaught defaults to slot 1)
//   master_obs*      → slot1_obs_*    (Obscura defaults to slot 1)
//   master_ora*      → slot1_ora_*    (Oratory defaults to slot 1)
//
// Returns the input unchanged if it is already slot-prefixed or unknown.
inline juce::String resolveFXParamAlias(const juce::String& paramId)
{
    // Already slot-prefixed
    if (paramId.startsWith("slot1_") || paramId.startsWith("slot2_") ||
        paramId.startsWith("slot3_"))
        return paramId;

    // aqua_ → slot1_aqua_
    if (paramId.startsWith("aqua_"))
        return "slot1_" + paramId;

    // mfx_ → slot2_mfx_  (note: mfx_formant* and mfx_breath* are Membrane Collection,
    //                      NOT part of MathFXChain — those stay at their master_ IDs)
    if (paramId.startsWith("mfx_ec") || paramId.startsWith("mfx_vs") ||
        paramId.startsWith("mfx_qs") || paramId.startsWith("mfx_ad"))
        return "slot2_" + paramId;

    // bfx_ → slot3_bfx_
    if (paramId.startsWith("bfx_"))
        return "slot3_" + paramId;

    // master_onsl* → slot1_onsl_* (strip "master_" prefix)
    if (paramId.startsWith("master_onsl"))
        return "slot1_onsl_" + paramId.fromFirstOccurrenceOf("master_onsl", false, true);

    // master_obs* (Obscura) → slot1_obs_*
    // Note: master_osmMembrane etc. are fXOsmosis (different effect) — exclude those
    if (paramId.startsWith("master_obsT") || paramId.startsWith("master_obsH") ||
        paramId.startsWith("master_obsR") || paramId.startsWith("master_obsE") ||
        paramId.startsWith("master_obsS") || paramId.startsWith("master_obsD") ||
        paramId.startsWith("master_obsPa") || paramId.startsWith("master_obsM"))
        return "slot1_obs_" + paramId.fromFirstOccurrenceOf("master_obs", false, true);

    // master_ora* → slot1_ora_*
    if (paramId.startsWith("master_ora"))
        return "slot1_ora_" + paramId.fromFirstOccurrenceOf("master_ora", false, true);

    return paramId; // no alias found — return unchanged
}
```

- [ ] **Step 4.4** — Build to verify:
```bash
cmake --build build 2>&1 | tail -10
```

---

### Task 5: Preset migration (fxVersion 1 → 2)

**Files to modify:** `Source/Core/PresetManager.h`

**Goal:** Presets without `slot1_chain` are detected as fxVersion 1 and automatically migrated. After migration, `"fxVersion": 2` is written.

- [ ] **Step 5.1** — Find the preset loading function in PresetManager.h. Look for where `juce::var` is parsed from JSON and engine parameters are applied. Confirm the function signature by reading the file.

- [ ] **Step 5.2** — Add the migration method as an inline free function near `resolveFXParamAlias()`:

```cpp
//==============================================================================
// migrateFxV1toV2() — upgrade legacy preset parameter keys to the slot system.
//
// Detection: absence of "slot1_chain" key in the "parameters" object.
//
// Migration strategy:
//   1. Scan for non-zero aqua_* mix → assign to Slot 1 (chain = Aquatic)
//   2. Scan for non-zero mfx_* mix  → assign to Slot 2 (chain = Math)
//   3. Scan for non-zero bfx_* mix  → assign to Slot 3 (chain = Boutique)
//   4. Singularity FX (master_onsl*, master_obs*, master_ora*) →
//      fill next available slot; warn and drop if all 3 already filled
//
// After migration, adds "fxVersion": 2 to the preset root object.
//
// @param presetRoot  The root juce::var of the parsed JSON preset (mutable).
//
inline void migrateFxV1toV2(juce::var& presetRoot)
{
    // Only run migration if "parameters" is an object
    auto* paramsObj = presetRoot["parameters"].getDynamicObject();
    if (paramsObj == nullptr)
        return;

    // Check if already v2 (slot1_chain present)
    // We look for any key starting with "slot1_"
    for (const auto& prop : paramsObj->getProperties())
    {
        if (prop.name.toString().startsWith("slot1_"))
            return; // already migrated
    }

    DBG("[PresetManager] fxV1→v2 migration: upgrading FX slot parameters");

    // Build flattened parameter map (engine name stripped — all params flat)
    // The preset format stores params as: parameters.engineName.paramId = value
    // We need to find aqua_*, mfx_*, bfx_*, master_onsl*, master_obs*, master_ora*
    // by scanning all engine parameter objects.

    std::vector<std::pair<juce::String, juce::var>> allParams;
    for (const auto& engineProp : paramsObj->getProperties())
    {
        if (auto* engineObj = engineProp.value.getDynamicObject())
        {
            for (const auto& paramProp : engineObj->getProperties())
                allParams.push_back({paramProp.name.toString(), paramProp.value});
        }
    }

    // Detect which legacy chains are active (mix > 0)
    bool aquaticActive    = false;
    bool mathActive       = false;
    bool boutiqueActive   = false;
    bool onslaughtActive  = false;
    bool obscuraActive    = false;
    bool oratoryActive    = false;

    for (const auto& [id, val] : allParams)
    {
        float v = static_cast<float>(static_cast<double>(val));
        if (v > 0.001f)
        {
            if (id == "aqua_fathom_mix" || id == "aqua_drift_mix" ||
                id == "aqua_tide_mix"   || id == "aqua_reef_mix"  ||
                id == "aqua_surface_mix"|| id == "aqua_biolume_mix")
                aquaticActive = true;

            if (id == "mfx_ecMix" || id == "mfx_vsMix" ||
                id == "mfx_qsMix" || id == "mfx_adMix")
                mathActive = true;

            if (id == "bfx_anMix" || id == "bfx_arMix" ||
                id == "bfx_caMix" || id == "bfx_smMix" || id == "bfx_acMix")
                boutiqueActive = true;

            if (id == "master_onslMix")
                onslaughtActive = true;

            if (id == "master_obsMix")
                obscuraActive = true;

            if (id == "master_oraMix")
                oratoryActive = true;
        }
    }

    // Build slot assignments
    // Priority: Suite chains take slots 1-3; Singularity FX fill remaining slots
    int nextFreeSlot = 1;

    // Assign suite chains to fixed default slots
    std::array<std::pair<int, juce::String>, 6> slotAssignments = {{
        { aquaticActive  ? 1 : 0,  "aquatic"   },
        { mathActive     ? 2 : 0,  "math"      },
        { boutiqueActive ? 3 : 0,  "boutique"  },
        { onslaughtActive ? -1 : 0,"onslaught" }, // -1 = assign to first free
        { obscuraActive  ? -1 : 0, "obscura"   },
        { oratoryActive  ? -1 : 0, "oratory"   },
    }};

    // Resolve -1 (first free slot after suites)
    std::set<int> usedSlots;
    if (aquaticActive)  usedSlots.insert(1);
    if (mathActive)     usedSlots.insert(2);
    if (boutiqueActive) usedSlots.insert(3);

    for (auto& [slot, chainName] : slotAssignments)
    {
        if (slot == -1)
        {
            // Find first free slot 1-3
            bool assigned = false;
            for (int s = 1; s <= 3; ++s)
            {
                if (usedSlots.find(s) == usedSlots.end())
                {
                    slot = s;
                    usedSlots.insert(s);
                    assigned = true;
                    break;
                }
            }
            if (!assigned)
            {
                // All 3 slots taken — drop this Singularity FX chain
                DBG("[PresetManager] fxVersion migration warning: Singularity FX '"
                    + chainName + "' dropped — no available slot.");
                slot = 0; // mark as dropped
            }
        }
    }

    // Map chain name → ChainID integer value (matches EpicChainSlotController::ChainID)
    const std::map<juce::String, int> chainIdMap = {
        {"aquatic",    1}, {"math",      2}, {"boutique",  3},
        {"onslaught",  4}, {"obscura",   5}, {"oratory",   6},
    };

    // Build a "master" (non-engine-keyed) section for slot control params
    auto* masterEngineObj = paramsObj->getProperty("master").getDynamicObject();
    if (masterEngineObj == nullptr)
    {
        auto* newMaster = new juce::DynamicObject();
        paramsObj->setProperty("master", juce::var(newMaster));
        masterEngineObj = newMaster;
    }

    for (const auto& [slot, chainName] : slotAssignments)
    {
        if (slot <= 0) continue; // dropped or not active

        juce::String slotPrefix = "slot" + juce::String(slot) + "_";

        // Set slot chain selector
        auto it = chainIdMap.find(chainName);
        if (it != chainIdMap.end())
            masterEngineObj->setProperty(slotPrefix + "chain",
                                         juce::var(static_cast<double>(it->second)));

        masterEngineObj->setProperty(slotPrefix + "mix",    juce::var(1.0));
        masterEngineObj->setProperty(slotPrefix + "bypass", juce::var(0.0));

        // Rename all matching parameter keys with slot prefix
        for (const auto& engineProp : paramsObj->getProperties())
        {
            if (auto* engineObj = engineProp.value.getDynamicObject())
            {
                // Collect keys to rename (avoid mutating while iterating)
                std::vector<std::pair<juce::String, juce::var>> toRename;
                for (const auto& paramProp : engineObj->getProperties())
                {
                    juce::String pid = paramProp.name.toString();
                    bool matches = false;

                    if (chainName == "aquatic"   && pid.startsWith("aqua_"))    matches = true;
                    if (chainName == "math"      && (pid.startsWith("mfx_ec") ||
                                                     pid.startsWith("mfx_vs") ||
                                                     pid.startsWith("mfx_qs") ||
                                                     pid.startsWith("mfx_ad"))) matches = true;
                    if (chainName == "boutique"  && pid.startsWith("bfx_"))     matches = true;
                    if (chainName == "onslaught" && pid.startsWith("master_onsl")) matches = true;
                    if (chainName == "obscura"   && (pid.startsWith("master_obsT") ||
                                                     pid.startsWith("master_obsH") ||
                                                     pid.startsWith("master_obsR") ||
                                                     pid.startsWith("master_obsE") ||
                                                     pid.startsWith("master_obsS") ||
                                                     pid.startsWith("master_obsD") ||
                                                     pid.startsWith("master_obsPa")||
                                                     pid.startsWith("master_obsM"))) matches = true;
                    if (chainName == "oratory"   && pid.startsWith("master_ora")) matches = true;

                    if (matches)
                        toRename.push_back({pid, paramProp.value});
                }

                for (const auto& [oldId, val] : toRename)
                {
                    juce::String newId = resolveFXParamAlias(oldId);
                    // Prepend slot prefix if not already prefixed
                    if (!newId.startsWith("slot"))
                        newId = slotPrefix + newId;
                    engineObj->removeProperty(oldId);
                    engineObj->setProperty(newId, val);
                }
            }
        }
    }

    // Tag with fxVersion 2
    if (auto* rootObj = presetRoot.getDynamicObject())
        rootObj->setProperty("fxVersion", juce::var(2));
}
```

- [ ] **Step 5.3** — Wire migration into the preset load path. Find the function in `PresetManager.h` that parses a JSON string and calls engine parameter setters. Immediately after parsing the JSON root and before applying parameters, add:

```cpp
        // fxVersion 1→2 migration (slot system)
        const int fxVersion = root.getProperty("fxVersion", juce::var(1));
        if (fxVersion < 2)
            migrateFxV1toV2(root);
```

- [ ] **Step 5.4** — Build to verify:
```bash
cmake --build build 2>&1 | tail -15
```

---

## Phase 2: Epic Chain Engines

> **All Task 6-9 are independent of each other and can be implemented in parallel by separate agents.**

### Task 6: ONRUSH chain (Expressive Lead)

**Files to create:**
- `Source/DSP/Effects/OnrushChain.h`
- `Source/DSP/Effects/OnrushChain.cpp`

**Spec:** Section 2.1 of `Docs/specs/2026-04-05-epic-chains-fx-design.md`
**Routing:** Mono In → Stereo Out
**Accent Color:** Molten Amber `#FF6F00`
**Parameter prefix:** `onr_` (slot-prefixed: `slot{N}_onr_`)

**Required includes:**
```cpp
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "../Saturator.h"
#include "../PolyBLEP.h"
#include "../ParameterSmoother.h"
#include "../StandardLFO.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <cmath>
```

- [ ] **Step 6.1** — Write the complete class skeleton (header section only — member variables, struct declarations, method signatures):

```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// [includes listed above]

namespace xoceanus
{

//==============================================================================
// OnrushChain — ONRUSH Expressive Lead FX Chain (5 stages)
//
// Source concept: PolySwell (Gemini Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 5 BBD Delay)
// Accent: Molten Amber #FF6F00
//
// Stage 1: Auto-Swell (Boss SG-1) — one-pole IIR envelope + Schmitt trigger
// Stage 2: Ring Modulator (Fairfield Randy's Revenge) — dual PolyBLEP + fastTanh
// Stage 3: Hard Clip Distortion (Boss DS-1) — soft-knee + 4x OVS + tilt EQ
// Stage 4: Envelope Filter (Boss TW-1) — env follower → fastPow2 freq → CytomicSVF
// Stage 5: Pitch-Sequenced BBD Delay (Chase Bliss Thermae) — Hermite + 2-step seq
//
// Parameter prefix: onr_ (17 params)
//==============================================================================
class OnrushChain
{
public:
    OnrushChain() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // Process mono input, writing stereo to L and R.
    // Caller must ensure L != R (separate output buffers).
    void processBlock(const float* monoIn, float* L, float* R, int numSamples,
                      double bpm = 0.0, double ppqPosition = -1.0);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "");
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "");

private:
    double sr_ = 44100.0;
    int    blockSize_ = 512;

    //==========================================================================
    // Stage 1 — Auto-Swell (Boss SG-1)
    //==========================================================================
    struct AutoSwellStage
    {
        // One-pole IIR envelope follower
        float envState   = 0.0f;
        float gainOut    = 1.0f;
        float schmittHi  = 0.0f; // Schmitt trigger high threshold (linear)
        float schmittLo  = 0.0f; // Schmitt trigger low threshold (linear)
        bool  swellOpen  = true; // true = gate open (passing signal)
        float attackCoeff  = 0.0f;
        float releaseCoeff = 0.0f;
        ParameterSmoother gainSmoother;

        void prepare(double sampleRate)
        {
            gainSmoother.prepare(sampleRate, 0.005f); // 5ms
            reset();
        }
        void reset()
        {
            envState  = 0.0f;
            gainOut   = 1.0f;
            swellOpen = true;
            gainSmoother.reset();
        }
        void setCoeffs(float attackMs, float releaseMs, float threshDb, double sampleRate)
        {
            float sr = static_cast<float>(sampleRate);
            attackCoeff  = FastMath::fastExp(-1.0f / (attackMs  * 0.001f * sr));
            releaseCoeff = FastMath::fastExp(-1.0f / (releaseMs * 0.001f * sr));
            float threshLin = FastMath::fastPow2(threshDb * (1.0f / 6.0205999f));
            schmittHi = threshLin * 1.06f; // +0.5dB hysteresis
            schmittLo = threshLin * 0.94f; // -0.5dB hysteresis
        }
        float process(float in)
        {
            // Envelope follower
            float absIn = std::abs(in);
            float coeff = absIn > envState ? attackCoeff : releaseCoeff;
            envState = flushDenormal(envState * coeff + absIn * (1.0f - coeff));

            // Schmitt trigger
            if (swellOpen  && envState < schmittLo) swellOpen = false;
            if (!swellOpen && envState > schmittHi) swellOpen = true;

            float targetGain = swellOpen ? 1.0f : 0.0f;
            gainSmoother.setTarget(targetGain);
            return in * gainSmoother.process();
        }
    } swell_;

    //==========================================================================
    // Stage 2 — Ring Modulator (Fairfield Randy's Revenge)
    //==========================================================================
    struct RingModStage
    {
        PolyBLEP sineCarrier;  // wave type = Sine
        PolyBLEP squareCarrier;// wave type = Square
        float dcOffset = 0.0f; // DC offset leak cap state

        void prepare(double sampleRate)
        {
            sineCarrier.prepare(sampleRate);
            sineCarrier.setWaveform(PolyBLEP::Waveform::Sine);
            squareCarrier.prepare(sampleRate);
            squareCarrier.setWaveform(PolyBLEP::Waveform::Square);
            dcOffset = 0.0f;
        }
        void reset() { dcOffset = 0.0f; }
        void setFreq(float hz) { sineCarrier.setFrequency(hz); squareCarrier.setFrequency(hz); }

        float process(float in, float blend, float mix)
        {
            float sine   = sineCarrier.processSample();
            float square = squareCarrier.processSample();
            // Manual morph crossfade (PolyBLEP has no native morph mode)
            float carrier = sine * (1.0f - blend) + square * blend;
            carrier = FastMath::fastTanh(carrier); // nonlinear character
            // DC offset leak cap (1-pole HP at ~5Hz)
            float modOut = in * carrier;
            dcOffset = flushDenormal(dcOffset + 0.9997f * (modOut - dcOffset));
            modOut -= dcOffset;
            return in + mix * (modOut - in);
        }
    } ringMod_;

    //==========================================================================
    // Stage 3 — Hard Clip Distortion (Boss DS-1 Golden)
    // 4x oversampling using simple 2x IIR up then 2x down
    //==========================================================================
    struct DistortionStage
    {
        // Anti-aliasing filter states for 4x oversampling (up/down)
        float aaUp1L = 0.0f, aaUp1R = 0.0f;
        float aaUp2L = 0.0f, aaUp2R = 0.0f;
        float aaDn1  = 0.0f;
        float aaDn2  = 0.0f;
        CytomicSVF tiltSVF;

        void prepare(double /*sampleRate*/) { tiltSVF.reset(); reset(); }
        void reset()
        {
            aaUp1L = aaUp1R = aaUp2L = aaUp2R = 0.0f;
            aaDn1 = aaDn2 = 0.0f;
            tiltSVF.reset();
        }

        // Soft-knee saturator: x / (1 + |x|)
        static float softKnee(float x, float drive)
        {
            float d = x * drive;
            return d / (1.0f + std::abs(d));
        }

        float process(float in, float driveLinear, float tilt, float outGain, double sampleRate)
        {
            // Simple 4x oversampling: upsample by 2 twice, process, downsample
            // Up-stage 1 (linear interpolation × 2)
            float up1a = (in + aaUp1L) * 0.5f;
            float up1b = in;
            aaUp1L = in;

            float up2a = (up1a + aaUp2L) * 0.5f;
            float up2b = up1a;
            float up2c = (up1b + aaUp2L) * 0.5f;
            float up2d = up1b;
            aaUp2L = up1b;

            // Apply saturation at 4x rate
            up2a = softKnee(up2a, driveLinear);
            up2b = softKnee(up2b, driveLinear);
            up2c = softKnee(up2c, driveLinear);
            up2d = softKnee(up2d, driveLinear);

            // Downsample 4→1 (simple averaging — adequate for this use)
            float out = (up2a + up2b + up2c + up2d) * 0.25f;

            // Tilt EQ: CytomicSVF low-shelf
            // tilt = -1.0 (dark) to +1.0 (bright)
            float tiltGainDb = tilt * 6.0f;
            float tiltFreq = 800.0f;
            tiltSVF.setLowShelf(tiltFreq, 0.707f, tiltGainDb, sampleRate);
            out = tiltSVF.processLP(out); // use LP output of low-shelf topology

            return out * outGain;
        }
    } dist_;

    //==========================================================================
    // Stage 4 — Envelope Filter (Boss TW-1 T Wah)
    //==========================================================================
    struct EnvFilterStage
    {
        float envState = 0.0f;
        float envAttack  = 0.0f;
        float envRelease = 0.0f;
        CytomicSVF svf;
        ParameterSmoother freqSmoother;

        void prepare(double sampleRate)
        {
            freqSmoother.prepare(sampleRate, 0.001f); // 1ms smoothing
            reset();
        }
        void reset()
        {
            envState = 0.0f;
            svf.reset();
            freqSmoother.reset();
        }
        void setTimeConsts(double sampleRate)
        {
            // Fixed fast attack, moderate release
            envAttack  = FastMath::fastExp(-1.0f / (0.001f * static_cast<float>(sampleRate)));
            envRelease = FastMath::fastExp(-1.0f / (0.050f * static_cast<float>(sampleRate)));
        }
        float process(float in, float baseFreq, float depthOctaves, float resonanceQ,
                      double sampleRate)
        {
            float absIn = std::abs(in);
            float coeff = absIn > envState ? envAttack : envRelease;
            envState = flushDenormal(envState * coeff + absIn * (1.0f - coeff));

            // Logarithmic frequency mapping: base * 2^(env * depth)
            float targetFreq = baseFreq * FastMath::fastPow2(envState * depthOctaves);
            targetFreq = std::max(20.0f, std::min(targetFreq, static_cast<float>(sampleRate) * 0.45f));

            freqSmoother.setTarget(targetFreq);
            float smoothedFreq = freqSmoother.process();

            svf.setLP(smoothedFreq, resonanceQ, sampleRate);
            return svf.processLP(in);
        }
    } envFilter_;

    //==========================================================================
    // Stage 5 — Pitch-Sequenced BBD Delay (Chase Bliss Thermae)
    // Hermite fractional delay. Mono → Stereo split at this stage.
    //==========================================================================
    struct BBDDelayStage
    {
        static constexpr float kMaxDelayMs = 1500.0f;
        static constexpr int kNumSteps = 2;

        std::vector<float> delayBufL;
        std::vector<float> delayBufR;
        int writePosL = 0;
        int writePosR = 0;
        float feedbackL = 0.0f;
        float feedbackR = 0.0f;

        // Sequencer
        int   currentStep = 0;
        float stepPhase   = 0.0f; // 0.0→1.0 per beat
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 8;
            delayBufL.assign(maxSamples, 0.0f);
            delayBufR.assign(maxSamples, 0.0f);
            reset();
        }
        void reset()
        {
            std::fill(delayBufL.begin(), delayBufL.end(), 0.0f);
            std::fill(delayBufR.begin(), delayBufR.end(), 0.0f);
            writePosL = writePosR = 0;
            feedbackL = feedbackR = 0.0f;
            currentStep = 0;
            stepPhase = 0.0f;
        }

        // Hermite cubic interpolation
        static float hermite(const std::vector<float>& buf, int writePos, float delaySamples)
        {
            int size = static_cast<int>(buf.size());
            float readF = static_cast<float>(writePos) - delaySamples;
            while (readF < 0.0f) readF += static_cast<float>(size);
            while (readF >= static_cast<float>(size)) readF -= static_cast<float>(size);
            int i1 = static_cast<int>(readF);
            int i0 = (i1 - 1 + size) % size;
            int i2 = (i1 + 1) % size;
            int i3 = (i1 + 2) % size;
            float t = readF - static_cast<float>(i1);
            float a = buf[i0], b = buf[i1], c = buf[i2], d = buf[i3];
            return b + 0.5f * t * (c - a + t * (2.0f * a - 5.0f * b + 4.0f * c - d
                                                + t * (3.0f * (b - c) + d - a)));
        }

        void processBlock(const float* monoIn, float* L, float* R, int numSamples,
                          float baseDelayMs, float semitone1, float semitone2,
                          float feedback, float mix, double bpm)
        {
            float srF = static_cast<float>(sr);
            float baseDelaySamples = std::max(1.0f, baseDelayMs * srF / 1000.0f);
            baseDelaySamples = std::min(baseDelaySamples,
                                        static_cast<float>(delayBufL.size()) - 2.0f);

            // Pitch ratio from semitones: 2^(s/12)
            float ratioL = FastMath::fastPow2(semitone1 / 12.0f);
            float ratioR = FastMath::fastPow2(semitone2 / 12.0f);
            float delayL = baseDelaySamples / std::max(0.01f, ratioL);
            float delayR = baseDelaySamples / std::max(0.01f, ratioR);
            delayL = std::min(delayL, static_cast<float>(delayBufL.size()) - 2.0f);
            delayR = std::min(delayR, static_cast<float>(delayBufR.size()) - 2.0f);

            int bufSize = static_cast<int>(delayBufL.size());

            for (int i = 0; i < numSamples; ++i)
            {
                float in = monoIn[i];

                float wetL = hermite(delayBufL, writePosL, delayL);
                float wetR = hermite(delayBufR, writePosR, delayR);

                delayBufL[writePosL] = flushDenormal(in + feedbackL * feedback);
                delayBufR[writePosR] = flushDenormal(in + feedbackR * feedback);
                feedbackL = wetL;
                feedbackR = wetR;

                writePosL = (writePosL + 1) % bufSize;
                writePosR = (writePosR + 1) % bufSize;

                L[i] = in + mix * (wetL - in);
                R[i] = in + mix * (wetR - in);
            }
        }
    } bbd_;

    //==========================================================================
    // Cached parameter pointers
    //==========================================================================
    std::atomic<float>* p_swellThresh  = nullptr;
    std::atomic<float>* p_swellAttack  = nullptr;
    std::atomic<float>* p_swellRelease = nullptr;
    std::atomic<float>* p_ringFreq     = nullptr;
    std::atomic<float>* p_ringWave     = nullptr;
    std::atomic<float>* p_ringMix      = nullptr;
    std::atomic<float>* p_distDrive    = nullptr;
    std::atomic<float>* p_distTilt     = nullptr;
    std::atomic<float>* p_distOut      = nullptr;
    std::atomic<float>* p_envBaseFreq  = nullptr;
    std::atomic<float>* p_envDepth     = nullptr;
    std::atomic<float>* p_envRes       = nullptr;
    std::atomic<float>* p_delayBase    = nullptr;
    std::atomic<float>* p_seqStep1     = nullptr;
    std::atomic<float>* p_seqStep2     = nullptr;
    std::atomic<float>* p_delayFb      = nullptr;
    std::atomic<float>* p_delayMix     = nullptr;

    juce::String paramPrefix_; // stores the slot prefix used at cache time
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OnrushChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    swell_.prepare(sampleRate);
    ringMod_.prepare(sampleRate);
    dist_.prepare(sampleRate);
    envFilter_.prepare(sampleRate);
    bbd_.prepare(sampleRate);
}

inline void OnrushChain::reset()
{
    swell_.reset();
    ringMod_.reset();
    dist_.reset();
    envFilter_.reset();
    bbd_.reset();
}

inline void OnrushChain::processBlock(const float* monoIn, float* L, float* R,
                                       int numSamples, double bpm, double /*ppqPosition*/)
{
    juce::ScopedNoDenormals noDenormals;
    if (!p_swellThresh) return; // not yet cached

    // ParamSnapshot: read all params once
    const float swellThresh  = p_swellThresh->load(std::memory_order_relaxed);
    const float swellAttack  = p_swellAttack->load(std::memory_order_relaxed);
    const float swellRelease = p_swellRelease->load(std::memory_order_relaxed);
    const float ringFreq     = p_ringFreq->load(std::memory_order_relaxed);
    const float ringWave     = p_ringWave->load(std::memory_order_relaxed);
    const float ringMix      = p_ringMix->load(std::memory_order_relaxed);
    const float distDrive    = p_distDrive->load(std::memory_order_relaxed);
    const float distTilt     = p_distTilt->load(std::memory_order_relaxed);
    const float distOut      = p_distOut->load(std::memory_order_relaxed);
    const float envBaseFreq  = p_envBaseFreq->load(std::memory_order_relaxed);
    const float envDepth     = p_envDepth->load(std::memory_order_relaxed);
    const float envRes       = p_envRes->load(std::memory_order_relaxed);
    const float delayBase    = p_delayBase->load(std::memory_order_relaxed);
    const float seqStep1     = p_seqStep1->load(std::memory_order_relaxed);
    const float seqStep2     = p_seqStep2->load(std::memory_order_relaxed);
    const float delayFb      = p_delayFb->load(std::memory_order_relaxed);
    const float delayMix     = p_delayMix->load(std::memory_order_relaxed);

    // Convert param units
    // distDrive is in dB (0–40) → linear
    float driveLinear = FastMath::fastPow2(distDrive * (1.0f / 6.0205999f));
    float outGainLinear = FastMath::fastPow2(distOut * (1.0f / 6.0205999f));

    // Update stage coefficients (cheap — only does math when values differ)
    swell_.setCoeffs(swellAttack, swellRelease, swellThresh, sr_);
    ringMod_.setFreq(ringFreq);
    envFilter_.setTimeConsts(sr_);

    // Process: mono pipeline until Stage 5 splits to stereo
    for (int i = 0; i < numSamples; ++i)
    {
        float x = monoIn[i];

        // Stage 1: Auto-Swell
        x = swell_.process(x);

        // Stage 2: Ring Modulator
        x = ringMod_.process(x, ringWave, ringMix);

        // Stage 3: Distortion
        x = dist_.process(x, driveLinear, distTilt, outGainLinear, sr_);

        // Stage 4: Envelope Filter
        x = envFilter_.process(x, envBaseFreq, envDepth, envRes, sr_);

        // Buffer mono for BBD stage (write to L as temp)
        L[i] = x;
    }

    // Stage 5: BBD Delay — expands mono → stereo
    bbd_.processBlock(L, L, R, numSamples, delayBase, seqStep1, seqStep2,
                      delayFb * 0.01f, delayMix * 0.01f, bpm);
}

inline void OnrushChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    using AP = juce::AudioParameterFloat;
    using NR = juce::NormalisableRange<float>;
    const juce::String p = slotPrefix + "onr_";

    layout.add(std::make_unique<AP>(juce::ParameterID{p + "swellThresh",  1}, p + "Swell Thresh",
        NR(-60.0f, 0.0f, 0.1f), -24.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "swellAttack",  1}, p + "Swell Attack",
        NR(1.0f, 500.0f, 0.1f), 50.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "swellRelease", 1}, p + "Swell Release",
        NR(10.0f, 2000.0f, 1.0f), 200.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "ringFreq",     1}, p + "Ring Freq",
        NR(20.0f, 2000.0f, 0.1f), 220.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "ringWave",     1}, p + "Ring Wave",
        NR(0.0f, 1.0f, 0.001f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "ringMix",      1}, p + "Ring Mix",
        NR(0.0f, 100.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "distDrive",    1}, p + "Dist Drive",
        NR(0.0f, 40.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "distTilt",     1}, p + "Dist Tilt",
        NR(-1.0f, 1.0f, 0.001f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "distOut",      1}, p + "Dist Out",
        NR(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "envBaseFreq",  1}, p + "Env Base Freq",
        NR(50.0f, 2000.0f, 1.0f), 200.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "envDepth",     1}, p + "Env Depth",
        NR(0.0f, 4.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "envRes",       1}, p + "Env Res",
        NR(0.707f, 10.0f, 0.001f), 2.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "delayBase",    1}, p + "Delay Base",
        NR(1.0f, 1000.0f, 0.1f), 250.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "seqStep1",     1}, p + "Seq Step 1",
        NR(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "seqStep2",     1}, p + "Seq Step 2",
        NR(-12.0f, 12.0f, 0.1f), 7.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "delayFb",      1}, p + "Delay Fb",
        NR(0.0f, 85.0f, 0.1f), 30.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "delayMix",     1}, p + "Delay Mix",
        NR(0.0f, 100.0f, 0.1f), 40.0f));
}

inline void OnrushChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    paramPrefix_ = slotPrefix;
    const juce::String p = slotPrefix + "onr_";
    p_swellThresh  = apvts.getRawParameterValue(p + "swellThresh");
    p_swellAttack  = apvts.getRawParameterValue(p + "swellAttack");
    p_swellRelease = apvts.getRawParameterValue(p + "swellRelease");
    p_ringFreq     = apvts.getRawParameterValue(p + "ringFreq");
    p_ringWave     = apvts.getRawParameterValue(p + "ringWave");
    p_ringMix      = apvts.getRawParameterValue(p + "ringMix");
    p_distDrive    = apvts.getRawParameterValue(p + "distDrive");
    p_distTilt     = apvts.getRawParameterValue(p + "distTilt");
    p_distOut      = apvts.getRawParameterValue(p + "distOut");
    p_envBaseFreq  = apvts.getRawParameterValue(p + "envBaseFreq");
    p_envDepth     = apvts.getRawParameterValue(p + "envDepth");
    p_envRes       = apvts.getRawParameterValue(p + "envRes");
    p_delayBase    = apvts.getRawParameterValue(p + "delayBase");
    p_seqStep1     = apvts.getRawParameterValue(p + "seqStep1");
    p_seqStep2     = apvts.getRawParameterValue(p + "seqStep2");
    p_delayFb      = apvts.getRawParameterValue(p + "delayFb");
    p_delayMix     = apvts.getRawParameterValue(p + "delayMix");
}

} // namespace xoceanus
```

- [ ] **Step 6.2** — Create `Source/DSP/Effects/OnrushChain.cpp`:
```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#include "OnrushChain.h"
```

- [ ] **Step 6.3** — Build to verify compilation. Fix any errors.

---

### Task 7: OMNISTEREO chain (Stereo Widener)

**Files to create:**
- `Source/DSP/Effects/OmnistereoChain.h`
- `Source/DSP/Effects/OmnistereoChain.cpp`

**Spec:** Section 2.2 of `Docs/specs/2026-04-05-epic-chains-fx-design.md`
**Routing:** Stereo In → Stereo Out (true stereo throughout)
**Accent Color:** Prismatic Silver `#B0C4DE`
**Parameter prefix:** `omni_` (16 params)

**Required includes:**
```cpp
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "../Saturator.h"
#include "../StandardLFO.h"
#include "../ParameterSmoother.h"
#include "../LushReverb.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <cmath>
```

- [ ] **Step 7.1** — Write the complete `OmnistereoChain` class:

```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// [includes above]

namespace xoceanus
{

//==============================================================================
// OmnistereoChain — OMNISTEREO Stereo Widener FX Chain (5 stages)
//
// Source concept: ChromaSpace (Gemini Pedalboard Series)
// Routing: Stereo In → Stereo Out (true stereo — L and R independent throughout)
// Accent: Prismatic Silver #B0C4DE
//
// Stage 1: Tape Saturation & Hysteresis (Strymon Deco V2)
// Stage 2: Precision Parametric EQ (Boss SP-1 Spectrum)
// Stage 3: BBD Vibrato (Boss VB-2)
// Stage 4: BBD Chorus Ensemble (Boss CE-1)
// Stage 5: FDN Reverb (Meris MercuryX) — LushReverb reuse
//
// Parameter prefix: omni_ (16 params)
//==============================================================================
class OmnistereoChain
{
public:
    OmnistereoChain() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void processBlock(float* L, float* R, int numSamples);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "");
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "");

private:
    double sr_ = 44100.0;

    //==========================================================================
    // Stage 1 — Tape Saturation & Hysteresis (Strymon Deco V2)
    // Saturator tape mode + Jiles-Atherton memory + wow/flutter via filtered S&H
    //==========================================================================
    struct TapeStage
    {
        Saturator satL, satR;
        // Jiles-Atherton hysteresis memory state (per channel)
        float jaStateL = 0.0f, jaStateR = 0.0f;
        // Wow/flutter: StandardLFO in S&H → one-pole LP (ParameterSmoother ~2Hz)
        StandardLFO wowLFO_L, wowLFO_R;
        ParameterSmoother wowSmooth_L, wowSmooth_R;
        // Fractional delay line for wow/flutter (max 5ms)
        std::vector<float> wowDelayL, wowDelayR;
        int wowWriteL = 0, wowWriteR = 0;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            satL.prepare(sampleRate); satL.setMode(Saturator::SaturationMode::Tape);
            satR.prepare(sampleRate); satR.setMode(Saturator::SaturationMode::Tape);
            // S&H wow LFOs — different rates per channel for organic drift
            wowLFO_L.prepare(sampleRate); wowLFO_L.setWaveform(StandardLFO::Waveform::SampleAndHold);
            wowLFO_L.setRate(1.3f);
            wowLFO_R.prepare(sampleRate); wowLFO_R.setWaveform(StandardLFO::Waveform::SampleAndHold);
            wowLFO_R.setRate(0.9f);
            // ~2Hz one-pole lowpass for smoothed S&H (Phantom Sniff correction #2)
            wowSmooth_L.prepare(sampleRate, 0.5f); // 500ms = ~2Hz effective
            wowSmooth_R.prepare(sampleRate, 0.5f);
            int maxWowSamples = static_cast<int>(0.005 * sampleRate) + 4;
            wowDelayL.assign(maxWowSamples, 0.0f);
            wowDelayR.assign(maxWowSamples, 0.0f);
            reset();
        }
        void reset()
        {
            jaStateL = jaStateR = 0.0f;
            wowWriteL = wowWriteR = 0;
            std::fill(wowDelayL.begin(), wowDelayL.end(), 0.0f);
            std::fill(wowDelayR.begin(), wowDelayR.end(), 0.0f);
            wowSmooth_L.reset(); wowSmooth_R.reset();
            satL.reset(); satR.reset();
        }
        void processBlock(float* L, float* R, int numSamples,
                          float drive, float bias, float wowDepth)
        {
            float srF = static_cast<float>(sr);
            float maxWowSamplesF = wowDepth * 0.005f * srF; // depth % * 5ms max
            int bufSize = static_cast<int>(wowDelayL.size());

            for (int i = 0; i < numSamples; ++i)
            {
                // Wow/flutter: filtered S&H → fractional delay modulation
                float rawWowL = wowLFO_L.processSample();
                float rawWowR = wowLFO_R.processSample();
                wowSmooth_L.setTarget(rawWowL);
                wowSmooth_R.setTarget(rawWowR);
                float smoothWowL = wowSmooth_L.process();
                float smoothWowR = wowSmooth_R.process();

                float delaySamplesL = std::max(0.0f, smoothWowL * maxWowSamplesF);
                float delaySamplesR = std::max(0.0f, smoothWowR * maxWowSamplesF);

                wowDelayL[wowWriteL] = L[i];
                wowDelayR[wowWriteR] = R[i];
                // Linear interpolation read
                auto readDelayInterp = [&](const std::vector<float>& buf, int wp, float d) -> float {
                    float rF = static_cast<float>(wp) - d;
                    int sz = static_cast<int>(buf.size());
                    while (rF < 0.0f) rF += static_cast<float>(sz);
                    int r0 = static_cast<int>(rF) % sz;
                    int r1 = (r0 + 1) % sz;
                    float fr = rF - static_cast<float>(static_cast<int>(rF));
                    return buf[r0] * (1.0f - fr) + buf[r1] * fr;
                };
                float wL = readDelayInterp(wowDelayL, wowWriteL, delaySamplesL);
                float wR = readDelayInterp(wowDelayR, wowWriteR, delaySamplesR);
                wowWriteL = (wowWriteL + 1) % bufSize;
                wowWriteR = (wowWriteR + 1) % bufSize;

                // Tape saturation + JA hysteresis (simplified: one-pole memory feedback)
                float satInL = wL + jaStateL * 0.1f + bias;
                float satInR = wR + jaStateR * 0.1f + bias;
                satL.setDrive(drive); satR.setDrive(drive);
                float satOutL = satL.processSample(satInL);
                float satOutR = satR.processSample(satInR);
                jaStateL = flushDenormal(jaStateL * 0.99f + (satOutL - wL) * 0.5f);
                jaStateR = flushDenormal(jaStateR * 0.99f + (satOutR - wR) * 0.5f);

                L[i] = satOutL;
                R[i] = satOutR;
            }
        }
    } tape_;

    //==========================================================================
    // Stage 2 — Precision Parametric EQ (Boss SP-1 Spectrum)
    // CytomicSVF Peak mode — shared coefficients, L and R processed identically
    // Coefficients recomputed only on param change (not per-sample)
    //==========================================================================
    struct ParamEQStage
    {
        CytomicSVF svfL, svfR;
        float lastFreq = 0.0f, lastQ = 0.0f, lastGain = 0.0f;
        double sr = 44100.0;

        void prepare(double sampleRate) { sr = sampleRate; reset(); }
        void reset() { svfL.reset(); svfR.reset(); lastFreq = lastQ = lastGain = 0.0f; }

        void processBlock(float* L, float* R, int numSamples,
                          float freq, float q, float gainDb)
        {
            if (std::abs(gainDb) < 0.01f) return; // flat — skip
            // Recompute only on change (no per-sample coefficient updates)
            if (freq != lastFreq || q != lastQ || gainDb != lastGain)
            {
                svfL.setPeak(freq, q, gainDb, sr);
                svfR.setPeak(freq, q, gainDb, sr);
                lastFreq = freq; lastQ = q; lastGain = gainDb;
            }
            for (int i = 0; i < numSamples; ++i)
            {
                L[i] = svfL.processBell(L[i], gainDb);
                R[i] = svfR.processBell(R[i], gainDb);
            }
        }
    } peq_;

    //==========================================================================
    // Stage 3 — BBD Vibrato (Boss VB-2)
    // Short fractional delay + StandardLFO sine + 4th-order Chebyshev LP darkening
    //==========================================================================
    struct VibratoStage
    {
        static constexpr float kMaxDelayMs = 20.0f;
        std::vector<float> delayL, delayR;
        int writeL = 0, writeR = 0;
        StandardLFO lfo;
        // 4th-order Chebyshev LP (cascaded 2nd-order) for BBD bucket-loss darkening
        CytomicSVF chebLP1, chebLP2;
        // Unlatch crossfade state
        bool   latched        = true;
        float  unlatchFade    = 1.0f; // 1.0 = vibrato active, 0.0 = dry
        float  unlatchTimeMs  = 50.0f;
        float  unlatchInc     = 0.0f;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 4;
            delayL.assign(maxSamples, 0.0f);
            delayR.assign(maxSamples, 0.0f);
            lfo.prepare(sampleRate);
            lfo.setWaveform(StandardLFO::Waveform::Sine);
            chebLP1.prepare(sampleRate);
            chebLP2.prepare(sampleRate);
            unlatchInc = 1.0f / (unlatchTimeMs * 0.001f * static_cast<float>(sampleRate));
            reset();
        }
        void reset()
        {
            std::fill(delayL.begin(), delayL.end(), 0.0f);
            std::fill(delayR.begin(), delayR.end(), 0.0f);
            writeL = writeR = 0;
            unlatchFade = 1.0f;
            chebLP1.reset(); chebLP2.reset();
        }

        void processBlock(float* L, float* R, int numSamples,
                          float rateHz, float depthMs, float unlatchMs)
        {
            lfo.setRate(rateHz);
            int bufSize = static_cast<int>(delayL.size());
            float maxDepthSamples = depthMs * static_cast<float>(sr) / 1000.0f;
            // Chebyshev LP cutoff linked to depth (deeper = darker)
            float lpCutoff = std::max(1000.0f, 8000.0f - depthMs * 600.0f);
            chebLP1.setLP(lpCutoff, 1.307f, sr); // Chebyshev 4th-order pole pair 1
            chebLP2.setLP(lpCutoff, 0.541f, sr); // Chebyshev 4th-order pole pair 2

            for (int i = 0; i < numSamples; ++i)
            {
                float mod = (lfo.processSample() * 0.5f + 0.5f) * maxDepthSamples;
                mod = std::max(0.0f, std::min(mod, static_cast<float>(bufSize) - 2.0f));

                delayL[writeL] = L[i];
                delayR[writeR] = R[i];

                float rF = static_cast<float>(writeL) - mod;
                while (rF < 0.0f) rF += static_cast<float>(bufSize);
                int r0 = static_cast<int>(rF) % bufSize;
                int r1 = (r0 + 1) % bufSize;
                float fr = rF - static_cast<float>(r0);
                float wetL_raw = delayL[r0] * (1.0f - fr) + delayL[r1] * fr;
                float wetR_raw = delayR[r0] * (1.0f - fr) + delayR[r1] * fr;

                // Chebyshev LP (4th-order)
                wetL_raw = chebLP1.processLP(wetL_raw);
                wetL_raw = chebLP2.processLP(wetL_raw);
                wetR_raw = chebLP1.processLP(wetR_raw);
                wetR_raw = chebLP2.processLP(wetR_raw);

                writeL = (writeL + 1) % bufSize;
                writeR = (writeR + 1) % bufSize;

                L[i] = L[i] * (1.0f - unlatchFade) + wetL_raw * unlatchFade;
                R[i] = R[i] * (1.0f - unlatchFade) + wetR_raw * unlatchFade;
            }
        }
    } vib_;

    //==========================================================================
    // Stage 4 — BBD Chorus Ensemble (Boss CE-1)
    // L/R split with 180° LFO phase offset, compander emulation, 50% dry
    //==========================================================================
    struct ChorusStage
    {
        static constexpr float kMaxDelayMs = 30.0f;
        std::vector<float> delayL, delayR;
        int writeL = 0, writeR = 0;
        StandardLFO lfoL, lfoR;
        // Compander emulation: RMS-based compressor (input) + expander (output)
        float compStateL = 0.0f, compStateR = 0.0f;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 4;
            delayL.assign(maxSamples, 0.0f);
            delayR.assign(maxSamples, 0.0f);
            lfoL.prepare(sampleRate);
            lfoL.setWaveform(StandardLFO::Waveform::Sine);
            lfoR.prepare(sampleRate);
            lfoR.setWaveform(StandardLFO::Waveform::Sine);
            lfoR.setPhaseOffset(0.5f); // 180° offset for stereo spread
            reset();
        }
        void reset()
        {
            std::fill(delayL.begin(), delayL.end(), 0.0f);
            std::fill(delayR.begin(), delayR.end(), 0.0f);
            writeL = writeR = 0;
            compStateL = compStateR = 0.0f;
        }

        void processBlock(float* L, float* R, int numSamples,
                          float rateHz, float depthMs, float compander)
        {
            lfoL.setRate(rateHz);
            lfoR.setRate(rateHz);
            int bufSize = static_cast<int>(delayL.size());
            float maxDepthSamples = depthMs * static_cast<float>(sr) / 1000.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                // Compander: compress before delay
                float rmsCoeffAtt = FastMath::fastExp(-1.0f / (0.005f * static_cast<float>(sr)));
                float rmsCoeffRel = FastMath::fastExp(-1.0f / (0.100f * static_cast<float>(sr)));
                auto updateRMS = [&](float& state, float x) -> float {
                    float e = x * x;
                    state = flushDenormal(state * (e > state ? rmsCoeffAtt : rmsCoeffRel)
                                         + e * (1.0f - (e > state ? rmsCoeffAtt : rmsCoeffRel)));
                    return std::sqrt(std::max(state, 1e-12f));
                };
                float rmsL = updateRMS(compStateL, L[i]);
                float rmsR = updateRMS(compStateR, R[i]);

                float compIn_L = L[i] / std::max(0.01f, rmsL * compander + (1.0f - compander));
                float compIn_R = R[i] / std::max(0.01f, rmsR * compander + (1.0f - compander));

                float modL = (lfoL.processSample() * 0.5f + 0.5f) * maxDepthSamples;
                float modR = (lfoR.processSample() * 0.5f + 0.5f) * maxDepthSamples;
                modL = std::min(modL, static_cast<float>(bufSize) - 2.0f);
                modR = std::min(modR, static_cast<float>(bufSize) - 2.0f);

                delayL[writeL] = compIn_L;
                delayR[writeR] = compIn_R;

                auto readLerp = [&](const std::vector<float>& buf, int wp, float d) -> float {
                    int sz = static_cast<int>(buf.size());
                    float rF = static_cast<float>(wp) - d;
                    while (rF < 0.0f) rF += static_cast<float>(sz);
                    int r0 = static_cast<int>(rF) % sz;
                    int r1 = (r0 + 1) % sz;
                    float fr = rF - static_cast<float>(r0);
                    return buf[r0] * (1.0f - fr) + buf[r1] * fr;
                };
                float wetL = readLerp(delayL, writeL, modL);
                float wetR = readLerp(delayR, writeR, modR);

                // Expander: expand after delay
                wetL *= (rmsL * compander + (1.0f - compander));
                wetR *= (rmsR * compander + (1.0f - compander));

                writeL = (writeL + 1) % bufSize;
                writeR = (writeR + 1) % bufSize;

                // 50% dry blend (fixed, per CE-1 spec)
                L[i] = L[i] * 0.5f + wetL * 0.5f;
                R[i] = R[i] * 0.5f + wetR * 0.5f;
            }
        }
    } chorus_;

    //==========================================================================
    // Stage 5 — FDN Reverb (Meris MercuryX) — direct LushReverb reuse
    //==========================================================================
    LushReverb reverb_;

    //==========================================================================
    // Cached parameter pointers
    //==========================================================================
    std::atomic<float>* p_tapeDrive    = nullptr;
    std::atomic<float>* p_tapeBias     = nullptr;
    std::atomic<float>* p_tapeWow      = nullptr;
    std::atomic<float>* p_eqFreq       = nullptr;
    std::atomic<float>* p_eqQ          = nullptr;
    std::atomic<float>* p_eqGain       = nullptr;
    std::atomic<float>* p_vibRate      = nullptr;
    std::atomic<float>* p_vibDepth     = nullptr;
    std::atomic<float>* p_vibUnlatch   = nullptr;
    std::atomic<float>* p_choRate      = nullptr;
    std::atomic<float>* p_choDepth     = nullptr;
    std::atomic<float>* p_choCompander = nullptr;
    std::atomic<float>* p_revSize      = nullptr;
    std::atomic<float>* p_revDamp      = nullptr;
    std::atomic<float>* p_revMod       = nullptr;
    std::atomic<float>* p_revMix       = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OmnistereoChain::prepare(double sampleRate, int /*maxBlockSize*/)
{
    sr_ = sampleRate;
    tape_.prepare(sampleRate);
    peq_.prepare(sampleRate);
    vib_.prepare(sampleRate);
    chorus_.prepare(sampleRate);
    reverb_.prepare(sampleRate);
}

inline void OmnistereoChain::reset()
{
    tape_.reset(); peq_.reset(); vib_.reset(); chorus_.reset(); reverb_.reset();
}

inline void OmnistereoChain::processBlock(float* L, float* R, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;
    if (!p_tapeDrive) return;

    const float tapeDrive    = p_tapeDrive->load(std::memory_order_relaxed);
    const float tapeBias     = p_tapeBias->load(std::memory_order_relaxed);
    const float tapeWow      = p_tapeWow->load(std::memory_order_relaxed);
    const float eqFreq       = p_eqFreq->load(std::memory_order_relaxed);
    const float eqQ          = p_eqQ->load(std::memory_order_relaxed);
    const float eqGain       = p_eqGain->load(std::memory_order_relaxed);
    const float vibRate      = p_vibRate->load(std::memory_order_relaxed);
    const float vibDepth     = p_vibDepth->load(std::memory_order_relaxed);
    const float vibUnlatch   = p_vibUnlatch->load(std::memory_order_relaxed);
    const float choRate      = p_choRate->load(std::memory_order_relaxed);
    const float choDepth     = p_choDepth->load(std::memory_order_relaxed);
    const float choCompander = p_choCompander->load(std::memory_order_relaxed);
    const float revSize      = p_revSize->load(std::memory_order_relaxed);
    const float revDamp      = p_revDamp->load(std::memory_order_relaxed);
    const float revMod       = p_revMod->load(std::memory_order_relaxed);
    const float revMix       = p_revMix->load(std::memory_order_relaxed);

    // Stage 1
    tape_.processBlock(L, R, numSamples, tapeDrive, tapeBias, tapeWow * 0.01f);

    // Stage 2
    peq_.processBlock(L, R, numSamples, eqFreq, eqQ, eqGain);

    // Stage 3
    vib_.processBlock(L, R, numSamples, vibRate, vibDepth, vibUnlatch);

    // Stage 4
    chorus_.processBlock(L, R, numSamples, choRate, choDepth, choCompander * 0.01f);

    // Stage 5
    if (revMix > 0.001f)
    {
        reverb_.setSize(revSize);
        reverb_.setDamping(revDamp);
        reverb_.setModulation(revMod * 0.01f);
        reverb_.setMix(revMix * 0.01f);
        reverb_.processBlock(L, R, L, R, numSamples);
    }
}

inline void OmnistereoChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    using AP = juce::AudioParameterFloat;
    using NR = juce::NormalisableRange<float>;
    const juce::String p = slotPrefix + "omni_";

    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tapeDrive",    1}, p + "Tape Drive",
        NR(0.0f, 24.0f, 0.1f), 6.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tapeBias",     1}, p + "Tape Bias",
        NR(-0.5f, 0.5f, 0.001f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tapeWow",      1}, p + "Tape Wow",
        NR(0.0f, 100.0f, 0.1f), 20.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "eqFreq",       1}, p + "EQ Freq",
        NR(100.0f, 10000.0f, 1.0f), 1000.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "eqQ",          1}, p + "EQ Q",
        NR(0.1f, 10.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "eqGain",       1}, p + "EQ Gain",
        NR(-15.0f, 15.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "vibRate",      1}, p + "Vib Rate",
        NR(0.1f, 10.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "vibDepth",     1}, p + "Vib Depth",
        NR(0.0f, 5.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "vibUnlatch",   1}, p + "Vib Unlatch",
        NR(1.0f, 200.0f, 1.0f), 50.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "choRate",      1}, p + "Cho Rate",
        NR(0.1f, 5.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "choDepth",     1}, p + "Cho Depth",
        NR(0.0f, 15.0f, 0.01f), 5.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "choCompander", 1}, p + "Cho Compander",
        NR(0.0f, 100.0f, 0.1f), 30.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revSize",      1}, p + "Rev Size",
        NR(0.0f, 1.0f, 0.001f), 0.5f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revDamp",      1}, p + "Rev Damp",
        NR(500.0f, 16000.0f, 1.0f), 5000.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revMod",       1}, p + "Rev Mod",
        NR(0.0f, 100.0f, 0.1f), 20.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revMix",       1}, p + "Rev Mix",
        NR(0.0f, 100.0f, 0.1f), 0.0f));
}

inline void OmnistereoChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "omni_";
    p_tapeDrive    = apvts.getRawParameterValue(p + "tapeDrive");
    p_tapeBias     = apvts.getRawParameterValue(p + "tapeBias");
    p_tapeWow      = apvts.getRawParameterValue(p + "tapeWow");
    p_eqFreq       = apvts.getRawParameterValue(p + "eqFreq");
    p_eqQ          = apvts.getRawParameterValue(p + "eqQ");
    p_eqGain       = apvts.getRawParameterValue(p + "eqGain");
    p_vibRate      = apvts.getRawParameterValue(p + "vibRate");
    p_vibDepth     = apvts.getRawParameterValue(p + "vibDepth");
    p_vibUnlatch   = apvts.getRawParameterValue(p + "vibUnlatch");
    p_choRate      = apvts.getRawParameterValue(p + "choRate");
    p_choDepth     = apvts.getRawParameterValue(p + "choDepth");
    p_choCompander = apvts.getRawParameterValue(p + "choCompander");
    p_revSize      = apvts.getRawParameterValue(p + "revSize");
    p_revDamp      = apvts.getRawParameterValue(p + "revDamp");
    p_revMod       = apvts.getRawParameterValue(p + "revMod");
    p_revMix       = apvts.getRawParameterValue(p + "revMix");
}

} // namespace xoceanus
```

- [ ] **Step 7.2** — Create `Source/DSP/Effects/OmnistereoChain.cpp`:
```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#include "OmnistereoChain.h"
```

- [ ] **Step 7.3** — Build to verify. Fix errors.

---

### Task 8: OBLITERATE chain (Heavy Stutter)

**Files to create:**
- `Source/DSP/Effects/ObliterateChain.h`
- `Source/DSP/Effects/ObliterateChain.cpp`

**Spec:** Section 2.3 of `Docs/specs/2026-04-05-epic-chains-fx-design.md`
**Routing:** Mono In → Stereo Out
**Accent Color:** Apocalypse Red `#8B0000`
**Parameter prefix:** `oblt_` (19 params)

**Required includes:**
```cpp
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "../Saturator.h"
#include "../StandardLFO.h"
#include "../ParameterSmoother.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <random>
#include <cmath>
```

- [ ] **Step 8.1** — Write the complete `ObliterateChain` class with 5 stage structs:

**Class skeleton:**
```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// [includes above]

namespace xoceanus
{

//==============================================================================
// ObliterateChain — OBLITERATE Heavy Stutter FX Chain (5 stages)
//
// Source concept: GritGlitch (Gemini Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 4 granular scatter)
// Accent: Apocalypse Red #8B0000
//
// Stage 1: Shimmer Reverb (OBNE Dark Star) — mini 4-tap FDN + PSOLA pitch shifter
// Stage 2: Octave Fuzz (Boss FZ-2) — abs(x) rectification + 8x OVS + Baxandall EQ
// Stage 3: Reverse Pitch Delay (Boss PS-3) — dual circular buffers + Hann crossfade
// Stage 4: Granular Looper (Hologram Microcosm) — GrainScheduler + stochastic pan
// Stage 5: Hard-Chopped Tremolo (Boss PN-2) — shape-select LFO + ping-pong L/R
//
// Parameter prefix: oblt_ (19 params)
//==============================================================================
class ObliterateChain
{
public:
    ObliterateChain() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // monoIn: single channel input. L/R: stereo output.
    void processBlock(const float* monoIn, float* L, float* R, int numSamples);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "");
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "");

private:
    double sr_ = 44100.0;

    //==========================================================================
    // Stage 1 — Shimmer Reverb (OBNE Dark Star)
    // 4-tap FDN + PSOLA pitch shifter in feedback (dual crossfading delay lines)
    //==========================================================================
    struct ShimmerReverbStage
    {
        static constexpr int kNumTaps = 4;
        static constexpr int kPrimeTaps[kNumTaps]; // defined below class

        std::array<std::vector<float>, kNumTaps> fdnBufs;
        std::array<int, kNumTaps> fdnWritePos{};
        std::array<float, kNumTaps> fdnState{};

        // PSOLA pitch shifter: dual crossfading delay lines
        std::vector<float> psola1, psola2;
        int  psolaWrite1 = 0, psolaWrite2 = 0;
        float psolaPhase = 0.0f;   // 0.0→1.0 grain phase
        bool  psolaToggle = false;  // which buffer is fading in
        float psolaGrain1 = 1.0f;  // fade state for buffer 1
        float psolaGrain2 = 0.0f;  // fade state for buffer 2
        float feedbackMix = 0.5f;

        double sr = 44100.0;

        void prepare(double sampleRate);
        void reset();

        float processSample(float in, float decaySec, int semitones, double sampleRate);
    } shimmer_;

    //==========================================================================
    // Stage 2 — Octave Fuzz (Boss FZ-2 Hyper Fuzz)
    // Full-wave rect + 8x oversampling + Baxandall tone stack
    //==========================================================================
    struct OctaveFuzzStage
    {
        CytomicSVF baxTreble, baxBass;
        // 8x oversampling state: 3 stages × 2 (up/down)
        std::array<float, 6> aaStateUp{};
        std::array<float, 6> aaStateDn{};
        double sr = 44100.0;

        void prepare(double sampleRate) { sr = sampleRate; reset(); }
        void reset()
        {
            baxTreble.reset(); baxBass.reset();
            aaStateUp.fill(0.0f); aaStateDn.fill(0.0f);
        }

        // Full-wave rectification: abs(x) * gain (oversampled)
        float process(float in, float gainLinear, float trebleDb, float bassDb)
        {
            // 8x oversampling: upsample 3× via linear interpolation
            float samples[8];
            float prev = aaStateUp[0];
            for (int k = 0; k < 8; ++k)
            {
                float t = static_cast<float>(k) / 8.0f;
                samples[k] = prev + t * (in - prev);
            }
            aaStateUp[0] = in;

            // Apply nonlinearity at 8x rate
            for (int k = 0; k < 8; ++k)
                samples[k] = std::abs(samples[k]) * gainLinear;

            // Downsample 8→1 (averaging)
            float out = 0.0f;
            for (int k = 0; k < 8; ++k) out += samples[k];
            out *= (1.0f / 8.0f);

            // Baxandall tone stack
            baxBass.setLowShelf(200.0f, 0.707f, bassDb, sr);
            baxTreble.setHighShelf(3000.0f, 0.707f, trebleDb, sr);
            out = baxBass.processLP(out);
            out = baxTreble.processHP(out); // high shelf applied to HP pass
            return out;
        }
    } fuzz_;

    //==========================================================================
    // Stage 3 — Reverse Pitch Delay (Boss PS-3 Mode 7)
    // Dual circular buffers: Buffer A writes forward, Buffer B reads backward
    //==========================================================================
    struct ReversePitchDelayStage
    {
        static constexpr float kMaxDelayMs = 2000.0f;
        std::vector<float> bufA, bufB;
        int  writeA = 0, writeB = 0;
        int  readBPos = 0;       // backward read pointer
        float hannPhase = 0.0f;  // Hann window crossfade phase
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 4;
            bufA.assign(maxSamples, 0.0f);
            bufB.assign(maxSamples, 0.0f);
            reset();
        }
        void reset()
        {
            std::fill(bufA.begin(), bufA.end(), 0.0f);
            std::fill(bufB.begin(), bufB.end(), 0.0f);
            writeA = writeB = hannPhase = 0;
            readBPos = 0;
        }

        // Hann window: 0.5 * (1 - cos(2π * phase))
        static float hann(float phase) { return 0.5f * (1.0f - std::cos(6.28318f * phase)); }

        float process(float in, float delayMs, float pitchRatio, float mix)
        {
            int bufSize  = static_cast<int>(bufA.size());
            int loopLen  = std::max(1, static_cast<int>(delayMs * static_cast<float>(sr) / 1000.0f));
            loopLen = std::min(loopLen, bufSize - 1);

            // Write forward into A
            bufA[writeA] = in;
            // Write forward into B (same as A — B is read backward)
            bufB[writeB] = in;
            writeA = (writeA + 1) % bufSize;
            writeB = (writeB + 1) % bufSize;

            // Read backward from B with pitch rate
            int backreadPos = ((writeB - readBPos + bufSize) % bufSize);
            float wetRaw = bufB[backreadPos];
            float wetHann = wetRaw * hann(hannPhase);

            // Advance backward read pointer (rate controls pitch)
            readBPos = static_cast<int>(readBPos + pitchRatio) % loopLen;

            // Update Hann phase
            hannPhase = std::fmod(hannPhase + (pitchRatio / static_cast<float>(loopLen)), 1.0f);

            return in + mix * (wetHann - in);
        }
    } revDelay_;

    //==========================================================================
    // Stage 4 — Granular Looper (Hologram Microcosm)
    // GrainScheduler + stochastic panning → Mono → Stereo here
    //==========================================================================
    struct GranularStage
    {
        static constexpr int kMaxGrains = 24;
        static constexpr float kCaptureMs = 500.0f;

        struct Grain
        {
            int   startIdx   = 0;
            int   length     = 0;      // in samples
            int   pos        = 0;      // current read position within grain
            float pan        = 0.0f;   // -1.0 to +1.0
            float ampL       = 1.0f;
            float ampR       = 1.0f;
            bool  active     = false;
        };

        std::array<Grain, kMaxGrains> grains{};
        std::vector<float> captureBuf;  // capture ring buffer
        int captureWrite = 0;
        int captureLen   = 0;

        float spawnAccum = 0.0f;  // fractional sample accumulator for spawn rate
        juce::Random rng{};

        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            captureLen = static_cast<int>(kCaptureMs * sampleRate / 1000.0);
            captureBuf.assign(captureLen, 0.0f);
            reset();
        }
        void reset()
        {
            std::fill(captureBuf.begin(), captureBuf.end(), 0.0f);
            captureWrite = 0;
            for (auto& g : grains) g.active = false;
            spawnAccum = 0.0f;
        }

        // Gaussian envelope approximation using Hann
        static float gaussianEnv(int pos, int length)
        {
            float t = static_cast<float>(pos) / static_cast<float>(std::max(1, length));
            return 0.5f * (1.0f - std::cos(6.28318f * t));
        }

        void spawnGrain(float grainSizeMs, float scatter)
        {
            for (auto& g : grains)
            {
                if (!g.active)
                {
                    int len = static_cast<int>(grainSizeMs * static_cast<float>(sr) / 1000.0f);
                    len = std::max(1, std::min(len, captureLen - 1));
                    // Random start position in capture buffer
                    int maxStart = std::max(1, captureLen - len);
                    g.startIdx = rng.nextInt(maxStart);
                    g.length   = len;
                    g.pos      = 0;
                    g.pan      = (rng.nextFloat() * 2.0f - 1.0f) * scatter;
                    // Equal-power pan
                    float angle = (g.pan + 1.0f) * 0.5f * 1.5707963f;
                    g.ampL = std::cos(angle);
                    g.ampR = std::sin(angle);
                    g.active = true;
                    break;
                }
            }
        }

        void processBlock(const float* monoIn, float* L, float* R, int numSamples,
                          float densityHz, float grainSizeMs, float scatter, float mix)
        {
            if (densityHz < 0.001f || mix < 0.001f)
            {
                for (int i = 0; i < numSamples; ++i) { L[i] = monoIn[i]; R[i] = monoIn[i]; }
                return;
            }

            float samplesPerGrain = static_cast<float>(sr) / std::max(0.01f, densityHz);
            float scatterNorm = scatter * 0.01f;

            for (int i = 0; i < numSamples; ++i)
            {
                // Feed capture buffer
                captureBuf[captureWrite] = monoIn[i];
                captureWrite = (captureWrite + 1) % captureLen;

                // Spawn grains at density rate
                spawnAccum += 1.0f;
                if (spawnAccum >= samplesPerGrain)
                {
                    spawnAccum -= samplesPerGrain;
                    spawnGrain(grainSizeMs, scatterNorm);
                }

                // Sum active grains
                float sumL = 0.0f, sumR = 0.0f;
                for (auto& g : grains)
                {
                    if (!g.active) continue;
                    int readIdx = (g.startIdx + g.pos) % captureLen;
                    float sample = captureBuf[readIdx] * gaussianEnv(g.pos, g.length);
                    sumL += sample * g.ampL;
                    sumR += sample * g.ampR;
                    g.pos++;
                    if (g.pos >= g.length) g.active = false;
                }

                float normFactor = 1.0f / std::max(1.0f, static_cast<float>(kMaxGrains) * 0.5f);
                L[i] = monoIn[i] + mix * (sumL * normFactor - monoIn[i]);
                R[i] = monoIn[i] + mix * (sumR * normFactor - monoIn[i]);
            }
        }
    } granular_;

    //==========================================================================
    // Stage 5 — Hard-Chopped Tremolo (Boss PN-2)
    // StandardLFO shape select + slew limiter + stereo ping-pong phase offset
    //==========================================================================
    struct TremoloStage
    {
        StandardLFO lfoL, lfoR;
        ParameterSmoother gainSmoothL, gainSmoothR;  // per-channel stereo

        void prepare(double sampleRate)
        {
            lfoL.prepare(sampleRate); lfoL.setWaveform(StandardLFO::Waveform::Triangle);
            lfoR.prepare(sampleRate); lfoR.setWaveform(StandardLFO::Waveform::Triangle);
            // Slew limiter via smoother: fast enough to preserve hard-chop character
            gainSmoothL.prepare(sampleRate, 0.002f); // 2ms slew
            gainSmoothR.prepare(sampleRate, 0.002f);
        }
        void reset()
        {
            gainSmoothL.reset(); gainSmoothR.reset();
        }

        void processBlock(float* L, float* R, int numSamples,
                          float rateHz, float shape, float width, float phaseOffsetDeg, float mix)
        {
            // shape: 0.0 = Triangle, 1.0 = Square
            auto wform = (shape > 0.5f) ? StandardLFO::Waveform::Square
                                        : StandardLFO::Waveform::Triangle;
            lfoL.setWaveform(wform); lfoR.setWaveform(wform);
            lfoL.setRate(rateHz); lfoR.setRate(rateHz);
            // Phase offset for R channel (ping-pong)
            lfoR.setPhaseOffset(phaseOffsetDeg / 360.0f);

            float widthN = width * 0.01f;

            for (int i = 0; i < numSamples; ++i)
            {
                float lfoOutL = (lfoL.processSample() * 0.5f + 0.5f);
                float lfoOutR = (lfoR.processSample() * 0.5f + 0.5f);
                // Apply depth (width)
                float gainL = 1.0f - widthN * (1.0f - lfoOutL);
                float gainR = 1.0f - widthN * (1.0f - lfoOutR);
                // Slew limit (prevents clicks on square wave without losing chop)
                gainSmoothL.setTarget(gainL); gainSmoothR.setTarget(gainR);
                float slewL = gainSmoothL.process();
                float slewR = gainSmoothR.process();

                float wetL = L[i] * slewL;
                float wetR = R[i] * slewR;
                L[i] = L[i] * (1.0f - mix) + wetL * mix;
                R[i] = R[i] * (1.0f - mix) + wetR * mix;
            }
        }
    } trem_;

    //==========================================================================
    // Cached parameter pointers (19 params)
    //==========================================================================
    std::atomic<float>* p_shimInterval   = nullptr;
    std::atomic<float>* p_shimDecay      = nullptr;
    std::atomic<float>* p_shimMix        = nullptr;
    std::atomic<float>* p_fuzzGain       = nullptr;
    std::atomic<float>* p_fuzzRectMix    = nullptr;
    std::atomic<float>* p_fuzzTreble     = nullptr;
    std::atomic<float>* p_fuzzBass       = nullptr;
    std::atomic<float>* p_revDelayTime   = nullptr;
    std::atomic<float>* p_revDelayPitch  = nullptr;
    std::atomic<float>* p_revDelayMix    = nullptr;
    std::atomic<float>* p_granDensity    = nullptr;
    std::atomic<float>* p_granSize       = nullptr;
    std::atomic<float>* p_granScatter    = nullptr;
    std::atomic<float>* p_granMix        = nullptr;
    std::atomic<float>* p_tremRate       = nullptr;
    std::atomic<float>* p_tremShape      = nullptr;
    std::atomic<float>* p_tremWidth      = nullptr;
    std::atomic<float>* p_tremPhase      = nullptr;
    std::atomic<float>* p_tremMix        = nullptr;
};

// ShimmerReverbStage prime tap lengths (samples at 48kHz, scaled in prepare())
constexpr int ObliterateChain::ShimmerReverbStage::kPrimeTaps[kNumTaps] = {
    1021, 1327, 1847, 2503
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void ObliterateChain::ShimmerReverbStage::prepare(double sampleRate)
{
    sr = sampleRate;
    float srScale = static_cast<float>(sampleRate) / 48000.0f;
    for (int t = 0; t < kNumTaps; ++t)
    {
        int len = static_cast<int>(kPrimeTaps[t] * srScale) + 2;
        fdnBufs[t].assign(len, 0.0f);
        fdnWritePos[t] = 0;
        fdnState[t]    = 0.0f;
    }
    // PSOLA pitch shifter buffers: 2× largest tap length
    int psolaLen = static_cast<int>(kPrimeTaps[kNumTaps - 1] * srScale * 2) + 4;
    psola1.assign(psolaLen, 0.0f);
    psola2.assign(psolaLen, 0.0f);
    psolaWrite1 = psolaWrite2 = 0;
    psolaPhase  = 0.0f;
    psolaToggle = false;
    psolaGrain1 = 1.0f; psolaGrain2 = 0.0f;
}

inline void ObliterateChain::ShimmerReverbStage::reset()
{
    for (int t = 0; t < kNumTaps; ++t)
    {
        std::fill(fdnBufs[t].begin(), fdnBufs[t].end(), 0.0f);
        fdnWritePos[t] = 0;
        fdnState[t]    = 0.0f;
    }
    std::fill(psola1.begin(), psola1.end(), 0.0f);
    std::fill(psola2.begin(), psola2.end(), 0.0f);
    psolaWrite1 = psolaWrite2 = 0;
    psolaPhase  = 0.0f;
    psolaToggle = false;
    psolaGrain1 = 1.0f; psolaGrain2 = 0.0f;
}

inline float ObliterateChain::ShimmerReverbStage::processSample(
    float in, float decaySec, int semitones, double sampleRate)
{
    float srF = static_cast<float>(sampleRate);
    float srScale = srF / 48000.0f;
    float decayCoeff = FastMath::fastExp(-1.0f / (decaySec * srF));

    // PSOLA pitch shift: ratio = 2^(semitones/12)
    float pitchRatio = FastMath::fastPow2(static_cast<float>(semitones) / 12.0f);

    // Read from FDN taps
    float fdnSum = 0.0f;
    for (int t = 0; t < kNumTaps; ++t)
    {
        int len = static_cast<int>(fdnBufs[t].size());
        int effectiveDelay = static_cast<int>(kPrimeTaps[t] * srScale);
        effectiveDelay = juce::jlimit(1, len - 1, effectiveDelay);
        int rp = ((fdnWritePos[t] - effectiveDelay) + len) % len;
        fdnState[t] = flushDenormal(fdnState[t] * decayCoeff + fdnBufs[t][rp] * (1.0f - decayCoeff));
        fdnSum += fdnState[t];
    }
    fdnSum /= static_cast<float>(kNumTaps);

    // PSOLA pitch-shifted feedback (50% of reverb tail)
    // Grain crossfade phase advance
    float grainLen = srF * 0.05f; // 50ms grains
    psolaPhase += pitchRatio / grainLen;
    if (psolaPhase >= 1.0f)
    {
        psolaPhase -= 1.0f;
        psolaToggle = !psolaToggle;
        if (psolaToggle) { psolaGrain1 = 0.0f; psolaGrain2 = 1.0f; }
        else             { psolaGrain1 = 1.0f; psolaGrain2 = 0.0f; }
    }
    // Crossfade grains
    float grainFade1 = 0.5f * (1.0f - std::cos(6.28318f * std::min(psolaPhase, 1.0f)));
    float psolaOut = 0.0f;
    int p1Len = static_cast<int>(psola1.size());
    int p2Len = static_cast<int>(psola2.size());
    if (p1Len > 0) psolaOut += psola1[psolaWrite1 % p1Len] * grainFade1;
    if (p2Len > 0) psolaOut += psola2[psolaWrite2 % p2Len] * (1.0f - grainFade1);

    float feedback = fdnSum * 0.5f + psolaOut * 0.5f;

    // Write into FDN delay lines
    float fdnIn = in + feedback * 0.5f;
    for (int t = 0; t < kNumTaps; ++t)
    {
        int len = static_cast<int>(fdnBufs[t].size());
        fdnBufs[t][fdnWritePos[t]] = flushDenormal(fdnIn);
        fdnWritePos[t] = (fdnWritePos[t] + 1) % len;
    }
    // Write pitch-shifted into PSOLA buffers
    if (p1Len > 0) psola1[psolaWrite1 % p1Len] = flushDenormal(fdnSum);
    if (p2Len > 0) psola2[psolaWrite2 % p2Len] = flushDenormal(fdnSum);
    psolaWrite1 = (psolaWrite1 + 1) % std::max(1, p1Len);
    psolaWrite2 = (psolaWrite2 + 1) % std::max(1, p2Len);

    return fdnSum;
}

inline void ObliterateChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_ = sampleRate;
    shimmer_.prepare(sampleRate);
    fuzz_.prepare(sampleRate);
    revDelay_.prepare(sampleRate);
    granular_.prepare(sampleRate, maxBlockSize);
    trem_.prepare(sampleRate);
}

inline void ObliterateChain::reset()
{
    shimmer_.reset();
    fuzz_.reset();
    revDelay_.reset();
    granular_.reset();
    trem_.reset();
}

inline void ObliterateChain::processBlock(const float* monoIn, float* L, float* R, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;
    if (!p_shimInterval) return;

    const float shimInterval  = p_shimInterval->load(std::memory_order_relaxed);
    const float shimDecay     = p_shimDecay->load(std::memory_order_relaxed);
    const float shimMix       = p_shimMix->load(std::memory_order_relaxed);
    const float fuzzGain      = p_fuzzGain->load(std::memory_order_relaxed);
    const float fuzzRectMix   = p_fuzzRectMix->load(std::memory_order_relaxed);
    const float fuzzTreble    = p_fuzzTreble->load(std::memory_order_relaxed);
    const float fuzzBass      = p_fuzzBass->load(std::memory_order_relaxed);
    const float revDelayTime  = p_revDelayTime->load(std::memory_order_relaxed);
    const float revDelayPitch = p_revDelayPitch->load(std::memory_order_relaxed);
    const float revDelayMix   = p_revDelayMix->load(std::memory_order_relaxed);
    const float granDensity   = p_granDensity->load(std::memory_order_relaxed);
    const float granSize      = p_granSize->load(std::memory_order_relaxed);
    const float granScatter   = p_granScatter->load(std::memory_order_relaxed);
    const float granMix       = p_granMix->load(std::memory_order_relaxed);
    const float tremRate      = p_tremRate->load(std::memory_order_relaxed);
    const float tremShape     = p_tremShape->load(std::memory_order_relaxed);
    const float tremWidth     = p_tremWidth->load(std::memory_order_relaxed);
    const float tremPhase     = p_tremPhase->load(std::memory_order_relaxed);
    const float tremMix       = p_tremMix->load(std::memory_order_relaxed);

    float fuzzGainLin = FastMath::fastPow2(fuzzGain * (1.0f / 6.0205999f));

    // Temp mono buffer (use L as scratch through stages 1-3)
    for (int i = 0; i < numSamples; ++i) L[i] = monoIn[i];

    // Stage 1: Shimmer Reverb
    if (shimMix > 0.001f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float shimOut = shimmer_.processSample(L[i], shimDecay, static_cast<int>(shimInterval), sr_);
            L[i] = L[i] + shimMix * 0.01f * (shimOut - L[i]);
        }
    }

    // Stage 2: Octave Fuzz
    for (int i = 0; i < numSamples; ++i)
    {
        float fuzzOut = fuzz_.process(L[i], fuzzGainLin, fuzzTreble, fuzzBass);
        L[i] = L[i] + fuzzRectMix * 0.01f * (fuzzOut - L[i]);
    }

    // Stage 3: Reverse Pitch Delay
    for (int i = 0; i < numSamples; ++i)
        L[i] = revDelay_.process(L[i], revDelayTime, revDelayPitch, revDelayMix * 0.01f);

    // Stage 4: Granular Looper — Mono → Stereo expansion
    granular_.processBlock(L, L, R, numSamples, granDensity, granSize, granScatter, granMix * 0.01f);

    // Stage 5: Hard-Chopped Tremolo (stereo)
    trem_.processBlock(L, R, numSamples, tremRate, tremShape, tremWidth, tremPhase, tremMix * 0.01f);
}

inline void ObliterateChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    using AP = juce::AudioParameterFloat;
    using NR = juce::NormalisableRange<float>;
    const juce::String p = slotPrefix + "oblt_";

    layout.add(std::make_unique<AP>(juce::ParameterID{p + "shimInterval",  1}, p + "Shim Interval",
        NR(-12.0f, 12.0f, 1.0f), 12.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "shimDecay",     1}, p + "Shim Decay",
        NR(0.1f, 10.0f, 0.01f), 2.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "shimMix",       1}, p + "Shim Mix",
        NR(0.0f, 100.0f, 0.1f), 30.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "fuzzGain",      1}, p + "Fuzz Gain",
        NR(0.0f, 40.0f, 0.1f), 12.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "fuzzRectMix",   1}, p + "Fuzz Rect Mix",
        NR(0.0f, 100.0f, 0.1f), 50.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "fuzzTreble",    1}, p + "Fuzz Treble",
        NR(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "fuzzBass",      1}, p + "Fuzz Bass",
        NR(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revDelayTime",  1}, p + "Rev Delay Time",
        NR(50.0f, 2000.0f, 1.0f), 500.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revDelayPitch", 1}, p + "Rev Delay Pitch",
        NR(0.25f, 4.0f, 0.001f), 1.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revDelayMix",   1}, p + "Rev Delay Mix",
        NR(0.0f, 100.0f, 0.1f), 40.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "granDensity",   1}, p + "Gran Density",
        NR(0.5f, 50.0f, 0.1f), 8.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "granSize",      1}, p + "Gran Size",
        NR(10.0f, 100.0f, 1.0f), 40.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "granScatter",   1}, p + "Gran Scatter",
        NR(0.0f, 100.0f, 0.1f), 50.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "granMix",       1}, p + "Gran Mix",
        NR(0.0f, 100.0f, 0.1f), 40.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tremRate",      1}, p + "Trem Rate",
        NR(0.1f, 20.0f, 0.01f), 4.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tremShape",     1}, p + "Trem Shape",
        NR(0.0f, 1.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tremWidth",     1}, p + "Trem Width",
        NR(0.0f, 100.0f, 0.1f), 80.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tremPhase",     1}, p + "Trem Phase",
        NR(0.0f, 360.0f, 1.0f), 180.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tremMix",       1}, p + "Trem Mix",
        NR(0.0f, 100.0f, 0.1f), 60.0f));
}

inline void ObliterateChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "oblt_";
    p_shimInterval   = apvts.getRawParameterValue(p + "shimInterval");
    p_shimDecay      = apvts.getRawParameterValue(p + "shimDecay");
    p_shimMix        = apvts.getRawParameterValue(p + "shimMix");
    p_fuzzGain       = apvts.getRawParameterValue(p + "fuzzGain");
    p_fuzzRectMix    = apvts.getRawParameterValue(p + "fuzzRectMix");
    p_fuzzTreble     = apvts.getRawParameterValue(p + "fuzzTreble");
    p_fuzzBass       = apvts.getRawParameterValue(p + "fuzzBass");
    p_revDelayTime   = apvts.getRawParameterValue(p + "revDelayTime");
    p_revDelayPitch  = apvts.getRawParameterValue(p + "revDelayPitch");
    p_revDelayMix    = apvts.getRawParameterValue(p + "revDelayMix");
    p_granDensity    = apvts.getRawParameterValue(p + "granDensity");
    p_granSize       = apvts.getRawParameterValue(p + "granSize");
    p_granScatter    = apvts.getRawParameterValue(p + "granScatter");
    p_granMix        = apvts.getRawParameterValue(p + "granMix");
    p_tremRate       = apvts.getRawParameterValue(p + "tremRate");
    p_tremShape      = apvts.getRawParameterValue(p + "tremShape");
    p_tremWidth      = apvts.getRawParameterValue(p + "tremWidth");
    p_tremPhase      = apvts.getRawParameterValue(p + "tremPhase");
    p_tremMix        = apvts.getRawParameterValue(p + "tremMix");
}

} // namespace xoceanus
```

> **Note on GranularStage prepare:** The granular stage's `prepare()` needs `maxBlockSize` passed through. Update the `GranularStage::prepare(double sampleRate, int /*maxBlockSize*/)` signature to match.

- [ ] **Step 8.2** — Create `Source/DSP/Effects/ObliterateChain.cpp`:
```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#include "ObliterateChain.h"
```

- [ ] **Step 8.3** — Build to verify. Fix errors.

---

### Task 9: OBSCURITY chain (Dark Ambient)

**Files to create:**
- `Source/DSP/Effects/ObscurityChain.h`
- `Source/DSP/Effects/ObscurityChain.cpp`

**Spec:** Section 2.4 of `Docs/specs/2026-04-05-epic-chains-fx-design.md`
**Routing:** Mono In → Stereo Out
**Accent Color:** Void Purple `#2D1B69`
**Parameter prefix:** `obsc_` (20 params)

**Required includes:**
```cpp
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "../PolyBLEP.h"
#include "../StandardLFO.h"
#include "../ParameterSmoother.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <array>
#include <cmath>
```

- [ ] **Step 9.1** — Write the complete `ObscurityChain` class:

```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// [includes above]

namespace xoceanus
{

//==============================================================================
// ObscurityChain — OBSCURITY Dark Ambient FX Chain (5 stages)
//
// Source concept: VoidAtmosphere (Gemini Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 3 dimension chorus)
// Accent: Void Purple #2D1B69
//
// Stage 1: PLL Synthesizer (EQD Data Corrupter) — LP→Schmitt→period→PolyBLEP
// Stage 2: Asymmetric Diode OD (Boss PW-2) — positive/negative mismatched clip
// Stage 3: Dimension Chorus (Boss DC-2) — dual fractional delay + 180° phase split
// Stage 4: Degrading BBD Delay (Boss DM-1) — BBD bandwidth loss + tanh accumulation
// Stage 5: Industrial Multi-Tap Reverb (Death By Audio Rooms) — 8 prime taps
//
// Parameter prefix: obsc_ (20 params)
//==============================================================================
class ObscurityChain
{
public:
    ObscurityChain() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // monoIn: single channel. L/R: stereo output.
    void processBlock(const float* monoIn, float* L, float* R, int numSamples);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "");
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "");

private:
    double sr_ = 44100.0;

    //==========================================================================
    // Stage 1 — PLL Synthesizer (EQD Data Corrupter)
    // Extreme LP → Schmitt trigger → period measurement → PolyBLEP square
    //==========================================================================
    struct PLLStage
    {
        // Extreme LP (one-pole, very low cutoff)
        float lpState = 0.0f;
        // Schmitt trigger
        bool  schmittState = false;
        // Period measurement
        int   periodCounter  = 0;
        int   lastPeriod     = 0;
        // PolyBLEP oscillators
        PolyBLEP squareOsc;
        PolyBLEP sub1Osc;   // period>>1 (sub octave 1)
        PolyBLEP sub2Osc;   // period>>2 (sub octave 2)
        ParameterSmoother freqGlide; // frequency smoothing for glide
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            squareOsc.prepare(sampleRate); squareOsc.setWaveform(PolyBLEP::Waveform::Square);
            sub1Osc.prepare(sampleRate);   sub1Osc.setWaveform(PolyBLEP::Waveform::Square);
            sub2Osc.prepare(sampleRate);   sub2Osc.setWaveform(PolyBLEP::Waveform::Square);
            freqGlide.prepare(sampleRate, 0.100f); // 100ms default glide
            reset();
        }
        void reset()
        {
            lpState = 0.0f;
            schmittState = false;
            periodCounter = lastPeriod = 0;
            freqGlide.reset();
        }
        void setGlideMs(float ms)
        {
            freqGlide.prepare(sr, ms * 0.001f);
        }

        float process(float in, float glideMs, float sub1Db, float sub2Db, float squareDb)
        {
            setGlideMs(glideMs);
            // Extreme LP (fc ≈ 200Hz tracking fundamental)
            float lpCoeff = FastMath::fastExp(-2.0f * 3.14159f * 200.0f / static_cast<float>(sr));
            lpState = flushDenormal(lpState * lpCoeff + in * (1.0f - lpCoeff));

            // Schmitt trigger zero-crossing (hysteresis ±0.05)
            bool prevState = schmittState;
            if (schmittState && lpState < -0.05f) schmittState = false;
            if (!schmittState && lpState > 0.05f)  schmittState = true;

            periodCounter++;
            if (schmittState && !prevState) // rising edge
            {
                // Period detected — update PLL frequency
                if (periodCounter > 0)
                {
                    lastPeriod = periodCounter;
                    periodCounter = 0;
                    float freq = static_cast<float>(sr) / static_cast<float>(lastPeriod);
                    freq = std::max(20.0f, std::min(freq, 2000.0f));
                    freqGlide.setTarget(freq);
                }
            }

            float smoothedFreq = freqGlide.process();
            squareOsc.setFrequency(smoothedFreq);
            sub1Osc.setFrequency(smoothedFreq * 0.5f);   // 1 octave down
            sub2Osc.setFrequency(smoothedFreq * 0.25f);  // 2 octaves down

            // Mix sub-oscillators
            float sq  = squareOsc.processSample();
            float s1  = sub1Osc.processSample();
            float s2  = sub2Osc.processSample();

            float sq_lin  = FastMath::fastPow2(squareDb * (1.0f / 6.0205999f));
            float s1_lin  = FastMath::fastPow2(sub1Db   * (1.0f / 6.0205999f));
            float s2_lin  = FastMath::fastPow2(sub2Db   * (1.0f / 6.0205999f));

            return in + sq * sq_lin + s1 * s1_lin + s2 * s2_lin;
        }
    } pll_;

    //==========================================================================
    // Stage 2 — Asymmetric Diode Overdrive (Boss PW-2)
    // Positive: 1 - exp(-x * drive)  |  Negative: -1 + exp(x * drive * 0.5)
    // 4x oversampling + pre-clip LP shelf ("Fat") + post-clip peak ("Muscle")
    //==========================================================================
    struct AsymDiodeStage
    {
        CytomicSVF fatSVF;    // Pre-clip resonant low-shelf
        CytomicSVF muscleSVF; // Post-clip wide-Q peak
        float aaState1 = 0.0f, aaState2 = 0.0f; // AA filter states
        double sr = 44100.0;

        void prepare(double sampleRate) { sr = sampleRate; reset(); }
        void reset() { fatSVF.reset(); muscleSVF.reset(); aaState1 = aaState2 = 0.0f; }

        static float diodeClip(float x, float drive)
        {
            if (x >= 0.0f)
                return 1.0f - FastMath::fastExp(-x * drive);
            else
                return -1.0f + FastMath::fastExp(x * drive * 0.5f);
        }

        float process(float in, float drive, float fatFreq, float fatRes,
                      float muscleFreq, float muscleGain, float outDb)
        {
            // Pre-clip "Fat" low-shelf
            fatSVF.setLowShelf(fatFreq, fatRes, 6.0f, sr);
            float preEQ = fatSVF.processLP(in);

            // 4x oversample + asymmetric clip
            float samples[4];
            float prev = aaState1;
            for (int k = 0; k < 4; ++k)
            {
                float t = static_cast<float>(k) / 4.0f;
                samples[k] = prev + t * (preEQ - prev);
            }
            aaState1 = preEQ;

            for (int k = 0; k < 4; ++k)
                samples[k] = diodeClip(samples[k], drive);

            float out = 0.0f;
            for (int k = 0; k < 4; ++k) out += samples[k];
            out *= 0.25f;

            // Post-clip "Muscle" wide-Q peak
            muscleSVF.setPeak(muscleFreq, 0.5f, muscleGain, sr);
            out = muscleSVF.processBell(out, muscleGain);

            // Output level
            return out * FastMath::fastPow2(outDb * (1.0f / 6.0205999f));
        }
    } diode_;

    //==========================================================================
    // Stage 3 — Dimension Chorus (Boss DC-2)
    // 2 parallel fractional delay lines + 180° LFO offset + hard-pan L/R
    // 4 exclusive mode buttons → 4 hardcoded Depth/Rate pairs
    //==========================================================================
    struct DimensionChorusStage
    {
        // Mode depth/rate presets matching DC-2 feel
        static constexpr float kModeDepth[4] = {2.0f, 4.0f, 6.5f, 10.0f}; // ms
        static constexpr float kModeRate[4]  = {0.5f, 0.8f, 1.2f, 1.8f};  // Hz

        std::vector<float> delay1, delay2;
        int write1 = 0, write2 = 0;
        StandardLFO lfo1, lfo2;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(15.0 * sampleRate / 1000.0) + 4;
            delay1.assign(maxSamples, 0.0f);
            delay2.assign(maxSamples, 0.0f);
            lfo1.prepare(sampleRate); lfo1.setWaveform(StandardLFO::Waveform::Sine);
            lfo2.prepare(sampleRate); lfo2.setWaveform(StandardLFO::Waveform::Sine);
            lfo2.setPhaseOffset(0.5f); // 180° offset (Phantom Sniff correction #3 — no native morph)
            reset();
        }
        void reset()
        {
            std::fill(delay1.begin(), delay1.end(), 0.0f);
            std::fill(delay2.begin(), delay2.end(), 0.0f);
            write1 = write2 = 0;
        }

        // Process stereo: hard-pan delay1→L, delay2→R
        void processBlock(const float* monoIn, float* L, float* R, int numSamples,
                          int activeMode /* 0-3 */)
        {
            activeMode = juce::jlimit(0, 3, activeMode);
            float depthMs  = kModeDepth[activeMode];
            float rateHz   = kModeRate[activeMode];
            lfo1.setRate(rateHz); lfo2.setRate(rateHz);

            float srF = static_cast<float>(sr);
            int bufSize = static_cast<int>(delay1.size());

            for (int i = 0; i < numSamples; ++i)
            {
                float mod1 = (lfo1.processSample() * 0.5f + 0.5f) * depthMs * srF / 1000.0f;
                float mod2 = (lfo2.processSample() * 0.5f + 0.5f) * depthMs * srF / 1000.0f;
                mod1 = std::min(mod1, static_cast<float>(bufSize) - 2.0f);
                mod2 = std::min(mod2, static_cast<float>(bufSize) - 2.0f);

                delay1[write1] = monoIn[i];
                delay2[write2] = monoIn[i];

                auto readLerp = [&](const std::vector<float>& buf, int wp, float d) -> float {
                    int sz = static_cast<int>(buf.size());
                    float rF = static_cast<float>(wp) - d;
                    while (rF < 0.0f) rF += static_cast<float>(sz);
                    int r0 = static_cast<int>(rF) % sz;
                    int r1 = (r0 + 1) % sz;
                    float fr = rF - static_cast<float>(r0);
                    return buf[r0] * (1.0f - fr) + buf[r1] * fr;
                };

                L[i] = readLerp(delay1, write1, mod1); // hard-pan L
                R[i] = readLerp(delay2, write2, mod2); // hard-pan R

                write1 = (write1 + 1) % bufSize;
                write2 = (write2 + 1) % bufSize;
            }
        }
    } dimChorus_;

    //==========================================================================
    // Stage 4 — Degrading BBD Delay (Boss DM-1)
    // LP 1.5kHz + fastTanh saturation in feedback path
    //==========================================================================
    struct BBDDegradeStage
    {
        static constexpr float kMaxDelayMs = 600.0f;
        std::vector<float> delayBuf;
        int writePos = 0;
        CytomicSVF feedbackLP; // Fixed at obsc_dlvGrit Hz
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 4;
            delayBuf.assign(maxSamples, 0.0f);
            reset();
        }
        void reset()
        {
            std::fill(delayBuf.begin(), delayBuf.end(), 0.0f);
            writePos = 0;
            feedbackLP.reset();
        }

        float processBlock_Sample(float in, float delayMs, float fbAmount,
                                   float compound, float gritHz)
        {
            int bufSize = static_cast<int>(delayBuf.size());
            float delaySamples = std::min(delayMs * static_cast<float>(sr) / 1000.0f,
                                          static_cast<float>(bufSize) - 2.0f);
            delaySamples = std::max(1.0f, delaySamples);

            // Read delayed (linear interpolation)
            float rF = static_cast<float>(writePos) - delaySamples;
            while (rF < 0.0f) rF += static_cast<float>(bufSize);
            int r0 = static_cast<int>(rF) % bufSize;
            int r1 = (r0 + 1) % bufSize;
            float fr = rF - static_cast<float>(r0);
            float wet = delayBuf[r0] * (1.0f - fr) + delayBuf[r1] * fr;

            // Feedback path: LP filter + tanh saturation (progressive degradation)
            feedbackLP.setLP(gritHz, 0.707f, sr);
            float fbProcessed = feedbackLP.processLP(wet);
            fbProcessed = FastMath::fastTanh(fbProcessed * (1.0f + compound));

            delayBuf[writePos] = flushDenormal(in + fbProcessed * fbAmount);
            writePos = (writePos + 1) % bufSize;

            return wet;
        }
    } bbd_;

    //==========================================================================
    // Stage 5 — Industrial Multi-Tap Reverb (Death By Audio Rooms)
    // 8 prime-spaced taps + cross-feedback + hard clipper at input
    //==========================================================================
    struct IndustrialReverbStage
    {
        static constexpr int kNumTaps = 8;
        static constexpr int kPrimeLengths[kNumTaps] = {
            557, 743, 1021, 1327, 1847, 2503, 3251, 4127
        };

        std::array<std::vector<float>, kNumTaps> tapBufs;
        std::array<int, kNumTaps> writePos{};
        std::array<float, kNumTaps> tapState{};
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            float srScale = static_cast<float>(sampleRate) / 48000.0f;
            for (int t = 0; t < kNumTaps; ++t)
            {
                int len = static_cast<int>(kPrimeLengths[t] * srScale) + 2;
                tapBufs[t].assign(len, 0.0f);
                writePos[t] = 0;
                tapState[t] = 0.0f;
            }
        }
        void reset()
        {
            for (int t = 0; t < kNumTaps; ++t)
            {
                std::fill(tapBufs[t].begin(), tapBufs[t].end(), 0.0f);
                writePos[t] = 0;
                tapState[t] = 0.0f;
            }
        }

        // Process mono → stereo: even taps → L, odd taps → R
        void processBlock(const float* monoIn, float* L, float* R, int numSamples,
                          float decay, float mix)
        {
            if (mix < 0.001f)
            {
                for (int i = 0; i < numSamples; ++i) { L[i] = monoIn[i]; R[i] = monoIn[i]; }
                return;
            }

            float decayCoeff = FastMath::fastExp(-1.0f / (decay * static_cast<float>(sr)));
            float srScale = static_cast<float>(sr) / 48000.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                // Hard clipper at input (ADC overload simulation)
                float clippedIn = juce::jlimit(-0.95f, 0.95f, monoIn[i]);

                // Read from all taps
                float tapOut[kNumTaps];
                float crossFeedback = 0.0f;
                for (int t = 0; t < kNumTaps; ++t)
                {
                    int len = static_cast<int>(tapBufs[t].size());
                    int effectiveDelay = static_cast<int>(kPrimeLengths[t] * srScale);
                    effectiveDelay = juce::jlimit(1, len - 1, effectiveDelay);
                    int rp = ((writePos[t] - effectiveDelay) + len) % len;
                    tapState[t] = flushDenormal(
                        tapState[t] * decayCoeff + tapBufs[t][rp] * (1.0f - decayCoeff));
                    tapOut[t]    = tapState[t];
                    crossFeedback += tapOut[t];
                }
                crossFeedback /= static_cast<float>(kNumTaps);

                // Write into taps with cross-feedback (no allpass — discrete echoes)
                for (int t = 0; t < kNumTaps; ++t)
                {
                    int len = static_cast<int>(tapBufs[t].size());
                    tapBufs[t][writePos[t]] = flushDenormal(
                        clippedIn + crossFeedback * 0.1f * decayCoeff);
                    writePos[t] = (writePos[t] + 1) % len;
                }

                // Stereo output: even taps → L, odd taps → R
                float wetL = 0.0f, wetR = 0.0f;
                for (int t = 0; t < kNumTaps; ++t)
                {
                    if (t % 2 == 0) wetL += tapOut[t];
                    else            wetR += tapOut[t];
                }
                wetL /= static_cast<float>(kNumTaps / 2);
                wetR /= static_cast<float>(kNumTaps / 2);

                L[i] = monoIn[i] + mix * (wetL - monoIn[i]);
                R[i] = monoIn[i] + mix * (wetR - monoIn[i]);
            }
        }
    } indRev_;

    //==========================================================================
    // Cached parameter pointers (20 params)
    //==========================================================================
    std::atomic<float>* p_pllGlide       = nullptr;
    std::atomic<float>* p_pllSub1        = nullptr;
    std::atomic<float>* p_pllSub2        = nullptr;
    std::atomic<float>* p_pllSquare      = nullptr;
    std::atomic<float>* p_odDrive        = nullptr;
    std::atomic<float>* p_odFatFreq      = nullptr;
    std::atomic<float>* p_odFatRes       = nullptr;
    std::atomic<float>* p_odMuscleFreq   = nullptr;
    std::atomic<float>* p_odMuscleGain   = nullptr;
    std::atomic<float>* p_odLevel        = nullptr;
    std::atomic<float>* p_dimMode1       = nullptr;
    std::atomic<float>* p_dimMode2       = nullptr;
    std::atomic<float>* p_dimMode3       = nullptr;
    std::atomic<float>* p_dimMode4       = nullptr;
    std::atomic<float>* p_dlvTime        = nullptr;
    std::atomic<float>* p_dlvFeedback    = nullptr;
    std::atomic<float>* p_dlvCompound    = nullptr;
    std::atomic<float>* p_dlvGrit        = nullptr;
    std::atomic<float>* p_revSize        = nullptr;
    std::atomic<float>* p_revMix         = nullptr;
};

// Static constexpr member definitions
constexpr float ObscurityChain::DimensionChorusStage::kModeDepth[4];
constexpr float ObscurityChain::DimensionChorusStage::kModeRate[4];
constexpr int   ObscurityChain::IndustrialReverbStage::kPrimeLengths[kNumTaps];

//==============================================================================
// Inline implementations
//==============================================================================

inline void ObscurityChain::prepare(double sampleRate, int /*maxBlockSize*/)
{
    sr_ = sampleRate;
    pll_.prepare(sampleRate);
    diode_.prepare(sampleRate);
    dimChorus_.prepare(sampleRate);
    bbd_.prepare(sampleRate);
    indRev_.prepare(sampleRate);
}

inline void ObscurityChain::reset()
{
    pll_.reset();
    diode_.reset();
    dimChorus_.reset();
    bbd_.reset();
    indRev_.reset();
}

inline void ObscurityChain::processBlock(const float* monoIn, float* L, float* R, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;
    if (!p_pllGlide) return;

    const float pllGlide      = p_pllGlide->load(std::memory_order_relaxed);
    const float pllSub1       = p_pllSub1->load(std::memory_order_relaxed);
    const float pllSub2       = p_pllSub2->load(std::memory_order_relaxed);
    const float pllSquare     = p_pllSquare->load(std::memory_order_relaxed);
    const float odDrive       = p_odDrive->load(std::memory_order_relaxed);
    const float odFatFreq     = p_odFatFreq->load(std::memory_order_relaxed);
    const float odFatRes      = p_odFatRes->load(std::memory_order_relaxed);
    const float odMuscleFreq  = p_odMuscleFreq->load(std::memory_order_relaxed);
    const float odMuscleGain  = p_odMuscleGain->load(std::memory_order_relaxed);
    const float odLevel       = p_odLevel->load(std::memory_order_relaxed);
    const float dimMode1      = p_dimMode1->load(std::memory_order_relaxed);
    const float dimMode2      = p_dimMode2->load(std::memory_order_relaxed);
    const float dimMode3      = p_dimMode3->load(std::memory_order_relaxed);
    const float dlvTime       = p_dlvTime->load(std::memory_order_relaxed);
    const float dlvFeedback   = p_dlvFeedback->load(std::memory_order_relaxed);
    const float dlvCompound   = p_dlvCompound->load(std::memory_order_relaxed);
    const float dlvGrit       = p_dlvGrit->load(std::memory_order_relaxed);
    const float revSize       = p_revSize->load(std::memory_order_relaxed);
    const float revMix        = p_revMix->load(std::memory_order_relaxed);

    // Resolve active DC-2 dimension mode (exclusive bool → int 0-3)
    int activeMode = 0;
    if (dimMode2 > 0.5f) activeMode = 1;
    if (dimMode3 > 0.5f) activeMode = 2;
    if (p_dimMode4->load(std::memory_order_relaxed) > 0.5f) activeMode = 3;

    float odDriveLin = FastMath::fastPow2(odDrive * (1.0f / 6.0205999f));

    // Stage 1: PLL + Stage 2: Diode OD (mono pipeline)
    for (int i = 0; i < numSamples; ++i)
    {
        float x = pll_.process(monoIn[i], pllGlide, pllSub1, pllSub2, pllSquare);
        x = diode_.process(x, odDriveLin, odFatFreq, odFatRes, odMuscleFreq, odMuscleGain, odLevel);
        L[i] = x; // temp mono
    }

    // Stage 3: Dimension Chorus — Mono → Stereo
    dimChorus_.processBlock(L, L, R, numSamples, activeMode);

    // Stage 4: BBD Delay (both channels)
    for (int i = 0; i < numSamples; ++i)
    {
        float wetL = bbd_.processBlock_Sample(L[i], dlvTime, dlvFeedback * 0.01f,
                                               dlvCompound, dlvGrit);
        float wetR = bbd_.processBlock_Sample(R[i], dlvTime, dlvFeedback * 0.01f,
                                               dlvCompound, dlvGrit);
        L[i] = wetL; R[i] = wetR;
    }

    // Stage 5: Industrial Reverb
    // Use temp mono sum for reverb input
    for (int i = 0; i < numSamples; ++i)
    {
        float mono = (L[i] + R[i]) * 0.5f;
        float outL, outR;
        // Write to single-sample temp arrays
        float mArr[1] = {mono};
        float lArr[1], rArr[1];
        indRev_.processBlock(mArr, lArr, rArr, 1, revSize, revMix * 0.01f);
        L[i] = lArr[0]; R[i] = rArr[0];
    }
}

inline void ObscurityChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    using AP  = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    using NR  = juce::NormalisableRange<float>;
    const juce::String p = slotPrefix + "obsc_";

    layout.add(std::make_unique<AP>(juce::ParameterID{p + "pllGlide",     1}, p + "PLL Glide",
        NR(0.0f, 200.0f, 1.0f), 50.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "pllSub1",      1}, p + "PLL Sub1",
        NR(-60.0f, 0.0f, 0.1f), -12.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "pllSub2",      1}, p + "PLL Sub2",
        NR(-60.0f, 0.0f, 0.1f), -18.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "pllSquare",    1}, p + "PLL Square",
        NR(-60.0f, 0.0f, 0.1f), -6.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "odDrive",      1}, p + "OD Drive",
        NR(0.0f, 40.0f, 0.1f), 12.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "odFatFreq",    1}, p + "OD Fat Freq",
        NR(50.0f, 2000.0f, 1.0f), 200.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "odFatRes",     1}, p + "OD Fat Res",
        NR(0.5f, 5.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "odMuscleFreq", 1}, p + "OD Muscle Freq",
        NR(500.0f, 8000.0f, 1.0f), 2000.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "odMuscleGain", 1}, p + "OD Muscle Gain",
        NR(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "odLevel",      1}, p + "OD Level",
        NR(-18.0f, 12.0f, 0.1f), 0.0f));
    // Exclusive mode buttons (4 bools — implementation uses float 0/1)
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dimMode1",     1}, p + "Dim Mode1",
        NR(0.0f, 1.0f, 1.0f), 1.0f)); // default mode 1 active
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dimMode2",     1}, p + "Dim Mode2",
        NR(0.0f, 1.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dimMode3",     1}, p + "Dim Mode3",
        NR(0.0f, 1.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dimMode4",     1}, p + "Dim Mode4",
        NR(0.0f, 1.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dlvTime",      1}, p + "DLV Time",
        NR(10.0f, 500.0f, 1.0f), 200.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dlvFeedback",  1}, p + "DLV Feedback",
        NR(0.0f, 85.0f, 0.1f), 40.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dlvCompound",  1}, p + "DLV Compound",
        NR(0.0f, 4.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dlvGrit",      1}, p + "DLV Grit",
        NR(200.0f, 8000.0f, 1.0f), 1500.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revSize",      1}, p + "Rev Size",
        NR(0.0f, 2.0f, 0.001f), 0.5f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revMix",       1}, p + "Rev Mix",
        NR(0.0f, 100.0f, 0.1f), 0.0f));
}

inline void ObscurityChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "obsc_";
    p_pllGlide       = apvts.getRawParameterValue(p + "pllGlide");
    p_pllSub1        = apvts.getRawParameterValue(p + "pllSub1");
    p_pllSub2        = apvts.getRawParameterValue(p + "pllSub2");
    p_pllSquare      = apvts.getRawParameterValue(p + "pllSquare");
    p_odDrive        = apvts.getRawParameterValue(p + "odDrive");
    p_odFatFreq      = apvts.getRawParameterValue(p + "odFatFreq");
    p_odFatRes       = apvts.getRawParameterValue(p + "odFatRes");
    p_odMuscleFreq   = apvts.getRawParameterValue(p + "odMuscleFreq");
    p_odMuscleGain   = apvts.getRawParameterValue(p + "odMuscleGain");
    p_odLevel        = apvts.getRawParameterValue(p + "odLevel");
    p_dimMode1       = apvts.getRawParameterValue(p + "dimMode1");
    p_dimMode2       = apvts.getRawParameterValue(p + "dimMode2");
    p_dimMode3       = apvts.getRawParameterValue(p + "dimMode3");
    p_dimMode4       = apvts.getRawParameterValue(p + "dimMode4");
    p_dlvTime        = apvts.getRawParameterValue(p + "dlvTime");
    p_dlvFeedback    = apvts.getRawParameterValue(p + "dlvFeedback");
    p_dlvCompound    = apvts.getRawParameterValue(p + "dlvCompound");
    p_dlvGrit        = apvts.getRawParameterValue(p + "dlvGrit");
    p_revSize        = apvts.getRawParameterValue(p + "revSize");
    p_revMix         = apvts.getRawParameterValue(p + "revMix");
}

} // namespace xoceanus
```

- [ ] **Step 9.2** — Create `Source/DSP/Effects/ObscurityChain.cpp`:
```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#include "ObscurityChain.h"
```

- [ ] **Step 9.3** — Build to verify. Fix errors.

---

## Phase 3: Integration & Finalization

### Task 10: Wire Epic Chains into slot controller

**Files to modify:** `Source/Core/EpicChainSlotController.h`

- [ ] **Step 10.1** — Add includes at the top of `EpicChainSlotController.h`:
```cpp
#include "../DSP/Effects/OnrushChain.h"
#include "../DSP/Effects/OmnistereoChain.h"
#include "../DSP/Effects/ObliterateChain.h"
#include "../DSP/Effects/ObscurityChain.h"
```

- [ ] **Step 10.2** — Add chain instances to `FXSlot` struct (uncomment the epic chain placeholders):
```cpp
        OnrushChain     onrush;
        OmnistereoChain omnistereo;
        ObliterateChain obliterate;
        ObscurityChain  obscurity;
```

- [ ] **Step 10.3** — In `prepare()`, add chain prepare calls inside the slot loop:
```cpp
        slot.onrush.prepare(sampleRate, maxBlockSize);
        slot.omnistereo.prepare(sampleRate, maxBlockSize);
        slot.obliterate.prepare(sampleRate, maxBlockSize);
        slot.obscurity.prepare(sampleRate, maxBlockSize);
```

- [ ] **Step 10.4** — In `reset()`, add chain reset calls:
```cpp
        slot.onrush.reset();
        slot.omnistereo.reset();
        slot.obliterate.reset();
        slot.obscurity.reset();
```

- [ ] **Step 10.5** — In `dispatchChain()`, fill in the epic chain cases:
```cpp
        case Onrush:
            // OnrushChain takes monoIn + stereo L/R out
            // Since buffer is already stereo, mix L+R to mono, then write stereo out
            {
                // Mono sum of input
                int safeSamples = numSamples; // already bounded by caller
                // Use temporary mono buffer — allocate on stack for small blocks
                // or use a pre-allocated scratch member for safety
                // (add a `std::vector<float> monoScratch_` member to FXSlot)
                // Then: for (int i = 0; i < numSamples; ++i)
                //           monoScratch_[i] = (L[i] + R[i]) * 0.5f;
                // slot.onrush.processBlock(monoScratch_.data(), L, R, numSamples, bpm, ppqPosition);
            }
            break;

        case Omnistereo:
            slot.omnistereo.processBlock(L, R, numSamples);
            break;

        case Obliterate:
            {
                // Same mono→stereo pattern as Onrush
                // slot.obliterate.processBlock(monoScratch_.data(), L, R, numSamples);
            }
            break;

        case Obscurity:
            {
                // Same mono→stereo pattern
                // slot.obscurity.processBlock(monoScratch_.data(), L, R, numSamples);
            }
            break;
```

> **Note on mono scratch buffer:** For Onrush, Obliterate, and Obscurity (Mono In → Stereo Out), add a `std::vector<float> monoScratch_` member to `FXSlot`. Pre-allocate in `prepare()` with `monoScratch.assign(maxBlockSize, 0.0f)`. Fill before dispatching to mono-in chains.

- [ ] **Step 10.6** — In `addParameters()`, add epic chain `addParameters()` calls for all 3 slots:
```cpp
        OnrushChain::addParameters(layout, slotPrefix);
        OmnistereoChain::addParameters(layout, slotPrefix);
        ObliterateChain::addParameters(layout, slotPrefix);
        ObscurityChain::addParameters(layout, slotPrefix);
```

- [ ] **Step 10.7** — In `cacheParameterPointers()`, add epic chain cache calls per slot:
```cpp
        slots_[n].onrush.cacheParameterPointers(apvts, prefix);
        slots_[n].omnistereo.cacheParameterPointers(apvts, prefix);
        slots_[n].obliterate.cacheParameterPointers(apvts, prefix);
        slots_[n].obscurity.cacheParameterPointers(apvts, prefix);
```

- [ ] **Step 10.8** — Build to verify:
```bash
cmake --build build 2>&1 | tail -30
```

---

### Task 11: CMakeLists.txt + full build validation

**Files to modify:** `CMakeLists.txt`

- [ ] **Step 11.1** — Read `CMakeLists.txt` to confirm the current source list. Find where the existing `.cpp` stubs are listed (around lines 176–494).

- [ ] **Step 11.2** — Add new header files to the source list. Find the `# Effects (shared, pure C++)` section (around line 159) and add after `BoutiqueFXChain.h`:
```cmake
    Source/DSP/Effects/OnrushChain.h
    Source/DSP/Effects/OmnistereoChain.h
    Source/DSP/Effects/ObliterateChain.h
    Source/DSP/Effects/ObscurityChain.h
```

And add `EpicChainSlotController.h` to the `Source/Core/` section (after `MPEManager.h`):
```cmake
    Source/Core/EpicChainSlotController.h
```

- [ ] **Step 11.3** — Add the new `.cpp` stubs. Find the Shapers block (around line 492) and add the new stubs before `Source/XOceanusProcessor.cpp`:
```cmake
    Source/Core/EpicChainSlotController.cpp
    Source/DSP/Effects/OnrushChain.cpp
    Source/DSP/Effects/OmnistereoChain.cpp
    Source/DSP/Effects/ObliterateChain.cpp
    Source/DSP/Effects/ObscurityChain.cpp
```

- [ ] **Step 11.4** — Full build:
```bash
eval "$(fnm env)" && fnm use 20
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build 2>&1 | tail -40
```

- [ ] **Step 11.5** — If build succeeds, run auval validation:
```bash
auval -v aumu Xocn XoOx
```
Expected output: `PASS`

- [ ] **Step 11.6** — If auval fails, investigate with verbose mode:
```bash
auval -v aumu Xocn XoOx 2>&1 | grep -E "FAIL|ERROR|parameter"
```
Common failure causes:
- Parameter ID collision (duplicate ID in the APVTS layout)
- Parameter count mismatch between versions
- Missing/extra parameters in the layout

- [ ] **Step 11.7** — Fix any issues and rebuild. Common build errors at this stage and their fixes:
  - **`use of undeclared identifier 'flushDenormal'`** → Add `using namespace xoceanus;` or ensure `FastMath.h` is included (it defines `flushDenormal`)
  - **`ambiguous overload for 'processBlock'`** → Check that all chain processBlock calls match the declared signatures
  - **Linker error for `kPrimeTaps`** → Ensure `constexpr` static member is defined outside the class in the header

- [ ] **Step 11.8** — Commit all new files:
```bash
git add Source/Core/EpicChainSlotController.h Source/Core/EpicChainSlotController.cpp
git add Source/DSP/Effects/OnrushChain.h Source/DSP/Effects/OnrushChain.cpp
git add Source/DSP/Effects/OmnistereoChain.h Source/DSP/Effects/OmnistereoChain.cpp
git add Source/DSP/Effects/ObliterateChain.h Source/DSP/Effects/ObliterateChain.cpp
git add Source/DSP/Effects/ObscurityChain.h Source/DSP/Effects/ObscurityChain.cpp
git add Source/Core/MasterFXChain.h Source/Core/PresetManager.h CMakeLists.txt
git commit -m "feat: Epic Chains — 4 new FX chain engines + unified 3-slot FX system

Adds ONRUSH, OMNISTEREO, OBLITERATE, OBSCURITY FX chains from the
Gemini Pedalboard Series concept. Replaces fixed MasterFXChain
AquaticFXSuite/MathFXChain/BoutiqueFXChain/Singularity FX positions
with EpicChainSlotController: 3 fully assignable slots, 50ms crossfade,
all 10 chains available in any slot. Preset fxVersion 1→2 migration
with parameter aliases for DAW automation backward compatibility."
```

---

### Task 12: CLAUDE.md documentation updates

**Files to modify:** `CLAUDE.md` (project root)

- [ ] **Step 12.1** — In the **Product Identity** header, update the engine count and add the 4 new engines to the registered list. Find the `**Engine modules (registered):**` line and append:
```
ONRUSH, OMNISTEREO, OBLITERATE, OBSCURITY
```
Update the count in the header from 76 to 80.

- [ ] **Step 12.2** — In the **Engine Modules table**, add 4 rows after OUTFLOW:
```markdown
| ONRUSH | PolySwell (Gemini Pedalboard Series) | Molten Amber `#FF6F00` |
| OMNISTEREO | ChromaSpace (Gemini Pedalboard Series) | Prismatic Silver `#B0C4DE` |
| OBLITERATE | GritGlitch (Gemini Pedalboard Series) | Apocalypse Red `#8B0000` |
| OBSCURITY | VoidAtmosphere (Gemini Pedalboard Series) | Void Purple `#2D1B69` |
```

- [ ] **Step 12.3** — In the **Parameter Prefix table**, add 4 rows:
```markdown
| Onrush | `onr_` | `onr_swellThresh` |
| Omnistereo | `omni_` | `omni_tapeDrive` |
| Obliterate | `oblt_` | `oblt_shimInterval` |
| Obscurity | `obsc_` | `obsc_pllGlide` |
```

- [ ] **Step 12.4** — In the **Key Files table**, add 5 rows:
```markdown
| `Source/Core/EpicChainSlotController.h` | 3-slot assignable FX controller (all 10 chains, 50ms crossfade, slot prefix routing) |
| `Source/DSP/Effects/OnrushChain.h` | ONRUSH 5-stage expressive lead FX chain (Auto-Swell → Ring Mod → Dist → Env Filter → BBD Delay) |
| `Source/DSP/Effects/OmnistereoChain.h` | OMNISTEREO 5-stage stereo widener FX chain (Tape Sat → EQ → Vibrato → Chorus → FDN Reverb) |
| `Source/DSP/Effects/ObliterateChain.h` | OBLITERATE 5-stage heavy stutter FX chain (Shimmer Rev → Octave Fuzz → Reverse Delay → Granular → Tremolo) |
| `Source/DSP/Effects/ObscurityChain.h` | OBSCURITY 5-stage dark ambient FX chain (PLL Synth → Asym Diode → Dimension Chorus → BBD Delay → Industrial Reverb) |
```

- [ ] **Step 12.5** — Commit the CLAUDE.md update:
```bash
git add CLAUDE.md
git commit -m "docs: register ONRUSH/OMNISTEREO/OBLITERATE/OBSCURITY in CLAUDE.md

Add 4 Epic Chain FX engines to Product Identity, Engine Modules table,
Parameter Prefix table, and Key Files table. Engine count: 76 → 80."
```

---

## Implementation Notes & Pitfalls

### DSP Correctness Checklist (verify before each build)

1. **No hardcoded sample rates:** All time/frequency math uses `sr_` or the `sampleRate` parameter. Never `44100.0` or `48000.0` as a magic constant (except as a scaling reference factor like `srScale = sr / 48000.0f`).

2. **flushDenormal on all feedback state:** Every `state = state * coeff + input * (1-coeff)` line that feeds back must call `flushDenormal()`. Missing this causes denormal CPU stalls.

3. **ParamSnapshot is mandatory:** All `p_param->load(std::memory_order_relaxed)` calls must be at the top of `processBlock()` before the sample loop. Never load inside the sample loop.

4. **ScopedNoDenormals at top of processBlock:** `juce::ScopedNoDenormals noDenormals;` must be the first line of every `processBlock()`.

5. **Buffer bounds safety:** All delay line reads/writes must clamp or wrap their indices. Use `juce::jlimit(0, bufSize-1, index)` for read positions.

6. **Phantom Sniff corrections applied:**
   - ONRUSH Stage 2: Two PolyBLEP instances with manual crossfade — NOT a single instance with morph mode
   - OMNISTEREO Stage 1: StandardLFO S&H + one-pole lowpass — NOT Perlin noise mode
   - All LFO rate changes: Pass through ParameterSmoother — NOT calling `setRate()` directly per sample
   - No `StandardLFO::morphBetween()` or shape morphing API exists — use manual crossfade where needed

7. **Mono→Stereo chains need mono scratch:** ONRUSH, OBLITERATE, OBSCURITY take mono input. The `FXSlot::monoScratch_` vector must be pre-allocated in `prepare()` and filled before dispatching.

8. **addParameters() version ID:** Use `juce::ParameterID{paramId, 1}` with version 1. This ensures JUCE APVTS generates stable automation IDs.

9. **CytomicSVF method names:** Confirm actual method names in `Source/DSP/CytomicSVF.h` before using `setLP()`, `setLowShelf()`, `processBell()`, etc. The exact names may differ from the examples in this plan — adapt to match the actual API.

10. **PolyBLEP waveform enum:** Confirm `PolyBLEP::Waveform::Sine` and `PolyBLEP::Waveform::Square` exist in `Source/DSP/PolyBLEP.h`. Adapt enum names if they differ.

11. **StandardLFO waveform enum:** Confirm `StandardLFO::Waveform::SampleAndHold` and `StandardLFO::Waveform::Sine` etc. in `Source/DSP/StandardLFO.h`.

12. **LushReverb API:** Confirm `setSize()`, `setDamping()`, `setModulation()`, `setMix()`, `processBlock(inL, inR, outL, outR, n)` method signatures in `Source/DSP/Effects/LushReverb.h`.

### Common Compilation Errors and Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| `no member named 'setLP' in 'CytomicSVF'` | Wrong method name | Read CytomicSVF.h and use actual LP setter name |
| `use of undeclared identifier 'flushDenormal'` | Missing include | CytomicSVF.h or FastMath.h defines it — ensure included |
| `ambiguous call to processBlock` | Overloaded signature conflict | Add explicit cast or rename one overload |
| `error: expected ';' after struct` | Missing semicolon after `} memberName_` | Add `;` |
| `duplicate parameter ID` | Two params registered with same ID string | Add slotPrefix correctly — check all add calls |
| `auval FAIL: parameter count mismatch` | Old preset state has different parameter count than current | Clear plugin state cache and re-run auval |

### Parameter Budget Per Slot (for documentation)

| Chain | Params | × 3 Slots |
|-------|--------|-----------|
| Aquatic | 52 | 156 |
| Math | 16 | 48 |
| Boutique | 25 | 75 |
| Onslaught | 8 | 24 |
| Obscura | 11 | 33 |
| Oratory | 9 | 27 |
| Onrush | 17 | 51 |
| Omnistereo | 16 | 48 |
| Obliterate | 19 | 57 |
| Obscurity | 20 | 60 |
| **Per-slot subtotal** | **193** | |
| **Total chain params** | | **579** |
| Slot control (chain + mix + bypass × 3) | 9 | |
| **Grand total** | | **~588** |

---

## Completion Criteria

- [ ] All 5 new files compile cleanly (`cmake --build build` with no errors)
- [ ] `auval -v aumu Xocn XoOx` returns `PASS`
- [ ] All 4 Epic Chain engines have `prepare()`, `reset()`, `processBlock()`, `addParameters()`, `cacheParameterPointers()`
- [ ] EpicChainSlotController processes 3 slots in series with correct crossfade
- [ ] Old aqua_*, mfx_*, bfx_*, master_onsl*, master_obs*, master_ora* params are aliased
- [ ] Preset loading with fxVersion absent triggers migration without crash
- [ ] CLAUDE.md updated with all 4 engines (engine list, table, prefix table, key files)
- [ ] All new headers follow `namespace xoceanus { ... }` convention
- [ ] No hardcoded sample rates (44100/48000 as magic constants)
- [ ] All feedback state variables have `flushDenormal()`
- [ ] All processBlock() start with `juce::ScopedNoDenormals`
