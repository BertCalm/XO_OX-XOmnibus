// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// OceanLayout.h  —  Phase 2 of the OceanView decomposition (issue #1184).
//
// OceanLayout owns all geometry/layout logic that previously lived directly in
// OceanView.  It holds references to every component it must position — no
// back-reference to OceanView.  It reads waterline height via OceanChildren.
//
// Construction
// ────────────
//   OceanLayout layout_{children_, /* LayoutTargets */ {  }};
//
//   OceanView::resized() becomes:
//     layout_.layoutForState(viewState_, getLocalBounds(), selectedSlot_,
//                            detailShowing_, firstLaunch_);
//   Phase 3 animation:
//     layout_.layoutForState(state, bounds, slot, detail, firstLaunch, progress01);
//
// Constraints (same as OceanChildren):
//   - No back-reference to OceanView (no OceanView* member).
//   - LayoutTargets members are plain Component& / Component* references; they
//     are never used to call back into OceanView — only setBounds/setVisible/
//     toFront.
//   - All geometry constants (kDashboardH, kStatusBarH, etc.) duplicated here
//     for now; Phase 3 should consolidate them into a shared constants header.
//
// Phase 3 will extract OceanStateMachine.  At that point the `viewState_`
// placeholder in applyLayout() will be replaced by a stateMachine_ query.

#include <juce_gui_basics/juce_gui_basics.h>
#include "OceanChildren.h"
#include "OceanBackground.h"
#include "AmbientEdge.h"
#include "EngineOrbit.h"
#include "CouplingSubstrate.h"
#include "DnaMapBrowser.h"
#include "DetailOverlay.h"
#include "PlaySurfaceOverlay.h"
#include "EnginePickerDrawer.h"
#include "SettingsDrawer.h"
#include "SubmarineOuijaPanel.h"
#include "ExpressionStrips.h"
#include "SubmarinePlaySurface.h"
#include "DotMatrixDisplay.h"
#include "SubmarineHudBar.h"
#include "SurfaceRightPanel.h"
#include "CouplingConfigPopup.h"
#include "../GalleryColors.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <array>
#include <algorithm>
#include <cmath>

// Forward declaration — DimOverlay is defined inside OceanView.h.
// OceanLayout only needs juce::Component& to call setBounds on it.

namespace xoceanus
{

/**
    LayoutTargets

    A plain struct holding non-owning references to every component that
    OceanLayout must position.  Constructed once by OceanView and passed to
    OceanLayout at construction.

    All members are non-null references (value-type members or direct Component
    member addresses from OceanView).  Pointers are used only where the component
    is genuinely optional (might not exist at layout time).
*/
struct LayoutTargets
{
    // Background layers
    OceanBackground&                background;
    CouplingSubstrate&              substrate;
    std::array<EngineOrbit, 5>&     orbits;
    AmbientEdge&                    ambientEdge;

    // Overlay panels
    DnaMapBrowser&                  browser;
    DetailOverlay&                  detailOverlay;
    CouplingConfigPopup&            couplingPopup;
    juce::Component&                dimOverlay;   // DimOverlay — typed as Component to avoid circular header
    PlaySurfaceOverlay&             playSurfaceOverlay;

    // Empty-state / first-launch helpers
    juce::Label&                    emptyStateLabel;
    juce::Component&                lifesaver;    // LifesaverOverlay — typed as Component

    // Floating header buttons
    juce::TextButton&               enginesButton;
    juce::TextButton&               presetPrev;
    juce::TextButton&               presetNext;
    juce::TextButton&               favButton;
    juce::TextButton&               settingsButton;
    juce::TextButton&               keysButton;
    juce::Label&                    presetNameLabel;

    // Drawers
    EnginePickerDrawer&             engineDrawer;
    SettingsDrawer&                 settingsDrawer;

    // Dashboard (non-children_ components)
    SubmarineHudBar&                hudBar;
    DotMatrixDisplay&               dotMatrix;
    juce::Component&                tabBar;   // DashboardTabBar — defined inside OceanView.h; typed as Component&
    ExpressionStrips&               exprStrips;
    SubmarinePlaySurface&           subPlaySurface;
    SubmarineOuijaPanel&            ouijaPanel;
    SurfaceRightPanel&              surfaceRight;

