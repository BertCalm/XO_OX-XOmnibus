// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// OceanView.h — Main radial container component for the XOceanus Ocean View.
//
// OceanView is the top-level UI component that replaces the 3-column
// ColumnLayoutManager-based Gallery layout.  It owns and orchestrates all
// Ocean sub-components in a radial composition: engine creatures orbit a
// central area, coupling threads connect them via CouplingSubstrate,
// and ambient layers wrap the whole scene.
//
// D6 (locked): NexusDisplay removed — preset identity lives in the HUD/DotMatrix
// strip (MasterFXStripCompact::setPresetName). DNA hexagons live in the preset
// browser overlay. See issue #1096.
//
// Architecture overview
// ─────────────────────
//   Z-order (bottom → top):
//     OceanBackground   — depth-gradient field
//     CouplingSubstrate — luminescent Bézier threads
//     EngineOrbit[0-4]  — creature orbital sprites
//     MacroSection      — 4 macro knobs (unique_ptr, needs APVTS)
//     EngineDetailPanel — detail panel (unique_ptr, needs Processor)
//     SidebarPanel      — settings/export/FX sidebar (unique_ptr)
//     AmbientEdge       — vignette + edge glow overlay
//     DnaMapBrowser     — full-window scatter map (BrowserOpen state)
//     DetailOverlay     — floating detail panel with backdrop (Step 4)
//     SubmarinePlaySurface — unified keyboard/pads surface
//     Floating controls — presetPrev/Next, fav, settings, KEYS
//     StatusBar         — bottom strip (unique_ptr)
//
// State machine
// ─────────────
//   Orbital       — all creatures orbit the centre (default)
//   ZoomIn        — one creature enlarged at centre, others minimised at edges
//   SplitTransform— 20% mini-orbital strip left, 80% EngineDetailPanel right
//   BrowserOpen   — full-window DnaMapBrowser
//
// Deferred init
// ─────────────
//   MacroSection, EngineDetailPanel, SidebarPanel, and StatusBar all require
//   references to the processor or APVTS at construction time.  OceanView
//   holds them as unique_ptrs and exposes initMacros(), initDetailPanel(),
//   initSidebar(), and initStatusBar() for the editor to call immediately
//   after construction before the component is made visible.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "OceanLayoutConstants.h"
#include "../Tokens.h"
#include "OceanBackground.h"
#include "AmbientEdge.h"
#include "EngineOrbit.h"
#include "CouplingSubstrate.h"
#include "CouplingConfigPopup.h"
#include "DnaMapBrowser.h"
#include "DetailOverlay.h"
// #include "PlaySurfaceOverlay.h"  -- cut(1B-#13): PlaySurfaceOverlay removed
#include "EnginePickerDrawer.h"
#include "SettingsDrawer.h"
#include "TideWaterline.h"
#include "ChordBarComponent.h"
#include "ChordBreakoutPanel.h"
#include "SeqBreakoutComponent.h"
#include "SeqStripComponent.h"
#include "MasterFXStripCompact.h"
#include "EpicSlotsPanel.h"
#include "TransportBar.h"
#include "ExpressionStrips.h"
#include "SubmarinePlaySurface.h"
#include "DotMatrixDisplay.h"
#include "SubmarineHudBar.h"
#include "SurfaceRightPanel.h"
#include "SubmarineMenuStyle.h"
#include "../Gallery/MacroSection.h"
#include "../Gallery/EngineDetailPanel.h"
#include "../Gallery/SidebarPanel.h"
#include "../Gallery/StatusBar.h"
#include "../Gallery/AdvancedFXPanel.h" // F3-002: MasterFX ADV button popup
#include "OceanChildren.h"
#include "OceanLayout.h"
#include "OceanStateMachine.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <vector>


namespace xoceanus
{

//==============================================================================
/**
    DimOverlay — #1008 FIX 7

    A transparent Component that sits in the Z-stack above the floating header
    buttons but below SubmarinePlaySurface.  When dimAlpha < 1.0 it fills its
    bounds with Ocean::abyss at (1 - dimAlpha) opacity, dimming everything
    underneath — including the header buttons that the old paint()-based rect
    could never reach because juce::Component::paint() draws behind children.

    setInterceptsMouseClicks(false, false) so the overlay is invisible to
    the event system and clicks pass through to SubmarinePlaySurface above it.
*/
struct DimOverlay : public juce::Component
{
    DimOverlay()
    {
        setInterceptsMouseClicks(false, false);
        setOpaque(false);
    }

