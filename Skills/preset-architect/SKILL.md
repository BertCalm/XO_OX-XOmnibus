# Skill: /preset-architect

**Invoke with:** `/preset-architect`
**Status:** LIVE
**Purpose:** Create high-quality `.xometa` presets from scratch — parameter design, DNA assignment, naming, mood selection, macro mapping, and quality gate.

---

## When to Use This Skill

Use this skill when:
- Writing new factory presets for any engine
- Backfilling mood coverage gaps
- Creating presets that demonstrate a specific engine feature
- Expanding thin mood folders
- Responding to a preset count audit

---

## Phase 1: Understand the Engine

Before writing a single parameter, know these things:

| Question | Where to Find the Answer |
|----------|-------------------------|
| What is this engine's sonic thesis? | `Docs/xomnibus_sound_design_guides.md` or the engine's concept brief in `Docs/concepts/` |
| What are its key parameters? | Sound design guide → "Key Parameters" section |
| What does M1–M4 control? | Sound design guide → "Macros" section |
| What coupling does it send/receive? | `Docs/coupling_audit.md` |
| What moods does it currently cover? | `Tools/audit_sonic_dna.py` output |

**Golden rule:** A dry patch on this engine should sound compelling before any effects are applied. If you need to rely on reverb for the sound to be interesting, rethink the parameters.

---

## Phase 2: Choose Mood and Sound Goal

### The 8 Moods

| Mood | Target Sounds | DNA Profile |
|------|--------------|-------------|
| **Foundation** | Bass, kick, sub, groove anchor, rhythmic glue | High `aggression` or `density`, lower `space` |
| **Atmosphere** | Pads, drones, washes, textural clouds | High `space` + `warmth`, low `aggression` |
| **Entangled** | Coupled/reactive — must use cross-engine coupling | High `movement`, `couplingIntensity` ≠ None |
| **Prism** | Leads, keys, bells, melodic lines, articulate voices | Moderate-to-high `brightness`, lower `density` |
| **Flux** | Glitchy, unstable, broken, lo-fi, experimental | High `movement` + `aggression`, lower `warmth` |
| **Aether** | Cinematic, transcendent, evolving, spiritual | High `space` + `movement`, lower `aggression` |
| **Family** | Constellation family engines (OHM/ORPHICA/OBBLIGATO/OTTONI/OLE) only | Per-engine |
| **Submerged** | Deep underwater, abyssal, pressure, bioluminescence | High `density` + `space`, low `brightness`, moderate `warmth` |

### Mood Coverage Check

Before adding to a mood, check coverage:
```bash
python3 Tools/audit_sonic_dna.py
```
Prioritize moods with fewer presets for this engine, especially Flux and Aether (historically thin).

---

## Phase 3: Parameter Design

### Design Principles

1. **Identity first** — M1 (CHARACTER) parameter should be the most expressive knob. Set it to a value that defines the sound's personality.

2. **Breathing required (D005)** — At least one LFO must be active and audible. Rate should not be 0.

3. **Velocity shapes timbre (D001)** — Set `velFilterAmt` (or equivalent) so playing harder makes the filter brighter.

4. **Macro golden rule** — Turn M1 from 0 to 1: the sound must change. Turn M2 from 0 to 1: movement must increase. Turn M3 from 0 to 1 with two engines active: coupling must increase. Turn M4 from 0 to 1: reverb/space must increase. If any macro does nothing, fix it before saving.

### Common Starting Points by Mood

**Foundation:**
- High `filterCutoff` self-resonance or heavy saturation
- Short attack, punchy decay
- Low reverb mix (< 0.15)
- LFO2 on amplitude for pump/swell

**Atmosphere:**
- Slow attack (> 0.5s), long release (> 2s)
- Moderate filter (40–60% open)
- LFO1 on filter cutoff, rate 0.03–0.15 Hz
- High reverb (> 0.4)

**Prism:**
- Fast attack (< 30ms), clean decay
- Filter open (60–80%)
- LFO vibrato subtle (depth < 0.15)
- Low reverb except for bell-type sounds

**Flux:**
- S&H LFO on pitch or filter (use lfo1Shape = S&H)
- High resonance (> 0.6)
- Moderate-to-high distortion/drive
- LFO rate mid-fast (0.5–4 Hz)

