// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "OperaConstants.h"

//==============================================================================
// OperaBreathEngine.h — opera::OperaBreathEngine
//
// Breath/noise engine for the XOpera (OPERA) synthesizer. Generates spectrally
// shaped noise with formant-sympathetic bandpass peaks and Kuramoto
// synchronization capture (Ciani's suggestion).
//
// Signal flow:
//   LCG white noise
//     -> one-pole lowpass (effort-shaped spectral slope)
//     -> 3 parallel bandpass peaks at formant centers (vowel-quality breath)
//     -> synchronization capture (ring-mod with fundamental, blend = r(t)^2)
//     -> stereo output scaled by breath level
//
// All code inline. No allocation on audio thread. No framework dependencies.
//==============================================================================

#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace opera
{

//==============================================================================
/// One formant bandpass filter (2nd-order resonant).
/// Implemented as a biquad bandpass to create vowel-quality peaks in the noise.
//==============================================================================
struct FormantBandpass
{
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    /// Recalculate coefficients for a bandpass at centerHz with given bandwidth.
    void setParams(float centerHz, float bandwidthHz, float sampleRate) noexcept
    {
        if (centerHz <= 0.0f || sampleRate <= 0.0f || bandwidthHz <= 0.0f)
            return;

        float w0 = kTwoPi * centerHz / sampleRate;
        // Clamp w0 to prevent instability near Nyquist
        if (w0 > 3.1f)
            w0 = 3.1f;

        float sinW0 = std::sin(w0);
        float cosW0 = std::cos(w0);

        // Q from bandwidth: Q = fc / bw
        float Q = centerHz / std::max(bandwidthHz, 1.0f);
        Q = clamp(Q, 0.5f, 50.0f);

        float alpha = sinW0 / (2.0f * Q);

        // Bandpass (constant-0 dB peak gain):
        float a0Inv = 1.0f / (1.0f + alpha);
        b0 = alpha * a0Inv;
        b1 = 0.0f;
        b2 = -alpha * a0Inv;
        a1 = -2.0f * cosW0 * a0Inv;
        a2 = (1.0f - alpha) * a0Inv;
    }

    float processSample(float in) noexcept
    {
        // Transposed Direct Form II — numerically stable for narrow bandpass
        float y = b0 * in + z1;
        z1 = b1 * in - a1 * y + z2;
        z2 = b2 * in - a2 * y;
        z1 = flushDenormal(z1);
        z2 = flushDenormal(z2);
        return flushDenormal(y);
    }

    void reset() noexcept
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }
};

//==============================================================================
/// OperaBreathEngine
///
/// Generates formant-sympathetic breath noise with synchronization capture.
///
/// Usage:
///   OperaBreathEngine breath;
///   breath.prepare (sampleRate);
///
///   // Per sample:
///   float formants[3] = { f1Hz, f2Hz, f3Hz };
///   breath.processSample (outL, outR, effort, breathLevel,
///                         fundamentalHz, orderParam, formants, 3);
//==============================================================================
class OperaBreathEngine
{
public:
    //==========================================================================
    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        if (sr <= 0.0f)
            sr = 48000.0f;

        invSr = 1.0f / sr;

