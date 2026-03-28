#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../GalleryColors.h"

namespace xolokun {

//==============================================================================
// WaveformDisplay — Full oscilloscope for EngineDetailPanel (200×80pt).
//
// Reads 256 samples from the processor's WaveformFifo at 30Hz (10Hz when
// A11y::prefersReducedMotion() is true) and draws a juce::Path waveform with
// a subtle gradient background and engine accent color stroke.
//
// CRT mode activates automatically when engineId == "Optic": overrides accent
// to Phosphor Green (#00FF41), adds a 2px glow pass, and draws a scanline
// overlay at 15% opacity.
class WaveformDisplay : public juce::Component, private juce::Timer
{
public:
    static constexpr size_t kDisplaySamples = 256;

    explicit WaveformDisplay(XOlokunProcessor& proc)
        : processor(proc)
    {
        waveformBuffer.fill(0.0f);
        accent = GalleryColors::get(GalleryColors::xoGold);
        const int hz = A11y::prefersReducedMotion() ? 10 : 30;
        startTimerHz(hz);
        setOpaque(false);
    }

    ~WaveformDisplay() override { stopTimer(); }

    // Which engine slot to read waveform data from.
    void setSlot(int slot) noexcept
    {
        currentSlot = slot;
    }

    // Accent color used for the waveform stroke and center line.
    void setAccentColour(juce::Colour c)
    {
        accent = c;
        repaint();
    }

    // Engine identity — enables CRT mode for "Optic".
    void setEngineId(const juce::String& id)
    {
        engineId = id;
        crtMode  = (id == "Optic");
        if (crtMode)
            accent = juce::Colour(0xFF00FF41); // Phosphor Green
        repaint();
    }

