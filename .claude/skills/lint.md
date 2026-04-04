# /lint — Code Quality & Static Analysis

Run static analysis, format checks, and architecture rule validation on the codebase.

## Usage

- `/lint` — Run all checks
- `/lint format` — Check formatting only (clang-format)
- `/lint static` — Static analysis only (cppcheck/clang-tidy)
- `/lint arch` — Architecture rule validation only
- `/lint presets` — Preset schema validation only
- `/lint --fix` — Auto-fix what's fixable

## Process

### 1. Architecture Rule Checks (always run)

These are XOceanus-specific invariants from CLAUDE.md. Verify by code inspection:

**Audio thread safety:**
- Grep for `new `, `malloc`, `calloc` in audio path files (SynthEngine implementations, processBlock)
- Grep for file I/O (`fopen`, `ifstream`, `ofstream`) in audio path files
- Flag any allocations or blocking I/O in render methods

**Parameter ID conventions:**
- Scan all engine headers for parameter ID strings
- Verify they use the correct prefix per engine (snap_, morph_, dub_, etc.)
- Flag any parameter IDs that don't match the frozen prefix table

**DSP header rule:**
- Verify all files in `Source/DSP/` and `Source/Engines/*/` headers contain inline implementations
- Verify `.cpp` files are stubs (registration only, < 20 lines)

**Denormal protection:**
- Check that feedback paths and filter implementations use `flushDenormals()` or equivalent
- Flag any IIR filter or feedback loop without denormal protection

### 2. Format Check (if clang-format available)

```bash
find Source/ -name "*.h" -o -name "*.cpp" | head -20 | xargs clang-format --dry-run --Werror
```

If clang-format is not installed, skip with note: "Install clang-format for formatting checks."

If `--fix` is set:
```bash
find Source/ -name "*.h" -o -name "*.cpp" | xargs clang-format -i
```

### 3. Static Analysis (if cppcheck available)

```bash
cppcheck --enable=warning,performance,portability --std=c++17 \
  --suppress=missingInclude -I Source/ -I Libs/JUCE/modules/ \
  Source/ 2>&1
```

If cppcheck is not installed, skip with note: "Install cppcheck for static analysis."

### 4. Preset Validation

```bash
python3 Tools/validate_presets.py --strict
```

If `--fix` is set:
```bash
python3 Tools/validate_presets.py --fix
```

### 5. Report

```markdown
## Lint Report
- **Status:** CLEAN / {N} issues found

### Architecture Rules
| Rule | Status | Issues |
|------|--------|--------|
| No audio-thread allocation | ✓/✗ | {details} |
| Parameter ID prefixes | ✓/✗ | {details} |
| DSP inline headers | ✓/✗ | {details} |
| Denormal protection | ✓/✗ | {details} |

### Formatting
- {N} files checked, {N} need formatting (or "clang-format not installed")

### Static Analysis
- {N} warnings, {N} performance, {N} portability (or "cppcheck not installed")

### Preset Validation
- {N} presets checked, {N} issues (or "all valid")

### Suggested Fixes
- {Actionable items, ordered by severity}
```

## Primitives Used
- `validate-fix-loop` — check → fix (if --fix) → re-check
- `chain-pipeline` — arch → format → static → presets → report

## Notes
- Architecture rule checks always run — they don't require external tools
- Format and static analysis are opportunistic — they report "not installed" gracefully
- Never auto-fix architecture violations — those require human judgment
- Preset validation uses the existing `validate_presets.py` tool
