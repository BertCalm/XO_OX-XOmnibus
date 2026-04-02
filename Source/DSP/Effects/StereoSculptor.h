#pragma once
#include <cmath>
#include <algorithm>
#include "../FastMath.h"

namespace xoceanus {

//==============================================================================
// StereoSculptor — Mid/Side frequency-dependent stereo field shaping.
//
// Splits signal into 3 frequency bands (low/mid/high), converts each to M/S,
// applies independent width control per band, then recombines.
//
// Typical use: mono bass, wide mids, hyper-spread highs.
//
// Controls:
//   lowWidth:   0..2 — stereo width for low band (0=mono, 1=natural, 2=wide)
//   midWidth:   0..2 — stereo width for mid band
//   highWidth:  0..2 — stereo width for high band
//   lowCross:   60..500Hz — low/mid crossover frequency
//   highCross:  2000..12000Hz — mid/high crossover frequency
//   mix:        0..1 — parallel blend
//
// CPU: 4 biquads (2 crossover pairs) + M/S encode/decode = very light.
//==============================================================================
class StereoSculptor
{
public:
    StereoSculptor() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        recalcCrossovers();
        reset();
    }

    void setLowWidth (float w)   { lowWidth  = std::clamp (w, 0.0f, 2.0f); }
    void setMidWidth (float w)   { midWidth  = std::clamp (w, 0.0f, 2.0f); }
    void setHighWidth (float w)  { highWidth = std::clamp (w, 0.0f, 2.0f); }

    // SRO: Dirty-flag coefficient caching — skip recalc if value unchanged
    void setLowCrossover (float f)
    {
        float clamped = std::clamp (f, 60.0f, 500.0f);
        if (std::abs (clamped - lowCross) > 0.01f)
        {
            lowCross = clamped;
            recalcCrossovers();
        }
    }

    void setHighCrossover (float f)
    {
        float clamped = std::clamp (f, 2000.0f, 12000.0f);
        if (std::abs (clamped - highCross) > 0.01f)
        {
            highCross = clamped;
            recalcCrossovers();
        }
    }

    void setMix (float m) { mix = std::clamp (m, 0.0f, 1.0f); }

    void processBlock (float* left, float* right, int numSamples)
    {
        if (mix < 0.001f)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            float dryL = left[i];
            float dryR = right[i];

            float inL = left[i];
            float inR = right[i];

            // Split into 3 bands using Linkwitz-Riley style cascaded filters
            // Low band: LPF at lowCross
            float lowL = processLPF (inL, lp1StateL);
            float lowR = processLPF (inR, lp1StateR);

            // Remainder after removing low
            float restL = inL - lowL;
            float restR = inR - lowR;

            // High band: HPF at highCross (from remainder)
            float highL = processHPF (restL, hp2StateL);
            float highR = processHPF (restR, hp2StateR);

            // Mid band: what's left
            float midL = restL - highL;
            float midR = restR - highR;

            // Apply M/S width per band
            applyWidth (lowL, lowR, lowWidth);
            applyWidth (midL, midR, midWidth);
            applyWidth (highL, highR, highWidth);

            // Recombine
            float wetL = lowL + midL + highL;
            float wetR = lowR + midR + highR;

            left[i]  = dryL + mix * (wetL - dryL);
            right[i] = dryR + mix * (wetR - dryR);
        }

        flushAllStates();
    }

    void reset()
    {
        lp1StateL = {}; lp1StateR = {};
        hp2StateL = {}; hp2StateR = {};
    }

private:
    double sr = 44100.0;
    float lowWidth  = 0.0f;  // Mono bass by default
    float midWidth  = 1.0f;  // Natural mids
    float highWidth = 1.5f;  // Wide highs
    float lowCross  = 200.0f;
    float highCross = 4000.0f;
    float mix       = 1.0f;

    struct BiquadState
    {
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    };

    struct BiquadCoeffs
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    };

    BiquadState lp1StateL, lp1StateR;
    BiquadCoeffs lp1Coeffs;

    BiquadState hp2StateL, hp2StateR;
    BiquadCoeffs hp2Coeffs;

    void recalcCrossovers()
    {
        calcLPF (lp1Coeffs, lowCross);
        calcHPF (hp2Coeffs, highCross);
    }

    void calcLPF (BiquadCoeffs& c, float freq)
    {
        // SRO: fastSin/fastCos replace std:: trig (per-setter coefficient calc)
        float w0 = 2.0f * 3.14159265f * freq / static_cast<float> (sr);
        float cosW0 = fastCos (w0);
        float sinW0 = fastSin (w0);
        float alpha = sinW0 / (2.0f * 0.707f);
        float a0 = 1.0f + alpha;
        c.b0 = ((1.0f - cosW0) / 2.0f) / a0;
        c.b1 = (1.0f - cosW0) / a0;
        c.b2 = ((1.0f - cosW0) / 2.0f) / a0;
        c.a1 = (-2.0f * cosW0) / a0;
        c.a2 = (1.0f - alpha) / a0;
    }

    void calcHPF (BiquadCoeffs& c, float freq)
    {
        // SRO: fastSin/fastCos replace std:: trig (per-setter coefficient calc)
        float w0 = 2.0f * 3.14159265f * freq / static_cast<float> (sr);
        float cosW0 = fastCos (w0);
        float sinW0 = fastSin (w0);
        float alpha = sinW0 / (2.0f * 0.707f);
        float a0 = 1.0f + alpha;
        c.b0 = ((1.0f + cosW0) / 2.0f) / a0;
        c.b1 = (-(1.0f + cosW0)) / a0;
        c.b2 = ((1.0f + cosW0) / 2.0f) / a0;
        c.a1 = (-2.0f * cosW0) / a0;
        c.a2 = (1.0f - alpha) / a0;
    }

    float processLPF (float in, BiquadState& s)
    {
        float out = lp1Coeffs.b0 * in + lp1Coeffs.b1 * s.x1 + lp1Coeffs.b2 * s.x2
                  - lp1Coeffs.a1 * s.y1 - lp1Coeffs.a2 * s.y2;
        s.x2 = s.x1; s.x1 = in;
        s.y2 = s.y1; s.y1 = out;
        return out;
    }

    float processHPF (float in, BiquadState& s)
    {
        float out = hp2Coeffs.b0 * in + hp2Coeffs.b1 * s.x1 + hp2Coeffs.b2 * s.x2
                  - hp2Coeffs.a1 * s.y1 - hp2Coeffs.a2 * s.y2;
        s.x2 = s.x1; s.x1 = in;
        s.y2 = s.y1; s.y1 = out;
        return out;
    }

    static void applyWidth (float& left, float& right, float width)
    {
        float mid  = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        side *= width;
        left  = mid + side;
        right = mid - side;
    }

    // SRO: Use shared flushDenormal from FastMath.h
    void flushAllStates()
    {
        auto flush = [] (BiquadState& s) {
            s.x1 = flushDenormal (s.x1); s.x2 = flushDenormal (s.x2);
            s.y1 = flushDenormal (s.y1); s.y2 = flushDenormal (s.y2);
        };
        flush (lp1StateL); flush (lp1StateR);
        flush (hp2StateL); flush (hp2StateR);
    }
};

} // namespace xoceanus
