---
name: seance
description: Structured engine quality evaluation against the ghost panel. Use when evaluating engine quality, performing deep engine review, or running a formal seance assessment.
argument-hint: "[engine-name e.g. Onset, Ouroboros]"
---

# Synth Seance — Engine Evaluation Protocol

Conduct a formal seance evaluation of engine **$ARGUMENTS** against the ghost panel and the 6 Binding Doctrines.

## Background

Read `Docs/seance_cross_reference.md` for the full seance methodology and previous findings. 29 seances have been completed across all registered engines. This skill runs a fresh evaluation or re-evaluation.

## Setup

1. Resolve the engine name to its Source directory in `Source/Engines/`.
2. Read all `.h` files in that engine directory.
3. Read any existing seance findings for this engine in `Docs/seance_cross_reference.md`.
4. Read any synthesis guides (e.g., `Docs/{engine}_synthesis_guide.md` if it exists).
5. Count presets using this engine in `Presets/XOlokun/`.

## The Ghost Panel

The seance convenes 8 ghosts — synthesizer pioneers whose perspectives evaluate the engine:

| Ghost | Perspective | What They Judge |
|-------|-------------|-----------------|
| **Bob Moog** | Analog warmth, playability | Filter character, keyboard expression, voltage-like control |
| **Don Buchla** | West Coast, gesture, complexity | Timbral richness, modulation depth, unconventional control |
| **Dave Smith** | Polyphony, digital precision | Voice architecture, parameter control, MIDI integration |
| **Ikutaro Kakehashi** | Accessibility, musical utility | Ease of use, preset quality, musical immediacy |
| **Suzanne Ciani** | Quadraphonic, spatial, feminine | Spatial character, movement, organic flow |
| **Klaus Schulze** | Cosmic, evolutionary, temporal | Long-form evolution, breathing, temporal depth |
| **Vangelis** | Emotional expression, immediacy | Velocity response, expression, emotional impact |
| **Isao Tomita** | Orchestral, timbral, cinematic | Timbral range, orchestral utility, cinematic scope |

## Evaluation Criteria

### 1. Doctrine Compliance (6 checks)

Run the same checks as `/validate-engine` but with seance-style commentary:

- **D001: Velocity Must Shape Timbre** — Does velocity make this engine come alive?
- **D002: Modulation is the Lifeblood** — Is the modulation system rich enough?
- **D003: The Physics IS the Synthesis** — If physical modeling, is it rigorous?
- **D004: Dead Parameters Are Broken Promises** — Every knob must do something.
- **D005: An Engine That Cannot Breathe Is a Photograph** — Can it evolve autonomously?
- **D006: Expression Input Is Not Optional** — Can a performer shape it in real time?

### 2. Sonic Identity Assessment

- **What is this engine's unique voice?** What can it make that nothing else can?
- **Dry patch quality:** Do patches sound compelling before effects?
- **Character range:** How different can two presets from this engine sound?
- **Init patch quality:** Is the first sound inviting or blank?

### 3. Preset Quality Review

Sample 5-10 presets from this engine and evaluate:
- Do all 4 macros produce audible, musical change?
- Does DNA accurately describe the sonic character?
- Are preset names evocative and appropriate?
- Is there mood diversity (not all in one category)?

### 4. Coupling Potential

- What does `getSampleForCoupling()` send? Is it useful to other engines?
- Which coupling types does `applyCouplingInput()` handle?
- Are there natural pairing partners? Which existing engines complement this one?
- Do Entangled presets using this engine demonstrate compelling coupling?

### 5. Blessing Candidates

Is there anything in this engine worthy of a Blessing? A Blessing is an exceptional design achievement — something novel, something that makes the ghost panel take notice. Review the 15 existing Blessings in `CLAUDE.md` for calibration.

### 6. Debate Relevance

Does this engine touch any of the 4 ongoing debates (DB001-DB004)?
- DB001: Mutual exclusivity vs. effect chaining
- DB002: Silence as paradigm vs. accessibility
- DB003: Init patch: immediate beauty vs. blank canvas
- DB004: Expression vs. Evolution: gesture vs. temporal depth

## Output Format

```
## Seance Report: {ENGINE_NAME}

### Ghost Panel Summary
| Ghost | Score (1-10) | Key Comment |
|-------|-------------|-------------|
| Moog | X | "..." |
| Buchla | X | "..." |
| Smith | X | "..." |
| Kakehashi | X | "..." |
| Ciani | X | "..." |
| Schulze | X | "..." |
| Vangelis | X | "..." |
| Tomita | X | "..." |

**Consensus Score: X.X / 10**

### Doctrine Compliance
| Doctrine | Status | Ghost Commentary |
|----------|--------|-----------------|
| D001 | PASS/FAIL | "..." |
| D002 | PASS/FAIL | "..." |
| D003 | PASS/N/A | "..." |
| D004 | PASS/FAIL | "..." |
| D005 | PASS/FAIL | "..." |
| D006 | PASS/FAIL | "..." |

### Sonic Identity
[Assessment of unique voice, dry patch quality, character range]

### Preset Review
[Sample of 5-10 presets with macro effectiveness and DNA accuracy]

### Coupling Assessment
[Coupling potential, natural partners, Entangled preset quality]

### Blessings
[Any Blessing candidates with justification]

### Debate Relevance
[How this engine relates to DB001-DB004]

### Recommendations
[Priority-ordered improvements, with specific code/preset changes]
```
