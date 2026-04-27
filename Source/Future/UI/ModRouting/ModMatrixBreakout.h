// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
// Wave 5 A3 — Mod-matrix slide-up breakout strip + panel.
//
// Two components are defined here:
//
//   ModMatrixStrip  — compact 28 px horizontal strip that lives in the
//                     editor footer (mirrors D6 sequencer strip / D7 chord
//                     strip pattern from ChordBarComponent.h).
//                     Shows:  [ MOD  ◆ N routes  active indicator  ▲ ]
//                     Clicking anywhere opens / closes the slide-up panel.
//
//   ModMatrixPanel  — full-height overlay panel (~60 % of editor height)
//                     that slides up from the bottom when the strip is
//                     clicked.  Contains:
//                       • ModMatrixDrawer  (per-engine 8-slot APVTS matrix)
//                       • DragDropModRouter's ModRouteListPanel (global routes)
//                       • Full source-handle strip (drag targets)
//
// Slide animation:
//   A juce::Timer at 60 Hz drives a simple spring animation
//   (target_y + (current_y − target_y) * decayFactor).  No per-frame alloc.
//
// Wave 5 A3 mount APPLIED — see XOceanusEditor.h for wiring details.

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DragDropModRouter.h"
#include "../../../UI/Gallery/ModMatrixDrawer.h"
#include "UI/GalleryColors.h"

namespace xoceanus
{

//==============================================================================
// ModMatrixPanel — the slide-up editor overlay.
//
// Not added as a regular child of ModMatrixStrip — it is added to the editor
// root so it can float above all other UI. ModMatrixStrip calls
// setEditorBounds() so the panel can position itself correctly.
//
class ModMatrixPanel : public juce::Component, public juce::ChangeListener, private juce::Timer
{
public:
    // Panel occupies 60 % of editor height when open.
    static constexpr float kPanelHeightFraction = 0.60f;

    // Animation speed: fraction of distance to cover per frame at 60 Hz.
    static constexpr float kSpringDecay = 0.72f;

    explicit ModMatrixPanel(juce::AudioProcessorValueTreeState& apvts,
                             ModRoutingModel& modModel,
                             DragDropModRouter& router)
        : apvts_(apvts)
        , modModel_(modModel)
        , router_(router)
        , drawer_(apvts)
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, true);

        // ── Background ───────────────────────────────────────────────────
        // Panel is not opaque — paint() draws a semi-opaque glass slab.

        // ── ModMatrixDrawer ───────────────────────────────────────────────
        addAndMakeVisible(drawer_);

        // ── Route list panel (shared route model) ─────────────────────────
        routeList_.setModel(&modModel_);
        addAndMakeVisible(routeList_);

        // ── Source handle strip ───────────────────────────────────────────
        // Provides drag sources inside the panel for users who haven't
        // discovered the drag-from-strip-at-top workflow.
        for (int i = 0; i < static_cast<int>(ModSourceId::Count); ++i)
        {
            auto h = std::make_unique<ModSourceHandle>(static_cast<ModSourceId>(i));
            addAndMakeVisible(*h);
            handles_.push_back(std::move(h));
        }

        // ── Close button ─────────────────────────────────────────────────
        closeButton_.setButtonText("CLOSE");
        closeButton_.setColour(juce::TextButton::buttonColourId,
                               juce::Colour(200, 204, 216).withAlpha(0.04f));
        closeButton_.setColour(juce::TextButton::textColourOffId,
                               juce::Colour(200, 204, 216).withAlpha(0.55f));
        closeButton_.onClick = [this]() { close(); };
        addAndMakeVisible(closeButton_);

        modModel_.addListener(this);

        A11y::setup(*this, "Mod Matrix Panel",
                    "Slide-up modulation matrix editor. Press Escape to close.");

