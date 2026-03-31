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
#include "Core/MacroSystem.h"
#include "DSP/EngineProfiler.h"
#include "DSP/SRO/SROAuditor.h"
#include <atomic>
#include <array>
#include <memory>

namespace xolokun {

// ── Per-slot waveform FIFO ─────────────────────────────────────────────────
// Lock-free SPSC ring: audio thread writes (push), UI thread reads
// (readLatest) at ~30Hz for oscilloscope display.  Power-of-two size for
// fast bitwise-AND masking — no modulo, no allocation.
//
// Thread-safety contract:
//   push()       — called ONLY from the audio thread
//   readLatest() — called ONLY from the message/UI thread
//
// Overwrite policy: if the UI hasn't drained since the last push the oldest
// data is silently overwritten.  This is intentional — stale waveform pixels
// are less harmful than blocking the audio thread.
struct WaveformFifo
{
    static constexpr size_t kSize = 512; // power-of-two (~10 ms @ 48 kHz)

    std::array<float, kSize> buffer {};
    std::atomic<size_t>      writeHead { 0 };

    // Default-constructible so it can live in a std::array.
    WaveformFifo()  = default;

    // Non-copyable — std::atomic is not copy/move-assignable.
    WaveformFifo(const WaveformFifo&)            = delete;
    WaveformFifo& operator=(const WaveformFifo&) = delete;

    // Audio thread: copy `count` samples into the ring and advance writeHead.
    // Never allocates.  Safe to call from processBlock with any block size.
    //
    // ARM memory ordering note: on ARMv7/AArch64, stores to the buffer array
    // and the writeHead store may be reordered by the CPU unless we insert an
    // explicit release fence between them.  Using memory_order_release on the
    // store alone is sufficient on x86 (TSO), but on ARM the standard mandates
    // only that the *store itself* is release-ordered with respect to subsequent
    // acquires on the same atomic — it does not prevent earlier non-atomic stores
    // (the buffer writes) from being observed after the atomic store.  The
    // explicit fence + relaxed store pattern is the portable ARM-safe idiom.
    void push(const float* samples, size_t count) noexcept
    {
        size_t head = writeHead.load(std::memory_order_relaxed);
        for (size_t i = 0; i < count; ++i)
            buffer[(head + i) & (kSize - 1)] = samples[i];
        std::atomic_thread_fence(std::memory_order_release);  // ARM safety: flush buffer[] before advancing head
        writeHead.store((head + count) & (kSize - 1),
                        std::memory_order_relaxed);
    }

    // UI thread: copy the most recent `count` samples into dest[0..count-1],
    // where dest[0] is the oldest sample in the window.
    // If count > kSize the leading samples are zero-filled.
    void readLatest(float* dest, size_t count) const noexcept
    {
        size_t head = writeHead.load(std::memory_order_acquire);
        if (count > kSize)
        {
            size_t pad = count - kSize;
            for (size_t i = 0; i < pad; ++i)
                dest[i] = 0.0f;
            dest  += pad;
            count  = kSize;
        }
        // Walk backwards from writeHead to find the start of the window,
        // then read forward so dest[0] is oldest and dest[count-1] is newest.
        size_t start = (head - count) & (kSize - 1);
        for (size_t i = 0; i < count; ++i)
            dest[i] = buffer[(start + i) & (kSize - 1)];
    }
};

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
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 22.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Preset management (UI thread only)
    PresetManager& getPresetManager() { return presetManager; }
    void applyPreset(const PresetData& preset);

    // Engine slot management (message thread only)
    // Slot 4 (0-indexed) is the Ghost Slot — see EngineRegistry::detectCollection().
    static constexpr int MaxSlots = 5;
    static constexpr float CrossfadeMs = 50.0f;
    void loadEngine(int slot, const std::string& engineId);
    void unloadEngine(int slot);
    SynthEngine* getEngine(int slot) const;

    // Optional callback fired on the message thread after an engine is loaded or unloaded.
    // The editor registers this to refresh only the affected tile immediately.
    std::function<void(int /*slot*/)> onEngineChanged;

    // ── XOuija UI State persistence bridge ────────────────────────────────────
    // PlaySurface registers these callbacks so the processor can include the
    // XOuija panel state (active bank, toggle states, MIDI learn CC mappings)
    // in getStateInformation() and restore it in setStateInformation().
    //
    // Usage (in PlaySurface::setProcessor()):
    //   processor_->onGetXOuijaState = [this]() { return xouijaPanel_.toValueTree(); };
    //   processor_->onSetXOuijaState = [this](const juce::ValueTree& t) {
    //       xouijaPanel_.fromValueTree(t); };
    std::function<juce::ValueTree()>                    onGetXOuijaState;
    std::function<void(const juce::ValueTree& /*state*/)> onSetXOuijaState;

