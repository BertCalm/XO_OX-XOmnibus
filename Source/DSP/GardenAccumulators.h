// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  GardenAccumulators.h — Shared Evolutionary State for the GARDEN Quad
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  Implements the three accumulator dimensions from the Visionary concept:
//    W (Warmth)    — leaky integrator, rises with sustained playing
//    A (Aggression) — rises with velocity/density, square-root decay
//    D (Dormancy)   — rises during silence, resets on note-on
//
//  Plus the Session Season state machine (Spring/Summer/Fall/Winter) and
//  the event-queue Mycorrhizal Network for cross-engine state propagation.
//
//  CPU cost: ~0.03% (block-rate accumulators + event scan). Memory: ~7.2 KB.
//  Decision G1 from kitchen-cpu-optimization-strategy.md.
//
//==============================================================================

#include "FastMath.h"
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <array>

namespace xoceanus {

//==============================================================================
// Session Season — timbral phenological state
//==============================================================================
enum class Season : int
{
    Spring = 0,   // Reawakening — slightly uneven, finding grip
    Summer = 1,   // Peak expression — lush, warm, deep
    Fall   = 2,   // Rich but strained — deeper tones, some roll-off
    Winter = 3    // Dormant — slow, cold, interior
};

//==============================================================================
// GardenAccumulators — W/A/D state + Season derivation
//
// Updated once per block (not per-sample). All time constants are in seconds.
// The accumulators are session-persistent: they survive note-on/off cycles
// and only reset on explicit user action (preset change / engine reset).
//==============================================================================
struct GardenAccumulators
{
    //--------------------------------------------------------------------------
    // State — public for cross-engine reading and UI display
    //--------------------------------------------------------------------------
    float W = 0.0f;       // Warmth [0, 1]
    float A = 0.0f;       // Aggression [0, 1]
    float D = 0.3f;       // Dormancy [0, 1] — starts above zero (cold start)
    Season season = Season::Spring;

    // Session time tracking
    double sessionTime = 0.0;   // seconds since prepare()
    double lastNoteTime = 0.0;  // seconds of last note-on

    //--------------------------------------------------------------------------
    // Configuration — set once per engine, tuned per-engine character
    //--------------------------------------------------------------------------
    float wFloor       = 0.05f;   // Warmth never decays below this
    float wRiseRate     = 0.002f;  // How fast W rises per note (velocity-scaled)
    float wDecayRate    = 0.0005f; // How fast W decays toward floor during silence
    float aThreshold    = 0.4f;    // Velocity threshold for aggression accumulation
    float aRiseRate     = 0.003f;  // How fast A rises
    float aDecayRate    = 0.001f;  // Base decay rate (applied via sqrt curve)
    float dRiseRate     = 0.0008f; // How fast D accumulates during silence
    float dDecayRate    = 0.01f;   // How fast D decays on note activity

    //--------------------------------------------------------------------------
    // Per-block update — call once per renderBlock()
    //--------------------------------------------------------------------------

    /// Update accumulators with current block's activity.
    /// @param blockSizeSec  Block duration in seconds (numSamples / sampleRate)
    /// @param activeVoices  Number of currently sounding voices
    /// @param avgVelocity   Average velocity of active voices [0, 1]
    /// @param noteOnCount   Number of note-ons in this block
    void update (float blockSizeSec, int activeVoices, float avgVelocity,
                 int noteOnCount) noexcept
    {
        sessionTime += blockSizeSec;
        bool hasActivity = activeVoices > 0;

        //-- Warmth (W) --
        if (hasActivity)
        {
            float voiceFactor = static_cast<float> (activeVoices) * 0.25f;
            W += wRiseRate * avgVelocity * voiceFactor * blockSizeSec * 60.0f;
        }
        else
        {
            W -= wDecayRate * (W - wFloor) * blockSizeSec * 60.0f;
        }
        W = std::clamp (W, wFloor, 1.0f);

        //-- Aggression (A) --
        if (hasActivity && avgVelocity > aThreshold)
        {
            float excess = avgVelocity - aThreshold;
            A += aRiseRate * excess * excess * blockSizeSec * 60.0f;
        }
        else
        {
            // Square-root decay — stress lingers, then releases
            float silenceSec = static_cast<float> (sessionTime - lastNoteTime);
            float sqrtDecay = std::sqrt (std::max (silenceSec, 0.01f));
            A -= aDecayRate * sqrtDecay * blockSizeSec * 60.0f;
        }
        A = std::clamp (A, 0.0f, 1.0f);

        //-- Dormancy (D) --
        if (hasActivity)
        {
            D -= dDecayRate * static_cast<float> (activeVoices) * blockSizeSec * 60.0f;
        }
        else
        {
            float silenceSec = static_cast<float> (sessionTime - lastNoteTime);
            D += dRiseRate * silenceSec * blockSizeSec * 60.0f;
        }
        D = std::clamp (D, 0.0f, 1.0f);

        //-- Note-on tracking --
        if (noteOnCount > 0)
            lastNoteTime = sessionTime;

        //-- Season derivation --
        updateSeason();
    }

    //--------------------------------------------------------------------------
    // Season state machine
    //--------------------------------------------------------------------------
    void updateSeason() noexcept
    {
        // High W, low A, low D = Summer (peak expression)
        if (W > 0.5f && A < 0.4f && D < 0.3f)
            season = Season::Summer;
        // High A over extended time, or W saturation = Fall
        else if (A > 0.5f || (W > 0.8f && A > 0.3f))
            season = Season::Fall;
        // High D = Winter
        else if (D > 0.5f)
            season = Season::Winter;
        // Default = Spring
        else
            season = Season::Spring;
    }

