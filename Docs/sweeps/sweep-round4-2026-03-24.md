# Round 4 Audit — 2026-03-24

Scope: Verify all Round 3 SG-01 and ER-01 fixes are in place, then spot-check
4 additional fleet engines for remaining patterns.

---

## Part 1 — Round 3 Fix Verification

### SilenceGate (SG-01) — 6 engines

All 6 engines now have full SilenceGate integration. Verified by `grep` against
all four call sites.

**OBRIX** (`Source/Engines/Obrix/ObrixEngine.h`)

VERIFIED FIX [SG-OBRIX]:
- Line 282: `prepareSilenceGate (sampleRate, maxBlockSize, 500.0f)` — 500ms hold
  for complex reef ecology DSP with reverb FX tails. Called in `prepare()`.
- Line 329–330: `wakeSilenceGate()` on note-on; `isSilenceGateBypassed()` bypass
  check with `buffer.clear(); return;` at start of `renderBlock()`.
- Line 1036: `analyzeForSilenceGate (buffer, numSamples)` at end of
  `renderBlock()`, after active voice count update and before brick complexity
  calculation.

**OPENSKY** (`Source/Engines/OpenSky/OpenSkyEngine.h`)

VERIFIED FIX [SG-OPENSKY]:
- Line 545: `prepareSilenceGate (sampleRate, maxBlockSize, 500.0f)` — 500ms hold
  for shimmer reverb tails. Called at end of `prepare()`.
- Lines 587–588: wake on note-on + bypass check at start of `renderBlock()`.
- Line 938: `analyzeForSilenceGate (buffer, numSamples)` after
  `envelopeOutput = peakEnv`, before `getSampleForCoupling`.

**OPTIC** (`Source/Engines/Optic/OpticEngine.h`)

VERIFIED FIX [SG-OPTIC]:
- Line 555: `prepareSilenceGate (sampleRate, maxBlockSize, 100.0f)` with a
  10-line comment in `prepare()` documenting the intentional exemption.
- No bypass check in `renderBlock()` — correct, per B005 (Zero-Audio Identity).
  OPTIC must run continuously to track spectral features. The SilenceGate
  contract is satisfied (prepare called) but the bypass short-circuit is
  intentionally omitted, matching the D006 aftertouch exemption precedent.

**ORBWEAVE** (`Source/Engines/Orbweave/OrbweaveEngine.h`)

VERIFIED FIX [SG-ORBWEAVE]:
- Line 223: `prepareSilenceGate (sampleRate, maxBlockSize, 500.0f)` — 500ms hold
  for phase-braided oscillators with reverb FX.
- Lines 247–248: wake on note-on + bypass check at start of `renderBlock()`.
- Line 524: `analyzeForSilenceGate (buffer, numSamples)` after active voice
  count update.

**OSTINATO** (`Source/Engines/Ostinato/OstinatoEngine.h`)

VERIFIED FIX [SG-OSTINATO]:
- Line 1535: `prepareSilenceGate (sampleRate, maxBlockSize, 500.0f)` — 500ms
  hold. Comment notes the sequencer-trigger special case.
- Line 1563: `wakeSilenceGate()` on live MIDI note-on.
- No bypass-on-empty-MIDI check — correct. OSTINATO's autonomous pattern
  sequencer fires triggers even when the MIDI buffer is empty. Bypassing on
  empty MIDI would silence patterns. The gate is still analyzed to detect
  genuine end-of-tail silence.
- Line 1763: `wakeSilenceGate()` inside `consumeTrigger()` branch — ensures
  sequencer-driven triggers also keep the gate open.
- Line 1853: `analyzeForSilenceGate (buffer, numSamples)` after
  `activeVoiceCounter` update and coupling accumulator reset.

**OUIE** (`Source/Engines/Ouie/OuieEngine.h`)

VERIFIED FIX [SG-OUIE]:
- Line 748: `prepareSilenceGate (sampleRate, maxBlockSize, 500.0f)` — 500ms hold
  for Karplus-Strong (KS) plucked string decay tails.
- Lines 787–788: wake on note-on + bypass check at start of `renderBlock()`.
- Line 1282: `analyzeForSilenceGate (buffer, numSamples)` after
  `activeVoices.store()`.

---

### EngineRegistry (ER-01)

VERIFIED FIX [ER-01]: `Source/Core/EngineRegistry.h` now has a 4-line
thread-safety comment immediately before `registerEngine()`:

```cpp
// Thread-safety: All registerEngine() calls happen during static initialization
// before main(). createEngine() is called from the message thread only.
// No concurrent access is possible under the current architecture.
// If this changes (e.g., dynamic plugin loading), add a mutex.
```

No mutex was added — the current single-threaded access contract is documented
rather than over-engineered. Correct.

---

### Build verification

`cmake --build build 2>&1 | grep "error:"` — zero errors. All 6 SilenceGate
fixes and the EngineRegistry comment compiled cleanly.

---

## Part 2 — 4-Engine Spot Check

