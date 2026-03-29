#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "OutshineMainComponent.h"

namespace xolokun {

class OutshineDocumentWindow : public juce::DocumentWindow
{
public:
    explicit OutshineDocumentWindow(XOlokunProcessor& processorRef)
        : juce::DocumentWindow("OUTSHINE",
                               juce::Colour(GalleryColors::get(GalleryColors::shellWhite())),
                               juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(false);
        setResizable(true, true);
        setResizeLimits(700, 500, 1600, 1000);

        auto mc = std::make_unique<OutshineMainComponent>(processorRef);
        setContentOwned(mc.release(), true);
        centreWithSize(900, 660);
    }

    void closeButtonPressed() override
    {
        if (auto* mc = dynamic_cast<OutshineMainComponent*>(getContentComponent()))
            mc->cancelPipeline();
        setVisible(false);
        delete this;
    }

    void paint(juce::Graphics& g) override
    {
        // Custom XO Gold title bar
        auto titleBar = getLocalBounds().removeFromTop(getTitleBarHeight());
        g.setColour(GalleryColors::get(GalleryColors::xoGold));
        g.fillRect(titleBar);
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::textDark())));
        g.setFont(GalleryFonts::display(13.0f));
        g.drawText("OUTSHINE", titleBar.reduced(12, 0), juce::Justification::centredLeft);

        // Body background
        auto body = getLocalBounds().withTrimmedTop(getTitleBarHeight());
        g.setColour(GalleryColors::get(GalleryColors::shellWhite()));
        g.fillRect(body);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineDocumentWindow)
};

} // namespace xolokun
