// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <vector>
#include <atomic>

namespace xoceanus
{

//==============================================================================
// AudioRingBuffer — lock-free circular audio buffer for cross-engine streaming.
//
// Designed for the AudioToBuffer coupling type introduced in XOpal Phase 1.
// One instance lives per source-engine slot inside OpalEngine (4 total, one
// per MegaCouplingMatrix slot).
//
// Write path (audio thread, called by MegaCouplingMatrix::processAudioRoute):
//   pushBlock() writes source engine stereo audio into the ring buffer every
//   block. When frozen, the main write head is held while a shadow head
//   continues to track elapsed time, enabling seamless resume on unfreeze.
//
// Read path (audio thread, called by OpalEngine grain scheduler):
//   readAt() returns an interpolated sample at a fractional buffer age.
//   fractionalOffset 0.0 = most recent sample; 1.0 = `capacity` samples ago.
//
// Thread safety:
//   - pushBlock() and readAt() both run on the audio thread — no data races.
//   - frozen is written from OPAL's FREEZE gesture handler (also audio thread,
//     driven by a parameter change) using memory_order_release; pushBlock()
//     reads with memory_order_acquire. Correct visibility is guaranteed.
//   - No heap allocation after prepare(). No locks. No exceptions.
//
// Full design spec: Docs/xopal_phase1_architecture.md §15.3
//
struct AudioRingBuffer
{
    static constexpr int kChannels = 2;

    // Per-channel sample storage: data[0] = L, data[1] = R.
    // Layout is planar (separate arrays) so readAt() accesses a single
    // contiguous channel without stride — cache-friendly for the grain scheduler.
    std::vector<float> data[kChannels];

    int capacity = 0; // buffer length in samples

    std::atomic<int> writeHead{0};       // main write position (frozen when freeze is on)
    std::atomic<int> shadowWriteHead{0}; // always-advancing position (tracks live stream
                                         // during freeze for seamless resume)
    std::atomic<bool> frozen{false};     // FREEZE state — set by OpalEngine gesture handler

    //--------------------------------------------------------------------------
    // Allocate buffers for `durationSeconds` of audio at `sampleRate`.
    // Call from OpalEngine::prepare() — never on the audio thread.
    // Calling prepare() again (e.g. on sample-rate change) is safe:
    // std::vector::assign resets content and the atomics are re-initialized.
    //--------------------------------------------------------------------------
    void prepare(int sampleRate, float durationSeconds)
    {
        capacity = static_cast<int>(static_cast<float>(sampleRate) * durationSeconds);
        for (int ch = 0; ch < kChannels; ++ch)
            data[ch].assign(static_cast<size_t>(capacity), 0.0f);

        writeHead.store(0, std::memory_order_relaxed);
        shadowWriteHead.store(0, std::memory_order_relaxed);
        frozen.store(false, std::memory_order_relaxed);
    }

    //--------------------------------------------------------------------------
    // Push a stereo block of audio into the ring buffer.
    //
    // srcL / srcR   — pointers into MegaCouplingMatrix::couplingBuffer[LR].
    //                 These are pre-allocated scratch arrays; no allocation here.
    // numSamples    — number of samples to write (≤ maxBlockSize from prepare())
    // level         — coupling amount (route.amount, 0.0–1.0); scales the write
    //
    // Called on the audio thread from MegaCouplingMatrix::processAudioRoute().
    // Must be real-time safe: no allocation, no I/O, no exceptions.
    //--------------------------------------------------------------------------
    void pushBlock(const float* srcL, const float* srcR, int numSamples, float level)
    {
        if (capacity <= 0 || srcL == nullptr || srcR == nullptr)
            return;

        const bool isFrozen = frozen.load(std::memory_order_acquire);
        int head = writeHead.load(std::memory_order_relaxed);
        int shadowHead = shadowWriteHead.load(std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            // Shadow head always advances — it tracks the live stream position
            // even while frozen, so the main head can rejoin seamlessly on release.
            ++shadowHead;

            if (!isFrozen)
            {
                const int w = head % capacity;
                data[0][static_cast<size_t>(w)] = srcL[i] * level;
                data[1][static_cast<size_t>(w)] = srcR[i] * level;
                ++head;
            }
        }

        // Release-store: grain scheduler's acquire-load in readAt() will see
        // the updated data before the updated writeHead.
        writeHead.store(head, std::memory_order_release);
        shadowWriteHead.store(shadowHead, std::memory_order_release);
    }

    //--------------------------------------------------------------------------
    // Read a single interpolated sample from the ring buffer.
    //
    // channel         — 0 = L, 1 = R
    // fractionalOffset — 0.0 = most recent written sample
    //                    1.0 = `capacity` samples ago (full buffer age)
    //                    Values outside [0, 1] are clamped implicitly by the
    //                    modular arithmetic (reads wrap around the ring).
    //
    // Returns a linearly-interpolated value between adjacent samples.
    // O(1) — no loops, no allocation. Safe to call from the grain scheduler
    // at grain-trigger frequency (up to ~120 grains/sec per voice × 12 voices).
    //--------------------------------------------------------------------------
    float readAt(int channel, float fractionalOffset) const
    {
        if (capacity <= 0 || channel < 0 || channel >= kChannels)
            return 0.0f;

        const int head = writeHead.load(std::memory_order_acquire);

        // Map fractionalOffset to an absolute sample position in the ring.
        // fractionalOffset = 0 → head (most recent); 1 → head - capacity (oldest).
        const float rawPos = static_cast<float>(head) - fractionalOffset * static_cast<float>(capacity);
        const int base = static_cast<int>(rawPos);
        const float frac = rawPos - static_cast<float>(base);

        // Wrap into [0, capacity) — handles negative base correctly.
        const int i0 = ((base % capacity) + capacity) % capacity;
        const int i1 = (((base + 1) % capacity) + capacity) % capacity;

        const float* ch = data[static_cast<size_t>(channel)].data();
        return ch[i0] + frac * (ch[i1] - ch[i0]);
    }

    //--------------------------------------------------------------------------
    // Unfreeze: advance the main write head to the shadow position so that
    // the next pushBlock() call resumes writing at the current live position,
    // not where the freeze began. Call from OpalEngine::handleFreezeRelease()
    // before clearing the frozen flag.
    //--------------------------------------------------------------------------
    void resumeFromShadow()
    {
        const int shadow = shadowWriteHead.load(std::memory_order_relaxed);
        writeHead.store(shadow, std::memory_order_release);
    }
};

} // namespace xoceanus
