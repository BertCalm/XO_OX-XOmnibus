// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "OperaConstants.h"
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace opera
{

//==============================================================================
// OperaConductor — Autonomous dramatic arc system for XOpera.
//
// Drives coupling strength K through shaped temporal arcs, triggered by
// note-on events or MIDI CC 20. Four arc shapes: Linear (triangle ramp),
// S-Curve (hermite sigmoid with asymmetric peak), Double-Peak (two-act
// dramatic structure), and Random (Catmull-Rom through seeded control points).
//
// Per-arc randomization adds +/-5% timing jitter and +/-3% peak jitter
// to prevent mechanical repetition. Manual override (arcMode=Both) uses
// max(conductorK, manualK) for seamless player takeover.
//
// All methods are noexcept and allocation-free for real-time safety.
//
// Usage:
//   OperaConductor conductor;
//   conductor.prepare(sampleRate);
//   conductor.setArcShape(1);    // S-Curve
//   conductor.setArcTime(8.0f);  // 8 seconds
//   conductor.setArcPeak(0.8f);  // 80% max coupling
//   conductor.setArcMode(1);     // Conductor mode
//   conductor.trigger();
//
//   // Per-sample in processBlock:
//   float k = conductor.processSample();
//
//==============================================================================
class OperaConductor
{
public:
    //--------------------------------------------------------------------------
    // Arc shape enum — matches opera_arcShape parameter values (0-3).
    //--------------------------------------------------------------------------
    enum ArcShape : int
    {
        kLinear = 0,
        kSCurve = 1,
        kDoublePeak = 2,
        kRandom = 3
    };

    //--------------------------------------------------------------------------
    // Arc mode enum — matches opera_arcMode parameter values (0-2).
    //--------------------------------------------------------------------------
    enum ArcMode : int
    {
        kManual = 0,
        kConductor = 1,
        kBoth = 2
    };

    //--------------------------------------------------------------------------
    // Lifecycle
    //--------------------------------------------------------------------------

    /// Prepare for playback. Must be called before processSample().
    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        reset();
    }

    /// Reset conductor to idle state. Safe to call from any thread.
    void reset() noexcept
    {
        phase = Phase::Idle;
        arcPosition = 0.0f;
        arcIncrement = 0.0f;
        currentIntensity = 0.0f;
        arcSeed = 0;
        effectiveTime = arcTimeSec;
        effectivePeak = arcPeakVal;
        rngState = 0x12345678u;
    }

    //--------------------------------------------------------------------------
    // Parameter setters — call per-block from parameter snapshot.
    //--------------------------------------------------------------------------

    /// Set arc shape (0=Linear, 1=S-Curve, 2=Double-Peak, 3=Random).
    void setArcShape(int shape) noexcept { arcShape = std::clamp(shape, 0, 3); }

    /// Set arc duration in seconds (clamped to 0.5-3600.0s).
    void setArcTime(float seconds) noexcept
    {
        arcTimeSec = std::clamp(seconds, 0.5f, 3600.0f); // 1-hour max for Schulze-scale arcs
    }

    /// Set arc peak intensity (0.0-1.0, maximum coupling K at climax).
    void setArcPeak(float peak) noexcept { arcPeakVal = std::clamp(peak, 0.0f, 1.0f); }

    /// Set arc mode (0=Manual, 1=Conductor, 2=Both).
    void setArcMode(int mode) noexcept { arcMode = std::clamp(mode, 0, 2); }

    //--------------------------------------------------------------------------
    // Trigger / Stop
    //--------------------------------------------------------------------------

    /// Start a new arc. Resets position to 0 and computes per-sample increment
    /// with per-arc jitter for natural variation.
    void trigger() noexcept
    {
        // Per-arc randomization: +/-5% timing, +/-3% peak
        float timingJitter = 1.0f + (nextRandom() * 0.1f - 0.05f);
        float peakJitter = 1.0f + (nextRandom() * 0.06f - 0.03f);

        effectiveTime = arcTimeSec * timingJitter;
        effectivePeak = std::clamp(arcPeakVal * peakJitter, 0.0f, 1.0f);

        phase = Phase::Running;
        arcPosition = 0.0f;
        currentIntensity = 0.0f;

        // Per-sample increment: advance by 1/(time * sampleRate) each sample
        if (sr > 0.0f && effectiveTime > 0.0f)
            arcIncrement = 1.0f / (effectiveTime * sr);
        else
            arcIncrement = 0.0f;

        // Seed for Random shape — deterministic within this arc, different
        // across arcs due to RNG state progression.
        arcSeed = rngState ^ static_cast<uint32_t>(effectiveTime * 1000.0f) ^ 0xDEADBEEFu;
    }

    /// Abort the current arc immediately. Output drops to 0.
    void stop() noexcept
    {
        phase = Phase::Idle;
        arcPosition = 0.0f;
        arcIncrement = 0.0f;
        currentIntensity = 0.0f;
    }

    //--------------------------------------------------------------------------
    // Per-sample processing
    //--------------------------------------------------------------------------

    /// Returns the conductor's K contribution for this sample [0, arcPeak].
    /// Must be called once per sample for correct timing.
    float processSample() noexcept
    {
        if (phase == Phase::Idle)
            return 0.0f;

        // Evaluate the arc shape at the current position
        float rawIntensity = 0.0f;

        switch (arcShape)
        {
        case kLinear:
            rawIntensity = shapeLinear(arcPosition);
            break;
        case kSCurve:
            rawIntensity = shapeSCurve(arcPosition);
            break;
        case kDoublePeak:
            rawIntensity = shapeDoublePeak(arcPosition);
            break;
        case kRandom:
            rawIntensity = shapeRandom(arcPosition, arcSeed);
            break;
        default:
            rawIntensity = shapeLinear(arcPosition);
            break;
        }

        // Clamp raw intensity to [0, 1] — Catmull-Rom can overshoot
        rawIntensity = std::clamp(rawIntensity, 0.0f, 1.0f);

        // Scale by effective peak (includes per-arc jitter)
        currentIntensity = rawIntensity * effectivePeak;

        // Advance arc position
        arcPosition += arcIncrement;

        if (arcPosition >= 1.0f)
        {
            // Arc complete — hold at 0 until next trigger
            phase = Phase::Idle;
            arcPosition = 0.0f;
            currentIntensity = 0.0f;
            return 0.0f;
        }

        return currentIntensity;
    }

    //--------------------------------------------------------------------------
    // Queries
    //--------------------------------------------------------------------------

    /// True if an arc is currently running.
    bool isActive() const noexcept { return phase == Phase::Running; }

    /// Normalized arc position [0, 1]. Returns 0 when idle.
    float getProgress() const noexcept { return arcPosition; }

    /// Current intensity output [0, arcPeak]. For UI display.
    float getIntensity() const noexcept { return currentIntensity; }

    /// Current arc mode. Useful for the processor's K computation.
    int getArcMode() const noexcept { return arcMode; }

    //--------------------------------------------------------------------------
    // Manual override helper — call from processor's per-sample loop.
    //
    // When arcMode == Both (2), returns max(conductorK, manualK).
    // When arcMode == Manual (0), returns manualK.
    // When arcMode == Conductor (1), returns conductorK.
    //--------------------------------------------------------------------------
    float computeEffectiveK(float manualK) noexcept
    {
        float conductorK = processSample();

        switch (arcMode)
        {
        case kManual:
            return manualK;
        case kConductor:
            return conductorK;
        case kBoth:
            return std::max(conductorK, manualK);
        default:
            return manualK;
        }
    }

private:
    //--------------------------------------------------------------------------
    // Phase state machine
    //--------------------------------------------------------------------------
    enum class Phase
    {
        Idle,
        Running
    };

    //--------------------------------------------------------------------------
    // Arc shape functions — all map normalized t [0,1] to intensity [0,1].
    //--------------------------------------------------------------------------

    /// Hermite smoothstep: 3t^2 - 2t^3
    static float smoothstep(float t) noexcept
    {
        t = std::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    /// Shape 0 — Linear: triangle ramp, peak at t=0.5.
    static float shapeLinear(float t) noexcept { return (t < 0.5f) ? (t * 2.0f) : (2.0f - t * 2.0f); }

    /// Shape 1 — S-Curve: modified sigmoid with peak at t=0.6.
    /// Build takes longer than resolution — natural dramatic shape.
    static float shapeSCurve(float t) noexcept
    {
        constexpr float peakT = 0.6f;

        if (t < peakT)
        {
            float x = t / peakT;
            return smoothstep(x);
        }
        else
        {
            float x = (t - peakT) / (1.0f - peakT);
            return 1.0f - smoothstep(x);
        }
    }

    /// Shape 2 — Double-Peak: two-act dramatic structure.
    /// Act 1: build to 80% at t=0.3
    /// Intermission: dip to 30% at t=0.5
    /// Act 2: build to 100% at t=0.75
    /// Resolution: fade to 0% at t=1.0
    static float shapeDoublePeak(float t) noexcept
    {
        if (t < 0.3f)
            return smoothstep(t / 0.3f) * 0.8f;
        else if (t < 0.5f)
            return 0.8f - smoothstep((t - 0.3f) / 0.2f) * 0.5f; // 0.8 -> 0.3
        else if (t < 0.75f)
            return 0.3f + smoothstep((t - 0.5f) / 0.25f) * 0.7f; // 0.3 -> 1.0
        else
            return 1.0f - smoothstep((t - 0.75f) / 0.25f); // 1.0 -> 0.0
    }

    /// Shape 3 — Random: Catmull-Rom spline through 5 seeded control points.
    /// Points[0] = 0 (start), Points[4] = 0 (end), 3 interior points random.
    /// Seed-deterministic: same arcSeed produces the same arc shape.
    static float shapeRandom(float t, uint32_t seed) noexcept
    {
        // Generate 5 control points: fixed endpoints, 3 random interior
        float cp[5];
        uint32_t rng = seed;

        cp[0] = 0.0f; // always start at 0
        for (int i = 1; i < 4; ++i)
        {
            rng = rng * 1664525u + 1013904223u; // Knuth LCG
            cp[i] = static_cast<float>(rng & 0xFFFFu) / 65536.0f;
        }
        cp[4] = 0.0f; // always end at 0

        // Map t [0,1] to 4 spline segments
        float segment = t * 4.0f;
        int idx = static_cast<int>(segment);
        if (idx > 3)
            idx = 3;
        float frac = segment - static_cast<float>(idx);

        // Catmull-Rom with clamped boundary indices
        float p0 = cp[clampIdx(idx - 1)];
        float p1 = cp[idx];
        float p2 = cp[clampIdx(idx + 1)];
        float p3 = cp[clampIdx(idx + 2)];

        // Catmull-Rom cubic evaluation
        float f2 = frac * frac;
        float f3 = f2 * frac;

        return 0.5f * ((2.0f * p1) + (-p0 + p2) * frac + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * f2 +
                       (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * f3);
    }

    /// Clamp spline index to valid control point range [0, 4].
    static int clampIdx(int i) noexcept { return (i < 0) ? 0 : (i > 4) ? 4 : i; }

    //--------------------------------------------------------------------------
    // Lightweight LCG RNG — used for per-arc jitter. No allocation.
    // Knuth TAOCP LCG: period 2^32, sufficient for jitter values.
    //--------------------------------------------------------------------------
    float nextRandom() noexcept
    {
        rngState = rngState * 1664525u + 1013904223u;
        return static_cast<float>(rngState & 0xFFFFu) / 65536.0f;
    }

    //--------------------------------------------------------------------------
    // State
    //--------------------------------------------------------------------------
    float sr = 44100.0f; // sample rate (updated by prepare())

    // Parameters (set per-block by the processor)
    int arcShape = kSCurve;  // default: S-Curve (natural dramatic shape)
    float arcTimeSec = 8.0f; // default: 8s (one phrase)
    float arcPeakVal = 0.8f; // default: 80% max coupling
    int arcMode = kManual;   // default: Manual

    // Per-arc effective values (include jitter)
    float effectiveTime = 8.0f;
    float effectivePeak = 0.8f;

    // Phase tracking
    Phase phase = Phase::Idle;
    float arcPosition = 0.0f;      // normalized [0, 1]
    float arcIncrement = 0.0f;     // per-sample advance
    float currentIntensity = 0.0f; // current output [0, arcPeak]

    // RNG state — persists across arcs for non-repeating jitter sequences.
    // arcSeed is derived from this per-trigger for the Random shape.
    uint32_t rngState = 0x12345678u;
    uint32_t arcSeed = 0;
};

} // namespace opera
