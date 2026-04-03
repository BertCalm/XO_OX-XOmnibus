// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../../Export/XOutshine.h"

namespace xoceanus
{

class OutshineExportBar : public juce::Component
{
public:
    std::function<void(const juce::String&, ExportFormat, const juce::File&)> onExportClicked;
    std::function<void()> onCancelClicked;

    OutshineExportBar()
    {
        setWantsKeyboardFocus(false);
        A11y::setup(*this, "Export Bar", "Pearl name, export format, and export controls");
        buildControls();
        // Initial size is advisory only — parent container calls setBounds().
        // Use proportional defaults so the bar is usable at any DPI. The
        // previous hard-coded 900×60 appeared at half-size on 2x Retina displays.
        setSize(900, 60);
    }

    ~OutshineExportBar() override = default;

    void setProgress(float progress, const juce::String& stageString)
    {
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
        currentProgress = progress;

        juce::String metaphor;
        if (progress < 0.15f)
            metaphor = "Opening the grain...";
        else if (progress < 0.35f)
            metaphor = "Layering nacre...";
        else if (progress < 0.75f)
            metaphor = "Pearl forming...";
        else if (progress < 0.95f)
            metaphor = "Packaging...";
        else
            metaphor = "Pearl complete.";

        progressMetaphorLabel.setText(metaphor, juce::dontSendNotification);
        progressTechLabel.setText(stageString, juce::dontSendNotification);
        repaint();
    }

    void setReadyToExport(bool ready)
    {
        readyToExport = ready;
        exportBtn.setEnabled(ready && unverifiedCount == 0);
        exportBtn.setAlpha(exportBtn.isEnabled() ? 1.0f : 0.4f);
    }

    void setUnverifiedCount(int count)
    {
        unverifiedCount = count;
        bool hasUnverified = (count > 0);
        unverifiedBadge.setVisible(hasUnverified);
        if (hasUnverified)
            unverifiedBadge.setText(juce::String(count) + " unverified root" + (count == 1 ? "" : "s"),
                                    juce::dontSendNotification);
        exportBtn.setEnabled(readyToExport && !hasUnverified);
        exportBtn.setAlpha(exportBtn.isEnabled() ? 1.0f : 0.4f);
    }

    void setExporting(bool exporting)
    {
        isExporting = exporting;
        exportBtn.setVisible(!exporting);
        cancelBtn.setVisible(exporting);
        pearlNameField.setEnabled(!exporting);
        formatSelector.setEnabled(!exporting);
        repaint();
    }

    void setGrainCount(int n)
    {
        grainCount = n;
        if (!isExporting)
        {
            int estSec = (int)(n * 0.5 + n * 1.0 + 2.0);
            progressTechLabel.setText("Analyzing " + juce::String(n) + " samples, ~" + juce::String(estSec) + " sec",
                                      juce::dontSendNotification);
        }
    }

