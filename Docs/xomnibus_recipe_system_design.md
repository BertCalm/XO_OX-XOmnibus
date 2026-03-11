# XOmnibus — Recipe System Design

**Status:** Design exploration
**Date:** 2026-03-10
**Author:** XO_OX Designs

---

## 1. Concept

A **Recipe** is a curated multi-engine configuration: which engines load into which slots, how they couple, and what the macros control. It is not a preset. It is a *starting point* — a wired-up instrument configuration that a user (or a preset designer) builds sounds on top of.

Think of it this way:
- A **Module** is a single instrument (SNAP, MORPH, DUB, DRIFT, BOB, FAT, ONSET, OVERWORLD, OPAL, ORGANON, OUROBOROS).
- A **Recipe** is a combination of modules with coupling routes already patched — a playable multi-engine instrument before any parameters are dialed in.
- A **Preset** is a complete snapshot of everything: engine parameters, coupling amounts, macro mappings, DNA. A preset always *implies* a recipe (it contains the engine list and coupling data), but a recipe exists independently of any preset.

The relationship: **Recipe configures the rig. Preset dials in the sound.**

Recipes solve a real problem. XOmnibus has 11 engines and 12 coupling types. The combinatorial space is vast. Most users will never explore BOB + OUROBOROS with Audio->Ring coupling on their own. Recipes collapse that space into named, curated starting points that say: "try this combination — we already wired the interesting connections."

---

## 2. Naming Taxonomy

### Backend Code

| Concept | Class/Type Name | File |
|---------|----------------|------|
| Individual engine | `SynthEngine` | `SynthEngine.h` |
| Engine type registry | `EngineRegistry` | `EngineRegistry.h` |
| Multi-engine configuration template | `Recipe` / `RecipeData` | `RecipeManager.h` (new) |
| Recipe library + browser | `RecipeManager` | `RecipeManager.h` (new) |
| Complete sound snapshot | `PresetData` | `PresetManager.h` |

The word "Module" replaces "Engine" in all user-facing UI strings. Users see "Modules" in the slot selector, not "Engines." Backend code keeps `SynthEngine` — no rename needed internally.

### UI Naming

| Internal Term | User-Facing Term | Where It Appears |
|--------------|-----------------|-----------------|
| Engine / SynthEngine | **Module** | Slot selector dropdown, module browser |
| Recipe / RecipeData | **Recipe** | Recipe browser, preset browser filter |
| Preset / PresetData | **Preset** | Preset browser, save dialog |

### Recipe Naming Convention

Modules follow the **XO + O-word** pattern (OddfeliX/OddOscar, XObese, etc.). Recipes should NOT follow this pattern — they are not instruments, they are configurations. Recipe names should be:

- **Two words, evocative, lowercase-friendly** — matching the preset naming spirit
- **Descriptive of sonic result**, not engine contents
- Examples: "Bass Station", "Chaos Feeder", "Ghost Orchestra", "Dub Laboratory"
- Max 25 characters
- No engine names in the recipe name (the UI shows which modules are loaded)

---

## 3. Data Format

### `.xorecipe` JSON Schema

Recipes get their own file format (`.xorecipe`) rather than extending `.xometa`. Rationale: recipes are templates, not complete sounds. Mixing the two concepts in one format creates ambiguity about which fields are required. A recipe intentionally omits per-engine parameter values — that is the preset's job.

