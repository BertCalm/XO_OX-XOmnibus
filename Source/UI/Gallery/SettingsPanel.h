#pragma once
// SettingsPanel.h — Column C / C6 tab: plugin-wide settings.
//
// Layout (320pt wide × variable height, scrollable via juce::Viewport):
//   1. Theme          (48pt)   — Dark/Light mode toggle
//   2. Accessibility  (72pt)   — Reduced Motion, future High Contrast
//   3. MPE            (160pt)  — All five MPE APVTS params (gracefully skipped if absent)
//   4. Performance    (48pt)   — Performance Lock (editor-local state, no APVTS)
//   5. MIDI Mappings  (variable) — Live CC→param table from MIDILearnManager
//   6. About          (80pt)   — Version, links
//
// Architectural notes:
//   • Header-only (.h), consistent with all Gallery Model components.
//   • GalleryColors / GalleryFonts / A11y namespaces used throughout.
//   • APVTS param guards: getParameter(id) checked before creating any attachment.
//   • MIDILearnManager pointer is optional — call setMidiLearnManager() from the editor.
//   • Performance Lock state exposed via isPerformanceLocked() + onPerformanceLockChanged.
//   • Section headers: Space Grotesk Bold 10pt ALL CAPS + 1px borderGray separator.

#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include "GalleryLookAndFeel.h"

// Forward-declare processor so we can take a reference.
// The full definition is in XOlokunProcessor.h, which is included by the editor
// before SidebarPanel.h and thus before this header.
namespace xolokun { class XOlokunProcessor; }

#include "../../XOlokunProcessor.h"

namespace xolokun {

//==============================================================================
class SettingsPanel : public juce::Component
{
public:
    //==========================================================================
    explicit SettingsPanel(XOlokunProcessor& proc)
        : processor(proc)
    {
        // ── Viewport + inner content component ───────────────────────────────
        content.owner = this;          // back-pointer for paint callbacks
        viewport.setViewedComponent(&content, false);
        viewport.setScrollBarsShown(true, false);
        viewport.getVerticalScrollBar().setColour(
            juce::ScrollBar::thumbColourId,
            GalleryColors::get(GalleryColors::borderGray()));
        addAndMakeVisible(viewport);

        // ── 1. THEME ─────────────────────────────────────────────────────────
        styleToggle(darkModeToggle, "Dark Mode");
        darkModeToggle.setToggleState(GalleryColors::darkMode(),
                                      juce::dontSendNotification);
        darkModeToggle.onClick = [this]
        {
            GalleryColors::darkMode() = darkModeToggle.getToggleState();
            // Save preference
            settingsFile->setValue("darkMode", GalleryColors::darkMode());
            settingsFile->saveIfNeeded();
            // Re-apply theme to the entire component tree
            if (auto* top = getTopLevelComponent())
            {
                if (auto* laf = dynamic_cast<GalleryLookAndFeel*>(&top->getLookAndFeel()))
                    laf->applyTheme();
                top->repaint();
            }
        };
        content.addAndMakeVisible(darkModeToggle);

        // ── 2. ACCESSIBILITY ─────────────────────────────────────────────────
        styleToggle(reducedMotionToggle, "Reduced Motion (WCAG 2.3.3)");
        // Read stored value (falls back to false if key absent or private browsing).
        {
            juce::PropertiesFile::Options opts;
            opts.applicationName     = "XOlokun";
            opts.filenameSuffix      = "settings";
            opts.osxLibrarySubFolder = "Application Support";
            settingsFile = std::make_unique<juce::PropertiesFile>(opts);
        }
        // Restore persisted dark mode preference (default true if not yet saved).
        GalleryColors::darkMode() = settingsFile->getBoolValue("darkMode", true);
        darkModeToggle.setToggleState(GalleryColors::darkMode(), juce::dontSendNotification);

        reducedMotionToggle.setToggleState(
            settingsFile->getBoolValue("reducedMotion", false),
            juce::dontSendNotification);
        reducedMotionToggle.onClick = [this]
        {
            bool v = reducedMotionToggle.getToggleState();
            settingsFile->setValue("reducedMotion", v);
            settingsFile->saveIfNeeded();
        };
        content.addAndMakeVisible(reducedMotionToggle);

        // High-Contrast placeholder label (future)
        highContrastNote.setText("High Contrast — coming soon",
                                 juce::dontSendNotification);
        highContrastNote.setFont(GalleryFonts::body(10.0f));
        highContrastNote.setColour(juce::Label::textColourId,
                                   GalleryColors::get(GalleryColors::textMid()).withAlpha(0.55f));
        content.addAndMakeVisible(highContrastNote);

        // ── 3. MPE ───────────────────────────────────────────────────────────
        auto& apvts = processor.getAPVTS();

        // mpe_enabled — AudioParameterBool → ButtonAttachment
        if (apvts.getParameter("mpe_enabled") != nullptr)
        {
            mpeEnabledToggle = std::make_unique<juce::ToggleButton>();
            styleToggle(*mpeEnabledToggle, "MPE Enabled");
            mpeEnabledAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, "mpe_enabled", *mpeEnabledToggle);
            content.addAndMakeVisible(*mpeEnabledToggle);
            // mpeEnabledLabel — text embedded in ToggleButton; no separate label needed
        }

