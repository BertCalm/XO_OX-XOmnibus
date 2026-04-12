// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// CouplingConfigPopup.h — Floating coupling configuration popup.
//
// Shows coupling type (tiered), direction, depth, and flow visualization
// when a coupling knot is double-clicked in the ocean.
//
// Design: Docs/design/ocean-ui-phase1-design.md
// Prototype: Tools/ui-preview/submarine.html (coupling-popup class)

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"

namespace xoceanus
{

struct CouplingTypeInfo
{
    juce::String name;
    juce::String label;
    juce::String tier;      // "safe", "standard", "caution", "exotic"
    juce::String desc;
    juce::String warning;   // empty if none
    juce::String sourceParam;
    juce::String destParam;
    juce::Colour tierColour;
};

class CouplingConfigPopup : public juce::Component
{
public:
    CouplingConfigPopup()
    {
        setInterceptsMouseClicks(true, true);
        setWantsKeyboardFocus(true);

        // Backdrop
        backdrop_.setInterceptsMouseClicks(true, false);
        backdrop_.onClick = [this]() { hide(); };
        addAndMakeVisible(backdrop_);

        // Type selector
        typeSelector_.onChange = [this]()
        {
            updateFlowDisplay();
            repaint();
        };
        addAndMakeVisible(typeSelector_);

        // Depth slider
        depthSlider_.setRange(0.0, 1.0, 0.01);
        depthSlider_.setValue(0.5);
        depthSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
        depthSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
        depthSlider_.setColour(juce::Slider::thumbColourId, juce::Colour(60, 180, 170));
        depthSlider_.setColour(juce::Slider::trackColourId, juce::Colour(200, 204, 216).withAlpha(0.1f));
        addAndMakeVisible(depthSlider_);

        // Direction buttons
        for (int i = 0; i < 3; ++i)
        {
            auto* btn = dirButtons_.add(new juce::TextButton(i == 0 ? juce::String::charToString(0x2192)  // →
                                                            : i == 1 ? juce::String::charToString(0x2190)  // ←
                                                                     : juce::String::charToString(0x2194))); // ↔
            btn->setColour(juce::TextButton::buttonColourId, juce::Colour(200, 204, 216).withAlpha(0.04f));
            btn->setColour(juce::TextButton::textColourOffId, juce::Colour(200, 204, 216).withAlpha(0.5f));
            btn->onClick = [this, i]()
            {
                activeDir_ = i;
                updateDirectionButtons();
                repaint();
            };
            addAndMakeVisible(btn);
        }
        activeDir_ = 0;
        updateDirectionButtons();

        // Done button
        doneBtn_.setButtonText("Done");
        doneBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 180, 170).withAlpha(0.1f));
        doneBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(60, 180, 170).withAlpha(0.9f));
        doneBtn_.onClick = [this]() { hide(); };
        addAndMakeVisible(doneBtn_);

        // Remove button
        removeBtn_.setButtonText("Remove");
        removeBtn_.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        removeBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(239, 68, 68).withAlpha(0.6f));
        removeBtn_.onClick = [this]()
        {
            if (onRemove) onRemove(currentRouteIndex_);
            hide();
        };
        addAndMakeVisible(removeBtn_);

        populateTypeSelector();
    }

    void show(int routeIndex, const juce::String& sourceName, juce::Colour sourceColour,
              const juce::String& destName, juce::Colour destColour,
              int couplingType, float depth)
    {
        currentRouteIndex_ = routeIndex;
        sourceName_ = sourceName;
        sourceColour_ = sourceColour;
        destName_ = destName;
        destColour_ = destColour;

        typeSelector_.setSelectedId(couplingType + 1, juce::dontSendNotification);
        depthSlider_.setValue(static_cast<double>(depth), juce::dontSendNotification);

        updateFlowDisplay();
        setVisible(true);
        resized();
        grabKeyboardFocus();
    }

    void hide()
    {
        if (onConfigChanged && currentRouteIndex_ >= 0)
        {
            onConfigChanged(currentRouteIndex_,
                           typeSelector_.getSelectedId() - 1,
                           static_cast<float>(depthSlider_.getValue()),
                           activeDir_);
        }
        setVisible(false);
        currentRouteIndex_ = -1;
    }

    void paint(juce::Graphics& g) override
    {
        auto panel = getPanelBounds().toFloat();

        // Panel shadow + body
        juce::DropShadow(juce::Colours::black.withAlpha(0.6f), 24, { 0, 8 })
            .drawForRectangle(g, panel.toNearestInt());
        g.setColour(juce::Colour(30, 33, 40).withAlpha(0.97f));
        g.fillRoundedRectangle(panel, 14.0f);
        g.setColour(juce::Colour(60, 180, 170).withAlpha(0.18f));
        g.drawRoundedRectangle(panel, 14.0f, 1.0f);

        // Direction bar background
        auto dirBar = panel.removeFromTop(48.0f);
        g.setColour(juce::Colour(60, 180, 170).withAlpha(0.06f));
        g.fillRoundedRectangle(dirBar, 14.0f);

        // Source name (left side of direction bar)
        g.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
        g.setColour(sourceColour_.withAlpha(0.8f));
        g.drawText(sourceName_,
                   juce::Rectangle<float>(panel.getX(), dirBar.getY(), 100.0f, dirBar.getHeight()).toNearestInt(),
                   juce::Justification::centred, true);

        // Dest name (right side of direction bar)
        g.setColour(destColour_.withAlpha(0.8f));
        g.drawText(destName_,
                   juce::Rectangle<float>(panel.getRight() - 100.0f, dirBar.getY(), 100.0f, dirBar.getHeight()).toNearestInt(),
                   juce::Justification::centred, true);

        // Flow description
        auto flowArea = panel.removeFromTop(80.0f);
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.4f));
        g.drawText(currentDesc_, flowArea.reduced(20, 0).toNearestInt(),
                   juce::Justification::centred, true);

        // Warning (if any)
        if (currentWarning_.isNotEmpty())
        {
            auto warnArea = panel.removeFromTop(24.0f);
            g.setColour(juce::Colour(245, 158, 11).withAlpha(0.06f));
            g.fillRect(warnArea);
            g.setColour(juce::Colour(245, 158, 11).withAlpha(0.75f));
            g.setFont(juce::Font(juce::FontOptions(10.0f)));
            g.drawText(juce::String::charToString(0x26A0) + " " + currentWarning_,
                       warnArea.reduced(16, 0).toNearestInt(),
                       juce::Justification::centredLeft, true);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        backdrop_.setBounds(bounds);

        auto panel = getPanelBounds();

        // Direction bar (top 48px)
        auto dirBar = panel.removeFromTop(48);
        auto dirCenter = dirBar.reduced(120, 8);
        int dirBtnW = 36;
        int dirX = dirCenter.getCentreX() - (dirBtnW * 3 + 8) / 2;
        for (int i = 0; i < 3; ++i)
        {
            dirButtons_[i]->setBounds(dirX, dirCenter.getY(), dirBtnW, dirCenter.getHeight());
            dirX += dirBtnW + 4;
        }

        // Flow area (80px)
        panel.removeFromTop(80);

        // Warning area if applicable
        if (currentWarning_.isNotEmpty())
            panel.removeFromTop(24);

        // Type selector + depth (middle)
        auto controlRow = panel.removeFromTop(36).reduced(16, 4);
        typeSelector_.setBounds(controlRow.removeFromLeft(180));
        controlRow.removeFromLeft(12);
        depthSlider_.setBounds(controlRow);

        // Footer (done + remove)
        auto footer = panel.removeFromTop(40).reduced(16, 4);
        removeBtn_.setBounds(footer.removeFromLeft(80));
        doneBtn_.setBounds(footer.removeFromRight(80));
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey) { hide(); return true; }
        return false;
    }

    // Callbacks
    std::function<void(int routeIndex, int newType, float newDepth, int direction)> onConfigChanged;
    std::function<void(int routeIndex)> onRemove;

