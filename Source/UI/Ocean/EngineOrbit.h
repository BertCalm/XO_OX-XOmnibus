// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// EngineOrbit.h — Single engine creature in the radial Ocean View.
//
// Represents one engine slot (0-4) as a living creature that orbits the
// centre nexus at a position determined by its depth zone and slot index.
//
// Spec §2.3 — Positioning:
//   angle  = slot_index * (2π / num_loaded_engines) + rotation_offset
//   radius = depth_zone_radius * min(width, height) / 2
//     Sunlit   → 30 %  of half-min-dimension
//     Twilight → 45 %
//     Midnight → 60 %
//
// Spec §2.3 — Creature:
//   64-80 px diameter circle containing a procedural creature sprite.
//   Accent-coloured border ring (2 px, 40 % alpha).
//   Engine name below in Overbit 12 px.
//   Breath animation: scale oscillation at voice-activity rate.
//   Coupling lean: directional tilt from active coupling routes.
//
// Spec §4 — Two-Level Dive:
//   Level 1 (single click)   — ZoomIn:        scale → 120 px, moves to centre.
//   Level 2 (double-click)   — SplitTransform: collapses to left strip.
//   Other engines → Minimized (32 px) while one is in ZoomIn / SplitTransform.
//
// Callbacks:
//   onClicked       — notifies parent (single click, slotIndex)
//   onDoubleClicked — notifies parent (double click, slotIndex)
//
// Animation is driven by a 30 Hz Timer.  Breath and scale-lerp are frozen
// when A11y::prefersReducedMotion() returns true.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../Gallery/CreatureRenderer.h"
#include "../EngineVocabulary.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

