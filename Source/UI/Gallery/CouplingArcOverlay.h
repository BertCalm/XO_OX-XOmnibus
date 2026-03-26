#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../../Core/MegaCouplingMatrix.h"

namespace xolokun {

//==============================================================================
// CouplingArcOverlay — transparent component drawn on top of the tile sidebar.
//
// Renders pulsing bioluminescent Bézier arcs between engine tile centres for
// every active coupling route in the MegaCouplingMatrix. The overlay is fully
// transparent to mouse events; clicks pass through to the tiles beneath.
//
// Usage:
//   - Construct with the XOlokunProcessor reference.
//   - Call setTileCenter(slot, centre) after tile bounds are set in resized().
//   - Add to the editor and set its bounds to cover the full editor area.
//   - The 30Hz timer drives continuous repaint; no other refresh needed.
//
// Arc geometry:
//   Control points bow 60px to the LEFT of the tile column so arcs never
//   overlap the right-side engine detail panel.
//
// Color coding by coupling type:
//   Audio routes  (FM / Ring / Wavetable / Buffer) → Twilight Blue   #0096C7
//   Modulation    (Amp / LFO / Env / Filter / Pitch / Rhythm)        → XO Gold #E9C46A
//   KnotTopology  (bidirectional entanglement)                        → Midnight Violet #7B2FBE
//
// Two-pass glow painting (Bézier cubic):
//   Pass 1: arcColor @ 0.08 alpha, 6px stroke — soft outer glow halo
//   Pass 2: arcColor @ glowAlpha, 2px stroke  — bright animated core
//   glowAlpha = 0.45 + 0.25 * sin(pulsePhase)  → range [0.20, 0.70]
//   pulsePhase advances 0.08 rad / timer tick (≈ 2.4 rad/s, ≈ 0.38 Hz)
//
class CouplingArcOverlay : public juce::Component, private juce::Timer
{
public:
    explicit CouplingArcOverlay(XOlokunProcessor& proc) : processor(proc)
    {
        setInterceptsMouseClicks(false, false); // pass-through to tiles beneath
        startTimerHz(30);
    }

    ~CouplingArcOverlay() override { stopTimer(); }

    // Called by XOlokunEditor::resized() once tile positions are finalised.
    // Centre is in the LOCAL coordinate space of this overlay component.
    void setTileCenter(int slot, juce::Point<float> centre)
    {
        if (slot >= 0 && slot < MegaCouplingMatrix::MaxSlots)
            tileCenters[static_cast<size_t>(slot)] = centre;
    }

    void timerCallback() override
    {
        // P1 fix: skip repaint when no active routes exist — avoids full
        // 1100×700 overlay redraw at 30Hz when the matrix is empty.
        const auto routes = processor.getCouplingMatrix().getRoutes();
        bool hasActive = false;
        for (const auto& r : routes)
        {
            if (r.active && r.amount >= 0.001f)
            { hasActive = true; break; }
        }
        if (hasActive)
            repaint();
    }