---

## Phase 4: Macro Mapping

Always label macros explicitly in `.xometa`:

```json
"macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]
```

The engine's sound design guide specifies what each macro controls. A few rules:

- **M1 (CHARACTER):** Should change the most fundamental timbral quality of the sound.
  - ODDFELIX: `snap_snap` (dart intensity)
  - ODYSSEY: `odyssey_journeyPos` (familiar→alien)
  - OBLONG: `bob_curiosity` (personality intensity)
  - etc. — always use the engine's primary character parameter
  
- **M2 (MOVEMENT):** Must increase perceptible motion when turned up.
  - Common target: LFO depth, auto-modulation depth, animation speed

- **M3 (COUPLING):** Should do NOTHING in single-engine presets (coupling amount = 0). In coupled presets it controls coupling intensity.

- **M4 (SPACE):** Always controls FX wetness (reverb mix, delay mix, or both).

---

## Phase 5: Assign Sonic DNA

DNA is the most important metadata — it powers preset search, morphing, and breeding.

### The 6 Dimensions

| Dimension | 0.0 | 0.5 | 1.0 | How to Estimate |
|-----------|-----|-----|-----|-----------------|
| `brightness` | Dark, muffled, sub-focused | Balanced midrange | Bright, airy, lots of highs | Filter position + presence of high harmonics |
| `warmth` | Cold, digital, hard | Moderate saturation | Warm, saturated, rounded | Drive/tape/saturation amount + slow drift |
| `movement` | Static, frozen | Slow evolution | Constant rapid motion | LFO depth × LFO rate + modulation density |
| `density` | Sparse, single voice | 2-3 layer | Thick, stacked, massive | Voice count + unison count + detuning |
| `space` | Dry, close, intimate | Moderate reverb | Vast, distant, huge reverb | Reverb size × mix + delay feedback |
| `aggression` | Gentle, soft, smooth | Moderate edge | Harsh, distorted, sharp attack | Drive + resonance + attack time + noise/grain |

### DNA Formulas (approximate)

These formulas are **conceptual** — they use generic names. Replace with engine-specific parameter names when computing by hand (e.g., `snap_filterCutoff` for ODDFELIX, `odyssey_filterCutoff` for ODYSSEY — see `Docs/xomnibus_sound_design_guides.md`). For automated computation, use `python3 Tools/compute_preset_dna.py`.

```
brightness  = (filterCutoff / 20000) * 0.7 + (harmonicRichness) * 0.3
warmth      = (driveAmount * 0.5) + (lfo1Rate < 0.1 ? 0.3 : 0.0) + (sub presence * 0.2)
movement    = (lfo1Depth * lfo1Rate / 5.0) * 0.5 + (lfo2Depth * lfo2Rate / 5.0) * 0.5
density     = (unisonCount / 4.0) * 0.5 + (polyphony > 4 ? 0.3 : 0.1) + (detuneWidth > 20 ? 0.2 : 0.0)
space       = (reverbMix * 0.7) + (delayFeedback * 0.3)
aggression  = (resonance * 0.3) + (driveHard ? 0.4 : driveAmount * 0.2) + (attackTime < 0.01 ? 0.3 : 0.0)
```

All values clamped to 0.0–1.0, rounded to 2 decimal places.

### DNA Coverage Rules

Check `Docs/sonic_dna_audit.md` for gaps. When adding a preset:
- If the engine has a `high-end gap` (max < 0.70) in a dimension, write a preset that pushes that dimension high
- If the engine has a `low-end gap` (min > 0.30) in a dimension, write a preset that brings that dimension low
- Don't cluster presets — spread them across the 6D space

**Automated DNA tool:** `python3 Tools/add_missing_dna.py` can auto-compute DNA from tags and engine context.

---

## Phase 6: Write the `.xometa` File

### Full Template

