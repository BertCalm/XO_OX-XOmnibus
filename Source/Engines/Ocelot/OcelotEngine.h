#pragma once

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "OcelotVoicePool.h"
#include "OcelotParamSnapshot.h"
#include "OcelotParameters.h"
#include <vector>

namespace xocelot {

class OcelotEngine final : public xomnibus::SynthEngine
{
public:
    OcelotEngine() = default;

    // ── Lifecycle ────────────────────────────────────────
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        maxBlock = maxBlockSize;
        voicePool.prepare(sampleRate);
        aftertouch.prepare(sampleRate);
        outputCacheL.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        snapshot = {};
    }

    void releaseResources() override { /* no-op: all state is member-allocated */ }

    void reset() override
    {
        voicePool.reset();
        std::fill(outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill(outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    // ── Audio ────────────────────────────────────────────
    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi,
                     int numSamples) override
    {
        // 1. Update snapshot from APVTS (once per block)
        if (apvtsRef != nullptr)
            snapshot.updateFrom(*apvtsRef);

        // 2. Process MIDI
        for (const auto metadata : midi)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                voicePool.noteOn(msg.getNoteNumber(),
                                 msg.getFloatVelocity(), snapshot);
            else if (msg.isNoteOff())
                voicePool.noteOff(msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                voicePool.allNotesOff();
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure(msg.getChannelPressureValue() / 127.0f);
            // D006: CC#1 mod wheel → ecosystem depth boost (+0–0.35, stronger cross-stratum modulation)
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
        }

        aftertouch.updateBlock(numSamples);
        const float atPressure = aftertouch.getSmoothedPressure(0);

        // D006: aftertouch increases ecosystem depth — more cross-stratum coupling under pressure
        // (sensitivity 0.3). Full pressure adds up to +0.3 to ecosystemDepth, thickening the
        // 12-route EcosystemMatrix cross-feed between ocelot habitat strata.
        // D006: mod wheel also deepens ecosystem (+0–0.35) — strengthens cross-stratum pathways.
        snapshot.ecosystemDepth = std::clamp(snapshot.ecosystemDepth + atPressure * 0.3f + modWheelAmount * 0.35f, 0.0f, 1.0f);

        // 3. Apply accumulated coupling modulation (consumed once per block)
        snapshot.ecosystemDepth = std::clamp(snapshot.ecosystemDepth + couplingEcosystemMod, 0.0f, 1.0f);
        snapshot.humidity       = std::clamp(snapshot.humidity       + couplingHumidityMod,  0.0f, 1.0f);
        snapshot.canopyWavefold = std::clamp(snapshot.canopyWavefold + couplingWavefoldMod,  0.0f, 1.0f);
        snapshot.floorPitch     = std::clamp(snapshot.floorPitch     + couplingPitchMod,     0.0f, 1.0f);
        snapshot.canopyPitch    = std::clamp(snapshot.canopyPitch    + couplingPitchMod,     0.0f, 1.0f);
        snapshot.creaturePitch  = std::clamp(snapshot.creaturePitch  + couplingPitchMod,     0.0f, 1.0f);
        // Reset for next block
        couplingEcosystemMod = 0.0f;
        couplingHumidityMod  = 0.0f;
        couplingWavefoldMod  = 0.0f;
        couplingPitchMod     = 0.0f;

        // 4. Clear buffer and render
        buffer.clear();
        auto* outL = buffer.getWritePointer(0);
        auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : outL;

        voicePool.renderBlock(outL, outR, numSamples, snapshot);

        // 4. Fill output cache for coupling (post-render, pre-FX)
        for (int i = 0; i < numSamples && i < static_cast<int>(outputCacheL.size()); ++i)
        {
            outputCacheL[static_cast<size_t>(i)] = outL[i];
            outputCacheR[static_cast<size_t>(i)] = outR[i];
        }
    }

    // ── Coupling ─────────────────────────────────────────
    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        auto si = static_cast<size_t>(sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];
        return 0.0f;
    }

    void applyCouplingInput(xomnibus::CouplingType type,
                            float amount,
                            const float* /*sourceBuffer*/,
                            int /*numSamples*/) override
    {
        // Coupling accumulates here; renderBlock consumes and resets each block.
        switch (type)
        {
            case xomnibus::CouplingType::EnvToMorph:
                // Partner envelope sweeps ecosystemDepth — more envelope activity
                // deepens cross-stratum coupling, thickening the biome's internal
                // feed network. Max +0.4 at amount=1.
                couplingEcosystemMod += amount * 0.4f;
                break;

            case xomnibus::CouplingType::AmpToFilter:
                // Partner amplitude modulates humidity — louder partner warms/opens
                // the biome atmosphere. Multiplier: 1 = no change, >1 = warmer.
                couplingHumidityMod = std::clamp(couplingHumidityMod + amount * 0.5f,
                                                  -0.5f, 0.5f);
                break;

            case xomnibus::CouplingType::AudioToFM:
                // Partner audio drives canopy wavefold depth — high-frequency
                // content from a partner adds harmonic folding to the emergent layer.
                couplingWavefoldMod += amount * 0.3f;
                break;

            case xomnibus::CouplingType::LFOToPitch:
            case xomnibus::CouplingType::AmpToPitch:
            case xomnibus::CouplingType::PitchToPitch:
                // Pitch nudge shared across all three strata pitch params.
                // Max ±0.05 semitone equivalent (subtle ecosystem resonance drift).
                couplingPitchMod += amount * 0.05f;
                break;

            default:
                break;
        }
    }

    // ── Parameters ───────────────────────────────────────
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        return xocelot::createParameterLayout();
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        apvtsRef = &apvts;
        // snapshot.updateFrom() is called each renderBlock, using the stored reference
    }

    // ── Identity ─────────────────────────────────────────
    juce::String getEngineId()     const override { return "Ocelot"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFE0A060); }
    int          getMaxVoices()    const override { return OcelotVoicePool::kMaxVoices; }
    int   getActiveVoiceCount()    const override { return voicePool.activeVoiceCount(); }

private:
    OcelotVoicePool voicePool;
    OcelotParamSnapshot snapshot;
    xomnibus::PolyAftertouch aftertouch;
    std::vector<float> outputCacheL, outputCacheR;
    juce::AudioProcessorValueTreeState* apvtsRef = nullptr;
    double sr = 44100.0;
    int maxBlock = 512;

    // ---- D006 Mod wheel — CC#1 deepens ecosystem cross-stratum modulation (+0–0.35) ----
    float modWheelAmount = 0.0f;

    // ---- Coupling accumulators (reset each block after consumption) ----
    float couplingEcosystemMod = 0.0f;  // EnvToMorph → ecosystemDepth offset
    float couplingHumidityMod  = 0.0f;  // AmpToFilter → humidity offset
    float couplingWavefoldMod  = 0.0f;  // AudioToFM → canopyWavefold offset
    float couplingPitchMod     = 0.0f;  // LFO/Amp/PitchToPitch → strata pitch offset
};

} // namespace xocelot
