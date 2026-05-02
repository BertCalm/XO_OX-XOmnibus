// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOceanusProcessor.h"
#include "../../Core/EngineRegistry.h"
#include "../GalleryColors.h"
#include "WaveformDisplay.h"
#include "EnginePickerPopup.h"
#include "CockpitHost.h"
#include "CreatureRenderer.h"

namespace xoceanus
{

//==============================================================================
// CompactEngineTile — slim tile in the left Column A (260pt wide).
//
// Layout (top to bottom, inside 9/11/8/14 px padding):
//   1. Left accent bar (3px wide, full tile height, flush x=0)
//   2. Header row: slot number (12px wide, 9px mono, T3) | engine name (flex, 11px bold, accent) | power button (16×16)
//   3. Mini macro knobs row: 4 arcs (SRC1/FILT/ENV/FX), ~20px wide each, 5px gap
//   4. Mini waveform: 22px tall, painted directly in paint() using accent color polyline
//   5. CPU bar: 3px tall, full content width, accent at 0.55 alpha
//
// NOTE: Mood dots, FX indicator, and footer row were removed in the v05 polish pass.
// Redesigned 2026-03-27: Removed porthole circle and creature renderer.
// Power button replaces mute toggle visually (same underlying isMuted state).
class CompactEngineTile : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    std::function<void(int)> onSelect; // called with slot index when clicked

    CompactEngineTile(XOceanusProcessor& proc, int slotIndex) : processor(proc), slot(slotIndex), miniWave(proc)
    {
        A11y::setup(*this, "Engine Slot " + juce::String(slotIndex + 1),
                    "Click to open engine detail, right-click for options");
        setExplicitFocusOrder(slotIndex + 1);
        // miniWave reads live audio from WaveformFifo at 30Hz (or 10Hz in
        // reduced-motion mode) and draws the real engine output.
        addAndMakeVisible(miniWave); // live waveform — bounds set in resized()
        macroValues.fill(0.0f);

        // P10 fix: cache parameter ID strings once in constructor to avoid
        // 4 juce::String allocations per tile per 10Hz tick.
        for (int m = 0; m < 4; ++m)
            cachedMacroIds[m] = "macro" + juce::String(m + 1);

        refresh();
        startTimerHz(10); // poll voice count + macros + coupling at 10Hz
    }

    ~CompactEngineTile() override { stopTimer(); }

    void refresh()
    {
        auto* eng = processor.getEngine(slot);
        bool newHasEngine = (eng != nullptr);
        juce::String newId = newHasEngine ? eng->getEngineId() : juce::String{};

        // Only repaint when state actually changed — avoids idle repaint overhead.
        if (!isLoading && newHasEngine == hasEngine && newId == engineId)
            return;

        isLoading = false; // engine arrived — clear loading state
        hasEngine = newHasEngine;
        engineId = newId;
        if (hasEngine)
        {
            auto arch = archetypeOf(engineId);
            auto tipText = arch.isEmpty() ? engineId.toUpperCase()
                                          : engineId.toUpperCase() + " \xe2\x80\x94 " + arch;
            setTooltip(tipText);
        }
        else
        {
            setTooltip("Slot " + juce::String(slot + 1) + juce::String(juce::CharPointer_UTF8(": empty \xe2\x80\x94 click to load engine")));
        }
        accent = hasEngine ? eng->getAccentColour() : GalleryColors::get(GalleryColors::emptySlot());
        miniWave.setSlot(slot);
        if (eng)
            miniWave.setAccentColour(eng->getAccentColour());
        // Sync mute state from processor so tile visual matches processor state
        // after preset load, session restore, or any external state change.
        isMuted = processor.isSlotMuted(slot);
        repaint();
    }

