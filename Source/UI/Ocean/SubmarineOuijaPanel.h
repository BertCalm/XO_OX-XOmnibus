// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// SubmarineOuijaPanel.h — Atmospheric circle-of-fifths harmonic surface for the
// XOceanus Ocean View.
//
// Renders a ghostly circle-of-fifths ring with a drifting planchette. The panel
// fills the play area of the dashboard when the OUIJA tab is active.
//
// Visual aesthetic: fully translucent, atmospheric, subdued. Nothing is opaque.
// All custom paint — no JUCE sub-widgets except for mouse hit regions tracked
// manually in the same style as TideWaterline and MasterFXStripCompact.
//
// Planchette modes:
//   DRIFT   — wanders freely (default).  Angle advances 0.003 rad/frame + sine wobble.
//   FREEZE  — angle stays fixed.
//   HOME    — springs back toward angle 0 (top of ring).
//   GOODBYE — unlocks from any note, resets to drift.  Fires onNoteReleased.
//
// Callbacks:
//   onNoteLocked  — planchette locks to a note.  Passes (noteIndex 0-11, midiNote).
//   onNoteReleased — planchette released from a note or GOODBYE pressed.
//   onCCOutput    — XOuija position outputs for CC mapping (circleX 0-1, influenceY 0-1).
//
// Timer: 30 Hz animation (planchette drift, trail aging).
//
// File is entirely self-contained (header-only) following the XOceanus convention.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include <functional>
#include <cmath>
#include <array>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

//==============================================================================
/**
    SubmarineOuijaPanel

    Atmospheric circle-of-fifths harmonic surface.  See file header for full docs.
*/
class SubmarineOuijaPanel : public juce::Component, private juce::Timer
{
public:
    //==========================================================================
    // Called when the planchette locks to a note.
    // noteIndex: 0-11 in circle-of-fifths order.
    // midiNote:  MIDI note number (C4=60 baseline + semitone offset).
    std::function<void(int noteIndex, int midiNote)> onNoteLocked;

    // Called when the planchette is released (GOODBYE button or unlock click).
    std::function<void()> onNoteReleased;

    // CC output: XOuija position as (circleX 0-1, influenceY 0-1).
    // Fires each animation frame while the planchette moves.
    // cc 0 = circleX (horizontal normalised position), cc 1 = influenceY (radius normalised).
    std::function<void(int cc, float value)> onCCOutput;

    //==========================================================================
    SubmarineOuijaPanel()
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, true);
        startTimerHz(30);
    }

    ~SubmarineOuijaPanel() override
    {
        stopTimer();
    }

