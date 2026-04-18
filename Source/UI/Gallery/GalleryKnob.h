// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include "MidiLearnMouseListener.h"

namespace xoceanus
{

//==============================================================================
// GalleryKnob — juce::Slider subclass that adds:
//   • double-click + Cmd+click reset to default value
//   • right-click MIDI Learn context menu (when wired via setupMidiLearn)
//   • amber pulsing ring while listening for CC; green "ML" badge when mapped
//
class GalleryKnob : public juce::Slider
{
public:
    GalleryKnob() = default;

    // Store the denormalized default value for Cmd+click reset.
    void setDefaultValue(double v) { defaultValue = v; }

    // Modulation arc state — called by parent component's timer (e.g. a coupling
    // polling timer) to show active modulation depth on this knob.
    //
    //   amount  — bipolar -1.0 to +1.0 depth as a fraction of the full arc sweep.
    //             Pass 0.0f to clear the arc.
    //   colour  — coupling-type colour from CouplingTypeColors::forType(); defaults
    //             to XO standard mod blue when omitted.
    //
    // Data is stored in JUCE's NamedValueSet properties so drawRotarySlider can
    // read it without any LookAndFeel signature change:
    //   "modAmount"  → float  (-1..1)
    //   "modColour"  → int64_t (ARGB as returned by Colour::getARGB())
    void setModulation(float amount, juce::Colour colour = juce::Colour(0xFF4488FF))
    {
        const float  clamped = juce::jlimit(-1.0f, 1.0f, amount);
        const auto   argb    = static_cast<int64_t>(colour.getARGB());
        auto&        props   = getProperties();

        const bool changed = (static_cast<float>(props["modAmount"]) != clamped)
                          || (static_cast<int64_t>(props["modColour"]) != argb);

        if (changed)
        {
            props.set("modAmount", clamped);
            props.set("modColour", argb);
            repaint();
        }
    }

    // Convenience overload — clear any active modulation arc.
    void clearModulation() { setModulation(0.0f); }

    // Wire up MIDI Learn for this knob.  Call after SliderAttachment is created.
    // The caller must store the returned listener pointer in a unique_ptr vector.
    MidiLearnMouseListener* setupMidiLearn(const juce::String& pid, MIDILearnManager& mgr)
    {
        paramId = pid;
        learnManager = &mgr;
        auto* ml = new MidiLearnMouseListener(pid, mgr);
        addMouseListener(ml, false);
        return ml;
    }

    // WCAG 2.4.7 — repaint on focus change so the LookAndFeel focus ring
    // appears/disappears immediately when the user tabs to/from this knob.
    void focusGained(FocusChangeType) override { repaint(); }
    void focusLost(FocusChangeType) override { repaint(); }

    // Hover ring — repaint on enter/exit so drawRotarySlider sees the updated
    // isMouseOver() state and can draw/remove the passive hover highlight.
    void mouseEnter(const juce::MouseEvent& e) override
    {
        juce::Slider::mouseEnter(e);
        repaint();
    }
    void mouseExit(const juce::MouseEvent& e) override
    {
        juce::Slider::mouseExit(e);
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Right-click is handled by MidiLearnMouseListener — don't swallow it here.
        if (e.mods.isRightButtonDown())
            return;

        // Cmd+click (macOS Command key) → reset to default immediately.
        if (e.mods.isCommandDown())
        {
            setValue(defaultValue, juce::sendNotificationAsync);
            juce::Slider::mouseDown(e);
            return;
        }
        juce::Slider::mouseDown(e);
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (e.mods.isShiftDown() && getInterval() == 0.0)
        {
            auto fine = w;
            fine.deltaY *= 0.05f;
            fine.deltaX *= 0.05f;
            juce::Slider::mouseWheelMove(e, fine);
        }
        else
        {
            juce::Slider::mouseWheelMove(e, w);
        }
    }

    void paint(juce::Graphics& g) override
    {
        juce::Slider::paint(g);
        drawMidiLearnOverlay(g);
    }

private:
    void drawMidiLearnOverlay(juce::Graphics& g)
    {
        if (!learnManager)
            return;
        bool isListening = learnManager->isLearning() && learnManager->getLearningParam() == paramId;
        bool isMapped = learnManager->hasMapping(paramId);
        if (!isListening && !isMapped)
            return;

        juce::Colour ringCol = isListening ? juce::Colour(GalleryColors::xoGold)        // XO Gold while listening
                                           : juce::Colour(0xFF4ADE80).withAlpha(0.50f); // soft green when mapped

        if (isListening)
        {
            double t = juce::Time::getMillisecondCounterHiRes() * 0.002;
            float pulse = 0.55f + 0.45f * (float)std::sin(t * juce::MathConstants<double>::twoPi);
            ringCol = ringCol.withAlpha(pulse);
            // NOTE: do NOT call repaint() here — we are inside paint(), which
            // would create a busy-loop (repaint storm). The editor's 10Hz timer
            // already schedules repaints during MIDI learn mode.
        }

        auto b = getLocalBounds().toFloat().reduced(3.0f);
        float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
        g.setColour(ringCol);
        g.drawEllipse(b.getCentreX() - r, b.getCentreY() - r, r * 2.0f, r * 2.0f, isListening ? 2.5f : 1.5f);

        // "ML" text badge — only on knobs >= 34px (hidden on header-size 32px knobs)
        if (isMapped && !isListening && getWidth() >= 34)
        {
            g.setFont(GalleryFonts::label(10.0f)); // (#885: 8pt→10pt legibility floor)
            g.setColour(juce::Colour(0xFF4ADE80).withAlpha(0.70f));
            g.drawText("ML", b.getRight() - 16, b.getBottom() - 12, 14, 11, juce::Justification::centred);
        }
    }

    double defaultValue = 0.0;
    juce::String paramId;
    MIDILearnManager* learnManager = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GalleryKnob)
};

//==============================================================================
// enableKnobReset — convenience helper: reads the parameter default value and
// wires up both double-click and Cmd+click reset on a GalleryKnob.
//
// Must be called AFTER SliderAttachment is created (the attachment sets the
// slider range, which is needed to correctly interpret the default value).
static inline void enableKnobReset(GalleryKnob& knob, juce::AudioProcessorValueTreeState& apvts,
                                   const juce::String& paramId)
{
    if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(paramId)))
    {
        double defaultVal = rp->getNormalisableRange().convertFrom0to1(rp->getDefaultValue());
        knob.setDefaultValue(defaultVal);
        knob.setDoubleClickReturnValue(true, defaultVal);
    }
}

} // namespace xoceanus
