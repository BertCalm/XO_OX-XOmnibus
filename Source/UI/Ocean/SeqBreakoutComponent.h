// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// SeqBreakoutComponent.h — Wave 5 C2: bottom slide-up sequencer breakout panel.
//
// Slides up from the bottom of the editor to ~60% of the editor height,
// mirroring the ChordBar breakout pattern.  Drives the Onset slot (slot0)
// PerEnginePatternSequencer via APVTS.
//
// Content layout (top → bottom inside the panel):
//
//   ┌─────────────────────────────────────────────────────────────────┐
//   │  drag-handle ──────────────────────────────────  [×]           │  8 px
//   ├──────────────────────────────────────────────────────────────────┤
//   │  Pattern grid   6 rows (families) × 4 cols = 24 pattern pills   │  ~120px
//   ├──────────────────────────────────────────────────────────────────┤
//   │  Step buttons   16 toggle pads (on/off via per-step gate cache)  │  ~56px
//   │  (C3 will add vertical-drag pitch edit; for C2 just on/off)      │
//   ├──────────────────────────────────────────────────────────────────┤
//   │  Controls row:  STEPS  CLOCK DIV  SWING  GATE  HUMANIZE  VEL    │  ~48px
//   └─────────────────────────────────────────────────────────────────┘
//
// APVTS parameters (prefix passed in constructor, default "slot0_seq_"):
//   _enabled    bool
//   _pattern    choice 0..23
//   _stepCount  int 1..16
//   _clockDiv   choice index 0..3 (→ 1/4 / 1/8 / 1/16 / 1/32)
//   _humanize   float 0..1
//   _baseVel    float 0..1
//   _rootNote   int 0..127
//
// NOTE — Step on/off for C2:
//   PerEnginePatternSequencer does not expose individual per-step on/off
//   as separate APVTS parameters in C1 — gates are computed algorithmically
//   per pattern.  In C2 we display the algorithmically-derived step state
//   (read from a mirror array updated by the 15 Hz timer) as read-only LEDs.
//   Clicking a pattern pill changes the whole pattern (the primary interaction).
//   Individual step overrides (C3) require new APVTS bool array params that
//   will be added in the C3 PR.
//
// Deferred to C3:
//   - Vertical-drag pitch editing per step (Maschine-style)
//   - Per-step on/off toggle override parameters
//   - Scroll-wheel velocity nudge
//
// Wave 5 C2 mount APPLIED — see OceanView.h initSeqStrip() and resized().

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include <functional>
#include <array>
#include <atomic>
#include <cmath>

namespace xoceanus
{

//==============================================================================
/**
    SeqBreakoutComponent

    Bottom slide-up sequencer panel for the Onset pilot slot (slot 0).
    Show/hide via setVisible(); parent is responsible for calling setBounds()
    to cover ~60% of editor height from the bottom edge.
*/
class SeqBreakoutComponent : public juce::Component,
                              private juce::Timer
{
public:
    // Minimum panel height (used by parent in setBounds).
    static constexpr int kMinHeight = 280;

    //==========================================================================
    // Mirror of PerEnginePatternSequencer::Pattern enum — duplicated here to
    // avoid pulling the DSP header into the UI include chain.
    static constexpr int kNumPatterns = 24;
    static constexpr int kNumFamilies = 6;
    static constexpr int kPatternsPerFamily = 4;

    static constexpr const char* kPatternNames[kNumPatterns] = {
        "Pulse","Surge","Ebb","Arc",
        "Sine","Square","Saw","Half",
        "Eucl3","Eucl5","Eucl7","Eucl9",
        "Tresillo","Clave","Backbeat","Boombap",
        "Drift","Sparkle","Foam","Riptide",
        "Fibonacci","Prime","Golden","Eddy"
    };

    static constexpr const char* kFamilyNames[kNumFamilies] = {
        "CRESTS","WAVES","REEFS","GROOVES","DRIFTS","STORMS"
    };

    // One accent colour per family (matches SeqStripComponent)
    static constexpr uint32_t kFamilyColors[kNumFamilies] = {
        0xFF48CAE4,  // CRESTS — cyan
        0xFF7B8CDE,  // WAVES  — periwinkle
        0xFF56CFB2,  // REEFS  — seafoam
        0xFFE9C46A,  // GROOVES — XO Gold
        0xFF9F7AEA,  // DRIFTS — violet
        0xFFEF6351   // STORMS — coral
    };

    static constexpr const char* kClockDivLabels[4] = { "1/4","1/8","1/16","1/32" };

    //==========================================================================
    explicit SeqBreakoutComponent(juce::AudioProcessorValueTreeState& apvts,
                                   const juce::String& slotPrefix = "slot0_seq_")
        : apvts_  (apvts)
        , prefix_ (slotPrefix)
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, true);

