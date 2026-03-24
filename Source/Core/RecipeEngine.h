#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <string>
#include <functional>
#include <optional>

namespace xolokun {

//==============================================================================
// RecipeEngine — One-touch configuration of the entire XOlokun architecture.
//
// A Recipe (.xorecipe) is a "Scene Architect" that sets up:
//   - Which engines load into the 4 slots
//   - Per-engine parameters (or reference to engine presets)
//   - Coupling matrix (types, routings, intensities)
//   - Master FX chain (all 18 stages)
//   - FX Sequencer configuration
//   - Macro assignments (CHARACTER/MOVEMENT/COUPLING/SPACE)
//   - A "variation axis" — one parameter users can sweep to explore
//
// Recipes are a superset of presets: a preset is a snapshot of state,
// a recipe is an *intent* — "massive evolving pad" or "gritty rhythmic bass".
//
// The format is designed for forward compatibility:
//   - Unknown engine IDs are ignored (future engines load as empty slots)
//   - Unknown FX stages are ignored (future effects skip gracefully)
//   - Unknown coupling types are ignored
//   - Version field enables migration
//   - Extensions array allows future systems (arpeggiator, MPE, etc.)
//
// Usage:
//   RecipeEngine recipes;
//   recipes.scanRecipeDirectory ("/path/to/Recipes");
//   auto recipe = recipes.loadRecipe ("Massive Evolving Pad.xorecipe");
//   recipes.applyRecipe (recipe, apvts, engineRegistry, couplingMatrix);
//==============================================================================
class RecipeEngine
{
public:
    //--------------------------------------------------------------------------
    // Data structures
    //--------------------------------------------------------------------------

    /// A single engine slot configuration
    struct EngineSlot
    {
        juce::String engineId;                        // e.g. "Odyssey", "Opal", "Obsidian"
        juce::String presetRef;                       // Optional: load a specific .xometa by name
        std::map<juce::String, float> parameters;     // Direct parameter overrides (if no presetRef)
    };

    /// Coupling route between two engine slots
    struct CouplingRoute
    {
        int sourceSlot = 0;                           // 0-3
        int destSlot = 0;                             // 0-3
        juce::String couplingType;                    // e.g. "FMCross", "RingMod", "WaveInject"
        float intensity = 0.0f;                       // 0.0-1.0
        std::map<juce::String, float> routeParams;    // Type-specific parameters
    };

    /// Macro assignment
    struct MacroAssignment
    {
        juce::String label;                           // Display label (e.g. "JOURNEY")
        std::vector<juce::String> targets;            // Parameter IDs this macro controls
        std::vector<float> ranges;                    // Amount per target (-1..+1)
    };

    /// Variation axis — the "one dial to explore" concept
    struct VariationAxis
    {
        juce::String label;                           // UI label (e.g. "Intensity", "Drift")
        std::vector<juce::String> targets;            // Params it sweeps
        std::vector<float> minValues;                 // Values at axis = 0
        std::vector<float> maxValues;                 // Values at axis = 1
    };

    /// Extension: future-proof container for systems not yet defined
    struct Extension
    {
        juce::String type;                            // e.g. "arpeggiator", "mpe_config"
        juce::var data;                               // Opaque JSON payload
    };

    /// Complete recipe
    struct Recipe
    {
        // Identity
        juce::String name;
        juce::String author;
        juce::String description;
        juce::String mood;                            // Foundation/Atmosphere/etc.
        juce::StringArray tags;
        int schemaVersion = 1;
        juce::String created;                         // ISO date

        // Engine configuration (1-4 slots)
        std::vector<EngineSlot> engines;

        // Coupling matrix
        std::vector<CouplingRoute> coupling;

        // Master FX settings (parameter ID → value)
        std::map<juce::String, float> masterFX;

        // Macros
        std::array<MacroAssignment, 4> macros;

        // Variation axis
        std::optional<VariationAxis> variationAxis;

        // Sonic DNA
        struct SonicDNA
        {
            float brightness = 0.5f;
            float warmth     = 0.5f;
            float movement   = 0.5f;
            float density    = 0.5f;
            float space      = 0.5f;
            float aggression = 0.5f;
        } dna;

        // Tempo hint (0 = freeform)
        float suggestedBPM = 0.0f;

