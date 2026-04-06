// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// OceanBackground.h — Radial depth-gradient background for the Ocean View.
//
// Renders the water-column scene viewed from above: bright shallow centre
// fading through twilight to deep abyss at the edges.  Concentric faint rings
// mark depth-zone transitions (Sunlit / Twilight / Midnight).
//
// When no coupling routes are active an additional ring-texture pass is drawn
// so the canvas never looks blank.
//
// Performance: the gradient is cached as a juce::Image and only rebuilt when
// the component bounds change.  Ring overlays are drawn directly each frame
// (they are simple ellipse strokes — negligible cost).
//
// Usage:
//   OceanBackground bg;
//   addAndMakeVisible(bg);
//   bg.setBounds(getLocalBounds());
//   bg.setHasCouplingRoutes(matrix.getRouteCount() > 0);

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"  // GalleryColors::Ocean, A11y::prefersReducedMotion

namespace xoceanus
{

//==============================================================================
/**
    OceanBackground

    A header-only JUCE Component that paints a radial depth-gradient background
    representing the ocean water column viewed from above.

    Colour journey (centre → edge):
        Ocean::shallow   (#142040)  →  centre
        Ocean::twilight  (#0E1428)  →  30 % radius
        Ocean::deep      (#0A0E18)  →  60 % radius
        Ocean::abyss     (#04040A)  →  edge

    Depth-zone rings (faint ellipse strokes overlaid on the gradient):
        Sunlit zone    — 30 % of half-min-dimension, sunlitTint   @ 4 % alpha
        Twilight zone  — 45 % of half-min-dimension, twilightTint @ 3 % alpha
        Midnight zone  — 60 % of half-min-dimension, midnightTint @ 4 % alpha

    When hasCouplingRoutes_ is false the component additionally draws faint
    concentric ring outlines (1 px, 5 % white) as a background texture so the
    canvas reads as "ocean" even when empty.
*/
class OceanBackground : public juce::Component
{
public:
    //==========================================================================
    // Depth-zone radii — fraction of min(width, height) / 2
    static constexpr float kSunlitRadius   = 0.30f;
    static constexpr float kTwilightRadius = 0.45f;
    static constexpr float kMidnightRadius = 0.60f;

    //==========================================================================
    OceanBackground()
    {
        setOpaque(true);
        setInterceptsMouseClicks(false, false);

        A11y::setup(*this,
                    "Ocean Background",
                    "Radial depth-gradient water-column background",
                    /*wantsKeyFocus=*/false);
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds();
        if (bounds.isEmpty())
            return;

        // Rebuild the cached gradient image if the bounds changed since the
        // last paint call (or if this is the first paint call).
        if (needsRedraw_ || cachedGradient_.isNull()
            || cachedGradient_.getWidth()  != bounds.getWidth()
            || cachedGradient_.getHeight() != bounds.getHeight())
        {
            rebuildGradientImage();
            needsRedraw_ = false;
        }

        // 1. Draw the cached radial-gradient image.
        g.drawImageAt(cachedGradient_, 0, 0);

        // 2. Overlay depth-zone ring fills (tinted, very low alpha).
        const float cx = static_cast<float>(bounds.getCentreX());
        const float cy = static_cast<float>(bounds.getCentreY());
        const float halfMin = static_cast<float>(std::min(bounds.getWidth(),
                                                           bounds.getHeight())) * 0.5f;

        paintDepthZoneRings(g, cx, cy, halfMin);

        // 3. When the ocean is empty (no coupling routes), draw additional faint
        //    concentric ring outlines as a background texture.
        if (!hasCouplingRoutes_)
            paintEmptyStateTexture(g, cx, cy, halfMin);
    }

    //==========================================================================
    void resized() override
    {
        // Flag the gradient cache as stale.  The image will be rebuilt on the
        // next paint() call at the correct size.
        needsRedraw_ = true;
    }

    //==========================================================================
    /**
        Call whenever the number of active coupling routes changes.
        Triggers a repaint so the empty-state texture can appear or disappear.
    */
    void setHasCouplingRoutes(bool hasRoutes)
    {
        if (hasCouplingRoutes_ != hasRoutes)
        {
            hasCouplingRoutes_ = hasRoutes;
            repaint();
        }
    }

    //==========================================================================
    // Accessors

    bool hasCouplingRoutes() const noexcept { return hasCouplingRoutes_; }

private:
    //==========================================================================
    juce::Image cachedGradient_;
    bool        hasCouplingRoutes_ = false;
    bool        needsRedraw_       = true;

