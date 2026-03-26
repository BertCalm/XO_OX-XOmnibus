#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Core/PresetManager.h"
#include "../GalleryColors.h"
#include "GalleryLookAndFeel.h"

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
        searchField.setTextToShowWhenEmpty("Search presets...",
                                           GalleryColors::get(GalleryColors::textMid()).withAlpha(0.4f));
        // Prototype: elevated bg, border, T1 text
        searchField.setColour(juce::TextEditor::backgroundColourId,
                              GalleryColors::get(GalleryColors::elevated()));
        searchField.setColour(juce::TextEditor::outlineColourId,
                              GalleryColors::border());
        searchField.setColour(juce::TextEditor::textColourId,
                              GalleryColors::get(GalleryColors::t1()));
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
            // Prototype: transparent default bg, t3 text, gold-dim active bg + gold text
            moodBtns[i].setColour(juce::TextButton::buttonColourId,
                                  juce::Colours::transparentBlack);
            moodBtns[i].setColour(juce::TextButton::textColourOffId,
                                  GalleryColors::get(GalleryColors::t3()));
            moodBtns[i].setColour(juce::TextButton::textColourOnId,
                                  GalleryColors::get(GalleryColors::xoGold));
            moodBtns[i].setColour(juce::TextButton::buttonOnColourId,
                                  GalleryColors::get(GalleryColors::xoGold).withAlpha(0.14f));
            moodBtns[i].onClick = [this, i]
            {
                activeMood = i;
                for (int j = 0; j < kNumMoods; ++j)
                    moodBtns[j].setToggleState(j == i, juce::dontSendNotification);
                updateFilter();
            };
            GalleryLookAndFeel::setMoodPillStyle(moodBtns[i]);
            addAndMakeVisible(moodBtns[i]);
        }
        moodBtns[0].setToggleState(true, juce::dontSendNotification);

        // Preset list
        listBox.setModel(this);
        listBox.setRowHeight(32); // Prototype: ~32px rows (7px padding + content)
        // Prototype: transparent bg (parent paints bg), subtle border
        listBox.setColour(juce::ListBox::backgroundColourId,
                          juce::Colours::transparentBlack);
        listBox.setColour(juce::ListBox::outlineColourId,
                          GalleryColors::border());
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
        {
            g.fillAll(juce::Colour(0x0BFFFFFF)); // rgba(255,255,255,0.045)
            // 2px accent left border on selected row
            g.setColour(juce::Colour(0xFF1E8B7E)); // default accent — overridden per-engine if available
            g.fillRect(0, 0, 2, h);
        }

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

        // Prototype: 5×5px mood pip
        g.setColour(dot.withAlpha(0.7f));
        g.fillEllipse(10.0f, h * 0.5f - 2.5f, 5.0f, 5.0f);

        // Preset name
        g.setColour(get(selected ? t1() : t2()));
        g.setFont(GalleryFonts::body(11.5f)); // Prototype: Inter 11.5px
        g.drawText(preset.name, 22, 0, w - 36, h,
                   juce::Justification::centredLeft, true);

        // Engine tag if multi-engine
        if (!preset.engines.isEmpty() && preset.engines[0].isNotEmpty())
        {
            auto tag = preset.engines[0].substring(0, juce::jmin(3, preset.engines[0].length())).toUpperCase();
            // Prototype: JetBrains Mono 8px for engine badge
            g.setColour(get(t3()).withAlpha(0.50f));
            g.setFont(GalleryFonts::value(8.0f));
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
        // Shell bg — shellWhite() resolves to #0E0E10 in dark mode
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(8, 6);

        // Search field row
        auto searchRow = b.removeFromTop(28);
        countLabel.setBounds(searchRow.removeFromRight(56));
        searchField.setBounds(searchRow.reduced(0, 2));

        b.removeFromTop(4);

        // Mood pills — wrap into rows of ~6 per line
        // Prototype: flex-wrap, pill shapes (border-radius 10px), Inter 9px, 2px 8px padding
        {
            const int pillH   = 20;
            const int hGap    = 4;
            const int vGap    = 4;
            int px = b.getX();
            int py = b.getY();
            const int maxX = b.getRight();

            for (int i = 0; i < kNumMoods; ++i)
            {
                // Measure pill width from text
                int pillW = juce::jmax(32, (int)GalleryFonts::body(9.0f)
                            .getStringWidthFloat(moodBtns[i].getButtonText()) + 18);

                if (px + pillW > maxX && i > 0)
                {
                    px = b.getX();
                    py += pillH + vGap;
                }
                moodBtns[i].setBounds(px, py, pillW, pillH);
                px += pillW + hGap;
            }
            b.removeFromTop(py - b.getY() + pillH); // no extra vGap — b.removeFromTop(4) below handles spacing
        }

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
