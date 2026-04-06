// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "SynthEngine.h"
#include "IAudioBufferSink.h"
#include "../Engines/Opal/OpalEngine.h"
#include "../DSP/FastMath.h"
#include <array>
#include <vector>
#include <atomic>
#include <memory>
#include <string>

namespace xoceanus
{

//==============================================================================
// MegaCouplingMatrix — Cross-engine modulation routing.
//
// Implements the "Normalled Matriarch" pattern:
//   - Pre-wired default routes for common engine pairs
//   - User overrides replace specific normalled routes
//   - Removing an override re-engages the default
//   - Supports per-sample (tight) and block-level (efficient) processing
//
// Thread safety:
//   - Route mutations (addRoute/removeUserRoute/clearRoutes) happen on the
//     message thread and swap into a double-buffered route list.
//   - processBlock() reads the active route list on the audio thread.
//   - No locks on the audio thread.
//
class MegaCouplingMatrix
{
public:
    static constexpr int MaxSlots = 5; // 4 primary + 1 Ghost Slot
    static constexpr int MaxRoutes = 64;

    struct CouplingRoute
    {
        int sourceSlot; // 0-4 (slot 4 = Ghost Slot)
        int destSlot;   // 0-4 (slot 4 = Ghost Slot)
        CouplingType type;
        float amount;     // 0.0 to 1.0 — target (set by UI / automation)
        bool isNormalled; // true = default, false = user-defined
        bool active = true;

        // One-pole smoothed amount — updated per block on the audio thread to
        // prevent zipper noise when `amount` is automated or changed by a macro.
        // `mutable` so it can be updated inside the const range-for in processBlock.
        // Initialised to `amount` in addRoute() so the first block plays at the
        // correct level without a ramp from zero. Preserved across double-buffer
        // flips because newRoutes is copy-constructed from the current list. (#684)
        mutable float smoothedAmount = 0.0f;

        // AudioToBuffer sink cache — resolved on the message thread by
        // resolveAudioToBufferSinks() to avoid dynamic_cast on the audio thread.
        // Non-null only for AudioToBuffer routes whose dest implements IAudioBufferSink.
        // Invalidated (set to nullptr) by notifyCouplingMatrixOfSwap() when the
        // destination slot is replaced with a non-IAudioBufferSink engine.
        // processAudioRoute() uses static_cast from this pointer instead of
        // dynamic_cast from dest — safe because resolveAudioToBufferSinks() already
        // verified the cast is valid on the message thread.
        IAudioBufferSink* sinkCache = nullptr;

        // Generation counter — stamped by resolveAudioToBufferSinks() on the
        // message thread with the value of routeGeneration_ at the time the
        // sinkCache was resolved. The audio thread skips any AudioToBuffer route
        // whose validGeneration differs from the current routeGeneration_, which
        // guarantees block-boundary atomicity: a sinkCache resolved against the
        // old engine configuration is never used after an engine hot-swap. (#755)
        uint64_t validGeneration = 0;
    };

    // Call from prepareToPlay to pre-allocate the coupling scratch buffers.
    // couplingBuffer: L-channel (or mono) scratch for all coupling types.
    // couplingBufferR: R-channel scratch, used exclusively for AudioToBuffer
    //                  routes which require stereo — see processAudioRoute() and
    //                  Docs/xopal_phase1_architecture.md §15.4.
    //
    // sampleRate is used to scale the control-rate decimation ratio so that the
    // effective modulation update rate stays near ~1.5 kHz regardless of sample
    // rate (e.g. 32 @ 48 kHz ≈ 1.5 kHz; 64 @ 96 kHz ≈ 1.5 kHz).
    //
    // #702: No default for sampleRate — callers must pass the actual rate from
    // prepareToPlay. Passing a hardcoded default (e.g. 44100) would silently
    // compute an incorrect controlRateRatio on 48 kHz / 96 kHz interfaces.
    void prepare(int maxBlockSize, double sampleRate)
    {
        couplingBuffer.resize(static_cast<size_t>(maxBlockSize), 0.0f);
        couplingBufferR.resize(static_cast<size_t>(maxBlockSize), 0.0f);

        // Target ~1.5 kHz control rate, rounded to nearest power of 2.
        // Clamped to [8, 256] to avoid extremes at unusual sample rates.
        const int rawRatio = static_cast<int>(sampleRate / 1500.0);
        // Round down to previous power of 2
        int ratio = 8;
        while (ratio * 2 <= rawRatio && ratio < 256)
            ratio *= 2;
        controlRateRatio = std::max(8, std::min(256, ratio));

        // Block-size-aware smoothing coefficient (#910).
        // Target: ~200ms time constant regardless of block size.
        // At 512 samples / 48kHz the old constant (0.99) gave ~200ms;
        // at 32 samples it collapsed to ~6ms — radically different automation feel.
        // tc = 0.2s * (sampleRate / blockSize) blocks; coeff = exp(-1 / tc)
        {
            const double blocksPerSecond = sampleRate / static_cast<double>(maxBlockSize);
            const double targetTimeConstantSeconds = 0.2; // 200ms
            const double targetBlocks = targetTimeConstantSeconds * blocksPerSecond;
            couplingSmooth_ = static_cast<float>(std::exp(-1.0 / targetBlocks));
        }

        // Reset all in-flight smoothedAmounts to their target to prevent a zipper
        // pop when the DAW changes buffer size mid-session (#910).
        auto routes = std::atomic_load(&routeList);
        if (routes)
        {
            for (auto& route : *routes)
                route.smoothedAmount = route.amount;
        }
    }

