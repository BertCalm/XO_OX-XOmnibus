// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include <map>

namespace xoceanus
{

//==============================================================================
// GalleryLookAndFeel — Gallery Model visual language (WCAG 2.1 AA compliant)
// Matched to v05 dark-mode-primary spec.
class GalleryLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GalleryLookAndFeel() { applyTheme(); }

    //==========================================================================
    // paintXOceanusKnob — shared static helper used by both drawRotarySlider
    // (GalleryKnob / juce::Slider) and MasterFXStripCompact::paintKnob (custom
    // Component that draws knobs directly without JUCE Slider children).
    //
    // Renders the arc track, value-fill arc, and indicator line.  The caller is
    // responsible for the knob body (drawn via the cached image in
    // drawRotarySlider, or skipped in the compact strip where the body is
    // omitted for density).
    //
    // Parameters
    //   cx, cy       — centre of the knob in component coordinates
    //   r            — arc radius (typically knob_radius - 3 px)
    //   value01      — normalised value [0, 1]
    //   isHover      — true when the pointer is over the knob
    //   fillColour   — colour for the value arc and indicator line
    //   startAngle   — arc start in radians (7-o'clock = 0.75π)
    //   endAngle     — arc end   in radians (5-o'clock = 2.25π)
    static void paintXOceanusKnob (juce::Graphics& g,
                                   float cx, float cy, float r,
                                   float value01, bool isHover,
                                   juce::Colour fillColour,
                                   float startAngle = 0.75f * juce::MathConstants<float>::pi,
                                   float endAngle   = 2.25f * juce::MathConstants<float>::pi)
    {
        using juce::Path;
        using juce::PathStrokeType;
        using juce::MathConstants;

        const float totalSweep = endAngle - startAngle;
        const float valueAngle = startAngle + value01 * totalSweep;

        // ── Track arc (full sweep, dimmed) ────────────────────────────────
        {
            Path bgArc;
            bgArc.addCentredArc (cx, cy, r, r, 0.0f, startAngle, endAngle, true);
            g.setColour (juce::Colour (GalleryColors::t3()));
            g.strokePath (bgArc, PathStrokeType (2.2f,
                PathStrokeType::curved, PathStrokeType::rounded));
        }

        // ── Value arc (partial sweep) ─────────────────────────────────────
        if (value01 > 0.001f)
        {
            // 12 % brightness boost on passive hover — consistent with
            // drawRotarySlider behaviour (see #1185 note in that method).
            const juce::Colour arcCol = isHover ? fillColour.brighter (0.12f) : fillColour;

            Path valArc;
            valArc.addCentredArc (cx, cy, r, r, 0.0f, startAngle, valueAngle, true);
            g.setColour (arcCol);
            g.strokePath (valArc, PathStrokeType (2.8f,
                PathStrokeType::curved, PathStrokeType::rounded));

            // ── Indicator dot at arc endpoint ─────────────────────────────
            // screen coords: x = cx + r·cos(angle − π/2)
            //                y = cy + r·sin(angle − π/2)
            const float dotX = cx + r * std::cos (valueAngle - MathConstants<float>::halfPi);
            const float dotY = cy + r * std::sin (valueAngle - MathConstants<float>::halfPi);
            constexpr float dotR = 1.8f;
            g.setColour (arcCol.withAlpha (0.9f));
            g.fillEllipse (dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
        }
    }

    // Re-apply all theme colors — call after toggling GalleryColors::darkMode().
    void applyTheme()
    {
        using namespace GalleryColors;

        // Rotary sliders
        setColour(juce::Slider::rotarySliderFillColourId, get(xoGold));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(t3()));
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(t2()));
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

        // Buttons
        setColour(juce::TextButton::buttonColourId, juce::Colour(elevated()));
        setColour(juce::TextButton::buttonOnColourId, get(xoGold));
        setColour(juce::TextButton::textColourOffId, juce::Colour(t1()));
        setColour(juce::TextButton::textColourOnId, juce::Colour(darkMode() ? Dark::bg : Light::textDark));

