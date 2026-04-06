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

        // Accessibility: the outer component announces as a group.
        setTitle("Preset Identity");
        setDescription("DNA hexagon, preset name, and mood badge.");
        setWantsKeyboardFocus(false);
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

    // Cached hit-test rects — computed in resized().
    juce::Rectangle<int> presetNameBounds_;
    juce::Rectangle<int> moodBadgeBounds_;

    //==========================================================================
    // Layout constants
    //==========================================================================

    static constexpr int   kDnaSize        = 64;
    static constexpr int   kGapHexName     = 8;
    static constexpr int   kGapNameMood    = 4;
    static constexpr float kPresetFontSize = 18.0f;
    static constexpr float kMoodFontSize   = 11.0f;
    static constexpr float kMoodPillAlpha  = 0.12f;
    static constexpr int   kMoodPillPadH   = 6;   // horizontal padding inside pill
    static constexpr int   kMoodPillPadV   = 2;   // vertical padding inside pill

    //==========================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusDisplay)
};

} // namespace xoceanus
