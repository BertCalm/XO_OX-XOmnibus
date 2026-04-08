---
name: engine-qa-gate
description: Zero-trust engine quality gate — starts at 0/10, awards points only with verified evidence. Compiles, validates, traces, and adversarially attacks. Never declares victory.
triggers:
  - engine qa gate
  - quality gate
  - qa gate
  - full audit
  - zero trust audit
---

# Engine QA Gate

**Zero-trust quality gate for XOceanus engines. Starts at 0/10. Points awarded only with evidence.**

This skill replaces ad-hoc audit cycling. It runs ALL checks, uses automated tooling for mechanical validation, and uses adversarial agents only for judgment calls. It never declares a score — it reports a pass/fail checklist.

## Philosophy

> "The score is 0 until proven otherwise."

Every prior audit skill started optimistic and required the user to push for depth. This skill inverts the model:
- Mechanical checks (compile, param matching, preset validation) run FIRST as automated scripts
- If any mechanical check fails, the gate STOPS and reports — no point auditing DSP math if the code won't compile
- Adversarial review only runs AFTER all mechanical checks pass
- The adversarial review is maximally hostile by default — no "standard" or "moderate" mode
- Findings are logged as GitHub issues BEFORE fixes are attempted
- Every fix is re-verified by re-running the mechanical checks

## Gate Stages (sequential — each must pass before the next runs)

### Stage 0: BUILD
Run: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build 2>&1`
Pass criteria: Zero errors. Warnings are noted but don't block.
If fail: STOP. Report errors. Fix before proceeding.

### Stage 1: PARAM PARITY
Run: `python3 Tools/validate_obrix_presets.py` (or equivalent for the target engine)
Also: grep count `params.push_back` in the engine vs grep count unique `obrix_*` IDs in the panel
Pass criteria:
- Zero ghost parameters in any preset
- Zero out-of-range enum values
- Zero non-binary values in binary params
- Engine param count == UI attachment count

### Stage 2: MECHANICAL DSP CHECKS
Automated grep-based checks (no AI judgment needed):
- `grep '(void)' Engine.h` — zero void casts of DSP values
- `grep 'std::pow\|std::exp\|std::log\|std::sin\|std::cos' Engine.h` inside renderBlock scope — zero libm transcendentals on audio thread (fastMath equivalents required)
- `grep 'new \|delete \|malloc\|push_back\|std::string\|juce::String' Engine.h` inside renderBlock — zero allocations
- `grep 'ScopedNoDenormals' Engine.h` — present at top of renderBlock
- `grep 'flushDenormal' Engine.h` — present on all feedback/filter state writes
- `grep 'isAllNotesOff\|isAllSoundOff' Engine.h` — handled
- `grep 'isPitchWheel' Engine.h` — handled
- `grep 'getControllerNumber.*64' Engine.h` — sustain pedal handled
- `grep 'getControllerNumber.*1' Engine.h` — mod wheel handled
- Check `sr <= 0` guard at top of renderBlock

### Stage 3: SIGNAL PATH TRACE
AI agent traces the render loop and verifies:
- Every `if (procType == X)` has all 6 cases (0-5) accounted for
- Wavefolder/RingMod apply to the correct signal at the correct stage
- Every coupling accumulator is: (a) atomic, (b) consumed in renderBlock, (c) zeroed in reset(), (d) zeroed in silence gate bypass
- Every voice field written in noteOn is read in renderBlock or initVoice
- Every voice field is zeroed in reset()

### Stage 4: ADVERSARIAL DSP REVIEW
Two parallel agents (Devil's Advocate style):
- Agent A: finds the catastrophic failure scenario for every code path
- Agent B: verifies the math — filter coefficients, wavefolder formula, ADSR curves

### Stage 5: PRESET QUALITY REVIEW
Invoke `/preset-audit-checklist` on a sample of 10 presets across moods.
Check: Do presets use OBRIX-specific features (reef ecology, harmonic field, coupling)?
Flag presets that are generic subtractive patches with a reef label.

### Stage 6: UI CONSISTENCY
- Every param has a knob/combo/button
- Conditional visibility works (proc3Fb hidden for Wavefolder/RingMod)
- Labels are correct (no inversions like the fieldPolarity bug)
- Touch targets ≥ 44pt
- A11y setup on all controls

### Stage 7: DOCUMENTATION ACCURACY
- Header comments match actual param count
- Signal path description matches actual code
- Engine ID matches PresetManager registry
- CLAUDE.md param prefix table is current

## Output Format

```
╔══════════════════════════════════════════════════
  ENGINE QA GATE: [Engine Name]
╔══════════════════════════════════════════════════

  Stage 0: BUILD           [PASS/FAIL] (evidence: build output)
  Stage 1: PARAM PARITY    [PASS/FAIL] (evidence: script output)
  Stage 2: MECHANICAL DSP  [PASS/FAIL] (evidence: grep results)
  Stage 3: SIGNAL PATH     [PASS/FAIL] (evidence: trace report)
  Stage 4: ADVERSARIAL DSP [PASS/FAIL] (evidence: DA findings)
  Stage 5: PRESET QUALITY  [PASS/FAIL] (evidence: audit checklist)
  Stage 6: UI CONSISTENCY  [PASS/FAIL] (evidence: grep + manual)
  Stage 7: DOCUMENTATION   [PASS/FAIL] (evidence: grep + diff)

  GATE RESULT: [PASS/FAIL]
  Score: [count of passing stages]/8
  Blocking items: [list of FAIL stage details]
╚══════════════════════════════════════════════════
```

## Key Differences from QDD

| QDD (old) | QA Gate (new) |
|-----------|---------------|
| Starts at 10, subtracts | Starts at 0, adds |
| Escalating strictness (user pushes) | Maximum strictness by default |
| AI reads code for mechanical checks | Scripts + grep for mechanical checks |
| Single pass, then "review harder" | Sequential stages, each must pass |
| Scores are opinions | Scores are evidence counts |
| Findings fixed inline | Issues logged first, then fixed |
| Compile optional | Compile is Stage 0 gate |
| Presets checked last (if ever) | Presets checked by script first |

## When to Use

- After any engine modification (DSP, params, coupling, presets)
- Before any commit that touches an engine
- Before any release or version tag
- When the user says "qa gate", "full audit", "quality gate"
- Invoke: `/engine-qa-gate [engine-name]`
