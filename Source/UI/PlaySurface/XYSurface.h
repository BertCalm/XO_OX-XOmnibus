// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

/*
    XYSurface.h
    ===========
    Wave 8 XY Surface play-surface tab.

    Implements the locked D3 design decisions (2026-04-25):
        A2: Engine + FX Chain params assignable to X and Y axes via right-click popup.
        B2: Five musical patterns: PULSE / DRIFT / TIDE / RIPPLE / CHAOS.
            Replaces the old CIRCLE / FIG-8 / SWEEP / RANDOM stubs in SurfaceRightPanel.
        C2: Tempo-sync default (1 bar), free-rate (Hz) toggle.

    This component does NOT modify SurfaceRightPanel.h, OceanView.h, or XOceanusEditor.h.
    Those files are wired by the host integrator. See Mount TODOs at bottom of this file.

    Signal flow:
        Pattern timer (30 Hz) → setXYPosition(x, y) → onXYChanged(x, y) callback
        Free mouse drag       → setXYPosition(x, y) → onXYChanged(x, y) callback

        Host wires onXYChanged → resolves assigned param IDs → setValueNotifyingHost()

    APVTS parameters (registered in XOceanusProcessor::createParameterLayout):
        Per slot (suffix _slot{0..3}):
            xy_pattern_slot{n}   — AudioParameterChoice: None/PULSE/DRIFT/TIDE/RIPPLE/CHAOS
            xy_speed_slot{n}     — AudioParameterFloat:  0.0–1.0 (default 0.5)
            xy_depth_slot{n}     — AudioParameterFloat:  0.0–1.0 (default 0.5)
            xy_sync_slot{n}      — AudioParameterChoice: Free/1bar-4/1bar-2/1bar/2bar/4bar
            xy_assignX_slot{n}   — AudioParameterChoice: index into canonical param list
            xy_assignY_slot{n}   — AudioParameterChoice: index into canonical param list
            xy_pos_x_slot{n}     — AudioParameterFloat:  0.0–1.0 (persists cursor position)
            xy_pos_y_slot{n}     — AudioParameterFloat:  0.0–1.0

    Canonical param enum (for xy_assignX/Y):
        0=None, 1=FilterCutoff, 2=FilterResonance, 3=LFORate, 4=LFODepth,
        5=EnvAttack, 6=EnvRelease, 7=Drive, 8=Macro1, 9=Macro2, 10=Macro3,
        11=Macro4, 12=FX1WetDry, 13=FX2WetDry, 14=FX3WetDry

    Namespace: xoceanus
    JUCE 8, C++17

    ── Mount TODOs (Wave 8 integration agent) ───────────────────────────────────
    TODO W8 mount: Wire onXYChanged in OceanView constructor:
        xyPanel_.onXYChanged = [this](float x, float y) { handleXYOutput(x, y); };

    TODO W8 mount: Implement handleXYOutput in OceanView:
        void handleXYOutput(float x, float y) {
            int slot = activeSlot_;
            auto* apvts = &processor_.getAPVTS();
            int xIdx = static_cast<int>(
                apvts->getParameter("xy_assignX_slot" + juce::String(slot))->getValue()
                * 14.f + 0.5f);
            int yIdx = static_cast<int>(
                apvts->getParameter("xy_assignY_slot" + juce::String(slot))->getValue()
                * 14.f + 0.5f);
            auto* px = apvts->getParameter(resolveXYParamId(slot, xIdx));
            auto* py = apvts->getParameter(resolveXYParamId(slot, yIdx));
            if (px) px->setValueNotifyingHost(px->convertTo0to1(x));
            if (py) py->setValueNotifyingHost(py->convertTo0to1(y));
        }

    TODO W8 mount: Implement resolveXYParamId(int slot, int canonIdx) in OceanView
        using the canonical mapping below (see resolveParamId() in this header).

    TODO W8 mount: Add XY APVTS parameters in XOceanusProcessor::createParameterLayout()
        — see "APVTS Parameter Registration" section at the bottom of this file for the
        exact push_back calls to add before `return layout;`.

    TODO W8 mount: Add #include "UI/PlaySurface/XYSurface.h" to OceanView or the host
        file that instantiates the play surface.

    TODO W8B mount (after Wave 5 C5): Add xyX_[4] and xyY_[4] atomics to
        XOceanusProcessor; register xyX/xyY as ModSource entries in ModMatrix.h.
    ─────────────────────────────────────────────────────────────────────────────
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/XYPatternGenerator.h"
#include "../GalleryColors.h"
#include <functional>
#include <cmath>
#include <array>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

//==============================================================================
/**
    XYSurface

    Wave 8 XY playing surface with:
      - 5 musical motion patterns (PULSE, DRIFT, TIDE, RIPPLE, CHAOS)
      - Per-axis param assignment via PopupMenu
      - Tempo-sync (default) or free-rate Hz toggle
      - MANUAL clutch: hold to override pattern with free mouse
*/
class XYSurface : public juce::Component, private juce::Timer
{
public:
    //==========================================================================
    // Canonical param indices for axis assignment (mirrors APVTS choices)
    enum class CanonParam : int
    {
        None           = 0,
        FilterCutoff   = 1,
        FilterRes      = 2,
        LFORate        = 3,
        LFODepth       = 4,
        EnvAttack      = 5,
        EnvRelease     = 6,
        Drive          = 7,
        Macro1         = 8,
        Macro2         = 9,
        Macro3         = 10,
        Macro4         = 11,
        FX1WetDry      = 12,
        FX2WetDry      = 13,
        FX3WetDry      = 14
    };
    static constexpr int kCanonParamCount = 15;

