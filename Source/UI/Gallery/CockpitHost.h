// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace xoceanus {

//==============================================================================
// CockpitHost — abstract interface implemented by XOceanusEditor.
//
// CockpitHost is intentionally NOT a juce::Component subclass to avoid a
// virtual-inheritance diamond conflict with juce::AudioProcessorEditor.
// Instead, use the CockpitHost::find() static helper to walk the parent chain:
//
//   // Inside a panel's paint():
//   float opacity = 1.0f;
//   if (auto* host = CockpitHost::find(this))
//       opacity = host->getCockpitOpacity();
//   g.setOpacity(opacity);
//   if (opacity < 0.05f) return; // B041 performance optimization
//
class CockpitHost
{
public:
    // Returns the current performance opacity (0.15–1.0).
    // 0.15 = ghost (silent), 1.0 = fully lit (maximum note activity).
    virtual float getCockpitOpacity() const = 0;

    // Walk parent components looking for a CockpitHost.
    // Returns nullptr if no CockpitHost is found in the ancestor chain.
    // O(depth) — typically 3-5 levels — safe to call from paint().
    static CockpitHost* find(juce::Component* start)
    {
        for (auto* c = start ? start->getParentComponent() : nullptr;
             c != nullptr;
             c = c->getParentComponent())
        {
            if (auto* host = dynamic_cast<CockpitHost*>(c))
                return host;
        }
        return nullptr;
    }

protected:
    ~CockpitHost() = default;
};

} // namespace xoceanus
