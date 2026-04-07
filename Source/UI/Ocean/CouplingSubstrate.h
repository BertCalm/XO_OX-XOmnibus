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
//   - Coupling Evolution: threads thicken and shift toward XO Gold over 60 seconds
//     of sustained active coupling; removed routes fade over ~2 seconds instead
//     of disappearing instantly.
//
// Usage:
//   CouplingSubstrate substrate;
//   addAndMakeVisible(substrate);
//   substrate.setBounds(getLocalBounds());
//   substrate.setCreatureCenter(slot, center);
//   substrate.setSlotVoiceCount(slot, count);   // call each timer tick
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
    static constexpr int   kMaxSlots             = 5;
    static constexpr float kMinStroke            = 1.0f;    ///< px at amount = 0
    static constexpr float kMaxStroke            = 4.0f;    ///< px at amount = 1
    static constexpr float kGlowAlphaBase        = 0.6f;    ///< peak pulse alpha (core line)
    static constexpr float kGlowAlphaMin         = 0.3f;    ///< trough pulse alpha (core line)
    static constexpr float kPulsePeriod          = 3.0f;    ///< seconds per full sine cycle
    static constexpr int   kParticlesPerRoute    = 6;       ///< particles per route (RouteState uses 8 slots)
    static constexpr float kParticleSpeed        = 0.15f;   ///< fraction of path per second
    static constexpr float kParticleDiameter     = 4.0f;    ///< px
    static constexpr float kControlBowFactor     = 0.15f;   ///< bow = chordLength * factor, clamped [20,80]
    static constexpr int   kTimerHz              = 30;      ///< timer frequency

    // Coupling Evolution constants
    static constexpr float kAgeIncrement         = 1.0f / (60.0f * 30.0f); ///< reaches 1.0 after 60 s at 30 Hz
    static constexpr float kMaxAgeStrokeScale    = 2.5f;   ///< mature threads are 2.5× base stroke
    static constexpr float kMaxAgeGlowScale      = 2.0f;   ///< mature threads glow at 2× kGlowAlphaBase
    static constexpr float kFadeDecay            = 0.95f;  ///< fadeAlpha_ multiplier per tick (~2 s to fade)
    static constexpr float kFadeRemoveThreshold  = 0.01f;  ///< remove fading route below this alpha

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

        // ── Coupling Evolution fields ──────────────────────────────────────
        /** Normalised coupling age: 0.0 → 1.0 over 60 seconds of active coupling.
            Drives stroke growth, glow intensification, and colour shift to XO Gold. */
        float couplingAge_ = 0.0f;

        /** True when the route was active (both endpoints had voices) last tick.
            Used to detect the transition into fade-out. */
        bool wasActive_ = false;

        /** Fade-out alpha for removed routes.  Initialised to the current pulse
            alpha when the route is removed; decays by kFadeDecay each tick.
            Active routes keep this at 1.0f (unused). */
        float fadeAlpha_ = 1.0f;

        /** True when this route is in fade-out (removed from the live matrix
            but still being rendered until fadeAlpha_ drops below threshold). */
        bool isFading_ = false;
    };

    //==========================================================================
    std::array<juce::Point<float>, kMaxSlots> centers_;
    std::vector<RouteState>                   routeStates_;

    /** Per-slot voice counts — set by the editor timer via setSlotVoiceCount().
        Used to determine whether both endpoints of a route are active, which
        drives the Coupling Evolution age accumulation. */
    std::array<int, kMaxSlots> slotVoiceCounts_{};

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
        Report the current voice count for a single engine slot.

        Call this from the editor's periodic timer alongside setCouplingRoutes()
        so that Coupling Evolution can detect when both endpoints of a route are
        actively playing.  Voices > 0 on both src and dst of a route causes that
        route's couplingAge_ to increment each timer tick.

        @param slot   Engine slot index, 0 – (kMaxSlots-1).
        @param count  Number of active voices for this slot (0 = silent).
    */
    void setSlotVoiceCount(int slot, int count)
    {
        if (slot >= 0 && slot < kMaxSlots)
            slotVoiceCounts_[static_cast<size_t>(slot)] = count;
    }

    //==========================================================================
    /**
        Replace the full list of active coupling routes.

        Unlike a simple clear-and-rebuild, this method diffs the incoming routes
        against the current routeStates_ so that Coupling Evolution history
        (couplingAge_) is preserved for routes that remain active, and removed
        routes are transitioned into a fade-out state instead of deleted
        immediately.

        New routes are seeded with evenly-spaced pulse phases (sync-flash fix
        #1006) and evenly-spaced particle t values.

        Starts the 30 Hz timer whenever any routes are active or fading; stops
        it only when routeStates_ is completely empty.

        @param routes  New set of active routes.  The vector is copied.
    */
    void setRoutes(const std::vector<CouplingRoute>& routes)
    {
        // ── Step 1: mark currently-active (non-fading) routes for removal ──
        // We'll clear the flag for routes that survive into the new set.
        for (auto& rs : routeStates_)
        {
            if (!rs.isFading_)
                rs.isFading_ = true;   // tentatively mark for fade; cleared below if still present
        }

        // ── Step 2: process incoming routes — match or create ──────────────
        // Use a stable bowSign counter so alternation is consistent for new routes.
        // Existing routes keep their original bowSign.
        float bowSign = 1.0f;
        const int numNew = static_cast<int>(routes.size());
        int newIdx = 0;

        for (const auto& route : routes)
        {
            // Look for an existing active (or just-tentatively-marked) entry
            // with the same source/dest/type key.
            RouteState* existing = nullptr;
            for (auto& rs : routeStates_)
            {
                if (rs.route.sourceSlot == route.sourceSlot
                    && rs.route.destSlot   == route.destSlot
                    && rs.route.type       == route.type)
                {
                    existing = &rs;
                    break;
                }
            }

            if (existing != nullptr)
            {
                // Route survived — update amount, clear the fade-out flag.
                existing->route.amount = route.amount;
                existing->isFading_    = false;
                existing->fadeAlpha_   = 1.0f;
            }
            else
            {
                // Brand-new route — build a fresh RouteState.
                RouteState rs;
                rs.route = route;

                // MEDIUM fix (#1006): stagger pulse phases so routes do not
                // all peak simultaneously on load.
                rs.pulsePhase = (numNew > 1)
                    ? (static_cast<float>(newIdx) / static_cast<float>(numNew))
                        * juce::MathConstants<float>::twoPi
                    : 0.0f;

                // Alternate bow direction for visual separation of overlapping threads.
                rs.bowSign = bowSign;
                bowSign = -bowSign;

                // Seed particles with evenly-spaced t values.
                const float step = 1.0f / static_cast<float>(kParticlesPerRoute);
                for (int p = 0; p < kParticlesPerRoute; ++p)
                    rs.particles[static_cast<size_t>(p)].t = static_cast<float>(p) * step;

                rs.isFading_    = false;
                rs.fadeAlpha_   = 1.0f;
                rs.couplingAge_ = 0.0f;
                rs.wasActive_   = false;

                routeStates_.push_back(std::move(rs));
            }

            ++newIdx;
        }

        // ── Step 3: transition removed routes into fade-out ────────────────
        // Routes still marked isFading_ = true were not found in the new set.
        // Their fadeAlpha_ will decay in timerCallback(); leave them in the
        // vector for now.  (They were already set to isFading_ = true above.)

        rebuildPaths();

        // ── Step 4: timer lifecycle ────────────────────────────────────────
        // Keep the timer alive as long as any entry (active or fading) remains.
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
    /** Returns true if there is at least one active (non-fading) coupling route. */
    bool hasActiveRoutes() const
    {
        for (const auto& rs : routeStates_)
            if (!rs.isFading_)
                return true;
        return false;
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        if (routeStates_.empty())
            return;

        const juce::Colour xoGold = juce::Colour(GalleryColors::xoGold);

        for (const auto& rs : routeStates_)
        {
            const juce::Colour typeColour = colourForType(rs.route.type);

            // ── Coupling Evolution: age-modulated properties ───────────────
            // couplingAge_ ∈ [0, 1]:  0 = brand new,  1 = fully mature (60 s).
            const float age = rs.couplingAge_;

            // Colour: lerp from type colour toward XO Gold as the connection matures.
            const juce::Colour baseColour = typeColour.interpolatedWith(xoGold, age);

            // 1. Compute stroke width — amount maps to [kMinStroke, kMaxStroke],
            //    then the age scale grows it up to kMaxAgeStrokeScale × that base.
            //    Clamp amount to [0, 1] so malformed routes don't produce negative widths.
            const float amount      = juce::jlimit(0.0f, 1.0f, rs.route.amount);
            const float baseStroke  = kMinStroke + amount * (kMaxStroke - kMinStroke);
            const float ageScale    = 1.0f + age * (kMaxAgeStrokeScale - 1.0f);
            const float strokeWidth = baseStroke * ageScale;

            // 2. Compute pulse alpha — sinusoidal pulse in [kGlowAlphaMin, kGlowAlphaBase],
            //    then scaled up by age (glow intensifies for mature connections).
            //    For fading routes, multiply by fadeAlpha_ so they dissolve smoothly.
            const float agePeakAlpha  = kGlowAlphaBase
                + age * (kGlowAlphaBase * kMaxAgeGlowScale - kGlowAlphaBase);
            const float ageMinAlpha   = kGlowAlphaMin
                + age * (kGlowAlphaMin  * kMaxAgeGlowScale - kGlowAlphaMin);
            float alpha = ageMinAlpha
                + (agePeakAlpha - ageMinAlpha)
                    * (0.5f + 0.5f * std::sin(rs.pulsePhase));

            if (rs.isFading_)
                alpha *= rs.fadeAlpha_;

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
            //    Skip particles for fading routes to avoid visual noise during fade.
            if (!rs.isFading_)
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

                if (!rs.isFading_)
                {
                    // ── Coupling Evolution: age accumulation ───────────────
                    // Increment age only when both endpoints have active voices.
                    const int src = rs.route.sourceSlot;
                    const int dst = rs.route.destSlot;
                    const bool srcActive = (src >= 0 && src < kMaxSlots)
                                           && (slotVoiceCounts_[static_cast<size_t>(src)] > 0);
                    const bool dstActive = (dst >= 0 && dst < kMaxSlots)
                                           && (slotVoiceCounts_[static_cast<size_t>(dst)] > 0);

                    if (srcActive && dstActive)
                    {
                        rs.couplingAge_ = juce::jmin(1.0f, rs.couplingAge_ + kAgeIncrement);
                        rs.wasActive_ = true;
                    }
                    else
                    {
                        rs.wasActive_ = false;
                    }

                    // Advance particles for active routes.
                    for (int p = 0; p < kParticlesPerRoute; ++p)
                    {
                        auto& particle = rs.particles[static_cast<size_t>(p)];
                        particle.t += tStep;
                        if (particle.t >= 1.0f)
                            particle.t -= 1.0f;  // wrap in [0, 1)
                    }
                }
                else
                {
                    // ── Fade-out: decay alpha each tick ────────────────────
                    rs.fadeAlpha_ *= kFadeDecay;
                }
            }
        }
        // When reducedMotion is true: phases and particles are frozen (no advance).
        // Fade-out still runs so removed routes eventually disappear even under
        // reduced motion — we only freeze the decorative animation, not the
        // state cleanup.
        else
        {
            for (auto& rs : routeStates_)
            {
                if (rs.isFading_)
                    rs.fadeAlpha_ *= kFadeDecay;
            }
        }

        // ── Prune fully-faded routes ───────────────────────────────────────
        // Erase fading entries whose alpha has dropped below the removal threshold.
        routeStates_.erase(
            std::remove_if(routeStates_.begin(), routeStates_.end(),
                           [](const RouteState& rs)
                           {
                               return rs.isFading_ && rs.fadeAlpha_ < kFadeRemoveThreshold;
                           }),
            routeStates_.end());

        // Stop the timer if all routes have been removed and faded out.
        if (routeStates_.empty())
            stopTimer();

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
