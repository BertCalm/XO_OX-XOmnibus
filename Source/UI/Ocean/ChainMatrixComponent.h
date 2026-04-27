// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// ChainMatrixComponent.h — Wave 5 C4: chain matrix slide-up panel.
//
// 4×4 grid (rows = source slots, columns = destination slots) showing all
// inter-slot chain links. Each cell carries 4 per-type toggle indicators
// (T=Trigger, M=Mod, P=Pattern, C=Clock/Tempo) rendered as small pills.
//
// Opening: OceanView calls show()/hide(), which calls PanelCoordinator
// requestOpen/release(PanelType::ChainMatrix). The panel slides up from
// the waterline boundary using a juce::ComponentAnimator.
//
// Styling: AccentColors chain family (teal/electric) on deep Ocean background.
// Grid rows = source slot (what triggers), columns = dest slot (what receives).
// Self-cells (diagonal) are rendered as X marks (disabled, no-op clicks).
//
// Callback: onMatrixChanged fires on the message thread whenever a link is
// toggled. OceanView wires this to refreshOrbitBadges() + processor state save.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/ChainMatrix.h"
#include "../AccentColors.h"
#include "../GalleryColors.h"  // also provides GalleryFonts namespace

namespace XOceanus
{

//==============================================================================
/**
    ChainMatrixComponent — slide-up panel with the 4×4 chain link grid.

    Each grid cell represents one (source, dest) slot pair. Inside each cell,
    4 type pills are rendered horizontally. Clicking a pill toggles that link.
    The diagonal (src==dst) is always disabled (shown as a dark cross).

    The panel closes when the user clicks the X button or clicks outside the
    panel area (via the parent OceanView hit-test escape path).
*/
class ChainMatrixComponent : public juce::Component
{
public:
    //==========================================================================
    /** Fired on the message thread when any link is toggled.
        First arg = slot index (0-3) whose count badge needs refresh. */
    std::function<void()> onMatrixChanged;

    //==========================================================================
    ChainMatrixComponent()
    {
        // Close button (top-right)
        closeBtn_.setButtonText("X");
        closeBtn_.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        closeBtn_.setColour(juce::TextButton::textColourOffId,
                            AccentColors::chainAccent.withAlpha(0.7f));
        closeBtn_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        closeBtn_.onClick = [this]() { if (onCloseClicked) onCloseClicked(); };
        addAndMakeVisible(closeBtn_);

        setOpaque(false);
    }

    ~ChainMatrixComponent() override = default;

    //==========================================================================
    /** Callback: fired when the close button is clicked (OceanView wires this). */
    std::function<void()> onCloseClicked;

    //==========================================================================
    /** Replace the live ChainMatrix reference.  Pointer must outlive this component.
        Called by OceanView after constructing ChainMatrix inside the processor. */
    void setMatrix(ChainMatrix* matrix) noexcept { matrix_ = matrix; repaint(); }

    /** Refresh display after external matrix change (e.g. setStateInformation). */
    void refresh() { repaint(); }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Background panel: dark ocean glass with teal border
        const juce::Colour panelBg  = juce::Colour(0xFF0A0E18).withAlpha(0.96f);
        const juce::Colour borderCol = AccentColors::chainPrimary.withAlpha(0.55f);

        g.setColour(panelBg);
        g.fillRoundedRectangle(bounds, 10.0f);
        g.setColour(borderCol);
        g.drawRoundedRectangle(bounds.reduced(0.5f), 10.0f, 1.5f);

        // Title
        g.setFont(xoceanus::GalleryFonts::label(13.0f).boldened());
        g.setColour(AccentColors::chainBright);
        g.drawText("Chain Matrix",
                   juce::Rectangle<float>(kPad, kTitleY, bounds.getWidth() - 2 * kPad, kTitleH),
                   juce::Justification::centredLeft, false);

        // Grid
        if (matrix_ == nullptr) return;

