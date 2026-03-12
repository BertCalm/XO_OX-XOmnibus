# /refactor — Safe Structured Refactoring

Plan and execute refactoring with pre/post validation. Ensures behavior is preserved while structure improves.

## Usage

- `/refactor {target}` — Refactor a specific file, module, or pattern
- `/refactor extract {what} from {where}` — Extract function/class/module
- `/refactor rename {old} {new}` — Rename with full codebase propagation
- `/refactor inline {target}` — Inline an unnecessary abstraction
- `/refactor --plan-only` — Generate the refactoring plan without executing

## Process

### 1. Baseline snapshot

Before any change, capture the current state:

**Behavioral baseline:**
- Run `/test` — record pass/fail state of every test
- Run `/benchmark` — record performance numbers
- Run `/lint` — record current violations (so we don't confuse pre-existing issues with regressions)
- Run `/debug-audio` if DSP is involved — record audio health

**Structural baseline:**
- Map the dependency graph of the target (what depends on it, what it depends on)
- Count references (how many callers, how many files)
- Note the public API surface (what's visible outside the module)

### 2. Refactoring plan

Classify the refactoring type and generate a plan:

**Common refactoring types:**

| Type | Description | Risk |
|------|-------------|------|
| Extract | Pull code into a new function/class/file | Low |
| Inline | Collapse unnecessary abstraction | Low |
| Rename | Change name across codebase | Medium (param ID freeze!) |
| Move | Relocate to better home | Medium |
| Restructure | Change internal organization | Medium |
| Interface change | Modify public API | High |
| Pattern migration | Change a repeated pattern across codebase | High |

**Plan format:**

```markdown
## Refactoring Plan: {description}
- **Type:** {type}
- **Target:** {file/module/pattern}
- **Risk:** Low / Medium / High

### Motivation
{Why this refactoring improves the code}

### Steps
1. {First atomic change}
2. {Second atomic change}
3. ...

### Files Affected
| File | Change Type | Lines |
|------|------------|-------|
| {path} | modify/create/delete | ~{n} |

### Invariants to Preserve
- {Behavior that must not change}
- {API contract that must hold}
- {Performance characteristic that must hold}

### Risks
- {What could go wrong}
- {Mitigation strategy}

### Validation
- [ ] All tests pass (same results as baseline)
- [ ] Benchmarks within tolerance of baseline
- [ ] Lint violations ≤ baseline count
- [ ] No new compiler warnings
```

### 3. Execute (unless --plan-only)

**Execution rules:**
- Make one atomic change at a time
- After each change, verify the code still compiles
- After every 2-3 changes, run `/test` to catch regressions early
- If a test breaks, revert the last change and investigate
- Never refactor and change behavior in the same commit

**XOmnibus-specific rules:**
- **NEVER** rename a parameter ID — use `/migrate` instead
- **NEVER** change DSP behavior during refactoring (separate concern)
- **NEVER** modify the `SynthEngine` interface without updating all engines
- Keep DSP inline in `.h` headers (don't extract to `.cpp`)
- Preserve `ParamSnapshot` pattern when refactoring engine code

### 4. Post-refactoring validation

Run the same checks as the baseline and compare:

```markdown
## Refactoring Results: {description}

### Behavioral Comparison
| Check | Before | After | Status |
|-------|--------|-------|--------|
| Tests | {n} pass, {n} fail | {n} pass, {n} fail | ✓ Same / ✗ Regression |
| Benchmarks | {metric} | {metric} | ✓ Within tolerance / ✗ Regressed |
| Lint | {n} violations | {n} violations | ✓ Same or better / ✗ Worse |
| Audio health | {status} | {status} | ✓ Same / ✗ Degraded |

### Structural Comparison
| Metric | Before | After | Δ |
|--------|--------|-------|---|
| Files | {n} | {n} | {+/-n} |
| Lines of code | {n} | {n} | {+/-n} |
| Public API surface | {n} symbols | {n} symbols | {+/-n} |
| Dependency depth | {n} | {n} | {+/-n} |

### Summary
{One paragraph: what improved, what's the same, any concerns}
```

### 5. Commit strategy

- One commit per logical refactoring step (not one giant commit)
- Commit messages start with `refactor:` prefix
- Each commit should independently compile and pass tests
- The final commit includes the validation comparison in its message body

## Primitives Used
- **validate-fix-loop** — Run refactoring step, validate, fix if broken, re-validate
- **bulk-transform** — For pattern migrations across many files

## Relationship to Other Skills
- Fed by: `/feature-review` (identifies structural issues), `/recommend` (prioritizes refactoring)
- Validates with: `/test`, `/lint`, `/benchmark`, `/debug-audio`
- Interacts with: `/migrate` (for anything involving parameter or engine name changes)

## Notes
- Refactoring should be behavior-preserving by definition — if behavior changes, it's a feature/fix, not a refactoring
- Small, validated steps beat big-bang rewrites every time
- When in doubt, `--plan-only` first
- Parameter IDs are NEVER refactored — they're migrated with aliases
- If the refactoring touches DSP code, always run `/debug-audio` pre and post
