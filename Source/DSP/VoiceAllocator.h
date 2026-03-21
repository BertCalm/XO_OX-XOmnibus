#pragma once
#include <cstdint>
#include <algorithm>

namespace xomnibus {

//==============================================================================
// VoiceAllocator — Shared voice allocation and stealing for the XOmnibus fleet.
//
// Consolidates the identical LRU voice-stealing pattern found in 18+ engines.
// Works with any voice array whose elements have `bool active` and
// `uint64_t startTime` fields (convention already followed by all engines).
//
// Two allocation strategies:
//   1. LRU (Least Recently Used) — steals the oldest active voice.
//      Used by Origami, Osteria, Bob, Morph, Dub, Drift, Bite, etc.
//
//   2. ReleasePriority — prefers stealing voices in release stage before
//      attacking/sustaining voices. Used by OpenSky. Requires the caller
//      to provide a "is releasing" predicate.
//
// Usage:
//   // Simple LRU:
//   int idx = VoiceAllocator::findFreeVoice (voices, maxPoly);
//
//   // Release-priority:
//   int idx = VoiceAllocator::findFreeVoicePreferRelease (
//       voices, kMaxVoices,
//       [](const MyVoice& v) { return v.ampEnv.stage == Stage::Release; }
//   );
//
// All functions are noexcept, allocation-free, and real-time safe.
//==============================================================================
struct VoiceAllocator
{
    //--------------------------------------------------------------------------
    // LRU allocation — the standard fleet pattern.
    //
    // Pass 1: find an inactive voice (active == false).
    // Pass 2: steal the oldest active voice (smallest startTime).
    //
    // VoiceArray must be indexable with [i] and elements must have:
    //   bool active;
    //   uint64_t startTime;
    //--------------------------------------------------------------------------
    template <typename VoiceArray>
    static int findFreeVoice (VoiceArray& voices, int maxPolyphony) noexcept
    {
        // Pass 1: find inactive voice
        for (int i = 0; i < maxPolyphony; ++i)
            if (!voices[i].active)
                return i;

        // Pass 2: LRU — steal oldest
        int oldestIdx = 0;
        uint64_t oldestTime = UINT64_MAX;
        for (int i = 0; i < maxPolyphony; ++i)
        {
            if (voices[i].startTime < oldestTime)
            {
                oldestTime = voices[i].startTime;
                oldestIdx = i;
            }
        }
        return oldestIdx;
    }

    //--------------------------------------------------------------------------
    // Release-priority allocation — OpenSky variant.
    //
    // Pass 1: find inactive voice.
    // Pass 2: prefer stealing the oldest voice in release stage.
    // Pass 3: if no release-stage voices, steal the oldest overall.
    //
    // IsReleasingFn: callable(const Voice&) -> bool
    //--------------------------------------------------------------------------
    template <typename VoiceArray, typename IsReleasingFn>
    static int findFreeVoicePreferRelease (VoiceArray& voices, int maxPolyphony,
                                           IsReleasingFn isReleasing) noexcept
    {
        // Pass 1: find inactive voice
        for (int i = 0; i < maxPolyphony; ++i)
            if (!voices[i].active)
                return i;

        // Pass 2+3: prefer release-stage, fallback to oldest overall
        int bestRelease = -1;
        uint64_t oldestReleaseTime = UINT64_MAX;
        int bestAny = 0;
        uint64_t oldestAnyTime = UINT64_MAX;

        for (int i = 0; i < maxPolyphony; ++i)
        {
            if (isReleasing (voices[i]) && voices[i].startTime < oldestReleaseTime)
            {
                oldestReleaseTime = voices[i].startTime;
                bestRelease = i;
            }
            if (voices[i].startTime < oldestAnyTime)
            {
                oldestAnyTime = voices[i].startTime;
                bestAny = i;
            }
        }

        return (bestRelease >= 0) ? bestRelease : bestAny;
    }

    //--------------------------------------------------------------------------
    // Count active voices. Useful for polyphony display and CPU budgeting.
    //--------------------------------------------------------------------------
    template <typename VoiceArray>
    static int countActive (const VoiceArray& voices, int maxPolyphony) noexcept
    {
        int count = 0;
        for (int i = 0; i < maxPolyphony; ++i)
            if (voices[i].active)
                ++count;
        return count;
    }

    //--------------------------------------------------------------------------
    // Find voice playing a specific note. Returns -1 if not found.
    // Voice must have `int currentNote` field.
    //--------------------------------------------------------------------------
    template <typename VoiceArray>
    static int findVoiceForNote (const VoiceArray& voices, int maxPolyphony, int note) noexcept
    {
        for (int i = 0; i < maxPolyphony; ++i)
            if (voices[i].active && voices[i].currentNote == note)
                return i;
        return -1;
    }
};

} // namespace xomnibus
