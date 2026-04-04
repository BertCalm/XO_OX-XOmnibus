// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOceanusProcessor.h"
#include "../../Core/PresetManager.h"
#include "../PresetBrowser/PresetBrowser.h"
#include "../GalleryColors.h"
#include "MacroSection.h"

namespace xoceanus
{

// PresetBrowser is included at the top of this file (before namespace xoceanus {)
// to avoid the nested-namespace problem.  The class is available here as
// xoceanus::PresetBrowser.

//==============================================================================
// NavButtonLookAndFeel — Space Grotesk SemiBold (Bold) with 0.16em letter-spacing
// for the prev/next preset navigation buttons.
class NavButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonText(juce::Graphics& g, juce::TextButton& btn, bool /*isMouseOver*/, bool /*isButtonDown*/) override
    {
        // Space Grotesk Bold at the button's height (clamped to 11–13px range)
        const float fontSize = juce::jlimit(11.0f, 13.0f, (float)btn.getHeight() * 0.55f);
        auto f = GalleryFonts::display(fontSize);
        // setExtraKerningFactor maps 1.0 → 100% extra gap per character;
        // 0.16 ≈ 0.16em matches the CSS letter-spacing spec.
        f = f.withExtraKerningFactor(0.16f);
        g.setFont(f);
        g.setColour(
            btn.findColour(btn.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId)
                .withMultipliedAlpha(btn.isEnabled() ? 1.0f : 0.5f));
        g.drawFittedText(btn.getButtonText(), btn.getLocalBounds(), juce::Justification::centred, 1, 1.0f);
    }
};

//==============================================================================
// PresetBrowserStrip — prev/next navigation + preset name display.
// Lives in the editor header. Calls processor.applyPreset() on navigation.
class PresetBrowserStrip : public juce::Component, private PresetManager::Listener
{
public:
    // Fires on the message thread after any preset is applied (prev, next, or
    // browser selection). The editor uses this to notify ABCompare.
    std::function<void()> onPresetLoaded;

    PresetBrowserStrip(XOceanusProcessor& proc) : processor(proc)
    {
        prevBtn.setButtonText("<");
        nextBtn.setButtonText(">");
        browseBtn.setButtonText("\xe2\x8a\x9e"); // ⊞ grid icon (UTF-8)
        prevBtn.setTooltip("Previous preset");
        nextBtn.setTooltip("Next preset");
        browseBtn.setTooltip("Browse all presets by mood");
        A11y::setup(prevBtn, "Previous Preset");
        A11y::setup(nextBtn, "Next Preset");
        A11y::setup(browseBtn, "Browse Presets", "Open preset browser by mood category");

        // Apply Space Grotesk SemiBold + 0.16em letter-spacing to nav buttons
        prevBtn.setLookAndFeel(&navLAF);
        nextBtn.setLookAndFeel(&navLAF);

        for (auto* btn : {&prevBtn, &nextBtn, &browseBtn})
        {
            btn->setColour(juce::TextButton::buttonColourId, GalleryColors::get(GalleryColors::shellWhite()));
            btn->setColour(juce::TextButton::textColourOffId, GalleryColors::get(GalleryColors::textMid()));
            addAndMakeVisible(*btn);
        }

        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setFont(GalleryFonts::body(12.5f));
        nameLabel.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textDark()));
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
            if (onPresetLoaded)
                onPresetLoaded();
        };

        nextBtn.onClick = [this]
        {
            auto& pm = processor.getPresetManager();
            pm.nextPreset();
            const auto& preset = pm.getCurrentPreset();
            processor.applyPreset(preset);
            if (macroSection && !preset.macroLabels.isEmpty())
                macroSection->setLabels(preset.macroLabels);
            if (onPresetLoaded)
                onPresetLoaded();
        };

        browseBtn.onClick = [this] { openBrowser(); };

        processor.getPresetManager().addListener(this);
        updateDisplay();
    }

    ~PresetBrowserStrip() override
    {
        // JUCE requires LookAndFeel to be cleared before the owning component
        // is destroyed — prevents dangling pointer in the LAF dispatch chain.
        prevBtn.setLookAndFeel(nullptr);
        nextBtn.setLookAndFeel(nullptr);
        processor.getPresetManager().removeListener(this);
    }

    // Show / hide the "Loading presets…" state.
    // Called by XOceanusEditor before and after scanPresetDirectoryAsync().
    void setScanning(bool scanning)
    {
        isScanning = scanning;
        prevBtn.setEnabled(!scanning);
        nextBtn.setEnabled(!scanning);
        browseBtn.setEnabled(!scanning);
        if (scanning)
            nameLabel.setText("Loading presets\xe2\x80\xa6", juce::dontSendNotification); // "…"
        else
            updateDisplay();
    }

    void updateDisplay()
    {
        auto& pm = processor.getPresetManager();
        int total = pm.getLibrarySize();
        bool hasPresets = total > 0;
        prevBtn.setEnabled(hasPresets && !isScanning);
        nextBtn.setEnabled(hasPresets && !isScanning);
        browseBtn.setEnabled(!isScanning);

        if (hasPresets)
        {
            auto name = pm.getCurrentPreset().name;
            nameLabel.setText(name.isEmpty() ? "—" : name, juce::dontSendNotification);
        }
        else
        {
            nameLabel.setText("no presets", juce::dontSendNotification);
        }
    }

    void resized() override
    {
        auto b = getLocalBounds();
        prevBtn.setBounds(b.removeFromLeft(30)); // UX11: widened from 22px for easier targeting
        browseBtn.setBounds(b.removeFromRight(30));
        nextBtn.setBounds(b.removeFromRight(30)); // UX11: widened from 22px for easier targeting
        nameLabel.setBounds(b);
    }

    void setMacroSection(MacroSection* ms) { macroSection = ms; }

