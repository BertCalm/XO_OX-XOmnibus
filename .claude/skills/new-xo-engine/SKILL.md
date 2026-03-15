---
name: new-xo-engine
description: Walk through creating a new XOmnibus engine from concept to integration. Use when designing a new synth engine, starting a new XO instrument, or planning engine architecture.
argument-hint: "[engine-name e.g. XOstinato]"
disable-model-invocation: true
---

# New XOmnibus Engine Wizard

Guide the user through creating a new engine: **$ARGUMENTS**

Read `Docs/xomnibus_new_engine_process.md` for the full process. This skill walks through each phase interactively.

## Phase 0: Ideation — "What Exhibition?"

Ask the user these questions and help them fill in the concept brief:

### The XO_OX Concept Test
1. **What's the XO word?** Must follow XO + O-word naming convention.
2. **What's the one-sentence thesis?** "XO_____ is a _____ synth that _____"
3. **What sound can ONLY this instrument make?** If nothing unique, the concept needs work.

### Gallery Fit Check
Read the engine table in `CLAUDE.md` and show the user what sonic territory is already covered. Identify the gap this new engine fills.

### Concept Brief
Help draft:
- Name, thesis, sound family, unique capability
- Personality in 3 words
- Engine approach (subtractive / wavetable / FM / granular / physical modeling / sample / hybrid)
- Accent color idea (check existing colors in CLAUDE.md to avoid conflicts)
- Best coupling partners (at least 2 existing engines)
- Coupling types it would use (as source and target)
- Mood affinity (which of the 7 moods would presets gravitate toward)

### Decision Gate
Confirm: concept brief written, XO word feels right, gallery gap clear, 2+ coupling partners, user is excited.

## Phase 1: Architect — "Design the Art"

### Parameter Namespace
Choose a 3-5 char prefix (check `CLAUDE.md` engine table to avoid collisions):
```
{prefix}_{paramName}  e.g., ost_filterCutoff
```

### Macro Mapping
| Macro | Label | Target |
|-------|-------|--------|
| M1 | CHARACTER | [the engine's defining parameter] |
| M2 | MOVEMENT | [modulation depth / LFO rate] |
| M3 | COUPLING | [internal mod or coupling amount] |
| M4 | SPACE | [FX depth — reverb, delay, chorus] |

### Coupling Interface Design
- What does `getSampleForCoupling()` return? (usually post-filter, pre-FX mono mix)
- Which `CouplingType` enums can this engine receive?
- Any coupling types it should never receive?

### Voice Architecture
- Max voices, voice stealing strategy, legato mode

### Signal Flow
Help design the signal flow diagram. All DSP in inline `.h` headers.

### Decision Gate
Confirm: full spec, parameter list with namespaced IDs, macro mapping, coupling interface, signal flow.

## Phase 2: Sandbox Build — "Build It Independently"

### Scaffold Structure
```
Source/Engines/{ShortName}/
├── {Name}Engine.h         ← Adapter implementing SynthEngine
├── {Name}Engine.cpp       ← One-line stub: REGISTER_ENGINE({Name}Engine)
├── {Name}Voice.h          ← Voice with oscillator + filter + envelopes
├── {Name}Oscillator.h     ← Core sound generation
├── {Name}DSP.h            ← Any unique DSP modules
└── {Name}Types.h          ← Parameter IDs, constants
```

### Dual-Target Rules (enforce these)
1. DSP in `.h` headers only — `.cpp` files are one-line stubs
2. ParamSnapshot pattern — cache parameter pointers once per block
3. No UI references in DSP
4. Namespaced parameter IDs from day one
5. MIDI in, audio out — same contract as `SynthEngine::renderBlock()`

### Architecture Rules (non-negotiable)
- Never allocate memory on the audio thread
- Never perform blocking I/O on the audio thread
- Denormal protection in all feedback/filter paths (use `flushDenormal()` from `Source/DSP/FastMath.h`)
- Engine hot-swap uses 50ms crossfade

### Doctrine Compliance (build in from the start)
- D001: Velocity must shape filter brightness, not just amplitude
- D002: Min 2 LFOs, mod wheel, aftertouch, 4 working macros
- D003: Cite sources for any physical modeling
- D004: Every declared parameter must affect audio output
- D005: At least one LFO with rate floor <= 0.01 Hz
- D006: Velocity→timbre + at least one CC

### Preset Design
Create presets in `.xometa` format from day one. Minimum 20 presets before integration.

### Decision Gate
Confirm: sounds good, has personality, 20+ presets, all 4 macros audible, DSP stability passes.

## Phase 3: Integration Prep — "Frame It for the Gallery"

### Write the Adapter
Generate `{Name}Engine.h` implementing `SynthEngine` interface from `Source/Core/SynthEngine.h`.

Key methods to implement:
- `getEngineId()` → canonical engine ID
- `getAccentColour()` → accent color
- `prepare()` / `releaseResources()` / `reset()`
- `renderBlock()` — core audio, real-time safe
- `getSampleForCoupling()` — O(1) cached output
- `applyCouplingInput()` — receive modulation from other engines
- `createParameterLayout()` — namespaced parameters
- `attachParameters()` — cache raw parameter pointers

### Register
Create `{Name}Engine.cpp`:
```cpp
#include "{Name}Engine.h"
REGISTER_ENGINE({Name}Engine)
```

## Phase 4: Gallery Install — "Hang It on the Wall"

### File Placement
```
Source/Engines/{ShortName}/    ← engine source
Presets/XOmnibus/{mood}/       ← presets by mood
```

### CMakeLists.txt
Add new source files to the build.

### Update Documentation
- `CLAUDE.md` — add to engine table with accent color and parameter prefix
- `Docs/xomnibus_master_specification.md` — engine catalog

### Cross-Engine Presets
Create 5-10 Entangled mood presets coupling the new engine with existing ones.

### Verification Checklist
```
[ ] Adapter compiles in XOmnibus build
[ ] Engine appears in EngineRegistry
[ ] Single-engine presets load and play
[ ] All 4 macros produce audible change
[ ] Coupling send works (getSampleForCoupling)
[ ] Coupling receive works (applyCouplingInput)
[ ] Cross-engine presets sound compelling
[ ] DNA computed for all presets
[ ] CPU within budget (<28% dual, <55% tri)
[ ] No clicks during engine hot-swap (50ms crossfade)
[ ] Gallery docs updated
[ ] All 6 Doctrines PASS (run /validate-engine)
[ ] DSP safety audit PASS (run /dsp-safety)
```