        setVisible(false);
    }

    ~ModMatrixPanel() override
    {
        stopTimer();
        modModel_.removeListener(this);
    }

    //==========================================================================
    // Load engine parameters — forward to the embedded drawer.
    void loadEngine(const juce::String& paramPrefix)
    {
        drawer_.clear();
        drawer_.loadEngine(paramPrefix);
    }

    //==========================================================================
    // Editor bounds — called by ModMatrixStrip::resized() so we know where
    // to position the panel within the editor root.
    void setEditorBounds(juce::Rectangle<int> editorBounds)
    {
        editorBounds_ = editorBounds;
        // Reposition immediately if already visible
        if (isVisible())
            applyCurrentPosition(/* animate = */false);
    }

    //==========================================================================
    // Open / close.
    void open()
    {
        if (isOpen_)
            return;
        isOpen_ = true;

        computeTargetBounds();

        // Start offscreen below the editor and animate upward.
        panelY_ = static_cast<float>(editorBounds_.getBottom());
        setBounds(computeFullBounds().withY(static_cast<int>(panelY_)));
        setVisible(true);
        toFront(false);
        startTimerHz(60);
    }

    void close()
    {
        if (!isOpen_)
            return;
        isOpen_ = false;
        startTimerHz(60); // animate back out
    }

    bool isOpen() const noexcept { return isOpen_; }

    void toggle() { isOpen_ ? close() : open(); }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        // Glass slab background
        g.setColour(juce::Colour(0xFF0A0E18).withAlpha(0.96f));
        g.fillRoundedRectangle(b.withTrimmedBottom(0.f), 10.f);

        // Top edge glowing border line
        g.setColour(juce::Colour(0xFF7FDBCA).withAlpha(0.22f));
        g.fillRect(b.getX() + 10.f, b.getY(), b.getWidth() - 20.f, 1.5f);

        // Title
        g.setFont(GalleryFonts::heading(11.0f));
        g.setColour(juce::Colour(GalleryColors::t1()).withAlpha(0.60f));
        g.drawText("MOD MATRIX",
                   static_cast<int>(b.getX()) + 18,
                   static_cast<int>(b.getY()),
                   160,
                   kTitleBarH,
                   juce::Justification::centredLeft, false);

        // Route count badge
        int count = modModel_.getRouteCount();
        if (count > 0)
        {
            juce::String badge = juce::String(count) + (count == 1 ? " route" : " routes");
            g.setFont(GalleryFonts::label(8.5f));
            g.setColour(juce::Colour(GalleryColors::t2()));
            g.drawText(badge,
                       static_cast<int>(b.getX()) + 18 + 104,
                       static_cast<int>(b.getY()),
                       120,
                       kTitleBarH,
                       juce::Justification::centredLeft, false);
        }

        // Separator under title bar
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.05f));
        g.fillRect(b.getX(), b.getY() + static_cast<float>(kTitleBarH),
                   b.getWidth(), 1.0f);

        // Source strip label
        g.setFont(GalleryFonts::label(7.5f));
        g.setColour(juce::Colour(GalleryColors::t3()));
        g.drawText("DRAG SOURCE →", 8, kTitleBarH + kBodyH - kHandleStripH, 100, kHandleStripH,
                   juce::Justification::centredLeft, false);

        // Separator above source strip
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.04f));
        g.fillRect(b.getX(), b.getBottom() - static_cast<float>(kHandleStripH + 1),
                   b.getWidth(), 1.0f);
    }

    void resized() override
    {
        auto b = getLocalBounds();

        // Title bar region
        auto titleArea = b.removeFromTop(kTitleBarH);

        // Close button (right side of title bar)
        closeButton_.setBounds(titleArea.removeFromRight(60).reduced(4, 5));

        // Content body
        auto body = b.removeFromTop(kBodyH);

        // Left half: ModMatrixDrawer (APVTS slots)
        int halfW = body.getWidth() / 2;
        drawer_.setBounds(body.removeFromLeft(halfW).reduced(4, 4));

        // Right half: global route list
        routeList_.setBounds(body.reduced(4, 4));

        // Source handle strip at the bottom
        auto handleArea = b.removeFromTop(kHandleStripH);
        const int n = static_cast<int>(handles_.size());
        if (n > 0)
        {
            const int hW = ModSourceHandle::kDiameter;
            const int gap = 8;
            const int totalW = n * hW + (n - 1) * gap;
            int xOff = handleArea.getX() + 110; // offset past the label

            for (int i = 0; i < n; ++i)
            {
                int hy = handleArea.getCentreY() - hW / 2;
                handles_[static_cast<size_t>(i)]->setBounds(xOff + i * (hW + gap), hy, hW, hW);
            }
        }
    }

    //==========================================================================
    // Key press: Escape closes the panel.
    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            close();
            return true;
        }
        return false;
    }

    // Click outside the panel bounds closes it.
    void mouseDown(const juce::MouseEvent& e) override
    {
        // If the click is in the glass area (not a child), do nothing special.
        juce::ignoreUnused(e);
    }

    //==========================================================================
    // ChangeListener — model changed, repaint the badge.
    void changeListenerCallback(juce::ChangeBroadcaster*) override { repaint(); }

