// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// ExportTabPanel.h — Persistent sidebar panel for Column C's Export tab.
//
// This is a lightweight entry-point panel, NOT a port of ExportDialog.
// It shows the current kit context, a quick format selector, a large EXPORT
// button that launches the full ExportDialog in a CallOutBox, and an export
// status/history strip.
//
// Layout (320pt wide, full sidebar height):
//   1. Section header        — 32pt  (XO Gold bar, "EXPORT" label)
//   2. Kit info              — 80pt  (preset name + engine list chips)
//   3. Format selector       — 48pt  (XPN / WAV / MIDI pill buttons)
//   4. Export button         — 52pt  (large gold CTA)
//   5. Quick settings        — 80pt  (sample rate + bit depth combos)
//   6. Status strip          — remainder (last export / progress / ready)
//
// Usage:
//   auto* panel = new ExportTabPanel(processor);
//   contentArea.addAndMakeVisible(*panel);
//   // Call panel->refresh() whenever the active preset changes.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../../Core/PresetManager.h"
#include "../../XOceanusProcessor.h"
#include "../ExportDialog/ExportDialog.h"

namespace xoceanus
{

//==============================================================================
// ExportTabPanel
//
class ExportTabPanel : public juce::Component
{
public:
    //==========================================================================
    explicit ExportTabPanel(XOceanusProcessor& proc) : processor(proc)
    {
        // ── Format selector pills ─────────────────────────────────────────────
        static const char* kFormatLabels[kNumFormats] = {"XPN", "WAV", "MIDI"};
        for (int i = 0; i < kNumFormats; ++i)
        {
            auto* btn = formatPills.add(new juce::TextButton(kFormatLabels[i]));
            btn->setClickingTogglesState(false);
            btn->setWantsKeyboardFocus(true);
            A11y::setup(*btn, juce::String(kFormatLabels[i]) + " format",
                        juce::String("Select ") + kFormatLabels[i] + " export format");
            btn->onClick = [this, i] { selectFormat(i); };
            addAndMakeVisible(btn);
        }

        // WAV and MIDI exporters are not yet implemented — disable with tooltip
        // so users understand they're coming rather than thinking the button is broken.
        formatPills[1]->setEnabled(false);
        formatPills[1]->setTooltip("WAV export coming in V1.1");
        formatPills[2]->setEnabled(false);
        formatPills[2]->setTooltip("MIDI export coming in V1.1");

        // ── Export CTA button ────────────────────────────────────────────────
        exportBtn.setButtonText("EXPORT");
        exportBtn.setWantsKeyboardFocus(true);
        A11y::setup(exportBtn, "Export", "Open export dialog to build the selected format pack");
        exportBtn.onClick = [this] { launchExportDialog(); };
        addAndMakeVisible(exportBtn);

        // ── Sample rate combo ────────────────────────────────────────────────
        sampleRateBox.addItem("44100 Hz", 1);
        sampleRateBox.addItem("48000 Hz", 2);
        sampleRateBox.addItem("96000 Hz", 3);
        sampleRateBox.setSelectedId(2, juce::dontSendNotification); // 48kHz default
        sampleRateBox.setWantsKeyboardFocus(true);
        A11y::setup(sampleRateBox, "Sample Rate", "Choose the output sample rate for export");
        addAndMakeVisible(sampleRateBox);

        // ── Bit depth combo ──────────────────────────────────────────────────
        bitDepthBox.addItem("16-bit", 1);
        bitDepthBox.addItem("24-bit", 2);
        bitDepthBox.addItem("32-bit", 3);
        bitDepthBox.setSelectedId(2, juce::dontSendNotification); // 24-bit default
        bitDepthBox.setWantsKeyboardFocus(true);
        A11y::setup(bitDepthBox, "Bit Depth", "Choose the output bit depth for export");
        addAndMakeVisible(bitDepthBox);

        // ── Status label ─────────────────────────────────────────────────────
        statusLabel.setFont(GalleryFonts::body(10.0f));
        statusLabel.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()));
        statusLabel.setJustificationType(juce::Justification::centredLeft);
        statusLabel.setText("Ready to export", juce::dontSendNotification);
        statusLabel.setWantsKeyboardFocus(false);
        addAndMakeVisible(statusLabel);

        // ── Progress bar (hidden until export in progress) ───────────────────
        progressBar.setPercentageDisplay(true);
        progressBar.setVisible(false);
        addAndMakeVisible(progressBar);