    // Set the engine pointers for the active slots (audio thread).
    // Each pointer is stored atomically so getActiveEngines() may be called
    // from the message thread without a data race (H04 fix).
    void setEngines(std::array<SynthEngine*, MaxSlots> engines)
    {
        for (int i = 0; i < MaxSlots; ++i)
            activeEngines[static_cast<size_t>(i)].store(engines[static_cast<size_t>(i)], std::memory_order_release);
    }

    // Get the engine pointers for the active slots (message thread — read-only).
    // Returns a plain array snapshot using acquire loads — safe to call
    // concurrently with setEngines() on the audio thread.
    std::array<SynthEngine*, MaxSlots> getActiveEngines() const
    {
        std::array<SynthEngine*, MaxSlots> result{};
        for (int i = 0; i < MaxSlots; ++i)
            result[static_cast<size_t>(i)] = activeEngines[static_cast<size_t>(i)].load(std::memory_order_acquire);
        return result;
    }

    //-- Route mutation (message thread only) ----------------------------------

    void clearRoutes()
    {
        auto newRoutes = std::make_shared<std::vector<CouplingRoute>>();
        std::atomic_store(&routeList, newRoutes);
    }

    // Returns true if adding a route from sourceSlot → destSlot with the given
    // type would create a directed cycle in the graph of audio-rate coupling routes.
    //
    // Only audio-rate types can cause within-block feedback loops:
    //   AudioToFM, AudioToRing, AudioToBuffer, AudioToWavetable.
    //   TriangularCoupling is block-rate (applyTriangularCouplingInput path) and
    //   therefore cannot create within-block feedback loops; it is excluded.
    //
    // Control-rate types (AmpToFilter, AmpToPitch, LFOToPitch, EnvToMorph,
    // EnvToDecay, PitchToPitch, FilterToFilter, AmpToChoke, RhythmToBlend)
    // are block-latent and cannot create within-block feedback, so they are
    // always safe to add without cycle checking.
    //
    // KnotTopology is INTENTIONALLY bidirectional and energy-preserving —
    // it is explicitly excluded from cycle detection.
    //
    // Algorithm: DFS from destSlot. If we can reach sourceSlot via existing
    // audio-rate routes (excluding KnotTopology), adding sourceSlot → destSlot
    // would close a cycle. O(V+E) where V = MaxSlots (5), E ≤ MaxRoutes (64).
    //
    // Call on the message thread before addRoute() for any audio-rate route.
    bool wouldCreateCycle(int sourceSlot, int destSlot, CouplingType type) const
    {
        // Only audio-rate types can create feedback loops.
        if (!isAudioRateType(type))
            return false;

        // KnotTopology is intentionally bidirectional — never block it.
        if (type == CouplingType::KnotTopology)
            return false;

        // Self-routes are already guarded in addRoute; treat as a cycle
        // here too so the caller can use one unified check.
        if (sourceSlot == destSlot)
            return true;

        // Single snapshot for the entire DFS traversal — prevents inconsistent
        // graph reads if another thread modifies routeList during recursion.
        auto snapshot = std::atomic_load(&routeList);
        if (!snapshot)
            return false;
        std::array<bool, MaxSlots> visited{};
        return dfsReachable(destSlot, sourceSlot, visited, *snapshot);
    }

