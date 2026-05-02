// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOceanusProcessor.h"
#include "../../Core/PresetManager.h"
#include "../PresetBrowser/PresetBrowser.h"
#include "../GalleryColors.h"
#include "../ToastOverlay.h"
#include "MacroSection.h"

namespace xoceanus
{

// PresetBrowser is included at the top of this file (before namespace xoceanus {)
// to avoid the nested-namespace problem.  The class is available here as
// xoceanus::PresetBrowser.
//
// Favorites persistence: reads/writes the same "presetFavorites" key in
// XOceanus.settings as PresetBrowser, using the same canonical preset ID
// format (name + "|" + first engine name).  Changes made via the strip's
// star button are immediately visible in the full PresetBrowser and vice versa
// because both components share the same settings file on disk (#916).

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
        browseBtn.setButtonText(juce::CharPointer_UTF8("\xe2\x8a\x9e")); // ⊞ grid icon
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
            processor.getUndoManager().beginNewTransaction("Load preset: " + preset.name);
            try
            {
                processor.applyPreset(preset);
            }
            catch (const std::exception& e)
            {
                DBG("Preset load failed (prev): " << e.what());
                // #879: show user-visible toast so the failure is not silent
                ToastOverlay::show("Failed to load preset: " + juce::String(e.what()),
                                   Toast::Level::Warn);
                // #894: flush any in-flight crossfade/FX state so the engine is not
                // left in an inconsistent state after a partial parameter application.
                processor.killDelayTails();
            }
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
            processor.getUndoManager().beginNewTransaction("Load preset: " + preset.name);
            try
            {
                processor.applyPreset(preset);
            }
            catch (const std::exception& e)
            {
                DBG("Preset load failed (next): " << e.what());
                // #879: show user-visible toast so the failure is not silent
                ToastOverlay::show("Failed to load preset: " + juce::String(e.what()),
                                   Toast::Level::Warn);
                // #894: flush any in-flight crossfade/FX state so the engine is not
                // left in an inconsistent state after a partial parameter application.
                processor.killDelayTails();
            }
            if (macroSection && !preset.macroLabels.isEmpty())
                macroSection->setLabels(preset.macroLabels);
            if (onPresetLoaded)
                onPresetLoaded();
        };

        browseBtn.onClick = [this] { openBrowser(); };

        // ── Favorite star button (#916) ───────────────────────────────────────
        // Filled ★ (U+2605) when favorited (XO Gold), outline ☆ (U+2606) when not.
        // Persists to the same XOceanus.settings "presetFavorites" key as PresetBrowser
        // so the two views stay in sync.
        favBtn.setColour(juce::TextButton::buttonColourId, GalleryColors::get(GalleryColors::shellWhite()));
        favBtn.setColour(juce::TextButton::textColourOffId, GalleryColors::get(GalleryColors::textMid()));
        favBtn.setTooltip("Toggle favorite");
        A11y::setup(favBtn, "Toggle Favorite", "Mark or unmark the current preset as a favorite");
        favBtn.onClick = [this]
        {
            auto& pm = processor.getPresetManager();
            if (pm.getLibrarySize() == 0)
                return;
            const auto& preset = pm.getCurrentPreset();
            toggleFavoriteInSettings(preset);
            updateFavBtn(preset);
        };
        addAndMakeVisible(favBtn);
        {
            // Open the shared settings file once so favorites can be read immediately.
            juce::PropertiesFile::Options opts;
            opts.applicationName     = "XOceanus";
            opts.filenameSuffix      = "settings";
            opts.osxLibrarySubFolder = "Application Support";
            favSettingsFile = std::make_unique<juce::PropertiesFile>(opts);
        }

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

    /** Toggle favourite state for the current preset and refresh the star button.
        Called by OceanView's favToggleButton onClick (fix #1005). */
    void toggleFavoriteForCurrentPreset()
    {
        auto& pm = processor.getPresetManager();
        if (pm.getLibrarySize() == 0)
            return;
        const auto& preset = pm.getCurrentPreset();
        toggleFavoriteInSettings(preset);
        updateFavBtn(preset);
    }

    // Show / hide the "Loading presets…" state.
    // Called by XOceanusEditor before and after scanPresetDirectoryAsync().
    void setScanning(bool scanning)
    {
        isScanning = scanning;
        prevBtn.setEnabled(!scanning);
        nextBtn.setEnabled(!scanning);
        browseBtn.setEnabled(!scanning);
        favBtn.setEnabled(!scanning);
        if (scanning)
            nameLabel.setText(juce::String(juce::CharPointer_UTF8("Loading presets\xe2\x80\xa6")), juce::dontSendNotification); // "…"
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
        favBtn.setEnabled(hasPresets && !isScanning);

        if (hasPresets)
        {
            const auto& preset = pm.getCurrentPreset();
            auto name = preset.name;
            nameLabel.setText(name.isEmpty() ? "—" : name, juce::dontSendNotification);
            updateFavBtn(preset);
        }
        else
        {
            nameLabel.setText("no presets", juce::dontSendNotification);
            // No preset — show outline star in dim color
            favBtn.setButtonText(juce::String::fromUTF8("\xe2\x98\x86")); // ☆
            favBtn.setColour(juce::TextButton::textColourOffId, GalleryColors::get(GalleryColors::textMid()));
        }
    }

    void resized() override
    {
        auto b = getLocalBounds();
        prevBtn.setBounds(b.removeFromLeft(30));   // UX11: widened from 22px for easier targeting
        browseBtn.setBounds(b.removeFromRight(30));
        nextBtn.setBounds(b.removeFromRight(30));  // UX11: widened from 22px for easier targeting
        favBtn.setBounds(b.removeFromRight(30));   // #916/#1108: star toggle widened to 30px tap floor
        nameLabel.setBounds(b);
    }

    void setMacroSection(MacroSection* ms) { macroSection = ms; }