    //==========================================================================
    XYSurface()
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, true);
        setWantsKeyboardFocus(false);
        startTimerHz(30);
    }

    ~XYSurface() override { stopTimer(); }

    //==========================================================================
    // Configuration

    /// Set which processor slot this surface drives (0..3)
    void setSlot(int slotIdx) noexcept { slot_ = juce::jlimit(0, 3, slotIdx); }

    /// Engine accent colour for pattern glow / cursor tint
    void setAccentColor(juce::Colour c) { accent_ = c; repaint(); }

    /// Host BPM and current bar phase [0,1) — update from SharedTransport at paint rate
    void setTransport(float bpm, float barPhase) noexcept
    {
        bpm_      = bpm;
        barPhase_ = barPhase;
    }

    /// Resolver: given (slotIdx, canonIdx) → APVTS param ID string.
    /// Wire this in OceanView with a lambda capturing &processor_.
    /// Default impl returns an empty string (no APVTS backing).
    std::function<juce::String(int slotIdx, int canonIdx)> resolveParamId;

    /// Called whenever (x, y) changes — wire to handleXYOutput in OceanView.
    /// Values in [0, 1].
    std::function<void(float x, float y)> onXYChanged;

    //==========================================================================
    // Direct position control (e.g. from pattern driver → calls onXYChanged)
    void setXYPosition(float x, float y)
    {
        x_ = juce::jlimit(0.0f, 1.0f, x);
        y_ = juce::jlimit(0.0f, 1.0f, y);
        if (onXYChanged)
            onXYChanged(x_, y_);
        repaint();
    }

    float getX() const noexcept { return x_; }
    float getY() const noexcept { return y_; }

    //==========================================================================
    // APVTS attachment (optional — call after component is added to the UI tree)
    //
    // If non-null, the component reads pattern/speed/depth/sync/assign params
    // from APVTS on each timer tick. This keeps the UI in sync with automation
    // and preset load without the host polling.
    void attachAPVTS(juce::AudioProcessorValueTreeState* apvts) noexcept
    {
        apvts_ = apvts;
    }

    //==========================================================================
    // juce::Component

    void resized() override { computeBounds(); }

    void paint(juce::Graphics& g) override
    {
        paintBackground(g);
        paintXYPad(g);
        paintReadoutRow(g);
        paintPatternRow(g);
        paintControlRow(g);
    }

    void mouseDown(juce::MouseEvent const& e) override
    {
        auto pos = e.position;

        // Pattern pills
        int pill = hitTestPatternPill(pos.x, pos.y);
        if (pill >= 0)
        {
            int next = (patternSel_ == pill) ? 0 : pill; // toggle; 0=None
            patternSel_ = next;
            generator_.setPattern(static_cast<XYPatternGenerator::Pattern>(patternSel_));
            if (patternSel_ == 0)
                writePositionToAPVTS(); // freeze last position
            repaint();
            return;
        }

        // MANUAL clutch pill
        if (manualPillBounds_.contains(pos.x, pos.y))
        {
            manualClutch_ = true;
            repaint();
            return;
        }

        // SYNC toggle pill
        if (syncPillBounds_.contains(pos.x, pos.y))
        {
            // Cycle through sync modes
            int next = (static_cast<int>(generator_.getSyncMode()) + 1) % 6;
            generator_.setSyncMode(static_cast<XYPatternGenerator::SyncMode>(next));
            repaint();
            return;
        }

        // ASSIGN X / ASSIGN Y pills
        if (assignXBounds_.contains(pos.x, pos.y))
        {
            showAssignMenu(true);
            return;
        }
        if (assignYBounds_.contains(pos.x, pos.y))
        {
            showAssignMenu(false);
            return;
        }

        // Speed / Depth drags — handled in mouseDrag
        if (speedSliderBounds_.contains(pos.x, pos.y))
        {
            dragTarget_ = DragTarget::Speed;
            dragStartX_ = pos.x;
            dragStartVal_ = generator_.getSpeed();
            return;
        }
        if (depthSliderBounds_.contains(pos.x, pos.y))
        {
            dragTarget_ = DragTarget::Depth;
            dragStartX_ = pos.x;
            dragStartVal_ = generator_.getDepth();
            return;
        }

        // XY pad — only if MANUAL clutch or no pattern active
        if (xyPadBounds_.contains(pos.x, pos.y) && (manualClutch_ || patternSel_ == 0))
        {
            dragging_ = true;
            updateXYFromMouse(pos);
        }
    }

    void mouseDrag(juce::MouseEvent const& e) override
    {
        auto pos = e.position;

        if (dragTarget_ == DragTarget::Speed || dragTarget_ == DragTarget::Depth)
        {
            float delta = (pos.x - dragStartX_) / 120.0f;
            float newVal = juce::jlimit(0.0f, 1.0f, dragStartVal_ + delta);
            if (dragTarget_ == DragTarget::Speed)
                generator_.setSpeed(newVal);
            else
                generator_.setDepth(newVal);
            repaint();
            return;
        }

        if (dragging_)
            updateXYFromMouse(pos);
    }

    void mouseUp(juce::MouseEvent const& /*e*/) override
    {
        dragging_    = false;
        manualClutch_ = false;
        dragTarget_  = DragTarget::None;
        repaint();
    }