    void timerCallback() override
    {
        bool needsRepaint = false;

        // ── Voice count ────────────────────────────────────────────────────────
        auto* eng = processor.getEngine(slot);

        // Ghost/empty slot: only check if an engine appeared, skip all polling (#188)
        if (!eng)
        {
            if (hasEngine)
            {
                refresh();
            }
            return;
        }

        int newCount = eng->getActiveVoiceCount();
        if (newCount != voiceCount)
        {
            voiceCount = newCount;
            needsRepaint = true;
        }

        // ── Breathing phase — #932 ─────────────────────────────────────────
        // Advance 0.5 Hz sine at 10 Hz timer tick: Δphase = 2π × 0.5 / 10
        if (voiceCount > 0)
        {
            breathPhase_ += juce::MathConstants<float>::twoPi * 0.05f;
            if (breathPhase_ > juce::MathConstants<float>::twoPi)
                breathPhase_ -= juce::MathConstants<float>::twoPi;
            needsRepaint = true;
        }
        else if (breathPhase_ != 0.0f)
        {
            breathPhase_ = 0.0f; // reset when silent — glow disappears cleanly
            needsRepaint = true;
        }

        // ── Macro values (APVTS, message-thread safe) ──────────────────────────
        // P10 fix: use pre-built cachedMacroIds — no juce::String allocation here.
        if (hasEngine)
        {
            auto& apvts = processor.getAPVTS();
            for (int m = 0; m < 4; ++m)
            {
                auto* p = apvts.getRawParameterValue(cachedMacroIds[m]);
                float newVal = p ? p->load() : 0.0f;
                if (std::abs(newVal - macroValues[m]) > 0.001f)
                {
                    macroValues[m] = newVal;
                    needsRepaint = true;
                }
            }
        }

        // ── Coupling route count ───────────────────────────────────────────────
        // P3 fix: only check coupling routes every 5th tick (2Hz effective) to
        // avoid copying the full routes vector 50×/sec across all tiles.
        ++couplingCheckCounter;
        if (couplingCheckCounter >= 5)
        {
            couplingCheckCounter = 0;
            auto routes = processor.getCouplingMatrix().getRoutes();
            int modCount = 0;   // LFO/Env/Amp/Filter/Pitch/Rhythm
            int audioCount = 0; // AudioTo*
            int knotCount = 0;  // KnotTopology
            for (const auto& r : routes)
            {
                if (!r.active)
                    continue;
                if (r.sourceSlot != slot && r.destSlot != slot)
                    continue;

                switch (r.type)
                {
                case CouplingType::AudioToFM:
                case CouplingType::AudioToRing:
                case CouplingType::AudioToWavetable:
                case CouplingType::AudioToBuffer:
                    audioCount = juce::jmin(audioCount + 1, 4);
                    break;
                case CouplingType::KnotTopology:
                    knotCount = juce::jmin(knotCount + 1, 4);
                    break;
                default:
                    modCount = juce::jmin(modCount + 1, 4);
                    break;
                }
            }
            int totalDots = juce::jmin(modCount + audioCount + knotCount, 4);
            if (totalDots != couplingDotCount || modCount != couplingModCount || audioCount != couplingAudioCount ||
                knotCount != couplingKnotCount)
            {
                couplingDotCount = totalDots;
                couplingModCount = modCount;
                couplingAudioCount = audioCount;
                couplingKnotCount = knotCount;
                needsRepaint = true;
            }
        }

        if (needsRepaint)
            repaint();
    }

