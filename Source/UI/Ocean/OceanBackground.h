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

#include <array>
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

        // 1b. Intensity-responsive gradient overlay — brightens center when audio plays.
        //     Prototype: center rgb(26+i*12, 36+i*6, 56+i*18) → edge stays dark.
        if (intensity_ > 0.01f)
        {
            const float cxf = static_cast<float>(bounds.getCentreX());
            const float cyf = static_cast<float>(bounds.getCentreY());
            const float maxDim = static_cast<float>(std::max(bounds.getWidth(),
                                                              bounds.getHeight())) * 0.65f;
            juce::ColourGradient intensityGrad(
                juce::Colour(static_cast<juce::uint8>(26 + intensity_ * 12),
                             static_cast<juce::uint8>(36 + intensity_ * 6),
                             static_cast<juce::uint8>(56 + intensity_ * 18)).withAlpha(intensity_ * 0.3f),
                cxf, cyf,
                juce::Colours::transparentBlack,
                cxf + maxDim, cyf, true);
            g.setGradientFill(intensityGrad);
            g.fillRect(bounds);
        }

        // 1c. Ambient breathing sweep — slow teal radial gradient drifts horizontally.
        //     Prototype: sin(time * 0.02) maps to X position, very subtle 1.8% alpha.
        if (!A11y::prefersReducedMotion())
        {
            const float bw = static_cast<float>(bounds.getWidth());
            const float bh = static_cast<float>(bounds.getHeight());
            const float breatheX = (std::sin(waveTime_ * 1.2f) * 0.5f + 0.5f) * bw;
            juce::ColourGradient breatheGrad(
                juce::Colour(60, 180, 170).withAlpha(0.018f),
                breatheX, bh * 0.5f,
                juce::Colours::transparentBlack,
                breatheX + bw * 0.4f, bh * 0.5f, true);
            g.setGradientFill(breatheGrad);
            g.fillRect(bounds);
        }

        // 2. Overlay depth-zone ring fills (tinted, very low alpha).
        const float cx = static_cast<float>(bounds.getCentreX());
        const float cy = static_cast<float>(bounds.getCentreY());
        const float halfMin = static_cast<float>(std::min(bounds.getWidth(),
                                                           bounds.getHeight())) * 0.5f;

        paintDepthZoneRings(g, cx, cy, halfMin);

        // 3. Animated wave surface — master output waveform trace + depth echoes.
        //    Driven by waveData_ pushed from OceanView's timer reading the
        //    master WaveformFifo.  Skipped if reduced motion is preferred.
        if (!A11y::prefersReducedMotion())
            paintWaveSurface(g, static_cast<float>(bounds.getWidth()),
                                static_cast<float>(bounds.getHeight()));

        // 4. Animated teal depth rings — ALWAYS drawn (not just when empty).
        //    Prototype: 4 rings at r=80,190,300,410 that wobble with intensity.
        if (!A11y::prefersReducedMotion())
            paintAnimatedDepthRings(g, cx, cy, static_cast<float>(bounds.getWidth()),
                                   static_cast<float>(bounds.getHeight()));

        // 5. Ghost buoy outlines — shown when no engines are loaded (FIX 12).
        //    Four faint dashed circles hint at the slot positions so new users
        //    know where to drop engines.
        if (engineCount_ == 0)
            paintGhostOutlines(g, static_cast<float>(bounds.getWidth()),
                               static_cast<float>(bounds.getHeight()));
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
    /**
        Push latest master output waveform data for the ocean wave surface.
        Called from OceanView's timer (10 Hz) — not from audio thread.

        @param samples  Array of waveform samples (typically 120 points,
                        downsampled from the 512-sample WaveformFifo)
        @param count    Number of samples (clamped to kWavePoints max)
        @param rms      RMS level 0-1, drives wave intensity/amplitude
    */
    void setWaveData(const float* samples, int count, float rms)
    {
        const int n = std::min(count, kWavePoints);
        for (int i = 0; i < n; ++i)
            waveData_[i] = samples[i];
        for (int i = n; i < kWavePoints; ++i)
            waveData_[i] = 0.0f;
        intensity_ = juce::jlimit(0.0f, 1.0f, rms);
        waveTime_ += 0.1f; // advance animation phase (~10 Hz tick)
    }

    //==========================================================================
    /**
        Call whenever the count of loaded engines changes.
        When engineCount_ == 0 the background paints ghost buoy outlines as an
        empty-state affordance.
    */
    void setEngineCount(int count)
    {
        const int clamped = juce::jmax(0, count);
        if (engineCount_ != clamped)
        {
            engineCount_ = clamped;
            repaint();
        }
    }

    //==========================================================================
    // Accessors

    bool hasCouplingRoutes() const noexcept { return hasCouplingRoutes_; }
    int  getEngineCount()    const noexcept { return engineCount_; }

