// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// ChainMatrix.h — 5×5 coupling matrix editor (Wave 5 C4, Issue #1428)
//
// A slide-up drawer from the bottom of the ocean viewport showing the full
// MegaCouplingMatrix as an interactive 5×5 grid.  Rows = source slots (0-4),
// columns = destination slots (0-4).
//
//   Empty cell   → click to open 15-type popup; picks type → addRoute(src,dst,type,0.5)
//   Filled cell  → click to open CouplingConfigPopup in a CallOutBox
//   Diagonal     → disabled (slot cannot couple to itself)
//
// Usage (from OceanView):
//
//   // Declare as member:
//   xoceanus::ChainMatrix chainMatrix_;
//
//   // Wire callbacks BEFORE addChildComponent:
//   chainMatrix_.onAddRoute = [this](int src, int dst, CouplingType type) { ... };
//   chainMatrix_.onEditRoute = [this](int src, int dst, int routeIdx) { ... };
//   chainMatrix_.onCloseRequested = [this]() { coordinatorRelease(PanelType::ChainMatrix); };
//
//   // Add as child — starts hidden:
//   addChildComponent(chainMatrix_);
//
//   // In resized():
//   chainMatrix_.setBounds(getLocalBounds().removeFromBottom(getHeight() / 2));
//
//   // Open/close:
//   chainMatrix_.open();
//   chainMatrix_.close();
//
// Thread safety: all methods must be called on the message thread.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/MegaCouplingMatrix.h"
#include "../../Core/SynthEngine.h"
#include "../GalleryColors.h"
#include "../Tokens.h"
#include <functional>
#include <vector>
#include <string>
#include <cmath>

namespace xoceanus
{

//==============================================================================
/**
    ChainMatrix

    Slide-up drawer showing the 5×5 cross-engine coupling matrix as an
    interactive grid.  One heavy panel in the PanelCoordinator system.

    See file header for usage and design notes.
*/
class ChainMatrix : public juce::Component,
                    public juce::Timer
{
public:
    //==========================================================================
    // Public API

    /** Fired when the user selects a coupling type for an empty cell.
        src/dst  = slot indices (0-4)
        type     = selected coupling type
        Parent calls MegaCouplingMatrix::addRoute() then calls refreshRoutes(). */
    std::function<void(int src, int dst, CouplingType type)> onAddRoute;

    /** Fired when the user clicks a filled cell to edit an existing route.
        src/dst     = slot indices
        routeIndex  = index into MCM route list (from findRoute or position)
        Parent should open CouplingConfigPopup via CallOutBox. */
    std::function<void(int src, int dst, int routeIndex,
                       juce::Rectangle<int> cellScreenBounds)> onEditRoute;

    /** Fired when the drawer wants to close (Esc, backdrop click, MATRIX toggle). */
    std::function<void()> onCloseRequested;

    //==========================================================================
    ChainMatrix()
    {
        setInterceptsMouseClicks(true, true);
        setWantsKeyboardFocus(true);
    }

    ~ChainMatrix() override { stopTimer(); }

    //==========================================================================
    // Open / close with 250 ms ease-out slide-up animation

    void open()
    {
        if (animState_ == AnimState::Open || animState_ == AnimState::Opening)
            return;

        setVisible(true);
        toFront(false);
        animState_    = AnimState::Opening;
        animProgress_ = 0.0f;
        startTimerHz(30);
        grabKeyboardFocus();
    }

    void close()
    {
        if (animState_ == AnimState::Closed || animState_ == AnimState::Closing)
            return;

        animState_    = AnimState::Closing;
        animProgress_ = 1.0f;
        startTimerHz(30);
    }

    bool isOpen() const noexcept
    {
        return animState_ == AnimState::Open || animState_ == AnimState::Opening;
    }

    void toggle() { isOpen() ? close() : open(); }

    //==========================================================================
    // Route data refresh — call after any MCM mutation (addRoute/removeRoute etc.)

    void refreshRoutes(const std::vector<MegaCouplingMatrix::CouplingRoute>& routes)
    {
        routes_ = routes;
        repaint();
    }

    //==========================================================================
    // juce::Component overrides

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour(juce::Colour(GalleryColors::Ocean::twilight).withAlpha(0.96f));
        g.fillRoundedRectangle(bounds.withTrimmedBottom(0.0f), 12.0f);

        // Header bar
        auto headerBounds = bounds.removeFromTop(static_cast<float>(kHeaderH));
        g.setColour(juce::Colour(GalleryColors::Ocean::shallow).withAlpha(0.80f));
        g.fillRoundedRectangle(headerBounds, 12.0f);
        g.fillRect(headerBounds.withTrimmedTop(6.0f)); // square the bottom corners

        // Header label
        g.setFont(XO::Tokens::Type::heading(XO::Tokens::Type::HeadingLarge));
        g.setColour(juce::Colour(GalleryColors::Ocean::foam));
        g.drawText("COUPLING MATRIX", headerBounds.toNearestInt(),
                   juce::Justification::centred, false);

        // Close × button
        paintCloseButton(g, headerBounds);

        // Dim overlay hint text when no routes
        const bool hasUserRoutes = !routes_.empty();

        // Grid area
        paintGrid(g, bounds);

        if (!hasUserRoutes)
        {
            g.setFont(XO::Tokens::Type::body(XO::Tokens::Type::BodyDefault));
            g.setColour(juce::Colour(GalleryColors::Ocean::salt).withAlpha(0.50f));
            g.drawText("Click an empty cell to add a coupling route",
                       bounds.toNearestInt(), juce::Justification::centred, false);
        }
    }

