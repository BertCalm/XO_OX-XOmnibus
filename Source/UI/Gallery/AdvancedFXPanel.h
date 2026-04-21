// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include "GalleryKnob.h"

namespace xoceanus
{

//==============================================================================
// AdvancedFXPanel — generic advanced parameter popup for any FX section.
// Takes an array of parameter ID/label pairs and displays them as rotary knobs.
class AdvancedFXPanel : public juce::Component
{
public:
    AdvancedFXPanel(juce::AudioProcessorValueTreeState& apvts, const juce::String& title,
                    const std::vector<std::pair<juce::String, juce::String>>& paramDefs)
    {
        titleText = title;
        int count = juce::jmin(static_cast<int>(paramDefs.size()), kMaxKnobs);
        numKnobs = count;

        for (int i = 0; i < count; ++i)
        {
            knobs[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setColour(juce::Slider::rotarySliderFillColourId,
                               GalleryColors::get(GalleryColors::textMid()).withAlpha(0.75f));
            addAndMakeVisible(knobs[i]);
            attach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, paramDefs[static_cast<size_t>(i)].first, knobs[i]);
            enableKnobReset(knobs[i], apvts, paramDefs[static_cast<size_t>(i)].first);
            A11y::setup(knobs[i], paramDefs[static_cast<size_t>(i)].second,
                        "Advanced FX parameter: " + paramDefs[static_cast<size_t>(i)].second);

            lbls[i].setText(paramDefs[static_cast<size_t>(i)].second, juce::dontSendNotification);
            lbls[i].setFont(GalleryFonts::heading(10.0f)); // (#885: 8.5pt→10pt legibility floor)
            lbls[i].setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()));
            lbls[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(lbls[i]);
            A11y::setup(lbls[i], paramDefs[static_cast<size_t>(i)].second + " Label", {}, false);
        }
        setSize(64 * count + 16, 96);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll(get(shellWhite()));
        g.setColour(get(textMid()));
        g.setFont(GalleryFonts::heading(10.0f)); // (#885: 8pt→10pt legibility floor)
        g.drawText(titleText, getLocalBounds().removeFromTop(14).reduced(8, 0), juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(8, 4);
        b.removeFromTop(14);
        int cw = numKnobs > 0 ? b.getWidth() / numKnobs : b.getWidth();
        for (int i = 0; i < numKnobs; ++i)
        {
            auto col = b.removeFromLeft(cw);
            int kh = 44;
            int ky = col.getCentreY() - (kh + 13) / 2;
            knobs[i].setBounds(col.getCentreX() - kh / 2, ky, kh, kh);
            lbls[i].setBounds(col.getX(), ky + kh + 2, col.getWidth(), 12);
        }
    }

private:
    static constexpr int kMaxKnobs = 8;
    juce::String titleText;
    int numKnobs = 0;
    std::array<GalleryKnob, kMaxKnobs> knobs;
    std::array<juce::Label, kMaxKnobs> lbls;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, kMaxKnobs> attach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedFXPanel)
};

} // namespace xoceanus
