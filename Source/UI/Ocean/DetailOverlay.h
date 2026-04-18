// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// DetailOverlay.h — Floating detail panel overlay for the Ocean View.
//
// Wraps EngineDetailPanel in a floating overlay with backdrop dimming,
// rounded container, and close button. Positioned inset from ocean edges.
//
// Design doc: Docs/design/ocean-ui-phase1-design.md Step 4
// Prototype: Tools/ui-preview/submarine.html (detail-panel class)

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../Gallery/EngineDetailPanel.h"

namespace xoceanus
{

class DetailOverlay : public juce::Component
{
public:
    DetailOverlay()
    {
        setInterceptsMouseClicks(true, true);
        setWantsKeyboardFocus(true);

        // Backdrop — semi-transparent, click to close
        backdrop_.setInterceptsMouseClicks(true, false);
        backdrop_.onClick = [this]() { hide(); };
        addAndMakeVisible(backdrop_);

        // Close button
        closeBtn_.setButtonText(juce::String::charToString(0x2715)); // ✕
        closeBtn_.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        closeBtn_.setColour(juce::TextButton::textColourOffId,
                           juce::Colour(200, 204, 216).withAlpha(0.5f));
        closeBtn_.onClick = [this]() { hide(); };
        addAndMakeVisible(closeBtn_);
    }

    void show(int slot, EngineDetailPanel* detailPanel)
    {
        if (!detailPanel) return;
        currentSlot_ = slot;
        detailPanel_ = detailPanel;

        // DON'T re-parent the detail panel — keep it in OceanView's child list
        // to preserve APVTS connections and child component state.
        // Instead, we just position it over our panel bounds and make it visible.
        setVisible(true);
        resized();   // positions backdrop, close button, and computes panel bounds

        // Position the detail panel (still a child of our parent, OceanView)
        // at absolute coordinates matching our panel area.
        auto panelRect = getPanelBounds();
        // Convert from our local coords to parent (OceanView) coords
        auto absPanel = panelRect.translated(getBounds().getX(), getBounds().getY());
        auto finalBounds = absPanel.reduced(4, 4).withTrimmedTop(8);
        detailPanel_->setBounds(finalBounds);
        detailPanel_->setVisible(true);
        bool loaded = detailPanel_->loadSlot(slot);
        detailPanel_->resized();
        detailPanel_->toFront(false);

        DBG("DetailOverlay::show slot=" + juce::String(slot)
            + " loaded=" + juce::String(loaded ? "YES" : "NO")
            + " panelBounds=" + finalBounds.toString()
            + " overlayBounds=" + getBounds().toString()
            + " panelParent=" + juce::String(detailPanel_->getParentComponent() ? detailPanel_->getParentComponent()->getName() : "NULL"));

        grabKeyboardFocus();

        if (onShown) onShown();
    }

    void hide()
    {
        if (detailPanel_)
            detailPanel_->setVisible(false);

        setVisible(false);
        currentSlot_ = -1;

        if (onHidden) onHidden();
    }

    bool isShowing() const noexcept { return isVisible() && currentSlot_ >= 0; }
    int getCurrentSlot() const noexcept { return currentSlot_; }

    void paint(juce::Graphics& g) override
    {
        // Panel background with rounded corners
        auto panelBounds = getPanelBounds().toFloat();

        // Panel shadow
        juce::DropShadow shadow(juce::Colours::black.withAlpha(0.6f), 24, { 0, 8 });
        shadow.drawForRectangle(g, panelBounds.toNearestInt());

        // Panel body
        g.setColour(juce::Colour(20, 23, 32).withAlpha(0.97f));
        g.fillRoundedRectangle(panelBounds, 16.0f);

        // Border
        g.setColour(juce::Colour(60, 180, 170).withAlpha(0.14f));
        g.drawRoundedRectangle(panelBounds, 16.0f, 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Backdrop fills everything
        backdrop_.setBounds(bounds);

        // Panel inset from edges (matching prototype: top:52, left:50, right:50, bottom:16)
        auto panel = getPanelBounds();

        // Close button top-right of panel
        closeBtn_.setBounds(panel.getRight() - 40, panel.getY() + 8, 32, 32);

        // Detail panel fills the panel interior with padding
        if (detailPanel_)
        {
            detailPanel_->setBounds(panel.reduced(4, 4).withTrimmedTop(8));
            detailPanel_->setVisible(true);
        }
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            hide();
            return true;
        }
        return false;
    }

    // Callbacks
    std::function<void()> onShown;
    std::function<void()> onHidden;

private:
    juce::Rectangle<int> getPanelBounds() const
    {
        auto bounds = getLocalBounds();
        // Inset: top 52px, left/right 50px, bottom 16px
        return bounds.withTrimmedTop(52)
                     .withTrimmedLeft(50)
                     .withTrimmedRight(50)
                     .withTrimmedBottom(16);
    }

    // Simple backdrop component that paints a semi-transparent fill
    struct Backdrop : public juce::Component
    {
        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colours::black.withAlpha(0.35f));
        }
        std::function<void()> onClick;
        void mouseDown(const juce::MouseEvent&) override
        {
            if (onClick) onClick();
        }
    };

    Backdrop backdrop_;
    juce::TextButton closeBtn_;
    EngineDetailPanel* detailPanel_ = nullptr; // non-owning — owned by OceanView
    int currentSlot_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DetailOverlay)
};

} // namespace xoceanus
