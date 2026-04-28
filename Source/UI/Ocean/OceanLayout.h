// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// OceanLayout.h  —  Phase 2 of the OceanView decomposition (issue #1184).
//
// OceanLayout owns all per-ViewState layout logic previously inlined in OceanView.
// It has no back-reference to OceanView — it reads children via OceanChildren
// accessors, and reads all other OceanView-owned components via the OceanViewContext
// struct (a bundle of non-owning references).
//
// Phase 3 will extract OceanStateMachine.  At that point the temporary
// OceanViewContext references tagged "TODO Phase 3 cleanup" can be
// reviewed for removal or migration.
//
// Construction order (unchanged from Phase 1)
// ─────────────────────────────────────────────
//   1. OceanView constructs OceanChildren children_{*this}
//   2. OceanView constructs OceanLayout layout_{children_, ctx_} where ctx_ is
//      built from OceanView's own members.
//   3. resized() delegates: layout_.layoutForState(viewState_, getLocalBounds(), 1.0f)
//   4. reorderZStack() delegates: layout_.reorderZStack()

#include <juce_gui_basics/juce_gui_basics.h>
#include "OceanChildren.h"
#include "OceanBackground.h"
#include "AmbientEdge.h"
#include "EngineOrbit.h"
#include "CouplingSubstrate.h"
#include "CouplingConfigPopup.h"
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

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <array>
#include <cmath>

namespace xoceanus
{

// DimOverlay is defined in OceanView.h.  OceanLayout only calls juce::Component
// base methods on it (setBounds, toFront), so it is stored as juce::Component&.

//==============================================================================
/**
    OceanViewContext

    A plain struct of non-owning references to all OceanView members that
    OceanLayout needs but that are NOT managed by OceanChildren.

    All fields are references — they bind at OceanLayout construction and remain
    valid for the lifetime of OceanView (which outlives OceanLayout).

    Fields tagged "// TODO Phase 3 cleanup" are temporary scaffolds that Phase 3
    will either remove or migrate once OceanStateMachine is extracted.

    Rules (enforced by code structure):
      - No OceanView* stored anywhere.
      - No callbacks into OceanView.
      - References are const where OceanLayout only reads; non-const where it
        calls setBounds / setVisible / toFront on the referenced component.
*/
struct OceanViewContext
{
    // ── State variables (read-only, via getters below) ────────────────────
    // TODO Phase 3 cleanup: these will migrate to OceanStateMachine.
    const int&   selectedSlot;      ///< which engine slot is "selected"
    const bool&  firstLaunch;       ///< true until first engine load
    const bool&  detailShowing;     ///< true while EngineDetailPanel is visible

    // ── Component references (layout calls setBounds/setVisible) ──────────
    OceanBackground&                            background;
    CouplingSubstrate&                          substrate;
    std::array<EngineOrbit, 5>&                 orbits;
    AmbientEdge&                                ambientEdge;
    DnaMapBrowser&                              browser;

    // ── Label / simple components ─────────────────────────────────────────
    juce::Label&                                emptyStateLabel;
    juce::Component&                            lifesaver;    ///< LifesaverOverlay — inner type of OceanView; TODO Phase 3 cleanup

    // ── Overlays ──────────────────────────────────────────────────────────
    DetailOverlay&                              detailOverlay;
    CouplingConfigPopup&                        couplingPopup;
    juce::Component&                            dimOverlay;   // DimOverlay — inner-type of OceanView.h; TODO Phase 3 cleanup
    PlaySurfaceOverlay&                         playSurfaceOverlay;

    // ── Dashboard ─────────────────────────────────────────────────────────
    SubmarineOuijaPanel&                        ouijaPanel;
    ExpressionStrips&                           exprStrips;
    DotMatrixDisplay&                           dotMatrix;
    SubmarineHudBar&                            hudBar;
    SubmarinePlaySurface&                       subPlaySurface;
    SurfaceRightPanel&                          surfaceRight;
    juce::Component&                            tabBar;  // DashboardTabBar is an OceanView inner type; TODO Phase 3 cleanup

