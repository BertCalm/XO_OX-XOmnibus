#pragma once
#include <mutex>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "../../Export/XPNExporter.h"
#include "../../Export/XDrip.h"
#include "../../Core/PresetManager.h"
#include "../../Core/EngineRegistry.h"
#include "../../Core/MegaCouplingMatrix.h"
#include "../GalleryColors.h"   // GalleryColors, prefixForEngine (no editor circular dep)

namespace xolokun {

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
    ExportDialog(PresetManager& pm,
                 juce::AudioProcessorValueTreeState* apvts = nullptr,
                 MegaCouplingMatrix* couplingMatrix = nullptr)
        : presetManager(pm), dialogApvts(apvts), liveCouplingMatrix(couplingMatrix)
    {
        setWantsKeyboardFocus(true);
        A11y::setup(*this, "XPN Export", "Export presets as MPC-compatible expansion pack");

        buildPresetSelection();
        buildPreviewSection();
        buildRenderSettings();
        buildEntangledMode();
        buildBundleConfig();
        buildSizeEstimate();
        buildProgressSection();
        buildActionButtons();

        setSize(520, 780);
    }

    ~ExportDialog() override
    {
        stopTimer();
        previewDrip.cancel();
        stopPreviewPlayback();
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
        g.drawText("XORIGINATE", headerArea.reduced(12, 0), juce::Justification::centredLeft);

        // Section dividers
        g.setColour(GalleryColors::get(GalleryColors::borderGray()));

        auto b = getLocalBounds().reduced(0, 0);
        b.removeFromTop(kHeaderH);

        int sectionY = kHeaderH + kPresetSectionH;
        g.drawLine(12, (float)sectionY, (float)getWidth() - 12, (float)sectionY, 1.0f);

        sectionY += kPreviewSectionH;
        g.drawLine(12, (float)sectionY, (float)getWidth() - 12, (float)sectionY, 1.0f);

        sectionY += kSettingsSectionH;
        g.drawLine(12, (float)sectionY, (float)getWidth() - 12, (float)sectionY, 1.0f);

        sectionY += kEntangledSectionH;
        g.drawLine(12, (float)sectionY, (float)getWidth() - 12, (float)sectionY, 1.0f);

        sectionY += kBundleSectionH;
        g.drawLine(12, (float)sectionY, (float)getWidth() - 12, (float)sectionY, 1.0f);

        // Section labels
        g.setFont(GalleryFonts::heading(9.0f));
        g.setColour(GalleryColors::get(GalleryColors::textMid()));
        g.drawText("PRESETS", 12, kHeaderH + 4, 200, 16, juce::Justification::topLeft);

        int previewLabelY = kHeaderH + kPresetSectionH + 4;
        g.drawText("PREVIEW", 12, previewLabelY, 200, 16, juce::Justification::topLeft);

        int settingsLabelY = kHeaderH + kPresetSectionH + kPreviewSectionH + 4;
        g.drawText("RENDER SETTINGS", 12, settingsLabelY, 200, 16, juce::Justification::topLeft);

        int entangledLabelY = settingsLabelY + kSettingsSectionH;
        // "ENTANGLED" label painted in XO Gold when toggle is active
        if (entangledToggle.getToggleState() && capturedSnapshot.hasActiveCoupling())
            g.setColour(GalleryColors::get(GalleryColors::xoGold));
        g.drawText("ENTANGLED", 12, entangledLabelY, 200, 16, juce::Justification::topLeft);
        g.setColour(GalleryColors::get(GalleryColors::textMid()));

        g.drawText("BUNDLE", 12, entangledLabelY + kEntangledSectionH, 200, 16, juce::Justification::topLeft);

        // Draw preview waveform
        paintPreviewWaveform(g);

        // Size estimate label
        if (!exporting)
        {
            auto estArea = getLocalBounds().reduced(12, 0);
            estArea.removeFromTop(kHeaderH + kPresetSectionH + kPreviewSectionH + kSettingsSectionH + kEntangledSectionH + kBundleSectionH + 4);
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

        // Preview section — play button + waveform display
        auto previewArea = area.removeFromTop(kPreviewSectionH).reduced(12, 0);
        previewArea.removeFromTop(18); // section label space
        auto previewRow = previewArea.removeFromTop(32);
        previewPlayBtn.setBounds(previewRow.removeFromLeft(32));
        previewRow.removeFromLeft(6);
        // Waveform display area is painted directly (no child component)
        previewWaveformBounds = previewRow.reduced(0, 2);

        // Render settings section — row 1: combo boxes, row 2: quick-profile buttons
        auto settingsArea = area.removeFromTop(kSettingsSectionH).reduced(12, 0);
        settingsArea.removeFromTop(20); // section label space
        auto settingsRow1 = settingsArea.removeFromTop(28);
        int settingsW = settingsRow1.getWidth() / 4;
        strategyBox.setBounds(settingsRow1.removeFromLeft(settingsW).reduced(2, 0));
        velLayersBox.setBounds(settingsRow1.removeFromLeft(settingsW).reduced(2, 0));
        bitDepthBox.setBounds(settingsRow1.removeFromLeft(settingsW).reduced(2, 0));
        sampleRateBox.setBounds(settingsRow1.removeFromLeft(settingsW).reduced(2, 0));
        settingsArea.removeFromTop(4);
        auto profileRow = settingsArea.removeFromTop(22);
        int profileBtnW = 88;
        profileMPCBtn.setBounds(profileRow.removeFromLeft(profileBtnW).reduced(2, 0));
        profileLightBtn.setBounds(profileRow.removeFromLeft(profileBtnW).reduced(2, 0));
        profileMaxBtn.setBounds(profileRow.removeFromLeft(profileBtnW).reduced(2, 0));

        // Entangled mode section
        auto entangledArea = area.removeFromTop(kEntangledSectionH).reduced(12, 0);
        entangledArea.removeFromTop(4);
        auto entangledRow = entangledArea.removeFromTop(20);
        entangledToggle.setBounds(entangledRow.removeFromLeft(140));
        entangledSummaryLabel.setBounds(entangledRow.reduced(4, 0));

        // Bundle config section
        auto bundleArea = area.removeFromTop(kBundleSectionH).reduced(12, 0);
        bundleArea.removeFromTop(20); // section label space
        auto nameRow = bundleArea.removeFromTop(28);
        bundleNameField.setBounds(nameRow);
        bundleArea.removeFromTop(6);
        auto engineRow = bundleArea.removeFromTop(28);
        coverEngineBox.setBounds(engineRow.removeFromLeft(engineRow.getWidth() / 2).reduced(0, 0));
        soundShapeToggle.setBounds(engineRow.reduced(4, 0));
        bundleArea.removeFromTop(6);
        auto outputRow = bundleArea.removeFromTop(24);
        outputDirBtn.setBounds(outputRow.removeFromRight(80).reduced(2, 0));
        outputDirLabel.setBounds(outputRow.reduced(2, 0));

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

    static constexpr int kHeaderH            = 36;
    static constexpr int kPresetSectionH     = 140;
    static constexpr int kPreviewSectionH    = 56;
    static constexpr int kSettingsSectionH   = 100;
    static constexpr int kEntangledSectionH  = 40;
    static constexpr int kBundleSectionH     = 122;
    static constexpr int kProgressH          = 44;

    PresetManager& presetManager;
    juce::AudioProcessorValueTreeState* dialogApvts = nullptr;
    bool exporting = false;
    std::unique_ptr<juce::Thread> exportThread;

    // Preset selection
    juce::ListBox presetList { "PresetList" };

    // Render settings
    juce::ComboBox strategyBox, velLayersBox, bitDepthBox, sampleRateBox;

    // Quick-profile buttons (surfaces ExportProfile enum to the user)
    juce::TextButton profileMPCBtn     { "MPC Standard" };
    juce::TextButton profileLightBtn   { "Lightweight" };
    juce::TextButton profileMaxBtn     { "Max Quality" };

    // Bundle config
    juce::TextEditor bundleNameField;
    juce::ComboBox coverEngineBox;
    juce::ToggleButton soundShapeToggle { "Sound Shape Auto" };

    // Output directory
    juce::File outputDir { juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                                       .getChildFile("XPN_Exports") };
    juce::Label outputDirLabel;
    juce::TextButton outputDirBtn { "CHOOSE..." };

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
    std::unique_ptr<juce::FileChooser> fileChooser;

    // XDrip preview
    XDrip previewDrip;
    juce::TextButton previewPlayBtn { ">" };
    juce::Rectangle<int> previewWaveformBounds;
    std::vector<float> cachedThumbnail;
    bool previewPlaying = false;
    int  previewPlaybackPos = 0;
    juce::AudioBuffer<float> previewAudioBuffer;
    std::unique_ptr<juce::AudioDeviceManager> previewDeviceManager;
    std::unique_ptr<juce::AudioSourcePlayer> previewPlayer;
    std::unique_ptr<PreviewAudioSource> previewSource;
    int selectedPresetIndex = -1;

    // Entangled mode — coupling snapshot export
    MegaCouplingMatrix* liveCouplingMatrix = nullptr;
    juce::ToggleButton entangledToggle { "Entangled Mode" };
    juce::Label entangledSummaryLabel;
    XOriginate::CouplingSnapshot capturedSnapshot;

    //==========================================================================
    // Build methods
    //==========================================================================

    void buildPresetSelection()
    {
        addAndMakeVisible(presetList);
        A11y::setup(presetList, "Preset List", "Select presets to include in export");
    }

    //==========================================================================
    // Preview section (XDrip)
    //==========================================================================

    void buildPreviewSection()
    {
        previewPlayBtn.setColour(juce::TextButton::buttonColourId,
                                  GalleryColors::get(GalleryColors::xoGold));
        previewPlayBtn.setColour(juce::TextButton::textColourOffId,
                                  juce::Colour(GalleryColors::Light::textDark));
        A11y::setup(previewPlayBtn, "Preview Play",
                    "Play a 2-second audio preview of the selected preset");
        addAndMakeVisible(previewPlayBtn);

        previewPlayBtn.onClick = [this]
        {
            if (previewPlaying)
            {
                stopPreviewPlayback();
            }
            else if (previewDrip.getState() == XDrip::State::Ready)
            {
                startPreviewPlayback();
            }
            else
            {
                triggerPreviewForSelectedPreset();
            }
        };
    }

    void triggerPreviewForSelectedPreset()
    {
        if (presetManager.library.empty()) return;

        // Use the selected preset, or the first one
        int idx = (selectedPresetIndex >= 0 &&
                   selectedPresetIndex < (int)presetManager.library.size())
                      ? selectedPresetIndex : 0;

        const auto& preset = presetManager.library[static_cast<size_t>(idx)];
        previewDrip.requestPreview(preset, dialogApvts);
        cachedThumbnail.clear();
        previewPlayBtn.setButtonText("...");
        repaint();

        // Start polling for render completion
        if (!isTimerRunning())
            startTimerHz(15);
    }

    void startPreviewPlayback()
    {
        previewAudioBuffer = previewDrip.getPreviewBuffer();
        if (previewAudioBuffer.getNumSamples() == 0) return;

        previewPlaybackPos = 0;
        previewPlaying = true;
        previewPlayBtn.setButtonText("||");

        // Use a simple timer-based playback via AudioDeviceManager
        if (!previewDeviceManager)
        {
            previewDeviceManager = std::make_unique<juce::AudioDeviceManager>();
            previewDeviceManager->initialiseWithDefaultDevices(0, 2);
            previewPlayer = std::make_unique<juce::AudioSourcePlayer>();
            previewDeviceManager->addAudioCallback(previewPlayer.get());
        }

        // Create a simple playback source
        previewSource = std::make_unique<PreviewAudioSource>(*this);
        previewPlayer->setSource(previewSource.get());

        if (!isTimerRunning())
            startTimerHz(15);

        juce::AccessibilityHandler::postAnnouncement(
            "Playing preview", juce::AccessibilityHandler::AnnouncementPriority::medium);
    }

    void stopPreviewPlayback()
    {
        previewPlaying = false;
        previewPlaybackPos = 0;

        if (previewPlayer)
            previewPlayer->setSource(nullptr);
        previewSource.reset();

        if (previewDrip.getState() == XDrip::State::Ready)
            previewPlayBtn.setButtonText(">");
        else
            previewPlayBtn.setButtonText(">");

        repaint();
    }

    //==========================================================================
    // Simple AudioSource for preview playback
    //==========================================================================

    struct PreviewAudioSource : public juce::AudioSource
    {
        PreviewAudioSource(ExportDialog& d) : dialog(d) {}

        void prepareToPlay(int, double) override {}
        void releaseResources() override {}

        void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override
        {
            info.clearActiveBufferRegion();
            if (!dialog.previewPlaying) return;

            auto& src = dialog.previewAudioBuffer;
            int srcChannels = src.getNumChannels();
            int srcSamples  = src.getNumSamples();
            int pos = dialog.previewPlaybackPos;

            if (pos >= srcSamples)
            {
                // Playback finished — schedule stop on message thread
                juce::MessageManager::callAsync([&dialog = dialog]
                {
                    dialog.stopPreviewPlayback();
                });
                return;
            }

            int samplesToRead = juce::jmin(info.numSamples, srcSamples - pos);

            for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch)
            {
                int srcCh = juce::jmin(ch, srcChannels - 1);
                info.buffer->copyFrom(ch, info.startSample, src, srcCh, pos, samplesToRead);
            }

            dialog.previewPlaybackPos += samplesToRead;
        }

        ExportDialog& dialog;
    };

    //==========================================================================
    // Preview waveform painting
    //==========================================================================

    void paintPreviewWaveform(juce::Graphics& g)
    {
        if (previewWaveformBounds.isEmpty()) return;

        auto area = previewWaveformBounds;

        // Background
        g.setColour(GalleryColors::get(GalleryColors::shellWhite()).darker(0.03f));
        g.fillRoundedRectangle(area.toFloat(), 3.0f);

        // Border
        g.setColour(GalleryColors::get(GalleryColors::borderGray()));
        g.drawRoundedRectangle(area.toFloat(), 3.0f, 0.5f);

        auto dripState = previewDrip.getState();

        if (dripState == XDrip::State::Rendering)
        {
            // Pulsing dots animation
            g.setColour(GalleryColors::get(GalleryColors::xoGold));
            g.setFont(GalleryFonts::label(9.0f));
            g.drawText("Rendering preview...", area, juce::Justification::centred);
        }
        else if (dripState == XDrip::State::Ready && !cachedThumbnail.empty())
        {
            // Draw waveform in XO Gold
            g.setColour(GalleryColors::get(GalleryColors::xoGold));

            float w = static_cast<float>(area.getWidth());
            float h = static_cast<float>(area.getHeight());
            float x0 = static_cast<float>(area.getX());
            float yMid = static_cast<float>(area.getCentreY());
            float halfH = h * 0.45f;

            int points = static_cast<int>(cachedThumbnail.size());
            float barWidth = w / static_cast<float>(points);

            for (int i = 0; i < points; ++i)
            {
                float val = cachedThumbnail[static_cast<size_t>(i)];
                float barH = val * halfH;
                float xPos = x0 + i * barWidth;

                // Symmetric waveform (mirrored top/bottom)
                g.fillRect(xPos, yMid - barH, juce::jmax(1.0f, barWidth - 0.5f), barH * 2.0f);
            }

            // Draw playback position indicator
            if (previewPlaying && previewAudioBuffer.getNumSamples() > 0)
            {
                float progress = static_cast<float>(previewPlaybackPos)
                                 / static_cast<float>(previewAudioBuffer.getNumSamples());
                float lineX = x0 + progress * w;
                g.setColour(juce::Colour(GalleryColors::Light::textDark));
                g.drawLine(lineX, static_cast<float>(area.getY()),
                           lineX, static_cast<float>(area.getBottom()), 1.5f);
            }
        }
        else if (dripState == XDrip::State::Error)
        {
            g.setColour(GalleryColors::get(GalleryColors::textMid()));
            g.setFont(GalleryFonts::label(8.5f));
            g.drawText("Preview failed", area, juce::Justification::centred);
        }
        else
        {
            // Idle — show hint
            g.setColour(GalleryColors::get(GalleryColors::textMid()).withAlpha(0.5f));
            g.setFont(GalleryFonts::label(8.5f));
            g.drawText("Click > to preview selected preset", area, juce::Justification::centred);
        }
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

        // Quick-profile buttons — one-click preset bundles for common use cases
        auto styleProfile = [this](juce::TextButton& btn, const juce::String& a11yTitle,
                                   const juce::String& a11yDesc)
        {
            btn.setColour(juce::TextButton::buttonColourId,
                          GalleryColors::get(GalleryColors::borderGray()));
            btn.setColour(juce::TextButton::textColourOffId,
                          GalleryColors::get(GalleryColors::textDark()));
            A11y::setup(btn, a11yTitle, a11yDesc);
            addAndMakeVisible(btn);
        };
        styleProfile(profileMPCBtn,   "MPC Standard Profile",
                     "Standard MPC export: minor 3rds, 3 velocity layers, 24-bit, 48kHz");
        styleProfile(profileLightBtn, "Lightweight Profile",
                     "Compact export: octaves only, 1 velocity layer, 16-bit, 44.1kHz");
        styleProfile(profileMaxBtn,   "Max Quality Profile",
                     "Chromatic export: chromatic, 3 velocity layers, 24-bit, 48kHz");

        profileMPCBtn.onClick   = [this] { applyProfile(ExportProfile::MPCStandard);  };
        profileLightBtn.onClick = [this] { applyProfile(ExportProfile::Lightweight);  };
        profileMaxBtn.onClick   = [this] { applyProfile(ExportProfile::MaxQuality);   };
    }

    void buildEntangledMode()
    {
        entangledToggle.setTooltip("Export with coupling state preserved as composite instrument");
        A11y::setup(entangledToggle, "Entangled Mode",
                    "When enabled, renders all engine slots as a single coupled instrument");
        addAndMakeVisible(entangledToggle);

        // Disable the toggle if no coupling matrix is available
        entangledToggle.setEnabled(liveCouplingMatrix != nullptr);

        entangledToggle.onClick = [this]
        {
            if (entangledToggle.getToggleState() && liveCouplingMatrix != nullptr)
            {
                // Capture the live coupling state when the user enables entangled mode
                capturedSnapshot = XOriginate::captureCouplingState(
                    EngineRegistry::instance(), *liveCouplingMatrix);

                if (capturedSnapshot.hasActiveCoupling())
                {
                    entangledSummaryLabel.setText(capturedSnapshot.getSummary(),
                                                  juce::dontSendNotification);
                    juce::AccessibilityHandler::postAnnouncement(
                        "Entangled mode enabled: " + juce::String(capturedSnapshot.activeRoutes.size())
                        + " coupling routes captured",
                        juce::AccessibilityHandler::AnnouncementPriority::medium);
                }
                else
                {
                    entangledSummaryLabel.setText("No active coupling routes",
                                                  juce::dontSendNotification);
                    juce::AccessibilityHandler::postAnnouncement(
                        "No active coupling routes found",
                        juce::AccessibilityHandler::AnnouncementPriority::medium);
                }
            }
            else
            {
                capturedSnapshot = {};
                entangledSummaryLabel.setText("", juce::dontSendNotification);
            }
            repaint();
        };

        entangledSummaryLabel.setFont(GalleryFonts::label(7.5f));
        entangledSummaryLabel.setColour(juce::Label::textColourId,
                                         GalleryColors::get(GalleryColors::textMid()));
        entangledSummaryLabel.setMinimumHorizontalScale(0.4f);
        A11y::setup(entangledSummaryLabel, "Coupling Summary",
                    "Shows active coupling routes that will be captured in the export");
        addAndMakeVisible(entangledSummaryLabel);
    }

    void buildBundleConfig()
    {
        bundleNameField.setTextToShowWhenEmpty("Bundle Name...", GalleryColors::get(GalleryColors::textMid()));
        A11y::setup(bundleNameField, "Bundle Name", "Name for the exported expansion pack");
        addAndMakeVisible(bundleNameField);

        // Cover engine picker — populated from live registry (all registered engines)
        coverEngineBox.addItem("Auto (first engine)", 1);
        auto registeredIds = EngineRegistry::instance().getRegisteredIds();
        std::sort(registeredIds.begin(), registeredIds.end());
        for (int i = 0; i < (int)registeredIds.size(); ++i)
            coverEngineBox.addItem(juce::String(registeredIds[(size_t)i]), i + 2);
        coverEngineBox.setSelectedId(1);
        A11y::setup(coverEngineBox, "Cover Engine", "Engine style for procedural cover art");
        addAndMakeVisible(coverEngineBox);

        // Sound Shape toggle
        soundShapeToggle.setTooltip("Auto-adjust render settings per preset character");
        A11y::setup(soundShapeToggle, "Sound Shape", "Enable automatic render optimization per preset type");
        addAndMakeVisible(soundShapeToggle);

        // Output directory
        auto updateOutputLabel = [this]
        {
            outputDirLabel.setText(outputDir.getFullPathName(), juce::dontSendNotification);
        };
        outputDirLabel.setFont(GalleryFonts::label(8.0f));
        outputDirLabel.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()));
        outputDirLabel.setMinimumHorizontalScale(0.5f);
        A11y::setup(outputDirLabel, "Output Directory", "Folder where exported XPN files will be saved");
        addAndMakeVisible(outputDirLabel);
        updateOutputLabel();

