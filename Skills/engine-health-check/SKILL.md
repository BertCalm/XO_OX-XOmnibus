# Skill: /engine-health-check

**Invoke with:** `/engine-health-check [engine-name]`
**Status:** LIVE
**Purpose:** Quick doctrine compliance check for any XOmnibus engine — covers D001–D006, coupling quality, and known bugs. Produces a pass/fail table with remediation notes.

---

## When to Use This Skill

Use this skill when:
- Adding a new engine to XOmnibus for the first time
- After any modification to an engine's DSP or adapter
- Preparing for a release or QA pass
- An engine is flagged in the seance cross-reference as having violations
- Running a Prism Sweep round on specific engines

**For a full seance (ghost council evaluation), use the `/synth-seance` skill instead.**
This skill is the fast, self-service checklist — no ghost council required.

---

## The 6 Doctrines

| ID | Name | What It Requires |
|----|------|-----------------|
| D001 | Velocity Must Shape Timbre | Velocity changes filter brightness / harmonic content — NOT just amplitude |
| D002 | Modulation is the Lifeblood | Min: 2 LFOs, mod wheel (CC1), aftertouch, 4 working macros, 4+ mod matrix slots |
| D003 | The Physics IS the Synthesis | Physical modeling engines must have rigorous citations and accurate physics |
| D004 | Dead Parameters Are Broken Promises | Every declared parameter must affect audio output |
| D005 | An Engine That Cannot Breathe Is a Photograph | At least one LFO with rate floor ≤ 0.01 Hz |
| D006 | Expression Input Is Not Optional | Velocity→timbre + at least one CC (aftertouch OR mod wheel OR expression pedal) |

---

## Phase 1: Source Code Read

Open `Source/Engines/{CanonicalName}/{CanonicalName}Engine.h` and check:

### D001 — Velocity → Timbre Check

```
Search for: velocity, noteOnVelocity, vel01, velToFilter, velFilterAmt
```

| Finding | Status |
|---------|--------|
| Velocity scales filter cutoff AND envelope depth | ✅ PASS |
| Velocity scales filter cutoff OR envelope depth | ⚠️ PARTIAL |
| Velocity scales amplitude only | ❌ FAIL |
| Velocity not wired at all | ❌ FAIL |

**Test:** Play a note at velocity 1 and velocity 127. Does the harmonic content change (not just volume)?

---

### D002 — Modulation Depth Check

```
Search for: lfo, LFO, SimpleLFO, modWheel, CC1, aftertouch, envFilterAmt
```

Minimum requirements:
- [ ] 2+ LFO objects instantiated (lfo1, lfo2 or equivalent)
- [ ] LFOs wired to at least 2 audible destinations
- [ ] Mod wheel (CC1) wired to LFO depth or filter cutoff
- [ ] Aftertouch wired to any audible parameter
- [ ] At least 4 mod matrix slots active

---

### D003 — Physics Rigor Check (only for physical modeling engines)

Applies to: OBSCURA, ORACLE, ORBITAL, OUROBOROS, OSPREY, OSTERIA, OWLFISH, ORIGAMI

```
Search for: physics, model, waveguide, modal, stiffness, resonator
Check: Are algorithm parameters cited? Are resonator ratios documented?
```

Physical modeling engines must cite their physical models. If there are magic numbers with no comment explaining them, flag for D003 review.

---

### D004 — Dead Parameter Check

```
For each parameter in createParameterLayout() / addParametersImpl():
  Search for its parameter ID string in the DSP code.
  If found only in layout and snapshot but never READ or APPLIED to DSP → DEAD.
```

Common dead parameter patterns:
```cpp
// Dead: declared but never applied
float morphGlide = *params.morphGlide;  // <- assigned but not used

// Dead: void-cast (deliberate disabling)
(void)type; (void)amount; (void)sourceBuffer; (void)numSamples;  // in applyCouplingInput
```

**Known historical dead parameters (from seance, may already be fixed):**
- ODDFELIX: `snap_macroDepth` — was a void cast
- OWLFISH: `owl_morphGlide` — was assigned but not applied
- OCELOT: all 4 macro parameters — were declared but routed to nothing
- ODYSSEY: `crossFmDepth` — declared, not applied in FM path
- OPAL: `opal_smear` — declared, no DSP

---

### D005 — Breathing Check

```
Search for: lfo.setRate, lfo1Rate, lfo2Rate, SimpleLFO, rate =
Check: Is the minimum rate parameter constrained to ≤ 0.01 Hz?
```

In JUCE parameter range:
```cpp
// PASS — floor at 0.01 Hz
juce::NormalisableRange<float>(0.01f, 20.0f, ...)

// FAIL — floor too high
juce::NormalisableRange<float>(0.1f, 20.0f, ...)  // min = 0.1 Hz (too fast)
juce::NormalisableRange<float>(1.0f, 20.0f, ...)  // min = 1 Hz (FAIL)
```

