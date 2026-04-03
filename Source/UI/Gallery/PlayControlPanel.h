// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// PlayControlPanel.h — Column C, C4 "PLAY" tab content panel (320pt wide).
//
// Layout top-to-bottom:
//   1. Expression Strip  (320 × 160pt) — Mod Wheel + Pitch Bend side by side
//   2. Macro Meters      (320 ×  48pt) — 4 horizontal fill bars (no SliderAttachment)
//   3. XY Pad            (160 × 160pt, centred) — assignable two-axis gesture pad
//   4. Scale Selector    (320 ×  28pt) — NamedModeSelector for PlaySurface quantization
//
// Architecture constraints:
//   • Header-only (.h) — DSP-free, UI-only component
//   • NO SliderAttachment for macros — avoids double-attachment with MacroSection
//   • Macro reads: getRawParameterValue("macroN")->load() at 10 Hz
//   • Macro writes: getAPVTS().getParameter("macroN")->setValueNotifyingHost(v)
//   • Pitch bend: spring-return physics at 30 Hz tick in timerCallback
//   • Uses GalleryColors, GalleryFonts, A11y from GalleryColors.h
//   • Requires XOceanusProcessor.h (for getAPVTS() and getMidiCollector())
//
// Scale selector stores state internally for V1; setScaleIndex(int) / getScaleIndex()
// let the parent (SidebarPanel / PlaySurface bridge) wire it later.

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <array>
#include <cmath>
#include "../../XOceanusProcessor.h"
#include "../GalleryColors.h"
#include "SpecializedWidgets.h"

namespace xoceanus
{

//==============================================================================
// PlayControlPanel
//==============================================================================
class PlayControlPanel : public juce::Component, private juce::Timer
{
public:
    //==========================================================================
    explicit PlayControlPanel(XOceanusProcessor& proc) : processor(proc)
    {
        // ── Scale selector ────────────────────────────────────────────────────
        // V1: internal choice, not yet wired to an APVTS parameter.
        // scaleSelector is built with a minimal stub APVTS so NamedModeSelector
        // can own its ComboBoxAttachment normally.  When a real "playScale" param
        // is added, swap to: std::make_unique<NamedModeSelector>(proc.getAPVTS(), "playScale", ...)
        //
        // For now we build a standalone ComboBox directly since there is no
        // APVTS parameter for scale in V1.  The NamedModeSelector pattern is
        // preserved for future wiring via the setScaleParamId() method.
        scaleCombo.addItem("Chromatic", 1);
        scaleCombo.addItem("Major", 2);
        scaleCombo.addItem("Minor", 3);
        scaleCombo.addItem("Pentatonic", 4);
        scaleCombo.addItem("Blues", 5);
        scaleCombo.addItem("Dorian", 6);
        scaleCombo.addItem("Mixolydian", 7);
        scaleCombo.setSelectedItemIndex(0, juce::dontSendNotification);
        scaleCombo.setVisible(false); // driven through custom paint
        addAndMakeVisible(scaleCombo);

        // Restore persisted scale choice from the last DAW session (#314).
        // Use persist=false to avoid immediately writing back the restored value
        // (the processor already holds the correct index).
        setScaleIndex(processor.getPlayScaleIndex(), /*persist=*/false);

        // ── Accessibility ─────────────────────────────────────────────────────
        A11y::setup(*this, "Play Control Panel",
                    "Expression strips, macro meters, XY pad, and scale selector for performance.");

        // ── Timer: 10 Hz meter refresh + 30 Hz pitch-bend physics ────────────
        // We use a single 33 ms (≈30 Hz) timer; meter refresh is gated every
        // 3rd tick (~10 Hz) by the tickCount counter.
        startTimerHz(30);
    }

    ~PlayControlPanel() override { stopTimer(); }

    void visibilityChanged() override
    {
        if (isVisible())
            startTimerHz(30);
        else
            stopTimer();
    }

    //==========================================================================
    // Public API — future wiring hooks
    //==========================================================================

    // Assign an APVTS parameter to the XY pad X axis.
    // Pass nullptr to clear the assignment.
    void setXParam(juce::RangedAudioParameter* param) noexcept { xyXParam = param; }

