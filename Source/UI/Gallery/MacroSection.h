// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include "GalleryKnob.h"
#include "MidiLearnMouseListener.h"
#include "CockpitHost.h"

namespace xoceanus
{

class MacroSection : public juce::Component
{
public:
    explicit MacroSection(juce::AudioProcessorValueTreeState& apvts)
    {
        A11y::setup(*this, "Macro Controls", "Four macro knobs: Tone, Tide, Couple, Depth");

        struct Def
        {
            const char* id;
            const char* label;
        };
        // Display labels — D11 locked names (2026-04-25): TONE / TIDE / COUPLE / DEPTH.
        // "CHAR"/"CHARACTER" and "MOVE"/"MOVEMENT" were ambiguous; new names are
        // musically evocative and unambiguous. Tooltips mirror the display names.
        static constexpr Def defs[4] = {
            {"macro1", "TONE"}, {"macro2", "TIDE"}, {"macro3", "COUPLE"}, {"macro4", "DEPTH"}};
        // wire(#orphan-sweep): D11 spec descriptions added (issue #1301 audit).
        // Previously tooltips were bare echoes ("Macro 1: TONE") with no semantic content.
        static constexpr const char* tooltipDescs[4] = {
            "TONE — timbral character (waveshaper drive, EQ tilt, filter character)",
            "TIDE — motion and rhythm (LFO rate, modulation depth, temporal movement)",
            "COUPLE — engine coupling depth (cross-engine modulation intensity)",
            "DEPTH — layering and intensity (density, saturation, voice complexity)"
        };
        static constexpr const char* tooltipLabels[4] = {"TONE", "TIDE", "COUPLE", "DEPTH"};
        for (int i = 0; i < 4; ++i)
        {
            knobs[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setColour(juce::Slider::rotarySliderFillColourId, GalleryColors::get(GalleryColors::xoGold));
            knobs[i].setTooltip(juce::String(tooltipDescs[i]));
            A11y::setup(knobs[i], juce::String("Macro ") + juce::String(i + 1) + " " + tooltipLabels[i]);
            addAndMakeVisible(knobs[i]);
            attach[i] =
                std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, defs[i].id, knobs[i]);
            enableKnobReset(knobs[i], apvts, defs[i].id);

            lbls[i].setText(defs[i].label, juce::dontSendNotification);
            lbls[i].setFont(GalleryFonts::value(10.0f)); // (#885: 9pt→10pt legibility floor)
            lbls[i].setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::xoGold).withAlpha(0.75f));
            lbls[i].setJustificationType(juce::Justification::centredRight);
            addAndMakeVisible(lbls[i]);
        }

