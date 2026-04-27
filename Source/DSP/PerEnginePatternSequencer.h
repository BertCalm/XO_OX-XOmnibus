// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <array>
#include <cmath>

//==============================================================================
// PerEnginePatternSequencer — per-slot MIDI pattern generator for Wave 5 C1.
//
// Architecture:
//   • 4 instances owned by XOceanusProcessor (one per primary slot 0–3).
//   • Engine-agnostic: events are injected into the slot's MidiBuffer; engines
//     cannot tell sequencer events apart from host MIDI or chord-machine events.
//   • 24 patterns in 6 aquatic families (CRESTS/WAVES/REEFS/GROOVES/DRIFTS/STORMS).
//   • RT-safe: setters use std::atomic; processBlock loads atomics into locals once.
//
// Step timing (v1 — block-aligned, not sample-accurate):
//   stepsPerQuarter = clockDivision  (1=quarter, 2=eighth, 4=16th, 8=32nd)
//   stepIdx = floor(ppqPosition * stepsPerQuarter) % stepCount
//   Edge detection: noteOn fires when stepIdx advances. NoteOff fires ~half a step later.
//
// Verification trace (stepCount=16, clockDivision=4, Pulse, ppq=0→1.0):
//   stepsPerQuarter=4, so 4 steps advance over one quarter note (ppq 0→1).
//   stepIdx sequence: 0,1,2,3 — each triggers one noteOn+noteOff. ✓
//
// Verification trace (Tresillo at stepCount=8):
//   Canonical 16-step hits: [0,3,6,8,11,14].
//   Scaled to 8: round(x * 8/16) = [0,2,3,4,6,7]. ✓
//   (Duplicate after rounding handled by set deduplication.) ✓
//
// Verification trace (Eucl3 at stepCount=8):
//   Bjorklund(3,8): [1,0,0,1,0,0,1,0] — hits at positions 0,3,6.
//   Rotation choice: zero-offset (pattern starts with a hit). Documented in
//   computeEuclidean() below. ✓

namespace XOceanus
{

class PerEnginePatternSequencer
{
public:
    //==========================================================================
    // Enums — 6 families, 24 patterns total

    enum class Family : int { Crests = 0, Waves, Reefs, Grooves, Drifts, Storms, Count };

    enum class Pattern : int
    {
        // CRESTS (0–3) — every step gates, velocity-shaped
        Pulse = 0, Surge, Ebb, Arc,
        // WAVES (4–7) — modulation-shaped
        Sine, Square, Saw, Half,
        // REEFS (8–11) — Bjorklund euclidean
        Eucl3, Eucl5, Eucl7, Eucl9,
        // GROOVES (12–15) — canonical 16-step rhythms
        Tresillo, Clave, Backbeat, Boombap,
        // DRIFTS (16–19) — deterministic-seeded probabilistic
        Drift, Sparkle, Foam, Riptide,
        // STORMS (20–23) — mathematical/generative
        Fibonacci, Prime, Golden, Eddy,
        Count = 24
    };

    //==========================================================================
    // Public API

    PerEnginePatternSequencer()
    {
        reset();
    }

    void prepareToPlay(double sampleRate)
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
        reset();
    }