    // Assign an APVTS parameter to the XY pad Y axis.
    // Pass nullptr to clear the assignment.
    void setYParam(juce::RangedAudioParameter* param) noexcept { xyYParam = param; }

    // Force the scale selector to a specific index (0-based).
    // When persist=true (default), the new index is written back to the
    // processor so it survives DAW session reload (#314).
    void setScaleIndex(int index, bool persist = true)
    {
        scaleIndex = juce::jlimit(0, kNumScales - 1, index);
        scaleCombo.setSelectedItemIndex(scaleIndex, juce::dontSendNotification);
        if (persist)
            processor.setPlayScaleIndex(scaleIndex);
        repaint();
    }

    int getScaleIndex() const noexcept { return scaleIndex; }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        auto b = getLocalBounds().toFloat();

        // ── Panel background ──────────────────────────────────────────────────
        g.setColour(get(shellWhite()));
        g.fillAll();

        // ── 1. Expression Strip ───────────────────────────────────────────────
        auto exprBounds = expressionStripBounds();
        paintExpressionStrip(g, exprBounds);

        // ── Divider ───────────────────────────────────────────────────────────
        g.setColour(get(borderGray()).withAlpha(0.4f));
        g.drawHorizontalLine(exprBounds.getBottom() + kSectionGap / 2, 8.0f, (float)getWidth() - 8.0f);

        // ── 2. Macro Meters ───────────────────────────────────────────────────
        auto macroBounds = macroMetersBounds();
        paintMacroMeters(g, macroBounds);

        // ── Divider ───────────────────────────────────────────────────────────
        g.setColour(get(borderGray()).withAlpha(0.4f));
        g.drawHorizontalLine(macroBounds.getBottom() + kSectionGap / 2, 8.0f, (float)getWidth() - 8.0f);

        // ── 3. XY Pad ─────────────────────────────────────────────────────────
        auto xyBounds = xyPadBounds();
        paintXYPad(g, xyBounds);

        // ── Divider ───────────────────────────────────────────────────────────
        g.setColour(get(borderGray()).withAlpha(0.4f));
        g.drawHorizontalLine(xyBounds.getBottom() + kSectionGap / 2, 8.0f, (float)getWidth() - 8.0f);

        // ── 4. Scale Selector ─────────────────────────────────────────────────
        auto scaleBounds = scaleSelectorBounds();
        paintScaleSelector(g, scaleBounds);

        (void)b;
    }

    void resized() override
    {
        // The scale combo box is driven visually through paint(); park it
        // off-screen (zero-size) so it never intercepts mouse events.
        scaleCombo.setBounds(0, 0, 0, 0);
    }

