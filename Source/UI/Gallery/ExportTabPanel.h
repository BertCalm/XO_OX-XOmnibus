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
#include "../../XOlokunProcessor.h"
#include "../ExportDialog/ExportDialog.h"

namespace xolokun {

//==============================================================================
// ExportTabPanel
//
class ExportTabPanel : public juce::Component
{
public:
    //==========================================================================
    explicit ExportTabPanel(XOlokunProcessor& proc)
        : processor(proc)
    {
        // ── Format selector pills ─────────────────────────────────────────────
        static const char* kFormatLabels[kNumFormats] = { "XPN", "WAV", "MIDI" };
        for (int i = 0; i < kNumFormats; ++i)
        {
            auto* btn = formatPills.add(new juce::TextButton(kFormatLabels[i]));
            btn->setClickingTogglesState(false);
            btn->setWantsKeyboardFocus(true);
            A11y::setup(*btn,
                        juce::String(kFormatLabels[i]) + " format",
                        juce::String("Select ") + kFormatLabels[i] + " export format");
            btn->onClick = [this, i] { selectFormat(i); };
            addAndMakeVisible(btn);
        }

        // ── Export CTA button ────────────────────────────────────────────────
        exportBtn.setButtonText("EXPORT");
        exportBtn.setWantsKeyboardFocus(true);
        A11y::setup(exportBtn, "Export", "Open export dialog to build the selected format pack");
        exportBtn.onClick = [this] { launchExportDialog(); };
        addAndMakeVisible(exportBtn);

        // ── Sample rate combo ────────────────────────────────────────────────
        sampleRateBox.addItem("44100 Hz",  1);
        sampleRateBox.addItem("48000 Hz",  2);
        sampleRateBox.addItem("96000 Hz",  3);
        sampleRateBox.setSelectedId(2, juce::dontSendNotification); // 48kHz default
        sampleRateBox.setWantsKeyboardFocus(true);
        A11y::setup(sampleRateBox, "Sample Rate", "Choose the output sample rate for export");
        addAndMakeVisible(sampleRateBox);

        // ── Bit depth combo ──────────────────────────────────────────────────
        bitDepthBox.addItem("16-bit",  1);
        bitDepthBox.addItem("24-bit",  2);
        bitDepthBox.addItem("32-bit",  3);
        bitDepthBox.setSelectedId(2, juce::dontSendNotification); // 24-bit default
        bitDepthBox.setWantsKeyboardFocus(true);
        A11y::setup(bitDepthBox, "Bit Depth", "Choose the output bit depth for export");
        addAndMakeVisible(bitDepthBox);

        // ── Status label ─────────────────────────────────────────────────────
        statusLabel.setFont(GalleryFonts::body(10.0f));
        statusLabel.setColour(juce::Label::textColourId,
                              GalleryColors::get(GalleryColors::textMid()));
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

        // ── Section header bar ───────────────────────────────────────────────
        auto headerArea = getLocalBounds().removeFromTop(kHeaderH);
        g.setColour(get(xoGold));
        g.fillRect(headerArea);
        g.setColour(juce::Colour(Light::textDark));
        g.setFont(GalleryFonts::display(11.0f));
        g.drawText("EXPORT", headerArea.reduced(12, 0), juce::Justification::centredLeft);

        // Dividers between sections
        g.setColour(get(borderGray()));
        auto drawDivider = [&](int y) {
            g.drawHorizontalLine(y, 8.0f, static_cast<float>(getWidth() - 8));
        };
        drawDivider(kHeaderH + kKitInfoH);
        drawDivider(kHeaderH + kKitInfoH + kFormatH);
        drawDivider(kHeaderH + kKitInfoH + kFormatH + kExportBtnH);
        drawDivider(kHeaderH + kKitInfoH + kFormatH + kExportBtnH + kQuickSettingsH);

        // ── Section labels ───────────────────────────────────────────────────
        g.setFont(GalleryFonts::heading(8.5f));
        g.setColour(get(textMid()));

        g.drawText("KIT",
                   12, kHeaderH + 4, 200, 14,
                   juce::Justification::topLeft);

        g.drawText("FORMAT",
                   12, kHeaderH + kKitInfoH + 4, 200, 14,
                   juce::Justification::topLeft);

        g.drawText("SETTINGS",
                   12, kHeaderH + kKitInfoH + kFormatH + kExportBtnH + 4, 200, 14,
                   juce::Justification::topLeft);

        g.drawText("STATUS",
                   12, kHeaderH + kKitInfoH + kFormatH + kExportBtnH + kQuickSettingsH + 4, 200, 14,
                   juce::Justification::topLeft);

        // ── Kit name ─────────────────────────────────────────────────────────
        {
            int kitY = kHeaderH + 20;
            g.setFont(GalleryFonts::display(13.0f));
            g.setColour(get(textDark()));
            g.drawText(kitNameCache,
                       12, kitY, getWidth() - 24, 18,
                       juce::Justification::centredLeft, true);

            // Engine chips row
            paintEngineChips(g, kHeaderH + 42);
        }
    }

