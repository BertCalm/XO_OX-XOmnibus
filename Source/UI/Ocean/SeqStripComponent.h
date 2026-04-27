// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// SeqStripComponent.h — Wave 5 C2: always-visible sequencer strip.
//
// A thin 24 px horizontal strip that lives permanently in the dashboard below the
// ChordBar (or wherever the wiring PR mounts it).  It shows:
//
//   [SEQ pill]  pattern-family dot  pattern name  | step LEDs (16 cells)  |  transport badge
//
// Clicking anywhere on the strip opens or closes the SeqBreakoutComponent.
// The strip does NOT own the breakout — the parent mounts both and calls
// setBreakout() so the strip can toggle visibility.
//
// APVTS parameter prefix: slot0_seq_  (Onset pilot, Wave 5 C2)
//   slot0_seq_enabled   (bool)
//   slot0_seq_pattern   (choice 0..23)
//   slot0_seq_stepCount (int  1..16)
//   slot0_seq_clockDiv  (choice index 0..3 → 1/4/1/8/1/16/1/32)
//   slot0_seq_humanize  (float 0..1)
//   slot0_seq_baseVel   (float 0..1)
//   slot0_seq_rootNote  (int 0..127)
//
// Timer at 15 Hz keeps the step-LED playhead current.
//
// TODO Wave5-C2 mount (in OceanView.h initChordBar or resized):
//   addAndMakeVisible(seqStrip_);
//   addAndMakeVisible(seqBreakout_);
//   seqStrip_.setBreakout(&seqBreakout_);
//   // in resized(), after the chord bar:
//   if (seqStrip_.isVisible())
//       seqStrip_.setBounds(dashArea.removeFromTop(SeqStripComponent::kStripHeight));
//   // overlay the breakout over the bottom 60% of the editor:
//   seqBreakout_.setBounds(fullBounds.withTop(fullBounds.getHeight() * 2 / 5));

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include <functional>
#include <array>
#include <atomic>

// SeqBreakoutComponent is a full include (not forward-declared) so toggleBreakout()
// can call setVisible() on it in the same translation unit.
// Include order: SeqStripComponent.h MUST be included AFTER SeqBreakoutComponent.h,
// OR you can include SeqBreakoutComponent.h first and then SeqStripComponent.h.
// The canonical include order is: SeqBreakoutComponent.h → SeqStripComponent.h.
// If included standalone, SeqBreakoutComponent.h is pulled in automatically here:
#include "SeqBreakoutComponent.h"

