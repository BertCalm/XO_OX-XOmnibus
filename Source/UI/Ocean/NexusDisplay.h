// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// NexusDisplay — Center identity hub for the Ocean View radial layout.
//
// Renders the current preset's sonic identity in three stacked layers:
//   1. DnaHexagon (64×64) — 6D Sonic DNA profile at full gallery size.
//   2. Preset name — 18px Space Grotesk Bold, XO Gold with a subtle glow.
//   3. Mood badge  — inline pill, mood colour at 12% alpha fill.
//
// Callbacks:
//   onPresetNameClicked — user clicked the name label (open DNA browser).
//   onDnaClicked        — user clicked the hexagon (cycle axis projection).
//
// Preferred height: ~110px  (64 hex + 8 gap + 22 name + 4 gap + 18 badge).
// Width is unconstrained; components are always horizontally centred.
//==============================================================================

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../Gallery/DnaHexagon.h"

namespace xoceanus
{

class NexusDisplay : public juce::Component
{
public:
    //==========================================================================
    NexusDisplay()
    {
        addAndMakeVisible(dnaHex_);
        dnaHex_.setAccentColor(accentColour_);

        // Accessibility: use A11y::setup() so reduced-motion and focus-ring
        // helpers are consistently wired (BLOCKER 2 fix).
        A11y::setup(*this,
                    "Preset Identity",
                    "Current preset DNA, name, and mood. Click name to browse presets.",
                    /*wantsKeyFocus=*/false);
    }

    //==========================================================================
    // juce::Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
        // ── Preset name ───────────────────────────────────────────────────────
        // Two-pass glow: shadow draw at 30% alpha offset 1px down, then sharp.
        if (presetNameBounds_.getWidth() > 0)
        {
            const juce::Font nameFont = GalleryFonts::display(kPresetFontSize);
            g.setFont(nameFont);

            // Shadow pass (glow approximation)
            g.setColour(accentColour_.withAlpha(0.30f));
            g.drawText(presetName_,
                       presetNameBounds_.translated(0, 1),
                       juce::Justification::centred,
                       false);

            // Sharp foreground pass
            g.setColour(accentColour_);
            g.drawText(presetName_,
                       presetNameBounds_,
                       juce::Justification::centred,
                       false);
        }

        // ── Mood badge ────────────────────────────────────────────────────────
        if (moodBadgeBounds_.getWidth() > 0)
        {
            const auto pillRect = moodBadgeBounds_.toFloat();

            // Fill: mood colour at 12% alpha
            g.setColour(moodColour_.withAlpha(kMoodPillAlpha));
            g.fillRoundedRectangle(pillRect, pillRect.getHeight() * 0.5f);

            // Mood name text at full opacity
            g.setFont(GalleryFonts::label(kMoodFontSize));
            g.setColour(moodColour_);
            g.drawText(moodName_,
                       moodBadgeBounds_,
                       juce::Justification::centred,
                       false);
        }

        // ── #909: Live parameter readouts ─────────────────────────────────────
        // Voice count + 4 macro bar gauges, drawn below the mood badge.
        // Rendered at low opacity so they are informational without dominating
        // the identity visual hierarchy.
        if (liveReadoutsBounds_.getWidth() > 0)
        {
            const int w = getWidth();
            const auto rb = liveReadoutsBounds_;

            // Voice count pill — "Vx" where x is the polyphonic voice count.
            g.setFont(GalleryFonts::value(9.0f));
            const juce::Colour readoutCol = accentColour_.withAlpha(0.55f);
            g.setColour(readoutCol);
            juce::String voiceStr = (voiceCount_ > 0)
                                        ? ("V" + juce::String(voiceCount_))
                                        : "V0";
            g.drawText(voiceStr,
                       juce::Rectangle<int>(rb.getX(), rb.getY(), 28, rb.getHeight()),
                       juce::Justification::centredLeft,
                       false);

            // Macro bar gauges: 4 tiny horizontal bars (M1-M4) filling the remainder.
            // Each bar is 8pt tall, 2pt gap between bars, label "M1"..."M4" on left.
            const int barAreaX = rb.getX() + 32;
            const int barAreaW = juce::jmax(0, w - barAreaX * 2); // symmetric margins
            const int barH = 8;   // increased from 4px for legibility
            const int barGapV = 3;
            const int totalBarH = 4 * barH + 3 * barGapV;
            const int barStartY = rb.getCentreY() - totalBarH / 2;

            for (int i = 0; i < 4; ++i)
            {
                const int barY = barStartY + i * (barH + barGapV);
                const float val = juce::jlimit(0.0f, 1.0f, macroValues_[i]);

                // Track background
                g.setColour(accentColour_.withAlpha(0.10f));
                g.fillRoundedRectangle(juce::Rectangle<float>(
                    (float)barAreaX, (float)barY, (float)barAreaW, (float)barH), 2.0f);

                // Fill proportional to value
                if (val > 0.001f)
                {
                    g.setColour(accentColour_.withAlpha(0.45f));
                    g.fillRoundedRectangle(juce::Rectangle<float>(
                        (float)barAreaX, (float)barY,
                        val * (float)barAreaW, (float)barH), 2.0f);
                }
            }
        }
    }

