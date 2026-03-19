#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Core/SynthEngine.h"
#include "Core/EngineRegistry.h"
#include "Core/MegaCouplingMatrix.h"
#include "Core/MasterFXChain.h"
#include "DSP/Effects/AquaticFXSuite.h"
#include "DSP/Effects/MathFXChain.h"
#include "DSP/Effects/BoutiqueFXChain.h"
#include "Core/ChordMachine.h"
#include "Core/MPEManager.h"
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

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    MegaCouplingMatrix couplingMatrix;
    AquaticFXSuite aquaticFX;
    MathFXChain mathFX;
    BoutiqueFXChain boutiqueFX;
    MasterFXChain masterFX;
    ChordMachine chordMachine;
    MPEManager mpeManager;
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

        // Aquatic FX Suite
        std::atomic<float>* aquaFathomDepth = nullptr;
        std::atomic<float>* aquaFathomPressure = nullptr;
        std::atomic<float>* aquaFathomMix = nullptr;
        std::atomic<float>* aquaDriftRate = nullptr;
        std::atomic<float>* aquaDriftWidth = nullptr;
        std::atomic<float>* aquaDriftDepth = nullptr;
        std::atomic<float>* aquaDriftMix = nullptr;
        std::atomic<float>* aquaTideRate = nullptr;
        std::atomic<float>* aquaTideShape = nullptr;
        std::atomic<float>* aquaTideTarget = nullptr;
        std::atomic<float>* aquaTideMix = nullptr;
        std::atomic<float>* aquaReefSize = nullptr;
        std::atomic<float>* aquaReefDamping = nullptr;
        std::atomic<float>* aquaReefDensity = nullptr;
        std::atomic<float>* aquaReefMix = nullptr;
        std::atomic<float>* aquaSurfaceLevel = nullptr;
        std::atomic<float>* aquaSurfaceTension = nullptr;
        std::atomic<float>* aquaSurfaceMix = nullptr;
        std::atomic<float>* aquaBiolumeGlow = nullptr;
        std::atomic<float>* aquaBiolumeSpectrum = nullptr;
        std::atomic<float>* aquaBiolumeDecay = nullptr;
        std::atomic<float>* aquaBiolumeMix = nullptr;

        // Mathematical FX Chain (CC 30-33)
        std::atomic<float>* mfxEcStability = nullptr;
        std::atomic<float>* mfxEcCoolRate = nullptr;
        std::atomic<float>* mfxEcThreshold = nullptr;
        std::atomic<float>* mfxEcMix = nullptr;
        std::atomic<float>* mfxVsCrystallize = nullptr;
        std::atomic<float>* mfxVsTension = nullptr;
        std::atomic<float>* mfxVsGrainSize = nullptr;
        std::atomic<float>* mfxVsMix = nullptr;
        std::atomic<float>* mfxQsObservation = nullptr;
        std::atomic<float>* mfxQsFeedback = nullptr;
        std::atomic<float>* mfxQsDelayCenter = nullptr;
        std::atomic<float>* mfxQsMix = nullptr;
        std::atomic<float>* mfxAdBifurcation = nullptr;
        std::atomic<float>* mfxAdDriveBase = nullptr;
        std::atomic<float>* mfxAdSpeed = nullptr;
        std::atomic<float>* mfxAdMix = nullptr;

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