        // Extensions (future-proof)
        std::vector<Extension> extensions;

        // Validity
        bool isValid() const { return name.isNotEmpty() && !engines.empty(); }
    };

    /// Catalog entry for browsing
    struct CatalogEntry
    {
        juce::String name;
        juce::String mood;
        juce::String author;
        juce::String description;
        juce::StringArray tags;
        juce::StringArray engineNames;
        juce::File filePath;
        Recipe::SonicDNA dna;
    };

    //--------------------------------------------------------------------------
    // API
    //--------------------------------------------------------------------------

    RecipeEngine() = default;

    /// Scan a directory (recursively) for .xorecipe files and build catalog
    void scanRecipeDirectory (const juce::File& directory)
    {
        catalog.clear();

        auto files = directory.findChildFiles (
            juce::File::findFiles, true, "*.xorecipe");

        for (const auto& file : files)
        {
            auto entry = parseCatalogEntry (file);
            if (entry.name.isNotEmpty())
                catalog.push_back (std::move (entry));
        }

        // Sort by mood, then name
        std::sort (catalog.begin(), catalog.end(),
            [] (const CatalogEntry& a, const CatalogEntry& b)
            {
                if (a.mood != b.mood) return a.mood < b.mood;
                return a.name < b.name;
            });
    }

    /// Get the full catalog for browsing
    const std::vector<CatalogEntry>& getCatalog() const { return catalog; }

    /// Filter catalog by mood
    std::vector<CatalogEntry> getByMood (const juce::String& mood) const
    {
        std::vector<CatalogEntry> result;
        for (const auto& e : catalog)
            if (e.mood == mood)
                result.push_back (e);
        return result;
    }

    /// Filter catalog by tag
    std::vector<CatalogEntry> getByTag (const juce::String& tag) const
    {
        std::vector<CatalogEntry> result;
        for (const auto& e : catalog)
            if (e.tags.contains (tag))
                result.push_back (e);
        return result;
    }

    /// Filter catalog by engine name
    std::vector<CatalogEntry> getByEngine (const juce::String& engineId) const
    {
        std::vector<CatalogEntry> result;
        for (const auto& e : catalog)
            if (e.engineNames.contains (engineId))
                result.push_back (e);
        return result;
    }

    //--------------------------------------------------------------------------
    // Loading
    //--------------------------------------------------------------------------