    //--------------------------------------------------------------------------
    // Timbral modifiers derived from accumulator state.
    // These are the DSP-facing outputs that engines read per block.
    //--------------------------------------------------------------------------

    /// High-frequency roll-off factor [0, 1]. Higher W = more roll-off.
    float getWarmthRolloff() const noexcept { return W * 0.6f; }

    /// Inter-voice phase variance reduction [0, 1]. Higher W = more lock.
    float getWarmthLockIn() const noexcept { return W * 0.4f; }

    /// Vibrato irregularity [0, 1]. Higher A = more irregular.
    float getAggressionVibrato() const noexcept { return A * 0.5f; }

    /// Bow pressure / transient harshness [0, 1]. Higher A = more harsh.
    float getAggressionHarshness() const noexcept { return A * 0.4f; }

    /// Initial attack noise amount [0, 1]. Higher D = more stiff attack.
    float getDormancyAttackNoise() const noexcept { return D * 0.3f; }

    /// Initial pitch variance in cents. Higher D = wider variance.
    float getDormancyPitchVariance() const noexcept { return D * 8.0f; }

    /// Season-based brightness modifier [-1, +1].
    float getSeasonBrightness() const noexcept
    {
        switch (season)
        {
            case Season::Spring: return 0.3f;   // bright, new
            case Season::Summer: return 0.0f;    // full, neutral
            case Season::Fall:   return -0.2f;   // warm, rolled off
            case Season::Winter: return -0.5f;   // cold, dark
        }
        return 0.0f;
    }

    //--------------------------------------------------------------------------
    // Reset — only on explicit user action (preset change, engine swap)
    //--------------------------------------------------------------------------
    void reset() noexcept
    {
        W = 0.0f;
        A = 0.0f;
        D = 0.3f;
        season = Season::Spring;
        sessionTime = 0.0;
        lastNoteTime = 0.0;
    }
};

//==============================================================================
// MycorrhizalEvent — a timed scalar message in the fungal network
//==============================================================================
struct MycorrhizalEvent
{
    float value = 0.0f;
    double deliveryTime = 0.0;
};

//==============================================================================
// MycorrhizalChannel — one directional connection between two entities.
// Event-queue based (Decision G1): 32-event circular buffer per channel.
// Memory: 32 * 12 bytes = 384 bytes per channel.
//==============================================================================
struct MycorrhizalChannel
{
    static constexpr int kMaxEvents = 32;

    void configure (float conductanceVal, float delaySec) noexcept
    {
        conductance = conductanceVal;
        delaySeconds = delaySec;
    }

    void send (float stressValue, double currentTime) noexcept
    {
        if (std::fabs (stressValue) < 1e-6f) return;
        events[writeIdx] = { stressValue * conductance,
                             currentTime + delaySeconds };
        writeIdx = (writeIdx + 1) & (kMaxEvents - 1);
        if (count < kMaxEvents) count++;
    }

    float receive (double currentTime) noexcept
    {
        float total = 0.0f;
        for (int i = 0; i < count; ++i)
        {
            if (events[i].deliveryTime <= currentTime && events[i].value != 0.0f)
            {
                total += events[i].value;
                events[i].value = 0.0f;  // consumed
            }
        }
        return total;
    }

    void reset() noexcept
    {
        for (auto& e : events) e = {};
        writeIdx = 0;
        count = 0;
    }

    float conductance = 0.5f;
    float delaySeconds = 4.0f;

private:
    std::array<MycorrhizalEvent, kMaxEvents> events {};
    int writeIdx = 0;
    int count = 0;
};

//==============================================================================
// GardenMycorrhizalNetwork — 6 channels for 4-voice network (Decision G2).
// Each voice pair has one bidirectional connection = 2 unidirectional channels.
// C(4,2) = 6 unique pairs. With asymmetric response, we need 12 channels
// (6 forward + 6 reverse), but for V1 we use 6 symmetric channels.
//==============================================================================
struct GardenMycorrhizalNetwork
{
    static constexpr int kNumChannels = 6;

    void configure (float conductance, float delaySec) noexcept
    {
        for (auto& ch : channels)
            ch.configure (conductance, delaySec);
    }

    /// Send stress from voice `from` to all connected voices.
    void sendStress (int fromVoice, float stressValue, double currentTime) noexcept
    {
        for (int i = 0; i < kNumChannels; ++i)
        {
            // Channel i connects a specific pair; broadcast from fromVoice
            // to all channels that touch it. For 4 voices, the mapping is:
            // ch0: 0↔1, ch1: 0↔2, ch2: 0↔3, ch3: 1↔2, ch4: 1↔3, ch5: 2↔3
            int a = channelPairA (i);
            int b = channelPairB (i);
            if (a == fromVoice || b == fromVoice)
                channels[i].send (stressValue, currentTime);
        }
    }

    /// Receive accumulated stress for voice `toVoice`.
    float receiveStress (int toVoice, double currentTime) noexcept
    {
        float total = 0.0f;
        for (int i = 0; i < kNumChannels; ++i)
        {
            int a = channelPairA (i);
            int b = channelPairB (i);
            if (a == toVoice || b == toVoice)
                total += channels[i].receive (currentTime);
        }
        return total;
    }

    void reset() noexcept
    {
        for (auto& ch : channels) ch.reset();
    }

private:
    std::array<MycorrhizalChannel, kNumChannels> channels;

    // Pair mapping for C(4,2) = 6 pairs
    static constexpr int channelPairA (int ch) noexcept
    {
        constexpr int a[] = { 0, 0, 0, 1, 1, 2 };
        return a[ch];
    }
    static constexpr int channelPairB (int ch) noexcept
    {
        constexpr int b[] = { 1, 2, 3, 2, 3, 3 };
        return b[ch];
    }
};

} // namespace xoceanus
