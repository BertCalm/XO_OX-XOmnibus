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

    Mouse input:
      - mouseDown/mouseDrag: updates circleX_ and influenceY_, fires
        onPositionChanged, and emits CC 85 (circleX) and CC 86 (influenceY)
        via onCCOutput. Also drives Planchette spring/move.
      - mouseUp: clears touching_ flag, triggers planchette release.

    Layout reservations (for Tasks 5-7):
      - Bottom 34px: future GestureButtonBar
      - Bottom 32px above that: future GOODBYE button
      - Harmonic surface occupies remaining area above

    B042 — Task 5 of 13 (XOuija PlaySurface V2)
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
    static constexpr float kSpringMs    = 150.0f;  // spring duration ms
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
        , warmHoldElapsedMs_ (0.0f)
        , driftTimeS_ (0.0f)
        , state_ (State::Lissajous)
        , driftEnabled_ (true)
        , displayText_ ("C · 0%")
    {
        setInterceptsMouseClicks (false, false);
        startTimerHz (60);
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

    /** Called on mouseDown — begin 150ms spring animation to (normX, normY). */
    void springTo (float normX, float normY)
    {
        springStartX_    = displayX_;
        springStartY_    = displayY_;
        springTargetX_   = normX;
        springTargetY_   = normY;
        springElapsedMs_ = 0.0f;
        state_           = State::Springing;
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
        //    Positioned below centre
        g.setColour (accentColour_.withAlpha (0.85f));
        g.setFont (juce::Font ("Georgia", 10.0f, juce::Font::italic));
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
                                              springElapsedMs_ / kSpringMs);
                // Ease-out: 1 - (1-t)^2
                const float ease = 1.0f - (1.0f - t) * (1.0f - t);
                displayX_ = springStartX_ + (springTargetX_ - springStartX_) * ease;
                displayY_ = springStartY_ + (springTargetY_ - springStartY_) * ease;

                if (springElapsedMs_ >= kSpringMs)
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
                if (! driftEnabled_)
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

                // Clamp so planchette stays fully on screen
                displayX_ = juce::jlimit (0.0f, 1.0f, displayX_);
                displayY_ = juce::jlimit (0.0f, 1.0f, displayY_);

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

    // Warm hold
    float         warmHoldElapsedMs_;

    // Drift time accumulator (seconds)
    float         driftTimeS_;

    State         state_;
    bool          driftEnabled_;
    juce::String  displayText_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Planchette)
};

//==============================================================================
class XOuijaPanel : public juce::Component
{
public:
    //==========================================================================
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

        // B042: add the planchette as a child — it manages its own bounds
        addAndMakeVisible (planchette_);
        planchette_.setAccentColour (accentColour_);
        updatePlanchetteText();
    }

    ~XOuijaPanel() override = default;

    //==========================================================================
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
        const auto b = getLocalBounds();
        const int reservedBottom = kGestureBarH + kGoodbyeH;  // 66px

        harmonicSurfaceBounds_ = b.withTrimmedBottom (reservedBottom);

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

        // Base font size for the "full" (home) marker
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

            // Draw
            g.setColour (markerColour);
            g.setFont (juce::Font ("Georgia", fontSize, juce::Font::italic));

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
        const juce::Colour labelColour = juce::Colours::white.withAlpha (0.20f);
        g.setColour (labelColour);
        g.setFont (juce::Font ("Georgia", 9.0f, juce::Font::italic));

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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XOuijaPanel)
};

} // namespace xolokun
