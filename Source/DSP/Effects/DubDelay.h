#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include "../FastMath.h"

namespace xomnibus {

//==============================================================================
// DubDelay — Stereo ping-pong delay with HP-filtered feedback path.
//
// Features:
//   - Up to 2000ms delay time with smooth interpolation
//   - Ping-pong stereo mode (left-right bounce)
//   - Tempo-sync with BPM and beat division
//   - High-pass filter in feedback path (~80Hz) to prevent mud buildup
//   - Feedback hard-limited to 0.95 — no runaway possible
//   - Denormal protection on all buffer reads and state variables
//
// Usage:
//   DubDelay delay;
//   delay.prepare(44100.0, 512);
//   delay.setDelayTime(375.0f);
//   delay.setFeedback(0.5f);
//   delay.setMix(0.3f);
//   delay.setPingPong(true);
//   delay.processBlock(inL, inR, outL, outR, numSamples);
//==============================================================================
class DubDelay
{
public:
    DubDelay() = default;

    //--------------------------------------------------------------------------
    /// Prepare the delay for playback. Allocates buffers for the given sample rate.
    /// @param sampleRate  Current sample rate in Hz.
    /// @param maxBlockSize  Maximum expected block size (unused, reserved).
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        // Allocate circular buffers for max 2000ms + margin
        int maxDelaySamples = static_cast<int> (sr * 2.1) + 1;
        bufferL.assign (static_cast<size_t> (maxDelaySamples), 0.0f);
        bufferR.assign (static_cast<size_t> (maxDelaySamples), 0.0f);
        bufferSize = maxDelaySamples;
        writePos = 0;

        // Compute HP filter coefficient for ~80Hz (1st-order Butterworth)
        updateHPCoefficient();

        // Clear state
        hpStateL = 0.0f;
        hpStateR = 0.0f;
        hpPrevInputL = 0.0f;
        hpPrevInputR = 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Set the delay time in milliseconds. Clamped to [1, 2000]ms.
    void setDelayTime (float ms)
    {
        delayTimeMs = clamp (ms, 1.0f, 2000.0f);
        tempoSyncEnabled = false;
    }

    /// Set feedback amount. Hard-limited to [0.0, 0.95] to prevent runaway.
    void setFeedback (float fb)
    {
        feedback = clamp (fb, 0.0f, 0.95f);
    }

    /// Set wet/dry mix. 0.0 = fully dry, 1.0 = fully wet.
    void setMix (float wet)
    {
        mix = clamp (wet, 0.0f, 1.0f);
    }

    /// Enable or disable ping-pong stereo bounce.
    void setPingPong (bool enabled)
    {
        pingPong = enabled;
    }

    /// Enable tempo-synced delay time. Overrides manual delay time.
    /// @param enabled  Whether tempo sync is active.
    /// @param bpm  Beats per minute (e.g. 120.0).
    /// @param division  Beat division (1.0 = quarter, 0.5 = eighth, 0.25 = sixteenth, etc.)
    void setTempoSync (bool enabled, double bpm, float division)
    {
        tempoSyncEnabled = enabled;
        if (enabled && bpm > 0.0)
        {
            // One beat = 60000 / bpm milliseconds
            double beatMs = 60000.0 / bpm;
            delayTimeMs = clamp (static_cast<float> (beatMs * static_cast<double> (division)),
                                 1.0f, 2000.0f);
        }
    }

