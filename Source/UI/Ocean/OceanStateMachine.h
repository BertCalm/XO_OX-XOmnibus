// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// OceanStateMachine.h  —  Phase 3 of the OceanView decomposition (issue #1184).
//
// OceanStateMachine owns the ViewState enum and all view-state transition logic
// previously inlined in OceanView.  It has NO back-reference to OceanView —
// communication flows exclusively via the onStateEntered callback registered at
// OceanView construction.
//
// Design contract (issue #1184 Phase 3 spec):
//   - No OceanView* or OceanView& anywhere in this class.
//   - No back-references to OceanLayout or OceanChildren.
//   - State changes fire onStateEntered, which OceanView wires to trigger a
//     layout pass + repaint.
//   - OceanView's existing public transitionToX() wrapper methods become
//     one-line forwarders to stateMachine_.requestTransition(...).
//   - The ViewState enum defined here replaces OceanLayout::ViewState (step 11).
//
// Note on animation:  The current implementation has NO animated (interpolated)
// state transitions — transitions are instantaneous.  The design doc's
// onAnimationFrame callback is stubbed for future use but never fired.
// OceanView remains a juce::Timer owner for its orbit-animation and
// position-save-debounce logic, which is unrelated to view-state transitions.
//
// Phase 4 cleanup candidates:
//   - selectedSlot_: currently owned here because it is part of the
//     transition contract (ZoomIn/SplitTransform require a slot arg).
//   - preBrowserState_ / preBrowserSlot_: transition history, belongs here.
//   - firstLaunch_ / detailShowing_: these are UI-layer flags, not
//     state-machine state.  They remain on OceanView (see OceanViewContext).

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace xoceanus
{

//==============================================================================
/**
    OceanStateMachine

    Owns the ViewState enum and manages all view-state transitions for
    OceanView.

    Design constraints (issue #1184):
      - No back-reference to OceanView.  Uses only the onStateEntered callback.
      - No back-reference to OceanLayout or OceanChildren.
      - Transitions are currently instantaneous (no animation timer).
        onAnimationFrame is stubbed for future use.

    Usage:
    @code
        // In OceanView constructor:
        stateMachine_.onStateEntered = [this](OceanStateMachine::ViewState s)
        {
            layout_.layoutForState(s, getLocalBounds(), 1.0f);
            layout_.reorderZStack();
            repaint();
        };
    @endcode
*/
class OceanStateMachine
{
public:
    //==========================================================================
    // ViewState — canonical definition (Phase 3 unification, issue #1184)
    //
    // This replaces the duplicate enum in OceanLayout::ViewState and
    // OceanView::ViewState.  Both of those are removed in Phase 3 step 11.
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
    // Construction
    //==========================================================================

    OceanStateMachine() = default;

    // Non-copyable (owns mutable state and holds std::function callbacks).
    OceanStateMachine(const OceanStateMachine&)            = delete;
    OceanStateMachine(OceanStateMachine&&)                 = delete;
    OceanStateMachine& operator=(const OceanStateMachine&) = delete;
    OceanStateMachine& operator=(OceanStateMachine&&)      = delete;

    //==========================================================================
    // Callbacks (wire at OceanView construction)
    //==========================================================================

    /** Called once when a state transition completes.
     *  OceanView wires this to trigger a layout pass + repaint. */
    std::function<void(ViewState)> onStateEntered;

    /** Called each animation tick during a transition [unused — stubs for future
     *  animated transitions.  Currently transitions are instantaneous.] */
    std::function<void(ViewState, float /*progress01*/)> onAnimationFrame;

    //==========================================================================
    // State accessors
    //==========================================================================

    /** Current view state. */
    ViewState currentState() const noexcept { return state_; }

    /** Index of the currently selected engine slot (-1 = none). */
    int selectedSlot() const noexcept { return selectedSlot_; }

    /** State active before the browser was opened (used by exitBrowser). */
    ViewState preBrowserState() const noexcept { return preBrowserState_; }

    /** Slot active before the browser was opened (-1 = none). */
    int preBrowserSlot() const noexcept { return preBrowserSlot_; }

    //==========================================================================
    // State mutators (used by OceanView for legacy state writes not yet
    // routed through requestTransition; will be cleaned up incrementally)
    //==========================================================================

    /** Force-set selectedSlot without triggering a transition. */
    void setSelectedSlot(int slot) noexcept { selectedSlot_ = slot; }

    //==========================================================================
    // Transition API
    //==========================================================================

    /** Transition to Orbital state.
     *  Fires onStateEntered(ViewState::Orbital) after updating internal state.
     *  Returns without firing if the state is already Orbital AND selectedSlot
     *  is already -1 (idempotent). */
    void transitionToOrbital()
    {
        state_        = ViewState::Orbital;
        selectedSlot_ = -1;
        fireStateEntered();
    }

    /** Transition to ZoomIn for the given slot.
     *
     *  Toggle detection (same state + same slot → Orbital) is handled by
     *  OceanView::transitionToZoomIn before this method is called.
     *  This method always transitions unconditionally. */
    void transitionToZoomIn(int slot)
    {
        state_        = ViewState::ZoomIn;
        selectedSlot_ = slot;
        fireStateEntered();
    }

    /** Transition to SplitTransform for the given slot. */
    void transitionToSplitTransform(int slot)
    {
        state_        = ViewState::SplitTransform;
        selectedSlot_ = slot;
        fireStateEntered();
    }

    /** Transition to BrowserOpen.
     *  Saves the current state so exitBrowser() can restore it. */
    void transitionToBrowser()
    {
        preBrowserState_ = state_;
        preBrowserSlot_  = selectedSlot_;
        state_           = ViewState::BrowserOpen;
        fireStateEntered();
    }

    /** Clear the pre-browser snapshot.
     *
     *  Called by OceanView::exitBrowser() before dispatching to a transition
     *  method, to prevent re-entry if that transition calls back into exitBrowser.
     */
    void clearPreBrowserState() noexcept
    {
        preBrowserState_ = ViewState::Orbital;
        preBrowserSlot_  = -1;
    }

private:
    //==========================================================================
    // State
    //==========================================================================

    ViewState state_          = ViewState::Orbital;
    int       selectedSlot_   = -1;

    /// State saved on entering BrowserOpen so exitBrowserState() can restore it.
    ViewState preBrowserState_ = ViewState::Orbital;
    int       preBrowserSlot_  = -1;

    //==========================================================================
    // Helpers
    //==========================================================================

    void fireStateEntered()
    {
        if (onStateEntered)
            onStateEntered(state_);
    }
};

} // namespace xoceanus
