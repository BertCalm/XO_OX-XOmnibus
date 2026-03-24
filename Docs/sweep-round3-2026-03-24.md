# Round 3 Audit — 2026-03-24

Scope: Verify all Round 2 fixes are in place, then expand to Kitchen Collection
(4 engines), MasterFXSequencer, EngineRegistry thread safety, and fleet-wide
SilenceGate coverage.

---

## Part 1 — Round 2 Fix Verification

### MegaCouplingMatrix.h

VERIFIED FIX [H04]: `activeEngines` is declared as
`std::array<std::atomic<SynthEngine*>, MaxSlots>` at line 273. Correct.

VERIFIED FIX [H05]: `processBlock()` line 136 uses `std::abs(route.amount)` for
the bipolar check. Correct.

VERIFIED FIX [H06]: `processBlock()` reads engine pointers with
`memory_order_relaxed` (audio thread, line 144-145); `setEngines()` writes with
`memory_order_release` (line 59); `getActiveEngines()` reads with
`memory_order_acquire` (line 71). Full acquire/release pairing in place. Correct.

---

### CouplingPresetManager.h

VERIFIED FIX [C01]: `typeNames[]` at line 319 has exactly 15 entries:
"AmpToFilter", "AmpToPitch", "LFOToPitch", "EnvToMorph", "AudioToFM",
"AudioToRing", "FilterToFilter", "AmpToChoke", "RhythmToBlend", "EnvToDecay",
"PitchToPitch", "AudioToWavetable", "AudioToBuffer", "KnotTopology",
"TriangularCoupling". Correct.

VERIFIED FIX [C02]: `jlimit` upper bound in `fromJSON()` (line 125) and
`bakeCurrent()` (line 227-228) both use
`static_cast<int>(CouplingType::TriangularCoupling)`. Correct.

---

### PresetManager.h

VERIFIED FIX [P01]: `validCouplingTypes` (line 319-323) includes "AudioToBuffer",
"KnotTopology", and "TriangularCoupling". Correct.

---

### OutlookEngine.h (DSP fixes)

VERIFIED FIX [O01]: Delay buffers use `std::vector<float>` sized dynamically in
`prepare()` at lines 91-92 (`kDelayBufSizeDynamic = static_cast<int>(kDelayMaxSeconds
* sampleRate) + 1`). No hardcoded 24000. Correct.

VERIFIED FIX [O02]: `applyCouplingInput()` line 347 has the `numSamples <= 0`
guard. Correct.

VERIFIED FIX [O03]: Coupling accumulators reset at end of `renderBlock()` at lines
328-330 (`couplingFilterMod = couplingParallaxMod = couplingHorizonMod = 0.0f`).
Correct.

VERIFIED FIX [O04]: `noiseSeed` is now a `uint32_t` member of the `Voice` struct
at line 533, with comment "F06: per-voice PRNG seed (was static thread_local)".
The `renderWave()` function takes a `uint32_t&` parameter. No `thread_local`
anywhere in the file. Correct.

---

### OxytocinParamSnapshot.h

VERIFIED FIX [F02]: All 29 `std::atomic<float>*` pointers cached in
`attachParameters()` (lines 65-96). `update()` calls `p->load(memory_order_relaxed)`
on each cached pointer — zero string lookups on audio thread. The legacy
`update(apvts&)` overload delegates to the no-arg `update()`. Correct.

---

### MasterFXSequencer.h

VERIFIED FIX [M01]: Both `static int clamp(int, int, int)` and
`static float clamp(float, float, float)` overloads exist at lines 370-377.
Both are used throughout the file without ambiguity. Correct.

---

### ObrixEngine.h

VERIFIED FIX [B01]: The aftertouch for-loop at line 592 reads:
`for (auto& v : voices) if (v.active) v.aftertouch = ...;`
The comment at line 591 confirms: "Braces added to prevent dangling-else".
Note: the braces are on the for-loop iteration not the if-body (single-line body,
no else clause). No dangling-else risk since there is no else. The original bug
(if-else chain after a bare if) is resolved. Correct.

---

### PlaySurface.h

