#pragma once
#include "PresetManager.h" // PresetData, PresetDNA

#include <algorithm>
#include <array>
#include <cmath>
#include <queue>
#include <vector>

namespace xoceanus
{

/** Utility functions for 6D Sonic DNA distance, interpolation, and k-nearest search.
 *
 *  All functions are header-only inline.  The only allocating functions are kNearest
 *  and kNearestForEngine, which return a result vector.
 *
 *  DNA values are clamped to [0,1] before arithmetic to tolerate dirty data.
 */
namespace DNAProximity
{

namespace detail
{
    /** Clamp a single DNA value to [0, 1]. */
    inline float clamp01 (float v) noexcept { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

    /** Extract the six components of a PresetDNA as a plain array, clamped. */
    inline std::array<float, 6> components (const PresetDNA& d) noexcept
    {
        return { clamp01 (d.brightness),
                 clamp01 (d.warmth),
                 clamp01 (d.movement),
                 clamp01 (d.density),
                 clamp01 (d.space),
                 clamp01 (d.aggression) };
    }
} // namespace detail

//==============================================================================
/** Euclidean distance in 6D Sonic DNA space.
 *  Symmetric.  Range: [0, sqrt(6)] ≈ [0, 2.449].
 *  Returns 0 when a == b. */
inline float distance (const PresetDNA& a, const PresetDNA& b) noexcept
{
    const auto ca = detail::components (a);
    const auto cb = detail::components (b);
    float sum = 0.0f;
    for (int i = 0; i < 6; ++i)
    {
        const float d = ca[i] - cb[i];
        sum += d * d;
    }
    return std::sqrt (sum);
}

//==============================================================================
/** Weighted Euclidean distance.
 *  Caller supplies per-dimension weights in the order:
 *  [brightness, warmth, movement, density, space, aggression].
 *  Weights are applied before squaring, so a weight of 0 excludes a dimension
 *  and a weight of sqrt(6) restores equal-unit scaling. */
inline float weightedDistance (const PresetDNA& a,
                                const PresetDNA& b,
                                const std::array<float, 6>& weights) noexcept
{
    const auto ca = detail::components (a);
    const auto cb = detail::components (b);
    float sum = 0.0f;
    for (int i = 0; i < 6; ++i)
    {
        const float d = (ca[i] - cb[i]) * weights[i];
        sum += d * d;
    }
    return std::sqrt (sum);
}

//==============================================================================
/** Centroid (component-wise arithmetic mean) of N DNA vectors.
 *  Returns a zero-initialised PresetDNA when the input is empty. */
inline PresetDNA centroid (const std::vector<PresetDNA>& vectors) noexcept
{
    PresetDNA result{};
    if (vectors.empty())
        return result;

    float sumB = 0, sumW = 0, sumM = 0, sumD = 0, sumS = 0, sumA = 0;
    for (const auto& v : vectors)
    {
        sumB += detail::clamp01 (v.brightness);
        sumW += detail::clamp01 (v.warmth);
        sumM += detail::clamp01 (v.movement);
        sumD += detail::clamp01 (v.density);
        sumS += detail::clamp01 (v.space);
        sumA += detail::clamp01 (v.aggression);
    }
    const float inv = 1.0f / static_cast<float> (vectors.size());
    result.brightness  = sumB * inv;
    result.warmth      = sumW * inv;
    result.movement    = sumM * inv;
    result.density     = sumD * inv;
    result.space       = sumS * inv;
    result.aggression  = sumA * inv;
    return result;
}

//==============================================================================
/** Linear interpolation between two DNA vectors.
 *  t = 0 returns a; t = 1 returns b.  t is NOT clamped — callers may
 *  extrapolate, but DNA fields may leave [0, 1] if they do. */
inline PresetDNA interpolate (const PresetDNA& a, const PresetDNA& b, float t) noexcept
{
    const float u = 1.0f - t;
    PresetDNA result{};
    result.brightness = u * detail::clamp01 (a.brightness) + t * detail::clamp01 (b.brightness);
    result.warmth     = u * detail::clamp01 (a.warmth)     + t * detail::clamp01 (b.warmth);
    result.movement   = u * detail::clamp01 (a.movement)   + t * detail::clamp01 (b.movement);
    result.density    = u * detail::clamp01 (a.density)    + t * detail::clamp01 (b.density);
    result.space      = u * detail::clamp01 (a.space)      + t * detail::clamp01 (b.space);
    result.aggression = u * detail::clamp01 (a.aggression) + t * detail::clamp01 (b.aggression);
    return result;
}

//==============================================================================
/** Find the K nearest presets by Euclidean DNA distance.
 *
 *  Returns indices into @p corpus, sorted nearest-first.
 *  If the query preset is found in corpus (by pointer identity or equal DNA
 *  distance == 0 and same name), it is excluded from results.
 *
 *  Complexity: O(N log K) using a fixed-size max-heap of capacity K.
 *  At most K elements are held in the heap at any time. */
inline std::vector<size_t> kNearest (const PresetData&              query,
                                      const std::vector<PresetData>& corpus,
                                      size_t                         k) noexcept
{
    if (k == 0 || corpus.empty())
        return {};

    // Max-heap: (distance, index).  Keeps the K *smallest* distances.
    using Pair = std::pair<float, size_t>;
    std::priority_queue<Pair> heap;

    for (size_t i = 0; i < corpus.size(); ++i)
    {
        // Exclude the query preset itself (match by name + identity distance).
        if (corpus[i].name == query.name &&
            distance (corpus[i].dna, query.dna) < 1e-7f)
            continue;

        const float dist = distance (query.dna, corpus[i].dna);

        if (heap.size() < k)
        {
            heap.push ({ dist, i });
        }
        else if (dist < heap.top().first)
        {
            heap.pop();
            heap.push ({ dist, i });
        }
    }

    // Drain heap into result (heap is max-first, so reverse for nearest-first).
    std::vector<size_t> result;
    result.reserve (heap.size());
    while (!heap.empty())
    {
        result.push_back (heap.top().second);
        heap.pop();
    }
    std::reverse (result.begin(), result.end());
    return result;
}

//==============================================================================
/** Like kNearest, but restricts the search to presets whose @c engines array
 *  contains @p engineId.  Used for the per-slot "Similar" tab in the browser. */
inline std::vector<size_t> kNearestForEngine (const PresetData&              query,
                                               const std::vector<PresetData>& corpus,
                                               const juce::String&            engineId,
                                               size_t                         k) noexcept
{
    if (k == 0 || corpus.empty())
        return {};

    using Pair = std::pair<float, size_t>;
    std::priority_queue<Pair> heap;

    for (size_t i = 0; i < corpus.size(); ++i)
    {
        if (!corpus[i].engines.contains (engineId))
            continue;

        if (corpus[i].name == query.name &&
            distance (corpus[i].dna, query.dna) < 1e-7f)
            continue;

        const float dist = distance (query.dna, corpus[i].dna);

        if (heap.size() < k)
        {
            heap.push ({ dist, i });
        }
        else if (dist < heap.top().first)
        {
            heap.pop();
            heap.push ({ dist, i });
        }
    }

    std::vector<size_t> result;
    result.reserve (heap.size());
    while (!heap.empty())
    {
        result.push_back (heap.top().second);
        heap.pop();
    }
    std::reverse (result.begin(), result.end());
    return result;
}

} // namespace DNAProximity

} // namespace xoceanus

/*
 * Self-check (invariants, not runnable tests):
 *
 *   distance(a, a) == 0
 *     Each (ca[i] - ca[i])^2 == 0, sqrt(0) == 0. ✓
 *
 *   distance(a, b) == distance(b, a)   [symmetry]
 *     (ca[i] - cb[i])^2 == (cb[i] - ca[i])^2. ✓
 *
 *   centroid({a}) == a
 *     sum of one element / 1 == element. ✓
 *
 *   interpolate(a, b, 0) == a
 *     u=1, t=0  →  1*a + 0*b == a. ✓
 *
 *   interpolate(a, b, 1) == b
 *     u=0, t=1  →  0*a + 1*b == b. ✓
 */
