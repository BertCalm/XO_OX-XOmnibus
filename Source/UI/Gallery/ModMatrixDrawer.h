// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// ModMatrixDrawer.h — Collapsible mod matrix panel for EngineDetailPanel.
//
// Displays up to 8 routing slots, each with:
//   Source ComboBox → Destination ComboBox → Depth Slider → Enable Toggle
//
// APVTS parameter discovery pattern:
//   {prefix}modSlot{i}Src  — Choice parameter (source)
//   {prefix}modSlot{i}Dst  — Choice parameter (destination)
//   {prefix}modSlot{i}Amt  — Float parameter, bipolar -1 to +1
//
// Expanded width: 220px. Collapsed: 0px (only MOD tab in parent is visible).
// Call loadEngine(prefix) after engine changes; clear() before loading new engine.

#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"

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
    static constexpr int kExpandedWidth = 220;
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
            const juce::String srcId = pfx + "modSlot" + juce::String(slotNum) + "Src";
            const juce::String dstId = pfx + "modSlot" + juce::String(slotNum) + "Dst";
            const juce::String amtId = pfx + "modSlot" + juce::String(slotNum) + "Amt";

            auto* srcParam = apvts_.getParameter(srcId);
            auto* dstParam = apvts_.getParameter(dstId);
            auto* amtParam = apvts_.getParameter(amtId);

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

                slots_[i].enableToggle->setActive(true);
                slots_[i].enableToggle->setEnabled(true);
                slots_[i].slotLabel->setAlpha(1.0f);
            }
            else
            {
                // Slot doesn't exist for this engine — show grayed out
                slots_[i].srcCombo->clear(juce::dontSendNotification);
                slots_[i].srcCombo->addItem("—", 1);
                slots_[i].srcCombo->setEnabled(false);
                slots_[i].dstCombo->clear(juce::dontSendNotification);
                slots_[i].dstCombo->addItem("—", 1);
                slots_[i].dstCombo->setEnabled(false);
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
        g.setColour(juce::Colour(60, 180, 170).withAlpha(0.06f));
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
    struct SlotRow
    {
        std::unique_ptr<juce::Label>        slotLabel;     // "1"–"8"
        std::unique_ptr<juce::ComboBox>     srcCombo;
        std::unique_ptr<juce::Label>        arrowLabel;    // "→"
        std::unique_ptr<juce::ComboBox>     dstCombo;
        std::unique_ptr<juce::Slider>       depthSlider;
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

        // Enable toggle
        row.enableToggle = std::make_unique<EnableToggle>();
        addAndMakeVisible(*row.enableToggle);
    }

    // Layout a single slot row within its bounding rect.
    // Layout (left→right, 4px padding top+bottom):
    //   [4px gap] [slotLabel 10px] [2px] [srcCombo ~54px] [arrowLabel 10px]
    //   [dstCombo ~54px] [2px] [depthSlider 40px] [2px] [toggle 12px] [4px]
    void layoutSlotRow(int i, int rx, int ry, int rw, int rh)
    {
        const int padV  = 4;
        const int padH  = 4;
        const int inner = rh - padV * 2;
        int x           = rx + padH;

        // Slot number label — narrow column
        const int labelW = 10;
        slots_[i].slotLabel->setBounds(x, ry + padV, labelW, inner);
        x += labelW + 2;

        // Remaining width budget: rw - padH*2 - labelW - 2 - 10 (arrow) - 40 (depth) - 12 (toggle) - 2 - 2 - 2
        // Two combo boxes share the remaining space equally
        const int arrowW  = 10;
        const int depthW  = 40;
        const int toggleW = 12;
        const int gaps    = 2 + 2 + 2 + 2 + padH; // arrow gaps + depth gap + toggle gap + right pad
        const int comboTotal = rw - padH - labelW - 2 - arrowW - depthW - toggleW - gaps;
        const int comboW     = juce::jmax(30, comboTotal / 2);

        // Source ComboBox
        slots_[i].srcCombo->setBounds(x, ry + padV, comboW, inner);
        x += comboW + 2;

        // Arrow
        slots_[i].arrowLabel->setBounds(x, ry + padV, arrowW, inner);
        x += arrowW;

        // Destination ComboBox
        slots_[i].dstCombo->setBounds(x, ry + padV, comboW, inner);
        x += comboW + 2;

        // Depth slider
        slots_[i].depthSlider->setBounds(x, ry + padV, depthW, inner);
        x += depthW + 2;

        // Enable toggle — centered vertically, 12x12
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
