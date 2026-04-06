// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include "../../Core/PresetManager.h"
#include "../Gallery/GalleryLookAndFeel.h"
#include "../Gallery/DnaHexagon.h"
// NOTE: GalleryColors, GalleryFonts, A11y, and PresetData are defined in
// XOceanusEditor.h, which includes this file.  The circular include is avoided
// intentionally — those symbols are already in scope when this header is
// compiled as part of XOceanusEditor.h.

namespace xoceanus
{

//==============================================================================
// PresetBrowser — Searchable, filterable browser for ~19,200+ factory presets.
//
// Features:
//   - Real-time text search (filters name, tags, description, engine names)
//   - Mood category filter buttons (all 16 moods: Foundation, Atmosphere, Entangled, Prism,
//     Flux, Aether, Family, Submerged, Coupling, Crystalline, Deep, Ethereal, Kinetic,
//     Luminous, Organic, Shadow)
//   - Scrollable preset list with name, mood, engine tags
//   - DNA-based "Find Similar" button (6D Euclidean distance)
//   - Favorites system: star button on each row, "Favorites" view filter (#718)
//   - Recently-used list: tracks last 20 loaded presets, "Recent" view filter (#718)
//   - Gallery Model look and feel (warm white shell, XO Gold accents)
//   - Full WCAG 2.1 AA accessibility (focus rings, screen reader labels)
//
// Favorites + Recent persistence:
//   Both are stored in XOceanus.settings via juce::PropertiesFile (same file as
//   SettingsPanel).  Favorite IDs are stored as a pipe-delimited string under key
//   "presetFavorites".  Recent IDs (most-recent first, max 20) are stored under key
//   "presetRecent".  A preset's canonical ID is its name + "|" + first engine name.
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
    //==========================================================================
    // Sort modes
    //==========================================================================

    // #716: Extended sort options.
    enum class SortMode { NameAZ, NameZA, Mood, Engine, DateRecent, Relevance };

    //==========================================================================
    // View filter — controls which preset subset is displayed (#718)
    //==========================================================================

    enum class ViewFilter { All, Favorites, Recent };

    PresetBrowser(PresetManager& pm) : presetManager(pm)
    {
        // --- Persistent settings file (same app name as SettingsPanel) ---
        {
            juce::PropertiesFile::Options opts;
            opts.applicationName    = "XOceanus";
            opts.filenameSuffix     = "settings";
            opts.osxLibrarySubFolder = "Application Support";
            settingsFile = std::make_unique<juce::PropertiesFile>(opts);
        }
        loadFavoritesFromDisk();
        loadRecentFromDisk();

        // --- View-filter buttons (All / Favorites / Recent) ---
        {
            auto setupViewBtn = [this](juce::TextButton& btn, const char* text, const char* desc, ViewFilter f)
            {
                btn.setButtonText(text);
                btn.setClickingTogglesState(true);
                btn.setRadioGroupId(99);
                btn.setColour(juce::TextButton::buttonColourId,    GalleryColors::get(GalleryColors::slotBg()));
                btn.setColour(juce::TextButton::buttonOnColourId,  GalleryColors::get(GalleryColors::xoGold));
                btn.setColour(juce::TextButton::textColourOnId,    GalleryColors::get(GalleryColors::textDark()));
                btn.setColour(juce::TextButton::textColourOffId,   GalleryColors::get(GalleryColors::textMid()));
                A11y::setup(btn, text, desc);
                const ViewFilter filter = f;
                btn.onClick = [this, filter]
                {
                    activeViewFilter = filter;
                    similarActive    = false;
                    applyFilters();
                };
                addAndMakeVisible(btn);
            };
            setupViewBtn(viewAllBtn,       "All",       "Show all presets",        ViewFilter::All);
            setupViewBtn(viewFavoritesBtn, "\xe2\x98\x85 Fav", "Show favorite presets only",   ViewFilter::Favorites);
            setupViewBtn(viewRecentBtn,    "Recent",    "Show recently used presets", ViewFilter::Recent);
            viewAllBtn.setToggleState(true, juce::dontSendNotification);
        }

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
                                      "Ethereal", "Kinetic",    "Luminous",   "Organic",   "Shadow"};

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

