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
        addAndMakeVisible(browser_);
        browser_.setVisible(false);

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

        // 10. PlaySurface overlay (hidden by default; manages its own visibility)
        addAndMakeVisible(playSurfaceOverlay_);

        // 11. Floating header controls
        addAndMakeVisible(presetPrev_);
        addAndMakeVisible(presetNext_);
        addAndMakeVisible(favButton_);
        addAndMakeVisible(settingsButton_);
        addAndMakeVisible(keysButton_);

        // 11b. #1008 FIX 7: DimOverlay sits above all buttons but below
        // PlaySurfaceOverlay.  Added after the buttons so it is painted on top.
        // reorderZStack() will enforce the correct final ordering on each
        // deferred init call.
        addAndMakeVisible(dimOverlay_);
        dimOverlay_.setVisible(false);  // hidden until dimAlpha_ drops below 1

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
        styleHeaderButton(presetPrev_,   44);
        styleHeaderButton(presetNext_,   44);
        styleHeaderButton(favButton_,    44);
        styleHeaderButton(settingsButton_, 44);
        styleHeaderButton(keysButton_,   56);

        // #1007 FIX 1: Tooltip text as fallback label for Unicode icon buttons.
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
        presetNameLabel_.setInterceptsMouseClicks(false, false); // pass clicks through to nexus
        addAndMakeVisible(presetNameLabel_);

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
        nexus_.onDnaClicked        = [this]() { if (onDnaClicked) onDnaClicked(); };

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
        Initialise the StatusBar.
        Must be the last deferred-init call — it sets fullyInitialised_ = true
        and triggers the first valid resized() pass.
    */
    void initStatusBar()
    {
        statusBar_ = std::make_unique<StatusBar>();
        addAndMakeVisible(*statusBar_);
        reorderZStack();

        // #1007 FIX 4: All 4 deferred-init methods have now been called.
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

        // Status bar always spans the full bottom strip.
        if (statusBar_)
            statusBar_->setBounds(0,
                                  getHeight() - kStatusBarH,
                                  getWidth(),
                                  kStatusBarH);

        // PlaySurface overlay always covers the full parent area so it can
        // self-position via repositionFromOffset() inside PlaySurfaceOverlay.
        playSurfaceOverlay_.setBounds(getLocalBounds());

        // #1008 FIX 7: DimOverlay also covers the full area so it can dim
        // everything including the floating header buttons above it.
        dimOverlay_.setBounds(getLocalBounds());
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

    void mouseMove(const juce::MouseEvent& e) override
    {
        // Proximity magnetism: in Orbital state, feed each creature the cursor
        // position (in OceanView coordinates) so it can drift toward it.
        // Other states leave the offset at zero — creatures are not in orbit.
        if (viewState_ == ViewState::Orbital)
        {
            const auto pos = e.getPosition().toFloat();
            for (auto& orbit : orbits_)
                orbit.setMouseProximity(pos);
        }
    }

    void mouseExit(const juce::MouseEvent& /*e*/) override
    {
        // Cursor has left the component entirely — clear all magnet targets so
        // creatures return smoothly to their resting positions.
        for (auto& orbit : orbits_)
            orbit.setMouseProximity({ -1000.0f, -1000.0f });
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

    /** Fired when the user clicks a preset dot in the DNA map browser. */
    std::function<void(int presetIndex)> onPresetSelected;

    /** Fired when the PlaySurface overlay is shown or hidden (including first-launch auto-show).
        Use this to persist the visibility preference so subsequent plugin launches restore the
        last user-chosen state.  true = overlay is now showing, false = hidden. */
    std::function<void(bool visible)> onPlaySurfaceVisibilityChanged;

    /** Fired when the user clicks the DNA hexagon in the nexus.
        Wire this to open the DnaMapBrowser or cycle the axis projection. */
    std::function<void()> onDnaClicked;

    //==========================================================================
    // State queries
    //==========================================================================

    ViewState getViewState()    const noexcept { return viewState_; }
    int       getSelectedSlot() const noexcept { return selectedSlot_; }

private:
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
            // Fix #1005: selected orbit must enter SplitTransform state (not ZoomIn)
            // so EngineOrbit renders the correct layout hint for the 20% mini-strip.
            orbits_[i].setInteractionState(
                i == slot ? EngineOrbit::InteractionState::SplitTransform
                          : EngineOrbit::InteractionState::Minimized);
        }

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

            for (int i = 0; i < 5; ++i)
                orbits_[i].setInteractionState(EngineOrbit::InteractionState::Orbital);

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
        // kNexusH = 200: 96 hex + 8 gap + 22 name + 4 gap + 15 mood + 6 gap
        //              + 41 readouts + 8 margin = 200px total.
        // Raised from 160 so the 96px hex + readout strip fits without clipping.
        constexpr int kNexusW = 160;
        constexpr int kNexusH = 200;
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

                // HIGH fix (#1006): expand bounds by kBreathPadding so the ±5%
                // breath scale oscillation paints inside the component rect.
                // kBreathPadding = ceil(kOrbitalSize * kBreathAmplitude) = ceil(3.6) = 4px.
                const int size = static_cast<int>(kOrbitSize_Orbital) + kBreathPadding * 2;
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

        // Hide panels that belong to other states.
        if (detail_)  { detail_->setVisible(false); }
        if (sidebar_) { sidebar_->setVisible(false); }
        browser_.setVisible(false);
    }

    void layoutZoomIn()
    {
        jassert(selectedSlot_ >= 0 && selectedSlot_ < 5);

        const auto area    = getLocalBounds().withTrimmedBottom(kStatusBarH);
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
        emptyStateLabel_.setVisible(false);  // ZoomIn always has an engine selected
    }

    void layoutSplitTransform()
    {
        jassert(selectedSlot_ >= 0 && selectedSlot_ < 5);

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
        // y starts at 48 (was 40) to clear the 44px header button strip (#1006).
        int  y        = 48;
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
        emptyStateLabel_.setVisible(false);  // SplitTransform always has an engine selected
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
        emptyStateLabel_.setVisible(false);  // browser has its own empty state
    }

    //==========================================================================
    // Floating header controls layout
    //==========================================================================

    void layoutFloatingControls()
    {
        // ── Left cluster: prev | presetName | next | fav ─────────────────────
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

        // #1007 FIX 3: Inline preset name label sits between < and > so the
        // spatial grouping "< Preset Name >" is immediately legible.
        // Width is capped at 160pt so it doesn't crowd the fav button.
        constexpr int kNameLabelW = 160;
        presetNameLabel_.setBounds(kLeftMargin + kNavW,
                                   kTopMargin,
                                   kNameLabelW, kBtnH);

        presetNext_.setBounds(kLeftMargin + kNavW + kNameLabelW + kGap,
                              kTopMargin,
                              kNavW, kBtnH);

        favButton_.setBounds(kLeftMargin + kNavW + kNameLabelW + kNavW + kGap * 2,
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
        if (detail_)   detail_->toFront(false);
        if (sidebar_)  sidebar_->toFront(false);
        browser_.toFront(false);
        presetPrev_.toFront(false);
        presetNext_.toFront(false);
        favButton_.toFront(false);
        settingsButton_.toFront(false);
        keysButton_.toFront(false);
        // #1008 FIX 7: dimOverlay_ above buttons but below PlaySurfaceOverlay.
        dimOverlay_.toFront(false);
        playSurfaceOverlay_.toFront(false);
        if (statusBar_) statusBar_->toFront(false);
    }

    //==========================================================================
    // State
    //==========================================================================

    ViewState viewState_       = ViewState::Orbital;
    int       selectedSlot_    = -1;
    float     dimAlpha_        = 1.0f;  ///< < 1 when PlaySurface or browser dims the scene

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

    // Overlay components.
    DnaMapBrowser        browser_;
    // #1008 FIX 7: DimOverlay must be declared BEFORE PlaySurfaceOverlay so
    // it is constructed first and can be placed below it in the Z-stack.
    DimOverlay           dimOverlay_;
    PlaySurfaceOverlay   playSurfaceOverlay_;

    // Floating header controls.
    juce::TextButton presetPrev_    { "<" };
    juce::TextButton presetNext_    { ">" };
    juce::TextButton favButton_     { juce::String::charToString (0x2605) };  // ★
    juce::TextButton settingsButton_{ juce::String::charToString (0x2699) };  // ⚙
    juce::TextButton keysButton_    { "KEYS" };

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

    // HIGH fix (#1006): padding added to orbital bounds so ±5% breath animation
    // paints inside the component rect.  ceil(72 * 0.05) = 4px each side.
    static constexpr int kBreathPadding = 4;

    // Orbit size aliases: reference EngineOrbit constants directly so there is
    // only one source of truth.  static_asserts below catch any drift.
    static constexpr float kOrbitSize_Orbital   = EngineOrbit::kOrbitalSize;
    static constexpr float kOrbitSize_ZoomIn    = EngineOrbit::kZoomInSize;
    static constexpr float kOrbitSize_Minimized = EngineOrbit::kMinimizedSize;

    // Compile-time guard: these must match EngineOrbit — if they drift the
    // assert fires at build time, not at runtime.
    static_assert(kOrbitSize_Orbital   == EngineOrbit::kOrbitalSize,   "kOrbitSize_Orbital out of sync");
    static_assert(kOrbitSize_ZoomIn    == EngineOrbit::kZoomInSize,    "kOrbitSize_ZoomIn out of sync");
    static_assert(kOrbitSize_Minimized == EngineOrbit::kMinimizedSize, "kOrbitSize_Minimized out of sync");

    //==========================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OceanView)
};

} // namespace xoceanus
