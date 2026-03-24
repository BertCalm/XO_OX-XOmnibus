#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include <cstdint>

#include "OxytocinVoice.h"
#include "OxytocinMemory.h"
#include "OxytocinTriangle.h"
#include "OxytocinParamSnapshot.h"

/// LFO with 5 waveforms matching ParamSnapshot lfoShape.
/// D005 floor lowered to 0.001 Hz — allows ~16-minute modulation cycles.
class OxyLFO
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; phase = 0.0f; shPhase = 0.0f; shValue = 0.0f; }

    /// Returns bipolar value [-1..1].
    float tick (float rateHz, int shape) noexcept
    {
        float safeRate = std::max (0.001f, rateHz);   // D005 floor (was 0.01f)
        float inc = static_cast<float> (safeRate / sr);

        phase += inc;
        if (phase >= 1.0f)
        {
            phase -= 1.0f;
            // S&H: new random value on each cycle
            if (shape == 4)
            {
                shSeed ^= shSeed << 13;
                shSeed ^= shSeed >> 17;
                shSeed ^= shSeed << 5;
                shValue = static_cast<float> (static_cast<int32_t>(shSeed)) * (1.0f / 2147483648.0f);
            }
        }

        switch (shape)
        {
            case 0:  // Sine
                return std::sin (phase * juce::MathConstants<float>::twoPi);
            case 1:  // Triangle
                return (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
            case 2:  // Saw
                return 2.0f * phase - 1.0f;
            case 3:  // Square
                return (phase < 0.5f) ? 1.0f : -1.0f;
            case 4:  // S&H
                return shValue;
            default:
                return std::sin (phase * juce::MathConstants<float>::twoPi);
        }
    }

private:
    double   sr       = 0.0;   // P1-7: default 0 so prepare() is always required
    float    phase    = 0.0f;
    float    shPhase  = 0.0f;
    float    shValue  = 0.0f;
    uint32_t shSeed   = 87654321u;
};

/// OxytocinEngine — top-level polyphonic engine.
/// Manages 8 voices (LRU stealing), two LFOs, global memory, and MIDI.
class OxytocinEngine
{
public:
    static constexpr int MaxVoices = 8;

    OxytocinEngine() = default;

    void prepare (double sampleRate, int maxBlockSize) noexcept
    {
        jassert (sampleRate > 0.0);  // P1-7: catch un-prepared usage
        sr = sampleRate;
        allocatedBlockSize = maxBlockSize;

        // P0-1: use HeapBlock sized to maxBlockSize to prevent stack overrun
        monoBuffer.realloc (maxBlockSize);
        juce::FloatVectorOperations::clear (monoBuffer.getData(), maxBlockSize);

        for (auto& v : voices)
            v.prepare (sampleRate);
        lfo1.prepare (sampleRate);
        lfo2.prepare (sampleRate);
        memory.reset();
        tick = 0;
    }

    void noteOn (int midiNote, int velocity) noexcept
    {
        float velF = static_cast<float> (velocity) / 127.0f;
        int   idx  = findFreeVoice (midiNote);
        voices[idx].noteOn (midiNote, velF);
        voices[idx].lastUseTick = ++tick;
    }

    void noteOff (int midiNote) noexcept
    {
        for (auto& v : voices)
            if (v.isActive() && v.note == midiNote)
                v.noteOff();
    }

    void pitchWheel (int value) noexcept
    {
        // ±2 semitones
        float bend = (static_cast<float> (value - 8192) / 8192.0f) * 2.0f;
        float ratio = std::pow (2.0f, bend / 12.0f);
        for (auto& v : voices)
            v.setPitchBend (ratio);
        pitchBendRatio = ratio;
    }

    // D006: mod wheel → entanglement boost
    void modWheel (int value) noexcept
    {
        modWheelValue = static_cast<float> (value) / 127.0f;
    }

    // D006: aftertouch → passion boost
    void aftertouch (int value) noexcept
    {
        aftertouchValue = static_cast<float> (value) / 127.0f;
    }

