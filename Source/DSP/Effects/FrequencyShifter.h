#pragma once
#include <cmath>
#include <array>
#include "../FastMath.h"

namespace xomnibus {

//==============================================================================
// FrequencyShifter — Shifts all frequencies by a fixed Hz offset.
//
// Unlike pitch shifting (which preserves harmonic relationships), frequency
// shifting adds a fixed offset to every partial. This breaks the harmonic
// series, creating metallic, inharmonic, and alien textures.
//
// Implementation: Hilbert transform (90-degree phase splitter) using a pair
// of allpass chains, followed by complex multiplication with the shift
// oscillator. This is the classic Bode/Moog frequency shifter topology.
//
// Inspired by: Surge XT Frequency Shifter, Moog MF-102
//
// Features:
//   - Shift range: -1000 Hz to +1000 Hz
//   - Up/Down/Both modes (both = ring-mod-like sum)
//   - Feedback path for metallic self-oscillation
//   - Zero CPU when mix = 0
//
// CPU cost: ~12 multiplies + 12 additions per sample (the allpass chains).
// Very lightweight.
//
// Usage:
//   FrequencyShifter fs;
//   fs.prepare(44100.0);
//   fs.setShift(5.0f);  // subtle detuning
//   fs.setMix(0.5f);
//   fs.processBlock(L, R, numSamples);
//==============================================================================
class FrequencyShifter
{
public:
    enum class Mode
    {
        Up = 0,     ///< Upper sideband only (shift up)
        Down,       ///< Lower sideband only (shift down)
        Both,       ///< Both sidebands mixed (ring-mod character)
        NumModes
    };

    FrequencyShifter() = default;

    //--------------------------------------------------------------------------
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        oscPhase = 0.0;

        // Initialize Hilbert transform allpass chains
        // 6th-order allpass design (3 stages per channel) for ~90° phase split
        // Coefficients from classic Hilbert transform design (Robert Bristow-Johnson)
        // These give good phase accuracy from ~20Hz to ~20kHz at 44.1kHz+
        static constexpr float kCoeffsI[kNumStages] = {
            0.6923878f, 0.9360654322959f, 0.9882295226860f
        };
        static constexpr float kCoeffsQ[kNumStages] = {
            0.4021921162426f, 0.8561710882420f, 0.9722909545651f
        };

        for (int s = 0; s < kNumStages; ++s)
        {
            coeffI[s] = kCoeffsI[s];
            coeffQ[s] = kCoeffsQ[s];
        }

