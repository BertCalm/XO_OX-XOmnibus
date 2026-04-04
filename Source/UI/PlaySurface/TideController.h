// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

/*
    TideController.h
    ================
    A 120pt circular water-surface simulation using the 2D wave equation on a
    16×16 grid. Rendered as a height-field with the engine accent colour.
    Assignable to any parameter (default: filter cutoff).

    Physics model — discrete 2D wave equation (FDTD):
        laplacian(x,y)  = h[x-1][y] + h[x+1][y] + h[x][y-1] + h[x][y+1]
                          - 4 * h[x][y]
        velocity[x][y] += laplacian * kPropagationSpeed
        velocity[x][y] *= kDamping
        height[x][y]   += velocity[x][y]
    Boundary: edge cells are clamped to 0 (absorbing boundary).

    Rendering:
        - Circular clip (120pt diameter).
        - Each 16×16 cell is filled by a solid colour derived from its height:
              positive → accent colour brightened toward white
              zero     → accent colour at base opacity
              negative → accent colour darkened toward black
        - A thin bioluminescent ring is drawn at the boundary when |avg| > 0.

    Interaction:
        - mouseDown  → add a large ripple (amplitude 1.0) at click position.
        - mouseDrag  → add smaller ripples (amplitude 0.4) continuously.
        - The mean height of the centre 4×4 cells is mapped to [0, 1] and
          reported via onValueChanged and getValue().

    Timer: 30 Hz for simulation step + repaint.

    Parameter wiring:
        Call setTargetParameter(param) to attach a JUCE parameter.
        When onValueChanged fires the attached parameter is updated via
        setValueNotifyingHost() on the message thread — safe because this
        component only ever runs on the message thread.

    Namespace: xoceanus
    JUCE 8, C++17
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <cmath>
#include <algorithm>
#include "../GalleryColors.h"

namespace xoceanus
{

//==============================================================================
class TideController : public juce::Component, private juce::Timer
{
public:
    //==========================================================================
    TideController()
    {
        // Grid state is zero-initialised by the = {} member declarations;
        // no further init required.

        setOpaque(false);

        // Add a tiny seed ripple at centre so the surface is not flat on first
        // appearance — gives the user a visual hint that it responds.
        addRipple(0.5f, 0.5f, 0.25f);

        startTimerHz(30);
    }

    ~TideController() override { stopTimer(); }

    //==========================================================================
    // Configuration

    /// Set the engine accent colour used to tint the height-field.
    void setAccentColor(juce::Colour c)
    {
        accent_ = c;
        repaint();
    }

    /// Attach a JUCE parameter. When the water surface value changes,
    /// setValueNotifyingHost() is called on this parameter.
    /// Pass nullptr to detach.
    void setTargetParameter(juce::RangedAudioParameter* param) { targetParam_ = param; }

    //==========================================================================
    // Output

    /// Current control value in [0, 1]. Thread-safe read (atomic float).
    float getValue() const { return currentValue_.load(std::memory_order_relaxed); }

    /// Called on the message thread whenever the output value changes.
    std::function<void(float)> onValueChanged;

    //==========================================================================
    // juce::Component overrides

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        float diameter = std::min(bounds.getWidth(), bounds.getHeight());
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float radius = diameter * 0.5f;

        // ── Circular clip ────────────────────────────────────────────────────
        juce::Path circle;
        circle.addEllipse(cx - radius, cy - radius, diameter, diameter);
        g.saveState();
        g.reduceClipRegion(circle);

        // ── Background — very dark base to make accent colours pop ───────────
        g.setColour(juce::Colour(0xFF111318));
        g.fillEllipse(cx - radius, cy - radius, diameter, diameter);

        // ── Height field — draw each cell ────────────────────────────────────
        float cellW = diameter / static_cast<float>(kGridSize);
        float cellH = diameter / static_cast<float>(kGridSize);
        float originX = cx - radius;
        float originY = cy - radius;

        for (int row = 0; row < kGridSize; ++row)
        {
            for (int col = 0; col < kGridSize; ++col)
            {
                float h = height[row][col];

                // Clamp to reasonable range for colour mapping
                float clamped = std::clamp(h, -1.0f, 1.0f);

                juce::Colour cellColour;
                if (clamped >= 0.0f)
                {
                    // Positive height: brighten toward white
                    cellColour = accent_.interpolatedWith(juce::Colours::white, clamped * 0.7f);
                    cellColour = cellColour.withAlpha(0.15f + clamped * 0.72f);
                }
                else
                {
                    // Negative height: darken toward black
                    float depth = -clamped;
                    cellColour = accent_.interpolatedWith(juce::Colours::black, depth * 0.8f);
                    cellColour = cellColour.withAlpha(0.08f + depth * 0.35f);
                }

                float x = originX + static_cast<float>(col) * cellW;
                float y = originY + static_cast<float>(row) * cellH;

                g.setColour(cellColour);
                // Slight overlap (+0.5) to avoid sub-pixel gaps between cells
                g.fillRect(x, y, cellW + 0.5f, cellH + 0.5f);
            }
        }

        g.restoreState();

        // ── Bioluminescent boundary ring ─────────────────────────────────────
        float avgMagnitude = computeCentreAvg();
        float ringAlpha = std::clamp(std::abs(avgMagnitude) * 2.5f, 0.0f, 0.65f);
        if (ringAlpha > 0.01f)
        {
            g.setColour(accent_.withAlpha(ringAlpha));
            g.drawEllipse(cx - radius + 1.5f, cy - radius + 1.5f,
                          diameter - 3.0f, diameter - 3.0f, 1.5f);
        }

        // ── Static outer ring ────────────────────────────────────────────────
        g.setColour(accent_.withAlpha(0.30f));
        g.drawEllipse(cx - radius + 0.5f, cy - radius + 0.5f,
                      diameter - 1.0f, diameter - 1.0f, 1.0f);

        // ── Label ────────────────────────────────────────────────────────────
        g.setColour(accent_.withAlpha(0.60f));
        g.setFont(GalleryFonts::body(8.0f));
        g.drawText("TIDE", bounds.removeFromBottom(12.0f), juce::Justification::centred, false);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        auto norm = mouseToNorm(e);
        addRipple(norm.x, norm.y, 1.0f);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        auto norm = mouseToNorm(e);
        addRipple(norm.x, norm.y, 0.4f);
    }

private:
    //==========================================================================
    // Constants

    static constexpr int kGridSize = 16;

    // Propagation speed coefficient — governs how fast wave energy travels
    // across cells per simulation tick. 0.4 is the stability sweet spot for
    // this grid size; values > 0.5 become numerically unstable.
    static constexpr float kPropagationSpeed = 0.40f;

    // Damping per tick: 0.98 → wave energy halves in ~35 ticks (~1.2s at 30Hz)
    static constexpr float kDamping = 0.98f;

    // Centre region for output value derivation: inner 4×4 (rows/cols 6–9)
    static constexpr int kCentreMin = 6;
    static constexpr int kCentreMax = 10; // exclusive

    //==========================================================================
    // Grid state (row-major: [row][col])

    float height[kGridSize][kGridSize]   = {};
    float velocity[kGridSize][kGridSize] = {};

    //==========================================================================
    // Visual / parameter state

    juce::Colour accent_{ 0xFFE9C46A }; // Default: XO Gold
    juce::RangedAudioParameter* targetParam_ = nullptr;
    std::atomic<float> currentValue_{ 0.5f };

    //==========================================================================
    // Simulation

    /// Add a circular ripple centred at normalised position (normX, normY).
    /// amplitude is the peak displacement injected at the closest grid cell.
    void addRipple(float normX, float normY, float amplitude)
    {
        // Map normalised [0,1] → grid integer coordinates
        int cx = static_cast<int>(std::clamp(normX, 0.0f, 1.0f) * static_cast<float>(kGridSize - 1) + 0.5f);
        int cy = static_cast<int>(std::clamp(normY, 0.0f, 1.0f) * static_cast<float>(kGridSize - 1) + 0.5f);
        cx = std::clamp(cx, 1, kGridSize - 2);
        cy = std::clamp(cy, 1, kGridSize - 2);

        // Splash: the centre cell plus its 4 immediate neighbours
        height[cy][cx] += amplitude;
        height[cy - 1][cx] += amplitude * 0.5f;
        height[cy + 1][cx] += amplitude * 0.5f;
        height[cy][cx - 1] += amplitude * 0.5f;
        height[cy][cx + 1] += amplitude * 0.5f;
    }

    /// Advance the 2D wave equation by one tick.
    ///
    /// Discrete FDTD wave equation:
    ///   laplacian(x,y)  = h[y][x-1] + h[y][x+1] + h[y-1][x] + h[y+1][x]
    ///                     - 4 * h[y][x]
    ///   velocity[y][x] += laplacian * kPropagationSpeed
    ///   velocity[y][x] *= kDamping
    ///   height[y][x]   += velocity[y][x]
    ///
    /// Boundary policy: edge cells (row/col 0 and kGridSize-1) are clamped
    /// to 0 after each step, acting as absorbing walls.
    void stepSimulation()
    {
        // Compute laplacian for interior cells and update velocity + height
        for (int row = 1; row < kGridSize - 1; ++row)
        {
            for (int col = 1; col < kGridSize - 1; ++col)
            {
                float laplacian = height[row][col - 1]
                                + height[row][col + 1]
                                + height[row - 1][col]
                                + height[row + 1][col]
                                - 4.0f * height[row][col];

                velocity[row][col] += laplacian * kPropagationSpeed;
                velocity[row][col] *= kDamping;
                height[row][col] += velocity[row][col];
            }
        }

        // Absorbing boundary — clamp edges to zero
        for (int i = 0; i < kGridSize; ++i)
        {
            height[0][i] = height[kGridSize - 1][i] = 0.0f;
            height[i][0] = height[i][kGridSize - 1] = 0.0f;
            velocity[0][i] = velocity[kGridSize - 1][i] = 0.0f;
            velocity[i][0] = velocity[i][kGridSize - 1] = 0.0f;
        }
    }

    /// Compute the mean height of the centre 4×4 region ([6..9] × [6..9]).
    /// Returns a value that is roughly in the range [-1, 1].
    float computeCentreAvg() const
    {
        float sum = 0.0f;
        int count = 0;
        for (int row = kCentreMin; row < kCentreMax; ++row)
        {
            for (int col = kCentreMin; col < kCentreMax; ++col)
            {
                sum += height[row][col];
                ++count;
            }
        }
        return count > 0 ? sum / static_cast<float>(count) : 0.0f;
    }

    /// Map the centre average height (roughly [-1, 1]) to a control value
    /// in [0, 1].  Uses a soft sigmoid-like mapping to keep small ripples
    /// near 0.5 and only reach the extremes on large impacts.
    static float heightToValue(float avgHeight)
    {
        // Compress the raw height through a gentle tanh to keep things
        // musical — a full-strength splash lands at ~0.95, not clipping 1.0.
        float shaped = std::tanh(avgHeight * 2.5f);   // [-1, 1] soft
        return std::clamp(0.5f + shaped * 0.5f, 0.0f, 1.0f);
    }

    //==========================================================================
    // Timer callback

    void timerCallback() override
    {
        stepSimulation();

        float avg = computeCentreAvg();
        float newValue = heightToValue(avg);

        float prev = currentValue_.load(std::memory_order_relaxed);
        if (std::abs(newValue - prev) > 0.0005f)
        {
            currentValue_.store(newValue, std::memory_order_relaxed);

            // Notify listener (always message thread)
            if (onValueChanged)
                onValueChanged(newValue);

            // Update the target parameter directly from message thread.
            // newValue is already in [0, 1] (normalised); setValueNotifyingHost
            // expects a normalised value, so no conversion is needed.
            if (targetParam_ != nullptr)
                targetParam_->setValueNotifyingHost(newValue);
        }

        repaint();
    }

    //==========================================================================
    // Helpers

    /// Convert a juce::MouseEvent position to normalised [0,1]×[0,1] grid coords.
    juce::Point<float> mouseToNorm(const juce::MouseEvent& e) const
    {
        auto b = getLocalBounds().toFloat();
        float diameter = std::min(b.getWidth(), b.getHeight());
        float originX = b.getCentreX() - diameter * 0.5f;
        float originY = b.getCentreY() - diameter * 0.5f;

        float nx = std::clamp((static_cast<float>(e.x) - originX) / diameter, 0.0f, 1.0f);
        float ny = std::clamp((static_cast<float>(e.y) - originY) / diameter, 0.0f, 1.0f);
        return { nx, ny };
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TideController)
};

} // namespace xoceanus
