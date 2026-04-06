// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// ToastOverlay.h — Non-blocking toast notification system for XOceanus.
//
// Design constraints (Ghost Council BLOCK resolution):
//   - JUCE child component — NOT a top-level window (AUv3 iOS cannot create UIWindows).
//   - Added as the LAST child of XOceanusEditor so it paints on top of all panels.
//   - setInterceptsMouseClicks(false, false) — mouse falls through to engine panels.
//   - Gallery Model design system: dark semi-transparent bg, Inter font, XO Gold accent.
//
// Usage (from anywhere, including non-UI code via callAsync):
//   ToastOverlay::show("Preset failed to load", Toast::Warn);
//   ToastOverlay::show("Engine loaded", Toast::Info, 2000);
//
// Integration: XOceanusEditor adds one instance as its last child and calls
//   ToastOverlay::setInstance(&toastOverlay_) in the constructor, then
//   ToastOverlay::setInstance(nullptr) in the destructor.

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <algorithm>

#include "GalleryColors.h"

namespace xoceanus
{

//==============================================================================
// Toast — a single notification message.
//
// Instances are value types held in ToastOverlay::toasts_ (no heap allocation
// per toast). All timing is measured in milliseconds via juce::Time.
struct Toast
{
    enum class Level { Info, Warn, Error };

    juce::String message;
    Level        level       = Level::Info;
    int          durationMs  = 3000;
    float        alpha       = 0.0f;       // 0.0 (invisible) → 1.0 (fully visible)
    int64_t      showTimeMs  = 0;          // absolute ms when toast became fully visible
    bool         fadingIn    = true;       // true = fading in, false = fading out or held
    bool         fadingOut   = false;      // true = fade-out countdown active
    float        targetHeight = 0.0f;     // resolved in timerCallback after first measure
};

//==============================================================================
// ToastOverlay — transparent overlay component that renders stacked toasts.
//
// Sizing: covers the entire editor bounds (set by XOceanusEditor::resized()).
// Painting: only the toast pills in the bottom-right corner; background is
//   fully transparent everywhere else (hitTest returns false, too).
//
class ToastOverlay : public juce::Component, private juce::Timer
{
public:
    // ── Constants ─────────────────────────────────────────────────────────────
    static constexpr int   kToastW       = 300;  // fixed width (px)
    static constexpr int   kPadH         = 12;   // horizontal padding inside pill
    static constexpr int   kPadV         = 10;   // vertical padding inside pill
    static constexpr int   kMarginRight  = 16;   // margin from right edge
    static constexpr int   kMarginBottom = 16;   // margin from bottom edge
    static constexpr int   kGap          = 8;    // vertical gap between stacked toasts
    static constexpr int   kMaxVisible   = 3;    // max simultaneous toasts
    static constexpr float kCorner       = 6.0f; // rounded-corner radius
    static constexpr float kFontSize     = 13.0f;

    static constexpr int kFadeInMs  = 150; // fade-in duration
    static constexpr int kFadeOutMs = 200; // fade-out duration
    static constexpr int kTimerHz   = 60;  // animation timer rate

    // ── Singleton access ──────────────────────────────────────────────────────
    // XOceanusEditor sets this in its constructor/destructor.
    // All callers use show() which marshals to the message thread.
    static void setInstance(ToastOverlay* inst)
    {
        // Written only on the message thread (constructor/destructor of the editor).
        instance_ = inst;
    }

    // Primary API — call from any thread.
    // Marshals to the message thread before touching JUCE objects.
    static void show(const juce::String& msg,
                     Toast::Level        lvl       = Toast::Level::Info,
                     int                 durationMs = 3000)
    {
        // Capture by value so the lambda owns its data even if called from a bg thread.
        juce::MessageManager::callAsync(
            [msg, lvl, durationMs]
            {
                if (instance_ != nullptr)
                    instance_->showToast(msg, lvl, durationMs);
            });
    }

    // ── Instance API ──────────────────────────────────────────────────────────
    ToastOverlay()
    {
        // Never consume mouse events — they must reach the panels beneath.
        setInterceptsMouseClicks(false, false);
        // Transparent background — we paint only the pill regions.
        setOpaque(false);
    }

