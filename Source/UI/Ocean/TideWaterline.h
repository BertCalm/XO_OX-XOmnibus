// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// TideWaterline.h — Submarine-style step sequencer strip for the XOceanus Ocean View.
//
// The TideWaterline sits at the boundary between the ocean viewport above and the
// dashboard below. It has two display states:
//
//   Collapsed  — 6 px thin teal gradient line acting as a visual separator.
//   Expanded   — 96 px panel: a controls row (top 32 px) + step grid (bottom ~64 px).
//
// The controls row provides pill buttons for Pattern, ClockDiv, Steps, Target1, Target2,
// and ENV, plus sliders for Depth, Smooth, and EnvAmount. All controls map directly to
// APVTS parameters via beginChangeGesture / setValueNotifyingHost / endChangeGesture.
//
// The step grid shows up to 16 steps with per-step velocity bars, center dots, a
// scrolling playhead, and beat markers. Per-step state (active, velocity, gate, rootNote)
// is stored locally in TideWaterline and does NOT map to APVTS — it is purely visual and
// available for higher-level preset serialization as desired.
//
// Timer fires at 30 Hz for playhead animation. The pattern preview miniature is rendered
// inline in the controls row.
//
// File is entirely self-contained (header-only inline implementation) following the
// XOceanus convention for UI components.

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/MasterFXSequencer.h"
#include "../GalleryColors.h"
#include "../Tokens.h"
#include <functional>
#include <cmath>
#include <array>
#include <cstdint>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

//==============================================================================
/**
    TideWaterline

    Submarine-style step sequencer strip. See file header for full documentation.
*/
class TideWaterline : public juce::Component,
                      private juce::Timer
{
public:
    //==========================================================================
    // Label tables — static constexpr so they live in the header with no ODR issue.
    // Pattern names tag each option with its functional category (#1178):
    // Vel = velocity-shape preset (which steps fire is unchanged)
    // Rhy = rhythm generator (which steps fire)
    // Gate = probabilistic gate generator
    // Without this tag, users assume "Ramp ▲" produces ascending RHYTHMS
    // (it only changes velocity over already-firing steps).
    static constexpr const char* kPatternNames[] = {
        "Vel: Pulse",   "Vel: Ramp \xe2\x96\xb2", "Vel: Ramp \xe2\x96\xbc", "Vel: Triangle",
        "Rhy: Eucl 3",  "Rhy: Eucl 5",            "Gate: Random",           "Gate: Scatter"
    };
    static constexpr const char* kClockLabels[] = {
        "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
        "\xe2\x85\x9b.", ("\xc2\xbc" "T")
    };
    static constexpr const char* kTargetNames[] = {
        "None", "SatDrive", "Delay FB", "Delay Mix", "Reverb Mix",
        "Mod Depth", "Mod Rate", "Comp Mix", "Spec Tilt", "Trans Atk",
        "Doppler", "Gran Smear", "Exciter", "Stereo W", "Psycho W",
        "Vibe Knob", "Osmosis", "Oneiric"
    };

    // Height constants
    static constexpr int kCollapsedHeight  = 6;
    static constexpr int kExpandedHeight   = 96;
    static constexpr int kControlsRowH     = 32;
    static constexpr int kMaxSteps         = 16;

    //==========================================================================
    // Per-step data — purely local, not in APVTS.
    struct Step
    {
        bool  active   = false;
        float velocity = 0.75f;
        int   rootNote = -1;    // -1 = none
        float gate     = 0.75f;
    };

    //==========================================================================
    // ── Sequence layer serialization (#1179) ──────────────────────────────────
    //
    // steps_ is NOT stored in APVTS.  To preserve user-edited patterns across
    // DAW sessions independently of preset loads, we expose a ValueTree round-
    // trip.  The processor includes this tree in getStateInformation() /
    // setStateInformation() via the onGetTideWaterlineState callback.
    //
    // Invariant: fromValueTree() does NOT call syncFromApvts() or applyPattern().
    // This guarantees session restore never re-applies an algorithmic template
    // on top of user edits — a preset load and a session restore are cleanly
    // separated code paths.

    /// Serialize current per-step data to a juce::ValueTree ("TideWaterlineSteps").
    juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree("TideWaterlineSteps");
        tree.setProperty("stepCount", currentSteps_, nullptr);
        tree.setProperty("expanded",  expanded_ ? 1 : 0, nullptr);  // F2-011
        for (int i = 0; i < kMaxSteps; ++i)
        {
            juce::ValueTree step("Step");
            step.setProperty("active",   steps_[i].active ? 1 : 0, nullptr);
            step.setProperty("velocity", static_cast<double>(steps_[i].velocity), nullptr);
            step.setProperty("gate",     static_cast<double>(steps_[i].gate),     nullptr);
            step.setProperty("rootNote", steps_[i].rootNote,                      nullptr);
            tree.addChild(step, -1, nullptr);
        }
        return tree;
    }

    /// Restore per-step data from a ValueTree produced by toValueTree().
    /// Does NOT call syncFromApvts() or applyPattern() — pattern is verbatim.
    /// Returns false without modifying state if the tree type does not match.
    bool fromValueTree(const juce::ValueTree& tree)
    {
        if (!tree.isValid() || tree.getType().toString() != "TideWaterlineSteps")
            return false;

        if (tree.hasProperty("stepCount"))
            currentSteps_ = juce::jlimit(1, kMaxSteps, static_cast<int>(tree.getProperty("stepCount")));

        // F2-011: Restore expanded state.
        if (tree.hasProperty("expanded"))
            setExpanded(static_cast<int>(tree.getProperty("expanded")) != 0);

        int childIdx = 0;
        for (auto child : tree)
        {
            if (childIdx >= kMaxSteps)
                break;
            if (child.getType().toString() == "Step")
            {
                steps_[childIdx].active   = (static_cast<int>(child.getProperty("active",   0)) != 0);
                steps_[childIdx].velocity = juce::jlimit(0.0f, 1.0f,
                    static_cast<float>(static_cast<double>(child.getProperty("velocity", 0.75))));
                steps_[childIdx].gate     = juce::jlimit(0.05f, 1.0f,
                    static_cast<float>(static_cast<double>(child.getProperty("gate",     0.75))));
                steps_[childIdx].rootNote = static_cast<int>(child.getProperty("rootNote", -1));
                ++childIdx;
            }
        }

        rebuildRootNoteLabels();
        repaint();
        return true;
    }

    //==========================================================================
    explicit TideWaterline(juce::AudioProcessorValueTreeState& apvts,
                           const MasterFXSequencer&            sequencer)
        : apvts_    (apvts)
        , sequencer_(sequencer)
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, true);

        // Seed steps from current APVTS pattern on construction.
        syncFromApvts();
        applyPattern(currentPattern_);
        rebuildRootNoteLabels();

        // 30 Hz timer for playhead animation.
        startTimerHz(30);

        A11y::setup(*this,
                    "Tide Waterline Sequencer",
                    "Step sequencer strip — click to expand or collapse, "
                    "interact with steps to edit velocity and gate",
                    /*wantsKeyFocus=*/true);
    }

    ~TideWaterline() override
    {
        stopTimer();
    }

    //==========================================================================
    void setExpanded(bool expanded)
    {
        if (expanded_ == expanded)
            return;
        expanded_ = expanded;
        if (onHeightChanged)
            onHeightChanged();
        repaint();
    }

    bool isExpanded() const noexcept { return expanded_; }

    int getDesiredHeight() const noexcept
    {
        return expanded_ ? kExpandedHeight : kCollapsedHeight;
    }

    /// Callback invoked whenever setExpanded() changes height.
    std::function<void()> onHeightChanged;

