#pragma once
#include "FastMath.h"
#include <cmath>

namespace xoceanus {

//==============================================================================
// ParameterSmoother — One-pole smoothing for zipper-free parameter automation.
//
// Consolidates the smoothing pattern found across 13+ engines. The algorithm:
//   smoothed += (target - smoothed) * coeff
//
// Default time constant: 5ms (the XOceanus fleet standard). Fast enough to
// track knob movements without audible latency; slow enough to suppress the
// zipper noise caused by stepped parameter changes at audio rate.
//
// Coefficient note: the coefficient is derived using 2π/T (angular frequency)
// rather than the simpler 1/T. This makes convergence ~6× faster than a
// strict RC filter at the same labeled time, which is exactly what parameter
// smoothing needs: settle quickly, but smooth the discontinuity. Think of the
// labeled `timeSec` as a comfort guide, not an RC specification.
//
// Usage:
//   ParameterSmoother cutoff;
//   cutoff.prepare (sampleRate);              // default 5ms
//   cutoff.prepare (sampleRate, 0.010f);      // custom 10ms
//   cutoff.set (targetValue);                 // call when param changes
//
//   // Per-sample:
//   float smoothedCutoff = cutoff.process();
//
// All methods are noexcept and allocation-free for real-time safety.
//==============================================================================
struct ParameterSmoother
{
    /// Initialize with sample rate and optional time constant (default 5ms).
    void prepare (float sampleRate, float timeSec = 0.005f) noexcept
    {
        if (sampleRate <= 0.0f || timeSec <= 0.0f)
        {
            coeff = 1.0f;
            return;
        }
        // Angular-frequency coefficient — see class comment for the full reasoning.
        coeff = 1.0f - std::exp (-kTwoPi / (timeSec * sampleRate));
    }

    /// Set the target value. The smoother will approach this over time.
    void set (float target) noexcept
    {
        targetValue = target;
    }

    /// Set target and snap immediately (no smoothing). Use on init or preset load.
    void snapTo (float value) noexcept
    {
        targetValue  = value;
        currentValue = value;
    }

    /// Advance one sample and return the smoothed value.
    float process() noexcept
    {
        currentValue += (targetValue - currentValue) * coeff;
        currentValue = flushDenormal (currentValue);
        if (std::fabs(currentValue - targetValue) < 1e-7f)
            currentValue = targetValue;
        return currentValue;
    }

    /// Get current smoothed value without advancing.
    float get() const noexcept { return currentValue; }

    //--------------------------------------------------------------------------
    // State — public for snapshot/restore
    //--------------------------------------------------------------------------
    float currentValue = 0.0f;
    float targetValue  = 0.0f;
    float coeff        = 1.0f;

private:
    static constexpr float kTwoPi = 6.28318530717958647692f;
};

//==============================================================================
// SmoothedParam — convenience wrapper that holds both the raw value and smoother.
// Useful when you need multiple smoothed parameters with the same time constant.
//
// Usage:
//   SmoothedParam params[4];
//   for (auto& p : params) p.prepare (sampleRate);
//
//   // Per block:
//   params[0].set (loadParam (pCutoff, 0.5f));
//   params[1].set (loadParam (pResonance, 0.0f));
//
//   // Per sample:
//   float cutoff = params[0].next();
//   float reso   = params[1].next();
//==============================================================================
struct SmoothedParam
{
    void prepare (float sampleRate, float timeSec = 0.005f) noexcept
    {
        smoother.prepare (sampleRate, timeSec);
    }

    void set (float target) noexcept { smoother.set (target); }
    void snapTo (float value) noexcept { smoother.snapTo (value); }
    float next() noexcept { return smoother.process(); }
    float get() const noexcept { return smoother.get(); }

private:
    ParameterSmoother smoother;
};

} // namespace xoceanus
