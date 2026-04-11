// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// MasterFXStripCompact.h — Submarine-style 36 px Master FX bus control strip for
// the XOceanus Ocean View.
//
// The strip renders 5 sections in a single horizontal row:
//
//   SAT  |  DELAY  |  REVERB  |  MOD  |  COMP
//
// Each section contains:
//   - A section label (8px bold, dimmed)
//   - One or two 24×24 px custom rotary knobs with value arc and indicator line
//   - An "ADV" button that fires the onAdvClicked callback so the parent can
//     open a full-parameter popup for that FX section
//
// DELAY has two knobs (MIX + FB); all other sections have one.
// Total: 6 knobs, 5 ADV buttons, 4 separators between sections.
//
// All knob values map directly to APVTS parameters via
// beginChangeGesture / setValueNotifyingHost / endChangeGesture.
//
// Interaction:
//   - Vertical drag on a knob: drag up = increase, drag down = decrease.
//     dy * 0.8 maps to a 0-100 range (i.e., 125 px = full sweep).
//   - Double-click on a knob: reset to the parameter's default value.
//   - Hover: knob brightens slightly (indicator line alpha 1.0 vs 0.8).
//
// File is entirely self-contained (header-only inline implementation) following
// the XOceanus convention for UI sub-components.

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include <functional>
#include <cmath>
#include <array>
#include <memory>

namespace xoceanus
{

//==============================================================================
/**
    MasterFXStripCompact

    Submarine-style 36 px horizontal strip showing the Master FX bus controls.
    See file header for full documentation.
*/
class MasterFXStripCompact : public juce::Component
{
public:
    //==========================================================================
    // Strip height — 48 px (scaled up from prototype's 36 px for usability).
    static constexpr int kStripHeight = 48;

    //==========================================================================
    // Knob table — 6 entries ordered left-to-right.
    struct KnobDef
    {
        const char* paramId;
        const char* knobLabel;
        const char* sectionLabel; // empty string for secondary knobs in same section
        int         sectionIndex; // 0=SAT, 1=DELAY, 2=REVERB, 3=MOD, 4=COMP
    };

    static constexpr KnobDef kKnobs[] = {
        { "master_satDrive",      "DRIVE", "SAT",   0 },
        { "master_delayMix",      "MIX",   "DELAY", 1 },
        { "master_delayFeedback", "FB",    "",       1 },   // secondary knob, same section
        { "master_reverbMix",     "MIX",   "REVERB",2 },
        { "master_modDepth",      "DEPTH", "MOD",   3 },
        { "master_compMix",       "GLUE",  "COMP",  4 },
    };
    static constexpr int kNumKnobs    = 6;
    static constexpr int kNumSections = 5;

    //==========================================================================
    explicit MasterFXStripCompact(juce::AudioProcessorValueTreeState& apvts)
        : apvts_(apvts)
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, true);
    }

    ~MasterFXStripCompact() override = default;

    //==========================================================================
    /// Fired when the user clicks an ADV button. sectionIndex: 0=SAT … 4=COMP.
    std::function<void(int sectionIndex)> onAdvClicked;

