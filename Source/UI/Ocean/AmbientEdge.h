// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// AmbientEdge.h — Ambient depth-zone glow along the plugin window border.
//
// Renders a soft inward gradient on each edge of the plugin window that
// reflects which depth zones (Sunlit / Twilight / Midnight) have engines
// loaded. The glow is purely decorative and transparent to mouse events.
//
// Visual rules:
//   Sunlit engines loaded   → top edge glows Cyan  (#48CAE4)
//   Midnight engines loaded → bottom edge glows Violet (#7B2FBE)
//   Both                    → top edge cyan, bottom violet; left/right blend
//   Twilight only           → both edges use Twilight blue (#0096C7)
//   No engines              → faint XO Gold (#E9C46A) at kIdleAlpha (5%)
//
// Animation:
//   When enginesActive_ is true, the glow alpha pulses gently ±kPulseDepth
//   at kPulseRate Hz (0.3 Hz). Frozen at the midpoint when
//   A11y::prefersReducedMotion() is true.
//
// Usage:
//   auto edge = std::make_unique<AmbientEdge>();
//   edge->setBounds(editor.getLocalBounds());
//   editor.addAndMakeVisible(*edge);
//   edge->setInterceptsMouseClicks(false, false); // pass-through
//   // Keep bounds in sync with editor:
//   edge->setBounds(getLocalBounds()); // inside editor::resized()
//
//   // Update when engine slots change:
//   edge->setActiveZones(sunlitLoaded, twilightLoaded, midnightLoaded);
//   edge->setEnginesActive(anyEngineLoaded);

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

namespace xoceanus
{

//==============================================================================
class AmbientEdge : public juce::Component, private juce::Timer
{
public:
    //==========================================================================
    // The three XOceanus depth zones, matching the water-column metaphor.
    enum class DepthZone { Sunlit, Twilight, Midnight };

    //==========================================================================
    AmbientEdge()
    {
        // Pass all mouse events through to components beneath this overlay.
        setInterceptsMouseClicks(false, false);
        setPaintingIsUnclipped(true);

        A11y::setup(*this,
                    "Ambient depth glow",
                    "Decorative edge glow indicating which depth zones have engines loaded.",
                    /*wantsKeyFocus=*/ false);
    }

    ~AmbientEdge() override { stopTimer(); }

    //==========================================================================
    // paint — draw one inward-fading gradient strip per edge.
    //
    // Edge assignment:
    //   Top    — shallowest active zone (Sunlit > Twilight > Midnight)
    //   Bottom — deepest active zone   (Midnight > Twilight > Sunlit)
    //   Left   — interpolated between top and bottom colours
    //   Right  — same as left
    //
    // Each strip is a ColourGradient from glowColour (at the edge) to
    // transparent (kGlowDepth px inward). Alpha = kMaxAlpha ± pulseOffset.
    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Compute current animated alpha offset.
        // When reduced motion is preferred or no engines are active, hold phase
        // at 0 so the glow is pinned to exactly kMaxAlpha (engines) / kIdleAlpha
        // (idle) without oscillating.
        const float pulseMod = (enginesActive_ && !A11y::prefersReducedMotion())
                                   ? kPulseDepth * std::sin(pulsePhase_)
                                   : 0.0f;

        if (!sunlit_ && !twilight_ && !midnight_)
        {
            // No depth zones active — use faint XO Gold at kIdleAlpha.
            const juce::Colour idleColour = GalleryColors::get(GalleryColors::xoGold);
            paintAllEdges(g, bounds, idleColour, idleColour, kIdleAlpha, kIdleAlpha);
        }
        else
        {
            const float topAlpha    = kMaxAlpha + pulseMod;
            const float bottomAlpha = kMaxAlpha + pulseMod;

            const juce::Colour topColour    = computeTopColour();
            const juce::Colour bottomColour = computeBottomColour();

            paintAllEdges(g, bounds, topColour, bottomColour, topAlpha, bottomAlpha);
        }
    }

    //==========================================================================
    // setActiveZones — update which depth zones have engines loaded.
    // Triggers an immediate repaint. Does not affect the pulse timer; call
    // setEnginesActive() separately to start/stop animation.
    void setActiveZones(bool sunlit, bool twilight, bool midnight)
    {
        if (sunlit_ == sunlit && twilight_ == twilight && midnight_ == midnight)
            return;

        sunlit_   = sunlit;
        twilight_ = twilight;
        midnight_ = midnight;
        repaint();
    }

