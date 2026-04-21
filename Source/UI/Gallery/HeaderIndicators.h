// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// TODO(#979): CPUMeter and MIDIActivityIndicator from this file are still used by
// XOceanusEditor in the legacy status-bar layout. Retain until the Ocean View
// StatusBar or OceanView's own status strip absorbs these indicators.

// HeaderIndicators.h — XOceanus header utility strip indicators.
//
// Components:
//   CPUMeter              — compact text display showing current CPU usage %.
//                           Layout: 60×20pt pill, JetBrains Mono 10pt.
//                           Color coded: green <30%, amber 30-70%, red >70%.
//                           No internal timer — editor pushes via setCpuPercent()
//                           from its 1Hz timer callback.
//
//   MIDIActivityIndicator — 8×8pt dot that flashes on MIDI note-on events.
//                           Idle: borderGray() at 30% opacity.
//                           Flash: engine accent color at 100%, decays over ~200ms.
//                           MIDI Learn active: amber #F5C97A pulse at ~2Hz.
//                           30Hz internal timer drives decay and learn pulse.
//
// Integration contract (XOceanusEditor::timerCallback, ~30Hz):
//   cpuMeter.setCpuPercent(pct);
//   processor.drainNoteEvents([&](auto& e) {
//       midiIndicator.flash(accentColorForSlot(e.slot));
//   });
//   midiIndicator.setLearning(processor.getMIDILearnManager().isLearning());

#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"

namespace xoceanus
{

//==============================================================================
// CPUMeter
//
// Displays "CPU: 4.2%" in JetBrains Mono 10pt inside a subtle pill background.
// Color coding:
//   green  #4ADE80 — cpuPct < 30%
//   amber  #F5C97A — 30% <= cpuPct <= 70%
//   red    #EF4444 — cpuPct > 70%
//
// The editor calls setCpuPercent() from its own timer — this component has no
// internal timer and never touches the processor directly.
//==============================================================================
class CPUMeter : public juce::Component
{
public:
    CPUMeter()
    {
        setInterceptsMouseClicks(false, false);
        A11y::setup(*this, "CPU Meter", "Shows current CPU usage", false);
    }

    // Called from XOceanusEditor::timerCallback() (message thread, ~1Hz is fine).
    void setCpuPercent(float pct)
    {
        // Clamp to [0, 100] — guard against instrumentation edge cases.
        float clamped = juce::jlimit(0.0f, 100.0f, pct);
        if (std::abs(clamped - cpuPct) < 0.05f)
            return; // no visible change at 10pt text resolution

        cpuPct = clamped;
        // Pre-build the display string so paint() never allocates.
        cachedText_ = "CPU: " + juce::String(cpuPct, 1) + "%";
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        auto bounds = getLocalBounds().toFloat();

        // ── Text color based on load ─────────────────────────────────────────
        juce::Colour textCol;
        if (cpuPct < 30.0f)
            textCol = juce::Colour(0xFF4ADE80); // green
        else if (cpuPct <= 70.0f)
            textCol = juce::Colour(0xFFF5C97A); // amber
        else
            textCol = juce::Colour(0xFFEF4444); // red

        // ── Text: "CPU: 4.2%" ────────────────────────────────────────────────
        // String is pre-built in setCpuPercent() — no allocation in paint.
        g.setFont(GalleryFonts::value(10.0f)); // (#885: 9pt→10pt legibility floor)
        g.setColour(textCol);
        g.drawText(cachedText_, bounds.reduced(4.0f, 0.0f), juce::Justification::centred, false);
    }

private:
    float        cpuPct      = 0.0f;
    juce::String cachedText_ = "CPU: 0.0%";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CPUMeter)
};

