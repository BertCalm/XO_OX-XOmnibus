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
// NOTE — Step interaction (C3):
//   Tap a step LED to toggle its gate override (on/off).  When any gate override
//   exists the bitmap is non-zero; the DSP engine respects the full bitmap mask.
//   Vertical drag on a step LED adjusts its pitch offset in ±12 semitone range
//   (Maschine-style): drag up = raise, drag down = lower.  First ~5px of drag
//   determines direction lock (horizontal = gate toggle intent, vertical = pitch).
//   A small arrow glyph is painted on any step whose pitch offset != 0.
//
// #1298 (long-press detail panel) — IMPLEMENTED:
//   Long-press (~300ms) on a step LED opens a CallOutBox popover with per-step
//   velocity, gate length, and pitch offset sliders.
//   - Scroll-wheel velocity nudge: deferred.
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
    StepDetailContent (#1298)

    Contents shown inside the per-step CallOutBox: velocity, gate length, and
    pitch offset sliders for one step.  The owning SeqBreakoutComponent creates
    a StepDetailContent, wraps it in a juce::CallOutBox, and disposes it on close.
*/
class StepDetailContent : public juce::Component
{
public:
    static constexpr int kWidth  = 200;
    static constexpr int kHeight = 148;

    // Callback fired whenever a value changes (prefix + param suffix, new value).
    std::function<void(const juce::String& suffix, float value)> onParamChange;

    StepDetailContent(int stepIdx,
                      float initVel,    // 0.0 = inherit
                      float initGlen,   // 0.0 = inherit
                      int   initPitch,  // semitones ±12
                      const juce::String& stepLabel)
        : stepIdx_(stepIdx)
        , vel_   (juce::jlimit(0.0f, 1.0f,  initVel))
        , glen_  (juce::jlimit(0.0f, 1.5f,  initGlen))
        , pitch_ (juce::jlimit(-12, 12, initPitch))
    {
        setSize(kWidth, kHeight);
        ignoreUnused(stepLabel);
    }

    void paint(juce::Graphics& g) override
    {
        // Background
        g.setColour(juce::Colour(0xFF151820));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

        static const juce::Font labelFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(8.5f));
        static const juce::Font valueFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withHeight(9.0f));

        const float w     = static_cast<float>(getWidth());
        const float rowH  = 36.0f;
        const float padX  = 12.0f;
        const float trackX1 = padX + 50.0f;
        const float trackX2 = w - padX;
        const float trackW  = trackX2 - trackX1;
        const float trackH  = 3.0f;

        struct Row { const char* label; float norm; float min; float max; const char* unit; };

        // Normalise pitch -12..+12 → 0..1 for slider display
        const float pitchNorm = (static_cast<float>(pitch_) + 12.0f) / 24.0f;

        const Row rows[3] = {
            { "VEL",    vel_ > 0.0f ? vel_ : 0.0f,   0.0f, 1.0f,  "%" },
            { "GLEN",   glen_ > 0.0f ? glen_ / 1.5f : 0.0f, 0.0f, 1.0f, "%" },
            { "PITCH",  pitchNorm,   0.0f, 1.0f, "st" }
        };

        const float trackY0 = 22.0f;
        const juce::Colour accent(0xFF48CAE4);

        for (int i = 0; i < 3; ++i)
        {
            const float ty  = trackY0 + static_cast<float>(i) * rowH;
            const float mid = ty + rowH * 0.5f;

            // Label
            g.setFont(labelFont);
            g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.55f));
            g.drawText(juce::String(rows[i].label),
                       juce::Rectangle<float>(padX, ty + 4.0f, 48.0f, 12.0f).toNearestInt(),
                       juce::Justification::centredLeft, false);

            // Track
            const float fill = rows[i].norm * trackW;
            g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.10f));
            g.fillRoundedRectangle(trackX1, mid - trackH * 0.5f, trackW, trackH, 1.5f);
            g.setColour(accent.withAlpha(0.28f));
            g.fillRoundedRectangle(trackX1, mid - trackH * 0.5f, fill, trackH, 1.5f);

            // Thumb
            const float thumbX = trackX1 + fill;
            g.setColour(accent.withAlpha(0.85f));
            g.fillEllipse(thumbX - 5.0f, mid - 5.0f, 10.0f, 10.0f);

            // Cache hit-test rect for drag handling
            trackRects_[i] = juce::Rectangle<float>(trackX1, mid - 8.0f, trackW, 16.0f);

            // Value label
            juce::String valStr;
            if (i == 0)
                valStr = vel_ <= 0.0f ? "inherit" : juce::String(static_cast<int>(vel_ * 127));
            else if (i == 1)
                valStr = glen_ <= 0.0f ? "inherit" : juce::String(static_cast<int>(glen_ * 100)) + "%";
            else
                valStr = (pitch_ >= 0 ? "+" : "") + juce::String(pitch_) + " st";

            g.setFont(valueFont);
            g.setColour(juce::Colour(0xFFE8E4DF).withAlpha(0.75f));
            g.drawText(valStr,
                       juce::Rectangle<float>(trackX1, mid + 5.0f, trackW, 11.0f).toNearestInt(),
                       juce::Justification::centred, false);
        }

        // Step number header
        g.setFont(labelFont);
        g.setColour(accent.withAlpha(0.60f));
        g.drawText("STEP " + juce::String(stepIdx_ + 1),
                   juce::Rectangle<float>(0.0f, 4.0f, w, 12.0f).toNearestInt(),
                   juce::Justification::centred, false);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        dragRow_      = -1;
        dragStartX_   = e.position.x;
        for (int i = 0; i < 3; ++i)
        {
            if (trackRects_[i].contains(e.position))
            {
                dragRow_ = i;
                break;
            }
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (dragRow_ < 0) return;

        const float x0     = trackRects_[dragRow_].getX();
        const float trackW = trackRects_[dragRow_].getWidth();
        if (trackW <= 0.0f) return;

        const float norm = juce::jlimit(0.0f, 1.0f, (e.position.x - x0) / trackW);

        if (dragRow_ == 0)
        {
            vel_ = norm; // 0 = inherit (left edge), >0 = override
            if (onParamChange)
                onParamChange("svel_" + juce::String(stepIdx_), vel_);
        }
        else if (dragRow_ == 1)
        {
            glen_ = norm * 1.5f;
            if (onParamChange)
                onParamChange("glen_" + juce::String(stepIdx_), glen_);
        }
        else // pitch
        {
            pitch_ = juce::jlimit(-12, 12, static_cast<int>(std::round(norm * 24.0f - 12.0f)));
            if (onParamChange)
                onParamChange("pitch_" + juce::String(stepIdx_), static_cast<float>(pitch_));
        }
        repaint();
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        // Double-click on a track resets it to inherit / zero.
        for (int i = 0; i < 3; ++i)
        {
            if (trackRects_[i].contains(e.position))
            {
                if (i == 0) { vel_ = 0.0f;  if (onParamChange) onParamChange("svel_"  + juce::String(stepIdx_), 0.0f); }
                if (i == 1) { glen_ = 0.0f; if (onParamChange) onParamChange("glen_"  + juce::String(stepIdx_), 0.0f); }
                if (i == 2) { pitch_ = 0;   if (onParamChange) onParamChange("pitch_" + juce::String(stepIdx_), 0.0f); }
                repaint();
                return;
            }
        }
    }

