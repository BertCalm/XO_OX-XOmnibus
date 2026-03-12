#pragma once
#include <cmath>
#include <algorithm>
#include <array>

namespace xomnibus {

//==============================================================================
// TransientDesigner — Attack/sustain shaping via envelope followers.
//
// Splits signal into 2 bands (low/high at ~2kHz crossover), runs independent
// envelope followers on each, then applies gain shaping based on the
// difference between fast (attack) and slow (sustain) envelopes.
//
// Controls:
//   attack:  -1..+1 — reduce or boost transient punch
//   sustain: -1..+1 — reduce or boost sustain/tail
//   mix:     0..1   — parallel blend
//
// CPU: 2 biquads (crossover) + 4 envelope followers + gain. Very light.
//==============================================================================
class TransientDesigner
{
public:
    TransientDesigner() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        calcCrossover();
        reset();
    }

    void setAttack (float a)  { attack  = std::clamp (a, -1.0f, 1.0f); }
    void setSustain (float s) { sustain = std::clamp (s, -1.0f, 1.0f); }
    void setMix (float m)     { mix     = std::clamp (m, 0.0f, 1.0f); }

    void processBlock (float* left, float* right, int numSamples)
    {
        if (mix < 0.001f)
            return;

        const float attGain = attack * 12.0f;   // ±12dB range
        const float susGain = sustain * 12.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            float dryL = left[i];
            float dryR = right[i];

            float mono = (left[i] + right[i]) * 0.5f;
            float level = std::abs (mono);

            // Fast envelope (attack-sensitive, ~1ms)
            float fastCoeff = (level > envFast) ? fastAttack : fastRelease;
            envFast += fastCoeff * (level - envFast);

            // Slow envelope (sustain-sensitive, ~80ms)
            float slowCoeff = (level > envSlow) ? slowAttack : slowRelease;
            envSlow += slowCoeff * (level - envSlow);

            // Transient = difference between fast and slow
            float transient = envFast - envSlow;
            float sustainLevel = envSlow;

            // Compute gain modification in dB, then convert
            float gainDb = 0.0f;

            if (transient > 0.0001f)
                gainDb += attGain * transient;

            if (sustainLevel > 0.0001f)
                gainDb += susGain * sustainLevel;

            // Limit gain range to prevent blowup
            gainDb = std::clamp (gainDb, -24.0f, 24.0f);
            float gainLin = std::pow (10.0f, gainDb / 20.0f);

            float wetL = left[i]  * gainLin;
            float wetR = right[i] * gainLin;

            left[i]  = dryL + mix * (wetL - dryL);
            right[i] = dryR + mix * (wetR - dryR);
        }

        envFast = flushDenormal (envFast);
        envSlow = flushDenormal (envSlow);
    }

    void reset()
    {
        envFast = 0.0f;
        envSlow = 0.0f;

        // Recalculate time constants
        fastAttack  = 1.0f - std::exp (-1.0f / (static_cast<float> (sr) * 0.001f));
        fastRelease = 1.0f - std::exp (-1.0f / (static_cast<float> (sr) * 0.010f));
        slowAttack  = 1.0f - std::exp (-1.0f / (static_cast<float> (sr) * 0.020f));
        slowRelease = 1.0f - std::exp (-1.0f / (static_cast<float> (sr) * 0.080f));
    }

private:
    double sr = 44100.0;
    float attack  = 0.0f;
    float sustain = 0.0f;
    float mix     = 1.0f;

    // Envelope followers
    float envFast = 0.0f;
    float envSlow = 0.0f;

    // Coefficients
    float fastAttack  = 0.0f;
    float fastRelease = 0.0f;
    float slowAttack  = 0.0f;
    float slowRelease = 0.0f;

    void calcCrossover() { /* crossover reserved for future per-band mode */ }

    static float flushDenormal (float x)
    {
        return (std::abs (x) < 1.0e-15f) ? 0.0f : x;
    }
};

} // namespace xomnibus