    // ── Header controls ───────────────────────────────────────────────────
    juce::TextButton&                           enginesButton;
    juce::TextButton&                           presetPrev;
    juce::TextButton&                           presetNext;
    juce::TextButton&                           favButton;
    juce::TextButton&                           settingsButton;
    juce::TextButton&                           keysButton;
    juce::Label&                                presetNameLabel;

    // ── Drawers ───────────────────────────────────────────────────────────
    EnginePickerDrawer&                         engineDrawer;
    SettingsDrawer&                             settingsDrawer;

};

//==============================================================================
/**
    OceanLayout

    Owns all ViewState-driven layout logic for OceanView.

    Design contract (from issue #1184 Phase 2 spec):
      - No back-reference to OceanView.  All access is via OceanChildren
        accessors (for deferred-init children) or OceanViewContext (for
        value-type / directly-owned components).
      - Behavior unchanged — pure refactor.
      - Header-only (matches Phase 1 OceanChildren pattern).
*/
class OceanLayout
{
public:
    //==========================================================================
    // ViewState (mirrors OceanView::ViewState — Phase 3 will unify ownership)
    //==========================================================================

    /** ViewState alias — matches OceanView::ViewState values exactly.
     *  TODO Phase 3 cleanup: replace with OceanStateMachine::ViewState. */
    enum class ViewState
    {
        Orbital,
        ZoomIn,
        SplitTransform,
        BrowserOpen
    };

    //==========================================================================
    // Construction
    //==========================================================================

    /**
        @param children  OceanChildren reference — stable for OceanView's lifetime.
        @param ctx       OceanViewContext — bundle of non-owning refs to every
                         OceanView-owned component and state variable that the
                         layout methods read.
    */
    OceanLayout(const OceanChildren& children, OceanViewContext ctx)
        : children_(children)
        , ctx_(std::move(ctx))
    {}

    // Non-copyable (holds references).
    OceanLayout(const OceanLayout&)            = delete;
    OceanLayout(OceanLayout&&)                 = delete;
    OceanLayout& operator=(const OceanLayout&) = delete;
    OceanLayout& operator=(OceanLayout&&)      = delete;

    //==========================================================================
    // Public API
    //==========================================================================

    /**
        Execute the full layout pass for the given ViewState.

        @param state        Which ViewState layout strategy to apply.
        @param bounds       The component's current local bounds.
        @param progress01   Animation progress [0,1].  Currently unused (Phase 3
                            will pass transition progress from OceanStateMachine).
    */
    void layoutForState(ViewState state,
                        juce::Rectangle<int> bounds,
                        float progress01 = 1.0f)
    {
        juce::ignoreUnused(progress01);
        bounds_ = bounds;

        switch (state)
        {
            case ViewState::Orbital:        layoutOrbital();        break;
            case ViewState::ZoomIn:         layoutZoomIn();         break;
            case ViewState::SplitTransform: layoutSplitTransform(); break;
            case ViewState::BrowserOpen:    layoutBrowser();        break;
        }

        layoutFloatingControls();
        layoutDashboard();
    }

