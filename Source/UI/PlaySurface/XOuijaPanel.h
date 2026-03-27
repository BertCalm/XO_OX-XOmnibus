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
      - YES / NO labels in Georgia italic 9px at opacity 0.20
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
    Namespace: xolokun
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

namespace xolokun {

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
class Planchette : public juce::Component,
                   private juce::Timer
{
public:
    //==========================================================================
    // Sizing constants (spec Section 4)
    //==========================================================================
    static constexpr int   kWidth       = 68;
    static constexpr int   kHeight      = 46;
    static constexpr float kPipDiameter = 6.0f;

    // Lissajous drift parameters
    static constexpr float kDriftFreqX   = 0.3f;   // Hz
    static constexpr float kDriftFreqY   = 0.2f;   // Hz
    static constexpr float kDriftAmp     = 0.15f;  // fraction of parent dimension
    static constexpr float kDriftPhase   = juce::MathConstants<float>::pi / 4.0f;  // π/4

    // Animation timing
    static constexpr float kSpringMs    = 80.0f;   // spring duration ms (QDD: was 150ms, reduced for snappier lock-on)
    static constexpr float kWarmHoldMs  = 400.0f;  // warm hold duration ms

    //==========================================================================
    enum class State { Lissajous, Springing, Touching, WarmHold };

    //==========================================================================
    Planchette()
        : accentColour_ (juce::Colour (0xFFE9C46A))
        , displayX_ (0.5f)
        , displayY_ (0.5f)
        , driftAnchorX_ (0.5f)
        , driftAnchorY_ (0.5f)
        , springStartX_ (0.5f)
        , springStartY_ (0.5f)
        , springTargetX_ (0.5f)
        , springTargetY_ (0.5f)
        , springElapsedMs_ (0.0f)
        , springDurationMs_ (kSpringMs)
        , warmHoldElapsedMs_ (0.0f)
        , driftTimeS_ (0.0f)
        , state_ (State::Lissajous)
        , driftEnabled_ (true)
        , displayText_ ("C · 0%")
        // Cache Georgia italic font once — avoids repeated construction in paint()
        , textFont_ (juce::Font ("Georgia", 10.0f, juce::Font::italic))
    {
        setInterceptsMouseClicks (false, false);
        startTimerHz (60);

        // WCAG Fix 1: accessibility API
        setAccessible (true);
        setTitle ("Planchette - Harmonic Position Indicator");
        setDescription ("Shows current position on circle of fifths and influence depth");
    }

    ~Planchette() override
    {
        stopTimer();
    }

    //==========================================================================
    // Public API
    //==========================================================================

    void setAccentColour (juce::Colour c)
    {
        accentColour_ = c;
        repaint();
    }

    void setDisplayText (juce::String text)
    {
        displayText_ = text;
        repaint();
    }

    /** Called on mouseDown — begin 80ms spring animation to (normX, normY) (300ms for GOODBYE). */
    void springTo (float normX, float normY)
    {
        springStartX_    = displayX_;
        springStartY_    = displayY_;
        springTargetX_   = normX;
        springTargetY_   = normY;
        springElapsedMs_ = 0.0f;
        springDurationMs_ = kSpringMs;
        state_           = State::Springing;
    }

    /** Slower spring used for GOODBYE — slides planchette to bottom over 300ms. */
    void springToSlow (float nx, float ny, float durationMs = 300.0f)
    {
        springStartX_     = displayX_;
        springStartY_     = displayY_;
        springTargetX_    = nx;
        springTargetY_    = ny;
        springElapsedMs_  = 0.0f;
        springDurationMs_ = durationMs;
        state_            = State::Springing;
    }

    /** Called on mouseDrag — snap to position once spring has completed. */
    void moveTo (float normX, float normY)
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
        driftAnchorX_       = displayX_;
        driftAnchorY_       = displayY_;
        warmHoldElapsedMs_  = 0.0f;
        // Reset drift phase so drift starts from anchor with zero displacement
        driftTimeS_         = 0.0f;
        state_              = State::WarmHold;
    }

    void setDriftEnabled (bool on) { driftEnabled_ = on; }
    bool isDriftEnabled()  const   { return driftEnabled_; }

    /** WCAG Fix 3: when true, Lissajous idle drift is suppressed and the planchette
        stays at its last anchor position. Wire to macOS accessibility preference
        (NSWorkspace.accessibilityDisplayShouldReduceMotion) when available. */
    void setReducedMotion (bool on) { reducedMotion_ = on; }
    bool isReducedMotion()  const   { return reducedMotion_; }

    /** Spring to centre (0.5, 0.5). */
    void snapHome()
    {
        springTo (0.5f, 0.5f);
    }

    float getDisplayX()  const noexcept { return displayX_; }
    float getDisplayY()  const noexcept { return displayY_; }

    bool isTouching() const noexcept
    {
        return state_ == State::Springing || state_ == State::Touching;
    }

    //==========================================================================
    // Component overrides
    //==========================================================================