    ExportFormat getSelectedFormat() const
    {
        switch (formatSelector.getSelectedId())
        {
        case 1:
            return ExportFormat::XPNPack;
        case 2:
            return ExportFormat::WAVFolder;
        case 3:
            return ExportFormat::XPMOnly;
        default:
            return ExportFormat::XPNPack;
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

        g.setColour(GalleryColors::get(GalleryColors::borderGray()));
        g.drawLine(0, 0, (float)getWidth(), 0, 1.0f);

        auto barArea = getLocalBounds().removeFromBottom(kBarH);
        g.setColour(GalleryColors::get(GalleryColors::borderGray()).withAlpha(0.5f));
        g.fillRect(barArea);
        if (currentProgress > 0.0f)
        {
            g.setColour(exportFailed ? juce::Colour(0xFFE05252) : GalleryColors::get(GalleryColors::xoGold));
            g.fillRect(barArea.withWidth((int)(barArea.getWidth() * currentProgress)));
        }
    }

    void resized() override
    {
        // Derive button widths from the component's own width so they scale on
        // HiDPI/Retina.  The previous hardcoded 80/140/120 px looked cramped on
        // 2x displays.  Proportions: cancel ≈ 9%, export ≈ 16%, badge ≈ 13%.
        const int w = getWidth();
        const int cancelW = juce::jmax(64, (int)(w * 0.09f));
        const int exportW = juce::jmax(110, (int)(w * 0.16f));
        const int badgeW = juce::jmax(90, (int)(w * 0.13f));

        auto area = getLocalBounds().reduced(8, 0);
        area.removeFromBottom(kBarH);

        auto controlRow = area.removeFromTop(kButtonH + 8).withTrimmedTop(4);

        pearlNameField.setBounds(controlRow.removeFromLeft(kNameFieldW).reduced(0, 2));
        controlRow.removeFromLeft(8);
        formatSelector.setBounds(controlRow.removeFromLeft(kFormatW).reduced(0, 2));
        controlRow.removeFromLeft(8);

        cancelBtn.setBounds(controlRow.removeFromRight(cancelW).reduced(0, 2));
        controlRow.removeFromRight(4);
        exportBtn.setBounds(controlRow.removeFromRight(exportW).reduced(0, 2));
        controlRow.removeFromRight(8);

        unverifiedBadge.setBounds(controlRow.removeFromRight(badgeW).reduced(0, 4));
        controlRow.removeFromRight(8);

        auto labelArea = area.reduced(0, 2);
        progressMetaphorLabel.setBounds(labelArea.removeFromTop(14));
        progressTechLabel.setBounds(labelArea.removeFromTop(13));
    }

private:
    void buildControls()
    {
        pearlNameField.setFont(GalleryFonts::body(12.0f));
        pearlNameField.setTextToShowWhenEmpty("Pearl Name", GalleryColors::get(GalleryColors::textMid()));
        pearlNameField.setInputRestrictions(64, {});
        A11y::setup(pearlNameField, "Pearl Name", "Name for the exported instrument");
        addAndMakeVisible(pearlNameField);

        formatSelector.addItem("XPN Pack", 1);
        formatSelector.addItem("WAV Folder", 2);
        formatSelector.addItem("XPM Only", 3);
        formatSelector.setSelectedId(1, juce::dontSendNotification);
        A11y::setup(formatSelector, "Export Format", "Choose XPN Pack, WAV Folder, or XPM Only");
        addAndMakeVisible(formatSelector);

        exportBtn.setColour(juce::TextButton::buttonColourId, GalleryColors::get(GalleryColors::xoGold));
        exportBtn.setColour(juce::TextButton::textColourOffId,
                            juce::Colour(GalleryColors::get(GalleryColors::textDark())));
        exportBtn.onClick = [this]() { onExportButtonClicked(); };
        exportBtn.setEnabled(false);
        exportBtn.setAlpha(0.4f);
        A11y::setup(exportBtn, "Pearl and Export", "Run the full pipeline and export the instrument");
        addAndMakeVisible(exportBtn);

        cancelBtn.setVisible(false);
        cancelBtn.onClick = [this]()
        {
            if (onCancelClicked)
                onCancelClicked();
        };
        A11y::setup(cancelBtn, "Cancel", "Cancel the export pipeline");
        addAndMakeVisible(cancelBtn);

        progressMetaphorLabel.setFont(GalleryFonts::body(11.0f));
        progressMetaphorLabel.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textDark()));
        addAndMakeVisible(progressMetaphorLabel);

        progressTechLabel.setFont(GalleryFonts::value(10.0f));
        progressTechLabel.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()));
        addAndMakeVisible(progressTechLabel);

        unverifiedBadge.setFont(GalleryFonts::value(9.0f));
        unverifiedBadge.setColour(juce::Label::textColourId, juce::Colour(0xFFE9A84A));
        unverifiedBadge.setVisible(false);
        addAndMakeVisible(unverifiedBadge);
    }

    void onExportButtonClicked()
    {
        if (!onExportClicked)
            return;

        auto chooser = std::make_shared<juce::FileChooser>(
            "Choose export location", juce::File::getSpecialLocation(juce::File::userDesktopDirectory), "");

        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectDirectories,
                             [this, chooser](const juce::FileChooser& fc)
                             {
                                 auto result = fc.getResult();
                                 if (result != juce::File())
                                 {
                                     onExportClicked(pearlNameField.getText(), getSelectedFormat(), result);
                                 }
                             });
    }

    juce::TextEditor pearlNameField;
    juce::ComboBox formatSelector;
    juce::TextButton exportBtn{"Pearl & Export"};
    juce::TextButton cancelBtn{"Cancel"};

    juce::Label progressMetaphorLabel;
    juce::Label progressTechLabel;
    juce::Label unverifiedBadge;

    float currentProgress{0.0f};
    bool isExporting{false};
    bool readyToExport{false};
    bool exportFailed{false};
    int grainCount{0};
    int unverifiedCount{0};

    static constexpr int kBarH = 3;
    static constexpr int kButtonH = 32;
    static constexpr int kNameFieldW = 200;
    static constexpr int kFormatW = 140;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineExportBar)
};

} // namespace xoceanus
