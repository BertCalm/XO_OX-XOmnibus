#pragma once

/*
    GestureTrailBuffer.h
    =====================
    B043 Gesture Trail — a 256-tuple ring buffer with freeze/replay and
    two-trail interference. Pure C++17, no JUCE dependency.

    B043: "Gesture Trail as First-Class Modulation Source — the bioluminescent
    performance trail promoted to a replayable, freezable, coupleable DSP
    modulation signal. The performer's gesture history becomes a live modulation
    output. Two performers' trails create interference-pattern modulation
    neither intended alone."  — Ratified 6-2.
*/

#include <array>
#include <cmath>
#include <utility>
#include <algorithm>

namespace xoceanus {

//==============================================================================
// TrailPoint
//==============================================================================

struct TrailPoint
{
    float  x         = 0.0f;  // normalized 0.0–1.0
    float  y         = 0.0f;  // normalized 0.0–1.0
    float  velocity  = 0.0f;  // clamped 0.0–1.0
    double timestamp = 0.0;   // seconds since plugin load (monotonic)
};

//==============================================================================
// GestureTrailBuffer
//==============================================================================

class GestureTrailBuffer
{
public:
    static constexpr int kBufferSize = 256;
    static constexpr int kMask       = kBufferSize - 1;

    GestureTrailBuffer() = default;

    //--------------------------------------------------------------------------
    // Ring buffer interface
    //--------------------------------------------------------------------------

    /** Add a point. velocity is clamped to [0, 1]. */
    void push(float x, float y, float velocity, double timestamp)
    {
        TrailPoint pt;
        pt.x         = x;
        pt.y         = y;
        pt.velocity  = std::max(0.0f, std::min(1.0f, velocity));
        pt.timestamp = timestamp;

        buffer_[head_] = pt;
        head_ = (head_ + 1) & kMask;

        if (count_ < kBufferSize)
            ++count_;
    }

    /** Number of valid points (0–256). */
    int count() const { return count_; }

    /** Oldest valid point. Undefined if count == 0. */
    TrailPoint oldest() const
    {
        int idx = (head_ - count_ + kBufferSize) & kMask;
        return buffer_[idx];
    }

    /** Newest valid point. Undefined if count == 0. */
    TrailPoint newest() const
    {
        int idx = (head_ - 1 + kBufferSize) & kMask;
        return buffer_[idx];
    }

    /**
     * Point by age: 0 = newest, count-1 = oldest.
     * Clamps age to [0, count-1] if out of range.
     */
    TrailPoint pointByAge(int age) const
    {
        if (count_ == 0) return TrailPoint{};
        age = std::max(0, std::min(age, count_ - 1));
        int idx = (head_ - 1 - age + kBufferSize * 2) & kMask;
        return buffer_[idx];
    }

    /** Reset count to 0 and unfreeze. */
    void clear()
    {
        count_    = 0;
        head_     = 0;
        isFrozen_ = false;
        frozenCount_ = 0;
    }

    //--------------------------------------------------------------------------
    // Freeze / replay (B043 Section 5.2)
    //--------------------------------------------------------------------------

    /** Copy live buffer to frozen snapshot and set frozen flag. */
    void freeze()
    {
        frozenBuffer_ = buffer_;
        frozenHead_   = head_;
        frozenCount_  = count_;
        isFrozen_     = true;
    }

    /** Clear the frozen flag (live trail continues to update). */
    void unfreeze()
    {
        isFrozen_ = false;
    }

    bool isFrozen() const { return isFrozen_; }

    int frozenCount() const { return frozenCount_; }

    /**
     * Point in frozen buffer at index (0 = oldest in frozen snapshot).
     * Returns empty TrailPoint if no frozen data or index out of range.
     */
    TrailPoint frozenPointAt(int index) const
    {
        if (frozenCount_ == 0 || index < 0 || index >= frozenCount_)
            return TrailPoint{};

        int idx = (frozenHead_ - frozenCount_ + index + kBufferSize * 2) & kMask;
        return frozenBuffer_[idx];
    }

    /**
     * Replay frozen buffer at normalizedTime in [0, 1].
     * Loops continuously using fmod; 0.0 = oldest point, approaching 1.0 =
     * newest point (wraps back to oldest at exactly 1.0).
     * Returns empty TrailPoint if frozen buffer is empty.
     */
    TrailPoint replayAt(float normalizedTime) const
    {
        if (frozenCount_ == 0) return TrailPoint{};

        // Wrap t to [0, 1)
        float t = normalizedTime - std::floor(normalizedTime);

        // Map to index in [0, frozenCount_)
        int index = static_cast<int>(t * static_cast<float>(frozenCount_));
        // Clamp to valid range (edge case when t is exactly 1.0 after wrap)
        index = std::max(0, std::min(index, frozenCount_ - 1));

        return frozenPointAt(index);
    }

    //--------------------------------------------------------------------------
    // Two-trail interference (B043 Section 5.3)
    //--------------------------------------------------------------------------

    /**
     * Compute interference between two trail points.
     * Returns {x, y}:
     *   x = clamp(a.x + b.x - 0.5, 0, 1)   — summed and clamped
     *   y = a.y * b.y                        — multiplied
     */
    static std::pair<float, float> interference(TrailPoint a, TrailPoint b)
    {
        float x = a.x + b.x - 0.5f;
        x = std::max(0.0f, std::min(1.0f, x));
        float y = a.y * b.y;
        return { x, y };
    }

private:
    std::array<TrailPoint, kBufferSize> buffer_{};
    int head_  = 0;
    int count_ = 0;

    // Frozen snapshot
    std::array<TrailPoint, kBufferSize> frozenBuffer_{};
    int  frozenHead_  = 0;
    int  frozenCount_ = 0;
    bool isFrozen_    = false;
};

} // namespace xoceanus