    void visibilityChanged() override
    {
        if (isVisible()) startTimerHz(60);
        else stopTimer();
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // 1. Translucent oval lens fill — accent at 15% opacity
        g.setColour (accentColour_.withAlpha (0.15f));
        g.fillEllipse (bounds);

        // 2. Border — accent at full opacity, 1.5px stroke
        g.setColour (accentColour_);
        g.drawEllipse (bounds.reduced (0.75f), 1.5f);

        // 3. Inner glow — accent at 40% opacity, slightly inset ellipse
        g.setColour (accentColour_.withAlpha (0.40f));
        g.drawEllipse (bounds.reduced (4.0f), 1.0f);

        // 4. Center pip — 6px filled circle
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        g.setColour (accentColour_);
        g.fillEllipse (cx - kPipDiameter * 0.5f,
                       cy - kPipDiameter * 0.5f,
                       kPipDiameter,
                       kPipDiameter);

        // 5. Interior text — Georgia italic 10px, accent at 85% opacity
        //    Positioned below centre. Uses cached textFont_ (not rebuilt each paint).
        g.setColour (accentColour_.withAlpha (0.85f));
        g.setFont (textFont_);
        const float textY = cy + kPipDiameter * 0.5f + 1.0f;
        g.drawText (displayText_,
                    juce::Rectangle<float> (bounds.getX(),
                                            textY,
                                            bounds.getWidth(),
                                            12.0f),
                    juce::Justification::centred,
                    false);
    }

private:
    //==========================================================================
    // Timer callback — 60Hz state machine
    //==========================================================================
    void timerCallback() override
    {
        constexpr float kDtMs = 1000.0f / 60.0f;  // ~16.67 ms per tick

        switch (state_)
        {
            case State::Springing:
            {
                springElapsedMs_ += kDtMs;
                const float t = juce::jlimit (0.0f, 1.0f,
                                              springElapsedMs_ / springDurationMs_);
                // Ease-out: 1 - (1-t)^2
                const float ease = 1.0f - (1.0f - t) * (1.0f - t);
                displayX_ = springStartX_ + (springTargetX_ - springStartX_) * ease;
                displayY_ = springStartY_ + (springTargetY_ - springStartY_) * ease;

                if (springElapsedMs_ >= springDurationMs_)
                {
                    displayX_ = springTargetX_;
                    displayY_ = springTargetY_;
                    state_    = State::Touching;
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
                    driftTimeS_   = 0.0f;
                    state_        = State::Lissajous;
                }
                break;
            }

            case State::Lissajous:
            {
                // WCAG Fix 3: skip drift entirely when reduced motion is requested
                if (! driftEnabled_ || reducedMotion_)
                    break;

                driftTimeS_ += kDtMs * 0.001f;  // convert ms → seconds

                // Compute Lissajous displacement as fraction of parent size
                const float driftX = kDriftAmp *
                    std::sin (juce::MathConstants<float>::twoPi * kDriftFreqX * driftTimeS_);
                const float driftY = kDriftAmp *
                    std::sin (juce::MathConstants<float>::twoPi * kDriftFreqY * driftTimeS_
                               + kDriftPhase);

                displayX_ = driftAnchorX_ + driftX;
                displayY_ = driftAnchorY_ + driftY;

                // Clamp so planchette stays fully on screen (Fix #20: use half-size margins
                // so the planchette body never goes off-screen at the edges).
                if (auto* parent = getParentComponent())
                {
                    const float parentW = static_cast<float> (parent->getWidth());
                    const float parentH = static_cast<float> (parent->getHeight());
                    const float marginX = (parentW > 0.0f)
                                         ? static_cast<float> (kWidth)  / (2.0f * parentW)
                                         : 0.0f;
                    const float marginY = (parentH > 0.0f)
                                         ? static_cast<float> (kHeight) / (2.0f * parentH)
                                         : 0.0f;
                    displayX_ = juce::jlimit (marginX, 1.0f - marginX, displayX_);
                    displayY_ = juce::jlimit (marginY, 1.0f - marginY, displayY_);
                }
                else
                {
                    displayX_ = juce::jlimit (0.0f, 1.0f, displayX_);
                    displayY_ = juce::jlimit (0.0f, 1.0f, displayY_);
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
            const float pw = static_cast<float> (parent->getWidth());
            const float ph = static_cast<float> (parent->getHeight());

            const int px = juce::roundToInt (displayX_ * pw - kWidth  * 0.5f);
            const int py = juce::roundToInt ((1.0f - displayY_) * ph - kHeight * 0.5f);

            setBounds (px, py, kWidth, kHeight);
        }
    }

    //==========================================================================
    // State
    //==========================================================================
    juce::Colour  accentColour_;
    float         displayX_;
    float         displayY_;

    // Drift anchor — centre of Lissajous figure
    float         driftAnchorX_;
    float         driftAnchorY_;

    // Spring animation
    float         springStartX_;
    float         springStartY_;
    float         springTargetX_;
    float         springTargetY_;
    float         springElapsedMs_;
    float         springDurationMs_;  // kSpringMs normally; 300ms for GOODBYE

    // Warm hold
    float         warmHoldElapsedMs_;

    // Drift time accumulator (seconds)
    float         driftTimeS_;

    State         state_;
    bool          driftEnabled_;
    juce::String  displayText_;

    // WCAG Fix 3: reduced motion path — when true, skip Lissajous drift animation.
    // TODO: wire to macOS NSWorkspace.accessibilityDisplayShouldReduceMotion via
    //       an NSApplicationDelegate callback. Currently opt-in only via setReducedMotion().
    bool          reducedMotion_ = false;

    // Cached font — initialized in constructor, avoids per-paint construction
    juce::Font    textFont_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Planchette)
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
    enum class Bank { XOuija, Dub, Coupling, Performance };

    struct ButtonDef
    {
        juce::String        label;
        char                shortcutKey  = 0;
        std::function<void()> action;
    };

    //==========================================================================
    GestureButtonBar()
        // Cache fonts once — avoids per-paint Font construction (UIX fix)
        : lockFont_   (juce::Font (14.0f, juce::Font::plain))
        , buttonFont_ (juce::Font (10.0f, juce::Font::plain))
    {
        // Default bank buttons are populated by XOuijaPanel::setupDefaultButtonBank()
        // so they remain empty here.

        // WCAG Fix 1: accessibility API
        setAccessible (true);
        setTitle ("Performance Gesture Buttons");
        setDescription ("Three configurable performance buttons: Freeze, Home, Drift");
    }

    ~GestureButtonBar() override = default;

    //==========================================================================
    // Public API
    //==========================================================================

    /** Switch bank; respects the lock pin. */
    void setBank (Bank bank)
    {
        if (bankLocked_)
            return;
        currentBank_ = bank;
        repaint();
    }

    Bank getBank() const noexcept { return currentBank_; }

    void setBankLocked (bool locked) { bankLocked_ = locked; repaint(); }
    bool isBankLocked() const noexcept { return bankLocked_; }

    /** Set the 3 button definitions for the current active bank (XOuija bank). */
    void setButtons (const std::array<ButtonDef, 3>& defs)
    {
        buttons_ = defs;
        repaint();
    }

    /** Check shortcut key; fire action and return true if handled. */
    bool handleKey (char c)
    {
        for (auto& btn : buttons_)
        {
            if (btn.shortcutKey != 0 &&
                std::toupper (static_cast<unsigned char> (c)) ==
                std::toupper (static_cast<unsigned char> (btn.shortcutKey)))
            {
                if (btn.action)
                    btn.action();
                pressedIndex_ = -1;  // no persistent highlight for key events
                repaint();
                return true;
            }
        }
        return false;
    }

    //==========================================================================
    // Component overrides
    //==========================================================================

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float w = b.getWidth();
        const float h = b.getHeight();

        // ---- Lock button (top-right, ~14px) ----
        const float lockSize = 14.0f;
        const float lockX = w - lockSize - 2.0f;
        const float lockY = 2.0f;
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.setFont (lockFont_);  // cached — avoids per-paint Font construction
        // Filled circle (locked) / empty circle (unlocked) — universally understood
        g.drawText (bankLocked_ ? "\xe2\x97\x8f" : "\xe2\x97\x8b",
                    juce::Rectangle<float> (lockX, lockY, lockSize, lockSize),
                    juce::Justification::centred, false);

        // ---- Three button slots ----
        const float cornerR = 3.0f;

        for (int i = 0; i < 3; ++i)
        {
            const juce::Rectangle<float> buttonRect = getButtonRect (i);

            const bool pressed = (pressedIndex_ == i);

            // Background
            if (pressed)
                g.setColour (accentColour_.withAlpha (0.30f));
            else
                g.setColour (juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, 0.06f));
            g.fillRoundedRectangle (buttonRect, cornerR);

            // Border (inactive only)
            if (! pressed)
            {
                g.setColour (juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, 0.12f));
                g.drawRoundedRectangle (buttonRect, cornerR, 1.0f);
            }

