#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include "GalleryKnob.h"
#include "MidiLearnMouseListener.h"
#include "AdvancedFXPanel.h"

namespace xolokun {

//==============================================================================
// MasterFXSection — 6-section FX strip with primary knobs per section +
// ADV buttons for deeper parameters. Always visible at bottom of Gallery Model.
//
// Layout: SAT | DELAY | REVERB | MOD | COMP | SEQ
class MasterFXSection : public juce::Component
{
public:
    explicit MasterFXSection(juce::AudioProcessorValueTreeState& apvts) : myApvts(apvts)
    {
        // Primary knob definitions: { paramID, knobLabel, sectionLabel }
        struct Def { const char* id; const char* label; const char* section; };
        static constexpr Def defs[kNumPrimaryKnobs] = {
            {"master_satDrive",      "DRIVE",  "SAT"},
            {"master_delayMix",      "MIX",    "DELAY"},
            {"master_delayFeedback", "FB",     ""},
            {"master_reverbMix",     "MIX",    "REVERB"},
            {"master_modDepth",      "DEPTH",  "MOD"},
            {"master_compMix",       "GLUE",   "COMP"},
        };

        for (int i = 0; i < kNumPrimaryKnobs; ++i)
        {
            knobs[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setColour(juce::Slider::rotarySliderFillColourId,
                               GalleryColors::get(GalleryColors::textMid()).withAlpha(0.7f));
            knobs[i].setTooltip(juce::String(defs[i].section) + " " + defs[i].label);
            A11y::setup(knobs[i], juce::String("Master FX ") + defs[i].section + " " + defs[i].label);
            addAndMakeVisible(knobs[i]);
            attach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, defs[i].id, knobs[i]);
            enableKnobReset (knobs[i], apvts, defs[i].id);

            lbls[i].setText(defs[i].label, juce::dontSendNotification);
            lbls[i].setFont(GalleryFonts::heading(7.5f));
            lbls[i].setColour(juce::Label::textColourId,
                              GalleryColors::get(GalleryColors::textMid()));
            lbls[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(lbls[i]);
        }

        // Section header labels
        static const char* sectionNames[kNumSections] = {"SAT", "DELAY", "REVERB", "MOD", "COMP", "SEQ"};
        for (int i = 0; i < kNumSections; ++i)
        {
            sectionHdrs[i].setText(sectionNames[i], juce::dontSendNotification);
            sectionHdrs[i].setFont(GalleryFonts::label(7.0f));
            sectionHdrs[i].setColour(juce::Label::textColourId,
                                     GalleryColors::get(GalleryColors::textMid()).withAlpha(0.50f));
            sectionHdrs[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(sectionHdrs[i]);
        }

        // ADV buttons for each section
        auto makeAdvBtn = [&](juce::TextButton& btn, const juce::String& tip, const juce::String& a11y) {
            btn.setButtonText("ADV");
            btn.setTooltip(tip);
            A11y::setup(btn, a11y, "Open advanced settings");
            btn.setColour(juce::TextButton::buttonColourId,
                          GalleryColors::get(GalleryColors::shellWhite()));
            btn.setColour(juce::TextButton::textColourOffId,
                          GalleryColors::get(GalleryColors::textMid()).withAlpha(0.6f));
            addAndMakeVisible(btn);
        };

        makeAdvBtn(advBtnDelay, "Delay Time, Ping Pong, Damping, Diffusion, Sync", "Advanced Delay");
        advBtnDelay.onClick = [this] {
            showAdvancedPanel(advBtnDelay, "DELAY ADVANCED", {
                {"master_delayTime",     "TIME"},
                {"master_delayPingPong", "P.PONG"},
                {"master_delayDamping",  "DAMP"},
                {"master_delayDiffusion","DIFF"},
                {"master_delaySync",     "SYNC"},
            });
        };

        makeAdvBtn(advBtnReverb, "Reverb Size", "Advanced Reverb");
        advBtnReverb.onClick = [this] {
            showAdvancedPanel(advBtnReverb, "REVERB ADVANCED", {
                {"master_reverbSize", "SIZE"},
            });
        };

        makeAdvBtn(advBtnMod, "Mod Rate, Mode, Feedback", "Advanced Modulation");
        advBtnMod.onClick = [this] {
            showAdvancedPanel(advBtnMod, "MOD ADVANCED", {
                {"master_modRate",     "RATE"},
                {"master_modMix",      "MIX"},
                {"master_modMode",     "MODE"},
                {"master_modFeedback", "FB"},
            });
        };

        makeAdvBtn(advBtnComp, "Comp Ratio, Attack, Release", "Advanced Compressor");
        advBtnComp.onClick = [this] {
            showAdvancedPanel(advBtnComp, "COMP ADVANCED", {
                {"master_compRatio",   "RATIO"},
                {"master_compAttack",  "ATTACK"},
                {"master_compRelease", "RELEASE"},
            });
        };

        // SEQ section: enable toggle + ADV
        seqToggle.setButtonText("SEQ");
        seqToggle.setTooltip("Enable Master FX Sequencer");
        A11y::setup(seqToggle, "Sequencer Enable", "Toggle the Master FX step sequencer");
        seqToggle.setColour(juce::TextButton::buttonColourId,
                            GalleryColors::get(GalleryColors::shellWhite()));
        seqToggle.setColour(juce::TextButton::buttonOnColourId,
                            GalleryColors::get(GalleryColors::xoGold));
        seqToggle.setColour(juce::TextButton::textColourOffId,
                            GalleryColors::get(GalleryColors::textMid()));
        seqToggle.setColour(juce::TextButton::textColourOnId,
                            GalleryColors::get(GalleryColors::textDark()));
        seqToggle.setClickingTogglesState(true);
        seqAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "master_seqEnabled", seqToggle);
        addAndMakeVisible(seqToggle);

        makeAdvBtn(advBtnSeq, "Seq Rate, Steps, Depth, Smooth, Pattern, Targets", "Advanced Sequencer");
        advBtnSeq.onClick = [this] {
            showAdvancedPanel(advBtnSeq, "SEQUENCER", {
                {"master_seqRate",      "RATE"},
                {"master_seqSteps",     "STEPS"},
                {"master_seqDepth",     "DEPTH"},
                {"master_seqSmooth",    "SMOOTH"},
                {"master_seqPattern",   "PATTERN"},
                {"master_seqTarget1",   "TGT 1"},
                {"master_seqTarget2",   "TGT 2"},
                {"master_seqEnvAmount", "ENV"},
            });
        };
    }

    // W45 fix: remove MIDI learn mouse listeners from knobs before unique_ptrs
    // are destroyed.  fxLearnListeners (declared after knobs) would be destroyed
    // first by default, leaving dangling raw pointers in the knobs' listener lists.
    ~MasterFXSection() override
    {
        for (int i = 0; i < kNumPrimaryKnobs; ++i)
            if (fxLearnListeners[i])
                knobs[i].removeMouseListener(fxLearnListeners[i].get());
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        auto b = getLocalBounds().toFloat();
        g.setColour(get(shellWhite()).darker(0.03f));
        g.fillRoundedRectangle(b, 6.0f);
        g.setColour(get(borderGray()));
        g.drawRoundedRectangle(b.reduced(0.5f), 6.0f, 1.0f);

        // Dividers between sections
        g.setColour(get(borderGray()).withAlpha(0.5f));
        for (int i = 0; i < kNumSections - 1; ++i)
        {
            if (divX[i] > 0)
                g.drawLine(static_cast<float>(divX[i]), 8.0f,
                           static_cast<float>(divX[i]), static_cast<float>(getHeight()) - 8, 1.0f);
        }
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(4, 3);

        // Section widths: proportional based on knob count
        // SAT(1 knob) : DELAY(2) : REVERB(1) : MOD(1) : COMP(1) : SEQ(toggle+adv)
        // Weights:  2 : 3 : 2 : 2 : 2 : 2  = 13 total
        static constexpr int weights[kNumSections] = {2, 3, 2, 2, 2, 2};
        static constexpr int totalWeight = 13;
        int totalW = b.getWidth();

        int sectionX[kNumSections + 1];
        sectionX[0] = b.getX();
        for (int i = 0; i < kNumSections; ++i)
            sectionX[i + 1] = sectionX[i] + (totalW * weights[i]) / totalWeight;

        // Record divider positions
        for (int i = 0; i < kNumSections - 1; ++i)
            divX[i] = sectionX[i + 1];

        int topY = b.getY();
        int h = b.getHeight();

        // Helper: lay out knobs in a section
        auto layoutKnob = [&](int knobIdx, int sx, int sw) {
            int kh = 36, lh = 10;
            int ky = topY + 10 + (h - 10 - kh - lh) / 2;
            knobs[knobIdx].setBounds(sx + (sw - kh) / 2, ky, kh, kh);
            lbls[knobIdx].setBounds(sx, ky + kh + 1, sw, lh);
        };

        auto layoutAdvBtn = [&](juce::TextButton& btn, int sx, int sw, int topOff) {
            btn.setBounds(sx + (sw - 28) / 2, topY + h - 16 + topOff, 28, 13);
        };

        // Section 0: SAT — 1 knob (DRIVE)
        {
            int sx = sectionX[0], sw = sectionX[1] - sectionX[0];
            sectionHdrs[0].setBounds(sx, topY, sw, 10);
            layoutKnob(0, sx, sw);
        }

        // Section 1: DELAY — 2 knobs (MIX, FB) + ADV
        {
            int sx = sectionX[1], sw = sectionX[2] - sectionX[1];
            sectionHdrs[1].setBounds(sx, topY, sw, 10);
            int halfW = sw / 2;
            layoutKnob(1, sx, halfW);
            layoutKnob(2, sx + halfW, halfW);
            layoutAdvBtn(advBtnDelay, sx, sw, 0);
        }

        // Section 2: REVERB — 1 knob (MIX) + ADV
        {
            int sx = sectionX[2], sw = sectionX[3] - sectionX[2];
            sectionHdrs[2].setBounds(sx, topY, sw, 10);
            layoutKnob(3, sx, sw);
            layoutAdvBtn(advBtnReverb, sx, sw, 0);
        }

        // Section 3: MOD — 1 knob (DEPTH) + ADV
        {
            int sx = sectionX[3], sw = sectionX[4] - sectionX[3];
            sectionHdrs[3].setBounds(sx, topY, sw, 10);
            layoutKnob(4, sx, sw);
            layoutAdvBtn(advBtnMod, sx, sw, 0);
        }

        // Section 4: COMP — 1 knob (GLUE) + ADV
        {
            int sx = sectionX[4], sw = sectionX[5] - sectionX[4];
            sectionHdrs[4].setBounds(sx, topY, sw, 10);
            layoutKnob(5, sx, sw);
            layoutAdvBtn(advBtnComp, sx, sw, 0);
        }

        // Section 5: SEQ — toggle + ADV
        {
            int sx = sectionX[5], sw = sectionX[6] - sectionX[5];
            sectionHdrs[5].setBounds(sx, topY, sw, 10);
            int btnW = juce::jmin(sw - 4, 36);
            seqToggle.setBounds(sx + (sw - btnW) / 2, topY + 14, btnW, 22);
            layoutAdvBtn(advBtnSeq, sx, sw, 0);
        }
    }

    // Wire MIDI Learn to each primary FX knob.
    // Call from XOlokunEditor after construction.
    void setupMidiLearn(MIDILearnManager& mgr)
    {
        static const char* ids[kNumPrimaryKnobs] = {
            "master_satDrive", "master_delayMix", "master_delayFeedback",
            "master_reverbMix", "master_modDepth", "master_compMix"
        };
        for (int i = 0; i < kNumPrimaryKnobs; ++i)
        {
            auto* ml = knobs[i].setupMidiLearn(ids[i], mgr);
            fxLearnListeners[i].reset(ml);
        }
    }

private:
    void showAdvancedPanel(juce::TextButton& btn, const juce::String& title,
                           const std::vector<std::pair<juce::String, juce::String>>& params)
    {
        juce::CallOutBox::launchAsynchronously(
            std::make_unique<AdvancedFXPanel>(myApvts, title, params),
            btn.getScreenBounds(),
            getTopLevelComponent());
    }

    static constexpr int kNumPrimaryKnobs = 6;
    static constexpr int kNumSections = 6;

    juce::AudioProcessorValueTreeState& myApvts;

    // Primary knobs
    std::array<GalleryKnob, kNumPrimaryKnobs> knobs;
    std::array<juce::Label, kNumPrimaryKnobs>  lbls;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, kNumPrimaryKnobs> attach;

    // Section headers
    std::array<juce::Label, kNumSections> sectionHdrs;

    // ADV buttons per section
    juce::TextButton advBtnDelay, advBtnReverb, advBtnMod, advBtnComp, advBtnSeq;

    // SEQ toggle
    juce::TextButton seqToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqAttach;

    // Divider X positions
    int divX[kNumSections - 1] = {};

    // MIDI learn listeners — destroyed before knobs
    std::array<std::unique_ptr<MidiLearnMouseListener>, kNumPrimaryKnobs> fxLearnListeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterFXSection)
};

} // namespace xolokun
