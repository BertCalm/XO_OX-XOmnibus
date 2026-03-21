# Skill: /preset-auditor

**Invoke with:** `/preset-auditor`
**Status:** LIVE
**Last Updated:** 2026-03-20 | **Version:** 1.0 | **Next Review:** On new mood or doctrine change
**Purpose:** Quality gate for existing `.xometa` presets. Audits DNA coverage, macro responsiveness, D004 compliance, naming conventions, and mood correctness. Use before committing new presets or after bulk preset generation.

---

## When to Use

- After generating presets with an agent (bulk or single)
- Before committing presets to the repo
- When a preset "doesn't sound right" and you need to diagnose why
- After engine parameter changes that might break existing presets
- As part of `/master-audit` Phase 3

---

## Phase 1: Schema Validation

Run the automated validator first:

```bash
python3 Tools/validate_presets.py Presets/XOmnibus/{mood}/{preset}.xometa
# Or for a whole mood folder:
python3 Tools/validate_presets.py Presets/XOmnibus/{mood}/
# Or fleet-wide:
python3 Tools/validate_presets.py Presets/XOmnibus/
```

**Schema errors to catch:**
- Missing required fields: `name`, `engines`, `parameters`, `dna`, `macros`, `mood`, `author`, `version`
- Engine ID not in canonical list (check `PresetManager.h:validEngineNames`)
- Parameter prefix mismatch (e.g., `snap_` params in an ORGANISM preset)
- `author` must be `"XO_OX Designs"` (not `"XO_OX"` — bulk-normalized 2026-03-14)
- `version` must be `"1.0"` for factory presets

---

## Phase 2: DNA Coverage

Every preset must have all 6 DNA dimensions with values in [0.0, 1.0]:

```json
"dna": {
  "brightness": 0.0–1.0,
  "warmth": 0.0–1.0,
  "movement": 0.0–1.0,
  "density": 0.0–1.0,
  "space": 0.0–1.0,
  "aggression": 0.0–1.0
}
```

**Common DNA errors:**
- Any dimension missing (null or absent) → run `/dna-designer`
- All values at 0.5 (auto-generated placeholder) → needs real calibration
- Brightness > 0.9 for Submerged mood (should be dark, < 0.4)
- Aggression > 0.8 for Atmosphere mood (should be calm, < 0.4)

Use `/dna-designer` to assign accurate values if DNA is missing or placeholder.

---

## Phase 3: Macro Responsiveness (D004)

Each preset must have all 4 macros wired to audible parameters:

```json
"macros": {
  "CHARACTER": 0.5,    // M1 — turning 0→1 must change character
  "MOVEMENT": 0.5,     // M2 — turning 0→1 must increase movement
  "COUPLING": 0.5,     // M3 — turning 0→1 must change coupling
  "SPACE": 0.5         // M4 — turning 0→1 must increase reverb/space
}
```

**Verification (manual):**
- Set M1 to 0, then 1 — sound must change noticeably
- Set M2 to 0, then 1 — movement/modulation must increase
- Set M3 to 0, then 1 — coupling intensity must change (only meaningful with 2+ engines)
- Set M4 to 0, then 1 — space/reverb must increase

If any macro does nothing, the preset violates D004. Fix the engine's macro wiring first, then rebuild the preset.

---

## Phase 4: Naming Convention

Preset names must follow:
- **2–3 words** (e.g., "Coral Architect", "Midnight Surge", "Abyssal Flicker")
- **Max 30 characters** including spaces
- **No synth jargon** ("FM pad", "wavetable sweep", "filter envelope" are forbidden)
- **No duplicates** across the entire `Presets/XOmnibus/` tree

```bash
# Check for duplicates fleet-wide
python3 Tools/validate_presets.py --check-duplicates
```

---

## Phase 5: Mood Correctness

Verify the preset's sound matches its declared mood:

| Mood | Character Check |
|------|----------------|
| Foundation | Low end present, high `aggression` or `density`? |
| Atmosphere | Long release (> 1s)? High reverb? Low aggression? |
| Entangled | Two engines active with coupling routes? `couplingIntensity` ≠ None? |
| Prism | Distinct melodic articulation? Moderate brightness? |
| Flux | Unstable, glitchy, or lo-fi element present? |
| Aether | Cinematic/evolving? Long modulation cycles? |
| Family | Engine is one of OHM/ORPHICA/OBBLIGATO/OTTONI/OLE? |
| Submerged | Dark character (brightness < 0.4)? Sub-bass or pressure elements? |

---

## Phase 6: D001 Velocity Check

Play the preset at velocity 30, then velocity 100. **The timbre must differ**, not just the volume:
- Filter should open at higher velocity
- Harmonic brightness should increase
- At minimum, the sound should feel "harder" at high velocity in a way beyond amplitude

If velocity only changes volume, the preset relies on the engine's D001 implementation but the preset's parameter settings may be blocking it (e.g., `velFilterAmt = 0`). Raise `velFilterAmt` (or engine equivalent) to at least 0.3.

---

## Phase 7: Entangled Preset Coupling Check

For `"mood": "Entangled"` presets only:

```json
{
  "engines": ["EnginA", "EngineB"],
  "coupling": {
    "routes": [
      {"source": "EngineA", "target": "EngineB", "type": "AmpToFilter", "amount": 0.4}
    ]
  }
}
```

**Required:**
- At least 2 engines loaded
- At least 1 active coupling route with amount > 0.1
- Route uses a non-STUB CouplingType (check `/coupling-interaction-cookbook`)
- M3 (COUPLING macro) audibly changes coupling intensity

---

## Quality Gate Checklist

Before marking preset as ship-ready:

- [ ] Schema validation passes (no missing fields, correct engine IDs)
- [ ] Author is `"XO_OX Designs"`
- [ ] All 6 DNA dimensions populated with calibrated values (not 0.5 placeholders)
- [ ] Name: 2–3 words, ≤ 30 chars, no jargon, no duplicate
- [ ] Mood matches sound character
- [ ] M1–M4 all produce audible change
- [ ] Velocity changes timbre (not just volume)
- [ ] For Entangled: coupling route present and active

---

## Related Skills

- `/dna-designer` — assign accurate 6D DNA when values are missing/placeholder
- `/preset-architect` — build a new preset from scratch with all rules applied
- `/coupling-preset-designer` — design Entangled mood coupling presets
- `/engine-health-check` — verify the engine is doctrinally sound before building presets

## Related Tools

- `Tools/validate_presets.py` — schema + duplicate + coupling validation
- `Tools/audit_sonic_dna.py` — DNA coverage gap analysis
- `Tools/migrate_presets.py` — engine name migration for legacy presets
