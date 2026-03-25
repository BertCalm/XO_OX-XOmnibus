#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>  // juce::MidiMessageCollector
#include "Core/SynthEngine.h"
#include "Core/EngineRegistry.h"
#include "Core/MegaCouplingMatrix.h"
#include "Core/CouplingCrossfader.h"
#include "Core/MasterFXChain.h"
#include "Core/ChordMachine.h"
#include "Core/MPEManager.h"
#include "Core/MIDILearnManager.h"
#include "Core/PresetManager.h"
#include "Core/CouplingPresetManager.h"
#include "DSP/EngineProfiler.h"
#include "DSP/SRO/SROAuditor.h"
#include <atomic>
#include <memory>

namespace xolokun {

class XOlokunProcessor : public juce::AudioProcessor
{
public:
    XOlokunProcessor();
    ~XOlokunProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "XOlokun"; }
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

    // ── Field Map note event queue ─────────────────────────────────────────────
    // Lock-free SPSC ring: audio thread writes (pushNoteEvent), UI thread drains
    // (drainNoteEvents). Both ends use only std::atomic<size_t> indices — no mutex,
    // no heap allocation after init. Power-of-two size for fast modulo.
    struct NoteMapEvent {
        int   midiNote;   // 0–127
        float velocity;   // 0.0–1.0
        int   slot;       // 0–3 (which engine slot played the note)
    };
    static constexpr size_t kNoteQueueSize = 1024; // power-of-two

    // Push from audio thread (non-blocking; drops event if queue is full)
    void pushNoteEvent(int midiNote, float velocity, int slot) noexcept
    {
        size_t head = noteQueueHead.load(std::memory_order_relaxed);
        size_t tail = noteQueueTail.load(std::memory_order_acquire);
        size_t nextHead = (head + 1) & (kNoteQueueSize - 1);
        if (nextHead == tail) return; // full — drop event rather than block
        noteQueue[head] = { midiNote, velocity, slot };
        noteQueueHead.store(nextHead, std::memory_order_release);
    }

    // Drain from UI/message thread — calls visitor for each pending event
    template <typename Fn>
    void drainNoteEvents(Fn&& visitor)
    {
        size_t tail = noteQueueTail.load(std::memory_order_relaxed);
        size_t head = noteQueueHead.load(std::memory_order_acquire);
        while (tail != head)
        {
            visitor(noteQueue[tail]);
            tail = (tail + 1) & (kNoteQueueSize - 1);
        }
        noteQueueTail.store(tail, std::memory_order_release);
    }

    // Coupling matrix — access for the UI visualization (message thread)
    const MegaCouplingMatrix& getCouplingMatrix() const { return couplingMatrix; }
    MegaCouplingMatrix& getCouplingMatrix() { return couplingMatrix; }
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

    // Coupling preset management — bake, save, load coupling presets (UI thread only)
    CouplingPresetManager& getCouplingPresetManager() { return couplingPresetManager; }

    // PlaySurface MIDI bridge — events from the on-screen PlaySurface are
    // enqueued here on the message thread and merged into processBlock's
    // MidiBuffer each audio callback.  Thread-safe by JUCE contract.
    juce::MidiMessageCollector& getMidiCollector() { return playSurfaceMidiCollector; }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    MegaCouplingMatrix couplingMatrix;
    CouplingCrossfader couplingCrossfader;

    // Pre-allocated route buffer for performance coupling overlay — avoids
    // heap allocation (make_shared/vector) on the audio thread each block.
    // mergedRoutePtr wraps a pre-reserved vector (capacity set in prepareToPlay);
    // processBlock clears and refills it from the baseline + perf routes.
    // The shared_ptr itself is created once in prepareToPlay; the inner vector
    // never reallocates because its capacity >= MaxRoutes.
    std::shared_ptr<std::vector<MegaCouplingMatrix::CouplingRoute>> mergedRoutePtr;

