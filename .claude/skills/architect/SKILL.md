---
name: architect
description: Review and approve proposed code changes before implementation. Use as a quality gate before applying fixes, enhancements, refactors, or new features to prevent conflicts, regressions, and inconsistent quality across the XOmnibus codebase.
argument-hint: [change-description e.g. "D001 velocity fix for BOB engine", "add breathing LFO to FAT"]
---

# XOmnibus Architect Review Gate

Review the proposed change **$ARGUMENTS** against XOmnibus architecture rules, DSP safety constraints, doctrine compliance, and cross-engine consistency before implementation.

## When to Invoke

- Before modifying any engine's DSP path (renderBlock, processSample, voice structs)
- Before adding new parameters, LFOs, or modulation sources
- Before changing shared DSP classes (CytomicSVF, FastMath, MegaCouplingMatrix)
- Before bulk fleet-wide fixes that touch multiple engines
- Before modifying preset schema or parameter IDs

## Review Checklist

### 1. Architecture Compliance

Read `CLAUDE.md` and verify:
- [ ] No memory allocation introduced on audio thread
- [ ] No blocking I/O introduced on audio thread
- [ ] DSP remains inline in `.h` headers (no logic in `.cpp` stubs)
- [ ] Parameter IDs are not renamed (frozen after release)
- [ ] Parameter prefix matches engine table in `CLAUDE.md`
- [ ] Engine implements `SynthEngine` interface correctly

### 2. Cross-Engine Consistency

Check that the proposed change:
- [ ] Uses the same pattern as similar engines (e.g., if adding velocity→filter, match the approach used in OPAL/OCELOT/OSPREY which are 6/6 engines)
- [ ] Does not introduce a one-off pattern that differs from fleet conventions
- [ ] Uses shared DSP utilities from `Source/DSP/` rather than engine-local reimplementations
- [ ] Reuses existing fast math functions (fastSin, fastExp, fastPow2, fastTan, fastTanh) instead of std:: equivalents in hot paths

### 3. Doctrine Impact

For each of the 6 Doctrines, assess whether the change:
- [ ] **D001** — Preserves or improves velocity→timbre mapping
- [ ] **D002** — Preserves or improves modulation infrastructure
- [ ] **D003** — Maintains physics rigor if applicable
- [ ] **D004** — Does not introduce dead parameters
- [ ] **D005** — Preserves or improves autonomous breathing
- [ ] **D006** — Preserves or improves expression input

### 4. Conflict Risk Assessment

- [ ] Check if the target files have recent uncommitted changes (`git status`)
- [ ] Check if other engines depend on modified shared code (`Source/DSP/`, `Source/Core/`)
- [ ] Verify the change does not break coupling compatibility (MegaCouplingMatrix routes)
- [ ] Verify preset compatibility — existing `.xometa` files must load unchanged

### 5. Performance Impact

- [ ] No new per-sample transcendental functions (std::pow, std::tan, std::sin, std::exp)
- [ ] New LFOs/modulators use fastSin, not std::sin
- [ ] Filter coefficient updates use fast approximations in per-sample paths
- [ ] Block-constant values are hoisted outside per-sample loops

### 6. Denormal Safety

- [ ] New feedback paths have flushDenormal() protection
- [ ] New filter states are flushed after updates
- [ ] Decay envelopes approaching zero are guarded

## Decision Framework

### APPROVE if:
- All checklist items pass
- Change is minimal and focused
- Consistent with fleet patterns

### APPROVE WITH CONDITIONS if:
- Minor issues found (suggest specific fixes)
- Missing denormal protection (easy to add)
- Could use a fast math function instead of std::

### REQUEST CHANGES if:
- Architecture rule violated (allocation on audio thread, parameter ID change)
- Cross-engine inconsistency (different approach than established fleet pattern)
- Missing doctrine compliance
- Risk of preset breakage

### REJECT if:
- Fundamentally wrong approach (would need to be reverted)
- Introduces security vulnerability or data loss risk
- Changes frozen parameter IDs
- Breaks SynthEngine interface contract

## Output Format

```
## Architect Review: {CHANGE_DESCRIPTION}

### Verdict: APPROVE / APPROVE WITH CONDITIONS / REQUEST CHANGES / REJECT

### Checklist
| Category | Status | Notes |
|----------|--------|-------|
| Architecture | PASS/FAIL | ... |
| Consistency | PASS/FAIL | ... |
| Doctrine Impact | PASS/FAIL | ... |
| Conflict Risk | PASS/FAIL | ... |
| Performance | PASS/FAIL | ... |
| Denormal Safety | PASS/FAIL | ... |

### Findings
[Specific issues with file:line references]

### Conditions / Required Changes
[If applicable — numbered list of required modifications before merge]

### Approved Scope
[Exact files and line ranges approved for modification]
```

## Fleet Reference: Engine Quality Tiers

When reviewing, use these engines as reference implementations:

**Gold standard (6/6 doctrines):** OPAL, OCELOT, OSPREY, OWLFISH, OHM, ORPHICA, OBBLIGATO, OTTONI, OLE

**Unconventional compliance (physics-based):** ORGANON (metabolic), OUROBOROS (chaotic attractor)

**Exempt:** OPTIC (visual engine — D001/D006 N/A)

Match proposed changes to the patterns in gold-standard engines whenever possible.