        setWantsKeyboardFocus(true);

        // Apply initial active-pill highlight
        selectFormat(0); // XPN default

        // Populate kit info
        refresh();
    }

    ~ExportTabPanel() override = default;

    //==========================================================================
    // setExportProgress — call from the export pipeline (on the message thread)
    // to drive the progress bar.  Pass progress in [0, 1]; a value < 0 signals
    // completion and resets the bar.
    //
    // Typical call sequence from the pipeline:
    //   panel->setExportProgress(0.0, "Starting…");
    //   panel->setExportProgress(0.33, "Rendering samples…");
    //   panel->setExportProgress(0.66, "Building XPN…");
    //   panel->setExportProgress(1.0, "Done");
    //   panel->setExportProgress(-1.0, "Export complete");
    void setExportProgress(double progress, const juce::String& stageText = {})
    {
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

        if (progress < 0.0)
        {
            // Completion: hide bar, reset state
            exportInProgress = false;
            progressValue = 0.0;
            progressBar.setVisible(false);
            statusLabel.setText(stageText.isEmpty() ? "Export complete" : stageText, juce::dontSendNotification);
        }
        else
        {
            exportInProgress = true;
            progressValue = juce::jlimit(0.0, 1.0, progress);
            progressBar.setVisible(true);
            if (stageText.isNotEmpty())
                statusLabel.setText(stageText, juce::dontSendNotification);
        }
        repaint();
    }

    //==========================================================================
    // Call after preset or engine changes to update the kit info display.
    void refresh()
    {
        const auto& preset = processor.getPresetManager().getCurrentPreset();

        // Kit name
        kitNameCache = preset.name.isEmpty() ? "No Preset Loaded" : preset.name;

        // Engine list (chip labels): up to 5 engine names
        engineChipLabels.clear();
        for (const auto& eng : preset.engines)
            engineChipLabels.add(eng.isEmpty() ? "—" : eng);

        // Status — reset to ready if not currently exporting
        if (!exportInProgress)
            statusLabel.setText("Ready to export", juce::dontSendNotification);

        repaint();
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        // ── Panel background ─────────────────────────────────────────────────
        g.fillAll(get(shellWhite()));

        // ── Section header bar — elevated bg, subtle XO Gold text ───────────
        auto headerArea = getLocalBounds().removeFromTop(kHeaderH);
        g.setColour(get(elevated())); // #242426 — elevated surface, not bright gold
        g.fillRect(headerArea);
        // Bottom separator
        g.setColour(get(borderGray()));
        g.drawHorizontalLine(kHeaderH - 1, 0.0f, static_cast<float>(getWidth()));
        // "EXPORT" label — XO Gold at 0.8 alpha (subtle, not a neon banner)
        g.setColour(juce::Colour(0xffE9C46A).withAlpha(0.80f));
        g.setFont(GalleryFonts::display(11.0f));
        g.drawText("EXPORT", headerArea.reduced(12, 0), juce::Justification::centredLeft);

        // Dividers between sections
        g.setColour(get(borderGray()));
        auto drawDivider = [&](int y) { g.drawHorizontalLine(y, 8.0f, static_cast<float>(getWidth() - 8)); };
        drawDivider(kHeaderH + kKitInfoH);
        drawDivider(kHeaderH + kKitInfoH + kFormatH);
        drawDivider(kHeaderH + kKitInfoH + kFormatH + kExportBtnH);
        drawDivider(kHeaderH + kKitInfoH + kFormatH + kExportBtnH + kQuickSettingsH);

        // ── Section labels ───────────────────────────────────────────────────
        g.setFont(GalleryFonts::heading(8.5f));
        g.setColour(get(textMid()));

        g.drawText("KIT", 12, kHeaderH + 4, 200, 14, juce::Justification::topLeft);

        g.drawText("FORMAT", 12, kHeaderH + kKitInfoH + 4, 200, 14, juce::Justification::topLeft);

        g.drawText("SETTINGS", 12, kHeaderH + kKitInfoH + kFormatH + kExportBtnH + 4, 200, 14,
                   juce::Justification::topLeft);

        g.drawText("STATUS", 12, kHeaderH + kKitInfoH + kFormatH + kExportBtnH + kQuickSettingsH + 4, 200, 14,
                   juce::Justification::topLeft);

        // ── Kit name ─────────────────────────────────────────────────────────
        {
            int kitY = kHeaderH + 20;
            g.setFont(GalleryFonts::display(13.0f));
            g.setColour(get(textDark()));
            g.drawText(kitNameCache, 12, kitY, getWidth() - 24, 18, juce::Justification::centredLeft, true);

            // Engine chips row
            paintEngineChips(g, kHeaderH + 42);
        }

        // ── Export button gold glow — painted behind the child TextButton ─────
        // Soft concentric ellipses simulate a blurred halo at 30% opacity total.
        {
            const int w = getWidth();
            const int btnY = kHeaderH + kKitInfoH + kFormatH + 10;
            const int btnH = kExportBtnH - 20;

            // Centre the glow on the button rectangle
            float cx = w * 0.5f;
            float cy = (float)btnY + btnH * 0.5f;
            float rx = (w - 24) * 0.5f; // matches exportBtn width / 2
            float ry = btnH * 0.5f;

            // Three concentric passes: outer (largest, most transparent) → inner
            // Combined they approximate a soft Gaussian blur at ~30% peak alpha.
            const juce::Colour goldBase = juce::Colour(0xffE9C46A);
            g.setColour(goldBase.withAlpha(0.08f));
            g.fillEllipse(cx - rx * 1.6f, cy - ry * 2.2f, rx * 3.2f, ry * 4.4f);

            g.setColour(goldBase.withAlpha(0.12f));
            g.fillEllipse(cx - rx * 1.3f, cy - ry * 1.7f, rx * 2.6f, ry * 3.4f);

            g.setColour(goldBase.withAlpha(0.10f));
            g.fillEllipse(cx - rx * 1.0f, cy - ry * 1.2f, rx * 2.0f, ry * 2.4f);
        }
    }

    void resized() override
    {
        const int w = getWidth();

        // ── Format pills ──────────────────────────────────────────────────────
        {
            int pillY = kHeaderH + kKitInfoH + 20;
            int pillH = 26;
            int totalW = w - 24;
            int pillW = totalW / kNumFormats;
            int x = 12;
            for (int i = 0; i < kNumFormats; ++i)
            {
                int thisW = (i == kNumFormats - 1) ? (12 + totalW - x) : pillW;
                formatPills[i]->setBounds(x, pillY, thisW - 4, pillH);
                x += pillW;
            }
        }

        // ── Export button ─────────────────────────────────────────────────────
        {
            int btnY = kHeaderH + kKitInfoH + kFormatH + 10;
            int btnH = kExportBtnH - 20;
            exportBtn.setBounds(12, btnY, w - 24, btnH);
        }

        // ── Quick settings row ────────────────────────────────────────────────
        {
            int settingsY = kHeaderH + kKitInfoH + kFormatH + kExportBtnH + 22;
            int comboH = 26;
            int halfW = (w - 28) / 2;
            sampleRateBox.setBounds(12, settingsY, halfW, comboH);
            bitDepthBox.setBounds(16 + halfW, settingsY, halfW, comboH);
        }

        // ── Status area ───────────────────────────────────────────────────────
        {
            int statusTop = kHeaderH + kKitInfoH + kFormatH + kExportBtnH + kQuickSettingsH;
            int statusH = getHeight() - statusTop;
            if (statusH < 10)
                statusH = 10;

            int labelY = statusTop + 20;
            statusLabel.setBounds(12, labelY, w - 24, 16);

            // Progress bar — shown below status label when active
            progressBar.setBounds(12, labelY + 22, w - 24, 12);
        }
    }

    //==========================================================================
    void lookAndFeelChanged() override { repaint(); }