        // mpe_zone — AudioParameterChoice → ComboBoxAttachment
        if (apvts.getParameter("mpe_zone") != nullptr)
        {
            mpeZoneBox = std::make_unique<juce::ComboBox>();
            mpeZoneBox->addItem("Off",   1);
            mpeZoneBox->addItem("Lower", 2);
            mpeZoneBox->addItem("Upper", 3);
            mpeZoneBox->addItem("Both",  4);
            styleComboBox(*mpeZoneBox);
            mpeZoneAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, "mpe_zone", *mpeZoneBox);
            content.addAndMakeVisible(*mpeZoneLabel);
            content.addAndMakeVisible(*mpeZoneBox);
        }

        // mpe_pitchBendRange — AudioParameterFloat → SliderAttachment
        if (apvts.getParameter("mpe_pitchBendRange") != nullptr)
        {
            mpePBSlider = std::make_unique<juce::Slider>(
                juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
            mpePBSlider->setRange(1.0, 96.0, 1.0);
            mpePBSlider->setTextValueSuffix(" st");
            mpePBSlider->setColour(juce::Slider::textBoxTextColourId,
                                   GalleryColors::get(GalleryColors::textMid()));
            mpePBSlider->setColour(juce::Slider::textBoxBackgroundColourId,
                                   juce::Colours::transparentBlack);
            mpePBSlider->setColour(juce::Slider::textBoxOutlineColourId,
                                   juce::Colours::transparentBlack);
            mpePBSlider->setColour(juce::Slider::trackColourId,
                                   GalleryColors::get(GalleryColors::xoGold));
            mpePBAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, "mpe_pitchBendRange", *mpePBSlider);
            content.addAndMakeVisible(*mpePBLabel);
            content.addAndMakeVisible(*mpePBSlider);
        }

