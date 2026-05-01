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
        ouijaLiveX_.store(0.0f, std::memory_order_relaxed);
        ouijaLiveY_.store(0.0f, std::memory_order_relaxed);
    }

    // Non-copyable, non-movable — owned by value in XOceanusProcessor.
    SlotModSourceRegistry(const SlotModSourceRegistry&) = delete;
    SlotModSourceRegistry& operator=(const SlotModSourceRegistry&) = delete;
    SlotModSourceRegistry(SlotModSourceRegistry&&) = delete;
    SlotModSourceRegistry& operator=(SlotModSourceRegistry&&) = delete;

    //==========================================================================
    // updateSourceValue (two-float overload) — message thread only.
    // Legacy overload for XouijaCell (ID=18): packs X+Y into one call.
    // Kept for back-compat; PlaySurface also calls the single-float overload
    // so new routes on XouijaX/Y IDs work independently.
    //
    void updateSourceValue(ModSourceId id, float bx, float by) noexcept
    {
        if (id == ModSourceId::XouijaCell)
        {
            ouijaCellX_.store(bx, std::memory_order_relaxed);
            ouijaCellY_.store(by, std::memory_order_relaxed);
        }
    }

    // updateSourceValue (single-float overload) — message thread only.
    // Handles XouijaX (ID=28) and XouijaY (ID=29) from #1383 A4.
    //
    void updateSourceValue(ModSourceId id, float value) noexcept
    {
        if (id == ModSourceId::XouijaX)
            ouijaLiveX_.store(value, std::memory_order_relaxed);
        else if (id == ModSourceId::XouijaY)
            ouijaLiveY_.store(value, std::memory_order_relaxed);
    }

    //==========================================================================
    // Audio-thread read accessors — called from XOceanusProcessor::processBlock.

    /// Legacy XouijaCell X-axis (ID=18), bipolar [-1, +1].
    float getXouijaCellX() const noexcept { return ouijaCellX_.load(std::memory_order_relaxed); }

    /// Legacy XouijaCell Y-axis (ID=18), bipolar [-1, +1].
    float getXouijaCellY() const noexcept { return ouijaCellY_.load(std::memory_order_relaxed); }

    /// XouijaX (ID=28): live planchette X axis.  #1383 A4.
    float getXouijaX() const noexcept { return ouijaLiveX_.load(std::memory_order_relaxed); }

    /// XouijaY (ID=29): live planchette Y axis (influence depth).  #1383 A4.
    float getXouijaY() const noexcept { return ouijaLiveY_.load(std::memory_order_relaxed); }

    //==========================================================================
    // reset — called from XOceanusProcessor::reset() / prepareToPlay().
    void reset() noexcept
    {
        ouijaCellX_.store(0.0f, std::memory_order_relaxed);
        ouijaCellY_.store(0.0f, std::memory_order_relaxed);
        ouijaLiveX_.store(0.0f, std::memory_order_relaxed);
        ouijaLiveY_.store(0.0f, std::memory_order_relaxed);
    }

private:
    // XouijaCell (ID=18) — back-compat atomics, bipolar [-1, +1].
    std::atomic<float> ouijaCellX_{0.0f};
    std::atomic<float> ouijaCellY_{0.0f};
    // XouijaX/Y (IDs=28/29) — per-axis atomics, #1383 A4.
    std::atomic<float> ouijaLiveX_{0.0f};
    std::atomic<float> ouijaLiveY_{0.0f};
};

} // namespace xoceanus