    void resized() override
    {
        const int w = getWidth();

        // ── Format pills ──────────────────────────────────────────────────────
        {
            int pillY  = kHeaderH + kKitInfoH + 20;
            int pillH  = 26;
            int totalW = w - 24;
            int pillW  = totalW / kNumFormats;
            int x      = 12;
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
            int settingsY  = kHeaderH + kKitInfoH + kFormatH + kExportBtnH + 22;
            int comboH     = 26;
            int halfW      = (w - 28) / 2;
            sampleRateBox.setBounds(12,           settingsY, halfW, comboH);
            bitDepthBox.setBounds  (16 + halfW,   settingsY, halfW, comboH);
        }

        // ── Status area ───────────────────────────────────────────────────────
        {
            int statusTop = kHeaderH + kKitInfoH + kFormatH + kExportBtnH + kQuickSettingsH;
            int statusH   = getHeight() - statusTop;
            if (statusH < 10) statusH = 10;

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
    static constexpr int kHeaderH       = 32;
    static constexpr int kKitInfoH      = 80;
    static constexpr int kFormatH       = 48;
    static constexpr int kExportBtnH    = 52;
    static constexpr int kQuickSettingsH = 80;
    static constexpr int kNumFormats    = 3;

    //==========================================================================
    XOlokunProcessor& processor;

    // Kit info (cached strings painted directly — no child labels needed)
    juce::String      kitNameCache;
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
    juce::Label       statusLabel;
    juce::ProgressBar progressBar { progressValue };
    double            progressValue  = 0.0;
    bool              exportInProgress = false;

    //==========================================================================
    // Paint engine name chips (small rounded pills per engine in the kit)
    void paintEngineChips(juce::Graphics& g, int y) const
    {
        using namespace GalleryColors;

        if (engineChipLabels.isEmpty())
        {
            g.setFont(GalleryFonts::body(10.0f));
            g.setColour(get(textMid()));
            g.drawText("No engines loaded", 12, y, getWidth() - 24, 16,
                       juce::Justification::centredLeft);
            return;
        }

        int x = 12;
        constexpr int chipH    = 16;
        constexpr int chipPadX = 6;
        constexpr int chipGap  = 4;
        constexpr int maxRight = 4; // margin from right edge

        g.setFont(GalleryFonts::label(9.0f));

        for (const auto& eng : engineChipLabels)
        {
            int chipW = juce::roundToInt(GalleryFonts::label(9.0f).getStringWidthFloat(eng))
                        + chipPadX * 2;

            // Wrap to next row if needed
            if (x + chipW > getWidth() - maxRight)
            {
                x  = 12;
                y += chipH + 3;
            }

            auto chipBounds = juce::Rectangle<float>(
                (float)x, (float)y, (float)chipW, (float)chipH);

            // Chip background — engine accent colour at low opacity
            auto accent = accentForEngine(eng);
            g.setColour(accent.withAlpha(0.18f));
            g.fillRoundedRectangle(chipBounds, 4.0f);

            // Chip border
            g.setColour(accent.withAlpha(0.55f));
            g.drawRoundedRectangle(chipBounds.reduced(0.5f), 4.0f, 0.8f);

            // Chip text
            g.setColour(darkMode()
                        ? accent.brighter(0.3f)
                        : accent.darker(0.25f));
            g.drawText(eng, x + chipPadX, y, chipW - chipPadX * 2, chipH,
                       juce::Justification::centredLeft);

            x += chipW + chipGap;
        }
    }

    //==========================================================================
    // Activate a format pill and update visual state
    void selectFormat(int idx)
    {
        activeFormat = idx;

        for (int i = 0; i < kNumFormats; ++i)
        {
            bool active = (i == idx);
            formatPills[i]->setColour(
                juce::TextButton::buttonColourId,
                active ? GalleryColors::get(GalleryColors::xoGold).withAlpha(0.20f)
                       : GalleryColors::get(GalleryColors::shellWhite()));
            formatPills[i]->setColour(
                juce::TextButton::buttonOnColourId,
                GalleryColors::get(GalleryColors::xoGold).withAlpha(0.20f));
            formatPills[i]->setColour(
                juce::TextButton::textColourOffId,
                active ? GalleryColors::get(GalleryColors::xoGold)
                       : GalleryColors::get(GalleryColors::textMid()));
        }

        repaint();
    }

    //==========================================================================
    // Launch the full ExportDialog in a CallOutBox (reuses editor pattern)
    void launchExportDialog()
    {
        // Apply the current panel's quick-settings to the dialog's profile
        // by selecting the matching combo IDs before construction.  The dialog
        // reads these from its own internal combos, so we carry intent via
        // applyProfile() after launch instead (see note below).

        juce::CallOutBox::launchAsynchronously(
            std::make_unique<ExportDialog>(
                processor.getPresetManager(),
                &processor.getAPVTS(),
                &processor.getCouplingMatrix()),
            exportBtn.getScreenBounds(),
            getTopLevelComponent());

        // Note: we intentionally do NOT apply the quick-settings from sampleRateBox /
        // bitDepthBox into the ExportDialog here.  The full dialog exposes its own
        // render-settings controls (including profiles), and reconciling two separate
        // combo states would require a shared ExportSettings struct — which is the
        // right V2 approach.  For V1, the quick-settings on this panel are advisory
        // UI only; they do not mutate export state.
    }

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportTabPanel)
};

} // namespace xolokun
