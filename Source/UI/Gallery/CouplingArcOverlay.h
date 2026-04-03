// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOceanusProcessor.h"
#include "../../Core/MegaCouplingMatrix.h"
#include "../GalleryColors.h"

namespace xoceanus
{

//==============================================================================
// CouplingArcOverlay — transparent component drawn on top of the tile sidebar.
//
// Renders pulsing bioluminescent Bézier arcs between engine tile centres for
// every active coupling route in the MegaCouplingMatrix. The overlay is fully
// transparent to mouse events; clicks pass through to the tiles beneath.
//
// Usage:
//   - Construct with the XOceanusProcessor reference.
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
//   Pass 1: arcColor @ 0.08 alpha, (strokeWidth+2)px — soft outer glow halo
//   Pass 2: arcColor @ glowAlpha,  (strokeWidth*0.5)px — bright animated core
//   strokeWidth = 4 + amount*4  → range [4px, 8px]
//   baseAlpha   = 0.15 + amount*0.55  → range [0.15, 0.70]
//   glowAlpha   = baseAlpha * (0.8 + 0.2*sin(pulsePhase))
//   pulsePhase advances 0.08 rad / timer tick (≈ 2.4 rad/s, ≈ 0.38 Hz)
//
// Coupling Currents (Vision Quest 015):
//   Each active arc renders 3 + amount*9 animated dots (capped at NumParticlesPerArc)
//   flowing along the Bézier path from source to destination.
//
//   FlowParticle.t ∈ [0, 1) tracks position along the path.
//   Speed scales with coupling amount: speed = 0.005 + amount * 0.03
//     → Low coupling:  lazy drift  (~0.005 / tick)
//     → High coupling: rapid flow  (~0.035 / tick at amount=1)
//
//   Render per particle:
//     Glow pass:  8px circle, arcColor @ 0.30 * particle.alpha
//     Core pass:  3–5px circle (lerped by t), arcColor @ particle.alpha
//
//   Particles are initialised once per arc slot with evenly-spaced t values
//   and re-initialised whenever a previously-inactive arc becomes active.
//
class CouplingArcOverlay : public juce::Component, private juce::Timer
{
public:
    explicit CouplingArcOverlay(XOceanusProcessor& proc) : processor(proc)
    {
        setInterceptsMouseClicks(false, false); // pass-through to tiles beneath
        cachedRoutes.reserve(16);               // Fix #14: pre-allocate to avoid reallocation at steady state
        // A11Y06: respect reduced-motion preference — drop to 1Hz refresh when active
        if (A11y::prefersReducedMotion())
            startTimerHz(1);
        else
            startTimerHz(30);
    }

    ~CouplingArcOverlay() override { stopTimer(); }

    // Called by XOceanusEditor::resized() once tile positions are finalised.
    // Centre is in the LOCAL coordinate space of this overlay component.
    void setTileCenter(int slot, juce::Point<float> centre)
    {
        if (slot >= 0 && slot < MegaCouplingMatrix::MaxSlots)
            tileCenters[static_cast<size_t>(slot)] = centre;
    }

    // Fix #387: dirty-flag gating + adaptive timer rate.
    // When no coupling routes are active, poll at 1 Hz (coupling route changes
    // happen at human interaction rate, ~1 Hz). When active routes exist, run
    // at 30 Hz for smooth arc animation. This eliminates ~29 wasted polling
    // cycles/second during idle (no coupling set up).
    void timerCallback() override
    {
        if (!isVisible())
            return;

        // P1 fix: skip repaint when no active routes exist — avoids full
        // 1100×700 overlay redraw at 30Hz when the matrix is empty.
        // P26 fix: cache the routes here so paint() doesn't call getRoutes() again.
        cachedRoutes = processor.getCouplingMatrix().getRoutes();
        bool hasActive = false;
        for (const auto& r : cachedRoutes)
        {
            if (r.active && r.amount >= 0.001f)
            {
                hasActive = true;
                break;
            }
        }

        if (hasActive)
        {
            // Wake to 30 Hz if we were in idle polling mode
            if (getTimerInterval() != 33)
                startTimerHz(30);
            repaint();
        }
        else
        {
            // Step down to 1 Hz idle polling when no routes are active — coupling
            // route changes are a deliberate user action, not a continuous stream.
            if (getTimerInterval() == 33)
                startTimerHz(1);
        }
    }