    void paint(juce::Graphics& g) override
    {
        // Snapshot coupling routes on the message thread — safe (getRoutes() uses atomic_load)
        const auto routes = processor.getCouplingMatrix().getRoutes();
        if (routes.empty())
            return;

        // Build index: key = (src << 2) | dst, value = max amount seen in this frame.
        // Collapse multiple routes on the same pair into one arc (visually cleaner).
        // We keep track of the dominant CouplingType for color selection.
        struct ArcInfo {
            float      amount = 0.0f;
            CouplingType type  = CouplingType::AmpToFilter;
        };
        std::array<ArcInfo, 12> arcMap {}; // C(5,2)=10 unique pairs with 5 slots; 12 for safety

        int arcCount = 0; // number of unique active pairs
        std::array<std::pair<int,int>, 12> arcPairs {};

        for (const auto& route : routes)
        {
            if (!route.active || route.amount < 0.001f)
                continue;
            if (route.sourceSlot < 0 || route.sourceSlot >= MegaCouplingMatrix::MaxSlots)
                continue;
            if (route.destSlot < 0 || route.destSlot >= MegaCouplingMatrix::MaxSlots)
                continue;

            // Canonical ordering: always store with lower slot first so we don't
            // draw two arcs between the same pair (forward + reverse KnotTopology).
            int lo = juce::jmin(route.sourceSlot, route.destSlot);
            int hi = juce::jmax(route.sourceSlot, route.destSlot);
            if (lo == hi) continue; // same slot — skip

            // Map (lo, hi) to a flat index.  5 slots → max 10 unique pairs.
            // We use a simple linear search over the live arcPairs array
            // because the count is always ≤ 10 — no hashmap overhead.
            int found = -1;
            for (int k = 0; k < arcCount; ++k)
                if (arcPairs[static_cast<size_t>(k)].first == lo && arcPairs[static_cast<size_t>(k)].second == hi)
                    { found = k; break; }

            if (found < 0 && arcCount < 12)
            {
                found = arcCount++;
                arcPairs[static_cast<size_t>(found)] = { lo, hi };
            }

            if (found >= 0)
            {
                auto& ai = arcMap[static_cast<size_t>(found)];
                if (route.amount > ai.amount)
                {
                    ai.amount = route.amount;
                    ai.type   = route.type;
                }
            }
        }

        if (arcCount == 0)
            return;

        for (int k = 0; k < arcCount; ++k)
        {
            const auto& [lo, hi] = arcPairs[static_cast<size_t>(k)];
            const auto& ai       = arcMap[static_cast<size_t>(k)];

            const auto from = tileCenters[static_cast<size_t>(lo)];
            const auto to   = tileCenters[static_cast<size_t>(hi)];

            // Skip degenerate arcs (tile center not yet set)
            if (from.isOrigin() && to.isOrigin())
                continue;

            // Choose arc color by coupling type category
            juce::Colour arcColor;
            switch (ai.type)
            {
                case CouplingType::AudioToFM:
                case CouplingType::AudioToRing:
                case CouplingType::AudioToWavetable:
                case CouplingType::AudioToBuffer:
                    arcColor = juce::Colour(0xFF0096C7); // Twilight Blue — audio-rate routes
                    break;
                case CouplingType::KnotTopology:
                    arcColor = juce::Colour(0xFF7B2FBE); // Midnight Violet — bidirectional entanglement
                    break;
                default:
                    arcColor = juce::Colour(0xFFE9C46A); // XO Gold — modulation routes
                    break;
            }

            // Bézier control points bow leftward (away from right panel)
            const float midX  = (from.x + to.x) * 0.5f - 60.0f;
            const juce::Point<float> cp1 (midX, from.y);
            const juce::Point<float> cp2 (midX, to.y);

            juce::Path arc;
            arc.startNewSubPath(from);
            arc.cubicTo(cp1, cp2, to);

            // Animate pulse — advance phase for this arc slot
            pulsePhase[static_cast<size_t>(k)] += 0.08f;
            if (pulsePhase[static_cast<size_t>(k)] > juce::MathConstants<float>::twoPi)
                pulsePhase[static_cast<size_t>(k)] -= juce::MathConstants<float>::twoPi;

            const float glowAlpha = 0.45f + 0.25f * std::sin(pulsePhase[static_cast<size_t>(k)]);

            // Pass 1: wide soft glow halo
            g.setColour(arcColor.withAlpha(0.08f));
            g.strokePath(arc, juce::PathStrokeType(6.0f));

            // Pass 2: bright animated core
            g.setColour(arcColor.withAlpha(glowAlpha));
            g.strokePath(arc, juce::PathStrokeType(2.0f));
        }
    }

private:
    XOlokunProcessor& processor;
    std::array<juce::Point<float>, MegaCouplingMatrix::MaxSlots> tileCenters {};
    float pulsePhase[12] {}; // one phase accumulator per unique arc pair (max 6 pairs, 12 for safety)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CouplingArcOverlay)
};

} // namespace xolokun