    /// Load a full recipe from file
    Recipe loadRecipe (const juce::File& file) const
    {
        Recipe recipe;

        auto json = juce::JSON::parse (file);
        if (!json.isObject())
            return recipe;

        auto* obj = json.getDynamicObject();
        if (!obj) return recipe;

        // Identity
        recipe.schemaVersion = static_cast<int> (obj->getProperty ("schema_version"));
        recipe.name          = obj->getProperty ("name").toString();
        recipe.author        = obj->getProperty ("author").toString();
        recipe.description   = obj->getProperty ("description").toString();
        recipe.mood          = obj->getProperty ("mood").toString();
        recipe.created       = obj->getProperty ("created").toString();
        recipe.suggestedBPM  = static_cast<float> (obj->getProperty ("suggestedBPM"));

        // Tags
        if (auto* tagsArray = obj->getProperty ("tags").getArray())
            for (const auto& t : *tagsArray)
                recipe.tags.add (t.toString());

        // Engines
        if (auto* enginesArray = obj->getProperty ("engines").getArray())
        {
            for (const auto& eng : *enginesArray)
            {
                if (auto* engObj = eng.getDynamicObject())
                {
                    EngineSlot slot;
                    slot.engineId  = engObj->getProperty ("id").toString();
                    slot.presetRef = engObj->getProperty ("presetRef").toString();

                    if (auto* paramsObj = engObj->getProperty ("parameters").getDynamicObject())
                        for (const auto& prop : paramsObj->getProperties())
                            slot.parameters[prop.name.toString()] = static_cast<float> (prop.value);

                    recipe.engines.push_back (std::move (slot));
                }
            }
        }

        // Coupling
        if (auto* couplingArray = obj->getProperty ("coupling").getArray())
        {
            for (const auto& c : *couplingArray)
            {
                if (auto* cObj = c.getDynamicObject())
                {
                    CouplingRoute route;
                    route.sourceSlot   = static_cast<int> (cObj->getProperty ("sourceSlot"));
                    route.destSlot     = static_cast<int> (cObj->getProperty ("destSlot"));
                    route.couplingType = cObj->getProperty ("type").toString();
                    route.intensity    = static_cast<float> (cObj->getProperty ("intensity"));

                    if (auto* rp = cObj->getProperty ("params").getDynamicObject())
                        for (const auto& prop : rp->getProperties())
                            route.routeParams[prop.name.toString()] = static_cast<float> (prop.value);

                    recipe.coupling.push_back (std::move (route));
                }
            }
        }

        // Master FX
        if (auto* fxObj = obj->getProperty ("masterFX").getDynamicObject())
            for (const auto& prop : fxObj->getProperties())
                recipe.masterFX[prop.name.toString()] = static_cast<float> (prop.value);

        // Macros
        if (auto* macrosArray = obj->getProperty ("macros").getArray())
        {
            for (int i = 0; i < juce::jmin (4, macrosArray->size()); ++i)
            {
                if (auto* mObj = (*macrosArray)[i].getDynamicObject())
                {
                    recipe.macros[static_cast<size_t> (i)].label = mObj->getProperty ("label").toString();

                    if (auto* targets = mObj->getProperty ("targets").getArray())
                        for (const auto& t : *targets)
                            recipe.macros[static_cast<size_t> (i)].targets.push_back (t.toString());

                    if (auto* ranges = mObj->getProperty ("ranges").getArray())
                        for (const auto& r : *ranges)
                            recipe.macros[static_cast<size_t> (i)].ranges.push_back (static_cast<float> (r));
                }
            }
        }

        // Variation axis
        if (auto* vaObj = obj->getProperty ("variationAxis").getDynamicObject())
        {
            VariationAxis va;
            va.label = vaObj->getProperty ("label").toString();

            if (auto* targets = vaObj->getProperty ("targets").getArray())
                for (const auto& t : *targets) va.targets.push_back (t.toString());
            if (auto* mins = vaObj->getProperty ("minValues").getArray())
                for (const auto& v : *mins) va.minValues.push_back (static_cast<float> (v));
            if (auto* maxs = vaObj->getProperty ("maxValues").getArray())
                for (const auto& v : *maxs) va.maxValues.push_back (static_cast<float> (v));

            recipe.variationAxis = std::move (va);
        }

        // DNA
        if (auto* dnaObj = obj->getProperty ("dna").getDynamicObject())
        {
            recipe.dna.brightness  = static_cast<float> (dnaObj->getProperty ("brightness"));
            recipe.dna.warmth      = static_cast<float> (dnaObj->getProperty ("warmth"));
            recipe.dna.movement    = static_cast<float> (dnaObj->getProperty ("movement"));
            recipe.dna.density     = static_cast<float> (dnaObj->getProperty ("density"));
            recipe.dna.space       = static_cast<float> (dnaObj->getProperty ("space"));
            recipe.dna.aggression  = static_cast<float> (dnaObj->getProperty ("aggression"));
        }

        // Extensions (forward-compatible: store as opaque data)
        if (auto* extArray = obj->getProperty ("extensions").getArray())
        {
            for (const auto& ext : *extArray)
            {
                if (auto* extObj = ext.getDynamicObject())
                {
                    Extension e;
                    e.type = extObj->getProperty ("type").toString();
                    e.data = extObj->getProperty ("data");
                    recipe.extensions.push_back (std::move (e));
                }
            }
        }

        return recipe;
    }

    //--------------------------------------------------------------------------
    // Application — applies recipe to live state
    //--------------------------------------------------------------------------

    /// Callback types for integration with EngineRegistry and CouplingMatrix
    using LoadEngineCallback = std::function<bool (int slot, const juce::String& engineId)>;
    using LoadEnginePresetCallback = std::function<bool (int slot, const juce::String& presetName)>;
    using SetCouplingCallback = std::function<void (int src, int dst,
                                                     const juce::String& type, float intensity,
                                                     const std::map<juce::String, float>& params)>;
    using SetMacroCallback = std::function<void (int macroIndex, const juce::String& label,
                                                  const std::vector<juce::String>& targets,
                                                  const std::vector<float>& ranges)>;

