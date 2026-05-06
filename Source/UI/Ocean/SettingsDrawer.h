// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// SettingsDrawer.h — Slide-from-right settings drawer for XOceanus Ocean View.
//
// A persistent dark panel (320px wide, full parent height) that animates in/out
// from the right edge of the ocean area.  It mirrors the EnginePickerDrawer
// pattern but slides from the RIGHT and hosts 5 settings sections:
//   1. Voice     — Polyphony, Voice Mode, Unison Voices, Unison Detune
//   2. Tuning    — Master Tune, Pitch Bend Range, Glide Time
//   3. MIDI      — MIDI Channel, MPE Mode, Velocity Curve
//   4. Engine    — Max Engines, Crossfade Time, Oversampling
//   5. Display   — UI Scale, Wave Sensitivity, Show Labels
//
// Usage (from OceanView or any parent Component):
//
//   // Declare as member:
//   xoceanus::SettingsDrawer settingsDrawer_;
//
//   // Wire up callback BEFORE adding to parent:
//   settingsDrawer_.onSettingChanged = [this](const juce::String& key, float value) {
//       handleSetting(key, value);
//   };
//
//   // Add as child — starts hidden:
//   addChildComponent(settingsDrawer_);
//
//   // In parent's resized():
//   //   Give the drawer its logical (fully-open) bounds.  The drawer moves
//   //   its own x-position each animation tick via setTopLeftPosition().
//   settingsDrawer_.setBounds(oceanArea.withLeft(oceanArea.getRight() - SettingsDrawer::kDrawerWidth)
//                                       .withWidth(SettingsDrawer::kDrawerWidth));
//
//   // Toggle open/close on a button press:
//   settingsButton_.onClick = [this]() { settingsDrawer_.toggle(); };
//
// Slide animation: 250 ms ease-out on open, linear on close. Timer ticks at
// ~16 ms (≈60 fps).  The drawer moves itself via setTopLeftPosition() — the
// parent component does not need to call resized() during animation.
//
// Thread safety: all methods must be called on the message thread.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include <algorithm>
#include <cmath>
#include <functional>