    // Mockup-matched tile: accent bar + header row + macro knobs + waveform + footer + CPU bar.
    // No porthole, no creature renderer. Power button replaces old mute toggle visually.
    void paint(juce::Graphics& g) override
    {
        // Dark Cockpit B041: active/sounding tiles stay fully lit.
        // Non-active tiles dim with cockpit opacity.
        // Fix #7: use cachedCockpitHost_ (set in parentHierarchyChanged()) instead
        // of dynamic_cast walk on every paint() call.
        {
            bool isSounding = (voiceCount > 0);
            float opacity = 1.0f;
            if (!isSounding)
            {
                if (cachedCockpitHost_ != nullptr)
                    opacity = cachedCockpitHost_->getCockpitOpacity();
                if (opacity < 0.05f)
                    return; // B041 performance optimization
            }
            g.setOpacity(opacity);
        }

        using namespace GalleryColors;
        // Asymmetric padding: 9px top, 11px right, 8px bottom, 14px left
        auto content = getLocalBounds().toFloat();
        content.removeFromTop(9.0f);
        content.removeFromRight(11.0f);
        content.removeFromBottom(8.0f);
        content.removeFromLeft(14.0f);

        auto b = getLocalBounds().toFloat().reduced(1.0f, 0.0f);
        bool hovered = isMouseOver();

        // ── Tile background ──────────────────────────────────────────────────
        // Always show a subtle engine personality tint when an engine is loaded
        if (hasEngine)
        {
            // Subtle accent wash — gives each engine visual identity
            g.setColour(accent.withAlpha(0.06f));
            g.fillRoundedRectangle(b, 4.0f);
        }

        // ── Idle breathing glow — #932 / #1159 ───────────────────────────────
        // When this engine slot has active voices, pulse the accent glow at
        // ~0.5 Hz so the tile visually "breathes" with sound. Range is
        // 0.15→0.35 — a sounding engine needs to be unambiguously
        // distinguishable from a silent one at a glance, not a 1 % delta on
        // top of the background tint.
        if (hasEngine && voiceCount > 0)
        {
            const float breathAlpha = 0.15f + 0.20f * std::sin(breathPhase_);
            g.setColour(accent.withAlpha(breathAlpha));
            g.fillRoundedRectangle(b, 4.0f);
        }

        // ── Depth-zone gradient overlay ──────────────────────────────────────
        // Sunlit engines get a warm cyan tint, Twilight get blue, Midnight get violet
        if (hasEngine)
        {
            const int zone = depthZoneOf(engineId); // already exists in this file
            juce::Colour zoneTop, zoneBot;
            switch (zone)
            {
            case 0: // Sunlit
                zoneTop = juce::Colour(0xFF48CAE4).withAlpha(0.04f); // warm cyan
                zoneBot = juce::Colours::transparentBlack;
                break;
            case 2: // Midnight
                zoneTop = juce::Colours::transparentBlack;
                zoneBot = juce::Colour(0xFF7B2FBE).withAlpha(0.04f); // violet
                break;
            default: // Twilight
                zoneTop = juce::Colour(0xFF3366FF).withAlpha(0.02f); // blue
                zoneBot = juce::Colour(0xFF3366FF).withAlpha(0.02f);
                break;
            }
            juce::ColourGradient zoneGrad(zoneTop, b.getX(), b.getY(), zoneBot, b.getX(), b.getBottom(), false);
            g.setGradientFill(zoneGrad);
            g.fillRoundedRectangle(b, 4.0f);
        }

        if (isSelected && hasEngine)
        {
            // Selected: stronger accent tint
            g.setColour(accent.withAlpha(0.12f));
            g.fillRoundedRectangle(b, 4.0f);
            // Left-to-right accent gradient fade
            juce::ColourGradient grad(accent.withAlpha(0.15f), b.getX(), b.getCentreY(),
                                      juce::Colours::transparentBlack, b.getRight(), b.getCentreY(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(b, 4.0f);
        }
        else if (hovered)
        {
            g.setColour(accent.withAlpha(0.09f));
            g.fillRoundedRectangle(b, 4.0f);
        }

        // Bottom border — 1px separator
        g.setColour(GalleryColors::border());
        g.fillRect(b.getX(), b.getBottom() - 1.0f, b.getWidth(), 1.0f);

        if (isLoading)
        {
            g.setColour(get(xoGold).withAlpha(0.5f));
            g.setFont(GalleryFonts::body(10.0f)); // (#885: 9pt→10pt legibility floor)
            g.drawText("LOADING...", b.toNearestInt(), juce::Justification::centred);
            if (hasKeyboardFocus(true))
                A11y::drawFocusRing(g, b, 8.0f);
            return;
        }

        if (hasEngine)
        {
            // Voice density for reactive alpha (accent bar, power button)
            const float kMaxVoices = 8.0f;
            float voiceDensity = (voiceCount > 0) ? juce::jmin(1.0f, std::sqrt((float)voiceCount / kMaxVoices)) : 0.0f;
            float stripAlpha = 0.38f + voiceDensity * 0.50f;

            // ── 1. Left accent bar ─────────────────────────────────────────
            // 3px wide, full tile height, flush x=0, rounded-right corners
            {
                float stripH = (float)getHeight();
                g.setColour(accent.withAlpha(stripAlpha));
                g.fillRoundedRectangle(0.0f, 0.0f, 3.0f, stripH, 1.5f);

                // Active glow (box-shadow: 0 0 8px accent) when selected
                if (isSelected)
                {
                    for (int gx = 1; gx <= 6; ++gx)
                    {
                        float glowAlpha = (0.12f / (float)gx) * (stripAlpha * 1.4f);
                        g.setColour(accent.withAlpha(juce::jmin(glowAlpha, 0.18f)));
                        g.fillRoundedRectangle(0.0f, -1.0f, 3.0f + (float)(gx * 2), stripH + 2.0f, 2.0f);
                    }
                }
            }

            // ── 2. Header row (inside content area) ───────────────────────
            // Creature icon (or slot number fallback) | engine name + depth dot | power button
            // Row height: 14px, sits at content top
            {
                const float rowH = 14.0f;
                const float rowY = content.getY();
                const float slotW = 24.0f; // enlarged to match 24px creature icon
                const float pwrW = 16.0f;
                const float pwrH = 16.0f;

                // ── D3 Scaffold: Creature icon corner overlay ──────────────
                // Try PNG sprite first (path: BinaryData or Assets/creatures/<id>_1x.png).
                // Falls back to procedural CreatureRenderer which always succeeds.
                // Falls back further to plain slot number if creature bounds are too small.
                {
                    const float creatureSize = 24.0f; // enlarged for visibility — own 24px slot
                    juce::Rectangle<float> creatureBounds(content.getX(), rowY + (rowH - creatureSize) * 0.5f,
                                                          creatureSize, creatureSize);

                    // Future PNG sprite path: BinaryData::<engineId>_creature_1x_png
                    // For now, no sprites exist — always use procedural renderer.
                    // When PNG assets land, add: if (spriteImage.isValid()) { g.drawImage(...); }
                    // The procedural renderer uses breathScale=1.0 (static at 10Hz, no breath
                    // animation on tiles — matches the "no height increase" constraint).
                    const float couplingLean = (couplingDotCount > 0)
                                                   ? juce::jlimit(-1.0f, 1.0f, (float)(couplingModCount - couplingAudioCount) * 0.33f)
                                                   : 0.0f;
                    CreatureRenderer::drawCreature(g, creatureBounds, engineId, accent, 1.0f, couplingLean);
                }

                // Engine name — 14px Overbit (D2), uppercase, accent color
                // Name starts after the 24px creature column + 3px gap
                float nameX = content.getX() + slotW + 3.0f;

                // ── D3 Scaffold: 6px depth zone dot next to engine name ────
                // Drawn just before the engine name text, 3px to the left of nameX.
                // Colors match DepthZoneDial zone palette exactly.
                {
                    const float dotDiam = 6.0f;
                    // Place dot centered vertically in the row, at the left edge of the name
                    float dotX = nameX;
                    float dotY = rowY + (rowH - dotDiam) * 0.5f;

                    const int zone = depthZoneOf(engineId);
                    juce::Colour dotColor;
                    switch (zone)
                    {
                    case 0: dotColor = juce::Colour(0xFF00E5FFu); break; // Sunlit — cyan
                    case 2: dotColor = juce::Colour(0xFF9B30FFu); break; // Midnight — violet
                    default: dotColor = juce::Colour(0xFF3366FFu); break; // Twilight — blue
                    }
                    g.setColour(dotColor.withAlpha(0.82f));
                    g.fillEllipse(dotX, dotY, dotDiam, dotDiam);

                    // Shift name text to the right of the dot + 3px gap
                    nameX += dotDiam + 3.0f;
                }

                float nameW = content.getWidth() - (nameX - content.getX()) - pwrW - 3.0f;
                g.setFont(GalleryFonts::engineName(14.0f));
                g.setColour(GalleryColors::ensureMinContrast(accent));
                // (#884: use ellipsizeText so long names show "..." instead of clipping silently)
                auto displayEngineId = GalleryUtils::ellipsizeText(g.getCurrentFont(), engineId.toUpperCase(), nameW);
                g.drawText(displayEngineId, (int)nameX, (int)rowY, (int)nameW, (int)rowH,
                           juce::Justification::centredLeft, false);

                // Power button — 16×16 circle, right edge of content
                // Active (unmuted): border + text in accent color
                // Muted: border + text in T3 color
                {
                    float pwrX = content.getRight() - pwrW;
                    float pwrY = rowY + (rowH - pwrH) * 0.5f;
                    juce::Colour pwrColor = isMuted ? GalleryColors::get(GalleryColors::t3()) : GalleryColors::ensureMinContrast(accent);
                    g.setColour(pwrColor);
                    g.drawEllipse(pwrX, pwrY, pwrW, pwrH, 1.0f);
                    g.setFont(GalleryFonts::value(10.0f)); // (#885: 8pt→10pt legibility floor)
                    g.drawText(isMuted ? "o" : "I", (int)pwrX, (int)pwrY, (int)pwrW, (int)pwrH,
                               juce::Justification::centred);
                }
            }

            // Top-down layout: each row stacks below the previous with 5px gaps.
            // Waveform stretches to fill remaining space (most flexible element).
            // Top-down layout: header → knobs → waveform (stretches to bottom).
            // Mood dots, FX indicator, CPU bar removed — not functional yet, wasted space.
            const float gap = 4.0f;
            const float knobArcDiam = 40.0f;
            const float knobLblH = 10.0f;
            const float knobRowH = knobArcDiam + knobLblH;

            float knobY = content.getY() + 16.0f + gap; // below header (16px for larger font)
            float waveTop = knobY + knobRowH + gap;
            float waveH = content.getBottom() - gap - waveTop; // stretches to bottom
            float waveY = waveTop;
            if (waveH < 10.0f)
                waveH = 10.0f;

            // ── 3. Mini macro knobs row ──────────────────────────────────
            {
                const float arcDiam = knobArcDiam;
                const float arcRadius = arcDiam * 0.5f;
                const float arcGap = 5.0f;
                const float arcStep = arcDiam + arcGap;

                // Left-align knobs (matches mockup)
                float kx = content.getX();

                // D11 locked macro labels (2026-04-25): TONE / TIDE / COUPLE / DEPTH.
                // The 40px arc holds these at 10pt Inter with room to spare.
                static const char* kLabels[4] = {"TONE", "TIDE", "COUPLE", "DEPTH"};

                for (int k = 0; k < 4; ++k)
                {
                    float cx = kx + arcRadius;
                    float cy = knobY + arcRadius;

                    // Store hit-test bounds (arc circle + label below it)
                    arcBounds_[k] = juce::Rectangle<float>(kx, knobY, arcDiam, arcDiam + knobLblH);

                    // 270° arc sweep, starts at ~135° (bottom-left), sweeps CW
                    const float startAngle = juce::MathConstants<float>::pi * 0.75f;
                    const float sweepAngle = juce::MathConstants<float>::pi * 1.5f;
                    const float fillPos = macroValues[k];

                    // Track arc (T4 color, 2.0px)
                    juce::Path trackArc;
                    trackArc.addCentredArc(cx, cy, arcRadius - 2.0f, arcRadius - 2.0f, 0.0f, startAngle,
                                           startAngle + sweepAngle, true);
                    g.setColour(GalleryColors::get(GalleryColors::t4()));
                    g.strokePath(trackArc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                                juce::PathStrokeType::rounded));

                    // Fill arc (accent, 2.5px)
                    juce::Path fillArc;
                    fillArc.addCentredArc(cx, cy, arcRadius - 2.0f, arcRadius - 2.0f, 0.0f, startAngle,
                                          startAngle + sweepAngle * fillPos, true);
                    g.setColour(accent.withAlpha(0.85f));
                    g.strokePath(fillArc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                                               juce::PathStrokeType::rounded));

                    // Label below arc (10pt mono, T2 — legibility floor #885)
                    g.setFont(GalleryFonts::value(10.0f));
                    g.setColour(GalleryColors::get(GalleryColors::t2()));
                    g.drawText(kLabels[k], (int)(kx - 2.0f), (int)(knobY + arcDiam + 1.0f), (int)(arcDiam + 4.0f),
                               (int)knobLblH, juce::Justification::centred);

                    kx += arcStep;
                }
            }

            // ── 4. Mini waveform area ────────────────────────────────────
            // Background tint; the live MiniWaveform child component paints on top.
            {
                float waveX = content.getX();
                float waveW = content.getWidth();

                g.setColour(juce::Colour(0x06FFFFFF)); // rgba(255,255,255,0.025)
                g.fillRoundedRectangle(waveX, waveY, waveW, waveH, 3.0f);

                // MiniWaveform accent color must stay in sync with accent changes.
                // miniWave.setAccentColour() is called in refresh() so it's always current.
            }

            // Footer (mood dots, FX indicator) and CPU bar removed —
            // not wired to real data yet, reclaimed space for larger elements.

            // ── 5. Coupling dots ─────────────────────────────────────────
            // Overlaid at bottom-left of waveform area (4px dots, 6px from bottom)
            paintCouplingDots(g, content.getX(), waveY + waveH - 6.0f);
        }
        else
        {
            // Empty slot — 28×28 dashed rounded rect centered, "+" inside,
            // "Add engine" label below. T4 color throughout.
            juce::Colour t4col = GalleryColors::get(GalleryColors::t4());

            float tileCx = (float)getWidth() * 0.5f;
            float tileCy = (float)getHeight() * 0.5f;

            const float btnW = 28.0f, btnH = 28.0f, btnR = 6.0f;
            float btnX = tileCx - btnW * 0.5f;
            float btnY = tileCy - btnH * 0.5f - 7.0f;

            {
                juce::Path btnRect;
                btnRect.addRoundedRectangle(btnX, btnY, btnW, btnH, btnR);
                juce::PathStrokeType stroke(1.0f);
                float dashPattern[] = {4.0f, 3.0f};
                juce::Path dashedBtn;
                stroke.createDashedStroke(dashedBtn, btnRect, dashPattern, 2);
                g.setColour(t4col.withAlpha(0.70f));
                g.strokePath(dashedBtn, juce::PathStrokeType(1.0f));
            }

            g.setFont(GalleryFonts::body(16.0f));
            g.setColour(t4col);
            g.drawText("+", (int)btnX, (int)btnY, (int)btnW, (int)btnH, juce::Justification::centred);

            g.setFont(GalleryFonts::body(10.0f));
            g.setColour(t4col.withAlpha(0.70f));
            float labelY = btnY + btnH + 4.0f;
            g.drawText("Add engine", (int)(tileCx - 40.0f), (int)labelY, 80, 14, juce::Justification::centred);
        }

        // Focus ring (WCAG 2.4.7)
        if (hasKeyboardFocus(true))
            A11y::drawFocusRing(g, b, 8.0f);
    }

