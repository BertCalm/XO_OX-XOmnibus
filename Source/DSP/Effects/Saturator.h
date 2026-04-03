// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include "../FastMath.h"

namespace xoceanus
{

//==============================================================================
// Saturator — Multi-mode saturation/distortion processor.
//
// Four saturation modes, each with distinct harmonic character:
//   - Tube:     Soft-clip tanh waveshaper (warm even harmonics)
//   - Tape:     Asymmetric soft-clip with bias (odd/even mix, compression)
//   - Digital:  Hard clip with slight rounding (harsh, aggressive)
//   - FoldBack: Wave-folding triangle fold (complex harmonics, synth-like)
//
// Includes DC blocking filter after saturation to remove offset from
// asymmetric clipping (especially Tape mode).
//
// Usage:
//   Saturator sat;
//   sat.setMode(Saturator::SaturationMode::Tube);
//   sat.setDrive(0.5f);
//   sat.setMix(1.0f);
//   float out = sat.processSample(input);
//   // or:
//   sat.processBlock(buffer, numSamples);
//==============================================================================
class Saturator
{
public:
    /// Available saturation modes.
    enum class SaturationMode
    {
        Tube,    ///< Warm tanh soft-clip
        Tape,    ///< Asymmetric soft-clip with bias
        Digital, ///< Hard clip with rounding
        FoldBack ///< Triangle wave-folding
    };

    Saturator() = default;

    //--------------------------------------------------------------------------
    /// Set the saturation mode.
    /// Recomputes driveGain so that calling setMode() after setDrive() always
    /// applies the correct gain formula for the new mode.
    void setMode(SaturationMode m)
    {
        mode = m;
        setDrive(drive); // recompute driveGain for new mode
    }

    /// Get the current saturation mode.
    SaturationMode getMode() const { return mode; }

    /// Set drive amount. 0.0 = clean, 1.0 = maximum saturation.
    /// Internally maps to gain: [1.0, 20.0] for Tube/Tape/Digital,
    /// [1.0, 5.0] for FoldBack.
    void setDrive(float d)
    {
        drive = clamp(d, 0.0f, 1.0f);

        if (mode == SaturationMode::FoldBack)
            driveGain = 1.0f + drive * 4.0f;
        else
            driveGain = 1.0f + drive * 19.0f;
    }

    /// Set dry/wet mix. 0.0 = fully dry, 1.0 = fully wet (saturated).
    void setMix(float wet) { mix = clamp(wet, 0.0f, 1.0f); }

    /// Set output gain to compensate for drive volume increase.
    /// @param gain  Linear gain multiplier (typically 0.1 to 1.0).
    void setOutputGain(float gain) { outputGain = clamp(gain, 0.0f, 2.0f); }

    //--------------------------------------------------------------------------
    /// Process a single sample through the saturator.
    /// @param input  Input sample.
    /// @return Saturated output sample.
    float processSample(float input)
    {
        float driven = input * driveGain;
        float saturated = 0.0f;

        switch (mode)
        {
        case SaturationMode::Tube:
            saturated = processTube(driven);
            break;
        case SaturationMode::Tape:
            saturated = processTape(driven);
            break;
        case SaturationMode::Digital:
            saturated = processDigital(driven);
            break;
        case SaturationMode::FoldBack:
            saturated = processFoldBack(driven);
            break;
        }

        // DC blocking filter: y[n] = x[n] - x[n-1] + R * y[n-1]
        // dcBlockR is computed in prepare() to maintain ~16 Hz cutoff at any
        // sample rate — removes DC offset from asymmetric clipping without
        // affecting audible bass content.
        float dcBlocked = saturated - dcPrevInput + dcBlockR * dcPrevOutput;
        dcPrevInput = saturated;
        dcPrevOutput = flushDenormal(dcBlocked);
        saturated = dcPrevOutput;

        // Apply output gain compensation
        saturated *= outputGain;

        // Dry/wet mix
        return input * (1.0f - mix) + saturated * mix;
    }

