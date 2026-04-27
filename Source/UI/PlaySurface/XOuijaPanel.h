// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

/*
    XOuijaPanel.h
    ==============
    The main XOuija panel surface component.

    Renders:
      - Dark background (#1e1e22) with rounded corners
      - Pre-generated 128x128 noise texture at opacity 0.05
      - Circle-of-fifths markers (13 positions) with tension coloring and
        parabolic arc layout, using HarmonicField math
      - DEEP / FREE harmonic influence labels in GalleryFonts::body italic 11px at opacity 0.45,
        plus translucent onboarding axis guides (KEY SELECTION / HARMONIC DEPTH) at opacity 0.12
      - Planchette (B042): translucent oval lens with Lissajous idle drift,
        spring lock-on, warm memory hold, and interior text
      - GestureButtonBar (Task 7): FREEZE/HOME/DRIFT buttons with bank system
      - GoodbyeButton (Task 7): warm terracotta GOODBYE button

    Mouse input:
      - mouseDown/mouseDrag: updates circleX_ and influenceY_, fires
        onPositionChanged, and emits CC 85 (circleX) and CC 86 (influenceY)
        via onCCOutput. Also drives Planchette spring/move.
      - mouseUp: clears touching_ flag, triggers planchette release.

    Layout (bottom-up):
      - Bottom 32px: GoodbyeButton
      - Above that 34px: GestureButtonBar (28px buttons + 6px padding)
      - Harmonic surface occupies remaining area above

    B042 — Task 7 of 13 (XOuija PlaySurface V2)
    B043 — Trail-to-DSP wiring: TrailModulator exposes trail_length and
           trail_velocity as normalized 0-1 modulation outputs. Updated in
           mouseDrag via updateTrailModulator(). Host reads via getTrailModulator().
    Namespace: xoceanus
    JUCE 8, C++17
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <functional>
#include <cstdint>
#include <utility>
#include <random>
#include <cmath>
#include <array>
#include <cctype>

#include "HarmonicField.h"
#include "GestureTrailBuffer.h"
#include "../GalleryColors.h" // A11y::prefersReducedMotion() — unified reduced-motion helper (#223)
#include <atomic>

#if JUCE_MAC
#include <CoreFoundation/CoreFoundation.h> // CFPreferencesGetAppBooleanValue
#endif

// iOS port archived 2026-03-26 (issue #1107) — xoceanus::a11y_platform::isReduceMotionEnabled()
// forward-declaration removed. Source/UI/Mobile/ recoverable from git history.

namespace xoceanus
{

//==============================================================================
// TrailModulator (B043 — Gesture Trail as First-Class Modulation Source)
//
// Exposes two normalized 0-1 modulation signals derived from the XOuija
// gesture trail ring buffer:
//
//   trail_length   — normalized fill level of the ring buffer (0 = empty,
//                    1 = all 256 slots filled). Maps to reverb send amount or
//                    any macro parameter wired by the host.
//
//   trail_velocity — running average of the most recent 8 velocity samples
//                    in the trail buffer (clamped 0-1). Maps to filter cutoff
//                    modulation depth.
//
// Connection point for parameter wiring:
//   After any mouseDrag in XOuijaPanel, onTrailModulatorChanged is fired with
//   the current TrailModulator state.  Wire onTrailModulatorChanged in PlaySurface
//   (or the editor) to call processor_->getAPVTS().getParameter("...") and set values.
//   Example in PlaySurface::constructor:
//
//     xouijaPanel_.onTrailModulatorChanged = [this](const TrailModulator& mod) {
//         if (processor_) {
//             // trail_length -> reverb macro (normalized 0-1)
//             if (auto* p = processor_->getAPVTS().getParameter("macro4"))
//                 p->setValueNotifyingHost(mod.trail_length);
//             // trail_velocity -> filter brightness
//             if (auto* p = processor_->getAPVTS().getParameter("macro1"))
//                 p->setValueNotifyingHost(mod.trail_velocity);
//         }
//     };
//
struct TrailModulator
{
    // Normalized 0-1 fill level (# of live points / kBufferSize)
    float trail_length = 0.0f;

    // Normalized 0-1 mean velocity of the most recent ≤8 trail points
    float trail_velocity = 0.0f;
};

//==============================================================================
// GestureButtonMidiLearnManager
//
// Lightweight MIDI learn for the three gesture buttons in GestureButtonBar.
// Each button slot (0, 1, 2) can be mapped to an incoming MIDI CC number.
//
// Workflow:
//   1. User right-clicks a button → enter learn mode for that slot
//   2. Any incoming MIDI CC is captured and assigned to that slot
//   3. When a CC fires: the button's action() is invoked
//
// Integration with XOuijaPanel::processMidiMessage(const juce::MidiMessage&):
//   Call processMidiMessage() from PlaySurface each time a CC arrives
//   (e.g. via a juce::MidiMessageCollector timer drain).
//
// Persistence: toValueTree() / fromValueTree() round-trip all mappings.
// The XOuijaPanel saves/restores these as part of its own ValueTree child.
//
struct GestureButtonMidiLearnManager
{
    static constexpr int kNoCC = -1; // sentinel: no mapping

    struct ButtonMapping
    {
        int cc = kNoCC;  // mapped CC number, or kNoCC
        int channel = 0; // 0 = omni (any channel)
    };

    // Per-button mappings (indices 0-2)
    std::array<ButtonMapping, 3> mappings{};

    // Currently learning slot (-1 = not learning)
    int learnSlot = -1;

    // Learn mode: enter for a specific button slot.
    // Returns false if slot is out of range.
    bool enterLearnMode(int slot)
    {
        if (slot < 0 || slot >= 3)
            return false;
        learnSlot = slot;
        return true;
    }

    void exitLearnMode() { learnSlot = -1; }

    bool isLearning() const noexcept { return learnSlot >= 0; }
    int getLearningSlot() const noexcept { return learnSlot; }

    // Check if a slot has a CC mapping.
    bool hasMapping(int slot) const noexcept
    {
        if (slot < 0 || slot >= 3)
            return false;
        return mappings[static_cast<std::size_t>(slot)].cc != kNoCC;
    }

    // Get the CC number for a slot (-1 = unmapped).
    int getCCForSlot(int slot) const noexcept
    {
        if (slot < 0 || slot >= 3)
            return kNoCC;
        return mappings[static_cast<std::size_t>(slot)].cc;
    }

    // Clear the mapping for a slot.
    void clearMapping(int slot)
    {
        if (slot >= 0 && slot < 3)
            mappings[static_cast<std::size_t>(slot)] = {};
    }

    // Process an incoming MIDI CC message.
    // If in learn mode: assigns the CC to the learning slot (exits learn mode).
    // Otherwise: returns the slot index whose action should be triggered, or -1.
    //
    // Usage in XOuijaPanel:
    //   int slot = midiLearnMgr_.processMidi(msg);
    //   if (slot >= 0 && bankDefs_[...][slot].action)
    //       bankDefs_[...][slot].action();
    int processMidi(const juce::MidiMessage& msg)
    {
        if (!msg.isController())
            return -1;

        const int cc = msg.getControllerNumber();
        const int ch = msg.getChannel(); // 1-16

        if (isLearning())
        {
            mappings[static_cast<std::size_t>(learnSlot)].cc = cc;
            mappings[static_cast<std::size_t>(learnSlot)].channel = ch;
            exitLearnMode();
            return -1; // don't also fire the action on the learn event
        }

        // Find which slot (if any) maps to this CC
        for (int i = 0; i < 3; ++i)
        {
            const auto& m = mappings[static_cast<std::size_t>(i)];
            if (m.cc == cc && (m.channel == 0 || m.channel == ch))
                return i;
        }
        return -1;
    }

    // ── Serialization ──────────────────────────────────────────────────────────

    juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree("GestureButtonMidiLearn");
        for (int i = 0; i < 3; ++i)
        {
            juce::ValueTree child("Slot");
            child.setProperty("index", i, nullptr);
            child.setProperty("cc", mappings[static_cast<std::size_t>(i)].cc, nullptr);
            child.setProperty("channel", mappings[static_cast<std::size_t>(i)].channel, nullptr);
            tree.appendChild(child, nullptr);
        }
        return tree;
    }

    bool fromValueTree(const juce::ValueTree& tree)
    {
        if (!tree.isValid() || !tree.hasType("GestureButtonMidiLearn"))
            return false;

        for (int c = 0; c < tree.getNumChildren(); ++c)
        {
            auto child = tree.getChild(c);
            if (!child.hasType("Slot"))
                continue;
            int idx = static_cast<int>(child["index"]);
            if (idx < 0 || idx >= 3)
                continue;
            const int cc = static_cast<int>(child["cc"]);
            const int ch = static_cast<int>(child["channel"]);
            // Validate ranges
            if (cc >= -1 && cc <= 127 && ch >= 0 && ch <= 16)
            {
                mappings[static_cast<std::size_t>(idx)].cc = cc;
                mappings[static_cast<std::size_t>(idx)].channel = ch;
            }
        }
        return true;
    }
};

//==============================================================================
/** Planchette (B042 — The Planchette as Autonomous Entity)
    A translucent oval lens that drifts autonomously when idle and springs
    to the performer's touch.

    State machine:
      Lissajous → [springTo] → Springing → [spring complete] → Touching
      Touching   → [release]  → WarmHold  → [400ms elapsed]  → Lissajous

    setInterceptsMouseClicks(false, false): XOuijaPanel handles all mouse
    events and drives this component via the public API.
*/
class Planchette : public juce::Component, private juce::Timer
{
public:
    //==========================================================================
    // Sizing constants (spec Section 4)
    //==========================================================================
    static constexpr int kWidth = 68;
    static constexpr int kHeight = 46;
    static constexpr float kPipDiameter = 6.0f;

    // Lissajous drift parameters
    static constexpr float kDriftFreqX = 0.3f;                                  // Hz
    static constexpr float kDriftFreqY = 0.2f;                                  // Hz
    static constexpr float kDriftAmp = 0.15f;                                   // fraction of parent dimension
    static constexpr float kDriftPhase = juce::MathConstants<float>::pi / 4.0f; // π/4

    // Animation timing
    static constexpr float kSpringMs = 80.0f;    // spring duration ms (QDD: was 150ms, reduced for snappier lock-on)
    static constexpr float kWarmHoldMs = 400.0f; // warm hold duration ms

    //==========================================================================
    enum class State
    {
        Lissajous,
        Springing,
        Touching,
        WarmHold
    };

    //==========================================================================
    Planchette()
        : accentColour_(juce::Colour(0xFFE9C46A)), displayX_(0.5f), displayY_(0.5f), driftAnchorX_(0.5f),
          driftAnchorY_(0.5f), springStartX_(0.5f), springStartY_(0.5f), springTargetX_(0.5f), springTargetY_(0.5f),
          springElapsedMs_(0.0f), springDurationMs_(kSpringMs), warmHoldElapsedMs_(0.0f), driftTimeS_(0.0f),
          state_(State::Lissajous), driftEnabled_(true), displayText_("C · 0%")
          // Cache body italic font once — avoids repeated construction in paint()
          ,
          textFont_(GalleryFonts::body(10.0f).withStyle(juce::Font::italic))
    {
        setInterceptsMouseClicks(false, false);
        startTimerHz(60);

        // WCAG Fix 1: accessibility API
        setAccessible(true);
        setTitle("Planchette - Harmonic Position Indicator");
        setDescription("Shows current position on circle of fifths and influence depth");
    }

    ~Planchette() override { stopTimer(); }

    //==========================================================================
    // Public API
    //==========================================================================

    void setAccentColour(juce::Colour c)
    {
        accentColour_ = c;
        repaint();
    }

    void setDisplayText(juce::String text)
    {
        displayText_ = text;
        repaint();
    }

    /** Called on mouseDown — begin 80ms spring animation to (normX, normY) (300ms for GOODBYE). */
    void springTo(float normX, float normY)
    {
        springStartX_ = displayX_;
        springStartY_ = displayY_;
        springTargetX_ = normX;
        springTargetY_ = normY;
        springElapsedMs_ = 0.0f;
        springDurationMs_ = kSpringMs;
        state_ = State::Springing;
    }

    /** Slower spring used for GOODBYE — slides planchette to bottom over 300ms. */
    void springToSlow(float nx, float ny, float durationMs = 300.0f)
    {
        springStartX_ = displayX_;
        springStartY_ = displayY_;
        springTargetX_ = nx;
        springTargetY_ = ny;
        springElapsedMs_ = 0.0f;
        springDurationMs_ = durationMs;
        state_ = State::Springing;
    }

    /** Called on mouseDrag — snap to position once spring has completed. */
    void moveTo(float normX, float normY)
    {
        if (state_ == State::Springing || state_ == State::Touching)
        {
            // If still springing, update target; snap once touching
            springTargetX_ = normX;
            springTargetY_ = normY;
            if (state_ == State::Touching)
            {
                displayX_ = normX;
                displayY_ = normY;
                updateBounds();
                repaint();
            }
        }
    }

    /** Called on mouseUp — begin warm hold, then return to Lissajous drift. */
    void release()
    {
        driftAnchorX_ = displayX_;
        driftAnchorY_ = displayY_;
        warmHoldElapsedMs_ = 0.0f;
        // Reset drift phase so drift starts from anchor with zero displacement
        driftTimeS_ = 0.0f;
        state_ = State::WarmHold;
    }

