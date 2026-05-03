// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// ModMatrixDrawer.h — Collapsible mod matrix panel for EngineDetailPanel.
//
// Displays up to 8 routing slots, each with:
//   Source ComboBox → Destination ComboBox → Depth Slider → Curve Picker → Q Toggle → Enable Toggle
//
// APVTS parameter discovery pattern:
//   {prefix}modSlot{i}Src   — Choice parameter (source)
//   {prefix}modSlot{i}Dst   — Choice parameter (destination)
//   {prefix}modSlot{i}Amt   — Float parameter, bipolar -1 to +1
//   {prefix}modSlot{i}Curve — Choice parameter (CurveShape: Linear/Exp/Log/S)
//   {prefix}modSlot{i}Quant — Bool parameter (quantize-to-scale)
//
// Expanded width: 260px (increased from 220px to accommodate curve+Q controls).
// Collapsed: 0px (only MOD tab in parent is visible).
// Call loadEngine(prefix) after engine changes; clear() before loading new engine.

#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include "../Tokens.h"
#include "../AccentColors.h"

namespace xoceanus
{

//==============================================================================
// ModMatrixSliderLnF — thin 3px track with teal accent for depth sliders.
class ModMatrixSliderLnF : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float /*minPos*/, float /*maxPos*/,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearHorizontal)
        {
            juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height,
                                                    sliderPos, 0, 0, style, slider);
            return;
        }

        const float trackH  = 3.0f;
        const float thumbR  = 4.0f;
        const float cy      = (float)y + (float)height * 0.5f;
        const float trackY  = cy - trackH * 0.5f;
        const float trackX  = (float)x + thumbR;
        const float trackW  = (float)width - thumbR * 2.0f;

        // Background track
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.10f));
        g.fillRoundedRectangle(trackX, trackY, trackW, trackH, trackH * 0.5f);

        // Filled portion (teal)
        float fillW = sliderPos - trackX;
        if (fillW > 0.0f)
        {
            g.setColour(juce::Colour(0xFF7FDBCA));
            g.fillRoundedRectangle(trackX, trackY, juce::jmin(fillW, trackW), trackH, trackH * 0.5f);
        }

        // Thumb — small teal circle
        g.setColour(juce::Colour(0xFF7FDBCA));
        g.fillEllipse(sliderPos - thumbR, cy - thumbR, thumbR * 2.0f, thumbR * 2.0f);
    }

    // Suppress default text box drawing
    void drawLabel(juce::Graphics&, juce::Label&) override {}
};

//==============================================================================
// ModMatrixComboLnF — ComboBox styling: dark glass bg, muted text, thin border.
class ModMatrixComboLnF : public juce::LookAndFeel_V4
{
public:
    void drawComboBox(juce::Graphics& g, int width, int height, bool /*isDown*/,
                      int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                      juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);

        // Bg: rgba(10,12,18,0.6)
        g.setColour(juce::Colour(10, 12, 18).withAlpha(0.6f));
        g.fillRoundedRectangle(bounds, 3.0f);

