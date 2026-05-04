// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
// Wave 5 C5: SlotModSourceRegistry — per-slot message-thread-origin ModSource values.
//
// Closes: issue #1360
// Design ref: ~/.claude/projects/-Users-joshuacramblet/memory/wave5-c1-sequencer-design-2026-04-26.md
//
// Ownership model
// ───────────────
// XOceanusProcessor holds a SlotModSourceRegistry by value.  It is constructed
// before the audio thread starts and lives for the processor lifetime.
//
// Thread model
// ────────────
// Message thread: updateSourceValue() — called from UI-thread callbacks when a
//   live value changes.
// Audio thread: read accessors — called from processBlock() to read current
//   values into the mod routing accumulation loop.
//
// All members use std::atomic<float> with relaxed ordering — a one-block-late
// value is acceptable for a continuous modulation source.
//
// No allocations after construction.  No virtual methods.  Safe on the RT thread.
//
// Frozen ModSource IDs (must not change — preset serialisation)
// ─────────────────────────────────────────────────────────────
// 18 retired (was XouijaCell, removed 2026-05-01)
//
// The SeqStepValue (15), LiveGate (16), BeatPhase (17), and SeqStepPitch (19)
// sources are read directly from PerEnginePatternSequencer::getLive*() — they
// do not go through this registry.  This registry handles sources whose values
// originate on the message thread (UI gestures) rather than the audio thread.
//
#pragma once
#include <atomic>
#include "Future/UI/ModRouting/ModSourceHandle.h" // ModSourceId — xoceanus::ModSourceId enum

namespace xoceanus
{

//==============================================================================
/**
    SlotModSourceRegistry

    Stores live bipolar values for ModSources whose origin is on the message
    thread (UI gestures) so the audio thread can read them lock-free.

    XouijaCell (ID 18) was removed 2026-05-01.  The registry is preserved as
    infrastructure for future message-thread-origin sources.

    Designed for extension: add a new atomic pair + updateSourceValue overload for
    each future message-thread-origin source.
*/
class SlotModSourceRegistry
{
public:
    //==========================================================================
    // Construction — no current live values to initialise.
    SlotModSourceRegistry() noexcept = default;

    // Non-copyable, non-movable — owned by value in XOceanusProcessor.
    SlotModSourceRegistry(const SlotModSourceRegistry&)            = delete;
    SlotModSourceRegistry& operator=(const SlotModSourceRegistry&) = delete;
    SlotModSourceRegistry(SlotModSourceRegistry&&)                 = delete;
    SlotModSourceRegistry& operator=(SlotModSourceRegistry&&)      = delete;

    //==========================================================================
    // updateSourceValue — message thread only.
    //
    // Called from UI-thread callbacks to push new live values into the registry.
    // Values are bipolar [-1, +1].
    //
    // No sources are currently handled here; add else-if branches as each lands.
    //
    void updateSourceValue(ModSourceId /*id*/, float /*bx*/, float /*by*/) noexcept
    {
        // Additional sources: add else-if branches as each lands.
    }

    //==========================================================================
    // reset — called from XOceanusProcessor::reset() / prepareToPlay().
    // Clears all live values back to neutral.  Audio-thread safe.
    void reset() noexcept
    {
        // No current live values to reset.
    }
};

} // namespace xoceanus
