#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "OutshineZoneMap.h"
#include "OutshineMPEPanel.h"
#include "../../Export/XOutshine.h"

namespace xolokun {

class OutshineAutoMode : public juce::Component
{
public:
    OutshineAutoMode()
    {
        setWantsKeyboardFocus(false);
        A11y::setup(*this, "Auto Mode Panel", "Classification summary, zone map, and MPE configuration");

        zoneMap  = std::make_unique<OutshineZoneMap>();
        mpePanel = std::make_unique<OutshineMPEPanel>();

        addAndMakeVisible(*zoneMap);
        addAndMakeVisible(*mpePanel);

        buildSummaryGrid();
        buildRebirthTeaser();
    }

    void populate(const std::vector<AnalyzedSample>& samples)
    {
        if (samples.empty()) return;

        bool isDrum = isDrumCategory(samples[0].category);

        typeValue.setText(isDrum ? "Drum" : "Keygroup", juce::dontSendNotification);
        zonesValue.setText(juce::String((int)samples.size()), juce::dontSendNotification);
        velValue.setText("4", juce::dontSendNotification);
        rrValue.setText("2", juce::dontSendNotification);

        zoneMap->setSamples(samples);
        mpePanel->setCategoryDefaults(isDrum ? MPECategory::Drum : MPECategory::Melodic);

        repaint();
    }

    OutshineZoneMap&   getZoneMap()  { return *zoneMap; }
    OutshineMPEPanel&  getMPEPanel() { return *mpePanel; }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

        // Header bar
        auto headerArea = getLocalBounds().removeFromTop(kHeaderH);
        g.setColour(GalleryColors::get(GalleryColors::xoGold));
        g.fillRect(headerArea);
        g.setColour(juce::Colour(GalleryColors::Light::textDark));
        g.setFont(GalleryFonts::display(13.0f));
        g.drawText("AUTO MODE", headerArea.reduced(12, 0), juce::Justification::centredLeft);

        // Spectrum View placeholder (below MPE panel)
        auto area = getLocalBounds().reduced(kPad);
        area.removeFromTop(kHeaderH + kSummaryH + 8 + kZoneMapH + 8 + kMPEH + 8 + kTeaserH + 8);
        if (area.getHeight() > 20)
        {
            g.setColour(GalleryColors::get(GalleryColors::borderGray()).withAlpha(0.2f));
            g.drawRoundedRectangle(area.toFloat().reduced(4), 6.0f, 1.0f);
            g.setColour(GalleryColors::get(GalleryColors::textMid()).withAlpha(0.35f));
            g.setFont(GalleryFonts::body(11.0f));
            g.drawText("Spectrum View \xe2\x80\x94 Phase 1B", area, juce::Justification::centred);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromTop(kHeaderH);
        area.reduce(kPad, 0);
        area.removeFromTop(4);

        // Summary grid: 2x2 layout
        auto summaryArea = area.removeFromTop(kSummaryH);
        int rowH = summaryArea.getHeight() / 2;
        int col1W = summaryArea.getWidth() / 2;

        auto row1 = summaryArea.removeFromTop(rowH);
        typeLabel.setBounds(row1.removeFromLeft(col1W / 2));
        typeValue.setBounds(row1.removeFromLeft(col1W / 2));
        zonesLabel.setBounds(row1.removeFromLeft(col1W / 2));
        zonesValue.setBounds(row1);

        auto row2 = summaryArea.removeFromTop(rowH);
        velLabel.setBounds(row2.removeFromLeft(col1W / 2));
        velValue.setBounds(row2.removeFromLeft(col1W / 2));
        rrLabel.setBounds(row2.removeFromLeft(col1W / 2));
        rrValue.setBounds(row2);

        area.removeFromTop(8);
        zoneMap->setBounds(area.removeFromTop(kZoneMapH));
        area.removeFromTop(8);
        mpePanel->setBounds(area.removeFromTop(kMPEH));
        area.removeFromTop(8);
        rebirthTeaser.setBounds(area.removeFromTop(kTeaserH));
    }

private:
    void buildSummaryGrid()
    {
        auto setupLabel = [this](juce::Label& lbl, const juce::String& text) {
            lbl.setText(text, juce::dontSendNotification);
            lbl.setFont(GalleryFonts::body(12.0f));
            lbl.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()));
            addAndMakeVisible(lbl);
        };
        auto setupValue = [this](juce::Label& lbl, const juce::String& text) {
            lbl.setText(text, juce::dontSendNotification);
            lbl.setFont(GalleryFonts::value(12.0f));
            lbl.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textDark()));
            lbl.setJustificationType(juce::Justification::centredRight);
            addAndMakeVisible(lbl);
        };

        setupLabel(typeLabel,  "Type");
        setupValue(typeValue,  "—");
        setupLabel(zonesLabel, "Zones");
        setupValue(zonesValue, "—");
        setupLabel(velLabel,   "Vel Layers");
        setupValue(velValue,   "—");
        setupLabel(rrLabel,    "Round Robin");
        setupValue(rrValue,    "—");
    }

    void buildRebirthTeaser()
    {
        rebirthTeaser.setText("Rebirth Mode \xe2\x80\x94 Phase 1B\n"
                              "Route grains through OPERA \xc2\xb7 OBRIX \xc2\xb7 ONSET",
                              juce::dontSendNotification);
        rebirthTeaser.setFont(GalleryFonts::body(11.0f));
        rebirthTeaser.setColour(juce::Label::textColourId,
                                GalleryColors::get(GalleryColors::textMid()));
        rebirthTeaser.setAlpha(0.35f);
        addAndMakeVisible(rebirthTeaser);
    }

    juce::Label typeLabel,   typeValue;
    juce::Label zonesLabel,  zonesValue;
    juce::Label velLabel,    velValue;
    juce::Label rrLabel,     rrValue;

    std::unique_ptr<OutshineZoneMap>   zoneMap;
    std::unique_ptr<OutshineMPEPanel>  mpePanel;
    juce::Label                        rebirthTeaser;

    static constexpr int kHeaderH   = 36;
    static constexpr int kSummaryH  = 60;
    static constexpr int kZoneMapH  = 64;
    static constexpr int kMPEH      = 120;
    static constexpr int kTeaserH   = 60;
    static constexpr int kPad       = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineAutoMode)
};

} // namespace xolokun
