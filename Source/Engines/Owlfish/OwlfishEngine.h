#pragma once

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "OwlfishVoice.h"
#include "OwlfishParamSnapshot.h"
#include "OwlfishParameters.h"
#include <vector>

namespace xowlfish {

//==============================================================================
// OwlfishEngine -- XOlokun SynthEngine adapter for the monophonic owlfish.
//
// This adapter wraps OwlfishVoice as a single-organ monophonic instrument
// inside the XOlokun multi-engine chassis. It translates the XOlokun
// SynthEngine interface into calls on the standalone voice, handling:
//
//   - Monophonic MIDI routing with lastNoteOn tracking
//   - Per-sample output coupling cache (outputCacheL/R)
//   - Parameter snapshot delegation via attachTo() / updateFrom()
//   - Frozen owl_ parameter namespace (never changes after release)
//
// Coupling stubs are left empty for now -- the MegaCouplingMatrix will wire
// them when the organism is registered in the XOlokun engine registry.
//
// Signal flow is identical to standalone: the voice processes its full
// organ chain (Abyss Habitat -> Owl Optics -> Diet -> Sacrificial Armor
// -> Amp Envelope -> Abyss Reverb -> Output) and the adapter just
// frames it for multi-engine life.
//==============================================================================

class OwlfishEngine final : public xolokun::SynthEngine
{
public:
    OwlfishEngine() = default;

    //--------------------------------------------------------------------------
    /// Prepare the organism for playback at the given sample rate.
    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        maxBlock = maxBlockSize;
        voice.prepare (sampleRate);
        aftertouch.prepare (sampleRate);
        outputCacheL.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        lastNoteOn = -1;
        silenceGate.prepare (sampleRate, maxBlockSize);
    }

    //--------------------------------------------------------------------------
    void releaseResources() override {}

    //--------------------------------------------------------------------------
    /// Reset the organism to silence. Re-prepare the voice and clear caches.
    void reset() override
    {
        voice.prepare (sr);
        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
        lastNoteOn = -1;
    }

    //--------------------------------------------------------------------------
    /// Render a block of audio through the full owlfish organ chain.
    /// Monophonic: only the most recent note sounds. NoteOff only releases
    /// when the matching note is released (prevents legato cutoff).
    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // Read parameters once per block (snapshot reads from cached atomics)
        snapshot.updateFrom();

