// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// OceanChildren.h  —  Phase 1 of the OceanView decomposition (issue #1184).
//
// OceanChildren owns and initialises all deferred-init unique_ptr child
// components that previously lived directly in OceanView.  It holds a
// reference to the parent Component (OceanView) used ONLY for
// addAndMakeVisible / addChildComponent calls — it never calls back into
// OceanView, never holds a typed OceanView* reference, and never fires
// callbacks of its own.  All post-init work (reorderZStack, resized, callback
// wiring) remains the responsibility of the OceanView wrapper methods.
//
// Construction order
// ──────────────────
//   1. OceanView constructs OceanChildren (via member init `children_{*this}`).
//   2. Editor calls children_.initMacros(apvts), initDetailPanel(proc), etc.
//   3. After each init call, OceanView's public wrapper calls reorderZStack()
//      and resized() as needed — OceanChildren never touches those.
//
// Phase 2 will extract OceanLayout; Phase 3 will extract OceanStateMachine.
// At that point the `parent_` reference for addChildComponent can potentially
// move to OceanLayout, but for now it lives here to keep Phase 1 mechanical.

#include <juce_gui_basics/juce_gui_basics.h>

// Child headers — these transitively pull in XOceanusProcessor, ChordMachine,
// MasterFXSequencer, and all other dependency types needed by the init methods.
#include "TideWaterline.h"
#include "ChordBarComponent.h"
#include "ChordBreakoutPanel.h"
#include "SeqBreakoutComponent.h"
#include "SeqStripComponent.h"
#include "MasterFXStripCompact.h"
#include "EpicSlotsPanel.h"
#include "TransportBar.h"
#include "DotMatrixDisplay.h"
#include "../Gallery/MacroSection.h"
#include "../Gallery/EngineDetailPanel.h"
#include "../Gallery/SidebarPanel.h"
#include "../Gallery/StatusBar.h"

#include <memory>

// Forward declarations for init-method parameters.
// These are fully defined by the child headers above, but explicit fwd-decls
// document the API boundary clearly.
namespace juce { class AudioProcessorValueTreeState; }
class XOceanusProcessor;
class MasterFXSequencer;
class ChordMachine;

namespace xoceanus
{

/**
    OceanChildren

    Owns and deferred-initialises all unique_ptr child components that
    previously lived as private members of OceanView.

    Rules (enforced by code structure, not just comments):
      - No back-reference to OceanView (no OceanView* member).
      - parent_ is used ONLY with addAndMakeVisible / addChildComponent.
      - Callbacks (onBackClicked, onHeightChanged, etc.) are wired by the
        OceanView wrapper after calling the corresponding init method here.
      - reorderZStack() / resized() are never called from inside this class.
*/
class OceanChildren
{
public:
    //==========================================================================
    // Construction
    //==========================================================================

    /**
        @param parent  The OceanView component.  Used exclusively for
                       addAndMakeVisible / addChildComponent.  Never stored as
                       a typed OceanView reference.
    */
    explicit OceanChildren(juce::Component& parent) noexcept
        : parent_(parent)
    {}

    // Non-copyable, non-movable (children hold references to parent).
    OceanChildren(const OceanChildren&)            = delete;
    OceanChildren(OceanChildren&&)                 = delete;
    OceanChildren& operator=(const OceanChildren&) = delete;
    OceanChildren& operator=(OceanChildren&&)      = delete;

    //==========================================================================
    // Deferred-init methods
    //
    // Each method:
    //  1. Constructs the unique_ptr child.
    //  2. Calls addAndMakeVisible / addChildComponent on parent_.
    //  3. Sets any visibility defaults that belong to the child's own state.
    //
    // Post-init duties (reorderZStack, resized, callback wiring) stay in the
    // OceanView wrapper that calls each initX() method.
    //==========================================================================

    /** Wire macro knobs to the AudioProcessorValueTreeState. */
    void initMacros(juce::AudioProcessorValueTreeState& apvts)
    {
        macros_ = std::make_unique<MacroSection>(apvts);
        parent_.addAndMakeVisible(*macros_);
    }

    /**
        Wire the EngineDetailPanel to the processor.

        Note: detail_->onBackClicked is NOT wired here.  OceanView's wrapper
        sets it up after this returns (callback references OceanView state).
    */
    void initDetailPanel(XOceanusProcessor& proc)
    {
        detail_ = std::make_unique<EngineDetailPanel>(proc);
        parent_.addChildComponent(*detail_);  // hidden until double-click
    }

    /** Initialise the SidebarPanel. */
    void initSidebar()
    {
        sidebar_ = std::make_unique<SidebarPanel>();
        parent_.addAndMakeVisible(*sidebar_);
        sidebar_->setVisible(false);
    }

    /**
        Initialise the TideWaterline.

        Note: waterline_->onHeightChanged is NOT wired here.  OceanView's
        wrapper wires the callback (it calls resized() on OceanView).
    */
    void initWaterline(juce::AudioProcessorValueTreeState& apvts,
                       const MasterFXSequencer& sequencer)
    {
        waterline_ = std::make_unique<TideWaterline>(apvts, sequencer);
        parent_.addAndMakeVisible(*waterline_);
    }