    struct ApplyCallbacks
    {
        LoadEngineCallback loadEngine;
        LoadEnginePresetCallback loadEnginePreset;
        SetCouplingCallback setCoupling;
        SetMacroCallback setMacro;
    };

    /// Apply a recipe to the live synth state.
    /// Returns true if all engines were loaded successfully.
    bool applyRecipe (const Recipe& recipe,
                      juce::AudioProcessorValueTreeState& apvts,
                      const ApplyCallbacks& callbacks) const
    {
        if (!recipe.isValid())
            return false;

        bool allEnginesLoaded = true;

        // 1. Load engines into slots
        for (int i = 0; i < static_cast<int> (recipe.engines.size()) && i < 4; ++i)
        {
            const auto& slot = recipe.engines[static_cast<size_t> (i)];

            if (callbacks.loadEngine)
            {
                if (!callbacks.loadEngine (i, slot.engineId))
                    allEnginesLoaded = false;
            }

            // Apply engine-specific preset or parameters
            if (slot.presetRef.isNotEmpty() && callbacks.loadEnginePreset)
            {
                callbacks.loadEnginePreset (i, slot.presetRef);
            }
            else
            {
                // Direct parameter overrides
                for (const auto& [paramId, value] : slot.parameters)
                {
                    if (auto* param = apvts.getRawParameterValue (paramId))
                        param->store (value);
                }
            }
        }

        // 2. Configure coupling matrix
        if (callbacks.setCoupling)
        {
            for (const auto& route : recipe.coupling)
            {
                callbacks.setCoupling (route.sourceSlot, route.destSlot,
                                       route.couplingType, route.intensity,
                                       route.routeParams);
            }
        }

        // 3. Set master FX parameters
        for (const auto& [paramId, value] : recipe.masterFX)
        {
            if (auto* param = apvts.getRawParameterValue (paramId))
                param->store (value);
        }

        // 4. Configure macros
        if (callbacks.setMacro)
        {
            for (int i = 0; i < 4; ++i)
            {
                const auto& macro = recipe.macros[static_cast<size_t> (i)];
                if (macro.label.isNotEmpty())
                    callbacks.setMacro (i, macro.label, macro.targets, macro.ranges);
            }
        }

        return allEnginesLoaded;
    }

    //--------------------------------------------------------------------------
    // Saving
    //--------------------------------------------------------------------------