    void setDimAlpha(float alpha)
    {
        if (std::abs(alpha - dimAlpha_) < 0.001f)
            return;
        dimAlpha_ = alpha;
        setVisible(dimAlpha_ < 0.999f);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        if (dimAlpha_ < 0.999f)
        {
            g.setColour(juce::Colour(GalleryColors::Ocean::abyss)
                            .withAlpha(1.0f - dimAlpha_));
            g.fillRect(getLocalBounds());
        }
    }

private:
    float dimAlpha_ = 1.0f;
};

//==============================================================================
/**
    OceanView

    The single top-level component of the XOceanus Ocean View redesign.
    Instantiated once by XOceanusEditor and sized to the full plugin window.

    Lifecycle
    ---------
    1. Construct OceanView.
    2. Call initMacros(apvts), initDetailPanel(proc), initSidebar(), initStatusBar()
       in any order before adding to the screen.
    3. Wire onEngineSelected, onEngineDiveDeep, onPresetSelected callbacks.
    4. Call setEngine(), setPresetName(), etc. as preset data arrives.
    5. Start a 10 Hz editor-side timer and call setCouplingRoutes() / setVoiceCount()
       from it to feed live state.
*/
class OceanView : public juce::Component,
                  private juce::Timer
{
public:
    //==========================================================================
    // View-state machine
    //==========================================================================

    // Phase 3 (#1184): ViewState unified.  The single canonical definition lives
    // in OceanStateMachine.  OceanView and OceanLayout both alias it via `using`
    // so all three classes share the same type — no static_casts needed.
    using ViewState = OceanStateMachine::ViewState;

    //==========================================================================
    // Wave 3 — Panel type registry (D4 locked)
    //==========================================================================

    /**
        PanelType — identifies each "heavy" panel that can be open at a time.

        Rule: only ONE heavy panel (anything that changes ocean layout or is
        full-window) may be open simultaneously.  PanelCoordinator enforces this.

        C4 note: ChainMatrix must call coordinator.requestOpen(PanelType::ChainMatrix)
        on open and coordinator.release(PanelType::ChainMatrix) on close.
        Use OceanView::getOrbitCenter(slotIndex) for chain-line anchor points.

    */
    enum class PanelType
    {
        None,             ///< No heavy panel open
        EnginePicker,     ///< EnginePickerDrawer — slides from left, dims ocean
        Settings,         ///< SettingsDrawer — slides from right, dims ocean
        Detail,           ///< EngineDetailPanel (EngineDetailPanel*) — full-window
        ChainMatrix,      ///< (Wave 5 C4) chain matrix slide-up — stub, no-op open/close
    };

    //==========================================================================
    // Construction / destruction
    //==========================================================================

    OceanView()
    {
        // ── Assign slot indices to orbits before wiring callbacks ─────────────
        for (int i = 0; i < 5; ++i)
            orbits_[i].setSlotIndex(i);

        // ── Z-ordered addAndMakeVisible sequence (bottom → top) ───────────────
        // 1. Background field
        addAndMakeVisible(background_);

        // 2. Coupling substrate (transparent overlay, sits above background)
        addAndMakeVisible(substrate_);

        // 3. Engine creature orbits (4 primary + 1 ghost slot)
        for (auto& orbit : orbits_)
            addAndMakeVisible(orbit);

        // 4. (D6) NexusDisplay removed — preset identity lives in MasterFXStripCompact
        //    dot-matrix. DNA hexagons live in the preset browser overlay. (#1096)

        // 5. Macro section (conditionally visible; placeholder until initMacros())
        // macros_ lives in children_ — added via children_.initMacros()

        // 6. AmbientEdge: vignette + edge glow (top of background stack)
        addAndMakeVisible(ambientEdge_);

        // 7. Detail panel placeholder until initDetailPanel()
        // detail_ lives in children_ — added via children_.initDetailPanel()

        // 8. Sidebar placeholder until initSidebar()
        // sidebar_ lives in children_ — added via children_.initSidebar()

        // 9. DNA map browser (hidden by default)
        // Use addChildComponent so it starts hidden without a visible flash.
        addChildComponent(browser_);

        // 9c. Detail overlay (hidden by default; floats above orbits/substrate)
        addChildComponent(detailOverlay_);

        // 9e. Coupling config popup (hidden by default; shown on knot double-click)
        addChildComponent(couplingPopup_);

        // 9b. BLOCKER 1: Empty-state label — shown when no engines are loaded.
        // Appears centred below the nexus with a subtle call-to-action.
        emptyStateLabel_.setText("Dive in — click a ghost slot to load an engine",
                                 juce::dontSendNotification);
        emptyStateLabel_.setFont(GalleryFonts::label(13.0f));
        emptyStateLabel_.setColour(juce::Label::textColourId,
                                   juce::Colour(GalleryColors::Ocean::foam).withAlpha(0.55f));
        emptyStateLabel_.setJustificationType(juce::Justification::centred);
        emptyStateLabel_.setVisible(false);  // visible only when numLoaded == 0
        addAndMakeVisible(emptyStateLabel_);

        // 9d. Step 6: Dashboard waterline + tab bar.
        // waterline_ lives in children_ — added via children_.initWaterline().
        addAndMakeVisible(tabBar_);

        // 9f. Expression strips (PB + MW) — always visible in play area.
        addAndMakeVisible(exprStrips_);
        // wire(1C-1): Forward ExpressionStrips callbacks outward so the editor can
        // route pitch bend and mod wheel to the processor's MIDI collector.
        exprStrips_.onPitchBend = [this](float v) { if (onExpressionPitchBend) onExpressionPitchBend(v); };
        exprStrips_.onModWheel  = [this](float v) { if (onExpressionModWheel)  onExpressionModWheel(v); };

        // 9g-vis. Dot-matrix visualizer — fills empty space right of macros.
        addAndMakeVisible(dotMatrix_);

        // 9h-hud. Floating HUD nav bar — top of ocean viewport.
        addAndMakeVisible(hudBar_);

        // 9g. Submarine play surface (replaces Gallery PlaySurface).
        addAndMakeVisible(subPlaySurface_);

        // 9h. Right-side panel for PAD/DRUM/XY modes.
        surfaceRight_.setVisible(false);
        surfaceRight_.onCloseClicked = [this]()
        {
            // D5 (1D-P2B): Drive all state through tabBar_.selectTab(0) so that
            // currentTab_, surfaceRight_ visibility, DashboardTabBar highlight, and
            // LATCH indicator are all updated via the single onTabChanged path.
            // selectTab() is a no-op if KEYS is already active.
            tabBar_.selectTab(0); // 0 = KEYS
        };
        addAndMakeVisible(surfaceRight_);

        // 11. Floating header controls
        // Old Gallery floating header buttons — hidden, replaced by SubmarineHudBar.
        enginesButton_.setVisible(false);
        enginesButton_.setInterceptsMouseClicks(false, false);
        presetPrev_.setVisible(false);
        presetPrev_.setInterceptsMouseClicks(false, false);
        presetNext_.setVisible(false);
        presetNext_.setInterceptsMouseClicks(false, false);
        favButton_.setVisible(false);
        favButton_.setInterceptsMouseClicks(false, false);
        settingsButton_.setVisible(false);
        settingsButton_.setInterceptsMouseClicks(false, false);
        keysButton_.setVisible(false);
        keysButton_.setInterceptsMouseClicks(false, false);

        // 11b. #1008 FIX 7: DimOverlay sits above all buttons but below
        // SubmarinePlaySurface.  Added after the buttons so it is painted on top.
        // reorderZStack() will enforce the correct final ordering on each
        // deferred init call.
        // Use addChildComponent so it starts hidden without a visible flash.
        addChildComponent(dimOverlay_);

        // 12. StatusBar placeholder until initStatusBar()
        // statusBar_ lives in children_ — added via children_.initStatusBar()

        // ── Button styling ────────────────────────────────────────────────────
        // #1007 FIX 1: Add PointingHandCursor + hover state so buttons look
        // polished rather than raw WinAmp-era TextButtons.
        auto styleHeaderButton = [this](juce::TextButton& btn, int minW)
        {
            btn.setColour(juce::TextButton::buttonColourId,
                          juce::Colour(GalleryColors::Ocean::deep));
            btn.setColour(juce::TextButton::buttonOnColourId,
                          juce::Colour(GalleryColors::xoGold).withAlpha(0.25f));
            btn.setColour(juce::TextButton::textColourOffId,
                          juce::Colour(GalleryColors::Ocean::foam));
            btn.setColour(juce::TextButton::textColourOnId,
                          juce::Colour(GalleryColors::xoGold));
            // Pointing cursor makes the interactive affordance unambiguous.
            btn.setMouseCursor(juce::MouseCursor::PointingHandCursor);
            // Hover tint: XO Gold at 15% alpha blended over Ocean::deep.
            // We wire mouseEnter/mouseExit to flip a gold highlight on the
            // button background colour so the hover state is visible.
            btn.onStateChange = [&btn]()
            {
                const bool hovered = btn.isOver();
                const juce::Colour baseColour = juce::Colour(GalleryColors::Ocean::deep);
                const juce::Colour hoverColour =
                    baseColour.interpolatedWith(juce::Colour(GalleryColors::xoGold), 0.15f);
                btn.setColour(juce::TextButton::buttonColourId,
                              hovered ? hoverColour : baseColour);
            };
            // #908: initial size hint — layoutFloatingControls() sets definitive bounds.
            btn.setSize(minW, 44); // 44pt height = WCAG AA minimum touch target
            juce::ignoreUnused(this);
        };
        styleHeaderButton(enginesButton_, 90);
        styleHeaderButton(presetPrev_,   44);
        styleHeaderButton(presetNext_,   44);
        styleHeaderButton(favButton_,    44);
        styleHeaderButton(settingsButton_, 44);
        styleHeaderButton(keysButton_,   56);

        // #1007 FIX 1: Tooltip text as fallback label for Unicode icon buttons.
        enginesButton_.setTooltip("Toggle Engine Library");
        favButton_.setTooltip("Favourite");
        settingsButton_.setTooltip("Settings");
        keysButton_.setTooltip("Toggle Play Surface (K)");
        presetPrev_.setTooltip("Previous preset");
        presetNext_.setTooltip("Next preset");

        // #1007 FIX 3: Inline preset name label between < and > buttons.
        // This creates spatial grouping so users understand the navigation relationship.
        presetNameLabel_.setFont(GalleryFonts::label(12.0f));
        presetNameLabel_.setColour(juce::Label::textColourId,
                                   juce::Colour(GalleryColors::Ocean::foam));
        presetNameLabel_.setJustificationType(juce::Justification::centred);
        presetNameLabel_.setInterceptsMouseClicks(true, false); // absorb clicks without passing to parent
        addChildComponent(presetNameLabel_); // hidden — preset info in FX strip
        A11y::setup(presetNameLabel_, "Current preset", "Name of the currently loaded preset");

        A11y::setup(keysButton_,      "Keys toggle", "Show or hide the Play Surface panel");
        A11y::setup(settingsButton_,  "Settings");
        A11y::setup(favButton_,       "Favourite");
        A11y::setup(presetPrev_,      "Previous preset");
        A11y::setup(presetNext_,      "Next preset");

        // ── Internal callbacks ────────────────────────────────────────────────
        for (int i = 0; i < 5; ++i)
        {
            orbits_[i].onClicked       = [this](int s) { handleOrbitClicked(s); };
            orbits_[i].onDoubleClicked = [this](int s)
            {
                auto* dp = children_.detailPanel();
                if (!dp) return;

                // Wave 3 3b / D7: register Detail as the active heavy panel.
                // coordinatorRequestOpen hides SurfaceRightPanel if it was open,
                // and closes any competing heavy panel (EnginePicker/Settings).
                coordinatorRequestOpen(PanelType::Detail);

                // Position as compact band centered in the ocean area
                {
                    auto ocean = getOceanArea().reduced(40, 0);
                    int panelH = juce::jmin(ocean.getHeight(), 280);
                    int panelY = ocean.getY() + (ocean.getHeight() - panelH) / 2;
                    dp->setBounds(ocean.withHeight(panelH).withY(panelY));
                }
                dp->loadSlot(s);
                dp->setVisible(true);
                dp->resized();

                // Nuclear Z-order: remove and re-add as the LAST child
                removeChildComponent(dp);
                addAndMakeVisible(*dp);
                detailShowing_ = true;
            };
            orbits_[i].onPositionChanged = [this](int slot)
            {
                auto pos = orbits_[slot].getNormalizedPosition();
                auto area = getOceanArea();
                int sz = orbits_[slot].getBuoySize() + kBreathPadding * 2;
                int x = static_cast<int>(pos.x * area.getWidth()) - sz / 2;
                int y = static_cast<int>(pos.y * area.getHeight()) - sz / 2;
                // Clamp to keep buoy fully inside ocean area
                x = juce::jlimit(0, area.getWidth() - sz, x);
                y = juce::jlimit(0, area.getHeight() - sz, y);
                orbits_[slot].setBounds(x + area.getX(), y + area.getY(), sz, sz);
                substrate_.setCreatureCenter(slot, orbits_[slot].getCenter());
                saveSlotPosition(slot);
            };
            orbits_[i].onDragMoved = [this](int slot)
            {
                substrate_.setCreatureCenter(slot, orbits_[slot].getVisualCenter());
            };
            // Q1 (#1356): forward preset pill click outward to editor.
            orbits_[i].onPresetPillClicked = [this](int slot)
            {
                if (onPresetPillClicked)
                    onPresetPillClicked(slot);
            };
        }

        // ── CouplingSubstrate knot interaction ────────────────────────────────
        substrate_.onKnotDoubleClicked = [this](int routeIndex)
        {
            showCouplingPopupForRoute(routeIndex);
        };

        couplingPopup_.onConfigChanged = [this](int routeIndex, int newType, float newDepth, int direction)
        {
            if (onCouplingConfigChanged)
                onCouplingConfigChanged(routeIndex, newType, newDepth, direction);
        };

        couplingPopup_.onRemove = [this](int routeIndex)
        {
            if (onCouplingDeleteRequested)
                onCouplingDeleteRequested(routeIndex);
        };

        substrate_.onKnotRightClicked = [this](int routeIndex, juce::Point<int> /*screenPos*/)
        {
            showKnotContextMenu(routeIndex);
        };

        browser_.onPresetSelected = [this](int idx)
        {
            if (onPresetSelected)
                onPresetSelected(idx);
            exitBrowser();
        };
        browser_.onDismissed = [this]() { exitBrowser(); };

        keysButton_.onClick = []() { /* KEYS tab activates keyboard in dashboard */ };

        // ── Step 6: Dashboard tab bar callback ───────────────────────────────
        // Wave 6.5 (#1306) collision note:
        //   PAD/XY tabs open SurfaceRightPanel.  All collision rules are already
        //   enforced by Wave 3 PanelCoordinator:
        //     (a) coordinatorApplyWidthGuard() — closes drawers when width < kMinWidth px.
        //     (b) coordinatorRequestOpen(PanelType::Detail) — hides SurfaceRightPanel
        //         while DetailOverlay is open; restored on coordinatorRelease().
        //   SurfaceRightPanel is a soft panel and intentionally coexists with
        //   drawers above kMinWidth px.  No additional coordinator call is required here.
        tabBar_.onTabChanged = [this](const juce::String& tab)
        {
            // D5 (1D-P2B): Set currentTab_ BEFORE visibility ops so layoutDashboard
            // reads the new canonical tab state (not a stale surfaceRight_.isOpen()).
            if (tab == "KEYS")        currentTab_ = OceanCurrentTab::Keys;
            else if (tab == "PAD")    currentTab_ = OceanCurrentTab::Pad;
            else if (tab == "XY")     currentTab_ = OceanCurrentTab::XY;

            // 1D-2A C2: single-controller pattern — onTabChanged owns surfaceRight_
            // open/visible state; OceanLayout::layoutDashboard() derives
            // subPlaySurface_ visibility from currentTab_ on the resized() pass below.
            if (tab == "KEYS")
            {
                // KEYS: right panel closed → layoutDashboard will show keyboard.
                // Track A (1D-P2B): visibility logic — what panels are shown.
                juce::Component* outgoing = surfaceRight_.isVisible() ? &surfaceRight_ : nullptr;
                surfaceRight_.setOpen(false);
                surfaceRight_.setVisible(false);
                // Track B (#18): cross-fade envelope wraps visibility ops.
                startTabCrossFade(outgoing, &subPlaySurface_);
            }
            else
            {
                // PAD/XY: right panel opens → layoutDashboard will hide keyboard.
                // Track A (1D-P2B): D1 sub-mode drives PAD vs DRUM grid.
                if (tab == "PAD")
                {
                    const bool kitMode = tabBar_.isKitSubMode();
                    surfaceRight_.setMode(kitMode ? SurfaceRightPanel::Mode::Drum
                                                  : SurfaceRightPanel::Mode::Pad);
                }
                else if (tab == "XY")
                    surfaceRight_.setMode(SurfaceRightPanel::Mode::XY);

                // Track A (1F P0-3): Guard against re-opening SurfaceRightPanel when the
                // Detail overlay is active.
                if (currentPanel_ != PanelType::Detail)
                {
                    juce::Component* outgoing = subPlaySurface_.isVisible() ? &subPlaySurface_ : nullptr;
                    surfaceRight_.setOpen(true);
                    surfaceRight_.setVisible(true);
                    // Track B (#18): cross-fade envelope wraps visibility ops.
                    startTabCrossFade(outgoing, &surfaceRight_);
                }
            }

            // Track A (1D-P2B): update LATCH indicator in status bar.
            if (auto* tb = children_.transportBar())
                tb->setLatchActive(tab == "KEYS");

            // Track A (1F P1-8): Forward tab index to editor for persistence.
            if (onDashboardTabChanged)
            {
                const int idx = (tab == "PAD") ? 1 : (tab == "XY") ? 2 : 0;
                onDashboardTabChanged(idx);
            }

            resized();
        };

        // D1 (#18 follow-on): sub-mode toggle in tab bar fires when NOTE/KIT pill clicked.
        // If PAD tab is currently active, immediately update SurfaceRightPanel mode.
        tabBar_.onNoteKitToggled = [this](bool kitMode)
        {
            if (currentTab_ == OceanCurrentTab::Pad)
                surfaceRight_.setMode(kitMode ? SurfaceRightPanel::Mode::Drum
                                              : SurfaceRightPanel::Mode::Pad);

            // P1-8 (1F): Forward kit sub-mode to editor for persistence.
            if (onDashboardKitSubModeChanged)
                onDashboardKitSubModeChanged(kitMode);
        };

        // SEQ toggle → expand/collapse TideWaterline.
        tabBar_.onSeqToggled = [this](bool on)
        {
            if (auto* wl = children_.waterline())
                wl->setExpanded(on);
        };

        // CHORD toggle → show/hide ChordBarComponent.
        tabBar_.onChordToggled = [this](bool on)
        {
            if (auto* cb = children_.chordBar())
            {
                cb->setVisible(on);
                resized(); // re-layout dashboard to accommodate chord bar
            }
        };

        // ── DetailOverlay callbacks ───────────────────────────────────────────
        // wire(1C-5): onShown — register Detail with the coordinator when the overlay
        // is explicitly shown via DetailOverlay::show() (future code path; show() is
        // currently not called but the callback is wired for forward-compatibility).
        detailOverlay_.onShown = [this]()
        {
            coordinatorRequestOpen(PanelType::Detail);
        };

        detailOverlay_.onHidden = [this]()
        {
            if (auto* dp = children_.detailPanel())
                dp->setVisible(false);
            // Wave 3 3b / D7: release Detail from coordinator so SurfaceRightPanel
            // is restored to its prior open state (surfaceRightWasOpenForDetail_).
            coordinatorRelease(PanelType::Detail);
        };

        // ── Buoy positions — load saved positions (or defaults) ──────────────
        loadSlotPositions();

        // ── Step 7: First-launch lifesaver ────────────────────────────────────
        addChildComponent(lifesaver_); // hidden by default, shown when no engines loaded
        lifesaver_.onClick = [this]()
        {
            firstLaunch_ = false;
            lifesaver_.setVisible(false);
            // Open engine picker drawer via coordinator (Wave 3 3b).
            coordinatorRequestOpen(PanelType::EnginePicker);
        };

        // ── Phase 3: Engine picker drawer — routed through PanelCoordinator ──
        addChildComponent(engineDrawer_); // starts hidden; toggle via enginesButton_
        engineDrawer_.onEngineSelected = [this](const juce::String& engineId)
        {
            coordinatorRelease(PanelType::EnginePicker);
            if (onEnginePickerRequested)
                onEnginePickerRequested();
            if (onEngineSelectedFromDrawer)
                onEngineSelectedFromDrawer(engineId);
        };
        enginesButton_.onClick = [this]()
        {
            // Toggle: close if already the active panel, otherwise request open.
            if (currentPanel_ == PanelType::EnginePicker)
                coordinatorRelease(PanelType::EnginePicker);
            else
                coordinatorRequestOpen(PanelType::EnginePicker);
        };

        // ── Settings drawer — routed through PanelCoordinator ────────────────
        addChildComponent(settingsDrawer_); // starts hidden; toggle via settingsButton_
        settingsButton_.onClick = [this]()
        {
            if (currentPanel_ == PanelType::Settings)
                coordinatorRelease(PanelType::Settings);
            else
                coordinatorRequestOpen(PanelType::Settings);
        };
        // wire(#orphan-sweep item 5): forward setting changes outward via OceanView's
        // onSettingChanged callback.  Also handle waveSensitivity locally (routes to
        // OceanBackground reactivity).  All other keys go straight to the editor.
        settingsDrawer_.onSettingChanged = [this](const juce::String& key, float value)
        {
            if (key == "waveSensitivity")
            {
                // waveSensitivity [0,1] → ocean background reactivity.
                background_.setReactivity(value);
                hudBar_.setReactLevel(value);
            }
            // Forward all settings changes to the editor so it can route remaining
            // keys to the processor or APVTS where receivers exist.
            if (onSettingChanged)
                onSettingChanged(key, value);
        };

        // ── HUD bar callbacks — routed through PanelCoordinator ──────────────
        hudBar_.onEnginesClicked = [this]()
        {
            if (currentPanel_ == PanelType::EnginePicker)
                coordinatorRelease(PanelType::EnginePicker);
            else
                coordinatorRequestOpen(PanelType::EnginePicker);
        };
        hudBar_.onSettingsClicked = [this]()
        {
            if (currentPanel_ == PanelType::Settings)
                coordinatorRelease(PanelType::Settings);
            else
                coordinatorRequestOpen(PanelType::Settings);
        };

        hudBar_.onUndo = [this]() { if (onUndoRequested) onUndoRequested(); };
        hudBar_.onRedo = [this]() { if (onRedoRequested) onRedoRequested(); };

        // REACT dial → OceanBackground visual reactivity (Piece 1, Wave 1B).
        // D1 (locked): REACT value scales waveform amplitude, ring pulse magnitude,
        // gradient brightness.  NOT master volume — that lives on the
        // VOLUME macro knob (D11).
        constexpr float kDefaultReactLevel = 0.80f;
        hudBar_.onReactChanged = [this](float value01)
        {
            background_.setReactivity(value01);
            // F3-006: Notify editor so it can persist the new value to the processor.
            if (onReactLevelChanged)
                onReactLevelChanged(value01);
        };
        background_.setReactivity(kDefaultReactLevel);

        // FIX 11: Chain mode toggles crosshair cursor over the ocean viewport
        // and clears any in-progress chain drawing on the substrate.
        hudBar_.onChainToggled = [this]() { applyChainModeVisuals(); };

        // fix(#1354): forward the 6 previously-unwired HUD bar callbacks outward
        // so the editor can route them to PresetManager / ABCompare / ExportDialog.
        hudBar_.onSave = [this]()
        {
            if (onSavePreset)
                onSavePreset();
        };

        hudBar_.onFavChanged = [this](bool newFavState)
        {
            if (onFavToggled)
                onFavToggled(newFavState);
        };

        hudBar_.onABCompareChanged = [this](bool active)
        {
            if (onABCompareToggled)
                onABCompareToggled(active);
        };

        hudBar_.onPresetPrev = [this]()
        {
            if (onPresetPrev)
                onPresetPrev();
        };

        hudBar_.onPresetNext = [this]()
        {
            if (onPresetNext)
                onPresetNext();
        };

        hudBar_.onExportClicked = [this]()
        {
            if (onExportClicked)
                onExportClicked();
        };

        // Preset name label click → open preset browser (sidebar Preset tab).
        hudBar_.onPresetNameClicked = [this]()
        {
            if (onPresetNameClicked)
                onPresetNameClicked();
        };

        // ── Keyboard focus ────────────────────────────────────────────────────
        setWantsKeyboardFocus(true);

        // ── Shared orbit animation timer ──────────────────────────────────────
        // One 30 Hz timer drives all EngineOrbit animations in lock-step,
        // synchronizing breathe/bob/wreath phases and reducing OS timer allocations.
        startTimerHz(30);

        // ── Phase 2 (#1184): construct OceanLayout now that all component members exist ─
        // LayoutTargets is a struct of component references; it captures the
        // address of each member so must be built after all members are alive.
        // layout_ must be constructed last in the constructor body.
        layout_ = std::make_unique<OceanLayout>(
            children_,
            LayoutTargets{
                background_,
                substrate_,
                orbits_,
                ambientEdge_,
                browser_,
                detailOverlay_,
                couplingPopup_,
                dimOverlay_,      // typed as juce::Component& in LayoutTargets
                emptyStateLabel_,
                lifesaver_,       // typed as juce::Component& in LayoutTargets
                enginesButton_,
                presetPrev_,
                presetNext_,
                favButton_,
                settingsButton_,
                keysButton_,
                presetNameLabel_,
                engineDrawer_,
                settingsDrawer_,
                hudBar_,
                dotMatrix_,
                tabBar_,
                exprStrips_,
                subPlaySurface_,
                surfaceRight_,
                // Phase 2.5 (#1184): layout-input state bindings (const-ref).
                selectedSlot_,
                detailShowing_,
                firstLaunch_,
                // D5 (1D-P2B): canonical tab state for keyboard visibility.
                currentTab_,
            });

        // ── Phase 3 (#1184): wire OceanStateMachine callbacks ────────────────
        // onStateEntered: a completed transition triggers layout + repaint.
        // Also syncs OceanView's mirror fields (viewState_, selectedSlot_) so
        // any remaining OceanView code that still reads them is consistent.
        // Step 9 removes the mirror fields; until then keep them in sync here.
        stateMachine_.onStateEntered = [this](OceanStateMachine::ViewState s)
        {
            // Sync selectedSlot_ mirror (removed in step 9 final cleanup).
            // viewState_ mirror removed in step 9 — read stateMachine_.currentState() instead.
            selectedSlot_ = stateMachine_.selectedSlot();

            jassert(layout_ != nullptr);
            layout_->layoutForState(s, getLocalBounds(), 1.0f);
            layout_->reorderZStack();
            repaint();

            // F2-006: Notify editor of ViewState change so it can persist the state.
            if (onViewStateChanged)
                onViewStateChanged(static_cast<int>(s), stateMachine_.selectedSlot());
        };

        // onAnimationFrame: stubbed for future animated transitions.
        // Transitions are currently instantaneous so this is never fired.
        stateMachine_.onAnimationFrame = [this](OceanStateMachine::ViewState s,
                                                float progress01)
        {
            jassert(layout_ != nullptr);
            layout_->layoutForState(s, getLocalBounds(), progress01);
            repaint();
        };
    }

    ~OceanView() override
    {
        stopTimer();
    }

    //==========================================================================
    // Deferred initialisation — called by XOceanusEditor before first show
    //
    // Each public initX() method is now a thin wrapper:
    //   1. delegates construction + addChild* to children_.initX()
    //   2. wires any callbacks that reference OceanView state (callbacks cannot
    //      live inside OceanChildren — they would create a back-reference)
    //   3. calls reorderZStack() — delegates to layout_.reorderZStack() (Phase 2)
    //   4. calls resized() where a layout pass is required
    //
    // Phase 3 will move state-machine callbacks into OceanStateMachine.
    //==========================================================================

    /** Wire macro knobs to the AudioProcessorValueTreeState. */
    void initMacros(juce::AudioProcessorValueTreeState& apvts)
    {
        children_.initMacros(apvts);
        reorderZStack();
        resized();
    }

    /** Wire the EngineDetailPanel to the processor. */
    void initDetailPanel(XOceanusProcessor& proc)
    {
        children_.initDetailPanel(proc);
        // Wire callback here — it references OceanView::dismissDetailPanel().
        children_.detailPanel()->onBackClicked = [this]() { dismissDetailPanel(); };
        reorderZStack();
        resized();
    }

    /** Initialise the SidebarPanel. */
    void initSidebar()
    {
        children_.initSidebar();
        reorderZStack();
        resized();
    }

    /**
        Initialise the TideWaterline (submarine step sequencer strip).
        Must be called after the processor is available — needs APVTS and
        the MasterFXSequencer reference for playhead tracking.
    */
    void initWaterline(juce::AudioProcessorValueTreeState& apvts,
                       const MasterFXSequencer& sequencer)
    {
        children_.initWaterline(apvts, sequencer);
        // Wire callback here — it calls OceanView::resized().
        children_.waterline()->onHeightChanged = [this]() { resized(); };
        reorderZStack();
    }

    /**
        Initialise the ChordBarComponent (submarine-style chord strip).
        Needs APVTS + ChordMachine reference.
    */
    void initChordBar(juce::AudioProcessorValueTreeState& apvts,
                      const ChordMachine& chordMachine)
    {
        children_.initChordBar(apvts, chordMachine);
        // wire(#orphan-sweep item 7): onVisibilityChanged and onInputModeChanged
        // were never assigned.  Wire them here so state changes propagate to the
        // editor/processor.  onVisibilityChanged also triggers a layout pass
        // because showing/hiding the chord bar changes the dashboard height.
        if (auto* cb = children_.chordBar())
        {
            cb->onVisibilityChanged = [this]()
            {
                resized(); // dashboard height changes when chord bar is shown/hidden
                if (onChordBarVisibilityChanged)
                    onChordBarVisibilityChanged();
            };
            cb->onInputModeChanged = [this](ChordBarComponent::InputMode mode)
            {
                if (onChordBarInputModeChanged)
                    onChordBarInputModeChanged(mode);
            };
        }
        reorderZStack();
    }

    /**
        Initialise the ChordBreakoutPanel (Wave 5 B3 mount).
        Must be called after initChordBar() — needs APVTS + ChordMachine reference.
    */
    void initChordBreakout(juce::AudioProcessorValueTreeState& apvts,
                           const ChordMachine& chordMachine)
    {
        children_.initChordBreakout(apvts, chordMachine);
        // F3-017: Wire chord breakout toggle → outbound callback for persistence.
        if (auto* panel = children_.chordBreakout())
        {
            panel->onBreakoutToggled = [this](bool isOpen)
            {
                if (onChordBreakoutToggled)
                    onChordBreakoutToggled(isOpen);
            };
        }
        reorderZStack();
    }

    /**
        Initialise the SeqStrip + SeqBreakout (Wave 5 C2 mount).
        Must be called after the processor is available — needs APVTS.
    */
    void initSeqStrip(juce::AudioProcessorValueTreeState& apvts)
    {
        children_.initSeqStrip(apvts);
        // F3-011: Wire seq breakout toggle → outbound callback for persistence.
        if (auto* strip = children_.seqStrip())
        {
            strip->onBreakoutToggled = [this](bool isOpen)
            {
                if (onSeqBreakoutToggled)
                    onSeqBreakoutToggled(isOpen);
            };
        }
        reorderZStack();
    }

    /** Initialise the compact Master FX strip (submarine-style). */
    void initMasterFxStrip(juce::AudioProcessorValueTreeState& apvts)
    {
        children_.initMasterFxStrip(apvts);

        // F3-002: Wire ADV buttons to launch AdvancedFXPanel popovers.
        // Each section maps to a set of "hidden" advanced parameters not exposed
        // on the main strip knobs.  Parameter sets mirror MasterFXSection.h.
        if (auto* strip = children_.masterFxStrip())
        {
            // Capture apvts by reference — safe because apvts outlives the editor.
            strip->onAdvClicked = [this, &apvts](int sectionIdx)
            {
                using ParamList = std::vector<std::pair<juce::String, juce::String>>;
                juce::String title;
                ParamList params;

                switch (sectionIdx)
                {
                    case 0: // SAT
                        title  = "SAT ADVANCED";
                        params = { {"master_satMode", "MODE"} };
                        break;
                    case 1: // DELAY
                        title  = "DELAY ADVANCED";
                        params = { {"master_delayTime",      "TIME"},
                                   {"master_delayPingPong",  "P.PONG"},
                                   {"master_delayDamping",   "DAMP"},
                                   {"master_delayDiffusion", "DIFF"},
                                   {"master_delaySync",      "SYNC"} };
                        break;
                    case 2: // REVERB
                        title  = "REVERB ADVANCED";
                        // I4B: Previously only 1 of 8 reverb params was listed.
                        // Expanded to all 7 user-facing parameters (IDs from XOceanusProcessor.cpp).
                        params = { {"master_reverbSize",      "SIZE"},
                                   {"master_reverbPreDelay",  "PRE"},
                                   {"master_reverbDecay",     "DECAY"},
                                   {"master_reverbDamping",   "DAMP"},
                                   {"master_reverbDiffusion", "DIFF"},
                                   {"master_reverbMod",       "MOD"},
                                   {"master_reverbWidth",     "WIDTH"} };
                        break;
                    case 3: // MOD
                        title  = "MOD ADVANCED";
                        params = { {"master_modRate",     "RATE"},
                                   {"master_modMix",      "MIX"},
                                   {"master_modMode",     "MODE"},
                                   {"master_modFeedback", "FB"} };
                        break;
                    case 4: // COMP
                        title  = "COMP ADVANCED";
                        params = { {"master_compRatio",   "RATIO"},
                                   {"master_compAttack",  "ATTACK"},
                                   {"master_compRelease", "RELEASE"} };
                        break;
                    default:
                        return; // unknown section — no-op
                }

                // Anchor the CallOutBox to the strip's screen bounds so it appears
                // adjacent to the ADV button that was clicked.
                // Re-read the strip pointer at call-time to avoid capturing the local var.
                juce::Rectangle<int> bounds;
                if (auto* s = children_.masterFxStrip())
                    bounds = s->getScreenBounds();
                // I4A: Capture parent at lambda creation time via explicit this->getTopLevelComponent().
                // Inside a custom-drawn MasterFXStripCompact, an unqualified getTopLevelComponent()
                // resolved through the strip's own scope may return null — using OceanView's
                // (captured this) top-level component ensures a valid parent for CallOutBox.
                auto* parentComp = this->getTopLevelComponent();
                if (parentComp == nullptr) parentComp = this;
                juce::CallOutBox::launchAsynchronously(
                    std::make_unique<AdvancedFXPanel>(apvts, title, params),
                    bounds, parentComp);
            };

            // I2: onPresetNav and onPresetClicked were declared but never assigned —
            // the "Welcome" preset menu was completely dead.  Wire them here using the
            // same OceanView-level callbacks that SubmarineHudBar uses (line ~645).
            strip->onPresetNav = [this](int direction)
            {
                if (direction < 0) { if (onPresetPrev) onPresetPrev(); }
                else               { if (onPresetNext) onPresetNext(); }
            };

            strip->onPresetClicked = [this]()
            {
                if (onPresetNameClicked) onPresetNameClicked();
            };
        }

        reorderZStack();
    }

    /**
        Initialise the Epic Slots panel — the 3-slot FX chain picker for the
        EpicChainSlotController. Mount beneath the Master FX strip.
    */
    void initEpicSlotsPanel(juce::AudioProcessorValueTreeState& apvts)
    {
        children_.initEpicSlotsPanel(apvts);
        reorderZStack();
    }

    /** Initialise the TransportBar (submarine-style bottom status strip). */
    void initTransportBar()
    {
        children_.initTransportBar();
        // wire(#orphan-sweep item 6): onTimeSigChanged was never assigned.
        // Forward time-sig changes outward via OceanView::onTimeSigChanged so
        // the editor can route them to the processor / APVTS once a receiver exists.
        if (auto* tb = children_.transportBar())
        {
            tb->onTimeSigChanged = [this](int num, int den)
            {
                if (onTimeSigChanged)
                    onTimeSigChanged(num, den);
            };
        }
        reorderZStack();
    }

    /// Get the TransportBar so the editor can push BPM/voices/CPU.
    TransportBar*      getTransportBar() noexcept { return children_.transportBar(); }
    TideWaterline*     getWaterline()    noexcept { return children_.waterline(); }
    DotMatrixDisplay*  getDotMatrix()    noexcept { return &dotMatrix_; }
    /// Get the SurfaceRightPanel so the editor can wire onOuijaCCOutput.
    SurfaceRightPanel& getSurfaceRight() noexcept { return surfaceRight_; }

    // (Starboard wiring uses XOceanusEditor::playSurface_ directly, not OceanView.
    //  OceanView only owns SubmarinePlaySurface; XOuijaPanel removed 2026-05-01.)

    /**
        Initialise the StatusBar.
        Must be the last deferred-init call — it sets fullyInitialised_ = true
        and triggers the first valid resized() pass.
    */
    void initStatusBar()
    {
        children_.initStatusBar();
        reorderZStack();

        // #1007 FIX 4: All deferred-init methods have now been called.
        // Unlock resized() and paint() before the first layout pass.
        fullyInitialised_ = true;
        resized();
    }

    //==========================================================================
    // juce::Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
        // #1007 FIX 4: Skip paint until fully initialised to avoid rendering
        // with partially-constructed child components.
        if (!fullyInitialised_)
            return;

        // #1008 FIX 7: The old paint()-based dim rect was behind all children
        // so header buttons were never dimmed.  Dimming is now handled by
        // dimOverlay_ (a transparent child component that sits above header
        // buttons but below PlaySurfaceOverlay).  Nothing to paint here.
        juce::ignoreUnused(g);
    }

