// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// PlaySurfaceOverlay.h — Frosted-glass slide-up panel wrapping the PlaySurface.
//
// The PlaySurface (keyboard / pads / drum / fretless) lives inside this overlay
// rather than at a fixed position in the window.  The overlay slides up from the
// bottom of the Ocean View with a 200ms ease-out animation and dims the content
// behind it.
//
// §6.1 Behavior:
//   – Slides up from bottom with 200ms ease-out animation (instant if reduced motion)
//   – Frosted glass panel: Ocean::deep at 92% opacity + rounded top corners (16px)
//   – Drag handle at top centre (40×4px pill, Ocean::salt at 40% alpha)
//   – Notifies parent via onDimStateChanged(bool) so the OceanView can dim to 35%
//
// §6.2 Content:
//   – PlaySurface is a child component (not recreated — it persists)
//   – Mode tabs and XOuija are rendered by PlaySurface internally
//
// §6.3 Auto-show / Persist:
//   – First launch: caller should call show() to start in KEYS mode
//   – Dismiss: drag down past 40% of height, click outside, press K or Escape
//   – MIDI auto-show: call onMidiNoteReceived() when a note arrives while hidden
//
// Positioning contract:
//   The parent OceanView must give this component its full-width bounds at the
//   bottom of the view (e.g. setBounds(0, 0, parentW, parentH)).  The overlay
//   manages its own Y offset by calling setTopLeftPosition() based on the current
//   slide animation value so it appears to slide up from below.
//
// Usage:
//   overlay_.onDimStateChanged = [this](bool dimmed) { setContentAlpha(dimmed ? 0.35f : 1.0f); };
//   addAndMakeVisible(overlay_);
//   overlay_.setBounds(getLocalBounds());
//   overlay_.show();  // auto-show on first launch
//   // Wire MIDI:
//   processor_.addMidiNoteListener([this] { overlay_.onMidiNoteReceived(); });

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../PlaySurface/PlaySurface.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <functional>

namespace xoceanus
{

//==============================================================================
/**
    PlaySurfaceOverlay

    A frosted-glass slide-up panel that wraps the PlaySurface component.
    Slides up from the bottom of the OceanView with a 200ms ease-out animation.
    Includes a drag handle for drag-to-dismiss gesture support.

    The parent OceanView is notified of dim-state changes via onDimStateChanged
    so it can alpha-fade its content to 35% while the overlay is visible.
*/
class PlaySurfaceOverlay : public juce::Component,
                           private juce::Timer
{
public:
    //==========================================================================
    // Public geometry constants — available to parent for layout calculations.
    static constexpr float kOverlayHeight   = 300.0f;  // Total panel height including handle
    static constexpr float kHandleWidth     = 40.0f;
    static constexpr float kHandleHeight    = 4.0f;
    static constexpr float kHandlePadding   = 10.0f;   // Space above + below handle pill
    static constexpr float kCornerRadius    = 16.0f;
    static constexpr float kBackgroundAlpha = 0.92f;
    static constexpr float kSlideAnimMs     = 200.0f;
    static constexpr float kDismissThreshold = 0.40f;  // Drag past 40% → dismiss

    //==========================================================================
    PlaySurfaceOverlay()
    {
        addAndMakeVisible(playSurface_);

        // Overlay is transparent (the panel background is drawn in paint()).
        setOpaque(false);

        // Start invisible below the viewport.
        setVisible(false);

        A11y::setup(*this,
                    "Play Surface Overlay",
                    "Slide-up keyboard and pad controller panel. Press Escape or K to dismiss.",
                    /*wantsKeyFocus=*/true);
    }

    ~PlaySurfaceOverlay() override
    {
        stopTimer();
    }

    //==========================================================================
    // juce::Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();

        // ── Frosted-glass panel background ──────────────────────────────────
        // Rounded top corners only.  We build a path with the top two corners
        // rounded and the bottom two corners square so the panel appears to
        // emerge from the bottom of the window.
        juce::Path panelPath;
        panelPath.addRoundedRectangle(b.getX(),
                                      b.getY(),
                                      b.getWidth(),
                                      b.getHeight(),
                                      kCornerRadius,
                                      kCornerRadius,
                                      /*topLeftIsRounded=*/   true,
                                      /*topRightIsRounded=*/  true,
                                      /*bottomLeftIsRounded=*/ false,
                                      /*bottomRightIsRounded=*/false);

        g.setColour(juce::Colour(GalleryColors::Ocean::deep).withAlpha(kBackgroundAlpha));
        g.fillPath(panelPath);

        // Subtle top-edge border — 1px Ocean::salt at 20% alpha — helps the
        // panel read as a distinct layer on top of the Ocean background.
        g.setColour(juce::Colour(GalleryColors::Ocean::salt).withAlpha(0.20f));
        g.strokePath(panelPath, juce::PathStrokeType(1.0f));

        // ── Drag handle ──────────────────────────────────────────────────────
        const auto handleBounds = getHandleBounds();
        g.setColour(juce::Colour(GalleryColors::Ocean::salt).withAlpha(0.40f));
        g.fillRoundedRectangle(handleBounds, kHandleHeight * 0.5f);
    }