private:
    //==========================================================================
    // Dimensions
    static constexpr float kPadMargin  = 10.0f;
    static constexpr float kReadoutH   = 36.0f;
    static constexpr float kPatternH   = 26.0f;
    static constexpr float kControlH   = 22.0f;

    // Teal constants (matching SurfaceRightPanel)
    static constexpr uint8_t kTealR = 60,  kTealG = 180, kTealB = 170;
    static constexpr uint8_t kSaltR = 200, kSaltG = 204, kSaltB = 216;

    //==========================================================================
    // State
    int   slot_          = 0;
    float x_             = 0.5f;
    float y_             = 0.5f;
    bool  dragging_      = false;
    bool  manualClutch_  = false;
    int   patternSel_    = 0; // 0=None, 1–5 map to Pattern enum

    int   assignXIdx_    = 0; // CanonParam index
    int   assignYIdx_    = 0;

    float bpm_           = 120.0f;
    float barPhase_      = 0.0f;

    juce::Colour accent_ = juce::Colour(kTealR, kTealG, kTealB);

    juce::AudioProcessorValueTreeState* apvts_ = nullptr;

    XYPatternGenerator generator_;

    enum class DragTarget { None, Speed, Depth };
    DragTarget dragTarget_  = DragTarget::None;
    float      dragStartX_  = 0.0f;
    float      dragStartVal_= 0.0f;

    //==========================================================================
    // Bounds (computed in resized / computeBounds)
    juce::Rectangle<float> xyPadBounds_{};
    juce::Rectangle<float> readoutRowBounds_{};
    juce::Rectangle<float> patternRowBounds_{};
    juce::Rectangle<float> controlRowBounds_{};

    juce::Rectangle<float> assignXBounds_{};
    juce::Rectangle<float> assignYBounds_{};
    juce::Rectangle<float> manualPillBounds_{};
    juce::Rectangle<float> syncPillBounds_{};
    juce::Rectangle<float> speedSliderBounds_{};
    juce::Rectangle<float> depthSliderBounds_{};

    static constexpr int kPatternCount = 5; // PULSE..CHAOS
    std::array<juce::Rectangle<float>, kPatternCount> patternPillBounds_{};

    //==========================================================================
    void computeBounds()
    {
        auto b = getLocalBounds().toFloat();

        float bottom = b.getBottom();
        controlRowBounds_ = juce::Rectangle<float>(b.getX() + kPadMargin, bottom - kControlH - 4.0f,
                                                    b.getWidth() - kPadMargin * 2.0f, kControlH);
        patternRowBounds_ = juce::Rectangle<float>(b.getX() + kPadMargin,
                                                    controlRowBounds_.getY() - kPatternH - 4.0f,
                                                    b.getWidth() - kPadMargin * 2.0f, kPatternH);
        readoutRowBounds_ = juce::Rectangle<float>(b.getX() + kPadMargin,
                                                    patternRowBounds_.getY() - kReadoutH - 4.0f,
                                                    b.getWidth() - kPadMargin * 2.0f, kReadoutH);
        xyPadBounds_      = juce::Rectangle<float>(b.getX() + kPadMargin, b.getY() + kPadMargin,
                                                    b.getWidth() - kPadMargin * 2.0f,
                                                    readoutRowBounds_.getY() - b.getY() - kPadMargin * 2.0f);
    }

    //==========================================================================
    // Hit testing
    int hitTestPatternPill(float px, float py) const
    {
        for (int i = 0; i < kPatternCount; ++i)
            if (patternPillBounds_[i].contains(px, py))
                return i + 1; // 1-indexed to match Pattern enum
        return -1;
    }

    //==========================================================================
    void updateXYFromMouse(juce::Point<float> pos)
    {
        auto& r = xyPadBounds_;
        if (r.getWidth() <= 0.0f || r.getHeight() <= 0.0f)
            return;
        x_ = juce::jlimit(0.0f, 1.0f, (pos.x - r.getX()) / r.getWidth());
        y_ = juce::jlimit(0.0f, 1.0f, (pos.y - r.getY()) / r.getHeight());
        if (onXYChanged)
            onXYChanged(x_, y_);
        repaint();
    }

    //==========================================================================
    // APVTS helpers

    void syncFromAPVTS()
    {
        if (!apvts_)
            return;
        auto suffix = juce::String("_slot") + juce::String(slot_);

        auto* pPat = apvts_->getParameter("xy_pattern" + suffix);
        if (pPat) patternSel_ = juce::roundToInt(pPat->getValue() * 5.0f);

        auto* pSpd = apvts_->getRawParameterValue("xy_speed" + suffix);
        if (pSpd) generator_.setSpeed(pSpd->load());

        auto* pDpt = apvts_->getRawParameterValue("xy_depth" + suffix);
        if (pDpt) generator_.setDepth(pDpt->load());

        auto* pSync = apvts_->getParameter("xy_sync" + suffix);
        if (pSync)
            generator_.setSyncMode(
                static_cast<XYPatternGenerator::SyncMode>(juce::roundToInt(pSync->getValue() * 5.0f)));

        auto* pAX = apvts_->getParameter("xy_assignX" + suffix);
        if (pAX) assignXIdx_ = juce::roundToInt(pAX->getValue() * 14.0f);

        auto* pAY = apvts_->getParameter("xy_assignY" + suffix);
        if (pAY) assignYIdx_ = juce::roundToInt(pAY->getValue() * 14.0f);
    }

    void writePositionToAPVTS()
    {
        if (!apvts_)
            return;
        auto suffix = juce::String("_slot") + juce::String(slot_);
        auto* px = apvts_->getParameter("xy_pos_x" + suffix);
        auto* py = apvts_->getParameter("xy_pos_y" + suffix);
        if (px) px->setValueNotifyingHost(x_);
        if (py) py->setValueNotifyingHost(y_);
    }

    //==========================================================================
    // Timer (30 Hz) — advance pattern, sync from APVTS, repaint
    void timerCallback() override
    {
        syncFromAPVTS();

        if (patternSel_ > 0 && !manualClutch_)
        {
            generator_.setPattern(static_cast<XYPatternGenerator::Pattern>(patternSel_));
            generator_.tick(1.0f / 30.0f, bpm_, barPhase_);
            x_ = generator_.getX();
            y_ = generator_.getY();
            if (onXYChanged)
                onXYChanged(x_, y_);
        }
        else
        {
            generator_.setPattern(XYPatternGenerator::Pattern::None);
        }
        repaint();
    }

    //==========================================================================
    // Assign param menu
    void showAssignMenu(bool isXAxis)
    {
        static const char* kParamLabels[kCanonParamCount] = {
            "None",
            "Filter Cutoff", "Filter Resonance",
            "LFO Rate",      "LFO Depth",
            "Env Attack",    "Env Release",
            "Drive",
            "Macro 1 (TONE)", "Macro 2 (TIDE)", "Macro 3 (COUPLE)", "Macro 4 (DEPTH)",
            "FX Slot 1 Wet/Dry", "FX Slot 2 Wet/Dry", "FX Slot 3 Wet/Dry"
        };

        juce::PopupMenu engineMenu;
        engineMenu.addItem(1, "None");
        for (int i = 1; i <= 11; ++i)
            engineMenu.addItem(i + 1, kParamLabels[i]);

        juce::PopupMenu fxMenu;
        for (int i = 12; i < kCanonParamCount; ++i)
            fxMenu.addItem(i + 1, kParamLabels[i]);

        juce::PopupMenu menu;
        menu.addSubMenu("Engine Params", engineMenu);
        menu.addSubMenu("Starboard FX",  fxMenu);

        int current = isXAxis ? assignXIdx_ : assignYIdx_;
        (void)current; // future: show checkmark on current selection

        auto options = juce::PopupMenu::Options()
                           .withTargetComponent(this)
                           .withPreferredPopupDirection(juce::PopupMenu::Options::PopupDirection::downwards);

        menu.showMenuAsync(options, [this, isXAxis](int result) {
            if (result <= 0)
                return;
            int idx = result - 1; // back to 0-based canon index
            if (isXAxis)
                assignXIdx_ = idx;
            else
                assignYIdx_ = idx;

            // Persist choice to APVTS
            if (apvts_)
            {
                auto suffix = juce::String("_slot") + juce::String(slot_);
                auto* p = apvts_->getParameter(isXAxis ? "xy_assignX" + suffix : "xy_assignY" + suffix);
                if (p) p->setValueNotifyingHost(static_cast<float>(idx) / 14.0f);
            }
            repaint();
        });
    }

    //==========================================================================
    // Short display labels for each canonical param
    static const char* shortParamLabel(int canonIdx) noexcept
    {
        switch (canonIdx)
        {
            case 0:  return "NONE";
            case 1:  return "CUTOFF";
            case 2:  return "RES";
            case 3:  return "LFO RATE";
            case 4:  return "LFO DEPTH";
            case 5:  return "ATTACK";
            case 6:  return "RELEASE";
            case 7:  return "DRIVE";
            case 8:  return "MACRO1";
            case 9:  return "MACRO2";
            case 10: return "MACRO3";
            case 11: return "MACRO4";
            case 12: return "FX1 WET";
            case 13: return "FX2 WET";
            case 14: return "FX3 WET";
            default: return "?";
        }
    }

    //==========================================================================
    // Sync mode display label
    static const char* syncLabel(XYPatternGenerator::SyncMode m) noexcept
    {
        switch (m)
        {
            case XYPatternGenerator::SyncMode::Free:    return "FREE";
            case XYPatternGenerator::SyncMode::Bar_1_4: return "1/4";
            case XYPatternGenerator::SyncMode::Bar_1_2: return "1/2";
            case XYPatternGenerator::SyncMode::Bar_1:   return "1bar";
            case XYPatternGenerator::SyncMode::Bar_2:   return "2bar";
            case XYPatternGenerator::SyncMode::Bar_4:   return "4bar";
            default:                                     return "?";
        }
    }

    //==========================================================================
    // Painting

    void paintBackground(juce::Graphics& g)
    {
        g.setColour(juce::Colour(0xFF16181e));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    }

    void paintXYPad(juce::Graphics& g)
    {
        auto& xr = xyPadBounds_;
        if (xr.getWidth() <= 0.0f)
            return;

        // Background
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.025f));
        g.fillRoundedRectangle(xr, 10.0f);
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.06f));
        g.drawRoundedRectangle(xr, 10.0f, 1.0f);

        // Crosshair
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.035f));
        float midX = xr.getX() + xr.getWidth()  * 0.5f;
        float midY = xr.getY() + xr.getHeight() * 0.5f;
        g.drawHorizontalLine(static_cast<int>(midY), xr.getX(), xr.getRight());
        g.drawVerticalLine  (static_cast<int>(midX), xr.getY(), xr.getBottom());

        // Pattern-specific visual overlays
        if (patternSel_ == static_cast<int>(XYPatternGenerator::Pattern::RIPPLE))
        {
            // Circular glow halo at seed point — 15px, 20% accent alpha
            float sx = xr.getX() + 0.5f * xr.getWidth();
            float sy = xr.getY() + 0.5f * xr.getHeight();
            float hr = 15.0f;
            juce::ColourGradient halo(accent_.withAlpha(0.20f), sx, sy,
                                      accent_.withAlpha(0.0f),  sx + hr * 2.0f, sy, true);
            g.setGradientFill(halo);
            g.fillEllipse(sx - hr * 2.0f, sy - hr * 2.0f, hr * 4.0f, hr * 4.0f);
        }

        if (patternSel_ == static_cast<int>(XYPatternGenerator::Pattern::CHAOS))
        {
            // Three dots in triangle arrangement — top-right corner, 40% accent
            float dx = xr.getRight() - 14.0f;
            float dy = xr.getY() + 8.0f;
            g.setColour(accent_.withAlpha(0.40f));
            g.fillEllipse(dx - 3.0f, dy - 3.0f, 6.0f, 6.0f);
            g.fillEllipse(dx + 5.0f, dy + 3.0f, 4.5f, 4.5f);
            g.fillEllipse(dx - 8.0f, dy + 3.0f, 4.5f, 4.5f);
        }

        // Cursor
        float cursorX = xr.getX() + x_ * xr.getWidth();
        float cursorY = xr.getY() + y_ * xr.getHeight();
        float cursorR = 7.0f;

        juce::Colour cursorColor = (patternSel_ > 0 && !manualClutch_)
                                       ? accent_
                                       : juce::Colour(kTealR, kTealG, kTealB);

        juce::ColourGradient glow(cursorColor.withAlpha(0.25f), cursorX, cursorY,
                                  cursorColor.withAlpha(0.0f),  cursorX + cursorR * 2.2f, cursorY, true);
        g.setGradientFill(glow);
        g.fillEllipse(cursorX - cursorR * 1.8f, cursorY - cursorR * 1.8f,
                      cursorR * 3.6f,            cursorR * 3.6f);

        g.setColour(cursorColor.withAlpha(0.45f));
        g.fillEllipse(cursorX - cursorR, cursorY - cursorR, cursorR * 2.0f, cursorR * 2.0f);
        g.setColour(cursorColor.withAlpha(0.80f));
        g.drawEllipse(cursorX - cursorR, cursorY - cursorR, cursorR * 2.0f, cursorR * 2.0f, 2.0f);

        // TIDE pattern: ghost trace ellipse hint
        if (patternSel_ == static_cast<int>(XYPatternGenerator::Pattern::TIDE))
        {
            float traceW = xr.getWidth() * generator_.getDepth() * 0.5f;
            float traceH = xr.getHeight() * generator_.getDepth() * 0.25f;
            g.setColour(accent_.withAlpha(0.08f));
            g.drawEllipse(xr.getCentreX() - traceW, xr.getCentreY() - traceH,
                          traceW * 2.0f, traceH * 2.0f, 1.0f);
        }
    }

    void paintReadoutRow(juce::Graphics& g)
    {
        auto& rr    = readoutRowBounds_;
        float halfW = rr.getWidth() * 0.5f;
        auto xSec   = juce::Rectangle<float>(rr.getX(),          rr.getY(), halfW, rr.getHeight());
        auto ySec   = juce::Rectangle<float>(rr.getX() + halfW,  rr.getY(), halfW, rr.getHeight());

        bool sameParam = (assignXIdx_ != 0 && assignXIdx_ == assignYIdx_);

        paintAxisReadout(g, xSec, "X", assignXIdx_, x_, assignXBounds_);
        paintAxisReadout(g, ySec, "Y", assignYIdx_, y_, assignYBounds_);

        // Same-param warning glyph ⚠
        if (sameParam)
        {
            float wx = rr.getCentreX() - 5.0f;
            float wy = rr.getY() + (rr.getHeight() - 12.0f) * 0.5f;
            g.setFont(9.0f);
            g.setColour(juce::Colour(0xFFD4AC0D).withAlpha(0.70f));
            g.drawText(juce::CharPointer_UTF8("\xe2\x9a\xa0"), // ⚠ U+26A0
                       static_cast<int>(wx), static_cast<int>(wy), 12, 12,
                       juce::Justification::centred, false);
        }
    }

    void paintAxisReadout(juce::Graphics& g,
                          juce::Rectangle<float> sec,
                          const char* axisLabel,
                          int canonIdx,
                          float value,
                          juce::Rectangle<float>& btnBoundsOut)
    {
        float sX = sec.getX() + 4.0f;
        float sW = sec.getWidth() - 8.0f;

        bool assigned = (canonIdx > 0);
        juce::Colour teal(kTealR, kTealG, kTealB);

        // Axis + param label
        g.setFont(GalleryFonts::label(8.0f));
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.30f));
        juce::String axisStr = juce::String(axisLabel) + ": " + shortParamLabel(canonIdx);
        g.drawText(axisStr, static_cast<int>(sX), static_cast<int>(sec.getY()),
                   static_cast<int>(sW), 14, juce::Justification::centredLeft, false);

        // Value percentage
        g.setFont(GalleryFonts::value(14.0f));
        g.setColour(teal.withAlpha(0.80f));
        int pct = static_cast<int>(value * 100.0f + 0.5f);
        g.drawText(juce::String(pct) + "%",
                   static_cast<int>(sX), static_cast<int>(sec.getY() + 14.0f),
                   static_cast<int>(sW * 0.5f), 18, juce::Justification::centredLeft, false);

        // ASSIGN button pill
        float btnW = 56.0f, btnH = 16.0f;
        float btnX = sec.getRight() - btnW - 4.0f;
        float btnY = sec.getY() + (sec.getHeight() - btnH) * 0.5f;
        btnBoundsOut = juce::Rectangle<float>(btnX, btnY, btnW, btnH);

        if (assigned)
        {
            g.setColour(teal.withAlpha(0.18f));
            g.fillRoundedRectangle(btnBoundsOut, 5.0f);
            g.setColour(teal.withAlpha(0.60f));
            g.drawRoundedRectangle(btnBoundsOut, 5.0f, 1.0f);
            g.setFont(GalleryFonts::label(9.0f));
            g.setColour(teal.withAlpha(0.90f));
            g.drawText(shortParamLabel(canonIdx),
                       static_cast<int>(btnX), static_cast<int>(btnY),
                       static_cast<int>(btnW), static_cast<int>(btnH),
                       juce::Justification::centred, false);
        }
        else
        {
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.07f));
            g.fillRoundedRectangle(btnBoundsOut, 5.0f);
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.07f));
            g.drawRoundedRectangle(btnBoundsOut, 5.0f, 1.0f);
            g.setFont(GalleryFonts::label(9.0f));
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.35f));
            juce::String assignLabel = juce::String("ASSIGN ") + axisLabel;
            g.drawText(assignLabel,
                       static_cast<int>(btnX), static_cast<int>(btnY),
                       static_cast<int>(btnW), static_cast<int>(btnH),
                       juce::Justification::centred, false);
        }
    }

    void paintPatternRow(juce::Graphics& g)
    {
        auto& ar = patternRowBounds_;

        // "AUTO:" label
        g.setFont(GalleryFonts::label(8.0f));
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.30f));
        float labelW = 34.0f;
        g.drawText("AUTO:", static_cast<int>(ar.getX()), static_cast<int>(ar.getY()),
                   static_cast<int>(labelW), static_cast<int>(ar.getHeight()),
                   juce::Justification::centredLeft, false);

        static const char* kPatternLabels[kPatternCount] = { "PULSE", "DRIFT", "TIDE", "RIPPLE", "CHAOS" };

        juce::Font pillFont = GalleryFonts::heading(8.0f);
        g.setFont(pillFont);

        float pillH   = 18.0f;
        float pillPad = 4.0f;
        float pillY   = ar.getY() + (ar.getHeight() - pillH) * 0.5f;
        float curX    = ar.getX() + labelW + 4.0f;

        for (int i = 0; i < kPatternCount; ++i)
        {
            float pillW = pillFont.getStringWidthFloat(juce::String(kPatternLabels[i])) + pillPad * 3.0f;
            patternPillBounds_[i] = juce::Rectangle<float>(curX, pillY, pillW, pillH);
            curX += pillW + 4.0f;
        }

        for (int i = 0; i < kPatternCount; ++i)
        {
            auto& pb  = patternPillBounds_[i];
            bool  sel = (patternSel_ == i + 1); // pill i+1 → Pattern enum i+1

            if (sel)
            {
                g.setColour(accent_.withAlpha(0.20f));
                g.fillRoundedRectangle(pb, 4.0f);
                g.setColour(accent_.withAlpha(0.40f));
                g.drawRoundedRectangle(pb, 4.0f, 1.0f);
                g.setColour(accent_.withAlpha(0.90f));
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
            g.drawText(juce::String(kPatternLabels[i]),
                       static_cast<int>(pb.getX()), static_cast<int>(pb.getY()),
                       static_cast<int>(pb.getWidth()), static_cast<int>(pb.getHeight()),
                       juce::Justification::centred, false);
        }

        // MANUAL clutch pill (far left after AUTO: label)
        float mW = 44.0f, mH = 16.0f;
        float mX = ar.getRight() - mW;
        float mY = ar.getY() + (ar.getHeight() - mH) * 0.5f;
        manualPillBounds_ = juce::Rectangle<float>(mX, mY, mW, mH);

        if (manualClutch_)
        {
            g.setColour(juce::Colour(0xFFE9C46A).withAlpha(0.25f)); // XO Gold
            g.fillRoundedRectangle(manualPillBounds_, 4.0f);
            g.setColour(juce::Colour(0xFFE9C46A).withAlpha(0.60f));
            g.drawRoundedRectangle(manualPillBounds_, 4.0f, 1.0f);
            g.setColour(juce::Colour(0xFFE9C46A).withAlpha(0.90f));
        }
        else
        {
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.05f));
            g.fillRoundedRectangle(manualPillBounds_, 4.0f);
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.07f));
            g.drawRoundedRectangle(manualPillBounds_, 4.0f, 1.0f);
            g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.35f));
        }
        g.setFont(GalleryFonts::label(8.0f));
        g.drawText("MANUAL", static_cast<int>(mX), static_cast<int>(mY),
                   static_cast<int>(mW), static_cast<int>(mH),
                   juce::Justification::centred, false);
    }

    void paintControlRow(juce::Graphics& g)
    {
        auto& cr = controlRowBounds_;

        // SPD mini-slider
        float spdX    = cr.getX();
        float spdLblW = 22.0f;
        float sldW    = 60.0f;
        float sldH    = 8.0f;
        float sldY    = cr.getY() + (cr.getHeight() - sldH) * 0.5f;

        g.setFont(GalleryFonts::label(8.0f));
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.30f));
        g.drawText("SPD", static_cast<int>(spdX), static_cast<int>(cr.getY()),
                   static_cast<int>(spdLblW), static_cast<int>(cr.getHeight()),
                   juce::Justification::centredLeft, false);

        speedSliderBounds_ = juce::Rectangle<float>(spdX + spdLblW, sldY, sldW, sldH);
        paintMiniSlider(g, speedSliderBounds_, generator_.getSpeed());

        // DPT mini-slider
        float dptX    = spdX + spdLblW + sldW + 10.0f;
        float dptLblW = 22.0f;
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.30f));
        g.drawText("DPT", static_cast<int>(dptX), static_cast<int>(cr.getY()),
                   static_cast<int>(dptLblW), static_cast<int>(cr.getHeight()),
                   juce::Justification::centredLeft, false);

        depthSliderBounds_ = juce::Rectangle<float>(dptX + dptLblW, sldY, sldW, sldH);
        paintMiniSlider(g, depthSliderBounds_, generator_.getDepth());

        // SYNC pill (cycles: FREE / 1/4 / 1/2 / 1bar / 2bar / 4bar)
        float spW = 36.0f, spH = 16.0f;
        float spX = cr.getRight() - spW;
        float spY = cr.getY() + (cr.getHeight() - spH) * 0.5f;
        syncPillBounds_ = juce::Rectangle<float>(spX, spY, spW, spH);

        bool isSynced = (generator_.getSyncMode() != XYPatternGenerator::SyncMode::Free);
        juce::Colour syncColor = isSynced ? accent_ : juce::Colour(kSaltR, kSaltG, kSaltB);

        g.setColour(syncColor.withAlpha(isSynced ? 0.18f : 0.05f));
        g.fillRoundedRectangle(syncPillBounds_, 4.0f);
        g.setColour(syncColor.withAlpha(isSynced ? 0.50f : 0.07f));
        g.drawRoundedRectangle(syncPillBounds_, 4.0f, 1.0f);
        g.setFont(GalleryFonts::label(8.0f));
        g.setColour(syncColor.withAlpha(isSynced ? 0.90f : 0.35f));
        g.drawText(syncLabel(generator_.getSyncMode()),
                   static_cast<int>(spX), static_cast<int>(spY),
                   static_cast<int>(spW), static_cast<int>(spH),
                   juce::Justification::centred, false);
    }

    void paintMiniSlider(juce::Graphics& g,
                         const juce::Rectangle<float>& bounds,
                         float value)
    {
        // Track
        g.setColour(juce::Colour(kSaltR, kSaltG, kSaltB).withAlpha(0.10f));
        g.fillRoundedRectangle(bounds, 3.0f);

        // Fill
        float fillW = bounds.getWidth() * juce::jlimit(0.0f, 1.0f, value);
        g.setColour(accent_.withAlpha(0.55f));
        g.fillRoundedRectangle(juce::Rectangle<float>(bounds.getX(), bounds.getY(), fillW, bounds.getHeight()), 3.0f);

        // Thumb
        float tx = bounds.getX() + fillW - 3.0f;
        float ty = bounds.getY() - 1.0f;
        g.setColour(accent_.withAlpha(0.90f));
        g.fillRoundedRectangle(tx, ty, 6.0f, bounds.getHeight() + 2.0f, 2.0f);
    }

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYSurface)
};

} // namespace xoceanus