private:
    //==========================================================================
    // Sub-component geometry (computed in resized(), used in paint/mouse).
    struct PillRegion
    {
        juce::Rectangle<float> bounds;
        int                    type; // 0=pattern 1=clockdiv 2=steps 3=target1 4=target2 5=env
    };
    struct SliderRegion
    {
        juce::Rectangle<float> track;  // full track area
        float                  value;  // 0–1
        int                    type;   // 0=depth 1=smooth 2=envamt
    };

    //==========================================================================
    // paint() entry
    void paint(juce::Graphics& g) override
    {
        const auto w = static_cast<float>(getWidth());
        const auto h = static_cast<float>(getHeight());

        if (expanded_)
            paintExpanded(g, w, h);
        else
            paintCollapsed(g, w, h);
    }

    //--------------------------------------------------------------------------
    void paintCollapsed(juce::Graphics& g, float w, float h) const
    {
        // Background: teal gradient from top to bottom.
        juce::ColourGradient bg(
            juce::XO::Tokens::Color::accent().withAlpha(0.04f),  0.0f, 0.0f,
            juce::XO::Tokens::Color::accent().withAlpha(0.02f),  0.0f, h,   false);
        bg.addColour(0.60, juce::XO::Tokens::Color::accent().withAlpha(0.12f));
        g.setGradientFill(bg);
        g.fillRect(0.0f, 0.0f, w, h);

        // Bottom border 1 px teal.
        g.setColour(juce::XO::Tokens::Color::accent().withAlpha(0.18f));
        g.fillRect(0.0f, h - 1.0f, w, 1.0f);

        // Glow under bottom border (box-shadow emulation via soft stripe).
        juce::ColourGradient glow(
            juce::XO::Tokens::Color::accent().withAlpha(0.10f), 0.0f, h - 1.0f,
            juce::Colours::transparentBlack,             0.0f, h + 7.0f, false);
        g.setGradientFill(glow);
        g.fillRect(0.0f, h - 1.0f, w, 8.0f);
    }

    //--------------------------------------------------------------------------
    void paintExpanded(juce::Graphics& g, float w, float h)
    {
        // Background
        g.setColour(juce::Colour(0xFF111820));
        g.fillRect(0.0f, 0.0f, w, h);

        // Top border 1 px
        g.setColour(juce::XO::Tokens::Color::accent().withAlpha(0.18f));
        g.fillRect(0.0f, 0.0f, w, 1.0f);

        // Bottom border 1 px
        g.setColour(juce::XO::Tokens::Color::accent().withAlpha(0.12f));
        g.fillRect(0.0f, h - 1.0f, w, 1.0f);

        // Controls row bottom border
        g.setColour(juce::XO::Tokens::Color::accent().withAlpha(0.08f));
        g.fillRect(0.0f, static_cast<float>(kControlsRowH) - 1.0f, w, 1.0f);

        paintControlsRow(g, w);
        paintStepsRow(g, w, h);
    }

    //--------------------------------------------------------------------------
    void paintControlsRow(juce::Graphics& g, float w)
    {
        const float rowH  = static_cast<float>(kControlsRowH);
        const float midY  = rowH * 0.5f;

        // We rebuild pill/slider regions each paint call.  This is cheap and
        // keeps layout in one place (resized() also calls layoutControls() but
        // paint may run before the first resized if bounds are set externally).
        layoutControls(w);

        static const juce::Font pillFont = XO::Tokens::Type::heading(XO::Tokens::Type::HeadingSmall); // D3;
        static const juce::Font labelFont = XO::Tokens::Type::body(XO::Tokens::Type::BodySmall); // D3;

        // ── Pill buttons ──
        for (const auto& pill : pillRegions_)
        {
            bool isActive = pillIsActive(pill.type);
            bool isHover  = (hoveredPill_ == pill.type && pill.type >= 0);

            juce::Colour textCol, borderCol;
            if (isActive)
            {
                textCol   = juce::Colour(127, 219, 202).withAlpha(0.80f);
                borderCol = juce::Colour(127, 219, 202).withAlpha(0.25f);
            }
            else if (isHover)
            {
                textCol   = juce::Colour(200, 204, 216).withAlpha(0.80f);
                borderCol = juce::Colour(200, 204, 216).withAlpha(0.16f);
            }
            else
            {
                textCol   = juce::Colour(200, 204, 216).withAlpha(0.50f);
                borderCol = juce::Colour(200, 204, 216).withAlpha(0.08f);
            }

            g.setColour(borderCol);
            g.drawRoundedRectangle(pill.bounds, 4.0f, 1.0f);

            g.setFont(pillFont);
            g.setColour(textCol);
            g.drawText(pillLabel(pill.type), pill.bounds.toNearestInt(),
                       juce::Justification::centred, false);
        }

        // ── Pattern preview canvas (60 × 16 px) ──
        if (patternPreviewBounds_.getWidth() > 0.0f)
        {
            const auto& ppb = patternPreviewBounds_;
            const int   stepCount = currentSteps_;
            const float barW = ppb.getWidth() / static_cast<float>(kMaxSteps);
            const float maxH = ppb.getHeight();

            for (int i = 0; i < kMaxSteps; ++i)
            {
                if (i >= stepCount)
                    continue;
                if (!steps_[i].active)
                    continue;
                const float bx = ppb.getX() + i * barW;
                const float bh = steps_[i].velocity * maxH;
                const float by = ppb.getBottom() - bh;
                g.setColour(juce::XO::Tokens::Color::accent().withAlpha(0.50f));
                g.fillRect(bx + 1.0f, by, barW - 1.0f, bh);
            }
            // Outline
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.07f));
            g.drawRect(ppb, 1.0f);
        }

        // ── Labels + sliders (Depth, Smooth, Amt) ──
        paintLabeledSlider(g, labelFont, midY,
                           depthLabelBounds_, depthSliderRegion_,
                           "DEPTH");
        paintLabeledSlider(g, labelFont, midY,
                           smoothLabelBounds_, smoothSliderRegion_,
                           "SMOOTH");
        if (envFollowEnabled_)
            paintLabeledSlider(g, labelFont, midY,
                               envAmtLabelBounds_, envAmtSliderRegion_,
                               "AMT");

        // ── Separators ──
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.07f));
        for (float sx : separatorXs_)
            g.fillRect(sx, midY - 8.0f, 1.0f, 16.0f);

        // ── ALGO badge ──
        if (algoBadgeBounds_.getWidth() > 0.0f)
        {
            static const juce::Font badgeFont = XO::Tokens::Type::heading(XO::Tokens::Type::HeadingSmall); // D3;
            const float alpha = sequencer_.isEnabled() ? 0.7f : 0.3f;
            g.setColour(juce::Colour(127, 219, 202).withAlpha(alpha * 0.2f));
            g.drawRoundedRectangle(algoBadgeBounds_, 3.0f, 1.0f);
            g.setFont(badgeFont);
            g.setColour(juce::Colour(127, 219, 202).withAlpha(alpha));
            g.drawText("ALGO", algoBadgeBounds_.toNearestInt(),
                       juce::Justification::centred, false);
        }

        // ── ENV breathe indicator ("~") ──
        if (envFollowEnabled_)
        {
            // Breathing alpha: 2s ease-in-out sine cycle.
            const float phase = std::sin(breathePhase_ * static_cast<float>(M_PI));
            const float alpha = 0.35f + phase * 0.35f;

            static const juce::Font breatheFont = XO::Tokens::Type::body(XO::Tokens::Type::BodyLarge); // D3;
            g.setFont(breatheFont);
            g.setColour(juce::Colour(127, 219, 202).withAlpha(alpha));
            g.drawText("~", breatheBounds_.toNearestInt(),
                       juce::Justification::centred, false);
        }
    }

    //--------------------------------------------------------------------------
    void paintLabeledSlider(juce::Graphics&          g,
                            const juce::Font&         labelFont,
                            float                     /*midY*/,
                            const juce::Rectangle<float>& labelBounds,
                            const SliderRegion&       sr,
                            const char*               labelText) const
    {
        if (labelBounds.isEmpty() || sr.track.isEmpty())
            return;

        // Label
        g.setFont(labelFont);
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.30f));
        g.drawText(labelText, labelBounds.toNearestInt(),
                   juce::Justification::centred, false);

        // Track
        const float trackY  = sr.track.getCentreY();
        const float trackX1 = sr.track.getX();
        const float trackX2 = sr.track.getRight();
        g.setColour(juce::Colour(200, 204, 216).withAlpha(0.10f));
        g.fillRect(trackX1, trackY - 1.5f, trackX2 - trackX1, 3.0f);

        // Thumb
        const float thumbX = trackX1 + sr.value * (trackX2 - trackX1);
        g.setColour(juce::Colour(127, 219, 202).withAlpha(0.80f));
        g.fillEllipse(thumbX - 5.0f, trackY - 5.0f, 10.0f, 10.0f);
        g.setColour(juce::Colour(127, 219, 202).withAlpha(0.50f));
        g.drawEllipse(thumbX - 5.0f, trackY - 5.0f, 10.0f, 10.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    void paintStepsRow(juce::Graphics& g, float w, float h)
    {
        const float rowY   = static_cast<float>(kControlsRowH);
        const float rowH   = h - rowY - 1.0f; // -1 for bottom border
        const int   stepCount = currentSteps_;
        const float stepW  = w / static_cast<float>(kMaxSteps);
        const int   playhead = sequencer_.getCurrentStep();

        for (int i = 0; i < kMaxSteps; ++i)
        {
            const float sx = i * stepW;
            const bool  isBeyond = (i >= stepCount);
            const bool  isActive = (!isBeyond && steps_[i].active);
            const bool  isPlayhead = (i == playhead && sequencer_.isEnabled());
            const bool  isBeat  = (i % 4 == 0);

            // Background fill
            {
                juce::Colour bg = juce::Colour(0xFF1A2332);
                if (isBeyond)
                    bg = bg.withAlpha(0.25f);
                if (isActive && i == playhead && sequencer_.isEnabled())
                    bg = juce::Colour(127, 219, 202).withAlpha(0.08f);
                g.setColour(isBeyond ? juce::Colour(0xFF1A2332).withAlpha(0.25f)
                                     : (isActive && isPlayhead)
                                           ? juce::Colour(127, 219, 202).withAlpha(0.08f)
                                           : juce::Colour(0xFF1A2332));
                g.fillRect(sx, rowY, stepW - 1.0f, rowH);
            }

            // Velocity fill bar (from bottom)
            if (isActive && !isBeyond)
            {
                const float velH = steps_[i].velocity * rowH;
                const float velY = rowY + rowH - velH;
                g.setColour(isPlayhead
                    ? juce::Colour(127, 219, 202).withAlpha(0.60f)
                    : juce::Colour(127, 219, 202).withAlpha(0.60f));
                g.fillRect(sx, velY, stepW - 1.0f, velH);
            }

            // Center dot
            {
                const float dotCX = sx + stepW * 0.5f;
                const float dotCY = rowY + rowH * 0.5f;
                const float dotR  = 3.0f;
                if (isPlayhead && sequencer_.isEnabled())
                {
                    g.setColour(juce::Colour(127, 219, 202).withAlpha(0.70f));
                    // Glow emulation: draw a slightly larger dim circle first.
                    g.fillEllipse(dotCX - dotR * 2.0f, dotCY - dotR * 2.0f,
                                  dotR * 4.0f, dotR * 4.0f);
                    g.setColour(juce::Colour(127, 219, 202).withAlpha(0.90f));
                }
                else
                {
                    g.setColour(juce::Colour(127, 219, 202)
                                    .withAlpha(isBeyond ? 0.04f : 0.18f));
                }
                g.fillEllipse(dotCX - dotR, dotCY - dotR, dotR * 2.0f, dotR * 2.0f);
            }

            // Root note label (shown when rootNote >= 0)
            if (!isBeyond && steps_[i].rootNote >= 0)
            {
                static const juce::Font noteFont = XO::Tokens::Type::heading(XO::Tokens::Type::HeadingSmall); // D3;
                g.setFont(noteFont);
                g.setColour(juce::Colour(127, 219, 202).withAlpha(0.70f));
                g.drawText(cachedRootNoteLabels_[static_cast<size_t>(i)],
                           juce::Rectangle<int>(static_cast<int>(sx),
                                                static_cast<int>(rowY) + 2,
                                                static_cast<int>(stepW),
                                                10),
                           juce::Justification::centred, false);
                // Custom root indicator: 2 px top border on step
                g.setColour(juce::Colour(127, 219, 202).withAlpha(0.60f));
                g.fillRect(sx, rowY, stepW - 1.0f, 2.0f);
            }

            // Right border (step divider or beat marker)
            const float borderAlpha = isBeat ? 0.25f : 0.0f;
            if (isBeat && !isBeyond)
            {
                g.setColour(juce::XO::Tokens::Color::accent().withAlpha(borderAlpha));
                g.fillRect(sx + stepW - 1.0f, rowY, 1.0f, rowH);
            }
            else
            {
                g.setColour(juce::Colour(10, 12, 18).withAlpha(0.80f));
                g.fillRect(sx + stepW - 1.0f, rowY, 1.0f, rowH);
            }
        }

        // Playhead bar (2 px wide, full step-row height, teal glow)
        if (sequencer_.isEnabled())
        {
            const float phX = static_cast<float>(playhead) * stepW
                              + stepW * 0.5f - 1.0f;
            // Soft glow: wider dim stripe behind the bright bar
            g.setColour(juce::Colour(127, 219, 202).withAlpha(0.20f));
            g.fillRect(phX - 2.0f, rowY, 6.0f, rowH);
            // Bright bar
            g.setColour(juce::Colour(127, 219, 202).withAlpha(0.90f));
            g.fillRect(phX, rowY, 2.0f, rowH);
        }
    }

    //==========================================================================
    void resized() override
    {
        if (expanded_)
            layoutControls(static_cast<float>(getWidth()));
    }

    //--------------------------------------------------------------------------
    /// Layout all control sub-regions from left to right in the controls row.
    /// Called from both resized() and paint() so it is always up-to-date.
    void layoutControls(float /*w*/)
    {
        pillRegions_.clear();
        separatorXs_.clear();

        const float rowH  = static_cast<float>(kControlsRowH);
        const float midY  = rowH * 0.5f;
        const float padX  = 8.0f;
        const float gap   = 6.0f;
        float       curX  = padX;

        // Helper: add a pill region.
        auto addPill = [&](int type, float pillW) {
            const float pillH = 18.0f;
            const float pillY = midY - pillH * 0.5f;
            juce::Rectangle<float> r(curX, pillY, pillW, pillH);
            pillRegions_.push_back({ r, type });
            curX += pillW + gap;
        };

        // Helper: add a vertical separator.
        auto addSep = [&]() {
            separatorXs_.push_back(curX);
            curX += 1.0f + gap;
        };

        // ── Pattern pill ──
        addPill(0, 52.0f);
        addSep();

        // ── Clock div pill ──
        addPill(1, 34.0f);

        // ── Steps pill ──
        addPill(2, 26.0f);
        addSep();

        // ── Target1 pill ──
        addPill(3, 60.0f);

        // ── Target2 pill ──
        addPill(4, 60.0f);
        addSep();

        // ── ENV pill ──
        addPill(5, 30.0f);

        // ── ENV breathe indicator ──
        breatheBounds_ = juce::Rectangle<float>(curX, midY - 8.0f, 14.0f, 16.0f);
        curX += envFollowEnabled_ ? 18.0f : gap;
        addSep();

        // ── ALGO badge ──
        algoBadgeBounds_ = juce::Rectangle<float>(curX, midY - 8.0f, 32.0f, 16.0f);
        curX += 38.0f + gap;
        addSep();

        // ── Pattern preview (60 × 16) ──
        patternPreviewBounds_ = juce::Rectangle<float>(curX, midY - 8.0f, 60.0f, 16.0f);
        curX += 60.0f + gap;
        addSep();

        // ── Depth label + slider ──
        depthLabelBounds_ = juce::Rectangle<float>(curX, midY - 12.0f, 30.0f, 10.0f);
        curX += 30.0f + 2.0f;
        depthSliderRegion_.track = juce::Rectangle<float>(curX, midY - 2.0f, 50.0f, 4.0f);
        depthSliderRegion_.value = currentDepth_;
        depthSliderRegion_.type  = 0;
        curX += 50.0f + gap;
        addSep();

        // ── Smooth label + slider ──
        smoothLabelBounds_ = juce::Rectangle<float>(curX, midY - 12.0f, 36.0f, 10.0f);
        curX += 36.0f + 2.0f;
        smoothSliderRegion_.track = juce::Rectangle<float>(curX, midY - 2.0f, 50.0f, 4.0f);
        smoothSliderRegion_.value = currentSmooth_;
        smoothSliderRegion_.type  = 1;
        curX += 50.0f + gap;

        // ── EnvAmt label + slider (only when env follow enabled) ──
        if (envFollowEnabled_)
        {
            addSep();
            envAmtLabelBounds_ = juce::Rectangle<float>(curX, midY - 12.0f, 18.0f, 10.0f);
            curX += 18.0f + 2.0f;
            envAmtSliderRegion_.track = juce::Rectangle<float>(curX, midY - 2.0f, 50.0f, 4.0f);
            envAmtSliderRegion_.value = currentEnvAmt_;
            envAmtSliderRegion_.type  = 2;
        }
    }

    //==========================================================================
    void mouseDown(const juce::MouseEvent& e) override
    {
        // Collapsed: toggle expand on click.
        if (!expanded_)
        {
            setExpanded(true);
            return;
        }

        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        // Controls row hit testing (top kControlsRowH px).
        if (my < static_cast<float>(kControlsRowH))
        {
            handleControlsRowClick(mx, my);
            return;
        }

        // Steps row.
        const float stepW = static_cast<float>(getWidth()) / static_cast<float>(kMaxSteps);
        const int   stepIdx = static_cast<int>(mx / stepW);
        if (stepIdx >= 0 && stepIdx < kMaxSteps)
        {
            dragStepIdx_    = stepIdx;
            dragStartY_     = e.y;
            dragStartX_     = e.x;
            dragStartVel_   = steps_[stepIdx].velocity;
            dragStartGate_  = steps_[stepIdx].gate;
            isDragging_     = false;

            // Toggle active on click (only commit if we don't drag).
            mouseDownWasToggle_ = true;
        }
    }

    //--------------------------------------------------------------------------
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!expanded_ || dragStepIdx_ < 0)
            return;

        const int dy = e.y - dragStartY_;
        const int dx = e.x - dragStartX_;

        if (std::abs(dy) > 3 || std::abs(dx) > 3)
            isDragging_ = true;

        if (!isDragging_)
            return;

        mouseDownWasToggle_ = false;

        if (e.mods.isShiftDown())
        {
            // Shift+drag = gate length (horizontal).
            float newGate = dragStartGate_ + static_cast<float>(dx) / 100.0f;
            newGate = juce::jlimit(0.05f, 1.0f, newGate);
            steps_[dragStepIdx_].gate = newGate;
        }
        else
        {
            // Vertical drag = velocity. Drag up = increase.
            float newVel = dragStartVel_ - static_cast<float>(dy) / 80.0f;
            newVel = juce::jlimit(0.05f, 1.0f, newVel);
            steps_[dragStepIdx_].velocity = newVel;
            // Activate step if dragging velocity.
            steps_[dragStepIdx_].active = true;
        }

        repaint();
    }

    //--------------------------------------------------------------------------
    void mouseUp(const juce::MouseEvent& /*e*/) override
    {
        if (!expanded_)
            return;

        if (dragStepIdx_ >= 0 && mouseDownWasToggle_ && !isDragging_)
        {
            // Pure click: toggle active.
            steps_[dragStepIdx_].active = !steps_[dragStepIdx_].active;
            repaint();
        }

        dragStepIdx_        = -1;
        isDragging_         = false;
        mouseDownWasToggle_ = false;
    }

    //==========================================================================
    /// Handle a click inside the controls row (top 32 px of expanded state).
    void handleControlsRowClick(float mx, float my)
    {
        // Check sliders first (value drag, not cycle).
        if (hitTestSlider(depthSliderRegion_, mx, my))
        {
            beginSliderDrag(0, mx);
            return;
        }
        if (hitTestSlider(smoothSliderRegion_, mx, my))
        {
            beginSliderDrag(1, mx);
            return;
        }
        if (envFollowEnabled_ && hitTestSlider(envAmtSliderRegion_, mx, my))
        {
            beginSliderDrag(2, mx);
            return;
        }

        // ALGO badge — click to toggle master_seqEnabled.
        if (algoBadgeBounds_.contains(mx, my))
        {
            const bool nowEnabled = !sequencer_.isEnabled();
            if (auto* p = apvts_.getParameter("master_seqEnabled"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(nowEnabled ? 1.0f : 0.0f);
                p->endChangeGesture();
            }
            repaint();
            return;
        }

        // Check pills.
        for (const auto& pill : pillRegions_)
        {
            if (pill.bounds.contains(mx, my))
            {
                handlePillClick(pill.type);
                return;
            }
        }
    }

    //--------------------------------------------------------------------------
    bool hitTestSlider(const SliderRegion& sr, float mx, float my) const
    {
        if (sr.track.isEmpty())
            return false;
        // Expand hit area vertically.
        auto hit = sr.track.withHeight(sr.track.getHeight() + 16.0f)
                           .withY(sr.track.getY() - 8.0f);
        return hit.contains(mx, my);
    }

    //--------------------------------------------------------------------------
    void beginSliderDrag(int sliderType, float mx)
    {
        // Map mouse X to 0-1 value and update parameter.
        SliderRegion* sr = nullptr;
        if (sliderType == 0) sr = &depthSliderRegion_;
        else if (sliderType == 1) sr = &smoothSliderRegion_;
        else if (sliderType == 2) sr = &envAmtSliderRegion_;
        if (!sr || sr->track.isEmpty()) return;

        const float t = juce::jlimit(0.0f, 1.0f,
            (mx - sr->track.getX()) / sr->track.getWidth());

        activeSliderType_ = sliderType;
        setSliderValue(sliderType, t);
    }

    //--------------------------------------------------------------------------
    void setSliderValue(int sliderType, float t)
    {
        juce::String paramId;
        if (sliderType == 0) paramId = "master_seqDepth";
        else if (sliderType == 1) paramId = "master_seqSmooth";
        else paramId = "master_seqEnvAmount";

        if (auto* param = apvts_.getParameter(paramId))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(t);
            param->endChangeGesture();
        }

        // Mirror locally.
        if (sliderType == 0) { currentDepth_ = t; depthSliderRegion_.value = t; }
        else if (sliderType == 1) { currentSmooth_ = t; smoothSliderRegion_.value = t; }
        else { currentEnvAmt_ = t; envAmtSliderRegion_.value = t; }

        repaint();
    }

    //--------------------------------------------------------------------------
    void handlePillClick(int pillType)
    {
        switch (pillType)
        {
        case 0: // Pattern — cycle 0-7
        {
            currentPattern_ = (currentPattern_ + 1) % 8;
            if (auto* p = apvts_.getParameter("master_seqPattern"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(
                    p->convertTo0to1(static_cast<float>(currentPattern_)));
                p->endChangeGesture();
            }
            applyPattern(currentPattern_);
            break;
        }
        case 1: // ClockDiv — cycle 0-7
        {
            currentClockDiv_ = (currentClockDiv_ + 1) % 8;
            if (auto* p = apvts_.getParameter("master_seqRate"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(
                    p->convertTo0to1(static_cast<float>(currentClockDiv_)));
                p->endChangeGesture();
            }
            break;
        }
        case 2: // Steps — cycle through [1,2,4,8,12,16]
        {
            static constexpr int kStepCycles[] = { 1, 2, 4, 8, 12, 16 };
            static constexpr int kNumCycles = 6;
            // Find current index.
            int idx = 0;
            for (int i = 0; i < kNumCycles; ++i)
                if (kStepCycles[i] == currentSteps_) { idx = i; break; }
            idx = (idx + 1) % kNumCycles;
            currentSteps_ = kStepCycles[idx];
            if (auto* p = apvts_.getParameter("master_seqSteps"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(
                    p->convertTo0to1(static_cast<float>(currentSteps_)));
                p->endChangeGesture();
            }
            break;
        }
        case 3: // Target1 — cycle 0-17
        {
            currentTarget1_ = (currentTarget1_ + 1) % 18;
            if (auto* p = apvts_.getParameter("master_seqTarget1"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(
                    p->convertTo0to1(static_cast<float>(currentTarget1_)));
                p->endChangeGesture();
            }
            break;
        }
        case 4: // Target2 — cycle 0-17
        {
            currentTarget2_ = (currentTarget2_ + 1) % 18;
            if (auto* p = apvts_.getParameter("master_seqTarget2"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(
                    p->convertTo0to1(static_cast<float>(currentTarget2_)));
                p->endChangeGesture();
            }
            break;
        }
        case 5: // ENV — toggle
        {
            envFollowEnabled_ = !envFollowEnabled_;
            if (auto* p = apvts_.getParameter("master_seqEnvFollow"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(envFollowEnabled_ ? 1.0f : 0.0f);
                p->endChangeGesture();
            }
            // Re-layout to show/hide ENV amt slider.
            layoutControls(static_cast<float>(getWidth()));
            break;
        }
        default:
            break;
        }
        repaint();
    }

    //==========================================================================
    /// Returns true if the given pill type should render in the "active/teal" state.
    bool pillIsActive(int pillType) const noexcept
    {
        switch (pillType)
        {
        case 0: return true;  // Pattern always looks selected (cycling widget)
        case 1: return true;  // ClockDiv always looks selected
        case 2: return true;  // Steps always looks selected
        case 3: return (currentTarget1_ != 0);
        case 4: return (currentTarget2_ != 0);
        case 5: return envFollowEnabled_;
        default: return false;
        }
    }

    //--------------------------------------------------------------------------
    /// Returns the display label for a given pill type.
    juce::String pillLabel(int pillType) const
    {
        switch (pillType)
        {
        case 0: return juce::String(kPatternNames[currentPattern_]).toUpperCase();
        case 1: return juce::String(kClockLabels[currentClockDiv_]);
        case 2: return juce::String(currentSteps_);
        case 3: return juce::String(kTargetNames[currentTarget1_]).toUpperCase();
        case 4: return juce::String(kTargetNames[currentTarget2_]).toUpperCase();
        case 5: return "ENV";
        default: return {};
        }
    }

    //==========================================================================
    /// Apply one of the 8 algorithmic patterns to fill the steps_ array.
    void applyPattern(int pattern)
    {
        const int n = currentSteps_;

        switch (pattern)
        {
        case 0: // Pulse — every other step active, velocity 0.8
            for (int i = 0; i < kMaxSteps; ++i)
            {
                steps_[i].active   = (i < n) && (i % 2 == 0);
                steps_[i].velocity = 0.80f;
            }
            break;

        case 1: // RampUp — all active, velocity ramps 0.15 → 1.0
            for (int i = 0; i < kMaxSteps; ++i)
            {
                steps_[i].active = (i < n);
                steps_[i].velocity = (n > 1)
                    ? 0.15f + (0.85f * static_cast<float>(i) / static_cast<float>(n - 1))
                    : 0.80f;
            }
            break;

        case 2: // RampDown — all active, velocity ramps 1.0 → 0.15
            for (int i = 0; i < kMaxSteps; ++i)
            {
                steps_[i].active = (i < n);
                steps_[i].velocity = (n > 1)
                    ? 1.0f - (0.85f * static_cast<float>(i) / static_cast<float>(n - 1))
                    : 0.80f;
            }
            break;

        case 3: // Triangle — all active, velocity V-shape
            for (int i = 0; i < kMaxSteps; ++i)
            {
                steps_[i].active = (i < n);
                if (n <= 1)
                {
                    steps_[i].velocity = 0.80f;
                }
                else
                {
                    const float t = static_cast<float>(i) / static_cast<float>(n - 1);
                    // Ramp up to midpoint then down — peak at 0.5.
                    float v = (t <= 0.5f)
                        ? 0.15f + 1.70f * t           // 0.15 → 1.0 over first half
                        : 1.0f - 1.70f * (t - 0.5f); // 1.0 → 0.15 over second half
                    steps_[i].velocity = juce::jlimit(0.10f, 1.0f, v);
                }
            }
            break;

        case 4: // Eucl3 — Euclidean 3-hit distribution, velocity 0.8
            for (int i = 0; i < kMaxSteps; ++i)
            {
                steps_[i].active   = (i < n) && euclideanHit(i, 3, n);
                steps_[i].velocity = 0.80f;
            }
            break;

        case 5: // Eucl5 — Euclidean 5-hit distribution, velocity 0.75
            for (int i = 0; i < kMaxSteps; ++i)
            {
                steps_[i].active   = (i < n) && euclideanHit(i, 5, n);
                steps_[i].velocity = 0.75f;
            }
            break;

        case 6: // RandomWalk — all active, Brownian velocity walk
        {
            float walk = 0.50f;
            uint32_t rng = 0xABCD1234u;
            for (int i = 0; i < kMaxSteps; ++i)
            {
                steps_[i].active = (i < n);
                if (i < n)
                {
                    // xorshift32
                    rng ^= rng << 13;
                    rng ^= rng >> 17;
                    rng ^= rng << 5;
                    const float delta = (static_cast<float>(rng & 0xFF) / 255.0f - 0.5f) * 0.30f;
                    walk = juce::jlimit(0.10f, 1.0f, walk + delta);
                    steps_[i].velocity = walk;
                }
            }
            break;
        }

        case 7: // Scatter — ~35% steps active (deterministic hash), velocity 0.6-0.95
            for (int i = 0; i < kMaxSteps; ++i)
            {
                if (i >= n)
                {
                    steps_[i].active = false;
                    continue;
                }
                uint32_t hash = static_cast<uint32_t>(i) * 2654435761u;
                const float threshold = static_cast<float>(hash & 0xFF) / 255.0f;
                steps_[i].active = (threshold < 0.35f);
                // Deterministic velocity in 0.6-0.95 range.
                uint32_t hash2 = (static_cast<uint32_t>(i) + 7u) * 1597334677u;
                steps_[i].velocity = 0.60f + (static_cast<float>(hash2 & 0xFF) / 255.0f) * 0.35f;
            }
            break;

        default:
            for (auto& s : steps_)
                s.active = false;
            break;
        }
    }

    //--------------------------------------------------------------------------
    /// Bjorklund's Euclidean rhythm algorithm — O(1) per step.
    static bool euclideanHit(int step, int hits, int totalSteps) noexcept
    {
        if (totalSteps <= 0 || hits <= 0)
            return false;
        if (hits >= totalSteps)
            return true;
        const int current  = (step * hits) / totalSteps;
        const int previous = ((step > 0 ? step - 1 : totalSteps - 1) * hits) / totalSteps;
        return current != previous;
    }

    //==========================================================================
    /// Read current parameter values from APVTS into local state.
    void syncFromApvts()
    {
        auto readInt = [&](const char* id) -> int {
            if (auto* p = apvts_.getParameter(id))
                return static_cast<int>(p->convertFrom0to1(p->getValue()) + 0.5f);
            return 0;
        };
        auto readFloat = [&](const char* id) -> float {
            if (auto* p = apvts_.getParameter(id))
                return p->getValue(); // 0-1 normalised
            return 0.0f;
        };

        currentPattern_  = juce::jlimit(0, 7,  readInt("master_seqPattern"));
        currentClockDiv_ = juce::jlimit(0, 7,  readInt("master_seqRate"));
        currentSteps_    = juce::jlimit(1, 16, readInt("master_seqSteps"));
        currentDepth_    = readFloat("master_seqDepth");
        currentSmooth_   = readFloat("master_seqSmooth");
        currentTarget1_  = juce::jlimit(0, 17, readInt("master_seqTarget1"));
        currentTarget2_  = juce::jlimit(0, 17, readInt("master_seqTarget2"));
        envFollowEnabled_ = (readInt("master_seqEnvFollow") != 0);
        currentEnvAmt_   = readFloat("master_seqEnvAmount");
    }

    //==========================================================================
    void timerCallback() override
    {
        // Advance breathe phase (2s period @ 30 Hz → 1/60 increment per tick).
        breathePhase_ = std::fmod(breathePhase_ + (1.0f / 60.0f), 1.0f);

        // Re-read playhead from sequencer (atomic read, message-thread safe).
        const int newPlayhead = sequencer_.getCurrentStep();
        if (newPlayhead != lastPlayhead_)
        {
            lastPlayhead_ = newPlayhead;
            if (expanded_)
                repaint();
        }
        else if (expanded_)
        {
            // Still need to repaint for breathe animation when env follow is on.
            if (envFollowEnabled_)
                repaint();
        }
    }

    //==========================================================================
    // References
    juce::AudioProcessorValueTreeState& apvts_;
    const MasterFXSequencer&            sequencer_;

    // Expansion state
    bool expanded_ = false;

    // Per-step data (kMaxSteps = 16)
    std::array<Step, kMaxSteps> steps_;

    // Cached per-step root-note label strings — rebuilt whenever steps_ root notes change.
    // Avoids constructing juce::String(int) inside a 16-iteration paint loop.
    std::array<juce::String, kMaxSteps> cachedRootNoteLabels_;

    void rebuildRootNoteLabels()
    {
        for (int i = 0; i < kMaxSteps; ++i)
            cachedRootNoteLabels_[static_cast<size_t>(i)] = juce::String(steps_[i].rootNote);
    }

    // APVTS-mirrored local values (written by pill/slider handlers)
    int   currentPattern_  = 0;
    int   currentClockDiv_ = 2; // Quarter note default
    int   currentSteps_    = 8;
    float currentDepth_    = 0.5f;
    float currentSmooth_   = 0.3f;
    int   currentTarget1_  = 0;
    int   currentTarget2_  = 0;
    bool  envFollowEnabled_ = false;
    float currentEnvAmt_   = 0.5f;

    // Timer state
    float breathePhase_ = 0.0f;
    int   lastPlayhead_ = -1;

    // Drag state
    int   dragStepIdx_        = -1;
    int   dragStartY_         = 0;
    int   dragStartX_         = 0;
    float dragStartVel_       = 0.75f;
    float dragStartGate_      = 0.75f;
    bool  isDragging_         = false;
    bool  mouseDownWasToggle_ = false;
    int   activeSliderType_   = -1;
    int   hoveredPill_        = -1;

    // Laid-out control regions (rebuilt each resized()/paint())
    std::vector<PillRegion>    pillRegions_;
    std::vector<float>         separatorXs_;
    juce::Rectangle<float>     patternPreviewBounds_;
    juce::Rectangle<float>     algoBadgeBounds_;
    juce::Rectangle<float>     breatheBounds_;
    juce::Rectangle<float>     depthLabelBounds_;
    juce::Rectangle<float>     smoothLabelBounds_;
    juce::Rectangle<float>     envAmtLabelBounds_;
    SliderRegion               depthSliderRegion_  {};
    SliderRegion               smoothSliderRegion_ {};
    SliderRegion               envAmtSliderRegion_ {};

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TideWaterline)
};

} // namespace xoceanus
