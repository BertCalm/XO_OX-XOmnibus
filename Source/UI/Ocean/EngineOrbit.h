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
#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
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

    //==========================================================================
    static constexpr float kOrbitalSize = 72.0f;   ///< Engine buoy diameter

    //==========================================================================
    EngineOrbit()
    {
        A11y::setup(*this, "Engine Buoy", "Engine slot. Double-click to edit.");
        setWantsKeyboardFocus(true);
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

            // Slot number below the circle
            if (slotIndex_ >= 0)
            {
                g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.0f)));
                g.setColour(ghostCol.withAlpha(0.25f));
                g.drawText("Slot " + juce::String(slotIndex_ + 1),
                           juce::Rectangle<float>(cx - 20.0f, cy + r + 4.0f, 40.0f, 10.0f).toNearestInt(),
                           juce::Justification::centred, false);
            }
            return;
        }

        const auto localBounds = getLocalBounds().toFloat();
        const float cx = localBounds.getCentreX();
        const float cy = localBounds.getCentreY();

        // ── Effective size with breath ──────────────────────────────────────
        const float breathAmplitude = A11y::prefersReducedMotion()
                                          ? 0.0f
                                          : kBreathAmplitude * std::sin(breathPhase_);
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
            // FIX 22: include bounceOffset_ so new buoys drop in from above
            g.addTransform(juce::AffineTransform::translation(0.0f, bobOffset_ + bounceOffset_));
            g.addTransform(juce::AffineTransform::rotation(tiltAngle_, cx, cy + bobOffset_ + bounceOffset_));
        }

        // ── Water reflection ellipse (subtle, below buoy) ──────────────────
        g.setColour(juce::Colour(60, 180, 170).withAlpha(0.04f));
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
                const float wreathAmp = 4.0f + wreathIntensity_ * 6.0f + wreathFlare_ * 12.0f;
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
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!isDragging_ || !hasEngine_) return;
        auto* parent = getParentComponent();
        if (!parent) return;

        auto parentPos = e.getEventRelativeTo(parent).position;

        // Normalize against the ocean area (not full OceanView which includes dashboard).
        // oceanAreaBounds_ is set by the parent via setOceanAreaBounds().
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
        // Click vs drag: only fire onClicked if minimal movement.
        // No delay needed — single-click selects in place (no orbit movement),
        // so double-click still hits the same component reliably.
        if (e.getDistanceFromDragStart() < 8 && onClicked)
            onClicked(slotIndex_);
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
        engineId_     = engineId;
        accentColour_ = accent;
        depthZone_    = zone;
        hasEngine_    = true;

        // Restore full mouse interactivity when an engine is loaded.
        setInterceptsMouseClicks(true, true);

        setTitle("Engine: " + engineId);
        setDescription("Depth zone: " + depthZoneName(zone) + ". Double-click to edit.");
        setTooltip(engineId);

        // Randomize wreath harmonics for per-engine variety
        wreathHarmonics_ = 6 + (juce::Random::getSystemRandom().nextInt(6));

        // Randomize bobbing parameters so each buoy floats at its own rhythm
        auto& rng = juce::Random::getSystemRandom();
        bobPhase_ = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        bobSpeed_ = 0.28f + rng.nextFloat() * 0.18f;
        bobAmp_   = 0.8f  + rng.nextFloat() * 0.7f;  // subtle idle bob (0.8-1.5px)

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
        playSurfaceVisible_ = false;
        rippleWriteIdx_   = 0;
        for (auto& r : ripples_) r.progress = 0.0f;

        repaint();
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
        const int n = std::min(count, kWreathBufferSize);
        for (int i = 0; i < n; ++i)
            wreathBuffer_[static_cast<size_t>(i)] = samples[i];
        wreathIntensity_ = juce::jlimit(0.0f, 1.0f, rms * 4.0f); // amplify RMS for visual effect
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

    juce::Point<float> getCenter() const
    {
        return getBounds().toFloat().getCentre();
    }

    //==========================================================================
    // Callbacks
    //==========================================================================

    std::function<void(int slotIndex)> onClicked;
    std::function<void(int slotIndex)> onDoubleClicked;
    std::function<void(int slotIndex)> onPositionChanged;

    //==========================================================================
    // Animation — called by OceanView's single shared timer at 30 Hz
    //==========================================================================

    /** Advance all animation state by one 30 Hz tick. Called by OceanView. */
    void stepAnimation()
    {
        if (!hasEngine_) return;

        const bool reducedMotion = A11y::prefersReducedMotion();

        // ── Breath phase advance ────────────────────────────────────────────
        if (!reducedMotion)
        {
            const float voiceRamp = juce::jlimit(0.0f, 1.0f,
                                                  static_cast<float>(voiceCount_) / 4.0f);
            breathRate_ = 0.2f + voiceRamp * 0.3f;
            breathPhase_ += breathRate_ / 30.0f * juce::MathConstants<float>::twoPi;
            if (breathPhase_ >= juce::MathConstants<float>::twoPi)
                breathPhase_ -= juce::MathConstants<float>::twoPi;
        }
        else
        {
            breathPhase_ = 0.0f;
        }

        // ── Wreath phase advance ────────────────────────────────────────────
        if (!reducedMotion)
        {
            wreathPhase_ += 1.8f / 30.0f;
            if (wreathPhase_ > 100.0f) wreathPhase_ -= 100.0f; // prevent float overflow
        }

        // ── Wreath flare decay ──────────────────────────────────────────────
        if (wreathFlare_ > 0.01f)
            wreathFlare_ *= 0.94f;
        else
            wreathFlare_ = 0.0f;

        // ── Bob + tilt animation (FIX 2) ───────────────────────────────────
        if (!reducedMotion)
        {
            bobOffset_  = std::sin(breathPhase_ * bobSpeed_ + bobPhase_)
                          * (bobAmp_ + wreathIntensity_ * 4.0f);
            tiltAngle_  = std::sin(breathPhase_ * 0.7f)
                          * 0.012f * (1.0f + wreathIntensity_ * 3.0f);
        }
        else
        {
            bobOffset_ = 0.0f;
            tiltAngle_ = 0.0f;
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
        // Repaint is triggered by OceanView after all orbits have been stepped.
    }

    /** Called by OceanView after stepping all orbits to trigger a repaint. */
    void requestRepaint() { repaint(); }

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
    bool pendingClickActive_ = false;   ///< true while waiting to fire single-click
    int  pendingClickSlot_   = -1;      ///< slot index for pending click
    juce::Point<float> dragStartPos_ {};
    juce::Point<float> normalizedPosition_ { 0.5f, 0.5f };
    juce::Rectangle<float> oceanAreaBounds_ {}; // parent ocean area for drag normalization

    // Animation
    float breathPhase_ = 0.0f;
    float breathRate_  = 0.2f;

    // Waveform wreath
    static constexpr int kWreathBufferSize = 128;
    std::array<float, kWreathBufferSize> wreathBuffer_ {};
    float      wreathPhase_     = 0.0f;
    float      wreathIntensity_ = 0.0f;
    float      wreathFlare_     = 0.0f;
    int        wreathHarmonics_ = 8;
    WreathShape wreathShape_    = WreathShape::Sine;

    // Bobbing / tilt animation (FIX 2)
    float bobPhase_  = 0.0f;   ///< random initial phase, set in setEngine()
    float bobSpeed_  = 0.28f;  ///< cycles-per-breath-cycle; randomized in setEngine()
    float bobAmp_    = 2.0f;   ///< pixels amplitude; randomized in setEngine()
    float bobOffset_ = 0.0f;   ///< current computed Y offset (updated by stepAnimation)
    float tiltAngle_ = 0.0f;   ///< current tilt radians (updated by stepAnimation)

    // Buoy drop splash animation (FIX 22)
    float splashAnim_   = 0.0f;  ///< 1.0 on load, decays to 0
    float bounceOffset_ = 0.0f;  ///< starts at -30px, eases to 0

    // Mute / solo (FIX 3)
    bool muted_  = false;
    bool soloed_ = false;

    // Dim alpha — set by OceanView when a sibling buoy is soloed (FIX 9)
    float dimAlpha_ = 1.0f;

    // Ripple effects (fixed-size, no heap allocation — RAC finding F4)
    static constexpr size_t kMaxRipples = 8;
    struct Ripple { float progress = 0.0f; };
    std::array<Ripple, kMaxRipples> ripples_ {};
    size_t rippleWriteIdx_ = 0;

    // Constants
    static constexpr float kBorderWidth      = 2.0f;
    static constexpr float kBreathAmplitude  = 0.05f;
    static constexpr float kNameFontSize     = 12.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineOrbit)
};

} // namespace xoceanus
