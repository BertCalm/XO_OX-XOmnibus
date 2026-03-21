# XOmnibus Build Sentinel Report — 2026-03-20

**Date:** 2026-03-20
**Engineer:** Claude Code (Build Sentinel)
**Engine count:** 42

---

## Build Result: PASS

### Plugin Build (AU)
- **cmake configure:** PASS — no errors, Rive/Oscar correctly reported as absent
- **cmake --build (XOmnibus_AU):** PASS — 75 of 75 steps complete
- **AU installed to:** `~/Library/Audio/Plug-Ins/Components/XOmnibus.component`
- **Warnings:** 2 minor JUCE-internal warnings (`juce_NSViewComponentPeer_mac.mm` unused variable) — pre-existing, not engine-related

### auval Result: PASS
```
auval -v aumu Xomn XoOx
AU VALIDATION SUCCEEDED.
```
All render tests passed: 22050 Hz / 96000 Hz / 48000 Hz / 192000 Hz / 11025 Hz / 44100 Hz.
MIDI, parameter setting, and ramped scheduling all passed.

---

## New Engine Registration Audit: ALL 3 PASS

### ORBWEAVE (Kelp Knot Purple `#8E4585`, prefix `weave_`)

| Check | Result |
|-------|--------|
| Engine header (`OrbweaveEngine.h`) | PRESENT — topological knot phase coupling, 4 strands |
| `addParameters()` defined | PASS — `OrbweaveEngine::addParameters()` at line 629 |
| `addParameters()` called in Processor | PASS — line 328 of `XOmnibusProcessor.cpp` |
| Factory registered in Processor | PASS — `registered_Orbweave` static registration (line 211) |
| Prefix map in PresetManager | PASS — `"Orbweave" → "weave"` (line 131) |
| Engine listed in PresetManager engine array | PASS — line 30 |
| Accent color in Editor | PASS — `juce::Colour(0xFF8E4585)` (line 109) |
| In CMakeLists.txt sources | PASS — lines 227–228 |

### OVERTONE (Spectral Ice `#A8D8EA`, prefix `over_`)

| Check | Result |
|-------|--------|
| Engine header (`OvertoneEngine.h`) | PRESENT — continued fraction spectral additive, 8 partials |
| `addParameters()` defined | PASS — `OvertoneEngine::addParameters()` at line 379 |
| `addParameters()` called in Processor | PASS — line 329 of `XOmnibusProcessor.cpp` |
| Factory registered in Processor | PASS — `registered_Overtone` static registration (line 216) |
| Prefix map in PresetManager | PASS — `"Overtone" → "over"` (line 132) |
| Engine listed in PresetManager engine array | PASS — line 30 |
| Accent color in Editor | PASS — `juce::Colour(0xFFA8D8EA)` (line 110) |
| In CMakeLists.txt sources | PASS — lines 229–230 |
| SilenceGate dependency | PASS — `Source/DSP/SRO/SilenceGate.h` exists |

### ORGANISM (Emergence Lime `#C6E377`, prefix `org_`)

| Check | Result |
|-------|--------|
| Engine header (`OrganismEngine.h`) | PRESENT — 16-cell 1D cellular automaton, Wolfram rules 0–255 |
| `addParameters()` defined | PASS — `OrganismEngine::addParameters()` at line 327 |
| `addParameters()` called in Processor | PASS — line 330 of `XOmnibusProcessor.cpp` |
| Factory registered in Processor | PASS — `registered_Organism` static registration (line 221) |
| Prefix map in PresetManager | PASS — `"Organism" → "org"` (line 133) |
| Engine listed in PresetManager engine array | PASS — line 30 |
| Accent color in Editor | PASS — `juce::Colour(0xFFC6E377)` (line 111) |
| In CMakeLists.txt sources | PASS — lines 231–232 |
| SilenceGate dependency | PASS — `Source/DSP/SRO/SilenceGate.h` exists |

---

## Test Suite Status: KNOWN FAILURES (pre-existing, not new-engine-related)

The `XOmnibusTests` target has 2 pre-existing failures that do NOT affect the plugin build:

1. **`Tests/ExportTests/XPNExportTests.cpp`** — `fatal error: 'Export/XOriginate.h' file not found`
   - `XOriginate.h` was never created; test references a planned but unimplemented file
   - Pre-existing: unrelated to ORBWEAVE/OVERTONE/ORGANISM

2. **`Tests/DoctrineTests/DoctrineTests.cpp`** — 4x `invalid range expression of type 'juce::AudioProcessorValueTreeState::ParameterLayout'`
   - JUCE API mismatch: `ParameterLayout` is not range-iterable in this JUCE version
   - Pre-existing: unrelated to new engines

These test failures require separate remediation and are tracked outside this sentinel.

---

## Parameter Freeze Declaration

**ORBWEAVE, OVERTONE, and ORGANISM parameter IDs are now FROZEN.**

The plugin compiled, linked, and passed auval with all three engines contributing to the parameter tree. Any change to parameter IDs will break preset compatibility.

Frozen parameter prefixes:
- ORBWEAVE: `weave_` (e.g., `weave_knotType`, `weave_braidDepth`, `weave_macroDepth`)
- OVERTONE: `over_` (e.g., `over_cfConstant`, `over_cfDepth`, `over_macroDepth`)
- ORGANISM: `org_` (e.g., `org_ruleSet`, `org_stepRate`, `org_macroRule`)

---

## Engine Count Confirmation

**44 engines registered** in `Source/XOmnibusProcessor.cpp` and listed in `Source/Core/PresetManager.h`.
*(OXBOW and OWARE added 2026-03-20, pushing fleet from 42 → 44. This sentinel was written before those additions.)*

All 44 engines present in `CLAUDE.md` engine table match the registration in code.

---

## Recommendations

1. Fix `Tests/ExportTests/XPNExportTests.cpp` — create `Export/XOriginate.h` stub or remove the include
2. Fix `Tests/DoctrineTests/DoctrineTests.cpp` — replace range-for on `ParameterLayout` with APVTS parameter list iteration
3. Schedule seances for ORBWEAVE, OVERTONE, ORGANISM — flagged in MEMORY.md pending audit queue
4. ORBWEAVE, OVERTONE, ORGANISM are eligible for preset development (Wave 4 equivalent)