private:
    //==========================================================================
    static constexpr int kTitleBarH    = 30;
    static constexpr int kHandleStripH = 32;
    static constexpr int kBodyH        = 220; // drawer + list body height

    //==========================================================================
    // Compute the full panel bounds within the editor coordinate space.
    juce::Rectangle<int> computeFullBounds() const
    {
        const int panelH = juce::roundToInt(
            static_cast<float>(editorBounds_.getHeight()) * kPanelHeightFraction);
        return { editorBounds_.getX(),
                 editorBounds_.getBottom() - panelH,
                 editorBounds_.getWidth(),
                 panelH };
    }

    void computeTargetBounds()
    {
        targetBounds_ = computeFullBounds();
    }

    void applyCurrentPosition(bool animate = true)
    {
        if (!animate)
        {
            panelY_ = static_cast<float>(isOpen_ ? targetBounds_.getY()
                                                  : editorBounds_.getBottom());
            setBounds(targetBounds_.withY(static_cast<int>(panelY_)));
            if (!isOpen_)
                setVisible(false);
            return;
        }

        // Spring step
        float targetY = static_cast<float>(isOpen_ ? targetBounds_.getY()
                                                    : editorBounds_.getBottom());
        panelY_ += (targetY - panelY_) * (1.0f - kSpringDecay);

        setBounds(targetBounds_.withY(static_cast<int>(panelY_)));
    }

    //==========================================================================
    void timerCallback() override
    {
        computeTargetBounds();
        applyCurrentPosition(/* animate = */true);

        float targetY = static_cast<float>(isOpen_ ? targetBounds_.getY()
                                                    : editorBounds_.getBottom());
        const float remaining = std::abs(panelY_ - targetY);

        if (remaining < 0.8f)
        {
            panelY_ = targetY;
            setBounds(targetBounds_.withY(static_cast<int>(panelY_)));
            stopTimer();

            if (!isOpen_)
                setVisible(false);
        }
    }

    //==========================================================================
    juce::AudioProcessorValueTreeState& apvts_;
    ModRoutingModel&                    modModel_;
    [[maybe_unused]] DragDropModRouter& router_;

    ModMatrixDrawer  drawer_;
    ModRouteListPanel routeList_;
    std::vector<std::unique_ptr<ModSourceHandle>> handles_;
    juce::TextButton closeButton_;

    juce::Rectangle<int> editorBounds_{};
    juce::Rectangle<int> targetBounds_{};

    bool  isOpen_{false};
    float panelY_{0.f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrixPanel)
};

//==============================================================================
// ModMatrixStrip — compact 28 px footer strip.
//
// Visual layout (left → right):
//   [8px] [◆ dot — active indicator] [MOD MATRIX label] [route count badge]
//   [flex gap] [active-source chips] [▲ caret]
//
class ModMatrixStrip : public juce::Component, public juce::ChangeListener
{
public:
    static constexpr int kStripHeight = 28;

