// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
// CreatureBehaviorTimer.h
// XOceanus — JUCE timer that polls OBRIX parameters and drives reef creature
// states via ReefBridge.
//
// Design:
//   Runs at 30 Hz on the JUCE message thread (juce::Timer guarantees this).
//   Reads normalised parameter values through a ParamReader functor so it
//   stays decoupled from the APVTS and parameter layout.
//   Evaluates per-creature thresholds to determine the target CreatureState.
//   Sends state transitions to reef_bridge::setCreatureState which dispatches
//   internally to the main thread — safe from timerCallback.
//
// Threading:
//   juce::Timer callbacks are guaranteed on the JUCE message thread.
//   reef_bridge::setCreatureState is also safe to call from any thread
//   because it dispatches via dispatch_async(main_queue).
//   The std::atomic<bool> noteActive_ allows the audio thread to signal
//   note-on state without a lock.
//
// Usage:
//   CreatureBehaviorTimer timer;
//   timer.configure(
//       [&apvts](int idx) { return apvts.getParameter(paramIds[idx])->getValue(); },
//       [this]() { return noteActive.load(); }
//   );
//   timer.addCreature({ 0, 3, 0.40f, 0.65f, 0.90f });  // creature 0 tracks param[3]
//   timer.addCreature({ 1, 7, 0.35f, 0.60f, 0.85f });  // creature 1 tracks param[7]
//   timer.start();
//
//   // From audio thread when a note starts:
//   timer.setNoteActive(true);
//
//   // In editor destructor:
//   timer.stop();
//
#include <juce_core/juce_core.h>
#include "ReefBridge.h"
#include <array>
#include <atomic>
#include <functional>

namespace xoceanus
{

//==============================================================================
// CreatureDriver — Specifies how a single creature maps to a parameter.
//
struct CreatureDriver
{
    int creatureId = 0;
    int paramIndex = 0; // Index passed to ParamReader

    // State thresholds — normalised 0.0–1.0 parameter values.
    // Values below idleThreshold    → Idle (after wake-up from sleep)
    // Values in [curious, excited)  → Curious
    // Values >= excitedThreshold    → Excited
    // Note active                   → Singing (overrides all)
    float idleThreshold = 0.40f;    // 0.00 – 0.40
    float curiousThreshold = 0.65f; // 0.40 – 0.65
    float excitedThreshold = 0.90f; // 0.65 – 0.90
    // excitedThreshold and above  = Excited

    // Runtime state — do not set manually; managed by timerCallback.
    reef_bridge::CreatureState currentState = reef_bridge::CreatureState::Sleeping;
    int64_t lastActivityMs = 0;
};

//==============================================================================
// CreatureBehaviorTimer
//
class CreatureBehaviorTimer : public juce::Timer
{
public:
    // Maximum creatures tracked simultaneously.
    static constexpr int kMaxCreatures = 32;

    // Milliseconds of param inactivity before a creature goes to sleep.
    static constexpr int kIdleTimeoutMs = 45000;

    // -------------------------------------------------------------------------
    // ParamReader: (paramIndex) → normalised value in [0.0, 1.0]
    // NoteActiveReader: () → true if any note is currently sounding
    //
    // Both functors are called on the JUCE message thread — they must not
    // block or perform I/O.  For APVTS parameters, getValue() is safe.
    // For note state, use an atomic set by the audio callback.
    //
    using ParamReader = std::function<float(int)>;
    using NoteActiveReader = std::function<bool()>;

    // -------------------------------------------------------------------------
    // configure() — Set the reader functions before calling start().
    // May be called again to hot-swap readers (e.g. on engine change).
    //
    void configure(ParamReader paramReader, NoteActiveReader noteReader)
    {
        paramReader_ = std::move(paramReader);
        noteReader_ = std::move(noteReader);
    }

    // -------------------------------------------------------------------------
    // addCreature() — Register a creature driver.
    // Call before start(), or at any time while the timer is running.
    // The driver is appended; the timer acquires its own copy.
    //
    void addCreature(const CreatureDriver& driver)
    {
        if (numCreatures_ < kMaxCreatures)
            creatures_[numCreatures_++] = driver;
    }

    // -------------------------------------------------------------------------
    // clearCreatures() — Remove all registered creature drivers.
    // Call when switching engines or rebuilding the reef layout.
    //
    void clearCreatures() { numCreatures_ = 0; }

    // -------------------------------------------------------------------------
    // setNoteActive() — Notify from the audio thread that a note started or
    // ended. The timer reads this atomically in the next tick.
    //
    void setNoteActive(bool active) { noteActiveOverride_.store(active, std::memory_order_release); }

    // -------------------------------------------------------------------------
    // start() / stop() — Timer lifecycle.
    //
    void start() { startTimerHz(30); }
    void stop() { stopTimer(); }

    // -------------------------------------------------------------------------
    // timerCallback() — Called at 30 Hz on the JUCE message thread.
    //
    void timerCallback() override
    {
        const int64_t now = juce::Time::currentTimeMillis();
        const bool noteActive = evaluateNoteActive();

        for (int i = 0; i < numCreatures_; ++i)
        {
            CreatureDriver& c = creatures_[i];
            reef_bridge::CreatureState newState;

            if (noteActive)
            {
                // SINGING override: note-on trumps all param-driven states.
                newState = reef_bridge::CreatureState::Singing;
                c.lastActivityMs = now;
            }
            else
            {
                const float paramValue = readParam(c.paramIndex);

                // Any non-trivial param movement resets the idle timeout.
                if (paramValue > 0.001f)
                    c.lastActivityMs = now;

                if ((now - c.lastActivityMs) > kIdleTimeoutMs)
                {
                    newState = reef_bridge::CreatureState::Sleeping;
                }
                else if (paramValue >= c.excitedThreshold)
                {
                    newState = reef_bridge::CreatureState::Excited;
                }
                else if (paramValue >= c.curiousThreshold)
                {
                    newState = reef_bridge::CreatureState::Curious;
                }
                else
                {
                    newState = reef_bridge::CreatureState::Idle;
                }
            }

            // Only forward state changes — no unnecessary bridge calls.
            if (newState != c.currentState)
            {
                c.currentState = newState;
                reef_bridge::setCreatureState(c.creatureId, newState);
            }
        }
    }

private:
    // -------------------------------------------------------------------------
    // Evaluate note-active: prefer the explicit atomic override if set by the
    // audio thread, otherwise fall back to the NoteActiveReader functor.
    //
    bool evaluateNoteActive() const
    {
        // The atomic value is authoritative when set from the audio thread.
        if (noteActiveOverride_.load(std::memory_order_acquire))
            return true;

        if (noteReader_)
            return noteReader_();

        return false;
    }

    float readParam(int index) const
    {
        if (paramReader_)
        {
            const float v = paramReader_(index);
            // Guard against out-of-range values from a newly-swapped reader.
            return (v < 0.0f) ? 0.0f : (v > 1.0f) ? 1.0f : v;
        }
        return 0.0f;
    }

    // -------------------------------------------------------------------------
    // Data
    //
    std::array<CreatureDriver, kMaxCreatures> creatures_{};
    int numCreatures_ = 0;
    ParamReader paramReader_;
    NoteActiveReader noteReader_;
    std::atomic<bool> noteActiveOverride_{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CreatureBehaviorTimer)
};

} // namespace xoceanus
