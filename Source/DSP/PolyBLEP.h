#pragma once
#include <cmath>
#include "FastMath.h"

namespace xolokun {

//==============================================================================
// PolyBLEP — Band-limited oscillator with polynomial BLEP anti-aliasing.
//
// Generates classic analog waveforms (Saw, Square, Triangle, Pulse) with
// minimal aliasing using polynomial band-limited step (polyBLEP) correction
// on discontinuities. The triangle is produced by integrating the square wave
// with a leaky integrator, then applying polyBLAMP correction.
//
// Usage:
//   PolyBLEP osc;
//   osc.setFrequency(440.0f, 44100.0f);
//   osc.setWaveform(PolyBLEP::Waveform::Saw);
//   float sample = osc.processSample();
//==============================================================================
class PolyBLEP
{
public:
    /// Available waveform shapes.
    enum class Waveform
    {
        Sine,
        Saw,
        Square,
        Triangle,
        Pulse
    };

    PolyBLEP() = default;

    //--------------------------------------------------------------------------
    /// Set the oscillator frequency and sample rate.
    /// @param freqHz    Desired frequency in Hz.
    /// @param sampleRate Current sample rate in Hz.
    void setFrequency (float freqHz, float sampleRate) noexcept
    {
        // Clamp frequency to [0, Nyquist/2] for stable anti-aliasing
        float maxFreq = sampleRate * 0.5f;
        if (freqHz < 0.0f)    freqHz = 0.0f;
        if (freqHz > maxFreq) freqHz = maxFreq;

        phaseIncrement = freqHz / sampleRate;
    }

    //--------------------------------------------------------------------------
    /// Set the waveform shape.
    void setWaveform (Waveform w) noexcept { waveform = w; }

    //--------------------------------------------------------------------------
    /// Set pulse width for Pulse waveform. Clamped to [0.01, 0.99].
    /// @param pw Pulse width, 0.5 = square wave.
    void setPulseWidth (float pw) noexcept
    {
        if (pw < 0.01f) pw = 0.01f;
        if (pw > 0.99f) pw = 0.99f;
        pulseWidth = pw;
    }

    //--------------------------------------------------------------------------
    /// Generate and return the next output sample.
    float processSample() noexcept
    {
        float out = 0.0f;
        float dt = phaseIncrement;

        switch (waveform)
        {
            case Waveform::Sine:
            {
                constexpr float twoPi = 6.28318530717958647692f;
                out = fastSin (phase * twoPi);
                break;
            }

            case Waveform::Saw:
            {
                // Naive saw: phase [0,1) mapped to [-1,1)
                out = 2.0f * phase - 1.0f;
                // Apply polyBLEP at the discontinuity (phase wraps at 1.0)
                out -= polyBLEP (phase, dt);
                break;
            }

            case Waveform::Square:
            {
                // Naive square: +1 for phase < 0.5, -1 for phase >= 0.5
                out = (phase < 0.5f) ? 1.0f : -1.0f;
                // polyBLEP at rising edge (phase = 0)
                out += polyBLEP (phase, dt);
                // polyBLEP at falling edge (phase = 0.5)
                out -= polyBLEP (wrapPhase (phase + 0.5f), dt);
                break;
            }

            case Waveform::Triangle:
            {
                // Derived from integrated square wave with polyBLAMP correction.
                // First compute raw square:
                float sq = (phase < 0.5f) ? 1.0f : -1.0f;
                sq += polyBLEP (phase, dt);
                sq -= polyBLEP (wrapPhase (phase + 0.5f), dt);

                // Leaky integration of square to produce triangle
                // Scale by 4*dt to normalize amplitude across frequencies
                if (dt > 0.0f)
                    triIntegrator = flushDenormal (triIntegrator + sq * 4.0f * dt);

                // Apply DC-blocking leaky integrator (very gentle leak)
                triIntegrator *= 0.999f;

                out = triIntegrator;
                break;
            }

            case Waveform::Pulse:
            {
                // Variable pulse width: +1 for phase < pw, -1 for phase >= pw
                out = (phase < pulseWidth) ? 1.0f : -1.0f;
                // polyBLEP at rising edge (phase = 0)
                out += polyBLEP (phase, dt);
                // polyBLEP at falling edge (phase = pulseWidth)
                out -= polyBLEP (wrapPhase (phase - pulseWidth + 1.0f), dt);
                break;
            }
        }

        // Advance phase (bounded wrap — prevents runaway loop on corrupted increment)
        phase += phaseIncrement;
        phase = std::fmod (phase, 1.0f);
        if (phase < 0.0f) phase += 1.0f;

        return out;
    }

    //--------------------------------------------------------------------------
    /// Reset the oscillator phase and internal state.
    void reset() noexcept
    {
        phase = 0.0f;
        triIntegrator = 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Set the oscillator phase directly.
    /// @param p Phase value in [0, 1).
    void setPhase (float p) noexcept
    {
        phase = wrapPhase (p);
    }

    /// Get the current phase.
    float getPhase() const noexcept { return phase; }

    //--------------------------------------------------------------------------
    /// Process a block of samples into a buffer.
    /// @param output  Pointer to output buffer.
    /// @param numSamples  Number of samples to generate.
    void processBlock (float* output, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample();
    }

private:
    //--------------------------------------------------------------------------
    /// 2nd-order polynomial BLEP correction.
    /// Smooths the discontinuity at a transition point in the waveform.
    /// @param t  Phase position relative to the discontinuity.
    /// @param dt Phase increment per sample (frequency / sampleRate).
    /// @return   Correction value to add/subtract from naive waveform.
    static float polyBLEP (float t, float dt) noexcept
    {
        if (dt <= 0.0f) return 0.0f;

        // Transition is within one sample AFTER the discontinuity
        if (t < dt)
        {
            float x = t / dt;        // x in [0, 1)
            return x + x - x * x - 1.0f;
        }
        // Transition is within one sample BEFORE the discontinuity
        if (t > 1.0f - dt)
        {
            float x = (t - 1.0f) / dt;  // x in (-1, 0]
            return x * x + x + x + 1.0f;
        }
        return 0.0f;
    }

    /// Wrap a phase value into [0, 1).
    static float wrapPhase (float p) noexcept
    {
        while (p >= 1.0f) p -= 1.0f;
        while (p < 0.0f)  p += 1.0f;
        return p;
    }

    Waveform waveform = Waveform::Sine;
    float phase = 0.0f;            // Current phase in [0, 1)
    float phaseIncrement = 0.0f;   // freq / sampleRate
    float pulseWidth = 0.5f;       // Pulse width for Pulse mode
    float triIntegrator = 0.0f;    // Leaky integrator state for triangle wave
};

} // namespace xolokun
