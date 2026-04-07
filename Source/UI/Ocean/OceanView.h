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
#include "DnaMapBrowser.h"
#include "PlaySurfaceOverlay.h"
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
class OceanView : public juce::Component, private juce::Timer
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
        addAndMakeVisible(browser_);
        browser_.setVisible(false);

        // 10. PlaySurface overlay (hidden by default; manages its own visibility)
        addAndMakeVisible(playSurfaceOverlay_);

        // 11. Floating header controls
        addAndMakeVisible(presetPrev_);
        addAndMakeVisible(presetNext_);
        addAndMakeVisible(favButton_);
        addAndMakeVisible(settingsButton_);
        addAndMakeVisible(keysButton_);

        // 12. StatusBar placeholder until initStatusBar()
        // statusBar_ is a unique_ptr — added in initStatusBar()

        // ── Button styling ────────────────────────────────────────────────────
        auto styleHeaderButton = [](juce::TextButton& btn, int minW)
        {
            btn.setColour(juce::TextButton::buttonColourId,
                          juce::Colour(GalleryColors::Ocean::deep));
            btn.setColour(juce::TextButton::buttonOnColourId,
                          juce::Colour(GalleryColors::xoGold).withAlpha(0.25f));
            btn.setColour(juce::TextButton::textColourOffId,
                          juce::Colour(GalleryColors::Ocean::foam));
            btn.setColour(juce::TextButton::textColourOnId,
                          juce::Colour(GalleryColors::xoGold));
            // #908: initial size hint — layoutFloatingControls() sets definitive bounds.
            btn.setSize(minW, 44); // 44pt height = WCAG AA minimum touch target
        };
        styleHeaderButton(presetPrev_,   44);
        styleHeaderButton(presetNext_,   44);
        styleHeaderButton(favButton_,    44);
        styleHeaderButton(settingsButton_, 44);
        styleHeaderButton(keysButton_,   56);

        favButton_.setTooltip("Toggle favourite");
        settingsButton_.setTooltip("Settings");
        keysButton_.setTooltip("Toggle Play Surface (K)");
        presetPrev_.setTooltip("Previous preset");
        presetNext_.setTooltip("Next preset");

        A11y::setup(keysButton_,      "Keys toggle", "Show or hide the Play Surface panel");
        A11y::setup(settingsButton_,  "Settings");
        A11y::setup(favButton_,       "Favourite");
        A11y::setup(presetPrev_,      "Previous preset");
        A11y::setup(presetNext_,      "Next preset");

        // ── Internal callbacks ────────────────────────────────────────────────
        for (int i = 0; i < 5; ++i)
        {
            orbits_[i].onClicked       = [this](int s) { transitionToZoomIn(s); };
            orbits_[i].onDoubleClicked = [this](int s) { transitionToSplitTransform(s); };
        }

        nexus_.onPresetNameClicked = [this]() { transitionToBrowser(); };

        browser_.onPresetSelected = [this](int idx)
        {
            if (onPresetSelected)
                onPresetSelected(idx);
            exitBrowser();
        };
        browser_.onDismissed = [this]() { exitBrowser(); };

        playSurfaceOverlay_.onDimStateChanged = [this](bool dim)
        {
            dimAlpha_ = dim ? 0.35f : 1.0f;
            repaint();
        };

        keysButton_.onClick = [this]() { togglePlaySurface(); };

        // ── Keyboard focus + polling timer ────────────────────────────────────
        setWantsKeyboardFocus(true);
        startTimerHz(10);
    }

    ~OceanView() override
    {
        stopTimer();
    }

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
        // JUCE renders children in the order they were added, so we need to
        // push macros below ambientEdge in paint order.  The simplest way is
        // to rearrange z-order explicitly.
        macros_->toFront(false);
        ambientEdge_.toFront(false);
        if (detail_)   detail_->toFront(false);
        if (sidebar_)  sidebar_->toFront(false);
        browser_.toFront(false);
        playSurfaceOverlay_.toFront(false);
        presetPrev_.toFront(false);
        presetNext_.toFront(false);
        favButton_.toFront(false);
        settingsButton_.toFront(false);
        keysButton_.toFront(false);
        if (statusBar_) statusBar_->toFront(false);

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

        // Push to correct z position.
        browser_.toFront(false);
        playSurfaceOverlay_.toFront(false);
        presetPrev_.toFront(false);
        presetNext_.toFront(false);
        favButton_.toFront(false);
        settingsButton_.toFront(false);
        keysButton_.toFront(false);
        if (statusBar_) statusBar_->toFront(false);

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

        browser_.toFront(false);
        playSurfaceOverlay_.toFront(false);
        presetPrev_.toFront(false);
        presetNext_.toFront(false);
        favButton_.toFront(false);
        settingsButton_.toFront(false);
        keysButton_.toFront(false);
        if (statusBar_) statusBar_->toFront(false);

        resized();
    }

    /**
        Initialise the StatusBar.
        Must be called before the component becomes visible.
    */
    void initStatusBar()
    {
        statusBar_ = std::make_unique<StatusBar>();
        addAndMakeVisible(*statusBar_);
        statusBar_->toFront(false);
        resized();
    }

    //==========================================================================
    // juce::Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
        // The dim overlay for PlaySurface / BrowserOpen is handled by the
        // individual overlay components (frosted glass, dimmed background).
        // We apply a faint dimming rect over the ocean scene when dimAlpha_ < 1.
        if (dimAlpha_ < 0.999f)
        {
            g.setColour(juce::Colour(GalleryColors::Ocean::abyss)
                            .withAlpha(1.0f - dimAlpha_));
            g.fillRect(getLocalBounds());
        }
    }

    void resized() override
    {
        switch (viewState_)
        {
            case ViewState::Orbital:        layoutOrbital();        break;
            case ViewState::ZoomIn:         layoutZoomIn();         break;
            case ViewState::SplitTransform: layoutSplitTransform(); break;
            case ViewState::BrowserOpen:    layoutBrowser();        break;
        }

        layoutFloatingControls();

        // Status bar always spans the full bottom strip.
        if (statusBar_)
            statusBar_->setBounds(0,
                                  getHeight() - kStatusBarH,
                                  getWidth(),
                                  kStatusBarH);

        // PlaySurface overlay always covers the full parent area so it can
        // self-position via repositionFromOffset() inside PlaySurfaceOverlay.
        playSurfaceOverlay_.setBounds(getLocalBounds());
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        // Escape: exit any overlay, or return to Orbital from any state.
        if (key == juce::KeyPress::escapeKey)
        {
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
        // Clicking the backdrop in ZoomIn mode (outside any creature) returns
        // to Orbital.  We check whether the click landed on a child component
        // via hitTest propagation — if we receive it here, no child caught it.
        if (viewState_ == ViewState::ZoomIn)
        {
            transitionToOrbital();
            juce::ignoreUnused(e);
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

        orbits_[slot].setEngine(engineId, accent, zone);
        resized();  // recompute polar positions now numLoaded has changed
    }

    /** Remove the engine from a slot. */
    void clearEngine(int slot)
    {
        if (slot < 0 || slot >= 5)
            return;

        orbits_[slot].clearEngine();

        // If the cleared slot was selected, return to Orbital.
        if (selectedSlot_ == slot)
            transitionToOrbital();
        else
            resized();
    }

    //==========================================================================
    // Preset data setters
    //==========================================================================

    void setPresetName(const juce::String& name)  { nexus_.setPresetName(name); }
    void setMoodName(const juce::String& mood)     { nexus_.setMoodName(mood); }
    void setMoodColour(juce::Colour colour)        { nexus_.setMoodColour(colour); }

    void setDNA(float b, float w, float m, float d, float s, float a)
    {
        nexus_.setDNA(b, w, m, d, s, a);
    }

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
            orbits_[slot].setVoiceCount(count);
    }

    // #909: Forward live readouts to NexusDisplay so Overview shows parameter activity.
    // voiceCount: total polyphonic voices across all slots.
    // macroValues: current normalised [0,1] values for macros 1-4.
    void setLiveReadouts(int voiceCount, const std::array<float, 4>& macroValues)
    {
        nexus_.setLiveReadouts(voiceCount, macroValues);
    }

    void setCouplingLean(int slot, float lean)
    {
        if (slot >= 0 && slot < 5)
            orbits_[slot].setCouplingLean(lean);
    }

    /** Notify OceanBackground of which depth zones are populated. */
    void setDepthZones(bool sunlit, bool twilight, bool midnight)
    {
        juce::ignoreUnused(sunlit, twilight, midnight);
        // OceanBackground always renders all three ring tiers; this hook is
        // reserved for future per-zone glow intensity adaptation.
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

    /** Fired when the user clicks a preset dot in the DNA map browser. */
    std::function<void(int presetIndex)> onPresetSelected;

    /** Fired when the PlaySurface overlay is shown or hidden (including first-launch auto-show).
        Use this to persist the visibility preference so subsequent plugin launches restore the
        last user-chosen state.  true = overlay is now showing, false = hidden. */
    std::function<void(bool visible)> onPlaySurfaceVisibilityChanged;

    //==========================================================================
    // State queries
    //==========================================================================

    ViewState getViewState()    const noexcept { return viewState_; }
    int       getSelectedSlot() const noexcept { return selectedSlot_; }

private:
    //==========================================================================
    // juce::Timer — 10 Hz state polling
    //==========================================================================

    void timerCallback() override
    {
        // Reserved for future live-state polling that the editor has not
        // already handled (e.g. per-frame breath sync, MIDI activity indicators).
        // Kept deliberately lightweight — real work is triggered by editor callbacks.
    }

    //==========================================================================
    // State machine transitions
    //==========================================================================

    void transitionToOrbital()
    {
        viewState_    = ViewState::Orbital;
        selectedSlot_ = -1;

        for (int i = 0; i < 5; ++i)
        {
            orbits_[i].setInteractionState(EngineOrbit::InteractionState::Orbital);
        }

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

        for (int i = 0; i < 5; ++i)
        {
            orbits_[i].setInteractionState(
                i == slot ? EngineOrbit::InteractionState::ZoomIn
                          : EngineOrbit::InteractionState::Minimized);
        }

        resized();

        if (onEngineSelected)
            onEngineSelected(slot);
    }

    void transitionToSplitTransform(int slot)
    {
        viewState_    = ViewState::SplitTransform;
        selectedSlot_ = slot;

        for (int i = 0; i < 5; ++i)
        {
            orbits_[i].setInteractionState(
                i == slot ? EngineOrbit::InteractionState::ZoomIn
                          : EngineOrbit::InteractionState::Minimized);
        }

        resized();

        if (onEngineDiveDeep)
            onEngineDiveDeep(slot);
    }

    void transitionToBrowser()
    {
        viewState_ = ViewState::BrowserOpen;
        resized();
    }

    void exitBrowser()
    {
        viewState_ = ViewState::Orbital;
        resized();
    }

    //==========================================================================
    // Layout strategies
    //==========================================================================

    void layoutOrbital()
    {
        const auto area   = getLocalBounds().withTrimmedBottom(kStatusBarH);
        const auto centerF = area.getCentre().toFloat();
        const float halfMin = static_cast<float>(std::min(area.getWidth(),
                                                          area.getHeight())) * 0.5f;

        // Background, substrate, and ambient edge always span the full area.
        background_.setBounds(area);
        ambientEdge_.setBounds(area);
        substrate_.setBounds(area);
        substrate_.setVisible(true);

        // ── Nexus (centre, slightly above geometric centre) ──────────────────
        constexpr int kNexusW = 160;
        constexpr int kNexusH = 120;
        nexus_.setBounds(static_cast<int>(centerF.x) - kNexusW / 2,
                         static_cast<int>(centerF.y) - kNexusH / 2 - 20,
                         kNexusW, kNexusH);
        nexus_.setVisible(true);

        // ── Macros below nexus ────────────────────────────────────────────────
        if (macros_)
        {
            macros_->setBounds(static_cast<int>(centerF.x) - 200,
                               nexus_.getBottom() + 8,
                               400,
                               static_cast<int>(kMacroStripH));
            macros_->setVisible(true);
        }

        // ── Engine creatures in polar orbit ───────────────────────────────────
        int numLoaded = 0;
        for (const auto& o : orbits_)
            if (o.hasEngine()) ++numLoaded;

        if (numLoaded > 0)
        {
            const float angleStep = juce::MathConstants<float>::twoPi
                                    / static_cast<float>(numLoaded);
            int idx = 0;
            for (int i = 0; i < 5; ++i)
            {
                if (!orbits_[i].hasEngine())
                {
                    orbits_[i].setVisible(false);
                    continue;
                }

                // Start angle from top (−π/2) and rotate clockwise.
                const float angle  = static_cast<float>(idx) * angleStep
                                     - juce::MathConstants<float>::halfPi;
                const float radius = radiusForZone(orbits_[i].getDepthZone()) * halfMin;
                const auto  pos    = polarToCartesian(angle, radius, centerF);

                const int size = static_cast<int>(kOrbitSize_Orbital);
                orbits_[i].setTargetBounds({
                    static_cast<int>(pos.x) - size / 2,
                    static_cast<int>(pos.y) - size / 2,
                    size, size
                });
                orbits_[i].setInteractionState(EngineOrbit::InteractionState::Orbital);
                orbits_[i].setVisible(true);

                substrate_.setCreatureCenter(i, pos);
                ++idx;
            }
        }
        else
        {
            for (auto& o : orbits_)
                o.setVisible(false);
        }

        // Hide panels that belong to other states.
        if (detail_)  { detail_->setVisible(false); }
        if (sidebar_) { sidebar_->setVisible(false); }
        browser_.setVisible(false);
    }

    void layoutZoomIn()
    {
        const auto area    = getLocalBounds().withTrimmedBottom(kStatusBarH);
        const auto centerF = area.getCentre().toFloat();
        const float halfMin = static_cast<float>(std::min(area.getWidth(),
                                                          area.getHeight())) * 0.5f;

        background_.setBounds(area);
        ambientEdge_.setBounds(area);
        substrate_.setBounds(area);
        substrate_.setVisible(true);

        // Nexus shifts toward top to give vertical room for the zoomed creature.
        constexpr int kNexusW = 160;
        constexpr int kNexusH = 100;
        nexus_.setBounds(static_cast<int>(centerF.x) - kNexusW / 2,
                         30,
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
                const int size = static_cast<int>(kOrbitSize_ZoomIn);
                orbits_[i].setTargetBounds({
                    static_cast<int>(centerF.x) - size / 2,
                    static_cast<int>(centerF.y) - size / 2 - 40,
                    size, size
                });
                substrate_.setCreatureCenter(i, orbits_[i].getCenter());
            }
            else
            {
                // Minimised creatures arranged along the outer arc.
                const float angle = arcStart + static_cast<float>(edgeIdx) * arcStep;
                const auto  pos   = polarToCartesian(angle, edgeRadius, centerF);
                const int   size  = static_cast<int>(kOrbitSize_Minimized);

                orbits_[i].setTargetBounds({
                    static_cast<int>(pos.x) - size / 2,
                    static_cast<int>(pos.y) - size / 2,
                    size, size
                });
                substrate_.setCreatureCenter(i, pos);
                ++edgeIdx;
            }

            orbits_[i].setVisible(true);
        }

        // Macros below the zoomed creature.
        if (macros_)
        {
            const int macroY = static_cast<int>(centerF.y)
                               + static_cast<int>(kOrbitSize_ZoomIn / 2.0f)
                               + 16;
            macros_->setBounds(static_cast<int>(centerF.x) - 200,
                               macroY, 400,
                               static_cast<int>(kMacroStripH));
            macros_->setVisible(true);
        }

        if (detail_)  { detail_->setVisible(false); }
        if (sidebar_) { sidebar_->setVisible(false); }
        browser_.setVisible(false);
    }

    void layoutSplitTransform()
    {
        const auto area   = getLocalBounds().withTrimmedBottom(kStatusBarH);
        const int  orbW   = static_cast<int>(static_cast<float>(area.getWidth())
                                              * kSplitOrbitalFraction);
        const int  detailW = area.getWidth() - orbW;

        // Background and ambient edge span the full area (they layer under everything).
        background_.setBounds(area);
        ambientEdge_.setBounds(area);

        // Substrate is clipped to the orbital strip in split mode.
        substrate_.setBounds(0, 0, orbW, area.getHeight());
        substrate_.setVisible(true);

        // ── Mini orbital strip ────────────────────────────────────────────────
        // Stack loaded creatures vertically within the left strip.
        int  y        = 40;
        int  stripCx  = orbW / 2;
        const int miniSize = static_cast<int>(kOrbitSize_Minimized);

        for (int i = 0; i < 5; ++i)
        {
            if (!orbits_[i].hasEngine())
            {
                orbits_[i].setVisible(false);
                continue;
            }

            const int sz = (i == selectedSlot_)
                               ? static_cast<int>(kOrbitSize_ZoomIn * 0.6f)
                               : miniSize;

            orbits_[i].setTargetBounds({
                stripCx - sz / 2,
                y,
                sz, sz
            });
            orbits_[i].setInteractionState(
                i == selectedSlot_ ? EngineOrbit::InteractionState::ZoomIn
                                   : EngineOrbit::InteractionState::Minimized);
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
    }

    void layoutBrowser()
    {
        const auto area = getLocalBounds().withTrimmedBottom(kStatusBarH);

        // Browser covers the entire canvas.
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
    }

    //==========================================================================
    // Floating header controls layout
    //==========================================================================

    void layoutFloatingControls()
    {
        // ── Left cluster: prev | next | fav ──────────────────────────────────
        // #908: WCAG 2.5.5 requires a minimum 44×44pt touch target.
        // Visual height stays ~28pt via GalleryLookAndFeel's text rendering,
        // but the component bounds are expanded to 44pt so pointer/touch events
        // have a compliant target size.  The button background fill matches the
        // ocean scene so the extra transparent hit area is invisible.
        constexpr int kBtnH      = 44;   // #908: WCAG AA minimum (was 28)
        constexpr int kNavW      = 44;   // #908: square touch target for nav arrows
        constexpr int kFavW      = 44;   // #908: square touch target for favourite star
        constexpr int kTopMargin = 0;    // anchored to top edge; visual centre is at 22pt
        constexpr int kLeftMargin = 4;
        constexpr int kGap       = 0;    // targets are flush — no visual gap needed

        presetPrev_.setBounds(kLeftMargin,
                              kTopMargin,
                              kNavW, kBtnH);

        presetNext_.setBounds(kLeftMargin + kNavW + kGap,
                              kTopMargin,
                              kNavW, kBtnH);

        favButton_.setBounds(kLeftMargin + kNavW * 2 + kGap * 2,
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

    /** Map a DepthZone to its fractional radius (as a fraction of halfMin). */
    static float radiusForZone(EngineOrbit::DepthZone zone) noexcept
    {
        switch (zone)
        {
            case EngineOrbit::DepthZone::Sunlit:   return 0.30f;
            case EngineOrbit::DepthZone::Twilight: return 0.45f;
            case EngineOrbit::DepthZone::Midnight: return 0.60f;
        }
        return 0.30f;
    }

    //==========================================================================
    // State
    //==========================================================================

    ViewState viewState_    = ViewState::Orbital;
    int       selectedSlot_ = -1;
    float     dimAlpha_     = 1.0f;  ///< < 1 when PlaySurface or browser dims the scene

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

    // Overlay components.
    DnaMapBrowser        browser_;
    PlaySurfaceOverlay   playSurfaceOverlay_;

    // Floating header controls.
    juce::TextButton presetPrev_    { "<" };
    juce::TextButton presetNext_    { ">" };
    juce::TextButton favButton_     { juce::String::charToString (0x2605) };  // ★
    juce::TextButton settingsButton_{ juce::String::charToString (0x2699) };  // ⚙
    juce::TextButton keysButton_    { "KEYS" };

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

    // Mirror the private EngineOrbit size constants so layout code can access them.
    // Values must be kept in sync with EngineOrbit.h.
    static constexpr float kOrbitSize_Orbital   = 72.0f;   ///< EngineOrbit::kOrbitalSize
    static constexpr float kOrbitSize_ZoomIn    = 120.0f;  ///< EngineOrbit::kZoomInSize
    static constexpr float kOrbitSize_Minimized = 32.0f;   ///< EngineOrbit::kMinimizedSize

    //==========================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OceanView)
};

} // namespace xoceanus
