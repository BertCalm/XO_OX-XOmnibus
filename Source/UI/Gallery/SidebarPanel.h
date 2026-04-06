// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// SidebarPanel.h — Column C tabbed sidebar (320pt).
//
// Six tabs: PRESET | COUPLE | FX | PERFORM | EXPORT | SETTINGS
//
// Tab bar spec:
//   Height:    32pt
//   Font:      Space Grotesk SemiBold 10pt ALL CAPS  (GalleryFonts::display)
//   Active:    full-opacity text + 2px engine accent color underline
//   Inactive:  rgba(80,76,70,0.75)  — WCAG-compliant (audit fix A-01)
//   Bg:        GalleryColors::shellWhite()
//
// Keyboard navigation (audit A-09):
//   Left/Right  — cycle tabs
//   Enter/Space — activate focused tab
//   Tab         — exit tab bar to content area
//
// All six tabs are live: wire setPresetManager() then setProcessor() from the editor.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
// PresetBrowser.h is included by XOceanusEditor.h before SidebarPanel.h,
// so xoceanus::PresetBrowser is already declared.  Guard the direct include
// so this header can also be compiled standalone without the full editor chain.
#ifndef XOLOKUN_PRESET_BROWSER_INCLUDED
#define XOLOKUN_PRESET_BROWSER_INCLUDED
#include "../PresetBrowser/PresetBrowser.h"
#endif
#include "CouplingInspectorPanel.h"
#include "FXInspectorPanel.h"
#include "PlayControlPanel.h"
#include "SettingsPanel.h"
#include "ExportTabPanel.h"
#include "../Outshine/OutshineSidebarPanel.h"

namespace xoceanus
{

//==============================================================================
// SidebarPanel — tabbed Column C browser/inspector panel.
//
// Lifetime note: call setPresetManager() before the component becomes visible
// so the C1 Preset tab can construct its PresetBrowser.
//
class SidebarPanel : public juce::Component
{
public:
    //==========================================================================
    enum Tab
    {
        Preset = 0,
        Couple,
        FX,
        Play,
        Export,
        Settings,
        NumTabs
    };

    //==========================================================================
    SidebarPanel()
    {
        // ── Tab buttons ───────────────────────────────────────────────────────
        for (int i = 0; i < NumTabs; ++i)
        {
            auto* btn = tabButtons.add(new juce::TextButton(tabLabels[i]));
            btn->setClickingTogglesState(false);
            btn->setWantsKeyboardFocus(true);

            // Apply WCAG AA compliant colours — inactive tab uses T2 (~4.6:1 on dark bg) not T3
            // (#924: T3 #5E5C5A at 9pt gives ~2.8:1; T2 meets the 4.5:1 AA requirement)
            btn->setColour(juce::TextButton::textColourOffId, GalleryColors::get(GalleryColors::t2()));
            btn->setColour(juce::TextButton::textColourOnId, GalleryColors::get(GalleryColors::textDark()));

            // Accessibility
            A11y::setup(*btn, juce::String(tabLabels[i]) + " tab",
                        juce::String("Switch to ") + tabLabels[i] + " panel");

            // Click handler — capture index by value
            btn->onClick = [this, i] { selectTab(static_cast<Tab>(i)); };

            addAndMakeVisible(btn);
        }

        setWantsKeyboardFocus(true);

        // ── Preset placeholder (C1 only) — shown until setPresetManager() wires the real browser ──
        setupPlaceholder(presetPlaceholder, "Preset Browser", "C1: Loading preset browser…");

        // ── Content area accessibility ────────────────────────────────────────
        contentArea.setTitle("Sidebar content area");
        contentArea.setDescription("Content panel for the selected sidebar tab");
        contentArea.setWantsKeyboardFocus(true);
        addAndMakeVisible(contentArea);

        // Default tab — Preset (shows placeholder until setPresetManager() called)
        selectTab(Preset);
    }

    ~SidebarPanel() override = default;