    void resized() override
    {
        // #1007 FIX 4: Don't layout before all deferred init methods have run.
        // initMacros / initDetailPanel / initSidebar / initStatusBar must all be
        // called before the component becomes interactive.
        if (!fullyInitialised_)
            return;

        // Wave 3 3b: Minimum-width guard — if window narrows enough to collide
        // a drawer with SurfaceRightPanel, close the drawer via coordinator.
        coordinatorApplyWidthGuard();

        // Phase 2+3 (#1184): all layout/geometry logic lives in OceanLayout.
        // OceanView passes only the state arguments; OceanLayout owns the
        // setBounds/setVisible calls for every child component.
        //
        // Z-order note: reorderZStack() is NOT called from here (#1163).
        // It is called exactly once per setup phase (after each initX()), and
        // on each visibility toggle that needs a re-stack.
        //
        // Phase 3: ViewState is now a unified alias (OceanStateMachine::ViewState)
        // shared by OceanView, OceanLayout, and OceanStateMachine — no static_cast.
        jassert(layout_ != nullptr);
        layout_->layoutForState(stateMachine_.currentState(), getLocalBounds());
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        // Escape: close engine drawer or settings drawer first, then exit overlays, then return to Orbital.
        if (key == juce::KeyPress::escapeKey)
        {
            if (settingsDrawer_.isOpen())
            {
                settingsDrawer_.close();
                return true;
            }
            if (engineDrawer_.isOpen())
            {
                engineDrawer_.close();
                return true;
            }
            if (chainStartSlot_ >= 0)
            {
                chainStartSlot_ = -1;
                substrate_.setChainInProgress(false);
                hudBar_.setChainModeActive(false);
                setMouseCursor(juce::MouseCursor::NormalCursor);
                return true;
            }
            if (detailShowing_)
            {
                dismissDetailPanel();
                return true;
            }
            if (stateMachine_.currentState() == ViewState::BrowserOpen)
            {
                exitBrowser();
                return true;
            }
            if (stateMachine_.currentState() != ViewState::Orbital)
            {
                transitionToOrbital();
                return true;
            }
            return false;
        }

        // P: toggle DNA map browser.
        if (key == juce::KeyPress('p') || key == juce::KeyPress('P'))
        {
            if (stateMachine_.currentState() == ViewState::BrowserOpen)
                exitBrowser();
            else
                transitionToBrowser();
            return true;
        }

        // K: toggle PlaySurface — no-op (PlaySurfaceOverlay removed, cut 1B-#13).
        if (key == juce::KeyPress('k') || key == juce::KeyPress('K'))
            return false;

        // 1–4: zoom-in to engine slot 0–3.
        auto kc = key.getKeyCode();
        if (kc >= '1' && kc <= '4')
        {
            const int slot = kc - '1';
            if (orbits_[slot].hasEngine())
                transitionToZoomIn(slot);
            return true;
        }

        // 5: zoom-in to ghost slot (index 4).
        if (key.getKeyCode() == juce::KeyPress('5').getKeyCode() &&
            orbits_[4].hasEngine())
        {
            transitionToZoomIn(4);
            return true;
        }

        // ── Orbit cycling — Tab / arrow keys (#974) ──────────────────────────

        // Helper: find the next populated slot after 'start' in direction +1 or -1,
        // wrapping around.  Returns -1 if no slots are populated.
        auto nextPopulatedSlot = [this](int start, int direction) -> int
        {
            for (int step = 1; step <= 5; ++step)
            {
                int candidate = (start + direction * step + 5) % 5;
                if (orbits_[candidate].hasEngine())
                    return candidate;
            }
            return -1;
        };

        // Tab / Right arrow: advance to next populated orbit slot.
        const bool isTab      = (key.getKeyCode() == juce::KeyPress::tabKey &&
                                  !key.getModifiers().isShiftDown());
        const bool isRight    = (key.getKeyCode() == juce::KeyPress::rightKey &&
                                  stateMachine_.currentState() == ViewState::Orbital);
        if (isTab || isRight)
        {
            const int from = (stateMachine_.selectedSlot() >= 0) ? stateMachine_.selectedSlot() : -1;
            const int next = nextPopulatedSlot(from, +1);
            if (next >= 0)
                transitionToZoomIn(next);
            return true;
        }

        // Shift+Tab / Left arrow: go back to previous populated orbit slot.
        const bool isShiftTab = (key.getKeyCode() == juce::KeyPress::tabKey &&
                                  key.getModifiers().isShiftDown());
        const bool isLeft     = (key.getKeyCode() == juce::KeyPress::leftKey &&
                                  stateMachine_.currentState() == ViewState::Orbital);
        if (isShiftTab || isLeft)
        {
            const int from = (stateMachine_.selectedSlot() >= 0) ? stateMachine_.selectedSlot() : 0;
            const int prev = nextPopulatedSlot(from, -1);
            if (prev >= 0)
                transitionToZoomIn(prev);
            return true;
        }

        // Up arrow: in ZoomIn state, step to the previous preset.
        if (key.getKeyCode() == juce::KeyPress::upKey &&
            stateMachine_.currentState() == ViewState::ZoomIn)
        {
            presetPrev_.triggerClick();
            return true;
        }

        // Down arrow: in ZoomIn state, step to the next preset.
        if (key.getKeyCode() == juce::KeyPress::downKey &&
            stateMachine_.currentState() == ViewState::ZoomIn)
        {
            presetNext_.triggerClick();
            return true;
        }

        return false;
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            // ── Right-click hit-detection order: buoy → empty ocean ─────────
            // (Knot right-clicks are handled by CouplingSubstrate itself via
            //  its onKnotRightClicked callback, so we only deal with buoys and
            //  the empty-ocean fallback here.)

            // 1. Check each visible orbit's bounds — populated slots first.
            for (int i = 0; i < 5; ++i)
            {
                if (!orbits_[i].isVisible() || !orbits_[i].hasEngine())
                    continue;

                // Convert OceanView-local click position to orbit-local.
                const auto orbitLocal = e.getEventRelativeTo(&orbits_[i]).position;
                if (orbits_[i].getLocalBounds().toFloat().contains(orbitLocal))
                {
                    showBuoyContextMenu(i);
                    return;
                }
            }

            // 2. Check empty slot circles — pass the slot index so "Add Engine..."
            //    can pre-select the correct target slot.
            for (int i = 0; i < 5; ++i)
            {
                if (!orbits_[i].isVisible() || orbits_[i].hasEngine())
                    continue;

                const auto orbitLocal = e.getEventRelativeTo(&orbits_[i]).position;
                if (orbits_[i].getLocalBounds().toFloat().contains(orbitLocal))
                {
                    showEmptyOceanContextMenu(i);
                    return;
                }
            }

            // 3. Empty ocean — no orbit hit at all.
            showEmptyOceanContextMenu();
            return;
        }