    void resized() override
    {
        // Mirror the waveform area calculation from paint() so miniWave occupies
        // exactly the same region that previously held the fake sine polyline.
        auto content = getLocalBounds().toFloat();
        content.removeFromTop(9.0f);
        content.removeFromRight(11.0f);
        content.removeFromBottom(8.0f);
        content.removeFromLeft(14.0f);

        const float gap = 4.0f;
        const float knobRowH = 40.0f + 10.0f; // knobArcDiam + knobLblH
        float knobY = content.getY() + 16.0f + gap;
        float waveTop = knobY + knobRowH + gap;
        float waveH = content.getBottom() - gap - waveTop;
        if (waveH < 10.0f) waveH = 10.0f;

        miniWave.setBounds(juce::Rectangle<float>(content.getX(), waveTop, content.getWidth(), waveH).toNearestInt());
    }

    // Fix #7: cache CockpitHost pointer once when the component hierarchy is set up,
    // avoiding a dynamic_cast walk on every paint() call (O(depth) per frame).
    void parentHierarchyChanged() override { cachedCockpitHost_ = CockpitHost::find(this); }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseMove(const juce::MouseEvent& e) override
    {
        if (hasEngine && getMuteToggleBounds().contains(e.getPosition()))
        {
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
            return;
        }
        if (hasEngine)
        {
            auto pos = e.position;
            for (int i = 0; i < 4; ++i)
            {
                if (arcBounds_[i].contains(pos))
                {
                    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
                    return;
                }
            }
        }
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    void mouseExit(const juce::MouseEvent&) override
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }
    void focusGained(juce::Component::FocusChangeType) override { repaint(); }
    void focusLost(juce::Component::FocusChangeType) override { repaint(); }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey)
        {
            if (hasEngine)
            {
                if (onSelect)
                    onSelect(slot);
            }
            else
                showLoadMenu();
            return true;
        }
        return false;
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // ── Arc macro knob interaction — check arcs FIRST ──────────────────────
        if (hasEngine && !e.mods.isPopupMenu())
        {
            auto pos = e.position;
            for (int i = 0; i < 4; ++i)
            {
                if (arcBounds_[i].contains(pos))
                {
                    activeArc_ = i;
                    auto& apvts = processor.getAPVTS();
                    auto* p = apvts.getParameter(cachedMacroIds[i]);
                    dragStartValue_ = p ? p->getValue() : macroValues[i];
                    return; // consumed — don't fall through to mute/menu logic
                }
            }
        }

        // Check if click is on the mute toggle (top-left 16×16pt, 4pt margin)
        auto toggleBounds = getMuteToggleBounds();
        if (toggleBounds.contains(e.getPosition()))
        {
            // Handle mute toggle click — toggle the muted state
            if (!e.mods.isPopupMenu())
            {
                isMuted = !isMuted;
                processor.setSlotMuted(slot, isMuted);
                repaint();
                return;
            }
        }

        if (!e.mods.isPopupMenu() || !hasEngine)
            return;

        juce::PopupMenu menu;
        menu.addSectionHeader("SLOT " + juce::String(slot + 1) + ": " + engineId.toUpperCase());
        menu.addSeparator();
        menu.addItem(100, "Change Engine...");
        menu.addItem(101, "Remove Engine");
        menu.addSeparator();

        juce::PopupMenu moveMenu;
        for (int i = 0; i < 4; ++i)
        {
            if (i == slot)
                continue;
            auto* targetEng = processor.getEngine(i);
            if (targetEng != nullptr)
            {
                // Target slot is occupied — warn user inline via item label
                juce::String occupantName = targetEng->getEngineId().toUpperCase();
                moveMenu.addItem(200 + i, "Replace " + occupantName + " in Slot " + juce::String(i + 1));
            }
            else
            {
                moveMenu.addItem(200 + i, "Move to Slot " + juce::String(i + 1));
            }
        }
        menu.addSubMenu("Move to Slot", moveMenu);

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                           [this](int result)
                           {
                               if (result == 100)
                                   showLoadMenu();
                               else if (result == 101)
                               {
                                   processor.unloadEngine(slot);
                               }
                               else if (result >= 200 && result < 204)
                               {
                                   int targetSlot = result - 200;
                                   auto* eng = processor.getEngine(slot);
                                   if (eng != nullptr)
                                   {
                                       auto currentId = eng->getEngineId().toStdString();
                                       processor.loadEngine(targetSlot, currentId);
                                       processor.unloadEngine(slot);
                                   }
                               }
                           });
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (activeArc_ < 0)
            return;

        auto& apvts = processor.getAPVTS();
        if (auto* param = apvts.getParameter(cachedMacroIds[activeArc_]))
        {
            float delta = -e.getDistanceFromDragStartY() * 0.003f;
            float newVal = std::clamp(dragStartValue_ + delta, 0.0f, 1.0f);
            param->beginChangeGesture();
            param->setValueNotifyingHost(newVal);
            param->endChangeGesture();
        }
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        // Release any active arc drag — do this before the drag-guard below so
        // a pure arc drag that ends here is always cleaned up.
        if (activeArc_ >= 0)
        {
            activeArc_ = -1;
            return; // consumed — don't trigger engine-select on arc drag-release
        }

        if (e.mouseWasDraggedSinceMouseDown())
            return;

        if (e.mods.isPopupMenu())
            return; // right-click handled by mouseDown

        // Don't navigate to engine if mute toggle was clicked
        auto toggleBounds = getMuteToggleBounds();
        if (toggleBounds.contains(e.getPosition()))
            return;

        if (hasEngine)
        {
            if (onSelect)
                onSelect(slot);
        }
        else
        {
            showLoadMenu();
        }
    }

    void setSelected(bool sel)
    {
        isSelected = sel;
        repaint();
    }