    void addRoute(CouplingRoute route)
    {
        // Self-route guard (existing check).
        if (route.sourceSlot == route.destSlot)
            return;

        // Graph-level cycle detection for audio-rate coupling types (Phase 3).
        // Prevents A→B→A feedback loops that would clip and rail within one block.
        // Control-rate types and KnotTopology are intentionally excluded — see
        // wouldCreateCycle() for the full rationale.
        if (wouldCreateCycle(route.sourceSlot, route.destSlot, route.type))
            return;

        // Seed smoothedAmount to the target amount so the first block starts at
        // the correct level rather than ramping up from zero. (#684)
        route.smoothedAmount = route.amount;

        auto current = std::atomic_load(&routeList);
        auto newRoutes = std::make_shared<std::vector<CouplingRoute>>(*current);
        newRoutes->push_back(route);
        std::atomic_store(&routeList, newRoutes);

        // Resolve sinkCache for any newly added AudioToBuffer route.
        // resolveAudioToBufferSinks() is cheap (only touches AudioToBuffer routes)
        // and must run on the message thread — safe here since addRoute is message-thread-only.
        if (route.type == CouplingType::AudioToBuffer)
            resolveAudioToBufferSinks();
    }

    void removeUserRoute(int sourceSlot, int destSlot, CouplingType type)
    {
        auto current = std::atomic_load(&routeList);
        auto newRoutes = std::make_shared<std::vector<CouplingRoute>>(*current);

        newRoutes->erase(
            std::remove_if(
                newRoutes->begin(), newRoutes->end(), [&](const CouplingRoute& r)
                { return r.sourceSlot == sourceSlot && r.destSlot == destSlot && r.type == type && !r.isNormalled; }),
            newRoutes->end());

        // Re-enable normalled route if it exists
        for (auto& r : *newRoutes)
            if (r.sourceSlot == sourceSlot && r.destSlot == destSlot && r.type == type && r.isNormalled)
                r.active = true;

        std::atomic_store(&routeList, newRoutes);
    }

    //-- Audio processing (audio thread only) ----------------------------------

    // Load the current route list once per audio callback.
    // Store the result in a local shared_ptr and pass it to processBlock()
    // to avoid repeated atomic reference count operations inside the block.
    std::shared_ptr<std::vector<CouplingRoute>> loadRoutes() const { return std::atomic_load(&routeList); }