    //==========================================================================
    /**
        Rebuild the cached gradient juce::Image at the current component size.

        Creates a software-rendered ARGB image and fills it with a radial
        ColourGradient using 4 colour stops matching the Ocean depth tokens:

            0.00  → Ocean::shallow   (centre)
            0.30  → Ocean::twilight  (inner-mid)
            0.60  → Ocean::deep      (outer-mid)
            1.00  → Ocean::abyss     (edge)

        The image is stored in cachedGradient_ and reused across paint() calls
        until the component is resized.
    */
    void rebuildGradientImage()
    {
        const int w = getWidth();
        const int h = getHeight();

        if (w <= 0 || h <= 0)
        {
            cachedGradient_ = juce::Image();
            return;
        }

        // Allocate (or reuse existing allocation when size is identical).
        if (cachedGradient_.getWidth() != w || cachedGradient_.getHeight() != h
            || cachedGradient_.isNull())
        {
            cachedGradient_ = juce::Image(juce::Image::ARGB, w, h, /*clearImage=*/true);
        }

        juce::Graphics ig(cachedGradient_);

        // Radial gradient: centre → edge.
        // juce::ColourGradient(c1, x1, y1, c2, x2, y2, isRadial)
        // For a radial gradient the start point is the centre and the end point
        // controls the radius.  We use the full half-diagonal so the gradient
        // always reaches the corners even on non-square components.
        const float cx = static_cast<float>(w) * 0.5f;
        const float cy = static_cast<float>(h) * 0.5f;

        // Radius to corner ensures no un-filled rectangle corners.
        const float cornerRadius = std::sqrt(cx * cx + cy * cy);

        juce::ColourGradient grad(
            juce::Colour(GalleryColors::Ocean::shallow),   // centre colour
            cx, cy,
            juce::Colour(GalleryColors::Ocean::abyss),     // edge colour
            cx + cornerRadius, cy,                          // point at edge
            /*isRadial=*/true
        );

        // Intermediate colour stops mapped to normalised [0, 1] positions.
        // juce::ColourGradient uses the distance from the centre point,
        // normalised by the distance to the end point (cornerRadius).
        // We target visual radii that are fractions of cornerRadius.
        grad.addColour(kSunlitRadius,   juce::Colour(GalleryColors::Ocean::twilight));
        grad.addColour(kMidnightRadius, juce::Colour(GalleryColors::Ocean::deep));

        ig.setGradientFill(grad);
        ig.fillAll();
    }

    //==========================================================================
    /**
        Paint the three depth-zone ring fills at the appropriate radii.
        These are drawn with very low alpha so they tint, not flood, the scene.
    */
    void paintDepthZoneRings(juce::Graphics& g,
                             float cx, float cy,
                             float halfMin) const
    {
        // Sunlit zone — warm cyan tint, 4 % alpha.
        {
            const float r = kSunlitRadius * halfMin;
            g.setColour(juce::Colour(GalleryColors::Ocean::sunlitTint).withAlpha(0.04f));
            g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
        }

        // Twilight zone — blue tint, 3 % alpha.
        {
            const float r = kTwilightRadius * halfMin;
            g.setColour(juce::Colour(GalleryColors::Ocean::twilightTint).withAlpha(0.03f));
            g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
        }

        // Midnight zone — violet tint, 4 % alpha.
        {
            const float r = kMidnightRadius * halfMin;
            g.setColour(juce::Colour(GalleryColors::Ocean::midnightTint).withAlpha(0.04f));
            g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
        }
    }

    //==========================================================================
    /**
        Paint a faint concentric ring texture for the empty-ocean state.

        Draws 1 px stroked ellipses at the three depth-zone radii plus a set of
        intermediate rings to fill the space.  All rings use 5 % white so they
        read as subtle grid-lines rather than imposing graphic elements.

        This pass is skipped when prefersReducedMotion() is true (although these
        rings are static, respecting the preference keeps all decorative elements
        behind the gate for consistency with animated counterparts).
    */
    void paintEmptyStateTexture(juce::Graphics& g,
                                float cx, float cy,
                                float halfMin) const
    {
        // Respect the reduce-motion / reduce-transparency preference.
        // Static rings are decorative — omitting them harms no functionality.
        if (A11y::prefersReducedMotion())
            return;

        const juce::Colour ringColour = juce::Colours::white.withAlpha(0.05f);
        g.setColour(ringColour);

        // Draw rings at evenly-spaced fractional radii from 0.10 to 0.90.
        // The three depth-zone radii naturally fall within this range and
        // receive no special treatment here (uniformity is the intent).
        constexpr int   kNumRings  = 9;
        constexpr float kRingStep  = 0.10f;  // fractions: 0.10, 0.20, … 0.90

        for (int i = 1; i <= kNumRings; ++i)
        {
            const float fraction = static_cast<float>(i) * kRingStep;
            const float r = fraction * halfMin;
            g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);
        }
    }

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OceanBackground)
};

} // namespace xoceanus