    ~ToastOverlay() override
    {
        stopTimer();
        // Safety: clear instance pointer if it still points to us.
        if (instance_ == this)
            instance_ = nullptr;
    }

    // Enqueue a new toast. Called on the message thread only (via show() or directly).
    void showToast(const juce::String& msg,
                   Toast::Level        lvl       = Toast::Level::Info,
                   int                 durationMs = 3000)
    {
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

        // Drop oldest visible toast when already at capacity.
        while (static_cast<int>(toasts_.size()) >= kMaxVisible)
            toasts_.erase(toasts_.begin());

        Toast t;
        t.message    = msg;
        t.level      = lvl;
        t.durationMs = juce::jmax(500, durationMs);
        t.alpha      = 0.0f;
        t.fadingIn   = true;
        t.fadingOut  = false;
        t.showTimeMs = 0; // will be set once alpha reaches 1.0
        toasts_.push_back(std::move(t));

        if (!isTimerRunning())
            startTimerHz(kTimerHz);
    }

    // hitTest: always false — the overlay is click-through everywhere.
    bool hitTest(int, int) override { return false; }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        if (toasts_.empty())
            return;

        auto font = GalleryFonts::body(kFontSize);
        g.setFont(font);

        // Build pill heights. Measure each toast's text height.
        // Toast pills stack from bottom-right, upward.
        const int maxTextW = kToastW - kPadH * 2;
        const int lineH    = static_cast<int>(std::ceil(kFontSize * 1.35f));

        // Compute bottom Y of the lowest toast (anchored to component bottom).
        int bottomY = getHeight() - kMarginBottom;

        // We'll draw in reverse order (oldest first, bottom) but measure all first.
        // Build (y, h, index) list from top so painting order is correct.
        struct PillInfo { int y, h, idx; };
        std::vector<PillInfo> pills;
        pills.reserve(toasts_.size());

        for (int i = static_cast<int>(toasts_.size()) - 1; i >= 0; --i)
        {
            const auto& toast = toasts_[static_cast<size_t>(i)];
            // Measure wrapped text height
            juce::AttributedString as;
            as.setText(toast.message);
            as.setFont(font);
            as.setColour(juce::Colours::white); // colour doesn't affect measurement
            as.setWordWrap(juce::AttributedString::byWord);
            juce::TextLayout tl;
            tl.createLayout(as, (float)maxTextW);
            const int textH = juce::jmax(lineH, static_cast<int>(std::ceil(tl.getHeight())));
            const int pillH = textH + kPadV * 2;

            const int y = bottomY - pillH;
            pills.push_back({y, pillH, i});
            bottomY = y - kGap;
        }

        // Draw pills (pills[0] is the bottom-most / newest).
        for (const auto& p : pills)
        {
            const auto& toast = toasts_[static_cast<size_t>(p.idx)];
            if (toast.alpha <= 0.005f)
                continue;

            const juce::Rectangle<float> pill(
                static_cast<float>(getWidth() - kMarginRight - kToastW),
                static_cast<float>(p.y),
                static_cast<float>(kToastW),
                static_cast<float>(p.h));

            // Semi-transparent dark background
            g.setColour(juce::Colour(0xDD1A1A2E).withAlpha(toast.alpha));
            g.fillRoundedRectangle(pill, kCorner);

            // Subtle border: 1px XO Gold at low alpha
            g.setColour(juce::Colour(GalleryColors::xoGold).withAlpha(0.25f * toast.alpha));
            g.drawRoundedRectangle(pill, kCorner, 1.0f);

            // Level accent bar — 3px left edge stripe
            juce::Colour levelColour = levelToColour(toast.level, toast.alpha);
            g.setColour(levelColour);
            g.fillRoundedRectangle(pill.getX(), pill.getY() + kCorner,
                                   3.0f, pill.getHeight() - kCorner * 2.0f, 1.5f);

            // Message text
            juce::Colour textColour = levelToTextColour(toast.level, toast.alpha);
            g.setColour(textColour);
            const juce::Rectangle<float> textBounds(
                pill.getX() + kPadH + 5.0f, // +5 for the accent bar
                pill.getY() + kPadV,
                pill.getWidth() - kPadH * 2.0f - 5.0f,
                pill.getHeight() - kPadV * 2.0f);
            g.drawFittedText(toast.message,
                             textBounds.toNearestInt(),
                             juce::Justification::centredLeft,
                             /* maxLines */ 5,
                             /* minimumHorizontalScale */ 1.0f);
        }
    }

