// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// OceanDetailHeader.h — Submarine-native header for the engine detail overlay.
//
// Implements the breadcrumb nav strip + engine identity row that appears at the
// top of the detail panel when an engine is double-clicked in Ocean View.
//
// Spec reference:
//   Tools/ui-preview/submarine.html lines 150–175 (detail-panel), 1332–1354 (breadcrumb),
//   2494–2513 (detail-header HTML structure).
//
// Visual language:
//   • NO Gallery code — no GalleryColors, no GalleryFonts, no GalleryLookAndFeel.
//   • All colours are expressed as rgba() constants matching the submarine palette.
//   • Panel background: rgba(20,23,32,0.97) — NOT opaque Gallery shell white.
//   • Teal accent: rgba(60,180,170) / rgba(127,219,202) — NOT GalleryColors::xoGold.
//   • Typography: JetBrains Mono for values, Space Grotesk-equivalent for labels.
//
// Layout (top → bottom, fixed height):
//
//   ┌──────────────────────────────────────────────────────────┐  ← kHeaderHeight
//   │ [breadcrumb row]  Ocean › Engine Name          9 px high │
//   ├─────────────────────── hairline ────────────────────────┤
//   │ [← back] [icon swatch] [Engine Name]  [Engine Type]    32px │
//   └──────────────────────────────────────────────────────────┘
//
// All interactions use std::function callbacks — no upward include dependency.
//
// Clean-slate mandate: issue #1098, design decision #1176.
// This component must NEVER include Gallery headers.

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <cmath>

namespace xoceanus
{

//==============================================================================
/**
 * OceanDetailHeader
 *
 * Submarine-native breadcrumb + engine identity row for the detail overlay.
 * Self-contained; no Gallery dependencies.
 *
 * Usage:
 *   OceanDetailHeader header;
 *   header.setEngineInfo("Obrix", "Modular Synthesis", accentColour);
 *   header.onBackClicked = [this]() { dismissDetail(); };
 *   addAndMakeVisible(header);
 *   header.setBounds(bounds.withHeight(OceanDetailHeader::kHeaderHeight));
 */
class OceanDetailHeader : public juce::Component
{
public:
    //==========================================================================
    // Fixed height consumed by this component
    static constexpr int kHeaderHeight    = 56; ///< total height (breadcrumb 16 + divider 1 + identity 39)
    static constexpr int kBreadcrumbHeight = 17;
    static constexpr int kIdentityHeight   = kHeaderHeight - kBreadcrumbHeight;

    OceanDetailHeader()
    {
        setInterceptsMouseClicks(true, true);

        // Back arrow button — 32×32, frosted-glass pill style
        backBtn_.setButtonText({});
        backBtn_.setTooltip("Back to Ocean view");
        addAndMakeVisible(backBtn_);

        backBtn_.onClick = [this]()
        {
            if (onBackClicked) onBackClicked();
        };
    }

    //==========================================================================
    // Public API

    /** Update the displayed engine name, type label, and accent colour swatch. */
    void setEngineInfo(const juce::String& engineName,
                       const juce::String& engineType,
                       juce::Colour accentColour)
    {
        engineName_  = engineName;
        engineType_  = engineType;
        accentColour_ = accentColour;
        repaint();
    }

    /** Callback — fired when the back arrow or breadcrumb "Ocean" item is clicked. */
    std::function<void()> onBackClicked;

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds();

