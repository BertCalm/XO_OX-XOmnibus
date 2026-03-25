#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../../Core/MegaCouplingMatrix.h"
#include "../CouplingVisualizer/CouplingVisualizer.h"
#include "../GalleryColors.h"
#include "GalleryKnob.h"

namespace xolokun {

//==============================================================================
// PerformanceViewPanel — real-time coupling performance interface.
//
// Layout:
//   ┌──────────────────────────────────────────────────────┐
//   │  [P] PERFORMANCE VIEW                   [BAKE] [X]  │
//   ├───────────────────────┬──────────────────────────────┤
//   │                       │  Route Detail Panel          │
//   │  CouplingVisualizer   │  Route 1: [Src] → [Tgt]     │
//   │  (diamond graph viz)  │  Type: [dropdown]  Depth: ═  │
//   │                       │  Route 2 / 3 / 4 ...         │
//   ├───────────────────────┴──────────────────────────────┤
//   │  ◉ CHARACTER  ◉ MOVEMENT  ◉ COUPLING  ◉ SPACE       │
//   └──────────────────────────────────────────────────────┘
//
// Fills the same bounds as OverviewPanel / EngineDetailPanel / ChordMachinePanel.
// Toggle via "P" header button, using the same fade animation pattern.
//
class PerformanceViewPanel : public juce::Component
{
public:
    PerformanceViewPanel (XOlokunProcessor& proc)
        : processor (proc),
          apvts (proc.getAPVTS()),
          couplingVisualizer (proc.getCouplingMatrix(),
                              [&proc] (int slot) -> juce::String
                              {
                                  auto* eng = proc.getEngine (slot);
                                  return eng ? eng->getEngineId() : juce::String{};
                              },
                              [&proc] (int slot) -> juce::Colour
                              {
                                  auto* eng = proc.getEngine (slot);
                                  return eng ? eng->getAccentColour()
                                             : juce::Colour (0xFF555555);
                              })
    {
        setTitle ("Performance View");
        setDescription ("Real-time coupling performance interface with route controls and macro knobs");

        addAndMakeVisible (couplingVisualizer);
        couplingVisualizer.start();

        // ── BAKE button ──
        addAndMakeVisible (bakeBtn);
        bakeBtn.setButtonText ("BAKE");
        bakeBtn.setTooltip ("Save current coupling performance state as a reusable preset");
        bakeBtn.setEnabled (true);
        A11y::setup (bakeBtn, "Bake Coupling", "Save current coupling overlay as a coupling preset");
        bakeBtn.setColour (juce::TextButton::buttonColourId,
                           GalleryColors::get (GalleryColors::xoGold).withAlpha (0.15f));
        bakeBtn.setColour (juce::TextButton::textColourOffId,
                           GalleryColors::get (GalleryColors::xoGold));
        bakeBtn.onClick = [this] { handleBake(); };

        // ── CLEAR button — reset overlay to inactive ──
        addAndMakeVisible (clearBtn);
        clearBtn.setButtonText ("CLR");
        clearBtn.setTooltip ("Clear all performance coupling routes");
        A11y::setup (clearBtn, "Clear Coupling", "Deactivate all performance coupling routes");
        clearBtn.setColour (juce::TextButton::buttonColourId,
                            GalleryColors::get (GalleryColors::slotBg()));
        clearBtn.setColour (juce::TextButton::textColourOffId,
                            GalleryColors::get (GalleryColors::textMid()));
        clearBtn.onClick = [this] {
            processor.getCouplingPresetManager().clearOverlay();
            couplingVisualizer.refresh();
            repaint();
        };

        // ── Coupling preset selector dropdown ──
        addAndMakeVisible (couplingPresetBox);
        couplingPresetBox.setTextWhenNothingSelected ("Coupling Presets...");
        couplingPresetBox.setTooltip ("Load a saved coupling preset");
        A11y::setup (couplingPresetBox, "Coupling Preset Selector",
                     "Choose a saved coupling performance preset to load");
        couplingPresetBox.setColour (juce::ComboBox::backgroundColourId,
                                     GalleryColors::get (GalleryColors::shellWhite()));
        couplingPresetBox.setColour (juce::ComboBox::outlineColourId,
                                     GalleryColors::get (GalleryColors::borderGray()));
        couplingPresetBox.setColour (juce::ComboBox::textColourId,
                                     GalleryColors::get (GalleryColors::textDark()));
        couplingPresetBox.onChange = [this] { handleCouplingPresetSelected(); };
        refreshCouplingPresetList();

        // ── Route sections (4 routes) ──
        for (int r = 0; r < kNumRoutes; ++r)
        {
            auto& section = routes[r];
            juce::String prefix = "cp_r" + juce::String (r + 1) + "_";

            // Active toggle
            section.activeBtn.setButtonText ("ON");
            section.activeBtn.setClickingTogglesState (true);
            A11y::setup (section.activeBtn, "Route " + juce::String (r + 1) + " Enable");
            section.activeBtn.setColour (juce::TextButton::buttonColourId,
                                         GalleryColors::get (GalleryColors::slotBg()));
            section.activeBtn.setColour (juce::TextButton::buttonOnColourId,
                                         GalleryColors::get (GalleryColors::xoGold));
            section.activeBtn.setColour (juce::TextButton::textColourOffId,
                                         GalleryColors::get (GalleryColors::textMid()));
            section.activeBtn.setColour (juce::TextButton::textColourOnId,
                                         GalleryColors::get (GalleryColors::textDark()));
            section.activeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                apvts, prefix + "active", section.activeBtn);
            addAndMakeVisible (section.activeBtn);

            // Coupling type combo (14 types from CouplingType enum)
            section.typeBox.addItemList ({
                "AmpToFilter", "AmpToPitch", "LFOToPitch", "EnvToMorph",
                "AudioToFM", "AudioToRing", "FilterToFilter", "AmpToChoke",
                "RhythmToBlend", "EnvToDecay", "PitchToPitch", "AudioToWavetable",
                "AudioToBuffer", "KnotTopology"
            }, 1);
            A11y::setup (section.typeBox, "Route " + juce::String (r + 1) + " Coupling Type");
            section.typeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, prefix + "type", section.typeBox);
            addAndMakeVisible (section.typeBox);

