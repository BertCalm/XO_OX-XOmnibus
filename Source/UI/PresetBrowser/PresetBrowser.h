// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/PresetManager.h"
#include "../Gallery/GalleryLookAndFeel.h"
// NOTE: GalleryColors, GalleryFonts, A11y, and PresetData are defined in
// XOceanusEditor.h, which includes this file.  The circular include is avoided
// intentionally — those symbols are already in scope when this header is
// compiled as part of XOceanusEditor.h.

namespace xoceanus
{

//==============================================================================
// PresetBrowser — Searchable, filterable browser for 10,028+ factory presets.
//
// Features:
//   - Real-time text search (filters name, tags, description, engine names)
//   - Mood category filter buttons (all 15 moods: Foundation, Atmosphere, Entangled, Prism,
//     Flux, Aether, Family, Submerged, Coupling, Crystalline, Deep, Ethereal, Kinetic,
//     Luminous, Organic)
//   - Scrollable preset list with name, mood, engine tags
//   - DNA-based "Find Similar" button (6D Euclidean distance)
//   - Gallery Model look and feel (warm white shell, XO Gold accents)
//   - Full WCAG 2.1 AA accessibility (focus rings, screen reader labels)
//
// Usage:
//   PresetBrowser browser(presetManager);
//   browser.onPresetSelected = [&](const PresetData& p) { applyPreset(p); };
//   addAndMakeVisible(browser);
//
class PresetBrowser : public juce::Component,
                      public juce::ListBoxModel,
                      public juce::TextEditor::Listener,
                      public juce::Timer
{
public:
    PresetBrowser(PresetManager& pm) : presetManager(pm)
    {
        // --- Search bar ---
        searchBox.setTextToShowWhenEmpty("Search presets...",
                                         GalleryColors::get(GalleryColors::textMid()).withAlpha(0.65f));
        searchBox.setFont(GalleryFonts::body(13.0f));
        searchBox.addListener(this);
        searchBox.setColour(juce::TextEditor::backgroundColourId, GalleryColors::get(GalleryColors::slotBg()));
        searchBox.setColour(juce::TextEditor::outlineColourId, GalleryColors::get(GalleryColors::borderGray()));
        searchBox.setColour(juce::TextEditor::textColourId, GalleryColors::get(GalleryColors::textDark()));
        A11y::setup(searchBox, "Search presets", "Type to filter preset list by name, tag, or engine");
        addAndMakeVisible(searchBox);

        // --- Mood filter buttons ---
        static const char* moods[] = {"All",      "Foundation", "Atmosphere", "Entangled", "Prism",       "Flux",
                                      "Aether",   "Family",     "Submerged",  "Coupling",  "Crystalline", "Deep",
                                      "Ethereal", "Kinetic",    "Luminous",   "Organic"};

        for (auto* mood : moods)
        {
            auto* btn = moodButtons.add(new juce::TextButton(mood));
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(42);
            btn->setColour(juce::TextButton::buttonColourId, GalleryColors::get(GalleryColors::shellWhite()));
            btn->setColour(juce::TextButton::buttonOnColourId, GalleryColors::get(GalleryColors::xoGold));
            btn->setColour(juce::TextButton::textColourOnId, GalleryColors::get(GalleryColors::textDark()));
            btn->setColour(juce::TextButton::textColourOffId, GalleryColors::get(GalleryColors::textMid()));
            btn->onClick = [this]
            {
                similarActive = false;
                applyFilters();
            };
            A11y::setup(*btn, juce::String("Filter: ") + mood, juce::String("Show only ") + mood + " presets");
            GalleryLookAndFeel::setMoodPillStyle(*btn);
            addAndMakeVisible(btn);
        }

        if (moodButtons.size() > 0)
            moodButtons[0]->setToggleState(true, juce::dontSendNotification);

        // --- Preset list ---
        listBox.setModel(this);
        listBox.setRowHeight(36);
        listBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        listBox.setColour(juce::ListBox::outlineColourId, GalleryColors::get(GalleryColors::borderGray()));
        listBox.setOutlineThickness(1);
        A11y::setup(listBox, "Preset list", "Scrollable list of presets — click to load");
        addAndMakeVisible(listBox);

        // --- Similar button (DNA distance) ---
        similarBtn.setButtonText("Find Similar");
        similarBtn.setColour(juce::TextButton::buttonColourId, GalleryColors::get(GalleryColors::slotBg()));
        similarBtn.setColour(juce::TextButton::textColourOffId, GalleryColors::get(GalleryColors::textMid()));
        similarBtn.onClick = [this] { findSimilar(); };
        A11y::setup(similarBtn, "Find Similar", "Find presets with similar sonic DNA to the selected preset");
        addAndMakeVisible(similarBtn);

        // --- Status label ---
        statusLabel.setFont(GalleryFonts::label(11.0f));
        statusLabel.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()));
        statusLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(statusLabel);