        // ComboBox
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(elevated()));
        setColour(juce::ComboBox::outlineColourId, border());
        setColour(juce::ComboBox::textColourId, juce::Colour(t1()));
        setColour(juce::ComboBox::arrowColourId, juce::Colour(t2()));

        // PopupMenu
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(elevated()));
        setColour(juce::PopupMenu::textColourId, juce::Colour(t1()));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, get(xoGold).withAlpha(0.15f));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(t1()));

        // ScrollBar
        setColour(juce::ScrollBar::thumbColourId, juce::Colour(t4()));
        setColour(juce::ScrollBar::backgroundColourId, juce::Colour(elevated()));

        // Labels
        setColour(juce::Label::textColourId, juce::Colour(t1()));
        setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);

        // ListBox / table
        setColour(juce::ListBox::backgroundColourId, juce::Colour(elevated()));
        setColour(juce::ListBox::textColourId, juce::Colour(t1()));
        setColour(juce::ListBox::outlineColourId, border());
    }

    //==========================================================================
    // drawRotarySlider — matches prototype knob exactly
    // Body: radial gradient (circle at 38% 32%, #4A4A4E 0%, #3A3A3E 25%, #282830 55%, #141418 100%)
    //       Body is rendered from a cached juce::Image (keyed by integer diameter)
    //       so the ColourGradient is only constructed once per distinct knob size —
    //       Lucy's gate: no per-frame gradient recomputation.
    // Arc track + fill + indicator dot: delegated to paintXOceanusKnob() so that
    //       MasterFXStripCompact and any other custom components share one path.
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                          float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override
    {
        using namespace GalleryColors;

        const bool enabled = slider.isEnabled();
        if (!enabled)
            g.setOpacity(0.35f);

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
        float radius = diameter * 0.5f;
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();

        // ── 1. Knob body — radial gradient, cached as juce::Image per diameter ──
        // Lucy's gate: ColourGradient is only constructed when a new knob size
        // appears (typically 3-5 distinct sizes per session).  The image is drawn
        // at component coordinates via drawImageAt with clipping, so each size
        // shares one cached texture. Cache is keyed by integer pixel diameter;
        // a mutable member is safe because LookAndFeel paint calls run on the
        // message thread exclusively.
        {
            const int iDiam = juce::roundToInt (diameter);
            auto it = bodyImageCache_.find (iDiam);
            if (it == bodyImageCache_.end())
            {
                // Build the body image at the exact integer size.
                juce::Image img (juce::Image::ARGB, iDiam, iDiam, true);
                juce::Graphics ig (img);

                const float r2 = iDiam * 0.5f;
                const float icx = r2, icy = r2;

                juce::ColourGradient grad (
                    juce::Colour (0xFF4A4A4E),
                    icx - r2 * 0.24f, icy - r2 * 0.36f,
                    juce::Colour (0xFF141418),
                    icx + r2, icy + r2,
                    true /* radial */);
                grad.addColour (0.25, juce::Colour (0xFF3A3A3E));
                grad.addColour (0.55, juce::Colour (0xFF282830));
                ig.setGradientFill (grad);
                ig.fillEllipse (0.0f, 0.0f, (float)iDiam, (float)iDiam);

                // Border ring
                ig.setColour (GalleryColors::border());
                ig.drawEllipse (0.0f, 0.0f, (float)iDiam, (float)iDiam, 1.0f);

                // Inner highlight — top-left specular
                ig.setColour (juce::Colour (0xFFFFFFFF).withAlpha (0.09f));
                ig.drawEllipse (1.0f, 1.0f, (float)(iDiam - 2), (float)(iDiam - 2), 0.5f);

                bodyImageCache_.emplace (iDiam, std::move (img));
                it = bodyImageCache_.find (iDiam);
            }

            // Draw the cached body image centred on the knob.
            const float imgX = cx - diameter * 0.5f;
            const float imgY = cy - diameter * 0.5f;
            g.drawImageAt (it->second, juce::roundToInt (imgX), juce::roundToInt (imgY));
        }

        // ── 2–6. Arc track + fill + indicator dot ──────────────────────────
        // Resolved via the shared static helper so all rotary controls
        // (GalleryKnob sliders, MasterFXStripCompact custom knobs, etc.)
        // use an identical paint path.
        float arcRadius = radius - 3.0f;

        // Passive hover: read the cached "hovered" property set by
        // GalleryKnob::mouseEnter / mouseExit instead of calling
        // Slider::isMouseOver() inside paint — on D2D-backed JUCE,
        // isMouseOver() can return the previous frame's value during fast
        // mouse moves and produce a one-frame flicker (#1185).
        bool hasHoveredProp = false;
        bool hoveredProp    = false;
        if (auto* v = slider.getProperties().getVarPointer ("hovered"))
        {
            hasHoveredProp = true;
            hoveredProp    = static_cast<bool> (*v);
        }
        const bool isPassiveHover = enabled
            && (hasHoveredProp ? hoveredProp : slider.isMouseOver())
            && !slider.isMouseButtonDown();

        auto fillColour = slider.findColour (juce::Slider::rotarySliderFillColourId);
        if (fillColour.isTransparent())
            fillColour = get (xoGold);

        paintXOceanusKnob (g, cx, cy, arcRadius,
                           enabled ? sliderPos : 0.0f,
                           isPassiveHover, fillColour,
                           rotaryStartAngle, rotaryEndAngle);

        // ── 6b. Modulation arc — colored arc from current value, depth = modAmount ──
        // Only drawn on knobs >= 28px (skip tiny knobs where the track is too thin
        // to distinguish an extra arc layer).  Data comes from GalleryKnob::setModulation
        // stored in the slider's NamedValueSet properties so the LookAndFeel
        // signature remains unchanged.
        if (diameter >= 28.0f)
        {
            const float modAmt = static_cast<float>(slider.getProperties()["modAmount"]);
            if (std::abs(modAmt) > 0.001f)
            {
                // Reconstruct the coupling-type colour from the stored ARGB value.
                // juce::int64 (not int64_t) — see GalleryKnob.h note.
                const juce::int64 argb = static_cast<juce::int64>(slider.getProperties()["modColour"]);
                juce::Colour modCol  = (argb != 0)
                                           ? juce::Colour(static_cast<juce::uint32>(argb))
                                           : juce::Colour(0xFF4488FF); // fallback blue

                // The mod arc extends from the current fill-end angle by modAmt
                // fraction of the full arc sweep, clamped to the track bounds.
                const float arcSweep = rotaryEndAngle - rotaryStartAngle;
                const float modStart = rotaryStartAngle + sliderPos * arcSweep;
                float modEnd   = modStart + modAmt * arcSweep;
                modEnd = juce::jlimit(rotaryStartAngle, rotaryEndAngle, modEnd);

                if (std::abs(modEnd - modStart) > 0.001f)
                {
                    // Arc body — 2.4px, 45% alpha (thinner + more transparent
                    // than the 2.8px fill arc so layers remain readable).
                    juce::Path modArc;
                    modArc.addCentredArc(cx, cy, arcRadius, arcRadius, 0.0f,
                                         juce::jmin(modStart, modEnd),
                                         juce::jmax(modStart, modEnd), true);
                    g.setColour(modCol.withAlpha(0.45f));
                    g.strokePath(modArc,
                                 juce::PathStrokeType(2.4f,
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

                    // Endpoint dot — 2px radius, 70% alpha, marks the modulated
                    // destination.  Uses the same trig convention as the fill dot:
                    //   screen_x = cx + r * cos(angle - pi/2)
                    //   screen_y = cy + r * sin(angle - pi/2)
                    constexpr float modDotR = 2.0f;
                    const float dotX = cx + arcRadius * std::cos(modEnd - juce::MathConstants<float>::halfPi);
                    const float dotY = cy + arcRadius * std::sin(modEnd - juce::MathConstants<float>::halfPi);
                    g.setColour(modCol.withAlpha(0.70f));
                    g.fillEllipse(dotX - modDotR, dotY - modDotR, modDotR * 2.0f, modDotR * 2.0f);
                }
            }
        }

        // ── 6c. Passive hover ring — 1px translucent ring, only when hovering ─
        // Shown at all slider positions (including zero) so the user always gets
        // visual confirmation that the knob is interactive when they hover over it.
        if (isPassiveHover)
        {
            g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.10f));
            g.drawEllipse(cx - radius, cy - radius, diameter, diameter, 1.0f);
        }

        // ── 7. Focus ring (WCAG 2.4.7) ────────────────────────────────────
        if (slider.hasKeyboardFocus(true))
        {
            g.setColour(A11y::focusRingColour().withAlpha(1.0f));
            g.drawEllipse(cx - radius - 2.0f, cy - radius - 2.0f, diameter + 4.0f, diameter + 4.0f, 2.0f);
        }

        // ── 8. Live value readout (during hover or interaction) ────────────
        // 28px knobs and below suppress in-knob readout — disc would be
        // only 14px wide with ~9.8px text, which clips. Tooltip handles it.
        // Hover, not just drag (#1165) — producers should be able to read a
        // parameter value without disturbing the control. Reuse the same
        // cached "hovered" property used by the passive-hover fill above so
        // the readout doesn't flicker on fast mouse moves (#1185).
        const bool isReadoutHovered = hasHoveredProp ? hoveredProp : slider.isMouseOver();
        if (diameter >= 28.0f
            && (isReadoutHovered
                || slider.isMouseButtonDown() || slider.isMouseOverOrDragging()))
        {
            float discR = radius * 0.44f;
            juce::String valStr;
            double val = slider.getValue();
            if (slider.getInterval() >= 1.0 && slider.getMaximum() <= 127.0)
                valStr = juce::String((int)val);
            else
                valStr = juce::String(val, 1);

            float fontSize = juce::jmax(8.0f, discR * 0.7f);
            g.setFont(GalleryFonts::value(fontSize));
            g.setColour(juce::Colour(GalleryColors::t1()).withAlpha(0.85f));
            g.drawText(valStr, juce::Rectangle<float>(cx - discR, cy - discR, discR * 2.0f, discR * 2.0f),
                       juce::Justification::centred);
        }
    }

    //==========================================================================
    // Button style helpers — set once at construction, read zero-alloc in paint.
    // Call these on any TextButton to opt it in to the corresponding style.
    static void setMoodPillStyle(juce::TextButton& btn) { btn.getProperties().set("moodPill", true); }
    static bool isMoodPill(const juce::Button& btn) { return btn.getProperties()["moodPill"]; }

    // Export button (gold fill) and Panic button (red accent) use property tags
    // instead of btn.getName() string comparison — avoids allocation per paint frame.
    static void setExportButtonStyle(juce::TextButton& btn) { btn.getProperties().set("buttonType", juce::String("export")); }
    static void setPanicButtonStyle(juce::TextButton& btn)  { btn.getProperties().set("buttonType", juce::String("panic")); }