    void setDriftEnabled(bool on) { driftEnabled_ = on; }
    bool isDriftEnabled() const { return driftEnabled_; }

    /** WCAG Fix 3: when true, Lissajous idle drift is suppressed and the planchette
        stays at its last anchor position. Wire to macOS accessibility preference
        (NSWorkspace.accessibilityDisplayShouldReduceMotion) when available. */
    void setReducedMotion(bool on) { reducedMotion_ = on; }
    bool isReducedMotion() const { return reducedMotion_; }

    /** Spring to centre (0.5, 0.5). */
    void snapHome() { springTo(0.5f, 0.5f); }

    float getDisplayX() const noexcept { return displayX_; }
    float getDisplayY() const noexcept { return displayY_; }

    bool isTouching() const noexcept { return state_ == State::Springing || state_ == State::Touching; }

    //==========================================================================
    // Component overrides
    //==========================================================================

    void visibilityChanged() override
    {
        if (isVisible())
            startTimerHz(60);
        else
            stopTimer();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // 1. Translucent oval lens fill — accent at 15% opacity
        g.setColour(accentColour_.withAlpha(0.15f));
        g.fillEllipse(bounds);

        // 2. Border — accent at full opacity, 1.5px stroke
        g.setColour(accentColour_);
        g.drawEllipse(bounds.reduced(0.75f), 1.5f);

        // 3. Inner glow — accent at 40% opacity, slightly inset ellipse
        g.setColour(accentColour_.withAlpha(0.40f));
        g.drawEllipse(bounds.reduced(4.0f), 1.0f);

        // 4. Center pip — 6px filled circle
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        g.setColour(accentColour_);
        g.fillEllipse(cx - kPipDiameter * 0.5f, cy - kPipDiameter * 0.5f, kPipDiameter, kPipDiameter);

        // 5. Interior text — body italic 10px, accent at 85% opacity
        //    Positioned below centre. Uses cached textFont_ (not rebuilt each paint).
        g.setColour(accentColour_.withAlpha(0.85f));
        g.setFont(textFont_);
        const float textY = cy + kPipDiameter * 0.5f + 1.0f;
        g.drawText(displayText_, juce::Rectangle<float>(bounds.getX(), textY, bounds.getWidth(), 12.0f),
                   juce::Justification::centred, false);
    }

private:
    //==========================================================================
    // Timer callback — 60Hz state machine
    //==========================================================================
    void timerCallback() override
    {
        constexpr float kDtMs = 1000.0f / 60.0f; // ~16.67 ms per tick

        switch (state_)
        {
        case State::Springing:
        {
            springElapsedMs_ += kDtMs;
            const float t = juce::jlimit(0.0f, 1.0f, springElapsedMs_ / springDurationMs_);
            // Ease-out: 1 - (1-t)^2
            const float ease = 1.0f - (1.0f - t) * (1.0f - t);
            displayX_ = springStartX_ + (springTargetX_ - springStartX_) * ease;
            displayY_ = springStartY_ + (springTargetY_ - springStartY_) * ease;

            if (springElapsedMs_ >= springDurationMs_)
            {
                displayX_ = springTargetX_;
                displayY_ = springTargetY_;
                state_ = State::Touching;
            }
            updateBounds();
            repaint();
            break;
        }

        case State::Touching:
            // Position is updated synchronously via moveTo(); nothing to do here.
            break;

        case State::WarmHold:
        {
            warmHoldElapsedMs_ += kDtMs;
            if (warmHoldElapsedMs_ >= kWarmHoldMs)
            {
                // Transition to drift: anchor is current displayX_/displayY_
                driftAnchorX_ = displayX_;
                driftAnchorY_ = displayY_;
                driftTimeS_ = 0.0f;
                state_ = State::Lissajous;
            }
            break;
        }

        case State::Lissajous:
        {
            // WCAG Fix 3: skip drift entirely when reduced motion is requested
            if (!driftEnabled_ || reducedMotion_)
                break;

            driftTimeS_ += kDtMs * 0.001f; // convert ms → seconds

            // Compute Lissajous displacement as fraction of parent size
            const float driftX = kDriftAmp * std::sin(juce::MathConstants<float>::twoPi * kDriftFreqX * driftTimeS_);
            const float driftY =
                kDriftAmp * std::sin(juce::MathConstants<float>::twoPi * kDriftFreqY * driftTimeS_ + kDriftPhase);

            displayX_ = driftAnchorX_ + driftX;
            displayY_ = driftAnchorY_ + driftY;

            // Clamp so planchette stays fully on screen (Fix #20: use half-size margins
            // so the planchette body never goes off-screen at the edges).
            if (auto* parent = getParentComponent())
            {
                const float parentW = static_cast<float>(parent->getWidth());
                const float parentH = static_cast<float>(parent->getHeight());
                const float marginX = (parentW > 0.0f) ? static_cast<float>(kWidth) / (2.0f * parentW) : 0.0f;
                const float marginY = (parentH > 0.0f) ? static_cast<float>(kHeight) / (2.0f * parentH) : 0.0f;
                displayX_ = juce::jlimit(marginX, 1.0f - marginX, displayX_);
                displayY_ = juce::jlimit(marginY, 1.0f - marginY, displayY_);
            }
            else
            {
                displayX_ = juce::jlimit(0.0f, 1.0f, displayX_);
                displayY_ = juce::jlimit(0.0f, 1.0f, displayY_);
            }

            updateBounds();
            repaint();
            break;
        }
        }
    }

    //==========================================================================
    // Position the planchette within its parent using normalised coords.
    //   px = displayX * parentW - kWidth/2
    //   py = (1 - displayY) * parentH - kHeight/2   (Y-flip: 0=bottom, 1=top)
    //==========================================================================
    void updateBounds()
    {
        if (auto* parent = getParentComponent())
        {
            const float pw = static_cast<float>(parent->getWidth());
            const float ph = static_cast<float>(parent->getHeight());

            const int px = juce::roundToInt(displayX_ * pw - kWidth * 0.5f);
            const int py = juce::roundToInt((1.0f - displayY_) * ph - kHeight * 0.5f);

            setBounds(px, py, kWidth, kHeight);
        }
    }

    //==========================================================================
    // State
    //==========================================================================
    juce::Colour accentColour_;
    float displayX_;
    float displayY_;

    // Drift anchor — centre of Lissajous figure
    float driftAnchorX_;
    float driftAnchorY_;

    // Spring animation
    float springStartX_;
    float springStartY_;
    float springTargetX_;
    float springTargetY_;
    float springElapsedMs_;
    float springDurationMs_; // kSpringMs normally; 300ms for GOODBYE

    // Warm hold
    float warmHoldElapsedMs_;

    // Drift time accumulator (seconds)
    float driftTimeS_;

    State state_;
    bool driftEnabled_;
    juce::String displayText_;

    // WCAG Fix 3 (RESOLVED): reduced motion path — when true, skip Lissajous drift animation.
    // Initialised via XOuijaPanel::syncReducedMotionFromSystem() on construction.
    // macOS: reads CFPreferences "reduceMotion" / "com.apple.universalaccess" (pure C).
    // iOS port archived 2026-03-26 (#1107); bridge removed from Source/UI/Mobile/.
    bool reducedMotion_ = false;

    // Cached font — initialized in constructor, avoids per-paint construction
    juce::Font textFont_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Planchette)
};

//==============================================================================
/** GestureButtonBar (Task 7 — Spec Section 6)
    Three configurable gesture buttons (FREEZE / HOME / DRIFT in default XOuija
    bank) with a 4-bank system and an optional lock pin.

    Auto-follows PerformanceStrip mode when unlocked. Each button has a label,
    single-character keyboard shortcut, and action callback.
*/
class GestureButtonBar : public juce::Component
{
public:
    //==========================================================================
    enum class Bank
    {
        XOuija,
        Dub,
        Coupling,
        Performance
    };

    struct ButtonDef
    {
        juce::String label;
        char shortcutKey = 0;
        std::function<void()> action;
    };

    //==========================================================================
    GestureButtonBar()
        // Cache fonts once — avoids per-paint Font construction (UIX fix)
        : lockFont_(juce::Font(14.0f, juce::Font::plain)), buttonFont_(juce::Font(10.0f, juce::Font::plain))
    {
        // Default bank buttons are populated by XOuijaPanel::setupDefaultButtonBank()
        // so they remain empty here.

        // WCAG Fix 1: accessibility API
        setAccessible(true);
        setTitle("Performance Gesture Buttons");
        setDescription("Three configurable performance buttons: Freeze, Home, Drift");
    }

    ~GestureButtonBar() override = default;

    //==========================================================================
    // Public API
    //==========================================================================

    /** Switch bank; respects the lock pin. */
    void setBank(Bank bank)
    {
        if (bankLocked_)
            return;
        currentBank_ = bank;
        repaint();
    }

    Bank getBank() const noexcept { return currentBank_; }

    void setBankLocked(bool locked)
    {
        bankLocked_ = locked;
        repaint();
    }
    bool isBankLocked() const noexcept { return bankLocked_; }

    /** Set the 3 button definitions for the current active bank (XOuija bank). */
    void setButtons(const std::array<ButtonDef, 3>& defs)
    {
        buttons_ = defs;
        repaint();
    }

    /** Check shortcut key; fire action and return true if handled. */
    bool handleKey(char c)
    {
        for (auto& btn : buttons_)
        {
            if (btn.shortcutKey != 0 && std::toupper(static_cast<unsigned char>(c)) ==
                                            std::toupper(static_cast<unsigned char>(btn.shortcutKey)))
            {
                if (btn.action)
                    btn.action();
                pressedIndex_ = -1; // no persistent highlight for key events
                repaint();
                return true;
            }
        }
        return false;
    }

    //==========================================================================
    // Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float w = b.getWidth();

        // ---- Lock button (top-right, ~14px) ----
        const float lockSize = 14.0f;
        const float lockX = w - lockSize - 2.0f;
        const float lockY = 2.0f;
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.setFont(lockFont_); // cached — avoids per-paint Font construction
        // Filled circle (locked) / empty circle (unlocked) — universally understood
        g.drawText(bankLocked_ ? "\xe2\x97\x8f" : "\xe2\x97\x8b",
                   juce::Rectangle<float>(lockX, lockY, lockSize, lockSize), juce::Justification::centred, false);

        // ---- Three button slots ----
        const float cornerR = 3.0f;

        for (int i = 0; i < 3; ++i)
        {
            const juce::Rectangle<float> buttonRect = getButtonRect(i);

            const bool pressed = (pressedIndex_ == i);
            const bool toggled = buttonToggled_[static_cast<std::size_t>(i)];
            const bool learning = (learnSlot_ == i);

            // Background
            if (learning)
            {
                // Feature 3: pulsing amber tint in learn mode
                // Use a phase derived from component-level time (simple fixed blink)
                const float learnAlpha = 0.55f;
                g.setColour(juce::Colour(0xFFE9A84A).withAlpha(learnAlpha));
            }
            else if (pressed)
                g.setColour(accentColour_.withAlpha(0.30f));
            else if (toggled)
                g.setColour(accentColour_.withAlpha(0.18f));
            else
                g.setColour(juce::Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.06f));
            g.fillRoundedRectangle(buttonRect, cornerR);

