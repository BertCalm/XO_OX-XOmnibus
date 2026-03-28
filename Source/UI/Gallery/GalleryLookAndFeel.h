#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"

namespace xolokun {

//==============================================================================
// GalleryLookAndFeel — Gallery Model visual language (WCAG 2.1 AA compliant)
// Matched to v05 dark-mode-primary spec.
class GalleryLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GalleryLookAndFeel() { applyTheme(); }

    // Re-apply all theme colors — call after toggling GalleryColors::darkMode().
    void applyTheme()
    {
        using namespace GalleryColors;

        // Rotary sliders
        setColour(juce::Slider::rotarySliderFillColourId,    get(xoGold));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(t3()));
        setColour(juce::Slider::textBoxTextColourId,         juce::Colour(t2()));
        setColour(juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);

        // Buttons
        setColour(juce::TextButton::buttonColourId,          juce::Colour(elevated()));
        setColour(juce::TextButton::buttonOnColourId,        get(xoGold));
        setColour(juce::TextButton::textColourOffId,         juce::Colour(t1()));
        setColour(juce::TextButton::textColourOnId,          juce::Colour(darkMode() ? Dark::bg : Light::textDark));

        // ComboBox
        setColour(juce::ComboBox::backgroundColourId,        juce::Colour(elevated()));
        setColour(juce::ComboBox::outlineColourId,           border());
        setColour(juce::ComboBox::textColourId,              juce::Colour(t1()));
        setColour(juce::ComboBox::arrowColourId,             juce::Colour(t2()));

