#pragma once
//==============================================================================
// OperaConstants.h — Shared constants for all XOpera DSP modules
//==============================================================================

#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace opera {

//==============================================================================
// Mathematical constants
//==============================================================================
static constexpr float kPi        = 3.14159265358979323846f;
static constexpr float kTwoPi     = 6.28318530717958647692f;
static constexpr float kHalfPi    = 1.57079632679489661923f;

//==============================================================================
// Engine constants
//==============================================================================
static constexpr int   kMaxPartials = 48;
static constexpr int   kMaxVoices   = 8;
static constexpr int   kMaxBlockSize = 2048;
static constexpr float kKmax        = 8.0f;   // maximum coupling strength K
static constexpr int   kKuraBlock   = 8;       // samples between Kuramoto updates (6kHz at 48kHz)

//==============================================================================
// Lookup table constants
//==============================================================================
static constexpr int   kSinTableSize = 1024;
static constexpr int   kSinTableMask = kSinTableSize - 1;

//==============================================================================
// Kuramoto constants
//==============================================================================
static constexpr float kHysteresisRatio         = 0.7f;
static constexpr float kLockPhaseThreshold      = kPi / 6.0f;
static constexpr float kLockedCouplingBoost     = 1.3f;
static constexpr float kEmotionalMemoryWindowMs = 500.0f;
static constexpr int   kClusterMinSize          = 3;
static constexpr int   kClusterMaxSize          = 5;
static constexpr int   kMaxClusters             = kMaxPartials / kClusterMinSize;

//==============================================================================
// Shared fast math (for modules that don't use the SinTable)
//==============================================================================
inline float fastSin(float x) noexcept
{
    constexpr float tp = 1.0f / kTwoPi;
    x = x * tp + 0.5f;
    x = x - static_cast<float>(static_cast<int>(x)) + (x < 0.0f ? 1.0f : 0.0f);
    x = (x - 0.5f) * 2.0f;
    constexpr float Q = 0.775f;
    float y = 4.0f * x * (1.0f - std::fabs(x));
    y = Q * y * (std::fabs(y) - 1.0f) + y;
    return y;
}

inline float fastCos(float x) noexcept { return fastSin(x + kHalfPi); }

inline float flushDenormal(float x) noexcept
{
    uint32_t bits;
    std::memcpy(&bits, &x, sizeof(bits));
    if ((bits & 0x7F800000) == 0 && (bits & 0x007FFFFF) != 0) return 0.0f;
    return x;
}

inline float clamp(float x, float lo, float hi) noexcept
{
    return (x < lo) ? lo : ((x > hi) ? hi : x);
}

inline float lerp(float a, float b, float t) noexcept
{
    return a + t * (b - a);
}

} // namespace opera