namespace xoceanus
{

//==============================================================================
class SettingsDrawer : public juce::Component,
                       public juce::Timer
{
public:
    //==========================================================================
    // Public API
    //==========================================================================

    // Fired when the user adjusts any control.
    // key   — a string identifier (e.g. "polyphony", "masterTune", "mpeMode")
    // value — normalised or raw value (depends on the control; see inline docs)
    std::function<void(const juce::String& key, float value)> onSettingChanged;

    SettingsDrawer();
    ~SettingsDrawer() override;

    // Animate slide in from the right
    void open();

    // Animate slide out to the right
    void close();

    bool isOpen()  const noexcept { return animState_ != AnimState::Closed; }
    void toggle()        { isOpen() ? close() : open(); }

    // Fix #1419: restore control values from a PropertiesFile, then broadcast
    // all settings so the processor is aligned on startup.
    void applySettings(juce::PropertiesFile& props);

    // Fix #1419: write current control values to a PropertiesFile for persistence.
    // Call whenever onSettingChanged fires to keep the file up-to-date.
    void saveSettings(juce::PropertiesFile& props) const;

    // juce::Component overrides
    void paint  (juce::Graphics& g) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    // Fix #1422: Escape key closes the drawer from within it.
    bool keyPressed(const juce::KeyPress& key) override;

    // juce::Timer override
    void timerCallback() override;

    //==========================================================================
    // Constants
    //==========================================================================
    static constexpr int kDrawerWidth = 320;

private:
    //==========================================================================
    // Design constants
    //==========================================================================
    static constexpr float kAnimDurationMs  = 250.0f;
    static constexpr int   kTimerIntervalMs = 16;
    static constexpr int   kHeaderH         = 52;
    static constexpr int   kSectionGap      = 16;
    static constexpr int   kSectionHeaderH  = 28;  // label + separator
    static constexpr int   kRowH            = 36;   // label + control row height
    static constexpr int   kScrollMarginH   = 8;    // top/bottom padding inside scroll

    //==========================================================================
    // Color palette (mirrors EnginePickerDrawer — Ocean design system)
    //==========================================================================
    static juce::Colour colDrawerBg()     noexcept { return juce::Colour(0xFF1A2332); }
    static juce::Colour colTeal()         noexcept { return juce::Colour(0xFF7FDBCA); }
    static juce::Colour colControlBg()    noexcept { return juce::Colour(0xFF243040); }
    static juce::Colour colLabel()        noexcept { return juce::Colour(0xFF8899AA); }
    static juce::Colour colValue()        noexcept { return juce::Colour(0xFFE0E8F0); }
    // Fix #1421: colBorder was identical to colControlBg — zero visual row separation.
    // Raised +12% lightness so separator lines are visually distinct from control fills.
    static juce::Colour colBorder()       noexcept { return juce::Colour(0xFF2E3E52); }
    static juce::Colour colCloseBtn()     noexcept { return juce::Colour(0xFF667788); }

    //==========================================================================
    // Animation state
    //==========================================================================
    enum class AnimState { Closed, Opening, Open, Closing };

    //==========================================================================
    // LookAndFeel for ComboBoxes — dark ocean theme
    //==========================================================================
    struct DarkComboLnF : public juce::LookAndFeel_V4
    {
        DarkComboLnF()
        {
            setColour(juce::ComboBox::backgroundColourId,    juce::Colour(0xFF243040));
            setColour(juce::ComboBox::textColourId,          juce::Colour(0xFFE0E8F0));
            setColour(juce::ComboBox::outlineColourId,       juce::Colour(0xFF2E4060));
            setColour(juce::ComboBox::arrowColourId,         juce::Colour(0xFF7FDBCA));
            setColour(juce::ComboBox::focusedOutlineColourId,juce::Colour(0xFF7FDBCA).withAlpha(0.55f));
            setColour(juce::PopupMenu::backgroundColourId,   juce::Colour(0xFF1A2332));
            setColour(juce::PopupMenu::textColourId,         juce::Colour(0xFFE0E8F0));
            setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xFF243040));
            setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colour(0xFF7FDBCA));
        }
    };

    //==========================================================================
    // LookAndFeel for Sliders — dark ocean theme
    //==========================================================================
    struct DarkSliderLnF : public juce::LookAndFeel_V4
    {
        void drawLinearSlider(juce::Graphics& g,
                              int x, int y, int width, int height,
                              float sliderPos,
                              float /*minSliderPos*/, float /*maxSliderPos*/,
                              const juce::Slider::SliderStyle /*style*/,
                              juce::Slider& /*slider*/) override
        {
            const float trackH  = 3.0f;
            const float thumbW  = 10.0f;
            const float thumbH  = 18.0f;
            const float trackY  = (float)y + ((float)height - trackH) * 0.5f;
            const float trackX1 = (float)x;
            const float trackX2 = (float)(x + width);

            // Track background
            g.setColour(juce::Colour(0xFF2E4060));
            g.fillRoundedRectangle(trackX1, trackY, trackX2 - trackX1, trackH, 1.5f);

            // Track fill (teal, from left to thumb)
            g.setColour(juce::Colour(0xFF7FDBCA).withAlpha(0.65f));
            g.fillRoundedRectangle(trackX1, trackY, sliderPos - trackX1, trackH, 1.5f);

            // Thumb
            const float thumbX = sliderPos - thumbW * 0.5f;
            const float thumbY = (float)y + ((float)height - thumbH) * 0.5f;
            g.setColour(juce::Colour(0xFF7FDBCA));
            g.fillRoundedRectangle(thumbX, thumbY, thumbW, thumbH, 3.0f);
        }

        int getSliderThumbRadius(juce::Slider&) override { return 5; }
    };

    //==========================================================================
    // LookAndFeel for ToggleButtons — dark ocean theme
    //==========================================================================
    struct DarkToggleLnF : public juce::LookAndFeel_V4
    {
        void drawToggleButton(juce::Graphics& g, juce::ToggleButton& btn,
                              bool /*hilighted*/, bool /*isDown*/) override
        {
            const bool on = btn.getToggleState();
            const auto b  = btn.getLocalBounds();

            // Track
            const float trackW = 32.0f;
            const float trackH = 16.0f;
            const float trackX = (float)b.getRight() - trackW;
            const float trackY = ((float)b.getHeight() - trackH) * 0.5f;
            g.setColour(on ? juce::Colour(0xFF2A5A50) : juce::Colour(0xFF243040));
            g.fillRoundedRectangle(trackX, trackY, trackW, trackH, trackH * 0.5f);
            g.setColour(on ? juce::Colour(0xFF7FDBCA).withAlpha(0.6f) : juce::Colour(0xFF2E4060));
            g.drawRoundedRectangle(trackX, trackY, trackW, trackH, trackH * 0.5f, 1.0f);

            // Thumb
            const float thumbD = trackH - 4.0f;
            const float thumbX = on ? (trackX + trackW - thumbD - 2.0f) : (trackX + 2.0f);
            const float thumbY = trackY + 2.0f;
            g.setColour(on ? juce::Colour(0xFF7FDBCA) : juce::Colour(0xFF667788));
            g.fillEllipse(thumbX, thumbY, thumbD, thumbD);
        }
    };

    //==========================================================================
    // Inner scrollable content component (holds all sections + controls)
    //==========================================================================
    class ContentComponent : public juce::Component
    {
    public:
        explicit ContentComponent(SettingsDrawer& owner) : owner_(owner) {}

        void paint(juce::Graphics& g) override
        {
            // Sections are painted as headers; controls paint themselves.
            for (const auto& sec : owner_.sections_)
            {
                // Section header background wash
                const auto headerR = juce::Rectangle<int>(0, sec.headerY,
                                                           getWidth(), kSectionHeaderH);
                juce::ColourGradient grad(colTeal().withAlpha(0.06f), 0.0f, (float)sec.headerY,
                                          juce::Colour(0x00000000), (float)getWidth(), 0.0f, false);
                g.setGradientFill(grad);
                g.fillRect(headerR);

                // Section title
                g.setFont(GalleryFonts::label(11.5f));
                g.setColour(colTeal());
                g.drawText(sec.title,
                           juce::Rectangle<int>(12, sec.headerY, getWidth() - 24, kSectionHeaderH - 1),
                           juce::Justification::centredLeft, false);

                // Separator line under title
                g.setColour(colTeal().withAlpha(0.18f));
                g.drawHorizontalLine(sec.headerY + kSectionHeaderH - 1, 0.0f, (float)getWidth());

                // Row labels (controls paint themselves via JUCE)
                for (const auto& row : sec.rows)
                {
                    g.setFont(GalleryFonts::body(12.0f));
                    g.setColour(colLabel());
                    g.drawText(row.label,
                               juce::Rectangle<int>(12, row.labelY, 100, kRowH),
                               juce::Justification::centredLeft, false);
                }
            }
        }

        void resized() override
        {
            owner_.layoutContent(getWidth());
        }

    private:
        SettingsDrawer& owner_;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ContentComponent)
    };

    //==========================================================================
    // Section / row data structures
    //==========================================================================

    struct RowData
    {
        juce::String label;
        int          labelY = 0;    // y of the row within ContentComponent
        juce::Component* control = nullptr;  // ComboBox, Slider, or ToggleButton (not owned here)
    };

    struct SectionData
    {
        juce::String         title;
        int                  headerY = 0;   // y of section header within ContentComponent
        std::vector<RowData> rows;
    };

    //==========================================================================
    // Private helper methods
    //==========================================================================

    void buildControls();
    void wireTooltips();   // V1 Lane B: set tooltip text on all setting controls
    void layoutContent(int contentWidth);
    void applyAnimPosition();

    void fireCombo   (const juce::String& key, juce::ComboBox&   combo);
    void fireSlider  (const juce::String& key, juce::Slider&     slider);
    void fireToggle  (const juce::String& key, juce::ToggleButton& btn);

    //==========================================================================
    // Member variables
    //==========================================================================

    // Animation
    AnimState animState_    = AnimState::Closed;
    float     animProgress_ = 0.0f;
    int64_t   animStartMs_  = 0;

    // Close button
    juce::Rectangle<int> closeBtnBounds_;
    bool                 closeBtnHovered_ = false;

    // LookAndFeels (must outlive the controls that use them)
    DarkComboLnF   comboLnF_;
    DarkSliderLnF  sliderLnF_;
    DarkToggleLnF  toggleLnF_;

    // ── Voice section ─────────────────────────────────────────────────────────
    juce::ComboBox   polyphonyCombo_;
    juce::ComboBox   voiceModeCombo_;
    juce::ComboBox   unisonVoicesCombo_;
    juce::Slider     unisonDetuneSlider_;

    // ── Tuning section ────────────────────────────────────────────────────────
    juce::Slider     masterTuneSlider_;
    juce::ComboBox   pitchBendCombo_;
    juce::Slider     glideTimeSlider_;

    // ── MIDI section ──────────────────────────────────────────────────────────
    juce::ComboBox   midiChannelCombo_;
    juce::ToggleButton mpeModeToggle_;
    juce::ComboBox   velocityCurveCombo_;

    // ── Engine section ────────────────────────────────────────────────────────
    juce::ComboBox   maxEnginesCombo_;
    juce::Slider     crossfadeTimeSlider_;
    juce::ComboBox   oversamplingCombo_;

    // ── Display section ───────────────────────────────────────────────────────
    juce::ComboBox   uiScaleCombo_;
    juce::Slider     waveSensitivitySlider_;
    juce::ToggleButton showLabelsToggle_;
    // #1447: in-app Reduce Motion toggle — disables lerp/fade animations for
    // users with vestibular sensitivity.  Writes to A11y::inAppReducedMotion()
    // which is read by all animation sites via A11y::prefersReducedMotion().
    juce::ToggleButton reduceMotionToggle_;

    // Scrollable content
    juce::Viewport      viewport_;
    ContentComponent    contentComp_;

    // Section layout data (populated in layoutContent)
    std::vector<SectionData> sections_;

    // Total content height (set in layoutContent)
    int contentHeight_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsDrawer)
};

