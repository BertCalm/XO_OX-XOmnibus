#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Core/SynthEngine.h"
#include "Core/EngineRegistry.h"
#include "Core/MegaCouplingMatrix.h"
#include "Core/MasterFXChain.h"
#include "Core/ChordMachine.h"
#include "Core/MPEManager.h"
#include "Core/MIDILearnManager.h"
#include "Core/PresetManager.h"
#include "DSP/EngineProfiler.h"
#include "DSP/SRO/SROAuditor.h"
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
    double getTailLengthSeconds() const override { return 6.0; }

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

    // Chord Machine — read access for UI, state control from message thread
    ChordMachine& getChordMachine() { return chordMachine; }

    // MPE Manager — per-note expression for Roli Seaboard, Linnstrument, Sensel, etc.
    MPEManager& getMPEManager() { return mpeManager; }

    // MIDI Learn Manager — CC → parameter mapping with host automation support
    MIDILearnManager& getMIDILearnManager() { return midiLearnManager; }

    // SRO: CPU profiling and resource optimization report (UI thread safe)
    SROAuditor::Report getSROReport() const { return sroAuditor.getReport(); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    MegaCouplingMatrix couplingMatrix;
    MasterFXChain masterFX;
    ChordMachine chordMachine;
    MPEManager mpeManager;
    MIDILearnManager midiLearnManager;
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
    std::array<juce::MidiBuffer, MaxSlots> slotMidi;  // per-slot MIDI from ChordMachine

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // SRO: Per-slot CPU profiling + fleet-wide auditor
    std::array<EngineProfiler, MaxSlots> engineProfilers;
    SROAuditor sroAuditor;

    // Cached raw parameter pointers — resolved once in prepareToPlay, read per-block.
    // Eliminates string-based hash map lookups from the audio thread.
    struct CachedParams {
        std::atomic<float>* masterVolume = nullptr;
        std::atomic<float>* cmEnabled = nullptr;
        std::atomic<float>* cmPalette = nullptr;
        std::atomic<float>* cmVoicing = nullptr;
        std::atomic<float>* cmSpread = nullptr;
        std::atomic<float>* cmSeqRunning = nullptr;
        std::atomic<float>* cmSeqBpm = nullptr;
        std::atomic<float>* cmSeqSwing = nullptr;
        std::atomic<float>* cmSeqGate = nullptr;
        std::atomic<float>* cmSeqPattern = nullptr;
        std::atomic<float>* cmVelCurve = nullptr;
        std::atomic<float>* cmHumanize = nullptr;
        std::atomic<float>* cmSidechainDuck = nullptr;
        std::atomic<float>* cmEnoMode = nullptr;
        // Family bleed params — cached to avoid string lookups on audio thread
        std::atomic<float>* ohmCommune  = nullptr;
        std::atomic<float>* obblBond    = nullptr;
        std::atomic<float>* oleDrama    = nullptr;

        // MPE parameters
        std::atomic<float>* mpeEnabled = nullptr;
        std::atomic<float>* mpeZone = nullptr;
        std::atomic<float>* mpePitchBendRange = nullptr;
        std::atomic<float>* mpePressureTarget = nullptr;
        std::atomic<float>* mpeSlideTarget = nullptr;
    } cachedParams;

    juce::MidiBuffer mpeMidiBuffer;  // MPE-processed MIDI (expression stripped)

    void cacheParameterPointers();
    void processFamilyBleed(std::array<SynthEngine*, MaxSlots>& enginePtrs);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOmnibusProcessor)
};

} // namespace xomnibus
