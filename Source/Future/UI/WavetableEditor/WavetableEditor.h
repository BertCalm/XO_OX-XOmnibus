// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// WavetableEditor.h — JUCE Component for editing and visualizing wavetable data.
//
// Displays the current frame of a WavetableOscillator as a filled waveform path,
// provides a morph slider to scrub through all frames, left/right frame navigation
// buttons, a normalize button, a generate popup menu (basic shapes), and a
// click-drag canvas for hand-drawing custom waveform shapes.
//
// Gallery Model compliance:
//   - GalleryColors tokens for all colors (theme-aware light/dark)
//   - GalleryFonts for all text
//   - XO Gold (#E9C46A) for accents, active states, and frame indicator
//   - Dark background (#0E0E10 in dark mode, #1A1A1C in dark mode surface)
//
// Default size: 400×200. Fully resizable — all geometry is derived from
// getLocalBounds() in paint() and resized().
//
// Threading contract:
//   WavetableOscillator::getFrameWritePointer() is called only from the
//   message thread (mouseDown/mouseDrag). Never call from the audio thread
//   while the editor is visible.
//
// Usage:
//   auto editor = std::make_unique<WavetableEditor>(myWavetableOscillator);
//   editor->setAccentColour(engineAccent);
//   addAndMakeVisible(*editor);

#include <juce_gui_basics/juce_gui_basics.h>
#include "../GalleryColors.h"
#include "../../DSP/WavetableOscillator.h"

namespace xoceanus
{

//==============================================================================
// WavetableEditor
//==============================================================================
class WavetableEditor : public juce::Component
{
public:
    //--------------------------------------------------------------------------
    // Construction
    //--------------------------------------------------------------------------

    /// @param osc        Reference to the WavetableOscillator this editor controls.
    ///                   The oscillator must outlive the editor.
    /// @param sampleRate The audio device sample rate used for Nyquist-correct
    ///                   anti-aliasing when generating basic tables (e.g. 44100,
    ///                   48000, 96000). Defaults to 44100 only as a fallback for
    ///                   editor previews where the audio device rate is unavailable.
    explicit WavetableEditor(WavetableOscillator& osc, double sampleRate = 44100.0)
        : oscRef(osc), sampleRate_(sampleRate)
    {
        // Generate default basic tables if the oscillator has nothing loaded.
        if (!oscRef.isLoaded())
            oscRef.generateBasicTables(sampleRate_);

        // ── Morph Slider ────────────────────────────────────────────────────
        morphSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        morphSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        morphSlider.setRange(0.0, 1.0, 0.0);
        morphSlider.setValue(oscRef.getPosition(), juce::dontSendNotification);
        morphSlider.setTooltip("Morph: scrub through wavetable frames (0 = first, 1 = last)");
        morphSlider.onValueChange = [this]
        {
            oscRef.setMorphPosition(static_cast<float>(morphSlider.getValue()));
            updateFrameLabel();
            repaint();
        };
        addAndMakeVisible(morphSlider);

        // ── Frame Navigation Buttons ────────────────────────────────────────
        prevFrameBtn.setButtonText("<");
        prevFrameBtn.setTooltip("Previous frame");
        prevFrameBtn.onClick = [this] { stepFrame(-1); };
        addAndMakeVisible(prevFrameBtn);

        nextFrameBtn.setButtonText(">");
        nextFrameBtn.setTooltip("Next frame");
        nextFrameBtn.onClick = [this] { stepFrame(+1); };
        addAndMakeVisible(nextFrameBtn);

        // ── Normalize Button ────────────────────────────────────────────────
        normalizeBtn.setButtonText("Normalize");
        normalizeBtn.setTooltip("Normalize current frame to peak ±1.0");
        normalizeBtn.onClick = [this]
        {
            oscRef.normalizeCurrentFrame(getCurrentFrameIndex());
            repaint();
        };
        addAndMakeVisible(normalizeBtn);

        // ── Generate Menu Button ────────────────────────────────────────────
        generateBtn.setButtonText("Generate");
        generateBtn.setTooltip("Replace current frame with a basic waveform shape");
        generateBtn.onClick = [this] { showGenerateMenu(); };
        addAndMakeVisible(generateBtn);

        // Accessibility
        A11y::setup(*this, "Wavetable Editor",
                    "Wavetable frame editor. Draw custom waveforms or generate basic shapes. "
                    "Use the morph slider to navigate between frames.");

        setSize(400, 200);
        updateFrameLabel();
    }

    ~WavetableEditor() override = default;

    //--------------------------------------------------------------------------
    // Configuration
    //--------------------------------------------------------------------------