        // Border: rgba(200,204,216,0.06)
        g.setColour(juce::Colour(200, 204, 216).withAlpha(box.isEnabled() ? 0.06f : 0.03f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);

        // Tiny dropdown arrow
        const float arrowSize = 4.0f;
        const float arrowX    = (float)width - arrowSize - 4.0f;
        const float arrowY    = (float)height * 0.5f - arrowSize * 0.3f;
        juce::Path arrow;
        arrow.addTriangle(arrowX, arrowY, arrowX + arrowSize, arrowY, arrowX + arrowSize * 0.5f, arrowY + arrowSize * 0.6f);
        g.setColour(juce::Colour(200, 204, 216).withAlpha(box.isEnabled() ? 0.3f : 0.1f));
        g.fillPath(arrow);
    }

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        g.setColour(juce::Colour(18, 20, 30));
        g.fillRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 4.0f);
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.06f));
        g.drawRoundedRectangle(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f, 4.0f, 1.0f);
    }

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool /*isTicked*/, bool /*hasSubMenu*/,
                           const juce::String& text, const juce::String& /*shortcut*/,
                           const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/) override
    {
        if (isSeparator)
        {
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.05f));
            g.fillRect(area.reduced(4, 0).withHeight(1));
            return;
        }
        if (isHighlighted)
        {
            g.setColour(juce::Colour(0xFF7FDBCA).withAlpha(0.12f));
            g.fillRect(area);
        }
        g.setFont(GalleryFonts::value(9.0f));
        g.setColour(isActive ? juce::Colour(200, 204, 216).withAlpha(0.6f)
                             : juce::Colour(200, 204, 216).withAlpha(0.25f));
        g.drawText(text, area.reduced(5, 0), juce::Justification::centredLeft, true);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return GalleryFonts::value(9.0f);
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds(3, 0, box.getWidth() - 18, box.getHeight());
        label.setFont(GalleryFonts::value(9.0f));
        label.setColour(juce::Label::textColourId, juce::Colour(200, 204, 216).withAlpha(0.6f));
    }
};

//==============================================================================
// ModMatrixDrawer — collapsible panel showing up to 8 mod routing slots.
class ModMatrixDrawer : public juce::Component
{
public:
    static constexpr int kExpandedWidth = 260; // expanded from 220px to fit curve picker + Q toggle
    static constexpr int kHeaderHeight  = 26;
    static constexpr int kSlotHeight    = 26;
    static constexpr int kMaxSlots      = 8;

    // Callback when expanded state changes — parent uses this to re-layout.
    std::function<void(bool expanded)> onExpandedChanged;

    //==========================================================================
    explicit ModMatrixDrawer(juce::AudioProcessorValueTreeState& apvts)
        : apvts_(apvts)
    {
        setOpaque(false);

        // Fix #1424: expose mod matrix to screen readers.
        A11y::setup(*this,
                    "Modulation Matrix",
                    "Eight modulation slots: each assigns a source to a destination with depth control");

        for (int i = 0; i < kMaxSlots; ++i)
            buildSlotRow(i);

        setExpanded(false, /*notify=*/false);
    }

    ~ModMatrixDrawer() override
    {
        clearAttachments();
    }

