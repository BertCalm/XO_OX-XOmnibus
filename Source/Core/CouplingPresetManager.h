// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SynthEngine.h"
#include "MegaCouplingMatrix.h"
#include "CouplingCrossfader.h"
#include "PresetManager.h"
#include <vector>
#include <algorithm>
#include <cmath>

namespace xoceanus {

//==============================================================================
// BakedCouplingState — Snapshot of the coupling performance overlay.
//
// Captures the complete live coupling configuration so it can be serialized,
// stored, and recalled. This is the data model for the BAKE button.
//
// Design principles:
//   - Captures APVTS cp_* param values, NOT MegaCouplingMatrix internal state.
//     The overlay is the user-facing performance layer; baseline routes are
//     part of the preset system and handled by PresetManager.
//   - Engine names are stored by canonical ID (not slot index) so baked
//     couplings survive engine slot reordering.
//   - All fields have safe defaults — a default-constructed BakedCouplingState
//     represents "no coupling" (all routes inactive, zero amounts).
//
struct BakedCouplingState
{
    static constexpr int kMaxRoutes = CouplingCrossfader::MaxRouteSlots;  // 4

    struct Route
    {
        bool active = false;
        int couplingTypeIndex = 0;         // CouplingType enum index (0 = AmpToFilter)
        float amount = 0.0f;               // -1.0 to 1.0 (bipolar)
        int sourceSlot = 0;                 // 0-3
        int targetSlot = 1;                 // 0-3
        juce::String sourceEngineName;      // Canonical engine ID at bake time
        juce::String targetEngineName;      // Canonical engine ID at bake time
    };

    juce::String name;                      // User-assigned preset name
    juce::String description;               // Optional description
    juce::String author = "User";
    std::array<Route, kMaxRoutes> routes;
    std::array<float, 4> macroValues = {{ 0.5f, 0.5f, 0.5f, 0.5f }};  // Macro 1-4

    //==========================================================================
    // Serialization — to/from juce::var (JSON-compatible)
    //==========================================================================

    juce::var toJSON() const
    {
        auto* root = new juce::DynamicObject();
        root->setProperty("schema_version", 1);
        root->setProperty("format", "xo_coupling_preset");
        root->setProperty("name", name);
        root->setProperty("description", description);
        root->setProperty("author", author);

        // Routes
        juce::var routesArray;
        for (int i = 0; i < kMaxRoutes; ++i)
        {
            const auto& r = routes[static_cast<size_t>(i)];
            auto* routeObj = new juce::DynamicObject();
            routeObj->setProperty("active", r.active);
            routeObj->setProperty("type", r.couplingTypeIndex);
            routeObj->setProperty("amount", static_cast<double>(r.amount));
            routeObj->setProperty("sourceSlot", r.sourceSlot);
            routeObj->setProperty("targetSlot", r.targetSlot);
            routeObj->setProperty("sourceEngine", r.sourceEngineName);
            routeObj->setProperty("targetEngine", r.targetEngineName);
            routesArray.append(juce::var(routeObj));
        }
        root->setProperty("routes", routesArray);

        // Macros
        juce::var macrosArray;
        for (int i = 0; i < 4; ++i)
            macrosArray.append(static_cast<double>(macroValues[static_cast<size_t>(i)]));
        root->setProperty("macros", macrosArray);

        return juce::var(root);
    }

