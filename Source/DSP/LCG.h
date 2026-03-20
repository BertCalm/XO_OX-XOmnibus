#pragma once
#include <cstdint>

namespace xomnibus {

//==============================================================================
// LCG — Shared Linear Congruential Generator (Knuth / Numerical Recipes).
//
// Constants: multiplier 1664525, addend 1013904223 (Knuth Vol.2 §3.6).
// Period: 2^32. Suitable for audio noise, stochastic grain, Brownian LFO.
//
// Usage:
//   LCG rng { 48271u };       // seed
//   float noise = rng.nextFloat();  // uniform [-1, +1)
//   float uni   = rng.nextUni();    // uniform [0, 1)
//==============================================================================
struct LCG
{
    uint32_t state;

    explicit LCG (uint32_t seed = 12345u) noexcept : state (seed) {}

    /// Advance and return raw 32-bit value.
    uint32_t next() noexcept
    {
        state = state * 1664525u + 1013904223u;
        return state;
    }

    /// Uniform float in [-1, +1).
    float nextFloat() noexcept
    {
        return static_cast<float> (next() & 0xFFFFu) / 32768.0f - 1.0f;
    }

    /// Uniform float in [0, 1).
    float nextUni() noexcept
    {
        return static_cast<float> (next() & 0xFFFFu) / 65536.0f;
    }
};

} // namespace xomnibus
