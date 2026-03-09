#pragma once
#include "SynthEngine.h"
#include <array>
#include <vector>

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
class MegaCouplingMatrix {
public:
    static constexpr int MaxSlots = 4;

    struct CouplingRoute {
        int sourceSlot;            // 0-3
        int destSlot;              // 0-3
        CouplingType type;
        float amount;              // 0.0 to 1.0
        bool isNormalled;          // true = default, false = user-defined
        bool active = true;
    };

    // Set the engine pointers for the active slots
    void setEngines(std::array<SynthEngine*, MaxSlots> engines)
    {
        activeEngines = engines;
    }

    // Clear all routes
    void clearRoutes() { routes.clear(); }

    // Add a coupling route
    void addRoute(CouplingRoute route) { routes.push_back(route); }

    // Remove user-defined route, re-enabling normalled default if one exists
    void removeUserRoute(int sourceSlot, int destSlot, CouplingType type)
    {
        routes.erase(
            std::remove_if(routes.begin(), routes.end(),
                [&](const CouplingRoute& r) {
                    return r.sourceSlot == sourceSlot
                        && r.destSlot == destSlot
                        && r.type == type
                        && !r.isNormalled;
                }),
            routes.end());

        // Re-enable normalled route if it exists
        for (auto& r : routes)
            if (r.sourceSlot == sourceSlot && r.destSlot == destSlot
                && r.type == type && r.isNormalled)
                r.active = true;
    }

    // Process all coupling routes for a block of audio.
    // Call this between engine renderBlock() calls.
    void processBlock(int numSamples)
    {
        for (const auto& route : routes)
        {
            if (!route.active || route.amount < 0.001f)
                continue;

            auto* source = activeEngines[route.sourceSlot];
            auto* dest = activeEngines[route.destSlot];
            if (!source || !dest)
                continue;

            // Build source buffer from coupling output
            couplingBuffer.resize(static_cast<size_t>(numSamples));
            for (int i = 0; i < numSamples; ++i)
                couplingBuffer[static_cast<size_t>(i)] =
                    source->getSampleForCoupling(0, i);

            dest->applyCouplingInput(route.type, route.amount,
                                    couplingBuffer.data(), numSamples);
        }
    }

    const std::vector<CouplingRoute>& getRoutes() const { return routes; }

private:
    std::vector<CouplingRoute> routes;
    std::array<SynthEngine*, MaxSlots> activeEngines = {};
    std::vector<float> couplingBuffer;
};

} // namespace xomnibus