namespace xoceanus
{

//==============================================================================
/**
    EngineOrbit

    A JUCE Component + 30 Hz Timer that renders one engine creature slot in
    the radial Ocean View.  The parent OceanView is responsible for computing
    and setting the target screen bounds via setTargetBounds(); EngineOrbit
    applies those bounds directly (the parent drives animation of position;
    this component animates only scale, breath, and glow).
*/
class EngineOrbit : public juce::Component,
                    public juce::SettableTooltipClient,  // #1008 FIX 4: setTooltip() for minimized creatures
                    private juce::Timer
{
public:
    //==========================================================================
    // Depth zone — mirrors OceanBackground radius constants.
    enum class DepthZone { Sunlit, Twilight, Midnight };

    // Interaction state — controlled by the parent OceanView.
    enum class InteractionState { Orbital, ZoomIn, SplitTransform, Minimized };

    //==========================================================================
    // Size constants (public so OceanView can reference them directly and avoid
    // mirrored copies that can drift out of sync).
    static constexpr float kOrbitalSize    = 72.0f;    ///< Normal orbital diameter
    static constexpr float kZoomInSize     = 120.0f;   ///< Zoomed-in diameter
    static constexpr float kMinimizedSize  = 32.0f;    ///< Minimized diameter

    //==========================================================================
    EngineOrbit()
    {
        // Accessibility: announces as an interactive creature slot.
        A11y::setup(*this, "Engine Creature", "Engine orbit slot. Click to zoom in.");
        setWantsKeyboardFocus(true);

        // MEDIUM fix (#1006): do NOT start the 30 Hz timer here.
        // The timer is started in setEngine() and stopped in clearEngine() so
        // empty slots never wake the message thread unnecessarily.
    }

    ~EngineOrbit() override
    {
        stopTimer();
    }

    //==========================================================================
    // juce::Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
        if (!hasEngine())
            return;

        const auto localBounds = getLocalBounds().toFloat();
        // Proximity magnetism: shift the draw origin toward the cursor.
        const float cx = localBounds.getCentreX() + magnetOffset_.x;
        const float cy = localBounds.getCentreY() + magnetOffset_.y;

        // ── Effective size ───────────────────────────────────────────────────
        // currentScale_ is the smoothed fractional scale (1.0 = kOrbitalSize).
        // Breath adds ± kBreathAmplitude on top of that.
        const float breathAmplitude = A11y::prefersReducedMotion()
                                          ? 0.0f
                                          : kBreathAmplitude * std::sin(breathPhase_);
        const float effectiveScale = currentScale_ * (1.0f + breathAmplitude);
        const float creatureSize   = kOrbitalSize * effectiveScale;
        const float halfSize       = creatureSize * 0.5f;

        // ── Per-creature heartbeat glow (Vangelis) ────────────────────────────
        // When PlaySurface is visible and this engine has active voices,
        // add a pulsing brightness boost proportional to voice activity.
        const float heartbeatGlow = (playSurfaceVisible_ && voiceCount_ > 0)
            ? 0.15f + 0.10f * std::sin(breathPhase_ * 2.0f)  // double-speed pulse
            : 0.0f;

        const juce::Rectangle<float> creatureBounds(cx - halfSize, cy - halfSize,
                                                    creatureSize, creatureSize);

        // ── ZoomIn glow ring (drawn first — behind the creature) ─────────────
        if (interactionState_ == InteractionState::ZoomIn)
        {
            const float glowRadius = halfSize + 8.0f;
            g.setColour(accentColour_.withAlpha(0.10f));
            g.fillEllipse(cx - glowRadius, cy - glowRadius,
                          glowRadius * 2.0f, glowRadius * 2.0f);
        }

        // ── Heartbeat outer glow ──────────────────────────────────────────────
        if (heartbeatGlow > 0.0f)
        {
            const float glowR = halfSize + 6.0f;
            g.setColour(accentColour_.withAlpha(heartbeatGlow * 0.5f));
            g.fillEllipse(cx - glowR, cy - glowR, glowR * 2.0f, glowR * 2.0f);
        }

        // ── Proximity shimmer (Buchla) ───────────────────────────────────────
        if (isShimmering_)
        {
            const float shimmerAlpha = 0.12f + 0.08f * std::sin(shimmerPhase_);
            const float shimmerR = halfSize + 10.0f;
            g.setColour(accentColour_.withAlpha(shimmerAlpha));
            g.fillEllipse(cx - shimmerR, cy - shimmerR, shimmerR * 2.0f, shimmerR * 2.0f);
        }

        // ── Accent border ring ───────────────────────────────────────────────
        // Drawn behind the sprite as a filled circle, then the sprite overdraw.
        // Ring = accent at 40 % alpha (+ heartbeat boost), kBorderWidth px wide.
        {
            const float ringRadius = halfSize;
            g.setColour(accentColour_.withAlpha(0.40f + heartbeatGlow));
            g.drawEllipse(cx - ringRadius, cy - ringRadius,
                          ringRadius * 2.0f, ringRadius * 2.0f,
                          kBorderWidth);
        }

        // ── Creature sprite ──────────────────────────────────────────────────
        // CreatureRenderer uses the engine ID to select the archetype.
        // breathScale = 1.0 + breathAmplitude (scaled into the same ± range).
        // couplingLean is passed through directly.
        CreatureRenderer::drawCreature(g, creatureBounds, engineId_,
                                       accentColour_,
                                       1.0f + breathAmplitude,
                                       couplingLean_);

        // ── Engine name label ────────────────────────────────────────────────
        // Orbital state: full name below the creature.
        // Minimized state (#1008 FIX 4): draw first 2 chars at 7px INSIDE the
        //   circle so the creature is identifiable at 32px.  The tooltip (set in
        //   setEngine()) provides the full name on hover.
        // ZoomIn / SplitTransform: name shown in detail panel, omit here.
        if (interactionState_ == InteractionState::Orbital)
        {
            const float labelY    = cy + halfSize + 3.0f;
            const float labelH    = kNameFontSize + 4.0f;
            const juce::Rectangle<float> labelBounds(0.0f, labelY,
                                                     localBounds.getWidth(), labelH);

            g.setFont(GalleryFonts::engineName(kNameFontSize));
            g.setColour(GalleryColors::get(GalleryColors::Ocean::foam));
            // MEDIUM fix: ellipsis=true so long engine names get "..." rather
            // than being silently clipped (#1006).
            g.drawText(engineId_, labelBounds.toNearestInt(),
                       juce::Justification::centredTop, true);
        }
        else if (interactionState_ == InteractionState::Minimized)
        {
            // #1008 FIX 4: 2-char initial inside the 32px circle so the creature
            // is identifiable even without hovering.
            const juce::String initials = engineId_.substring(0, 2).toUpperCase();
            g.setFont(GalleryFonts::label(kMinimizedLabelSize));
            g.setColour(accentColour_.withAlpha(0.85f));
            const juce::Rectangle<float> initBounds(cx - halfSize, cy - halfSize,
                                                     creatureSize, creatureSize);
            g.drawText(initials, initBounds.toNearestInt(),
                       juce::Justification::centred, false);
        }

        // ── Accessibility focus ring ─────────────────────────────────────────
        if (hasKeyboardFocus(false))
        {
            A11y::drawCircularFocusRing(g, cx, cy, halfSize + kBorderWidth + 2.0f);
        }
    }