    /**
        Establish the Z-order of all child components.

        Called once per setup phase (not per frame — see issue #1163).
    */
    void reorderZStack()
    {
        ctx_.ambientEdge.toFront(false);
        // Engine orbits must sit above the ambient edge and below the HUD / overlays
        // so they're visible even when near the waterline boundary.
        for (auto& orbit : ctx_.orbits)
            orbit.toFront(false);
        // Fix 6: macros must render above the vignette overlay (ambientEdge_).
        if (auto* m = children_.macros()) m->toFront(false);
        if (auto* dp = children_.detailPanel()) dp->toFront(false);
        if (auto* sb = children_.sidebar())     sb->toFront(false);
        ctx_.browser.toFront(false);
        // DetailOverlay floats above orbits/substrate/browser but below header buttons.
        ctx_.detailOverlay.toFront(false);
        // Phase 2: CouplingConfigPopup sits above detailOverlay_ but below header buttons.
        ctx_.couplingPopup.toFront(false);
        ctx_.presetPrev.toFront(false);
        ctx_.presetNext.toFront(false);
        ctx_.favButton.toFront(false);
        ctx_.settingsButton.toFront(false);
        ctx_.keysButton.toFront(false);
        ctx_.presetNameLabel.toFront(false);
        // #1008 FIX 7: dimOverlay_ above buttons but below PlaySurfaceOverlay.
        ctx_.dimOverlay.toFront(false);
        // Empty-state elements: emptyStateLabel_ and lifesaver_ must float above
        // the HUD bar (otherwise hudBar_ — added later — buries them).  Place them
        // here so they appear in the ocean viewport above all other ocean content
        // but are covered by the engine picker / settings drawers when open.
        ctx_.emptyStateLabel.toFront(false);
        ctx_.lifesaver.toFront(false);
        // Step 6: waterline and tab bar sit above the dim overlay but below
        // the PlaySurface so they are always legible.
        ctx_.hudBar.toFront(false);
        ctx_.surfaceRight.toFront(false);
        ctx_.exprStrips.toFront(false);
        ctx_.subPlaySurface.toFront(false);
        ctx_.playSurfaceOverlay.toFront(false);
        ctx_.ouijaPanel.toFront(false);
        if (auto* wl = children_.waterline())      wl->toFront(false);
        if (auto* fx = children_.masterFxStrip())  fx->toFront(false);
        if (auto* es = children_.epicSlots())      es->toFront(false);
        ctx_.tabBar.toFront(false);
        if (auto* cb = children_.chordBar())       cb->toFront(false);
        // Wave 5 C2: seq strip sits just below chord bar in the dashboard.
        if (auto* ss = children_.seqStrip())       ss->toFront(false);
        // Wave 5 B3 + C2: breakout panels float above all dashboard content.
        if (auto* cbp = children_.chordBreakout()) cbp->toFront(false);
        if (auto* sbr = children_.seqBreakout())   sbr->toFront(false);
        if (auto* tb  = children_.transportBar())  tb->toFront(false);
        if (auto* sb2 = children_.statusBar())     sb2->toFront(false);

        // Drawers and modal overlays must sit above EVERYTHING — including
        // the dashboard, keyboard, transport bar, and status bar.
        ctx_.engineDrawer.toFront(false);
        ctx_.settingsDrawer.toFront(false);
        // DetailOverlay backdrop sits above dashboard.
        ctx_.detailOverlay.toFront(false);
        // The EngineDetailPanel must sit ABOVE the overlay backdrop so its
        // knobs, waveform, and labels are visible (not hidden behind the
        // overlay's dark fill).  Only relevant when detail is showing.
        if (auto* dp = children_.detailPanel(); dp && dp->isVisible())
            dp->toFront(false);
        ctx_.couplingPopup.toFront(false);
    }

private:
    //==========================================================================
    // References
    //==========================================================================

    const OceanChildren& children_;        ///< deferred-init children
    OceanViewContext     ctx_;             ///< all other OceanView-owned refs
    juce::Rectangle<int> bounds_;         ///< set at start of layoutForState(); read by per-state helpers

    //==========================================================================
    // Layout constants (mirrored from OceanView for self-containment)
    // TODO Phase 3 cleanup: centralise in a shared LayoutConstants header.
    //==========================================================================

    static constexpr int   kStatusBarH           = 28;
    static constexpr float kMacroStripH          = 60.0f;
    static constexpr float kSplitOrbitalFraction = 0.20f;
    static constexpr int   kWaterlineH           = 6;
    static constexpr int   kDashboardH           = 340;
    static constexpr int   kTabBarH              = 30;
    static constexpr int   kBreathPadding        = 30;
    static constexpr float kOrbitSize_Orbital    = EngineOrbit::kOrbitalSize;