private:
    int   stepIdx_ = 0;
    float vel_     = 0.0f;
    float glen_    = 0.0f;
    int   pitch_   = 0;

    std::array<juce::Rectangle<float>, 3> trackRects_{};
    int   dragRow_    = -1;
    float dragStartX_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepDetailContent)
};

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

        // ── Step LED gate toggle (C3) ──
        // If the mouseDown was on a step LED and drag did not resolve as a pitch edit,
        // interpret the mouseUp as a gate toggle — unless a long-press popup was shown.
        if (dragStepIdx_ >= 0)
        {
            // #1298: if long-press popup already opened, skip the gate toggle.
            if (!longPressTriggered_ && !stepDragConsumed_ && stepDragMode_ != StepDragMode::Pitch)
            {
                // Tap: toggle gate override for this step.
                const bool newGate = !stepGateOverrides_[static_cast<size_t>(dragStepIdx_)];
                stepGateOverrides_[static_cast<size_t>(dragStepIdx_)] = newGate;
                setApvtsBool("gate_" + juce::String(dragStepIdx_), newGate);
                repaint();
            }
            // Reset drag state
            dragStepIdx_        = -1;
            stepDragMode_       = StepDragMode::Pending;
            stepDragConsumed_   = false;
            longPressStep_      = -1;
            longPressElapsed_   = 0;
            longPressTriggered_ = false;
            return;
        }

        // ── Step row area fallthrough guard ──
        if (stepRowBounds_.contains(pos))
            return;

        // ── Controls row hit test ──
        handleControlsClick(pos, w, h);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        // C3: handle step LED pitch drag (Maschine-style vertical edit)
        if (dragStepIdx_ >= 0)
        {
            const float dy = e.position.y - dragStepStartY_;
            const float dx = e.position.x - dragStartX_;

            // Direction lock: first ~5px movement resolves the mode
            if (stepDragMode_ == StepDragMode::Pending)
            {
                const float dist = std::sqrt(dx * dx + dy * dy);
                if (dist > 5.0f)
                {
                    if (std::abs(dy) >= std::abs(dx))
                        stepDragMode_ = StepDragMode::Pitch;
                    else
                        stepDragMode_ = StepDragMode::GateToggle;

                    // #1298: any drag movement cancels the long-press popup countdown.
                    longPressStep_    = -1;
                    longPressElapsed_ = 0;
                }
            }

            if (stepDragMode_ == StepDragMode::Pitch)
            {
                // Vertical drag: each ~8px = 1 semitone. Drag up = positive offset.
                // Using negative dy because screen Y increases downward.
                const float kPxPerSemitone = 8.0f;
                const int delta = static_cast<int>(std::round(-dy / kPxPerSemitone));
                const int newPitch = juce::jlimit(-12, 12, dragStepStartPitch_ + delta);

                if (newPitch != static_cast<int>(stepPitchOffsets_[static_cast<size_t>(dragStepIdx_)]))
                {
                    stepPitchOffsets_[static_cast<size_t>(dragStepIdx_)] = static_cast<int8_t>(newPitch);
                    setApvtsParamValue("pitch_" + juce::String(dragStepIdx_), static_cast<float>(newPitch));
                    stepDragConsumed_ = true;
                    repaint();
                }
            }
            return;
        }

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
        activeSlider_     = SliderTarget::None;
        dragStartX_       = e.position.x;
        dragStepIdx_      = -1;
        stepDragMode_     = StepDragMode::Pending;
        stepDragConsumed_ = false;

        // C3: check if we hit a step LED first
        const int stepHit = hitTestStepRow(e.position);
        if (stepHit >= 0)
        {
            dragStepIdx_       = stepHit;
            dragStepStartY_    = e.position.y;
            dragStepStartPitch_ = static_cast<int>(stepPitchOffsets_[static_cast<size_t>(stepHit)]);

            // #1298: start long-press countdown for detail popover.
            // Uses the existing 15Hz sync timer; kLongPressTicks ticks ≈ 300ms.
            longPressStep_    = stepHit;
            longPressTriggered_ = false;
            longPressElapsed_ = 0;
            return; // control tracks handled only if step not hit
        }

        // No step hit — cancel any pending long-press
        longPressStep_    = -1;
        longPressElapsed_ = 0;

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
        hoveredStep_    = -1;
        repaint();
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const int prevPat  = hoveredPattern_;
        const int prevStep = hoveredStep_;
        hoveredPattern_ = -1;
        hoveredStep_    = -1;

        if (patternGridBounds_.contains(e.position))
        {
            int hit = hitTestPatternGrid(e.position);
            if (hit >= 0)
                hoveredPattern_ = hit;
        }

        // C3: step LED hover
        if (stepRowBounds_.contains(e.position))
            hoveredStep_ = hitTestStepRow(e.position);

        if (hoveredPattern_ != prevPat || hoveredStep_ != prevStep)
            repaint();
    }