    MasterFXChain masterFX;
    ChordMachine chordMachine;
    MPEManager mpeManager;
    MIDILearnManager midiLearnManager;
    PresetManager presetManager;
    CouplingPresetManager couplingPresetManager;

    // Engine slots — shared_ptr for atomic swap between message and audio threads.
    // The audio thread reads via atomic_load; the message thread writes via atomic_store.
    struct EngineSlot {
        std::shared_ptr<SynthEngine> engine;
    };
    std::array<std::shared_ptr<SynthEngine>, MaxSlots> engines;

    // Crossfade state for engine hot-swap — audio-thread-only after the
    // pending command is consumed.  Never touched by the message thread.
    struct CrossfadeState {
        std::shared_ptr<SynthEngine> outgoing;  // engine being faded out
        float fadeGain = 0.0f;                    // 1.0 → 0.0 during crossfade
        int fadeSamplesRemaining = 0;
    };
    std::array<CrossfadeState, MaxSlots> crossfades;

    // Lock-free mailbox: message thread writes a pending swap command; audio
    // thread drains it at the top of processBlock.  Single-producer /
    // single-consumer per slot — only one swap can be in-flight at a time
    // because loadEngine/unloadEngine run on the message (UI) thread which is
    // single-threaded.  `ready` is the SPSC handshake flag:
    //   message thread: fill fields → release-store ready=true
    //   audio   thread: acquire-load ready → consume fields → store ready=false
    // No mutex needed; no heap allocation on the audio thread.
    struct PendingCrossfade {
        std::shared_ptr<SynthEngine> outgoing;
        float fadeGain               = 0.0f;
        int   fadeSamplesRemaining   = 0;
        std::atomic<bool> ready      { false };

        // Non-copyable — std::atomic<bool> cannot be copy/move-assigned.
        PendingCrossfade() = default;
        PendingCrossfade(const PendingCrossfade&) = delete;
        PendingCrossfade& operator=(const PendingCrossfade&) = delete;
    };
    std::array<PendingCrossfade, MaxSlots> pendingCrossfades;

    std::array<juce::AudioBuffer<float>, MaxSlots> engineBuffers;
    juce::AudioBuffer<float> crossfadeBuffer;
    std::array<juce::MidiBuffer, MaxSlots> slotMidi;  // per-slot MIDI from ChordMachine

    // External audio input capture — sized once in prepareToPlay, NEVER resized in processBlock.
    // OsmosisEngine reads raw pointers into this buffer within the same processBlock call.
    juce::AudioBuffer<float> externalInputBuffer;

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

        // Coupling performance overlay — 4 route slots × 5 params = 20 params
        // Cached for audio-thread access (no string lookups in processBlock)
        struct CouplingRouteParams {
            std::atomic<float>* active = nullptr;   // cp_rN_active
            std::atomic<float>* type   = nullptr;   // cp_rN_type
            std::atomic<float>* amount = nullptr;   // cp_rN_amount
            std::atomic<float>* source = nullptr;   // cp_rN_source
            std::atomic<float>* target = nullptr;   // cp_rN_target
        };
        std::array<CouplingRouteParams, CouplingCrossfader::MaxRouteSlots> cpRoutes;
    } cachedParams;

    juce::MidiBuffer mpeMidiBuffer;  // MPE-processed MIDI (expression stripped)

    // Field Map SPSC queue storage (audio-thread write / UI-thread read)
    std::array<NoteMapEvent, kNoteQueueSize> noteQueue {};
    std::atomic<size_t> noteQueueHead { 0 };
    std::atomic<size_t> noteQueueTail { 0 };

    // PlaySurface MIDI collector — message thread enqueues, processBlock drains.
    juce::MidiMessageCollector playSurfaceMidiCollector;

    void cacheParameterPointers();
    void processFamilyBleed(std::array<SynthEngine*, MaxSlots>& enginePtrs);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOlokunProcessor)
};

} // namespace xolokun
