// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOceanusProcessor.h"
#include "../EngineVocabulary.h"
#include "../GalleryColors.h"
#include "MidiLearnMouseListener.h"

namespace xoceanus
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
    explicit MacroHeroStrip(XOceanusProcessor& proc) : processor(proc)
    {
        for (int i = 0; i < 4; ++i)
        {
            addChildComponent(pillars[i]);      // hidden until loadEngine
            addChildComponent(pillarLabels[i]);
        }
    }

    // Remove MIDI learn mouse listeners from their sliders before the
    // unique_ptrs are destroyed — prevents use-after-free if a Slider still
    // holds a raw pointer to the deleted listener during teardown.
    ~MacroHeroStrip() override
    {
        for (int i = 0; i < (int)pillarLearnListeners.size() && i < 4; ++i)
            if (pillarLearnListeners[i])
                pillars[i].removeMouseListener(pillarLearnListeners[i].get());
    }

    // Optional: wire MIDI learn before the first loadEngine() call.
    void setMidiLearnManager(MIDILearnManager* mgr) { learnManager = mgr; }

    // Compact mode: suppress header, sliders fill full height. Used in Zone 2.
    void setCompactMode(bool c) { compactMode_ = c; resized(); repaint(); }

    // Call after an engine slot is selected. Returns true if at least one
    // macro param was found and the strip should be shown.
    bool loadEngine(const juce::String& engId, const juce::String& paramPrefix, juce::Colour accent)
    {
        engineName = engId;
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
                    juce::String defaultLabel =
                        raw.isEmpty() ? juce::String("M" + juce::String(foundIds.size())) : raw.toUpperCase();
                    foundNames.add(EngineVocabulary::labelFor(engineName, rp->getParameterID(), defaultLabel));
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
                if (compactMode_)
                {
                    // Submarine: XO Gold faders, visible track against dark glass
                    pillars[i].setColour(juce::Slider::trackColourId,
                                         juce::Colour(GalleryColors::xoGold).withAlpha(0.25f));
                    pillars[i].setColour(juce::Slider::thumbColourId,
                                         juce::Colour(GalleryColors::xoGold));
                    pillars[i].setColour(juce::Slider::backgroundColourId,
                                         juce::Colour(200, 204, 216).withAlpha(0.08f));
                }
                else
                {
                    pillars[i].setColour(juce::Slider::trackColourId, accent.withAlpha(0.18f));
                    pillars[i].setColour(juce::Slider::thumbColourId, accent);
                    pillars[i].setColour(juce::Slider::backgroundColourId,
                                         GalleryColors::get(GalleryColors::borderGray()));
                }
                // §1301: extend macro pillar tooltip beyond bare name to include
                // engine context and sweep hint (≤ 60 chars per voice/tone spec).
                juce::String macroTip = engineName + " " + foundNames[i]
                    + " macro (drag to sweep)";
                pillars[i].setTooltip(macroTip);
                A11y::setup(pillars[i], foundNames[i], macroTip);

                pillarLabels[i].setText(foundNames[i], juce::dontSendNotification);
                pillarLabels[i].setFont(GalleryFonts::value(10.0f)); // (#885: 8pt→10pt legibility floor)
                pillarLabels[i].setColour(juce::Label::textColourId,
                                          juce::Colour(GalleryColors::xoGold).withAlpha(0.75f));
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

        if (compactMode_)
        {
            // Submarine dark glass background — distinct from panel bg (20,23,32)
            g.setColour(juce::Colour(30, 34, 46));
            g.fillRoundedRectangle(b, 6.0f);
            // Subtle teal border
            g.setColour(juce::Colour(60, 180, 170).withAlpha(0.10f));
            g.drawRoundedRectangle(b.reduced(0.5f), 6.0f, 1.0f);
            // "MACROS" label at top
            g.setFont(GalleryFonts::value(9.0f));
            g.setColour(juce::Colour(GalleryColors::xoGold).withAlpha(0.5f));
            g.drawText("MACROS", b.toNearestInt().removeFromTop(16).reduced(2, 0),
                       juce::Justification::centred);
        }
        else
        {
            // Gallery mode background
            g.setColour(get(elevated()));
            g.fillRoundedRectangle(b, 4.0f);
            // Gradient overlay
            juce::ColourGradient overlay(juce::Colour(0x04FFFFFF), 0.0f, 0.0f,
                                         juce::Colours::transparentBlack, 0.0f, b.getHeight(), false);
            g.setGradientFill(overlay);
            g.fillRoundedRectangle(b, 4.0f);
        }

        if (!compactMode_)
        {
            // Accent top strip
            g.setColour(accentColour);
            g.fillRect(b.removeFromTop(3.0f));

            // Engine name header line
            g.setColour(accentColour.darker(0.2f));
            g.setFont(GalleryFonts::display(11.0f));
            g.drawText(engineName.toUpperCase() + "  —  MACROS", 8, 3, getWidth() - 16, kHeaderH - 3,
                       juce::Justification::centredLeft);

            // Thin separator under header
            g.setColour(juce::Colour(GalleryColors::xoGold).withAlpha(0.45f));
            g.drawHorizontalLine(kHeaderH, 8.0f, (float)(getWidth() - 8));
        }
    }

    // W25: Repaint when the LookAndFeel (theme) changes so colours update.
    void lookAndFeelChanged() override { repaint(); }

    void resized() override
    {
        if (numMacros == 0)
            return;

        const int labelH = 14;
        const int sliderY = compactMode_ ? 18 : (kHeaderH + 4); // 18px: below "MACROS" label
        const int sliderH = getHeight() - sliderY - labelH - 4;
        const int colW = getWidth() / juce::jmax(1, numMacros);

        for (int i = 0; i < numMacros; ++i)
        {
            int x = i * colW;
            pillars[i].setBounds(x + 4, sliderY, colW - 8, juce::jmax(8, sliderH));
            pillarLabels[i].setBounds(x, getHeight() - labelH - 2, colW, labelH);
        }
    }

private:
    static constexpr int kHeaderH = 22;

    XOceanusProcessor& processor;
    juce::String engineName;
    juce::Colour accentColour{GalleryColors::get(GalleryColors::borderGray())};
    int numMacros = 0;
    bool compactMode_ = false;
    MIDILearnManager* learnManager = nullptr;

    std::array<juce::Slider, 4> pillars;
    std::array<juce::Label, 4> pillarLabels;
    // Destruction order: listeners → attachments → sliders.
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> attachments;
    std::vector<std::unique_ptr<MidiLearnMouseListener>> pillarLearnListeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroHeroStrip)
};

} // namespace xoceanus