    // Process all coupling routes for a block of audio.
    // Pass the shared_ptr obtained from loadRoutes() — avoids an atomic reload
    // (and its LOCK prefix) inside this method.
    void processBlock(int numSamples, const std::shared_ptr<std::vector<CouplingRoute>>& routes)
    {
        if (!routes || routes->empty())
            return;

        // Load the current generation once per block (acquire) so processAudioRoute()
        // can compare it against each route's validGeneration without re-reading the
        // atomic inside the hot per-route loop. (#755)
        const uint64_t currentGen = routeGeneration_.load(std::memory_order_acquire);

        for (const auto& route : *routes)
        {
            // One-pole smoother for coupling amount — prevents zipper noise on automation.
            // couplingSmooth_ is computed in prepare() for a ~200ms time constant that is
            // invariant to block size, so macro sweeps sound identical across all DAW buffer
            // sizes. `smoothedAmount` is mutable so it can be updated here despite the const
            // range-for reference. (#684, #910)
            route.smoothedAmount += (route.amount - route.smoothedAmount) * (1.0f - couplingSmooth_);

            if (!route.active || std::abs(route.smoothedAmount) < 0.001f)
                continue;

            // Bounds check slot indices to prevent OOB access
            if (route.sourceSlot < 0 || route.sourceSlot >= MaxSlots || route.destSlot < 0 ||
                route.destSlot >= MaxSlots)
                continue;

            auto* source = activeEngines[static_cast<size_t>(route.sourceSlot)].load(std::memory_order_relaxed);
            auto* dest = activeEngines[static_cast<size_t>(route.destSlot)].load(std::memory_order_relaxed);
            if (!source || !dest)
                continue;

            // Issue #421: skip coupling delivery to silence-gated destination engines.
            // If the dest engine skipped renderBlock() (silence gate active), its
            // coupling accumulation variables are never reset, causing stale modulation
            // to pop on the first audible block after the gate releases. Skipping
            // delivery here prevents accumulation while the engine is idle.
            // AudioToBuffer routes are exempt: the ring buffer is the sink, not the
            // engine's accumulation vars, so delivery is always safe.
            if (route.type != CouplingType::AudioToBuffer && dest->isSilenceGateBypassed())
                continue;

            // Use pre-allocated scratch buffer (sized in prepare()).
            // Audio coupling types carry stereo content — mix L+R to mono.
            // Modulation coupling types (LFO, env, amp) are inherently mono.
            // AudioToBuffer is an audio route but is handled separately via
            // processAudioRoute() below — it bypasses the mono mixdown.
            const int limit = juce::jmin(numSamples, static_cast<int>(couplingBuffer.size()));

            // KNOT: bidirectional irreducible coupling — handled by its own path.
            // Both engines act as source and destination simultaneously.
            // Must be dispatched before the isAudioRoute branch.
            if (route.type == CouplingType::KnotTopology)
            {
                processKnotRoute(source, dest, route, limit);
                continue;
            }

            // TriangularCoupling (#15) semantic path (fixes #420 + #434):
            // Extract the source engine's love-triangle state (I/P/C) via
            // getLoveTriangleState().  For Oxytocin sources this returns the
            // actual effective Intimacy/Passion/Commitment values; for all other
            // engines it returns {0,0,0} (a drum machine has no love state).
            //
            // The state is delivered to the destination via applyTriangularCouplingInput()
            // rather than the generic applyCouplingInput() path.  This bypasses the
            // RMS-only encoding used by the old audio-rate path (fixed #420) and
            // ensures every destination engine receives a meaningful signal via the
            // default AmpToFilter fallback in SynthEngine::applyTriangularCouplingInput()
            // rather than silently dropping the route (fixed #434).
            if (route.type == CouplingType::TriangularCoupling)
            {
                SynthEngine::LoveTriangleState state = source->getLoveTriangleState();
                dest->applyTriangularCouplingInput(state, route.smoothedAmount);
                continue;
            }

            const bool isAudioRoute =
                route.type == CouplingType::AudioToWavetable || route.type == CouplingType::AudioToFM ||
                route.type == CouplingType::AudioToRing || route.type == CouplingType::AudioToBuffer;

            // AudioToBuffer routes bypass the mono mixdown — they require true
            // stereo and write directly into the destination's AudioRingBuffer.
            // Handled by processAudioRoute(); skip the standard path.
            if (route.type == CouplingType::AudioToBuffer)
            {
                processAudioRoute(source, dest, route, limit, currentGen);
                continue;
            }

            if (isAudioRoute)
            {
                // Audio routes: per-sample stereo-to-mono mixdown (must stay audio rate)
                for (int i = 0; i < limit; ++i)
                    couplingBuffer[static_cast<size_t>(i)] =
                        (source->getSampleForCoupling(0, i) + source->getSampleForCoupling(1, i)) * 0.5f;
            }
            else
            {
                // SRO: Control-rate decimation for modulation coupling types.
                // Modulation signals (amp, LFO, envelope, pitch) change slowly —
                // sample source every controlRateRatio samples and linearly
                // interpolate between control points (Buchla 266 topology).
                // Saves ~97% of getSampleForCoupling calls for these route types.
                fillControlRateBuffer(source, limit);
            }

            dest->applyCouplingInput(route.type, route.smoothedAmount, couplingBuffer.data(), limit);
        }
    }

    std::vector<CouplingRoute> getRoutes() const
    {
        auto routes = std::atomic_load(&routeList);
        return routes ? *routes : std::vector<CouplingRoute>{};
    }

    //-- Hot-swap coupling integrity -------------------------------------------