    //==========================================================================
    // Load mod matrix params for the given engine prefix.
    // Scans APVTS for {prefix}modSlot{i}Src (i = 1..kMaxSlots).
    // Populates destination ComboBox choices from the Dst param's choices.
    // Returns the number of active slots found (0–8).
    int loadEngine(const juce::String& paramPrefix)
    {
        clearAttachments();
        activePrefix_ = paramPrefix;
        activeSlotCount_ = 0;

        const juce::String pfx = paramPrefix.endsWith("_") ? paramPrefix : (paramPrefix + "_");

        for (int i = 0; i < kMaxSlots; ++i)
        {
            const int slotNum = i + 1; // 1-indexed
            const juce::String srcId   = pfx + "modSlot" + juce::String(slotNum) + "Src";
            const juce::String dstId   = pfx + "modSlot" + juce::String(slotNum) + "Dst";
            const juce::String amtId   = pfx + "modSlot" + juce::String(slotNum) + "Amt";
            const juce::String curveId = pfx + "modSlot" + juce::String(slotNum) + "Curve";
            const juce::String quantId = pfx + "modSlot" + juce::String(slotNum) + "Quant";

            auto* srcParam   = apvts_.getParameter(srcId);
            auto* dstParam   = apvts_.getParameter(dstId);
            auto* amtParam   = apvts_.getParameter(amtId);
            auto* curveParam = apvts_.getParameter(curveId);
            auto* quantParam = apvts_.getParameter(quantId);

            const bool slotExists = (srcParam != nullptr);

            if (slotExists)
            {
                ++activeSlotCount_;

                // Source ComboBox — standard sources always available
                slots_[i].srcCombo->setEnabled(true);
                slots_[i].srcCombo->clear(juce::dontSendNotification);
                slots_[i].srcCombo->addItem("None",       1);
                slots_[i].srcCombo->addItem("LFO 1",      2);
                slots_[i].srcCombo->addItem("LFO 2",      3);
                slots_[i].srcCombo->addItem("Envelope",   4);
                slots_[i].srcCombo->addItem("Velocity",   5);
                slots_[i].srcCombo->addItem("Key Track",  6);
                slots_[i].srcCombo->addItem("Mod Wheel",  7);
                slots_[i].srcCombo->addItem("Aftertouch", 8);

                // Destination ComboBox — populated from engine's Dst param choices
                slots_[i].dstCombo->setEnabled(true);
                slots_[i].dstCombo->clear(juce::dontSendNotification);
                if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(dstParam))
                {
                    int itemId = 1;
                    for (const auto& choice : choiceParam->choices)
                        slots_[i].dstCombo->addItem(choice, itemId++);
                }
                else
                {
                    // Fallback: single "Engine Default" entry if no choice param
                    slots_[i].dstCombo->addItem("Engine Default", 1);
                    slots_[i].dstCombo->setEnabled(false);
                }

                // Wire APVTS attachments
                srcAttachments_[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                    apvts_, srcId, *slots_[i].srcCombo);

                if (dstParam && dynamic_cast<juce::AudioParameterChoice*>(dstParam))
                    dstAttachments_[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                        apvts_, dstId, *slots_[i].dstCombo);

                if (amtParam)
                    amtAttachments_[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                        apvts_, amtId, *slots_[i].depthSlider);

                // D9 E3: wire curve picker → APVTS (read initial value; updates via callback)
                if (curveParam)
                {
                    const int curveVal = juce::roundToInt(curveParam->getValue()
                                            * (float)(curveParam->getNumSteps() - 1));
                    slots_[i].curvePicker->setSelectedIndex(curveVal, /*notify=*/false);
                    slots_[i].curvePicker->onCurveSelected = [this, curveId](int idx) {
                        if (auto* p = apvts_.getParameter(curveId))
                            p->setValueNotifyingHost(p->convertTo0to1((float)idx));
                    };
                    slots_[i].curvePicker->setEnabled(true);
                }
                else
                {
                    slots_[i].curvePicker->setSelectedIndex(0, false);
                    slots_[i].curvePicker->setEnabled(false);
                }

                // D9 E3: wire quantize toggle → APVTS (read initial value; updates via callback)
                if (quantParam)
                {
                    slots_[i].quantToggle->setQuantized(quantParam->getValue() >= 0.5f, /*notify=*/false);
                    slots_[i].quantToggle->onToggled = [this, quantId](bool on) {
                        if (auto* p = apvts_.getParameter(quantId))
                            p->setValueNotifyingHost(on ? 1.0f : 0.0f);
                    };
                    slots_[i].quantToggle->setEnabled(true);
                }
                else
                {
                    slots_[i].quantToggle->setQuantized(false, false);
                    slots_[i].quantToggle->setEnabled(false);
                }

                slots_[i].enableToggle->setActive(true);
                slots_[i].enableToggle->setEnabled(true);
                slots_[i].slotLabel->setAlpha(1.0f);
            }
            else
            {
                // Slot doesn't exist for this engine — show grayed out
                slots_[i].srcCombo->clear(juce::dontSendNotification);
                slots_[i].srcCombo->addItem(juce::String(juce::CharPointer_UTF8("\xe2\x80\x94")), 1);
                slots_[i].srcCombo->setEnabled(false);
                slots_[i].dstCombo->clear(juce::dontSendNotification);
                slots_[i].dstCombo->addItem(juce::String(juce::CharPointer_UTF8("\xe2\x80\x94")), 1);
                slots_[i].dstCombo->setEnabled(false);
                slots_[i].curvePicker->setSelectedIndex(0, false);
                slots_[i].curvePicker->setEnabled(false);
                slots_[i].quantToggle->setQuantized(false, false);
                slots_[i].quantToggle->setEnabled(false);
                slots_[i].enableToggle->setActive(false);
                slots_[i].enableToggle->setEnabled(false);
                slots_[i].slotLabel->setAlpha(0.3f);
            }
        }

