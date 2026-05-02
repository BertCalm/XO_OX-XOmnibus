// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// ChordBarComponent.h — Submarine-style compact chord control bar for the XOceanus Ocean View.
//
// The ChordBarComponent sits in the dashboard area and is toggled visible/hidden by the CHORD
// button. When visible it renders a single horizontal row (~28 px tall) containing:
//
//   Palette dot + pill  |  Voicing pill  |  Spread slider  |  Root pill  |  Mini piano  |
//   Rhythm pill  |  Velocity curve pill  |  Swing/Gate/Human sliders  |
//   Mode pills (LIVE / SEQ / ENO)
//
// All controls that have APVTS parameters go through beginChangeGesture / setValueNotifyingHost /
// endChangeGesture. Local-only controls (velocityCurve, humanize, mode) are stored in
// member variables and fed back to the ChordMachine via the onParameterChanged callback or are
// purely visual indicators read from cm_.
//
// Timer fires at 15 Hz to keep chord-assignment visualization current.
// File is entirely self-contained (header-only inline implementation) following the
// XOceanus convention for UI components.

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/ChordMachine.h"
#include "../../Core/ScaleHelpers.h"
#include "../GalleryColors.h"
#include "../AccentColors.h"
#include <functional>
#include <cmath>
#include <array>
#include <cstdint>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

//==============================================================================
/**
    ChordBarComponent

    Compact single-row horizontal chord control bar. See file header for full
    documentation.
*/
class ChordBarComponent : public juce::Component,
                          private juce::Timer,
                          private juce::AudioProcessorValueTreeState::Listener  // F3-003: DAW automation
{
public:
    //==========================================================================
    // Label tables — static constexpr so they live in the header with no ODR issue.
    // Palette names (#1177): DARK→LUSH (minor 9 is jazz-lush), SWEET→BREEZY (major 9).
    static constexpr const char* kPaletteNames[]  = { "WARM","BRIGHT","TENSION","OPEN","LUSH","BREEZY","COMPLEX","RAW" };
    static constexpr const char* kVoicingNames[]  = {
        // Tertian (0-4) — indices MUST remain fixed for preset backward compat
        "ROOT-SPREAD", "DROP-2", "QUARTAL", "UPPER-STRUCT", "UNISON",
        // Quartal family (5-6)
        "QUARTAL-3", "QUARTAL-4",
        // Quintal family (7-8)
        "QUINTAL-3", "QUINTAL-4",
        // Modal-world family (9-13)
        "HIJAZ", "BHAIRAVI", "YO", "IN", "PHRYG-DOM",
        // Drone family (14-19)
        "DRONE-P5", "DRONE-P4", "DRONE-M3", "DRONE-m3", "DRONE-M2", "DRONE-m2"
    };
    static constexpr int kNumVoicings = static_cast<int>(VoicingMode::NumModes);
    static constexpr const char* kRhythmNames[]   = { "Four","Off","Synco","Stab","Gate","Pulse","Broken","Rest" };
    static constexpr const char* kVelCurveNames[] = { "Equal","RootHvy","TopBrt","V-Shape" };

    // Palette accent colors (0xAARRGGBB)
    static constexpr uint32_t kPaletteColors[] = {
        0xFFFF8A65,  // Warm     — warm orange
        0xFFFFD54F,  // Bright   — yellow
        0xFFEF5350,  // Tension  — red
        0xFF81D4FA,  // Open     — sky blue
        0xFF7E57C2,  // Dark     — purple
        0xFFF48FB1,  // Sweet    — pink
        0xFFFF7043,  // Complex  — deep orange
        0xFFBDBDBD   // Raw      — grey
    };

    // Palette semitone intervals (same as ChordDistributor internal table)
    static constexpr int kPaletteIntervals[8][4] = {
        {0, 3, 7, 10},  // Warm     — minor 7th
        {0, 4, 7, 11},  // Bright   — major 7th
        {0, 4, 7, 10},  // Tension  — dominant 7th
        {0, 5, 7, 12},  // Open     — sus4 + octave
        {0, 3, 7, 14},  // Dark     — minor 9th
        {0, 4, 7, 14},  // Sweet    — add9
        {0, 4, 7, 17},  // Complex  — 11th
        {0, 7, 12, 19}  // Raw      — power + octave
    };

    // Mode enum for LIVE / SEQ / ENO
    enum class ChordMode { Live = 0, Seq, Eno, NumModes };

    // B2: Input mode (AUTO / PAD / DEG) — maps to ChordInputMode enum
    enum class InputMode { Auto = 0, Pad, Deg, NumModes };

    // Height constant — the bar is always 28 px when visible.
    static constexpr int kBarHeight = 28;

    //==========================================================================
    explicit ChordBarComponent(juce::AudioProcessorValueTreeState& apvts,
                                const ChordMachine&                 chordMachine)
        : apvts_       (apvts)
        , cm_          (chordMachine)
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, true);

        // Fix #1424: expose chord machine panel to screen readers.
        A11y::setup(*this,
                    "Chord Machine",
                    "Chord sequencer and harmonic input mode selector");

        // Sync initial state from APVTS.
        syncFromApvts();

        // F3-003: Register APVTS listener so DAW automation of these parameters
        // is reflected in the UI without waiting for the 15 Hz poll cycle.
        apvts_.addParameterListener("cm_palette",    this);
        apvts_.addParameterListener("cm_voicing",    this);
        apvts_.addParameterListener("cm_seq_pattern", this);

        // 15 Hz timer for chord assignment / sequencer visualization updates.
        startTimerHz(15);
    }

    ~ChordBarComponent() override
    {
        // F3-003: Mirror addParameterListener registrations — must remove before destruction.
        apvts_.removeParameterListener("cm_palette",    this);
        apvts_.removeParameterListener("cm_voicing",    this);
        apvts_.removeParameterListener("cm_seq_pattern", this);
        stopTimer();
    }

    //==========================================================================
    /// Show or hide the bar (wraps Component::setVisible so callers can hook
    /// onVisibilityChanged if they need to trigger layout changes).
    void setVisible(bool shouldBeVisible) override
    {
        Component::setVisible(shouldBeVisible);
        if (onVisibilityChanged)
            onVisibilityChanged();
    }

    /// Callback fired whenever setVisible() changes the visibility state.
    std::function<void()> onVisibilityChanged;

    /// B2: Callback fired when input mode pill changes (AUTO/PAD/DEG).
    /// Optional — used for testability and parent-component notification.
    std::function<void(InputMode)> onInputModeChanged;