            // Depth slider (bipolar -1.0 to 1.0)
            section.depthSlider.setSliderStyle (juce::Slider::LinearHorizontal);
            section.depthSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 16);
            section.depthSlider.setColour (juce::Slider::thumbColourId,
                                           GalleryColors::get (GalleryColors::xoGold));
            section.depthSlider.setColour (juce::Slider::trackColourId,
                                           GalleryColors::get (GalleryColors::borderGray()));
            section.depthSlider.setTooltip ("Route " + juce::String (r + 1) + " depth (bipolar)");
            A11y::setup (section.depthSlider, "Route " + juce::String (r + 1) + " Depth");
            section.depthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                apvts, prefix + "amount", section.depthSlider);
            addAndMakeVisible (section.depthSlider);

            // Source slot selector
            section.sourceBox.addItemList ({ "Slot 1", "Slot 2", "Slot 3", "Slot 4" }, 1);
            A11y::setup (section.sourceBox, "Route " + juce::String (r + 1) + " Source Slot");
            section.sourceAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, prefix + "source", section.sourceBox);
            addAndMakeVisible (section.sourceBox);

            // Target slot selector
            section.targetBox.addItemList ({ "Slot 1", "Slot 2", "Slot 3", "Slot 4" }, 1);
            A11y::setup (section.targetBox, "Route " + juce::String (r + 1) + " Target Slot");
            section.targetAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, prefix + "target", section.targetBox);
            addAndMakeVisible (section.targetBox);

            // Route label
            section.label.setFont (GalleryFonts::heading (9.0f));
            section.label.setColour (juce::Label::textColourId,
                                     GalleryColors::get (GalleryColors::textDark()));
            section.label.setJustificationType (juce::Justification::centredLeft);
            addAndMakeVisible (section.label);
        }

        // ── Macro knobs (performance context duplicate) ──
        struct MacroDef { const char* id; const char* label; };
        static constexpr MacroDef macroDefs[4] = {
            { "macro1", "CHARACTER" }, { "macro2", "MOVEMENT" },
            { "macro3", "COUPLING" }, { "macro4", "SPACE" }
        };
        for (int i = 0; i < 4; ++i)
        {
            macroKnobs[i].setSliderStyle (juce::Slider::RotaryVerticalDrag);
            macroKnobs[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            macroKnobs[i].setColour (juce::Slider::rotarySliderFillColourId,
                                     GalleryColors::get (GalleryColors::xoGold));
            macroKnobs[i].setTooltip (juce::String ("Macro ") + juce::String (i + 1)
                                      + ": " + macroDefs[i].label);
            A11y::setup (macroKnobs[i], juce::String ("Macro ") + juce::String (i + 1)
                                        + " " + macroDefs[i].label);
            addAndMakeVisible (macroKnobs[i]);
            macroAttach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                apvts, macroDefs[i].id, macroKnobs[i]);
            enableKnobReset (macroKnobs[i], apvts, macroDefs[i].id);

            macroLabels[i].setText (macroDefs[i].label, juce::dontSendNotification);
            macroLabels[i].setFont (GalleryFonts::heading (8.0f));
            macroLabels[i].setColour (juce::Label::textColourId,
                                      GalleryColors::get (GalleryColors::textMid()));
            macroLabels[i].setJustificationType (juce::Justification::centred);
            addAndMakeVisible (macroLabels[i]);
        }

        updateRouteLabels();
    }

    // Call from editor timer or on engine change to keep route labels current.
    void refresh()
    {
        couplingVisualizer.refresh();
        updateRouteLabels();
        repaint();
    }

    // Refresh the coupling preset dropdown from the CouplingPresetManager library.
    void refreshCouplingPresetList()
    {
        couplingPresetBox.clear (juce::dontSendNotification);
        auto& cpm = processor.getCouplingPresetManager();
        auto names = cpm.getPresetNames();
        if (names.isEmpty())
        {
            couplingPresetBox.setTextWhenNothingSelected ("No presets saved");
        }
        else
        {
            couplingPresetBox.setTextWhenNothingSelected ("Coupling Presets...");
            for (int i = 0; i < names.size(); ++i)
                couplingPresetBox.addItem (names[i], i + 1);
        }
    }

    void paint (juce::Graphics& g) override
    {
        using namespace GalleryColors;
        g.fillAll (get (slotBg()));

        // ── Header bar ──
        auto headerArea = getLocalBounds().removeFromTop (kHeaderH).toFloat();
        g.setColour (get (xoGold));
        g.fillRect (headerArea);

        g.setColour (juce::Colours::white);
        g.setFont (GalleryFonts::display (14.0f));
        g.drawText ("PERFORMANCE VIEW", headerArea.reduced (12, 0),
                     juce::Justification::centredLeft);

        // ── BAKE flash feedback ──
        if (bakeFlashAlpha > 0.0f)
        {
            g.setColour (get (xoGold).withAlpha (bakeFlashAlpha));
            g.fillRect (getLocalBounds().toFloat());
        }

        // ── Separator between coupling strip and route panel ──
        auto body = getLocalBounds().withTrimmedTop (kHeaderH).withTrimmedBottom (kMacroStripH);
        int sepX = body.getX() + (int) (body.getWidth() * kStripRatio);
        g.setColour (get (borderGray()));
        g.drawVerticalLine (sepX, (float) body.getY(), (float) body.getBottom());

        // ── Route section backgrounds ──
        auto routeArea = body.withTrimmedLeft ((int) (body.getWidth() * kStripRatio) + 1)
                             .reduced (6, 4);
        int routeH = routeArea.getHeight() / kNumRoutes;

        for (int r = 0; r < kNumRoutes; ++r)
        {
            auto sectionRect = routeArea.withY (routeArea.getY() + r * routeH)
                                        .withHeight (routeH).reduced (0, 2).toFloat();

            // Subtle rounded rect background per route
            g.setColour (get (shellWhite()).withAlpha (0.5f));
            g.fillRoundedRectangle (sectionRect, 4.0f);
            g.setColour (get (borderGray()).withAlpha (0.4f));
            g.drawRoundedRectangle (sectionRect, 4.0f, 0.5f);
        }

        // ── Macro strip separator ──
        auto macroStripArea = getLocalBounds().removeFromBottom (kMacroStripH).toFloat();
        g.setColour (get (borderGray()));
        g.drawHorizontalLine ((int) macroStripArea.getY(), 0.0f, (float) getWidth());

        // ── Macro strip background ──
        g.setColour (get (shellWhite()));
        g.fillRect (macroStripArea);

        // ── XO Gold accent at top of macro strip ──
        g.setColour (get (xoGold));
        g.fillRect (macroStripArea.removeFromTop (2.0f));

        // ── Macro strip header ──
        g.setColour (get (textMid()));
        g.setFont (GalleryFonts::heading (8.0f));
        g.drawText ("MACROS", getLocalBounds().removeFromBottom (kMacroStripH)
                                              .removeFromTop (14),
                     juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds();

        // ── Header ──
        auto header = area.removeFromTop (kHeaderH);
        bakeBtn.setBounds (header.removeFromRight (60).reduced (8, 6));
        clearBtn.setBounds (header.removeFromRight (42).reduced (4, 6));
        header.removeFromRight (4);  // spacing
        couplingPresetBox.setBounds (header.removeFromRight (160).reduced (4, 6));

        // ── Macro strip at bottom ──
        auto macroArea = area.removeFromBottom (kMacroStripH).reduced (8, 4);
        macroArea.removeFromTop (14); // space for "MACROS" label
        int macroKnobW = macroArea.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
        {
            auto col = macroArea.removeFromLeft (macroKnobW);
            int kh = 44;
            int kx = col.getCentreX() - kh / 2;
            macroKnobs[i].setBounds (kx, col.getY(), kh, kh);
            macroLabels[i].setBounds (kx, col.getY() + kh + 1, kh, 12);
        }

        // ── Body: coupling strip (left 40%) + route panel (right 60%) ──
        auto body = area;
        int stripW = (int) (body.getWidth() * kStripRatio);
        couplingVisualizer.setBounds (body.removeFromLeft (stripW));

        // ── Route sections ──
        auto routePanel = body.reduced (6, 4);
        int routeH = routePanel.getHeight() / kNumRoutes;

        for (int r = 0; r < kNumRoutes; ++r)
        {
            auto section = routePanel.withY (routePanel.getY() + r * routeH)
                                     .withHeight (routeH).reduced (4, 4);

            auto& rt = routes[r];

            // Row 1: label + active toggle
            auto row1 = section.removeFromTop (20);
            rt.activeBtn.setBounds (row1.removeFromRight (36).reduced (0, 1));
            rt.label.setBounds (row1);

            section.removeFromTop (2);

            // Row 2: source → target selectors
            auto row2 = section.removeFromTop (22);
            int halfW = (row2.getWidth() - 16) / 2;
            rt.sourceBox.setBounds (row2.removeFromLeft (halfW).reduced (0, 1));
            row2.removeFromLeft (16); // arrow spacing
            rt.targetBox.setBounds (row2.removeFromLeft (halfW).reduced (0, 1));

            section.removeFromTop (2);

            // Row 3: type dropdown + depth slider
            auto row3 = section.removeFromTop (22);
            rt.typeBox.setBounds (row3.removeFromLeft (row3.getWidth() * 2 / 5).reduced (0, 1));
            row3.removeFromLeft (4);
            rt.depthSlider.setBounds (row3.reduced (0, 1));
        }
    }

private:
    void updateRouteLabels()
    {
        for (int r = 0; r < kNumRoutes; ++r)
        {
            juce::String prefix = "cp_r" + juce::String (r + 1) + "_";

            // Read source/target slot from APVTS to display engine names
            int srcSlot = 0, tgtSlot = 1;
            if (auto* srcParam = apvts.getRawParameterValue (prefix + "source"))
                srcSlot = juce::roundToInt (srcParam->load());
            if (auto* tgtParam = apvts.getRawParameterValue (prefix + "target"))
                tgtSlot = juce::roundToInt (tgtParam->load());

            juce::String srcName = "-";
            juce::String tgtName = "-";
            if (auto* eng = processor.getEngine (srcSlot))
                srcName = eng->getEngineId().toUpperCase();
            if (auto* eng = processor.getEngine (tgtSlot))
                tgtName = eng->getEngineId().toUpperCase();

            routes[r].label.setText (
                "Route " + juce::String (r + 1) + ":  " + srcName + "  \xe2\x86\x92  " + tgtName,
                juce::dontSendNotification);
        }
    }

    //==========================================================================
    // BAKE handler — capture overlay, prompt for name, save to disk
    //==========================================================================
    void handleBake()
    {
        auto& cpm = processor.getCouplingPresetManager();

        // Quick check: is there anything to bake?
        auto snapshot = cpm.bakeCurrent ("Preview");
        if (!snapshot.hasActiveRoutes())
        {
            // Nothing to bake — flash the button red briefly as feedback
            bakeBtn.setColour (juce::TextButton::buttonColourId, juce::Colour (0x40FF4444));
            juce::Timer::callAfterDelay (400, [safeThis = juce::Component::SafePointer<PerformanceViewPanel> (this)] {
                if (safeThis)
                {
                    safeThis->bakeBtn.setColour (juce::TextButton::buttonColourId,
                        GalleryColors::get (GalleryColors::xoGold).withAlpha (0.15f));
                    safeThis->repaint();
                }
            });
            return;
        }

        // Show a name input dialog
        auto* alertWindow = new juce::AlertWindow (
            "Bake Coupling Preset",
            "Enter a name for this coupling configuration:",
            juce::MessageBoxIconType::NoIcon, this);
        alertWindow->addTextEditor ("name", "Untitled Coupling", "Preset Name:");
        alertWindow->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        alertWindow->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        alertWindow->enterModalState (true, juce::ModalCallbackFunction::create (
            [safeThis = juce::Component::SafePointer<PerformanceViewPanel> (this), alertWindow] (int result)
            {
                if (result == 1 && safeThis)
                {
                    auto presetName = alertWindow->getTextEditorContents ("name").trim();
                    if (presetName.isEmpty())
                        presetName = "Untitled Coupling";

                    safeThis->performBake (presetName);
                }
                delete alertWindow;
            }), false);
    }

    void performBake (const juce::String& presetName)
    {
        auto& cpm = processor.getCouplingPresetManager();

        // Capture the current overlay state
        auto state = cpm.bakeCurrent (presetName);
        state.author = "User";

        // Determine file path
        auto dir = CouplingPresetManager::getDefaultDirectory();
        auto sanitizedName = presetName.replaceCharacters (" /\\:*?\"<>|", "___________");
        auto file = dir.getChildFile (sanitizedName + ".xocoupling");

        // Handle name collisions — append a number if needed
        int suffix = 1;
        while (file.existsAsFile() && suffix < 100)
        {
            file = dir.getChildFile (sanitizedName + "_" + juce::String (suffix) + ".xocoupling");
            ++suffix;
        }

        // Save to disk
        if (cpm.saveToFile (file, state))
        {
            // Re-scan the library to pick up the new preset
            cpm.scanDirectory (CouplingPresetManager::getDefaultDirectory());
            refreshCouplingPresetList();

            // Visual feedback — XO Gold flash
            triggerBakeFlash();
        }
    }

    //==========================================================================
    // Coupling preset recall handler
    //==========================================================================
    void handleCouplingPresetSelected()
    {
        int selectedId = couplingPresetBox.getSelectedId();
        if (selectedId <= 0)
            return;

        int index = selectedId - 1;
        auto& cpm = processor.getCouplingPresetManager();
        const auto* preset = cpm.getPreset (index);
        if (preset)
        {
            cpm.loadBakedCoupling (*preset);
            couplingVisualizer.refresh();
            updateRouteLabels();
            repaint();
        }
    }

    //==========================================================================
    // BAKE flash animation — brief XO Gold overlay that fades out
    //==========================================================================
    void triggerBakeFlash()
    {
        bakeFlashAlpha = 0.3f;
        repaint();

        // Fade out over 500ms using a timer callback chain
        fadeOutBakeFlash();
    }

    void fadeOutBakeFlash()
    {
        juce::Timer::callAfterDelay (50, [safeThis = juce::Component::SafePointer<PerformanceViewPanel> (this)] {
            if (!safeThis) return;

            safeThis->bakeFlashAlpha -= 0.05f;
            if (safeThis->bakeFlashAlpha <= 0.0f)
            {
                safeThis->bakeFlashAlpha = 0.0f;
                safeThis->repaint();
                return;
            }

            safeThis->repaint();
            safeThis->fadeOutBakeFlash();
        });
    }

    static constexpr int kHeaderH     = 34;
    static constexpr int kMacroStripH = 80;
    static constexpr int kNumRoutes   = 4;
    static constexpr float kStripRatio = 0.40f;

    XOlokunProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    // Left panel: full-graph coupling visualizer (replaces CouplingStripEditor)
    CouplingVisualizer couplingVisualizer;

    // Header controls
    juce::TextButton bakeBtn;
    juce::TextButton clearBtn;
    juce::ComboBox   couplingPresetBox;

    // BAKE flash animation state
    float bakeFlashAlpha = 0.0f;

    // Per-route controls
    struct RouteSection
    {
        juce::Label      label;
        juce::TextButton activeBtn;
        juce::ComboBox   typeBox;
        juce::Slider     depthSlider;
        juce::ComboBox   sourceBox;
        juce::ComboBox   targetBox;

        // APVTS attachments — must be destroyed before the controls they reference.
        // Declared after controls so they are destroyed first (reverse declaration order).
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   activeAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   depthAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> targetAttach;
    };
    std::array<RouteSection, kNumRoutes> routes;

    // Bottom macro strip
    std::array<GalleryKnob, 4> macroKnobs;
    std::array<juce::Label, 4>  macroLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> macroAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PerformanceViewPanel)
};

} // namespace xolokun
