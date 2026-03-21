# Skill: /coupling-preset-designer

**Invoke with:** `/coupling-preset-designer`
**Status:** LIVE
**Last Updated:** 2026-03-20 | **Version:** 1.0 | **Next Review:** On new engine addition or coupling type change
**Purpose:** Design a coupling preset from scratch — engine pair selection, coupling type mapping, parameter crafting, DNA assignment, and final `.xometa` output.

---

## When to Use This Skill

Use this skill when:
- Creating a new preset that highlights cross-engine coupling as its primary feature
- A user asks for a "coupled" or "Entangled" mood preset
- Filling coverage gaps in the `Entangled` mood folder
- Demonstrating what a specific `CouplingType` sounds like in practice

---

## Phase 1: Engine Pair Selection

### Step 1A — Identify Source and Target

Every coupling preset has a **source engine** (sends modulation) and a **target engine** (receives modulation). Start here:

1. Pick the **source**: What is generating the primary rhythmic, tonal, or envelope event?
   - Percussive/rhythmic source → ONSET (score 4), ODDFELIX (score 2), OUROBOROS (score 5)
   - Melodic/tonal source → ODYSSEY, ORBITAL, OBSCURA, ORACLE
   - Textural/granular source → OPAL, OVERWORLD, ORGANON

2. Pick the **target**: What parameter do you want to modulate?
   - Filter cutoff → any engine with `AmpToFilter` (see coupling audit — most engines)
   - Grain/morph position → OPAL (`Env→Morph`), ODYSSEY (`Env→Morph`), ORGANON (`Env→Morph`)
   - FM depth → ORBITAL, OUROBOROS, ORIGAMI, OBSCURA, ORGANON, OSTERIA
   - Pitch → engines with `AmpToPitch` or `LFOToPitch`
   - Decay time → ONSET, OBLIQUE, ORIGAMI
   - Rhythm blend → ONSET, OBSCURA, OBLIQUE, OCEANIC, OUROBOROS

3. **Check the coupling audit table** (`Docs/coupling_audit.md`):
   - Source needs `getSampleForCoupling` = PROPER (not STUB)
   - Target needs `applyCouplingInput` = PARTIAL or FULL (not STUB)
   - **NEVER pair two STUB engines** — no modulation will occur

### Step 1B — Check for Double Stubs

Engines with STUB coupling input (no implemented coupling reception as of audit):
- OCELOT — stub (all CouplingTypes void-cast)
- OWLFISH — stub (all CouplingTypes void-cast)

These **cannot be targets**. They can be sources if `getSampleForCoupling` returns PROPER data.

---

## Phase 2: CouplingType Selection

### The 12 Coupling Types

| Type | ID | Semantic | Best Source→Target Pair |
|------|----|----------|------------------------|
| `AmpToFilter` | 1 | Source amplitude → target filter cutoff | ONSET→OVERBITE, ODDFELIX→ODYSSEY |
| `AmpToPitch` | 2 | Source amplitude → target pitch | ONSET→OBLONG, OUROBOROS→ODDFELIX |
| `LFOToPitch` | 3 | Source LFO output → target pitch | OVERDUB→ODYSSEY, ORBITAL→OPAL |
| `EnvToMorph` | 4 | Source envelope → target morph/wavetable scan | ODYSSEY→OPAL, ORACLE→ORGANON |
| `AudioToFM` | 5 | Source audio → target FM input | ORACLE→ORGANON, OUROBOROS→ONSET |
| `AudioToRing` | 6 | Source audio × target (ring mod) | ODDFELIX→ORBITAL, OPTIC→ORBITAL |
| `FilterToFilter` | 7 | Source filter output → target filter input | OPTIC→any (Optic is analysis-only) |
| `AmpToChoke` | 8 | Source amplitude chokes target | ONSET→ONSET (voice choke), ODDFELIX→ONSET |
| `RhythmToBlend` | 9 | Source rhythm → target blend param | OUROBOROS→ONSET, OVERWORLD→OPAL |
| `EnvToDecay` | 10 | Source envelope → target decay time | OUROBOROS→ONSET, ODDFELIX→OBLIQUE |
| `PitchToPitch` | 11 | Source pitch → target pitch (harmony) | ODYSSEY→OBLONG, ORBITAL→OPAL |
| `AudioToWavetable` | 12 | Source audio → target wavetable buffer | OVERWORLD→OPAL, ORACLE→ORBITAL |

### Choosing Coupling Amount

| Effect Goal | `amount` Range |
|-------------|---------------|
| Barely perceptible breathing | 0.05–0.15 |
| Audible but subtle | 0.15–0.35 |
| Equal partnership — both engines shape each other | 0.35–0.65 |
| Source dominates target | 0.65–0.85 |
| Full domination | 0.85–1.0 |

---

## Phase 3: Build the Preset Parameters

### Step 3A — Set the Foundation

1. Start with the **target engine** parameters — set a sound that can breathe and move. Don't over-process; leave room for the coupling to arrive.
2. Set the **source engine** to produce clear events: either rhythmic (for envelope coupling) or tonal (for audio FM/wavetable coupling).