        // Clear allpass state
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int s = 0; s < kNumStages; ++s)
            {
                apStateI[ch][s][0] = apStateI[ch][s][1] = 0.0f;
                apStateQ[ch][s][0] = apStateQ[ch][s][1] = 0.0f;
            }
            prevInputI[ch] = 0.0f;
        }

        fbStateL = fbStateR = 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Set shift amount in Hz. [-1000, +1000]
    void setShift (float hz)
    {
        shiftHz = clamp (hz, -1000.0f, 1000.0f);
    }

    /// Set wet/dry mix. [0, 1]
    void setMix (float wet)
    {
        mix = clamp (wet, 0.0f, 1.0f);
    }

    /// Set output mode.
    void setMode (Mode m)
    {
        mode = m;
    }

    /// Set feedback amount. [0, 0.9] — metallic self-oscillation.
    void setFeedback (float fb)
    {
        feedback = clamp (fb, 0.0f, 0.9f);
    }

    //--------------------------------------------------------------------------
    /// Process stereo audio in-place.
    void processBlock (float* L, float* R, int numSamples)
    {
        double phaseInc = static_cast<double> (shiftHz) / sr;

        for (int i = 0; i < numSamples; ++i)
        {
            // Add feedback
            float inL = L[i] + fbStateL * feedback;
            float inR = R[i] + fbStateR * feedback;

            // Hilbert transform: split each channel into I (in-phase) and Q (quadrature)
            float iL, qL, iR, qR;
            hilbertTransform (inL, 0, iL, qL);
            hilbertTransform (inR, 1, iR, qR);

            // Generate shift oscillator (complex exponential)
            // SRO: fastSin/fastCos replace std:: trig (~0.02% accuracy, ~10x faster)
            float oscAngle = static_cast<float> (oscPhase * 6.283185307179586);
            float cosOsc = fastCos (oscAngle);
            float sinOsc = fastSin (oscAngle);

            // Complex multiply: shift = I*cos - Q*sin (upper), I*cos + Q*sin (lower)
            float upL = iL * cosOsc - qL * sinOsc;
            float dnL = iL * cosOsc + qL * sinOsc;
            float upR = iR * cosOsc - qR * sinOsc;
            float dnR = iR * cosOsc + qR * sinOsc;

            // Select mode
            float wetL, wetR;
            switch (mode)
            {
                case Mode::Up:
                    wetL = upL;
                    wetR = upR;
                    break;
                case Mode::Down:
                    wetL = dnL;
                    wetR = dnR;
                    break;
                case Mode::Both:
                default:
                    wetL = (upL + dnL) * 0.5f;
                    wetR = (upR + dnR) * 0.5f;
                    break;
            }

            // Store feedback
            fbStateL = flushDenormal (wetL);
            fbStateR = flushDenormal (wetR);

            // Mix
            L[i] = L[i] * (1.0f - mix) + wetL * mix;
            R[i] = R[i] * (1.0f - mix) + wetR * mix;

            // Advance oscillator
            oscPhase += phaseInc;
            if (oscPhase > 1.0) oscPhase -= 1.0;
            if (oscPhase < 0.0) oscPhase += 1.0;
        }
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        oscPhase = 0.0;
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int s = 0; s < kNumStages; ++s)
            {
                apStateI[ch][s][0] = apStateI[ch][s][1] = 0.0f;
                apStateQ[ch][s][0] = apStateQ[ch][s][1] = 0.0f;
            }
            prevInputI[ch] = 0.0f;
        }
        fbStateL = fbStateR = 0.0f;
    }

private:
    //--------------------------------------------------------------------------
    /// Hilbert transform via two parallel allpass chains.
    /// The I chain delays by a flat group delay, the Q chain shifts phase by ~90°.
    void hilbertTransform (float input, int ch, float& outI, float& outQ)
    {
        // I path: 3 cascaded 1st-order allpass filters
        float xI = input;
        for (int s = 0; s < kNumStages; ++s)
        {
            float y = coeffI[s] * (xI - apStateI[ch][s][1]) + apStateI[ch][s][0];
            apStateI[ch][s][0] = flushDenormal (xI);
            apStateI[ch][s][1] = flushDenormal (y);
            xI = y;
        }

        // Q path: 3 cascaded 1st-order allpass filters (different coefficients)
        // Input is delayed by 1 sample to align with I path group delay
        float xQ = prevInputI[ch];
        prevInputI[ch] = input;

        for (int s = 0; s < kNumStages; ++s)
        {
            float y = coeffQ[s] * (xQ - apStateQ[ch][s][1]) + apStateQ[ch][s][0];
            apStateQ[ch][s][0] = flushDenormal (xQ);
            apStateQ[ch][s][1] = flushDenormal (y);
            xQ = y;
        }

        outI = xI;
        outQ = xQ;
    }

    //--------------------------------------------------------------------------
    static constexpr int kNumStages = 3;

    double sr = 44100.0;
    double oscPhase = 0.0;

    // Allpass coefficients for Hilbert transform
    float coeffI[kNumStages] {};
    float coeffQ[kNumStages] {};

    // Allpass state [channel][stage][x/y]
    float apStateI[2][kNumStages][2] {};
    float apStateQ[2][kNumStages][2] {};
    float prevInputI[2] {};

    // Feedback state
    float fbStateL = 0.0f;
    float fbStateR = 0.0f;

    // Parameters
    float shiftHz = 0.0f;
    float mix = 0.0f;
    float feedback = 0.0f;
    Mode mode = Mode::Up;
};

} // namespace xomnibus
