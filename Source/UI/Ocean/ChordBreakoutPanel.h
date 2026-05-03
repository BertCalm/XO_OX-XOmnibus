// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// ChordBreakoutPanel.h — Slide-up chord breakout editor (Wave 5 B3).
//
// Bitwig-style slide-up panel that covers ~60% of the editor height.  Opens when
// the user clicks the "CHORD" button on any ChordSlotStrip, or the CHORD button in
// ChordBarComponent.
//
// ── Layout ─────────────────────────────────────────────────────────────────────────────
//
//   ┌─────────────────────────────────────────────────────────────────────────────────┐
//   │  CHORD EDITOR  [SLT 1 | SLT 2 | SLT 3 | SLT 4]              [×]  Close        │ ← header (32 px)
//   ├─────────────────────────────────────────────────────────────────────────────────┤
//   │  ChordBarComponent (full bar — palette, voicing, spread, root, piano, etc.)     │ ← 28 px
//   ├─────────────────────────────────────────────────────────────────────────────────┤
//   │                                                                                 │
//   │  Per-slot routing column (ChordSlotStrip for each of 4 slots)                  │ ← 4 × 28 px
//   │                                                                                 │
//   ├─────────────────────────────────────────────────────────────────────────────────┤
//   │  Input mode row (active slot):  AUTO-HARMONIZE  |  PAD-PER-CHORD  |  SCALE-DEG │ ← 36 px
//   └─────────────────────────────────────────────────────────────────────────────────┘
//
// ── Slide animation ────────────────────────────────────────────────────────────────────
//
// The panel is a child of the editor and sits at the BOTTOM of the editor area.
// When closed, it is translated fully below the editor bounds (off-screen downward).
// openForSlot(slotIndex) / close() animate it vertically using a juce::Timer at ~60 fps
// with an ease-out curve.  The active slot tab is highlighted.
//
// ── Input mode strip ───────────────────────────────────────────────────────────────────
//
// The bottom row shows three input mode pills for the active slot, wired to the
// APVTS parameter "cm_slot_input_mode_N" (N = active slot, 0-based).  These
// correspond to the three B2 input modes:
//   AUTO-HARMONIZE  — notes harmonized automatically from root + palette/voicing.
//   PAD-PER-CHORD   — each pad triggers a different chord voicing.
//   SCALE-DEGREE    — pads select scale degrees from the current voicing.
//
// The input mode parameter is declared here; its APVTS registration TODO is below.
//
// ── APVTS parameters needed ────────────────────────────────────────────────────────────
//
// Wave 5 B3 mount APPLIED:
//   - cm_slot_input_mode_N params added to XOceanusProcessor.cpp createParameterLayout()
//   - ChordBreakoutPanel mounted in OceanView via initChordBreakout(apvts, chordMachine)
//   - member chordBreakout_ declared in OceanView.h, bounds set in resized()

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/ChordMachine.h"
#include "../GalleryColors.h"
#include "../Tokens.h"
#include "ChordBarComponent.h"
#include "ChordSlotStrip.h"
#include <array>
#include <functional>
#include <memory>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

