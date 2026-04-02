// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOceanusProcessor.h"
#include "../GalleryColors.h"
#include "GalleryKnob.h"
#include "MidiLearnMouseListener.h"

// clang-format off
namespace xoceanus {

//==============================================================================
// ObrixDetailPanel — flagship custom hierarchical UI for the OBRIX engine.
//
// OBRIX is the reef: 81 params (obrix_*) spanning Brick Sources, Brick Ecology,
// Harmonic Field, Environmental, FX/Stateful, and Remaining sections.
//
// Sections stacked vertically inside a juce::Viewport:
//   1. Brick Stack View  (~200pt)  — Src1 + Src2 with accordion expand/collapse
//   2. Reef Ecology      (~120pt)  — resident/competition/symbiosis + ecology indicator
//   3. Harmonic Field    (~80pt)   — JI attractor/repulsor controls
//   4. Environmental     (~80pt)   — temp/pressure/current/turbidity row
//   5. FX + Stateful     (~80pt)   — serial/parallel toggle, stress/bleach/reset
//   6. Other             (auto)    — any obrix_* params not covered above
//
// Smart accordion: compactMode is recalculated in resized(); when true, expanding
// one brick collapses the other.  Both bricks can be open when height > 500pt.
//
// Destruction order: MIDI learn listeners → attachments → knobs/combos.
// (Members are destroyed bottom-up per C++ declaration order — listeners declared
//  AFTER knobs/combos to ensure listeners are gone before their observed controls.)
//
// Gallery code: OBRIX | Accent: Reef Jade #1E8B7E | Prefix: obrix_
// All 81 obrix_* params wired — D004 doctrine compliance guaranteed.
//==============================================================================

class ObrixDetailPanel : public juce::Component
{
public:
    // ── Reef Jade accent (OBRIX canonical color) ────────────────────────────
    static constexpr uint32_t kReefJade   = 0xFF1E8B7E;
    static constexpr uint32_t kAmberColor = 0xFFE9A84A;
    static constexpr uint32_t kBleached   = 0xFFF5F5F5;

