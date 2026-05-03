// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// SurfaceRightPanel.h — Submarine-style right-side panel for PAD / DRUM / XY playing surfaces.
//
// This panel slides out from the right edge of the ocean viewport when PAD, DRUM, or XY
// mode is selected. It shares horizontal space with the ocean (it is NOT inside the
// dashboard). At 420 px wide (max 40 % of window width), it contains:
//
//   • A 36 px header with the mode name and a close button (two crossed lines, no glyph).
//   • A mode-specific content area:
//       PAD  — 4×4 chromatic note grid. Top row = C2–F2 … bottom row = A3–D4.
//       DRUM — 4×4 drum grid with unique per-row tints and 3-char GM labels.
//       XY   — Large 2D control surface with crosshair, cursor glow, axis readouts,
//              and an auto-motion pill row.
//
// All rendering is fully custom (no JUCE widgets). Inline header-only implementation.
//
// Usage:
//   auto panel = std::make_unique<SurfaceRightPanel>();
//   panel->onNoteOn  = [this](int n, float v) { processor_.addNote(n, v); };
//   panel->onNoteOff = [this](int n)           { processor_.removeNote(n); };
//   panel->setMode(SurfaceRightPanel::Mode::Pad);
//   panel->setOpen(true);
//   addAndMakeVisible(*panel);

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../Tokens.h"
// Note: Starboard.h excluded — cut(1B-#*): file deleted in Track A 1B reduction pass.
#include <functional>
#include <cmath>
#include <array>

namespace xoceanus
{

//==============================================================================
/**
    SurfaceRightPanel

    Right-side sliding panel containing PAD / DRUM / XY playing surfaces.
    See file header for full documentation.
*/
class SurfaceRightPanel : public juce::Component
{
public:
    //==========================================================================
    enum class Mode { Pad, Drum, XY };

    static constexpr int kPanelWidth  = 420;
    static constexpr int kHeaderH     = 36;