//==============================================================================
/**
    ChordBreakoutPanel

    Slide-up chord editor covering ~60% of editor height.  Driven by openForSlot()
    and close().  Self-contained animation, no parent callbacks needed.
*/
class ChordBreakoutPanel final : public juce::Component,
                                 private juce::Timer
{
public:
    static constexpr int kHeaderH  = 32;
    static constexpr int kBarH     = ChordBarComponent::kBarHeight;   // 28
    static constexpr int kStripH   = ChordSlotStrip::kHeight;         // 28
    static constexpr int kInputH   = 36;
    static constexpr int kDivH     = 1;

    /// Minimum recommended height = header + bar + 4 strips + input row.
    static constexpr int kMinHeight =
        kHeaderH + kBarH + kDivH + kStripH * 4 + kDivH + kInputH;

    //==========================================================================
    explicit ChordBreakoutPanel(juce::AudioProcessorValueTreeState& apvts,
                                const ChordMachine&                 chordMachine)
        : apvts_      (apvts)
        , cm_         (chordMachine)
    {
        setOpaque(true);

        // ── ChordBarComponent (full chord control bar) ──────────────────────
        chordBar_ = std::make_unique<ChordBarComponent>(apvts_, cm_);
        addAndMakeVisible(chordBar_.get());

        // ── Per-slot ChordSlotStrips ────────────────────────────────────────
        for (int i = 0; i < kChordSlots; ++i)
        {
            slotStrips_[i] = std::make_unique<ChordSlotStrip>(apvts_, cm_, i);
            slotStrips_[i]->onOpenBreakout = [this](int slot) {
                // Switching active slot within the panel — just highlight it.
                setActiveSlot(slot);
            };
            addAndMakeVisible(slotStrips_[i].get());
        }

        // ── Input mode pill strip ───────────────────────────────────────────
        // Constructed inline; laid out in resized().
    }

    ~ChordBreakoutPanel() override
    {
        stopTimer();
    }

    //==========================================================================
    /// Open the panel, animating it up from below, and highlight @p slotIndex.
    void openForSlot(int slotIndex)
    {
        activeSlot_ = juce::jlimit(0, kChordSlots - 1, slotIndex);
        isOpen_ = true;
        setVisible(true);
        startTimerHz(60); // animation
        repaint();
        // F3-017: Notify persistence layer that the panel is now open.
        if (onBreakoutToggled) onBreakoutToggled(true);
    }

    /// Close the panel, animating it back down.
    void close()
    {
        isOpen_ = false;
        startTimerHz(60); // animation
        // F3-017: Notify persistence layer that the panel is now closed.
        if (onBreakoutToggled) onBreakoutToggled(false);
    }

    /** F3-017: Callback fired whenever the panel opens or closes.
        Wire from OceanView so the editor can persist the state. */
    std::function<void(bool isOpen)> onBreakoutToggled;

    bool isOpen() const noexcept { return isOpen_; }

    /** F3-017: Restore open/close state from a saved session.
        Call after the component has been laid out so the animation start
        position is correct.  Uses slot 0 as the default highlight slot
        (session state only tracks whether the panel was open, not which slot). */
    void setIsOpenFromState(bool open)
    {
        if (open && !isOpen_)
            openForSlot(0);   // re-open at slot 0 (safe default on restore)
        else if (!open && isOpen_)
            close();
    }

    //==========================================================================
    void resized() override
    {
        const int w = getWidth();
        int y = 0;

        // Header.
        y += kHeaderH;

        // ChordBarComponent.
        chordBar_->setBounds(0, y, w, kBarH);
        y += kBarH + kDivH;

        // Per-slot strips.
        for (int i = 0; i < kChordSlots; ++i)
        {
            slotStrips_[i]->setBounds(0, y, w, kStripH);
            y += kStripH;
        }
        // y now points to the input row area.
    }

    //--------------------------------------------------------------------------
    void paint(juce::Graphics& g) override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());

        // Panel background.
        g.setColour(juce::Colour(0xFF0B1219));
        g.fillRect(0.0f, 0.0f, w, h);

        // Top drag handle / title bar.
        {
            g.setColour(juce::Colour(0xFF111820));
            g.fillRect(0.0f, 0.0f, w, static_cast<float>(kHeaderH));

            // Drag handle visual (3-dot indicator).
            const float dotY = kHeaderH * 0.5f;
            const juce::Colour dotCol = juce::Colour(200, 204, 216).withAlpha(0.25f);
            for (int d = -1; d <= 1; ++d)
            {
                g.setColour(dotCol);
                g.fillEllipse(w * 0.5f + d * 6.0f - 2.0f, dotY - 2.0f, 4.0f, 4.0f);
            }

            // Title.
            static const juce::Font titleFont = XO::Tokens::Type::heading(XO::Tokens::Type::BodyDefault); // D3
            g.setFont(titleFont);
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.45f));
            g.drawText("CHORD EDITOR", 12, 0, 120, kHeaderH,
                       juce::Justification::centredLeft, false);

            // Slot tabs (SLT 1–4).
            paintSlotTabs(g);

            // Close button.
            paintCloseButton(g);

            // Bottom border for header.
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.07f));
            g.fillRect(0.0f, static_cast<float>(kHeaderH - 1), w, 1.0f);
        }

        // Divider between ChordBar and slot strips.
        {
            const float divY = static_cast<float>(kHeaderH + kBarH);
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.05f));
            g.fillRect(0.0f, divY, w, 1.0f);
        }

        // Divider + input mode row (below the last strip).
        {
            const int stripBottom = kHeaderH + kBarH + kDivH + kStripH * kChordSlots;
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.05f));
            g.fillRect(0.0f, static_cast<float>(stripBottom), w, 1.0f);

            paintInputModeRow(g, stripBottom + 1);
        }
    }

