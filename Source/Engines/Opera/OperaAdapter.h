#pragma once
//==============================================================================
// OperaAdapter.h — XOmnibus adapter for opera::OperaEngine
//
// Engine: XOpera — Kuramoto-coupled additive-vocal synthesis
// Accent: Aria Gold #D4AF37 | Prefix: opera_ | Voices: 8
// Creature: The Humpback Whale (Mesopelagic/SOFAR)
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "OperaEngine.h"

namespace xomnibus {

class OperaAdapter : public SynthEngine
{
public:
    OperaAdapter() = default;

    juce::String getEngineId() const override { return engine_.getEngineId(); }
    juce::Colour getAccentColour() const override { return engine_.getAccentColour(); }
    int getMaxVoices() const override { return engine_.getMaxVoices(); }
    int getActiveVoiceCount() const override { return engine_.getActiveVoiceCount(); }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        engine_.prepare(sampleRate, maxBlockSize);
        prepareSilenceGate(sampleRate, maxBlockSize, 500.0f);
    }

    void releaseResources() override { engine_.releaseResources(); }
    void reset() override { engine_.reset(); }

    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi, int numSamples) override
    {
        for (const auto& metadata : midi)
        {
            if (metadata.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        }

        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        engine_.renderBlock(buffer, midi, numSamples);
        analyzeForSilenceGate(buffer, numSamples);
    }

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        return engine_.getSampleForCoupling(channel, sampleIndex);
    }

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override
    {
        engine_.applyCouplingInput(static_cast<int>(type), amount, sourceBuffer, numSamples);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        return opera::OperaEngine::createParameterLayout();
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        engine_.attachParameters(apvts);
    }

private:
    opera::OperaEngine engine_;
};

} // namespace xomnibus
