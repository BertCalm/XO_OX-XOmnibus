// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// SubmarineMenuStyle.h — Custom JUCE LookAndFeel for popup menus in the
// XOceanus submarine UI, with a 150 ms opacity fade-in animation.
//
// Design reference: Tools/ui-preview/submarine.html (.submarine-menu class)
//
// Visual spec:
//   Panel  — 192 px wide, dark gradient (#1e2028 → #16181e), 8 px radius, 1 px
//             teal border at 15 % alpha, outer teal rim at 5 % alpha, 4 px top/bottom padding.
//   Items  — 30 px tall, 11 px font, 14 px left/right padding.
//             Normal text: rgba(200,204,216, 0.70).  Highlighted: 0.95 alpha.
//             Highlight bg: rgba(60,180,170, 0.10).
//   Danger — detected via item.colour != transparent.
//             Text: rgba(239,68,68, 0.70) / hover 0.95.  Highlight bg: rgba(239,68,68, 0.10).
//   Disabled — 0.35 alpha applied over the text colour.
//   Separator — 1 px, rgba(200,204,216, 0.06), 3 px vertical margin, 10 px side inset.
//   Emoji — rendered inline; macOS handles emoji in JUCE font rendering correctly.
//
// Fade animation (150 ms):
//   preparePopupMenuWindow() — called by JUCE when the window is created; stores
//                              a WeakReference to it and sets initial alpha to 0.
//   resetFade()    — record birth time, start 60 Hz timer.
//   timerCallback() — drives Component::setAlpha(fade) on the stored window ptr;
//                     stops when fade reaches 1. Draw methods always paint at full
//                     opacity — the window-level alpha handles the visual fade.
//
// JUCE 8 compatibility notes:
//   - Font must be constructed via juce::Font(juce::FontOptions(size)) — the
//     single-float constructor is [[deprecated]] in JUCE 8 and triggers fall-back
//     to a wrong typeface when the deprecated path is taken.
//   - backgroundColourId must be registered with alpha < 1 (we use 0xFE / 254)
//     so that JUCE sets the popup window non-opaque.  An opaque window ignores
//     Component::setAlpha() and causes the gradient fill to composite against a
//     white background, making the dark glass panel invisible.
//   - getPopupMenuBorderSizeWithOptions must NOT return 0; returning 0 triggers a
//     JUCE 8 workaround path that dismisses the menu on mouse-up prematurely.
//     We return 1 and clip our own rounded-rect background to the full bounds.
//
// Usage:
//   SubmarineMenuLookAndFeel laf;
//   SubmarineMenuLookAndFeel::showWithFade(laf, menu, opts, callback);
//
// Header-only, inline — matches XOceanus Ocean UI convention.

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