        // ── Breadcrumb row ────────────────────────────────────────────────────
        {
            auto bc = bounds.withHeight(kBreadcrumbHeight);

            // Row background — very subtle teal tint
            g.setColour(juce::Colour(0x05'3C'B4'AA)); // rgba(60,180,170,0.02)
            g.fillRect(bc);

            // Hairline at bottom of breadcrumb row
            g.setColour(juce::Colour(0x0D'3C'B4'AA)); // rgba(60,180,170,0.05)
            g.drawHorizontalLine(bc.getBottom() - 1, (float)bc.getX(), (float)bc.getRight());

            // Text: "Ocean ›  EngineName"
            // "Ocean" — teal-tinted clickable crumb
            const juce::Colour crumbActive  { 0x80'7F'DB'CA }; // rgba(127,219,202,0.50)
            const juce::Colour crumbSep     { 0x26'C8'CC'D8 }; // rgba(200,204,216,0.15)
            const juce::Colour crumbPassive { 0x4D'C8'CC'D8 }; // rgba(200,204,216,0.30)

            const juce::Font bcFont = juce::Font(juce::FontOptions().withName("JetBrains Mono")
                                                                     .withHeight(9.0f));

            int x = bc.getX() + 16;
            const int cy = bc.getCentreY();

            // "Ocean" crumb (clickable style)
            g.setFont(bcFont);
            g.setColour(crumbActive);
            const juce::String oceanCrumb { "Ocean" };
            const int oceanW = bcFont.getStringWidth(oceanCrumb) + 2;
            g.drawText(oceanCrumb, x, bc.getY(), oceanW, bc.getHeight(),
                       juce::Justification::centredLeft, false);
            x += oceanW;

            // Separator "›"
            g.setColour(crumbSep);
            const juce::String sep { "  \xe2\x80\xba  " }; // UTF-8 for ›
            const int sepW = bcFont.getStringWidth(sep);
            g.drawText(sep, x, bc.getY(), sepW, bc.getHeight(),
                       juce::Justification::centredLeft, false);
            x += sepW;

            // Engine name crumb (passive — current location)
            g.setColour(crumbPassive);
            g.drawText(engineName_, x, bc.getY(), bc.getRight() - x - 8, bc.getHeight(),
                       juce::Justification::centredLeft, true);

            (void)cy; // layout ref, used implicitly
        }

        // ── Identity row ─────────────────────────────────────────────────────
        {
            auto id = bounds.withTrimmedTop(kBreadcrumbHeight);

            // Engine colour swatch — 20×20 rounded rect
            const int swatchSize = 20;
            const int swatchX    = backBtn_.getRight() + 10;
            const int swatchY    = id.getCentreY() - swatchSize / 2;
            const juce::Rectangle<float> swatch {
                (float)swatchX, (float)swatchY, (float)swatchSize, (float)swatchSize
            };
            g.setColour(accentColour_.withAlpha(0.85f));
            g.fillRoundedRectangle(swatch, 5.0f);
            g.setColour(accentColour_.brighter(0.3f).withAlpha(0.4f));
            g.drawRoundedRectangle(swatch, 5.0f, 1.0f);

            // Engine name — 14px, foam white
            const juce::Font nameFont = juce::Font(
                juce::FontOptions().withName("Space Grotesk").withHeight(14.0f)
            );
            const juce::Colour foamCol { 0xFFE8E4DF }; // GalleryColors::Ocean::foam, inlined
            const juce::Colour saltCol { 0xFF9E9B97 }; // secondary text

            const int textX = swatchX + swatchSize + 10;
            const int textW = id.getRight() - textX - 16;

            g.setFont(nameFont);
            g.setColour(foamCol);
            g.drawText(engineName_, textX, id.getY() + 2, textW, 18,
                       juce::Justification::centredLeft, true);

            // Engine type — 10px, salt colour
            const juce::Font typeFont = juce::Font(
                juce::FontOptions().withName("Space Grotesk").withHeight(10.0f)
            );
            g.setFont(typeFont);
            g.setColour(saltCol);
            g.drawText(engineType_, textX, id.getY() + 20, textW, 14,
                       juce::Justification::centredLeft, true);
        }
    }

    void resized() override
    {
        const auto id = getLocalBounds().withTrimmedTop(kBreadcrumbHeight);

        // Back button — 30×30, left-aligned in identity row
        backBtn_.setBounds(id.getX() + 12,
                           id.getCentreY() - 15,
                           30, 30);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Breadcrumb "Ocean" hit region
        if (e.position.getY() < kBreadcrumbHeight)
        {
            // Approximate hit: first ~50px is the "Ocean" crumb
            if (e.position.getX() < 70)
            {
                if (onBackClicked) onBackClicked();
            }
        }
    }

private:
    //==========================================================================
    // Back arrow button — draws a left-pointing chevron
    struct BackArrowButton : public juce::Button
    {
        BackArrowButton() : juce::Button("back") {}

        void paintButton(juce::Graphics& g, bool isHighlighted, bool isDown) override
        {
            const auto b = getLocalBounds().toFloat().reduced(1.0f);

            // Frosted-glass pill
            const float alpha = isDown ? 0.18f : (isHighlighted ? 0.10f : 0.05f);
            g.setColour(juce::Colour(0xFFE8E4DF).withAlpha(alpha));
            g.fillRoundedRectangle(b, 7.0f);

            const float borderAlpha = isHighlighted ? 0.22f : 0.08f;
            g.setColour(juce::Colour(0xFFE8E4DF).withAlpha(borderAlpha));
            g.drawRoundedRectangle(b, 7.0f, 1.0f);

            // ← chevron
            const float cx = b.getCentreX();
            const float cy = b.getCentreY();
            const float w  = 7.0f;
            const float h  = 5.0f;
            juce::Path arrow;
            arrow.startNewSubPath(cx + w * 0.5f, cy - h * 0.5f);
            arrow.lineTo(cx - w * 0.5f, cy);
            arrow.lineTo(cx + w * 0.5f, cy + h * 0.5f);

            const float arrowAlpha = isDown ? 1.0f : (isHighlighted ? 0.90f : 0.60f);
            g.setColour(juce::Colour(0xFF7FDBCA).withAlpha(arrowAlpha)); // teal
            g.strokePath(arrow, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }
    };

    BackArrowButton backBtn_;
    juce::String    engineName_ { "—" };
    juce::String    engineType_ { "" };
    juce::Colour    accentColour_ { 0xFF3CB4AA }; // default teal

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OceanDetailHeader)
};

} // namespace xoceanus