        const float cellW  = (bounds.getWidth()  - 2.0f * kPad - kLabelW) / static_cast<float>(kSlots);
        const float cellH  = (kGridH) / static_cast<float>(kSlots);
        const float gridX0 = kPad + kLabelW;
        const float gridY0 = kTitleY + kTitleH + kSpacing;

        // Column header labels (destination slots)
        g.setFont(xoceanus::GalleryFonts::label(9.0f));
        g.setColour(AccentColors::chainAccent.withAlpha(0.8f));
        for (int dst = 0; dst < kSlots; ++dst)
        {
            const float cx = gridX0 + dst * cellW + cellW * 0.5f;
            g.drawText("S" + juce::String(dst + 1) + " DST",
                       juce::Rectangle<float>(cx - cellW * 0.5f, gridY0 - 14.0f, cellW, 12.0f).toNearestInt(),
                       juce::Justification::centred, false);
        }

        // Row header labels (source slots)
        g.setFont(xoceanus::GalleryFonts::label(9.0f));
        g.setColour(AccentColors::chainAccent.withAlpha(0.8f));
        for (int src = 0; src < kSlots; ++src)
        {
            const float ry = gridY0 + src * cellH + cellH * 0.5f;
            g.drawText("S" + juce::String(src + 1),
                       juce::Rectangle<float>(kPad, ry - 7.0f, kLabelW - 4.0f, 14.0f).toNearestInt(),
                       juce::Justification::centredRight, false);
        }

        // Grid cells
        for (int src = 0; src < kSlots; ++src)
        {
            for (int dst = 0; dst < kSlots; ++dst)
            {
                const juce::Rectangle<float> cellRect {
                    gridX0 + dst * cellW + 2.0f,
                    gridY0 + src * cellH + 2.0f,
                    cellW  - 4.0f,
                    cellH  - 4.0f
                };

                if (src == dst)
                {
                    // Diagonal: disabled cross
                    g.setColour(juce::Colour(0xFF1A2030));
                    g.fillRoundedRectangle(cellRect, 4.0f);
                    g.setColour(AccentColors::chainDim.withAlpha(0.40f));
                    const float cx = cellRect.getCentreX();
                    const float cy = cellRect.getCentreY();
                    const float hw = 6.0f;
                    g.drawLine(cx - hw, cy - hw, cx + hw, cy + hw, 1.5f);
                    g.drawLine(cx - hw, cy + hw, cx + hw, cy - hw, 1.5f);
                    continue;
                }

                // Normal cell background
                g.setColour(juce::Colour(0xFF111824));
                g.fillRoundedRectangle(cellRect, 4.0f);
                g.setColour(AccentColors::chainDim.withAlpha(0.25f));
                g.drawRoundedRectangle(cellRect.reduced(0.5f), 4.0f, 0.75f);

                // 4 type pills — horizontal row inside cell
                const float pillW   = (cellRect.getWidth()  - 6.0f) / 4.0f;
                const float pillH   = juce::jmin(14.0f, cellRect.getHeight() - 6.0f);
                const float pillY   = cellRect.getCentreY() - pillH * 0.5f;
                const float pillX0  = cellRect.getX() + 3.0f;

                for (int ti = 0; ti < ChainMatrix::kNumTypes; ++ti)
                {
                    const auto type   = static_cast<ChainType>(ti);
                    const bool active = matrix_->isActive(src, dst, type);
                    const juce::Rectangle<float> pill {
                        pillX0 + ti * pillW + 0.5f,
                        pillY,
                        pillW - 1.0f,
                        pillH
                    };

                    if (active)
                    {
                        g.setColour(pillColour(type).withAlpha(0.88f));
                        g.fillRoundedRectangle(pill, 3.0f);
                        g.setFont(xoceanus::GalleryFonts::label(7.5f).boldened());
                        g.setColour(juce::Colour(0xFF050810));
                        g.drawText(chainTypeLabel(type), pill.toNearestInt(),
                                   juce::Justification::centred, false);
                    }
                    else
                    {
                        g.setColour(AccentColors::chainDim.withAlpha(0.30f));
                        g.drawRoundedRectangle(pill.reduced(0.25f), 3.0f, 0.75f);
                        g.setFont(xoceanus::GalleryFonts::label(7.0f));
                        g.setColour(AccentColors::chainDim.withAlpha(0.55f));
                        g.drawText(chainTypeLabel(type), pill.toNearestInt(),
                                   juce::Justification::centred, false);
                    }
                }
            }
        }