    //==========================================================================
    // Wire the PresetBrowser for the C1 Preset tab.
    // Must be called before the panel becomes visible.
    // Pass the processor's PresetManager reference.
    void setPresetManager(PresetManager& pm)
    {
        if (presetBrowser == nullptr)
        {
            presetBrowser = std::make_unique<PresetBrowser>(pm);
            contentArea.addChildComponent(*presetBrowser);
            presetBrowser->setVisible(activeTab == Preset);
            presetPlaceholder.setVisible(false); // browser is now live
            resized();                           // reflow so browser gets bounds
        }
    }

    //==========================================================================
    // Wire the ExportTabPanel for the C5 Export tab, and lazily construct the
    // C2–C4, C6 panels (CouplingInspector, FXInspector, PlayControl, Settings).
    // Call once from the editor after setPresetManager(), passing the processor.
    // Safe to call repeatedly — construction is guarded.
    void setProcessor(XOceanusProcessor& proc)
    {
        if (exportPanel == nullptr)
        {
            exportPanel = std::make_unique<ExportTabPanel>(proc);
            contentArea.addChildComponent(*exportPanel);
            exportPanel->setVisible(activeTab == Export);
        }

        if (outshineSidebar == nullptr)
        {
            outshineSidebar = std::make_unique<OutshineSidebarPanel>(proc);
            contentArea.addChildComponent(*outshineSidebar);
            outshineSidebar->setVisible(false); // hidden — ExportTabPanel owns the full Export tab area
        }

        if (couplingPanel == nullptr)
        {
            couplingPanel = std::make_unique<CouplingInspectorPanel>(proc);
            contentArea.addChildComponent(*couplingPanel);
            couplingPanel->setVisible(activeTab == Couple);
        }

        if (fxPanel == nullptr)
        {
            fxPanel = std::make_unique<FXInspectorPanel>(proc.getAPVTS());
            contentArea.addChildComponent(*fxPanel);
            fxPanel->setVisible(activeTab == FX);
        }

        if (playPanel == nullptr)
        {
            playPanel = std::make_unique<PlayControlPanel>(proc);
            contentArea.addChildComponent(*playPanel);
            playPanel->setVisible(activeTab == Play);
        }

        if (settingsPanel == nullptr)
        {
            settingsPanel = std::make_unique<SettingsPanel>(proc);
            contentArea.addChildComponent(*settingsPanel);
            settingsPanel->setVisible(activeTab == Settings);
        }

        resized();

        // Restore previously selected tab now that all content panels are live (#199).
        {
            juce::PropertiesFile::Options opts;
            opts.applicationName = "XOceanus";
            opts.filenameSuffix = "settings";
            opts.osxLibrarySubFolder = "Application Support";
            juce::PropertiesFile settings(opts);
            const int saved = settings.getIntValue("sidebarTab", static_cast<int>(Preset));
            selectTab(static_cast<Tab>(juce::jlimit(0, static_cast<int>(NumTabs) - 1, saved)));
        }
    }

    //==========================================================================
    // Accessor for the SettingsPanel so the editor can wire MIDILearnManager.
    SettingsPanel* getSettingsPanel() noexcept { return settingsPanel.get(); }

    //==========================================================================
    // Notify the Export tab that the active preset changed (call from editor's
    // preset-changed callback).
    void refreshExportPanel()
    {
        if (exportPanel != nullptr)
            exportPanel->refresh();
    }

    //==========================================================================
    // Fix S2: Refresh the preset browser list after an async scan completes.
    // Call from the editor's scanPresetDirectoryAsync completion callback so that
    // any already-open PresetBrowser (constructed before the scan finished) re-reads
    // the library and rebuilds its filtered list.
    void refreshPresetBrowser()
    {
        if (presetBrowser != nullptr)
            presetBrowser->refresh();
    }

    //==========================================================================
    // Engine accent color — used for active tab underline.
    // Defaults to Reef Jade teal (mockup --accent); call setEngineAccent() when the loaded engine changes.
    juce::Colour engineAccent{juce::Colour(0xFF1E8B7E)};

    void setEngineAccent(juce::Colour c)
    {
        engineAccent = c;
        repaint();
    }

