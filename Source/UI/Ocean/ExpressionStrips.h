// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// ExpressionStrips.h — Pitch Bend + Mod Wheel vertical strips for the XOceanus Ocean View.
//
// A 36px-wide vertical column that sits to the left of the PlaySurface.  It
// contains two stacked expression strips:
//
//   PB (Pitch Bend) — spring-returns to centre (0) on mouseUp.  Value range -1..+1.
//   MW (Mod Wheel)  — latches at the released position.  Value range 0..+1.
//
// Both strips are drawn with all-custom JUCE paint calls (no JUCE widgets).
// Callbacks fire on every drag update so the host can forward MIDI CC/pitch-bend.
//
// Visual spec:
//   Container     36px wide, full parent height, padding 4px, gap 4px between strips.
//   Strip         28px wide, centred, rgba(200,204,216,0.03) bg, 1px border
//                 rgba(200,204,216,0.06), radius 4px.
//   Fill bar      Bottom-anchored, width 100%, rgba(127,219,202,0.30).
//                 PB: 50% at rest.  MW: 0% at rest.
//   Centre line   PB only — 1px horizontal at 50%, rgba(200,204,216,0.15).
//   Label         Bottom-centre, 7px bold, rgba(200,204,216,0.30), tracking 0.5px.
//
// Usage:
//   auto strips = std::make_unique<ExpressionStrips>();
//   strips->setBounds(0, 0, ExpressionStrips::kStripWidth, parentHeight);
//   strips->onPitchBend = [this](float v){ sendMidiPitchBend(v); };
//   strips->onModWheel  = [this](float v){ sendMidiCC(1, v); };
//   addAndMakeVisible(*strips);

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <algorithm>
#include <cmath>

namespace xoceanus
{

//==============================================================================
/**
    ExpressionStrips

    Vertical dual-strip expression controller: Pitch Bend (spring-return) and
    Mod Wheel (latch).  See file header for full visual and interaction spec.
*/
class ExpressionStrips : public juce::Component
{
public:
    //==========================================================================
    static constexpr int kStripWidth = 36;

    //==========================================================================
    // Callbacks fired on each value change — set before first use.
    std::function<void(float /*value*/)> onPitchBend; // -1..+1
    std::function<void(float /*value*/)> onModWheel;  //  0..+1

    //==========================================================================
    ExpressionStrips()
    {
        setInterceptsMouseClicks(true, true);
    }

    ~ExpressionStrips() override = default;

    //==========================================================================
    // Push values externally (e.g. received MIDI input).
    void setPitchBend(float v)
    {
        pbValue_ = juce::jlimit(-1.0f, 1.0f, v);
        repaint();
    }

    void setModWheel(float v)
    {
        mwValue_ = juce::jlimit(0.0f, 1.0f, v);
        repaint();
    }

private:
    //==========================================================================
    // Internal state
    float pbValue_   = 0.0f;   // -1..+1,  0 = centre
    float mwValue_   = 0.0f;   //  0..+1,  0 = bottom
    int   dragging_  = -1;     // 0=PB, 1=MW, -1=none

    //==========================================================================
    // Layout helpers — computed fresh from current bounds each call.
    // Padding and gap match the CSS-prototype spec.
    static constexpr int kPad  = 4;
    static constexpr int kGap  = 4;
    static constexpr int kInnerW = 28; // strip inner width
    static constexpr int kLabelH = 14; // reserved pixels at strip bottom for label

    struct StripBounds
    {
        juce::Rectangle<int> strip; // the full strip rect inside the component
        juce::Rectangle<int> inner; // 28px wide, centred inside strip
    };

    // Returns rects for strip index (0=PB, 1=MW) based on current component bounds.
    StripBounds stripBounds(int index) const noexcept
    {
        const int totalH = getHeight();
        const int availH = totalH - 2 * kPad - kGap;
        const int h0     = availH / 2;
        const int h1     = availH - h0;

        const int y = (index == 0) ? kPad
                                   : kPad + h0 + kGap;
        const int h = (index == 0) ? h0 : h1;

        juce::Rectangle<int> strip (0, y, getWidth(), h);
        const int innerX = (getWidth() - kInnerW) / 2;
        juce::Rectangle<int> inner (innerX, y, kInnerW, h);

        return { strip, inner };
    }

    // Map mouseY (relative to component) to a normalised [0,1] value within a strip.
    // Returns 1.0 at the top of the strip, 0.0 at the bottom.
    float mousePosToNorm(int mouseY, const juce::Rectangle<int>& inner) const noexcept
    {
        const float rel = static_cast<float>(mouseY - inner.getY());
        const float norm = 1.0f - (rel / static_cast<float>(inner.getHeight()));
        return juce::jlimit(0.0f, 1.0f, norm);
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        for (int i = 0; i < 2; ++i)
            paintStrip(g, i);
    }