private:
    //==========================================================================
    // Note names in circle-of-fifths order (12 notes, starting at C).
    static constexpr const char* kNoteNames[12] = {
        "C", "G", "D", "A", "E", "B", "Gb", "Db", "Ab", "Eb", "Bb", "F"
    };

    // Semitone offsets from C4 (60) for each fifths position.
    static constexpr int kSemitoneOffsets[12] = {
        0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5
    };

    // ARGB colors per note (chromatic spectrum, one per semitone in fifths order).
    static constexpr juce::uint32 kNoteColors[12] = {
        0xFFEF5350, 0xFFFF7043, 0xFFFFA726, 0xFFFFCA28,
        0xFFFFEE58, 0xFFD4E157, 0xFF66BB6A, 0xFF26A69A,
        0xFF42A5F5, 0xFF5C6BC0, 0xFF7E57C2, 0xFFAB47BC
    };

    //==========================================================================
    // Planchette state machine.
    enum class GestureMode { Drift, Freeze, Home, Goodbye };

    GestureMode gestureMode_   = GestureMode::Drift;
    float       planchetteAngle_ = 0.0f;   // current angle in radians
    float       time_            = 0.0f;   // accumulated time in frames (for wobble)

    int  lockedNoteIndex_ = -1;   // -1 = not locked

    //==========================================================================
    // Trail: ring buffer of 60 position samples.
    struct TrailPoint
    {
        float x = 0.0f;
        float y = 0.0f;
        float age = 0.0f;   // 0 = just added, increments 0.02 per frame
    };

    static constexpr int kTrailCapacity = 60;
    std::array<TrailPoint, kTrailCapacity> trail_;
    int   trailHead_  = 0;
    int   trailCount_ = 0;

    //==========================================================================
    // Gesture button hit regions (rebuilt in paint).
    struct ButtonRegion
    {
        juce::Rectangle<float> bounds;
        GestureMode            mode;
    };
    std::array<ButtonRegion, 4> buttonRegions_;
    bool buttonRegionsBuilt_ = false;

    // Hover state for gesture buttons.
    GestureMode hoveredButton_ = GestureMode::Goodbye; // sentinel: no hover when == Goodbye AND gestureMode != Goodbye
    bool        anyButtonHovered_ = false;

    //==========================================================================
    // Cached geometry (computed from component bounds each paint call).
    struct Geometry
    {
        float cx = 0.0f;        // centre x
        float cy = 0.0f;        // centre y
        float ringRadius = 0.0f;
        float noteCircleRadius = 0.0f;
        float planchetteOrbitRadius = 0.0f;
        // note positions
        std::array<float, 12> nx;
        std::array<float, 12> ny;
    };

    mutable Geometry    geo_;
    mutable bool        geoValid_          = false;
    mutable juce::Font  cachedNoteFont_    { juce::FontOptions{}.withHeight(10.0f) };

    //==========================================================================
    // juce::Timer
    void timerCallback() override
    {
        // Panel is hidden (parent tab not OUIJA): skip the whole frame —
        // planchette physics, trail decay, CC emission, and repaint all
        // depend on the panel being on-screen. Parent tab visibility changes
        // do not fire visibilityChanged() here, so the cheap per-tick guard
        // is the right tool. (isShowing() walks ancestors.)
        if (! isShowing())
            return;

        time_ += 1.0f;
        advancePlanchette();
        ageTrail();
        emitCCOutput();
        repaint();
    }

    //--------------------------------------------------------------------------
    void advancePlanchette()
    {
        if (gestureMode_ == GestureMode::Freeze)
            return;

        if (gestureMode_ == GestureMode::Home)
        {
            // Spring toward angle 0.
            planchetteAngle_ = planchetteAngle_ + (0.0f - planchetteAngle_) * 0.05f;
            pushTrailPoint();
            return;
        }

        if (lockedNoteIndex_ >= 0 && gestureMode_ != GestureMode::Home)
        {
            // Spring toward locked note angle.
            const float targetAngle = noteAngle(lockedNoteIndex_);
            float delta = targetAngle - planchetteAngle_;
            // Normalise delta to [-PI, PI].
            while (delta >  static_cast<float>(M_PI)) delta -= static_cast<float>(2.0 * M_PI);
            while (delta < -static_cast<float>(M_PI)) delta += static_cast<float>(2.0 * M_PI);
            planchetteAngle_ += delta * 0.08f;
            pushTrailPoint();
            return;
        }

        // Drift mode.
        const float wobble = std::sin(time_ * 0.7f) * 0.002f;
        planchetteAngle_ += 0.003f + wobble;
        pushTrailPoint();
    }

    //--------------------------------------------------------------------------
    void ageTrail()
    {
        for (int i = 0; i < trailCount_; ++i)
        {
            int idx = (trailHead_ - 1 - i + kTrailCapacity) % kTrailCapacity;
            trail_[idx].age += 0.02f;
        }
        // Remove points that are fully faded (age >= 1.0 means alpha <= 0).
        while (trailCount_ > 0)
        {
            int oldest = (trailHead_ - trailCount_ + kTrailCapacity) % kTrailCapacity;
            if (trail_[oldest].age >= 1.0f)
                --trailCount_;
            else
                break;
        }
    }

    //--------------------------------------------------------------------------
    void pushTrailPoint()
    {
        ensureGeoValid();
        const float px = geo_.cx + std::cos(planchetteAngle_) * geo_.planchetteOrbitRadius;
        const float py = geo_.cy + std::sin(planchetteAngle_) * geo_.planchetteOrbitRadius;

        trail_[trailHead_] = { px, py, 0.0f };
        trailHead_ = (trailHead_ + 1) % kTrailCapacity;
        if (trailCount_ < kTrailCapacity)
            ++trailCount_;
    }

    //--------------------------------------------------------------------------
    void emitCCOutput()
    {
        if (!onCCOutput)
            return;
        ensureGeoValid();
        if (geo_.ringRadius <= 0.0f)
            return;

        const float px = geo_.cx + std::cos(planchetteAngle_) * geo_.planchetteOrbitRadius;
        const float py = geo_.cy + std::sin(planchetteAngle_) * geo_.planchetteOrbitRadius;

        const float circleX   = juce::jlimit(0.0f, 1.0f, (px - (geo_.cx - geo_.ringRadius)) / (2.0f * geo_.ringRadius));
        const float influenceY = juce::jlimit(0.0f, 1.0f, (py - (geo_.cy - geo_.ringRadius)) / (2.0f * geo_.ringRadius));

        onCCOutput(0, circleX);
        onCCOutput(1, influenceY);
    }

    //==========================================================================
    // Geometry helpers.

    // Angle for the i-th note in circle-of-fifths order.
    // Notes are spaced evenly at 2*PI/12 = 30 degrees.
    // Index 0 (C) starts at -PI/2 so that C is at the top.
    static float noteAngle(int i) noexcept
    {
        return static_cast<float>(-M_PI / 2.0) + (static_cast<float>(i) / 12.0f) * static_cast<float>(2.0 * M_PI);
    }

    void ensureGeoValid() const
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        if (w < 2.0f || h < 2.0f)
            return;
        if (geoValid_)
            return;

        geo_.cx = w * 0.5f;
        // Centre slightly above vertical midpoint to leave room for buttons at bottom.
        geo_.cy = h * 0.46f;

        const float minDim = std::min(w, h);
        geo_.ringRadius            = minDim * 0.38f;
        geo_.noteCircleRadius      = std::max(14.0f, geo_.ringRadius * 0.12f);
        geo_.planchetteOrbitRadius = geo_.ringRadius * 0.65f;

        for (int i = 0; i < 12; ++i)
        {
            const float a = noteAngle(i);
            geo_.nx[i] = geo_.cx + std::cos(a) * geo_.ringRadius;
            geo_.ny[i] = geo_.cy + std::sin(a) * geo_.ringRadius;
        }

        cachedNoteFont_ = juce::Font(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(geo_.noteCircleRadius * 0.7f));

        geoValid_ = true;
    }

    //==========================================================================
    // Paint entry.
    void paint(juce::Graphics& g) override
    {
        geoValid_ = false;   // recompute from current bounds each frame
        ensureGeoValid();

        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());

        paintBackground(g, w, h);
        paintConnectionWeb(g);
        paintInnerCircle(g);
        paintTrail(g);
        paintPlanchetteLine(g);
        paintNoteRing(g);
        paintPlanchette(g);
        buildAndPaintButtons(g, w, h);
    }

    void resized() override
    {
        geoValid_ = false;
        buttonRegionsBuilt_ = false;
    }

    //--------------------------------------------------------------------------
    void paintBackground(juce::Graphics& g, float w, float h) const
    {
        g.setColour(juce::Colour(10, 12, 18).withAlpha(0.40f));
        g.fillRect(0.0f, 0.0f, w, h);
    }

    //--------------------------------------------------------------------------
    // Thin lines connecting each adjacent pair of notes on the ring.
    void paintConnectionWeb(juce::Graphics& g) const
    {
        const float ncr = geo_.noteCircleRadius;
        const juce::Colour webColour = juce::Colour(127, 219, 202).withAlpha(0.06f);

        for (int i = 0; i < 12; ++i)
        {
            const int j = (i + 1) % 12;

            // Direction from centre toward each note.
            const float ax = geo_.nx[i] - geo_.cx;
            const float ay = geo_.ny[i] - geo_.cy;
            const float bx = geo_.nx[j] - geo_.cx;
            const float by = geo_.ny[j] - geo_.cy;

            const float lenA = std::sqrt(ax * ax + ay * ay);
            const float lenB = std::sqrt(bx * bx + by * by);
            if (lenA < 0.001f || lenB < 0.001f)
                continue;

            // Start/end points pulled inward by (ncr + 4) from the note centres.
            const float inset = ncr + 4.0f;
            const float x1 = geo_.nx[i] - (ax / lenA) * inset;
            const float y1 = geo_.ny[i] - (ay / lenA) * inset;
            const float x2 = geo_.nx[j] - (bx / lenB) * inset;
            const float y2 = geo_.ny[j] - (by / lenB) * inset;

            g.setColour(webColour);
            g.drawLine(x1, y1, x2, y2, 0.5f);
        }
    }

    //--------------------------------------------------------------------------
    void paintInnerCircle(juce::Graphics& g) const
    {
        const float r = geo_.ringRadius * 0.20f;
        g.setColour(juce::Colour(127, 219, 202).withAlpha(0.08f));
        g.drawEllipse(geo_.cx - r, geo_.cy - r, r * 2.0f, r * 2.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    void paintTrail(juce::Graphics& g) const
    {
        for (int i = 0; i < trailCount_; ++i)
        {
            const int idx = (trailHead_ - 1 - i + kTrailCapacity) % kTrailCapacity;
            const TrailPoint& tp = trail_[idx];
            const float alpha = std::max(0.0f, 0.3f - tp.age * 0.3f);
            if (alpha <= 0.001f)
                continue;

            g.setColour(juce::Colour(127, 219, 202).withAlpha(alpha));
            g.fillEllipse(tp.x - 1.0f, tp.y - 1.0f, 2.0f, 2.0f);
        }
    }

    //--------------------------------------------------------------------------
    // Dashed line from centre to planchette position.
    void paintPlanchetteLine(juce::Graphics& g) const
    {
        const float px = geo_.cx + std::cos(planchetteAngle_) * geo_.planchetteOrbitRadius;
        const float py = geo_.cy + std::sin(planchetteAngle_) * geo_.planchetteOrbitRadius;

        juce::Path linePath;
        linePath.startNewSubPath(geo_.cx, geo_.cy);
        linePath.lineTo(px, py);

        const float dashLengths[2] = { 3.0f, 3.0f };
        juce::Path dashedPath;
        juce::PathStrokeType strokeType(1.0f);
        strokeType.createDashedStroke(dashedPath, linePath, dashLengths, 2);

        g.setColour(juce::Colour(127, 219, 202).withAlpha(0.08f));
        g.fillPath(dashedPath);
    }

    //--------------------------------------------------------------------------
    void paintNoteRing(juce::Graphics& g) const
    {
        const float ncr = geo_.noteCircleRadius;

        for (int i = 0; i < 12; ++i)
        {
            const bool isLocked = (lockedNoteIndex_ == i);
            const juce::Colour noteBaseColour(kNoteColors[i]);

            // Fill
            const float fillAlpha  = isLocked ? 0.80f : 0.20f;
            g.setColour(noteBaseColour.withAlpha(fillAlpha));
            g.fillEllipse(geo_.nx[i] - ncr, geo_.ny[i] - ncr, ncr * 2.0f, ncr * 2.0f);

            // Stroke
            const float strokeAlpha = isLocked ? 1.00f : 0.40f;
            const float strokeWidth = isLocked ? 2.5f : 1.0f;
            g.setColour(noteBaseColour.withAlpha(strokeAlpha));
            g.drawEllipse(geo_.nx[i] - ncr, geo_.ny[i] - ncr, ncr * 2.0f, ncr * 2.0f, strokeWidth);

            // Label — same font for both locked and unlocked (Bold at fontSize).
            // Locked state is distinguished by color, not font weight change.
            if (isLocked)
                g.setColour(juce::Colours::white);
            else
                g.setColour(juce::Colour(200, 204, 216).withAlpha(0.60f));

            g.setFont(cachedNoteFont_);
            g.drawText(juce::String(kNoteNames[i]),
                       juce::Rectangle<float>(geo_.nx[i] - ncr, geo_.ny[i] - ncr,
                                              ncr * 2.0f, ncr * 2.0f).toNearestInt(),
                       juce::Justification::centred,
                       false);
        }
    }

    //--------------------------------------------------------------------------
    void paintPlanchette(juce::Graphics& g) const
    {
        const float px = geo_.cx + std::cos(planchetteAngle_) * geo_.planchetteOrbitRadius;
        const float py = geo_.cy + std::sin(planchetteAngle_) * geo_.planchetteOrbitRadius;
        constexpr float kBodyRadius = 8.0f;

        // Body fill
        g.setColour(juce::Colour(127, 219, 202).withAlpha(0.15f));
        g.fillEllipse(px - kBodyRadius, py - kBodyRadius, kBodyRadius * 2.0f, kBodyRadius * 2.0f);

        // Body stroke
        g.setColour(juce::Colour(127, 219, 202).withAlpha(0.50f));
        g.drawEllipse(px - kBodyRadius, py - kBodyRadius, kBodyRadius * 2.0f, kBodyRadius * 2.0f, 2.0f);
    }

    //--------------------------------------------------------------------------
    // Build gesture button hit regions and paint them.
    // Buttons are positioned as an absolutely-placed row at bottom centre, 8px from bottom.
    void buildAndPaintButtons(juce::Graphics& g, float w, float h)
    {
        struct ButtonDef { const char* label; GestureMode mode; };
        static constexpr ButtonDef kButtons[4] = {
            { "FREEZE",  GestureMode::Freeze  },
            { "HOME",    GestureMode::Home    },
            { "DRIFT",   GestureMode::Drift   },
            { "GOODBYE", GestureMode::Goodbye },
        };

        constexpr float kPadH    = 10.0f;  // horizontal padding
        constexpr float kPadV    = 4.0f;   // vertical padding
        constexpr float kFontSz  = 8.0f;
        constexpr float kGap     = 4.0f;
        constexpr float kRadius  = 4.0f;
        constexpr float kBtnH    = kFontSz + kPadV * 2.0f;

        static const juce::Font btnFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(kFontSz));

        // Measure button widths.
        std::array<float, 4> btnWidths{};
        float totalW = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            btnWidths[i] = btnFont.getStringWidthFloat(juce::String(kButtons[i].label)) + kPadH * 2.0f;
            totalW += btnWidths[i];
        }
        totalW += kGap * 3.0f;

        float startX = (w - totalW) * 0.5f;
        const float btnY = h - kBtnH - 8.0f;

        for (int i = 0; i < 4; ++i)
        {
            const GestureMode mode  = kButtons[i].mode;
            const float bw          = btnWidths[i];
            const juce::Rectangle<float> bounds(startX, btnY, bw, kBtnH);

            buttonRegions_[i] = { bounds, mode };

            const bool isActive  = (gestureMode_ == mode && mode != GestureMode::Goodbye);
            const bool isHovered = (anyButtonHovered_ && hoveredButton_ == mode);

            juce::Colour textCol, borderCol, bgCol;
            if (isActive)
            {
                textCol   = juce::Colour(127, 219, 202).withAlpha(0.80f);
                borderCol = juce::Colour(127, 219, 202).withAlpha(0.25f);
                bgCol     = juce::Colour(127, 219, 202).withAlpha(0.06f);
            }
            else if (isHovered)
            {
                textCol   = juce::Colour(200, 204, 216).withAlpha(0.70f);
                borderCol = juce::Colour(200, 204, 216).withAlpha(0.15f);
                bgCol     = juce::Colour(10, 12, 18).withAlpha(0.60f);
            }
            else
            {
                textCol   = juce::Colour(200, 204, 216).withAlpha(0.40f);
                borderCol = juce::Colour(200, 204, 216).withAlpha(0.08f);
                bgCol     = juce::Colour(10, 12, 18).withAlpha(0.60f);
            }

            g.setColour(bgCol);
            g.fillRoundedRectangle(bounds, kRadius);

            g.setColour(borderCol);
            g.drawRoundedRectangle(bounds, kRadius, 1.0f);

            g.setFont(btnFont);
            g.setColour(textCol);
            g.drawText(juce::String(kButtons[i].label),
                       bounds.toNearestInt(),
                       juce::Justification::centred,
                       false);

            startX += bw + kGap;
        }

        buttonRegionsBuilt_ = true;
    }

    //==========================================================================
    // Mouse interaction.

    void mouseDown(const juce::MouseEvent& e) override
    {
        const juce::Point<float> pos = e.position;

        // Test gesture buttons first.
        if (buttonRegionsBuilt_)
        {
            for (const auto& btn : buttonRegions_)
            {
                if (btn.bounds.contains(pos))
                {
                    handleGestureButtonClick(btn.mode);
                    return;
                }
            }
        }

        // Test note circles.
        ensureGeoValid();
        const float ncr        = geo_.noteCircleRadius;
        const float hitRadius  = ncr * 1.5f;

        int  closestNote = -1;
        float closestDist = hitRadius + 1.0f;

        for (int i = 0; i < 12; ++i)
        {
            const float dx = pos.x - geo_.nx[i];
            const float dy = pos.y - geo_.ny[i];
            const float dist = std::sqrt(dx * dx + dy * dy);
            if (dist < hitRadius && dist < closestDist)
            {
                closestDist = dist;
                closestNote = i;
            }
        }

        if (closestNote >= 0)
        {
            if (lockedNoteIndex_ == closestNote)
            {
                // Already locked — toggle off.
                lockedNoteIndex_ = -1;
                gestureMode_     = GestureMode::Drift;
                if (onNoteReleased)
                    onNoteReleased();
            }
            else
            {
                // Lock to this note.
                lockedNoteIndex_ = closestNote;
                // Snap gesture mode back to drift so spring logic runs.
                if (gestureMode_ == GestureMode::Freeze || gestureMode_ == GestureMode::Home)
                    gestureMode_ = GestureMode::Drift;

                const int midiNote = 60 + kSemitoneOffsets[closestNote];
                if (onNoteLocked)
                    onNoteLocked(closestNote, midiNote);
            }
            repaint();
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        updateButtonHover(e.position);
    }

    void mouseExit(const juce::MouseEvent& /*e*/) override
    {
        anyButtonHovered_ = false;
        repaint();
    }

    //--------------------------------------------------------------------------
    void updateButtonHover(juce::Point<float> pos)
    {
        if (!buttonRegionsBuilt_)
            return;

        bool found = false;
        for (const auto& btn : buttonRegions_)
        {
            if (btn.bounds.contains(pos))
            {
                hoveredButton_    = btn.mode;
                anyButtonHovered_ = true;
                found = true;
                break;
            }
        }
        if (!found)
            anyButtonHovered_ = false;

        repaint();
    }

    //--------------------------------------------------------------------------
    void handleGestureButtonClick(GestureMode mode)
    {
        if (mode == GestureMode::Goodbye)
        {
            // Release any locked note, reset to drift.
            lockedNoteIndex_ = -1;
            gestureMode_     = GestureMode::Drift;
            if (onNoteReleased)
                onNoteReleased();
        }
        else
        {
            gestureMode_ = mode;
            if (mode == GestureMode::Drift || mode == GestureMode::Home)
            {
                // Drift and Home release the note lock so the planchette can move.
                if (lockedNoteIndex_ >= 0 && mode == GestureMode::Home)
                {
                    lockedNoteIndex_ = -1;
                    if (onNoteReleased)
                        onNoteReleased();
                }
            }
        }
        repaint();
    }

    //==========================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubmarineOuijaPanel)
};

} // namespace xoceanus