private:
    void presetLoaded(const PresetData& preset) override { nameLabel.setText(preset.name, juce::dontSendNotification); }

    void openBrowser()
    {
        auto& pm = processor.getPresetManager();

        // SafePointer becomes null if this component is destroyed while the
        // CallOutBox is still open (e.g., plugin editor closed during browse).
        juce::Component::SafePointer<PresetBrowserStrip> safeThis(this);

        // Build the full PresetBrowser (search + DNA + 48px rows).
        auto browser = std::make_unique<PresetBrowser>(pm);

        // CQ13: removed &proc capture — dangling if strip is destroyed while CallOutBox is open.
        // All processor access now guarded through safeThis so the lambda is safe after strip death.
        browser->onPresetSelected = [safeThis](const PresetData& preset)
        {
            if (safeThis == nullptr)
                return;
            safeThis->processor.getPresetManager().setCurrentPreset(preset);
            safeThis->processor.applyPreset(preset);
            safeThis->nameLabel.setText(preset.name, juce::dontSendNotification);
            if (safeThis->macroSection && !preset.macroLabels.isEmpty())
                safeThis->macroSection->setLabels(preset.macroLabels);
            if (safeThis->onPresetLoaded)
                safeThis->onPresetLoaded();
        };

        // Size: 560 wide × 520 tall — accommodates 48px rows, search bar,
        // mood filter strip, and the "Find Similar" DNA button at the bottom.
        browser->setSize(560, 520);

        juce::CallOutBox::launchAsynchronously(std::move(browser), browseBtn.getScreenBounds(), getTopLevelComponent());
    }

    XOceanusProcessor& processor;
    NavButtonLookAndFeel navLAF; // Space Grotesk + 0.16em kerning for prev/next
    juce::TextButton prevBtn, nextBtn, browseBtn;
    juce::Label nameLabel;
    MacroSection* macroSection = nullptr;
    bool isScanning = false; // true while scanPresetDirectoryAsync() is in flight

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserStrip)
};

} // namespace xoceanus
