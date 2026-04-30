// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
// Wave 5 C5: SlotModSourceRegistry — per-slot sequencer step values as ModSources.
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
// Message thread: updateSourceValue() — called from XouijaPinStore::onPinChanged
//   and similar UI-thread callbacks when a pinned/live value changes.
// Audio thread: getXouijaCellX() / getXouijaCellY() — called from processBlock()
//   to read the current bipolar value into the mod routing accumulation loop.
//
// All members use std::atomic<float> with relaxed ordering — a one-block-late
// value is acceptable for a continuous modulation source.
//
// No allocations after construction.  No virtual methods.  Safe on the RT thread.
//
// Frozen ModSource IDs (must not change — preset serialisation)
// ─────────────────────────────────────────────────────────────
// ModSourceId::XouijaCell = 18   (bipolar X+Y, 4 capture slots, #1360)
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

    Currently hosts:
      - ModSourceId::XouijaCell — pinned XOuija (X, Y) position, bipolar [-1, +1].
        X represents the circle-of-fifths position; Y the influence depth.
        Written by XouijaPinStore::onPinChanged; read from processBlock.

    Designed for extension: add a new atomic pair + updateSourceValue overload for
    each future message-thread-origin source.
*/
class SlotModSourceRegistry
{
public:
    //==========================================================================
    // Construction — initialise all live values to 0.0f (neutral / no modulation).
    SlotModSourceRegistry() noexcept
    {
        ouijaCellX_.store(0.0f, std::memory_order_relaxed);
        ouijaCellY_.store(0.0f, std::memory_order_relaxed);
    }

    // Non-copyable, non-movable — owned by value in XOceanusProcessor.
    SlotModSourceRegistry(const SlotModSourceRegistry&)            = delete;
    SlotModSourceRegistry& operator=(const SlotModSourceRegistry&) = delete;
    SlotModSourceRegistry(SlotModSourceRegistry&&)                 = delete;
    SlotModSourceRegistry& operator=(SlotModSourceRegistry&&)      = delete;

    //==========================================================================
    // updateSourceValue — message thread only.
    //
    // Called from UI-thread callbacks (e.g. XouijaPinStore::onPinChanged) to
    // push new live values into the registry.  Values are bipolar [-1, +1].
    //
    // Only ModSourceId::XouijaCell is handled here; all other sources either
    // live on the audio thread (sequencers) or are not yet implemented.
    //
    void updateSourceValue(ModSourceId id, float bx, float by) noexcept
    {
        if (id == ModSourceId::XouijaCell)
        {
            ouijaCellX_.store(bx, std::memory_order_relaxed);
            ouijaCellY_.store(by, std::memory_order_relaxed);
        }
        // Additional sources: add else-if branches as each lands.
    }

    //==========================================================================
    // Audio-thread read accessors — called from XOceanusProcessor::processBlock.

    /// Pinned XOuija X-axis (circle-of-fifths), bipolar [-1, +1].
    float getXouijaCellX() const noexcept
    {
        return ouijaCellX_.load(std::memory_order_relaxed);
    }

    /// Pinned XOuija Y-axis (influence depth), bipolar [-1, +1].
    float getXouijaCellY() const noexcept
    {
        return ouijaCellY_.load(std::memory_order_relaxed);
    }

    //==========================================================================
    // reset — called from XOceanusProcessor::reset() / prepareToPlay().
    // Clears all live values back to neutral (0.0f).  Audio-thread safe.
    void reset() noexcept
    {
        ouijaCellX_.store(0.0f, std::memory_order_relaxed);
        ouijaCellY_.store(0.0f, std::memory_order_relaxed);
    }

private:
    // XouijaCell (ModSourceId::XouijaCell = 18) — bipolar [-1, +1].
    // Initialised to 0.0f so unconnected XouijaCell routes produce zero offset.
    std::atomic<float> ouijaCellX_{0.0f};
    std::atomic<float> ouijaCellY_{0.0f};
};

} // namespace xoceanus
