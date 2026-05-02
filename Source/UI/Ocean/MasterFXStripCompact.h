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
#include "../Tokens.h"
#include "../Gallery/GalleryLookAndFeel.h"
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

    /// Fired when prev/next preset buttons are clicked. direction: -1 or +1.
    std::function<void(int direction)> onPresetNav;
    /// Fired when the preset name display is clicked (open browser).
    std::function<void()> onPresetClicked;

    /// Set the displayed preset name (pushed from editor).
    void setPresetName(const juce::String& name)
    {
        if (presetName_ != name)
        {
            presetName_ = name;
            repaint();
        }
    }

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
        // F3-014: layoutControls() removed from paint(); geometry is now cached
        // by resized() and only recomputed when the component's size actually changes.
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

        // Left-align the FX controls — preset display fills the right.
        float startX = padX;

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

        // ── Dot-matrix preset display in the remaining right space ──
        const float presetPadding = 24.0f;
        const float presetLeft = curX + presetPadding;
        const float presetRight = w - padX;
        if (presetRight - presetLeft > 100.0f) // only if enough space
        {
            // Prev/Next buttons (18×18)
            const float btnSz = 18.0f;
            presetPrevBounds_ = juce::Rectangle<float>(presetLeft, midY - btnSz * 0.5f, btnSz, btnSz);
            presetNextBounds_ = juce::Rectangle<float>(presetRight - btnSz, midY - btnSz * 0.5f, btnSz, btnSz);
            // Preset name fills between prev/next
            presetNameBounds_ = juce::Rectangle<float>(
                presetPrevBounds_.getRight() + 8.0f, midY - 10.0f,
                presetNextBounds_.getX() - presetPrevBounds_.getRight() - 16.0f, 20.0f);
            hasPresetDisplay_ = true;
        }
        else
        {
            hasPresetDisplay_ = false;
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
        g.setColour(XO::Tokens::Color::accent().withAlpha(0.06f));
        g.fillRect(0.0f, h - 1.0f, w, 1.0f);

        // Fonts.
        const juce::Font sectionFont = XO::Tokens::Type::heading(XO::Tokens::Type::HeadingSmall); // D3;

        const juce::Font knobLabelFont = XO::Tokens::Type::heading(XO::Tokens::Type::HeadingSmall); // D3;

        const juce::Font advFont = XO::Tokens::Type::heading(XO::Tokens::Type::HeadingSmall); // D3;

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

        // ── Dot-matrix preset display (right side) ──
        if (hasPresetDisplay_)
        {
            // Background panel — dot-matrix feel
            auto displayRect = juce::Rectangle<float>(
                presetPrevBounds_.getX() - 6.0f,
                h * 0.5f - 14.0f,
                presetNextBounds_.getRight() - presetPrevBounds_.getX() + 12.0f,
                28.0f);
            g.setColour(juce::Colour(8, 10, 14).withAlpha(0.60f));
            g.fillRoundedRectangle(displayRect, 4.0f);
            g.setColour(XO::Tokens::Color::accent().withAlpha(0.08f));
            g.drawRoundedRectangle(displayRect, 4.0f, 1.0f);

            // Dot-matrix grid texture (subtle)
            g.setColour(XO::Tokens::Color::accent().withAlpha(0.03f));
            for (float dy = displayRect.getY() + 3.0f; dy < displayRect.getBottom() - 2.0f; dy += 3.0f)
                for (float dx = displayRect.getX() + 3.0f; dx < displayRect.getRight() - 2.0f; dx += 3.0f)
                    g.fillRect(dx, dy, 1.0f, 1.0f);

            // Prev button (◀)
            {
                const auto& pb = presetPrevBounds_;
                g.setColour(juce::Colour(200, 204, 216).withAlpha(hoveredPresetBtn_ == 0 ? 0.6f : 0.25f));
                juce::Path tri;
                tri.addTriangle(pb.getRight() - 4.0f, pb.getY() + 4.0f,
                                pb.getRight() - 4.0f, pb.getBottom() - 4.0f,
                                pb.getX() + 4.0f, pb.getCentreY());
                g.fillPath(tri);
            }

            // Next button (▶)
            {
                const auto& nb = presetNextBounds_;
                g.setColour(juce::Colour(200, 204, 216).withAlpha(hoveredPresetBtn_ == 1 ? 0.6f : 0.25f));
                juce::Path tri;
                tri.addTriangle(nb.getX() + 4.0f, nb.getY() + 4.0f,
                                nb.getX() + 4.0f, nb.getBottom() - 4.0f,
                                nb.getRight() - 4.0f, nb.getCentreY());
                g.fillPath(tri);
            }

            // Preset name — dot-matrix monospace style
            g.setFont(GalleryFonts::dotMatrix(12.0f));
            g.setColour(juce::Colour(127, 219, 202).withAlpha(0.75f));
            g.drawText(presetName_.isEmpty() ? "INIT" : presetName_,
                       presetNameBounds_.toNearestInt(),
                       juce::Justification::centred, true);
        }
    }

    //--------------------------------------------------------------------------
    // paintKnob — delegates to GalleryLookAndFeel::paintXOceanusKnob so that the
    // compact strip and all GalleryKnob (juce::Slider) rotary controls share one
    // paint implementation.
    //
    // The compact strip omits the radial gradient body (too expensive and visually
    // over-heavy at 32 px in a 48 px dense strip) but uses identical arc geometry,
    // stroke weights, and indicator-dot positions.  The fill colour matches the
    // Ocean teal accent used across the strip chrome.
    static void paintKnob (juce::Graphics& g,
                           float cx, float cy, float r,
                           float value01, bool isHover)
    {
        // Ocean teal fill colour — matches strip accent (127, 219, 202) at 60% alpha.
        // GalleryLookAndFeel::paintXOceanusKnob will further apply a 12% brighter
        // lift on isHover, matching GalleryKnob hover behaviour.
        const juce::Colour fillColour = juce::Colour (127, 219, 202).withAlpha (0.60f);

        GalleryLookAndFeel::paintXOceanusKnob (g, cx, cy, r, value01, isHover, fillColour);

        // I3: Restore center body dot + indicator spoke that Wave 2C consolidation
        // (commit cca0667e) dropped when paint was delegated to GalleryLookAndFeel.
        // NOT in GalleryLookAndFeel — intentionally local so macro knobs are unaffected.

        // Center body dot: filled circle at knob center, radius 35% of knob radius.
        // Uses the same dark body colour as the macro knob palette (4A4A4E tone).
        {
            const juce::Colour bodyDot = juce::Colour (0x4A, 0x4A, 0x4E).withAlpha (isHover ? 1.0f : 0.85f);
            const float dotR = r * 0.35f;
            g.setColour (bodyDot);
            g.fillEllipse (cx - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
        }

        // Indicator spoke: 1.5px line from r*0.25 to r*0.75 at the current value angle.
        // Angle math mirrors GalleryLookAndFeel::paintXOceanusKnob (startAngle = 0.75π,
        // endAngle = 2.25π, JUCE convention: subtract halfPi to map angle → screen coord).
        if (value01 > 0.001f)
        {
            constexpr float kStart = 0.75f * juce::MathConstants<float>::pi;
            constexpr float kEnd   = 2.25f * juce::MathConstants<float>::pi;
            const float valueAngle = kStart + value01 * (kEnd - kStart);
            const float screenAngle = valueAngle - juce::MathConstants<float>::halfPi;
            const float cosA = std::cos (screenAngle);
            const float sinA = std::sin (screenAngle);
            const float x0 = cx + r * 0.25f * cosA;
            const float y0 = cy + r * 0.25f * sinA;
            const float x1 = cx + r * 0.75f * cosA;
            const float y1 = cy + r * 0.75f * sinA;
            const juce::Colour spokeCol = fillColour.withAlpha (isHover ? 1.0f : 0.85f);
            g.setColour (spokeCol);
            juce::Path spoke;
            spoke.startNewSubPath (x0, y0);
            spoke.lineTo (x1, y1);
            g.strokePath (spoke, juce::PathStrokeType (1.5f,
                juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        }
    }

    //==========================================================================
    // Mouse handling

    void mouseDown(const juce::MouseEvent& e) override
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        // Preset display buttons.
        if (hasPresetDisplay_)
        {
            if (presetPrevBounds_.expanded(4.0f).contains(mx, my))
            {
                if (onPresetNav) onPresetNav(-1);
                return;
            }
            if (presetNextBounds_.expanded(4.0f).contains(mx, my))
            {
                if (onPresetNav) onPresetNav(1);
                return;
            }
            if (presetNameBounds_.contains(mx, my))
            {
                if (onPresetClicked) onPresetClicked();
                return;
            }
        }

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
    int hoveredPresetBtn_ = -1; // 0=prev, 1=next, -1=none

    // Preset display state
    juce::String presetName_;
    bool hasPresetDisplay_ = false;
    juce::Rectangle<float> presetPrevBounds_;
    juce::Rectangle<float> presetNextBounds_;
    juce::Rectangle<float> presetNameBounds_;

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