    //==========================================================================
    // Per-state layout helpers (body migrated from OceanView methods)
    //==========================================================================

    void layoutOrbital()
    {
        const auto area    = oceanArea();
        const auto centerF = area.getCentre().toFloat();

        ctx_.background.setBounds(area);
        ctx_.ambientEdge.setBounds(area);
        ctx_.substrate.setBounds(area);
        ctx_.substrate.setVisible(true);

        int numLoaded = 0;
        for (const auto& o : ctx_.orbits)
            if (o.hasEngine()) ++numLoaded;

        ctx_.background.setEngineCount(numLoaded);

        if (auto* m = children_.macros())
            m->setVisible(numLoaded > 0);

        for (int i = 0; i < 5; ++i)
        {
            if (i == 4 && !ctx_.orbits[i].hasEngine())
            {
                ctx_.orbits[i].setVisible(false);
                continue;
            }

            auto pos = ctx_.orbits[i].getNormalizedPosition();
            int sz = ctx_.orbits[i].getBuoySize() + kBreathPadding * 2;
            int x = static_cast<int>(pos.x * area.getWidth()) - sz / 2;
            int y = static_cast<int>(pos.y * area.getHeight()) - sz / 2;
            x = juce::jlimit(0, area.getWidth() - sz, x);
            y = juce::jlimit(0, area.getHeight() - sz, y);
            ctx_.orbits[i].setBounds(x + area.getX(), y + area.getY(), sz, sz);
            ctx_.orbits[i].setOceanAreaBounds(area.toFloat());
            ctx_.orbits[i].setVisible(true);

            if (ctx_.orbits[i].hasEngine())
                ctx_.substrate.setCreatureCenter(i, ctx_.orbits[i].getCenter());
        }

        ctx_.emptyStateLabel.setVisible(numLoaded == 0);
        if (numLoaded == 0)
        {
            ctx_.emptyStateLabel.setBounds(
                static_cast<int>(centerF.x) - 150,
                static_cast<int>(centerF.y) + 20,
                300, 32);
        }

        ctx_.lifesaver.setVisible(ctx_.firstLaunch && numLoaded == 0);
        ctx_.lifesaver.setBounds(oceanArea());

        if (auto* dp = children_.detailPanel(); dp && !ctx_.detailShowing)
            dp->setVisible(false);
        if (auto* sb = children_.sidebar()) sb->setVisible(false);
        ctx_.browser.setVisible(false);
    }

    void layoutZoomIn()
    {
        jassert(ctx_.selectedSlot >= 0 && ctx_.selectedSlot < 5);

        const auto area    = oceanArea();
        const auto centerF = area.getCentre().toFloat();
        const float halfMin = static_cast<float>(std::min(area.getWidth(),
                                                          area.getHeight())) * 0.5f;

        ctx_.background.setBounds(area);
        ctx_.ambientEdge.setBounds(area);
        ctx_.substrate.setBounds(area);
        ctx_.substrate.setVisible(true);

        int edgeCount = 0;
        for (int i = 0; i < 5; ++i)
            if (ctx_.orbits[i].hasEngine() && i != ctx_.selectedSlot) ++edgeCount;

        const float edgeRadius = halfMin * 0.85f;
        const float arcStart   = juce::MathConstants<float>::halfPi;
        const float arcTotal   = juce::MathConstants<float>::twoPi * 0.75f;
        const float arcStep    = (edgeCount > 1) ? arcTotal / static_cast<float>(edgeCount - 1)
                                                 : 0.0f;

        int edgeIdx = 0;
        for (int i = 0; i < 5; ++i)
        {
            if (!ctx_.orbits[i].hasEngine())
            {
                ctx_.orbits[i].setVisible(false);
                continue;
            }

            if (i == ctx_.selectedSlot)
            {
                const int size = static_cast<int>(kOrbitSize_Orbital);
                ctx_.orbits[i].setBounds(
                    static_cast<int>(centerF.x) - size / 2,
                    static_cast<int>(centerF.y) - size / 2 - 40,
                    size, size);
                ctx_.substrate.setCreatureCenter(i, ctx_.orbits[i].getCenter());
            }
            else
            {
                const float angle = arcStart + static_cast<float>(edgeIdx) * arcStep;
                const auto  pos   = polarToCartesian(angle, edgeRadius, centerF);
                const int   size  = static_cast<int>(kOrbitSize_Orbital);

                ctx_.orbits[i].setBounds(
                    static_cast<int>(pos.x) - size / 2,
                    static_cast<int>(pos.y) - size / 2,
                    size, size);
                ctx_.substrate.setCreatureCenter(i, pos);
                ++edgeIdx;
            }

            ctx_.orbits[i].setVisible(true);
        }

        if (auto* m = children_.macros())
            m->setVisible(true);

        if (auto* dp = children_.detailPanel(); dp && !ctx_.detailShowing)
            dp->setVisible(false);
        if (auto* sb = children_.sidebar()) sb->setVisible(false);
        ctx_.browser.setVisible(false);
        ctx_.emptyStateLabel.setVisible(false);
    }

