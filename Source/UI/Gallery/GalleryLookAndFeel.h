#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"

namespace xolokun {

//==============================================================================
// GalleryLookAndFeel — Gallery Model visual language (WCAG 2.1 AA compliant)
class GalleryLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GalleryLookAndFeel() { applyTheme(); }

    // Re-apply all theme colors — call after toggling GalleryColors::darkMode().
    void applyTheme()
    {
        using namespace GalleryColors;
        setColour(juce::Slider::rotarySliderFillColourId,        get(xoGold));
        setColour(juce::Slider::rotarySliderOutlineColourId,     get(borderGray()));
        setColour(juce::Slider::textBoxTextColourId,             get(textMid()));
        setColour(juce::Slider::textBoxBackgroundColourId,       juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId,          juce::Colours::transparentBlack);
        setColour(juce::TextButton::buttonColourId,              get(shellWhite()));
        setColour(juce::TextButton::buttonOnColourId,            get(xoGold));
        setColour(juce::TextButton::textColourOffId,             get(textDark()));
        setColour(juce::ComboBox::backgroundColourId,            get(shellWhite()));
        setColour(juce::ComboBox::outlineColourId,               get(borderGray()));
        setColour(juce::PopupMenu::backgroundColourId,           get(shellWhite()));
        setColour(juce::PopupMenu::textColourId,                 get(textDark()));
        setColour(juce::PopupMenu::highlightedBackgroundColourId,get(xoGold).withAlpha(0.25f));
        setColour(juce::ScrollBar::thumbColourId,                get(borderGray()));
        setColour(juce::Label::textColourId,                     get(textDark()));
    }

    // Ecological Instrument paradigm — zone arcs + setpoint marker + value arc.
    // Visual language mirrors the web PlaySurface:
    //   Track ring   : faint full-sweep baseline
    //   Zone arcs    : Sunlit/Twilight/Midnight depth bands at 0.22 opacity
    //   Value arc    : engine accent color, shows current parameter value
    //   Setpoint ▲   : XO Gold triangle pointing outward at current position
    //   Center disc  : warm white inner field (Gallery Model shell)
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(6.0f);
        float r     = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        float cx    = bounds.getCentreX();
        float cy    = bounds.getCentreY();
        float sweep = endAngle - startAngle;
        float fillAngle = startAngle + sliderPos * sweep;
        float arcR  = r - 2.0f;

        // Focus ring (WCAG 2.4.7)
        if (slider.hasKeyboardFocus(true))
            A11y::drawCircularFocusRing(g, cx, cy, r + 2.0f);