    void paint(juce::Graphics& g) override
    {
        // Use the routes snapshot captured in timerCallback() — no extra copy.
        const auto& routes = cachedRoutes;
        if (routes.empty())
            return;

        // Build index: key = (src << 2) | dst, value = max amount seen in this frame.
        // Collapse multiple routes on the same pair into one arc (visually cleaner).
        // We keep track of the dominant CouplingType for color selection.
        struct ArcInfo
        {
            float amount = 0.0f;
            CouplingType type = CouplingType::AmpToFilter;
        };
        std::array<ArcInfo, 12> arcMap{}; // C(5,2)=10 unique pairs with 5 slots; 12 for safety

        int arcCount = 0; // number of unique active pairs
        std::array<std::pair<int, int>, 12> arcPairs{};

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
            if (lo == hi)
                continue; // same slot — skip

            // Map (lo, hi) to a flat index.  5 slots → max 10 unique pairs.
            // We use a simple linear search over the live arcPairs array
            // because the count is always ≤ 10 — no hashmap overhead.
            int found = -1;
            for (int k = 0; k < arcCount; ++k)
                if (arcPairs[static_cast<size_t>(k)].first == lo && arcPairs[static_cast<size_t>(k)].second == hi)
                {
                    found = k;
                    break;
                }

            if (found < 0 && arcCount < 12)
            {
                found = arcCount++;
                arcPairs[static_cast<size_t>(found)] = {lo, hi};
            }

            if (found >= 0)
            {
                auto& ai = arcMap[static_cast<size_t>(found)];
                if (route.amount > ai.amount)
                {
                    ai.amount = route.amount;
                    ai.type = route.type;
                }
            }
        }

        if (arcCount == 0)
            return;