            // Border — thicker + coloured when toggled or in learn mode
            if (learning)
            {
                g.setColour(juce::Colour(0xFFE9A84A));
                g.drawRoundedRectangle(buttonRect, cornerR, 1.5f);
            }
            else if (toggled)
            {
                g.setColour(accentColour_.withAlpha(0.80f));
                g.drawRoundedRectangle(buttonRect, cornerR, 1.5f);
            }
            else if (!pressed)
            {
                g.setColour(juce::Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.12f));
                g.drawRoundedRectangle(buttonRect, cornerR, 1.0f);
            }

            // Label
            const float textAlpha = (pressed || toggled || learning) ? 1.0f : 0.65f;
            g.setColour(learning ? juce::Colour(0xFFE9A84A) : juce::Colours::white.withAlpha(textAlpha));
            g.setFont(buttonFont_); // cached — avoids per-paint Font construction; 10px

            // Feature 3: show CC number in learn mode, otherwise normal label
            juce::String label = buttons_[static_cast<std::size_t>(i)].label.toUpperCase();
            if (learning)
                label = "LEARN...";
            g.drawText(label, buttonRect, juce::Justification::centred, false);
        }
    }

    // Feature 3: callback fired when user right-clicks a button to enter learn mode.
    // Owner (XOuijaPanel) wires this to call midiLearnMgr_.enterLearnMode(slot).
    std::function<void(int /*slot*/)> onEnterLearnMode;

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Check lock icon click first — sits above the button row
        float lockX = static_cast<float>(getWidth()) - 18.0f;
        if (e.position.x >= lockX && e.position.y < 18.0f)
        {
            bankLocked_ = !bankLocked_;
            repaint();
            return;
        }

        // Feature 3: right-click → enter MIDI learn mode for that button
        if (e.mods.isRightButtonDown())
        {
            const int idx = hitTestButton(e.position.x, e.position.y);
            if (idx >= 0)
            {
                // Toggle: if already learning this slot, cancel; else enter
                if (learnSlot_ == idx)
                {
                    learnSlot_ = -1;
                    repaint();
                }
                else
                {
                    learnSlot_ = idx;
                    repaint();
                    if (onEnterLearnMode)
                        onEnterLearnMode(idx);
                }
            }
            return;
        }

        pressedIndex_ = hitTestButton(e.position.x, e.position.y);
        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
            return; // right-click handled in mouseDown

        const int idx = hitTestButton(e.position.x, e.position.y);
        if (idx >= 0 && idx == pressedIndex_)
        {
            const auto& btn = buttons_[static_cast<std::size_t>(idx)];
            if (btn.action)
                btn.action();
        }
        pressedIndex_ = -1;
        repaint();
    }

    void setAccentColour(juce::Colour c)
    {
        accentColour_ = c;
        repaint();
    }

    /** Returns the rect for button at index (0, 1, or 2). */
    juce::Rectangle<float> getButtonRect(int index) const
    {
        float w = static_cast<float>(getWidth());
        float gutter = 4.0f;
        float buttonW = (w - gutter * 4.0f) / 3.0f;
        float buttonH = 28.0f;
        float y = (static_cast<float>(getHeight()) - buttonH) / 2.0f;
        return {gutter + static_cast<float>(index) * (buttonW + gutter), y, buttonW, buttonH};
    }

    //==========================================================================
    // Feature 1: Per-button toggle state
    //   Tracks whether each button is in a "toggled on" state for persistence.
    //   The XOuijaPanel sets these via setButtonToggleState() when stateful
    //   actions are triggered (e.g. FREEZE, LOOP, MUTE, INVERT, LATCH, BYPASS).
    //==========================================================================

    void setButtonToggleState(int index, bool on)
    {
        if (index >= 0 && index < 3)
        {
            buttonToggled_[static_cast<std::size_t>(index)] = on;
            repaint();
        }
    }

    bool getButtonToggleState(int index) const noexcept
    {
        if (index < 0 || index >= 3)
            return false;
        return buttonToggled_[static_cast<std::size_t>(index)];
    }

    //==========================================================================
    // Feature 3: MIDI learn visual state
    //   When a slot is in learn mode, its button pulses/highlights differently.
    //   Call setLearnSlot(-1) to exit visual learn mode.
    //==========================================================================

    void setLearnSlot(int slot)
    {
        learnSlot_ = slot;
        repaint();
    }

    int getLearnSlot() const noexcept { return learnSlot_; }

    //==========================================================================
    // Feature 1 + 3: Persistence — ValueTree serialization
    //   Saves: active bank index, bank-lock state, per-button toggle states,
    //          and MIDI learn mappings.
    //   Called from XOuijaPanel::toValueTree() / fromValueTree().
    //==========================================================================

    juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree("GestureButtonBar");
        tree.setProperty("bank", static_cast<int>(currentBank_), nullptr);
        tree.setProperty("bankLocked", bankLocked_, nullptr);
        tree.setProperty("toggle0", buttonToggled_[0], nullptr);
        tree.setProperty("toggle1", buttonToggled_[1], nullptr);
        tree.setProperty("toggle2", buttonToggled_[2], nullptr);
        return tree;
    }

    void fromValueTree(const juce::ValueTree& tree)
    {
        if (!tree.isValid() || !tree.hasType("GestureButtonBar"))
            return;

        const int bankIdx = static_cast<int>(tree["bank"]);
        if (bankIdx >= 0 && bankIdx <= 3)
            currentBank_ = static_cast<Bank>(bankIdx);

        bankLocked_ = static_cast<bool>(tree["bankLocked"]);

        buttonToggled_[0] = static_cast<bool>(tree["toggle0"]);
        buttonToggled_[1] = static_cast<bool>(tree["toggle1"]);
        buttonToggled_[2] = static_cast<bool>(tree["toggle2"]);

        repaint();
    }

private:
    //==========================================================================
    int hitTestButton(float mx, float my) const
    {
        const float h = static_cast<float>(getHeight());
        const float buttonH = 28.0f;
        const float buttonTop = h - buttonH;

        if (my < buttonTop)
            return -1;

        for (int i = 0; i < 3; ++i)
        {
            auto r = getButtonRect(i);
            if (mx >= r.getX() && mx < r.getRight())
                return i;
        }
        return -1;
    }

    //==========================================================================
    std::array<ButtonDef, 3> buttons_;
    Bank currentBank_ = Bank::XOuija;
    bool bankLocked_ = false;
    int pressedIndex_ = -1;
    juce::Colour accentColour_{juce::Colour(0xFFE9C46A)};

    // Feature 1: per-button toggle on/off state (for persistence)
    std::array<bool, 3> buttonToggled_{};

    // Feature 3: which slot is in MIDI learn mode (-1 = none)
    int learnSlot_ = -1;

    // Cached fonts — initialized in constructor, avoids per-paint construction
    juce::Font lockFont_;   // 14px plain — for lock indicator circle
    juce::Font buttonFont_; // 10px plain — for button labels

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GestureButtonBar)
};

//==============================================================================
//==============================================================================
// MoodSliderBar (Wave 5 D2)
//
// A compact horizontal strip housing three bipolar sliders:
//   Brightness (dark ↔ bright), Tension (calm ↔ tense), Density (sparse ↔ dense)
//
// Each slider is 0–1 internally (default 0.5 = neutral).
// Renders as three pill-shaped tracks with a centred-thumb convention:
//   left half = negative pole, right half = positive pole.
//   At 0.5 (neutral) the thumb sits centre and the track is uniform gray.
//
// Labels are abbreviated (B / T / D) to fit within the narrow XOuija panel.
//
// onMoodChanged is fired on every drag event.  It is called on the message
// thread and must not block or allocate.
//
// Layout: bar occupies a fixed kMoodBarH = 42px strip inserted above the
// GestureButtonBar.  XOuijaPanel::resized() handles all positioning.
//==============================================================================
class MoodSliderBar : public juce::Component
{
public:
    static constexpr int kMoodBarH = 42;

    struct MoodValues
    {
        float brightness = 0.5f; // 0=dark,   1=bright
        float tension    = 0.5f; // 0=calm,   1=tense
        float density    = 0.5f; // 0=sparse, 1=dense
    };

    std::function<void(const MoodValues&)> onMoodChanged;

    MoodSliderBar()
    {
        setOpaque(false);
        setAccessible(true);
        setTitle("XOuija Mood Sliders");
        setDescription("Brightness, Tension, and Density sliders that shape the heatmap overlay");
        setWantsKeyboardFocus(false);
    }

    ~MoodSliderBar() override = default;

    //==========================================================================
    // API
    //==========================================================================
    void setMoodValues(const MoodValues& v)
    {
        mood_ = v;
        mood_.brightness = juce::jlimit(0.0f, 1.0f, mood_.brightness);
        mood_.tension    = juce::jlimit(0.0f, 1.0f, mood_.tension);
        mood_.density    = juce::jlimit(0.0f, 1.0f, mood_.density);
        repaint();
    }

    const MoodValues& getMoodValues() const noexcept { return mood_; }

    void setAccentColour(juce::Colour c) { accentColour_ = c; repaint(); }

    //==========================================================================
    // Rendering
    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        if (b.isEmpty())
            return;

        // Subtle separator line at top
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.drawHorizontalLine(0, b.getX(), b.getRight());

        // Labels: B  T  D  — each occupies 1/3 width
        constexpr int kN = 3;
        const float slotW = b.getWidth() / static_cast<float>(kN);
        const float trackH = 5.0f;
        const float thumbR = 5.0f;
        const float trackY = b.getCentreY() + 6.0f; // below label
        constexpr const char* kLabels[kN] = {"B", "T", "D"};
        const float kValues[kN] = {mood_.brightness, mood_.tension, mood_.density};

        static const juce::Font kFont = GalleryFonts::body(9.0f).withStyle(juce::Font::bold);
        g.setFont(kFont);

        for (int i = 0; i < kN; ++i)
        {
            const float slotLeft = b.getX() + static_cast<float>(i) * slotW;
            const float trackLeft  = slotLeft + thumbR + 2.0f;
            const float trackRight = slotLeft + slotW - thumbR - 2.0f;
            const float trackLen   = trackRight - trackLeft;

            // Label above the track
            const float labelY = b.getY() + 2.0f;
            g.setColour(juce::Colours::white.withAlpha(0.55f));
            g.drawText(kLabels[i],
                       juce::Rectangle<float>(slotLeft, labelY, slotW, 12.0f),
                       juce::Justification::centred, false);

            // Track background — two halves
            const float trackMid = trackLeft + trackLen * 0.5f;
            const float v = kValues[i]; // [0,1]
            const float thumbX = trackLeft + v * trackLen;

            // Left half (neutral→negative pole) — slightly cooler color
            g.setColour(juce::Colour(0xFF2a2a30));
            g.fillRoundedRectangle(trackLeft, trackY - trackH * 0.5f,
                                   trackLen, trackH, 2.0f);

            // Active fill — from centre to thumb
            const float fillLeft  = std::min(thumbX, trackMid);
            const float fillRight = std::max(thumbX, trackMid);
            const float fillW = fillRight - fillLeft;
            if (fillW > 0.5f)
            {
                const float alpha = 0.5f + std::abs(v - 0.5f); // brighter when away from neutral
                g.setColour(accentColour_.withAlpha(alpha));
                g.fillRoundedRectangle(fillLeft, trackY - trackH * 0.5f,
                                       fillW, trackH, 2.0f);
            }

            // Thumb
            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.fillEllipse(thumbX - thumbR, trackY - thumbR, thumbR * 2.0f, thumbR * 2.0f);

            // Neutral tick mark at centre
            g.setColour(juce::Colours::white.withAlpha(0.20f));
            g.fillRect(juce::Rectangle<float>(trackMid - 0.5f, trackY - trackH * 0.5f - 1.0f,
                                              1.0f, trackH + 2.0f));
        }
    }

    //==========================================================================
    // Mouse handling — drag any of the 3 slider tracks
    //==========================================================================
    void mouseDown(const juce::MouseEvent& e) override { updateFromMouse(e); }
    void mouseDrag(const juce::MouseEvent& e) override { updateFromMouse(e); }

private:
    MoodValues   mood_;
    juce::Colour accentColour_{ juce::Colour(0xFFE9C46A) }; // XO Gold default

    //==========================================================================
    // Map mouse X to [0,1] within the appropriate slider slot.
    //==========================================================================
    void updateFromMouse(const juce::MouseEvent& e)
    {
        const float bW = static_cast<float>(getWidth());
        if (bW <= 0.0f)
            return;

        const float mx = static_cast<float>(e.getPosition().x);
        constexpr int kN = 3;
        const float slotW = bW / static_cast<float>(kN);
        const float thumbR = 5.0f;

        const int slot = juce::jlimit(0, kN - 1, static_cast<int>(mx / slotW));
        const float slotLeft  = static_cast<float>(slot) * slotW;
        const float trackLeft  = slotLeft + thumbR + 2.0f;
        const float trackRight = slotLeft + slotW - thumbR - 2.0f;
        const float trackLen   = trackRight - trackLeft;

        if (trackLen <= 0.0f)
            return;

        const float v = juce::jlimit(0.0f, 1.0f, (mx - trackLeft) / trackLen);

        switch (slot)
        {
        case 0: mood_.brightness = v; break;
        case 1: mood_.tension    = v; break;
        case 2: mood_.density    = v; break;
        default: break;
        }

        repaint();

        if (onMoodChanged)
            onMoodChanged(mood_);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoodSliderBar)
};


/** GoodbyeButton (Task 7 — Spec Section 7)
    Warm Terracotta full-width button that resets the XOuija session.
    Color: #E07A5F at 80% opacity normally, 100% on press.
    Text: "RELEASE" in body italic 11px (GalleryFonts::body), white at 90 %
    opacity. Class name keeps "Goodbye" for the gesture-bank wiring; the
    visible label is RELEASE per #1171 (atmospheric → functional).
*/
class GoodbyeButton : public juce::Component
{
public:
    //==========================================================================
    GoodbyeButton()
        // Cache body italic font once — avoids repeated construction in paint()
        : labelFont_(GalleryFonts::body(11.0f).withStyle(juce::Font::italic))
    {
        // WCAG Fix 1: accessibility API
        setAccessible(true);
        setTitle("Release - All Notes Off");
        setDescription("Releases the planchette from the locked note and silences all voices");
    }
    ~GoodbyeButton() override = default;

    //==========================================================================
    std::function<void()> onGoodbye;

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float alpha = pressed_ ? 1.0f : 0.80f;
        const juce::Colour base{0xFFE07A5F}; // Warm Terracotta

        // Background — no border
        g.setColour(base.withAlpha(alpha));
        g.fillRoundedRectangle(b, 4.0f);

        // Label — uses cached labelFont_ (not rebuilt each paint)
        g.setColour(juce::Colours::white.withAlpha(0.90f));
        g.setFont(labelFont_);
        g.drawText("RELEASE", b, juce::Justification::centred, false);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        pressed_ = true;
        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (getLocalBounds().toFloat().contains(e.position))
            if (onGoodbye)
                onGoodbye();
        pressed_ = false;
        repaint();
    }

private:
    bool pressed_ = false;
    juce::Font labelFont_; // cached in constructor — body italic 11px

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GoodbyeButton)
};

//==============================================================================
class XOuijaPanel : public juce::Component
{
public:
    // Sizing constants (spec Section 2.2)
    //==========================================================================
    static constexpr int kMinWidth = 155;
    static constexpr int kMaxWidth = 185;
    static constexpr int kPreferredWidth = 165;

