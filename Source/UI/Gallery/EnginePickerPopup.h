// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// EnginePickerPopup.h — Searchable engine selection component for XOceanus.
//
// Replaces the flat PopupMenu engine list with:
//   - Text search across engine name, category, and archetype description
//   - Category filter pills (ALL | SYNTH | PERC | BASS | PAD | STRING | ORGAN | VOCAL | FX | UTILITY)
//   - Results grouped by water-column depth zone (Sunlit / Twilight / Midnight)
//   - 8px accent color dot + engine name + category badge per row
//
// Designed to be shown in a juce::CallOutBox:
//   auto* popup = new xoceanus::EnginePickerPopup();
//   popup->onEngineSelected = [this](const juce::String& id) { ... };
//   juce::CallOutBox::launchAsynchronously(std::unique_ptr<Component>(popup),
//                                          targetBounds, parentComp);
//
// Size: 340 x 420pt.

#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Core/EngineRegistry.h"
#include "../GalleryColors.h"
#include "../EngineRoster.h"

namespace xoceanus
{

//==============================================================================
// Lightweight LookAndFeel override so category pill buttons render with a
// compact 8pt label font without requiring a full LookAndFeel subclass elsewhere.
struct PillButtonLookAndFeel : public juce::LookAndFeel_V4
{
    void drawButtonText(juce::Graphics& g, juce::TextButton& btn, bool /*isMouseOver*/, bool /*isButtonDown*/) override
    {
        g.setFont(GalleryFonts::label(10.0f)); // (#885: 8pt→10pt legibility floor)
        g.setColour(btn.findColour(btn.getToggleState() ? juce::TextButton::textColourOnId
                                                        : juce::TextButton::textColourOffId));
        g.drawFittedText(btn.getButtonText(), btn.getLocalBounds().reduced(1, 0), juce::Justification::centred, 1,
                         0.9f);
    }
};

//==============================================================================
// EnginePickerPopup — header-only implementation
class EnginePickerPopup : public juce::Component,
                          public juce::ListBoxModel,
                          public juce::TextEditor::Listener,
                          public juce::KeyListener
{
public:
    // Unhide juce::Component::keyPressed(KeyPress) — we override the KeyListener
    // two-argument form, which would otherwise hide the Component one-argument form.
    using juce::Component::keyPressed;

    //==========================================================================
    // Callback invoked when the user selects an engine. Caller is responsible
    // for dismissing the owning CallOutBox (or call dismissPopup()).
    std::function<void(const juce::String& engineId)> onEngineSelected;

    //==========================================================================
    EnginePickerPopup()
    {
        A11y::setup(*this, "Engine Picker", "Search and select an engine by name, category, or description");

        buildMetadataTable();
        collectRegisteredEngines();

        // ── Search field ─────────────────────────────────────────────────────
        searchField.setTextToShowWhenEmpty("Search " + juce::String(allEngineIds.size()) + " engines...",
                                           GalleryColors::get(GalleryColors::t3()).withAlpha(0.80f));
        searchField.setColour(juce::TextEditor::backgroundColourId, GalleryColors::get(GalleryColors::surface()));
        searchField.setColour(juce::TextEditor::outlineColourId, GalleryColors::borderMd());
        searchField.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(GalleryColors::xoGold));
        searchField.setColour(juce::TextEditor::textColourId, GalleryColors::get(GalleryColors::t1()));
        searchField.setFont(GalleryFonts::body(11.0f));
        searchField.setReturnKeyStartsNewLine(false);
        searchField.addListener(this);
        searchField.addKeyListener(this);
        addAndMakeVisible(searchField);

        // ── Category pill buttons ─────────────────────────────────────────────
        // Short readable labels — font is 7.5pt to keep all pills fitting in 340pt.
        // Index 0 = "Start" — beginner-curated subset for new users (#719).
        //   Shows only engines flagged beginner=true in the metadata table.
        //   "Start" is the default selection so new users see a manageable set.
        // Index 1 = "ALL"   — full fleet.
        // Index 2–10 = synthesis type / role categories.
        static const char* kCatLabels[] = {"Start", "ALL",    "Synth", "Perc",  "Bass", "Pad",
                                            "String", "Organ", "Vocal", "FX",   "Util"};
        static const char* kCatTooltips[] = {
            "Starter engines — curated for new users (depth zone: Sunlit)",
            "All engines", "Synthesizer", "Percussion", "Bass", "Pad", "String", "Organ", "Vocal", "FX", "Utility",
        };
        for (int i = 0; i < kNumCategories; ++i)
        {
            catBtns[i].setButtonText(kCatLabels[i]);
            catBtns[i].setTooltip(kCatTooltips[i]);
            catBtns[i].setClickingTogglesState(false);
            catBtns[i].setLookAndFeel(&pillLnF);
            stylePillButton(catBtns[i], i == 0);
            const int catIdx = i;
            catBtns[i].onClick = [this, catIdx]
            {
                activeCategory = catIdx;
                for (int j = 0; j < kNumCategories; ++j)
                    stylePillButton(catBtns[j], j == catIdx);
                updateFilter();
            };
            addAndMakeVisible(catBtns[i]);
        }

        // ── Results list ──────────────────────────────────────────────────────
        listBox.setModel(this);
        listBox.setRowHeight(kRowHeight);
        listBox.setColour(juce::ListBox::backgroundColourId, GalleryColors::get(GalleryColors::elevated()));
        listBox.setColour(juce::ListBox::outlineColourId, GalleryColors::border());
        listBox.setOutlineThickness(1);
        listBox.addKeyListener(this);
        addAndMakeVisible(listBox);

        // ── Empty state label (shown when search returns no results) ──────────
        emptyLabel.setText("No engines match your search", juce::dontSendNotification);
        emptyLabel.setFont(GalleryFonts::body(11.0f));
        emptyLabel.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::t3()));
        emptyLabel.setJustificationType(juce::Justification::centred);
        addChildComponent(emptyLabel); // hidden by default

        // ── Count label ───────────────────────────────────────────────────────
        countLabel.setFont(GalleryFonts::label(10.0f)); // (#885: 8.5pt→10pt legibility floor)
        countLabel.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()).withAlpha(0.55f));
        countLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(countLabel);

        updateFilter();

        // #180 HiDPI scaling: scale the popup dimensions by the primary display's
        // device-pixel ratio so the component occupies the same physical screen area
        // on Retina/HiDPI displays and pill labels remain ≥12pt effective size.
        {
            const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
            const float scale = (display != nullptr) ? juce::jmax(1.0f, (float)display->scale) : 1.0f;
            setSize(juce::roundToInt(340.0f * scale), juce::roundToInt(420.0f * scale));
        }

        // Focus the search field immediately so the user can start typing.
        // SafePointer guards against the rare case where the CallOutBox dismisses
        // this component before the 50ms timer fires (e.g. host window close).
        juce::Component::SafePointer<EnginePickerPopup> safeThis(this);
        juce::Timer::callAfterDelay(50,
                                    [safeThis]
                                    {
                                        if (safeThis != nullptr)
                                            safeThis->searchField.grabKeyboardFocus();
                                    });
    }

    ~EnginePickerPopup() override
    {
        // Remove key listeners explicitly to prevent dangling raw pointer reads
        // if JUCE's component destruction order tries to broadcast a final key event.
        searchField.removeKeyListener(this);
        listBox.removeKeyListener(this);
        // Detach custom LookAndFeel before it destructs (JUCE requirement).
        for (auto& btn : catBtns)
            btn.setLookAndFeel(nullptr);
    }

    //==========================================================================
    // juce::ListBoxModel interface
    //==========================================================================
    int getNumRows() override { return (int)flatRows.size(); }

    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool isSelected) override
    {
        if (row < 0 || row >= (int)flatRows.size())
            return;

        const auto& fr = flatRows[static_cast<size_t>(row)];
        using namespace GalleryColors;

        // ── Section header rows ───────────────────────────────────────────────
        if (fr.isSectionHeader)
        {
            // Subtle tinted header background — raised above the list bg
            g.fillAll(get(raised()).withAlpha(darkMode() ? 1.0f : 0.6f));

            // Left accent stripe
            g.setColour(fr.headerColor.withAlpha(0.70f));
            g.fillRect(0, 0, 3, h);

            // Header label — uppercase, T3 tonal color with zone hue blended in
            g.setFont(GalleryFonts::label(10.0f)); // (#885: 8pt→10pt legibility floor)
            g.setColour(get(t3()).interpolatedWith(fr.headerColor, 0.35f));
            {
                auto displayLabel = GalleryUtils::ellipsizeText(g.getCurrentFont(), fr.headerLabel.toUpperCase(), (float)(w - 14));
                g.drawText(displayLabel, 10, 0, w - 14, h, juce::Justification::centredLeft, false);
            }
            return;
        }

        // ── Engine rows ───────────────────────────────────────────────────────
        if (isSelected)
        {
            g.fillAll(juce::Colour(0x0BFFFFFF));
            g.setColour(juce::Colour(get(xoGold)));
            g.fillRect(0, 0, 2, h);
        }
        else if (row % 2 == 0)
            g.fillAll(get(elevated()));
        else
            g.fillAll(get(surface()));

        // Accent dot
        const float dotR = 4.0f;
        const float dotCy = h * 0.5f;
        const float dotCx = 10.0f + dotR;
        g.setColour(fr.accent);
        g.fillEllipse(dotCx - dotR, dotCy - dotR, dotR * 2.0f, dotR * 2.0f);

        // Engine name — top half of row
        const int nameY = 3;
        const int nameH = 16;
        g.setFont(GalleryFonts::display(11.0f));
        g.setColour(isSelected ? fr.accent : get(t1()));
        {
            auto displayName = GalleryUtils::ellipsizeText(g.getCurrentFont(), fr.engineId.toUpperCase(), (float)(w - 74));
            g.drawText(displayName, 24, nameY, w - 74, nameH, juce::Justification::centredLeft, false);
        }

        // Archetype subtitle — below engine name, muted T3 color
        if (!fr.archetype.isEmpty())
        {
            g.setFont(GalleryFonts::body(10.0f)); // (#885: 8pt→10pt legibility floor)
            g.setColour(get(t3()).withAlpha(0.70f));
            auto displayArchetype = GalleryUtils::ellipsizeText(g.getCurrentFont(), fr.archetype, (float)(w - 74));
            g.drawText(displayArchetype, 24, nameY + nameH, w - 74, 11, juce::Justification::centredLeft, false);
        }

        // Category badge — right side, T4 muted mono (vertically centred in name row)
        g.setFont(GalleryFonts::value(10.0f)); // (#885: 8pt→10pt legibility floor)
        g.setColour(get(t4()));
        {
            auto displayCategory = GalleryUtils::ellipsizeText(g.getCurrentFont(), fr.category, 62.0f);
            g.drawText(displayCategory, w - 66, nameY, 62, nameH, juce::Justification::centredRight, false);
        }
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override { commitSelection(row); }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override { commitSelection(row); }

    void selectedRowsChanged(int lastRowSelected) override
    {
        // Selection change via keyboard navigation does not commit — only
        // explicit click (listBoxItemClicked) or Enter/Return commits.
        (void)lastRowSelected;
    }

    //==========================================================================
    // juce::TextEditor::Listener
    //==========================================================================
    void textEditorTextChanged(juce::TextEditor&) override { updateFilter(); }
    void textEditorReturnKeyPressed(juce::TextEditor&) override { commitSelection(listBox.getSelectedRow()); }
    void textEditorEscapeKeyPressed(juce::TextEditor&) override { dismissPopup(); }

    //==========================================================================
    // juce::KeyListener — arrow keys in search field navigate list
    //==========================================================================
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            dismissPopup();
            return true;
        }

        if (key == juce::KeyPress::returnKey)
        {
            commitSelection(listBox.getSelectedRow());
            return true;
        }

        const int numRows = (int)flatRows.size();
        if (numRows == 0)
            return false;

        if (key == juce::KeyPress::downKey)
        {
            int cur = listBox.getSelectedRow();
            int next = (cur < 0) ? 0 : juce::jmin(cur + 1, numRows - 1);
            // Skip section headers
            while (next < numRows && flatRows[static_cast<size_t>(next)].isSectionHeader)
                ++next;
            if (next < numRows)
            {
                listBox.selectRow(next);
                listBox.scrollToEnsureRowIsOnscreen(next);
            }
            return true;
        }

        if (key == juce::KeyPress::upKey)
        {
            int cur = listBox.getSelectedRow();
            int prev = (cur <= 0) ? 0 : cur - 1;
            // Skip section headers
            while (prev > 0 && flatRows[static_cast<size_t>(prev)].isSectionHeader)
                --prev;
            if (!flatRows[static_cast<size_t>(prev)].isSectionHeader)
            {
                listBox.selectRow(prev);
                listBox.scrollToEnsureRowIsOnscreen(prev);
            }
            return true;
        }

        (void)originatingComponent;
        return false;
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        // Use elevated surface so the popup reads as a raised card in both themes.
        g.fillAll(get(elevated()));
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(8, 6);

        // Search row (32pt): search field + count label right-anchored
        auto searchRow = b.removeFromTop(32);
        countLabel.setBounds(searchRow.removeFromRight(52));
        searchField.setBounds(searchRow.reduced(0, 2));

        b.removeFromTop(4);

        // Category pill row (28pt)
        auto pillRow = b.removeFromTop(28);
        const int totalW = pillRow.getWidth();
        // Distribute pills evenly — "ALL" is slightly wider
        const int pillW = totalW / kNumCategories;
        for (int i = 0; i < kNumCategories; ++i)
            catBtns[i].setBounds(
                pillRow
                    .removeFromLeft((i < kNumCategories - 1) ? pillW : pillRow.getWidth()) // last pill gets remainder
                    .reduced(1, 0));

        b.removeFromTop(4);

        listBox.setBounds(b);
        emptyLabel.setBounds(listBox.getBounds());
    }

