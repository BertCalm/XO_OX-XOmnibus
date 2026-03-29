#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "OutshineZoneMap.h"
#include "OutshineMPEPanel.h"
#include "../../Export/XOutshine.h"
#include "../../Export/RebirthProfiles.h"

namespace xolokun {

class OutshineAutoMode : public juce::Component,
                         private juce::Button::Listener,
                         private juce::Slider::Listener
{
public:
    OutshineAutoMode()
    {
        setWantsKeyboardFocus(false);
        A11y::setup(*this, "Auto Mode Panel", "Classification summary, zone map, MPE configuration, and Rebirth Mode");

        zoneMap  = std::make_unique<OutshineZoneMap>();
        mpePanel = std::make_unique<OutshineMPEPanel>();

        addAndMakeVisible(*zoneMap);
        addAndMakeVisible(*mpePanel);

        buildSummaryGrid();
        buildRebirthPanel();
    }

    ~OutshineAutoMode() override
    {
        rebirthToggle.removeListener(this);
        intensitySlider.removeListener(this);
        for (auto* btn : profileButtons)
            btn->removeListener(this);
    }

    void populate(const std::vector<AnalyzedSample>& samples)
    {
        if (samples.empty()) return;

        detectedCategory = samples[0].category;
        bool isDrum = isDrumCategory(detectedCategory);

        typeValue.setText(isDrum ? "Drum" : "Keygroup", juce::dontSendNotification);
        zonesValue.setText(juce::String((int)samples.size()), juce::dontSendNotification);
        velValue.setText("4", juce::dontSendNotification);
        rrValue.setText("2", juce::dontSendNotification);

        zoneMap->setSamples(samples);
        mpePanel->setCategoryDefaults(isDrum ? MPECategory::Drum : MPECategory::Melodic);

        // Auto-select recommended profile for detected category
        auto recommendedId = autoProfileForCategory(detectedCategory);
        selectProfile(recommendedId, false /* don't fire callback — auto-selection */);

        updateMismatchWarning();
        repaint();
    }

    OutshineZoneMap&   getZoneMap()  { return *zoneMap; }
    OutshineMPEPanel&  getMPEPanel() { return *mpePanel; }

    // Returns the current Rebirth settings for use by the export pipeline.
    RebirthSettings getRebirthSettings() const
    {
        RebirthSettings s;
        s.enabled   = rebirthToggle.getToggleState();
        s.profileId = activeProfileId;
        s.intensity = (float)intensitySlider.getValue();
        s.chaosAmount = 0.0f; // Phase 1C
        return s;
    }

    // Optional callback — called whenever any Rebirth setting changes.
    std::function<void(const RebirthSettings&)> onRebirthChanged;

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

        // Header bar
        auto headerArea = getLocalBounds().removeFromTop(kHeaderH);
        g.setColour(GalleryColors::get(GalleryColors::xoGold));
        g.fillRect(headerArea);
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::textDark())));
        g.setFont(GalleryFonts::display(13.0f));
        g.drawText("AUTO MODE", headerArea.reduced(12, 0), juce::Justification::centredLeft);

        // Rebirth section header
        auto area = getLocalBounds().reduced(kPad);
        area.removeFromTop(kHeaderH + kSummaryH + 8 + kZoneMapH + 8 + kMPEH + 8);

        // Section divider line above Rebirth
        g.setColour(GalleryColors::get(GalleryColors::borderGray()));
        g.drawLine((float)area.getX(), (float)area.getY(),
                   (float)area.getRight(), (float)area.getY(), 1.0f);
        area.removeFromTop(4);

        // "REBIRTH MODE" label next to toggle
        auto toggleRow = area.removeFromTop(kToggleH);
        // Leave space for toggle button (drawn as component) then draw label
        auto labelAfterToggle = toggleRow;
        labelAfterToggle.removeFromLeft(kToggleW + 8);
        g.setColour(GalleryColors::get(GalleryColors::textDark()));
        g.setFont(GalleryFonts::display(12.0f));
        g.drawText("REBIRTH MODE", labelAfterToggle, juce::Justification::centredLeft);

        // Spectrum View placeholder (below all panels)
        auto remaining = getLocalBounds().reduced(kPad);
        remaining.removeFromTop(kHeaderH + kSummaryH + 8 + kZoneMapH + 8 + kMPEH + 8
                                + 4 + kToggleH + kProfileH + kSliderH + kDescH + kWarnH + 12);
        if (remaining.getHeight() > 20)
        {
            g.setColour(GalleryColors::get(GalleryColors::borderGray()).withAlpha(0.2f));
            g.drawRoundedRectangle(remaining.toFloat().reduced(4), 6.0f, 1.0f);
            g.setColour(GalleryColors::get(GalleryColors::textMid()).withAlpha(0.35f));
            g.setFont(GalleryFonts::body(11.0f));
            g.drawText("Spectrum View \xe2\x80\x94 Phase 1B", remaining, juce::Justification::centred);
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

        // Rebirth section divider gap + toggle row
        area.removeFromTop(4); // matches divider line gap in paint()

        auto toggleRow = area.removeFromTop(kToggleH);
        rebirthToggle.setBounds(toggleRow.removeFromLeft(kToggleW).reduced(0, 4));

        // Profile card buttons row
        auto profileRow = area.removeFromTop(kProfileH);
        int cardW = profileRow.getWidth() / kNumProfiles;
        for (auto* btn : profileButtons)
        {
            btn->setBounds(profileRow.removeFromLeft(cardW).reduced(2, 2));
        }

        // Intensity slider row
        auto sliderRow = area.removeFromTop(kSliderH);
        intensityLabel.setBounds(sliderRow.removeFromLeft(72));
        intensitySlider.setBounds(sliderRow);

        // Per-profile intensity description
        intensityDesc.setBounds(area.removeFromTop(kDescH));

        // Material mismatch warning
        mismatchWarning.setBounds(area.removeFromTop(kWarnH));
    }