```json
{
  "schema_version": 1,
  "name": "[2-3 words, max 30 chars, evocative, no jargon]",
  "mood": "[Foundation|Atmosphere|Entangled|Prism|Flux|Aether|Family]",
  "engines": ["[EngineID]"],
  "author": "XO_OX Designs",
  "version": "1.0.0",
  "description": "[One sentence. What does it sound like? What does it do? No jargon.]",
  "tags": ["[tag1]", "[tag2]", "[tag3]"],
  "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
  "couplingIntensity": "None",
  "tempo": null,
  "dna": {
    "brightness": 0.00,
    "warmth": 0.00,
    "movement": 0.00,
    "density": 0.00,
    "space": 0.00,
    "aggression": 0.00
  },
  "parameters": {
    "[EngineID]": {
      "[prefix_paramName]": 0.0
    }
  },
  "coupling": {
    "pairs": []
  },
  "sequencer": null
}
```

### Engine ID Reference

| Gallery Code | Engine ID in .xometa |
|-------------|---------------------|
| ODDFELIX | `OddfeliX` |
| ODDOSCAR | `OddOscar` |
| OVERDUB | `Overdub` |
| ODYSSEY | `Odyssey` |
| OBLONG | `Oblong` |
| OBESE | `Obese` |
| ONSET | `Onset` |
| OVERWORLD | `Overworld` |
| OPAL | `Opal` |
| ORBITAL | `Orbital` |
| ORGANON | `Organon` |
| OUROBOROS | `Ouroboros` |
| OBSIDIAN | `Obsidian` |
| OVERBITE | `Overbite` |
| ORIGAMI | `Origami` |
| ORACLE | `Oracle` |
| OBSCURA | `Obscura` |
| OCEANIC | `Oceanic` |
| OCELOT | `Ocelot` |
| OPTIC | `Optic` |
| OBLIQUE | `Oblique` |
| OSPREY | `Osprey` |
| OSTERIA | `Osteria` |
| OWLFISH | `Owlfish` |
| OHM | `Ohm` |
| ORPHICA | `Orphica` |
| OBBLIGATO | `Obbligato` |
| OTTONI | `Ottoni` |
| OLE | `Ole` |
| OVERLAP | `Overlap` |
| OUTWIT | `Outwit` |
| OMBRE | `Ombre` |
| ORCA | `Orca` |
| OCTOPUS | `Octopus` |
| OSTINATO | `Ostinato` |
| OPENSKY | `OpenSky` |
| OCEANDEEP | `OceanDeep` |
| OUIE | `Ouie` |
| OBRIX | `Obrix` |
| ORBWEAVE | `Orbweave` |
| OVERTONE | `Overtone` |
| ORGANISM | `Organism` |

---

## Phase 7: Name and File

### Naming Rules
- 2–3 words, maximum 30 characters
- Evocative and poetic — describe the feeling, not the mechanism
- No synth jargon ("FM pad", "wavetable sweep", "filter cutoff")
- No duplicates — check existing names in `Presets/XOmnibus/`
- No slashes, colons, or special characters except spaces and hyphens

**Good names:** "Warm Amber Fog", "Neon Tetra Chase", "Drift Between Worlds", "Copper Kettle", "Still Water Moon"
**Bad names:** "OBLONG FM Pad", "Preset_003", "Filter Sweep Bass", "Warm/Dark/Slow"

### File Location
`Presets/XOmnibus/{Mood}/{name_slug}.xometa`

Where `name_slug` = name with spaces replaced by `_`:
- "Warm Amber Fog" → `Warm_Amber_Fog.xometa`

---

## Quality Gate Before Saving

- [ ] Every macro (M1–M4) produces an audible change across its full range
- [ ] At least one LFO is active and audible (rate ≤ 20 Hz, depth > 0)
- [ ] Velocity shapes the timbre (not just amplitude)
- [ ] DNA values are assigned and non-zero
- [ ] Name is unique in the library
- [ ] Description is one clear sentence describing what it sounds like
- [ ] At least 3 tags
- [ ] No default/zero values for parameters that should be expressive
- [ ] Run: `python3 Tools/validate_presets.py` — must pass

---

## Tool Reference

| Task | Command |
|------|---------|
| Check DNA completeness | `python3 Tools/audit_sonic_dna.py` |
| Add missing DNA automatically | `python3 Tools/add_missing_dna.py` |
| Validate preset schema | `python3 Tools/validate_presets.py` |
| Check duplicate names | `python3 Tools/validate_presets.py --check-duplicates` |
| Compute DNA from parameters | `python3 Tools/compute_preset_dna.py --preset path/to/preset.xometa` |
| Breed two presets | `python3 Tools/breed_presets.py` |