        repaint();
        return activeSlotCount_;
    }

    // Clear all APVTS attachments. Call before loadEngine() for a new engine.
    void clear()
    {
        clearAttachments();
        activeSlotCount_ = 0;
        activePrefix_ = {};

        for (int i = 0; i < kMaxSlots; ++i)
        {
            slots_[i].srcCombo->clear(juce::dontSendNotification);
            slots_[i].dstCombo->clear(juce::dontSendNotification);
            slots_[i].srcCombo->setEnabled(false);
            slots_[i].dstCombo->setEnabled(false);
            slots_[i].curvePicker->setSelectedIndex(0, false);
            slots_[i].curvePicker->setEnabled(false);
            slots_[i].quantToggle->setQuantized(false, false);
            slots_[i].quantToggle->setEnabled(false);
            slots_[i].enableToggle->setActive(false);
            slots_[i].enableToggle->setEnabled(false);
        }
        repaint();
    }

    //==========================================================================
    bool isExpanded() const noexcept { return expanded_; }

    void setExpanded(bool expanded, bool notify = true)
    {
        if (expanded_ == expanded)
            return;
        expanded_ = expanded;
        setVisible(expanded_); // parent controls visibility / width animation
        if (notify && onExpandedChanged)
            onExpandedChanged(expanded_);
        repaint();
    }

    void toggleExpanded() { setExpanded(!expanded_); }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        // Background — submarine dark glass
        g.setColour(juce::Colour(24, 27, 38));
        g.fillRect(b);

        // Right border accent line
        g.setColour(XO::Tokens::Color::accent().withAlpha(0.06f));
        g.fillRect(b.getRight() - 1.0f, b.getY(), 1.0f, b.getHeight());

        // Header region
        auto headerBounds = b.removeFromTop((float)kHeaderHeight);

        // Header bottom separator
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.04f));
        g.fillRect(headerBounds.getX(), headerBounds.getBottom() - 1.0f,
                   headerBounds.getWidth(), 1.0f);

        // "MOD MATRIX" title
        g.setFont(GalleryFonts::heading(10.0f));
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.4f));
        const int arrowW = 14;
        g.drawText("MOD MATRIX",
                   (int)headerBounds.getX() + 8,
                   (int)headerBounds.getY(),
                   (int)headerBounds.getWidth() - arrowW - 10,
                   (int)headerBounds.getHeight(),
                   juce::Justification::centredLeft, false);

        // Collapse arrow ▾ (rotated 90° when expanded to indicate closeable)
        g.setFont(juce::Font(10.0f));
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.35f));
        g.drawText("v",
                   (int)headerBounds.getRight() - arrowW - 4,
                   (int)headerBounds.getY(),
                   arrowW,
                   (int)headerBounds.getHeight(),
                   juce::Justification::centred, false);

        // Slot row bottom borders
        for (int i = 0; i < kMaxSlots; ++i)
        {
            const int rowY = kHeaderHeight + i * kSlotHeight;
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.03f));
            g.fillRect(0, rowY + kSlotHeight - 1, getWidth(), 1);
        }
    }

    void resized() override
    {
        const int w = getWidth();

        for (int i = 0; i < kMaxSlots; ++i)
        {
            const int rowY = kHeaderHeight + i * kSlotHeight;
            layoutSlotRow(i, 0, rowY, w, kSlotHeight);
        }
    }

    // Header click — collapse the drawer
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.y < kHeaderHeight)
            toggleExpanded();
    }