namespace xoceanus
{

//==============================================================================
/**
    SeqStripComponent

    Always-visible 24 px sequencer strip for the Onset pilot sequencer (slot 0).
    Clicking toggles the SeqBreakoutComponent slide-up panel.
*/
class SeqStripComponent : public juce::Component,
                          private juce::Timer
{
public:
    static constexpr int kStripHeight = 24;

    //==========================================================================
    // Pattern family colour accents (one per family)
    // Order: CRESTS, WAVES, REEFS, GROOVES, DRIFTS, STORMS
    static constexpr uint32_t kFamilyColors[6] = {
        0xFF48CAE4,  // CRESTS — cyan
        0xFF7B8CDE,  // WAVES  — periwinkle
        0xFF56CFB2,  // REEFS  — seafoam
        0xFFE9C46A,  // GROOVES — XO Gold
        0xFF9F7AEA,  // DRIFTS — violet
        0xFFEF6351   // STORMS — coral
    };

    // Canonical name table — must mirror PerEnginePatternSequencer::Pattern order exactly.
    static constexpr const char* kPatternNames[24] = {
        "Pulse","Surge","Ebb","Arc",                 // CRESTS 0-3
        "Sine","Square","Saw","Half",                 // WAVES  4-7
        "Eucl3","Eucl5","Eucl7","Eucl9",              // REEFS  8-11
        "Tresillo","Clave","Backbeat","Boombap",       // GROOVES12-15
        "Drift","Sparkle","Foam","Riptide",            // DRIFTS 16-19
        "Fibonacci","Prime","Golden","Eddy"            // STORMS 20-23
    };

    static constexpr const char* kFamilyNames[6] = {
        "CRESTS","WAVES","REEFS","GROOVES","DRIFTS","STORMS"
    };

    //==========================================================================
    explicit SeqStripComponent(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "slot0_seq_")
        : apvts_  (apvts)
        , prefix_ (slotPrefix)
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, true);
        startTimerHz(15);
    }

    ~SeqStripComponent() override
    {
        stopTimer();
    }

    //==========================================================================
    /** Wire the breakout panel so clicking the strip toggles it. */
    void setBreakout(SeqBreakoutComponent* breakout) noexcept
    {
        breakout_ = breakout;
    }

    /** Call from the audio thread notification path (e.g. timer-based mirror) to
        update the displayed step position for the playhead LED animation. */
    void setCurrentStep(int step) noexcept
    {
        currentStep_.store(step, std::memory_order_relaxed);
    }

    /** Set whether the host transport is playing (for the badge colour). */
    void setIsPlaying(bool playing) noexcept
    {
        isPlaying_.store(playing, std::memory_order_relaxed);
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());

        // ── Background ──
        g.setColour(juce::Colour(0xFF0E111A));
        g.fillRect(0.0f, 0.0f, w, h);

        // Top border
        g.setColour(juce::Colour(0xFF48CAE4).withAlpha(0.12f));
        g.fillRect(0.0f, 0.0f, w, 1.0f);

        // Bottom border
        g.setColour(juce::Colour(0xFF48CAE4).withAlpha(0.06f));
        g.fillRect(0.0f, h - 1.0f, w, 1.0f);

        const float midY = h * 0.5f;

        // ── Read current param state ──
        const int  patternIdx = readParamInt("pattern", 0);
        const int  stepCount  = readParamInt("stepCount", 16);
        const bool enabled    = readParamBool("enabled");
        const int  curStep    = currentStep_.load(std::memory_order_relaxed);
        const bool playing    = isPlaying_.load(std::memory_order_relaxed);
        const bool open       = breakoutOpen_;

        const int familyIdx = juce::jlimit(0, 5, patternIdx / 4);
        const juce::Colour familyCol = juce::Colour(kFamilyColors[familyIdx]);

        static const juce::Font pillFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(9.0f));

        static const juce::Font labelFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withHeight(8.5f));

        float x = 6.0f;

        // ── SEQ pill ──
        {
            const juce::String seqLabel = "SEQ";
            const float pillW = 28.0f;
            const float pillH = 14.0f;
            juce::Rectangle<float> pill(x, midY - pillH * 0.5f, pillW, pillH);

            juce::Colour pillBg   = open  ? familyCol.withAlpha(0.20f) : juce::Colours::transparentBlack;
            juce::Colour pillBorder = open ? familyCol.withAlpha(0.45f) : familyCol.withAlpha(0.25f);
            juce::Colour pillText  = open ? familyCol : familyCol.withAlpha(0.65f);

            if (!pillBg.isTransparent())
            {
                g.setColour(pillBg);
                g.fillRoundedRectangle(pill, 3.0f);
            }
            g.setColour(pillBorder);
            g.drawRoundedRectangle(pill, 3.0f, 1.0f);
            g.setFont(pillFont);
            g.setColour(pillText);
            g.drawText(seqLabel, pill.toNearestInt(), juce::Justification::centred, false);

            x += pillW + 6.0f;
        }

        // ── Family dot ──
        {
            const float dotR = 3.5f;
            g.setColour(enabled ? familyCol : familyCol.withAlpha(0.35f));
            g.fillEllipse(x, midY - dotR, dotR * 2.0f, dotR * 2.0f);
            x += dotR * 2.0f + 5.0f;
        }

        // ── Pattern name ──
        {
            const juce::String nameStr = juce::String(kPatternNames[juce::jlimit(0, 23, patternIdx)]).toUpperCase();
            const float nameW = 54.0f;
            juce::Rectangle<float> nameRect(x, midY - 6.0f, nameW, 12.0f);
            g.setFont(labelFont);
            g.setColour(enabled ? juce::Colour(0xFFE8E4DF) : juce::Colour(0xFF5E6878));
            g.drawText(nameStr, nameRect.toNearestInt(), juce::Justification::centredLeft, true);
            x += nameW + 8.0f;
        }

        // ── Separator ──
        g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.10f));
        g.fillRect(x, midY - 7.0f, 1.0f, 14.0f);
        x += 7.0f;

        // ── Step LEDs ──
        {
            const float ledW   = 6.0f;
            const float ledH   = 5.0f;
            const float gap    = 2.0f;
            const float ledsTotal = 16.0f * (ledW + gap) - gap;

            for (int i = 0; i < 16; ++i)
            {
                const float lx = x + static_cast<float>(i) * (ledW + gap);
                const float ly = midY - ledH * 0.5f;

                const bool inRange  = (i < stepCount);
                const bool isCursor = (playing && enabled && i == curStep && inRange);

                juce::Colour ledCol;
                if (isCursor)
                    ledCol = familyCol;
                else if (inRange)
                    ledCol = enabled ? familyCol.withAlpha(0.22f) : juce::Colour(0xFF3A3938);
                else
                    ledCol = juce::Colour(0xFF1A1A1C);

                g.setColour(ledCol);
                g.fillRoundedRectangle(lx, ly, ledW, ledH, 1.5f);
            }

            x += ledsTotal + 8.0f;
        }

        // ── Separator ──
        g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.10f));
        g.fillRect(x, midY - 7.0f, 1.0f, 14.0f);
        x += 7.0f;

        // ── Transport badge ──
        {
            const float badgeW = 36.0f;
            const float badgeH = 13.0f;
            juce::Rectangle<float> badge(x, midY - badgeH * 0.5f, badgeW, badgeH);

            juce::Colour badgeCol;
            if (!enabled)
                badgeCol = juce::Colour(0xFF3A3938);
            else if (playing)
                badgeCol = juce::Colour(0xFF4CAF50).withAlpha(0.80f); // green = playing
            else
                badgeCol = juce::Colour(0xFFE9C46A).withAlpha(0.55f); // gold = ready/stopped

            g.setColour(badgeCol.withAlpha(0.14f));
            g.fillRoundedRectangle(badge, 3.0f);
            g.setColour(badgeCol);
            g.drawRoundedRectangle(badge, 3.0f, 1.0f);

            g.setFont(pillFont);
            g.setColour(badgeCol);
            const juce::String badgeText = !enabled ? "OFF" : playing ? "RUN" : "RDY";
            g.drawText(badgeText, badge.toNearestInt(), juce::Justification::centred, false);
        }

        // ── Expand chevron at right edge ──
        {
            const float cx = w - 14.0f;
            const float cy = midY;
            const float chevSize = 4.0f;
            juce::Path chevron;
            if (!open)
            {
                // ∧ (open = up arrow since breakout slides up from bottom)
                chevron.startNewSubPath(cx - chevSize, cy + chevSize * 0.5f);
                chevron.lineTo(cx, cy - chevSize * 0.5f);
                chevron.lineTo(cx + chevSize, cy + chevSize * 0.5f);
            }
            else
            {
                // ∨ (close = down arrow)
                chevron.startNewSubPath(cx - chevSize, cy - chevSize * 0.5f);
                chevron.lineTo(cx, cy + chevSize * 0.5f);
                chevron.lineTo(cx + chevSize, cy - chevSize * 0.5f);
            }
            g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.50f));
            g.strokePath(chevron, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        }
    }

    void mouseUp(const juce::MouseEvent& /*e*/) override
    {
        toggleBreakout();
    }

    void mouseEnter(const juce::MouseEvent& /*e*/) override { repaint(); }
    void mouseExit(const juce::MouseEvent& /*e*/)  override { repaint(); }

