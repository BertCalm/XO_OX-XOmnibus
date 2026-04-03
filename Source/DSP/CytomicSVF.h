// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include "FastMath.h"

namespace xoceanus
{

//==============================================================================
// CytomicSVF — Andrew Simper's topology-preserving state-variable filter.
//
// Implements the trapezoidal integration (TPT) SVF from Cytomic's technical
// papers. Unconditionally stable at all cutoff/resonance settings. Supports
// 8 filter modes from a single processing kernel.
//
// References:
//   - Andy Simper, "Linear Trapezoidal Integrated SVF" (Cytomic, 2013)
//   - Vadim Zavalishin, "The Art of VA Filter Design" (2018), Chapter 4
//
// Usage:
//   CytomicSVF filter;
//   filter.setCoefficients(1000.0f, 0.5f, 44100.0f);
//   float out = filter.processSample(inputSample);
//==============================================================================
class CytomicSVF
{
public:
    /// Available filter modes.
    enum class Mode
    {
        LowPass,
        HighPass,
        BandPass,
        Notch,
        Peak,
        LowShelf,
        HighShelf,
        AllPass
    };

    CytomicSVF() = default;

    //--------------------------------------------------------------------------
    /// Set the filter mode. Safe to call per-sample if needed.
    void setMode(Mode m) noexcept { mode = m; }

    /// Get the current filter mode.
    Mode getMode() const noexcept { return mode; }

    //--------------------------------------------------------------------------
    /// Compute filter coefficients for the given cutoff, resonance, and sample rate.
    /// @param cutoffHz  Cutoff frequency in Hz, clamped to [20, sampleRate * 0.49].
    /// @param resonance Resonance in [0, 1]. 0 = no resonance, 1 = self-oscillation.
    /// @param sampleRate  Current sample rate in Hz.
    /// @param shelfGainDb  Gain in dB for LowShelf/HighShelf modes (ignored for others).
    void setCoefficients(float cutoffHz, float resonance, float sampleRate, float shelfGainDb = 0.0f) noexcept
    {
        if (sampleRate <= 0.0f)
            return;
        // Clamp cutoff to safe range: [20 Hz, just below Nyquist]
        float nyquistLimit = sampleRate * 0.49f;
        float fc = cutoffHz;
        if (fc < 20.0f)
            fc = 20.0f;
        if (fc > nyquistLimit)
            fc = nyquistLimit;

        // Clamp resonance to [0, 1]
        float res = resonance;
        if (res < 0.0f)
            res = 0.0f;
        if (res > 1.0f)
            res = 1.0f;

        // Prewarp cutoff frequency for trapezoidal integration
        constexpr float pi = 3.14159265358979323846f;
        // SRO: fastTan replaces std::tan (per-setter coefficient calc)
        g = fastTan(pi * fc / sampleRate);

        // Map resonance [0,1] to damping factor k.
        // k = 2 at res=0 (Butterworth), k = 0 at res=1 (self-oscillation).
        k = 2.0f - 2.0f * res;

        // Shelf gain coefficient (only used in shelf modes)
        if (mode == Mode::LowShelf || mode == Mode::HighShelf)
        {
            // SRO: dbToGain replaces std::pow (per-setter shelf calc)
            // pow(10, dB/40) = dbToGain(dB/2) since dbToGain(x) = pow(10, x/20)
            A = dbToGain(shelfGainDb * 0.5f);
        }
        else
        {
            A = 1.0f;
        }

        // Precompute common denominator terms
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    //--------------------------------------------------------------------------
    /// Fast per-sample coefficient update using fastTan approximation.
    /// Avoids std::tan overhead — suitable for modulated cutoff in audio loops.
    /// Accurate to ~0.03% for cutoff < 0.25 × sampleRate.
    void setCoefficients_fast(float cutoffHz, float resonance, float sampleRate) noexcept
    {
        if (sampleRate <= 0.0f)
            return;
        float nyquistLimit = sampleRate * 0.49f;
        float fc = cutoffHz;
        if (fc < 20.0f)
            fc = 20.0f;
        if (fc > nyquistLimit)
            fc = nyquistLimit;

        float res = resonance;
        if (res < 0.0f)
            res = 0.0f;
        if (res > 1.0f)
            res = 1.0f;

        constexpr float pi = 3.14159265358979323846f;
        g = fastTan(pi * fc / sampleRate);
        k = 2.0f - 2.0f * res;

        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    //--------------------------------------------------------------------------
    /// Process a single sample through the filter.
    /// @param input  Input sample value.
    /// @return Filtered output sample.
    float processSample(float input) noexcept
    {
        // TPT SVF tick — compute v1 (bandpass) and v2 (lowpass) simultaneously
        float v3 = input - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;

        // Update state variables with denormal protection
        ic1eq = flushDenormal(2.0f * v1 - ic1eq);
        ic2eq = flushDenormal(2.0f * v2 - ic2eq);

        // Compute output based on selected mode.
        // All modes are derived from the same v0/v1/v2 signals:
        //   v0 = input, v1 = bandpass, v2 = lowpass
        //   highpass = v0 - k*v1 - v2
        //   notch    = lowpass + highpass
        //   peak     = lowpass - highpass
        //   allpass  = lowpass + highpass - k*bandpass = 2*lowpass - notch...
        //              more precisely: v0 - 2*k*v1
        switch (mode)
        {
        case Mode::LowPass:
            return v2;

        case Mode::HighPass:
            return input - k * v1 - v2;

        case Mode::BandPass:
            return v1;

        case Mode::Notch:
            // LP + HP = input - k * v1
            return input - k * v1;

        case Mode::Peak:
            // LP - HP = 2*v2 - input + k*v1
            return 2.0f * v2 - input + k * v1;

        case Mode::AllPass:
            // input - 2 * k * v1
            return input - 2.0f * k * v1;

        case Mode::LowShelf:
        {
            // LowShelf: boost/cut below cutoff
            // output = input + (A^2 - 1) * v2
            float A2 = A * A;
            return input + (A2 - 1.0f) * v2;
        }

        case Mode::HighShelf:
        {
            // HighShelf: boost/cut above cutoff
            // output = input + (A^2 - 1) * (input - k*v1 - v2)
            float A2 = A * A;
            float hp = input - k * v1 - v2;
            return input + (A2 - 1.0f) * hp;
        }
        }

        return v2; // fallback
    }

    //--------------------------------------------------------------------------
    /// Reset all internal state (call on voice start or when bypassed).
    void reset() noexcept
    {
        ic1eq = 0.0f;
        ic2eq = 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Process a block of samples in-place.
    /// @param samples  Pointer to sample buffer.
    /// @param numSamples  Number of samples to process.
    void processBlock(float* samples, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            samples[i] = processSample(samples[i]);
    }

private:
    Mode mode = Mode::LowPass;

    // Coefficients (recomputed when cutoff/resonance/sampleRate change)
    float g = 0.0f;  // prewarped cutoff: tan(pi * fc / sr)
    float k = 2.0f;  // damping: 2 - 2*resonance
    float A = 1.0f;  // shelf gain factor
    float a1 = 0.0f; // 1 / (1 + g*(g+k))
    float a2 = 0.0f; // g * a1
    float a3 = 0.0f; // g * a2

    // State variables (trapezoidal integrator memories)
    float ic1eq = 0.0f; // integrator 1 state
    float ic2eq = 0.0f; // integrator 2 state
};

} // namespace xoceanus
