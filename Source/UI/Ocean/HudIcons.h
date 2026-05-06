// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// HudIcons.h — Standard juce::Path icon factory for the SubmarineHudBar.
//
// Each static method returns a juce::Path drawn in a normalised 0–1 unit space
// (top-left origin, x right, y down).  The caller scales by transforming the
// path to the desired pixel bounds via AffineTransform::scale + translate.
//
// Included icons:
//   makeGearIcon()   — 8-tooth gear with filled centre and concentric hole
//   makeUndoIcon()   — counterclockwise arc + left-pointing arrowhead
//   makeRedoIcon()   — clockwise arc + right-pointing arrowhead
//   makeExportIcon() — down-arrow into tray (export-to-disk convention)
//
// Visual weight is calibrated to match the 32×32 px icon buttons in SubmarineHudBar
// at ~55% alpha (idle) / ~85% alpha (hovered).

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

//==============================================================================
/**
    HudIcons

    Inline static factory methods returning juce::Path objects for the
    SubmarineHudBar icon buttons.  All paths live in a 1×1 unit space;
    apply juce::AffineTransform::scale(size).translated(x, y) to position.
*/
struct HudIcons
{
    //==========================================================================
    /**
        8-tooth trapezoidal gear icon.

        Two sub-paths:
          1. Gear outline (filled): alternating between tooth tips at outerR
             and valley points at innerR.
          2. Centre hole (separate path, caller draws over fill with bg colour
             or uses it as a clip): circle at holeR.

        Caller pattern:
            auto gear = HudIcons::makeGearIcon();
            g.setColour(iconColour);
            g.fillPath(gear, transform);
            // Punch hole:
            auto hole = HudIcons::makeGearHole();
            g.setColour(backgroundColour);
            g.fillPath(hole, transform);
    */
    static juce::Path makeGearIcon()
    {
        // Unit space: centre at (0.5, 0.5).
        const float cx        = 0.5f;
        const float cy        = 0.5f;
        const int   nTeeth    = 8;
        const float outerR    = 0.42f;   // tip of teeth (fraction of unit space)
        const float innerR    = 0.29f;   // root / body radius
        const float halfTooth = static_cast<float>(M_PI)
                              / static_cast<float>(nTeeth) * 0.55f;

        juce::Path gear;

        for (int i = 0; i < nTeeth * 2; ++i)
        {
            const float baseAngle = static_cast<float>(i)
                                  * static_cast<float>(M_PI)
                                  / static_cast<float>(nTeeth);
            const bool isTip = (i % 2 == 0);
            const float r    = isTip ? outerR : innerR;

            if (isTip)
            {
                const float a1 = baseAngle - halfTooth;
                const float a2 = baseAngle + halfTooth;

                const float x1 = cx + outerR * std::sin(a1);
                const float y1 = cy - outerR * std::cos(a1);
                const float x2 = cx + outerR * std::sin(a2);
                const float y2 = cy - outerR * std::cos(a2);

                if (i == 0)
                    gear.startNewSubPath(x1, y1);
                else
                    gear.lineTo(x1, y1);

                gear.lineTo(x2, y2);
            }
            else
            {
                const float x = cx + r * std::sin(baseAngle);
                const float y = cy - r * std::cos(baseAngle);
                gear.lineTo(x, y);
            }
        }
        gear.closeSubPath();
        return gear;
    }

    /** Centre hole for the gear icon (to punch through with background colour). */
    static juce::Path makeGearHole()
    {
        const float cx    = 0.5f;
        const float cy    = 0.5f;
        const float holeR = 0.145f;   // matches visual weight of 8-tooth gear
        juce::Path hole;
        hole.addEllipse(cx - holeR, cy - holeR, holeR * 2.0f, holeR * 2.0f);
        return hole;
    }

    //==========================================================================
    /**
        Undo icon — counterclockwise arc (≈240°) with left-pointing arrowhead
        at the terminus.  Unit space: centre at (0.5, 0.5), arc radius ≈ 0.32.

        To draw:
            auto path = HudIcons::makeUndoIcon();
            g.strokePath(path, juce::PathStrokeType(strokeW,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded), transform);
    */
    static juce::Path makeUndoIcon()
    {
        return makeArrowArc(/*isRedo=*/false);
    }

    //==========================================================================
    /**
        Redo icon — clockwise arc (≈240°) with right-pointing arrowhead.
        Mirror of makeUndoIcon().
    */
    static juce::Path makeRedoIcon()
    {
        return makeArrowArc(/*isRedo=*/true);
    }