    // ──────────────────────────────────────────────────────────────────────────
    // Constructor — eagerly creates all ~81 knobs and attachments.
    // This panel is only instantiated when an OBRIX engine is loaded, so the
    // eager cost is acceptable and avoids deferred-allocation race conditions.
    // ──────────────────────────────────────────────────────────────────────────
    explicit ObrixDetailPanel(XOceanusProcessor& proc, MIDILearnManager* midiLearn = nullptr)
        : apvts(proc.getAPVTS()), learnManager(midiLearn)
    {
        const juce::Colour accent = juce::Colour(kReefJade);

        // ── Helpers ──────────────────────────────────────────────────────────
        // makeKnob: configure a GalleryKnob, add it to this component, wire
        //   a SliderAttachment, set knob reset, optionally wire MIDI learn.
        //   Returns pointer to the attachment (stored by caller in a member).
        auto makeKnob = [&](GalleryKnob& knob,
                            const juce::String& pid,
                            juce::Colour fill = juce::Colour(kReefJade))
            -> std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        {
            knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knob.setColour(juce::Slider::rotarySliderFillColourId, fill);
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(pid)))
                knob.setTooltip(rp->getName(64));
            A11y::setup(knob, pid, "OBRIX: " + pid, true);
            addAndMakeVisible(knob);
            auto att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, pid, knob);
            enableKnobReset(knob, apvts, pid);
            if (learnManager)
            {
                auto* ml = knob.setupMidiLearn(pid, *learnManager);
                midiLearnListeners.emplace_back(ml);
            }
            return att;
        };

        auto makeComboBox = [&](juce::ComboBox& cb, const juce::String& pid)
            -> std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        {
            cb.setColour(juce::ComboBox::backgroundColourId,
                         GalleryColors::get(GalleryColors::slotBg()));
            cb.setColour(juce::ComboBox::textColourId,
                         GalleryColors::get(GalleryColors::textDark()));
            cb.setColour(juce::ComboBox::outlineColourId,
                         juce::Colour(kReefJade).withAlpha(0.55f));
            A11y::setup(cb, pid, "OBRIX: " + pid, true);
            addAndMakeVisible(cb);
            // Populate choices from the parameter's string array
            if (auto* cp = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(pid)))
            {
                cb.clear(juce::dontSendNotification);
                for (int i = 0; i < cp->choices.size(); ++i)
                    cb.addItem(cp->choices[i], i + 1);
            }
            return std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, pid, cb);
        };

        auto makeButton = [&](juce::TextButton& btn, const juce::String& pid,
                               const juce::String& offText, const juce::String& onText)
            -> std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        {
            btn.setButtonText(offText);
            btn.setColour(juce::TextButton::buttonColourId,
                          GalleryColors::get(GalleryColors::slotBg()));
            btn.setColour(juce::TextButton::buttonOnColourId,
                          juce::Colour(kReefJade).withAlpha(0.80f));
            btn.setColour(juce::TextButton::textColourOffId,
                          GalleryColors::get(GalleryColors::textDark()));
            btn.setColour(juce::TextButton::textColourOnId,
                          juce::Colours::white);  // white-on-accent for contrast on Reef Jade
            btn.setToggleable(true);
            btn.setClickingTogglesState(true);
            (void)onText; // toggled text update handled via paint if needed
            A11y::setup(btn, pid, "OBRIX: " + pid, true);
            addAndMakeVisible(btn);
            return std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, pid, btn);
        };

        auto makeLabel = [&](juce::Label& lbl, const juce::String& text,
                              float sizePt = 8.0f) {
            lbl.setText(text, juce::dontSendNotification);
            lbl.setFont(GalleryFonts::label(sizePt));
            lbl.setColour(juce::Label::textColourId,
                          GalleryColors::get(GalleryColors::textMid()));
            lbl.setJustificationType(juce::Justification::centred);
            lbl.setInterceptsMouseClicks(false, false);
            addAndMakeVisible(lbl);
        };

        // ── Section 1 — Brick Stack: Source 1 ───────────────────────────────
        brick1Header.setButtonText("SRC 1");
        brick1Header.setColour(juce::TextButton::buttonColourId,
                               juce::Colour(kReefJade).withAlpha(0.18f));
        brick1Header.setColour(juce::TextButton::buttonOnColourId,
                               juce::Colour(kReefJade).withAlpha(0.35f));
        brick1Header.setColour(juce::TextButton::textColourOffId,
                               GalleryColors::get(GalleryColors::textDark()));
        brick1Header.setColour(juce::TextButton::textColourOnId,
                               juce::Colour(kReefJade));
        brick1Header.setToggleable(true);
        brick1Header.setClickingTogglesState(true);
        brick1Header.setToggleState(true, juce::dontSendNotification);
        brick1Header.onClick = [this] { onBrickHeaderClicked(0); };
        A11y::setup(brick1Header, "Source 1 section expand/collapse", {}, true);
        addAndMakeVisible(brick1Header);

        brick2Header.setButtonText("SRC 2");
        brick2Header.setColour(juce::TextButton::buttonColourId,
                               juce::Colour(kReefJade).withAlpha(0.18f));
        brick2Header.setColour(juce::TextButton::buttonOnColourId,
                               juce::Colour(kReefJade).withAlpha(0.35f));
        brick2Header.setColour(juce::TextButton::textColourOffId,
                               GalleryColors::get(GalleryColors::textDark()));
        brick2Header.setColour(juce::TextButton::textColourOnId,
                               juce::Colour(kReefJade));
        brick2Header.setToggleable(true);
        brick2Header.setClickingTogglesState(true);
        brick2Header.setToggleState(false, juce::dontSendNotification);
        brick2Header.onClick = [this] { onBrickHeaderClicked(1); };
        A11y::setup(brick2Header, "Source 2 section expand/collapse", {}, true);
        addAndMakeVisible(brick2Header);

        // Src 1 params
        src1TypeAtt  = makeComboBox(src1TypeCB,  "obrix_src1Type");
        src1TuneAtt  = makeKnob   (src1TuneKnob, "obrix_src1Tune");
        src1PWAtt    = makeKnob   (src1PWKnob,   "obrix_src1PW");
        makeLabel(src1TuneLbl,  "TUNE");
        makeLabel(src1PWLbl,    "PW");

        // Src 2 params
        src2TypeAtt  = makeComboBox(src2TypeCB,  "obrix_src2Type");
        src2TuneAtt  = makeKnob   (src2TuneKnob, "obrix_src2Tune");
        src2PWAtt    = makeKnob   (src2PWKnob,   "obrix_src2PW");
        makeLabel(src2TuneLbl, "TUNE");
        makeLabel(src2PWLbl,   "PW");

        // Shared source params (always visible in brick section)
        srcMixAtt    = makeKnob(srcMixKnob,    "obrix_srcMix");
        fmDepthAtt   = makeKnob(fmDepthKnob,   "obrix_fmDepth",   accent.brighter(0.1f));
        makeLabel(srcMixLbl,   "SRC MIX");
        makeLabel(fmDepthLbl,  "FM DEPTH");

        // Proc 1 (under Src 1)
        proc1TypeAtt   = makeComboBox(proc1TypeCB,   "obrix_proc1Type");
        proc1CutoffAtt = makeKnob    (proc1CutKnob,  "obrix_proc1Cutoff",  juce::Colour(0xFFFF6B6B));
        proc1ResoAtt   = makeKnob    (proc1ResKnob,  "obrix_proc1Reso",    juce::Colour(0xFFFF6B6B));
        proc1FbAtt     = makeKnob    (proc1FbKnob,   "obrix_proc1Feedback",juce::Colour(0xFFFF6B6B));
        makeLabel(proc1CutLbl, "CUTOFF");
        makeLabel(proc1ResLbl, "RESO");
        makeLabel(proc1FbLbl,  "FB");

        // Proc 2 (under Src 2)
        proc2TypeAtt   = makeComboBox(proc2TypeCB,   "obrix_proc2Type");
        proc2CutoffAtt = makeKnob    (proc2CutKnob,  "obrix_proc2Cutoff",  juce::Colour(0xFFFF6B6B));
        proc2ResoAtt   = makeKnob    (proc2ResKnob,  "obrix_proc2Reso",    juce::Colour(0xFFFF6B6B));
        proc2FbAtt     = makeKnob    (proc2FbKnob,   "obrix_proc2Feedback",juce::Colour(0xFFFF6B6B));
        makeLabel(proc2CutLbl, "CUTOFF");
        makeLabel(proc2ResLbl, "RESO");
        makeLabel(proc2FbLbl,  "FB");

        // ── Section 2 — Reef Ecology ─────────────────────────────────────────
        reefResidentAtt      = makeComboBox(reefResidentCB,    "obrix_reefResident");
        residentStrengthAtt  = makeKnob    (residentStrKnob,   "obrix_residentStrength");
        competitionAtt       = makeKnob    (competitionKnob,   "obrix_competitionStrength",
                                            juce::Colour(0xFFFF6B6B));
        symbiosisAtt         = makeKnob    (symbiosisKnob,     "obrix_symbiosisStrength",
                                            juce::Colour(0xFF48CAE4));
        makeLabel(residentStrLbl,  "STRENGTH");
        makeLabel(competitionLbl,  "COMPETE");
        makeLabel(symbiosisLbl,    "SYMBIOSIS");
        // Ecology indicator is drawn in paint()

        // ── Section 3 — Harmonic Field ───────────────────────────────────────
        fieldStrengthAtt  = makeKnob    (fieldStrengthKnob, "obrix_fieldStrength");
        fieldPolarityAtt  = makeButton  (fieldPolarityBtn,  "obrix_fieldPolarity",
                                         "ATTRACTOR", "REPULSOR");
        fieldRateAtt      = makeKnob    (fieldRateKnob,     "obrix_fieldRate");
        fieldPrimeLimitAtt= makeComboBox(fieldPrimeLimitCB, "obrix_fieldPrimeLimit");
        makeLabel(fieldStrLbl,  "STRENGTH");
        makeLabel(fieldRateLbl, "RATE");
        makeLabel(fieldPrimeLbl,"PRIME LIMIT");

        // ── Section 4 — Environmental ────────────────────────────────────────
        envTempAtt      = makeKnob(envTempKnob,     "obrix_envTemp",     juce::Colour(0xFFE9A84A));
        envPressureAtt  = makeKnob(envPressureKnob, "obrix_envPressure", juce::Colour(0xFF48CAE4));
        envCurrentAtt   = makeKnob(envCurrentKnob,  "obrix_envCurrent",  juce::Colour(0xFF00FF41));
        envTurbidityAtt = makeKnob(envTurbidityKnob,"obrix_envTurbidity",juce::Colour(0xFF9E9B97));
        makeLabel(envTempLbl,     "TEMP");
        makeLabel(envPressureLbl, "PRESSURE");
        makeLabel(envCurrentLbl,  "CURRENT");
        makeLabel(envTurbidityLbl,"TURBIDITY");

        // ── Section 5a — FX Mode ─────────────────────────────────────────────
        fxModeAtt = makeComboBox(fxModeCB, "obrix_fxMode");

        // FX slots (3 × type/mix/param)
        fx1TypeAtt  = makeComboBox(fx1TypeCB,   "obrix_fx1Type");
        fx1MixAtt   = makeKnob   (fx1MixKnob,  "obrix_fx1Mix",   juce::Colour(0xFFBF40FF));
        fx1ParamAtt = makeKnob   (fx1ParamKnob,"obrix_fx1Param", juce::Colour(0xFFBF40FF));
        fx2TypeAtt  = makeComboBox(fx2TypeCB,   "obrix_fx2Type");
        fx2MixAtt   = makeKnob   (fx2MixKnob,  "obrix_fx2Mix",   juce::Colour(0xFFBF40FF));
        fx2ParamAtt = makeKnob   (fx2ParamKnob,"obrix_fx2Param", juce::Colour(0xFFBF40FF));
        fx3TypeAtt  = makeComboBox(fx3TypeCB,   "obrix_fx3Type");
        fx3MixAtt   = makeKnob   (fx3MixKnob,  "obrix_fx3Mix",   juce::Colour(0xFFBF40FF));
        fx3ParamAtt = makeKnob   (fx3ParamKnob,"obrix_fx3Param", juce::Colour(0xFFBF40FF));
        makeLabel(fx1MixLbl,  "MIX");  makeLabel(fx1ParamLbl,  "PARAM");
        makeLabel(fx2MixLbl,  "MIX");  makeLabel(fx2ParamLbl,  "PARAM");
        makeLabel(fx3MixLbl,  "MIX");  makeLabel(fx3ParamLbl,  "PARAM");

        // ── Section 5b — Stateful Synthesis ──────────────────────────────────
        stressDecayAtt = makeKnob  (stressDecayKnob, "obrix_stressDecay", juce::Colour(0xFFE9A84A));
        bleachRateAtt  = makeKnob  (bleachRateKnob,  "obrix_bleachRate",  juce::Colour(0xFFF0EDE8));
        stateResetAtt  = makeKnob  (stateResetKnob,  "obrix_stateReset",  juce::Colour(0xFF9E9B97));
        makeLabel(stressDecayLbl, "STRESS\nDECAY");
        makeLabel(bleachRateLbl,  "BLEACH\nRATE");
        makeLabel(stateResetLbl,  "RESET");

        // ── Section 6 — Amp Envelope ─────────────────────────────────────────
        ampAttAtt  = makeKnob(ampAttKnob, "obrix_ampAttack",  juce::Colour(0xFF00FF41));
        ampDecAtt  = makeKnob(ampDecKnob, "obrix_ampDecay",   juce::Colour(0xFF00FF41));
        ampSusAtt  = makeKnob(ampSusKnob, "obrix_ampSustain", juce::Colour(0xFF00FF41));
        ampRelAtt  = makeKnob(ampRelKnob, "obrix_ampRelease", juce::Colour(0xFF00FF41));
        makeLabel(ampAttLbl, "ATK");
        makeLabel(ampDecLbl, "DEC");
        makeLabel(ampSusLbl, "SUS");
        makeLabel(ampRelLbl, "REL");

        // ── Section 7 — Modulators (4 slots) ─────────────────────────────────
        for (int i = 0; i < 4; ++i)
        {
            juce::String n = juce::String(i + 1);
            modTypeAtts  [i] = makeComboBox(modTypeCBs  [i], "obrix_mod" + n + "Type");
            modTargetAtts[i] = makeComboBox(modTargetCBs[i], "obrix_mod" + n + "Target");
            modDepthAtts [i] = makeKnob    (modDepthKnobs[i],"obrix_mod" + n + "Depth");
            modRateAtts  [i] = makeKnob    (modRateKnobs [i],"obrix_mod" + n + "Rate");
            makeLabel(modDepthLbls[i], "DEPTH");
            makeLabel(modRateLbls [i], "RATE");
        }

        // ── Section 8 — Proc 3 (post-mix processor) ──────────────────────────
        proc3TypeAtt   = makeComboBox(proc3TypeCB,  "obrix_proc3Type");
        proc3CutoffAtt = makeKnob    (proc3CutKnob, "obrix_proc3Cutoff", juce::Colour(0xFFFF6B6B));
        proc3ResoAtt   = makeKnob    (proc3ResKnob, "obrix_proc3Reso",   juce::Colour(0xFFFF6B6B));
        makeLabel(proc3CutLbl, "CUTOFF");
        makeLabel(proc3ResLbl, "RESO");

        // ── Section 9 — Other / Global params ────────────────────────────────
        levelAtt        = makeKnob    (levelKnob,        "obrix_level");
        polyphonyAtt    = makeComboBox(polyphonyCB,      "obrix_polyphony");
        pitchBendAtt    = makeKnob    (pitchBendKnob,    "obrix_pitchBendRange");
        glideTimeAtt    = makeKnob    (glideTimeKnob,    "obrix_glideTime");
        gestureTypeAtt  = makeComboBox(gestureTypeCB,    "obrix_gestureType");
        flashTriggerAtt = makeKnob    (flashTriggerKnob, "obrix_flashTrigger",
                                       juce::Colour(kReefJade));
        wtBankAtt       = makeComboBox(wtBankCB,         "obrix_wtBank");
        unisonDetuneAtt = makeKnob    (unisonDetuneKnob, "obrix_unisonDetune");
        driftRateAtt    = makeKnob    (driftRateKnob,    "obrix_driftRate",  juce::Colour(0xFF7FDBCA));
        driftDepthAtt   = makeKnob    (driftDepthKnob,   "obrix_driftDepth", juce::Colour(0xFF7FDBCA));
        journeyModeAtt  = makeKnob    (journeyModeKnob,  "obrix_journeyMode");
        distanceAtt     = makeKnob    (distanceKnob,     "obrix_distance");
        airAtt          = makeKnob    (airKnob,          "obrix_air");
        macroCharAtt    = makeKnob    (macroCharKnob,    "obrix_macroCharacter", juce::Colour(GalleryColors::xoGold));
        macroMoveAtt    = makeKnob    (macroMoveKnob,    "obrix_macroMovement",  juce::Colour(GalleryColors::xoGold));
        macroCoupAtt    = makeKnob    (macroCoupKnob,    "obrix_macroCoupling",  juce::Colour(GalleryColors::xoGold));
        macroSpaceAtt   = makeKnob    (macroSpaceKnob,   "obrix_macroSpace",     juce::Colour(GalleryColors::xoGold));

        makeLabel(levelLbl,       "LEVEL");
        makeLabel(pitchBendLbl,   "BEND");
        makeLabel(glideTimeLbl,   "GLIDE");
        makeLabel(flashTrigLbl,   "FLASH");
        makeLabel(unisonDetLbl,   "UNISON");
        makeLabel(driftRateLbl,   "DRIFT RATE");
        makeLabel(driftDepthLbl,  "DRIFT DEPTH");
        makeLabel(journeyLbl,     "JOURNEY");
        makeLabel(distanceLbl,    "DISTANCE");
        makeLabel(airLbl,         "AIR");
        makeLabel(macroCharLbl,   "CHARACTER");
        makeLabel(macroMoveLbl,   "MOVEMENT");
        makeLabel(macroCoupLbl,   "COUPLING");
        makeLabel(macroSpaceLbl,  "SPACE");
    }

    ~ObrixDetailPanel() override
    {
        // Remove MIDI learn mouse listeners from all child components before
        // the unique_ptrs in midiLearnListeners are destroyed.  Without this,
        // JUCE's component tree teardown may try to deliver a final mouse event
        // to a listener whose knob has already been destroyed, causing a dangling
        // raw pointer dereference.  Walking all child components is safe here
        // because MidiLearnMouseListener stores a paramId, not the knob pointer.
        for (auto& ml : midiLearnListeners)
        {
            if (ml)
            {
                for (int i = getNumChildComponents() - 1; i >= 0; --i)
                    getChildComponent(i)->removeMouseListener(ml.get());
            }
        }
    }

    // ── Public layout query ──────────────────────────────────────────────────
    // Returns the total pixel height this panel needs for a given width.
    // The parent Viewport calls this to size the content component.
    int getRequiredHeight(int /*availableWidth*/) const
    {
        // Heights for each major band:
        int h = kPad;
        h += kSectionHeaderH;                              // "SOURCES" header
        h += kBrickHeaderH + kBrickExpandedH * 2 + kPad;  // Brick Stack (both open)
        h += kPad + kSectionHeaderH + 120;                 // Reef Ecology
        h += kPad + kSectionHeaderH + 80;                  // Harmonic Field
        h += kPad + kSectionHeaderH + 80;                  // Environmental
        h += kPad + kSectionHeaderH + 80;                  // Amp Envelope
        h += kPad + kSectionHeaderH + 200;                 // Modulators (4 slots × ~50pt)
        h += kPad + kSectionHeaderH + 80;                  // Proc 3
        h += kPad + kSectionHeaderH + 80;                  // FX Mode + slots
        h += kPad + kSectionHeaderH + 80;                  // Stateful Synthesis
        h += kPad + kSectionHeaderH + 160;                 // Other/Global
        h += kPad;
        return h;
    }

    void resized() override
    {
        // Recompute compact mode based on available height.
        if (auto* vp = findParentComponentOfClass<juce::Viewport>())
            compactMode = (vp->getHeight() <= 500);
        else
            compactMode = false;

        int y = kPad;
        const int W = getWidth();
        const int knobW = kKnobSize;

        // Convenience: place a knob+label column starting at cx, cy.
        auto placeKnob = [&](GalleryKnob& knob, juce::Label& lbl, int cx, int cy, int w) {
            knob.setBounds(cx + (w - knobW) / 2, cy, knobW, knobW);
            lbl.setBounds(cx, cy + knobW + 1, w, 13);
        };

        // ── SOURCES section header ────────────────────────────────────────
        y += kSectionHeaderH; // header drawn in paint()

        // ── Brick 1 header toggle ─────────────────────────────────────────
        brick1Header.setBounds(kPad, y, W - kPad * 2, kBrickHeaderH);
        y += kBrickHeaderH + 2;

        // ── Brick 1 expanded body ─────────────────────────────────────────
        brick1BodyY = y;
        brick1BodyH = brick1Header.getToggleState() ? kBrickExpandedH : 0;
        if (brick1Header.getToggleState())
        {
            const int comboH = 22;
            const int bodyX  = kPad + 8;
            const int bodyW  = W - bodyX - kPad - 8;
            // Row 1: type combo (full width)
            src1TypeCB.setBounds(bodyX, y, bodyW, comboH);
            y += comboH + 4;
            // Row 2: tune + PW knobs
            int col = bodyW / 4;
            placeKnob(src1TuneKnob, src1TuneLbl, bodyX,         y, col);
            placeKnob(src1PWKnob,   src1PWLbl,   bodyX + col,   y, col);
            // Proc1 section under src1
            proc1TypeCB.setBounds(bodyX + col * 2, y, col * 2, comboH);
            y += comboH + 4;
            placeKnob(proc1CutKnob, proc1CutLbl, bodyX + col * 2,          y, col);
            placeKnob(proc1ResKnob, proc1ResLbl, bodyX + col * 3,          y, col);
            y += knobW + 14 + 4;
            placeKnob(proc1FbKnob,  proc1FbLbl,  bodyX + col * 2, y, col);
            y += knobW + 14 + kPad;
        }
        else
        {
            // Hidden — set zero bounds
            src1TypeCB.setBounds(0, 0, 0, 0);
            src1TuneKnob.setBounds(0, 0, 0, 0); src1TuneLbl.setBounds(0,0,0,0);
            src1PWKnob.setBounds(0, 0, 0, 0);   src1PWLbl.setBounds(0,0,0,0);
            proc1TypeCB.setBounds(0, 0, 0, 0);
            proc1CutKnob.setBounds(0,0,0,0); proc1CutLbl.setBounds(0,0,0,0);
            proc1ResKnob.setBounds(0,0,0,0); proc1ResLbl.setBounds(0,0,0,0);
            proc1FbKnob.setBounds(0,0,0,0);  proc1FbLbl.setBounds(0,0,0,0);
        }

        // Shared between bricks — src mix and FM depth
        {
            const int colW2 = (W - kPad * 2) / 4;
            placeKnob(srcMixKnob,  srcMixLbl,  kPad,         y, colW2);
            placeKnob(fmDepthKnob, fmDepthLbl, kPad + colW2, y, colW2);
            y += knobW + 14 + kPad;
        }

        // ── Brick 2 header toggle ─────────────────────────────────────────
        brick2Header.setBounds(kPad, y, W - kPad * 2, kBrickHeaderH);
        y += kBrickHeaderH + 2;

        // ── Brick 2 expanded body ─────────────────────────────────────────
        brick2BodyY = y;
        brick2BodyH = brick2Header.getToggleState() ? kBrickExpandedH : 0;
        if (brick2Header.getToggleState())
        {
            const int comboH = 22;
            const int bodyX  = kPad + 8;
            const int bodyW  = W - bodyX - kPad - 8;
            int col = bodyW / 4;

            src2TypeCB.setBounds(bodyX, y, bodyW, comboH);
            y += comboH + 4;
            placeKnob(src2TuneKnob, src2TuneLbl, bodyX,         y, col);
            placeKnob(src2PWKnob,   src2PWLbl,   bodyX + col,   y, col);
            proc2TypeCB.setBounds(bodyX + col * 2, y, col * 2, comboH);
            y += comboH + 4;
            placeKnob(proc2CutKnob, proc2CutLbl, bodyX + col * 2, y, col);
            placeKnob(proc2ResKnob, proc2ResLbl, bodyX + col * 3, y, col);
            y += knobW + 14 + 4;
            placeKnob(proc2FbKnob,  proc2FbLbl,  bodyX + col * 2, y, col);
            y += knobW + 14 + kPad;
        }
        else
        {
            src2TypeCB.setBounds(0,0,0,0);
            src2TuneKnob.setBounds(0,0,0,0); src2TuneLbl.setBounds(0,0,0,0);
            src2PWKnob.setBounds(0,0,0,0);   src2PWLbl.setBounds(0,0,0,0);
            proc2TypeCB.setBounds(0,0,0,0);
            proc2CutKnob.setBounds(0,0,0,0); proc2CutLbl.setBounds(0,0,0,0);
            proc2ResKnob.setBounds(0,0,0,0); proc2ResLbl.setBounds(0,0,0,0);
            proc2FbKnob.setBounds(0,0,0,0);  proc2FbLbl.setBounds(0,0,0,0);
        }

        // ── REEF ECOLOGY section ──────────────────────────────────────────
        y += kSectionHeaderH;
        reefEcologyHeaderY = y - kSectionHeaderH;
        {
            const int comboH = 22;
            const int indicW = 24; // ecology color indicator dot area
            const int x0 = kPad;
            const int aW = W - kPad * 2;
            // Row 1: resident combo + ecology indicator
            reefResidentCB.setBounds(x0, y, aW - indicW - 6, comboH);
            reefEcologyIndicatorBounds = { W - kPad - indicW, y, indicW, comboH };
            y += comboH + 4;
            // Row 2: knobs
            const int colW4 = aW / 4;
            placeKnob(residentStrKnob, residentStrLbl, x0,            y, colW4);
            placeKnob(competitionKnob, competitionLbl, x0 + colW4,    y, colW4);
            placeKnob(symbiosisKnob,   symbiosisLbl,   x0 + colW4*2,  y, colW4);
            y += knobW + 14 + kPad;
        }

        // ── HARMONIC FIELD section ────────────────────────────────────────
        y += kSectionHeaderH;
        harmonicFieldHeaderY = y - kSectionHeaderH;
        {
            const int comboH = 22;
            const int aW  = W - kPad * 2;
            const int x0  = kPad;
            const int colW4 = aW / 4;
            placeKnob(fieldStrengthKnob, fieldStrLbl,   x0,           y, colW4);
            fieldPolarityBtn.setBounds(x0 + colW4, y, colW4, knobW);
            placeKnob(fieldRateKnob,     fieldRateLbl,  x0 + colW4*2, y, colW4);
            fieldPrimeLimitCB.setBounds(x0 + colW4*3, y, colW4, comboH);
            fieldPrimeLbl.setBounds(x0 + colW4*3, y + comboH + 2, colW4, 13);
            y += knobW + 14 + kPad;
        }

        // ── ENVIRONMENTAL section ─────────────────────────────────────────
        y += kSectionHeaderH;
        environmentalHeaderY = y - kSectionHeaderH;
        {
            const int aW    = W - kPad * 2;
            const int x0    = kPad;
            const int colW4 = aW / 4;
            placeKnob(envTempKnob,     envTempLbl,     x0,           y, colW4);
            placeKnob(envPressureKnob, envPressureLbl, x0 + colW4,   y, colW4);
            placeKnob(envCurrentKnob,  envCurrentLbl,  x0 + colW4*2, y, colW4);
            placeKnob(envTurbidityKnob,envTurbidityLbl,x0 + colW4*3, y, colW4);
            y += knobW + 14 + kPad;
        }

        // ── AMP ENVELOPE section ──────────────────────────────────────────
        y += kSectionHeaderH;
        ampEnvHeaderY = y - kSectionHeaderH;
        {
            const int aW    = W - kPad * 2;
            const int x0    = kPad;
            const int colW4 = aW / 4;
            placeKnob(ampAttKnob, ampAttLbl, x0,            y, colW4);
            placeKnob(ampDecKnob, ampDecLbl, x0 + colW4,    y, colW4);
            placeKnob(ampSusKnob, ampSusLbl, x0 + colW4*2,  y, colW4);
            placeKnob(ampRelKnob, ampRelLbl, x0 + colW4*3,  y, colW4);
            y += knobW + 14 + kPad;
        }

        // ── MODULATORS section (4 slots) ──────────────────────────────────
        y += kSectionHeaderH;
        modulatorsHeaderY = y - kSectionHeaderH;
        {
            const int comboH = 20;
            const int aW     = W - kPad * 2;
            const int x0     = kPad;
            for (int i = 0; i < 4; ++i)
            {
                const int colW2 = aW / 4;
                // Each mod slot: type combo | target combo | depth knob | rate knob
                modTypeCBs  [i].setBounds(x0,            y, colW2, comboH);
                modTargetCBs[i].setBounds(x0 + colW2,    y, colW2, comboH);
                placeKnob(modDepthKnobs[i], modDepthLbls[i], x0 + colW2*2, y, colW2);
                placeKnob(modRateKnobs [i], modRateLbls [i], x0 + colW2*3, y, colW2);
                y += juce::jmax(comboH, knobW) + 14 + 4;
            }
            y += kPad;
        }

        // ── PROC 3 (post-mix) section ─────────────────────────────────────
        y += kSectionHeaderH;
        proc3HeaderY = y - kSectionHeaderH;
        {
            const int comboH = 22;
            const int aW     = W - kPad * 2;
            const int x0     = kPad;
            const int colW3  = aW / 3;
            proc3TypeCB.setBounds(x0, y, colW3, comboH);
            placeKnob(proc3CutKnob, proc3CutLbl, x0 + colW3,   y, colW3);
            placeKnob(proc3ResKnob, proc3ResLbl, x0 + colW3*2, y, colW3);
            y += knobW + 14 + kPad;
        }

        // ── FX section ────────────────────────────────────────────────────
        y += kSectionHeaderH;
        fxHeaderY = y - kSectionHeaderH;
        {
            const int btnH   = 22;
            const int comboH = 22;
            const int aW     = W - kPad * 2;
            const int x0     = kPad;
            // FX mode combo box
            fxModeCB.setBounds(x0, y, 90, btnH);
            y += btnH + 4;
            // FX slots in 3 rows
            for (int i = 0; i < 3; ++i)
            {
                juce::ComboBox& typeCB   = (i==0)?fx1TypeCB  :(i==1)?fx2TypeCB  :fx3TypeCB;
                GalleryKnob&    mixKnob  = (i==0)?fx1MixKnob :(i==1)?fx2MixKnob :fx3MixKnob;
                GalleryKnob&    prmKnob  = (i==0)?fx1ParamKnob:(i==1)?fx2ParamKnob:fx3ParamKnob;
                juce::Label&    mixLbl   = (i==0)?fx1MixLbl  :(i==1)?fx2MixLbl  :fx3MixLbl;
                juce::Label&    prmLbl   = (i==0)?fx1ParamLbl:(i==1)?fx2ParamLbl:fx3ParamLbl;

                const int colW3 = aW / 3;
                typeCB.setBounds(x0, y, colW3, comboH);
                placeKnob(mixKnob, mixLbl, x0 + colW3,   y, colW3);
                placeKnob(prmKnob, prmLbl, x0 + colW3*2, y, colW3);
                y += juce::jmax(comboH, knobW) + 14 + 4;
            }
            y += kPad;
        }

        // ── STATEFUL SYNTHESIS section ────────────────────────────────────
        y += kSectionHeaderH;
        statefulHeaderY = y - kSectionHeaderH;
        {
            const int aW    = W - kPad * 2;
            const int x0    = kPad;
            const int colW3 = aW / 3;
            placeKnob(stressDecayKnob, stressDecayLbl, x0,           y, colW3);
            placeKnob(bleachRateKnob,  bleachRateLbl,  x0 + colW3,   y, colW3);
            placeKnob(stateResetKnob,  stateResetLbl,  x0 + colW3*2, y, colW3);
            y += knobW + 14 + kPad;
        }

        // ── OTHER / GLOBAL section ────────────────────────────────────────
        y += kSectionHeaderH;
        otherHeaderY = y - kSectionHeaderH;
        {
            const int comboH = 22;
            const int aW     = W - kPad * 2;
            const int x0     = kPad;
            const int colW4  = aW / 4;

            // Macros row
            placeKnob(macroCharKnob, macroCharLbl, x0,            y, colW4);
            placeKnob(macroMoveKnob, macroMoveLbl, x0 + colW4,    y, colW4);
            placeKnob(macroCoupKnob, macroCoupLbl, x0 + colW4*2,  y, colW4);
            placeKnob(macroSpaceKnob,macroSpaceLbl,x0 + colW4*3,  y, colW4);
            y += knobW + 14 + 4;

            // Level + polyphony + pitch bend + glide
            placeKnob(levelKnob,     levelLbl,     x0,            y, colW4);
            polyphonyCB.setBounds(x0 + colW4, y, colW4, comboH);
            placeKnob(pitchBendKnob, pitchBendLbl, x0 + colW4*2,  y, colW4);
            placeKnob(glideTimeKnob, glideTimeLbl, x0 + colW4*3,  y, colW4);
            y += knobW + 14 + 4;

            // Gesture + wavetable + unison + journey
            gestureTypeCB.setBounds(x0, y, colW4, comboH);
            placeKnob(flashTriggerKnob, flashTrigLbl, x0 + colW4,    y, colW4);
            wtBankCB.setBounds(x0 + colW4*2, y, colW4, comboH);
            placeKnob(unisonDetuneKnob, unisonDetLbl, x0 + colW4*3,  y, colW4);
            y += knobW + 14 + 4;

            // Drift + journey + distance + air
            placeKnob(driftRateKnob,  driftRateLbl,  x0,           y, colW4);
            placeKnob(driftDepthKnob, driftDepthLbl, x0 + colW4,   y, colW4);
            placeKnob(journeyModeKnob,journeyLbl,    x0 + colW4*2, y, colW4);
            placeKnob(distanceKnob,   distanceLbl,   x0 + colW4*3, y, colW4);
            y += knobW + 14 + 4;

            // Air (last straggler)
            placeKnob(airKnob, airLbl, x0, y, colW4);
            y += knobW + 14 + kPad;
        }

        // Store final content height for parent
        totalContentHeight = y + kPad;
    }

    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        const juce::Colour accent = juce::Colour(kReefJade);

        g.fillAll(get(slotBg()));

        // Section header font cached as a static local — constructed once,
        // reused across all 9 section header calls per paint().
        static const auto kSectionFont = juce::Font(juce::FontOptions{}.withTypeface(GalleryFonts::spaceGroteskBold()).withHeight(10.0f));

        // Helper: draw a section header strip
        auto drawSectionHeader = [&](int headerY, const juce::String& name,
                                      juce::Colour col)
        {
            g.setColour(col.withAlpha(0.06f));
            g.fillRect(0, headerY, getWidth(), kSectionHeaderH);
            g.setColour(col.withAlpha(0.20f));
            g.drawHorizontalLine(headerY, 4.0f, (float)(getWidth() - 4));

            const int dotSize = 6;
            const int dotX    = 10;
            const int dotY    = headerY + (kSectionHeaderH - dotSize) / 2;
            g.setColour(col);
            g.fillEllipse((float)dotX, (float)dotY, (float)dotSize, (float)dotSize);

            g.setColour(col.brighter(0.15f));
            g.setFont(kSectionFont);
            g.drawText(name,
                       dotX + dotSize + 6, headerY,
                       getWidth() - dotX - dotSize - 16, kSectionHeaderH,
                       juce::Justification::centredLeft);
        };

        // ── Section headers (y positions tracked during resized()) ────────
        drawSectionHeader(kPad,               "SOURCES",           accent);
        drawSectionHeader(reefEcologyHeaderY,  "REEF ECOLOGY",      juce::Colour(0xFF48CAE4));
        drawSectionHeader(harmonicFieldHeaderY,"HARMONIC FIELD",    accent.brighter(0.2f));
        drawSectionHeader(environmentalHeaderY,"ENVIRONMENT",       juce::Colour(0xFFE9A84A));
        drawSectionHeader(ampEnvHeaderY,       "AMP ENVELOPE",      juce::Colour(0xFF00FF41));
        drawSectionHeader(modulatorsHeaderY,   "MODULATORS",        juce::Colour(0xFFBF40FF));
        drawSectionHeader(proc3HeaderY,        "POST-MIX PROC",     juce::Colour(0xFFFF6B6B));
        drawSectionHeader(fxHeaderY,           "FX",                juce::Colour(0xFFBF40FF));
        drawSectionHeader(statefulHeaderY,     "STATEFUL SYNTHESIS",juce::Colour(0xFFE9A84A));
        drawSectionHeader(otherHeaderY,        "GLOBAL",            juce::Colour(GalleryColors::xoGold));

        // ── Brick inlay backgrounds ────────────────────────────────────────
        // Src1
        if (brick1Header.getToggleState() && brick1BodyH > 0)
        {
            juce::Rectangle<int> inlay(kPad + 4, brick1BodyY - 2,
                                        getWidth() - kPad * 2 - 8, brick1BodyH + 4);
            g.setColour(accent.withAlpha(0.05f));
            g.fillRoundedRectangle(inlay.toFloat(), 4.0f);
            g.setColour(accent.withAlpha(0.20f));
            g.drawRoundedRectangle(inlay.toFloat(), 4.0f, 1.0f);
        }
        // Src2
        if (brick2Header.getToggleState() && brick2BodyH > 0)
        {
            juce::Rectangle<int> inlay(kPad + 4, brick2BodyY - 2,
                                        getWidth() - kPad * 2 - 8, brick2BodyH + 4);
            g.setColour(juce::Colour(kReefJade).withAlpha(0.03f));
            g.fillRoundedRectangle(inlay.toFloat(), 4.0f);
            g.setColour(accent.withAlpha(0.14f));
            g.drawRoundedRectangle(inlay.toFloat(), 4.0f, 1.0f);
        }

        // ── Ecology state indicator ───────────────────────────────────────
        // Colors: Reef Jade = healthy, amber = stressed, near-white = bleached.
        // We do a lightweight read of the stress/bleach levels if available by
        // polling the APVTS stateReset param direction as a proxy, otherwise
        // we color by reefResident mode:
        //   Off/Symbiote → Reef Jade, Competitor → amber, Parasite → amber-red
        {
            if (!reefEcologyIndicatorBounds.isEmpty())
            {
                auto indicBounds = reefEcologyIndicatorBounds.toFloat().reduced(4.0f);
                float cx = indicBounds.getCentreX();
                float cy = indicBounds.getCentreY();
                float r  = juce::jmin(indicBounds.getWidth(), indicBounds.getHeight()) * 0.5f;

                // Read resident mode (0=Off, 1=Competitor, 2=Symbiote, 3=Parasite)
                int resident = 0;
                if (auto* p = apvts.getParameter("obrix_reefResident"))
                    if (auto* cp = dynamic_cast<juce::AudioParameterChoice*>(p))
                        resident = cp->getIndex();

                juce::Colour indicCol;
                switch (resident)
                {
                    case 0: indicCol = juce::Colour(kReefJade);  break;  // Off — healthy
                    case 1: indicCol = juce::Colour(kAmberColor);break;  // Competitor — stressed
                    case 2: indicCol = juce::Colour(kReefJade).brighter(0.4f); break; // Symbiote
                    case 3: indicCol = juce::Colour(0xFFFF6B6B); break;  // Parasite — danger
                    default:indicCol = juce::Colour(kReefJade);  break;
                }

                // Glow
                g.setColour(indicCol.withAlpha(0.18f));
                g.fillEllipse(cx - r - 3.0f, cy - r - 3.0f, (r + 3.0f) * 2.0f, (r + 3.0f) * 2.0f);
                // Fill
                g.setColour(indicCol);
                g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
                // Rim
                g.setColour(indicCol.darker(0.2f));
                g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);
            }
        }

        // ── Field polarity label update ───────────────────────────────────
        // Update the toggle button text to reflect current state.
        {
            bool polOn = fieldPolarityBtn.getToggleState();
            fieldPolarityBtn.setButtonText(polOn ? "REPULSOR" : "ATTRACTOR");
        }

        // fxModeCB is a ComboBox — label is shown natively; no manual update needed.
    }

    // ── Total content height (filled by resized()) ───────────────────────────
    int getTotalContentHeight() const noexcept { return totalContentHeight; }