    static BakedCouplingState fromJSON(const juce::var& json)
    {
        BakedCouplingState state;

        if (!json.isObject())
            return state;

        auto* obj = json.getDynamicObject();
        if (!obj)
            return state;

        // Validate format tag
        auto format = obj->getProperty("format").toString();
        if (format.isNotEmpty() && format != "xo_coupling_preset")
            return state;

        state.name        = obj->getProperty("name").toString();
        state.description = obj->getProperty("description").toString();
        state.author      = obj->getProperty("author").toString();
        if (state.author.isEmpty())
            state.author = "User";

        // Routes
        auto routesVar = obj->getProperty("routes");
        if (routesVar.isArray())
        {
            auto* arr = routesVar.getArray();
            int count = juce::jmin(kMaxRoutes, static_cast<int>(arr->size()));
            for (int i = 0; i < count; ++i)
            {
                auto routeVar = (*arr)[i];
                if (!routeVar.isObject()) continue;
                auto* routeObj = routeVar.getDynamicObject();
                if (!routeObj) continue;

                auto& r = state.routes[static_cast<size_t>(i)];
                r.active           = static_cast<bool>(routeObj->getProperty("active"));
                r.couplingTypeIndex = juce::jlimit(0, static_cast<int>(CouplingType::TriangularCoupling),
                                                   static_cast<int>(routeObj->getProperty("type")));
                r.amount           = juce::jlimit(-1.0f, 1.0f,
                                                   static_cast<float>(routeObj->getProperty("amount")));
                r.sourceSlot       = juce::jlimit(0, 3, static_cast<int>(routeObj->getProperty("sourceSlot")));
                r.targetSlot       = juce::jlimit(0, 3, static_cast<int>(routeObj->getProperty("targetSlot")));
                r.sourceEngineName = resolveEngineAlias(routeObj->getProperty("sourceEngine").toString());
                r.targetEngineName = resolveEngineAlias(routeObj->getProperty("targetEngine").toString());
            }
        }

        // Macros
        auto macrosVar = obj->getProperty("macros");
        if (macrosVar.isArray())
        {
            auto* arr = macrosVar.getArray();
            int count = juce::jmin(4, static_cast<int>(arr->size()));
            for (int i = 0; i < count; ++i)
                state.macroValues[static_cast<size_t>(i)] =
                    juce::jlimit(0.0f, 1.0f, static_cast<float>((*arr)[i]));
        }

        return state;
    }

    //==========================================================================
    // Validity check
    //==========================================================================

    bool hasActiveRoutes() const
    {
        for (const auto& r : routes)
            if (r.active && std::abs(r.amount) > 0.001f)
                return true;
        return false;
    }
};

//==============================================================================
// CouplingPresetManager — Bake, save, load, and browse coupling presets.
//
// This is the Phase D implementation for the Coupling Performance System.
// It manages a library of baked coupling states stored as `.xocoupling` JSON files.
//
// Design:
//   - UI-thread only (no audio-thread calls)
//   - File format: `.xocoupling` JSON (same parse/serialize pattern as `.xometa`)
//   - Storage: `CouplingPresets/` directory alongside engine presets
//   - Maximum file size guard (same as PresetManager: 1 MB)
//   - Backward compatible: old sessions ignore coupling presets entirely
//
// Usage:
//   CouplingPresetManager cpm(apvts, getEngineFn);
//   auto state = cpm.bakeCurrent();             // Capture current overlay
//   cpm.saveToFile(file, state);                // Persist
//   cpm.loadBakedCoupling(state);               // Restore to APVTS
//
class CouplingPresetManager
{
public:
    static constexpr int64_t kMaxCouplingFileSize = 1024 * 1024;

    //--------------------------------------------------------------------------
    // Listener interface — notified when the coupling preset library changes.
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void couplingPresetLibraryChanged() = 0;
    };

    //--------------------------------------------------------------------------
    // Constructor requires APVTS reference (for reading/writing cp_* params)
    // and a function to resolve slot index → engine ID at bake time.
    CouplingPresetManager(
        juce::AudioProcessorValueTreeState& apvtsRef,
        std::function<juce::String(int)> slotToEngineIdFn)
        : apvts(apvtsRef),
          getEngineIdForSlot(std::move(slotToEngineIdFn))
    {
    }

    //==========================================================================
    // BAKE — Capture the current coupling performance overlay state.
    //==========================================================================