        setTitle("Preset Browser");
        setDescription("Browse, search, and filter XOceanus presets by mood and sonic DNA");
        setWantsKeyboardFocus(true);

        applyFilters();
    }

    //==========================================================================
    // Callback: preset selected (set by parent)
    //==========================================================================

    std::function<void(const PresetData&)> onPresetSelected;

    //==========================================================================
    // Accessibility
    //==========================================================================

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return std::make_unique<juce::AccessibilityHandler>(
            *this, juce::AccessibilityRole::group,
            juce::AccessibilityActions{}.addAction(juce::AccessibilityActionType::focus,
                                                   [this] { searchBox.grabKeyboardFocus(); }));
    }

    //==========================================================================
    // Layout
    //==========================================================================

    void resized() override
    {
        auto area = getLocalBounds().reduced(8);

        // Search bar
        searchBox.setBounds(area.removeFromTop(32));
        area.removeFromTop(6);

        // Mood pills — flex-wrap layout
        {
            const int pillH = 22;
            const int hGap = 4;
            const int vGap = 4;
            int px = area.getX();
            int py = area.getY();
            const int maxX = area.getRight();

            for (auto* btn : moodButtons)
            {
                int pillW = juce::jmax(
                    36, juce::roundToInt(GalleryFonts::body(9.0f).getStringWidthFloat(btn->getButtonText())) + 20);
                if (px + pillW > maxX && btn != moodButtons.getFirst())
                {
                    px = area.getX();
                    py += pillH + vGap;
                }
                btn->setBounds(px, py, pillW, pillH);
                px += pillW + hGap;
            }
            int moodHeight = py - area.getY() + pillH;
            area.removeFromTop(moodHeight);
        }
        area.removeFromTop(6);

        // Bottom bar: status + similar button
        auto bottomBar = area.removeFromBottom(32);
        similarBtn.setBounds(bottomBar.removeFromRight(120).reduced(0, 2));
        statusLabel.setBounds(bottomBar);

        // List fills remaining space
        listBox.setBounds(area);
    }

    void paint(juce::Graphics& g) override { g.fillAll(GalleryColors::get(GalleryColors::shellWhite())); }

    //==========================================================================
    // ListBoxModel implementation
    //==========================================================================

    int getNumRows() override { return static_cast<int>(filteredPresets.size()); }

    juce::String getNameForRow(int row) override
    {
        if (row < 0 || row >= static_cast<int>(filteredPresets.size()))
            return {};
        const auto& p = filteredPresets[static_cast<size_t>(row)];
        return p.name + (p.mood.isEmpty() ? "" : ", " + p.mood);
    }

    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool isSelected) override
    {
        if (row < 0 || row >= static_cast<int>(filteredPresets.size()))
            return;

        const auto& preset = filteredPresets[static_cast<size_t>(row)];

        // Row background
        if (isSelected)
        {
            g.fillAll(juce::Colour(0x0BFFFFFF)); // rgba(255,255,255,0.045)
            // 2px accent left border on selected row
            g.setColour(juce::Colour(0xFF1E8B7E));
            g.fillRect(0, 0, 2, h);
        }
        else if (row % 2 != 0)
            g.fillAll(juce::Colour(0x05FFFFFF)); // very subtle zebra

        // Mood accent dot (5px to match prototype)
        g.setColour(moodColour(preset.mood).withAlpha(0.75f));
        g.fillEllipse(10.0f, h * 0.5f - 2.5f, 5.0f, 5.0f);

        // Preset name — Inter 11.5px
        g.setFont(GalleryFonts::body(11.5f));
        g.setColour(GalleryColors::get(GalleryColors::textDark()));
        g.drawText(preset.name, 22, 0, w - 34, h, juce::Justification::centredLeft, true);

        // Engine tag badge — JetBrains Mono 9px, right-aligned
        g.setFont(GalleryFonts::value(9.0f));
        g.setColour(GalleryColors::get(GalleryColors::textMid()));

        juce::String meta = preset.mood;
        for (const auto& eng : preset.engines)
        {
            if (eng.isNotEmpty())
                meta += juce::String::fromUTF8("  \xc2\xb7  ") + eng; // middle dot separator
        }
        g.drawText(meta, 22, h / 2, w - 34, h / 2, juce::Justification::centredLeft, true);

        // Bottom separator
        g.setColour(GalleryColors::get(GalleryColors::borderGray()).withAlpha(0.25f));
        g.drawHorizontalLine(h - 1, 10.0f, static_cast<float>(w) - 10.0f);
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override
    {
        if (row >= 0 && row < static_cast<int>(filteredPresets.size()))
        {
            selectedIndex = row;
            if (onPresetSelected)
                onPresetSelected(filteredPresets[static_cast<size_t>(row)]);
        }
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
    {
        // Single-click now loads; double-click is a no-op.
        (void)row;
    }

    void returnKeyPressed(int lastRowSelected) override
    {
        if (lastRowSelected >= 0 && lastRowSelected < static_cast<int>(filteredPresets.size()))
        {
            selectedIndex = lastRowSelected;
            if (onPresetSelected)
                onPresetSelected(filteredPresets[static_cast<size_t>(lastRowSelected)]);
        }
    }

    //==========================================================================
    // Keyboard navigation
    //==========================================================================

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::returnKey)
        {
            if (selectedIndex >= 0 && selectedIndex < static_cast<int>(filteredPresets.size()))
            {
                if (onPresetSelected)
                    onPresetSelected(filteredPresets[static_cast<size_t>(selectedIndex)]);
            }
            return true;
        }
        if (key == juce::KeyPress::escapeKey)
        {
            searchBox.clear();
            similarActive = false;
            applyFilters();
            return true;
        }
        return false;
    }

    //==========================================================================
    // TextEditor::Listener
    //==========================================================================

    void textEditorTextChanged(juce::TextEditor&) override
    {
        similarActive = false;
        startTimer(150); // debounce — timerCallback calls applyFilters()
    }

    // juce::Timer
    void timerCallback() override
    {
        stopTimer();
        juce::Component::SafePointer<PresetBrowser> safe(this);
        if (safe != nullptr)
            safe->applyFilters();
    }

    //==========================================================================
    // Public API
    //==========================================================================

    /** Refresh the displayed list (call after preset library changes). */
    void refresh() { applyFilters(); }