    //==========================================================================
    /**
        Export icon — down-arrow into tray (export-to-disk convention).

        Composed of:
          - A down-pointing arrow (vertical stem + triangular head)
          - A horizontal tray/shelf line below with short upturned ends

        All coordinates in 0–1 unit space.
    */
    static juce::Path makeExportIcon()
    {
        juce::Path p;

        // ── Arrow: one continuous subpath — stem top → tip → left wing ──
        const float stemX  = 0.5f;
        const float stemT  = 0.12f;  // top of stem
        const float stemB  = 0.60f;  // shoulder Y where arrowhead begins

        // ── Arrowhead dimensions ──
        const float ahW  = 0.30f;  // half-width at shoulder
        const float ahT  = stemB;   // shoulder Y
        const float ahB  = 0.76f;  // tip Y

        // Stem enters the tip and continues up the left wing in a single subpath.
        // The right wing re-starts from the tip (unavoidable for a Y-shape).
        p.startNewSubPath(stemX - ahW, ahT);  // left wing end
        p.lineTo(stemX, ahB);                 // down to tip
        p.lineTo(stemX, stemT);               // up the stem to top

        p.startNewSubPath(stemX, ahB);        // back to tip
        p.lineTo(stemX + ahW, ahT);           // right wing

        // ── Tray / shelf line ──
        const float trayY  = 0.86f;
        const float trayL  = 0.18f;  // left X
        const float trayR  = 0.82f;  // right X
        const float rimH   = 0.10f;  // height of upturned rim ends

        p.startNewSubPath(trayL, trayY - rimH);
        p.lineTo(trayL, trayY);
        p.lineTo(trayR, trayY);
        p.lineTo(trayR, trayY - rimH);

        return p;
    }

    //==========================================================================
    /**
        Helper: draw a path at pixel bounds using a uniform scale transform.

        @param g          Graphics context.
        @param path       Normalised 0–1 path from this header.
        @param bounds     Pixel destination rectangle.
        @param colour     Stroke / fill colour.
        @param strokeW    If > 0 the path is stroked; if 0 it is filled.
    */
    static void drawIconInBounds(juce::Graphics& g,
                                 const juce::Path& path,
                                 const juce::Rectangle<float>& bounds,
                                 juce::Colour colour,
                                 float strokeW = 1.5f)
    {
        const auto xf = juce::AffineTransform::scale(bounds.getWidth(),
                                                      bounds.getHeight())
                            .translated(bounds.getX(), bounds.getY());
        g.setColour(colour);
        if (strokeW > 0.0f)
        {
            g.strokePath(path,
                         juce::PathStrokeType(strokeW,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded),
                         xf);
        }
        else
        {
            g.fillPath(path, xf);
        }
    }

    //==========================================================================
    /**
        3×3 dot-grid icon — used for the MATRIX button in SubmarineHudBar.

        Nine filled circles arranged in a 3-row × 3-column grid, evenly spaced
        in the 0–1 unit square.  Each dot has radius ~0.07; gap ~0.16 between centres.

        Caller pattern:
            auto grid = HudIcons::makeGridIcon();
            HudIcons::drawIconInBounds(g, grid, bounds, iconColour, 0.0f);
    */
    static juce::Path makeGridIcon()
    {
        juce::Path p;

        const float dotR   = 0.065f;
        const float start  = 0.20f;
        const float step   = 0.30f;

        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                const float cx = start + static_cast<float>(col) * step;
                const float cy = start + static_cast<float>(row) * step;
                p.addEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
            }
        }

        return p;
    }

private:
    //==========================================================================
    /** Shared arc+arrowhead builder for undo/redo. */
    static juce::Path makeArrowArc(bool isRedo)
    {
        // Unit space: arc centred at (0.5, 0.5), radius 0.30.
        const float cx       = 0.5f;
        const float cy       = 0.5f;
        const float r        = 0.30f;
        const float arrowLen = 0.18f;

        juce::Path arc;

        if (!isRedo)
        {
            // Undo: arc sweeps from ~210° to ~30° (counterclockwise visual).
            const float startA = static_cast<float>(7.0 * M_PI / 6.0);
            const float endA   = static_cast<float>(M_PI / 6.0);

            arc.addArc(cx - r, cy - r, r * 2.0f, r * 2.0f,
                       startA, endA, /*startAsNewSubPath=*/true);

            // Arrowhead at endA.
            const float tipX  = cx + r * std::sin(endA);
            const float tipY  = cy - r * std::cos(endA);
            const float tanA  = endA + static_cast<float>(M_PI * 0.5);
            const float armA1 = tanA + static_cast<float>(M_PI * 0.6);
            const float armA2 = tanA - static_cast<float>(M_PI * 0.6);

            arc.startNewSubPath(tipX, tipY);
            arc.lineTo(tipX + arrowLen * std::sin(armA1),
                       tipY - arrowLen * std::cos(armA1));
            arc.startNewSubPath(tipX, tipY);
            arc.lineTo(tipX + arrowLen * std::sin(armA2),
                       tipY - arrowLen * std::cos(armA2));
        }
        else
        {
            // Redo: mirror — arc sweeps clockwise.
            const float startA = static_cast<float>(5.0 * M_PI / 6.0);
            const float endA   = static_cast<float>(11.0 * M_PI / 6.0);

            arc.addArc(cx - r, cy - r, r * 2.0f, r * 2.0f,
                       startA, endA, /*startAsNewSubPath=*/true);

            const float tipX  = cx + r * std::sin(endA);
            const float tipY  = cy - r * std::cos(endA);
            const float tanA  = endA - static_cast<float>(M_PI * 0.5);
            const float armA1 = tanA + static_cast<float>(M_PI * 0.6);
            const float armA2 = tanA - static_cast<float>(M_PI * 0.6);

            arc.startNewSubPath(tipX, tipY);
            arc.lineTo(tipX + arrowLen * std::sin(armA1),
                       tipY - arrowLen * std::cos(armA1));
            arc.startNewSubPath(tipX, tipY);
            arc.lineTo(tipX + arrowLen * std::sin(armA2),
                       tipY - arrowLen * std::cos(armA2));
        }

        return arc;
    }
};

} // namespace xoceanus
