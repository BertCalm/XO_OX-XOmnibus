#pragma once

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
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
        silenceGate.prepare(sampleRate, maxBlockSize);
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
            {
                silenceGate.wake();
                voicePool.noteOn(msg.getNoteNumber(),
                                 msg.getFloatVelocity(), snapshot);
            }
            else if (msg.isNoteOff())
                voicePool.noteOff(msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                voicePool.allNotesOff();
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure(msg.getChannelPressureValue() / 127.0f);
            // D006: CC#1 mod wheel → ecosystem depth boost (+0–0.35, stronger cross-stratum modulation)
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
            else if (msg.isPitchWheel())
                pitchBendNorm = xomnibus::PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }

        if (silenceGate.isBypassed() && midi.isEmpty()) { buffer.clear(); return; }

        aftertouch.updateBlock(numSamples);
        const float atPressure = aftertouch.getSmoothedPressure(0);

        // D006: aftertouch increases ecosystem depth — more cross-stratum coupling under pressure
        // (sensitivity 0.3). Full pressure adds up to +0.3 to ecosystemDepth, thickening the
        // 12-route EcosystemMatrix cross-feed between ocelot habitat strata.
        // D006: mod wheel also deepens ecosystem (+0–0.35) — strengthens cross-stratum pathways.
        snapshot.ecosystemDepth = std::clamp(snapshot.ecosystemDepth + atPressure * 0.3f + modWheelAmount * 0.35f, 0.0f, 1.0f);

        // Pitch bend: ±2 semitones (propagated to strata via snapshot)
        snapshot.pitchBendSemitones = pitchBendNorm * 2.0f;

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

        silenceGate.analyzeBlock(buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples);
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
        // TODO: applyCouplingInput stub — coupling is currently a no-op. Implement engine-specific modulation routing before V1 ships.
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
    xomnibus::SilenceGate silenceGate;
    OcelotVoicePool voicePool;
    OcelotParamSnapshot snapshot;
    xomnibus::PolyAftertouch aftertouch;
    std::vector<float> outputCacheL, outputCacheR;
    juce::AudioProcessorValueTreeState* apvtsRef = nullptr;
    double sr = 44100.0;
    int maxBlock = 512;

    // ---- D006 Mod wheel — CC#1 deepens ecosystem cross-stratum modulation (+0–0.35) ----
    float modWheelAmount = 0.0f;
    float pitchBendNorm  = 0.0f;
};

} // namespace xocelot
