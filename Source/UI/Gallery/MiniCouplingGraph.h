// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// MiniCouplingGraph.h — Mini coupling node diagram for the bottom of Column A.
//
// Renders a compact (260×80pt) snapshot of the coupling topology: 5 slot nodes
// arranged vertically to match the CompactEngineTile positions, connected by
// pulsing Bézier arcs that bow leftward (consistent with CouplingArcOverlay).
//
// Color coding (matches CouplingArcOverlay):
//   Audio routes  (AudioToFM / AudioToRing / AudioToWavetable / AudioToBuffer)
//                 → Twilight Blue  #0096C7
//   Modulation    (all other types)
//                 → XO Gold       #E9C46A
//   KnotTopology  (bidirectional entanglement)
//                 → Midnight Violet #7B2FBE
//
// Arc animation: 0.3 Hz pulse. Alpha oscillates in [0.35, 0.60].
// Timer rate: 5 Hz (low visual priority — miniature widget).
//
// Integration (XOceanusEditor):
//   addAndMakeVisible(miniCouplingGraph);
//   // In resized(), after tile bounds are set:
//   miniCouplingGraph.setBounds(columnA.removeFromBottom(80));
//   // In timerCallback():
//   miniCouplingGraph.refresh();
//   for (int i = 0; i < 5; ++i)
//       miniCouplingGraph.setNodeCenter(i, tiles[i].getBounds().getCentreY());

#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOceanusProcessor.h"
#include "../../Core/MegaCouplingMatrix.h"
#include "../../Core/SynthEngine.h"
#include "../GalleryColors.h"
#include "CockpitHost.h"

namespace xoceanus {

//==============================================================================
class MiniCouplingGraph : public juce::Component, private juce::Timer
{
public:
    explicit MiniCouplingGraph(XOceanusProcessor& proc)
        : processor(proc)
    {
        setInterceptsMouseClicks(false, false); // pass-through; read-only diagram

        A11y::setup(*this, "Coupling Graph", "Mini visualization of active engine coupling routes");

        // Default node Y positions: evenly spaced placeholders.
        // Overwritten by setNodeCenter() once tile bounds are finalised.
        for (int i = 0; i < MegaCouplingMatrix::MaxSlots; ++i)
            nodeY[static_cast<size_t>(i)] = 0.0f;

        startTimerHz(5);
    }

    ~MiniCouplingGraph() override { stopTimer(); }

    //--------------------------------------------------------------------------
    // Called by XOceanusEditor::resized() (and timerCallback) to keep node
    // positions aligned with tile centres. Only the Y coordinate is variable;
    // X is fixed to the horizontal centre of this component.
    void setNodeCenter(int slot, float y)
    {
        if (slot >= 0 && slot < MegaCouplingMatrix::MaxSlots)
            nodeY[static_cast<size_t>(slot)] = y;
    }

    //--------------------------------------------------------------------------
    // Re-read engine presence and coupling routes from the processor.
    // Must be called on the message thread. The editor's timerCallback() does
    // this automatically; call manually after engine load/unload if needed.
    void refresh()
    {
        // Engine presence — determines filled vs. empty node style.
        for (int i = 0; i < MegaCouplingMatrix::MaxSlots; ++i)
        {
            auto* eng = processor.getEngine(i);
            slotHasEngine[static_cast<size_t>(i)] = (eng != nullptr);
            slotAccent[static_cast<size_t>(i)] = eng
                ? eng->getAccentColour()
                : GalleryColors::get(GalleryColors::borderGray()).withAlpha(0.30f);
        }

        // Coupling routes — collapse to unique (lo, hi) pairs, same as CouplingArcOverlay.
        arcs.clear();

        const auto routes = processor.getCouplingMatrix().getRoutes();
        if (routes.empty())
            return;

        // Temporary collapse table: max C(5,2) = 10 unique pairs.
        struct PairEntry {
            int  lo = -1, hi = -1;
            float amount = 0.0f;
            CouplingType type = CouplingType::AmpToFilter;
        };
        std::array<PairEntry, 12> table {};
        int count = 0;

        for (const auto& route : routes)
        {
            if (!route.active || route.amount < 0.001f)
                continue;
            if (route.sourceSlot < 0 || route.sourceSlot >= MegaCouplingMatrix::MaxSlots)
                continue;
            if (route.destSlot < 0 || route.destSlot >= MegaCouplingMatrix::MaxSlots)
                continue;

            int lo = juce::jmin(route.sourceSlot, route.destSlot);
            int hi = juce::jmax(route.sourceSlot, route.destSlot);
            if (lo == hi) continue;

            // Find existing entry for this pair.
            int found = -1;
            for (int k = 0; k < count; ++k)
                if (table[static_cast<size_t>(k)].lo == lo &&
                    table[static_cast<size_t>(k)].hi == hi)
                    { found = k; break; }

            if (found < 0 && count < 12)
            {
                found = count++;
                table[static_cast<size_t>(found)].lo = lo;
                table[static_cast<size_t>(found)].hi = hi;
            }

            if (found >= 0 && route.amount > table[static_cast<size_t>(found)].amount)
            {
                table[static_cast<size_t>(found)].amount = route.amount;
                table[static_cast<size_t>(found)].type   = route.type;
            }
        }

        arcs.reserve(static_cast<size_t>(count));
        for (int k = 0; k < count; ++k)
        {
            const auto& e = table[static_cast<size_t>(k)];
            ArcData ad;
            ad.src    = e.lo;
            ad.dst    = e.hi;
            ad.amount = e.amount;
            ad.color  = colorForType(e.type);
            arcs.push_back(ad);
        }

        repaint();
    }

