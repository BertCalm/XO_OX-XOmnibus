#pragma once

/*
    KeysMode.h
    ==========
    Seaboard-style 2-octave scrollable keyboard with Y-velocity, X-pitch-glide,
    and XOuija-reactive coloring.

    Spec Section 8.4 -- KeysMode (Desktop)

    Namespace: xolokun
    JUCE 8, C++17
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <array>
#include <cmath>

#include "HarmonicField.h"

namespace xolokun {

//==============================================================================
/**
    KeysMode -- a 2-octave scrollable Seaboard-style keyboard.

    - 2 octaves visible, scrollable C1-C7
    - Y-position -> velocity (top = 127, bottom = 1)
    - X-drag from press origin -> pitch glide / pitch wheel
    - XOuija-reactive coloring via HarmonicField
*/
class KeysMode : public juce::Component
{
public:
    //==========================================================================
    // Constants

    static constexpr int kOctavesVisible   = 2;
    static constexpr int kNaturalKeysVisible = 14; // 2 octaves x 7 natural keys
    static constexpr int kSemitonesVisible   = 24; // 2 x 12
    static constexpr int kOctaveMin = 1;
    static constexpr int kOctaveMax = 5; // so 2 visible octaves fit C1-C7

    // Colors
    static constexpr uint32_t kNaturalKeyColor = 0xFFF0EDE8; // Warm white
    static constexpr uint32_t kSharpKeyColor   = 0xFF2A2A2A; // Charcoal
    static constexpr uint32_t kXoGold          = 0xFFE9C46A; // Root note marker

    // Key proportions
    static constexpr float kSharpWidthRatio  = 0.60f;
    static constexpr float kSharpHeightRatio = 0.60f;

    // Pitch glide: full +-1 over 1/4 of the panel width
    static constexpr float kPitchGlideSensitivity = 4.0f;

    //==========================================================================
    KeysMode()
    {
        accent_ = juce::Colour (kXoGold);
        setOpaque (false);

        // WCAG Fix 2: accessibility API
        setAccessible (true);
        setTitle ("Keyboard - Seaboard Style");
        setDescription ("Two-octave keyboard with velocity from vertical position and pitch glide from horizontal drag");
    }

    ~KeysMode() override = default;

    //==========================================================================
    // Public API

    /** Set the harmonic root (0=C...11=B) from XOuija. */
    void setRootKey (int rootKey)
    {
        rootKey_ = ((rootKey % 12) + 12) % 12;
        repaint();
    }

    /** Set the accent colour for in-key glow highlights. */
    void setAccentColour (juce::Colour c)
    {
        accent_ = c;
        repaint();
    }

    /** Get the first visible octave (1-5). */
    int getBaseOctave() const noexcept { return baseOctave_; }

    /** Set the first visible octave; clamped to [kOctaveMin, kOctaveMax]. */
    void setBaseOctave (int octave)
    {
        baseOctave_ = juce::jlimit (kOctaveMin, kOctaveMax, octave);
        repaint();
    }

    /** Scroll up by one octave. */
    void octaveUp()   { setBaseOctave (baseOctave_ + 1); }

    /** Scroll down by one octave. */
    void octaveDown() { setBaseOctave (baseOctave_ - 1); }

    /** Returns a display string like "C3-B4". */
    juce::String getVisibleRange() const
    {
        int topNote   = baseMidi() + kSemitonesVisible - 1;
        int topOctave = topNote / 12 - 1;
        return "C" + juce::String (baseOctave_) + "-B" + juce::String (topOctave);
    }

    //==========================================================================
    // Callbacks

    std::function<void(int midiNote, float velocity)> onNoteOn;
    std::function<void(int midiNote)>                  onNoteOff;
    std::function<void(float pitchBend)>               onPitchBend; // -1..+1

    /** If set, MIDI events are enqueued here (thread-safe delivery to audio thread). */
    juce::MidiMessageCollector* midiCollector = nullptr;

    /** MIDI channel for outgoing messages (1-16). */
    int midiChannel = 1;

    //==========================================================================
    // juce::Component overrides

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        float panelW = bounds.getWidth();
        float panelH = bounds.getHeight();

        float naturalW = panelW / static_cast<float> (kNaturalKeysVisible);
        float naturalH = panelH;
        float sharpW   = naturalW * kSharpWidthRatio;
        float sharpH   = naturalH * kSharpHeightRatio;

        int base = baseMidi();