```json
{
  "schema_version": 1,
  "name": "Bass Station",
  "description": "Warm analog bass with dub echo processing. BOB provides the fundamental, DUB adds space.",
  "author": "XO_OX Designs",
  "tags": ["bass", "dub", "warm", "analog"],
  "category": "Bass & Keys",

  "slots": [
    { "slot": 0, "engine": "Bob" },
    { "slot": 1, "engine": "Dub" }
  ],

  "coupling": {
    "routingMode": "Independent",
    "pairs": [
      {
        "engineA": "Bob",
        "engineB": "Dub",
        "type": "Amp->Filter",
        "amount": 0.35
      },
      {
        "engineA": "Dub",
        "engineB": "Bob",
        "type": "Filter->Filter",
        "amount": 0.2
      }
    ]
  },

  "macroLabels": ["WARMTH", "ECHO DEPTH", "COUPLING", "SPACE"],

  "macroMappings": [
    { "macro": 0, "target": "bob_fltCutoff", "range": [800, 8000] },
    { "macro": 0, "target": "bob_fltChar", "range": [0.3, 0.9] },
    { "macro": 1, "target": "dub_sendAmount", "range": [0.0, 0.8] },
    { "macro": 2, "target": "_coupling_amount_0", "range": [0.0, 1.0] },
    { "macro": 3, "target": "master_reverbMix", "range": [0.0, 0.6] }
  ],

  "defaultDNA": {
    "brightness": 0.3,
    "warmth": 0.8,
    "movement": 0.4,
    "density": 0.5,
    "space": 0.5,
    "aggression": 0.2
  }
}
```

### Schema Fields

| Field | Required | Description |
|-------|----------|-------------|
| `schema_version` | Yes | Always `1` for forward compat |
| `name` | Yes | Display name, max 25 chars, unique |
| `description` | Yes | 1-2 sentences, max 120 chars |
| `author` | Yes | "XO_OX Designs" for factory recipes |
| `tags` | Yes | Searchable keywords, min 3 |
| `category` | Yes | Browsing category (see below) |
| `slots` | Yes | Array of `{ slot, engine }` objects. 2-4 entries. |
| `coupling.routingMode` | Yes | "Independent", "Shared", or "Chain" |
| `coupling.pairs` | Yes | Array of coupling routes (can be empty for uncoupled recipes) |
| `macroLabels` | Yes | 4 custom macro display names |
| `macroMappings` | No | Default macro-to-parameter wiring for this recipe |
| `defaultDNA` | No | Approximate sonic center for this configuration |

### Recipe Categories

| Category | Description |
|----------|-------------|
| Bass & Keys | Melodic fundamentals — basses, leads, keys |
| Pads & Textures | Evolving, sustained, atmospheric |
| Percussion | Drum kits, percussive layers, rhythmic |
| Experimental | Chaos, noise, feedback, generative |
| Cinematic | Scoring, tension, impact, transition |
| Production | Mix-ready combos — sidechain, ducking, layered |

---

## 4. Example Recipes

### 4.1 Bass Station
- **Slots:** BOB (1) + DUB (2)
- **Coupling:** BOB Amp->Filter DUB (0.35), DUB Filter->Filter BOB (0.2)
- **Routing:** Independent
- **Macros:** WARMTH, ECHO DEPTH, COUPLING, SPACE
- **Idea:** BOB's warm oscillators through DUB's echo/delay processing. The coupling makes the bass pump through the delay — louder notes push the delay filter open.

### 4.2 Chaos Feeder
- **Slots:** OUROBOROS (1) + ORGANON (2) + ONSET (3)
- **Coupling:** OUROBOROS Audio->FM ORGANON (0.6), ORGANON Env->Decay ONSET (0.4), ONSET Rhythm->Blend OUROBOROS (0.3)
- **Routing:** Chain
- **Macros:** RECURSION, GROWTH, RHYTHM, VOID
- **Idea:** A three-way feedback loop. OUROBOROS feeds its self-modulating audio into ORGANON's living oscillators. ORGANON's envelopes shape ONSET's decay. ONSET's rhythmic patterns modulate OUROBOROS's blend. Turn up all macros and hold on.

### 4.3 Ghost Orchestra
- **Slots:** MORPH (1) + DRIFT (2) + OPAL (3)
- **Coupling:** MORPH Env->Morph DRIFT (0.3), DRIFT LFO->Pitch OPAL (0.15), OPAL Filter->Filter MORPH (0.2)
- **Routing:** Shared
- **Macros:** SHIMMER, ALIEN, CRYSTAL, SPACE
- **Idea:** Three pad engines creating shifting harmonic landscapes. MORPH's wavetable envelopes morph DRIFT's position. DRIFT's slow LFO detunes OPAL. OPAL's filter feeds back into MORPH. The result is a self-evolving orchestral texture.

