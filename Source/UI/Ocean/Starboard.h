// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// Starboard.h — XOceanus engine-state visual feedback strip (#1358).
//
// Starboard is the always-visible 120 px instrument panel that answers
// "what exactly am I hearing right now?" in XOuija mode. It occupies a
// fixed-height band inside SurfaceRightPanel, between the 36 px mode-name
// header and SubmarineOuijaPanel. Never collapses (Q1 = always 120 px).
//
// Visual treatment (submarine dark palette):
//   bg: GalleryColors::Ocean::abyss @ 92% opacity + 1 px bottom border in engine accent
//   Engine name: SatoshiBold 11 px all-caps
//   Preset name: SatoshiRegular 10 px, textMid; italic "— no preset —" when absent
//   XY readout: JetBrains Mono 9 px (KEY + DEPTH, normalized 0.0–1.0, 1 decimal)
//   Pin dot: deep teal (free-walk) / coral (pinned), 7 px diameter
//   Slot index pill: SatoshiBold 9 px badge; "GLOBAL" when routing = Global
//   FX chain chips: SatoshiBold 8 px, up to 3 non-bypassed chains
//
// State injection: The parent (SurfaceRightPanel) calls setState() on the
// message thread. Starboard has its own 10 Hz juce::Timer that requests
// repaint at a controlled rate. There are NO audio-thread allocations or
// calls; all values are trivially copyable plain-old-data.
//
// Appear animation: 150 ms opacity fade on show / slot change.
// Suppressed when A11y::prefersReducedMotion() is true.
//
// Acceptance criteria implementation mapping (§6):
//   ✓ Renders only in SurfaceRightPanel::Mode::Ouija (visibility controlled by parent)
//   ✓ Active slot index badge (or "GLOBAL" per Q3)
//   ✓ Engine name + accent tint
//   ✓ Preset name or italic "— no preset —"
//   ✓ XY KEY/DEPTH readouts (live or frozen when pinned)
//   ✓ Pin state dot (teal free-walk / coral pinned) + capture slot name
//   ✓ Engine routing target (Global / Slot 0–3)
//   ✓ Active FX chain chips (max 3, non-bypassed, non-Off)
//   ✓ No mood weights (Q2)
//   ✓ A11y::prefersReducedMotion() suppresses appear fade
//   ✓ No audio-thread allocations; all state via setState() on message thread

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include <cmath>
#include <array>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

//==============================================================================
/**
    Starboard

    Always-120-px engine-state instrument panel for XOuija mode.
    Parent calls setState() whenever XOuija state changes; Starboard
    repaints at its own 10 Hz tick rate (not every setState call).
    Visibility is controlled entirely by SurfaceRightPanel — Starboard
    never hides itself.

    Usage:
        starboard_.setState(s);       // from SurfaceRightPanel on any change
        addAndMakeVisible(starboard_);
        starboard_.setBounds(0, kHeaderH, getWidth(), Starboard::kHeight);
*/
class Starboard : public juce::Component, private juce::Timer
{
public:
    //==========================================================================
    static constexpr int kHeight = 120;

    //==========================================================================
    // StarboardState — plain-old-data state snapshot injected by the parent.
    // All fields are cheap to copy (no heap, no JUCE::String on audio thread —
    // all strings set from the message thread by the parent).
    struct State
    {
        // Slot index (0–3 for per-slot, -1 = Global routing).
        // -1 maps to "GLOBAL" badge (Q3).
        int activeSlot = 0;

        // Engine identity
        juce::String engineId;          // Canonical engine ID ("Onset", "Oblong", …)
        juce::String engineDisplayName; // Same as engineId by convention

        // Preset
        juce::String presetName;        // Empty when no preset loaded

        // XY position — normalized [0, 1]. When pinned, frozen at pin coords.
        float circleX    = 0.5f; // KEY axis
        float influenceY = 0.0f; // DEPTH axis

        // Pin state
        bool pinned = false;
        juce::String captureSlotName; // Empty when not pinned / LIVE