VERIFIED FIX [PS01]: `juce::MidiMessageCollector* midiCollector = nullptr` exists
at line 76. Notes route through it when set (lines 165-177), with std::function
fallback. Correct.

VERIFIED FIX [PS02]: Fretless mode maps X=pitch (line 280-287, C1 MIDI 24 to C7
MIDI 96) and Y=expression (line 292, velocity 0.35–1.0). Correct.

VERIFIED FIX [PS03]: Drag in fretless mode sends continuous pitch bend (lines
316-332), NOT note-on/off pairs. Pitch wheel value computed as
`8192 + (int)(bendNorm * 8191.0f)`. Correct.

VERIFIED FIX [PS04]: Drum mode uses MPC Bank A note table at lines 214-219
(pad 0 = 37, pad 1 = 36, etc.). Correct.

PARTIAL CONCERN [PS05 — TouchExpression atomic double-buffer]: The original
Round 2 spec mentioned "ToucheExpression uses atomic double-buffer." No class
named `ToucheExpression` or `TouchExpression` was found in PlaySurface.h. The
file has a `PerformanceStrip` component (Zone 3) that stores `stripX`/`stripY` as
plain `float` members and updates them via `tick()` and `updateStrip()` — both
on the message thread, called from `NoteInputZone`. There is no audio-thread
access path to these values, so the lack of atomics is not a correctness bug.
If the original bug report referred to a different component, it may no longer
exist or was renamed. STATUS: No atomic double-buffer implemented, but no
audio-thread race found either. Monitoring only.

---

## Part 2 — New Areas

### Kitchen Collection — 4-engine spot check

Engines audited: OgreEngine.h (OGRE/Cellar), OvenEngine.h (OVEN/Kitchen),
OverwashEngine.h (OVERWASH/Broth), OddfellowEngine.h (ODDFELLOW/Fusion)

**Hardcoded buffer sizes:**

CLEAN [KC-01]: No hardcoded 24000, 44100, or 48000 sample counts found in any of
the 4 engines. All delay/reverb buffers are sized either from `sampleRate` in
`prepare()` or use small fixed-size arrays (e.g. modal resonators, not delay
lines). Correct.

**getRawParameterValue in processBlock:**

CLEAN [KC-02]: All four engines call `getRawParameterValue()` only inside
`attachParameters()`. `processBlock()`/`renderBlock()` reads exclusively via
cached `std::atomic<float>*` pointers. Correct.

**Unguarded division by numSamples:**

CLEAN [KC-03]: Ogre, Oven, and Oddfellow do not divide by `numSamples` inside
`applyCouplingInput()`. They read `buf[numSamples - 1]` (last sample value) which
is protected by the `numSamples <= 0` guard at the top. Overwash's
`applyCouplingInput()` ignores `numSamples` entirely (parameter named
`/*numSamples*/`) — no division risk. Correct.

**Coupling accumulators not reset:**

CLEAN [KC-04]:
- OgreEngine: resets `couplingFilterMod = couplingPitchMod = 0.0f` in
  `renderBlock()` before use (lines 280-281). Correct.
- OvenEngine: resets `couplingFilterMod = couplingPitchMod = couplingBodyMod =
  0.0f` at lines 610-612. Correct.
- OverwashEngine: resets `extFilterMod = extPitchMod = extRingMod = 0.0f` at
  lines 518-520. Also resets in `prepare()` at line 221. Correct.
- OddfellowEngine: resets `couplingFilterMod = couplingPitchMod = couplingReedMod
  = 0.0f` at lines 432-434. Correct.

---

### MasterFX Chain — additional clamp audit

CLEAN [MX-01]: The `MasterFXSequencer` file uses only its own `static clamp()`
overloads (both int and float), not `std::clamp` or `juce::jlimit`. All six call
sites in the file (`setSteps`, `setDepth`, `setSmooth`, `setEnvFollowAmount`,
`effectiveDepth`, `randomWalkState`) resolve cleanly to the correct typed overload.
No implicit truncation risk. Correct.

---

### EngineRegistry thread safety

NEW BUG [ER-01] — SEVERITY: LOW