    void resized() override
    {
        const auto bounds = getLocalBounds();
        const int w = bounds.getWidth();

        // ── DNA Hexagon: 64×64, horizontally centred at top ──────────────────
        const int hexX = (w - kDnaSize) / 2;
        dnaHex_.setBounds(hexX, 0, kDnaSize, kDnaSize);

        int y = kDnaSize + kGapHexName;

        // ── Preset name bounds ────────────────────────────────────────────────
        // Measure text width so clicks hit accurately; cap at component width.
        const juce::Font nameFont = GalleryFonts::display(kPresetFontSize);
        const int nameTextW = juce::jmin(
            static_cast<int>(std::ceil(nameFont.getStringWidthFloat(presetName_))) + 2,
            w);
        const int nameH = static_cast<int>(std::ceil(kPresetFontSize)) + 4;
        presetNameBounds_ = juce::Rectangle<int>((w - nameTextW) / 2, y, nameTextW, nameH);
        y += nameH + kGapNameMood;

        // ── Mood badge bounds ─────────────────────────────────────────────────
        // Pill width = text width + horizontal padding on each side.
        const juce::Font moodFont = GalleryFonts::label(kMoodFontSize);
        const int moodTextW = static_cast<int>(std::ceil(moodFont.getStringWidthFloat(moodName_)));
        const int pillW = moodTextW + 2 * kMoodPillPadH;
        const int pillH = static_cast<int>(std::ceil(kMoodFontSize)) + 2 * kMoodPillPadV;
        moodBadgeBounds_ = juce::Rectangle<int>((w - pillW) / 2, y, pillW, pillH);
        y += pillH + kGapMoodReadout;

        // ── #909: Live readouts strip — voice count pill + 4 macro bars ───────
        // Height = 4 bars × 8pt + 3 gaps × 3pt = 41pt total
        constexpr int kReadoutH = 41;
        liveReadoutsBounds_ = juce::Rectangle<int>(0, y, w, kReadoutH);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Hit-test hexagon first (it owns the top region).
        if (dnaHex_.getBounds().contains(e.getPosition()))
        {
            if (onDnaClicked)
                onDnaClicked();
            return;
        }

        // Then test the preset name label.
        if (presetNameBounds_.contains(e.getPosition()))
        {
            if (onPresetNameClicked)
                onPresetNameClicked();
        }
    }

    //==========================================================================
    // Data setters
    //==========================================================================

    void setPresetName(const juce::String& name)
    {
        if (presetName_ == name)
            return;
        presetName_ = name;
        resized();   // re-measure name bounds
        repaint();
    }

    void setMoodName(const juce::String& mood)
    {
        if (moodName_ == mood)
            return;
        moodName_ = mood;
        resized();   // re-measure pill bounds
        repaint();
    }

    void setMoodColour(juce::Colour colour)
    {
        moodColour_ = colour;
        repaint();
    }

    void setDNA(float brightness, float warmth, float movement,
                float density, float space, float aggression)
    {
        dnaHex_.setDNA(brightness, warmth, movement, density, space, aggression);
        // DnaHexagon calls repaint() internally.
    }

    void setAccentColour(juce::Colour accent)
    {
        accentColour_ = accent;
        dnaHex_.setAccentColor(accent);
        repaint();
    }

    // #909: Live parameter feedback — voice count and active macro values.
    // Call from the editor's timer (at ~10 Hz) to keep readouts current.
    // voiceCount: total polyphonic voices across all loaded engines.
    // macroValues[4]: current normalised [0,1] value for macros 1-4.
    void setLiveReadouts(int voiceCount, const std::array<float, 4>& macroValues)
    {
        bool changed = (voiceCount != voiceCount_);
        for (int i = 0; i < 4; ++i)
            if (std::abs(macroValues[i] - macroValues_[i]) > 0.005f)
                changed = true;

        if (!changed)
            return;

        voiceCount_ = voiceCount;
        macroValues_ = macroValues;
        repaint();
    }

    //==========================================================================
    // Callbacks
    //==========================================================================

    /** Fired when the user clicks the preset name label. Use to open the DNA browser. */
    std::function<void()> onPresetNameClicked;

    /** Fired when the user clicks the DNA hexagon. Use to cycle axis projections. */
    std::function<void()> onDnaClicked;

private:
    //==========================================================================
    // Child components
    //==========================================================================

    DnaHexagon dnaHex_;

    //==========================================================================
    // State
    //==========================================================================

    juce::String  presetName_  = "Init";
    juce::String  moodName_    = "Foundation";
    juce::Colour  moodColour_  = juce::Colour(0xFF9E9B97);  // Ocean::salt
    juce::Colour  accentColour_ = juce::Colour(GalleryColors::xoGold);

    // #909: Live parameter readouts — updated via setLiveReadouts()
    int voiceCount_ = 0;
    std::array<float, 4> macroValues_{{0.0f, 0.0f, 0.0f, 0.0f}};

    // Cached layout rects — computed in resized().
    juce::Rectangle<int> presetNameBounds_;
    juce::Rectangle<int> moodBadgeBounds_;
    juce::Rectangle<int> liveReadoutsBounds_; // #909: voice count + macro bars

    //==========================================================================
    // Layout constants
    //==========================================================================

    static constexpr int   kDnaSize        = 64;
    static constexpr int   kGapHexName     = 8;
    static constexpr int   kGapNameMood    = 4;
    static constexpr int   kGapMoodReadout = 6;  // #909: gap between mood badge and live readouts
    static constexpr float kPresetFontSize = 18.0f;
    static constexpr float kMoodFontSize   = 11.0f;
    static constexpr float kMoodPillAlpha  = 0.12f;
    static constexpr int   kMoodPillPadH   = 6;   // horizontal padding inside pill
    static constexpr int   kMoodPillPadV   = 2;   // vertical padding inside pill

    //==========================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusDisplay)
};

} // namespace xoceanus
