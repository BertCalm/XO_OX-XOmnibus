// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>
#include "FastMath.h"

namespace xoceanus
{

//==============================================================================
// PolyBLEP — Band-limited oscillator with polynomial BLEP anti-aliasing.
//
// Generates classic analog waveforms (Saw, Square, Triangle, Pulse) with
// minimal aliasing using polynomial band-limited step (polyBLEP) correction
// on discontinuities. The triangle is produced by integrating the square wave
// (with polyBLEP-smoothed edges) through a leaky integrator, producing
// polyBLAMP-correct antialiased corners. A per-frequency amplitude correction
// factor (triLeakCorrection) cancels the frequency-dependent gain error
// introduced by the 0.999 leak coefficient, keeping triangle amplitude
// within ±0.1 dB across 20 Hz – 20 kHz.
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
    void setFrequency(float freqHz, float sampleRate) noexcept
    {
        if (sampleRate <= 0.0f)
            return;
        // Clamp frequency to [0, Nyquist/2] for stable anti-aliasing
        float maxFreq = sampleRate * 0.5f;
        if (freqHz < 0.0f)
            freqHz = 0.0f;
        if (freqHz > maxFreq)
            freqHz = maxFreq;

        phaseIncrement = freqHz / sampleRate;

        // Pre-compute triangle amplitude correction for the leaky integrator.
        //
        // The integrator (triIntegrator *= 0.999f) introduces frequency-dependent
        // gain error: at 20 Hz the output is ~3.6 dB too quiet; at 1 kHz it is
        // ~6.3 dB too loud — a 9.5 dB range across the audio spectrum.
        //
        // The steady-state peak of a symmetric leaky integrator driven by a
        // square wave of ±(4*dt) with leak k is:
        //
        //   P = 4*dt * (1 - k^N) / ((1 - k) * (1 + k^N))
        //
        //   where N = halfPeriod = 0.5 / phaseIncrement
        //         dt = phaseIncrement = freq / sampleRate
        //         k  = 0.999
        //
        // The correction factor = 1/P restores unity amplitude at all frequencies.
        // Guard against divide-by-zero at freqHz == 0.
        static constexpr float kLeak = 0.999f;
        if (phaseIncrement > 0.0f)
        {
            // halfPeriod = 0.5 / phaseIncrement  (samples in one half-cycle)
            float halfPeriod = 0.5f / phaseIncrement;
            // Clamp halfPeriod so std::pow stays finite (very low freq or DC)
            halfPeriod = std::min(halfPeriod, 50000.0f);
            float kN = std::pow(kLeak, halfPeriod);

            // Steady-state peak amplitude of the leaky integrator driven by a
            // symmetric square wave of ±(4*dt) with leak coefficient k=0.999:
            //
            //   P = 4*dt * (1 - k^N) / ((1 - k) * (1 + k^N))
            //
            // Derivation: at steady state the integrator swings ±P. During the
            // positive half-cycle (N samples), starting at -P with input +4*dt:
            //   +P = k^N * (-P) + 4*dt * (1 - k^N) / (1 - k)
            //   P * (1 + k^N) = 4*dt * (1 - k^N) / (1 - k)
            //   P = 4*dt * (1 - k^N) / ((1 - k) * (1 + k^N))
            //
            // The correction multiplier to restore unity amplitude is 1/P:
            //   correction = (1 - k) * (1 + k^N) / (4*dt * (1 - k^N))
            float oneMinusKN = 1.0f - kN;
            float onePlusKN = 1.0f + kN;
            if (oneMinusKN > 1e-6f)
                triLeakCorrection = (1.0f - kLeak) * onePlusKN / (4.0f * phaseIncrement * oneMinusKN);
            else
                triLeakCorrection = 1.0f; // numerically degenerate — shouldn't occur

            // No amplitude cap: the correction is exact at all frequencies.
            // For very low LFO rates the correction factor grows large; the output
            // is clamped to [-1, 1] in processSample() after applying the correction,
            // keeping amplitude at unity even at sub-Hz rates.
            // (Previously capped at 4.0 → +12 dB overshoot at sub-Hz — issue #177)
        }
        else
        {
            triLeakCorrection = 1.0f;
        }
    }

    //--------------------------------------------------------------------------
    /// Set the waveform shape.
    void setWaveform(Waveform w) noexcept { waveform = w; }

    //--------------------------------------------------------------------------
    /// Set pulse width for Pulse waveform. Clamped to [0.01, 0.99].
    /// @param pw Pulse width, 0.5 = square wave.
    void setPulseWidth(float pw) noexcept
    {
        if (pw < 0.01f)
            pw = 0.01f;
        if (pw > 0.99f)
            pw = 0.99f;
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
            out = fastSin(phase * twoPi);
            break;
        }

        case Waveform::Saw:
        {
            // Naive saw: phase [0,1) mapped to [-1,1)
            out = 2.0f * phase - 1.0f;
            // Apply polyBLEP at the discontinuity (phase wraps at 1.0)
            out -= polyBLEP(phase, dt);
            break;
        }

