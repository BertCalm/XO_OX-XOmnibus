#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Core/SynthEngine.h"
#include "Core/EngineRegistry.h"
#include "Core/MegaCouplingMatrix.h"
#include "Core/MasterFXChain.h"
#include "Core/PresetManager.h"
#include <atomic>
#include <memory>

namespace xomnibus {

class XOmnibusProcessor : public juce::AudioProcessor
{
public:
    XOmnibusProcessor();
    ~XOmnibusProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "XOmnibus"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Preset management (UI thread only)
    PresetManager& getPresetManager() { return presetManager; }
    void applyPreset(const PresetData& preset);

    // Engine slot management (message thread only)
    static constexpr int MaxSlots = 4;
    static constexpr float CrossfadeMs = 50.0f;
    void loadEngine(int slot, const std::string& engineId);
    void unloadEngine(int slot);
    SynthEngine* getEngine(int slot) const;

    // Optional callback fired on the message thread after an engine is loaded or unloaded.
    // The editor registers this to refresh only the affected tile immediately.
    std::function<void(int /*slot*/)> onEngineChanged;

    // Coupling matrix — read-only access for the UI visualization (message thread)
    const MegaCouplingMatrix& getCouplingMatrix() const { return couplingMatrix; }
    void addCouplingRoute(MegaCouplingMatrix::CouplingRoute route) { couplingMatrix.addRoute(route); }
    void removeCouplingRoute(int srcSlot, int dstSlot, CouplingType type) { couplingMatrix.removeUserRoute(srcSlot, dstSlot, type); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    MegaCouplingMatrix couplingMatrix;
    MasterFXChain masterFX;
    PresetManager presetManager;

    // Engine slots — shared_ptr for atomic swap between message and audio threads.
    // The audio thread reads via atomic_load; the message thread writes via atomic_store.
    struct EngineSlot {
        std::shared_ptr<SynthEngine> engine;
    };
    std::array<std::shared_ptr<SynthEngine>, MaxSlots> engines;

    // Crossfade state for engine hot-swap
    struct CrossfadeState {
        std::shared_ptr<SynthEngine> outgoing;  // engine being faded out
        float fadeGain = 0.0f;                    // 1.0 → 0.0 during crossfade
        int fadeSamplesRemaining = 0;
    };
    std::array<CrossfadeState, MaxSlots> crossfades;

    std::array<juce::AudioBuffer<float>, MaxSlots> engineBuffers;
    juce::AudioBuffer<float> crossfadeBuffer;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOmnibusProcessor)
};

} // namespace xomnibus