    explicit ModMatrixStrip(juce::AudioProcessorValueTreeState& apvts,
                             ModRoutingModel& modModel,
                             DragDropModRouter& router)
        : modModel_(modModel)
        , panel_(apvts, modModel, router)
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, false);

        modModel_.addListener(this);

        A11y::setup(*this, "Mod Matrix Strip",
                    "Click to open the modulation matrix editor. "
                    + juce::String(modModel_.getRouteCount())
                    + " active routes.",
                    /* wantsKeyFocus = */true);
    }

    ~ModMatrixStrip() override
    {
        modModel_.removeListener(this);
    }

    //==========================================================================
    // loadEngine — called when active engine changes.
    void loadEngine(const juce::String& paramPrefix)
    {
        panel_.loadEngine(paramPrefix);
    }

    //==========================================================================
    // setEditorBounds — must be called from XOceanusEditor::resized() so the
    // panel can correctly place itself as a floating overlay.
    // The panel must be added to the editor root, not to the strip — pass the
    // editor's root component via addPanelToParent() once during construction.
    void setEditorBounds(juce::Rectangle<int> editorBounds)
    {
        editorBounds_ = editorBounds;
        panel_.setEditorBounds(editorBounds);
    }

    // addPanelToParent — add the slide-up panel to the editor root component
    // so it floats above all other UI.  Call once during editor construction
    // after addAndMakeVisible(*modMatrixStrip_).
    void addPanelToParent(juce::Component& editorRoot)
    {
        editorRoot.addChildComponent(panel_);
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        const float midY = h * 0.5f;

        const bool panelOpen = panel_.isOpen();
        const int  routeCount = modModel_.getRouteCount();
        const bool hasRoutes = (routeCount > 0);

        // Background
        g.setColour(juce::Colour(0xFF0A0E18));
        g.fillRect(0.f, 0.f, w, h);

        // Top border line — accent when panel is open
        const float borderAlpha = panelOpen ? 0.35f : 0.10f;
        g.setColour(juce::Colour(0xFF7FDBCA).withAlpha(borderAlpha));
        g.fillRect(0.f, 0.f, w, 1.5f);

        // ── Active indicator dot ─────────────────────────────────────────
        const float dotR = 4.5f;
        const float dotX = 12.f;
        const float dotY = midY;

        if (hasRoutes)
        {
            // Outer glow
            g.setColour(juce::Colour(0xFF7FDBCA).withAlpha(0.15f));
            g.fillEllipse(dotX - dotR - 2.f, dotY - dotR - 2.f,
                          (dotR + 2.f) * 2.f, (dotR + 2.f) * 2.f);
            // Core
            g.setColour(juce::Colour(0xFF7FDBCA).withAlpha(0.85f));
            g.fillEllipse(dotX - dotR, dotY - dotR, dotR * 2.f, dotR * 2.f);
        }
        else
        {
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.15f));
            g.drawEllipse(dotX - dotR, dotY - dotR, dotR * 2.f, dotR * 2.f, 1.f);
        }

        // ── "MOD MATRIX" label ────────────────────────────────────────────
        g.setFont(GalleryFonts::heading(9.5f));
        g.setColour(juce::Colour(200, 204, 216).withAlpha(panelOpen ? 0.85f : 0.50f));
        g.drawText("MOD MATRIX",
                   26, 0, 90, static_cast<int>(h),
                   juce::Justification::centredLeft, false);

        // ── Route count badge ─────────────────────────────────────────────
        if (hasRoutes)
        {
            juce::String badge = juce::String(routeCount);
            const float badgeX = 120.f;
            const float badgeW = 18.f;
            const float badgeH = 13.f;
            const float badgeY = midY - badgeH * 0.5f;

            g.setColour(juce::Colour(0xFF7FDBCA).withAlpha(0.15f));
            g.fillRoundedRectangle(badgeX, badgeY, badgeW, badgeH, 4.f);
            g.setColour(juce::Colour(0xFF7FDBCA).withAlpha(0.75f));
            g.drawRoundedRectangle(badgeX, badgeY, badgeW, badgeH, 4.f, 1.f);

            g.setFont(GalleryFonts::value(8.0f));
            g.setColour(juce::Colour(0xFF7FDBCA));
            g.drawText(badge,
                       static_cast<int>(badgeX),
                       static_cast<int>(badgeY),
                       static_cast<int>(badgeW),
                       static_cast<int>(badgeH),
                       juce::Justification::centred, false);
        }

        // ── Per-source activity chips (mini dots in source colors) ────────
        // Show one tiny colored dot per unique source that has at least one
        // active route.  Positioned in the center section.
        {
            auto routes = modModel_.getRoutesCopy();
            // Collect unique source IDs (in order of first appearance)
            std::vector<int> seenSources;
            for (const auto& r : routes)
            {
                bool found = false;
                for (int s : seenSources) if (s == r.sourceId) { found = true; break; }
                if (!found)
                    seenSources.push_back(r.sourceId);
            }

            const float chipR = 3.5f;
            float chipX = 148.f;
            for (int srcId : seenSources)
            {
                // Find colour
                juce::Colour chipCol(GalleryColors::xoGold);
                for (const auto& info : kAllModSourcesForStrip)
                    if (info.id == srcId) { chipCol = juce::Colour(info.colour); break; }

                g.setColour(chipCol.withAlpha(0.80f));
                g.fillEllipse(chipX - chipR, midY - chipR, chipR * 2.f, chipR * 2.f);
                chipX += chipR * 2.f + 3.f;

                if (chipX > w - 40.f)
                    break; // don't overflow
            }
        }

        // ── Caret ▲ / ▼ (right edge) ──────────────────────────────────────
        g.setFont(juce::Font(9.0f));
        g.setColour(juce::Colour(200, 204, 216).withAlpha(panelOpen ? 0.75f : 0.35f));
        g.drawText(panelOpen ? juce::CharPointer_UTF8("\xe2\x96\xbc")
                             : juce::CharPointer_UTF8("\xe2\x96\xb2"),
                   static_cast<int>(w) - 20, 0, 16, static_cast<int>(h),
                   juce::Justification::centred, false);

        // Hover highlight
        if (isMouseOver())
        {
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.03f));
            g.fillRect(0.f, 0.f, w, h);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isLeftButtonDown())
        {
            panel_.toggle();
            repaint();
        }
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&)  override { repaint(); }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::spaceKey || key.getKeyCode() == juce::KeyPress::returnKey)
        {
            panel_.toggle();
            repaint();
            return true;
        }
        return false;
    }

    //==========================================================================
    // ChangeListener — repaint when route count changes
    void changeListenerCallback(juce::ChangeBroadcaster*) override { repaint(); }