private:
    void presetLoaded(const PresetData& preset) override
    {
        nameLabel.setText(preset.name, juce::dontSendNotification);
        updateFavBtn(preset);
    }

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
            safeThis->processor.getUndoManager().beginNewTransaction("Load preset: " + preset.name);
            try
            {
                safeThis->processor.applyPreset(preset);
            }
            catch (const std::exception& e)
            {
                DBG("Preset load failed (browser select): " << e.what());
                // #879: show user-visible toast so the failure is not silent
                ToastOverlay::show("Failed to load preset: " + juce::String(e.what()),
                                   Toast::Level::Warn);
                // #894: flush any in-flight crossfade/FX state so the engine is not
                // left in an inconsistent state after a partial parameter application.
                safeThis->processor.killDelayTails();
            }
            safeThis->nameLabel.setText(preset.name, juce::dontSendNotification);
            safeThis->updateFavBtn(preset); // #916: refresh strip star after browser selection
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

    //==========================================================================
    // Favorites helpers (#916) — mirror the same persistence used by PresetBrowser.
    //==========================================================================

    /** Returns the canonical preset ID used by both PresetBrowser and this strip. */
    static juce::String presetId(const PresetData& p)
    {
        juce::String eng = p.engines.isEmpty() ? "" : p.engines[0];
        return p.name + "|" + eng;
    }

    /** Returns true if the preset is currently in the persisted favorites set. */
    bool isFavoriteInSettings(const PresetData& p) const
    {
        if (!favSettingsFile)
            return false;
        juce::String raw = favSettingsFile->getValue("presetFavorites", "");
        if (raw.isEmpty())
            return false;
        juce::StringArray ids;
        ids.addTokens(raw, "|", "");
        return ids.contains(presetId(p));
    }

    /** Toggle favorite status for the preset and persist the change. */
    void toggleFavoriteInSettings(const PresetData& p)
    {
        if (!favSettingsFile)
            return;
        juce::String raw = favSettingsFile->getValue("presetFavorites", "");
        juce::StringArray ids;
        if (raw.isNotEmpty())
            ids.addTokens(raw, "|", "");
        const juce::String id = presetId(p);
        if (ids.contains(id))
            ids.removeString(id);
        else
            ids.add(id);
        favSettingsFile->setValue("presetFavorites", ids.joinIntoString("|"));
        favSettingsFile->saveIfNeeded();
    }

    /** Update the star button visual to reflect the current favorite state. */
    void updateFavBtn(const PresetData& p)
    {
        const bool fav = isFavoriteInSettings(p);
        if (fav)
        {
            // Filled star — XO Gold
            favBtn.setButtonText(juce::String::fromUTF8("\xe2\x98\x85")); // ★
            favBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(GalleryColors::xoGold));
        }
        else
        {
            // Outline star — dim text color
            favBtn.setButtonText(juce::String::fromUTF8("\xe2\x98\x86")); // ☆
            favBtn.setColour(juce::TextButton::textColourOffId, GalleryColors::get(GalleryColors::textMid()));
        }
    }

    XOceanusProcessor& processor;
    NavButtonLookAndFeel navLAF; // Space Grotesk + 0.16em kerning for prev/next
    juce::TextButton prevBtn, nextBtn, browseBtn;
    juce::TextButton favBtn;     // #916: favorite star toggle
    juce::Label nameLabel;
    MacroSection* macroSection = nullptr;
    bool isScanning = false; // true while scanPresetDirectoryAsync() is in flight
    std::unique_ptr<juce::PropertiesFile> favSettingsFile; // #916: shared settings for favorites

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserStrip)
};

} // namespace xoceanus
