#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"

// SpecializedWidgets.h — V1-required shared UI components for XOlokun.
//
// Components:
//   NamedModeSelector — horizontal pill row for choice parameters with meaningful
//                       names (biome, ERA vertex, maqam, knot type, etc.).
//                       Richer than a ComboBox; each pill is visually distinct.
//
//   BipolarAxisBar    — horizontal fill bar with center = 0, left = negative,
//                       right = positive. Labeled endpoints, gradient fill,
//                       live value indicator. For bipolar interaction axes
//                       (STRIFE/LOVE, Analog/Digital, Soft/Hard, etc.).
//
// Both components:
//   • Use GalleryColors / GalleryFonts / A11y from GalleryColors.h
//   • Live in namespace xolokun
//   • Use APVTS attachments (ComboBoxAttachment / SliderAttachment)
//   • Work at any width — fully responsive layout
//   • Header-only (.h)

namespace xolokun {

//==============================================================================
//  NamedModeSelector
//
//  A row of labeled pills (rounded rect "tab buttons") bound to an APVTS
//  choice parameter. Each pill maps to one choice index. The active pill is
//  filled with the mode's accent color; inactive pills show the color as
//  text at 60 % opacity.
//
//  Typical height: 32 pt (but adapts to whatever bounds are given).
//
//  Usage example (OVERWORLD ERA triangle):
//
//      juce::StringArray eraNames { "Analog", "Digital", "Hybrid" };
//      std::vector<juce::Colour> eraColors {
//          juce::Colour(0xFFE9A84A),   // Analog — amber
//          juce::Colour(0xFF39FF14),   // Digital — neon green
//          juce::Colour(0xFF00A6D6),   // Hybrid  — cyan
//      };
//      eraSelector = std::make_unique<NamedModeSelector>(
//          apvts, "ow_era", eraNames, &eraColors);
//      addAndMakeVisible(*eraSelector);
//
//  Usage example (no per-mode colors, uses engine accent for all):
//
//      juce::StringArray knotNames { "Trefoil", "Figure-Eight", "Torus", "Solomon" };
//      knotSelector = std::make_unique<NamedModeSelector>(
//          apvts, "weave_knotType", knotNames,
//          nullptr,                              // use defaultAccent for all
//          juce::Colour(0xFF8E4585));            // ORBWEAVE Kelp Knot Purple
//==============================================================================
class NamedModeSelector : public juce::Component
{
public:
    // paramId       — APVTS choice parameter ID to bind.
    // modeNames     — display label for each choice index (must match param choices count).
    // modeColors    — optional per-mode accent color vector; nullptr uses defaultAccent for all.
    // defaultAccent — engine accent colour used when modeColors is nullptr, or as fallback
    //                 when the modeColors vector is shorter than modeNames.
    NamedModeSelector(juce::AudioProcessorValueTreeState& apvts,
                      const juce::String&                  paramId,
                      const juce::StringArray&              modeNames,
                      const std::vector<juce::Colour>*      modeColors  = nullptr,
                      juce::Colour                          defaultAccent = juce::Colour(GalleryColors::xoGold))
        : pid(paramId),
          names(modeNames),
          accent(defaultAccent)
    {
        // Copy per-mode colors (may be nullptr → empty)
        if (modeColors != nullptr)
            colors = *modeColors;

        // Hidden combo box is the APVTS attachment vehicle.
        // We drive it from mouseDown / keyboard, it drives the parameter.
        // Populate items BEFORE creating the attachment — the ComboBoxAttachment
        // reads the current APVTS value to set the initial selection on construction,
        // which requires items to already be present (mirrors CouplingPopover pattern).
        combo.clear(juce::dontSendNotification);
        for (int i = 0; i < names.size(); ++i)
            combo.addItem(names[i], i + 1);

        combo.setVisible(false);
        addAndMakeVisible(combo);

        // APVTS attachment — full undo / redo + host automation support.
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, paramId, combo);

        // Observe combo changes (driven by host / undo) so we can repaint.
        combo.onChange = [this] { repaint(); };

        // Accessibility
        A11y::setup(*this,
                    juce::String("Mode selector: ") + paramId,
                    juce::String("Use Left/Right arrow keys or click to select a mode."));

        // Optional small icon per mode — reserved for future use.
        // Set via setModeIcon(int index, const juce::Image&) before first paint.
    }

    // Optional: override the accent colour for a specific mode after construction.
    void setModeColor(int index, juce::Colour c)
    {
        if (index >= 0 && index < (int)colors.size())
            colors[(size_t)index] = c;
        else
        {
            colors.resize((size_t)juce::jmax(index + 1, (int)colors.size()),
                          accent);
            colors[(size_t)index] = c;
        }
        repaint();
    }