    // Read all cp_* params from APVTS and engine names from slots.
    // Returns a BakedCouplingState snapshot of the current live configuration.
    BakedCouplingState bakeCurrent(const juce::String& presetName = "Untitled") const
    {
        BakedCouplingState state;
        state.name = presetName;

        for (int r = 0; r < BakedCouplingState::kMaxRoutes; ++r)
        {
            juce::String prefix = "cp_r" + juce::String(r + 1) + "_";
            auto& route = state.routes[static_cast<size_t>(r)];

            // Read APVTS values
            if (auto* p = apvts.getRawParameterValue(prefix + "active"))
                route.active = p->load() > 0.5f;

            if (auto* p = apvts.getRawParameterValue(prefix + "type"))
                route.couplingTypeIndex = juce::jlimit(0,
                    static_cast<int>(CouplingType::TriangularCoupling),
                    juce::roundToInt(p->load()));

            if (auto* p = apvts.getRawParameterValue(prefix + "amount"))
                route.amount = juce::jlimit(-1.0f, 1.0f, p->load());

            if (auto* p = apvts.getRawParameterValue(prefix + "source"))
                route.sourceSlot = juce::jlimit(0, 3, juce::roundToInt(p->load()));

            if (auto* p = apvts.getRawParameterValue(prefix + "target"))
                route.targetSlot = juce::jlimit(0, 3, juce::roundToInt(p->load()));

            // Resolve engine names from slot indices
            route.sourceEngineName = getEngineIdForSlot(route.sourceSlot);
            route.targetEngineName = getEngineIdForSlot(route.targetSlot);
        }

        // Capture macro values
        static const char* macroIds[] = { "macro1", "macro2", "macro3", "macro4" };
        for (int i = 0; i < 4; ++i)
        {
            if (auto* p = apvts.getRawParameterValue(macroIds[i]))
                state.macroValues[static_cast<size_t>(i)] = juce::jlimit(0.0f, 1.0f, p->load());
        }

        return state;
    }

    //==========================================================================
    // LOAD — Restore a baked coupling state to the APVTS overlay.
    //==========================================================================

    // Write a BakedCouplingState into the cp_* APVTS params.
    // This restores the coupling overlay to a previously baked configuration.
    //
    // `slotRemap`: If true (default), attempts to remap engine names to
    // current slot indices. If the baked engine isn't loaded, the route is
    // deactivated rather than applied to the wrong engine.
    void loadBakedCoupling(const BakedCouplingState& state, bool slotRemap = true)
    {
        for (int r = 0; r < BakedCouplingState::kMaxRoutes; ++r)
        {
            juce::String prefix = "cp_r" + juce::String(r + 1) + "_";
            const auto& route = state.routes[static_cast<size_t>(r)];

            int sourceSlot = route.sourceSlot;
            int targetSlot = route.targetSlot;
            bool routeActive = route.active;

            // Slot remapping: find where the baked engines currently live
            if (slotRemap && route.active)
            {
                int remappedSource = findSlotForEngine(route.sourceEngineName);
                int remappedTarget = findSlotForEngine(route.targetEngineName);

                if (remappedSource >= 0 && remappedTarget >= 0)
                {
                    sourceSlot = remappedSource;
                    targetSlot = remappedTarget;
                }
                else
                {
                    // One or both engines not loaded — deactivate this route.
                    // The user can re-enable after loading the required engines.
                    routeActive = false;
                }
            }

            // Write to APVTS via setValueNotifyingHost for DAW automation support
            setParam(prefix + "active", routeActive ? 1.0f : 0.0f);
            setParam(prefix + "type",   static_cast<float>(route.couplingTypeIndex));
            setParam(prefix + "amount", route.amount);
            setParam(prefix + "source", static_cast<float>(sourceSlot));
            setParam(prefix + "target", static_cast<float>(targetSlot));
        }

        // Restore macros
        static const char* macroIds[] = { "macro1", "macro2", "macro3", "macro4" };
        for (int i = 0; i < 4; ++i)
            setParam(macroIds[i], state.macroValues[static_cast<size_t>(i)]);
    }

    //==========================================================================
    // BAKE INTO PRESET — merge overlay into the current .xometa preset.
    //==========================================================================

