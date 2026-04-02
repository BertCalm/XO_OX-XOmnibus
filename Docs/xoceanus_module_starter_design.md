# XOlokun — Module Starter Design

**Status:** Design
**Date:** 2026-03-10
**Author:** XO_OX Designs

---

## Problem

When a user loads a single engine into a slot, they get… silence. They need to find a preset or start tweaking parameters from scratch. Recipes solve this for multi-engine configurations (2-4 slots), but a single ingredient isn't a recipe. There's no curated "here's a good starting point for this engine" moment.

Meanwhile, 67% of the existing preset library is already single-engine. The infrastructure exists — it just isn't surfaced at the right moment.

## Solution: Module Starter

**Module Starter** is a per-engine curated entry point, not a new format or system. It's a UX behavior built on existing `.xometa` presets tagged with `"starter"`.

### How It Works

1. **User loads an engine** into an empty slot (via the module browser)
2. **The system automatically loads that engine's default starter preset** — the slot is immediately playable
3. **The preset browser filters to show all presets for that engine** — the user can audition from there
4. **The starter preset is a real preset** — it shows in the browser, can be saved as a variant, appears in the mood it belongs to

No new file format. No new manager class. No new directory structure. Just a tag, a convention, and a UI behavior.

### The `"starter"` Convention

Each engine designates **one** preset as its Module Starter by including `"starter"` in its tags array and adding a `"starterFor"` field:

```json
{
  "name": "Warm Drift",
  "mood": "Foundation",
  "engines": ["XOdyssey"],
  "tags": ["pad", "warm", "evolving", "starter"],
  "starterFor": "Odyssey",
  ...
}
```

**Rules:**
- Exactly one starter per engine (enforced by tooling, not schema)
- Starter presets live in their natural mood category, not a special directory
- The `"starterFor"` field names the engine's short name (OddfeliX, OddOscar, Overdub, Odyssey, Oblong, Obese, Overbite, Onset, Overworld, Opal, Organon, Ouroboros)
- Starters must be Foundation or Prism mood — approachable, not experimental
- Macro M1-M4 must all produce obvious audible change (the golden rule)
- The preset should sound good dry (before effects) — first impressions matter

### Schema Addition

Add one optional field to `xometa_schema.json`:

```json
"starterFor": {
  "type": "string",
  "description": "If present, designates this preset as the Module Starter for the named engine. Exactly one preset per engine should have this field.",
  "enum": ["OddfeliX", "OddOscar", "Overdub", "Odyssey", "Oblong", "Obese", "Overbite", "Onset", "Overworld", "Opal", "Organon", "Ouroboros"]
}
```

### UI Behavior

**On engine load into empty slot:**
1. Look up the starter preset for that engine (`starterFor == engineShortName`)
2. Load it automatically (50ms crossfade, same as engine hot-swap)
3. Show a subtle toast: "Loaded starter: Warm Drift" (dismisses after 2s)
4. Filter the preset browser to show that engine's presets
5. Highlight the loaded starter in the list

**If no starter exists** (e.g., engine is brand new): load engine defaults silently, no toast.

**Starter vs. Recipe interaction:** If a user loads a recipe, the recipe's configuration takes priority. Module Starters only fire when loading a single engine into a single empty slot.

### Starter Preset Guidelines

Each starter preset should showcase the engine's **core identity** — the thing that makes it different from every other engine:

| Engine | Starter Identity | Suggested Mood |
|--------|-----------------|----------------|
| ODDFELIX | Punchy pluck with filter sweep | Foundation |
| ODDOSCAR | Blooming pad that evolves with macro | Prism |
| OVERDUB | Warm delay wash — play a note and hear the space | Foundation |
| ODYSSEY | Mid-journey Climax position, half-alien | Prism |
| OBLONG | Tactile, fuzzy lead with curiosity | Foundation |
| OBESE | Thick stacked unison, filter opening | Foundation |
| ONSET | Punchy 808 kit, circuit side | Foundation |
| OVERWORLD | Classic NES square-wave pulse melody | Prism |
| OPAL | Granular cloud from a simple tone | Prism |
| ORGANON | Breathing organ with warm harmonics | Prism |
| OUROBOROS | Self-modulating lead, controlled chaos | Prism |

### Implementation

**Phase 1 — Data:**
- Tag 7 existing single-engine presets as starters (one per core engine)
- Add `"starterFor"` field to those presets
- Add `"starterFor"` to `xometa_schema.json`

**Phase 2 — Behavior:**
- `PresetManager::getStarterForEngine(engineShortName)` — scans preset library for matching `starterFor` field, caches result at startup
- Wire engine-load path in `EngineRegistry` to call `PresetManager::getStarterForEngine()` and apply the preset

**Phase 3 — UI:**
- Toast notification on starter load
- Preset browser auto-filter on engine load
- Starter badge in preset browser (small star or bolt icon)

### What This Is Not

- **Not init presets.** These are real, evocative, named presets — not "Init Patch 47." The master spec explicitly says no to clinical naming.
- **Not a new format.** Module Starters are regular `.xometa` files with one extra field.
- **Not required.** An engine without a starter still works — you just get silence until you load a preset manually.
- **Not recipes.** Recipes are multi-engine (2-4 slots). Module Starters are single-engine. The two systems don't overlap.

### Relationship to Recipes

```
User action                → System response
─────────────────────────────────────────────────
Load 1 engine into slot    → Module Starter (auto-loads starter preset)
Browse recipes, pick one   → Recipe system (loads 2-4 engines + coupling)
Browse presets, pick one   → Standard preset load (1-4 engines, complete sound)
```

Three entry points, three mechanisms, zero overlap.