private:
    PresetManager& presetManager;
    juce::TextEditor searchBox;
    juce::OwnedArray<juce::TextButton> moodButtons;
    juce::ListBox listBox{"PresetList", nullptr};
    juce::TextButton similarBtn;
    juce::Label statusLabel;

    std::vector<PresetData> filteredPresets;
    int selectedIndex = -1;
    bool similarActive = false;

    //==========================================================================
    // Filtering
    //==========================================================================

    void applyFilters()
    {
        juce::String searchText = searchBox.getText().trim().toLowerCase();

        // Get selected mood
        juce::String selectedMood;
        for (auto* btn : moodButtons)
        {
            if (btn->getToggleState())
            {
                selectedMood = btn->getButtonText();
                break;
            }
        }

        filteredPresets.clear();
        const auto& allPresets = presetManager.getLibrary();

        for (const auto& preset : allPresets)
        {
            // Mood filter (skip if "All" is selected)
            if (selectedMood.isNotEmpty() && selectedMood != "All")
            {
                if (preset.mood.compareIgnoreCase(selectedMood) != 0)
                    continue;
            }

            // Text search filter — matches name, description, tags, and engines
            if (searchText.isNotEmpty())
            {
                bool matches = false;
                if (preset.name.toLowerCase().contains(searchText))
                    matches = true;
                else if (preset.description.toLowerCase().contains(searchText))
                    matches = true;
                else
                {
                    for (const auto& tag : preset.tags)
                        if (tag.toLowerCase().contains(searchText))
                        {
                            matches = true;
                            break;
                        }
                }
                if (!matches)
                {
                    for (const auto& eng : preset.engines)
                        if (eng.toLowerCase().contains(searchText))
                        {
                            matches = true;
                            break;
                        }
                }

                if (!matches)
                    continue;
            }

            filteredPresets.push_back(preset);
        }

        // Sort alphabetically by name
        std::sort(filteredPresets.begin(), filteredPresets.end(),
                  [](const PresetData& a, const PresetData& b) { return a.name.compareIgnoreCase(b.name) < 0; });

        listBox.updateContent();
        listBox.deselectAllRows();
        selectedIndex = -1;

        juce::String statusText =
            juce::String(filteredPresets.size()) + " / " + juce::String(allPresets.size()) + " presets";
        if (similarActive)
            statusText = "Similar: " + statusText;
        statusLabel.setText(statusText, juce::dontSendNotification);
    }

    //==========================================================================
    // Find Similar (DNA distance)
    //==========================================================================

    void findSimilar()
    {
        if (selectedIndex < 0 || selectedIndex >= static_cast<int>(filteredPresets.size()))
            return;

        const auto& selected = filteredPresets[static_cast<size_t>(selectedIndex)];
        auto similar = presetManager.findSimilar(selected.dna, 20);

        filteredPresets.clear();
        for (auto& preset : similar)
            filteredPresets.push_back(std::move(preset));

        similarActive = true;

        // Reset mood to "All"
        if (moodButtons.size() > 0)
            moodButtons[0]->setToggleState(true, juce::dontSendNotification);
        searchBox.clear();

        listBox.updateContent();
        listBox.deselectAllRows();
        selectedIndex = -1;

        statusLabel.setText("Similar to: " + selected.name + " (" + juce::String(filteredPresets.size()) + " results)",
                            juce::dontSendNotification);
    }

    //==========================================================================
    // Mood colour mapping
    //==========================================================================

    static juce::Colour moodColour(const juce::String& mood)
    {
        if (mood == "Foundation")
            return juce::Colour(0xFF00A6D6); // Neon Tetra Blue
        if (mood == "Atmosphere")
            return juce::Colour(0xFFE8839B); // Axolotl Gill Pink
        if (mood == "Entangled")
            return juce::Colour(0xFF7B2D8B); // Violet
        if (mood == "Prism")
            return juce::Colour(0xFF0066FF); // Electric Blue
        if (mood == "Flux")
            return juce::Colour(0xFFE9A84A); // Amber
        if (mood == "Aether")
            return juce::Colour(0xFFA78BFA); // Lavender
        if (mood == "Family")
            return juce::Colour(0xFFE9C46A); // XO Gold
        if (mood == "Submerged")
            return juce::Colour(0xFF2D0A4E); // Trench Violet
        if (mood == "Coupling")
            return juce::Colour(0xFF1A6B5A); // Oxbow Teal
        if (mood == "Crystalline")
            return juce::Colour(0xFFA8D8EA); // Spectral Ice
        if (mood == "Deep")
            return juce::Colour(0xFF003366); // Synth Bass Blue
        if (mood == "Ethereal")
            return juce::Colour(0xFF9B5DE5); // Synapse Violet
        if (mood == "Kinetic")
            return juce::Colour(0xFFE5B80B); // Crate Wax Yellow
        if (mood == "Luminous")
            return juce::Colour(0xFFC6E377); // Emergence Lime
        if (mood == "Organic")
            return juce::Colour(0xFF228B22); // Forest Green
        return juce::Colour(GalleryColors::borderGray());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};

} // namespace xoceanus