        for (int k = 0; k < arcCount; ++k)
        {
            const auto& [lo, hi] = arcPairs[static_cast<size_t>(k)];
            const auto& ai = arcMap[static_cast<size_t>(k)];

            const auto from = tileCenters[static_cast<size_t>(lo)];
            const auto to = tileCenters[static_cast<size_t>(hi)];

            // CQ12: skip if EITHER endpoint is at origin (tile center not yet set).
            // Using && was too permissive — a valid arc could still draw toward (0,0).
            if (from.isOrigin() || to.isOrigin())
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
            const float midX = (from.x + to.x) * 0.5f - 60.0f;
            const juce::Point<float> cp1(midX, from.y);
            const juce::Point<float> cp2(midX, to.y);

            juce::Path arc;
            arc.startNewSubPath(from);
            arc.cubicTo(cp1, cp2, to);

            // Animate pulse — advance phase for this arc slot
            pulsePhase[static_cast<size_t>(k)] += 0.08f;
            if (pulsePhase[static_cast<size_t>(k)] > juce::MathConstants<float>::twoPi)
                pulsePhase[static_cast<size_t>(k)] -= juce::MathConstants<float>::twoPi;

            // Amount-weighted glow: low coupling = subtle, high coupling = vivid.
            // baseAlpha range: 0.15 (amount=0) → 0.70 (amount=1).
            const float baseAlpha = 0.15f + ai.amount * 0.55f;
            const float glowAlpha = baseAlpha * (0.8f + 0.2f * std::sin(pulsePhase[static_cast<size_t>(k)]));

            // Stroke width scales with coupling amount: 4px (0%) → 8px (100%).
            const float strokeWidth = 4.0f + ai.amount * 4.0f;

            // Pass 1: wide soft glow halo
            g.setColour(arcColor.withAlpha(0.08f));
            g.strokePath(arc, juce::PathStrokeType(strokeWidth + 2.0f));

            // Pass 2: bright animated core
            g.setColour(arcColor.withAlpha(glowAlpha));
            g.strokePath(arc, juce::PathStrokeType(strokeWidth * 0.5f));

            // ----------------------------------------------------------------
            // Coupling Currents — particle flow (Vision Quest 015)
            // ----------------------------------------------------------------

            // Lazily initialise particles for this arc slot when first activated
            // or when the slot was previously unused (all t values are 0 together,
            // which would cluster every particle at the source — spread them out).
            auto& particles = flowParticles[static_cast<size_t>(k)];
            if (!particleInitialised[static_cast<size_t>(k)])
            {
                for (int p = 0; p < NumParticlesPerArc; ++p)
                {
                    particles[static_cast<size_t>(p)].t =
                        static_cast<float>(p) / static_cast<float>(NumParticlesPerArc);
                    particles[static_cast<size_t>(p)].alpha = 0.6f;
                }
                particleInitialised[static_cast<size_t>(k)] = true;
            }

            // Speed scales with coupling amount so high-coupling feels energetic.
            const float speed = 0.005f + ai.amount * 0.03f;

            // Particle count scales with coupling amount: 3 (amount≈0) → 12 (amount=1).
            const int nParticles = std::min(3 + static_cast<int>(ai.amount * 9.0f), NumParticlesPerArc);

            // Pre-compute arc path length for getPointAlongPath() calls.
            const float arcLength = arc.getLength();

            for (int p = 0; p < nParticles; ++p)
            {
                auto& particle = particles[static_cast<size_t>(p)];

                // Advance position and wrap
                particle.t += speed;
                if (particle.t >= 1.0f)
                    particle.t -= 1.0f;

                // Evaluate position on the Bézier path via JUCE's arc-length
                // parameterisation (t=0 → source, t=1 → destination).
                const float distAlongPath = particle.t * arcLength;
                const auto pos = arc.getPointAlongPath(distAlongPath);

                // Particle diameter interpolates from 3px (near source) to 5px
                // (near destination) to indicate flow direction.
                const float diameter = 3.0f + particle.t * 2.0f;
                const float radius = diameter * 0.5f;

                // Glow pass: larger halo at 30% of particle alpha
                g.setColour(arcColor.withAlpha(particle.alpha * 0.30f));
                g.fillEllipse(pos.x - 4.0f, pos.y - 4.0f, 8.0f, 8.0f);

                // Core dot pass: solid filled circle
                g.setColour(arcColor.withAlpha(particle.alpha));
                g.fillEllipse(pos.x - radius, pos.y - radius, diameter, diameter);
            }
        }

        // Clear initialisation flags for arc slots that were not active this frame
        // so their particles will be re-spread when the route re-activates later.
        for (int k = arcCount; k < MaxArcSlots; ++k)
            particleInitialised[static_cast<size_t>(k)] = false;
    }

private:
    // -------------------------------------------------------------------------
    // FlowParticle — one animated dot travelling along a coupling arc
    // -------------------------------------------------------------------------
    struct FlowParticle
    {
        float t = 0.0f;     // normalised position along path: 0 = source, 1 = dest
        float alpha = 0.6f; // particle opacity (fixed; could be modulated in future)
    };

    static constexpr int NumParticlesPerArc = 12; // particles per unique arc pair
    static constexpr int MaxArcSlots = 12;        // must match arcMap / arcPairs size

    XOceanusProcessor& processor;
    std::array<juce::Point<float>, MegaCouplingMatrix::MaxSlots> tileCenters{};
    float pulsePhase[MaxArcSlots]{}; // one phase accumulator per unique arc pair

    // Particle state — indexed [arcSlot][particleIndex]
    std::array<std::array<FlowParticle, NumParticlesPerArc>, MaxArcSlots> flowParticles{};
    bool particleInitialised[MaxArcSlots]{}; // false → spread particles evenly on next paint

    std::vector<MegaCouplingMatrix::CouplingRoute> cachedRoutes; // P26: populated in timerCallback()

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CouplingArcOverlay)
};

} // namespace xoceanus
