#include "OrigamiEngine.h"
// T6: full processor type needed for getModRouteCount / getModRouteDestParamId etc.
// XOceanusProcessor.h includes OrigamiEngine.h, so this include is safe — the header
// guard prevents double-inclusion; by the time we get here OrigamiEngine.h is done.
#include "../../XOceanusProcessor.h"

namespace xoceanus
{

// T6: cacheGlobalModRoutes — scan the current global-mod-route snapshot for routes
// that target any of Origami's 5 modulated parameters.  Stores the route index (or -1)
// per target so renderBlock() can apply offsets in O(1) without per-sample strcmp.
//
// Thread-safety: called on the message thread (from setProcessorPtr() and from
// XOceanusProcessor::flushModRoutesSnapshot()). The audio thread reads the cached
// arrays read-only. A one-block lag on route add/remove is acceptable — worst case
// is a missed mod-offset for a single block.
//
// DSP safety: no allocation, no locks, no logging.
void OrigamiEngine::cacheGlobalModRoutes() noexcept
{
    // Reset all targets to "no active route"
    for (int t = 0; t < kOrigamiGlobalModTargets; ++t)
    {
        cachedRouteIndices_[t] = -1;
        cachedVelScaled_[t]    = false;
        cachedRangeSpan_[t]    = 0.0f;
    }

    if (processorPtr_ == nullptr)
    {
        modAccumPtr_ = nullptr;
        return;
    }

    // Cache the raw accumulator pointer — used by applyGlobalModRoutes() without
    // needing to call through the full XOceanusProcessor type in the header.
    modAccumPtr_ = processorPtr_->getModRouteAccumPtr();

    int numRoutes = processorPtr_->getModRouteCount();
    for (int ri = 0; ri < numRoutes; ++ri)
    {
        const char* destId = processorPtr_->getModRouteDestParamId(ri);
        if (destId == nullptr || destId[0] == '\0')
            continue;

        for (int t = 0; t < kOrigamiGlobalModTargets; ++t)
        {
            if (std::strcmp(destId, kGlobalModTargetIds[t]) == 0)
            {
                // Last matching route wins if multiple routes target the same param.
                cachedRouteIndices_[t] = ri;
                cachedVelScaled_[t]    = processorPtr_->isModRouteVelocityScaled(ri);
                cachedRangeSpan_[t]    = processorPtr_->getModRouteRangeSpan(ri);
                break;
            }
        }
    }
}

} // namespace xoceanus
