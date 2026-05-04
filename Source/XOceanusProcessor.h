// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
// [trigger-build: genome batch 35→53/86]
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h> // juce::MidiMessageCollector
#include "Core/SynthEngine.h"
#include "Core/EngineRegistry.h"
#include "Core/MegaCouplingMatrix.h"
#include "Core/CouplingCrossfader.h"
#include "Core/MasterFXChain.h"
#include "Core/EpicChainSlotController.h"
#include "Core/ChordMachine.h"
#include "Core/MPEManager.h"
#include "Core/MIDILearnManager.h"
#include "Core/PresetManager.h"
#include "Core/CouplingPresetManager.h"
#include "Core/MacroSystem.h"
#include "Core/DNAModulationBus.h"
#include "Core/PartnerAudioBus.h"
#include "Core/BrothCoordinator.h"
#include "Core/SharedTransport.h"
#include "Core/SlotModSourceRegistry.h" // Wave5-C5: future msg-thread ModSources
#include "DSP/EngineProfiler.h"
#include "DSP/SRO/SROAuditor.h"
#include "DSP/PerEnginePatternSequencer.h"
// Wave 5 A1: Global drag-drop mod routing model (message-thread side).
// The header lives in Future/ but we reference it in-place per spec.
#include "Future/UI/ModRouting/DragDropModRouter.h"
#include <algorithm>
#include <atomic>
#include <array>
#include <memory>
#include <vector>

namespace xoceanus
{

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

    std::array<float, kSize> buffer{};
    std::atomic<size_t> writeHead{0};

    // Default-constructible so it can live in a std::array.
    WaveformFifo() = default;

    // Non-copyable — std::atomic is not copy/move-assignable.
    WaveformFifo(const WaveformFifo&) = delete;
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
        std::atomic_thread_fence(std::memory_order_release); // ARM safety: flush buffer[] before advancing head
        writeHead.store((head + count) & (kSize - 1), std::memory_order_relaxed);
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
            dest += pad;
            count = kSize;
        }
        // Walk backwards from writeHead to find the start of the window,
        // then read forward so dest[0] is oldest and dest[count-1] is newest.
        size_t start = (head - count) & (kSize - 1);
        for (size_t i = 0; i < count; ++i)
            dest[i] = buffer[(start + i) & (kSize - 1)];
    }
};

