// ObservandumEngine.cpp — Crystalline Phase Distortion synthesis
// All DSP is inline in ObservandumEngine.h per XOceanus convention.
#include "ObservandumEngine.h"
// T6: full processor type needed for getModRouteCount / getModRouteDestParamId etc.
// XOceanusProcessor.h includes ObservandumEngine.h, so this include is safe — the
// header guard prevents double-inclusion; by the time we get here ObservandumEngine.h
// is done.
#include "../../XOceanusProcessor.h"

namespace xoceanus
{

// T6: cacheGlobalModRoutes — scan the current global-mod-route snapshot for routes
// that target any of Observandum's 5 modulated parameters.  Stores the route index
// (or -1) per target so renderBlock() can call applyObservGlobalModRoutes() in O(1)
// without strncmp.
//
// Thread-safety: called on the message thread (from setProcessorPtr() and from any
// future flushModRoutesSnapshot() callback).  The audio thread reads the cached arrays
// read-only.  A one-block lag is acceptable — worst case is a missed mod-offset for
// a single block when a route is added or removed.
//
// DSP safety: no allocation, no locks, no logging.
void ObservandumEngine::cacheGlobalModRoutes() noexcept
{
    // Reset all targets to "no active route"
    for (int t = 0; t < kObservandumGlobalModTargets; ++t)
    {
        observGlobalModRouteIdx_[t]  = -1;
        observGlobalModVelScaled_[t] = false;
        observGlobalModRangeSpan_[t] = 0.0f;
    }

    if (processorPtr_ == nullptr)
    {
        observModAccumPtr_ = nullptr;
        return;
    }

    // Cache the raw accumulator pointer — used by applyObservGlobalModRoutes() without
    // needing to call through the full XOceanusProcessor type in the header.
    observModAccumPtr_ = processorPtr_->getModRouteAccumPtr();

    int numRoutes = processorPtr_->getModRouteCount();
    for (int ri = 0; ri < numRoutes; ++ri)
    {
        const char* destId = processorPtr_->getModRouteDestParamId(ri);
        if (destId == nullptr || destId[0] == '\0')
            continue;

        for (int t = 0; t < kObservandumGlobalModTargets; ++t)
        {
            if (std::strcmp(destId, kObservGlobalModTargetIds[t]) == 0)
            {
                // Last matching route wins if multiple routes target the same param.
                observGlobalModRouteIdx_[t]  = ri;
                observGlobalModVelScaled_[t] = processorPtr_->isModRouteVelocityScaled(ri);
                observGlobalModRangeSpan_[t] = processorPtr_->getModRouteRangeSpan(ri);
                break;
            }
        }
    }
}

} // namespace xoceanus