            // Label
            const float textAlpha = pressed ? 1.0f : 0.65f;
            g.setColour (juce::Colours::white.withAlpha (textAlpha));
            g.setFont (buttonFont_);  // cached — avoids per-paint Font construction; 10px
            g.drawText (buttons_[static_cast<std::size_t> (i)].label.toUpperCase(),
                        buttonRect,
                        juce::Justification::centred,
                        false);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        // Check lock icon click first — sits above the button row
        float lockX = static_cast<float> (getWidth()) - 18.0f;
        if (e.position.x >= lockX && e.position.y < 18.0f)
        {
            bankLocked_ = !bankLocked_;
            repaint();
            return;
        }

        pressedIndex_ = hitTestButton (e.position.x, e.position.y);
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        const int idx = hitTestButton (e.position.x, e.position.y);
        if (idx >= 0 && idx == pressedIndex_)
        {
            const auto& btn = buttons_[static_cast<std::size_t> (idx)];
            if (btn.action)
                btn.action();
        }
        pressedIndex_ = -1;
        repaint();
    }

    void setAccentColour (juce::Colour c) { accentColour_ = c; repaint(); }

    /** Returns the rect for button at index (0, 1, or 2). */
    juce::Rectangle<float> getButtonRect (int index) const
    {
        float w = static_cast<float> (getWidth());
        float gutter  = 4.0f;
        float buttonW = (w - gutter * 4.0f) / 3.0f;
        float buttonH = 28.0f;
        float y = (static_cast<float> (getHeight()) - buttonH) / 2.0f;
        return { gutter + static_cast<float> (index) * (buttonW + gutter), y, buttonW, buttonH };
    }

