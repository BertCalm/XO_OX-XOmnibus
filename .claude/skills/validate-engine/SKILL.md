---
name: validate-engine
description: Run quality checks on a synthesizer engine against the 6 Doctrines. Use when adding a new engine, modifying engine DSP, fixing doctrine violations, or reviewing engine quality.
argument-hint: [engine-name e.g. Onset, Ouroboros, Optic]
---

# Engine Doctrine Validator

Validate engine **$ARGUMENTS** against the 6 Binding Doctrines. Read the engine source code, then systematically check each doctrine. Report PASS/FAIL with file:line evidence.

## Setup

1. Resolve the engine name to its Source directory. Check `Source/Engines/` for a matching directory (case-insensitive). Use the engine table in `CLAUDE.md` if needed.
2. Identify the parameter prefix from `CLAUDE.md` (e.g., `snap_`, `dub_`, `onset_`).
3. Read all `.h` files in the engine directory — DSP lives inline in headers.

## D001: Velocity Must Shape Timbre

**Rule:** Velocity drives filter brightness / harmonic content — not just amplitude.

Check for:
- A filter envelope parameter (e.g., `{prefix}_filterEnvAmount`, `{prefix}_filterEnvDepth`)
- Velocity scaling the envelope depth: look for `velocity` multiplied with filter/envelope values
- The filter cutoff being modulated by this envelope (not just gain/amplitude)
- Bonus: secondary velocity→timbre paths (fold depth, PD depth, morph position, formant shift)

**FAIL if:** velocity only controls amplitude, or filter envelope exists but isn't velocity-scaled.

## D002: Modulation is the Lifeblood

**Rule:** Minimum: 2 LFOs, mod wheel/aftertouch, 4 working macros, 4+ mod matrix slots.

Check for:
- At least 2 LFO instances (search for `lfo`, `LFO`, `lfoRate`, `lfoDepth`)
- Mod wheel connection: grep for `controllerNumber == 1` or `modWheel` or `ccValues[1]`
- Aftertouch connection: grep for `isChannelPressure` or `aftertouch` or `polyAftertouch` or `PolyAftertouch`
- 4 macro parameters: `{prefix}_macroCharacter`, `{prefix}_macroMovement`, `{prefix}_macroCoupling`, `{prefix}_macroSpace` (or similar naming)
- Mod matrix with 4+ assignable slots (if applicable)

**FAIL if:** fewer than 2 LFOs, no mod wheel, no aftertouch, or macros don't produce audible change.

## D003: The Physics IS the Synthesis

**Rule:** Rigor and citation required for any physically-modeled engine.

Check for:
- Physical modeling claims in comments (waveguide, Karplus-Strong, mass-spring, modal, etc.)
- Academic citations or references to published synthesis methods
- Mathematical rigor: proper differential equation discretization, not approximate heuristics
- Only applies to engines with physical modeling — skip for purely subtractive/FM/wavetable engines

**PASS by default** for non-physical engines. **FAIL if** physics claims exist without citations or rigorous math.

## D004: Dead Parameters Are Broken Promises

**Rule:** Every declared parameter must affect audio output.

Check for:
- All parameters declared in `createParameterLayout()` (grep for `std::make_unique<AudioParameterFloat>` or similar)
- Each parameter must be read in `renderBlock()` or a function called from it
- Look for parameters that are declared but never dereferenced (`->load()` or `->get()`)
- Check `attachParameters()` — every cached pointer must be used in the audio path

**FAIL if:** any parameter is declared but never read on the audio thread, or is read but doesn't affect output.

## D005: An Engine That Cannot Breathe Is a Photograph

**Rule:** Every engine needs at least one LFO with rate floor <= 0.01 Hz.

Check for:
- LFO rate parameter with minimum value <= 0.01 (look in `createParameterLayout()` for range)
- The LFO must modulate something audible (pitch, filter, amplitude, morph, etc.)
- Autonomous modulation: sound should evolve without user input

**FAIL if:** no LFO exists, or LFO rate floor is above 0.01 Hz, or LFO output is never routed.

## D006: Expression Input Is Not Optional

**Rule:** Velocity→timbre + at least one CC (aftertouch / mod wheel / expression).

Check for:
- Velocity→timbre mapping (covered by D001, but verify it exists)
- At least one of:
  - Channel pressure / aftertouch: `PolyAftertouch` integration or manual `isChannelPressure` handling
  - Mod wheel (CC1): `controllerNumber == 1` or `modWheel` handling in MIDI processing
  - Expression (CC11): `controllerNumber == 11`
- The CC must produce an audible timbral change, not just amplitude

**Exception:** OPTIC is exempt as a visual-only engine (B005 Zero-Audio Identity).

**FAIL if:** no CC input, or CC only affects amplitude.

## Output Format

```
## Engine Validation: {ENGINE_NAME}

### Summary
| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 Velocity→Timbre | PASS/FAIL | file:line |
| D002 Modulation | PASS/FAIL | file:line |
| D003 Physics Rigor | PASS/N/A | file:line |
| D004 Dead Parameters | PASS/FAIL | count |
| D005 Breathing LFO | PASS/FAIL | file:line |
| D006 Expression Input | PASS/FAIL | file:line |

**Overall: X/6 PASS**

### Detailed Findings
[Per-doctrine details with specific code references]

### Recommendations
[Actionable fixes for any FAIL results, with code snippets]
```
