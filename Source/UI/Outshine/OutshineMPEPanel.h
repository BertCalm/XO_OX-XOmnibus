#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"

namespace xoceanus {

enum class MPECategory { Melodic, Drum };

struct MPERoute
{
    juce::String dimensionName;
    juce::String destination;
    float        amount { 0.0f };
    bool         active { true };
    bool         manualConfigOnly { false };
    float        liveValue { 0.0f };
};

class OutshineMPEPanel : public juce::Component
{
public:
    OutshineMPEPanel()
    {
        setWantsKeyboardFocus(false);
        A11y::setup(*this, "MPE Expression Panel", "MPE expression route configuration");
    }

    void setCategoryDefaults(MPECategory category)
    {
        routes.clear();
        if (category == MPECategory::Melodic)
        {
            routes.push_back({ "Slide (CC74)", "Filter Cutoff",      0.80f, true,  true  });
            routes.push_back({ "Pressure",     "Filter Resonance",   0.40f, true,  false });
            routes.push_back({ "Pitch Bend",   "\xc2\xb1" "24 semitones", 1.0f, true,  false });
            routes.push_back({ "Velocity",     "Amplitude + Bright", 1.0f,  true,  false });
            routes.push_back({ "ModWheel",     "Filter Cutoff",      0.50f, true,  false });
        }
        else
        {
            routes.push_back({ "Slide (CC74)", "Pitch \xc2\xb1" "2 semi", 0.0f, true, true  });
            routes.push_back({ "Pressure",     "Choke Speed",        0.40f, true,  false });
            routes.push_back({ "Pitch Bend",   "(omitted)",          0.0f,  false, false });
            routes.push_back({ "Velocity",     "Amplitude + Drive",  1.0f,  true,  false });
            routes.push_back({ "ModWheel",     "(omitted)",          0.0f,  false, false });
        }
        repaint();
    }

    void setLiveValue(int routeIndex, float value)
    {
        if (routeIndex >= 0 && routeIndex < (int)routes.size())
        {
            routes[(size_t)routeIndex].liveValue = value;
            repaint();
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

        auto area = getLocalBounds().reduced(kPad, 0);
        for (int i = 0; i < (int)routes.size(); ++i)
        {
            auto rowBounds = area.removeFromTop(kRowH);
            paintRoute(g, routes[(size_t)i], rowBounds);

            if (i < (int)routes.size() - 1)
            {
                g.setColour(GalleryColors::get(GalleryColors::borderGray()));
                g.drawLine((float)rowBounds.getX(), (float)rowBounds.getBottom(),
                           (float)rowBounds.getRight(), (float)rowBounds.getBottom(), 1.0f);
            }
        }
    }

    void resized() override {}

private:
    void paintRoute(juce::Graphics& g, const MPERoute& route,
                    juce::Rectangle<int> rowBounds) const
    {
        if (!route.active)
        {
            g.setColour(GalleryColors::get(GalleryColors::textMid()).withAlpha(0.4f));
        }

        // Gold left border for active routes
        if (route.active && route.amount != 0.0f)
        {
            g.setColour(GalleryColors::get(GalleryColors::xoGold));
            g.fillRect(rowBounds.removeFromLeft(2));
        }
        else
        {
            rowBounds.removeFromLeft(2);
        }

        rowBounds.reduce(4, 2);

        // Dimension name
        g.setColour(route.active ? GalleryColors::get(GalleryColors::textDark())
                                 : GalleryColors::get(GalleryColors::textMid()).withAlpha(0.4f));
        g.setFont(GalleryFonts::body(11.0f));
        int nameW = 110;
        g.drawText(route.dimensionName, rowBounds.removeFromLeft(nameW),
                   juce::Justification::centredLeft);

        // Arrow
        g.setColour(GalleryColors::get(GalleryColors::textMid()));
        g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x86\x92")),
                   rowBounds.removeFromLeft(16),
                   juce::Justification::centred);

        // Destination label
        int destW = 120;
        g.setColour(route.active ? GalleryColors::get(GalleryColors::textDark())
                                 : GalleryColors::get(GalleryColors::textMid()).withAlpha(0.4f));
        g.drawText(route.destination, rowBounds.removeFromLeft(destW),
                   juce::Justification::centredLeft);

        // Mini progress bar
        if (route.active && route.amount != 0.0f)
        {
            auto barBounds = rowBounds.removeFromLeft(64).reduced(0, 6);
            g.setColour(GalleryColors::get(GalleryColors::borderGray()));
            g.fillRoundedRectangle(barBounds.toFloat(), 3.0f);
            g.setColour(GalleryColors::get(GalleryColors::xoGold));
            g.fillRoundedRectangle(barBounds.toFloat().withWidth(barBounds.getWidth() * std::abs(route.amount)), 3.0f);
        }

        // "(manual config)" annotation
        if (route.manualConfigOnly)
        {
            g.setColour(juce::Colour(0xFFE9A84A));
            g.setFont(GalleryFonts::body(9.0f));
            g.drawText("(manual config)", rowBounds, juce::Justification::centredLeft);
        }
    }

    std::vector<MPERoute> routes;

    static constexpr int kRowH = 24;
    static constexpr int kPad  = 8;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineMPEPanel)
};

} // namespace xoceanus