    // Phase 2.5 (#1184): layout-input state — const refs to OceanView members.
    // Removed from layoutForState() per-call args; now read directly from here.
    const int&  selectedSlot;    ///< OceanView::selectedSlot_
    const bool& detailShowing;   ///< OceanView::detailShowing_
    const bool& firstLaunch;     ///< OceanView::firstLaunch_
};

//==============================================================================
/**
    OceanLayout

    Encapsulates all geometry/layout strategies extracted from OceanView.

    Responsibilities:
      - `layoutForState()` — called from OceanView::resized(); dispatches to the
        correct per-state layout strategy and also runs the dashboard/overlay
        layout that is state-independent.
      - `reorderZStack()` — static Z-order enforcement; called once per setup
        phase and on each visibility toggle that needs a re-stack.
      - Geometry helpers: `computeOceanArea()`, `getEffectiveDashboardH()`,
        `polarToCartesian()`.

    Rules:
      - Holds `const OceanChildren& children_` to read waterline height.
      - Holds `LayoutTargets targets_` (struct of component references) for all
        other components it must position.
      - No back-reference to OceanView.  State required by layout strategies
        (viewState, selectedSlot, detailShowing, firstLaunch) is passed as
        arguments to `applyLayout()`.
      - Temporary getters on OceanView that Phase 2 added (if any) are
        documented as Phase-3 cleanup in that file.
*/
class OceanLayout
{
public:
    //==========================================================================
    // Construction
    //==========================================================================

    OceanLayout(const OceanChildren& children, LayoutTargets targets) noexcept
        : children_(children)
        , targets_(targets)
    {}

    // Non-copyable, non-movable — holds references.
    OceanLayout(const OceanLayout&)            = delete;
    OceanLayout(OceanLayout&&)                 = delete;
    OceanLayout& operator=(const OceanLayout&) = delete;
    OceanLayout& operator=(OceanLayout&&)      = delete;

    //==========================================================================
    // Primary entry point — called from OceanView::resized()
    //==========================================================================

    /**
        Apply the full window layout.

        The `selectedSlot`, `detailShowing`, and `firstLaunch` state values are
        now read from `targets_` (const refs to OceanView members) rather than
        passed per-call.

        @param viewState   Current state machine state (Orbital / ZoomIn / etc.)
        @param fullBounds  OceanView's local bounds (from getLocalBounds()).
        @param progress01  Normalised animation progress [0, 1] for in-flight
                           transitions (default 1.0 = fully settled).  Reserved
                           for Phase 3 — currently unused (juce::ignoreUnused).
    */
    enum class ViewState
    {
        Orbital,
        ZoomIn,
        SplitTransform,
        BrowserOpen
    };

    void layoutForState(ViewState            viewState,
                        juce::Rectangle<int> fullBounds,
                        float                progress01 = 1.0f)
    {
        juce::ignoreUnused(progress01);  // Phase 3 will use this for animation interpolation.

        // ── Ocean-area strategy (state-dependent) ────────────────────────────
        switch (viewState)
        {
            case ViewState::Orbital:
                layoutOrbital(fullBounds);
                break;
            case ViewState::ZoomIn:
                layoutZoomIn(fullBounds);
                break;
            case ViewState::SplitTransform:
                layoutSplitTransform(fullBounds);
                break;
            case ViewState::BrowserOpen:
                layoutBrowser(fullBounds);
                break;
        }

        layoutFloatingControls(fullBounds);

        // ── Step 6: Submarine dashboard layout ──────────────────────────────
        layoutDashboard(fullBounds);
    }

    //==========================================================================
    // Z-stack enforcement
    //==========================================================================