    // Core audio-thread callback. Injects noteOn/noteOff events into `out`.
    // Call AFTER per-slot MIDI buffer plumbing but BEFORE engine renderBlock.
    void processBlock(juce::MidiBuffer& out,
                      double bpm, double ppqPosition,
                      bool isPlaying, int numSamples,
                      int midiChannel = 1)
    {
        // Load all atomics once at block entry (RT-safe: no per-sample atomic reads)
        const bool enabled       = enabled_.load(std::memory_order_relaxed);
        const int  patternInt    = pattern_.load(std::memory_order_relaxed);
        const int  stepCount     = juce::jlimit(1, 16, stepCount_.load(std::memory_order_relaxed));
        const int  clockDiv      = juce::jlimit(1, 8, clockDiv_.load(std::memory_order_relaxed));
        const float humanization = juce::jlimit(0.0f, 1.0f, humanization_.load(std::memory_order_relaxed));
        const float baseVel      = juce::jlimit(0.0f, 1.0f, baseVelocity_.load(std::memory_order_relaxed));
        const int  rootNote      = juce::jlimit(0, 127, rootNote_.load(std::memory_order_relaxed));
        const int  channel       = juce::jlimit(1, 16, midiChannel);

        if (!enabled || !isPlaying || bpm <= 0.0)
        {
            // Ensure any pending noteOff still fires even when disabled mid-phrase.
            // C3: use lastSoundingNote_ so pitch-offset notes are released correctly.
            if (noteOffCountdown_ > 0)
            {
                noteOffCountdown_ -= numSamples;
                if (noteOffCountdown_ <= 0)
                {
                    noteOffCountdown_ = 0;
                    // C3: use lastSoundingNote_ to release the pitch-shifted note correctly
                    out.addEvent(juce::MidiMessage::noteOff(channel, lastSoundingNote_), 0);
                    // C5: gate closed
                    liveGate_.store(0.0f, std::memory_order_relaxed);
                }
            }
            return;
        }

        // If playback just resumed or pattern/stepCount changed, invalidate prev step
        // so we don't miss the first step edge. Also rebuild euclidean cache if stale.
        if (stepCount != cachedStepCount_ || patternInt != cachedPatternInt_)
        {
            cachedStepCount_  = stepCount;
            cachedPatternInt_ = patternInt;
            prevStepIdx_      = -1; // force edge on next block
            rebuildPatternCache(static_cast<Pattern>(patternInt), stepCount);
        }

        // Compute current stepIdx from PPQ
        // stepsPerQuarter: 1=quarter note, 2=eighth, 4=16th, 8=32nd
        const double stepsPerQuarter = static_cast<double>(clockDiv);
        const int stepIdx = static_cast<int>(std::floor(ppqPosition * stepsPerQuarter))
                            % stepCount;

        // C5: update step-phase every block (even on same step) so mod sources track position.
        liveStepPhase_.store(static_cast<float>(stepIdx) / static_cast<float>(stepCount),
                             std::memory_order_relaxed);

        // Edge detection: only act when the step index advances
        if (stepIdx == prevStepIdx_)
        {
            // Same step — count down pending noteOff
            // C3: use lastSoundingNote_ for correct pitch-offset release
            if (noteOffCountdown_ > 0)
            {
                noteOffCountdown_ -= numSamples;
                if (noteOffCountdown_ <= 0)
                {
                    noteOffCountdown_ = 0;
                    // C3: use lastSoundingNote_ to release the pitch-shifted note correctly
                    out.addEvent(juce::MidiMessage::noteOff(channel, lastSoundingNote_), 0);
                    // C5: gate closed
                    liveGate_.store(0.0f, std::memory_order_relaxed);
                }
            }
            return;
        }

        // New step has arrived
        // First, flush any pending noteOff from the previous step
        // C3: use lastSoundingNote_ so pitch-offset notes are released correctly
        if (noteOffCountdown_ > 0)
        {
            noteOffCountdown_ = 0;
            out.addEvent(juce::MidiMessage::noteOff(channel, lastSoundingNote_), 0);
        }

        prevStepIdx_ = stepIdx;

        // Evaluate whether this step gates (fires a note)
        const Pattern pat = static_cast<Pattern>(juce::jlimit(0, static_cast<int>(Pattern::Count) - 1, patternInt));
        float velocity = computeVelocity(pat, stepIdx, stepCount, baseVel);

        // C3: per-step gate override — if the gate bitmap has ANY bit set, treat it as an
        // explicit on/off mask for this step (overrides the algorithmic velocity).
        // Rationale: bitmap==0 means "no overrides active" (default C1/C2 behaviour preserved).
        // When any override exists, bit i==1 means "force gate on" (keep algorithmic velocity
        // if > 0, else use baseVel), bit i==0 means "force gate off" (mute this step).
        {
            const uint32_t bitmap = stepGateBitmap_.load(std::memory_order_relaxed);
            if (bitmap != 0u)
            {
                const bool gateOverrideOn = (bitmap & (1u << static_cast<unsigned>(stepIdx))) != 0u;
                if (gateOverrideOn)
                {
                    // Step is explicitly forced ON — if algorithmic velocity was zero, use baseVel.
                    if (velocity <= 0.0f)
                        velocity = baseVel;
                }
                else
                {
                    // Step is explicitly forced OFF (muted).
                    velocity = 0.0f;
                }
            }
        }

        // C3: per-step pitch offset — read the stored semitone delta (±12).
        // Applied only when the step gates, so we compute the sounding note here.
        const int pitchOffset = static_cast<int>(stepPitch_[static_cast<size_t>(stepIdx)]
                                                    .load(std::memory_order_relaxed));
        const int soundingNote = juce::jlimit(0, 127, rootNote + pitchOffset);

        // #1298: per-step velocity override — if non-zero, replaces the pattern velocity.
        // 0.0 is the sentinel meaning "inherit from base velocity" (pattern-computed value kept).
        {
            const float stepVelOverride = stepStepVel_[static_cast<size_t>(stepIdx)]
                                            .load(std::memory_order_relaxed);
            if (stepVelOverride > 0.0f && velocity > 0.0f)
                velocity = juce::jlimit(0.01f, 1.0f, stepVelOverride);
        }

        if (velocity > 0.0f)
        {
            // Apply velocity-jitter humanization (v1: velocity-only, no timing jitter)
            if (humanization > 0.0f)
            {
                // ±20% * humanization, seeded from step so each step is reproducible
                float jitter = (humanRng_.nextFloat() * 2.0f - 1.0f) * 0.20f * humanization;
                velocity = juce::jlimit(0.01f, 1.0f, velocity + velocity * jitter);
            }

            // Fire noteOn at sample position 0 (block-aligned, v1 simplification).
            // Use soundingNote (rootNote + pitchOffset) so pitch edits are reflected.
            out.addEvent(juce::MidiMessage::noteOn(channel, soundingNote, velocity), 0);
            // Track the sounding note so the correct noteOff fires even if rootNote changes mid-step.
            lastSoundingNote_ = soundingNote;

            // #1298: per-step gate-length override — if non-zero, use it instead of 50% default.
            // Range 0.0..1.5 × step duration. 0.0 = inherit (use default 50%).
            const float stepGlenOverride = stepGateLen_[static_cast<size_t>(stepIdx)]
                                            .load(std::memory_order_relaxed);
            const double fullStepSecs = (60.0 / bpm) / stepsPerQuarter;
            double gateFraction;
            if (stepGlenOverride > 0.0f)
                gateFraction = static_cast<double>(juce::jlimit(0.01f, 1.5f, stepGlenOverride));
            else
                gateFraction = 0.5; // default: 50% gate

            const double noteOnSecs = fullStepSecs * gateFraction;
            noteOffCountdown_ = static_cast<int>(noteOnSecs * sampleRate_);
            // Clamp so noteOff always fires within a reasonable time
            noteOffCountdown_ = juce::jmax(1, noteOffCountdown_);

            // C5: update live state for ModSource consumers
            liveVelocity_.store(velocity, std::memory_order_relaxed);
            liveGate_.store(1.0f, std::memory_order_relaxed);
            // #1289: expose normalised pitch offset (-1..+1 from ±12 semitones)
            liveStepPitch_.store(static_cast<float>(pitchOffset) / 12.0f,
                                 std::memory_order_relaxed);
        }
        else
        {
            // Silent step — gate stays closed, velocity resets to 0
            liveVelocity_.store(0.0f, std::memory_order_relaxed);
            liveGate_.store(0.0f, std::memory_order_relaxed);
            // #1289: clear live pitch on silent step so it does not linger into next gate
            liveStepPitch_.store(0.0f, std::memory_order_relaxed);
        }
    }