    void resized() override
    {
        // The PlaySurface occupies the full overlay area below the drag handle.
        // Handle area = kHandlePadding + kHandleHeight + kHandlePadding.
        const float handleAreaH = kHandlePadding * 2.0f + kHandleHeight;
        const auto  bounds      = getLocalBounds();

        playSurface_.setBounds(bounds.withTrimmedTop(static_cast<int>(handleAreaH)));
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const auto handleBounds = getHandleBounds().expanded(kHandlePadding, kHandlePadding);
        if (handleBounds.contains(e.position))
        {
            dragging_     = true;
            dragStartY_   = e.position.y;
            // Freeze the timer while the user is dragging — the drag offset
            // controls the visual position directly.
            stopTimer();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!dragging_)
            return;

        // Convert vertical drag distance to a 0–1 slide offset.
        // Positive dragDelta = user dragged downward = panel moving toward hidden.
        const float dragDelta = e.position.y - dragStartY_;
        const float newOffset = juce::jlimit(0.0f, 1.0f,
                                             dragDelta / kOverlayHeight);
        currentSlideOffset_ = newOffset;
        repositionFromOffset();
        repaint();
    }

    void mouseUp(const juce::MouseEvent& /*e*/) override
    {
        if (!dragging_)
            return;

        dragging_ = false;

        if (currentSlideOffset_ >= kDismissThreshold)
        {
            // User dragged far enough — treat as a dismiss gesture.
            hide();
        }
        else
        {
            // Snap back to fully shown.
            targetSlideOffset_ = 0.0f;
            if (A11y::prefersReducedMotion())
            {
                currentSlideOffset_ = 0.0f;
                repositionFromOffset();
            }
            else
            {
                startTimerHz(60);
            }
        }
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey ||
            key == juce::KeyPress('k') ||
            key == juce::KeyPress('K'))
        {
            hide();
            return true;
        }
        return false;
    }

    //==========================================================================
    // Show / hide API
    //==========================================================================

    /** Slide the overlay up into view.  Safe to call when already showing. */
    void show()
    {
        showing_           = true;
        targetSlideOffset_ = 0.0f;
        setVisible(true);

        if (A11y::prefersReducedMotion())
        {
            currentSlideOffset_ = 0.0f;
            repositionFromOffset();
        }
        else
        {
            startTimerHz(60);
        }

        grabKeyboardFocus();

        if (onDimStateChanged)
            onDimStateChanged(true);
    }

    /** Slide the overlay down out of view.  Safe to call when already hidden. */
    void hide()
    {
        showing_           = false;
        targetSlideOffset_ = 1.0f;

        if (A11y::prefersReducedMotion())
        {
            currentSlideOffset_ = 1.0f;
            repositionFromOffset();
            setVisible(false);
            // Reduced-motion: instant dismiss — notify immediately.
            if (onDimStateChanged)
                onDimStateChanged(false);
        }
        else
        {
            startTimerHz(60);
            // setVisible(false) and onDimStateChanged(false) are both deferred
            // to timerCallback() so the parent un-dims only after the slide-out
            // animation finishes (avoids a jarring flash before the panel clears).
        }
    }

    /** Returns true if the overlay is currently shown (or animating toward shown). */
    bool isShowing() const noexcept { return showing_; }

    /** Toggle between shown and hidden states. */
    void toggle()
    {
        if (showing_)
            hide();
        else
            show();
    }

    /** Call when a MIDI note-on arrives.  Auto-shows the overlay if it is hidden. */
    void onMidiNoteReceived()
    {
        if (!showing_)
            show();
    }

    //==========================================================================
    // Child component accessor
    //==========================================================================

    /** Returns a reference to the contained PlaySurface for event wiring. */
    PlaySurface& getPlaySurface() noexcept { return playSurface_; }

    //==========================================================================
    // Callback — fired whenever the overlay transitions between dim/undim state.
    // The parent OceanView wires this to alpha-fade its background content.
    //   true  = overlay is showing → parent should dim to ~35% alpha
    //   false = overlay is hiding  → parent should restore full alpha
    std::function<void(bool dimmed)> onDimStateChanged;

