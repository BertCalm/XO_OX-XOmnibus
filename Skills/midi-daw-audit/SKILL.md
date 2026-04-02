# Skill: /midi-daw-audit

**Invoke with:** `/midi-daw-audit [engine-name | processor | full]`
**Status:** LIVE
**Purpose:** Comprehensive MIDI and DAW compatibility audit for XOceanus engines and the host processor. Covers CC handling thread safety, MIDI learn integration, parameter automation exposure, state persistence, MPE configuration, pitch bend, sustain, transport sync, and plugin contract compliance. Produces a pass/fail report with remediation notes.

---

## Scope vs. Related Skills

| Skill | Covers |
|-------|--------|
| `/dsp-safety` | Audio thread allocations, blocking I/O, denormals, renderBlock safety |
| `/engine-health-check` | D001–D006 doctrine compliance (velocity, LFOs, expression coverage) |
| **/midi-daw-audit** | CC handling thread safety, MIDI learn wiring, APVTS automation exposure, state persistence, MPE, pitch bend, sustain, transport sync, plugin contract |

Run all three for a complete pre-release check. This skill does **not** duplicate doctrine checks — if D006 (expression coverage) fails, `/engine-health-check` is the right tool.

---

## When to Use This Skill

- Before any release or AU/AUv3 submission
- After modifying `XOceanusProcessor.cpp` or any engine's MIDI handling
- When a user reports "CC not working in [DAW]" or "automation lost after reopen"
- After adding a new engine that handles its own MIDI (not routed through ChordMachine)
- After adding new non-APVTS state that needs DAW project recall
- When integrating a new controller type (MPE, MIDI 2.0, OSC bridge)
- After renaming or versioning parameters

---

## The 10 Compatibility Checks

| ID | Check | Scope |
|----|-------|-------|
| M01 | CC handling thread safety | `MIDILearnManager`, per-engine MIDI loops |
| M02 | MIDI learn wired end-to-end | `XOceanusProcessor` integration |
| M03 | APVTS parameter exposure | All parameters host-automatable |
| M04 | State persistence completeness | Non-APVTS state in `getStateInformation` |
| M05 | MPE configuration backed by APVTS | `mpe_*` params cached and synced |
| M06 | Pitch bend response | Per-engine pitch bend handler |
| M07 | Sustain pedal (CC64) | Held note management |
| M08 | DAW transport sync | Host BPM / PPQ / isPlaying |
| M09 | Parameter ID versioning | `ParameterID("id", version)` format |
| M10 | Plugin contract compliance | Tail time, latency, bus layout |

---

## M01 — CC Handling Thread Safety

**Problem:** `beginChangeGesture()` / `endChangeGesture()` signal host automation recording. They must be called from the **message thread** only. Calling them from the audio thread causes undefined behavior and crashes in Pro Tools (AudioSuite), Logic Pro, and Reaper.

**Where to check:**
```
Source/Core/MIDILearnManager.h → processMidi()
Any engine renderBlock() MIDI loop that manually sets params
```

**Pattern to find (FAIL):**
```cpp
// FAIL — gesture calls on audio thread
p->beginChangeGesture();
p->setValueNotifyingHost(...);
p->endChangeGesture();
```

**Correct pattern (PASS):**
```cpp
// PASS — audio thread: setValueNotifyingHost() only
p->setValueNotifyingHost(p->convertTo0to1(paramValue));

// PASS — message thread (UI callbacks, timer, button press): gestures allowed
p->beginChangeGesture();
p->setValueNotifyingHost(...);
p->endChangeGesture();
```

**Remediation:** Remove `beginChangeGesture()` / `endChangeGesture()` from any function that runs on the audio thread. Move them to the UI callback or learn-complete callback on the message thread.

---

## M02 — MIDI Learn Wired End-to-End

Five integration points must all be present for `MIDILearnManager` to function:

| Point | Location | Check |
|-------|----------|-------|
| `#include "Core/MIDILearnManager.h"` | `XOceanusProcessor.h` | Header included |
| `MIDILearnManager midiLearnManager` | `XOceanusProcessor.h` private | Member declared |
| `getMIDILearnManager()` | `XOceanusProcessor.h` public | Accessor exposed to UI |
| `midiLearnManager.setAPVTS(&apvts)` | Constructor | APVTS pointer set |
| `midiLearnManager.loadDefaultMappings()` | Constructor | Default CC map installed |
| `midiLearnManager.processMidi(midi)` | `processBlock()` | Called each block |
| `checkPendingLearn()` | Timer or editor | Learn captures finalized |

