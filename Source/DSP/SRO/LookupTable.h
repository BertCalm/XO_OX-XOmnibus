#pragma once
#include <array>
#include <cmath>
#include <functional>

namespace xomnibus {

//==============================================================================
// LookupTable — Pre-computed function table with linear interpolation.
//
// Replaces expensive per-sample transcendental math (sin, exp, tanh, pow)
// with a table lookup + lerp. Trades a small amount of RAM for massive CPU
// savings in hot inner loops.
//
// Usage:
//     // At initialization (not real-time):
//     static const auto sinTable = LookupTable<4096>::generate(
//         0.0f, 6.2831853f,  // range: 0 to 2*pi
//         [](float x) { return std::sin(x); }
//     );
//
//     // In renderBlock() (real-time safe):
//     float value = sinTable.lookup(phase * 6.2831853f);
//
// Pre-built tables:
//     SROTables::sin()    — sin(x) for x in [0, 2*pi], 4096 points
//     SROTables::tanh()   — tanh(x) for x in [-4, 4], 2048 points
//     SROTables::exp()    — exp(x) for x in [-10, 10], 2048 points
//     SROTables::pow2()   — 2^x for x in [-10, 10], 2048 points
//
// Design:
//   - Fixed-size std::array (no heap allocation after construction)
//   - Linear interpolation between table entries (~0.01% error at 4096 points)
//   - Clamps out-of-range inputs to table boundaries
//   - Constexpr-friendly size; generate() can run at static init
//==============================================================================
template <int TableSize = 4096>
class LookupTable
{
    static_assert (TableSize >= 64, "Table size must be at least 64");
    static_assert ((TableSize & (TableSize - 1)) == 0, "Table size must be a power of 2");

public:
    LookupTable() = default;

    /// Construct with pre-computed values and range.
    LookupTable (std::array<float, TableSize + 1> data, float rangeMin, float rangeMax)
        : table (data), minVal (rangeMin), maxVal (rangeMax),
          invRange (static_cast<float> (TableSize) / (rangeMax - rangeMin))
    {
    }

    //--------------------------------------------------------------------------
    /// Generate a table from a lambda/function over [rangeMin, rangeMax].
    /// Call at initialization time (not real-time).
    static LookupTable generate (float rangeMin, float rangeMax,
                                 const std::function<float(float)>& func)
    {
        std::array<float, TableSize + 1> data {};
        float step = (rangeMax - rangeMin) / static_cast<float> (TableSize);
        for (int i = 0; i <= TableSize; ++i)
            data[static_cast<size_t> (i)] = func (rangeMin + static_cast<float> (i) * step);
        return LookupTable (data, rangeMin, rangeMax);
    }

    //--------------------------------------------------------------------------
    /// Look up a value with linear interpolation. Real-time safe.
    /// Input is clamped to the table range.
    float lookup (float x) const noexcept
    {
        // Clamp to range
        if (x <= minVal) return table[0];
        if (x >= maxVal) return table[TableSize];

        // Map x to table index
        float indexF = (x - minVal) * invRange;
        int index = static_cast<int> (indexF);
        float frac = indexF - static_cast<float> (index);

        // Linear interpolation
        return table[static_cast<size_t> (index)]
             + frac * (table[static_cast<size_t> (index + 1)]
                     - table[static_cast<size_t> (index)]);
    }

    /// Normalized lookup: input in [0, 1] maps to full table range.
    float lookupNormalized (float t) const noexcept
    {
        return lookup (minVal + t * (maxVal - minVal));
    }

    float getMin() const noexcept { return minVal; }
    float getMax() const noexcept { return maxVal; }

private:
    std::array<float, TableSize + 1> table {};
    float minVal  = 0.0f;
    float maxVal  = 1.0f;
    float invRange = static_cast<float> (TableSize);
};

//==============================================================================
// SROTables — Pre-built lookup tables for common transcendental functions.
//
// These are created once at static initialization (or on first access via
// Meyer's singleton) and shared across all engines.
//==============================================================================
struct SROTables
{
    /// sin(x), x in [0, 2*pi]. 4096 points → ~0.001% max error.
    static const LookupTable<4096>& sin()
    {
        static const auto table = LookupTable<4096>::generate (
            0.0f, 6.2831853f,
            [] (float x) { return std::sin (x); });
        return table;
    }

    /// tanh(x), x in [-4, 4]. 2048 points → ~0.01% max error.
    /// Covers the useful saturation range; beyond ±4 tanh is effectively ±1.
    static const LookupTable<2048>& tanh()
    {
        static const auto table = LookupTable<2048>::generate (
            -4.0f, 4.0f,
            [] (float x) { return std::tanh (x); });
        return table;
    }

    /// exp(x), x in [-10, 10]. 2048 points → ~0.05% max error.
    /// Covers envelope and gain curve ranges.
    static const LookupTable<2048>& exp()
    {
        static const auto table = LookupTable<2048>::generate (
            -10.0f, 10.0f,
            [] (float x) { return std::exp (x); });
        return table;
    }

    /// 2^x, x in [-10, 10]. 2048 points → ~0.02% max error.
    /// Critical for pitch calculations (semitone → frequency).
    static const LookupTable<2048>& pow2()
    {
        static const auto table = LookupTable<2048>::generate (
            -10.0f, 10.0f,
            [] (float x) { return std::pow (2.0f, x); });
        return table;
    }

    /// Soft-clip curve, x in [-2, 2]. 1024 points.
    /// Musical cubic saturation matching FastMath::softClip.
    static const LookupTable<1024>& softClip()
    {
        static const auto table = LookupTable<1024>::generate (
            -2.0f, 2.0f,
            [] (float x) {
                if (x <= -1.5f) return -1.0f;
                if (x >=  1.5f) return  1.0f;
                return x - (x * x * x) / 3.375f;
            });
        return table;
    }
};

} // namespace xomnibus
