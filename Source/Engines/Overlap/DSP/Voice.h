// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// Voice.h — xoverlap::Voice
//
// One of 6 synthesis voices in the OVERLAP engine. Each voice produces a
// pulsed oscillator signal that feeds into the knot-topology FDN.
//
// Synthesis model: A sine/pulse hybrid oscillator whose phase is driven by a
// MIDI pitch. The voice envelope (ADSR) shapes the amplitude. A "pulse rate"
// parameter modulates the oscillator's waveform between smooth sine and hard
// pulse — higher rates produce shorter, more impulsive excitation into the FDN,
// creating the "jellyfish tentacle" tangled-reverberation character.
//
// Voice allocation state (noteOnOrder, noteOffOrder, held, midiNote) is public
// so XOverlapEngine::allocateVoice() and handleNoteOff() can inspect it directly
// without friend declarations.
//==============================================================================

#include "FastMath.h"
#include <cstdint>
#include <cmath>

namespace xoverlap
{

//==============================================================================
// Simple ADSR envelope used by Voice.
// env.setParams() is called per-block to update parameters without allocation.
//==============================================================================
struct VoiceEnvelope
{
    //==========================================================================
    enum class Stage
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    float attack = 0.05f;
    float decay = 1.0f;
    float sustain = 0.7f;
    float release = 2.0f;

    float level = 0.0f;
    float coeff = 0.0f;
    float target = 0.0f;
    Stage stage = Stage::Idle;
    float sampleRate = 0.0f;  // Sentinel: must be set by prepare() before use

    //==========================================================================
    void prepare(float sr) noexcept
    {
        sampleRate = sr;
        level = 0.0f;
        stage = Stage::Idle;
        coeff = 0.0f;
        target = 0.0f;
    }

    void reset() noexcept
    {
        level = 0.0f;
        stage = Stage::Idle;
    }

    void setParams(float a, float d, float s, float r) noexcept
    {
        attack = juce::jmax(0.0001f, a);
        decay = juce::jmax(0.0001f, d);
        sustain = juce::jmax(0.0f, s);
        release = juce::jmax(0.0001f, r);
    }

    void noteOn() noexcept
    {
        stage = Stage::Attack;
        // Exponential attack: compute per-sample coefficient
        coeff = computeCoeff(attack, 1.0f);
        target = 1.0f + (1.0f - level) * 0.0001f; // overshoot target so level reaches 1
    }

    void noteOff() noexcept
    {
        if (stage != Stage::Idle)
        {
            stage = Stage::Release;
            coeff = computeCoeff(release, 0.0001f);
            target = 0.0f;
        }
    }

    // Returns true while envelope is producing audio
    bool isActive() const noexcept { return stage != Stage::Idle; }

    // Per-sample tick — returns current envelope level
    float tick() noexcept
    {
        switch (stage)
        {
        case Stage::Attack:
            level += coeff * (1.001f - level);
            if (level >= 1.0f)
            {
                level = 1.0f;
                stage = Stage::Decay;
                coeff = computeCoeff(decay, sustain);
                target = sustain;
            }
            break;

        case Stage::Decay:
            level += coeff * (sustain - level);
            if (std::fabs(level - sustain) < 0.0001f)
            {
                level = sustain;
                stage = Stage::Sustain;
            }
            break;

        case Stage::Sustain:
            level = sustain;
            break;

        case Stage::Release:
            level += coeff * (0.0f - level);
            if (level < 0.0001f)
            {
                level = 0.0f;
                stage = Stage::Idle;
            }
            break;

        case Stage::Idle:
        default:
            level = 0.0f;
            break;
        }
        return level;
    }

private:
    // Compute per-sample coefficient for exponential approach:
    // coefficient = 1 - exp(-log(1/threshold) / (timeSec * sampleRate))
    float computeCoeff(float timeSec, float /*targetLevel*/) const noexcept
    {
        return 1.0f - fastExp(-6.907755f / (timeSec * sampleRate));
    }
};

//==============================================================================
class Voice
{
public:
    //==========================================================================
    // Voice allocation state — read directly by the adapter
    uint64_t noteOnOrder = 0;
    uint64_t noteOffOrder = 0;
    bool held = false;
    int midiNote = -1;