    // Called by XOceanusProcessor::loadEngine() after an engine swap on `slot`.
    //
    // AudioToBuffer routes require the destination to implement IAudioBufferSink.
    // Any other engine as dest cannot receive the route — it is marked inactive
    // ("orphaned") so the UI can surface it as dimmed/disconnected.
    //
    // All other coupling types (AmpToFilter, LFOToPitch, etc.) survive the swap
    // because every SynthEngine must handle unknown coupling types gracefully
    // in applyCouplingInput(). They remain active — the new engine will receive
    // them and may produce different (but never crash-inducing) behaviour.
    //
    // If newEngineId is "Opal", any previously suspended AudioToBuffer routes
    // to this slot are re-activated (OPAL is being restored to the slot).
    //
    // After updating active flags, resolveAudioToBufferSinks() is called to
    // update sinkCache pointers for all AudioToBuffer routes on this slot.
    //
    // Called on the message thread. Uses the same double-buffered atomic swap
    // pattern as addRoute/removeUserRoute — safe for the audio thread.
    // newEnginePtr: the raw SynthEngine* for the new engine in this slot.
    // Must be called AFTER the processor's engines[slot] is updated, so we
    // can update activeEngines[slot] to match before resolving sink caches.
    // (W2 Audit CRITICAL-1: resolveAudioToBufferSinks was reading stale activeEngines)
    void notifyCouplingMatrixOfSwap(int slot, const std::string& /*newEngineId*/, SynthEngine* newEnginePtr = nullptr)
    {
        // Update our activeEngines cache for this slot so resolveAudioToBufferSinks
        // reads the CURRENT engine, not the stale one from the last setEngines() call.
        if (slot >= 0 && slot < MaxSlots)
            activeEngines[static_cast<size_t>(slot)].store(newEnginePtr, std::memory_order_release);

        // Re-resolve sink caches. This does the dynamic_cast on the message thread
        // and determines whether the new engine supports IAudioBufferSink.
        // No hardcoded engine names — capability is detected via the interface.
        // (W2 Audit HIGH-3: removed hardcoded "Opal" string)
        resolveAudioToBufferSinks();

        // Now activate/suspend AudioToBuffer routes based on resolved sinkCache.
        auto current = std::atomic_load(&routeList);
        auto newRoutes = std::make_shared<std::vector<CouplingRoute>>(*current);

        bool changed = false;
        for (auto& r : *newRoutes)
        {
            if (r.type != CouplingType::AudioToBuffer || r.destSlot != slot)
                continue;

            bool shouldBeActive = (r.sinkCache != nullptr);
            if (r.active != shouldBeActive)
            {
                r.active = shouldBeActive;
                changed = true;
            }
        }

        if (changed)
            std::atomic_store(&routeList, newRoutes);
    }

    // Resolve IAudioBufferSink* caches for all AudioToBuffer routes.
    //
    // Call on the message thread after any engine change (engine swap, preset
    // load, or addRoute) to pre-compute the downcast that processAudioRoute()
    // would otherwise perform on every audio callback.
    //
    // Thread safety: reads engine pointers via acquire loads (getActiveEngines()),
    // then publishes a new route list via atomic_store — same double-buffer pattern
    // used by addRoute / removeUserRoute. The audio thread will see the updated
    // sinkCache pointers atomically on the next loadRoutes() call.
    //
    // The dynamic_cast<IAudioBufferSink*> is intentionally done here (message
    // thread) rather than in processAudioRoute() (audio thread) — RTTI on the
    // audio thread can cause priority inversion on some RTOS schedulers.
    //
    void resolveAudioToBufferSinks()
    {
        auto engines = getActiveEngines(); // acquire loads — safe from message thread
        auto current = std::atomic_load(&routeList);
        auto newRoutes = std::make_shared<std::vector<CouplingRoute>>(*current);

        // Bump the generation before stamping routes so the audio thread can never
        // observe a route whose validGeneration matches the new generation but whose
        // sinkCache still points to the old engine. The release ordering ensures that
        // all route writes below are visible to the audio thread before it can load
        // the incremented generation. (#755)
        const uint64_t newGen = routeGeneration_.fetch_add(1, std::memory_order_release) + 1;

        bool changed = false;
        for (auto& r : *newRoutes)
        {
            if (r.type != CouplingType::AudioToBuffer)
                continue;

            if (r.destSlot < 0 || r.destSlot >= MaxSlots)
            {
                if (r.sinkCache != nullptr || r.validGeneration != newGen)
                {
                    r.sinkCache = nullptr;
                    r.validGeneration = newGen;
                    changed = true;
                }
                continue;
            }

            auto* destEngine = engines[static_cast<size_t>(r.destSlot)];
            auto* sink = (destEngine != nullptr) ? dynamic_cast<IAudioBufferSink*>(destEngine) : nullptr;

            if (sink != r.sinkCache || r.validGeneration != newGen)
            {
                r.sinkCache = sink;
                r.validGeneration = newGen;
                changed = true;
            }
        }

        if (changed)
            std::atomic_store(&routeList, newRoutes);
    }

private:
    // SRO: Control-rate decimation ratio for modulation coupling types.
    // Computed in prepare() to maintain ~1.5 kHz control rate at any sample rate.
    // #702: Initialised to 0 (not a hardcoded 48 kHz guess) so any processBlock()
    // call before prepare() is detected early. fillControlRateBuffer() treats
    // controlRateRatio <= 0 as "not prepared" and reads every sample (ratio = 1),
    // which is safe but CPU-heavy — a clear signal that prepare() was missed.
    int controlRateRatio = 0;