        // --- Sort ComboBox ---
        sortBox.addItem("A \xe2\x86\x92 Z",    static_cast<int>(SortMode::NameAZ)      + 1);
        sortBox.addItem("Z \xe2\x86\x92 A",    static_cast<int>(SortMode::NameZA)      + 1);
        sortBox.addItem("By Mood",              static_cast<int>(SortMode::Mood)        + 1);
        sortBox.addItem("By Engine",            static_cast<int>(SortMode::Engine)      + 1); // #716
        sortBox.addItem("Recent",               static_cast<int>(SortMode::DateRecent)  + 1); // #716
        sortBox.addItem("Relevance",            static_cast<int>(SortMode::Relevance)   + 1);
        sortBox.setSelectedId(static_cast<int>(SortMode::NameAZ) + 1, juce::dontSendNotification);
        sortBox.setColour(juce::ComboBox::backgroundColourId,    GalleryColors::get(GalleryColors::slotBg()));
        sortBox.setColour(juce::ComboBox::outlineColourId,       GalleryColors::get(GalleryColors::borderGray()));
        sortBox.setColour(juce::ComboBox::textColourId,          GalleryColors::get(GalleryColors::textMid()));
        sortBox.setColour(juce::ComboBox::arrowColourId,         GalleryColors::get(GalleryColors::textMid()));
        sortBox.onChange = [this]
        {
            const int selId = sortBox.getSelectedId() - 1;
            if (selId >= 0)
            {
                userSortMode = static_cast<SortMode>(selId);
                // When no search is active, immediately apply the user's choice;
                // when search is active the mode is driven by textEditorTextChanged.
                if (searchBox.getText().trim().isEmpty())
                {
                    currentSortMode = userSortMode;
                    applyFilters();
                }
            }
        };
        A11y::setup(sortBox, "Sort order", "Choose how to sort the preset list");
        addAndMakeVisible(sortBox);

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
    // Favorites API (#718)
    //==========================================================================

    /** Returns a stable preset ID used as the favorites/recent key. */
    static juce::String presetId(const PresetData& p)
    {
        juce::String eng = p.engines.isEmpty() ? "" : p.engines[0];
        return p.name + "|" + eng;
    }

    /** Toggle the favorite status of a preset and persist the change. */
    void toggleFavorite(const PresetData& p)
    {
        const juce::String id = presetId(p);
        if (favoriteIds.contains(id))
            favoriteIds.removeString(id);
        else
            favoriteIds.add(id);
        saveFavoritesToDisk();
        listBox.repaint(); // refresh star glyphs
    }

    bool isFavorite(const PresetData& p) const { return favoriteIds.contains(presetId(p)); }

    /** Call this whenever a preset is loaded so the recent list stays current. */
    void notifyPresetLoaded(const PresetData& p)
    {
        const juce::String id = presetId(p);
        recentIds.removeString(id);
        recentIds.insert(0, id);
        while (recentIds.size() > kMaxRecentPresets)
            recentIds.remove(recentIds.size() - 1);
        saveRecentToDisk();
        if (activeViewFilter == ViewFilter::Recent)
            applyFilters();
    }

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

        // View-filter bar: All | Fav | Recent  (#718)
        {
            auto filterRow = area.removeFromTop(26);
            const int btnW = filterRow.getWidth() / 3;
            viewAllBtn.setBounds(filterRow.removeFromLeft(btnW).reduced(2, 1));
            viewFavoritesBtn.setBounds(filterRow.removeFromLeft(btnW).reduced(2, 1));
            viewRecentBtn.setBounds(filterRow.reduced(2, 1));
        }
        area.removeFromTop(4);

        // Search bar + sort control on the same row
        {
            auto headerRow = area.removeFromTop(32);
            // Sort ComboBox takes a fixed 96px on the right with a 6px gap
            sortBox.setBounds(headerRow.removeFromRight(96));
            headerRow.removeFromRight(6);
            searchBox.setBounds(headerRow);
        }
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
            // Cap the mood pill area to 3 rows maximum so that the preset listBox
            // can never be compressed to zero height at narrow widths (#771).
            int moodHeight = py - area.getY() + pillH;
            moodHeight = std::min(moodHeight, pillH * 3 + vGap * 2);
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

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

