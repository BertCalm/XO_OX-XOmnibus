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
//   [ ☰ Engines ]  [ ↶ ]  [ ↷ ]  ————————spacer————————  [ Chain ]  [ Export ]  REACT [dial]  [ ⚙ ]
//
// Button styles follow the submarine frosted-glass pill aesthetic:
//   • Default : bg rgba(14,16,22,0.70), border rgba(200,204,216,0.10), text rgba(200,204,216,0.55)
//   • Hover   : bg rgba(200,204,216,0.08), border rgba(200,204,216,0.18), text rgba(200,204,216,0.85)
//   • Active  : bg rgba(60,180,170,0.10),  border rgba(60,180,170,0.35),  text rgba(60,180,170,0.90)
//
// Icon buttons (Undo, Redo, Settings) are 32×32 px squares with custom Path geometry:
//   • Undo    — counterclockwise arc with left-pointing arrowhead
//   • Redo    — clockwise arc with right-pointing arrowhead
//   • Settings — standard gear: 8 evenly-spaced trapezoidal teeth around a centre hole
//
// The Chain button prepends a small chain-link icon (two interlocking ovals).
//
// The REACT dial is a 28×28 px rotary knob (same style as the SAT/DELAY/REVERB dials
// in MasterFXStripCompact): value arc from 135° to 405°, teal fill, indicator line.
// Vertical drag changes the normalised value (0–1). Drag up = increase.
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
    static constexpr int kDialSize     = 28;   // REACT rotary dial diameter

    //==========================================================================
    // Callbacks — parent wires these in the editor constructor.

    std::function<void()>      onEnginesClicked;
    std::function<void()>      onUndo;
    std::function<void()>      onRedo;
    std::function<void()>      onChainToggled;   // toggles chain mode; check chainModeActive_ to read new state
    std::function<void()>      onExportClicked;
    std::function<void()>      onSettingsClicked;
    std::function<void(float)> onReactChanged;   // 0–1 normalised — ocean visual reactivity

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

    void setReactLevel(float level01)
    {
        const float clamped = juce::jlimit(0.0f, 1.0f, level01);
        if (reactLevel_ != clamped)
        {
            reactLevel_ = clamped;
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
        kRegDial     = 5,   // REACT rotary dial
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

        // --- REACT dial (28×28 rotary knob) ---
        {
            const float d = static_cast<float>(kDialSize);
            juce::Rectangle<float> dialRect(rx - d, (h - d) * 0.5f, d, d);
            regions_.push_back({ dialRect.expanded(4.0f), kRegDial });
            reactDialBounds_ = dialRect;
            rx -= d + gap;
        }

        // --- "REACT" label (9 px, uppercase) ---
        {
            const float labelW = 36.0f;
            reactLabelBounds_ = juce::Rectangle<float>(
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

        // --- REACT label ---
        {
            static const juce::Font reactLabelFont(juce::FontOptions{}
                          .withName(juce::Font::getDefaultSansSerifFontName())
                          .withHeight(9.0f));
            g.setFont(reactLabelFont);
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.25f));
            g.drawText("REACT", reactLabelBounds_.toNearestInt(),
                       juce::Justification::centredRight, false);
        }

        // --- REACT dial ---
        paintReactDial(g);

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
    // Gear icon — standard gear: 8 evenly-spaced trapezoidal teeth around a
    // filled centre circle with a concentric hole (recognizable at small sizes).

    void paintGearIcon(juce::Graphics& g,
                       const juce::Rectangle<float>& bounds)
    {
        const bool isHov = (hoveredRegion_ == kRegSettings);
        const juce::Colour col = isHov
            ? juce::Colour(200, 204, 216).withAlpha(0.85f)
            : juce::Colour(200, 204, 216).withAlpha(0.55f);

        g.setColour(col);

        const float cx      = bounds.getCentreX();
        const float cy      = bounds.getCentreY();
        const int   nTeeth  = 8;
        const float outerR  = 8.0f;   // tip of teeth
        const float innerR  = 5.5f;   // root of teeth (body radius)
        const float holeR   = 2.8f;   // centre hole radius
        const float halfTooth = static_cast<float>(M_PI) / static_cast<float>(nTeeth) * 0.55f; // half-angular width of each tooth tip

        juce::Path gear;

        // Build gear outline by alternating between tooth tips and body valleys.
        for (int i = 0; i < nTeeth * 2; ++i)
        {
            const float baseAngle = static_cast<float>(i) * static_cast<float>(M_PI) / static_cast<float>(nTeeth);
            const bool  isTip     = (i % 2 == 0);
            const float r         = isTip ? outerR : innerR;

            if (isTip)
            {
                // Tooth: two points at (baseAngle ± halfTooth) at outerR
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
                // Valley: single point at innerR
                const float x = cx + r * std::sin(baseAngle);
                const float y = cy - r * std::cos(baseAngle);
                gear.lineTo(x, y);
            }
        }
        gear.closeSubPath();

        g.fillPath(gear);

        // Punch centre hole (overdraw with background colour).
        // Use the component's background colour — frosted dark.
        g.setColour(juce::Colour(14, 16, 22));
        g.fillEllipse(cx - holeR, cy - holeR, holeR * 2.0f, holeR * 2.0f);
    }

    //--------------------------------------------------------------------------
    // REACT rotary dial — same style as SAT/DELAY/REVERB dials in MasterFXStripCompact

    void paintReactDial(juce::Graphics& g)
    {
        using juce::MathConstants;
        using juce::Colour;
        using juce::Path;
        using juce::PathStrokeType;

        const bool isHov = (hoveredRegion_ == kRegDial);

        const float cx = reactDialBounds_.getCentreX();
        const float cy = reactDialBounds_.getCentreY();
        const float r  = (reactDialBounds_.getWidth() * 0.5f) - 2.0f;

        const float startAngle = 0.75f * MathConstants<float>::pi;  // 135°
        const float endAngle   = 2.25f * MathConstants<float>::pi;  // 405°
        const float totalSweep = endAngle - startAngle;             // 270°
        const float valueAngle = startAngle + reactLevel_ * totalSweep;

        // Background arc (full sweep).
        {
            Path bgArc;
            bgArc.addCentredArc(cx, cy, r, r, 0.0f, startAngle, endAngle, true);
            g.setColour(Colour(200, 204, 216).withAlpha(0.08f));
            g.strokePath(bgArc, PathStrokeType(3.0f,
                PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Value arc (partial sweep).
        if (reactLevel_ > 0.01f)
        {
            Path valArc;
            valArc.addCentredArc(cx, cy, r, r, 0.0f, startAngle, valueAngle, true);
            g.setColour(Colour(127, 219, 202).withAlpha(0.60f));
            g.strokePath(valArc, PathStrokeType(3.0f,
                PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Centre dot.
        const float dotR = r * 0.35f;
        g.setColour(Colour(200, 204, 216).withAlpha(0.06f));
        g.fillEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
        g.setColour(Colour(200, 204, 216).withAlpha(0.10f));
        g.drawEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f, 1.0f);

        // Indicator line — brighter on hover.
        const float indStart = r * 0.25f;
        const float indEnd   = r * 0.75f;
        const float indAlpha = isHov ? 1.0f : 0.80f;
        const float cosA     = std::cos(valueAngle);
        const float sinA     = std::sin(valueAngle);
        g.setColour(Colour(127, 219, 202).withAlpha(indAlpha));
        g.drawLine(cx + indStart * cosA, cy + indStart * sinA,
                   cx + indEnd   * cosA, cy + indEnd   * sinA,
                   1.5f);
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

            case kRegDial:
                dialDragging_   = true;
                dialDragStartY_ = e.y;
                dialDragStartV_ = reactLevel_;
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
        if (!dialDragging_)
            return;

        // Map vertical delta to 0–1. Drag up = increase. 100 px = full sweep.
        const float dy    = static_cast<float>(dialDragStartY_ - e.y);
        const float range = 100.0f;
        const float newV  = juce::jlimit(0.0f, 1.0f,
                                         dialDragStartV_ + dy / range);
        reactLevel_ = newV;
        repaint();

        if (onReactChanged)
            onReactChanged(reactLevel_);
    }

    void mouseUp(const juce::MouseEvent& /*e*/) override
    {
        dialDragging_ = false;
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
    float reactLevel_      = 0.80f; // default 80% reactivity

    // Dial drag state (REACT rotary)
    bool  dialDragging_   = false;
    int   dialDragStartY_ = 0;
    float dialDragStartV_ = 0.0f;

    // Hover tracking
    int hoveredRegion_ = -1;

    // Layout rects — rebuilt each buildLayout() call.
    juce::Rectangle<float> enginesBounds_;
    juce::Rectangle<float> undoBounds_;
    juce::Rectangle<float> redoBounds_;
    juce::Rectangle<float> chainBounds_;
    juce::Rectangle<float> exportBounds_;
    juce::Rectangle<float> reactLabelBounds_;
    juce::Rectangle<float> reactDialBounds_;
    juce::Rectangle<float> settingsBounds_;

    // Hit-test regions
    std::vector<HudRegion> regions_;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubmarineHudBar)
};

} // namespace xoceanus