**Verify default CC map uses correct APVTS IDs:**

| CC | Expected APVTS ID | Wrong (historical) |
|----|-------------------|-------------------|
| CC1 Mod Wheel | `macro2` | `macro_movement` |
| CC2 Breath | `macro1` | `macro_character` |
| CC11 Expression | `macro4` | `macro_space` |
| CC74 Brightness | `macro1` | `macro_character` |

**Test:** Enable MIDI learn for `snap_filterCutoff`. Move a physical knob. Parameter should respond immediately. Save and reopen DAW project — mapping should survive.

---

## M03 — APVTS Parameter Exposure

Every parameter that should be DAW-automatable must be registered in `createParameterLayout()`. Parameters that exist only as engine members, raw atomics, or manager internal state are **invisible to the host**.

**Check `createParameterLayout()` completeness:**

```
Required parameter groups:
✅ Master parameters (masterVolume)
✅ 4 Macros (macro1–macro4)
✅ All engine parameters (via engine::addParameters())
✅ Chord Machine parameters (cm_*)
✅ Master FX parameters (master_*)
✅ MPE parameters (mpe_enabled, mpe_zone, mpe_pitchBendRange, mpe_pressureTarget, mpe_slideTarget)
```

**For each engine, verify `addParameters()` is called in `createParameterLayout()`:**
```
Search: createParameterLayout() in XOceanusProcessor.cpp
Find: [EngineName]Engine::addParameters(params)
All 43 registered engines must appear.
```

**`ParameterID` version field:** Every parameter must use `juce::ParameterID("id", 1)` (not a bare string). The version number is required by JUCE 7+ for AU parameter stability. A bare `"id"` string compiles but produces unstable AU parameter hashes across builds.

```cpp
// PASS — versioned
params.push_back(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID("snap_filterCutoff", 1), "Filter Cutoff", ...));

// FAIL — unversioned (breaks AU parameter recall)
params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "snap_filterCutoff", "Filter Cutoff", ...));
```

---

## M04 — State Persistence Completeness

`getStateInformation()` / `setStateInformation()` must capture **all** state that isn't automatically stored in the APVTS. Anything missing is silently lost on DAW project reopen.

**Non-APVTS state that requires explicit serialization:**

| State | Serialization Method | Location |
|-------|---------------------|----------|
| MIDI learn mappings | `midiLearnManager.toJSON()` → XML attribute `"midiLearnMappings"` | `getStateInformation` |
| Chord Machine sequencer step data | `chordMachine.saveState()` → stored in preset `sequencerData` | `applyPreset` |
| Engine slot assignments (which engine in which slot) | Must be serialized if not handled elsewhere | `getStateInformation` |

**Pattern check in `getStateInformation()`:**
```cpp
// PASS — MIDI mappings included
auto state = apvts.copyState();
auto xml = std::unique_ptr<juce::XmlElement>(state.createXml());
xml->setAttribute("midiLearnMappings", juce::JSON::toString(midiLearnManager.toJSON()));
copyXmlToBinary(*xml, destData);
```

**Pattern check in `setStateInformation()`:**
```cpp
// PASS — with backward-compatible fallback
if (xml->hasAttribute("midiLearnMappings"))
    midiLearnManager.fromJSON(juce::JSON::parse(xml->getStringAttribute("midiLearnMappings")));
else
    midiLearnManager.loadDefaultMappings();  // older project → restore defaults
```

**Backward compatibility rule:** Always use `xml->hasAttribute()` before reading new attributes. Projects saved before a feature was added must not crash or produce empty state.

---

## M05 — MPE Configuration Backed by APVTS

MPE settings must be APVTS-backed so the host saves/recalls them with the project. Settings stored only in `MPEManager` atomic members are lost on project reopen.

**Required APVTS parameters:**

| ID | Type | Default | Notes |
|----|------|---------|-------|
| `mpe_enabled` | AudioParameterBool | false | Enables MPE zone parsing |
| `mpe_zone` | AudioParameterChoice | Off | Off/Lower/Upper/Both |
| `mpe_pitchBendRange` | AudioParameterFloat 1–96 | 48 | Semitones per spec |
| `mpe_pressureTarget` | AudioParameterChoice | Filter Cutoff | 6 targets (matches `ExpressionTarget` enum) |
| `mpe_slideTarget` | AudioParameterChoice | Filter Cutoff | 6 targets (matches `ExpressionTarget` enum) |