### Step 3B — Macro Mapping for Coupling Presets

**Rule: M3 (COUPLING) must directly control the `amount` field in the coupling pairs array in every coupled preset.** Note: `couplingIntensity` in the `.xometa` schema is a descriptive label (`"Subtle"`, `"Moderate"`, `"Deep"`) derived from the amount value — it is not a runtime parameter. The actual macro target is the numeric `amount` field.

Standard coupling preset macro mapping:
| Macro | Label | What It Controls |
|-------|-------|-----------------|
| M1 | CHARACTER | Source engine's primary timbral character |
| M2 | MOVEMENT | Target engine's primary movement/modulation |
| M3 | COUPLING | Coupling route amount (0 = decoupled, 1 = full coupling) |
| M4 | SPACE | Shared reverb/delay depth |

### Step 3C — Verify the Chain

Before writing the preset, confirm:
- [ ] Source engine `getSampleForCoupling` channel output matches the coupling type
  - ch0 = left audio, ch1 = right audio
  - ch2 = envelope signal (most engines), or LFO (ODDOSCAR), or attractor velocity X (OUROBOROS)
  - ch3 = attractor velocity Y (OUROBOROS only), individual mod channels (OPTIC only)
- [ ] Target engine `applyCouplingInput` switch case for the chosen `CouplingType` is not `break` or void-cast
- [ ] `couplingAmount` > 0.0 in the coupling pair

---

## Phase 4: Write the `.xometa` File

### Template

```json
{
  "schema_version": 1,
  "name": "[2-3 evocative words, max 30 chars]",
  "mood": "Entangled",
  "engines": ["[SourceEngineID]", "[TargetEngineID]"],
  "author": "XO_OX Designs",
  "version": "1.0.0",
  "description": "[What the coupling does in one sentence. Name the source engine action → target engine response.]",
  "tags": ["coupled", "[source character]", "[target character]"],
  "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
  "couplingIntensity": "[Subtle|Moderate|Deep]",
  "tempo": null,
  "dna": {
    "brightness": 0.0,
    "warmth": 0.0,
    "movement": 0.0,
    "density": 0.0,
    "space": 0.0,
    "aggression": 0.0
  },
  "parameters": {
    "[SourceEngineID]": {},
    "[TargetEngineID]": {}
  },
  "coupling": {
    "pairs": [
      {
        "engineA": "[SourceEngineID]",
        "engineB": "[TargetEngineID]",
        "type": "[CouplingType]",
        "amount": 0.4
      }
    ]
  },
  "sequencer": null
}
```

### `couplingIntensity` Labels

| `amount` | `couplingIntensity` |
|----------|---------------------|
| 0.0–0.25 | `"Subtle"` |
| 0.26–0.60 | `"Moderate"` |
| 0.61–1.00 | `"Deep"` |
| Mixed routes | Use the highest single route's label |

---

## Phase 5: Assign Sonic DNA

After writing parameters, assign 6D DNA values. Coupling presets have characteristic DNA patterns:

| DNA Dimension | Typical for Coupling Presets | Notes |
|---------------|------------------------------|-------|
| `brightness` | Source engine drives this | Percussive source → higher brightness |
| `warmth` | Target engine provides this | Pad/texture targets → higher warmth |
| `movement` | Always higher (0.5+) | Coupling IS movement |
| `density` | Depends on voice count | 2 engines = density 0.5–0.7 |
| `space` | Match reverb/delay mix | |
| `aggression` | Source aggression drives this | Percussive source → higher aggression |

**Rule:** `movement` should never be below 0.40 for an Entangled mood preset — if the coupling doesn't create movement, reconsider the routing.

---

## Phase 6: Name and File

1. **Name rules:** 2-3 words, max 30 chars, evocative, no synth jargon
   - Good: "Drum Eats Bass", "Orbit Eats Light", "Grain Oracle"
   - Bad: "ONSET OVERBITE Coupled", "Amp Filter Preset 1"

2. **File location:** `Presets/XOmnibus/Entangled/{name_slug}.xometa`
   - Slug: `name.replace(" ", "_").replace("/", "-")`

3. **Run DNA validator:** `python3 Tools/audit_sonic_dna.py` to verify DNA completeness

---

## Quality Gate Before Saving

- [ ] Source engine produces clear, audible events that change over time
- [ ] Target engine responds audibly to the source (turn M3 to 0 and back to confirm)
- [ ] M3 (COUPLING) macro produces audible change across its full range
- [ ] DNA `movement` ≥ 0.40
- [ ] Preset name unique in the library (check with `Tools/validate_presets.py`)
- [ ] No dead parameters (every value set to non-default should affect the sound)
- [ ] Description accurately describes what the coupling DOES, not just what it IS

---

## Proven Pair Reference

For quick-start proven pairs, use the `/coupling-interaction-cookbook` skill.
For the source data, see: `Docs/coupling_preset_library.md`, `Docs/coupling_audit.md`