    /**
        Enforce the static Z-order of all Ocean components.

        Called exactly once per setup phase (after each initX() call in OceanView)
        and on visibility toggles that need a re-stack.  Never called from
        resized() (#1163).
    */
    void reorderZStack()
    {
        targets_.ambientEdge.toFront(false);
        // Engine orbits sit above ambient edge and below HUD/overlays.
        for (auto& orbit : targets_.orbits)
            orbit.toFront(false);
        // Fix 6: macros render above the vignette overlay (ambientEdge_).
        if (auto* m = children_.macros())         m->toFront(false);
        if (auto* dp = children_.detailPanel())   dp->toFront(false);
        if (auto* sb = children_.sidebar())       sb->toFront(false);
        targets_.browser.toFront(false);
        // DetailOverlay floats above orbits/substrate/browser but below header buttons.
        targets_.detailOverlay.toFront(false);
        // Phase 2: CouplingConfigPopup sits above detailOverlay_ but below header buttons.
        targets_.couplingPopup.toFront(false);
        targets_.presetPrev.toFront(false);
        targets_.presetNext.toFront(false);
        targets_.favButton.toFront(false);
        targets_.settingsButton.toFront(false);
        targets_.keysButton.toFront(false);
        targets_.presetNameLabel.toFront(false);
        // #1008 FIX 7: dimOverlay_ above buttons but below PlaySurfaceOverlay.
        targets_.dimOverlay.toFront(false);
        // Empty-state elements float above HUD bar.
        targets_.emptyStateLabel.toFront(false);
        targets_.lifesaver.toFront(false);
        // Step 6: waterline and tab bar sit above the dim overlay but below
        // the PlaySurface so they are always legible.
        targets_.hudBar.toFront(false);
        targets_.surfaceRight.toFront(false);
        targets_.exprStrips.toFront(false);
        targets_.subPlaySurface.toFront(false);
        targets_.playSurfaceOverlay.toFront(false);
        targets_.ouijaPanel.toFront(false);
        if (auto* wl = children_.waterline())      wl->toFront(false);
        if (auto* fx = children_.masterFxStrip())  fx->toFront(false);
        if (auto* es = children_.epicSlots())      es->toFront(false);
        targets_.tabBar.toFront(false);
        if (auto* cb = children_.chordBar())       cb->toFront(false);
        // Wave 5 C2: seq strip sits just below chord bar in the dashboard.
        if (auto* ss = children_.seqStrip())       ss->toFront(false);
        // Wave 5 B3 + C2: breakout panels float above all dashboard content.
        if (auto* cbp = children_.chordBreakout()) cbp->toFront(false);
        if (auto* sbr = children_.seqBreakout())   sbr->toFront(false);
        if (auto* tb  = children_.transportBar())  tb->toFront(false);
        if (auto* sb2 = children_.statusBar())     sb2->toFront(false);

        // Drawers and modal overlays must sit above EVERYTHING.
        targets_.engineDrawer.toFront(false);
        targets_.settingsDrawer.toFront(false);
        // DetailOverlay backdrop sits above dashboard.
        targets_.detailOverlay.toFront(false);
        // The EngineDetailPanel must sit ABOVE the overlay backdrop.
        if (auto* dp = children_.detailPanel(); dp && dp->isVisible())
            dp->toFront(false);
        targets_.couplingPopup.toFront(false);
    }

    //==========================================================================
    // Geometry helpers
    //==========================================================================

    /**
        Effective dashboard height — collapses when SurfaceRightPanel is open
        (keyboard hidden; only macros + FX + tabs remain).
    */
    int getEffectiveDashboardH() const noexcept
    {
        if (targets_.surfaceRight.isOpen() && targets_.surfaceRight.isVisible())
            return static_cast<int>(kMacroStripH) + 48 + kTabBarH;
        return kDashboardH;
    }

    /**
        Returns the ocean viewport rectangle — the area above the waterline,
        dashboard, and status bar.  Narrows from the right when SurfaceRightPanel
        is open.

        @param fullBounds  The owning component's local bounds.
    */
    juce::Rectangle<int> computeOceanArea(juce::Rectangle<int> fullBounds) const
    {
        const int wlH     = children_.waterline()
                              ? children_.waterline()->getDesiredHeight()
                              : kWaterlineH;
        const int bottomH = getEffectiveDashboardH() + wlH + kStatusBarH;
        auto area         = fullBounds.withTrimmedBottom(bottomH);

        if (targets_.surfaceRight.isOpen() && targets_.surfaceRight.isVisible())
        {
            const int rpW = std::min(SurfaceRightPanel::kPanelWidth,
                                     static_cast<int>(area.getWidth() * 0.40f));
            area = area.withTrimmedRight(rpW);
        }
        return area;
    }

