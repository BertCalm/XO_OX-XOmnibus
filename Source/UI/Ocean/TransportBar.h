// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// TransportBar.h — Submarine-style 28 px transport + status bar for the XOceanus Ocean View.
//
// D13 layout (Wave 2B):  [ Play BPM TAP ] [ 4/4 ] [ CHAIN ]
// Two spatial groups on the left, CHAIN on the right.  Voice count, CPU meter, MIDI dot,
// and status dot removed at v1 per D13 D1 (lean layout).
// P1-10 (1F): INT/HOST/AUTO sync pills removed — had no APVTS backing.
//
// The bar sits at the very bottom of the plugin window. It renders two zones in a single
// horizontal row:
//
//   LEFT  — Transport strip: Play, BPM (drag-editable), TAP, Time Sig
//   RIGHT — Status info: CHAIN button
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
//   - Time sig (numerator OR denominator): click cycles 4/4→3/4→6/8→7/8→5/4→4/4 (D13 C2)
//   - CHAIN button: click fires onCoupleClicked (variable name kept for wiring stability, D13)
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
                     public juce::TooltipClient,
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

        // Fix #1424: expose transport controls to screen readers.
        A11y::setup(*this,
                    "Transport Bar",
                    "Play, stop, tempo, time signature, and MIDI activity display");

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

    // D3 (1D-P2B): LATCH status indicator — call when Keys mode activates/deactivates.
    // Displayed as a small read-only pill in the status bar right area.
    void setLatchActive(bool active)
    {
        if (latchActive_ != active)
        {
            latchActive_ = active;
            repaint();
        }
    }

    //==========================================================================
    // Callbacks — parent wires these in the editor constructor.

    std::function<void()>         onPlayToggled;
    std::function<void()>         onCoupleClicked;
    std::function<void(double)>   onBpmChanged;
    std::function<void(int, int)> onTimeSigChanged;

    //==========================================================================
    // juce::TooltipClient — §1301 batch 4: full transport bar coverage.
    // Returns a tooltip string based on which region is currently hovered.
    juce::String getTooltip() override
    {
        switch (hoveredRegion_)
        {
            case kRegPlay:
                return playing_ ? "Stop transport" : "Start transport";
            case kRegBpm:
                return "Tempo — drag up/down to change BPM";
            case kRegTap:
                return "Tap tempo — tap repeatedly to set BPM";
            case kRegTimeSigN:
                return "Time signature numerator — click to cycle beat count (4, 3, 6, 7, 5)";
            case kRegTimeSigD:
                return "Time signature denominator — click to cycle note value (4, 4, 8, 8, 4)";
            // P1-10 (1F): kRegSyncInt/Host/Auto (5-7) removed with sync pills.
            case kRegCouple:
                return "Open coupling inspector";
            default:
                return {};
        }
    }

