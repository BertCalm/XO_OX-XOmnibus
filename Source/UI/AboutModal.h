// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// AboutModal.h — D12: About/Lore modal opened by clicking the "O" brand badge.
//
// The modal is a child-component overlay drawn directly inside XOceanusEditor.
// It has two tabs: About (default) and Lore.
//
// Usage:
//   // Declare as member in the editor:
//   xoceanus::AboutModal aboutModal_;
//
//   // Add as child (invisible by default):
//   addChildComponent(aboutModal_);
//   aboutModal_.setAlwaysOnTop(true);
//
//   // In resized(), give it centered bounds or full-window bounds:
//   aboutModal_.setBounds(getLocalBounds());
//
//   // Open from the O-badge click:
//   aboutModal_.openTab(xoceanus::AboutModal::Tab::About);
//
//   // Long-press → open with Lore tab (see OBadgeButton below):
//   aboutModal_.openTab(xoceanus::AboutModal::Tab::Lore);
//
// The modal dismisses itself on:
//   - clicking outside the card area
//   - pressing Escape
//   - clicking the × close button
//
// File is header-only (XOceanus UI convention).

#include <juce_gui_basics/juce_gui_basics.h>
#include "GalleryColors.h"
#include <functional>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

//==============================================================================
/**
    AboutModal

    Frosted-glass overlay card with About and Lore tabs.
    Opened by clicking (or long-pressing) the "O" brand badge.
    D12 spec: About tab shows version + tagline + credits.
             Lore tab shows scrollable engine mythology stub.
*/
class AboutModal : public juce::Component,
                   public juce::KeyListener
{
public:
    //==========================================================================
    enum class Tab { About, Lore };

    //==========================================================================
    AboutModal()
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, true);
        setVisible(false);

        // Build version string once (same pattern as SettingsPanel.h).
        {
#if defined(JucePlugin_VersionString)
            versionStr_ = juce::String(JucePlugin_VersionString);
#else
            versionStr_ = "v0.x-dev";
#endif
        }

        // Lore viewport + content
        loreContent_.setOpaque(false);
        loreContent_.setInterceptsMouseClicks(false, false);
        loreViewport_.setScrollBarsShown(true, false);
        loreViewport_.setViewedComponent(&loreContent_, false);
        loreViewport_.setOpaque(false);
        loreViewport_.getVerticalScrollBar().setColour(
            juce::ScrollBar::thumbColourId,
            juce::Colour(200, 204, 216).withAlpha(0.25f));

        addChildComponent(loreViewport_);
    }

    ~AboutModal() override = default;

    //==========================================================================
    /** Open the modal, showing the specified tab. */
    void openTab(Tab tab)
    {
        activeTab_ = tab;
        setVisible(true);
        toFront(true);
        repaint();
        // Register as key listener on the top-level component so Escape works.
        if (auto* top = getTopLevelComponent())
            top->addKeyListener(this);
    }

    /** Close the modal. */
    void close()
    {
        setVisible(false);
        if (auto* top = getTopLevelComponent())
            top->removeKeyListener(this);
    }

    bool isShowing() const noexcept { return isVisible(); }

    //==========================================================================
    // Bring juce::Component::keyPressed(key) into scope so the 1-arg Component
    // virtual is not hidden by our 2-arg KeyListener override below.
    using juce::Component::keyPressed;

    // juce::KeyListener — D12: Escape closes the modal.
    // Must match KeyListener::keyPressed (2-arg pure virtual); the 1-arg
    // Component::keyPressed is re-exposed via the using declaration above.
    bool keyPressed(const juce::KeyPress& key,
                    juce::Component* /*originatingComponent*/) override
    {
        if (isVisible() && key == juce::KeyPress::escapeKey)
        {
            close();
            return true;
        }
        return false;
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        if (!isVisible())
            return;

        using juce::Colour;

        const auto bounds  = getLocalBounds().toFloat();
        auto       cardR   = getCardBounds();

        // ── Dim overlay behind the card ──────────────────────────────────────
        g.setColour(Colour(0, 0, 0).withAlpha(0.60f));
        g.fillRect(bounds);

        // ── Card background ──────────────────────────────────────────────────
        g.setColour(Colour(14, 16, 22).withAlpha(0.96f));
        g.fillRoundedRectangle(cardR, 10.0f);

        // Card border
        g.setColour(Colour(200, 204, 216).withAlpha(0.12f));
        g.drawRoundedRectangle(cardR, 10.0f, 1.0f);

        // ── Title bar ("XOceanus" + version) ────────────────────────────────
        const float titleH = 48.0f;
        auto        titleR = cardR.removeFromTop(titleH);

        // Title bar teal accent line at top of card
        g.setColour(Colour(60, 180, 170).withAlpha(0.70f));
        g.fillRect(cardR.getX() + 1.0f, cardR.getY() - titleH,
                   cardR.getWidth() - 2.0f, 2.0f);

        const juce::Font nameFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(16.0f));

        const juce::Font verFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultMonospacedFontName())
            .withHeight(10.0f));

        g.setFont(nameFont);
        g.setColour(Colour(200, 204, 216).withAlpha(0.90f));
        auto nameRect = titleR.reduced(16.0f, 0.0f);
        g.drawText("XOceanus", nameRect.toNearestInt(),
                   juce::Justification::centredLeft, false);

        // Version alongside the name
        const float nameW = nameFont.getStringWidthFloat("XOceanus") + 8.0f;
        g.setFont(verFont);
        g.setColour(Colour(127, 219, 202).withAlpha(0.70f));
        g.drawText(versionStr_, juce::Rectangle<float>(
                       titleR.getX() + 16.0f + nameW + 6.0f,
                       titleR.getY(),
                       140.0f,
                       titleH).toNearestInt(),
                   juce::Justification::centredLeft, false);

        // Close button "×"  — top-right of title bar
        const float closeSz = 20.0f;
        closeBtnBounds_ = juce::Rectangle<float>(
            titleR.getRight() - closeSz - 10.0f,
            titleR.getCentreY() - closeSz * 0.5f,
            closeSz, closeSz);
        const bool closeHov = closeBtnBounds_.contains(lastMousePt_);
        g.setColour(closeHov ? Colour(200, 204, 216).withAlpha(0.80f)
                             : Colour(200, 204, 216).withAlpha(0.40f));
        const juce::Font closeFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withHeight(14.0f));
        g.setFont(closeFont);
        g.drawText("\xc3\x97", closeBtnBounds_.toNearestInt(),  // UTF-8 "×"
                   juce::Justification::centred, false);

        // ── Tab bar ──────────────────────────────────────────────────────────
        const auto fullCard = getCardBounds();
        const float tabH    = 32.0f;
        const float tabY    = fullCard.getY() + titleH;
        const float tabW    = (fullCard.getWidth() - 2.0f) * 0.5f;

        const juce::Font tabFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(9.0f));

        for (int t = 0; t < 2; ++t)
        {
            const bool isActive = (static_cast<int>(activeTab_) == t);
            tabBounds_[t] = juce::Rectangle<float>(
                fullCard.getX() + 1.0f + static_cast<float>(t) * tabW,
                tabY,
                tabW,
                tabH);

            const juce::Colour tabBg = isActive
                ? Colour(60, 180, 170).withAlpha(0.10f)
                : Colour(200, 204, 216).withAlpha(0.04f);
            const juce::Colour tabText = isActive
                ? Colour(127, 219, 202).withAlpha(0.90f)
                : Colour(200, 204, 216).withAlpha(0.45f);

            g.setColour(tabBg);
            g.fillRect(tabBounds_[t]);

            // Active tab indicator line
            if (isActive)
            {
                g.setColour(Colour(127, 219, 202).withAlpha(0.60f));
                g.fillRect(tabBounds_[t].getX(), tabBounds_[t].getBottom() - 2.0f,
                           tabBounds_[t].getWidth(), 2.0f);
            }

            g.setFont(tabFont);
            g.setColour(tabText);
            static const char* kTabLabels[2] = { "ABOUT", "LORE" };
            g.drawText(kTabLabels[t], tabBounds_[t].toNearestInt(),
                       juce::Justification::centred, false);
        }

        // Divider below tabs
        g.setColour(Colour(200, 204, 216).withAlpha(0.08f));
        g.fillRect(fullCard.getX() + 1.0f, tabY + tabH, fullCard.getWidth() - 2.0f, 1.0f);

        // ── Tab content ──────────────────────────────────────────────────────
        const float contentY = tabY + tabH + 1.0f;
        const float contentH = fullCard.getBottom() - contentY - 1.0f;
        const auto contentRect = juce::Rectangle<float>(
            fullCard.getX() + 1.0f, contentY,
            fullCard.getWidth() - 2.0f, contentH);

        if (activeTab_ == Tab::About)
            paintAboutTab(g, contentRect);
        // Lore tab is painted by the loreViewport_ child component.
    }

    void resized() override
    {
        const auto cardR   = getCardBounds();
        const float titleH = 48.0f;
        const float tabH   = 32.0f;
        const float contentY = cardR.getY() + titleH + tabH + 1.0f;
        const float contentH = cardR.getBottom() - contentY - 8.0f;

        const auto contentRect = juce::Rectangle<float>(
            cardR.getX() + 1.0f, contentY,
            cardR.getWidth() - 2.0f, contentH);

        // Only show viewport for Lore tab; reposition always so switching tabs
        // doesn't require another resize pass.
        loreViewport_.setBounds(contentRect.toNearestInt());
        loreContent_.setSize(loreViewport_.getWidth() - loreViewport_.getScrollBarThickness(),
                             1400);  // content height; scrollable (expanded for mythology excerpts)
        loreViewport_.setVisible(activeTab_ == Tab::Lore);
    }

    //==========================================================================
    void mouseDown(const juce::MouseEvent& e) override
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        // Click outside the card → close.
        if (!getCardBounds().contains(mx, my))
        {
            close();
            return;
        }

        // Close button
        if (closeBtnBounds_.contains(mx, my))
        {
            close();
            return;
        }

        // Tab switching
        for (int t = 0; t < 2; ++t)
        {
            if (tabBounds_[t].contains(mx, my))
            {
                activeTab_ = static_cast<Tab>(t);
                loreViewport_.setVisible(activeTab_ == Tab::Lore);
                repaint();
                return;
            }
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        lastMousePt_ = juce::Point<float>(static_cast<float>(e.x),
                                          static_cast<float>(e.y));
        repaint();
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        lastMousePt_ = { -1.0f, -1.0f };
        repaint();
    }

