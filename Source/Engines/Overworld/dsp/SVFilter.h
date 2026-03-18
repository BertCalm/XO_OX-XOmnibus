#pragma once
#include <cmath>
#include <algorithm>

namespace xoverworld {

//==============================================================================
// SVFilter — State-Variable Filter (LP / HP / BP)
//
// Topology: Chamberlin SVF. Matched-Z coefficient calculation.
// Chip synth signal chain uses this as the post-voice master tonestack.
// Operates mono, inline, no allocation.
//
// Modes: 0 = Low-pass, 1 = High-pass, 2 = Band-pass
//==============================================================================
class SVFilter
{
public:
    SVFilter() = default;

    void prepare(float sampleRate)
    {
        sr = sampleRate;
        low = band = 0.0f;
        updateCoeffs();
    }

    void setMode(int mode)
    {
        filterMode = std::max(0, std::min(2, mode));
    }

    void setCutoff(float hz)
    {
        cutoffHz = std::max(20.0f, std::min(hz, sr * 0.49f));
        updateCoeffs();
    }

    void setResonance(float q)
    {
        // q in [0, 1] — maps to Q factor [0.5, 10]
        resonance = std::max(0.0f, std::min(1.0f, q));
        updateCoeffs();
    }

    float process(float x)
    {
        // Chamberlin 2-pole SVF
        low  = low  + f * band;
        float high = x - low - q * band;
        band = f * high + band;

        // Denormal flush
        low  = flushDenormal(low);
        band = flushDenormal(band);

        switch (filterMode)
        {
            case 1:  return high;
            case 2:  return band;
            default: return low;
        }
    }

private:
    void updateCoeffs()
    {
        // Matched-Z: f = 2 * sin(pi * fc / sr)
        f = 2.0f * std::sin(3.14159265358979323846f * cutoffHz / sr);
        f = std::max(0.0001f, std::min(f, 1.9999f));

        // Q: resonance [0,1] → Q factor [0.5, 10], damping q = 1/Q
        float Q = 0.5f + resonance * 9.5f;
        q = 1.0f / Q;
    }

    static float flushDenormal(float x)
    {
        return (std::abs(x) < 1e-15f) ? 0.0f : x;
    }

    float sr        = 44100.0f;
    float cutoffHz  = 8000.0f;
    float resonance = 0.0f;
    int   filterMode = 0;

    // Coefficients
    float f  = 1.0f;
    float q  = 1.0f;

    // State
    float low  = 0.0f;
    float band = 0.0f;
};

} // namespace xoverworld