    /// Save a recipe to a .xorecipe file
    static bool saveRecipe (const Recipe& recipe, const juce::File& file)
    {
        auto* root = new juce::DynamicObject();

        root->setProperty ("schema_version", recipe.schemaVersion);
        root->setProperty ("name", recipe.name);
        root->setProperty ("author", recipe.author);
        root->setProperty ("description", recipe.description);
        root->setProperty ("mood", recipe.mood);
        root->setProperty ("created", recipe.created);
        root->setProperty ("suggestedBPM", recipe.suggestedBPM);

        // Tags
        juce::Array<juce::var> tags;
        for (const auto& t : recipe.tags) tags.add (t);
        root->setProperty ("tags", tags);

        // Engines
        juce::Array<juce::var> engines;
        for (const auto& slot : recipe.engines)
        {
            auto* eng = new juce::DynamicObject();
            eng->setProperty ("id", slot.engineId);
            if (slot.presetRef.isNotEmpty())
                eng->setProperty ("presetRef", slot.presetRef);

            if (!slot.parameters.empty())
            {
                auto* params = new juce::DynamicObject();
                for (const auto& [k, v] : slot.parameters)
                    params->setProperty (k, v);
                eng->setProperty ("parameters", juce::var (params));
            }

            engines.add (juce::var (eng));
        }
        root->setProperty ("engines", engines);

        // Coupling
        juce::Array<juce::var> coupling;
        for (const auto& route : recipe.coupling)
        {
            auto* c = new juce::DynamicObject();
            c->setProperty ("sourceSlot", route.sourceSlot);
            c->setProperty ("destSlot", route.destSlot);
            c->setProperty ("type", route.couplingType);
            c->setProperty ("intensity", route.intensity);

            if (!route.routeParams.empty())
            {
                auto* rp = new juce::DynamicObject();
                for (const auto& [k, v] : route.routeParams)
                    rp->setProperty (k, v);
                c->setProperty ("params", juce::var (rp));
            }

            coupling.add (juce::var (c));
        }
        root->setProperty ("coupling", coupling);

        // Master FX
        auto* fx = new juce::DynamicObject();
        for (const auto& [k, v] : recipe.masterFX)
            fx->setProperty (k, v);
        root->setProperty ("masterFX", juce::var (fx));

        // Macros
        juce::Array<juce::var> macros;
        for (const auto& macro : recipe.macros)
        {
            auto* m = new juce::DynamicObject();
            m->setProperty ("label", macro.label);
            juce::Array<juce::var> targets, ranges;
            for (const auto& t : macro.targets) targets.add (t);
            for (const auto& r : macro.ranges)  ranges.add (r);
            m->setProperty ("targets", targets);
            m->setProperty ("ranges", ranges);
            macros.add (juce::var (m));
        }
        root->setProperty ("macros", macros);

        // Variation axis
        if (recipe.variationAxis.has_value())
        {
            auto* va = new juce::DynamicObject();
            va->setProperty ("label", recipe.variationAxis->label);
            juce::Array<juce::var> targets, mins, maxs;
            for (const auto& t : recipe.variationAxis->targets)   targets.add (t);
            for (const auto& v : recipe.variationAxis->minValues)  mins.add (v);
            for (const auto& v : recipe.variationAxis->maxValues)  maxs.add (v);
            va->setProperty ("targets", targets);
            va->setProperty ("minValues", mins);
            va->setProperty ("maxValues", maxs);
            root->setProperty ("variationAxis", juce::var (va));
        }

        // DNA
        auto* dna = new juce::DynamicObject();
        dna->setProperty ("brightness", recipe.dna.brightness);
        dna->setProperty ("warmth",     recipe.dna.warmth);
        dna->setProperty ("movement",   recipe.dna.movement);
        dna->setProperty ("density",    recipe.dna.density);
        dna->setProperty ("space",      recipe.dna.space);
        dna->setProperty ("aggression", recipe.dna.aggression);
        root->setProperty ("dna", juce::var (dna));

        // Extensions
        if (!recipe.extensions.empty())
        {
            juce::Array<juce::var> exts;
            for (const auto& ext : recipe.extensions)
            {
                auto* e = new juce::DynamicObject();
                e->setProperty ("type", ext.type);
                e->setProperty ("data", ext.data);
                exts.add (juce::var (e));
            }
            root->setProperty ("extensions", exts);
        }

        auto jsonStr = juce::JSON::toString (juce::var (root), true);
        return file.replaceWithText (jsonStr);
    }

private:
    std::vector<CatalogEntry> catalog;

    CatalogEntry parseCatalogEntry (const juce::File& file) const
    {
        CatalogEntry entry;
        entry.filePath = file;

        auto json = juce::JSON::parse (file);
        if (!json.isObject()) return entry;

        auto* obj = json.getDynamicObject();
        if (!obj) return entry;

        entry.name        = obj->getProperty ("name").toString();
        entry.mood        = obj->getProperty ("mood").toString();
        entry.author      = obj->getProperty ("author").toString();
        entry.description = obj->getProperty ("description").toString();

        if (auto* tags = obj->getProperty ("tags").getArray())
            for (const auto& t : *tags) entry.tags.add (t.toString());

        if (auto* engines = obj->getProperty ("engines").getArray())
            for (const auto& e : *engines)
                if (auto* eObj = e.getDynamicObject())
                    entry.engineNames.add (eObj->getProperty ("id").toString());

        if (auto* dna = obj->getProperty ("dna").getDynamicObject())
        {
            entry.dna.brightness  = static_cast<float> (dna->getProperty ("brightness"));
            entry.dna.warmth      = static_cast<float> (dna->getProperty ("warmth"));
            entry.dna.movement    = static_cast<float> (dna->getProperty ("movement"));
            entry.dna.density     = static_cast<float> (dna->getProperty ("density"));
            entry.dna.space       = static_cast<float> (dna->getProperty ("space"));
            entry.dna.aggression  = static_cast<float> (dna->getProperty ("aggression"));
        }

        return entry;
    }
};

} // namespace xolokun