    // Envelope — referenced directly by the adapter: v.env.setParams(...)
    VoiceEnvelope env;

    // Glide coefficient — set once per block by the adapter from olap_glide.
    // 0 = instant (no glide); values near 1 = very slow glide.
    float glideCoeff = 0.005f;

    //==========================================================================
    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        invSr = 1.0f / sr;
        phase = 0.0f;
        glideFreq = 0.0f;
        velocity = 0.0f;
        voiceIdx = 0;
        env.prepare(sr);
    }

    void setVoiceIndex(int idx) noexcept { voiceIdx = idx; }

    void reset() noexcept
    {
        phase = 0.0f;
        held = false;
        midiNote = -1;
        velocity = 0.0f;
        glideFreq = 0.0f;
        env.reset();
    }

    bool isActive() const noexcept { return env.isActive(); }

    //==========================================================================
    void noteOn(int note, float vel, uint64_t order) noexcept
    {
        midiNote = note;
        velocity = vel;
        held = true;
        noteOnOrder = order;
        noteOffOrder = 0;

        float targetFreq = midiToFreq(note);
        if (glideFreq < 1.0f)
            glideFreq = targetFreq;
        targetGlideFreq = targetFreq;

        // Reset phase for clean attack
        phase = 0.0f;
        env.noteOn();
    }

    void noteOff(uint64_t order) noexcept
    {
        held = false;
        noteOffOrder = order;
        env.noteOff();
    }

    //==========================================================================
    // process() — advance oscillator one sample and return output.
    // pulseRate (0.01–8 Hz) drives a pulsed waveform character:
    //   at low pulse rate → near-sine
    //   at high pulse rate → short burst / click (impulsive FDN excitation)
    float process(float pulseRate) noexcept
    {
        if (!env.isActive())
            return 0.0f;

        // Glide: one-pole smoothing toward target frequency (coeff set per-block)
        glideFreq += glideCoeff * (targetGlideFreq - glideFreq);

        // Advance oscillator phase
        float phaseInc = glideFreq * invSr;
        phase += phaseInc;
        if (phase >= 1.0f)
            phase -= 1.0f;

        // Waveform: blend sine with a narrow Gaussian pulse at phase = 0.
        // pulseRate / maxPulse controls the burst sharpness.
        // We use the continuous phase to generate a sine, then modulate
        // its envelope with a pulse-width function driven by pulseRate.
        float sineOut = fastSin(phase * 6.28318530f);

        // Pulse modulation: at high pulseRate, multiply by a raised cosine
        // window that peaks once per cycle.  Width = 1/pulseRate normalized.
        const float maxPulse = 8.0f;
        float normalizedPulse = juce::jmin(pulseRate / maxPulse, 1.0f);
        float halfWidth = 0.5f * (1.0f - normalizedPulse * 0.85f);
        float distFromCenter = std::fabs(phase - 0.5f); // 0 at cycle center
        float pulseEnv;
        if (distFromCenter < halfWidth)
            pulseEnv = 0.5f + 0.5f * fastCos((distFromCenter / halfWidth) * 3.14159265f);
        else
            pulseEnv = 0.0f;

        // Blend: low pulse rate → pure sine; high pulse rate → pulsed output
        float oscOut = sineOut * (1.0f - normalizedPulse) + pulseEnv * normalizedPulse;

        // Velocity scaling on amplitude
        float envLevel = env.tick();
        return oscOut * envLevel * velocity;
    }

    // phase is public — accessed by Entrainment (Kuramoto coupling)
    float phase = 0.0f;

    // targetGlideFreq is public — written by the adapter for Ocean Current drift (D004)
    float targetGlideFreq = 440.0f;

    // velocity is read by the legato retrigger path in the adapter
    float velocity = 0.0f;

private:
    //==========================================================================
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    // F04: invSr must NOT be initialised as 1/sr at declaration time (sr=0 → 1/0 = Inf/UB).
    // Initialise to 0.0f sentinel; prepare() will overwrite with the correct reciprocal.
    float invSr = 0.0f; // sentinel: 0.0 until prepare() sets it (was 1/44100, which hardcoded SR)
    float glideFreq = 0.0f;
    int voiceIdx = 0;
};

} // namespace xoverlap