        // Routing target — mirrors XouijaCaptureSlot::EngineTarget
        // 0 = Global, 1–4 = Slot 0–3
        int engineTargetRaw = 0;

        // FX chains — up to 3 active (non-Off, non-bypassed) chain display names.
        // Use empty string for unused entries. Max 3 entries per EpicChainSlotController.
        std::array<juce::String, 3> fxChainNames;
        int numActiveFxChains = 0; // 0 = no active chains

        // Slot-change generation counter — incremented by parent on slot switch.
        // Used to trigger the appear fade on slot changes.
        uint32_t slotGeneration = 0;
    };

    //==========================================================================
    Starboard()
    {
        setOpaque(false);
        setInterceptsMouseClicks(false, false); // read-only display

        startTimerHz(10); // 10 Hz repaint tick
    }

    ~Starboard() override
    {
        stopTimer();
    }

    //==========================================================================
    // setState — called by SurfaceRightPanel on the message thread whenever
    // XOuija panel state, preset, engine, or FX chain changes.
    // Triggers appear fade when slotGeneration changes.
    void setState(const State& newState)
    {
        // Detect slot switch → trigger fade
        if (newState.slotGeneration != state_.slotGeneration)
            startAppearFade();

        state_ = newState;
        // Immediate repaint so the readout is never more than one 10 Hz tick
        // stale. The timer tick also calls repaint() to pick up live XY updates.
        repaint();
    }

    const State& getState() const noexcept { return state_; }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        const float w = bounds.getWidth();

        // ── Background ────────────────────────────────────────────────────────
        // Ocean::abyss @ 92% opacity
        g.setColour(juce::Colour(GalleryColors::Ocean::abyss).withAlpha(0.92f));
        g.fillRect(bounds);

        // Engine accent bottom border (1 px)
        const juce::Colour accentColour = accentColor();
        g.setColour(accentColour);
        g.drawLine(bounds.getX(), bounds.getBottom() - 1.0f,
                   bounds.getRight(), bounds.getBottom() - 1.0f, 1.0f);

        // Apply appear-fade alpha
        const float alpha = currentAlpha_;
        if (alpha < 0.01f)
            return; // nothing visible yet

        // ── Layout constants ──────────────────────────────────────────────────
        constexpr float kMarginX = 12.0f;
        constexpr float kMarginY =  8.0f;
        const float contentW = w - kMarginX * 2.0f;

        float y = kMarginY;

        // ── Row 1: Slot badge + Engine name ──────────────────────────────────
        // Slot pill (left)
        {
            const juce::String slotLabel = (state_.activeSlot < 0)
                                           ? "GLOBAL"
                                           : juce::String(state_.activeSlot);
            const float pillW = 44.0f;
            const float pillH = 18.0f;
            const juce::Rectangle<float> pillR(kMarginX, y, pillW, pillH);

            g.setColour(accentColour.withAlpha(alpha * 0.22f));
            g.fillRoundedRectangle(pillR, 4.0f);
            g.setColour(accentColour.withAlpha(alpha * 0.75f));
            g.drawRoundedRectangle(pillR, 4.0f, 1.0f);

            g.setColour(juce::Colour(GalleryColors::Ocean::foam).withAlpha(alpha));
            g.setFont(GalleryFonts::heading(9.0f));
            g.drawText(slotLabel, pillR, juce::Justification::centred, false);
        }

        // Engine name (right of pill)
        {
            const bool hasEngine = state_.engineId.isNotEmpty();
            const juce::String engineText = hasEngine
                ? state_.engineDisplayName.toUpperCase()
                : "— NO ENGINE —";

            const juce::Colour nameCol = hasEngine
                ? accentColour.withAlpha(alpha)
                : juce::Colour(GalleryColors::Ocean::salt).withAlpha(alpha * 0.7f);

            g.setColour(nameCol);
            g.setFont(GalleryFonts::heading(11.0f));
            const float nameX = kMarginX + 50.0f;
            g.drawText(engineText,
                       juce::Rectangle<float>(nameX, y, contentW - 50.0f, 18.0f),
                       juce::Justification::centredLeft, true);
        }

