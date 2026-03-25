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
        processor.getWaveformFifo(currentSlot).readLatest(waveformBuffer.data(),
                                                          kDisplaySamples);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        const float w     = bounds.getWidth();
        const float h     = bounds.getHeight();
        const float midY  = h * 0.5f;

        // ── Background gradient ──────────────────────────────────────────────
        {
            const juce::Colour topColour    = GalleryColors::get(GalleryColors::slotBg());
            const juce::Colour bottomColour = topColour.darker(0.08f);
            g.setGradientFill(juce::ColourGradient(topColour,    0.0f, 0.0f,
                                                   bottomColour, 0.0f, h,
                                                   false));
            g.fillRoundedRectangle(bounds, 3.0f);
        }

        // ── Center line (1px, 50% opacity) ──────────────────────────────────
        g.setColour(accent.withAlpha(0.50f));
        g.drawHorizontalLine(juce::roundToInt(midY), 0.0f, w);

        // ── Build waveform path ──────────────────────────────────────────────
        juce::Path wavePath;
        const float xStep = w / static_cast<float>(kDisplaySamples - 1);

        for (size_t i = 0; i < kDisplaySamples; ++i)
        {
            // Clamp sample to [-1, +1] — guard against NaN/Inf from an
            // uninitialized or overdriven FIFO.
            const float sample = juce::jlimit(-1.0f, 1.0f, waveformBuffer[i]);
            const float x = static_cast<float>(i) * xStep;
            const float y = midY - sample * (midY * 0.90f); // 90% height scaling

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
            for (float y = 0.0f; y < h; y += 2.0f)
                g.drawHorizontalLine(juce::roundToInt(y), 0.0f, w);
        }
    }

private:
    XOlokunProcessor&                  processor;
    std::array<float, kDisplaySamples> waveformBuffer {};
    int                                currentSlot = 0;
    juce::Colour                       accent;
    juce::String                       engineId;
    bool                               crtMode = false;

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
        processor.getWaveformFifo(currentSlot).readLatest(miniBuffer.data(),
                                                         kMiniSamples);
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

        juce::Path p;
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
                p.startNewSubPath(static_cast<float>(x), y);
            else
                p.lineTo(static_cast<float>(x), y);
        }

        g.setColour(accent);
        g.strokePath(p, juce::PathStrokeType(1.0f));
    }

private:
    XOlokunProcessor&               processor;
    std::array<float, kMiniSamples> miniBuffer {};
    int                             currentSlot = 0;
    juce::Colour                    accent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiniWaveform)
};

} // namespace xolokun
