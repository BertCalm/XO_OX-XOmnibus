#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"

namespace xolokun {

class OutshineShellState : public juce::Component,
                           public juce::FileDragAndDropTarget
{
public:
    std::function<void(const juce::StringArray&)> onFilesDropped;

    OutshineShellState()
    {
        setWantsKeyboardFocus(false);
        A11y::setup(*this, "Outshine Shell", "Drop WAV files, folders, or XPN archives to begin");
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

        // Gold border glow on drag-enter
        if (dragActive)
        {
            g.setColour(GalleryColors::get(GalleryColors::xoGold).withAlpha(0.7f));
            g.drawRoundedRectangle(getLocalBounds().reduced(4).toFloat(), 8.0f, 2.0f);
        }
        else
        {
            g.setColour(GalleryColors::get(GalleryColors::borderGray()));
            g.drawRoundedRectangle(getLocalBounds().reduced(4).toFloat(), 8.0f, 1.0f);
        }

        // Oyster icon placeholder (64x64 centered)
        auto center = getLocalBounds().getCentre();
        juce::Rectangle<float> iconArea(center.x - 32.0f,
                                        center.y - 60.0f,
                                        64.0f, 64.0f);
        g.setColour(GalleryColors::get(GalleryColors::borderGray()));
        g.fillEllipse(iconArea);
        g.setColour(GalleryColors::get(GalleryColors::textMid()));
        g.setFont(GalleryFonts::value(22.0f));
        g.drawText(juce::String(juce::CharPointer_UTF8("\xf0\x9f\xaa\xb8")),
                   iconArea.toNearestInt(), juce::Justification::centred);

        // Text labels
        g.setColour(GalleryColors::get(GalleryColors::textDark()));
        g.setFont(GalleryFonts::display(14.0f));
        g.drawText("The shell is open.",
                   getLocalBounds().withY(center.y + 16).withHeight(22),
                   juce::Justification::centred);

        g.setColour(GalleryColors::get(GalleryColors::textMid()));
        g.setFont(GalleryFonts::body(13.0f));
        g.drawText("Drop a grain to begin.",
                   getLocalBounds().withY(center.y + 40).withHeight(18),
                   juce::Justification::centred);

        g.setColour(juce::Colour(0xFF666666));
        g.setFont(GalleryFonts::body(11.0f));
        g.drawText("WAV files, folders, or XPN archives.",
                   getLocalBounds().withY(center.y + 60).withHeight(16),
                   juce::Justification::centred);
    }

    void resized() override {}

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }

    void fileDragEnter(const juce::StringArray&, int, int) override
    {
        dragActive = true;
        repaint();
    }

    void fileDragExit(const juce::StringArray&) override
    {
        dragActive = false;
        repaint();
    }

    void filesDropped(const juce::StringArray& files, int, int) override
    {
        dragActive = false;
        repaint();
        if (onFilesDropped)
            onFilesDropped(files);
    }

private:
    bool dragActive { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineShellState)
};

} // namespace xolokun
