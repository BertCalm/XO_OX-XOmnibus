// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// CouplingSubstrate.h — Luminescent coupling-thread layer for the Ocean View.
//
// Renders curved Bézier lines between engine creature positions, representing
// active coupling routes.  This is the substrate of the ocean: coupling is
// always visible, never relegated to a sidebar tab.
//
// Replaces CouplingArcOverlay.h (which drew arcs between column tiles in the
// old Gallery Model layout).
//
// Features:
//   - Quadratic Bézier threads between creature centres
//   - Stroke width proportional to coupling amount (1 px at 10 %, 4 px at 100 %)
//   - Per-type colour from CouplingColors.h
//   - Pulsing opacity (sine, 3-second period)
//   - Particle flow along each thread (6 particles per route by default)
//   - Accessibility: reduced-motion gate freezes phases and skips particle advance
//   - Empty state: nothing drawn — OceanBackground rings handle that case
//
// Usage:
//   CouplingSubstrate substrate;
//   addAndMakeVisible(substrate);
//   substrate.setBounds(getLocalBounds());
//   substrate.setCreatureCenter(slot, center);
//   substrate.setRoutes(matrix.getRoutes());

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CouplingColors.h"   // CouplingTypeColors::forType()
#include "../GalleryColors.h"    // A11y::prefersReducedMotion()

#include <array>
#include <cmath>
#include <vector>

namespace xoceanus
{

//==============================================================================
/**
    Describes a single active coupling route between two engine slots.

    Store CouplingType as int so this struct stays POD-friendly and avoids
    a dependency on SynthEngine.h in callers that already have the matrix data
    as raw integers.  colourForType() casts back to CouplingType when needed.
*/
struct CouplingRoute
{
    int   sourceSlot = -1;
    int   destSlot   = -1;
    int   type       = 0;      ///< CouplingType cast to int
    float amount     = 0.0f;   ///< 0.0 – 1.0
};

//==============================================================================
/**
    CouplingSubstrate

    A transparent, non-opaque JUCE Component that overlays the Ocean View and
    paints luminescent coupling threads between engine creature positions.

    Coordinate space: the component should share its bounds with the parent
    Ocean View so that creature-centre positions set via setCreatureCenter() are
    in the component's local coordinate system.

    Timer: runs at 30 Hz when routes are active, stops automatically when the
    route list is empty to avoid waking the message thread needlessly.
*/
class CouplingSubstrate : public juce::Component, private juce::Timer
{
public:
    //==========================================================================
    // Rendering constants — all constexpr so the compiler can fold them.
    static constexpr int   kMaxSlots         = 5;
    static constexpr float kMinStroke        = 1.0f;    ///< px at amount = 0
    static constexpr float kMaxStroke        = 4.0f;    ///< px at amount = 1
    static constexpr float kGlowAlphaBase    = 0.6f;    ///< peak pulse alpha (core line)
    static constexpr float kGlowAlphaMin     = 0.3f;    ///< trough pulse alpha (core line)
    static constexpr float kPulsePeriod      = 3.0f;    ///< seconds per full sine cycle
    static constexpr int   kParticlesPerRoute = 6;      ///< particles per route (RouteState uses 8 slots)
    static constexpr float kParticleSpeed    = 0.15f;   ///< fraction of path per second
    static constexpr float kParticleDiameter = 4.0f;   ///< px
    static constexpr float kControlBowFactor = 0.15f;  ///< bow = chordLength * factor, clamped [20,80]
    static constexpr int   kTimerHz          = 30;      ///< timer frequency

private:
    //==========================================================================
    /** Single particle: position along the Bézier path expressed as a
        normalised parameter t ∈ [0, 1). */
    struct Particle
    {
        float t = 0.0f;
    };

    //==========================================================================
    /** Per-route animation state, including the pre-built path. */
    struct RouteState
    {
        CouplingRoute route;

        /** Pre-built quadratic Bézier path for this route. */
        juce::Path path;

        /** Control point of the Bézier (needed for analytical point-on-curve). */
        juce::Point<float> control;

        /** Phase accumulator in radians; advanced by timerCallback(). */
        float pulsePhase = 0.0f;

        /** Bow direction: +1 or -1 (alternates per route to spread overlapping threads). */
        float bowSign = 1.0f;

        /** Particles for this route.  Only kParticlesPerRoute are active;
            the array is fixed-size to avoid heap churn in the timer callback. */
        std::array<Particle, 8> particles{};
    };

    //==========================================================================
    std::array<juce::Point<float>, kMaxSlots> centers_;
    std::vector<RouteState>                   routeStates_;

public:
    //==========================================================================
    CouplingSubstrate()
    {
        // Transparent overlay — do not fill background.
        setOpaque(false);
        // Substrate is purely decorative; mouse events pass through to creatures.
        setInterceptsMouseClicks(false, false);
        // Zero-initialise creature centres to an off-screen sentinel.
        centers_.fill(juce::Point<float>(-1.0f, -1.0f));

        A11y::setup(*this,
                    "Coupling Substrate",
                    "Luminescent coupling threads between engine creatures",
                    /*wantsKeyFocus=*/false);
    }

