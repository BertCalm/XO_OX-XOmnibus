#pragma once
#include "ShaperEngine.h"
#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace xolokun {

//==============================================================================
// Factory function type for creating shaper instances.
using ShaperFactory = std::function<std::unique_ptr<ShaperEngine>()>;

//==============================================================================
// ShaperRegistry — Manages shaper type registration and the 6 active slots.
//
// Slot layout:
//   Insert[0..3] — one per engine, processes engine output before mix
//   Bus[0..1]    — processes the mixed bus before MasterFXChain
//
// Shapers register at compile time via REGISTER_SHAPER macro.
// The processor loads/swaps shapers at runtime with 50ms crossfade.
//
class ShaperRegistry
{
public:
    static constexpr int MaxInserts = 4;
    static constexpr int MaxBus = 2;
    static constexpr int TotalSlots = MaxInserts + MaxBus;
    static constexpr float CrossfadeMs = 50.0f;

    static ShaperRegistry& instance()
    {
        static ShaperRegistry reg;
        return reg;
    }

    //-- Registration ----------------------------------------------------------

    bool registerShaper (const std::string& id, ShaperFactory factory)
    {
        if (factories.count (id) > 0)
        {
            jassertfalse; // Duplicate shaper ID
            return false;
        }
        factories[id] = std::move (factory);
        return true;
    }

    std::unique_ptr<ShaperEngine> createShaper (const std::string& id) const
    {
        auto it = factories.find (id);
        if (it != factories.end())
            return it->second();
        return nullptr;
    }

    std::vector<std::string> getRegisteredShaperIds() const
    {
        std::vector<std::string> ids;
        ids.reserve (factories.size());
        for (const auto& [id, _] : factories)
            ids.push_back (id);
        return ids;
    }

    //-- Slot Management -------------------------------------------------------

    // Load a shaper into an insert slot (0-3). Pass empty string to clear.
    void loadInsert (int slot, const std::string& shaperId, double sampleRate, int maxBlockSize)
    {
        jassert (slot >= 0 && slot < MaxInserts);
        if (shaperId.empty())
        {
            inserts[slot].reset();
            return;
        }
        auto shaper = createShaper (shaperId);
        if (shaper)
        {
            shaper->prepare (sampleRate, maxBlockSize);
            inserts[slot] = std::move (shaper);
        }
    }

    // Load a shaper into a bus slot (0-1). Pass empty string to clear.
    void loadBus (int slot, const std::string& shaperId, double sampleRate, int maxBlockSize)
    {
        jassert (slot >= 0 && slot < MaxBus);
        if (shaperId.empty())
        {
            bus[slot].reset();
            return;
        }
        auto shaper = createShaper (shaperId);
        if (shaper)
        {
            shaper->prepare (sampleRate, maxBlockSize);
            bus[slot] = std::move (shaper);
        }
    }

    // Access active shapers for processing
    ShaperEngine* getInsert (int slot) { return (slot >= 0 && slot < MaxInserts) ? inserts[slot].get() : nullptr; }
    ShaperEngine* getBus (int slot)    { return (slot >= 0 && slot < MaxBus)     ? bus[slot].get()     : nullptr; }

    const ShaperEngine* getInsert (int slot) const { return (slot >= 0 && slot < MaxInserts) ? inserts[slot].get() : nullptr; }
    const ShaperEngine* getBus (int slot) const    { return (slot >= 0 && slot < MaxBus)     ? bus[slot].get()     : nullptr; }

    // Process an engine's output through its insert shaper
    void processInsert (int slot, juce::AudioBuffer<float>& buffer,
                        juce::MidiBuffer& midi, int numSamples)
    {
        auto* shaper = getInsert (slot);
        if (shaper && ! shaper->isBypassed())
        {
            if (shaper->silenceGate.isBypassed())
                return; // SRO: input is silent, skip processing

            shaper->processBlock (buffer, midi, numSamples);
            shaper->silenceGate.analyzeBlock (buffer, numSamples);
        }
    }

    // Process the mixed bus through bus shapers (in series)
    void processBus (juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi, int numSamples)
    {
        for (int i = 0; i < MaxBus; ++i)
        {
            auto* shaper = getBus (i);
            if (shaper && ! shaper->isBypassed())
            {
                if (shaper->silenceGate.isBypassed())
                    continue;

                shaper->processBlock (buffer, midi, numSamples);
                shaper->silenceGate.analyzeBlock (buffer, numSamples);
            }
        }
    }

    // Prepare all active shapers (called from prepareToPlay)
    void prepareAll (double sampleRate, int maxBlockSize)
    {
        for (auto& s : inserts) if (s) s->prepare (sampleRate, maxBlockSize);
        for (auto& s : bus)     if (s) s->prepare (sampleRate, maxBlockSize);
    }

    // Reset all active shapers
    void resetAll()
    {
        for (auto& s : inserts) if (s) s->reset();
        for (auto& s : bus)     if (s) s->reset();
    }

    // Global bypass toggle (A/B comparison)
    void setGlobalBypass (bool bypassed)
    {
        globalBypassed = bypassed;
    }

    bool isGlobalBypassed() const { return globalBypassed; }

private:
    ShaperRegistry() = default;

    std::unordered_map<std::string, ShaperFactory> factories;
    std::array<std::unique_ptr<ShaperEngine>, MaxInserts> inserts {};
    std::array<std::unique_ptr<ShaperEngine>, MaxBus> bus {};
    bool globalBypassed = false;
};

//==============================================================================
// Registration macro — use in each shaper's .cpp file
#define REGISTER_SHAPER(ShaperId, ShaperClass)                               \
    static bool _shaper_##ShaperClass##_registered =                         \
        xolokun::ShaperRegistry::instance().registerShaper(                 \
            ShaperId, []() { return std::make_unique<xolokun::ShaperClass>(); });

} // namespace xolokun