    void resized() override
    {
        // No child components to lay out; bounds are set by the parent OceanView.
    }

    void mouseDown(const juce::MouseEvent& /*e*/) override
    {
        if (onClicked)
            onClicked(slotIndex_);
    }

    void mouseDoubleClick(const juce::MouseEvent& /*e*/) override
    {
        if (onDoubleClicked)
            onDoubleClicked(slotIndex_);
    }

    //==========================================================================
    // Engine data
    //==========================================================================

    /**
        Assign an engine to this slot.

        @param engineId   Canonical engine ID (e.g. "Obrix", "Oxytocin").
        @param accent     Engine accent colour.
        @param zone       Depth zone determining orbital radius.
    */
    void setEngine(const juce::String& engineId,
                   juce::Colour        accent,
                   DepthZone           zone)
    {
        engineId_    = engineId;
        accentColour_ = accent;
        depthZone_   = zone;
        hasEngine_   = true;

        // Announce the new engine name to accessibility clients.
        setTitle("Engine: " + engineId);
        setDescription("Depth zone: " + depthZoneName(zone) + ". Click to zoom in.");

        // #1008 FIX 4: Always set a tooltip so hovering over a minimized (32px)
        // creature shows the engine name.  The tooltip is visible in all states,
        // but it is most critical when the label text is suppressed (Minimized).
        setTooltip(engineId);

        // MEDIUM fix (#1006): start the animation timer only when an engine is
        // loaded — avoids 30 Hz wakeups on empty slots.
        if (!isTimerRunning())
            startTimerHz(30);

        repaint();
    }

    /** Remove the engine from this slot. The component renders nothing when empty. */
    void clearEngine()
    {
        hasEngine_ = false;
        engineId_  = {};
        setTitle("Empty engine slot");
        setDescription({});

        // MEDIUM fix (#1006): stop the timer when the slot is emptied —
        // no animation needed for a component that renders nothing.
        stopTimer();

        repaint();
    }

    bool          hasEngine()       const noexcept { return hasEngine_; }
    juce::String  getEngineId()     const noexcept { return engineId_; }
    juce::Colour  getAccentColour() const noexcept { return accentColour_; }
    DepthZone     getDepthZone()    const noexcept { return depthZone_; }

    //==========================================================================
    // Live state updates — called from the parent's timer or audio thread proxy
    //==========================================================================

    /**
        Update the current voice count.  Drives the breath rate:
        0 voices → 0.2 Hz idle;  >0 voices → scales toward 0.5 Hz.
    */
    void setVoiceCount(int count)
    {
        voiceCount_ = count;
    }

    /**
        Set the coupling lean direction.

        @param lean  -1.0 (full left lean) → 0.0 (neutral) → +1.0 (full right lean).
                     Passed directly to CreatureRenderer for directional tilt.
    */
    void setCouplingLean(float lean)
    {
        couplingLean_ = juce::jlimit(-1.0f, 1.0f, lean);
    }

    /** Set the slot index (0-4). Used by the onClicked / onDoubleClicked callbacks. */
    void setSlotIndex(int index)
    {
        slotIndex_ = index;
    }

    /**
        Notify the creature whether the PlaySurface is currently visible.

        When true and voiceCount_ > 0, the creature displays a pulsing
        brightness boost (heartbeat glow) on top of the normal breath animation.
        Call this whenever the PlaySurface panel shows or hides.
    */
    void setPlaySurfaceVisible(bool visible)
    {
        playSurfaceVisible_ = visible;
    }

    //==========================================================================
    // Orbital positioning
    //==========================================================================

    /**
        Apply new target bounds immediately.

        The parent OceanView computes the orbital position from polar coordinates
        and calls this each time the layout changes.  Positional animation is the
        parent's responsibility (it may use ComponentAnimator or direct setBounds
        at fixed intervals); EngineOrbit's own Timer handles only scale and breath.
    */
    void setTargetBounds(juce::Rectangle<int> targetBounds)
    {
        setBounds(targetBounds);
    }

