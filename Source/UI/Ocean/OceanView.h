// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// OceanView.h — Main radial container component for the XOceanus Ocean View.
//
// OceanView is the top-level UI component that replaces the 3-column
// ColumnLayoutManager-based Gallery layout.  It owns and orchestrates all
// Ocean sub-components in a radial composition: engine creatures orbit a
// central NexusDisplay, coupling threads connect them via CouplingSubstrate,
// and ambient layers wrap the whole scene.
//
// Architecture overview
// ─────────────────────
//   Z-order (bottom → top):
//     OceanBackground   — depth-gradient field
//     CouplingSubstrate — luminescent Bézier threads
//     EngineOrbit[0-4]  — creature orbital sprites
//     NexusDisplay      — centre DNA + preset identity
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
//   Orbital       — all creatures orbit the nexus (default)
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
#include "NexusDisplay.h"
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
#include "MasterFXStripCompact.h"
#include "TransportBar.h"
#include "SubmarineOuijaPanel.h"
#include "ExpressionStrips.h"
#include "SubmarinePlaySurface.h"
#include "DotMatrixDisplay.h"
#include "SurfaceRightPanel.h"
#include "../Gallery/MacroSection.h"
#include "../Gallery/EngineDetailPanel.h"
#include "../Gallery/SidebarPanel.h"
#include "../Gallery/StatusBar.h"

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
class OceanView : public juce::Component
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

        // 4. Nexus: DNA hexagon + preset name + mood badge
        addAndMakeVisible(nexus_);

        // 5. Macro section (conditionally visible; placeholder until initMacros())
        // macros_ is a unique_ptr — added in initMacros()

        // 6. AmbientEdge: vignette + edge glow (top of background stack)
        addAndMakeVisible(ambientEdge_);

        // 7. Detail panel placeholder until initDetailPanel()
        // detail_ is a unique_ptr — added in initDetailPanel()

        // 8. Sidebar placeholder until initSidebar()
        // sidebar_ is a unique_ptr — added in initSidebar()

        // 9. DNA map browser (hidden by default)
        // Use addChildComponent so it starts hidden without a visible flash.
        addChildComponent(browser_);

        // 9c. Detail overlay (hidden by default; floats above orbits/substrate)
        addChildComponent(detailOverlay_);

        // 9e. Coupling config popup (hidden by default; shown on knot double-click)
        addChildComponent(couplingPopup_);

        // 9b. BLOCKER 1: Empty-state label — shown when no engines are loaded.
        // Appears centred below the nexus with a subtle call-to-action.
        emptyStateLabel_.setText("Load an engine to begin",
                                 juce::dontSendNotification);
        emptyStateLabel_.setFont(GalleryFonts::label(13.0f));
        emptyStateLabel_.setColour(juce::Label::textColourId,
                                   juce::Colour(GalleryColors::Ocean::foam).withAlpha(0.55f));
        emptyStateLabel_.setJustificationType(juce::Justification::centred);
        emptyStateLabel_.setVisible(false);  // visible only when numLoaded == 0
        addAndMakeVisible(emptyStateLabel_);

        // 9d. Step 6: Dashboard waterline + tab bar.
        // waterline_ is deferred (needs APVTS + sequencer) — see initWaterline().
        addAndMakeVisible(tabBar_);

        // 9e. Submarine XOuija panel (hidden by default, shown when OUIJA tab selected).
        ouijaPanel_.setVisible(false);
        addAndMakeVisible(ouijaPanel_);

        // 9f. Expression strips (PB + MW) — always visible in play area.
        addAndMakeVisible(exprStrips_);

        // 9g-vis. Dot-matrix visualizer — fills empty space right of macros.
        addAndMakeVisible(dotMatrix_);

        // 9g. Submarine play surface (replaces Gallery PlaySurface).
        addAndMakeVisible(subPlaySurface_);

        // 9h. Right-side panel for PAD/DRUM/XY/OUIJA modes.
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
        addAndMakeVisible(enginesButton_);
        addAndMakeVisible(presetPrev_);
        addAndMakeVisible(presetNext_);
        addAndMakeVisible(favButton_);
        addAndMakeVisible(settingsButton_);
        addAndMakeVisible(keysButton_);

        // 11b. #1008 FIX 7: DimOverlay sits above all buttons but below
        // PlaySurfaceOverlay.  Added after the buttons so it is painted on top.
        // reorderZStack() will enforce the correct final ordering on each
        // deferred init call.
        // Use addChildComponent so it starts hidden without a visible flash.
        addChildComponent(dimOverlay_);

        // 12. StatusBar placeholder until initStatusBar()
        // statusBar_ is a unique_ptr — added in initStatusBar()

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
        addAndMakeVisible(presetNameLabel_);
        A11y::setup(presetNameLabel_, "Current preset", "Name of the currently loaded preset");

        A11y::setup(keysButton_,      "Keys toggle", "Show or hide the Play Surface panel");
        A11y::setup(settingsButton_,  "Settings");
        A11y::setup(favButton_,       "Favourite");
        A11y::setup(presetPrev_,      "Previous preset");
        A11y::setup(presetNext_,      "Next preset");

        // ── Internal callbacks ────────────────────────────────────────────────
        for (int i = 0; i < 5; ++i)
        {
            orbits_[i].onClicked       = [this](int s) { transitionToZoomIn(s); };
            orbits_[i].onDoubleClicked = [this](int s)
            {
                if (detail_)
                    detailOverlay_.show(s, detail_.get());
            };
            orbits_[i].onPositionChanged = [this](int slot)
            {
                auto pos = orbits_[slot].getNormalizedPosition();
                auto area = getOceanArea();
                int sz = orbits_[slot].getBuoySize() + kBreathPadding * 2;
                int x = static_cast<int>(pos.x * area.getWidth()) - sz / 2;
                int y = static_cast<int>(pos.y * area.getHeight()) - sz / 2;
                orbits_[slot].setBounds(x + area.getX(), y + area.getY(), sz, sz);
                substrate_.setCreatureCenter(slot, orbits_[slot].getCenter());
                saveSlotPosition(slot);
            };
        }

        nexus_.onPresetNameClicked = [this]() { transitionToBrowser(); };
        nexus_.onDnaClicked        = [this]() { if (onDnaClicked) onDnaClicked(); };

        // ── CouplingSubstrate knot interaction ────────────────────────────────
        substrate_.onKnotDoubleClicked = [this](int routeIndex)
        {
            // Get route info to populate popup.
            // For now, show with placeholder names — will wire to real engine names.
            couplingPopup_.show(routeIndex, "Engine A", juce::Colour(60, 180, 170),
                               "Engine B", juce::Colour(140, 100, 220),
                               0, 0.5f);
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
                // PAD/DRUM/XY/OUIJA: right panel opens, keyboard HIDES.
                subPlaySurface_.setVisible(false);
                ouijaPanel_.setVisible(false);

                if (tab == "PAD")        surfaceRight_.setMode(SurfaceRightPanel::Mode::Pad);
                else if (tab == "DRUM")  surfaceRight_.setMode(SurfaceRightPanel::Mode::Drum);
                else if (tab == "XY")    surfaceRight_.setMode(SurfaceRightPanel::Mode::XY);
                else if (tab == "OUIJA") surfaceRight_.setMode(SurfaceRightPanel::Mode::Ouija);

                surfaceRight_.setOpen(true);
                surfaceRight_.setVisible(true);
            }

            resized();
        };

        // SEQ toggle → expand/collapse TideWaterline.
        tabBar_.onSeqToggled = [this](bool on)
        {
            if (waterline_)
                waterline_->setExpanded(on);
        };

        // CHORD toggle → show/hide ChordBarComponent.
        tabBar_.onChordToggled = [this](bool on)
        {
            if (chordBar_)
            {
                chordBar_->setVisible(on);
                resized(); // re-layout dashboard to accommodate chord bar
            }
        };

        // ── DetailOverlay callbacks ───────────────────────────────────────────
        detailOverlay_.onHidden = [this]()
        {
            if (detail_)
                detail_->setVisible(false);
        };

        // ── Buoy positions — load saved positions (or defaults) ──────────────
        loadSlotPositions();

        // ── Step 7: First-launch lifesaver ────────────────────────────────────
        addChildComponent(lifesaver_); // hidden by default, shown when no engines loaded
        lifesaver_.onClick = [this]()
        {
            firstLaunch_ = false;
            lifesaver_.setVisible(false);
            // Open engine picker drawer instead of external callback
            engineDrawer_.open();
        };

        // ── Phase 3: Engine picker drawer ────────────────────────────────────
        addChildComponent(engineDrawer_); // starts hidden; toggle via enginesButton_
        engineDrawer_.onEngineSelected = [this](const juce::String& engineId)
        {
            engineDrawer_.close();
            if (onEnginePickerRequested)
                onEnginePickerRequested();
            if (onEngineSelectedFromDrawer)
                onEngineSelectedFromDrawer(engineId);
        };
        enginesButton_.onClick = [this]() { engineDrawer_.toggle(); };

        // ── Settings drawer (slide from right) ────────────────────────────────
        addChildComponent(settingsDrawer_); // starts hidden; toggle via settingsButton_
        settingsButton_.onClick = [this]() { settingsDrawer_.toggle(); };

        // ── Keyboard focus ────────────────────────────────────────────────────
        setWantsKeyboardFocus(true);
    }

    ~OceanView() override = default;

    //==========================================================================
    // Deferred initialisation — called by XOceanusEditor before first show
    //==========================================================================

    /**
        Wire macro knobs to the AudioProcessorValueTreeState.
        Must be called before the component becomes visible.
    */
    void initMacros(juce::AudioProcessorValueTreeState& apvts)
    {
        macros_ = std::make_unique<MacroSection>(apvts);
        addAndMakeVisible(*macros_);

        // Re-stack above the orbits and nexus, but below ambientEdge.
        macros_->toFront(false);
        reorderZStack();
        resized();
    }

    /**
        Wire the EngineDetailPanel to the processor.
        Must be called before the component becomes visible.
    */
    void initDetailPanel(XOceanusProcessor& proc)
    {
        detail_ = std::make_unique<EngineDetailPanel>(proc);
        addAndMakeVisible(*detail_);
        detail_->setVisible(false);

        reorderZStack();
        resized();
    }

    /**
        Initialise the SidebarPanel.
        Must be called before the component becomes visible.
    */
    void initSidebar()
    {
        sidebar_ = std::make_unique<SidebarPanel>();
        addAndMakeVisible(*sidebar_);
        sidebar_->setVisible(false);

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
        waterline_ = std::make_unique<TideWaterline>(apvts, sequencer);
        waterline_->onHeightChanged = [this]()
        {
            // When waterline expands/collapses, re-layout the whole view.
            resized();
        };
        addAndMakeVisible(*waterline_);
        reorderZStack();
    }

    /**
        Initialise the ChordBarComponent (submarine-style chord strip).
        Needs APVTS + ChordMachine reference.
    */
    void initChordBar(juce::AudioProcessorValueTreeState& apvts,
                      const ChordMachine& chordMachine)
    {
        chordBar_ = std::make_unique<ChordBarComponent>(apvts, chordMachine);
        chordBar_->setVisible(false); // starts hidden, toggled by CHORD button
        addAndMakeVisible(*chordBar_);
        reorderZStack();
    }

    /**
        Initialise the compact Master FX strip (submarine-style).
    */
    void initMasterFxStrip(juce::AudioProcessorValueTreeState& apvts)
    {
        masterFxStrip_ = std::make_unique<MasterFXStripCompact>(apvts);
        addAndMakeVisible(*masterFxStrip_);
        reorderZStack();
    }

    /**
        Initialise the TransportBar (submarine-style bottom status strip).
    */
    void initTransportBar()
    {
        transportBar_ = std::make_unique<TransportBar>();
        addAndMakeVisible(*transportBar_);
        reorderZStack();
    }

    /// Get the TransportBar so the editor can push BPM/voices/CPU.
    TransportBar*      getTransportBar() noexcept { return transportBar_.get(); }
    TideWaterline*     getWaterline()    noexcept { return waterline_.get(); }
    DotMatrixDisplay*  getDotMatrix()    noexcept { return &dotMatrix_; }

    /**
        Initialise the StatusBar.
        Must be the last deferred-init call — it sets fullyInitialised_ = true
        and triggers the first valid resized() pass.
    */
    void initStatusBar()
    {
        statusBar_ = std::make_unique<StatusBar>();
        addAndMakeVisible(*statusBar_);
        reorderZStack();

        // #1007 FIX 4: All 5 deferred-init methods have now been called.
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

        switch (viewState_)
        {
            case ViewState::Orbital:        layoutOrbital();        break;
            case ViewState::ZoomIn:         layoutZoomIn();         break;
            case ViewState::SplitTransform: layoutSplitTransform(); break;
            case ViewState::BrowserOpen:    layoutBrowser();        break;
        }

        layoutFloatingControls();

        // ── Step 6: Submarine dashboard layout ──────────────────────────────
        // Slice the window into: ocean viewport | waterline | dashboard | status bar.
        const auto  fullBounds = getLocalBounds();
        const auto  oceanArea  = getOceanArea();  // already excludes waterline + dashboard + status + right panel

        // Right-side panel (PAD/DRUM/XY/OUIJA) — sits beside the ocean.
        if (surfaceRight_.isOpen() && surfaceRight_.isVisible())
        {
            const int rpW = std::min(SurfaceRightPanel::kPanelWidth,
                                     static_cast<int>(fullBounds.getWidth() * 0.40f));
            const int wlH2 = waterline_ ? waterline_->getDesiredHeight() : kWaterlineH;
            const int bottomH = getEffectiveDashboardH() + wlH2 + kStatusBarH;
            surfaceRight_.setBounds(oceanArea.getRight(),
                                    fullBounds.getY(),
                                    rpW,
                                    fullBounds.getHeight() - bottomH);
        }

        // Waterline separator strip — height is dynamic (6px collapsed, 96px expanded).
        const int wlH = waterline_ ? waterline_->getDesiredHeight() : kWaterlineH;
        if (waterline_)
            waterline_->setBounds(fullBounds.getX(),
                                  oceanArea.getBottom(),
                                  fullBounds.getWidth(),
                                  wlH);

        // Dashboard area: between the waterline and the status bar.
        auto dashArea = fullBounds
                            .withTrimmedTop(oceanArea.getHeight() + wlH)
                            .withTrimmedBottom(kStatusBarH);

        // Macro strip (top of dashboard) — macros left, dot-matrix right.
        {
            auto macroRow = dashArea.removeFromTop(static_cast<int>(kMacroStripH));
            if (macros_)
            {
                // Macros take ~360px on the left (5 knobs × ~70px each).
                const int macroW = std::min(360, macroRow.getWidth() / 2);
                macros_->setBounds(macroRow.removeFromLeft(macroW));
            }
            // Dot-matrix display fills the remaining space.
            dotMatrix_.setBounds(macroRow.reduced(4, 4));
        }

        // Master FX compact strip (48px, between macros and tab bar).
        if (masterFxStrip_)
            masterFxStrip_->setBounds(dashArea.removeFromTop(48));

        // Tab bar row.
        tabBar_.setBounds(dashArea.removeFromTop(kTabBarH));

        // Chord bar (visible when CHORD toggle is on, ~28px).
        if (chordBar_ && chordBar_->isVisible())
            chordBar_->setBounds(dashArea.removeFromTop(42));

        // Expression strips (36px) on the left of the play area.
        exprStrips_.setBounds(dashArea.removeFromLeft(ExpressionStrips::kStripWidth));

        // Remaining dashboard space → Submarine PlaySurface (KEYS keyboard).
        // The old PlaySurfaceOverlay is hidden. OUIJA is in the right panel now.
        playSurfaceOverlay_.setVisible(false);
        ouijaPanel_.setVisible(false); // ouija now lives inside SurfaceRightPanel
        subPlaySurface_.setBounds(dashArea);
        // Only show keyboard when right panel is closed (KEYS mode).
        // When right panel is open (PAD/DRUM/XY/OUIJA), keyboard hides.
        if (!surfaceRight_.isOpen() || !surfaceRight_.isVisible())
            subPlaySurface_.setVisible(true);
        else
            subPlaySurface_.setVisible(false);

        // Transport bar (submarine) replaces the old status bar at the bottom.
        if (transportBar_)
            transportBar_->setBounds(0,
                                     getHeight() - kStatusBarH,
                                     getWidth(),
                                     kStatusBarH);

        // Legacy status bar (Gallery) — hidden when transport bar is active.
        if (statusBar_)
        {
            if (transportBar_)
                statusBar_->setVisible(false);
            else
                statusBar_->setBounds(0,
                                      getHeight() - kStatusBarH,
                                      getWidth(),
                                      kStatusBarH);
        }

        // DetailOverlay covers the ocean area (excludes dashboard).
        detailOverlay_.setBounds(oceanArea);

        // Phase 2: CouplingConfigPopup covers the full ocean area.
        couplingPopup_.setBounds(getOceanArea());

        // #1008 FIX 7: DimOverlay covers the ocean + waterline area so the dim
        // effect extends to the waterline but doesn't dim the dashboard itself.
        dimOverlay_.setBounds(fullBounds.withTrimmedBottom(getEffectiveDashboardH() + kStatusBarH));

        // Phase 3: Engine picker drawer — full height of ocean area, fixed width.
        engineDrawer_.setBounds(oceanArea.withWidth(EnginePickerDrawer::kDrawerWidth));

        // Settings drawer — slides from the RIGHT edge of the ocean area.
        settingsDrawer_.setBounds(oceanArea
            .withLeft(oceanArea.getRight() - SettingsDrawer::kDrawerWidth)
            .withWidth(SettingsDrawer::kDrawerWidth));
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

            // 1. Check each visible orbit's bounds.
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

            // 2. Empty ocean — no buoy or knot hit.
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
        juce::ignoreUnused(e);
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
        nexus_.setPresetName(name);
        // #1007 FIX 3: Keep the inline header label in sync so the spatial grouping
        // "< Preset Name >" is always accurate.
        presetNameLabel_.setText(name, juce::dontSendNotification);
    }
    void setMoodName(const juce::String& mood)     { nexus_.setMoodName(mood); }
    void setMoodColour(juce::Colour colour)        { nexus_.setMoodColour(colour); }

    void setDNA(float b, float w, float m, float d, float s, float a)
    {
        nexus_.setDNA(b, w, m, d, s, a);
    }

    /** Direct access to the NexusDisplay for feeding session DNA drift. */
    NexusDisplay& getNexus() { return nexus_; }

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

    /** Step 8c: Trigger a ripple animation on the buoy wreath for the given slot.
        Called from the editor timer when the voice count increases (note-on). */
    void triggerBuoyRipple(int slot)
    {
        if (slot >= 0 && slot < 5)
            orbits_[slot].triggerRipple();
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

    // #909: Forward live readouts to NexusDisplay so Overview shows parameter activity.
    // voiceCount: total polyphonic voices across all slots.
    // macroValues: current normalised [0,1] values for macros 1-4.
    void setLiveReadouts(int voiceCount, const std::array<float, 4>& macroValues)
    {
        nexus_.setLiveReadouts(voiceCount, macroValues);
    }

    // Feature 6 (Schulze): Sustained-voice DNA accumulation.
    // Call from the editor's 10 Hz timer to drive continuous DNA drift when
    // voices are held.  totalVoices is the sum across all engine slots.
    void tickSustainedDna(int totalVoices, float dtSeconds)
    {
        nexus_.setSustainedVoiceCount(totalVoices);
        nexus_.tickSustainedDna(dtSeconds);
    }

    // Feature 7 (Schulze): Forward coupling timeline data to StatusBar.
    // Reads current route states from the substrate and pushes a snapshot
    // to the StatusBar's session timeline strip.
    void updateCouplingTimeline()
    {
        if (!statusBar_)
            return;

        const auto snapshots = substrate_.getTimelineSnapshot();
        std::vector<StatusBar::TimelineEntry> entries;
        entries.reserve(snapshots.size());
        for (const auto& s : snapshots)
            entries.push_back({ s.colour, s.age });
        statusBar_->setCouplingTimeline(entries);
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

    MacroSection*      getMacroSection()  noexcept { return macros_.get(); }
    EngineDetailPanel* getDetailPanel()   noexcept { return detail_.get(); }
    SidebarPanel*      getSidebar()       noexcept { return sidebar_.get(); }
    StatusBar*         getStatusBar()     noexcept { return statusBar_.get(); }

    juce::TextButton& presetPrevButton()   noexcept { return presetPrev_; }
    juce::TextButton& presetNextButton()   noexcept { return presetNext_; }
    juce::TextButton& favToggleButton()    noexcept { return favButton_; }
    juce::TextButton& settingsTogButton()  noexcept { return settingsButton_; }

    //==========================================================================
    // Navigation callbacks — fired to XOceanusEditor
    //==========================================================================

    /** Fired when an engine slot is selected (zoom-in). -1 means deselected. */
    std::function<void(int slot)> onEngineSelected;

    /** Fired when an engine slot enters SplitTransform (double-click dive). */
    std::function<void(int slot)> onEngineDiveDeep;

    /** Fired when the user selects an engine from the Engine Library drawer (Phase 3).
        The editor should load this engine into the active slot (or next available). */
    std::function<void(const juce::String& engineId)> onEngineSelectedFromDrawer;

    /** Fired when the user clicks a preset dot in the DNA map browser. */
    std::function<void(int presetIndex)> onPresetSelected;

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

            const juce::Font tabFont(juce::FontOptions{}
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

        const juce::String& activeTab() const noexcept
        {
            static const juce::String names[] = {"KEYS","PAD","DRUM","XY","OUIJA"};
            return names[juce::jlimit(0, kNumTabs - 1, activeIdx_)];
        }

        std::function<void(const juce::String&)> onTabChanged;
        std::function<void(bool)> onSeqToggled;
        std::function<void(bool)> onChordToggled;

    private:
        static constexpr int kNumTabs = 5;
        static constexpr const char* kTabNames[kNumTabs] = {"KEYS", "PAD", "DRUM", "XY", "OUIJA"};

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
            g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
            g.drawText("CLICK ME",
                       juce::Rectangle<float>(cx - 40, drawY - 8, 80, 16).toNearestInt(),
                       juce::Justification::centred, false);

            // Subtitle
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.25f));
            g.setFont(juce::Font(juce::FontOptions(9.0f)));
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

    /** Show a PopupMenu for a right-click on engine buoy at @p slot. */
    void showBuoyContextMenu(int slot)
    {
        const bool muted  = slotMuted_ [static_cast<size_t>(slot)];
        const bool soloed = slotSoloed_[static_cast<size_t>(slot)];

        juce::PopupMenu menu;

        // Item 1: Open Detail
        menu.addItem(1, "Open Detail");

        menu.addSeparator();

        // Items 2-3: Mute / Solo toggles
        menu.addItem(2, muted  ? "Unmute" : "Mute");
        menu.addItem(3, soloed ? "Unsolo" : "Solo");

        menu.addSeparator();

        // Item 4: Swap Engine
        menu.addItem(4, "Swap Engine...");

        // Item 5: Remove (destructive — shown in a slightly dimmer colour by
        // default via JUCE's native popup styling)
        menu.addItem(5, "Remove");

        menu.showMenuAsync(juce::PopupMenu::Options{},
            [this, slot](int result)
            {
                switch (result)
                {
                    case 1:
                        // Open Detail — transition to ZoomIn then split.
                        transitionToZoomIn(slot);
                        transitionToSplitTransform(slot);
                        break;

                    case 2:
                        // Mute toggle.
                        slotMuted_[static_cast<size_t>(slot)] = !slotMuted_[static_cast<size_t>(slot)];
                        if (onEngineMuteToggled)
                            onEngineMuteToggled(slot);
                        break;

                    case 3:
                        // Solo toggle.
                        slotSoloed_[static_cast<size_t>(slot)] = !slotSoloed_[static_cast<size_t>(slot)];
                        if (onEngineSoloToggled)
                            onEngineSoloToggled(slot);
                        break;

                    case 4:
                        // Swap Engine — open the engine picker drawer.
                        engineDrawer_.open();
                        break;

                    case 5:
                        // Remove engine.
                        if (onEngineRemoveRequested)
                            onEngineRemoveRequested(slot);
                        break;

                    default:
                        break;
                }
            });
    }

    /** Show a PopupMenu for a right-click on a coupling knot at @p chainIndex. */
    void showKnotContextMenu(int chainIndex)
    {
        juce::PopupMenu menu;

        menu.addItem(1, "Edit Coupling");
        menu.addSeparator();
        menu.addItem(2, "Delete Chain");

        menu.showMenuAsync(juce::PopupMenu::Options{},
            [this, chainIndex](int result)
            {
                switch (result)
                {
                    case 1:
                        // Show the coupling config popup (same as double-click).
                        couplingPopup_.show(chainIndex,
                                            "Engine A", juce::Colour(60, 180, 170),
                                            "Engine B", juce::Colour(140, 100, 220),
                                            0, 0.5f);
                        break;

                    case 2:
                        if (onCouplingDeleteRequested)
                            onCouplingDeleteRequested(chainIndex);
                        break;

                    default:
                        break;
                }
            });
    }

    /** Show a PopupMenu for a right-click on empty ocean (no buoy or knot hit). */
    void showEmptyOceanContextMenu()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Add Engine...");

        menu.showMenuAsync(juce::PopupMenu::Options{},
            [this](int result)
            {
                if (result == 1)
                    engineDrawer_.open();
            });
    }

    //==========================================================================
    // State machine transitions
    //==========================================================================

    void transitionToOrbital()
    {
        viewState_    = ViewState::Orbital;
        selectedSlot_ = -1;

        resized();

        if (onEngineSelected)
            onEngineSelected(-1);
    }

    void transitionToZoomIn(int slot)
    {
        // Toggling the same slot returns to Orbital.
        if (viewState_ == ViewState::ZoomIn && selectedSlot_ == slot)
        {
            transitionToOrbital();
            return;
        }

        viewState_    = ViewState::ZoomIn;
        selectedSlot_ = slot;

        resized();

        if (onEngineSelected)
            onEngineSelected(slot);
    }

    void transitionToSplitTransform(int slot)
    {
        viewState_    = ViewState::SplitTransform;
        selectedSlot_ = slot;

        resized();

        if (onEngineDiveDeep)
            onEngineDiveDeep(slot);
    }

    void transitionToBrowser()
    {
        // Snapshot the pre-browser state so exitBrowser() can restore it exactly.
        preBrowserState_    = viewState_;
        preBrowserSlot_     = selectedSlot_;

        viewState_ = ViewState::BrowserOpen;
        resized();
    }

    void exitBrowser()
    {
        // Restore whatever state was active before the browser was opened.
        if (preBrowserState_ == ViewState::ZoomIn && preBrowserSlot_ >= 0)
        {
            // transitionToZoomIn re-enters ZoomIn and fires onEngineSelected.
            transitionToZoomIn(preBrowserSlot_);
        }
        else if (preBrowserState_ == ViewState::SplitTransform && preBrowserSlot_ >= 0)
        {
            transitionToSplitTransform(preBrowserSlot_);
        }
        else
        {
            // Default: return to Orbital and clear selection.
            viewState_    = ViewState::Orbital;
            selectedSlot_ = -1;

            resized();

            if (onEngineSelected)
                onEngineSelected(-1);
        }

        // Reset saved pre-browser state so it cannot be accidentally re-used.
        preBrowserState_ = ViewState::Orbital;
        preBrowserSlot_  = -1;
    }

    //==========================================================================
    // Layout strategies
    //==========================================================================

    void layoutOrbital()
    {
        // Step 6: use getOceanArea() so background/substrate/nexus only fill
        // the ocean viewport above the waterline, not the full window.
        const auto area    = getOceanArea();
        const auto centerF = area.getCentre().toFloat();

        // Background, substrate, and ambient edge span the ocean viewport only.
        background_.setBounds(area);
        ambientEdge_.setBounds(area);
        substrate_.setBounds(area);
        substrate_.setVisible(true);

        // ── Nexus (centre, slightly above geometric centre) ──────────────────
        // kNexusH = 200: 96 hex + 8 gap + 22 name + 4 gap + 15 mood + 6 gap
        //              + 41 readouts + 8 margin = 200px total.
        // Raised from 160 so the 96px hex + readout strip fits without clipping.
        constexpr int kNexusW = 160;
        constexpr int kNexusH = 200;
        nexus_.setBounds(static_cast<int>(centerF.x) - kNexusW / 2,
                         static_cast<int>(centerF.y) - kNexusH / 2 - 20,
                         kNexusW, kNexusH);
        nexus_.setVisible(true);

        // ── Engine creatures (freeform normalized positions) ─────────────────
        int numLoaded = 0;
        for (const auto& o : orbits_)
            if (o.hasEngine()) ++numLoaded;

        // ── Macros: now positioned in the dashboard strip via resized() ──────
        // Fix 4: only show macros when at least one engine is loaded.
        if (macros_)
            macros_->setVisible(numLoaded > 0);

        if (numLoaded > 0)
        {
            for (int i = 0; i < 5; ++i)
            {
                if (!orbits_[i].hasEngine())
                {
                    orbits_[i].setVisible(false);
                    continue;
                }

                auto pos = orbits_[i].getNormalizedPosition();
                int sz = orbits_[i].getBuoySize() + kBreathPadding * 2;
                int x = static_cast<int>(pos.x * area.getWidth()) - sz / 2;
                int y = static_cast<int>(pos.y * area.getHeight()) - sz / 2;
                orbits_[i].setBounds(x + area.getX(), y + area.getY(), sz, sz);
                orbits_[i].setVisible(true);

                substrate_.setCreatureCenter(i, orbits_[i].getCenter());
            }
        }
        else
        {
            // BLOCKER 1: empty-state — no engines loaded.
            // Show a centred call-to-action label and hide all orbits.
            for (auto& o : orbits_)
                o.setVisible(false);

            emptyStateLabel_.setVisible(true);
            emptyStateLabel_.setBounds(
                static_cast<int>(centerF.x) - 150,
                static_cast<int>(centerF.y) + 20,
                300, 32);
        }

        // If we have engines, keep the empty-state label hidden.
        if (numLoaded > 0)
            emptyStateLabel_.setVisible(false);

        // Step 7: Show the pulsing lifesaver ring on first launch when empty.
        lifesaver_.setVisible(firstLaunch_ && numLoaded == 0);
        lifesaver_.setBounds(getOceanArea());

        // Hide panels that belong to other states.
        if (detail_)  { detail_->setVisible(false); }
        if (sidebar_) { sidebar_->setVisible(false); }
        browser_.setVisible(false);
    }

    void layoutZoomIn()
    {
        jassert(selectedSlot_ >= 0 && selectedSlot_ < 5);

        // Step 6: use getOceanArea() so background/substrate/nexus only fill
        // the ocean viewport above the waterline.
        const auto area    = getOceanArea();
        const auto centerF = area.getCentre().toFloat();
        const float halfMin = static_cast<float>(std::min(area.getWidth(),
                                                          area.getHeight())) * 0.5f;

        background_.setBounds(area);
        ambientEdge_.setBounds(area);
        substrate_.setBounds(area);
        substrate_.setVisible(true);

        // Nexus shifts toward top to give vertical room for the zoomed creature.
        // kNexusH = 200: 96 hex + 8 gap + 22 name + 4 gap + 15 mood + 6 gap
        //              + 41 readouts + 8 margin = 200px total.
        // Raised from 140 so the 96px hex + readout strip fits without clipping.
        constexpr int kNexusW = 160;
        constexpr int kNexusH = 200;
        nexus_.setBounds(static_cast<int>(centerF.x) - kNexusW / 2,
                         area.getY() + 30,
                         kNexusW, kNexusH);
        nexus_.setVisible(true);

        // Count non-selected loaded engines (for edge positioning).
        int edgeCount = 0;
        for (int i = 0; i < 5; ++i)
            if (orbits_[i].hasEngine() && i != selectedSlot_) ++edgeCount;

        // Distribute minimised creatures evenly along the far edge arc.
        const float edgeRadius = halfMin * 0.85f;
        const float arcStart   = juce::MathConstants<float>::halfPi;  // bottom
        const float arcTotal   = juce::MathConstants<float>::twoPi * 0.75f;  // 3/4 circle
        const float arcStep    = (edgeCount > 1) ? arcTotal / static_cast<float>(edgeCount - 1)
                                                 : 0.0f;

        int edgeIdx = 0;
        for (int i = 0; i < 5; ++i)
        {
            if (!orbits_[i].hasEngine())
            {
                orbits_[i].setVisible(false);
                continue;
            }

            if (i == selectedSlot_)
            {
                // Zoomed-in creature at the centre (slightly above geometric centre).
                const int size = static_cast<int>(kOrbitSize_Orbital);
                orbits_[i].setBounds(
                    static_cast<int>(centerF.x) - size / 2,
                    static_cast<int>(centerF.y) - size / 2 - 40,
                    size, size);
                substrate_.setCreatureCenter(i, orbits_[i].getCenter());
            }
            else
            {
                // Minimised creatures arranged along the outer arc.
                const float angle = arcStart + static_cast<float>(edgeIdx) * arcStep;
                const auto  pos   = polarToCartesian(angle, edgeRadius, centerF);
                const int   size  = static_cast<int>(kOrbitSize_Orbital);

                orbits_[i].setBounds(
                    static_cast<int>(pos.x) - size / 2,
                    static_cast<int>(pos.y) - size / 2,
                    size, size);
                substrate_.setCreatureCenter(i, pos);
                ++edgeIdx;
            }

            orbits_[i].setVisible(true);
        }

        // Macros: now positioned in the dashboard strip via resized().
        if (macros_)
            macros_->setVisible(true);

        if (detail_)  { detail_->setVisible(false); }
        if (sidebar_) { sidebar_->setVisible(false); }
        browser_.setVisible(false);
        emptyStateLabel_.setVisible(false);  // ZoomIn always has an engine selected
    }

    void layoutSplitTransform()
    {
        jassert(selectedSlot_ >= 0 && selectedSlot_ < 5);

        // Step 6: use getOceanArea() so background/ambient edge stay within the
        // ocean viewport above the waterline.
        const auto area    = getOceanArea();
        const int  orbW    = static_cast<int>(static_cast<float>(area.getWidth())
                                               * kSplitOrbitalFraction);
        const int  detailW = area.getWidth() - orbW;

        // Background and ambient edge span the ocean viewport.
        background_.setBounds(area);
        ambientEdge_.setBounds(area);

        // Substrate is clipped to the orbital strip in split mode.
        substrate_.setBounds(0, 0, orbW, area.getHeight());
        substrate_.setVisible(true);

        // ── Mini orbital strip ────────────────────────────────────────────────
        // Stack loaded creatures vertically within the left strip.
        // y starts at 48 (was 40) to clear the 44px header button strip (#1006).
        int  y        = 48;
        int  stripCx  = orbW / 2;
        const int miniSize = static_cast<int>(kOrbitSize_Orbital);

        for (int i = 0; i < 5; ++i)
        {
            if (!orbits_[i].hasEngine())
            {
                orbits_[i].setVisible(false);
                continue;
            }

            const int sz = (i == selectedSlot_)
                               ? static_cast<int>(kOrbitSize_Orbital * 0.6f)
                               : miniSize;

            orbits_[i].setBounds(stripCx - sz / 2, y, sz, sz);
            orbits_[i].setVisible(true);

            substrate_.setCreatureCenter(i, orbits_[i].getCenter());
            y += sz + 16;
        }

        // Nexus and macros are hidden in SplitTransform — the detail panel
        // owns the identity display on the right.
        nexus_.setVisible(false);
        if (macros_) macros_->setVisible(false);

        // ── Detail panel occupies the right 80% ───────────────────────────────
        if (detail_)
        {
            detail_->setBounds(orbW, 0, detailW, area.getHeight());
            detail_->setVisible(true);
            detail_->loadSlot(selectedSlot_);
        }

        if (sidebar_) sidebar_->setVisible(false);
        browser_.setVisible(false);
        emptyStateLabel_.setVisible(false);  // SplitTransform always has an engine selected
    }

    void layoutBrowser()
    {
        // Step 6: browser covers the ocean viewport above the waterline only.
        const auto area = getOceanArea();

        // Browser covers the ocean viewport.
        browser_.setBounds(area);
        browser_.setVisible(true);

        // All orbital components are hidden while the browser is open.
        background_.setBounds(area);  // keep background behind the browser
        ambientEdge_.setBounds(area);
        substrate_.setVisible(false);

        for (auto& o : orbits_)
            o.setVisible(false);

        nexus_.setVisible(false);
        if (macros_)  macros_->setVisible(false);
        if (detail_)  detail_->setVisible(false);
        if (sidebar_) sidebar_->setVisible(false);
        emptyStateLabel_.setVisible(false);  // browser has its own empty state
    }

    //==========================================================================
    // Floating header controls layout
    //==========================================================================

    void layoutFloatingControls()
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
        enginesButton_.setBounds(kLeftMargin, kTopMargin, kEnginesW, kBtnH);
        const int afterEngines = kLeftMargin + kEnginesW + 4;

        presetPrev_.setBounds(afterEngines,
                              kTopMargin,
                              kNavW, kBtnH);

        // #1007 FIX 3: Inline preset name label sits between < and > so the
        // spatial grouping "< Preset Name >" is immediately legible.
        // Width is capped at 160pt so it doesn't crowd the fav button.
        constexpr int kNameLabelW = 160;
        presetNameLabel_.setBounds(afterEngines + kNavW,
                                   kTopMargin,
                                   kNameLabelW, kBtnH);

        presetNext_.setBounds(afterEngines + kNavW + kNameLabelW + kGap,
                              kTopMargin,
                              kNavW, kBtnH);

        favButton_.setBounds(afterEngines + kNavW + kNameLabelW + kNavW + kGap * 2,
                             kTopMargin,
                             kFavW, kBtnH);

        // ── Right cluster: settings | KEYS ────────────────────────────────────
        constexpr int kSettingsW = 44;   // #908: minimum square tap target
        constexpr int kKeysW    = 56;    // KEYS label needs slightly more width
        constexpr int kRightMargin = 4;

        settingsButton_.setBounds(getWidth() - kRightMargin - kSettingsW - kGap - kKeysW,
                                  kTopMargin,
                                  kSettingsW, kBtnH);

        keysButton_.setBounds(getWidth() - kRightMargin - kKeysW,
                              kTopMargin,
                              kKeysW, kBtnH);
    }

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
        const int wlH = waterline_ ? waterline_->getDesiredHeight() : kWaterlineH;
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
     *  overlapped the NexusDisplay border at the default 1100×750 window size.
     *  0.38 gives ~10 px clearance between the nexus edge and the creature edge.
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
    // Buoy position persistence (PropertiesFile, keyed by slot index)
    // Design decision D1: Global positions, NOT per-preset. Never APVTS.
    //==========================================================================

    /** Save a single slot's normalised position to the XOceanus settings file. */
    void saveSlotPosition(int slot)
    {
        juce::PropertiesFile::Options opts;
        opts.applicationName     = "XOceanus";
        opts.filenameSuffix      = "settings";
        opts.osxLibrarySubFolder = "Application Support";
        juce::PropertiesFile settings(opts);

        auto pos = orbits_[slot].getNormalizedPosition();
        settings.setValue("buoyPosX_" + juce::String(slot), static_cast<double>(pos.x));
        settings.setValue("buoyPosY_" + juce::String(slot), static_cast<double>(pos.y));
        settings.saveIfNeeded();
    }

    /** Load all 5 slot positions from settings; fall back to default arc positions. */
    void loadSlotPositions()
    {
        // Default positions: five buoys spread across the ocean in an arc.
        const float defaultPositions[5][2] = {
            { 0.30f, 0.40f }, // slot 0
            { 0.55f, 0.30f }, // slot 1
            { 0.70f, 0.50f }, // slot 2
            { 0.45f, 0.60f }, // slot 3
            { 0.20f, 0.55f }, // slot 4 (ghost)
        };

        juce::PropertiesFile::Options opts;
        opts.applicationName     = "XOceanus";
        opts.filenameSuffix      = "settings";
        opts.osxLibrarySubFolder = "Application Support";
        juce::PropertiesFile settings(opts);

        for (int i = 0; i < 5; ++i)
        {
            const float x = static_cast<float>(
                settings.getDoubleValue("buoyPosX_" + juce::String(i),
                                        static_cast<double>(defaultPositions[i][0])));
            const float y = static_cast<float>(
                settings.getDoubleValue("buoyPosY_" + juce::String(i),
                                        static_cast<double>(defaultPositions[i][1])));
            orbits_[i].setNormalizedPosition({ x, y });
        }
    }

    /**
        Restore the canonical Z-order of all overlay and floating components.

        Called after each deferred init (initMacros, initDetailPanel, initSidebar,
        initStatusBar) because addAndMakeVisible() pushes the new component to the
        front and disturbs the Z-stack.

        Order (bottom → top):
          ambientEdge_ | detail_ | sidebar_ | browser_ |
          presetPrev_ | presetNext_ | favButton_ | settingsButton_ | keysButton_ |
          dimOverlay_  ← #1008 FIX 7: above buttons, so buttons are dimmed |
          playSurfaceOverlay_ | statusBar_
    */
    void reorderZStack()
    {
        ambientEdge_.toFront(false);
        // Fix 6: macros must render above the vignette overlay (ambientEdge_).
        if (macros_) macros_->toFront(false);
        if (detail_)   detail_->toFront(false);
        if (sidebar_)  sidebar_->toFront(false);
        browser_.toFront(false);
        // DetailOverlay floats above orbits/substrate/browser but below header buttons.
        detailOverlay_.toFront(false);
        // Phase 2: CouplingConfigPopup sits above detailOverlay_ but below header buttons.
        couplingPopup_.toFront(false);
        presetPrev_.toFront(false);
        presetNext_.toFront(false);
        favButton_.toFront(false);
        settingsButton_.toFront(false);
        keysButton_.toFront(false);
        presetNameLabel_.toFront(false);
        // #1008 FIX 7: dimOverlay_ above buttons but below PlaySurfaceOverlay.
        dimOverlay_.toFront(false);
        // Step 6: waterline and tab bar sit above the dim overlay but below
        // the PlaySurface so they are always legible.
        surfaceRight_.toFront(false);
        exprStrips_.toFront(false);
        subPlaySurface_.toFront(false);
        playSurfaceOverlay_.toFront(false);
        ouijaPanel_.toFront(false);
        if (waterline_) waterline_->toFront(false);
        if (masterFxStrip_) masterFxStrip_->toFront(false);
        tabBar_.toFront(false);
        if (chordBar_) chordBar_->toFront(false);
        if (transportBar_) transportBar_->toFront(false);
        if (statusBar_) statusBar_->toFront(false);
    }

    //==========================================================================
    // State
    //==========================================================================

    ViewState viewState_       = ViewState::Orbital;
    int       selectedSlot_    = -1;
    float     dimAlpha_        = 1.0f;  ///< < 1 when PlaySurface or browser dims the scene

    /// Per-slot mute / solo toggle state — tracked locally so the context menu
    /// label can reflect the current state without a round-trip to the processor.
    std::array<bool, 5> slotMuted_  {};
    std::array<bool, 5> slotSoloed_ {};

    /// Step 7: true until the user loads their first engine or clicks the lifesaver.
    bool firstLaunch_ = true;

    /// State saved on entering BrowserOpen so exitBrowser() can restore it exactly.
    ViewState preBrowserState_ = ViewState::Orbital;
    int       preBrowserSlot_  = -1;

    //==========================================================================
    // Child components — Z-ordered (bottom → top) as declared
    //==========================================================================

    OceanBackground background_;
    CouplingSubstrate substrate_;
    std::array<EngineOrbit, 5> orbits_;      ///< 4 primary engine slots + 1 ghost slot
    NexusDisplay nexus_;
    AmbientEdge  ambientEdge_;

    // Deferred-init components (require external references at construction time).
    std::unique_ptr<MacroSection>      macros_;
    std::unique_ptr<EngineDetailPanel> detail_;
    std::unique_ptr<SidebarPanel>      sidebar_;
    std::unique_ptr<StatusBar>         statusBar_;

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

    // Phase 2: Coupling knot configuration popup (shown on double-click of a knot).
    CouplingConfigPopup  couplingPopup_;

    // Step 6: Submarine dashboard — waterline separator + tab bar.
    std::unique_ptr<TideWaterline>        waterline_;
    std::unique_ptr<ChordBarComponent>    chordBar_;
    std::unique_ptr<MasterFXStripCompact> masterFxStrip_;
    std::unique_ptr<TransportBar>         transportBar_;
    SubmarineOuijaPanel                   ouijaPanel_;
    ExpressionStrips                      exprStrips_;
    DotMatrixDisplay                      dotMatrix_;
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
    static constexpr int   kDashboardH          = 400;    ///< macros (60) + FX (48) + tabs (30) + play (~262)
    static constexpr int   kTabBarH             = 30;

    // HIGH fix (#1006): padding added to orbital bounds so ±5% breath animation
    // paints inside the component rect.  ceil(72 * 0.05) = 4px each side.
    static constexpr int kBreathPadding = 4;

    // Orbit size alias: reference EngineOrbit constant directly.
    static constexpr float kOrbitSize_Orbital = EngineOrbit::kOrbitalSize;

    //==========================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OceanView)
};

} // namespace xoceanus
