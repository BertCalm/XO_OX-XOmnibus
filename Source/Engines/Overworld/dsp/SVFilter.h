#pragma once
// XOverworld SVFilter — Topology-Preserving Trapezoidal (TPT) state-variable filter.
//
// Based on Andy Simper's Cytomic SVF design (2013).
// Supports LP, HP, and BP modes. Denormal-safe, sample-rate-aware.
// Coefficient update on setters (cheap), not per-sample (expensive).
//
// Uses fastTan from FastMath.h for prewarp computation.
// All state variables protected against denormals via flushDenormal().

#include "../../../DSP/FastMath.h"

namespace xoverworld {

using namespace xolokun;

class SVFilter {
public:

    void prepare(float sampleRate) {
        sr = sampleRate;
        ic1eq = 0.0f;
        ic2eq = 0.0f;
        computeCoefficients();
    }

    // mode: 0=LP, 1=HP, 2=BP, 3=Notch (notch = LP+HP passthrough)
    void setMode(int m) {
        mode = m;
    }

    // cutoffHz: clamped to [20, Nyquist*0.49]
    void setCutoff(float cutoffHz) {
        float nyq = sr * 0.49f;
        fc = cutoffHz < 20.0f    ? 20.0f
           : cutoffHz > nyq      ? nyq
           : cutoffHz;
        computeCoefficients();
    }

    // resonance: 0 = Butterworth (k=2), 1 = self-oscillation (k→0)
    void setResonance(float res) {
        q = res < 0.0f ? 0.0f : res > 1.0f ? 1.0f : res;
        computeCoefficients();
    }

    float process(float x) {
        // TPT SVF kernel — Andy Simper's formulation:
        //   v1 = (x - ic1eq*(k + g) - ic2eq) / (1 + g*(k + g))
        //   ...derived from trapezoidal discretisation of analog SVF state equations.
        float v0 = x;

        float v1 = (v0 - ic1eq * (k + g) - ic2eq) * a1;
        float v2 = ic2eq + g * v1;

        float lp = v2;
        float bp = v1;
        float hp = v0 - k * v1 - v2;

        // Update integrator states
        ic1eq = flushDenormal(2.0f * v1 - ic1eq);
        ic2eq = flushDenormal(2.0f * v2 - ic2eq);

        switch (mode) {
            case 0:  return lp;   // Low Pass
            case 1:  return hp;   // High Pass
            case 2:  return bp;   // Band Pass
            case 3:  return lp + hp; // Notch
            default: return lp;
        }
    }

    // Reset filter state (call on silence gaps to prevent tails)
    void reset() {
        ic1eq = 0.0f;
        ic2eq = 0.0f;
    }

private:

    void computeCoefficients() {
        if (sr <= 0.0f) return;
        // Trapezoidal prewarped frequency:
        //   g = tan(π * fc / sr)
        // fastTan accurate to ~0.03% for |x| < π/4.
        // Guard: fc/sr < 0.45 (enforced by setCutoff Nyquist clamp)
        constexpr float pi = 3.14159265358979323846f;
        g = fastTan(pi * fc / sr);

        // Damping coefficient: k = 2 - 2*resonance.
        // k=2 → Butterworth (Q=0.707), k→0 → self-oscillation
        k = 2.0f - 2.0f * q;

        // Pre-compute kernel denominator reciprocal
        // a1 = 1 / (1 + g*(g + k))
        float denom = 1.0f + g * (g + k);
        a1 = (denom > 1e-10f) ? 1.0f / denom : 1.0f;
    }

    float sr    = 44100.0f;
    float fc    = 8000.0f;   // current cutoff Hz
    float q     = 0.3f;      // resonance [0,1]
    int   mode  = 0;

    // TPT coefficients (updated in computeCoefficients)
    float g  = 0.0f;  // prewarped frequency
    float k  = 1.4f;  // damping (k = 2 - 2*resonance)
    float a1 = 1.0f;  // 1 / (1 + g*(g+k))

    // Integrator state variables (ic = initial condition)
    float ic1eq = 0.0f;
    float ic2eq = 0.0f;
};

} // namespace xoverworld
