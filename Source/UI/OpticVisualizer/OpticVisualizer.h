// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "../../Engines/Optic/OpticEngine.h"
#include "../../DSP/FastMath.h"
#include "../GalleryColors.h"
#include <array>
#include <cmath>

namespace xoceanus {

//==============================================================================
// OpticVisualizer — Winamp/Milkdrop-inspired audio-reactive visualizer.
//
// Runs entirely on the UI thread at 30 Hz. Reads modulation data from
// OpticModOutputs (lock-free atomics) — zero audio-thread interaction.
//
// 4 visualization modes:
//   Scope     — Oscilloscope waveform with phosphor trail
//   Spectrum  — 8-band spectrum analyzer with peak hold
//   Milkdrop  — Psychedelic particle field with feedback (the Winamp one)
//   Particles — Granular particle cloud driven by spectral energy
//
// Accent: Phosphor Green #00FF41
//==============================================================================
class OpticVisualizer : public juce::Component, private juce::Timer
{
public:

    static constexpr int kPhosphorGreen  = 0xFF00FF41;
    static constexpr int kPhosphorDim    = 0xFF004D13;
    static constexpr int kCRTBackground  = 0xFF050A05;
    static constexpr int kScanlineAlpha  = 0x18000000;

    //--------------------------------------------------------------------------
    // Construction
    //--------------------------------------------------------------------------

    explicit OpticVisualizer (OpticEngine* engine = nullptr)
        : engineRef (engine)
    {
        setOpaque (true);
        setTitle ("Optic Visualizer");
        setDescription ("Audio-reactive visualization display. "
                        "Shows waveform, spectrum, or particle animation driven by the Optic engine.");
        resetParticles();
    }

    void setEngine (OpticEngine* engine) { engineRef = engine; }

    void start() { startTimerHz (reducedMotion ? 10 : 30); }
    void stop()  { stopTimer(); }

    /// Enable reduced motion mode (WCAG 2.3.3 Animation from Interactions).
    /// Falls back to Scope mode and reduces frame rate.
    void setReducedMotion (bool enabled)
    {
        reducedMotion = enabled;
        if (reducedMotion && (vizMode == Mode::Milkdrop || vizMode == Mode::Particles))
            vizMode = Mode::Scope;
        if (isTimerRunning())
            start(); // re-start at appropriate frame rate
    }

    bool isReducedMotion() const { return reducedMotion; }

    //--------------------------------------------------------------------------
    // Component
    //--------------------------------------------------------------------------

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // CRT-style dark green-black background
        g.fillAll (juce::Colour (kCRTBackground));

        if (engineRef == nullptr) { paintNoSignal (g, bounds); return; }

        // Read mod data (lock-free)
        auto& mods = engineRef->getModOutputs();
        FrameData frame;
        frame.pulse     = mods.getPulse();
        frame.bass      = mods.getBass();
        frame.mid       = mods.getMid();
        frame.high      = mods.getHigh();
        frame.centroid  = mods.getCentroid();
        frame.flux      = mods.getFlux();
        frame.energy    = mods.getEnergy();
        frame.transient = mods.getTransient();

        // Render based on mode
        switch (vizMode)
        {
            case Mode::Scope:     paintScope (g, bounds, frame);     break;
            case Mode::Spectrum:  paintSpectrum (g, bounds, frame);  break;
            case Mode::Milkdrop:  paintMilkdrop (g, bounds, frame);  break;
            case Mode::Particles: paintParticles (g, bounds, frame); break;
        }

        // CRT scanlines overlay
        paintScanlines (g, bounds);

        // AutoPulse indicator (bottom-left throb)
        if (frame.pulse > 0.01f)
            paintPulseIndicator (g, bounds, frame.pulse);
    }

    void resized() override { }

    //--------------------------------------------------------------------------
    // Mode control
    //--------------------------------------------------------------------------

    enum class Mode { Scope, Spectrum, Milkdrop, Particles };

    void setMode (Mode m)
    {
        // WCAG 2.3.3: don't allow high-motion modes when reduced motion is active
        if (reducedMotion && (m == Mode::Milkdrop || m == Mode::Particles))
            m = Mode::Scope;
        vizMode = m;
    }
    Mode getMode() const { return vizMode; }

    void setFeedback (float f)  { feedback = juce::jlimit (0.0f, 1.0f, f); }
    void setSpeed (float s)     { speed = juce::jlimit (0.1f, 4.0f, s); }
    void setIntensity (float i) { intensity = juce::jlimit (0.0f, 1.0f, i); }

private:

    //--------------------------------------------------------------------------
    // Frame data (snapshot from atomics)
    //--------------------------------------------------------------------------

    struct FrameData {
        float pulse = 0, bass = 0, mid = 0, high = 0;
        float centroid = 0, flux = 0, energy = 0, transient = 0;
    };