    /// Main process block.  Clears and fills the stereo output buffer.
    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer&          midiMessages,
                       ParamSnapshot&             snap) noexcept
    {
        juce::ScopedNoDenormals noDenormals;
        const int numSamples = buffer.getNumSamples();
        jassert (numSamples <= allocatedBlockSize);  // P0-1: guard
        buffer.clear();

        // Process MIDI events (sample-accurate position ignored for simplicity — block-level)
        for (const auto meta : midiMessages)
        {
            auto msg = meta.getMessage();
            if      (msg.isNoteOn())         noteOn  (msg.getNoteNumber(), msg.getVelocity());
            else if (msg.isNoteOff())        noteOff (msg.getNoteNumber());
            else if (msg.isPitchWheel())     pitchWheel (msg.getPitchWheelValue());
            else if (msg.isController())
            {
                if (msg.getControllerNumber() == 1)  modWheel (msg.getControllerValue());
            }
            else if (msg.isChannelPressure()) aftertouch (msg.getChannelPressureValue());
            else if (msg.isAftertouch())     aftertouch (msg.getAfterTouchValue());
        }

        // D006: apply mod wheel and aftertouch to this block's snap
        snap.entanglement = std::clamp (snap.entanglement + modWheelValue * 0.5f, 0.0f, 1.0f);
        snap.passion      = std::clamp (snap.passion      + aftertouchValue * 0.3f, 0.0f, 1.0f);

        // Honour voice count from param
        int maxV = std::clamp (snap.voices, 1, MaxVoices);

        // LFO ticks (block-rate approximation — tick once per block)
        float lfo1Val = lfo1.tick (snap.lfoRate,  snap.lfoShape);
        float lfo2Val = lfo2.tick (snap.lfo2Rate, 0 /*sine always for triangle modulation*/);

        // M2 (MOVEMENT): scale all envelope rates
        // Handled via snap directly — MOVEMENT macro scales rates in host.

        // LFO2 → CHARACTER position for triangle
        float charPos = 0.5f + lfo2Val * snap.lfo2Depth * 0.5f;
        charPos = std::clamp (charPos, 0.0f, 1.0f);
        auto triangleCoords = OxytocinTriangle::fromCharacterPosition (charPos);

        // Sum active voices
        bool anyActive = false;
        float sumI = 0.0f, sumP = 0.0f, sumC = 0.0f;
        int   activeCount = 0;

        for (int vi = 0; vi < maxV; ++vi)
        {
            auto& v = voices[vi];
            if (!v.isActive()) continue;

            // P0-1: clear only the needed samples in the heap buffer
            juce::FloatVectorOperations::clear (monoBuffer.getData(), numSamples);

            // Apply LFO1 to cutoff (depth → frequency modulation in semitones)
            // We modify a local copy of snap for this voice
            ParamSnapshot voiceSnap = snap;
            voiceSnap.cutoff *= std::pow (2.0f, lfo1Val * snap.lfoDepth * 2.0f / 12.0f);

            // LFO2 → triangle position modulates I/P/C balance
            // Blend snap params toward triangle coords by lfo2 depth
            float blend = snap.lfo2Depth;
            voiceSnap.intimacy   = snap.intimacy   * (1.0f - blend) + triangleCoords.I * blend;
            voiceSnap.passion    = snap.passion    * (1.0f - blend) + triangleCoords.P * blend;
            voiceSnap.commitment = snap.commitment * (1.0f - blend) + triangleCoords.C * blend;

            // D004: pass voice index so detune can spread voices
            v.processBlock (monoBuffer.getData(), numSamples, voiceSnap, memory, vi, maxV);
            anyActive = true;

            // Accumulate love values for memory
            sumI += v.lastEffI;
            sumP += v.lastEffP;
            sumC += v.lastEffC;
            ++activeCount;

            // Mix to stereo with pan
            float panL = std::sqrt (std::max (0.0f, 0.5f - snap.pan * 0.5f));
            float panR = std::sqrt (std::max (0.0f, 0.5f + snap.pan * 0.5f));

            auto* outL = buffer.getWritePointer (0);
            auto* outR = buffer.getWritePointer (1);
            for (int s = 0; s < numSamples; ++s)
            {
                outL[s] += monoBuffer[s] * panL;
                outR[s] += monoBuffer[s] * panR;
            }
        }

        // Update global memory
        float avgI = (activeCount > 0) ? (sumI / activeCount) : 0.0f;
        float avgP = (activeCount > 0) ? (sumP / activeCount) : 0.0f;
        float avgC = (activeCount > 0) ? (sumC / activeCount) : 0.0f;
        float blockTime = static_cast<float> (numSamples) / static_cast<float> (sr);
        memory.update (avgI, avgP, avgC, anyActive, snap.memoryDepth, snap.memoryDecay, blockTime);

        // Apply master output gain
        float gainLinear = std::pow (10.0f, snap.output / 20.0f);

        // Simple voice count normalisation (prevent loudness spike with many voices)
        if (activeCount > 1)
            gainLinear /= std::sqrt (static_cast<float> (activeCount));

        buffer.applyGain (gainLinear);

        // Clip guard
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int s = 0; s < numSamples; ++s)
                data[s] = std::clamp (data[s], -1.0f, 1.0f);
        }
    }

    const OxytocinMemory& getMemory() const noexcept { return memory; }

private:
    int findFreeVoice (int midiNote) noexcept
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
        int    oldest    = 0;
        uint64_t oldestT = voices[0].lastUseTick;
        for (int i = 1; i < MaxVoices; ++i)
            if (voices[i].lastUseTick < oldestT)
            {
                oldestT = voices[i].lastUseTick;
                oldest  = i;
            }
        return oldest;
    }

    double   sr = 0.0;   // P1-7: default 0 so prepare() is always required
    int      allocatedBlockSize = 0;
    uint64_t tick = 0;
    float    pitchBendRatio = 1.0f;

    // D006: expression input state
    float    modWheelValue  = 0.0f;   // 0..1, last received CC1
    float    aftertouchValue = 0.0f;  // 0..1, last received aftertouch

    // P0-1: HeapBlock replaces fixed std::array<float, 4096>
    juce::HeapBlock<float> monoBuffer;

    std::array<OxytocinVoice, MaxVoices> voices;
    OxytocinMemory  memory;
    OxyLFO          lfo1;
    OxyLFO          lfo2;
};
