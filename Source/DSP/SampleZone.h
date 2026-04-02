// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cstring>
#include <algorithm>
#include <array>

namespace xoceanus {

//==============================================================================
// SampleZone — Multi-zone sample mapping for SamplerVoice.
//
// Maps MIDI note + velocity ranges to sample indices (into an external sample
// pool). Supports up to 128 zones, round-robin cycling per zone group, and
// a compact query API that returns all matching zones for a given note + velocity.
//
// Design:
//   A Zone defines a rectangular region of the (note, velocity) space. Multiple
//   zones can overlap the same note/velocity pair — the engine chooses which
//   sample to play (typically the first match, or via round-robin cycling).
//
//   Round-robin is tracked per zone group (identified by an integer groupId).
//   Groups with the same groupId share a common RR counter, so layered zones
//   at the same pitch cycle through alternative samples automatically.
//
// Usage:
//   SampleZone mapping;
//
//   // Build the mapping (non-audio thread):
//   SampleZone::Zone z;
//   z.lowNote  = 60; z.highNote = 72;
//   z.lowVel   = 0;  z.highVel  = 127;
//   z.sampleIndex = 0;
//   z.gain     = 1.0f;
//   z.groupId  = 0;
//   mapping.addZone (z);
//
//   // Query at noteOn (audio thread — read-only, no allocation):
//   SampleZone::QueryResult result;
//   int count = mapping.getZonesForNote (note, velocity, result);
//   for (int i = 0; i < count; ++i)
//   {
//       int sampleIdx = result.zones[i]->sampleIndex;
//       float gain    = result.zones[i]->gain;
//       // ... trigger voice with sampleIdx and gain
//   }
//
//   // Advance round-robin after a noteOn fires:
//   mapping.advanceRoundRobin (groupId);
//
//==============================================================================

struct SampleZone
{
    //==========================================================================
    // Zone descriptor
    //==========================================================================

    struct Zone
    {
        int   lowNote    = 0;     ///< Lowest MIDI note (inclusive)
        int   highNote   = 127;   ///< Highest MIDI note (inclusive)
        int   lowVel     = 0;     ///< Lowest MIDI velocity (inclusive, 0–127)
        int   highVel    = 127;   ///< Highest MIDI velocity (inclusive, 0–127)
        int   sampleIndex = 0;    ///< Index into the engine's sample pool
        float gain       = 1.0f;  ///< Linear gain applied when this zone triggers

        /// Round-robin group ID. Zones sharing the same groupId share a common
        /// RR counter. Use -1 to mark a zone as non-RR (always triggers).
        int   groupId    = -1;

        /// Round-robin position within this zone's group. Used by getZonesForNote
        /// to select which RR layer to activate. Updated by advanceRoundRobin().
        int   rrPosition = 0;
    };

    //==========================================================================
    // Constants
    //==========================================================================

    static constexpr int kMaxZones       = 128;   ///< Full keyboard worth of zones
    static constexpr int kMaxQueryHits   = 16;    ///< Max zones returned per query
    static constexpr int kMaxGroups      = 64;    ///< Max distinct groupId values

    //==========================================================================
    // Query result — stack-allocated, no heap
    //==========================================================================

    struct QueryResult
    {
        /// Pointers into the SampleZone's internal zone array.
        /// Valid until addZone() or clearZones() is called (non-audio-thread ops).
        const Zone* zones[kMaxQueryHits] {};
        int         count = 0;

        void clear() noexcept { count = 0; }
    };

    //==========================================================================
    // Zone management (call from non-audio thread only)
    //==========================================================================

    /// Remove all zones. Safe to call before re-loading a new sample set.
    void clearZones() noexcept
    {
        numZones_ = 0;
        std::memset (rrCounters_, 0, sizeof (rrCounters_));
    }

    /// Add a zone. Returns true if successful, false if the table is full.
    /// Call from a non-audio thread only.
    bool addZone (const Zone& z) noexcept
    {
        if (numZones_ >= kMaxZones) return false;
        zones_[numZones_++] = z;
        return true;
    }

    /// Replace an existing zone by index. Useful for editing presets.
    bool setZone (int index, const Zone& z) noexcept
    {
        if (index < 0 || index >= numZones_) return false;
        zones_[index] = z;
        return true;
    }

    int getNumZones() const noexcept { return numZones_; }

    const Zone* getZone (int index) const noexcept
    {
        if (index < 0 || index >= numZones_) return nullptr;
        return &zones_[index];
    }

    //==========================================================================
    // Query (real-time safe — read-only, no allocation)
    //==========================================================================