    //--------------------------------------------------------------------------
    // Timer callback — triggers repaint at 30 Hz
    //--------------------------------------------------------------------------

    void timerCallback() override
    {
        frameCount++;

        // Sync viz parameters from APVTS (via engine accessors) so that
        // optic_vizMode, optic_vizFeedback, optic_vizSpeed, optic_vizIntensity
        // actually drive the visualizer. Runs on the UI thread — safe to read
        // the atomic parameter pointers via the engine's getViz*() accessors.
        if (engineRef != nullptr)
        {
            int modeIdx = engineRef->getVizMode();
            Mode newMode = Mode::Milkdrop; // default matches APVTS default index 2
            switch (modeIdx)
            {
                case 0: newMode = Mode::Scope;     break;
                case 1: newMode = Mode::Spectrum;  break;
                case 2: newMode = Mode::Milkdrop;  break;
                case 3: newMode = Mode::Particles; break;
                default: break;
            }
            // setMode() respects reducedMotion guard internally
            setMode (newMode);
            setFeedback  (engineRef->getVizFeedback());
            setSpeed     (engineRef->getVizSpeed());
            setIntensity (engineRef->getVizIntensity());
        }

        repaint();
    }

    //--------------------------------------------------------------------------
    // Scope mode — oscilloscope waveform with phosphor glow trail
    //--------------------------------------------------------------------------

    void paintScope (juce::Graphics& g, juce::Rectangle<float> bounds, const FrameData& f)
    {
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float w = bounds.getWidth();
        float h = bounds.getHeight() * 0.35f;

        // Push new waveform point from modulation signals
        float y = f.bass * 0.5f + f.mid * 0.3f + f.high * 0.2f;
        y += f.pulse * 0.4f;
        scopeHistory[scopeWriteIdx] = y;
        scopeWriteIdx = (scopeWriteIdx + 1) % kScopeHistorySize;

        // Draw trail (older = dimmer)
        int trailLen = (int)(feedback * 4) + 1;
        for (int trail = trailLen; trail >= 0; --trail)
        {
            float alpha = (1.0f - (float)trail / (trailLen + 1)) * intensity;

            juce::Path wavePath;
            bool started = false;

            for (int i = 0; i < kScopeHistorySize; ++i)
            {
                int idx = (scopeWriteIdx - kScopeHistorySize + i + trail * 3
                          + kScopeHistorySize * 100) % kScopeHistorySize;
                float val = scopeHistory[idx];

                float t = (float)i / (kScopeHistorySize - 1);
                float xPos = bounds.getX() + t * w;
                float yPos = cy + val * h * (0.5f + f.energy * 0.5f);

                // Add Lissajous-like wobble
                float wobble = fastSin (t * 6.28f * (1.0f + f.centroid * 2.0f)
                                        + frameCount * speed * 0.05f) * h * 0.1f * f.energy;
                yPos += wobble;

                if (!started) { wavePath.startNewSubPath (xPos, yPos); started = true; }
                else wavePath.lineTo (xPos, yPos);
            }

            auto c = juce::Colour (kPhosphorGreen).withAlpha (alpha * 0.6f);
            g.setColour (c);
            g.strokePath (wavePath, juce::PathStrokeType (
                1.5f + f.energy * 2.0f, juce::PathStrokeType::curved));
        }

        // Phosphor glow center line
        g.setColour (juce::Colour (kPhosphorGreen).withAlpha (0.15f));
        g.drawHorizontalLine ((int)cy, bounds.getX(), bounds.getRight());
    }

    //--------------------------------------------------------------------------
    // Spectrum mode — 8-band analyzer with peak hold
    //--------------------------------------------------------------------------