    //==========================================================================
    /**
        Set the centre position of an engine creature in local coordinates.

        @param slot   Engine slot index, 0 – (kMaxSlots-1).
        @param center Centre point in the component's local coordinate space.
    */
    void setCreatureCenter(int slot, juce::Point<float> center)
    {
        if (slot < 0 || slot >= kMaxSlots)
            return;

        if (centers_[slot] == center)
            return;

        centers_[slot] = center;
        rebuildPaths();
        repaint();
    }

    //==========================================================================
    /**
        Replace the full list of active coupling routes.

        Existing RouteState pulse phases are reset; particles are seeded with
        evenly-spaced t values so they fan out immediately rather than all
        starting at t = 0.

        Starts the 30 Hz timer when routes become non-empty; stops it when the
        list is empty (avoids unnecessary wakeups).

        @param routes  New set of routes.  The vector is copied.
    */
    void setRoutes(const std::vector<CouplingRoute>& routes)
    {
        routeStates_.clear();
        routeStates_.reserve(routes.size());

        float bowSign = 1.0f;
        const int numRoutes = static_cast<int>(routes.size());
        int routeIdx = 0;
        for (const auto& route : routes)
        {
            RouteState rs;
            rs.route = route;
            // MEDIUM fix (#1006): seed pulse phases distributed across [0, 2π)
            // so routes do not all peak simultaneously on load (sync-flash bug).
            // Phase = i / N × 2π, giving an evenly-spread stagger.
            rs.pulsePhase = (numRoutes > 1)
                ? (static_cast<float>(routeIdx) / static_cast<float>(numRoutes))
                    * juce::MathConstants<float>::twoPi
                : 0.0f;
            rs.bowSign   = bowSign;
            bowSign = -bowSign;  // alternate bow direction
            ++routeIdx;

            // Seed particles with evenly-spaced t values.
            const float step = 1.0f / static_cast<float>(kParticlesPerRoute);
            for (int p = 0; p < kParticlesPerRoute; ++p)
                rs.particles[static_cast<size_t>(p)].t = static_cast<float>(p) * step;
            // Remaining slots (if kParticlesPerRoute < 8) are left at t = 0 but
            // are never iterated in paint() since we loop to kParticlesPerRoute.

            routeStates_.push_back(std::move(rs));
        }

        rebuildPaths();

        // Manage timer lifecycle.
        if (!routeStates_.empty())
        {
            if (!isTimerRunning())
                startTimerHz(kTimerHz);
        }
        else
        {
            stopTimer();
        }

        repaint();
    }

    //==========================================================================
    /** Returns true if there is at least one active coupling route. */
    bool hasActiveRoutes() const { return !routeStates_.empty(); }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        if (routeStates_.empty())
            return;

        for (const auto& rs : routeStates_)
        {
            const juce::Colour baseColour = colourForType(rs.route.type);

            // 1. Compute stroke width — linear mapping: amount → [kMinStroke, kMaxStroke].
            //    Clamp amount to [0, 1] so malformed routes don't produce negative widths.
            const float amount = juce::jlimit(0.0f, 1.0f, rs.route.amount);
            const float strokeWidth = kMinStroke + amount * (kMaxStroke - kMinStroke);

            // 2. Compute pulse alpha — sinusoidal pulse in [kGlowAlphaMin, kGlowAlphaBase].
            const float alpha = kGlowAlphaMin
                + (kGlowAlphaBase - kGlowAlphaMin)
                    * (0.5f + 0.5f * std::sin(rs.pulsePhase));

            // 3. Outer glow pass — 2× stroke width, very low alpha.
            {
                juce::PathStrokeType glowStroke(strokeWidth * 2.0f,
                                                juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded);
                g.setColour(baseColour.withAlpha(alpha * 0.15f));
                g.strokePath(rs.path, glowStroke);
            }

            // 4. Core line pass — computed stroke width, full pulse alpha.
            {
                juce::PathStrokeType coreStroke(strokeWidth,
                                                juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded);
                g.setColour(baseColour.withAlpha(alpha));
                g.strokePath(rs.path, coreStroke);
            }

            // 5. Particles — small filled circles positioned along the Bézier.
            {
                const float particleAlpha = alpha * 0.9f;
                g.setColour(baseColour.withAlpha(particleAlpha));

                // Retrieve end-points from the route for the analytic formula.
                const int   src = rs.route.sourceSlot;
                const int   dst = rs.route.destSlot;
                const bool  srcValid = (src >= 0 && src < kMaxSlots);
                const bool  dstValid = (dst >= 0 && dst < kMaxSlots);
                if (!srcValid || !dstValid)
                    continue;

                const juce::Point<float> p0 = centers_[static_cast<size_t>(src)];
                const juce::Point<float> p2 = centers_[static_cast<size_t>(dst)];
                const juce::Point<float> p1 = rs.control;

                // Skip if either center is still at the off-screen sentinel (-1, -1).
                // Matches the guard used in rebuildPaths() to avoid painting particles
                // along degenerate paths before creature positions are registered.
                if (p0.x < 0.0f || p2.x < 0.0f)
                    continue;

                for (int p = 0; p < kParticlesPerRoute; ++p)
                {
                    const float t   = rs.particles[static_cast<size_t>(p)].t;
                    const float it  = 1.0f - t;
                    // Quadratic Bézier: B(t) = (1-t)²·P₀ + 2(1-t)t·P₁ + t²·P₂
                    const float px  = it * it * p0.x + 2.0f * it * t * p1.x + t * t * p2.x;
                    const float py  = it * it * p0.y + 2.0f * it * t * p1.y + t * t * p2.y;
                    const float r   = kParticleDiameter * 0.5f;
                    g.fillEllipse(px - r, py - r, kParticleDiameter, kParticleDiameter);
                }
            }
        }
    }

private:
    //==========================================================================
    /** Timer callback — advances animation state at kTimerHz and repaints. */
    void timerCallback() override
    {
        const bool reducedMotion = A11y::prefersReducedMotion();

        if (!reducedMotion)
        {
            // Phase advance per tick: one full cycle in kPulsePeriod seconds.
            const float phaseStep = juce::MathConstants<float>::twoPi
                                    / (kPulsePeriod * static_cast<float>(kTimerHz));
            // Particle advance per tick.
            const float tStep = kParticleSpeed / static_cast<float>(kTimerHz);

            for (auto& rs : routeStates_)
            {
                rs.pulsePhase += phaseStep;
                if (rs.pulsePhase >= juce::MathConstants<float>::twoPi)
                    rs.pulsePhase -= juce::MathConstants<float>::twoPi;

                for (int p = 0; p < kParticlesPerRoute; ++p)
                {
                    auto& particle = rs.particles[static_cast<size_t>(p)];
                    particle.t += tStep;
                    if (particle.t >= 1.0f)
                        particle.t -= 1.0f;  // wrap in [0, 1)
                }
            }
        }
        // When reducedMotion is true: phases and particles are frozen (no advance).

        repaint();
    }