private:
    //==========================================================================
    // Internal flat row representation (engine rows + section headers)
    struct FlatRow
    {
        bool isSectionHeader = false;

        // For engine rows
        juce::String engineId;
        juce::String category;
        juce::String archetype;
        juce::Colour accent;
        int depthZone = 0; // 0=Sunlit 1=Twilight 2=Midnight

        // For section headers
        juce::String headerLabel;
        juce::Colour headerColor;
    };

    //==========================================================================
    // Engine metadata — sourced from the shared EngineRoster.h table.
    // Do NOT add engines here; add them to Source/UI/EngineRoster.h instead.
    //==========================================================================

    /// Alias so the rest of this class can refer to EngineRosterEntry without
    /// the namespace qualifier.
    using EngineInfo = ::xoceanus::EngineRosterEntry;

    static const EngineInfo* engineMetadataTable()
    {
        return ::xoceanus::engineRosterTable();
    }

    //==========================================================================
    // Build a lookup map from lowercased engine ID → metadata index.
    // Uses the sentinel entry (id == nullptr) to terminate — no hardcoded count.
    void buildMetadataTable()
    {
        const auto* table = engineMetadataTable();
        for (int i = 0; table[i].id != nullptr; ++i)
        {
            juce::String key = juce::String(table[i].id).toLowerCase();
            // First entry wins — duplicate IDs in the table are intentionally ignored.
            if (metaLookup.find(key.toStdString()) == metaLookup.end())
                metaLookup[key.toStdString()] = i;
        }
    }

    // Returns a pointer to the EngineInfo for an engine ID (case-insensitive).
    // Returns nullptr if not in the static table.
    const EngineInfo* metaFor(const juce::String& engineId) const
    {
        auto it = metaLookup.find(engineId.toLowerCase().toStdString());
        if (it != metaLookup.end())
            return engineMetadataTable() + it->second;
        return nullptr;
    }

    juce::String categoryOf(const juce::String& engineId) const
    {
        if (auto* m = metaFor(engineId))
            return m->category;
        return "Synth";
    }

    juce::String archetypeOf(const juce::String& engineId) const
    {
        if (auto* m = metaFor(engineId))
            return m->archetype;
        return "";
    }

    int depthZoneOf(const juce::String& engineId) const
    {
        if (auto* m = metaFor(engineId))
            return m->depthZone;
        return 1; // default Twilight
    }

    //==========================================================================
    // Collect engine IDs from the live registry
    void collectRegisteredEngines()
    {
        allEngineIds.clear();
        const auto& reg = EngineRegistry::instance();
        for (const auto& stdId : reg.getRegisteredIds())
            allEngineIds.add(juce::String(stdId.c_str()));

        // Sort alphabetically for consistent initial display
        allEngineIds.sort(true);
    }

    //==========================================================================
    // Search predicate
    bool matchesQuery(const juce::String& engineId, const juce::String& category, const juce::String& archetype,
                      const juce::String& query) const
    {
        if (query.isEmpty())
            return true;
        auto q = query.toLowerCase();
        if (engineId.toLowerCase().contains(q))
            return true;
        if (category.toLowerCase().contains(q))
            return true;
        if (archetype.toLowerCase().contains(q))
            return true;
        return false;
    }

    //==========================================================================
    // Category filter — index matches catBtns[] order (#719):
    //   0=START (beginner engines — Sunlit zone only)
    //   1=ALL
    //   2=SYNTH  3=PERC  4=BASS  5=PAD  6=STRING  7=ORGAN  8=VOCAL  9=FX  10=UTILITY
    bool matchesCategory(const juce::String& engineId, const juce::String& category, int catFilter) const
    {
        if (catFilter == 0)
        {
            // "Start" — show only beginner-friendly engines (Sunlit zone, depthZone == 0)
            if (auto* m = metaFor(engineId))
                return m->depthZone == 0;
            return false;
        }
        if (catFilter == 1)
            return true; // ALL
        static const char* kFilterToCategory[] = {
            "",           // 0 Start (handled above)
            "",           // 1 ALL (handled above)
            "Synth",      // 2
            "Percussion", // 3
            "Bass",       // 4
            "Pad",        // 5
            "String",     // 6
            "Organ",      // 7
            "Vocal",      // 8
            "FX",         // 9
            "Utility",    // 10
        };
        if (catFilter < 0 || catFilter >= static_cast<int>(std::size(kFilterToCategory)))
            return true;
        return category == kFilterToCategory[catFilter];
    }

    //==========================================================================
    // Rebuild flatRows from current filter state and repopulate listBox
    void updateFilter()
    {
        flatRows.clear();

        auto query = searchField.getText().trim();

        // Depth zone colors
        static const juce::Colour kZoneColors[3] = {
            juce::Colour(0xFF48CAE4), // Sunlit  — cyan
            juce::Colour(0xFF0096C7), // Twilight — blue
            juce::Colour(0xFF7B2FBE), // Midnight — violet
        };
        static const char* kZoneLabels[3] = {
            "Sunlit Zone",
            "Twilight Zone",
            "Midnight Zone",
        };

        // Collect matching engines into per-zone buckets
        juce::StringArray zones[3];

        for (const auto& id : allEngineIds)
        {
            auto cat = categoryOf(id);
            auto arch = archetypeOf(id);

            if (!matchesQuery(id, cat, arch, query))
                continue;
            if (!matchesCategory(id, cat, activeCategory))
                continue;

            int zone = depthZoneOf(id);
            zone = juce::jlimit(0, 2, zone);
            zones[zone].add(id);
        }

        // Build flat list: header + engines for each non-empty zone
        int totalEngines = 0;
        for (int z = 0; z < 3; ++z)
        {
            if (zones[z].isEmpty())
                continue;

            // Section header row
            FlatRow hdr;
            hdr.isSectionHeader = true;
            hdr.headerLabel = kZoneLabels[z];
            hdr.headerColor = kZoneColors[z];
            flatRows.push_back(std::move(hdr));

            // Engine rows within this zone
            for (const auto& id : zones[z])
            {
                FlatRow er;
                er.isSectionHeader = false;
                er.engineId = id;
                er.category = categoryOf(id);
                er.archetype = archetypeOf(id);
                er.accent = GalleryColors::accentForEngine(id);
                er.depthZone = z;
                flatRows.push_back(std::move(er));
                ++totalEngines;
            }
        }

        listBox.updateContent();
        listBox.deselectAllRows();

        // Auto-select first engine row so arrow-down works immediately
        for (int i = 0; i < (int)flatRows.size(); ++i)
        {
            if (!flatRows[static_cast<size_t>(i)].isSectionHeader)
            {
                listBox.selectRow(i);
                break;
            }
        }

        // Show empty state label when no engines match; hide list to avoid blank area
        const bool isEmpty = (totalEngines == 0);
        emptyLabel.setVisible(isEmpty);
        listBox.setVisible(!isEmpty);

        juce::String countText = juce::String(totalEngines) + " engines";
        if (activeCategory == 0 && totalEngines > 0)
            countText += "  (tip: tap ALL for full fleet)";
        countLabel.setText(countText, juce::dontSendNotification);
    }

    //==========================================================================
    // Commit engine selection and fire callback
    void commitSelection(int row)
    {
        if (row < 0 || row >= (int)flatRows.size())
            return;

        const auto& fr = flatRows[static_cast<size_t>(row)];
        if (fr.isSectionHeader)
            return;

        if (onEngineSelected)
            onEngineSelected(fr.engineId);

        dismissPopup();
    }

    void dismissPopup()
    {
        if (auto* callOut = findParentComponentOfClass<juce::CallOutBox>())
            callOut->dismiss();
    }

    //==========================================================================
    // Pill button styling helper
    void stylePillButton(juce::TextButton& btn, bool active) const
    {
        using namespace GalleryColors;
        btn.setColour(juce::TextButton::buttonColourId,
                      active ? get(xoGold).withAlpha(0.14f) : juce::Colour(0x00000000));
        btn.setColour(juce::TextButton::textColourOffId, active ? get(xoGoldText()) : get(t3()));
        btn.setColour(juce::TextButton::textColourOnId, get(xoGoldText()));
        btn.setColour(juce::TextButton::buttonOnColourId, get(xoGold).withAlpha(0.14f));
        btn.setToggleState(active, juce::dontSendNotification);
    }

    //==========================================================================
    static constexpr int kNumCategories = 11; // Start + ALL + 9 role categories (#719)
    // Section headers share the same row height as engine rows — both rendered
    // inside paintListBoxItem, distinguished by FlatRow::isSectionHeader.
    static constexpr int kRowHeight = 34;

    // Engine ID list from registry
    juce::StringArray allEngineIds;

    // Metadata lookup: lowercase engine id → index in kTable
    std::unordered_map<std::string, int> metaLookup;

    // Flat display rows (headers interleaved with engine rows)
    std::vector<FlatRow> flatRows;

    // LookAndFeel for compact pill buttons (must outlive catBtns[])
    PillButtonLookAndFeel pillLnF;

    // UI
    juce::TextEditor searchField;
    juce::TextButton catBtns[kNumCategories];
    juce::ListBox listBox;
    juce::Label countLabel;
    juce::Label emptyLabel;

    int activeCategory = 0; // 0 = Start (beginner view — #719); 1 = ALL; 2-10 = role categories

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnginePickerPopup)
};

} // namespace xoceanus