private:
    //==========================================================================
    // Sub-component geometry (computed in layoutControls(), used in paint/mouse).

    // Identifies which interactive region the mouse is over / inside.
    enum class RegionType : int
    {
        None = -1,
        Palette     = 0,
        Voicing     = 1,
        SpreadSlider= 2,
        Root        = 3,
        MiniPiano   = 4,
        Rhythm      = 5,
        VelCurve    = 6,
        SwingSlider = 7,
        GateSlider  = 8,
        HumanSlider = 9,
        ModeLive    = 11,
        ModeSeq     = 12,
        ModeEno     = 13,
        // B2: Input mode pills
        InputAuto   = 14,
        InputPad    = 15,
        InputDeg    = 16,
    };

    struct PillRegion
    {
        juce::Rectangle<float> bounds;
        RegionType             type;
    };

    struct SliderRegion
    {
        juce::Rectangle<float> track;   // full track rect (centred on midY)
        float                  value;   // 0–1
        RegionType             type;
    };

    //==========================================================================
    // paint() entry
    void paint(juce::Graphics& g) override
    {
        layoutControls(static_cast<float>(getWidth()));
        paintBar(g);
    }

    //--------------------------------------------------------------------------
    void paintBar(juce::Graphics& g)
    {
        const float w    = static_cast<float>(getWidth());
        const float h    = static_cast<float>(getHeight());
        const float midY = h * 0.5f;

        // Background — very subtle dark fill so it reads as a distinct strip.
        g.setColour(juce::Colour(0xFF111820));
        g.fillRect(0.0f, 0.0f, w, h);

        // Top + bottom borders.
        g.setColour(juce::Colour(60, 180, 170).withAlpha(0.10f));
        g.fillRect(0.0f, 0.0f, w, 1.0f);
        g.setColour(juce::Colour(60, 180, 170).withAlpha(0.07f));
        g.fillRect(0.0f, h - 1.0f, w, 1.0f);

        // ── Fonts ──
        static const juce::Font pillFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(9.0f));
        static const juce::Font labelFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withHeight(8.0f));

        // ── Separators ──
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.07f));
        for (float sx : separatorXs_)
            g.fillRect(sx, midY - 8.0f, 1.0f, 16.0f);

        // ── Palette dot ──
        if (!paletteDotBounds_.isEmpty())
        {
            g.setColour(juce::Colour(kPaletteColors[currentPalette_]));
            g.fillEllipse(paletteDotBounds_);
        }

        // ── Pill buttons ──
        for (const auto& pill : pillRegions_)
        {
            const bool isHover = (hoveredRegion_ == pill.type);
            paintPill(g, pillFont, pill, isHover);
        }

        // ── Spread slider ──
        paintLabeledSlider(g, labelFont, midY, spreadLabelBounds_, spreadSlider_, "SPREAD");

        // ── Mini piano ──
        if (!miniPianoBounds_.isEmpty())
            paintMiniPiano(g);

        // ── Swing / Gate / Human sliders ──
        paintLabeledSlider(g, labelFont, midY, swingLabelBounds_, swingSlider_,  "SWING");
        paintLabeledSlider(g, labelFont, midY, gateLabelBounds_,  gateSlider_,   "GATE");
        paintLabeledSlider(g, labelFont, midY, humanLabelBounds_, humanSlider_,  "HUMAN");

        // ── B2: Pad grid overlay (PadPerChord mode) ──
        if (currentInputMode_ == InputMode::Pad && !padGridBounds_.isEmpty())
            paintPadGrid(g, pillFont);

        // ── B2: Degree readout (ScaleDegree mode) ──
        if (currentInputMode_ == InputMode::Deg && !degreeReadoutBounds_.isEmpty())
            paintDegreeReadout(g, pillFont);
    }

    //--------------------------------------------------------------------------
    // Paint a single pill button.
    void paintPill(juce::Graphics& g, const juce::Font& font,
                   const PillRegion& pill, bool isHover) const
    {
        const bool isActive = pillIsActive(pill.type);

        juce::Colour textCol, borderCol, bgCol;
        if (isActive)
        {
            textCol   = juce::Colour(127, 219, 202).withAlpha(0.80f);
            borderCol = juce::Colour(127, 219, 202).withAlpha(0.25f);
            bgCol     = juce::Colour(127, 219, 202).withAlpha(0.06f);
        }
        else if (isHover)
        {
            textCol   = juce::Colour(200, 204, 216).withAlpha(0.80f);
            borderCol = juce::Colour(200, 204, 216).withAlpha(0.16f);
            bgCol     = juce::Colours::transparentBlack;
        }
        else
        {
            textCol   = juce::Colour(200, 204, 216).withAlpha(0.50f);
            borderCol = juce::Colour(200, 204, 216).withAlpha(0.08f);
            bgCol     = juce::Colours::transparentBlack;
        }

        // Palette pill gets the palette color for text + border.
        if (pill.type == RegionType::Palette)
        {
            textCol   = juce::Colour(kPaletteColors[currentPalette_]).withAlpha(isHover ? 1.0f : 0.85f);
            borderCol = juce::Colour(kPaletteColors[currentPalette_]).withAlpha(0.50f);
            bgCol     = juce::Colours::transparentBlack;
        }

        if (!bgCol.isTransparent())
        {
            g.setColour(bgCol);
            g.fillRoundedRectangle(pill.bounds, 4.0f);
        }

        g.setColour(borderCol);
        g.drawRoundedRectangle(pill.bounds, 4.0f, 1.0f);

        g.setFont(font);
        g.setColour(textCol);
        g.drawText(pillLabel(pill.type), pill.bounds.toNearestInt(),
                   juce::Justification::centred, false);
    }

    //--------------------------------------------------------------------------
    void paintLabeledSlider(juce::Graphics&               g,
                             const juce::Font&              labelFont,
                             float                          /*midY*/,
                             const juce::Rectangle<float>&  labelBounds,
                             const SliderRegion&            sr,
                             const char*                    labelText) const
    {
        if (labelBounds.isEmpty() || sr.track.isEmpty())
            return;

        // Label
        g.setFont(labelFont);
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.30f));
        g.drawText(labelText, labelBounds.toNearestInt(),
                   juce::Justification::centred, false);

        // Track — 3 px tall
        const float trackY  = sr.track.getCentreY();
        const float trackX1 = sr.track.getX();
        const float trackX2 = sr.track.getRight();
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.10f));
        g.fillRect(trackX1, trackY - 1.5f, trackX2 - trackX1, 3.0f);

        // Thumb — 10 px circle
        const float thumbX = trackX1 + sr.value * (trackX2 - trackX1);
        g.setColour(juce::Colour(127, 219, 202).withAlpha(0.80f));
        g.fillEllipse(thumbX - 5.0f, trackY - 5.0f, 10.0f, 10.0f);
        g.setColour(juce::Colour(127, 219, 202).withAlpha(0.40f));
        g.drawEllipse(thumbX - 5.0f, trackY - 5.0f, 10.0f, 10.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    /// Paint the 2-octave mini piano canvas showing the current chord tones.
    void paintMiniPiano(juce::Graphics& g) const
    {
        const auto& pb         = miniPianoBounds_;
        const float pianoW     = pb.getWidth();
        const float pianoH     = pb.getHeight();
        const int   numSemis   = 24;           // 2 octaves
        const int   root       = cm_.getLiveRoot();

        // Build the active semitone set from ChordMachine's current assignment.
        const auto assign = cm_.getCurrentAssignment();
        // We highlight by semitone offset from root (mod 12), plus across both octaves.
        bool activeSemi[24] = {};
        for (int i = 0; i < 4; ++i)
        {
            if (assign.midiNotes[i] < 0)
                continue;
            int offset = assign.midiNotes[i] - root;
            if (offset >= 0 && offset < numSemis)
                activeSemi[offset] = true;
        }

        // Also highlight palette intervals as a fallback reference.
        const int palIdx = juce::jlimit(0, 7, currentPalette_);
        for (int k = 0; k < 4; ++k)
        {
            int iv = kPaletteIntervals[palIdx][k];
            if (iv >= 0 && iv < numSemis)
                activeSemi[iv] = true;
        }

        // Count white keys in 2 octaves (14 per 2 octaves: C D E F G A B  x2)
        // Pattern of semitones in an octave: 0=C,1=C#,2=D,3=D#,4=E,5=F,6=F#,7=G,8=G#,9=A,10=A#,11=B
        static constexpr bool kIsBlack[12] = { false,true,false,true,false,false,true,false,true,false,true,false };
        const int totalWhite = 14; // 7 per octave × 2
        const float whiteW = pianoW / static_cast<float>(totalWhite);
        const float whiteH = pianoH;
        const float blackH = pianoH * 0.60f;
        const float blackW = whiteW * 0.65f;

        const juce::Colour whiteKeyCol  = juce::Colour(200, 204, 216).withAlpha(0.08f);
        const juce::Colour blackKeyCol  = juce::Colour(20,  22,  28).withAlpha(0.80f);
        const juce::Colour paletteCol   = juce::Colour(kPaletteColors[palIdx]);

        // Draw white keys first
        float whiteX = pb.getX();
        for (int semi = 0; semi < numSemis; ++semi)
        {
            if (kIsBlack[semi % 12])
                continue;

            const juce::Colour fillCol = activeSemi[semi]
                ? paletteCol.withAlpha(0.88f)
                : whiteKeyCol;

            g.setColour(fillCol);
            g.fillRect(whiteX, pb.getY(), whiteW - 1.0f, whiteH);

            // Key border
            g.setColour(juce::Colour(20, 22, 28).withAlpha(0.60f));
            g.drawRect(whiteX, pb.getY(), whiteW - 1.0f, whiteH, 1.0f);

            whiteX += whiteW;
        }

        // Draw black keys on top
        float blackRefX = pb.getX();
        int   whiteCount = 0;
        for (int semi = 0; semi < numSemis; ++semi)
        {
            if (kIsBlack[semi % 12])
            {
                // Black key sits between two white keys — offset by ~60% of white width.
                const float bx = blackRefX - blackW * 0.5f;
                const juce::Colour fillCol = activeSemi[semi]
                    ? paletteCol.withAlpha(0.80f)
                    : blackKeyCol;

                g.setColour(fillCol);
                g.fillRect(bx, pb.getY(), blackW, blackH);
            }
            else
            {
                whiteCount++;
                blackRefX = pb.getX() + static_cast<float>(whiteCount) * whiteW;
            }
        }

        // Draw small dots below chord notes.
        // Re-use whiteX as the cursor; blackCenterX tracks the centre of the last black key slot.
        const float dotY = pb.getBottom() + 2.0f;
        whiteX = pb.getX();
        float blackCenterX = pb.getX(); // centre X of current black key (right edge of prev white key)
        for (int semi = 0; semi < numSemis; ++semi)
        {
            if (kIsBlack[semi % 12])
            {
                if (activeSemi[semi])
                {
                    // Black key centre is at the right edge of the preceding white key.
                    g.setColour(paletteCol.withAlpha(0.70f));
                    g.fillEllipse(blackCenterX - 2.0f, dotY, 4.0f, 4.0f);
                }
            }
            else
            {
                if (activeSemi[semi])
                {
                    const float cx = whiteX + whiteW * 0.5f;
                    g.setColour(paletteCol.withAlpha(0.70f));
                    g.fillEllipse(cx - 2.0f, dotY, 4.0f, 4.0f);
                }
                whiteX += whiteW;
                blackCenterX = whiteX; // right edge of this white key = centre of the next black key
            }
        }

        // Outer border
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.08f));
        g.drawRect(pb, 1.0f);
    }

    //--------------------------------------------------------------------------
    /// B2: Paint the 16-pad chord grid (PadPerChord mode).
    /// Renders in a compact row — 16 pads × (padCellW × h).
    /// Cached gradient: none needed (solid fill per pad).
    void paintPadGrid(juce::Graphics& g, const juce::Font& font) const
    {
        const auto& r    = padGridBounds_;
        const float cellW = r.getWidth() / 16.0f;
        const float cellH = r.getHeight();

        // Teal chain color from AccentColors (AAA contrast on dark bg)
        const juce::Colour activeCol  = XOceanus::AccentColors::chainBright;   // #90F2FA AAA
        const juce::Colour inactiveCol = juce::Colour(200, 204, 216).withAlpha(0.18f);
        const juce::Colour selectedCol = XOceanus::AccentColors::chainAccent;  // #6CEBF4 AAA

        for (int i = 0; i < 16; ++i)
        {
            const float cx = r.getX() + static_cast<float>(i) * cellW;
            const juce::Rectangle<float> cell(cx + 1.0f, r.getY() + 1.0f, cellW - 2.0f, cellH - 2.0f);

            const PadChordSlot slot = cm_.getPadChord(i);
            const bool isSelected = (i == selectedPadIndex_);

            // Background tint
            if (slot.active)
            {
                g.setColour((isSelected ? selectedCol : activeCol).withAlpha(0.10f));
                g.fillRoundedRectangle(cell, 2.0f);
            }

            // Border
            const juce::Colour borderCol = isSelected
                ? selectedCol.withAlpha(0.60f)
                : (slot.active ? activeCol.withAlpha(0.28f) : inactiveCol);
            g.setColour(borderCol);
            g.drawRoundedRectangle(cell, 2.0f, 1.0f);

            // Root letter label (WCAG AAA: chainBright on dark bg = 13.5:1)
            if (slot.active)
            {
                const juce::String rootLetter = ChordMachine::midiNoteToName(slot.rootNote);
                g.setFont(font);
                g.setColour(isSelected ? selectedCol : activeCol.withAlpha(0.85f));
                g.drawText(rootLetter, cell.toNearestInt(), juce::Justification::centred, false);
            }
        }
    }

    //--------------------------------------------------------------------------
    /// B2: Paint the scale-degree readout (ScaleDegree mode).
    /// Shows Roman numerals for all scale degrees; highlights the last-triggered degree.
    void paintDegreeReadout(juce::Graphics& g, const juce::Font& font) const
    {
        const auto& r = degreeReadoutBounds_;
        const int scaleIdx = cm_.getGlobalScaleIndex();
        const int degreeCount = kScaleIntervalCount[std::max(0, std::min(scaleIdx, kNumScaleTypes - 1))];
        if (degreeCount <= 0) return;

        const float cellW = r.getWidth() / static_cast<float>(degreeCount);
        const float cellH = r.getHeight();
        const int lastDegree = cm_.getLastIncomingDegree();

        // AAA color for active degree: chainBright (#90F2FA, 13.5:1)
        const juce::Colour activeCol   = XOceanus::AccentColors::chainBright;
        const juce::Colour inactiveCol = juce::Colour(200, 204, 216).withAlpha(0.35f);

        for (int d = 0; d < degreeCount; ++d)
        {
            const float cx = r.getX() + static_cast<float>(d) * cellW;
            const juce::Rectangle<float> cell(cx + 1.0f, r.getY() + 1.0f, cellW - 2.0f, cellH - 2.0f);
            const bool isActive = (d == lastDegree);

            if (isActive)
            {
                g.setColour(activeCol.withAlpha(0.12f));
                g.fillRoundedRectangle(cell, 2.0f);
                g.setColour(activeCol.withAlpha(0.45f));
                g.drawRoundedRectangle(cell, 2.0f, 1.0f);
            }

            // Compute Roman numeral for this degree
            const bool minorQ = isDegreeMinorQuality(d, scaleIdx);
            const char* roman = degreeRomanNumeral(d, scaleIdx, minorQ);

            g.setFont(font);
            g.setColour(isActive ? activeCol : inactiveCol);
            g.drawText(juce::String(roman), cell.toNearestInt(), juce::Justification::centred, false);
        }
    }

    //==========================================================================
    void resized() override
    {
        layoutControls(static_cast<float>(getWidth()));
    }

    //--------------------------------------------------------------------------
    /// Layout all control sub-regions left-to-right.
    /// Called from both resized() and paint() so regions are always current.
    void layoutControls(float /*w*/)
    {
        pillRegions_.clear();
        separatorXs_.clear();

        const float h    = static_cast<float>(getHeight() > 0 ? getHeight() : kBarHeight);
        const float midY = h * 0.5f;
        const float padX = 8.0f;
        const float gap  = 4.0f;
        float       curX = padX;

        const float pillH = 16.0f;
        const float pillY = midY - pillH * 0.5f;

        // Helper: add a pill.
        auto addPill = [&](RegionType type, float pillW) {
            pillRegions_.push_back({ juce::Rectangle<float>(curX, pillY, pillW, pillH), type });
            curX += pillW + gap;
        };

        // Helper: add a vertical separator.
        auto addSep = [&]() {
            separatorXs_.push_back(curX);
            curX += 1.0f + gap;
        };

        // ── Palette dot (8×8) ──
        paletteDotBounds_ = juce::Rectangle<float>(curX, midY - 4.0f, 8.0f, 8.0f);
        curX += 8.0f + gap;

        // ── Palette pill ──
        addPill(RegionType::Palette, 52.0f);
        addSep();

        // ── Voicing pill ──
        addPill(RegionType::Voicing, 60.0f);
        addSep();

        // ── "SPREAD" label + slider ──
        spreadLabelBounds_ = juce::Rectangle<float>(curX, midY - 12.0f, 32.0f, 10.0f);
        curX += 32.0f + 2.0f;
        spreadSlider_.track = juce::Rectangle<float>(curX, midY - 2.0f, 50.0f, 4.0f);
        spreadSlider_.value = currentSpread_;
        spreadSlider_.type  = RegionType::SpreadSlider;
        curX += 50.0f + gap;
        addSep();

        // ── Root pill ──
        addPill(RegionType::Root, 34.0f);

        // ── Mini piano (60×20) ──
        miniPianoBounds_ = juce::Rectangle<float>(curX, midY - 10.0f, 60.0f, 20.0f);
        curX += 60.0f + gap;
        addSep();

        // ── Rhythm pill ──
        addPill(RegionType::Rhythm, 44.0f);
        addSep();

        // ── Velocity curve pill ──
        addPill(RegionType::VelCurve, 46.0f);
        addSep();

        // ── Swing label + slider ──
        swingLabelBounds_ = juce::Rectangle<float>(curX, midY - 12.0f, 30.0f, 10.0f);
        curX += 30.0f + 2.0f;
        swingSlider_.track = juce::Rectangle<float>(curX, midY - 2.0f, 50.0f, 4.0f);
        swingSlider_.value = currentSwing_;
        swingSlider_.type  = RegionType::SwingSlider;
        curX += 50.0f + gap;

        // ── Gate label + slider ──
        gateLabelBounds_ = juce::Rectangle<float>(curX, midY - 12.0f, 28.0f, 10.0f);
        curX += 28.0f + 2.0f;
        gateSlider_.track = juce::Rectangle<float>(curX, midY - 2.0f, 50.0f, 4.0f);
        gateSlider_.value = currentGate_;
        gateSlider_.type  = RegionType::GateSlider;
        curX += 50.0f + gap;

        // ── Human label + slider ──
        humanLabelBounds_ = juce::Rectangle<float>(curX, midY - 12.0f, 32.0f, 10.0f);
        curX += 32.0f + 2.0f;
        humanSlider_.track = juce::Rectangle<float>(curX, midY - 2.0f, 50.0f, 4.0f);
        humanSlider_.value = currentHumanize_;
        humanSlider_.type  = RegionType::HumanSlider;
        curX += 50.0f + gap;
        addSep();

        // ── Mode pills: LIVE / SEQ / ENO ──
        addPill(RegionType::ModeLive, 30.0f);
        addPill(RegionType::ModeSeq,  28.0f);
        addPill(RegionType::ModeEno,  28.0f);
        addSep();

        // ── B2: Input mode pills: AUTO / PAD / DEG ──
        addPill(RegionType::InputAuto, 32.0f);
        addPill(RegionType::InputPad,  28.0f);
        addPill(RegionType::InputDeg,  28.0f);

        // ── B2: Pad grid / degree readout — use remaining width to right of pills ──
        // Only occupies space when the respective mode is active.
        addSep();
        const float remainW = static_cast<float>(getWidth() > 0 ? getWidth() : 800) - curX - padX;
        padGridBounds_      = juce::Rectangle<float>{};
        degreeReadoutBounds_ = juce::Rectangle<float>{};
        if (currentInputMode_ == InputMode::Pad && remainW > 80.0f)
        {
            padGridBounds_ = juce::Rectangle<float>(curX, midY - 8.0f, remainW, 16.0f);
        }
        else if (currentInputMode_ == InputMode::Deg && remainW > 40.0f)
        {
            degreeReadoutBounds_ = juce::Rectangle<float>(curX, midY - 8.0f, remainW, 16.0f);
        }
    }

    //==========================================================================
    void mouseDown(const juce::MouseEvent& e) override
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        // Check sliders first (drag semantics, not cycle).
        if (hitTestSlider(spreadSlider_, mx, my)) { beginSliderDrag(RegionType::SpreadSlider, mx); return; }
        if (hitTestSlider(swingSlider_,  mx, my)) { beginSliderDrag(RegionType::SwingSlider,  mx); return; }
        if (hitTestSlider(gateSlider_,   mx, my)) { beginSliderDrag(RegionType::GateSlider,   mx); return; }
        if (hitTestSlider(humanSlider_,  mx, my)) { beginSliderDrag(RegionType::HumanSlider,  mx); return; }

        // Check pill regions.
        for (const auto& pill : pillRegions_)
        {
            if (pill.bounds.contains(mx, my))
            {
                handlePillClick(pill.type);
                return;
            }
        }

        // Mini piano — click to cycle root note.
        if (miniPianoBounds_.expanded(4.0f).contains(mx, my))
        {
            handleRootCycle(+1);
            return;
        }

        // ── B2: Pad grid click ──
        if (currentInputMode_ == InputMode::Pad && !padGridBounds_.isEmpty()
            && padGridBounds_.expanded(4.0f).contains(mx, my))
        {
            const float cellW = padGridBounds_.getWidth() / 16.0f;
            const int padIdx  = juce::jlimit(0, 15,
                static_cast<int>((mx - padGridBounds_.getX()) / cellW));

            if (e.mods.isRightButtonDown())
                showPadEditMenu(padIdx);
            else
                selectedPadIndex_ = padIdx;
            repaint();
            return;
        }
    }

    //--------------------------------------------------------------------------
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (activeSliderType_ == RegionType::None)
            return;

        const float mx = static_cast<float>(e.x);
        const SliderRegion* sr = sliderForType(activeSliderType_);
        if (!sr || sr->track.isEmpty())
            return;

        const float t = juce::jlimit(0.0f, 1.0f,
            (mx - sr->track.getX()) / sr->track.getWidth());
        commitSliderValue(activeSliderType_, t);
    }

    //--------------------------------------------------------------------------
    void mouseUp(const juce::MouseEvent& /*e*/) override
    {
        if (activeSliderType_ != RegionType::None)
        {
            // End change gesture for the parameter whose drag just ended.
            endSliderGesture(activeSliderType_);
            activeSliderType_ = RegionType::None;
        }
    }

    //--------------------------------------------------------------------------
    void mouseMove(const juce::MouseEvent& e) override
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);
        RegionType newHover = RegionType::None;

        for (const auto& pill : pillRegions_)
            if (pill.bounds.contains(mx, my)) { newHover = pill.type; break; }

        if (newHover != hoveredRegion_)
        {
            hoveredRegion_ = newHover;
            repaint();
        }
    }

    //--------------------------------------------------------------------------
    void mouseExit(const juce::MouseEvent& /*e*/) override
    {
        if (hoveredRegion_ != RegionType::None)
        {
            hoveredRegion_ = RegionType::None;
            repaint();
        }
    }

    //==========================================================================
    // ── Slider drag helpers ──

    bool hitTestSlider(const SliderRegion& sr, float mx, float my) const noexcept
    {
        if (sr.track.isEmpty())
            return false;
        // Expand hit area ±8 px vertically.
        auto hit = sr.track.withY(sr.track.getY() - 8.0f)
                           .withHeight(sr.track.getHeight() + 16.0f);
        return hit.contains(mx, my);
    }

    void beginSliderDrag(RegionType type, float mx)
    {
        activeSliderType_ = type;
        const SliderRegion* sr = sliderForType(type);
        if (!sr || sr->track.isEmpty()) { activeSliderType_ = RegionType::None; return; }

        // Begin APVTS gesture.
        beginSliderGesture(type);

        const float t = juce::jlimit(0.0f, 1.0f,
            (mx - sr->track.getX()) / sr->track.getWidth());
        commitSliderValue(type, t);
    }

    void beginSliderGesture(RegionType type)
    {
        const char* paramId = sliderParamId(type);
        if (paramId != nullptr)
            if (auto* p = apvts_.getParameter(paramId))
                p->beginChangeGesture();
    }

    void endSliderGesture(RegionType type)
    {
        const char* paramId = sliderParamId(type);
        if (paramId != nullptr)
            if (auto* p = apvts_.getParameter(paramId))
                p->endChangeGesture();
    }

    void commitSliderValue(RegionType type, float t)
    {
        // Update APVTS parameter if one exists.
        const char* paramId = sliderParamId(type);
        if (paramId != nullptr)
        {
            if (auto* p = apvts_.getParameter(paramId))
                p->setValueNotifyingHost(t);
        }

        // Mirror locally.
        switch (type)
        {
        case RegionType::SpreadSlider: currentSpread_   = t; spreadSlider_.value = t; break;
        case RegionType::SwingSlider:  currentSwing_    = t; swingSlider_.value  = t; break;
        case RegionType::GateSlider:   currentGate_     = t; gateSlider_.value   = t; break;
        case RegionType::HumanSlider:  currentHumanize_ = t; humanSlider_.value  = t; break;
        default: break;
        }

        repaint();
    }

    const SliderRegion* sliderForType(RegionType type) const noexcept
    {
        switch (type)
        {
        case RegionType::SpreadSlider: return &spreadSlider_;
        case RegionType::SwingSlider:  return &swingSlider_;
        case RegionType::GateSlider:   return &gateSlider_;
        case RegionType::HumanSlider:  return &humanSlider_;
        default:                       return nullptr;
        }
    }

    SliderRegion* sliderForType(RegionType type) noexcept
    {
        return const_cast<SliderRegion*>(
            static_cast<const ChordBarComponent*>(this)->sliderForType(type));
    }

    /// Returns the APVTS parameter ID for a slider type, or nullptr if local-only.
    static const char* sliderParamId(RegionType type) noexcept
    {
        switch (type)
        {
        case RegionType::SwingSlider:  return "cm_seq_swing";
        case RegionType::GateSlider:   return "cm_seq_gate";
        case RegionType::SpreadSlider: return nullptr; // no APVTS param for spread
        case RegionType::HumanSlider:  return nullptr; // local-only
        default:                       return nullptr;
        }
    }

    //==========================================================================
    // ── Pill click handlers ──

    void handlePillClick(RegionType type)
    {
        switch (type)
        {
        case RegionType::Palette:
        {
            currentPalette_ = (currentPalette_ + 1) % 8;
            if (auto* p = apvts_.getParameter("cm_palette"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(currentPalette_)));
                p->endChangeGesture();
            }
            break;
        }
        case RegionType::Voicing:
        {
            currentVoicing_ = (currentVoicing_ + 1) % kNumVoicings;
            if (auto* p = apvts_.getParameter("cm_voicing"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(currentVoicing_)));
                p->endChangeGesture();
            }
            break;
        }
        case RegionType::Root:
            handleRootCycle(+1);
            break;

        case RegionType::Rhythm:
        {
            currentRhythm_ = (currentRhythm_ + 1) % 8;
            if (auto* p = apvts_.getParameter("cm_seq_pattern"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(currentRhythm_)));
                p->endChangeGesture();
            }
            break;
        }
        case RegionType::VelCurve:
            currentVelCurve_ = (currentVelCurve_ + 1) % 4;
            break;


        case RegionType::ModeLive:
            currentMode_ = ChordMode::Live;
            syncModeToApvts();
            break;

        case RegionType::ModeSeq:
            currentMode_ = ChordMode::Seq;
            syncModeToApvts();
            break;

        case RegionType::ModeEno:
            currentMode_ = ChordMode::Eno;
            syncModeToApvts();
            break;

        // ── B2: Input mode pills ──
        case RegionType::InputAuto:
            currentInputMode_ = InputMode::Auto;
            syncInputModeToApvts();
            break;

        case RegionType::InputPad:
            currentInputMode_ = InputMode::Pad;
            syncInputModeToApvts();
            break;

        case RegionType::InputDeg:
            currentInputMode_ = InputMode::Deg;
            syncInputModeToApvts();
            break;

        default:
            break;
        }

        repaint();
    }

    //--------------------------------------------------------------------------
    /// B2: Show right-click popup menu to edit a pad chord slot.
    /// Popup is message-thread only — no audio-thread access.
    void showPadEditMenu(int padIdx)
    {
        const PadChordSlot current = cm_.getPadChord(padIdx);

        juce::PopupMenu menu;
        menu.addSectionHeader("Pad " + juce::String(padIdx + 1) + juce::String(juce::CharPointer_UTF8(" \xe2\x80\x94 Edit Chord")));

        // Root note submenu (C0–B5 range, chromatic)
        juce::PopupMenu rootMenu;
        static constexpr const char* kNoteNames[12] = {
            "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
        };
        // Show 3 octaves: 48-83 (C3-B5) as a reasonable playable range
        for (int oct = 3; oct <= 5; ++oct)
        {
            for (int semi = 0; semi < 12; ++semi)
            {
                const int midiNote = oct * 12 + semi;
                const juce::String name = juce::String(kNoteNames[semi]) + juce::String(oct);
                const bool isCurrent    = (midiNote == current.rootNote);
                rootMenu.addItem(1000 + midiNote, name, true, isCurrent);
            }
        }
        menu.addSubMenu("Root Note", rootMenu);

        // Voicing submenu
        juce::PopupMenu voicingMenu;
        for (int v = 0; v < kNumVoicings; ++v)
        {
            const bool isCurrent = (static_cast<int>(current.voicing) == v);
            voicingMenu.addItem(2000 + v, juce::String(kVoicingNames[v]), true, isCurrent);
        }
        menu.addSubMenu("Voicing", voicingMenu);

        // Inversion
        juce::PopupMenu invMenu;
        static constexpr const char* kInvNames[] = { "Root Position", "1st Inversion", "2nd Inversion", "3rd Inversion" };
        for (int i = 0; i < 4; ++i)
            invMenu.addItem(3000 + i, kInvNames[i], true, (current.inversion == i));
        menu.addSubMenu("Inversion", invMenu);

        // Active toggle
        menu.addSeparator();
        menu.addItem(4000, current.active ? "Silence this pad" : "Activate this pad");

        // Sync callback: writes APVTS params for the selected pad slot.
        // We copy padIdx into a lambda — menu runs async on message thread.
        const int capturedPadIdx = padIdx;
        menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(this),
            [this, capturedPadIdx, current](int result)
            {
                if (result <= 0) return; // dismissed

                PadChordSlot updated = current;

                if (result >= 1000 && result < 2000)
                    updated.rootNote = result - 1000;
                else if (result >= 2000 && result < 3000)
                    updated.voicing = static_cast<VoicingMode>(result - 2000);
                else if (result >= 3000 && result < 4000)
                    updated.inversion = result - 3000;
                else if (result == 4000)
                    updated.active = !current.active;

                // Write through APVTS (3 params per slot: root, voicing, inv)
                const juce::String prefix = "chord_pad_" + juce::String(capturedPadIdx) + "_";
                if (auto* p = apvts_.getParameter(prefix + "root"))
                {
                    p->beginChangeGesture();
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(updated.rootNote)));
                    p->endChangeGesture();
                }
                if (auto* p = apvts_.getParameter(prefix + "voicing"))
                {
                    p->beginChangeGesture();
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(updated.voicing)));
                    p->endChangeGesture();
                }
                if (auto* p = apvts_.getParameter(prefix + "inv"))
                {
                    p->beginChangeGesture();
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(updated.inversion)));
                    p->endChangeGesture();
                }
                repaint();
            });
    }

    //--------------------------------------------------------------------------
    /// Cycle the root note chromatically (±1 semitone) and push to APVTS.
    /// The live root from ChordMachine is used as the baseline.
    void handleRootCycle(int delta)
    {
        currentRoot_ = ((currentRoot_ + delta) % 12 + 12) % 12;
        // No direct APVTS root parameter defined — this is a live hint; repaint only.
        repaint();
    }

    //--------------------------------------------------------------------------
    /// Push current mode state to APVTS parameters.
    void syncModeToApvts()
    {
        const bool seqRunning = (currentMode_ == ChordMode::Seq);
        if (auto* p = apvts_.getParameter("cm_seq_running"))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(seqRunning ? 1.0f : 0.0f);
            p->endChangeGesture();
        }

        // wire(1C-1): ENO pill — write cm_eno_mode APVTS param so the audio thread
        // picks up the change via cachedParams.cmEnoMode → chordMachine.setEnoMode()
        // in processBlock (one audio block of latency is acceptable for a mode toggle).
        const bool enoActive = (currentMode_ == ChordMode::Eno);
        if (auto* p = apvts_.getParameter("cm_eno_mode"))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(enoActive ? 1.0f : 0.0f);
            p->endChangeGesture();
        }
    }

    //--------------------------------------------------------------------------
    /// B2: Push input mode selection to APVTS.
    void syncInputModeToApvts()
    {
        if (auto* p = apvts_.getParameter("chord_input_mode"))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(currentInputMode_)));
            p->endChangeGesture();
        }

        if (onInputModeChanged)
            onInputModeChanged(currentInputMode_);
    }


    //==========================================================================
    // ── Pill active state & labels ──

    bool pillIsActive(RegionType type) const noexcept
    {
        switch (type)
        {
        case RegionType::Palette:   return true;  // always displays as selected (cycling widget)
        case RegionType::Voicing:   return true;
        case RegionType::Root:      return true;
        case RegionType::Rhythm:    return true;
        case RegionType::VelCurve:  return true;
        case RegionType::ModeLive:  return (currentMode_ == ChordMode::Live);
        case RegionType::ModeSeq:   return (currentMode_ == ChordMode::Seq);
        case RegionType::ModeEno:   return (currentMode_ == ChordMode::Eno);
        // B2: input mode pills
        case RegionType::InputAuto: return (currentInputMode_ == InputMode::Auto);
        case RegionType::InputPad:  return (currentInputMode_ == InputMode::Pad);
        case RegionType::InputDeg:  return (currentInputMode_ == InputMode::Deg);
        default:                    return false;
        }
    }

    juce::String pillLabel(RegionType type) const
    {
        switch (type)
        {
        case RegionType::Palette:  return juce::String(kPaletteNames[currentPalette_]).toUpperCase();
        case RegionType::Voicing:  return juce::String(kVoicingNames[currentVoicing_]).toUpperCase();
        case RegionType::Root:     return rootName(currentRoot_);
        case RegionType::Rhythm:   return juce::String(kRhythmNames[currentRhythm_]).toUpperCase();
        case RegionType::VelCurve: return juce::String(kVelCurveNames[currentVelCurve_]).toUpperCase();
        case RegionType::ModeLive:  return "LIVE";
        case RegionType::ModeSeq:   return "SEQ";
        case RegionType::ModeEno:   return "ENO";
        // B2: input mode pills
        case RegionType::InputAuto: return "AUTO";
        case RegionType::InputPad:  return "PAD";
        case RegionType::InputDeg:  return "DEG";
        default:                    return {};
        }
    }

    //--------------------------------------------------------------------------
    /// Returns note name (C, C#, D, ..., B) for a semitone offset 0-11.
    static juce::String rootName(int semi) noexcept
    {
        static constexpr const char* kNames[12] = {
            "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
        };
        return juce::String(kNames[((semi % 12) + 12) % 12]);
    }

    //==========================================================================
    // F3-003: AudioProcessorValueTreeState::Listener — called on any thread
    // when cm_palette / cm_voicing / cm_seq_pattern are changed by DAW automation.
    // newValue is the normalized 0..1 parameter value; convert to actual int range
    // using the same convertFrom0to1 pattern as syncFromApvts().
    void parameterChanged(const juce::String& parameterID, float newValue) override
    {
        auto toInt = [&](const char* id) -> int {
            if (auto* p = apvts_.getParameter(id))
                return static_cast<int>(p->convertFrom0to1(newValue) + 0.5f);
            return 0;
        };

        if (parameterID == "cm_palette")
            currentPalette_ = juce::jlimit(0, 7, toInt("cm_palette"));
        else if (parameterID == "cm_voicing")
            currentVoicing_ = juce::jlimit(0, kNumVoicings - 1, toInt("cm_voicing"));
        else if (parameterID == "cm_seq_pattern")
            currentRhythm_ = juce::jlimit(0, 7, toInt("cm_seq_pattern"));

        // parameterChanged() can fire on any thread — marshal repaint to message thread.
        juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<ChordBarComponent>(this)]()
        {
            if (safeThis != nullptr)
                safeThis->repaint();
        });
    }

    //==========================================================================
    /// Read current APVTS parameter values into local state.
    void syncFromApvts()
    {
        auto readInt = [&](const char* id) -> int {
            if (auto* p = apvts_.getParameter(id))
                return static_cast<int>(p->convertFrom0to1(p->getValue()) + 0.5f);
            return 0;
        };
        auto readFloat = [&](const char* id) -> float {
            if (auto* p = apvts_.getParameter(id))
                return p->getValue();
            return 0.0f;
        };

        currentPalette_ = juce::jlimit(0, 7, readInt("cm_palette"));
        currentVoicing_ = juce::jlimit(0, kNumVoicings - 1, readInt("cm_voicing"));
        currentRhythm_  = juce::jlimit(0, 7, readInt("cm_seq_pattern"));
        currentSwing_   = readFloat("cm_seq_swing");
        currentGate_    = readFloat("cm_seq_gate");

        const bool seqRunning = (readInt("cm_seq_running") != 0);
        if (seqRunning)
            currentMode_ = ChordMode::Seq;
        else if (cm_.isEnoMode())
            currentMode_ = ChordMode::Eno;
        else
            currentMode_ = ChordMode::Live;

        // Sync read-only values from ChordMachine into local mirrors.
        currentSpread_   = cm_.getSpread();
        currentHumanize_ = cm_.getHumanize();

        // Init local sliders.
        spreadSlider_.value = currentSpread_;
        swingSlider_.value  = currentSwing_;
        gateSlider_.value   = currentGate_;
        humanSlider_.value  = currentHumanize_;

        // Root: derive from cm_.getLiveRoot() semitone class.
        const int liveRoot  = cm_.getLiveRoot();
        currentRoot_ = ((liveRoot % 12) + 12) % 12;

        // B2: input mode
        const int modeInt = readInt("chord_input_mode");
        if (modeInt >= 0 && modeInt < static_cast<int>(InputMode::NumModes))
            currentInputMode_ = static_cast<InputMode>(modeInt);
    }

    //==========================================================================
    void timerCallback() override
    {
        // Sync state that changes on the audio thread (chord assignment, sequencer running).
        // This must run even when hidden so mode state stays correct when the
        // bar is re-shown.
        const bool seqNowRunning = cm_.isSequencerRunning();
        if (seqNowRunning != lastSeqRunning_)
        {
            lastSeqRunning_ = seqNowRunning;
            if (!seqNowRunning && currentMode_ == ChordMode::Seq)
                currentMode_ = ChordMode::Live;
        }

        // Skip the per-tick repaint when hidden — the mini-piano animation
        // is not visible, and the state sync above already covers correctness.
        if (! isShowing())
            return;

        repaint();
    }

    //==========================================================================
    // References
    juce::AudioProcessorValueTreeState& apvts_;
    const ChordMachine&                 cm_;

    // APVTS-mirrored + local state
    int        currentPalette_  = 0;
    int        currentVoicing_  = 0;
    int        currentRhythm_   = 0;
    int        currentVelCurve_ = 0;
    float      currentSpread_   = 0.5f;
    float      currentSwing_    = 0.0f;
    float      currentGate_     = 0.75f;
    float      currentHumanize_ = 0.0f;
    ChordMode  currentMode_     = ChordMode::Live;
    int        currentRoot_     = 0;   // 0=C, semitone class

    // Timer state
    bool lastSeqRunning_ = false;

    // B2: Input mode state
    InputMode  currentInputMode_ = InputMode::Auto;
    int        selectedPadIndex_ = 0;  // selected pad in PadPerChord mode

    // Drag state
    RegionType activeSliderType_ = RegionType::None;
    RegionType hoveredRegion_    = RegionType::None;

    // Laid-out regions (rebuilt each layoutControls() call)
    std::vector<PillRegion>    pillRegions_;
    std::vector<float>         separatorXs_;

    juce::Rectangle<float>     paletteDotBounds_;
    juce::Rectangle<float>     miniPianoBounds_;

    juce::Rectangle<float>     spreadLabelBounds_;
    juce::Rectangle<float>     swingLabelBounds_;
    juce::Rectangle<float>     gateLabelBounds_;
    juce::Rectangle<float>     humanLabelBounds_;

    SliderRegion               spreadSlider_  {};
    SliderRegion               swingSlider_   {};
    SliderRegion               gateSlider_    {};
    SliderRegion               humanSlider_   {};

    // B2: pad grid and degree readout bounds (computed in layoutControls)
    juce::Rectangle<float>     padGridBounds_        {};
    juce::Rectangle<float>     degreeReadoutBounds_  {};

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordBarComponent)
};

} // namespace xoceanus