        // Left-click: Clicking the backdrop in ZoomIn mode (outside any creature)
        // returns to Orbital.  We check whether the click landed on a child
        // component via hitTest propagation — if we receive it here, no child
        // caught it.
        if (stateMachine_.currentState() == ViewState::ZoomIn)
        {
            transitionToOrbital();
            juce::ignoreUnused(e);
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        // Update chain line endpoint during chain-in-progress
        if (hudBar_.isChainModeActive() && chainStartSlot_ >= 0)
        {
            auto localPos = e.getPosition().toFloat();
            // Translate to substrate's coordinate space
            auto subPos = localPos - substrate_.getPosition().toFloat();
            substrate_.setChainMousePos(subPos);
        }
    }

    void mouseExit(const juce::MouseEvent& /*e*/) override
    {
        // F2-013: Cancel any in-progress coupling chain when the cursor leaves OceanView.
        // Without this, the ghost chain line stays rendered indefinitely.
        if (chainStartSlot_ >= 0)
        {
            chainStartSlot_ = -1;
            substrate_.setChainInProgress(false, -1, {});
            setMouseCursor(juce::MouseCursor::NormalCursor);
            repaint();
        }
    }

    //==========================================================================
    // Engine slot management
    //==========================================================================

    /**
        Assign an engine to a slot.

        @param slot      0–4.
        @param engineId  Canonical engine ID (e.g. "Oxytocin").
        @param accent    Engine accent colour.
        @param zone      Depth zone determining the orbital radius.
    */
    void setEngine(int slot,
                   const juce::String& engineId,
                   juce::Colour        accent,
                   EngineOrbit::DepthZone zone)
    {
        if (slot < 0 || slot >= 5)
            return;

        // Step 7: Dismiss first-launch lifesaver when the first engine is loaded.
        firstLaunch_ = false;
        lifesaver_.setVisible(false);

        orbits_[slot].setEngine(engineId, accent, zone);
        resized();  // recompute polar positions now numLoaded has changed
    }

    /** Remove the engine from a slot. */
    void clearEngine(int slot)
    {
        if (slot < 0 || slot >= 5)
            return;

        orbits_[slot].clearEngine();

        // Fix #1005 (callAsync race): only reset view state if we are currently
        // zoomed into THIS slot.  If the user has already double-clicked into a
        // different slot (selectedSlot_ != slot) or returned to Orbital between
        // when the engine removal was queued and when this runs, do not clobber
        // the new state.  Only ZoomIn / SplitTransform warrant a reset.
        const bool slotIsSelected = (stateMachine_.selectedSlot() == slot);
        const bool inEngagedState = (stateMachine_.currentState() == ViewState::ZoomIn ||
                                     stateMachine_.currentState() == ViewState::SplitTransform);
        if (slotIsSelected && inEngagedState)
            transitionToOrbital();
        else
            resized();
    }

    //==========================================================================
    // Preset data setters
    //==========================================================================

    void setPresetName(const juce::String& name)
    {
        // D6 (#1096): delegate to MasterFXStripCompact dot-matrix display.
        if (auto* fx = children_.masterFxStrip())
            fx->setPresetName(name);
        // #1007 FIX 3: Keep the inline header label in sync so the spatial grouping
        // "< Preset Name >" is always accurate.
        presetNameLabel_.setText(name, juce::dontSendNotification);
        // I1b: HUD bar was skipped in the delegation — arrows fired but display never updated.
        hudBar_.setPresetName(name);
    }
    // D6 (#1096): mood identity lives in browser/HUD — no-op here.
    void setMoodName(const juce::String&) {}
    void setMoodColour(juce::Colour)      {}

    // D6 (#1096): DNA hexagon lives in preset browser overlay — no-op here.
    // TODO(#1096-followup): route DNA to DnaMapBrowser or a dedicated overlay.
    void setDNA(float /*b*/, float /*w*/, float /*m*/, float /*d*/, float /*s*/, float /*a*/) {}

    void setPresetDots(std::vector<PresetDot> dots)
    {
        browser_.setPresets(std::move(dots));
    }

    void setActivePresetIndex(int index)
    {
        browser_.setActivePresetIndex(index);
    }

    //==========================================================================
    // Coupling data
    //==========================================================================

    /**
        Replace the full coupling route list.  Called from the editor's 10 Hz
        timer so the substrate threads reflect the live coupling matrix.
    */
    void setCouplingRoutes(const std::vector<CouplingRoute>& routes)
    {
        substrate_.setRoutes(routes);
        background_.setHasCouplingRoutes(!routes.empty());
    }

    //==========================================================================
    // Live state updates
    //==========================================================================

    void setVoiceCount(int slot, int count)
    {
        if (slot >= 0 && slot < 5)
        {
            orbits_[slot].setVoiceCount(count);
            // Forward to CouplingSubstrate so Coupling Evolution can detect
            // when both endpoints of a route are actively playing.
            substrate_.setSlotVoiceCount(slot, count);
        }
    }

    void pushSlotWaveData(int slot, const float* samples, int count, float rms)
    {
        if (slot >= 0 && slot < 5)
            orbits_[slot].setWreathData(samples, count, rms);
    }

    /**
        Wave 3 — 3a: Returns the current on-screen visual centre of the buoy for
        the given engine slot (0-4), accounting for spring offsets.

        C4 chain matrix should call this to get reliable anchor points for routing
        lines.  As long as positions are persisted (Wave 3 Gap 1 fix), these
        coordinates are stable and consistent between sessions.

        @param slot  Engine slot index 0-4.
        @returns     Visual centre in OceanView local coordinates.  Returns {0,0}
                     for out-of-range slot indices.
    */
    juce::Point<float> getOrbitCenter(int slot) const noexcept
    {
        if (slot >= 0 && slot < 5)
            return orbits_[slot].getVisualCenter();
        return {};
    }

    /** Set the preset name shown on the buoy's preset pill (#1356).
        Pass an empty string to show "—". Ignored for out-of-range slots. */
    void setOrbitPresetName(int slot, const juce::String& name)
    {
        if (slot >= 0 && slot < 5)
            orbits_[slot].setPresetName(name);
    }

    /** Step 8c: Trigger a ripple animation on the buoy wreath for the given slot.
        Called from the editor timer when the voice count increases (note-on). */
    void triggerBuoyRipple(int slot)
    {
        if (slot >= 0 && slot < 5)
            orbits_[slot].triggerRipple();
    }

    /**
        Set the visual reactivity of the ocean background (REACT dial, 0–1).
        Propagates directly to OceanBackground so the change is applied on the
        next repaint without going through the audio-update path.

        Called internally from the HUD REACT dial callback; exposed publicly so
        the editor can also restore a saved value on preset load.
    */
    void setReactivity(float value01)
    {
        background_.setReactivity(value01);  // triggers repaint internally
        hudBar_.setReactLevel(value01);
    }

    // wire(#orphan-sweep item 2): expose HUD fav-button bounds for walkthrough step 6.
    // Translates from hudBar_ local coords to OceanView local coords.
    juce::Rectangle<int> getHudFavBounds() const noexcept
    {
        auto localFav = hudBar_.getFavBounds();
        if (localFav.isEmpty()) return {};
        return localFav.translated(hudBar_.getX(), hudBar_.getY());
    }

    // F-003 / #1395: expose preset-name pill in HudBar for walkthrough step 3.
    // browser_.getBounds() is {} unless BrowserOpen state; use the preset-name
    // label instead — it is always visible and opens the browser on click.
    // Translates from hudBar_ local coords to OceanView local coords.
    juce::Rectangle<int> getDnaMapBrowserBounds() const noexcept
    {
        auto localName = hudBar_.getPresetNameBounds();
        if (localName.isEmpty()) return {};
        return localName.translated(hudBar_.getX(), hudBar_.getY());
    }

    // wire(#orphan-sweep item 2): expose orbit slot 1 bounds for walkthrough step 4 (couple).
    // Uses slot 1 (second engine buoy) as the visual target for the coupling step.
    juce::Rectangle<int> getOrbitBounds(int slot) const noexcept
    {
        if (slot >= 0 && slot < 5)
            return orbits_[slot].getBounds();
        return {};
    }

    /**
        Push master output waveform data to the ocean background wave surface.
        Call from the editor's 10 Hz timer with the processor's master WaveformFifo.
    */
    void pushMasterWaveData(const WaveformFifo& fifo)
    {
        // Read 120 samples from the 512-sample fifo (downsampled)
        constexpr int kDisplayPoints = 120;
        std::array<float, 512> raw {};
        fifo.readLatest(raw.data(), 512);

        // Downsample 512 → 120 points
        std::array<float, kDisplayPoints> display {};
        const float step = 512.0f / kDisplayPoints;
        for (int i = 0; i < kDisplayPoints; ++i)
            display[static_cast<size_t>(i)] = raw[static_cast<size_t>(static_cast<int>(i * step))];

        // Compute RMS from last 64 samples
        float rms = 0.0f;
        for (int i = 512 - 64; i < 512; ++i)
            rms += raw[static_cast<size_t>(i)] * raw[static_cast<size_t>(i)];
        rms = std::sqrt(rms / 64.0f);

        background_.setWaveData(display.data(), kDisplayPoints, rms);
        background_.repaint();
    }

    // D6 (#1096): NexusDisplay removed. Live readouts previously shown on the
    // central nexus — no replacement target yet.
    // TODO(#1096-followup): route voice-count readout to HUD/StatusBar overlay.
    void setLiveReadouts(int /*voiceCount*/, const std::array<float, 4>& /*macroValues*/) {}

    // D6 (#1096): DNA drift animation was driven by the hidden NexusDisplay.
    // No-op now that NexusDisplay is removed.
    void tickSustainedDna(int /*totalVoices*/, float /*dtSeconds*/) {}

    // Feature 7 (Schulze): Forward coupling timeline data to StatusBar.
    // Reads current route states from the substrate and pushes a snapshot
    // to the StatusBar's session timeline strip.
    void updateCouplingTimeline()
    {
        auto* sb = children_.statusBar();
        if (!sb)
            return;

        const auto snapshots = substrate_.getTimelineSnapshot();
        std::vector<StatusBar::TimelineEntry> entries;
        entries.reserve(snapshots.size());
        for (const auto& s : snapshots)
            entries.push_back({ s.colour, s.age });
        sb->setCouplingTimeline(entries);
    }

    void setCouplingLean(int slot, float lean)
    {
        if (slot >= 0 && slot < 5)
            orbits_[slot].setCouplingLean(lean);
    }

    /** Notify AmbientEdge of which depth zones are populated so the edge glow activates. */
    void setDepthZones(bool sunlit, bool twilight, bool midnight)
    {
        ambientEdge_.setActiveZones(sunlit, twilight, midnight);
        ambientEdge_.setEnginesActive(sunlit || twilight || midnight);
    }

    //==========================================================================
    // PlaySurface passthrough — legacy stubs (cut 1B-#13)
    //==========================================================================

    SubmarinePlaySurface& getSubmarinePlaySurface() { return subPlaySurface_; }
    void togglePlaySurface() {}
    void showPlaySurface()   {}
    void hidePlaySurface()   {}
    bool isPlaySurfaceVisible() const { return false; }
    void onMidiNoteReceived()         {}

    //==========================================================================
    // Child component accessors (for editor wiring)
    //==========================================================================

    MacroSection*      getMacroSection()  noexcept { return children_.macros(); }
    EngineDetailPanel* getDetailPanel()   noexcept { return children_.detailPanel(); }
    SidebarPanel*      getSidebar()       noexcept { return children_.sidebar(); }
    StatusBar*         getStatusBar()     noexcept { return children_.statusBar(); }

    juce::TextButton& presetPrevButton()   noexcept { return presetPrev_; }
    juce::TextButton& presetNextButton()   noexcept { return presetNext_; }
    juce::TextButton& favToggleButton()    noexcept { return favButton_; }
    juce::TextButton& settingsTogButton()  noexcept { return settingsButton_; }

    // Fix #1419: expose SettingsDrawer so the editor can call applySettings() on
    // startup (restore persisted values) and saveSettings() in the onSettingChanged callback.
    SettingsDrawer& settingsDrawer() noexcept { return settingsDrawer_; }

    //==========================================================================
    // Navigation callbacks — fired to XOceanusEditor
    //==========================================================================

    /** Fired when the HUD undo button is clicked. Parent should call UndoManager::undo(). */
    std::function<void()> onUndoRequested;

    /** Fired when the HUD redo button is clicked. Parent should call UndoManager::redo(). */
    std::function<void()> onRedoRequested;

    /** Fired when an engine slot is selected (zoom-in). -1 means deselected. */
    std::function<void(int slot)> onEngineSelected;

    /** F2-006: Fired when the OceanView transitions to a new ViewState.
        @param stateInt  0=Orbital, 1=ZoomIn, 2=SplitTransform, 3=BrowserOpen.
        @param slot      Active engine slot for ZoomIn/SplitTransform; -1 otherwise. */
    std::function<void(int stateInt, int slot)> onViewStateChanged;

    /** F2-012: Called from OceanView's 30Hz timerCallback to pull per-slot waveform
        data from the processor.  Wire in the editor to call pushSlotWaveData() for
        each active slot so wreath data is updated at the same rate as the animation. */
    std::function<void()> onPullWaveformData;

    /** Fired when an engine slot enters SplitTransform (double-click dive). */
    std::function<void(int slot)> onEngineDiveDeep;

    /** Fired when the user selects an engine from the Engine Library drawer (Phase 3).
        The editor should load this engine into the active slot (or next available). */
    std::function<void(const juce::String& engineId)> onEngineSelectedFromDrawer;

    /** Fired when the user clicks a preset dot in the DNA map browser. */
    std::function<void(int presetIndex)> onPresetSelected;

    /** Fired when the user completes a chain gesture (two orbit clicks in chain mode).
        The editor should create a coupling route between sourceSlot and destSlot. */
    std::function<void(int sourceSlot, int destSlot)> onCouplingRouteRequested;

    /** Fired when the user clicks the DNA hexagon in the nexus.
        Wire this to open the DnaMapBrowser or cycle the axis projection. */
    std::function<void()> onDnaClicked;

    /** Step 7: Fired when the user clicks the first-launch lifesaver ring.
        Wire this to open the engine picker so the user can load their first engine. */
    std::function<void()> onEnginePickerRequested;

    /** Fired when the user right-clicks a buoy and chooses "Mute" / "Unmute".
        OceanView tracks toggle state internally so the menu label reflects current
        state.  The callback receives the slot index; callers should query
        isSlotMuted(slot) for the new value after the callback fires. */
    std::function<void(int slot)> onEngineMuteToggled;

    /** Fired when the user right-clicks a buoy and chooses "Solo" / "Unsolo".
        OceanView tracks toggle state internally.  Callers should query
        isSlotSoloed(slot) for the new value after the callback fires. */
    std::function<void(int slot)> onEngineSoloToggled;

    /** Fired when the user right-clicks a buoy and chooses "Remove". */
    std::function<void(int slot)> onEngineRemoveRequested;

    /** Fired when the user right-clicks a coupling knot and chooses "Delete Chain". */
    std::function<void(int chainIndex)> onCouplingDeleteRequested;

    /** Fired when the user confirms changes in the coupling config popup (Done or Escape).
        Arguments: routeIndex, newType, newDepth [0–1], direction (0=fwd, 1=rev, 2=bidi). */
    std::function<void(int routeIndex, int newType, float newDepth, int direction)> onCouplingConfigChanged;

    /** Fired when the user chooses "Add Coupling..." from a buoy context menu. */
    std::function<void(int slot)> onChainModeRequested;

    // wire(#orphan-sweep item 5): SettingsDrawer fires onSettingChanged but the
    // callback was never assigned.  OceanView forwards it outward via this public
    // callback so the editor can route settings to the processor.
    // key: e.g. "polyphony", "masterTune", "mpeMode", "waveSensitivity", etc.
    // value: raw/normalised float (semantics depend on key — see SettingsDrawer.h).
    std::function<void(const juce::String& key, float value)> onSettingChanged;