    void layoutSplitTransform()
    {
        jassert(ctx_.selectedSlot >= 0 && ctx_.selectedSlot < 5);

        const auto area    = oceanArea();
        const int  orbW    = static_cast<int>(static_cast<float>(area.getWidth())
                                               * kSplitOrbitalFraction);
        const int  detailW = area.getWidth() - orbW;

        ctx_.background.setBounds(area);
        ctx_.ambientEdge.setBounds(area);

        ctx_.substrate.setBounds(0, 0, orbW, area.getHeight());
        ctx_.substrate.setVisible(true);

        int  y        = 48;
        int  stripCx  = orbW / 2;
        const int miniSize = static_cast<int>(kOrbitSize_Orbital);

        for (int i = 0; i < 5; ++i)
        {
            if (!ctx_.orbits[i].hasEngine())
            {
                ctx_.orbits[i].setVisible(false);
                continue;
            }

            const int sz = (i == ctx_.selectedSlot)
                               ? static_cast<int>(kOrbitSize_Orbital * 0.6f)
                               : miniSize;

            ctx_.orbits[i].setBounds(stripCx - sz / 2, y, sz, sz);
            ctx_.orbits[i].setVisible(true);

            ctx_.substrate.setCreatureCenter(i, ctx_.orbits[i].getCenter());
            y += sz + 16;
        }

        if (auto* m = children_.macros()) m->setVisible(false);

        if (auto* dp = children_.detailPanel())
        {
            dp->setBounds(orbW, 0, detailW, area.getHeight());
            dp->setVisible(true);
            dp->loadSlot(ctx_.selectedSlot);
        }

        if (auto* sb = children_.sidebar()) sb->setVisible(false);
        ctx_.browser.setVisible(false);
        ctx_.emptyStateLabel.setVisible(false);
    }

    void layoutBrowser()
    {
        const auto area = oceanArea();

        ctx_.browser.setBounds(area);
        ctx_.browser.setVisible(true);

        ctx_.background.setBounds(area);
        ctx_.ambientEdge.setBounds(area);
        ctx_.substrate.setVisible(false);

        for (auto& o : ctx_.orbits)
            o.setVisible(false);

        if (auto* m = children_.macros()) m->setVisible(false);
        if (auto* dp = children_.detailPanel(); dp && !ctx_.detailShowing)
            dp->setVisible(false);
        if (auto* sb = children_.sidebar()) sb->setVisible(false);
        ctx_.emptyStateLabel.setVisible(false);
    }