    //--------------------------------------------------------------------------
    void timerCallback() override
    {
        // NEVER allocate here — waveformBuffer is pre-allocated in the constructor.
        std::array<float, kDisplaySamples> newBuf;
        processor.getWaveformFifo(currentSlot).readLatest(newBuf.data(),
                                                          kDisplaySamples);

        // P2 fix: only repaint when the waveform data actually changed —
        // spot-check 8 evenly-spaced samples to avoid a full 256-sample compare.
        bool changed = false;
        for (size_t i = 0; i < kDisplaySamples; i += 32)
        {
            if (newBuf[i] != waveformBuffer[i]) { changed = true; break; }
        }
        waveformBuffer = newBuf;
        if (changed)
            repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Label removed: EngineDetailPanel already draws "OSCILLOSCOPE" in the
        // 12px label row immediately above this component (duplicate fix).
        // Waveform area is the full component bounds.
        const auto waveArea  = bounds;
        const float ww       = waveArea.getWidth();
        const float wh       = waveArea.getHeight();
        const float waveTop  = waveArea.getY();
        const float midY     = waveTop + wh * 0.5f;

        // ── Background — flat surface() fill ────────────────────────────────
        g.setColour(GalleryColors::get(GalleryColors::surface()));
        g.fillRoundedRectangle(waveArea, 3.0f);

        // ── Center line (1px, 30% opacity) — always drawn ───────────────────
        g.setColour(accent.withAlpha(0.30f));
        g.drawHorizontalLine(juce::roundToInt(midY), waveArea.getX(), waveArea.getRight());

        // ── Skip waveform path when FIFO is all zeros (idle/no engine) ───────
        bool allZero = true;
        for (const float s : waveformBuffer)
        {
            if (s != 0.0f) { allZero = false; break; }
        }
        if (allZero)
            return;

        // ── Build waveform path ──────────────────────────────────────────────
        wavePath.clear();
        const float xStep = ww / static_cast<float>(kDisplaySamples - 1);

        for (size_t i = 0; i < kDisplaySamples; ++i)
        {
            // Clamp sample to [-1, +1] — guard against NaN/Inf from an
            // uninitialized or overdriven FIFO.
            const float sample = juce::jlimit(-1.0f, 1.0f, waveformBuffer[i]);
            const float x = waveArea.getX() + static_cast<float>(i) * xStep;
            const float y = midY - sample * (wh * 0.45f); // 90% of half-height

            if (i == 0)
                wavePath.startNewSubPath(x, y);
            else
                wavePath.lineTo(x, y);
        }

        const juce::PathStrokeType mainStroke(1.5f,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded);

        // ── CRT glow pass (OPTIC only) ───────────────────────────────────────
        if (crtMode)
        {
            // First pass: wide, faint glow halo (2px extra radius via larger
            // stroke weight and high transparency).
            g.setColour(accent.withAlpha(0.25f));
            g.strokePath(wavePath, juce::PathStrokeType(5.0f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
            // Second pass: tighter inner glow.
            g.setColour(accent.withAlpha(0.45f));
            g.strokePath(wavePath, juce::PathStrokeType(3.0f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        }

        // ── Main stroke ──────────────────────────────────────────────────────
        g.setColour(accent);
        g.strokePath(wavePath, mainStroke);

        // ── CRT scanline overlay (OPTIC only) ────────────────────────────────
        if (crtMode)
        {
            const juce::Colour scanlineColour = juce::Colours::black.withAlpha(0.15f);
            g.setColour(scanlineColour);
            for (float sy = waveTop; sy < waveTop + wh; sy += 2.0f)
                g.drawHorizontalLine(juce::roundToInt(sy), waveArea.getX(), waveArea.getRight());
        }
    }

private:
    XOlokunProcessor&                  processor;
    std::array<float, kDisplaySamples> waveformBuffer {};
    int                                currentSlot = 0;
    juce::Colour                       accent;
    juce::String                       engineId;
    bool                               crtMode = false;
    juce::Path                         wavePath; // P10: cached to avoid alloc per paint()

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};

//==============================================================================
// MiniWaveform — Compact waveform thumbnail for CompactEngineTile (32×16pt).
//
// Reads 64 samples from the processor's WaveformFifo at 30Hz (10Hz with
// reduced motion). Draws a thin 1px path downsampled to component width.
// No background, no center line, no CRT mode.
// Renders nothing when slot is empty (all zeros stay invisible via skip).
class MiniWaveform : public juce::Component, private juce::Timer
{
public:
    static constexpr size_t kMiniSamples = 64;

    explicit MiniWaveform(XOlokunProcessor& proc)
        : processor(proc)
    {
        miniBuffer.fill(0.0f);
        accent = GalleryColors::get(GalleryColors::xoGold);
        const int hz = A11y::prefersReducedMotion() ? 10 : 30;
        startTimerHz(hz);
        setOpaque(false);
    }

    ~MiniWaveform() override { stopTimer(); }

    // Which engine slot to read waveform data from.
    void setSlot(int slot) noexcept
    {
        currentSlot = slot;
    }

    // Accent color used for the waveform stroke.
    void setAccentColour(juce::Colour c)
    {
        accent = c;
        repaint();
    }

    //--------------------------------------------------------------------------
    void timerCallback() override
    {
        // NEVER allocate here — miniBuffer is pre-allocated in the constructor.
        std::array<float, kMiniSamples> newBuf;
        processor.getWaveformFifo(currentSlot).readLatest(newBuf.data(),
                                                          kMiniSamples);

        // P2 fix (MiniWaveform): spot-check 4 evenly-spaced samples; only
        // repaint when data actually changed to avoid 30Hz idle repaints.
        bool changed = false;
        for (size_t i = 0; i < kMiniSamples; i += 16)
        {
            if (newBuf[i] != miniBuffer[i]) { changed = true; break; }
        }
        miniBuffer = newBuf;
        if (changed)
            repaint();
    }

    void paint(juce::Graphics& g) override
    {
        // When the slot is empty (buffer all zeros), skip painting entirely so
        // an unloaded tile shows nothing rather than a flat center line.
        bool allZero = true;
        for (const float s : miniBuffer)
        {
            if (s != 0.0f) { allZero = false; break; }
        }
        if (allZero)
            return;

        const float w    = static_cast<float>(getWidth());
        const float h    = static_cast<float>(getHeight());
        const float midY = h * 0.5f;

        miniPath.clear();
        const int iw = getWidth();
        for (int x = 0; x < iw; ++x)
        {
            // Downsample: map each output pixel to an input sample index.
            const size_t idx = static_cast<size_t>(
                x * static_cast<int>(kMiniSamples) / iw);
            // Use ?? 0 equivalent: guard the index, never rely on ||-coercion
            // of 0.0f (0.0 is a valid sample value — CLAUDE.md critical pattern).
            const float sample = juce::jlimit(-1.0f, 1.0f, miniBuffer[idx]);
            const float y = midY - sample * midY * 0.9f; // 90% height scaling

            if (x == 0)
                miniPath.startNewSubPath(static_cast<float>(x), y);
            else
                miniPath.lineTo(static_cast<float>(x), y);
        }

        g.setColour(accent);
        g.strokePath(miniPath, juce::PathStrokeType(1.0f));
    }

private:
    XOlokunProcessor&               processor;
    std::array<float, kMiniSamples> miniBuffer {};
    int                             currentSlot = 0;
    juce::Colour                    accent;
    juce::Path                      miniPath; // P11: cached to avoid alloc per paint()

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiniWaveform)
};

} // namespace xolokun