private:
    //==========================================================================
    // Wave surface constants
    static constexpr int kWavePoints = 120;

    juce::Image cachedGradient_;
    bool        hasCouplingRoutes_ = false;
    bool        needsRedraw_       = true;
    int         engineCount_       = 0;  ///< number of loaded engines; 0 → ghost outlines shown

    // Wave surface state (pushed from OceanView timer, not audio thread)
    std::array<float, kWavePoints> waveData_ {};
    float waveTime_   = 0.0f;
    float intensity_  = 0.0f;

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

        // #1008 FIX 3: 4 gradient stops that correctly represent all 3 depth zones.
        // Zone 1 (Sunlit):   centre → kSunlitRadius   → Ocean::shallow → Ocean::twilight
        // Zone 2 (Twilight): kSunlitRadius → midpoint → Ocean::twilight → midpoint colour
        // Zone 3 (Midnight): midpoint → kMidnightRadius → Ocean::deep
        // Edge:              kMidnightRadius → corner   → Ocean::abyss (already set)
        //
        // Positions are in juce::ColourGradient normalised space [0,1] where 1.0 =
        // cornerRadius.  Depth-zone constants are fractions of halfMin, so we
        // scale by halfMin/cornerRadius to align them visually on non-square windows.
        const float halfMin = std::min(cx, cy);   // cx = w/2, cy = h/2
        const float scale   = (cornerRadius > 0.0f) ? halfMin / cornerRadius : 1.0f;

        // Stop 1: inner edge of sunlit zone → twilight colour
        grad.addColour(kSunlitRadius * scale,
                       juce::Colour(GalleryColors::Ocean::twilight));

        // Stop 2: midpoint between twilight and midnight zones — bridging colour.
        // Derived as the 50% blend of twilight and deep so there is no sharp jump.
        const float midStop = ((kTwilightRadius + kMidnightRadius) * 0.5f) * scale;
        const juce::Colour midColour =
            juce::Colour(GalleryColors::Ocean::twilight)
                .interpolatedWith(juce::Colour(GalleryColors::Ocean::deep), 0.5f);
        grad.addColour(midStop, midColour);

        // Stop 3: outer edge of midnight zone → deep colour
        grad.addColour(kMidnightRadius * scale,
                       juce::Colour(GalleryColors::Ocean::deep));

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
        Paint animated teal depth rings — always visible, wobble with intensity.
        Prototype: 4 rings at r=80, 190, 300, 410px that wobble with wave time.
    */
    void paintAnimatedDepthRings(juce::Graphics& g,
                                  float cx, float cy,
                                  float /*w*/, float /*h*/) const
    {
        const juce::Colour teal(60, 180, 170);
        static constexpr float kRingRadii[] = { 80.0f, 190.0f, 300.0f, 410.0f };

        for (int i = 0; i < 4; ++i)
        {
            const float baseR = kRingRadii[i];
            // Wobble: subtle radius oscillation driven by wave time + ring index
            const float wobble = std::sin(waveTime_ * 0.3f + baseR * 0.005f)
                               * intensity_ * 4.0f;
            const float r = baseR + wobble;
            const float alpha = 0.025f + intensity_ * 0.018f;

            g.setColour(teal.withAlpha(alpha));
            g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);
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
        // Static rings are purely decorative and not animated — they do not
        // trigger vestibular issues, so prefersReducedMotion does not gate them.
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
    /**
        Paint the animated ocean wave surface.

        Draws 4 waveform traces across the ocean:
          - 3 subtle depth echo layers (parallax, progressively fading)
          - 1 primary master-output trace (thick, with glow fill underneath)

        The primary trace shows the actual master bus waveform; the depth layers
        are scaled/phase-shifted copies for visual depth.  Wave amplitude scales
        with intensity_ (RMS level from master output).
    */
    void paintWaveSurface(juce::Graphics& g, float w, float h) const
    {
        // Build wave displacement array from master output data + sine modulation
        // When no audio is playing (intensity_ ≈ 0), subtle ambient sine waves
        // keep the ocean alive.  When audio plays, real waveform data dominates.
        const float baseAmp = 1.5f + intensity_ * 14.0f;
        const float chop    = intensity_ * 7.0f;

        std::array<float, kWavePoints> waveY {};
        for (int i = 0; i < kWavePoints; ++i)
        {
            const float fi = static_cast<float>(i);
            // Ambient sine waves (always present, subtle)
            float ambient = std::sin(waveTime_ * 0.8f + fi * 0.08f) * baseAmp
                          + std::sin(waveTime_ * 1.3f + fi * 0.15f) * baseAmp * 0.6f
                          + std::sin(waveTime_ * 2.1f + fi * 0.22f) * chop;
            // Blend in real waveform data when audio is active
            float real = waveData_[static_cast<size_t>(i)] * 40.0f * intensity_;
            waveY[static_cast<size_t>(i)] = ambient + real;
        }

        const juce::Colour teal(60, 180, 170);

        // ── 3 depth echo layers (subtle, behind primary) ──
        for (int layer = 0; layer < 3; ++layer)
        {
            const float yBase = h * (0.22f + static_cast<float>(layer) * 0.18f);
            const float alpha = 0.025f + intensity_ * 0.03f - static_cast<float>(layer) * 0.006f;
            const float scale = 0.5f - static_cast<float>(layer) * 0.1f;
            const float phaseOff = static_cast<float>(layer) * 0.6f;

            juce::Path depthPath;
            for (int i = 0; i < kWavePoints; ++i)
            {
                const float x = (static_cast<float>(i) / kWavePoints) * w;
                const float wave = waveY[static_cast<size_t>(i)] * scale;
                const float drift = std::sin(waveTime_ * (0.7f - static_cast<float>(layer) * 0.15f)
                                           + static_cast<float>(i) * 0.05f + phaseOff)
                                  * (2.0f + intensity_ * 3.0f);
                const float y = yBase + wave + drift;
                if (i == 0) depthPath.startNewSubPath(x, y);
                else        depthPath.lineTo(x, y);
            }
            g.setColour(teal.withAlpha(std::max(0.0f, alpha)));
            g.strokePath(depthPath, juce::PathStrokeType(0.7f));
        }

        // ── Primary waveform trace (master output) ──
        const float primaryY = h * 0.45f;

        juce::Path primaryPath;
        for (int i = 0; i < kWavePoints; ++i)
        {
            const float x = (static_cast<float>(i) / kWavePoints) * w;
            const float y = primaryY + waveY[static_cast<size_t>(i)] * 1.2f;
            if (i == 0) primaryPath.startNewSubPath(x, y);
            else        primaryPath.lineTo(x, y);
        }

        // Glow fill underneath the primary trace
        juce::Path fillPath(primaryPath);
        fillPath.lineTo(w, primaryY + 40.0f);
        fillPath.lineTo(0.0f, primaryY + 40.0f);
        fillPath.closeSubPath();
        juce::ColourGradient glowFill(
            teal.withAlpha(0.04f + intensity_ * 0.06f),
            0.0f, primaryY,
            juce::Colours::transparentBlack,
            0.0f, primaryY + 40.0f, false);
        g.setGradientFill(glowFill);
        g.fillPath(fillPath);

        // Thick primary line
        g.setColour(teal.withAlpha(0.15f + intensity_ * 0.35f));
        g.strokePath(primaryPath, juce::PathStrokeType(2.0f + intensity_ * 1.5f));

        // Bright core on top
        g.setColour(juce::Colour(120, 220, 210).withAlpha(0.08f + intensity_ * 0.25f));
        g.strokePath(primaryPath, juce::PathStrokeType(1.0f));
    }

    //==========================================================================
    /**
        Paint four ghost buoy outlines when no engines are loaded (FIX 12).

        Positions match the prototype spec:
            slot 0: (30%W, 35%H)   slot 1: (70%W, 35%H)
            slot 2: (30%W, 65%H)   slot 3: (70%W, 65%H)

        Each ghost is drawn as:
          - A dashed circle stroke (4px dash, 4px gap), radius 28px,
            teal rgba(127,219,202,0.12)
          - A "+" centred in the circle, rgba(127,219,202,0.15), 20px font
          - A slot number below the circle, rgba(127,219,202,0.10), 8px font
    */
    void paintGhostOutlines(juce::Graphics& g, float w, float h) const
    {
        static constexpr float kGhostPositions[4][2] = {
            { 0.30f, 0.35f }, { 0.70f, 0.35f },
            { 0.30f, 0.65f }, { 0.70f, 0.65f }
        };
        const juce::Colour ghostCol(127, 219, 202);

        for (int i = 0; i < 4; ++i)
        {
            const float gx = kGhostPositions[i][0] * w;
            const float gy = kGhostPositions[i][1] * h;
            const float r  = 28.0f;

            // ── Dashed circle ─────────────────────────────────────────────
            juce::Path circle;
            circle.addEllipse(gx - r, gy - r, r * 2.0f, r * 2.0f);
            juce::Path dashedCircle;
            const float dashes[] = { 4.0f, 4.0f };
            juce::PathStrokeType(1.5f).createDashedStroke(dashedCircle, circle, dashes, 2);
            g.setColour(ghostCol.withAlpha(0.12f));
            g.fillPath(dashedCircle);

            // ── "+" text ──────────────────────────────────────────────────
            g.setFont(juce::Font(juce::FontOptions{}.withHeight(20.0f)));
            g.setColour(ghostCol.withAlpha(0.15f));
            g.drawText("+",
                       juce::Rectangle<float>(gx - 15.0f, gy - 12.0f, 30.0f, 24.0f).toNearestInt(),
                       juce::Justification::centred, false);

            // ── Slot number ───────────────────────────────────────────────
            g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.0f)));
            g.setColour(ghostCol.withAlpha(0.10f));
            g.drawText(juce::String(i + 1),
                       juce::Rectangle<float>(gx - 10.0f, gy + r + 4.0f, 20.0f, 10.0f).toNearestInt(),
                       juce::Justification::centred, false);
        }
    }

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OceanBackground)
};

} // namespace xoceanus
