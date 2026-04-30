// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Core/PresetManager.h"
#include "../GalleryColors.h"
#include "GalleryLookAndFeel.h"
#include "DnaHexagon.h"

namespace xoceanus
{

//==============================================================================
// PresetBrowserPanel — full preset browser shown in a CallOutBox.
// Provides mood tabs + name search + scrollable list.
// Calls onPresetSelected callback when the user clicks a preset row.
class PresetBrowserPanel : public juce::Component, public juce::ListBoxModel, public juce::Timer
{
public:
    /** Construct a preset browser panel.
        @param pm         The preset manager to read from.
        @param onSelect   Callback fired when a preset row is clicked.
        @param engineFilter   If non-empty, only presets whose \c engines array contains
                              this engine ID are shown.  Pass empty string for no filter
                              (global browser — original behaviour). */
    PresetBrowserPanel(const PresetManager& pm,
                       std::function<void(const PresetData&)> onSelect,
                       const juce::String& engineFilter = {},
                       int slotIndex = -1)
        : presetManager(pm), onPresetSelected(std::move(onSelect)),
          engineFilter_(engineFilter), slotIndex_(slotIndex)
    {
        // ── Per-slot header (Q1 — #1356) ─────────────────────────────────────
        // Only shown when an engine filter is active (i.e. opened from a buoy pill).
        // Displays engine name + slot badge on left, × close button on right.
        if (engineFilter_.isNotEmpty())
        {
            // Engine name label
            engineHeaderLabel_.setFont(GalleryFonts::display(11.0f));
            engineHeaderLabel_.setColour(juce::Label::textColourId,
                                         GalleryColors::get(GalleryColors::xoGold).withAlpha(0.85f));
            engineHeaderLabel_.setText(engineFilter_.toUpperCase() +
                                       (slotIndex_ >= 0 ? " \xc2\xb7 Slot " + juce::String(slotIndex_ + 1) : ""),
                                       juce::dontSendNotification);
            addAndMakeVisible(engineHeaderLabel_);
            A11y::setup(engineHeaderLabel_, "Engine preset filter",
                        "Showing presets for " + engineFilter_ + " only");

            // Close button
            closeButton_.setButtonText(juce::String(juce::CharPointer_UTF8("\xc3\x97")));
            closeButton_.setColour(juce::TextButton::textColourOffId,
                                   GalleryColors::get(GalleryColors::t3()));
            closeButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            closeButton_.onClick = [this] { if (onCloseRequested) onCloseRequested(); };
            addAndMakeVisible(closeButton_);
            A11y::setup(closeButton_, "Close preset browser", "Close this preset browser");
        }

        // Search field
        searchField.setTextToShowWhenEmpty("Search presets...",
                                           GalleryColors::get(GalleryColors::textMid()).withAlpha(0.65f));
        // Prototype: elevated bg, border, T1 text
        searchField.setColour(juce::TextEditor::backgroundColourId, GalleryColors::get(GalleryColors::elevated()));
        searchField.setColour(juce::TextEditor::outlineColourId, GalleryColors::border());
        searchField.setColour(juce::TextEditor::textColourId, GalleryColors::get(GalleryColors::t1()));
        searchField.setFont(GalleryFonts::body(11.0f));
        searchField.onTextChange = [this] { startTimer(150); };
        addAndMakeVisible(searchField);
        A11y::setup(searchField, "Preset Search", "Type to filter presets by name");

        // Mood filter buttons (ALL = index 0, then 16 moods)
        static const char* moodLabels[] = {"ALL",      "Foundation", "Atmosphere", "Entangled", "Prism",       "Flux",
                                           "Aether",   "Family",     "Submerged",  "Coupling",  "Crystalline", "Deep",
                                           "Ethereal", "Kinetic",    "Luminous",   "Organic",   "Shadow"};
        for (int i = 0; i < kNumMoods; ++i)
        {
            moodBtns[i].setButtonText(moodLabels[i]);
            moodBtns[i].setClickingTogglesState(false);
            // Prototype: transparent default bg, t3 text, gold-dim active bg + gold text
            moodBtns[i].setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            moodBtns[i].setColour(juce::TextButton::textColourOffId, GalleryColors::get(GalleryColors::t3()));
            moodBtns[i].setColour(juce::TextButton::textColourOnId, GalleryColors::get(GalleryColors::xoGold));
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
            A11y::setup(moodBtns[i], juce::String("Mood Filter: ") + moodLabels[i],
                        "Filter preset list by mood category");
        }
        moodBtns[0].setToggleState(true, juce::dontSendNotification);

        // Preset list
        listBox.setModel(this);
        listBox.setRowHeight(32); // Prototype: ~32px rows (7px padding + content)
        // Prototype: transparent bg (parent paints bg), subtle border
        listBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        listBox.setColour(juce::ListBox::outlineColourId, GalleryColors::border());
        listBox.setOutlineThickness(1);
        addAndMakeVisible(listBox);
        A11y::setup(listBox, "Preset List", "Scrollable list of presets matching current filter");

        // Count label
        countLabel.setFont(GalleryFonts::label(10.0f)); // (#885: 8.5pt→10pt legibility floor)
        countLabel.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()).withAlpha(0.55f));
        countLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(countLabel);
        A11y::setup(countLabel, "Preset Count", "Number of presets matching current filter", false);

