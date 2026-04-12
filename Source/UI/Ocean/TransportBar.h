// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// TransportBar.h — Submarine-style 28 px transport + status bar for the XOceanus Ocean View.
//
// The bar sits at the very bottom of the plugin window. It renders two zones in a single
// horizontal row:
//
//   LEFT  — Transport strip: Play, BPM (drag-editable), TAP, Time Sig, Sync pills (INT/HOST/AUTO)
//   RIGHT — Status info: Voices count, COUPLE button, CPU meter, MIDI flash dot, status dot
//
// All values are pushed from the editor's timerCallback via the public setter API:
//   setBpm(), setVoiceCount(), setCpuPercent(), setMidiFlash(), setPlaying()
//
// User interactions fire std::function callbacks so the parent editor can wire them to
// the plugin state without any upward dependency from this component.
//
// Interaction details:
//   - Play button: click toggles → fires onPlayToggled
//   - BPM region: vertical drag (up = +BPM, down = -BPM, 0.5 BPM per pixel)
//   - TAP button: records last 5 timestamps; gap > 2000 ms resets; avg interval → BPM → fires onBpmChanged
//   - Time sig numerator: click cycles [2,3,4,5,6,7]
//   - Time sig denominator: click cycles [2,4,8]
//   - Sync pills: click a pill to activate → fires (no separate callback; parent reads back via poll)
//   - COUPLE button: click fires onCoupleClicked
//
// Timer runs at 10 Hz — drives MIDI flash decay and any future animation.
//
// File is entirely self-contained (header-only inline implementation) following
// the XOceanus convention for UI sub-components.

#include <juce_gui_basics/juce_gui_basics.h>
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
    TransportBar

    Submarine-style 28 px horizontal strip at the bottom of the Ocean View.
    See file header for full documentation.
*/
class TransportBar : public juce::Component,
                     private juce::Timer
{
public:
    //==========================================================================
    static constexpr int kBarHeight = 28;

    //==========================================================================
    TransportBar()
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, true);

        // 10 Hz timer — drives MIDI flash decay.
        startTimerHz(10);
    }

    ~TransportBar() override
    {
        stopTimer();
    }

    //==========================================================================
    // Public setter API — call from editor timerCallback (message thread).

    void setBpm(double bpm)
    {
        if (bpm_ != bpm)
        {
            bpm_ = bpm;
            repaint();
        }
    }

    void setVoiceCount(int count)
    {
        if (voiceCount_ != count)
        {
            voiceCount_ = count;
            repaint();
        }
    }

    void setCpuPercent(float pct)
    {
        if (cpuPercent_ != pct)
        {
            cpuPercent_ = pct;
            repaint();
        }
    }

    /// Call this whenever MIDI activity is detected — starts the flash decay.
    void setMidiFlash()
    {
        midiFlashAlpha_ = 1.0f;
        repaint();
    }

    void setPlaying(bool playing)
    {
        if (playing_ != playing)
        {
            playing_ = playing;
            repaint();
        }
    }

    //==========================================================================
    // Callbacks — parent wires these in the editor constructor.

    std::function<void()>         onPlayToggled;
    std::function<void()>         onCoupleClicked;
    std::function<void(double)>   onBpmChanged;
    std::function<void(int, int)> onTimeSigChanged;

