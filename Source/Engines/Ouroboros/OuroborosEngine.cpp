#include "OuroborosEngine.h"
// T6: full processor type needed for getModRouteCount / getModRouteDestParamId etc.
// XOceanusProcessor.h includes OuroborosEngine.h so the header guard prevents
// double-inclusion; by the time we get here OuroborosEngine.h is already done.
#include "../../XOceanusProcessor.h"

namespace xoceanus
{

// T6: cacheGlobalModRoutes — scan the current global-mod-route snapshot for routes
// that target any of Ouroboros's 5 modulated parameters.  Stores the route index
// (or -1) per target so renderBlock() can call getModRouteAccum() in O(1) without
// strncmp on the audio thread.
//
// Thread-safety: called on the message thread (from setProcessorPtr() and from any
// future flushModRoutesSnapshot() callback).  The audio thread reads the cached arrays
// read-only.  A one-block lag is acceptable — worst case is a missed mod-offset for
// a single block when a route is added or removed.
//
// DSP safety: no allocation, no locks, no logging.
void OuroborosEngine::cacheGlobalModRoutes() noexcept
{
    // Reset all targets to "no active route"
    for (int t = 0; t < kOuroborosGlobalModTargets; ++t)
    {
        globalModRouteIdx_[t]  = -1;
        globalModVelScaled_[t] = false;
        globalModRangeSpan_[t] = 0.0f;
    }

    if (processorPtr_ == nullptr)
    {
        modAccumPtr_ = nullptr;
        return;
    }

    // Cache the raw accumulator pointer — used by renderBlock() without needing
    // to call through the full XOceanusProcessor type in the header.
    modAccumPtr_ = processorPtr_->getModRouteAccumPtr();

    int numRoutes = processorPtr_->getModRouteCount();
    for (int ri = 0; ri < numRoutes; ++ri)
    {
        const char* destId = processorPtr_->getModRouteDestParamId(ri);
        if (destId == nullptr || destId[0] == '\0')
            continue;

        for (int t = 0; t < kOuroborosGlobalModTargets; ++t)
        {
            if (std::strcmp(destId, kGlobalModTargetIds[t]) == 0)
            {
                // Last matching route wins if multiple routes target the same param.
                globalModRouteIdx_[t]  = ri;
                globalModVelScaled_[t] = processorPtr_->isModRouteVelocityScaled(ri);
                globalModRangeSpan_[t] = processorPtr_->getModRouteRangeSpan(ri);
                break;
            }
        }
    }
}

} // namespace xoceanus