**CachedParams check** — all 5 must be initialized in `cacheParameterPointers()`:
```cpp
cachedParams.mpeEnabled        = apvts.getRawParameterValue("mpe_enabled");
cachedParams.mpeZone           = apvts.getRawParameterValue("mpe_zone");
cachedParams.mpePitchBendRange = apvts.getRawParameterValue("mpe_pitchBendRange");
cachedParams.mpePressureTarget = apvts.getRawParameterValue("mpe_pressureTarget");
cachedParams.mpeSlideTarget    = apvts.getRawParameterValue("mpe_slideTarget");
```

**processBlock sync check** — MPEManager must be driven from cached params each block:
```cpp
if (cachedParams.mpeEnabled)
{
    mpeManager.setMPEEnabled(cachedParams.mpeEnabled->load() >= 0.5f);
    mpeManager.setZoneLayout(static_cast<MPEZoneLayout>(
        static_cast<int>(cachedParams.mpeZone->load())));
    mpeManager.setPitchBendRange(
        static_cast<int>(cachedParams.mpePitchBendRange->load()));
    mpeManager.setPressureTarget(static_cast<MPEManager::ExpressionTarget>(
        static_cast<int>(cachedParams.mpePressureTarget->load())));
    mpeManager.setSlideTarget(static_cast<MPEManager::ExpressionTarget>(
        static_cast<int>(cachedParams.mpeSlideTarget->load())));
}
```

**ExpressionTarget enum alignment** — the choice parameter string count must match the enum's `NumTargets`:
```
MPEManager::ExpressionTarget: FilterCutoff(0), Volume(1), WavetablePosition(2),
                               FXSend(3), MacroCharacter(4), MacroMovement(5)
→ 6 entries required in mpe_pressureTarget and mpe_slideTarget choices
```

---

## M06 — Pitch Bend Response (Per-Engine)

Every MIDI-capable engine must respond to pitch bend messages.

**Search pattern:**
```
In renderBlock() MIDI loop:
  msg.isPitchWheel()
  msg.getPitchWheelValue()  // returns -8192..8191
```

**Standard implementation:**
```cpp
if (msg.isPitchWheel())
{
    float semitones = (msg.getPitchWheelValue() / 8192.0f) * pitchBendRange;
    // Apply to all active voice oscillators
}
```

**Range parameter:** Pitch bend range should be exposed as a parameter (typical range: ±2 or ±12 semitones). Check that the range default matches the engine's musical intent — a bass synth needs ±2; a lead synth benefits from ±12.

**MPE pitch bend:** In MPE mode, per-channel pitch bend is handled by `MPEManager` — the engine should read `mpeVoice.pitchBendSemitones` rather than global pitch bend. Check that MPE-enabled engines don't double-apply bend.

**Exception:** OPTIC (visual engine) — pitch bend intentionally not applicable.

---

## M07 — Sustain Pedal (CC64)

Sustain pedal must prevent note-off from releasing voices. Missing sustain support is a common DAW/live performance complaint.

**Search pattern:**
```
In renderBlock() MIDI loop:
  msg.isController() && msg.getControllerNumber() == 64
  sustainPedal, sustainHeld, cc64
```

**Standard pattern:**
```cpp
if (msg.isController() && msg.getControllerNumber() == 64)
{
    sustainHeld = (msg.getControllerValue() >= 64);
    if (!sustainHeld)
        releaseHeldNotes();  // release any notes held by sustain
}

if (msg.isNoteOff())
{
    if (sustainHeld)
        markNoteHeldBySustain(note);  // defer release
    else
        releaseVoice(note);
}
```

**Minimum requirement:** At least one sustain mode (hold all active voices). Full sustain (hold notes from pedal-down until pedal-up) is the standard expectation.

---

## M08 — DAW Transport Sync

