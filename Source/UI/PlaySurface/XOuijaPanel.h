#pragma once

/*
    XOuijaPanel.h
    =============
    Stub for the XOuija harmonic navigation panel.
    Full implementation added in Task 4; this file must exist for Task 11 to
    compile while the standalone XOuija component is developed separately.

    Public interface required by PlaySurface V2:
      - kMinWidth / kMaxWidth  (static constexpr int)
      - onPositionChanged      (std::function<void(float circleX, float influenceY)>)
      - onGoodbye              (std::function<void()>)
      - setAccentColour(juce::Colour)

    Namespace: xolokun
    JUCE 8, C++17
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace xolokun {

//==============================================================================
/**
    XOuijaPanel — Ouija-board harmonic navigation panel.

    Hosts the planchette (B042), bioluminescent trail (B043), GOODBYE button,
    and circle-of-fifths ruler that drives pad / keys harmonic field coloring.

    Callbacks:
      onPositionChanged(circleX, influenceY)
        circleX  [0..1] = position on circle-of-fifths ruler (left=Gb, right=F#)
        influenceY [0..1] = up-down influence weight (reserved for modulation depth)

      onGoodbye()
        Fired when the GOODBYE button is pressed.
        The parent (PlaySurface) routes this to All Notes Off CC #123.
*/
class XOuijaPanel : public juce::Component
{
public:
    //==========================================================================
    // Width constraints used by PlaySurface::resized()
    static constexpr int kMinWidth = 120;
    static constexpr int kMaxWidth = 220;

    //==========================================================================
    // Callbacks
    std::function<void(float circleX, float influenceY)> onPositionChanged;
    std::function<void()> onGoodbye;

    // CC output callback — fired when the planchette position or influence
    // changes in a way that maps to MIDI CC 85-90 (Spec Section 5.3).
    // PlaySurface wires this to processor_->pushCCOutput() (Task 12).
    std::function<void(uint8_t cc, uint8_t value)> onCCOutput;

    //==========================================================================
    XOuijaPanel()
    {
        accent_ = juce::Colour (0xFFE9C46A); // XO Gold default
    }

    ~XOuijaPanel() override = default;

    //==========================================================================
    void setAccentColour (juce::Colour c)
    {
        accent_ = c;
        repaint();
    }

    //==========================================================================
    // Remote planchette control — called by PlaySurface::handleIncomingCC()
    // to allow external CC (e.g. from a hardware controller) to move the
    // planchette without mouse interaction. Values are normalised [0..1].
    void setCirclePosition (float normX)
    {
        planchetteNorm_ = juce::jlimit (0.0f, 1.0f, normX);
        circleX_ = planchetteNorm_;
        repaint();
        if (onPositionChanged)
            onPositionChanged (planchetteNorm_, influenceY_);
    }

    void setInfluenceDepth (float normY)
    {
        influenceY_ = juce::jlimit (0.0f, 1.0f, normY);
        repaint();
        if (onPositionChanged)
            onPositionChanged (planchetteNorm_, influenceY_);
    }

    //==========================================================================
    // Mod matrix source accessors (Spec Section 5.4).
    // Return normalised [0..1] values suitable for mod matrix routing.
    float getModCircle()    const { return circleX_; }
    float getModInfluence() const { return influenceY_; }
    // Velocity from gesture trail: 0.0 if no trail data yet.
    // Derived from planchette horizontal speed (0=still, 1=fast sweep).
    float getModVelocity()  const { return trailVelocity_; }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        // Background
        g.setColour (juce::Colour (0xFF1A1A1A));
        g.fillRect (b);

        // Border
        g.setColour (accent_.withAlpha (0.25f));
        g.drawRect (b, 1.0f);

        // Label
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (10.0f)));
        g.setColour (accent_.withAlpha (0.55f));
        g.drawText ("XOuija", b.reduced (4), juce::Justification::centredTop);

        // Circle of fifths ruler — horizontal bar in the middle
        float rulerY = b.getCentreY();
        float rulerX = b.getX() + 8.0f;
        float rulerW = b.getWidth() - 16.0f;
        g.setColour (accent_.withAlpha (0.18f));
        g.drawHorizontalLine (static_cast<int>(rulerY), rulerX, rulerX + rulerW);

        // Planchette dot at current position
        float planchetteX = rulerX + planchetteNorm_ * rulerW;
        g.setColour (accent_.withAlpha (0.80f));
        g.fillEllipse (planchetteX - 6.0f, rulerY - 6.0f, 12.0f, 12.0f);

        // GOODBYE button at bottom
        auto goodbyeRect = b.removeFromBottom (28.0f).reduced (6.0f, 4.0f);
        g.setColour (accent_.withAlpha (goodbyeHeld_ ? 0.45f : 0.15f));
        g.fillRoundedRectangle (goodbyeRect, 4.0f);
        g.setColour (accent_.withAlpha (goodbyeHeld_ ? 1.0f : 0.55f));
        g.drawRoundedRectangle (goodbyeRect, 4.0f, 1.0f);
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f)).boldened());
        g.drawText ("GOODBYE", goodbyeRect, juce::Justification::centred);
    }

    //==========================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        auto b = getLocalBounds().toFloat();
        auto goodbyeRect = b.removeFromBottom (28.0f).reduced (6.0f, 4.0f);

        if (goodbyeRect.contains (e.position))
        {
            goodbyeHeld_ = true;
            repaint();
            if (onGoodbye) onGoodbye();
            return;
        }

        updatePlanchette (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        updatePlanchette (e);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        goodbyeHeld_ = false;
        repaint();
    }

private:
    juce::Colour accent_;
    float planchetteNorm_ = 0.5f;  // 0=Gb, 0.5=C, 1=F# — drives the planchette dot
    bool  goodbyeHeld_    = false;

    // Mod matrix sources (Spec Section 5.4)
    float circleX_       = 0.5f;  // mirrors planchetteNorm_; kept as named source
    float influenceY_    = 0.5f;  // up-down influence [0..1] (driven by setInfluenceDepth)
    float trailVelocity_ = 0.0f;  // horizontal drag speed, updated in updatePlanchette

    // Previous planchette position for velocity calculation
    float prevPlanchetteNorm_ = 0.5f;

    void updatePlanchette (const juce::MouseEvent& e)
    {
        auto b = getLocalBounds().toFloat();
        float rulerX = b.getX() + 8.0f;
        float rulerW = b.getWidth() - 16.0f;

        if (rulerW <= 0.0f) return;

        prevPlanchetteNorm_ = planchetteNorm_;
        planchetteNorm_ = juce::jlimit (0.0f, 1.0f,
                                        (static_cast<float>(e.x) - rulerX) / rulerW);
        circleX_ = planchetteNorm_;

        // Track gesture velocity (normalised absolute delta per update)
        trailVelocity_ = juce::jlimit (0.0f, 1.0f,
                                       std::abs (planchetteNorm_ - prevPlanchetteNorm_) * 10.0f);

        repaint();

        if (onPositionChanged)
            onPositionChanged (planchetteNorm_, influenceY_);

        // Emit CC 85 (circle position, 0-127) via onCCOutput callback
        if (onCCOutput)
        {
            auto toMidi = [](float v) -> uint8_t {
                return static_cast<uint8_t>(juce::jlimit (0, 127, static_cast<int>(v * 127.0f)));
            };
            onCCOutput (85, toMidi (planchetteNorm_));
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XOuijaPanel)
};

} // namespace xolokun
