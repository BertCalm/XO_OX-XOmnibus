#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include "GalleryKnob.h"
#include "MidiLearnMouseListener.h"

namespace xolokun {

class MacroSection : public juce::Component
{
public:
    explicit MacroSection(juce::AudioProcessorValueTreeState& apvts)
    {
        struct Def { const char* id; const char* label; };
        static constexpr Def defs[4] = {
            {"macro1","CHARACTER"}, {"macro2","MOVEMENT"},
            {"macro3","COUPLING"},  {"macro4","SPACE"}
        };
        for (int i = 0; i < 4; ++i)
        {
            knobs[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setColour(juce::Slider::rotarySliderFillColourId,
                               GalleryColors::get(GalleryColors::xoGold));
            knobs[i].setTooltip(juce::String("Macro ") + juce::String(i + 1) + ": " + defs[i].label);
            A11y::setup (knobs[i], juce::String ("Macro ") + juce::String (i + 1) + " " + defs[i].label);
            addAndMakeVisible(knobs[i]);
            attach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, defs[i].id, knobs[i]);
            enableKnobReset (knobs[i], apvts, defs[i].id);

            lbls[i].setText(defs[i].label, juce::dontSendNotification);
            lbls[i].setFont(GalleryFonts::heading(8.0f));
            lbls[i].setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()));
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

        masterLbl.setText("MASTER", juce::dontSendNotification);
        masterLbl.setFont(GalleryFonts::heading(8.0f));
        masterLbl.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()));
        masterLbl.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(masterLbl);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        auto b = getLocalBounds().toFloat();
        g.setColour(get(shellWhite()));
        g.fillRoundedRectangle(b, 6.0f);
        g.setColour(get(borderGray()));
        g.drawRoundedRectangle(b.reduced(0.5f), 6.0f, 1.0f);

        // XO Gold top stripe — macros are brand constants, not ocean-depth values
        g.setColour(get(xoGold));
        g.fillRect(b.removeFromTop(3.0f));

        // MACROS label
        g.setColour(get(xoGoldText()));
        g.setFont(GalleryFonts::heading(8.0f));
        g.drawText("MACROS", getLocalBounds().removeFromTop(20), juce::Justification::centred);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(6, 2);
        b.removeFromTop(18);
        const int kh = 50, lh = 13;
        auto macroArea = b.removeFromLeft(b.getWidth() - 68);
        int cw = macroArea.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
        {
            auto col = macroArea.removeFromLeft(cw);
            int kx = col.getCentreX() - kh / 2;
            knobs[i].setBounds(kx, col.getY(), kh, kh);
            lbls[i].setBounds(kx, col.getY() + kh + 1, kh, lh);
        }
        int mx = b.getCentreX() - 22;
        master.setBounds(mx, b.getY(), 44, 44);
        masterLbl.setBounds(mx, b.getY() + 45, 44, lh);
    }

    // Called by PresetBrowserStrip when a preset with custom macroLabels is loaded.
    void setLabels(const juce::StringArray& labels)
    {
        static const char* defaults[4] = {"CHARACTER","MOVEMENT","COUPLING","SPACE"};
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