    /** Convert polar angle + radius to Cartesian in the given coordinate frame. */
    static juce::Point<float> polarToCartesian(float angle,
                                               float radius,
                                               juce::Point<float> center) noexcept
    {
        return {
            center.x + radius * std::cos(angle),
            center.y + radius * std::sin(angle)
        };
    }

private:
    //==========================================================================
    // Layout strategies (state-specific)
    //==========================================================================

    void layoutOrbital(juce::Rectangle<int> fullBounds)
    {
        // Step 6: use computeOceanArea() so background/substrate/nexus only fill
        // the ocean viewport above the waterline, not the full window.
        const auto area    = computeOceanArea(fullBounds);
        const auto centerF = area.getCentre().toFloat();

        // Background, substrate, and ambient edge span the ocean viewport only.
        targets_.background.setBounds(area);
        targets_.ambientEdge.setBounds(area);
        targets_.substrate.setBounds(area);
        targets_.substrate.setVisible(true);

        // (D6 / #1096): NexusDisplay removed — no bounds to set here.

        // ── Engine creatures (freeform normalized positions) ─────────────────
        int numLoaded = 0;
        for (const auto& o : targets_.orbits)
            if (o.hasEngine()) ++numLoaded;

        // FIX 12: Keep OceanBackground informed so it can show/hide ghost outlines.
        targets_.background.setEngineCount(numLoaded);

        // ── Macros: now positioned in the dashboard strip via layoutForState() ──
        // Fix 4: only show macros when at least one engine is loaded.
        if (auto* m = children_.macros())
            m->setVisible(numLoaded > 0);

        // Layout all 4 primary orbits at their normalized positions regardless of
        // whether an engine is loaded.  Empty slots render as ghost outlines
        // (dashed circle + "+" sign) via EngineOrbit::paint().  Slot 4 (index 4)
        // is the ghost overflow slot and stays hidden when no engine occupies it.
        for (int i = 0; i < 5; ++i)
        {
            if (i == 4 && !targets_.orbits[i].hasEngine())
            {
                // Ghost overflow slot — only visible when an engine is assigned.
                targets_.orbits[i].setVisible(false);
                continue;
            }

            auto pos = targets_.orbits[i].getNormalizedPosition();
            int sz = targets_.orbits[i].getBuoySize() + kBreathPadding * 2;
            int x = static_cast<int>(pos.x * area.getWidth()) - sz / 2;
            int y = static_cast<int>(pos.y * area.getHeight()) - sz / 2;
            // Clamp buoy fully inside ocean area (prevent clipping by dashboard)
            x = juce::jlimit(0, area.getWidth() - sz, x);
            y = juce::jlimit(0, area.getHeight() - sz, y);
            targets_.orbits[i].setBounds(x + area.getX(), y + area.getY(), sz, sz);
            targets_.orbits[i].setOceanAreaBounds(area.toFloat());
            targets_.orbits[i].setVisible(true);

            if (targets_.orbits[i].hasEngine())
                targets_.substrate.setCreatureCenter(i, targets_.orbits[i].getCenter());
        }

        // Empty-state label: centred below the ocean midpoint when no engines loaded.
        targets_.emptyStateLabel.setVisible(numLoaded == 0);
        if (numLoaded == 0)
        {
            targets_.emptyStateLabel.setBounds(
                static_cast<int>(centerF.x) - 150,
                static_cast<int>(centerF.y) + 20,
                300, 32);
        }

        // Step 7: Show the pulsing lifesaver ring on first launch when empty.
        targets_.lifesaver.setVisible(targets_.firstLaunch && numLoaded == 0);
        targets_.lifesaver.setBounds(computeOceanArea(fullBounds));

        // Hide panels that belong to other states.
        if (auto* dp = children_.detailPanel(); dp && !targets_.detailShowing)
            dp->setVisible(false);
        if (auto* sb = children_.sidebar()) sb->setVisible(false);
        targets_.browser.setVisible(false);
    }