private:
    //==========================================================================
    // Geometry structs — rebuilt in resized(), used in paint() + mouse handlers.

    struct KnobRegion
    {
        juce::Rectangle<float> bounds;   // 24×24 px knob canvas
        int                    knobIndex;
    };

    struct AdvRegion
    {
        juce::Rectangle<float> bounds;
        int                    sectionIndex;
    };

    //==========================================================================
    // paint() / resized()

    void paint(juce::Graphics& g) override
    {
        layoutControls(static_cast<float>(getWidth()));
        paintStrip(g);
    }

    void resized() override
    {
        layoutControls(static_cast<float>(getWidth()));
    }

    //--------------------------------------------------------------------------
    void layoutControls(float w)
    {
        knobRegions_.clear();
        advRegions_.clear();
        separatorXs_.clear();

        const float h    = static_cast<float>(getHeight() > 0 ? getHeight() : kStripHeight);
        const float midY = h * 0.5f;

        // Horizontal layout: padX on each side, gap between elements.
        const float padX    = 16.0f;
        const float gap     = 4.0f;
        const float knobSz  = 32.0f;
        const float pillH   = 14.0f;
        const float pillW   = 22.0f;   // "ADV" button width
        const float labelW  = 36.0f;   // section label min-width

        // We lay out sections left-to-right. We need to measure total width
        // so we can centre the strip content. Do two passes: measure then place.

        // Section definitions (sectionIndex 0-4) — how many knobs each has.
        // knobCountPerSection: SAT=1, DELAY=2, REVERB=1, MOD=1, COMP=1
        static constexpr int kKnobsPerSection[kNumSections] = { 1, 2, 1, 1, 1 };
        static constexpr const char* kSectionNames[kNumSections] = {
            "SAT", "DELAY", "REVERB", "MOD", "COMP"
        };

        // Measure total content width.
        // Each section: labelW + (nKnobs * (knobSz + gap)) + pillW
        // Between sections: gap + 1(separator) + gap
        float totalContent = 0.0f;
        for (int s = 0; s < kNumSections; ++s)
        {
            if (s > 0) totalContent += gap + 1.0f + gap; // separator
            totalContent += labelW + gap;
            totalContent += static_cast<float>(kKnobsPerSection[s]) * (knobSz + gap);
            totalContent += pillW;
        }

        // Centre the content. If narrower than available, shift inward.
        float startX = padX;
        const float availW = w - 2.0f * padX;
        if (totalContent < availW)
            startX = padX + (availW - totalContent) * 0.5f;

        float curX = startX;
        int   knobIdx = 0;

        for (int s = 0; s < kNumSections; ++s)
        {
            // Separator before section (except first).
            if (s > 0)
            {
                separatorXs_.push_back(curX + gap * 0.5f);
                curX += gap + 1.0f + gap;
            }

            // Section label — right-aligned in its labelW slot.
            sectionLabelBounds_[s] = juce::Rectangle<float>(curX, midY - 5.0f, labelW, 10.0f);
            curX += labelW + gap;

            // Knobs.
            const int nk = kKnobsPerSection[s];
            for (int k = 0; k < nk; ++k)
            {
                KnobRegion kr;
                kr.bounds     = juce::Rectangle<float>(curX, midY - knobSz * 0.5f, knobSz, knobSz);
                kr.knobIndex  = knobIdx;
                knobRegions_.push_back(kr);
                curX += knobSz + gap;
                ++knobIdx;
            }

            // ADV button.
            AdvRegion ar;
            ar.bounds       = juce::Rectangle<float>(curX, midY - pillH * 0.5f, pillW, pillH);
            ar.sectionIndex = s;
            advRegions_.push_back(ar);
            curX += pillW;

            // Suppress unused variable warning for kSectionNames.
            (void)kSectionNames;
        }
    }

    //--------------------------------------------------------------------------
    void paintStrip(juce::Graphics& g)
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());

        // Background.
        g.setColour(juce::Colour(10, 12, 18).withAlpha(0.30f));
        g.fillRect(0.0f, 0.0f, w, h);

        // Bottom border 1 px.
        g.setColour(juce::Colour(60, 180, 170).withAlpha(0.06f));
        g.fillRect(0.0f, h - 1.0f, w, 1.0f);

        // Fonts.
        const juce::Font sectionFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(10.0f));

        const juce::Font knobLabelFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(8.0f));

        const juce::Font advFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(8.0f));

        // Separators.
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.05f));
        for (float sx : separatorXs_)
            g.fillRect(sx, h * 0.5f - 10.0f, 1.0f, 20.0f);

        // Section labels.
        static constexpr const char* kSectionNames[kNumSections] = {
            "SAT", "DELAY", "REVERB", "MOD", "COMP"
        };
        g.setFont(sectionFont);
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.30f));
        for (int s = 0; s < kNumSections; ++s)
        {
            if (!sectionLabelBounds_[s].isEmpty())
                g.drawText(kSectionNames[s],
                           sectionLabelBounds_[s].toNearestInt(),
                           juce::Justification::centredRight, false);
        }

        // Knobs.
        for (const auto& kr : knobRegions_)
        {
            const int   ki      = kr.knobIndex;
            const float value01 = readParamValue(ki);
            const bool  isHover = (hoveredKnob_ == ki);
            const float cx      = kr.bounds.getCentreX();
            const float cy      = kr.bounds.getCentreY();
            const float r       = (kr.bounds.getWidth() * 0.5f) - 2.0f;
            paintKnob(g, cx, cy, r, value01, isHover);

            // Knob sub-label (below the knob, 7px).
            if (!kr.bounds.isEmpty())
            {
                const juce::Rectangle<float> labelRect(
                    kr.bounds.getX() - 4.0f,
                    kr.bounds.getBottom() - 1.0f,
                    kr.bounds.getWidth() + 8.0f,
                    9.0f);
                g.setFont(knobLabelFont);
                g.setColour(juce::Colour(200, 204, 216).withAlpha(0.20f));
                g.drawText(kKnobs[ki].knobLabel,
                           labelRect.toNearestInt(),
                           juce::Justification::centred, false);
            }
        }

        // ADV buttons.
        g.setFont(advFont);
        for (const auto& ar : advRegions_)
        {
            const bool isHover = (hoveredAdv_ == ar.sectionIndex);
            const juce::Colour textCol  = isHover
                ? juce::Colour(200, 204, 216).withAlpha(0.50f)
                : juce::Colour(200, 204, 216).withAlpha(0.20f);
            const juce::Colour borderCol = isHover
                ? juce::Colour(200, 204, 216).withAlpha(0.12f)
                : juce::Colour(200, 204, 216).withAlpha(0.05f);

            g.setColour(borderCol);
            g.drawRoundedRectangle(ar.bounds, 2.0f, 1.0f);

            g.setColour(textCol);
            g.drawText("ADV", ar.bounds.toNearestInt(),
                       juce::Justification::centred, false);
        }
    }

    //--------------------------------------------------------------------------
    void paintKnob(juce::Graphics& g,
                   float cx, float cy, float r,
                   float value01, bool isHover) const
    {
        using juce::MathConstants;
        using juce::Colour;
        using juce::Path;
        using juce::PathStrokeType;

        const float startAngle = 0.75f * MathConstants<float>::pi;   // 135°
        const float endAngle   = 2.25f * MathConstants<float>::pi;   // 405°
        const float totalSweep = endAngle - startAngle;              // 270°
        const float valueAngle = startAngle + value01 * totalSweep;

        // Background arc (full sweep).
        {
            Path bgArc;
            bgArc.addCentredArc(cx, cy, r, r, 0.0f, startAngle, endAngle, true);
            g.setColour(Colour(200, 204, 216).withAlpha(0.08f));
            g.strokePath(bgArc, PathStrokeType(3.0f,
                PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Value arc (partial sweep).
        if (value01 > 0.01f)
        {
            Path valArc;
            valArc.addCentredArc(cx, cy, r, r, 0.0f, startAngle, valueAngle, true);
            g.setColour(Colour(127, 219, 202).withAlpha(0.60f));
            g.strokePath(valArc, PathStrokeType(3.0f,
                PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Center dot.
        const float dotR = r * 0.35f;
        g.setColour(Colour(200, 204, 216).withAlpha(0.06f));
        g.fillEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
        g.setColour(Colour(200, 204, 216).withAlpha(0.10f));
        g.drawEllipse(cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f, 1.0f);

        // Indicator line — slightly brighter on hover.
        const float indStart  = r * 0.25f;
        const float indEnd    = r * 0.75f;
        const float indAlpha  = isHover ? 1.0f : 0.80f;
        const float cosA      = std::cos(valueAngle);
        const float sinA      = std::sin(valueAngle);
        g.setColour(Colour(127, 219, 202).withAlpha(indAlpha));
        g.drawLine(cx + indStart * cosA, cy + indStart * sinA,
                   cx + indEnd   * cosA, cy + indEnd   * sinA,
                   1.5f);
    }

    //==========================================================================
    // Mouse handling

    void mouseDown(const juce::MouseEvent& e) override
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        // ADV buttons.
        for (const auto& ar : advRegions_)
        {
            if (ar.bounds.expanded(2.0f).contains(mx, my))
            {
                if (onAdvClicked)
                    onAdvClicked(ar.sectionIndex);
                return;
            }
        }

        // Knobs.
        for (const auto& kr : knobRegions_)
        {
            if (kr.bounds.expanded(3.0f).contains(mx, my))
            {
                dragKnobIdx_   = kr.knobIndex;
                dragStartY_    = e.y;
                dragStartVal_  = readParamValue(kr.knobIndex);
                beginKnobGesture(kr.knobIndex);
                return;
            }
        }
    }

    //--------------------------------------------------------------------------
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (dragKnobIdx_ < 0)
            return;

        // Vertical drag: up = increase, down = decrease.
        // Sensitivity: 125 px covers the full 0-1 range (dy * 0.8 / 100).
        const float dy        = static_cast<float>(e.y - dragStartY_);
        const float delta     = -dy * (0.8f / 100.0f);
        const float newValue  = juce::jlimit(0.0f, 1.0f, dragStartVal_ + delta);

        setKnobValue(dragKnobIdx_, newValue);
        repaint();
    }

    //--------------------------------------------------------------------------
    void mouseUp(const juce::MouseEvent& /*e*/) override
    {
        if (dragKnobIdx_ >= 0)
        {
            endKnobGesture(dragKnobIdx_);
            dragKnobIdx_ = -1;
        }
    }

    //--------------------------------------------------------------------------
    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        for (const auto& kr : knobRegions_)
        {
            if (kr.bounds.expanded(3.0f).contains(mx, my))
            {
                resetKnobToDefault(kr.knobIndex);
                repaint();
                return;
            }
        }
    }

    //--------------------------------------------------------------------------
    void mouseMove(const juce::MouseEvent& e) override
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        int newHoverKnob = -1;
        int newHoverAdv  = -1;

        for (const auto& kr : knobRegions_)
            if (kr.bounds.expanded(3.0f).contains(mx, my)) { newHoverKnob = kr.knobIndex; break; }

        for (const auto& ar : advRegions_)
            if (ar.bounds.expanded(2.0f).contains(mx, my)) { newHoverAdv = ar.sectionIndex; break; }

        if (newHoverKnob != hoveredKnob_ || newHoverAdv != hoveredAdv_)
        {
            hoveredKnob_ = newHoverKnob;
            hoveredAdv_  = newHoverAdv;
            repaint();
        }
    }

    //--------------------------------------------------------------------------
    void mouseExit(const juce::MouseEvent& /*e*/) override
    {
        if (hoveredKnob_ != -1 || hoveredAdv_ != -1)
        {
            hoveredKnob_ = -1;
            hoveredAdv_  = -1;
            repaint();
        }
    }

    //==========================================================================
    // APVTS parameter helpers

    float readParamValue(int knobIndex) const noexcept
    {
        if (knobIndex < 0 || knobIndex >= kNumKnobs)
            return 0.0f;
        auto* p = apvts_.getParameter(kKnobs[knobIndex].paramId);
        return p ? p->getValue() : 0.0f;
    }

    void beginKnobGesture(int knobIndex)
    {
        if (knobIndex < 0 || knobIndex >= kNumKnobs) return;
        if (auto* p = apvts_.getParameter(kKnobs[knobIndex].paramId))
            p->beginChangeGesture();
    }

    void endKnobGesture(int knobIndex)
    {
        if (knobIndex < 0 || knobIndex >= kNumKnobs) return;
        if (auto* p = apvts_.getParameter(kKnobs[knobIndex].paramId))
            p->endChangeGesture();
    }

    void setKnobValue(int knobIndex, float value01)
    {
        if (knobIndex < 0 || knobIndex >= kNumKnobs) return;
        if (auto* p = apvts_.getParameter(kKnobs[knobIndex].paramId))
            p->setValueNotifyingHost(value01);
    }

    void resetKnobToDefault(int knobIndex)
    {
        if (knobIndex < 0 || knobIndex >= kNumKnobs) return;
        auto* p = apvts_.getParameter(kKnobs[knobIndex].paramId);
        if (!p) return;
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->getDefaultValue());
        p->endChangeGesture();
    }

    //==========================================================================
    // Member data

    // APVTS reference
    juce::AudioProcessorValueTreeState& apvts_;

    // Drag state
    int   dragKnobIdx_  = -1;
    int   dragStartY_   = 0;
    float dragStartVal_ = 0.0f;

    // Hover state
    int hoveredKnob_ = -1;
    int hoveredAdv_  = -1;

    // Laid-out regions (rebuilt each layoutControls() call)
    std::vector<KnobRegion>  knobRegions_;
    std::vector<AdvRegion>   advRegions_;
    std::vector<float>       separatorXs_;

    // Per-section label rects (5 sections)
    std::array<juce::Rectangle<float>, kNumSections> sectionLabelBounds_ {};

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterFXStripCompact)
};

} // namespace xoceanus