    //--------------------------------------------------------------------------
    void timerCallback() override
    {
        // Advance pulse phase at 0.3 Hz (≈ 1.885 rad/s).
        // At 5 Hz timer: step = 2π × 0.3 / 5 ≈ 0.3770 rad per tick.
        pulsePhase += 0.3770f;
        if (pulsePhase > juce::MathConstants<float>::twoPi)
            pulsePhase -= juce::MathConstants<float>::twoPi;

        // P9 fix: only repaint when there are active arcs to animate.
        // With no routes the phase advance is still kept current for when
        // routes appear, but we skip the 260×80pt repaint while idle.
        if (!arcs.empty())
            repaint();
    }

    //--------------------------------------------------------------------------
    void paint(juce::Graphics& g) override
    {
        // Dark Cockpit B041: apply performance opacity
        float opacity = 1.0f;
        if (auto* host = CockpitHost::find(this))
            opacity = host->getCockpitOpacity();
        if (opacity < 0.05f) return; // B041 performance optimization
        g.setOpacity(opacity);

        using namespace GalleryColors;

        // Border-top separator — visual separation from tile list above
        g.setColour(GalleryColors::border());
        g.fillRect(0.0f, 0.0f, (float)getWidth(), 1.0f);

        const float w    = static_cast<float>(getWidth());
        const float nodeX = w * 0.5f;  // nodes centred horizontally

        //-- Pass 1: arcs (drawn behind nodes) ---------------------------------
        if (!arcs.empty())
        {
            const float alpha = 0.35f + 0.25f * std::sin(pulsePhase);

            for (const auto& arc : arcs)
            {
                const float y0 = nodeY[static_cast<size_t>(arc.src)];
                const float y1 = nodeY[static_cast<size_t>(arc.dst)];

                // Skip degenerate arcs (node positions not yet assigned).
                if (y0 <= 0.0f && y1 <= 0.0f)
                    continue;

                // Bézier control points bow 40px to the LEFT (away from right panel),
                // matching the direction used by CouplingArcOverlay (which bows 60px).
                const float midX = nodeX - 40.0f;
                const juce::Point<float> from (nodeX, y0);
                const juce::Point<float> to   (nodeX, y1);
                const juce::Point<float> cp1  (midX,  y0);
                const juce::Point<float> cp2  (midX,  y1);

                juce::Path path;
                path.startNewSubPath(from);
                path.cubicTo(cp1, cp2, to);

                // Soft outer glow halo
                g.setColour(arc.color.withAlpha(0.08f));
                g.strokePath(path, juce::PathStrokeType(5.0f));

                // Bright animated core
                g.setColour(arc.color.withAlpha(alpha));
                g.strokePath(path, juce::PathStrokeType(1.5f));
            }
        }

        //-- Pass 2: nodes -----------------------------------------------------
        for (int i = 0; i < MegaCouplingMatrix::MaxSlots; ++i)
        {
            const float cy = nodeY[static_cast<size_t>(i)];
            if (cy <= 0.0f)
                continue; // position not yet assigned

            const bool filled = slotHasEngine[static_cast<size_t>(i)];

            if (filled)
            {
                // Active node: engine accent color, 8pt filled circle.
                const float r = 4.0f; // radius = half of 8pt diameter
                g.setColour(slotAccent[static_cast<size_t>(i)]);
                g.fillEllipse(nodeX - r, cy - r, r * 2.0f, r * 2.0f);

                // Subtle accent ring for definition against light backgrounds.
                g.setColour(slotAccent[static_cast<size_t>(i)].darker(0.2f).withAlpha(0.5f));
                g.drawEllipse(nodeX - r, cy - r, r * 2.0f, r * 2.0f, 0.75f);
            }
            else
            {
                // Empty node: borderGray at 30% alpha, 6pt outlined circle.
                const float r = 3.0f; // radius = half of 6pt diameter
                g.setColour(get(borderGray()).withAlpha(0.30f));
                g.drawEllipse(nodeX - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);
            }
        }
    }

private:
    //--------------------------------------------------------------------------
    // Map CouplingType to the canonical arc color used across all coupling UI.
    static juce::Colour colorForType(CouplingType t)
    {
        switch (t)
        {
            case CouplingType::AudioToFM:
            case CouplingType::AudioToRing:
            case CouplingType::AudioToWavetable:
            case CouplingType::AudioToBuffer:
                return juce::Colour(0xFF0096C7); // Twilight Blue — audio-rate routes

            case CouplingType::KnotTopology:
                return juce::Colour(0xFF7B2FBE); // Midnight Violet — bidirectional entanglement

            default:
                return juce::Colour(0xFFE9C46A); // XO Gold — modulation routes
        }
    }

    //--------------------------------------------------------------------------
    struct ArcData {
        int src = 0, dst = 0;
        float amount = 0.0f;
        juce::Colour color;
    };

    XOceanusProcessor& processor;

    // Vertical centres for the 5 slot nodes (local Y in this component's space).
    // Updated via setNodeCenter(); X is always centred on the component width.
    std::array<float, MegaCouplingMatrix::MaxSlots> nodeY {};

    // Per-slot state (updated in refresh()).
    std::array<bool,         MegaCouplingMatrix::MaxSlots> slotHasEngine {};
    std::array<juce::Colour, MegaCouplingMatrix::MaxSlots> slotAccent    {};

    // Collapsed arc list (at most 10 unique pairs for 5 slots).
    std::vector<ArcData> arcs;

    // Single shared pulse phase — all arcs animate in sync for visual cohesion
    // at this miniature scale (per-arc phases would be visually noisy at 260×80pt).
    float pulsePhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiniCouplingGraph)
};

} // namespace xoceanus
