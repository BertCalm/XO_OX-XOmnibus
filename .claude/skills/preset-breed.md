# /preset-breed — Sonic DNA Breeding & Preset Generation

Creative workflow for breeding, interpolating, and mutating presets using 6D Sonic DNA. Wraps the Python tools in `Tools/` into a guided creative process.

## Usage

- `/preset-breed {presetA} {presetB}` — Breed two presets together
- `/preset-breed mutate {preset}` — Random mutation of an existing preset
- `/preset-breed interpolate {presetA} {presetB} --steps 5` — Generate interpolation series
- `/preset-breed mood {mood}` — Generate a preset optimized for a target mood
- `/preset-breed batch --parents {mood} --count 10` — Batch breed within a mood category

## Core Concepts

### 6D Sonic DNA

Every preset has a DNA signature — six dimensions (0.0–1.0):

| Dimension | What It Represents |
|-----------|-------------------|
| **Brightness** | Spectral energy distribution (dark ↔ bright) |
| **Warmth** | Harmonic richness and saturation (cold ↔ warm) |
| **Movement** | Temporal variation and modulation (static ↔ animated) |
| **Density** | Layer count and spectral fill (sparse ↔ thick) |
| **Space** | Reverb, delay, spatial width (dry ↔ vast) |
| **Aggression** | Distortion, noise, edge (gentle ↔ aggressive) |

### Mood Categories & DNA Ranges

| Mood | Typical DNA Profile |
|------|-------------------|
| **Foundation** | Low movement, moderate density, low space |
| **Atmosphere** | High space, low aggression, moderate warmth |
| **Entangled** | High density, high movement, high coupling |
| **Prism** | High brightness, moderate movement, varied |
| **Flux** | High movement, varied density, moderate aggression |
| **Aether** | High space, low density, high brightness |

## Process

### 1. Parent selection

If breeding two presets:
- Load both parent `.xometa` files
- Display their Sonic DNA side by side
- Show their engine configurations (which engines active, which slots)
- Highlight compatible and conflicting traits

```markdown
### Parent Comparison
| Dimension | Parent A | Parent B | Delta |
|-----------|----------|----------|-------|
| Brightness | 0.7 | 0.3 | 0.4 |
| Warmth | 0.5 | 0.8 | 0.3 |
| Movement | 0.2 | 0.9 | 0.7 ← high divergence |
| Density | 0.6 | 0.5 | 0.1 |
| Space | 0.4 | 0.7 | 0.3 |
| Aggression | 0.1 | 0.6 | 0.5 ← high divergence |

**Engine slots:**
- A: [OddfeliX, Odyssey, Opal, —]
- B: [Overdub, Oblong, —, Onset]
- Shared engines: none → full hybrid potential
```

### 2. Breeding strategy

Choose how to combine DNA:

| Strategy | Description |
|----------|-------------|
| **Crossover** | Take dimensions from each parent (e.g., brightness from A, warmth from B) |
| **Blend** | Average each dimension (weighted or equal) |
| **Dominant** | One parent provides base, other provides accents |
| **Mutant** | Breed then randomly perturb 1-2 dimensions |

For engine slot breeding:
- If parents share engines → blend parameters
- If parents use different engines → hybrid (mix engine selections)
- Coupling routes: merge compatible routes, drop conflicting ones

### 3. Parameter generation

For each engine in the offspring:
- Interpolate parameter values between parents (if both use that engine)
- Carry forward from the donating parent (if only one uses that engine)
- Macro assignments: blend M1-M4 targets, ensuring all four produce audible change

**Coupling generation:**
- Inherit coupling routes that both parents support
- For novel engine combinations, suggest coupling types based on engine compatibility
- Coupling intensity = average of parents (adjustable)

### 4. DNA validation

Verify the offspring's DNA falls in a valid range:
- All dimensions 0.0–1.0
- DNA matches the target mood profile (if mood was specified)
- No extreme clustering (all dimensions at same value = boring)
- Movement > 0 (static presets need at least subtle animation)

### 5. Naming

Generate an evocative name following preset conventions:
- 2-3 words, max 30 characters
- No jargon, no engine names, no technical terms
- Mood-appropriate tone
- No duplicates against existing factory presets in `Presets/XOlokun/`

### 6. Output

Write the offspring `.xometa` file:
- Place in appropriate mood folder: `Presets/XOlokun/{mood}/`
- Include full Sonic DNA
- Include lineage metadata (parent names)
- Include macro assignments
- Include coupling configuration

```markdown
### Breeding Result
- **Name:** "{offspring name}"
- **Mood:** {mood}
- **Parents:** "{parentA}" × "{parentB}"
- **Strategy:** {crossover/blend/dominant/mutant}
- **DNA:** B{n} W{n} M{n} D{n} S{n} A{n}
- **Engines:** [{slot1}, {slot2}, {slot3}, {slot4}]
- **File:** Presets/XOlokun/{mood}/{filename}.xometa
```

### 7. Batch mode

For `--count N`:
- Generate N offspring with varied strategies
- Distribute across breeding strategies (not all the same)
- Verify no duplicate names
- Verify DNA diversity (offspring should differ from each other)
- Run `/preset-qa` on all generated presets

## Python Tools Integration

Leverage existing tools in `Tools/`:
- DNA calculation and comparison utilities
- Breeding/interpolation algorithms
- Mood classification
- Batch processing pipelines

Invoke via: `python3 Tools/{script}.py {args}`

## Primitives Used
- **bulk-transform** — Batch preset generation and validation
- **validate-fix-loop** — Generate preset, validate with `/preset-qa`, fix issues

## Relationship to Other Skills
- Validates with: `/preset-qa` (every bred preset must pass QA)
- Related: `/research` (for understanding mood profiles and DNA ranges)
- Related: `/coupling-design` (for novel coupling routes in hybrid offspring)

## Notes
- Bred presets must meet the same quality bar as hand-crafted presets
- "Dry patches must sound compelling before effects" — don't breed by stacking space/effects
- Macro assignments are critical: M1-M4 must all produce audible change
- High-divergence parents produce the most interesting offspring but also the most failures
- Interpolation series are great for understanding the space between two sounds
- Always run `/preset-qa` on results — breeding can produce invalid configurations
