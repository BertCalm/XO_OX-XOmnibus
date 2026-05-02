// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

/*
    KeysMode.h
    ==========
    Seaboard-style 2-octave scrollable keyboard with Y-velocity, X-pitch-glide,
    and harmonic-field-reactive coloring.

    Spec Section 8.4 -- KeysMode (Desktop)

    Namespace: xoceanus
    JUCE 8, C++17
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <array>
#include <cmath>

#include "HarmonicField.h"
#include "../GalleryColors.h"
#include "../Tokens.h" // theme-aware color accessors (#393)

namespace xoceanus
{

//==============================================================================
/**
    KeysMode -- a 2-octave scrollable Seaboard-style keyboard.

    - 2 octaves visible, scrollable C1-C7
    - Y-position -> velocity (top = 127, bottom = 1)
    - X-drag from press origin -> pitch glide / pitch wheel
    - Harmonic-field-reactive coloring via HarmonicField
*/
class KeysMode : public juce::Component
{
public:
    //==========================================================================
    // Constants

    static constexpr int kOctavesVisible = 2;
    static constexpr int kNaturalKeysVisible = 28; // 4 octaves x 7 natural keys
    static constexpr int kSemitonesVisible = 48;   // 4 x 12
    static constexpr int kOctaveMin = 1;
    static constexpr int kOctaveMax = 5; // so 2 visible octaves fit C1-C7

    // Colors
    static constexpr uint32_t kNaturalKeyColor = 0xFF1A1C22; // Dark ocean natural
    static constexpr uint32_t kSharpKeyColor   = 0xFF0C0E12; // Near-black sharp
    static constexpr uint32_t kXoGold = 0xFFE9C46A;          // Root note marker

    // Key proportions
    static constexpr float kSharpWidthRatio = 0.60f;
    static constexpr float kSharpHeightRatio = 0.60f;

    // Pitch glide: full +-1 over 1/4 of the panel width
    static constexpr float kPitchGlideSensitivity = 4.0f;

    //==========================================================================
    KeysMode()
        // Cache range-label font once — avoids per-paint Font construction (UIX Fix 1B)
        : rangeLabelFont_(juce::Font(11.0f))
    {
        accent_ = juce::Colour(kXoGold);
        setOpaque(false);

        // WCAG Fix 2: accessibility API
        setAccessible(true);
        setTitle("Keyboard - Seaboard Style");
        setDescription("Two-octave keyboard with velocity from vertical position and pitch glide from horizontal drag");
    }

    ~KeysMode() override = default;

    //==========================================================================
    // Public API

    /** Set the harmonic root (0=C...11=B). */
    void setRootKey(int rootKey)
    {
        rootKey_ = ((rootKey % 12) + 12) % 12;
        repaint();
    }

    /** Set the accent colour for in-key glow highlights. */
    void setAccentColour(juce::Colour c)
    {
        accent_ = c;
        repaint();
    }

    /** Get the first visible octave (1-5). */
    int getBaseOctave() const noexcept { return baseOctave_; }

    /** Set the first visible octave; clamped to [kOctaveMin, kOctaveMax]. */
    void setBaseOctave(int octave)
    {
        baseOctave_ = juce::jlimit(kOctaveMin, kOctaveMax, octave);
        repaint();
    }

    /** Scroll up by one octave. */
    void octaveUp() { setBaseOctave(baseOctave_ + 1); }

    /** Scroll down by one octave. */
    void octaveDown() { setBaseOctave(baseOctave_ - 1); }

    /** Returns a display string like "C3-B4". */
    juce::String getVisibleRange() const
    {
        int topNote = baseMidi() + kSemitonesVisible - 1;
        int topOctave = topNote / 12 - 1;
        return "C" + juce::String(baseOctave_) + "-B" + juce::String(topOctave);
    }

    //==========================================================================
    // Callbacks

    std::function<void(int midiNote, float velocity)> onNoteOn;
    std::function<void(int midiNote)> onNoteOff;
    std::function<void(float pitchBend)> onPitchBend;   // -1..+1
    std::function<void(float aftertouch)> onAftertouch; // 0..+1

    /** If set, MIDI events are enqueued here (thread-safe delivery to audio thread). */
    juce::MidiMessageCollector* midiCollector = nullptr;

    /** MIDI channel for outgoing messages (1-16). */
    int midiChannel = 1;