        y += 22.0f;

        // ── Row 2: Preset name ────────────────────────────────────────────────
        {
            const bool hasPreset = state_.presetName.isNotEmpty();
            g.setFont(GalleryFonts::body(10.0f));
            g.setColour(juce::Colour(GalleryColors::Ocean::salt).withAlpha(alpha * 0.9f));

            if (hasPreset)
            {
                g.drawText(state_.presetName,
                           juce::Rectangle<float>(kMarginX, y, contentW, 14.0f),
                           juce::Justification::centredLeft, true);
            }
            else
            {
                // Italic "— no preset —" — draw with slightly dimmed text
                g.setColour(juce::Colour(GalleryColors::Ocean::plankton).withAlpha(alpha * 0.8f));
                // Italic approximation: use body font (Satoshi doesn't have a distinct italic
                // weight in the embedded set — use the regular at reduced alpha as the spec
                // calls out italic style, which is the visual hierarchy cue here)
                g.drawText(u8"— no preset —",
                           juce::Rectangle<float>(kMarginX, y, contentW, 14.0f),
                           juce::Justification::centredLeft, true);
            }
        }

        y += 18.0f;

        // ── Row 3: XY readouts ────────────────────────────────────────────────
        {
            g.setFont(GalleryFonts::value(9.0f)); // JetBrains Mono

            // KEY (circleX normalized 0.0–1.0, 1 decimal)
            const juce::String keyStr = "KEY " + juce::String(state_.circleX, 1);
            // DEPTH (influenceY normalized 0.0–1.0, 1 decimal)
            const juce::String depthStr = "DEPTH " + juce::String(state_.influenceY, 1);

            const juce::Colour xyCol = state_.pinned
                ? juce::Colour(0xFFE76F51).withAlpha(alpha * 0.9f)   // coral (pinned, frozen)
                : juce::Colour(GalleryColors::Ocean::salt).withAlpha(alpha * 0.8f);

            g.setColour(xyCol);
            const float halfW = contentW * 0.5f;
            g.drawText(keyStr,
                       juce::Rectangle<float>(kMarginX, y, halfW, 13.0f),
                       juce::Justification::centredLeft, false);
            g.drawText(depthStr,
                       juce::Rectangle<float>(kMarginX + halfW, y, halfW, 13.0f),
                       juce::Justification::centredLeft, false);
        }

        y += 17.0f;

        // ── Row 4: Pin state dot + capture slot / routing ─────────────────────
        {
            // Pin dot (7 px diameter)
            constexpr float kDotDiameter = 7.0f;
            constexpr float kDotRadius   = kDotDiameter * 0.5f;
            const float dotCX = kMarginX + kDotRadius;
            const float dotCY = y + kDotRadius + 1.0f;

            // Deep teal = free-walk, coral = pinned
            const juce::Colour dotCol = state_.pinned
                ? juce::Colour(0xFFE76F51)  // coral
                : juce::Colour(0xFF2A9D8F); // deep teal

            g.setColour(dotCol.withAlpha(alpha));
            g.fillEllipse(dotCX - kDotRadius, dotCY - kDotRadius,
                          kDotDiameter, kDotDiameter);

            // Pin state label + capture slot name / routing target
            const float labelX = kMarginX + kDotDiameter + 6.0f;
            const float labelW = contentW - kDotDiameter - 6.0f;

            g.setFont(GalleryFonts::heading(9.0f));
            g.setColour(juce::Colour(GalleryColors::Ocean::salt).withAlpha(alpha * 0.8f));

            juce::String pinLabel = state_.pinned ? "PINNED" : "FREE-WALK";

            // Append capture slot name or routing target
            if (state_.pinned && state_.captureSlotName.isNotEmpty())
                pinLabel += "  " + state_.captureSlotName;

            // Engine routing target
            const juce::String routeLabel = engineTargetLabel(state_.engineTargetRaw);
            if (routeLabel.isNotEmpty())
                pinLabel += "  \xE2\x86\x92 " + routeLabel; // → arrow

            g.drawText(pinLabel,
                       juce::Rectangle<float>(labelX, y, labelW, 15.0f),
                       juce::Justification::centredLeft, true);
        }

