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

    // Register an engine type (called at static init via REGISTER_ENGINE)
    bool registerEngine(const std::string& id, EngineFactory factory)
    {
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

private:
    EngineRegistry() = default;
    std::unordered_map<std::string, EngineFactory> factories;
};

//==============================================================================
// REGISTER_ENGINE macro — call in each engine's .cpp file.
//
// Usage:
//   REGISTER_ENGINE(SnapEngine)
//
// This creates a static bool that registers the engine factory at program start.
//
#define REGISTER_ENGINE(EngineClass) \
    static bool registered_##EngineClass = \
        xomnibus::EngineRegistry::instance().registerEngine( \
            #EngineClass, \
            []() -> std::unique_ptr<xomnibus::SynthEngine> { \
                return std::make_unique<EngineClass>(); \
            })

} // namespace xomnibus