private:
    //==========================================================================

    /** Returns the centered card rectangle within the parent bounds. */
    juce::Rectangle<float> getCardBounds() const
    {
        const float cardW = 420.0f;
        const float cardH = 340.0f;
        const float cx    = static_cast<float>(getWidth())  * 0.5f;
        const float cy    = static_cast<float>(getHeight()) * 0.5f;
        return { cx - cardW * 0.5f, cy - cardH * 0.5f, cardW, cardH };
    }

    //--------------------------------------------------------------------------
    void paintAboutTab(juce::Graphics& g, const juce::Rectangle<float>& r)
    {
        using juce::Colour;

        const float pad  = 20.0f;
        float y = r.getY() + pad;

        const juce::Font headFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(11.0f));

        const juce::Font bodyFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withHeight(10.0f));

        // Tagline
        g.setFont(headFont);
        g.setColour(Colour(127, 219, 202).withAlpha(0.85f));
        g.drawText("The Ocean of Synthesis",
                   juce::Rectangle<float>(r.getX() + pad, y, r.getWidth() - pad * 2.0f, 16.0f).toNearestInt(),
                   juce::Justification::centredLeft, false);
        y += 22.0f;

        // Version row
        g.setFont(bodyFont);
        g.setColour(Colour(200, 204, 216).withAlpha(0.50f));
        g.drawText("Version  " + versionStr_,
                   juce::Rectangle<float>(r.getX() + pad, y, r.getWidth() - pad * 2.0f, 14.0f).toNearestInt(),
                   juce::Justification::centredLeft, false);
        y += 18.0f;

        // Credits
        g.setColour(Colour(200, 204, 216).withAlpha(0.40f));
        g.drawText("XO_OX Designs  \xc2\xb7  2026",   // UTF-8 middle dot
                   juce::Rectangle<float>(r.getX() + pad, y, r.getWidth() - pad * 2.0f, 14.0f).toNearestInt(),
                   juce::Justification::centredLeft, false);
        y += 24.0f;

        // Divider
        g.setColour(Colour(200, 204, 216).withAlpha(0.07f));
        g.fillRect(r.getX() + pad, y, r.getWidth() - pad * 2.0f, 1.0f);
        y += 12.0f;

        // Engine count blurb
        g.setFont(bodyFont);
        g.setColour(Colour(200, 204, 216).withAlpha(0.45f));
        g.drawText("86 synthesis engines  \xc2\xb7  Kitchen Collection  \xc2\xb7  XPN expansion packs",
                   juce::Rectangle<float>(r.getX() + pad, y, r.getWidth() - pad * 2.0f, 14.0f).toNearestInt(),
                   juce::Justification::centredLeft, false);
        y += 20.0f;

        // URL hint
        g.setColour(Colour(127, 219, 202).withAlpha(0.55f));
        g.drawText("xo-ox.org  \xc2\xb7  Field Guide  \xc2\xb7  Patreon",
                   juce::Rectangle<float>(r.getX() + pad, y, r.getWidth() - pad * 2.0f, 14.0f).toNearestInt(),
                   juce::Justification::centredLeft, false);
    }

    //==========================================================================
    // State

    Tab    activeTab_  = Tab::About;
    juce::String versionStr_;

    // Cached hit regions (rebuilt each paint).
    juce::Rectangle<float> closeBtnBounds_;
    juce::Rectangle<float> tabBounds_[2];
    juce::Point<float>     lastMousePt_ { -1.0f, -1.0f };

    // Lore tab scrollable viewport — content is a custom component that paints the stub text.
    juce::Viewport  loreViewport_;

    struct LoreContent : public juce::Component
    {
        LoreContent() { setOpaque(false); setInterceptsMouseClicks(false, false); }

        void paint(juce::Graphics& g) override
        {
            using juce::Colour;

            const float pad  = 16.0f;
            float y = pad;
            const float w    = static_cast<float>(getWidth()) - pad * 2.0f;

            const juce::Font headFont(juce::FontOptions{}
                .withName(juce::Font::getDefaultSansSerifFontName())
                .withStyle("Bold")
                .withHeight(11.0f));

            const juce::Font bodyFont(juce::FontOptions{}
                .withName(juce::Font::getDefaultSansSerifFontName())
                .withHeight(10.0f));

            auto drawLine = [&](const juce::Font& f, juce::Colour col, const char* text, float lineH)
            {
                g.setFont(f);
                g.setColour(col);
                g.drawText(text,
                           juce::Rectangle<float>(pad, y, w, lineH).toNearestInt(),
                           juce::Justification::topLeft, true);
                y += lineH + 4.0f;
            };

            drawLine(headFont, Colour(127, 219, 202).withAlpha(0.85f),
                     "The XOceanus Aquarium", 14.0f);

            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "The XOceanus aquarium contains 86 engines, each a creature", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "of the deep. Sunlit zone holds bright, harmonic engines.", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "Twilight zone holds complex, evolving textures. Midnight zone", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "holds the dark, experimental, and alien sounds.", 12.0f);

            y += 8.0f;

            drawLine(headFont, Colour(127, 219, 202).withAlpha(0.70f),
                     "Field Guide", 14.0f);

            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.45f),
                     "Each engine has a mythology entry in the XO-OX Field Guide.", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.45f),
                     "Visit xo-ox.org for the full Field Guide (~52K words,", 12.0f);  // ~
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.45f),
                     "15 posts) and engine mythology for every creature.", 12.0f);

            y += 8.0f;

            drawLine(headFont, Colour(127, 219, 202).withAlpha(0.70f),
                     "Kitchen Collection", 14.0f);

            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.45f),
                     "Six quads of complementary engines curated for immediate play.", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.45f),
                     "Unlocks progressively via Patreon milestones.", 12.0f);

            y += 8.0f;

            // --- Engine Mythology Excerpts ---
            drawLine(headFont, Colour(127, 219, 202).withAlpha(0.70f),
                     "Engine Mythology", 14.0f);

            // OXYTOCIN
            drawLine(headFont, Colour(155, 93, 229).withAlpha(0.80f),
                     "OXYTOCIN  \xc2\xb7  Synapse Violet  \xc2\xb7  Fleet Leader", 11.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "Models three legendary circuits in one voice: the Steinhart-Hart NTC", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "thermal network, the Sallen-Key saturation stage, and a Serge circular", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "modulation topology. The longer you hold a note, the deeper the love type.", 12.0f);
            y += 4.0f;

            // OPERA
            drawLine(headFont, Colour(212, 175, 55).withAlpha(0.80f),
                     "OPERA  \xc2\xb7  Aria Gold  \xc2\xb7  Mesopelagic", 11.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "The Humpback Whale sings through the SOFAR channel. Eight voices", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "share a Kuramoto mean-field: coupled limit cycles from Kuramoto (1975),", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "weighted by Peterson-Barney formant data. Synchrony is a threshold event.", 12.0f);
            y += 4.0f;

            // OFFERING
            drawLine(headFont, Colour(229, 184, 11).withAlpha(0.80f),
                     "OFFERING  \xc2\xb7  Crate Wax Yellow  \xc2\xb7  Rubble Zone", 11.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "The Mantis Shrimp strikes. Eight drum slots, eight distinct synthesis", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "topologies: TR-808 metallic networks, Karplus-Strong comb, acoustic", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "contact physics, city-specific compressor chains. Detroit always runs late.", 12.0f);
            y += 4.0f;

            // OWARE
            drawLine(headFont, Colour(181, 136, 62).withAlpha(0.80f),
                     "OWARE  \xc2\xb7  Akan Goldweight  \xc2\xb7  Tuned Percussion", 11.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "Modal ratio tables from Rossing (2000). Material alpha decay exponents", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "from beam dispersion theory. Per-voice thermal drift approaches a new", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "tuning target every ~4 seconds. Each bar has its own stable character.", 12.0f);
            y += 4.0f;

            // OGIVE
            drawLine(headFont, Colour(155, 27, 48).withAlpha(0.80f),
                     "OGIVE  \xc2\xb7  Selenium Ruby  \xc2\xb7  Scanned Synthesis", 11.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "A spring-mass glass surface initialized from classic waveforms, scanned", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "by a read-head tracing a Gothic arch trajectory. Verlet integration drives", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "nonlinear cubic springs. Sound arrives through architecture.", 12.0f);
            y += 4.0f;

            // OLVIDO
            drawLine(headFont, Colour(59, 110, 143).withAlpha(0.80f),
                     "OLVIDO  \xc2\xb7  Coelacanth Blue  \xc2\xb7  The Forgetting Engine", 11.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "Six frequency bands dissolve at different rates, f\xc2\xb2-scaled per", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "Phillips (1977) ocean wave dissipation. High frequencies vanish first;", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "the fundamental remembers longest. Forgetting is a physical law.", 12.0f);
            y += 4.0f;

            // OSTRACON
            drawLine(headFont, Colour(192, 120, 90).withAlpha(0.80f),
                     "OSTRACON  \xc2\xb7  Shard Terracotta  \xc2\xb7  The Remembering Engine", 11.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "All voices write to a shared circular buffer simultaneously, then read", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "back through Mellotron-style oxide degradation: bandwidth loss, flutter,", 12.0f);
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.55f),
                     "and ghost bleed from previous revolutions. Memory is communal.", 12.0f);
            y += 4.0f;

            // Footer note
            drawLine(bodyFont, Colour(200, 204, 216).withAlpha(0.28f),
                     "Full mythology: xo-ox.org  \xc2\xb7  Field Guide  \xc2\xb7  ~52K words", 10.0f);
        }
    } loreContent_;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutModal)
};

