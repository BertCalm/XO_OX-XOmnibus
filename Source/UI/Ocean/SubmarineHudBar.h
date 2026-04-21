// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// SubmarineHudBar.h — Floating submarine HUD navigation bar for the XOceanus Ocean View.
//
// The bar floats at the top of the ocean viewport (positioned by the parent at
// top: 12px, left: 16px, right: 16px). Height is 40 px. The container itself
// is transparent and passes mouse clicks through to the button hit regions;
// setInterceptsMouseClicks(true, true) is used so this component receives events
// but only consumes them when a hit region is matched.
//
// Layout (left → right):
//
//   [ ☰ Engines ]  [ ↶ ]  [ ↷ ]  ————————spacer————————  [ Chain ]  [ Export ]  Output  [slider]  [ ⚙ ]
//
// Button styles follow the submarine frosted-glass pill aesthetic:
//   • Default : bg rgba(14,16,22,0.70), border rgba(200,204,216,0.10), text rgba(200,204,216,0.55)
//   • Hover   : bg rgba(200,204,216,0.08), border rgba(200,204,216,0.18), text rgba(200,204,216,0.85)
//   • Active  : bg rgba(60,180,170,0.10),  border rgba(60,180,170,0.35),  text rgba(60,180,170,0.90)
//
// Icon buttons (Undo, Redo, Settings) are 32×32 px squares with custom Path geometry:
//   • Undo    — counterclockwise arc with left-pointing arrowhead
//   • Redo    — clockwise arc with right-pointing arrowhead
//   • Settings — small circle surrounded by 6 equally-spaced circular bumps (gear)
//
// The Chain button prepends a small chain-link icon (two interlocking ovals).
//
// The Output slider is 70 px wide, 3 px track, teal fill, 10 px thumb.
// Horizontal drag changes the normalised value (0–1). Re-reads start position
// on mouseDown so there are no jumps.
//
// All interactions fire std::function callbacks — parent wires them without any
// upward include dependency on this component.
//
// File is entirely self-contained (header-only inline) following the XOceanus
// convention for UI sub-components.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include <functional>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

//==============================================================================
/**
    SubmarineHudBar

    Floating frosted-glass navigation bar pinned to the top of the Ocean View.
    See file header for full documentation.
*/
class SubmarineHudBar : public juce::Component
{
public:
    //==========================================================================
    static constexpr int kBarHeight    = 40;
    static constexpr int kBtnHeight    = 28;
    static constexpr int kIconBtnSize  = 32;
    static constexpr int kSliderWidth  = 70;

    //==========================================================================
    // Callbacks — parent wires these in the editor constructor.

    std::function<void()>      onEnginesClicked;
    std::function<void()>      onUndo;
    std::function<void()>      onRedo;
    std::function<void()>      onChainToggled;   // toggles chain mode; check chainModeActive_ to read new state
    std::function<void()>      onExportClicked;
    std::function<void()>      onSettingsClicked;
    std::function<void(float)> onOutputChanged;  // 0–1 normalised

    //==========================================================================
    SubmarineHudBar()
    {
        setOpaque(false);
        // Container passes through events to hit regions via manual hit-testing.
        // Children are painted manually — no real child components.
        setInterceptsMouseClicks(true, true);
    }

    //==========================================================================
    // State pushers — call from editor on the message thread.

    void setChainModeActive(bool active)
    {
        if (chainModeActive_ != active)
        {
            chainModeActive_ = active;
            repaint();
        }
    }

    /** Returns true when the Chain mode toggle is currently active. */
    bool isChainModeActive() const noexcept { return chainModeActive_; }

    void setOutputLevel(float level01)
    {
        const float clamped = juce::jlimit(0.0f, 1.0f, level01);
        if (outputLevel_ != clamped)
        {
            outputLevel_ = clamped;
            repaint();
        }
    }

private:
    //==========================================================================
    // Region IDs — used for hit-testing and hover tracking.