//==============================================================================
// Inline method definitions
//==============================================================================

inline SettingsDrawer::SettingsDrawer()
    : contentComp_(*this)
{
    buildControls();

    // Fix #1422: register with screen reader and accept keyboard focus so Tab
    // can navigate the controls inside and Escape can close the drawer.
    A11y::setup(*this, "Settings", "Plugin settings panel — polyphony, voice mode, MIDI, UI scale");

    // Viewport — vertical scroll only; no horizontal scroll bar
    viewport_.setViewedComponent(&contentComp_, false);
    viewport_.setScrollBarsShown(true, false);
    viewport_.setScrollBarThickness(4);
    viewport_.getVerticalScrollBar().setColour(
        juce::ScrollBar::thumbColourId, colTeal().withAlpha(0.35f));
    addAndMakeVisible(viewport_);

    setVisible(false);
    setSize(kDrawerWidth, 600);
}

inline SettingsDrawer::~SettingsDrawer()
{
    stopTimer();
    // Detach LookAndFeels before controls destruct (JUCE requirement)
    polyphonyCombo_.setLookAndFeel(nullptr);
    voiceModeCombo_.setLookAndFeel(nullptr);
    unisonVoicesCombo_.setLookAndFeel(nullptr);
    unisonDetuneSlider_.setLookAndFeel(nullptr);
    masterTuneSlider_.setLookAndFeel(nullptr);
    pitchBendCombo_.setLookAndFeel(nullptr);
    glideTimeSlider_.setLookAndFeel(nullptr);
    midiChannelCombo_.setLookAndFeel(nullptr);
    mpeModeToggle_.setLookAndFeel(nullptr);
    velocityCurveCombo_.setLookAndFeel(nullptr);
    maxEnginesCombo_.setLookAndFeel(nullptr);
    crossfadeTimeSlider_.setLookAndFeel(nullptr);
    oversamplingCombo_.setLookAndFeel(nullptr);
    uiScaleCombo_.setLookAndFeel(nullptr);
    waveSensitivitySlider_.setLookAndFeel(nullptr);
    showLabelsToggle_.setLookAndFeel(nullptr);
    reduceMotionToggle_.setLookAndFeel(nullptr);

    viewport_.setViewedComponent(nullptr, false);
}