    // wire(#orphan-sweep item 6): TransportBar fires onTimeSigChanged but the
    // callback was never assigned.  Forward outward so the editor/processor can act.
    std::function<void(int numerator, int denominator)> onTimeSigChanged;

    // wire(#orphan-sweep item 7): ChordBarComponent fires these callbacks but they
    // were never assigned.  Forward outward so state changes are observable.
    std::function<void()> onChordBarVisibilityChanged;
    std::function<void(ChordBarComponent::InputMode)> onChordBarInputModeChanged;

    // fix(#1354): HUD bar preset/save/export callbacks — forwarded outward so the
    // editor can route to PresetManager / ABCompare / ExportDialog.

    /** Fired when the user clicks SAVE in the HUD bar.
        Editor should prompt for a name (or overwrite current preset) and write the
        .xometa file via PresetManager::savePresetToFile(). */
    std::function<void()> onSavePreset;

    /** Fired when the user clicks ♥ in the HUD bar.
        @param newFavState   true = preset is now a favourite; false = removed.
        Editor should call PresetBrowser::toggleFavorite() on the current preset. */
    std::function<void(bool newFavState)> onFavToggled;

    /** Fired when the user clicks A/B in the HUD bar.
        @param active   true = A/B mode is now on; false = off.
        Editor should drive the ABCompare component (enter/exit compare mode). */
    std::function<void(bool active)> onABCompareToggled;

    /** Fired when the user clicks ◀ in the HUD bar.
        Editor should call PresetManager::previousPreset() then applyPreset(). */
    std::function<void()> onPresetPrev;

    /** Fired when the user clicks ▶ in the HUD bar.
        Editor should call PresetManager::nextPreset() then applyPreset(). */
    std::function<void()> onPresetNext;

    /** Fired when the user clicks EXPORT in the HUD bar.
        Editor should open the ExportDialog via juce::CallOutBox. */
    std::function<void()> onExportClicked;

    /** Fired when the user clicks the preset name label in the HUD bar.
        Editor should open the preset browser (e.g. sidebar Preset tab). */
    std::function<void()> onPresetNameClicked;

    /** Fired when the user clicks the preset pill on an engine buoy (#1356).
        @param slotIndex  The slot whose pill was clicked (0–3).
        Editor should open a per-slot CallOutBox(PresetBrowserPanel) filtered to that engine. */
    std::function<void(int slotIndex)> onPresetPillClicked;

    /** F3-006: Fired whenever the REACT dial value changes so the editor can
        persist it to the processor's persisted state.  value01 ∈ [0, 1]. */
    std::function<void(float value01)> onReactLevelChanged;

    /** F3-011/F3-017: Fired when the Seq or Chord breakout panel opens or closes.
        The editor wires these to the processor's persisted-state setters. */
    std::function<void(bool isOpen)> onSeqBreakoutToggled;
    std::function<void(bool isOpen)> onChordBreakoutToggled;

    /** P1-8 (1F): Fired when the dashboard tab changes (0=KEYS, 1=PAD, 2=XY).
        Editor wires this to processor.setPersistedDashboardTab() so the active
        tab survives DAW session save/reload. */
    std::function<void(int tabIndex)> onDashboardTabChanged;

    /** P1-8 (1F): Fired when the NOTE/KIT sub-mode pill is toggled.
        Editor wires this to processor.setPersistedKitSubMode(). */
    std::function<void(bool kitMode)> onDashboardKitSubModeChanged;

    /** P1-8 (1F): Restore the dashboard tab index and kit sub-mode from a
     *  saved session.  Called by the editor after initOceanView() completes.
     *  @param tabIndex  0=KEYS, 1=PAD, 2=XY (clamped to valid range).
     *  @param kitMode   true = KIT, false = NOTE. */
    void restoreDashboardTab(int tabIndex, bool kitMode)
    {
        tabBar_.setKitSubMode(kitMode);
        const int clamped = juce::jlimit(0, 2, tabIndex);
        if (clamped != 0)          // KEYS is already default — avoid spurious re-layout
            tabBar_.selectTab(clamped);
    }

    // wire(1C-1): ExpressionStrips — pitch bend (-1..+1) and mod wheel (0..+1)
    // forwarded outward so the editor can route to the MIDI collector.
    // Wired to exprStrips_.onPitchBend / onModWheel in initLayoutAndComponents().
    std::function<void(float pitchBend)> onExpressionPitchBend;  // -1..+1
    std::function<void(float modWheel)>  onExpressionModWheel;   //  0..+1

    //==========================================================================
    // State queries
    //==========================================================================

    // Phase 3 (#1184): viewState_ removed — read from OceanStateMachine.
    ViewState getViewState()    const noexcept { return stateMachine_.currentState(); }
    int       getSelectedSlot() const noexcept { return stateMachine_.selectedSlot(); }

    /** F3-011/F3-017: Restore Seq and Chord breakout panel open states from a saved session.
        Called by the editor in initOceanView() after all panels are laid out.
        Silently no-ops if either panel has not yet been initialised. */
    void restoreBreakoutState(bool seqOpen, bool chordOpen)
    {
        if (seqOpen)
            if (auto* s = children_.seqBreakout())
                s->setIsOpenFromState(true);
        if (chordOpen)
            if (auto* p = children_.chordBreakout())
                p->setIsOpenFromState(true);
    }

    /** F2-006/F2-015: Public entry point for externally triggering a ZoomIn transition.
        Used by the editor to restore persisted navigation state on session reload.
        Safe to call before the component is visible (deferred via callAfterDelay). */
    void requestZoomIn(int slot)
    {
        if (slot >= 0 && slot < 5)
            transitionToZoomIn(slot);
    }

    bool isSlotMuted  (int slot) const noexcept
    {
        if (slot < 0 || slot >= 5) return false;
        return slotMuted_[static_cast<size_t>(slot)];
    }

    bool isSlotSoloed (int slot) const noexcept
    {
        if (slot < 0 || slot >= 5) return false;
        return slotSoloed_[static_cast<size_t>(slot)];
    }

private:
    //==========================================================================
    // Inner helper components — Step 6 submarine dashboard
    //==========================================================================

    /** Thin teal gradient strip that visually separates the ocean viewport
        from the submarine dashboard below it. */
    // WaterlineSeparator replaced by TideWaterline (deferred-init unique_ptr).
    // See initWaterline() and Source/UI/Ocean/TideWaterline.h.

    /** Horizontal tab strip that selects the play-surface mode shown in the
        dashboard area below the macro strip. */
    /** Submarine-style tab bar — all custom paint, no TextButtons.
        Matches prototype: 10px uppercase, rounded-top tabs, teal active state.
        SEQ + CHORD toggles on the right side. */
    struct DashboardTabBar : public juce::Component,
                                 public juce::TooltipClient  // #21: tooltip support
    {
        DashboardTabBar()
        {
            setInterceptsMouseClicks(true, true);
            setOpaque(false);
        }

        // P1-11 (1F): Geometry extracted from paint() into rebuildHitRegions().
        // paint() now only reads cached rects; no bounds computation at 60 Hz.
        void resized() override { rebuildHitRegions(); }

        // #21 (2D): return per-region tooltip based on hoveredIdx_.
        juce::String getTooltip() override
        {
            if (hoveredToggleSEQ_)
                return "SEQ — toggle step sequencer strip";
            if (hoveredToggleCHORD_)
                return "CHORD — show/hide chord bar";
            if (hoveredTabIdx_ >= 0 && hoveredTabIdx_ < kNumTabs)
            {
                switch (hoveredTabIdx_)
                {
                    case 0: return "KEYS — chromatic keyboard (MPE)";
                    case 1: return "PAD — 4x4 chromatic note grid";
                    case 2: return "DRUM — 4x4 GM drum pads";
                    case 3: return "XY — 2D continuous expression surface";
                    default: break;
                }
            }
            return {};
        }

        void paint(juce::Graphics& g) override
        {
            const auto b = getLocalBounds().toFloat();
            // Bottom border
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.05f));
            g.fillRect(0.0f, b.getBottom() - 1.0f, b.getWidth(), 1.0f);

            static const juce::Font tabFont = XO::Tokens::Type::heading(XO::Tokens::Type::BodyDefault); // D3
            g.setFont(tabFont);

