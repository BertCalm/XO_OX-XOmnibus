// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// EngineOrbit.h — Floating buoy in the Ocean View.
//
// Represents one engine slot (0-4) or FX slot as a buoy floating in the ocean.
// Engines render as circles with waveform wreaths showing real audio output.
// FX slots render as hexagons (small for FX engines, large for master FX).
//
// Spec (ocean-ui-phase1-design.md):
//   - Buoys are freeform-draggable within the ocean viewport
//   - Double-click opens the floating detail overlay
//   - Waveform wreath reads from WaveformFifo (real audio visualization)
//   - Creature sprites render inside the buoy circle (D5 hybrid decision)
//   - Ripple rings expand on note-on events
//   - Breath animation scales with voice activity
//
// RAC Audit:
//   - No InteractionState enum (removed per merged Steps 3+5)
//   - Ripples use std::array<Ripple,8> not std::vector (F4)
//   - Wreath reads from WaveformFifo, not WaveformRingBuffer (F1-F3)

#include <array>
#include <unordered_map>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../Tokens.h"
#include "../Gallery/CreatureRenderer.h"
#include "../EngineVocabulary.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

namespace xoceanus
{

//==============================================================================
/**
    EngineOrbit — a floating buoy in the Ocean View.

    Renders one engine/FX slot as a buoy with:
      - Circle body (engines) or hexagon body (FX)
      - Waveform wreath ring showing real audio output
      - Creature sprite inside the buoy (engines only)
      - Breath animation, coupling lean, heartbeat glow
      - Ripple rings on note-on
      - Freeform drag positioning

    Animation is driven by OceanView's single shared 30 Hz Timer, which
    calls stepAnimation() on every tick and requestRepaint() for orbits
    that have engines.  Breath and wreath are frozen when
    A11y::prefersReducedMotion() returns true.
*/
class EngineOrbit : public juce::Component,
                    public juce::SettableTooltipClient
{
public:
    //==========================================================================
    enum class DepthZone  { Sunlit, Twilight, Midnight };
    enum class BuoyType   { Engine, FxEngine, MasterFx };
    enum class WreathShape { Sine, Saw, Square, Tri, Noise, Organ, Pulse, Harmonic };
    enum class InputState  { Idle, UserDragging, Settling };

    //==========================================================================
    static constexpr float kOrbitalSize = 72.0f;   ///< Engine buoy diameter

    //==========================================================================
    EngineOrbit()
    {
        A11y::setup(*this, "Engine Buoy", "Engine slot. Double-click to edit.");
        // F3-005: setWantsKeyboardFocus(true) removed — EngineOrbit has no keyPressed()
        // handler, creating a keyboard trap.  OceanView owns keyboard focus for this view.
        // Allow glow, wreath, bob animation, and name label to paint outside
        // the component's bounds without being clipped by JUCE.
        setPaintingIsUnclipped(true);
        // Ghost slots are visual only — pass mouse events through until an
        // engine is loaded.  setEngine() restores full interactivity.
        setInterceptsMouseClicks(false, false);
        // Animation is driven by OceanView's shared 30 Hz timer via stepAnimation().
    }

    ~EngineOrbit() override = default;

    //==========================================================================
    // juce::Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
        if (!hasEngine_)
        {
            // Ghost slot: dashed circle outline + "+" sign + slot number label.
            // Matches the HTML prototype empty-state spec and OceanBackground ghost style.
            const auto localBounds = getLocalBounds().toFloat();
            const float cx = localBounds.getCentreX();
            const float cy = localBounds.getCentreY();
            const float r  = getBuoyRadius();
            const juce::Colour ghostCol(127, 219, 202);

            // Dashed circle outline (1.5px stroke, 4px dash / 4px gap)
            juce::Path circle;
            circle.addEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
            juce::Path dashedCircle;
            const float dashes[] = { 4.0f, 4.0f };
            juce::PathStrokeType(1.5f).createDashedStroke(dashedCircle, circle, dashes, 2);
            g.setColour(ghostCol.withAlpha(0.30f));
            g.fillPath(dashedCircle);

            // "+" centred in the circle
            g.setFont(juce::Font(juce::FontOptions{}.withHeight(20.0f)));
            g.setColour(ghostCol.withAlpha(0.35f));
            g.drawText("+",
                       juce::Rectangle<float>(cx - 15.0f, cy - 12.0f, 30.0f, 24.0f).toNearestInt(),
                       juce::Justification::centred, false);

            // #1168: Invitation text below the circle — depth-zone-aware, no slot numbers.
            {
                juce::String inviteText;
                switch (depthZone_)
                {
                    case DepthZone::Sunlit:   inviteText = "Drop here to begin"; break;
                    case DepthZone::Twilight: inviteText = juce::CharPointer_UTF8("Twilight \xc2\xb7 Drop an engine"); break;
                    case DepthZone::Midnight: inviteText = juce::CharPointer_UTF8("Midnight \xc2\xb7 Drop an engine"); break;
                    default:                  inviteText = "Drop an engine here"; break;
                }
                g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.0f)));
                g.setColour(ghostCol.withAlpha(0.25f));
                g.drawText(inviteText,
                           juce::Rectangle<float>(cx - 60.0f, cy + r + 4.0f, 120.0f, 10.0f).toNearestInt(),
                           juce::Justification::centred, false);
            }

            // Q1 (#1356): "no engine" dim preset pill for empty slots.
            // Tooltip wired in EngineOrbit constructor via setTooltip() is not applicable
            // here (ghost slots use setInterceptsMouseClicks(false)); the pill text itself
            // serves as the affordance label.
            {
                const float labelH = kNameFontSize + 4.0f;
                const float pillW  = 68.0f;
                const float pillX  = cx - pillW * 0.5f;
                const float pillY  = cy + r + 3.0f + labelH + 1.0f;
                const float pillH  = 13.0f;
                juce::Path pillPath;
                pillPath.addRoundedRectangle(pillX, pillY, pillW, pillH, 4.0f);
                g.setColour(ghostCol.withAlpha(0.07f));
                g.fillPath(pillPath);
                g.setColour(ghostCol.withAlpha(0.15f));
                g.strokePath(pillPath, juce::PathStrokeType(0.75f));
                g.setFont(GalleryFonts::label(8.0f));
                g.setColour(ghostCol.withAlpha(0.20f));
                g.drawText("no engine",
                           juce::Rectangle<float>(pillX + 3.0f, pillY, pillW - 6.0f, pillH).toNearestInt(),
                           juce::Justification::centredLeft, false);
            }
            return;
        }

        const auto localBounds = getLocalBounds().toFloat();
        const float cx = localBounds.getCentreX();
        const float cy = localBounds.getCentreY();

        // ── Effective size with breath ──────────────────────────────────────
        // Activity-driven breath (existing) + hover breath (1Hz, ±3% scale, #12).
        const float actBreath  = A11y::prefersReducedMotion()
            ? 0.0f
            : kBreathAmplitude * std::sin(breathPhase_) * activityLevel_;
        const float hoverBreath = A11y::prefersReducedMotion()
            ? 0.0f
            : kHoverBreathAmplitude * std::sin(hoverBreathPhase_) * hoverBreathLevel_;
        const float breathAmplitude = actBreath + hoverBreath;
        const float creatureSize = kOrbitalSize * (1.0f + breathAmplitude);
        const float halfSize     = creatureSize * 0.5f;
        const float radius       = getBuoyRadius();

        // ── Heartbeat glow ─────────────────────────────────────────────────
        const float heartbeatGlow = (playSurfaceVisible_ && voiceCount_ > 0)
            ? 0.15f + 0.10f * std::sin(breathPhase_ * 2.0f)
            : 0.0f;

        // ── Apply bobbing + tilt transforms (FIX 2) ────────────────────────
        // ScopedSaveState ensures these transforms don't bleed to parent painting.
        juce::Graphics::ScopedSaveState saveState(g);
        if (!A11y::prefersReducedMotion())
        {
            // Tidepool float: lateral sway + vertical bob + drop-in bounce
            g.addTransform(juce::AffineTransform::translation(lateralOffset_, bobOffset_ + bounceOffset_));
            g.addTransform(juce::AffineTransform::rotation(tiltAngle_, cx + lateralOffset_,
                                                            cy + bobOffset_ + bounceOffset_));
        }

        // ── Water reflection ellipse (subtle, below buoy) ──────────────────
        g.setColour(XO::Tokens::Color::accent().withAlpha(0.04f));
        g.fillEllipse(cx - radius * 0.75f, cy + radius + 3.0f,
                       radius * 1.5f, 5.0f);

        // ── Glow halo ──────────────────────────────────────────────────────
        {
            juce::ColourGradient glowGrad(
                accentColour_.withAlpha(0.09f + heartbeatGlow * 0.3f),
                cx, cy, accentColour_.withAlpha(0.0f),
                cx + radius * 2.2f, cy, true);
            g.setGradientFill(glowGrad);
            g.fillRect(cx - radius * 2.5f, cy - radius * 2.5f,
                       radius * 5.0f, radius * 5.0f);
        }

        // ── Active-state halo ring (#13) — RMS-driven outer glow ───────────
        if (activityLevel_ > 0.005f && !A11y::prefersReducedMotion())
        {
            const float haloPulse  = 0.5f + 0.5f * std::sin(breathPhase_ * 3.0f);
            const float haloAlpha  = activityLevel_ * (0.25f + haloPulse * 0.20f);
            const float haloRadius = radius * (1.6f + activityLevel_ * 0.8f + haloPulse * 0.3f);
            juce::ColourGradient haloGrad(
                accentColour_.withAlpha(haloAlpha),
                cx, cy,
                accentColour_.withAlpha(0.0f),
                cx + haloRadius, cy, true);
            g.setGradientFill(haloGrad);
            g.fillEllipse(cx - haloRadius, cy - haloRadius,
                          haloRadius * 2.0f, haloRadius * 2.0f);
        }

        const bool isFx = (buoyType_ == BuoyType::FxEngine || buoyType_ == BuoyType::MasterFx);

        if (isFx)
        {
            // ── HEXAGON BODY (FX buoys) ────────────────────────────────────
            juce::Path hexPath;
            for (int i = 0; i < 6; ++i)
            {
                const float a = (juce::MathConstants<float>::pi / 3.0f) * i
                              - juce::MathConstants<float>::pi / 6.0f;
                const float px = cx + radius * std::cos(a);
                const float py = cy + radius * std::sin(a);
                if (i == 0) hexPath.startNewSubPath(px, py);
                else        hexPath.lineTo(px, py);
            }
            hexPath.closeSubPath();

            // Body fill
            juce::ColourGradient hexGrad(
                accentColour_.withAlpha(0.16f), cx - 3, cy - 3,
                accentColour_.withAlpha(0.05f), cx + radius, cy + radius, true);
            g.setGradientFill(hexGrad);
            g.fillPath(hexPath);

            // Border
            g.setColour(accentColour_.withAlpha(0.40f + heartbeatGlow));
            g.strokePath(hexPath, juce::PathStrokeType(kBorderWidth));

            // Inner hex ring
            juce::Path innerHex;
            for (int i = 0; i < 6; ++i)
            {
                const float a = (juce::MathConstants<float>::pi / 3.0f) * i
                              - juce::MathConstants<float>::pi / 6.0f;
                const float r2 = radius - 5.0f;
                const float px = cx + r2 * std::cos(a);
                const float py = cy + r2 * std::sin(a);
                if (i == 0) innerHex.startNewSubPath(px, py);
                else        innerHex.lineTo(px, py);
            }
            innerHex.closeSubPath();
            g.setColour(accentColour_.withAlpha(0.12f));
            g.strokePath(innerHex, juce::PathStrokeType(1.0f));
        }
        else
        {
            // ── CIRCLE BODY + WAVEFORM WREATH (engine buoys) ───────────────

            // Circle body fill
            juce::ColourGradient bodyGrad(
                accentColour_.withAlpha(0.20f), cx - 5, cy - 5,
                accentColour_.withAlpha(0.06f), cx + radius, cy + radius, true);
            g.setGradientFill(bodyGrad);
            g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

            // ── Waveform wreath (FIX 1: shape-blend) ──────────────────────
            // Blends idle shape function (personality) with live audio data.
            // wreathIntensity_ == 0 → pure shape, == 1 → pure audio buffer.
            if (!A11y::prefersReducedMotion())
            {
                const float wreathRadius = radius + 3.0f;
                // Zero at idle (smooth circle). Activity + audio drive oscillation.
                const float wreathAmp = activityLevel_ * 4.0f
                                      + wreathIntensity_ * 6.0f + wreathFlare_ * 12.0f;

                // At idle (near-zero amplitude): draw a clean ellipse instead
                // of the 128-point shape path.  This avoids sub-pixel
                // antialiasing shimmer from path reconstruction on repaint.
                if (wreathAmp < 0.5f)
                {
                    g.setColour(accentColour_.withAlpha(0.10f));
                    g.drawEllipse(cx - wreathRadius, cy - wreathRadius,
                                  wreathRadius * 2.0f, wreathRadius * 2.0f, 6.0f);
                    g.setColour(accentColour_.withAlpha(0.50f));
                    g.drawEllipse(cx - wreathRadius, cy - wreathRadius,
                                  wreathRadius * 2.0f, wreathRadius * 2.0f, 1.5f);
                }
                else
                {
                constexpr int steps = 128;

                juce::Path wreathPath;
                for (int i = 0; i <= steps; ++i)
                {
                    const float angle = (static_cast<float>(i) / steps)
                                      * juce::MathConstants<float>::twoPi;
                    const float waveT = angle * static_cast<float>(wreathHarmonics_) + wreathPhase_;
                    // Read from wreath buffer (real audio)
                    const int bufIdx = static_cast<int>(
                        std::fmod(std::abs(waveT), juce::MathConstants<float>::twoPi)
                        / juce::MathConstants<float>::twoPi * kWreathBufferSize) % kWreathBufferSize;
                    const float audioSample = wreathBuffer_[static_cast<size_t>(bufIdx)];
                    const float shapeSample = computeWreathSample(waveT);
                    // Blend: idle → shape dominates, active → audio dominates
                    const float blend = wreathIntensity_; // 0=shape only, 1=audio only
                    const float sample = shapeSample * (1.0f - blend) + audioSample * blend;
                    const float displacement = sample * wreathAmp;
                    const float r = wreathRadius + displacement;
                    const float px = cx + r * std::cos(angle);
                    const float py = cy + r * std::sin(angle);
                    if (i == 0) wreathPath.startNewSubPath(px, py);
                    else        wreathPath.lineTo(px, py);
                }
                wreathPath.closeSubPath();

                // Glow pass
                g.setColour(accentColour_.withAlpha(0.10f));
                g.strokePath(wreathPath, juce::PathStrokeType(6.0f));

                // Core pass
                const float coreAlpha = std::min(1.0f,
                    0.50f + wreathIntensity_ * 0.30f + wreathFlare_ * 0.40f);
                g.setColour(accentColour_.withAlpha(coreAlpha));
                g.strokePath(wreathPath, juce::PathStrokeType(1.5f + wreathIntensity_ * 0.5f));
                } // end wreath path block

                // ── Ambient edge glow (FIX 4: post-wreath, over wreath) ────
                juce::ColourGradient ambientGlow(
                    accentColour_.withAlpha(0.08f), cx, cy,
                    accentColour_.withAlpha(0.0f), cx + radius * 2.2f, cy, true);
                g.setGradientFill(ambientGlow);
                g.fillEllipse(cx - radius * 2.2f, cy - radius * 2.2f,
                              radius * 4.4f, radius * 4.4f);
            }

            // Subtle inner reference ring
            g.setColour(accentColour_.withAlpha(0.08f));
            g.drawEllipse(cx - radius + 1, cy - radius + 1,
                          (radius - 1) * 2.0f, (radius - 1) * 2.0f, 1.0f);

            // ── Creature sprite inside buoy (D5 hybrid) ────────────────────
            const juce::Rectangle<float> creatureBounds(
                cx - halfSize, cy - halfSize, creatureSize, creatureSize);
            CreatureRenderer::drawCreature(g, creatureBounds, engineId_,
                                           accentColour_,
                                           1.0f + breathAmplitude,
                                           couplingLean_);
        }

        // ── FX icon (for FX buoys only) ────────────────────────────────────
        if (isFx)
        {
            g.setFont(juce::Font(juce::FontOptions(buoyType_ == BuoyType::MasterFx ? 20.0f : 15.0f)));
            g.setColour(accentColour_.withAlpha(0.8f));
            g.drawText(juce::String::charToString(0x2756), // ❖
                       juce::Rectangle<float>(cx - 10, cy - 10, 20, 20).toNearestInt(),
                       juce::Justification::centred, false);
        }

        // ── Engine name label ──────────────────────────────────────────────
        {
            const float labelY = cy + radius + 3.0f;
            const float labelH = kNameFontSize + 4.0f;
            // Fjørd for engine identity text (submarine typography system)
            g.setFont(juce::Font(juce::FontOptions{}
                .withTypeface(GalleryFonts::moodTypeface(GalleryFonts::MoodType::Foundation))
                .withHeight(isFx ? 8.0f : 11.0f)));
            g.setColour(accentColour_.withAlpha(0.58f));
            g.drawText(engineId_.toUpperCase(),
                       juce::Rectangle<float>(0.0f, labelY, localBounds.getWidth(), labelH).toNearestInt(),
                       juce::Justification::centredTop, true);

            // Type sublabel for FX
            if (buoyType_ == BuoyType::MasterFx)
            {
                g.setFont(juce::Font(juce::FontOptions(7.0f)));
                g.setColour(juce::Colour(200, 204, 216).withAlpha(0.28f));
                g.drawText("MASTER FX",
                           juce::Rectangle<float>(0.0f, labelY + labelH - 2, localBounds.getWidth(), 10.0f).toNearestInt(),
                           juce::Justification::centredTop, false);
            }
            else if (buoyType_ == BuoyType::FxEngine)
            {
                g.setFont(juce::Font(juce::FontOptions(7.0f)));
                g.setColour(juce::Colour(200, 204, 216).withAlpha(0.22f));
                g.drawText("FX ENGINE",
                           juce::Rectangle<float>(0.0f, labelY + labelH - 2, localBounds.getWidth(), 10.0f).toNearestInt(),
                           juce::Justification::centredTop, false);
            }

            // ── Preset name pill (Q1 — #1356) ────────────────────────────────
            // Dedicated pill below engine name. Shows current preset name (truncated)
            // or "—" when none loaded. Empty engine state handled in the !hasEngine_ branch.
            if (!isFx)
            {
                const float pillW = juce::jmin(localBounds.getWidth() - 8.0f, 80.0f);
                const float pillX = cx - pillW * 0.5f;
                const float pillY = labelY + labelH + 1.0f;
                const float pillH = 13.0f;
                const float pillR = 4.0f;

                // Pill background
                juce::Path pillPath;
                pillPath.addRoundedRectangle(pillX, pillY, pillW, pillH, pillR);
                g.setColour(accentColour_.withAlpha(0.10f));
                g.fillPath(pillPath);
                g.setColour(accentColour_.withAlpha(0.22f));
                g.strokePath(pillPath, juce::PathStrokeType(0.75f));

                // Preset name text
                const juce::String displayName = presetName_.isEmpty() ? juce::String(juce::CharPointer_UTF8("\xe2\x80\x94"))  // em dash
                                                                        : presetName_;
                g.setFont(GalleryFonts::label(8.0f));
                g.setColour(accentColour_.withAlpha(presetName_.isEmpty() ? 0.30f : 0.65f));
                g.drawText(displayName,
                           juce::Rectangle<float>(pillX + 3.0f, pillY, pillW - 6.0f, pillH).toNearestInt(),
                           juce::Justification::centredLeft, true);
            }
        }

        // ── Depth zone ring (thin colored outer ring) ─────────────────────
        {
            static constexpr juce::uint32 kDepthColors[] = {
                0x66FFDC78, // sunlit — warm gold
                0x6664A0DC, // twilight — cool blue
                0x663C5AA0, // midnight — deep blue
            };
            const int zoneIdx = juce::jlimit(0, 2, static_cast<int>(depthZone_));
            g.setColour(juce::Colour(kDepthColors[zoneIdx]));
            g.drawEllipse(cx - radius - 4.0f, cy - radius - 4.0f,
                          (radius + 4.0f) * 2.0f, (radius + 4.0f) * 2.0f, 2.0f);
        }

        // ── Slot number badge (top-left) ──────────────────────────────────
        if (!isFx && slotIndex_ >= 0)
        {
            const float badgeX = cx - radius * 0.7f;
            const float badgeY = cy - radius * 0.7f;
            const float badgeR = 10.0f;
            g.setColour(juce::Colour(14, 16, 22).withAlpha(0.82f));
            g.fillEllipse(badgeX - badgeR, badgeY - badgeR, badgeR * 2.0f, badgeR * 2.0f);
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.18f));
            g.drawEllipse(badgeX - badgeR, badgeY - badgeR, badgeR * 2.0f, badgeR * 2.0f, 1.0f);
            g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.0f).withStyle("Bold")));
            g.setColour(juce::Colour(200, 204, 216).withAlpha(0.75f));
            g.drawText(juce::String(slotIndex_ + 1),
                       juce::Rectangle<float>(badgeX - badgeR, badgeY - badgeR, badgeR * 2.0f, badgeR * 2.0f).toNearestInt(),
                       juce::Justification::centred, false);
        }

        // ── Selection pulsing ring (when selected) ────────────────────────
        if (selected_)
        {
            const float selPulse = 0.7f + 0.3f * std::sin(breathPhase_ * 4.0f);
            g.setColour(juce::Colour(127, 219, 202).withAlpha(0.4f * selPulse));
            g.drawEllipse(cx - radius - 6.0f, cy - radius - 6.0f,
                          (radius + 6.0f) * 2.0f, (radius + 6.0f) * 2.0f, 2.0f);
        }

        // ── Ripple rings (note-on feedback) ────────────────────────────────
        for (size_t ri = 0; ri < kMaxRipples; ++ri)
        {
            const auto& rip = ripples_[ri];
            if (rip.progress <= 0.0f || rip.progress >= 1.0f)
                continue;
            const float ripRadius = radius + rip.progress * 80.0f;
            const float ripAlpha  = (1.0f - rip.progress) * 0.4f;
            g.setColour(accentColour_.withAlpha(ripAlpha));
            g.drawEllipse(cx - ripRadius, cy - ripRadius,
                          ripRadius * 2.0f, ripRadius * 2.0f,
                          2.0f * (1.0f - rip.progress));
        }

        // ── Splash rings (FIX 22) — expanding rings when buoy first loads ──
        if (splashAnim_ > 0.01f)
        {
            float splashR1 = radius + (1.0f - splashAnim_) * 40.0f;
            g.setColour(juce::Colour(127, 219, 202).withAlpha(splashAnim_ * 0.4f));
            g.drawEllipse(cx - splashR1, cy - splashR1,
                          splashR1 * 2.0f, splashR1 * 2.0f,
                          2.0f * splashAnim_);
            float splashR2 = radius + (1.0f - splashAnim_) * 60.0f;
            g.setColour(juce::Colour(127, 219, 202).withAlpha(splashAnim_ * 0.12f));
            g.drawEllipse(cx - splashR2, cy - splashR2,
                          splashR2 * 2.0f, splashR2 * 2.0f, 1.0f);
        }

        // ── Mute / solo badges (FIX 3) ────────────────────────────────────
        // Mute badge: top-right of buoy, red circle with white "M"
        if (muted_)
        {
            const float mbX = cx + radius * 0.5f;
            const float mbY = cy - radius * 0.5f;
            constexpr float mbR = 8.0f;
            g.setColour(juce::Colour(239, 68, 68).withAlpha(0.9f));
            g.fillEllipse(mbX - mbR, mbY - mbR, mbR * 2.0f, mbR * 2.0f);
            g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f).withStyle("Bold")));
            g.setColour(juce::Colours::white);
            g.drawText("M",
                       juce::Rectangle<float>(mbX - mbR, mbY - mbR, mbR * 2.0f, mbR * 2.0f).toNearestInt(),
                       juce::Justification::centred, false);
        }

        // Solo badge: top-right (or top-left if muted occupies top-right),
        // gold circle with dark "S" + outer ring
        if (soloed_)
        {
            // Offset to top-left so it doesn't collide with the mute badge
            const float sbX = muted_ ? cx - radius * 0.5f : cx + radius * 0.5f;
            const float sbY = cy - radius * 0.5f;
            constexpr float sbR = 8.0f;
            // Outer gold ring
            g.setColour(juce::Colour(233, 196, 106).withAlpha(0.35f));
            g.drawEllipse(sbX - sbR - 2.0f, sbY - sbR - 2.0f,
                          (sbR + 2.0f) * 2.0f, (sbR + 2.0f) * 2.0f, 2.0f);
            // Fill
            g.setColour(juce::Colour(233, 196, 106).withAlpha(0.95f));
            g.fillEllipse(sbX - sbR, sbY - sbR, sbR * 2.0f, sbR * 2.0f);
            // "S" label
            g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f).withStyle("Bold")));
            g.setColour(juce::Colour(30, 20, 5));
            g.drawText("S",
                       juce::Rectangle<float>(sbX - sbR, sbY - sbR, sbR * 2.0f, sbR * 2.0f).toNearestInt(),
                       juce::Justification::centred, false);
        }

        // ── Dim overlay (mute/solo alpha dimming — FIX 9) ─────────────────
        // When a sibling buoy is soloed (dimAlpha_ < 1), darken this buoy by
        // drawing a semi-transparent Ocean::abyss rectangle on top.
        // Muted buoys receive alpha 0.30 → overlay at 70% opacity.
        // Non-soloed siblings receive alpha 0.50 → overlay at 50% opacity.
        if (dimAlpha_ < 0.99f)
        {
            g.setColour(juce::Colour(0xFF0A0E18).withAlpha(1.0f - dimAlpha_));
            g.fillRect(getLocalBounds());
        }

        // ── Accessibility focus ring ───────────────────────────────────────
        if (hasKeyboardFocus(false))
            A11y::drawCircularFocusRing(g, cx, cy, radius + kBorderWidth + 2.0f);
    }

    void resized() override {}

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!hasEngine_) return;
        isDragging_ = true;
        dragStartPos_ = e.position;

        // Spring physics: record anchor in parent coords, accounting for any
        // pre-existing offset so grabbing a settling orbit doesn't snap to home.
        auto* parent = getParentComponent();
        if (parent)
        {
            auto parentPos = e.getEventRelativeTo(parent).position;
            dragAnchorParent_ = parentPos - springOffset_;
            lastDragParentPos_ = parentPos;
            prevDragParentPos_ = parentPos;
        }

        inputState_ = InputState::UserDragging;
        springVelocity_ = {};
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!isDragging_ || !hasEngine_) return;
        auto* parent = getParentComponent();
        if (!parent) return;

        auto parentPos = e.getEventRelativeTo(parent).position;

        // Track mouse positions for velocity estimation on release
        prevDragParentPos_ = lastDragParentPos_;
        lastDragParentPos_ = parentPos;

        // Use setBounds (via onPositionChanged) during drag — proven to keep
        // coupling curves attached.  setTransform is used only for post-release
        // spring settle where there are no coupling curve endpoints to track.
        const auto area = oceanAreaBounds_.isEmpty()
            ? parent->getLocalBounds().toFloat()
            : oceanAreaBounds_;

        normalizedPosition_.x = juce::jlimit(0.08f, 0.92f,
            (parentPos.x - area.getX()) / area.getWidth());
        normalizedPosition_.y = juce::jlimit(0.08f, 0.85f,
            (parentPos.y - area.getY()) / area.getHeight());

        if (onPositionChanged)
            onPositionChanged(slotIndex_);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        isDragging_ = false;

        if (e.getDistanceFromDragStart() < 8)
        {
            // Click, not drag — clear any micro-offset and fire handler.
            inputState_ = InputState::Idle;
            springOffset_ = {};
            springVelocity_ = {};
            setTransform({});

            // Q1 (#1356): Check whether the click landed on the preset pill.
            // Pill is only rendered for engine buoys (not FX), and only when an engine is loaded.
            const bool isEngineBuoy = (buoyType_ == BuoyType::Engine);
            if (isEngineBuoy && onPresetPillClicked)
            {
                const auto pillBounds = getPresetPillBounds();
                if (pillBounds.contains(e.position.toInt()))
                {
                    onPresetPillClicked(slotIndex_);
                    return;
                }
            }

            if (onClicked) onClicked(slotIndex_);
        }
        else
        {
            // Drag release: home is already at drop position (setBounds was
            // called continuously during drag via onPositionChanged).
            // Add a brief spring settle with momentum for organic feel.
            springOffset_ = {};
            setTransform({});
            inputState_ = InputState::Settling;
            springVelocity_ = (lastDragParentPos_ - prevDragParentPos_) * 15.0f;

            constexpr float kMaxVel = 400.0f;
            springVelocity_.x = juce::jlimit(-kMaxVel, kMaxVel, springVelocity_.x);
            springVelocity_.y = juce::jlimit(-kMaxVel, kMaxVel, springVelocity_.y);
        }
    }

    void mouseEnter(const juce::MouseEvent& /*e*/) override
    {
        if (!hasEngine_) return;
        isHovered_ = true;
    }

    void mouseExit(const juce::MouseEvent& /*e*/) override
    {
        isHovered_ = false;
    }

    void mouseDoubleClick(const juce::MouseEvent& /*e*/) override
    {
        if (onDoubleClicked)
            onDoubleClicked(slotIndex_);
    }

    //==========================================================================
    // Engine data
    //==========================================================================

    void setEngine(const juce::String& engineId, juce::Colour accent, DepthZone zone)
    {
        // Guard: the editor calls this every timer tick.  Only run the
        // full setup (splash animation, randomization) on a real change.
        if (hasEngine_ && engineId_ == engineId)
        {
            // Update accent/zone in case they changed, but skip splash.
            accentColour_ = accent;
            depthZone_    = zone;
            return;
        }

        engineId_     = engineId;
        accentColour_ = accent;
        depthZone_    = zone;
        hasEngine_    = true;

        // Restore full mouse interactivity when an engine is loaded.
        setInterceptsMouseClicks(true, true);

        setTitle("Engine: " + engineId);
        setDescription("Depth zone: " + depthZoneName(zone) + ". Double-click to edit.");
        setTooltip(getEngineTagline(engineId));  // Wave 9b: evocative buoy tooltips

        // Randomize wreath harmonics for per-engine variety
        wreathHarmonics_ = 6 + (juce::Random::getSystemRandom().nextInt(6));

        // Randomize tidepool phase so each buoy drifts independently
        auto& rng = juce::Random::getSystemRandom();
        bobPhase_ = rng.nextFloat() * juce::MathConstants<float>::twoPi;

        // FIX 22: trigger drop splash + bounce-in on load
        splashAnim_   = 1.0f;
        bounceOffset_ = -30.0f;

        repaint();
    }

    void clearEngine()
    {
        hasEngine_   = false;
        engineId_    = {};
        setTitle("Empty engine slot");
        setDescription({});

        // Ghost slots are visual only — pass mouse events through so the
        // lifesaver overlay and ocean background can receive them.
        setInterceptsMouseClicks(false, false);

        breathPhase_      = 0.0f;
        wreathPhase_      = 0.0f;
        wreathFlare_      = 0.0f;
        wreathIntensity_  = 0.0f;
        activityLevel_    = 0.0f;
        lateralOffset_    = 0.0f;
        playSurfaceVisible_ = false;
        rippleWriteIdx_   = 0;
        for (auto& r : ripples_) r.progress = 0.0f;

        repaint();
    }

    /** Returns a one-line evocative tagline for the tooltip of each engine buoy.
        Format: "{EngineName} — {identity}". Falls back to engineId if not catalogued.
        Source: seance verdicts + CLAUDE.md engine identity cards.
        Wave 9b: all 93 engines complete. */
    static juce::String getEngineTagline(const juce::String& engineId)
    {
        static const std::unordered_map<std::string, std::string> kTaglines {
            { "Odyssey",    "Odyssey — warm VA polysynth. The drifter." },
            { "OddfeliX",   "OddfeliX — feliX the neon tetra. Glitch-phase character synth." },
            { "OddOscar",   "OddOscar — Oscar the axolotl. Morph-spectrum oscillator." },
            { "Oxbow",      "Oxbow — Oscar-pole resonator. The spine of deep water." },
            { "Opaline",    "Opaline — prepared piano. Bloom over long time." },
            { "Overwash",   "Overwash — tide breath layer. Felt, not heard." },
            { "Onset",      "Onset — percussive transient sculptor. Attack IS the sound." },
            { "Oxytocin",   "Oxytocin — circuit-love synthesis. Fleet leader." },
            { "Ouroboros",  "Ouroboros — strange-attractor feedback. Eats itself." },
            { "Organism",   "Organism — cellular automata synthesis. Life as DSP." },
            { "Origami",    "Origami — spectral fold synthesis. Each crease adds a harmonic." },
            { "Obscura",    "Obscura — physical string model. Daguerreotype resonance." },
            { "Oware",      "Oware — Akan tuned percussion. Goldweight rhythms." },
            { "Opera",      "Opera — additive-vocal Kuramoto. Aria through coupling." },
            { "Offering",   "Offering — boom bap drums. The ritual of the loop." },
            { "Optic",      "Optic — pulse-code synthesis. Light as waveform." },
            { "Ostinato",   "Ostinato — firelight repetition engine. Rhythm builds heat." },
            { "Oceanic",    "Oceanic — phosphorescent tidal pad. The surface itself." },
            { "Oblique",    "Oblique — prism spectrum. Color bent through angle." },
            { "Orca",       "Orca — ring-mod hunt engine. Predator frequency." },
            { "Organon",    "Organon — variational metabolism engine. Life finds its shape." },
            { "Obsidian",   "Obsidian — crystal-shard granulator. Dark geometric light." },
            { "Oracle",     "Oracle — GENDY stochastic oracle. Shapes from chaos." },
            { "Orbital",    "Orbital — grouped harmonic envelope. The constellation rises." },
            { "Obese",      "Obese — mojo-control aliasing machine. Fat by design." },
            { "Overbite",   "Overbite — five-fang percussion beast. Bite everything." },
            { "Overworld",  "Overworld — ERA timbral crossfade. Three dimensions of tone." },
            { "Opal",       "Opal — grain-shift cloudscape. Opalescence through chaos." },
            { "Obrix",      "Obrix — reef jade modulation grid. Coupling knots unbound." },
            { "Orbweave",   "Orbweave — knot-phase topography. Trefoil tangles sound." },
            { "Osprey",     "Osprey — shore-resonance bird call. Coastline memory." },
            { "Osteria",    "Osteria — wine-dark modal resonator. Cellar harmonics." },
            { "Owlfish",    "Owlfish — Mixtur-Trautonium hybrid. Deep-water alien tones." },
            { "Ohm",        "Ohm — sage modulation source. Resistance IS intention." },
            { "Orphica",    "Orphica — siren seafoam pluck. Lyre from the abyss." },
            { "Obbligato",  "Obbligato — breath-stacked voices. Counterpoint lives." },
            { "Ottoni",     "Ottoni — patina brass synth. Time melts the metal." },
            { "Ole",        "Ole — hibiscus rhythm cascade. Dance through the spectrum." },
            { "Overlap",    "Overlap — knotted cross-coupling engine. Tangle breeds tone." },
            { "Outwit",     "Outwit — chromatophore amber morph. Camouflage through timbre." },
            { "Ombre",      "Ombre — shadow-mauve drift machine. Darkness has texture." },
            { "Octopus",    "Octopus — eight-armed chromatophore synth. Each arm a voice." },
            { "Opensky",    "Opensky — Shepard shimmer tower. Forever ascending light." },
            { "Ouie",       "Ouie — hammerhead interval axis. Love and strife collide." },
            { "Overdub",    "Overdub — spring-reverb echo chamber. Metallic splash memory." },
            { "Oblong",     "Oblong — amber-warm vowel filter. Burnished formant breath." },
            { "Overtone",   "Overtone — continued-fraction converger. π sings itself." },
            { "Oort",       "Oort — Markov cloud wanderer. Random with amnesia." },
            { "Opsin",      "Opsin — photon-flux Hebbian learner. Light teaches itself." },
            // Wave 9b batch 3 — final 43 engines
            { "Oaken",      "Oaken — age-deepened wood resonance. Ancient fiber sings." },
            { "Oasis",      "Oasis — spring well harmonic bloom. Water finds itself." },
            { "Obelisk",    "Obelisk — monolithic modal mass. Stone sustains forever." },
            { "Obiont",     "Obiont — evolutionary gene engine. Mutation breeds tone." },
            { "Observandum","Observandum — spectral witness bank. Each overtone observed." },
            { "OceanDeep",  "OceanDeep — trench pressure-synthesis. The deep IS alive." },
            { "Ocelot",     "Ocelot — spotted jungle oscillator. Wildcat frequency." },
            { "Ochre",      "Ochre — earth pigment synthesis. Clay resonance model." },
            { "Octant",     "Octant — navigational bearing engine. Course through space." },
            { "Octave",     "Octave — tonewheel organ circuit. Draw-bar warmth lives." },
            { "Oddfellow",  "Oddfellow — spectral-shift stranger. Odd harmony seeker." },
            { "Ogive",      "Ogive — scanned glass synthesis. Needle through crystal." },
            { "Ogre",       "Ogre — sub-bass monster engine. Rumble and resonance." },
            { "Okeanos",    "Okeanos — warm Titan ocean myth. Ancient water memory." },
            { "Olate",      "Olate — fretless glass slide. Portamento through water." },
            { "Oleg",       "Oleg — harmonic soul drive. Blues through tonewheel." },
            { "Ollotron",   "Ollotron — bank-switched oscillator. Each voice a drawer." },
            { "Olvido",     "Olvido — spectral erosion engine. The forgetting deepens." },
            { "Omega",      "Omega — synth bass apex engine. Where all currents meet." },
            { "Onda",       "Onda — bound-state wave engine. Trapped vibration sings." },
            { "Ondine",     "Ondine — water sprite drift engine. Liquid phase wander." },
            { "Onkolo",     "Onkolo — drum core resonance. Stick strikes soul." },
            { "Oobleck",    "Oobleck — non-Newtonian feedback. Stiff when disturbed." },
            { "Ooze",       "Ooze — Reynolds viscosity synth. Flow through drag." },
            { "Opcode",     "Opcode — algorithmic depth engine. Code becomes tone." },
            { "OpenSky",    "OpenSky — Shepard sky tower. Rise without ceiling." },
            { "Orchard",    "Orchard — bow-pressure string synth. Fruit ripens music." },
            { "Orrery",     "Orrery — planetary motion engine. Orbit through sound." },
            { "Ortolan",    "Ortolan — song-phase bird memory. Melody from plumage." },
            { "Osier",      "Osier — wind through willow. Reed-weave resonance." },
            { "Osmosis",    "Osmosis — permeability diffusion. Through membrane tone flows." },
            { "Ostracon",   "Ostracon — tape buffer memory. Mellotron-spirit recall." },
            { "Otis",       "Otis — soul drive tonewheel. Righteous overdrive." },
            { "Oto",        "Oto — drawbar organ classic. Rotary speaker dance." },
            { "Outcrop",    "Outcrop — terrain-type stone. Ridge synthesis model." },
            { "Outflow",    "Outflow — current-speed river. Flow becomes frequency." },
            { "Outlook",    "Outlook — horizon-scan engine. See beyond the wave." },
            { "Oven",       "Oven — hammer-bright piano. Heat in every strike." },
            { "Overcast",   "Overcast — cloud-dense pad. Gray diffusion synthesis." },
            { "Overflow",   "Overflow — current-surge torrent. Spillover tone." },
            { "Overgrow",   "Overgrow — growth-rate garden. Vines devour octaves." },
            { "Overtide",   "Overtide — tidal depth surge. Rising water frequency." },
            { "Overworn",   "Overworn — felt-age decay engine. Warmth from wear." },
            { "Oxalis",     "Oxalis — leaf-tension spring. Wood sorrel resonance." },
            { "Oxidize",    "Oxidize — age-rate rust engine. Patina builds timbre." },
        };

        auto it = kTaglines.find(engineId.toStdString());
        if (it != kTaglines.end())
            return juce::String(it->second);

        // Fallback: engine name only (all 93 engines catalogued)
        return engineId;
    }

    bool          hasEngine()       const noexcept { return hasEngine_; }
    juce::String  getEngineId()     const noexcept { return engineId_; }
    juce::Colour  getAccentColour() const noexcept { return accentColour_; }
    DepthZone     getDepthZone()    const noexcept { return depthZone_; }

    //==========================================================================
    // Buoy type
    //==========================================================================

    void setBuoyType(BuoyType type) { buoyType_ = type; }
    BuoyType getBuoyType() const noexcept { return buoyType_; }

    float getBuoyRadius() const noexcept
    {
        switch (buoyType_)
        {
            case BuoyType::MasterFx: return 38.0f;
            case BuoyType::FxEngine: return 26.0f;
            default:                 return kOrbitalSize * 0.5f; // 36
        }
    }

    int getBuoySize() const noexcept
    {
        return static_cast<int>(getBuoyRadius() * 2.0f);
    }

    //==========================================================================
    // Live state updates
    //==========================================================================

    void setVoiceCount(int count) { voiceCount_ = count; }
    void setCouplingLean(float lean) { couplingLean_ = juce::jlimit(-1.0f, 1.0f, lean); }
    void setSlotIndex(int index) { slotIndex_ = index; }
    void setSelected(bool sel) { selected_ = sel; repaint(); }
    /// Set the ocean area bounds (in parent coordinates) so drag normalization is correct.
    void setOceanAreaBounds(juce::Rectangle<float> area) { oceanAreaBounds_ = area; }
    void setPlaySurfaceVisible(bool visible) { playSurfaceVisible_ = visible; }

    //==========================================================================
    // Waveform wreath data (pushed from OceanView timer reading WaveformFifo)
    //==========================================================================

    void setWreathData(const float* samples, int count, float rms)
    {
        // F2-021: Wreath data must be pushed from the message thread only —
        // it is read by paint() on the same thread without any synchronisation.
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

        const int n = std::min(count, kWreathBufferSize);
        for (int i = 0; i < n; ++i)
            wreathBuffer_[static_cast<size_t>(i)] = samples[i];
        // Noise gate: below -34 dB (~0.02 RMS) is silence.
        // Smooth toward target to prevent frame-to-frame jitter.
        const float target = rms > 0.02f
            ? juce::jlimit(0.0f, 1.0f, rms * 4.0f) : 0.0f;
        wreathIntensity_ += (target - wreathIntensity_) * 0.15f;
        if (wreathIntensity_ < 0.005f) wreathIntensity_ = 0.0f;
    }

    void setWreathShape(WreathShape s) { wreathShape_ = s; }

    //==========================================================================
    // Mute / solo state
    //==========================================================================

    void setMuted(bool m)  { muted_  = m; repaint(); }
    void setSoloed(bool s) { soloed_ = s; repaint(); }

    bool isMuted()  const noexcept { return muted_;  }
    bool isSoloed() const noexcept { return soloed_; }

    //==========================================================================
    // Dim alpha — driven by OceanView when any sibling buoy is soloed.
    // soloed sibling: set to 0.50f; muted: set to 0.30f; normal: 1.0f.
    //==========================================================================

    void setDimAlpha(float a)
    {
        const float clamped = juce::jlimit(0.0f, 1.0f, a);
        if (std::abs(clamped - dimAlpha_) > 0.005f)
        {
            dimAlpha_ = clamped;
            repaint();
        }
    }

    float getDimAlpha() const noexcept { return dimAlpha_; }

    //==========================================================================
    // Ripple + flare triggers (called from UI timer on note-on events)
    //==========================================================================

    void triggerRipple()
    {
        ripples_[rippleWriteIdx_].progress = 0.001f; // start
        rippleWriteIdx_ = (rippleWriteIdx_ + 1) % kMaxRipples;
        wreathFlare_ = 1.0f;
    }

    //==========================================================================
    // Freeform positioning
    //==========================================================================

    juce::Point<float> getNormalizedPosition() const noexcept { return normalizedPosition_; }

    void setNormalizedPosition(juce::Point<float> pos)
    {
        normalizedPosition_ = pos;
    }

    //==========================================================================
    // Spring physics (VQ 015 Step 4)
    //==========================================================================

    InputState getInputState() const noexcept { return inputState_; }
    juce::Point<float> getSpringOffset() const noexcept { return springOffset_; }

    /** Centre of the buoy including any spring displacement (for coupling curves). */
    juce::Point<float> getVisualCenter() const
    {
        auto c = getBounds().toFloat().getCentre();
        return { c.x + springOffset_.x, c.y + springOffset_.y };
    }

    /** Kill any in-flight spring animation (call on view-state transitions). */
    void resetSpring()
    {
        inputState_ = InputState::Idle;
        springOffset_ = {};
        springVelocity_ = {};
        setTransform({});
    }

    juce::Point<float> getCenter() const
    {
        return getBounds().toFloat().getCentre();
    }

    //==========================================================================
    // Preset pill (Q1 — #1356)
    //==========================================================================

    /** Set the preset name shown in the pill below the engine name.
        Pass an empty string to display "—". */
    void setPresetName(const juce::String& name)
    {
        if (presetName_ == name) return;
        presetName_ = name;
        repaint();
    }

    juce::String getPresetName() const noexcept { return presetName_; }

    /** Returns the screen-space bounds of the preset pill, for attaching a CallOutBox. */
    juce::Rectangle<int> getPresetPillBounds() const
    {
        const auto localBounds = getLocalBounds().toFloat();
        const float cx = localBounds.getCentreX();
        const float cy = localBounds.getCentreY();
        const float radius = getBuoyRadius();
        const float labelH = kNameFontSize + 4.0f;
        const float pillY = cy + radius + 3.0f + labelH + 1.0f;
        const float pillH = 13.0f;
        const float pillW = juce::jmin(localBounds.getWidth() - 8.0f, 80.0f);
        const float pillX = cx - pillW * 0.5f;
        return juce::Rectangle<float>(pillX, pillY, pillW, pillH).toNearestInt();
    }

    //==========================================================================
    // Callbacks
    //==========================================================================

    std::function<void(int slotIndex)> onClicked;
    std::function<void(int slotIndex)> onDoubleClicked;
    std::function<void(int slotIndex)> onPositionChanged;
    std::function<void(int slotIndex)> onDragMoved;  ///< visual pos changed during drag
    /** Fired when the user clicks the preset pill. Slot has an engine loaded. */
    std::function<void(int slotIndex)> onPresetPillClicked;

    //==========================================================================
    // Animation — called by OceanView's single shared timer at 30 Hz
    //==========================================================================

    /** Advance all animation state by one 30 Hz tick. Called by OceanView. */
    void stepAnimation()
    {
        if (!hasEngine_) return;

        // F2-008: Recover from system-yanked mouse capture (Alt+Tab, Cmd+Tab, window focus loss).
        // JUCE has no mouseLostCapture() virtual — poll in the animation tick instead.
        // If we believe a drag is in progress but no mouse button is down, cancel it.
        if (isDragging_ && !juce::Desktop::getInstance().getMainMouseSource().isDragging())
        {
            isDragging_ = false;
            inputState_ = InputState::Settling;
        }

        const bool reducedMotion = A11y::prefersReducedMotion();
        constexpr float twoPi = juce::MathConstants<float>::twoPi;

        // ── Activity level — driven by actual audio energy only ─────────────
        // Voice count is unreliable at idle (engines report active voices for
        // LFOs, feedback tails, etc.).  Audio RMS = true playing intensity.
        const float rawActivity = wreathIntensity_;
        activityLevel_ += (rawActivity - activityLevel_) * 0.02f; // ~1.7s smoothing
        if (activityLevel_ < 0.005f) activityLevel_ = 0.0f;      // hard floor

        // ── Breath phase advance ────────────────────────────────────────────
        if (!reducedMotion)
        {
            breathRate_ = 0.15f + activityLevel_ * 0.35f;
            breathPhase_ += breathRate_ / 30.0f * twoPi;
            if (breathPhase_ >= twoPi)
                breathPhase_ -= twoPi;
        }
        else
        {
            breathPhase_ = 0.0f;
        }

        // ── Hover breathing (#12) — 1 Hz gentle scale ±3% on hover ─────────
        if (!reducedMotion)
        {
            const float hoverTarget = isHovered_ ? 1.0f : 0.0f;
            hoverBreathLevel_ += (hoverTarget - hoverBreathLevel_) * 0.20f;
            if (hoverBreathLevel_ < 0.005f) hoverBreathLevel_ = 0.0f;
            if (hoverBreathLevel_ > 0.005f)
            {
                hoverBreathPhase_ += (1.0f / 30.0f) * juce::MathConstants<float>::twoPi;
                if (hoverBreathPhase_ >= juce::MathConstants<float>::twoPi)
                    hoverBreathPhase_ -= juce::MathConstants<float>::twoPi;
            }
        }
        else
        {
            hoverBreathLevel_ = 0.0f;
            hoverBreathPhase_ = 0.0f;
        }

        // ── Wreath phase advance ────────────────────────────────────────────
        // Only spin the wreath ring when there's meaningful audio.
        // Threshold high enough to ignore noise floor.
        if (!reducedMotion && wreathIntensity_ > 0.05f)
        {
            wreathPhase_ += 1.8f / 30.0f * wreathIntensity_;
            if (wreathPhase_ > 100.0f) wreathPhase_ -= 100.0f;
        }

        // ── Wreath flare decay ──────────────────────────────────────────────
        if (wreathFlare_ > 0.01f)
            wreathFlare_ *= 0.94f;
        else
            wreathFlare_ = 0.0f;

        // ── Tidepool dynamics — multi-frequency float (VQ 015) ─────────────
        // Two independent slow phases, per-engine bobPhase_ as offset,
        // so each buoy drifts at its own rhythm.  ALL motion amplitude is
        // proportional to activity — true stillness at idle, motion only
        // emerges when the engine is producing sound.
        if (!reducedMotion)
        {
            // Phase advance: keep ticking even at idle so motion resumes
            // smoothly — it's the amplitude that gates visibility, not phase.
            tidePhaseSlow_ += (0.012f + activityLevel_ * 0.008f) * twoPi / 30.0f;
            tidePhaseMed_  += (0.03f  + activityLevel_ * 0.02f)  * twoPi / 30.0f;
            if (tidePhaseSlow_ > twoPi) tidePhaseSlow_ -= twoPi;
            if (tidePhaseMed_  > twoPi) tidePhaseMed_  -= twoPi;

            // Y-axis: zero at idle → 3.5px at full activity
            const float ampY = activityLevel_ * 3.5f;
            bobOffset_ = std::sin(tidePhaseSlow_ + bobPhase_) * ampY
                       + std::sin(tidePhaseMed_ + bobPhase_ * 1.7f) * ampY * 0.35f;

            // X-axis: zero at idle → 2px at full activity
            const float ampX = activityLevel_ * 2.0f;
            lateralOffset_ = std::sin(tidePhaseSlow_ * 0.7f + bobPhase_ * 1.3f) * ampX
                           + std::sin(tidePhaseMed_ * 1.2f + bobPhase_ * 0.8f) * ampX * 0.3f;

            // Tilt: zero at idle, proportional to activity
            tiltAngle_ = activityLevel_ * std::sin(tidePhaseMed_ + bobPhase_) * 0.015f;

            // Dead zone: snap to zero below 0.5px to prevent sub-pixel
            // antialiasing shimmer on retina displays.
            if (std::abs(bobOffset_) < 0.5f)     bobOffset_     = 0.0f;
            if (std::abs(lateralOffset_) < 0.5f)  lateralOffset_ = 0.0f;
            if (std::abs(tiltAngle_) < 0.001f)    tiltAngle_     = 0.0f;
        }
        else
        {
            bobOffset_     = 0.0f;
            lateralOffset_ = 0.0f;
            tiltAngle_     = 0.0f;
        }

        // ── Ripple advance ──────────────────────────────────────────────────
        for (auto& rip : ripples_)
        {
            if (rip.progress > 0.0f && rip.progress < 1.0f)
                rip.progress += 0.016f; // ~0.5s total at 30Hz
            if (rip.progress >= 1.0f)
                rip.progress = 0.0f;
        }

        // ── Splash animation decay (FIX 22) ────────────────────────────────
        if (splashAnim_ > 0.01f)
        {
            splashAnim_   *= 0.94f;
            bounceOffset_ *= 0.88f;
        }
        else
        {
            splashAnim_   = 0.0f;
            bounceOffset_ = 0.0f;
        }

        // ── Spring physics — damped harmonic oscillator (VQ 015 Step 4) ────
        // When Settling, drive springOffset_ toward zero (home position) with
        // momentum from drag release.  Slightly underdamped for organic bounce.
        if (inputState_ == InputState::Settling)
        {
            constexpr float dt        = 1.0f / 30.0f;
            constexpr float stiffness = 300.0f;   // spring constant (N/m equiv)
            constexpr float damping   = 22.0f;    // < 2*sqrt(k) = slightly underdamped

            const float ax = -stiffness * springOffset_.x - damping * springVelocity_.x;
            const float ay = -stiffness * springOffset_.y - damping * springVelocity_.y;

            springVelocity_.x += ax * dt;
            springVelocity_.y += ay * dt;
            springOffset_.x   += springVelocity_.x * dt;
            springOffset_.y   += springVelocity_.y * dt;

            // Snap to home when displacement and velocity are negligible
            const float offsetSq = springOffset_.x * springOffset_.x
                                 + springOffset_.y * springOffset_.y;
            const float velSq    = springVelocity_.x * springVelocity_.x
                                 + springVelocity_.y * springVelocity_.y;

            if (offsetSq < 0.25f && velSq < 1.0f)
            {
                springOffset_   = {};
                springVelocity_ = {};
                inputState_     = InputState::Idle;
                setTransform({});
            }
            else
            {
                setTransform(juce::AffineTransform::translation(
                    springOffset_.x, springOffset_.y));
            }
        }

        // ── Image cache toggle ──────────────────────────────────────────────
        // When idle, cache paint() as a bitmap.  The CouplingSubstrate is a
        // transparent overlay that repaints 30x/sec — JUCE's compositor must
        // re-render everything behind it.  Without caching, orbits get their
        // 128-point wreath path reconstructed every frame, producing sub-pixel
        // antialiasing variance (the "shaking" at idle).  With caching, JUCE
        // blits the frozen bitmap — identical pixels every frame.
        const bool shouldCache = !isAnimating();
        if (shouldCache != bufferedToImage_)
        {
            bufferedToImage_ = shouldCache;
            setBufferedToImage(shouldCache);
        }

        // Repaint is triggered by OceanView after all orbits have been stepped.
    }

    /** Called by OceanView after stepping all orbits to trigger a repaint. */
    void requestRepaint() { repaint(); }

    /** True when this orbit has visual changes that need repainting.
        Idle orbits return false — stopping the 30Hz repaint eliminates
        sub-pixel antialiasing shimmer from the wreath path. */
    bool isAnimating() const noexcept
    {
        return activityLevel_    > 0.01f
            || wreathFlare_      > 0.01f
            || splashAnim_       > 0.01f
            || hoverBreathLevel_ > 0.005f
            || isHovered_
            || inputState_      != InputState::Idle;
    }