    // Block-size-aware one-pole smoothing coefficient for coupling amounts (#910).
    // Recomputed in prepare() so the ~200ms time constant is invariant to block size.
    // Default 0.99f matches the old per-block constant at 48kHz/512 samples.
    float couplingSmooth_ = 0.99f;

    //-- Cycle detection helpers (message thread only) -------------------------

    // Returns true for coupling types that propagate at audio rate and can
    // therefore create within-block feedback loops when cycles exist.
    // KnotTopology is intentionally excluded — it is bidirectional by design
    // and energy-preserving; blocking it would destroy valid patches.
    // TriangularCoupling is now block-rate (routed via applyTriangularCouplingInput,
    // not via the audio-sample buffer path), so it is excluded here.
    static bool isAudioRateType(CouplingType type) noexcept
    {
        return type == CouplingType::AudioToFM || type == CouplingType::AudioToRing ||
               type == CouplingType::AudioToBuffer || type == CouplingType::AudioToWavetable;
    }

    // DFS reachability check over the current audio-rate route graph.
    // Returns true if `target` is reachable from `from` via audio-rate routes
    // (excluding KnotTopology). Used by wouldCreateCycle() to detect cycles
    // before inserting a new route.
    //
    // `visited` is a stack-allocated boolean array (MaxSlots = 5 entries) that
    // prevents infinite loops through already-explored nodes. It is passed by
    // reference so a single allocation serves the entire recursive call tree.
    //
    // Called on the message thread only — receives a pre-snapshotted route vector
    // from wouldCreateCycle() to guarantee a consistent graph view across all
    // recursion depths.
    bool dfsReachable(int from, int target, std::array<bool, MaxSlots>& visited,
                      const std::vector<CouplingRoute>& routes) const
    {
        if (from == target)
            return true;
        if (from < 0 || from >= MaxSlots || visited[static_cast<size_t>(from)])
            return false;

        visited[static_cast<size_t>(from)] = true;

        for (const auto& r : routes)
        {
            if (r.sourceSlot == from && isAudioRateType(r.type) && r.type != CouplingType::KnotTopology)
            {
                if (dfsReachable(r.destSlot, target, visited, routes))
                    return true;
            }
        }
        return false;
    }

    std::shared_ptr<std::vector<CouplingRoute>> routeList = std::make_shared<std::vector<CouplingRoute>>();
    std::array<std::atomic<SynthEngine*>, MaxSlots> activeEngines{};

    // Generation counter for AudioToBuffer sink cache invalidation (#755).
    // Incremented (release) on the message thread every time sink caches are
    // re-resolved (resolveAudioToBufferSinks). Each CouplingRoute stores the
    // generation it was resolved for in validGeneration; processAudioRoute()
    // loads this counter (acquire) once per audio block and skips stale routes.
    std::atomic<uint64_t> routeGeneration_{ 0 };

    std::vector<float> couplingBuffer;  // L / mono scratch — pre-allocated in prepare()
    std::vector<float> couplingBufferR; // R scratch for AudioToBuffer stereo push

    //-- SRO: Control-rate buffer fill with linear interpolation ----------------
    //
    // For modulation routes (AmpToFilter, LFOToPitch, EnvToMorph, etc.),
    // the source signal is inherently control-rate. Instead of reading
    // every sample, we read every controlRateRatio-th sample and linearly
    // interpolate between control points. This mirrors the Buchla 266
    // "source of uncertainty" topology where CV line capacitance produces
    // smooth transitions between discrete control updates.
    //
    void fillControlRateBuffer(SynthEngine* source, int numSamples)
    {
        if (numSamples <= 0)
            return;

        // #702: If prepare() was never called, controlRateRatio is 0.
        // Fall back to ratio=1 (per-sample, no decimation) — safe but CPU-heavy.
        // This should never happen in production; the assertion in prepare() and
        // the sr>0 guard in loadEngine() are the first lines of defence.
        const int effectiveRatio = (controlRateRatio > 0) ? controlRateRatio : 1;

        float currentVal = source->getSampleForCoupling(0, 0);

        int blockStart = 0;
        while (blockStart < numSamples)
        {
            int blockEnd = std::min(blockStart + effectiveRatio, numSamples);
            // Read next control point (or last sample if at buffer end)
            float nextVal = source->getSampleForCoupling(0, std::min(blockEnd, numSamples - 1));

            // Linearly interpolate between control points
            int blockLen = blockEnd - blockStart;
            float invLen = (blockLen > 1) ? 1.0f / static_cast<float>(blockLen) : 1.0f;
            for (int i = 0; i < blockLen; ++i)
            {
                float t = static_cast<float>(i) * invLen;
                couplingBuffer[static_cast<size_t>(blockStart + i)] =
                    flushDenormal(currentVal + t * (nextVal - currentVal));
            }

            currentVal = nextVal;
            blockStart = blockEnd;
        }
    }