private:
    //==========================================================================
    int hitTestButton (float mx, float my) const
    {
        const float h = static_cast<float> (getHeight());
        const float buttonH = 28.0f;
        const float buttonTop = h - buttonH;

        if (my < buttonTop)
            return -1;

        for (int i = 0; i < 3; ++i)
        {
            auto r = getButtonRect (i);
            if (mx >= r.getX() && mx < r.getRight())
                return i;
        }
        return -1;
    }

    //==========================================================================
    std::array<ButtonDef, 3>  buttons_;
    Bank                      currentBank_   = Bank::XOuija;
    bool                      bankLocked_    = false;
    int                       pressedIndex_  = -1;
    juce::Colour              accentColour_  { juce::Colour (0xFFE9C46A) };

    // Cached fonts — initialized in constructor, avoids per-paint construction
    juce::Font lockFont_;    // 14px plain — for lock indicator circle
    juce::Font buttonFont_;  // 10px plain — for button labels

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GestureButtonBar)
};

//==============================================================================
/** GoodbyeButton (Task 7 — Spec Section 7)
    Warm Terracotta full-width button that resets the XOuija session.
    Color: #E07A5F at 80% opacity normally, 100% on press.
    Text: "GOODBYE" in Georgia italic 11px, white at 90% opacity.
*/
class GoodbyeButton : public juce::Component
{
public:
    //==========================================================================
    GoodbyeButton()
        // Cache Georgia italic font once — avoids repeated construction in paint()
        : labelFont_ (juce::Font ("Georgia", 11.0f, juce::Font::italic))
    {
        // WCAG Fix 1: accessibility API
        setAccessible (true);
        setTitle ("Goodbye - All Notes Off");
        setDescription ("Resets harmonic position and silences all voices");
    }
    ~GoodbyeButton() override = default;

    //==========================================================================
    std::function<void()> onGoodbye;

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        const auto  b      = getLocalBounds().toFloat();
        const float alpha  = pressed_ ? 1.0f : 0.80f;
        const juce::Colour base { 0xFFE07A5F };  // Warm Terracotta

        // Background — no border
        g.setColour (base.withAlpha (alpha));
        g.fillRoundedRectangle (b, 4.0f);

        // Label — uses cached labelFont_ (not rebuilt each paint)
        g.setColour (juce::Colours::white.withAlpha (0.90f));
        g.setFont (labelFont_);
        g.drawText ("GOODBYE", b, juce::Justification::centred, false);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        pressed_ = true;
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (getLocalBounds().toFloat().contains(e.position))
            if (onGoodbye) onGoodbye();
        pressed_ = false;
        repaint();
    }

private:
    bool       pressed_   = false;
    juce::Font labelFont_; // cached in constructor — Georgia italic 11px

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GoodbyeButton)
};

//==============================================================================
class XOuijaPanel : public juce::Component
{
public:
    // Sizing constants (spec Section 2.2)
    //==========================================================================
    static constexpr int kMinWidth       = 155;
    static constexpr int kMaxWidth       = 185;
    static constexpr int kPreferredWidth = 165;

    // Aspect ratio ~9:19.5  (width : height)
    static constexpr float kAspectRatio = 9.0f / 19.5f;

    //==========================================================================
    // Reserved layout heights (for Tasks 5-7)
    //==========================================================================
    static constexpr int kGestureBarH  = 34;  // Task 7: GestureButtonBar
    static constexpr int kGoodbyeH     = 32;  // Task 7: GOODBYE button