    //==========================================================================
    /**
        Rebuild the juce::Path and Bézier control point for every RouteState.
        Called whenever centres or routes change.
    */
    void rebuildPaths()
    {
        for (auto& rs : routeStates_)
        {
            const int src = rs.route.sourceSlot;
            const int dst = rs.route.destSlot;

            if (src < 0 || src >= kMaxSlots || dst < 0 || dst >= kMaxSlots)
            {
                rs.path.clear();
                continue;
            }

            const juce::Point<float> from = centers_[static_cast<size_t>(src)];
            const juce::Point<float> to   = centers_[static_cast<size_t>(dst)];

            // Skip routes with sentinel (un-registered) centres.
            if (from.x < 0.0f || to.x < 0.0f)
            {
                rs.path.clear();
                continue;
            }

            rs.path    = buildBezierPath(from, to, rs.bowSign, rs.control);
        }
    }

    //==========================================================================
    /**
        Build a quadratic Bézier path from @p from to @p to, with the control
        point offset perpendicular to the chord by kControlBow * bowSign pixels.

        The control point is also written back through @p controlOut so that
        paint() can use the analytic formula for particle positioning without
        re-deriving it.

        @param from       Source point.
        @param to         Destination point.
        @param bowSign    +1.0f or -1.0f — which side of the chord to bow toward.
        @param controlOut Receives the computed control point.
        @return           The quadratic Bézier path.
    */
    static juce::Path buildBezierPath(juce::Point<float>  from,
                                      juce::Point<float>  to,
                                      float               bowSign,
                                      juce::Point<float>& controlOut)
    {
        // Midpoint of the chord.
        const float mx = (from.x + to.x) * 0.5f;
        const float my = (from.y + to.y) * 0.5f;

        // Perpendicular direction (rotate chord vector 90°, then normalise).
        const float dx = to.x - from.x;
        const float dy = to.y - from.y;
        const float len = std::sqrt(dx * dx + dy * dy);

        juce::Point<float> control;
        if (len < 1.0f)
        {
            // Degenerate case: source and dest are the same point.
            control = { mx, my };
        }
        else
        {
            // Perpendicular unit vector: (-dy/len, dx/len)
            const float perpX = -dy / len;
            const float perpY =  dx / len;
            // Bow is proportional to chord length, clamped to [20, 80] px so
            // short connections get a visible arc and long ones don't over-curve.
            const float bow = std::clamp(len * kControlBowFactor, 20.0f, 80.0f);
            control = { mx + perpX * bow * bowSign,
                        my + perpY * bow * bowSign };
        }

        controlOut = control;

        juce::Path path;
        path.startNewSubPath(from);
        path.quadraticTo(control, to);
        return path;
    }

    //==========================================================================
    /**
        Map a raw CouplingType integer to its canonical display colour.
        Falls back to a neutral grey for out-of-range values.
    */
    static juce::Colour colourForType(int typeInt)
    {
        // The CouplingType enum has 15 values (0 – 14).
        // Cast is safe for in-range values; out-of-range falls to the default
        // branch of the switch inside CouplingTypeColors::forType().
        return CouplingTypeColors::forType(static_cast<CouplingType>(typeInt));
    }

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CouplingSubstrate)
};

} // namespace xoceanus
