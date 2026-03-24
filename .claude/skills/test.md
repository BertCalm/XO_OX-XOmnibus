# /test — Run Tests & Report Results

Execute the XOlokun test suite, parse results, and report pass/fail with actionable detail.

## Usage

- `/test` — Build and run all tests
- `/test dsp` — Run only DSP stability tests
- `/test coupling` — Run only coupling matrix tests
- `/test presets` — Run only preset round-trip tests
- `/test --no-build` — Run without rebuilding (use existing binary)

## Process

### 1. Build test binary (unless --no-build)

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target XOlokunTests
```

Verify `build/XOlokunTests` exists.

### 2. Run tests

```bash
./build/XOlokunTests
```

Capture stdout, stderr, and exit code. Time the run.

### 3. Parse results

The test runner outputs lines like:
- `[PASS] TestName`
- `[FAIL] TestName: reason`
- `DSP Stability Tests: X/Y passed`
- `Coupling Tests: X/Y passed`
- `Preset Tests: X/Y passed`

Parse these into structured results.

### 4. Report

```markdown
## Test Report
- **Duration:** {time}
- **Result:** {PASS / FAIL}
- **Summary:** {passed}/{total} tests passed

### By Suite
| Suite | Passed | Failed | Status |
|-------|--------|--------|--------|
| DSP Stability | X/Y | Z | ✓/✗ |
| Coupling Matrix | X/Y | Z | ✓/✗ |
| Preset Round-Trip | X/Y | Z | ✓/✗ |

### Failures (if any)
- **TestName:** reason
  - File: `path:line`
  - Suggestion: {what to investigate}
```

### 5. On failure
- For each failed test, identify the assertion that failed
- Read the relevant test code and the source it tests
- Provide a focused diagnosis: what the test expects vs. what happened
- Suggest investigation steps (do NOT auto-fix test code)

## Primitives Used
- `chain-pipeline` — build → run → parse → report
- `validate-fix-loop` — if tests fail, diagnose and suggest (but don't auto-fix without permission)

## Test Categories Reference

| Suite | File | What It Validates |
|-------|------|-------------------|
| DSP Stability | `Tests/DSPTests/DSPStabilityTests.h` | FastMath, CytomicSVF, PolyBLEP, engine rendering (no NaN/Inf/denormal) |
| Coupling | `Tests/CouplingTests/CouplingMatrixTests.h` | MegaCouplingMatrix routing, 12 coupling types |
| Preset | `Tests/PresetTests/PresetRoundTripTests.h` | .xometa load → save → compare round-trip fidelity |

## Notes
- Tests require no audio device or GUI — pure DSP validation
- Exit code 0 = all pass, non-zero = failure count
- If the test binary doesn't exist and --no-build is set, prompt to build first