    /// Set the engine accent colour for waveform stroke and active states.
    void setAccentColour(juce::Colour c)
    {
        accent = c;
        repaint();
    }

    //--------------------------------------------------------------------------
    // juce::Component overrides
    //--------------------------------------------------------------------------

    void resized() override
    {
        auto bounds = getLocalBounds();
        // Derive control heights from the font to scale correctly on HiDPI/Retina.
        // The previous hardcoded 28/32 px were fine at 1x but too small at 2x.
        // Round up to an even number so pixels stay sharp.
        const int controlH = juce::roundToInt(GalleryFonts::body(12.0f).getHeight() * 2.0f + 4);
        const int btnW = juce::roundToInt(GalleryFonts::body(12.0f).getHeight() * 2.4f);
        const int gap = 4;

        // Bottom controls strip
        auto controlStrip = bounds.removeFromBottom(controlH + gap * 2).reduced(gap, gap);

        // [ < ] [ > ]  [frame label gap]  [Normalize]  [Generate]
        auto prevBounds = controlStrip.removeFromLeft(btnW);
        controlStrip.removeFromLeft(gap);
        auto nextBounds = controlStrip.removeFromLeft(btnW);
        controlStrip.removeFromLeft(gap * 2);

        // Right side: Normalize + Generate
        auto genBounds = controlStrip.removeFromRight(80);
        controlStrip.removeFromRight(gap);
        auto normBounds = controlStrip.removeFromRight(80);
        controlStrip.removeFromRight(gap);

        // Remaining space in the middle: morph slider
        auto morphBounds = controlStrip; // what's left

        prevFrameBtn.setBounds(prevBounds);
        nextFrameBtn.setBounds(nextBounds);
        morphSlider.setBounds(morphBounds);
        normalizeBtn.setBounds(normBounds);
        generateBtn.setBounds(genBounds);

        // Canvas area: everything above the control strip
        canvasBounds = bounds.toFloat();
    }