//------------------------------------------------------------------------------
inline void SettingsDrawer::buildControls()
{
    // ── Helper lambdas ─────────────────────────────────────────────────────────
    auto styleCombo = [this](juce::ComboBox& c) {
        c.setLookAndFeel(&comboLnF_);
        c.setJustificationType(juce::Justification::centredLeft);
        contentComp_.addAndMakeVisible(c);
    };

    auto styleSlider = [this](juce::Slider& s,
                               double min, double max, double init,
                               const juce::String& suffix) {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 20);
        s.setRange(min, max);
        s.setValue(init, juce::dontSendNotification);
        s.setTextValueSuffix(suffix);
        // Show 0 decimal places — all settings sliders use integer-sensible units
        // (cents, ms, %). Without this, JUCE renders continuous-range values as
        // "0.00000..." which truncates to "0.00..." in the 44 px textbox.
        s.setNumDecimalPlacesToDisplay(0);
        s.setLookAndFeel(&sliderLnF_);
        s.setColour(juce::Slider::textBoxTextColourId,       colValue());
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00000000));
        s.setColour(juce::Slider::textBoxOutlineColourId,    juce::Colour(0x00000000));
        contentComp_.addAndMakeVisible(s);
    };

    auto styleToggle = [this](juce::ToggleButton& t) {
        t.setLookAndFeel(&toggleLnF_);
        t.setButtonText({});  // no text — the row label acts as the label
        contentComp_.addAndMakeVisible(t);
    };

    // ── 1. Voice ──────────────────────────────────────────────────────────────
    polyphonyCombo_.addItemList({ "4", "8", "16", "32", "64" }, 1);
    polyphonyCombo_.setSelectedItemIndex(2, juce::dontSendNotification); // 16 default
    polyphonyCombo_.onChange = [this] { fireCombo("polyphony", polyphonyCombo_); };
    styleCombo(polyphonyCombo_);

    voiceModeCombo_.addItemList({ "Poly", "Mono", "Legato", "Unison" }, 1);
    voiceModeCombo_.setSelectedItemIndex(0, juce::dontSendNotification);
    voiceModeCombo_.onChange = [this] { fireCombo("voiceMode", voiceModeCombo_); };
    styleCombo(voiceModeCombo_);

    unisonVoicesCombo_.addItemList({ "1", "2", "4", "8" }, 1);
    unisonVoicesCombo_.setSelectedItemIndex(0, juce::dontSendNotification);
    unisonVoicesCombo_.onChange = [this] { fireCombo("unisonVoices", unisonVoicesCombo_); };
    styleCombo(unisonVoicesCombo_);

    styleSlider(unisonDetuneSlider_, 0.0, 100.0, 0.0, " ct");
    unisonDetuneSlider_.onValueChange = [this] {
        fireSlider("unisonDetune", unisonDetuneSlider_);
    };

    // ── 2. Tuning ─────────────────────────────────────────────────────────────
    styleSlider(masterTuneSlider_, -100.0, 100.0, 0.0, " ct");
    masterTuneSlider_.setTextValueSuffix(" ct");
    masterTuneSlider_.onValueChange = [this] {
        fireSlider("masterTune", masterTuneSlider_);
    };

    pitchBendCombo_.addItemList({ "1", "2", "5", "7", "12", "24" }, 1);
    pitchBendCombo_.setSelectedItemIndex(3, juce::dontSendNotification); // 7 semitones
    pitchBendCombo_.onChange = [this] { fireCombo("pitchBendRange", pitchBendCombo_); };
    styleCombo(pitchBendCombo_);

    styleSlider(glideTimeSlider_, 0.0, 100.0, 0.0, " ms");
    glideTimeSlider_.onValueChange = [this] {
        fireSlider("glideTime", glideTimeSlider_);
    };

    // ── 3. MIDI ───────────────────────────────────────────────────────────────
    midiChannelCombo_.addItem("All", 1);
    for (int ch = 1; ch <= 16; ++ch)
        midiChannelCombo_.addItem(juce::String(ch), ch + 1);
    midiChannelCombo_.setSelectedItemIndex(0, juce::dontSendNotification);
    midiChannelCombo_.onChange = [this] { fireCombo("midiChannel", midiChannelCombo_); };
    styleCombo(midiChannelCombo_);

    styleToggle(mpeModeToggle_);
    mpeModeToggle_.onStateChange = [this] {
        fireToggle("mpeMode", mpeModeToggle_);
    };

    velocityCurveCombo_.addItemList({ "Soft", "Linear", "Hard", "Fixed" }, 1);
    velocityCurveCombo_.setSelectedItemIndex(1, juce::dontSendNotification);
    velocityCurveCombo_.onChange = [this] { fireCombo("velocityCurve", velocityCurveCombo_); };
    styleCombo(velocityCurveCombo_);

    // ── 4. Engine ─────────────────────────────────────────────────────────────
    maxEnginesCombo_.addItemList({ "1", "2", "3", "4" }, 1);
    maxEnginesCombo_.setSelectedItemIndex(3, juce::dontSendNotification); // 4 default
    maxEnginesCombo_.onChange = [this] { fireCombo("maxEngines", maxEnginesCombo_); };
    styleCombo(maxEnginesCombo_);

    styleSlider(crossfadeTimeSlider_, 10.0, 200.0, 50.0, " ms");
    crossfadeTimeSlider_.onValueChange = [this] {
        fireSlider("crossfadeTime", crossfadeTimeSlider_);
    };

    oversamplingCombo_.addItemList({ "Off", "2x", "4x" }, 1);
    oversamplingCombo_.setSelectedItemIndex(0, juce::dontSendNotification);
    oversamplingCombo_.onChange = [this] { fireCombo("oversampling", oversamplingCombo_); };
    styleCombo(oversamplingCombo_);

    // ── 5. Display ────────────────────────────────────────────────────────────
    uiScaleCombo_.addItemList({ "75%", "100%", "125%", "150%" }, 1);
    uiScaleCombo_.setSelectedItemIndex(1, juce::dontSendNotification); // 100% default
    uiScaleCombo_.onChange = [this] { fireCombo("uiScale", uiScaleCombo_); };
    styleCombo(uiScaleCombo_);

    styleSlider(waveSensitivitySlider_, 0.0, 100.0, 50.0, " %");
    waveSensitivitySlider_.onValueChange = [this] {
        fireSlider("waveSensitivity", waveSensitivitySlider_);
    };

    styleToggle(showLabelsToggle_);
    showLabelsToggle_.setToggleState(true, juce::dontSendNotification);
    showLabelsToggle_.onStateChange = [this] {
        fireToggle("showLabels", showLabelsToggle_);
    };

    // #1447: Reduce Motion toggle — writes directly to A11y::inAppReducedMotion().
    // All animation sites (EngineOrbit::stepAnimation, hover-fade, modal-slide, etc.)
    // check A11y::prefersReducedMotion() which reads this flag, so no further wiring
    // is needed.  onSettingChanged is also fired so the processor/APVTS can persist it.
    styleToggle(reduceMotionToggle_);
    reduceMotionToggle_.setToggleState(false, juce::dontSendNotification);
    reduceMotionToggle_.onStateChange = [this] {
        const bool enabled = reduceMotionToggle_.getToggleState();
        A11y::setReducedMotion(enabled);
        fireToggle("reduceMotion", reduceMotionToggle_);
    };

    wireTooltips();  // V1 Lane B: wire all setting control tooltips
}