        master.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        master.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        master.setColour(juce::Slider::rotarySliderFillColourId, GalleryColors::get(GalleryColors::textMid()));
        master.setTooltip("VOLUME — Master output volume");
        A11y::setup(master, "Master Volume");
        addAndMakeVisible(master);
        masterAttach =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "masterVolume", master);
        enableKnobReset(master, apvts, "masterVolume");

        masterLbl.setText("VOL", juce::dontSendNotification); // D11: "VOLUME" exceeds 30px at 9pt mono — display stays "VOL"
        masterLbl.setFont(GalleryFonts::value(10.0f)); // (#885: 9pt→10pt legibility floor)
        masterLbl.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::xoGold).withAlpha(0.75f));
        masterLbl.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(masterLbl);
    }

    // W45 fix: remove MIDI learn mouse listeners from knobs before the unique_ptrs
    // are destroyed.  Member destruction order (reverse of declaration) would delete
    // macroLearnListeners BEFORE knobs, leaving dangling raw pointers in the knobs'
    // listener lists.  Explicit removal here prevents use-after-free.
    ~MacroSection() override
    {
        for (int i = 0; i < 4; ++i)
            if (macroLearnListeners[i])
                knobs[i].removeMouseListener(macroLearnListeners[i].get());
        if (masterLearnListener)
            master.removeMouseListener(masterLearnListener.get());
    }

    void paint(juce::Graphics& g) override
    {
        // Dark Cockpit B041: apply performance opacity
        float opacity = 1.0f;
        if (auto* host = CockpitHost::find(this))
            opacity = host->getCockpitOpacity();
        if (opacity < 0.05f)
            return; // B041 performance optimization
        g.setOpacity(opacity);

        using namespace GalleryColors;

        // Macro row top-highlight gradient (prototype spec)
        juce::ColourGradient grad(juce::Colour(0xFFFFFFFF).withAlpha(0.04f), 0.0f, 0.0f,
                                  juce::Colours::transparentBlack, 0.0f, (float)getHeight(), false);
        g.setGradientFill(grad);
        g.fillRect(getLocalBounds());

        // Subtle left separator — visually divides macro zone from title/logo area in header.
        // No card background or gold stripe: the header already provides shellWhite + gold bottom stripe.
        g.setColour(get(borderGray()).withAlpha(0.30f));
        g.drawVerticalLine(0, 4.0f, (float)getHeight() - 4);
    }

    void resized() override
    {
        // Layout: LABEL [KNOB] › LABEL [KNOB] › ... — label left of knob, tightly grouped
        auto b = getLocalBounds().reduced(4, 2);
        constexpr int kh = 48;             // knob diameter (#901: bumped 44→48pt for better discoverability)
        constexpr int lblW = 30;           // label width (TONE/TIDE/COUPLE/DEPTH fit; VOL displayed (VOLUME > 30px at 9pt mono))
        constexpr int gap = 2;             // between label and knob
        constexpr int groupGap = 4;        // between knob→next label
        int ky = (b.getHeight() - kh) / 2; // vertically center knobs

        int x = b.getX();
        for (int i = 0; i < 4; ++i)
        {
            lbls[i].setBounds(x, ky, lblW, kh);
            x += lblW + gap;
            knobs[i].setBounds(x, b.getY() + ky, kh, kh);
            x += kh + groupGap;
        }
        // Master volume
        masterLbl.setBounds(x, ky, lblW, kh);
        x += lblW + gap;
        master.setBounds(x, b.getY() + ky, kh, kh);
    }

    // Called by PresetBrowserStrip when a preset with custom macroLabels is loaded.
    // Short display names are used to fit the compact header layout.
    void setLabels(const juce::StringArray& labels)
    {
        static const char* defaults[4] = {"TONE", "TIDE", "COUPLE", "DEPTH"}; // D11 locked names
        for (int i = 0; i < 4; ++i)
        {
            auto text = (i < labels.size() && labels[i].isNotEmpty()) ? labels[i] : juce::String(defaults[i]);
            lbls[i].setText(text, juce::dontSendNotification);
        }
    }

    // Wire MIDI Learn to each macro knob and master.
    // Call after construction (from XOceanusEditor) so the attachment already exists.
    void setupMidiLearn(MIDILearnManager& mgr)
    {
        static const char* ids[4] = {"macro1", "macro2", "macro3", "macro4"};
        for (int i = 0; i < 4; ++i)
        {
            auto* ml = knobs[i].setupMidiLearn(ids[i], mgr);
            macroLearnListeners[i].reset(ml);
        }
        auto* ml = master.setupMidiLearn("masterVolume", mgr);
        masterLearnListener.reset(ml);
    }

private:
    std::array<GalleryKnob, 4> knobs;
    std::array<juce::Label, 4> lbls;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> attach;
    GalleryKnob master;
    juce::Label masterLbl;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAttach;
    // MIDI learn listeners — destroyed before knobs (reverse declaration order)
    std::array<std::unique_ptr<MidiLearnMouseListener>, 4> macroLearnListeners;
    std::unique_ptr<MidiLearnMouseListener> masterLearnListener;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroSection)
};

} // namespace xoceanus