private:
    //==========================================================================
    /** Returns a -1..+1 sample for the given phase t based on wreathShape_. */
    float computeWreathSample(float t) const
    {
        switch (wreathShape_)
        {
            case WreathShape::Sine:
                return std::sin(t);
            case WreathShape::Saw:
                return 2.0f * std::fmod(t / juce::MathConstants<float>::twoPi, 1.0f) - 1.0f;
            case WreathShape::Square:
                return std::sin(t) > 0.0f ? 1.0f : -1.0f;
            case WreathShape::Tri:
                return 2.0f * std::abs(2.0f * std::fmod(t / juce::MathConstants<float>::twoPi, 1.0f) - 1.0f) - 1.0f;
            case WreathShape::Noise:
                return std::sin(t * 7.3f) * std::cos(t * 3.1f) + std::sin(t * 13.7f) * 0.5f;
            case WreathShape::Organ:
                return std::sin(t) * 0.6f + std::sin(t * 2.0f) * 0.25f + std::sin(t * 3.0f) * 0.15f;
            case WreathShape::Pulse:
                return std::sin(t) > 0.3f ? 1.0f : -1.0f;
            case WreathShape::Harmonic:
                return std::sin(t) + std::sin(t * 3.0f) * 0.4f + std::sin(t * 5.0f) * 0.2f;
            default:
                return std::sin(t);
        }
    }

    //==========================================================================
    static juce::String depthZoneName(DepthZone zone) noexcept
    {
        switch (zone)
        {
            case DepthZone::Sunlit:   return "Sunlit";
            case DepthZone::Twilight: return "Twilight";
            case DepthZone::Midnight: return "Midnight";
        }
        return {};
    }

    //==========================================================================
    // Engine state
    juce::String engineId_;
    juce::String presetName_;    ///< Current preset name for the pill (Q1 — #1356). Empty = "—".
    juce::Colour accentColour_   = juce::Colour(GalleryColors::xoGold);
    DepthZone    depthZone_      = DepthZone::Sunlit;
    BuoyType     buoyType_       = BuoyType::Engine;
    bool         hasEngine_      = false;
    bool         selected_       = false;
    int          slotIndex_      = 0;

    // Live audio state
    int   voiceCount_          = 0;
    float couplingLean_        = 0.0f;
    bool  playSurfaceVisible_  = false;

    // Freeform drag
    bool isDragging_ = false;
    // pendingClickActive_ / pendingClickSlot_ reserved for future single-click disambiguation
    juce::Point<float> dragStartPos_ {};
    juce::Point<float> normalizedPosition_ { 0.5f, 0.5f };
    juce::Rectangle<float> oceanAreaBounds_ {}; // parent ocean area for drag normalization

    // Spring physics (VQ 015 Step 4)
    InputState inputState_ = InputState::Idle;
    juce::Point<float> springOffset_ {};        ///< px displacement from home (setBounds) position
    juce::Point<float> springVelocity_ {};      ///< px/second
    juce::Point<float> dragAnchorParent_ {};    ///< parent-coord anchor for drag offset computation
    juce::Point<float> lastDragParentPos_ {};   ///< last frame mouse pos (parent coords)
    juce::Point<float> prevDragParentPos_ {};   ///< frame before last (for velocity estimation)

    // Animation
    float breathPhase_ = 0.0f;
    float breathRate_  = 0.2f;

    // Hover breathing (#12) — 1Hz gentle scale ±3%
    bool  isHovered_         = false;
    float hoverBreathLevel_  = 0.0f;
    float hoverBreathPhase_  = 0.0f;

    // Waveform wreath
    static constexpr int kWreathBufferSize = 128;
    std::array<float, kWreathBufferSize> wreathBuffer_ {};
    float      wreathPhase_     = 0.0f;
    float      wreathIntensity_ = 0.0f;
    float      wreathFlare_     = 0.0f;
    int        wreathHarmonics_ = 8;
    WreathShape wreathShape_    = WreathShape::Sine;

    // Tidepool dynamics (VQ 015) — replaces single-sine bob
    float bobPhase_      = 0.0f;   ///< random per-engine phase offset (set in setEngine)
    float bobOffset_     = 0.0f;   ///< current Y offset (px, from tidepool calc)
    float lateralOffset_ = 0.0f;   ///< current X offset (px, lateral sway)
    float tiltAngle_     = 0.0f;   ///< current tilt (radians)
    float activityLevel_ = 0.0f;   ///< smoothed 0..1 blend of voiceCount + audio RMS
    float tidePhaseSlow_ = 0.0f;   ///< primary swell phase (~80s idle cycle)
    float tidePhaseMed_  = 0.0f;   ///< secondary wave phase (~33s idle cycle)

    // Buoy drop splash animation (FIX 22)
    float splashAnim_   = 0.0f;  ///< 1.0 on load, decays to 0
    float bounceOffset_ = 0.0f;  ///< starts at -30px, eases to 0

    // Mute / solo (FIX 3)
    bool muted_  = false;
    bool soloed_ = false;

    // Dim alpha — set by OceanView when a sibling buoy is soloed (FIX 9)
    float dimAlpha_ = 1.0f;

    // Image cache state — tracks setBufferedToImage() calls
    bool bufferedToImage_ = false;

    // Ripple effects (fixed-size, no heap allocation — RAC finding F4)
    static constexpr size_t kMaxRipples = 8;
    struct Ripple { float progress = 0.0f; };
    std::array<Ripple, kMaxRipples> ripples_ {};
    size_t rippleWriteIdx_ = 0;

    // Constants
    static constexpr float kBorderWidth           = 2.0f;
    static constexpr float kBreathAmplitude       = 0.05f;  ///< ±5% activity-driven breath
    static constexpr float kHoverBreathAmplitude  = 0.03f;  ///< ±3% hover breath (#12)
    static constexpr float kNameFontSize          = 12.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineOrbit)
};

} // namespace xoceanus