        // mpe_pressureTarget — AudioParameterChoice → ComboBoxAttachment
        if (apvts.getParameter("mpe_pressureTarget") != nullptr)
        {
            mpePressureBox = std::make_unique<juce::ComboBox>();
            mpePressureBox->addItem("Filter Cutoff",           1);
            mpePressureBox->addItem("Volume",                  2);
            mpePressureBox->addItem("Wavetable",               3);
            mpePressureBox->addItem("FX Send",                 4);
            mpePressureBox->addItem("Macro 1 (CHARACTER)",     5);
            mpePressureBox->addItem("Macro 2 (MOVEMENT)",      6);
            styleComboBox(*mpePressureBox);
            mpePressureAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, "mpe_pressureTarget", *mpePressureBox);
            content.addAndMakeVisible(*mpePressureLabel);
            content.addAndMakeVisible(*mpePressureBox);
        }

        // mpe_slideTarget — AudioParameterChoice → ComboBoxAttachment
        if (apvts.getParameter("mpe_slideTarget") != nullptr)
        {
            mpeSlideBox = std::make_unique<juce::ComboBox>();
            mpeSlideBox->addItem("Filter Cutoff",           1);
            mpeSlideBox->addItem("Volume",                  2);
            mpeSlideBox->addItem("Wavetable",               3);
            mpeSlideBox->addItem("FX Send",                 4);
            mpeSlideBox->addItem("Macro 1 (CHARACTER)",     5);
            mpeSlideBox->addItem("Macro 2 (MOVEMENT)",      6);
            styleComboBox(*mpeSlideBox);
            mpeSlideAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, "mpe_slideTarget", *mpeSlideBox);
            content.addAndMakeVisible(*mpeSlideLabel);
            content.addAndMakeVisible(*mpeSlideBox);
        }

        // If NO MPE params existed at all, show a placeholder note.
        bool anyMpeParam = (mpeEnabledToggle != nullptr);
        if (!anyMpeParam)
        {
            mpeNotConfigured.setText("Not yet configured",
                                     juce::dontSendNotification);
            mpeNotConfigured.setFont(GalleryFonts::body(10.0f));
            mpeNotConfigured.setColour(juce::Label::textColourId,
                                       GalleryColors::get(GalleryColors::textMid()).withAlpha(0.55f));
            content.addAndMakeVisible(mpeNotConfigured);
        }

        // ── 4. PERFORMANCE LOCK ───────────────────────────────────────────────
        styleToggle(perfLockToggle, "Performance Lock");
        perfLockToggle.setToggleState(perfLocked, juce::dontSendNotification);
        perfLockToggle.onClick = [this]
        {
            perfLocked = perfLockToggle.getToggleState();
            if (onPerformanceLockChanged)
                onPerformanceLockChanged(perfLocked);
        };
        content.addAndMakeVisible(perfLockToggle);

        perfLockNote.setText("Blocks param changes during performance",
                             juce::dontSendNotification);
        perfLockNote.setFont(GalleryFonts::body(9.5f));
        perfLockNote.setColour(juce::Label::textColourId,
                               GalleryColors::get(GalleryColors::textMid()).withAlpha(0.65f));
        content.addAndMakeVisible(perfLockNote);

        // ── 5. MIDI MAPPINGS placeholder ─────────────────────────────────────
        // The real table is painted in paintMidiTable().
        // "Clear All" button — wired once setMidiLearnManager() is called.
        clearAllBtn.setButtonText("Clear All");
        clearAllBtn.setColour(juce::TextButton::buttonColourId,
                              GalleryColors::get(GalleryColors::shellWhite()));
        clearAllBtn.setColour(juce::TextButton::textColourOffId,
                              GalleryColors::get(GalleryColors::textMid()));
        clearAllBtn.onClick = [this]
        {
            if (midiLearnMgr != nullptr)
            {
                midiLearnMgr->clearAllMappings();
                repaint();
            }
        };
        content.addAndMakeVisible(clearAllBtn);

        // ── 6. ABOUT ─────────────────────────────────────────────────────────
        aboutNameLabel.setText("XOlokun", juce::dontSendNotification);
        aboutNameLabel.setFont(GalleryFonts::display(16.0f));
        aboutNameLabel.setColour(juce::Label::textColourId,
                                 GalleryColors::get(GalleryColors::textDark()));
        aboutNameLabel.setJustificationType(juce::Justification::centredLeft);
        content.addAndMakeVisible(aboutNameLabel);

        // Version — use JucePlugin_VersionString if available, otherwise hardcode.
        {
            juce::String ver;
#if defined(JucePlugin_VersionString)
            ver = JucePlugin_VersionString;
#else
            ver = "v0.5.0-dev";
#endif
            aboutVersionLabel.setText(ver, juce::dontSendNotification);
        }
        aboutVersionLabel.setFont(GalleryFonts::value(10.0f));
        aboutVersionLabel.setColour(juce::Label::textColourId,
                                    GalleryColors::get(GalleryColors::textMid()));
        aboutVersionLabel.setJustificationType(juce::Justification::centredLeft);
        content.addAndMakeVisible(aboutVersionLabel);

        aboutMakerLabel.setText("XO_OX Designs", juce::dontSendNotification);
        aboutMakerLabel.setFont(GalleryFonts::body(10.0f));
        aboutMakerLabel.setColour(juce::Label::textColourId,
                                  GalleryColors::get(GalleryColors::textMid()));
        aboutMakerLabel.setJustificationType(juce::Justification::centredLeft);
        content.addAndMakeVisible(aboutMakerLabel);

        // Clickable URL labels
        styleUrlLabel(aboutWebLabel,  "xo-ox.org",          "https://xo-ox.org");
        styleUrlLabel(aboutPatreonLabel, "patreon.com/c/XO_OX", "https://www.patreon.com/c/XO_OX");
        content.addAndMakeVisible(aboutWebLabel);
        content.addAndMakeVisible(aboutPatreonLabel);

        // ── Accessibility wiring ──────────────────────────────────────────────
        A11y::setup(*this, "Settings", "Plugin settings panel");
    }

    ~SettingsPanel() override = default;

    //==========================================================================
    // Wire the optional MIDILearnManager for the MIDI Mappings table.
    // Call from the editor (SidebarPanel::setMidiLearnManager → forwarded here).
    void setMidiLearnManager(MIDILearnManager* mgr)
    {
        midiLearnMgr = mgr;
        repaint();
    }

    //==========================================================================
    bool isPerformanceLocked() const noexcept { return perfLocked; }

    /// Fired on the message thread whenever the Performance Lock toggle changes.
    std::function<void(bool)> onPerformanceLockChanged;

    //==========================================================================
    void resized() override
    {
        viewport.setBounds(getLocalBounds());
        layoutContent();
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));
    }

    //==========================================================================
    // lookAndFeelChanged — refresh colour-dependent labels when theme switches.
    void lookAndFeelChanged() override
    {
        darkModeToggle.setToggleState(GalleryColors::darkMode(),
                                      juce::dontSendNotification);
        repaint();
    }

