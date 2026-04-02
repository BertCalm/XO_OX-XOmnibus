// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"

namespace xoceanus {

class OutshineGrainStrip : public juce::Component
{
public:
    std::function<void(const juce::StringArray&)> onGrainsChanged;

    OutshineGrainStrip()
    {
        setWantsKeyboardFocus(true);
        A11y::setup(*this, "Grain Strip", "List of loaded audio grains");
        addAndMakeVisible(scrollContainer);
        scrollContainer.setViewedComponent(&chipHolder, false);
        addAndMakeVisible(countLabel);
        countLabel.setFont(GalleryFonts::value(11.0f));
        countLabel.setColour(juce::Label::textColourId,
                             GalleryColors::get(GalleryColors::textMid()));
        countLabel.setJustificationType(juce::Justification::centredRight);
    }

    void addGrains(const juce::StringArray& paths)
    {
        for (const auto& p : paths)
        {
            if (!grainPaths.contains(p))
                grainPaths.add(p);
        }
        rebuildChips();
        notifyChanged();
    }

    void removeGrain(int index)
    {
        if (index >= 0 && index < grainPaths.size())
        {
            grainPaths.remove(index);
            rebuildChips();
            notifyChanged();
        }
    }

    void clear()
    {
        grainPaths.clear();
        rebuildChips();
        notifyChanged();
    }

    juce::StringArray getGrainPaths() const { return grainPaths; }

    /** Scroll the strip so that the chip at 'index' is visible and highlight it
        briefly with an accent border.  Called from OutshineZoneMap::onZoneClicked
        so that clicking a zone in the zone map reveals the corresponding grain chip.
        Safe to call with out-of-range indices — silently ignored. */
    void highlightGrain (int index)
    {
        if (index < 0 || index >= chips.size()) return;

        // Record the new highlight index and repaint (chip paint path checks this).
        highlightedChipIndex = index;
        repaint();

        // Scroll the viewport so the chip is visible.
        if (auto* chip = chips[index])
        {
            auto chipBounds = chip->getBounds();
            scrollContainer.setViewPosition (
                juce::jmax (0, chipBounds.getX() - kChipGap), 0);
        }

        // Clear the highlight after 800 ms so it doesn't persist indefinitely.
        juce::Timer::callAfterDelay (800, [safeThis = juce::Component::SafePointer<OutshineGrainStrip>(this)]()
        {
            if (safeThis != nullptr)
            {
                safeThis->highlightedChipIndex = -1;
                safeThis->repaint();
            }
        });
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));
        // Top border separator
        g.setColour(GalleryColors::get(GalleryColors::borderGray()));
        g.drawLine(0, 0, (float)getWidth(), 0, 1.0f);

        // Draw accent ring around the highlighted chip (set by highlightGrain()).
        if (highlightedChipIndex >= 0 && highlightedChipIndex < chips.size())
        {
            if (auto* chip = chips[highlightedChipIndex])
            {
                // getBounds() is relative to chipHolder; convert to scrollContainer viewport.
                auto chipBoundsInHolder = chip->getBounds();
                auto viewPos = scrollContainer.getViewPosition();
                auto chipInView = chipBoundsInHolder.translated(-viewPos.x, 0);
                auto chipInStrip = chipInView.translated(scrollContainer.getX(), scrollContainer.getY());

                g.setColour(GalleryColors::get(GalleryColors::xoGold).withAlpha(0.9f));
                g.drawRoundedRectangle(chipInStrip.toFloat().expanded(2.0f), 4.0f, 2.0f);
            }
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4, 4);
        countLabel.setBounds(area.removeFromRight(kCountW));
        scrollContainer.setBounds(area);
        rebuildChips();
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (grainPaths.isEmpty()) return false;

        if (key == juce::KeyPress::leftKey)
        {
            focusedChipIndex = juce::jmax(0, focusedChipIndex - 1);
            repaint();
            return true;
        }
        if (key == juce::KeyPress::rightKey)
        {
            focusedChipIndex = juce::jmin(grainPaths.size() - 1, focusedChipIndex + 1);
            repaint();
            return true;
        }
        if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        {
            if (focusedChipIndex >= 0 && focusedChipIndex < grainPaths.size())
            {
                removeGrain(focusedChipIndex);
                focusedChipIndex = juce::jmin(focusedChipIndex, grainPaths.size() - 1);
            }
            return true;
        }
        return false;
    }

private:
    void rebuildChips()
    {
        chipHolder.removeAllChildren();
        chips.clear();

        int x = kChipGap;
        for (int i = 0; i < grainPaths.size(); ++i)
        {
            juce::String shortName = juce::File(grainPaths[i]).getFileNameWithoutExtension();
            if (shortName.length() > 20)
                shortName = shortName.substring(0, 18) + juce::String(juce::CharPointer_UTF8("\xe2\x80\xa6"));

            auto* chip = new juce::TextButton(shortName + "  " + juce::String(juce::CharPointer_UTF8("\xc3\x97")));
            chip->setSize(juce::roundToInt(GalleryFonts::body(12.0f).getStringWidthFloat(shortName)) + 48, kChipH);
            chip->setTopLeftPosition(x, (getHeight() - kChipH) / 2);
            chip->setColour(juce::TextButton::buttonColourId,
                            GalleryColors::get(GalleryColors::borderGray()));
            chip->setColour(juce::TextButton::textColourOffId,
                            GalleryColors::get(GalleryColors::textDark()));

            int capturedIndex = i;
            chip->onClick = [this, capturedIndex]() { removeGrain(capturedIndex); };

            A11y::setup(*chip,
                        "Remove " + juce::File(grainPaths[i]).getFileName(),
                        "Remove this grain from the strip");

            chipHolder.addAndMakeVisible(chip);
            chips.add(chip);
            x += chip->getWidth() + kChipGap;
        }

        chipHolder.setSize(juce::jmax(x, scrollContainer.getWidth()), getHeight());

        countLabel.setText(juce::String(grainPaths.size()) + " grain" +
                           (grainPaths.size() == 1 ? "" : "s"),
                           juce::dontSendNotification);
        repaint();
    }

    void notifyChanged()
    {
        if (onGrainsChanged)
            onGrainsChanged(grainPaths);
    }

    juce::StringArray          grainPaths;
    juce::Viewport             scrollContainer;
    juce::Component            chipHolder;
    juce::OwnedArray<juce::TextButton> chips;
    juce::Label                countLabel;
    int                        focusedChipIndex    { -1 };
    int                        highlightedChipIndex { -1 };  // set by highlightGrain(), cleared after 800ms

    static constexpr int kChipH    = 28;
    static constexpr int kChipGap  = 4;
    static constexpr int kChipPadX = 8;
    static constexpr int kCountW   = 72;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineGrainStrip)
};

} // namespace xoceanus