        outputDirBtn.setColour(juce::TextButton::buttonColourId,
                               GalleryColors::get(GalleryColors::borderGray()));
        outputDirBtn.setColour(juce::TextButton::textColourOffId,
                               GalleryColors::get(GalleryColors::textDark()));
        A11y::setup(outputDirBtn, "Choose Output Folder", "Browse for the export destination folder");
        addAndMakeVisible(outputDirBtn);

        outputDirBtn.onClick = [this, updateOutputLabel]
        {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Choose XPN Export Folder", outputDir, "", false);
            fileChooser->launchAsync(
                juce::FileBrowserComponent::openMode |
                juce::FileBrowserComponent::canSelectDirectories,
                [this, updateOutputLabel](const juce::FileChooser& fc)
                {
                    auto result = fc.getResult();
                    if (result.isDirectory())
                    {
                        outputDir = result;
                        updateOutputLabel();
                        juce::AccessibilityHandler::postAnnouncement(
                            "Output folder set to " + outputDir.getFullPathName(),
                            juce::AccessibilityHandler::AnnouncementPriority::medium);
                    }
                });
        };
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

    XOriginate::RenderSettings getCurrentSettings() const
    {
        XOriginate::RenderSettings s;

        switch (strategyBox.getSelectedId())
        {
            case 1:  s.noteStrategy = XOriginate::RenderSettings::NoteStrategy::EveryMinor3rd; break;
            case 2:  s.noteStrategy = XOriginate::RenderSettings::NoteStrategy::Chromatic; break;
            case 3:  s.noteStrategy = XOriginate::RenderSettings::NoteStrategy::EveryFifth; break;
            case 4:  s.noteStrategy = XOriginate::RenderSettings::NoteStrategy::OctavesOnly; break;
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
        auto est = XOriginate::estimateExportSize(settings, presetCount);

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
        auto result = XOriginate::validateBatch(presetManager.library);

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

        // Capture UI state on the message thread before launching worker
        auto capturedSettings = getCurrentSettings();
        juce::String capturedName = bundleNameField.getText().trim();
        juce::String capturedCoverEngine;
        if (coverEngineBox.getSelectedId() > 1)
            capturedCoverEngine = coverEngineBox.getText();

        // Capture entangled state on the message thread before launching worker
        bool useEntangled = entangledToggle.getToggleState()
                            && capturedSnapshot.hasActiveCoupling();
        auto snapshotCopy = useEntangled ? capturedSnapshot : XOriginate::CouplingSnapshot{};

        // Export runs on a worker thread
        struct ExportThread : public juce::Thread
        {
            ExportThread(ExportDialog& d,
                         XOriginate::RenderSettings s,
                         juce::String name,
                         juce::String coverEng,
                         bool entangled,
                         XOriginate::CouplingSnapshot snap)
                : juce::Thread("XPN-Export"), dialog(d),
                  settings(std::move(s)), bundleName(std::move(name)),
                  coverEngine(std::move(coverEng)),
                  useEntangled(entangled), snapshot(std::move(snap)) {}

            void run() override
            {
                XOriginate exporter;

                // Wire the shared APVTS so renderNoteToWav() produces real audio.
                // If dialogApvts is null the render falls back to silent placeholders.
                exporter.setAPVTS(dialog.dialogApvts);

                XOriginate::BundleConfig config;
                config.name = bundleName;
                config.bundleId = "com.xo-ox.xolokun." + config.name.toLowerCase().replace(" ", "-");
                config.outputDir = dialog.outputDir;
                config.outputDir.createDirectory();

                if (coverEngine.isNotEmpty())
                    config.coverEngine = coverEngine;

                auto& presets = dialog.presetManager.library;

                auto progressCb = [this](XOriginate::Progress& p)
                {
                    dialog.progressValue = (double)p.overallProgress;

                    auto text = p.presetName
                        + (useEntangled ? juce::String(" [Entangled]") : juce::String())
                        + " — note "
                        + juce::String(p.currentNote) + "/" + juce::String(p.totalNotes)
                        + " (" + juce::String(p.currentPreset) + "/"
                        + juce::String(p.totalPresets) + ")";

                    {
                        std::lock_guard<std::mutex> lock(dialog.progressTextMutex);
                        dialog.lastProgressText = text;
                    }

                    if (dialog.shouldCancel.load())
                        p.cancelled = true;
                };

                XOriginate::ExportResult result;
                if (useEntangled)
                    result = exporter.exportCoupledSnapshot(config, settings, snapshot,
                                                            presets, progressCb);
                else
                    result = exporter.exportBundle(config, settings, presets, progressCb);

                dialog.exportResult = result;
                dialog.exportFinished.store(true);
            }

            ExportDialog& dialog;
            XOriginate::RenderSettings settings;
            juce::String bundleName;
            juce::String coverEngine;
            bool useEntangled;
            XOriginate::CouplingSnapshot snapshot;
        };

        exportThread = std::make_unique<ExportThread>(*this, capturedSettings,
                                                         capturedName, capturedCoverEngine,
                                                         useEntangled, snapshotCopy);
        exportThread->startThread();
    }

    void cancelExport()
    {
        shouldCancel.store(true);
        juce::AccessibilityHandler::postAnnouncement(
            "Cancelling export", juce::AccessibilityHandler::AnnouncementPriority::medium);
    }

    // Timer callback for UI updates during export and preview
    void timerCallback() override
    {
        // Poll XDrip preview state
        auto dripState = previewDrip.getState();
        if (dripState == XDrip::State::Ready && cachedThumbnail.empty())
        {
            cachedThumbnail = previewDrip.getThumbnail();
            previewPlayBtn.setButtonText(">");
            repaint();

            juce::AccessibilityHandler::postAnnouncement(
                "Preview ready", juce::AccessibilityHandler::AnnouncementPriority::medium);
        }
        else if (dripState == XDrip::State::Rendering)
        {
            repaint(); // Animate the rendering indicator
        }

        // Repaint during playback for position indicator
        if (previewPlaying)
            repaint();

        // Stop timer if nothing is active
        if (!exporting && !previewPlaying && dripState != XDrip::State::Rendering)
        {
            // Only stop if export is also not pending finish
            if (!exportFinished.load())
                stopTimer();
        }

        // Export progress
        {
            std::lock_guard<std::mutex> lock(progressTextMutex);
            progressLabel.setText(lastProgressText, juce::dontSendNotification);
        }

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
    std::mutex progressTextMutex;
    juce::String lastProgressText;
    std::atomic<bool> exportFinished { false };
    XOriginate::ExportResult exportResult;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportDialog)
};

} // namespace xolokun