`EngineRegistry` (Source/Core/EngineRegistry.h) has no synchronization
whatsoever. The internal `std::unordered_map<std::string, EngineFactory> factories`
is accessed from `registerEngine()` (called at static-initialization time),
`createEngine()`, `getRegisteredIds()`, and `isRegistered()`.

In practice, all registrations happen before `main()` runs (via static-init
REGISTER_ENGINE macros) and lookups happen on the message thread only — so there
is no concurrent access today. However, the CLAUDE.md specifies that "Export
systems must run on non-audio worker threads." If any worker thread ever calls
`createEngine()` while a late static-init runs, this is a data race on the map.

Recommended fix: wrap `factories` in a `juce::CriticalSection` with a `const` read
path, or document in a comment that registrations are strictly compile-time and
lookups are message-thread only.

---

### Fleet-wide SilenceGate coverage

AUDIT METHOD: Identified all 71 files declaring `class *Engine* : public
SynthEngine`, then cross-referenced against files containing any mention of
`silenceGate` (either the protected member or wrapper methods).

NEW BUG [SG-01] — SEVERITY: MEDIUM
6 engines have zero SilenceGate integration — they never call
`silenceGate.prepare()`, `silenceGate.wake()`, `silenceGate.isBypassed()`, or
`silenceGate.analyzeBlock()`. These engines will run their DSP every block
regardless of whether any notes are active, burning idle CPU.

Missing engines:
1. `Source/Engines/Obrix/ObrixEngine.h` (OBRIX) — 81 params, complex reef ecology DSP
2. `Source/Engines/OpenSky/OpenSkyEngine.h` (OPENSKY) — supersaw + shimmer reverb
3. `Source/Engines/Optic/OpticEngine.h` (OPTIC) — visual engine (intentionally
   zero-audio? But still processes per block)
4. `Source/Engines/Orbweave/OrbweaveEngine.h` (ORBWEAVE) — topological knot coupling
5. `Source/Engines/Ostinato/OstinatoEngine.h` (OSTINATO) — pattern engine with "bar of
   silence" logic but no SilenceGate
6. `Source/Engines/Ouie/OuieEngine.h` (OUIE) — duophonic hammerhead synth

Note on OPTIC: This engine produces visual output, not audio. Its `renderBlock()`
may not output audio samples at all. It may be intentionally exempt from the
SilenceGate pattern. Recommend confirming and adding a comment if so.

Fix pattern (example for OBRIX):
```cpp
// In prepare():
silenceGate.prepare(sampleRate, maxBlockSize);
silenceGate.setHoldTime(200.0f); // standard engine hold

// In renderBlock() — after MIDI parsing, before DSP:
if (silenceGate.isBypassed() && midi.isEmpty()) {
    buffer.clear();
    return;
}
// ... DSP ...

// At end of renderBlock():
silenceGate.analyzeBlock(buffer.getReadPointer(0),
                         buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr,
                         numSamples);
```

---

## Summary

| Category | Count | IDs |
|----------|-------|-----|
| VERIFIED FIX | 15 | H04, H05, H06, C01, C02, P01, O01-O04, F02, M01, B01, PS01-PS04 |
| PARTIAL CONCERN | 1 | PS05 (TouchExpression atomic — no race found, monitoring) |
| NEW BUG (LOW) | 1 | ER-01 (EngineRegistry no synchronization) |
| NEW BUG (MEDIUM) | 1 | SG-01 (6 engines missing SilenceGate: OBRIX, OPENSKY, OPTIC, ORBWEAVE, OSTINATO, OUIE) |
| CLEAN | 5 | KC-01–KC-04, MX-01 |

### Priority order for fixes
1. **SG-01** (Medium): 6 engines burn idle CPU. OPTIC can be documented as exempt.
   The other 5 need a 5-line prepare/wake/bypass/analyze integration each.
2. **ER-01** (Low): Add a comment clarifying single-threaded access contract, or
   add a mutex if worker-thread engine creation is ever needed.
3. **PS05** (Monitor): No fix needed unless a future audio-thread read path is
   added to PerformanceStrip position data.

All Round 2 fixes confirmed correct. No regressions detected.
