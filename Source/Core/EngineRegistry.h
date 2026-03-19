#pragma once
#include "SynthEngine.h"
#include <functional>
#include <unordered_map>
#include <array>

namespace xomnibus {

//==============================================================================
// Factory function type for creating engine instances.
using EngineFactory = std::function<std::unique_ptr<SynthEngine>()>;

//==============================================================================
// EngineRegistry — Manages engine type registration and the 4 active slots.
//
// Engines register at compile time via REGISTER_ENGINE macro.
// The processor loads/swaps engines at runtime with 50ms crossfade.
//
class EngineRegistry {
public:
    static constexpr int MaxSlots = 4;
    static constexpr float CrossfadeMs = 50.0f;

    static EngineRegistry& instance()
    {
        static EngineRegistry reg;
        return reg;
    }

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
    [[nodiscard]] std::unique_ptr<SynthEngine> createEngine(const std::string& id) const
    {
        auto it = factories.find(id);
        if (it != factories.end())
            return it->second();
        return nullptr;
    }

    // Get all registered engine IDs
    [[nodiscard]] std::vector<std::string> getRegisteredIds() const
    {
        std::vector<std::string> ids;
        ids.reserve(factories.size());
        for (const auto& [id, _] : factories)
            ids.push_back(id);
        return ids;
    }

    [[nodiscard]] bool isRegistered(const std::string& id) const
    {
        return factories.count(id) > 0;
    }

private:
    EngineRegistry() = default;
    std::unordered_map<std::string, EngineFactory> factories;
};

//==============================================================================
// REGISTER_ENGINE macro — convenience for engines whose class name matches
// their canonical engine ID. Currently unused: XOmnibus registers all engines
// centrally in XOmnibusProcessor.cpp with explicit canonical IDs (e.g.
// "OddfeliX" → SnapEngine) because class names don't match engine IDs.
//
#define REGISTER_ENGINE(EngineClass) \
    static bool registered_##EngineClass = \
        xomnibus::EngineRegistry::instance().registerEngine( \
            #EngineClass, \
            []() -> std::unique_ptr<xomnibus::SynthEngine> { \
                return std::make_unique<EngineClass>(); \
            })

} // namespace xomnibus