    //==========================================================================
    // setEnginesActive — enable/disable the pulse animation.
    // Pass true when at least one engine slot is occupied; false otherwise.
    // Automatically starts or stops the 30 Hz timer, respecting
    // A11y::prefersReducedMotion() (frozen when reduced motion is on).
    void setEnginesActive(bool active)
    {
        if (enginesActive_ == active)
            return;

        enginesActive_ = active;

        if (active && !A11y::prefersReducedMotion())
        {
            startTimerHz(kTimerHz);
        }
        else
        {
            stopTimer();
            pulsePhase_ = 0.0f; // Reset phase so glow lands at peak on next paint.
            repaint();
        }
    }

private:
    //==========================================================================
    // timerCallback — advance pulse phase and request a repaint.
    void timerCallback() override
    {
        // Phase step = (Hz / timerHz) * 2π so one full sine cycle = 1/kPulseRate seconds.
        constexpr float kTwoPi    = static_cast<float>(2.0 * M_PI);
        constexpr float kPhaseStep = kPulseRate / static_cast<float>(kTimerHz) * kTwoPi;

        pulsePhase_ += kPhaseStep;
        if (pulsePhase_ >= kTwoPi)
            pulsePhase_ -= kTwoPi; // Wrap to avoid floating-point drift over time.

        repaint();
    }

    //==========================================================================
    // computeTopColour — returns the glow colour for the top / shallow edge.
    // Priority: Sunlit (cyan) > Twilight (blue) > Midnight (violet).
    [[nodiscard]] juce::Colour computeTopColour() const noexcept
    {
        if (sunlit_)   return GalleryColors::get(GalleryColors::Ocean::edgeCyan);
        if (twilight_) return GalleryColors::get(GalleryColors::Ocean::twilightTint);
        // midnight only
        return GalleryColors::get(GalleryColors::Ocean::edgeViolet);
    }

    // computeBottomColour — returns the glow colour for the bottom / deep edge.
    // Priority: Midnight (violet) > Twilight (blue) > Sunlit (cyan).
    [[nodiscard]] juce::Colour computeBottomColour() const noexcept
    {
        if (midnight_) return GalleryColors::get(GalleryColors::Ocean::edgeViolet);
        if (twilight_) return GalleryColors::get(GalleryColors::Ocean::twilightTint);
        // sunlit only
        return GalleryColors::get(GalleryColors::Ocean::edgeCyan);
    }

    // computeSideColour — interpolates top/bottom at a given t ∈ [0,1].
    // Used for the left and right edges so they blend naturally across the
    // full height of the plugin window.
    [[nodiscard]] static juce::Colour computeSideColour(juce::Colour top, juce::Colour bottom, float t) noexcept
    {
        return top.interpolatedWith(bottom, t);
    }