        // #923: Spinner overlay drawn in the listBox area while library scan runs.
        if (isScanning_)
        {
            auto spinArea = listBox.getBounds().toFloat();
            if (spinArea.isEmpty())
                spinArea = getLocalBounds().toFloat();

            // Semi-transparent background
            g.setColour(GalleryColors::get(GalleryColors::shellWhite()).withAlpha(0.85f));
            g.fillRect(spinArea);

            const float cx = spinArea.getCentreX();
            const float cy = spinArea.getCentreY() - 14.0f;
            const float r  = 16.0f;

            // Track circle
            g.setColour(GalleryColors::get(GalleryColors::borderGray()));
            g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 2.0f);

            // Spinning arc (XO Gold) — 120° sweep rotating at spinnerAngle_
            {
                juce::Path arc;
                const float sweepRad = juce::MathConstants<float>::twoPi / 3.0f; // 120°
                arc.addArc(cx - r, cy - r, r * 2.0f, r * 2.0f,
                           spinnerAngle_, spinnerAngle_ + sweepRad, true);
                g.setColour(juce::Colour(GalleryColors::xoGold));
                g.strokePath(arc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
            }

            // "Scanning presets…" caption
            g.setColour(GalleryColors::get(GalleryColors::textMid()));
            g.setFont(GalleryFonts::body(12.0f));
            g.drawText("Scanning presets\xe2\x80\xa6",
                       juce::Rectangle<float>(cx - 80.0f, cy + r + 6.0f, 160.0f, 18.0f),
                       juce::Justification::centred, false);
        }
    }

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
        const juce::Colour moodDot = moodColour(preset.mood);
        g.setColour(moodDot.withAlpha(0.75f));
        g.fillEllipse(10.0f, h * 0.5f - 2.5f, 5.0f, 5.0f);

        // ── DNA Hexagon (20×20) — right side before engine meta ──────────────
        // Drawn inline using the same geometry as DnaHexagon::paint().
        {
            static constexpr float kHexSize  = 20.0f;
            static constexpr float kHexRight = 28.0f; // distance from right edge to hex right
            const float hexX = static_cast<float>(w) - kHexRight;
            const float hexY = (static_cast<float>(h) - kHexSize) * 0.5f;
            const float cx   = hexX + kHexSize * 0.5f;
            const float cy   = hexY + kHexSize * 0.5f;
            const float baseR = kHexSize * 0.5f - 1.5f;
            const juce::Colour hexAccent = moodDot;

            // Guide hexagon
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

            // DNA shape — brightness, warmth, movement, density, space, aggression
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

        // ── Star / favorite indicator (#718) ─────────────────────────────────
        // Drawn as a Unicode ★ (U+2605) left of the hex glyph.
        // Tap the star area to toggle favorite (handled in listBoxItemClicked).
        {
            const bool fav = favoriteIds.contains(presetId(preset));
            g.setFont(GalleryFonts::body(12.0f));
            g.setColour(fav ? juce::Colour(GalleryColors::xoGold).withAlpha(0.90f)
                             : GalleryColors::get(GalleryColors::borderGray()).withAlpha(0.45f));
            // Star sits to the left of the hex glyph (kHexRight=28 → star at w-50)
            g.drawText(juce::String::fromUTF8("\xe2\x98\x85"), w - 52, 0, 18, h, juce::Justification::centred, false);
        }

        // Preset name — Inter 11.5px (right margin extended for hex + star)
        g.setFont(GalleryFonts::body(11.5f));
        g.setColour(GalleryColors::get(GalleryColors::textDark()));
        {
            auto displayName = GalleryUtils::ellipsizeText(g.getCurrentFont(), preset.name, (float)(w - 72));
            g.drawText(displayName, 22, 0, w - 72, h / 2, juce::Justification::centredLeft, false);
        }

        // Engine tag badge — JetBrains Mono 10pt (#885: 9pt→10pt legibility floor)
        g.setFont(GalleryFonts::value(10.0f));
        g.setColour(GalleryColors::get(GalleryColors::textMid()));

        juce::String meta = preset.mood;
        for (const auto& eng : preset.engines)
        {
            if (eng.isNotEmpty())
                meta += juce::String::fromUTF8("  \xc2\xb7  ") + eng; // middle dot separator
        }
        {
            auto displayMeta = GalleryUtils::ellipsizeText(g.getCurrentFont(), meta, (float)(w - 72));
            g.drawText(displayMeta, 22, h / 2, w - 72, h / 2, juce::Justification::centredLeft, false);
        }

        // Bottom separator
        g.setColour(GalleryColors::get(GalleryColors::borderGray()).withAlpha(0.25f));
        g.drawHorizontalLine(h - 1, 10.0f, static_cast<float>(w) - 10.0f);
    }

    void listBoxItemClicked(int row, const juce::MouseEvent& e) override
    {
        if (row < 0 || row >= static_cast<int>(filteredPresets.size()))
            return;

        auto& preset = filteredPresets[static_cast<size_t>(row)];
        const int rowW = listBox.getWidth();

        // If the click landed on the star zone (right edge, 18px wide from w-52),
        // toggle favorite instead of loading the preset (#718).
        const int starLeft = rowW - 52;
        if (e.x >= starLeft && e.x < starLeft + 18)
        {
            toggleFavorite(preset);
            return;
        }

        selectedIndex = row;
        notifyPresetLoaded(preset);
        if (onPresetSelected)
            onPresetSelected(preset);
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
            auto& preset  = filteredPresets[static_cast<size_t>(lastRowSelected)];
            notifyPresetLoaded(preset);
            if (onPresetSelected)
                onPresetSelected(preset);
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
                auto& preset = filteredPresets[static_cast<size_t>(selectedIndex)];
                notifyPresetLoaded(preset);
                if (onPresetSelected)
                    onPresetSelected(preset);
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

        // Auto-switch sort mode based on whether search text is present
        const bool hasSearch = searchBox.getText().trim().isNotEmpty();
        if (hasSearch)
        {
            currentSortMode = SortMode::Relevance;
            sortBox.setSelectedId(static_cast<int>(SortMode::Relevance) + 1, juce::dontSendNotification);
        }
        else
        {
            currentSortMode = userSortMode;
            sortBox.setSelectedId(static_cast<int>(userSortMode) + 1, juce::dontSendNotification);
        }

        startTimer(150); // debounce — timerCallback calls applyFilters()
    }

    // juce::Timer
    void timerCallback() override
    {
        if (isScanning_)
        {
            // Spinner animation — advance arc and repaint overlay (#923).
            spinnerAngle_ += juce::MathConstants<float>::twoPi / 40.0f; // ~40 steps per rotation at 20fps
            if (spinnerAngle_ >= juce::MathConstants<float>::twoPi)
                spinnerAngle_ -= juce::MathConstants<float>::twoPi;
            repaint();
            return;
        }

        // Debounce path (search text changed)
        stopTimer();
        // applyFilters() is non-blocking (async) so there is no need to guard
        // against re-entry — a new dispatch simply bumps filterGeneration_ and
        // supersedes any in-flight background job.
        applyFilters();
    }

    //==========================================================================
    // Public API
    //==========================================================================

    /** Refresh the displayed list (call after preset library changes). */
    void refresh() { applyFilters(); }

    // ── #923: Library scan progress indicator ────────────────────────────────
    // Call setScanning(true) before scanPresetDirectoryAsync() launches; call
    // setScanning(false) in the completion callback.  While scanning:
    //   - The listBox is hidden and a spinner overlay takes its place.
    //   - Mood/sort/search controls are disabled.
    //   - A "Scanning 19,574 presets…" status label is shown.
    //   - A lightweight timer rotates the spinner arc at ~20fps.
    void setScanning(bool scanning)
    {
        isScanning_ = scanning;

        // Disable interactive controls while the library is not yet ready.
        searchBox.setEnabled(!scanning);
        sortBox.setEnabled(!scanning);
        similarBtn.setEnabled(!scanning);
        for (auto* btn : moodButtons)
            btn->setEnabled(!scanning);
        for (auto* btn : {&viewAllBtn, &viewFavoritesBtn, &viewRecentBtn})
            btn->setEnabled(!scanning);

        if (scanning)
        {
            listBox.setVisible(false);
            statusLabel.setText("Scanning presets\xe2\x80\xa6", juce::dontSendNotification);
            spinnerAngle_ = 0.0f;
            startTimerHz(20); // drives spinner animation
        }
        else
        {
            listBox.setVisible(true);
            stopTimer();
            applyFilters(); // rebuild list from the now-ready library
        }

        repaint();
    }

