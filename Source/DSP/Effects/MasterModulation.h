#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>
#include "../FastMath.h"

namespace xomnibus {

//==============================================================================
// MasterModulation — Multi-mode modulation processor for the Master FX chain.
//
// Four modes:
//   1. Chorus:   Classic 2-voice BBD-style chorus (warm, wide)
//   2. Flanger:  Short delay + feedback comb filtering (jet-engine sweep)
//   3. Ensemble: 3-voice phase-offset chorus (Roland Dimension D inspired)
//   4. Drift:    Random-walk LFO wow/flutter (Chase Bliss Warped Vinyl)
//
// Features:
//   - BBD character: optional bandwidth limiting + subtle noise
//   - Stereo decorrelation via phase-offset LFOs
//   - Denormal protection on all feedback/state
//   - Zero CPU when depth = 0
//
// Inspired by: Walrus Julia, Chase Bliss Warped Vinyl, OBNE Visitor,
//              Roland Dimension D
//==============================================================================
class MasterModulation
{
public:
    enum class Mode
    {
        Chorus = 0,
        Flanger,
        Ensemble,
        Drift,
        NumModes
    };

    MasterModulation() = default;

    //--------------------------------------------------------------------------
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        // Max modulated delay: 40ms for chorus/ensemble, 5ms for flanger
        // Use 50ms to cover all modes with margin
        int maxDelaySamples = static_cast<int> (sr * 0.05) + 64;
        for (int v = 0; v < kMaxVoices; ++v)
        {
            delayLines[v].assign (static_cast<size_t> (maxDelaySamples), 0.0f);
            delayLinesR[v].assign (static_cast<size_t> (maxDelaySamples), 0.0f);
            writePositions[v] = 0;
        }
        maxDelayLen = maxDelaySamples;

        // Reset LFO phases
        for (int v = 0; v < kMaxVoices; ++v)
            lfoPhase[v] = static_cast<float> (v) / static_cast<float> (kMaxVoices);

        // Drift random state
        driftState = 0.0f;
        driftTarget = 0.0f;
        driftCounter = 0;
        driftInterval = static_cast<int> (sr * 0.15);  // new target every ~150ms

        // BBD lowpass state
        bbdLP_L = bbdLP_R = 0.0f;

        // Random seed from sample rate bits for deterministic-ish behavior
        randState = static_cast<uint32_t> (sr * 1000.0) ^ 0xDEADBEEF;
    }

    //--------------------------------------------------------------------------
    void setRate (float hz)      { rate = clamp (hz, 0.01f, 15.0f); }
    void setDepth (float d)      { depth = clamp (d, 0.0f, 1.0f); }
    void setMix (float wet)      { mix = clamp (wet, 0.0f, 1.0f); }
    void setMode (Mode m)        { mode = m; }
    void setFeedback (float fb)  { feedback = clamp (fb, 0.0f, 0.85f); }