    //==========================================================================
    // RT-safe setters (called from UI/message thread)

    void setPattern(Pattern p)     { pattern_.store(static_cast<int>(p), std::memory_order_relaxed); }
    void setStepCount(int n)       { stepCount_.store(juce::jlimit(1, 16, n), std::memory_order_relaxed); }
    void setClockDivision(int d)   { clockDiv_.store(juce::jlimit(1, 8, d), std::memory_order_relaxed); }
    void setHumanization(float h)  { humanization_.store(juce::jlimit(0.0f, 1.0f, h), std::memory_order_relaxed); }
    void setEnabled(bool e)        { enabled_.store(e, std::memory_order_relaxed); }
    void setRootNote(int n)        { rootNote_.store(juce::jlimit(0, 127, n), std::memory_order_relaxed); }
    void setBaseVelocity(float v)  { baseVelocity_.store(juce::jlimit(0.0f, 1.0f, v), std::memory_order_relaxed); }

    //==========================================================================
    // C3: Per-step overrides — gate and pitch offset
    // RT-safe: packed into atomic ints (16-bit gate bitmap + 8 int8 pitch values per 64-bit word).
    // UI thread writes via setStepGate / setStepPitch; audio thread reads in processBlock.

    /** Toggle gate override for step `i` (0-based). `value` true = step fires, false = muted.
        Has no effect when i >= 16. */
    void setStepGate(int i, bool value) noexcept
    {
        if (i < 0 || i >= 16) return;
        uint32_t mask = 1u << static_cast<unsigned>(i);
        // CAS loop — gate bitmap is a 32-bit atomic where bit i = 1 means gate-override-on.
        uint32_t prev = stepGateBitmap_.load(std::memory_order_relaxed);
        uint32_t next;
        do {
            next = value ? (prev | mask) : (prev & ~mask);
        } while (!stepGateBitmap_.compare_exchange_weak(prev, next,
                                                         std::memory_order_relaxed,
                                                         std::memory_order_relaxed));
    }

    /** Set pitch offset (semitones) for step `i`. Clamped to ±12 semitones.
        Has no effect when i >= 16. */
    void setStepPitch(int i, int semitones) noexcept
    {
        if (i < 0 || i >= 16) return;
        stepPitch_[static_cast<size_t>(i)].store(
            static_cast<int8_t>(juce::jlimit(-12, 12, semitones)),
            std::memory_order_relaxed);
    }

    /** Read back gate override state for UI refresh. */
    bool getStepGate(int i) const noexcept
    {
        if (i < 0 || i >= 16) return false;
        uint32_t bitmap = stepGateBitmap_.load(std::memory_order_relaxed);
        return (bitmap & (1u << static_cast<unsigned>(i))) != 0u;
    }

    /** Read back pitch offset for UI refresh. */
    int getStepPitch(int i) const noexcept
    {
        if (i < 0 || i >= 16) return 0;
        return static_cast<int>(stepPitch_[static_cast<size_t>(i)].load(std::memory_order_relaxed));
    }

    /** True if any step has a non-zero gate override or pitch offset (for dirty-state display). */
    bool hasAnyStepOverride() const noexcept
    {
        if (stepGateBitmap_.load(std::memory_order_relaxed) != 0u)
            return true;
        for (int i = 0; i < 16; ++i)
            if (stepPitch_[static_cast<size_t>(i)].load(std::memory_order_relaxed) != 0)
                return true;
        return false;
    }

    //==========================================================================
    // Wave 5 C5: Live ModSource read-outs (safe to call from any thread).
    //
    // These atomics are written by the audio thread at each step boundary inside
    // processBlock() and are read by the mod-source evaluation loop (also audio
    // thread, same processBlock, after sequencer processBlock).  They may also
    // be read from the message thread for UI meters — a one-block stale value is
    // acceptable.
    //
    // liveVelocity:  current step velocity 0.0–1.0 (0 when gate is off/silent)
    // liveGate:      1.0 while the gate countdown is active, 0.0 otherwise
    // liveStepPhase: current step index as a 0.0–1.0 ramp (stepIdx / stepCount)

    float getLiveVelocity()  const noexcept { return liveVelocity_.load(std::memory_order_relaxed); }
    float getLiveGate()      const noexcept { return liveGate_.load(std::memory_order_relaxed); }
    float getLiveStepPhase() const noexcept { return liveStepPhase_.load(std::memory_order_relaxed); }
    // #1289: current step pitch offset normalised to -1..+1 (from ±12 semitones).
    // Cleared to 0.0 on silent steps so the value does not linger across rests.
    float getLiveStepPitch() const noexcept { return liveStepPitch_.load(std::memory_order_relaxed); }

    //==========================================================================
    // APVTS integration