private:
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
        // P1-10 (1F): Sync pills (INT/HOST/AUTO) removed — had no APVTS backing
        // and no callback.  Clicking changed nothing in the audio engine.
        // IDs 5-7 retired to keep existing RegionId numeric values stable.
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
        const float gap5 =  5.0f;

        // ---- Left side: transport strip ----
        // D13 D1: three spatial groups — [ Play BPM TAP ] [ 4/4 ] [ INT HOST AUTO ]
        // Groups separated by ~14 px inter-group gaps.
        const float btnH     = 18.0f;
        const float btnY     = (h - btnH) * 0.5f;
        const float groupGap = 14.0f;  // D13 D1: inter-group gap

        float x = padX;

        // ── Group 1: Play | BPM | TAP ──────────────────────────────────────────

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
            x += 22.0f + groupGap;  // D13 D1: group gap after Group 1
        }

        // Separator "|" between Group 1 and Group 2
        sepX_[0] = x - groupGap * 0.5f;

        // ── Group 2: Time sig [ 4/4 ] ──────────────────────────────────────────

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
            x += 14.0f + groupGap;  // D13 D1: group gap after Group 2
        }

        // Separator "|" between Group 2 and right-side controls.
        // P1-10 (1F): Group 3 sync pills (INT/HOST/AUTO) removed.
        // No APVTS backing, no audio-engine callback — Potemkin controls.
        // The space they occupied is now breathing room between time-sig and CHAIN.
        sepX_[1] = x - groupGap * 0.5f;

        // ---- Right side: D13 D1 lean layout — CHAIN button only ----
        // Voice count, CPU meter, MIDI dot, status dot removed at v1 per D13 D1.
        // Work right-to-left from right edge.

        float rx = w - padX;

        // D13 D1: removed at v1 — status dot
        // D13 D1: removed at v1 — MIDI indicator
        // D13 D1: removed at v1 — CPU meter
        // D13 D1: removed at v1 — Voices label

        // CHAIN button — ~46px wide, 16px tall
        // Renders as "CHAIN" per D13 — variable name kept for wiring stability.
        {
            const float pillW = 46.0f;
            const float pillH = 16.0f;
            juce::Rectangle<float> r(rx - pillW, (h - pillH) * 0.5f, pillW, pillH);
            regions_.push_back({ r, kRegCouple });
            coupleBounds_ = r;
            // rx -= pillW + gap;  // nothing further right
        }

        // voicesBounds_ kept as member to avoid cascading removal; set off-screen.
        voicesBounds_ = juce::Rectangle<float>(-200.0f, 0.0f, 0.0f, 0.0f);

        // D3 (1D-P2B): LATCH status pill — left of CHAIN button.
        // Read-only indicator: visible only when LATCH is active (Keys mode).
        {
            const float pillW = 38.0f;
            const float pillH = 16.0f;
            latchBounds_ = juce::Rectangle<float>(rx - pillW - coupleBounds_.getWidth() - gap5 * 3,
                                                   (h - pillH) * 0.5f, pillW, pillH);
        }
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

        const juce::Font monoFont = GalleryFonts::dotMatrix(12.0f);

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

        // P1-10 (1F): Sync pills (INT/HOST/AUTO) removed — see buildLayout() comment.

        // ================================================================
        // RIGHT SIDE — D13 D1: lean layout — LATCH indicator + CHAIN button.
        // Voices, CPU meter, MIDI dot, status dot removed at v1 per D13 D1.

        // D3 (1D-P2B): LATCH status pill — read-only, left of CHAIN.
        // Visible only when LATCH is active (Keys mode). Alpha-faded when inactive
        // so it reads as a passive indicator regardless of mode.
        {
            g.setFont(pillFont);
            if (latchActive_)
            {
                // Active state: amber tint, visible border
                g.setColour(Colour(233, 196, 106).withAlpha(0.07f));
                g.fillRoundedRectangle(latchBounds_, 3.0f);
                g.setColour(Colour(233, 196, 106).withAlpha(0.30f));
                g.drawRoundedRectangle(latchBounds_, 3.0f, 1.0f);
                g.setColour(Colour(233, 196, 106).withAlpha(0.80f));
            }
            else
            {
                // Inactive: very subtle ghost (shows label exists, not lit)
                g.setColour(Colour(200, 204, 216).withAlpha(0.04f));
                g.drawRoundedRectangle(latchBounds_, 3.0f, 1.0f);
                g.setColour(Colour(200, 204, 216).withAlpha(0.20f));
            }
            g.drawText("LATCH", latchBounds_.toNearestInt(),
                       juce::Justification::centred, false);
        }

        // --- CHAIN button ---
        // Renders as "CHAIN" per D13 — variable name (coupleBounds_) kept for wiring stability.
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
            // Renders as "CHAIN" per D13 — variable name kept for wiring stability.
            g.drawText("CHAIN", coupleBounds_.toNearestInt(),
                       juce::Justification::centred, false);
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
                    // D13 C2: both numerator and denominator regions fire the same
                    // combined cycle: 4/4→3/4→6/8→7/8→5/4→4/4.
                    cycleTimeSig();
                    if (onTimeSigChanged)
                        onTimeSigChanged(timeSigNumerator_, timeSigDenominator_);
                    repaint();
                    break;

                case kRegTimeSigD:
                    // D13 C2: denominator click fires the same combined cycle as numerator.
                    cycleTimeSig();
                    if (onTimeSigChanged)
                        onTimeSigChanged(timeSigNumerator_, timeSigDenominator_);
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

    /** D13 C2 — Combined time-signature cycle.
        Clicking either the numerator or denominator region walks a fixed table of
        musically meaningful combinations: 4/4 → 3/4 → 6/8 → 7/8 → 5/4 → 4/4.
        Both `timeSigNumerator_` and `timeSigDenominator_` are updated atomically
        so the display always shows a valid combination.
        The `onTimeSigChanged(num, den)` callback fires after this returns. */
    void cycleTimeSig()
    {
        struct TimeSig { int num; int den; };
        static constexpr TimeSig kTable[] = {
            { 4, 4 },
            { 3, 4 },
            { 6, 8 },
            { 7, 8 },
            { 5, 4 },
        };
        static constexpr int kCount = static_cast<int>(sizeof(kTable) / sizeof(kTable[0]));

        // Find the current entry and advance to the next.
        int nextIdx = 0;
        for (int i = 0; i < kCount; ++i)
        {
            if (kTable[i].num == timeSigNumerator_ && kTable[i].den == timeSigDenominator_)
            {
                nextIdx = (i + 1) % kCount;
                break;
            }
        }
        timeSigNumerator_   = kTable[nextIdx].num;
        timeSigDenominator_ = kTable[nextIdx].den;
    }

    // Legacy individual-axis helpers kept so existing external call sites compile;
    // they are no longer called from the mouseDown handler (D13 C2).
    void cycleTimeSigNumerator()   { cycleTimeSig(); }
    void cycleTimeSigDenominator() { cycleTimeSig(); }

    //==========================================================================
    // State

    // Transport
    bool   playing_             = false;
    double bpm_                 = 120.0;
    int    timeSigNumerator_    = 4;
    int    timeSigDenominator_  = 4;

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

    // D3 (1D-P2B): LATCH active state — updated by setLatchActive()
    bool latchActive_ = true; // default Keys mode = LATCH active

    // Layout rects (rebuilt each buildLayout call)
    juce::Rectangle<float> playBtnBounds_;
    juce::Rectangle<float> bpmBounds_;
    juce::Rectangle<float> bpmLabelBounds_;
    juce::Rectangle<float> tapBtnBounds_;
    juce::Rectangle<float> timeSigNBounds_;
    juce::Rectangle<float> timeSigSlashBounds_;
    juce::Rectangle<float> timeSigDBounds_;
    juce::Rectangle<float> voicesBounds_;
    juce::Rectangle<float> coupleBounds_;
    juce::Rectangle<float> latchBounds_;  // D3 (1D-P2B): LATCH status indicator
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
