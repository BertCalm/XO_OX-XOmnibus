#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include "GalleryKnob.h"
#include "MidiLearnMouseListener.h"
#include "CockpitHost.h"

namespace xolokun {

class MacroSection : public juce::Component
{
public:
    explicit MacroSection(juce::AudioProcessorValueTreeState& apvts)
    {
        A11y::setup(*this, "Macro Controls", "Four macro knobs: Character, Movement, Coupling, Space");

        struct Def { const char* id; const char* label; };
        // Short display labels (fit compact header); tooltips/a11y retain full names.
        static constexpr Def defs[4] = {
            {"macro1","CHAR"}, {"macro2","MOVE"},
            {"macro3","COUP"}, {"macro4","SPACE"}
        };
        static constexpr const char* tooltipLabels[4] = {"CHARACTER","MOVEMENT","COUPLING","SPACE"};
        for (int i = 0; i < 4; ++i)
        {
            knobs[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setColour(juce::Slider::rotarySliderFillColourId,
                               GalleryColors::get(GalleryColors::xoGold));
            knobs[i].setTooltip(juce::String("Macro ") + juce::String(i + 1) + ": " + tooltipLabels[i]);
            A11y::setup (knobs[i], juce::String ("Macro ") + juce::String (i + 1) + " " + tooltipLabels[i]);
            addAndMakeVisible(knobs[i]);
            attach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, defs[i].id, knobs[i]);
            enableKnobReset (knobs[i], apvts, defs[i].id);

            lbls[i].setText(defs[i].label, juce::dontSendNotification); // short label (CHAR/MOVE/COUP/SPACE)
            lbls[i].setFont(GalleryFonts::value(8.0f));
            lbls[i].setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::xoGold).withAlpha(0.75f));
            lbls[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(lbls[i]);
        }

        master.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        master.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        master.setColour(juce::Slider::rotarySliderFillColourId,
                         GalleryColors::get(GalleryColors::textMid()));
        master.setTooltip("Master output volume");
        A11y::setup (master, "Master Volume");
        addAndMakeVisible(master);
        masterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "masterVolume", master);
        enableKnobReset (master, apvts, "masterVolume");

        masterLbl.setText("VOL", juce::dontSendNotification);
        masterLbl.setFont(GalleryFonts::value(8.0f));
        masterLbl.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::xoGold).withAlpha(0.75f));
        masterLbl.setJustificationType(juce::Justification::centred);
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
        if (opacity < 0.05f) return; // B041 performance optimization
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
        // Header integration: effective height ~44px (52pt header - 4px reduced).
        // Prototype: KNOB_HEADER = 32px
        auto b = getLocalBounds().reduced(4, 2);
        constexpr int kh = 32, lh = 10;
        int numKnobs = 5; // 4 macros + master
        int cw = b.getWidth() / numKnobs;

        for (int i = 0; i < 4; ++i)
        {
            auto col = b.removeFromLeft(cw);
            int kx = col.getCentreX() - kh / 2;
            knobs[i].setBounds(kx, col.getY(), kh, kh);
            lbls[i].setBounds(kx - 4, col.getY() + kh + 1, kh + 8, lh);
        }
        // Master volume in the remaining column
        auto masterCol = b;
        int mx = masterCol.getCentreX() - kh / 2;
        master.setBounds(mx, masterCol.getY(), kh, kh);
        masterLbl.setBounds(mx - 4, masterCol.getY() + kh + 1, kh + 8, lh);
    }

    // Called by PresetBrowserStrip when a preset with custom macroLabels is loaded.
    // Short display names are used to fit the compact header layout.
    void setLabels(const juce::StringArray& labels)
    {
        static const char* defaults[4] = {"CHAR","MOVE","COUP","SPACE"};
        for (int i = 0; i < 4; ++i)
        {
            auto text = (i < labels.size() && labels[i].isNotEmpty())
                            ? labels[i] : juce::String(defaults[i]);
            lbls[i].setText(text, juce::dontSendNotification);
        }
    }

    // Wire MIDI Learn to each macro knob and master.
    // Call after construction (from XOlokunEditor) so the attachment already exists.
    void setupMidiLearn(MIDILearnManager& mgr)
    {
        static const char* ids[4] = { "macro1", "macro2", "macro3", "macro4" };
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
    std::array<juce::Label,  4> lbls;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> attach;
    GalleryKnob master;
    juce::Label  masterLbl;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAttach;
    // MIDI learn listeners — destroyed before knobs (reverse declaration order)
    std::array<std::unique_ptr<MidiLearnMouseListener>, 4> macroLearnListeners;
    std::unique_ptr<MidiLearnMouseListener> masterLearnListener;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroSection)
};

} // namespace xolokun
