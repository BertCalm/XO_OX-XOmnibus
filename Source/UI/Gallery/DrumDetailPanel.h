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
// DrumDetailPanel — custom hierarchical UI for drum synth engines (ONSET).
//
// Replaces the generic ParameterGrid when the ONSET engine is loaded.
// Provides a drum-machine-style layout with 5 collapsible sections:
//
//   ┌──────────────────────────────────────────┐
//   │  VOICES  (collapsible section)           │
//   ├────────┬───────┬──────┬──────┬─── ... ───┤
//   │ KICK   │SNARE  │ HH-C │ HH-O │  ...      │
//   │ Decay  │ Decay │Decay │Decay │           │
//   │ Tone   │ Tone  │ Tone │ Tone │           │
//   │ Snap   │ Snap  │ Snap │ Snap │           │
//   │ Body   │ Body  │ Body │ Body │           │
//   │ Blend  │ Blend │Blend │Blend │           │
//   │ Pitch  │ Pitch │Pitch │Pitch │           │
//   │ Level  │ Level │Level │Level │           │
//   │ Pan    │ Pan   │ Pan  │ Pan  │           │
//   │ Char   │ Char  │ Char │ Char │           │
//   ├──────────────────────────────────────────┤
//   │ GLOBAL: Level | Drive | Tone             │
//   │ MACROS: Machine | Punch | Space | Mutate │
//   │ XVC: (6 coupling knobs)                  │
//   │ FX:  Delay / Reverb / LoFi               │
//   └──────────────────────────────────────────┘
//
// The voice column headers and global/macro/XVC/FX sections are collapsible.
// AlgoMode and EnvShape per-voice are combo boxes placed above the knob column.
//
// Destruction order: MIDI learn listeners → attachments → knobs/combos.
//
// Gallery code: DrumDetailPanel | ONSET prefix: perc_ | Accent: Electric Blue #0066FF
//==============================================================================

class DrumDetailPanel : public juce::Component
{
public:
    // ── Electric Blue accent (ONSET canonical color) ────────────────────────
    static constexpr uint32_t kElectricBlue = 0xFF0066FF;

    // ── Number of voices and per-voice parameter count ───────────────────────
    static constexpr int kNumVoices    = 8;
    static constexpr int kNumVoiceKnobs = 9; // decay, tone, snap, body, blend, pitch, level, pan, character

    // ── Layout constants ──────────────────────────────────────────────────────
    static constexpr int kKnobSize      = 40;   // px — slightly smaller than DrumPadGrid's 48 to fit 8 columns
    static constexpr int kLabelH        = 13;
    static constexpr int kCellW         = kKnobSize + 4;
    static constexpr int kCellH         = kKnobSize + kLabelH + 4;
    static constexpr int kHeaderH       = 20;   // voice column header height
    static constexpr int kComboH        = 20;   // combo box height
    static constexpr int kPad           = 6;    // outer/inner padding
    static constexpr int kSectionHeaderH= 24;   // global/macro/xvc/fx section header

    // ── Voice names matching OnsetEngine kVoiceCfg ───────────────────────────
    static constexpr const char* kVoiceNames[kNumVoices] = {
        "Kick", "Snare", "HH-C", "HH-O", "Clap", "Tom", "PercA", "PercB"
    };

