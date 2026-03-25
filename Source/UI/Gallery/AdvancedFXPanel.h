#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include "GalleryKnob.h"

namespace xolokun {

//==============================================================================
// AdvancedFXPanel — generic advanced parameter popup for any FX section.
// Takes an array of parameter ID/label pairs and displays them as rotary knobs.
class AdvancedFXPanel : public juce::Component
{
public:
    AdvancedFXPanel(juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& title,
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
            enableKnobReset (knobs[i], apvts, paramDefs[static_cast<size_t>(i)].first);

            lbls[i].setText(paramDefs[static_cast<size_t>(i)].second, juce::dontSendNotification);
            lbls[i].setFont(GalleryFonts::heading(8.5f));
            lbls[i].setColour(juce::Label::textColourId,
                              GalleryColors::get(GalleryColors::textMid()));
            lbls[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(lbls[i]);
        }
        setSize(64 * count + 16, 96);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll(get(shellWhite()));
        g.setColour(get(textMid()).withAlpha(0.40f));
        g.setFont(GalleryFonts::heading(8.0f));
        g.drawText(titleText, getLocalBounds().removeFromTop(14).reduced(8, 0),
                   juce::Justification::centredLeft);
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
    std::array<juce::Label, kMaxKnobs>  lbls;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, kMaxKnobs> attach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedFXPanel)
};

} // namespace xolokun
