// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include <cstdint>

#include "../../DSP/FastMath.h"
#include "OxytocinVoice.h"
#include "OxytocinMemory.h"
#include "OxytocinTriangle.h"
#include "OxytocinParamSnapshot.h"
#include "../../DSP/FastMath.h" // fastPow2, dbToGain

namespace xoxytocin
{

/// LFO with 5 waveforms matching ParamSnapshot lfoShape.
/// D005 floor lowered to 0.001 Hz — allows ~16-minute modulation cycles.
class OxyLFO
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        phase = 0.0f;
        shPhase = 0.0f;
        shValue = 0.0f;
    }

    /// Returns bipolar value [-1..1].
    /// blockSize must be passed so the block-rate tick advances phase correctly.
    /// Without this, the LFO ticks once per block but only advances one sample's
    /// worth of phase — causing the effective rate to be divided by blockSize. (F11)
    float tick(float rateHz, int shape, int blockSize = 1) noexcept
    {
        float safeRate = std::max(0.001f, rateHz); // D005 floor (was 0.01f)
        // F11 fix: advance by blockSize samples so block-rate ticking matches
        // per-sample rate. The LFO is evaluated once per block; its phase must
        // jump by the same amount as if it had been ticked blockSize times.
        float inc = static_cast<float>(safeRate / sr) * static_cast<float>(blockSize);

        phase += inc;
        if (phase >= 1.0f)
        {
            // Wrap phase — use fmodf to handle cases where inc > 1.0 (very fast
            // LFO at large block sizes) without getting stuck in an infinite loop.
            phase = std::fmod(phase, 1.0f);
            // S&H: new random value on each cycle
            if (shape == 4)
            {
                shSeed ^= shSeed << 13;
                shSeed ^= shSeed >> 17;
                shSeed ^= shSeed << 5;
                shValue = static_cast<float>(static_cast<int32_t>(shSeed)) * (1.0f / 2147483648.0f);
            }
        }

        switch (shape)
        {
        case 0: // Sine — fastSin is block-rate here; ~0.01% error (sufficient for LFO)
            return xoceanus::fastSin(phase * juce::MathConstants<float>::twoPi);
        case 1: // Triangle
            return (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
        case 2: // Saw
            return 2.0f * phase - 1.0f;
        case 3: // Square
            return (phase < 0.5f) ? 1.0f : -1.0f;
        case 4: // S&H
            return shValue;
        default:
            return xoceanus::fastSin(phase * juce::MathConstants<float>::twoPi);
        }
    }

private:
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float phase = 0.0f;
    float shPhase = 0.0f;
    float shValue = 0.0f;
    uint32_t shSeed = 87654321u;
};

/// OxytocinEngine — top-level polyphonic engine.
/// Manages 8 voices (LRU stealing), two LFOs, global memory, and MIDI.
class OxytocinEngine
{
public:
    static constexpr int MaxVoices = 8;

    OxytocinEngine() = default;

    void prepare(double sampleRate, int maxBlockSize) noexcept
    {
        jassert(sampleRate > 0.0); // P1-7: catch un-prepared usage
        sr = sampleRate;
        allocatedBlockSize = maxBlockSize;

        // P0-1: use HeapBlock sized to maxBlockSize to prevent stack overrun
        monoBuffer.realloc(maxBlockSize);
        juce::FloatVectorOperations::clear(monoBuffer.getData(), maxBlockSize);
        // ADDITIVE: stereo scratch for additive mixing
        scratchL.realloc(maxBlockSize);
        scratchR.realloc(maxBlockSize);
        juce::FloatVectorOperations::clear(scratchL.getData(), maxBlockSize);
        juce::FloatVectorOperations::clear(scratchR.getData(), maxBlockSize);

        for (auto& v : voices)
            v.prepare(sampleRate);
        lfo1.prepare(sampleRate);
        lfo2.prepare(sampleRate);
        memory.reset();
        tick = 0;
    }