private:
    //==========================================================================
    // ── Slot tabs ─────────────────────────────────────────────────────────────

    void paintSlotTabs(juce::Graphics& g) const
    {
        static const juce::Font tabFont = XO::Tokens::Type::heading(XO::Tokens::Type::BodySmall); // D3: 8.5→9
        g.setFont(tabFont);

        const float startX = 140.0f;
        const float tabW   = 36.0f;
        const float tabH   = 18.0f;
        const float tabY   = (kHeaderH - tabH) * 0.5f;
        const float gap    = 4.0f;

        for (int i = 0; i < kChordSlots; ++i)
        {
            const juce::Rectangle<float> tb{startX + i * (tabW + gap), tabY, tabW, tabH};
            const bool isActive = (i == activeSlot_);
            const juce::Colour teal = juce::Colour(127, 219, 202);

            if (isActive)
            {
                g.setColour(teal.withAlpha(0.10f));
                g.fillRoundedRectangle(tb, 3.0f);
                g.setColour(teal.withAlpha(0.30f));
                g.drawRoundedRectangle(tb, 3.0f, 1.0f);
                g.setColour(teal.withAlpha(0.90f));
            }
            else
            {
                g.setColour(juce::Colour(200, 204, 216).withAlpha(0.07f));
                g.drawRoundedRectangle(tb, 3.0f, 1.0f);
                g.setColour(juce::Colour(200, 204, 216).withAlpha(0.40f));
            }
            g.drawText("SLT " + juce::String(i + 1), tb.toNearestInt(),
                       juce::Justification::centred, false);
        }
    }

    //==========================================================================
    // ── Close button ──────────────────────────────────────────────────────────

    juce::Rectangle<float> closeButtonBounds() const noexcept
    {
        return {static_cast<float>(getWidth()) - 36.0f, 7.0f, 26.0f, 18.0f};
    }

    void paintCloseButton(juce::Graphics& g) const
    {
        const auto cb = closeButtonBounds();
        const bool hov = (hoveredCloseBtn_);
        const juce::Colour col = hov
            ? juce::Colour(200, 204, 216).withAlpha(0.80f)
            : juce::Colour(200, 204, 216).withAlpha(0.35f);

        static const juce::Font btnFont = XO::Tokens::Type::heading(XO::Tokens::Type::BodySmall); // D3
        g.setFont(btnFont);
        g.setColour(col);
        g.drawText("\xc3\x97", cb.toNearestInt(), juce::Justification::centred, false); // UTF-8 ×
    }

    //==========================================================================
    // ── Input mode row ────────────────────────────────────────────────────────

    void paintInputModeRow(juce::Graphics& g, int rowY) const
    {
        static const juce::Font pillFont = XO::Tokens::Type::heading(XO::Tokens::Type::BodySmall); // D3: 8.5→9
        g.setFont(pillFont);

        static constexpr const char* kInputModeLabels[3] = {
            "AUTO-HARMONIZE", "PAD-PER-CHORD", "SCALE-DEGREE"
        };

        const float rowH  = static_cast<float>(kInputH);
        const float pillH = 16.0f;
        const float pillY = rowY + (rowH - pillH) * 0.5f;
        const float pillW = 90.0f;
        const float gap   = 6.0f;
        float curX = 12.0f;

        for (int m = 0; m < 3; ++m)
        {
            const juce::Rectangle<float> pb{curX, pillY, pillW, pillH};
            const bool isActive = (m == currentInputMode_);
            const bool hov = (hoveredInputMode_ == m);
            const juce::Colour teal = juce::Colour(127, 219, 202);

            juce::Colour txtCol, bdrCol, bgCol;
            if (isActive)
            {
                txtCol = teal.withAlpha(0.90f);
                bdrCol = teal.withAlpha(0.28f);
                bgCol  = teal.withAlpha(0.07f);
            }
            else if (hov)
            {
                txtCol = juce::Colour(200, 204, 216).withAlpha(0.80f);
                bdrCol = juce::Colour(200, 204, 216).withAlpha(0.18f);
                bgCol  = juce::Colours::transparentBlack;
            }
            else
            {
                txtCol = juce::Colour(200, 204, 216).withAlpha(0.45f);
                bdrCol = juce::Colour(200, 204, 216).withAlpha(0.08f);
                bgCol  = juce::Colours::transparentBlack;
            }

            if (!bgCol.isTransparent())
            {
                g.setColour(bgCol);
                g.fillRoundedRectangle(pb, 4.0f);
            }
            g.setColour(bdrCol);
            g.drawRoundedRectangle(pb, 4.0f, 1.0f);
            g.setColour(txtCol);
            g.drawText(kInputModeLabels[m], pb.toNearestInt(), juce::Justification::centred, false);

            curX += pillW + gap;
        }
    }

    //==========================================================================
    // ── Mouse events ──────────────────────────────────────────────────────────

    void mouseDown(const juce::MouseEvent& e) override
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        // Close button.
        if (closeButtonBounds().expanded(4.0f).contains(mx, my))
        {
            close();
            return;
        }

        // Slot tab clicks (inside header).
        if (my >= 0.0f && my <= static_cast<float>(kHeaderH))
        {
            const float startX = 140.0f;
            const float tabW   = 36.0f;
            const float gap    = 4.0f;
            for (int i = 0; i < kChordSlots; ++i)
            {
                const juce::Rectangle<float> tb{startX + i * (tabW + gap), 7.0f, tabW, 18.0f};
                if (tb.expanded(3.0f).contains(mx, my))
                {
                    setActiveSlot(i);
                    return;
                }
            }
        }

        // Input mode pills.
        const int stripBottom = kHeaderH + kBarH + kDivH + kStripH * kChordSlots;
        if (my >= static_cast<float>(stripBottom))
        {
            const float pillH = 16.0f;
            const float pillW = 90.0f;
            const float gap   = 6.0f;
            const float pillY = stripBottom + 1 + (kInputH - pillH) * 0.5f;
            float curX = 12.0f;
            for (int m = 0; m < 3; ++m)
            {
                const juce::Rectangle<float> pb{curX, pillY, pillW, pillH};
                if (pb.expanded(4.0f).contains(mx, my))
                {
                    setInputMode(m);
                    return;
                }
                curX += pillW + gap;
            }
        }
    }

    //--------------------------------------------------------------------------
    void mouseMove(const juce::MouseEvent& e) override
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        const bool nowHoverClose = closeButtonBounds().expanded(4.0f).contains(mx, my);
        if (nowHoverClose != hoveredCloseBtn_)
        {
            hoveredCloseBtn_ = nowHoverClose;
            repaint();
        }

        // Input mode hover.
        const int stripBottom = kHeaderH + kBarH + kDivH + kStripH * kChordSlots;
        int newInputHov = -1;
        if (my >= static_cast<float>(stripBottom))
        {
            const float pillH = 16.0f;
            const float pillW = 90.0f;
            const float gap   = 6.0f;
            const float pillY = stripBottom + 1 + (kInputH - pillH) * 0.5f;
            float curX = 12.0f;
            for (int m = 0; m < 3; ++m)
            {
                const juce::Rectangle<float> pb{curX, pillY, pillW, pillH};
                if (pb.expanded(4.0f).contains(mx, my)) { newInputHov = m; break; }
                curX += pillW + gap;
            }
        }
        if (newInputHov != hoveredInputMode_)
        {
            hoveredInputMode_ = newInputHov;
            repaint();
        }
    }

    //--------------------------------------------------------------------------
    void mouseExit(const juce::MouseEvent&) override
    {
        bool changed = false;
        if (hoveredCloseBtn_)  { hoveredCloseBtn_ = false; changed = true; }
        if (hoveredInputMode_ != -1) { hoveredInputMode_ = -1; changed = true; }
        if (changed) repaint();
    }

    //==========================================================================
    // ── Slot / input mode state ───────────────────────────────────────────────

    void setActiveSlot(int slot)
    {
        activeSlot_ = juce::jlimit(0, kChordSlots - 1, slot);
        syncInputModeFromApvts();
        repaint();
    }

    void setInputMode(int mode)
    {
        currentInputMode_ = juce::jlimit(0, 2, mode);

        const juce::String paramId = "cm_slot_input_mode_" + juce::String(activeSlot_);
        if (auto* p = apvts_.getParameter(paramId))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(currentInputMode_)));
            p->endChangeGesture();
        }
        repaint();
    }

    void syncInputModeFromApvts()
    {
        const juce::String paramId = "cm_slot_input_mode_" + juce::String(activeSlot_);
        if (auto* p = apvts_.getParameter(paramId))
        {
            currentInputMode_ = juce::jlimit(0, 2,
                static_cast<int>(p->convertFrom0to1(p->getValue()) + 0.5f));
        }
        else
        {
            currentInputMode_ = 0; // fallback: AUTO-HARMONIZE if param not yet registered
        }
    }

    //==========================================================================
    // ── Slide animation ───────────────────────────────────────────────────────
    //
    // The panel translates between two Y positions:
    //   closedY_  = parent.getHeight()            (fully off-screen below)
    //   openY_    = parent.getHeight() - getHeight()  (fully visible)
    //
    // currentYFrac_ tracks the normalised position: 0.0 = closed, 1.0 = open.
    // Each timer tick advances it by kAnimStep toward the target.

    static constexpr float kAnimStep = 0.12f; // ease-out step per 60Hz tick

    void timerCallback() override
    {
        const float target = isOpen_ ? 1.0f : 0.0f;
        const float delta  = target - currentYFrac_;

        if (std::abs(delta) < 0.001f)
        {
            currentYFrac_ = target;
            stopTimer();

            if (!isOpen_)
                setVisible(false); // fully closed — hide so it doesn't steal events
        }
        else
        {
            // Ease-out: large steps when far, small steps near target.
            currentYFrac_ += delta * kAnimStep * 8.0f;
            currentYFrac_  = juce::jlimit(0.0f, 1.0f, currentYFrac_);
        }

        // Update Y position relative to parent.
        if (auto* parent = getParentComponent())
        {
            const int parentH = parent->getHeight();
            const int panelH  = getHeight();
            const int openY   = parentH - panelH;
            const int closedY = parentH;
            const int newY    = static_cast<int>(closedY + (openY - closedY) * currentYFrac_);
            setTopLeftPosition(getX(), newY);
        }
    }

    //==========================================================================
    juce::AudioProcessorValueTreeState& apvts_;
    const ChordMachine&                 cm_;

    std::unique_ptr<ChordBarComponent>                  chordBar_;
    std::array<std::unique_ptr<ChordSlotStrip>, kChordSlots> slotStrips_;

    int   activeSlot_        = 0;
    int   currentInputMode_  = 0;   // 0=AutoHarmonize, 1=PadPerChord, 2=ScaleDegree
    bool  isOpen_            = false;
    float currentYFrac_      = 0.0f; // 0=closed, 1=open

    bool  hoveredCloseBtn_   = false;
    int   hoveredInputMode_  = -1;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordBreakoutPanel)
};

} // namespace xoceanus
