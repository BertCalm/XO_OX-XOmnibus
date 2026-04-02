# Skill: /coupling-debugger

**Invoke with:** `/coupling-debugger`
**Status:** LIVE
**Last Updated:** 2026-03-20 | **Version:** 1.0 | **Next Review:** On new coupling type addition or MegaCouplingMatrix changes
**Purpose:** Diagnose and fix broken or inaudible coupling routes. Coupling is XOceanus's signature feature — when it doesn't work, this skill finds why.

---

## When to Use

- Coupling between two engines is set up but produces no audible effect
- A preset's Entangled coupling sounds flat/dead
- After adding a new engine and coupling doesn't fire
- CouplingType changes have no effect
- M3 (COUPLING macro) turns from 0 to 1 with no change

---

## Diagnostic Tree

Work through these phases in order. Stop when you find the root cause.

---

## Phase 1: Verify Both Engines Are Active

```
Source engine (sender) → Target engine (receiver)
```

**Check 1:** Are both engine slots occupied? Empty slots produce no audio and send no coupling signals.

**Check 2:** Is the source engine producing audio? A completely silent source (e.g., OPTIC which is a visual-only engine) cannot send an audio coupling signal. Check:
```bash
grep "getEngineId" Source/Engines/{SourceEngine}/*.h | grep "return"
# Visual-only: OPTIC — intentionally has no audio coupling output
```

**Check 3:** Is the coupling route `active = true` in the preset JSON?

```json
"coupling": {
  "routes": [
    {"source": "EngineA", "target": "EngineB", "type": "AmpToFilter", "amount": 0.4, "active": true}
  ]
}
```

If `active` is missing or `false`, the route is bypassed.

---

## Phase 2: Check Coupling Amount

The route amount must exceed the skip threshold in `MegaCouplingMatrix.h`:

```cpp
if (!route.active || route.amount < 0.001f)
    continue;  // route is skipped
```

**Common error:** Amount set to 0.0 (default) in a new preset. Set to 0.3–0.6 for audible coupling.

**Also check M3 macro mapping:** If M3 (COUPLING) is wired to coupling amount and set to 0, the route fires but at 0 strength. Set M3 to 0.5 as preset default.

---

## Phase 3: Verify CouplingType Is Not STUB

Check the coupling type in `/coupling-interaction-cookbook`:

```bash
grep -A2 "STUB\|stub" Source/Core/MegaCouplingMatrix.h
```

**STUB types** are registered but have no DSP implementation — they accept the signal but do nothing. Common STUBs per engine pair vary; check the cookbook's Tier 4 list.

**Fix:** Change to a Tier 1–3 CouplingType supported by the target engine. See the cookbook for proven pairs.

---

## Phase 4: Check Source Engine `getSampleForCoupling()`

The source engine must return non-zero values from this method:

```bash
grep -n "getSampleForCoupling" Source/Engines/{SourceEngine}/*.h
```

**Check:** Is the return value always 0.0f? Is it conditional on a parameter that's set to zero?

Common issues:
- `getSampleForCoupling()` returns `outputBuffer[ch][sample]` but the engine is in bypass
- Source engine's envelope is at 0 (note has released) — no signal to send
- Sample index out of bounds (returns 0 on guard clause)

---

## Phase 5: Check Target Engine `applyCouplingInput()`

The target engine must handle the CouplingType it receives:

```bash
grep -n "applyCouplingInput\|CouplingType::{type}" Source/Engines/{TargetEngine}/*.h
```

**Check:** Is there a `case CouplingType::{type}:` in the switch? Or does the switch fall through to a default no-op?

```cpp
case CouplingType::AmpToFilter:
    coupledFilterMod += amount * sourceAvg;  // ← this must exist and be non-zero
    break;
```

**Common issues:**
- CouplingType not in the switch → coupling is silently ignored
- `coupledFilterMod` accumulated but never applied in `renderBlock()`
- Amount multiplied by zero (parameter not read from APVTS)

---

## Phase 6: Verify `applyCouplingInput()` Effect Reaches Audio Output

Even if the signal is received, it may not reach the output:

```bash
grep -n "coupledFilterMod\|coupledPitchMod\|coupledAmpMod" Source/Engines/{TargetEngine}/*.h
# Trace the variable from applyCouplingInput() to renderBlock()
```

**Check:** Is the coupling mod variable applied inside `renderBlock()` or just stored and never read?

Pattern to look for:
```cpp
// In renderBlock():
float cutoff = params.filterCutoff + coupledFilterMod;  // ← this line must exist
coupledFilterMod = 0.0f;  // ← reset after use (or use atomic clear)
```

If the mod variable is set in `applyCouplingInput()` but never consumed in `renderBlock()`, it's a D004 violation (dead coupling input).

---

## Phase 7: Check MegaCouplingMatrix Routing

Verify the matrix is routing at all:

```bash
grep -n "fillControlRateBuffer\|applyCouplingToEngines" Source/Core/MegaCouplingMatrix.h
```

**Check:** Is `kControlRateRatio` causing the coupling signal to only update every 32 samples? This is expected behavior — coupling is control-rate, not sample-rate. But if the matrix update isn't called from `XOceanusProcessor.processBlock()`, nothing moves.

```bash
grep -n "megaMatrix\|MegaCoupling" Source/XOceanusProcessor.cpp | head -20
```

---

## Fix Patterns

| Root Cause | Fix |
|-----------|-----|
| Route `amount < 0.001` | Set amount to 0.3–0.6 in preset |
| STUB CouplingType | Switch to Tier 1–3 type (see cookbook) |
| `getSampleForCoupling()` returns 0 | Check source engine audio output path |
| `applyCouplingInput()` missing case | Add case + DSP implementation in target engine |
| Coupling mod not consumed in `renderBlock()` | Wire mod variable to filter/pitch/amp calculation |
| `active = false` in preset | Set `"active": true` |
| M3 macro at 0.0 | Set M3 default to 0.5 in preset |

---

## Quick Verification Test

After any fix, test with:
1. Load preset with known coupling (e.g., any Entangled mood preset from Docs/)
2. Slowly turn M3 from 0 to 1
3. Disable coupling (set amount to 0) → re-enable (set to 0.5) — difference must be audible
4. Try different CouplingTypes on the same route

---

## Related Skills

- `/coupling-interaction-cookbook` — find proven CouplingTypes for the engine pair
- `/coupling-preset-designer` — build a new coupling preset correctly from scratch
- `/engine-health-check` — verify both source and target engines are doctrinally sound
- `/master-audit` — fleet-wide coupling quality check

## Related Code

- `Source/Core/MegaCouplingMatrix.h` — routing + control-rate buffer
- `Source/Core/SynthEngine.h` — `getSampleForCoupling()` + `applyCouplingInput()` interface
- `Source/Core/PresetManager.h` — coupling route loading from `.xometa`
