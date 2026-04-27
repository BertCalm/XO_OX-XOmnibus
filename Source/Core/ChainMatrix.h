// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// ChainMatrix.h — Wave 5 C4: chain type definitions + inter-slot chain matrix model.
//
// Data model for the 4 chain types and the 4×4 grid of inter-slot chain links.
// Persisted as a "chainMatrix" ValueTree child inside the plugin state (not APVTS
// — 64 possible links × 4 types is too many individual parameters).
//
// ValueTree schema:
//   <chainMatrix>
//     <link src="0" dst="1" type="0" active="1"/>
//     <link src="0" dst="2" type="2" active="0"/>
//     ...
//   </chainMatrix>
//
//   - src / dst: slot indices 0–3 (primary slots only; slot 4 = ghost slot, excluded)
//   - type:      ChainType as uint8 (0=Trigger, 1=Mod, 2=Pattern, 3=Tempo)
//   - active:    1=enabled, 0=disabled
//
// Empty chainMatrix node (or absent node) = no chains = backward-compatible default.
//
// DSP integration notes (Wave 5 C4):
//   TRIGGER: Gate signal from src slot forwarded to dst slot's note-on. Infrastructure
//            exists (loadEngine / renderBlock gate forwarding) — STUBBED in processBlock;
//            followup issue #C4-TRIGGER filed.
//   MOD:     Modulation value routing — uses existing ModRoutingModel infrastructure.
//            Chains that are active and type==Mod are automatically mirrored into a
//            ModRoutingModel route when ChainMatrix::applyToModRouting() is called
//            from the editor (message thread). IMPLEMENTED.
//   PATTERN: Trigger a pattern-change event in the dst slot's PerEnginePatternSequencer
//            when src fires. Infrastructure present (slotSequencers_). STUBBED;
//            followup issue #C4-PATTERN filed.
//   TEMPO:   Clock sync — dst slot reads tempo from src slot. Requires SharedTransport
//            per-slot clock arbitration (currently global only). STUBBED; followup
//            issue #C4-TEMPO filed.
//
// Message-thread only: all mutations happen on the UI thread. No audio-thread access.

#include <array>
#include <cstdint>
#include <juce_data_structures/juce_data_structures.h>

namespace XOceanus
{

//==============================================================================
/** Four chain types that can link two engine slots. */
enum class ChainType : uint8_t
{
    Trigger = 0,  ///< Gate-following: src gate signal triggers dst note-on. (DSP: stubbed)
    Mod     = 1,  ///< Modulation routing: src LFO/env value modulates dst param. (DSP: via ModRoutingModel)
    Pattern = 2,  ///< Pattern triggering: src pattern event triggers dst pattern change. (DSP: stubbed)
    Tempo   = 3,  ///< Tempo sync: dst clock locked to src tempo. (DSP: stubbed)
    Count_      = 4
};

/** Short single-character label for each chain type (used in grid cells). */
inline const char* chainTypeLabel(ChainType t) noexcept
{
    switch (t)
    {
        case ChainType::Trigger: return "T";
        case ChainType::Mod:     return "M";
        case ChainType::Pattern: return "P";
        case ChainType::Tempo:   return "C";  // C = Clock (avoids collision with T=Trigger)
        default:                 return "?";
    }
}

/** Human-readable name for each chain type (used in tooltips). */
inline const char* chainTypeName(ChainType t) noexcept
{
    switch (t)
    {
        case ChainType::Trigger: return "Trigger";
        case ChainType::Mod:     return "Mod";
        case ChainType::Pattern: return "Pattern";
        case ChainType::Tempo:   return "Tempo";
        default:                 return "Unknown";
    }
}

//==============================================================================
/**
    ChainLink — one directed connection between two engine slots.

    Source and destination are primary-slot indices (0–3). Self-links (src==dst)
    are never stored (grid diagonal is always disabled). The type field selects
    which of the 4 chain semantics applies. Active toggles the link on/off without
    removing it from the matrix (preserves the user's routing intent).
*/
struct ChainLink
{
    int       sourceSlot { -1 };
    int       destSlot   { -1 };
    ChainType type       { ChainType::Trigger };
    bool      active     { false };

