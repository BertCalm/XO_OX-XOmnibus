#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../EngineVocabulary.h"
#include "../GalleryColors.h"
#include "MidiLearnMouseListener.h"

namespace xolokun
{

//==============================================================================
// MacroHeroStrip — full-width "character portrait" panel showing 4 tall pillar
// sliders for an engine's macro parameters. Appears at the top of EngineDetailPanel
// when an engine with macro params is selected.
//
// Macro param discovery: iterates APVTS for params matching "{prefix}_macro*"
// (up to 4 collected in declaration order). If fewer than 4 are found the
// remaining pillars are hidden. If none are found the strip hides itself.
class MacroHeroStrip : public juce::Component
{
public:
    explicit MacroHeroStrip(XOlokunProcessor& proc) : processor(proc) {}

    // Optional: wire MIDI learn before the first loadEngine() call.
    void setMidiLearnManager(MIDILearnManager* mgr) { learnManager = mgr; }

    // Call after an engine slot is selected. Returns true if at least one
    // macro param was found and the strip should be shown.
    bool loadEngine(const juce::String& engId, const juce::String& paramPrefix,
                    juce::Colour accent)
    {
        engineName   = engId;
        accentColour = accent;

        // Clear previous attachments (must happen before slider rebuild)
        for (auto& att : attachments)
            att.reset();

        // Determine the full prefix used for param lookup (some prefixes already
        // contain a trailing underscore in the frozen table, normalise here).
        juce::String pfx = paramPrefix;
        if (!pfx.endsWithChar('_'))
            pfx += "_";
        juce::String macroPrefix = pfx + "macro"; // e.g. "oasis_macro"

        // Collect up to 4 macro param IDs from the APVTS in declaration order.
        auto& apvts = processor.getAPVTS();
        juce::StringArray foundIds, foundNames;

        for (auto* p : processor.getParameters())
        {
            if (foundIds.size() >= 4)
                break;
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            {
                if (rp->getParameterID().startsWithIgnoreCase(macroPrefix))
                {
                    foundIds.add(rp->getParameterID());
                    // Derive short label: vocabulary override first, then
                    // everything after "_macro" → uppercase as fallback.
                    juce::String raw = rp->getParameterID().substring(macroPrefix.length());
                    juce::String defaultLabel = raw.isEmpty()
                        ? juce::String("M" + juce::String(foundIds.size()))
                        : raw.toUpperCase();
                    foundNames.add(EngineVocabulary::labelFor(
                        engineName, rp->getParameterID(), defaultLabel));
                }
            }
        }

        numMacros = foundIds.size();
        if (numMacros == 0)
        {
            setVisible(false);
            return false;
        }

        // Configure visible pillars and attach to found params
        for (int i = 0; i < 4; ++i)
        {
            bool active = (i < numMacros);
            pillars[i].setVisible(active);
            pillarLabels[i].setVisible(active);

            if (active)
            {
                pillars[i].setSliderStyle(juce::Slider::LinearVertical);
                pillars[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
                pillars[i].setColour(juce::Slider::trackColourId,
                                     accent.withAlpha(0.18f));
                pillars[i].setColour(juce::Slider::thumbColourId, accent);
                pillars[i].setColour(juce::Slider::backgroundColourId,
                                     GalleryColors::get(GalleryColors::borderGray()));
                pillars[i].setTooltip(foundNames[i]);
                A11y::setup(pillars[i], foundNames[i]);

                pillarLabels[i].setText(foundNames[i], juce::dontSendNotification);
                pillarLabels[i].setFont(GalleryFonts::heading(8.0f));
                pillarLabels[i].setColour(juce::Label::textColourId, accent.darker(0.2f));
                pillarLabels[i].setJustificationType(juce::Justification::centred);

                attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    apvts, foundIds[i], pillars[i]);

                // Re-wire MIDI learn for the new param ID
                if (learnManager)
                {
                    // Remove old listener if re-loading (engine swap)
                    if (i < (int)pillarLearnListeners.size() && pillarLearnListeners[i])
                        pillars[i].removeMouseListener(pillarLearnListeners[i].get());

                    auto* ml = attachMidiLearn(pillars[i], foundIds[i], *learnManager);
                    if (i < (int)pillarLearnListeners.size())
                        pillarLearnListeners[i].reset(ml);
                    else
                        pillarLearnListeners.emplace_back(ml);
                }
            }
        }

        setVisible(true);
        resized();
        repaint();
        return true;
    }

    void paint(juce::Graphics& g) override
    {
        if (numMacros == 0)
            return;

        using namespace GalleryColors;
        auto b = getLocalBounds().toFloat();

        // Background — slightly tinted with accent
        g.setColour(get(shellWhite()));
        g.fillRoundedRectangle(b, 4.0f);

        // Accent top strip
        g.setColour(accentColour);
        g.fillRect(b.removeFromTop(3.0f));

        // Engine name header line
        g.setColour(accentColour.darker(0.2f));
        g.setFont(GalleryFonts::display(11.0f));
        g.drawText(engineName.toUpperCase() + "  —  MACROS",
                   8, 3, getWidth() - 16, kHeaderH - 3,
                   juce::Justification::centredLeft);

        // Thin separator under header
        g.setColour(juce::Colour(GalleryColors::xoGold).withAlpha(0.45f));
        g.drawHorizontalLine(kHeaderH, 8.0f, (float)(getWidth() - 8));
    }

    void resized() override
    {
        if (numMacros == 0)
            return;

        const int labelH  = 14;
        const int sliderY = kHeaderH + 4;
        const int sliderH = getHeight() - sliderY - labelH - 4;
        const int colW    = getWidth() / 4;

        for (int i = 0; i < numMacros; ++i)
        {
            int x = i * colW;
            pillars[i].setBounds(x + 4, sliderY, colW - 8, juce::jmax(8, sliderH));
            pillarLabels[i].setBounds(x, getHeight() - labelH - 2, colW, labelH);
        }
    }

private:
    static constexpr int kHeaderH = 22;

    XOlokunProcessor& processor;
    juce::String       engineName;
    juce::Colour       accentColour { GalleryColors::get(GalleryColors::borderGray()) };
    int                numMacros = 0;
    MIDILearnManager*  learnManager = nullptr;

    std::array<juce::Slider, 4> pillars;
    std::array<juce::Label,  4> pillarLabels;
    // Destruction order: listeners → attachments → sliders.
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> attachments;
    std::vector<std::unique_ptr<MidiLearnMouseListener>> pillarLearnListeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroHeroStrip)
};

} // namespace xolokun
