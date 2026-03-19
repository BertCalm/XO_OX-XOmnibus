#pragma once
#include <atomic>
#include <cstdint>

namespace xomnibus {

//==============================================================================
// OscarAnimState — lock-free atomic bridge between the MorphEngine (audio
// thread) and OscarRiveComponent (UI thread).
//
// The MorphEngine writes these values during renderBlock(). The UI timer
// reads them at 60 Hz. No locks, no allocation, no cross-thread contention.
//
// Layout mirrors OpticModOutputs in OpticVisualizer — one struct of atomics,
// one raw pointer held by both owner and observer. The engine owns the struct.
//==============================================================================
struct OscarAnimState
{
    //--------------------------------------------------------------------------
    // Written by MorphEngine on the audio thread
    //--------------------------------------------------------------------------

    /// Gill oscillation speed — maps to Rive input "gillSpeed".
    /// Derived from morph_drift parameter + Perlin noise phase velocity.
    /// Range 0.0 (barely breathing) to 1.0 (excited / fast flutter).
    std::atomic<float> gillSpeed { 0.25f };

    /// Wavetable morph position — maps to Rive input "morphPosition".
    /// 0.0 = sine (rest), 1.0 = saw (regenerating), 2.0 = square (display),
    /// 3.0 = noise (dissolved into reef).
    std::atomic<float> morphPosition { 0.0f };

    /// True during the sustain phase of any active voice.
    /// Rising edge → state machine "noteOn" trigger.
    std::atomic<bool> voiceActive { false };

    /// Peak amplitude of the current block (post-filter, pre-clip).
    /// Used to drive subtle body-swell on loud chords.
    /// Range 0.0–1.0.
    std::atomic<float> outputLevel { 0.0f };

    //--------------------------------------------------------------------------
    // Written by the game/level layer (main thread)
    //--------------------------------------------------------------------------

    /// 0 = hatchling, 1 = juvenile, 2 = adolescent, 3 = adult, 4 = ancient,
    /// 5 = elder. Maps to Rive blend state "evolutionLevel".
    std::atomic<int> evolutionLevel { 0 };

    /// Triggers CELEBRATION state for ~3 seconds. Set to true on level clear,
    /// OscarRiveComponent resets to false after firing the trigger.
    std::atomic<bool> celebrationTrigger { false };

    /// Triggers ALERT state. Held true while boss phase is active.
    std::atomic<bool> bossMode { false };

    //--------------------------------------------------------------------------
    // Helpers for the UI thread
    //--------------------------------------------------------------------------

    /// Snapshot all values in one consistent read sweep.
    /// The UI timer calls this once per frame; tolerates torn reads since
    /// none of these values require atomicity across fields.
    struct Snapshot
    {
        float gillSpeed      = 0.25f;
        float morphPosition  = 0.0f;
        bool  voiceActive    = false;
        float outputLevel    = 0.0f;
        int   evolutionLevel = 0;
        bool  celebration    = false;
        bool  bossMode       = false;
    };

    Snapshot snapshot() const
    {
        Snapshot s;
        s.gillSpeed      = gillSpeed.load (std::memory_order_relaxed);
        s.morphPosition  = morphPosition.load (std::memory_order_relaxed);
        s.voiceActive    = voiceActive.load (std::memory_order_relaxed);
        s.outputLevel    = outputLevel.load (std::memory_order_relaxed);
        s.evolutionLevel = evolutionLevel.load (std::memory_order_relaxed);
        s.celebration    = celebrationTrigger.load (std::memory_order_relaxed);
        s.bossMode       = bossMode.load (std::memory_order_relaxed);
        return s;
    }
};

} // namespace xomnibus