        // PopupMenu
        setColour(juce::PopupMenu::backgroundColourId,       juce::Colour(elevated()));
        setColour(juce::PopupMenu::textColourId,             juce::Colour(t1()));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, get(xoGold).withAlpha(0.15f));
        setColour(juce::PopupMenu::highlightedTextColourId,  juce::Colour(t1()));

        // ScrollBar
        setColour(juce::ScrollBar::thumbColourId,            juce::Colour(t4()));
        setColour(juce::ScrollBar::backgroundColourId,       juce::Colour(elevated()));

        // Labels
        setColour(juce::Label::textColourId,                 juce::Colour(t1()));
        setColour(juce::Label::backgroundColourId,           juce::Colours::transparentBlack);
        setColour(juce::Label::outlineColourId,              juce::Colours::transparentBlack);

        // ListBox / table
        setColour(juce::ListBox::backgroundColourId,         juce::Colour(elevated()));
        setColour(juce::ListBox::textColourId,               juce::Colour(t1()));
        setColour(juce::ListBox::outlineColourId,            border());
    }

    //==========================================================================
    // drawRotarySlider — matches prototype knob exactly
    // Body: radial gradient (circle at 38% 32%, #4A4A4E 0%, #3A3A3E 25%, #282830 55%, #141418 100%)
    // Arc track: 1.4px, T3 (#5E5C5A)
    // Arc fill: 1.8px, engine accent
    // Indicator dot: r=1.8 at arc endpoint
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        using namespace GalleryColors;

        auto bounds  = juce::Rectangle<int>(x, y, width, height).toFloat();
        float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
        float radius = diameter * 0.5f;
        float cx     = bounds.getCentreX();
        float cy     = bounds.getCentreY();

        // ── DPI scale factor — used for stroke widths throughout ───────────
        const float dpiScale = juce::jmax(1.0f,
            (float) juce::Desktop::getInstance().getDisplays()
                .getDisplayForPoint(slider.getScreenBounds().getCentre())
                ->scale);

        // ── 1. Knob body — radial gradient matching prototype ──────────────
        {
            juce::ColourGradient grad;
            grad = juce::ColourGradient(
                juce::Colour(0xFF4A4A4E),
                cx - radius * 0.24f,
                cy - radius * 0.36f,
                juce::Colour(0xFF141418),
                cx + radius, cy + radius,
                true /* radial */);
            grad.addColour(0.25, juce::Colour(0xFF3A3A3E));
            grad.addColour(0.55, juce::Colour(0xFF282830));
            g.setGradientFill(grad);
            g.fillEllipse(cx - radius, cy - radius, diameter, diameter);
        }

        // ── 2. Outer border ring — prototype: 1px rgba(255,255,255,0.07) ───
        g.setColour(GalleryColors::border());
        g.drawEllipse(cx - radius, cy - radius, diameter, diameter, 1.0f);

        // ── 3. Inner highlight — top-left specular ──────────────────────────
        g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.09f));
        g.drawEllipse(cx - radius + 1.0f, cy - radius + 1.0f,
                      diameter - 2.0f, diameter - 2.0f, 0.5f);

        // ── 4. Arc track — 1.4px, T3 (#5E5C5A) ────────────────────────────
        float arcRadius = radius - 3.0f;
        {
            juce::Path trackArc;
            trackArc.addCentredArc(cx, cy, arcRadius, arcRadius, 0.0f,
                                   rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(juce::Colour(GalleryColors::t3()));
            g.strokePath(trackArc, juce::PathStrokeType(2.2f / dpiScale, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
        }

        // ── 5. Arc fill — 1.8px, engine accent ────────────────────────────
        float fillEnd = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        if (sliderPos > 0.001f)
        {
            juce::Path fillArc;
            fillArc.addCentredArc(cx, cy, arcRadius, arcRadius, 0.0f,
                                  rotaryStartAngle, fillEnd, true);

            auto fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
            if (fillColour.isTransparent()) fillColour = get(xoGold);

            g.setColour(fillColour);
            g.strokePath(fillArc, juce::PathStrokeType(2.8f / dpiScale, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));

            // ── 6. Indicator dot at arc endpoint ────────────────────────────
            // Screen coords: x = cx + r*cos(angle - pi/2), y = cy + r*sin(angle - pi/2)
            float dotX = cx + arcRadius * std::cos(fillEnd - juce::MathConstants<float>::halfPi);
            float dotY = cy + arcRadius * std::sin(fillEnd - juce::MathConstants<float>::halfPi);
            float dotR = 1.8f / dpiScale;
            g.setColour(fillColour.withAlpha(0.9f));
            g.fillEllipse(dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
        }

        // ── 7. Focus ring (WCAG 2.4.7) ────────────────────────────────────
        if (slider.hasKeyboardFocus(true))
        {
            g.setColour(A11y::focusRingColour().withAlpha(1.0f));
            g.drawEllipse(cx - radius - 2.0f, cy - radius - 2.0f,
                          diameter + 4.0f, diameter + 4.0f, 2.0f / dpiScale);
        }

        // ── 8. Live value readout (during interaction) ─────────────────────
        // 32px knobs (diameter < 40) suppress in-knob readout — disc would be
        // only 14px wide with ~9.8px text, which clips. Tooltip handles it instead.
        if (diameter >= 28.0f && (slider.isMouseButtonDown() || slider.isMouseOverOrDragging()))
        {
            float discR = radius * 0.44f;
            juce::String valStr;
            double val = slider.getValue();
            if (slider.getInterval() >= 1.0 && slider.getMaximum() <= 127.0)
                valStr = juce::String((int)val);
            else
                valStr = juce::String(val, 1);

            float fontSize = juce::jmax(7.0f, discR * 0.7f);
            g.setFont(GalleryFonts::value(fontSize));
            g.setColour(juce::Colour(GalleryColors::t1()).withAlpha(0.85f));
            g.drawText(valStr,
                       juce::Rectangle<float>(cx - discR, cy - discR, discR * 2.0f, discR * 2.0f),
                       juce::Justification::centred);
        }
    }

    //==========================================================================
    // Mood pill style helpers
    // Call this on any TextButton to opt it in to pill rendering.
    static void setMoodPillStyle(juce::TextButton& btn)
    {
        btn.getProperties().set("moodPill", true);
    }

    static bool isMoodPill(const juce::Button& btn)
    {
        return btn.getProperties()["moodPill"];
    }

    //==========================================================================
    // drawButtonBackground — matches prototype button styles
    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour& /*bgColour*/, bool isOver, bool isDown) override
    {
        using namespace GalleryColors;

        auto bounds = btn.getLocalBounds().toFloat();

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
                bg = juce::Colours::transparentBlack;            // default

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
        const juce::String name = btn.getName();

        bool isExport = name.containsIgnoreCase("export");
        bool isPanic  = name.containsIgnoreCase("panic");

        juce::Colour bg, borderCol;

        if (isExport)
        {
            bg        = isDown ? get(xoGold).darker(0.15f)
                               : (isOver ? get(xoGold).brighter(0.05f) : get(xoGold));
            borderCol = get(xoGold).darker(0.2f);
        }
        else if (isPanic)
        {
            bg        = isDown ? juce::Colour(0xFFFF6B6B)
                               : juce::Colour(elevated());
            borderCol = juce::Colour(0xFFFF6B6B).withAlpha(isDown ? 0.60f : 0.25f);
        }
        else
        {
            bg        = isDown ? juce::Colour(raised())
                               : (isOver ? juce::Colour(elevated()).brighter(0.03f)
                                         : juce::Colour(elevated()));
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
    void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
                        bool /*isOver*/, bool /*isDown*/) override
    {
        if (isMoodPill(btn))
        {
            const bool toggled = btn.getToggleState();
            auto textColour = btn.findColour(toggled ? juce::TextButton::textColourOnId
                                                     : juce::TextButton::textColourOffId);
            g.setColour(textColour);
            g.setFont(GalleryFonts::body(9.0f));
            g.drawFittedText(btn.getButtonText(),
                             btn.getLocalBounds(),
                             juce::Justification::centred,
                             1);
            return;
        }

        // Default JUCE text rendering for all other buttons
        juce::LookAndFeel_V4::drawButtonText(g, btn, false, false);
    }

    //==========================================================================
    // drawComboBox — elevated card with T3 arrow, prototype-spec colors
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                      juce::ComboBox& box) override
    {
        using namespace GalleryColors;
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
        arrow.addTriangle(arrowZone.getX(),         arrowZone.getY(),
                          arrowZone.getRight(),      arrowZone.getY(),
                          arrowZone.getCentreX(),    arrowZone.getBottom());
        g.setColour(juce::Colour(t2()).withAlpha(isButtonDown ? 0.9f : 0.55f));
        g.fillPath(arrow);
    }

    //==========================================================================
    // drawLinearSlider — track T4, fill accent, thumb with specular
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        using namespace GalleryColors;

        bool isHorizontal = (style == juce::Slider::LinearHorizontal ||
                             style == juce::Slider::LinearBar);
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();

        // Track background — T4 at 35% opacity
        auto trackBounds = isHorizontal
            ? bounds.withSizeKeepingCentre(bounds.getWidth(), 4.0f)
            : bounds.withSizeKeepingCentre(4.0f, bounds.getHeight());

        g.setColour(juce::Colour(t4()).withAlpha(0.35f));
        g.fillRoundedRectangle(trackBounds, 2.0f);

        // Filled portion — accent color
        auto fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
        if (fillColour.isTransparent()) fillColour = get(xoGold);

        auto filledBounds = trackBounds;
        if (isHorizontal)
            filledBounds.setWidth(juce::jmax(0.0f, sliderPos - (float)x)); // CQ04: clamp to zero — sliderPos can be left of x at minimum
        else
            filledBounds = filledBounds.withTop(sliderPos);

        g.setColour(fillColour);
        g.fillRoundedRectangle(filledBounds, 2.0f);

        // Thumb
        constexpr float thumbSize = 12.0f;
        juce::Point<float> thumbCenter;
        if (isHorizontal)
            thumbCenter = { sliderPos, bounds.getCentreY() };
        else
            thumbCenter = { bounds.getCentreX(), sliderPos };

        // Drop shadow
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillEllipse(thumbCenter.x - thumbSize * 0.5f + 1.0f,
                      thumbCenter.y - thumbSize * 0.5f + 1.0f,
                      thumbSize, thumbSize);

        // Body
        g.setColour(fillColour.brighter(0.1f));
        g.fillEllipse(thumbCenter.x - thumbSize * 0.5f,
                      thumbCenter.y - thumbSize * 0.5f,
                      thumbSize, thumbSize);

        // Border
        g.setColour(GalleryColors::border());
        g.drawEllipse(thumbCenter.x - thumbSize * 0.5f,
                      thumbCenter.y - thumbSize * 0.5f,
                      thumbSize, thumbSize, 1.0f);

        // Specular highlight
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.fillEllipse(thumbCenter.x - thumbSize * 0.25f,
                      thumbCenter.y - thumbSize * 0.25f,
                      thumbSize * 0.5f, thumbSize * 0.5f);
    }

    //==========================================================================
    // drawTooltip — JetBrains Mono 9px, raised background, 2-layer shadow
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

        // Text — JetBrains Mono 9px
        g.setColour(juce::Colour(t1()));
        g.setFont(GalleryFonts::value(9.0f));
        g.drawText(text, bounds.reduced(8, 4), juce::Justification::centredLeft, true);
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
            g.setColour(border());
            g.fillRect(sepArea.getX(), sepArea.getCentreY(), sepArea.getWidth(), 1);
            return;
        }

        if (isHighlighted && isActive)
        {
            g.setColour(get(xoGold).withAlpha(0.15f));
            g.fillRoundedRectangle(area.toFloat().reduced(2.0f, 1.0f), 3.0f);
        }

        auto textArea    = area.reduced(12, 0);
        auto textColour  = isActive ? juce::Colour(t1()) : juce::Colour(t2()).withAlpha(0.5f);
        if (textColourToUse) textColour = *textColourToUse;

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

} // namespace xolokun
