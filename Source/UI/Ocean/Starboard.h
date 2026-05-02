// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
// DORMANT (2026-05-01): preserved for future EpicSlotsPanel reuse — not currently
// rendered. The Starboard::State struct's "what are you hearing" semantics may be
// resurrected for a per-slot FX strip readout in the EpicSlotsPanel polish work.
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

        // Fix #1423: expose engine/preset/XY panel to screen readers.
        A11y::setup(*this,
                    "Engine Status Panel",
                    "Shows current engine, preset, XY position, and FX chains",
                    /*wantsKeyFocus=*/false);

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

        // Fix #1423: announce engine/preset changes to screen readers.
        const bool engineChanged = (newState.engineId    != state_.engineId);
        const bool presetChanged = (newState.presetName  != state_.presetName);

        state_ = newState;
        // Mark dirty; the 10 Hz timer will call repaint() on the next tick.
        // This prevents 30 Hz XOuija position callbacks from driving 30 Hz repaints
        // on a component that is contractually a 10 Hz display.
        stateDirty_ = true;

        // Post accessibility notification when meaningful content changes.
        // getAccessibilityHandler() may return nullptr if the component is not yet
        // added to the tree or if accessibility is disabled — guard accordingly.
        if ((engineChanged || presetChanged) && isAccessible())
            if (auto* handler = getAccessibilityHandler())
                handler->notifyAccessibilityEvent(juce::AccessibilityEvent::valueChanged);
    }

    const State& getState() const noexcept { return state_; }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        const float w = bounds.getWidth();

        // Apply appear-fade alpha to the entire component (background + border + content).
        // Using a transparency layer means the fade drives all draw calls uniformly —
        // no per-draw-call alpha multiplication required.
        const float alpha = currentAlpha_;
        if (alpha < 0.01f)
            return; // nothing visible yet — skip paint entirely

        g.beginTransparencyLayer(alpha);

        // ── Background ────────────────────────────────────────────────────────
        // Ocean::abyss @ 92% opacity (multiplied by fade alpha via transparency layer)
        g.setColour(juce::Colour(GalleryColors::Ocean::abyss).withAlpha(0.92f));
        g.fillRect(bounds);

        // Engine accent bottom border (1 px)
        const juce::Colour accentColour = accentColor();
        g.setColour(accentColour);
        g.drawLine(bounds.getX(), bounds.getBottom() - 1.0f,
                   bounds.getRight(), bounds.getBottom() - 1.0f, 1.0f);

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

            g.setColour(accentColour.withAlpha(0.22f));
            g.fillRoundedRectangle(pillR, 4.0f);
            g.setColour(accentColour.withAlpha(0.75f));
            g.drawRoundedRectangle(pillR, 4.0f, 1.0f);

            g.setColour(juce::Colour(GalleryColors::Ocean::foam));
            // Fix #1425: raised from 9 px (unreadable) to 11 px (acceptable minimum).
            g.setFont(GalleryFonts::heading(11.0f));
            g.drawText(slotLabel, pillR, juce::Justification::centred, false);
        }

        // Engine name (right of pill)
        {
            const bool hasEngine = state_.engineId.isNotEmpty();
            const juce::String engineText = hasEngine
                ? state_.engineDisplayName.toUpperCase()
                : "— NO ENGINE —";

            const juce::Colour nameCol = hasEngine
                ? accentColour
                : juce::Colour(GalleryColors::Ocean::salt).withAlpha(0.7f);

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
            g.setColour(juce::Colour(GalleryColors::Ocean::salt).withAlpha(0.9f));

            if (hasPreset)
            {
                g.drawText(state_.presetName,
                           juce::Rectangle<float>(kMarginX, y, contentW, 14.0f),
                           juce::Justification::centredLeft, true);
            }
            else
            {
                // Fix #1429: apply AffineTransform skew to produce faux-italic for the
                // "no preset" placeholder.  Satoshi lacks an embedded italic variant;
                // a 12° shear (tan(12°)≈0.213) produces a legible italicised appearance
                // without a separate font file.  We restore the transform afterwards so
                // subsequent draw calls are unaffected.
                g.setColour(juce::Colour(GalleryColors::Ocean::plankton).withAlpha(0.8f));
                {
                    const juce::Rectangle<float> placeholderR(kMarginX, y, contentW, 14.0f);
                    // Shear around the vertical centre of the text baseline.
                    const float shearAnchorX = placeholderR.getX();
                    g.addTransform(juce::AffineTransform::shear(0.213f, 0.0f)
                                       .translated(-shearAnchorX * 0.213f, 0.0f));
                    g.drawText(u8"— no preset —",
                               placeholderR,
                               juce::Justification::centredLeft, true);
                    // Restore identity — undo the shear.
                    g.addTransform(juce::AffineTransform::shear(-0.213f, 0.0f)
                                       .translated(shearAnchorX * 0.213f, 0.0f));
                }
            }
        }

        y += 18.0f;

        // ── Row 3: XY readouts ────────────────────────────────────────────────
        {
            // Fix #1425: raised from 9 px (unreadable) to 11 px for scan-readable XY values.
            g.setFont(GalleryFonts::value(11.0f)); // JetBrains Mono

            // KEY (circleX normalized 0.0–1.0, 1 decimal)
            const juce::String keyStr = "KEY " + juce::String(state_.circleX, 1);
            // DEPTH (influenceY normalized 0.0–1.0, 1 decimal)
            const juce::String depthStr = "DEPTH " + juce::String(state_.influenceY, 1);

            const juce::Colour xyCol = state_.pinned
                ? juce::Colour(0xFFE76F51).withAlpha(0.9f)   // coral (pinned, frozen)
                : juce::Colour(GalleryColors::Ocean::salt).withAlpha(0.8f);

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

            // Fix #1427: color-only pin state (teal vs coral) is invisible to
            // color-vision-deficient users.  Added shape cue: free-walk = open circle
            // outline (unfilled); pinned = filled circle.  Color retained as secondary cue.
            // Deep teal = free-walk (outline), coral = pinned (filled).
            const juce::Colour dotCol = state_.pinned
                ? juce::Colour(0xFFE76F51)  // coral
                : juce::Colour(0xFF2A9D8F); // deep teal

            g.setColour(dotCol);
            if (state_.pinned)
            {
                // Pinned: filled circle
                g.fillEllipse(dotCX - kDotRadius, dotCY - kDotRadius,
                              kDotDiameter, kDotDiameter);
            }
            else
            {
                // Free-walk: outline circle — shape distinguishes state without relying on color alone
                g.drawEllipse(dotCX - kDotRadius, dotCY - kDotRadius,
                              kDotDiameter, kDotDiameter, 1.5f);
            }

            // Pin state label + capture slot name / routing target
            const float labelX = kMarginX + kDotDiameter + 6.0f;
            const float labelW = contentW - kDotDiameter - 6.0f;

            // Fix #1425: raised from 9 px (unreadable) to 11 px for pin state label.
            g.setFont(GalleryFonts::heading(11.0f));
            g.setColour(juce::Colour(GalleryColors::Ocean::salt).withAlpha(0.8f));

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
                // Fix #1425: raised from 8 px to 10 px for supplemental label readability.
                g.setFont(GalleryFonts::heading(10.0f));
                g.setColour(juce::Colour(GalleryColors::Ocean::plankton).withAlpha(0.5f));
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

                // Fix #1425: raised FX chip font from 8 px (unreadable) to 10 px.
                g.setFont(GalleryFonts::heading(10.0f));

                for (int i = 0; i < maxChips; ++i)
                {
                    const auto& name = state_.fxChainNames[static_cast<size_t>(i)];
                    if (name.isEmpty())
                        continue;

                    // Measure chip width against the uppercased string — the drawn text
                    // is toUpperCase(), so we must measure the same string to avoid clipping.
                    const juce::String upperName = name.toUpperCase();
                    const float textW = GalleryFonts::heading(10.0f)
                        .getStringWidthFloat(upperName) + 10.0f;
                    const float chipW = juce::jmax(textW, 30.0f);

                    if (chipX + chipW > kMarginX + contentW)
                        break; // overflow — skip remaining chips

                    const juce::Rectangle<float> chipR(chipX, y, chipW, chipH);

                    g.setColour(accentColour.withAlpha(0.15f));
                    g.fillRoundedRectangle(chipR, 3.0f);
                    g.setColour(accentColour.withAlpha(0.4f));
                    g.drawRoundedRectangle(chipR, 3.0f, 0.75f);

                    g.setColour(juce::Colour(GalleryColors::Ocean::foam).withAlpha(0.85f));
                    g.drawText(upperName, chipR,
                               juce::Justification::centred, false);

                    chipX += chipW + chipGap;
                }
            }
        }

        g.endTransparencyLayer();
    }

    void resized() override {}

    //==========================================================================
    // Appear fade API — also called externally on slot switch.
    // Drives alpha from 0 → 1 over exactly kFadeDurationMs milliseconds
    // using elapsed real time rather than tick-step accumulation.
    void startAppearFade()
    {
        if (A11y::prefersReducedMotion())
        {
            currentAlpha_ = 1.0f;
            fadeActive_   = false;
            return;
        }
        currentAlpha_  = 0.0f;
        fadeStartMs_   = juce::Time::getMillisecondCounterHiRes();
        fadeActive_    = true;
    }

private:
    //==========================================================================
    // Timer callback — drives repaint at 10 Hz and advances appear animation.
    // Alpha is computed from elapsed real time so the fade always takes exactly
    // kFadeDurationMs regardless of timer jitter or tick interval.
    void timerCallback() override
    {
        bool needsRepaint = stateDirty_;
        stateDirty_ = false;

        if (fadeActive_)
        {
            const double elapsed = juce::Time::getMillisecondCounterHiRes() - fadeStartMs_;
            currentAlpha_ = juce::jmin(1.0f, static_cast<float>(elapsed / kFadeDurationMs));
            needsRepaint = true;
            if (currentAlpha_ >= 1.0f)
                fadeActive_ = false; // fade complete — final repaint still fires below
        }

        if (needsRepaint)
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
    static constexpr double kFadeDurationMs = 150.0; // target duration in real milliseconds
    bool   fadeActive_   = false;   // true while fade is in progress
    double fadeStartMs_  = 0.0;    // juce::Time::getMillisecondCounterHiRes() at fade start
    float  currentAlpha_ = 1.0f;   // current display alpha [0, 1]

    // Repaint throttle — set by setState(), cleared by timerCallback()
    bool stateDirty_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Starboard)
};

} // namespace xoceanus
