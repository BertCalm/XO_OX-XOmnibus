# Skill: /master-audit

**Invoke with:** `/master-audit`
**Status:** LIVE
**Last Updated:** 2026-03-20 | **Version:** 1.0 | **Next Review:** Monthly or on major milestone
**Purpose:** Orchestrate all XOceanus QA checks into a comprehensive fleet-wide health report. Use before releases, after major engine additions, or when you need a full picture of what needs attention.

---

## When to Use

- Pre-release QA pass
- After adding 3+ new engines
- Monthly/quarterly fleet health check
- When the user asks "what's the state of the fleet?"
- After completing a Prism Sweep round

---

## Phase 1: Code Integrity

### 1.1 Engine Registration Audit
Check that every engine in CLAUDE.md is registered in `Source/XOceanusProcessor.cpp`:

```bash
grep -c "registerEngine" Source/XOceanusProcessor.cpp
# Should equal 42 (one per engine)
```

Verify `getEngineId()` in each adapter matches the registered key (these MUST match):

```bash
grep -n "getEngineId\|registerEngine" Source/XOceanusProcessor.cpp | head -100
grep -rn "getEngineId" Source/Engines/ | grep "return " | sort
```

**Common ID mismatches to check:**
- Engine registered as "XOverlap" but returns "Overlap" (or vice versa)
- Capitalization errors: "Oceandeep" vs "OceanDeep"
- Confirm PresetManager.resolveEngineAlias() doesn't silently mask ID bugs

### 1.2 Parameter Prefix Freeze Check
Confirm no engine has renamed its parameter prefix (these are frozen):

```bash
grep -rn "parameter_prefix\|param_prefix\|PREFIX" Source/Core/PresetManager.h | head -20
```

### 1.3 Architecture Rules Spot-Check
For each of the 5 most recently modified engine headers:
- No `new`/`malloc` in `renderBlock()` or `processBlock()`
- No `std::cout`, file I/O, or mutex locks in audio thread
- `flushDenormal()` applied to all feedback/filter accumulator paths
- ParamSnapshot pattern used (all param pointers cached once per block)

---

## Phase 2: Doctrine Compliance (Per-Engine)

Run `/engine-health-check` on any engine with recent changes. For fleet-wide sweep, check each engine against D001–D006:

| Doctrine | Check Command / Method |
|----------|----------------------|
| D001 velocity→timbre | Search `velFilter\|velCutoff\|velocity.*filter` in engine header |
| D002 modulation depth | Count LFO structs; verify mod wheel + aftertouch wired |
| D003 physics rigor | Check inline citations in physically-modeled engines |
| D004 no dead params | Run `python3 Tools/validate_presets.py --check-params` |
| D005 engine breathes | Search `lfoRate.*0.01\|nr(0.01` in each header |
| D006 expression required | Search `aftertouch\|modWheel\|CC1` in each adapter |

**Fleet-wide doctrine status (as of 2026-03-20):** All 6 doctrines resolved fleet-wide.
Update this table after each audit run:

| Doctrine | Fleet Status | Violating Engines |
|----------|-------------|-------------------|
| D001 | RESOLVED | — |
| D002 | RESOLVED | — |
| D003 | N/A (physical only) | — |
| D004 | RESOLVED | — |
| D005 | RESOLVED | — |
| D006 | RESOLVED | — |

---

## Phase 3: Preset Health

### 3.1 Coverage Check
```bash
python3 Tools/audit_sonic_dna.py
# Reports: engines with < 5 presets, missing moods, DNA anomalies
```

### 3.2 Duplicate Detection
```bash
python3 Tools/validate_presets.py --check-duplicates
# Zero duplicates is the target
```

### 3.3 DNA Coverage
```bash
python3 Tools/validate_presets.py --check-dna
# All presets must have all 6 DNA dimensions populated
```

### 3.4 Mood Distribution Check
Target distribution across 8 moods (Foundation/Atmosphere/Entangled/Prism/Flux/Aether/Family/Submerged):
- No engine should have zero presets in any mood (except Family which is Constellation-only)
- Flux and Aether historically thin — flag engines with < 3 presets there

---

## Phase 4: Coupling Quality

Run `/coupling-interaction-cookbook` reference to verify:
- All Tier 1 engine pairs have at least one Entangled preset
- No Tier 4 (STUB) coupling type used in shipped presets
- Coupling amount in presets is in range 0.1–0.9 (not 0.0 or 1.0 extremes)

```bash
python3 Tools/validate_presets.py --check-coupling

# Precise STUB coupling check (avoids matching "stubborn" in preset descriptions)
grep -rl '"type".*"STUB"\|"STUB".*"type"' Presets/ 2>/dev/null
# Zero results = PASS
```

> **Note:** `grep -rl "stub" Presets/` produces false positives (matches "stubborn",
> "stubby" etc. in preset descriptions). Always use the JSON key-targeted pattern above.

---

## Phase 5: Seance Coverage

Check `Docs/seances/seance_cross_reference.md`:
- How many of 76 engines have seance verdicts?
- Any engines with score < 7.0 (candidates for improvement)?
- Any unresolved P0 bugs?

```bash
grep -c "^|" Docs/seances/seance_cross_reference.md  # count seance rows
grep "P0" Docs/seances/seance_cross_reference.md | grep -v "None\|fixed"  # open P0s
```

---

## Phase 6: Documentation Currency

Check for stale references in key docs:
```bash
# Engine count should be 42 everywhere
grep -rn "38 engines\|34 engines\|38 character\|34 of 34\|38 of 38" Docs/ Skills/ README.md CLAUDE.md

# Mood count should be 8 everywhere
grep -rn "7 mood\|7 categories" Docs/ Skills/ README.md
```

Fix any stale counts found.

---

## Phase 7: Fleet Health Report

Produce a summary report in `Docs/fleet_health_{YYYY-MM-DD}.md`:

```markdown
# XOceanus Fleet Health — {DATE}

## Summary
- Engines registered: 42 / 42
- Doctrine violations: {count} (list)
- Total presets: {count}
- Seances complete: {count} / 42
- Open P0 bugs: {count}
- Fleet health score: {score}/100

## Priority Actions
1. {highest priority fix}
2. {second priority fix}
3. {third priority fix}

## Per-Engine Status
| Engine | Presets | Seance | D-Violations | P0 |
...
```

---

## Quality Gate

Before marking master-audit complete:
- [ ] All 42 engines registered + IDs verified
- [ ] Zero D001–D006 violations (or all tracked)
- [ ] Zero duplicate presets
- [ ] 100% DNA coverage on all presets
- [ ] Fleet health report committed to `Docs/`
- [ ] CLAUDE.md engine count matches reality

---

## Related Skills

- `/engine-health-check` — per-engine doctrine check (called in Phase 2)
- `/preset-auditor` — per-preset quality check (called in Phase 3)
- `/coupling-debugger` — fix broken routes found in Phase 4
- `/sro-optimizer` — CPU optimization (run after doctrine fixes)
- `/synth-seance` — deep engine evaluation (for engines scoring < 7.0)

## Related Tools

- `Tools/audit_sonic_dna.py` — DNA coverage analysis
- `Tools/validate_presets.py` — preset schema + duplicate + coupling validation
- `Tools/migrate_presets.py` — preset engine name migration