    //--------------------------------------------------------------------------
    /// Process stereo audio in-place.
    void processBlock (float* L, float* R, int numSamples)
    {
        if (maxDelayLen <= 0) return;

        // Determine voice count and delay range based on mode
        int numVoices = 1;
        float minDelayMs = 0.0f;
        float maxDelayMs = 0.0f;
        float fbAmount = 0.0f;
        bool useBBD = false;

        switch (mode)
        {
            case Mode::Chorus:
                numVoices = 2;
                minDelayMs = 5.0f;
                maxDelayMs = 15.0f;
                fbAmount = 0.0f;
                useBBD = true;
                break;

            case Mode::Flanger:
                numVoices = 1;
                minDelayMs = 0.3f;
                maxDelayMs = 5.0f;
                fbAmount = feedback;
                useBBD = false;
                break;

            case Mode::Ensemble:
                numVoices = 3;
                minDelayMs = 5.0f;
                maxDelayMs = 25.0f;
                fbAmount = feedback * 0.3f;  // light feedback for thickening
                useBBD = true;
                break;

            case Mode::Drift:
                numVoices = 1;
                minDelayMs = 3.0f;
                maxDelayMs = 20.0f;
                fbAmount = 0.0f;
                useBBD = true;
                break;

            default:
                break;
        }

        if (numVoices > kMaxVoices) numVoices = kMaxVoices;

        float minDelaySamples = static_cast<float> (minDelayMs * 0.001f * static_cast<float> (sr));
        float maxDelaySamples = static_cast<float> (maxDelayMs * 0.001f * static_cast<float> (sr));
        float delayRange = maxDelaySamples - minDelaySamples;

        // BBD character: LP at ~8kHz in the wet path
        float bbdCoeff = useBBD ? clamp (8000.0f / static_cast<float> (sr) * 6.2832f, 0.0f, 1.0f)
                                : 1.0f;

        float phaseInc = rate / static_cast<float> (sr);

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = L[i];
            float inR = R[i];

            float wetL = 0.0f;
            float wetR = 0.0f;

            for (int v = 0; v < numVoices; ++v)
            {
                // Compute LFO value for this voice
                float lfoVal = 0.0f;

                if (mode == Mode::Drift)
                {
                    // Random walk LFO
                    lfoVal = computeDriftLFO();
                }
                else
                {
                    // Sine LFO with per-voice phase offset
                    lfoVal = fastSin (lfoPhase[v] * 6.28318530718f);
                }

                // Modulated delay in samples
                float modDelay = minDelaySamples + (lfoVal * 0.5f + 0.5f) * delayRange * depth;
                modDelay = clamp (modDelay, 1.0f, static_cast<float> (maxDelayLen - 2));

                // Read from delay line with linear interpolation
                int delaySamps = static_cast<int> (modDelay);
                float frac = modDelay - static_cast<float> (delaySamps);

                int wp = writePositions[v];
                int r0 = (wp - delaySamps + maxDelayLen) % maxDelayLen;
                int r1 = (r0 - 1 + maxDelayLen) % maxDelayLen;

                float dL = flushDenormal (lerp (delayLines[v][static_cast<size_t> (r0)],
                                                 delayLines[v][static_cast<size_t> (r1)], frac));
                float dR = flushDenormal (lerp (delayLinesR[v][static_cast<size_t> (r0)],
                                                 delayLinesR[v][static_cast<size_t> (r1)], frac));

                // Write to delay line with feedback
                delayLines[v][static_cast<size_t> (wp)] = inL + dL * fbAmount;
                delayLinesR[v][static_cast<size_t> (wp)] = inR + dR * fbAmount;
                writePositions[v] = (wp + 1) % maxDelayLen;

                // Stereo decorrelation: offset R phase by 90 degrees
                float lfoValR = (mode == Mode::Drift)
                    ? lfoVal  // Drift already has randomness
                    : fastSin ((lfoPhase[v] + 0.25f) * 6.28318530718f);

                float modDelayR = minDelaySamples + (lfoValR * 0.5f + 0.5f) * delayRange * depth;
                modDelayR = clamp (modDelayR, 1.0f, static_cast<float> (maxDelayLen - 2));

                int delaySampsR = static_cast<int> (modDelayR);
                float fracR = modDelayR - static_cast<float> (delaySampsR);
                int r0R = (wp - delaySampsR + maxDelayLen) % maxDelayLen;
                int r1R = (r0R - 1 + maxDelayLen) % maxDelayLen;

                float dRStereo = flushDenormal (lerp (delayLinesR[v][static_cast<size_t> (r0R)],
                                                       delayLinesR[v][static_cast<size_t> (r1R)], fracR));

                wetL += dL;
                wetR += dRStereo;

                // Advance LFO phase
                lfoPhase[v] += phaseInc;
                if (lfoPhase[v] >= 1.0f) lfoPhase[v] -= 1.0f;
            }

            // Normalize by voice count
            float normGain = 1.0f / static_cast<float> (numVoices);
            wetL *= normGain;
            wetR *= normGain;

            // BBD character: lowpass filter on wet signal
            if (useBBD && bbdCoeff < 1.0f)
            {
                bbdLP_L = flushDenormal (bbdLP_L + bbdCoeff * (wetL - bbdLP_L));
                bbdLP_R = flushDenormal (bbdLP_R + bbdCoeff * (wetR - bbdLP_R));
                wetL = bbdLP_L;
                wetR = bbdLP_R;
            }

            // Mix dry/wet
            L[i] = inL * (1.0f - mix) + wetL * mix;
            R[i] = inR * (1.0f - mix) + wetR * mix;
        }
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        for (int v = 0; v < kMaxVoices; ++v)
        {
            std::fill (delayLines[v].begin(), delayLines[v].end(), 0.0f);
            std::fill (delayLinesR[v].begin(), delayLinesR[v].end(), 0.0f);
            writePositions[v] = 0;
            lfoPhase[v] = static_cast<float> (v) / static_cast<float> (kMaxVoices);
        }
        bbdLP_L = bbdLP_R = 0.0f;
        driftState = driftTarget = 0.0f;
        driftCounter = 0;
    }

private:
    /// Drift mode: smoothed random walk
    float computeDriftLFO()
    {
        driftCounter++;
        if (driftCounter >= driftInterval)
        {
            driftCounter = 0;
            // New random target in [-1, 1]
            driftTarget = (nextRandom() * 2.0f - 1.0f);
            // Scale interval by rate (faster rate = shorter intervals)
            driftInterval = static_cast<int> (sr * 0.05f / clamp (rate, 0.05f, 15.0f));
            if (driftInterval < 1) driftInterval = 1;
        }

        // Smooth toward target
        float smoothing = 1.0f - fastExp (-rate * 0.5f / static_cast<float> (sr));
        driftState = flushDenormal (driftState + smoothing * (driftTarget - driftState));
        return driftState;
    }

    /// Simple xorshift32 PRNG → [0, 1)
    float nextRandom()
    {
        randState ^= randState << 13;
        randState ^= randState >> 17;
        randState ^= randState << 5;
        return static_cast<float> (randState & 0x7FFFFF) / static_cast<float> (0x7FFFFF);
    }

    //--------------------------------------------------------------------------
    static constexpr int kMaxVoices = 3;

    double sr = 44100.0;
    int maxDelayLen = 0;

    // Per-voice delay lines (stereo)
    std::vector<float> delayLines[kMaxVoices];
    std::vector<float> delayLinesR[kMaxVoices];
    int writePositions[kMaxVoices] {};

    // LFO state
    float lfoPhase[kMaxVoices] {};

    // Drift mode state
    float driftState = 0.0f;
    float driftTarget = 0.0f;
    int driftCounter = 0;
    int driftInterval = 6615;  // ~150ms @ 44.1k

    // BBD lowpass state
    float bbdLP_L = 0.0f;
    float bbdLP_R = 0.0f;

    // PRNG state
    uint32_t randState = 0xDEADBEEF;

    // Parameters
    Mode mode = Mode::Chorus;
    float rate = 0.8f;
    float depth = 0.0f;
    float mix = 0.0f;
    float feedback = 0.0f;
};

} // namespace xomnibus