        // Read initial state from APVTS
        syncFromApvts();

        // 15 Hz timer — keeps step-LED mirror and param readback current
        startTimerHz(15);
    }

    ~SeqBreakoutComponent() override
    {
        stopTimer();
    }

    //==========================================================================
    /** Called by the strip to propagate the current sequencer step position
        (for the step-LED playhead highlight). Safe to call from any thread. */
    void setCurrentStep(int step) noexcept
    {
        currentStep_.store(step, std::memory_order_relaxed);
    }

    /** True when the host transport is running — used for playhead colouring. */
    void setIsPlaying(bool playing) noexcept
    {
        isPlaying_.store(playing, std::memory_order_relaxed);
    }

    /** Update the mirror of the sequencer's step-gate values so the step LEDs
        can show which steps are active for the current pattern.
        Values are 0..1 gate/velocity factors (0 = rest, >0 = gate).
        Call from a 15 Hz timer or a change-notification path.
        Safe to call from the message thread. */
    void setStepGateMirror(const std::array<float, 16>& gates) noexcept
    {
        stepGates_ = gates;
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());

        // ── Panel background + border ──
        {
            // Drop shadow at top edge — alpha ramp using two fillRect passes to avoid
            // per-frame gradient object construction (CLAUDE.md architecture rule).
            g.setColour(juce::Colour(0xFF000000).withAlpha(0.25f));
            g.fillRect(0.0f, 0.0f, w, 3.0f);
            g.setColour(juce::Colour(0xFF000000).withAlpha(0.12f));
            g.fillRect(0.0f, 3.0f, w, 3.0f);
            g.setColour(juce::Colour(0xFF000000).withAlpha(0.05f));
            g.fillRect(0.0f, 6.0f, w, 2.0f);

            // Main panel background
            g.setColour(juce::Colour(0xFF0E111A));
            g.fillRect(0.0f, 8.0f, w, h - 8.0f);

            // Top drag-handle bar
            g.setColour(juce::Colour(0xFF1A1F2E));
            g.fillRect(0.0f, 0.0f, w, 8.0f);

            // Drag handle pill
            g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.25f));
            const float handleW = 32.0f;
            g.fillRoundedRectangle(w * 0.5f - handleW * 0.5f, 2.5f, handleW, 3.0f, 1.5f);

            // Top border
            g.setColour(juce::Colour(0xFF48CAE4).withAlpha(0.15f));
            g.fillRect(0.0f, 8.0f, w, 1.0f);
        }

        // Layout sub-sections
        layoutAndPaint(g, w, h);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        const juce::Point<float> pos = e.position;
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());

        // ── Close button hit test ──
        {
            const float cbSize = 16.0f;
            juce::Rectangle<float> cb(w - cbSize - 8.0f, 8.0f + 6.0f, cbSize, cbSize);
            if (cb.contains(pos))
            {
                setVisible(false);
                return;
            }
        }

        // ── Pattern grid hit test ──
        if (patternGridBounds_.contains(pos))
        {
            handlePatternGridClick(pos);
            return;
        }

        // ── Step toggle hit test ──
        // C2 shows read-only step LEDs derived from the algorithm.
        // Clicking a step pill selects the closest pattern that accentuates it
        // (deferred full override to C3 — for C2 just repaint).
        if (stepRowBounds_.contains(pos))
        {
            // No-op for C2 (step on/off overrides are a C3 feature).
            // C3 TODO: set per-step bool parameter slot0_seq_stepOverride_i.
            return;
        }

        // ── Controls row hit test ──
        handleControlsClick(pos, w, h);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        // Drag on slider controls (horizontal)
        const juce::Point<float> pos = e.position;
        if (activeSlider_ != SliderTarget::None)
        {
            updateSliderFromDrag(pos.x);
            return;
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        activeSlider_ = SliderTarget::None;
        dragStartX_   = e.position.x;

        // Identify which control track was pressed
        if (swingTrack_.contains(e.position))
        {
            activeSlider_ = SliderTarget::Swing;
            dragStartVal_ = currentSwing_;
        }
        else if (gateTrack_.contains(e.position))
        {
            activeSlider_ = SliderTarget::Gate;
            dragStartVal_ = currentGate_;
        }
        else if (humanizeTrack_.contains(e.position))
        {
            activeSlider_ = SliderTarget::Humanize;
            dragStartVal_ = currentHumanize_;
        }
        else if (velTrack_.contains(e.position))
        {
            activeSlider_ = SliderTarget::Velocity;
            dragStartVal_ = currentVelocity_;
        }
    }

    void mouseExit(const juce::MouseEvent& /*e*/) override
    {
        hoveredPattern_ = -1;
        repaint();
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const int prev = hoveredPattern_;
        hoveredPattern_ = -1;

        if (patternGridBounds_.contains(e.position))
        {
            int hit = hitTestPatternGrid(e.position);
            if (hit >= 0)
                hoveredPattern_ = hit;
        }

        if (hoveredPattern_ != prev)
            repaint();
    }

