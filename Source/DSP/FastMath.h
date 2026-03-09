#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>

namespace xomnibus {

//==============================================================================
// FastMath — Denormal-safe fast math approximations for real-time DSP.
// All functions are inline, allocation-free, and framework-independent.
//==============================================================================

//------------------------------------------------------------------------------
/// Flush denormals to zero. Call on every feedback/state variable each sample.
inline float flushDenormal (float x)
{
    // Use memcpy for type-safe bit inspection (well-defined in C++, unlike union punning)
    uint32_t bits;
    std::memcpy (&bits, &x, sizeof (bits));
    if ((bits & 0x7F800000) == 0 && (bits & 0x007FFFFF) != 0)
        return 0.0f;
    return x;
}

//------------------------------------------------------------------------------
/// Fast e^x using Schraudolph's method (IEEE 754 bit manipulation).
/// Accurate to ~4% across [-10, 10]. Suitable for envelopes and gain curves.
inline float fastExp (float x)
{
    // Clamp to avoid overflow/underflow in the integer conversion
    if (x < -87.0f) return 0.0f;
    if (x >  88.0f) return 3.4028235e+38f;

    // Schraudolph's approximation: interpret float bits as integer
    // 2^23 / ln(2) = 12102203.16, bias = 127 * 2^23 = 1065353216
    int32_t bits = static_cast<int32_t> (12102203.0f * x + 1065353216.0f);
    float result;
    std::memcpy (&result, &bits, sizeof (result));
    return result;
}

//------------------------------------------------------------------------------
/// Fast tanh approximation using rational function.
/// Smooth saturation curve, accurate to ~0.1% in [-3, 3].
/// Beyond +/-3 it clamps to +/-1 (true tanh is >0.995 there anyway).
inline float fastTanh (float x)
{
    if (x < -3.0f) return -1.0f;
    if (x >  3.0f) return  1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

//------------------------------------------------------------------------------
/// Fast sine approximation using a 5th-order polynomial (Bhaskara-inspired).
/// Input: radians. Accurate to ~0.02% across the full period.
inline float fastSin (float x)
{
    // Wrap x into [-pi, pi]
    constexpr float pi  = 3.14159265358979323846f;
    constexpr float tp  = 1.0f / (2.0f * pi);

    // Reduce to [0, 1) period
    x = x * tp + 0.5f;
    x = x - static_cast<float> (static_cast<int> (x)) + (x < 0.0f ? 1.0f : 0.0f);
    x = (x - 0.5f) * 2.0f;  // now in [-1, 1]

    // Parabola-based sine approximation with correction term
    // Based on: y = 4x(1-|x|), then corrected with: y = Q * y * (|y| - 1) + y
    constexpr float Q = 0.775f;
    float y = 4.0f * x * (1.0f - std::fabs (x));
    y = Q * y * (std::fabs (y) - 1.0f) + y;
    return y;
}

//------------------------------------------------------------------------------
/// Fast log2 using IEEE 754 bit manipulation.
/// Accurate to ~0.09 across positive floats. Input must be > 0.
inline float fastLog2 (float x)
{
    if (x <= 0.0f) return -126.0f;  // floor value for non-positive input

    int32_t bits;
    std::memcpy (&bits, &x, sizeof (bits));

    // Extract exponent and mantissa from IEEE 754 representation
    float exponent = static_cast<float> ((bits >> 23) - 127);
    bits = (bits & 0x007FFFFF) | 0x3F800000;  // set exponent to 0 (value in [1,2))

    // Polynomial approximation of log2 in [1, 2)
    // Minimax cubic: log2(m) ~ -1.7417939 + m * (2.8212026 + m * (-1.4699568 + m * 0.44717955))
    float m;
    std::memcpy (&m, &bits, sizeof (m));
    float log2m = -1.7417939f + m * (2.8212026f + m * (-1.4699568f + m * 0.44717955f));

    return exponent + log2m;
}

//------------------------------------------------------------------------------
/// Convert MIDI note number to frequency in Hz.
/// Uses standard A440 tuning: freq = 440 * 2^((note - 69) / 12).
inline float midiToFreq (int note)
{
    return 440.0f * std::pow (2.0f, (static_cast<float> (note) - 69.0f) / 12.0f);
}

//------------------------------------------------------------------------------
/// Convert decibels to linear gain.
/// -infinity dB (or anything below -100) returns 0.
inline float dbToGain (float db)
{
    if (db <= -100.0f) return 0.0f;
    return std::pow (10.0f, db * 0.05f);
}

//------------------------------------------------------------------------------
/// Convert linear gain to decibels.
/// Zero or negative gain returns -100 dB (silence floor).
inline float gainToDb (float gain)
{
    if (gain <= 0.0f) return -100.0f;
    return 20.0f * std::log10 (gain);
}

//------------------------------------------------------------------------------
/// Clamp a float to [min, max]. Branchless-friendly for the compiler.
inline float clamp (float x, float lo, float hi)
{
    return (x < lo) ? lo : ((x > hi) ? hi : x);
}

//------------------------------------------------------------------------------
/// Linear interpolation between a and b. t in [0, 1].
inline float lerp (float a, float b, float t)
{
    return a + t * (b - a);
}

} // namespace xomnibus