private:
    //==========================================================================
    // Layout constants
    static constexpr int kHeaderH = 32;
    static constexpr int kKitInfoH = 80;
    static constexpr int kFormatH = 48;
    static constexpr int kExportBtnH = 52;
    static constexpr int kQuickSettingsH = 80;
    static constexpr int kNumFormats = 3;

    //==========================================================================
    XOceanusProcessor& processor;

    // Kit info (cached strings painted directly — no child labels needed)
    juce::String kitNameCache;
    juce::StringArray engineChipLabels;

    // Format pills
    juce::OwnedArray<juce::TextButton> formatPills;
    int activeFormat = 0; // 0=XPN, 1=WAV, 2=MIDI

    // Export CTA
    juce::TextButton exportBtn;

    // Quick settings
    juce::ComboBox sampleRateBox;
    juce::ComboBox bitDepthBox;

    // Status
    juce::Label statusLabel;
    juce::ProgressBar progressBar{progressValue};
    double progressValue = 0.0;
    bool exportInProgress = false;

    //==========================================================================
    // Paint engine name chips (small rounded pills per engine in the kit)
    void paintEngineChips(juce::Graphics& g, int y) const
    {
        using namespace GalleryColors;

        if (engineChipLabels.isEmpty())
        {
            g.setFont(GalleryFonts::body(10.0f));
            g.setColour(get(textMid()));
            g.drawText("No engines loaded", 12, y, getWidth() - 24, 16, juce::Justification::centredLeft);
            return;
        }

        int x = 12;
        constexpr int chipH = 16;
        constexpr int chipPadX = 6;
        constexpr int chipGap = 4;
        constexpr int maxRight = 4; // margin from right edge

        g.setFont(GalleryFonts::label(9.0f));

        for (const auto& eng : engineChipLabels)
        {
            int chipW = juce::roundToInt(GalleryFonts::label(9.0f).getStringWidthFloat(eng)) + chipPadX * 2;

            // Wrap to next row if needed
            if (x + chipW > getWidth() - maxRight)
            {
                x = 12;
                y += chipH + 3;
            }

            auto chipBounds = juce::Rectangle<float>((float)x, (float)y, (float)chipW, (float)chipH);

            // Chip background — engine accent colour at low opacity
            auto accent = accentForEngine(eng);
            g.setColour(accent.withAlpha(0.18f));
            g.fillRoundedRectangle(chipBounds, 4.0f);

            // Chip border
            g.setColour(accent.withAlpha(0.55f));
            g.drawRoundedRectangle(chipBounds.reduced(0.5f), 4.0f, 0.8f);

            // Chip text
            g.setColour(darkMode() ? accent.brighter(0.3f) : accent.darker(0.25f));
            g.drawText(eng, x + chipPadX, y, chipW - chipPadX * 2, chipH, juce::Justification::centredLeft);

            x += chipW + chipGap;
        }
    }

    //==========================================================================
    // Activate a format pill and update visual state.
    // Active:   XO Gold text + subtle gold bg tint (15% alpha) — feels selected, not garish
    // Inactive: elevated bg, T2 text, no special fill (border drawn in paint/LookAndFeel)
    void selectFormat(int idx)
    {
        activeFormat = idx;

        const juce::Colour goldActive = juce::Colour(0xffE9C46A);
        const juce::Colour inactiveText = GalleryColors::get(GalleryColors::t2());
        const juce::Colour inactiveBg = GalleryColors::get(GalleryColors::elevated());
        const juce::Colour activeBg = goldActive.withAlpha(0.15f);

        for (int i = 0; i < kNumFormats; ++i)
        {
            bool active = (i == idx);
            formatPills[i]->setColour(juce::TextButton::buttonColourId, active ? activeBg : inactiveBg);
            formatPills[i]->setColour(juce::TextButton::buttonOnColourId, activeBg);
            formatPills[i]->setColour(juce::TextButton::textColourOffId, active ? goldActive : inactiveText);
            formatPills[i]->setColour(juce::TextButton::textColourOnId, goldActive);
        }

        repaint();
    }

    //==========================================================================
    // Launch the full ExportDialog in a CallOutBox (reuses editor pattern).
    // The quick-settings combos (sampleRateBox / bitDepthBox) are forwarded to
    // the dialog as soft defaults via setInitialSettings() — the user can still
    // change any value inside the dialog after it opens.
    void launchExportDialog()
    {
        auto dialog = std::make_unique<ExportDialog>(processor.getPresetManager(),
                                                     &processor.getAPVTS(),
                                                     &processor.getCouplingMatrix());

        // Forward panel quick-settings as initial defaults.
        // Combo IDs are identical between panel and dialog for SR (1/2/3).
        // Bit depth ID 3 (32-bit) is clamped inside setInitialSettings() to 24-bit.
        dialog->setInitialSettings(sampleRateBox.getSelectedId(),
                                   bitDepthBox.getSelectedId());

        juce::CallOutBox::launchAsynchronously(std::move(dialog),
                                               exportBtn.getScreenBounds(),
                                               getTopLevelComponent());
    }

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportTabPanel)
};

} // namespace xoceanus