        // Reset all filter states
        reset();
    }

    //==========================================================================
    void reset() noexcept
    {
        noiseState = 48271u;
        effortFilterState = 0.0f;
        capturePhase = 0.0f;

        for (int i = 0; i < kMaxFormants; ++i)
        {
            formantBP[i].reset();
            prevFormantFreqs[i] = 0.0f;
        }
    }

    //==========================================================================
    /// Process one sample of breath, stereo output.
    ///
    /// @param outL, outR       Output samples (additive: adds breath to existing signal)
    /// @param effort           0.0 (whisper) to 1.0 (belt) — spectral shaping
    /// @param breathLevel      opera_breath parameter: 0 = silent, 1 = full
    /// @param fundamental      Fundamental frequency in Hz (for sync capture)
    /// @param orderParam       Kuramoto order parameter r(t), 0.0 to 1.0
    /// @param formantFreqs     Array of formant center frequencies (Hz)
    /// @param numFormants      Number of formant peaks to track (max 3 used)
    //==========================================================================
    void processSample(float& outL, float& outR, float effort, float breathLevel, float fundamental, float orderParam,
                       const float* formantFreqs, int numFormants) noexcept
    {
        if (breathLevel <= 0.0001f)
            return;

        // ------------------------------------------------------------------
        // 1. LCG white noise — Knuth TAOCP, deterministic, no allocation
        // ------------------------------------------------------------------
        float noise = nextNoiseSample();

        // ------------------------------------------------------------------
        // 2. Spectral shaping by effort — one-pole lowpass
        //    Effort=0 (whisper): 200 Hz cutoff (dark, breathy)
        //    Effort=1 (belt):    12200 Hz cutoff (bright, sibilant)
        //    Quadratic mapping compresses whisper territory
        // ------------------------------------------------------------------
        float effortClamped = clamp(effort, 0.0f, 1.0f);
        float breathCutoff = 200.0f + effortClamped * effortClamped * 12000.0f;

        // matched-Z one-pole coefficient: exp(-2*pi*fc/sr)
        float breathCoeff = std::exp(-kTwoPi * breathCutoff * invSr);

        effortFilterState = noise + (effortFilterState - noise) * breathCoeff;
        effortFilterState = flushDenormal(effortFilterState);

        float shapedBreath = effortFilterState;

        // ------------------------------------------------------------------
        // 3. Effort scales amplitude inversely at high effort
        //    Belt is more focused/dry, whisper is more breathy
        // ------------------------------------------------------------------
        float breathAmplitude = breathLevel * (1.0f - effortClamped * 0.3f);

        // ------------------------------------------------------------------
        // 4. Formant-sympathetic bandpass peaks (vowel quality in the noise)
        //    Up to 3 bandpass filters at F1, F2, F3 center frequencies
        // ------------------------------------------------------------------
        int numBP = std::min(numFormants, kMaxFormants);
        float formantSum = 0.0f;

        for (int i = 0; i < numBP; ++i)
        {
            float fc = formantFreqs[i];
            if (fc <= 0.0f || fc >= sr * 0.499f)
                continue;

            // Update bandpass coefficients only when formant frequency changes
            // significantly (avoids per-sample trig recalculation)
            if (std::fabs(fc - prevFormantFreqs[i]) > 1.0f)
            {
                // Bandwidth: wider for lower formants, narrower for higher
                // Typical vocal bandwidths: F1~80Hz, F2~90Hz, F3~120Hz
                float bw = kFormantBandwidths[i];
                formantBP[i].setParams(fc, bw, sr);
                prevFormantFreqs[i] = fc;
            }

            float bpOut = formantBP[i].processSample(shapedBreath);
            // Decreasing gain per formant: F1 strongest, F2 moderate, F3 subtle
            formantSum += bpOut * kFormantGains[i];
        }

        // Blend: shaped broadband + formant peaks
        // The formant peaks ride on top of the spectrally shaped noise floor
        float breathSignal = shapedBreath * 0.4f + formantSum * 0.6f;

        // ------------------------------------------------------------------
        // 5. Synchronization capture (Ciani's suggestion)
        //    As Kuramoto order parameter r(t) increases, noise becomes pitched.
        //    Ring-modulate noise with sine at the fundamental frequency.
        //    Blend amount = r(t)^2 (quadratic ramp for perceptual smoothness).
        //    At r=0: pure noise. At r=1: fully pitched noise.
        // ------------------------------------------------------------------
        float r = clamp(orderParam, 0.0f, 1.0f);
        float captureAmount = r * r; // quadratic: gentle onset, strong at sync

        if (captureAmount > 0.0001f && fundamental > 0.0f)
        {
            // Advance capture oscillator phase
            float phaseInc = kTwoPi * fundamental * invSr;
            capturePhase += phaseInc;

            // Wrap phase to avoid float precision loss over time
            if (capturePhase > kTwoPi)
                capturePhase -= kTwoPi * static_cast<float>(static_cast<int>(capturePhase * kInvTwoPi));

            float captureSine = fastSin(capturePhase);

            // Crossfade: (1 - capture)*noise + capture*(noise * sine)
            // The ring-modulated component creates pitched noise
            breathSignal = breathSignal * (1.0f - captureAmount) + breathSignal * captureSine * captureAmount;
        }

        // ------------------------------------------------------------------
        // 6. Apply amplitude and output to stereo
        //    Slight stereo decorrelation: one channel gets a sample-delayed noise
        // ------------------------------------------------------------------
        float finalBreath = breathSignal * breathAmplitude;

        // FIX F6: symmetric stereo decorrelation — both channels get opposite-sign noise
        // grains so L and R have equal RMS. Previous code added stereoNoise only to R,
        // creating a hard-coded ~0.05× level imbalance at high breath settings.
        float stereoNoise = nextNoiseSample() * 0.05f * breathAmplitude;

        outL += finalBreath - stereoNoise;
        outR += finalBreath + stereoNoise;
    }

private:
    //==========================================================================
    static constexpr float kInvTwoPi = 1.0f / kTwoPi;
    static constexpr int kMaxFormants = 3;

    // Typical vocal formant bandwidths (Hz) for F1, F2, F3
    // Based on Peterson & Barney (1952) / Fant (1960) averages
    static constexpr float kFormantBandwidths[kMaxFormants] = {80.0f, 90.0f, 120.0f};

    // Per-formant amplitude gains: F1 strongest, F3 most subtle
    static constexpr float kFormantGains[kMaxFormants] = {1.0f, 0.7f, 0.4f};

    //==========================================================================
    float sr = 0.0f;  // Sentinel: must be set by prepare() before use (guarded with fallback)
    float invSr = 0.0f;  // Sentinel: computed from sr in prepare()

    // LCG noise state (Knuth TAOCP, same as StandardLFO S&H)
    uint32_t noiseState = 48271u;

    // Effort-shaping one-pole lowpass state
    float effortFilterState = 0.0f;

    // Synchronization capture oscillator phase (radians)
    float capturePhase = 0.0f;

    // Formant bandpass filters
    FormantBandpass formantBP[kMaxFormants];
    float prevFormantFreqs[kMaxFormants] = {};

    //==========================================================================
    /// LCG white noise: deterministic, fast, no allocation.
    /// Returns value in [-1.0, +1.0).
    float nextNoiseSample() noexcept
    {
        noiseState = noiseState * 1664525u + 1013904223u;
        return static_cast<float>(noiseState & 0xFFFF) / 32768.0f - 1.0f;
    }
};

} // namespace opera