    // ── Field Map note event queue ─────────────────────────────────────────────
    // Lock-free SPSC ring: audio thread writes (pushNoteEvent), UI thread drains
    // (drainNoteEvents). Both ends use only std::atomic<size_t> indices — no mutex,
    // no heap allocation after init. Power-of-two size for fast modulo.
    struct NoteMapEvent {
        int   midiNote;   // 0–127
        float velocity;   // 0.0–1.0
        int   slot;       // 0–4 (which engine slot played the note; 4 = Ghost Slot)
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

    // ── Per-slot waveform FIFOs ────────────────────────────────────────────
    // One WaveformFifo per engine slot.  After each engine renders its block
    // in processBlock, call:
    //   waveformFifos[slot].push(engineBuffers[slot].getReadPointer(0),
    //                            static_cast<size_t>(numSamples));
    // The UI oscilloscope component calls getWaveformFifo(slot).readLatest()
    // at its ~30Hz repaint timer to grab the latest window of samples.
    std::array<WaveformFifo, MaxSlots> waveformFifos;

    // UI-thread accessor — returns a const reference; safe to call at any time.
    const WaveformFifo& getWaveformFifo(int slot) const noexcept
    {
        if (slot < 0 || slot >= MaxSlots) slot = 0;  // safety fallback — clamp OOB slot
        return waveformFifos[static_cast<size_t>(slot)];
    }

    // Coupling matrix — access for the UI visualization (message thread)
    const MegaCouplingMatrix& getCouplingMatrix() const { return couplingMatrix; }
    MegaCouplingMatrix& getCouplingMatrix() { return couplingMatrix; }
    void addCouplingRoute(MegaCouplingMatrix::CouplingRoute route) { couplingMatrix.addRoute(route); }
    // DEPRECATED: no external callers — everyone uses getCouplingMatrix().removeUserRoute() directly.
    void removeCouplingRoute(int srcSlot, int dstSlot, CouplingType type) { couplingMatrix.removeUserRoute(srcSlot, dstSlot, type); }

    // Chord Machine — read access for UI, state control from message thread
    ChordMachine& getChordMachine() { return chordMachine; }

    // MPE Manager — per-note expression for Roli Seaboard, Linnstrument, Sensel, etc.
    MPEManager& getMPEManager() { return mpeManager; }

    // MIDI Learn Manager — CC → parameter mapping with host automation support
    MIDILearnManager& getMIDILearnManager() { return midiLearnManager; }

    // SRO: CPU profiling and resource optimization report (UI thread safe)
    // NOTE: SRO profiling runs in processBlock but no UI surface reads this report yet.
    // Wire to a StatusBar or debug panel in V1.1.
    SROAuditor::Report getSROReport() const { return sroAuditor.getReport(); }

    // Coupling preset management — bake, save, load coupling presets (UI thread only)
    CouplingPresetManager& getCouplingPresetManager() { return couplingPresetManager; }

    // PlaySurface MIDI bridge — events from the on-screen PlaySurface are
    // enqueued here on the message thread and merged into processBlock's
    // MidiBuffer each audio callback.  Thread-safe by JUCE contract.
    juce::MidiMessageCollector& getMidiCollector() { return playSurfaceMidiCollector; }

    // ── CC Output queue (UI thread → audio thread) ───────────────────────────
    // Push a CC event from the UI/message thread; emitted as MIDI output in the
    // next processBlock call. Lock-free SPSC — never blocks the UI thread.
    // Drop semantics: if the queue is full (256 events pending) the event is
    // silently discarded — acceptable for CC output which is not audio-critical.
    void pushCCOutput(uint8_t channel, uint8_t cc, uint8_t value) noexcept
    {
        size_t head = ccOutputHead_.load(std::memory_order_relaxed);
        size_t next = (head + 1) % kCCQueueSize;
        if (next == ccOutputTail_.load(std::memory_order_acquire))
            return; // queue full — drop (acceptable for CC)
        ccOutputQueue_[head] = { channel, cc, value };
        ccOutputHead_.store(next, std::memory_order_release);
    }

    // ── Performance gesture API (message thread safe) ─────────────────────────

    // Mute/unmute a slot. Audio thread reads slotMuted[] before mixing each
    // engine's contribution into the output buffer.
    void setSlotMuted(int slot, bool muted) noexcept
    {
        if (slot >= 0 && slot < MaxSlots)
            slotMuted[slot].store(muted, std::memory_order_relaxed);
    }
    bool isSlotMuted(int slot) const noexcept
    {
        if (slot < 0 || slot >= MaxSlots) return false;
        return slotMuted[slot].load(std::memory_order_relaxed);
    }