### 4.4 Industrial Press
- **Slots:** FAT (1) + ONSET (2) + SNAP (3)
- **Coupling:** FAT Audio->Ring ONSET (0.5), ONSET Amp->Choke SNAP (0.7), SNAP Amp->Filter FAT (0.4)
- **Routing:** Independent
- **Macros:** WEIGHT, IMPACT, CHOKE, GRIT
- **Idea:** FAT's heavy samples ring-modulate ONSET's percussion. ONSET's hits choke SNAP. SNAP's amplitude drives FAT's filter. Three engines fighting for space — the result sounds like a factory floor.

### 4.5 Liquid Keys
- **Slots:** BOB (1) + MORPH (2)
- **Coupling:** BOB LFO->Pitch MORPH (0.1), MORPH Env->Morph BOB (0.25)
- **Routing:** Independent
- **Macros:** CHARACTER, MOVEMENT, COUPLING, SPACE
- **Idea:** BOB's warm character with MORPH's wavetable shimmer. BOB's curiosity LFO gently detunes MORPH. MORPH's envelope morphs BOB's texture. Playable, expressive, usable on every track.

### 4.6 Dub Laboratory
- **Slots:** DUB (1) + SNAP (2) + DRIFT (3)
- **Coupling:** SNAP Amp->Filter DUB (0.5), DUB Audio->FM DRIFT (0.15), DRIFT Filter->Filter DUB (0.3)
- **Routing:** Chain
- **Macros:** HITS, ECHO, ALIEN, SPACE
- **Idea:** SNAP fires percussive hits into DUB's mixing desk. DUB's echoes FM-modulate DRIFT's pads. DRIFT's filter feeds back into DUB. Classic dub production topology with a psychedelic twist.

---

## 5. UI Integration

### Recipe Browser

Recipes appear in the **preset browser** as a top-level tab, not a separate window. The browser gains two tabs:

```
[ Presets ]  [ Recipes ]
```