    void noteOn(int midiNote, int velocity) noexcept
    {
        float velF = static_cast<float>(velocity) / 127.0f;
        int idx = findFreeVoice(midiNote);
        voices[idx].noteOn(midiNote, velF);
        voices[idx].lastUseTick = ++tick;
    }

    void noteOff(int midiNote) noexcept
    {
        for (auto& v : voices)
            if (v.isActive() && v.note == midiNote)
                v.noteOff();
    }

    void pitchWheel(int value) noexcept
    {
        // Range is set by RPN 0 (pitch bend sensitivity); defaults to ±2 semitones.
        float bend = (static_cast<float>(value - 8192) / 8192.0f) * static_cast<float>(pitchBendSemitones);
        // F02 fix: use fastPow2 instead of std::pow — called on audio thread during MIDI processing.
        float ratio = xoceanus::fastPow2(bend / 12.0f);
        for (auto& v : voices)
            v.setPitchBend(ratio);
        pitchBendRatio = ratio;
    }

    /// Handle a MIDI controller message. Tracks RPN 0 (pitch bend sensitivity)
    /// via the standard CC 101 / CC 100 / CC 6 / CC 38 sequence.
    void controller(int cc, int val) noexcept
    {
        switch (cc)
        {
        case 1:
            modWheel(val);
            break;

        // RPN select: CC 101 = RPN MSB, CC 100 = RPN LSB
        case 101:
            rpnMsb = static_cast<uint8_t>(val);
            break;
        case 100:
            rpnLsb = static_cast<uint8_t>(val);
            break;

        // Data Entry MSB (CC 6) — sets the value for the selected RPN.
        // RPN 0 (MSB=0, LSB=0) = pitch bend sensitivity in semitones.
        case 6:
            if (rpnMsb == 0 && rpnLsb == 0)
            {
                // MIDI spec: MSB = semitones, LSB (CC 38) = cents (ignored here).
                pitchBendSemitones = std::clamp(val, 1, 24);
            }
            break;

        default:
            break;
        }
    }

    // D006: mod wheel → entanglement boost
    void modWheel(int value) noexcept { modWheelValue = static_cast<float>(value) / 127.0f; }

    // D006: aftertouch → passion boost
    void aftertouch(int value) noexcept { aftertouchValue = static_cast<float>(value) / 127.0f; }