    // Fire the chord machine — triggers an immediate one-shot chord using the
    // current palette/voicing on the live root note (or MIDI C4 if no root).
    // Sets the chordFirePending flag; processBlock consumes it on the next block.
    void fireChordMachine() noexcept
    {
        chordFirePending.store(true, std::memory_order_release);
    }

    // Trigger a coupling energy burst — temporarily boosts all active coupling
    // route amounts to 1.0 for ~500ms, then decays back to 1.0 multiplier.
    // couplingBurstGain is read by processBlock as a multiplier on route amounts.
    void triggerCouplingBurst() noexcept
    {
        couplingBurstGain.store(kCouplingBurstPeak, std::memory_order_relaxed);
        // Read sample rate via atomicSampleRate_ (safe on message thread).
        // currentSampleRate is NOT used here — it may be concurrently written
        // by prepareToPlay() on the audio thread (data race → UB).
        couplingBurstSamplesRemaining.store(
            static_cast<int>(atomicSampleRate_.load(std::memory_order_relaxed)
                             * kCouplingBurstMs / 1000.0),
            std::memory_order_relaxed);
    }

    // Kill all delay/reverb tails — calls reset() on the master FX chain,
    // clearing all delay buffers and reverb state instantly.
    // Message-thread only: posts an atomic flag consumed by processBlock.
    void killDelayTails() noexcept
    {
        killDelayTailsPending.store(true, std::memory_order_release);
    }

    // CPU processing load as a fraction 0.0–1.0 (or higher during overload).
    // Measured in processBlock() as elapsed wall time / buffer duration.
    // Safe to call from any thread.
    float getProcessingLoad() const noexcept
    {
        return processingLoad.load(std::memory_order_relaxed);
    }

