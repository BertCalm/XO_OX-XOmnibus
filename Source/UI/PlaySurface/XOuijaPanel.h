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

    Mouse input:
      - mouseDown/mouseDrag: updates circleX_ and influenceY_, fires
        onPositionChanged, and emits CC 85 (circleX) and CC 86 (influenceY)
        via onCCOutput
      - mouseUp: clears touching_ flag

    Layout reservations (for Tasks 5-7):
      - Bottom 34px: future GestureButtonBar
      - Bottom 32px above that: future GOODBYE button
      - Harmonic surface occupies remaining area above

    B042 — Task 4 of 13 (XOuija PlaySurface V2)
    Namespace: xolokun
    JUCE 8, C++17
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <functional>
#include <cstdint>
#include <utility>
#include <random>

#include "HarmonicField.h"

namespace xolokun {

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

        // Future tasks will call setBounds() on child components here.
        // (No child components exist in Task 4.)
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
    }

    //==========================================================================
    // Mouse handling
    //==========================================================================

    void mouseDown (const juce::MouseEvent& e) override
    {
        touching_ = true;
        updateFromMouse (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        updateFromMouse (e);
    }

    void mouseUp (const juce::MouseEvent& /*e*/) override
    {
        touching_ = false;
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
    // Update state from mouse and fire callbacks
    //==========================================================================
    void updateFromMouse (const juce::MouseEvent& e)
    {
        auto [nx, ny] = mouseToNormalized (e);
        circleX_    = nx;
        influenceY_ = ny;

        repaint();

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