    /// Find all zones that cover (note, velocity).
    ///
    /// Round-robin zones: only zones whose rrPosition matches the current RR
    /// counter for their groupId are returned. Non-RR zones (groupId == -1)
    /// are always returned if they match the note/velocity range.
    ///
    /// @param note     MIDI note [0, 127]
    /// @param velocity MIDI velocity [0, 127]
    /// @param out      QueryResult to fill (resets count before writing)
    /// @return         Number of matching zones written into out
    int getZonesForNote (int note, int velocity, QueryResult& out) const noexcept
    {
        out.count = 0;

        for (int i = 0; i < numZones_ && out.count < kMaxQueryHits; ++i)
        {
            const Zone& z = zones_[i];

            // Range check
            if (note     < z.lowNote  || note     > z.highNote)  continue;
            if (velocity < z.lowVel   || velocity > z.highVel)   continue;

            // Round-robin check
            if (z.groupId >= 0 && z.groupId < kMaxGroups)
            {
                if (z.rrPosition != rrCounters_[z.groupId]) continue;
            }

            out.zones[out.count++] = &z;
        }

        return out.count;
    }

    /// Advance the round-robin counter for a group after a noteOn fires.
    /// Call once per noteOn for each groupId returned in the QueryResult.
    /// This rotates to the next layer for the next trigger of the same key.
    ///
    /// @param groupId  The groupId to advance. Ignored if out of range.
    void advanceRoundRobin (int groupId) noexcept
    {
        if (groupId < 0 || groupId >= kMaxGroups) return;

        // Count how many zones belong to this group to wrap the counter
        int groupSize = 0;
        for (int i = 0; i < numZones_; ++i)
            if (zones_[i].groupId == groupId)
                ++groupSize;

        if (groupSize > 0)
            rrCounters_[groupId] = (rrCounters_[groupId] + 1) % groupSize;
    }

    //==========================================================================
    // Helpers
    //==========================================================================

    /// Build a simple chromatic mapping: one zone per MIDI note, all velocities.
    /// The sampleIndex for note N is (N - lowestNote), clamped to [0, numSamples-1].
    /// Useful for keyboard-split instruments where each key has its own sample.
    void buildChromaticMapping (int lowestNote, int highestNote,
                                int numSamples, float uniformGain = 1.0f) noexcept
    {
        clearZones();
        lowestNote  = std::clamp (lowestNote,  0, 127);
        highestNote = std::clamp (highestNote, lowestNote, 127);

        for (int note = lowestNote; note <= highestNote && numZones_ < kMaxZones; ++note)
        {
            Zone z;
            z.lowNote    = note;
            z.highNote   = note;
            z.lowVel     = 0;
            z.highVel    = 127;
            z.sampleIndex = std::clamp (note - lowestNote, 0, numSamples - 1);
            z.gain        = uniformGain;
            z.groupId     = -1;   // no round-robin
            zones_[numZones_++] = z;
        }
    }

    /// Build a velocity-layer mapping: numLayers evenly-divided velocity bands,
    /// all spanning the same note range. Layer 0 = softest, layer N-1 = loudest.
    /// sampleBaseIndex: first layer gets index sampleBaseIndex, next +1, etc.
    void buildVelocityLayers (int lowNote, int highNote,
                              int numLayers, int sampleBaseIndex,
                              int groupId = 0, float uniformGain = 1.0f) noexcept
    {
        if (numLayers <= 0) return;

        const int velRange = 128 / numLayers;

        for (int layer = 0; layer < numLayers && numZones_ < kMaxZones; ++layer)
        {
            Zone z;
            z.lowNote     = std::clamp (lowNote,  0, 127);
            z.highNote    = std::clamp (highNote, 0, 127);
            z.lowVel      = layer * velRange;
            z.highVel     = (layer == numLayers - 1) ? 127 : (layer + 1) * velRange - 1;
            z.sampleIndex = sampleBaseIndex + layer;
            z.gain        = uniformGain;
            z.groupId     = groupId;
            z.rrPosition  = layer;
            zones_[numZones_++] = z;
        }
    }

    //==========================================================================
    // State queries
    //==========================================================================

    int getRoundRobinCounter (int groupId) const noexcept
    {
        if (groupId < 0 || groupId >= kMaxGroups) return 0;
        return rrCounters_[groupId];
    }

    /// Returns true if (note, velocity) maps to at least one zone.
    bool hasZoneForNote (int note, int velocity) const noexcept
    {
        for (int i = 0; i < numZones_; ++i)
        {
            const Zone& z = zones_[i];
            if (note     >= z.lowNote  && note     <= z.highNote &&
                velocity >= z.lowVel   && velocity <= z.highVel)
                return true;
        }
        return false;
    }

private:
    //==========================================================================
    // Storage
    //==========================================================================
    std::array<Zone, kMaxZones> zones_ {};
    int numZones_ = 0;

    // Per-group round-robin counters. Index = groupId.
    int rrCounters_[kMaxGroups] {};
};

} // namespace xoceanus