private:
    //==========================================================================
    void timerCallback() override
    {
        // Mirror transport state from APVTS into local flags.
        // (Actual step position is updated by the parent calling setCurrentStep()
        // on a 15 Hz timer or whenever the processor fires a change notification.)
        if (isShowing())
            repaint();
    }

    //--------------------------------------------------------------------------
    void toggleBreakout()
    {
        if (breakout_ == nullptr)
            return;

        breakoutOpen_ = !breakoutOpen_;
        breakout_->setVisible(breakoutOpen_);
        repaint();
    }

    //--------------------------------------------------------------------------
    int readParamInt(const juce::String& suffix, int fallback) const
    {
        if (auto* p = apvts_.getParameter(prefix_ + suffix))
            return static_cast<int>(p->convertFrom0to1(p->getValue()) + 0.5f);
        return fallback;
    }

    bool readParamBool(const juce::String& suffix) const
    {
        if (auto* p = apvts_.getParameter(prefix_ + suffix))
            return p->convertFrom0to1(p->getValue()) > 0.5f;
        return false;
    }

    //==========================================================================
    juce::AudioProcessorValueTreeState& apvts_;
    juce::String                        prefix_;
    SeqBreakoutComponent*               breakout_    = nullptr;
    bool                                breakoutOpen_ = false;

    std::atomic<int>  currentStep_{0};
    std::atomic<bool> isPlaying_{false};

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SeqStripComponent)
};

} // namespace xoceanus