//------------------------------------------------------------------------------
inline void SettingsDrawer::wireTooltips()
{
    // Voice section
    polyphonyCombo_.setTooltip("Select maximum simultaneous voices or reduce to save CPU");
    voiceModeCombo_.setTooltip("Select voice allocation mode for note stacking or mono glide");
    unisonVoicesCombo_.setTooltip("Select stacked voice count for unison width or single-voice clarity");
    unisonDetuneSlider_.setTooltip("Drag to spread unison voices apart in pitch for chorus width");
    // Tuning section
    masterTuneSlider_.setTooltip("Drag to shift global pitch up or down in cents for instrument tuning");
    pitchBendCombo_.setTooltip("Select pitch bend wheel range in semitones for expressive control");
    glideTimeSlider_.setTooltip("Drag to set portamento slide time between notes in milliseconds");
    // MIDI section
    midiChannelCombo_.setTooltip("Select MIDI channel to respond to or All to accept any channel");
    mpeModeToggle_.setTooltip("Toggle MPE mode for per-note pitch, pressure, and slide expression");
    velocityCurveCombo_.setTooltip("Select velocity response curve to match your keyboard touch sensitivity");
    // Engine section
    maxEnginesCombo_.setTooltip("Select active engine slot count to reduce CPU on lower-spec systems");
    crossfadeTimeSlider_.setTooltip("Drag to set engine hot-swap crossfade duration to prevent clicks");
    oversamplingCombo_.setTooltip("Select internal oversampling rate for higher alias rejection at CPU cost");
    // Display section
    uiScaleCombo_.setTooltip("Select UI zoom level to scale the interface for your display or vision");
    waveSensitivitySlider_.setTooltip("Drag to set how strongly the ocean surface reacts to audio input");
    showLabelsToggle_.setTooltip("Toggle parameter labels on engine controls for cleaner or annotated view");
    reduceMotionToggle_.setTooltip("Disable lerp and fade animations for users with vestibular sensitivity");
}

//------------------------------------------------------------------------------
inline void SettingsDrawer::fireCombo(const juce::String& key, juce::ComboBox& combo)
{
    if (onSettingChanged)
        onSettingChanged(key, static_cast<float>(combo.getSelectedItemIndex()));
}

inline void SettingsDrawer::fireSlider(const juce::String& key, juce::Slider& slider)
{
    if (onSettingChanged)
        onSettingChanged(key, static_cast<float>(slider.getValue()));
}

inline void SettingsDrawer::fireToggle(const juce::String& key, juce::ToggleButton& btn)
{
    if (onSettingChanged)
        onSettingChanged(key, btn.getToggleState() ? 1.0f : 0.0f);
}