    //==========================================================================
    Tab getActiveTab() const noexcept { return activeTab; }

    void selectTab(Tab t)
    {
        activeTab = t;

        // Update tab button colors once here — avoids setColour() calls inside paint()
        for (int i = 0; i < NumTabs; ++i)
        {
            bool isActive = (i == static_cast<int>(activeTab));
            tabButtons[i]->setColour(juce::TextButton::textColourOffId, isActive
                                                                            ? GalleryColors::get(GalleryColors::t1())
                                                                            : GalleryColors::get(GalleryColors::t2())); // #924: T2 meets WCAG AA 4.5:1; T3 was ~2.8:1
            tabButtons[i]->setColour(juce::TextButton::textColourOnId, GalleryColors::get(GalleryColors::t1()));
        }

        // Show/hide content panels
        bool havePresetBrowser = (presetBrowser != nullptr);
        if (havePresetBrowser)
            presetBrowser->setVisible(t == Preset);
        presetPlaceholder.setVisible(!havePresetBrowser && t == Preset);

        if (couplingPanel)
            couplingPanel->setVisible(t == Couple);
        if (fxPanel)
            fxPanel->setVisible(t == FX);
        if (playPanel)
            playPanel->setVisible(t == Play);
        if (exportPanel)
            exportPanel->setVisible(t == Export);
        if (outshineSidebar)
            outshineSidebar->setVisible(false); // always hidden — off-screen
        if (settingsPanel)
            settingsPanel->setVisible(t == Settings);

        // Refresh coupling data whenever the Couple tab becomes active
        if (t == Couple && couplingPanel)
            couplingPanel->refresh();

        repaint();

        // Persist tab selection across sessions (#199).
        {
            juce::PropertiesFile::Options opts;
            opts.applicationName = "XOceanus";
            opts.filenameSuffix = "settings";
            opts.osxLibrarySubFolder = "Application Support";
            juce::PropertiesFile settings(opts);
            settings.setValue("sidebarTab", static_cast<int>(t));
        }

        // Move keyboard focus into the content area — only grab focus on
        // a visible child so invisible panels don't silently swallow input (#205).
        juce::Component* focusTarget = nullptr;
        for (int ci = 0; ci < contentArea.getNumChildComponents(); ++ci)
        {
            auto* child = contentArea.getChildComponent(ci);
            if (child != nullptr && child->isVisible())
            {
                focusTarget = child;
                break;
            }
        }
        if (focusTarget != nullptr)
            focusTarget->grabKeyboardFocus();
        else
            contentArea.grabKeyboardFocus();
    }