    void layoutFloatingControls()
    {
        constexpr int kBtnH        = 44;
        constexpr int kNavW        = 44;
        constexpr int kFavW        = 44;
        constexpr int kEnginesW    = 90;
        constexpr int kTopMargin   = 0;
        constexpr int kLeftMargin  = 4;
        constexpr int kGap         = 0;

        ctx_.enginesButton.setBounds(kLeftMargin, kTopMargin, kEnginesW, kBtnH);
        const int afterEngines = kLeftMargin + kEnginesW + 4;

        ctx_.presetPrev.setBounds(afterEngines,
                                  kTopMargin,
                                  kNavW, kBtnH);

        constexpr int kNameLabelW = 160;
        ctx_.presetNameLabel.setBounds(afterEngines + kNavW,
                                       kTopMargin,
                                       kNameLabelW, kBtnH);

        ctx_.presetNext.setBounds(afterEngines + kNavW + kNameLabelW + kGap,
                                  kTopMargin,
                                  kNavW, kBtnH);

        ctx_.favButton.setBounds(afterEngines + kNavW + kNameLabelW + kNavW + kGap * 2,
                                 kTopMargin,
                                 kFavW, kBtnH);

        constexpr int kSettingsW   = 44;
        constexpr int kKeysW       = 56;
        constexpr int kRightMargin = 4;

        ctx_.settingsButton.setBounds(bounds_.getWidth() - kRightMargin - kSettingsW - kGap - kKeysW,
                                      kTopMargin,
                                      kSettingsW, kBtnH);

        ctx_.keysButton.setBounds(bounds_.getWidth() - kRightMargin - kKeysW,
                                  kTopMargin,
                                  kKeysW, kBtnH);
    }

    void layoutDashboard()
    {
        const auto fullBounds = bounds_;
        const auto oa         = oceanArea();

        // Right-side panel (PAD/DRUM/XY) — sits beside the ocean.
        if (ctx_.surfaceRight.isOpen() && ctx_.surfaceRight.isVisible())
        {
            const int rpW = std::min(SurfaceRightPanel::kPanelWidth,
                                     static_cast<int>(fullBounds.getWidth() * 0.40f));
            const int wlH2 = children_.waterline()
                                 ? children_.waterline()->getDesiredHeight()
                                 : kWaterlineH;
            const int bottomH = effectiveDashboardH() + wlH2 + kStatusBarH;
            ctx_.surfaceRight.setBounds(oa.getRight(),
                                        fullBounds.getY(),
                                        rpW,
                                        fullBounds.getHeight() - bottomH);
        }

        ctx_.hudBar.setBounds(oa.getX() + 16,
                              oa.getY() + 12,
                              oa.getWidth() - 32,
                              40);

        const int wlH = children_.waterline()
                            ? children_.waterline()->getDesiredHeight()
                            : kWaterlineH;
        if (auto* wl = children_.waterline())
            wl->setBounds(fullBounds.getX(), oa.getBottom(),
                          fullBounds.getWidth(), wlH);

        auto dashArea = fullBounds
                            .withTrimmedTop(oa.getHeight() + wlH)
                            .withTrimmedBottom(kStatusBarH);

        // Macro strip (top of dashboard).
        {
            auto macroRow = dashArea.removeFromTop(static_cast<int>(kMacroStripH));
            if (auto* m = children_.macros())
            {
                const int macroW = std::min(480, macroRow.getWidth() / 2);
                m->setBounds(macroRow.removeFromLeft(macroW));
            }
            ctx_.dotMatrix.setBounds(macroRow.reduced(4, 4));
        }

        if (auto* fx = children_.masterFxStrip())
            fx->setBounds(dashArea.removeFromTop(48));

        if (auto* es = children_.epicSlots())
            es->setBounds(dashArea.removeFromTop(EpicSlotsPanel::preferredHeight()));

        ctx_.tabBar.setBounds(dashArea.removeFromTop(kTabBarH));

        {
            auto* cb = children_.chordBar();
            if (cb && cb->isVisible())
                cb->setBounds(dashArea.removeFromTop(42));
        }

        if (auto* ss = children_.seqStrip())
            ss->setBounds(dashArea.removeFromTop(SeqStripComponent::kStripHeight));

        if (auto* cbp = children_.chordBreakout())
        {
            const int panelH = static_cast<int>(bounds_.getHeight() * 0.60f);
            cbp->setSize(bounds_.getWidth(), panelH);
            if (!cbp->isOpen())
                cbp->setTopLeftPosition(0, bounds_.getHeight());
        }

        if (auto* sb = children_.seqBreakout())
            sb->setBounds(bounds_.withTop(bounds_.getHeight() * 2 / 5));

        ctx_.exprStrips.setBounds(dashArea.removeFromLeft(ExpressionStrips::kStripWidth));

        ctx_.playSurfaceOverlay.setVisible(false);
        ctx_.ouijaPanel.setVisible(false);
        ctx_.subPlaySurface.setBounds(dashArea);
        if (!ctx_.surfaceRight.isOpen() || !ctx_.surfaceRight.isVisible())
            ctx_.subPlaySurface.setVisible(true);
        else
            ctx_.subPlaySurface.setVisible(false);

        if (auto* tb = children_.transportBar())
            tb->setBounds(0, bounds_.getHeight() - kStatusBarH,
                          bounds_.getWidth(), kStatusBarH);

        if (auto* sb2 = children_.statusBar())
        {
            if (children_.transportBar())
                sb2->setVisible(false);
            else
                sb2->setBounds(0, bounds_.getHeight() - kStatusBarH,
                               bounds_.getWidth(), kStatusBarH);
        }

        ctx_.detailOverlay.setBounds(fullBounds);
        ctx_.couplingPopup.setBounds(fullBounds);
        ctx_.dimOverlay.setBounds(fullBounds);

        ctx_.engineDrawer.setBounds(fullBounds.withWidth(EnginePickerDrawer::kDrawerWidth));

        ctx_.settingsDrawer.setBounds(fullBounds
            .withLeft(fullBounds.getRight() - SettingsDrawer::kDrawerWidth)
            .withWidth(SettingsDrawer::kDrawerWidth));

        // Nuclear safeguard: ensure detail panel is hidden when not actively showing.
        if (auto* dp = children_.detailPanel(); dp && !ctx_.detailShowing)
            dp->setVisible(false);
    }