private:
    //==========================================================================
    // Layout constants
    static constexpr int kPad        = 12;   // outer horizontal padding
    static constexpr int kRowH       = 24;   // standard control row height
    static constexpr int kHeaderH    = 18;   // section header height
    static constexpr int kGap        = 6;    // gap between rows
    static constexpr int kSectionGap = 12;   // gap between sections

    //==========================================================================
    // Helpers

    // Style a ToggleButton and its companion label uniformly.
    void styleToggle(juce::ToggleButton& btn, const juce::String& text)
    {
        btn.setButtonText(text);
        btn.setColour(juce::ToggleButton::textColourId,
                      GalleryColors::get(GalleryColors::textDark()));
        btn.setColour(juce::ToggleButton::tickColourId,
                      GalleryColors::get(GalleryColors::xoGold));
        btn.setColour(juce::ToggleButton::tickDisabledColourId,
                      GalleryColors::get(GalleryColors::borderGray()));
        btn.setWantsKeyboardFocus(true);
    }

    void styleComboBox(juce::ComboBox& cb)
    {
        cb.setColour(juce::ComboBox::backgroundColourId,
                     GalleryColors::get(GalleryColors::slotBg()));
        cb.setColour(juce::ComboBox::outlineColourId,
                     GalleryColors::get(GalleryColors::borderGray()));
        cb.setColour(juce::ComboBox::textColourId,
                     GalleryColors::get(GalleryColors::textDark()));
        cb.setColour(juce::ComboBox::arrowColourId,
                     GalleryColors::get(GalleryColors::textMid()));
        cb.setTextWhenNothingSelected({});
    }

    void styleUrlLabel(juce::Label& lbl, const juce::String& text, const juce::String& url)
    {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setFont(GalleryFonts::body(10.0f));
        lbl.setColour(juce::Label::textColourId,
                      juce::Colour(GalleryColors::darkMode() ? 0xFF58A6FF : 0xFF0066CC));
        lbl.setInterceptsMouseClicks(true, false);

        // Open the URL on click via a mouseListener lambda approach.
        // We store a raw string in the label's ComponentID and handle it in mouseUp.
        lbl.setComponentID(url);
        lbl.addMouseListener(this, false);
    }

    // Handle URL label clicks
    void mouseUp(const juce::MouseEvent& e) override
    {
        if (auto* lbl = dynamic_cast<juce::Label*>(e.eventComponent))
        {
            auto url = lbl->getComponentID();
            if (url.startsWith("http"))
                juce::URL(url).launchInDefaultBrowser();
        }
    }

    //==========================================================================
    // Draw a section header (ALL CAPS Space Grotesk Bold 10pt + separator line).
    // Called by ContentWithHeaders::paint() via the back-pointer to this class.

    void drawSectionHeader(juce::Graphics& g, const juce::String& title,
                           int x, int y, int w) const
    {
        using namespace GalleryColors;
        // Separator line
        g.setColour(get(borderGray()));
        g.drawHorizontalLine(y + kHeaderH - 2, static_cast<float>(x), static_cast<float>(x + w));

        // Label
        g.setFont(GalleryFonts::display(10.0f));
        g.setColour(get(textMid()));
        g.drawText(title.toUpperCase(), x, y, w, kHeaderH - 4,
                   juce::Justification::bottomLeft, false);
    }

    //==========================================================================
    // paintMidiTable — renders MIDI CC→param rows within the given bounds.
    // Returns the height actually consumed (for dynamic layout).
    int paintMidiTable(juce::Graphics& g, int x, int y, int w) const
    {
        using namespace GalleryColors;

        if (midiLearnMgr == nullptr)
        {
            g.setFont(GalleryFonts::body(10.0f));
            g.setColour(get(textMid()).withAlpha(0.65f));
            g.drawText("MIDI Learn: right-click any knob", x, y, w, kRowH,
                       juce::Justification::centredLeft);
            return kRowH;
        }

        auto mappings = midiLearnMgr->getMappings();
        if (mappings.empty())
        {
            g.setFont(GalleryFonts::body(10.0f));
            g.setColour(get(textMid()).withAlpha(0.55f));
            g.drawText("No MIDI mappings active", x, y, w, kRowH,
                       juce::Justification::centredLeft);
            return kRowH;
        }

        int rowY = y;
        for (const auto& m : mappings)
        {
            // Alternate row tint
            bool odd = ((&m - &mappings[0]) % 2 == 1);
            if (odd)
            {
                g.setColour(get(slotBg()));
                g.fillRect(x, rowY, w, kRowH);
            }

            // CC number — JetBrains Mono 10pt
            g.setFont(GalleryFonts::value(10.0f));
            g.setColour(get(textDark()));
            g.drawText("CC " + juce::String(m.ccNumber),
                       x + 4, rowY, 46, kRowH,
                       juce::Justification::centredLeft, false);

            // Param ID — Inter 10pt, truncated
            g.setFont(GalleryFonts::body(10.0f));
            g.setColour(get(textMid()));
            g.drawText(m.paramId,
                       x + 54, rowY, w - 54 - 28, kRowH,
                       juce::Justification::centredLeft, true);

            rowY += kRowH;
        }
        return rowY - y;
    }

    //==========================================================================
    // layoutContent — positions all sub-components and sets the inner content
    // component's total height so the viewport can scroll correctly.
    void layoutContent()
    {
        const int w    = getWidth();
        const int inner = w - kPad * 2;

        int y = kGap;

        auto placeToggleRow = [&](juce::ToggleButton& btn, int rowH = kRowH) -> int
        {
            btn.setBounds(kPad, y, inner, rowH);
            y += rowH + kGap;
            return y;
        };

        auto placeComboRow = [&](juce::Label& lbl, juce::ComboBox& cb) -> int
        {
            lbl.setBounds(kPad, y, inner / 2, kRowH);
            cb.setBounds(kPad + inner / 2, y, inner / 2, kRowH);
            y += kRowH + kGap;
            return y;
        };

        auto placeSliderRow = [&](juce::Label& lbl, juce::Slider& sl) -> int
        {
            lbl.setBounds(kPad, y, inner / 2, kRowH);
            sl.setBounds(kPad + inner / 2, y, inner / 2, kRowH);
            y += kRowH + kGap;
            return y;
        };

        // ── 1. THEME ─────────────────────────────────────────────────────────
        sectionY[0] = y;
        y += kHeaderH + kGap;
        placeToggleRow(darkModeToggle);   // button text serves as the label

        y += kSectionGap;

        // ── 2. ACCESSIBILITY ─────────────────────────────────────────────────
        sectionY[1] = y;
        y += kHeaderH + kGap;
        placeToggleRow(reducedMotionToggle);
        highContrastNote.setBounds(kPad, y, inner, kRowH - 4);
        y += kRowH + kGap;

        y += kSectionGap;

        // ── 3. MPE ───────────────────────────────────────────────────────────
        sectionY[2] = y;
        y += kHeaderH + kGap;

        if (mpeEnabledToggle != nullptr)
        {
            mpeEnabledToggle->setBounds(kPad, y, inner, kRowH);
            y += kRowH + kGap;
        }
        if (mpeZoneBox != nullptr)
            placeComboRow(*mpeZoneLabel, *mpeZoneBox);
        if (mpePBSlider != nullptr)
            placeSliderRow(*mpePBLabel, *mpePBSlider);
        if (mpePressureBox != nullptr)
            placeComboRow(*mpePressureLabel, *mpePressureBox);
        if (mpeSlideBox != nullptr)
            placeComboRow(*mpeSlideLabel, *mpeSlideBox);
        if (mpeEnabledToggle == nullptr)
        {
            mpeNotConfigured.setBounds(kPad, y, inner, kRowH);
            y += kRowH + kGap;
        }

        y += kSectionGap;

        // ── 4. PERFORMANCE LOCK ───────────────────────────────────────────────
        sectionY[3] = y;
        y += kHeaderH + kGap;
        placeToggleRow(perfLockToggle);   // button text serves as the label
        perfLockNote.setBounds(kPad, y, inner, kRowH - 4);
        y += kRowH + kGap;

        y += kSectionGap;

        // ── 5. MIDI MAPPINGS ─────────────────────────────────────────────────
        sectionY[4] = y;
        y += kHeaderH + kGap;

        // Estimate table height for layout (painted, not component-based)
        int midiRowCount = 1; // minimum 1 for empty/no-manager placeholder
        if (midiLearnMgr != nullptr)
        {
            auto m = midiLearnMgr->getMappings();
            midiRowCount = m.empty() ? 1 : static_cast<int>(m.size());
        }
        midiTableY = y;
        midiTableH = midiRowCount * kRowH;
        y += midiTableH + kGap;

        clearAllBtn.setBounds(kPad, y, 72, kRowH - 2);
        clearAllBtn.setVisible(midiLearnMgr != nullptr);
        y += kRowH + kGap;

        y += kSectionGap;

        // ── 6. ABOUT ─────────────────────────────────────────────────────────
        sectionY[5] = y;
        y += kHeaderH + kGap;
        aboutNameLabel.setBounds(kPad, y, inner, 22);
        y += 22 + 2;
        aboutVersionLabel.setBounds(kPad, y, inner, kRowH - 4);
        y += kRowH;
        aboutMakerLabel.setBounds(kPad, y, inner, kRowH - 4);
        y += kRowH;
        aboutWebLabel.setBounds(kPad, y, inner, kRowH - 4);
        y += kRowH;
        aboutPatreonLabel.setBounds(kPad, y, inner, kRowH - 4);
        y += kRowH + kGap;

        y += kPad; // bottom breathing room

        content.setSize(w, y);
    }

    //==========================================================================
    // The inner content component — we subclass to paint section headers and
    // the MIDI table (which is painted rather than component-based).
    struct ContentWithHeaders : public juce::Component
    {
        SettingsPanel* owner = nullptr;

        void paint(juce::Graphics& g) override
        {
            g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));

            if (owner == nullptr)
                return;

            const int w     = getWidth();
            const int inner = w - SettingsPanel::kPad * 2;

            static const char* kTitles[] = {
                "Theme", "Accessibility", "MPE", "Performance", "MIDI Mappings", "About"
            };
            for (int i = 0; i < 6; ++i)
                owner->drawSectionHeader(g, kTitles[i],
                                         SettingsPanel::kPad,
                                         owner->sectionY[i],
                                         inner);

            // MIDI table
            owner->paintMidiTable(g, SettingsPanel::kPad,
                                   owner->midiTableY, inner);
        }
    };

    //==========================================================================
    XOlokunProcessor& processor;
    MIDILearnManager* midiLearnMgr = nullptr;
    bool              perfLocked   = false;

    // Layout tracking (set by layoutContent, read by paint callbacks)
    int  sectionY[6]  = {};
    int  midiTableY   = 0;
    int  midiTableH   = 0;

    // Persistent settings (reduced motion, etc.)
    std::unique_ptr<juce::PropertiesFile> settingsFile;

    // Viewport + scrolled inner component
    juce::Viewport      viewport;
    ContentWithHeaders  content;

    // ── 1. Theme ─────────────────────────────────────────────────────────────
    juce::ToggleButton darkModeToggle  { "Dark Mode" };

    // ── 2. Accessibility ─────────────────────────────────────────────────────
    juce::ToggleButton reducedMotionToggle { "Reduced Motion (WCAG 2.3.3)" };
    juce::Label        highContrastNote;

    // ── 3. MPE (all optional — present only if APVTS params exist) ───────────
    std::unique_ptr<juce::ToggleButton>  mpeEnabledToggle;   // text embedded in button
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> mpeEnabledAttach;

    std::unique_ptr<juce::ComboBox>      mpeZoneBox;
    std::unique_ptr<juce::Label>         mpeZoneLabel      = makeParamLabel("Zone");
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mpeZoneAttach;

    std::unique_ptr<juce::Slider>        mpePBSlider;
    std::unique_ptr<juce::Label>         mpePBLabel        = makeParamLabel("Pitch Bend");
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mpePBAttach;

    std::unique_ptr<juce::ComboBox>      mpePressureBox;
    std::unique_ptr<juce::Label>         mpePressureLabel  = makeParamLabel("Pressure");
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mpePressureAttach;

    std::unique_ptr<juce::ComboBox>      mpeSlideBox;
    std::unique_ptr<juce::Label>         mpeSlideLabel     = makeParamLabel("Slide");
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mpeSlideAttach;

    juce::Label                          mpeNotConfigured;   // shown when no MPE params

    // ── 4. Performance Lock ───────────────────────────────────────────────────
    juce::ToggleButton perfLockToggle  { "Performance Lock" };
    juce::Label        perfLockNote;

    // ── 5. MIDI Mappings (painted, not component rows) ────────────────────────
    juce::TextButton   clearAllBtn;

    // ── 6. About ─────────────────────────────────────────────────────────────
    juce::Label        aboutNameLabel;
    juce::Label        aboutVersionLabel;
    juce::Label        aboutMakerLabel;
    juce::Label        aboutWebLabel;
    juce::Label        aboutPatreonLabel;

    //==========================================================================
    // Factory helper — creates a right-aligned param name label.
    static std::unique_ptr<juce::Label> makeParamLabel(const juce::String& text)
    {
        auto lbl = std::make_unique<juce::Label>();
        lbl->setText(text, juce::dontSendNotification);
        lbl->setFont(GalleryFonts::body(11.0f));
        lbl->setColour(juce::Label::textColourId,
                       GalleryColors::get(GalleryColors::textMid()));
        lbl->setJustificationType(juce::Justification::centredLeft);
        return lbl;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanel)
};

} // namespace xolokun