    /// Main process block.  Adds this engine's contribution to the stereo output buffer.
    /// ADDITIVE: does not clear the buffer — uses scratchL/scratchR for isolation.
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, ParamSnapshot& snap) noexcept
    {
        juce::ScopedNoDenormals noDenormals;
        const int numSamples = buffer.getNumSamples();
        jassert(numSamples <= allocatedBlockSize); // P0-1: guard
        // ADDITIVE: render into scratch, then add to output buffer at end
        juce::FloatVectorOperations::clear(scratchL.getData(), numSamples);
        juce::FloatVectorOperations::clear(scratchR.getData(), numSamples);

        // Process MIDI events (sample-accurate position ignored for simplicity — block-level)
        for (const auto meta : midiMessages)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn())
                noteOn(msg.getNoteNumber(), msg.getVelocity());
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber());
            else if (msg.isPitchWheel())
                pitchWheel(msg.getPitchWheelValue());
            else if (msg.isController())
                controller(msg.getControllerNumber(), msg.getControllerValue());
            else if (msg.isChannelPressure())
                aftertouch(msg.getChannelPressureValue());
            else if (msg.isAftertouch())
                aftertouch(msg.getAfterTouchValue());
        }

        // D006: apply mod wheel and aftertouch to this block's snap
        snap.entanglement = std::clamp(snap.entanglement + modWheelValue * 0.5f, 0.0f, 1.0f);
        snap.passion = std::clamp(snap.passion + aftertouchValue * 0.3f, 0.0f, 1.0f);

        // Honour voice count from param
        int maxV = std::clamp(snap.voices, 1, MaxVoices);

        // LFO ticks (block-rate approximation — tick once per block).
        // F11 fix: pass numSamples so each tick advances the correct amount of phase.
        float lfo1Val = lfo1.tick(snap.lfoRate, snap.lfoShape, numSamples);
        float lfo2Val = lfo2.tick(snap.lfo2Rate, 0 /*sine always for triangle modulation*/, numSamples);

        // M2 (MOVEMENT): scale all envelope rates
        // Handled via snap directly — MOVEMENT macro scales rates in host.

        // LFO2 → CHARACTER position for triangle
        float charPos = 0.5f + lfo2Val * snap.lfo2Depth * 0.5f;
        charPos = std::clamp(charPos, 0.0f, 1.0f);
        auto triangleCoords = OxytocinTriangle::fromCharacterPosition(charPos);

        // Sum active voices
        bool anyActive = false;
        float sumI = 0.0f, sumP = 0.0f, sumC = 0.0f;
        int activeCount = 0;

        // F07 fix: hoist pan gain computation out of the voice loop — snap.pan is
        // constant for the block and recomputing sqrt()/max() inside the loop wastes cycles.
        const float panL = std::sqrt(std::max(0.0f, 0.5f - snap.pan * 0.5f));
        const float panR = std::sqrt(std::max(0.0f, 0.5f + snap.pan * 0.5f));

        // F05/F06 fix: pre-compute LFO1 cutoff multiplier and detune ratio at block rate
        // using fastPow2 instead of std::pow (called per active voice in the old code).
        const float lfo1CutoffMult = xoceanus::fastPow2(lfo1Val * snap.lfoDepth * 2.0f / 12.0f);
        // Pan gains are block-constant (snap.pan is stable per block) and identical
        // for every voice — compute once here instead of inside the per-voice loop.
        // TODO(#): intended refactor to use these in the voice loop; currently the
        // loop recomputes per-voice. Kept as [[maybe_unused]] until the refactor lands.
        [[maybe_unused]] const float blockPanL = std::sqrt(std::max(0.0f, 0.5f - snap.pan * 0.5f));
        [[maybe_unused]] const float blockPanR = std::sqrt(std::max(0.0f, 0.5f + snap.pan * 0.5f));

        for (int vi = 0; vi < maxV; ++vi)
        {
            auto& v = voices[vi];
            if (!v.isActive())
                continue;

            // P0-1: clear only the needed samples in the heap buffer
            juce::FloatVectorOperations::clear(monoBuffer.getData(), numSamples);

            // Apply LFO1 to cutoff (depth → frequency modulation in semitones)
            // We modify a local copy of snap for this voice
            // (fastPow2: ~0.1% error — per-voice per-block)
            ParamSnapshot voiceSnap = snap;
            // F05/F06: fastPow2 pre-computed above; * (1.0f/12.0f) avoids per-call division
            voiceSnap.cutoff *= lfo1CutoffMult;

            // LFO2 → triangle position modulates I/P/C balance
            // Blend snap params toward triangle coords by lfo2 depth
            float blend = snap.lfo2Depth;
            voiceSnap.intimacy = snap.intimacy * (1.0f - blend) + triangleCoords.I * blend;
            voiceSnap.passion = snap.passion * (1.0f - blend) + triangleCoords.P * blend;
            voiceSnap.commitment = snap.commitment * (1.0f - blend) + triangleCoords.C * blend;

            // D004: pass voice index so detune can spread voices
            v.processBlock(monoBuffer.getData(), numSamples, voiceSnap, memory, vi, maxV);
            anyActive = true;

            // Accumulate love values for memory
            sumI += v.lastEffI;
            sumP += v.lastEffP;
            sumC += v.lastEffC;
            ++activeCount;

            // Mix to stereo with pan — accumulate into scratch using block-level panL/panR
            auto* sL = scratchL.getData();
            auto* sR = scratchR.getData();
            for (int s = 0; s < numSamples; ++s)
            {
                sL[s] += monoBuffer[s] * panL;
                sR[s] += monoBuffer[s] * panR;
            }
        }

        // Update global memory
        float avgI = (activeCount > 0) ? (sumI / activeCount) : 0.0f;
        float avgP = (activeCount > 0) ? (sumP / activeCount) : 0.0f;
        float avgC = (activeCount > 0) ? (sumC / activeCount) : 0.0f;
        float blockTime = static_cast<float>(numSamples) / static_cast<float>(sr);
        memory.update(avgI, avgP, avgC, anyActive, snap.memoryDepth, snap.memoryDecay, blockTime);

        // Apply master output gain.
        // F04 fix: use dbToGain (fastExp-based) instead of std::pow — audio thread.
        float gainLinear = xoceanus::dbToGain(snap.output);

        // Simple voice count normalisation (prevent loudness spike with many voices)
        if (activeCount > 1)
            gainLinear /= std::sqrt(static_cast<float>(activeCount));

        auto* sL = scratchL.getData();
        auto* sR = scratchR.getData();
        for (int s = 0; s < numSamples; ++s)
        {
            sL[s] *= gainLinear;
            sR[s] *= gainLinear;
        }

        // Clip guard on scratch (only Oxytocin's signal — not earlier engines)
        for (int s = 0; s < numSamples; ++s)
        {
            sL[s] = std::clamp(sL[s], -1.0f, 1.0f);
            sR[s] = std::clamp(sR[s], -1.0f, 1.0f);
        }

        // ADDITIVE: mix processed scratch into the output buffer
        auto* outL = buffer.getWritePointer(0);
        for (int s = 0; s < numSamples; ++s)
            outL[s] += sL[s];
        if (buffer.getNumChannels() > 1)
        {
            auto* outR = buffer.getWritePointer(1);
            for (int s = 0; s < numSamples; ++s)
                outR[s] += sR[s];
        }
    }

    const OxytocinMemory& getMemory() const noexcept { return memory; }

    /// Returns the number of voices currently in their active (non-Idle) state.
    /// Safe to call from the audio thread (all state lives on the audio thread).
    int getActiveVoiceCount() const noexcept
    {
        int count = 0;
        for (const auto& v : voices)
            if (v.isActive())
                ++count;
        return count;
    }

