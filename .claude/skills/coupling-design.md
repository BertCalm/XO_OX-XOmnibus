# /coupling-design — Cross-Engine Modulation Route Design

Guided workflow for designing modulation routes through the MegaCouplingMatrix. 15 coupling types × 20 engines = massive design space — this skill navigates it.

## Usage

- `/coupling-design {engineA} {engineB}` — Design coupling between two specific engines
- `/coupling-design explore {engine}` — Show all viable coupling routes for an engine
- `/coupling-design preset {presetName}` — Analyze and suggest improvements for a preset's coupling
- `/coupling-design matrix` — Visualize the full coupling compatibility matrix
- `/coupling-design wild` — Generate unexpected/experimental coupling combinations

## Core Concepts

### MegaCouplingMatrix

The coupling matrix defines how engines modulate each other. It's not just "connect A to B" — the coupling *type* determines the modulation character.

### 12 Coupling Types

Reference `Source/Core/MegaCouplingMatrix.h` for the authoritative list. Common types include:

| Type | Character | Best For |
|------|-----------|----------|
| Frequency mod | Metallic, bell-like, harsh | Adding inharmonic content |
| Amplitude mod | Tremolo, gating, rhythmic | Movement and rhythm |
| Ring mod | Sidebands, atonal | Aggressive textures |
| Phase mod | Warm FM-like, complex | Rich harmonic content |
| Filter mod | Wah, sweeps, formant | Timbral movement |
| Waveshape mod | Distortion morphing | Character transformation |
| Grain mod | Granular parameter control | Texture manipulation |
| Pitch mod | Vibrato, detuning, intervals | Pitch effects |
| Envelope mod | Dynamic shaping | Articulation transfer |
| Spectral mod | Frequency-domain coupling | Spectral morphing |
| Chaos mod | Nonlinear, unpredictable | Experimental textures |
| Sync mod | Hard/soft sync behaviors | Classic sync sounds |

### Engine Compatibility

Not all coupling types work well between all engine pairs. Factors:
- **Source character:** What kind of signal does the source engine produce?
- **Target parameters:** What can the target engine accept as modulation?
- **Rate compatibility:** Fast modulator + slow target = different than slow + fast
- **Sonic intent:** Does this coupling serve the preset's mood and character?

## Process

### 1. Engine analysis

For each engine in the coupling pair, analyze:

```markdown
### {Engine} Coupling Profile
- **Signal type:** {oscillator / noise / granular / wavetable / etc.}
- **Best as source for:** {coupling types where this engine excels as modulator}
- **Best as target for:** {coupling types where this engine responds well}
- **Coupling parameters:** {which parameters accept external modulation}
- **Character range:** {what kinds of sounds does coupling add?}
```

Read the engine's source in `Source/Engines/{engine}/` and its sound design guide entry.

### 2. Route suggestion

Based on engine analysis, suggest coupling routes:

```markdown
### Suggested Routes: {EngineA} ↔ {EngineB}

#### Route 1: {EngineA} → {EngineB} via {CouplingType}
- **Direction:** A modulates B
- **Character:** {description of the sonic result}
- **Intensity range:** {low: subtle effect, mid: clear modulation, high: extreme}
- **Sweet spot:** {recommended intensity and parameter targets}
- **Mood fit:** {which moods this coupling supports}

#### Route 2: {EngineB} → {EngineA} via {CouplingType}
- **Direction:** B modulates A
- **Character:** {different because direction matters}
...

#### Bidirectional: {EngineA} ↔ {EngineB}
- **Feedback potential:** {yes/no, and character if yes}
- **Stability notes:** {risk of runaway feedback, recommended limits}
```

### 3. Compatibility matrix (for `matrix` mode)

Generate a compatibility overview:

```
           OddFx  OddOs  Ovdub  Odyss  Oblong ...
OddfeliX    —      ★★★    ★★     ★★★★   ★★
OddOscar   ★★★     —      ★      ★★     ★★★
Overdub     ★★      ★      —      ★★★    ★★
...
```

★ rating = number of high-quality coupling routes between the pair.

### 4. Preset coupling analysis (for `preset` mode)

Load the preset and evaluate its coupling:

```markdown
### Coupling Analysis: "{preset name}"
- **Active engines:** [{engines}]
- **Current routes:** {count}

| Source | Target | Type | Intensity | Quality | Suggestion |
|--------|--------|------|-----------|---------|------------|
| {eng} | {eng} | {type} | {0-1} | Good/OK/Weak | {keep/adjust/replace} |

### Unused Potential
| Source | Target | Type | Why It Would Work |
|--------|--------|------|------------------|
| {eng} | {eng} | {type} | {description} |

### Suggestions
1. {Add/modify/remove route — description and rationale}
2. ...
```

### 5. Wild mode

For experimental/unexpected combinations:
- Pair engines that aren't typically coupled
- Use unusual coupling types (chaos mod, spectral mod)
- Suggest bidirectional feedback loops (with stability warnings)
- Push intensity into extreme ranges
- Cross mood boundaries (atmospheric engine coupled aggressively)

Flag all wild suggestions with stability notes and recommended safety limits.

### 6. Design output

```markdown
## Coupling Design: {description}
- **Engines:** {list}
- **Routes:** {count}
- **Character:** {one-line sonic description}
- **Mood affinity:** {which moods this coupling design supports}

### Route Map
{ASCII or description of signal flow}

### Parameter Settings
| Route | Source Param | Target Param | Coupling Type | Intensity | Notes |
|-------|-------------|-------------|---------------|-----------|-------|
| 1 | {param} | {param} | {type} | {0-1} | {notes} |

### Macro Integration
- M3 (COUPLING macro) should control: {which route intensities}
- Other macro interactions: {M1-CHARACTER, M2-MOVEMENT effects on coupling}
```

## Primitives Used
- None directly — this is a design/analysis skill (read-only unless generating preset modifications)

## Relationship to Other Skills
- Fed by: `/research` (understand engine capabilities), `/feature-review` (evaluate coupling implementation)
- Feeds into: `/preset-breed` (coupling routes for offspring), `/recommend` (coupling improvements)
- Validates with: `/debug-audio` (stability of feedback routes), `/preset-qa` (coupling in presets)

## Notes
- Coupling is what makes XOlokun unique — it's where "engines couple, collide, and mutate"
- Direction matters: A→B ≠ B→A for most coupling types
- Bidirectional coupling can create feedback — always note stability limits
- The COUPLING macro (M3) should be the primary user control for coupling intensity
- Denormal protection is critical in feedback coupling paths
- Engine hot-swap (50ms crossfade) affects coupling — routes are broken/reformed during swap
- Wild mode is where the most interesting sounds live, but also the most crashes