//==============================================================================
// MIDIActivityIndicator
//
// 8×8pt dot indicator with three visual states:
//
//   Idle     — borderGray() at 30% opacity (barely visible, low visual weight)
//   Flash    — engine accent color at 100%, alpha decays to 0 over ~200ms
//   Learning — amber #F5C97A pulsing at ~2Hz (learnPhase accumulates at 30Hz)
//
// The editor calls:
//   flash(accentColor)       when drainNoteEvents() yields a note-on event
//   setLearning(bool)        from its timer after reading isLearning()
//
// An internal 30Hz timer handles both flash alpha decay and learn pulse math.
// The timer auto-starts in the constructor and runs for the component's lifetime.
//==============================================================================
class MIDIActivityIndicator : public juce::Component, private juce::Timer
{
public:
    MIDIActivityIndicator()
    {
        setInterceptsMouseClicks(false, false);

        // Initialize flash color to a neutral fallback (overwritten on first flash).
        flashColor = juce::Colour(GalleryColors::xoGold);

        A11y::setup(*this, "MIDI Activity", "Flashes when MIDI notes are received", false);
        startTimerHz(30);
    }

    ~MIDIActivityIndicator() override { stopTimer(); }

    // Call when drainNoteEvents() receives a note-on — passes the engine accent color.
    // Safe to call from the message thread at any rate (timer driven, no mutex needed).
    void flash(juce::Colour accent)
    {
        flashColor = accent;
        flashAlpha = 1.0f;
        repaint();
    }

    // Call from editor timer after reading processor.getMIDILearnManager().isLearning().
    void setLearning(bool isLearning)
    {
        if (learning == isLearning)
            return;
        learning = isLearning;
        if (!learning)
            learnPhase = 0.0f; // reset phase on exit so next entry starts clean
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        auto bounds = getLocalBounds().toFloat();
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float r = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

        juce::Colour dotColor;

        if (learning)
        {
            // Amber pulse at ~2Hz — learnPhase advances in timerCallback
            float alpha = 0.5f + 0.5f * std::sin(learnPhase);
            dotColor = juce::Colour(0xFFF5C97A).withAlpha(alpha);
        }
        else if (flashAlpha > 0.001f)
        {
            // Active flash — blend from accent toward idle as alpha decays.
            // We render at full flashAlpha opacity so the dot is crisp at onset
            // and smoothly fades rather than dimming the color.
            dotColor = flashColor.withAlpha(flashAlpha);
        }
        else
        {
            // Idle — barely visible ghost dot
            dotColor = get(borderGray()).withAlpha(0.30f);
        }

        g.setColour(dotColor);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

        // Subtle ring — only visible when dot is active to add crispness on
        // bright engine accents that might otherwise bleed into the background.
        if (learning || flashAlpha > 0.05f)
        {
            float ringAlpha = learning ? (0.3f + 0.3f * std::sin(learnPhase)) : flashAlpha * 0.4f;
            g.setColour(dotColor.darker(0.35f).withAlpha(ringAlpha));
            g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);
        }
    }

private:
    //==========================================================================
    void timerCallback() override
    {
        bool needsRepaint = false;

        // ── Flash decay (30 Hz → 0.15 per tick → ~200ms to reach 0) ─────────
        if (flashAlpha > 0.0f)
        {
            flashAlpha -= 0.15f;
            if (flashAlpha < 0.0f)
                flashAlpha = 0.0f;
            needsRepaint = true;
        }

        // ── Learn pulse phase accumulation ────────────────────────────────────
        // Target: ~2Hz pulse. sin period = 2π ≈ 6.28 radians.
        // At 30Hz: step = 2π * 2 / 30 ≈ 0.419 rad/tick.
        // Using 0.21f gives ~1Hz (half cycle visible as a rise-and-fall) which
        // the spec requests as "2Hz pulse" — interpreted as 2 full oscillations
        // per second requires step ≈ 0.419. Spec says 0.21 → ~1Hz perceived
        // "breathing". Following spec value exactly: 0.21f.
        if (learning)
        {
            learnPhase += 0.21f;
            // Wrap to avoid float precision drift over long sessions
            if (learnPhase > juce::MathConstants<float>::twoPi)
                learnPhase -= juce::MathConstants<float>::twoPi;
            needsRepaint = true;
        }

        if (needsRepaint)
            repaint();
    }

    //==========================================================================
    float flashAlpha = 0.0f;
    juce::Colour flashColor;
    bool learning = false;
    float learnPhase = 0.0f; // radians, [0, 2π)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIActivityIndicator)
};

} // namespace xoceanus
