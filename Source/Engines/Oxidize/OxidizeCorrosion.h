// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <cstdint>
#include "../../DSP/FastMath.h"

namespace xoceanus {
namespace oxidize {

//==============================================================================
// CorrosionMode
//
// Six waveshaper modes for OXIDIZE (engine #77).
// Ordered from smoothest to most destructive.
//==============================================================================
enum class CorrosionMode
{
    Valve         = 0,  // tanh — smooth tube saturation
    Transformer   = 1,  // x/(1+|x*d|) — low-mid thickening
    BrokenSpeaker = 2,  // asymmetric clip + noise injection
    TapeSat       = 3,  // tanh with 3rd harmonic emphasis
    Rust          = 4,  // asymmetric: positive clips harder as rust (age) grows
    Acid          = 5   // wavefolder: sin(x * drive * pi)
};

//==============================================================================
// Individual mode implementations — all pure static inline functions.
//
// Contract:
//   - input: pre-gain signal, typically in [-1, 1] before drive is applied
//   - drive: waveshaper intensity [0, ~4]; engine scales this from age + param
//   - rust:  age-derived [0, 1] used by Rust mode for asymmetry accumulation
//   - return: processed sample; callers must apply output gain themselves
//
// All outputs pass through flushDenormal() to prevent denormal CPU spikes in
// feedback paths (CLAUDE.md architecture rule).
//==============================================================================

//------------------------------------------------------------------------------
// Valve — tanh tube saturation.
// The canonical smooth saturator: even harmonics dominant at low drive, broad
// harmonic spread at high drive. Symmetric, so no DC offset.
//------------------------------------------------------------------------------
static inline float processValve(float input, float drive) noexcept
{
    // fastTanh is a Padé approx (~2.6% error) — sufficient for saturation.
    // For full accuracy at very low drive levels, std::tanh would be preferable,
    // but the difference is inaudible above ~-60 dB.
    const float x = input * (1.0f + drive * 3.0f); // drive maps [0,1] → [1,4]× gain
    return flushDenormal(std::tanh(x));
}

//------------------------------------------------------------------------------
// Transformer — soft clip with low-mid thickening.
// x / (1 + |x * drive|)  is a memoryless soft-knee limiter that compresses
// peaks while preserving the body of the waveform. The mid-frequency "thickness"
// comes from the frequency-dependent gain reduction pattern.
//------------------------------------------------------------------------------
static inline float processTransformer(float input, float drive) noexcept
{
    const float d = 1.0f + drive * 3.0f;
    const float x = input * d;
    const float y = x / (1.0f + std::abs(x));
    return flushDenormal(y);
}

//------------------------------------------------------------------------------
// BrokenSpeaker — asymmetric clip with noise injection.
// Positive half clips at clipPos, negative half clips at clipNeg (harder).
// A small white-noise injection (PRNG-derived) simulates cone flutter and
// voice-coil damage. The asymmetry creates audible even harmonics (DC offset
// is intentional — reproduces the sag of an over-driven, failing speaker).
//
// noise parameter: caller passes a float in [0,1] derived from PRNG each
// sample. We accept it here so the function remains stateless and testable.
//------------------------------------------------------------------------------
static inline float processBrokenSpeaker(float input, float drive, float noise) noexcept
{
    // Scale input by drive
    const float x       = input * (1.0f + drive * 2.5f);
    const float clipPos = 1.0f;               // positive: full rail
    const float clipNeg = -(0.6f + 0.2f * drive); // negative: clips harder with drive

    float y;
    if (x > clipPos)
        y = clipPos;
    else if (x < clipNeg)
        y = clipNeg;
    else
        y = x;

    // Noise injection: scaled to drive so pristine signal is clean
    // noise is pre-computed in [0,1]; centre around 0 and scale
    const float noiseAmt = drive * 0.04f;
    y += (noise * 2.0f - 1.0f) * noiseAmt;

    return flushDenormal(y);
}

//------------------------------------------------------------------------------
// TapeSat — tanh with 3rd harmonic emphasis.
// Tape saturation has a distinctive odd-harmonic colour from the hysteresis
// loop. We approximate it by blending a fundamental tanh with a 3× gain copy,
// which injects a strong 3rd harmonic without complex hysteresis modelling.
// The 0.15 coefficient is calibrated so the harmonic injection is audible but
// doesn't dominate — sounds thicker than Valve without being boxy.
//------------------------------------------------------------------------------
static inline float processTapeSat(float input, float drive) noexcept
{
    const float d = 1.0f + drive * 2.5f;
    const float x = input * d;
    const float y = std::tanh(x) + 0.15f * std::tanh(x * 3.0f);
    // Normalise output so it stays near [-1,1] regardless of harmonic addition
    return flushDenormal(y / 1.15f);
}

//------------------------------------------------------------------------------
// Rust — asymmetric waveshaper whose asymmetry increases with age.
// As rust accumulates (0→1), the positive clipping threshold drops toward a
// hard limit while the negative side remains relatively open. The result is an
// increasing DC offset and an audible shift toward odd+even harmonic chaos.
// At rust=0.0 it behaves like a mild soft-clip. At rust=1.0 the positive half
// is severely crushed.
//------------------------------------------------------------------------------
static inline float processRust(float input, float drive, float rust) noexcept
{
    const float x = input * (1.0f + drive * 2.0f);

    // Positive clipping threshold falls from 1.0 to 0.3 as rust grows
    const float posClip = 1.0f - rust * 0.7f;
    // Negative threshold: starts at -1.0, creeps in slightly at high rust
    const float negClip = -(1.0f - rust * 0.15f);

    float y;
    if (x > posClip)
        y = posClip;
    else if (x < negClip)
        y = negClip;
    else
        y = x;

    // Additional soft-saturation on the positive side using tanh to round the
    // clipping shoulder (avoids hard clicks when the threshold is low)
    if (x > 0.0f)
        y = std::tanh(y / (posClip + 1e-6f)) * posClip;

    return flushDenormal(y);
}

//------------------------------------------------------------------------------
// Acid — wavefolder using sin(x * drive * pi).
// Wavefolding creates metallic, unstable upper partials not found in
// conventional saturation. At low drive it sounds like a subtle shimmer;
// at high drive the waveform folds back on itself repeatedly, producing
// inharmonic content that becomes increasingly chaotic and "corroded".
//------------------------------------------------------------------------------
static inline float processAcid(float input, float drive) noexcept
{
    constexpr float kPi = 3.14159265358979f;
    // Scale drive: 1 fold at drive=0 (no folding), up to ~6 folds at drive=1
    const float foldAmt = 1.0f + drive * 5.0f;
    const float y = std::sin(input * foldAmt * kPi);
    return flushDenormal(y);
}

//==============================================================================
// processCorrosion() — mode dispatcher.
//
// Accepts the BrokenSpeaker noise value as a parameter so the caller can
// supply a per-sample PRNG float; for all other modes the value is ignored.
//
//   input  — signal sample (pre-drive scaling is applied inside each mode)
//   drive  — waveshaper intensity [0, 1] (modes internally remap the range)
//   mode   — which waveshaper to apply
//   rust   — age-derived asymmetry parameter for Rust mode [0, 1]
//   noise  — per-sample uniform noise in [0, 1] for BrokenSpeaker mode
//==============================================================================
static inline float processCorrosion(float input,
                                     float drive,
                                     CorrosionMode mode,
                                     float rust,
                                     float noise = 0.5f) noexcept
{
    switch (mode)
    {
        case CorrosionMode::Valve:         return processValve(input, drive);
        case CorrosionMode::Transformer:   return processTransformer(input, drive);
        case CorrosionMode::BrokenSpeaker: return processBrokenSpeaker(input, drive, noise);
        case CorrosionMode::TapeSat:       return processTapeSat(input, drive);
        case CorrosionMode::Rust:          return processRust(input, drive, rust);
        case CorrosionMode::Acid:          return processAcid(input, drive);
        default:                           return flushDenormal(input);
    }
}

} // namespace oxidize
} // namespace xoceanus