    /**
        Called by OceanView::mouseMove when the cursor moves within the Orbital
        view.  Computes a target drift offset so the creature drifts up to
        kMagnetStrength px toward the cursor when it is within kMagnetRange px.
        Passing a point far off-screen (e.g. {-1000, -1000}) forces an immediate
        reset to zero — used from OceanView::mouseExit.

        Has no effect when hasEngine_ is false or prefersReducedMotion() is true.
    */
    void setMouseProximity(juce::Point<float> mousePos)
    {
        if (!hasEngine_ || A11y::prefersReducedMotion())
        {
            targetMagnetOffset_ = { 0.0f, 0.0f };
            return;
        }

        auto centre = getBounds().getCentre().toFloat();
        auto delta  = mousePos - centre;
        float dist  = delta.getDistanceFromOrigin();

        if (dist < kMagnetRange && dist > 1.0f)
        {
            float strength = (1.0f - dist / kMagnetRange) * kMagnetStrength;
            // juce::Point<float> has no normalised() — divide by distance manually.
            targetMagnetOffset_ = (delta / dist) * strength;
        }
        else
        {
            targetMagnetOffset_ = { 0.0f, 0.0f };
        }
    }

    //==========================================================================
    // Interaction state management
    //==========================================================================

    /**
        Transition to a new interaction state.

        Updates targetScale_ so that timerCallback() smoothly lerps currentScale_
        toward the new size.  Triggers a repaint.
    */
    void setInteractionState(InteractionState state)
    {
        if (interactionState_ == state)
            return;

        interactionState_ = state;

        switch (state)
        {
            case InteractionState::Orbital:
                targetScale_ = 1.0f;
                break;

            case InteractionState::ZoomIn:
                targetScale_ = kZoomInSize / kOrbitalSize;
                break;

            case InteractionState::Minimized:
                targetScale_ = kMinimizedSize / kOrbitalSize;
                break;

            case InteractionState::SplitTransform:
                targetScale_ = kMinimizedSize / kOrbitalSize;
                break;
        }

        repaint();
    }

    InteractionState getInteractionState() const noexcept { return interactionState_; }

    //==========================================================================
    // Geometry helpers
    //==========================================================================

    /** Returns the component centre in parent coordinates (for coupling threads). */
    juce::Point<float> getCenter() const
    {
        return getBounds().toFloat().getCentre();
    }

    //==========================================================================
    // Callbacks
    //==========================================================================

    /** Fired on single click. Argument is the slot index (0-4). */
    std::function<void(int slotIndex)> onClicked;

    /** Fired on double-click. Argument is the slot index (0-4). */
    std::function<void(int slotIndex)> onDoubleClicked;

private:
    //==========================================================================
    // juce::Timer override — 30 Hz animation tick
    //==========================================================================

    void timerCallback() override
    {
        if (!hasEngine_)
            return;

        const bool reducedMotion = A11y::prefersReducedMotion();

        // ── Breath phase advance ─────────────────────────────────────────────
        if (!reducedMotion)
        {
            // Target breath rate: 0.2 Hz idle, ramps linearly to 0.5 Hz when
            // voices are active.  The ramp is bounded at 4 voices to avoid
            // jitter from high polyphony.
            const float voiceRamp  = juce::jlimit(0.0f, 1.0f,
                                                   static_cast<float>(voiceCount_) / 4.0f);
            breathRate_  = 0.2f + voiceRamp * (0.5f - 0.2f);

            // Advance phase by one tick (30 Hz).
            // Phase wraps at 2π to keep sin() argument in a clean range.
            breathPhase_ += breathRate_ / 30.0f * juce::MathConstants<float>::twoPi;
            if (breathPhase_ >= juce::MathConstants<float>::twoPi)
                breathPhase_ -= juce::MathConstants<float>::twoPi;
        }
        else
        {
            // Freeze breath at sin(0) = 0 (no scale oscillation).
            breathPhase_ = 0.0f;
        }

        // ── Scale smooth-lerp toward targetScale_ ────────────────────────────
        // 15 % per tick at 30 Hz ≈ 4-5 tick ramp to within 1 % of target.
        // Bypass lerp under reduced motion — jump straight to the target.
        if (reducedMotion)
        {
            currentScale_ = targetScale_;
        }
        else
        {
            constexpr float kLerpAlpha = 0.15f;
            currentScale_ += (targetScale_ - currentScale_) * kLerpAlpha;

            // Clamp rounding error: snap when within 0.1 % of target.
            if (std::abs(currentScale_ - targetScale_) < 0.001f)
                currentScale_ = targetScale_;
        }

        // ── Proximity magnetism smooth-lerp ──────────────────────────────────
        // Bypass under reduced motion — jump straight to target (which is always
        // zero when prefersReducedMotion() is true, set in setMouseProximity).
        if (reducedMotion)
        {
            magnetOffset_ = targetMagnetOffset_;
        }
        else
        {
            magnetOffset_ = magnetOffset_
                            + (targetMagnetOffset_ - magnetOffset_) * kMagnetLerp;

            // Snap to zero when essentially at rest to avoid endless tiny repaints.
            if (magnetOffset_.getDistanceFromOrigin() < 0.01f
                && targetMagnetOffset_.getDistanceFromOrigin() < 0.01f)
            {
                magnetOffset_ = { 0.0f, 0.0f };
            }
        }

        // ── Proximity shimmer: dwell detection ────────────────────────────────
        if (targetMagnetOffset_.getDistanceFromOrigin() > 0.1f)
        {
            ++dwellTicks_;
            if (dwellTicks_ >= 60)  // 2 seconds at 30 Hz
                isShimmering_ = true;
        }
        else
        {
            dwellTicks_ = 0;
            isShimmering_ = false;
        }

        // Shimmer animation phase advance
        if (isShimmering_)
        {
            shimmerPhase_ += juce::MathConstants<float>::twoPi / (1.5f * 30.0f);  // 1.5s period
            if (shimmerPhase_ >= juce::MathConstants<float>::twoPi)
                shimmerPhase_ -= juce::MathConstants<float>::twoPi;
        }
        else
        {
            shimmerPhase_ = 0.0f;
        }

        repaint();
    }