        // Process MIDI -- monophonic with lastNoteOn tracking
        for (const auto metadata : midi)
        {
            auto msg = metadata.getMessage();

            if (msg.isNoteOn())
            {
                silenceGate.wake();
                lastNoteOn = msg.getNoteNumber();
                voice.noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), snapshot);
            }
            else if (msg.isNoteOff())
            {
                // Only release if this note-off matches the currently playing note
                if (msg.getNoteNumber() == lastNoteOn)
                {
                    voice.noteOff();
                    lastNoteOn = -1;
                }
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                voice.noteOff();
                lastNoteOn = -1;
            }
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure (msg.getChannelPressureValue() / 127.0f);
            // D006: mod wheel (CC#1) → mixtur depth (Mixtur-Trautonium subharmonic presence)
            // Wheel up = deeper subharmonic stack from the Abyss Habitat — the owlfish
            // descends deeper, expanding its resonant body. Full wheel adds +0.45 to subMix.
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
            else if (msg.isPitchWheel()) pitchBendNorm = xolokun::PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }

        if (silenceGate.isBypassed() && midi.isEmpty()) { buffer.clear(); return; }

        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);

        // D006: aftertouch increases grain density — denser predatory grain cloud on pressure
        // (sensitivity 0.25). Full pressure adds up to +0.25 to grainDensity (range 0–1),
        // raising cloud from ~10 grains/sec toward ~200 grains/sec. The owlfish hunts harder.
        snapshot.grainDensity = std::clamp (snapshot.grainDensity + atPressure * 0.25f, 0.0f, 1.0f);

        // D006: mod wheel deepens subharmonic mix — full wheel adds +0.45 to subMix.
        // The owlfish descends deeper into its Mixtur-Trautonium resonant abyss.
        snapshot.subMix = std::clamp (snapshot.subMix + modWheelAmount * 0.45f + couplingSubMod, 0.0f, 1.0f);

        // DSP FIX: Apply coupling modulation to grain density (AmpToFilter → grain cloud)
        snapshot.grainDensity = std::clamp (snapshot.grainDensity + couplingGrainMod, 0.0f, 1.0f);

        // Reset coupling accumulators after use
        couplingGrainMod = 0.0f;
        couplingSubMod = 0.0f;
        couplingPitchMod = 0.0f; // applied above as combinedSemitones offset to applyPitchBend

        // Apply pitch bend and LFOToPitch coupling to voice target frequency.
        // couplingPitchMod is in semitones (±1 at amount=1.0); combined with pitch
        // bend before passing the unified ratio to the voice so both offsets compose.
        if (voice.isActive())
        {
            float combinedSemitones = pitchBendNorm * 2.0f + couplingPitchMod;
            voice.applyPitchBend (xolokun::PitchBendUtil::semitonesToFreqRatio (combinedSemitones));
        }

        // Render the organism
        buffer.clear();
        auto* outL = buffer.getWritePointer (0);
        auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : outL;

        if (voice.isActive())
            voice.process (outL, outR, numSamples, snapshot);

        // Cache output for per-sample coupling reads
        for (int i = 0; i < numSamples && i < static_cast<int> (outputCacheL.size()); ++i)
        {
            outputCacheL[static_cast<size_t> (i)] = outL[i];
            outputCacheR[static_cast<size_t> (i)] = outR[i];
        }

        silenceGate.analyzeBlock (buffer.getReadPointer (0), buffer.getReadPointer (1), numSamples);
    }

    //--------------------------------------------------------------------------
    /// Return a cached output sample for cross-engine coupling.
    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0) return 0.0f;
        auto si = static_cast<size_t> (sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];
        return 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Coupling input — routes external modulation into the owlfish organ chain.
    void applyCouplingInput (xolokun::CouplingType type,
                             float amount,
                             const float* sourceBuffer,
                             int numSamples) override
    {
        (void) sourceBuffer;
        (void) numSamples;

        switch (type)
        {
            case xolokun::CouplingType::LFOToPitch:
                // External LFO modulates pitch — applied as pitch offset in semitones.
                // Max ±1 semitone at amount=1.0 — subtle organic drift from partner engine.
                couplingPitchMod = amount * 1.0f;
                break;

            case xolokun::CouplingType::AmpToFilter:
                // External amplitude modulates grain density — partner loudness
                // increases predatory grain cloud activity.
                couplingGrainMod = amount * 0.3f;
                break;

            case xolokun::CouplingType::EnvToMorph:
                // External envelope modulates subharmonic mix — partner dynamics
                // push the owlfish deeper into its Mixtur-Trautonium resonant abyss.
                couplingSubMod = amount * 0.2f;
                break;

            default:
                break;
        }
    }

    //--------------------------------------------------------------------------
    /// Create the frozen owl_ parameter layout for XOlokun APVTS.
    juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout() override
    {
        return xowlfish::createParameterLayout();
    }

    //--------------------------------------------------------------------------
    /// Attach parameter snapshot to the XOlokun APVTS.
    /// Called once after the host creates the value tree.
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        snapshot.attachTo (apvts);
    }

    //--------------------------------------------------------------------------
    /// Engine identity -- used by EngineRegistry and preset routing.
    juce::String getEngineId() const override { return "Owlfish"; }

    /// Accent colour -- Abyssal Gold, the owlfish's bioluminescent signature.
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFB8860B); }

    /// Max voices -- the owlfish is a solitary organism. Always 1.
    int getMaxVoices() const override { return 1; }

    /// Active voice count -- 0 or 1.
    int getActiveVoiceCount() const override { return voice.isActive() ? 1 : 0; }

private:
    xolokun::SilenceGate      silenceGate;
    OwlfishVoice               voice;
    OwlfishParamSnapshot       snapshot;
    xolokun::PolyAftertouch   aftertouch;
    std::vector<float>         outputCacheL, outputCacheR;

    int    lastNoteOn    = -1;
    double sr            = 44100.0;
    int    maxBlock      = 512;

    // D006: mod wheel (CC#1) — deepens mixtur subharmonic mix (+0.45 at full wheel)
    float  modWheelAmount = 0.0f;

    float  pitchBendNorm    = 0.0f;  // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // Coupling modulation accumulators (DSP FIX: was no-op stub)
    float  couplingPitchMod = 0.0f;  // semitones from LFOToPitch
    float  couplingGrainMod = 0.0f;  // grain density boost from AmpToFilter
    float  couplingSubMod   = 0.0f;  // subharmonic mix boost from EnvToMorph
};

} // namespace xowlfish