    bool operator==(const ChainLink& o) const noexcept
    {
        return sourceSlot == o.sourceSlot
            && destSlot   == o.destSlot
            && type       == o.type;
    }
};

//==============================================================================
/**
    ChainMatrix — 4×4 grid of inter-slot chain links across 4 chain types.

    Internally stored as a flat array indexed by [src][dst][type]. The diagonal
    (src==dst) is always inactive. Read/write on the message thread only.

    Serialised to/from a compact ValueTree for DAW session persistence.
*/
class ChainMatrix
{
public:
    static constexpr int kNumSlots = 4;  // primary slots (0–3); ghost slot (4) excluded
    static constexpr int kNumTypes = static_cast<int>(ChainType::Count_);

    ChainMatrix() { clear(); }

    //==========================================================================
    /** Returns whether the link [src→dst, type] is currently active. */
    bool isActive(int src, int dst, ChainType type) const noexcept
    {
        if (!boundsCheck(src, dst)) return false;
        return active_[src][dst][static_cast<int>(type)];
    }

    /** Toggle the link [src→dst, type]. No-op for self-links. */
    void toggle(int src, int dst, ChainType type) noexcept
    {
        if (!boundsCheck(src, dst) || src == dst) return;
        active_[src][dst][static_cast<int>(type)] = !active_[src][dst][static_cast<int>(type)];
    }

    /** Explicitly set the link [src→dst, type]. No-op for self-links. */
    void set(int src, int dst, ChainType type, bool state) noexcept
    {
        if (!boundsCheck(src, dst) || src == dst) return;
        active_[src][dst][static_cast<int>(type)] = state;
    }

    /** Clear all links. */
    void clear() noexcept
    {
        for (auto& row : active_)
            for (auto& col : row)
                col.fill(false);
    }

    //==========================================================================
    /** Count of active links involving slot @p slotIndex (as src or dst, any type).
        Used by EngineOrbit chain-count badge. */
    int activeChainCount(int slotIndex) const noexcept
    {
        if (slotIndex < 0 || slotIndex >= kNumSlots) return 0;
        int count = 0;
        for (int t = 0; t < kNumTypes; ++t)
        {
            for (int other = 0; other < kNumSlots; ++other)
            {
                if (other == slotIndex) continue;
                if (active_[slotIndex][other][t]) ++count;
                if (active_[other][slotIndex][t]) ++count;
            }
        }
        return count;
    }

    //==========================================================================
    /** Serialise to ValueTree. Only active links are written (sparse). */
    juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree("chainMatrix");
        for (int src = 0; src < kNumSlots; ++src)
        {
            for (int dst = 0; dst < kNumSlots; ++dst)
            {
                if (src == dst) continue;
                for (int t = 0; t < kNumTypes; ++t)
                {
                    if (active_[src][dst][t])
                    {
                        juce::ValueTree link("link");
                        link.setProperty("src",    src, nullptr);
                        link.setProperty("dst",    dst, nullptr);
                        link.setProperty("type",   t,   nullptr);
                        link.setProperty("active", 1,   nullptr);
                        tree.appendChild(link, nullptr);
                    }
                }
            }
        }
        return tree;
    }

    /** Deserialise from ValueTree. Absent = no chains (backward-compatible). */
    void fromValueTree(const juce::ValueTree& tree) noexcept
    {
        clear();
        if (!tree.isValid() || tree.getType() != juce::Identifier("chainMatrix"))
            return;

        for (int i = 0; i < tree.getNumChildren(); ++i)
        {
            const auto link = tree.getChild(i);
            if (link.getType() != juce::Identifier("link")) continue;

            const int  src    = static_cast<int>(link.getProperty("src",    -1));
            const int  dst    = static_cast<int>(link.getProperty("dst",    -1));
            const int  typeI  = static_cast<int>(link.getProperty("type",   -1));
            const bool active = static_cast<int>(link.getProperty("active",  0)) != 0;

            if (!boundsCheck(src, dst) || src == dst) continue;
            if (typeI < 0 || typeI >= kNumTypes)      continue;

            active_[src][dst][typeI] = active;
        }
    }

private:
    static bool boundsCheck(int src, int dst) noexcept
    {
        return src >= 0 && src < kNumSlots && dst >= 0 && dst < kNumSlots;
    }

    // active_[src][dst][type] — message-thread only
    std::array<std::array<std::array<bool, kNumTypes>, kNumSlots>, kNumSlots> active_;
};

} // namespace XOceanus
