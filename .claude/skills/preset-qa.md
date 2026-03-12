# /preset-qa — Full Preset Validation Pipeline

Comprehensive quality assurance for .xometa presets: schema, DNA, naming, coupling, parameter sanity, and mood distribution.

## Usage

- `/preset-qa` — Validate all presets
- `/preset-qa {mood}` — Validate one mood category (Foundation, Atmosphere, Entangled, Prism, Flux, Aether)
- `/preset-qa {file.xometa}` — Validate a single preset
- `/preset-qa --fix` — Auto-fix what's fixable, report what's not
- `/preset-qa --report` — Generate detailed distribution report (no fixing)

## Process

### 1. Schema Validation

```bash
python3 Tools/validate_presets.py --strict
```

Checks against `Docs/xometa_schema.json`:
- Required fields present (name, engines, parameters, macros, sonicDNA, tags)
- Field types correct
- Engine IDs are valid (canonical O-prefix names)
- Parameter keys use correct frozen prefixes

### 2. Sonic DNA Validation

```bash
python3 Tools/compute_preset_dna.py --verify
```

- All 6 dimensions present: brightness, warmth, movement, density, space, aggression
- Values in 0.0–1.0 range
- DNA is consistent with parameter values (not fabricated)

### 3. Naming Convention Check

For each preset:
- Name is 2-3 words
- Max 30 characters
- No duplicates across all moods
- No jargon or technical terms
- Evocative and descriptive

### 4. Macro Validation

For each preset:
- 4 macros defined (CHARACTER, MOVEMENT, COUPLING, SPACE)
- Each macro has a label and parameter mappings
- Macros produce audible change (mappings reference real parameters)

### 5. Coupling Validation (multi-engine presets)

For presets with 2+ engines:
- Coupling routes reference valid engines in the preset
- Coupling types are valid MegaCouplingMatrix enums
- Coupling intensity values in range

### 6. Distribution Analysis

Count presets per mood, per engine, per coupling type:

```markdown
### Mood Distribution
| Mood | Count | Target | Status |
|------|-------|--------|--------|
| Foundation | {n} | ~167 | {over/under/ok} |
| Atmosphere | {n} | ~167 | ... |
| ... | | | |

### Engine Coverage
| Engine | Presets | % of Total |
|--------|---------|------------|
| Organon | {n} | {%} |
| ... | | |

### Coupling Usage
| Type | Count |
|------|-------|
| FreqMod | {n} |
| ... | |
```

### 7. Report

```markdown
## Preset QA Report
- **Presets Scanned:** {count}
- **Status:** PASS / {N} issues

### Issues by Severity
- **Errors (must fix):** {count} — schema violations, invalid engine IDs
- **Warnings (should fix):** {count} — naming, DNA drift, macro coverage
- **Info:** {count} — distribution imbalances, coverage gaps

### Detailed Issues
| Preset | File | Issue | Severity | Auto-fixable |
|--------|------|-------|----------|-------------|
| {name} | {path} | {description} | error/warn | yes/no |

### Distribution Summary
{mood, engine, coupling tables}
```

### 8. Auto-fix (if --fix)

```bash
python3 Tools/validate_presets.py --fix
python3 Tools/apply_renames.py  # resolve legacy engine name aliases
```

Then re-validate to confirm fixes.

## Primitives Used
- `validate-fix-loop` — validate → fix → re-validate
- `chain-pipeline` — schema → DNA → naming → macros → coupling → distribution → report

## Notes
- Schema validation is the hard gate — other checks are advisory
- Auto-fix handles: legacy engine name aliases, missing DNA recomputation, field type coercion
- Auto-fix does NOT handle: naming creativity, macro mapping design, coupling route design
- Distribution analysis helps identify gaps for future preset creation