    //==========================================================================
    // Constructor / Destructor
    //==========================================================================
    XOuijaPanel()
        : accentColour_ (juce::Colour (0xFFE9C46A))  // XO Gold
    {
        generateNoiseTexture();
        setOpaque (false);

        // WCAG Fix 1: accessibility API
        setAccessible (true);
        setTitle ("XOuija Harmonic Navigator");
        setDescription ("Circle of fifths harmonic navigation surface with influence depth control");

        // B042: add the planchette as a child — it manages its own bounds
        addAndMakeVisible (planchette_);
        planchette_.setAccentColour (accentColour_);
        updatePlanchetteText();

        // Task 7: gesture button bar and GOODBYE button
        addAndMakeVisible (gestureButtons_);
        gestureButtons_.setAccentColour (accentColour_);

        addAndMakeVisible (goodbyeButton_);

        // Wire GOODBYE action
        goodbyeButton_.onGoodbye = [this]()
        {
            // Reset position state
            circleX_    = 0.5f;
            influenceY_ = 0.0f;

            // Animate planchette to bottom-centre (influenceY=0 → bottom)
            planchette_.springToSlow (0.5f, 0.0f, 300.0f);

            // Clear trail buffer
            trailBuffer_.clear();

            // Emit CC resets: CC 85→64 (centre), CC 86→0 (no influence)
            // CC 88 = trail freeze state (Spec Section 11.1). GOODBYE unfreezes.
            if (onCCOutput)
            {
                onCCOutput (85, 64);
                onCCOutput (86, 0);
                onCCOutput (88, 0); // CC 88 = trail freeze state (Spec Section 11.1). GOODBYE unfreezes.
            }

            // Update planchette text
            updatePlanchetteText();

            // Notify PlaySurface (for All Notes Off etc.)
            if (onGoodbye)
                onGoodbye();

            repaint();
        };

        // Wire default XOuija button bank
        setupDefaultButtonBank();

        setWantsKeyboardFocus (true);

        // ── Cache marker fonts (Fix #10: avoid per-paint Font construction) ──
        // Sizes = kBaseFontSize (11.0) * sizeFactor from markerProperties()
        constexpr float kBaseFontSize = 11.0f;
        constexpr float kSizeFactors[4] = { 1.00f, 0.85f, 0.70f, 0.55f };
        for (int i = 0; i < 4; ++i)
            markerFonts_[i] = juce::Font ("Georgia",
                                           kBaseFontSize * kSizeFactors[i],
                                           juce::Font::italic);
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

    //==========================================================================
    // Public state accessors
    //==========================================================================

    /** Normalised horizontal circle-of-fifths position in [0,1]. */
    float getCirclePosition() const noexcept { return circleX_; }

    /** Normalised influence depth in [0,1]. */
    float getInfluenceDepth() const noexcept { return influenceY_; }

    /** Semitone of the nearest key (0=C, 1=C#, … 11=B). */
    int getCurrentKey() const noexcept
    {
        return HarmonicField::positionToKey (circleX_);
    }

    /** True while the user finger / mouse button is down. */
    bool isTouching() const noexcept { return touching_; }

    //==========================================================================
    // Setters (for remote CC / automation input)
    //==========================================================================

    void setAccentColour (juce::Colour c)
    {
        accentColour_ = c;
        planchette_.setAccentColour (c);
        gestureButtons_.setAccentColour (c);
        repaint();
    }

    juce::Colour getAccentColour() const noexcept { return accentColour_; }

    /** Drive circleX from an external source (e.g. incoming CC 85). */
    void setCirclePosition (float x)
    {
        circleX_ = juce::jlimit (0.0f, 1.0f, x);
        repaint();
    }

    /** Drive influenceY from an external source (e.g. incoming CC 86). */
    void setInfluenceDepth (float y)
    {
        influenceY_ = juce::jlimit (0.0f, 1.0f, y);
        repaint();
    }

    //==========================================================================
    // Component overrides
    //==========================================================================

    void resized() override
    {
        // Compute harmonic surface area — the area used for marker layout
        // excludes the two reserved strips at the bottom.
        auto b = getLocalBounds();
        const int reservedBottom = kGestureBarH + kGoodbyeH;  // 66px

        harmonicSurfaceBounds_ = b.withTrimmedBottom (reservedBottom);

        // GOODBYE button: 32px at the very bottom, full width
        goodbyeButton_.setBounds (b.removeFromBottom (kGoodbyeH));

        // GestureButtonBar: 34px above GOODBYE, full width
        gestureButtons_.setBounds (b.removeFromBottom (kGestureBarH));

        // Planchette manages its own setBounds() inside timerCallback via
        // updateBounds(). No explicit positioning needed here.
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // ------------------------------------------------------------------
        // 1. Background: dark rounded rect
        // ------------------------------------------------------------------
        g.setColour (juce::Colour (0xFF1e1e22));
        g.fillRoundedRectangle (bounds, 8.0f);

        // ------------------------------------------------------------------
        // 2. Noise texture at opacity 0.05
        // ------------------------------------------------------------------
        if (noiseImage_.isValid())
        {
            g.setOpacity (0.05f);
            // Tile the 128x128 image across the component
            g.drawImage (noiseImage_,
                         bounds,
                         juce::RectanglePlacement::fillDestination,
                         false);
            g.setOpacity (1.0f);
        }

        // ------------------------------------------------------------------
        // 3. Circle-of-fifths markers
        // ------------------------------------------------------------------
        paintMarkers (g);

        // ------------------------------------------------------------------
        // 4. YES / NO labels
        // ------------------------------------------------------------------
        paintYesNoLabels (g);

        // ------------------------------------------------------------------
        // 5. Bioluminescent gesture trail (spec Section 4.5)
        //    Rendered after static elements, below planchette (child component).
        // ------------------------------------------------------------------
        paintTrail (g);
    }

    //==========================================================================
    // Mouse handling
    //==========================================================================

    void mouseDown (const juce::MouseEvent& e) override
    {
        touching_ = true;
        auto [nx, ny] = mouseToNormalized (e);
        circleX_    = nx;
        influenceY_ = ny;

        // Reset trail tracking so the first drag step doesn't record a giant jump
        lastTrailPixelX_ = e.position.x;
        lastTrailPixelY_ = e.position.y;

        planchette_.springTo (nx, ny);
        updatePlanchetteText();

        repaint();
        fireCallbacks();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        auto [nx, ny] = mouseToNormalized (e);
        circleX_    = nx;
        influenceY_ = ny;

        // Record trail point with distance-based spacing (~4px minimum)
        const float dx = e.position.x - lastTrailPixelX_;
        const float dy = e.position.y - lastTrailPixelY_;
        if (dx * dx + dy * dy >= 16.0f)  // 4px distance threshold (squared)
        {
            const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
            const float vel = std::clamp (std::sqrt (dx * dx + dy * dy) / 50.0f, 0.0f, 1.0f);
            trailBuffer_.push (circleX_, influenceY_, vel, now);
            lastTrailPixelX_ = e.position.x;
            lastTrailPixelY_ = e.position.y;
        }

        planchette_.moveTo (nx, ny);
        updatePlanchetteText();

        repaint();
        fireCallbacks();
    }

    void mouseUp (const juce::MouseEvent& /*e*/) override
    {
        touching_ = false;
        planchette_.release();
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        const char c = static_cast<char> (
            std::toupper (static_cast<unsigned char> (key.getTextCharacter())));

        if (c == 'G')
        {
            // Trigger GOODBYE action directly (G for GOODBYE)
            if (goodbyeButton_.onGoodbye)
                goodbyeButton_.onGoodbye();
            return true;
        }

        return gestureButtons_.handleKey (c);
    }

private:
    //==========================================================================
    // State
    //==========================================================================
    float        circleX_    = 0.5f;   // 0=Gb, 0.5=C, 1.0=F#
    float        influenceY_ = 0.0f;   // 0=NO, 1=YES
    bool         touching_   = false;
    juce::Colour accentColour_;

    juce::Image  noiseImage_;
    juce::Rectangle<int> harmonicSurfaceBounds_;

    // B042 — Planchette child component
    Planchette   planchette_;

    // B043 — Gesture trail ring buffer + last-recorded pixel position
    GestureTrailBuffer trailBuffer_;
    float              lastTrailPixelX_ = 0.0f;
    float              lastTrailPixelY_ = 0.0f;

    // Task 7 — Gesture button bar and GOODBYE button
    GestureButtonBar   gestureButtons_;
    GoodbyeButton      goodbyeButton_;

    // ── Cached marker fonts (Fix #10: avoid per-paint Font construction) ─────
    // 4 size brackets corresponding to HarmonicField::markerProperties() output:
    //   [0] dist=0     → sizeFactor 1.00 → 11.0px  (home key)
    //   [1] dist=1-2   → sizeFactor 0.85 → 9.35px
    //   [2] dist=3-4   → sizeFactor 0.70 → 7.70px
    //   [3] dist=5-6   → sizeFactor 0.55 → 6.05px
    // Initialized in the constructor body after member initializers.
    juce::Font markerFonts_[4];

    //==========================================================================
    // Note names for planchette display text (12-entry, indexed by semitone)
    //==========================================================================
    static constexpr const char* kDisplayNoteNames[12] =
    {
        "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
    };

    //==========================================================================
    // Helper: format "Key · Pct%" and push to planchette
    //==========================================================================
    void updatePlanchetteText()
    {
        const int key = HarmonicField::positionToKey (circleX_);
        const int pct = juce::roundToInt (influenceY_ * 100.0f);
        const juce::String noteName (kDisplayNoteNames[key]);
        planchette_.setDisplayText (noteName + " \xc2\xb7 " + juce::String (pct) + "%");
    }

    //==========================================================================
    // Noise texture generation (deterministic, seed 42)
    //==========================================================================
    void generateNoiseTexture()
    {
        constexpr int kNoiseW = 128;
        constexpr int kNoiseH = 128;

        noiseImage_ = juce::Image (juce::Image::ARGB, kNoiseW, kNoiseH, true);

        std::mt19937 rng (42u);
        std::uniform_int_distribution<int> dist (0, 255);

        juce::Image::BitmapData bmp (noiseImage_,
                                     juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < kNoiseH; ++y)
        {
            for (int x = 0; x < kNoiseW; ++x)
            {
                const uint8_t v = static_cast<uint8_t> (dist (rng));
                // ARGB: full alpha, same value for R/G/B (grayscale)
                bmp.setPixelColour (x, y,
                    juce::Colour (static_cast<uint8_t> (255), v, v, v));
            }
        }
    }

    //==========================================================================
    // Marker rendering
    //==========================================================================
    void paintMarkers (juce::Graphics& g)
    {
        const auto b = harmonicSurfaceBounds_.toFloat();
        if (b.isEmpty())
            return;

        // Marker band baseline: 40% from top of the harmonic surface
        const float baselineY = b.getY() + b.getHeight() * 0.40f;

        // Base font size for the "full" (home) marker (used for text sizing only)
        constexpr float kBaseFontSize = 11.0f;

        // Current key — used to calculate tension distance for each marker
        const int currentKey = HarmonicField::positionToKey (circleX_);

        for (int idx = 0; idx < 13; ++idx)
        {
            // Normalised X position of this marker
            const float normX = static_cast<float> (idx) / 12.0f;

            // Screen X
            const float markerX = b.getX() + normX * b.getWidth();

            // Parabolic arc vertical offset (positive = downward in screen coords)
            const float arcOffset = HarmonicField::markerArcY (idx);

            // Screen Y
            const float markerY = baselineY + arcOffset;

            // Tension distance from current key to this marker's key
            const int markerKey = HarmonicField::kFifthsSemitones[
                static_cast<std::size_t> (idx)];
            const int dist = HarmonicField::fifthsDistance (currentKey, markerKey);

            // Size and opacity
            const auto [sizeFactor, opacity] = HarmonicField::markerProperties (dist);
            const float fontSize = kBaseFontSize * sizeFactor;

            // Tension color
            const auto [r, g2, b2] = HarmonicField::tensionColor (dist);
            const juce::Colour markerColour = juce::Colour::fromFloatRGBA (r, g2, b2, opacity);

            // Label text
            const juce::String label (HarmonicField::kNoteNames[
                static_cast<std::size_t> (idx)]);

            // Select the cached font bracket (Fix #10: avoid per-marker Font construction).
            // Brackets match HarmonicField::markerProperties() size factor tiers.
            int fontBracket = 3;
            if      (dist == 0) fontBracket = 0;
            else if (dist <= 2) fontBracket = 1;
            else if (dist <= 4) fontBracket = 2;

            // Draw
            g.setColour (markerColour);
            g.setFont (markerFonts_[fontBracket]);

            // Centre the text at (markerX, markerY)
            const float textW = fontSize * 2.2f;   // generous width for "F#" etc.
            const float textH = fontSize + 2.0f;
            g.drawText (label,
                        juce::Rectangle<float> (markerX - textW * 0.5f,
                                                markerY - textH * 0.5f,
                                                textW,
                                                textH),
                        juce::Justification::centred,
                        false);
        }
    }

    //==========================================================================
    // YES / NO labels
    //==========================================================================
    void paintYesNoLabels (juce::Graphics& g)
    {
        const auto b = getLocalBounds().toFloat();
        // Opacity raised 0.20 → 0.45: still subtle but now readable (UIX Fix 2)
        const juce::Colour labelColour = juce::Colours::white.withAlpha (0.45f);
        g.setColour (labelColour);
        // Static local font — constructed once, shared across all paint() calls
        // Font size raised 9px → 11px for improved legibility (UIX Fix 2)
        static const juce::Font kYesNoFont ("Georgia", 11.0f, juce::Font::italic);
        g.setFont (kYesNoFont);

        // YES — near the top of the component
        const float yesY = b.getY() + 10.0f;
        g.drawText ("YES",
                    juce::Rectangle<float> (b.getX(), yesY, b.getWidth(), 14.0f),
                    juce::Justification::centred,
                    false);

        // NO — above the bottom reserved area
        const float reservedBottom = static_cast<float> (kGestureBarH + kGoodbyeH);
        const float noY = b.getBottom() - reservedBottom - 18.0f;
        g.drawText ("NO",
                    juce::Rectangle<float> (b.getX(), noY, b.getWidth(), 14.0f),
                    juce::Justification::centred,
                    false);
    }

    //==========================================================================
    // Bioluminescent trail rendering (B043 — spec Section 4.5)
    //==========================================================================
    void paintTrail (juce::Graphics& g)
    {
        const int n = trailBuffer_.count();
        if (n == 0)
            return;

        const float w = static_cast<float> (getWidth());
        const float h = static_cast<float> (getHeight());
        const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;

        // ------------------------------------------------------------------
        // Frozen trail replay at 50% opacity (all frozen points, 4px, 25% alpha)
        // ------------------------------------------------------------------
        if (trailBuffer_.isFrozen())
        {
            const int fc = trailBuffer_.frozenCount();
            for (int i = 0; i < fc; ++i)
            {
                const TrailPoint pt = trailBuffer_.frozenPointAt (i);
                const float px = pt.x * w;
                const float py = (1.0f - pt.y) * h;
                const float radius = 2.0f;  // 4px diameter

                g.setColour (accentColour_.withAlpha (0.25f));
                g.fillEllipse (px - radius, py - radius,
                               radius * 2.0f, radius * 2.0f);
            }
        }

        // ------------------------------------------------------------------
        // Live trail: up to 12 most recent points, exponential fade over 1.5s
        // ------------------------------------------------------------------
        const int maxPoints = std::min (n, 12);
        for (int age = 0; age < maxPoints; ++age)
        {
            const TrailPoint pt = trailBuffer_.pointByAge (age);
            const double ptAge = now - pt.timestamp;

            // Skip if older than 1.5 seconds
            if (ptAge > 1.5)
                continue;

            // Alpha: exponential decay — reaches near-zero at 1.5s
            const float alpha = static_cast<float> (std::exp (-ptAge * 2.0));

            // Pixel position (Y-flipped: normalised 0=bottom, 1=top)
            const float px = pt.x * w;
            const float py = (1.0f - pt.y) * h;

            // Core radius: shrinks from ~5px to 1px as alpha decays
            const float coreRadius = 5.0f * alpha + 1.0f;

            // --- Pass A: outer glow (3× radius, 50% of alpha) ---
            const float glowRadius = coreRadius * 3.0f;
            g.setColour (accentColour_.withAlpha (alpha * 0.50f));
            g.fillEllipse (px - glowRadius, py - glowRadius,
                           glowRadius * 2.0f, glowRadius * 2.0f);

            // --- Pass B: core (1× radius, full alpha) ---
            g.setColour (accentColour_.withAlpha (alpha));
            g.fillEllipse (px - coreRadius, py - coreRadius,
                           coreRadius * 2.0f, coreRadius * 2.0f);
        }
    }

    //==========================================================================
    // Mouse → normalised position
    //==========================================================================

    /** Converts a mouse event to {circleX, influenceY}.
        circleX : left=0, right=1
        influenceY: top=1 (YES), bottom=0 (NO)  — inverted from screen coords. */
    std::pair<float, float> mouseToNormalized (const juce::MouseEvent& e) const
    {
        const auto b = harmonicSurfaceBounds_.toFloat();
        if (b.isEmpty())
            return { circleX_, influenceY_ };

        const float mx = static_cast<float> (e.getPosition().x);
        const float my = static_cast<float> (e.getPosition().y);

        const float nx = juce::jlimit (0.0f, 1.0f,
                                       (mx - b.getX()) / b.getWidth());

        // Inverted: top of surface → 1.0 (YES), bottom → 0.0 (NO)
        const float ny = juce::jlimit (0.0f, 1.0f,
                                       1.0f - (my - b.getY()) / b.getHeight());

        return { nx, ny };
    }

    //==========================================================================
    // Fire onPositionChanged and onCCOutput callbacks
    //==========================================================================
    void fireCallbacks()
    {
        if (onPositionChanged)
            onPositionChanged (circleX_, influenceY_);

        if (onCCOutput)
        {
            // CC 85: circle-of-fifths position (0–127)
            const auto ccX = static_cast<uint8_t> (
                juce::roundToInt (circleX_ * 127.0f));
            // CC 86: influence depth (0–127)
            const auto ccY = static_cast<uint8_t> (
                juce::roundToInt (influenceY_ * 127.0f));

            onCCOutput (85, ccX);
            onCCOutput (86, ccY);
        }
    }

    //==========================================================================
    // Setup default XOuija button bank (FREEZE / HOME / DRIFT)
    //==========================================================================
    void setupDefaultButtonBank()
    {
        std::array<GestureButtonBar::ButtonDef, 3> defs;

        // Slot 0: FREEZE (F) — toggle trail freeze/unfreeze + CC 88
        defs[0].label       = "FREEZE [F]";
        defs[0].shortcutKey = 'F';
        defs[0].action      = [this]()
        {
            if (trailBuffer_.isFrozen())
            {
                trailBuffer_.unfreeze();
                if (onCCOutput) onCCOutput (88, 0);
            }
            else
            {
                trailBuffer_.freeze();
                if (onCCOutput) onCCOutput (88, 127);
            }
            repaint();
        };

        // Slot 1: HOME (H) — snap planchette to centre + fire position callbacks
        defs[1].label       = "HOME [H]";
        defs[1].shortcutKey = 'H';
        defs[1].action      = [this]()
        {
            planchette_.snapHome();
            circleX_    = 0.5f;
            influenceY_ = 0.5f;
            updatePlanchetteText();
            fireCallbacks();
            // CC 89→127 signals "home" event
            if (onCCOutput)
            {
                onCCOutput (85, 64);
                onCCOutput (86, 64);
                onCCOutput (89, 127);
            }
            repaint();
        };

        // Slot 2: DRIFT (D) — toggle Lissajous drift + CC 90
        defs[2].label       = "DRIFT [D]";
        defs[2].shortcutKey = 'D';
        defs[2].action      = [this]()
        {
            const bool newState = ! planchette_.isDriftEnabled();
            planchette_.setDriftEnabled (newState);
            if (onCCOutput) onCCOutput (90, newState ? 127 : 0);
        };

        gestureButtons_.setButtons (defs);
    }

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XOuijaPanel)
};

} // namespace xolokun
