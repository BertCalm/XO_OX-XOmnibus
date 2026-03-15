---
name: preset-audit
description: Health check on XOmnibus presets. Use when auditing preset quality, checking for duplicates, validating DNA coverage, verifying schema compliance, or reviewing preset naming conventions.
argument-hint: "[mood|engine|all] e.g. Atmosphere, OddfeliX, all"
---

# Preset Health Audit

Audit presets for **$ARGUMENTS**. If argument is a mood (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family), scan that mood directory. If an engine name, find all presets using that engine. If "all", scan everything.

## Preset Location

All factory presets: `Presets/XOmnibus/{mood}/*.xometa`

7 moods: Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family

## Check 1: Schema Compliance

Read the schema from `Docs/xometa_schema.json`. For each `.xometa` file, verify:

**Required fields:**
- `schemaVersion` (integer, currently 1)
- `name` (string, 2-3 words, max 30 chars)
- `mood` (one of: Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family)
- `engines` (array of 1-4 engine IDs — must be canonical O-prefix names or resolvable legacy names)
- `author` (string)
- `parameters` (object keyed by engine ID, containing namespaced parameter values)

**Required DNA fields (6D Sonic DNA):**
- `dna.brightness` (float 0.0-1.0)
- `dna.warmth` (float 0.0-1.0)
- `dna.movement` (float 0.0-1.0)
- `dna.density` (float 0.0-1.0)
- `dna.space` (float 0.0-1.0)
- `dna.aggression` (float 0.0-1.0)

**Macro labels (4 required):**
- `macroLabels` array with exactly 4 entries
- Standard: CHARACTER, MOVEMENT, COUPLING, SPACE (custom labels allowed)

**Coupling (for multi-engine presets):**
- `couplingPairs` array (required if engines.length > 1)
- Each pair: `engineA`, `engineB`, `type` (valid CouplingType), `amount` (-1.0 to 1.0)
- `couplingIntensity`: None, Low, Medium, High

## Check 2: Naming Conventions

- 2-3 words, evocative, max 30 characters
- No duplicates across the entire library
- No jargon (avoid: "Test", "Default", "Untitled", "New Preset")
- No engine names in preset names (the engine is metadata, not identity)

## Check 3: DNA Coverage & Distribution

- Every preset must have all 6 DNA values filled (no nulls, no zeros across all 6)
- Check for unrealistic DNA (all values at 0.5 = likely auto-filled, not hand-tuned)
- Check mood distribution: each mood should have reasonable coverage
- Flag extreme clustering (many presets with near-identical DNA)

## Check 4: Parameter Validity

- Parameter keys must use the correct prefix for their engine (e.g., OddfeliX params use `snap_`)
- Parameter values should be within declared ranges (0.0-1.0 for normalized params)
- Cross-reference: engine listed in `engines` array must have corresponding entry in `parameters`
- No orphaned parameters (params for engines not listed in `engines`)

## Check 5: Macro Effectiveness

For each preset, check that macro parameters exist:
- `{prefix}_macroCharacter` (or M1 equivalent)
- `{prefix}_macroMovement` (or M2 equivalent)
- `{prefix}_macroCoupling` (or M3 equivalent)
- `{prefix}_macroSpace` (or M4 equivalent)

Flag presets where macros are missing or set to zero depth.

## Check 6: Duplicate Detection

- Compare preset names (case-insensitive) — no duplicates allowed
- Compare DNA vectors — flag presets with Euclidean distance < 0.05 (near-identical sonic fingerprint)
- Compare parameter snapshots — flag presets with >95% identical parameter values

## Running Existing Tools

If available, also run:
```bash
python Tools/validate_presets.py
python Tools/audit_sonic_dna.py
python Tools/find_missing_dna.py
```

## Output Format

```
## Preset Audit: {SCOPE}

### Summary
| Check | Status | Issues |
|-------|--------|--------|
| Schema Compliance | PASS/FAIL | count |
| Naming | PASS/FAIL | count |
| DNA Coverage | PASS/FAIL | count |
| Parameter Validity | PASS/FAIL | count |
| Macro Effectiveness | PASS/WARN | count |
| Duplicates | PASS/FAIL | count |

**Total presets scanned:** N
**Health score:** X/100

### Issues Found
[Grouped by check, with file paths and specific problems]

### Recommendations
[Priority-ordered fixes]
```
