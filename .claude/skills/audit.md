# /audit — Targeted Subsystem Health Check

Quick, focused health check for a specific XOmnibus subsystem. Lighter than /roomba, designed for rapid validation after making changes to one area.

## Usage

- `/audit engines` — Engine registration, interface compliance, accent colors
- `/audit presets` — Preset schema, engine refs, parameter prefixes, coverage
- `/audit mpe` — MPE implementation consistency across all voice engines
- `/audit build` — CMakeLists.txt sync, include paths, missing files
- `/audit params` — Parameter prefix consistency (code vs docs vs presets)
- `/audit docs` — CLAUDE.md and master spec alignment with code
- `/audit coupling` — Coupling matrix, engine compatibility, coupling presets
- `/audit all` — Equivalent to /roomba (runs everything)

## Process by Subsystem

### /audit engines

1. List all directories under Source/Engines/
2. For each engine:
   - Verify .h exists with SynthEngine interface implementation
   - Verify .cpp exists with REGISTER_ENGINE or Processor.cpp registration
   - Verify getEngineId() returns canonical O-prefix name
   - Verify getAccentColour() matches CLAUDE.md
   - Verify getMaxVoices() returns sensible value
   - Verify renderBlock() exists and processes audio
3. Check for engines in CLAUDE.md not in Source/Engines/ and vice versa
4. Report as table with pass/fail per check per engine

### /audit presets

1. Parse all .xometa files under Presets/XOmnibus/
2. Validate JSON structure (schema_version, name, mood, engines, parameters)
3. Check engine names resolve via validEngineNames/resolveEngineAlias
4. Check parameter key prefixes match expected engine prefix
5. Flag duplicates, missing descriptions, name length > 30 chars
6. Report: coverage matrix (presets per engine, per mood) + issues list

### /audit mpe

1. Read Source/Core/MPEManager.h — verify MPEVoiceExpression struct exists
2. For each engine with getMaxVoices() > 0:
   - Voice struct has `MPEVoiceExpression mpeExpression` member
   - MIDI loop extracts `msg.getChannel()` for noteOn/noteOff
   - noteOn/noteOff signatures accept midiChannel parameter
   - MPE expression initialized on voice start
   - Pitch bend applied in audio rendering (look for `pitchBendSemitones`)
3. For meta-engines (Overworld) and non-voice engines (Optic): verify skip is appropriate
4. Check XOmnibusProcessor for MPE zone manager integration
5. Report per-engine checklist table

### /audit build

1. Find all .h and .cpp files under Source/
2. Compare against CMakeLists.txt target_sources list
3. Flag missing files (exist but not in CMake)
4. Flag phantom files (in CMake but don't exist)
5. Verify include paths resolve
6. Check test build configuration matches main build

### /audit params

1. For each engine, extract actual parameter prefix from code (first param ID string)
2. Compare against CLAUDE.md table
3. Sample 10 presets per engine, check param key prefixes
4. Report three-way comparison: code | CLAUDE.md | presets

### /audit docs

1. Verify CLAUDE.md engine count matches actual
2. Verify every file in Key Files table exists
3. Verify parameter prefix table matches code
4. Verify accent color table matches code
5. Check master spec for stale claims
6. Flag docs referencing deprecated engine names without aliases

### /audit coupling

1. Read Source/Core/MegaCouplingMatrix.h
2. Verify all CouplingType enum values are handled
3. Check each engine's applyCouplingInput() handles expected types
4. Verify coupling presets reference valid engine pairs
5. Report coupling compatibility matrix

## Report Format

```markdown
## Audit: {subsystem}
**Status:** HEALTHY / {N} ISSUES

### Findings
| # | Severity | Issue | Location | Fix |
|---|----------|-------|----------|-----|
| 1 | {P0-P3} | {description} | {file:line} | {suggested fix} |

### Summary
- Checked: {N items}
- Passed: {N}
- Issues: {N} ({N} critical, {N} high, {N} medium, {N} low)
```

## When to Use

- After modifying a specific engine — `/audit engines`
- After adding/editing presets — `/audit presets`
- After adding MPE to an engine — `/audit mpe`
- After touching CMakeLists.txt — `/audit build`
- After editing CLAUDE.md — `/audit docs`
- Quick sanity check — pick the subsystem you just changed

## Notes

- Each audit is independent and fast (30-60 seconds)
- For a comprehensive sweep, use `/roomba` instead
- Audit findings include exact file paths and line numbers for quick navigation
- Severity levels: P0 (blocks runtime), P1 (data integrity), P2 (consistency), P3 (polish)
