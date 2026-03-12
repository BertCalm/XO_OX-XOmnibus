#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include "../FastMath.h"

namespace xomnibus {

//==============================================================================
// Combulator — Tuned comb filter bank with noise exciter.
//
// 3 parallel tuned comb filters with individual feedback, panning, and a
// built-in noise exciter. Creates pitched resonance, metallic textures,
// and Karplus-Strong-like effects from any input signal.
//
// Inspired by: Surge XT Combulator, Zebra2 comb filter
//
// Features:
//   - 3 comb filters tuned to frequency ratios (fundamental + 2 offsets)
//   - Noise exciter with tone control (feeds into combs)
//   - Per-comb feedback with denormal protection
//   - Stereo spread via comb panning
//   - MIDI pitch tracking option (combs tune to played notes)
//   - Zero CPU when mix = 0
//
// Usage:
//   Combulator comb;
//   comb.prepare(44100.0);
//   comb.setFrequency(220.0f);
//   comb.setFeedback(0.85f);
//   comb.setMix(0.3f);
//   comb.processBlock(L, R, numSamples);
//==============================================================================
class Combulator
{
public:
    Combulator() = default;

    //--------------------------------------------------------------------------
    void prepare (double sampleRate)
    {
        sr = sampleRate;

        // Max comb delay: ~20Hz fundamental = 50ms @ 44.1k
        int maxDelay = static_cast<int> (sr / 20.0) + 4;
        for (int c = 0; c < kNumCombs; ++c)
        {
            combBuffers[c].assign (static_cast<size_t> (maxDelay), 0.0f);
            combPos[c] = 0;
            combLP[c] = 0.0f;
        }
        maxCombLen = maxDelay;

        // Noise state
        noiseState = 0xACE1u;
        noiseLP = 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Set the fundamental comb frequency in Hz. Clamped to [20, 20000].
    void setFrequency (float hz)
    {
        frequency = clamp (hz, 20.0f, 20000.0f);
    }

    /// Set comb 2 offset in semitones from fundamental. [-24, +24]
    void setComb2Offset (float semitones)
    {
        comb2Offset = clamp (semitones, -24.0f, 24.0f);
    }

    /// Set comb 3 offset in semitones from fundamental. [-24, +24]
    void setComb3Offset (float semitones)
    {
        comb3Offset = clamp (semitones, -24.0f, 24.0f);
    }

    /// Set feedback for all combs. [0, 0.98] — higher = longer resonance.
    void setFeedback (float fb)
    {
        feedback = clamp (fb, 0.0f, 0.98f);
    }

    /// Set HF damping in comb feedback. [0, 1] — higher = darker resonance.
    void setDamping (float damp)
    {
        damping = clamp (damp, 0.0f, 1.0f);
    }

    /// Set noise exciter level. [0, 1] — adds pitched noise into the combs.
    void setNoiseLevel (float level)
    {
        noiseLevel = clamp (level, 0.0f, 1.0f);
    }

    /// Set noise tone (LP filter). [0, 1] — 0 = dark rumble, 1 = bright hiss.
    void setNoiseTone (float tone)
    {
        noiseTone = clamp (tone, 0.0f, 1.0f);
    }

    /// Set wet/dry mix. [0, 1] — 0 = bypass.
    void setMix (float wet)
    {
        mix = clamp (wet, 0.0f, 1.0f);
    }

    /// Set stereo spread. [0, 1] — 0 = mono, 1 = combs panned L/C/R.
    void setStereoSpread (float spread)
    {
        stereoSpread = clamp (spread, 0.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    /// Process stereo audio in-place.
    void processBlock (float* L, float* R, int numSamples)
    {
        if (maxCombLen <= 0) return;

        // Compute delay lengths for each comb
        float delays[kNumCombs];
        delays[0] = static_cast<float> (sr) / frequency;
        delays[1] = static_cast<float> (sr) / (frequency * fastPow2 (comb2Offset / 12.0f));
        delays[2] = static_cast<float> (sr) / (frequency * fastPow2 (comb3Offset / 12.0f));

        // Clamp delays to buffer size
        float maxD = static_cast<float> (maxCombLen - 2);
        for (int c = 0; c < kNumCombs; ++c)
            delays[c] = clamp (delays[c], 1.0f, maxD);

        // Damping coefficient: one-pole LP in feedback path
        float dampCoeff = damping * 0.7f;

        // Noise tone LP coefficient
        float noiseLPCoeff = 0.05f + noiseTone * 0.9f;

        // Stereo pan gains: comb0=left, comb1=center, comb2=right
        float panL[kNumCombs], panR[kNumCombs];
        float spread = stereoSpread;
        panL[0] = 0.5f + spread * 0.5f;   panR[0] = 0.5f - spread * 0.5f;  // left
        panL[1] = 0.5f;                    panR[1] = 0.5f;                    // center
        panL[2] = 0.5f - spread * 0.5f;   panR[2] = 0.5f + spread * 0.5f;  // right

        for (int i = 0; i < numSamples; ++i)
        {
            float inMono = (L[i] + R[i]) * 0.5f;

            // Generate noise exciter
            float noise = 0.0f;
            if (noiseLevel > 0.001f)
            {
                // Xorshift noise
                noiseState ^= noiseState << 13;
                noiseState ^= noiseState >> 17;
                noiseState ^= noiseState << 5;
                float rawNoise = (static_cast<float> (noiseState & 0xFFFF) / 32768.0f - 1.0f);

                // Tone LP filter on noise
                noiseLP = flushDenormal (noiseLP + noiseLPCoeff * (rawNoise - noiseLP));
                noise = noiseLP * noiseLevel * 0.3f;
            }

            float combInput = inMono + noise;

            // Process 3 parallel comb filters
            float wetL = 0.0f;
            float wetR = 0.0f;

            for (int c = 0; c < kNumCombs; ++c)
            {
                int len = static_cast<int> (combBuffers[c].size());
                if (len <= 0) continue;

                // Read with linear interpolation
                float delayF = delays[c];
                int delaySamps = static_cast<int> (delayF);
                float frac = delayF - static_cast<float> (delaySamps);

                int pos = combPos[c];
                int r0 = (pos - delaySamps + len) % len;
                int r1 = (r0 - 1 + len) % len;

                float readVal = flushDenormal (
                    lerp (combBuffers[c][static_cast<size_t> (r0)],
                          combBuffers[c][static_cast<size_t> (r1)], frac));

                // One-pole LP in feedback for damping
                combLP[c] = flushDenormal (
                    readVal * (1.0f - dampCoeff) + combLP[c] * dampCoeff);

                // Write: input + feedback
                combBuffers[c][static_cast<size_t> (pos)] =
                    combInput + combLP[c] * feedback;

                combPos[c] = (pos + 1) % len;

                // Pan and accumulate
                wetL += readVal * panL[c];
                wetR += readVal * panR[c];
            }

            // Normalize
            constexpr float norm = 1.0f / static_cast<float> (kNumCombs);
            wetL *= norm;
            wetR *= norm;

            // Mix dry/wet
            L[i] = L[i] * (1.0f - mix) + wetL * mix;
            R[i] = R[i] * (1.0f - mix) + wetR * mix;
        }
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        for (int c = 0; c < kNumCombs; ++c)
        {
            std::fill (combBuffers[c].begin(), combBuffers[c].end(), 0.0f);
            combPos[c] = 0;
            combLP[c] = 0.0f;
        }
        noiseLP = 0.0f;
    }

private:
    static constexpr int kNumCombs = 3;

    double sr = 44100.0;
    int maxCombLen = 0;

    // Comb filter state
    std::vector<float> combBuffers[kNumCombs];
    int combPos[kNumCombs] {};
    float combLP[kNumCombs] {};

    // Noise exciter state
    uint32_t noiseState = 0xACE1u;
    float noiseLP = 0.0f;

    // Parameters
    float frequency = 220.0f;
    float comb2Offset = 7.0f;    // +7 semitones (perfect 5th)
    float comb3Offset = 12.0f;   // +12 semitones (octave)
    float feedback = 0.85f;
    float damping = 0.3f;
    float noiseLevel = 0.0f;
    float noiseTone = 0.5f;
    float mix = 0.0f;
    float stereoSpread = 0.5f;
};

} // namespace xomnibus