    void paint(juce::Graphics& g) override
    {
        // ── Background ───────────────────────────────────────────────────────
        const auto bg = GalleryColors::darkMode() ? juce::Colour(GalleryColors::Dark::bg)
                                                  : juce::Colour(GalleryColors::Light::shellWhite);

        g.setColour(bg);
        g.fillRoundedRectangle(canvasBounds, 4.0f);

        // ── Grid lines ───────────────────────────────────────────────────────
        const float midY = canvasBounds.getCentreY();
        const juce::Colour gridColor = GalleryColors::darkMode() ? juce::Colour(GalleryColors::Dark::t4)
                                                                 : juce::Colour(GalleryColors::Light::borderGray);

        g.setColour(gridColor.withAlpha(0.4f));
        // Horizontal centre line
        g.drawHorizontalLine(juce::roundToInt(midY), canvasBounds.getX(), canvasBounds.getRight());
        // ±0.5 guide lines
        const float q1Y = canvasBounds.getY() + canvasBounds.getHeight() * 0.25f;
        const float q3Y = canvasBounds.getY() + canvasBounds.getHeight() * 0.75f;
        g.drawHorizontalLine(juce::roundToInt(q1Y), canvasBounds.getX(), canvasBounds.getRight());
        g.drawHorizontalLine(juce::roundToInt(q3Y), canvasBounds.getX(), canvasBounds.getRight());

        // ── Waveform fill + stroke ────────────────────────────────────────────
        const int frameIdx = getCurrentFrameIndex();
        const float* frameData = oscRef.getFrameReadPointer(frameIdx);
        const int fs = oscRef.getFrameSize();

        if (frameData != nullptr && fs > 0)
        {
            const float cw = canvasBounds.getWidth();
            const float ch = canvasBounds.getHeight();
            const float cx = canvasBounds.getX();
            const float cy = canvasBounds.getY();

            juce::Path wavePath, fillPath;
            const int displaySamples = juce::jmin(fs, 2048);

            for (int s = 0; s < displaySamples; ++s)
            {
                const float t = static_cast<float>(s) / static_cast<float>(displaySamples - 1);
                const float px = cx + t * cw;

                // Map sample index into table, potentially downsampling for display.
                const int tableIdx = s * fs / displaySamples;
                const float sample = juce::jlimit(-1.0f, 1.0f, frameData[tableIdx]);
                const float py = midY - sample * (ch * 0.45f); // 90% of half-height

                if (s == 0)
                {
                    wavePath.startNewSubPath(px, py);
                    fillPath.startNewSubPath(px, midY);
                    fillPath.lineTo(px, py);
                }
                else
                {
                    wavePath.lineTo(px, py);
                    fillPath.lineTo(px, py);
                }
            }

            // Close fill path back to centre line
            fillPath.lineTo(cx + cw, midY);
            fillPath.closeSubPath();

            // Fill — accent at low alpha for a soft glow
            g.setColour(accent.withAlpha(0.12f));
            g.fillPath(fillPath);

            // Stroke
            g.setColour(accent.withAlpha(0.85f));
            g.strokePath(wavePath,
                         juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // ── Frame label (top-right) ───────────────────────────────────────────
        const juce::Colour labelColor = GalleryColors::darkMode() ? juce::Colour(GalleryColors::Dark::t2)
                                                                  : juce::Colour(GalleryColors::Light::textMid);

        g.setFont(GalleryFonts::value(11.0f));
        g.setColour(labelColor);
        g.drawText(frameLabel, canvasBounds.reduced(6.0f, 4.0f).toNearestInt(), juce::Justification::topRight, false);

        // ── Canvas border ─────────────────────────────────────────────────────
        g.setColour(GalleryColors::border());
        g.drawRoundedRectangle(canvasBounds, 4.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    // Mouse drawing — click-drag to paint samples into the current frame
    //--------------------------------------------------------------------------

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!canvasBounds.contains(e.position))
            return;
        isDragging = true;
        applyDrawAt(e.position);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!isDragging)
            return;
        applyDrawAt(e.position);
    }

    void mouseUp(const juce::MouseEvent&) override { isDragging = false; }

private:
    //--------------------------------------------------------------------------
    // Helpers
    //--------------------------------------------------------------------------

    /// Returns the frame index that corresponds to the current morph position.
    int getCurrentFrameIndex() const noexcept
    {
        const int nf = oscRef.getNumFrames();
        if (nf <= 0)
            return 0;
        const float pos = oscRef.getPosition();
        const float fposF = pos * static_cast<float>(nf - 1);
        return juce::jlimit(0, nf - 1, static_cast<int>(std::round(fposF)));
    }

    /// Step the current frame by `delta` (−1 or +1) and update morph slider.
    void stepFrame(int delta)
    {
        const int nf = oscRef.getNumFrames();
        if (nf <= 0)
            return;
        const int cur = getCurrentFrameIndex();
        const int next = juce::jlimit(0, nf - 1, cur + delta);
        const float newPos = (nf > 1) ? static_cast<float>(next) / static_cast<float>(nf - 1) : 0.0f;
        oscRef.setMorphPosition(newPos);
        morphSlider.setValue(newPos, juce::dontSendNotification);
        updateFrameLabel();
        repaint();
    }

    /// Paint a sample value at the given pixel position within the canvas.
    void applyDrawAt(juce::Point<float> pt)
    {
        if (!canvasBounds.contains(pt))
            return;

        const int fs = oscRef.getFrameSize();
        float* frame = oscRef.getFrameWritePointer(getCurrentFrameIndex());
        if (frame == nullptr || fs <= 0)
            return;

        // Map x pixel → sample index
        const float relX = (pt.x - canvasBounds.getX()) / canvasBounds.getWidth();
        const int sampleIdx = juce::jlimit(0, fs - 1, static_cast<int>(relX * static_cast<float>(fs)));

        // Map y pixel → amplitude [-1, +1] (top = +1, bottom = -1)
        const float relY = (pt.y - canvasBounds.getY()) / canvasBounds.getHeight();
        const float amp = juce::jlimit(-1.0f, 1.0f, 1.0f - relY * 2.0f);

        frame[sampleIdx] = amp;

        // Interpolate between last drawn position and current to avoid gaps
        if (lastDrawIdx >= 0 && lastDrawIdx != sampleIdx)
        {
            const int lo = juce::jmin(lastDrawIdx, sampleIdx);
            const int hi = juce::jmax(lastDrawIdx, sampleIdx);
            const float loAmp = (lo == lastDrawIdx) ? lastDrawAmp : amp;
            const float hiAmp = (hi == lastDrawIdx) ? lastDrawAmp : amp;
            const float span = static_cast<float>(hi - lo);
            for (int i = lo; i <= hi; ++i)
            {
                const float t = (span > 0.0f) ? static_cast<float>(i - lo) / span : 0.0f;
                frame[i] = loAmp + t * (hiAmp - loAmp);
            }
        }

        lastDrawIdx = sampleIdx;
        lastDrawAmp = amp;
        repaint();
    }

    void updateFrameLabel()
    {
        const int nf = oscRef.getNumFrames();
        const int cur = getCurrentFrameIndex() + 1; // 1-based display
        frameLabel = "Frame " + juce::String(cur) + " / " + juce::String(nf > 0 ? nf : 1);
    }

    /// Show the Generate popup menu and fill the current frame on selection.
    void showGenerateMenu()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Sine");
        menu.addItem(2, "Sawtooth");
        menu.addItem(3, "Square");
        menu.addItem(4, "Triangle");
        menu.addItem(5, "Noise");
        menu.addSeparator();
        menu.addItem(6, "All Basic Tables (4 frames)");

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(generateBtn).withMinimumWidth(120),
                           [this](int result)
                           {
                               if (result == 0)
                                   return;

                               if (result == 6)
                               {
                                   // Replace entire table with the default 4-frame basic set.
                                   oscRef.generateBasicTables(sampleRate_);
                                   morphSlider.setValue(0.0, juce::dontSendNotification);
                                   updateFrameLabel();
                                   repaint();
                                   return;
                               }

                               fillCurrentFrameWithShape(result);
                           });
    }