        // Pass 1: draw natural keys (background layer)
        int naturalIndex = 0;
        for (int semi = 0; semi < kSemitonesVisible; ++semi)
        {
            int midiNote   = base + semi;
            int pitchClass = midiNote % 12;

            if (isSharp (pitchClass))
                continue;

            float x = static_cast<float> (naturalIndex) * naturalW;
            juce::Rectangle<float> keyRect (x, 0.0f, naturalW, naturalH);

            drawKey (g, keyRect, midiNote, false, sharpW, sharpH);
            ++naturalIndex;
        }

        // Pass 2: draw sharp keys (foreground layer, on top)
        naturalIndex = 0;
        for (int semi = 0; semi < kSemitonesVisible; ++semi)
        {
            int midiNote   = base + semi;
            int pitchClass = midiNote % 12;

            if (isSharp (pitchClass))
            {
                float leftNaturalLeft = static_cast<float> (naturalIndex - 1) * naturalW;
                float x = leftNaturalLeft + naturalW - sharpW * 0.5f;
                juce::Rectangle<float> keyRect (x, 0.0f, sharpW, sharpH);
                drawKey (g, keyRect, midiNote, true, sharpW, sharpH);
            }
            else
            {
                ++naturalIndex;
            }
        }

        // Range label (small, bottom-right)
        auto labelBounds = getLocalBounds().reduced (4);
        auto labelStrip  = labelBounds.removeFromBottom (16);
        g.setColour (juce::Colour (0xFF888888));
        g.setFont (juce::Font (11.0f));
        g.drawText (getVisibleRange(),
                    labelStrip.removeFromRight (60),
                    juce::Justification::bottomRight, false);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        int hit = hitTestKey (e.x, e.y);
        if (hit < 0)
            return;

        activeNote_   = hit;
        pressOriginX_ = static_cast<float> (e.x);

        float velocity = yToVelocity (static_cast<float> (e.y));
        sendNoteOn (hit, velocity);
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (activeNote_ < 0)
            return;

        float dx        = static_cast<float> (e.x) - pressOriginX_;
        float panelW    = static_cast<float> (getWidth());
        float bendFloat = dx / (panelW / kPitchGlideSensitivity);
        bendFloat       = juce::jlimit (-1.0f, 1.0f, bendFloat);

        if (onPitchBend)
            onPitchBend (bendFloat);

        // Convert to MIDI pitch wheel (0-16383, centre=8192)
        int wheelValue = static_cast<int> (8192.0f + bendFloat * 8191.0f);
        wheelValue     = juce::jlimit (0, 16383, wheelValue);

        if (midiCollector != nullptr)
        {
            auto msg = juce::MidiMessage::pitchWheel (midiChannel, wheelValue);
            midiCollector->addMessageToQueue (msg);
        }
    }

    void mouseUp (const juce::MouseEvent& /*e*/) override
    {
        if (activeNote_ >= 0)
        {
            sendNoteOff (activeNote_);
            activeNote_ = -1;
        }

        // Reset pitch wheel to centre
        if (midiCollector != nullptr)
        {
            auto msg = juce::MidiMessage::pitchWheel (midiChannel, 8192);
            midiCollector->addMessageToQueue (msg);
        }

        if (onPitchBend)
            onPitchBend (0.0f);

        repaint();
    }

    void mouseWheelMove (const juce::MouseEvent& /*e*/,
                         const juce::MouseWheelDetails& wheel) override
    {
        if (wheel.deltaY > 0.0f)
            octaveUp();
        else if (wheel.deltaY < 0.0f)
            octaveDown();
    }

