// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// ChordSlotStrip.h — Per-engine-slot chord info strip + routing toggle (Wave 5 B3).
//
// One strip instance is created per primary engine slot (0–3).  It renders a compact
// horizontal row (~28 px) showing:
//
//   SLOT label  |  current chord tones (4 note pills)  |  routing toggle  |  CHORD button
//
// ── Routing toggle ──────────────────────────────────────────────────────────────────────
// Three-position toggle cycled by click:
//   CHORD→SEQ   (ChordUpstream)  — chord distributes notes, seq sequences them.
//   SEQ→CHORD   (SeqUpstream)    — seq drives timing; raw MIDI passes through so
//                                  the engine's own arp/step-seq runs, chord shapes
//                                  pitch via the ChordMachine palette/voicing.
//   PARALLEL    (Parallel)       — chord + seq both fire independently; raw MIDI
//                                  merged with chord output in the slot buffer.
//
// ── CHORD button ────────────────────────────────────────────────────────────────────────
// Clicking the CHORD button (or anywhere on the strip body) calls
// onOpenBreakout(slotIndex_) so the parent can slide up ChordBreakoutPanel.
//
// ── APVTS wiring ────────────────────────────────────────────────────────────────────────
// Routes through APVTS parameter "cm_slot_route_N" (N = slot index, 0-based).
// beginChangeGesture / setValueNotifyingHost / endChangeGesture on each click.
//
// Timer at 15 Hz updates the chord-note pills from ChordMachine::getCurrentAssignment().
//
// File is header-only (XOceanus UI convention).
//
// Wave 5 B3 mount APPLIED — ChordBreakoutPanel is mounted in OceanView via
// initChordBreakout(). ChordBreakoutPanel internally owns 4 ChordSlotStrip instances.
// External ChordSlotStrip instances are not separately mounted at this time.

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/ChordMachine.h"
#include "../GalleryColors.h"
#include "../Tokens.h"
#include <functional>
#include <array>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

