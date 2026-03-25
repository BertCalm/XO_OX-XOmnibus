#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Core/PresetManager.h"
#include "../GalleryColors.h"

namespace xolokun {

//==============================================================================
// PresetBrowserPanel — full preset browser shown in a CallOutBox.
// Provides mood tabs + name search + scrollable list.
// Calls onPresetSelected callback when the user clicks a preset row.
class PresetBrowserPanel : public juce::Component,
                           public juce::ListBoxModel
{
public:
    PresetBrowserPanel(const PresetManager& pm,
                       std::function<void(const PresetData&)> onSelect)
        : presetManager(pm), onPresetSelected(std::move(onSelect))
    {
        // Search field
        searchField.setTextToShowWhenEmpty("Search presets\xe2\x80\xa6",
                                           GalleryColors::get(GalleryColors::textMid()).withAlpha(0.4f));
        searchField.setColour(juce::TextEditor::backgroundColourId,
                              GalleryColors::get(GalleryColors::slotBg()));
        searchField.setColour(juce::TextEditor::outlineColourId,
                              GalleryColors::get(GalleryColors::borderGray()));
        searchField.setColour(juce::TextEditor::textColourId,
                              GalleryColors::get(GalleryColors::textDark()));
        searchField.setFont(GalleryFonts::body(11.0f));
        searchField.onTextChange = [this] { updateFilter(); };
        addAndMakeVisible(searchField);

        // Mood filter buttons (ALL = index 0, then 15 moods)
        static const char* moodLabels[] = {
            "ALL", "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family",
            "Submerged", "Coupling", "Crystalline", "Deep", "Ethereal", "Kinetic", "Luminous", "Organic"
        };
        for (int i = 0; i < kNumMoods; ++i)
        {
            moodBtns[i].setButtonText(moodLabels[i]);
            moodBtns[i].setClickingTogglesState(false);
            moodBtns[i].setColour(juce::TextButton::buttonColourId,
                                  GalleryColors::get(GalleryColors::shellWhite()));
            moodBtns[i].setColour(juce::TextButton::textColourOffId,
                                  GalleryColors::get(GalleryColors::textMid()));
            moodBtns[i].setColour(juce::TextButton::buttonOnColourId,
                                  GalleryColors::get(GalleryColors::xoGold).withAlpha(0.2f));
            moodBtns[i].onClick = [this, i]
            {
                activeMood = i;
                for (int j = 0; j < kNumMoods; ++j)
                    moodBtns[j].setToggleState(j == i, juce::dontSendNotification);
                updateFilter();
            };
            addAndMakeVisible(moodBtns[i]);
        }
        moodBtns[0].setToggleState(true, juce::dontSendNotification);

        // Preset list
        listBox.setModel(this);
        listBox.setRowHeight(24);
        listBox.setColour(juce::ListBox::backgroundColourId,
                          GalleryColors::get(GalleryColors::slotBg()));
        listBox.setColour(juce::ListBox::outlineColourId,
                          GalleryColors::get(GalleryColors::borderGray()));
        listBox.setOutlineThickness(1);
        addAndMakeVisible(listBox);

        // Count label
        countLabel.setFont(GalleryFonts::label(8.5f));
        countLabel.setColour(juce::Label::textColourId,
                             GalleryColors::get(GalleryColors::textMid()).withAlpha(0.55f));
        countLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(countLabel);

        updateFilter();
        setSize(380, 340);
    }

    // juce::ListBoxModel interface
    int getNumRows() override { return (int)filtered.size(); }

    void paintListBoxItem(int row, juce::Graphics& g,
                          int w, int h, bool selected) override
    {
        if (row < 0 || row >= (int)filtered.size())
            return;

        const auto& preset = filtered[static_cast<size_t>(row)];
        using namespace GalleryColors;

        if (selected)
            g.fillAll(get(xoGold).withAlpha(0.22f));
        else if (row % 2 == 0)
            g.fillAll(get(shellWhite()));
        else
            g.fillAll(get(slotBg()));

        // Mood accent dot
        static const juce::Colour moodColors[] = {
            juce::Colour(0xFF00A6D6), // Foundation  → OddfeliX/Neon Tetra Blue
            juce::Colour(0xFFE8839B), // Atmosphere  → OddOscar/Axolotl Gill Pink
            juce::Colour(0xFF7B2D8B), // Entangled   → Drift/Violet
            juce::Colour(0xFF0066FF), // Prism       → Onset/Blue
            juce::Colour(0xFFE9A84A), // Flux        → Bob/Amber
            juce::Colour(0xFFA78BFA), // Aether      → Opal/Lavender
            juce::Colour(0xFFFF8A7A), // Family      → Rascal Coral
            juce::Colour(0xFF2D0A4E), // Submerged   → Trench Violet
            juce::Colour(0xFF1A6B5A), // Coupling    → Oxbow Teal
            juce::Colour(0xFFA8D8EA), // Crystalline → Spectral Ice
            juce::Colour(0xFF003366), // Deep        → Synth Bass Blue
            juce::Colour(0xFF9B5DE5), // Ethereal    → Synapse Violet
            juce::Colour(0xFFE5B80B), // Kinetic     → Crate Wax Yellow
            juce::Colour(0xFFC6E377), // Luminous    → Emergence Lime
            juce::Colour(0xFF228B22), // Organic     → Forest Green
        };
        static const char* moodIds[] = {
            "Foundation", "Atmosphere", "Entangled",  "Prism",   "Flux",     "Aether",   "Family",  "Submerged",
            "Coupling",   "Crystalline","Deep",        "Ethereal","Kinetic",  "Luminous", "Organic"
        };
        juce::Colour dot = get(borderGray());
        for (int mi = 0; mi < 15; ++mi)
            if (preset.mood == moodIds[mi]) { dot = moodColors[mi]; break; }

        g.setColour(dot.withAlpha(0.7f));
        g.fillEllipse(8.0f, h * 0.5f - 3.5f, 7.0f, 7.0f);

        // Preset name
        g.setColour(get(selected ? textDark() : textMid()));
        g.setFont(GalleryFonts::body(10.5f));
        g.drawText(preset.name, 22, 0, w - 36, h,
                   juce::Justification::centredLeft, true);

        // Engine tag if multi-engine
        if (!preset.engines.isEmpty() && preset.engines[0].isNotEmpty())
        {
            auto tag = preset.engines[0].substring(0, juce::jmin(3, preset.engines[0].length())).toUpperCase();
            g.setColour(get(textMid()).withAlpha(0.50f));
            g.setFont(GalleryFonts::label(7.5f));
            g.drawText(tag, w - 28, 0, 26, h, juce::Justification::centredRight);
        }
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
    {
        selectRow(row);
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override
    {
        listBox.selectRow(row);
    }

    void selectedRowsChanged(int) override
    {
        int row = listBox.getSelectedRow();
        selectRow(row);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll(get(shellWhite()));
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(8, 6);

        // Search field row
        auto searchRow = b.removeFromTop(28);
        countLabel.setBounds(searchRow.removeFromRight(56));
        searchField.setBounds(searchRow.reduced(0, 2));

        b.removeFromTop(4);

        // Mood tabs row
        auto moodRow = b.removeFromTop(24);
        int bw = moodRow.getWidth() / kNumMoods;
        for (int i = 0; i < kNumMoods; ++i)
            moodBtns[i].setBounds(moodRow.removeFromLeft(bw).reduced(1, 0));

        b.removeFromTop(4);

        // Preset list
        listBox.setBounds(b);
    }

private:
    void selectRow(int row)
    {
        if (row >= 0 && row < (int)filtered.size() && onPresetSelected)
            onPresetSelected(filtered[static_cast<size_t>(row)]);
    }

    void updateFilter()
    {
        static const char* moodNames[] = {
            "", "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family",
            "Submerged", "Coupling", "Crystalline", "Deep", "Ethereal", "Kinetic", "Luminous", "Organic"
        };

        auto query = searchField.getText().trim();

        // Start with mood filter (index 0 = ALL = no filter)
        const auto& lib = presetManager.getLibrary();
        filtered.clear();

        for (const auto& p : lib)
        {
            bool moodMatch = (activeMood == 0) || (p.mood == moodNames[activeMood]);
            bool nameMatch = query.isEmpty() || p.name.containsIgnoreCase(query);
            if (moodMatch && nameMatch)
                filtered.push_back(p);
        }

        // Sort alphabetically within current filter
        std::sort(filtered.begin(), filtered.end(),
                  [](const PresetData& a, const PresetData& b) {
                      return a.name.compareIgnoreCase(b.name) < 0;
                  });

        listBox.updateContent();
        listBox.deselectAllRows();

        countLabel.setText(juce::String(filtered.size()) + " presets",
                           juce::dontSendNotification);
    }

    static constexpr int kNumMoods = 16; // ALL + 15 moods

    const PresetManager& presetManager;
    std::function<void(const PresetData&)> onPresetSelected;

    juce::TextEditor searchField;
    juce::TextButton moodBtns[kNumMoods];
    juce::ListBox listBox;
    juce::Label countLabel;
    std::vector<PresetData> filtered;
    int activeMood = 0; // 0 = ALL

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserPanel)
};

} // namespace xolokun
