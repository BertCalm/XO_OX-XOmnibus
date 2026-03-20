#pragma once

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
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
        aftertouch.prepare (sampleRate);
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
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure (msg.getChannelPressureValue() / 127.0f);
            // D006: mod wheel (CC#1) → mixtur depth (Mixtur-Trautonium subharmonic presence)
            // Wheel up = deeper subharmonic stack from the Abyss Habitat — the owlfish
            // descends deeper, expanding its resonant body. Full wheel adds +0.45 to subMix.
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
        }

        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);

        // D006: aftertouch increases grain density — denser predatory grain cloud on pressure
        // (sensitivity 0.25). Full pressure adds up to +0.25 to grainDensity (range 0–1),
        // raising cloud from ~10 grains/sec toward ~200 grains/sec. The owlfish hunts harder.
        snapshot.grainDensity = std::clamp (snapshot.grainDensity + atPressure * 0.25f, 0.0f, 1.0f);

        // D006: mod wheel deepens subharmonic mix — full wheel adds +0.45 to subMix.
        // The owlfish descends deeper into its Mixtur-Trautonium resonant abyss.
        snapshot.subMix = std::clamp (snapshot.subMix + modWheelAmount * 0.45f, 0.0f, 1.0f);

        // Apply coupling accumulators (consumed once per block, then reset)
        snapshot.subMix      = std::clamp (snapshot.subMix      + couplingSubMixMod, 0.0f, 1.0f);
        snapshot.filterCutoff = std::clamp (snapshot.filterCutoff + couplingFilterMod, 0.0f, 1.0f);
        // Pitch coupling: offset all three relevant pitch-like params proportionally
        // (morphGlide is normalised 0–1, so a ±0.05 nudge is a subtle harmonic shift)
        snapshot.morphGlide  = std::clamp (snapshot.morphGlide  + couplingPitchMod,  0.0f, 1.0f);
        couplingSubMixMod = 0.0f;
        couplingFilterMod = 0.0f;
        couplingPitchMod  = 0.0f;

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
    /// Receive modulation from a partner engine via the MegaCouplingMatrix.
    /// Accumulators are consumed once per renderBlock and then reset.
    void applyCouplingInput (xomnibus::CouplingType type,
                             float amount,
                             const float* /*sourceBuffer*/,
                             int /*numSamples*/) override
    {
        switch (type)
        {
            case xomnibus::CouplingType::AudioToFM:
                // Partner audio drives Mixtur-Trautonium subharmonic depth —
                // the incoming signal acts as an FM carrier that deepens the
                // owlfish's subharmonic resonance stack. Max +0.4 at amount=1.
                couplingSubMixMod += amount * 0.4f;
                break;

            case xomnibus::CouplingType::AmpToFilter:
                // Partner amplitude opens the OWL OPTICS filter — louder partner
                // brightens the owlfish's spectral window. Additive cutoff offset
                // (+0–0.25 in normalised filterCutoff units at amount=1).
                couplingFilterMod += amount * 0.25f;
                break;

            case xomnibus::CouplingType::EnvToMorph:
                // Partner envelope sweeps subMix toward deeper body resonance.
                // Envelope peak pushes the harmonic stack into richer territory.
                couplingSubMixMod += amount * 0.3f;
                break;

            case xomnibus::CouplingType::LFOToPitch:
            case xomnibus::CouplingType::AmpToPitch:
            case xomnibus::CouplingType::PitchToPitch:
                // Monophonic pitch nudge — partner engine adds ±0.05 semitone
                // equivalent (normalised) to the owlfish's current pitch state.
                couplingPitchMod += amount * 0.05f;
                break;

            default:
                break;
        }
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
    OwlfishVoice               voice;
    OwlfishParamSnapshot       snapshot;
    xomnibus::PolyAftertouch   aftertouch;
    std::vector<float>         outputCacheL, outputCacheR;

    int    lastNoteOn    = -1;
    double sr            = 44100.0;
    int    maxBlock      = 512;

    // D006: mod wheel (CC#1) — deepens mixtur subharmonic mix (+0.45 at full wheel)
    float  modWheelAmount = 0.0f;

    // Coupling accumulators — consumed once per renderBlock, then reset
    float  couplingSubMixMod = 0.0f;  // AudioToFM / EnvToMorph → subMix offset
    float  couplingFilterMod = 0.0f;  // AmpToFilter → filterCutoff offset
    float  couplingPitchMod  = 0.0f;  // LFO/Amp/PitchToPitch → morphGlide offset
};

} // namespace xowlfish
