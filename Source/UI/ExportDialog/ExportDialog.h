#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../../Export/XPNExporter.h"
#include "../../Core/PresetManager.h"
#include "../XOmnibusEditor.h"

namespace xomnibus {

//==============================================================================
// ExportDialog — XPN export workflow UI with Gallery Model styling,
// WCAG 2.1 AA accessibility, and full keyboard navigation.
//
// Sections:
//   1. Preset selection (filterable list with checkboxes)
//   2. Render settings (strategy, vel layers, bit depth, sample rate)
//   3. Bundle config (name, description, cover engine)
//   4. Size estimate (live update)
//   5. Progress display with per-note granularity
//   6. Completion summary
//
class ExportDialog : public juce::Component,
                     private juce::Timer
{
public:
    ExportDialog(PresetManager& pm)
        : presetManager(pm)
    {
        setWantsKeyboardFocus(true);
        A11y::setup(*this, "XPN Export", "Export presets as MPC-compatible expansion pack");

        buildPresetSelection();
        buildRenderSettings();
        buildBundleConfig();
        buildSizeEstimate();
        buildProgressSection();
        buildActionButtons();

        setSize(520, 640);
    }

    ~ExportDialog() override
    {
        stopTimer();
    }

    //==========================================================================
    // Paint — Gallery Model styling
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

        // Header
        auto headerArea = getLocalBounds().removeFromTop(kHeaderH);
        g.setColour(GalleryColors::get(GalleryColors::xoGold));
        g.fillRect(headerArea);
        g.setColour(juce::Colour(GalleryColors::Light::textDark));
        g.setFont(GalleryFonts::display(14.0f));
        g.drawText("XPN EXPORT", headerArea.reduced(12, 0), juce::Justification::centredLeft);

        // Section dividers
        g.setColour(GalleryColors::get(GalleryColors::borderGray()));

        auto b = getLocalBounds().reduced(0, 0);
        b.removeFromTop(kHeaderH);

        int sectionY = kHeaderH + kPresetSectionH;
        g.drawLine(12, (float)sectionY, (float)getWidth() - 12, (float)sectionY, 1.0f);

        sectionY += kSettingsSectionH;
        g.drawLine(12, (float)sectionY, (float)getWidth() - 12, (float)sectionY, 1.0f);

        sectionY += kBundleSectionH;
        g.drawLine(12, (float)sectionY, (float)getWidth() - 12, (float)sectionY, 1.0f);

        // Section labels
        g.setFont(GalleryFonts::heading(9.0f));
        g.setColour(GalleryColors::get(GalleryColors::textMid()));
        g.drawText("PRESETS", 12, kHeaderH + 4, 200, 16, juce::Justification::topLeft);
        g.drawText("RENDER SETTINGS", 12, kHeaderH + kPresetSectionH + 4, 200, 16, juce::Justification::topLeft);
        g.drawText("BUNDLE", 12, kHeaderH + kPresetSectionH + kSettingsSectionH + 4, 200, 16, juce::Justification::topLeft);

        // Size estimate label
        if (!exporting)
        {
            auto estArea = getLocalBounds().reduced(12, 0);
            estArea.removeFromTop(kHeaderH + kPresetSectionH + kSettingsSectionH + kBundleSectionH + 4);
            estArea = estArea.removeFromTop(20);
            g.setFont(GalleryFonts::label(8.5f));
            g.setColour(GalleryColors::get(GalleryColors::textMid()));
            g.drawText(sizeEstimateText, estArea, juce::Justification::centredLeft);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromTop(kHeaderH);

        // Preset selection section
        auto presetArea = area.removeFromTop(kPresetSectionH).reduced(12, 20);
        presetList.setBounds(presetArea);

        // Render settings section
        auto settingsArea = area.removeFromTop(kSettingsSectionH).reduced(12, 22);
        int settingsW = settingsArea.getWidth() / 4;
        strategyBox.setBounds(settingsArea.removeFromLeft(settingsW).reduced(2, 0).removeFromTop(28));
        velLayersBox.setBounds(settingsArea.removeFromLeft(settingsW).reduced(2, 0).removeFromTop(28));
        bitDepthBox.setBounds(settingsArea.removeFromLeft(settingsW).reduced(2, 0).removeFromTop(28));
        sampleRateBox.setBounds(settingsArea.removeFromLeft(settingsW).reduced(2, 0).removeFromTop(28));

        // Bundle config section
        auto bundleArea = area.removeFromTop(kBundleSectionH).reduced(12, 22);
        auto nameRow = bundleArea.removeFromTop(28);
        bundleNameField.setBounds(nameRow);
        bundleArea.removeFromTop(6);
        auto engineRow = bundleArea.removeFromTop(28);
        coverEngineBox.setBounds(engineRow.removeFromLeft(engineRow.getWidth() / 2).reduced(0, 0));
        soundShapeToggle.setBounds(engineRow.reduced(4, 0));

        // Size estimate (just painted, no child component)
        area.removeFromTop(24);

        // Progress section
        auto progressArea = area.removeFromTop(kProgressH).reduced(12, 0);
        progressBar.setBounds(progressArea.removeFromTop(20));
        progressLabel.setBounds(progressArea.removeFromTop(16));

        // Action buttons
        auto buttonArea = area.reduced(12, 8);
        int btnW = 120, btnH = 32;
        cancelBtn.setBounds(buttonArea.removeFromRight(btnW).removeFromTop(btnH));
        buttonArea.removeFromRight(8);
        exportBtn.setBounds(buttonArea.removeFromRight(btnW).removeFromTop(btnH));
        buttonArea.removeFromRight(8);
        validateBtn.setBounds(buttonArea.removeFromRight(btnW).removeFromTop(btnH));
    }

    //==========================================================================
    // Keyboard navigation
    //==========================================================================

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            if (exporting)
                cancelExport();
            else if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
                dw->closeButtonPressed();
            return true;
        }
        if (key == juce::KeyPress::returnKey && !exporting)
        {
            startExport();
            return true;
        }
        return false;
    }

    //==========================================================================
    // Export quick profiles
    //==========================================================================

    enum class ExportProfile { MPCStandard, Lightweight, MaxQuality };

    void applyProfile(ExportProfile profile)
    {
        switch (profile)
        {
            case ExportProfile::MPCStandard:
                strategyBox.setSelectedId(1);    // EveryMinor3rd
                velLayersBox.setSelectedId(3);   // 3 layers
                bitDepthBox.setSelectedId(2);    // 24-bit
                sampleRateBox.setSelectedId(2);  // 48kHz
                break;
            case ExportProfile::Lightweight:
                strategyBox.setSelectedId(4);    // OctavesOnly
                velLayersBox.setSelectedId(1);   // 1 layer
                bitDepthBox.setSelectedId(1);    // 16-bit
                sampleRateBox.setSelectedId(1);  // 44.1kHz
                break;
            case ExportProfile::MaxQuality:
                strategyBox.setSelectedId(2);    // Chromatic
                velLayersBox.setSelectedId(3);   // 3 layers
                bitDepthBox.setSelectedId(2);    // 24-bit
                sampleRateBox.setSelectedId(2);  // 48kHz
                break;
        }
        updateSizeEstimate();
    }

