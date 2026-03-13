#pragma once

#include "../../Core/SynthEngine.h"
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
        }

        // 3. Clear buffer and render
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
                            const float* sourceBuffer,
                            int numSamples) override
    {
        // Translate XOmnibus CouplingType to XOcelot StrataModulation
        // These accumulate and are consumed by the next renderBlock via EcosystemMatrix
        // For now, route supported types to coupling cache
        // (Full routing wired when running inside XOmnibus)
        (void)type; (void)amount; (void)sourceBuffer; (void)numSamples;
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
    std::vector<float> outputCacheL, outputCacheR;
    juce::AudioProcessorValueTreeState* apvtsRef = nullptr;
    double sr = 44100.0;
    int maxBlock = 512;
};

} // namespace xocelot
