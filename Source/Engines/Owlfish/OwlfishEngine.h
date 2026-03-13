#pragma once

#include "../../Core/SynthEngine.h"
#include "OwlfishVoice.h"
#include "OwlfishParamSnapshot.h"
#include "OwlfishParameters.h"
#include <vector>

namespace xowlfish {

//==============================================================================
// OwlfishEngine -- XOmnibus SynthEngine adapter for the monophonic owlfish.
//
// This adapter wraps OwlfishVoice as a single-organ monophonic instrument
// inside the XOmnibus multi-engine chassis. It translates the XOmnibus
// SynthEngine interface into calls on the standalone voice, handling:
//
//   - Monophonic MIDI routing with lastNoteOn tracking
//   - Per-sample output coupling cache (outputCacheL/R)
//   - Parameter snapshot delegation via attachTo() / updateFrom()
//   - Frozen owl_ parameter namespace (never changes after release)
//
// Coupling stubs are left empty for now -- the MegaCouplingMatrix will wire
// them when the organism is registered in the XOmnibus engine registry.
//
// Signal flow is identical to standalone: the voice processes its full
// organ chain (Abyss Habitat -> Owl Optics -> Diet -> Sacrificial Armor
// -> Amp Envelope -> Abyss Reverb -> Output) and the adapter just
// frames it for multi-engine life.
//==============================================================================

class OwlfishEngine final : public xomnibus::SynthEngine
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
        outputCacheL.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        lastNoteOn = -1;
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
        // Read parameters once per block (snapshot reads from cached atomics)
        snapshot.updateFrom();

        // Process MIDI -- monophonic with lastNoteOn tracking
        for (const auto metadata : midi)
        {
            auto msg = metadata.getMessage();

            if (msg.isNoteOn())
            {
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
    }

    //--------------------------------------------------------------------------
    /// Return a cached output sample for cross-engine coupling.
    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto si = static_cast<size_t> (sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];
        return 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Coupling input stub -- to be wired by MegaCouplingMatrix later.
    void applyCouplingInput (xomnibus::CouplingType type,
                             float amount,
                             const float* sourceBuffer,
                             int numSamples) override
    {
        (void) type;
        (void) amount;
        (void) sourceBuffer;
        (void) numSamples;
    }

    //--------------------------------------------------------------------------
    /// Create the frozen owl_ parameter layout for XOmnibus APVTS.
    juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout() override
    {
        return xowlfish::createParameterLayout();
    }

    //--------------------------------------------------------------------------
    /// Attach parameter snapshot to the XOmnibus APVTS.
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
    OwlfishVoice         voice;
    OwlfishParamSnapshot snapshot;
    std::vector<float>   outputCacheL, outputCacheR;

    int    lastNoteOn = -1;
    double sr         = 44100.0;
    int    maxBlock   = 512;
};

} // namespace xowlfish
