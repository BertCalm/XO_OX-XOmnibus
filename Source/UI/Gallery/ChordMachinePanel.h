// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOceanusProcessor.h"
#include "../GalleryColors.h"

namespace xoceanus
{

//==============================================================================
// ChordMachinePanel — visual interface for the Chord Machine.
//
// Shows: chord strip (4 slot cards), 16-step grid, control knobs.
// Polls ChordMachine state at 15Hz for real-time visualization.
//
class ChordMachinePanel : public juce::Component, private juce::Timer
{
public:
    explicit ChordMachinePanel(XOceanusProcessor& proc) : processor(proc)
    {
        setTitle("Chord Machine");
        setDescription("Generative chord sequencer with palette, voicing, and pattern controls");

        // ON/OFF toggle
        addAndMakeVisible(enableBtn);
        enableBtn.setButtonText("OFF");
        enableBtn.setClickingTogglesState(true);
        A11y::setup(enableBtn, "Chord Machine Enable", "Toggle chord machine on or off");
        enableBtn.onClick = [this]
        {
            bool on = enableBtn.getToggleState();
            if (auto* p = processor.getAPVTS().getParameter("cm_enabled"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(on ? 1.0f : 0.0f);
                p->endChangeGesture();
            }
            enableBtn.setButtonText(on ? "ON" : "OFF");
        };

        // Sequencer play/stop
        addAndMakeVisible(seqBtn);
        seqBtn.setButtonText("SEQ");
        seqBtn.setClickingTogglesState(true);
        seqBtn.onClick = [this]
        {
            bool on = seqBtn.getToggleState();
            if (auto* p = processor.getAPVTS().getParameter("cm_seq_running"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(on ? 1.0f : 0.0f);
                p->endChangeGesture();
            }
        };

        // Palette selector
        addAndMakeVisible(paletteBox);
        paletteBox.addItemList({"WARM", "BRIGHT", "TENSION", "OPEN", "DARK", "SWEET", "COMPLEX", "RAW"}, 1);
        paletteBox.setSelectedId(1, juce::dontSendNotification);
        paletteBox.onChange = [this]
        {
            if (auto* p = processor.getAPVTS().getParameter("cm_palette"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(static_cast<float>(paletteBox.getSelectedItemIndex()) / 7.0f);
                p->endChangeGesture();
            }
        };

        // Voicing selector
        addAndMakeVisible(voicingBox);
        voicingBox.addItemList({"ROOT-SPREAD", "DROP-2", "QUARTAL", "UPPER STRUCT", "UNISON"}, 1);
        voicingBox.setSelectedId(1, juce::dontSendNotification);
        voicingBox.onChange = [this]
        {
            if (auto* p = processor.getAPVTS().getParameter("cm_voicing"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(static_cast<float>(voicingBox.getSelectedItemIndex()) / 4.0f);
                p->endChangeGesture();
            }
        };

        // Pattern selector
        addAndMakeVisible(patternBox);
        patternBox.addItemList({"FOUR", "OFF-BEAT", "SYNCO", "STAB", "GATE", "PULSE", "BROKEN", "REST"}, 1);
        patternBox.setSelectedId(2, juce::dontSendNotification);
        patternBox.onChange = [this]
        {
            // W13 fix: do NOT call applyPattern() directly from the UI thread —
            // it races with the audio thread reading steps[].active.
            // Instead, write the APVTS parameter; processBlock() syncs it via
            // chordMachine.applyPattern() on every block (W12 fix, XOceanusProcessor.cpp).
            auto idx = patternBox.getSelectedItemIndex();
            if (auto* p = processor.getAPVTS().getParameter("cm_seq_pattern"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(static_cast<float>(idx) / 7.0f);
                p->endChangeGesture();
            }
        };

        // Velocity curve selector
        addAndMakeVisible(velCurveBox);
        velCurveBox.addItemList({"EQUAL", "ROOT HEAVY", "TOP BRIGHT", "V-SHAPE"}, 1);
        velCurveBox.setSelectedId(2, juce::dontSendNotification);
        velCurveBox.onChange = [this]
        {
            if (auto* p = processor.getAPVTS().getParameter("cm_vel_curve"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(static_cast<float>(velCurveBox.getSelectedItemIndex()) / 3.0f);
                p->endChangeGesture();
            }
        };

        // Knobs
        auto makeKnob = [this](juce::Slider& knob, const juce::String& paramId)
        {
            knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
            addAndMakeVisible(knob);
            attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.getAPVTS(), paramId, knob));
        };

        makeKnob(spreadKnob, "cm_spread");
        makeKnob(bpmKnob, "cm_seq_bpm");
        makeKnob(swingKnob, "cm_seq_swing");
        makeKnob(gateKnob, "cm_seq_gate");
        makeKnob(humanizeKnob, "cm_humanize");
        makeKnob(duckKnob, "cm_sidechain_duck");

        // ENO mode toggle
        addAndMakeVisible(enoBtn);
        enoBtn.setButtonText("ENO");
        enoBtn.setClickingTogglesState(true);
        enoBtn.onClick = [this]
        {
            if (auto* p = processor.getAPVTS().getParameter("cm_eno_mode"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(enoBtn.getToggleState() ? 1.0f : 0.0f);
                p->endChangeGesture();
            }
        };

        startTimerHz(10); // Reduced from 15Hz — sufficient for step highlighting
    }

    ~ChordMachinePanel() override { stopTimer(); }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        auto& cm = processor.getChordMachine();
        auto assignment = cm.getCurrentAssignment();

        // ── Chord Strip (4 slot cards) ──
        auto stripArea = getLocalBounds().reduced(6, 0).withY(kTopBarH + 4).withHeight(kStripH);

        int cardW = (stripArea.getWidth() - 18) / 4;
        for (int i = 0; i < 4; ++i)
        {
            auto card = stripArea.withX(stripArea.getX() + i * (cardW + 6)).withWidth(cardW);

            // Card background
            g.setColour(get(slotBg()));
            g.fillRoundedRectangle(card.toFloat(), 4.0f);
            g.setColour(get(borderGray()));
            g.drawRoundedRectangle(card.toFloat(), 4.0f, 1.0f);

            // Engine accent bar at bottom
            auto* eng = processor.getEngine(i);
            juce::Colour accent = eng ? accentForEngine(eng->getEngineId()) : get(emptySlot());
            g.setColour(accent);
            g.fillRect(card.removeFromBottom(4).toFloat());

            // Slot number
            g.setColour(get(textMid()));
            g.setFont(GalleryFonts::body(10.0f));
            g.drawText("S" + juce::String(i + 1), card.removeFromTop(14), juce::Justification::centred);

            // MIDI note name
            g.setColour(get(textDark()));
            g.setFont(GalleryFonts::display(16.0f));
            g.drawText(ChordMachine::midiNoteToName(assignment.midiNotes[i]), card.removeFromTop(22),
                       juce::Justification::centred);

            // Engine name
            g.setColour(accent);
            g.setFont(GalleryFonts::body(10.0f));
            juce::String eName = eng ? eng->getEngineId() : "—";
            g.drawText(eName, card.removeFromTop(14), juce::Justification::centred);
        }

        // ── Step Grid (16 steps) ──
        auto gridArea = getLocalBounds().reduced(6, 0).withY(kTopBarH + kStripH + 10).withHeight(kGridH);

        int stepW = (gridArea.getWidth() - 30) / 16;
        int curStep = cm.getCurrentStep();
        bool seqOn = cm.isSequencerRunning();

        for (int s = 0; s < 16; ++s)
        {
            auto stepR = gridArea.withX(gridArea.getX() + s * (stepW + 2)).withWidth(stepW);

            auto stepData = cm.getStep(s);
            bool isActive = stepData.active;
            bool isCurrent = seqOn && (s == curStep);

            // Step cell
            juce::Colour cellCol;
            if (isCurrent && isActive)
                cellCol = get(xoGold);
            else if (isCurrent)
                cellCol = get(xoGold).withAlpha(0.3f);
            else if (isActive)
                cellCol = get(textDark()).withAlpha(0.15f);
            else
                cellCol = get(slotBg());

            g.setColour(cellCol);
            g.fillRoundedRectangle(stepR.toFloat().withTrimmedBottom(16), 3.0f);
            g.setColour(get(borderGray()));
            g.drawRoundedRectangle(stepR.toFloat().withTrimmedBottom(16), 3.0f, 0.5f);

            // Beat marker (steps 0, 4, 8, 12)
            if ((s & 3) == 0)
            {
                g.setColour(get(textMid()).withAlpha(0.4f));
                g.fillRect(stepR.getX(), stepR.getY() - 3, stepW, 2);
            }

            // Root note label below
            if (stepData.rootNote >= 0)
            {
                g.setColour(get(textMid()));
                g.setFont(GalleryFonts::label(10.0f)); // (#885: 8pt→10pt legibility floor)
                g.drawText(ChordMachine::midiNoteToName(stepData.rootNote),
                           stepR.withY(stepR.getBottom() - 14).withHeight(14), juce::Justification::centred);
            }
        }

        // ── Knob Labels ──
        auto knobArea = getLocalBounds().reduced(6, 0).withY(kTopBarH + kStripH + kGridH + 16).withHeight(kKnobH);

        static const char* knobLabels[] = {"SPREAD", "BPM", "SWING", "GATE", "HUMAN", "DUCK"};
        int knobW = (knobArea.getWidth() - 5 * 8) / 6;
        for (int i = 0; i < 6; ++i)
        {
            auto labelR = knobArea.withX(knobArea.getX() + i * (knobW + 8)).withWidth(knobW).removeFromTop(14);
            g.setColour(get(textMid()));
            g.setFont(GalleryFonts::body(10.0f)); // (#885: 9pt→10pt legibility floor)
            g.drawText(knobLabels[i], labelR, juce::Justification::centred);
        }

        // Spread label (dynamic)
        float curSpread = cm.getSpread();
        g.setColour(get(xoGold));
        g.setFont(GalleryFonts::heading(10.0f)); // (#885: 8pt→10pt legibility floor)
        auto spreadLabelR = knobArea.withWidth(knobW).removeFromBottom(12);
        g.drawText(ChordMachine::spreadLabel(curSpread), spreadLabelR, juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(6, 0);

        // ── Top control bar ──
        auto top = area.removeFromTop(kTopBarH);
        enableBtn.setBounds(top.removeFromLeft(44).reduced(2));
        paletteBox.setBounds(top.removeFromLeft(90).reduced(2));
        voicingBox.setBounds(top.removeFromLeft(100).reduced(2));
        seqBtn.setBounds(top.removeFromLeft(44).reduced(2));
        patternBox.setBounds(top.removeFromLeft(85).reduced(2));
        top.removeFromLeft(8);
        velCurveBox.setBounds(top.removeFromLeft(95).reduced(2));
        top.removeFromLeft(4);
        enoBtn.setBounds(top.removeFromLeft(44).reduced(2));

        // ── Knobs ──
        auto knobArea = getLocalBounds().reduced(6, 0).withY(kTopBarH + kStripH + kGridH + 16).withHeight(kKnobH);

        int knobW = (knobArea.getWidth() - 5 * 8) / 6;
        juce::Slider* knobs[] = {&spreadKnob, &bpmKnob, &swingKnob, &gateKnob, &humanizeKnob, &duckKnob};
        for (int i = 0; i < 6; ++i)
        {
            auto r = knobArea.withX(knobArea.getX() + i * (knobW + 8)).withWidth(knobW);
            knobs[i]->setBounds(r.withTrimmedTop(14));
        }
    }

    //==========================================================================
    // Accessibility: expose per-step state to screen readers.
    // JUCE routes getAccessibleDescription() through the component's
    // AccessibilityHandler; we update it in timerCallback so the current
    // sequencer step is always announced.
    juce::String buildStepDescription() const
    {
        auto& cm = processor.getChordMachine();
        int curStep = cm.getCurrentStep();
        bool seqOn = cm.isSequencerRunning();

        juce::String desc = "Chord Machine sequencer";
        if (seqOn)
            desc += ", playing, step " + juce::String(curStep + 1) + " of 16";
        else
            desc += ", stopped";

        // Summarise active steps for screen readers
        juce::StringArray activeNotes;
        for (int s = 0; s < 16; ++s)
        {
            auto step = cm.getStep(s);
            if (step.active && step.rootNote >= 0)
                activeNotes.add("S" + juce::String(s + 1) + ":" + ChordMachine::midiNoteToName(step.rootNote));
        }
        if (activeNotes.isEmpty())
            desc += ", no active steps";
        else
            desc += ", active steps: " + activeNotes.joinIntoString(" ");

        return desc;
    }

private:
    void timerCallback() override
    {
        // Only repaint when sequencer step changes (avoids full redraw at 10Hz)
        int step = processor.getChordMachine().getCurrentStep();
        if (step != lastPaintedStep)
        {
            lastPaintedStep = step;
            // Update accessible description so screen readers announce the
            // current step position without needing separate focusable children
            // for each painted step cell (addresses #392).
            setDescription(buildStepDescription());
            repaint();
        }
    }
    int lastPaintedStep = -1;

    static constexpr int kTopBarH = 30;
    static constexpr int kStripH = 80;
    static constexpr int kGridH = 70;
    static constexpr int kKnobH = 90;

    XOceanusProcessor& processor;

    juce::TextButton enableBtn, seqBtn, enoBtn;
    juce::ComboBox paletteBox, voicingBox, patternBox, velCurveBox;
    juce::Slider spreadKnob, bpmKnob, swingKnob, gateKnob, humanizeKnob, duckKnob;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordMachinePanel)
};

} // namespace xoceanus