    //==========================================================================
    // Keyboard navigation — A-09 compliance
    bool keyPressed(const juce::KeyPress& key) override
    {
        const int n = static_cast<int>(NumTabs);

        if (key == juce::KeyPress::leftKey || key == juce::KeyPress::rightKey)
        {
            const int direction = (key == juce::KeyPress::rightKey) ? 1 : -1;
            int next = static_cast<int>(activeTab) + direction;
            next = (next + n) % n;
            // Skip invisible tabs — cycling must only land on visible tabs (#202, #205).
            // Guard against infinite loop by capping iterations at NumTabs.
            for (int guard = 0; guard < n; ++guard)
            {
                if (tabButtons[next] != nullptr && tabButtons[next]->isVisible())
                    break;
                next = (next + direction + n) % n;
            }
            selectTab(static_cast<Tab>(next));

            // Return focus to the newly active tab button
            tabButtons[next]->grabKeyboardFocus();
            return true;
        }

        if (key == juce::KeyPress::returnKey || key == juce::KeyPress(' '))
        {
            // Already on activeTab — move focus into content
            contentArea.grabKeyboardFocus();
            return true;
        }

        return false;
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        // ── Panel background — shell bg (GalleryColors::shellWhite() = #0E0E10 dark) ─
        g.fillAll(get(shellWhite())); // shell bg in dark mode

        if (getWidth() <= 48)
        {
            // Draw vertical icon strip: single letter per tab
            auto tabH = getHeight() / NumTabs;
            for (int i = 0; i < NumTabs; ++i)
            {
                auto y = i * tabH;
                bool active = (i == static_cast<int>(activeTab));
                // Active: T1, inactive: T2 (#924: T2 meets WCAG AA 4.5:1 in collapsed icon mode)
                g.setColour(juce::Colour(active ? GalleryColors::t1() : GalleryColors::t2()));
                g.setFont(GalleryFonts::display(11.0f));
                // Use 4th character for PERFORM ('F') to avoid collision with PRESET ('P') and EXPORT ('E').
                // PERFORM[3]='F' is unique across all six tab labels in collapsed mode.
                juce::String icon(i == Play ? juce::String(tabLabels[i][3]) : juce::String(tabLabels[i][0]));
                g.drawText(icon, 0, y, getWidth(), tabH, juce::Justification::centred);
                if (active)
                {
                    // Engine accent bar for active tab in collapsed mode
                    g.setColour(engineAccent);
                    g.fillRect(0, y, 2, tabH);
                }
            }
            return;
        }

        // ── Left border separator from Column B — border() layer separator ───
        g.setColour(border());
        g.drawVerticalLine(0, 0.0f, static_cast<float>(getHeight()));

        // ── Tab bar background — surface layer (GalleryColors::surface()) ────
        g.setColour(get(surface())); // surface color — one level above shell
        g.fillRect(0, 0, getWidth(), kTabBarH);

        // ── Bottom border under tab bar — border() separator ──────────────────
        g.setColour(border());
        g.drawHorizontalLine(kTabBarH - 1, 0.0f, static_cast<float>(getWidth()));

        // ── Active tab accent underline — 2px engine accent ──────────────────
        // Prototype: 2px bottom border in engine accent color
        if (tabButtons[activeTab] != nullptr)
        {
            auto btnBounds = tabButtons[activeTab]->getBounds();
            g.setColour(engineAccent);
            g.fillRect(btnBounds.getX(), kTabBarH - kUnderlineH, btnBounds.getWidth(), kUnderlineH);
        }

        // ── Focus ring on focused tab button ─────────────────────────────────
        // (Tab button colors are set in selectTab() to avoid setColour() calls inside paint())
        for (int i = 0; i < NumTabs; ++i)
        {
            if (tabButtons[i] != nullptr && tabButtons[i]->hasKeyboardFocus(false))
                A11y::drawFocusRing(g, tabButtons[i]->getBounds().toFloat(), 2.0f);
        }

        if (hasKeyboardFocus(true))
            A11y::drawFocusRing(g, getLocalBounds().toFloat(), 4.0f);
    }

    void focusGained(FocusChangeType) override { repaint(); }
    void focusLost(FocusChangeType) override { repaint(); }

    void resized() override
    {
        if (getWidth() <= 48)
            return; // Collapsed icon strip — layout degenerates, skip

        const int w = getWidth();

        // ── Tab bar — proportional widths, skip hidden tabs ──────────────────
        juce::Font tabFont = GalleryFonts::display(10.0f);
        float totalNatural = 0.0f;
        float natWidths[NumTabs] = {};
        for (int i = 0; i < NumTabs; ++i)
        {
            if (!tabButtons[i]->isVisible())
                continue;
            natWidths[i] = tabFont.getStringWidthFloat(tabLabels[i]) + 22.0f;
            totalNatural += natWidths[i];
        }
        const float scale = (totalNatural > 0.0f) ? (float)w / totalNatural : 1.0f;
        int x = 0;
        for (int i = 0; i < NumTabs; ++i)
        {
            if (!tabButtons[i]->isVisible())
            {
                tabButtons[i]->setBounds(0, -100, 0, 0); // hidden tab: park off-screen to skip hit-testing
                continue;
            }
            bool isLast = true;
            for (int j = i + 1; j < NumTabs; ++j)
                if (tabButtons[j]->isVisible())
                {
                    isLast = false;
                    break;
                }
            int thisW = isLast ? (w - x) : juce::roundToInt(natWidths[i] * scale);
            tabButtons[i]->setBounds(x, 0, thisW, kTabBarH);
            x += thisW;
        }

        // ── Content area ──────────────────────────────────────────────────────
        auto content = juce::Rectangle<int>(0, kTabBarH, w, getHeight() - kTabBarH);
        contentArea.setBounds(content);

        // Layout each content child to fill the content area
        auto inner = contentArea.getLocalBounds().reduced(8, 8);

        if (presetBrowser != nullptr)
            presetBrowser->setBounds(inner);

        // ExportTabPanel owns the full Export tab content area.
        // OutshineSidebarPanel is hidden and parked off-screen so it doesn't intercept input.
        if (exportPanel != nullptr)
            exportPanel->setBounds(contentArea.getLocalBounds());

        if (outshineSidebar != nullptr)
        {
            outshineSidebar->setBounds(0, 0, 0, 0);
            outshineSidebar->setVisible(false);
        }

        if (couplingPanel != nullptr)
            couplingPanel->setBounds(inner);

        if (fxPanel != nullptr)
            fxPanel->setBounds(inner);

        if (playPanel != nullptr)
            playPanel->setBounds(inner);

        if (settingsPanel != nullptr)
            settingsPanel->setBounds(inner);

        presetPlaceholder.setBounds(inner);
    }