    void layoutZoomIn(juce::Rectangle<int> fullBounds)
    {
        const int  selectedSlot  = targets_.selectedSlot;
        const bool detailShowing = targets_.detailShowing;
        jassert(selectedSlot >= 0 && selectedSlot < 5);

        // Step 6: use computeOceanArea() so background/substrate only fill the
        // ocean viewport above the waterline.
        const auto area    = computeOceanArea(fullBounds);
        const auto centerF = area.getCentre().toFloat();
        const float halfMin = static_cast<float>(std::min(area.getWidth(),
                                                          area.getHeight())) * 0.5f;

        targets_.background.setBounds(area);
        targets_.ambientEdge.setBounds(area);
        targets_.substrate.setBounds(area);
        targets_.substrate.setVisible(true);

        // (D6 / #1096): NexusDisplay removed — no bounds to set here.

        // Count non-selected loaded engines (for edge positioning).
        int edgeCount = 0;
        for (int i = 0; i < 5; ++i)
            if (targets_.orbits[i].hasEngine() && i != selectedSlot) ++edgeCount;

        // Distribute minimised creatures evenly along the far edge arc.
        const float edgeRadius = halfMin * 0.85f;
        const float arcStart   = juce::MathConstants<float>::halfPi;  // bottom
        const float arcTotal   = juce::MathConstants<float>::twoPi * 0.75f;  // 3/4 circle
        const float arcStep    = (edgeCount > 1) ? arcTotal / static_cast<float>(edgeCount - 1)
                                                 : 0.0f;

        int edgeIdx = 0;
        for (int i = 0; i < 5; ++i)
        {
            if (!targets_.orbits[i].hasEngine())
            {
                targets_.orbits[i].setVisible(false);
                continue;
            }

            if (i == selectedSlot)
            {
                // Zoomed-in creature at the centre (slightly above geometric centre).
                const int size = static_cast<int>(kOrbitSize_Orbital);
                targets_.orbits[i].setBounds(
                    static_cast<int>(centerF.x) - size / 2,
                    static_cast<int>(centerF.y) - size / 2 - 40,
                    size, size);
                targets_.substrate.setCreatureCenter(i, targets_.orbits[i].getCenter());
            }
            else
            {
                // Minimised creatures arranged along the outer arc.
                const float angle = arcStart + static_cast<float>(edgeIdx) * arcStep;
                const auto  pos   = polarToCartesian(angle, edgeRadius, centerF);
                const int   size  = static_cast<int>(kOrbitSize_Orbital);

                targets_.orbits[i].setBounds(
                    static_cast<int>(pos.x) - size / 2,
                    static_cast<int>(pos.y) - size / 2,
                    size, size);
                targets_.substrate.setCreatureCenter(i, pos);
                ++edgeIdx;
            }

            targets_.orbits[i].setVisible(true);
        }

        // Macros: now positioned in the dashboard strip via applyLayout().
        if (auto* m = children_.macros())
            m->setVisible(true);

        if (auto* dp = children_.detailPanel(); dp && !detailShowing)
            dp->setVisible(false);
        if (auto* sb = children_.sidebar()) sb->setVisible(false);
        targets_.browser.setVisible(false);
        targets_.emptyStateLabel.setVisible(false);  // ZoomIn always has an engine selected
    }