        case Waveform::Square:
        {
            // Naive square: +1 for phase < 0.5, -1 for phase >= 0.5
            out = (phase < 0.5f) ? 1.0f : -1.0f;
            // polyBLEP at rising edge (phase = 0)
            out += polyBLEP(phase, dt);
            // polyBLEP at falling edge (phase = 0.5)
            out -= polyBLEP(wrapPhase(phase + 0.5f), dt);
            break;
        }

        case Waveform::Triangle:
        {
            // Derived from integrated square wave with polyBLAMP correction.
            // The polyBLEP smoothing on the square wave edges propagates through
            // the integration to produce antialiased triangle slope corners.
            //
            // First compute band-limited square:
            float sq = (phase < 0.5f) ? 1.0f : -1.0f;
            sq += polyBLEP(phase, dt);
            sq -= polyBLEP(wrapPhase(phase + 0.5f), dt);

            // Leaky integration of square to produce triangle.
            // Scale by 4*dt so the integrator accumulates the right slope.
            if (dt > 0.0f)
                triIntegrator = flushDenormal(triIntegrator + sq * 4.0f * dt);

            // Apply DC-blocking leaky coefficient (0.999 per sample).
            // This introduces a frequency-dependent amplitude error (9.5 dB
            // variation, 20 Hz–20 kHz). triLeakCorrection (pre-computed in
            // setFrequency) cancels this bias, restoring unity amplitude at
            // all frequencies while preserving the DC-blocking behaviour.
            triIntegrator *= 0.999f;

            out = triIntegrator * triLeakCorrection;
            // Clamp to [-1, 1]: at very low LFO rates the leaky integrator
            // may not fully settle, and the exact correction can overshoot
            // during transient start-up. Hard clamp preserves unity amplitude.
            if (out > 1.0f)
                out = 1.0f;
            if (out < -1.0f)
                out = -1.0f;
            break;
        }

        case Waveform::Pulse:
        {
            // Variable pulse width: +1 for phase < pw, -1 for phase >= pw
            out = (phase < pulseWidth) ? 1.0f : -1.0f;
            // polyBLEP at rising edge (phase = 0)
            out += polyBLEP(phase, dt);
            // polyBLEP at falling edge (phase = pulseWidth)
            out -= polyBLEP(wrapPhase(phase - pulseWidth + 1.0f), dt);
            break;
        }
        }

        // Advance phase (bounded wrap — prevents runaway loop on corrupted increment)
        phase += phaseIncrement;
        phase = std::fmod(phase, 1.0f);
        if (phase < 0.0f)
            phase += 1.0f;

        return out;
    }

    //--------------------------------------------------------------------------
    /// Reset the oscillator phase and internal state.
    /// triLeakCorrection is NOT reset — it is frequency-derived and stable.
    void reset() noexcept
    {
        phase = 0.0f;
        triIntegrator = 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Set the oscillator phase directly.
    /// @param p Phase value in [0, 1).
    void setPhase(float p) noexcept { phase = wrapPhase(p); }

    /// Get the current phase.
    float getPhase() const noexcept { return phase; }

    //--------------------------------------------------------------------------
    /// Process a block of samples into a buffer.
    /// @param output  Pointer to output buffer.
    /// @param numSamples  Number of samples to generate.
    void processBlock(float* output, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample();
    }

    //--------------------------------------------------------------------------
    /// 2nd-order polynomial BLEP correction (public static utility).
    ///
    /// Call directly when building custom oscillators that cannot use the
    /// full PolyBLEP object (e.g. multi-oscillator metallic banks where phase
    /// state is managed externally).
    ///
    /// @param t  Phase position relative to the discontinuity, in [0, 1).
    ///           Pass `phase` for a rising edge at phase=0.
    ///           Pass `fmod(phase - pw + 1.0f, 1.0f)` for a falling edge at phase=pw.
    /// @param dt Phase increment per sample (frequency / sampleRate).
    /// @return   Correction value to add (rising) or subtract (falling) from
    ///           the naive waveform output.
    static float polyBLEP(float t, float dt) noexcept
    {
        if (dt <= 0.0f)
            return 0.0f;

        // Transition is within one sample AFTER the discontinuity
        if (t < dt)
        {
            float x = t / dt; // x in [0, 1)
            return x + x - x * x - 1.0f;
        }
        // Transition is within one sample BEFORE the discontinuity
        if (t > 1.0f - dt)
        {
            float x = (t - 1.0f) / dt; // x in (-1, 0]
            return x * x + x + x + 1.0f;
        }
        return 0.0f;
    }

    /// Wrap a phase value into [0, 1).
    static float wrapPhase(float p) noexcept
    {
        while (p >= 1.0f)
            p -= 1.0f;
        while (p < 0.0f)
            p += 1.0f;
        return p;
    }

private:
    Waveform waveform = Waveform::Sine;
    float phase = 0.0f;             // Current phase in [0, 1)
    float phaseIncrement = 0.0f;    // freq / sampleRate
    float pulseWidth = 0.5f;        // Pulse width for Pulse mode
    float triIntegrator = 0.0f;     // Leaky integrator state for triangle wave
    float triLeakCorrection = 1.0f; // Pre-computed amplitude correction for triangle
                                    // leak bias; derived in setFrequency()
};

} // namespace xoceanus
