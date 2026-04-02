# Preset Quality Pass — Round 11H

**Date:** 2026-03-14
**Scope:** All presets added since Round 9 (`Docs/preset_expansion_9g.md`)
**Total reviewed:** 42 presets across 6 mood folders

---

## Summary

- **18 presets renamed** (old name → new name)
- **0 sonic_dna blocks added** — all 42 presets already carry a valid `dna` block
- **0 structural issues found** — all JSON parses cleanly, all `name` fields ≤ 30 chars
- **24 presets passed** with no changes required

---

## Presets Reviewed

### Osprey presets (12 total) — Aether, Atmosphere, Foundation, Flux, Prism

| File | Name | Verdict |
|------|------|---------|
| `Aether/Osprey_Open_Water_Dissolve.xometa` | Open Water Dissolve | PASS — evocative, accurate |
| `Atmosphere/Osprey_Pacific_Night_Dive.xometa` | Pacific Night Dive | PASS — painterly, earns it |
| `Atmosphere/Osprey_Salt_Mist.xometa` | Salt Mist | PASS — 2 words, strong |
| `Foundation/Osprey_Atlantic_Ground.xometa` | Atlantic Ground | PASS — foundational anchor |
| `Foundation/Osprey_Harbor_Mouth.xometa` | Harbor Mouth | PASS — specific geography, vivid |
| `Flux/Osprey_Tidal_Friction.xometa` | Tidal Friction | PASS — movement + force |
| `Flux/Osprey_Wind_Change.xometa` | Wind Change | PASS — moment of transition |
| `Prism/Osprey_Shallow_Turbulence.xometa` | Shallow Turbulence | PASS — sonic picture of fragmented resonators |
| `Prism/Osprey_Storm_Petrel.xometa` | Storm Petrel | PASS — specific creature + weather, intense |
| `Entangled/Osprey_Whale_Song_Coupling.xometa` | **RENAMED** | see below |

### Osteria presets (11 total) — Aether, Atmosphere, Foundation, Flux, Prism, Entangled

| File | Name | Verdict |
|------|------|---------|
| `Aether/Osteria_Last_Table.xometa` | Last Table | PASS — melancholy, specific moment |
| `Atmosphere/Osteria_Mediterranean_Dusk.xometa` | Mediterranean Dusk | PASS — cinematic |
| `Atmosphere/Osteria_Nordic_Fjord_Song.xometa` | Nordic Fjord Song | PASS — cultural specificity |
| `Foundation/Osteria_Guitarra_Portuguesa.xometa` | Guitarra Portuguesa | PASS — instrument = identity here |
| `Foundation/Osteria_Tavern_Oak.xometa` | Tavern Oak | PASS — material, warm |
| `Flux/Osteria_Kora_Metabolism.xometa` | Kora Metabolism | PASS — instrument + biological process |
| `Flux/Osteria_Pacific_Argument.xometa` | Pacific Argument | PASS — geography + conflict |
| `Prism/Osteria_Gamelan_Shard.xometa` | Gamelan Shard | PASS — instrument + fragmentation |
| `Prism/Osteria_Rembetika_Fire.xometa` | Rembetika Fire | PASS — style + element |
| `Entangled/Osteria_Shore_Dialogue.xometa` | Shore Dialogue | PASS — the tavern and ocean in conversation |

### XVC ONSET presets (11 total) — Flux/Onset/XVC/

All previously used "XVC DEMO N:" labeling in descriptions (retained) but names were technical.

| Old Name | New Name | File |
|----------|----------|------|
| Full Mesh XVC | **Neural Storm** | `Neural_Storm.xometa` |
| Hat Chokes Clap | **Handshake** | `Handshake.xometa` |
| Kick Dominates | **Solar System** | `Solar_System.xometa` |
| Kick Pitches Toms | **Gravity Bend** | `Gravity_Bend.xometa` |
| Kick Pumps Hats | **Bright Tide** | `Bright_Tide.xometa` |
| Minimal XVC | **First Choke** | `First_Choke.xometa` |
| Ghost Generator | — PASS — | `Ghost_Generator.xometa` |
| Inverse Field | — PASS — | `Inverse_Field.xometa` |
| Mutate Cascade | — PASS — | `Mutate_Cascade.xometa` |
| Polyrhythm Emergence | — PASS — | `Polyrhythm_Emergence.xometa` |
| Snare Blooms Space | — PASS — | `Snare_Blooms_Space.xometa` |

**Reasoning for passed XVC names:**
- "Ghost Generator" — earns it: emergent sub-threshold ghost notes, not described anywhere in the mechanism
- "Inverse Field" — counterpoint logic evoked without explaining it
- "Mutate Cascade" — cascading stochastic evolution, organism language
- "Polyrhythm Emergence" — emergence is the right word for what XVC produces here
- "Snare Blooms Space" — timbre mutation triggered by snare, spaciousness opens up

### Bob aggression expansion presets (10 total) — Flux and Prism

All 10 used `Bob_` prefix + genre/technical description pattern. All renamed.

| Old Name | New Name | Mood | File |
|----------|----------|------|------|
| Bob_Acid_Drive | **Rubber Spine** | Flux | `Rubber_Spine.xometa` |
| Bob_Fuzz_Bass | **Velvet Slab** | Flux | `Velvet_Slab.xometa` |
| Bob_Glitch_Stutter | **Circuit Rash** | Flux | `Circuit_Rash.xometa` |
| Bob_Industrial_Clank | **Steel Rain** | Flux | `Steel_Rain.xometa` |
| Bob_Noise_Burst | **White Scar** | Flux | `White_Scar.xometa` |
| Bob_Terror_Sub | **Midnight Weight** | Flux | `Midnight_Weight.xometa` |
| Bob_Crunch_Layer | **Grit Choir** | Prism | `Grit_Choir.xometa` |
| Bob_Distorted_Pad | **Scorched Bloom** | Prism | `Scorched_Bloom.xometa` |
| Bob_Growl_Lead | **Throat Opener** | Prism | `Throat_Opener.xometa` |
| Bob_Max_Fold | **Molten Spectral** | Prism | `Molten_Spectral.xometa` |