    // Aspect ratio ~9:19.5  (width : height)
    static constexpr float kAspectRatio = 9.0f / 19.5f;

    //==========================================================================
    // Reserved layout heights (for Tasks 5-7)
    //==========================================================================
    static constexpr int kGestureBarH = 34; // Task 7: GestureButtonBar
    static constexpr int kGoodbyeH = 32;    // Task 7: GOODBYE button
    static constexpr int kMoodBarH  = MoodSliderBar::kMoodBarH; // Wave 5 D2: mood sliders

    //==========================================================================
    // Constructor / Destructor
    //==========================================================================
    XOuijaPanel() : accentColour_(juce::Colour(0xFFE9C46A)) // XO Gold
    {
        generateNoiseTexture();
        setOpaque(false);

        // WCAG Fix 1: accessibility API
        setAccessible(true);
        setTitle("XOuija Harmonic Navigator");
        setDescription("Circle of fifths harmonic navigation surface with influence depth control");

        // B042: add the planchette as a child — it manages its own bounds
        addAndMakeVisible(planchette_);
        A11y::setup(planchette_, "Planchette - Harmonic Position Indicator",
                    "Shows current position on circle of fifths and influence depth", false);
        planchette_.setAccentColour(accentColour_);
        updatePlanchetteText();

        // Task 7: gesture button bar and GOODBYE button
        addAndMakeVisible(gestureButtons_);
        A11y::setup(gestureButtons_, "Gesture Buttons", "XOuija gesture action bank: Freeze, Home, Drift and more");
        gestureButtons_.setAccentColour(accentColour_);

        addAndMakeVisible(goodbyeButton_);
        A11y::setup(goodbyeButton_, "Goodbye - All Notes Off", "Resets harmonic position and silences all voices");

        // Wire GOODBYE action
        goodbyeButton_.onGoodbye = [this]()
        {
            // Reset position state
            circleX_ = 0.5f;
            influenceY_ = 0.0f;

            // Animate planchette to bottom-centre (influenceY=0 → bottom)
            planchette_.springToSlow(0.5f, 0.0f, 300.0f);

            // Clear trail buffer
            trailBuffer_.clear();

            // Emit CC resets: CC 85→64 (centre), CC 86→0 (no influence)
            // CC 88 = trail freeze state (Spec Section 11.1). GOODBYE unfreezes.
            if (onCCOutput)
            {
                onCCOutput(85, 64);
                onCCOutput(86, 0);
                onCCOutput(88, 0); // CC 88 = trail freeze state (Spec Section 11.1). GOODBYE unfreezes.
            }

            // Update planchette text
            updatePlanchetteText();

            // Notify PlaySurface (for All Notes Off etc.)
            if (onGoodbye)
                onGoodbye();

            repaint();
        };

        // Wave 5 D2: Mood slider bar (B/T/D sliders + heatmap toggle)
        addAndMakeVisible(moodSliderBar_);
        moodSliderBar_.setAccentColour(accentColour_);
        moodSliderBar_.onMoodChanged = [this](const MoodSliderBar::MoodValues& m)
        {
            moodState_ = m;
            moodHeatmapDirty_ = true;
            repaint();
            // Forward to host callback so PlaySurface can sync APVTS params
            if (onMoodChanged)
                onMoodChanged(m);
        };

        // Wire all four gesture button banks (XOuija active by default)
        setupDefaultButtonBank();     // Bank 0: FREEZE / HOME / DRIFT
        setupDubButtonBank();         // Bank 1: LOOP / MUTE / RESET
        setupCouplingButtonBank();    // Bank 2: BOOST / INVERT / CLEAR
        setupPerformanceButtonBank(); // Bank 3: PANIC / LATCH / BYPASS

        // Feature 3: wire the MIDI learn entry callback from GestureButtonBar.
        // When the user right-clicks a button, onEnterLearnMode fires here and
        // we forward it to the GestureButtonMidiLearnManager.
        gestureButtons_.onEnterLearnMode = [this](int slot)
        {
            midiLearnMgr_.enterLearnMode(slot);
            // Visual: bar already shows "LEARN..." via learnSlot_; no further action needed.
        };

        // WCAG Fix 3 (resolved): sync reduced-motion flag from system accessibility
        // setting so the planchette skips Lissajous drift when the user has enabled
        // Settings > Accessibility > Reduce Motion.
        syncReducedMotionFromSystem();

        setWantsKeyboardFocus(true);

        // ── Cache marker fonts (Fix #10: avoid per-paint Font construction) ──
        // Sizes = kBaseFontSize (11.0) * sizeFactor from markerProperties()
        constexpr float kBaseFontSize = 11.0f;
        constexpr float kSizeFactors[4] = {1.00f, 0.85f, 0.70f, 0.55f};
        for (int i = 0; i < 4; ++i)
            markerFonts_[i] = GalleryFonts::body(kBaseFontSize * kSizeFactors[i]).withStyle(juce::Font::italic);
    }

    ~XOuijaPanel() override = default;

    // Callbacks (set by owner before displaying)
    //==========================================================================

    /** Called when the user drags on the surface.
        circleX in [0,1] (0=Gb, 0.5=C, 1=F#), influenceY in [0,1] (0=NO, 1=YES). */
    std::function<void(float circleX, float influenceY)> onPositionChanged;

    /** Called to emit a MIDI CC value (cc 85 = circleX, cc 86 = influenceY). */
    std::function<void(uint8_t cc, uint8_t value)> onCCOutput;

    /** Placeholder — wired by Task 7 to trigger GOODBYE gesture. */
    std::function<void()> onGoodbye;

    // B043 — Trail-to-DSP wiring
    // Fired in mouseDrag after each trail point is recorded with updated
    // TrailModulator values. Wire in PlaySurface to forward trail_length and
    // trail_velocity to actual AudioProcessorParameter objects. Example:
    //
    //   xouijaPanel_.onTrailModulatorChanged = [this](const TrailModulator& mod) {
    //       if (processor_) {
    //           if (auto* p = processor_->getAPVTS().getParameter("macro4"))
    //               p->setValueNotifyingHost(mod.trail_length);
    //       }
    //   };
    std::function<void(const TrailModulator&)> onTrailModulatorChanged;

    // Wave 5 D2 — Mood Sliders + Heatmap
    // Fired when the user moves any of the three mood sliders.
    // The host (PlaySurface) should forward the values to the APVTS parameters
    // xouija_brightness / xouija_tension / xouija_density so that DAW automation
    // and session recall work.
    //
    // Example wiring in PlaySurface::connectXOuija():
    //
    //   xouijaPanel_.onMoodChanged = [this](const MoodSliderBar::MoodValues& m) {
    //       auto& apvts = processor_->getAPVTS();
    //       if (auto* p = apvts.getParameter("xouija_brightness"))
    //           p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(m.brightness));
    //       if (auto* p = apvts.getParameter("xouija_tension"))
    //           p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(m.tension));
    //       if (auto* p = apvts.getParameter("xouija_density"))
    //           p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(m.density));
    //   };
    //
    std::function<void(const MoodSliderBar::MoodValues&)> onMoodChanged;

    //==========================================================================
    // Public state accessors
    //==========================================================================

    /** Normalised horizontal circle-of-fifths position in [0,1]. */
    float getCirclePosition() const noexcept { return circleX_; }

    /** Normalised influence depth in [0,1]. */
    float getInfluenceDepth() const noexcept { return influenceY_; }

    /** Semitone of the nearest key (0=C, 1=C#, … 11=B). */
    int getCurrentKey() const noexcept { return HarmonicField::positionToKey(circleX_); }

    /** True while the user finger / mouse button is down. */
    bool isTouching() const noexcept { return touching_; }

    //==========================================================================
    // Setters (for remote CC / automation input)
    //==========================================================================

    void setAccentColour(juce::Colour c)
    {
        accentColour_ = c;
        planchette_.setAccentColour(c);
        gestureButtons_.setAccentColour(c);
        moodSliderBar_.setAccentColour(c);
        repaint();
    }

    juce::Colour getAccentColour() const noexcept { return accentColour_; }

    /** Switch the GestureButtonBar to a named bank and load its button definitions.
        Banks:
          XOuija     — FREEZE / HOME / DRIFT  (XOuija-specific; default)
          Dub        — LOOP / MUTE / RESET    (DubSpace strip mode helpers)
          Coupling   — BOOST / INVERT / CLEAR (Coupling strip mode helpers)
          Performance— PANIC / LATCH / BYPASS (global performance emergencies)
        Respects the GestureButtonBar's bank-lock pin: if locked, this is a no-op. */
    void activateGestureBank(GestureButtonBar::Bank bank) { switchGestureBank(bank); }

    /** Propagate a reduced-motion preference to the planchette's animation path.
        Call this once after construction (and again if the system setting changes).
        Reads the system "Reduce Motion" accessibility setting when no argument is
        supplied — macOS: CFPreferences "reduceMotion" / com.apple.universalaccess;
        iOS: wired via A11y::prefersReducedMotion() in GalleryColors.h. */
    void setReducedMotion(bool on) { planchette_.setReducedMotion(on); }

    /** Query the OS and update the planchette's reduced-motion flag accordingly.
        Delegates to A11y::prefersReducedMotion() — the unified platform helper in
        GalleryColors.h — rather than reading CFPreferences directly (#223).
        Covers macOS ("reduceMotion" / com.apple.universalaccess) and the in-app
        SettingsPanel override. iOS port archived 2026-03-26 (#1107). */
    void syncReducedMotionFromSystem() { planchette_.setReducedMotion(A11y::prefersReducedMotion()); }

    /** Drive circleX from an external source (e.g. incoming CC 85). */
    void setCirclePosition(float x)
    {
        circleX_ = juce::jlimit(0.0f, 1.0f, x);
        repaint();
    }

    /** Drive influenceY from an external source (e.g. incoming CC 86). */
    void setInfluenceDepth(float y)
    {
        influenceY_ = juce::jlimit(0.0f, 1.0f, y);
        repaint();
    }

    //==========================================================================
    // Feature 2 (B043): Trail modulator accessor
    //   Returns the most-recently-computed trail modulation values.
    //   Updated in mouseDrag; call getTrailModulator() at any time from the
    //   UI thread to read current trail_length and trail_velocity.
    //==========================================================================

    const TrailModulator& getTrailModulator() const noexcept { return trailModulator_; }

    //==========================================================================
    // Preset sensitivity hint type — used by setPresetHint() below.
    // Defined here (before first use) so the method signature compiles;
    // the member variable presetHint_ is declared later in the private section.
    //==========================================================================
    struct SensitivityHint
    {
        // Per-macro sweep range (|depthMax - depthMin| from PresetMacroTarget).
        // 0 = macro wired to no targets (dead knob), 1 = full-range sweep.
        float macroRange[4] = {0.5f, 0.5f, 0.5f, 0.5f};

        // 6D Sonic DNA (0-1 each), sourced from PresetDNA.
        float dnaMovement   = 0.5f; // weights X-axis sensitivity contribution
        float dnaSpace      = 0.5f; // weights Y-axis sensitivity contribution
        float dnaAggression = 0.5f; // sharpens the overall contrast of the map

        // True when a real preset hint has been pushed (not default-constructed).
        bool hasRealData = false;
    };

    //==========================================================================
    // Feature 4 (B044): Preset sensitivity hint — drives real parameter
    // sensitivity computation in recomputeSensitivityMap().
    //
    // Call this from the PlaySurface (or PresetManager::Listener::presetLoaded)
    // after each preset load.  Example integration:
    //
    //   void presetLoaded(const PresetData& p) override
    //   {
    //       XOuijaPanel::SensitivityHint hint;
    //       for (int m = 0; m < 4; ++m)
    //       {
    //           float totalRange = 0.0f;
    //           for (auto& t : p.macroTargets[m])
    //               totalRange += std::abs(t.depthMax - t.depthMin);
    //           // Normalise: saturate at 4× unitary range
    //           hint.macroRange[m] = juce::jlimit(0.0f, 1.0f, totalRange / 4.0f);
    //       }
    //       hint.dnaMovement   = p.dna.movement;
    //       hint.dnaSpace      = p.dna.space;
    //       hint.dnaAggression = p.dna.aggression;
    //       hint.hasRealData   = true;
    //       xouijaPanel_.setPresetHint(hint);
    //   }
    //==========================================================================

    void setPresetHint(const SensitivityHint& hint)
    {
        presetHint_ = hint;
        sensitivityMapDirty_ = true;
        repaint();
    }

    //==========================================================================
    // Wave 5 D2: Mood state API
    //
    // setMoodState() — push new mood values (e.g. from APVTS parameter changes)
    //   without triggering the onMoodChanged callback (host→panel direction).
    //   The heatmap overlay is recomputed and the panel repainted.
    //
    // setHeatmapVisible() — show/hide the mood heatmap overlay.
    //   The MoodSliderBar is always visible; the overlay toggle only affects the
    //   coloured heatmap drawn over the harmonic surface.
    //
    // getMoodState() — read current mood values (e.g. for APVTS sync on restore).
    //==========================================================================
    void setMoodState(const MoodSliderBar::MoodValues& v)
    {
        moodState_ = v;
        moodState_.brightness = juce::jlimit(0.0f, 1.0f, moodState_.brightness);
        moodState_.tension    = juce::jlimit(0.0f, 1.0f, moodState_.tension);
        moodState_.density    = juce::jlimit(0.0f, 1.0f, moodState_.density);
        moodSliderBar_.setMoodValues(moodState_);
        moodHeatmapDirty_ = true;
        repaint();
    }