    // Optional: set a 12×12 icon image shown to the left of the pill label.
    void setModeIcon(int index, const juce::Image& img)
    {
        if (index < 0) return;
        if (index >= (int)icons.size())
            icons.resize((size_t)(index + 1));
        icons[(size_t)index] = img;
        repaint();
    }

    //--------------------------------------------------------------------------
    void paint(juce::Graphics& g) override
    {
        if (names.isEmpty()) return;

        int selected = getSelectedIndex();
        auto b = getLocalBounds().toFloat();

        // Draw background strip — subtle shell white card
        g.setColour(GalleryColors::get(GalleryColors::shellWhite()).withAlpha(0.0f));
        g.fillRoundedRectangle(b, 12.0f);

        int n = names.size();
        float pillW  = b.getWidth() / static_cast<float>(n);
        float pillH  = b.getHeight();

        for (int i = 0; i < n; ++i)
        {
            juce::Rectangle<float> pill(b.getX() + i * pillW, b.getY(), pillW, pillH);
            pill = pill.reduced(2.0f, 1.0f);

            juce::Colour modeCol = colorForIndex(i);
            bool isActive = (i == selected);

            if (isActive)
            {
                // Filled pill — active mode
                g.setColour(modeCol);
                g.fillRoundedRectangle(pill, 12.0f);
            }
            else
            {
                // Transparent pill with colored border at low opacity
                g.setColour(modeCol.withAlpha(0.18f));
                g.fillRoundedRectangle(pill, 12.0f);
                g.setColour(modeCol.withAlpha(0.35f));
                g.drawRoundedRectangle(pill.reduced(0.5f), 12.0f, 1.0f);
            }

            // Optional icon (left of text, 12×12, centered vertically)
            float textX = pill.getX() + 6.0f;
            float textW = pill.getWidth() - 12.0f;

            if (i < (int)icons.size() && icons[(size_t)i].isValid())
            {
                float iconSz = 12.0f;
                float iconY  = pill.getCentreY() - iconSz * 0.5f;
                g.drawImage(icons[(size_t)i],
                            juce::Rectangle<float>(textX, iconY, iconSz, iconSz),
                            juce::RectanglePlacement::centred);
                textX += iconSz + 4.0f;
                textW -= iconSz + 4.0f;
            }

            // Pill label
            juce::Colour textCol = isActive
                ? juce::Colours::white
                : modeCol.withAlpha(0.60f);

            g.setColour(textCol);
            g.setFont(GalleryFonts::label(juce::jmax(8.0f, pillH * 0.36f)));
            g.drawText(names[i],
                       juce::Rectangle<float>(textX, pill.getY(), textW, pill.getHeight()),
                       juce::Justification::centred, true);
        }

        // Focus ring (WCAG 2.4.7)
        if (hasKeyboardFocus(true))
            A11y::drawFocusRing(g, b, 12.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown()) return;

        int n = names.size();
        if (n == 0) return;

        float pillW = (float)getWidth() / (float)n;
        int clicked = (int)(e.position.x / pillW);
        clicked = juce::jlimit(0, n - 1, clicked);

        selectIndex(clicked);
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        int n = names.size();
        if (n == 0) return false;

        int current = getSelectedIndex();
        if (key == juce::KeyPress::leftKey)
        {
            selectIndex(juce::jmax(0, current - 1));
            return true;
        }
        if (key == juce::KeyPress::rightKey)
        {
            selectIndex(juce::jmin(n - 1, current + 1));
            return true;
        }
        return false;
    }

    void resized() override
    {
        // Hidden combo lives outside the visible area; just park it zero-size.
        combo.setBounds(0, 0, 0, 0);
    }

    // Returns the currently selected mode index (0-based), or -1 if none.
    int getSelectedIndex() const
    {
        return combo.getSelectedItemIndex(); // 0-based
    }

private:
    void selectIndex(int index)
    {
        // setSelectedItemIndex triggers the ComboBoxAttachment → APVTS → parameter update.
        combo.setSelectedItemIndex(index, juce::sendNotificationSync);
        repaint();
    }

    juce::Colour colorForIndex(int i) const
    {
        if (i >= 0 && i < (int)colors.size())
            return colors[(size_t)i];
        return accent;
    }

    //──────────────────────────────────────────────────────────────────────
    juce::String                        pid;
    juce::StringArray                   names;
    std::vector<juce::Colour>           colors;
    std::vector<juce::Image>            icons;
    juce::Colour                        accent;

    // Attachment vehicle: hidden combo box owned by this component.
    // Declaration order: combo before attachment (attachment binds to combo).
    juce::ComboBox                                                          combo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamedModeSelector)
};