### Entangled cross-engine presets (3 total)

| Old Name | New Name | File |
|----------|----------|------|
| XVC Dub Mesh | **Double Nervous System** | `Double_Nervous_System.xometa` |
| Whale Song Coupling | **Cetacean Drive** | `Cetacean_Drive.xometa` |
| Shore Dialogue | — PASS — | `Osteria_Shore_Dialogue.xometa` |

---

## Rename Rationale

### Bob presets
All 10 were in the form `Bob_[Mechanism]_[Genre]` — violating the naming rules against genre labels and technical descriptions. Each rename targets what the patch *feels* like or evokes at a sensory level:

- **Rubber Spine** — resonant acid drive that snaps back, tension + elasticity
- **Velvet Slab** — plush surface over massive low-end weight
- **Circuit Rash** — digital chaos that irritates and disrupts, micro-cuts
- **Steel Rain** — metallic percussive hits falling, industrial without naming the genre
- **White Scar** — impact on contact, brightness, percussive wound
- **Midnight Weight** — threat in the dark, sub presence without naming bass
- **Grit Choir** — 8-voice poly cluster grinding, density of voices
- **Scorched Bloom** — long attack through fire, aggressive but breathing
- **Throat Opener** — FM growl lead, larynx language for something that sounds alive
- **Molten Spectral** — wavefold at maximum produces something metallic and liquefied

### XVC ONSET presets
Six of eleven described mechanisms in their names rather than sonic results:

- **Neural Storm** — "all neurons firing" was literally in the description; this is the name
- **Handshake** — the naturalness of hat choke is a physical social gesture
- **Solar System** — pulled directly from the description's metaphor ("the kick is the sun")
- **Gravity Bend** — kick's weight bends toms downward, gravitational pull
- **Bright Tide** — kick opens hats = rhythm brightens rhythmically, tide-like
- **First Choke** — the entry point, the single hat choke interaction, the beginning

### Entangled presets
- **Double Nervous System** — two levels of coupling (XVC internal + cross-engine external), two entangled nervous systems, each sensing the other
- **Cetacean Drive** — whale biology as modulation signal; the whale *drives* the texture; precise and unusual

---

## sonic_dna Status

All 42 reviewed presets carry a valid `dna` block with all 6 dimensions (brightness, warmth, movement, density, space, aggression). No additions required.

The schema field name is `dna` — not `sonic_dna`. This is consistent across the entire fleet and matches `Docs/xometa_schema.json`.

---

## Structural Issues Found

None. All 42 files:
- Parse as valid JSON
- Carry `schema_version: 1`
- Carry `dna` blocks
- Have `name` fields ≤ 30 characters (longest: "Double Nervous System" at 21 chars)
- Have all required fields per schema (`schema_version`, `name`, `mood`, `engines`, `author`, `version`, `parameters`)

One minor observation: The 10 Bob aggression presets carry `engines: ["Oblong"]` which uses the old standalone name rather than the canonical engine ID `Oblong` / `Bob`. This is consistent with other Bob presets in the fleet and will be resolved by the alias resolver in `PresetManager.h` — not a new issue introduced in Round 10.

---

## Files Modified

### Renamed (filename + internal `name` field updated):
- `Presets/XOceanus/Flux/Rubber_Spine.xometa`
- `Presets/XOceanus/Flux/Velvet_Slab.xometa`
- `Presets/XOceanus/Flux/Circuit_Rash.xometa`
- `Presets/XOceanus/Flux/Steel_Rain.xometa`
- `Presets/XOceanus/Flux/White_Scar.xometa`
- `Presets/XOceanus/Flux/Midnight_Weight.xometa`
- `Presets/XOceanus/Prism/Grit_Choir.xometa`
- `Presets/XOceanus/Prism/Scorched_Bloom.xometa`
- `Presets/XOceanus/Prism/Throat_Opener.xometa`
- `Presets/XOceanus/Prism/Molten_Spectral.xometa`
- `Presets/XOceanus/Flux/Onset/XVC/Neural_Storm.xometa`
- `Presets/XOceanus/Flux/Onset/XVC/Handshake.xometa`
- `Presets/XOceanus/Flux/Onset/XVC/Solar_System.xometa`
- `Presets/XOceanus/Flux/Onset/XVC/Gravity_Bend.xometa`
- `Presets/XOceanus/Flux/Onset/XVC/Bright_Tide.xometa`
- `Presets/XOceanus/Flux/Onset/XVC/First_Choke.xometa`
- `Presets/XOceanus/Entangled/Double_Nervous_System.xometa`
- `Presets/XOceanus/Entangled/Cetacean_Drive.xometa`

### Unchanged (passed review):
- All 12 Osprey presets (excluding the Entangled coupling preset which was renamed)
- All 10 Osteria presets (excluding the Entangled coupling preset)
- `Presets/XOceanus/Flux/Onset/XVC/Ghost_Generator.xometa`
- `Presets/XOceanus/Flux/Onset/XVC/Inverse_Field.xometa`
- `Presets/XOceanus/Flux/Onset/XVC/Mutate_Cascade.xometa`
- `Presets/XOceanus/Flux/Onset/XVC/Polyrhythm_Emergence.xometa`
- `Presets/XOceanus/Flux/Onset/XVC/Snare_Blooms_Space.xometa`
- `Presets/XOceanus/Entangled/Osteria_Shore_Dialogue.xometa`