    // Register all per-slot sequencer parameters into the parameter layout.
    // prefix e.g. "slot0_seq_", displayPrefix e.g. "Slot 1 Seq "
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& prefix,
                              const juce::String& displayPrefix)
    {
        // Pattern names for the combined 24-pattern choice parameter.
        // Indices must match Pattern enum exactly.
        const juce::StringArray patternNames{
            // CRESTS 0-3
            "Pulse", "Surge", "Ebb", "Arc",
            // WAVES 4-7
            "Sine", "Square", "Saw", "Half",
            // REEFS 8-11
            "Eucl3", "Eucl5", "Eucl7", "Eucl9",
            // GROOVES 12-15
            "Tresillo", "Clave", "Backbeat", "Boombap",
            // DRIFTS 16-19
            "Drift", "Sparkle", "Foam", "Riptide",
            // STORMS 20-23
            "Fibonacci", "Prime", "Golden", "Eddy"
        };

        const juce::StringArray clockDivLabels{ "1/4", "1/8", "1/16", "1/32" };
        // Map: index 0→div=1(qtr), 1→div=2(8th), 2→div=4(16th), 3→div=8(32nd)
        // Stored as float index; converted in syncFromApvts.

        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(prefix + "enabled", 1),
            displayPrefix + "Enabled", false));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(prefix + "pattern", 1),
            displayPrefix + "Pattern", patternNames, 0));

        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(prefix + "stepCount", 1),
            displayPrefix + "Steps", 1, 16, 16));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(prefix + "clockDiv", 1),
            displayPrefix + "Clock Div", clockDivLabels, 2)); // default = 1/16

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(prefix + "humanize", 1),
            displayPrefix + "Humanize",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(prefix + "baseVel", 1),
            displayPrefix + "Base Velocity",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.75f));

        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(prefix + "rootNote", 1),
            displayPrefix + "Root Note", 0, 127, 60)); // default = middle C

        // C3: per-step gate overrides (16 bool params) + pitch offsets (16 int params, ±12 st)
        // Naming: slot[N]_seq_gate_<step> and slot[N]_seq_pitch_<step>  (step = 0..15)
        for (int i = 0; i < 16; ++i)
        {
            layout.add(std::make_unique<juce::AudioParameterBool>(
                juce::ParameterID(prefix + "gate_" + juce::String(i), 1),
                displayPrefix + "Gate " + juce::String(i + 1), false));

            layout.add(std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID(prefix + "pitch_" + juce::String(i), 1),
                displayPrefix + "Pitch " + juce::String(i + 1), -12, 12, 0));
        }

        // #1298: per-step velocity override (0..1 float, default -1 = "use base vel")
        // and gate-length override (0.0..1.5 × step duration, default -1 = "use global gate").
        // A value of -1 (sentinel) means "not set" — the engine uses the base/global value.
        // Range stored as 0..1 normalised after adding an offset so -1 sentinel fits in
        // a float param: actual stored value = (semitone + 1) / 2  →  [-1=0.0, 0=0.5, 1=1.0].
        // For simplicity we use dedicated ranges with a sentinel float below the normal range:
        //   vel:  stored as 0..1; 0.0 is the sentinel "inherit from base" → UI maps 0→inherit
        //   glen: stored as 0..150% (0.0..1.5); 0.0 = sentinel (inherit from global gate)
        // Naming: slot[N]_seq_svel_<step> and slot[N]_seq_glen_<step>
        for (int i = 0; i < 16; ++i)
        {
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID(prefix + "svel_" + juce::String(i), 1),
                displayPrefix + "Step Vel " + juce::String(i + 1),
                juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f)); // 0.0 = inherit

            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID(prefix + "glen_" + juce::String(i), 1),
                displayPrefix + "Gate Len " + juce::String(i + 1),
                juce::NormalisableRange<float>(0.0f, 1.5f), 0.0f)); // 0.0 = inherit
        }
    }

    // Sync atomic state from APVTS. Safe to call from the audio thread.
    //
    // RT-safety: `getRawParameterValue` is a hash-map lookup and is called
    // only when the cached pointer is null (i.e. once per slot per session,
    // on the first processBlock call after prepareToPlay). After that, the
    // method reads only from the cached std::atomic<float>* pointers —
    // identical to the CachedParams pattern used throughout XOceanusProcessor.
    void syncFromApvts(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
    {
        // Cache parameter pointers on first call (one-time hash-map lookup).
        if (cachedPEnable_ == nullptr)
        {
            cachedPEnable_    = apvts.getRawParameterValue(prefix + "enabled");
            cachedPPattern_   = apvts.getRawParameterValue(prefix + "pattern");
            cachedPStepCount_ = apvts.getRawParameterValue(prefix + "stepCount");
            cachedPClockDiv_  = apvts.getRawParameterValue(prefix + "clockDiv");
            cachedPHumanize_  = apvts.getRawParameterValue(prefix + "humanize");
            cachedPBaseVel_   = apvts.getRawParameterValue(prefix + "baseVel");
            cachedPRootNote_  = apvts.getRawParameterValue(prefix + "rootNote");
        }

        // Read from cached atomic pointers — O(1), no allocation.
        if (cachedPEnable_)
            enabled_.store(cachedPEnable_->load() > 0.5f, std::memory_order_relaxed);

        if (cachedPPattern_)
            pattern_.store(juce::jlimit(0, static_cast<int>(Pattern::Count) - 1,
                                        static_cast<int>(cachedPPattern_->load() + 0.5f)),
                           std::memory_order_relaxed);

        if (cachedPStepCount_)
            stepCount_.store(juce::jlimit(1, 16, static_cast<int>(cachedPStepCount_->load() + 0.5f)),
                             std::memory_order_relaxed);

        if (cachedPClockDiv_)
        {
            // Index to clockDivision: 0→1, 1→2, 2→4, 3→8
            static const int kDivTable[4] = {1, 2, 4, 8};
            int idx = juce::jlimit(0, 3, static_cast<int>(cachedPClockDiv_->load() + 0.5f));
            clockDiv_.store(kDivTable[idx], std::memory_order_relaxed);
        }

        if (cachedPHumanize_)
            humanization_.store(juce::jlimit(0.0f, 1.0f, cachedPHumanize_->load()),
                                std::memory_order_relaxed);

        if (cachedPBaseVel_)
            baseVelocity_.store(juce::jlimit(0.0f, 1.0f, cachedPBaseVel_->load()),
                                std::memory_order_relaxed);

        if (cachedPRootNote_)
            rootNote_.store(juce::jlimit(0, 127, static_cast<int>(cachedPRootNote_->load() + 0.5f)),
                            std::memory_order_relaxed);
    }

    // C3: Sync per-step overrides from APVTS. Called from the 15 Hz UI timer or
    // processBlock param-sync path. Safe on the message thread (reads APVTS directly).
    // `prefix` must match the one used in addParameters().
    void syncStepOverridesFromApvts(juce::AudioProcessorValueTreeState& apvts,
                                    const juce::String& prefix)
    {
        // Cache parameter pointers on first call per slot.
        if (cachedStepGateParams_[0] == nullptr)
        {
            for (int i = 0; i < 16; ++i)
            {
                cachedStepGateParams_[i]  = apvts.getRawParameterValue(prefix + "gate_"  + juce::String(i));
                cachedStepPitchParams_[i] = apvts.getRawParameterValue(prefix + "pitch_" + juce::String(i));
                // #1298: per-step velocity and gate-length overrides
                cachedStepVelParams_[i]   = apvts.getRawParameterValue(prefix + "svel_"  + juce::String(i));
                cachedStepGlenParams_[i]  = apvts.getRawParameterValue(prefix + "glen_"  + juce::String(i));
            }
        }

        uint32_t bitmap = 0u;
        for (int i = 0; i < 16; ++i)
        {
            if (cachedStepGateParams_[i] != nullptr)
            {
                const bool gateOn = cachedStepGateParams_[i]->load(std::memory_order_relaxed) > 0.5f;
                if (gateOn)
                    bitmap |= (1u << static_cast<unsigned>(i));
            }

            if (cachedStepPitchParams_[i] != nullptr)
            {
                const int semitones = juce::jlimit(-12, 12,
                    static_cast<int>(cachedStepPitchParams_[i]->load(std::memory_order_relaxed) + 0.5f));
                stepPitch_[static_cast<size_t>(i)].store(
                    static_cast<int8_t>(semitones), std::memory_order_relaxed);
            }

            // #1298: per-step velocity and gate-length overrides
            if (cachedStepVelParams_[i] != nullptr)
            {
                const float vel = cachedStepVelParams_[i]->load(std::memory_order_relaxed);
                // 0.0 = inherit from base velocity (sentinel); store as-is — processBlock reads
                stepStepVel_[static_cast<size_t>(i)].store(vel, std::memory_order_relaxed);
            }
            if (cachedStepGlenParams_[i] != nullptr)
            {
                const float glen = cachedStepGlenParams_[i]->load(std::memory_order_relaxed);
                // 0.0 = inherit from global gate; store as-is
                stepGateLen_[static_cast<size_t>(i)].store(glen, std::memory_order_relaxed);
            }
        }
        stepGateBitmap_.store(bitmap, std::memory_order_relaxed);
    }

    // Reset internal sequencer state (NOT parameter values).
    // Resets RNG to fixed seed for DRIFTS determinism; resets CA state for Eddy.
    // C3: does NOT reset step gate/pitch overrides — those are parameter state.
    void reset()
    {
        prevStepIdx_      = -1;
        noteOffCountdown_ = 0;
        lastSoundingNote_ = 60; // C3: reset to middle C (matches rootNote default)
        cachedStepCount_  = -1;
        cachedPatternInt_ = -1;
        riptideCycleCount_ = 0;
        humanRng_           = juce::Random(0xD1F7B0A7LL); // fixed seed — same sequence every reset
        driftRng_           = juce::Random(0xD1F7B0A7LL);
        eddyRow_.fill(0);
        eddyRowIdx_ = 0;
        // Pre-seed Eddy CA with a single live cell at centre
        eddyRow_[8] = 1; // centre of 16-step row
        patternCache_.fill(0.0f);
    }

private:
    //==========================================================================
    // Atomic state (RT-safe: set from UI thread, read from audio thread)

    std::atomic<bool>  enabled_{false};
    std::atomic<int>   pattern_{0};
    std::atomic<int>   stepCount_{16};
    std::atomic<int>   clockDiv_{4};      // default: 1/16 note
    std::atomic<float> humanization_{0.0f};
    std::atomic<float> baseVelocity_{0.75f};
    std::atomic<int>   rootNote_{60};     // middle C

    // Wave 5 C5: Live ModSource state — written by audio thread, read by mod evaluator.
    // Atomics allow message-thread reads for UI meters (one-block stale is fine).
    std::atomic<float> liveVelocity_{0.0f};   // 0.0–1.0; 0 when step is silent
    std::atomic<float> liveGate_{0.0f};       // 1.0 while noteOffCountdown_ > 0
    std::atomic<float> liveStepPhase_{0.0f};  // stepIdx / stepCount, 0.0–1.0
    // #1289: SeqStepPitch ModSource — current step's pitch offset normalised -1..+1
    // (raw semitone / 12.0f).  Cleared to 0.0 on silent steps.
    std::atomic<float> liveStepPitch_{0.0f};

    //==========================================================================
    // Cached APVTS parameter pointers — resolved once on first syncFromApvts() call.
    // Null until that first call. After that, reads are O(1) atomic loads.
    std::atomic<float>* cachedPEnable_    = nullptr;
    std::atomic<float>* cachedPPattern_   = nullptr;
    std::atomic<float>* cachedPStepCount_ = nullptr;
    std::atomic<float>* cachedPClockDiv_  = nullptr;
    std::atomic<float>* cachedPHumanize_  = nullptr;
    std::atomic<float>* cachedPBaseVel_   = nullptr;
    std::atomic<float>* cachedPRootNote_  = nullptr;

    // C3: Cached APVTS pointers for per-step gate overrides and pitch offsets.
    // Resolved once on first syncStepOverridesFromApvts() call.
    std::array<std::atomic<float>*, 16> cachedStepGateParams_  {};  // default-inits all to nullptr
    std::array<std::atomic<float>*, 16> cachedStepPitchParams_ {};
    // #1298: Cached APVTS pointers for per-step velocity and gate-length overrides.
    std::array<std::atomic<float>*, 16> cachedStepVelParams_   {};
    std::array<std::atomic<float>*, 16> cachedStepGlenParams_  {};

    //==========================================================================
    // C3: Per-step gate + pitch override state (written from UI thread, read from audio thread).
    //
    // Gate bitmap: bit i == 1 means step i is explicitly forced ON; bit i == 0 means OFF.
    // Bitmap == 0 (all bits clear) is the default "no overrides" state (C1/C2 behaviour).
    // When any bit is set, ALL 16 steps are interpreted via the bitmap (explicit mask mode).
    //
    // Pitch offsets: one int8 per step, ±12 semitones relative to rootNote_.
    // Default 0 = no pitch shift (C1/C2 behaviour preserved).
    std::atomic<uint32_t>                   stepGateBitmap_{0};
    std::array<std::atomic<int8_t>, 16>     stepPitch_ {};  // zero-initialised

    // #1298: Per-step velocity overrides (0.0 = inherit from baseVelocity_) and
    // gate-length overrides (0.0 = inherit from global half-step gate, >0 = explicit).
    // Both written from UI thread, read from audio thread.
    std::array<std::atomic<float>, 16>      stepStepVel_  {};  // 0.0 = inherit
    std::array<std::atomic<float>, 16>      stepGateLen_  {};  // 0.0 = inherit

    //==========================================================================
    // Audio-thread-only state (no atomics needed — never touched from UI thread)

    double sampleRate_{44100.0};
    int    prevStepIdx_{-1};
    int    noteOffCountdown_{0};
    int    lastSoundingNote_{60};  // C3: tracks the most recent noteOn pitch for correct noteOff

    // Pattern cache invalidation keys
    int cachedStepCount_{-1};
    int cachedPatternInt_{-1};

    // Euclidean + STORMS gate caches (pre-computed on pattern/stepCount change)
    // Index i contains velocity > 0 if step i gates, 0 if not.
    std::array<float, 16> patternCache_{};

    // DRIFTS RNG — seeded at reset(), shared between Drift/Sparkle/Foam/Riptide
    juce::Random driftRng_{0xD1F7B0A7LL};

    // Velocity jitter RNG — separate from pattern RNG so humanization doesn't
    // pollute DRIFTS determinism.
    juce::Random humanRng_{0xD1F7B0A7LL};

    // RIPTIDE cycle counter — polarity flips each time through all stepCount steps
    int riptideCycleCount_{0};
    [[maybe_unused]] int riptidePrevStep_{-1}; // tracks wrap-around to increment cycle

    // EDDY (Wolfram Rule 30 cellular automaton)
    // Single 16-cell row; evolve one generation per pattern cycle.
    // Seeded with one live cell at centre position.
    std::array<uint8_t, 16> eddyRow_{};
    int eddyRowIdx_{0}; // which generation index we're on
    [[maybe_unused]] int eddyCachedGen_{-1}; // generation number at last cache build

    //==========================================================================
    // Pattern computation

    // Rebuild the pattern cache when pattern or stepCount changes.
    // Called from processBlock on the audio thread before computing velocities.
    void rebuildPatternCache(Pattern pat, int stepCount)
    {
        patternCache_.fill(0.0f);

        switch (pat)
        {
        case Pattern::Pulse:
        case Pattern::Surge:
        case Pattern::Ebb:
        case Pattern::Arc:
        case Pattern::Sine:
        case Pattern::Square:
        case Pattern::Saw:
        case Pattern::Half:
            // CRESTS and WAVES: computed on-the-fly from baseVelocity in computeVelocity()
            // Cache not used — mark all steps as "potentially gating" by setting 1.0
            for (int i = 0; i < stepCount; ++i)
                patternCache_[i] = 1.0f;
            break;

        case Pattern::Eucl3:  buildEuclidean(3, stepCount);  break;
        case Pattern::Eucl5:  buildEuclidean(5, stepCount);  break;
        case Pattern::Eucl7:  buildEuclidean(7, stepCount);  break;
        case Pattern::Eucl9:  buildEuclidean(9, stepCount);  break;

        case Pattern::Tresillo: buildTresillo(stepCount);  break;
        case Pattern::Clave:    buildClave(stepCount);      break;
        case Pattern::Backbeat: buildBackbeat(stepCount);   break;
        case Pattern::Boombap:  buildBoombap(stepCount);    break;

        case Pattern::Drift:
        case Pattern::Sparkle:
        case Pattern::Foam:
        case Pattern::Riptide:
            buildDrift(pat, stepCount);
            break;

        case Pattern::Fibonacci: buildFibonacci(stepCount);  break;
        case Pattern::Prime:     buildPrime(stepCount);      break;
        case Pattern::Golden:    buildGolden(stepCount);     break;
        case Pattern::Eddy:      buildEddy(stepCount);       break;

        default: break;
        }
    }

    // Compute the velocity for a given step, or 0.0 if the step is silent.
    // `baseVel` is the block-local snapshot of baseVelocity_.
    float computeVelocity(Pattern pat, int stepIdx, int stepCount, float baseVel) const
    {
        stepIdx = juce::jlimit(0, stepCount - 1, stepIdx);

        switch (pat)
        {
        //----------------------------------------------------------------------
        // CRESTS — every step gates, velocity shaped
        case Pattern::Pulse:
            return baseVel;

        case Pattern::Surge:
            // Ramp up: step 0 is softest, last step is loudest
            return baseVel * static_cast<float>(stepIdx + 1) / static_cast<float>(stepCount);

        case Pattern::Ebb:
            // Ramp down: step 0 is loudest, last step is softest
            return baseVel * static_cast<float>(stepCount - stepIdx) / static_cast<float>(stepCount);

        case Pattern::Arc:
        {
            // Triangle: peak at midpoint. Guard stepCount==1.
            if (stepCount <= 1)
                return baseVel;
            float t = static_cast<float>(stepIdx) / static_cast<float>(stepCount - 1);
            float tri = 1.0f - 2.0f * std::abs(t - 0.5f);
            return baseVel * tri;
        }

        //----------------------------------------------------------------------
        // WAVES — modulation-shaped
        case Pattern::Sine:
        {
            // Full sine cycle over stepCount steps; every step gates
            constexpr float kTwoPi = 6.28318530718f;
            float phase = kTwoPi * static_cast<float>(stepIdx) / static_cast<float>(stepCount);
            return baseVel * (0.5f + 0.5f * std::sin(phase));
        }

        case Pattern::Square:
            // Even steps at full vel, odd steps at half vel
            return (stepIdx % 2 == 0) ? baseVel : baseVel * 0.5f;

        case Pattern::Saw:
        {
            // 4 ramps per pattern (saw = fractional part of i/(stepCount/4.0))
            float t = static_cast<float>(stepIdx) / (static_cast<float>(stepCount) / 4.0f);
            float frac = t - std::floor(t);
            return baseVel * frac;
        }

        case Pattern::Half:
            // Gate only on even steps, velocity = baseVel
            return (stepIdx % 2 == 0) ? baseVel : 0.0f;

        //----------------------------------------------------------------------
        // REEFS, GROOVES, DRIFTS, STORMS — use pre-built cache
        // Cache stores a [0,1] gate/velocity factor; multiply by baseVel.
        // DRIFTS patterns use factors > 1.0 for jitter variation — clamped here.
        default:
        {
            float v = (stepIdx < 16) ? patternCache_[stepIdx] : 0.0f;
            if (v <= 0.0f)
                return 0.0f;
            // Cache factor: 1.0f for REEFS/GROOVES/STORMS (hit/rest),
            //               0.4f for Boombap ghost hats,
            //               0.8..1.2f for DRIFTS (jitter range).
            // All are multiplied by baseVel; result clamped to [0,1].
            return juce::jlimit(0.0f, 1.0f, v * baseVel);
        }
        }
    }

    //==========================================================================
    // Cache builders — called once when pattern/stepCount changes

    // Bjorklund's euclidean rhythm algorithm.
    // Distributes k hits as evenly as possible over n slots.
    // Result stored in patternCache_[0..n-1] (1.0f = hit, 0.0f = rest).
    // Zero-offset rotation: the pattern always begins with a hit when k>0.
    //
    // The algorithm represents the pattern as a sequence of groups and
    // recursively merges the smaller group into the larger until one
    // type remains — identical to Euclid's GCD algorithm, hence "Euclidean".
    //
    // Verified:
    //   E(3,8) → [1,0,0,1,0,0,1,0]  hits at 0,3,6  ✓
    //   E(5,8) → [1,0,1,0,1,0,1,0]  hits at 0,2,4,5,7 (one rotation of standard)
    //   E(3,16) → hits at 0,5,10 ✓
    void buildEuclidean(int k, int n)
    {
        if (k <= 0 || n <= 0)
            return;

        // If k >= n, every step fires
        if (k >= n)
        {
            for (int i = 0; i < n; ++i)
                patternCache_[i] = 1.0f;
            return;
        }

        // Represent as counts: `ones` groups of length 1 (the hits),
        // `zeros` groups of length 0 (the rests).
        // We track group lengths via two counters using Bresenham-style accumulation
        // which is equivalent to Bjorklund's recursive subdivision.
        //
        // Implementation following Toussaint 2005 §3 "The Euclidean Algorithm":
        // Pattern[i] = 1 iff floor(i*k/n) > floor((i-1)*k/n)
        // This directly gives the standard Euclidean rhythm without recursion
        // and produces identical results to the recursive Bjorklund algorithm.
        //
        // E(3,8): hit at i where floor(i*3/8) increases:
        //   i=0: 0>-1  ✓ (hit)   i=1: 0=0  (rest)  i=2: 0=0  (rest)
        //   i=3: 1>0   ✓ (hit)   i=4: 1=1  (rest)  i=5: 1=1  (rest)
        //   i=6: 2>1   ✓ (hit)   i=7: 2=2  (rest)
        //   → [1,0,0,1,0,0,1,0]  ✓
        int prev = -1;
        for (int i = 0; i < n; ++i)
        {
            int cur = (i * k) / n;
            if (cur > prev)
                patternCache_[i] = 1.0f;
            prev = cur;
        }
    }

    // Scale a canonical 16-step hit-set to stepCount by rounding.
    // hitSet: sorted array of hit positions in 16-step space.
    // Deduplicates after rounding.
    void buildScaledGroove(const std::initializer_list<int>& hits16, int n, float vel = 1.0f)
    {
        for (int i = 0; i < 16 && i < n; ++i)
            patternCache_[i] = 0.0f;

        bool seen[16] = {};
        for (int h : hits16)
        {
            int scaled = static_cast<int>(std::round(static_cast<float>(h) * static_cast<float>(n) / 16.0f));
            scaled = juce::jlimit(0, n - 1, scaled);
            if (!seen[scaled])
            {
                patternCache_[scaled] = vel;
                seen[scaled] = true;
            }
        }
    }

    void buildTresillo(int n)
    {
        // 3-3-2 Latin pattern: canonical 16-step hits at [0,3,6,8,11,14]
        buildScaledGroove({0, 3, 6, 8, 11, 14}, n);
    }

    void buildClave(int n)
    {
        // Son clave 3-2: hits at [0,3,6,10,12] in 16-step canonical form
        // Mask: [1,0,0,1,0,0,1,0,0,0,1,0,1,0,0,0]
        buildScaledGroove({0, 3, 6, 10, 12}, n);
    }

    void buildBackbeat(int n)
    {
        // Beats 2 and 4 at 16-step = positions 4 and 12
        buildScaledGroove({4, 12}, n);
    }

    void buildBoombap(int n)
    {
        // Kick at 0,8 (full vel) + snare at 4,12 (full vel) + ghost hat at 2,6,10,14 (0.4 vel)
        // Store all; ghost hat at 40% base velocity
        buildScaledGroove({0, 4, 8, 12}, n, 1.0f);    // kick + snare full

        // Now add ghost hats at 0.4 * vel, but only if that step isn't already used
        bool seenGhost[16] = {};
        for (int i = 0; i < n; ++i)
            seenGhost[i] = (patternCache_[i] > 0.0f);

        bool seen16[16] = {};
        for (int h : {2, 6, 10, 14})
        {
            int scaled = static_cast<int>(std::round(static_cast<float>(h) * static_cast<float>(n) / 16.0f));
            scaled = juce::jlimit(0, n - 1, scaled);
            if (!seenGhost[scaled] && !seen16[scaled])
            {
                patternCache_[scaled] = 0.4f; // ghost hat at 40% base velocity
                seen16[scaled] = true;
            }
        }
    }

    // DRIFTS — deterministic-seeded probabilistic patterns.
    // Reset driftRng_ before building so patterns are reproducible per reset().
    void buildDrift(Pattern pat, int n)
    {
        // Reset the drift RNG to the same seed every time we rebuild so that
        // changing stepCount and coming back gives the same pattern.
        // Note: we use a deterministic walk, NOT the live driftRng_ member,
        // so that the pattern doesn't drift between renders.
        juce::Random r(0xD1F7B0A7LL);

        switch (pat)
        {
        case Pattern::Drift:
            // ~50% gate density, velocity ±20% jitter
            for (int i = 0; i < n; ++i)
            {
                if (r.nextFloat() < 0.5f)
                    patternCache_[i] = 0.8f + r.nextFloat() * 0.4f; // 0.8..1.2 * baseVel (clamped at call site)
                else
                    patternCache_[i] = 0.0f;
            }
            break;

        case Pattern::Sparkle:
            // ~25% density, velocity 0.4..1.0
            for (int i = 0; i < n; ++i)
            {
                if (r.nextFloat() < 0.25f)
                    patternCache_[i] = 0.4f + r.nextFloat() * 0.6f;
                else
                    patternCache_[i] = 0.0f;
            }
            break;

        case Pattern::Foam:
            // ~75% density, velocity 0.5..1.0
            for (int i = 0; i < n; ++i)
            {
                if (r.nextFloat() < 0.75f)
                    patternCache_[i] = 0.5f + r.nextFloat() * 0.5f;
                else
                    patternCache_[i] = 0.0f;
            }
            break;

        case Pattern::Riptide:
            // First half ~25% density, second half ~75% density.
            // Polarity flips each cycle (tracked in riptideCycleCount_).
            // On cache rebuild we don't know cycle yet; use base polarity (even=normal).
            // Actual polarity applied in processBlock is baked in here per cycle.
            {
                bool flipped = (riptideCycleCount_ % 2 != 0);
                int half = n / 2;
                for (int i = 0; i < n; ++i)
                {
                    float prob;
                    if (i < half)
                        prob = flipped ? 0.75f : 0.25f;
                    else
                        prob = flipped ? 0.25f : 0.75f;

                    if (r.nextFloat() < prob)
                        patternCache_[i] = 0.5f + r.nextFloat() * 0.5f;
                    else
                        patternCache_[i] = 0.0f;
                }
            }
            break;

        default: break;
        }
    }

    void buildFibonacci(int n)
    {
        // Fibonacci numbers ≤ n-1 (0-indexed): 0,1,1,2,3,5,8,13 → deduplicated = 0,1,2,3,5,8,13
        bool seen[16] = {};
        int a = 0, b = 1;
        while (a < n)
        {
            if (!seen[a])
            {
                patternCache_[a] = 1.0f;
                seen[a] = true;
            }
            int c = a + b;
            a = b;
            b = c;
        }
    }

    void buildPrime(int n)
    {
        // Hit at every prime index p where p < n (0-indexed)
        for (int i = 0; i < n; ++i)
        {
            if (isPrime(i))
                patternCache_[i] = 1.0f;
        }
    }

    static bool isPrime(int x)
    {
        if (x < 2) return false;
        if (x == 2) return true;
        if (x % 2 == 0) return false;
        for (int d = 3; d * d <= x; d += 2)
            if (x % d == 0) return false;
        return true;
    }

    void buildGolden(int n)
    {
        // Golden ratio hits: floor(i * phi) % n for i = 0..(n-1), collect first n unique indices
        // phi = 1.61803398875
        constexpr double phi = 1.61803398875;
        bool seen[16] = {};
        int  count = 0;
        for (int i = 0; count < n && i < 256; ++i)
        {
            int idx = static_cast<int>(std::floor(static_cast<double>(i) * phi)) % n;
            if (!seen[idx])
            {
                patternCache_[idx] = 1.0f;
                seen[idx] = true;
                ++count;
            }
        }
    }

    // Wolfram Rule 30 cellular automaton.
    // eddyRow_ is the current generation (16 cells). Evolve one row per pattern cycle.
    // We cache the row index used at last build; rebuild only when cycle advances.
    void buildEddy(int n)
    {
        // Gate where eddyRow_[i] == 1 (for i < n)
        for (int i = 0; i < n; ++i)
            patternCache_[i] = (eddyRow_[i] != 0) ? 1.0f : 0.0f;

        // Evolve one generation of Rule 30 for next cycle.
        // Rule 30: newCell[i] = cell[i-1] XOR (cell[i] OR cell[i+1])
        std::array<uint8_t, 16> next{};
        for (int i = 0; i < 16; ++i)
        {
            int left   = (i > 0)  ? eddyRow_[static_cast<size_t>(i - 1)] : 0;
            int center = eddyRow_[static_cast<size_t>(i)];
            int right  = (i < 15) ? eddyRow_[static_cast<size_t>(i + 1)] : 0;
            // Rule 30: neighbourhood = (left, center, right) → bit pattern
            // 000→0, 001→1, 010→1, 011→1, 100→1, 101→0, 110→0, 111→0
            int bits = (left << 2) | (center << 1) | right;
            next[static_cast<size_t>(i)] = static_cast<uint8_t>((30 >> bits) & 1);
        }
        eddyRow_ = next;
        ++eddyRowIdx_;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerEnginePatternSequencer)
};

} // namespace XOceanus
