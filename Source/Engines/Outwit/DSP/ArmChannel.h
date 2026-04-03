// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// ArmChannel.h — XOutwit single arm: Wolfram cellular automaton voice
//
// Each arm is one of 8 independent "tentacles" of the Giant Pacific Octopus.
// It runs a 1D Wolfram elementary cellular automaton (rule 0-255) on a circular
// tape of configurable length. Cell transitions clock at setStepRate Hz.
// Active cells trigger oscillator pulses; the pitch, filter, waveshape, level,
// and pan are all per-arm.
//
// Adapter usage:
//   arm.prepare(sampleRate)
//   arm.setParams(rule, length, level, pitchSt, filterHz, wave, pan)
//   arm.setStepRate(hz)
//   arm.noteOn(midiNote, velocity)
//   arm.noteOff()
//   float s = arm.processSample(0.0f, chromAmount, filterRes, filterType, triggerThresh)
//   bool stepped = arm.justStepped           // true for exactly one sample after a CA step
//   float d = arm.getDensity()               // fraction of live cells [0,1]
//   arm.applySynapse(sourceDensity, amount)  // density from neighbour arm drives step phase
//
// DSP:
//   - Oscillator: Saw/Pulse/Sine selectable per arm, PolyBLEP anti-aliased
//   - Amplitude envelope: simple AR per-step trigger
//   - Filter: State-variable filter (LP/BP/HP)
//   - Chromatophore modulation: scales filter cutoff per-step
//==============================================================================

#include "FastMath.h"
#include <array>
#include <algorithm>
#include <cstdint>
#include <cmath>

namespace xoutwit
{

class ArmChannel
{
public:
    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        invSr = 1.0f / sr;

        // Cache step envelope release coefficient (Seance P4: was recomputed every sample)
        stepEnvRelCoeff = xoutwit::fastExp(-1.0f / (0.02f * sr)); // ~20ms release

        // Reset oscillator + filter + CA state
        oscPhase = 0.0f;
        stepPhase = 0.0f;
        justStepped = false;
        svfIc1 = svfIc2 = 0.0f;
        stepEnvGain = 0.0f;
        noteActive = false;
        currentNote = 60;
        currentVelocity = 0.0f;
        baseFreqHz = 261.63f;
        synapsePhaseNudge = 0.0f;

        // Init CA tape to a single active cell in the centre
        tape.fill(false);
        tape[0] = true;
    }

    //==========================================================================
    // Parameter update (once per block)
    //==========================================================================

    // rule      [0,255]   : Wolfram elementary CA rule number
    // length    [4,64]    : CA tape length (cells)
    // level     [0,1]     : arm output level
    // pitchSt   [-24,24]  : semitone offset from MIDI note
    // filterHz  [20,20000]: per-arm base filter cutoff
    // wave      [0,1,2]   : 0=Saw, 1=Pulse, 2=Sine
    // pan       [-1,1]    : stereo pan (stored but panning done in adapter)
    void setParams(int rule, int length, float level, int pitchSt, float filterHz, int wave, float pan) noexcept
    {
        caRule = static_cast<uint8_t>(std::clamp(rule, 0, 255));
        tapeLen = std::clamp(length, 4, kMaxTape);
        armLevel = std::clamp(level, 0.0f, 1.0f);
        pitchOffset = pitchSt;
        baseFilterHz = std::clamp(filterHz, 20.0f, 20000.0f);
        waveShape = std::clamp(wave, 0, 2);
        armPan = std::clamp(pan, -1.0f, 1.0f);

        // Recompute frequency if note is held
        if (noteActive)
        {
            float newFreq = xoutwit::midiToFreqTune(currentNote, static_cast<float>(pitchOffset));
            if (gliding)
                targetFreqHz = newFreq; // Seance P1: update glide target, don't jump
            else
                baseFreqHz = newFreq;
        }
    }

    //--------------------------------------------------------------------------
    void setStepRate(float hz) noexcept { stepRate = std::max(0.01f, hz); }

    // Seance P1: set portamento smoothing rate (1.0 = instant, near 0 = slow)
    void setGlideRate(float rate) noexcept { glideRate = std::clamp(rate, 0.0001f, 1.0f); }

    //==========================================================================
    // MIDI
    //==========================================================================

    void noteOn(int midiNote, float velocity) noexcept
    {
        currentNote = midiNote;
        currentVelocity = velocity;
        noteActive = true;
        oscPhase = 0.0f;
        baseFreqHz = xoutwit::midiToFreqTune(midiNote, static_cast<float>(pitchOffset));
        targetFreqHz = baseFreqHz; // Seance P1: sync target with base on hard trigger
        gliding = false;

        // Re-seed CA tape on note-on — starts from a single live cell
        tape.fill(false);
        tape[0] = true;
        stepPhase = 0.0f;
    }