    //==========================================================================
    // juce::Component overrides

    // WCAG 2.1.1: Page Up / Page Down shift the visible octave range (#385).
    // Keyboard-only users can scroll the key surface without a mouse wheel.
    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::pageUpKey)
        {
            octaveUp();
            return true;
        }
        if (key == juce::KeyPress::pageDownKey)
        {
            octaveDown();
            return true;
        }
        return false;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        float panelW = bounds.getWidth();
        float panelH = bounds.getHeight();

        float naturalW = panelW / static_cast<float>(kNaturalKeysVisible);
        float naturalH = panelH;
        float sharpW = naturalW * kSharpWidthRatio;
        float sharpH = naturalH * kSharpHeightRatio;

        int base = baseMidi();

        // Pass 1: draw natural keys (background layer)
        int naturalIndex = 0;
        for (int semi = 0; semi < kSemitonesVisible; ++semi)
        {
            int midiNote = base + semi;
            int pitchClass = midiNote % 12;

            if (isSharp(pitchClass))
                continue;

            float x = static_cast<float>(naturalIndex) * naturalW;
            juce::Rectangle<float> keyRect(x, 0.0f, naturalW, naturalH);

            drawKey(g, keyRect, midiNote, false, sharpW, sharpH);
            ++naturalIndex;
        }

        // Pass 2: draw sharp keys (foreground layer, on top)
        naturalIndex = 0;
        for (int semi = 0; semi < kSemitonesVisible; ++semi)
        {
            int midiNote = base + semi;
            int pitchClass = midiNote % 12;

            if (isSharp(pitchClass))
            {
                float leftNaturalLeft = static_cast<float>(naturalIndex - 1) * naturalW;
                float x = leftNaturalLeft + naturalW - sharpW * 0.5f;
                juce::Rectangle<float> keyRect(x, 0.0f, sharpW, sharpH);
                drawKey(g, keyRect, midiNote, true, sharpW, sharpH);
            }
            else
            {
                ++naturalIndex;
            }
        }

        // ── Expression affordance overlay (#888) ─────────────────────────────
        // Vertical velocity gradient: top = loud (bright accent), bottom = soft.
        // Shows players that Y position controls velocity (Seaboard-style).
        {
            juce::ColourGradient velGrad(accent_.withAlpha(0.06f), bounds.getX(), bounds.getY(),
                                         accent_.withAlpha(0.00f), bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(velGrad);
            g.fillRect(bounds);
        }

        // Velocity affordance label — top-left, very dim
        {
            g.setColour(accent_.withAlpha(0.25f));
            g.setFont(rangeLabelFont_);
            g.drawText(juce::CharPointer_UTF8("\xe2\x86\x91 vel"),
                       juce::Rectangle<float>(4.0f, 2.0f, 32.0f, 12.0f),
                       juce::Justification::centredLeft, false);
        }

        // Pitch-bend affordance: subtle horizontal arrow at the left edge showing
        // that left/right drag after touch-down bends pitch.
        if (activeKeyCount_ == 0)
        {
            g.setColour(accent_.withAlpha(0.18f));
            g.setFont(rangeLabelFont_);
            g.drawText(juce::CharPointer_UTF8("\xe2\x86\x94 bend"),
                       juce::Rectangle<float>(4.0f, bounds.getHeight() - 14.0f, 40.0f, 12.0f),
                       juce::Justification::centredLeft, false);
        }

        // Range label (small, bottom-right)
        auto labelBounds = getLocalBounds().reduced(4);
        auto labelStrip = labelBounds.removeFromBottom(16);
        g.setColour(juce::Colour(0xFF888888));
        g.setFont(rangeLabelFont_); // cached — avoids per-paint Font construction (UIX Fix 1B)
        g.drawText(getVisibleRange(), labelStrip.removeFromRight(60), juce::Justification::bottomRight, false);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int hit = hitTestKey(e.x, e.y);
        if (hit < 0)
            return;

        // Polyphony guard: if already at max voices, ignore
        if (activeKeyCount_ >= kMaxPolyphony)
            return;

        // Add to active keys array
        activeKeys_[activeKeyCount_] = {hit, static_cast<float>(e.x), static_cast<float>(e.y)};
        ++activeKeyCount_;

        float velocity = yToVelocity(static_cast<float>(e.y));
        sendNoteOn(hit, velocity);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (activeKeyCount_ == 0)
            return;

        // Pitch bend driven from the most recently added key's originX
        int lastKeyIdx = activeKeyCount_ - 1;
        float dx = static_cast<float>(e.x) - activeKeys_[lastKeyIdx].originX;
        float panelW = static_cast<float>(getWidth());
        float bendFloat = dx / (panelW / kPitchGlideSensitivity);
        bendFloat = juce::jlimit(-1.0f, 1.0f, bendFloat);

        if (onPitchBend)
            onPitchBend(bendFloat);

        // Convert to MIDI pitch wheel (0-16383, centre=8192)
        // QDD Fix 7: use 8192.0f as full-scale factor so:
        //   bend=-1 → 8192-8192=0, bend=0 → 8192, bend=+1 → 16384→clamped 16383
        int wheelValue = static_cast<int>(std::round(8192.0f + bendFloat * 8192.0f));
        wheelValue = std::clamp(wheelValue, 0, 16383);

        if (midiCollector != nullptr)
        {
            auto pitchMsg = juce::MidiMessage::pitchWheel(midiChannel, wheelValue);
            pitchMsg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            midiCollector->addMessageToQueue(pitchMsg);
        }

        // Aftertouch from Y-drag (downward = more pressure)
        float dy = static_cast<float>(e.y) - activeKeys_[lastKeyIdx].originY;
        float aftertouch = std::clamp(dy / (static_cast<float>(getHeight()) * 0.3f), 0.0f, 1.0f);
        int atValue = static_cast<int>(aftertouch * 127.0f);

        if (midiCollector != nullptr)
        {
            auto atMsg = juce::MidiMessage::channelPressureChange(midiChannel, atValue);
            atMsg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            midiCollector->addMessageToQueue(atMsg);
        }

        if (onAftertouch)
            onAftertouch(aftertouch);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (activeKeyCount_ == 0)
        {
            repaint();
            return;
        }

        // Find which active key to release — prefer the most recently added that
        // hit-tests to the current release position; fall back to the last slot.
        int hitNote = hitTestKey(e.x, e.y);
        int releaseIdx = activeKeyCount_ - 1; // default: most recent

        for (int i = activeKeyCount_ - 1; i >= 0; --i)
        {
            if (activeKeys_[i].midiNote == hitNote)
            {
                releaseIdx = i;
                break;
            }
        }

        int releasedNote = activeKeys_[releaseIdx].midiNote;
        sendNoteOff(releasedNote);

        // Compact the array: move last slot into the released slot
        if (releaseIdx != activeKeyCount_ - 1)
            activeKeys_[releaseIdx] = activeKeys_[activeKeyCount_ - 1];
        activeKeys_[activeKeyCount_ - 1] = {}; // zero out vacated tail
        --activeKeyCount_;

        // Reset pitch wheel and aftertouch only when all keys released
        if (activeKeyCount_ == 0)
        {
            if (midiCollector != nullptr)
            {
                auto pitchMsg = juce::MidiMessage::pitchWheel(midiChannel, 8192);
                pitchMsg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
                midiCollector->addMessageToQueue(pitchMsg);

                auto atMsg = juce::MidiMessage::channelPressureChange(midiChannel, 0);
                atMsg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
                midiCollector->addMessageToQueue(atMsg);
            }

            if (onPitchBend)
                onPitchBend(0.0f);

            if (onAftertouch)
                onAftertouch(0.0f);
        }

        repaint();
    }

    void mouseWheelMove(const juce::MouseEvent& /*e*/, const juce::MouseWheelDetails& wheel) override
    {
        if (wheel.deltaY > 0.0f)
            octaveUp();
        else if (wheel.deltaY < 0.0f)
            octaveDown();
    }