private:
    //==========================================================================
    // Slider targets
    enum class SliderTarget { None, Swing, Gate, Humanize, Velocity };

    //==========================================================================
    // ── Layout constants ──
    static constexpr float kHandleH    = 8.0f;
    static constexpr float kSectionGap = 10.0f;
    static constexpr float kPadding    = 12.0f;

    //==========================================================================
    // ── Main layout + paint ──

    void layoutAndPaint(juce::Graphics& g, float w, float h)
    {
        float y = kHandleH + kSectionGap;

        // ── Pattern grid ──
        const float gridH = paintPatternGrid(g, kPadding, y, w - kPadding * 2.0f);
        patternGridBounds_ = juce::Rectangle<float>(kPadding, y, w - kPadding * 2.0f, gridH);
        y += gridH + kSectionGap;

        // ── Divider ──
        paintDivider(g, y, w);
        y += 1.0f + kSectionGap;

        // ── Step LEDs row ──
        const float stepH = paintStepRow(g, kPadding, y, w - kPadding * 2.0f);
        stepRowBounds_ = juce::Rectangle<float>(kPadding, y, w - kPadding * 2.0f, stepH);
        y += stepH + kSectionGap;

        // ── Divider ──
        paintDivider(g, y, w);
        y += 1.0f + kSectionGap;

        // ── Controls row ──
        paintControlsRow(g, kPadding, y, w - kPadding * 2.0f, h - y - kPadding);

        // ── Close button ──
        paintCloseButton(g, w);
    }

    //--------------------------------------------------------------------------
    /** Paint the 6×4 pattern pill grid. Returns height consumed. */
    float paintPatternGrid(juce::Graphics& g, float x, float y, float areaW)
    {
        static const juce::Font pillFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(9.0f));

        static const juce::Font familyFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(8.0f));

        const int  selectedPattern = currentPattern_;
        const float familyLabelW   = 54.0f;
        const float pillAreaW      = areaW - familyLabelW - 4.0f;
        const float pillW          = (pillAreaW - static_cast<float>(kPatternsPerFamily - 1) * 4.0f)
                                       / static_cast<float>(kPatternsPerFamily);
        const float pillH          = 18.0f;
        const float rowGap         = 5.0f;
        const float totalH         = static_cast<float>(kNumFamilies) * pillH
                                       + static_cast<float>(kNumFamilies - 1) * rowGap;

        // Clear cached pill rects
        for (auto& r : pillRects_)
            r = juce::Rectangle<float>();

        for (int fam = 0; fam < kNumFamilies; ++fam)
        {
            const float rowY        = y + static_cast<float>(fam) * (pillH + rowGap);
            const juce::Colour famCol = juce::Colour(kFamilyColors[fam]);

            // Family label
            g.setFont(familyFont);
            g.setColour(famCol.withAlpha(0.55f));
            g.drawText(juce::String(kFamilyNames[fam]),
                       juce::Rectangle<float>(x, rowY, familyLabelW, pillH).toNearestInt(),
                       juce::Justification::centredLeft, false);

            for (int col = 0; col < kPatternsPerFamily; ++col)
            {
                const int patIdx = fam * kPatternsPerFamily + col;
                const float px   = x + familyLabelW + 4.0f + static_cast<float>(col) * (pillW + 4.0f);
                const juce::Rectangle<float> pill(px, rowY, pillW, pillH);

                // Cache for hit testing
                pillRects_[patIdx] = pill;

                const bool isSelected = (patIdx == selectedPattern);
                const bool isHovered  = (patIdx == hoveredPattern_);

                juce::Colour bg, border, text;
                if (isSelected)
                {
                    bg     = famCol.withAlpha(0.22f);
                    border = famCol.withAlpha(0.60f);
                    text   = famCol;
                }
                else if (isHovered)
                {
                    bg     = famCol.withAlpha(0.08f);
                    border = famCol.withAlpha(0.28f);
                    text   = juce::Colour(0xFFE8E4DF).withAlpha(0.80f);
                }
                else
                {
                    bg     = juce::Colours::transparentBlack;
                    border = juce::Colour(0xFF9E9B97).withAlpha(0.12f);
                    text   = juce::Colour(0xFF9E9B97).withAlpha(0.55f);
                }

                if (!bg.isTransparent())
                {
                    g.setColour(bg);
                    g.fillRoundedRectangle(pill, 3.0f);
                }

                g.setColour(border);
                g.drawRoundedRectangle(pill, 3.0f, 1.0f);

                g.setFont(pillFont);
                g.setColour(text);
                g.drawText(juce::String(kPatternNames[patIdx]).toUpperCase(),
                           pill.toNearestInt(),
                           juce::Justification::centred, true);
            }
        }

        return totalH;
    }

    //--------------------------------------------------------------------------
    /** Paint the 16-step LED row.  Returns height consumed. */
    float paintStepRow(juce::Graphics& g, float x, float y, float areaW)
    {
        static const juce::Font labelFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(8.0f));

        const float ledH      = 28.0f;
        const float gap       = 3.0f;
        const float ledW      = (areaW - 15.0f * gap) / 16.0f;
        const int   stepCount = currentStepCount_;
        const int   curStep   = currentStep_.load(std::memory_order_relaxed);
        const bool  playing   = isPlaying_.load(std::memory_order_relaxed);
        const bool  enabled   = currentEnabled_;
        const int   patIdx    = currentPattern_;
        const int   famIdx    = juce::jlimit(0, kNumFamilies - 1, patIdx / kPatternsPerFamily);
        const juce::Colour famCol = juce::Colour(kFamilyColors[famIdx]);

        // Row header
        g.setFont(labelFont);
        g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.40f));
        g.drawText("STEPS", juce::Rectangle<float>(x, y - 14.0f, 40.0f, 12.0f).toNearestInt(),
                   juce::Justification::centredLeft, false);

        for (int i = 0; i < 16; ++i)
        {
            const float lx       = x + static_cast<float>(i) * (ledW + gap);
            const bool  inRange  = (i < stepCount);
            const bool  isCursor = (playing && enabled && i == curStep && inRange);
            const bool  hasGate  = inRange && (stepGates_[static_cast<size_t>(i)] > 0.0f);

            // LED rectangle
            juce::Rectangle<float> ledRect(lx, y, ledW, ledH);

            juce::Colour ledBg, ledBorder;
            if (isCursor)
            {
                // Playhead: bright family colour
                ledBg     = famCol.withAlpha(0.90f);
                ledBorder = famCol;
            }
            else if (hasGate && inRange)
            {
                // Active step with gate: coloured
                const float gateFactor = juce::jlimit(0.2f, 1.0f, stepGates_[static_cast<size_t>(i)]);
                ledBg     = famCol.withAlpha(0.20f + gateFactor * 0.25f);
                ledBorder = famCol.withAlpha(0.35f + gateFactor * 0.30f);
            }
            else if (inRange)
            {
                // In range but rest (gate == 0)
                ledBg     = juce::Colour(0xFF1A1F2E);
                ledBorder = juce::Colour(0xFF9E9B97).withAlpha(0.12f);
            }
            else
            {
                // Out of range (beyond stepCount)
                ledBg     = juce::Colour(0xFF0E0E10);
                ledBorder = juce::Colour(0xFF9E9B97).withAlpha(0.06f);
            }

            g.setColour(ledBg);
            g.fillRoundedRectangle(ledRect, 3.0f);
            g.setColour(ledBorder);
            g.drawRoundedRectangle(ledRect, 3.0f, 1.0f);

            // Step number (1-indexed, small, bottom-aligned)
            if (inRange)
            {
                g.setFont(labelFont);
                g.setColour(isCursor ? juce::Colour(0xFF0E111A)
                                     : juce::Colour(0xFF9E9B97).withAlpha(0.30f));
                g.drawText(juce::String(i + 1),
                           juce::Rectangle<float>(lx, y + ledH - 11.0f, ledW, 10.0f).toNearestInt(),
                           juce::Justification::centred, false);
            }
        }

        // C3 deferred annotation
        static const juce::Font deferredFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withHeight(7.5f));
        g.setFont(deferredFont);
        g.setColour(juce::Colour(0xFF5E6878));
        g.drawText("pitch edit in C3", juce::Rectangle<float>(x, y + ledH + 2.0f, areaW, 10.0f).toNearestInt(),
                   juce::Justification::centredRight, false);

        return ledH + 14.0f; // +14 for the header label + deferred annotation text
    }

    //--------------------------------------------------------------------------
    /** Paint the horizontal controls row (Steps / Clock Div / Swing / Gate / Humanize / Vel). */
    void paintControlsRow(juce::Graphics& g, float x, float y, float areaW, float /*areaH*/)
    {
        static const juce::Font labelFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(8.0f));
        static const juce::Font valueFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withHeight(9.0f));

        // Each control occupies equal width
        const float ctrlW  = areaW / 6.0f;
        const float rowH   = 38.0f;

        struct CtrlInfo
        {
            const char* label;
            const char* valueStr;
            float       sliderVal; // 0..1 normalised for slider controls, -1 for non-slider
        };

        // Build display strings
        const int stepCount = juce::jlimit(1, 16, currentStepCount_);
        const int clockIdx  = juce::jlimit(0, 3, currentClockDiv_);

        // Rebuild slider layout rects (used in mouse hit-testing)
        const float trackY    = y + rowH * 0.62f;
        const float trackH_px = 3.0f;

        for (int i = 0; i < 6; ++i)
        {
            const float cx = x + static_cast<float>(i) * ctrlW;
            const juce::Rectangle<float> cell(cx, y, ctrlW - 4.0f, rowH);

            // Label
            const char* labelText = nullptr;
            juce::String valStr;
            bool isSlider = false;
            float sliderVal = 0.0f;

            switch (i)
            {
            case 0:
                labelText = "STEPS";
                valStr    = juce::String(stepCount);
                break;
            case 1:
                labelText = "CLOCK";
                valStr    = juce::String(kClockDivLabels[clockIdx]);
                break;
            case 2:
                labelText = "SWING";
                isSlider  = true;
                sliderVal = currentSwing_;
                valStr    = juce::String(static_cast<int>(currentSwing_ * 100)) + "%";
                swingTrack_ = juce::Rectangle<float>(cx + 4.0f, trackY - 6.0f, ctrlW - 12.0f, 12.0f);
                break;
            case 3:
                labelText = "GATE";
                isSlider  = true;
                sliderVal = currentGate_;
                valStr    = juce::String(static_cast<int>(currentGate_ * 100)) + "%";
                gateTrack_ = juce::Rectangle<float>(cx + 4.0f, trackY - 6.0f, ctrlW - 12.0f, 12.0f);
                break;
            case 4:
                labelText = "HUMAN";
                isSlider  = true;
                sliderVal = currentHumanize_;
                valStr    = juce::String(static_cast<int>(currentHumanize_ * 100)) + "%";
                humanizeTrack_ = juce::Rectangle<float>(cx + 4.0f, trackY - 6.0f, ctrlW - 12.0f, 12.0f);
                break;
            case 5:
                labelText = "VEL";
                isSlider  = true;
                sliderVal = currentVelocity_;
                valStr    = juce::String(static_cast<int>(currentVelocity_ * 100)) + "%";
                velTrack_ = juce::Rectangle<float>(cx + 4.0f, trackY - 6.0f, ctrlW - 12.0f, 12.0f);
                break;
            }

            // Label
            g.setFont(labelFont);
            g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.40f));
            g.drawText(juce::String(labelText),
                       juce::Rectangle<float>(cx, y, ctrlW - 4.0f, 11.0f).toNearestInt(),
                       juce::Justification::centred, false);

            // Value / slider
            if (isSlider)
            {
                const float trackX1 = cx + 4.0f;
                const float trackX2 = cx + ctrlW - 8.0f;
                const float ty      = trackY;

                // Track
                g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.10f));
                g.fillRoundedRectangle(trackX1, ty - trackH_px * 0.5f, trackX2 - trackX1, trackH_px, 1.5f);

                // Fill to thumb
                g.setColour(juce::Colour(0xFF48CAE4).withAlpha(0.28f));
                g.fillRoundedRectangle(trackX1, ty - trackH_px * 0.5f,
                                       (trackX2 - trackX1) * sliderVal, trackH_px, 1.5f);

                // Thumb
                const float thumbX = trackX1 + sliderVal * (trackX2 - trackX1);
                g.setColour(juce::Colour(0xFF48CAE4).withAlpha(0.85f));
                g.fillEllipse(thumbX - 5.0f, ty - 5.0f, 10.0f, 10.0f);

                // Value text
                g.setFont(valueFont);
                g.setColour(juce::Colour(0xFFE8E4DF).withAlpha(0.70f));
                g.drawText(valStr,
                           juce::Rectangle<float>(cx, trackY + 8.0f, ctrlW - 4.0f, 12.0f).toNearestInt(),
                           juce::Justification::centred, false);
            }
            else
            {
                // Steps / Clock: clickable value pill
                const float pillH = 18.0f;
                juce::Rectangle<float> vPill(cx + 4.0f, y + 12.0f, ctrlW - 12.0f, pillH);

                // Store pill rect for hit testing
                if (i == 0)       stepsPill_    = vPill;
                else if (i == 1)  clockDivPill_ = vPill;

                g.setColour(juce::Colour(0xFF1A1F2E));
                g.fillRoundedRectangle(vPill, 3.0f);
                g.setColour(juce::Colour(0xFF48CAE4).withAlpha(0.20f));
                g.drawRoundedRectangle(vPill, 3.0f, 1.0f);

                g.setFont(valueFont);
                g.setColour(juce::Colour(0xFFE8E4DF).withAlpha(0.80f));
                g.drawText(valStr, vPill.toNearestInt(), juce::Justification::centred, false);
            }
        }

        // Enabled toggle at far right
        const float enabledPillW = 40.0f;
        const float enabledPillH = 16.0f;
        juce::Rectangle<float> ePill(x + areaW - enabledPillW, y, enabledPillW, enabledPillH);
        enabledPill_ = ePill;

        const juce::Colour enCol = currentEnabled_ ? juce::Colour(0xFF48CAE4) : juce::Colour(0xFF5E6878);
        g.setColour(enCol.withAlpha(currentEnabled_ ? 0.18f : 0.08f));
        g.fillRoundedRectangle(ePill, 4.0f);
        g.setColour(enCol.withAlpha(0.50f));
        g.drawRoundedRectangle(ePill, 4.0f, 1.0f);
        static const juce::Font enableFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(9.0f));
        g.setFont(enableFont);
        g.setColour(enCol);
        g.drawText(currentEnabled_ ? "ON" : "OFF", ePill.toNearestInt(), juce::Justification::centred, false);
    }

    //--------------------------------------------------------------------------
    void paintDivider(juce::Graphics& g, float y, float w) const
    {
        g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.08f));
        g.fillRect(kPadding, y, w - kPadding * 2.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    void paintCloseButton(juce::Graphics& g, float w) const
    {
        static const juce::Font iconFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withHeight(10.0f));

        const float cbSize = 16.0f;
        juce::Rectangle<float> cb(w - cbSize - 8.0f, kHandleH + 6.0f, cbSize, cbSize);
        g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.25f));
        g.fillRoundedRectangle(cb, 3.0f);
        g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.65f));
        g.drawRoundedRectangle(cb, 3.0f, 1.0f);
        g.setFont(iconFont);
        g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.80f));
        g.drawText("X", cb.toNearestInt(), juce::Justification::centred, false);
    }

    //==========================================================================
    // ── Hit testing helpers ──

    int hitTestPatternGrid(const juce::Point<float>& pos) const
    {
        for (int i = 0; i < kNumPatterns; ++i)
            if (!pillRects_[i].isEmpty() && pillRects_[i].contains(pos))
                return i;
        return -1;
    }

    void handlePatternGridClick(const juce::Point<float>& pos)
    {
        const int hit = hitTestPatternGrid(pos);
        if (hit < 0) return;

        setApvtsParamByIndex("pattern", hit, kNumPatterns - 1);
        currentPattern_ = hit;
        repaint();
    }

    void handleControlsClick(const juce::Point<float>& pos, float /*w*/, float /*h*/)
    {
        // Enabled toggle
        if (enabledPill_.contains(pos))
        {
            const bool newVal = !currentEnabled_;
            setApvtsBool("enabled", newVal);
            currentEnabled_ = newVal;
            repaint();
            return;
        }

        // Steps pill — scroll or click to cycle
        if (stepsPill_.contains(pos))
        {
            const int newSteps = (currentStepCount_ % 16) + 1;  // 1→2→...→16→1
            setApvtsParamValue("stepCount", static_cast<float>(newSteps));
            currentStepCount_ = newSteps;
            repaint();
            return;
        }

        // Clock div pill — click to cycle
        if (clockDivPill_.contains(pos))
        {
            const int newIdx = (currentClockDiv_ + 1) % 4;
            setApvtsParamByIndex("clockDiv", newIdx, 3);
            currentClockDiv_ = newIdx;
            repaint();
            return;
        }
    }

    void updateSliderFromDrag(float newX)
    {
        if (activeSlider_ == SliderTarget::None) return;

        juce::Rectangle<float>* trackRect = nullptr;
        float* valuePtr = nullptr;
        const char* paramSuffix = nullptr;

        switch (activeSlider_)
        {
        case SliderTarget::Swing:
            trackRect  = &swingTrack_;
            valuePtr   = &currentSwing_;
            paramSuffix = "humanize"; // swing lives in humanize for C2 (no separate APVTS param)
            // Note: PerEnginePatternSequencer C1 does not have a separate swing param.
            // Swing is deferred — for C2 we display it as a visual-only control.
            // TODO C3: add slot0_seq_swing APVTS parameter.
            break;
        case SliderTarget::Gate:
            // Gate length also not a separate C1 param — visual only for C2.
            trackRect  = &gateTrack_;
            valuePtr   = &currentGate_;
            paramSuffix = nullptr; // visual only until C3 adds slot0_seq_gateLen
            break;
        case SliderTarget::Humanize:
            trackRect  = &humanizeTrack_;
            valuePtr   = &currentHumanize_;
            paramSuffix = "humanize";
            break;
        case SliderTarget::Velocity:
            trackRect  = &velTrack_;
            valuePtr   = &currentVelocity_;
            paramSuffix = "baseVel";
            break;
        default:
            return;
        }

        if (trackRect == nullptr || trackRect->isEmpty()) return;

        const float x0   = trackRect->getX();
        const float x1   = trackRect->getRight();
        const float range = x1 - x0;
        if (range <= 0.0f) return;

        const float newVal = juce::jlimit(0.0f, 1.0f, (newX - x0) / range);
        if (valuePtr != nullptr)
            *valuePtr = newVal;

        if (paramSuffix != nullptr)
            setApvtsFloat(paramSuffix, newVal);

        repaint();
    }

    //==========================================================================
    // ── APVTS helpers ──

    void setApvtsBool(const juce::String& suffix, bool val)
    {
        if (auto* p = apvts_.getParameter(prefix_ + suffix))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(val ? 1.0f : 0.0f);
            p->endChangeGesture();
        }
    }

    void setApvtsFloat(const juce::String& suffix, float val)
    {
        if (auto* p = apvts_.getParameter(prefix_ + suffix))
        {
            p->beginChangeGesture();
            // convertTo0to1 handles range mapping (works for AudioParameterFloat)
            p->setValueNotifyingHost(p->convertTo0to1(val));
            p->endChangeGesture();
        }
    }

    // Generic setter that uses the parameter's own convertTo0to1 mapping.
    // Works for AudioParameterInt, AudioParameterChoice, and AudioParameterFloat
    // because JUCE's default implementations handle their respective ranges.
    void setApvtsParamValue(const juce::String& suffix, float realValue)
    {
        if (auto* p = apvts_.getParameter(prefix_ + suffix))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(realValue));
            p->endChangeGesture();
        }
    }

    // Convenience for choice / int params where we know the integer index directly.
    void setApvtsParamByIndex(const juce::String& suffix, int idx, int /*maxIdx*/)
    {
        // Pass the raw integer as a float — JUCE AudioParameterInt/Choice
        // convertTo0to1 maps (idx - rangeStart) / rangeLength correctly.
        setApvtsParamValue(suffix, static_cast<float>(idx));
    }

    int readParamInt(const juce::String& suffix, int fallback) const
    {
        if (auto* p = apvts_.getParameter(prefix_ + suffix))
            return static_cast<int>(p->convertFrom0to1(p->getValue()) + 0.5f);
        return fallback;
    }

    float readParamFloat(const juce::String& suffix, float fallback) const
    {
        if (auto* p = apvts_.getParameter(prefix_ + suffix))
            return p->convertFrom0to1(p->getValue());
        return fallback;
    }

    bool readParamBool(const juce::String& suffix) const
    {
        if (auto* p = apvts_.getParameter(prefix_ + suffix))
            return p->convertFrom0to1(p->getValue()) > 0.5f;
        return false;
    }

    //--------------------------------------------------------------------------
    void syncFromApvts()
    {
        currentPattern_   = juce::jlimit(0, kNumPatterns - 1, readParamInt("pattern", 0));
        currentStepCount_ = juce::jlimit(1, 16, readParamInt("stepCount", 16));
        currentClockDiv_  = juce::jlimit(0, 3, readParamInt("clockDiv", 2));
        currentHumanize_  = juce::jlimit(0.0f, 1.0f, readParamFloat("humanize", 0.0f));
        currentVelocity_  = juce::jlimit(0.0f, 1.0f, readParamFloat("baseVel", 0.75f));
        currentEnabled_   = readParamBool("enabled");
    }

    //==========================================================================
    void timerCallback() override
    {
        syncFromApvts();
        if (isShowing())
            repaint();
    }

    //==========================================================================
    // References
    juce::AudioProcessorValueTreeState& apvts_;
    juce::String                        prefix_;

    // APVTS-mirrored state
    int   currentPattern_   = 0;
    int   currentStepCount_ = 16;
    int   currentClockDiv_  = 2;    // default index 2 → 1/16
    float currentSwing_     = 0.0f;
    float currentGate_      = 0.50f;
    float currentHumanize_  = 0.0f;
    float currentVelocity_  = 0.75f;
    bool  currentEnabled_   = false;

    // Step gate mirror (updated by setStepGateMirror from parent)
    std::array<float, 16> stepGates_{};

    // Playback state (updated by parent at 15 Hz)
    std::atomic<int>  currentStep_{0};
    std::atomic<bool> isPlaying_{false};

    // Layout rectangles (rebuilt each paint)
    juce::Rectangle<float> patternGridBounds_;
    juce::Rectangle<float> stepRowBounds_;

    // Pill rects for pattern grid hit testing
    std::array<juce::Rectangle<float>, kNumPatterns> pillRects_{};

    // Control hit-test rects
    juce::Rectangle<float> swingTrack_;
    juce::Rectangle<float> gateTrack_;
    juce::Rectangle<float> humanizeTrack_;
    juce::Rectangle<float> velTrack_;
    juce::Rectangle<float> stepsPill_;
    juce::Rectangle<float> clockDivPill_;
    juce::Rectangle<float> enabledPill_;

    // Drag state
    SliderTarget activeSlider_ = SliderTarget::None;
    float        dragStartX_   = 0.0f;
    float        dragStartVal_ = 0.0f;

    // Hover state
    int hoveredPattern_ = -1;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SeqBreakoutComponent)
};

} // namespace xoceanus
