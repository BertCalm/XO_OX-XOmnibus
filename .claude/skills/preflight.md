# /preflight — Pre-Commit Quality Gate

Run the full quality chain before committing or opening a PR. Catches issues early.

## Usage

- `/preflight` — Run full chain: lint → build → test → benchmark compare
- `/preflight quick` — Lint + build only (skip tests and benchmarks)
- `/preflight --fix` — Auto-fix lint issues, then run full chain

## Process

This skill chains other skills in sequence with gates between each step.
A gate failure stops the pipeline and reports what needs fixing.

### Pipeline

```
lint (arch rules + format + presets)
  → GATE: zero architecture violations?
    → YES: continue
    → NO: stop, report violations

build (cmake configure + build + verify artifacts)
  → GATE: build succeeded and artifacts exist?
    → YES: continue
    → NO: stop, report build errors

test (run all test suites)
  → GATE: all tests pass?
    → YES: continue
    → NO: stop, report failures

benchmark --compare (if baseline exists)
  → GATE: no regressions > 25%?
    → YES: continue
    → NO: warn (don't block — performance is advisory)
```

### Quick Mode

Only runs:
```
lint (arch rules only — no external tools needed)
  → GATE: clean?
build
  → GATE: succeeded?
```

### Report

```markdown
## Preflight Report
- **Mode:** full / quick
- **Result:** READY TO COMMIT / BLOCKED

### Pipeline Results
| Step | Status | Duration | Notes |
|------|--------|----------|-------|
| Lint | ✓/✗ | {time} | {issue count or "clean"} |
| Build | ✓/✗ | {time} | {artifact status} |
| Test | ✓/✗ | {time} | {pass/fail count} |
| Benchmark | ✓/⚠/— | {time} | {regression status or "no baseline"} |

### Blockers (if any)
1. {What failed and what to do about it}

### Warnings (non-blocking)
- {Advisory items like benchmark regressions or missing optional tools}
```

## Primitives Used
- `chain-pipeline` — lint → build → test → benchmark with gates
- `validate-fix-loop` — within lint step if --fix is set

## When to Use
- Before every commit that touches Source/ or Tests/
- Before opening a PR
- After resolving merge conflicts
- After pulling significant upstream changes

## Notes
- Quick mode is for rapid iteration — use full mode before PRs
- Benchmark gate is advisory (warn, don't block) because perf varies by machine
- If a gate fails, the report tells you exactly what to fix — no guessing
- This skill invokes /lint, /build, /test, and /benchmark internally