private:
    // ── Power button hit area ────────────────────────────────────────────────
    // 16×16 button, top-right of content area (right edge = getWidth()-11, top = 9).
    // Hit area extended to 24×24 for touch comfort.
    juce::Rectangle<int> getMuteToggleBounds() const
    {
        // content right = getWidth() - 11; power button right-aligned there
        // content top = 9; header row height = 14; button centered in row
        int pwrRight = getWidth() - 11;
        int pwrX = pwrRight - 16;
        int rowCy = 9 + 7; // content top + half header row
        return {pwrX - 4, rowCy - 12, 24, 24};
    }

    // ── Coupling indicator dots ──────────────────────────────────────────────
    // Up to 4 dots (4×4pt) in a row. Color per coupling category:
    //   XO Gold (#E9C46A)        — modulation routes (LFO/Env/Amp/Filter/Pitch/Rhythm)
    //   Twilight Blue (#1B4F8A)  — audio-rate routes (AudioTo*)
    //   Midnight Violet (#7B2FBE) — KnotTopology routes
    void paintCouplingDots(juce::Graphics& g, float startX, float startY) const
    {
        if (!hasEngine || couplingDotCount == 0)
            return;

        // Build a color list: mod dots first, then audio, then knot
        juce::Colour dotColors[4];
        int idx = 0;
        for (int i = 0; i < couplingModCount && idx < 4; ++i, ++idx)
            dotColors[idx] = juce::Colour(GalleryColors::xoGold); // XO Gold
        for (int i = 0; i < couplingAudioCount && idx < 4; ++i, ++idx)
            dotColors[idx] = juce::Colour(0xFF1B4F8A); // Twilight Blue
        for (int i = 0; i < couplingKnotCount && idx < 4; ++i, ++idx)
            dotColors[idx] = juce::Colour(0xFF7B2FBE); // Midnight Violet

        const float dotSize = 4.0f;
        const float dotSpacing = 6.0f;

        for (int d = 0; d < couplingDotCount && d < 4; ++d)
        {
            float dotX = startX + static_cast<float>(d) * dotSpacing;
            g.setColour(dotColors[d].withAlpha(0.85f));
            g.fillEllipse(dotX, startY, dotSize, dotSize);
        }
    }

    // ── Engine archetype lookup ──────────────────────────────────────────────
    // Returns the one-line archetype description for an engine ID (case-insensitive).
    // Mirrors the archetype strings in EnginePickerPopup::engineMetadataTable().
    // Returns empty string if not found (tooltip will omit the archetype suffix).
    static juce::String archetypeOf(const juce::String& engineId)
    {
        struct ArchEntry { const char* id; const char* archetype; };
        static const ArchEntry kTable[] = {
            // Kitchen Collection — Organs
            { "Oto",        "tonewheel drawbar organ" },
            { "Octave",     "Hammond tonewheel simulation" },
            { "Oleg",       "theatre pipe organ" },
            { "Otis",       "gospel soul organ drive" },
            // Kitchen Collection — Pianos
            { "Oven",       "Steinway concert grand piano" },
            { "Ochre",      "wooden resonator piano" },
            { "Obelisk",    "grand piano sympathetic resonance" },
            { "Opaline",    "prepared piano rust and objects" },
            // Kitchen Collection — Bass
            { "Ogre",       "sub bass synthesizer" },
            { "Olate",      "fretless bass guitar" },
            { "Oaken",      "upright double bass" },
            { "Omega",      "analog synth bass" },
            // Kitchen Collection — Strings
            { "Orchard",    "orchestral strings bow pressure" },
            { "Overgrow",   "overgrown string textures" },
            { "Osier",      "willow wind strings" },
            { "Oxalis",     "wood sorrel lilac strings" },
            // Kitchen Collection — Pads
            { "Overwash",   "tide foam diffusion pad" },
            { "Overworn",   "worn felt texture pad" },
            { "Overflow",   "deep current flowing pad" },
            { "Overcast",   "cloud diffusion pad" },
            // Kitchen Collection — EPs
            { "Oasis",      "desert spring electric piano" },
            { "Oddfellow",  "spectral fingerprint cache EP" },
            { "Onkolo",     "spectral amber resonant EP" },
            { "Opcode",     "dark turquoise code-driven EP" },
            // Flagship + core synths
            { "Obrix",      "modular brick reef synthesizer" },
            { "Oxytocin",   "circuit love triangle synthesizer" },
            { "Overbite",   "apex predator modal synthesizer" },
            { "Overworld",  "ERA triangle timbral crossfade" },
            { "Ouroboros",  "strange attractor chaotic synthesizer" },
            { "Oracle",     "GENDY stochastic maqam synthesis" },
            { "Orbital",    "group envelope synthesizer" },
            { "Opal",       "granular cloud synthesizer" },
            { "Obsidian",   "crystal resonant synthesizer" },
            { "Origami",    "fold-point waveshaping synthesizer" },
            { "Obscura",    "daguerreotype physical modeling" },
            { "Oblique",    "prismatic bounce synth" },
            { "Organism",   "cellular automata generative synth" },
            { "Orbweave",   "topological knot coupling engine" },
            { "Overtone",   "continued fraction spectral synth" },
            { "Oxbow",      "entangled reverb synthesizer" },
            { "Outlook",    "panoramic dual wavetable synth" },
            { "Overlap",    "knot matrix FDN synthesizer" },
            { "Orca",       "apex predator wavetable echolocation" },
            { "Octopus",    "decentralized alien intelligence synth" },
            { "Ombre",      "dual narrative memory synthesizer" },
            { "OpenSky",    "euphoric shimmer supersaw synth" },
            // Percussion
            { "Onset",      "cross-voice coupling percussion" },
            { "Offering",   "psychology-driven boom bap drums" },
            { "Oware",      "Akan tuned mallet percussion" },
            { "Ostinato",   "modal membrane world rhythm engine" },
            // Vocal
            { "Opera",      "additive-vocal Kuramoto synchrony" },
            { "Obbligato",  "breath articulation vocal synth" },
            // Bass synths
            { "Oblong",     "resonant bass synthesizer" },
            { "Obese",      "fat saturation bass synth" },
            // Organ & wind
            { "Organon",    "variational metabolism organ synth" },
            { "Ohm",        "sage analog organ synthesizer" },
            { "Ottoni",     "patina brass organ synthesizer" },
            { "Ole",        "hibiscus flamenco organ synth" },
            // String / physical modeling
            { "Orphica",    "siren seafoam plucked string" },
            { "Osprey",     "shore coastline cultural synthesis" },
            { "Osteria",    "porto wine shore string synth" },
            { "Owlfish",    "Mixtur-Trautonium string modeling" },
            // Character
            { "OddfeliX",   "neon tetra character synth" },
            { "OddOscar",   "axolotl character synth" },
            { "Odyssey",    "drift analog poly synthesizer" },
            { "Overdub",    "spring reverb dub synthesizer" },
            { "Oceanic",    "chromatophore phosphorescent synth" },
            { "Ocelot",     "biome crossfade ocelot synth" },
            { "Osmosis",    "external audio membrane synth" },
            // Utility
            { "Optic",      "visual modulation zero-audio engine" },
            { "Outwit",     "chromatophore amber effect engine" },
            // Additional engines
            { "OceanDeep",  "hydrostatic deep ocean synthesizer" },
            { "Ouie",       "duophonic hammerhead synthesizer" },
            { nullptr, nullptr }, // sentinel
        };

        const juce::String lower = engineId.toLowerCase();
        for (int i = 0; kTable[i].id != nullptr; ++i)
        {
            if (lower == juce::String(kTable[i].id).toLowerCase())
                return juce::String(kTable[i].archetype);
        }
        return {};
    }

    // ── D3 Scaffold: Depth zone lookup ──────────────────────────────────────
    // Returns 0=Sunlit, 1=Twilight, 2=Midnight.
    // Table mirrors DepthZoneDial::depthZoneOf() exactly — single source of truth
    // is DepthZoneDial; this copy kept in sync until a shared utility header exists.
    static int depthZoneOf(const juce::String& engineId)
    {
        static const std::pair<const char*, int> kZoneTable[] = {
            // Sunlit (0)
            {"Oto", 0}, {"Octave", 0}, {"Oleg", 0}, {"Otis", 0}, {"Obelisk", 0},
            {"Orchard", 0}, {"Osier", 0}, {"Overwash", 0}, {"Overworld", 0}, {"Oasis", 0},
            {"OddfeliX", 0}, {"OddOscar", 0}, {"Ohm", 0}, {"Optic", 0}, {"Opensky", 0},
            // Twilight (1)
            {"Oven", 1}, {"Ochre", 1}, {"Opaline", 1}, {"Olate", 1}, {"Oaken", 1},
            {"Overgrow", 1}, {"Oxalis", 1}, {"Overworn", 1}, {"Overcast", 1}, {"Oddfellow", 1},
            {"Onkolo", 1}, {"Opcode", 1}, {"Onset", 1}, {"Offering", 1}, {"Oware", 1},
            {"Ostinato", 1}, {"Opera", 1}, {"Obbligato", 1}, {"Oblong", 1}, {"Obese", 1},
            {"Organon", 1}, {"Ottoni", 1}, {"Ole", 1}, {"Orphica", 1}, {"Osprey", 1},
            {"Osteria", 1}, {"Opal", 1}, {"Orbital", 1}, {"Origami", 1}, {"Obscura", 1},
            {"Oblique", 1}, {"Organism", 1}, {"Overtone", 1}, {"Outlook", 1}, {"Oceanic", 1},
            {"Ocelot", 1}, {"Ombre", 1}, {"Odyssey", 1}, {"Overdub", 1}, {"Osmosis", 1},
            {"Outwit", 1}, {"Obiont", 1}, {"Okeanos", 1}, {"Outflow", 1},
            // Midnight (2)
            {"Ogre", 2}, {"Omega", 2}, {"Overflow", 2}, {"Obrix", 2}, {"Oxytocin", 2},
            {"Overbite", 2}, {"Ouroboros", 2}, {"Oracle", 2}, {"Obsidian", 2}, {"Orbweave", 2},
            {"Oxbow", 2}, {"Orca", 2}, {"Octopus", 2}, {"Owlfish", 2}, {"Overlap", 2},
            {"Oceandeep", 2}, {"Ouie", 2},
            {nullptr, 0}, // sentinel
        };

        const juce::String lower = engineId.toLowerCase();
        for (int i = 0; kZoneTable[i].first != nullptr; ++i)
        {
            if (lower == juce::String(kZoneTable[i].first).toLowerCase())
                return kZoneTable[i].second;
        }
        return 1; // default Twilight
    }

    void showLoadMenu()
    {
        auto* picker = new EnginePickerPopup();
        // Use SafePointer so the callback is a no-op if the tile is destroyed
        // before the CallOutBox closes (e.g. rapid slot changes or window close).
        auto safeThis = juce::Component::SafePointer<CompactEngineTile>(this);
        picker->onEngineSelected = [safeThis](const juce::String& engineId)
        {
            if (safeThis == nullptr)
                return;
            safeThis->isLoading = true;
            safeThis->repaint();
            safeThis->processor.loadEngine(safeThis->slot, engineId.toStdString());
            juce::Timer::callAfterDelay(0,
                                        [safeThis]
                                        {
                                            if (safeThis == nullptr)
                                                return;
                                            if (safeThis->onSelect)
                                                safeThis->onSelect(safeThis->slot);
                                        });
        };
        picker->setSize(280, 400);
        juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::Component>(picker), getScreenBounds(), nullptr);
    }

    XOceanusProcessor& processor;
    int slot;
    juce::String engineId;
    juce::Colour accent;
    bool hasEngine = false;
    bool isSelected = false;
    bool isLoading = false;
    bool isMuted = false; // synced to processor.slotMuted[] via setSlotMuted/isSlotMuted
    int voiceCount = 0;

    // Macro bar values — updated in timerCallback at 10Hz
    std::array<float, 4> macroValues{};

    // Coupling dot state — updated in timerCallback at 10Hz
    int couplingDotCount = 0;
    int couplingModCount = 0;
    int couplingAudioCount = 0;
    int couplingKnotCount = 0;

    // P3 fix: tick counter to run coupling check only every 5th tick (2Hz effective)
    int couplingCheckCounter = 0;

    // #932: breathing phase for active-voice glow (0.5 Hz sine, radians)
    float breathPhase_ = 0.0f;

    // P10 fix: pre-built parameter ID strings — avoids 4 allocations/tick/tile
    std::array<juce::String, 4> cachedMacroIds;

    // Fix #7: cached CockpitHost pointer — set in parentHierarchyChanged(),
    // used in paint() to avoid dynamic_cast walk on every frame.
    CockpitHost* cachedCockpitHost_ = nullptr;

    // Arc macro drag interaction — populated in paint(), used in mouseDown/Drag/Up.
    juce::Rectangle<float> arcBounds_[4]{};
    int   activeArc_      = -1;
    float dragStartValue_ = 0.0f;

    MiniWaveform miniWave;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompactEngineTile)
};

} // namespace xoceanus