private:
    juce::Rectangle<int> getPanelBounds() const
    {
        auto bounds = getLocalBounds();
        int w = std::min(440, bounds.getWidth() - 40);
        int h = std::min(320, bounds.getHeight() - 40);
        return { (bounds.getWidth() - w) / 2, (bounds.getHeight() - h) / 2, w, h };
    }

    void populateTypeSelector()
    {
        typeSelector_.addSectionHeading("SAFE");
        typeSelector_.addItem("Amp -> Filter", 1);
        typeSelector_.addItem("Amp -> Pitch", 2);
        typeSelector_.addItem("LFO -> Pitch", 3);
        typeSelector_.addItem("Env -> Morph", 4);
        typeSelector_.addItem("Env -> Decay", 5);
        typeSelector_.addSectionHeading("STANDARD");
        typeSelector_.addItem("Filter -> Filter", 6);
        typeSelector_.addItem("Pitch -> Pitch", 7);
        typeSelector_.addItem("Rhythm -> Blend", 8);
        typeSelector_.addItem("Amp -> Choke", 9);
        typeSelector_.addSectionHeading("CAUTION");
        typeSelector_.addItem("Audio -> FM", 10);
        typeSelector_.addItem("Audio -> Ring", 11);
        typeSelector_.addItem("Audio -> Wavetable", 12);
        typeSelector_.addItem("Audio -> Buffer", 13);
        typeSelector_.addSectionHeading("EXOTIC");
        typeSelector_.addItem("Knot Topology", 14);
        typeSelector_.addItem("Triangular", 15);
        typeSelector_.setSelectedId(1);
    }

    void updateFlowDisplay()
    {
        int id = typeSelector_.getSelectedId();
        static const char* descs[] = {
            "", // 0
            "Engine loudness modulates filter cutoff",
            "Engine loudness modulates pitch",
            "LFO cross-modulation of pitch",
            "Envelope drives wavetable/morph position",
            "Envelope controls decay time",
            "Filter output feeds another filter",
            "Harmonic pitch tracking between engines",
            "Rhythm patterns drive blend/morph",
            "Engine A amplitude chokes Engine B",
            "Raw audio as FM modulation source",
            "Ring modulation between engines",
            "Audio snapshots used as wavetable source",
            "Stereo audio stream to grain buffer",
            "Bidirectional — both engines modulate each other",
            "Love-triangle state transfer (I/P/C)"
        };
        static const char* warnings[] = {
            "", "", "", "", "", "",
            "Narrow support — works best with OPTIC",
            "", "",
            "Best with percussion engines (ONSET, OVERBITE)",
            "Audio-rate — feedback loops are blocked automatically",
            "Risk of DC drift if both engines have DC offset",
            "Best with band-limited sources",
            "Only OPAL supports this as a destination",
            "Irreducible coupling — neither engine can be separated",
            "Designed for Oxytocin — degrades for other engines"
        };

        currentDesc_ = (id > 0 && id <= 15) ? descs[id] : "";
        currentWarning_ = (id > 0 && id <= 15) ? warnings[id] : "";
    }

    void updateDirectionButtons()
    {
        for (int i = 0; i < 3; ++i)
        {
            bool active = (i == activeDir_);
            dirButtons_[i]->setColour(juce::TextButton::buttonColourId,
                active ? juce::Colour(60, 180, 170).withAlpha(0.15f)
                       : juce::Colour(200, 204, 216).withAlpha(0.04f));
            dirButtons_[i]->setColour(juce::TextButton::textColourOffId,
                active ? juce::Colour(60, 180, 170).withAlpha(0.9f)
                       : juce::Colour(200, 204, 216).withAlpha(0.5f));
        }
    }

    struct Backdrop : public juce::Component
    {
        void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black.withAlpha(0.4f)); }
        std::function<void()> onClick;
        void mouseDown(const juce::MouseEvent&) override { if (onClick) onClick(); }
    };

    Backdrop backdrop_;
    juce::ComboBox typeSelector_;
    juce::Slider depthSlider_;
    juce::OwnedArray<juce::TextButton> dirButtons_;
    juce::TextButton doneBtn_, removeBtn_;

    int currentRouteIndex_ = -1;
    int activeDir_ = 0;
    juce::String sourceName_, destName_;
    juce::Colour sourceColour_, destColour_;
    juce::String currentDesc_, currentWarning_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CouplingConfigPopup)
};

} // namespace xoceanus
