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

class OcelotEngine final : public xolokun::SynthEngine
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
                pitchBendNorm = xolokun::PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }

        // Biome crossfade: trigger BiomeMorph in all voices when biome param changes.
        // Comparison uses lastBiome so this fires only on the block where biome changed —
        // not every block — avoiding redundant crossfade resets.
        if (snapshot.biome != lastBiome)
        {
            voicePool.setBiomeTarget(snapshot.biome);
            lastBiome = snapshot.biome;
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

        // Apply coupling input mods (accumulated by applyCouplingInput since last block).
        // Each mod is added to the snapshot field and clamped, then reset to zero.
        if (couplingFilterMod != 0.0f)
        {
            snapshot.canopySpectralFilter = std::clamp(snapshot.canopySpectralFilter + couplingFilterMod, 0.0f, 1.0f);
            couplingFilterMod = 0.0f;
        }
        if (couplingEcosystemMod != 0.0f)
        {
            snapshot.ecosystemDepth = std::clamp(snapshot.ecosystemDepth + couplingEcosystemMod, 0.0f, 1.0f);
            couplingEcosystemMod = 0.0f;
        }
        if (couplingFloorMod != 0.0f)
        {
            snapshot.floorTension = std::clamp(snapshot.floorTension + couplingFloorMod, 0.0f, 1.0f);
            couplingFloorMod = 0.0f;
        }
        if (couplingDensityMod != 0.0f)
        {
            snapshot.density = std::clamp(snapshot.density + couplingDensityMod, 0.0f, 1.0f);
            couplingDensityMod = 0.0f;
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

    void applyCouplingInput(xolokun::CouplingType type,
                            float amount,
                            const float* /*sourceBuffer*/,
                            int /*numSamples*/) override
    {
        // Ocelot coupling: external modulation accumulates into snapshot fields
        // and is consumed in the next renderBlock() call. Each type maps to the
        // most ecologically coherent stratum parameter.
        switch (type)
        {
            case xolokun::CouplingType::AmpToFilter:
                // Partner amplitude (e.g. ONSET drum hits) opens canopy spectral filter.
                // Sensitivity 0.25: prevents filter blow-out from loud percussive partners.
                couplingFilterMod = std::clamp(couplingFilterMod + amount * 0.25f, -0.5f, 0.5f);
                break;

            case xolokun::CouplingType::LFOToPitch:
            case xolokun::CouplingType::AmpToPitch:
            case xolokun::CouplingType::PitchToPitch:
                // Partner pitch/LFO modulates ecosystem depth — more signal flow between strata.
                // Cross-stratum coupling deepens when another engine's melodic content arrives.
                couplingEcosystemMod = std::clamp(couplingEcosystemMod + amount * 0.3f, 0.0f, 0.5f);
                break;

            case xolokun::CouplingType::EnvToMorph:
                // External envelope sweeps floor tension — pluck character changes with
                // the dynamics of the partner engine (feliX's darts make the floor tighter).
                couplingFloorMod = std::clamp(couplingFloorMod + amount * 0.2f, -0.3f, 0.3f);
                break;

            case xolokun::CouplingType::EnvToDecay:
                // External envelope decays → density decreases (space opens up between strata).
                couplingDensityMod = std::clamp(couplingDensityMod - amount * 0.15f, -0.3f, 0.0f);
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
    xolokun::SilenceGate silenceGate;
    OcelotVoicePool voicePool;
    OcelotParamSnapshot snapshot;
    xolokun::PolyAftertouch aftertouch;
    std::vector<float> outputCacheL, outputCacheR;
    juce::AudioProcessorValueTreeState* apvtsRef = nullptr;
    double sr = 44100.0;
    int maxBlock = 512;

    // ---- D006 Mod wheel — CC#1 deepens ecosystem cross-stratum modulation (+0–0.35) ----
    float modWheelAmount = 0.0f;
    float pitchBendNorm  = 0.0f;

    // ---- Biome tracking — prevents redundant setBiomeTarget calls each block ----
    int lastBiome = 0;  // 0=Jungle (matches BiomeMorph::prepare default)

    // ---- Coupling accumulators — reset each block, accumulated by applyCouplingInput ----
    // Applied to snapshot in renderBlock before voice rendering.
    float couplingFilterMod    = 0.0f;  // AmpToFilter → canopySpectralFilter ±0.5
    float couplingEcosystemMod = 0.0f;  // LFO/Amp/PitchToPitch → ecosystemDepth +0–0.5
    float couplingFloorMod     = 0.0f;  // EnvToMorph → floorTension ±0.3
    float couplingDensityMod   = 0.0f;  // EnvToDecay → density -0–0.3
};

} // namespace xocelot