// Fix #1419: restore saved settings into controls, then fire each callback so
// the processor/APVTS matches the restored UI state on plugin reload.
inline void SettingsDrawer::applySettings(juce::PropertiesFile& props)
{
    // Helper: restore a ComboBox index, then fire its onChange to sync processor.
    auto restoreCombo = [&](const juce::String& key, juce::ComboBox& combo, int defaultIdx)
    {
        const int idx = props.getIntValue("drawer_" + key, defaultIdx);
        combo.setSelectedItemIndex(juce::jlimit(0, combo.getNumItems() - 1, idx),
                                   juce::dontSendNotification);
        if (onSettingChanged)
            onSettingChanged(key, static_cast<float>(combo.getSelectedItemIndex()));
    };

    auto restoreSlider = [&](const juce::String& key, juce::Slider& slider, double defaultVal)
    {
        const double val = props.getDoubleValue("drawer_" + key, defaultVal);
        slider.setValue(juce::jlimit(slider.getMinimum(), slider.getMaximum(), val),
                        juce::dontSendNotification);
        if (onSettingChanged)
            onSettingChanged(key, static_cast<float>(slider.getValue()));
    };

    auto restoreToggle = [&](const juce::String& key, juce::ToggleButton& btn, bool defaultVal)
    {
        const bool on = props.getBoolValue("drawer_" + key, defaultVal);
        btn.setToggleState(on, juce::dontSendNotification);
        if (onSettingChanged)
            onSettingChanged(key, on ? 1.0f : 0.0f);
    };

    // F2-016: Restore drawer open state.
    if (props.getBoolValue("drawer_isOpen", false))
        open();

    restoreCombo  ("polyphony",        polyphonyCombo_,        2);
    restoreCombo  ("voiceMode",        voiceModeCombo_,        0);
    restoreCombo  ("unisonVoices",     unisonVoicesCombo_,     0);
    restoreSlider ("unisonDetune",     unisonDetuneSlider_,    0.0);
    restoreSlider ("masterTune",       masterTuneSlider_,      0.0);
    restoreCombo  ("pitchBendRange",   pitchBendCombo_,        3);
    restoreSlider ("glideTime",        glideTimeSlider_,       0.0);
    restoreCombo  ("midiChannel",      midiChannelCombo_,      0);
    restoreToggle ("mpeMode",          mpeModeToggle_,         false);
    restoreCombo  ("velocityCurve",    velocityCurveCombo_,    1);
    restoreCombo  ("maxEngines",       maxEnginesCombo_,       3);
    restoreSlider ("crossfadeTime",    crossfadeTimeSlider_,  50.0);
    restoreCombo  ("oversampling",     oversamplingCombo_,     0);
    restoreCombo  ("uiScale",          uiScaleCombo_,          1);
    restoreSlider ("waveSensitivity",  waveSensitivitySlider_, 50.0);
    restoreToggle ("showLabels",       showLabelsToggle_,      true);

    // #1447: restore Reduce Motion and apply immediately so animation state
    // is correct before the first paint (don't wait for user to toggle it again).
    {
        const bool rm = props.getBoolValue("drawer_reduceMotion", false);
        reduceMotionToggle_.setToggleState(rm, juce::dontSendNotification);
        A11y::setReducedMotion(rm);
        if (onSettingChanged)
            onSettingChanged("reduceMotion", rm ? 1.0f : 0.0f);
    }
}

// Fix #1419: persist current control values so they survive plugin reload.
inline void SettingsDrawer::saveSettings(juce::PropertiesFile& props) const
{
    // F2-016: Persist drawer open state so it survives plugin reload.
    props.setValue("drawer_isOpen",          isOpen());
    props.setValue("drawer_polyphony",       polyphonyCombo_.getSelectedItemIndex());
    props.setValue("drawer_voiceMode",       voiceModeCombo_.getSelectedItemIndex());
    props.setValue("drawer_unisonVoices",    unisonVoicesCombo_.getSelectedItemIndex());
    props.setValue("drawer_unisonDetune",    unisonDetuneSlider_.getValue());
    props.setValue("drawer_masterTune",      masterTuneSlider_.getValue());
    props.setValue("drawer_pitchBendRange",  pitchBendCombo_.getSelectedItemIndex());
    props.setValue("drawer_glideTime",       glideTimeSlider_.getValue());
    props.setValue("drawer_midiChannel",     midiChannelCombo_.getSelectedItemIndex());
    props.setValue("drawer_mpeMode",         mpeModeToggle_.getToggleState());
    props.setValue("drawer_velocityCurve",   velocityCurveCombo_.getSelectedItemIndex());
    props.setValue("drawer_maxEngines",      maxEnginesCombo_.getSelectedItemIndex());
    props.setValue("drawer_crossfadeTime",   crossfadeTimeSlider_.getValue());
    props.setValue("drawer_oversampling",    oversamplingCombo_.getSelectedItemIndex());
    props.setValue("drawer_uiScale",         uiScaleCombo_.getSelectedItemIndex());
    props.setValue("drawer_waveSensitivity", waveSensitivitySlider_.getValue());
    props.setValue("drawer_showLabels",      showLabelsToggle_.getToggleState());
    props.setValue("drawer_reduceMotion",    reduceMotionToggle_.getToggleState());
    props.saveIfNeeded();
}

//------------------------------------------------------------------------------
inline void SettingsDrawer::open()
{
    if (animState_ == AnimState::Open || animState_ == AnimState::Opening)
        return;

    setVisible(true);
    toFront(false);
    if (animState_ != AnimState::Closing)
        animProgress_ = 0.0f;
    animState_   = AnimState::Opening;
    animStartMs_ = juce::Time::currentTimeMillis()
                   - (int64_t)(animProgress_ * kAnimDurationMs);
    startTimer(kTimerIntervalMs);
}