    // ─────────────────────────────────────────────────────────────────────────
    // Constructor — eagerly creates all knobs, combo boxes, and attachments.
    // ─────────────────────────────────────────────────────────────────────────
    explicit DrumDetailPanel(XOceanusProcessor& proc, MIDILearnManager* midiLearn = nullptr)
        : apvts(proc.getAPVTS()), learnManager(midiLearn)
    {
        setFocusContainerType(juce::Component::FocusContainerType::keyboardFocusContainer);
        setTitle("ONSET Drum Synth Parameters");
        setDescription("Drum synth: 8 voices (Kick, Snare, HH-C, HH-O, Clap, Tom, PercA, PercB) "
                       "with per-voice Decay/Tone/Snap/Body/Blend/Pitch/Level/Pan/Character, "
                       "global level/drive/tone, macros, XVC coupling, and FX.");

        const juce::Colour accent = juce::Colour(kElectricBlue);

        // ── Helpers ──────────────────────────────────────────────────────────

        auto makeKnob = [&](GalleryKnob& knob,
                             const juce::String& pid,
                             juce::Colour fill = juce::Colour(kElectricBlue))
            -> std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        {
            knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knob.setColour(juce::Slider::rotarySliderFillColourId, fill);
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(pid)))
                knob.setTooltip(rp->getName(64));
            A11y::setup(knob, pid, "ONSET: " + pid, true);
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
                         accent.withAlpha(0.55f));
            A11y::setup(cb, pid, "ONSET: " + pid, true);
            addAndMakeVisible(cb);
            if (auto* cp = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(pid)))
            {
                cb.clear(juce::dontSendNotification);
                for (int i = 0; i < cp->choices.size(); ++i)
                    cb.addItem(cp->choices[i], i + 1);
            }
            return std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, pid, cb);
        };

        auto makeLabel = [&](juce::Label& lbl, const juce::String& text, float sizePt = 8.0f) {
            lbl.setText(text, juce::dontSendNotification);
            lbl.setFont(GalleryFonts::label(sizePt));
            lbl.setColour(juce::Label::textColourId,
                          GalleryColors::get(GalleryColors::textMid()));
            lbl.setJustificationType(juce::Justification::centred);
            lbl.setInterceptsMouseClicks(false, false);
            addAndMakeVisible(lbl);
        };

        // ── Per-voice parameters ─────────────────────────────────────────────
        // Each voice has: algoMode (combo), envShape (combo),
        //   decay, tone, snap, body, blend, pitch, level, pan, character (knobs)

        for (int v = 0; v < kNumVoices; ++v)
        {
            juce::String pre = "perc_v" + juce::String(v + 1) + "_";
            juce::String vName = juce::String(kVoiceNames[v]);

            // Combo boxes
            algoModeAtts[v]  = makeComboBox(algoModeCBs[v],  pre + "algoMode");
            envShapeAtts[v]  = makeComboBox(envShapeCBs[v],  pre + "envShape");

            // Knobs — in display order (top to bottom)
            decayAtts[v]     = makeKnob(decayKnobs[v],     pre + "decay");
            toneAtts[v]      = makeKnob(toneKnobs[v],      pre + "tone");
            snapAtts[v]      = makeKnob(snapKnobs[v],       pre + "snap");
            bodyAtts[v]      = makeKnob(bodyKnobs[v],       pre + "body");
            blendAtts[v]     = makeKnob(blendKnobs[v],      pre + "blend",   accent.withSaturation(0.5f));
            pitchAtts[v]     = makeKnob(pitchKnobs[v],      pre + "pitch",   juce::Colour(0xFFE9A84A));
            levelAtts[v]     = makeKnob(levelKnobs[v],      pre + "level",   accent.withAlpha(0.85f));
            panAtts[v]       = makeKnob(panKnobs[v],        pre + "pan",     GalleryColors::get(GalleryColors::t3()));
            characterAtts[v] = makeKnob(characterKnobs[v],  pre + "character");

            // Labels (row labels — painted in paint(), so no Label components needed there,
            // but we do need a per-voice column header label)
            makeLabel(voiceHeaderLabels[v], vName, 9.0f);
        }

        // ── Row labels (painted as text, not juce::Label, for efficiency) ────
        // (See paint() below.)

        // ── Global section ────────────────────────────────────────────────────
        globalLevelAtt   = makeKnob(globalLevelKnob,   "perc_level");
        globalDriveAtt   = makeKnob(globalDriveKnob,   "perc_drive",       juce::Colour(0xFFFF6B6B));
        globalToneAtt    = makeKnob(globalToneKnob,     "perc_masterTone");
        globalBreathAtt  = makeKnob(globalBreathKnob,   "perc_breathRate",  juce::Colour(0xFF00FF41));
        charGritAtt      = makeKnob(charGritKnob,       "perc_char_grit",   juce::Colour(0xFFFF6B6B));
        charWarmthAtt    = makeKnob(charWarmthKnob,     "perc_char_warmth", juce::Colour(0xFFE9A84A));

        makeLabel(globalLevelLbl,  "LEVEL");
        makeLabel(globalDriveLbl,  "DRIVE");
        makeLabel(globalToneLbl,   "TONE");
        makeLabel(globalBreathLbl, "BREATH");
        makeLabel(charGritLbl,     "GRIT");
        makeLabel(charWarmthLbl,   "WARMTH");

        // ── Macro section ─────────────────────────────────────────────────────
        macroMachineAtt = makeKnob(macroMachineKnob, "perc_macro_machine", juce::Colour(0xFFE9C46A));
        macroPunchAtt   = makeKnob(macroPunchKnob,   "perc_macro_punch",   juce::Colour(0xFFE9C46A));
        macroSpaceAtt   = makeKnob(macroSpaceKnob,   "perc_macro_space",   juce::Colour(0xFFE9C46A));
        macroMutateAtt  = makeKnob(macroMutateKnob,  "perc_macro_mutate",  juce::Colour(0xFFE9C46A));

        makeLabel(macroMachineLbl, "MACHINE");
        makeLabel(macroPunchLbl,   "PUNCH");
        makeLabel(macroSpaceLbl,   "SPACE");
        makeLabel(macroMutateLbl,  "MUTATE");

        // ── XVC section ───────────────────────────────────────────────────────
        xvcKickSnareAtt   = makeKnob(xvcKickSnareKnob,   "perc_xvc_kick_to_snare_filter",  juce::Colour(0xFFBF40FF));
        xvcSnareHatAtt    = makeKnob(xvcSnareHatKnob,    "perc_xvc_snare_to_hat_decay",    juce::Colour(0xFFBF40FF));
        xvcKickTomAtt     = makeKnob(xvcKickTomKnob,     "perc_xvc_kick_to_tom_pitch",     juce::Colour(0xFFBF40FF));
        xvcSnarePercAtt   = makeKnob(xvcSnarePercKnob,   "perc_xvc_snare_to_perc_blend",   juce::Colour(0xFFBF40FF));
        xvcHatChokeAtt    = makeKnob(xvcHatChokeKnob,    "perc_xvc_hat_choke",             juce::Colour(0xFFBF40FF));
        xvcGlobalAmtAtt   = makeKnob(xvcGlobalAmtKnob,   "perc_xvc_global_amount",         juce::Colour(0xFFBF40FF));

        makeLabel(xvcKickSnareLbl,  "K>SN FLT");
        makeLabel(xvcSnareHatLbl,   "SN>HH DC");
        makeLabel(xvcKickTomLbl,    "K>TOM P");
        makeLabel(xvcSnarePercLbl,  "SN>PC BL");
        makeLabel(xvcHatChokeLbl,   "HH CHOKE");
        makeLabel(xvcGlobalAmtLbl,  "XVC AMT");

        // ── FX section ────────────────────────────────────────────────────────
        fxDelayTimeAtt  = makeKnob(fxDelayTimeKnob,  "perc_fx_delay_time",     juce::Colour(0xFF48CAE4));
        fxDelayFbAtt    = makeKnob(fxDelayFbKnob,    "perc_fx_delay_feedback",  juce::Colour(0xFF48CAE4));
        fxDelayMixAtt   = makeKnob(fxDelayMixKnob,   "perc_fx_delay_mix",       juce::Colour(0xFF48CAE4));
        fxReverbSizeAtt = makeKnob(fxReverbSizeKnob, "perc_fx_reverb_size",     juce::Colour(0xFF9B5DE5));
        fxReverbDecAtt  = makeKnob(fxReverbDecKnob,  "perc_fx_reverb_decay",    juce::Colour(0xFF9B5DE5));
        fxReverbMixAtt  = makeKnob(fxReverbMixKnob,  "perc_fx_reverb_mix",      juce::Colour(0xFF9B5DE5));
        fxLofiBitsAtt   = makeKnob(fxLofiBitsKnob,   "perc_fx_lofi_bits",       juce::Colour(0xFF9E9B97));
        fxLofiMixAtt    = makeKnob(fxLofiMixKnob,    "perc_fx_lofi_mix",        juce::Colour(0xFF9E9B97));

        makeLabel(fxDelayTimeLbl,  "DLY TIME");
        makeLabel(fxDelayFbLbl,    "DLY FB");
        makeLabel(fxDelayMixLbl,   "DLY MIX");
        makeLabel(fxReverbSizeLbl, "RVB SIZE");
        makeLabel(fxReverbDecLbl,  "RVB DEC");
        makeLabel(fxReverbMixLbl,  "RVB MIX");
        makeLabel(fxLofiBitsLbl,   "LO BITS");
        makeLabel(fxLofiMixLbl,    "LO MIX");

        // ── Section collapse buttons ──────────────────────────────────────────
        auto initSectionBtn = [&](juce::TextButton& btn, const juce::String& label, bool expanded) {
            btn.setButtonText(label);
            btn.setColour(juce::TextButton::buttonColourId,
                          GalleryColors::get(GalleryColors::surface()));
            btn.setColour(juce::TextButton::buttonOnColourId,
                          accent.withAlpha(0.18f));
            btn.setColour(juce::TextButton::textColourOffId,
                          GalleryColors::get(GalleryColors::t1()));
            btn.setColour(juce::TextButton::textColourOnId,
                          accent);
            btn.setToggleable(true);
            btn.setClickingTogglesState(true);
            btn.setToggleState(expanded, juce::dontSendNotification);
            addAndMakeVisible(btn);
        };

        initSectionBtn(voiceSectionBtn,   "VOICES",    true);
        initSectionBtn(globalSectionBtn,  "GLOBAL",    true);
        initSectionBtn(macroSectionBtn,   "MACROS",    true);
        initSectionBtn(xvcSectionBtn,     "XVC",       false);
        initSectionBtn(fxSectionBtn,      "FX",        false);

        voiceSectionBtn.onClick  = [this] { resized(); repaint(); };
        globalSectionBtn.onClick = [this] { resized(); repaint(); };
        macroSectionBtn.onClick  = [this] { resized(); repaint(); };
        xvcSectionBtn.onClick    = [this] { resized(); repaint(); };
        fxSectionBtn.onClick     = [this] { resized(); repaint(); };

        (void)accent; // suppress unused warning if captured only in lambdas above
    }

    ~DrumDetailPanel() override
    {
        // Destruction order: MIDI learn listeners → attachments → knobs.
        // Listeners hold raw pointers to knobs — destroy them first.
        for (auto* ml : midiLearnListeners)
        {
            // The knob that owns this listener may be gone by the time we reach
            // the listener's destructor, but removeMouseListener should be called
            // before the knob is destroyed.  The member declarations below place
            // midiLearnListeners after all knob members, so C++ destroys them
            // in declaration order (listeners go first here via explicit clear).
            (void)ml; // listeners are raw pointers stored in the vector;
                      // they are owned by the GalleryKnob they were attached to.
        }
        midiLearnListeners.clear();
    }

    // ── Required height for the viewport ─────────────────────────────────────
    int getRequiredHeight(int availableWidth) const
    {
        // Voices section
        int voiceH = 0;
        if (voiceSectionBtn.getToggleState())
        {
            // Column header + 2 combos + kNumVoiceKnobs rows of knobs
            int comboRows = 2; // algoMode + envShape
            voiceH = kHeaderH + comboRows * (kComboH + 2) + kNumVoiceKnobs * kCellH + kPad * 2;
        }

        // Determine how many columns of global/macro/xvc/fx knobs fit
        int knobbyCols = juce::jmax(1, availableWidth / kCellW);
        auto rowsForN = [&](int n) { return (n + knobbyCols - 1) / knobbyCols; };

        int globalH = globalSectionBtn.getToggleState()
                      ? kSectionHeaderH + rowsForN(6) * kCellH + kPad
                      : 0;
        int macroH  = macroSectionBtn.getToggleState()
                      ? kSectionHeaderH + rowsForN(4) * kCellH + kPad
                      : 0;
        int xvcH    = xvcSectionBtn.getToggleState()
                      ? kSectionHeaderH + rowsForN(6) * kCellH + kPad
                      : 0;
        int fxH     = fxSectionBtn.getToggleState()
                      ? kSectionHeaderH + rowsForN(8) * kCellH + kPad
                      : 0;

        int totalH = kPad
            + kSectionHeaderH + voiceH   // voices header + content
            + kSectionHeaderH + globalH  // global header + content (header already counted above)
            + kSectionHeaderH + macroH
            + kSectionHeaderH + xvcH
            + kSectionHeaderH + fxH
            + kPad;

        // Simpler approach: just lay out in resized() and report the bottom-most y
        // We can't call resized() from a const, so use a parallel calculation.
        // Let's compute the precise Y offset:
        int y = kPad;
        // Voices section header
        y += kSectionHeaderH;
        if (voiceSectionBtn.getToggleState())
        {
            y += kHeaderH; // column headers
            y += 2 * (kComboH + 2); // 2 combos per voice
            y += kNumVoiceKnobs * kCellH;
            y += kPad;
        }
        // Global
        y += kSectionHeaderH;
        if (globalSectionBtn.getToggleState())
        {
            y += rowsForN(6) * kCellH + kPad;
        }
        // Macros
        y += kSectionHeaderH;
        if (macroSectionBtn.getToggleState())
        {
            y += rowsForN(4) * kCellH + kPad;
        }
        // XVC
        y += kSectionHeaderH;
        if (xvcSectionBtn.getToggleState())
        {
            y += rowsForN(6) * kCellH + kPad;
        }
        // FX
        y += kSectionHeaderH;
        if (fxSectionBtn.getToggleState())
        {
            y += rowsForN(8) * kCellH + kPad;
        }

        y += kPad;
        (void)totalH; // suppress unused-variable warning
        return y;
    }

    // ── paint ─────────────────────────────────────────────────────────────────
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        const juce::Colour accent = juce::Colour(kElectricBlue);

        // Row labels for the voice grid (left of column 0)
        // We paint these over the full-width background with T3 text.
        if (voiceSectionBtn.getToggleState() && !voiceGridBounds.isEmpty())
        {
            static const char* kRowLabels[kNumVoiceKnobs] = {
                "DECAY", "TONE", "SNAP", "BODY", "BLEND", "PITCH", "LEVEL", "PAN", "CHAR"
            };

            g.setFont(GalleryFonts::value(8.0f));
            g.setColour(get(t3()));

            int ry = voiceGridBounds.getY()
                     + kHeaderH
                     + 2 * (kComboH + 2); // offset past column headers and combos

            for (int r = 0; r < kNumVoiceKnobs; ++r)
            {
                g.drawText(kRowLabels[r],
                           kPad, ry + (kCellH - kLabelH) / 2,
                           kRowLabelW - 4, kLabelH,
                           juce::Justification::centredRight);
                ry += kCellH;
            }

            // Thin divider between row labels and voice columns
            g.setColour(get(borderGray()).withAlpha(0.4f));
            g.drawVerticalLine(kRowLabelW + kPad / 2,
                               (float)voiceGridBounds.getY(),
                               (float)voiceGridBounds.getBottom());

            // Section background tint for the voice grid area
            g.setColour(get(surface()));
            g.fillRect(voiceGridBounds.withTop(voiceGridBounds.getY() + kHeaderH));
        }

        // Thin accent underline on section headers (drawn over the TextButton)
        auto paintSectionUnderline = [&](const juce::Rectangle<int>& hdrBounds) {
            g.setColour(accent.withAlpha(0.35f));
            g.fillRect(hdrBounds.getX(), hdrBounds.getBottom() - 2, hdrBounds.getWidth(), 2);
        };

        if (!voiceSectionHdrBounds.isEmpty())   paintSectionUnderline(voiceSectionHdrBounds);
        if (!globalSectionHdrBounds.isEmpty())  paintSectionUnderline(globalSectionHdrBounds);
        if (!macroSectionHdrBounds.isEmpty())   paintSectionUnderline(macroSectionHdrBounds);
        if (!xvcSectionHdrBounds.isEmpty())     paintSectionUnderline(xvcSectionHdrBounds);
        if (!fxSectionHdrBounds.isEmpty())      paintSectionUnderline(fxSectionHdrBounds);
    }

    // ── resized ───────────────────────────────────────────────────────────────
    void resized() override
    {
        const int w = getWidth();
        int y = kPad;

        // ── VOICES section header ─────────────────────────────────────────────
        voiceSectionHdrBounds = { 0, y, w, kSectionHeaderH };
        voiceSectionBtn.setBounds(voiceSectionHdrBounds);
        y += kSectionHeaderH;

        if (voiceSectionBtn.getToggleState())
        {
            // The voice grid: 8 columns.  Row label column occupies kRowLabelW px.
            int colAreaW = w - kRowLabelW - kPad;
            int colW = juce::jmax(kCellW, colAreaW / kNumVoices);
            // Clamp so 8 columns always fit; if the panel is too narrow, let it scroll
            // (the parent Viewport handles that).
            int gridX = kRowLabelW + kPad;
            int gridStartY = y;

            // Record grid bounds for paint()
            voiceGridBounds = { 0, gridStartY, w, kHeaderH + 2 * (kComboH + 2) + kNumVoiceKnobs * kCellH + kPad };

            // ── Column headers (voice names) ──────────────────────────────────
            for (int v = 0; v < kNumVoices; ++v)
            {
                int cx = gridX + v * colW + (colW - kCellW) / 2;
                voiceHeaderLabels[v].setBounds(cx, y, colW, kHeaderH);
            }
            y += kHeaderH;

            // ── Combo: AlgoMode ───────────────────────────────────────────────
            for (int v = 0; v < kNumVoices; ++v)
            {
                int cx = gridX + v * colW;
                algoModeCBs[v].setBounds(cx, y, colW - 2, kComboH);
            }
            y += kComboH + 2;

            // ── Combo: EnvShape ───────────────────────────────────────────────
            for (int v = 0; v < kNumVoices; ++v)
            {
                int cx = gridX + v * colW;
                envShapeCBs[v].setBounds(cx, y, colW - 2, kComboH);
            }
            y += kComboH + 2;

            // ── Knob rows ─────────────────────────────────────────────────────
            // Rows: decay, tone, snap, body, blend, pitch, level, pan, character
            auto placeRow = [&](GalleryKnob (&knobs)[kNumVoices], int rowY) {
                for (int v = 0; v < kNumVoices; ++v)
                {
                    int cx = gridX + v * colW + (colW - kKnobSize) / 2;
                    knobs[v].setBounds(cx, rowY + 2, kKnobSize, kKnobSize);
                }
            };

            placeRow(decayKnobs,     y); y += kCellH;
            placeRow(toneKnobs,      y); y += kCellH;
            placeRow(snapKnobs,      y); y += kCellH;
            placeRow(bodyKnobs,      y); y += kCellH;
            placeRow(blendKnobs,     y); y += kCellH;
            placeRow(pitchKnobs,     y); y += kCellH;
            placeRow(levelKnobs,     y); y += kCellH;
            placeRow(panKnobs,       y); y += kCellH;
            placeRow(characterKnobs, y); y += kCellH;

            y += kPad;
        }
        else
        {
            voiceGridBounds = {};
            // Hide all voice widgets
            for (int v = 0; v < kNumVoices; ++v)
            {
                voiceHeaderLabels[v].setVisible(false);
                algoModeCBs[v].setVisible(false);
                envShapeCBs[v].setVisible(false);
                decayKnobs[v].setVisible(false); toneKnobs[v].setVisible(false);
                snapKnobs[v].setVisible(false);  bodyKnobs[v].setVisible(false);
                blendKnobs[v].setVisible(false); pitchKnobs[v].setVisible(false);
                levelKnobs[v].setVisible(false); panKnobs[v].setVisible(false);
                characterKnobs[v].setVisible(false);
            }
        }

        // Restore voice widget visibility when expanded
        if (voiceSectionBtn.getToggleState())
        {
            for (int v = 0; v < kNumVoices; ++v)
            {
                voiceHeaderLabels[v].setVisible(true);
                algoModeCBs[v].setVisible(true);
                envShapeCBs[v].setVisible(true);
                decayKnobs[v].setVisible(true); toneKnobs[v].setVisible(true);
                snapKnobs[v].setVisible(true);  bodyKnobs[v].setVisible(true);
                blendKnobs[v].setVisible(true); pitchKnobs[v].setVisible(true);
                levelKnobs[v].setVisible(true); panKnobs[v].setVisible(true);
                characterKnobs[v].setVisible(true);
            }
        }

        // ── GLOBAL section ────────────────────────────────────────────────────
        globalSectionHdrBounds = { 0, y, w, kSectionHeaderH };
        globalSectionBtn.setBounds(globalSectionHdrBounds);
        y += kSectionHeaderH;

        if (globalSectionBtn.getToggleState())
        {
            y = layoutKnobRow(y, w,
                { &globalLevelKnob, &globalDriveKnob, &globalToneKnob,
                  &globalBreathKnob, &charGritKnob,   &charWarmthKnob },
                { &globalLevelLbl,  &globalDriveLbl,  &globalToneLbl,
                  &globalBreathLbl, &charGritLbl,     &charWarmthLbl });
            y += kPad;
        }
        else
        {
            setRowVisible(false,
                { &globalLevelKnob, &globalDriveKnob, &globalToneKnob,
                  &globalBreathKnob, &charGritKnob, &charWarmthKnob },
                { &globalLevelLbl, &globalDriveLbl, &globalToneLbl,
                  &globalBreathLbl, &charGritLbl, &charWarmthLbl });
        }

        // ── MACROS section ────────────────────────────────────────────────────
        macroSectionHdrBounds = { 0, y, w, kSectionHeaderH };
        macroSectionBtn.setBounds(macroSectionHdrBounds);
        y += kSectionHeaderH;

        if (macroSectionBtn.getToggleState())
        {
            y = layoutKnobRow(y, w,
                { &macroMachineKnob, &macroPunchKnob, &macroSpaceKnob, &macroMutateKnob },
                { &macroMachineLbl,  &macroPunchLbl,  &macroSpaceLbl,  &macroMutateLbl });
            y += kPad;
        }
        else
        {
            setRowVisible(false,
                { &macroMachineKnob, &macroPunchKnob, &macroSpaceKnob, &macroMutateKnob },
                { &macroMachineLbl,  &macroPunchLbl,  &macroSpaceLbl,  &macroMutateLbl });
        }

        // ── XVC section ───────────────────────────────────────────────────────
        xvcSectionHdrBounds = { 0, y, w, kSectionHeaderH };
        xvcSectionBtn.setBounds(xvcSectionHdrBounds);
        y += kSectionHeaderH;

        if (xvcSectionBtn.getToggleState())
        {
            y = layoutKnobRow(y, w,
                { &xvcKickSnareKnob, &xvcSnareHatKnob, &xvcKickTomKnob,
                  &xvcSnarePercKnob, &xvcHatChokeKnob, &xvcGlobalAmtKnob },
                { &xvcKickSnareLbl,  &xvcSnareHatLbl,  &xvcKickTomLbl,
                  &xvcSnarePercLbl,  &xvcHatChokeLbl,  &xvcGlobalAmtLbl });
            y += kPad;
        }
        else
        {
            setRowVisible(false,
                { &xvcKickSnareKnob, &xvcSnareHatKnob, &xvcKickTomKnob,
                  &xvcSnarePercKnob, &xvcHatChokeKnob, &xvcGlobalAmtKnob },
                { &xvcKickSnareLbl,  &xvcSnareHatLbl,  &xvcKickTomLbl,
                  &xvcSnarePercLbl,  &xvcHatChokeLbl,  &xvcGlobalAmtLbl });
        }

        // ── FX section ────────────────────────────────────────────────────────
        fxSectionHdrBounds = { 0, y, w, kSectionHeaderH };
        fxSectionBtn.setBounds(fxSectionHdrBounds);
        y += kSectionHeaderH;

        if (fxSectionBtn.getToggleState())
        {
            y = layoutKnobRow(y, w,
                { &fxDelayTimeKnob, &fxDelayFbKnob,    &fxDelayMixKnob,
                  &fxReverbSizeKnob,&fxReverbDecKnob,  &fxReverbMixKnob,
                  &fxLofiBitsKnob,  &fxLofiMixKnob },
                { &fxDelayTimeLbl,  &fxDelayFbLbl,     &fxDelayMixLbl,
                  &fxReverbSizeLbl, &fxReverbDecLbl,   &fxReverbMixLbl,
                  &fxLofiBitsLbl,   &fxLofiMixLbl });
            y += kPad;
        }
        else
        {
            setRowVisible(false,
                { &fxDelayTimeKnob, &fxDelayFbKnob,    &fxDelayMixKnob,
                  &fxReverbSizeKnob,&fxReverbDecKnob,  &fxReverbMixKnob,
                  &fxLofiBitsKnob,  &fxLofiMixKnob },
                { &fxDelayTimeLbl,  &fxDelayFbLbl,     &fxDelayMixLbl,
                  &fxReverbSizeLbl, &fxReverbDecLbl,   &fxReverbMixLbl,
                  &fxLofiBitsLbl,   &fxLofiMixLbl });
        }

        (void)y; // final y is the content bottom; parent uses getRequiredHeight()
    }