    //==========================================================================
    // paintAllEdges — paint the four edge glow strips.
    //
    // Each strip is a linear gradient from glowColour.withAlpha(alpha) at the
    // physical edge to transparent at kGlowDepth pixels inward.
    //
    // topColour/bottomColour: the pure hue for each respective edge.
    // topAlpha/bottomAlpha:   the maximum alpha at that edge (includes pulse).
    // Left and right use the midpoint blend of top/bottom for their hue, and
    // the average of top/bottom alpha for their intensity.
    //
    // Alpha is clamped to [0, kMaxAlpha] to ensure we never exceed spec.
    void paintAllEdges(juce::Graphics&       g,
                       juce::Rectangle<float> bounds,
                       juce::Colour           topColour,
                       juce::Colour           bottomColour,
                       float                  topAlpha,
                       float                  bottomAlpha) const
    {
        topAlpha    = juce::jlimit(0.0f, kMaxAlpha, topAlpha);
        bottomAlpha = juce::jlimit(0.0f, kMaxAlpha, bottomAlpha);

        const float sideAlpha = (topAlpha + bottomAlpha) * 0.5f;
        const juce::Colour sideColour = computeSideColour(topColour, bottomColour, 0.5f);

        const float w     = bounds.getWidth();
        const float h     = bounds.getHeight();

        // Scale glow depth to 2.5% of the smallest window dimension so it
        // remains proportional across different plugin window sizes, with a
        // minimum of kGlowDepth (20px) to stay visible on small windows.
        const float depth = std::max(kGlowDepth,
                                     std::min(w, h) * 0.025f);
        const float x0    = bounds.getX();
        const float y0    = bounds.getY();

        // ── Top edge ─────────────────────────────────────────────────────────
        {
            const juce::Rectangle<float> strip(x0, y0, w, depth);
            juce::ColourGradient grad(topColour.withAlpha(topAlpha), x0, y0,
                                     topColour.withAlpha(0.0f),      x0, y0 + depth,
                                     false);
            g.setGradientFill(grad);
            g.fillRect(strip);
        }

        // ── Bottom edge ───────────────────────────────────────────────────────
        {
            const float stripY = y0 + h - depth;
            const juce::Rectangle<float> strip(x0, stripY, w, depth);
            juce::ColourGradient grad(bottomColour.withAlpha(bottomAlpha), x0, y0 + h,
                                     bottomColour.withAlpha(0.0f),         x0, stripY,
                                     false);
            g.setGradientFill(grad);
            g.fillRect(strip);
        }

        // ── Left edge ────────────────────────────────────────────────────────
        // We use a vertical gradient so the left strip shades from the top
        // colour at the top corner to the bottom colour at the bottom corner,
        // mimicking how the horizontal edges transition. The horizontal
        // gradient component fades the glow inward from the left wall.
        //
        // Implementation: paint two rectangles — one for the left strip drawn
        // with a plain flat colour at sideAlpha, then blend in a horizontal
        // fade-to-transparent gradient. Because JUCE ColourGradient supports
        // only one gradient per fill, we layer a second pass using Porter-Duff
        // SrcOver compositing (the default) with a left→right gradient from
        // solid-black to transparent-black using multiply-through-alpha.
        //
        // Simpler and visually equivalent: paint the strip using a horizontal
        // gradient from sideColour@sideAlpha → transparent, which is the
        // same approach used by every other edge. A per-pixel vertical colour
        // shift would require either a shader or a large off-screen image and
        // is not worth the complexity for a 12%-alpha decorative element.
        {
            const juce::Rectangle<float> strip(x0, y0, depth, h);
            juce::ColourGradient grad(sideColour.withAlpha(sideAlpha), x0,          y0,
                                     sideColour.withAlpha(0.0f),       x0 + depth,  y0,
                                     false);
            g.setGradientFill(grad);
            g.fillRect(strip);
        }

        // ── Right edge ────────────────────────────────────────────────────────
        {
            const float stripX = x0 + w - depth;
            const juce::Rectangle<float> strip(stripX, y0, depth, h);
            juce::ColourGradient grad(sideColour.withAlpha(sideAlpha), x0 + w,       y0,
                                     sideColour.withAlpha(0.0f),       stripX,        y0,
                                     false);
            g.setGradientFill(grad);
            g.fillRect(strip);
        }
    }

    //==========================================================================
    // Member state
    bool  sunlit_       = false;  // Sunlit zone has at least one engine loaded
    bool  twilight_     = false;  // Twilight zone has at least one engine loaded
    bool  midnight_     = false;  // Midnight zone has at least one engine loaded
    bool  enginesActive_ = false; // Any engine loaded — drives pulse animation
    float pulsePhase_   = 0.0f;  // Current position in the sine pulse [0, 2π)

    //==========================================================================
    // Render / animation constants
    static constexpr float kGlowDepth  = 20.0f;  // Inward fade distance in pixels
    static constexpr float kMaxAlpha   = 0.12f;  // Peak glow alpha (active engines)
    static constexpr float kIdleAlpha  = 0.05f;  // Glow alpha when no engines loaded (XO Gold)
    static constexpr float kPulseRate  = 0.3f;   // Sine oscillation frequency in Hz
    static constexpr float kPulseDepth = 0.02f;  // ±alpha variation around kMaxAlpha
    static constexpr int   kTimerHz    = 30;      // Timer tick rate in Hz

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmbientEdge)
};

} // namespace xoceanus
