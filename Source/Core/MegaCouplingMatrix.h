#pragma once
#include "SynthEngine.h"
#include "../Engines/Opal/OpalEngine.h"
#include <array>
#include <vector>
#include <atomic>
#include <memory>

namespace xomnibus {

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
class MegaCouplingMatrix {
public:
    static constexpr int MaxSlots = 4;
    static constexpr int MaxRoutes = 64;

    struct CouplingRoute {
        int sourceSlot;            // 0-3
        int destSlot;              // 0-3
        CouplingType type;
        float amount;              // 0.0 to 1.0
        bool isNormalled;          // true = default, false = user-defined
        bool active = true;
    };

    // Call from prepareToPlay to pre-allocate the coupling scratch buffers.
    // couplingBuffer: L-channel (or mono) scratch for all coupling types.
    // couplingBufferR: R-channel scratch, used exclusively for AudioToBuffer
    //                  routes which require stereo — see processAudioRoute() and
    //                  Docs/xopal_phase1_architecture.md §15.4.
    void prepare(int maxBlockSize)
    {
        couplingBuffer.resize(static_cast<size_t>(maxBlockSize), 0.0f);
        couplingBufferR.resize(static_cast<size_t>(maxBlockSize), 0.0f);
    }

    // Set the engine pointers for the active slots (audio thread)
    void setEngines(std::array<SynthEngine*, MaxSlots> engines)
    {
        activeEngines = engines;
    }

    //-- Route mutation (message thread only) ----------------------------------

    void clearRoutes()
    {
        auto newRoutes = std::make_shared<std::vector<CouplingRoute>>();
        std::atomic_store(&routeList, newRoutes);
    }

    void addRoute(CouplingRoute route)
    {
        auto current = std::atomic_load(&routeList);
        auto newRoutes = std::make_shared<std::vector<CouplingRoute>>(*current);
        newRoutes->push_back(route);
        std::atomic_store(&routeList, newRoutes);
    }

    void removeUserRoute(int sourceSlot, int destSlot, CouplingType type)
    {
        auto current = std::atomic_load(&routeList);
        auto newRoutes = std::make_shared<std::vector<CouplingRoute>>(*current);

        newRoutes->erase(
            std::remove_if(newRoutes->begin(), newRoutes->end(),
                [&](const CouplingRoute& r) {
                    return r.sourceSlot == sourceSlot
                        && r.destSlot == destSlot
                        && r.type == type
                        && !r.isNormalled;
                }),
            newRoutes->end());

        // Re-enable normalled route if it exists
        for (auto& r : *newRoutes)
            if (r.sourceSlot == sourceSlot && r.destSlot == destSlot
                && r.type == type && r.isNormalled)
                r.active = true;

        std::atomic_store(&routeList, newRoutes);
    }

    //-- Audio processing (audio thread only) ----------------------------------

    // Load the current route list once per audio callback.
    // Store the result in a local shared_ptr and pass it to processBlock()
    // to avoid repeated atomic reference count operations inside the block.
    std::shared_ptr<std::vector<CouplingRoute>> loadRoutes() const
    {
        return std::atomic_load(&routeList);
    }

    // Process all coupling routes for a block of audio.
    // Pass the shared_ptr obtained from loadRoutes() — avoids an atomic reload
    // (and its LOCK prefix) inside this method.
    void processBlock(int numSamples,
                      const std::shared_ptr<std::vector<CouplingRoute>>& routes)
    {
        if (!routes || routes->empty())
            return;

        for (const auto& route : *routes)
        {
            if (!route.active || route.amount < 0.001f)
                continue;

            // Bounds check slot indices to prevent OOB access
            if (route.sourceSlot < 0 || route.sourceSlot >= MaxSlots
                || route.destSlot < 0 || route.destSlot >= MaxSlots)
                continue;

            auto* source = activeEngines[static_cast<size_t>(route.sourceSlot)];
            auto* dest = activeEngines[static_cast<size_t>(route.destSlot)];
            if (!source || !dest)
                continue;

            // Use pre-allocated scratch buffer (sized in prepare()).
            // Audio coupling types carry stereo content — mix L+R to mono.
            // Modulation coupling types (LFO, env, amp) are inherently mono.
            // AudioToBuffer is an audio route but is handled separately via
            // processAudioRoute() below — it bypasses the mono mixdown.
            const bool isAudioRoute =
                route.type == CouplingType::AudioToWavetable
             || route.type == CouplingType::AudioToFM
             || route.type == CouplingType::AudioToRing
             || route.type == CouplingType::AudioToBuffer;

            const int limit = juce::jmin(numSamples, static_cast<int>(couplingBuffer.size()));

            // AudioToBuffer routes bypass the mono mixdown — they require true
            // stereo and write directly into the destination's AudioRingBuffer.
            // Handled by processAudioRoute(); skip the standard path.
            if (route.type == CouplingType::AudioToBuffer)
            {
                processAudioRoute(source, dest, route, limit);
                continue;
            }

            if (isAudioRoute)
            {
                for (int i = 0; i < limit; ++i)
                    couplingBuffer[static_cast<size_t>(i)] =
                        (source->getSampleForCoupling(0, i)
                       + source->getSampleForCoupling(1, i)) * 0.5f;
            }
            else
            {
                for (int i = 0; i < limit; ++i)
                    couplingBuffer[static_cast<size_t>(i)] =
                        source->getSampleForCoupling(0, i);
            }

            dest->applyCouplingInput(route.type, route.amount,
                                    couplingBuffer.data(), numSamples);
        }
    }