    void layoutSplitTransform(juce::Rectangle<int> fullBounds)
    {
        const int selectedSlot = targets_.selectedSlot;
        jassert(selectedSlot >= 0 && selectedSlot < 5);

        // Step 6: use computeOceanArea() so background/ambient edge stay within the
        // ocean viewport above the waterline.
        const auto area    = computeOceanArea(fullBounds);
        const int  orbW    = static_cast<int>(static_cast<float>(area.getWidth())
                                               * kSplitOrbitalFraction);
        const int  detailW = area.getWidth() - orbW;

        // Background and ambient edge span the ocean viewport.
        targets_.background.setBounds(area);
        targets_.ambientEdge.setBounds(area);

        // Substrate is clipped to the orbital strip in split mode.
        targets_.substrate.setBounds(0, 0, orbW, area.getHeight());
        targets_.substrate.setVisible(true);

        // ── Mini orbital strip ────────────────────────────────────────────────
        // Stack loaded creatures vertically within the left strip.
        // y starts at 48 (was 40) to clear the 44px header button strip (#1006).
        int  y        = 48;
        int  stripCx  = orbW / 2;
        const int miniSize = static_cast<int>(kOrbitSize_Orbital);

        for (int i = 0; i < 5; ++i)
        {
            if (!targets_.orbits[i].hasEngine())
            {
                targets_.orbits[i].setVisible(false);
                continue;
            }

            const int sz = (i == selectedSlot)
                               ? static_cast<int>(kOrbitSize_Orbital * 0.6f)
                               : miniSize;

            targets_.orbits[i].setBounds(stripCx - sz / 2, y, sz, sz);
            targets_.orbits[i].setVisible(true);

            targets_.substrate.setCreatureCenter(i, targets_.orbits[i].getCenter());
            y += sz + 16;
        }

        // (D6 / #1096) NexusDisplay removed. Macros hidden in SplitTransform —
        // the detail panel owns the identity display on the right.
        if (auto* m = children_.macros()) m->setVisible(false);

        // ── Detail panel occupies the right 80% ───────────────────────────────
        if (auto* dp = children_.detailPanel())
        {
            dp->setBounds(orbW, 0, detailW, area.getHeight());
            dp->setVisible(true);
            dp->loadSlot(selectedSlot);
        }

        if (auto* sb = children_.sidebar()) sb->setVisible(false);
        targets_.browser.setVisible(false);
        targets_.emptyStateLabel.setVisible(false);  // SplitTransform always has engine selected
    }

    void layoutBrowser(juce::Rectangle<int> fullBounds)
    {
        const bool detailShowing = targets_.detailShowing;
        // Step 6: browser covers the ocean viewport above the waterline only.
        const auto area = computeOceanArea(fullBounds);

        // Browser covers the ocean viewport.
        targets_.browser.setBounds(area);
        targets_.browser.setVisible(true);

        // All orbital components are hidden while the browser is open.
        targets_.background.setBounds(area);  // keep background behind the browser
        targets_.ambientEdge.setBounds(area);
        targets_.substrate.setVisible(false);

        for (auto& o : targets_.orbits)
            o.setVisible(false);

        // (D6 / #1096): NexusDisplay removed — no nexus_.setVisible() needed here.
        if (auto* m = children_.macros()) m->setVisible(false);
        if (auto* dp = children_.detailPanel(); dp && !detailShowing)
            dp->setVisible(false);
        if (auto* sb = children_.sidebar()) sb->setVisible(false);
        targets_.emptyStateLabel.setVisible(false);  // browser has its own empty state
    }

    //==========================================================================
    // Floating header controls layout
    //==========================================================================

    void layoutFloatingControls(juce::Rectangle<int> /*fullBounds*/)
    {
        // ── Left cluster: engines | prev | presetName | next | fav ───────────
        // #908: WCAG 2.5.5 requires a minimum 44×44pt touch target.
        constexpr int kBtnH        = 44;   // #908: WCAG AA minimum (was 28)
        constexpr int kNavW        = 44;   // #908: square touch target for nav arrows
        constexpr int kFavW        = 44;   // #908: square touch target for favourite star
        constexpr int kEnginesW    = 90;   // Phase 3: Engines button width
        constexpr int kTopMargin   = 0;    // anchored to top edge
        constexpr int kLeftMargin  = 4;
        constexpr int kGap         = 0;    // targets are flush

        // Phase 3: Engines button (leftmost)
        targets_.enginesButton.setBounds(kLeftMargin, kTopMargin, kEnginesW, kBtnH);
        const int afterEngines = kLeftMargin + kEnginesW + 4;

        targets_.presetPrev.setBounds(afterEngines,
                                      kTopMargin,
                                      kNavW, kBtnH);

        // #1007 FIX 3: Inline preset name label sits between < and > so the
        // spatial grouping "< Preset Name >" is immediately legible.
        // Width is capped at 160pt so it doesn't crowd the fav button.
        constexpr int kNameLabelW = 160;
        targets_.presetNameLabel.setBounds(afterEngines + kNavW,
                                           kTopMargin,
                                           kNameLabelW, kBtnH);

        targets_.presetNext.setBounds(afterEngines + kNavW + kNameLabelW + kGap,
                                      kTopMargin,
                                      kNavW, kBtnH);

        targets_.favButton.setBounds(afterEngines + kNavW + kNameLabelW + kNavW + kGap * 2,
                                     kTopMargin,
                                     kFavW, kBtnH);

        // ── Right cluster: settings | KEYS ────────────────────────────────────
        constexpr int kSettingsW   = 44;   // #908: minimum square tap target
        constexpr int kKeysW       = 56;   // KEYS label needs slightly more width
        constexpr int kRightMargin = 4;

        // Width of the owning component is read from the button's parent; since
        // OceanLayout doesn't hold an OceanView*, we get parent width via the
        // button itself (which must already be added as a child).
        const int parentW = targets_.keysButton.getParentWidth();

        targets_.settingsButton.setBounds(parentW - kRightMargin - kSettingsW - kGap - kKeysW,
                                          kTopMargin,
                                          kSettingsW, kBtnH);

        targets_.keysButton.setBounds(parentW - kRightMargin - kKeysW,
                                      kTopMargin,
                                      kKeysW, kBtnH);
    }