    enum RegionId
    {
        kRegEngines  = 0,
        kRegUndo     = 1,
        kRegRedo     = 2,
        kRegChain    = 3,
        kRegExport   = 4,
        kRegSlider   = 5,
        kRegSettings = 6,
    };

    struct HudRegion
    {
        juce::Rectangle<float> bounds;
        int                    id;
    };

    //==========================================================================
    // Layout helpers

    /** Rebuild all HudRegion rectangles from the current component width. */
    void buildLayout()
    {
        regions_.clear();

        const float w   = static_cast<float>(getWidth());
        const float h   = static_cast<float>(getHeight() > 0 ? getHeight() : kBarHeight);
        const float gap = 8.0f;

        const float btnY    = (h - static_cast<float>(kBtnHeight))   * 0.5f;
        const float iconY   = (h - static_cast<float>(kIconBtnSize))  * 0.5f;

        float x = 0.0f; // left cursor

        // --- Engines button (text pill, wider) ---
        {
            const float btnW = 74.0f;
            juce::Rectangle<float> r(x, btnY, btnW, static_cast<float>(kBtnHeight));
            regions_.push_back({ r, kRegEngines });
            enginesBounds_ = r;
            x += btnW + gap;
        }

        // --- Undo icon button (32×32) ---
        {
            juce::Rectangle<float> r(x, iconY, static_cast<float>(kIconBtnSize),
                                              static_cast<float>(kIconBtnSize));
            regions_.push_back({ r, kRegUndo });
            undoBounds_ = r;
            x += static_cast<float>(kIconBtnSize) + gap;
        }

        // --- Redo icon button (32×32) ---
        {
            juce::Rectangle<float> r(x, iconY, static_cast<float>(kIconBtnSize),
                                              static_cast<float>(kIconBtnSize));
            regions_.push_back({ r, kRegRedo });
            redoBounds_ = r;
            x += static_cast<float>(kIconBtnSize) + gap;
        }

        // Spacer — flex:1 — we'll fill it implicitly by working right-to-left next.

        // ---- Right side: work right-to-left ----
        float rx = w;

        // --- Settings icon button (32×32) ---
        {
            juce::Rectangle<float> r(rx - static_cast<float>(kIconBtnSize), iconY,
                                     static_cast<float>(kIconBtnSize),
                                     static_cast<float>(kIconBtnSize));
            regions_.push_back({ r, kRegSettings });
            settingsBounds_ = r;
            rx -= static_cast<float>(kIconBtnSize) + gap;
        }

        // --- Output slider (70 px wide) ---
        {
            const float sliderH  = 3.0f;
            const float thumbD   = 10.0f;
            // Reserve a touch region that is kBtnHeight tall for easier dragging.
            juce::Rectangle<float> sliderTouch(rx - static_cast<float>(kSliderWidth),
                                               (h - static_cast<float>(kBtnHeight)) * 0.5f,
                                               static_cast<float>(kSliderWidth),
                                               static_cast<float>(kBtnHeight));
            regions_.push_back({ sliderTouch, kRegSlider });
            sliderBounds_ = sliderTouch;

            // Visual track centred vertically in the touch region.
            sliderTrackBounds_ = juce::Rectangle<float>(
                sliderTouch.getX(),
                sliderTouch.getCentreY() - sliderH * 0.5f,
                sliderTouch.getWidth(),
                sliderH);

            // Thumb (rendered during paint; stored as convenience reference).
            (void)thumbD;

            rx -= static_cast<float>(kSliderWidth) + gap;
        }

        // --- "OUTPUT" label (9 px, uppercase) ---
        {
            const float labelW = 42.0f;
            outputLabelBounds_ = juce::Rectangle<float>(
                rx - labelW,
                (h - 9.0f) * 0.5f,
                labelW, 9.0f);
            rx -= labelW + gap;
        }

        // --- Export button (text pill) ---
        {
            const float btnW = 64.0f;
            juce::Rectangle<float> r(rx - btnW, btnY, btnW,
                                     static_cast<float>(kBtnHeight));
            regions_.push_back({ r, kRegExport });
            exportBounds_ = r;
            rx -= btnW + gap;
        }

        // --- Chain button (text pill with chain-link icon) ---
        {
            const float btnW = 70.0f;
            juce::Rectangle<float> r(rx - btnW, btnY, btnW,
                                     static_cast<float>(kBtnHeight));
            regions_.push_back({ r, kRegChain });
            chainBounds_ = r;
            // rx -= btnW + gap;  // spacer fills remaining space
        }
    }