    //-- AudioToBuffer push path -----------------------------------------------
    //
    // Phase 2 implementation (Round 11F), Phase 3 cycle detection added:
    //
    //   1. Cycle detection: if source == dest (same engine slot), skip entirely.
    //      Prevents a single engine from streaming into its own grain buffer,
    //      which would create infinite DC feedback in the granulator.
    //      Graph-level cycle detection (A→B→A) is enforced upstream in addRoute()
    //      via wouldCreateCycle() / dfsReachable() — audio-rate cycles are blocked
    //      before they enter the route list.
    //
    //   2. Fills couplingBuffer (L) and couplingBufferR (R) with the source
    //      engine's per-sample output cache via getSampleForCoupling().
    //
    //   3. Uses route.sinkCache (IAudioBufferSink*) — pre-resolved on the message
    //      thread by resolveAudioToBufferSinks(). OpalEngine implements the interface
    //      in Phase 2; any future receiver engine simply inherits IAudioBufferSink.
    //
    //   4. Fetches the per-slot AudioRingBuffer via getGrainBuffer(sourceSlot).
    //      Slot matching ensures multiple simultaneous AudioToBuffer sources each
    //      write into a dedicated ring, preventing sample-accurate collisions.
    //
    //   5. Calls AudioRingBuffer::pushBlock() — real-time safe, no allocation.
    //      dest->applyCouplingInput() is NOT called; the ring buffer is the sink.
    //
    // Full design: Docs/xopal_phase1_architecture.md §15.4
    // Phase 2 summary: Docs/audio_to_buffer_phase2.md
    //
    // currentGen is the routeGeneration_ value loaded once at the top of processBlock.
    // It is passed in to avoid re-reading the atomic on every AudioToBuffer route.
    void processAudioRoute(SynthEngine* source, SynthEngine* dest, const CouplingRoute& route,
                           int numSamples, uint64_t currentGen)
    {
        // Phase 2 cycle detection: skip self-routes.
        // A source slot routing to itself (AudioToBuffer with sourceSlot == destSlot)
        // would feed the grain buffer with its own output — instant DC accumulation.
        if (route.sourceSlot == route.destSlot)
            return;

        // (#755) Generation guard — block-boundary atomicity for hot-swap safety.
        //
        // sinkCache was resolved on the message thread for a specific engine
        // configuration. After an engine hot-swap, resolveAudioToBufferSinks()
        // increments routeGeneration_ and re-stamps each route's validGeneration.
        // Until the audio thread picks up the new route list (next loadRoutes()
        // call), it may see the OLD route whose sinkCache points to the OLD engine.
        //
        // By comparing route.validGeneration against currentGen (loaded once at the
        // top of processBlock with memory_order_acquire), we guarantee that any route
        // whose sinkCache was resolved against a superseded engine configuration is
        // silently dropped for exactly one block — the window between the swap and
        // the next loadRoutes() call. No audio is written to the stale engine.
        if (route.validGeneration != currentGen)
            return;

        // Step 1: fill stereo scratch buffers from source coupling cache.
        for (int i = 0; i < numSamples; ++i)
        {
            couplingBuffer[static_cast<size_t>(i)] = source->getSampleForCoupling(0, i);
            couplingBufferR[static_cast<size_t>(i)] = source->getSampleForCoupling(1, i);
        }

        // Step 2: obtain the IAudioBufferSink from the pre-resolved cache.
        // sinkCache is populated on the message thread by resolveAudioToBufferSinks().
        //
        // SAFETY (W2 Audit CRITICAL-1/CRITICAL-2 + #755): The generation guard above
        // ensures sinkCache was resolved for the same engine configuration that is
        // currently active. The null check below is a belt-and-suspenders guard for
        // any race where sinkCache was legitimately nullptr (non-IAudioBufferSink dest).
        auto* sink = route.sinkCache;
        if (sink == nullptr || dest == nullptr)
            return;

        // Step 3: obtain the per-slot ring buffer via the IAudioBufferSink interface.
        // route.sourceSlot identifies which input slot on the sink receives this source.
        // getGrainBuffer() returns nullptr for out-of-range slots.
        AudioRingBuffer* rb = sink->getGrainBuffer(route.sourceSlot);
        if (rb == nullptr)
            return;

        // Step 4: push audio into the ring buffer.
        // pushBlock() is lock-free and allocation-free. The level parameter
        // scales each written sample by route.amount (0.0–1.0).
        // Freeze state is managed internally by AudioRingBuffer::pushBlock().
        rb->pushBlock(couplingBuffer.data(), couplingBufferR.data(), numSamples, route.smoothedAmount);

        // Do NOT call dest->applyCouplingInput() — the ring buffer is the exclusive
        // sink for AudioToBuffer routes. OpalEngine reads from it during renderBlock().
    }