    void resized() override
    {
        buildLayout();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Close button
        if (closeBtnBounds_.contains(e.position.toFloat()))
        {
            if (onCloseRequested)
                onCloseRequested();
            return;
        }

        // Grid cell hit-test
        const auto [col, row] = hitTestGrid(e.position.toFloat());
        if (col < 0 || row < 0)
            return;

        // Diagonal — disabled
        if (col == row)
            return;

        // Check for existing route on this (row=src, col=dst) pair
        const int routeIdx = findFirstRouteIndex(row, col);

        if (routeIdx >= 0)
        {
            // Edit existing route
            if (onEditRoute)
            {
                const auto cellRect = getCellBounds(row, col);
                onEditRoute(row, col, routeIdx, cellRect.toNearestInt()
                                                         .translated(getScreenX(), getScreenY()));
            }
        }
        else
        {
            // Add new route — show 15-type popup
            showTypePopup(row, col);
        }
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            if (onCloseRequested)
                onCloseRequested();
            return true;
        }
        return false;
    }

    // juce::Timer override — animation ticks at 30 Hz
    void timerCallback() override
    {
        const float step = XO::Tokens::Motion::EaseOutStep30Hz;

        if (animState_ == AnimState::Opening)
        {
            animProgress_ += step * (1.0f - animProgress_) + step * 0.05f;
            if (animProgress_ >= 0.99f)
            {
                animProgress_ = 1.0f;
                animState_    = AnimState::Open;
                stopTimer();
            }
        }
        else if (animState_ == AnimState::Closing)
        {
            animProgress_ -= step * animProgress_ + step * 0.05f;
            if (animProgress_ <= 0.01f)
            {
                animProgress_ = 0.0f;
                animState_    = AnimState::Closed;
                stopTimer();
                setVisible(false);
                return;
            }
        }

        // Slide position: fully hidden = fully below parent bottom edge
        // Animate the Y offset so the drawer slides up from the bottom.
        updatePosition();
        repaint();
    }

    //==========================================================================
    // Position helper — call from timerCallback() and from parent's resized()
    // after calling setBounds() so the initial position is correct.

    void updatePosition()
    {
        // The parent calls setBounds() with the FULLY OPEN target bounds.
        // We shift the component down by (1 - animProgress_) * height to
        // slide it into view.
        const int fullH = getHeight();
        const int offY  = static_cast<int>((1.0f - animProgress_) * static_cast<float>(fullH));
        setTopLeftPosition(getX(), getParentHeight() - fullH + offY);
    }

    //==========================================================================
    static constexpr int kHeaderH  = 40;
    static constexpr int kCellSize = 52;
    static constexpr int kCellGap  = 6;
    static constexpr int kLabelW   = 24;

