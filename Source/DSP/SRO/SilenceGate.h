// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <cstring>
#include <atomic>

namespace xoceanus {

//==============================================================================
// SilenceGate — Zero-idle bypass for inactive engine slots.
//
// Monitors the output level of an engine's render block. When the signal falls
// below a threshold (-90 dB by default) and stays there for a configurable
// hold time, the gate signals that the engine can be bypassed entirely.
//
// CRITICAL INTEGRATION ORDER:
//   In renderBlock(), you MUST parse MIDI and call wake() BEFORE checking
//   isBypassed(). A one-block delay between note-on and gate opening is
//   audible and unacceptable for performance use. The performer's first
//   note must never be swallowed by a sleeping gate.
//
// Usage in renderBlock():
//     // Step 1: ALWAYS parse MIDI first, even if bypassed
//     for (const auto metadata : midi)
//         if (metadata.getMessage().isNoteOn())
//             silenceGate.wake();
//
//     // Step 2: THEN check bypass
//     if (silenceGate.isBypassed() && midi.isEmpty())
//     {
//         buffer.clear();
//         return;  // zero CPU — skip all DSP
//     }
//
//     // Step 3: render audio ...
//
//     // Step 4: analyze output for silence detection
//     silenceGate.analyzeBlock(buffer.getReadPointer(0),
//                              buffer.getReadPointer(1), numSamples);
//
// The gate re-opens immediately when a note-on is received (call wake()).
//
// Hold time guidance:
//   Percussive engines (ONSET, OVERBITE, ODDFELIX): 100ms (default)
//   Standard engines (OBLONG, ODYSSEY, OBLIQUE):    200ms
//   Reverb-tail engines (OVERDUB, OPAL, OCEANIC):   500ms
//   Infinite-sustain (ORGANON, OUROBOROS):           1000ms
//
// Standby concept (future):
//   Between "fully active" and "bypassed," a standby state could maintain
//   filter state, phase positions, and envelope levels while skipping the
//   inner voice loop — ~5% of full cost but seamless re-entry without
//   filter ramp-up transients. Critical for orchestral/cinematic use.
//
// Design:
//   - Zero allocation (all state is stack/member)
//   - Lock-free (atomic bypassed flag for UI thread reads)
//   - Hysteresis: signal must exceed threshold by 6 dB to re-open
//   - Hold time prevents flutter on decaying tails
//==============================================================================
class SilenceGate
{
public:
    static constexpr float kDefaultThresholdDb = -90.0f;
    static constexpr float kHysteresisDb       =   6.0f;
    static constexpr float kDefaultHoldMs      = 100.0f;

    void prepare (double sampleRate, int /*maxBlockSize*/) noexcept
    {
        sr = sampleRate;
        setHoldTime (holdTimeMs);
        reset();
    }

    void reset() noexcept
    {
        holdCounter     = 0;
        bypassed.store (false, std::memory_order_relaxed);
        peakLevel       = 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Set the silence threshold in dB (default: -90 dB).
    void setThreshold (float thresholdDb) noexcept
    {
        thresholdLinear   = dbToLinear (thresholdDb);
        reopenLinear      = dbToLinear (thresholdDb + kHysteresisDb);
    }

    /// Set the hold time in milliseconds before bypass engages.
    void setHoldTime (float ms) noexcept
    {
        holdTimeMs = ms;
        // Convert ms to block-count assuming analyzeBlock called once per block
        // We store the hold in samples, then compare against accumulated silent samples
        holdSamples = static_cast<int> (sr * ms * 0.001);
    }

    //--------------------------------------------------------------------------
    /// Returns true if the engine should skip all processing.
    /// Safe to call from audio thread (relaxed atomic).
    bool isBypassed() const noexcept
    {
        return bypassed.load (std::memory_order_relaxed);
    }

    /// Returns the current peak level (linear) for UI display.
    float getPeakLevel() const noexcept { return peakLevel; }

    //--------------------------------------------------------------------------
    /// Analyze a rendered block to update the gate state.
    /// Call AFTER renderBlock() has written audio.
    /// @param left   Left channel buffer (or mono).
    /// @param right  Right channel buffer (nullptr for mono engines).
    /// @param numSamples  Block size.
    void analyzeBlock (const float* left, const float* right, int numSamples) noexcept
    {
        // Find peak absolute sample across both channels
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            float absL = std::fabs (left[i]);
            if (absL > peak) peak = absL;
        }
        if (right != nullptr)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                float absR = std::fabs (right[i]);
                if (absR > peak) peak = absR;
            }
        }

        peakLevel = peak;

        if (bypassed.load (std::memory_order_relaxed))
        {
            // Currently bypassed — check if signal exceeds reopen threshold
            if (peak > reopenLinear)
            {
                bypassed.store (false, std::memory_order_relaxed);
                holdCounter = 0;
            }
        }
        else
        {
            // Currently active — check if signal has fallen below threshold
            if (peak < thresholdLinear)
            {
                holdCounter += numSamples;
                if (holdCounter >= holdSamples)
                    bypassed.store (true, std::memory_order_relaxed);
            }
            else
            {
                holdCounter = 0;
            }
        }
    }

    //--------------------------------------------------------------------------
    /// Force the gate open immediately. Call on note-on, preset change, etc.
    void wake() noexcept
    {
        bypassed.store (false, std::memory_order_relaxed);
        holdCounter = 0;
    }

private:
    double sr          = 44100.0;
    float holdTimeMs   = kDefaultHoldMs;
    int holdSamples    = 4410;     // 100ms at 44.1kHz
    int holdCounter    = 0;

    float thresholdLinear = 3.16e-5f;  // -90 dB
    float reopenLinear    = 6.31e-5f;  // -84 dB (threshold + hysteresis)
    float peakLevel       = 0.0f;

    std::atomic<bool> bypassed { false };

    static float dbToLinear (float db) noexcept
    {
        return std::pow (10.0f, db / 20.0f);
    }
};

} // namespace xoceanus
