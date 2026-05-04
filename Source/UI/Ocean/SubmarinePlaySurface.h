// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// SubmarinePlaySurface.h — Multi-mode musical input surface for XOceanus Ocean View.
//
// Replaces the Gallery-style PlaySurface with a submarine-prototype-matched component.
// Four modes controlled externally via setMode():
//
//   KEYS — 4-octave MPE keyboard. White keys fill width; black keys overlay at correct
//           positions. Drag across keys fires note-off/note-on transitions. Supports
//           Y-position velocity on press.
//
//   PAD  — 4x4 chromatic note grid (MPC layout). Top row = highest pitch. Teal accent.
//          Velocity derived from Y position within each pad.
//
//   DRUM — 4x4 drum pad grid (GM drum map from MIDI 36). Purple-teal bicolour accent.
//          Named pads: Kick, Snare, Clap, HiHat, … Tamb.
//
//   XY   — 2D control surface. Click/drag moves cursor, fires onXYChanged(x, y) in
//          0-1 normalized range. Crosshair guide lines + axis labels.
//
// All rendering is custom paint — no JUCE widgets used.
// Fully inline header-only, following XOceanus UI component convention.
//
// Usage:
//   auto surf = std::make_unique<SubmarinePlaySurface>();
//   surf->onNoteOn  = [this](int n, float v) { processor_.addNote(n, v); };
//   surf->onNoteOff = [this](int n)           { processor_.removeNote(n); };
//   surf->setMode(SubmarinePlaySurface::Mode::Keys);
//   surf->setAccentColour(engineColour_);
//   addAndMakeVisible(*surf);

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <array>
#include <cmath>
#include "../Tokens.h" // Session 2C #14: submarine keyboard token theming

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

//==============================================================================
/**
    SubmarinePlaySurface

    Multi-mode musical input surface (Keys / Pad / Drum / XY).
    See file header for full documentation.
*/
class SubmarinePlaySurface : public juce::Component
{
public:
    //==========================================================================
    enum class Mode { Keys, Pad, Drum, XY };

    //==========================================================================
    SubmarinePlaySurface()
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, true);
        setWantsKeyboardFocus(false);
    }

    ~SubmarinePlaySurface() override = default;

    //==========================================================================
    // Public API

    void setMode(Mode m)
    {
        if (mode_ == m)
            return;
        // Release any active note when switching modes
        releaseAllNotes();
        mode_ = m;
        repaint();
    }

    Mode getMode() const noexcept { return mode_; }

    void setAccentColour(juce::Colour c)
    {
        accent_ = c;
        repaint();
    }

    /** Set base octave for KEYS mode (default 2 = C2, C2-C5 range). Clamped to [0, 8]. */
    void setOctave(int oct)
    {
        baseOctave_ = juce::jlimit(0, 8, oct);
        releaseAllNotes();
        repaint();
    }

    int getOctave() const noexcept { return baseOctave_; }

    //==========================================================================
    // MIDI output callbacks

    std::function<void(int note, float velocity)> onNoteOn;
    std::function<void(int note)>                 onNoteOff;
    std::function<void(int note, float pressure)> onAftertouch;
    std::function<void(float x, float y)>         onXYChanged; // XY mode, 0-1 range

    //==========================================================================
    // juce::Component overrides

    void paint(juce::Graphics& g) override
    {
        switch (mode_)
        {
            case Mode::Keys: paintKeys(g); break;
            case Mode::Pad:  paintPads(g, false); break;
            case Mode::Drum: paintPads(g, true);  break;
            case Mode::XY:   paintXY(g);   break;
        }
    }

    void resized() override
    {
        computeGridCells();
        if (mode_ == Mode::Keys)
            computeKeyLayout();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        switch (mode_)
        {
            case Mode::Keys: handleKeysDown(e); break;
            case Mode::Pad:
            case Mode::Drum: handlePadDown(e);  break;
            case Mode::XY:   handleXYDown(e);   break;
        }
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        switch (mode_)
        {
            case Mode::Keys: handleKeysDrag(e); break;
            case Mode::Pad:
            case Mode::Drum: break; // pads don't slide
            case Mode::XY:   handleXYDrag(e);   break;
        }
        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        switch (mode_)
        {
            case Mode::Keys: handleKeysUp(e); break;
            case Mode::Pad:
            case Mode::Drum: handlePadUp(e); break;
            case Mode::XY:   handleXYUp();   break;
        }
        repaint();
    }