private:
    PresetManager& presetManager;
    juce::TextEditor searchBox;
    juce::ComboBox sortBox;
    juce::OwnedArray<juce::TextButton> moodButtons;
    juce::ListBox listBox{"PresetList", nullptr};
    juce::TextButton similarBtn;
    juce::Label statusLabel;

    // ── View-filter buttons — All / Favorites / Recent (#718) ────────────────
    juce::TextButton viewAllBtn;
    juce::TextButton viewFavoritesBtn;
    juce::TextButton viewRecentBtn;
    ViewFilter activeViewFilter = ViewFilter::All;

    // ── Favorites + Recent persistence (#718) ────────────────────────────────
    // PropertiesFile is opened in the constructor and held alive for the
    // lifetime of the browser (same XOceanus.settings as SettingsPanel).
    std::unique_ptr<juce::PropertiesFile> settingsFile;
    juce::StringArray favoriteIds;  // pipe-delimited preset IDs in settings
    juce::StringArray recentIds;    // most-recent first, max kMaxRecentPresets
    static constexpr int kMaxRecentPresets = 20;

    std::vector<PresetData> filteredPresets;
    int selectedIndex  = -1;
    bool similarActive = false;

    // Sort state: userSortMode persists the user's manual choice; currentSortMode
    // may temporarily become Relevance while search text is active.
    SortMode userSortMode   = SortMode::NameAZ;
    SortMode currentSortMode = SortMode::NameAZ;

    // Async filter state — filter+sort runs on a background thread (#753).
    // filterGeneration_ is incremented on every dispatch so stale completions
    // (from superseded keystrokes) are silently discarded.
    // Results travel through move-captured lambda locals; no shared buffer needed.
    std::atomic<uint32_t> filterGeneration_{ 0 };
    std::atomic<bool>     filterPending_{ false };

    // #923: Library scan progress state.
    // isScanning_ is set via setScanning() by the editor when the async preset
    // directory scan is in progress.  spinnerAngle_ is advanced by timerCallback()
    // and used in paint() to draw a rotating arc indicator.
    bool  isScanning_    = false;
    float spinnerAngle_  = 0.0f;

    //==========================================================================
    // Filtering — async implementation (#753)
    //
    // The expensive linear scan + std::sort over ~19,200+ presets is moved off
    // the JUCE message thread.  applyFilters() captures the current UI state,
    // dispatches a background job, then returns immediately.  When the job
    // finishes it posts results back via callAsync for a zero-copy swap into
    // filteredPresets.
    //
    // Stale-result coalescing: each dispatch increments filterGeneration_.  The
    // background lambda captures the generation it was launched with; if a newer
    // dispatch has arrived by the time it tries to commit, it discards its results
    // silently.  This means rapid keystrokes never pile up visible intermediate
    // redraws — only the last one wins.
    //
    // Lifetime safety: the callAsync lambda captures a SafePointer<PresetBrowser>
    // by value.  If the component is destroyed before the message arrives the
    // SafePointer is null and we bail out without touching any dangling memory.
    //==========================================================================

    void applyFilters()
    {
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

        // --- Capture all message-thread UI state before entering the background ---
        const juce::String searchText    = searchBox.getText().trim().toLowerCase();
        const SortMode     sortMode      = currentSortMode;
        const bool         isSimilar     = similarActive;
        const ViewFilter   viewFilter    = activeViewFilter;
        // Capture snapshots of favorites + recent so background thread has stable copies.
        const juce::StringArray favSnapshot    = favoriteIds;
        const juce::StringArray recentSnapshot = recentIds;

        juce::String selectedMood;
        for (auto* btn : moodButtons)
        {
            if (btn->getToggleState())
            {
                selectedMood = btn->getButtonText();
                break;
            }
        }

        // Grab a shared_ptr snapshot of the library — O(1) reference-counted
        // pointer copy, NOT a 19 MB vector copy.  The background thread holds
        // this shared_ptr for the lifetime of its filter pass; a concurrent
        // scanPresetDirectoryAsync() swap never invalidates the pointed-to data.
        auto librarySnapshot = presetManager.getLibrary();
        const int totalCount = static_cast<int>(librarySnapshot->size());

        // Bump generation so any in-flight job from a previous keystroke knows
        // its results are obsolete.
        const uint32_t myGeneration = ++filterGeneration_;
        filterPending_.store(true);

        // Show a lightweight "filtering…" hint in the status bar so the user
        // knows work is in progress during large searches.
        statusLabel.setText("Filtering\xe2\x80\xa6", juce::dontSendNotification);

        // Capture a SafePointer by value for the message-thread callback.
        juce::Component::SafePointer<PresetBrowser> safeThis(this);

        juce::Thread::launch([safeThis,
                              searchText,
                              selectedMood,
                              sortMode,
                              isSimilar,
                              viewFilter,
                              favSnapshot,
                              recentSnapshot,
                              librarySnapshot,   // shared_ptr copy — O(1), ref-counted
                              totalCount,
                              myGeneration,
                              // Raw pointer only used inside callAsync (message thread),
                              // never dereferenced on the background thread.
                              rawThis = this]() mutable
        {
            // ---- Background thread: filter ----
            std::vector<PresetData> filtered;
            filtered.reserve(librarySnapshot->size());

            for (const auto& preset : *librarySnapshot)
            {
                // ── View-filter: Favorites / Recent (#718) ────────────────────
                if (viewFilter == ViewFilter::Favorites)
                {
                    juce::String eng = preset.engines.isEmpty() ? "" : preset.engines[0];
                    if (!favSnapshot.contains(preset.name + "|" + eng))
                        continue;
                }
                else if (viewFilter == ViewFilter::Recent)
                {
                    juce::String eng = preset.engines.isEmpty() ? "" : preset.engines[0];
                    if (!recentSnapshot.contains(preset.name + "|" + eng))
                        continue;
                }

                // Mood filter (skip "All")
                if (selectedMood.isNotEmpty() && selectedMood != "All")
                {
                    if (preset.mood.compareIgnoreCase(selectedMood) != 0)
                        continue;
                }

                // Text search — matches name, description, tags, and engines
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

                filtered.push_back(preset);
            }

            // For Recent view: re-order filtered results to match recentSnapshot order
            if (viewFilter == ViewFilter::Recent && !filtered.empty())
            {
                std::sort(filtered.begin(), filtered.end(),
                          [&](const PresetData& a, const PresetData& b)
                          {
                              juce::String aEng = a.engines.isEmpty() ? "" : a.engines[0];
                              juce::String bEng = b.engines.isEmpty() ? "" : b.engines[0];
                              int aIdx = recentSnapshot.indexOf(a.name + "|" + aEng);
                              int bIdx = recentSnapshot.indexOf(b.name + "|" + bEng);
                              if (aIdx < 0) aIdx = 99999;
                              if (bIdx < 0) bIdx = 99999;
                              return aIdx < bIdx;
                          });
            }

            // ---- Background thread: sort ----
            switch (sortMode)
            {
                case SortMode::NameAZ:
                    std::sort(filtered.begin(), filtered.end(),
                              [](const PresetData& a, const PresetData& b)
                              { return a.name.compareIgnoreCase(b.name) < 0; });
                    break;

                case SortMode::NameZA:
                    std::sort(filtered.begin(), filtered.end(),
                              [](const PresetData& a, const PresetData& b)
                              { return a.name.compareIgnoreCase(b.name) > 0; });
                    break;

                case SortMode::Mood:
                    std::sort(filtered.begin(), filtered.end(),
                              [](const PresetData& a, const PresetData& b)
                              {
                                  const int moodCmp = a.mood.compareIgnoreCase(b.mood);
                                  if (moodCmp != 0)
                                      return moodCmp < 0;
                                  return a.name.compareIgnoreCase(b.name) < 0;
                              });
                    break;

                case SortMode::Engine: // #716
                    // Sort by first engine name (alphabetical), then by preset name.
                    std::sort(filtered.begin(), filtered.end(),
                              [](const PresetData& a, const PresetData& b)
                              {
                                  const juce::String engA = a.engines.isEmpty() ? juce::String{} : a.engines[0];
                                  const juce::String engB = b.engines.isEmpty() ? juce::String{} : b.engines[0];
                                  const int engCmp = engA.compareIgnoreCase(engB);
                                  if (engCmp != 0)
                                      return engCmp < 0;
                                  return a.name.compareIgnoreCase(b.name) < 0;
                              });
                    break;

                case SortMode::DateRecent: // #716
                    // Sort by source file modification time, newest first.
                    // Presets without a source file sort to the end alphabetically.
                    std::sort(filtered.begin(), filtered.end(),
                              [](const PresetData& a, const PresetData& b)
                              {
                                  const bool aHasFile = a.sourceFile.existsAsFile();
                                  const bool bHasFile = b.sourceFile.existsAsFile();
                                  if (aHasFile && bHasFile)
                                  {
                                      const auto tA = a.sourceFile.getLastModificationTime();
                                      const auto tB = b.sourceFile.getLastModificationTime();
                                      if (tA != tB)
                                          return tA > tB; // newest first
                                  }
                                  else if (aHasFile != bHasFile)
                                  {
                                      return aHasFile; // files before in-memory presets
                                  }
                                  return a.name.compareIgnoreCase(b.name) < 0;
                              });
                    break;

                case SortMode::Relevance:
                {
                    // Tier 1 = exact name match, 2 = name starts-with, 3 = name contains,
                    // 4 = tag/engine contains, 5 = description contains.
                    // Within a tier, sort alphabetically.
                    auto tier = [&](const PresetData& p) -> int
                    {
                        const juce::String lname = p.name.toLowerCase();
                        if (lname == searchText)           return 1;
                        if (lname.startsWith(searchText))  return 2;
                        if (lname.contains(searchText))    return 3;
                        for (const auto& tag : p.tags)
                            if (tag.toLowerCase().contains(searchText))
                                return 4;
                        for (const auto& eng : p.engines)
                            if (eng.toLowerCase().contains(searchText))
                                return 4;
                        return 5; // description contains (already filtered-in above)
                    };
                    std::sort(filtered.begin(), filtered.end(),
                              [&](const PresetData& a, const PresetData& b)
                              {
                                  const int ta = tier(a), tb = tier(b);
                                  if (ta != tb)
                                      return ta < tb;
                                  return a.name.compareIgnoreCase(b.name) < 0;
                              });
                    break;
                }
            }

            // Build the status string while still on the background thread to
            // keep string allocation off the message thread.
            juce::String statusText =
                juce::String(static_cast<int>(filtered.size()))
                + " / " + juce::String(totalCount) + " presets";
            if (isSimilar)
                statusText = "Similar: " + statusText;
            else if (viewFilter == ViewFilter::Favorites)
                statusText = juce::String::fromUTF8("\xe2\x98\x85 ") + statusText;
            else if (viewFilter == ViewFilter::Recent)
                statusText = "Recent: " + statusText;

            // ---- Post results back to the message thread ----
            juce::MessageManager::callAsync([safeThis,
                                             rawThis,
                                             myGeneration,
                                             filtered  = std::move(filtered),
                                             statusText = std::move(statusText)]() mutable
            {
                // Bail out if the component was destroyed while we were running.
                if (safeThis == nullptr)
                    return;

                // Discard stale results if a newer filter dispatch has since
                // been launched (e.g. user typed another character mid-flight).
                if (rawThis->filterGeneration_.load() != myGeneration)
                    return;

                rawThis->filteredPresets = std::move(filtered);
                rawThis->listBox.updateContent();
                rawThis->listBox.deselectAllRows();
                rawThis->selectedIndex = -1;
                rawThis->statusLabel.setText(statusText, juce::dontSendNotification);
                rawThis->filterPending_.store(false);
            });
        });
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

    //==========================================================================
    // Favorites + Recent persistence helpers (#718)
    //==========================================================================

    /** Load favoriteIds from the settings file.  Pipe-delimited string → StringArray. */
    void loadFavoritesFromDisk()
    {
        if (!settingsFile)
            return;
        juce::String raw = settingsFile->getValue("presetFavorites", "");
        favoriteIds.clear();
        if (raw.isNotEmpty())
            favoriteIds.addTokens(raw, "|", "");
    }

    /** Persist favoriteIds to the settings file as a pipe-delimited string. */
    void saveFavoritesToDisk()
    {
        if (!settingsFile)
            return;
        settingsFile->setValue("presetFavorites", favoriteIds.joinIntoString("|"));
        settingsFile->saveIfNeeded();
    }

    /** Load recentIds from the settings file.  Pipe-delimited string → StringArray. */
    void loadRecentFromDisk()
    {
        if (!settingsFile)
            return;
        juce::String raw = settingsFile->getValue("presetRecent", "");
        recentIds.clear();
        if (raw.isNotEmpty())
            recentIds.addTokens(raw, "|", "");
        // Enforce max on restore (handles schema changes between versions)
        while (recentIds.size() > kMaxRecentPresets)
            recentIds.remove(recentIds.size() - 1);
    }

    /** Persist recentIds to the settings file. */
    void saveRecentToDisk()
    {
        if (!settingsFile)
            return;
        settingsFile->setValue("presetRecent", recentIds.joinIntoString("|"));
        settingsFile->saveIfNeeded();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};

} // namespace xoceanus
