#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../../Core/PresetManager.h"
#include "../PresetBrowser/PresetBrowser.h"
#include "../GalleryColors.h"
#include "MacroSection.h"

namespace xolokun {

// PresetBrowser is included at the top of this file (before namespace xolokun {)
// to avoid the nested-namespace problem.  The class is available here as
// xolokun::PresetBrowser.

//==============================================================================
// PresetBrowserStrip — prev/next navigation + preset name display.
// Lives in the editor header. Calls processor.applyPreset() on navigation.
class PresetBrowserStrip : public juce::Component,
                           private PresetManager::Listener
{
public:
    // Fires on the message thread after any preset is applied (prev, next, or
    // browser selection). The editor uses this to notify ABCompare.
    std::function<void()> onPresetLoaded;

    PresetBrowserStrip(XOlokunProcessor& proc)
        : processor(proc)
    {
        prevBtn.setButtonText("<");
        nextBtn.setButtonText(">");
        browseBtn.setButtonText("\xe2\x8a\x9e"); // ⊞ grid icon (UTF-8)
        prevBtn.setTooltip("Previous preset");
        nextBtn.setTooltip("Next preset");
        browseBtn.setTooltip("Browse all presets by mood");
        A11y::setup (prevBtn, "Previous Preset");
        A11y::setup (nextBtn, "Next Preset");
        A11y::setup (browseBtn, "Browse Presets", "Open preset browser by mood category");

        for (auto* btn : {&prevBtn, &nextBtn, &browseBtn})
        {
            btn->setColour(juce::TextButton::buttonColourId,
                           GalleryColors::get(GalleryColors::shellWhite()));
            btn->setColour(juce::TextButton::textColourOffId,
                           GalleryColors::get(GalleryColors::textMid()));
            addAndMakeVisible(*btn);
        }

        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setFont(GalleryFonts::heading(10.5f));
        nameLabel.setColour(juce::Label::textColourId,
                            GalleryColors::get(GalleryColors::textDark()));
        nameLabel.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(nameLabel);

        prevBtn.onClick = [this]
        {
            auto& pm = processor.getPresetManager();
            pm.previousPreset();
            const auto& preset = pm.getCurrentPreset();
            processor.applyPreset(preset);
            if (macroSection && !preset.macroLabels.isEmpty())
                macroSection->setLabels(preset.macroLabels);
            if (onPresetLoaded) onPresetLoaded();
        };

        nextBtn.onClick = [this]
        {
            auto& pm = processor.getPresetManager();
            pm.nextPreset();
            const auto& preset = pm.getCurrentPreset();
            processor.applyPreset(preset);
            if (macroSection && !preset.macroLabels.isEmpty())
                macroSection->setLabels(preset.macroLabels);
            if (onPresetLoaded) onPresetLoaded();
        };

        browseBtn.onClick = [this] { openBrowser(); };

        processor.getPresetManager().addListener(this);
        updateDisplay();
    }

    ~PresetBrowserStrip() override
    {
        processor.getPresetManager().removeListener(this);
    }

    void updateDisplay()
    {
        auto& pm = processor.getPresetManager();
        int total = pm.getLibrarySize();
        bool hasPresets = total > 0;
        prevBtn.setEnabled(hasPresets);
        nextBtn.setEnabled(hasPresets);

        if (hasPresets)
        {
            auto name = pm.getCurrentPreset().name;
            nameLabel.setText(name.isEmpty() ? "—" : name,
                              juce::dontSendNotification);
        }
        else
        {
            nameLabel.setText("no presets", juce::dontSendNotification);
        }
    }

    void resized() override
    {
        auto b = getLocalBounds();
        prevBtn.setBounds(b.removeFromLeft(22));
        browseBtn.setBounds(b.removeFromRight(30));
        nextBtn.setBounds(b.removeFromRight(22));
        nameLabel.setBounds(b);
    }

    void setMacroSection(MacroSection* ms) { macroSection = ms; }

private:
    void presetLoaded(const PresetData& preset) override
    {
        nameLabel.setText(preset.name, juce::dontSendNotification);
    }

    void openBrowser()
    {
        auto& pm = processor.getPresetManager();

        // SafePointer becomes null if this component is destroyed while the
        // CallOutBox is still open (e.g., plugin editor closed during browse).
        juce::Component::SafePointer<PresetBrowserStrip> safeThis(this);

        // Build the full PresetBrowser (search + DNA + 48px rows).
        auto browser = std::make_unique<PresetBrowser>(pm);

        // Wire the onPresetSelected callback — same logic as the old panel.
        browser->onPresetSelected = [safeThis, &proc = processor](const PresetData& preset)
        {
            proc.getPresetManager().setCurrentPreset(preset);
            proc.applyPreset(preset);
            // Only touch UI members if the strip is still alive.
            if (safeThis != nullptr)
            {
                safeThis->nameLabel.setText(preset.name, juce::dontSendNotification);
                if (safeThis->macroSection && !preset.macroLabels.isEmpty())
                    safeThis->macroSection->setLabels(preset.macroLabels);
                if (safeThis->onPresetLoaded) safeThis->onPresetLoaded();
            }
        };

        // Size: 560 wide × 520 tall — accommodates 48px rows, search bar,
        // mood filter strip, and the "Find Similar" DNA button at the bottom.
        browser->setSize(560, 520);

        juce::CallOutBox::launchAsynchronously(
            std::move(browser),
            browseBtn.getScreenBounds(),
            getTopLevelComponent());
    }

    XOlokunProcessor& processor;
    juce::TextButton prevBtn, nextBtn, browseBtn;
    juce::Label nameLabel;
    MacroSection* macroSection = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserStrip)
};

} // namespace xolokun