private:
    // ── Smart accordion toggle logic ─────────────────────────────────────────
    // brickIdx: 0 = Src1, 1 = Src2
    void onBrickHeaderClicked(int brickIdx)
    {
        if (compactMode)
        {
            // Close the OTHER brick when this one opens
            if (brickIdx == 0 && brick1Header.getToggleState())
                brick2Header.setToggleState(false, juce::dontSendNotification);
            else if (brickIdx == 1 && brick2Header.getToggleState())
                brick1Header.setToggleState(false, juce::dontSendNotification);
        }
        resized();
        repaint();
        // Notify parent viewport to resize content
        if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        {
            auto* content = vp->getViewedComponent();
            if (content == this)
                setSize(getWidth(), getRequiredHeight(getWidth()));
        }
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Constants
    // ──────────────────────────────────────────────────────────────────────────
    static constexpr int kPad             = 10;
    static constexpr int kSectionHeaderH  = 22;
    static constexpr int kBrickHeaderH    = 26;
    static constexpr int kBrickExpandedH  = 110; // approximate per-brick body height
    static constexpr int kKnobSize        = 44;

    // ──────────────────────────────────────────────────────────────────────────
    // State
    // ──────────────────────────────────────────────────────────────────────────
    juce::AudioProcessorValueTreeState& apvts;
    MIDILearnManager*                   learnManager = nullptr;
    bool                                compactMode  = false;
    int                                 totalContentHeight = 1200;

    // Section header Y positions — recorded during resized(), used in paint()
    int reefEcologyHeaderY   = 0;
    int harmonicFieldHeaderY = 0;
    int environmentalHeaderY = 0;
    int ampEnvHeaderY        = 0;
    int modulatorsHeaderY    = 0;
    int proc3HeaderY         = 0;
    int fxHeaderY            = 0;
    int statefulHeaderY      = 0;
    int otherHeaderY         = 0;

    // Brick body geometry — recorded during resized(), used in paint()
    int brick1BodyY = 0, brick1BodyH = 0;
    int brick2BodyY = 0, brick2BodyH = 0;

    // Ecology indicator rect
    juce::Rectangle<int> reefEcologyIndicatorBounds;

    // ──────────────────────────────────────────────────────────────────────────
    // Controls (MUST be declared before attachments; listeners declared AFTER
    // attachments so they are destroyed first — preserving safe-pointer order)
    // ──────────────────────────────────────────────────────────────────────────

    // ── Section 1: Brick Stack ────────────────────────────────────────────────
    juce::TextButton brick1Header, brick2Header;

    // Src 1
    juce::ComboBox src1TypeCB;
    GalleryKnob    src1TuneKnob, src1PWKnob;
    juce::Label    src1TuneLbl, src1PWLbl;

    // Src 2
    juce::ComboBox src2TypeCB;
    GalleryKnob    src2TuneKnob, src2PWKnob;
    juce::Label    src2TuneLbl, src2PWLbl;

    // Shared source
    GalleryKnob    srcMixKnob, fmDepthKnob;
    juce::Label    srcMixLbl, fmDepthLbl;

    // Proc 1 (under Src 1)
    juce::ComboBox proc1TypeCB;
    GalleryKnob    proc1CutKnob, proc1ResKnob, proc1FbKnob;
    juce::Label    proc1CutLbl, proc1ResLbl, proc1FbLbl;

    // Proc 2 (under Src 2)
    juce::ComboBox proc2TypeCB;
    GalleryKnob    proc2CutKnob, proc2ResKnob, proc2FbKnob;
    juce::Label    proc2CutLbl, proc2ResLbl, proc2FbLbl;

    // ── Section 2: Reef Ecology ───────────────────────────────────────────────
    juce::ComboBox reefResidentCB;
    GalleryKnob    residentStrKnob, competitionKnob, symbiosisKnob;
    juce::Label    residentStrLbl, competitionLbl, symbiosisLbl;

    // ── Section 3: Harmonic Field ─────────────────────────────────────────────
    GalleryKnob    fieldStrengthKnob, fieldRateKnob;
    juce::TextButton fieldPolarityBtn;
    juce::ComboBox fieldPrimeLimitCB;
    juce::Label    fieldStrLbl, fieldRateLbl, fieldPrimeLbl;

    // ── Section 4: Environmental ──────────────────────────────────────────────
    GalleryKnob    envTempKnob, envPressureKnob, envCurrentKnob, envTurbidityKnob;
    juce::Label    envTempLbl, envPressureLbl, envCurrentLbl, envTurbidityLbl;

    // ── Section 5: Amp Envelope ───────────────────────────────────────────────
    GalleryKnob    ampAttKnob, ampDecKnob, ampSusKnob, ampRelKnob;
    juce::Label    ampAttLbl, ampDecLbl, ampSusLbl, ampRelLbl;

    // ── Section 6: Modulators (4 slots) ──────────────────────────────────────
    std::array<juce::ComboBox, 4> modTypeCBs, modTargetCBs;
    std::array<GalleryKnob,   4> modDepthKnobs, modRateKnobs;
    std::array<juce::Label,   4> modDepthLbls, modRateLbls;

    // ── Section 7: Proc 3 ─────────────────────────────────────────────────────
    juce::ComboBox proc3TypeCB;
    GalleryKnob    proc3CutKnob, proc3ResKnob;
    juce::Label    proc3CutLbl, proc3ResLbl;

    // ── Section 8: FX ─────────────────────────────────────────────────────────
    juce::ComboBox   fxModeCB;
    juce::ComboBox   fx1TypeCB, fx2TypeCB, fx3TypeCB;
    GalleryKnob      fx1MixKnob, fx1ParamKnob;
    GalleryKnob      fx2MixKnob, fx2ParamKnob;
    GalleryKnob      fx3MixKnob, fx3ParamKnob;
    juce::Label      fx1MixLbl, fx1ParamLbl;
    juce::Label      fx2MixLbl, fx2ParamLbl;
    juce::Label      fx3MixLbl, fx3ParamLbl;

    // ── Section 9: Stateful Synthesis ─────────────────────────────────────────
    GalleryKnob    stressDecayKnob, bleachRateKnob, stateResetKnob;
    juce::Label    stressDecayLbl, bleachRateLbl, stateResetLbl;

    // ── Section 10: Global / Other ────────────────────────────────────────────
    GalleryKnob    levelKnob, pitchBendKnob, glideTimeKnob, flashTriggerKnob;
    GalleryKnob    unisonDetuneKnob, driftRateKnob, driftDepthKnob;
    GalleryKnob    journeyModeKnob, distanceKnob, airKnob;
    GalleryKnob    macroCharKnob, macroMoveKnob, macroCoupKnob, macroSpaceKnob;
    juce::ComboBox polyphonyCB, gestureTypeCB, wtBankCB;
    juce::Label    levelLbl, pitchBendLbl, glideTimeLbl, flashTrigLbl;
    juce::Label    unisonDetLbl, driftRateLbl, driftDepthLbl;
    juce::Label    journeyLbl, distanceLbl, airLbl;
    juce::Label    macroCharLbl, macroMoveLbl, macroCoupLbl, macroSpaceLbl;

    // ──────────────────────────────────────────────────────────────────────────
    // Attachments — declared AFTER all knobs/combos so they are destroyed first
    // (APVTS attachments deregister listeners; the slider must still be alive)
    // ──────────────────────────────────────────────────────────────────────────
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Src 1
    std::unique_ptr<ComboAtt>  src1TypeAtt;
    std::unique_ptr<SliderAtt> src1TuneAtt, src1PWAtt;
    // Src 2
    std::unique_ptr<ComboAtt>  src2TypeAtt;
    std::unique_ptr<SliderAtt> src2TuneAtt, src2PWAtt;
    // Shared source
    std::unique_ptr<SliderAtt> srcMixAtt, fmDepthAtt;
    // Proc 1
    std::unique_ptr<ComboAtt>  proc1TypeAtt;
    std::unique_ptr<SliderAtt> proc1CutoffAtt, proc1ResoAtt, proc1FbAtt;
    // Proc 2
    std::unique_ptr<ComboAtt>  proc2TypeAtt;
    std::unique_ptr<SliderAtt> proc2CutoffAtt, proc2ResoAtt, proc2FbAtt;
    // Reef Ecology
    std::unique_ptr<ComboAtt>  reefResidentAtt;
    std::unique_ptr<SliderAtt> residentStrengthAtt, competitionAtt, symbiosisAtt;
    // Harmonic Field
    std::unique_ptr<SliderAtt> fieldStrengthAtt, fieldRateAtt;
    std::unique_ptr<ButtonAtt> fieldPolarityAtt;
    std::unique_ptr<ComboAtt>  fieldPrimeLimitAtt;
    // Environmental
    std::unique_ptr<SliderAtt> envTempAtt, envPressureAtt, envCurrentAtt, envTurbidityAtt;
    // Amp Envelope
    std::unique_ptr<SliderAtt> ampAttAtt, ampDecAtt, ampSusAtt, ampRelAtt;
    // Modulators
    std::array<std::unique_ptr<ComboAtt>,  4> modTypeAtts, modTargetAtts;
    std::array<std::unique_ptr<SliderAtt>, 4> modDepthAtts, modRateAtts;
    // Proc 3
    std::unique_ptr<ComboAtt>  proc3TypeAtt;
    std::unique_ptr<SliderAtt> proc3CutoffAtt, proc3ResoAtt;
    // FX
    std::unique_ptr<ComboAtt>  fxModeAtt;
    std::unique_ptr<ComboAtt>  fx1TypeAtt, fx2TypeAtt, fx3TypeAtt;
    std::unique_ptr<SliderAtt> fx1MixAtt, fx1ParamAtt;
    std::unique_ptr<SliderAtt> fx2MixAtt, fx2ParamAtt;
    std::unique_ptr<SliderAtt> fx3MixAtt, fx3ParamAtt;
    // Stateful
    std::unique_ptr<SliderAtt> stressDecayAtt, bleachRateAtt, stateResetAtt;
    // Global
    std::unique_ptr<SliderAtt> levelAtt, pitchBendAtt, glideTimeAtt, flashTriggerAtt;
    std::unique_ptr<SliderAtt> unisonDetuneAtt, driftRateAtt, driftDepthAtt;
    std::unique_ptr<SliderAtt> journeyModeAtt, distanceAtt, airAtt;
    std::unique_ptr<SliderAtt> macroCharAtt, macroMoveAtt, macroCoupAtt, macroSpaceAtt;
    std::unique_ptr<ComboAtt>  polyphonyAtt, gestureTypeAtt, wtBankAtt;

    // ──────────────────────────────────────────────────────────────────────────
    // MIDI learn listeners — declared LAST so they are destroyed first
    // (before the sliders they observe)
    // ──────────────────────────────────────────────────────────────────────────
    std::vector<std::unique_ptr<MidiLearnMouseListener>> midiLearnListeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObrixDetailPanel)
};

} // namespace xoceanus
// clang-format on
