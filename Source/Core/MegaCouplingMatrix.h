#pragma once
#include "SynthEngine.h"
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

    // Call from prepareToPlay to pre-allocate the coupling scratch buffer.
    void prepare(int maxBlockSize)
    {
        couplingBuffer.resize(static_cast<size_t>(maxBlockSize), 0.0f);
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

    // Process all coupling routes for a block of audio.
    // Call this between engine renderBlock() calls.
    void processBlock(int numSamples)
    {
        // Atomically grab the current route list — no lock, no allocation
        auto routes = std::atomic_load(&routeList);
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

            // Use pre-allocated scratch buffer (sized in prepare())
            for (int i = 0; i < numSamples && i < static_cast<int>(couplingBuffer.size()); ++i)
                couplingBuffer[static_cast<size_t>(i)] =
                    source->getSampleForCoupling(0, i);

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
    std::vector<float> couplingBuffer;  // pre-allocated in prepare()
};

} // namespace xomnibus