The **Recipes tab** shows:
- Category filter sidebar (Bass & Keys, Pads & Textures, etc.)
- Recipe cards in a grid, each showing:
  - Recipe name (bold, Space Grotesk)
  - Module badges (colored dots using each engine's accent color)
  - Coupling indicator (line connecting the dots, gold if coupled)
  - Description text (Inter, secondary color)
  - Tag pills below

### Loading a Recipe

When a user selects a recipe:
1. The slot configuration loads (engines swap with 50ms crossfade)
2. Coupling routes are applied to the MegaCouplingMatrix
3. Macro labels update
4. All engine parameters reset to **init state** (not to a specific preset)
5. The preset browser automatically filters to show only presets compatible with this recipe (matching engine combination)
6. The UI shows a "Recipe: Bass Station" badge in the header strip

### Preset-Recipe Relationship in the Browser

When browsing presets, a new filter option appears:

```
Filter by: [ All Moods v ]  [ All Modules v ]  [ All Recipes v ]
```

The "All Recipes" filter shows presets grouped by their implied recipe — presets that use the same engine combination and similar coupling topology. This is a computed grouping, not a stored field. A preset does not need to know which recipe it belongs to.

### Recipe Badge

When a recipe is active, a small badge appears in the XOmnibus header bar:

```
RECIPE: Bass Station  [x]
```

Clicking `[x]` clears the recipe context but keeps the current engine/coupling state. This lets advanced users start from a recipe and then diverge.

---

## 6. Relationship to Presets

| | Recipe | Preset |
|---|--------|--------|
| **Contains engines** | Yes (which modules, which slots) | Yes (which modules) |
| **Contains coupling routes** | Yes (type + default amounts) | Yes (type + exact amounts) |
| **Contains engine parameters** | No | Yes (full parameter snapshot) |
| **Contains macro labels** | Yes | Yes |
| **Contains macro mappings** | Yes (default wiring) | Implicit (via parameter values) |
| **Contains DNA** | Optional default center | Yes (exact fingerprint) |
| **File format** | `.xorecipe` | `.xometa` |
| **User-editable** | No (factory only, v1) | Yes (save/load) |
| **Purpose** | Template — "wire up this rig" | Snapshot — "recall this exact sound" |

### Workflow

1. User browses recipes, picks "Bass Station"
2. BOB loads in slot 1, DUB loads in slot 2, coupling routes engage
3. User hears init-state BOB + DUB with coupling active
4. User browses presets filtered to BOB+DUB combinations
5. User loads "Submarine Echo" preset — parameters fill in, coupling amounts adjust
6. User tweaks, saves as new preset — the `.xometa` captures everything
7. The saved preset stands alone. It does not reference the recipe.

### Presets Are Self-Contained

A preset saved from a recipe context contains all the information needed to recreate the sound without the recipe. The recipe is a discovery and configuration tool, not a runtime dependency. This preserves the existing `.xometa` contract: every preset is a complete, portable snapshot.

---

## 7. Implementation Sketch

### New Files

| File | Purpose |
|------|---------|
| `Source/Core/RecipeManager.h` | `RecipeData` struct + `RecipeManager` class (load, browse, apply) |
| `Source/UI/RecipeBrowser.h` | Recipe browser tab component |
| `Recipes/` | Factory `.xorecipe` files, organized by category |
| `Docs/xorecipe_schema.json` | JSON Schema for `.xorecipe` format |

### RecipeData Struct

```cpp
namespace xomnibus {

struct RecipeSlot {
    int slotIndex;           // 0-3
    juce::String engineId;   // e.g. "Bob", "Dub"
};

struct RecipeMacroMapping {
    int macroIndex;          // 0-3
    juce::String targetParam;
    float rangeMin;
    float rangeMax;
};

struct RecipeData {
    juce::String name;
    juce::String description;
    juce::String author;
    juce::String category;
    juce::StringArray tags;

    std::vector<RecipeSlot> slots;

    juce::String routingMode;  // "Independent", "Shared", "Chain"
    std::vector<CouplingPair> couplingPairs;

    juce::StringArray macroLabels;  // 4 entries
    std::vector<RecipeMacroMapping> macroMappings;

    PresetDNA defaultDNA;

    juce::File sourceFile;
};

} // namespace xomnibus
```

### RecipeManager Class

```cpp
class RecipeManager {
public:
    // Scan Recipes/ directory for .xorecipe files
    void scanRecipeDirectory(const juce::File& directory);

    // Get all recipes, optionally filtered by category
    std::vector<RecipeData> getRecipes(const juce::String& category = {}) const;

    // Apply a recipe to the engine registry and coupling matrix.
    // Called on the message thread. Engine swaps use 50ms crossfade.
    void applyRecipe(const RecipeData& recipe,
                     EngineRegistry& registry,
                     MegaCouplingMatrix& matrix);

    // Check if a preset matches a recipe's engine/coupling signature
    bool presetMatchesRecipe(const PresetData& preset,
                             const RecipeData& recipe) const;

    // Get all recipes that match a preset's engine combination
    std::vector<RecipeData> findRecipesForPreset(const PresetData& preset) const;

private:
    std::vector<RecipeData> library;
    bool parseRecipeJSON(const juce::String& json, RecipeData& out);
};
```

### Integration Points

1. **EngineRegistry** — No changes needed. `RecipeManager::applyRecipe()` calls `createEngine()` and manages slot assignment externally, same as the existing preset loading path.

2. **MegaCouplingMatrix** — No changes needed. `applyRecipe()` calls `clearRoutes()` then `addRoute()` for each recipe coupling pair.

3. **PresetManager** — Add a method `getPresetsForEngines(StringArray engines)` that returns presets using the same engine combination. This enables the "filter presets by active recipe" feature.

4. **XOmnibusProcessor** — Add `RecipeManager` as a member. Wire `applyRecipe()` to the message thread handler that currently manages engine slot changes.

5. **UI** — Add `RecipeBrowser` as a tab in the existing preset browser component. The recipe browser uses the same grid layout and styling as the preset browser but with recipe-specific card rendering (module badges, coupling visualization).

### Migration / Compatibility

- Recipes are additive. They do not change the preset format.
- Existing presets continue to work exactly as before.
- Users who never touch recipes see no difference — the Presets tab is selected by default.
- Factory recipes ship in `Recipes/{category}/` alongside the existing `Presets/` directory.

### Phase Plan

| Phase | Scope |
|-------|-------|
| **1 — Format + Data** | Define `.xorecipe` schema, write 6 factory recipes, implement `RecipeManager` parse/load |
| **2 — Apply** | Wire `applyRecipe()` to engine slots and coupling matrix, preset filtering by engine combo |
| **3 — UI** | Recipe browser tab, recipe cards, module badges, active recipe badge |
| **4 — Macro Mappings** | Implement `macroMappings` in recipes so macros auto-wire on recipe load |
| **5 — Community** | User-created recipes (save current config as recipe), recipe sharing |

---

## Resolved Design Decisions

### 1. Presets store a soft recipe reference — DECIDED: Yes

Presets include an optional `"recipe": "Bass Station"` string in `.xometa`. This is a **soft reference** — a hint, not a dependency. If the recipe doesn't exist, the preset loads perfectly (it remains self-contained). When present, it enables instant "show me all presets for this recipe" filtering without fuzzy engine-matching heuristics.

**Change to `.xometa` schema:** Add optional `"recipe"` field (string, max 25 chars).

### 2. Recipe versioning — DECIDED: Skip for v1

Recipes ship as stable artifacts in the initial release. No version field, no migration machinery. Recipes only change when XO_OX Designs updates them. By the time recipes might need evolution (year 2, new modules), real user feedback will inform whether versioning is needed. If so, a `schema_version` bump and migration note can be added then.

### 3. Recipes are strictly multi-engine (2-4 slots) — DECIDED: Yes

A single ingredient isn't a recipe. The recipe metaphor implies combination. Single-engine quick-start is a separate mechanism (e.g., per-engine init presets or a "Module Starter" feature) — not a recipe.

### 4. Preset lineage tracking — DECIDED: Lightweight parent reference

Factory presets are immutable — only XO_OX Designs edits them. When a user saves a variant:
- The `.xometa` stores `"parentPreset": "Submarine Echo"` (the original factory preset name)
- The save dialog suggests a name like `"Submarine Echo — [user suffix]"` with an editable suffix
- The browser can show "variants of this preset" by querying the `parentPreset` field
- Lineage stays **flat**: variants of variants still point to the original factory preset, not intermediate saves. Simple lineage, not a version graph.

**Change to `.xometa` schema:** Add optional `"parentPreset"` field (string).

### 5. Community preset ecosystem — DECIDED: Plan for it

The architecture supports three preset tiers:

| Tier | Path | Source | Editable |
|------|------|--------|----------|
| Factory | `Presets/XOmnibus/{mood}/` | Ships with app | No |
| User | `~/XOmnibus/Presets/User/` | Saved by user | Yes |
| Community | `~/XOmnibus/Presets/Community/` | Downloaded/shared | No (read-only locally) |

The sharing mechanism (post-v1): a curated gallery on the XO_OX website where users upload `.xometa` files, XO_OX reviews and features the best, and they become available in-app. This parallels the XPN export pipeline — presets are the next shareable artifact after sound packs.

Community recipes follow the same pattern: `~/XOmnibus/Recipes/User/` and `~/XOmnibus/Recipes/Community/`.

---

## Future Considerations

- **Module Starter feature:** A separate mechanism for single-engine quick-start (not recipes). Could be per-engine init presets with curated macro mappings. Design TBD.
- **Preset naming generator:** The variant naming convention (`"Original — suffix"`) could be enhanced with a smart suffix generator that suggests descriptive terms based on what parameters changed from the parent.
- **Community curation pipeline:** Voting, featuring, and quality review process for community presets. Could integrate with the blog and social presence for community building.
- **Adopted presets:** Top community presets could be "adopted" into the factory library in future releases, with credit to the creator.