    // Seance P1: Mono legato glide — update target pitch without resetting
    // CA state, oscillator phase, or step envelope. Actual frequency
    // interpolation is handled per-sample via glideFreq in the adapter.
    void setGlideTarget(int midiNote, float velocity) noexcept
    {
        currentNote = midiNote;
        currentVelocity = velocity;
        targetFreqHz = xoutwit::midiToFreqTune(midiNote, static_cast<float>(pitchOffset));
        gliding = true;
    }

    void noteOff() noexcept
    {
        noteActive = false;
        gliding = false;
    }

    //==========================================================================
    // processSample — call every sample
    //
    //   extInput     : reserved (currently unused, always 0.0f from adapter)
    //   chromAmount  : [0,1] chromatophore modulation depth (opens filter with CA density)
    //   filterRes    : [0,1] resonance (shared across all arms, per adapter design)
    //   filterType   : 0=LP, 1=BP, 2=HP
    //   triggerThresh: [0,1] minimum CA density fraction to allow oscillator output
    //==========================================================================

    float processSample(float /*extInput*/, float chromAmount, float filterRes, int filterType,
                        float triggerThresh) noexcept
    {
        justStepped = false;

        if (!noteActive && stepEnvGain < 0.00001f)
            return 0.0f;

        // --- CA step clock ---
        float stepInc = stepRate * invSr;
        stepPhase += stepInc + synapsePhaseNudge;
        synapsePhaseNudge = 0.0f; // consume

        if (stepPhase >= 1.0f)
        {
            stepPhase -= 1.0f;
            advanceCA();
            justStepped = true;

            // Trigger step envelope only if density > threshold
            if (getDensity() > triggerThresh)
                stepEnvGain = currentVelocity * armLevel;
        }

        // --- Step envelope (simple AR) ---
        // Attack: 1 sample (instant); release: exponential (~20ms)
        // Coefficient cached in prepare() (Seance P4)
        stepEnvGain *= stepEnvRelCoeff;
        stepEnvGain = xoutwit::flushDenormal(stepEnvGain);

        if (stepEnvGain < 0.00001f)
        {
            stepEnvGain = 0.0f;
            if (!noteActive)
                return 0.0f;
        }

        // --- Glide (Seance P1): smoothly interpolate toward target frequency ---
        if (gliding && targetFreqHz > 0.0f)
        {
            baseFreqHz += (targetFreqHz - baseFreqHz) * glideRate;
            // No flushDenormal needed: convergence snap below (< 0.01f) prevents subnormals for MIDI-range frequencies
            if (std::abs(targetFreqHz - baseFreqHz) < 0.01f)
            {
                baseFreqHz = targetFreqHz;
                gliding = false;
            }
        }

        // --- Oscillator ---
        float freq = baseFreqHz;
        float phaseInc = freq * invSr;
        oscPhase += phaseInc;
        if (oscPhase >= 1.0f)
            oscPhase -= 1.0f;

        float osc = generateOscillator(oscPhase, phaseInc);

        // --- Chromatophore: modulate filter cutoff with CA density ---
        float density = getDensity();
        float modCutoff = baseFilterHz * (1.0f + chromAmount * density * 3.0f);
        modCutoff = std::clamp(modCutoff, 20.0f, 20000.0f);

        // --- SVF filter (TPT topology) ---
        float filtered = processSVF(osc, modCutoff, filterRes, filterType);

        return filtered * stepEnvGain;
    }

    //==========================================================================
    // SYNAPSE coupling: a neighbour arm's density nudges this arm's step phase
    //==========================================================================

    void applySynapse(float sourceDensity, float amount) noexcept
    {
        // Dense source arm slightly accelerates this arm's step clock
        synapsePhaseNudge += sourceDensity * amount * 0.05f;
    }

    //==========================================================================
    // getDensity — fraction of live cells in tape [0,1]
    //==========================================================================

    float getDensity() const noexcept
    {
        int live = 0;
        for (int i = 0; i < tapeLen; ++i)
            if (tape[static_cast<size_t>(i)])
                ++live;
        return static_cast<float>(live) / static_cast<float>(std::max(1, tapeLen));
    }

    //==========================================================================
    // Public state (accessed by adapter)
    //==========================================================================

    bool justStepped = false; // true for 1 sample after each CA step

private:
    //==========================================================================
    // Wolfram elementary CA
    //==========================================================================

    static constexpr int kMaxTape = 64;

    std::array<bool, kMaxTape> tape{};
    std::array<bool, kMaxTape> tapeTmp{};