Also check: Are LFOs actually instantiated AND called in the render path? Some engines declare LFO structs but never call `.tick()`.

---

### D006 — Expression Check

**Mod wheel (CC1):**
```
Search in renderBlock() / MIDI handler:
  getControllerNumber() == 1  or  isController() + CC#1
```

| Finding | Status |
|---------|--------|
| CC1 captured AND applied to audio parameter | ✅ PASS |
| CC1 captured but stripped/ignored | ❌ FAIL |
| No CC handling at all | ❌ FAIL |

**Aftertouch:**
```
Search: isAftertouch, isChannelPressure, getAfterTouchValue, getChannelPressureValue
```

| Finding | Status |
|---------|--------|
| Aftertouch captured AND applied to audio parameter | ✅ PASS |
| Aftertouch captured but not applied | ❌ FAIL |
| No aftertouch handling | ❌ FAIL |

**Exception:** OPTIC is explicitly exempt from D006 (visual-only engine, no pitch/filter).

---

## Phase 2: Coupling Quality Check

Open `Docs/coupling_audit.md` and find the engine. Check:

| Metric | Check |
|--------|-------|
| `applyCouplingInput` status | STUB = fail; PARTIAL/FULL = pass |
| `getSampleForCoupling` status | STUB (returns 0) = fail; SCALAR/PROPER = pass |
| Coupling types implemented | 0 = fail; 1–3 = partial; 4+ = good |
| ch2+ output populated | No ch2 = limited; ch2 envelope = good |

---

## Phase 3: Known Bug Check

Check `Docs/seance_cross_reference.md` for the engine row. Look for P0 bugs:

| Bug ID | Description | Status to Verify |
|--------|-------------|-----------------|
| P0-01 | OBSIDIAN R channel filter bypass | Check right channel processes through both filters |
| P0-02 | OSTERIA warmthFilter processes mixL only | Check mixR routes through warmthFilter |
| P0-03 | ORIGAMI STFT race condition if blockSize < 512 | Check blockSize guard |
| P0-04 | OBSIDIAN formant param ID collision | Check pFormantResonance ≠ pFormantIntensity |

**Note:** These P0 bugs were identified during seance (2026-03-14). Some may already be resolved. Always cross-reference `Docs/seance_cross_reference.md` for the current open/fixed status before flagging them as active issues.

---

## Phase 4: Generate Health Report

After completing all checks, produce a health table:

```
=== ENGINE HEALTH CHECK: [ENGINE NAME] ===
Date: [date]

DOCTRINE COMPLIANCE
-------------------
D001 (Velocity→Timbre):    [✅ PASS | ⚠️ PARTIAL | ❌ FAIL] — [note]
D002 (Modulation Depth):   [✅ PASS | ⚠️ PARTIAL | ❌ FAIL] — [note]
D003 (Physics Rigor):      [✅ PASS | N/A | ❌ FAIL]        — [note]
D004 (No Dead Params):     [✅ PASS | ⚠️ WARNING | ❌ FAIL]  — [note if any dead params found]
D005 (Engine Breathes):    [✅ PASS | ❌ FAIL]              — [LFO rate floor / count]
D006 (Expression):         [✅ PASS | ⚠️ PARTIAL | ❌ FAIL] — [mod wheel: yes/no | aftertouch: yes/no]

COUPLING QUALITY
----------------
Input quality:  [STUB | PARTIAL | FULL] — [types implemented]
Output quality: [STUB | SCALAR | PROPER] — [channels]
Coupling score: [1–5]

KNOWN BUGS
----------
[None known | List any P0 bugs with status: open/fixed]

OVERALL
-------
[🟢 HEALTHY | 🟡 NEEDS ATTENTION | 🔴 CRITICAL ISSUES]

REMEDIATION PRIORITIES (if any):
1. [highest priority fix]
2. [next fix]
```

---

## Remediation Quick Reference

| Violation | Fastest Fix |
|-----------|------------|
| D001: velocity amplitude only | Wire velocity to `filterEnvDepth` param using `vel01 * velFilterAmt` multiplier |
| D002: no LFOs | Add `SimpleLFO lfo1, lfo2` using `/mod-matrix-builder` template |
| D004: dead parameter | Either wire it to DSP or remove it from `createParameterLayout()` |
| D005: LFO rate floor too high | Change `NormalisableRange` minimum from 0.1f to 0.01f |
| D006: no mod wheel | Add CC1 handler in `renderBlock()` MIDI loop |
| D006: no aftertouch | Add `isChannelPressure()` / `isAftertouch()` handler in MIDI loop |
| Coupling STUB | Copy `applyCouplingInput` pattern from `BiteEngine.h` or `OrbitalEngine.h` |

For full remediation templates, use `/mod-matrix-builder`.
For complete seance-level review, use `/synth-seance`.