    const MoodSliderBar::MoodValues& getMoodState() const noexcept { return moodState_; }

    void setHeatmapVisible(bool visible)
    {
        heatmapVisible_ = visible;
        repaint();
    }

    bool isHeatmapVisible() const noexcept { return heatmapVisible_; }

    //==========================================================================
    // Feature 3: MIDI message processing for gesture button MIDI learn.
    //   Call this from PlaySurface::handleMidiMessage() (or equivalent) whenever
    //   a MIDI message arrives on the message thread. When a CC arrives and
    //   matches a button mapping (or is being learned), the gesture button action
    //   is fired and/or the learn mapping is stored.
    //
    //   The active bank's button actions are used — so buttons fire regardless
    //   of which bank is currently displayed (all banks are checked for CC hits).
    //
    //   Returns true if the message was consumed.
    //==========================================================================

    bool processMidiMessage(const juce::MidiMessage& msg)
    {
        if (!msg.isController())
            return false;

        const int slot = midiLearnMgr_.processMidi(msg);

        // If learn mode captured a CC, update the visual state on the bar
        if (!midiLearnMgr_.isLearning())
            gestureButtons_.setLearnSlot(-1); // exit visual learn mode

        if (slot >= 0)
        {
            // Fire the action for the active bank's button at this slot
            const auto bank = gestureButtons_.getBank();
            const auto& defs = bankDefs_[static_cast<std::size_t>(bank)];
            const auto& btn = defs[static_cast<std::size_t>(slot)];
            if (btn.action)
                btn.action();
            return true;
        }

        return false;
    }

    //==========================================================================
    // Feature 1: ValueTree state persistence (active bank, toggle states,
    // MIDI learn CC mappings, position).
    // Called from PlaySurface/editor to participate in DAW session recall.
    //==========================================================================

    juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree("XOuijaPanel");

        // Circle and influence position
        tree.setProperty("circleX", static_cast<double>(circleX_), nullptr);
        tree.setProperty("influenceY", static_cast<double>(influenceY_), nullptr);

        // Drift enabled state
        tree.setProperty("driftEnabled", planchette_.isDriftEnabled(), nullptr);

        // Toggle flags for stateful buttons across all banks
        tree.setProperty("dubLoopActive", dubLoopActive_, nullptr);
        tree.setProperty("dubMuteActive", dubMuteActive_, nullptr);
        tree.setProperty("couplingInverted", couplingInverted_, nullptr);
        tree.setProperty("perfLatchActive", perfLatchActive_, nullptr);
        tree.setProperty("perfBypassActive", perfBypassActive_, nullptr);

        // GestureButtonBar bank + per-button toggle state
        tree.appendChild(gestureButtons_.toValueTree(), nullptr);

        // MIDI learn CC mappings
        tree.appendChild(midiLearnMgr_.toValueTree(), nullptr);

        // Wave 5 D2: mood state + heatmap visibility
        tree.setProperty("moodBrightness", static_cast<double>(moodState_.brightness), nullptr);
        tree.setProperty("moodTension",    static_cast<double>(moodState_.tension),    nullptr);
        tree.setProperty("moodDensity",    static_cast<double>(moodState_.density),    nullptr);
        tree.setProperty("heatmapVisible", heatmapVisible_, nullptr);

        return tree;
    }

    void fromValueTree(const juce::ValueTree& tree)
    {
        if (!tree.isValid() || !tree.hasType("XOuijaPanel"))
            return;

        // Restore circle/influence position
        circleX_ = juce::jlimit(0.0f, 1.0f, static_cast<float>(static_cast<double>(tree["circleX"])));
        influenceY_ = juce::jlimit(0.0f, 1.0f, static_cast<float>(static_cast<double>(tree["influenceY"])));

        // Restore drift
        const bool drift = static_cast<bool>(tree["driftEnabled"]);
        planchette_.setDriftEnabled(drift);

        // Restore toggle flags
        dubLoopActive_ = static_cast<bool>(tree["dubLoopActive"]);
        dubMuteActive_ = static_cast<bool>(tree["dubMuteActive"]);
        couplingInverted_ = static_cast<bool>(tree["couplingInverted"]);
        perfLatchActive_ = static_cast<bool>(tree["perfLatchActive"]);
        perfBypassActive_ = static_cast<bool>(tree["perfBypassActive"]);

        // Restore GestureButtonBar state (bank + per-button toggles)
        auto barTree = tree.getChildWithName("GestureButtonBar");
        if (barTree.isValid())
        {
            gestureButtons_.fromValueTree(barTree);

            // Re-apply the bank's button definitions so that buttons_ is current
            const auto bank = gestureButtons_.getBank();
            gestureButtons_.setButtons(bankDefs_[static_cast<std::size_t>(bank)]);
        }

        // Sync button toggle visuals to the GestureButtonBar after restore
        syncToggleStatesToBar();

        // Restore MIDI learn CC mappings
        auto midiTree = tree.getChildWithName("GestureButtonMidiLearn");
        if (midiTree.isValid())
            midiLearnMgr_.fromValueTree(midiTree);

        updatePlanchetteText();

        // Sensitivity map must be recomputed after a preset load because the
        // new preset's parameter mapping may differ from the previous one.
        sensitivityMapDirty_ = true;

        // Wave 5 D2: restore mood state + heatmap visibility
        if (tree.hasProperty("moodBrightness"))
        {
            MoodSliderBar::MoodValues m;
            m.brightness = juce::jlimit(0.0f, 1.0f,
                               static_cast<float>(static_cast<double>(tree["moodBrightness"])));
            m.tension    = juce::jlimit(0.0f, 1.0f,
                               static_cast<float>(static_cast<double>(tree["moodTension"])));
            m.density    = juce::jlimit(0.0f, 1.0f,
                               static_cast<float>(static_cast<double>(tree["moodDensity"])));
            moodState_ = m;
            moodSliderBar_.setMoodValues(moodState_);
            moodHeatmapDirty_ = true;
        }
        if (tree.hasProperty("heatmapVisible"))
            heatmapVisible_ = static_cast<bool>(tree["heatmapVisible"]);

        repaint();
    }

    //==========================================================================
    // Component overrides
    //==========================================================================

    void resized() override
    {
        // Compute harmonic surface area — the area used for marker layout
        // excludes the three reserved strips at the bottom.
        auto b = getLocalBounds();
        const int reservedBottom = kGestureBarH + kGoodbyeH + kMoodBarH; // 108px

        harmonicSurfaceBounds_ = b.withTrimmedBottom(reservedBottom);

        // GOODBYE button: 32px at the very bottom, full width
        goodbyeButton_.setBounds(b.removeFromBottom(kGoodbyeH));

        // GestureButtonBar: 34px above GOODBYE, full width
        gestureButtons_.setBounds(b.removeFromBottom(kGestureBarH));

        // Wave 5 D2: MoodSliderBar: 42px above GestureButtonBar, full width
        moodSliderBar_.setBounds(b.removeFromBottom(kMoodBarH));

        // Planchette manages its own setBounds() inside timerCallback via
        // updateBounds(). No explicit positioning needed here.
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // ------------------------------------------------------------------
        // 1. Background: dark rounded rect
        // ------------------------------------------------------------------
        g.setColour(juce::Colour(0xFF1e1e22));
        g.fillRoundedRectangle(bounds, 8.0f);

        // ------------------------------------------------------------------
        // 2. Noise texture at opacity 0.05
        // ------------------------------------------------------------------
        if (noiseImage_.isValid())
        {
            g.setOpacity(0.05f);
            // Tile the 128x128 image across the component
            g.drawImage(noiseImage_, bounds, juce::RectanglePlacement::fillDestination, false);
            g.setOpacity(1.0f);
        }

        // ------------------------------------------------------------------
        // 3. Parameter sensitivity map overlay at 8% white opacity.
        //    Lazy recompute: regenerate if the map is stale (e.g. after a
        //    preset load or on first paint).
        // ------------------------------------------------------------------
        if (sensitivityMapDirty_)
            recomputeSensitivityMap();

        if (sensitivityMap_.isValid())
        {
            g.setOpacity(0.08f);
            g.drawImage(sensitivityMap_, bounds, juce::RectanglePlacement::fillDestination, false);
            g.setOpacity(1.0f);
        }

        // ------------------------------------------------------------------
        // 4. Wave 5 D2: Mood heatmap overlay
        //    Renders a colour-coded heat over the harmonic surface showing
        //    which cells respond most strongly to the current mood settings.
        //    Drawn at 30% opacity so it complements but doesn't obscure
        //    the sensitivity map or markers.
        // ------------------------------------------------------------------
        if (heatmapVisible_)
            paintMoodHeatmap(g);

        // ------------------------------------------------------------------
        // 5. Circle-of-fifths markers
        // ------------------------------------------------------------------
        paintMarkers(g);

        // ------------------------------------------------------------------
        // 6. YES / NO labels
        // ------------------------------------------------------------------
        paintYesNoLabels(g);

        // ------------------------------------------------------------------
        // 7. Bioluminescent gesture trail (spec Section 4.5)
        //    Rendered after static elements, below planchette (child component).
        // ------------------------------------------------------------------
        paintTrail(g);
    }

    //==========================================================================
    // Mouse handling
    //==========================================================================

    void mouseDown(const juce::MouseEvent& e) override
    {
        touching_ = true;
        auto [nx, ny] = mouseToNormalized(e);
        circleX_ = nx;
        influenceY_ = ny;

        // Reset trail tracking so the first drag step doesn't record a giant jump
        lastTrailPixelX_ = e.position.x;
        lastTrailPixelY_ = e.position.y;

        planchette_.springTo(nx, ny);
        updatePlanchetteText();

        repaint();
        fireCallbacks();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        auto [nx, ny] = mouseToNormalized(e);
        circleX_ = nx;
        influenceY_ = ny;

        // Record trail point with distance-based spacing (~4px minimum)
        const float dx = e.position.x - lastTrailPixelX_;
        const float dy = e.position.y - lastTrailPixelY_;
        if (dx * dx + dy * dy >= 16.0f) // 4px distance threshold (squared)
        {
            const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
            const float vel = std::clamp(std::sqrt(dx * dx + dy * dy) / 50.0f, 0.0f, 1.0f);
            trailBuffer_.push(circleX_, influenceY_, vel, now);
            lastTrailPixelX_ = e.position.x;
            lastTrailPixelY_ = e.position.y;

            // B043 — Update TrailModulator and fire callback for DSP wiring.
            updateTrailModulator();
        }

        planchette_.moveTo(nx, ny);
        updatePlanchetteText();

        repaint();
        fireCallbacks();
    }

    void mouseUp(const juce::MouseEvent& /*e*/) override
    {
        touching_ = false;
        planchette_.release();
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(key.getTextCharacter())));

        if (c == 'G')
        {
            // Trigger GOODBYE action directly (G for GOODBYE)
            if (goodbyeButton_.onGoodbye)
                goodbyeButton_.onGoodbye();
            return true;
        }

        return gestureButtons_.handleKey(c);
    }