private:
    //==========================================================================
    // State

    int   rootKey_      = 0;   // C
    int   baseOctave_   = 3;   // default C3-B4
    int   activeNote_   = -1;
    float pressOriginX_ = 0.0f;
    juce::Colour accent_;

    //==========================================================================
    // Helpers

    /** MIDI note of the lowest visible key.
        baseMidi = (baseOctave_ + 1) * 12, so baseOctave_=3 -> 48 (C3). */
    int baseMidi() const noexcept { return (baseOctave_ + 1) * 12; }

    /** True if pitchClass (0-11) is a sharp/flat (black key). */
    static bool isSharp (int pitchClass) noexcept
    {
        return pitchClass == 1  || pitchClass == 3  ||
               pitchClass == 6  || pitchClass == 8  || pitchClass == 10;
    }

    /** Map Y pixel position to velocity [1, 127]. top=127, bottom=1. */
    float yToVelocity (float y) const noexcept
    {
        float h = static_cast<float> (getHeight());
        if (h <= 0.0f) return 64.0f;
        float norm     = juce::jlimit (0.0f, 1.0f, y / h);
        return 127.0f - norm * 126.0f;
    }

    //--------------------------------------------------------------------------
    /** Draw a single key with XOuija-reactive coloring. */
    void drawKey (juce::Graphics& g,
                  const juce::Rectangle<float>& rect,
                  int midiNote,
                  bool sharp,
                  float /*sharpW*/, float /*sharpH*/)
    {
        bool inKey  = HarmonicField::isInKey (midiNote, rootKey_);
        bool isRoot = HarmonicField::isRoot  (midiNote, rootKey_);
        bool active = (midiNote == activeNote_);

        // --- Base fill -------------------------------------------------------
        juce::Colour baseColor = sharp ? juce::Colour (kSharpKeyColor)
                                       : juce::Colour (kNaturalKeyColor);

        if (!inKey && !active)
            baseColor = baseColor.withAlpha (0.20f);

        g.setColour (baseColor);
        g.fillRect (rect);

        // --- Accent glow overlay (in-key or active) --------------------------
        if (inKey || active)
        {
            float glowAlpha = active ? 0.30f : 0.12f;
            g.setColour (accent_.withAlpha (glowAlpha));
            g.fillRect (rect);
        }

        // --- Root note: XO Gold bottom border --------------------------------
        if (isRoot)
        {
            float borderH = sharp ? 3.0f : 5.0f;
            juce::Rectangle<float> border (rect.getX(),
                                            rect.getBottom() - borderH,
                                            rect.getWidth(),
                                            borderH);
            g.setColour (juce::Colour (kXoGold));
            g.fillRect (border);
        }

        // --- Key outline (hairline) ------------------------------------------
        g.setColour (sharp ? juce::Colour (0xFF1A1A1A) : juce::Colour (0xFFBBB8B2));
        g.drawRect (rect, 1.0f);
    }

    //--------------------------------------------------------------------------
    /** Hit-test (px, py) -> MIDI note number, or -1 if no key hit.
        Checks sharp keys first (they visually overlap naturals). */
    int hitTestKey (int px, int py) const
    {
        float panelW   = static_cast<float> (getWidth());
        float panelH   = static_cast<float> (getHeight());
        float naturalW = panelW / static_cast<float> (kNaturalKeysVisible);
        float naturalH = panelH;
        float sharpW   = naturalW * kSharpWidthRatio;
        float sharpH   = naturalH * kSharpHeightRatio;
        int   base     = baseMidi();

        // --- Check sharps first ----------------------------------------------
        int naturalIndex = 0;
        for (int semi = 0; semi < kSemitonesVisible; ++semi)
        {
            int midiNote   = base + semi;
            int pitchClass = midiNote % 12;

            if (isSharp (pitchClass))
            {
                float leftNaturalLeft = static_cast<float> (naturalIndex - 1) * naturalW;
                float x = leftNaturalLeft + naturalW - sharpW * 0.5f;
                juce::Rectangle<float> keyRect (x, 0.0f, sharpW, sharpH);
                if (keyRect.contains (static_cast<float> (px), static_cast<float> (py)))
                    return midiNote;
            }
            else
            {
                ++naturalIndex;
            }
        }

        // --- Check naturals --------------------------------------------------
        naturalIndex = 0;
        for (int semi = 0; semi < kSemitonesVisible; ++semi)
        {
            int midiNote   = base + semi;
            int pitchClass = midiNote % 12;

            if (isSharp (pitchClass))
                continue;

            float x = static_cast<float> (naturalIndex) * naturalW;
            juce::Rectangle<float> keyRect (x, 0.0f, naturalW, naturalH);
            if (keyRect.contains (static_cast<float> (px), static_cast<float> (py)))
                return midiNote;

            ++naturalIndex;
        }

        return -1;
    }

    //--------------------------------------------------------------------------
    void sendNoteOn (int midiNote, float velocity)
    {
        if (midiCollector != nullptr)
        {
            auto velByte = static_cast<juce::uint8> (
                juce::jlimit (1, 127, static_cast<int> (velocity)));
            auto msg = juce::MidiMessage::noteOn (midiChannel, midiNote, velByte);
            midiCollector->addMessageToQueue (msg);
        }
        if (onNoteOn)
            onNoteOn (midiNote, velocity / 127.0f);
    }

    void sendNoteOff (int midiNote)
    {
        if (midiCollector != nullptr)
        {
            auto msg = juce::MidiMessage::noteOff (midiChannel, midiNote);
            midiCollector->addMessageToQueue (msg);
        }
        if (onNoteOff)
            onNoteOff (midiNote);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeysMode)
};

} // namespace xolokun
