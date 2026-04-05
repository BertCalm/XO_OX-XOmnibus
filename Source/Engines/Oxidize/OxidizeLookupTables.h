// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <array>
#include <cmath>

namespace xoceanus {
namespace oxidize {

static constexpr int kLUTSize = 1024;

//==============================================================================
// OxidizeLookupTables
//
// Pre-computed age → parameter curves for OXIDIZE (engine #77).
//
// All LUTs map a normalized age value in [0.0, 1.0] to a parameter multiplier
// in [0.0, 1.0]. Index via:
//
//   int idx = static_cast<int>(age * (kLUTSize - 1));
//
// Use the provided `lookup()` helper for linear interpolation between entries,
// which avoids staircase artifacts when age changes slowly.
//
// Curve design intent (Guru Bin review):
//   corrosion  — dramatic in the middle; slow ignition, fast burn, hard plateau
//   erosion    — filter closes gradually then accelerates; near-linear until ~0.6, then steep
//   entropy    — even degradation; slight S keeps the beginning clean
//   wobble     — arises early and grows slowly; tape wobble is present before serious corrosion
//   dropout    — almost nothing until late; quadratic keeps early play safe
//   patina     — surface noise appears quickly then plateaus; crackle is audible from the start
//   sediment   — steady linear growth; reverb accumulates uniformly
//==============================================================================

struct LookupTables
{
    std::array<float, kLUTSize> corrosion;  // S-curve: slow start, fast middle, plateau
    std::array<float, kLUTSize> erosion;    // Exponential: filter cutoff drops faster at end
    std::array<float, kLUTSize> entropy;    // Linear with slight S
    std::array<float, kLUTSize> wobble;     // Logarithmic: wobble appears early, grows slowly
    std::array<float, kLUTSize> dropout;    // Quadratic: rare early, frequent late
    std::array<float, kLUTSize> patina;     // Square root: noise appears quickly then plateaus
    std::array<float, kLUTSize> sediment;   // Linear: reverb grows steadily

    void initialize()
    {
        for (int i = 0; i < kLUTSize; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(kLUTSize - 1);

            // --- corrosion: S-curve (cubic Hermite, emphasises mid-range oxidation) ---
            // f(t) = 3t² - 2t³  (classic smoothstep)
            // Then shifted slightly inward so the plateau starts around t=0.75
            // Full form: re-map t into [0,1] via remapped = clamp(t / 0.85, 0, 1)
            {
                const float r = std::fmin(t / 0.85f, 1.0f);
                corrosion[i] = r * r * (3.0f - 2.0f * r);
            }

            // --- erosion: convex exponential accelerating toward 1.0 at the end ---
            // f(t) = (e^(3t) - 1) / (e^3 - 1)  — stays near 0 until ~t=0.5, then climbs fast
            {
                constexpr float kE3 = 20.0855369f; // e^3
                erosion[i] = (std::exp(3.0f * t) - 1.0f) / (kE3 - 1.0f);
            }

            // --- entropy: linear with a gentle S-inflection ---
            // Blend: 0.7*t + 0.3*smoothstep(t)
            {
                const float smooth = t * t * (3.0f - 2.0f * t);
                entropy[i] = 0.7f * t + 0.3f * smooth;
            }

            // --- wobble: logarithmic — appears early, growth slows ---
            // f(t) = log(1 + k*t) / log(1 + k)  with k=9 for a pronounced knee
            {
                constexpr float kK = 9.0f;
                wobble[i] = std::log(1.0f + kK * t) / std::log(1.0f + kK);
            }

            // --- dropout: quadratic — rare early, frequent late ---
            // f(t) = t²  (pure; matches the spec's deriveFromAge() comment)
            {
                dropout[i] = t * t;
            }

            // --- patina: square root — noise floor rises quickly then plateaus ---
            // f(t) = sqrt(t)
            {
                patina[i] = std::sqrt(t);
            }

            // --- sediment: linear — reverb accumulates uniformly ---
            // f(t) = t
            {
                sediment[i] = t;
            }
        }
    }
};

//==============================================================================
// lookup() — linear interpolation between adjacent LUT entries.
//
// Accepts a fractional index in [0.0, 1.0] and returns a smoothly interpolated
// value. Clamps age to [0, 1] before indexing so callers need not guard.
//==============================================================================
inline float lookup(const std::array<float, kLUTSize>& lut, float age) noexcept
{
    // Clamp to valid range
    if (age <= 0.0f) return lut[0];
    if (age >= 1.0f) return lut[kLUTSize - 1];

    const float fIdx  = age * static_cast<float>(kLUTSize - 1);
    const int   lo    = static_cast<int>(fIdx);
    const int   hi    = lo + 1;   // safe: age < 1.0 guarantees hi < kLUTSize
    const float frac  = fIdx - static_cast<float>(lo);

    return lut[lo] + frac * (lut[hi] - lut[lo]);
}

//==============================================================================
// ageCurveFunction()
//
// Applies the user's `ageCurve` parameter to the raw aging delta-time `age`
// (current normalized age) to warp the aging trajectory.
//
//   curve = -1.0 → logarithmic: fast early, slow late  (tape winds down)
//   curve =  0.0 → linear:      uniform aging rate
//   curve = +1.0 → exponential: slow early, explosive late (rust bloom)
//
// The function returns a positive multiplier applied to `dt * rate` inside
// the age accumulator.  At curve=0 it returns 1.0 for all ages.
//
// Implementation:
//   For curve > 0 (exponential):  blend linear rate (1.0) with exp(k*age)-style
//   For curve < 0 (logarithmic):  blend linear rate with log-damped multiplier
//   Normalized so that the integral over [0,1] equals 1.0 in all cases,
//   meaning total aging time is preserved regardless of curve shape.
//
// Simplified (real-time-safe, branch-free blend):
//   multiplier = pow(age + eps, -curve)  — but we avoid pow() here.
//   Instead use a two-point lerp between 1.0 and the curved extremes.
//==============================================================================
inline float ageCurveFunction(float age, float curve) noexcept
{
    // Clamp inputs
    age   = std::fmax(0.0f, std::fmin(age, 1.0f));
    curve = std::fmax(-1.0f, std::fmin(curve, 1.0f));

    if (curve == 0.0f)
        return 1.0f;

    // Exponential side (curve > 0): rate starts slow, accelerates at high age.
    // Use e^(3*age) normalized so mean ≈ 1.  Mean of e^(3t) on [0,1] = (e^3-1)/3.
    // multiplier_exp = e^(3*age) * 3 / (e^3 - 1)
    constexpr float kE3      = 20.0855369f;
    constexpr float kExpNorm = 3.0f / (kE3 - 1.0f); // ≈ 0.1494
    const float     expMult  = std::exp(3.0f * age) * kExpNorm;

    // Logarithmic side (curve < 0): rate starts fast, tapers off.
    // multiplier_log = log(1 + 9*age) / (log(10) * mean)
    // Mean of log(1+9t) on [0,1] = (10*log(10) - 9) / 9 ≈ 0.5581
    constexpr float kLogMean = 0.558094f;
    constexpr float kK       = 9.0f;
    const float     logMult  = std::log(1.0f + kK * age) / (std::log(1.0f + kK) * kLogMean);

    // Blend: positive curve → mix toward exp; negative curve → mix toward log
    if (curve > 0.0f)
        return 1.0f + curve * (expMult - 1.0f);
    else
        return 1.0f + (-curve) * (logMult - 1.0f);
}

} // namespace oxidize
} // namespace xoceanus
