---
name: export-xpn
description: Validate XPN/XPM export files for MPC compatibility. Use when exporting presets to MPC format, reviewing export code, or debugging MPC import failures.
argument-hint: "[file-or-check] e.g. Source/Export/, validate, rules"
disable-model-invocation: true
---

# XPN Export Validator

Validate XPN export pipeline and output files for MPC compatibility. The 3 critical XPM rules are **non-negotiable** — a single violation causes broken MPC imports.

## The 3 Critical XPM Rules

### Rule 1: KeyTrack = True
**Every XPM keygroup must set `KeyTrack` to `True`.**
- Samples must transpose across keyboard zones
- Without this, every key plays the same pitch
- Search export code for `KeyTrack` and verify it's always set to `True`

### Rule 2: RootNote = 0
**Every XPM keygroup must set `RootNote` to `0`.**
- This is MPC's auto-detect convention
- The MPC calculates root note from the sample filename/metadata
- Setting any other value causes pitch offset errors

### Rule 3: Empty Layer VelStart = 0
**Empty layers must have `VelStart` set to `0`.**
- Prevents ghost triggering of empty velocity layers
- Without this, silent notes fire at certain velocities, wasting voices
- Check that unused velocity layers are properly zeroed

## What to Validate

### If argument is "validate" or "rules":
Explain the 3 rules and check the export source code for compliance.

### If argument is a file path:
Read the file(s) and check for rule violations.

### Default (no argument):
1. Read all files in `Source/Export/`
2. Check each for the 3 XPM rules
3. Read any XPN-related Python tools in `Tools/` (e.g., `xpn_bundle_builder.py`, `xpn_drum_export.py`, `xpn_keygroup_export.py`, `xpn_kit_expander.py`, `xpn_packager.py`)
4. Cross-reference against `Docs/xpn_sound_shape_rendering.md`

## Export Pipeline Structure

The XPN export pipeline must run on **non-audio worker threads** (Architecture Rule). Verify:
- Export operations don't block the audio thread
- File I/O happens on background threads
- Progress callbacks use message-thread-safe mechanisms

## Validation Checklist

For each export module, verify:

```
[ ] KeyTrack always set to True (never False, never omitted)
[ ] RootNote always set to 0 (never calculated, never hardcoded to other values)
[ ] Empty layers have VelStart = 0 (not 1, not omitted)
[ ] Export runs on worker thread (not audio thread)
[ ] File paths are cross-platform safe (no hardcoded separators)
[ ] Sample rate conversion handled correctly
[ ] Bit depth preserved or properly dithered
[ ] Zone boundaries don't overlap or gap
```

## Output Format

```
## XPN Export Validation

### 3 Critical Rules
| Rule | Status | Location |
|------|--------|----------|
| KeyTrack = True | PASS/FAIL | file:line |
| RootNote = 0 | PASS/FAIL | file:line |
| VelStart = 0 (empty) | PASS/FAIL | file:line |

### Pipeline Safety
| Check | Status |
|-------|--------|
| Non-audio thread | PASS/FAIL |
| Cross-platform paths | PASS/FAIL |

### Issues Found
[Specific violations with file:line and fix]
```