inline void SettingsDrawer::close()
{
    if (animState_ == AnimState::Closed || animState_ == AnimState::Closing)
        return;

    if (animState_ != AnimState::Opening)
        animProgress_ = 1.0f;
    animState_   = AnimState::Closing;
    animStartMs_ = juce::Time::currentTimeMillis()
                   - (int64_t)((1.0f - animProgress_) * kAnimDurationMs);
    startTimer(kTimerIntervalMs);
}

//------------------------------------------------------------------------------
inline void SettingsDrawer::timerCallback()
{
    const int64_t now     = juce::Time::currentTimeMillis();
    const float   elapsed = (float)(now - animStartMs_);
    const float   t       = juce::jlimit(0.0f, 1.0f, elapsed / kAnimDurationMs);

    if (animState_ == AnimState::Opening)
    {
        animProgress_ = t;
        applyAnimPosition();

        if (t >= 1.0f)
        {
            animState_    = AnimState::Open;
            animProgress_ = 1.0f;
            stopTimer();
        }
    }
    else if (animState_ == AnimState::Closing)
    {
        animProgress_ = 1.0f - t;
        applyAnimPosition();

        if (t >= 1.0f)
        {
            animState_    = AnimState::Closed;
            animProgress_ = 0.0f;
            stopTimer();
            setVisible(false);
        }
    }
}

//------------------------------------------------------------------------------
inline void SettingsDrawer::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds();

    // ── Main background ───────────────────────────────────────────────────────
    g.setColour(colDrawerBg());
    g.fillRect(bounds);

    // ── Left-edge shadow/border (visible when sliding from right) ─────────────
    g.setColour(colTeal().withAlpha(0.10f));
    g.drawVerticalLine(0, 0.0f, (float)bounds.getHeight());
    g.setColour(colBorder());
    g.drawVerticalLine(1, 0.0f, (float)bounds.getHeight());

    // ── Title bar ─────────────────────────────────────────────────────────────
    {
        const auto titleR = bounds.withHeight(kHeaderH).toFloat();

        juce::ColourGradient titleGrad(colTeal().withAlpha(0.08f),
                                       titleR.getWidth(), 0.0f,
                                       juce::Colour(0x00000000), 0.0f, 0.0f, false);
        g.setGradientFill(titleGrad);
        g.fillRect(titleR);

        g.setFont(GalleryFonts::display(15.0f));
        g.setColour(colTeal());
        g.drawText("Settings",
                   bounds.withHeight(kHeaderH).withTrimmedLeft(48).withTrimmedRight(16),
                   juce::Justification::centredLeft, false);

        // Close button (×) on the LEFT side for a right drawer
        g.setFont(GalleryFonts::body(17.0f));
        g.setColour(closeBtnHovered_ ? juce::Colour(0xFFFFFFFF) : colCloseBtn());
        g.drawText(juce::String::fromUTF8("\xC3\x97"),
                   closeBtnBounds_, juce::Justification::centred, false);

        g.setColour(colBorder());
        g.drawHorizontalLine(kHeaderH - 1, 0.0f, (float)bounds.getWidth());
    }

    // ── Bottom fade gradient ──────────────────────────────────────────────────
    {
        const int fadeH = 40;
        const int fadeY = bounds.getHeight() - fadeH;
        juce::ColourGradient fadeGrad(juce::Colour(0x00000000),       0.0f, (float)fadeY,
                                      colDrawerBg().withAlpha(0.94f), 0.0f, (float)bounds.getHeight(),
                                      false);
        g.setGradientFill(fadeGrad);
        g.fillRect(0, fadeY, bounds.getWidth(), fadeH);
    }
}

//------------------------------------------------------------------------------
inline void SettingsDrawer::resized()
{
    auto b = getLocalBounds();

    // ── Title bar ─────────────────────────────────────────────────────────────
    auto titleBar = b.removeFromTop(kHeaderH);
    // Close button: 36×36, on the LEFT side (x=8) for right-side drawer
    closeBtnBounds_ = titleBar.removeFromLeft(44).withSizeKeepingCentre(36, 36);

    // ── Scrollable content ────────────────────────────────────────────────────
    viewport_.setBounds(b);
    layoutContent(b.getWidth());
}

