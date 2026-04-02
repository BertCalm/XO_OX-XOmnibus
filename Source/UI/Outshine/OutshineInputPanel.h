// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "OutshineFolderBrowser.h"

namespace xoceanus {

class OutshineInputPanel : public juce::Component,
                           public juce::FileDragAndDropTarget
{
public:
    std::function<void(const juce::StringArray&)> onFilesSelected;

    OutshineInputPanel()
    {
        setWantsKeyboardFocus(true);
        A11y::setup(*this, "Input Panel", "Drop zone and file browser for loading samples");

        folderBrowser = std::make_unique<OutshineFolderBrowser>();
        folderBrowser->onFilesConfirmed = [this](const juce::StringArray& files) {
            if (onFilesSelected)
                onFilesSelected(files);
        };
        addAndMakeVisible(*folderBrowser);

        A11y::setup(selectAllBtn, "Select All WAVs", "Select all WAV files in the current folder");
        selectAllBtn.onClick = [this]() { folderBrowser->selectAllWavFiles(); };
        addAndMakeVisible(selectAllBtn);

        A11y::setup(addSelectedBtn, "Add Selected", "Add selected files to the grain strip");
        addSelectedBtn.onClick = [this]() {
            juce::StringArray selected = folderBrowser->getSelectedFilePaths();
            if (selected.isEmpty()) return;
            if (onFilesSelected)
                onFilesSelected(selected);
        };
        addAndMakeVisible(addSelectedBtn);
    }

    ~OutshineInputPanel() override = default;

    void setRecentGrains(const juce::StringArray& recents) { recentGrains = recents; }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

        if (isModeA)
        {
            // Draw drop zone on the left half
            if (dropZoneDragActive)
            {
                g.setColour(GalleryColors::get(GalleryColors::xoGold));
                g.drawRoundedRectangle(dropZoneArea.toFloat().reduced(1), 8.0f, 2.0f);
            }
            else
            {
                g.setColour(GalleryColors::get(GalleryColors::borderGray()));
                // Dashed border
                juce::Path dashedPath;
                dashedPath.addRoundedRectangle(dropZoneArea.toFloat().reduced(1), 8.0f);
                juce::PathStrokeType stroke(1.0f);
                juce::Path stroked;
                float dashData[] = { 4.0f, 4.0f };
                stroke.createDashedStroke(stroked, dashedPath, dashData, 2);
                g.fillPath(stroked);
            }

            // Drop zone text
            g.setColour(GalleryColors::get(GalleryColors::textDark()));
            g.setFont(GalleryFonts::body(13.0f));
            g.drawText("Drop grains here",
                       dropZoneArea.withHeight(dropZoneArea.getHeight() / 2).translated(0, dropZoneArea.getHeight() / 4 - 10),
                       juce::Justification::centred);

            g.setColour(GalleryColors::get(GalleryColors::textMid()));
            g.setFont(GalleryFonts::body(11.0f));
            g.drawText("WAV \xe2\x80\xa2 Folders \xe2\x80\xa2 XPN",
                       dropZoneArea.withHeight(dropZoneArea.getHeight() / 2).translated(0, dropZoneArea.getHeight() / 4 + 10),
                       juce::Justification::centred);
        }
    }

    void resized() override
    {
        const bool shouldBeModeA = (getWidth() >= 700);

        if (shouldBeModeA != isModeA)
            switchToMode(shouldBeModeA);

        if (isModeA)
        {
            auto area = getLocalBounds().reduced(8);
            auto left  = area.removeFromLeft(area.getWidth() / 2).reduced(4);
            auto right = area.reduced(4);

            dropZoneArea = left;

            auto btnBar = right.removeFromBottom(kButtonBarH);
            int btnW = btnBar.getWidth() / 2 - 4;
            selectAllBtn.setBounds(btnBar.removeFromLeft(btnW));
            btnBar.removeFromLeft(8);
            addSelectedBtn.setBounds(btnBar.removeFromLeft(btnW));

            folderBrowser->setBounds(right);
            selectAllBtn.setVisible(true);
            addSelectedBtn.setVisible(true);

            if (tabComponent)
                tabComponent->setVisible(false);
        }
        else
        {
            if (tabComponent)
            {
                tabComponent->setBounds(getLocalBounds());
                tabComponent->setVisible(true);
            }
            selectAllBtn.setVisible(false);
            addSelectedBtn.setVisible(false);
        }
    }

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }

    void fileDragEnter(const juce::StringArray&, int, int) override
    {
        dropZoneDragActive = true;
        repaint();
    }

    void fileDragExit(const juce::StringArray&) override
    {
        dropZoneDragActive = false;
        repaint();
    }

    void filesDropped(const juce::StringArray& files, int, int) override
    {
        dropZoneDragActive = false;
        repaint();
        if (onFilesSelected)
            onFilesSelected(files);
    }

private:
    void buildModeB()
    {
        tabComponent = std::make_unique<juce::TabbedComponent>(
            juce::TabbedButtonBar::TabsAtTop);
        tabComponent->setColour(juce::TabbedComponent::backgroundColourId,
                                GalleryColors::get(GalleryColors::shellWhite()));

        tabFolderBrowser = std::make_unique<OutshineFolderBrowser>();
        tabFolderBrowser->onFilesConfirmed = [this](const juce::StringArray& files) {
            if (onFilesSelected)
                onFilesSelected(files);
        };

        tabComponent->addTab("Drop Zone",     GalleryColors::get(GalleryColors::shellWhite()), nullptr, false);
        tabComponent->addTab("Browse Files",  GalleryColors::get(GalleryColors::shellWhite()), tabFolderBrowser.get(), false);
        tabComponent->addTab("Recent Grains", GalleryColors::get(GalleryColors::shellWhite()), nullptr, false);
        addAndMakeVisible(*tabComponent);
    }

    void switchToMode(bool modeA)
    {
        isModeA = modeA;
        if (!modeA && !tabComponent)
            buildModeB();
    }

    bool isModeA { true };
    bool dropZoneDragActive { false };
    juce::Rectangle<int> dropZoneArea;

    std::unique_ptr<OutshineFolderBrowser>   folderBrowser;
    juce::TextButton                          selectAllBtn { "Select All WAVs" };
    juce::TextButton                          addSelectedBtn { "Add Selected \xe2\x86\x92" };

    std::unique_ptr<juce::TabbedComponent>   tabComponent;
    std::unique_ptr<OutshineFolderBrowser>   tabFolderBrowser;

    juce::StringArray recentGrains;

    static constexpr int kButtonH    = 28;
    static constexpr int kButtonBarH = 36;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineInputPanel)
};

} // namespace xoceanus
