// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// DotMatrixDisplay — Retro submarine dot-matrix LED visualizer for the macro strip.
//
// A real-time audio visualization panel rendered as a grid of small square dots
// that light up in teal, evoking sonar / submarine instrumentation. Fits in the
// empty space to the right of the macro knobs in the macro strip area.
//
// Four display modes (right-click to cycle):
//   Waveform       — scrolling stereo waveform, left top / right bottom.
//   Spectrum       — 16-band vertical bar FFT with smooth decay.
//   EnginePulse    — 4 wide columns pulsing per engine activity level.
//   SequencerMirror — 16 step columns mirroring TideWaterline state.
//
// Feed data from the editor timer via pushLevels / pushSpectrum /
// pushEngineActivity / pushSequencerStep.  Timer fires at 30 Hz for animation.
//
// When no data arrives for > 2 s the component falls into an idle "breathing"
// mode: dots fade in/out slowly from the centre outward, like a sonar ping.
//==============================================================================

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

class DotMatrixDisplay : public juce::Component,
                         private juce::Timer
{
public:
    //==========================================================================
    enum class Mode { Waveform, Spectrum, EnginePulse, SequencerMirror, NumModes };

    //==========================================================================
    DotMatrixDisplay()
    {
        setInterceptsMouseClicks(true, false);

        // Initialise ring buffer and decay state to silence.
        waveBuffer_.fill(0.0f);
        specBins_.fill(0.0f);
        engineLevels_.fill(0.0f);
        seqActive_.fill(false);

        startTimerHz(30);
    }

    ~DotMatrixDisplay() override { stopTimer(); }

    //==========================================================================
    // Mode control
    //==========================================================================

    void setMode(Mode m) { mode_ = m; repaint(); }
    Mode getMode() const { return mode_; }

    //==========================================================================
    // Data push — call from editor timer on the message thread
    //==========================================================================

    /** Push a stereo RMS frame (0–1 each). Used by Waveform mode. */
    void pushLevels(float leftRMS, float rightRMS)
    {
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

        const float l = juce::jlimit(0.0f, 1.0f, leftRMS);
        const float r = juce::jlimit(0.0f, 1.0f, rightRMS);

        // Load head for this cycle; ARM safety pattern: write buffer first, then release-store head.
        int head = waveHead_.load(std::memory_order_relaxed);

        // Write buffer slot first.
        waveBuffer_[head] = l;
        waveBufferR_[head] = r;

        // Advance head with release semantics so paint() sees buffered writes before new index.
        // Use store(release) directly — fence+relaxed is NOT a release sequence on ARM (Apple Silicon).
        int next = (head + 1) % kWaveLen;
        waveHead_.store(next, std::memory_order_release);

        lastPushMs_ = juce::Time::getMillisecondCounterHiRes();
        dataDirty_ = true;  // F2-018
    }

    /** Push 16 normalised frequency bins (0–1). Used by Spectrum mode. */
    void pushSpectrum(const float* bins, int numBins)
    {
        const int n = std::min(numBins, kSpecBins);
        for (int i = 0; i < n; ++i)
            specBins_[i] = std::max(specBins_[i], juce::jlimit(0.0f, 1.0f, bins[i]));
        lastPushMs_ = juce::Time::getMillisecondCounterHiRes();
        dataDirty_ = true;  // F2-018
    }

    /** Push 4 engine activity levels (0–1). Used by EnginePulse mode. */
    void pushEngineActivity(const float* levels, int /*n*/)
    {
        for (int i = 0; i < 4; ++i)
            engineLevels_[i] = std::max(engineLevels_[i],
                                        juce::jlimit(0.0f, 1.0f, levels[i]));
        lastPushMs_ = juce::Time::getMillisecondCounterHiRes();
        dataDirty_ = true;  // F2-018
    }

    /** Push current sequencer step state. Used by SequencerMirror mode. */
    void pushSequencerStep(int currentStep, int totalSteps, bool active)
    {
        seqCurrent_  = currentStep;
        seqTotal_    = juce::jlimit(1, 16, totalSteps);
        if (currentStep >= 0 && currentStep < 16)
            seqActive_[currentStep] = active;
        lastPushMs_ = juce::Time::getMillisecondCounterHiRes();
        dataDirty_ = true;  // F2-018
    }

private:
    //==========================================================================
    // Constants
    //==========================================================================

    static constexpr int   kWaveLen  = 128;   // ring buffer length
    static constexpr int   kSpecBins = 16;
    static constexpr int   kDotSize  = 2;     // dot pixel size
    static constexpr int   kPitch    = 3;     // dot + gap = 3px cell
    static constexpr int   kPad      = 4;     // inner padding each side

    // Dot brightness thresholds (0–1 value → colour tier)
    static constexpr float kLowThresh  = 0.10f;
    static constexpr float kMidThresh  = 0.35f;
    static constexpr float kHighThresh = 0.65f;

    // Idle breathing constants
    static constexpr double kIdleTimeoutMs = 2000.0;

    //==========================================================================
    // Colour helpers — returns one of 5 teal tiers based on normalised value
    //==========================================================================

    static juce::Colour dotColour(float v) noexcept
    {
        if (v < kLowThresh)   return juce::Colour(0x083CB4AA);  // off  ~3% alpha
        if (v < kMidThresh)   return juce::Colour(0x263CB4AA);  // low  ~15%
        if (v < kHighThresh)  return juce::Colour(0x663CB4AA);  // mid  ~40%
        if (v < 0.90f)        return juce::Colour(0xB33CB4AA);  // high ~70%
        return                       juce::Colour(0xE57FDBCA);  // peak ~90%
    }

    //==========================================================================
    // Paint
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // ── Background ────────────────────────────────────────────────────────
        g.setColour(juce::Colour(0x800A0C0E));   // rgba(8,10,14,0.50)
        g.fillRoundedRectangle(bounds, 6.0f);

        // ── Border ────────────────────────────────────────────────────────────
        g.setColour(juce::Colour(0x0F3CB4AA));   // rgba(60,180,170,0.06)
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

        // ── Dot grid ──────────────────────────────────────────────────────────
        const int areaX = kPad;
        const int areaY = kPad;
        const int areaW = getWidth()  - kPad * 2;
        const int areaH = getHeight() - kPad * 2;

        cols_ = std::max(1, areaW / kPitch);
        rows_ = std::max(1, areaH / kPitch);

        const bool idle = isIdle();

        switch (mode_)
        {
            case Mode::Waveform:        paintWaveform       (g, areaX, areaY, idle); break;
            case Mode::Spectrum:        paintSpectrum       (g, areaX, areaY, idle); break;
            case Mode::EnginePulse:     paintEnginePulse    (g, areaX, areaY, idle); break;
            case Mode::SequencerMirror: paintSequencerMirror(g, areaX, areaY, idle); break;
            default: break;
        }

        // ── Mode label ────────────────────────────────────────────────────────
        static constexpr const char* kLabels[] = { "WAV", "FFT", "ENG", "SEQ" };
        g.setFont(GalleryFonts::dotMatrix(8.0f));
        g.setColour(juce::Colour(0x33C8CCD8));   // rgba(200,204,216,0.20)
        g.drawText(kLabels[static_cast<int>(mode_)],
                   getWidth() - 22, getHeight() - 10, 20, 8,
                   juce::Justification::centredRight, false);
    }

    //==========================================================================
    // Waveform mode
    //==========================================================================

    void paintWaveform(juce::Graphics& g, int ax, int ay, bool idle)
    {
        const int halfRows = rows_ / 2;

        if (idle)
        {
            paintIdleBreath(g, ax, ay);
            return;
        }

        // Map ring buffer columns to the visible column count.
        // The most recent sample is at waveHead_ - 1, oldest at waveHead_.
        // Read head with acquire semantics to ensure we see all buffer writes.
        const int head = waveHead_.load(std::memory_order_acquire);

        for (int col = 0; col < cols_; ++col)
        {
            // Map col 0 = oldest, col cols_-1 = newest
            const int bufIdx = (head + col) % kWaveLen;
            const float lVal = waveBuffer_[bufIdx];
            const float rVal = waveBufferR_[bufIdx];

            // Left channel: top half, fills downward from the vertical centre
            const int lHeight = juce::roundToInt(lVal * halfRows);
            for (int row = 0; row < halfRows; ++row)
            {
                const int distFromCentre = halfRows - 1 - row;   // 0 at centre
                const float v = (distFromCentre < lHeight) ? lVal : 0.0f;
                drawDot(g, ax + col * kPitch, ay + row * kPitch, v);
            }

            // Right channel: bottom half, fills upward from the vertical centre
            const int rHeight = juce::roundToInt(rVal * (rows_ - halfRows));
            for (int row = halfRows; row < rows_; ++row)
            {
                const int distFromCentre = row - halfRows;        // 0 at centre
                const float v = (distFromCentre < rHeight) ? rVal : 0.0f;
                drawDot(g, ax + col * kPitch, ay + row * kPitch, v);
            }
        }
    }

    //==========================================================================
    // Spectrum mode
    //==========================================================================

    void paintSpectrum(juce::Graphics& g, int ax, int ay, bool idle)
    {
        if (idle)
        {
            paintIdleBreath(g, ax, ay);
            return;
        }

        // Distribute 16 bands across the available columns with 1-col gap between.
        // Band width = max 1, evenly partitioned.
        const int gapCols  = 1;
        const int numBands = kSpecBins;
        // Total slot width per band (including gap): cols_ / numBands
        const int slotW = std::max(1, cols_ / numBands);
        const int barW  = std::max(1, slotW - gapCols);

        for (int b = 0; b < numBands; ++b)
        {
            const float mag   = juce::jlimit(0.0f, 1.0f, specBins_[b]);
            const int barH    = juce::roundToInt(mag * rows_);
            const int colBase = ax + b * slotW * kPitch;

            for (int row = 0; row < rows_; ++row)
            {
                const int distFromBottom = rows_ - 1 - row;
                const float v = (distFromBottom < barH) ? mag : 0.0f;
                for (int dc = 0; dc < barW; ++dc)
                    drawDot(g, colBase + dc * kPitch, ay + row * kPitch, v);
            }
        }
    }

    //==========================================================================
    // Engine Pulse mode
    //==========================================================================

    void paintEnginePulse(juce::Graphics& g, int ax, int ay, bool idle)
    {
        if (idle)
        {
            paintIdleBreath(g, ax, ay);
            return;
        }

        // 4 wide columns, centred in the available area.
        const int numEngines  = 4;
        const int colSlotCols = cols_ / numEngines;
        const int barCols     = std::max(1, colSlotCols - 1);  // 1-col gap each side
        const int offsetCols  = (colSlotCols - barCols) / 2;

        for (int e = 0; e < numEngines; ++e)
        {
            const float level  = juce::jlimit(0.0f, 1.0f, engineLevels_[e]);
            const int barH     = std::max(1, juce::roundToInt(level * rows_));
            const int colBase  = ax + (e * colSlotCols + offsetCols) * kPitch;

            for (int row = 0; row < rows_; ++row)
            {
                const int distFromBottom = rows_ - 1 - row;
                const float v = (distFromBottom < barH) ? level : 0.0f;
                for (int dc = 0; dc < barCols; ++dc)
                    drawDot(g, colBase + dc * kPitch, ay + row * kPitch, v);
            }
        }
    }

    //==========================================================================
    // Sequencer Mirror mode
    //==========================================================================

    void paintSequencerMirror(juce::Graphics& g, int ax, int ay, bool /*idle*/)
    {
        // Always show the sequencer grid even in idle — steps don't need live audio.
        const int numSteps = std::max(1, seqTotal_);
        const int slotW    = std::max(1, cols_ / numSteps);  // columns per step slot
        const int barW     = std::max(1, slotW - 1);         // 1-col gap

        for (int s = 0; s < numSteps; ++s)
        {
            const bool isCurrent = (s == seqCurrent_);
            const bool isActive  = seqActive_[s];

            // Current playhead: full bright column (peak tier).
            // Active non-current: medium tier single centre dot.
            // Inactive: dim single bottom dot.
            const int colBase = ax + (s * slotW) * kPitch;

            for (int row = 0; row < rows_; ++row)
            {
                float v = 0.0f;

                if (isCurrent)
                {
                    // Full bright column — use peak brightness.
                    v = 1.0f;
                }
                else if (isActive)
                {
                    // Single dot at vertical centre.
                    if (row == rows_ / 2)
                        v = 0.55f;
                }
                else
                {
                    // Single dim dot at the bottom row.
                    if (row == rows_ - 1)
                        v = 0.12f;
                }

                for (int dc = 0; dc < barW; ++dc)
                    drawDot(g, colBase + dc * kPitch, ay + row * kPitch, v);
            }
        }
    }

    //==========================================================================
    // Idle breathing pattern
    //==========================================================================

    void paintIdleBreath(juce::Graphics& g, int ax, int ay)
    {
        // Sonar-ping effect: concentric rings of brightness emanating from centre,
        // pulsing at ~0.5 Hz.  Uses breathPhase_ advanced in timerCallback().
        const float cx = cols_ * 0.5f;
        const float cy = rows_ * 0.5f;
        const float maxR = std::sqrt(cx * cx + cy * cy);

        for (int row = 0; row < rows_; ++row)
        {
            for (int col = 0; col < cols_; ++col)
            {
                const float dx = col - cx;
                const float dy = row - cy;
                const float dist = std::sqrt(dx * dx + dy * dy);

                // Normalise distance 0–1.
                const float normDist = dist / maxR;

                // Traveling wave: brightness peaks where phase matches distance.
                // Phase travels outward: phase - normDist in [0,1] → sinusoidal.
                const float wave = 0.5f + 0.5f * std::sin(
                    static_cast<float>(M_PI) * 2.0f
                    * (breathPhase_ - normDist * 1.8f));

                // Attenuate by distance so the ring fades outward.
                const float v = wave * (1.0f - normDist * 0.85f) * 0.30f;

                drawDot(g, ax + col * kPitch, ay + row * kPitch, v);
            }
        }
    }

    //==========================================================================
    // Dot primitive
    //==========================================================================

    inline void drawDot(juce::Graphics& g, int x, int y, float v) const
    {
        g.setColour(dotColour(v));
        g.fillRect(x, y, kDotSize, kDotSize);
    }

    //==========================================================================
    // Mouse interaction — right-click or Ctrl+click cycles mode
    //==========================================================================

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown() || e.mods.isCtrlDown())
        {
            const int next = (static_cast<int>(mode_) + 1)
                             % static_cast<int>(Mode::NumModes);
            mode_ = static_cast<Mode>(next);
            repaint();
        }
    }

    //==========================================================================
    // Timer — 30 Hz
    //==========================================================================

    void timerCallback() override
    {
        // Decay spectrum bins each frame: multiply by 0.92.
        for (float& bin : specBins_)
            bin *= 0.92f;

        // Decay engine levels with slightly slower falloff.
        for (float& lvl : engineLevels_)
            lvl *= 0.88f;

        // Advance idle breathing phase.
        breathPhase_ += 0.017f;   // ~0.5 Hz at 30 fps
        if (breathPhase_ > 1.0f)
            breathPhase_ -= 1.0f;

        // Skip the repaint when hidden (status bar collapsed etc.) — decay
        // above still runs so the display is not stale when re-shown.
        if (! isShowing())
            return;

        // F2-018: Only repaint when new data was pushed or the breathing phase changed.
        // The breathing phase changes every tick (animation), so always repaint when
        // idle to keep the breath animation smooth.  When active data arrives, dataDirty_
        // ensures we never miss a frame.  Clear the flag BEFORE repaint() so a push that
        // arrives during paint() is not silently dropped.
        if (dataDirty_ || !isIdle())
        {
            dataDirty_ = false;
            repaint();
        }
    }

    //==========================================================================
    // Helpers
    //==========================================================================

    bool isIdle() const noexcept
    {
        const double now = juce::Time::getMillisecondCounterHiRes();
        return (now - lastPushMs_) > kIdleTimeoutMs;
    }

    //==========================================================================
    // State
    //==========================================================================

    Mode mode_ = Mode::Waveform;

    // Waveform ring buffers
    std::array<float, kWaveLen> waveBuffer_{};   // left channel
    std::array<float, kWaveLen> waveBufferR_{};  // right channel
    std::atomic<int> waveHead_{0};

    // Spectrum decay buffer
    std::array<float, kSpecBins> specBins_{};

    // Engine pulse levels (decayed in timer)
    std::array<float, 4> engineLevels_{};

    // Sequencer state
    std::array<bool, 16> seqActive_{};
    int seqCurrent_ = 0;
    int seqTotal_   = 16;

    // Idle breathing
    float breathPhase_ = 0.0f;

    // Last time data was pushed (ms)
    double lastPushMs_ = 0.0;

    // F2-018: Set by any push method; cleared by timerCallback before repaint().
    // Ensures repaint() is only called when content has actually changed.
    bool dataDirty_ = false;

    // Cached grid dimensions (computed in paint from current size)
    mutable int cols_ = 0;
    mutable int rows_ = 0;

    //==========================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DotMatrixDisplay)
};

} // namespace xoceanus