    //-- KnotTopology bidirectional coupling path ------------------------------
    //
    // KNOT creates mutual, co-evolving entanglement between two engines.
    // Unlike all other coupling types (sender→receiver), KnotTopology applies
    // modulation in BOTH directions within a single processing pass:
    //
    //   Pass A: source engine output → applyCouplingInput on dest (AmpToFilter)
    //   Pass B: dest engine output   → applyCouplingInput on source (AmpToFilter)
    //
    // Both passes use AmpToFilter semantics — the established control-rate
    // coupling type that every engine is guaranteed to handle. Future engines
    // may inspect the CouplingType::KnotTopology tag in applyCouplingInput()
    // to route differently, but AmpToFilter is the safe universal fallback.
    //
    // Linking number (1-5) scales the effective modulation amount for both
    // directions. Higher linking = more deeply entangled parameter pairs, so
    // the coupling is stronger. Encoding uses the full amount range:
    //
    //   linkingNum = juce::roundToInt(amount * 4.0f) + 1   →  [1, 5]
    //   scaledAmount = static_cast<float>(linkingNum) / 5.0f
    //
    // The route's `amount` field therefore simultaneously encodes linking depth
    // AND modulation strength — a compact single-float encoding that preserves
    // compatibility with CouplingRoute.amount (0.0–1.0 convention).
    //
    // Self-routes (sourceSlot == destSlot) are skipped to prevent feedback.
    // Both source and dest must be non-null (checked by the caller).
    //
    // Control-rate: uses fillControlRateBuffer (controlRateRatio decimation)
    // for both directions. KNOT modulation signals change slowly — no need for
    // audio-rate resolution.
    //
    void processKnotRoute(SynthEngine* source, SynthEngine* dest, const CouplingRoute& route, int numSamples)
    {
        // Self-route guard: an engine cannot be KNOT-coupled to itself.
        if (route.sourceSlot == route.destSlot)
            return;

        // Compute linking number from amount and derive scaled modulation depth.
        // linkingNum range: [1, 5] — maps amount 0.0→1.0 to integer 1→5.
        // Uses static_cast + 0.5f rounding instead of juce::roundToInt to avoid
        // JUCE dependency on the audio thread. NaN guard prevents UB on corrupt input.
        const float rawLinking = route.smoothedAmount * 4.0f;
        const int linkingNum = (std::isnan(rawLinking) ? 0 : static_cast<int>(rawLinking + 0.5f)) + 1;
        const float scaledAmount = static_cast<float>(linkingNum) / 5.0f;

        // Pass A: source → dest (standard direction).
        // Fill the control-rate buffer from source's coupling output cache,
        // then apply it to dest with AmpToFilter semantics.
        fillControlRateBuffer(source, numSamples);
        dest->applyCouplingInput(CouplingType::AmpToFilter, scaledAmount, couplingBuffer.data(), numSamples);

        // Pass B: dest → source (reverse direction — the KNOT difference).
        // Reuse the same scratch buffer (overwrite is safe; Pass A is done).
        // Apply the same scaledAmount so both directions are symmetric.
        fillControlRateBuffer(dest, numSamples);
        source->applyCouplingInput(CouplingType::AmpToFilter, scaledAmount, couplingBuffer.data(), numSamples);
    }
};

} // namespace xoceanus