class XOceanusProcessor : public juce::AudioProcessor
{
public:
    XOceanusProcessor();
    ~XOceanusProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "XOceanus"; }
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
    juce::UndoManager& getUndoManager() { return undoManager; }

    // Wave 5 A1: Global mod routing model — owned by the processor so both the
    // editor and the audio-thread snapshot path share the same instance.
    // Message-thread only: all ModRoutingModel mutations must happen on the UI thread.
    ModRoutingModel& getModRoutingModel() { return modRoutingModel_; }
    const ModRoutingModel& getModRoutingModel() const { return modRoutingModel_; }

    // Called by the editor (message thread) whenever the route table changes.
    // Copies the route list into a lock-free snapshot array so processBlock can
    // read it without allocating or holding a lock.
    // Max routes: ModRoutingModel::MaxRoutes (32).
    void flushModRoutesSnapshot() noexcept;

    // Wave 5 C5: Read-only access to per-slot sequencer live state.
    // Used by UI components to display live step values; audio thread uses the
    // atomics directly via slotSequencers_ (private, same translation unit).
    const XOceanus::PerEnginePatternSequencer& getSlotSequencer(int slot) const noexcept
    {
        return slotSequencers_[static_cast<size_t>(juce::jlimit(0, kNumPrimarySlots - 1, slot))];
    }

    // Wave 5 A1: Write the current LFO1 output so the global router can use it
    // as a mod source.  Called from OrreryEngine::renderBlock (audio thread).
    // Use relaxed ordering — a single-sample jitter is acceptable for mod routing.
    void setGlobalLFO1(float v) noexcept { globalLFO1_.store(v, std::memory_order_relaxed); }

    // Read the global cutoff mod offset computed from global mod routes.
    // Called by OrreryEngine::renderBlock on the audio thread.
    // Zero when no global route targets orry_fltCutoff.
    float getGlobalCutoffModOffset() const noexcept
    {
        return globalCutoffModOffset_.load(std::memory_order_relaxed);
    }

    // B1: Read the accumulated modOffset for a given route slot (audio-thread only).
    // Returns 0.0f for out-of-range indices.  Engines that opt into generic global
    // mod routing scan routesSnapshot_ on load, remember matching slot indices, then
    // call this each renderBlock to retrieve the current offset.
    // Units: normalised modOffset (depth * srcVal) — multiply by your parameter's
    // range span to convert to parameter units.
    float getModRouteAccum(int routeIdx) const noexcept
    {
        if (routeIdx < 0 || routeIdx >= kMaxGlobalRoutes) return 0.0f;
        return routeModAccum_[static_cast<size_t>(routeIdx)];
    }

    // T6: Raw pointer to the accumulator array for engines that need a forward-
    // declaration-safe consumption path (i.e. header-inline renderBlock methods
    // that cannot include XOceanusProcessor.h without creating a circular dep).
    // Audio-thread only — the array is written then read within the same block.
    // Size = kMaxGlobalRoutes (32); OpalEngine uses cached indices to index it.
    const float* getModRouteAccumPtr() const noexcept { return routeModAccum_.data(); }

    // B1: Read the number of active routes in the snapshot (audio-thread or message-thread).
    int getModRouteCount() const noexcept
    {
        return routesSnapshotCount_.load(std::memory_order_relaxed);
    }

    // T6: Per-route snapshot accessors for engines that opt into generic global
    // mod routing.  Engines call these in their attachParameters() / onModRoutesChanged()
    // pass to find route indices that target their parameters.
    //
    // destParamId: the raw char* stored in the snapshot (fixed-length, null-terminated).
    //   Compare against your OpalParam::GRAIN_SIZE etc. with std::strcmp.
    // velocityScaled: true → routeModAccum_[ri] holds raw depth; engine multiplies by
    //   voice.velocity (or avgVelocity) at consumption.
    // destParamRangeSpan: pre-cached (end - start) for the destination parameter.
    //   Multiply getModRouteAccum(ri) by this to convert normalised offset to param units.
    const char* getModRouteDestParamId(int ri) const noexcept
    {
        if (ri < 0 || ri >= kMaxGlobalRoutes) return "";
        return routesSnapshot_[static_cast<size_t>(ri)].destParamId;
    }
    bool isModRouteVelocityScaled(int ri) const noexcept
    {
        if (ri < 0 || ri >= kMaxGlobalRoutes) return false;
        return routesSnapshot_[static_cast<size_t>(ri)].velocityScaled;
    }
    float getModRouteRangeSpan(int ri) const noexcept
    {
        if (ri < 0 || ri >= kMaxGlobalRoutes) return 0.0f;
        return routesSnapshot_[static_cast<size_t>(ri)].destParamRangeSpan;
    }

    // Preset management (UI thread only)
    PresetManager& getPresetManager() { return presetManager; }
    void applyPreset(const PresetData& preset);

    // ── Per-slot preset data model (#1378) ───────────────────────────────────
    // Stores which preset is currently loaded in each primary slot (0–3).
    // Written on the message thread only; read by UI components (Starboard,
    // EngineOrbit pill) via getSlotPreset() and notified via SlotPresetListener.
    //
    // Listener interface — register via addSlotPresetListener / removeSlotPresetListener.
    // Called on the message thread whenever setSlotPreset() is invoked.
    struct SlotPresetListener
    {
        virtual ~SlotPresetListener() = default;
        virtual void slotPresetChanged(int slotIdx, const PresetData& preset) = 0;
    };

    // Bounds-checked accessor: jassert in debug, clamp to slot 0 in release.
    // Returns a const reference — valid for the lifetime of the processor.
    const PresetData& getSlotPreset(int slotIdx) const noexcept
    {
        jassert(slotIdx >= 0 && slotIdx < kNumPrimarySlots);
        const int safe = juce::jlimit(0, kNumPrimarySlots - 1, slotIdx);
        return slotPresets_[static_cast<size_t>(safe)];
    }

    // Stores the preset for the given slot and notifies all registered listeners.
    // Message thread only.
    void setSlotPreset(int slotIdx, const PresetData& preset);

    void addSlotPresetListener(SlotPresetListener* l)
    {
        if (l != nullptr)
            slotPresetListeners_.push_back(l);
    }
    void removeSlotPresetListener(SlotPresetListener* l)
    {
        slotPresetListeners_.erase(
            std::remove(slotPresetListeners_.begin(), slotPresetListeners_.end(), l),
            slotPresetListeners_.end());
    }

    // Engine slot management (message thread only)
    // Slot 4 (0-indexed) is the Ghost Slot — see EngineRegistry::detectCollection().
    static constexpr int MaxSlots = 5;
    // Primary engine slots (0–3): eligible for per-slot pattern sequencer.
    // The Ghost Slot (index 4) is excluded from sequencer instances.
    static constexpr int kNumPrimarySlots = 4;
    static constexpr float CrossfadeMs = 50.0f;
    void loadEngine(int slot, const std::string& engineId);
    void unloadEngine(int slot);
    SynthEngine* getEngine(int slot) const;

    // Optional callback fired on the message thread after an engine is loaded or unloaded.
    // The editor registers this to refresh only the affected tile immediately.
    std::function<void(int /*slot*/)> onEngineChanged;

    // ── TideWaterline sequence layer persistence bridge (#1179) ───────────────
    // OceanView registers these callbacks so the processor can include per-step
    // data (active, velocity, gate, rootNote for all 16 steps) in DAW state.
    // This is the D1 architectural foundation: step patterns survive DAW session
    // recall independently of any preset load.
    //
    // Usage (in OceanView::initWaterline(), after waterline_ construction):
    //   processor_.onGetTideWaterlineState = [this]() {
    //       return waterline_ ? waterline_->toValueTree() : juce::ValueTree{};
    //   };
    //   processor_.onSetTideWaterlineState = [this](const juce::ValueTree& t) {
    //       if (waterline_) waterline_->fromValueTree(t);
    //   };
    //   // Consume state that arrived before the editor was open:
    //   auto deferred = processor_.getPersistedTideWaterlineState();
    //   if (deferred.isValid() && waterline_)
    //   {
    //       waterline_->fromValueTree(deferred);
    //       processor_.clearPersistedTideWaterlineState();
    //   }
    std::function<juce::ValueTree()> onGetTideWaterlineState;
    std::function<void(const juce::ValueTree& /*state*/)> onSetTideWaterlineState;

    // ── Field Map note event queue ─────────────────────────────────────────────
    // Lock-free SPSC ring: audio thread writes (pushNoteEvent), UI thread drains
    // (drainNoteEvents). Both ends use only std::atomic<size_t> indices — no mutex,
    // no heap allocation after init. Power-of-two size for fast modulo.
    struct NoteMapEvent
    {
        int midiNote;   // 0–127
        float velocity; // 0.0–1.0
        int slot;       // 0–4 (which engine slot played the note; 4 = Ghost Slot)
    };
    static constexpr size_t kNoteQueueSize = 1024; // power-of-two

    // Push from audio thread (non-blocking; drops event if queue is full)
    void pushNoteEvent(int midiNote, float velocity, int slot) noexcept
    {
        size_t head = noteQueueHead.load(std::memory_order_relaxed);
        size_t tail = noteQueueTail.load(std::memory_order_acquire);
        size_t nextHead = (head + 1) & (kNoteQueueSize - 1);
        if (nextHead == tail)
            return; // full — drop event rather than block
        noteQueue[head] = {midiNote, velocity, slot};
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
    WaveformFifo masterOutputFifo;  // master bus output (post-FX) for ocean wave surface

    // UI-thread accessor — returns a const reference; safe to call at any time.
    const WaveformFifo& getWaveformFifo(int slot) const noexcept
    {
        if (slot < 0 || slot >= MaxSlots)
            slot = 0; // safety fallback — clamp OOB slot
        return waveformFifos[static_cast<size_t>(slot)];
    }

    const WaveformFifo& getMasterWaveformFifo() const noexcept { return masterOutputFifo; }

    // Coupling matrix — access for the UI visualization (message thread)
    const MegaCouplingMatrix& getCouplingMatrix() const { return couplingMatrix; }
    MegaCouplingMatrix& getCouplingMatrix() { return couplingMatrix; }
    void addCouplingRoute(MegaCouplingMatrix::CouplingRoute route) { couplingMatrix.addRoute(route); }
    // DEPRECATED: no external callers — everyone uses getCouplingMatrix().removeUserRoute() directly.
    void removeCouplingRoute(int srcSlot, int dstSlot, CouplingType type)
    {
        couplingMatrix.removeUserRoute(srcSlot, dstSlot, type);
    }

    // Chord Machine — read access for UI, state control from message thread
    ChordMachine& getChordMachine() { return chordMachine; }

    // Host transport — read/write access for UI (e.g. TransportBar time-sig wiring).
    // The processor updates this from the PlayHead once per audio block; engines
    // that tempo-sync read BPM/beat/isPlaying here during renderBlock().
    xoceanus::SharedTransport& getHostTransport() { return hostTransport; }

    // Master FX chain — read access for UI (sequencer playhead, etc.)
    MasterFXChain& getMasterFXChain() { return masterFX; }

    // MPE Manager — per-note expression for Roli Seaboard, Linnstrument, Sensel, etc.
    MPEManager& getMPEManager() { return mpeManager; }

    // MIDI Learn Manager — CC → parameter mapping with host automation support
    MIDILearnManager& getMIDILearnManager() { return midiLearnManager; }

    // SRO: CPU profiling and resource optimization report (UI thread safe)
    // NOTE: SRO profiling runs in processBlock but no UI surface reads this report yet.
    // Wire to a StatusBar or debug panel when that surface lands.
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
        ccOutputQueue_[head] = {channel, cc, value};
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
        if (slot < 0 || slot >= MaxSlots)
            return false;
        return slotMuted[slot].load(std::memory_order_relaxed);
    }

    // Fire the chord machine — triggers an immediate one-shot chord using the
    // current palette/voicing on the live root note (or MIDI C4 if no root).
    // Sets the chordFirePending flag; processBlock consumes it on the next block.
    void fireChordMachine() noexcept { chordFirePending.store(true, std::memory_order_release); }

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
            static_cast<int>(atomicSampleRate_.load(std::memory_order_relaxed) * kCouplingBurstMs / 1000.0),
            std::memory_order_relaxed);
    }

    // Kill all delay/reverb tails — calls reset() on the master FX chain,
    // clearing all delay buffers and reverb state instantly.
    // Message-thread only: posts an atomic flag consumed by processBlock.
    void killDelayTails() noexcept { killDelayTailsPending.store(true, std::memory_order_release); }

    // ── Sound on First Launch — replay (§1282 Settings > Experience) ─────────
    // Re-arms the first-breath experience on demand (called from SettingsPanel's
    // "Hear the Greeting Again" button).  Message-thread safe: re-loads Oxbow in
    // slot 0 with the Breath Mist parameters, then sets firstBreathPending_ so
    // processBlock injects the C3 note on the next audio block.
    // Note: this does NOT reset hasLaunchedBefore_ — it only re-arms one play.
    // Idempotent: calling while a breath is already pending is a no-op.
    void replayFirstBreath()
    {
        if (firstBreathPending_.load(std::memory_order_relaxed))
            return; // already armed — don't double-arm
        // Re-load Oxbow in slot 0 so the engine is ready for the note.
        loadEngine(0, "Oxbow");
        // Re-apply the Breath Mist preset parameters inline (same values as first launch).
        struct BreathMistParam { const char* id; float value; };
        static const BreathMistParam kBreathMistParams[] = {
            {"oxb_size",          0.1f},
            {"oxb_decay",         0.5f},
            {"oxb_entangle",      0.06f},
            {"oxb_erosionRate",   0.05f},
            {"oxb_erosionDepth",  0.08f},
            {"oxb_convergence",   4.0f},
            {"oxb_resonanceQ",    3.5f},
            {"oxb_resonanceMix",  0.15f},
            {"oxb_cantilever",    0.12f},
            {"oxb_damping",       7000.0f},
            {"oxb_predelay",      0.0f},
            {"oxb_dryWet",        0.15f},
            {"oxb_exciterDecay",  0.05f},
            {"oxb_exciterBright", 0.5f},
        };
        for (const auto& p : kBreathMistParams)
        {
            if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(p.id)))
                param->setValueNotifyingHost(param->convertTo0to1(p.value));
        }
        firstBreathPending_.store(true, std::memory_order_release);
    }

    // CPU processing load as a fraction 0.0–1.0 (or higher during overload).
    // Measured in processBlock() as elapsed wall time / buffer duration.
    // Safe to call from any thread.
    float getProcessingLoad() const noexcept { return processingLoad.load(std::memory_order_relaxed); }

    // wire(#orphan-sweep item 2): expose first-breath active state to the message thread.
    // firstBreathActive_ is audio-thread-only (no atomic), so we mirror it via an
    // atomic updated each processBlock tick.  The message thread uses this to gate
    // the FirstHourWalkthrough prompt so the tour bubble does not appear while the
    // greeting note is still playing (race fix per Wave 9c spec §1303).
    bool isFirstBreathActive() const noexcept
    {
        return firstBreathActiveMirror_.load(std::memory_order_relaxed);
    }

    // Dark Cockpit B041: note activity level (0.0 = silent, 1.0 = max activity).
    // Computed from output RMS in processBlock() with ~100ms attack / ~500ms release.
    // Safe to call from any thread.
    float getNoteActivity() const noexcept { return noteActivity_.load(std::memory_order_relaxed); }

    // ── Wave5-C5: SlotModSourceRegistry — message-thread-origin ModSources ──────
    // Infrastructure for future UI-gesture-origin ModSources.
    // XouijaCell (ID 18) was removed 2026-05-01.
    SlotModSourceRegistry& getModSourceRegistry() noexcept { return modSourceRegistry_; }
    const SlotModSourceRegistry& getModSourceRegistry() const noexcept { return modSourceRegistry_; }

    // ── #1357: XY Surface position atomics (W8B mount) ───────────────────────
    // Per-slot XY surface position in [0, 1].  Written by SurfaceRightPanel::onXYChanged
    // on the message thread; read by the mod routing system (DragDropModRouter)
    // on the audio thread as ModSourceId::XYX0..XYY3.
    //
    // Thread-safety: std::atomic<float> with relaxed ordering — a one-block-late
    // value is acceptable for a continuous modulation source.
    //
    // setXYPosition: call from SurfaceRightPanel::onXYChanged (message thread).
    // getXYPosition: call from audio thread inside processBlock.
    void setXYPosition(int slot, float x, float y) noexcept
    {
        if (slot < 0 || slot >= kNumPrimarySlots) return;
        xyX_[static_cast<size_t>(slot)].store(x, std::memory_order_relaxed);
        xyY_[static_cast<size_t>(slot)].store(y, std::memory_order_relaxed);
    }
    float getXYX(int slot) const noexcept
    {
        if (slot < 0 || slot >= kNumPrimarySlots) return 0.5f;
        return xyX_[static_cast<size_t>(slot)].load(std::memory_order_relaxed);
    }
    float getXYY(int slot) const noexcept
    {
        if (slot < 0 || slot >= kNumPrimarySlots) return 0.5f;
        return xyY_[static_cast<size_t>(slot)].load(std::memory_order_relaxed);
    }

    // ── Editor UI state persistence (closes #314, #357) ───────────────────────
    // These fields are written on the message thread by the editor/sidebar and
    // read back during setStateInformation to restore UI state.  They are plain
    // ints/bools — no audio-thread access — so no atomics are needed.

    // PlaySurface scale selector index (#314): 0=Chromatic … 6=Mixolydian.
    // PlayControlPanel calls setPlayScaleIndex / getPlayScaleIndex via its
    // processor reference.
    void setPlayScaleIndex(int idx) noexcept { persistedPlayScaleIndex = idx; }
    int getPlayScaleIndex() const noexcept { return persistedPlayScaleIndex; }

    // Editor tile focus (#357-a): selected slot (-1 = overview).
    void setPersistedSelectedSlot(int slot) noexcept { persistedSelectedSlot = slot; }
    int getPersistedSelectedSlot() const noexcept { return persistedSelectedSlot; }

    // Signal flow active section (#357-b): 0=SRC1 … 5=OUT.
    void setPersistedSignalFlowSection(int sec) noexcept { persistedSignalFlowSection = sec; }
    int getPersistedSignalFlowSection() const noexcept { return persistedSignalFlowSection; }

    // Dark Cockpit bypass (#357-c): true = bypass (hold full opacity).
    void setPersistedCockpitBypass(bool v) noexcept { persistedCockpitBypass = v; }
    bool getPersistedCockpitBypass() const noexcept { return persistedCockpitBypass; }

    // D4: Register lock + current register (per-instance, per-DAW-session).
    void setPersistedRegisterLocked(bool v) noexcept { persistedRegisterLocked = v; }
    bool getPersistedRegisterLocked() const noexcept { return persistedRegisterLocked; }
    void setPersistedRegisterCurrent(int r) noexcept { persistedRegisterCurrent = r; }
    int  getPersistedRegisterCurrent() const noexcept { return persistedRegisterCurrent; }

    // P1-8 (1F): Dashboard tab + kit sub-mode persistence.
    void setPersistedDashboardTab(int tab) noexcept    { persistedDashboardTab_ = juce::jlimit(0, 2, tab); }
    int  getPersistedDashboardTab()  const noexcept    { return persistedDashboardTab_; }
    void setPersistedKitSubMode(bool kit) noexcept     { persistedKitSubMode_ = kit; }
    bool getPersistedKitSubMode()  const noexcept      { return persistedKitSubMode_; }

    // F2-006: OceanView ViewState + zoomed slot persistence.
    // Written by OceanView's onStateEntered callback (message thread only).
    void setPersistedOceanViewState(int state) noexcept { persistedOceanViewState_ = state; }
    int  getPersistedOceanViewState() const noexcept    { return persistedOceanViewState_; }
    void setPersistedOceanViewSlot(int slot)  noexcept  { persistedOceanViewSlot_  = slot;  }
    int  getPersistedOceanViewSlot()  const noexcept    { return persistedOceanViewSlot_;  }

    // F3-006: REACT dial level persistence (0.0–1.0; default 0.80).
    // Written by OceanView HUD dial callback; read back in initHudCallbacks().
    void  setPersistedReactLevel(float v) noexcept  { persistedReactLevel_ = juce::jlimit(0.0f, 1.0f, v); }
    float getPersistedReactLevel() const noexcept   { return persistedReactLevel_; }

    // F3-011/F3-017: Sequencer and Chord breakout panel open-state persistence.
    // Written by OceanView when panels open/close; read back after session restore.
    void setPersistedSeqBreakoutOpen(bool v)   noexcept { persistedSeqBreakoutOpen_   = v; }
    bool getPersistedSeqBreakoutOpen()  const noexcept  { return persistedSeqBreakoutOpen_;   }
    void setPersistedChordBreakoutOpen(bool v) noexcept { persistedChordBreakoutOpen_ = v; }
    bool getPersistedChordBreakoutOpen() const noexcept { return persistedChordBreakoutOpen_; }

    // #1179 — TideWaterline deferred state pickup.
    // OceanView calls this in initWaterline() to apply state that arrived via
    // setStateInformation() before the editor window was first opened.
    juce::ValueTree getPersistedTideWaterlineState() const noexcept
    {
        return persistedTideWaterlineState_;
    }
    void clearPersistedTideWaterlineState() noexcept
    {
        persistedTideWaterlineState_ = juce::ValueTree{};
    }

    // ── Settings-drawer session controls (#1359) ─────────────────────────────
    // All setters are message-thread-only; atomics are read by the audio thread.
    // Range-clamping is applied in each setter — callers must not assume unclamped
    // values are stored.

    // Global polyphony cap: 1..32 voices.  Audio thread reads polyphonyCap_ via
    // atomic to apply per-engine voice-count limits (engines honour their own cap
    // if it is lower).  Default 16.
    void setPolyphony(int voices) noexcept
    {
        polyphonyCap_.store(juce::jlimit(1, 32, voices), std::memory_order_relaxed);
    }
    int getPolyphony() const noexcept { return polyphonyCap_.load(std::memory_order_relaxed); }

    // Global voice mode: 0=Poly, 1=Mono, 2=Legato, 3=Unison.  Default 0.
    // Engines that expose their own voiceMode param continue to honour that param;
    // this is the session-level override applied at the processor layer.
    void setVoiceMode(int mode) noexcept
    {
        voiceMode_.store(juce::jlimit(0, 3, mode), std::memory_order_relaxed);
    }
    int getVoiceMode() const noexcept { return voiceMode_.load(std::memory_order_relaxed); }

    // Master tune in Hz.  Range 415.0..466.0 (±1 semitone around A=440).
    // Written to the APVTS "masterTune" param so it is automatable + DAW-persisted.
    // Message-thread only; don't call from the audio thread.
    void setMasterTune(float hz)
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter("masterTune")))
            p->setValueNotifyingHost(p->convertTo0to1(juce::jlimit(415.0f, 466.0f, hz)));
    }

    // Pitch bend range in semitones: 1..24.  Written to the APVTS "pitchBendRange"
    // param so hosts can automate it and session recall works.
    // Message-thread only.
    void setPitchBendRange(int semitones)
    {
        const int clamped = juce::jlimit(1, 24, semitones);
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter("pitchBendRange")))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(clamped)));
    }

    // MPE on/off.  Written to the existing APVTS "mpe_enabled" param so the existing
    // processBlock path picks it up on the next block via cachedParams.mpeEnabled.
    void setMpeEnabled(bool enabled)
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter("mpe_enabled")))
            p->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
    }

    // MIDI channel filter: 0=omni (all channels), 1..16=specific channel.
    // Audio thread reads midiChannel_ and skips note-on/off events that don't match.
    void setMidiChannel(int channel) noexcept
    {
        midiChannel_.store(juce::jlimit(0, 16, channel), std::memory_order_relaxed);
    }
    int getMidiChannel() const noexcept { return midiChannel_.load(std::memory_order_relaxed); }

    // Oversampling factor index: 0=1x, 1=2x, 2=4x, 3=8x.  Default 0 (no oversampling).
    // Audio thread reads oversamplingFactor_ and applies the factor to processing.
    void setOversamplingFactor(int factorIdx) noexcept
    {
        oversamplingFactor_.store(juce::jlimit(0, 3, factorIdx), std::memory_order_relaxed);
    }
    int getOversamplingFactor() const noexcept { return oversamplingFactor_.load(std::memory_order_relaxed); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // UndoManager must be declared before apvts — C++ initialises members in
    // declaration order, and apvts is constructed with &undoManager.
    // 30 000 ms history window; max 30 coalesced transactions.
    juce::UndoManager undoManager{30000, 30};

    juce::AudioProcessorValueTreeState apvts;
    MacroSystem macroSystem_;
    // Phase 0 wildcard infrastructure (FX gap analysis, 2026-04-27).
    // Per-engine DNA bus updated at preset load + each block from M1 CHARACTER macro.
    // Consumed by FX chains via get(slot, axis). See Source/Core/DNAModulationBus.h.
    xoceanus::DNAModulationBus dnaBus_;
    // Per-engine-slot mono audio bus published after each renderBlock(),
    // consumed by Pack 1 FX chains (Otrium triangular ducking).
    // See Source/Core/PartnerAudioBus.h for lifecycle.
    xoceanus::PartnerAudioBus partnerAudioBus_;
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
    xoceanus::EpicChainSlotController epicSlots;  // 3-slot Epic Chains FX router
    ChordMachine chordMachine;

    // Unified host transport — the processor updates this from the PlayHead
    // once per audio block, then engines that tempo-sync (Outwit, Organon,
    // Orrery) read bpm/beat/isPlaying here during their renderBlock().
    xoceanus::SharedTransport hostTransport;
    MPEManager mpeManager;
    MIDILearnManager midiLearnManager;
    PresetManager presetManager;
    CouplingPresetManager couplingPresetManager;

    // Engine slots — shared_ptr for atomic swap between message and audio threads.
    // The audio thread reads via atomic_load; the message thread writes via atomic_store.
    struct EngineSlot
    {
        std::shared_ptr<SynthEngine> engine;
    };
    std::array<std::shared_ptr<SynthEngine>, MaxSlots> engines;

    // Crossfade state for engine hot-swap — audio-thread-only after the
    // pending command is consumed.  Never touched by the message thread.
    struct CrossfadeState
    {
        std::shared_ptr<SynthEngine> outgoing; // engine being faded out
        float fadeGain = 0.0f;                 // 1.0 → 0.0 during crossfade
        int fadeSamplesRemaining = 0;
        bool needsAllNotesOff = false; // inject CC123 on first crossfade block (#360)
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
    struct PendingCrossfade
    {
        std::shared_ptr<SynthEngine> outgoing;
        float fadeGain = 0.0f;
        int fadeSamplesRemaining = 0;
        bool needsAllNotesOff = true; // inject CC123 on first block (#360)
        std::atomic<bool> ready{false};

        // Non-copyable — std::atomic<bool> cannot be copy/move-assigned.
        PendingCrossfade() = default;
        PendingCrossfade(const PendingCrossfade&) = delete;
        PendingCrossfade& operator=(const PendingCrossfade&) = delete;
    };
    std::array<PendingCrossfade, MaxSlots> pendingCrossfades;

    // Pre-allocated family-bleed scratch array — capacity reserved in prepareToPlay
    // so processFamilyBleed never heap-allocates on the audio thread.
    struct FamilySlot
    {
        int slot;
        SynthEngine* eng;
    };
    juce::Array<FamilySlot> familySlots_;

    std::array<juce::AudioBuffer<float>, MaxSlots> engineBuffers;
    juce::AudioBuffer<float> crossfadeBuffer;
    std::array<juce::MidiBuffer, MaxSlots> slotMidi; // per-slot MIDI from ChordMachine

    // Wave 5 C1: per-slot pattern sequencer (primary slots 0–3 only; Ghost Slot excluded).
    // Each instance is engine-agnostic — events appear in slotMidi[] transparently.
    std::array<XOceanus::PerEnginePatternSequencer, kNumPrimarySlots> slotSequencers_;

    // External audio input capture — sized once in prepareToPlay, NEVER resized in processBlock.
    // OsmosisEngine reads raw pointers into this buffer within the same processBlock call.
    juce::AudioBuffer<float> externalInputBuffer;

    // #700: Initialize to 0.0 (not 44100) so any code that reads currentSampleRate
    // before prepareToPlay() gets an obviously-invalid value (0.0) rather than
    // silently computing with the wrong 44100.0 rate on a 48kHz or 96kHz interface.
    // All audio-thread consumers already guard sr > 0.0; message-thread consumers
    // (triggerCouplingBurst, loadEngine crossfade) also guard via the max(1,…) idiom.
    std::atomic<double> currentSampleRate{0.0};
    // atomicSampleRate_ mirrors currentSampleRate for safe cross-thread reads.
    // Written in prepareToPlay() (same time as currentSampleRate), read on the
    // message thread in triggerCouplingBurst(). currentSampleRate is now also
    // atomic, eliminating the data race (FIX 4).
    std::atomic<double> atomicSampleRate_{0.0};
    std::atomic<int> currentBlockSize{512};

    // Engine generation counter — incremented on every loadEngine() / unloadEngine()
    // call (message thread).  Read on the audio thread in processBlock() to detect
    // engine swaps without storing a raw pointer to a potentially-freed object.
    // Replaces the raw firstBreathEnginePtr_ comparison (#756).
    std::atomic<uint64_t> engineGeneration_{0};

    // SRO: Per-slot CPU profiling + fleet-wide auditor
    std::array<EngineProfiler, MaxSlots> engineProfilers;
    SROAuditor sroAuditor;

    // Cached raw parameter pointers — resolved once in prepareToPlay, read per-block.
    // Eliminates string-based hash map lookups from the audio thread.
    struct CachedParams
    {
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
        // Per-slot chord/seq routing (Wave 5 B3)
        std::array<std::atomic<float>*, 4> cmSlotRoute = {nullptr, nullptr, nullptr, nullptr};

        // B2: input mode + global key/scale
        std::atomic<float>* cmInputMode   = nullptr; // chord_input_mode (0=AUTO, 1=PAD, 2=DEG)
        std::atomic<float>* cmGlobalRoot  = nullptr; // cm_global_root (0-11)
        std::atomic<float>* cmGlobalScale = nullptr; // cm_global_scale (0-8)

        // B2: pad chord slots (16 × 3 params)
        struct PadChordParams
        {
            std::atomic<float>* root    = nullptr; // chord_pad_N_root   [0,127]
            std::atomic<float>* voicing = nullptr; // chord_pad_N_voicing [0,NumModes-1]
            std::atomic<float>* inv     = nullptr; // chord_pad_N_inv    [0,3]
        };
        std::array<PadChordParams, 16> padChords;

        // Family bleed params — cached to avoid string lookups on audio thread
        std::atomic<float>* ohmCommune = nullptr;
        std::atomic<float>* obblBond = nullptr;
        std::atomic<float>* oleDrama = nullptr;

        // wire(1C-4): Global session params — masterTune (415-466 Hz) and
        // pitchBendRange (1-24 semitones).  Cached here (not in processBlock via
        // getRawParameterValue) to satisfy Architect Condition 1 (B009 thread safety).
        std::atomic<float>* masterTune      = nullptr; // "masterTune"      (415..466 Hz)
        std::atomic<float>* pitchBendRange  = nullptr; // "pitchBendRange"  (1..24 semitones)

        // P1-6 (1F): macro1 (CHARACTER) — previously fetched via getRawParameterValue
        // inside processBlock (per-block hash lookup).  Cached here per C1 invariant.
        std::atomic<float>* macro1          = nullptr; // "macro1"          (0..1)

        // MPE parameters
        std::atomic<float>* mpeEnabled = nullptr;
        std::atomic<float>* mpeZone = nullptr;
        std::atomic<float>* mpePitchBendRange = nullptr;
        std::atomic<float>* mpePressureTarget = nullptr;
        std::atomic<float>* mpeSlideTarget = nullptr;

        // Coupling performance overlay — 4 route slots × 5 params = 20 params
        // Cached for audio-thread access (no string lookups in processBlock)
        struct CouplingRouteParams
        {
            std::atomic<float>* active = nullptr; // cp_rN_active
            std::atomic<float>* type = nullptr;   // cp_rN_type
            std::atomic<float>* amount = nullptr; // cp_rN_amount
            std::atomic<float>* source = nullptr; // cp_rN_source
            std::atomic<float>* target = nullptr; // cp_rN_target
        };
        std::array<CouplingRouteParams, CouplingCrossfader::MaxRouteSlots> cpRoutes;

    } cachedParams;

    juce::MidiBuffer mpeMidiBuffer; // MPE-processed MIDI (expression stripped)

    // Field Map SPSC queue storage (audio-thread write / UI-thread read)
    std::array<NoteMapEvent, kNoteQueueSize> noteQueue{};
    std::atomic<size_t> noteQueueHead{0};
    std::atomic<size_t> noteQueueTail{0};

    // ── CC Output SPSC queue (UI-thread write / audio-thread read) ────────────
    // Carries CC events from UI widgets (message thread) to MIDI output (audio thread).
    // Head written by UI thread; tail read/advanced by audio thread.
    struct CCOutputEvent
    {
        uint8_t channel = 0;    // 0-15 (MIDI channel minus 1)
        uint8_t controller = 0; // CC number
        uint8_t value = 0;      // 0-127
    };
    static constexpr size_t kCCQueueSize = 256;
    std::array<CCOutputEvent, kCCQueueSize> ccOutputQueue_{};
    std::atomic<size_t> ccOutputHead_{0}; // written by UI thread
    std::atomic<size_t> ccOutputTail_{0}; // read by audio thread

    // PlaySurface MIDI collector — message thread enqueues, processBlock drains.
    juce::MidiMessageCollector playSurfaceMidiCollector;

    // Set to true once setStateInformation() successfully restores saved state.
    // Used by prepareToPlay() to load a default engine on first launch (no saved state).
    // Must be atomic: setStateInformation runs on the message thread, prepareToPlay
    // can be called from any thread depending on host. (Audit P0-2 CRITICAL-1)
    std::atomic<bool> hasRestoredState{false};

    // ── Sound on First Launch (§1.1.1 Principle 4) ───────────────────────────
    // On TRUE first launch (no saved state AND never launched before), XOceanus
    // auto-plays a gentle OXBOW pad so the plugin makes music before the performer
    // touches a single control.  The note stops automatically after 30 seconds
    // or as soon as any MIDI input or engine change arrives.
    //
    // Thread model:
    //   hasLaunchedBefore_         — written by message thread (setStateInformation /
    //                                prepareToPlay), read by audio thread.  Atomic.
    //   firstBreathPending_        — set by prepareToPlay (any thread), consumed once
    //                                by the first processBlock call.  Atomic.
    //   firstBreathActive_         — audio-thread-only after consumption.  No atomic.
    //   firstBreathCountdown_      — audio-thread-only 30-second failsafe in samples.  No atomic.
    //   firstBreathFading_         — true once user interaction triggers the 200 ms fade.
    //   firstBreathFadeCountdown_  — samples remaining in the fade window before note-off fires.
    //   kFirstBreathNote           — MIDI C3 (48). Gentle, low, non-intrusive.
    //   kFirstBreathVelocity       — soft (60/127 ≈ 0.47).
    //   kFirstBreathTimeoutMs      — 30 000 ms failsafe auto-stop.
    //   kFirstBreathFadeMs         — 200 ms fade on user interaction (spec §1300).
    std::atomic<bool> hasLaunchedBefore_{false};
    std::atomic<bool> firstBreathPending_{false};
    // wire(#orphan-sweep item 2): atomic mirror of firstBreathActive_ for isFirstBreathActive().
    // Written by processBlock (audio thread), read by message thread.
    std::atomic<bool> firstBreathActiveMirror_{false};
    // Audio-thread-only state (no atomics needed):
    bool         firstBreathActive_{false};
    int          firstBreathCountdown_{0};
    bool         firstBreathFading_{false};     // true during the 200 ms interaction fade
    int          firstBreathFadeCountdown_{0};  // samples remaining in fade window
    uint64_t     firstBreathGeneration_{0}; // engineGeneration_ value at arm time; if it changes, breath is cancelled
    static constexpr int  kFirstBreathNote       = 48;     // C3
    static constexpr float kFirstBreathVelocity  = 60.0f / 127.0f;
    static constexpr int  kFirstBreathTimeoutMs  = 30000;  // 30-second failsafe
    static constexpr int  kFirstBreathFadeMs     = 200;    // §1300: fade window before note-off on user interaction

    // ── Per-slot mute state ───────────────────────────────────────────────────
    // Written by message thread (setSlotMuted), read by audio thread per block.
    std::array<std::atomic<bool>, MaxSlots> slotMuted{}; // default false

    // ── wire(1C-4): pitchBendRange change-detection (audio thread only) ─────────
    // Tracks the last RPN 0 value injected into slotMidi so we only re-emit the
    // 6-message sequence when the user actually changes the setting.
    // -1 sentinel forces injection on the first processBlock() call.
    int lastInjectedPitchBendRange_{-1};

    // ── CC11 Expression pedal — per-channel tracking (audio thread only) ────────
    // expressionValue_[ch]: 0.0–1.0, updated from CC11 events in processBlock().
    // CC11 passes through to all engine slots for per-engine handling.
    // Processor-level value is available for future coupling/macro use.
    std::array<float, 16> expressionValue_{}; // default 0.0 (no expression)

    // ── CC1 Mod Wheel — block-latched scalar (audio thread only) ─────────────
    // modWheelValue_: 0.0–1.0, latched once per block from the first CC1 event
    // found in the raw MIDI buffer.  Late-arriving CC1 events within a block are
    // intentionally ignored to avoid sub-block jitter in the mod matrix.
    // Persistent across blocks — retains the most recent CC1 value until updated.
    // Audio-thread-only; no atomics needed.
    float modWheelValue_{0.0f};

    // ── Channel Pressure (Aftertouch) — block-latched scalar (audio thread only) ──
    // aftertouchValue_: 0.0–1.0, latched from channel-pressure (0xD0) events.
    // Latch policy mirrors modWheelValue_: most-recent event per block wins
    // (no first-only guard — channel pressure events are rare relative to CC1,
    // so sub-block jitter is not a practical concern; simpler to just overwrite).
    // Persistent across blocks — retains last value until next pressure event.
    // Audio-thread-only; no atomics needed.
    float aftertouchValue_{0.0f};

    // ── CC64 sustain pedal — fleet-wide hold (audio thread only) ─────────────
    // sustainHeld_[ch]: true while CC64 >= 64 on MIDI channel ch (0-based).
    // sustainPendingNoteOffs_[slot][ch]: bitmask of notes (0–127) whose note-off
    // was suppressed while sustain was held; released when the pedal lifts.
    // All fields are audio-thread-only — no atomics needed.
    std::array<bool, 16> sustainHeld_{};
    // 128 notes × 16 channels × MaxSlots — stored as uint64_t[2] bitmasks per channel.
    // Bit N is set when note N is pending release on that slot+channel.
    struct SustainNoteSet
    {
        uint64_t lo = 0; // notes 0–63
        uint64_t hi = 0; // notes 64–127
        void set(int note)
        {
            if (note < 64)
                lo |= (1ULL << note);
            else
                hi |= (1ULL << (note - 64));
        }
        void clear(int note)
        {
            if (note < 64)
                lo &= ~(1ULL << note);
            else
                hi &= ~(1ULL << (note - 64));
        }
        bool test(int note) const { return note < 64 ? (lo >> note) & 1 : (hi >> (note - 64)) & 1; }
        bool any() const { return lo || hi; }
        void clearAll()
        {
            lo = 0;
            hi = 0;
        }
    };
    // [slot][channel]
    std::array<std::array<SustainNoteSet, 16>, MaxSlots> sustainPendingNoteOffs_{};

    // ── Chord machine fire pending flag ──────────────────────────────────────
    // message thread sets → audio thread consumes and clears.
    std::atomic<bool> chordFirePending{false};

    // ── Chord fire deferred note-off ──────────────────────────────────────────
    // When chordFirePending is consumed, note-ons are injected immediately and
    // chordFireNoteOffCountdown is set to ~kChordHoldMs worth of samples.
    // Each subsequent block decrements the countdown; when it reaches zero the
    // audio thread injects note-offs for the stored chordFireNotes.
    // This replaces the old same-block note-off which gave only ~10ms of hold.
    std::atomic<int> chordFireNoteOffCountdown{0};
    static constexpr int kChordHoldMs = 100; // hold chord for ~100ms before note-off
    std::array<std::atomic<int>, kChordSlots>
        chordFireNotes{}; // notes fired; 0 = empty (MIDI note 0 / C-1 excluded as sentinel)

    // ── Coupling burst state ──────────────────────────────────────────────────
    // couplingBurstGain: multiplier applied to all active route amounts (1.0 = no boost).
    // Starts at kCouplingBurstPeak when triggered, decays to 1.0 over kCouplingBurstMs.
    static constexpr float kCouplingBurstPeak = 2.0f; // max boost factor
    static constexpr float kCouplingBurstMs = 500.0f; // decay duration in ms
    std::atomic<float> couplingBurstGain{1.0f};
    std::atomic<int> couplingBurstSamplesRemaining{0};

    // ── Kill delay tails pending flag ────────────────────────────────────────
    // message thread sets → audio thread consumes (calls masterFX.reset()) and clears.
    std::atomic<bool> killDelayTailsPending{false};

    // ── CPU processing load ───────────────────────────────────────────────────
    // Updated each block: elapsed / buffer_duration. Smoothed with a leaky integrator.
    std::atomic<float> processingLoad{0.0f};

    // ── Dark Cockpit B041: note activity ─────────────────────────────────────
    // RMS-derived signal (0.0 = silent, 1.0 = maximum) smoothed with one-pole
    // filter (~100ms attack, ~500ms release). Read by getCockpitOpacity() in editor.
    std::atomic<float> noteActivity_{0.0f};

    // ── #1357: XY Surface position atomics (W8B mount) ───────────────────────
    // Per-slot XY position [0, 1] written by SurfaceRightPanel::onXYChanged (message thread)
    // and read by the mod routing system as ModSourceId::XYX0..XYY3 (audio thread).
    // Initialised to 0.5 (centre) so modulation is neutral before first interaction.
    // Note: std::atomic is not copy/move-constructible, so initialise via default ctor
    // and store 0.5f in the XOceanusProcessor constructor body (see XOceanusProcessor.cpp).
    std::array<std::atomic<float>, kNumPrimarySlots> xyX_;
    std::array<std::atomic<float>, kNumPrimarySlots> xyY_;
    // Wave5-C5: message-thread-origin ModSource live values (XouijaCell, etc.)
    SlotModSourceRegistry modSourceRegistry_;
    // Timestamp of the start of the current processBlock call (high-res ticks).
    juce::int64 processBlockStartTick{0};

    void cacheParameterPointers();
    void processFamilyBleed(std::array<SynthEngine*, MaxSlots>& enginePtrs);
    void processBrothCoupling(std::array<SynthEngine*, MaxSlots>& enginePtrs);

    // ── Engine graveyard — deferred destruction off the audio thread ──────────
    // When a crossfade completes, the outgoing engine's shared_ptr is deposited
    // here by the audio thread instead of being reset() in-place (which would
    // trigger ~SynthEngine() with 20+ vector deallocations on the RT thread).
    // The message thread drains the graveyard via drainGraveyard(), called
    // asynchronously via juce::MessageManager::callAsync.
    //
    // Thread-safety contract:
    //   depositInGraveyard() — called ONLY from the audio thread (processBlock)
    //   drainGraveyard()     — called ONLY from the message thread (callAsync)
    //
    // Overflow policy: if the graveyard is full (16 simultaneous engine swaps
    // faster than the message thread can drain), the engine is intentionally
    // leaked via shared_ptr::reset() without destruction.  This is a safety
    // valve that should never trigger in normal use.
    static constexpr int kGraveyardSize = 16;
    std::array<std::shared_ptr<SynthEngine>, kGraveyardSize> graveyard_;
    std::atomic<int> graveyardWritePos_{0}; // written by audio thread only
    std::atomic<int> graveyardReadPos_{0};  // written by message thread only

    void depositInGraveyard(std::shared_ptr<SynthEngine> engine) noexcept;
    void drainGraveyard();

    // BROTH quad coordinator — stateless helper, no heap allocation
    BrothCoordinator brothCoordinator_;

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
            midiOut.addEvent(juce::MidiMessage::controllerEvent(evt.channel + 1, evt.controller, evt.value),
                             numSamples - 1);
            tail = (tail + 1) % kCCQueueSize;
        }
        ccOutputTail_.store(tail, std::memory_order_release);
    }

    // ── Editor UI state storage (closes #314, #357) ──────────────────────────
    // Written on the message thread only.  No audio-thread access — plain ints.
    int persistedPlayScaleIndex = 0;     // #314: PlayControlPanel scale selector
    int persistedSelectedSlot = -1;      // #357: editor tile focus (-1 = overview)
    int persistedSignalFlowSection = 0;  // #357: signal flow active section
    bool persistedCockpitBypass = false; // #357: Dark Cockpit bypass state
    bool persistedRegisterLocked = false; // D4: register lock toggle
    int  persistedRegisterCurrent = 0;   // D4: current register index (0=Gallery, 1=Performance, 2=Coupling)

    // P1-8 (1F): Persist DashboardTabBar active tab + NOTE/KIT sub-mode so DAW
    // session reload restores the user's last PAD+KIT (or XY) layout.
    // 0=KEYS, 1=PAD, 2=XY — matches DashboardTabBar's kTabNames order.
    int  persistedDashboardTab_    = 0;   // default: KEYS
    bool persistedKitSubMode_      = false; // default: NOTE (not KIT)
    int  persistedOceanViewState_ = 0;   // F2-006: OceanView ViewState (0=Orbital,1=ZoomIn,2=Split,3=Browser)
    int  persistedOceanViewSlot_  = -1;  // F2-006: slot index when ViewState is ZoomIn/Split
    float persistedReactLevel_    = 0.80f; // F3-006: REACT dial visual reactivity level
    bool  persistedSeqBreakoutOpen_   = false; // F3-011: sequencer breakout panel open state
    bool  persistedChordBreakoutOpen_ = false; // F3-017: chord breakout panel open state

    // ── #1178: TideWaterline deferred step-sequence state ────────────────────
    // Holds the "TideWaterlineSteps" tree from setStateInformation() when the
    // editor was not yet open at restore time.  OceanView picks it up in
    // initWaterline() via getPersistedTideWaterlineState().
    // Message-thread only — no atomic needed.
    juce::ValueTree persistedTideWaterlineState_;

    // ── #1378: Per-slot preset data model ────────────────────────────────────
    // Tracks which preset is loaded in each primary slot (0–3).
    // Initialised to default-constructed PresetData (empty name, no engines).
    // Written/read on the message thread only — no atomic needed.
    std::array<PresetData, kNumPrimarySlots> slotPresets_;
    std::vector<SlotPresetListener*> slotPresetListeners_;

    // ── #1359: Settings-drawer session state ─────────────────────────────────
    // Written on the message thread (set* setters above); read on the audio thread.
    // Atomics with relaxed ordering — a one-block-late value is acceptable for
    // session controls that are set interactively, not in tight automation loops.
    std::atomic<int> polyphonyCap_{16};      // 1..32 global voice cap
    std::atomic<int> voiceMode_{0};          // 0=Poly,1=Mono,2=Legato,3=Unison
    std::atomic<int> midiChannel_{0};        // 0=omni, 1..16=specific channel
    std::atomic<int> oversamplingFactor_{0}; // 0=1x,1=2x,2=4x,3=8x

    // ── External MIDI Clock state — audio thread only (closes #359) ──────────
    // Used to derive BPM from incoming 0xF8 pulses.
    // lastClockSampleTime: the processBlock-local sample position of the most
    //   recent 0xF8 pulse, accumulated across blocks via blockSampleOffset.
    // midiClockBlockOffset: running sample count from the start of the current
    //   DAW-session, advanced by numSamples each block.
    // midiClockIntervalSamples: smoothed inter-16th-note interval (6 pulses).
    double midiClockBlockOffset_ = 0.0;   // total samples elapsed (audio thread only)
    double midiClockLastStepTime_ = -1.0; // sample time of last step boundary, or -1
    float midiClockDerivedBPM_ = 122.0f;  // current BPM derived from external clock

    // ── Wave 5 A1: Global mod routing ────────────────────────────────────────
    // ModRoutingModel — message-thread source of truth for all global routes.
    // Owned here so both the editor overlay and the processor share one instance.
    ModRoutingModel modRoutingModel_;

    // RT-safe snapshot of mod routes for the audio thread.
    // flushModRoutesSnapshot() (message thread) writes; processBlock reads.
    // Protocol: snapshotVersion_ acts as a generation counter.
    //   message thread: fill routesSnapshot_[], increment snapshotVersion_ (release)
    //   audio   thread: load snapshotVersion_ (acquire) to decide whether to re-read
    //
    // Max 32 routes; fixed-size array avoids any audio-thread allocation.
    struct GlobalModRouteSnapshot
    {
        int     sourceId{0};
        float   depth{0.0f};
        bool    bipolar{false};
        bool    valid{false};
        char    destParamId[64]{};  // fixed-length to avoid std::string on audio thread
        // Wave 5 C5: per-route slot index for sequencer-scoped sources.
        // -1 = not slot-scoped (backward-compat default).  0–3 = slot to query.
        int     slotIndex{-1};

        // B1: Pre-resolved destination parameter pointer (resolved on message thread in
        // flushModRoutesSnapshot).  Lifetime: as long as the APVTS (i.e., the processor).
        // nullptr when destParamId is not registered in the current APVTS layout (e.g. the
        // engine that owns that param is not loaded).
        // Audio thread reads this pointer; never dereferences it for the range — range
        // is pre-cached in destParamRangeSpan below.
        juce::RangedAudioParameter* destParam{nullptr};

        // Pre-computed range span (end - start) for the destination parameter so the
        // audio thread can scale the normalised modOffset to param units without calling
        // any juce:: method.  Set to 0.0f when destParam is nullptr (route is generic
        // and consumed by routeModAccum_ only).
        float destParamRangeSpan{0.0f};

        // True iff this route targets "orry_fltCutoff" — avoids strncmp on audio thread.
        // This flag is set in flushModRoutesSnapshot by pointer identity (compare resolved
        // destParam against the cached orryCutoffParam_ pointer).
        bool isOrryCutoff{false};

        // T5: True iff this route's source is ModSourceId::Velocity (id 3).
        // When set, routeModAccum_[ri] holds the raw depth (not depth*srcVal).
        // The consuming engine multiplies by voice.velocity at render time to satisfy
        // D001 (per-voice latch).  This tag makes the contract split typesafe — engines
        // can assert or branch on velocityScaled rather than re-inspecting sourceId.
        bool velocityScaled{false};

        // T6: Onset engine global-parameter destination flags.
        // Set in flushModRoutesSnapshot() by pointer identity — avoids strncmp on audio thread.
        // When set, the route eval loop accumulates modOffset into the corresponding
        // Onset atomic float (globalPercLevelModOffset_, etc.) for OnsetEngine to read.
        bool isPercLevel{false};
        bool isPercPunch{false};
        bool isPercTone{false};
        bool isPercGrit{false};
    };
    static constexpr int kMaxGlobalRoutes = ModRoutingModel::MaxRoutes;
    std::array<GlobalModRouteSnapshot, kMaxGlobalRoutes> routesSnapshot_{};
    std::atomic<int> routesSnapshotCount_{0};   // written by message thread, read by audio thread
    std::atomic<int> snapshotVersion_{0};        // generation counter
    int audioSnapshotVersion_{-1};               // audio thread: last consumed version (audio-thread-only)

    // Cached pointer to the orry_fltCutoff parameter — resolved once in cacheParameterPointers()
    // and used in flushModRoutesSnapshot() to set isOrryCutoff by pointer identity (no strncmp).
    juce::RangedAudioParameter* orryCutoffParam_{nullptr};

    // T6: Cached pointers to Onset global parameters — resolved once in cacheParameterPointers()
    // and used in flushModRoutesSnapshot() to set isPercXxx flags by pointer identity.
    // nullptr when Onset is not registered (safe — corresponding atomic stays 0.0f).
    juce::RangedAudioParameter* percLevelParam_{nullptr};
    juce::RangedAudioParameter* percPunchParam_{nullptr};
    juce::RangedAudioParameter* percToneParam_{nullptr};
    juce::RangedAudioParameter* percGritParam_{nullptr};

    // B1: Per-route modulation accumulator — audio thread writes modOffset here each block.
    // Index matches routesSnapshot_ slot.  Engines can call getModRouteAccum(routeIdx) to
    // read the current offset for their parameter.  Cleared (zeroed) every block before
    // the route eval loop so stale values don't persist when routes are removed.
    //
    // Usage pattern for engines:
    //   1. On load / parameter-cache pass: scan routesSnapshot_ for routes targeting my params.
    //      Store matching route indices.
    //   2. In renderBlock: read getModRouteAccum(routeIdx) and add to the parameter offset.
    //
    // NOTE: routeModAccum_ is written by the audio thread only.  It is exposed read-only to
    // engines via getModRouteAccum() (audio-thread-only accessor — no atomics needed here
    // because all readers and writers are on the same audio thread during processBlock).
    // Plain floats are sufficient; no cross-thread access occurs after the write.
    std::array<float, kMaxGlobalRoutes> routeModAccum_{};

    // LFO1 value written by OrreryEngine on the audio thread.
    // Global router reads this as the LFO1 source value.
    std::atomic<float> globalLFO1_{0.0f};

    // Global cutoff mod offset — computed in processBlock by evaluating global
    // mod routes, read by OrreryEngine::renderBlock via getGlobalCutoffModOffset().
    // Units: same as modCutoffOffset (pre-multiplied by 8000 Hz).
    std::atomic<float> globalCutoffModOffset_{0.0f};

    // T6: Onset global-parameter mod offsets — computed each processBlock from routes
    // that target the corresponding Onset parameters.  OnsetEngine reads these via
    // pointers set by setPercModOffsetPtrs() at engine-load time.
    // For velocityScaled routes, the accumulator holds depth (not depth*vel); OnsetEngine
    // multiplies by lastTriggerVelocity at consumption (Strategy 2, D001 compliance).
    // Units: normalised [−1, +1] offset applied to the [0, 1] parameter range.
    std::atomic<float> globalPercLevelModOffset_{0.0f};
    std::atomic<float> globalPercPunchModOffset_{0.0f};
    std::atomic<float> globalPercToneModOffset_{0.0f};
    std::atomic<float> globalPercGritModOffset_{0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOceanusProcessor)
};

} // namespace xoceanus