    //==========================================================================
    // Dashboard layout (state-independent)
    //==========================================================================

    void layoutDashboard(juce::Rectangle<int> fullBounds)
    {
        const auto oceanArea = computeOceanArea(fullBounds);

        // Right-side panel (PAD/DRUM/XY) — sits beside the ocean.
        if (targets_.surfaceRight.isOpen() && targets_.surfaceRight.isVisible())
        {
            const int rpW = std::min(SurfaceRightPanel::kPanelWidth,
                                     static_cast<int>(fullBounds.getWidth() * 0.40f));
            const int wlH2 = children_.waterline()
                              ? children_.waterline()->getDesiredHeight()
                              : kWaterlineH;
            const int bottomH = getEffectiveDashboardH() + wlH2 + kStatusBarH;
            targets_.surfaceRight.setBounds(oceanArea.getRight(),
                                            fullBounds.getY(),
                                            rpW,
                                            fullBounds.getHeight() - bottomH);
        }

        // HUD nav bar — floats at top of ocean area (12px from top, 16px from sides).
        targets_.hudBar.setBounds(oceanArea.getX() + 16,
                                  oceanArea.getY() + 12,
                                  oceanArea.getWidth() - 32,
                                  40);

        // Waterline separator strip — height is dynamic (6px collapsed, 96px expanded).
        const int wlH = children_.waterline()
                          ? children_.waterline()->getDesiredHeight()
                          : kWaterlineH;
        if (auto* wl = children_.waterline())
            wl->setBounds(fullBounds.getX(), oceanArea.getBottom(), fullBounds.getWidth(), wlH);

        // Dashboard area: between the waterline and the status bar.
        auto dashArea = fullBounds
                            .withTrimmedTop(oceanArea.getHeight() + wlH)
                            .withTrimmedBottom(kStatusBarH);

        // Macro strip (top of dashboard) — macros left, dot-matrix right.
        {
            auto macroRow = dashArea.removeFromTop(static_cast<int>(kMacroStripH));
            if (auto* m = children_.macros())
            {
                const int macroW = std::min(480, macroRow.getWidth() / 2);
                m->setBounds(macroRow.removeFromLeft(macroW));
            }
            // Dot-matrix display fills the remaining space.
            targets_.dotMatrix.setBounds(macroRow.reduced(4, 4));
        }

        // Master FX compact strip (48px, between macros and tab bar).
        if (auto* fx = children_.masterFxStrip())
            fx->setBounds(dashArea.removeFromTop(48));

        // Epic Slots panel (3-slot FX picker — below Master FX strip).
        if (auto* es = children_.epicSlots())
            es->setBounds(dashArea.removeFromTop(EpicSlotsPanel::preferredHeight()));

        // Tab bar row.
        targets_.tabBar.setBounds(dashArea.removeFromTop(kTabBarH));

        // Chord bar (visible when CHORD toggle is on, ~28px).
        {
            auto* cb = children_.chordBar();
            if (cb && cb->isVisible())
                cb->setBounds(dashArea.removeFromTop(42));
        }

        // Seq strip — Wave 5 C2 mount: always-visible 24px strip below chord bar.
        if (auto* ss = children_.seqStrip())
            ss->setBounds(dashArea.removeFromTop(SeqStripComponent::kStripHeight));

        // ChordBreakoutPanel — Wave 5 B3 mount: bottom 60% overlay.
        if (auto* cbp = children_.chordBreakout())
        {
            const int panelH = static_cast<int>(fullBounds.getHeight() * 0.60f);
            cbp->setSize(fullBounds.getWidth(), panelH);
            if (!cbp->isOpen())
                cbp->setTopLeftPosition(0, fullBounds.getHeight()); // off-screen when closed
        }

        // SeqBreakoutComponent — Wave 5 C2 mount: bottom ~60% overlay.
        if (auto* sb = children_.seqBreakout())
            sb->setBounds(fullBounds.withTop(fullBounds.getHeight() * 2 / 5));

        // Expression strips (36px) on the left of the play area.
        targets_.exprStrips.setBounds(dashArea.removeFromLeft(ExpressionStrips::kStripWidth));

        // Remaining dashboard space → Submarine PlaySurface (KEYS keyboard).
        targets_.playSurfaceOverlay.setVisible(false);
        targets_.ouijaPanel.setVisible(false);
        targets_.subPlaySurface.setBounds(dashArea);
        // Only show keyboard when right panel is closed (KEYS mode).
        if (!targets_.surfaceRight.isOpen() || !targets_.surfaceRight.isVisible())
            targets_.subPlaySurface.setVisible(true);
        else
            targets_.subPlaySurface.setVisible(false);

        // Transport bar (submarine) replaces the old status bar at the bottom.
        if (auto* tb = children_.transportBar())
            tb->setBounds(0, fullBounds.getHeight() - kStatusBarH,
                          fullBounds.getWidth(), kStatusBarH);

        // Legacy status bar (Gallery) — hidden when transport bar is active.
        if (auto* sb2 = children_.statusBar())
        {
            if (children_.transportBar())
                sb2->setVisible(false);
            else
                sb2->setBounds(0, fullBounds.getHeight() - kStatusBarH,
                               fullBounds.getWidth(), kStatusBarH);
        }

        // ── Modal overlays and drawers: FULL WINDOW HEIGHT ──────────────────
        // These must cover the full window including dashboard/keyboard.
        targets_.detailOverlay.setBounds(fullBounds);
        targets_.couplingPopup.setBounds(fullBounds);
        targets_.dimOverlay.setBounds(fullBounds);
        targets_.engineDrawer.setBounds(fullBounds.withWidth(EnginePickerDrawer::kDrawerWidth));
        targets_.settingsDrawer.setBounds(
            fullBounds
                .withLeft(fullBounds.getRight() - SettingsDrawer::kDrawerWidth)
                .withWidth(SettingsDrawer::kDrawerWidth));

        // Nuclear safeguard: ensure detail panel is hidden when not actively showing.
        // (The caller's detailShowing flag guards this properly; this is the last word.)
        // Note: this check is redundant here since each layout strategy already
        // enforces it.  Kept as belt-and-suspenders because the pattern existed
        // in the original OceanView::resized().
    }

    //==========================================================================
    // State
    //==========================================================================

    const OceanChildren& children_;  ///< Reads waterline height; no OceanView ref.
    LayoutTargets        targets_;   ///< All component references OceanLayout positions.

    //==========================================================================
    // Layout constants
    // (Phase 3 TODO: consolidate into a shared OceanLayoutConstants header)
    //==========================================================================

    static constexpr int   kStatusBarH          = 28;
    static constexpr float kMacroStripH         = 60.0f;
    static constexpr float kSplitOrbitalFraction = 0.20f;
    static constexpr int   kWaterlineH          = 6;
    static constexpr int   kDashboardH          = 340;
    static constexpr int   kTabBarH             = 30;

    // HIGH fix (#1006): padding for orbital breath animation.
    static constexpr int kBreathPadding = 30;

    // Orbit size alias: reference EngineOrbit constant directly.
    static constexpr float kOrbitSize_Orbital = EngineOrbit::kOrbitalSize;
};

} // namespace xoceanus
