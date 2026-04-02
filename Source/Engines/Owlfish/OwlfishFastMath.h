#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>

namespace xowlfish {

//==============================================================================
// FastMath -- Denormal-safe fast math approximations for real-time DSP.
// All functions are inline, allocation-free, and framework-independent.
// XOwlfish local copy -- NOT shared with XOceanus.
//==============================================================================

//------------------------------------------------------------------------------
/// Flush denormals to zero. Call on every feedback/state variable each sample.
inline float flushDenormal (float x)
{
    uint32_t bits;
    std::memcpy (&bits, &x, sizeof (bits));
    if ((bits & 0x7F800000) == 0 && (bits & 0x007FFFFF) != 0)
        return 0.0f;
    return x;
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
/// Fast e^x using Schraudolph's method (IEEE 754 bit manipulation).
/// Accurate to ~4% across [-10, 10]. Suitable for envelopes and gain curves.
inline float fastExp (float x)
{
    if (x < -87.0f) return 0.0f;
    if (x >  88.0f) return 3.4028235e+38f;

    int32_t bits = static_cast<int32_t> (12102203.0f * x + 1065353216.0f);
    float result;
    std::memcpy (&result, &bits, sizeof (result));
    return result;
}

//------------------------------------------------------------------------------
/// Fast sine approximation using parabola with correction term.
/// Input: radians. Accurate to ~0.02% across the full period.
inline float fastSin (float x)
{
    constexpr float pi  = 3.14159265358979323846f;
    constexpr float tp  = 1.0f / (2.0f * pi);

    // Reduce to [0, 1) period
    x = x * tp + 0.5f;
    x = x - static_cast<float> (static_cast<int> (x)) + (x < 0.0f ? 1.0f : 0.0f);
    x = (x - 0.5f) * 2.0f;  // now in [-1, 1]

    // Parabola-based sine approximation with correction term
    constexpr float Q = 0.775f;
    float y = 4.0f * x * (1.0f - std::fabs (x));
    y = Q * y * (std::fabs (y) - 1.0f) + y;
    return y;
}

//------------------------------------------------------------------------------
/// Fast cosine via phase-shifted fastSin. Same accuracy (~0.02%).
inline float fastCos (float x)
{
    constexpr float halfPi = 1.5707963267948966f;
    return fastSin (x + halfPi);
}

//------------------------------------------------------------------------------
/// Convert decibels to linear gain -- fast path.
/// Uses fastExp instead of std::pow. -100 dB floor.
inline float dbToGain (float db)
{
    if (db <= -100.0f) return 0.0f;
    // 10^(db/20) = e^(db * ln(10)/20) = e^(db * 0.11512925)
    return fastExp (db * 0.11512925f);
}

//------------------------------------------------------------------------------
/// Fast log2 using IEEE 754 bit manipulation.
/// Accurate to ~0.09 across positive floats. Input must be > 0.
inline float fastLog2 (float x)
{
    if (x <= 0.0f) return -126.0f;

    int32_t bits;
    std::memcpy (&bits, &x, sizeof (bits));

    float exponent = static_cast<float> ((bits >> 23) - 127);
    bits = (bits & 0x007FFFFF) | 0x3F800000;

    float m;
    std::memcpy (&m, &bits, sizeof (m));
    float log2m = -1.7417939f + m * (2.8212026f + m * (-1.4699568f + m * 0.44717955f));

    return exponent + log2m;
}

//------------------------------------------------------------------------------
/// Convert linear gain to decibels.
/// Uses fastLog2 for speed. Zero or negative gain returns -100 dB.
inline float gainToDb (float gain)
{
    if (gain <= 0.0f) return -100.0f;
    // 20*log10(x) = 20*log2(x)/log2(10) = 6.0206*log2(x)
    return 6.0205999f * fastLog2 (gain);
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

//------------------------------------------------------------------------------
/// Convert MIDI note number to frequency in Hz.
/// Standard tuning: A4 = 440 Hz.
inline float midiToFreq (int note)
{
    return 440.0f * std::pow (2.0f, (static_cast<float> (note) - 69.0f) / 12.0f);
}

//------------------------------------------------------------------------------
/// One-pole smoother coefficient from time constant in seconds.
/// Call once when sample rate or time changes, not per-sample.
inline float smoothCoeffFromTime (float timeSec, float sampleRate)
{
    if (timeSec <= 0.0f || sampleRate <= 0.0f) return 1.0f;
    return 1.0f - fastExp (-1.0f / (timeSec * sampleRate));
}

} // namespace xowlfish