    // Convert the current performance overlay into CouplingPair structs
    // suitable for inclusion in a PresetData's couplingPairs vector.
    // This is the "bake into preset" action — it makes the overlay permanent.
    std::vector<CouplingPair> overlayToCouplingPairs() const
    {
        static const char* typeNames[] = {
            "AmpToFilter", "AmpToPitch", "LFOToPitch", "EnvToMorph",
            "AudioToFM", "AudioToRing", "FilterToFilter", "AmpToChoke",
            "RhythmToBlend", "EnvToDecay", "PitchToPitch", "AudioToWavetable",
            "AudioToBuffer", "KnotTopology", "TriangularCoupling"
        };

        std::vector<CouplingPair> pairs;

        for (int r = 0; r < BakedCouplingState::kMaxRoutes; ++r)
        {
            juce::String prefix = "cp_r" + juce::String(r + 1) + "_";

            auto* activeParam = apvts.getRawParameterValue(prefix + "active");
            if (!activeParam || activeParam->load() <= 0.5f)
                continue;

            auto* typeParam   = apvts.getRawParameterValue(prefix + "type");
            auto* amountParam = apvts.getRawParameterValue(prefix + "amount");
            auto* sourceParam = apvts.getRawParameterValue(prefix + "source");
            auto* targetParam = apvts.getRawParameterValue(prefix + "target");

            if (!typeParam || !amountParam || !sourceParam || !targetParam)
                continue;

            float amount = amountParam->load();
            if (std::abs(amount) < 0.001f)
                continue;

            int typeIdx   = juce::jlimit(0, static_cast<int>(CouplingType::TriangularCoupling),
                                          juce::roundToInt(typeParam->load()));
            int srcSlot   = juce::jlimit(0, 3, juce::roundToInt(sourceParam->load()));
            int tgtSlot   = juce::jlimit(0, 3, juce::roundToInt(targetParam->load()));

            juce::String srcEngine = getEngineIdForSlot(srcSlot);
            juce::String tgtEngine = getEngineIdForSlot(tgtSlot);

            if (srcEngine.isEmpty() || tgtEngine.isEmpty())
                continue;

            CouplingPair cp;
            cp.engineA = srcEngine;
            cp.engineB = tgtEngine;
            cp.type    = typeNames[typeIdx];
            cp.amount  = juce::jlimit(-1.0f, 1.0f, amount);
            pairs.push_back(cp);
        }

        return pairs;
    }

    //==========================================================================
    // File I/O — .xocoupling format
    //==========================================================================

    // Save a baked coupling state to a .xocoupling file.
    bool saveToFile(const juce::File& file, const BakedCouplingState& state)
    {
        auto json = juce::JSON::toString(state.toJSON(), false);
        if (json.isEmpty())
            return false;

        return file.replaceWithText(json);
    }

    // Load a baked coupling state from a .xocoupling file.
    // Returns a default (empty) state on any parse error.
    BakedCouplingState loadFromFile(const juce::File& file)
    {
        if (!file.existsAsFile())
            return {};

        if (file.getSize() > kMaxCouplingFileSize)
            return {};

        auto jsonString = file.loadFileAsString();
        if (jsonString.isEmpty())
            return {};

        auto parsed = juce::JSON::parse(jsonString);
        return BakedCouplingState::fromJSON(parsed);
    }

    //==========================================================================
    // Library management — scan, browse, select coupling presets
    //==========================================================================

    // Scan a directory (recursively) for .xocoupling files.
    void scanDirectory(const juce::File& directory)
    {
        if (!directory.isDirectory())
            return;

        library.clear();

        for (const auto& file :
             directory.findChildFiles(juce::File::findFiles, true, "*.xocoupling"))
        {
            if (file.getSize() > kMaxCouplingFileSize)
                continue;

            auto state = loadFromFile(file);
            if (state.name.isNotEmpty())
            {
                LibraryEntry entry;
                entry.state = std::move(state);
                entry.sourceFile = file;
                library.push_back(std::move(entry));
            }
        }

        // Sort alphabetically by name
        std::sort(library.begin(), library.end(),
                  [](const LibraryEntry& a, const LibraryEntry& b)
                  {
                      return a.state.name.compareIgnoreCase(b.state.name) < 0;
                  });

        notifyListeners();
    }