    std::vector<CouplingRoute> getRoutes() const
    {
        auto routes = std::atomic_load(&routeList);
        return routes ? *routes : std::vector<CouplingRoute>{};
    }

private:
    std::shared_ptr<std::vector<CouplingRoute>> routeList =
        std::make_shared<std::vector<CouplingRoute>>();
    std::array<SynthEngine*, MaxSlots> activeEngines = {};
    std::vector<float> couplingBuffer;   // L / mono scratch — pre-allocated in prepare()
    std::vector<float> couplingBufferR;  // R scratch for AudioToBuffer stereo push

    //-- AudioToBuffer push path -----------------------------------------------
    //
    // Phase 2 implementation (Round 11F):
    //
    //   1. Cycle detection: if source == dest (same engine slot), skip entirely.
    //      Prevents a single engine from streaming into its own grain buffer,
    //      which would create infinite DC feedback in the granulator.
    //      Full graph-level cycle detection (A→B→A) is deferred to Phase 3.
    //
    //   2. Fills couplingBuffer (L) and couplingBufferR (R) with the source
    //      engine's per-sample output cache via getSampleForCoupling().
    //
    //   3. Downcasts dest to OpalEngine — OPAL is the sole AudioToBuffer
    //      receiver in Phase 2. A future IAudioBufferSink interface will
    //      replace the downcast once additional receiver engines exist (Phase 3).
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
    void processAudioRoute(SynthEngine* source, SynthEngine* dest,
                           const CouplingRoute& route, int numSamples)
    {
        // Phase 2 cycle detection: skip self-routes.
        // A source slot routing to itself (AudioToBuffer with sourceSlot == destSlot)
        // would feed the grain buffer with its own output — instant DC accumulation.
        if (route.sourceSlot == route.destSlot)
            return;

        // Step 1: fill stereo scratch buffers from source coupling cache.
        for (int i = 0; i < numSamples; ++i)
        {
            couplingBuffer[static_cast<size_t>(i)]  = source->getSampleForCoupling(0, i);
            couplingBufferR[static_cast<size_t>(i)] = source->getSampleForCoupling(1, i);
        }

        // Step 2: downcast dest to OpalEngine — the only AudioToBuffer receiver in Phase 2.
        // dynamic_cast returns nullptr if dest is not an OpalEngine; this is the
        // safe guard for all non-OPAL destinations until Phase 3 adds an interface.
        auto* opalDest = dynamic_cast<OpalEngine*>(dest);
        if (opalDest == nullptr)
            return;

        // Step 3: obtain the per-slot ring buffer.
        // route.sourceSlot identifies which input slot on OPAL receives this source.
        // OpalEngine::getGrainBuffer() returns nullptr for out-of-range slots.
        AudioRingBuffer* rb = opalDest->getGrainBuffer(route.sourceSlot);
        if (rb == nullptr)
            return;

        // Step 4: push audio into the ring buffer.
        // pushBlock() is lock-free and allocation-free. The level parameter
        // scales each written sample by route.amount (0.0–1.0).
        // Freeze state is managed internally by AudioRingBuffer::pushBlock().
        rb->pushBlock(couplingBuffer.data(), couplingBufferR.data(),
                      numSamples, route.amount);

        // Do NOT call dest->applyCouplingInput() — the ring buffer is the exclusive
        // sink for AudioToBuffer routes. OpalEngine reads from it during renderBlock().
    }
};

} // namespace xomnibus