private:
    int findFreeVoice(int midiNote) noexcept
    {
        // 1. Re-trigger same note
        for (int i = 0; i < MaxVoices; ++i)
            if (voices[i].note == midiNote && voices[i].isActive())
                return i;

        // 2. Find idle voice
        for (int i = 0; i < MaxVoices; ++i)
            if (!voices[i].isActive())
                return i;

        // 3. LRU steal: oldest voice
        int oldest = 0;
        uint64_t oldestT = voices[0].lastUseTick;
        for (int i = 1; i < MaxVoices; ++i)
            if (voices[i].lastUseTick < oldestT)
            {
                oldestT = voices[i].lastUseTick;
                oldest = i;
            }
        return oldest;
    }

    double sr = 0.0; // P1-7: default 0 so prepare() is always required
    int allocatedBlockSize = 0;
    uint64_t tick = 0;
    float pitchBendRatio = 1.0f;

    // Pitch bend sensitivity in semitones — set via MIDI RPN 0 (CC101/CC100/CC6).
    // Default 2 semitones per MIDI convention; range [1, 24].
    int pitchBendSemitones = 2;

    // RPN state machine (CC 101 / CC 100 / CC 6) for pitch bend sensitivity.
    uint8_t rpnMsb = 127; // 127 = null RPN (no selection)
    uint8_t rpnLsb = 127;

    // D006: expression input state
    float modWheelValue = 0.0f;   // 0..1, last received CC1
    float aftertouchValue = 0.0f; // 0..1, last received aftertouch

    // P0-1: HeapBlock replaces fixed std::array<float, 4096>
    juce::HeapBlock<float> monoBuffer;
    // ADDITIVE: stereo scratch buffers — Oxytocin renders here, then adds to output buffer
    juce::HeapBlock<float> scratchL;
    juce::HeapBlock<float> scratchR;

    std::array<OxytocinVoice, MaxVoices> voices;
    OxytocinMemory memory;
    OxyLFO lfo1;
    OxyLFO lfo2;
};

} // namespace xoxytocin