    void paintSpectrum (juce::Graphics& g, juce::Rectangle<float> bounds, const FrameData& f)
    {
        float bandValues[8] = {
            f.bass * 0.7f, f.bass * 0.5f,       // Sub, Bass
            f.mid * 0.4f,  f.mid * 0.5f, f.mid * 0.3f,  // Lo-Mid, Mid, Hi-Mid
            f.high * 0.4f, f.high * 0.3f, f.high * 0.2f  // Presence, Brilliance, Air
        };

        // Add pulse throb to all bands
        for (int i = 0; i < 8; ++i)
            bandValues[i] = juce::jlimit (0.0f, 1.0f, bandValues[i] + f.pulse * 0.2f);

        float barW = bounds.getWidth() / 10.0f;
        float maxH = bounds.getHeight() * 0.85f;
        float bottom = bounds.getBottom() - bounds.getHeight() * 0.05f;

        for (int i = 0; i < 8; ++i)
        {
            float x = bounds.getX() + barW * (i + 1);
            float barH = bandValues[i] * maxH * intensity;

            // Update peak hold
            if (bandValues[i] > peakHold[i])
                peakHold[i] = bandValues[i];
            else
                peakHold[i] *= 0.97f; // Slow decay

            // Bar gradient: green at bottom, bright green at top
            juce::ColourGradient grad (
                juce::Colour (kPhosphorDim).withAlpha (0.7f), x, bottom,
                juce::Colour (kPhosphorGreen).withAlpha (0.9f), x, bottom - barH,
                false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (x, bottom - barH, barW * 0.75f, barH, 2.0f);

            // Peak hold marker
            float peakY = bottom - peakHold[i] * maxH * intensity;
            g.setColour (juce::Colour (kPhosphorGreen).withAlpha (0.9f));
            g.fillRect (x, peakY, barW * 0.75f, 2.0f);
        }
    }

    //--------------------------------------------------------------------------
    // Milkdrop mode — the Winamp one: swirling particles with feedback trails
    //--------------------------------------------------------------------------

    void paintMilkdrop (juce::Graphics& g, juce::Rectangle<float> bounds, const FrameData& f)
    {
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.4f;

        // Feedback trail (dim previous frame)
        // Simulated by drawing a translucent dark overlay
        g.setColour (juce::Colour (kCRTBackground).withAlpha (1.0f - feedback * 0.85f));
        g.fillRect (bounds);

        float t = frameCount * speed * 0.02f;
        float energyScale = 0.5f + f.energy * 0.5f;

        // Central mandala — Lissajous orbits modulated by spectrum
        int nCurves = 5;
        for (int c = 0; c < nCurves; ++c)
        {
            float aFreq = (float)(c + 1);
            float bFreq = (float)(c + 2);
            float phase = t + c * 1.2f + f.centroid * 3.14f;

            juce::Path curve;
            constexpr int pts = 200;
            for (int i = 0; i < pts; ++i)
            {
                float u = (float)i / pts * 6.283f;
                float r = radius * energyScale
                         * (0.6f + 0.4f * fastSin (u * 3.0f + t * 0.5f));
                float x = cx + r * fastSin (aFreq * u + phase);
                float y = cy + r * fastCos (bFreq * u + phase * 0.7f);

                if (i == 0) curve.startNewSubPath (x, y);
                else curve.lineTo (x, y);
            }
            curve.closeSubPath();

            // Color shifts with centroid: green → cyan → white
            float hue = 0.33f + f.centroid * 0.15f;
            float sat = 0.7f - f.energy * 0.2f;
            auto color = juce::Colour::fromHSL (hue, sat, 0.5f + f.energy * 0.3f, 1.0f);
            g.setColour (color.withAlpha (intensity * 0.35f / nCurves * (nCurves - c)));
            g.strokePath (curve, juce::PathStrokeType (
                1.0f + f.energy * 1.5f, juce::PathStrokeType::curved));
        }

        // Burst particles on transient / pulse
        if (f.transient > 0.5f || f.pulse > 0.6f)
        {
            int nBurst = (int)(f.pulse * 12 + f.transient * 8);
            for (int i = 0; i < nBurst; ++i)
            {
                float angle = (float)i / nBurst * 6.283f + t;
                float dist = radius * 0.3f * (0.5f + f.energy) * ((frameCount * 7 + i) % 3 * 0.3f + 0.3f);
                float px = cx + dist * fastCos (angle);
                float py = cy + dist * fastSin (angle);
                float sz = 2.0f + f.pulse * 4.0f;
                g.setColour (juce::Colour (kPhosphorGreen).withAlpha (intensity * 0.6f));
                g.fillEllipse (px - sz * 0.5f, py - sz * 0.5f, sz, sz);
            }
        }
    }

    //--------------------------------------------------------------------------
    // Particles mode — granular cloud driven by spectral energy
    //--------------------------------------------------------------------------

    void paintParticles (juce::Graphics& g, juce::Rectangle<float> bounds, const FrameData& f)
    {
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();

        // Update particle positions
        for (auto& p : particles)
        {
            // Gravity toward center
            float dx = cx - p.x, dy = cy - p.y;
            float dist = std::sqrt (dx * dx + dy * dy) + 1.0f;
            float gravity = 0.2f / dist;
            p.vx += dx * gravity * speed * 0.01f;
            p.vy += dy * gravity * speed * 0.01f;

            // Spectral forces
            p.vx += (f.centroid - 0.5f) * 0.5f * f.energy;
            p.vy += (f.bass - f.high) * 0.3f;

            // Pulse burst — kick particles outward
            if (f.pulse > 0.5f)
            {
                float burstAngle = std::atan2 (p.y - cy, p.x - cx);
                float burstForce = f.pulse * 3.0f;
                p.vx += fastCos (burstAngle) * burstForce;
                p.vy += fastSin (burstAngle) * burstForce;
            }

            // Friction
            p.vx *= 0.96f;
            p.vy *= 0.96f;

            p.x += p.vx;
            p.y += p.vy;

            // Wrap to bounds
            if (p.x < bounds.getX()) p.x = bounds.getRight();
            if (p.x > bounds.getRight()) p.x = bounds.getX();
            if (p.y < bounds.getY()) p.y = bounds.getBottom();
            if (p.y > bounds.getBottom()) p.y = bounds.getY();

            // Draw particle
            float alpha = juce::jlimit (0.05f, 0.9f,
                (1.0f - dist / (bounds.getWidth() * 0.5f)) * intensity);
            float sz = p.size * (1.0f + f.energy * 0.5f);

            auto color = juce::Colour (kPhosphorGreen)
                .interpolatedWith (juce::Colours::cyan, f.centroid * 0.4f);
            g.setColour (color.withAlpha (alpha));
            g.fillEllipse (p.x - sz * 0.5f, p.y - sz * 0.5f, sz, sz);
        }
    }

    //--------------------------------------------------------------------------
    // CRT scanline overlay
    //--------------------------------------------------------------------------

    void paintScanlines (juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setColour (juce::Colour (kScanlineAlpha));
        int h = (int)bounds.getHeight();
        int w = (int)bounds.getWidth();
        for (int y = 0; y < h; y += 3)
            g.fillRect (0, y, w, 1);
    }

    //--------------------------------------------------------------------------
    // AutoPulse throb indicator
    //--------------------------------------------------------------------------

    void paintPulseIndicator (juce::Graphics& g, juce::Rectangle<float> bounds, float pulse)
    {
        float sz = 8.0f + pulse * 12.0f;
        float x = bounds.getX() + 16.0f;
        float y = bounds.getBottom() - 16.0f;
        g.setColour (juce::Colour (kPhosphorGreen).withAlpha (pulse * 0.8f));
        g.fillEllipse (x - sz * 0.5f, y - sz * 0.5f, sz, sz);
    }

    //--------------------------------------------------------------------------
    // No-signal display
    //--------------------------------------------------------------------------

    void paintNoSignal (juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setColour (juce::Colour (kPhosphorDim).withAlpha (0.3f));
        g.setFont (GalleryFonts::display (14.0f));
        g.drawText ("OPTIC — NO SIGNAL", bounds, juce::Justification::centred);

        // Idle scanline sweep
        float sweepY = bounds.getY() + std::fmod (frameCount * 2.0f, bounds.getHeight());
        g.setColour (juce::Colour (kPhosphorGreen).withAlpha (0.1f));
        g.fillRect (bounds.getX(), sweepY, bounds.getWidth(), 2.0f);
    }

    //--------------------------------------------------------------------------
    // Particle system
    //--------------------------------------------------------------------------

    struct Particle {
        float x = 0, y = 0, vx = 0, vy = 0, size = 2.0f;
    };

    static constexpr int kNumParticles = 200;

    void resetParticles()
    {
        // Seed particles in a centered cluster relative to component bounds
        auto b = getLocalBounds().toFloat();
        float cx = b.getWidth()  > 0 ? b.getCentreX() : 400.0f;
        float cy = b.getHeight() > 0 ? b.getCentreY() : 200.0f;
        float hw = b.getWidth()  > 0 ? b.getWidth()  * 0.25f : 200.0f;
        float hh = b.getHeight() > 0 ? b.getHeight() * 0.25f : 100.0f;

        uint32_t seed = 12345;
        for (auto& p : particles)
        {
            seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
            p.x = cx - hw + (float)(seed % (uint32_t)(hw * 2.0f + 1.0f));
            seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
            p.y = cy - hh + (float)(seed % (uint32_t)(hh * 2.0f + 1.0f));
            p.vx = 0.0f;
            p.vy = 0.0f;
            seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
            p.size = 1.5f + (float)(seed % 40) * 0.1f;
        }
    }

    //--------------------------------------------------------------------------
    // State
    //--------------------------------------------------------------------------

    OpticEngine* engineRef = nullptr;
    Mode vizMode = Mode::Milkdrop;
    bool reducedMotion = false;
    float feedback = 0.6f;
    float speed = 1.0f;
    float intensity = 0.7f;

    uint64_t frameCount = 0;

    // Scope history ring buffer
    static constexpr int kScopeHistorySize = 256;
    float scopeHistory[kScopeHistorySize] = {};
    int scopeWriteIdx = 0;

    // Spectrum peak hold
    float peakHold[8] = {};

    // Particle system
    std::array<Particle, kNumParticles> particles;
};

} // namespace xoceanus