private:
    // Small mirror of the source colour table used for the strip chips.
    // We can't reference ModulateFromMenu.h here to avoid a circular dep —
    // duplicate the minimal data we need.
    struct SourceColorEntry { int id; uint32_t colour; };
    static constexpr SourceColorEntry kAllModSourcesForStrip[] = {
        { 0,  0xFF00CED1 }, // LFO1
        { 1,  0xFFA8D8EA }, // LFO2
        { 6,  0xFF7EC8E3 }, // LFO3
        { 2,  0xFFE8701A }, // Envelope
        { 7,  0xFFFFAA55 }, // ENV2
        { 8,  0xFFE9C46A }, // TONE
        { 9,  0xFF7FDBCA }, // TIDE
        { 10, 0xFFFF8A65 }, // COUPLE
        { 11, 0xFF9B89D4 }, // DEPTH
        { 3,  0xFFC6E377 }, // Velocity
        { 4,  0xFFFF8A7A }, // Aftertouch
        { 5,  0xFF4169E1 }, // ModWheel
        { 12, 0xFF9898D0 }, // MIDI CC
        { 13, 0xFFFFD54F }, // MPE Pressure
        { 14, 0xFFFF7043 }, // MPE Slide
        { 15, 0xFF81D4FA }, // Seq Step
        { 16, 0xFFF48FB1 }, // Chord Tone
        { 17, 0xFF80CBC4 }, // Beat Phase
    };

    //==========================================================================
    ModRoutingModel& modModel_;
    ModMatrixPanel   panel_;
    juce::Rectangle<int> editorBounds_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrixStrip)
};

} // namespace xoceanus