//==============================================================================
/**
    OBadgeButton

    The "O" brand badge — a small circular button in the top-left corner of the
    Ocean View that opens the AboutModal.  D12 spec:
      - Single click → open About tab.
      - Long-press (≥500 ms) → open Lore tab.
        TODO D12: long-press detection stubbed; click opens About tab only.
        Full long-press wiring deferred — a 500ms Timer would need to be added
        and state managed across mouseUp cancellation.

    The button is placed as a direct child of XOceanusEditor (sits above OceanView)
    so it overlays the OceanView without modifying Wave 1B files.
*/
class OBadgeButton : public juce::Component,
                     public juce::SettableTooltipClient
{
public:
    //==========================================================================
    // Callback — wired to aboutModal_.openTab() in the editor.
    std::function<void()> onClick;

    //==========================================================================
    OBadgeButton()
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, false);
        setSize(kBadgeSize, kBadgeSize);
        setTooltip("About XOceanus");
    }

    static constexpr int kBadgeSize = 28;

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        using juce::Colour;

        const float sz  = static_cast<float>(kBadgeSize);
        const float pad = 2.0f;
        const juce::Rectangle<float> circle(pad, pad, sz - pad * 2.0f, sz - pad * 2.0f);

        const bool isHov = isMouseOver();

        // Outer ring
        const juce::Colour ringCol = isHov
            ? Colour(127, 219, 202).withAlpha(0.65f)
            : Colour(200, 204, 216).withAlpha(0.35f);
        g.setColour(ringCol);
        g.drawEllipse(circle, 1.5f);

        // "O" glyph
        const juce::Font badgeFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(13.0f));

        const juce::Colour textCol = isHov
            ? Colour(127, 219, 202).withAlpha(0.90f)
            : Colour(200, 204, 216).withAlpha(0.60f);

        g.setFont(badgeFont);
        g.setColour(textCol);
        g.drawText("O", getLocalBounds(), juce::Justification::centred, false);
    }

    //==========================================================================
    void mouseDown(const juce::MouseEvent&) override
    {
        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        repaint();
        if (getLocalBounds().contains(e.x, e.y))
        {
            // D12: single click → About tab (long-press → Lore is a TODO).
            // TODO D12: long-press → Lore tab (requires 500ms timer + mouseUp cancellation).
            if (onClick)
                onClick();
        }
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBadgeButton)
};

} // namespace xoceanus