    // Dark Cockpit B041: note activity level (0.0 = silent, 1.0 = max activity).
    // Computed from output RMS in processBlock() with ~100ms attack / ~500ms release.
    // Safe to call from any thread.
    float getNoteActivity() const noexcept { return noteActivity_.load(std::memory_order_relaxed); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    MacroSystem macroSystem_;
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

    std::atomic<double> currentSampleRate { 44100.0 };
    // atomicSampleRate_ mirrors currentSampleRate for safe cross-thread reads.
    // Written in prepareToPlay() (same time as currentSampleRate), read on the
    // message thread in triggerCouplingBurst(). currentSampleRate is now also
    // atomic, eliminating the data race (FIX 4).
    std::atomic<double> atomicSampleRate_ { 44100.0 };
    std::atomic<int> currentBlockSize { 512 };

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

    // ── CC Output SPSC queue (UI-thread write / audio-thread read) ────────────
    // Carries CC events from XOuija (message thread) to MIDI output (audio thread).
    // Head written by UI thread; tail read/advanced by audio thread.
    struct CCOutputEvent {
        uint8_t channel    = 0;   // 0-15 (MIDI channel minus 1)
        uint8_t controller = 0;   // CC number (85-90 for XOuija)
        uint8_t value      = 0;   // 0-127
    };
    static constexpr size_t kCCQueueSize = 256;
    std::array<CCOutputEvent, kCCQueueSize> ccOutputQueue_ {};
    std::atomic<size_t> ccOutputHead_ { 0 };   // written by UI thread
    std::atomic<size_t> ccOutputTail_ { 0 };   // read by audio thread

    // PlaySurface MIDI collector — message thread enqueues, processBlock drains.
    juce::MidiMessageCollector playSurfaceMidiCollector;

    // Set to true once setStateInformation() successfully restores saved state.
    // Used by prepareToPlay() to load a default engine on first launch (no saved state).
    // Must be atomic: setStateInformation runs on the message thread, prepareToPlay
    // can be called from any thread depending on host. (Audit P0-2 CRITICAL-1)
    std::atomic<bool> hasRestoredState { false };

    // ── Per-slot mute state ───────────────────────────────────────────────────
    // Written by message thread (setSlotMuted), read by audio thread per block.
    std::array<std::atomic<bool>, MaxSlots> slotMuted {};  // default false

    // ── CC11 Expression pedal — per-channel tracking (audio thread only) ────────
    // expressionValue_[ch]: 0.0–1.0, updated from CC11 events in processBlock().
    // CC11 passes through to all engine slots for per-engine handling.
    // Processor-level value is available for future coupling/macro use.
    std::array<float, 16> expressionValue_ {};  // default 0.0 (no expression)

    // ── CC64 sustain pedal — fleet-wide hold (audio thread only) ─────────────
    // sustainHeld_[ch]: true while CC64 >= 64 on MIDI channel ch (0-based).
    // sustainPendingNoteOffs_[slot][ch]: bitmask of notes (0–127) whose note-off
    // was suppressed while sustain was held; released when the pedal lifts.
    // All fields are audio-thread-only — no atomics needed.
    std::array<bool, 16> sustainHeld_ {};
    // 128 notes × 16 channels × MaxSlots — stored as uint64_t[2] bitmasks per channel.
    // Bit N is set when note N is pending release on that slot+channel.
    struct SustainNoteSet {
        uint64_t lo = 0;  // notes 0–63
        uint64_t hi = 0;  // notes 64–127
        void set(int note)   { if (note < 64) lo |= (1ULL << note); else hi |= (1ULL << (note - 64)); }
        void clear(int note) { if (note < 64) lo &= ~(1ULL << note); else hi &= ~(1ULL << (note - 64)); }
        bool test(int note) const { return note < 64 ? (lo >> note) & 1 : (hi >> (note - 64)) & 1; }
        bool any() const { return lo || hi; }
        void clearAll() { lo = 0; hi = 0; }
    };
    // [slot][channel]
    std::array<std::array<SustainNoteSet, 16>, MaxSlots> sustainPendingNoteOffs_ {};

    // ── Chord machine fire pending flag ──────────────────────────────────────
    // message thread sets → audio thread consumes and clears.
    std::atomic<bool> chordFirePending { false };

    // ── Chord fire deferred note-off ──────────────────────────────────────────
    // When chordFirePending is consumed, note-ons are injected immediately and
    // chordFireNoteOffCountdown is set to ~kChordHoldMs worth of samples.
    // Each subsequent block decrements the countdown; when it reaches zero the
    // audio thread injects note-offs for the stored chordFireNotes.
    // This replaces the old same-block note-off which gave only ~10ms of hold.
    std::atomic<int> chordFireNoteOffCountdown { 0 };
    static constexpr int kChordHoldMs = 100;  // hold chord for ~100ms before note-off
    std::array<std::atomic<int>, kChordSlots> chordFireNotes {};  // notes fired; 0 = empty (MIDI note 0 / C-1 excluded as sentinel)

    // ── Coupling burst state ──────────────────────────────────────────────────
    // couplingBurstGain: multiplier applied to all active route amounts (1.0 = no boost).
    // Starts at kCouplingBurstPeak when triggered, decays to 1.0 over kCouplingBurstMs.
    static constexpr float kCouplingBurstPeak = 2.0f;   // max boost factor
    static constexpr float kCouplingBurstMs   = 500.0f; // decay duration in ms
    std::atomic<float> couplingBurstGain          { 1.0f };
    std::atomic<int>   couplingBurstSamplesRemaining { 0 };

    // ── Kill delay tails pending flag ────────────────────────────────────────
    // message thread sets → audio thread consumes (calls masterFX.reset()) and clears.
    std::atomic<bool> killDelayTailsPending { false };

    // ── CPU processing load ───────────────────────────────────────────────────
    // Updated each block: elapsed / buffer_duration. Smoothed with a leaky integrator.
    std::atomic<float> processingLoad { 0.0f };

    // ── Dark Cockpit B041: note activity ─────────────────────────────────────
    // RMS-derived signal (0.0 = silent, 1.0 = maximum) smoothed with one-pole
    // filter (~100ms attack, ~500ms release). Read by getCockpitOpacity() in editor.
    std::atomic<float> noteActivity_ { 0.0f };
    // Timestamp of the start of the current processBlock call (high-res ticks).
    juce::int64 processBlockStartTick { 0 };

    void cacheParameterPointers();
    void processFamilyBleed(std::array<SynthEngine*, MaxSlots>& enginePtrs);

    // Drain the CC output queue into the MIDI output buffer.
    // Called from the audio thread at the end of processBlock.
    // Events are placed at numSamples-1 (end of block) so they don't
    // precede audio events generated earlier in the same block.
    void drainCCOutput(juce::MidiBuffer& midiOut, int numSamples) noexcept
    {
        size_t tail = ccOutputTail_.load(std::memory_order_relaxed);
        size_t head = ccOutputHead_.load(std::memory_order_acquire);
        while (tail != head)
        {
            const auto& evt = ccOutputQueue_[tail];
            midiOut.addEvent(
                juce::MidiMessage::controllerEvent(evt.channel + 1, evt.controller, evt.value),
                numSamples - 1);
            tail = (tail + 1) % kCCQueueSize;
        }
        ccOutputTail_.store(tail, std::memory_order_release);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOlokunProcessor)
};

} // namespace xolokun