    //==========================================================================
    SurfaceRightPanel()
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, true);
        setWantsKeyboardFocus(false);

    }

    ~SurfaceRightPanel() override = default;

    //==========================================================================
    // Public API

    void setMode(Mode m)
    {
        if (mode_ == m)
            return;
        releaseActive();
        mode_ = m;
        repaint();
    }

    Mode getMode() const noexcept { return mode_; }

    void setOpen(bool open)
    {
        if (open_ == open)
            return;
        if (!open)
            releaseActive();
        open_ = open;
        setVisible(open_);
        repaint();
    }

    bool isOpen() const noexcept { return open_; }

    //==========================================================================
    // MIDI / XY output callbacks

    std::function<void(int note, float velocity)> onNoteOn;
    std::function<void(int note)>                 onNoteOff;
    std::function<void(float x, float y)>         onXYChanged;

    // Close button callback
    std::function<void()> onCloseClicked;

    // D4 (1D-P2B): XY pad grid visibility toggle.
    // gridVisible = true  → visible grid: bg α=0.20, border α=0.40.
    // gridVisible = false → ghost grid (current default): bg α=0.025, border α=0.06.
    // Default is true (APVTS default = true).
    void setGridVisible(bool visible)
    {
        if (xyGridVisible_ == visible) return;
        xyGridVisible_ = visible;
        repaint();
    }
    bool isGridVisible() const noexcept { return xyGridVisible_; }

    // Fires when the GRID pill in XY mode row 2 is clicked.
    std::function<void(bool gridOn)> onGridToggled;

    //==========================================================================
    // juce::Component overrides

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // ---- Container background ----
        // linear-gradient(to bottom, #16181e, #1a1c22 40%, #1a1c22)
        juce::ColourGradient bgGrad(juce::Colour(0xFF16181e), bounds.getX(), bounds.getY(),
                                    juce::Colour(0xFF1a1c22), bounds.getX(), bounds.getY() + bounds.getHeight() * 0.4f,
                                    false);
        bgGrad.addColour(1.0, juce::Colour(0xFF1a1c22));
        g.setGradientFill(bgGrad);
        g.fillRect(bounds);

        // Left border — 1px rgba(60,180,170,0.08)
        g.setColour(XO::Tokens::Color::accent().withAlpha(0.08f));
        g.drawVerticalLine(0, bounds.getY(), bounds.getBottom());

        // ---- Header ----
        paintHeader(g);

        // ---- Content area ----
        auto contentBounds = bounds.withTrimmedTop(static_cast<float>(kHeaderH));
        switch (mode_)
        {
            case Mode::Pad:   paintPadMode(g, contentBounds);  break;
            case Mode::Drum:  paintDrumMode(g, contentBounds); break;
            case Mode::XY:    paintXYMode(g, contentBounds);   break;
        }
    }

    void resized() override
    {
        computePadBounds();
        computeCloseBtnBounds();
        computeXYPadBounds();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        auto pos = e.position;

        // Close button hit test
        if (closeBtnBounds_.contains(pos))
        {
            closeBtnPressed_ = true;
            repaint();
            return;
        }

        // Mode-specific hit tests
        switch (mode_)
        {
            case Mode::Pad:
            case Mode::Drum:  handlePadDown(pos); break;
            case Mode::XY:    handleXYDown(pos);  break;
        }
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (closeBtnPressed_)
            return;

        switch (mode_)
        {
            case Mode::Pad:
            case Mode::Drum:  break; // pads are momentary, no slide
            case Mode::XY:    handleXYDrag(e.position); break;
        }
        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (closeBtnPressed_)
        {
            closeBtnPressed_ = false;
            if (closeBtnBounds_.contains(e.position))
            {
                if (onCloseClicked)
                    onCloseClicked();
            }
            repaint();
            return;
        }

        switch (mode_)
        {
            case Mode::Pad:
            case Mode::Drum:  handlePadUp(); break;
            case Mode::XY:    handleXYUp();  break;
        }
        repaint();
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        bool overClose = closeBtnBounds_.contains(e.position);
        if (overClose != closeBtnHover_)
        {
            closeBtnHover_ = overClose;
            repaint();
        }

        int padHover = hitTestGrid(e.position.x, e.position.y);
        if (padHover != hoverPad_)
        {
            hoverPad_ = padHover;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent& /*e*/) override
    {
        if (closeBtnHover_)
        {
            closeBtnHover_ = false;
            repaint();
        }
        if (hoverPad_ >= 0)
        {
            hoverPad_ = -1;
            repaint();
        }
    }

private:
    //==========================================================================
    // Color constants (spec: teal = rgba(60,180,170), salt = rgba(200,204,216))

    static constexpr uint8_t kTealR = 60,  kTealG = 180, kTealB = 170;
    static constexpr uint8_t kSaltR = 200, kSaltG = 204, kSaltB = 216;
    static constexpr uint8_t kPurpR = 140, kPurpG = 100, kPurpB = 220;

    // Grid constants
    static constexpr int   kGridSize   = 4;
    static constexpr float kGridGap    = 6.0f;
    static constexpr float kGridPadX   = 10.0f;
    static constexpr float kGridPadY   = 10.0f;
    static constexpr float kGridRadius = 10.0f;

    // XY constants
    static constexpr float kXYPadMargin  = 12.0f;
    static constexpr float kXYReadoutH   = 40.0f;
    static constexpr float kXYAutoH      = 30.0f;

    //==========================================================================
    // PAD mode — note name table
    // MPC layout: Row 0 (top) = C2 D2 E2 F2, Row 1 = G2 A2 B2 C3,
    //             Row 2 = D3 E3 F3 G3,        Row 3 = A3 B3 C4 D4
    static constexpr const char* kPadNoteNames[16] = {
        "C2", "D2", "E2", "F2",
        "G2", "A2", "B2", "C3",
        "D3", "E3", "F3", "G3",
        "A3", "B3", "C4", "D4"
    };

    // MIDI note for each PAD cell (row 0 top = C2 = MIDI 36)
    static int midiForPad(int idx) noexcept
    {
        // Row 0 top → MIDI 36 (C2), laid out chromatically left-to-right, top-to-bottom.
        // But MPC convention: top row highest. So row 0 = highest octave band.
        // Spec: Row 0 = C2 D2 E2 F2, Row 1 = G2 A2 B2 C3,
        //       Row 2 = D3 E3 F3 G3, Row 3 = A3 B3 C4 D4
        // C2 = MIDI 36, D2=38, E2=40, F2=41, G2=43, A2=45, B2=47, C3=48,
        // D3=50, E3=52, F3=53, G3=55, A3=57, B3=59, C4=60, D4=62
        static const int kNotes[16] = {
            36, 38, 40, 41,  // C2 D2 E2 F2
            43, 45, 47, 48,  // G2 A2 B2 C3
            50, 52, 53, 55,  // D3 E3 F3 G3
            57, 59, 60, 62   // A3 B3 C4 D4
        };
        return (idx >= 0 && idx < 16) ? kNotes[idx] : 36;
    }

    //==========================================================================
    // DRUM mode — label and MIDI note tables (spec-exact)
    static constexpr const char* kDrumLabels[16] = {
        "KCK", "SNR", "CHH", "OHH",
        "CLP", "RDE", "CRS", "SHK",
        "PC1", "PC2", "PC3", "PC4",
        "FX1", "FX2", "FX3", "FX4"
    };

    static int midiForDrum(int idx) noexcept
    {
        static const int kNotes[16] = {
            36, 38, 42, 46,  // KCK, SNR, CHH, OHH
            39, 51, 49, 69,  // CLP, RDE, CRS, SHK
            47, 48, 50, 52,  // PC1, PC2, PC3, PC4
            53, 54, 55, 56   // FX1, FX2, FX3, FX4
        };
        return (idx >= 0 && idx < 16) ? kNotes[idx] : 36;
    }

    // Per-row drum tint: RGBA uint32 (ARGB format for juce::Colour(uint32))
    // Row 0: deep teal, Row 1: medium teal, Row 2: lighter teal, Row 3: dim cool blue
    static juce::Colour drumTintForRow(int row) noexcept
    {
        switch (row)
        {
            case 0:  return juce::Colour(kTealR,       kTealG,       kTealB).withAlpha(0.12f);
            case 1:  return juce::Colour(kTealR + 10,  kTealG - 20,  kTealB + 10).withAlpha(0.10f);
            case 2:  return juce::Colour(kTealR - 14,  kTealG,       kTealB + 4).withAlpha(0.08f);
            default: return juce::Colour(50,            160,           170).withAlpha(0.06f);
        }
    }

    static juce::Colour drumLabelColourForRow(int row) noexcept
    {
        switch (row)
        {
            case 0:  return juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.70f);
            case 1:  return juce::Colour(kTealR + 10, kTealG - 20, kTealB + 10).withAlpha(0.65f);
            case 2:  return juce::Colour(kTealR - 14, kTealG, kTealB + 4).withAlpha(0.60f);
            default: return juce::Colour(50, 160, 170).withAlpha(0.50f);
        }
    }

    //==========================================================================
    // XY auto-motion modes
    static constexpr const char* kAutoMotionLabels[] = { "CIRCLE", "FIG-8", "SWEEP", "RANDOM" };
    static constexpr int kAutoMotionCount = 4;

    //==========================================================================
    // State
    Mode  mode_            = Mode::Pad;
    bool  open_            = false;

    // Active pad / close button state
    int   pressedPad_      = -1; // -1 = none
    int   hoverPad_        = -1;
    bool  closeBtnPressed_ = false;
    bool  closeBtnHover_   = false;

    // XY state
    float xyX_             = 0.5f;
    float xyY_             = 0.5f;
    bool  xyDragging_      = false;

    // XY auto-motion selected pill (-1 = none)
    int   autoMotionSel_   = -1;

    // D4 (1D-P2B): XY grid visibility — true = visible (ON), false = ghost.
    // Default true; persisted via APVTS xy_pad_grid_visible (wired in OceanView).
    bool  xyGridVisible_   = true;
    juce::Rectangle<float> xyGridPillBounds_;  // D4: GRID pill — computed lazily in paint


    // XY assign label state (toggle for hover, not interactive here)
    // assignXHover_ / assignYHover_ reserved for future XY-assign hover highlight

    //==========================================================================
    // Precomputed geometry — rebuilt in resized()

    std::array<juce::Rectangle<float>, 16> padBounds_{};
    juce::Rectangle<float>                 closeBtnBounds_{};
    juce::Rectangle<float>                 xyPadBounds_{};
    juce::Rectangle<float>                 xyReadoutBounds_{};
    juce::Rectangle<float>                 xyAutoBounds_{};

    // Auto-motion pill bounds (computed lazily in paint)
    std::array<juce::Rectangle<float>, kAutoMotionCount> autoMotionPillBounds_{};

    //==========================================================================
    // Geometry computation

    void computePadBounds()
    {
        auto b = getLocalBounds().toFloat().withTrimmedTop(static_cast<float>(kHeaderH));

        float availW = b.getWidth()  - kGridPadX * 2.0f;
        float availH = b.getHeight() - kGridPadY * 2.0f;

        float cellSize = std::min(
            (availW - kGridGap * (kGridSize - 1)) / static_cast<float>(kGridSize),
            (availH - kGridGap * (kGridSize - 1)) / static_cast<float>(kGridSize));

        float gridW  = cellSize * kGridSize + kGridGap * (kGridSize - 1);
        float gridH  = cellSize * kGridSize + kGridGap * (kGridSize - 1);
        float startX = b.getX() + (b.getWidth()  - gridW) * 0.5f;
        float startY = b.getY() + (b.getHeight() - gridH) * 0.5f;

        for (int row = 0; row < kGridSize; ++row)
        {
            for (int col = 0; col < kGridSize; ++col)
            {
                int idx  = row * kGridSize + col;
                float cx = startX + static_cast<float>(col) * (cellSize + kGridGap);
                float cy = startY + static_cast<float>(row) * (cellSize + kGridGap);
                padBounds_[idx] = juce::Rectangle<float>(cx, cy, cellSize, cellSize);
            }
        }
    }

    void computeCloseBtnBounds()
    {
        // 22x22 close button, right-aligned in header with 7px margin
        auto b = getLocalBounds().toFloat();
        float btnSize = 22.0f;
        float margin  = 7.0f;
        closeBtnBounds_ = juce::Rectangle<float>(
            b.getRight() - btnSize - margin,
            b.getY() + (static_cast<float>(kHeaderH) - btnSize) * 0.5f,
            btnSize, btnSize);
    }

    void computeXYPadBounds()
    {
        auto b = getLocalBounds().toFloat().withTrimmedTop(static_cast<float>(kHeaderH));
        float margin = kXYPadMargin;
        float readH  = kXYReadoutH;
        float autoH  = kXYAutoH;

        xyPadBounds_     = b.reduced(margin).withTrimmedBottom(readH + autoH);
        xyReadoutBounds_ = juce::Rectangle<float>(b.getX() + margin, xyPadBounds_.getBottom() + 4.0f,
                                                   b.getWidth() - margin * 2.0f, readH - 4.0f);
        xyAutoBounds_    = juce::Rectangle<float>(b.getX() + margin,
                                                   xyReadoutBounds_.getBottom() + 2.0f,
                                                   b.getWidth() - margin * 2.0f, autoH - 2.0f);
    }

    //==========================================================================
    // Hit testing

    int hitTestGrid(float px, float py) const
    {
        for (int i = 0; i < 16; ++i)
            if (padBounds_[i].contains(px, py))
                return i;
        return -1;
    }

    int hitTestAutoMotionPill(float px, float py) const
    {
        // P1-5 (1F): WCAG 2.5.8 — expand hit area to ≥24×24 px so touch/stylus
        // users can reliably activate pills without pixel-precision clicking.
        // Visual bounds are ~18px tall; the expanded area adds 3px each side.
        for (int i = 0; i < kAutoMotionCount; ++i)
            if (autoMotionPillBounds_[i].expanded(0.0f, 3.0f).contains(px, py))
                return i;
        return -1;
    }

    /** Velocity from Y position. Top = 1.0, bottom = 0.3 (spec). */
    static float velFromY(float py, const juce::Rectangle<float>& rect) noexcept
    {
        if (rect.getHeight() <= 0.0f)
            return 0.65f;
        float norm = juce::jlimit(0.0f, 1.0f, (py - rect.getY()) / rect.getHeight());
        return 1.0f - norm * 0.7f; // [0.3, 1.0]
    }

    //==========================================================================
    // MIDI fire helpers

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

    void releaseActive()
    {
        if (pressedPad_ >= 0)
        {
            int note = (mode_ == Mode::Drum) ? midiForDrum(pressedPad_)
                                             : midiForPad(pressedPad_);
            fireNoteOff(note);
            pressedPad_ = -1;
        }
        xyDragging_ = false;
    }

    //==========================================================================
    // Mouse handlers — PAD / DRUM

    void handlePadDown(juce::Point<float> pos)
    {
        int idx = hitTestGrid(pos.x, pos.y);
        if (idx < 0)
            return;
        pressedPad_ = idx;
        float vel   = velFromY(pos.y, padBounds_[idx]);
        int note    = (mode_ == Mode::Drum) ? midiForDrum(idx) : midiForPad(idx);
        fireNoteOn(note, vel);
    }

    void handlePadUp()
    {
        if (pressedPad_ < 0)
            return;
        int note = (mode_ == Mode::Drum) ? midiForDrum(pressedPad_)
                                         : midiForPad(pressedPad_);
        fireNoteOff(note);
        pressedPad_ = -1;
    }

    //==========================================================================
    // Mouse handlers — XY

    void updateXY(juce::Point<float> pos)
    {
        auto& r = xyPadBounds_;
        if (r.getWidth() <= 0.0f || r.getHeight() <= 0.0f)
            return;
        xyX_ = juce::jlimit(0.0f, 1.0f, (pos.x - r.getX()) / r.getWidth());
        xyY_ = juce::jlimit(0.0f, 1.0f, (pos.y - r.getY()) / r.getHeight());
        if (onXYChanged)
            onXYChanged(xyX_, xyY_);
    }

    void handleXYDown(juce::Point<float> pos)
    {
        // D4 (1D-P2B): check GRID toggle pill before auto-motion pills.
        // P1-5 (1F): WCAG 2.5.8 — expand hit area by 3px per side for ≥24px effective target.
        if (xyGridPillBounds_.getWidth() > 0.0f && xyGridPillBounds_.expanded(0.0f, 3.0f).contains(pos))
        {
            xyGridVisible_ = !xyGridVisible_;
            if (onGridToggled) onGridToggled(xyGridVisible_);
            repaint();
            return;
        }

        // Check auto-motion pills first
        int pill = hitTestAutoMotionPill(pos.x, pos.y);
        if (pill >= 0)
        {
            autoMotionSel_ = (autoMotionSel_ == pill) ? -1 : pill;
            repaint();
            return;
        }

        if (xyPadBounds_.contains(pos))
        {
            xyDragging_ = true;
            updateXY(pos);
        }
    }

    void handleXYDrag(juce::Point<float> pos)
    {
        if (xyDragging_)
            updateXY(pos);
    }

    void handleXYUp()
    {
        xyDragging_ = false;
    }

    //==========================================================================
    // Paint — Header

    void paintHeader(juce::Graphics& g)
    {
        auto b = getLocalBounds().toFloat();
        auto hdr = b.withHeight(static_cast<float>(kHeaderH));

        // Header bottom border — 1px rgba(200,204,216,0.05)
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.05f));
        g.drawHorizontalLine(kHeaderH - 1, hdr.getX(), hdr.getRight());

        // Mode title — 10px weight 600, uppercase, letter-spacing 1.5px, color teal @65%
        jassert(mode_ == Mode::Pad || mode_ == Mode::Drum || mode_ == Mode::XY);
        const char* modeStr = (mode_ == Mode::Pad) ? "PAD"
                             : (mode_ == Mode::Drum) ? "DRUM"
                             : "XY";
        g.setFont(GalleryFonts::heading(10.0f));
        g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.65f));

        // Approximate letter-spacing 1.5px by using a slightly wider layout rect
        float titleX    = hdr.getX() + 14.0f;
        float titleY    = hdr.getY();
        float titleW    = hdr.getWidth() - 14.0f - 36.0f; // leave room for close btn
        float titleH    = static_cast<float>(kHeaderH);
        g.drawText(juce::String(modeStr), static_cast<int>(titleX), static_cast<int>(titleY),
                   static_cast<int>(titleW), static_cast<int>(titleH),
                   juce::Justification::centredLeft, false);

        // Close button — 22x22 rounded rect with crossed-lines X
        auto& cb = closeBtnBounds_;
        float cbRadius = 5.0f;

        // Background: hover fills with rgba(200,204,216,0.06)
        if (closeBtnHover_ || closeBtnPressed_)
        {
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(closeBtnPressed_ ? 0.10f : 0.06f));
            g.fillRoundedRectangle(cb, cbRadius);
        }

        // Border — 1px rgba(200,204,216,0.07)
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.07f));
        g.drawRoundedRectangle(cb, cbRadius, 1.0f);

        // X — two crossed lines drawn as a Path
        float alpha = closeBtnPressed_ ? 0.8f : (closeBtnHover_ ? 0.65f : 0.35f);
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(alpha));

        float cx  = cb.getCentreX();
        float cy  = cb.getCentreY();
        float arm = 4.5f; // half-length of each arm
        float lw  = 1.5f; // line width

        juce::Path xPath;
        // Diagonal /
        xPath.startNewSubPath(cx - arm, cy + arm);
        xPath.lineTo(cx + arm, cy - arm);
        // Diagonal back-slash
        xPath.startNewSubPath(cx - arm, cy - arm);
        xPath.lineTo(cx + arm, cy + arm);

        g.strokePath(xPath, juce::PathStrokeType(lw, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    //==========================================================================
    // Paint — PAD mode

    void paintPadMode(juce::Graphics& g, juce::Rectangle<float> /*contentBounds*/)
    {
        if (padBounds_[0].getWidth() <= 0.0f)
            computePadBounds();

        for (int i = 0; i < 16; ++i)
        {
            auto& r    = padBounds_[i];
            bool  pressed = (pressedPad_ == i);
            bool  hover   = (!pressed && hoverPad_ == i);

            // Background gradient
            juce::ColourGradient padGrad;
            if (pressed)
            {
                // Active: linear-gradient(to bottom, rgba(60,180,170,0.22), rgba(60,180,170,0.10))
                padGrad = juce::ColourGradient(
                    juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.22f), r.getX(), r.getY(),
                    juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.10f), r.getX(), r.getBottom(),
                    false);
            }
            else if (hover)
            {
                // Hover: linear-gradient(to bottom, rgba(60,180,170,0.09), rgba(60,180,170,0.04))
                padGrad = juce::ColourGradient(
                    juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.09f), r.getX(), r.getY(),
                    juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.04f), r.getX(), r.getBottom(),
                    false);
            }
            else
            {
                // Default: linear-gradient(145deg, rgba(200,204,216,0.05), rgba(200,204,216,0.02))
                padGrad = juce::ColourGradient(
                    juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.05f), r.getTopLeft().x, r.getTopLeft().y,
                    juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.02f), r.getBottomRight().x, r.getBottomRight().y,
                    false);
            }

            g.setGradientFill(padGrad);
            g.fillRoundedRectangle(r, kGridRadius);

            // Radial glow on active
            if (pressed)
            {
                float cx = r.getCentreX(), cy = r.getCentreY();
                float rad = std::min(r.getWidth(), r.getHeight()) * 0.5f;
                juce::ColourGradient glow(
                    juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.25f), cx, cy,
                    juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.0f),  cx + rad, cy,
                    true);
                g.setGradientFill(glow);
                g.fillRoundedRectangle(r, kGridRadius);
            }

            // Border
            if (pressed)
                g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.40f));
            else if (hover)
                g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.18f));
            else
                g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.06f));
            g.drawRoundedRectangle(r, kGridRadius, 1.0f);

            // Note name — 14px weight 600, centered
            g.setFont(GalleryFonts::heading(14.0f));
            if (pressed)
                g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.90f));
            else
                g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.45f));

            // Place note name in upper-center of pad
            float noteY  = r.getY() + r.getHeight() * 0.30f;
            float noteH  = 18.0f;
            g.drawText(juce::String(kPadNoteNames[i]),
                       static_cast<int>(r.getX()), static_cast<int>(noteY),
                       static_cast<int>(r.getWidth()), static_cast<int>(noteH),
                       juce::Justification::centred, false);

            // Velocity hint — 9px monospace, rgba(200,204,216,0.18)
            g.setFont(GalleryFonts::value(9.0f));
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.18f));

            int midiNote  = midiForPad(i);
            juce::String velHint = juce::String(midiNote);
            float hintY = r.getBottom() - 16.0f;
            g.drawText(velHint,
                       static_cast<int>(r.getX()), static_cast<int>(hintY),
                       static_cast<int>(r.getWidth()), 14,
                       juce::Justification::centred, false);
        }
    }

    //==========================================================================
    // Paint — DRUM mode

    void paintDrumMode(juce::Graphics& g, juce::Rectangle<float> /*contentBounds*/)
    {
        if (padBounds_[0].getWidth() <= 0.0f)
            computePadBounds();

        for (int i = 0; i < 16; ++i)
        {
            auto& r    = padBounds_[i];
            int   row  = i / kGridSize;
            bool  pressed = (pressedPad_ == i);
            bool  hover   = (!pressed && hoverPad_ == i);

            juce::Colour rowTint = drumTintForRow(row);

            // Background gradient
            juce::ColourGradient padGrad;
            if (pressed)
            {
                // Active: linear-gradient(135deg, rgba(140,100,220,0.18), rgba(60,180,170,0.12))
                padGrad = juce::ColourGradient(
                    juce::Colour(kPurpR, kPurpG, kPurpB).withAlpha(0.18f), r.getTopLeft().x, r.getTopLeft().y,
                    juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.12f), r.getBottomRight().x, r.getBottomRight().y,
                    false);
            }
            else if (hover)
            {
                padGrad = juce::ColourGradient(
                    juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.09f), r.getX(), r.getY(),
                    juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.04f), r.getX(), r.getBottom(),
                    false);
            }
            else
            {
                // Base: row-specific tint blended over dark
                padGrad = juce::ColourGradient(
                    juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.04f), r.getTopLeft().x, r.getTopLeft().y,
                    juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.01f), r.getBottomRight().x, r.getBottomRight().y,
                    false);
            }

            g.setGradientFill(padGrad);
            g.fillRoundedRectangle(r, kGridRadius);

            // Row tint overlay (normal state)
            if (!pressed && !hover)
            {
                g.setColour(rowTint);
                g.fillRoundedRectangle(r, kGridRadius);
            }

            // Active radial glow — purple
            if (pressed)
            {
                float cx = r.getCentreX(), cy = r.getCentreY();
                float rad = std::min(r.getWidth(), r.getHeight()) * 0.5f;
                juce::ColourGradient glow(
                    juce::Colour(kPurpR, kPurpG, kPurpB).withAlpha(0.22f), cx, cy,
                    juce::Colour(kPurpR, kPurpG, kPurpB).withAlpha(0.0f),  cx + rad, cy,
                    true);
                g.setGradientFill(glow);
                g.fillRoundedRectangle(r, kGridRadius);
            }

            // Border
            if (pressed)
                g.setColour(juce::Colour(kPurpR, kPurpG, kPurpB).withAlpha(0.40f));
            else if (hover)
                g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.18f));
            else
                g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.06f));
            g.drawRoundedRectangle(r, kGridRadius, 1.0f);

            // Label — 16px bold, centered
            g.setFont(GalleryFonts::heading(16.0f));
            if (pressed)
                g.setColour(juce::Colour(kPurpR, kPurpG, kPurpB).withAlpha(0.90f));
            else
                g.setColour(drumLabelColourForRow(row));

            g.drawText(juce::String(kDrumLabels[i]),
                       static_cast<int>(r.getX()), static_cast<int>(r.getY()),
                       static_cast<int>(r.getWidth()), static_cast<int>(r.getHeight()),
                       juce::Justification::centred, false);
        }
    }

    //==========================================================================
    // Paint — XY mode

    void paintXYMode(juce::Graphics& g, juce::Rectangle<float> /*contentBounds*/)
    {
        if (xyPadBounds_.getWidth() <= 0.0f)
            computeXYPadBounds();

        auto& xr = xyPadBounds_;

        // ---- XY Pad ----
        // D4 (1D-P2B): grid alpha depends on GRID toggle state.
        //   ON  (visible): bg α=0.20, border α=0.40
        //   OFF (ghost):   bg α=0.025, border α=0.06 (original subtle state)
        const float gridBgAlpha     = xyGridVisible_ ? 0.20f : 0.025f;
        const float gridBorderAlpha = xyGridVisible_ ? 0.40f : 0.06f;
        const float crosshairAlpha  = xyGridVisible_ ? 0.08f : 0.035f;

        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(gridBgAlpha));
        g.fillRoundedRectangle(xr, 10.0f);

        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(gridBorderAlpha));
        g.drawRoundedRectangle(xr, 10.0f, 1.0f);

        // Crosshair — horizontal + vertical center lines, 1px
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(crosshairAlpha));
        float midX = xr.getX() + xr.getWidth()  * 0.5f;
        float midY = xr.getY() + xr.getHeight() * 0.5f;
        g.drawHorizontalLine(static_cast<int>(midY), xr.getX(), xr.getRight());
        g.drawVerticalLine  (static_cast<int>(midX), xr.getY(), xr.getBottom());

        // Cursor — 14px circle at (xyX_, xyY_)
        float cursorX = xr.getX() + xyX_ * xr.getWidth();
        float cursorY = xr.getY() + xyY_ * xr.getHeight();
        float cursorR = 7.0f; // 14px diameter / 2

        // Glow shadow underneath
        juce::ColourGradient cursorGlow(
            juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.25f), cursorX, cursorY,
            juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.0f),  cursorX + cursorR * 2.2f, cursorY,
            true);
        g.setGradientFill(cursorGlow);
        g.fillEllipse(cursorX - cursorR * 1.8f, cursorY - cursorR * 1.8f,
                      cursorR * 3.6f,            cursorR * 3.6f);

        // Fill — rgba(60,180,170,0.45)
        g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.45f));
        g.fillEllipse(cursorX - cursorR, cursorY - cursorR, cursorR * 2.0f, cursorR * 2.0f);

        // Border — 2px rgba(60,180,170,0.80)
        g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.80f));
        g.drawEllipse(cursorX - cursorR, cursorY - cursorR, cursorR * 2.0f, cursorR * 2.0f, 2.0f);

        // ---- Readout row (below pad) ----
        paintXYReadouts(g);

        // ---- Auto-motion row ----
        paintXYAutoMotion(g);
    }

    void paintXYReadouts(juce::Graphics& g)
    {
        auto& rr  = xyReadoutBounds_;
        float halfW = rr.getWidth() * 0.5f;

        // X section — left half
        auto xSection = juce::Rectangle<float>(rr.getX(), rr.getY(), halfW, rr.getHeight());
        // Y section — right half
        auto ySection = juce::Rectangle<float>(rr.getX() + halfW, rr.getY(), halfW, rr.getHeight());

        // Helper lambda to draw one axis readout
        auto drawReadout = [&](juce::Rectangle<float> section,
                                const char* axisLabel, const char* paramLabel,
                                float value, const char* assignLabel)
        {
            float sX = section.getX() + 4.0f;
            float sW = section.getWidth() - 8.0f;

            // Axis + param label — 8px uppercase rgba(200,204,216,0.30)
            g.setFont(GalleryFonts::label(8.0f));
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.30f));
            juce::String axisStr = juce::String(axisLabel) + ": " + paramLabel;
            g.drawText(axisStr, static_cast<int>(sX), static_cast<int>(section.getY()),
                       static_cast<int>(sW), 14,
                       juce::Justification::centredLeft, false);

            // Value — 14px monospace rgba(60,180,170,0.80)
            g.setFont(GalleryFonts::value(14.0f));
            g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.80f));
            int pct = static_cast<int>(value * 100.0f + 0.5f);
            g.drawText(juce::String(pct) + "%",
                       static_cast<int>(sX), static_cast<int>(section.getY() + 14.0f),
                       static_cast<int>(sW * 0.5f), 18,
                       juce::Justification::centredLeft, false);

            // ASSIGN button — 9px uppercase, border, radius 5
            float btnW = 52.0f, btnH = 16.0f;
            float btnX = section.getRight() - btnW - 4.0f;
            float btnY = section.getY() + (section.getHeight() - btnH) * 0.5f;
            auto btnBounds = juce::Rectangle<float>(btnX, btnY, btnW, btnH);

            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.07f));
            g.fillRoundedRectangle(btnBounds, 5.0f);
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.07f));
            g.drawRoundedRectangle(btnBounds, 5.0f, 1.0f);

            g.setFont(GalleryFonts::label(9.0f));
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.35f));
            g.drawText(juce::String(assignLabel),
                       static_cast<int>(btnX), static_cast<int>(btnY),
                       static_cast<int>(btnW), static_cast<int>(btnH),
                       juce::Justification::centred, false);
        };

        drawReadout(xSection, "X", "CUTOFF", xyX_,  "ASSIGN X");
        drawReadout(ySection, "Y", "RES",    xyY_,  "ASSIGN Y");
    }

    void paintXYAutoMotion(juce::Graphics& g)
    {
        auto& ar = xyAutoBounds_;

        // "AUTO:" label
        g.setFont(GalleryFonts::label(8.0f));
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.30f));
        float labelW = 34.0f;
        g.drawText("AUTO:", static_cast<int>(ar.getX()), static_cast<int>(ar.getY()),
                   static_cast<int>(labelW), static_cast<int>(ar.getHeight()),
                   juce::Justification::centredLeft, false);

        // Compute and paint pills
        float pillH   = 18.0f;
        float pillPad = 4.0f;
        float pillStartX = ar.getX() + labelW + 4.0f;
        float pillY  = ar.getY() + (ar.getHeight() - pillH) * 0.5f;

        // Pre-measure pill widths
        juce::Font pillFont = GalleryFonts::heading(8.0f);
        g.setFont(pillFont);

        float curX = pillStartX;
        for (int i = 0; i < kAutoMotionCount; ++i)
        {
            float pillW = pillFont.getStringWidthFloat(juce::String(kAutoMotionLabels[i])) + pillPad * 3.0f;
            autoMotionPillBounds_[i] = juce::Rectangle<float>(curX, pillY, pillW, pillH);
            curX += pillW + 4.0f;
        }

        for (int i = 0; i < kAutoMotionCount; ++i)
        {
            auto& pb  = autoMotionPillBounds_[i];
            bool  sel = (autoMotionSel_ == i);

            // Pill background
            if (sel)
            {
                g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.20f));
                g.fillRoundedRectangle(pb, 4.0f);
                g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.40f));
                g.drawRoundedRectangle(pb, 4.0f, 1.0f);
                g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.85f));
            }
            else
            {
                g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.05f));
                g.fillRoundedRectangle(pb, 4.0f);
                g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.07f));
                g.drawRoundedRectangle(pb, 4.0f, 1.0f);
                g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.40f));
            }

            g.setFont(GalleryFonts::heading(8.0f));
            g.drawText(juce::String(kAutoMotionLabels[i]),
                       static_cast<int>(pb.getX()), static_cast<int>(pb.getY()),
                       static_cast<int>(pb.getWidth()), static_cast<int>(pb.getHeight()),
                       juce::Justification::centred, false);
        }

        // "1 bar" time dropdown stub — simple pill at far right
        float dropW = 40.0f, dropH = 18.0f;
        float dropX = ar.getRight() - dropW;
        float dropY = ar.getY() + (ar.getHeight() - dropH) * 0.5f;
        auto dropBounds = juce::Rectangle<float>(dropX, dropY, dropW, dropH);

        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.05f));
        g.fillRoundedRectangle(dropBounds, 4.0f);
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.07f));
        g.drawRoundedRectangle(dropBounds, 4.0f, 1.0f);

        g.setFont(GalleryFonts::label(8.0f));
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.35f));
        g.drawText("1 bar",
                   static_cast<int>(dropX), static_cast<int>(dropY),
                   static_cast<int>(dropW), static_cast<int>(dropH),
                   juce::Justification::centred, false);

        // D4 (1D-P2B): GRID toggle pill — left of "1 bar" dropdown.
        // All-caps pill matching XY mode aesthetic. Row 2 (auto-motion row),
        // only rendered when XY mode is active.
        float gridPillW = 34.0f, gridPillH = 18.0f;
        float gridPillX = dropX - gridPillW - 6.0f;
        float gridPillY = ar.getY() + (ar.getHeight() - gridPillH) * 0.5f;
        xyGridPillBounds_ = juce::Rectangle<float>(gridPillX, gridPillY, gridPillW, gridPillH);

        if (xyGridVisible_)
        {
            g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.15f));
            g.fillRoundedRectangle(xyGridPillBounds_, 4.0f);
            g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.35f));
            g.drawRoundedRectangle(xyGridPillBounds_, 4.0f, 1.0f);
            g.setColour(juce::Colour(kTealR, kTealG, kTealB).withAlpha(0.80f));
        }
        else
        {
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.05f));
            g.fillRoundedRectangle(xyGridPillBounds_, 4.0f);
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.08f));
            g.drawRoundedRectangle(xyGridPillBounds_, 4.0f, 1.0f);
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.35f));
        }
        g.setFont(GalleryFonts::heading(8.0f));
        g.drawText("GRID",
                   static_cast<int>(gridPillX), static_cast<int>(gridPillY),
                   static_cast<int>(gridPillW), static_cast<int>(gridPillH),
                   juce::Justification::centred, false);
    }

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SurfaceRightPanel)
};

} // namespace xoceanus