        // Legend row (below grid)
        const float legendY = gridY0 + kSlots * cellH + kSpacing;
        g.setFont(xoceanus::GalleryFonts::label(8.5f));
        const char* legends[] = { "T=Trigger", "M=Mod", "P=Pattern", "C=Clock" };
        const float lw = (bounds.getWidth() - 2.0f * kPad) / 4.0f;
        for (int i = 0; i < 4; ++i)
        {
            const auto type = static_cast<ChainType>(i);
            g.setColour(pillColour(type).withAlpha(0.75f));
            g.drawText(legends[i],
                       juce::Rectangle<float>(kPad + i * lw, legendY, lw, 12.0f).toNearestInt(),
                       juce::Justification::centred, false);
        }
    }

    void resized() override
    {
        const auto bounds = getLocalBounds();
        const int bx = bounds.getRight() - kBtnSize - kPad;
        closeBtn_.setBounds(bx, kPad, kBtnSize, kBtnSize);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (matrix_ == nullptr) return;

        const float cellW  = (getWidth()  - 2.0f * kPad - kLabelW) / static_cast<float>(kSlots);
        const float cellH  = kGridH / static_cast<float>(kSlots);
        const float gridX0 = kPad + kLabelW;
        const float gridY0 = kTitleY + kTitleH + kSpacing;

        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        // Hit-test: is the click inside the grid?
        if (mx < gridX0 || my < gridY0 || my > gridY0 + kSlots * cellH) return;

        const int dst = static_cast<int>((mx - gridX0) / cellW);
        const int src = static_cast<int>((my - gridY0) / cellH);

        if (dst < 0 || dst >= kSlots || src < 0 || src >= kSlots) return;
        if (src == dst) return; // diagonal disabled

        // Which pill within the cell?
        const float cellRect_x = gridX0 + dst * cellW + 2.0f;
        const float pillW      = ((cellW - 4.0f) - 6.0f) / 4.0f;
        const float pillX0     = cellRect_x + 3.0f;
        const int   typeIdx    = static_cast<int>((mx - pillX0) / pillW);
        if (typeIdx < 0 || typeIdx >= ChainMatrix::kNumTypes) return;

        matrix_->toggle(src, dst, static_cast<ChainType>(typeIdx));
        repaint();
        if (onMatrixChanged) onMatrixChanged();
    }

private:
    //==========================================================================
    static juce::Colour pillColour(ChainType type) noexcept
    {
        switch (type)
        {
            case ChainType::Trigger: return AccentColors::chainAccent;
            case ChainType::Mod:     return juce::Colour::fromRGB(0x90, 0xD4, 0xFF);  // icy blue
            case ChainType::Pattern: return juce::Colour::fromRGB(0xB0, 0xFF, 0xA0);  // lime green
            case ChainType::Tempo:   return juce::Colour::fromRGB(0xFF, 0xD4, 0x60);  // warm gold
            default:                 return AccentColors::chainBright;
        }
    }

    //==========================================================================
    // Layout constants
    static constexpr float kPad     = 12.0f;
    static constexpr float kLabelW  = 32.0f;
    static constexpr float kTitleY  = 12.0f;
    static constexpr float kTitleH  = 20.0f;
    static constexpr float kSpacing = 6.0f;
    static constexpr float kGridH   = 160.0f;  // total grid height
    static constexpr int   kSlots   = ChainMatrix::kNumSlots;
    static constexpr int   kBtnSize = 20;

    //==========================================================================
    ChainMatrix*    matrix_   { nullptr };
    juce::TextButton closeBtn_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainMatrixComponent)
};

} // namespace XOceanus