        // ── 1. Ecological depth zone arcs ────────────────────────────────
        // Wider than the track so they create a soft halo behind it.
        // Sunlit 0–55% | Twilight 55–80% | Midnight 80–100% of sweep.
        struct ZoneBand { float normStart, normEnd; uint32_t rgba; };
        constexpr ZoneBand kZones[3] = {
            { 0.00f, 0.55f, 0xFF48CAE4 },   // Sunlit  — tropical cyan
            { 0.55f, 0.80f, 0xFF0096C7 },   // Twilight — deep blue
            { 0.80f, 1.00f, 0xFF7B2FBE }    // Midnight — bioluminescent violet
        };
        for (const auto& z : kZones)
        {
            float a0 = startAngle + z.normStart * sweep;
            float a1 = startAngle + z.normEnd   * sweep;
            juce::Path zp;
            zp.addCentredArc(cx, cy, arcR, arcR, 0.0f, a0, a1, true);
            g.setColour(juce::Colour(z.rgba).withAlpha(0.22f));
            g.strokePath(zp, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        }

        // ── 2. Track ring — full-sweep baseline ──────────────────────────
        {
            juce::Path track;
            track.addCentredArc(cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
            g.setColour(GalleryColors::get(GalleryColors::borderGray()).withAlpha(0.40f));
            g.strokePath(track, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }

        // ── 3. Value arc — engine accent color ───────────────────────────
        if (sliderPos > 0.001f)
        {
            juce::Path fill;
            fill.addCentredArc(cx, cy, arcR, arcR, 0.0f, startAngle, fillAngle, true);
            auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);
            g.setColour(accent);
            g.strokePath(fill, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }

        // ── 4. Center disc — Gallery Model warm white inner field ────────
        {
            float discR = r * 0.44f;
            g.setColour(GalleryColors::get(GalleryColors::shellWhite()));
            g.fillEllipse(cx - discR, cy - discR, discR * 2.0f, discR * 2.0f);
            g.setColour(GalleryColors::get(GalleryColors::borderGray()).withAlpha(0.35f));
            g.drawEllipse(cx - discR, cy - discR, discR * 2.0f, discR * 2.0f, 1.0f);

            // Live value readout during interaction — center disc becomes the face
            if (slider.isMouseButtonDown() || slider.isMouseOverOrDragging())
            {
                juce::String valStr;
                double val = slider.getValue();
                // Integers for whole-number ranges (e.g. semitones, MIDI CCs);
                // one decimal place for continuous parameters.
                if (slider.getInterval() >= 1.0 && slider.getMaximum() <= 127.0)
                    valStr = juce::String((int)val);
                else
                    valStr = juce::String(val, 1);

                float fontSize = juce::jmax(7.0f, discR * 0.7f);
                g.setFont(GalleryFonts::value(fontSize));
                g.setColour(GalleryColors::get(GalleryColors::textDark()).withAlpha(0.85f));
                g.drawText(valStr,
                           juce::Rectangle<float>(cx - discR, cy - discR, discR * 2.0f, discR * 2.0f),
                           juce::Justification::centred);
            }
        }

        // ── 5. Setpoint triangle ▲ — XO Gold marker at player's intent ──
        // Tip at the arc edge, base pointing inward. Screen coords: Y increases downward,
        // so point at angle θ = (cx + arcR·sin θ, cy − arcR·cos θ).
        {
            float tipR   = arcR + 1.5f;
            float tipX   = cx + tipR * std::sin(fillAngle);
            float tipY   = cy - tipR * std::cos(fillAngle);

            // Unit vectors: radial outward (rdx,rdy) and perpendicular (pdx,pdy)
            float rdx =  std::sin(fillAngle);
            float rdy = -std::cos(fillAngle);
            float pdx = -rdy;   // rotate radial 90° CCW in screen space
            float pdy =  rdx;

            constexpr float kTriH = 5.5f;   // height (depth into arc)
            constexpr float kTriW = 3.2f;   // half-width of base

            juce::Path tri;
            tri.startNewSubPath(tipX, tipY);
            tri.lineTo(tipX - rdx * kTriH + pdx * kTriW,
                       tipY - rdy * kTriH + pdy * kTriW);
            tri.lineTo(tipX - rdx * kTriH - pdx * kTriW,
                       tipY - rdy * kTriH - pdy * kTriW);
            tri.closeSubPath();

            g.setColour(juce::Colour(GalleryColors::xoGold));
            g.fillPath(tri);
        }
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour& bg, bool highlighted, bool down) override
    {
        auto b = btn.getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(down ? GalleryColors::get(GalleryColors::xoGold)
                         : highlighted ? bg.brighter(0.06f) : bg);
        g.fillRoundedRectangle(b, 5.0f);

        // Focus ring (WCAG 2.4.7) — use focus colour instead of border when focused
        if (btn.hasKeyboardFocus (true))
        {
            g.setColour (A11y::focusRingColour());
            g.drawRoundedRectangle (b, 5.0f, 2.0f);
        }
        else
        {
            g.setColour(GalleryColors::get(GalleryColors::borderGray()));
            g.drawRoundedRectangle(b, 5.0f, 1.0f);
        }
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                      juce::ComboBox& box) override
    {
        using namespace GalleryColors;
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

        // Rounded container
        g.setColour(get(slotBg()));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Border
        g.setColour(get(borderGray()).withAlpha(box.hasKeyboardFocus(true) ? 0.8f : 0.4f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

        // Dropdown arrow (right side)
        auto arrowZone = bounds.removeFromRight(height * 0.7f).reduced(height * 0.25f);
        juce::Path arrow;
        arrow.addTriangle(arrowZone.getX(), arrowZone.getY(),
                          arrowZone.getRight(), arrowZone.getY(),
                          arrowZone.getCentreX(), arrowZone.getBottom());
        g.setColour(get(textMid()).withAlpha(isButtonDown ? 0.9f : 0.55f));
        g.fillPath(arrow);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        using namespace GalleryColors;

        bool isHorizontal = (style == juce::Slider::LinearHorizontal ||
                             style == juce::Slider::LinearBar);

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();

        // Track background
        auto trackBounds = isHorizontal
            ? bounds.withSizeKeepingCentre(bounds.getWidth(), 4.0f)
            : bounds.withSizeKeepingCentre(4.0f, bounds.getHeight());

        g.setColour(get(borderGray()).withAlpha(0.3f));
        g.fillRoundedRectangle(trackBounds, 2.0f);

        // Filled portion
        auto fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
        if (fillColour.isTransparent()) fillColour = get(xoGold);

        auto filledBounds = trackBounds;
        if (isHorizontal)
            filledBounds.setWidth(sliderPos - (float)x);
        else
            filledBounds = filledBounds.withTop(sliderPos);

        g.setColour(fillColour);
        g.fillRoundedRectangle(filledBounds, 2.0f);

        // Thumb
        float thumbSize = isHorizontal ? 12.0f : 12.0f;
        juce::Point<float> thumbCenter;
        if (isHorizontal)
            thumbCenter = { sliderPos, bounds.getCentreY() };
        else
            thumbCenter = { bounds.getCentreX(), sliderPos };

        // Shadow
        g.setColour(juce::Colours::black.withAlpha(0.15f));
        g.fillEllipse(thumbCenter.x - thumbSize/2 + 1, thumbCenter.y - thumbSize/2 + 1, thumbSize, thumbSize);

        // Thumb body
        g.setColour(fillColour.brighter(0.15f));
        g.fillEllipse(thumbCenter.x - thumbSize/2, thumbCenter.y - thumbSize/2, thumbSize, thumbSize);

        // Thumb highlight
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        g.fillEllipse(thumbCenter.x - thumbSize/4, thumbCenter.y - thumbSize/4, thumbSize/2, thumbSize/2);
    }

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        using namespace GalleryColors;
        g.fillAll(get(slotBg()));
        g.setColour(get(borderGray()).withAlpha(0.3f));
        g.drawRect(0, 0, width, height, 1);
    }

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool /*hasSubMenu*/,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* /*icon*/, const juce::Colour* textColourToUse) override
    {
        using namespace GalleryColors;

        if (isSeparator)
        {
            auto sepArea = area.reduced(8, 0);
            g.setColour(get(borderGray()).withAlpha(0.25f));
            g.fillRect(sepArea.getX(), sepArea.getCentreY(), sepArea.getWidth(), 1);
            return;
        }

        auto textArea = area.reduced(12, 0);

        if (isHighlighted && isActive)
        {
            g.setColour(get(xoGold).withAlpha(0.15f));
            g.fillRoundedRectangle(area.toFloat().reduced(2, 1), 3.0f);
        }

        auto textColour = isActive ? get(textDark()) : get(textMid()).withAlpha(0.5f);
        if (textColourToUse) textColour = *textColourToUse;

        g.setColour(textColour);
        g.setFont(GalleryFonts::body(13.0f));
        g.drawFittedText(text, textArea, juce::Justification::centredLeft, 1);

        if (isTicked)
        {
            g.setColour(get(xoGold));
            auto tickBounds = area.toFloat().removeFromLeft(24.0f).reduced(6.0f);
            g.fillEllipse(tickBounds.getCentreX() - 3, tickBounds.getCentreY() - 3, 6, 6);
        }

        if (shortcutKeyText.isNotEmpty())
        {
            g.setColour(get(textMid()).withAlpha(0.45f));
            g.setFont(GalleryFonts::value(10.0f));
            g.drawFittedText(shortcutKeyText, textArea, juce::Justification::centredRight, 1);
        }
    }
};

} // namespace xolokun