//==============================================================================
/**
    SubmarineMenuLookAndFeel

    Custom LookAndFeel for popup menus in the XOceanus submarine UI.
    Inherits privately from juce::Timer to drive the 150 ms fade-in animation.

    The fade is implemented at the window level via Component::setAlpha() rather
    than per-pixel alpha in the draw methods.  Draw methods always paint at full
    design-spec opacity; the window opacity ramps 0 → 1 over 150 ms.

    Exported constant kDangerRed can be applied to PopupMenu::Item::colour by
    OceanView (or any caller) to mark destructive actions; this class detects
    non-transparent item colours and switches to the danger palette.
*/
class SubmarineMenuLookAndFeel : public juce::LookAndFeel_V4,
                                 private juce::Timer
{
public:
    //==========================================================================
    // Colour constants (match HTML prototype)
    static constexpr juce::uint8 kBg0_R = 0x28, kBg0_G = 0x2c, kBg0_B = 0x38;  // #282c38 top
    static constexpr juce::uint8 kBg1_R = 0x20, kBg1_G = 0x24, kBg1_B = 0x2e;  // #20242e bottom

    static constexpr float kTealR = 60.f,  kTealG = 180.f, kTealB = 170.f;
    static constexpr float kTextR = 200.f, kTextG = 204.f, kTextB = 216.f;

    // Teal border / rim
    static constexpr float kBorderAlpha = 0.35f;   // 1 px teal border
    static constexpr float kRimAlpha    = 0.12f;   // outer rim (approximated via second stroke)

    // Item dimensions / typography
    static constexpr int   kItemHeight    = 30;
    static constexpr int   kMenuWidth     = 192;
    static constexpr int   kHPad         = 14;
    static constexpr int   kVertPad      = 4;
    static constexpr float kFontSize     = 11.0f;
    static constexpr float kCornerRadius = 8.0f;
    // Must be >= 1 — returning 0 from getPopupMenuBorderSizeWithOptions triggers a
    // JUCE 8 special-case that dismisses the menu on mouse-up without a selection.
    static constexpr int   kBorderSize   = 1;

    // Alpha levels
    static constexpr float kTextNormal  = 0.70f;
    static constexpr float kTextHover   = 0.95f;
    static constexpr float kTextDisable = 0.35f;
    static constexpr float kHlBgAlpha   = 0.18f;

    // Separator
    static constexpr float kSepAlpha  = 0.06f;
    static constexpr int   kSepInset  = 10;
    static constexpr int   kSepVMarg  = 3;
    static constexpr int   kSepHeight = 1;

    //==========================================================================
    /** Danger colour for destructive menu items.
        Assign to PopupMenu::Item::colour when building the menu so that this
        LookAndFeel can switch to the red danger palette automatically.
    */
    static inline const juce::Colour kDangerRed { static_cast<juce::uint8>(239),
                                                   static_cast<juce::uint8>(68),
                                                   static_cast<juce::uint8>(68) };

    //==========================================================================
    SubmarineMenuLookAndFeel()
    {
        // Register the background colour with alpha = 0xFE (254/255 ≈ 99.6 %).
        // This is one LSB below fully opaque, which is enough to cause JUCE to
        // call setOpaque(false) on the popup window — a prerequisite for
        // Component::setAlpha() to work and for the rounded-corner transparency
        // to composite correctly against the desktop.  Visually indistinguishable
        // from fully opaque at full fade.
        setColour (juce::PopupMenu::backgroundColourId,
                   juce::Colour (kBg0_R, kBg0_G, kBg0_B).withAlpha (static_cast<juce::uint8> (0xFE)));
        setColour (juce::PopupMenu::textColourId,
                   juce::Colour (static_cast<juce::uint8> (kTextR),
                                 static_cast<juce::uint8> (kTextG),
                                 static_cast<juce::uint8> (kTextB)).withAlpha (kTextNormal));
        setColour (juce::PopupMenu::highlightedBackgroundColourId,
                   juce::Colour (static_cast<juce::uint8> (kTealR),
                                 static_cast<juce::uint8> (kTealG),
                                 static_cast<juce::uint8> (kTealB)).withAlpha (kHlBgAlpha));
        setColour (juce::PopupMenu::highlightedTextColourId,
                   juce::Colour (static_cast<juce::uint8> (kTextR),
                                 static_cast<juce::uint8> (kTextG),
                                 static_cast<juce::uint8> (kTextB)).withAlpha (kTextHover));
    }

    //==========================================================================
    // Fade API
    //==========================================================================

    /** Record the panel birth time and begin 60 Hz alpha-ramp driving. */
    void resetFade()
    {
        menuBirthMs_ = juce::Time::getMillisecondCounterHiRes();
        startTimerHz (60);
    }

    /** Returns 0..1 opacity based on elapsed time since resetFade(). */
    float getFadeAlpha() const
    {
        const double elapsed = juce::Time::getMillisecondCounterHiRes() - menuBirthMs_;
        return juce::jlimit (0.0f, 1.0f, static_cast<float> (elapsed / 150.0));
    }

    //==========================================================================
    // LookAndFeel overrides — exact signatures from juce_LookAndFeel_V2.h
    //==========================================================================

    /** Called by JUCE when the popup window component is created.
        We store a WeakReference to the window and set its initial alpha to 0 so
        the 60 Hz timer can ramp it to 1 via Component::setAlpha(). */
    void preparePopupMenuWindow (juce::Component& newWindow) override
    {
        menuWindow_ = &newWindow;
        // Start fully visible — fade disabled pending investigation
        newWindow.setAlpha (1.0f);
    }

    /** Panel background: dark gradient, teal border, outer rim.
        Colours are drawn at their full design-spec alpha — window-level opacity
        (set via Component::setAlpha in timerCallback) drives the visible fade. */
    void drawPopupMenuBackgroundWithOptions (juce::Graphics& g,
                                             int width,
                                             int height,
                                             const juce::PopupMenu::Options&) override
    {
        // --- Shadow / backdrop — slightly larger dark fill separates menu from ocean ---
        g.setColour (juce::Colour (0, 0, 0).withAlpha (0.6f));
        g.fillRoundedRectangle (-2.0f, -2.0f,
                                static_cast<float> (width)  + 4.0f,
                                static_cast<float> (height) + 4.0f,
                                kCornerRadius + 2.0f);

        // --- Gradient fill ---
        juce::ColourGradient grad (
            juce::Colour (kBg0_R, kBg0_G, kBg0_B),
            0.0f, 0.0f,
            juce::Colour (kBg1_R, kBg1_G, kBg1_B),
            0.0f, static_cast<float> (height),
            false);

        g.setGradientFill (grad);
        g.fillRoundedRectangle (0.0f, 0.0f,
                                static_cast<float> (width),
                                static_cast<float> (height),
                                kCornerRadius);

        // --- Outer rim (teal, 5 % alpha) — drawn as a slightly larger stroke ---
        g.setColour (juce::Colour (static_cast<juce::uint8> (kTealR),
                                   static_cast<juce::uint8> (kTealG),
                                   static_cast<juce::uint8> (kTealB))
                         .withAlpha (kRimAlpha));
        g.drawRoundedRectangle (0.5f, 0.5f,
                                static_cast<float> (width)  - 1.0f,
                                static_cast<float> (height) - 1.0f,
                                kCornerRadius, 2.0f);

        // --- 1 px teal border (15 % alpha) ---
        g.setColour (juce::Colour (static_cast<juce::uint8> (kTealR),
                                   static_cast<juce::uint8> (kTealG),
                                   static_cast<juce::uint8> (kTealB))
                         .withAlpha (kBorderAlpha));
        g.drawRoundedRectangle (0.5f, 0.5f,
                                static_cast<float> (width)  - 1.0f,
                                static_cast<float> (height) - 1.0f,
                                kCornerRadius, 1.0f);
    }

    /** Per-item rendering: separators, highlights, danger / normal text. */
    void drawPopupMenuItemWithOptions (juce::Graphics& g,
                                       const juce::Rectangle<int>& area,
                                       bool isHighlighted,
                                       const juce::PopupMenu::Item& item,
                                       const juce::PopupMenu::Options&) override
    {
        // --- Separator ---
        if (item.isSeparator)
        {
            const int y    = area.getY() + kSepVMarg + (area.getHeight() / 2);
            const int xL   = area.getX() + kSepInset;
            const int xR   = area.getRight() - kSepInset;

            g.setColour (juce::Colour (static_cast<juce::uint8> (kTextR),
                                       static_cast<juce::uint8> (kTextG),
                                       static_cast<juce::uint8> (kTextB))
                             .withAlpha (kSepAlpha));
            g.fillRect (xL, y, xR - xL, kSepHeight);
            return;
        }

        // --- Detect danger item (any non-transparent item.colour signals danger) ---
        const bool isDanger = !item.colour.isTransparent();

        // Base text colour (danger vs normal)
        const juce::Colour baseText = isDanger
            ? juce::Colour (static_cast<juce::uint8> (239),
                            static_cast<juce::uint8> (68),
                            static_cast<juce::uint8> (68))
            : juce::Colour (static_cast<juce::uint8> (kTextR),
                            static_cast<juce::uint8> (kTextG),
                            static_cast<juce::uint8> (kTextB));

        // Highlight background
        if (isHighlighted && item.isEnabled)
        {
            const juce::Colour hlBg = isDanger
                ? juce::Colour (static_cast<juce::uint8> (239),
                                static_cast<juce::uint8> (68),
                                static_cast<juce::uint8> (68)).withAlpha (kHlBgAlpha)
                : juce::Colour (static_cast<juce::uint8> (kTealR),
                                static_cast<juce::uint8> (kTealG),
                                static_cast<juce::uint8> (kTealB)).withAlpha (kHlBgAlpha);

            g.setColour (hlBg);
            g.fillRect (area);
        }

        // Text alpha — disabled takes priority, then hover vs normal
        const float textAlpha = !item.isEnabled ? kTextDisable
                                                 : (isHighlighted ? kTextHover : kTextNormal);

        g.setColour (baseText.withAlpha (textAlpha));
        // JUCE 8: juce::Font(float) is [[deprecated]]; use FontOptions constructor.
        g.setFont (juce::Font (juce::FontOptions (kFontSize)));

        const juce::Rectangle<int> textArea = area.withTrimmedLeft (kHPad)
                                                   .withTrimmedRight (kHPad);

        // Draw text left-aligned, vertically centred; emoji rendered inline by macOS
        g.drawText (item.text, textArea, juce::Justification::centredLeft, true);
    }

    /** Size hint: separator = 7 px tall (1 px + 2×3 px margin); items = kItemHeight. */
    void getIdealPopupMenuItemSizeWithOptions (const juce::String& /*text*/,
                                               bool isSeparator,
                                               int  /*standardMenuItemHeight*/,
                                               int& idealWidth,
                                               int& idealHeight,
                                               const juce::PopupMenu::Options&) override
    {
        idealWidth  = kMenuWidth;
        idealHeight = isSeparator ? (kSepVMarg * 2 + kSepHeight) : kItemHeight;
    }

    /** 11 px font for all menu text.
        JUCE 8: juce::Font(float) is [[deprecated]]; use FontOptions constructor. */
    juce::Font getPopupMenuFont() override
    {
        return juce::Font (juce::FontOptions (kFontSize));
    }

    /** Minimal border so JUCE does not trigger the border-size-0 dismiss quirk.
        Our custom background already paints the rounded panel with all padding. */
    int getPopupMenuBorderSizeWithOptions (const juce::PopupMenu::Options&) override
    {
        return kBorderSize;
    }

    //==========================================================================
    // Static show helper
    //==========================================================================

    /** Call resetFade() then show the menu asynchronously.
        The LAF must outlive the menu's lifetime (store it as a member). */
    static void showWithFade (SubmarineMenuLookAndFeel&       laf,
                              juce::PopupMenu&                menu,
                              const juce::PopupMenu::Options& opts,
                              std::function<void (int)>       callback)
    {
        laf.resetFade();
        menu.showMenuAsync (opts, std::move (callback));
    }

private:
    //==========================================================================
    void timerCallback() override
    {
        const float alpha = getFadeAlpha();

        // Drive window-level opacity ramp.  Using Component::setAlpha() rather
        // than per-pixel alpha in draw methods avoids compositing against the
        // window background colour (which would show grey/white on opaque windows).
        if (auto* win = menuWindow_.get())
            win->setAlpha (alpha);

        if (alpha >= 1.0f)
        {
            stopTimer();
            menuWindow_ = nullptr;
        }
    }

    //==========================================================================
    double menuBirthMs_ = 0.0;
    juce::WeakReference<juce::Component> menuWindow_;
};
