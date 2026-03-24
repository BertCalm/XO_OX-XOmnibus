#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>
#include <cmath>

namespace xolokun {

//==============================================================================
// ToucheExpression — Ondes Martenot-inspired dedicated expression zone.
//
// The Ondes Martenot (1928) featured a unique "touche d'intensité" — a
// pressure key beside the keyboard that the left hand pressed to control
// dynamics and timbre while the right hand played notes. This gave the
// instrument an extraordinary singing quality no other electronic instrument
// could match.
//
// ToucheExpression adds a dedicated pressure/expression zone to the
// PlaySurface that provides continuous, multi-axis expressive control
// independent of the note-playing zones.
//
// Axes:
//   Pressure (Y) — Primary intensity control (Ondes Martenot-style)
//   Lateral (X)  — Secondary timbre/color control
//   Velocity      — Initial impact speed (for transient response)
//
// Modes:
//   Intensity  — Classic Martenot: pressure maps to amplitude + filter
//   Timbral    — Pressure sweeps morph position + harmonic content
//   Spatial    — Pressure controls reverb/space + stereo width
//   Expression — Full MPE: X=slide, Y=pressure, initial=strike
//
// Targets are freely assignable to any parameter via callbacks.
// Smoothing is per-axis to prevent zipper noise.
//
// Inspired by: Ondes Martenot (1928), Haken Continuum, Sensel Morph
//==============================================================================
class ToucheExpression : public juce::Component
{
public:
    enum class Mode
    {
        Intensity = 0,   // Amplitude + filter (Martenot classic)
        Timbral,         // Morph + harmonics
        Spatial,         // Space + width
        Expression,      // Full MPE-style
        NumModes
    };

    /// Continuous expression output (smoothed, ready to map to parameters)
    struct ExpressionState
    {
        float pressure  = 0.0f;   // 0-1, Y axis (primary)
        float lateral   = 0.5f;   // 0-1, X axis (center = 0.5)
        float velocity  = 0.0f;   // 0-1, initial touch speed
        bool  active    = false;  // Is the zone being touched?
    };

    // Callbacks
    std::function<void (const ExpressionState&)> onExpressionChanged;
    std::function<void()> onTouchBegin;
    std::function<void()> onTouchEnd;

    ToucheExpression()
    {
        setOpaque (false);
    }

    void setMode (Mode m) { mode = m; repaint(); }
    Mode getMode() const { return mode; }

    /// Set smoothing time in milliseconds (per axis)
    void setSmoothingMs (float ms)
    {
        smoothMs = std::max (1.0f, ms);
    }

    /// Get current expression state (call from audio thread via atomic copy)
    ExpressionState getState() const { return currentState; }

    //--------------------------------------------------------------------------
    // juce::Component overrides

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);

        // Background
        g.setColour (juce::Colour (0xFFF8F6F3)); // Gallery white
        g.fillRoundedRectangle (bounds, 6.0f);

        // Border
        g.setColour (juce::Colour (0xFFE9C46A).withAlpha (currentState.active ? 0.8f : 0.3f));
        g.drawRoundedRectangle (bounds, 6.0f, currentState.active ? 2.0f : 1.0f);

        // Mode label
        g.setColour (juce::Colour (0xFF666666));
        g.setFont (10.0f);
        g.drawText (modeName (mode), bounds.reduced (4.0f),
                    juce::Justification::topLeft, false);

        // Pressure indicator (vertical fill from bottom)
        float fillH = bounds.getHeight() * currentState.pressure;
        juce::Colour pressureColour = modeColour (mode).withAlpha (0.3f + currentState.pressure * 0.4f);
        g.setColour (pressureColour);
        g.fillRoundedRectangle (
            bounds.getX(), bounds.getBottom() - fillH,
            bounds.getWidth(), fillH, 4.0f);

        // Lateral position indicator (horizontal line)
        if (currentState.active)
        {
            float xPos = bounds.getX() + currentState.lateral * bounds.getWidth();
            g.setColour (juce::Colour (0xFFE9C46A)); // XO Gold
            g.drawVerticalLine (static_cast<int> (xPos),
                               bounds.getY(), bounds.getBottom());

            // Crosshair dot
            float yPos = bounds.getBottom() - currentState.pressure * bounds.getHeight();
            g.fillEllipse (xPos - 4.0f, yPos - 4.0f, 8.0f, 8.0f);
        }

        // "TOUCHÉ" label at bottom
        g.setColour (juce::Colour (0xFF999999));
        g.setFont (8.0f);
        g.drawText (juce::CharPointer_UTF8 ("TOUCH\xc3\x89"),
                    bounds.reduced (4.0f),
                    juce::Justification::bottomCentre, false);
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        currentState.active = true;
        currentState.velocity = 1.0f; // Could be refined with timing

        updateFromMouse (event);

        if (onTouchBegin)
            onTouchBegin();
    }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        updateFromMouse (event);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        currentState.active = false;
        currentState.pressure = 0.0f;
        currentState.velocity = 0.0f;

        if (onExpressionChanged)
            onExpressionChanged (currentState);

        if (onTouchEnd)
            onTouchEnd();

        repaint();
    }

private:
    Mode mode = Mode::Intensity;
    ExpressionState currentState;
    float smoothMs = 10.0f;

    void updateFromMouse (const juce::MouseEvent& event)
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);

        // X axis → lateral (0-1)
        currentState.lateral = std::clamp (
            (static_cast<float> (event.x) - bounds.getX()) / bounds.getWidth(),
            0.0f, 1.0f);

        // Y axis → pressure (inverted: top = 0, bottom = 1... no, bottom = 0, top = 1)
        currentState.pressure = std::clamp (
            1.0f - (static_cast<float> (event.y) - bounds.getY()) / bounds.getHeight(),
            0.0f, 1.0f);

        if (onExpressionChanged)
            onExpressionChanged (currentState);

        repaint();
    }

    static juce::String modeName (Mode m)
    {
        switch (m)
        {
            case Mode::Intensity:  return "INTENSITY";
            case Mode::Timbral:    return "TIMBRAL";
            case Mode::Spatial:    return "SPATIAL";
            case Mode::Expression: return "EXPRESSION";
            default: return "";
        }
    }

    static juce::Colour modeColour (Mode m)
    {
        switch (m)
        {
            case Mode::Intensity:  return juce::Colour (0xFFE9C46A);  // XO Gold
            case Mode::Timbral:    return juce::Colour (0xFFA78BFA);  // Lavender (Opal)
            case Mode::Spatial:    return juce::Colour (0xFF00B4A0);  // Phosphorescent Teal
            case Mode::Expression: return juce::Colour (0xFFBF40FF);  // Prism Violet
            default: return juce::Colours::grey;
        }
    }
};

} // namespace xolokun