//==============================================================================
/**
    ChordSlotStrip

    Compact per-slot row showing chord tones + chord/seq routing toggle.
    One instance per primary engine slot (slots 0–3).
*/
class ChordSlotStrip final : public juce::Component,
                             private juce::Timer
{
public:
    static constexpr int kHeight = 28; ///< Preferred height in pixels.

    //==========================================================================
    /**
        Construct a strip for @p slotIndex (0-based, must be 0–3).

        @param apvts       Processor's APVTS — used to read/write cm_slot_route_N.
        @param chordMachine Read-only ref to ChordMachine for note-pill display.
        @param slotIndex   Which engine slot this strip represents (0–3).
    */
    explicit ChordSlotStrip(juce::AudioProcessorValueTreeState& apvts,
                            const ChordMachine&                 chordMachine,
                            int                                 slotIndex)
        : apvts_      (apvts)
        , cm_         (chordMachine)
        , slotIndex_  (juce::jlimit(0, kChordSlots - 1, slotIndex))
    {
        jassert(slotIndex >= 0 && slotIndex < kChordSlots);
        setOpaque(false);
        setInterceptsMouseClicks(true, false);
        syncRoutingFromApvts();
        startTimerHz(15);
    }

    ~ChordSlotStrip() override { stopTimer(); }

    //==========================================================================
    /// Called when the user clicks the CHORD button or strip body.
    /// Argument: slot index (0–3).
    std::function<void(int)> onOpenBreakout;

private:
    //==========================================================================
    // Layout constants (px)
    static constexpr float kPadX       = 6.0f;
    static constexpr float kGap        = 4.0f;
    static constexpr float kPillH      = 16.0f;
    static constexpr float kSlotLblW   = 30.0f;  // "SLT 1" label
    static constexpr float kNotePillW  = 28.0f;  // per-note pill
    static constexpr float kRouteBtnW  = 68.0f;  // routing toggle pill
    static constexpr float kChordBtnW  = 44.0f;  // "CHORD" open button

    enum class HitZone { None, RouteToggle, ChordBtn };

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const float w    = static_cast<float>(getWidth());
        const float h    = static_cast<float>(getHeight());
        const float midY = h * 0.5f;

        // Subtle background.
        g.setColour(juce::Colour(0xFF0E1520));
        g.fillRect(0.0f, 0.0f, w, h);

        // Bottom divider.
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.05f));
        g.fillRect(0.0f, h - 1.0f, w, 1.0f);

        static const juce::Font pillFont  = XO::Tokens::Type::heading(XO::Tokens::Type::BodySmall);  // D3: 8.5→9 (nearest standard)
        static const juce::Font labelFont = XO::Tokens::Type::body(XO::Tokens::Type::BodySmall);    // D3: 8.0→9 (nearest standard)

        float curX = kPadX;
        const float pillY = midY - kPillH * 0.5f;

        // ── Slot label ────────────────────────────────────────────────────────
        {
            const juce::Colour lblCol = juce::Colour(200, 204, 216).withAlpha(0.35f);
            g.setFont(labelFont);
            g.setColour(lblCol);
            g.drawText("SLT " + juce::String(slotIndex_ + 1),
                       juce::Rectangle<float>(curX, pillY, kSlotLblW, kPillH).toNearestInt(),
                       juce::Justification::centredLeft, false);
            curX += kSlotLblW + kGap;
        }

        // ── Note pills (4 chord tones) ────────────────────────────────────────
        {
            // Palette accent colors — mirror of ChordBarComponent::kPaletteColors.
            static constexpr uint32_t kPaletteColors[8] = {
                0xFFFF8A65, 0xFFFFD54F, 0xFFEF5350, 0xFF81D4FA,
                0xFF7E57C2, 0xFFF48FB1, 0xFFFF7043, 0xFFBDBDBD
            };
            const auto assign = cm_.getCurrentAssignment();
            const uint32_t paletteColorRaw = kPaletteColors[
                juce::jlimit(0, 7, static_cast<int>(cm_.getPalette()))];
            const juce::Colour noteCol = juce::Colour(paletteColorRaw);

            for (int i = 0; i < kChordSlots; ++i)
            {
                const int note = assign.midiNotes[i];
                const bool hasNote = (note >= 0 && note <= 127);
                const bool isThisSlot = (i == slotIndex_);

                juce::Colour bgCol  = juce::Colours::transparentBlack;
                juce::Colour txtCol = juce::Colour(200, 204, 216).withAlpha(0.30f);
                juce::Colour bdrCol = juce::Colour(200, 204, 216).withAlpha(0.06f);

                if (hasNote)
                {
                    txtCol = isThisSlot
                        ? noteCol.withAlpha(0.95f)
                        : noteCol.withAlpha(0.55f);
                    bdrCol = noteCol.withAlpha(isThisSlot ? 0.40f : 0.15f);
                    if (isThisSlot)
                        bgCol = noteCol.withAlpha(0.08f);
                }

                const juce::Rectangle<float> pillBounds{curX, pillY, kNotePillW, kPillH};

                if (!bgCol.isTransparent())
                {
                    g.setColour(bgCol);
                    g.fillRoundedRectangle(pillBounds, 3.0f);
                }
                g.setColour(bdrCol);
                g.drawRoundedRectangle(pillBounds, 3.0f, 1.0f);

                g.setFont(pillFont);
                g.setColour(txtCol);
                const juce::String label = hasNote ? ChordMachine::midiNoteToName(note) : "-";
                g.drawText(label, pillBounds.toNearestInt(), juce::Justification::centred, false);

                curX += kNotePillW + kGap;
            }
        }

        // ── Separator ─────────────────────────────────────────────────────────
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.07f));
        g.fillRect(curX, midY - 8.0f, 1.0f, 16.0f);
        curX += 1.0f + kGap;

        // ── Routing toggle pill ───────────────────────────────────────────────
        {
            routeToggleBounds_ = juce::Rectangle<float>(curX, pillY, kRouteBtnW, kPillH);
            const bool hovered = (hoveredZone_ == HitZone::RouteToggle);

            const juce::Colour activeCol = juce::Colour(127, 219, 202);
            juce::Colour txtCol   = hovered ? activeCol.withAlpha(0.90f) : activeCol.withAlpha(0.65f);
            juce::Colour bdrCol   = hovered ? activeCol.withAlpha(0.30f) : activeCol.withAlpha(0.14f);
            juce::Colour bgCol    = hovered ? activeCol.withAlpha(0.07f) : juce::Colours::transparentBlack;

            if (!bgCol.isTransparent())
            {
                g.setColour(bgCol);
                g.fillRoundedRectangle(routeToggleBounds_, 4.0f);
            }
            g.setColour(bdrCol);
            g.drawRoundedRectangle(routeToggleBounds_, 4.0f, 1.0f);

            g.setFont(pillFont);
            g.setColour(txtCol);
            g.drawText(chordSeqRoutingName(currentRouting_),
                       routeToggleBounds_.toNearestInt(),
                       juce::Justification::centred, false);
            curX += kRouteBtnW + kGap;
        }

        // ── CHORD open button ─────────────────────────────────────────────────
        {
            chordBtnBounds_ = juce::Rectangle<float>(curX, pillY, kChordBtnW, kPillH);
            const bool hovered = (hoveredZone_ == HitZone::ChordBtn);
            const juce::Colour gold = XO::Tokens::Color::primary();

            juce::Colour txtCol = hovered ? gold.withAlpha(0.95f) : gold.withAlpha(0.60f);
            juce::Colour bdrCol = hovered ? gold.withAlpha(0.30f) : gold.withAlpha(0.12f);
            juce::Colour bgCol  = hovered ? gold.withAlpha(0.07f) : juce::Colours::transparentBlack;

            if (!bgCol.isTransparent())
            {
                g.setColour(bgCol);
                g.fillRoundedRectangle(chordBtnBounds_, 4.0f);
            }
            g.setColour(bdrCol);
            g.drawRoundedRectangle(chordBtnBounds_, 4.0f, 1.0f);

            g.setFont(pillFont);
            g.setColour(txtCol);
            g.drawText("CHORD", chordBtnBounds_.toNearestInt(), juce::Justification::centred, false);
        }
    }

    //--------------------------------------------------------------------------
    void mouseDown(const juce::MouseEvent& e) override
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        if (routeToggleBounds_.expanded(4.0f).contains(mx, my))
        {
            cycleRoutingMode();
            return;
        }

        if (chordBtnBounds_.expanded(4.0f).contains(mx, my))
        {
            if (onOpenBreakout)
                onOpenBreakout(slotIndex_);
            return;
        }

        // Click anywhere else on the strip also opens the breakout.
        if (onOpenBreakout)
            onOpenBreakout(slotIndex_);
    }

    //--------------------------------------------------------------------------
    void mouseMove(const juce::MouseEvent& e) override
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);
        HitZone newZone = HitZone::None;

        if (routeToggleBounds_.expanded(4.0f).contains(mx, my))
            newZone = HitZone::RouteToggle;
        else if (chordBtnBounds_.expanded(4.0f).contains(mx, my))
            newZone = HitZone::ChordBtn;

        if (newZone != hoveredZone_)
        {
            hoveredZone_ = newZone;
            repaint();
        }
    }

    //--------------------------------------------------------------------------
    void mouseExit(const juce::MouseEvent&) override
    {
        if (hoveredZone_ != HitZone::None)
        {
            hoveredZone_ = HitZone::None;
            repaint();
        }
    }

    //--------------------------------------------------------------------------
    void timerCallback() override
    {
        if (isShowing())
            repaint();
    }

    //==========================================================================
    // ── Routing mode helpers ──

    /// Read routing mode from APVTS and cache locally.
    void syncRoutingFromApvts()
    {
        const juce::String paramId = "cm_slot_route_" + juce::String(slotIndex_);
        if (auto* p = apvts_.getParameter(paramId))
        {
            const int idx = static_cast<int>(p->convertFrom0to1(p->getValue()) + 0.5f);
            currentRouting_ = static_cast<ChordSeqRoutingMode>(
                juce::jlimit(0, static_cast<int>(ChordSeqRoutingMode::NumModes) - 1, idx));
        }
    }

    /// Cycle to the next routing mode and push to APVTS.
    void cycleRoutingMode()
    {
        const int next = (static_cast<int>(currentRouting_) + 1)
                         % static_cast<int>(ChordSeqRoutingMode::NumModes);
        currentRouting_ = static_cast<ChordSeqRoutingMode>(next);

        const juce::String paramId = "cm_slot_route_" + juce::String(slotIndex_);
        if (auto* p = apvts_.getParameter(paramId))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(next)));
            p->endChangeGesture();
        }

        repaint();
    }

    //==========================================================================
    juce::AudioProcessorValueTreeState& apvts_;
    const ChordMachine&                 cm_;
    const int                           slotIndex_;

    ChordSeqRoutingMode                 currentRouting_ = ChordSeqRoutingMode::ChordUpstream;
    HitZone                             hoveredZone_    = HitZone::None;

    // Laid-out regions (rebuilt each paint).
    mutable juce::Rectangle<float>     routeToggleBounds_;
    mutable juce::Rectangle<float>     chordBtnBounds_;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordSlotStrip)
};

} // namespace xoceanus