    //--------------------------------------------------------------------------
    /// Process a block of stereo audio through the delay.
    /// Input and output buffers may alias (in-place processing is safe).
    /// @param leftIn   Input left channel buffer.
    /// @param rightIn  Input right channel buffer.
    /// @param leftOut  Output left channel buffer.
    /// @param rightOut Output right channel buffer.
    /// @param numSamples  Number of samples to process.
    void processBlock (float* leftIn, float* rightIn,
                       float* leftOut, float* rightOut, int numSamples)
    {
        // Guard: if prepare() hasn't been called, pass audio through unchanged
        if (bufferSize <= 0)
        {
            if (leftIn != leftOut)
                std::copy(leftIn, leftIn + numSamples, leftOut);
            if (rightIn != rightOut)
                std::copy(rightIn, rightIn + numSamples, rightOut);
            return;
        }

        float delaySamplesF = static_cast<float> (delayTimeMs * 0.001 * sr);
        int delaySamples = static_cast<int> (delaySamplesF);
        float frac = delaySamplesF - static_cast<float> (delaySamples);

        // Clamp to valid range
        if (delaySamples < 1) delaySamples = 1;
        if (delaySamples >= bufferSize - 1) delaySamples = bufferSize - 2;

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = leftIn[i];
            float inR = rightIn[i];

            // Read from circular buffer with linear interpolation
            int readPos0 = (writePos - delaySamples + bufferSize) % bufferSize;
            int readPos1 = (readPos0 - 1 + bufferSize) % bufferSize;

            float delL = flushDenormal (lerp (bufferL[static_cast<size_t> (readPos0)],
                                              bufferL[static_cast<size_t> (readPos1)], frac));
            float delR = flushDenormal (lerp (bufferR[static_cast<size_t> (readPos0)],
                                              bufferR[static_cast<size_t> (readPos1)], frac));

            // High-pass filter in feedback path (~80Hz) to prevent mud buildup
            // 1st-order HP: y[n] = hpCoeff * (y[n-1] + x[n] - x[n-1])
            float hpOutL = hpCoeff * (hpStateL + delL - hpPrevInputL);
            float hpOutR = hpCoeff * (hpStateR + delR - hpPrevInputR);
            hpPrevInputL = delL;
            hpPrevInputR = delR;
            hpStateL = flushDenormal (hpOutL);
            hpStateR = flushDenormal (hpOutR);

            float fbL = hpOutL * feedback;
            float fbR = hpOutR * feedback;

            // Write to buffer: ping-pong cross-feeds L<->R, normal feeds L->L, R->R
            if (pingPong)
            {
                bufferL[static_cast<size_t> (writePos)] = inL + fbR;
                bufferR[static_cast<size_t> (writePos)] = inR + fbL;
            }
            else
            {
                bufferL[static_cast<size_t> (writePos)] = inL + fbL;
                bufferR[static_cast<size_t> (writePos)] = inR + fbR;
            }

            writePos = (writePos + 1) % bufferSize;

            // Mix dry/wet
            leftOut[i]  = inL * (1.0f - mix) + delL * mix;
            rightOut[i] = inR * (1.0f - mix) + delR * mix;
        }
    }

    //--------------------------------------------------------------------------
    /// Reset all delay state without reallocating buffers.
    void reset()
    {
        std::fill (bufferL.begin(), bufferL.end(), 0.0f);
        std::fill (bufferR.begin(), bufferR.end(), 0.0f);
        writePos = 0;
        hpStateL = 0.0f;
        hpStateR = 0.0f;
        hpPrevInputL = 0.0f;
        hpPrevInputR = 0.0f;
    }

private:
    void updateHPCoefficient()
    {
        // 1st-order high-pass at ~80Hz
        // RC = 1 / (2 * pi * fc), alpha = RC / (RC + dt)
        constexpr float hpFreq = 80.0f;
        constexpr float twoPi = 6.28318530718f;
        float rc = 1.0f / (twoPi * hpFreq);
        float dt = 1.0f / static_cast<float> (sr);
        hpCoeff = rc / (rc + dt);
    }

    double sr = 44100.0;
    int bufferSize = 0;
    std::vector<float> bufferL;
    std::vector<float> bufferR;
    int writePos = 0;

    float delayTimeMs = 375.0f;
    float feedback = 0.5f;
    float mix = 0.3f;
    bool pingPong = true;
    bool tempoSyncEnabled = false;

    // HP filter state (feedback path, ~80Hz)
    float hpCoeff = 0.0f;
    float hpStateL = 0.0f;
    float hpStateR = 0.0f;
    float hpPrevInputL = 0.0f;
    float hpPrevInputR = 0.0f;
};

} // namespace xomnibus