/*
 ══════════════════════════════════════════════════════════════════════════════
  APVTS Parameter Registration — add these to XOceanusProcessor::createParameterLayout()
  before the `return layout;` statement (after the Wave 5 C1 sequencer block).

  ── Wave 8: XY Surface parameters (8 params × 4 slots = 32 params) ──────────
  {
      const juce::StringArray kXYPatterns {"None","PULSE","DRIFT","TIDE","RIPPLE","CHAOS"};
      const juce::StringArray kXYSync     {"Free","1bar/4","1bar/2","1bar","2bar","4bar"};
      const juce::StringArray kXYAssign   {
          "None","Filter Cutoff","Filter Res","LFO Rate","LFO Depth",
          "Env Attack","Env Release","Drive",
          "Macro1","Macro2","Macro3","Macro4",
          "FX1 Wet","FX2 Wet","FX3 Wet"
      };
      for (int s = 0; s < 4; ++s)
      {
          const juce::String sfx = "_slot" + juce::String(s);
          params.push_back(std::make_unique<juce::AudioParameterChoice>(
              juce::ParameterID("xy_pattern" + sfx, 1),
              "XY Pattern Slot " + juce::String(s + 1), kXYPatterns, 0));
          params.push_back(std::make_unique<juce::AudioParameterFloat>(
              juce::ParameterID("xy_speed"   + sfx, 1),
              "XY Speed Slot "   + juce::String(s + 1),
              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
          params.push_back(std::make_unique<juce::AudioParameterFloat>(
              juce::ParameterID("xy_depth"   + sfx, 1),
              "XY Depth Slot "   + juce::String(s + 1),
              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
          params.push_back(std::make_unique<juce::AudioParameterChoice>(
              juce::ParameterID("xy_sync"    + sfx, 1),
              "XY Sync Slot "    + juce::String(s + 1), kXYSync, 3)); // default: 1bar
          params.push_back(std::make_unique<juce::AudioParameterChoice>(
              juce::ParameterID("xy_assignX" + sfx, 1),
              "XY Assign X Slot "+ juce::String(s + 1), kXYAssign, 0));
          params.push_back(std::make_unique<juce::AudioParameterChoice>(
              juce::ParameterID("xy_assignY" + sfx, 1),
              "XY Assign Y Slot "+ juce::String(s + 1), kXYAssign, 0));
          params.push_back(std::make_unique<juce::AudioParameterFloat>(
              juce::ParameterID("xy_pos_x"   + sfx, 1),
              "XY Pos X Slot "   + juce::String(s + 1),
              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
          params.push_back(std::make_unique<juce::AudioParameterFloat>(
              juce::ParameterID("xy_pos_y"   + sfx, 1),
              "XY Pos Y Slot "   + juce::String(s + 1),
              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
      }
  }
 ══════════════════════════════════════════════════════════════════════════════
*/