private:
    //==========================================================================
    // Coupling type catalogue — 15 types in three tiers

    struct TypeEntry
    {
        CouplingType type;
        const char*  label;    // short display name for cell + popup
        const char*  tier;     // "safe" | "standard" | "exotic"
    };

    static const TypeEntry* typeEntries() noexcept
    {
        static const TypeEntry entries[] = {
            // Safe
            { CouplingType::AmpToFilter,      "Amp→Flt",  "safe"     },
            { CouplingType::AmpToPitch,        "Amp→Pit",  "safe"     },
            { CouplingType::LFOToPitch,        "LFO→Pit",  "safe"     },
            { CouplingType::EnvToMorph,        "Env→Mor",  "safe"     },
            { CouplingType::FilterToFilter,    "Flt→Flt",  "safe"     },
            // Standard
            { CouplingType::AudioToFM,         "Aud→FM",   "standard" },
            { CouplingType::AudioToRing,       "Aud→Rng",  "standard" },
            { CouplingType::AudioToWavetable,  "Aud→Wt",   "standard" },
            { CouplingType::AudioToBuffer,     "Aud→Buf",  "standard" },
            { CouplingType::RhythmToBlend,     "Rhy→Bln",  "standard" },
            { CouplingType::EnvToDecay,        "Env→Dcy",  "standard" },
            { CouplingType::PitchToPitch,      "Pit→Pit",  "standard" },
            // Exotic
            { CouplingType::AmpToChoke,        "Amp→Chk",  "exotic"   },
            { CouplingType::KnotTopology,      "Knot",     "exotic"   },
            { CouplingType::TriangularCoupling,"Triangle", "exotic"   },
        };
        return entries;
    }

    static constexpr int kTypeCount = 15;

    static const char* labelForType(CouplingType t) noexcept
    {
        const auto* e = typeEntries();
        for (int i = 0; i < kTypeCount; ++i)
            if (e[i].type == t) return e[i].label;
        return "???";
    }

    //==========================================================================
    // Layout helpers

    /** Grid origin point (top-left of the 5×5 cell array) within our local bounds. */
    juce::Point<float> gridOrigin() const
    {
        const float gridW = static_cast<float>(kLabelW + 5 * kCellSize + 4 * kCellGap);
        const float gridH = static_cast<float>(kLabelW + 5 * kCellSize + 4 * kCellGap);
        const float cw    = static_cast<float>(getWidth());
        const float ch    = static_cast<float>(getHeight() - kHeaderH);
        return {
            (cw - gridW) * 0.5f,
            static_cast<float>(kHeaderH) + (ch - gridH) * 0.5f
        };
    }

    juce::Rectangle<float> getCellBounds(int row, int col) const
    {
        const auto origin = gridOrigin();
        const float ox = origin.x + static_cast<float>(kLabelW) +
                         static_cast<float>(col) * (kCellSize + kCellGap);
        const float oy = origin.y + static_cast<float>(kLabelW) +
                         static_cast<float>(row) * (kCellSize + kCellGap);
        return { ox, oy,
                 static_cast<float>(kCellSize),
                 static_cast<float>(kCellSize) };
    }

    std::pair<int,int> hitTestGrid(juce::Point<float> pos) const
    {
        for (int r = 0; r < 5; ++r)
            for (int c = 0; c < 5; ++c)
                if (getCellBounds(r, c).contains(pos))
                    return { c, r };
        return { -1, -1 };
    }

    void buildLayout()
    {
        // Close button — top-right of header
        const float btnSize = 24.0f;
        closeBtnBounds_ = juce::Rectangle<float>(
            static_cast<float>(getWidth()) - btnSize - 10.0f,
            (static_cast<float>(kHeaderH) - btnSize) * 0.5f,
            btnSize, btnSize);
    }

    //==========================================================================
    // Route helpers

    int findFirstRouteIndex(int src, int dst) const
    {
        for (int i = 0; i < static_cast<int>(routes_.size()); ++i)
        {
            const auto& r = routes_[static_cast<size_t>(i)];
            if (r.sourceSlot == src && r.destSlot == dst)
                return i;
        }
        return -1;
    }

    /** Count user routes on this (src, dst) pair. */
    int countRoutesOnCell(int src, int dst) const
    {
        int n = 0;
        for (const auto& r : routes_)
            if (r.sourceSlot == src && r.destSlot == dst && !r.isNormalled)
                ++n;
        return n;
    }

    /** Get the max depth (amount) for routes on this (src, dst) pair. */
    float maxDepthOnCell(int src, int dst) const
    {
        float maxAmt = 0.0f;
        for (const auto& r : routes_)
            if (r.sourceSlot == src && r.destSlot == dst)
                maxAmt = std::max(maxAmt, r.amount);
        return maxAmt;
    }

    /** Get the first coupling type on this (src, dst) pair. */
    CouplingType firstTypeOnCell(int src, int dst) const
    {
        for (const auto& r : routes_)
            if (r.sourceSlot == src && r.destSlot == dst)
                return r.type;
        return CouplingType::AmpToFilter; // fallback, never displayed
    }

    //==========================================================================
    // Painting

    void paintCloseButton(juce::Graphics& g,
                          const juce::Rectangle<float>& /*headerBounds*/)
    {
        const auto  c    = closeBtnBounds_.getCentre();
        const float size = closeBtnBounds_.getWidth() * 0.30f;
        const bool  hov  = closeBtnHovered_;

        g.setColour(hov ? juce::Colour(GalleryColors::Ocean::foam).withAlpha(0.90f)
                        : juce::Colour(GalleryColors::Ocean::salt).withAlpha(0.55f));
        g.drawLine(c.x - size, c.y - size, c.x + size, c.y + size, 1.5f);
        g.drawLine(c.x + size, c.y - size, c.x - size, c.y + size, 1.5f);
    }

    void paintGrid(juce::Graphics& g, const juce::Rectangle<float>& /*contentBounds*/)
    {
        const auto origin = gridOrigin();

        // Column header labels (0..4)
        g.setFont(XO::Tokens::Type::mono(XO::Tokens::Type::MonoSmall));
        g.setColour(juce::Colour(GalleryColors::Ocean::salt).withAlpha(0.60f));
        for (int c = 0; c < 5; ++c)
        {
            const float ox = origin.x + static_cast<float>(kLabelW) +
                             static_cast<float>(c) * (kCellSize + kCellGap) +
                             static_cast<float>(kCellSize) * 0.5f;
            g.drawText(juce::String(c),
                       juce::Rectangle<float>(ox - 10.0f,
                                              origin.y,
                                              20.0f,
                                              static_cast<float>(kLabelW)),
                       juce::Justification::centred, false);
        }

        // Row header labels (0..4)
        for (int r = 0; r < 5; ++r)
        {
            const float oy = origin.y + static_cast<float>(kLabelW) +
                             static_cast<float>(r) * (kCellSize + kCellGap) +
                             static_cast<float>(kCellSize) * 0.5f;
            g.drawText(juce::String(r),
                       juce::Rectangle<float>(origin.x,
                                              oy - 10.0f,
                                              static_cast<float>(kLabelW) - 4.0f,
                                              20.0f),
                       juce::Justification::centredRight, false);
        }

        // Cells
        for (int r = 0; r < 5; ++r)
        {
            for (int c = 0; c < 5; ++c)
            {
                const auto cellRect = getCellBounds(r, c);
                paintCell(g, r, c, cellRect);
            }
        }
    }

    void paintCell(juce::Graphics& g, int row, int col,
                   const juce::Rectangle<float>& rect)
    {
        const bool isDiagonal = (row == col);
        const int  routeIdx   = findFirstRouteIndex(row, col);
        const bool hasFilled  = (routeIdx >= 0);
        const bool isHovered  = (hoveredRow_ == row && hoveredCol_ == col);

        if (isDiagonal)
        {
            // Cross-hatched — disabled
            g.setColour(juce::Colour(GalleryColors::Ocean::plankton).withAlpha(0.15f));
            g.fillRoundedRectangle(rect, 4.0f);

            // Cross-hatch lines
            g.setColour(juce::Colour(GalleryColors::Ocean::plankton).withAlpha(0.20f));
            const float x1 = rect.getX();
            const float y1 = rect.getY();
            const float x2 = rect.getRight();
            const float y2 = rect.getBottom();
            g.drawLine(x1, y1, x2, y2, 1.0f);
            g.drawLine(x2, y1, x1, y2, 1.0f);
            return;
        }

        if (hasFilled)
        {
            const float  depth    = maxDepthOnCell(row, col);
            const CouplingType ct = firstTypeOnCell(row, col);
            const int    count    = countRoutesOnCell(row, col);
            const bool   exotic   = isExoticType(ct);

            // Fill
            const auto fillColor = exotic
                ? XO::Tokens::Color::warning().withAlpha(0.18f)
                : XO::Tokens::Color::accent().withAlpha(0.22f);
            const auto borderColor = exotic
                ? XO::Tokens::Color::warning().withAlpha(isHovered ? 0.70f : 0.45f)
                : XO::Tokens::Color::accent().withAlpha(isHovered ? 0.80f : 0.55f);

            g.setColour(fillColor);
            g.fillRoundedRectangle(rect, 4.0f);
            g.setColour(borderColor);
            g.drawRoundedRectangle(rect, 4.0f, 1.0f);

            // Depth bar — inner bottom strip, height proportional to amount
            const float barH    = rect.getHeight() * std::max(0.04f, depth);
            const auto  barRect = rect.withTrimmedTop(rect.getHeight() - barH).reduced(3.0f, 0.0f);
            const auto  barColor = exotic
                ? XO::Tokens::Color::warning().withAlpha(0.55f)
                : XO::Tokens::Color::accent().withAlpha(0.65f);
            g.setColour(barColor);
            g.fillRoundedRectangle(barRect, 2.0f);

            // Type label
            g.setFont(XO::Tokens::Type::mono(XO::Tokens::Type::MonoTiny));
            g.setColour(juce::Colour(GalleryColors::Ocean::foam).withAlpha(0.85f));
            g.drawText(labelForType(ct),
                       rect.reduced(3.0f, 2.0f).withTrimmedBottom(barH + 2.0f).toNearestInt(),
                       juce::Justification::centredTop, true);

            // Count badge (if > 1 route)
            if (count > 1)
            {
                const float badgeR = 8.0f;
                const float bx = rect.getRight() - badgeR - 2.0f;
                const float by = rect.getY() + 2.0f;
                g.setColour(XO::Tokens::Color::accent());
                g.fillEllipse(bx, by, badgeR * 2.0f, badgeR * 2.0f);
                g.setFont(XO::Tokens::Type::mono(XO::Tokens::Type::MonoTiny));
                g.setColour(juce::Colour(GalleryColors::Ocean::abyss));
                g.drawText(juce::String(count),
                           juce::Rectangle<float>(bx, by, badgeR * 2.0f, badgeR * 2.0f).toNearestInt(),
                           juce::Justification::centred, false);
            }
        }
        else
        {
            // Empty cell
            const auto borderColor = isHovered
                ? juce::Colour(GalleryColors::Ocean::plankton).withAlpha(0.55f)
                : juce::Colour(GalleryColors::Ocean::plankton).withAlpha(0.25f);
            const auto bgColor = isHovered
                ? juce::Colour(GalleryColors::Ocean::shallow).withAlpha(0.25f)
                : juce::Colour(0, 0, 0).withAlpha(0.0f);

            g.setColour(bgColor);
            g.fillRoundedRectangle(rect, 4.0f);
            g.setColour(borderColor);
            g.drawRoundedRectangle(rect.reduced(0.5f), 4.0f, 1.0f);
        }
    }

    static bool isExoticType(CouplingType t) noexcept
    {
        return t == CouplingType::KnotTopology
            || t == CouplingType::TriangularCoupling
            || t == CouplingType::AmpToChoke;
    }

    //==========================================================================
    // Type popup — 15 types grouped by tier

    void showTypePopup(int src, int dst)
    {
        juce::PopupMenu menu;
        const auto* entries = typeEntries();

        // Safe group
        juce::PopupMenu safeMenu;
        juce::PopupMenu standardMenu;
        juce::PopupMenu exoticMenu;

        for (int i = 0; i < kTypeCount; ++i)
        {
            const auto& e = entries[i];
            const int   itemId = i + 1; // 1-based for PopupMenu

            juce::String label(e.label);
            label = label.replace("\xe2\x86\x92", "->"); // ensure ASCII safety

            if (juce::String(e.tier) == "safe")
                safeMenu.addItem(itemId, label);
            else if (juce::String(e.tier) == "standard")
                standardMenu.addItem(itemId, label);
            else
                exoticMenu.addItem(itemId, label);
        }

        menu.addSubMenu("Safe", safeMenu);
        menu.addSubMenu("Standard", standardMenu);
        menu.addSubMenu("Exotic", exoticMenu);

        // Use async show — required because JUCE_MODAL_LOOPS_PERMITTED=0 in plugin context.
        menu.showMenuAsync(
            juce::PopupMenu::Options()
                .withTargetComponent(this)
                .withTargetScreenArea(getCellBounds(src, dst)
                                         .toNearestInt()
                                         .translated(getScreenX(), getScreenY())),
            [this, src, dst](int result)
            {
                if (result <= 0) return; // dismissed
                const int idx = result - 1;
                if (idx < 0 || idx >= kTypeCount) return;

                const CouplingType selected = typeEntries()[idx].type;

                if (onAddRoute)
                    onAddRoute(src, dst, selected);
            });
    }

    //==========================================================================
    // Mouse hover

    void mouseMove(const juce::MouseEvent& e) override
    {
        const auto [col, row] = hitTestGrid(e.position.toFloat());
        const bool  closeHov  = closeBtnBounds_.contains(e.position.toFloat());

        if (col != hoveredCol_ || row != hoveredRow_ || closeHov != closeBtnHovered_)
        {
            hoveredCol_      = col;
            hoveredRow_      = row;
            closeBtnHovered_ = closeHov;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        hoveredCol_ = hoveredRow_ = -1;
        closeBtnHovered_ = false;
        repaint();
    }

    //==========================================================================
    // Animation state

    enum class AnimState { Closed, Opening, Open, Closing };

    AnimState animState_    = AnimState::Closed;
    float     animProgress_ = 0.0f;

    //==========================================================================
    // State

    std::vector<MegaCouplingMatrix::CouplingRoute> routes_;

    // Hover tracking
    int  hoveredRow_ = -1;
    int  hoveredCol_ = -1;
    bool closeBtnHovered_ = false;

    // Layout rect — rebuilt by buildLayout()
    juce::Rectangle<float> closeBtnBounds_;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainMatrix)
};

} // namespace xoceanus