private:
    // -------------------------------------------------------------------------
    // Build helpers
    // -------------------------------------------------------------------------

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
        setupValue(typeValue,  "\xe2\x80\x94");
        setupLabel(zonesLabel, "Zones");
        setupValue(zonesValue, "\xe2\x80\x94");
        setupLabel(velLabel,   "Vel Layers");
        setupValue(velValue,   "\xe2\x80\x94");
        setupLabel(rrLabel,    "Round Robin");
        setupValue(rrValue,    "\xe2\x80\x94");
    }

    void buildRebirthPanel()
    {
        // ------------------------------------------------------------------
        // Toggle button
        // ------------------------------------------------------------------
        rebirthToggle.setButtonText("Enable");
        rebirthToggle.setToggleState(false, juce::dontSendNotification);
        rebirthToggle.setColour(juce::ToggleButton::textColourId,
                                GalleryColors::get(GalleryColors::textMid()));
        rebirthToggle.addListener(this);
        addAndMakeVisible(rebirthToggle);

        // ------------------------------------------------------------------
        // Profile card buttons (radio group)
        //
        // Accent colors from CLAUDE.md engine table:
        //   OBRIX    Reef Jade              #1E8B7E
        //   ONSET    Electric Blue          #0066FF
        //   OWARE    Akan Goldweight        #B5883E
        //   OPERA    Aria Gold              #D4AF37
        //   OVERWASH Tide Foam White        #F0F8FF  (use a readable tint instead)
        // ------------------------------------------------------------------
        static const struct ProfileInfo
        {
            RebirthProfileID id;
            const char*      engineName;
            const char*      producerLabel;
            juce::uint32     accentArgb;
        } kProfiles[kNumProfiles] = {
            { RebirthProfileID::OBRIX,    "OBRIX",    "Harmonic Character", 0xFF1E8B7E },
            { RebirthProfileID::ONSET,    "ONSET",    "Percussive Crunch",  0xFF0066FF },
            { RebirthProfileID::OWARE,    "OWARE",    "Resonant Body",      0xFFB5883E },
            { RebirthProfileID::OPERA,    "OPERA",    "Harmonic Shimmer",   0xFFD4AF37 },
            { RebirthProfileID::OVERWASH, "OVERWASH", "Deep Diffusion",     0xFF4A90D9 },
        };

        for (int i = 0; i < kNumProfiles; ++i)
        {
            auto* btn = profileButtons[i];
            const auto& info = kProfiles[i];

            btn->setButtonText(juce::String(info.engineName) + "\n" + info.producerLabel);
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(kProfileRadioGroup);

            // Style: use accent color as active background, grey when inactive
            juce::Colour accent = juce::Colour(info.accentArgb);
            btn->setColour(juce::TextButton::buttonColourId,
                           GalleryColors::get(GalleryColors::borderGray()));
            btn->setColour(juce::TextButton::buttonOnColourId,  accent);
            btn->setColour(juce::TextButton::textColourOffId,
                           GalleryColors::get(GalleryColors::textMid()));
            btn->setColour(juce::TextButton::textColourOnId,
                           accent.getBrightness() > 0.6f
                               ? juce::Colours::black
                               : juce::Colours::white);

            // Tag which profile this button represents via component ID
            btn->setComponentID(juce::String(i));

            btn->addListener(this);
            addAndMakeVisible(btn);
        }

        // Select OBRIX as default
        activeProfileId = RebirthProfileID::OBRIX;
        profileButtons[0]->setToggleState(true, juce::dontSendNotification);

        // ------------------------------------------------------------------
        // Intensity slider
        // ------------------------------------------------------------------
        intensityLabel.setText("Intensity", juce::dontSendNotification);
        intensityLabel.setFont(GalleryFonts::body(11.0f));
        intensityLabel.setColour(juce::Label::textColourId,
                                 GalleryColors::get(GalleryColors::textMid()));
        intensityLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(intensityLabel);

        intensitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
        intensitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
        intensitySlider.setRange(0.0, 1.0, 0.01);
        intensitySlider.setValue(0.7, juce::dontSendNotification);
        intensitySlider.setColour(juce::Slider::thumbColourId,
                                  GalleryColors::get(GalleryColors::xoGold));
        intensitySlider.setColour(juce::Slider::trackColourId,
                                  GalleryColors::get(GalleryColors::xoGold).withAlpha(0.5f));
        intensitySlider.addListener(this);
        addAndMakeVisible(intensitySlider);

        // ------------------------------------------------------------------
        // Per-profile intensity description
        // ------------------------------------------------------------------
        intensityDesc.setFont(GalleryFonts::body(10.0f));
        intensityDesc.setColour(juce::Label::textColourId,
                                GalleryColors::get(GalleryColors::textMid()));
        intensityDesc.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(intensityDesc);

        // ------------------------------------------------------------------
        // Material mismatch warning
        // ------------------------------------------------------------------
        mismatchWarning.setFont(GalleryFonts::body(10.0f));
        mismatchWarning.setColour(juce::Label::textColourId,
                                  juce::Colour(0xFFE07B39)); // warm warning amber
        mismatchWarning.setJustificationType(juce::Justification::centredLeft);
        mismatchWarning.setVisible(false);
        addAndMakeVisible(mismatchWarning);

        // Set initial description from default profile
        updateProfileDescription();
        setRebirthControlsEnabled(false); // disabled until toggle is on
    }

    // -------------------------------------------------------------------------
    // State updates
    // -------------------------------------------------------------------------

    void selectProfile(RebirthProfileID id, bool fireCallback = true)
    {
        activeProfileId = id;

        // Sync button states
        for (auto* btn : profileButtons)
        {
            int idx = btn->getComponentID().getIntValue();
            bool shouldBeOn = (static_cast<int>(id) == idx);
            if (btn->getToggleState() != shouldBeOn)
                btn->setToggleState(shouldBeOn, juce::dontSendNotification);
        }

        updateProfileDescription();
        updateMismatchWarning();

        if (fireCallback && onRebirthChanged)
            onRebirthChanged(getRebirthSettings());
    }

    void updateProfileDescription()
    {
        const auto& profile = getRebirthProfile(activeProfileId);
        intensityDesc.setText(profile.intensityDescription, juce::dontSendNotification);
    }

    void updateMismatchWarning()
    {
        const auto& profile = getRebirthProfile(activeProfileId);

        // Check whether the detected sample category matches the profile's
        // intended material. A mismatch is flagged when:
        //   - The profile is PERCUSSIVE-optimised (ONSET) but sample is melodic, or
        //   - The profile is TONAL-optimised (OPERA/OBRIX) but sample is percussive, or
        //   - The profile warns via recommendedTransientMax against the current category.
        //
        // Simple heuristic: if the auto-recommended profile for this category
        // differs from the selected profile, surface a gentle warning.
        bool categoryKnown = (detectedCategory != SampleCategory::Unknown);

        if (!categoryKnown)
        {
            mismatchWarning.setVisible(false);
            return;
        }

        auto recommendedId = autoProfileForCategory(detectedCategory);
        if (recommendedId == activeProfileId)
        {
            mismatchWarning.setVisible(false);
            return;
        }

        // Build a human-readable mismatch string
        const auto& recommended = getRebirthProfile(recommendedId);
        juce::String msg = juce::String("\xe2\x9a\xa0\xef\xb8\x8f") // U+26A0 WARNING SIGN
                           + " This material suits "
                           + recommended.engineName
                           + " \xe2\x80\x94 current profile may yield unexpected results.";
        mismatchWarning.setText(msg, juce::dontSendNotification);
        mismatchWarning.setVisible(true);
    }

    void setRebirthControlsEnabled(bool enabled)
    {
        for (auto* btn : profileButtons)
            btn->setEnabled(enabled);

        intensitySlider.setEnabled(enabled);
        intensityDesc.setAlpha(enabled ? 1.0f : 0.4f);

        // When disabled, hide the mismatch warning even if one would exist
        if (!enabled)
            mismatchWarning.setVisible(false);
        else
            updateMismatchWarning();
    }

    // -------------------------------------------------------------------------
    // juce::Button::Listener
    // -------------------------------------------------------------------------

    void buttonClicked(juce::Button* button) override
    {
        if (button == &rebirthToggle)
        {
            setRebirthControlsEnabled(rebirthToggle.getToggleState());
            if (onRebirthChanged)
                onRebirthChanged(getRebirthSettings());
            return;
        }

        // Profile card buttons
        for (int i = 0; i < kNumProfiles; ++i)
        {
            if (button == profileButtons[i] && profileButtons[i]->getToggleState())
            {
                selectProfile(static_cast<RebirthProfileID>(i));
                return;
            }
        }
    }

    // -------------------------------------------------------------------------
    // juce::Slider::Listener
    // -------------------------------------------------------------------------

    void sliderValueChanged(juce::Slider* slider) override
    {
        if (slider == &intensitySlider)
        {
            if (onRebirthChanged)
                onRebirthChanged(getRebirthSettings());
        }
    }

    // -------------------------------------------------------------------------
    // Member variables
    // -------------------------------------------------------------------------

    // Summary grid labels
    juce::Label typeLabel,   typeValue;
    juce::Label zonesLabel,  zonesValue;
    juce::Label velLabel,    velValue;
    juce::Label rrLabel,     rrValue;

    // Sub-panels
    std::unique_ptr<OutshineZoneMap>   zoneMap;
    std::unique_ptr<OutshineMPEPanel>  mpePanel;

    // Rebirth panel controls
    juce::ToggleButton rebirthToggle;

    static constexpr int kNumProfiles     = 5;
    static constexpr int kProfileRadioGroup = 4200; // arbitrary unique group ID

    juce::TextButton profileBtn0, profileBtn1, profileBtn2, profileBtn3, profileBtn4;
    juce::TextButton* profileButtons[kNumProfiles] = {
        &profileBtn0, &profileBtn1, &profileBtn2, &profileBtn3, &profileBtn4
    };

    juce::Label  intensityLabel;
    juce::Slider intensitySlider;
    juce::Label  intensityDesc;
    juce::Label  mismatchWarning;

    // State
    RebirthProfileID activeProfileId { RebirthProfileID::OBRIX };
    SampleCategory   detectedCategory { SampleCategory::Unknown };

    // Layout constants
    static constexpr int kHeaderH   = 36;
    static constexpr int kSummaryH  = 60;
    static constexpr int kZoneMapH  = 64;
    static constexpr int kMPEH      = 120;
    // Rebirth section:
    static constexpr int kToggleH   = 28;
    static constexpr int kToggleW   = 80;
    static constexpr int kProfileH  = 44;
    static constexpr int kSliderH   = 28;
    static constexpr int kDescH     = 18;
    static constexpr int kWarnH     = 18;
    static constexpr int kPad       = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineAutoMode)
};

} // namespace xolokun
