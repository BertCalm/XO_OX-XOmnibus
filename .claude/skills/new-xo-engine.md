# /new-xo-engine — Engine Scaffold & Integration

Walk through ideation, architecture, scaffold, and integration of a new XOlokun engine.
Referenced in CLAUDE.md. Full process documented in `Docs/xolokun_new_engine_process.md`.

## Usage

- `/new-xo-engine` — Interactive guided walkthrough (asks questions)
- `/new-xo-engine {name}` — Start with a name already chosen

## Process

### Phase 1: Ideation & Identity

Ask the user:
1. **Name** — Must follow XO + O-word convention (e.g., XOrbital, XObscura)
2. **Character** — What sonic identity? What's the one-sentence pitch?
3. **Sonic Pillars** — 3-5 defining characteristics
4. **Accent Color** — Hex color for Gallery Model UI
5. **Reference Artists/Sounds** — What should it sound like?
6. **Coupling Affinity** — Which existing engines does it pair well with?

Validate:
- Name follows O-word convention
- Name not already taken (check EngineRegistry.h)
- Color not too close to existing engine colors

### Phase 2: Architecture

Define:
1. **Parameter Prefix** — Frozen shortname (e.g., `orbital_`, `optic_`)
2. **Parameter List** — Core parameters with ranges and defaults
3. **DSP Approach** — Synthesis method, algorithm outline
4. **Coupling Types** — Which MegaCouplingMatrix coupling types it accepts/produces
5. **Macro Mappings** — Default CHARACTER/MOVEMENT/COUPLING/SPACE targets

Validate:
- Prefix not already taken (check CLAUDE.md prefix table)
- Parameters use `{prefix}_{paramName}` format
- DSP approach is feasible inline in a .h header

### Phase 3: Scaffold

Create files using `scaffold-and-register` primitive:

```
Source/Engines/{Name}/
├── {Name}Engine.h          # Full DSP implementation (inline)
├── {Name}Engine.cpp         # One-line registration stub
└── {Name}EngineParams.h     # Parameter ID constants
```

**{Name}Engine.h template:**
- Implements `SynthEngine` interface
- All DSP inline
- ParamSnapshot pattern for parameter caching
- Denormal protection on feedback/filter paths
- 50ms crossfade support for hot-swap

**{Name}Engine.cpp template:**
```cpp
#include "{Name}Engine.h"
REGISTER_ENGINE("{Name}", {Name}Engine)
```

### Phase 4: Registration & Integration

1. **Add to EngineRegistry.h** — Register in factory
2. **Add to CMakeLists.txt** — Add source files to build
3. **Add to CLAUDE.md** — Engine table, parameter prefix table
4. **Add to engine catalog** — `Docs/xo_mega_tool_engine_catalog.md`
5. **Create sound design entry** — `Docs/xolokun_sound_design_guides.md`

### Phase 5: Verification

1. **Build** — Invoke `/build` to verify compilation
2. **Test** — Invoke `/test` to verify no regressions
3. **Lint** — Invoke `/lint` to verify architecture rules
4. **Review** — Invoke `/review` on the new files

### Phase 6: Initial Presets

1. Create 5-10 Foundation presets demonstrating the engine's character
2. Create 2-3 Entangled presets showing coupling with existing engines
3. Validate with `/preset-qa`

## Primitives Used
- `scaffold-and-register` — create files → register → verify
- `chain-pipeline` — ideation → architecture → scaffold → register → verify → presets
- `validate-fix-loop` — build/test/lint verification at the end

## Rules (from CLAUDE.md)
- Parameter IDs use `{shortname}_{paramName}` format from day one
- Presets use .xometa JSON format from day one
- DSP lives in inline .h headers (portable)
- DSP works without UI references (just parameters)
- M1-M4 macros produce audible change in every preset
- Define coupling compatibility early

## Notes
- This is an interactive skill — it asks questions at each phase
- Each phase is a gate: don't proceed until the previous phase is validated
- The user can exit at any phase and resume later
- New engines are designed as standalone instruments first, then integrated