private:

    static constexpr int kHeaderH          = 36;
    static constexpr int kPresetSectionH   = 140;
    static constexpr int kSettingsSectionH  = 70;
    static constexpr int kBundleSectionH   = 90;
    static constexpr int kProgressH        = 44;

    PresetManager& presetManager;
    bool exporting = false;
    std::unique_ptr<juce::Thread> exportThread;

    // Preset selection
    juce::ListBox presetList { "PresetList" };

    // Render settings
    juce::ComboBox strategyBox, velLayersBox, bitDepthBox, sampleRateBox;

    // Bundle config
    juce::TextEditor bundleNameField;
    juce::ComboBox coverEngineBox;
    juce::ToggleButton soundShapeToggle { "Sound Shape Auto" };

    // Size estimate
    juce::String sizeEstimateText = "Estimated size: calculating...";

    // Progress
    juce::ProgressBar progressBar { progressValue };
    double progressValue = 0.0;
    juce::Label progressLabel;

    // Action buttons
    juce::TextButton exportBtn { "EXPORT" };
    juce::TextButton cancelBtn { "CANCEL" };
    juce::TextButton validateBtn { "VALIDATE" };

    // Export state
    std::atomic<bool> shouldCancel { false };

    //==========================================================================
    // Build methods
    //==========================================================================

    void buildPresetSelection()
    {
        addAndMakeVisible(presetList);
        A11y::setup(presetList, "Preset List", "Select presets to include in export");
    }

    void buildRenderSettings()
    {
        // Note strategy
        strategyBox.addItem("Minor 3rds", 1);
        strategyBox.addItem("Chromatic", 2);
        strategyBox.addItem("5ths", 3);
        strategyBox.addItem("Octaves", 4);
        strategyBox.setSelectedId(1);
        strategyBox.setTooltip("Note sampling density");
        A11y::setup(strategyBox, "Note Strategy", "Choose how many notes are sampled across the keyboard");
        strategyBox.onChange = [this] { updateSizeEstimate(); };
        addAndMakeVisible(strategyBox);

        // Velocity layers
        velLayersBox.addItem("1 Layer", 1);
        velLayersBox.addItem("2 Layers", 2);
        velLayersBox.addItem("3 Layers", 3);
        velLayersBox.setSelectedId(1);
        velLayersBox.setTooltip("Velocity layers per note");
        A11y::setup(velLayersBox, "Velocity Layers", "Number of velocity layers to render per note");
        velLayersBox.onChange = [this] { updateSizeEstimate(); };
        addAndMakeVisible(velLayersBox);

        // Bit depth
        bitDepthBox.addItem("16-bit", 1);
        bitDepthBox.addItem("24-bit", 2);
        bitDepthBox.setSelectedId(2);
        bitDepthBox.setTooltip("WAV bit depth");
        A11y::setup(bitDepthBox, "Bit Depth", "Audio sample bit depth");
        bitDepthBox.onChange = [this] { updateSizeEstimate(); };
        addAndMakeVisible(bitDepthBox);

        // Sample rate
        sampleRateBox.addItem("44.1 kHz", 1);
        sampleRateBox.addItem("48 kHz", 2);
        sampleRateBox.setSelectedId(2);
        sampleRateBox.setTooltip("WAV sample rate");
        A11y::setup(sampleRateBox, "Sample Rate", "Audio sample rate in kHz");
        sampleRateBox.onChange = [this] { updateSizeEstimate(); };
        addAndMakeVisible(sampleRateBox);
    }

    void buildBundleConfig()
    {
        bundleNameField.setTextToShowWhenEmpty("Bundle Name...", GalleryColors::get(GalleryColors::textMid()));
        A11y::setup(bundleNameField, "Bundle Name", "Name for the exported expansion pack");
        addAndMakeVisible(bundleNameField);

        // Cover engine picker
        coverEngineBox.addItem("Auto (first engine)", 1);
        static const char* engines[] = {
            "OddfeliX", "OddOscar", "Overdub", "Odyssey", "Oblong",
            "Obese", "Onset", "Overworld", "Opal", "Orbital",
            "Organon", "Ouroboros", "Obsidian", "Overbite", "Origami",
            "Oracle", "Obscura", "Oceanic", "Optic", "Oblique"
        };
        for (int i = 0; i < 20; ++i)
            coverEngineBox.addItem(engines[i], i + 2);
        coverEngineBox.setSelectedId(1);
        A11y::setup(coverEngineBox, "Cover Engine", "Engine style for procedural cover art");
        addAndMakeVisible(coverEngineBox);

        // Sound Shape toggle
        soundShapeToggle.setTooltip("Auto-adjust render settings per preset character");
        A11y::setup(soundShapeToggle, "Sound Shape", "Enable automatic render optimization per preset type");
        addAndMakeVisible(soundShapeToggle);
    }

    void buildSizeEstimate()
    {
        updateSizeEstimate();
    }

    void buildProgressSection()
    {
        progressBar.setColour(juce::ProgressBar::foregroundColourId,
                              GalleryColors::get(GalleryColors::xoGold));
        addAndMakeVisible(progressBar);

        progressLabel.setFont(GalleryFonts::label(8.0f));
        progressLabel.setColour(juce::Label::textColourId,
                                GalleryColors::get(GalleryColors::textMid()));
        A11y::setup(progressLabel, "Export Progress", "Current export progress status");
        addAndMakeVisible(progressLabel);
    }

    void buildActionButtons()
    {
        auto styleBtn = [this](juce::TextButton& btn, const juce::String& a11yTitle,
                               const juce::String& a11yDesc, bool isPrimary)
        {
            btn.setColour(juce::TextButton::buttonColourId,
                          isPrimary ? GalleryColors::get(GalleryColors::xoGold)
                                    : GalleryColors::get(GalleryColors::borderGray()));
            btn.setColour(juce::TextButton::textColourOffId,
                          isPrimary ? juce::Colour(GalleryColors::Light::textDark)
                                    : GalleryColors::get(GalleryColors::textDark()));
            A11y::setup(btn, a11yTitle, a11yDesc);
            addAndMakeVisible(btn);
        };

        styleBtn(exportBtn, "Export Button", "Start exporting the selected presets", true);
        styleBtn(cancelBtn, "Cancel Button", "Cancel the current export or close the dialog", false);
        styleBtn(validateBtn, "Validate Button", "Check presets for issues before export", false);

        exportBtn.onClick = [this] { startExport(); };
        cancelBtn.onClick = [this]
        {
            if (exporting)
                cancelExport();
            else if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
                dw->closeButtonPressed();
        };
        validateBtn.onClick = [this] { runValidation(); };
    }

    //==========================================================================
    // Size estimation
    //==========================================================================

    XPNExporter::RenderSettings getCurrentSettings() const
    {
        XPNExporter::RenderSettings s;

        switch (strategyBox.getSelectedId())
        {
            case 1:  s.noteStrategy = XPNExporter::RenderSettings::NoteStrategy::EveryMinor3rd; break;
            case 2:  s.noteStrategy = XPNExporter::RenderSettings::NoteStrategy::Chromatic; break;
            case 3:  s.noteStrategy = XPNExporter::RenderSettings::NoteStrategy::EveryFifth; break;
            case 4:  s.noteStrategy = XPNExporter::RenderSettings::NoteStrategy::OctavesOnly; break;
            default: break;
        }

        s.velocityLayers = velLayersBox.getSelectedId();
        s.bitDepth = (bitDepthBox.getSelectedId() == 1) ? 16 : 24;
        s.sampleRate = (sampleRateBox.getSelectedId() == 1) ? 44100.0 : 48000.0;
        s.useSoundShapes = soundShapeToggle.getToggleState();

        return s;
    }

    void updateSizeEstimate()
    {
        auto settings = getCurrentSettings();
        int presetCount = juce::jmax(1, (int)presetManager.library.size());
        auto est = XPNExporter::estimateExportSize(settings, presetCount);

        juce::String sizeStr;
        if (est.totalBytes > 1024 * 1024 * 1024)
            sizeStr = juce::String(est.totalBytes / (1024.0 * 1024.0 * 1024.0), 1) + " GB";
        else if (est.totalBytes > 1024 * 1024)
            sizeStr = juce::String(est.totalBytes / (1024.0 * 1024.0), 1) + " MB";
        else
            sizeStr = juce::String(est.totalBytes / 1024.0, 1) + " KB";

        sizeEstimateText = "Estimated: " + sizeStr + " | "
            + juce::String(est.totalWavFiles) + " WAV files | "
            + juce::String(est.notesPerPreset) + " notes/preset";

        repaint();
    }

    //==========================================================================
    // Validation
    //==========================================================================

    void runValidation()
    {
        auto result = XPNExporter::validateBatch(presetManager.library);

        juce::String msg;
        if (result.valid)
        {
            msg = "All presets valid.";
            if (result.warnings.size() > 0)
                msg += "\n\nWarnings:\n" + result.warnings.joinIntoString("\n");
        }
        else
        {
            msg = "Errors found:\n" + result.errors.joinIntoString("\n");
            if (result.warnings.size() > 0)
                msg += "\n\nWarnings:\n" + result.warnings.joinIntoString("\n");
        }

        juce::AlertWindow::showMessageBoxAsync(
            result.valid ? juce::MessageBoxIconType::InfoIcon
                         : juce::MessageBoxIconType::WarningIcon,
            "Pre-Export Validation", msg);

        // Announce to screen readers
        juce::AccessibilityHandler::postAnnouncement(
            result.valid ? "Validation passed" : "Validation found errors",
            juce::AccessibilityHandler::AnnouncementPriority::medium);
    }

    //==========================================================================
    // Export execution
    //==========================================================================

    void startExport()
    {
        if (exporting) return;

        auto bundleName = bundleNameField.getText().trim();
        if (bundleName.isEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "Missing Name", "Please enter a bundle name.");
            bundleNameField.grabKeyboardFocus();
            return;
        }

        exporting = true;
        shouldCancel.store(false);
        exportBtn.setEnabled(false);
        validateBtn.setEnabled(false);
        cancelBtn.setButtonText("CANCEL");
        progressValue = 0.0;

        juce::AccessibilityHandler::postAnnouncement(
            "Export started", juce::AccessibilityHandler::AnnouncementPriority::high);

        startTimerHz(15); // UI refresh for progress

        // Export runs on a worker thread
        struct ExportThread : public juce::Thread
        {
            ExportThread(ExportDialog& d) : juce::Thread("XPN-Export"), dialog(d) {}

            void run() override
            {
                XPNExporter exporter;

                XPNExporter::BundleConfig config;
                config.name = dialog.bundleNameField.getText().trim();
                config.bundleId = "com.xo-ox.xomnibus." + config.name.toLowerCase().replace(" ", "-");
                config.outputDir = juce::File::getSpecialLocation(
                    juce::File::userDesktopDirectory).getChildFile("XPN_Exports");
                config.outputDir.createDirectory();

                if (dialog.coverEngineBox.getSelectedId() > 1)
                    config.coverEngine = dialog.coverEngineBox.getText();

                auto settings = dialog.getCurrentSettings();
                auto& presets = dialog.presetManager.library;

                auto result = exporter.exportBundle(config, settings, presets,
                    [this, &presets](XPNExporter::Progress& p)
                    {
                        dialog.progressValue = (double)p.overallProgress;
                        dialog.lastProgressText = p.presetName + " — note "
                            + juce::String(p.currentNote) + "/" + juce::String(p.totalNotes)
                            + " (" + juce::String(p.currentPreset) + "/"
                            + juce::String(p.totalPresets) + ")";

                        if (dialog.shouldCancel.load())
                            p.cancelled = true;
                    });

                dialog.exportResult = result;
                dialog.exportFinished.store(true);
            }

            ExportDialog& dialog;
        };

        exportThread = std::make_unique<ExportThread>(*this);
        exportThread->startThread();
    }

    void cancelExport()
    {
        shouldCancel.store(true);
        juce::AccessibilityHandler::postAnnouncement(
            "Cancelling export", juce::AccessibilityHandler::AnnouncementPriority::medium);
    }

    // Timer callback for UI updates during export
    void timerCallback() override
    {
        progressLabel.setText(lastProgressText, juce::dontSendNotification);

        if (exportFinished.load())
        {
            stopTimer();
            exporting = false;
            exportBtn.setEnabled(true);
            validateBtn.setEnabled(true);
            cancelBtn.setButtonText("CLOSE");
            progressValue = exportResult.success ? 1.0 : progressValue;

            if (exportThread)
            {
                exportThread->waitForThreadToExit(2000);
                exportThread.reset();
            }

            juce::String summary;
            if (exportResult.success)
            {
                juce::String sizeStr;
                if (exportResult.totalSizeBytes > 1024 * 1024)
                    sizeStr = juce::String(exportResult.totalSizeBytes / (1024.0 * 1024.0), 1) + " MB";
                else
                    sizeStr = juce::String(exportResult.totalSizeBytes / 1024.0, 1) + " KB";

                summary = "Export complete!\n\n"
                    + juce::String(exportResult.presetsExported) + " presets, "
                    + juce::String(exportResult.samplesRendered) + " samples\n"
                    + "Total size: " + sizeStr + "\n"
                    + "Location: " + exportResult.outputFile.getFullPathName();
            }
            else
            {
                summary = "Export failed: " + exportResult.errorMessage;
            }

            progressLabel.setText(exportResult.success ? "Complete!" : "Failed",
                                  juce::dontSendNotification);

            juce::AccessibilityHandler::postAnnouncement(
                exportResult.success ? "Export complete" : "Export failed",
                juce::AccessibilityHandler::AnnouncementPriority::high);

            juce::AlertWindow::showMessageBoxAsync(
                exportResult.success ? juce::MessageBoxIconType::InfoIcon
                                     : juce::MessageBoxIconType::WarningIcon,
                "XPN Export", summary);

            exportFinished.store(false);
        }
    }

    // Shared state between export thread and UI
    juce::String lastProgressText;
    std::atomic<bool> exportFinished { false };
    XPNExporter::ExportResult exportResult;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportDialog)
};

} // namespace xomnibus