private:
    //==========================================================================
    // State

    struct ActiveKey
    {
        int midiNote = -1;
        float originX = 0.0f;
        float originY = 0.0f;
    };

    static constexpr int kMaxPolyphony = 8;

    int rootKey_ = 0;    // C
    int baseOctave_ = 3; // default C3-B4
    std::array<ActiveKey, kMaxPolyphony> activeKeys_{};
    int activeKeyCount_ = 0;
    juce::Colour accent_;

    // Cached font — initialized in constructor, avoids per-paint construction (UIX Fix 1B)
    juce::Font rangeLabelFont_;

    //==========================================================================
    // Helpers

    /** MIDI note of the lowest visible key.
        baseMidi = (baseOctave_ + 1) * 12, so baseOctave_=3 -> 48 (C3). */
    int baseMidi() const noexcept { return (baseOctave_ + 1) * 12; }

    /** True if pitchClass (0-11) is a sharp/flat (black key). */
    static bool isSharp(int pitchClass) noexcept
    {
        return pitchClass == 1 || pitchClass == 3 || pitchClass == 6 || pitchClass == 8 || pitchClass == 10;
    }

    /** Map Y pixel position to velocity [1, 127]. top=127, bottom=1. */
    float yToVelocity(float y) const noexcept
    {
        float h = static_cast<float>(getHeight());
        if (h <= 0.0f)
            return 64.0f;
        float norm = juce::jlimit(0.0f, 1.0f, y / h);
        return 127.0f - norm * 126.0f;
    }

    //--------------------------------------------------------------------------
    /** Draw a single key with harmonic-field-reactive coloring. */
    void drawKey(juce::Graphics& g, const juce::Rectangle<float>& rect, int midiNote, bool sharp, float /*sharpW*/,
                 float /*sharpH*/)
    {
        bool inKey = HarmonicField::isInKey(midiNote, rootKey_);
        bool isRoot = HarmonicField::isRoot(midiNote, rootKey_);

        bool active = false;
        for (int ki = 0; ki < activeKeyCount_; ++ki)
            if (activeKeys_[ki].midiNote == midiNote)
            {
                active = true;
                break;
            }

        // Rounded key rectangle
        const float cornerR = 4.0f;

        // --- Base fill (dark theme — ocean keyboard) -------------------------
        if (sharp)
        {
            // Black keys: near-black with subtle gradient
            juce::ColourGradient sharpGrad(
                juce::Colour(0xFF0C0E12).withAlpha(0.92f), rect.getX(), rect.getY(),
                juce::Colour(0xFF080A0E).withAlpha(0.96f), rect.getX(), rect.getBottom(), false);
            g.setGradientFill(sharpGrad);
            g.fillRoundedRectangle(rect, cornerR);

            // Border
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.04f));
            g.drawRoundedRectangle(rect, cornerR, 1.0f);
        }
        else
        {
            // White keys: dark with subtle gradient
            juce::ColourGradient natGrad(
                juce::Colour(200, 204, 216).withAlpha(0.10f), rect.getX(), rect.getY(),
                juce::Colour(200, 204, 216).withAlpha(0.05f), rect.getX(), rect.getBottom(), false);
            g.setGradientFill(natGrad);
            g.fillRoundedRectangle(rect, cornerR);

            // Border
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.07f));
            g.drawRoundedRectangle(rect, cornerR, 1.0f);
        }

        // --- Hover/active glow (teal for white, purple for black) ------------
        if (active)
        {
            if (sharp)
            {
                // Purple active glow for black keys
                juce::ColourGradient activeGrad(
                    juce::Colour(140, 100, 220).withAlpha(0.35f), rect.getX(), rect.getY(),
                    juce::Colour(140, 100, 220).withAlpha(0.18f), rect.getX(), rect.getBottom(), false);
                g.setGradientFill(activeGrad);
                g.fillRoundedRectangle(rect, cornerR);
            }
            else
            {
                // Teal active glow for white keys
                juce::ColourGradient activeGrad(
                    XO::Tokens::Color::accent().withAlpha(0.28f), rect.getX(), rect.getY(),
                    XO::Tokens::Color::accent().withAlpha(0.14f), rect.getX(), rect.getBottom(), false);
                g.setGradientFill(activeGrad);
                g.fillRoundedRectangle(rect, cornerR);
            }

            // Pressure glow from bottom (like prototype's .pressure div)
            juce::Colour pressureColor = sharp
                ? juce::Colour(140, 100, 220).withAlpha(0.35f)
                : XO::Tokens::Color::accent().withAlpha(0.35f);
            juce::ColourGradient pressGrad(
                pressureColor, rect.getX(), rect.getBottom(),
                juce::Colours::transparentBlack, rect.getX(), rect.getBottom() - rect.getHeight() * 0.6f, false);
            g.setGradientFill(pressGrad);
            g.fillRoundedRectangle(rect, cornerR);

            // Inset box shadow effect
            juce::Colour shadowColor = sharp
                ? juce::Colour(140, 100, 220).withAlpha(0.2f)
                : XO::Tokens::Color::accent().withAlpha(0.2f);
            g.setColour(shadowColor);
            g.drawRoundedRectangle(rect.reduced(1.0f), cornerR, 2.0f);
        }
        else if (inKey)
        {
            // Subtle in-key highlight
            float glowAlpha = 0.08f;
            g.setColour(accent_.withAlpha(glowAlpha));
            g.fillRoundedRectangle(rect, cornerR);
        }

        // --- Root note: XO Gold bottom border --------------------------------
        if (isRoot)
        {
            float borderH = sharp ? 3.0f : 5.0f;
            juce::Rectangle<float> border(rect.getX() + 2.0f, rect.getBottom() - borderH,
                                           rect.getWidth() - 4.0f, borderH);
            g.setColour(juce::Colour(0xFFE9C46A)); // XO Gold
            g.fillRoundedRectangle(border, 1.5f);
        }

        // --- Note label on white keys (like prototype) -----------------------
        if (!sharp)
        {
            int pitchClass = midiNote % 12;
            int octave = midiNote / 12 - 1;
            static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            juce::String label = juce::String(noteNames[pitchClass]) + juce::String(octave);

            g.setFont(juce::Font(juce::FontOptions(8.0f)));
            g.setColour(juce::Colour(200, 204, 216).withAlpha(active ? 0.45f : 0.18f));
            g.drawText(label,
                       juce::Rectangle<float>(rect.getX(), rect.getBottom() - 16.0f,
                                              rect.getWidth(), 14.0f).toNearestInt(),
                       juce::Justification::centred, false);
        }
    }

    //--------------------------------------------------------------------------
    /** Hit-test (px, py) -> MIDI note number, or -1 if no key hit.
        Checks sharp keys first (they visually overlap naturals). */
    int hitTestKey(int px, int py) const
    {
        float panelW = static_cast<float>(getWidth());
        float panelH = static_cast<float>(getHeight());
        float naturalW = panelW / static_cast<float>(kNaturalKeysVisible);
        float naturalH = panelH;
        float sharpW = naturalW * kSharpWidthRatio;
        float sharpH = naturalH * kSharpHeightRatio;
        int base = baseMidi();

        // --- Check sharps first ----------------------------------------------
        int naturalIndex = 0;
        for (int semi = 0; semi < kSemitonesVisible; ++semi)
        {
            int midiNote = base + semi;
            int pitchClass = midiNote % 12;

            if (isSharp(pitchClass))
            {
                float leftNaturalLeft = static_cast<float>(naturalIndex - 1) * naturalW;
                float x = leftNaturalLeft + naturalW - sharpW * 0.5f;
                juce::Rectangle<float> keyRect(x, 0.0f, sharpW, sharpH);
                if (keyRect.contains(static_cast<float>(px), static_cast<float>(py)))
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
            int midiNote = base + semi;
            int pitchClass = midiNote % 12;

            if (isSharp(pitchClass))
                continue;

            float x = static_cast<float>(naturalIndex) * naturalW;
            juce::Rectangle<float> keyRect(x, 0.0f, naturalW, naturalH);
            if (keyRect.contains(static_cast<float>(px), static_cast<float>(py)))
                return midiNote;

            ++naturalIndex;
        }

        return -1;
    }

    //--------------------------------------------------------------------------
    void sendNoteOn(int midiNote, float velocity)
    {
        if (midiCollector != nullptr)
        {
            auto velByte = static_cast<juce::uint8>(juce::jlimit(1, 127, static_cast<int>(velocity)));
            auto msg = juce::MidiMessage::noteOn(midiChannel, midiNote, velByte);
            msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            midiCollector->addMessageToQueue(msg);
        }
        else if (onNoteOn)
            onNoteOn(midiNote, velocity / 127.0f);
    }

    void sendNoteOff(int midiNote)
    {
        if (midiCollector != nullptr)
        {
            auto msg = juce::MidiMessage::noteOff(midiChannel, midiNote);
            msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            midiCollector->addMessageToQueue(msg);
        }
        else if (onNoteOff)
            onNoteOff(midiNote);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeysMode)
};

} // namespace xoceanus