    //==========================================================================
    // Helpers
    //==========================================================================

    static juce::String depthZoneName(DepthZone zone) noexcept
    {
        switch (zone)
        {
            case DepthZone::Sunlit:   return "Sunlit";
            case DepthZone::Twilight: return "Twilight";
            case DepthZone::Midnight: return "Midnight";
        }
        return {};
    }

    //==========================================================================
    // Engine state
    //==========================================================================

    juce::String engineId_;
    juce::Colour accentColour_  = juce::Colour(GalleryColors::xoGold);
    DepthZone    depthZone_     = DepthZone::Sunlit;
    bool         hasEngine_     = false;

    // Interaction / layout
    InteractionState interactionState_ = InteractionState::Orbital;
    int              slotIndex_        = 0;

    // Live audio state
    int   voiceCount_        = 0;
    float couplingLean_      = 0.0f;   // -1..+1
    bool  playSurfaceVisible_ = false;  // Heartbeat glow active when true + voiceCount_ > 0

    // Animation state — owned entirely by the Timer thread (message thread only)
    float breathPhase_   = 0.0f;        ///< Current radians in the breath sine
    float breathRate_    = 0.2f;        ///< Hz — 0.2 idle, up to 0.5 with voices
    float currentScale_  = 1.0f;        ///< Smoothed fraction of kOrbitalSize
    float targetScale_   = 1.0f;        ///< Desired fraction (set by setInteractionState)

    // Proximity magnetism — creature drifts toward nearby cursor
    juce::Point<float> magnetOffset_       = { 0.0f, 0.0f };   ///< Smoothed draw offset (px)
    juce::Point<float> targetMagnetOffset_ = { 0.0f, 0.0f };   ///< Desired offset (px)

    // Proximity shimmer: cursor dwell detection
    int   dwellTicks_    = 0;       ///< Ticks cursor has been within magnet range
    bool  isShimmering_  = false;   ///< True after 2s dwell
    float shimmerPhase_  = 0.0f;   ///< Phase for shimmer pulse animation

    //==========================================================================
    // Private size constants (non-public ones; kOrbitalSize/kZoomInSize/kMinimizedSize
    // are declared in the public section above so OceanView can reference them).
    //==========================================================================

    static constexpr float kBorderWidth         = 2.0f;   ///< Accent ring stroke width
    static constexpr float kBreathAmplitude    = 0.05f;  ///< ±5 % scale oscillation
    static constexpr float kNameFontSize       = 12.0f;  ///< Overbit label below creature
    static constexpr float kMinimizedLabelSize = 7.0f;   ///< #1008: 2-char initial inside 32px circle

    // Proximity magnetism constants
    static constexpr float kMagnetRange    = 60.0f;   ///< Cursor proximity radius (px)
    static constexpr float kMagnetStrength = 8.0f;    ///< Max drift distance (px)
    static constexpr float kMagnetLerp     = 0.08f;   ///< Smoothing factor per 30 Hz tick

    //==========================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineOrbit)
};

} // namespace xoceanus