private:
    //==========================================================================
    // EnableToggle — a simple 12x12 square toggle painted directly.
    class EnableToggle : public juce::Component
    {
    public:
        EnableToggle()
        {
            setWantsKeyboardFocus(true);
            setTitle("Enable routing slot");
            setDescription("Toggle to enable or disable this mod routing slot.");
        }

        bool isActive() const noexcept { return active_; }

        void setActive(bool a)
        {
            active_ = a;
            repaint();
        }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat().reduced(1.0f);

            // Border
            g.setColour(juce::Colour(200, 204, 216).withAlpha(isEnabled() ? 0.12f : 0.05f));
            g.drawRect(b, 1.0f);

            // Teal fill when active
            if (active_ && isEnabled())
            {
                g.setColour(juce::Colour(0xFF7FDBCA).withAlpha(0.75f));
                g.fillRect(b.reduced(2.0f));
            }

            if (hasKeyboardFocus(false))
                A11y::drawFocusRing(g, getLocalBounds().toFloat(), 4.0f);
        }

        void mouseDown(const juce::MouseEvent&) override
        {
            if (isEnabled())
            {
                active_ = !active_;
                repaint();
                if (onToggled)
                    onToggled(active_);
            }
        }

        bool keyPressed(const juce::KeyPress& key) override
        {
            if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
            {
                if (isEnabled())
                {
                    active_ = !active_;
                    repaint();
                    if (onToggled)
                        onToggled(active_);
                }
                return true;
            }
            return false;
        }

        std::function<void(bool)> onToggled;

    private:
        bool active_ = false;
    };

    //==========================================================================
    // CurvePicker — 4-button segmented control for CurveShape selection.
    // Renders tiny glyph thumbnails (16×12 px each) for Linear/Exp/Log/S.
    // WCAG AAA: active segment uses chainAccent (≥7:1 against dark bg).
    class CurvePicker : public juce::Component
    {
    public:
        static constexpr int kNumShapes = 4; // Linear, Exp, Log, S
        static constexpr int kSegW      = 16;
        static constexpr int kSegH      = 12;

        std::function<void(int)> onCurveSelected; // callback with shape index 0–3

        CurvePicker()
        {
            setWantsKeyboardFocus(true);
            setTitle("Curve shape");
            setDescription("Select modulation curve shape: Linear, Exponential, Logarithmic, or S-curve.");
        }

        int getSelectedIndex() const noexcept { return selectedIdx_; }

        void setSelectedIndex(int idx, bool notify = true)
        {
            idx = juce::jlimit(0, kNumShapes - 1, idx);
            if (selectedIdx_ == idx)
                return;
            selectedIdx_ = idx;
            repaint();
            if (notify && onCurveSelected)
                onCurveSelected(selectedIdx_);
        }

        void paint(juce::Graphics& g) override
        {
            const int w = kSegW;
            const int h = kSegH;

            for (int i = 0; i < kNumShapes; ++i)
            {
                const int x = i * w;
                const bool active = (i == selectedIdx_);

                // Background: dim when inactive, chain-cool tint when active
                if (active)
                    g.setColour(XOceanus::AccentColors::chainAccent.withAlpha(0.18f));
                else
                    g.setColour(juce::Colour(200, 204, 216).withAlpha(0.05f));
                g.fillRect(x, 0, w, h);

                // Border
                g.setColour(active ? XOceanus::AccentColors::chainAccent.withAlpha(0.5f)
                                   : juce::Colour(200, 204, 216).withAlpha(0.10f));
                g.drawRect(x, 0, w, h, 1);

                // Glyph — tiny waveform thumbnail
                paintCurveGlyph(g, i, x, 0, w, h, active);
            }

            // Keyboard focus ring
            if (hasKeyboardFocus(false))
                A11y::drawFocusRing(g, getLocalBounds().toFloat(), 3.0f);
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            const int idx = e.x / kSegW;
            if (idx >= 0 && idx < kNumShapes)
                setSelectedIndex(idx, true);
        }

        bool keyPressed(const juce::KeyPress& key) override
        {
            if (key == juce::KeyPress::leftKey)  { setSelectedIndex(selectedIdx_ - 1, true); return true; }
            if (key == juce::KeyPress::rightKey) { setSelectedIndex(selectedIdx_ + 1, true); return true; }
            return false;
        }

    private:
        // Paint a tiny glyph representing the curve shape.
        // Each glyph is a 3-point polyline drawn in the centre 12×8 px of the segment.
        static void paintCurveGlyph(juce::Graphics& g, int shape,
                                    int bx, int by, int bw, int bh,
                                    bool active)
        {
            const juce::Colour col = active ? XOceanus::AccentColors::chainAccent
                                            : juce::Colour(200, 204, 216).withAlpha(0.40f);
            g.setColour(col);

            // Glyph area — leave 2px border each side
            const float gx = (float)bx + 2.0f;
            const float gy = (float)by + 2.0f;
            const float gw = (float)bw - 4.0f;
            const float gh = (float)bh - 4.0f;

            juce::Path p;
            // Each curve is described as points in normalised [0,1]×[0,1] space
            // (x=phase, y=output, y=0 at bottom, y=1 at top)
            switch (shape)
            {
                case 0: // Linear: diagonal from bottom-left to top-right
                {
                    p.startNewSubPath(gx, gy + gh);
                    p.lineTo(gx + gw, gy);
                    break;
                }
                case 1: // Exp: concave-up (starts slow, ends fast)
                {
                    p.startNewSubPath(gx, gy + gh);
                    p.quadraticTo(gx + gw * 0.8f, gy + gh, gx + gw, gy);
                    break;
                }
                case 2: // Log: concave-down (starts fast, ends slow)
                {
                    p.startNewSubPath(gx, gy + gh);
                    p.quadraticTo(gx, gy, gx + gw, gy);
                    break;
                }
                case 3: // S-curve: smoothstep
                {
                    p.startNewSubPath(gx, gy + gh);
                    p.cubicTo(gx + gw * 0.2f, gy + gh,
                              gx + gw * 0.8f, gy,
                              gx + gw, gy);
                    break;
                }
                default:
                    p.startNewSubPath(gx, gy + gh);
                    p.lineTo(gx + gw, gy);
                    break;
            }

            g.strokePath(p, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
        }

        int selectedIdx_ = 0; // 0 = Linear (default)
    };

    //==========================================================================
    // QuantizeToggle — small "Q" pill button.
    // Lit state uses chainAccent (chain cool palette, WCAG AAA).
    // Unlit is dimmed.
    class QuantizeToggle : public juce::Component
    {
    public:
        std::function<void(bool)> onToggled;

        QuantizeToggle()
        {
            setWantsKeyboardFocus(true);
            setTitle("Quantize to scale");
            setDescription("Toggle quantize-to-scale: routes pitch modulation to the nearest scale degree.");
        }

        bool isQuantized() const noexcept { return quantized_; }

        void setQuantized(bool q, bool notify = true)
        {
            if (quantized_ == q)
                return;
            quantized_ = q;
            repaint();
            if (notify && onToggled)
                onToggled(quantized_);
        }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();

            // Background
            if (quantized_ && isEnabled())
                g.setColour(XOceanus::AccentColors::chainAccent.withAlpha(0.20f));
            else
                g.setColour(juce::Colour(200, 204, 216).withAlpha(0.05f));
            g.fillRoundedRectangle(b, 3.0f);

            // Border
            if (quantized_ && isEnabled())
                g.setColour(XOceanus::AccentColors::chainAccent.withAlpha(0.60f));
            else
                g.setColour(juce::Colour(200, 204, 216).withAlpha(0.12f));
            g.drawRoundedRectangle(b.reduced(0.5f), 3.0f, 1.0f);

            // "Q" label — chainAccent when lit, dimmed when off
            g.setFont(GalleryFonts::value(7.0f));
            if (quantized_ && isEnabled())
                g.setColour(XOceanus::AccentColors::chainAccent); // AAA contrast
            else
                g.setColour(juce::Colour(200, 204, 216).withAlpha(isEnabled() ? 0.30f : 0.12f));
            g.drawText("Q", b.toNearestInt(), juce::Justification::centred, false);

            if (hasKeyboardFocus(false))
                A11y::drawFocusRing(g, b, 3.0f);
        }

        void mouseDown(const juce::MouseEvent&) override
        {
            if (isEnabled())
                setQuantized(!quantized_, true);
        }

        bool keyPressed(const juce::KeyPress& key) override
        {
            if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
            {
                if (isEnabled()) setQuantized(!quantized_, true);
                return true;
            }
            return false;
        }

    private:
        bool quantized_ = false;
    };

    //==========================================================================
    struct SlotRow
    {
        std::unique_ptr<juce::Label>        slotLabel;     // "1"–"8"
        std::unique_ptr<juce::ComboBox>     srcCombo;
        std::unique_ptr<juce::Label>        arrowLabel;    // "→"
        std::unique_ptr<juce::ComboBox>     dstCombo;
        std::unique_ptr<juce::Slider>       depthSlider;
        std::unique_ptr<CurvePicker>        curvePicker;   // D9 E3: Linear/Exp/Log/S
        std::unique_ptr<QuantizeToggle>     quantToggle;   // D9 E3: quantize-to-scale
        std::unique_ptr<EnableToggle>       enableToggle;

        // LookAndFeel instances owned per-slot to avoid cross-contamination
        std::unique_ptr<ModMatrixSliderLnF> sliderLnF;
        std::unique_ptr<ModMatrixComboLnF>  comboLnF;
    };

    //==========================================================================
    void buildSlotRow(int i)
    {
        auto& row = slots_[i];

        // Slot number label
        row.slotLabel = std::make_unique<juce::Label>();
        row.slotLabel->setText(juce::String(i + 1), juce::dontSendNotification);
        row.slotLabel->setFont(GalleryFonts::value(8.0f));
        row.slotLabel->setColour(juce::Label::textColourId, juce::Colour(200, 204, 216).withAlpha(0.2f));
        row.slotLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*row.slotLabel);

        // Source ComboBox
        row.comboLnF = std::make_unique<ModMatrixComboLnF>();
        row.srcCombo = std::make_unique<juce::ComboBox>();
        row.srcCombo->setLookAndFeel(row.comboLnF.get());
        row.srcCombo->setScrollWheelEnabled(true);
        addAndMakeVisible(*row.srcCombo);

        // Arrow label
        row.arrowLabel = std::make_unique<juce::Label>();
        row.arrowLabel->setText(u8"\u2192", juce::dontSendNotification);
        row.arrowLabel->setFont(juce::Font(9.0f));
        row.arrowLabel->setColour(juce::Label::textColourId, juce::Colour(200, 204, 216).withAlpha(0.2f));
        row.arrowLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*row.arrowLabel);

        // Destination ComboBox — shares the same LookAndFeel instance
        row.dstCombo = std::make_unique<juce::ComboBox>();
        row.dstCombo->setLookAndFeel(row.comboLnF.get());
        row.dstCombo->setScrollWheelEnabled(true);
        addAndMakeVisible(*row.dstCombo);

        // Depth slider
        row.sliderLnF = std::make_unique<ModMatrixSliderLnF>();
        row.depthSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
        row.depthSlider->setLookAndFeel(row.sliderLnF.get());
        row.depthSlider->setRange(0.0, 100.0, 0.1);
        row.depthSlider->setValue(0.0, juce::dontSendNotification);
        addAndMakeVisible(*row.depthSlider);

        // D9 E3: Curve picker — 4-segment Linear/Exp/Log/S
        row.curvePicker = std::make_unique<CurvePicker>();
        addAndMakeVisible(*row.curvePicker);

        // D9 E3: Quantize toggle — "Q" pill, chain-cool accent
        row.quantToggle = std::make_unique<QuantizeToggle>();
        addAndMakeVisible(*row.quantToggle);

        // Enable toggle
        row.enableToggle = std::make_unique<EnableToggle>();
        addAndMakeVisible(*row.enableToggle);
    }

    // Layout a single slot row within its bounding rect.
    // Layout (left→right, 4px padding top+bottom):
    //   [4px gap] [slotLabel 10px] [2px] [srcCombo ~40px] [arrowLabel 10px]
    //   [dstCombo ~40px] [2px] [depthSlider 32px] [2px] [curvePicker 64px] [2px] [Q 14px] [2px] [toggle 12px] [4px]
    void layoutSlotRow(int i, int rx, int ry, int rw, int rh)
    {
        const int padV    = 4;
        const int padH    = 4;
        const int inner   = rh - padV * 2;
        int x             = rx + padH;

        // Fixed-width columns
        const int labelW  = 10;
        const int arrowW  = 10;
        const int depthW  = 32;
        const int curveW  = CurvePicker::kSegW * CurvePicker::kNumShapes; // 4 × 16 = 64px
        const int quantW  = 14;
        const int toggleW = 12;

        // Budget: rw - padH*2 - labelW - arrowW - depthW - curveW - quantW - toggleW - gaps
        // gaps: 2 (after label) + 0 (before arrow, arrow follows srcCombo) + 2 (after dst) + 2 (after depth) + 2 (after curve) + 2 (after quant) + 4 (right pad)
        const int fixedUsed = padH + labelW + 2 + arrowW + depthW + curveW + quantW + toggleW + (2 + 2 + 2 + 2 + padH);
        const int comboTotal = rw - fixedUsed;
        const int comboW     = juce::jmax(24, comboTotal / 2);

        // Slot number label
        slots_[i].slotLabel->setBounds(x, ry + padV, labelW, inner);
        x += labelW + 2;

        // Source ComboBox
        slots_[i].srcCombo->setBounds(x, ry + padV, comboW, inner);
        x += comboW;

        // Arrow
        slots_[i].arrowLabel->setBounds(x, ry + padV, arrowW, inner);
        x += arrowW;

        // Destination ComboBox
        slots_[i].dstCombo->setBounds(x, ry + padV, comboW, inner);
        x += comboW + 2;

        // Depth slider
        slots_[i].depthSlider->setBounds(x, ry + padV, depthW, inner);
        x += depthW + 2;

        // Curve picker — 4 × 16px = 64px wide, full inner height
        const int curveY = ry + (rh - CurvePicker::kSegH) / 2;
        slots_[i].curvePicker->setBounds(x, curveY, curveW, CurvePicker::kSegH);
        x += curveW + 2;

        // Quantize toggle — "Q" pill, 14×12px centred vertically
        const int quantH = 12;
        const int quantY = ry + (rh - quantH) / 2;
        slots_[i].quantToggle->setBounds(x, quantY, quantW, quantH);
        x += quantW + 2;

        // Enable toggle — 12×12px centred vertically
        const int toggleY = ry + (rh - toggleW) / 2;
        slots_[i].enableToggle->setBounds(x, toggleY, toggleW, toggleW);
    }

    void clearAttachments()
    {
        for (int i = 0; i < kMaxSlots; ++i)
        {
            srcAttachments_[i].reset();
            dstAttachments_[i].reset();
            amtAttachments_[i].reset();
            // Curve and quant are wired via callbacks — clear them to prevent
            // dangling captures to old APVTS parameter IDs after engine change.
            slots_[i].curvePicker->onCurveSelected = nullptr;
            slots_[i].quantToggle->onToggled = nullptr;
        }
    }

    //==========================================================================
    juce::AudioProcessorValueTreeState& apvts_;

    SlotRow slots_[kMaxSlots];

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        srcAttachments_[kMaxSlots];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        dstAttachments_[kMaxSlots];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        amtAttachments_[kMaxSlots];

    juce::String activePrefix_;
    int          activeSlotCount_ = 0;
    bool         expanded_        = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrixDrawer)
};

} // namespace xoceanus
