// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/PresetManager.h"

namespace xoceanus
{

//==============================================================================
// DnaHexagon — 6D Sonic DNA visualized as a distorted hexagon.
//
// Each of the 6 DNA axes maps to one vertex of a regular hexagon, displacing it
// radially outward/inward:
//   radius * (0.3 + 0.7 * dna[i])   →  min 30%, max 100% of base radius
//
// Vertex order (flat-top orientation, starting from top-right, going clockwise):
//   0 = brightness  (top-right,    30 deg)
//   1 = warmth      (right,        90 deg)
//   2 = movement    (bottom-right, 150 deg)
//   3 = density     (bottom-left,  210 deg)
//   4 = space       (left,         270 deg)
//   5 = aggression  (top-left,     330 deg)
//
// Sizes: works at any dimension — 48x48 for preset cards, 24x24 for header mini,
//        120x120 for detail view hover.
//
// Usage:
//   DnaHexagon hex;
//   hex.setDNA(preset.dna);
//   hex.setAccentColor(engineAccent);
//   addAndMakeVisible(hex);
//
class DnaHexagon : public juce::Component,
                   public juce::SettableTooltipClient
{
public:
    DnaHexagon() = default;

    //==========================================================================
    // API
    //==========================================================================

    /** Set all 6 DNA axes from a PresetDNA struct (values clamped to [0,1]). */
    void setDNA(const PresetDNA& dna)
    {
        dna_[0] = juce::jlimit(0.0f, 1.0f, dna.brightness);
        dna_[1] = juce::jlimit(0.0f, 1.0f, dna.warmth);
        dna_[2] = juce::jlimit(0.0f, 1.0f, dna.movement);
        dna_[3] = juce::jlimit(0.0f, 1.0f, dna.density);
        dna_[4] = juce::jlimit(0.0f, 1.0f, dna.space);
        dna_[5] = juce::jlimit(0.0f, 1.0f, dna.aggression);
        updateTooltip();
        repaint();
    }

    /** Set all 6 DNA values individually (values clamped to [0,1]). */
    void setDNA(float brightness, float warmth, float movement,
                float density, float space, float aggression)
    {
        dna_[0] = juce::jlimit(0.0f, 1.0f, brightness);
        dna_[1] = juce::jlimit(0.0f, 1.0f, warmth);
        dna_[2] = juce::jlimit(0.0f, 1.0f, movement);
        dna_[3] = juce::jlimit(0.0f, 1.0f, density);
        dna_[4] = juce::jlimit(0.0f, 1.0f, space);
        dna_[5] = juce::jlimit(0.0f, 1.0f, aggression);
        updateTooltip();
        repaint();
    }

    /** Set the engine accent color used for fill and stroke. */
    void setAccentColor(juce::Colour c)
    {
        accent_ = c;
        repaint();
    }

    //==========================================================================
    // juce::Component
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();

        // Use the smaller of width/height to guarantee a clean circle.
        // Subtract 1.5px padding so the stroke doesn't clip at the edges.
        const float baseRadius = (juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f) - 1.5f;
        if (baseRadius < 2.0f)
            return;

        // ── Guide hexagon (regular, undistorted) ─────────────────────────────
        // Painted first so it sits behind the distorted shape.
        {
            juce::Path guide;
            for (int i = 0; i < 6; ++i)
            {
                const float angle = juce::MathConstants<float>::pi / 6.0f
                                    + static_cast<float>(i) * juce::MathConstants<float>::pi / 3.0f;
                const float vx = cx + baseRadius * std::cos(angle);
                const float vy = cy + baseRadius * std::sin(angle);
                if (i == 0)
                    guide.startNewSubPath(vx, vy);
                else
                    guide.lineTo(vx, vy);
            }
            guide.closeSubPath();

            g.setColour(accent_.withAlpha(0.15f));
            g.strokePath(guide, juce::PathStrokeType(1.0f));
        }

        // ── DNA shape ─────────────────────────────────────────────────────────
        // Each vertex displaced radially: r = baseRadius * (0.3 + 0.7 * dna[i])
        juce::Path shape;
        for (int i = 0; i < 6; ++i)
        {
            const float angle = juce::MathConstants<float>::pi / 6.0f
                                + static_cast<float>(i) * juce::MathConstants<float>::pi / 3.0f;
            const float r = baseRadius * (0.3f + 0.7f * dna_[static_cast<size_t>(i)]);
            const float vx = cx + r * std::cos(angle);
            const float vy = cy + r * std::sin(angle);
            if (i == 0)
                shape.startNewSubPath(vx, vy);
            else
                shape.lineTo(vx, vy);
        }
        shape.closeSubPath();

        // Fill: accent at 20% alpha
        g.setColour(accent_.withAlpha(0.20f));
        g.fillPath(shape);

        // Stroke: accent at 60% alpha, 1.5px
        g.setColour(accent_.withAlpha(0.60f));
        g.strokePath(shape, juce::PathStrokeType(1.5f));
    }

private:
    //==========================================================================
    // Tooltip: build and apply axis value string on each DNA update.
    //==========================================================================

    void updateTooltip()
    {
        setTooltip(juce::String("DNA: Brightness ") + juce::String(dna_[0], 2)
                   + "  Warmth " + juce::String(dna_[1], 2)
                   + "  Movement " + juce::String(dna_[2], 2)
                   + "  Density " + juce::String(dna_[3], 2)
                   + "  Space " + juce::String(dna_[4], 2)
                   + "  Aggression " + juce::String(dna_[5], 2));
    }

    // Vertex order: brightness, warmth, movement, density, space, aggression
    std::array<float, 6> dna_ = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
    juce::Colour accent_ = juce::Colour(0xFFE9C46A); // XO Gold default

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DnaHexagon)
};

} // namespace xoceanus