    // Return the number of coupling presets in the library.
    int getLibrarySize() const { return static_cast<int>(library.size()); }

    // Return the name of a coupling preset at the given index.
    juce::String getPresetName(int index) const
    {
        if (index >= 0 && index < static_cast<int>(library.size()))
            return library[static_cast<size_t>(index)].state.name;
        return {};
    }

    // Return the full BakedCouplingState at the given index.
    const BakedCouplingState* getPreset(int index) const
    {
        if (index >= 0 && index < static_cast<int>(library.size()))
            return &library[static_cast<size_t>(index)].state;
        return nullptr;
    }

    // Return the source file for a coupling preset at the given index.
    juce::File getPresetFile(int index) const
    {
        if (index >= 0 && index < static_cast<int>(library.size()))
            return library[static_cast<size_t>(index)].sourceFile;
        return {};
    }

    // Return all preset names as a StringArray (for UI dropdowns).
    juce::StringArray getPresetNames() const
    {
        juce::StringArray names;
        for (const auto& entry : library)
            names.add(entry.state.name);
        return names;
    }

    // Delete a coupling preset by index (removes file from disk too).
    bool deletePreset(int index)
    {
        if (index < 0 || index >= static_cast<int>(library.size()))
            return false;

        auto& entry = library[static_cast<size_t>(index)];
        if (entry.sourceFile.existsAsFile())
            entry.sourceFile.deleteFile();

        library.erase(library.begin() + index);
        notifyListeners();
        return true;
    }

    //==========================================================================
    // Default storage directory
    //==========================================================================

    // Get the default directory for coupling presets.
    // Creates it if it doesn't exist.
    static juce::File getDefaultDirectory()
    {
        auto appDir = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory)
            .getChildFile("XOceanus")
            .getChildFile("CouplingPresets");

        if (!appDir.isDirectory())
            appDir.createDirectory();

        return appDir;
    }

    //==========================================================================
    // Listener management
    //==========================================================================

    void addListener(Listener* l)
    {
        if (l != nullptr)
            listeners.push_back(l);
    }

    void removeListener(Listener* l)
    {
        listeners.erase(
            std::remove(listeners.begin(), listeners.end(), l),
            listeners.end());
    }

    //==========================================================================
    // Reset overlay — deactivate all performance routes
    //==========================================================================

    void clearOverlay()
    {
        for (int r = 0; r < BakedCouplingState::kMaxRoutes; ++r)
        {
            juce::String prefix = "cp_r" + juce::String(r + 1) + "_";
            setParam(prefix + "active", 0.0f);
            setParam(prefix + "amount", 0.0f);
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::function<juce::String(int)> getEngineIdForSlot;

    struct LibraryEntry
    {
        BakedCouplingState state;
        juce::File sourceFile;
    };

    std::vector<LibraryEntry> library;
    std::vector<Listener*> listeners;

    //--------------------------------------------------------------------------
    // Helper: write a value to an APVTS parameter by ID.
    void setParam(const juce::String& paramId, float rawValue)
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(paramId)))
            p->setValueNotifyingHost(p->convertTo0to1(rawValue));
    }

    //--------------------------------------------------------------------------
    // Helper: find which slot (0-3) currently holds an engine with the given ID.
    // Returns -1 if not found.
    int findSlotForEngine(const juce::String& engineId) const
    {
        if (engineId.isEmpty())
            return -1;

        juce::String canonical = resolveEngineAlias(engineId);
        for (int i = 0; i < MegaCouplingMatrix::MaxSlots; ++i)
        {
            juce::String slotEngine = getEngineIdForSlot(i);
            if (slotEngine.isNotEmpty()
                && resolveEngineAlias(slotEngine) == canonical)
                return i;
        }
        return -1;
    }

    //--------------------------------------------------------------------------
    void notifyListeners()
    {
        for (auto* l : listeners)
            if (l)
                l->couplingPresetLibraryChanged();
    }
};

} // namespace xoceanus
