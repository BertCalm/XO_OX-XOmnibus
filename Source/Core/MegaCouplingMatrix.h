#pragma once
#include "SynthEngine.h"
#include "IAudioBufferSink.h"
#include <array>
#include <vector>
#include <atomic>
#include <memory>
#include <algorithm>

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
        for (auto& v : audioBufferAdjacency) v.clear();
        auto newRoutes = std::make_shared<std::vector<CouplingRoute>>();
        std::atomic_store(&routeList, newRoutes);
    }

    void addRoute(CouplingRoute route)
    {
        auto current = std::atomic_load(&routeList);
        auto newRoutes = std::make_shared<std::vector<CouplingRoute>>(*current);

        if (route.type == CouplingType::AudioToBuffer)
        {
            // Reject self-routes immediately.
            if (route.sourceSlot == route.destSlot)
            {
                juce::Logger::writeToLog ("AudioToBuffer: rejected self-route on slot "
                                          + juce::String (route.sourceSlot));
                return;
            }

            // Reject duplicate (srcSlot, dstSlot) pairs — second push would overwrite first.
            for (const auto& existing : *newRoutes)
            {
                if (existing.type == CouplingType::AudioToBuffer
                    && existing.sourceSlot == route.sourceSlot
                    && existing.destSlot   == route.destSlot)
                {
                    juce::Logger::writeToLog ("AudioToBuffer: duplicate route rejected");
                    return;
                }
            }

            // Reject routes that would create a feedback cycle (A→B→A, A→B→C→A, ...).
            // Normalled routes are authored acyclic — skip cycle check for them.
            if (!route.isNormalled && wouldCreateCycle (route.sourceSlot, route.destSlot))
            {
                juce::Logger::writeToLog ("AudioToBuffer: cycle detected, route "
                    + juce::String (route.sourceSlot) + " \xe2\x86\x92 "
                    + juce::String (route.destSlot) + " rejected");
                return;
            }

            // Safe to commit — register in adjacency list.
            audioBufferAdjacency[static_cast<size_t>(route.sourceSlot)]
                .push_back (route.destSlot);
        }

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

        // Remove from adjacency list if this was an AudioToBuffer route.
        if (type == CouplingType::AudioToBuffer)
        {
            auto& neighbors = audioBufferAdjacency[static_cast<size_t>(sourceSlot)];
            neighbors.erase (std::remove (neighbors.begin(), neighbors.end(), destSlot),
                             neighbors.end());
        }

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

    // Directed adjacency list for AudioToBuffer cycle detection (message thread only).
    // audioBufferAdjacency[srcSlot] = dest slots reachable from srcSlot via AudioToBuffer.
    std::array<std::vector<int>, MaxSlots> audioBufferAdjacency;

    //-- AudioToBuffer cycle detection (message thread, O(MaxSlots²) = O(16)) --
    // DFS from dstSlot: if srcSlot is reachable, adding srcSlot→dstSlot closes a loop.
    bool wouldCreateCycle (int srcSlot, int dstSlot) const
    {
        bool visited[MaxSlots] = {};
        int  stack[MaxSlots];
        int  top = 0;
        stack[top++] = dstSlot;

        while (top > 0)
        {
            int node = stack[--top];
            if (node == srcSlot) return true;
            if (visited[node])   continue;
            visited[node] = true;
            for (int neighbor : audioBufferAdjacency[static_cast<size_t>(node)])
                stack[top++] = neighbor;
        }
        return false;
    }

    //-- AudioToBuffer push path -----------------------------------------------
    //
    // Phase 3 implementation (Round 12D):
    //
    //   1. Self-route and cycle checks are fully handled in addRoute().
    //      processAudioRoute() trusts that the committed route list is acyclic.
    //
    //   2. Fills couplingBuffer (L) and couplingBufferR (R) from source cache.
    //
    //   3. Downcasts dest to IAudioBufferSink. Any conforming engine receives;
    //      non-conforming engines return nullptr and are silently skipped.
    //
    //   4. Validates sourceSlot against sink->getNumInputSlots().
    //
    //   5. pushBlock() into the ring buffer — lock-free, allocation-free.
    //
    // Full design: Docs/xopal_phase1_architecture.md §15.4
    // Phase 3 spec: Docs/audio_to_buffer_phase3_spec.md
    //
    void processAudioRoute (SynthEngine* source, SynthEngine* dest,
                            const CouplingRoute& route, int numSamples)
    {
        // Step 1: fill stereo scratch buffers from source coupling cache.
        for (int i = 0; i < numSamples; ++i)
        {
            couplingBuffer [static_cast<size_t>(i)] = source->getSampleForCoupling(0, i);
            couplingBufferR[static_cast<size_t>(i)] = source->getSampleForCoupling(1, i);
        }

        // Step 2: query dest for IAudioBufferSink (Phase 3: replaces OpalEngine* downcast).
        auto* sink = dynamic_cast<IAudioBufferSink*>(dest);
        if (sink == nullptr)
            return;

        // Step 3: validate slot index against the sink's declared capacity.
        if (route.sourceSlot >= sink->getNumInputSlots())
        {
            juce::Logger::writeToLog ("AudioToBuffer: sourceSlot "
                + juce::String (route.sourceSlot)
                + " exceeds sink capacity "
                + juce::String (sink->getNumInputSlots()));
            return;
        }

        // Step 4: obtain the per-slot ring buffer and push audio.
        AudioRingBuffer* rb = sink->getGrainBuffer (route.sourceSlot);
        if (rb == nullptr)
            return;

        rb->pushBlock (couplingBuffer.data(), couplingBufferR.data(),
                       numSamples, route.amount);
    }
};

} // namespace xomnibus