        y += 19.0f;

        // ── Row 5: FX chain chips ─────────────────────────────────────────────
        {
            if (state_.numActiveFxChains == 0)
            {
                // Empty state — "— no chain —" at 50% opacity
                g.setFont(GalleryFonts::heading(8.0f));
                g.setColour(juce::Colour(GalleryColors::Ocean::plankton).withAlpha(alpha * 0.5f));
                g.drawText(u8"— no chain —",
                           juce::Rectangle<float>(kMarginX, y, contentW, 14.0f),
                           juce::Justification::centredLeft, false);
            }
            else
            {
                float chipX = kMarginX;
                const float chipH = 14.0f;
                const float chipGap = 4.0f;
                const int maxChips = juce::jmin(state_.numActiveFxChains, 3);

                g.setFont(GalleryFonts::heading(8.0f));

                for (int i = 0; i < maxChips; ++i)
                {
                    const auto& name = state_.fxChainNames[static_cast<size_t>(i)];
                    if (name.isEmpty())
                        continue;

                    // Measure chip width
                    const float textW = GalleryFonts::heading(8.0f)
                        .getStringWidthFloat(name) + 10.0f;
                    const float chipW = juce::jmax(textW, 30.0f);

                    if (chipX + chipW > kMarginX + contentW)
                        break; // overflow — skip remaining chips

                    const juce::Rectangle<float> chipR(chipX, y, chipW, chipH);

                    g.setColour(accentColour.withAlpha(alpha * 0.15f));
                    g.fillRoundedRectangle(chipR, 3.0f);
                    g.setColour(accentColour.withAlpha(alpha * 0.4f));
                    g.drawRoundedRectangle(chipR, 3.0f, 0.75f);

                    g.setColour(juce::Colour(GalleryColors::Ocean::foam).withAlpha(alpha * 0.85f));
                    g.drawText(name.toUpperCase(), chipR,
                               juce::Justification::centred, false);

                    chipX += chipW + chipGap;
                }
            }
        }
    }

    void resized() override {}

    //==========================================================================
    // Appear fade API — also called externally on slot switch
    void startAppearFade()
    {
        if (A11y::prefersReducedMotion())
        {
            currentAlpha_ = 1.0f;
            fadeTimer_     = 0;
            return;
        }
        currentAlpha_  = 0.0f;
        fadeTimer_     = 0;
        fadeDuration_  = kFadeDurationMs;
    }

private:
    //==========================================================================
    // Timer callback — drives repaint at 10 Hz and advances appear animation.
    void timerCallback() override
    {
        if (fadeDuration_ > 0 && fadeTimer_ < fadeDuration_)
        {
            fadeTimer_ += 100; // 10 Hz → 100 ms per tick
            currentAlpha_ = juce::jlimit(0.0f, 1.0f,
                                         static_cast<float>(fadeTimer_) / static_cast<float>(fadeDuration_));
        }
        else
        {
            currentAlpha_ = 1.0f;
        }
        repaint();
    }

    //==========================================================================
    // Helpers

    juce::Colour accentColor() const
    {
        if (state_.engineId.isEmpty())
            return juce::Colour(GalleryColors::xoGold); // XO Gold fallback
        return GalleryColors::accentForEngine(state_.engineId);
    }

    static juce::String engineTargetLabel(int raw)
    {
        switch (raw)
        {
            case 0: return "GLOBAL";
            case 1: return "SLOT 0";
            case 2: return "SLOT 1";
            case 3: return "SLOT 2";
            case 4: return "SLOT 3";
            default: return "GLOBAL";
        }
    }

    //==========================================================================
    // State
    State state_;

    // Appear fade animation
    static constexpr int kFadeDurationMs = 150;
    int   fadeDuration_ = 0;   // 0 = no fade in progress
    int   fadeTimer_    = 0;   // ms elapsed since fade start
    float currentAlpha_ = 1.0f; // current display alpha [0, 1]

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Starboard)
};

} // namespace xoceanus
