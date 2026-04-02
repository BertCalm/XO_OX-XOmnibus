// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "SynthEngine.h"
#include <functional>
#include <unordered_map>
#include <array>

namespace xoceanus {

//==============================================================================
// Factory function type for creating engine instances.
using EngineFactory = std::function<std::unique_ptr<SynthEngine>()>;

//==============================================================================
// EngineRegistry — Manages engine type registration and the 5 active slots.
//
// Engines register centrally in XOceanusProcessor.cpp.
// The processor loads/swaps engines at runtime with 50ms crossfade.
//
// Slot 4 (the 5th slot, 0-indexed) is the "Ghost Slot" — it materialises in
// the editor only when all 4 primary slots contain engines from the same
// Kitchen Collection quad. See detectCollection() below.
//
class EngineRegistry {
public:
    static constexpr int MaxSlots = 5;
    static constexpr float CrossfadeMs = 50.0f;

    static EngineRegistry& instance()
    {
        static EngineRegistry reg;
        return reg;
    }

    // Thread-safety: All registerEngine() calls happen during static initialization
    // before main(). createEngine() is called from the message thread only.
    // No concurrent access is possible under the current architecture.
    // If this changes (e.g., dynamic plugin loading), add a mutex.

    // Register an engine type (called at static init via REGISTER_ENGINE).
    // Returns false if an engine with this ID is already registered.
    bool registerEngine(const std::string& id, EngineFactory factory)
    {
        if (factories.count(id) > 0)
        {
            jassertfalse; // Duplicate engine ID — check REGISTER_ENGINE calls
            return false;
        }
        factories[id] = std::move(factory);
        return true;
    }

    // Create an engine by ID
    std::unique_ptr<SynthEngine> createEngine(const std::string& id) const
    {
        auto it = factories.find(id);
        if (it != factories.end())
            return it->second();
        return nullptr;
    }

    // Get all registered engine IDs
    std::vector<std::string> getRegisteredIds() const
    {
        std::vector<std::string> ids;
        ids.reserve(factories.size());
        for (const auto& [id, _] : factories)
            ids.push_back(id);
        return ids;
    }

    bool isRegistered(const std::string& id) const
    {
        return factories.count(id) > 0;
    }

    // ── Ghost Slot: Kitchen Collection detection ───────────────────────────
    //
    // Returns the collection name if all 4 primary slots (indices 0–3) are
    // occupied and every engine belongs to the same Kitchen Collection quad.
    // Returns an empty string if the condition is not met.
    //
    // Collection membership (engine IDs as returned by SynthEngine::getEngineId()):
    //   Chef    (Organs):  OTO, OCTAVE, OLEG, OTIS
    //   Kitchen (Pianos):  OVEN, OCHRE, OBELISK, OPALINE
    //   Cellar  (Bass):    OGRE, OLATE, OAKEN, OMEGA
    //   Garden  (Strings): ORCHARD, OVERGROW, OSIER, OXALIS
    //   Broth   (Pads):    OVERWASH, OVERWORN, OVERFLOW, OVERCAST
    //   Fusion  (EP):      OKEANOS, ODDFELLOW, ONKOLO, OPCODE
    //
    // slotEngineIds: the engine IDs for slots 0–3 (primary slots only).
    // Empty string = empty slot.
    //
    static juce::String detectCollection(const std::array<juce::String, 4>& slotEngineIds)
    {
        // All 4 primary slots must be occupied.
        for (const auto& id : slotEngineIds)
            if (id.isEmpty())
                return {};

        // Collection membership tables — engine IDs normalised to uppercase.
        struct Collection {
            const char* name;
            const char* members[4];
        };
        static const Collection collections[] =
        {
            { "Chef",    { "OTO", "OCTAVE", "OLEG", "OTIS"                        } },
            { "Kitchen", { "OVEN", "OCHRE", "OBELISK", "OPALINE"                  } },
            { "Cellar",  { "OGRE", "OLATE", "OAKEN", "OMEGA"                      } },
            { "Garden",  { "ORCHARD", "OVERGROW", "OSIER", "OXALIS"               } },
            { "Broth",   { "OVERWASH", "OVERWORN", "OVERFLOW", "OVERCAST"         } },
            { "Fusion",  { "OKEANOS", "ODDFELLOW", "ONKOLO", "OPCODE"             } },
        };

        for (const auto& col : collections)
        {
            // Check whether every loaded engine is in this collection.
            bool allMatch = true;
            for (const auto& slotId : slotEngineIds)
            {
                const juce::String upper = slotId.toUpperCase();
                bool found = false;
                for (const char* member : col.members)
                    if (upper == juce::String(member)) { found = true; break; }
                if (!found) { allMatch = false; break; }
            }
            if (allMatch)
                return juce::String(col.name);
        }

        return {}; // engines span multiple collections, or none matched
    }

private:
    EngineRegistry() = default;
    std::unordered_map<std::string, EngineFactory> factories;
};

} // namespace xoceanus