    /// Write a basic shape into the current frame.
    /// shapeId: 1=Sine, 2=Saw, 3=Square, 4=Tri, 5=Noise
    void fillCurrentFrameWithShape(int shapeId)
    {
        const int fs = oscRef.getFrameSize();
        float* frame = oscRef.getFrameWritePointer(getCurrentFrameIndex());
        if (frame == nullptr || fs <= 0)
            return;

        // We write using additive synthesis (band-limited) for saw/square/tri,
        // and direct formula for sine and noise.
        constexpr float k2Pi = 6.28318530718f;

        if (shapeId == 1) // Sine
        {
            for (int s = 0; s < fs; ++s)
                frame[s] = std::sin(k2Pi * static_cast<float>(s) / static_cast<float>(fs));
        }
        else if (shapeId == 2) // Sawtooth (band-limited, ~32 harmonics)
        {
            for (int s = 0; s < fs; ++s)
                frame[s] = 0.0f;
            for (int h = 1; h <= 32; ++h)
                for (int s = 0; s < fs; ++s)
                    frame[s] += (1.0f / static_cast<float>(h)) *
                                std::sin(k2Pi * static_cast<float>(h) * static_cast<float>(s) / static_cast<float>(fs));
            normalizeFrameInPlace(frame, fs);
        }
        else if (shapeId == 3) // Square (odd harmonics, ~16 terms)
        {
            for (int s = 0; s < fs; ++s)
                frame[s] = 0.0f;
            for (int h = 1; h <= 31; h += 2)
                for (int s = 0; s < fs; ++s)
                    frame[s] += (1.0f / static_cast<float>(h)) *
                                std::sin(k2Pi * static_cast<float>(h) * static_cast<float>(s) / static_cast<float>(fs));
            normalizeFrameInPlace(frame, fs);
        }
        else if (shapeId == 4) // Triangle (odd harmonics, alternating sign, 1/n^2)
        {
            for (int s = 0; s < fs; ++s)
                frame[s] = 0.0f;
            float sign = 1.0f;
            for (int h = 1; h <= 31; h += 2)
            {
                for (int s = 0; s < fs; ++s)
                    frame[s] += sign * (1.0f / static_cast<float>(h * h)) *
                                std::sin(k2Pi * static_cast<float>(h) * static_cast<float>(s) / static_cast<float>(fs));
                sign = -sign;
            }
            normalizeFrameInPlace(frame, fs);
        }
        else if (shapeId == 5) // Noise (white noise, normalized)
        {
            juce::Random rng;
            for (int s = 0; s < fs; ++s)
                frame[s] = rng.nextFloat() * 2.0f - 1.0f;
        }

        repaint();
    }

    /// In-place peak normalize utility for generated frames.
    static void normalizeFrameInPlace(float* frame, int fs) noexcept
    {
        float peak = 0.001f;
        for (int s = 0; s < fs; ++s)
            peak = std::max(peak, std::abs(frame[s]));
        for (int s = 0; s < fs; ++s)
            frame[s] /= peak;
    }

    //--------------------------------------------------------------------------
    // Members
    //--------------------------------------------------------------------------

    WavetableOscillator& oscRef;
    double sampleRate_ = 44100.0; // Audio device rate — used for Nyquist-correct table generation

    // Controls
    juce::Slider morphSlider;
    juce::TextButton prevFrameBtn, nextFrameBtn;
    juce::TextButton normalizeBtn, generateBtn;

    // State
    juce::Rectangle<float> canvasBounds{};
    juce::Colour accent{GalleryColors::xoGold};
    juce::String frameLabel{"Frame 1 / 1"};

    // Drawing state (mouse drag)
    bool isDragging = false;
    int lastDrawIdx = -1;
    float lastDrawAmp = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableEditor)
};

} // namespace xoceanus