private:
    //==========================================================================
    // Private members
    //==========================================================================

    PlaySurface playSurface_;

    bool  showing_            = false;
    bool  dragging_           = false;
    float dragStartY_         = 0.0f;
    float currentSlideOffset_ = 1.0f;  // 0 = fully shown, 1 = fully hidden (below bounds)
    float targetSlideOffset_  = 1.0f;  // animation target — starts hidden

    //==========================================================================
    // juce::Timer — drives slide-up / slide-down animation at 60 Hz.
    //==========================================================================

    void timerCallback() override
    {
        // Lerp current offset toward target at ~15% per frame (≈ 60 fps).
        // This gives an exponential ease-out feel matching the spec.
        constexpr float kLerpRate = 0.15f;
        const float     delta     = targetSlideOffset_ - currentSlideOffset_;

        if (std::abs(delta) < 0.005f)
        {
            // Close enough — snap to target and stop animating.
            currentSlideOffset_ = targetSlideOffset_;
            stopTimer();

            // If we just finished hiding, make the component invisible so it
            // no longer participates in layout or hit-testing, then notify the
            // parent to restore full alpha.  Deferring here ensures the parent
            // un-dims only after the slide-out animation is visually complete.
            if (!showing_)
            {
                setVisible(false);
                if (onDimStateChanged)
                    onDimStateChanged(false);
            }
        }
        else
        {
            currentSlideOffset_ += delta * kLerpRate;
        }

        repositionFromOffset();
        repaint();
    }

    //==========================================================================
    // Helpers
    //==========================================================================

    /**
        Compute the bounding rect of the drag handle pill in local coordinates.
        The handle is centred horizontally at the top of the panel, separated
        from the top edge by kHandlePadding.
    */
    juce::Rectangle<float> getHandleBounds() const
    {
        const float cx = static_cast<float>(getWidth()) * 0.5f;
        const float pillX = cx - kHandleWidth * 0.5f;
        const float pillY = kHandlePadding;
        return { pillX, pillY, kHandleWidth, kHandleHeight };
    }

    /**
        Reposition this component so it appears to slide up from the bottom of
        its parent.  currentSlideOffset_ = 0 means fully visible (top of panel
        sits kOverlayHeight from the bottom of the parent), 1 means fully hidden
        (the entire panel is below the bottom edge of the parent).

        The parent must have assigned this component full-parent-width bounds
        starting at y=0 (i.e. the component covers the entire parent area so
        the child PlaySurface's absolute position is predictable).  We shift the
        component's top edge downward by currentSlideOffset_ * kOverlayHeight to
        create the slide effect.
    */
    void repositionFromOffset()
    {
        if (auto* parent = getParentComponent())
        {
            const int parentH = parent->getHeight();
            const int parentW = parent->getWidth();

            // Cap overlay height at 50% of parent so it doesn't swallow the Ocean
            // View on small windows or compact plugin hosts.
            const float overlayH = std::min(kOverlayHeight,
                                            static_cast<float>(parentH) * 0.50f);

            // Y position: when offset=0, top is at (parentH - overlayH).
            //             when offset=1, top is at parentH (fully off-screen below).
            const int newY = parentH
                             - static_cast<int>(overlayH)
                             + static_cast<int>(currentSlideOffset_ * overlayH);

            setBounds(0, newY, parentW, static_cast<int>(overlayH));
        }
    }

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaySurfaceOverlay)
};

} // namespace xoceanus
