// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "OutshineDocumentWindow.h"
#include "../../XOceanusProcessor.h"

namespace xoceanus
{

class OutshineSidebarPanel : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    explicit OutshineSidebarPanel(XOceanusProcessor& processorRef) : processor(processorRef)
    {
        setWantsKeyboardFocus(false);
        A11y::setup(*this, "Outshine Sidebar", "Drop grains or open the full Outshine window");
        buildLayout();
    }

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }

    void fileDragEnter(const juce::StringArray&, int, int) override
    {
        dropActive = true;
        repaint();
    }

    void fileDragExit(const juce::StringArray&) override
    {
        dropActive = false;
        repaint();
    }

    void filesDropped(const juce::StringArray& files, int, int) override
    {
        dropActive = false;
        repaint();

        juce::StringArray paths;
        for (const auto& f : files)
        {
            juce::File file(f);
            if (file.hasFileExtension(".xpn"))
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::InfoIcon, "XPN Upgrade",
                    "Original program data will be rebuilt from scratch.\n"
                    "Zone map, expression routes, and LUFS normalization will be regenerated.",
                    "OK");
            }
            paths.add(f);
        }

        if (!paths.isEmpty())
            launchWithGrains(paths);
    }

    void onPearlExported(const juce::String& pearlName, int numZones)
    {
        juce::String truncated = pearlName.length() > 24
                                     ? pearlName.substring(0, 22) + juce::String(juce::CharPointer_UTF8("\xe2\x80\xa6"))
                                     : pearlName;
        lastPearlStatus = truncated + " (" + juce::String(numZones) + " zones)";
        statusLabel.setText(lastPearlStatus, juce::dontSendNotification);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

        // Header bar
        auto headerArea = getLocalBounds().removeFromTop(kHeaderH);
        g.setColour(GalleryColors::get(GalleryColors::xoGold));
        g.fillRect(headerArea);
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::textDark())));
        g.setFont(GalleryFonts::heading(9.0f));
        g.drawText("OUTSHINE", headerArea.reduced(8, 0), juce::Justification::centredLeft);

        // Drop zone
        auto b = getLocalBounds().reduced(8);
        b.removeFromTop(kHeaderH);
        auto dropZone = b.removeFromTop(kDropZoneH);

        juce::Colour borderCol =
            dropActive ? GalleryColors::get(GalleryColors::xoGold) : GalleryColors::get(GalleryColors::borderGray());

        // Dashed border via path stroke
        g.setColour(borderCol);
        juce::Path dashedRect;
        dashedRect.addRoundedRectangle(dropZone.toFloat().reduced(1), 6.0f);
        juce::Path stroked;
        juce::PathStrokeType stroke(dropActive ? 2.0f : 1.0f);
        float dashData[] = {4.0f, 4.0f};
        stroke.createDashedStroke(stroked, dashedRect, dashData, 2);
        g.fillPath(stroked);

        g.setColour(GalleryColors::get(GalleryColors::textMid()));
        g.setFont(GalleryFonts::body(11.0f));
        g.drawText("Drop a grain here", dropZone, juce::Justification::centred);

        // Section divider
        b.removeFromTop(4);
        g.setColour(GalleryColors::get(GalleryColors::borderGray()));
        g.drawLine(8, (float)(kHeaderH + 8 + kDropZoneH + 4), (float)(getWidth() - 8),
                   (float)(kHeaderH + 8 + kDropZoneH + 4), 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(8);
        area.removeFromTop(kHeaderH);   // header is painted, not a child
        area.removeFromTop(kDropZoneH); // drop zone is painted
        area.removeFromTop(8);
        statusLabel.setBounds(area.removeFromTop(kStatusH));
        area.removeFromTop(4);
        openOysterBtn.setBounds(area.removeFromTop(kButtonH));
    }

private:
    void buildLayout()
    {
        // Status
        statusLabel.setText(lastPearlStatus, juce::dontSendNotification);
        statusLabel.setFont(GalleryFonts::body(11.0f));
        statusLabel.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()));
        addAndMakeVisible(statusLabel);

        // Open button
        openOysterBtn.setColour(juce::TextButton::buttonColourId, GalleryColors::get(GalleryColors::xoGold));
        openOysterBtn.setColour(juce::TextButton::textColourOffId,
                                juce::Colour(GalleryColors::get(GalleryColors::textDark())));
        openOysterBtn.onClick = [this]() { launchOutshine(); };
        A11y::setup(openOysterBtn, "Open the Oyster", "Open the full Outshine window");
        addAndMakeVisible(openOysterBtn);
    }

    void launchOutshine()
    {
        if (outshineWindow != nullptr)
        {
            outshineWindow->toFront(true);
            return;
        }
        outshineWindow = new OutshineDocumentWindow(processor);
        outshineWindow->centreWithSize(900, 660);
        outshineWindow->setVisible(true);
    }

    void launchWithGrains(const juce::StringArray& filePaths)
    {
        launchOutshine();

        if (auto* doc = outshineWindow.getComponent())
        {
            if (auto* mc = dynamic_cast<OutshineMainComponent*>(doc->getContentComponent()))
                mc->onGrainsChanged(filePaths);
        }
    }

    XOceanusProcessor& processor;
    juce::Component::SafePointer<OutshineDocumentWindow> outshineWindow;

    juce::Label statusLabel;
    juce::TextButton openOysterBtn{"Open the Oyster"};

    bool dropActive{false};
    juce::String lastPearlStatus{"No pearls forged yet"};

    static constexpr int kHeaderH = 28;
    static constexpr int kDropZoneH = 60;
    static constexpr int kStatusH = 32;
    static constexpr int kButtonH = 32;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineSidebarPanel)
};

} // namespace xoceanus