    void paintStrip(juce::Graphics& g, int index) const
    {
        const auto [strip, inner] = stripBounds(index);

        // ---- Background fill ----
        g.setColour(juce::Colour::fromFloatRGBA(200.0f/255.0f,
                                                 204.0f/255.0f,
                                                 216.0f/255.0f,
                                                 0.03f));
        g.fillRoundedRectangle(inner.toFloat(), 4.0f);

        // ---- Border ----
        g.setColour(juce::Colour::fromFloatRGBA(200.0f/255.0f,
                                                 204.0f/255.0f,
                                                 216.0f/255.0f,
                                                 0.06f));
        g.drawRoundedRectangle(inner.toFloat().reduced(0.5f), 4.0f, 1.0f);

        // ---- Fill bar ----
        const float fillFrac  = (index == 0)
                                ? 0.5f + pbValue_ * 0.5f   // PB: 0.5 at rest
                                : mwValue_;                  // MW: 0 at rest

        const int fillH  = juce::roundToInt(fillFrac * static_cast<float>(inner.getHeight()));
        const int fillY  = inner.getBottom() - fillH;

        juce::Rectangle<int> fillRect (inner.getX(), fillY, inner.getWidth(), fillH);

        // Clip to inner rect (handles edge case when fillH > inner.getHeight())
        fillRect = fillRect.getIntersection(inner);

        if (fillRect.getHeight() > 0)
        {
            g.setColour(juce::Colour::fromFloatRGBA(127.0f/255.0f,
                                                     219.0f/255.0f,
                                                     202.0f/255.0f,
                                                     0.30f));
            // Clip paint to inner rounded rect via a saved state + clip path
            {
                juce::Graphics::ScopedSaveState saved (g);
                juce::Path clipPath;
                clipPath.addRoundedRectangle(inner.toFloat(), 4.0f);
                g.reduceClipRegion(clipPath);
                g.fillRect(fillRect);
            }
        }

        // ---- Centre line (PB only) ----
        if (index == 0)
        {
            const int lineY = inner.getY() + inner.getHeight() / 2;
            g.setColour(juce::Colour::fromFloatRGBA(200.0f/255.0f,
                                                     204.0f/255.0f,
                                                     216.0f/255.0f,
                                                     0.15f));
            g.drawHorizontalLine(lineY, static_cast<float>(inner.getX()),
                                        static_cast<float>(inner.getRight()));
        }

        // ---- Label ----
        const char* label = (index == 0) ? "PB" : "MW";

        juce::Font labelFont (juce::FontOptions{}
                                .withHeight(7.0f)
                                .withStyle("Bold"));
        g.setFont(labelFont);
        g.setColour(juce::Colour::fromFloatRGBA(200.0f/255.0f,
                                                 204.0f/255.0f,
                                                 216.0f/255.0f,
                                                 0.30f));

        const juce::Rectangle<int> labelArea (inner.getX(),
                                               inner.getBottom() - kLabelH,
                                               inner.getWidth(),
                                               kLabelH);
        g.drawFittedText(label, labelArea,
                         juce::Justification::centred, /*maxLines=*/ 1);
    }

    //==========================================================================
    // Returns 0 if the point is over the PB strip, 1 for MW, -1 for neither.
    int hitTestStrip(const juce::Point<int>& pos) const noexcept
    {
        for (int i = 0; i < 2; ++i)
        {
            const auto [strip, inner] = stripBounds(i);
            (void)strip; // unused — hit-test on the inner rect for best UX
            if (inner.expanded(4).contains(pos))
                return i;
        }
        return -1;
    }

    //==========================================================================
    void mouseDown(const juce::MouseEvent& e) override
    {
        dragging_ = hitTestStrip(e.getPosition());
        if (dragging_ >= 0)
            updateValueFromMouse(e.getPosition());
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (dragging_ >= 0)
            updateValueFromMouse(e.getPosition());
    }

    void mouseUp(const juce::MouseEvent& /*e*/) override
    {
        if (dragging_ == 0)
        {
            // Pitch Bend spring-returns to centre.
            pbValue_ = 0.0f;
            if (onPitchBend)
                onPitchBend(pbValue_);
            repaint();
        }
        dragging_ = -1;
    }

    //==========================================================================
    void updateValueFromMouse(const juce::Point<int>& pos)
    {
        if (dragging_ < 0)
            return;

        const auto [strip, inner] = stripBounds(dragging_);
        (void)strip;
        const float norm = mousePosToNorm(pos.getY(), inner);

        if (dragging_ == 0)
        {
            // Norm 0..1 → PB -1..+1
            pbValue_ = juce::jlimit(-1.0f, 1.0f, norm * 2.0f - 1.0f);
            if (onPitchBend)
                onPitchBend(pbValue_);
        }
        else
        {
            mwValue_ = norm;
            if (onModWheel)
                onModWheel(mwValue_);
        }

        repaint();
    }

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExpressionStrips)
};

} // namespace xoceanus