private:
    // ── Pixel width reserved for row labels on the left of the voice grid ────
    static constexpr int kRowLabelW = 48;

    // ── Layout helper: place a flat list of knob+label pairs in a wrapping grid ─
    // Returns the new y after the last row.
    int layoutKnobRow(int startY, int availW,
                      std::initializer_list<GalleryKnob*> knobs,
                      std::initializer_list<juce::Label*> labels)
    {
        int cols = juce::jmax(1, availW / kCellW);
        int offsetX = (availW - juce::jmin((int)knobs.size(), cols) * kCellW) / 2;
        int y = startY;
        int idx = 0;

        auto kit = knobs.begin();
        auto lit = labels.begin();

        while (kit != knobs.end())
        {
            int col = idx % cols;
            int row = idx / cols;
            int cx = offsetX + col * kCellW;
            int cy = y + row * kCellH;
            int knobX = cx + (kCellW - kKnobSize) / 2;

            (*kit)->setBounds(knobX, cy + 2, kKnobSize, kKnobSize);
            (*kit)->setVisible(true);
            if (lit != labels.end())
            {
                (*lit)->setBounds(cx, cy + kKnobSize + 4, kCellW, kLabelH);
                (*lit)->setVisible(true);
                ++lit;
            }
            ++kit;
            ++idx;
        }

        int rows = ((int)knobs.size() + cols - 1) / cols;
        return startY + rows * kCellH;
    }

    void setRowVisible(bool visible,
                       std::initializer_list<GalleryKnob*> knobs,
                       std::initializer_list<juce::Label*> labels)
    {
        for (auto* k : knobs) k->setVisible(visible);
        for (auto* l : labels) l->setVisible(visible);
    }

    // ── APVTS reference ───────────────────────────────────────────────────────
    juce::AudioProcessorValueTreeState& apvts;
    MIDILearnManager* learnManager;

    // ── Cached layout rectangles for paint() ─────────────────────────────────
    juce::Rectangle<int> voiceGridBounds;
    juce::Rectangle<int> voiceSectionHdrBounds;
    juce::Rectangle<int> globalSectionHdrBounds;
    juce::Rectangle<int> macroSectionHdrBounds;
    juce::Rectangle<int> xvcSectionHdrBounds;
    juce::Rectangle<int> fxSectionHdrBounds;

    // ── Section collapse buttons ──────────────────────────────────────────────
    juce::TextButton voiceSectionBtn;
    juce::TextButton globalSectionBtn;
    juce::TextButton macroSectionBtn;
    juce::TextButton xvcSectionBtn;
    juce::TextButton fxSectionBtn;

    // ── Per-voice column header labels ────────────────────────────────────────
    juce::Label voiceHeaderLabels[kNumVoices];

    // ── Per-voice combo boxes ─────────────────────────────────────────────────
    juce::ComboBox algoModeCBs[kNumVoices];
    juce::ComboBox envShapeCBs[kNumVoices];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> algoModeAtts[kNumVoices];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> envShapeAtts[kNumVoices];

    // ── Per-voice knobs ───────────────────────────────────────────────────────
    GalleryKnob decayKnobs[kNumVoices];
    GalleryKnob toneKnobs[kNumVoices];
    GalleryKnob snapKnobs[kNumVoices];
    GalleryKnob bodyKnobs[kNumVoices];
    GalleryKnob blendKnobs[kNumVoices];
    GalleryKnob pitchKnobs[kNumVoices];
    GalleryKnob levelKnobs[kNumVoices];
    GalleryKnob panKnobs[kNumVoices];
    GalleryKnob characterKnobs[kNumVoices];

    // ── Per-voice knob attachments ────────────────────────────────────────────
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAtts[kNumVoices];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAtts[kNumVoices];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> snapAtts[kNumVoices];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bodyAtts[kNumVoices];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> blendAtts[kNumVoices];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchAtts[kNumVoices];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAtts[kNumVoices];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtts[kNumVoices];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> characterAtts[kNumVoices];

    // ── Global section knobs + labels ─────────────────────────────────────────
    GalleryKnob globalLevelKnob;
    GalleryKnob globalDriveKnob;
    GalleryKnob globalToneKnob;
    GalleryKnob globalBreathKnob;
    GalleryKnob charGritKnob;
    GalleryKnob charWarmthKnob;

    juce::Label globalLevelLbl;
    juce::Label globalDriveLbl;
    juce::Label globalToneLbl;
    juce::Label globalBreathLbl;
    juce::Label charGritLbl;
    juce::Label charWarmthLbl;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalLevelAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalDriveAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalToneAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalBreathAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> charGritAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> charWarmthAtt;

    // ── Macro section knobs + labels ──────────────────────────────────────────
    GalleryKnob macroMachineKnob;
    GalleryKnob macroPunchKnob;
    GalleryKnob macroSpaceKnob;
    GalleryKnob macroMutateKnob;

    juce::Label macroMachineLbl;
    juce::Label macroPunchLbl;
    juce::Label macroSpaceLbl;
    juce::Label macroMutateLbl;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> macroMachineAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> macroPunchAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> macroSpaceAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> macroMutateAtt;

    // ── XVC section knobs + labels ────────────────────────────────────────────
    GalleryKnob xvcKickSnareKnob;
    GalleryKnob xvcSnareHatKnob;
    GalleryKnob xvcKickTomKnob;
    GalleryKnob xvcSnarePercKnob;
    GalleryKnob xvcHatChokeKnob;
    GalleryKnob xvcGlobalAmtKnob;

    juce::Label xvcKickSnareLbl;
    juce::Label xvcSnareHatLbl;
    juce::Label xvcKickTomLbl;
    juce::Label xvcSnarePercLbl;
    juce::Label xvcHatChokeLbl;
    juce::Label xvcGlobalAmtLbl;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xvcKickSnareAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xvcSnareHatAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xvcKickTomAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xvcSnarePercAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xvcHatChokeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xvcGlobalAmtAtt;

    // ── FX section knobs + labels ─────────────────────────────────────────────
    GalleryKnob fxDelayTimeKnob;
    GalleryKnob fxDelayFbKnob;
    GalleryKnob fxDelayMixKnob;
    GalleryKnob fxReverbSizeKnob;
    GalleryKnob fxReverbDecKnob;
    GalleryKnob fxReverbMixKnob;
    GalleryKnob fxLofiBitsKnob;
    GalleryKnob fxLofiMixKnob;

    juce::Label fxDelayTimeLbl;
    juce::Label fxDelayFbLbl;
    juce::Label fxDelayMixLbl;
    juce::Label fxReverbSizeLbl;
    juce::Label fxReverbDecLbl;
    juce::Label fxReverbMixLbl;
    juce::Label fxLofiBitsLbl;
    juce::Label fxLofiMixLbl;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxDelayTimeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxDelayFbAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxDelayMixAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxReverbSizeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxReverbDecAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxReverbMixAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxLofiBitsAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxLofiMixAtt;

    // ── MIDI learn listeners ──────────────────────────────────────────────────
    // Raw pointers — owned by the GalleryKnobs they are attached to.
    // Cleared explicitly in destructor (before knob members are destroyed).
    std::vector<MidiLearnMouseListener*> midiLearnListeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumDetailPanel)
};

} // namespace xoceanus
// clang-format on