private:
    //==========================================================================
    // Constants — colors (Session 2C #14: teal via XO::Tokens, purple/salt raw)
    static juce::Colour tokenTeal()       { return XO::Tokens::Color::accent(); }
    static juce::Colour tokenTealBright() { return XO::Tokens::Color::accentBright(); }
    // Teal (retained for accent_ default initializer below)
    static constexpr uint8_t kTealR = 60, kTealG = 180, kTealB = 170;
    // Purple accent (black key active, drum active)
    static constexpr uint8_t kPurpR = 140, kPurpG = 100, kPurpB = 220;
    // Salt text color
    static constexpr uint8_t kSaltR = 200, kSaltG = 204, kSaltB = 216;

    // Keyboard layout constants
    static constexpr int   kOctavesVisible  = 4;
    static constexpr int   kWhiteKeysTotal  = 28; // 4 octaves × 7 white
    static constexpr float kBlackWidthRatio = 0.60f;
    static constexpr float kBlackHeightRatio = 0.55f;
    static constexpr float kKeyGap = 1.0f;
    static constexpr float kKeyPadH = 10.0f;  // top padding
    static constexpr float kKeyPadB = 14.0f;  // bottom padding
    static constexpr float kKeyPadX = 10.0f;  // horizontal padding

    // Pad grid
    static constexpr int   kGridSize = 4;
    static constexpr float kGridGap  = 6.0f;
    static constexpr float kGridPad  = 8.0f;
    static constexpr float kGridRadius = 10.0f;

    // Semitone → is black key
    static bool isBlack(int semitone) noexcept
    {
        // semitone within octave (0=C)
        int pc = semitone % 12;
        return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
    }

    // Drum pad names (GM approximate, 4×4 top-left to bottom-right)
    static constexpr const char* kDrumNames[16] = {
        "Kick",  "Snare", "Clap",  "HiHat",
        "OpenHH","Tom1",  "Tom2",  "Tom3",
        "Rim",   "Cowbel","Crash", "Ride",
        "Perc1", "Perc2", "Shaker","Tamb"
    };

    //==========================================================================
    // State

    Mode           mode_       = Mode::Keys;
    juce::Colour   accent_     = juce::Colour(kTealR, kTealG, kTealB);
    int            baseOctave_ = 2;

    // Keys mode — active key tracking
    int  activeNote_   = -1;  // single active note (keyboard)
    bool keyIsDown_    = false;

    // Pad / Drum mode — active pad index (-1 = none)
    int  activePadIdx_ = -1;

    // XY mode
    float xyX_ = 0.5f;
    float xyY_ = 0.5f;
    bool  xyActive_ = false;

    //==========================================================================
    // Precomputed geometry — rebuilt in resized()

    // White key rects (28 keys for 4 octaves)
    std::array<juce::Rectangle<float>, 28> whiteRects_{};
    // Black key rects (20 keys for 4 octaves)
    struct BlackKey { juce::Rectangle<float> rect; int midiNote; };
    std::array<BlackKey, 20> blackKeys_{};
    int numBlackKeys_ = 0;

    // 4x4 grid cells
    std::array<juce::Rectangle<float>, 16> gridCells_{};

    //==========================================================================
    // Geometry computation

    void computeKeyLayout()
    {
        auto b = getLocalBounds().toFloat();
        float x0   = b.getX()      + kKeyPadX;
        float y0   = b.getY()      + kKeyPadH;
        float xEnd = b.getRight()  - kKeyPadX;
        float yEnd = b.getBottom() - kKeyPadB;

        float totalW = xEnd - x0;
        float totalH = yEnd - y0;

        // Each white key gets an equal share of (totalW - gaps) / 14
        float whiteW = (totalW - kKeyGap * (kWhiteKeysTotal - 1)) / static_cast<float>(kWhiteKeysTotal);

        // Build white key rects
        for (int i = 0; i < kWhiteKeysTotal; ++i)
        {
            float kx = x0 + static_cast<float>(i) * (whiteW + kKeyGap);
            whiteRects_[i] = juce::Rectangle<float>(kx, y0, whiteW, totalH);
        }

        // Black key geometry
        float blackW = whiteW * kBlackWidthRatio;
        float blackH = totalH * kBlackHeightRatio;

        // Pattern of black keys within one octave (positions between white keys):
        // C#=between0-1, D#=between1-2, (skip E-F), F#=between3-4, G#=between4-5, A#=between5-6
        // White key sequence per octave: C,D,E,F,G,A,B  (indices 0-6 mod 7)
        // Black keys sit at the right edge of their left white neighbour - blackW/2
        static const int kBlackAfterWhite[] = { 0, 1, 3, 4, 5 }; // per-octave white indices

        numBlackKeys_ = 0;
        int base = baseMidiForKeys();

        // Walk all octaves (5 black keys per octave × 4 octaves = 20 max)
        static constexpr int kMaxBlackKeys = kOctavesVisible * 5;
        for (int oct = 0; oct < kOctavesVisible && numBlackKeys_ < kMaxBlackKeys; ++oct)
        {
            int whiteOffset = oct * 7; // first white key index for this octave
            for (int bi = 0; bi < 5 && numBlackKeys_ < kMaxBlackKeys; ++bi)
            {
                int wIdx = whiteOffset + kBlackAfterWhite[bi];
                if (wIdx >= kWhiteKeysTotal - 1)
                    break; // don't overshoot

                // Centre of the black key sits at the right boundary of its left neighbour
                float bx = whiteRects_[wIdx].getRight() - blackW * 0.5f - kKeyGap * 0.5f;
                blackKeys_[numBlackKeys_].rect = juce::Rectangle<float>(bx, y0, blackW, blackH);

                // Semitone offsets for black keys in order: 1, 3, 6, 8, 10
                static const int kBlackSemitones[] = { 1, 3, 6, 8, 10 };
                blackKeys_[numBlackKeys_].midiNote = base + oct * 12 + kBlackSemitones[bi];
                ++numBlackKeys_;
            }
        }
    }

    void computeGridCells()
    {
        auto b = getLocalBounds().toFloat();

        // Available area with padding
        float availW = b.getWidth()  - kGridPad * 2.0f;
        float availH = b.getHeight() - kGridPad * 2.0f;

        // Cell size: square, limited by whichever axis is tighter
        float cellSize = std::min((availW - kGridGap * (kGridSize - 1)) / static_cast<float>(kGridSize),
                                   (availH - kGridGap * (kGridSize - 1)) / static_cast<float>(kGridSize));

        // Centre the grid
        float gridW = cellSize * kGridSize + kGridGap * (kGridSize - 1);
        float gridH = cellSize * kGridSize + kGridGap * (kGridSize - 1);
        float startX = b.getX() + (b.getWidth()  - gridW) * 0.5f;
        float startY = b.getY() + (b.getHeight() - gridH) * 0.5f;

        for (int row = 0; row < kGridSize; ++row)
        {
            for (int col = 0; col < kGridSize; ++col)
            {
                int idx = row * kGridSize + col;
                float cx = startX + static_cast<float>(col) * (cellSize + kGridGap);
                float cy = startY + static_cast<float>(row) * (cellSize + kGridGap);
                gridCells_[idx] = juce::Rectangle<float>(cx, cy, cellSize, cellSize);
            }
        }
    }

    //==========================================================================
    // MIDI helpers

    int baseMidiForKeys() const noexcept
    {
        // MIDI = 12*(octave+1) + semitone.  baseOctave_=2 → C2 = 36
        return 12 * (baseOctave_ + 1);
    }

    int midiNoteForWhiteKey(int whiteIdx) const noexcept
    {
        // White key sequence per octave: C=0,D=2,E=4,F=5,G=7,A=9,B=11
        static const int kWhiteSemitones[] = { 0, 2, 4, 5, 7, 9, 11 };
        int oct    = whiteIdx / 7;
        int degree = whiteIdx % 7;
        return baseMidiForKeys() + oct * 12 + kWhiteSemitones[degree];
    }

    int midiNoteForPad(int idx) const noexcept
    {
        // MPC layout: row 0 (top) = highest, row 3 (bottom) = lowest
        // Base MIDI 36 (C2), row step = 4 notes per row
        int row = idx / kGridSize;
        int col = idx % kGridSize;
        int invertedRow = (kGridSize - 1) - row; // row 3 becomes 0 (lowest)
        return 36 + invertedRow * kGridSize + col;
    }

    static int midiNoteForDrum(int idx) noexcept
    {
        // GM drum note map (approximate standard assignments)
        static const int kGmNotes[16] = {
            36, 38, 39, 42,  // Kick, Snare, Clap, Closed HH
            46, 45, 47, 48,  // Open HH, Low Tom, Mid Tom, High Tom
            37, 56, 49, 51,  // Rim, Cowbell, Crash, Ride
            60, 61, 70, 54   // Perc1(HiBongo), Perc2(LowBongo), Shaker, Tamb
        };
        return (idx >= 0 && idx < 16) ? kGmNotes[idx] : 36;
    }

    /** Velocity from Y position within a rectangle. Top = 1.0, bottom = 0.1. */
    static float velFromY(float py, const juce::Rectangle<float>& rect) noexcept
    {
        if (rect.getHeight() <= 0.0f)
            return 0.75f;
        float norm = juce::jlimit(0.0f, 1.0f, (py - rect.getY()) / rect.getHeight());
        return 1.0f - norm * 0.9f; // [0.1, 1.0]
    }

    //==========================================================================
    // Note fire helpers

    void fireNoteOn(int note, float velocity)
    {
        if (onNoteOn)
            onNoteOn(note, velocity);
    }

    void fireNoteOff(int note)
    {
        if (onNoteOff)
            onNoteOff(note);
    }

    void releaseAllNotes()
    {
        if (keyIsDown_ && activeNote_ >= 0)
            fireNoteOff(activeNote_);
        if (activePadIdx_ >= 0)
        {
            int note = (mode_ == Mode::Drum) ? midiNoteForDrum(activePadIdx_)
                                              : midiNoteForPad(activePadIdx_);
            fireNoteOff(note);
        }
        activeNote_    = -1;
        keyIsDown_     = false;
        activePadIdx_  = -1;
        xyActive_      = false;
    }

    //==========================================================================
    // Hit testing

    /** Returns white key index (0-13) or -1. */
    int hitWhiteKey(float px, float py) const
    {
        for (int i = 0; i < kWhiteKeysTotal; ++i)
            if (whiteRects_[i].contains(px, py))
                return i;
        return -1;
    }

    /** Returns black key array index or -1. */
    int hitBlackKey(float px, float py) const
    {
        for (int i = 0; i < numBlackKeys_; ++i)
            if (blackKeys_[i].rect.contains(px, py))
                return i;
        return -1;
    }

    /** Returns MIDI note at (px, py), -1 if none. Blacks checked first. */
    int hitTestKey(float px, float py) const
    {
        int bi = hitBlackKey(px, py);
        if (bi >= 0)
            return blackKeys_[bi].midiNote;
        int wi = hitWhiteKey(px, py);
        if (wi >= 0)
            return midiNoteForWhiteKey(wi);
        return -1;
    }

    /** Returns grid cell index (0-15) or -1. */
    int hitTestGrid(float px, float py) const
    {
        for (int i = 0; i < 16; ++i)
            if (gridCells_[i].contains(px, py))
                return i;
        return -1;
    }

    //==========================================================================
    // Mouse handlers — KEYS

    void handleKeysDown(const juce::MouseEvent& e)
    {
        int note = hitTestKey(static_cast<float>(e.x), static_cast<float>(e.y));
        if (note < 0)
            return;

        // Latch: release previous note before starting new one
        if (activeNote_ >= 0 && activeNote_ != note)
            fireNoteOff(activeNote_);

        activeNote_ = note;
        keyIsDown_  = true;
        float vel = [&]() -> float {
            int bi = hitBlackKey(static_cast<float>(e.x), static_cast<float>(e.y));
            if (bi >= 0)
                return velFromY(static_cast<float>(e.y), blackKeys_[bi].rect);
            int wi = hitWhiteKey(static_cast<float>(e.x), static_cast<float>(e.y));
            if (wi >= 0)
                return velFromY(static_cast<float>(e.y), whiteRects_[wi]);
            return 0.75f;
        }();
        fireNoteOn(activeNote_, vel);
    }

    void handleKeysDrag(const juce::MouseEvent& e)
    {
        if (!keyIsDown_)
            return;

        const float px = static_cast<float>(e.x);
        const float py = static_cast<float>(e.y);

        int note = hitTestKey(px, py);

        // ── Aftertouch emission (#1182) ─────────────────────────────────────
        // While a key is held, Y-position within the current key's rect drives
        // aftertouch pressure (0..1). The callback was previously declared but
        // never invoked — XOceanus could not deliver MPE-style expression from
        // the on-screen keyboard.
        if (activeNote_ >= 0 && onAftertouch)
        {
            juce::Rectangle<float> rect;
            int bi = hitBlackKey(px, py);
            if (bi >= 0)
                rect = blackKeys_[bi].rect;
            else
            {
                int wi = hitWhiteKey(px, py);
                if (wi >= 0)
                    rect = whiteRects_[wi];
            }
            if (rect.getHeight() > 0.0f)
            {
                const float norm = juce::jlimit(0.0f, 1.0f,
                                                (py - rect.getY()) / rect.getHeight());
                // Top of key = max pressure; bottom = 0. Matches velFromY
                // convention so press-deeper reads as harder.
                onAftertouch(activeNote_, 1.0f - norm);
            }
        }

        if (note < 0 || note == activeNote_)
            return;

        // ── Legato key-slide (#1182) ─────────────────────────────────────────
        // Fire the new note-on BEFORE the old note-off so the voice allocator
        // has the chance to tie/steal the voice rather than opening an audible
        // gap between the two notes.
        const int  oldNote = activeNote_;
        activeNote_        = note;
        fireNoteOn(activeNote_, 0.75f);
        fireNoteOff(oldNote);
    }

    void handleKeysUp(const juce::MouseEvent& /*e*/)
    {
        // Latch mode: note sustains after release.  It is silenced when
        // a different key is pressed (handleKeysDown releases the old note)
        // or when the mode changes (releaseAllNotes).
        keyIsDown_ = false;
    }

    //==========================================================================
    // Mouse handlers — PAD / DRUM

    void handlePadDown(const juce::MouseEvent& e)
    {
        int idx = hitTestGrid(static_cast<float>(e.x), static_cast<float>(e.y));
        if (idx < 0)
            return;
        activePadIdx_ = idx;
        float vel = velFromY(static_cast<float>(e.y), gridCells_[idx]);
        int note = (mode_ == Mode::Drum) ? midiNoteForDrum(idx) : midiNoteForPad(idx);
        fireNoteOn(note, vel);
    }

    void handlePadUp(const juce::MouseEvent& /*e*/)
    {
        if (activePadIdx_ < 0)
            return;
        int note = (mode_ == Mode::Drum) ? midiNoteForDrum(activePadIdx_)
                                          : midiNoteForPad(activePadIdx_);
        fireNoteOff(note);
        activePadIdx_ = -1;
    }

    //==========================================================================
    // Mouse handlers — XY

    void updateXY(float px, float py)
    {
        auto b = getLocalBounds().toFloat().reduced(1.0f); // inside border
        xyX_ = juce::jlimit(0.0f, 1.0f, (px - b.getX()) / b.getWidth());
        xyY_ = juce::jlimit(0.0f, 1.0f, (py - b.getY()) / b.getHeight());
        if (onXYChanged)
            onXYChanged(xyX_, xyY_);
    }

    void handleXYDown(const juce::MouseEvent& e)
    {
        xyActive_ = true;
        updateXY(static_cast<float>(e.x), static_cast<float>(e.y));
    }

    void handleXYDrag(const juce::MouseEvent& e)
    {
        if (!xyActive_)
            return;
        updateXY(static_cast<float>(e.x), static_cast<float>(e.y));
    }

    void handleXYUp()
    {
        xyActive_ = false;
    }

    //==========================================================================
    // Paint — KEYS mode

    void paintKeys(juce::Graphics& g)
    {
        // Ensure layout is computed if resized() hasn't been called yet
        if (whiteRects_[0].getWidth() <= 0.0f)
            computeKeyLayout();

        const float cornerR = 4.0f;

        // Pass 1: white keys
        for (int i = 0; i < kWhiteKeysTotal; ++i)
        {
            int note   = midiNoteForWhiteKey(i);
            bool active = (keyIsDown_ && activeNote_ == note);
            auto& r    = whiteRects_[i];

            // Base gradient: rgba(200,204,216,0.1) → rgba(200,204,216,0.05)
            if (!active)
            {
                juce::ColourGradient bg(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.10f),
                                         r.getX(), r.getY(),
                                         juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.05f),
                                         r.getX(), r.getBottom(), false);
                g.setGradientFill(bg);
                g.fillRoundedRectangle(r, cornerR);

                // Border: rgba(200,204,216,0.07)
                g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.07f));
                g.drawRoundedRectangle(r, cornerR, 1.0f);
            }
            else
            {
                // Active: teal gradient
                juce::ColourGradient bg(tokenTeal().withAlpha(0.28f),
                                         r.getX(), r.getY(),
                                         tokenTeal().withAlpha(0.14f),
                                         r.getX(), r.getBottom(), false);
                g.setGradientFill(bg);
                g.fillRoundedRectangle(r, cornerR);

                // Inset border
                g.setColour(tokenTeal().withAlpha(0.35f));
                g.drawRoundedRectangle(r.reduced(1.0f), cornerR, 2.0f);

                // Pressure fill from bottom, height 60%
                float pressH = r.getHeight() * 0.60f;
                juce::ColourGradient pressGrad(
                    tokenTeal().withAlpha(0.35f),
                    r.getX(), r.getBottom(),
                    juce::Colours::transparentBlack,
                    r.getX(), r.getBottom() - pressH, false);
                g.setGradientFill(pressGrad);
                g.fillRoundedRectangle(r, cornerR);
            }

            // Note label at bottom of white key
            {
                static const char* kNoteNames[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
                int pc  = note % 12;
                int oct = note / 12 - 1;
                juce::String label = juce::String(kNoteNames[pc]) + juce::String(oct);
                static const juce::Font keyLabelFont(juce::FontOptions(8.0f));
                g.setFont(keyLabelFont);
                g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(active ? 0.45f : 0.18f));
                g.drawText(label,
                           r.withTop(r.getBottom() - 16.0f).toNearestInt(),
                           juce::Justification::centred, false);
            }
        }

        // Pass 2: black keys (on top)
        for (int i = 0; i < numBlackKeys_; ++i)
        {
            int note   = blackKeys_[i].midiNote;
            bool active = (keyIsDown_ && activeNote_ == note);
            auto& r    = blackKeys_[i].rect;

            if (!active)
            {
                // Base: rgba(12,14,18,0.92) → rgba(8,10,14,0.96)
                juce::ColourGradient bg(juce::Colour(12, 14, 18).withAlpha(0.92f),
                                         r.getX(), r.getY(),
                                         juce::Colour(8, 10, 14).withAlpha(0.96f),
                                         r.getX(), r.getBottom(), false);
                g.setGradientFill(bg);
                g.fillRoundedRectangle(r, cornerR);

                // Border: rgba(200,204,216,0.04)
                g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.04f));
                g.drawRoundedRectangle(r, cornerR, 1.0f);
            }
            else
            {
                // Active: purple gradient
                juce::ColourGradient bg(juce::Colour(kPurpR, kPurpG, kPurpB).withAlpha(0.35f),
                                         r.getX(), r.getY(),
                                         juce::Colour(kPurpR, kPurpG, kPurpB).withAlpha(0.18f),
                                         r.getX(), r.getBottom(), false);
                g.setGradientFill(bg);
                g.fillRoundedRectangle(r, cornerR);

                // Inset border
                g.setColour(juce::Colour(kPurpR, kPurpG, kPurpB).withAlpha(0.4f));
                g.drawRoundedRectangle(r.reduced(1.0f), cornerR, 2.0f);

                // Pressure fill from bottom, height 60%
                float pressH = r.getHeight() * 0.60f;
                juce::ColourGradient pressGrad(
                    juce::Colour(kPurpR, kPurpG, kPurpB).withAlpha(0.25f),
                    r.getX(), r.getBottom(),
                    juce::Colours::transparentBlack,
                    r.getX(), r.getBottom() - pressH, false);
                g.setGradientFill(pressGrad);
                g.fillRoundedRectangle(r, cornerR);
            }
        }
    }

    //==========================================================================
    // Paint — PAD and DRUM modes

    void paintPads(juce::Graphics& g, bool isDrum)
    {
        if (gridCells_[0].getWidth() <= 0.0f)
            computeGridCells();

        for (int i = 0; i < 16; ++i)
        {
            auto& r   = gridCells_[i];
            bool active = (activePadIdx_ == i);

            if (isDrum)
                paintDrumCell(g, r, i, active);
            else
                paintPadCell(g, r, i, active);
        }
    }

    void paintPadCell(juce::Graphics& g, const juce::Rectangle<float>& r, int idx, bool active)
    {
        const float cr = kGridRadius;

        // Determine note name for label
        int note = midiNoteForPad(idx);
        static const char* kNoteNames[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        int pc  = note % 12;
        int oct = note / 12 - 1;
        juce::String noteLabel = juce::String(kNoteNames[pc]) + juce::String(oct);

        if (!active)
        {
            // Background: linear-gradient(145deg, rgba(200,204,216,0.05), rgba(200,204,216,0.02))
            // Approximate 145deg as top-left → bottom-right
            juce::ColourGradient bg(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.05f),
                                     r.getX(), r.getY(),
                                     juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.02f),
                                     r.getRight(), r.getBottom(), false);
            g.setGradientFill(bg);
            g.fillRoundedRectangle(r, cr);

            // Border: rgba(200,204,216,0.06)
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.06f));
            g.drawRoundedRectangle(r, cr, 1.0f);
        }
        else
        {
            // Active background: teal gradient
            juce::ColourGradient bg(tokenTeal().withAlpha(0.22f),
                                     r.getX(), r.getY(),
                                     tokenTeal().withAlpha(0.10f),
                                     r.getX(), r.getBottom(), false);
            g.setGradientFill(bg);
            g.fillRoundedRectangle(r, cr);

            // Active border
            g.setColour(tokenTeal().withAlpha(0.40f));
            g.drawRoundedRectangle(r, cr, 1.0f);

            // Radial glow (#14)
            juce::ColourGradient glow(tokenTeal().withAlpha(0.25f),
                                       r.getCentreX(), r.getCentreY(),
                                       juce::Colours::transparentBlack,
                                       r.getCentreX() + r.getWidth() * 0.5f * 0.7f, r.getCentreY(), true);
            g.setGradientFill(glow);
            g.fillRoundedRectangle(r, cr);
        }

        // Note label: 12px weight 600, centre of cell
        g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
        g.setColour(active ? tokenTealBright().withAlpha(0.90f)
                           : juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.45f));
        g.drawText(noteLabel, r.toNearestInt(), juce::Justification::centred, false);

        // Velocity hint: 8px monospace, bottom-centre, rgba(200,204,216,0.18)
        if (!active)
        {
            g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 8.0f, 0)));
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.18f));
            juce::String velHint = juce::String(note);
            g.drawText(velHint,
                       r.withTop(r.getBottom() - 14.0f).toNearestInt(),
                       juce::Justification::centred, false);
        }
    }

    void paintDrumCell(juce::Graphics& g, const juce::Rectangle<float>& r, int idx, bool active)
    {
        const float cr = kGridRadius;

        if (!active)
        {
            // Background: linear-gradient(135deg, rgba(200,204,216,0.04), rgba(200,204,216,0.02))
            juce::ColourGradient bg(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.04f),
                                     r.getX(), r.getY(),
                                     juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.02f),
                                     r.getRight(), r.getBottom(), false);
            g.setGradientFill(bg);
            g.fillRoundedRectangle(r, cr);

            // Border: rgba(200,204,216,0.05)
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.05f));
            g.drawRoundedRectangle(r, cr, 1.0f);
        }
        else
        {
            // Active: purple-teal bicolour gradient
            juce::ColourGradient bg(juce::Colour(kPurpR, kPurpG, kPurpB).withAlpha(0.18f),
                                     r.getX(), r.getY(),
                                     tokenTeal().withAlpha(0.12f),
                                     r.getRight(), r.getBottom(), false);
            g.setGradientFill(bg);
            g.fillRoundedRectangle(r, cr);

            // Active border
            g.setColour(juce::Colour(kPurpR, kPurpG, kPurpB).withAlpha(0.40f));
            g.drawRoundedRectangle(r, cr, 1.0f);
        }

        // Drum label: 8px uppercase, top-left area
        g.setFont(juce::Font(juce::FontOptions(8.0f)));
        g.setColour(active ? juce::Colour(kPurpR, kPurpG, kPurpB).withAlpha(0.90f)
                           : juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.30f));

        juce::String label = juce::String(kDrumNames[idx]).toUpperCase();
        // Draw centred vertically in the top half
        juce::Rectangle<float> labelArea(r.getX() + 4.0f, r.getY() + r.getHeight() * 0.5f - 6.0f,
                                          r.getWidth() - 8.0f, 12.0f);
        g.drawText(label, labelArea.toNearestInt(), juce::Justification::centred, false);
    }

    //==========================================================================
    // Paint — XY mode

    void paintXY(juce::Graphics& g)
    {
        auto b = getLocalBounds().toFloat();

        // Background: rgba(200,204,216,0.025) fill, border 1px rgba(200,204,216,0.06)
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.025f));
        g.fillRoundedRectangle(b, 10.0f);

        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.06f));
        g.drawRoundedRectangle(b, 10.0f, 1.0f);

        // Crosshair lines: center-horizontal and center-vertical, 1px rgba(200,204,216,0.035)
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.035f));
        g.drawLine(b.getX(), cy, b.getRight(), cy, 1.0f);
        g.drawLine(cx, b.getY(), cx, b.getBottom(), 1.0f);

        // Axis labels: 8px uppercase, rgba(200,204,216,0.18)
        g.setFont(juce::Font(juce::FontOptions(8.0f)));
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.18f));

        // "X" at bottom centre
        g.drawText("X", juce::Rectangle<float>(b.getCentreX() - 10.0f, b.getBottom() - 16.0f, 20.0f, 14.0f).toNearestInt(),
                   juce::Justification::centred, false);

        // "Y" on left side — draw rotated using a transform
        {
            juce::Graphics::ScopedSaveState ss(g);
            g.addTransform(juce::AffineTransform::rotation(
                static_cast<float>(-M_PI * 0.5),
                b.getX() + 12.0f, b.getCentreY()));
            g.drawText("Y", juce::Rectangle<float>(b.getX() + 12.0f - 10.0f, b.getCentreY() - 7.0f, 20.0f, 14.0f).toNearestInt(),
                       juce::Justification::centred, false);
        }

        // Cursor: 14px circle at (xyX_, xyY_) position
        {
            float cursorX = b.getX() + xyX_ * b.getWidth();
            float cursorY = b.getY() + xyY_ * b.getHeight();
            const float cursorR = 7.0f; // radius → diameter 14

            // Glow shadow (#14 — token teal)
            juce::ColourGradient glowGrad(tokenTeal().withAlpha(0.30f),
                                           cursorX, cursorY,
                                           juce::Colours::transparentBlack,
                                           cursorX + cursorR + 7.0f, cursorY, true);
            g.setGradientFill(glowGrad);
            g.fillEllipse(cursorX - cursorR - 7.0f, cursorY - cursorR - 7.0f,
                          (cursorR + 7.0f) * 2.0f, (cursorR + 7.0f) * 2.0f);

            // Fill (#14)
            g.setColour(tokenTeal().withAlpha(0.45f));
            g.fillEllipse(cursorX - cursorR, cursorY - cursorR, cursorR * 2.0f, cursorR * 2.0f);

            // Border: 2px bright teal (#14)
            g.setColour(tokenTealBright().withAlpha(0.80f));
            g.drawEllipse(cursorX - cursorR, cursorY - cursorR, cursorR * 2.0f, cursorR * 2.0f, 2.0f);
        }
    }

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubmarinePlaySurface)
};

} // namespace xoceanus