        updateFilter();
        // #194: preferred initial size; layout is responsive and will reflow
        // correctly at any width ≥ kMinWidth via resized().
        setSize(380, 340);
    }

    //==========================================================================
    // Minimum size constants — callers (e.g. SidebarPanel) can read these to
    // enforce a floor and prevent clipping of the search row or mood pills.
    static constexpr int kMinWidth = 280;
    static constexpr int kMinHeight = 240;

    // juce::ListBoxModel interface
    int getNumRows() override { return (int)filtered.size(); }

    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (row < 0 || row >= (int)filtered.size())
            return;

        const auto& preset = filtered[static_cast<size_t>(row)];
        using namespace GalleryColors;

        // ── Mood color lookup (#933) ─────────────────────────────────────────
        // Resolved before selected/background paint so both the left-border and
        // the mood pip use the same colour without duplicating the lookup.
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
            juce::Colour(0xFF1A0A2E), // Shadow      → Void Indigo
        };
        static const char* moodIds[] = {"Foundation", "Atmosphere", "Entangled", "Prism",    "Flux",
                                        "Aether",     "Family",     "Submerged", "Coupling", "Crystalline",
                                        "Deep",       "Ethereal",   "Kinetic",   "Luminous", "Organic",
                                        "Shadow"};
        juce::Colour dot = get(borderGray());
        for (int mi = 0; mi < 16; ++mi)
            if (preset.mood == moodIds[mi])
            {
                dot = moodColors[mi];
                break;
            }

        if (selected)
        {
            g.fillAll(juce::Colour(0x0BFFFFFF)); // rgba(255,255,255,0.045)
        }

        // #933: 2px mood-colored left border on every row (selected rows are fully
        // opaque; unselected rows use 0.45 alpha so the list stays readable).
        g.setColour(dot.withAlpha(selected ? 1.0f : 0.45f));
        g.fillRect(0, 0, 2, h);

        // Mood pip — 5×5px circle to the right of the left border
        g.setColour(dot.withAlpha(0.7f));
        g.fillEllipse(10.0f, h * 0.5f - 2.5f, 5.0f, 5.0f);

        // ── DNA Hexagon (20×20) — right-aligned before the engine tag ─────────
        // Drawn inline to avoid per-row Component allocation overhead.
        // Layout from right: [4px margin][20px hex][4px gap][engine tag 26px][4px margin]
        {
            static constexpr float kHexSize   = 20.0f;
            static constexpr float kHexRight  = 54.0f; // distance from right edge to hex right
            const float hexX = static_cast<float>(w) - kHexRight;
            const float hexY = (static_cast<float>(h) - kHexSize) * 0.5f;
            const float cx   = hexX + kHexSize * 0.5f;
            const float cy   = hexY + kHexSize * 0.5f;
            const float baseR = kHexSize * 0.5f - 1.5f;

            // Pick accent from mood colour (reuses dot) so the hex color matches the pill
            const juce::Colour hexAccent = dot;

            // Guide hexagon (regular)
            {
                juce::Path guide;
                for (int vi = 0; vi < 6; ++vi)
                {
                    const float angle = juce::MathConstants<float>::pi / 6.0f
                                        + static_cast<float>(vi) * juce::MathConstants<float>::pi / 3.0f;
                    const float vx = cx + baseR * std::cos(angle);
                    const float vy = cy + baseR * std::sin(angle);
                    if (vi == 0) guide.startNewSubPath(vx, vy); else guide.lineTo(vx, vy);
                }
                guide.closeSubPath();
                g.setColour(hexAccent.withAlpha(0.15f));
                g.strokePath(guide, juce::PathStrokeType(0.75f));
            }

            // DNA shape — vertex order: brightness, warmth, movement, density, space, aggression
            const float dnaVals[6] = {
                preset.dna.brightness, preset.dna.warmth,   preset.dna.movement,
                preset.dna.density,    preset.dna.space,    preset.dna.aggression
            };
            {
                juce::Path shape;
                for (int vi = 0; vi < 6; ++vi)
                {
                    const float angle = juce::MathConstants<float>::pi / 6.0f
                                        + static_cast<float>(vi) * juce::MathConstants<float>::pi / 3.0f;
                    const float r  = baseR * (0.3f + 0.7f * juce::jlimit(0.0f, 1.0f, dnaVals[vi]));
                    const float vx = cx + r * std::cos(angle);
                    const float vy = cy + r * std::sin(angle);
                    if (vi == 0) shape.startNewSubPath(vx, vy); else shape.lineTo(vx, vy);
                }
                shape.closeSubPath();
                g.setColour(hexAccent.withAlpha(0.20f));
                g.fillPath(shape);
                g.setColour(hexAccent.withAlpha(0.60f));
                g.strokePath(shape, juce::PathStrokeType(1.0f));
            }
        }

        // Preset name — reduced right margin to leave room for hex + engine tag
        g.setColour(get(selected ? t1() : t2()));
        g.setFont(GalleryFonts::body(11.5f)); // Prototype: Inter 11.5px
        {
            auto displayName = GalleryUtils::ellipsizeText(g.getCurrentFont(), preset.name, (float)(w - 60));
            g.drawText(displayName, 22, 0, w - 60, h, juce::Justification::centredLeft, false);
        }

        // Engine tag if multi-engine (rightmost 26px)
        if (!preset.engines.isEmpty() && preset.engines[0].isNotEmpty())
        {
            auto tag = preset.engines[0].substring(0, juce::jmin(3, preset.engines[0].length())).toUpperCase();
            // Engine tag badge — JetBrains Mono 10pt (#885: 8pt→10pt legibility floor)
            g.setColour(get(t3()).withAlpha(0.50f));
            g.setFont(GalleryFonts::value(10.0f));
            g.drawText(tag, w - 30, 0, 28, h, juce::Justification::centredRight);
        }
    }

    juce::String getNameForRow(int row) override
    {
        if (row < 0 || row >= (int)filtered.size())
            return {};
        const auto& p = filtered[(size_t)row];
        return p.name + (p.mood.isEmpty() ? "" : ", " + p.mood);
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
    {
        // Single-click now loads; double-click is a no-op (selectRow already called).
        (void)row;
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override
    {
        listBox.selectRow(row);
        selectRow(row);
    }

    void returnKeyPressed(int lastRowSelected) override { selectRow(lastRowSelected); }

    void selectedRowsChanged(int) override {}

    void paint(juce::Graphics& g) override
    {
        // Shell bg — shellWhite() resolves to #0E0E10 in dark mode
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));
    }

    /** Optional callback: user clicked the × close button in the per-slot header.
        The owning CallOutBox will handle actual dismissal via juce::CallOutBox::dismiss()
        or by the parent deleting the component; this fires before that. */
    std::function<void()> onCloseRequested;

    void resized() override
    {
        auto b = getLocalBounds().reduced(8, 6);

        // Per-slot header row (only present when engine filter is active — #1356)
        if (engineFilter_.isNotEmpty())
        {
            auto headerRow = b.removeFromTop(22);
            closeButton_.setBounds(headerRow.removeFromRight(22).reduced(1, 2));
            engineHeaderLabel_.setBounds(headerRow);
            b.removeFromTop(2);
        }

        // Search field row
        auto searchRow = b.removeFromTop(28);
        countLabel.setBounds(searchRow.removeFromRight(56));
        searchField.setBounds(searchRow.reduced(0, 2));

        b.removeFromTop(4);

        // Mood pills — wrap into rows of ~6 per line
        // Prototype: flex-wrap, pill shapes (border-radius 10px), Inter 9px, 2px 8px padding
        {
            const int pillH = 20;
            const int hGap = 4;
            const int vGap = 4;
            int px = b.getX();
            int py = b.getY();
            const int maxX = b.getRight();

            for (int i = 0; i < kNumMoods; ++i)
            {
                // Measure pill width from text
                int pillW = juce::jmax(
                    32,
                    juce::roundToInt(GalleryFonts::body(9.0f).getStringWidthFloat(moodBtns[i].getButtonText())) + 18);

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

    // juce::Timer
    void timerCallback() override
    {
        stopTimer();
        juce::Component::SafePointer<PresetBrowserPanel> safe(this);
        if (safe != nullptr)
            safe->updateFilter();
    }

private:
    void selectRow(int row)
    {
        if (row >= 0 && row < (int)filtered.size() && onPresetSelected)
            onPresetSelected(filtered[static_cast<size_t>(row)]);
    }

    void updateFilter()
    {
        static const char* moodNames[] = {"",         "Foundation", "Atmosphere", "Entangled", "Prism",       "Flux",
                                          "Aether",   "Family",     "Submerged",  "Coupling",  "Crystalline", "Deep",
                                          "Ethereal", "Kinetic",    "Luminous",   "Organic",   "Shadow"};

        auto query = searchField.getText().trim();

        // Start with mood filter (index 0 = ALL = no filter).
        // getLibrary() returns a shared_ptr — O(1) copy, no vector allocation.
        const auto lib = presetManager.getLibrary();
        filtered.clear();

        for (const auto& p : *lib)
        {
            bool moodMatch   = (activeMood == 0) || (p.mood == moodNames[activeMood]);
            bool nameMatch   = query.isEmpty() || p.name.containsIgnoreCase(query);
            // Q3 engine filter (#1356): if engineFilter_ is set, only show presets that
            // list this engine in their engines array.
            bool engineMatch = engineFilter_.isEmpty() || p.engines.contains(engineFilter_);
            if (moodMatch && nameMatch && engineMatch)
                filtered.push_back(p);
        }

        // Sort alphabetically within current filter
        std::sort(filtered.begin(), filtered.end(),
                  [](const PresetData& a, const PresetData& b) { return a.name.compareIgnoreCase(b.name) < 0; });

        listBox.updateContent();
        listBox.deselectAllRows();

        countLabel.setText(juce::String(filtered.size()) + " presets", juce::dontSendNotification);
    }

    static constexpr int kNumMoods = 17; // ALL + 16 moods

    const PresetManager& presetManager;
    std::function<void(const PresetData&)> onPresetSelected;
    juce::String engineFilter_; ///< If non-empty, only presets for this engine are shown (#1356).
    int          slotIndex_ = -1; ///< Slot index displayed in the header badge (#1356). -1 = not shown.

    // Per-slot header components (only visible when engineFilter_ is set — #1356)
    juce::Label      engineHeaderLabel_;
    juce::TextButton closeButton_;

    juce::TextEditor searchField;
    juce::TextButton moodBtns[kNumMoods];
    juce::ListBox listBox;
    juce::Label countLabel;
    std::vector<PresetData> filtered;
    int activeMood = 0; // 0 = ALL

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserPanel)
};

} // namespace xoceanus