private:
    //==========================================================================
    // State
    //==========================================================================
    float circleX_ = 0.5f;    // 0=Gb, 0.5=C, 1.0=F#
    float influenceY_ = 0.0f; // 0=NO, 1=YES
    bool touching_ = false;
    juce::Colour accentColour_;

    juce::Image noiseImage_;
    juce::Rectangle<int> harmonicSurfaceBounds_;

    // B042 — Planchette child component
    Planchette planchette_;

    // B043 — Gesture trail ring buffer + last-recorded pixel position
    GestureTrailBuffer trailBuffer_;
    float lastTrailPixelX_ = 0.0f;
    float lastTrailPixelY_ = 0.0f;

    // B043 — Trail modulator (computed in mouseDrag, read by onTrailModulatorChanged)
    TrailModulator trailModulator_;

    // Feature 3 — MIDI learn manager for gesture buttons
    GestureButtonMidiLearnManager midiLearnMgr_;

    // Task 7 — Gesture button bar and GOODBYE button
    GestureButtonBar gestureButtons_;
    GoodbyeButton goodbyeButton_;

    // Wave 5 D2 — Mood sliders + heatmap
    MoodSliderBar          moodSliderBar_;
    MoodSliderBar::MoodValues moodState_;      // current mood (brightness/tension/density)
    bool                   heatmapVisible_ = true;   // toggle for the overlay
    juce::Image            moodHeatmapImage_;         // 64×64 ARGB, coloured heatmap
    bool                   moodHeatmapDirty_ = true;  // triggers lazy recompute

    // Per-bank button definitions: indexed by GestureButtonBar::Bank enum.
    // Populated in setupDefaultButtonBank() / setupDubButtonBank() /
    // setupCouplingButtonBank() / setupPerformanceButtonBank().
    // Active bank is loaded into gestureButtons_ by switchGestureBank().
    std::array<std::array<GestureButtonBar::ButtonDef, 3>, 4> bankDefs_;

    // Stateful toggle flags for Dub bank buttons
    bool dubLoopActive_ = false;
    bool dubMuteActive_ = false;

    // Stateful toggle flags for Coupling bank buttons
    bool couplingInverted_ = false;

    // Stateful toggle flags for Performance bank buttons
    bool perfLatchActive_ = false;
    bool perfBypassActive_ = false;

    // ── Cached marker fonts (Fix #10: avoid per-paint Font construction) ─────
    // 4 size brackets corresponding to HarmonicField::markerProperties() output:
    //   [0] dist=0     → sizeFactor 1.00 → 11.0px  (home key)
    //   [1] dist=1-2   → sizeFactor 0.85 → 9.35px
    //   [2] dist=3-4   → sizeFactor 0.70 → 7.70px
    //   [3] dist=5-6   → sizeFactor 0.55 → 6.05px
    // Initialized in the constructor body after member initializers.
    juce::Font markerFonts_[4];

    // ── Parameter Sensitivity Map ─────────────────────────────────────────────
    // 64×64 single-channel luminance texture precomputed on preset load.
    // Rendered as a white overlay at 8% opacity over the harmonic surface,
    // giving the performer a visual hint of which XY regions drive the most
    // parameter movement for the current preset.
    //
    // sensitivityMapDirty_ is set true on construction and whenever a preset
    // is loaded (fromValueTree). recomputeSensitivityMap() is called lazily
    // on the first paint() call after the flag is raised.
    juce::Image sensitivityMap_;        // 64×64 ARGB luminance texture (white pixels, alpha = sensitivity)
    bool sensitivityMapDirty_ = true;   // triggers recompute on next paint()

    // ── Preset Sensitivity Hint ───────────────────────────────────────────────
    // Pushed by the host (PlaySurface / PresetManager listener) after each
    // preset load via setPresetHint().  Drives real parameter sensitivity
    // computation in recomputeSensitivityMap().
    // (SensitivityHint struct is defined in the public section above.)
    SensitivityHint presetHint_;

    //==========================================================================
    // Note names for planchette display text (12-entry, indexed by semitone)
    //==========================================================================
    static constexpr const char* kDisplayNoteNames[12] = {"C",  "C#", "D",  "Eb", "E",  "F",
                                                          "F#", "G",  "Ab", "A",  "Bb", "B"};

    //==========================================================================
    // Helper: format "Key · Pct%" and push to planchette
    //==========================================================================
    void updatePlanchetteText()
    {
        const int key = HarmonicField::positionToKey(circleX_);
        const int pct = juce::roundToInt(influenceY_ * 100.0f);
        const juce::String noteName(kDisplayNoteNames[key]);
        planchette_.setDisplayText(noteName + " \xc2\xb7 " + juce::String(pct) + "%");
    }

    //==========================================================================
    // B043 — Update TrailModulator from current trailBuffer_ state.
    // Computes trail_length (fill ratio) and trail_velocity (mean recent velocity).
    // Called from mouseDrag after each new trail point is recorded.
    // Fires onTrailModulatorChanged if set.
    //==========================================================================
    void updateTrailModulator()
    {
        const int n = trailBuffer_.count();

        // trail_length: normalized fill level (0 = empty, 1 = all 256 filled)
        trailModulator_.trail_length = static_cast<float>(n) / static_cast<float>(GestureTrailBuffer::kBufferSize);

        // trail_velocity: mean of the most recent ≤8 trail point velocities
        const int sampleCount = std::min(n, 8);
        if (sampleCount > 0)
        {
            float velSum = 0.0f;
            for (int age = 0; age < sampleCount; ++age)
                velSum += trailBuffer_.pointByAge(age).velocity;
            trailModulator_.trail_velocity = velSum / static_cast<float>(sampleCount);
        }
        else
        {
            trailModulator_.trail_velocity = 0.0f;
        }

        if (onTrailModulatorChanged)
            onTrailModulatorChanged(trailModulator_);
    }

    //==========================================================================
    // Feature 1: Sync per-bank button toggle states to GestureButtonBar visuals.
    // Call after fromValueTree() to ensure the bar reflects restored state.
    // Mapping: each bank has up to 2 toggle buttons; indices are bank-specific.
    //   XOuija   [0]=FREEZE, [2]=DRIFT   (drift stored via planchette_.isDriftEnabled)
    //   Dub      [0]=LOOP,   [1]=MUTE
    //   Coupling [1]=INVERT
    //   Perf     [1]=LATCH,  [2]=BYPASS
    //==========================================================================
    void syncToggleStatesToBar()
    {
        const auto bank = gestureButtons_.getBank();

        // Reset all three slots first
        gestureButtons_.setButtonToggleState(0, false);
        gestureButtons_.setButtonToggleState(1, false);
        gestureButtons_.setButtonToggleState(2, false);

        switch (bank)
        {
        case GestureButtonBar::Bank::XOuija:
            gestureButtons_.setButtonToggleState(0, trailBuffer_.isFrozen());
            gestureButtons_.setButtonToggleState(2, planchette_.isDriftEnabled());
            break;

        case GestureButtonBar::Bank::Dub:
            gestureButtons_.setButtonToggleState(0, dubLoopActive_);
            gestureButtons_.setButtonToggleState(1, dubMuteActive_);
            break;

        case GestureButtonBar::Bank::Coupling:
            gestureButtons_.setButtonToggleState(1, couplingInverted_);
            break;

        case GestureButtonBar::Bank::Performance:
            gestureButtons_.setButtonToggleState(1, perfLatchActive_);
            gestureButtons_.setButtonToggleState(2, perfBypassActive_);
            break;
        }
    }

    //==========================================================================
    // Noise texture generation (deterministic, seed 42)
    //==========================================================================
    void generateNoiseTexture()
    {
        constexpr int kNoiseW = 128;
        constexpr int kNoiseH = 128;

        noiseImage_ = juce::Image(juce::Image::ARGB, kNoiseW, kNoiseH, true);

        std::mt19937 rng(42u);
        std::uniform_int_distribution<int> dist(0, 255);

        juce::Image::BitmapData bmp(noiseImage_, juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < kNoiseH; ++y)
        {
            for (int x = 0; x < kNoiseW; ++x)
            {
                const uint8_t v = static_cast<uint8_t>(dist(rng));
                // ARGB: full alpha, same value for R/G/B (grayscale)
                bmp.setPixelColour(x, y, juce::Colour(static_cast<uint8_t>(255), v, v, v));
            }
        }
    }

    //==========================================================================
    // Wave 5 D2: Mood Heatmap
    //
    // Precomputes a 64×64 ARGB texture that colour-codes each point on the
    // harmonic surface according to its "affinity" for the current mood vector
    // (brightness, tension, density).
    //
    // Heatmap colour model:
    //   Each grid cell has an implicit texture vector derived from its XY
    //   position on the circle-of-fifths surface:
    //     cellBrightness  ≈ horizontal X position (left=dark, right=bright)
    //     cellTension     ≈ vertical Y position   (bottom=calm, top=tense)
    //     cellDensity     ≈ radial distance from centre (near=sparse, far=dense)
    //
    //   Affinity (heat) = 1 − Euclidean distance(cellVector, moodVector)
    //   Clamped to [0, 1].
    //
    //   Heat colour mapping:
    //     heat 0.0  → transparent (no overlay)
    //     heat 0.5  → XO Gold tint at 15% alpha
    //     heat 1.0  → XO Gold tint at 60% alpha
    //
    // When D1 (per-cell texture vectors) is merged, replace the implicit
    // position-derived cellVector with the per-cell stored vector from D1's
    // data structure.  The heat calculation below is designed to be a drop-in
    // replacement: swap the cellBrightness/cellTension/cellDensity lines.
    //
    // Performance: 64×64 float buffer (~16 KB).  Full recompute takes <0.3 ms
    // on an M1 Mac in Debug.  Triggered lazily on next paint() after any mood
    // slider drag or setMoodState() call.
    //==========================================================================
    void recomputeMoodHeatmap()
    {
        constexpr int kW = 64;
        constexpr int kH = 64;

        float buf[kW * kH];

        const float mB = moodState_.brightness; // [0,1]
        const float mT = moodState_.tension;     // [0,1]
        const float mD = moodState_.density;     // [0,1]

        for (int row = 0; row < kH; ++row)
        {
            // ny: 0=top (high influence/tense), 1=bottom (low influence/calm)
            // Invert so cellTension increases toward top (higher Y on screen = more tense)
            const float ny = static_cast<float>(row) / static_cast<float>(kH - 1);

            for (int col = 0; col < kW; ++col)
            {
                const float nx = static_cast<float>(col) / static_cast<float>(kW - 1);

                // ── Implicit cell texture vector (placeholder for D1 data) ──
                // D1 will replace these three lines with per-cell stored vectors.
                const float cellBrightness = nx;                               // left=dark, right=bright
                const float cellTension    = 1.0f - ny;                       // top=tense, bottom=calm
                const float cellDensity    = 2.0f * std::sqrt((nx - 0.5f) * (nx - 0.5f)
                                                             + (ny - 0.5f) * (ny - 0.5f));  // radial [0,√2] → clamp to [0,1]
                const float cellDensityClamped = juce::jlimit(0.0f, 1.0f, cellDensity);

                // Euclidean distance in mood-space [0, √3]
                const float dB = cellBrightness  - mB;
                const float dT = cellTension     - mT;
                const float dD = cellDensityClamped - mD;
                const float dist = std::sqrt(dB * dB + dT * dT + dD * dD);

                // Affinity: 1 at exact match, 0 at maximum distance (√3 ≈ 1.73)
                constexpr float kMaxDist = 1.732f; // √3
                const float heat = juce::jlimit(0.0f, 1.0f, 1.0f - dist / kMaxDist);

                buf[row * kW + col] = heat;
            }
        }

        // 3×3 box blur for smooth edges
        float blurred[kW * kH];
        for (int row = 0; row < kH; ++row)
        {
            for (int col = 0; col < kW; ++col)
            {
                float sum = 0.0f;
                int   cnt = 0;
                for (int dr = -1; dr <= 1; ++dr)
                {
                    const int nr = juce::jlimit(0, kH - 1, row + dr);
                    for (int dc = -1; dc <= 1; ++dc)
                    {
                        const int nc = juce::jlimit(0, kW - 1, col + dc);
                        sum += buf[nr * kW + nc];
                        ++cnt;
                    }
                }
                blurred[row * kW + col] = sum / static_cast<float>(cnt);
            }
        }

        // Encode as ARGB: hue = XO Gold tint, alpha encodes heat
        // heat=0 → alpha=0 (transparent), heat=1 → alpha=153 (60% of 255)
        // We use the accentColour_ so engine-specific accent colors tint the heatmap.
        moodHeatmapImage_ = juce::Image(juce::Image::ARGB, kW, kH, true);
        {
            const float r = accentColour_.getFloatRed();
            const float gr= accentColour_.getFloatGreen();
            const float bl= accentColour_.getFloatBlue();

            juce::Image::BitmapData data(moodHeatmapImage_, juce::Image::BitmapData::writeOnly);
            for (int row = 0; row < kH; ++row)
            {
                for (int col = 0; col < kW; ++col)
                {
                    const float heat = blurred[row * kW + col];
                    // Alpha: 0 at heat=0, up to 153 (~60%) at heat=1.0
                    const uint8_t alpha = static_cast<uint8_t>(heat * 153.0f);
                    const uint8_t rc    = static_cast<uint8_t>(r  * 255.0f);
                    const uint8_t gc    = static_cast<uint8_t>(gr * 255.0f);
                    const uint8_t bc    = static_cast<uint8_t>(bl * 255.0f);
                    data.setPixelColour(col, row, juce::Colour::fromRGBA(rc, gc, bc, alpha));
                }
            }
        }

        moodHeatmapDirty_ = false;
    }

    //==========================================================================
    // Wave 5 D2: paintMoodHeatmap — render the heatmap over the harmonic surface.
    // Drawn at 30% total opacity so the coloring is clear but not blinding.
    // The image itself encodes alpha via the heat value; the outer 30% opacity
    // further attenuates the full image (effective peak = 60% × 30% = 18%).
    //==========================================================================
    void paintMoodHeatmap(juce::Graphics& g)
    {
        if (moodHeatmapDirty_)
            recomputeMoodHeatmap();

        if (!moodHeatmapImage_.isValid())
            return;

        const auto surfaceBounds = harmonicSurfaceBounds_.toFloat();
        g.setOpacity(0.30f);
        g.drawImage(moodHeatmapImage_, surfaceBounds,
                    juce::RectanglePlacement::fillDestination, false);
        g.setOpacity(1.0f);
    }

    //==========================================================================
    // Parameter Sensitivity Map
    //
    // Precomputes a 64×64 luminance texture that visualises how strongly each
    // XY region of the performance surface affects synthesiser parameters for
    // the current preset.  Rendered as a semi-transparent white overlay at 8%
    // opacity in paint().
    //
    // When a real preset hint is available (presetHint_.hasRealData == true),
    // the map is derived from actual macro parameter ranges and 6D Sonic DNA:
    //
    //   X axis drives TONE   (macro[0]) + TIDE   (macro[1]).  [D11]
    //   Y axis drives COUPLE (macro[2]) + DEPTH  (macro[3]).  [D11]
    //
    // For each pixel at normalised position (nx, ny):
    //
    //   xSens(nx)  — how much TONE + TIDE change at this X position.
    //                Peaks near the centre of the circle of fifths where harmonic
    //                tension is highest; the peak shifts with circleX_ (the current
    //                key), matching the HarmonicField marker layout.
    //                Scaled by macroRange[0] + macroRange[1] and dnaMovement.
    //
    //   ySens(ny)  — how much COUPLE + DEPTH change at this Y position.
    //                Peaks at both extremes (NO=0, YES=1) because influence depth
    //                has maximum effect at the poles.
    //                Scaled by macroRange[2] + macroRange[3] and dnaSpace.
    //
    //   sensitivity = lerp(xSens, ySens, axisBalance)
    //                 where axisBalance shifts toward X when movement DNA is high
    //                 and toward Y when space DNA is high.
    //
    //   contrast amplifier: dnaAggression sharpens the map (power curve) so
    //                 high-aggression presets show a crisper hot-zone.
    //
    // A single-pass 3×3 box blur ensures smooth, edge-free output.
    //
    // Fallback: when no real preset data is available the method uses the
    // original radial-heuristic shape (identical to the V1 scaffold) so the
    // panel always renders a plausible-looking overlay.
    //
    // Performance: all arithmetic is on a 64×64 float buffer (~16 KB).
    // Measured at <0.4 ms on an M1 Mac in Debug builds.
    //==========================================================================
    void recomputeSensitivityMap()
    {
        constexpr int kW = 64;
        constexpr int kH = 64;

        // ── Step 1: Allocate raw float buffer ────────────────────────────────
        // Stack-allocated; 64×64 floats = 16 KB — well within stack limits.
        float buf[kW * kH];

        // ── Step 2: Fill buffer with sensitivity values ───────────────────────
        if (presetHint_.hasRealData)
        {
            // ----------------------------------------------------------------
            // REAL ANALYSIS PATH
            // ----------------------------------------------------------------

            // X-axis contribution weights: TONE + TIDE macro range widths, [D11]
            // further scaled by the movement DNA dimension.
            // A dead macro (range=0) contributes nothing; a full-sweep macro (range=1)
            // contributes maximally.
            const float xMacroStrength = (presetHint_.macroRange[0] + presetHint_.macroRange[1])
                                         * (0.5f + presetHint_.dnaMovement * 0.5f); // [0, 2] * [0.5, 1]

            // Y-axis contribution weights: COUPLE + DEPTH macro range widths, [D11]
            // further scaled by the space DNA dimension.
            const float yMacroStrength = (presetHint_.macroRange[2] + presetHint_.macroRange[3])
                                         * (0.5f + presetHint_.dnaSpace * 0.5f);

            // Axis balance: 0 = pure X, 1 = pure Y.
            // When both are equal we blend 50/50.  Clamp to avoid a fully-black map
            // (total = 0 should never happen in practice, but guard defensively).
            const float totalStrength = xMacroStrength + yMacroStrength;
            const float axisBalance   = (totalStrength > 1e-6f)
                                        ? (yMacroStrength / totalStrength)
                                        : 0.5f; // equal blend if all macros are dead

            // Normalised X-axis contribution magnitudes (saturate to [0, 1]).
            const float xNorm = juce::jlimit(0.0f, 1.0f, xMacroStrength * 0.5f);
            const float yNorm = juce::jlimit(0.0f, 1.0f, yMacroStrength * 0.5f);

            // Key-biased centre: circleX_ shifts the X hot-zone in [0.3, 0.7].
            // This matches the HarmonicField marker arc so the glow follows
            // whichever key the planchette is currently closest to.
            const float hotX = 0.3f + circleX_ * 0.4f;

            // Y hot-zones sit at both poles (influenceY_ = 0 and 1) because
            // the COUPLE / DEPTH macros respond most at full-on and full-off [D11]
            // influence depth.  We model this as two Gaussian peaks at y=0 and y=1.
            // Their relative weights follow influenceY_ so the current position
            // pulls the glow toward whichever pole is active.
            const float yPoleLow  = 1.0f - influenceY_; // weight for the NO=0 pole
            const float yPoleHigh = influenceY_;          // weight for the YES=1 pole

            // Aggression drives the contrast (power curve exponent).
            // Low aggression (0) → exponent 1 (linear, soft glow)
            // High aggression (1) → exponent 2.5 (sharp hot-zone)
            const float exponent = 1.0f + presetHint_.dnaAggression * 1.5f;

            for (int row = 0; row < kH; ++row)
            {
                const float ny = static_cast<float>(row) / static_cast<float>(kH - 1); // 0=top=NO, 1=bottom=YES

                // X-axis sensitivity at this row: Gaussian centred on hotX,
                // width σ ≈ 0.25 (spans ≈ half the surface width).
                // Independent of ny — the X contribution is a horizontal gradient
                // that varies only left–right.
                for (int col = 0; col < kW; ++col)
                {
                    const float nx = static_cast<float>(col) / static_cast<float>(kW - 1);

                    // --- X sensitivity ---
                    // Gaussian centred on key-biased hotX.
                    const float dxHot = nx - hotX;
                    const float xSens = xNorm * std::exp(-dxHot * dxHot * 8.0f); // σ ≈ 0.25

                    // --- Y sensitivity ---
                    // Two Gaussian poles at ny=0 and ny=1 weighted by current influenceY_.
                    const float dyLow  = ny;         // distance from y=0 (NO pole)
                    const float dyHigh = 1.0f - ny;  // distance from y=1 (YES pole)
                    const float gaussLow  = std::exp(-dyLow  * dyLow  * 12.0f); // σ ≈ 0.20
                    const float gaussHigh = std::exp(-dyHigh * dyHigh * 12.0f);
                    const float ySens = yNorm * (yPoleLow * gaussLow + yPoleHigh * gaussHigh);

                    // --- Blend X and Y contributions ---
                    const float raw = (1.0f - axisBalance) * xSens + axisBalance * ySens;

                    // --- Contrast sharpening (power curve) ---
                    buf[row * kW + col] = std::pow(juce::jlimit(0.0f, 1.0f, raw), exponent);
                }
            }
        }
        else
        {
            // ----------------------------------------------------------------
            // FALLBACK HEURISTIC PATH (no preset data available)
            //
            // Identical to the original V1 scaffold: radial falloff centred on
            // a key-biased position with 3-lobe angular modulation.  Gives a
            // plausible-looking glow on first launch / before any preset loads.
            // ----------------------------------------------------------------
            const float biasCx = 0.3f + circleX_ * 0.4f;
            const float biasCy = 0.4f + influenceY_ * 0.2f;

            for (int row = 0; row < kH; ++row)
            {
                const float ny = static_cast<float>(row) / static_cast<float>(kH - 1);
                for (int col = 0; col < kW; ++col)
                {
                    const float nx = static_cast<float>(col) / static_cast<float>(kW - 1);
                    const float dx = nx - biasCx;
                    const float dy = ny - biasCy;
                    const float dist = std::sqrt(dx * dx + dy * dy);

                    float s = std::max(0.0f, 1.0f - dist * 1.8f);
                    const float angle = std::atan2(dy, dx);
                    s *= 0.7f + 0.3f * std::abs(std::sin(angle * 3.0f));
                    buf[row * kW + col] = s;
                }
            }
        }

        // ── Step 3: 3×3 box blur — one pass for smooth, edge-free output ─────
        // The blur operates on the float buffer and writes into a second buffer
        // so we never read partially-written values.
        float blurred[kW * kH];

        for (int row = 0; row < kH; ++row)
        {
            for (int col = 0; col < kW; ++col)
            {
                // Accumulate the 3×3 neighbourhood (clamped to image edges).
                float sum = 0.0f;
                int   cnt = 0;
                for (int dr = -1; dr <= 1; ++dr)
                {
                    const int nr = juce::jlimit(0, kH - 1, row + dr);
                    for (int dc = -1; dc <= 1; ++dc)
                    {
                        const int nc = juce::jlimit(0, kW - 1, col + dc);
                        sum += buf[nr * kW + nc];
                        ++cnt;
                    }
                }
                blurred[row * kW + col] = sum / static_cast<float>(cnt);
            }
        }

        // ── Step 4: Write ARGB image ─────────────────────────────────────────
        // White pixel (R=G=B=255), alpha encodes sensitivity.
        // paint() draws this image at 8% total opacity so the effective per-pixel
        // alpha is already attenuated by the GPU blending stage.
        sensitivityMap_ = juce::Image(juce::Image::ARGB, kW, kH, true);
        {
            juce::Image::BitmapData data(sensitivityMap_, juce::Image::BitmapData::writeOnly);
            for (int row = 0; row < kH; ++row)
            {
                for (int col = 0; col < kW; ++col)
                {
                    const uint8_t alpha = static_cast<uint8_t>(
                        juce::jlimit(0.0f, 1.0f, blurred[row * kW + col]) * 255.0f);
                    data.setPixelColour(col, row, juce::Colour::fromRGBA(255, 255, 255, alpha));
                }
            }
        }

        sensitivityMapDirty_ = false;
    }

    //==========================================================================
    // Marker rendering
    //==========================================================================
    void paintMarkers(juce::Graphics& g)
    {
        const auto b = harmonicSurfaceBounds_.toFloat();
        if (b.isEmpty())
            return;

        // Marker band baseline: 40% from top of the harmonic surface
        const float baselineY = b.getY() + b.getHeight() * 0.40f;

        // Base font size for the "full" (home) marker (used for text sizing only)
        constexpr float kBaseFontSize = 11.0f;

        // Current key — used to calculate tension distance for each marker
        const int currentKey = HarmonicField::positionToKey(circleX_);

        for (int idx = 0; idx < 13; ++idx)
        {
            // Normalised X position of this marker
            const float normX = static_cast<float>(idx) / 12.0f;

            // Screen X
            const float markerX = b.getX() + normX * b.getWidth();

            // Parabolic arc vertical offset (positive = downward in screen coords)
            const float arcOffset = HarmonicField::markerArcY(idx);

            // Screen Y
            const float markerY = baselineY + arcOffset;

            // Tension distance from current key to this marker's key
            const int markerKey = HarmonicField::kFifthsSemitones[static_cast<std::size_t>(idx)];
            const int dist = HarmonicField::fifthsDistance(currentKey, markerKey);

            // Size and opacity
            const auto [sizeFactor, opacity] = HarmonicField::markerProperties(dist);
            const float fontSize = kBaseFontSize * sizeFactor;

            // Tension color
            const auto [r, g2, b2] = HarmonicField::tensionColor(dist);
            const juce::Colour markerColour = juce::Colour::fromFloatRGBA(r, g2, b2, opacity);

            // Label text
            const juce::String label(HarmonicField::kNoteNames[static_cast<std::size_t>(idx)]);

            // Select the cached font bracket (Fix #10: avoid per-marker Font construction).
            // Brackets match HarmonicField::markerProperties() size factor tiers.
            int fontBracket = 3;
            if (dist == 0)
                fontBracket = 0;
            else if (dist <= 2)
                fontBracket = 1;
            else if (dist <= 4)
                fontBracket = 2;

            // Draw
            g.setColour(markerColour);
            g.setFont(markerFonts_[fontBracket]);

            // Centre the text at (markerX, markerY)
            const float textW = fontSize * 2.2f; // generous width for "F#" etc.
            const float textH = fontSize + 2.0f;
            g.drawText(label, juce::Rectangle<float>(markerX - textW * 0.5f, markerY - textH * 0.5f, textW, textH),
                       juce::Justification::centred, false);
        }
    }

    //==========================================================================
    // Harmonic influence labels + onboarding axis guides
    // "YES" / "NO" replaced with "DEEP" / "FREE" (issue #887):
    //   DEEP  — Y near top    — harmonic field strongly colors the pads
    //   FREE  — Y near bottom — pads play freely without harmonic constraint
    //==========================================================================
    void paintYesNoLabels(juce::Graphics& g)
    {
        const auto b = getLocalBounds().toFloat();
        // Opacity raised 0.20 → 0.45: still subtle but now readable (UIX Fix 2)
        const juce::Colour labelColour = juce::Colours::white.withAlpha(0.45f);
        g.setColour(labelColour);
        // Static local font — constructed once, shared across all paint() calls
        // Font size raised 9px → 11px for improved legibility (UIX Fix 2)
        static const juce::Font kLabelFont = GalleryFonts::body(11.0f).withStyle(juce::Font::italic);
        g.setFont(kLabelFont);

        // DEEP — near the top of the harmonic surface (high influence)
        const float deepY = b.getY() + 10.0f;
        g.drawText("DEEP", juce::Rectangle<float>(b.getX(), deepY, b.getWidth(), 14.0f),
                   juce::Justification::centred, false);

        // FREE — above the bottom reserved area (no harmonic constraint)
        const float reservedBottom = static_cast<float>(kGestureBarH + kGoodbyeH);
        const float freeY = b.getBottom() - reservedBottom - 18.0f;
        g.drawText("FREE", juce::Rectangle<float>(b.getX(), freeY, b.getWidth(), 14.0f),
                   juce::Justification::centred, false);

        // ── Onboarding axis labels (translucent guide text, issue #887) ──────
        // Watermark-style text that helps first-time users understand the axes.
        {
            static const juce::Font kGuideFont = GalleryFonts::body(10.0f);
            g.setFont(kGuideFont);
            g.setColour(juce::Colours::white.withAlpha(0.12f));

            // X-axis label — centered horizontally, near the top edge
            g.drawText(juce::CharPointer_UTF8("\xe2\x86\x90 KEY SELECTION \xe2\x86\x92"),
                       juce::Rectangle<float>(b.getX(), b.getY() + 8.0f, b.getWidth(), 14.0f),
                       juce::Justification::centred, false);

            // Y-axis label — rotated vertically on the right edge, centered vertically
            // on the harmonic surface (above the gesture bar).
            const float surfaceBottom = b.getBottom() - reservedBottom;
            const float surfaceCentreY = (b.getY() + surfaceBottom) * 0.5f;
            const float rightEdgeX = b.getRight() - 12.0f;

            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText(kGuideFont,
                                 juce::CharPointer_UTF8("HARMONIC DEPTH \xe2\x86\x95"),
                                 0.0f, 0.0f);

            // Rotate -90° around the right-edge anchor, then translate to position
            const juce::AffineTransform rotate =
                juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi)
                    .translated(rightEdgeX, surfaceCentreY);
            glyphs.draw(g, rotate);
        }
    }

    //==========================================================================
    // Bioluminescent trail rendering (B043 — spec Section 4.5)
    //==========================================================================
    void paintTrail(juce::Graphics& g)
    {
        const int n = trailBuffer_.count();
        if (n == 0)
            return;

        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;

        // ------------------------------------------------------------------
        // Frozen trail replay at 50% opacity (all frozen points, 4px, 25% alpha)
        // ------------------------------------------------------------------
        if (trailBuffer_.isFrozen())
        {
            const int fc = trailBuffer_.frozenCount();
            for (int i = 0; i < fc; ++i)
            {
                const TrailPoint pt = trailBuffer_.frozenPointAt(i);
                const float px = pt.x * w;
                const float py = (1.0f - pt.y) * h;
                const float radius = 2.0f; // 4px diameter

                g.setColour(accentColour_.withAlpha(0.25f));
                g.fillEllipse(px - radius, py - radius, radius * 2.0f, radius * 2.0f);
            }
        }

        // ------------------------------------------------------------------
        // Live trail: up to 12 most recent points, exponential fade over 1.5s
        // ------------------------------------------------------------------
        const int maxPoints = std::min(n, 12);
        for (int age = 0; age < maxPoints; ++age)
        {
            const TrailPoint pt = trailBuffer_.pointByAge(age);
            const double ptAge = now - pt.timestamp;

            // Skip if older than 1.5 seconds
            if (ptAge > 1.5)
                continue;

            // Alpha: exponential decay — reaches near-zero at 1.5s
            const float alpha = static_cast<float>(std::exp(-ptAge * 2.0));

            // Pixel position (Y-flipped: normalised 0=bottom, 1=top)
            const float px = pt.x * w;
            const float py = (1.0f - pt.y) * h;

            // Core radius: shrinks from ~5px to 1px as alpha decays
            const float coreRadius = 5.0f * alpha + 1.0f;

            // --- Pass A: outer glow (3× radius, 50% of alpha) ---
            const float glowRadius = coreRadius * 3.0f;
            g.setColour(accentColour_.withAlpha(alpha * 0.50f));
            g.fillEllipse(px - glowRadius, py - glowRadius, glowRadius * 2.0f, glowRadius * 2.0f);

            // --- Pass B: core (1× radius, full alpha) ---
            g.setColour(accentColour_.withAlpha(alpha));
            g.fillEllipse(px - coreRadius, py - coreRadius, coreRadius * 2.0f, coreRadius * 2.0f);
        }
    }

    //==========================================================================
    // Mouse → normalised position
    //==========================================================================

    /** Converts a mouse event to {circleX, influenceY}.
        circleX : left=0, right=1
        influenceY: top=1 (YES), bottom=0 (NO)  — inverted from screen coords. */
    std::pair<float, float> mouseToNormalized(const juce::MouseEvent& e) const
    {
        const auto b = harmonicSurfaceBounds_.toFloat();
        if (b.isEmpty())
            return {circleX_, influenceY_};

        const float mx = static_cast<float>(e.getPosition().x);
        const float my = static_cast<float>(e.getPosition().y);

        const float nx = juce::jlimit(0.0f, 1.0f, (mx - b.getX()) / b.getWidth());

        // Inverted: top of surface → 1.0 (YES), bottom → 0.0 (NO)
        const float ny = juce::jlimit(0.0f, 1.0f, 1.0f - (my - b.getY()) / b.getHeight());

        return {nx, ny};
    }

    //==========================================================================
    // Fire onPositionChanged and onCCOutput callbacks
    //==========================================================================
    void fireCallbacks()
    {
        if (onPositionChanged)
            onPositionChanged(circleX_, influenceY_);

        if (onCCOutput)
        {
            // CC 85: circle-of-fifths position (0–127)
            const auto ccX = static_cast<uint8_t>(juce::roundToInt(circleX_ * 127.0f));
            // CC 86: influence depth (0–127)
            const auto ccY = static_cast<uint8_t>(juce::roundToInt(influenceY_ * 127.0f));

            onCCOutput(85, ccX);
            onCCOutput(86, ccY);
        }
    }

    //==========================================================================
    // switchGestureBank — load per-bank button defs into the GestureButtonBar.
    // Respects the GestureButtonBar lock pin: if locked, this is a no-op.
    //==========================================================================
    void switchGestureBank(GestureButtonBar::Bank bank)
    {
        if (gestureButtons_.isBankLocked())
            return; // Lock pin engaged — don't change the active bank or buttons

        gestureButtons_.setBank(bank);
        gestureButtons_.setButtons(bankDefs_[static_cast<std::size_t>(bank)]);
    }

    //==========================================================================
    // Setup default XOuija button bank (FREEZE / HOME / DRIFT)
    //==========================================================================
    void setupDefaultButtonBank()
    {
        std::array<GestureButtonBar::ButtonDef, 3>& defs =
            bankDefs_[static_cast<std::size_t>(GestureButtonBar::Bank::XOuija)];

        // Slot 0: FREEZE (F) — toggle trail freeze/unfreeze + CC 88
        defs[0].label = "FREEZE [F]";
        defs[0].shortcutKey = 'F';
        defs[0].action = [this]()
        {
            if (trailBuffer_.isFrozen())
            {
                trailBuffer_.unfreeze();
                if (onCCOutput)
                    onCCOutput(88, 0);
                gestureButtons_.setButtonToggleState(0, false);
            }
            else
            {
                trailBuffer_.freeze();
                if (onCCOutput)
                    onCCOutput(88, 127);
                gestureButtons_.setButtonToggleState(0, true);
            }
            repaint();
        };

        // Slot 1: HOME (H) — snap planchette to centre + fire position callbacks
        defs[1].label = "HOME [H]";
        defs[1].shortcutKey = 'H';
        defs[1].action = [this]()
        {
            planchette_.snapHome();
            circleX_ = 0.5f;
            influenceY_ = 0.5f;
            updatePlanchetteText();
            fireCallbacks();
            // CC 89→127 signals "home" event
            if (onCCOutput)
            {
                onCCOutput(85, 64);
                onCCOutput(86, 64);
                onCCOutput(89, 127);
            }
            repaint();
        };

        // Slot 2: DRIFT (D) — toggle Lissajous drift + CC 90
        defs[2].label = "DRIFT [D]";
        defs[2].shortcutKey = 'D';
        defs[2].action = [this]()
        {
            const bool newState = !planchette_.isDriftEnabled();
            planchette_.setDriftEnabled(newState);
            gestureButtons_.setButtonToggleState(2, newState);
            if (onCCOutput)
                onCCOutput(90, newState ? 127 : 0);
        };

        // Load XOuija bank as the initial active bank
        gestureButtons_.setButtons(defs);
    }

    //==========================================================================
    // Setup Dub button bank — DUB LOOP / MUTE / RESET
    // These are live dub-space performance actions that fire CC 91-93.
    //   CC 91: DUB LOOP toggle (0=off, 127=on)
    //   CC 92: DUB MUTE toggle (0=unmuted, 127=muted)
    //   CC 93: DUB RESET (momentary 127 pulse)
    //==========================================================================
    void setupDubButtonBank()
    {
        std::array<GestureButtonBar::ButtonDef, 3>& defs =
            bankDefs_[static_cast<std::size_t>(GestureButtonBar::Bank::Dub)];

        // Slot 0: LOOP (L) — toggle delay loop capture in DubSpace strip mode
        defs[0].label = "LOOP [L]";
        defs[0].shortcutKey = 'L';
        defs[0].action = [this]()
        {
            dubLoopActive_ = !dubLoopActive_;
            gestureButtons_.setButtonToggleState(0, dubLoopActive_);
            if (onCCOutput)
                onCCOutput(91, dubLoopActive_ ? 127 : 0);
        };

        // Slot 1: MUTE (M) — mute DubSpace XY output (hold to momentary mute)
        defs[1].label = "MUTE [M]";
        defs[1].shortcutKey = 'M';
        defs[1].action = [this]()
        {
            dubMuteActive_ = !dubMuteActive_;
            gestureButtons_.setButtonToggleState(1, dubMuteActive_);
            if (onCCOutput)
                onCCOutput(92, dubMuteActive_ ? 127 : 0);
        };

        // Slot 2: RESET (R) — send CC 93 momentary pulse to reset dub state
        defs[2].label = "RESET [R]";
        defs[2].shortcutKey = 'R';
        defs[2].action = [this]()
        {
            dubLoopActive_ = false;
            dubMuteActive_ = false;
            gestureButtons_.setButtonToggleState(0, false);
            gestureButtons_.setButtonToggleState(1, false);
            if (onCCOutput)
                onCCOutput(93, 127);
        };
    }

    //==========================================================================
    // Setup Coupling button bank — BOOST / INVERT / CLEAR
    // Controls the coupling influence surface while in Coupling strip mode.
    //   CC 94: BOOST — push coupling depth to maximum for one bar
    //   CC 95: INVERT — invert coupling polarity (CC value: 0=normal, 127=inverted)
    //   CC 96: CLEAR — momentary: clear all active coupling routes
    //==========================================================================
    void setupCouplingButtonBank()
    {
        std::array<GestureButtonBar::ButtonDef, 3>& defs =
            bankDefs_[static_cast<std::size_t>(GestureButtonBar::Bank::Coupling)];

        // Slot 0: BOOST (B) — momentary coupling depth surge (CC 94 pulse)
        defs[0].label = "BOOST [B]";
        defs[0].shortcutKey = 'B';
        defs[0].action = [this]()
        {
            if (onCCOutput)
                onCCOutput(94, 127);
        };

        // Slot 1: INVERT (I) — toggle coupling polarity (CC 95)
        defs[1].label = "INVERT [I]";
        defs[1].shortcutKey = 'I';
        defs[1].action = [this]()
        {
            couplingInverted_ = !couplingInverted_;
            gestureButtons_.setButtonToggleState(1, couplingInverted_);
            if (onCCOutput)
                onCCOutput(95, couplingInverted_ ? 127 : 0);
        };

        // Slot 2: CLEAR (C) — momentary: signal all coupling routes clear (CC 96)
        defs[2].label = "CLEAR [C]";
        defs[2].shortcutKey = 'C';
        defs[2].action = [this]()
        {
            couplingInverted_ = false;
            gestureButtons_.setButtonToggleState(1, false);
            if (onCCOutput)
                onCCOutput(96, 127);
        };
    }

    //==========================================================================
    // Setup Performance button bank — PANIC / LATCH / BYPASS
    // Global performance emergency actions.
    //   CC 99: PANIC momentary pulse (value 127 = all notes off triggered)
    //   CC 97: LATCH toggle — hold notes on (0=off, 127=on)
    //   CC 98: BYPASS toggle — bypass all FX (0=active, 127=bypassed)
    // PANIC also fires onGoodbye() for All Notes Off on channels 1-16.
    //==========================================================================
    void setupPerformanceButtonBank()
    {
        std::array<GestureButtonBar::ButtonDef, 3>& defs =
            bankDefs_[static_cast<std::size_t>(GestureButtonBar::Bank::Performance)];

        // Slot 0: PANIC (P) — All Notes Off on channels 1-16.
        // CC 99 value 127 = PANIC event (dedicated, no collision with LATCH CC 97).
        defs[0].label = "PANIC [P]";
        defs[0].shortcutKey = 'P';
        defs[0].action = [this]()
        {
            // Fire GOODBYE (All Notes Off) via the existing onGoodbye pathway
            if (onGoodbye)
                onGoodbye();
            // CC 99 value 127 = dedicated PANIC event signal
            if (onCCOutput)
                onCCOutput(99, 127);
        };

        // Slot 1: LATCH (T) — toggle note latch (hold notes sustained)
        defs[1].label = "LATCH [T]";
        defs[1].shortcutKey = 'T';
        defs[1].action = [this]()
        {
            perfLatchActive_ = !perfLatchActive_;
            gestureButtons_.setButtonToggleState(1, perfLatchActive_);
            if (onCCOutput)
                onCCOutput(97, perfLatchActive_ ? 127 : 0);
        };

        // Slot 2: BYPASS (Y) — toggle FX chain bypass
        defs[2].label = "BYPASS [Y]";
        defs[2].shortcutKey = 'Y';
        defs[2].action = [this]()
        {
            perfBypassActive_ = !perfBypassActive_;
            gestureButtons_.setButtonToggleState(2, perfBypassActive_);
            if (onCCOutput)
                onCCOutput(98, perfBypassActive_ ? 127 : 0);
        };
    }

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOuijaPanel)
};

} // namespace xoceanus