private:
    // bodyImageCache_ — Lucy's gate: one juce::Image per integer knob diameter.
    // The radial gradient for the knob body is computed once per distinct size and
    // stored here. LookAndFeel paint methods run on the message thread, so a
    // mutable std::map is safe without additional locking.
    mutable std::map<int, juce::Image> bodyImageCache_;

    static bool isExportButton(const juce::Button& btn)
    {
        const juce::var& t = btn.getProperties()["buttonType"];
        return t.isString() && t.toString() == "export";
    }
    static bool isPanicButton(const juce::Button& btn)
    {
        const juce::var& t = btn.getProperties()["buttonType"];
        return t.isString() && t.toString() == "panic";
    }
public:

    //==========================================================================
    // Button style tagging — O(1) integer lookup instead of per-repaint
    // String::containsIgnoreCase() on every button's name (#1161).
    static constexpr int kBtnStyleDefault = 0;
    static constexpr int kBtnStyleExport  = 1;
    static constexpr int kBtnStylePanic   = 2;

    static void setButtonStyle(juce::Button& btn, int style) { btn.getProperties().set("btnStyle", style); }

    static int getButtonStyle(const juce::Button& btn) noexcept
    {
        const auto& p = btn.getProperties();
        const auto* v = p.getVarPointer("btnStyle");
        return v != nullptr ? static_cast<int>(*v) : kBtnStyleDefault;
    }

    //==========================================================================
    // drawButtonBackground — matches prototype button styles
    void drawButtonBackground(juce::Graphics& g, juce::Button& btn, const juce::Colour& /*bgColour*/, bool isOver,
                              bool isDown) override
    {
        using namespace GalleryColors;

        auto bounds = btn.getLocalBounds().toFloat();

        if (!btn.isEnabled())
        {
            constexpr float cornerRadius = 5.0f;
            g.setColour(get(GalleryColors::elevated()).withAlpha(0.5f));
            g.fillRoundedRectangle(bounds, cornerRadius);
            return;
        }

        // ── Mood pill rendering ──────────────────────────────────────────────
        if (isMoodPill(btn))
        {
            const bool toggled = btn.getToggleState();
            constexpr float pillRadius = 10.0f; // half of 20px pill height

            // Background
            juce::Colour bg;
            if (toggled)
                bg = juce::Colour(0xFFE9C46A).withAlpha(0.14f); // gold-dim active
            else if (isOver)
                bg = juce::Colour(0xFFFFFFFF).withAlpha(0.03f); // hover
            else
                bg = juce::Colours::transparentBlack; // default

            g.setColour(bg);
            g.fillRoundedRectangle(bounds, pillRadius);

            // Border
            juce::Colour borderCol;
            if (btn.hasKeyboardFocus(true))
                borderCol = A11y::focusRingColour().withAlpha(0.8f);
            else if (toggled)
                borderCol = juce::Colour(0xFFE9C46A).withAlpha(0.35f); // gold active border
            else
                borderCol = GalleryColors::border(); // default subtle border

            float borderThickness = btn.hasKeyboardFocus(true) ? 1.5f : 1.0f;
            g.setColour(borderCol);
            g.drawRoundedRectangle(bounds.reduced(0.5f), pillRadius, borderThickness);

            return;
        }

        // ── Standard button rendering ────────────────────────────────────────
        // Dispatch by integer property tag — set at button setup via
        // setButtonStyle(). Avoids String::containsIgnoreCase() allocation
        // on every repaint of every button (#1161).
        const int style = getButtonStyle(btn);

       #if JUCE_DEBUG
        // Catch accidental regression: buttons whose name matches export/panic
        // but which were never tagged with setButtonStyle().
        if (style == kBtnStyleDefault)
        {
            const juce::String n = btn.getName();
            jassert(! n.containsIgnoreCase("export"));
            jassert(! n.containsIgnoreCase("panic"));
        }
       #endif

        juce::Colour bg, borderCol;

        if (style == kBtnStyleExport)
        {
            bg = isDown ? get(xoGold).darker(0.15f) : (isOver ? get(xoGold).brighter(0.05f) : get(xoGold));
            borderCol = get(xoGold).darker(0.2f);
        }
        else if (style == kBtnStylePanic)
        {
            bg = isDown ? juce::Colour(0xFFFF6B6B) : juce::Colour(elevated());
            borderCol = juce::Colour(0xFFFF6B6B).withAlpha(isDown ? 0.60f : 0.25f);
        }
        else
        {
            bg = isDown ? juce::Colour(raised())
                        : (isOver ? juce::Colour(elevated()).brighter(0.03f) : juce::Colour(elevated()));
            borderCol = isOver ? borderMd() : border();
        }

        g.setColour(bg);
        g.fillRoundedRectangle(bounds, 5.0f);

        // Focus ring (WCAG 2.4.7) — overrides border when focused
        if (btn.hasKeyboardFocus(true))
        {
            g.setColour(A11y::focusRingColour().withAlpha(0.8f));
            g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.5f);
        }
        else
        {
            g.setColour(borderCol);
            g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.0f);
        }
    }

    //==========================================================================
    // drawButtonText — pill buttons use Inter 9px; standard buttons use default
    void drawButtonText(juce::Graphics& g, juce::TextButton& btn, bool /*isOver*/, bool /*isDown*/) override
    {
        if (isMoodPill(btn))
        {
            const bool toggled = btn.getToggleState();
            auto textColour =
                btn.findColour(toggled ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId);
            g.setColour(textColour);
            g.setFont(GalleryFonts::body(10.0f)); // (#885: 9pt→10pt legibility floor)
            g.drawFittedText(btn.getButtonText(), btn.getLocalBounds(), juce::Justification::centred, 1);
            return;
        }

        // Default JUCE text rendering for all other buttons
        juce::LookAndFeel_V4::drawButtonText(g, btn, false, false);
    }

    //==========================================================================
    // drawComboBox — elevated card with T3 arrow, prototype-spec colors
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int /*buttonX*/, int /*buttonY*/,
                      int /*buttonW*/, int /*buttonH*/, juce::ComboBox& box) override
    {
        using namespace GalleryColors;

        if (!box.isEnabled())
            g.setOpacity(0.35f);

        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

        // Fill
        g.setColour(juce::Colour(elevated()));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Border — A11y focus ring when focused, standard border otherwise
        if (box.hasKeyboardFocus(true))
        {
            g.setColour(A11y::focusRingColour());
            g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 2.0f);
        }
        else
        {
            g.setColour(border());
            g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
        }

        // Dropdown arrow
        auto arrowZone = bounds.removeFromRight((float)height * 0.7f).reduced((float)height * 0.25f);
        juce::Path arrow;
        arrow.addTriangle(arrowZone.getX(), arrowZone.getY(), arrowZone.getRight(), arrowZone.getY(),
                          arrowZone.getCentreX(), arrowZone.getBottom());
        g.setColour(juce::Colour(t2()).withAlpha(isButtonDown ? 0.9f : 0.55f));
        g.fillPath(arrow);
    }

    //==========================================================================
    // drawLinearSlider — track T4, fill accent, thumb with specular
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                          float /*minSliderPos*/, float /*maxSliderPos*/, juce::Slider::SliderStyle style,
                          juce::Slider& slider) override
    {
        using namespace GalleryColors;

        if (!slider.isEnabled())
            g.setOpacity(0.35f);

        bool isHorizontal = (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar);
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();

        // Track background — T4 at 35% opacity
        auto trackBounds = isHorizontal ? bounds.withSizeKeepingCentre(bounds.getWidth(), 4.0f)
                                        : bounds.withSizeKeepingCentre(4.0f, bounds.getHeight());

        g.setColour(juce::Colour(t4()).withAlpha(0.35f));
        g.fillRoundedRectangle(trackBounds, 2.0f);

        // Filled portion — accent color
        auto fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
        if (fillColour.isTransparent())
            fillColour = get(xoGold);

        auto filledBounds = trackBounds;
        if (isHorizontal)
            filledBounds.setWidth(
                juce::jmax(0.0f, sliderPos - (float)x)); // CQ04: clamp to zero — sliderPos can be left of x at minimum
        else
            filledBounds = filledBounds.withTop(sliderPos);

        g.setColour(fillColour);
        g.fillRoundedRectangle(filledBounds, 2.0f);

        // Thumb
        constexpr float thumbSize = 12.0f;
        juce::Point<float> thumbCenter;
        if (isHorizontal)
            thumbCenter = {sliderPos, bounds.getCentreY()};
        else
            thumbCenter = {bounds.getCentreX(), sliderPos};

        // Drop shadow
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillEllipse(thumbCenter.x - thumbSize * 0.5f + 1.0f, thumbCenter.y - thumbSize * 0.5f + 1.0f, thumbSize,
                      thumbSize);

        // Body
        g.setColour(fillColour.brighter(0.1f));
        g.fillEllipse(thumbCenter.x - thumbSize * 0.5f, thumbCenter.y - thumbSize * 0.5f, thumbSize, thumbSize);

        // Border
        g.setColour(GalleryColors::border());
        g.drawEllipse(thumbCenter.x - thumbSize * 0.5f, thumbCenter.y - thumbSize * 0.5f, thumbSize, thumbSize, 1.0f);

        // Specular highlight
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.fillEllipse(thumbCenter.x - thumbSize * 0.25f, thumbCenter.y - thumbSize * 0.25f, thumbSize * 0.5f,
                      thumbSize * 0.5f);
    }

    //==========================================================================
    // drawTooltip — Satoshi body 10pt prose (was JetBrains Mono — #1169),
    // raised background, 2-layer shadow
    void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override
    {
        using namespace GalleryColors;
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

        // Shadow (2 layers)
        g.setColour(juce::Colour(0x66000000));
        g.fillRoundedRectangle(bounds.translated(0, 2).expanded(2), 5.0f);

        // Background
        g.setColour(juce::Colour(raised()));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Border
        g.setColour(borderMd());
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Text — Satoshi body 10pt. Tooltips are prose ("desert spring electric
        // piano"), not values — reserve the mono GalleryFonts::value() for
        // numeric readouts (BPM, Hz, dB). 10pt holds the #885 legibility floor.
        g.setColour(juce::Colour(t1()));
        g.setFont(GalleryFonts::body(10.0f));
        {
            auto tooltipBounds = bounds.reduced(8, 4);
            auto displayText = GalleryUtils::ellipsizeText(g.getCurrentFont(), text, (float)tooltipBounds.getWidth());
            g.drawText(displayText, tooltipBounds, juce::Justification::centredLeft, false);
        }
    }


    //==========================================================================
    // getDefaultScrollbarWidth — 4px slim scrollbar matching prototype
    int getDefaultScrollbarWidth() override { return 4; }

    //==========================================================================
    // drawPopupMenuBackground — elevated card, T4 border
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        using namespace GalleryColors;
        g.setColour(juce::Colour(elevated()));
        g.fillRoundedRectangle(juce::Rectangle<int>(0, 0, width, height).toFloat(), 4.0f);
        g.setColour(border());
        g.drawRoundedRectangle(juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f), 4.0f, 1.0f);
    }

    //==========================================================================
    // drawPopupMenuItem — T1 text, gold highlight, T3 separator
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                           bool isHighlighted, bool isTicked, bool /*hasSubMenu*/, const juce::String& text,
                           const juce::String& shortcutKeyText, const juce::Drawable* /*icon*/,
                           const juce::Colour* textColourToUse) override
    {
        using namespace GalleryColors;

        if (isSeparator)
        {
            auto sepArea = area.reduced(8, 0);
            g.setColour(border());
            g.fillRect(sepArea.getX(), sepArea.getCentreY(), sepArea.getWidth(), 1);
            return;
        }

        if (isHighlighted && isActive)
        {
            g.setColour(get(xoGold).withAlpha(0.15f));
            g.fillRoundedRectangle(area.toFloat().reduced(2.0f, 1.0f), 3.0f);
        }

        auto textArea = area.reduced(12, 0);
        auto textColour = isActive ? juce::Colour(t1()) : juce::Colour(t2()).withAlpha(0.5f);
        if (textColourToUse)
            textColour = *textColourToUse;

        g.setColour(textColour);
        g.setFont(GalleryFonts::body(13.0f));
        g.drawFittedText(text, textArea, juce::Justification::centredLeft, 1);

        if (isTicked)
        {
            g.setColour(get(xoGold));
            auto tickBounds = area.toFloat().removeFromLeft(24.0f).reduced(6.0f);
            g.fillEllipse(tickBounds.getCentreX() - 3.0f, tickBounds.getCentreY() - 3.0f, 6.0f, 6.0f);
        }

        if (shortcutKeyText.isNotEmpty())
        {
            g.setColour(juce::Colour(t2()).withAlpha(0.45f));
            g.setFont(GalleryFonts::value(10.0f));
            g.drawFittedText(shortcutKeyText, textArea, juce::Justification::centredRight, 1);
        }
    }
};

} // namespace xoceanus