            // Draw tabs using pre-computed regions (rebuildHitRegions() wrote them).
            for (int i = 0; i < static_cast<int>(tabRegions_.size()); ++i)
            {
                // P1-11 (1F): read pre-computed cached bounds (no geometry in paint()).
                const auto& tr = tabRegions_[static_cast<size_t>(i)];
                const bool active  = (activeIdx_ == i);
                // #20 (2D): hover highlight using tabHoverAlpha_ at this region
                const bool hovered = (hoveredTabIdx_ == i && !active);

                if (active)
                {
                    g.setColour(XO::Tokens::Color::accent().withAlpha(0.07f));
                    g.fillRoundedRectangle(tr.getX(), tr.getY(), tr.getWidth(), tr.getHeight() + 2.0f, 6.0f);
                    g.setColour(XO::Tokens::Color::accent().withAlpha(0.90f));
                }
                else if (hovered)
                {
                    // #20: hover tint — 1px outline + slight fill
                    g.setColour(XO::Tokens::Color::accent().withAlpha(0.04f * tabHoverAlpha_));
                    g.fillRoundedRectangle(tr.getX(), tr.getY(), tr.getWidth(), tr.getHeight() + 2.0f, 6.0f);
                    g.setColour(XO::Tokens::Color::accent().withAlpha(0.30f * tabHoverAlpha_));
                    g.drawRoundedRectangle(tr.getX(), tr.getY(), tr.getWidth(), tr.getHeight() + 2.0f, 6.0f, 1.0f);
                    g.setColour(juce::Colour(200, 204, 216).withAlpha(0.55f));
                }
                else
                {
                    g.setColour(juce::Colour(200, 204, 216).withAlpha(0.35f));
                }
                g.drawText(kTabNames[i], tr.toNearestInt(), juce::Justification::centred, false);

                // D1 (1D-P2B): NOTE/KIT toggle pill — right-adjacent to the PAD tab.
                if (i == 1) // PAD tab
                {
                    const auto& sr = noteKitBounds_;
                    if (sr.getWidth() > 0.0f)
                    {
                        const char* subLabel = kitSubMode_ ? "KIT" : "NOTE";
                        if (active)
                        {
                            if (kitSubMode_)
                            {
                                g.setColour(juce::Colour(233, 196, 106).withAlpha(0.08f));
                                g.fillRoundedRectangle(sr, 4.0f);
                                g.setColour(juce::Colour(233, 196, 106).withAlpha(0.35f));
                                g.drawRoundedRectangle(sr, 4.0f, 1.0f);
                                g.setColour(juce::Colour(233, 196, 106).withAlpha(0.85f));
                            }
                            else
                            {
                                g.setColour(juce::Colour(60, 180, 170).withAlpha(0.06f));
                                g.fillRoundedRectangle(sr, 4.0f);
                                g.setColour(juce::Colour(60, 180, 170).withAlpha(0.25f));
                                g.drawRoundedRectangle(sr, 4.0f, 1.0f);
                                g.setColour(juce::Colour(60, 180, 170).withAlpha(0.70f));
                            }
                        }
                        else
                        {
                            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.04f));
                            g.drawRoundedRectangle(sr, 4.0f, 1.0f);
                            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.25f));
                        }
                        g.setFont(tabFont);
                        g.drawText(subLabel, sr.toNearestInt(), juce::Justification::centred, false);
                    }
                }
            }

            // SEQ + CHORD toggles — draw using pre-computed bounds.
            for (int t = 1; t >= 0; --t) // CHORD first (rightmost), then SEQ
            {
                const char* label = (t == 0) ? "SEQ" : "CHORD";
                const bool on = (t == 0) ? seqOn_ : chordOn_;
                // P1-11 (1F): read pre-computed cached bounds (no geometry in paint()).
                const auto& pr = (t == 0) ? seqBounds_ : chordBounds_;
                if (pr.getWidth() <= 0.0f) continue;
                // #20 (2D): hover state on toggle pills
                const bool hov = (t == 0) ? hoveredToggleSEQ_ : hoveredToggleCHORD_;

                if (on)
                {
                    g.setColour(juce::Colour(127, 219, 202).withAlpha(0.08f));
                    g.fillRoundedRectangle(pr, 4.0f);
                    g.setColour(juce::Colour(127, 219, 202).withAlpha(0.25f));
                    g.drawRoundedRectangle(pr, 4.0f, 1.0f);
                    g.setColour(juce::Colour(127, 219, 202).withAlpha(0.90f));
                }
                else if (hov)
                {
                    // #20: hover state on toggle pills
                    g.setColour(XO::Tokens::Color::accent().withAlpha(0.05f * tabHoverAlpha_));
                    g.fillRoundedRectangle(pr, 4.0f);
                    g.setColour(juce::Colour(200, 204, 216).withAlpha(0.20f * tabHoverAlpha_));
                    g.drawRoundedRectangle(pr, 4.0f, 1.0f);
                    g.setColour(juce::Colour(200, 204, 216).withAlpha(0.60f));
                }
                else
                {
                    g.setColour(juce::Colour(200, 204, 216).withAlpha(0.08f));
                    g.drawRoundedRectangle(pr, 4.0f, 1.0f);
                    g.setColour(juce::Colour(200, 204, 216).withAlpha(0.35f));
                }
                g.drawText(label, pr.toNearestInt(), juce::Justification::centred, false);
            }
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            const auto pos = e.position;
            // Check tabs
            for (int i = 0; i < static_cast<int>(tabRegions_.size()); ++i)
            {
                if (tabRegions_[static_cast<size_t>(i)].contains(pos))
                {
                    activeIdx_ = i;
                    if (onTabChanged) onTabChanged(kTabNames[i]);
                    repaint();
                    return;
                }
            }
            // SEQ toggle
            if (seqBounds_.contains(pos))
            {
                seqOn_ = !seqOn_;
                if (onSeqToggled) onSeqToggled(seqOn_);
                repaint();
                return;
            }
            // CHORD toggle
            if (chordBounds_.contains(pos))
            {
                chordOn_ = !chordOn_;
                if (onChordToggled) onChordToggled(chordOn_);
                repaint();
                return;
            }
            // D1 (1D-P2B): NOTE/KIT toggle — only active when PAD tab is selected.
            if (noteKitBounds_.getWidth() > 0.0f && noteKitBounds_.contains(pos))
            {
                kitSubMode_ = !kitSubMode_;
                if (onNoteKitToggled) onNoteKitToggled(kitSubMode_);
                repaint();
                return;
            }
        }

        // #20: hover tracking for tab bar items
        void mouseMove(const juce::MouseEvent& e) override
        {
            updateHoverState(e.position);
        }

        void mouseEnter(const juce::MouseEvent& e) override
        {
            updateHoverState(e.position);
        }

        void mouseExit(const juce::MouseEvent&) override
        {
            hoveredTabIdx_     = -1;
            hoveredToggleSEQ_  = false;
            hoveredToggleCHORD_= false;
            repaint();
        }

        // #20: driven by 30Hz tick from parent OceanView for smooth alpha fade
        void stepHoverAnimation()
        {
            const bool anyHovered = (hoveredTabIdx_ >= 0 || hoveredToggleSEQ_ || hoveredToggleCHORD_);
            const float target = anyHovered ? 1.0f : 0.0f;
            const float next = XO::Tokens::Motion::hoverFadeStep(tabHoverAlpha_, target);
            if (std::abs(next - tabHoverAlpha_) > 0.005f)
            {
                tabHoverAlpha_ = next;
                repaint();
            }
        }

        juce::String activeTab() const noexcept
        {
            return kTabNames[activeIdx_];
        }

        /** Programmatically select a tab by index (0=KEYS, 1=PAD, 2=XY).
         *  Fires onTabChanged callback and repaints — mirrors a user click.
         *  Used by OceanView::surfaceRight_.onCloseClicked to snap KEYS highlight
         *  back into sync after the panel is dismissed via the X button (D5). */
        void selectTab(int idx)
        {
            idx = juce::jlimit(0, kNumTabs - 1, idx);
            if (activeIdx_ == idx) return;
            activeIdx_ = idx;
            if (onTabChanged) onTabChanged(kTabNames[static_cast<size_t>(idx)]);
            repaint();
        }

        // D1 (1D-P2B): accessor for current NOTE/KIT sub-mode state.
        bool isKitSubMode() const noexcept { return kitSubMode_; }

        /** P1-8 (1F): Programmatically set NOTE/KIT sub-mode without firing the
         *  onNoteKitToggled callback — used by session-restore to pre-configure
         *  the pill before the first layout pass. */
        void setKitSubMode(bool kitMode) noexcept
        {
            kitSubMode_ = kitMode;
            rebuildHitRegions(); // pill label changes width when switching NOTE↔KIT
            repaint();
        }

        std::function<void(const juce::String&)> onTabChanged;
        std::function<void(bool)> onSeqToggled;
        std::function<void(bool)> onChordToggled;
        // D1 (1D-P2B): fires when NOTE/KIT pill is clicked (true = KIT, false = NOTE).
        std::function<void(bool kitMode)> onNoteKitToggled;

    private:
        // Three modes: KEYS, PAD, XY.  DRUM merged into PADS+KIT (#18).
        static constexpr int kNumTabs = 3;
        static constexpr std::array<const char*, kNumTabs> kTabNames = {"KEYS", "PAD", "XY"};
        static_assert(kTabNames.size() == kNumTabs, "kTabNames size mismatch");

        void updateHoverState(juce::Point<float> pos)
        {
            int  newTabHover      = -1;
            bool newSEQHover      = false;
            bool newCHORDHover    = false;

            for (int i = 0; i < static_cast<int>(tabRegions_.size()); ++i)
                if (tabRegions_[static_cast<size_t>(i)].contains(pos))
                    newTabHover = i;

            if (seqBounds_.contains(pos))   newSEQHover   = true;
            if (chordBounds_.contains(pos)) newCHORDHover = true;

            if (newTabHover != hoveredTabIdx_
                || newSEQHover   != hoveredToggleSEQ_
                || newCHORDHover != hoveredToggleCHORD_)
            {
                hoveredTabIdx_      = newTabHover;
                hoveredToggleSEQ_   = newSEQHover;
                hoveredToggleCHORD_ = newCHORDHover;
                repaint();
            }
        }

        int  activeIdx_ = 0;
        bool seqOn_     = false;
        bool chordOn_   = false;
        bool kitSubMode_ = false; // D1 (1D-P2B): false=NOTE, true=KIT

        // #20/#21: hover tracking
        int  hoveredTabIdx_      = -1;
        bool hoveredToggleSEQ_   = false;
        bool hoveredToggleCHORD_ = false;
        float tabHoverAlpha_     = 0.0f; // animated 0→1 on hover enter, 1→0 on exit

        std::vector<juce::Rectangle<float>> tabRegions_;
        juce::Rectangle<float> seqBounds_;
        juce::Rectangle<float> chordBounds_;
        juce::Rectangle<float> noteKitBounds_; // D1 (1D-P2B): NOTE/KIT toggle pill

        // P1-11 (1F): Compute hit-region geometry once per resize (not per paint).
        // Called from resized(), setKitSubMode(), and selectTab() (indirectly via
        // the repaint path — but only resized() is the geometry trigger).
        // Must stay in sync with the visual layout in paint().
        void rebuildHitRegions()
        {
            const auto b = getLocalBounds().toFloat();
            if (b.isEmpty()) return;

            static const juce::Font tabFont(juce::FontOptions{}
                .withName(juce::Font::getDefaultSansSerifFontName())
                .withStyle("Bold")
                .withHeight(10.0f));

            tabRegions_.clear();
            noteKitBounds_ = {};
            float x = 16.0f;

            for (int i = 0; i < kNumTabs; ++i)
            {
                const float tw = tabFont.getStringWidthFloat(kTabNames[i]) + 28.0f;
                const float th = b.getHeight() - 1.0f;
                tabRegions_.push_back(juce::Rectangle<float>(x, 0.0f, tw, th));
                x += tw + 2.0f;

                if (i == 1) // PAD tab — NOTE/KIT pill follows
                {
                    const char* subLabel = kitSubMode_ ? "KIT" : "NOTE";
                    const float pillW = tabFont.getStringWidthFloat(subLabel) + 14.0f;
                    // P1-5 (1F): WCAG 2.5.8 — NOTE/KIT pill ≥ 24px tall.
                    const float pillH = juce::jmax(24.0f, b.getHeight() - 6.0f);
                    noteKitBounds_ = juce::Rectangle<float>(x, (b.getHeight() - pillH) * 0.5f, pillW, pillH);
                    x += pillW + 4.0f;
                }
            }

            // SEQ + CHORD toggles — right side, compute right-to-left.
            float rx = b.getRight() - 16.0f;
            for (int t = 1; t >= 0; --t) // CHORD first (rightmost), then SEQ
            {
                const char* label = (t == 0) ? "SEQ" : "CHORD";
                const float pw = tabFont.getStringWidthFloat(label) + 16.0f;
                const float ph = b.getHeight() - 6.0f;
                juce::Rectangle<float> pr(rx - pw, 3.0f, pw, ph);
                if (t == 0) seqBounds_ = pr; else chordBounds_ = pr;
                rx -= pw + 6.0f;
            }
        }
    };

    //==========================================================================
    // Step 7: First-launch lifesaver overlay
    //==========================================================================

    struct LifesaverOverlay : public juce::Component, private juce::Timer
    {
        LifesaverOverlay()
        {
            setInterceptsMouseClicks(true, false);
            startTimerHz(30);
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            const float cx = bounds.getCentreX();
            const float cy = bounds.getCentreY() - 20.0f;
            const float lr = 40.0f;
            const float pulse = 1.0f + std::sin(phase_ * 2.0f) * 0.08f;
            const float bobY = std::sin(phase_ * 0.6f) * 6.0f;
            const float drawY = cy + bobY;

            // Outer glow
            juce::ColourGradient glow(
                juce::Colour(239, 68, 68).withAlpha(0.08f), cx, drawY,
                juce::Colours::transparentBlack, cx + lr * 2.5f, drawY, true);
            g.setGradientFill(glow);
            g.fillEllipse(cx - lr * 3, drawY - lr * 3, lr * 6, lr * 6);

            // Red ring
            g.setColour(juce::Colour(239, 68, 68).withAlpha(0.7f));
            juce::Path ringPath;
            ringPath.addEllipse(cx - lr * pulse, drawY - lr * pulse, lr * 2 * pulse, lr * 2 * pulse);
            g.strokePath(ringPath, juce::PathStrokeType(8.0f));

            // White stripes (lifesaver pattern — 4 segments)
            for (int i = 0; i < 4; ++i)
            {
                float a1 = juce::MathConstants<float>::halfPi * i
                          + juce::MathConstants<float>::pi / 4.0f + phase_ * 0.15f;
                float a2 = a1 + juce::MathConstants<float>::pi / 6.0f;
                juce::Path stripe;
                stripe.addCentredArc(cx, drawY, lr * pulse, lr * pulse, 0, a1, a2, true);
                g.setColour(juce::Colours::white.withAlpha(0.85f));
                g.strokePath(stripe, juce::PathStrokeType(8.0f));
            }

            // Inner circle
            g.setColour(juce::Colour(239, 68, 68).withAlpha(0.3f));
            g.drawEllipse(cx - (lr * pulse - 6), drawY - (lr * pulse - 6),
                          (lr * pulse - 6) * 2, (lr * pulse - 6) * 2, 1.0f);

            // "CLICK ME" text
            float textAlpha = 0.5f + std::sin(phase_ * 2.5f) * 0.2f;
            g.setColour(juce::Colour(200, 204, 216).withAlpha(textAlpha));
            static const juce::Font boldFont(juce::FontOptions(12.0f).withStyle("Bold"));
            g.setFont(boldFont);
            g.drawText("CLICK ME",
                       juce::Rectangle<float>(cx - 40, drawY - 8, 80, 16).toNearestInt(),
                       juce::Justification::centred, false);

            // Subtitle
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.25f));
            static const juce::Font captionFont(juce::FontOptions(9.0f));
            g.setFont(captionFont);
            g.drawText("Drop your first engine into the ocean",
                       juce::Rectangle<float>(cx - 140, drawY + lr + 16, 280, 14).toNearestInt(),
                       juce::Justification::centred, false);
        }

        void mouseDown(const juce::MouseEvent&) override
        {
            if (onClick) onClick();
        }

        void timerCallback() override
        {
            phase_ += 0.033f;
            repaint();
        }

        std::function<void()> onClick;
        float phase_ = 0.0f;
    };

    //==========================================================================
    // Context menus
    //==========================================================================

    /** Show a submarine-styled PopupMenu for a right-click on engine buoy at @p slot. */
    void showBuoyContextMenu(int slot)
    {
        const bool muted  = slotMuted_ [static_cast<size_t>(slot)];
        const bool soloed = slotSoloed_[static_cast<size_t>(slot)];

        juce::PopupMenu menu;
        menu.setLookAndFeel(&menuLnF_);

        menu.addItem(1, juce::String::fromUTF8("\xf0\x9f\x94\x8d  Open Detail"));           // 🔍
        menu.addItem(2, juce::String::fromUTF8("\xf0\x9f\x94\x84  Swap Engine..."));         // 🔄
        menu.addSeparator();
        menu.addItem(3, muted  ? juce::String::fromUTF8("\xf0\x9f\x94\x8a  Unmute")         // 🔊
                               : juce::String::fromUTF8("\xf0\x9f\x94\x87  Mute"));          // 🔇
        menu.addItem(4, soloed ? juce::String::fromUTF8("\xf0\x9f\x8e\xaf  Unsolo")         // 🎯
                               : juce::String::fromUTF8("\xf0\x9f\x8e\xaf  Solo"));          // 🎯
        menu.addItem(5, juce::String::fromUTF8("\xe2\xad\x90  Set as Master"));               // ⭐
        menu.addItem(6, juce::String::fromUTF8("\xf0\x9f\x94\x97  Add Coupling..."));        // 🔗
        menu.addSeparator();

        // Danger item: Remove
        juce::PopupMenu::Item removeItem;
        removeItem.itemID = 7;
        removeItem.text   = juce::String::fromUTF8("\xf0\x9f\x97\x91  Remove Engine");       // 🗑
        removeItem.colour = SubmarineMenuLookAndFeel::kDangerRed;
        menu.addItem(removeItem);

        SubmarineMenuLookAndFeel::showWithFade(menuLnF_, menu,
            // wire(1C-fix): withTargetComponent ensures menus anchor to the plugin
            // window on macOS AU (empty Options{} may appear at screen origin).
            juce::PopupMenu::Options{}.withTargetComponent(this),
            [this, slot](int result)
            {
                switch (result)
                {
                    case 1:
                        transitionToZoomIn(slot);
                        transitionToSplitTransform(slot);
                        break;
                    case 2:
                        // Force-select the target slot (no toggle) so the drawer
                        // replacement lands on the correct slot even if it was
                        // already selected before the right-click.
                        selectedSlot_ = slot;
                        for (int i = 0; i < 5; ++i)
                            orbits_[i].setSelected(i == slot);
                        if (onEngineSelected)
                            onEngineSelected(slot);
                        engineDrawer_.open();
                        break;
                    case 3:
                        slotMuted_[static_cast<size_t>(slot)] = !slotMuted_[static_cast<size_t>(slot)];
                        if (onEngineMuteToggled)
                            onEngineMuteToggled(slot);
                        break;
                    case 4:
                        slotSoloed_[static_cast<size_t>(slot)] = !slotSoloed_[static_cast<size_t>(slot)];
                        if (onEngineSoloToggled)
                            onEngineSoloToggled(slot);
                        break;
                    case 5:
                        // Set as Master — future: promote this slot's engine to primary.
                        break;
                    case 6:
                        // Add Coupling — enter chain-drawing mode anchored to this slot.
                        // Enable chain mode (same as pressing the HUD CHAIN button) and
                        // pre-arm this slot as the chain start so the next click completes
                        // the coupling route.
                        if (!hudBar_.isChainModeActive())
                            toggleChainMode();
                        chainStartSlot_ = slot;
                        substrate_.setChainInProgress(true, slot, orbits_[slot].getCenter());
                        if (onChainModeRequested)
                            onChainModeRequested(slot);
                        break;
                    case 7:
                        if (onEngineRemoveRequested)
                            onEngineRemoveRequested(slot);
                        break;
                    default:
                        break;
                }
            });
    }

    /** Populate the CouplingConfigPopup for the given routeIndex with real
        engine names, accent colours, and the live coupling type / depth. */
    void showCouplingPopupForRoute(int routeIndex)
    {
        const auto* route = substrate_.getRoute(routeIndex);
        if (route == nullptr)
            return;

        auto nameForSlot = [this](int slot) -> juce::String
        {
            if (slot < 0 || slot >= static_cast<int>(orbits_.size()))
                return "—";
            if (! orbits_[slot].hasEngine())
                return "Empty";
            juce::String id = orbits_[slot].getEngineId();
            if (id.length() > 24)
                id = id.substring(0, 23) + juce::String(juce::CharPointer_UTF8("\xe2\x80\xa6")); // …
            return id;
        };

        auto accentForSlot = [this](int slot) -> juce::Colour
        {
            if (slot < 0 || slot >= static_cast<int>(orbits_.size()))
                return juce::Colour(GalleryColors::xoGold);
            return orbits_[slot].getAccentColour();
        };

        couplingPopup_.show(routeIndex,
                            nameForSlot(route->sourceSlot), accentForSlot(route->sourceSlot),
                            nameForSlot(route->destSlot),   accentForSlot(route->destSlot),
                            route->type, route->amount);
    }

    /** Show a submarine-styled PopupMenu for a right-click on a coupling knot. */
    void showKnotContextMenu(int chainIndex)
    {
        juce::PopupMenu menu;
        menu.setLookAndFeel(&menuLnF_);

        menu.addItem(1, juce::String::fromUTF8("\xf0\x9f\x94\x84  Flip Direction"));        // 🔄
        menu.addItem(2, juce::String::fromUTF8("\xe2\x9a\x99  Configure..."));                // ⚙
        menu.addItem(3, juce::String::fromUTF8("\xf0\x9f\x93\x8b  Copy Settings"));          // 📋
        menu.addSeparator();

        juce::PopupMenu::Item removeItem;
        removeItem.itemID = 4;
        removeItem.text   = juce::String::fromUTF8("\xf0\x9f\x97\x91  Remove Coupling");     // 🗑
        removeItem.colour = SubmarineMenuLookAndFeel::kDangerRed;
        menu.addItem(removeItem);

        SubmarineMenuLookAndFeel::showWithFade(menuLnF_, menu,
            // wire(1C-fix): withTargetComponent ensures menus anchor to the plugin window.
            juce::PopupMenu::Options{}.withTargetComponent(this),
            [this, chainIndex](int result)
            {
                switch (result)
                {
                    case 1:
                        // Flip coupling direction — future: reverse source/target.
                        break;
                    case 2:
                        showCouplingPopupForRoute(chainIndex);
                        break;
                    case 3:
                        // Copy coupling settings to clipboard — future.
                        break;
                    case 4:
                        if (onCouplingDeleteRequested)
                            onCouplingDeleteRequested(chainIndex);
                        break;
                    default:
                        break;
                }
            });
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Chain-mode helpers
    // ─────────────────────────────────────────────────────────────────────────

    /** Apply cursor + substrate state that corresponds to the current
        hudBar_ chain-mode flag.  Call after flipping the flag. */
    void applyChainModeVisuals()
    {
        const bool chainOn = hudBar_.isChainModeActive();
        setMouseCursor(chainOn ? juce::MouseCursor::CrosshairCursor
                               : juce::MouseCursor::NormalCursor);
        chainStartSlot_ = -1;
        substrate_.setChainInProgress(false);
    }

    /** Toggle chain mode exactly as the HUD CHAIN button does. */
    void toggleChainMode()
    {
        hudBar_.setChainModeActive(!hudBar_.isChainModeActive());
        applyChainModeVisuals();
    }

    // ─────────────────────────────────────────────────────────────────────────

    /** Show a submarine-styled PopupMenu for a right-click on an empty slot.
        @param slot  0–4 if a specific empty orbit was hit; -1 for open ocean. */
    void showEmptyOceanContextMenu(int slot = -1)
    {
        juce::PopupMenu menu;
        menu.setLookAndFeel(&menuLnF_);

        menu.addItem(1, juce::String::fromUTF8("\xe2\x9e\x95  Add Engine..."));               // ➕
        menu.addItem(2, juce::String::fromUTF8("\xf0\x9f\x94\x97  Toggle Chain Mode"));      // 🔗

        SubmarineMenuLookAndFeel::showWithFade(menuLnF_, menu,
            // wire(1C-fix): withTargetComponent ensures menus anchor to the plugin window.
            juce::PopupMenu::Options{}.withTargetComponent(this),
            [this, slot](int result)
            {
                switch (result)
                {
                    case 1:
                        // If the user right-clicked a specific empty orbit, pre-select
                        // that slot so the drawer's engine-selected callback lands there.
                        if (slot >= 0 && slot < 5)
                        {
                            selectedSlot_ = slot;
                            for (int i = 0; i < 5; ++i)
                                orbits_[i].setSelected(i == slot);
                            if (onEngineSelected)
                                onEngineSelected(slot);
                        }
                        engineDrawer_.open();
                        break;
                    case 2:
                        // Mirror the HUD CHAIN button — toggle chain-drawing mode.
                        toggleChainMode();
                        break;
                    case 3:
                        // TODO: Paste engine from clipboard.
                        // Requires a JSON engine-clipboard mechanism (copyEngine /
                        // pasteEngine) — not yet implemented fleet-wide.
                        break;
                    default:
                        break;
                }
            });
    }

    //==========================================================================
    // #18 — Tab content cross-fade (200ms easeOutStep per D4)
    //==========================================================================

    /** Called from timerCallback to animate tabFadeIn_ / tabFadeOut_ each 30Hz tick.
        The fade targets are set when onTabChanged fires:
          - incoming panel starts at alpha=0, target=1  → tabFadeIn_
          - outgoing panel starts at alpha=1, target=0  → tabFadeOut_ (if still set)
        Uses Tokens::Motion::tabFadeStep() (200ms convergence at 30Hz). */
    void stepTabCrossFade()
    {
        bool needRepaint = false;

        // Fade in: bring the incoming panel to full opacity
        if (tabFadeInPanel_ != nullptr)
        {
            tabFadeIn_ = XO::Tokens::Motion::tabFadeStep(tabFadeIn_, 1.0f);
            tabFadeInPanel_->setAlpha(tabFadeIn_);
            if (tabFadeIn_ >= 0.99f)
            {
                tabFadeInPanel_->setAlpha(1.0f);
                tabFadeInPanel_ = nullptr;
            }
            needRepaint = true;
        }

        // Fade out: bring the outgoing panel to zero then hide it
        if (tabFadeOutPanel_ != nullptr)
        {
            tabFadeOut_ = XO::Tokens::Motion::tabFadeStep(tabFadeOut_, 0.0f);
            tabFadeOutPanel_->setAlpha(tabFadeOut_);
            if (tabFadeOut_ <= 0.01f)
            {
                tabFadeOutPanel_->setAlpha(0.0f);
                tabFadeOutPanel_->setVisible(false);
                tabFadeOutPanel_->setAlpha(1.0f); // reset for next time it becomes visible
                tabFadeOutPanel_ = nullptr;
            }
            needRepaint = true;
        }

        juce::ignoreUnused(needRepaint);
    }

    /** Initiate a cross-fade from @p outgoing to @p incoming.
        Pass nullptr for outgoing on the first show (no previous panel). */
    void startTabCrossFade(juce::Component* outgoing, juce::Component* incoming)
    {
        // Cancel any running fades first
        if (tabFadeInPanel_ != nullptr)
        {
            tabFadeInPanel_->setAlpha(1.0f);
            tabFadeInPanel_ = nullptr;
        }
        if (tabFadeOutPanel_ != nullptr)
        {
            tabFadeOutPanel_->setAlpha(0.0f);
            tabFadeOutPanel_->setVisible(false);
            tabFadeOutPanel_->setAlpha(1.0f);
            tabFadeOutPanel_ = nullptr;
        }

        if (outgoing != nullptr && outgoing->isVisible())
        {
            tabFadeOutPanel_ = outgoing;
            tabFadeOut_      = 1.0f; // start opaque, fade to 0
        }

        if (incoming != nullptr)
        {
            incoming->setAlpha(0.0f);
            incoming->setVisible(true);
            tabFadeInPanel_ = incoming;
            tabFadeIn_      = 0.0f; // start transparent, fade to 1
        }
    }

    //==========================================================================
    // Shared orbit animation timer (juce::Timer override)
    //==========================================================================

    void timerCallback() override
    {
        // F2-012: Pull per-slot waveform data at the same 30Hz rate as orbit animation,
        // so wreath visualisation is always in sync with the animation frame.
        // The callback is wired by the editor to read from processor WaveformFifos.
        if (onPullWaveformData)
            onPullWaveformData();

        // Drive all orbit animations from one synchronized 30 Hz timer.
        for (auto& orbit : orbits_)
            orbit.stepAnimation();

        // Update coupling curves with visual positions (spring offset included).
        // Skip orbits that are being dragged — their drag callback handles it
        // exclusively to avoid competing updates.
        for (int i = 0; i < 5; ++i)
            if (orbits_[i].hasEngine()
                && orbits_[i].getInputState() != EngineOrbit::InputState::UserDragging)
                substrate_.setCreatureCenter(i, orbits_[i].getVisualCenter());

        // Repaint only orbits that are actively animating.
        // Idle orbits freeze after their last frame — no 30Hz repaint means
        // no sub-pixel antialiasing shimmer from wreath path recalculation.
        for (auto& orbit : orbits_)
            if (orbit.hasEngine() && orbit.isAnimating())
                orbit.requestRepaint();

        // #19: step HUD bar click-depth spring-back animation (80ms per D4).
        hudBar_.stepClickDepthAnimation();

        // #20: step tab bar hover fade animation (150ms ease-out per D4).
        tabBar_.stepHoverAnimation();

        // #18: step tab content cross-fade alpha (200ms ease-out per D4).
        stepTabCrossFade();

        // Wave 3 — 3a: Position-save debounce countdown (500 ms / ~15 ticks at 30 Hz).
        // positionSaveCountdown_ is armed by schedulePositionSave() on each drag frame;
        // when it reaches zero we flush all 5 positions in one PropertiesFile write.
        if (positionSaveCountdown_ > 0)
        {
            positionSaveCountdown_ -= getTimerInterval(); // F2-020: use actual interval, not hardcoded 1000/30
            if (positionSaveCountdown_ <= 0)
            {
                positionSaveCountdown_ = 0;
                flushSlotPositions();
            }
        }
    }

    //==========================================================================
    // State machine transitions
    //==========================================================================

    void handleOrbitClicked(int slot)
    {
        if (hudBar_.isChainModeActive())
        {
            if (chainStartSlot_ < 0)
            {
                // First click: start the chain from this slot
                chainStartSlot_ = slot;
                substrate_.setChainInProgress(true, slot, orbits_[slot].getCenter());
            }
            else if (chainStartSlot_ != slot)
            {
                // Second click on a different slot: create coupling route
                if (onCouplingRouteRequested)
                    onCouplingRouteRequested(chainStartSlot_, slot);

                // Reset chain state
                chainStartSlot_ = -1;
                substrate_.setChainInProgress(false);
                hudBar_.setChainModeActive(false);
                setMouseCursor(juce::MouseCursor::NormalCursor);
            }
            // Clicking the same slot cancels the chain
            else
            {
                chainStartSlot_ = -1;
                substrate_.setChainInProgress(false);
            }
            return;
        }

        // Selection in place — no state transition, no orbit movement.
        // Visual emphasis (glow, ring) handled by EngineOrbit::setSelected().
        selectOrbitInPlace(slot);
    }

    void selectOrbitInPlace(int slot)
    {
        // Toggle: clicking the already-selected orbit deselects it.
        if (stateMachine_.selectedSlot() == slot)
        {
            selectedSlot_ = -1;                    // mirror (Phase 3; removed in step 9)
            stateMachine_.setSelectedSlot(-1);
            for (auto& o : orbits_)
                o.setSelected(false);
            if (onEngineSelected)
                onEngineSelected(-1);
            return;
        }

        selectedSlot_ = slot;                      // mirror (Phase 3; removed in step 9)
        stateMachine_.setSelectedSlot(slot);
        for (int i = 0; i < 5; ++i)
            orbits_[i].setSelected(i == slot);

        if (onEngineSelected)
            onEngineSelected(slot);
    }

    void dismissDetailPanel()
    {
        if (auto* dp = children_.detailPanel())
            dp->setVisible(false);
        dimOverlay_.setVisible(false);
        detailShowing_ = false;
        // Wave 3 3b / D7: release Detail panel from coordinator; restores
        // SurfaceRightPanel if it was hidden for the detail overlay.
        coordinatorRelease(PanelType::Detail);
    }

    void transitionToOrbital()
    {
        dismissDetailPanel();

        // Kill any in-flight spring animations before layout changes setBounds.
        for (auto& orbit : orbits_)
            orbit.resetSpring();

        // Phase 3 (#1184): state mutation + layout + repaint delegated to
        // OceanStateMachine.  onStateEntered fires layoutForState + repaint.
        stateMachine_.transitionToOrbital();

        if (onEngineSelected)
            onEngineSelected(-1);
    }

    void transitionToZoomIn(int slot)
    {
        // Toggling the same slot returns to Orbital.
        if (stateMachine_.currentState() == OceanStateMachine::ViewState::ZoomIn
            && stateMachine_.selectedSlot() == slot)
        {
            transitionToOrbital();
            return;
        }

        dismissDetailPanel();
        for (auto& orbit : orbits_)
            orbit.resetSpring();

        // Phase 3 (#1184): state mutation + layout + repaint delegated to
        // OceanStateMachine.  onStateEntered fires layoutForState + repaint.
        stateMachine_.transitionToZoomIn(slot);

        if (onEngineSelected)
            onEngineSelected(slot);
    }

    void transitionToSplitTransform(int slot)
    {
        dismissDetailPanel();
        for (auto& orbit : orbits_)
            orbit.resetSpring();

        // Phase 3 (#1184): state mutation + layout + repaint delegated to
        // OceanStateMachine.  onStateEntered fires layoutForState + repaint.
        stateMachine_.transitionToSplitTransform(slot);

        if (onEngineDiveDeep)
            onEngineDiveDeep(slot);
    }

    void transitionToBrowser()
    {
        dismissDetailPanel();
        for (auto& orbit : orbits_)
            orbit.resetSpring();

        // Phase 3 (#1184): pre-browser state snapshot + state mutation +
        // layout + repaint all delegated to OceanStateMachine.
        // transitionToBrowser() saves preBrowserState_/Slot_ before
        // transitioning to BrowserOpen; onStateEntered fires layout + repaint.
        stateMachine_.transitionToBrowser();
    }

    void exitBrowser()
    {
        // Read pre-browser state from OceanStateMachine (canonical owner).
        const auto preState = stateMachine_.preBrowserState();
        const int  preSlot  = stateMachine_.preBrowserSlot();

        // Restore whatever state was active before the browser was opened.
        if (preState == OceanStateMachine::ViewState::ZoomIn && preSlot >= 0)
        {
            // transitionToZoomIn re-enters ZoomIn and fires onEngineSelected.
            transitionToZoomIn(preSlot);
        }
        else if (preState == OceanStateMachine::ViewState::SplitTransform && preSlot >= 0)
        {
            transitionToSplitTransform(preSlot);
        }
        else
        {
            // Default: return to Orbital and clear selection.
            // Phase 3 (#1184): delegate to stateMachine_ instead of writing
            // viewState_ directly.
            stateMachine_.transitionToOrbital();

            if (onEngineSelected)
                onEngineSelected(-1);
        }

        // Reset saved pre-browser state so it cannot be accidentally re-used.
        stateMachine_.clearPreBrowserState();
    }

    //==========================================================================
    // Layout strategies — Phase 2 (#1184)
    //
    // layoutOrbital(), layoutZoomIn(), layoutSplitTransform(), layoutBrowser(),
    // layoutFloatingControls() have all been moved to OceanLayout.
    //
    // They are no longer declared here.  OceanView::resized() now calls:
    //   layout_->layoutForState(state, bounds)
    //
    // OceanLayout::layoutForState() dispatches to the per-state layout strategies
    // and also runs the state-independent dashboard layout.  The selectedSlot,
    // detailShowing, and firstLaunch state values are now const-ref bindings in
    // LayoutTargets (Phase 2.5, #1184) rather than per-call arguments.
    //==========================================================================

    //==========================================================================
    // Geometry helpers — Phase 2 (#1184): delegate to OceanLayout
    //==========================================================================

    /** Returns the ocean viewport bounds — the area above the waterline separator
        and submarine dashboard.  Previously this was everything minus the status
        bar; it now also excludes the waterline and dashboard rows.
        Phase 2: delegates to OceanLayout::computeOceanArea(). */
    juce::Rectangle<int> getOceanArea() const
    {
        jassert(layout_ != nullptr);
        return layout_->computeOceanArea(getLocalBounds());
    }

    /** Effective dashboard height — collapses when right panel is open
        (keyboard hidden, only macros + FX + tabs remain).
        Phase 2: delegates to OceanLayout::getEffectiveDashboardH(). */
    int getEffectiveDashboardH() const
    {
        jassert(layout_ != nullptr);
        return layout_->getEffectiveDashboardH();
    }

    /** Convert polar angle + radius to Cartesian in the given coordinate frame.
        Phase 2: delegates to OceanLayout::polarToCartesian (static). */
    static juce::Point<float> polarToCartesian(float angle,
                                               float radius,
                                               juce::Point<float> center) noexcept
    {
        return OceanLayout::polarToCartesian(angle, radius, center);
    }

    /** Map a DepthZone to its fractional radius (as a fraction of halfMin).
     *
     *  #1008 FIX 5: Sunlit radius raised from 0.30 → 0.38.
     *  At 0.30 the creature edge (radius + kOrbitalSize/2 ≈ 0.30*halfMin + 36px)
     *  overlapped the centre area at the default 1100×750 window size.
     *  0.38 gives ~10 px clearance around the centre.
     */
    static float radiusForZone(EngineOrbit::DepthZone zone) noexcept
    {
        switch (zone)
        {
            case EngineOrbit::DepthZone::Sunlit:   return 0.38f;  // #1008: was 0.30
            case EngineOrbit::DepthZone::Twilight: return 0.45f;
            case EngineOrbit::DepthZone::Midnight: return 0.60f;
        }
        return 0.38f;
    }

    //==========================================================================
    // Wave 3 — 3b: PanelCoordinator
    //
    // Rule: ONE "heavy" panel open at a time.  A heavy panel is anything that
    // changes ocean layout or is full-window (EnginePicker, Settings, Detail).
    //
    // Behaviour table:
    //   Opening EnginePicker  → closes Settings (and vice versa).
    //   Opening Detail        → hides SurfaceRightPanel (D7, restored on close).
    //   Opening ChainMatrix   → (Wave 5 C4) stub — currently a no-op.
    //   Minimum width guard   → if width < kMinWidth and drawer + SurfaceRightPanel
    //                           are both open, close the drawer.
    //
    // Usage from C4 chain matrix:
    //   coordinator_.requestOpen(PanelType::ChainMatrix);   // on open
    //   coordinator_.release(PanelType::ChainMatrix);        // on close
    //   oceanView.getOrbitCenter(slotIndex);                  // chain anchor points
    //==========================================================================

    /**
        Request that a panel become the active heavy panel.

        If a different heavy panel is already open, it is closed first.
        For ChainMatrix stub, records the current panel type
        and does nothing else — Wave 5 C4 will fill the open/close logic.
    */
    void coordinatorRequestOpen(PanelType requested)
    {
        if (currentPanel_ == requested)
            return; // already open — no-op

        // Close the current heavy panel before opening the new one.
        coordinatorCloseCurrentPanel();

        // Fix #1428: ChainMatrix is an unimplemented stub.
        // Do NOT record it as the active panel — that would leave currentPanel_
        // in a state where Escape closes a panel the user cannot see.
        // Return early before committing currentPanel_.
        if (requested == PanelType::ChainMatrix)
            return; // stub panel: no-op, no state update

        currentPanel_ = requested;

        switch (requested)
        {
            case PanelType::EnginePicker:
                engineDrawer_.open();
                // Close the settings drawer if somehow still open.
                if (settingsDrawer_.isOpen()) settingsDrawer_.close();
                break;

            case PanelType::Settings:
                settingsDrawer_.open();
                // Close the engine picker if somehow still open.
                if (engineDrawer_.isOpen()) engineDrawer_.close();
                break;

            case PanelType::Detail:
                // D7: hide SurfaceRightPanel while Detail is shown; remember state.
                surfaceRightWasOpenForDetail_ = surfaceRight_.isOpen() && surfaceRight_.isVisible();
                if (surfaceRightWasOpenForDetail_)
                {
                    surfaceRight_.setOpen(false);
                    surfaceRight_.setVisible(false);
                    resized();
                }
                // Actual Detail panel show is handled by the double-click callback;
                // this call only records the panel type for coordinator awareness.
                break;

            case PanelType::ChainMatrix:
                // Unreachable — guarded by early return above.
                break;

            case PanelType::None:
                break;
        }
    }

    /**
        Release a panel type from the coordinator.

        If @p released matches the current panel, closes it and resets to None.
        Must be called when the panel is dismissed by any means (close button, ESC, etc.).
    */
    void coordinatorRelease(PanelType released)
    {
        if (currentPanel_ != released)
            return; // not the active panel — nothing to do

        coordinatorCloseCurrentPanel();
        currentPanel_ = PanelType::None;
    }

    /** Close whatever panel is currently tracked as the active heavy panel. */
    void coordinatorCloseCurrentPanel()
    {
        switch (currentPanel_)
        {
            case PanelType::EnginePicker:
                if (engineDrawer_.isOpen())   engineDrawer_.close();
                break;

            case PanelType::Settings:
                if (settingsDrawer_.isOpen()) settingsDrawer_.close();
                break;

            case PanelType::Detail:
                // D7: restore SurfaceRightPanel if it was hidden for the detail panel.
                if (surfaceRightWasOpenForDetail_)
                {
                    surfaceRightWasOpenForDetail_ = false;
                    surfaceRight_.setOpen(true);
                    surfaceRight_.setVisible(true);
                    resized();
                }
                break;

            case PanelType::ChainMatrix:
                // Wave 5 C4 stub — no-op close.
                break;

            case PanelType::None:
                break;
        }
    }

    /** Minimum-width guard: if window < kMinWidth px wide and both a drawer and
     *  SurfaceRightPanel are open, close the drawer to prevent visual collision. */
    void coordinatorApplyWidthGuard()
    {
        static_assert(kMinWidth > 700, "kMinWidth must exceed legacy 700 threshold");
        const bool surfaceRightOpen = surfaceRight_.isOpen() && surfaceRight_.isVisible();
        const bool drawerOpen = engineDrawer_.isOpen() || settingsDrawer_.isOpen();
        if (getWidth() < kMinWidth && surfaceRightOpen && drawerOpen)
        {
            if (engineDrawer_.isOpen())
            {
                engineDrawer_.close();
                if (currentPanel_ == PanelType::EnginePicker)
                    currentPanel_ = PanelType::None;
            }
            if (settingsDrawer_.isOpen())
            {
                settingsDrawer_.close();
                if (currentPanel_ == PanelType::Settings)
                    currentPanel_ = PanelType::None;
            }
        }
    }

    //==========================================================================
    // Buoy position persistence (PropertiesFile, keyed by slot index)
    // Wave 3 — 3a: Global positions, NOT per-preset. Never APVTS. (D2, D3 locked)
    //
    // Key format:  buoy_slot_<N>_x  /  buoy_slot_<N>_y   (N = 0-4)
    // Written:     debounced 500 ms after last drag frame (avoid I/O on every frame)
    // Read:        once on OceanView construction via loadSlotPositions()
    // Defaults:    cross pattern — slot0 top-left, 1 top-right, 2 bottom-left,
    //              3 bottom-right, 4 ghost centre-bottom
    //==========================================================================

    /** Build the PropertiesFile options shared by save and load. */
    static juce::PropertiesFile::Options makePropertiesOptions() noexcept
    {
        juce::PropertiesFile::Options opts;
        opts.applicationName     = "XOceanus";
        opts.filenameSuffix      = "settings";
        opts.osxLibrarySubFolder = "Application Support";
        return opts;
    }

    /**
        Arm the 500 ms debounce timer so all dirty slot positions are written
        together on the next timer fire.  Call after any drag-position change.
        Avoids one PropertiesFile open/close per drag frame.
    */
    void schedulePositionSave()
    {
        positionSaveCountdown_ = kPositionSaveDelayMs;
    }

    /**
        Flush all 5 slot positions to the XOceanus settings file immediately.
        Called from timerCallback() once the debounce countdown reaches zero.
    */
    void flushSlotPositions()
    {
        juce::PropertiesFile settings(makePropertiesOptions());
        for (int i = 0; i < 5; ++i)
        {
            auto pos = orbits_[i].getNormalizedPosition();
            settings.setValue("buoy_slot_" + juce::String(i) + "_x", static_cast<double>(pos.x));
            settings.setValue("buoy_slot_" + juce::String(i) + "_y", static_cast<double>(pos.y));
        }
        settings.saveIfNeeded();
    }

    /**
        Legacy: kept so any remaining call sites that persist a single slot
        still compile.  Routes through the debounced batch writer — the actual
        I/O happens 500 ms later via flushSlotPositions().

        @deprecated  Call schedulePositionSave() directly; single-slot writes
                     are batched automatically.
    */
    void saveSlotPosition(int /*slot*/)
    {
        schedulePositionSave();
    }

    /** Load all 5 slot positions from settings; fall back to default cross pattern. */
    void loadSlotPositions()
    {
        // Default cross pattern (spec D8 / Wave 3 3a):
        //   slot 0 top-left, 1 top-right, 2 bottom-left, 3 bottom-right, 4 ghost centre-bottom.
        const float defaultPositions[5][2] = {
            { 0.25f, 0.30f }, // slot 0 — top-left
            { 0.75f, 0.30f }, // slot 1 — top-right
            { 0.25f, 0.70f }, // slot 2 — bottom-left
            { 0.75f, 0.70f }, // slot 3 — bottom-right
            { 0.50f, 0.80f }, // slot 4 — ghost centre-bottom
        };

        juce::PropertiesFile settings(makePropertiesOptions());

        for (int i = 0; i < 5; ++i)
        {
            // Try canonical Wave 3 key names first; fall back to pre-Wave3 legacy
            // names so existing installations keep their saved positions.
            const juce::String keyX = "buoy_slot_" + juce::String(i) + "_x";
            const juce::String keyY = "buoy_slot_" + juce::String(i) + "_y";
            const juce::String legX = "buoyPosX_" + juce::String(i);
            const juce::String legY = "buoyPosY_" + juce::String(i);

            const float defX = defaultPositions[i][0];
            const float defY = defaultPositions[i][1];

            const float x = static_cast<float>(
                settings.containsKey(keyX) ? settings.getDoubleValue(keyX, static_cast<double>(defX))
                                           : settings.getDoubleValue(legX, static_cast<double>(defX)));
            const float y = static_cast<float>(
                settings.containsKey(keyY) ? settings.getDoubleValue(keyY, static_cast<double>(defY))
                                           : settings.getDoubleValue(legY, static_cast<double>(defY)));

            orbits_[i].setNormalizedPosition({ x, y });
        }
    }

    /**
        Restore the canonical Z-order of all overlay and floating components.

        Called ONCE after each deferred-init method (initMacros, initDetailPanel,
        initSidebar, initStatusBar, initWaterline, initChordBar, initMasterFxStrip,
        initEpicSlotsPanel, initTransportBar) because addAndMakeVisible() pushes the
        newly constructed component to the front and disturbs the Z-stack.

        NOT called from resized() — z-order does not change when bounds change.
        (Doing so caused O(n²) toFront() calls per animation frame — refs #1163.)
        For the EngineDetailPanel, the onDoubleClicked handler uses the nuclear
        remove/addAndMakeVisible approach to force it to the absolute front.

        Order (bottom → top):
          ambientEdge_ | orbits_ | children_.macros() | children_.detailPanel() | children_.sidebar() | browser_ |
          detailOverlay_ | couplingPopup_ |
          presetPrev_ | presetNext_ | favButton_ | settingsButton_ | keysButton_ |
          dimOverlay_  ← #1008 FIX 7: above buttons, so buttons are dimmed |
          emptyStateLabel_ | lifesaver_ | hudBar_ | surfaceRight_ | exprStrips_ |
          subPlaySurface_ |
          children_.waterline() | children_.masterFxStrip() | children_.epicSlots() | tabBar_ | children_.chordBar() |
          children_.transportBar() | children_.statusBar() |
          engineDrawer_ | settingsDrawer_ | detailOverlay_ | children_.detailPanel() | couplingPopup_
    */
    void reorderZStack()
    {
        // Phase 2 (#1184): Z-stack logic delegated to OceanLayout.
        jassert(layout_ != nullptr);
        layout_->reorderZStack();
    }

    //==========================================================================
    // State
    //==========================================================================

    // Phase 3 (#1184): viewState_ removed — canonical state lives in stateMachine_.
    // selectedSlot_ is kept as a mirror for LayoutTargets (const int& binding);
    // it is updated in onStateEntered + selectOrbitInPlace.
    // Step 9 will remove selectedSlot_ once LayoutTargets binds to stateMachine_ directly.
    int       selectedSlot_    = -1;

    /// Per-slot mute / solo toggle state — tracked locally so the context menu
    /// label can reflect the current state without a round-trip to the processor.
    std::array<bool, 5> slotMuted_  {};
    std::array<bool, 5> slotSoloed_ {};

    /// Step 7: true until the user loads their first engine or clicks the lifesaver.
    bool firstLaunch_ = true;
    bool detailShowing_ = false;
    int  chainStartSlot_ = -1;  // -1 = no chain in progress

    // Phase 3 (#1184): preBrowserState_ and preBrowserSlot_ moved to
    // OceanStateMachine (owned by stateMachine_ member).  exitBrowser() reads
    // stateMachine_.preBrowserState() / preBrowserSlot() instead.

    // Wave 3 — 3a: Position-save debounce.
    // Counts down in ms from kPositionSaveDelayMs to 0 in timerCallback().
    // Armed by schedulePositionSave(); flushed to disk in flushSlotPositions().
    int positionSaveCountdown_ = 0;

    // Wave 3 — 3b: PanelCoordinator state.
    // Tracks which heavy panel is currently open. Initialises to None on load.
    PanelType currentPanel_           = PanelType::None;
    // D7: whether SurfaceRightPanel was open before DetailOverlay was shown.
    // Restored on DetailOverlay close.
    bool surfaceRightWasOpenForDetail_ = false;

    // #18: Tab cross-fade state (200ms per D4).
    // tabFadeInPanel_  — panel currently fading from alpha 0 → 1 (incoming tab content)
    // tabFadeOutPanel_ — panel currently fading from alpha 1 → 0 (outgoing tab content)
    juce::Component* tabFadeInPanel_  = nullptr;
    juce::Component* tabFadeOutPanel_ = nullptr;
    float            tabFadeIn_       = 1.0f;
    float            tabFadeOut_      = 0.0f;

    //==========================================================================
    // Phase 1+2 decomposition (#1184):
    //   OceanChildren owns all deferred-init unique_ptr children.
    //   OceanLayout owns all layout/geometry logic.
    //   (Phase 3 will add OceanStateMachine sm_.)
    //
    // children_ must be declared BEFORE all value-type component members so
    // it is ready to receive addAndMakeVisible calls from its init methods.
    // layout_ is a unique_ptr initialized at the end of the constructor body
    // (after all component members exist) using a LayoutTargets struct.
    //==========================================================================

    OceanChildren              children_{*this};
    std::unique_ptr<OceanLayout> layout_;

    // Phase 3 (#1184): OceanStateMachine owns ViewState + all transition logic.
    OceanStateMachine            stateMachine_;

    //==========================================================================
    // Child components — Z-ordered (bottom → top) as declared
    //
    // Deferred-init unique_ptr children have moved to OceanChildren above.
    //==========================================================================

    OceanBackground background_;
    CouplingSubstrate substrate_;
    std::array<EngineOrbit, 5> orbits_;      ///< 4 primary engine slots + 1 ghost slot
    AmbientEdge  ambientEdge_;

    // BLOCKER 1: empty-state label — shown when no engines are loaded.
    juce::Label          emptyStateLabel_;

    // Step 7: Pulsing lifesaver ring shown on first launch (no engines loaded).
    LifesaverOverlay     lifesaver_;

    // Overlay components.
    DnaMapBrowser        browser_;
    // #1008 FIX 7: DimOverlay must be declared before SubmarinePlaySurface
    // so it is constructed first and placed below in the Z-stack.
    DimOverlay           dimOverlay_;

    // Step 4: Floating detail overlay (wraps EngineDetailPanel with backdrop + close btn).
    DetailOverlay        detailOverlay_;

    // Submarine-styled popup menu LookAndFeel (dark glass, teal accents, fade-in).
    SubmarineMenuLookAndFeel menuLnF_;

    // Phase 2: Coupling knot configuration popup (shown on double-click of a knot).
    CouplingConfigPopup  couplingPopup_;

    // Step 6: Submarine dashboard.
    // unique_ptr members (waterline_, chordBar_, chordBreakout_, seqStrip_,
    // seqBreakout_, masterFxStrip_, epicSlots_, transportBar_) moved to
    // OceanChildren (children_) as part of Phase 1 decomposition (#1184).
    ExpressionStrips                      exprStrips_;
    DotMatrixDisplay                      dotMatrix_;
    SubmarineHudBar                       hudBar_;
    SubmarinePlaySurface                  subPlaySurface_;
    SurfaceRightPanel                     surfaceRight_;
    DashboardTabBar      tabBar_;
    // D5 (1D-P2B): canonical tab state — single source of truth for keyboard visibility.
    // Set in tabBar_.onTabChanged BEFORE layout runs. Read by OceanLayout::layoutDashboard()
    // via LayoutTargets::currentTab to fix the Detail-coordinator desync bug.
    OceanCurrentTab      currentTab_ = OceanCurrentTab::Keys; // default = KEYS

    // Floating header controls.
    juce::TextButton enginesButton_ { juce::String::charToString(0x2261) + " Engines" }; // ≡ Engines
    juce::TextButton presetPrev_    { "<" };
    juce::TextButton presetNext_    { ">" };
    juce::TextButton favButton_     { juce::String::charToString (0x2605) };  // ★
    juce::TextButton settingsButton_{ juce::String::charToString (0x2699) };  // ⚙
    juce::TextButton keysButton_    { "KEYS" };

    // Phase 3: Engine picker drawer (slide from left)
    EnginePickerDrawer engineDrawer_;

    // Settings drawer (slide from right)
    SettingsDrawer settingsDrawer_;

    // #1007 FIX 3: Inline preset name label between < and > for spatial grouping.
    juce::Label      presetNameLabel_;

    // #1007 FIX 4: Guard that all 4 deferred init methods have been called.
    // resized() and paint() check this flag before executing — prevents layout
    // and rendering crashes during the construction-to-visible window.
    bool fullyInitialised_ = false;

    //==========================================================================
    // Layout constants
    //==========================================================================

    static constexpr int   kMinWidth            = 960;
    static constexpr int   kMinHeight           = 600;
    static constexpr int   kDefaultWidth        = 1100;
    static constexpr int   kDefaultHeight       = 750;
    // 1D-P2: layout constants aliased from shared single-source-of-truth.
    // See Source/UI/Ocean/OceanLayoutConstants.h for canonical values + budget math.
    static constexpr int   kStatusBarH          = ocean_layout::kStatusBarH;
    static constexpr float kMacroStripH         = ocean_layout::kMacroStripH;
    static constexpr float kSplitOrbitalFraction = 0.20f;  ///< 20% width for mini orbital
    static constexpr int   kWaterlineH          = ocean_layout::kWaterlineH;
    static constexpr int   kDashboardH          = ocean_layout::kDashboardH;
    static constexpr int   kTabBarH             = ocean_layout::kTabBarH;

    // HIGH fix (#1006): padding added to orbital bounds so ±5% breath animation
    // paints inside the component rect.  ceil(72 * 0.05) = 4px each side.
    static constexpr int kBreathPadding = 30;

    // Orbit size alias: reference EngineOrbit constant directly.
    static constexpr float kOrbitSize_Orbital = EngineOrbit::kOrbitalSize;

    // Wave 3 — 3a: debounce delay for position saves (ms).
    // schedulePositionSave() arms this; timerCallback() decrements at 30 Hz.
    static constexpr int kPositionSaveDelayMs = 500;

    //==========================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OceanView)
};

} // namespace xoceanus
