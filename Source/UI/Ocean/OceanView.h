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
//     PlaySurfaceOverlay— slide-up keyboard/pads (on-demand)
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
#include "TideWaterline.h"
#include "ChordBarComponent.h"
#include "ChordBreakoutPanel.h"
#include "SeqBreakoutComponent.h"
#include "SeqStripComponent.h"
#include "MasterFXStripCompact.h"
#include "EpicSlotsPanel.h"
#include "TransportBar.h"
#include "SubmarineOuijaPanel.h"
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
    buttons but below PlaySurfaceOverlay.  When dimAlpha < 1.0 it fills its
    bounds with Ocean::abyss at (1 - dimAlpha) opacity, dimming everything
    underneath — including the header buttons that the old paint()-based rect
    could never reach because juce::Component::paint() draws behind children.

    setInterceptsMouseClicks(false, false) so the overlay is invisible to
    the event system and clicks pass through to PlaySurfaceOverlay above it.
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

    /** Interaction states that control the full layout strategy. */
    enum class ViewState
    {
        Orbital,          ///< Default: all creatures orbit the nexus
        ZoomIn,           ///< One creature enlarged at centre, others minimised
        SplitTransform,   ///< 20% mini-orbital strip left, 80% detail panel right
        BrowserOpen       ///< Full-window DNA map browser
    };

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

        XOuija note: XOuijaRouting (future standalone routing overlay) must call
        coordinator.requestOpen(PanelType::XOuijaRouting) on open.  It MAY coexist
        with SurfaceRightPanel but NOT with DetailOverlay or ChainMatrix.
    */
    enum class PanelType
    {
        None,             ///< No heavy panel open
        EnginePicker,     ///< EnginePickerDrawer — slides from left, dims ocean
        Settings,         ///< SettingsDrawer — slides from right, dims ocean
        Detail,           ///< EngineDetailPanel (EngineDetailPanel*) — full-window
        ChainMatrix,      ///< (Wave 5 C4) chain matrix slide-up — stub, no-op open/close
        XOuijaRouting     ///< (Future) XOuija routing overlay — stub, no-op open/close
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
        emptyStateLabel_.setText("Dive in — double-click a ghost slot to load an engine",
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

        // 9e. Submarine XOuija panel (hidden; HARMONIC tab removed per D4 #1174).
        ouijaPanel_.setVisible(false);
        addAndMakeVisible(ouijaPanel_);

        // 9f. Expression strips (PB + MW) — always visible in play area.
        addAndMakeVisible(exprStrips_);

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
            surfaceRight_.setOpen(false);
            surfaceRight_.setVisible(false);
            // Switch tab bar back to KEYS
            resized();
        };
        addAndMakeVisible(surfaceRight_);

        // 10. PlaySurface overlay (hidden by default; manages its own visibility)
        addAndMakeVisible(playSurfaceOverlay_);

        // 11. Floating header controls
        // Old Gallery floating header buttons — hidden, replaced by SubmarineHudBar.
        enginesButton_.setVisible(false);
        presetPrev_.setVisible(false);
        presetNext_.setVisible(false);
        favButton_.setVisible(false);
        settingsButton_.setVisible(false);
        keysButton_.setVisible(false);

        // 11b. #1008 FIX 7: DimOverlay sits above all buttons but below
        // PlaySurfaceOverlay.  Added after the buttons so it is painted on top.
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

        playSurfaceOverlay_.onDimStateChanged = [this](bool dim)
        {
            dimAlpha_ = dim ? 0.50f : 1.0f;
            // #1008 FIX 7: drive the overlay component so header buttons are
            // covered.  dimOverlay_ is a transparent child above buttons.
            dimOverlay_.setDimAlpha(dimAlpha_);

            // Feature 3 (Vangelis): Forward PlaySurface visibility to all
            // engine orbits so active-voice creatures glow brighter.
            for (auto& orbit : orbits_)
                orbit.setPlaySurfaceVisible(dim);
        };

        keysButton_.onClick = [this]() { togglePlaySurface(); };

        // ── Step 6: Dashboard tab bar callback ───────────────────────────────
        // Wave 6.5 (#1306) collision note:
        //   PAD/DRUM/XY tabs open SurfaceRightPanel.  All collision rules are already
        //   enforced by Wave 3 PanelCoordinator:
        //     (a) coordinatorApplyWidthGuard() — closes drawers when width < 700 px.
        //     (b) coordinatorRequestOpen(PanelType::Detail) — hides SurfaceRightPanel
        //         while DetailOverlay is open; restored on coordinatorRelease().
        //   SurfaceRightPanel is a soft panel and intentionally coexists with
        //   drawers above 700 px.  No additional coordinator call is required here.
        tabBar_.onTabChanged = [this](const juce::String& tab)
        {
            if (tab == "KEYS")
            {
                // KEYS: keyboard in dashboard, right panel closed.
                surfaceRight_.setOpen(false);
                surfaceRight_.setVisible(false);
                subPlaySurface_.setVisible(true);
                ouijaPanel_.setVisible(false);
            }
            else
            {
                // PAD/DRUM/XY/HARMONIC: right panel opens, keyboard HIDES.
                // HARMONIC re-enabled now that XOuija CC wiring is complete (#1304).
                subPlaySurface_.setVisible(false);
                ouijaPanel_.setVisible(false);

                if (tab == "PAD")           surfaceRight_.setMode(SurfaceRightPanel::Mode::Pad);
                else if (tab == "DRUM")     surfaceRight_.setMode(SurfaceRightPanel::Mode::Drum);
                else if (tab == "XY")       surfaceRight_.setMode(SurfaceRightPanel::Mode::XY);
                else if (tab == "HARMONIC") surfaceRight_.setMode(SurfaceRightPanel::Mode::Ouija);

                surfaceRight_.setOpen(true);
                surfaceRight_.setVisible(true);
            }

            resized();
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
        };
        background_.setReactivity(kDefaultReactLevel);

        // FIX 11: Chain mode toggles crosshair cursor over the ocean viewport
        // and clears any in-progress chain drawing on the substrate.
        hudBar_.onChainToggled = [this]() { applyChainModeVisuals(); };

        // ── Keyboard focus ────────────────────────────────────────────────────
        setWantsKeyboardFocus(true);

        // ── Shared orbit animation timer ──────────────────────────────────────
        // One 30 Hz timer drives all EngineOrbit animations in lock-step,
        // synchronizing breathe/bob/wreath phases and reducing OS timer allocations.
        startTimerHz(30);

        // ── Phase 3 (#1184): Wire OceanStateMachine callbacks ────────────────
        // onStateEntered fires after every state transition and triggers a
        // layout pass + repaint.  No OceanView* is stored in OceanStateMachine;
        // this lambda is the only coupling point.
        stateMachine_.onStateEntered = [this](OceanStateMachine::ViewState s)
        {
            // Sync local mirror members so OceanViewContext const-refs stay
            // valid for OceanLayout (Phase 3 step 10 TODO: remove mirrors).
            viewState_    = static_cast<ViewState>(static_cast<int>(s));
            selectedSlot_ = stateMachine_.selectedSlot();

            // Phase 3 step 11: OceanLayout now takes OceanStateMachine::ViewState
            // directly — no cast needed.
            layout_.layoutForState(s, getLocalBounds(), 1.0f);
            repaint();
        };
        // onAnimationFrame is stubbed for future animated transitions.
        // Currently unused — transitions are instantaneous.
        stateMachine_.onAnimationFrame = [](OceanStateMachine::ViewState /*s*/,
                                            float /*progress01*/)
        {
            // Future: layout_.layoutForState(s, getLocalBounds(), progress01);
            //         repaint();
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
    //   3. calls reorderZStack() to restore Z-order after addChild* disturbs it
    //   4. calls resized() where a layout pass is required
    //
    // Phase 2 will move reorderZStack() into OceanLayout; Phase 3 will move
    // state-machine callbacks into OceanStateMachine.
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
        reorderZStack();
    }

    /**
        Initialise the SeqStrip + SeqBreakout (Wave 5 C2 mount).
        Must be called after the processor is available — needs APVTS.
    */
    void initSeqStrip(juce::AudioProcessorValueTreeState& apvts)
    {
        children_.initSeqStrip(apvts);
        reorderZStack();
    }

    /** Initialise the compact Master FX strip (submarine-style). */
    void initMasterFxStrip(juce::AudioProcessorValueTreeState& apvts)
    {
        children_.initMasterFxStrip(apvts);
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
        reorderZStack();
    }

    /// Get the TransportBar so the editor can push BPM/voices/CPU.
    TransportBar*      getTransportBar() noexcept { return children_.transportBar(); }
    TideWaterline*     getWaterline()    noexcept { return children_.waterline(); }
    DotMatrixDisplay*  getDotMatrix()    noexcept { return &dotMatrix_; }
    /// Get the SurfaceRightPanel so the editor can wire onOuijaCCOutput.
    SurfaceRightPanel& getSurfaceRight() noexcept { return surfaceRight_; }

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
        if (!fullyInitialised_)
            return;

        // Wave 3 3b: Minimum-width guard — close drawers if window is too narrow.
        coordinatorApplyWidthGuard();

        // Phase 2 (#1184): delegate all layout logic to OceanLayout.
        // Phase 3: viewState_ is a mirror of stateMachine_.currentState()
        //          kept in sync via onStateEntered callback.
        layout_.layoutForState(
            stateMachine_.currentState(),
            getLocalBounds(),
            1.0f);
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
            if (viewState_ == ViewState::BrowserOpen)
            {
                exitBrowser();
                return true;
            }
            if (viewState_ != ViewState::Orbital)
            {
                transitionToOrbital();
                return true;
            }
            return false;
        }

        // P: toggle DNA map browser.
        if (key == juce::KeyPress('p') || key == juce::KeyPress('P'))
        {
            if (viewState_ == ViewState::BrowserOpen)
                exitBrowser();
            else
                transitionToBrowser();
            return true;
        }

        // K: toggle PlaySurface.
        if (key == juce::KeyPress('k') || key == juce::KeyPress('K'))
        {
            togglePlaySurface();
            return true;
        }

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
                                  viewState_ == ViewState::Orbital);
        if (isTab || isRight)
        {
            const int from = (selectedSlot_ >= 0) ? selectedSlot_ : -1;
            const int next = nextPopulatedSlot(from, +1);
            if (next >= 0)
                transitionToZoomIn(next);
            return true;
        }

        // Shift+Tab / Left arrow: go back to previous populated orbit slot.
        const bool isShiftTab = (key.getKeyCode() == juce::KeyPress::tabKey &&
                                  key.getModifiers().isShiftDown());
        const bool isLeft     = (key.getKeyCode() == juce::KeyPress::leftKey &&
                                  viewState_ == ViewState::Orbital);
        if (isShiftTab || isLeft)
        {
            const int from = (selectedSlot_ >= 0) ? selectedSlot_ : 0;
            const int prev = nextPopulatedSlot(from, -1);
            if (prev >= 0)
                transitionToZoomIn(prev);
            return true;
        }

        // Up arrow: in ZoomIn state, step to the previous preset.
        if (key.getKeyCode() == juce::KeyPress::upKey &&
            viewState_ == ViewState::ZoomIn)
        {
            presetPrev_.triggerClick();
            return true;
        }

        // Down arrow: in ZoomIn state, step to the next preset.
        if (key.getKeyCode() == juce::KeyPress::downKey &&
            viewState_ == ViewState::ZoomIn)
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
        if (viewState_ == ViewState::ZoomIn)
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
        const bool slotIsSelected = (selectedSlot_ == slot);
        const bool inEngagedState = (viewState_ == ViewState::ZoomIn ||
                                     viewState_ == ViewState::SplitTransform);
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
    // PlaySurface passthrough
    //==========================================================================

    PlaySurface& getPlaySurface()       { return playSurfaceOverlay_.getPlaySurface(); }
    SubmarinePlaySurface& getSubmarinePlaySurface() { return subPlaySurface_; }
    void showPlaySurface()
    {
        playSurfaceOverlay_.show();
        if (onPlaySurfaceVisibilityChanged)
            onPlaySurfaceVisibilityChanged(true);
    }
    void hidePlaySurface()
    {
        playSurfaceOverlay_.hide();
        if (onPlaySurfaceVisibilityChanged)
            onPlaySurfaceVisibilityChanged(false);
    }
    void togglePlaySurface()
    {
        if (playSurfaceOverlay_.isShowing())
            hidePlaySurface();
        else
            showPlaySurface();
    }
    bool isPlaySurfaceVisible() const   { return playSurfaceOverlay_.isShowing(); }
    void onMidiNoteReceived()           { playSurfaceOverlay_.onMidiNoteReceived(); }

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

    //==========================================================================
    // Navigation callbacks — fired to XOceanusEditor
    //==========================================================================

    /** Fired when the HUD undo button is clicked. Parent should call UndoManager::undo(). */
    std::function<void()> onUndoRequested;

    /** Fired when the HUD redo button is clicked. Parent should call UndoManager::redo(). */
    std::function<void()> onRedoRequested;

    /** Fired when an engine slot is selected (zoom-in). -1 means deselected. */
    std::function<void(int slot)> onEngineSelected;

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

    /** Fired when the PlaySurface overlay is shown or hidden (including first-launch auto-show).
        Use this to persist the visibility preference so subsequent plugin launches restore the
        last user-chosen state.  true = overlay is now showing, false = hidden. */
    std::function<void(bool visible)> onPlaySurfaceVisibilityChanged;

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

    //==========================================================================
    // State queries
    //==========================================================================

    ViewState getViewState()    const noexcept { return viewState_; }
    int       getSelectedSlot() const noexcept { return selectedSlot_; }

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
    struct DashboardTabBar : public juce::Component
    {
        DashboardTabBar()
        {
            setInterceptsMouseClicks(true, true);
            setOpaque(false);
        }

        void paint(juce::Graphics& g) override
        {
            const auto b = getLocalBounds().toFloat();
            // Bottom border
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.05f));
            g.fillRect(0.0f, b.getBottom() - 1.0f, b.getWidth(), 1.0f);

            static const juce::Font tabFont(juce::FontOptions{}
                .withName(juce::Font::getDefaultSansSerifFontName())
                .withStyle("Bold")
                .withHeight(10.0f));
            g.setFont(tabFont);

            // Rebuild tab regions
            tabRegions_.clear();
            float x = 16.0f;
            for (int i = 0; i < kNumTabs; ++i)
            {
                const float tw = tabFont.getStringWidthFloat(kTabNames[i]) + 28.0f;
                const float th = b.getHeight() - 1.0f;
                juce::Rectangle<float> tr(x, 0.0f, tw, th);
                tabRegions_.push_back(tr);

                const bool active = (activeIdx_ == i);
                if (active)
                {
                    g.setColour(juce::Colour(60, 180, 170).withAlpha(0.07f));
                    g.fillRoundedRectangle(tr.getX(), tr.getY(), tr.getWidth(), tr.getHeight() + 2.0f, 6.0f);
                    g.setColour(juce::Colour(60, 180, 170).withAlpha(0.90f));
                }
                else
                {
                    g.setColour(juce::Colour(200, 204, 216).withAlpha(0.35f));
                }
                g.drawText(kTabNames[i], tr.toNearestInt(), juce::Justification::centred, false);
                x += tw + 2.0f;
            }

            // SEQ + CHORD toggles from the right
            float rx = b.getRight() - 16.0f;
            for (int t = 1; t >= 0; --t) // CHORD first (rightmost), then SEQ
            {
                const char* label = (t == 0) ? "SEQ" : "CHORD";
                const bool on = (t == 0) ? seqOn_ : chordOn_;
                const float pw = tabFont.getStringWidthFloat(label) + 16.0f;
                const float ph = b.getHeight() - 6.0f;
                juce::Rectangle<float> pr(rx - pw, 3.0f, pw, ph);
                if (t == 0) seqBounds_ = pr; else chordBounds_ = pr;

                if (on)
                {
                    g.setColour(juce::Colour(127, 219, 202).withAlpha(0.08f));
                    g.fillRoundedRectangle(pr, 4.0f);
                    g.setColour(juce::Colour(127, 219, 202).withAlpha(0.25f));
                    g.drawRoundedRectangle(pr, 4.0f, 1.0f);
                    g.setColour(juce::Colour(127, 219, 202).withAlpha(0.90f));
                }
                else
                {
                    g.setColour(juce::Colour(200, 204, 216).withAlpha(0.08f));
                    g.drawRoundedRectangle(pr, 4.0f, 1.0f);
                    g.setColour(juce::Colour(200, 204, 216).withAlpha(0.35f));
                }
                g.drawText(label, pr.toNearestInt(), juce::Justification::centred, false);
                rx -= pw + 6.0f;
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
        }

        juce::String activeTab() const noexcept
        {
            return kTabNames[activeIdx_];
        }

        std::function<void(const juce::String&)> onTabChanged;
        std::function<void(bool)> onSeqToggled;
        std::function<void(bool)> onChordToggled;

    private:
        // Five modes: KEYS, PAD, DRUM, XY, HARMONIC.
        // HARMONIC (XOuija Ouija mode) re-enabled after CC wiring landed in #1304.
        // PAD+DRUM merge deferred (#1174 follow-up).
        static constexpr int kNumTabs = 5;
        static constexpr const char* kTabNames[kNumTabs] = {"KEYS", "PAD", "DRUM", "XY", "HARMONIC"};

        int  activeIdx_ = 0;
        bool seqOn_     = false;
        bool chordOn_   = false;

        std::vector<juce::Rectangle<float>> tabRegions_;
        juce::Rectangle<float> seqBounds_;
        juce::Rectangle<float> chordBounds_;
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
            juce::PopupMenu::Options{},
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
                        stateMachine_.setSelectedSlot(slot);  // keep state machine in sync
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
            juce::PopupMenu::Options{},
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

        juce::PopupMenu::Item pasteItem;
        pasteItem.itemID    = 3;
        pasteItem.text      = juce::String::fromUTF8("\xf0\x9f\x93\x8b  Paste Engine");      // 📋
        pasteItem.isEnabled = false;  // TODO: enable once a JSON engine-clipboard is implemented
        menu.addItem(pasteItem);

        SubmarineMenuLookAndFeel::showWithFade(menuLnF_, menu,
            juce::PopupMenu::Options{},
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
                            stateMachine_.setSelectedSlot(slot);  // keep state machine in sync
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
    // Shared orbit animation timer (juce::Timer override)
    //==========================================================================

    void timerCallback() override
    {
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

        // Wave 3 — 3a: Position-save debounce countdown (500 ms / ~15 ticks at 30 Hz).
        // positionSaveCountdown_ is armed by schedulePositionSave() on each drag frame;
        // when it reaches zero we flush all 5 positions in one PropertiesFile write.
        if (positionSaveCountdown_ > 0)
        {
            positionSaveCountdown_ -= 1000 / 30; // subtract one tick worth of ms
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
        if (selectedSlot_ == slot)
        {
            selectedSlot_ = -1;
            stateMachine_.setSelectedSlot(-1);  // keep state machine in sync
            for (auto& o : orbits_)
                o.setSelected(false);
            if (onEngineSelected)
                onEngineSelected(-1);
            return;
        }

        selectedSlot_ = slot;
        stateMachine_.setSelectedSlot(slot);  // keep state machine in sync
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

    // ── Phase 3 (#1184): transition wrappers ─────────────────────────────────
    // Each wrapper performs OceanView-specific pre/post work (spring reset,
    // dismissDetailPanel, external callbacks) and then delegates state ownership
    // to stateMachine_.  The stateMachine_ fires onStateEntered which triggers
    // layout_.layoutForState() + repaint() — replacing the old direct resized() call.

    void transitionToOrbital()
    {
        dismissDetailPanel();

        // Kill any in-flight spring animations before layout changes setBounds.
        for (auto& orbit : orbits_)
            orbit.resetSpring();

        // Delegate state update; onStateEntered → layout + repaint.
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

        // Delegate state update; onStateEntered → layout + repaint.
        stateMachine_.transitionToZoomIn(slot);

        if (onEngineSelected)
            onEngineSelected(slot);
    }

    void transitionToSplitTransform(int slot)
    {
        dismissDetailPanel();
        for (auto& orbit : orbits_)
            orbit.resetSpring();

        // Delegate state update; onStateEntered → layout + repaint.
        stateMachine_.transitionToSplitTransform(slot);

        if (onEngineDiveDeep)
            onEngineDiveDeep(slot);
    }

    void transitionToBrowser()
    {
        dismissDetailPanel();
        for (auto& orbit : orbits_)
            orbit.resetSpring();

        // Delegate state update (also snapshots pre-browser state internally).
        // onStateEntered → layout + repaint.
        stateMachine_.transitionToBrowser();
    }

    void exitBrowser()
    {
        // Ask stateMachine_ what the pre-browser state was and reset it.
        const auto savedState = stateMachine_.preBrowserState();
        const int  savedSlot  = stateMachine_.preBrowserSlot();

        // Clear pre-browser state before dispatching to prevent re-entry.
        stateMachine_.clearPreBrowserState();

        // Restore whatever state was active before the browser was opened.
        if (savedState == OceanStateMachine::ViewState::ZoomIn && savedSlot >= 0)
        {
            // transitionToZoomIn re-enters ZoomIn and fires onEngineSelected.
            transitionToZoomIn(savedSlot);
        }
        else if (savedState == OceanStateMachine::ViewState::SplitTransform && savedSlot >= 0)
        {
            transitionToSplitTransform(savedSlot);
        }
        else
        {
            // Default: return to Orbital and clear selection.
            stateMachine_.transitionToOrbital();

            if (onEngineSelected)
                onEngineSelected(-1);
        }
    }

    //==========================================================================
    // Layout strategies  [Phase 2 #1184: MIGRATED to OceanLayout]
    //
    // layoutOrbital(), layoutZoomIn(), layoutSplitTransform(),
    // layoutBrowser(), layoutFloatingControls() — all removed.
    // OceanView::resized() now delegates to layout_.layoutForState().
    // See Source/UI/Ocean/OceanLayout.h for the implementations.
    //==========================================================================

    //==========================================================================
    // Geometry helpers
    //==========================================================================

    /** Returns the ocean viewport bounds — the area above the waterline separator
        and submarine dashboard.  Previously this was everything minus the status
        bar; it now also excludes the waterline and dashboard rows. */
    /// Effective dashboard height — collapses when right panel is open
    /// (keyboard hidden, only macros + FX + tabs remain).
    int getEffectiveDashboardH() const
    {
        if (surfaceRight_.isOpen() && surfaceRight_.isVisible())
            return static_cast<int>(kMacroStripH) + 48 + kTabBarH; // macros + FX + tabs, no keyboard
        return kDashboardH;
    }

    juce::Rectangle<int> getOceanArea() const
    {
        const int wlH = children_.waterline() ? children_.waterline()->getDesiredHeight() : kWaterlineH;
        const int bottomH = getEffectiveDashboardH() + wlH + kStatusBarH;
        auto area = getLocalBounds().withTrimmedBottom(bottomH);
        // When right panel is open, ocean narrows from the right.
        if (surfaceRight_.isOpen() && surfaceRight_.isVisible())
        {
            const int rpW = std::min(SurfaceRightPanel::kPanelWidth,
                                     static_cast<int>(area.getWidth() * 0.40f));
            area = area.withTrimmedRight(rpW);
        }
        return area;
    }

    /** Convert polar angle + radius to Cartesian in the given coordinate frame. */
    juce::Point<float> polarToCartesian(float angle,
                                        float radius,
                                        juce::Point<float> center) const
    {
        return {
            center.x + radius * std::cos(angle),
            center.y + radius * std::sin(angle)
        };
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
    //   Opening XOuijaRouting → (future) stub — currently a no-op.
    //   Minimum width guard   → if width < 700 and drawer + SurfaceRightPanel
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
        For ChainMatrix and XOuijaRouting stubs, records the current panel type
        and does nothing else — Wave 5 C4 will fill the open/close logic.
    */
    void coordinatorRequestOpen(PanelType requested)
    {
        if (currentPanel_ == requested)
            return; // already open — no-op

        // Close the current heavy panel before opening the new one.
        coordinatorCloseCurrentPanel();

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
                // Wave 5 C4 stub — no-op open.  C4 author: fill this branch with
                // chain matrix show logic and call coordinator_.requestOpen(
                // PanelType::ChainMatrix) from the chain matrix open action.
                break;

            case PanelType::XOuijaRouting:
                // Future XOuija routing overlay stub — no-op open.
                // MAY coexist with SurfaceRightPanel but NOT with Detail or ChainMatrix.
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

            case PanelType::XOuijaRouting:
                // Future stub — no-op close.
                break;

            case PanelType::None:
                break;
        }
    }

    /** Minimum-width guard: if window < 700 px wide and both a drawer and
     *  SurfaceRightPanel are open, close the drawer to prevent visual collision. */
    void coordinatorApplyWidthGuard()
    {
        const bool surfaceRightOpen = surfaceRight_.isOpen() && surfaceRight_.isVisible();
        const bool drawerOpen = engineDrawer_.isOpen() || settingsDrawer_.isOpen();
        if (getWidth() < 700 && surfaceRightOpen && drawerOpen)
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
          subPlaySurface_ | playSurfaceOverlay_ | ouijaPanel_ |
          children_.waterline() | children_.masterFxStrip() | children_.epicSlots() | tabBar_ | children_.chordBar() |
          children_.transportBar() | children_.statusBar() |
          engineDrawer_ | settingsDrawer_ | detailOverlay_ | children_.detailPanel() | couplingPopup_
    */
    void reorderZStack()
    {
        // Phase 2 (#1184): delegated to OceanLayout.
        // See Source/UI/Ocean/OceanLayout.h for the full Z-order sequence.
        layout_.reorderZStack();
    }

    //==========================================================================
    // State
    //==========================================================================

    // Phase 3 (#1184): viewState_ and selectedSlot_ are mirrors of
    // stateMachine_.currentState() / selectedSlot(), kept in sync via the
    // onStateEntered callback.  They exist to:
    //   (a) satisfy OceanViewContext const-refs that OceanLayout reads, and
    //   (b) allow the many comparison sites in OceanView to use the local
    //       ViewState enum without qualifying OceanStateMachine::ViewState.
    // Phase 4 TODO: remove mirrors; route OceanLayout via parameter or accessor.
    ViewState viewState_       = ViewState::Orbital;
    int       selectedSlot_    = -1;
    float     dimAlpha_        = 1.0f;  ///< < 1 when PlaySurface or browser dims the scene

    /// Per-slot mute / solo toggle state — tracked locally so the context menu
    /// label can reflect the current state without a round-trip to the processor.
    std::array<bool, 5> slotMuted_  {};
    std::array<bool, 5> slotSoloed_ {};

    /// Step 7: true until the user loads their first engine or clicks the lifesaver.
    bool firstLaunch_ = true;
    bool detailShowing_ = false;
    int  chainStartSlot_ = -1;  // -1 = no chain in progress

    // Phase 3 (#1184): preBrowserState_ / preBrowserSlot_ moved to
    // OceanStateMachine.  OceanView::exitBrowser() reads them via
    // stateMachine_.preBrowserState() / preBrowserSlot().

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

    //==========================================================================
    // Phase 1 decomposition (#1184): OceanChildren owns all deferred-init
    // unique_ptr children.  Constructed before value-type members so it is
    // ready to receive addAndMakeVisible calls from its init methods.
    // Phase 2: OceanLayout layout_ added.  Phase 3: OceanStateMachine stateMachine_ added.
    //==========================================================================

    OceanChildren children_{*this};

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
    // #1008 FIX 7: DimOverlay must be declared BEFORE PlaySurfaceOverlay so
    // it is constructed first and can be placed below it in the Z-stack.
    DimOverlay           dimOverlay_;
    PlaySurfaceOverlay   playSurfaceOverlay_;

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
    SubmarineOuijaPanel                   ouijaPanel_;
    ExpressionStrips                      exprStrips_;
    DotMatrixDisplay                      dotMatrix_;
    SubmarineHudBar                       hudBar_;
    SubmarinePlaySurface                  subPlaySurface_;
    SurfaceRightPanel                     surfaceRight_;
    DashboardTabBar      tabBar_;

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
    static constexpr int   kStatusBarH          = 28;
    static constexpr float kMacroStripH         = 60.0f;  // #901: 56→60pt to fit 48pt knobs + 6pt pad
    static constexpr float kSplitOrbitalFraction = 0.20f;  ///< 20% width for mini orbital
    static constexpr int   kWaterlineH          = 6;
    static constexpr int   kDashboardH          = 340;    ///< macros (60) + FX (48) + tabs (30) + play (~202)
    static constexpr int   kTabBarH             = 30;

    // HIGH fix (#1006): padding added to orbital bounds so ±5% breath animation
    // paints inside the component rect.  ceil(72 * 0.05) = 4px each side.
    static constexpr int kBreathPadding = 30;

    // Orbit size alias: reference EngineOrbit constant directly.
    static constexpr float kOrbitSize_Orbital = EngineOrbit::kOrbitalSize;

    // Wave 3 — 3a: debounce delay for position saves (ms).
    // schedulePositionSave() arms this; timerCallback() decrements at 30 Hz.
    static constexpr int kPositionSaveDelayMs = 500;

    //==========================================================================
    // Phase 2 decomposition (#1184): OceanLayout owns all layout logic.
    // Phase 3 (#1184): OceanStateMachine owns ViewState + transition logic.
    // Both declared LAST so all referenced members are already constructed.
    //
    // buildLayoutContext() assembles the non-owning OceanViewContext that
    // OceanLayout needs.  It is called once during member initialisation.
    //==========================================================================

    OceanViewContext buildLayoutContext()
    {
        return OceanViewContext {
            /* selectedSlot       */ selectedSlot_,
            /* firstLaunch        */ firstLaunch_,
            /* detailShowing      */ detailShowing_,
            /* background         */ background_,
            /* substrate          */ substrate_,
            /* orbits             */ orbits_,
            /* ambientEdge        */ ambientEdge_,
            /* browser            */ browser_,
            /* emptyStateLabel    */ emptyStateLabel_,
            /* lifesaver          */ lifesaver_,
            /* detailOverlay      */ detailOverlay_,
            /* couplingPopup      */ couplingPopup_,
            /* dimOverlay         */ dimOverlay_,
            /* playSurfaceOverlay */ playSurfaceOverlay_,
            /* ouijaPanel         */ ouijaPanel_,
            /* exprStrips         */ exprStrips_,
            /* dotMatrix          */ dotMatrix_,
            /* hudBar             */ hudBar_,
            /* subPlaySurface     */ subPlaySurface_,
            /* surfaceRight       */ surfaceRight_,
            /* tabBar             */ tabBar_,
            /* enginesButton      */ enginesButton_,
            /* presetPrev         */ presetPrev_,
            /* presetNext         */ presetNext_,
            /* favButton          */ favButton_,
            /* settingsButton     */ settingsButton_,
            /* keysButton         */ keysButton_,
            /* presetNameLabel    */ presetNameLabel_,
            /* engineDrawer       */ engineDrawer_,
            /* settingsDrawer     */ settingsDrawer_
        };
    }

    // Phase 2: OceanLayout owns all ViewState-driven layout logic.
    // Constructed after all members it references (build order enforced by
    // placement here at the end of the member list).
    OceanLayout layout_{ children_, buildLayoutContext() };

    //==========================================================================
    // Phase 3 decomposition (#1184): OceanStateMachine owns ViewState enum and
    // all transition logic.  Callbacks wired in OceanView ctor body.
    // Declared AFTER layout_ so layout_ is ready when callbacks are wired.
    //==========================================================================

    OceanStateMachine stateMachine_;

    //==========================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OceanView)
};

} // namespace xoceanus