    //--------------------------------------------------------------------------
    /// Process a block of samples in-place.
    /// @param data  Pointer to sample buffer.
    /// @param numSamples  Number of samples to process.
    void processBlock(float* data, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            data[i] = processSample(data[i]);
    }

    //--------------------------------------------------------------------------
    /// Set the sample rate. Must be called in prepareToPlay() before processing.
    /// Recomputes the DC blocker coefficient to maintain a ~16 Hz cutoff at any
    /// sample rate (16 Hz at 44.1 kHz, same at 48 kHz, 88.2 kHz, 96 kHz, etc.).
    void prepare(double sampleRate)
    {
        dcBlockR = 1.0f - (2.0f * 3.14159265f * 16.0f / static_cast<float>(sampleRate));
        reset();
    }

    //--------------------------------------------------------------------------
    /// Reset internal state (DC blocker).
    void reset()
    {
        dcPrevInput = 0.0f;
        dcPrevOutput = 0.0f;
    }

private:
    //--------------------------------------------------------------------------
    // Saturation algorithms

    /// Tube: tanh soft-clip. Warm, symmetric, compressing.
    float processTube(float x) const { return fastTanh(x); }

    /// Tape: asymmetric soft-clip with positive bias.
    /// Adds even harmonics and gentle compression on positive peaks,
    /// harder clipping on negative peaks (like magnetic tape saturation).
    float processTape(float x) const
    {
        // Add slight positive bias for asymmetry
        float biased = x + 0.1f;

        // Asymmetric soft-clip: positive side compresses gently,
        // negative side clips harder
        if (biased >= 0.0f)
        {
            // Gentle positive compression: x / (1 + |x|)
            return biased / (1.0f + biased) - 0.05f;
        }
        else
        {
            // Harder negative clip with tanh
            return fastTanh(biased * 1.5f) - 0.05f;
        }
    }

    /// Digital: hard clip at [-1, 1] with slight cubic rounding near the edges.
    float processDigital(float x) const
    {
        if (x > 1.0f)
            return 1.0f;
        if (x < -1.0f)
            return -1.0f;

        // Slight cubic rounding near clip point for less harsh aliasing
        // y = 1.5x - 0.5x^3 (soft clip polynomial, normalized to [-1, 1])
        return 1.5f * x - 0.5f * x * x * x;
    }

    /// FoldBack: triangle wave-folding. Input folds back from +/-1 thresholds,
    /// creating complex harmonic content that changes with drive level.
    float processFoldBack(float x) const
    {
        // Iterative fold-back: reflect signal at +/-1 boundaries
        constexpr float threshold = 1.0f;
        constexpr int maxFolds = 8; // prevent infinite loop on extreme input

        float folded = x;
        for (int f = 0; f < maxFolds; ++f)
        {
            if (folded > threshold)
                folded = 2.0f * threshold - folded;
            else if (folded < -threshold)
                folded = -2.0f * threshold - folded;
            else
                break;
        }

        // Hard clamp after folding — extreme inputs can exceed [-1,1]
        // even after maxFolds iterations
        if (folded > threshold)
            folded = threshold;
        if (folded < -threshold)
            folded = -threshold;

        return folded;
    }

    //--------------------------------------------------------------------------
    SaturationMode mode = SaturationMode::Tube;
    float drive = 0.0f;
    float driveGain = 1.0f;
    float mix = 1.0f;
    float outputGain = 1.0f;

    // DC blocking filter state.
    // dcBlockR is computed in prepare() as 1 - 2π*16/sampleRate so the
    // ~16 Hz cutoff is correct at any sample rate (44.1, 48, 88.2, 96 kHz).
    float dcBlockR = 0.995f; // default safe until prepare() is called
    float dcPrevInput = 0.0f;
    float dcPrevOutput = 0.0f;
};

} // namespace xoceanus