//------------------------------------------------------------------------------
inline void SettingsDrawer::layoutContent(int contentWidth)
{
    // Build the section/row layout data and position all controls.
    // This is called from ContentComponent::resized() and from resized().
    // No heap allocation — just arithmetic and setBounds calls.

    const int colLabelW   = 108;   // label column width
    const int colControlX = colLabelW + 8;
    const int colControlW = contentWidth - colControlX - 12;
    const int sliderH     = 24;
    const int comboH      = 26;
    const int toggleH     = 26;

    sections_.clear();

    int y = kScrollMarginH;

    // Helper: add a section header, return updated y after the header
    auto addSection = [&](const char* title) -> int
    {
        y += kSectionGap;
        SectionData sec;
        sec.title   = title;
        sec.headerY = y;
        sections_.push_back(std::move(sec));
        y += kSectionHeaderH;
        return (int)sections_.size() - 1; // index into sections_
    };

    // Helper: add a combo row
    auto addComboRow = [&](int secIdx, const char* label, juce::ComboBox& combo)
    {
        const int rowY = y;
        RowData row;
        row.label   = label;
        row.labelY  = rowY + (kRowH - comboH) / 2;
        row.control = &combo;
        sections_[(size_t)secIdx].rows.push_back(std::move(row));
        combo.setBounds(colControlX, rowY + (kRowH - comboH) / 2, colControlW, comboH);
        y += kRowH;
    };

    // Helper: add a slider row
    auto addSliderRow = [&](int secIdx, const char* label, juce::Slider& slider)
    {
        const int rowY = y;
        RowData row;
        row.label   = label;
        row.labelY  = rowY + (kRowH - sliderH) / 2;
        row.control = &slider;
        sections_[(size_t)secIdx].rows.push_back(std::move(row));
        slider.setBounds(colControlX, rowY + (kRowH - sliderH) / 2, colControlW, sliderH);
        y += kRowH;
    };

    // Helper: add a toggle row
    auto addToggleRow = [&](int secIdx, const char* label, juce::ToggleButton& btn)
    {
        const int rowY = y;
        RowData row;
        row.label   = label;
        row.labelY  = rowY + (kRowH - toggleH) / 2;
        row.control = &btn;
        sections_[(size_t)secIdx].rows.push_back(std::move(row));
        // Toggle occupies the right 36px of the control column (pill + track)
        btn.setBounds(colControlX, rowY + (kRowH - toggleH) / 2, colControlW, toggleH);
        y += kRowH;
    };

    // ── 1. Voice ──────────────────────────────────────────────────────────────
    {
        int s = addSection("VOICE");
        addComboRow (s, "Polyphony",      polyphonyCombo_);
        addComboRow (s, "Voice Mode",     voiceModeCombo_);
        addComboRow (s, "Unison Voices",  unisonVoicesCombo_);
        addSliderRow(s, "Unison Detune",  unisonDetuneSlider_);
    }

    // ── 2. Tuning ─────────────────────────────────────────────────────────────
    {
        int s = addSection("TUNING");
        addSliderRow(s, "Master Tune",    masterTuneSlider_);
        addComboRow (s, "Pitch Bend",     pitchBendCombo_);
        addSliderRow(s, "Glide Time",     glideTimeSlider_);
    }

    // ── 3. MIDI ───────────────────────────────────────────────────────────────
    {
        int s = addSection("MIDI");
        addComboRow (s, "MIDI Channel",   midiChannelCombo_);
        addToggleRow(s, "MPE Mode",       mpeModeToggle_);
        addComboRow (s, "Velocity Curve", velocityCurveCombo_);
    }

    // ── 4. Engine ─────────────────────────────────────────────────────────────
    {
        int s = addSection("ENGINE");
        addComboRow (s, "Max Engines",    maxEnginesCombo_);
        addSliderRow(s, "Crossfade",      crossfadeTimeSlider_);
        addComboRow (s, "Oversampling",   oversamplingCombo_);
    }

    // ── 5. Display ────────────────────────────────────────────────────────────
    {
        int s = addSection("DISPLAY");
        addComboRow (s, "UI Scale",       uiScaleCombo_);
        addSliderRow(s, "Wave Sens.",     waveSensitivitySlider_);
        addToggleRow(s, "Show Labels",    showLabelsToggle_);
        addToggleRow(s, "Reduce Motion",  reduceMotionToggle_);
    }

    y += kScrollMarginH;
    contentHeight_ = y;
    contentComp_.setSize(contentWidth, contentHeight_);
    contentComp_.repaint();
}

//------------------------------------------------------------------------------
inline void SettingsDrawer::mouseMove(const juce::MouseEvent& e)
{
    const bool nowHovered = closeBtnBounds_.contains(e.getPosition());
    if (nowHovered != closeBtnHovered_)
    {
        closeBtnHovered_ = nowHovered;
        repaint(closeBtnBounds_);
    }
}

inline void SettingsDrawer::mouseDown(const juce::MouseEvent& e)
{
    if (closeBtnBounds_.contains(e.getPosition()))
        close();
}

inline void SettingsDrawer::mouseExit(const juce::MouseEvent& /*e*/)
{
    if (closeBtnHovered_)
    {
        closeBtnHovered_ = false;
        repaint(closeBtnBounds_);
    }
}

// Fix #1422: allow Escape to close the drawer when it has keyboard focus.
inline bool SettingsDrawer::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey && isOpen())
    {
        close();
        return true; // consumed
    }
    return false;
}

//------------------------------------------------------------------------------
inline void SettingsDrawer::applyAnimPosition()
{
    if (getParentComponent() == nullptr)
        return;

    const int parentWidth = getParentComponent()->getWidth();
    const float t = juce::jlimit(0.0f, 1.0f, animProgress_);

    // Ease-out on open, linear on close
    const float easedT = (animState_ == AnimState::Opening || animState_ == AnimState::Open)
                             ? std::pow(t, 0.4f)
                             : t;

    // Fully open  = parentWidth - kDrawerWidth (flush with right edge)
    // Fully closed = parentWidth (completely off-screen right)
    const int openX   = parentWidth - kDrawerWidth;
    const int hiddenX = parentWidth;
    const int newX    = juce::roundToInt((float)hiddenX + easedT * (float)(openX - hiddenX));
    setTopLeftPosition(newX, getY());
}

} // namespace xoceanus