    //==========================================================================
    // paint()

    void paint(juce::Graphics& g) override
    {
        buildLayout();

        // --- Engines button ---
        paintTextButton(g, enginesBounds_, "ENGINES", kRegEngines,
                        false, /*withMenuIcon=*/true);

        // --- Undo icon button ---
        paintIconButton(g, undoBounds_, kRegUndo, false);
        paintUndoIcon(g, undoBounds_, /*isRedo=*/false);

        // --- Redo icon button ---
        paintIconButton(g, redoBounds_, kRegRedo, false);
        paintUndoIcon(g, redoBounds_, /*isRedo=*/true);

        // --- Chain button (active state if chainModeActive_) ---
        paintChainButton(g);

        // --- Export button ---
        paintTextButton(g, exportBounds_, "EXPORT", kRegExport,
                        false, /*withMenuIcon=*/false);

        // --- Output label ---
        {
            static const juce::Font outputLabelFont(juce::FontOptions{}
                          .withName(juce::Font::getDefaultSansSerifFontName())
                          .withHeight(9.0f));
            g.setFont(outputLabelFont);
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.25f));
            g.drawText("OUTPUT", outputLabelBounds_.toNearestInt(),
                       juce::Justification::centredRight, false);
        }

        // --- Output slider ---
        paintOutputSlider(g);

        // --- Settings icon button ---
        paintIconButton(g, settingsBounds_, kRegSettings, false);
        paintGearIcon(g, settingsBounds_);
    }

    void resized() override
    {
        buildLayout();
    }

    //--------------------------------------------------------------------------
    // Button paint helpers

    /** Resolve the frosted-glass colours for a given region. */
    struct BtnColors
    {
        juce::Colour bg;
        juce::Colour border;
        juce::Colour text;
    };

    BtnColors resolveColors(int regionId, bool isActive) const
    {
        const bool isHov = (hoveredRegion_ == regionId);

        if (isActive)
        {
            return {
                juce::Colour(60, 180, 170).withAlpha(0.10f),
                juce::Colour(60, 180, 170).withAlpha(0.35f),
                juce::Colour(60, 180, 170).withAlpha(0.90f)
            };
        }

        if (isHov)
        {
            return {
                juce::Colour(200, 204, 216).withAlpha(0.08f),
                juce::Colour(200, 204, 216).withAlpha(0.18f),
                juce::Colour(200, 204, 216).withAlpha(0.85f)
            };
        }

        return {
            juce::Colour(14, 16, 22).withAlpha(0.70f),
            juce::Colour(200, 204, 216).withAlpha(0.10f),
            juce::Colour(200, 204, 216).withAlpha(0.55f)
        };
    }

    void paintTextButton(juce::Graphics& g,
                         const juce::Rectangle<float>& bounds,
                         const char* label,
                         int regionId,
                         bool isActive,
                         bool withMenuIcon)
    {
        const auto c = resolveColors(regionId, isActive);

        g.setColour(c.bg);
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(c.border);
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

        static const juce::Font pillFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(11.0f));

        g.setFont(pillFont);
        g.setColour(c.text);

        if (withMenuIcon)
        {
            // Draw a small "≡" (three horizontal lines) on the left, then the label.
            const float cx  = bounds.getX() + 14.0f;
            const float cy  = bounds.getCentreY();
            const float lw  = 8.0f;
            const float gap = 2.5f;
            g.setColour(c.text);
            for (int i = -1; i <= 1; ++i)
            {
                g.fillRect(juce::Rectangle<float>(cx - lw * 0.5f,
                                                  cy + static_cast<float>(i) * gap - 0.5f,
                                                  lw, 1.0f));
            }
            // Label to the right of the icon.
            juce::Rectangle<float> textArea(bounds.getX() + 24.0f, bounds.getY(),
                                            bounds.getWidth() - 28.0f, bounds.getHeight());
            g.setColour(c.text);
            g.drawText(label, textArea.toNearestInt(), juce::Justification::centredLeft, false);
        }
        else
        {
            g.drawText(label, bounds.toNearestInt(), juce::Justification::centred, false);
        }
    }

    void paintIconButton(juce::Graphics& g,
                         const juce::Rectangle<float>& bounds,
                         int regionId,
                         bool isActive)
    {
        const auto c = resolveColors(regionId, isActive);

        g.setColour(c.bg);
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(c.border);
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    // Chain button — text pill with interlocking-oval icon

    void paintChainButton(juce::Graphics& g)
    {
        const auto c = resolveColors(kRegChain, chainModeActive_);

        g.setColour(c.bg);
        g.fillRoundedRectangle(chainBounds_, 8.0f);
        g.setColour(c.border);
        g.drawRoundedRectangle(chainBounds_, 8.0f, 1.0f);

        // Chain-link icon: two interlocking ovals drawn as a Path.
        // Left oval slightly to the left, right oval slightly to the right.
        const float iconCX = chainBounds_.getX() + 14.0f;
        const float iconCY = chainBounds_.getCentreY();
        const float ow     = 7.0f;  // oval full width
        const float oh     = 5.0f;  // oval full height
        const float offset = 2.5f;  // horizontal centre offset from icon centre

        g.setColour(c.text);

        // Left link
        {
            juce::Path oval;
            oval.addEllipse(iconCX - offset - ow * 0.5f,
                            iconCY - oh * 0.5f, ow, oh);
            g.strokePath(oval, juce::PathStrokeType(1.3f));
        }
        // Right link (overlaps left by ~offset)
        {
            juce::Path oval;
            oval.addEllipse(iconCX + offset - ow * 0.5f,
                            iconCY - oh * 0.5f, ow, oh);
            g.strokePath(oval, juce::PathStrokeType(1.3f));
        }

        // Label text
        static const juce::Font pillFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(11.0f));

        g.setFont(pillFont);
        g.setColour(c.text);

        juce::Rectangle<float> textArea(chainBounds_.getX() + 24.0f, chainBounds_.getY(),
                                        chainBounds_.getWidth() - 28.0f, chainBounds_.getHeight());
        g.drawText("CHAIN", textArea.toNearestInt(), juce::Justification::centredLeft, false);
    }

    //--------------------------------------------------------------------------
    // Undo / Redo icon — curved arc with arrowhead tip

    void paintUndoIcon(juce::Graphics& g,
                       const juce::Rectangle<float>& bounds,
                       bool isRedo)
    {
        const bool isHov = (hoveredRegion_ == (isRedo ? kRegRedo : kRegUndo));
        const juce::Colour col = isHov
            ? juce::Colour(200, 204, 216).withAlpha(0.85f)
            : juce::Colour(200, 204, 216).withAlpha(0.55f);

        g.setColour(col);

        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float r  = 6.5f;   // arc radius
        const float arrowLen = 4.0f;

        // Arc: for undo (counterclockwise arrow), arc sweeps from ~210° to ~30°.
        // For redo (clockwise), mirror horizontally.
        // Angles in JUCE are clockwise from 12 o'clock (radians).
        // We draw a 240° arc leaving a gap at the top where the arrowhead sits.

        juce::Path arc;

        if (!isRedo)
        {
            // Undo: arc goes counterclockwise → draw from ~(7 o'clock) to ~(11 o'clock).
            // In JUCE angles (clockwise from 12 o'clock):
            //   start = 210° (7 o'clock) = 7π/6 rad
            //   end   =  30° (1 o'clock) = π/6  rad  (sweeping back through 0)
            const float startA = static_cast<float>(7.0 * M_PI / 6.0);
            const float endA   = static_cast<float>(M_PI / 6.0);

            arc.addArc(cx - r, cy - r, r * 2.0f, r * 2.0f,
                       startA, endA, /*startAsNewSubPath=*/true);

            // Arrowhead at the end of the arc (at ~30°, pointing left/down).
            // Tip point on arc at endA:
            const float tipX = cx + r * std::sin(endA);
            const float tipY = cy - r * std::cos(endA);

            // Tangent direction at endA (clockwise tangent): rotate endA by +90°.
            const float tanA = endA + static_cast<float>(M_PI * 0.5);

            // Arrowhead: two short lines fanning from tip.
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
            // Redo: mirror of undo — arc goes clockwise.
            const float startA = static_cast<float>(5.0 * M_PI / 6.0); // ~150° (5 o'clock mirror)
            const float endA   = static_cast<float>(11.0 * M_PI / 6.0); // ~330°

            arc.addArc(cx - r, cy - r, r * 2.0f, r * 2.0f,
                       startA, endA, /*startAsNewSubPath=*/true);

            // Arrowhead at endA.
            const float tipX = cx + r * std::sin(endA);
            const float tipY = cy - r * std::cos(endA);

            // Counter-clockwise tangent at endA: rotate endA by -90°.
            const float tanA = endA - static_cast<float>(M_PI * 0.5);

            const float armA1 = tanA + static_cast<float>(M_PI * 0.6);
            const float armA2 = tanA - static_cast<float>(M_PI * 0.6);

            arc.startNewSubPath(tipX, tipY);
            arc.lineTo(tipX + arrowLen * std::sin(armA1),
                       tipY - arrowLen * std::cos(armA1));
            arc.startNewSubPath(tipX, tipY);
            arc.lineTo(tipX + arrowLen * std::sin(armA2),
                       tipY - arrowLen * std::cos(armA2));
        }

        g.strokePath(arc, juce::PathStrokeType(1.5f,
                                               juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    }

    //--------------------------------------------------------------------------
    // Gear icon — small circle + 6 outer circular bumps

    void paintGearIcon(juce::Graphics& g,
                       const juce::Rectangle<float>& bounds)
    {
        const bool isHov = (hoveredRegion_ == kRegSettings);
        const juce::Colour col = isHov
            ? juce::Colour(200, 204, 216).withAlpha(0.85f)
            : juce::Colour(200, 204, 216).withAlpha(0.55f);

        g.setColour(col);

        const float cx    = bounds.getCentreX();
        const float cy    = bounds.getCentreY();
        const float innerR = 3.5f;  // centre circle radius
        const float bumpR  = 2.0f;  // radius of each outer bump
        const float bumpOrbit = 7.5f; // distance from centre to bump centre

        // Centre circle
        g.drawEllipse(cx - innerR, cy - innerR,
                      innerR * 2.0f, innerR * 2.0f, 1.5f);

        // 6 outer bumps equally spaced
        for (int i = 0; i < 6; ++i)
        {
            const float angle = static_cast<float>(i) * static_cast<float>(M_PI / 3.0);
            const float bx    = cx + bumpOrbit * std::cos(angle);
            const float by    = cy + bumpOrbit * std::sin(angle);
            g.fillEllipse(bx - bumpR, by - bumpR, bumpR * 2.0f, bumpR * 2.0f);
        }
    }

    //--------------------------------------------------------------------------
    // Output slider

    void paintOutputSlider(juce::Graphics& g)
    {
        const float trackH = 3.0f;
        const float thumbD = 10.0f;

        const float trackX = sliderTrackBounds_.getX();
        const float trackY = sliderTrackBounds_.getY();
        const float trackW = sliderTrackBounds_.getWidth();

        // Thumb X position
        const float thumbX = trackX + outputLevel_ * trackW;

        // Track background
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.10f));
        g.fillRoundedRectangle(trackX, trackY, trackW, trackH, trackH * 0.5f);

        // Filled portion (left of thumb)
        if (outputLevel_ > 0.0f)
        {
            g.setColour(juce::Colour(60, 180, 170).withAlpha(0.50f));
            g.fillRoundedRectangle(trackX, trackY,
                                   thumbX - trackX, trackH,
                                   trackH * 0.5f);
        }

        // Thumb circle — centre on thumbX, vertically centred on track
        const float thumbY = trackY + trackH * 0.5f;
        g.setColour(juce::Colour(60, 180, 170).withAlpha(0.80f));
        g.fillEllipse(thumbX - thumbD * 0.5f, thumbY - thumbD * 0.5f,
                      thumbD, thumbD);
        g.setColour(juce::Colour(60, 180, 170).withAlpha(0.50f));
        g.drawEllipse(thumbX - thumbD * 0.5f, thumbY - thumbD * 0.5f,
                      thumbD, thumbD, 1.0f);
    }

    //==========================================================================
    // Mouse handling

    int hitRegion(const juce::MouseEvent& e) const
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        for (const auto& reg : regions_)
        {
            if (reg.bounds.expanded(2.0f).contains(mx, my))
                return reg.id;
        }
        return -1;
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const int id = hitRegion(e);

        switch (id)
        {
            case kRegEngines:
                if (onEnginesClicked)
                    onEnginesClicked();
                break;

            case kRegUndo:
                if (onUndo)
                    onUndo();
                break;

            case kRegRedo:
                if (onRedo)
                    onRedo();
                break;

            case kRegChain:
                chainModeActive_ = !chainModeActive_;
                repaint();
                if (onChainToggled)
                    onChainToggled();
                break;

            case kRegExport:
                if (onExportClicked)
                    onExportClicked();
                break;

            case kRegSlider:
                sliderDragging_   = true;
                sliderDragStartX_ = e.x;
                sliderDragStartV_ = outputLevel_;
                break;

            case kRegSettings:
                if (onSettingsClicked)
                    onSettingsClicked();
                break;

            default:
                break;
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!sliderDragging_)
            return;

        // Map horizontal delta to 0–1. Full slider width = full range.
        const float dx    = static_cast<float>(e.x - sliderDragStartX_);
        const float range = static_cast<float>(kSliderWidth);
        const float newV  = juce::jlimit(0.0f, 1.0f,
                                         sliderDragStartV_ + dx / range);
        outputLevel_ = newV;
        repaint();

        if (onOutputChanged)
            onOutputChanged(outputLevel_);
    }

    void mouseUp(const juce::MouseEvent& /*e*/) override
    {
        sliderDragging_ = false;
        repaint();
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const int id = hitRegion(e);

        if (id != hoveredRegion_)
        {
            hoveredRegion_ = id;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent& /*e*/) override
    {
        if (hoveredRegion_ != -1)
        {
            hoveredRegion_ = -1;
            repaint();
        }
    }

    //==========================================================================
    // State

    bool  chainModeActive_ = false;
    float outputLevel_     = 0.80f; // default 80% output

    // Slider drag state
    bool  sliderDragging_   = false;
    int   sliderDragStartX_ = 0;
    float sliderDragStartV_ = 0.0f;

    // Hover tracking
    int hoveredRegion_ = -1;

    // Layout rects — rebuilt each buildLayout() call.
    juce::Rectangle<float> enginesBounds_;
    juce::Rectangle<float> undoBounds_;
    juce::Rectangle<float> redoBounds_;
    juce::Rectangle<float> chainBounds_;
    juce::Rectangle<float> exportBounds_;
    juce::Rectangle<float> outputLabelBounds_;
    juce::Rectangle<float> sliderBounds_;
    juce::Rectangle<float> sliderTrackBounds_;
    juce::Rectangle<float> settingsBounds_;

    // Hit-test regions
    std::vector<HudRegion> regions_;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubmarineHudBar)
};

} // namespace xoceanus