    //==========================================================================
    // lookAndFeelChanged() — repaint when the theme switches
    void lookAndFeelChanged() override { repaint(); }

private:
    //==========================================================================
    // Prototype: 32px tab bar, 2px accent underline
    static constexpr int kTabBarH = 32;
    static constexpr int kUnderlineH = 2;

    static constexpr const char* tabLabels[NumTabs] = {"PRESET", "COUPLE", "FX", "PERFORM", "EXPORT", "SETTINGS"};

    //==========================================================================
    // Inactive tab text colour — WCAG AA on shellWhite (audit fix A-01)
    static juce::Colour inactiveTabColour() { return juce::Colour(80, 76, 70).withAlpha(0.75f); }

    //==========================================================================
    // Helper — configure a placeholder label for tabs without V1 content
    void setupPlaceholder(juce::Label& lbl, const juce::String& title, const juce::String& subtitle)
    {
        lbl.setText(title + "\n\n" + subtitle, juce::dontSendNotification);
        lbl.setFont(GalleryFonts::body(11.0f));
        lbl.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()));
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setVisible(false);
        contentArea.addChildComponent(lbl);
    }

    //==========================================================================
    // Inner container for the content area — handles Tab key focus traversal
    struct ContentArea : public juce::Component
    {
        ContentArea()
        {
            setTitle("Sidebar content");
            setWantsKeyboardFocus(true);
            setFocusContainerType(FocusContainerType::keyboardFocusContainer);
        }
        void paint(juce::Graphics&) override {} // transparent — parent paints bg
    };

    //==========================================================================
    Tab activeTab = Preset;
    juce::OwnedArray<juce::TextButton> tabButtons;

    ContentArea contentArea;

    // C1 — Preset Browser (constructed lazily when setPresetManager() is called)
    std::unique_ptr<PresetBrowser> presetBrowser;

    // C2 — Coupling Inspector (constructed lazily when setProcessor() is called)
    std::unique_ptr<CouplingInspectorPanel> couplingPanel;

    // C3 — FX Inspector (constructed lazily when setProcessor() is called)
    std::unique_ptr<FXInspectorPanel> fxPanel;

    // C4 — Play Control (constructed lazily when setProcessor() is called)
    std::unique_ptr<PlayControlPanel> playPanel;

    // C5 — Export Tab Panel (constructed lazily when setProcessor() is called)
    std::unique_ptr<ExportTabPanel> exportPanel;

    // Outshine sidebar — attached to Export tab below ExportTabPanel
    std::unique_ptr<OutshineSidebarPanel> outshineSidebar;

    // C6 — Settings (constructed lazily when setProcessor() is called)
    std::unique_ptr<SettingsPanel> settingsPanel;

    // Preset placeholder — shown when presetBrowser is null (before setPresetManager() is called)
    juce::Label presetPlaceholder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidebarPanel)
};

} // namespace xoceanus