    /** Initialise the ChordBarComponent. */
    void initChordBar(juce::AudioProcessorValueTreeState& apvts,
                      const ChordMachine& chordMachine)
    {
        chordBar_ = std::make_unique<ChordBarComponent>(apvts, chordMachine);
        chordBar_->setVisible(false);  // starts hidden, toggled by CHORD button
        parent_.addAndMakeVisible(*chordBar_);
    }

    /**
        Initialise the ChordBreakoutPanel (Wave 5 B3 mount).
        Must be called after initChordBar().
    */
    void initChordBreakout(juce::AudioProcessorValueTreeState& apvts,
                           const ChordMachine& chordMachine)
    {
        chordBreakout_ = std::make_unique<ChordBreakoutPanel>(apvts, chordMachine);
        parent_.addAndMakeVisible(*chordBreakout_);
        chordBreakout_->setVisible(false);  // hidden until opened via ChordSlotStrip
    }

    /**
        Initialise the SeqStrip + SeqBreakout (Wave 5 C2 mount).
        Must be called after the processor is available.
    */
    void initSeqStrip(juce::AudioProcessorValueTreeState& apvts)
    {
        seqBreakout_ = std::make_unique<SeqBreakoutComponent>(apvts);
        seqStrip_    = std::make_unique<SeqStripComponent>(apvts);
        parent_.addAndMakeVisible(*seqBreakout_);
        parent_.addAndMakeVisible(*seqStrip_);
        seqStrip_->setBreakout(seqBreakout_.get());
        seqBreakout_->setVisible(false);  // hidden until strip click
    }

    /** Initialise the compact Master FX strip. */
    void initMasterFxStrip(juce::AudioProcessorValueTreeState& apvts)
    {
        masterFxStrip_ = std::make_unique<MasterFXStripCompact>(apvts);
        parent_.addAndMakeVisible(*masterFxStrip_);
    }

    /** Initialise the Epic Slots panel. */
    void initEpicSlotsPanel(juce::AudioProcessorValueTreeState& apvts)
    {
        epicSlots_ = std::make_unique<EpicSlotsPanel>(apvts);
        parent_.addAndMakeVisible(*epicSlots_);
    }

    /** Initialise the TransportBar. */
    void initTransportBar()
    {
        transportBar_ = std::make_unique<TransportBar>();
        parent_.addAndMakeVisible(*transportBar_);
    }

    /**
        Initialise the StatusBar.

        This is the last deferred-init call.  OceanView's wrapper sets
        fullyInitialised_ = true and calls resized() after this returns.
    */
    void initStatusBar()
    {
        statusBar_ = std::make_unique<StatusBar>();
        parent_.addAndMakeVisible(*statusBar_);
    }

    //==========================================================================
    // Child accessors
    //
    // Raw pointer returns: callers must check for nullptr (component may not
    // have been initialised yet).  All child lifetimes are tied to this object.
    //==========================================================================

    MacroSection*          macros()         const noexcept { return macros_.get(); }
    EngineDetailPanel*     detailPanel()    const noexcept { return detail_.get(); }
    SidebarPanel*          sidebar()        const noexcept { return sidebar_.get(); }
    StatusBar*             statusBar()      const noexcept { return statusBar_.get(); }
    TideWaterline*         waterline()      const noexcept { return waterline_.get(); }
    ChordBarComponent*     chordBar()       const noexcept { return chordBar_.get(); }
    ChordBreakoutPanel*    chordBreakout()  const noexcept { return chordBreakout_.get(); }
    SeqStripComponent*     seqStrip()       const noexcept { return seqStrip_.get(); }
    SeqBreakoutComponent*  seqBreakout()    const noexcept { return seqBreakout_.get(); }
    MasterFXStripCompact*  masterFxStrip()  const noexcept { return masterFxStrip_.get(); }
    EpicSlotsPanel*        epicSlots()      const noexcept { return epicSlots_.get(); }
    TransportBar*          transportBar()   const noexcept { return transportBar_.get(); }

private:
    //==========================================================================
    // Parent reference — ONLY for addAndMakeVisible / addChildComponent
    //==========================================================================

    juce::Component& parent_;  ///< OceanView — NEVER read for state; write-only addChild*

    //==========================================================================
    // Owned children (deferred-init, unique_ptr)
    //==========================================================================

    std::unique_ptr<MacroSection>          macros_;
    std::unique_ptr<EngineDetailPanel>     detail_;
    std::unique_ptr<SidebarPanel>          sidebar_;
    std::unique_ptr<StatusBar>             statusBar_;

    std::unique_ptr<TideWaterline>         waterline_;
    std::unique_ptr<ChordBarComponent>     chordBar_;
    std::unique_ptr<ChordBreakoutPanel>    chordBreakout_;
    std::unique_ptr<SeqStripComponent>     seqStrip_;
    std::unique_ptr<SeqBreakoutComponent>  seqBreakout_;
    std::unique_ptr<MasterFXStripCompact>  masterFxStrip_;
    std::unique_ptr<EpicSlotsPanel>        epicSlots_;
    std::unique_ptr<TransportBar>          transportBar_;

};

} // namespace xoceanus