Engines audited: XOverlapAdapter.h (OVERLAP), OrganismEngine.h (ORGANISM),
OsmosisEngine.h (OSMOSIS), OverworldEngine.h (OVERWORLD)

### SilenceGate coverage

CLEAN [SC-01]: All 4 spot-check engines have full SilenceGate integration
(all 4 call sites present). Verified by grep:
- OVERLAP:   silenceGate.prepare / silenceGate.wake / silenceGate.isBypassed / silenceGate.analyzeBlock
- ORGANISM:  prepareSilenceGate / wakeSilenceGate / isSilenceGateBypassed / analyzeForSilenceGate
- OSMOSIS:   prepareSilenceGate / isSilenceGateBypassed / wakeSilenceGate / analyzeForSilenceGate
- OVERWORLD: silenceGate.prepare / silenceGate.wake / silenceGate.isBypassed / silenceGate.analyzeBlock

---

### Hardcoded sample rates / buffer sizes

CLEAN [SC-02]: All occurrences of `44100` in the 4 engines are default member
initializers (e.g., `float sr = 44100.f;`) that are immediately overwritten in
`prepare(double sampleRate, ...)`. No hardcoded sample rate is used for buffer
sizing or DSP math.

One clarification: `OverworldEngine::kHaasDelayLen = 16` is documented as
"~0.3ms at 48kHz" (comment only). The actual sample count (16) is used as a
fixed ring buffer for a sub-millisecond Haas widener, producing 0.36ms at
44.1kHz and 0.33ms at 48kHz. Both are well within the psychoacoustic Haas zone.
Not a bug.

---

### getRawParameterValue in renderBlock

CLEAN [SC-03]: All 4 engines call `getRawParameterValue()` only inside
`attachParameters()`. All `renderBlock()` paths read exclusively via cached
`std::atomic<float>*` pointers. Correct.

---

### Coupling accumulator reset

NEW BUG [OL-01] — SEVERITY: LOW-MEDIUM

`XOverlapAdapter.h` (OVERLAP) does NOT reset its coupling accumulators at the
end of `renderBlock()`.

Four accumulator members:
- `extPitchMod`   (semitones → tangle depth perturbation)
- `extFilterMod`  (Hz offset → filter cutoff via AmpToFilter / FilterToFilter)
- `extRingMod`    (amplitude factor → AudioToRing)
- `extDelayMod`   (relative modulation of FDN delay base → AudioToFM)

These are set by `applyCouplingInput()` (lines 419, 423, 427, 431, 435, 439,
443) and consumed at block start in `renderBlock()` (lines 206–208). However,
they are never zeroed after consumption. The values persist until the next
`applyCouplingInput()` call for that coupling type.

**Effect**: If a coupling route is deactivated (e.g., the user removes the
route in the UI), `applyCouplingInput()` stops being called for that type, but
the last non-zero accumulator value continues modulating OVERLAP every block
indefinitely — until `reset()` is called. This manifests as a "sticky"
coupling: filter cutoff, delay time, or ring modulation appears to freeze at
the last coupled value even after the route is disconnected.

**Recommended fix** — at start of `renderBlock()`, before applying accumulators:
```cpp
const float consumedPitch  = extPitchMod;  extPitchMod  = 0.0f;
const float consumedFilter = extFilterMod; extFilterMod = 0.0f;
const float consumedRing   = extRingMod;   extRingMod   = 0.0f;
const float consumedDelay  = extDelayMod;  extDelayMod  = 0.0f;
// ... then use consumedXxx in the modulation math below
```

CLEAN [SC-04-ORGANISM]: ORGANISM correctly resets its coupling accumulators
(`couplingFilterMod = 0.f; couplingPitchMod = 0.f`) at block start in
`renderBlock()` (lines 731–732) with a comment "Resets each block". Correct.

CLEAN [SC-04-OSMOSIS]: OSMOSIS is an analysis source engine (no coupling
input). No accumulators needed. Correct.

CLEAN [SC-04-OVERWORLD]: OVERWORLD resets `externalFilterMod`,
`externalEraMod`, `externalEraYMod` at the end of `renderBlock()` (lines
440–442). Correct.

---

## Summary

| Category | Count | IDs |
|----------|-------|-----|
| VERIFIED FIX | 8 | SG-OBRIX, SG-OPENSKY, SG-OPTIC, SG-ORBWEAVE, SG-OSTINATO, SG-OUIE, ER-01, build-clean |
| CLEAN | 5 | SC-01 (SilenceGate), SC-02 (hardcoded SR), SC-03 (getRaw), SC-04-ORGANISM, SC-04-OVERWORLD |
| NEW BUG (LOW-MEDIUM) | 1 | OL-01 (OVERLAP coupling accumulators not reset) |

### Priority order for fixes

1. **OL-01** (Low-Medium): OVERLAP sticky coupling after route removal. Fix is
   5 lines: consume-and-zero the 4 accumulators at start of `renderBlock()`.
   No architectural change required.

All Round 3 bugs (SG-01, ER-01) confirmed fixed. No regressions detected.
Spot-check found one new accumulator bug in OVERLAP (OL-01).