    //==========================================================================
    // Mouse interaction — dispatched to active sub-region
    //==========================================================================
    void mouseDown(const juce::MouseEvent& e) override
    {
        wakeTimer(); // fix #387: wake adaptive timer on interaction
        if (e.mods.isRightButtonDown())
            return;

        if (expressionStripBounds().contains(e.position.toInt()))
        {
            handleExpressionMouseDown(e);
        }
        else if (macroMetersBounds().contains(e.position.toInt()))
        {
            handleMacroMouseDown(e);
            macroMouseDrag = true;
        }
        else if (xyPadBounds().contains(e.position.toInt()))
        {
            handleXYMouseDown(e);
        }
        else if (scaleSelectorBounds().contains(e.position.toInt()))
        {
            handleScaleMouseDown(e);
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
            return;

        if (activeExprStrip != ExprStrip::None)
        {
            handleExpressionMouseDrag(e);
        }
        else if (macroMouseDrag)
        {
            handleMacroDrag(e);
        }
        else if (xyDragging)
        {
            handleXYMouseDrag(e);
        }
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        (void)e;

        // Pitch bend springs back to centre on release
        if (activeExprStrip == ExprStrip::PitchBend)
            pitchBendSpringActive = true;

        activeExprStrip = ExprStrip::None;
        macroMouseDrag = false;
        activeMacroIndex = -1;
        xyDragging = false;
    }

    //==========================================================================
    // Timer — 30 Hz tick; meter refresh every 3rd tick (10 Hz)
    // Fix #387: dirty-flag gating — track idle ticks and step down to 1 Hz
    // after ~2 seconds of no state change, stepping back up to 30 Hz on any
    // activity. This eliminates ~58 wasted ticks/second when transport is stable.
    //==========================================================================
    void timerCallback() override
    {
        ++tickCount;
        bool anyChange = false;

        // ── Pitch bend spring physics (every tick = 30 Hz) ────────────────────
        if (pitchBendSpringActive && !xyDragging)
        {
            // pos += (0.5 - pos) * 0.15  per tick
            pitchBendPos += (0.5f - pitchBendPos) * 0.15f;
            if (std::abs(pitchBendPos - 0.5f) < 0.002f)
            {
                pitchBendPos = 0.5f;
                pitchBendSpringActive = false;
            }

            // Optionally send MIDI pitch bend to the collector
            sendPitchBendMidi(pitchBendPos);
            repaint(expressionStripBounds());
            anyChange = true;
        }

        // ── Macro meter refresh (every 3rd tick ≈ 10 Hz) ─────────────────────
        if ((tickCount % 3) == 0)
        {
            auto& apvts = processor.getAPVTS();
            bool changed = false;
            for (int i = 0; i < 4; ++i)
            {
                auto* raw = apvts.getRawParameterValue(macroParamIds[i]);
                float v = raw ? raw->load() : 0.0f;
                if (v != macroValues[i])
                {
                    macroValues[i] = v;
                    changed = true;
                }
            }

            // Mod wheel value from CC1 / dedicated param (if any)
            auto* mwRaw = apvts.getRawParameterValue("modWheel");
            float mwVal = mwRaw ? mwRaw->load() : modWheelPos;
            if (mwVal != modWheelPos)
            {
                modWheelPos = mwVal;
                changed = true;
            }

            if (changed)
            {
                repaint();
                anyChange = true;
            }
        }

        // ── Adaptive timer rate (fix #387) ────────────────────────────────────
        // After ~60 ticks (~2s) of no change, drop to 1 Hz polling to save CPU.
        // Wake back to 30 Hz on any state change (including mouse events via
        // the wakeTimer() helper called from mouse handlers).
        if (anyChange)
        {
            idleTickCount = 0;
            if (getTimerInterval() != 33) // currently in 1 Hz mode → wake up
                startTimerHz(30);
        }
        else
        {
            ++idleTickCount;
            if (idleTickCount > 60 && getTimerInterval() == 33)
                startTimerHz(1); // step down to 1 Hz idle polling
        }
    }

    // Call from mouse event handlers to wake the timer back to 30 Hz
    void wakeTimer()
    {
        idleTickCount = 0;
        if (getTimerInterval() != 33)
            startTimerHz(30);
    }

private:
    //==========================================================================
    // Layout constants (all in points, matching the 320pt panel width)
    //==========================================================================
    static constexpr int kPanelW = 320;
    static constexpr int kExprStripH = 160; // Expression Strip total height
    static constexpr int kStripW = 28;      // Individual mod/pitch strip width
    static constexpr int kStripH = 140;     // Strip fill-bar height
    static constexpr int kMacroH = 48;      // Macro Meters total height
    static constexpr int kMacroBarH = 8;    // Per-bar height
    static constexpr int kMacroGap = 4;     // Gap between macro bars
    static constexpr int kXYSize = 160;     // XY Pad square size
    static constexpr int kScaleH = 28;      // Scale Selector height
    static constexpr int kSectionGap = 8;   // Vertical gap between sections
    static constexpr int kPadTop = 8;       // Top padding

    static constexpr int kNumScales = 7;

    // Macro parameter IDs — must match MacroSection and XOceanusProcessor layout
    static constexpr const char* macroParamIds[4] = {"macro1", "macro2", "macro3", "macro4"};

    // Macro display labels — short form matches MacroSection
    static constexpr const char* macroLabels[4] = {"CHAR", "MOVE", "COUP", "SPACE"};

    // Macro bar colors per spec
    const juce::Colour macroColors[4] = {
        juce::Colour(0xFFE9C46A), // M1 XO Gold
        juce::Colour(0xFF00FF41), // M2 Phosphor Green
        juce::Colour(0xFFBF40FF), // M3 Prism Violet
        juce::Colour(0xFF00B4A0), // M4 Teal
    };

    // Scale names
    const juce::StringArray scaleNames{"Chromatic", "Major", "Minor", "Pentatonic", "Blues", "Dorian", "Mixolydian"};

    //==========================================================================
    // Sub-region bounds helpers (relative to component origin, with top padding)
    //==========================================================================
    juce::Rectangle<int> expressionStripBounds() const { return {0, kPadTop, getWidth(), kExprStripH}; }

    juce::Rectangle<int> macroMetersBounds() const
    {
        int top = kPadTop + kExprStripH + kSectionGap;
        return {0, top, getWidth(), kMacroH};
    }

    juce::Rectangle<int> xyPadBounds() const
    {
        int top = kPadTop + kExprStripH + kSectionGap + kMacroH + kSectionGap;
        int centreX = getWidth() / 2 - kXYSize / 2;
        return {centreX, top, kXYSize, kXYSize};
    }

    juce::Rectangle<int> scaleSelectorBounds() const
    {
        int top = kPadTop + kExprStripH + kSectionGap + kMacroH + kSectionGap + kXYSize + kSectionGap;
        return {0, top, getWidth(), kScaleH};
    }

    //==========================================================================
    // Paint helpers
    //==========================================================================

    // ── 1. Expression Strip ───────────────────────────────────────────────────
    void paintExpressionStrip(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        using namespace GalleryColors;

        auto bf = bounds.toFloat();

        // Section label
        g.setFont(GalleryFonts::label(8.0f));
        g.setColour(get(textMid()));
        g.drawText("EXPRESSION", bf.withHeight(12.0f), juce::Justification::centred, false);

        // Vertical centre of the strip area (below section label)
        float stripAreaY = bf.getY() + 14.0f;
        float stripAreaH = bf.getHeight() - 14.0f;

        // Engine accent for strip fill (fallback to XO Gold if no slot 0 engine)
        juce::Colour accentCol = juce::Colour(GalleryColors::xoGold);
        if (auto* eng = processor.getEngine(0))
            accentCol = eng->getAccentColour();

        // ── Mod Wheel (left strip) ────────────────────────────────────────────
        float modX = bf.getCentreX() - kStripW * 2.0f - 8.0f; // offset left of centre
        float stripY = stripAreaY + (stripAreaH - kStripH) * 0.5f;
        paintVerticalStrip(g, modX, stripY, kStripW, kStripH, modWheelPos, accentCol, "MOD");

        // ── Pitch Bend (right strip) ──────────────────────────────────────────
        float pbX = bf.getCentreX() + 8.0f;
        paintPitchBendStrip(g, pbX, stripY, kStripW, kStripH, pitchBendPos, accentCol, "BEND");
    }

    // Vertical fill strip (mod wheel style — fills from bottom)
    void paintVerticalStrip(juce::Graphics& g, float x, float y, float w, float h,
                            float normalizedValue, // 0..1
                            juce::Colour fillColor, const juce::String& label)
    {
        using namespace GalleryColors;

        float radius = w * 0.35f;

        // Track background
        g.setColour(get(borderGray()).withAlpha(0.35f));
        g.fillRoundedRectangle(x, y, w, h, radius);

        // Track outline
        g.setColour(get(borderGray()).withAlpha(0.60f));
        g.drawRoundedRectangle(x + 0.5f, y + 0.5f, w - 1.0f, h - 1.0f, radius, 1.0f);

        // Fill from bottom
        float fillH = h * juce::jlimit(0.0f, 1.0f, normalizedValue);
        float fillY = y + h - fillH;
        if (fillH > 1.0f)
        {
            g.setColour(fillColor.withAlpha(0.85f));
            // Clip fill to rounded track via path
            juce::Path fillPath;
            fillPath.addRoundedRectangle(x, y, w, h, radius);
            g.saveState();
            g.reduceClipRegion(fillPath);
            g.fillRect(x, fillY, w, fillH);
            g.restoreState();
        }

        // Label below strip
        g.setFont(GalleryFonts::label(7.0f));
        g.setColour(get(textMid()));
        g.drawText(label, juce::Rectangle<float>(x - 8.0f, y + h + 4.0f, w + 16.0f, 10.0f),
                   juce::Justification::centred, false);
    }

    // Pitch bend strip — center = no bend, spring-return visual
    void paintPitchBendStrip(juce::Graphics& g, float x, float y, float w, float h,
                             float normalizedPos, // 0 = full down, 0.5 = centre, 1 = full up
                             juce::Colour fillColor, const juce::String& label)
    {
        using namespace GalleryColors;

        float radius = w * 0.35f;

        // Track background
        g.setColour(get(borderGray()).withAlpha(0.35f));
        g.fillRoundedRectangle(x, y, w, h, radius);

        // Track outline
        g.setColour(get(borderGray()).withAlpha(0.60f));
        g.drawRoundedRectangle(x + 0.5f, y + 0.5f, w - 1.0f, h - 1.0f, radius, 1.0f);

        // Center detent line
        float centreY = y + h * 0.5f;
        g.setColour(get(textMid()).withAlpha(0.45f));
        g.drawHorizontalLine((int)centreY, x + 3.0f, x + w - 3.0f);

        // Fill from centre toward current position
        float handleY = y + h * (1.0f - juce::jlimit(0.0f, 1.0f, normalizedPos));
        float fillStart = juce::jmin(handleY, centreY);
        float fillEnd = juce::jmax(handleY, centreY);
        float fillH = fillEnd - fillStart;

        if (fillH > 1.0f)
        {
            juce::Path fillPath;
            fillPath.addRoundedRectangle(x, y, w, h, radius);
            g.saveState();
            g.reduceClipRegion(fillPath);
            g.setColour(fillColor.withAlpha(0.70f));
            g.fillRect(x, fillStart, w, fillH);
            g.restoreState();
        }

        // Handle indicator (bright 4pt horizontal bar)
        g.setColour(fillColor.brighter(0.3f));
        g.fillRoundedRectangle(x + 2.0f, handleY - 2.0f, w - 4.0f, 4.0f, 2.0f);

        // Label below
        g.setFont(GalleryFonts::label(7.0f));
        g.setColour(get(textMid()));
        g.drawText(label, juce::Rectangle<float>(x - 8.0f, y + h + 4.0f, w + 16.0f, 10.0f),
                   juce::Justification::centred, false);
    }

    // ── 2. Macro Meters ───────────────────────────────────────────────────────
    void paintMacroMeters(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        using namespace GalleryColors;

        auto bf = bounds.toFloat();

        // Section label
        g.setFont(GalleryFonts::label(8.0f));
        g.setColour(get(textMid()));
        g.drawText("MACROS", bf.withHeight(10.0f), juce::Justification::centred, false);

        float barAreaY = bf.getY() + 11.0f;
        float labelW = 38.0f; // width reserved for the left-side text label
        float barX = bf.getX() + labelW + 4.0f;
        float barW = bf.getWidth() - labelW - 12.0f;

        for (int i = 0; i < 4; ++i)
        {
            float rowY = barAreaY + i * (kMacroBarH + kMacroGap);
            juce::Colour col = macroColors[i];

            // Label
            g.setFont(GalleryFonts::label(8.0f));
            g.setColour(col);
            g.drawText(macroLabels[i], juce::Rectangle<float>(bf.getX() + 4.0f, rowY, labelW - 4.0f, (float)kMacroBarH),
                       juce::Justification::centredRight, false);

            // Track background
            g.setColour(col.withAlpha(0.15f));
            g.fillRoundedRectangle(barX, rowY, barW, (float)kMacroBarH, 3.0f);

            // Fill (width proportional to normalized param value 0..1)
            float fillW = barW * juce::jlimit(0.0f, 1.0f, macroValues[i]);
            if (fillW > 1.0f)
            {
                g.setColour(col.withAlpha(0.80f));
                g.fillRoundedRectangle(barX, rowY, fillW, (float)kMacroBarH, 3.0f);
            }

            // Track outline
            g.setColour(col.withAlpha(0.30f));
            g.drawRoundedRectangle(barX + 0.5f, rowY + 0.5f, barW - 1.0f, (float)kMacroBarH - 1.0f, 3.0f, 0.75f);
        }
    }

    // ── 3. XY Pad ─────────────────────────────────────────────────────────────
    void paintXYPad(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        using namespace GalleryColors;

        auto bf = bounds.toFloat();
        float rad = 8.0f;

        // Engine accent (fallback XO Gold)
        juce::Colour accentCol = juce::Colour(GalleryColors::xoGold);
        if (auto* eng = processor.getEngine(0))
            accentCol = eng->getAccentColour();

        // Background card
        g.setColour(get(slotBg()).withAlpha(0.85f));
        g.fillRoundedRectangle(bf, rad);

        g.setColour(get(borderGray()).withAlpha(0.50f));
        g.drawRoundedRectangle(bf.reduced(0.5f), rad, 1.0f);

        // Grid lines at 25/50/75 %
        g.setColour(get(borderGray()).withAlpha(0.22f));
        for (float frac : {0.25f, 0.50f, 0.75f})
        {
            float lx = bf.getX() + bf.getWidth() * frac;
            float ly = bf.getY() + bf.getHeight() * frac;
            // Vertical grid line
            g.drawLine(lx, bf.getY() + 4.0f, lx, bf.getBottom() - 4.0f, 1.0f);
            // Horizontal grid line
            g.drawLine(bf.getX() + 4.0f, ly, bf.getRight() - 4.0f, ly, 1.0f);
        }

        // Crosshair
        float cx = bf.getX() + bf.getWidth() * xyPos.x;
        float cy = bf.getY() + bf.getHeight() * (1.0f - xyPos.y); // y=0 at bottom

        g.setColour(accentCol.withAlpha(0.35f));
        g.drawLine(cx, bf.getY() + 4.0f, cx, bf.getBottom() - 4.0f, 0.75f);
        g.drawLine(bf.getX() + 4.0f, cy, bf.getRight() - 4.0f, cy, 0.75f);

        // Handle circle
        float circleR = 8.0f;
        g.setColour(accentCol);
        g.fillEllipse(cx - circleR, cy - circleR, circleR * 2.0f, circleR * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawEllipse(cx - circleR + 0.5f, cy - circleR + 0.5f, circleR * 2.0f - 1.0f, circleR * 2.0f - 1.0f, 1.0f);

        // "XY" unassigned watermark
        if (xyXParam == nullptr && xyYParam == nullptr)
        {
            g.setFont(GalleryFonts::label(9.0f));
            g.setColour(get(textMid()).withAlpha(0.35f));
            g.drawText("XY", bf, juce::Justification::centred, false);
        }

        // Focus ring
        if (hasKeyboardFocus(true))
            A11y::drawFocusRing(g, bf, rad);
    }

    // ── 4. Scale Selector ─────────────────────────────────────────────────────
    void paintScaleSelector(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        using namespace GalleryColors;

        auto bf = bounds.toFloat();
        int n = scaleNames.size();
        if (n == 0)
            return;

        float pillW = bf.getWidth() / (float)n;
        float pillH = bf.getHeight();

        juce::Colour accent = juce::Colour(GalleryColors::xoGold);

        for (int i = 0; i < n; ++i)
        {
            juce::Rectangle<float> pill(bf.getX() + i * pillW, bf.getY(), pillW, pillH);
            pill = pill.reduced(2.0f, 1.0f);

            bool isActive = (i == scaleIndex);

            if (isActive)
            {
                g.setColour(accent);
                g.fillRoundedRectangle(pill, 10.0f);
            }
            else
            {
                g.setColour(accent.withAlpha(0.13f));
                g.fillRoundedRectangle(pill, 10.0f);
                g.setColour(accent.withAlpha(0.28f));
                g.drawRoundedRectangle(pill.reduced(0.5f), 10.0f, 1.0f);
            }

            juce::Colour textCol = isActive ? juce::Colours::white : accent.withAlpha(0.55f);

            g.setFont(GalleryFonts::label(juce::jmax(6.5f, pillH * 0.34f)));
            g.setColour(textCol);
            g.drawText(scaleNames[i], pill, juce::Justification::centred, true);
        }

        if (hasKeyboardFocus(true))
            A11y::drawFocusRing(g, bf, 10.0f);
    }

    //==========================================================================
    // Mouse interaction helpers
    //==========================================================================

    // ── Expression strip dispatch ─────────────────────────────────────────────
    enum class ExprStrip
    {
        None,
        ModWheel,
        PitchBend
    };
    ExprStrip activeExprStrip = ExprStrip::None;

    void handleExpressionMouseDown(const juce::MouseEvent& e)
    {
        auto eb = expressionStripBounds().toFloat();

        // Determine which strip was hit
        float modX = eb.getCentreX() - kStripW * 2.0f - 8.0f;
        float pbX = eb.getCentreX() + 8.0f;
        float stripY = eb.getY() + 14.0f + (eb.getHeight() - 14.0f - kStripH) * 0.5f;

        juce::Rectangle<float> modRect(modX, stripY, (float)kStripW, (float)kStripH);
        juce::Rectangle<float> pbRect(pbX, stripY, (float)kStripW, (float)kStripH);

        if (modRect.contains(e.position))
        {
            activeExprStrip = ExprStrip::ModWheel;
            setModWheelFromY(e.position.y, modRect);
        }
        else if (pbRect.contains(e.position))
        {
            activeExprStrip = ExprStrip::PitchBend;
            pitchBendSpringActive = false;
            setPitchBendFromY(e.position.y, pbRect);
        }
    }

    void handleExpressionMouseDrag(const juce::MouseEvent& e)
    {
        auto eb = expressionStripBounds().toFloat();
        float stripY = eb.getY() + 14.0f + (eb.getHeight() - 14.0f - kStripH) * 0.5f;

        if (activeExprStrip == ExprStrip::ModWheel)
        {
            float modX = eb.getCentreX() - kStripW * 2.0f - 8.0f;
            juce::Rectangle<float> modRect(modX, stripY, (float)kStripW, (float)kStripH);
            setModWheelFromY(e.position.y, modRect);
        }
        else if (activeExprStrip == ExprStrip::PitchBend)
        {
            float pbX = eb.getCentreX() + 8.0f;
            juce::Rectangle<float> pbRect(pbX, stripY, (float)kStripW, (float)kStripH);
            setPitchBendFromY(e.position.y, pbRect);
        }
    }

    void setModWheelFromY(float mouseY, juce::Rectangle<float> stripRect)
    {
        // Fill from bottom → y=bottom → 0, y=top → 1
        float norm = 1.0f - juce::jlimit(0.0f, 1.0f, (mouseY - stripRect.getY()) / stripRect.getHeight());
        modWheelPos = norm;

        // Send to parameter if it exists, else send directly as MIDI CC1
        auto& apvts = processor.getAPVTS();
        if (auto* param = apvts.getParameter("modWheel"))
        {
            param->setValueNotifyingHost(norm);
        }
        else
        {
            // Direct MIDI CC1 injection
            auto msg = juce::MidiMessage::controllerEvent(1, 1, juce::roundToInt(norm * 127.0f));
            msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            processor.getMidiCollector().addMessageToQueue(msg);
        }

        repaint(expressionStripBounds());
    }

    void setPitchBendFromY(float mouseY, juce::Rectangle<float> stripRect)
    {
        // y=top → 1.0 (up bend), y=bottom → 0.0 (down bend), y=centre → 0.5
        float norm = 1.0f - juce::jlimit(0.0f, 1.0f, (mouseY - stripRect.getY()) / stripRect.getHeight());
        pitchBendPos = norm;
        sendPitchBendMidi(norm);
        repaint(expressionStripBounds());
    }

    void sendPitchBendMidi(float normPos)
    {
        // normPos: 0..1 → 0x0000..0x3FFF, with 0.5 = centre (0x2000)
        int pitchValue = juce::roundToInt(normPos * 16383.0f);          // 14-bit, 0x0000..0x3FFF
        auto msg = juce::MidiMessage::pitchWheel(1, pitchValue - 8192); // JUCE wants −8192..+8191
        msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
        processor.getMidiCollector().addMessageToQueue(msg);
    }

    // ── Macro bar drag ────────────────────────────────────────────────────────
    int activeMacroIndex = -1;
    bool macroMouseDrag = false;

    void handleMacroMouseDown(const juce::MouseEvent& e)
    {
        auto mb = macroMetersBounds().toFloat();
        float barAreaY = mb.getY() + 11.0f;
        float labelW = 38.0f;
        float barX = mb.getX() + labelW + 4.0f;
        float barW = mb.getWidth() - labelW - 12.0f;

        for (int i = 0; i < 4; ++i)
        {
            float rowY = barAreaY + i * (kMacroBarH + kMacroGap);
            juce::Rectangle<float> barRect(barX, rowY, barW, (float)kMacroBarH);
            // Extend hit area ±4pt vertically for easier targeting
            if (barRect.expanded(0.0f, 4.0f).contains(e.position))
            {
                activeMacroIndex = i;
                applyMacroDrag(e.position.x, barX, barW, i);
                break;
            }
        }
    }

    void handleMacroDrag(const juce::MouseEvent& e)
    {
        if (activeMacroIndex < 0)
            return;
        auto mb = macroMetersBounds().toFloat();
        float labelW = 38.0f;
        float barX = mb.getX() + labelW + 4.0f;
        float barW = mb.getWidth() - labelW - 12.0f;
        applyMacroDrag(e.position.x, barX, barW, activeMacroIndex);
    }

    void applyMacroDrag(float mouseX, float barX, float barW, int macroIndex)
    {
        float norm = juce::jlimit(0.0f, 1.0f, (mouseX - barX) / barW);
        macroValues[macroIndex] = norm;

        auto& apvts = processor.getAPVTS();
        if (auto* param = apvts.getParameter(macroParamIds[macroIndex]))
            param->setValueNotifyingHost(norm);

        repaint(macroMetersBounds());
    }

    // ── XY Pad drag ───────────────────────────────────────────────────────────
    bool xyDragging = false;

    void handleXYMouseDown(const juce::MouseEvent& e)
    {
        xyDragging = true;
        updateXYFromMouse(e.position);
    }

    void handleXYMouseDrag(const juce::MouseEvent& e) { updateXYFromMouse(e.position); }

    void updateXYFromMouse(juce::Point<float> mousePos)
    {
        auto xyb = xyPadBounds().toFloat();
        float nx = juce::jlimit(0.0f, 1.0f, (mousePos.x - xyb.getX()) / xyb.getWidth());
        float ny = 1.0f - juce::jlimit(0.0f, 1.0f, (mousePos.y - xyb.getY()) / xyb.getHeight());

        xyPos = {nx, ny};

        if (xyXParam != nullptr)
            xyXParam->setValueNotifyingHost(nx);
        if (xyYParam != nullptr)
            xyYParam->setValueNotifyingHost(ny);

        repaint(xyPadBounds());
    }

    // ── Scale selector click ──────────────────────────────────────────────────
    void handleScaleMouseDown(const juce::MouseEvent& e)
    {
        auto sb = scaleSelectorBounds().toFloat();
        int n = scaleNames.size();
        if (n == 0)
            return;

        float pillW = sb.getWidth() / (float)n;
        int clicked = (int)((e.position.x - sb.getX()) / pillW);
        clicked = juce::jlimit(0, n - 1, clicked);

        setScaleIndex(clicked);
    }

    //==========================================================================
    // State
    //==========================================================================
    XOceanusProcessor& processor;

    // Expression strip state
    float modWheelPos = 0.0f;  // 0..1 (normalized, 0 = no mod, 1 = max)
    float pitchBendPos = 0.5f; // 0..1 (0.5 = centre = no bend)
    bool pitchBendSpringActive = false;

    // Macro meter values (raw 0..1, polled from APVTS every 10 Hz)
    std::array<float, 4> macroValues{0.0f, 0.0f, 0.0f, 0.0f};

    // XY pad state
    juce::Point<float> xyPos{0.5f, 0.5f};           // 0..1 on each axis
    juce::RangedAudioParameter* xyXParam = nullptr; // assigned externally
    juce::RangedAudioParameter* xyYParam = nullptr;

    // Scale selector
    int scaleIndex = 0; // 0 = Chromatic (default)

    // Scale combo (parked, invisible — drives internal scaleIndex)
    juce::ComboBox scaleCombo;

    // Timer tick counter for 10 Hz gating inside 30 Hz timer
    int tickCount = 0;

    // Fix #387: idle tick counter for adaptive timer rate (30 Hz → 1 Hz after ~2s idle)
    int idleTickCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayControlPanel)
};

} // namespace xoceanus