Host tempo and transport state should drive any time-based behavior (sequencers, sync'd LFOs, arpeggiation).

**Check `processBlock()` for playhead usage:**
```cpp
if (auto* playHead = getPlayHead())
{
    if (auto pos = playHead->getPosition())
    {
        if (auto ppq = pos->getPpqPosition()) { /* use *ppq */ }
        if (auto bpmOpt = pos->getBpm())      { hostBPM = *bpmOpt; }
        bool isPlaying = pos->getIsPlaying();
    }
}
```

**Sync checklist:**

| Feature | Required |
|---------|---------|
| ChordMachine BPM follows host BPM | ✅ Required |
| ChordMachine sequencer pauses when host stops | ✅ Required |
| Per-engine sync'd LFOs follow host BPM | ✅ Required (where LFO has sync mode) |
| Master FX delay sync (master_delaySync param) | ✅ Required |
| MIDI Learn timing is transport-independent | ✅ Required (CC → param, no transport dependency) |

**Fallback:** When `getPlayHead()` returns null (standalone app) or `getBpm()` has no value, fall back to internal BPM (`cm_seq_bpm` parameter). Never dereference an `Optional` without checking `hasValue()`.

---

## M09 — Parameter ID Versioning and Stability

XOceanus has a hard rule: **parameter IDs are frozen after release.** A changed ID breaks all saved automation in every DAW project that used it.

**Version number format check:**
```cpp
// PASS — ParameterID with explicit version
juce::ParameterID("snap_filterCutoff", 1)

// FAIL — bare string (JUCE 7+ deprecation warning, unstable AU hash)
"snap_filterCutoff"
```

**Frozen prefix audit** — spot-check a sample of engines:
```
snap_*    → OddfeliX only
morph_*   → OddOscar only
macro1/2/3/4 → global macros only
mpe_*     → processor-level MPE only
master_*  → master FX only
cm_*      → Chord Machine only
```

**Never-rename rule:** If a parameter must be replaced, deprecate it:
1. Keep the old ID in `createParameterLayout()` with `[DEPRECATED]` in display name
2. Add the new ID alongside it
3. Map old → new in `setStateInformation()` for migration
4. Remove old ID only after a major version bump with a migration tool

**Check `resolveEngineAlias()`** in `Source/Core/PresetManager.h` — all legacy engine name aliases must be present when integrating migrated presets.

---

## M10 — Plugin Contract Compliance

The JUCE `AudioProcessor` contract has several methods XOceanus must implement correctly for DAW compatibility.

**Check `XOceanusProcessor.h` for required overrides:**

| Method | Expected Value | Risk If Wrong |
|--------|---------------|---------------|
| `acceptsMidi()` | `true` | No MIDI routed into plugin |
| `producesMidi()` | `false` | Some hosts disable track routing if true when unexpected |
| `getTailLengthSeconds()` | `6.0` | Host cuts playback tail too early; reverbs/delays clip |
| `getLatencySamples()` | 0 (or actual if lookahead added) | DAW compensates incorrectly; MIDI sync offset |
| `isBusesLayoutSupported()` | must accept stereo output | Hosts may refuse to instantiate |
| `getName()` | `"XOceanus"` | Preset/project file association |

**Bus layout check:**
```cpp
// Minimum: stereo output, no required inputs (synth)
BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)
```

**Tail length note:** With master reverb and delay, 6 seconds is appropriate. If engines with very long tails are added (OBSIDIAN, OPAL with large grain sizes), consider bumping to 8–10 seconds.

**`prepareToPlay()` must handle repeated calls:** DAW hosts call `prepareToPlay()` when sample rate or block size changes mid-session. All filters, buffers, and profilers must safely re-initialize. Check that no state is left dirty from a previous call.

---

## Phase 1: Processor-Level Audit

Run this when auditing `XOceanusProcessor.*`:

```
1. Open Source/XOceanusProcessor.h
2. Verify: MIDILearnManager member + accessor present          → M02
3. Verify: mpe_* params in CachedParams, all non-null         → M05
4. Open Source/XOceanusProcessor.cpp
5. Check processMidi() call in processBlock()                  → M02
6. Check MPE sync block in processBlock()                      → M05
7. Check transport sync block in processBlock()                → M08
8. Check getStateInformation includes midiLearnMappings        → M04
9. Check setStateInformation restores with fallback            → M04
10. Check createParameterLayout() has mpe_* params             → M05
11. Check all ParameterID uses have version number             → M09
12. Verify plugin contract methods                             → M10
```

---

## Phase 2: Engine-Level Audit

Run this for each individual engine being checked (invoke with engine name):

```
1. Open Source/Engines/{Name}/{Name}Engine.h
2. Search renderBlock() MIDI loop for:
   - isPitchWheel() handling                                    → M06
   - CC64 sustain handling                                      → M07
   - isController() without beginChangeGesture()                → M01
3. Verify no direct parameter hash lookups in renderBlock()
   (should use ParamSnapshot or cached raw pointers)
4. Verify no AudioProcessorValueTreeState calls on audio thread
```

---

## Phase 3: Generate Report

After completing all checks, produce a compatibility table:

```
=== MIDI/DAW AUDIT: [TARGET] ===
Date: [date]
Auditor: Claude Code

MIDI / HOST INTEGRATION
-----------------------
M01 CC thread safety:       [✅ PASS | ❌ FAIL] — [note]
M02 MIDI learn wired:       [✅ PASS | ⚠️ PARTIAL | ❌ FAIL] — [missing points]
M03 APVTS exposure:         [✅ PASS | ⚠️ PARTIAL | ❌ FAIL] — [missing params if any]
M04 State persistence:      [✅ PASS | ⚠️ PARTIAL | ❌ FAIL] — [missing state]
M05 MPE APVTS-backed:       [✅ PASS | ❌ FAIL] — [nullptr pointers if any]
M06 Pitch bend response:    [✅ PASS | ❌ FAIL | N/A] — [range: ±N semitones]
M07 Sustain pedal CC64:     [✅ PASS | ❌ FAIL] — [hold behavior]
M08 Transport sync:         [✅ PASS | ⚠️ PARTIAL | ❌ FAIL] — [synced features]
M09 Param ID versioning:    [✅ PASS | ❌ FAIL] — [unversioned IDs if any]
M10 Plugin contract:        [✅ PASS | ❌ FAIL] — [tail/latency/bus]

OVERALL
-------
[🟢 DAW COMPATIBLE | 🟡 MINOR ISSUES | 🔴 CRITICAL — DO NOT RELEASE]

ISSUES (if any):
P0 [crash-level]: [description]
P1 [data loss]:   [description]
P2 [degraded UX]: [description]

REMEDIATION ORDER:
1. [P0 fixes first]
2. [P1 fixes]
3. [P2 fixes]
```

---

## Remediation Quick Reference

| Check | Fastest Fix |
|-------|------------|
| M01: gesture calls on audio thread | Remove `beginChangeGesture()`/`endChangeGesture()` from `processMidi()` — use `setValueNotifyingHost()` only |
| M02: MIDI learn not wired | Add member + accessor to header; `setAPVTS()` + `loadDefaultMappings()` in constructor; `processMidi(midi)` in `processBlock()` |
| M02: wrong default CC IDs | `"macro_movement"` → `"macro2"`, `"macro_character"` → `"macro1"`, `"macro_space"` → `"macro4"` |
| M03: param missing from layout | Add to `createParameterLayout()` and call `addParameters()` for engine |
| M03: bare string ParameterID | Wrap: `juce::ParameterID("id", 1)` |
| M04: MIDI mappings not saved | Add `xml->setAttribute("midiLearnMappings", ...)` in `getStateInformation()` + `fromJSON()` in `setStateInformation()` |
| M05: MPE params nullptr | Add to `createParameterLayout()`, init in `cacheParameterPointers()`, sync in `processBlock()` |
| M06: no pitch bend | Add `isPitchWheel()` handler in engine's `renderBlock()` MIDI loop |
| M07: no sustain | Add CC64 handler with `sustainHeld` flag and deferred note release |
| M08: no transport sync | Wrap `getPlayHead()` → `getPosition()` → `getPpqPosition()` / `getBpm()` / `getIsPlaying()` |
| M10: tail too short | Increase `getTailLengthSeconds()` return value |

---

## Related Resources

- `Source/Core/MIDILearnManager.h` — Full MIDI learn implementation
- `Source/Core/MPEManager.h` — MPE zone + per-voice expression state
- `Source/XOceanusProcessor.h/.cpp` — All processor-level integration points
- `Docs/xoceanus_mobile_and_midi_spec.md` — MPE controller layout + spec
- `Docs/xoceanus_master_specification.md` — Plugin format targets + AU/VST3 roadmap
- `/dsp-safety` — Audio thread allocation / blocking I/O (separate skill)
- `/engine-health-check` — D001–D006 doctrine compliance (separate skill)