private:
    //==========================================================================
    // Slider targets
    enum class SliderTarget { None, Swing, Gate, Humanize, Velocity };

    // C3: direction lock for step LED drag (resolved after first ~5px movement)
    enum class StepDragMode { Pending, Pitch, GateToggle };

    //==========================================================================
    // ── Layout constants ──
    static constexpr float kHandleH    = 8.0f;
    static constexpr float kSectionGap = 10.0f;
    static constexpr float kPadding    = 12.0f;

    // #1298: long-press ticks at 15Hz ≈ 300ms (5 ticks × 67ms each)
    static constexpr int kLongPressTicks = 5;

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
    /** Paint the 16-step LED row with C3 gate overrides, pitch arrows, and hover.
        Returns height consumed. */
    float paintStepRow(juce::Graphics& g, float x, float y, float areaW)
    {
        static const juce::Font labelFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(8.0f));
        static const juce::Font arrowFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(8.5f));

        const float ledH      = 36.0f;  // C3: taller to accommodate pitch arrow glyph
        const float gap       = 3.0f;
        const float ledW      = (areaW - 15.0f * gap) / 16.0f;
        const int   stepCount = currentStepCount_;
        const int   curStep   = currentStep_.load(std::memory_order_relaxed);
        const bool  playing   = isPlaying_.load(std::memory_order_relaxed);
        const bool  enabled   = currentEnabled_;
        const int   patIdx    = currentPattern_;
        const int   famIdx    = juce::jlimit(0, kNumFamilies - 1, patIdx / kPatternsPerFamily);
        const juce::Colour famCol = juce::Colour(kFamilyColors[famIdx]);

        // Determine whether any gate override is active (any bit set in the override array).
        // If so, the overrides act as an explicit mask for all steps.
        bool anyGateOverride = false;
        for (int i = 0; i < 16; ++i)
            if (stepGateOverrides_[i]) { anyGateOverride = true; break; }

        // Row header
        g.setFont(labelFont);
        g.setColour(juce::Colour(0xFF9E9B97).withAlpha(0.40f));
        g.drawText("STEPS", juce::Rectangle<float>(x, y - 14.0f, 40.0f, 12.0f).toNearestInt(),
                   juce::Justification::centredLeft, false);

        // C3/C4: hint label (right-aligned)
        g.setColour(juce::Colour(0xFF5E6878));
        g.drawText("drag=pitch  tap=gate  hold=detail",
                   juce::Rectangle<float>(x, y - 14.0f, areaW, 12.0f).toNearestInt(),
                   juce::Justification::centredRight, false);

        for (int i = 0; i < 16; ++i)
        {
            const float lx       = x + static_cast<float>(i) * (ledW + gap);
            const bool  inRange  = (i < stepCount);
            const bool  isCursor = (playing && enabled && i == curStep && inRange);
            const bool  isHovered = (i == hoveredStep_ && inRange);

            // C3: gate state resolves as follows:
            //   - If any gate override active: use stepGateOverrides_[i] as the gate state.
            //   - Otherwise: use the algorithmic stepGates_ mirror (C1/C2 behaviour).
            const bool hasGate = inRange && (
                anyGateOverride ? stepGateOverrides_[i]
                                : (stepGates_[static_cast<size_t>(i)] > 0.0f));

            // C3: pitch offset for this step
            const int pitchOffset = static_cast<int>(stepPitchOffsets_[static_cast<size_t>(i)]);

            // LED rectangle
            juce::Rectangle<float> ledRect(lx, y, ledW, ledH);

            juce::Colour ledBg, ledBorder;
            if (isCursor)
            {
                // Playhead: bright family colour
                ledBg     = famCol.withAlpha(0.90f);
                ledBorder = famCol;
            }
            else if (isHovered)
            {
                // Hover: slightly brightened version of gate state
                ledBg     = hasGate ? famCol.withAlpha(0.35f)
                                    : juce::Colour(0xFF252A3A);
                ledBorder = famCol.withAlpha(0.55f);
            }
            else if (hasGate && inRange)
            {
                // Active step with gate: coloured.
                // C3: if step has a gate override, use a slightly different alpha to
                // distinguish override-on from algorithmic-on (slightly brighter border).
                const float gateFactor = anyGateOverride ? 1.0f
                    : juce::jlimit(0.2f, 1.0f, stepGates_[static_cast<size_t>(i)]);
                ledBg     = famCol.withAlpha(0.20f + gateFactor * 0.25f);
                ledBorder = anyGateOverride ? famCol.withAlpha(0.75f)
                                            : famCol.withAlpha(0.35f + gateFactor * 0.30f);
            }
            else if (inRange)
            {
                // In range but rest (gate == 0).
                // C3: if this step is explicitly muted via override, use a darker background.
                ledBg     = anyGateOverride ? juce::Colour(0xFF0D1018)
                                           : juce::Colour(0xFF1A1F2E);
                ledBorder = anyGateOverride ? juce::Colour(0xFF9E9B97).withAlpha(0.06f)
                                           : juce::Colour(0xFF9E9B97).withAlpha(0.12f);
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

            // C3: pitch arrow glyph — drawn at centre-top of the LED when pitch != 0.
            // Arrow points up for positive offset, down for negative.
            // Intensity scales with |offset| / 12.
            if (inRange && pitchOffset != 0)
            {
                const float arrowAlpha = 0.45f + 0.55f * (std::abs(pitchOffset) / 12.0f);
                const juce::Colour arrowCol = isCursor
                    ? juce::Colour(0xFF0E111A)
                    : famCol.withAlpha(arrowAlpha);
                g.setFont(arrowFont);
                g.setColour(arrowCol);
                // Unicode arrows: up = 0x25B2 (▲), down = 0x25BC (▼)
                const juce::String arrow = (pitchOffset > 0) ? juce::String::charToString(0x25B2)
                                                              : juce::String::charToString(0x25BC);
                g.drawText(arrow,
                           juce::Rectangle<float>(lx, y + 3.0f, ledW, 10.0f).toNearestInt(),
                           juce::Justification::centred, false);

                // Small semitone number below the arrow (only if |offset| > 0)
                static const juce::Font pitchNumFont(juce::FontOptions{}
                    .withName(juce::Font::getDefaultSansSerifFontName())
                    .withHeight(7.0f));
                g.setFont(pitchNumFont);
                g.setColour(arrowCol.withAlpha(arrowAlpha * 0.80f));
                const juce::String pitchStr = (pitchOffset > 0 ? "+" : "") + juce::String(pitchOffset);
                g.drawText(pitchStr,
                           juce::Rectangle<float>(lx, y + 13.0f, ledW, 9.0f).toNearestInt(),
                           juce::Justification::centred, false);
            }

            // #1298: small dot indicator on steps that have vel or glen override set.
            if (inRange && (stepVelOverrides_[i] > 0.0f || stepGlenOverrides_[i] > 0.0f))
            {
                const juce::Colour dotCol = isCursor
                    ? juce::Colour(0xFF0E111A).withAlpha(0.70f)
                    : juce::Colour(0xFFE9C46A).withAlpha(0.65f); // XO Gold dot = "detail set"
                g.setColour(dotCol);
                g.fillEllipse(lx + ledW * 0.5f - 2.0f, y + ledH - 7.0f, 4.0f, 4.0f);
            }

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

        // C3: section height increased to ledH + 14 (header 14px above + no deferred annotation)
        return ledH + 14.0f;
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

    // C3: returns step index (0-15) if pos is inside the step row area, else -1.
    // Uses the stepRowBounds_ rect + per-step LED geometry to do per-cell hit testing.
    int hitTestStepRow(const juce::Point<float>& pos) const
    {
        if (!stepRowBounds_.contains(pos))
            return -1;

        const float areaW  = stepRowBounds_.getWidth();
        const float gap    = 3.0f;
        const float ledW   = (areaW - 15.0f * gap) / 16.0f;
        const float rowX   = stepRowBounds_.getX();

        for (int i = 0; i < 16; ++i)
        {
            const float lx = rowX + static_cast<float>(i) * (ledW + gap);
            const juce::Rectangle<float> cell(lx, stepRowBounds_.getY(), ledW, stepRowBounds_.getHeight());
            if (cell.contains(pos))
                return i;
        }
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
            // wire(#orphan-sweep item 8): slot{n}_seq_swing now exists in APVTS
            // (added to PerEnginePatternSequencer::addParameters() in this PR).
            // Removed the "humanize" workaround — swing is now its own real param.
            paramSuffix = "swing";
            break;
        case SliderTarget::Gate:
            trackRect  = &gateTrack_;
            valuePtr   = &currentGate_;
            // wire(#orphan-sweep item 8): slot{n}_seq_gateLen now exists in APVTS.
            paramSuffix = "gateLen";
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

        // C3: sync per-step gate overrides and pitch offsets
        // #1298: also sync per-step velocity and gate-length overrides
        for (int i = 0; i < 16; ++i)
        {
            stepGateOverrides_[i] = readParamBool("gate_" + juce::String(i));
            const int pitch = juce::jlimit(-12, 12, readParamInt("pitch_" + juce::String(i), 0));
            stepPitchOffsets_[i] = static_cast<int8_t>(pitch);

            stepVelOverrides_[i]  = juce::jlimit(0.0f, 1.0f,
                                        readParamFloat("svel_" + juce::String(i), 0.0f));
            stepGlenOverrides_[i] = juce::jlimit(0.0f, 1.5f,
                                        readParamFloat("glen_" + juce::String(i), 0.0f));
        }
    }

    //==========================================================================
    void timerCallback() override
    {
        // #1298: long-press detection — if we're in a pending long-press state,
        // check whether 300ms has elapsed.  longPressElapsed_ counts 15Hz ticks
        // (each tick = ~67ms; 5 ticks ≈ 333ms).
        if (longPressStep_ >= 0 && !longPressTriggered_)
        {
            ++longPressElapsed_;
            if (longPressElapsed_ >= kLongPressTicks)
            {
                longPressTriggered_ = true;
                openStepDetailPopup(longPressStep_);
            }
        }

        syncFromApvts();
        if (isShowing())
            repaint();
    }

    // #1298: open the per-step detail CallOutBox for the given step index.
    void openStepDetailPopup(int stepIdx)
    {
        if (stepIdx < 0 || stepIdx >= 16) return;

        const float vel   = readParamFloat("svel_"  + juce::String(stepIdx), 0.0f);
        const float glen  = readParamFloat("glen_"  + juce::String(stepIdx), 0.0f);
        const int   pitch = juce::jlimit(-12, 12,
                                readParamInt("pitch_" + juce::String(stepIdx), 0));

        auto* content = new StepDetailContent(
            stepIdx, vel, glen, pitch,
            "Step " + juce::String(stepIdx + 1));

        const juce::String pref = prefix_; // capture for lambda
        juce::AudioProcessorValueTreeState& apvts = apvts_;

        content->onParamChange = [&apvts, pref](const juce::String& suffix, float value)
        {
            // Suffix comes from StepDetailContent without the slot prefix, e.g. "svel_3".
            // We need to call setValueNotifyingHost via the APVTS parameter.
            if (auto* p = apvts.getParameter(pref + suffix))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(p->convertTo0to1(value));
                p->endChangeGesture();
            }
        };

        // Compute the step LED's local rect for CallOutBox anchor.
        // stepRowBounds_ is in component-local coordinates (rebuilt each paint call).
        const float areaW = stepRowBounds_.getWidth();
        const float gap   = 3.0f;
        const float ledW  = (areaW - 15.0f * gap) / 16.0f;
        const float lx    = stepRowBounds_.getX() + static_cast<float>(stepIdx) * (ledW + gap);
        const juce::Rectangle<int> ledLocalRect(
            static_cast<int>(lx),
            static_cast<int>(stepRowBounds_.getY()),
            static_cast<int>(ledW),
            static_cast<int>(stepRowBounds_.getHeight()));

        // Launch from the top-level editor so the CallOutBox renders above all
        // OceanView overlays (DrawerOverlay, PlaySurfaceOverlay, DimOverlay).
        // CallOutBox::launchAsynchronously requires areaToPointTo in the parent's
        // local coordinate space — convert from SeqBreakout-local via getLocalArea.
        // F-005 / #1396: fixes step-edit popups occluded by Ocean overlays.
        auto* topLevel = getTopLevelComponent();
        const juce::Rectangle<int> areaInTopLevel =
            topLevel->getLocalArea(this, ledLocalRect);
        juce::CallOutBox::launchAsynchronously(
            std::unique_ptr<juce::Component>(content),
            areaInTopLevel,
            topLevel);
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

    // Drag state (controls row sliders)
    SliderTarget activeSlider_ = SliderTarget::None;
    float        dragStartX_   = 0.0f;
    float        dragStartVal_ = 0.0f;

    // C3: Per-step gate override + pitch offset mirrors (message-thread cache; synced from APVTS)
    // stepGateOverrides_[i] == true means the step has an explicit gate override.
    // The displayed gate state = stepGateOverrides_[i] OR (algorithmic gate && no override bitmap active).
    std::array<bool, 16>  stepGateOverrides_ {};
    std::array<int8_t, 16> stepPitchOffsets_  {};  // ±12 semitones

    // #1298: Per-step velocity and gate-length override mirrors (message-thread cache).
    // 0.0 = inherit sentinel for both.
    std::array<float, 16> stepVelOverrides_  {};   // 0.0 = inherit from baseVel
    std::array<float, 16> stepGlenOverrides_ {};   // 0.0 = inherit from global gate

    // C3: Step LED drag state for Maschine-style vertical pitch editing
    int          dragStepIdx_      = -1;        // which step is being dragged (-1 = none)
    float        dragStepStartY_   = 0.0f;      // mouseDown Y position in component coords
    int          dragStepStartPitch_ = 0;        // pitch offset at drag start
    StepDragMode stepDragMode_     = StepDragMode::Pending;
    bool         stepDragConsumed_ = false;     // true once direction locked; suppresses tap action

    // #1298: Long-press state for per-step detail popover.
    // longPressStep_ = step index awaiting long-press (-1 = none).
    // longPressElapsed_ counts 15Hz timer ticks; fires popup at kLongPressTicks.
    int  longPressStep_      = -1;
    int  longPressElapsed_   = 0;
    bool longPressTriggered_ = false;

    // Hover state
    int hoveredPattern_ = -1;
    int hoveredStep_    = -1;   // C3: step LED hover for visual feedback

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SeqBreakoutComponent)
};

} // namespace xoceanus