    //==========================================================================
    // Geometry helpers
    //==========================================================================

    /** Effective dashboard height — collapses when the right panel is open.
     *  Inlined from OceanView::getEffectiveDashboardH() — same formula, no OceanView ref. */
    int effectiveDashboardH() const noexcept
    {
        if (ctx_.surfaceRight.isOpen() && ctx_.surfaceRight.isVisible())
            return static_cast<int>(kMacroStripH) + 48 + kTabBarH; // macros + FX + tabs, no keyboard
        return kDashboardH;
    }

    /** Ocean viewport rectangle — area above the dashboard / waterline / status bar.
     *  Inlined from OceanView::getOceanArea() — same formula, no OceanView ref. */
    juce::Rectangle<int> oceanArea() const
    {
        const int wlH     = children_.waterline()
                                ? children_.waterline()->getDesiredHeight()
                                : kWaterlineH;
        const int bottomH = effectiveDashboardH() + wlH + kStatusBarH;
        auto area = bounds_.withTrimmedBottom(bottomH);
        if (ctx_.surfaceRight.isOpen() && ctx_.surfaceRight.isVisible())
        {
            const int rpW = std::min(SurfaceRightPanel::kPanelWidth,
                                     static_cast<int>(area.getWidth() * 0.40f));
            area = area.withTrimmedRight(rpW);
        }
        return area;
    }

    static juce::Point<float> polarToCartesian(float angle,
                                               float radius,
                                               juce::Point<float> center) noexcept
    {
        return {
            center.x + radius * std::cos(angle),
            center.y + radius * std::sin(angle)
        };
    }
};

} // namespace xoceanus