    void resized() override
    {
        // No child components to lay out — we paint directly in paint().
    }

private:
    //==========================================================================
    void timerCallback() override
    {
        if (toasts_.empty())
        {
            stopTimer();
            return;
        }

        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        const float  dtSec = static_cast<float>(1.0 / kTimerHz);

        bool anyActive = false;
        bool needsRepaint = false;

        for (auto& t : toasts_)
        {
            if (t.fadingIn)
            {
                // Advance alpha toward 1.0 at rate: 1.0 / (kFadeInMs / 1000)
                const float step = dtSec / (kFadeInMs / 1000.0f);
                t.alpha = juce::jlimit(0.0f, 1.0f, t.alpha + step);
                needsRepaint = true;
                if (t.alpha >= 1.0f)
                {
                    t.fadingIn  = false;
                    t.showTimeMs = static_cast<int64_t>(nowMs);
                }
                anyActive = true;
            }
            else if (!t.fadingOut)
            {
                // Hold — check if duration has elapsed
                const int64_t elapsedMs = static_cast<int64_t>(nowMs) - t.showTimeMs;
                if (elapsedMs >= static_cast<int64_t>(t.durationMs))
                    t.fadingOut = true;
                anyActive = true;
            }
            else
            {
                // Fading out
                const float step = dtSec / (kFadeOutMs / 1000.0f);
                t.alpha = juce::jlimit(0.0f, 1.0f, t.alpha - step);
                needsRepaint = true;
                if (t.alpha <= 0.0f)
                {
                    // Mark for removal (don't erase mid-loop — done below).
                    t.alpha = -1.0f; // sentinel
                }
                else
                {
                    anyActive = true;
                }
            }
        }

        // Remove completed toasts (sentineled with alpha = -1.0f).
        toasts_.erase(
            std::remove_if(toasts_.begin(), toasts_.end(),
                           [](const Toast& t) { return t.alpha < 0.0f; }),
            toasts_.end());

        if (needsRepaint)
            repaint();

        if (!anyActive && toasts_.empty())
            stopTimer();
    }

    // Severity → accent stripe colour
    static juce::Colour levelToColour(Toast::Level lvl, float alpha) noexcept
    {
        juce::Colour base;
        switch (lvl)
        {
            case Toast::Level::Info:  base = juce::Colour(GalleryColors::xoGold); break;
            case Toast::Level::Warn:  base = juce::Colour(0xFFE9C46A); break; // XO Gold
            case Toast::Level::Error: base = juce::Colour(0xFFE07070); break; // Soft red
            default:                  base = juce::Colour(GalleryColors::xoGold); break;
        }
        return base.withAlpha(alpha);
    }

    // Severity → message text colour
    static juce::Colour levelToTextColour(Toast::Level lvl, float alpha) noexcept
    {
        juce::Colour base;
        switch (lvl)
        {
            case Toast::Level::Info:  base = juce::Colour(0xFFF0EDE8); break; // GalleryColors Dark::t1
            case Toast::Level::Warn:  base = juce::Colour(0xFFE9C46A); break; // XO Gold
            case Toast::Level::Error: base = juce::Colour(0xFFE07070); break; // Soft red
            default:                  base = juce::Colour(0xFFF0EDE8); break;
        }
        return base.withAlpha(alpha);
    }

    //==========================================================================
    std::vector<Toast> toasts_;

    // Global singleton — set by XOceanusEditor constructor/destructor.
    // Access only on the message thread (enforced by show() marshaling).
    static ToastOverlay* instance_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToastOverlay)
};

// Out-of-line static definition. Lives in the header (inline) to satisfy the
// header-only convention used across the XOceanus DSP and UI codebase.
inline ToastOverlay* ToastOverlay::instance_ = nullptr;

} // namespace xoceanus