//==============================================================================
//  BipolarAxisBar
//
//  A horizontal fill bar where center = 0, left = −1, right = +1 (or the
//  normalized equivalents of the parameter range). Gradient fill emanates
//  from center toward the current value. Endpoint labels + a live value
//  indicator (4 pt wide bright bar) at the value position.
//
//  Typical height: 28 pt (but adapts to whatever bounds are given).
//
//  The component contains a hidden juce::Slider (Linear Horizontal) that
//  carries the SliderAttachment. Mouse interaction drives the hidden slider
//  directly via setValue(); double-click and Cmd+click both reset to center.
//
//  Usage example (OUIE HAMMER axis):
//
//      hammerAxis = std::make_unique<BipolarAxisBar>(
//          apvts, "ouie_macroHammer",
//          "STRIFE", "LOVE",
//          juce::Colour(0xFFFF4444),   // left — red aggression
//          juce::Colour(0xFF44BBFF));  // right — blue harmony
//      addAndMakeVisible(*hammerAxis);
//
//  Usage example (OBESE MOJO axis):
//
//      mojoAxis = std::make_unique<BipolarAxisBar>(
//          apvts, "fat_mojoAxis",
//          "ANALOG", "DIGITAL",
//          juce::Colour(0xFFFF8800),
//          juce::Colour(0xFF00CCFF));
//      addAndMakeVisible(*mojoAxis);
//==============================================================================
class BipolarAxisBar : public juce::Component
{
public:
    // paramId    — APVTS float parameter (should be bipolar, e.g. −1..+1 or 0..1 centered).
    // leftLabel  — text shown at the left edge (negative pole name).
    // rightLabel — text shown at the right edge (positive pole name).
    // leftColor  — gradient/label colour for the negative side.
    // rightColor — gradient/label colour for the positive side.
    BipolarAxisBar(juce::AudioProcessorValueTreeState& apvts,
                   const juce::String&                  paramId,
                   const juce::String&                  leftLabel,
                   const juce::String&                  rightLabel,
                   juce::Colour                         leftColor,
                   juce::Colour                         rightColor)
        : pid(paramId),
          lblLeft(leftLabel),
          lblRight(rightLabel),
          colLeft(leftColor),
          colRight(rightColor)
    {
        // Determine the parameter's natural range so we can find the center.
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(paramId)))
        {
            auto range = rp->getNormalisableRange();
            paramMin   = range.start;
            paramMax   = range.end;
            // Center: midpoint of the range (handles 0..1 and −1..+1 equally)
            paramCenter = (paramMin + paramMax) * 0.5;
            defaultVal  = static_cast<double>(range.convertFrom0to1(rp->getDefaultValue()));
        }

        // Hidden slider — carries the SliderAttachment, never painted.
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setVisible(false);
        slider.setRange(paramMin, paramMax);
        addAndMakeVisible(slider);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, paramId, slider);

        // Repaint when the slider value changes (driven by host / undo).
        slider.onValueChange = [this] { repaint(); };

        // Accessibility
        A11y::setup(*this,
                    juce::String("Bipolar axis: ") + leftLabel + " / " + rightLabel,
                    juce::String("Drag left for ") + leftLabel +
                    juce::String(", drag right for ") + rightLabel +
                    juce::String(". Double-click or Cmd+click to reset to center."));
    }

    //--------------------------------------------------------------------------
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        auto b         = getLocalBounds().toFloat();
        float barH     = b.getHeight();
        float labelW   = juce::jmax(36.0f, barH * 2.0f);  // adaptive label width
        float barX     = b.getX() + labelW;
        float barRight = b.getRight() - labelW;
        float barWidth = barRight - barX;
        if (barWidth < 4.0f) barWidth = 4.0f;

        // ── 1. Left label ─────────────────────────────────────────────────
        g.setFont(GalleryFonts::label(juce::jmax(7.0f, barH * 0.32f)));
        g.setColour(colLeft);
        g.drawText(lblLeft,
                   juce::Rectangle<float>(b.getX(), b.getY(), labelW - 4.0f, barH),
                   juce::Justification::centredRight, true);

        // ── 2. Right label ────────────────────────────────────────────────
        g.setColour(colRight);
        g.drawText(lblRight,
                   juce::Rectangle<float>(barRight + 4.0f, b.getY(), labelW - 4.0f, barH),
                   juce::Justification::centredLeft, true);

        // ── 3. Bar track ─────────────────────────────────────────────────
        float trackPad  = barH * 0.25f;
        float trackY    = b.getY() + trackPad;
        float trackH    = barH - trackPad * 2.0f;
        float trackR    = trackH * 0.5f;

        juce::Rectangle<float> track(barX, trackY, barWidth, trackH);

        // Track background
        g.setColour(get(borderGray()).withAlpha(0.40f));
        g.fillRoundedRectangle(track, trackR);

        // ── 4. Center detent line ─────────────────────────────────────────
        float centerX = barX + barWidth * 0.5f;
        g.setColour(get(textMid()).withAlpha(0.50f));
        g.drawVerticalLine(juce::roundToInt(centerX), trackY, trackY + trackH);

        // ── 5. Gradient fill from center toward current value ─────────────
        double val       = slider.getValue();
        double normVal   = (paramMax > paramMin)
                               ? (val - paramMin) / (paramMax - paramMin)
                               : 0.5;
        double normCenter = (paramMax > paramMin)
                                ? (paramCenter - paramMin) / (paramMax - paramMin)
                                : 0.5;

        float fillStart  = (float)normCenter * barWidth;
        float fillEnd    = (float)normVal    * barWidth;

        if (std::abs(fillEnd - fillStart) > 0.5f)
        {
            float fx = barX + juce::jmin(fillStart, fillEnd);
            float fw = std::abs(fillEnd - fillStart);

            bool isPositive = (val >= paramCenter);
            juce::Colour fillNear = isPositive ? colRight.withAlpha(0.60f)
                                               : colLeft.withAlpha(0.60f);
            juce::Colour fillFar  = isPositive ? colRight.withAlpha(0.90f)
                                               : colLeft.withAlpha(0.90f);

            // Gradient: more opaque toward the value tip, more transparent at center.
            juce::ColourGradient grad;
            if (isPositive)
                grad = juce::ColourGradient(fillNear, fx, 0.0f,
                                            fillFar,  fx + fw, 0.0f, false);
            else
                grad = juce::ColourGradient(fillFar,  fx, 0.0f,
                                            fillNear, fx + fw, 0.0f, false);

            g.setGradientFill(grad);
            g.fillRoundedRectangle(fx, trackY, fw, trackH, trackR);
        }

        // ── 6. Value indicator — 4 pt bright vertical bar at value position ─
        float indicatorX = barX + (float)normVal * barWidth;
        float indHalfW   = 2.0f;
        juce::Colour indColor = (val >= paramCenter) ? colRight : colLeft;

        g.setColour(indColor.brighter(0.4f));
        g.fillRoundedRectangle(indicatorX - indHalfW,
                               trackY - 2.0f,
                               indHalfW * 2.0f,
                               trackH  + 4.0f,
                               indHalfW);

        // ── 7. Focus ring (WCAG 2.4.7) ───────────────────────────────────
        if (hasKeyboardFocus(true))
            A11y::drawFocusRing(g, b, trackR);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown()) return;

        // Cmd+click → reset to center (mirrors GalleryKnob pattern)
        if (e.mods.isCommandDown())
        {
            slider.setValue(paramCenter, juce::sendNotificationSync);
            return;
        }

        setValueFromMouseX(e.position.x);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown()) return;
        setValueFromMouseX(e.position.x);
    }

    void mouseDoubleClick(const juce::MouseEvent& /*e*/) override
    {
        // Double-click → reset to center
        slider.setValue(paramCenter, juce::sendNotificationSync);
    }

    void resized() override
    {
        // Hidden slider parked off-screen (zero-size safe).
        slider.setBounds(0, 0, 0, 0);
    }

private:
    // Convert a mouse x position (in component space) to a parameter value and
    // apply it via the slider → attachment → APVTS chain.
    void setValueFromMouseX(float mouseX)
    {
        float labelW   = juce::jmax(36.0f, (float)getHeight() * 2.0f);
        float barX     = labelW;
        float barRight = (float)getWidth() - labelW;
        float barWidth = barRight - barX;
        if (barWidth < 1.0f) return;

        float norm = juce::jlimit(0.0f, 1.0f, (mouseX - barX) / barWidth);
        double val = paramMin + norm * (paramMax - paramMin);
        slider.setValue(val, juce::sendNotificationSync);
    }

    //──────────────────────────────────────────────────────────────────────
    juce::String                        pid;
    juce::String                        lblLeft;
    juce::String                        lblRight;
    juce::Colour                        colLeft;
    juce::Colour                        colRight;

    double paramMin    = -1.0;
    double paramMax    =  1.0;
    double paramCenter =  0.0;
    double defaultVal  =  0.0;

    // Attachment vehicle: hidden slider.
    // Declaration order: slider before attachment (attachment binds to slider).
    juce::Slider                                                             slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BipolarAxisBar)
};

} // namespace xolokun