private:
    //==========================================================================
    // Sync mode enum
    enum class SyncMode { Int = 0, Host, Auto, NumModes };

    //==========================================================================
    // Hit-test regions — rebuilt each paint() / resized() call.

    struct Region
    {
        juce::Rectangle<float> bounds;
        int                    id;   // arbitrary tag used to identify region
    };

    enum RegionId
    {
        kRegPlay      = 0,
        kRegBpm       = 1,
        kRegTap       = 2,
        kRegTimeSigN  = 3,
        kRegTimeSigD  = 4,
        kRegSyncInt   = 5,
        kRegSyncHost  = 6,
        kRegSyncAuto  = 7,
        kRegCouple    = 8,
    };

    //==========================================================================
    // paint()

    void paint(juce::Graphics& g) override
    {
        buildLayout();
        paintBar(g);
    }

    void resized() override
    {
        buildLayout();
    }

    //--------------------------------------------------------------------------
    void buildLayout()
    {
        regions_.clear();

        const float w  = static_cast<float>(getWidth());
        const float h  = static_cast<float>(getHeight() > 0 ? getHeight() : kBarHeight);

        // Padding / gap constants matching prototype spec.
        const float padX = 12.0f;
        const float gap  =  6.0f;
        const float gap5 =  5.0f;

        // ---- Left side: transport strip ----
        // Elements: Play | BPM-value "BPM" label | TAP | "|" | TimeSigN "/" TimeSigD | "|" | INT HOST AUTO
        const float btnH  = 18.0f;
        const float btnY  = (h - btnH) * 0.5f;

        float x = padX;

        // Play button (22 × 18 px)
        {
            juce::Rectangle<float> r(x, btnY, 22.0f, btnH);
            regions_.push_back({ r, kRegPlay });
            playBtnBounds_ = r;
            x += 22.0f + gap5;
        }

        // BPM value region (monospace bold 11px, min-width 28px)
        {
            juce::Rectangle<float> r(x, btnY, 32.0f, btnH);
            regions_.push_back({ r, kRegBpm });
            bpmBounds_ = r;
            x += 32.0f + 3.0f;
        }

        // "BPM" label (9px static text, no interaction — just skip the width)
        bpmLabelBounds_ = juce::Rectangle<float>(x, (h - 10.0f) * 0.5f, 22.0f, 10.0f);
        x += 22.0f + gap5;

        // TAP button (22 × 18 px)
        {
            juce::Rectangle<float> r(x, btnY, 22.0f, btnH);
            regions_.push_back({ r, kRegTap });
            tapBtnBounds_ = r;
            x += 22.0f + gap5;
        }

        // Separator "|"
        sepX_[0] = x + 3.0f;
        x += 8.0f + gap5;

        // Time sig numerator (monospace 10px, weight 600)
        {
            juce::Rectangle<float> r(x, (h - 14.0f) * 0.5f, 14.0f, 14.0f);
            regions_.push_back({ r, kRegTimeSigN });
            timeSigNBounds_ = r;
            x += 14.0f + 2.0f;
        }

        // "/" label
        timeSigSlashBounds_ = juce::Rectangle<float>(x, (h - 12.0f) * 0.5f, 6.0f, 12.0f);
        x += 6.0f + 2.0f;

        // Time sig denominator
        {
            juce::Rectangle<float> r(x, (h - 14.0f) * 0.5f, 14.0f, 14.0f);
            regions_.push_back({ r, kRegTimeSigD });
            timeSigDBounds_ = r;
            x += 14.0f + gap5;
        }

        // Separator "|"
        sepX_[1] = x + 3.0f;
        x += 8.0f + gap5;

        // Sync pills: INT HOST AUTO
        static constexpr const char* kSyncLabels[3] = { "INT", "HOST", "AUTO" };
        for (int i = 0; i < 3; ++i)
        {
            const float pillW = (i == 1) ? 26.0f : 22.0f; // HOST wider
            juce::Rectangle<float> r(x, btnY + 2.0f, pillW, btnH - 4.0f);
            regions_.push_back({ r, kRegSyncInt + i });
            syncPillBounds_[i] = r;
            x += pillW + 3.0f;
        }
        (void)kSyncLabels;

        // ---- Right side: status info ----
        // Work right-to-left from right edge.

        float rx = w - padX;

        // Status dot (5 × 5 px, always on)
        const float dotSz = 5.0f;
        statusDotBounds_ = juce::Rectangle<float>(rx - dotSz, (h - dotSz) * 0.5f, dotSz, dotSz);
        rx -= dotSz + gap;

        // MIDI indicator "●" — approx 10px wide
        midiDotBounds_ = juce::Rectangle<float>(rx - 10.0f, (h - 12.0f) * 0.5f, 10.0f, 12.0f);
        rx -= 10.0f + gap;

        // CPU: "CPU: N%"  — reserve ~60px
        cpuBounds_ = juce::Rectangle<float>(rx - 60.0f, (h - 12.0f) * 0.5f, 60.0f, 12.0f);
        rx -= 60.0f + gap;

        // COUPLE button — ~46px wide, 16px tall
        {
            const float pillW = 46.0f;
            const float pillH = 16.0f;
            juce::Rectangle<float> r(rx - pillW, (h - pillH) * 0.5f, pillW, pillH);
            regions_.push_back({ r, kRegCouple });
            coupleBounds_ = r;
            rx -= pillW + gap;
        }

        // Voices: "Voices: N" — reserve ~55px, flush right of spacer
        // (Spacer is implicit — voices label simply sits after x and before COUPLE)
        // Place it just to the right of the left-side content.
        voicesBounds_ = juce::Rectangle<float>(x + gap, (h - 12.0f) * 0.5f, 55.0f, 12.0f);
    }

    //--------------------------------------------------------------------------
    void paintBar(juce::Graphics& g)
    {
        using juce::Colour;

        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());

        // Background
        g.setColour(Colour(10, 12, 18).withAlpha(0.95f));
        g.fillRect(0.0f, 0.0f, w, h);

        // Top border — 1 px
        g.setColour(Colour(200, 204, 216).withAlpha(0.06f));
        g.fillRect(0.0f, 0.0f, w, 1.0f);

        // ---- Fonts ----
        const juce::Font bodyFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withHeight(10.0f));

        const juce::Font monoFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultMonospacedFontName())
            .withStyle("Bold")
            .withHeight(11.0f));

        const juce::Font smallFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withHeight(9.0f));

        const juce::Font pillFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultSansSerifFontName())
            .withStyle("Bold")
            .withHeight(8.0f));

        const juce::Font monoTimeSigFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultMonospacedFontName())
            .withStyle("Bold")
            .withHeight(10.0f));

        // ================================================================
        // LEFT SIDE — Transport

        // --- Play button ---
        {
            const bool isHov = (hoveredRegion_ == kRegPlay);
            const juce::Colour bgCol =
                playing_  ? Colour(127, 219, 202).withAlpha(0.08f)
                           : Colour(200, 204, 216).withAlpha(0.06f);
            const juce::Colour borderCol =
                playing_  ? Colour(127, 219, 202).withAlpha(0.30f)
                : isHov   ? Colour(200, 204, 216).withAlpha(0.20f)
                           : Colour(200, 204, 216).withAlpha(0.10f);
            const juce::Colour textCol =
                playing_  ? Colour(127, 219, 202).withAlpha(0.90f)
                : isHov   ? Colour(200, 204, 216).withAlpha(0.80f)
                           : Colour(200, 204, 216).withAlpha(0.50f);

            g.setColour(bgCol);
            g.fillRoundedRectangle(playBtnBounds_, 3.0f);
            g.setColour(borderCol);
            g.drawRoundedRectangle(playBtnBounds_, 3.0f, 1.0f);

            // Draw play/stop icon as a filled shape instead of text glyphs
            // (UTF-8 triangles don't render reliably across all fonts).
            g.setColour(textCol);
            if (playing_)
            {
                // Stop: filled square
                const float sq = 7.0f;
                g.fillRect(playBtnBounds_.getCentreX() - sq * 0.5f,
                           playBtnBounds_.getCentreY() - sq * 0.5f, sq, sq);
            }
            else
            {
                // Play: filled triangle pointing right
                juce::Path tri;
                const float cx = playBtnBounds_.getCentreX();
                const float cy = playBtnBounds_.getCentreY();
                tri.addTriangle(cx - 4.0f, cy - 5.0f,
                                cx - 4.0f, cy + 5.0f,
                                cx + 5.0f, cy);
                g.fillPath(tri);
            }
        }

        // --- BPM value ---
        {
            const bool isHov = (hoveredRegion_ == kRegBpm || bpmDragging_);
            const juce::Colour col = isHov
                ? Colour(127, 219, 202).withAlpha(1.0f)
                : Colour(127, 219, 202).withAlpha(0.80f);

            g.setFont(monoFont);
            g.setColour(col);

            // Format: one decimal place, e.g. "120.0"
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%.1f", bpm_);
            g.drawText(buf, bpmBounds_.toNearestInt(), juce::Justification::centred, false);
        }

        // --- "BPM" label ---
        {
            g.setFont(smallFont);
            g.setColour(Colour(200, 204, 216).withAlpha(0.30f));
            g.drawText("BPM", bpmLabelBounds_.toNearestInt(),
                       juce::Justification::centredLeft, false);
        }

        // --- TAP button ---
        {
            const bool isHov  = (hoveredRegion_ == kRegTap);
            const bool isFlash = tapFlashFrames_ > 0;
            const juce::Colour bgCol =
                isFlash ? Colour(127, 219, 202).withAlpha(0.20f)
                        : Colour(200, 204, 216).withAlpha(0.06f);
            const juce::Colour borderCol =
                isHov   ? Colour(200, 204, 216).withAlpha(0.20f)
                        : Colour(200, 204, 216).withAlpha(0.10f);
            const juce::Colour textCol =
                isHov   ? Colour(200, 204, 216).withAlpha(0.80f)
                        : Colour(200, 204, 216).withAlpha(0.50f);

            g.setColour(bgCol);
            g.fillRoundedRectangle(tapBtnBounds_, 3.0f);
            g.setColour(borderCol);
            g.drawRoundedRectangle(tapBtnBounds_, 3.0f, 1.0f);

            g.setFont(pillFont);
            g.setColour(textCol);
            g.drawText("TAP", tapBtnBounds_.toNearestInt(),
                       juce::Justification::centred, false);
        }

        // --- Separators "|" ---
        {
            g.setFont(bodyFont);
            g.setColour(Colour(200, 204, 216).withAlpha(0.12f));
            const float midY = static_cast<float>(getHeight()) * 0.5f;
            for (float sx : sepX_)
                g.drawText("|",
                           juce::Rectangle<int>(static_cast<int>(sx) - 4, static_cast<int>(midY) - 6, 8, 12),
                           juce::Justification::centred, false);
        }

        // --- Time sig numerator ---
        {
            const bool isHov = (hoveredRegion_ == kRegTimeSigN);
            const juce::Colour col = isHov
                ? Colour(200, 204, 216).withAlpha(0.70f)
                : Colour(200, 204, 216).withAlpha(0.45f);

            g.setFont(monoTimeSigFont);
            g.setColour(col);

            char buf[4];
            std::snprintf(buf, sizeof(buf), "%d", timeSigNumerator_);
            g.drawText(buf, timeSigNBounds_.toNearestInt(),
                       juce::Justification::centred, false);
        }

        // --- "/" ---
        {
            g.setFont(smallFont);
            g.setColour(Colour(200, 204, 216).withAlpha(0.30f));
            g.drawText("/", timeSigSlashBounds_.toNearestInt(),
                       juce::Justification::centred, false);
        }

        // --- Time sig denominator ---
        {
            const bool isHov = (hoveredRegion_ == kRegTimeSigD);
            const juce::Colour col = isHov
                ? Colour(200, 204, 216).withAlpha(0.70f)
                : Colour(200, 204, 216).withAlpha(0.45f);

            g.setFont(monoTimeSigFont);
            g.setColour(col);

            char buf[4];
            std::snprintf(buf, sizeof(buf), "%d", timeSigDenominator_);
            g.drawText(buf, timeSigDBounds_.toNearestInt(),
                       juce::Justification::centred, false);
        }

        // --- Sync pills ---
        {
            static constexpr const char* kLabels[3] = { "INT", "HOST", "AUTO" };
            g.setFont(pillFont);

            for (int i = 0; i < 3; ++i)
            {
                const bool isActive = (static_cast<int>(syncMode_) == i);
                const bool isHov    = (hoveredRegion_ == kRegSyncInt + i);

                const juce::Colour textCol =
                    isActive ? Colour(127, 219, 202).withAlpha(0.80f)
                    : isHov  ? Colour(200, 204, 216).withAlpha(0.50f)
                             : Colour(200, 204, 216).withAlpha(0.35f);
                const juce::Colour borderCol =
                    isActive ? Colour(127, 219, 202).withAlpha(0.25f)
                    : isHov  ? Colour(200, 204, 216).withAlpha(0.12f)
                             : Colour(200, 204, 216).withAlpha(0.06f);

                g.setColour(borderCol);
                g.drawRoundedRectangle(syncPillBounds_[i], 3.0f, 1.0f);

                g.setColour(textCol);
                g.drawText(kLabels[i], syncPillBounds_[i].toNearestInt(),
                           juce::Justification::centred, false);
            }
        }

        // ================================================================
        // RIGHT SIDE — Status info

        // --- Voices ---
        {
            g.setFont(bodyFont);
            g.setColour(Colour(200, 204, 216).withAlpha(0.35f));

            char buf[24];
            std::snprintf(buf, sizeof(buf), "Voices: %d", voiceCount_);
            g.drawText(buf, voicesBounds_.toNearestInt(),
                       juce::Justification::centredLeft, false);
        }

        // --- COUPLE button ---
        {
            const bool isHov = (hoveredRegion_ == kRegCouple);
            const juce::Colour textCol = isHov
                ? Colour(233, 196, 106).withAlpha(0.80f)
                : Colour(233, 196, 106).withAlpha(0.50f);
            const juce::Colour borderCol = isHov
                ? Colour(233, 196, 106).withAlpha(0.30f)
                : Colour(233, 196, 106).withAlpha(0.15f);

            g.setColour(borderCol);
            g.drawRoundedRectangle(coupleBounds_, 3.0f, 1.0f);

            g.setFont(pillFont);
            g.setColour(textCol);
            g.drawText("COUPLE", coupleBounds_.toNearestInt(),
                       juce::Justification::centred, false);
        }

        // --- CPU meter ---
        {
            // "CPU: " prefix in dim color, value in dynamic color based on load.
            const float cpu = cpuPercent_;
            const juce::Colour valCol =
                (cpu > 85.0f) ? Colour(239, 68, 68).withAlpha(0.80f)
                : (cpu > 60.0f) ? Colour(233, 196, 106).withAlpha(0.80f)
                               : Colour(127, 219, 202).withAlpha(0.70f);

            // Draw in two parts: "CPU: " dim + "N%" colored.
            // Measure "CPU: " width at the body font.
            g.setFont(bodyFont);

            const juce::String prefix("CPU: ");
            const int prefixW = static_cast<int>(
                std::ceil(bodyFont.getStringWidthFloat(prefix)));

            const juce::Rectangle<float> prefixRect(
                cpuBounds_.getX(), cpuBounds_.getY(),
                static_cast<float>(prefixW), cpuBounds_.getHeight());
            const juce::Rectangle<float> valRect(
                cpuBounds_.getX() + static_cast<float>(prefixW), cpuBounds_.getY(),
                cpuBounds_.getWidth() - static_cast<float>(prefixW), cpuBounds_.getHeight());

            g.setColour(Colour(200, 204, 216).withAlpha(0.35f));
            g.drawText(prefix, prefixRect.toNearestInt(),
                       juce::Justification::centredLeft, false);

            char valBuf[8];
            std::snprintf(valBuf, sizeof(valBuf), "%.0f%%", cpu);

            g.setColour(valCol);
            g.drawText(valBuf, valRect.toNearestInt(),
                       juce::Justification::centredLeft, false);
        }

        // --- MIDI flash dot ---
        {
            const float alpha = 0.15f + midiFlashAlpha_ * 0.75f; // 0.15 base, up to 0.90
            const juce::Colour col = (midiFlashAlpha_ > 0.05f)
                ? Colour(127, 219, 202).withAlpha(juce::jlimit(0.0f, 1.0f, alpha))
                : Colour(200, 204, 216).withAlpha(0.15f);

            g.setFont(bodyFont);
            g.setColour(col);
            // UTF-8: "●" = \xe2\x97\x8f
            g.drawText("\xe2\x97\x8f", midiDotBounds_.toNearestInt(),
                       juce::Justification::centred, false);
        }

        // --- Status dot (always on, 5 × 5 px circle) ---
        {
            g.setColour(Colour(60, 180, 170).withAlpha(0.55f));
            g.fillEllipse(statusDotBounds_);
        }
    }

    //==========================================================================
    // Mouse handling

    void mouseDown(const juce::MouseEvent& e) override
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        for (const auto& reg : regions_)
        {
            if (!reg.bounds.expanded(2.0f).contains(mx, my))
                continue;

            switch (reg.id)
            {
                case kRegPlay:
                    if (onPlayToggled)
                        onPlayToggled();
                    break;

                case kRegBpm:
                    bpmDragging_   = true;
                    bpmDragStartY_ = e.y;
                    bpmDragStartV_ = bpm_;
                    break;

                case kRegTap:
                    handleTap();
                    tapFlashFrames_ = 2; // show flash for 2 timer ticks (~200ms at 10Hz)
                    repaint();
                    break;

                case kRegTimeSigN:
                    cycleTimeSigNumerator();
                    if (onTimeSigChanged)
                        onTimeSigChanged(timeSigNumerator_, timeSigDenominator_);
                    repaint();
                    break;

                case kRegTimeSigD:
                    cycleTimeSigDenominator();
                    if (onTimeSigChanged)
                        onTimeSigChanged(timeSigNumerator_, timeSigDenominator_);
                    repaint();
                    break;

                case kRegSyncInt:
                    syncMode_ = SyncMode::Int;
                    repaint();
                    break;

                case kRegSyncHost:
                    syncMode_ = SyncMode::Host;
                    repaint();
                    break;

                case kRegSyncAuto:
                    syncMode_ = SyncMode::Auto;
                    repaint();
                    break;

                case kRegCouple:
                    if (onCoupleClicked)
                        onCoupleClicked();
                    break;

                default:
                    break;
            }
            return;
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!bpmDragging_)
            return;

        // Vertical drag: up = +BPM, down = -BPM, 0.5 BPM per pixel.
        const float dy      = static_cast<float>(e.y - bpmDragStartY_);
        const double newBpm = juce::jlimit(20.0, 300.0,
                                           bpmDragStartV_ + static_cast<double>(-dy * 0.5f));

        bpm_ = newBpm;
        repaint();

        if (onBpmChanged)
            onBpmChanged(bpm_);
    }

    void mouseUp(const juce::MouseEvent& /*e*/) override
    {
        bpmDragging_ = false;
        repaint();
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const float mx = static_cast<float>(e.x);
        const float my = static_cast<float>(e.y);

        int newHover = -1;
        for (const auto& reg : regions_)
        {
            if (reg.bounds.expanded(2.0f).contains(mx, my))
            {
                newHover = reg.id;
                break;
            }
        }

        if (newHover != hoveredRegion_)
        {
            hoveredRegion_ = newHover;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent& /*e*/) override
    {
        if (hoveredRegion_ != -1)
        {
            hoveredRegion_ = -1;
            repaint();
        }
    }

    //==========================================================================
    // timerCallback — 10 Hz

    void timerCallback() override
    {
        bool needRepaint = false;

        // Decay MIDI flash (full decay over ~200ms = 2 ticks at 10Hz).
        if (midiFlashAlpha_ > 0.0f)
        {
            midiFlashAlpha_ = juce::jmax(0.0f, midiFlashAlpha_ - 0.5f);
            needRepaint = true;
        }

        // Decay TAP button flash.
        if (tapFlashFrames_ > 0)
        {
            --tapFlashFrames_;
            needRepaint = true;
        }

        if (needRepaint)
            repaint();
    }

    //==========================================================================
    // Tap tempo logic

    void handleTap()
    {
        const int64_t nowMs = juce::Time::currentTimeMillis();

        // Reset ring buffer if gap > 2000 ms.
        if (tapCount_ > 0)
        {
            const int64_t gap = nowMs - tapTimes_[static_cast<size_t>((tapHead_ - 1 + kTapBufSize) % kTapBufSize)];
            if (gap > 2000LL)
                tapCount_ = 0;
        }

        // Store timestamp.
        tapTimes_[static_cast<size_t>(tapHead_)] = nowMs;
        tapHead_ = (tapHead_ + 1) % kTapBufSize;
        if (tapCount_ < kTapBufSize)
            ++tapCount_;

        // Need at least 2 taps to compute an interval.
        if (tapCount_ < 2)
            return;

        // Average interval from the stored timestamps.
        // The ring buffer holds tapCount_ entries ending at (tapHead_-1).
        const int samples = tapCount_;
        int64_t totalMs = 0LL;
        for (int i = 1; i < samples; ++i)
        {
            const int a = (tapHead_ - samples + i - 1 + kTapBufSize) % kTapBufSize;
            const int b = (tapHead_ - samples + i     + kTapBufSize) % kTapBufSize;
            totalMs += tapTimes_[static_cast<size_t>(b)] - tapTimes_[static_cast<size_t>(a)];
        }
        const double avgMs   = static_cast<double>(totalMs) / static_cast<double>(samples - 1);
        const double newBpm  = juce::jlimit(20.0, 300.0, 60000.0 / avgMs);

        bpm_ = newBpm;
        repaint();

        if (onBpmChanged)
            onBpmChanged(bpm_);
    }

    //==========================================================================
    // Time sig helpers

    void cycleTimeSigNumerator()
    {
        static constexpr int kNumerators[] = { 2, 3, 4, 5, 6, 7 };
        static constexpr int kCount = static_cast<int>(sizeof(kNumerators) / sizeof(kNumerators[0]));

        int idx = 0;
        for (int i = 0; i < kCount; ++i)
        {
            if (kNumerators[i] == timeSigNumerator_)
            {
                idx = (i + 1) % kCount;
                break;
            }
        }
        timeSigNumerator_ = kNumerators[idx];
    }

    void cycleTimeSigDenominator()
    {
        static constexpr int kDenominators[] = { 2, 4, 8 };
        static constexpr int kCount = static_cast<int>(sizeof(kDenominators) / sizeof(kDenominators[0]));

        int idx = 0;
        for (int i = 0; i < kCount; ++i)
        {
            if (kDenominators[i] == timeSigDenominator_)
            {
                idx = (i + 1) % kCount;
                break;
            }
        }
        timeSigDenominator_ = kDenominators[idx];
    }

    //==========================================================================
    // State

    // Transport
    bool   playing_             = false;
    double bpm_                 = 120.0;
    int    timeSigNumerator_    = 4;
    int    timeSigDenominator_  = 4;
    SyncMode syncMode_          = SyncMode::Int;

    // Status
    int   voiceCount_  = 0;
    float cpuPercent_  = 0.0f;
    float midiFlashAlpha_ = 0.0f;

    // BPM drag
    bool   bpmDragging_    = false;
    int    bpmDragStartY_  = 0;
    double bpmDragStartV_  = 120.0;

    // TAP tempo ring buffer (5 entries)
    static constexpr int kTapBufSize = 5;
    std::array<int64_t, kTapBufSize> tapTimes_ {};
    int tapHead_  = 0;
    int tapCount_ = 0;
    int tapFlashFrames_ = 0;

    // Hover / interaction
    int hoveredRegion_ = -1;

    // Layout rects (rebuilt each buildLayout call)
    juce::Rectangle<float> playBtnBounds_;
    juce::Rectangle<float> bpmBounds_;
    juce::Rectangle<float> bpmLabelBounds_;
    juce::Rectangle<float> tapBtnBounds_;
    juce::Rectangle<float> timeSigNBounds_;
    juce::Rectangle<float> timeSigSlashBounds_;
    juce::Rectangle<float> timeSigDBounds_;
    std::array<juce::Rectangle<float>, 3> syncPillBounds_ {};
    juce::Rectangle<float> voicesBounds_;
    juce::Rectangle<float> coupleBounds_;
    juce::Rectangle<float> cpuBounds_;
    juce::Rectangle<float> midiDotBounds_;
    juce::Rectangle<float> statusDotBounds_;
    std::array<float, 2>   sepX_ {};

    // Hit-test regions
    std::vector<Region> regions_;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};

} // namespace xoceanus
