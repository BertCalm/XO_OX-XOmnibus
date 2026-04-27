// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// Wave65SurfaceWiring.h
//
// Issue #1306 — Wave 6.5: Pad/Drum collision handling + editor wiring.
//
// This header provides:
//   1. isPercussionEngine(engineId) — returns true for engines that should
//      auto-switch the PlaySurface to PADS + drum sub-mode on load.
//      Currently: Onset, Offering.  Add more as fleet expands.
//
//   2. pollLayoutModeParams(apvts, cachedValues, playSurface) — call from
//      timerCallback() to detect APVTS slot[N]_layout_mode changes and
//      forward them to PlaySurface::setLayoutMode(). The cache array avoids
//      calling setLayoutMode() on every tick (would spam resized()).
//
// Mount instructions for XOceanusEditor.h:
// ─────────────────────────────────────────
//
//   A. After Wave 5 A3 modMatrixStrip block inside proc.onEngineChanged
//      (inside the MessageManager::callAsync lambda, slot 0..3 only):
//
//        // TODO Wave6.5 mount — A: auto-switch surface to PADS on percussion engines
//        if (slot >= 0 && slot < kNumPrimarySlots)
//        {
//            if (auto* eng = processor.getEngine(slot))
//                oceanView_.getPlaySurface().setSurfaceDefault(
//                    Wave65::isPercussionEngine(eng->getEngineId()));
//        }
//
//   B. After the PlaySurface accent colour block in timerCallback()
//      (approx. after `playSurface_.setAccentColour(accent);`):
//
//        // TODO Wave6.5 mount — B: forward slot[N]_layout_mode changes to PlaySurface
//        Wave65::pollLayoutModeParams(processor.getAPVTS(),
//                                     layoutModeCache_,
//                                     oceanView_.getPlaySurface());
//
//   C. Add `std::array<int, 4> layoutModeCache_ { -1, -1, -1, -1 };` to the
//      private members of XOceanusEditor (near lastLayoutMode_ comment block).
//
// Collision handling (item 1 from issue):
// ─────────────────────────────────────────
// The Wave 3 PanelCoordinator in OceanView already enforces the heavy-panel
// collision rule.  PADS/DRUM/XY tabs open SurfaceRightPanel via the tab bar
// callback which routes through coordinatorRequestOpen(PanelType::Detail)
// and the minimum-width guard.  No additional collision logic is required for
// Wave 6.5: the PADS tab is already wired into the coordinator as of Wave 3.
//
// For the record: the collision rule for PADS mode specifically is:
//   - Selecting PADS tab → opens SurfaceRightPanel (mode Pad)
//   - Opening DetailOverlay → hides SurfaceRightPanel (coordinator D7 rule)
//   - Closing DetailOverlay → restores SurfaceRightPanel to prior state
// All three already fire correctly via coordinatorRequestOpen / coordinatorRelease.
//==============================================================================

#include <juce_audio_processors/juce_audio_processors.h>
#include "../PlaySurface/PlaySurface.h"

namespace Wave65
{

//==============================================================================
/** Returns true if the given engine ID should auto-switch the PlaySurface to
    PADS + drum sub-mode (▦) when loaded into any slot.

    Percussion engines: Onset, Offering.
    Oware is a tuned percussion instrument — it can use KEYS mode appropriately,
    so it is intentionally excluded from auto-switch.  Overbite is a five-fang
    synth with percussive attack but pitched output — also excluded.
*/
inline bool isPercussionEngine (const juce::String& engineId)
{
    return engineId.equalsIgnoreCase ("Onset")
        || engineId.equalsIgnoreCase ("Offering");
}

//==============================================================================
/** Poll all four slot[N]_layout_mode APVTS parameters and call
    playSurface.setLayoutMode(newValue) if any have changed since last call.

    @param apvts        The processor's AudioProcessorValueTreeState.
    @param cache        Per-slot cache array (size 4). Init to { -1, -1, -1, -1 }.
                        Pass the same array on every call (state is retained between
                        calls to detect changes).
    @param playSurface  The PlaySurface to forward layout mode changes to.

    Thread-safety: call only from the message thread (timerCallback / UI thread).
*/
inline void pollLayoutModeParams (juce::AudioProcessorValueTreeState& apvts,
                                  std::array<int, 4>& cache,
                                  PlaySurface& playSurface)
{
    for (int s = 0; s < 4; ++s)
    {
        const juce::String paramId = "slot" + juce::String (s) + "_layout_mode";
        auto* raw = apvts.getRawParameterValue (paramId);
        if (raw == nullptr)
            continue;

        const int newVal = static_cast<int> (raw->load());
        if (newVal != cache[static_cast<size_t> (s)])
        {
            cache[static_cast<size_t> (s)] = newVal;
            // Only propagate the change for the "primary" focus slot (slot 0)
            // or the slot whose layout_mode parameter the user last touched.
            // PlaySurface::setLayoutMode is idempotent for mode 0 (no-op).
            playSurface.setLayoutMode (newVal);
        }
    }
}

} // namespace Wave65
