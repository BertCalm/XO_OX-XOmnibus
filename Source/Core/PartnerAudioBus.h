// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include <array>
#include <vector>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// PartnerAudioBus — per-engine-slot mono audio bus for FX chains.
//
// Phase 0 wildcard infrastructure (FX gap analysis, 2026-04-27 Pack 1).
//
// Allows FX chains (e.g. OtriumChain) to read each engine slot's most-recent
// mono mix per block without breaching the matrix-mediates-coupling boundary.
// The bus is published by XOceanusProcessor after each engine's renderBlock()
// and consumed by Pack 1 FX chains during their own processBlock().
//
// Lifecycle per audio block:
//   1. processBlock() top → clearForBlock()  (zeroes per-slot sample counts)
//   2. for each engine slot i (0..3):
//        engine[i]->renderBlock(buf[i], midi[i], n)
//        bus.publish(i, buf[i].L, buf[i].R, n)        ← engines write
//   3. epicSlots.processBlock(...)                    ← FX chains read
//        chain.getMono(slot) for slot in {A,B,C}
//
// Threading: single-threaded on the audio thread. Engine writes and chain
// reads are sequential within the same processBlock() call. No locks, no
// atomics — kept simple because the read/write order is deterministic.
//==============================================================================
class PartnerAudioBus
{
public:
    static constexpr int kNumSlots = 4; // matches XOceanus' 4-slot engine fleet

    void prepare(int maxBlockSize)
    {
        maxBlock_ = maxBlockSize;
        for (auto& buf : monoBuffers_)
            buf.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        for (auto& n : slotSamples_) n = 0;
    }

    // Call at the top of each audio block, before any engine renders.
    // Resets all slot sample counts so unpublished slots return nullptr.
    void clearForBlock() noexcept
    {
        for (auto& n : slotSamples_) n = 0;
    }

    // Audio thread: called once per engine slot after its renderBlock().
    // Stores mono mix (0.5 * (L + R)) into the slot's buffer.
    void publish(int slot, const float* L, const float* R, int numSamples) noexcept
    {
        if (slot < 0 || slot >= kNumSlots || numSamples <= 0 || maxBlock_ <= 0)
            return;
        const int n = std::min(numSamples, maxBlock_);
        auto& buf = monoBuffers_[static_cast<size_t>(slot)];
        for (int i = 0; i < n; ++i)
            buf[static_cast<size_t>(i)] = (L[i] + R[i]) * 0.5f;
        slotSamples_[static_cast<size_t>(slot)] = n;
    }

    // Audio thread: read mono buffer for a slot. Pointer is valid until the
    // next clearForBlock() or publish() to the same slot. Returns nullptr if
    // the slot was not published this block (e.g. silence-gated engine).
    const float* getMono(int slot) const noexcept
    {
        if (slot < 0 || slot >= kNumSlots
            || slotSamples_[static_cast<size_t>(slot)] <= 0)
            return nullptr;
        return monoBuffers_[static_cast<size_t>(slot)].data();
    }

    int getNumSamples(int slot) const noexcept
    {
        return (slot < 0 || slot >= kNumSlots)
                   ? 0
                   : slotSamples_[static_cast<size_t>(slot)];
    }

private:
    std::array<std::vector<float>, kNumSlots> monoBuffers_;
    std::array<int, kNumSlots> slotSamples_{};
    int maxBlock_ = 0;
};

} // namespace xoceanus