    int tapeLen = 16;
    uint8_t caRule = 110;

    void advanceCA() noexcept
    {
        for (int i = 0; i < tapeLen; ++i)
        {
            int left = (i - 1 + tapeLen) % tapeLen;
            int right = (i + 1) % tapeLen;

            // 3-cell neighbourhood → bit pattern [L, C, R]
            uint8_t pattern = (tape[static_cast<size_t>(left)] ? 4u : 0u) | (tape[static_cast<size_t>(i)] ? 2u : 0u) |
                              (tape[static_cast<size_t>(right)] ? 1u : 0u);

            tapeTmp[static_cast<size_t>(i)] = (caRule >> pattern) & 1u;
        }
        tape = tapeTmp;
    }

    //==========================================================================
    // Oscillator
    //==========================================================================

    float oscPhase = 0.0f;
    int waveShape = 0; // 0=Saw, 1=Pulse, 2=Sine

    // PolyBLEP correction — reduces aliasing at discontinuities (Seance P3)
    // t = phase distance from discontinuity, dt = phaseInc
    static float polyBLEP(float t, float dt) noexcept
    {
        if (t < dt) // rising edge: t in [0, dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        else if (t > 1.0f - dt) // falling edge: t in (1-dt, 1)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    float generateOscillator(float phase, float phaseInc) noexcept
    {
        switch (waveShape)
        {
        case 0: // Saw with PolyBLEP anti-aliasing
        {
            float saw = 2.0f * phase - 1.0f;
            saw -= polyBLEP(phase, phaseInc);
            return saw;
        }

        case 1: // Pulse (50% duty) with PolyBLEP anti-aliasing
        {
            float pulse = phase < 0.5f ? 1.0f : -1.0f;
            pulse += polyBLEP(phase, phaseInc); // discontinuity at phase=0
            float shifted = phase + 0.5f;
            if (shifted >= 1.0f)
                shifted -= 1.0f;
            pulse -= polyBLEP(shifted, phaseInc); // discontinuity at phase=0.5
            return pulse;
        }

        case 2: // Sine (no anti-aliasing needed)
            return xoutwit::fastSin(phase * 6.28318530f);

        default:
        {
            float saw = 2.0f * phase - 1.0f;
            saw -= polyBLEP(phase, phaseInc);
            return saw;
        }
        }
    }

    //==========================================================================
    // State-variable filter (Chamberlin TPT)
    //==========================================================================

    float svfIc1 = 0.0f; // integrator 1 state (band output)
    float svfIc2 = 0.0f; // integrator 2 state (LP output)

    float processSVF(float input, float cutoffHz, float resonance, int type) noexcept
    {
        // TPT SVF — see Zavalishin "The Art of VA Filter Design"
        cutoffHz = std::min(cutoffHz, 0.499f / invSr); // #604: clamp fc < Nyquist before tan to prevent overflow
        float f = xoutwit::fastTan(3.14159265f * cutoffHz * invSr);
        float q = std::max(0.5f, 0.5f + resonance * 9.5f); // map [0,1] -> [0.5, 10]
        float r = 1.0f / q;

        float hp = (input - r * svfIc1 - svfIc2) / (1.0f + r * f + f * f);
        float bp = f * hp + svfIc1;
        float lp = f * bp + svfIc2;

        svfIc1 = xoutwit::flushDenormal(f * hp + bp);
        svfIc2 = xoutwit::flushDenormal(f * bp + lp);

        switch (type)
        {
        case 1:
            return bp;
        case 2:
            return hp;
        default:
            return lp;
        }
    }

    //==========================================================================
    // Per-arm parameters (set via setParams / setStepRate)
    //==========================================================================

    float armLevel = 0.7f;
    float baseFilterHz = 4000.0f;
    float armPan = 0.0f;
    int pitchOffset = 0;

    float stepRate = 4.0f;  // Hz
    float stepPhase = 0.0f; // [0, 1)

    //==========================================================================
    // Voice state
    //==========================================================================

    bool noteActive = false;
    int currentNote = 60;
    float currentVelocity = 0.0f;
    float baseFreqHz = 261.63f;
    float stepEnvGain = 0.0f;

    float synapsePhaseNudge = 0.0f;

    // Glide state (Seance P1)
    float targetFreqHz = 261.63f;
    float glideRate = 0.005f; // per-sample smoothing rate (set from adapter)
    bool gliding = false;

    //==========================================================================
    // Engine state
    //==========================================================================

    float sr = 48000.0f;
    float invSr = 1.0f / 48000.0f;
    float stepEnvRelCoeff = 0.9986f; // cached: fastExp(-1/(0.02*48000)), updated in prepare()
};

} // namespace xoutwit
