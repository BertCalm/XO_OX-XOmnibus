# XOceanus State Persistence Audit
**Date:** 2026-03-21
**Auditor:** Claude Code (Sonnet 4.6)
**DAW targets:** PreSonus Studio One, Akai MPC
**Files audited:**
- `Source/XOceanusProcessor.h/.cpp`
- `Source/Core/PresetManager.h`
- `Source/Core/EngineRegistry.h`
- `Source/Core/MegaCouplingMatrix.h`

---

## Summary Table

| Component | Saved | Restored | Format | Verdict |
|-----------|-------|----------|--------|---------|
| Engine selection (4 slots) | NO | NO | — | **CRITICAL BUG** |
| All engine parameter values | YES | YES | XML via APVTS | Working |
| Macro positions (M1–M4) | YES | YES | APVTS float params | Working |
| Coupling matrix baseline routes | NO | NO | — | **CRITICAL BUG** |
| Performance overlay (cp_r1–r4) | YES | YES | APVTS choice/float params | Working |
| ChordMachine sequencer steps | NO | NO | — | **BUG** |
| ChordMachine settings (BPM, palette, etc.) | YES | YES | APVTS params | Working |
| Current preset name/path | NO | NO | — | Gap (cosmetic) |
| MIDI learn mappings | YES | YES | JSON attribute on XML | Working |
| MasterFX parameters | YES | YES | APVTS float params | Working |
| MPE settings | YES | YES | APVTS params | Working |
| State schema version | NO | — | — | **BUG** (future risk) |

---

## What Is Working

### APVTS saves everything it owns
`getStateInformation` (line 1557) calls `apvts.copyState()` and serializes it to XML via `state.createXml()`, then calls `copyXmlToBinary()`. This correctly persists:
- `masterVolume`, `macro1`–`macro4`
- All 1000+ engine parameter values (every engine's `addParameters()` was called at construction)
- Chord Machine settings (`cm_enabled`, `cm_palette`, `cm_voicing`, `cm_seq_bpm`, etc.)
- Coupling performance overlay (`cp_r1_active`, `cp_r1_type`, `cp_r1_amount`, etc.)
- MasterFX chain parameters (`master_satDrive`, etc.)
- MPE parameters (`mpe_enabled`, `mpe_zone`, etc.)

### MIDI learn mappings
`midiLearnManager.toJSON()` is serialized to a JSON string and stored as an XML attribute (`midiLearnMappings`) on the APVTS XML element. On restore, `midiLearnManager.fromJSON()` is called, with a graceful fallback to `loadDefaultMappings()` for projects saved before this feature existed. This is correctly implemented.

### Format is safe (XML, not binary)
The state is serialized to XML (`state.createXml()`) wrapped in JUCE's binary envelope (`copyXmlToBinary`). XML is human-readable and forward-compatible for adding new parameters — new params added to APVTS get their default values if absent from old saves, which is the correct behavior.

---

## Critical Bugs

### BUG 1 — Engine slot selections are never saved or restored

**Severity: Critical. Every Studio One session save/load will lose the engine layout.**

**Evidence:**
`getStateInformation` (line 1557–1568) only saves `apvts.copyState()` + MIDI learn JSON. There is no code saving which engine is in each of the 4 slots.

`setStateInformation` (line 1571–1589) only calls `apvts.replaceState()` + `midiLearnManager.fromJSON()`. There is no call to `loadEngine()`.

The APVTS parameter layout (`createParameterLayout`, line 467–638) contains no `xo_slot_0_engine`, `xo_slot_1_engine`, etc. parameters. Engine slot selection is purely runtime state managed by `engines[4]` (atomic shared_ptr array) — it is never written into the APVTS ValueTree.

**What happens in Studio One:**
1. User loads XOceanus, selects OceanDeep → Orbit → Oware → Obrix in slots 0–3
2. User saves the session
3. User closes and reopens Studio One
4. XOceanus loads: all 4 slots are **empty** (null engines, silent plugin)
5. Parameter values for all engines are restored by APVTS — but no engine is loaded to use them

**Fix required** in `Source/XOceanusProcessor.cpp`:

In `getStateInformation`, after building the XML, add slot engine IDs as XML attributes:
```cpp
void XOceanusProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml)
    {
        // Save engine slot selections
        for (int i = 0; i < MaxSlots; ++i)
        {
            auto eng = std::atomic_load(&engines[i]);
            juce::String slotKey = "slot" + juce::String(i) + "Engine";
            xml->setAttribute(slotKey, eng ? eng->getEngineId() : "");
        }

        // MIDI learn mappings
        juce::String mappingsJson = juce::JSON::toString(midiLearnManager.toJSON());
        xml->setAttribute("midiLearnMappings", mappingsJson);
        copyXmlToBinary(*xml, destData);
    }
}
```

In `setStateInformation`, after `apvts.replaceState()`, restore engine slots:
```cpp
void XOceanusProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));

        // Restore engine slots — must happen AFTER apvts.replaceState()
        // so parameters are in place when attachParameters() is called
        for (int i = 0; i < MaxSlots; ++i)
        {
            juce::String slotKey = "slot" + juce::String(i) + "Engine";
            if (xml->hasAttribute(slotKey))
            {
                juce::String engineId = xml->getStringAttribute(slotKey);
                if (engineId.isNotEmpty())
                {
                    // Resolve legacy aliases (e.g. "Snap" -> "OddfeliX")
                    engineId = xoceanus::resolveEngineAlias(engineId);
                    if (EngineRegistry::instance().isRegistered(engineId.toStdString()))
                        loadEngine(i, engineId.toStdString());
                }
            }
        }

        // Restore MIDI learn mappings
        if (xml->hasAttribute("midiLearnMappings"))
        {
            juce::var mappings = juce::JSON::parse(xml->getStringAttribute("midiLearnMappings"));
            midiLearnManager.fromJSON(mappings);
        }
        else
        {
            midiLearnManager.loadDefaultMappings();
        }
    }
}
```

**Important:** `loadEngine()` currently calls `newEngine->prepare(currentSampleRate, currentBlockSize)` (line 1495). During `setStateInformation`, the processor may not yet have been through `prepareToPlay()` — `currentSampleRate` will be the default `44100.0` and `currentBlockSize` will be `512`. This is acceptable: when `prepareToPlay()` fires (which always follows `setStateInformation`), it calls `eng->prepare()` again for all active slots (lines 1078–1085). The engine will be re-prepared with the correct rate. No crash risk.

---

### BUG 2 — MegaCouplingMatrix baseline routes are never saved or restored

**Severity: Critical. User-built coupling setups vanish on session reload.**

**Evidence:**
The `MegaCouplingMatrix` stores routes in `routeList` (an `atomic<shared_ptr<vector<CouplingRoute>>>`). This is pure runtime state. `getStateInformation` never calls `couplingMatrix.getRoutes()`. `setStateInformation` never calls `couplingMatrix.addRoute()`.

The performance overlay routes (`cp_r1_active`, etc.) are APVTS params and **are** saved. But the "baseline" routes — the ones added via the coupling strip UI (addCouplingRoute/removeCouplingRoute) — are not in the APVTS and are lost.

**What is lost specifically:**
- All `CouplingRoute` entries with `isNormalled = false` (user-created routes)
- Their `sourceSlot`, `destSlot`, `type`, `amount`, and `active` state

**Fix required** in `Source/XOceanusProcessor.cpp`:

Add a helper to serialize/deserialize `CouplingRoute` vectors into the XML state:

In `getStateInformation`, save the baseline routes:
```cpp
// Save baseline coupling routes
auto routes = couplingMatrix.getRoutes();
auto* routesElem = xml->createNewChildElement("BaselineCouplingRoutes");
for (const auto& r : routes)
{
    auto* re = routesElem->createNewChildElement("Route");
    re->setAttribute("src", r.sourceSlot);
    re->setAttribute("dst", r.destSlot);
    re->setAttribute("type", static_cast<int>(r.type));
    re->setAttribute("amount", r.amount);
    re->setAttribute("normalled", r.isNormalled);
    re->setAttribute("active", r.active);
}
```

In `setStateInformation`, restore them after loading engines:
```cpp
// Restore baseline coupling routes
couplingMatrix.clearRoutes();
if (auto* routesElem = xml->getChildByName("BaselineCouplingRoutes"))
{
    for (auto* re : routesElem->getChildIterator())
    {
        MegaCouplingMatrix::CouplingRoute r;
        r.sourceSlot  = re->getIntAttribute("src", 0);
        r.destSlot    = re->getIntAttribute("dst", 1);
        r.type        = static_cast<CouplingType>(re->getIntAttribute("type", 0));
        r.amount      = static_cast<float>(re->getDoubleAttribute("amount", 0.5));
        r.isNormalled = re->getBoolAttribute("normalled", false);
        r.active      = re->getBoolAttribute("active", true);
        // Bounds check before adding
        if (r.sourceSlot >= 0 && r.sourceSlot < MaxSlots
         && r.destSlot >= 0 && r.destSlot < MaxSlots
         && r.sourceSlot != r.destSlot)
        {
            couplingMatrix.addRoute(r);
        }
    }
}
```

**Note on CouplingType serialization:** Serializing the enum as an integer (its ordinal) is fragile if the enum order ever changes. Consider serializing as string (e.g. `"AmpToFilter"`) and deserializing via the `validCouplingTypes` table in `PresetManager.h`.

---

### BUG 3 — ChordMachine per-step sequencer data is not saved in DAW sessions

**Severity: Moderate. Session reload loses custom step sequences.**

**Evidence:**
The ChordMachine has a full sequencer with per-step on/off, velocity, and gate data. This is serialized via `chordMachine.getState()` (which produces a `juce::var` JSON object, line 540 in ChordMachine.h) — but this is only used in preset saving/loading (`applyPreset` calls `chordMachine.restoreState(preset.sequencerData)`).

The APVTS params for ChordMachine (`cm_seq_pattern`, `cm_seq_bpm`, etc.) capture the *pattern template selection* but not custom per-step overrides. If the user edits individual steps, those edits live only in `chordMachine`'s internal state and are not in the APVTS ValueTree.

`getStateInformation` does not call `chordMachine.getState()`. `setStateInformation` does not call `chordMachine.restoreState()`.

**Fix required:**

In `getStateInformation`:
```cpp
// Save ChordMachine sequencer step data
juce::String cmStateJson = juce::JSON::toString(chordMachine.getState());
xml->setAttribute("chordMachineState", cmStateJson);
```

In `setStateInformation`:
```cpp
// Restore ChordMachine sequencer step data
if (xml->hasAttribute("chordMachineState"))
{
    juce::var cmState = juce::JSON::parse(xml->getStringAttribute("chordMachineState"));
    chordMachine.restoreState(cmState);
}
```

---

## Other Findings (Non-Critical)

### FINDING 4 — No state schema version tag

**Severity: Low (future risk).**

Neither `getStateInformation` nor `setStateInformation` writes or reads a version number for the plugin state format. Adding one now (while the format is still simple) allows future-safe migration. Without it, any change to what gets saved requires heuristic detection.

**Recommended fix:**
```cpp
xml->setAttribute("stateVersion", 2);  // Bump when format changes
```

In `setStateInformation`:
```cpp
int stateVersion = xml->getIntAttribute("stateVersion", 1);  // 1 = legacy (no slot/coupling data)
if (stateVersion >= 2)
{
    // restore slot engines and coupling routes
}
```

### FINDING 5 — No default engine on fresh load

**Severity: Low (UX gap).**

When XOceanus is first inserted on a track in Studio One with no saved state, the constructor (line 400–417) does not call `loadEngine()`. All 4 slots are null. The user sees 4 blank tiles and no audio. There is no "default patch" experience.

**Recommended fix:**
```cpp
// In XOceanusProcessor constructor, after cacheParameterPointers():
loadEngine(0, "OddfeliX");   // feliX the neon tetra — engaging default
```
Or use a curated 2-engine default (e.g. OddfeliX + Orbital) that demonstrates coupling immediately.

### FINDING 6 — Current preset name is not persisted

**Severity: Low (cosmetic).**

`PresetManager::currentPreset` holds the last-loaded preset name. This is not saved to `getStateInformation`, so after session reload the preset browser will show no "current preset" highlight. The actual parameter values are restored via APVTS, so this is a display gap only.

**Fix:** Save `presetManager.getCurrentPreset().name` and `.sourceFile.getFullPathName()` as XML attributes; on restore, set `presetManager.setCurrentPreset()` if the file still exists.

### FINDING 7 — Race condition risk on restore with prepareToPlay ordering

**Severity: Low (defensive note).**

`setStateInformation` is called on the message thread. If the fix for BUG 1 calls `loadEngine()` during `setStateInformation`, and `prepareToPlay()` has not yet been called (likely in Studio One's "cold load" path), then the engine is prepared with `currentSampleRate = 44100` and `currentBlockSize = 512` (the hard-coded defaults from line 126–127).

This is safe because `prepareToPlay()` unconditionally calls `eng->prepare()` for all active slots. However, if any engine's constructor allocates based on sample rate (unusual but possible), there is a two-phase prepare that may leave internal buffers at the wrong size until `prepareToPlay` fires.

**Mitigation already in place:** `prepareToPlay` (lines 1078–1085) iterates all slots and calls `eng->prepare(sampleRate, samplesPerBlock)`. No extra fix needed, but this should be noted in code comments.

---

## Studio One Test Checklist

Work through these in order. Each test builds on the one before.

### Test 1 — Baseline round-trip (single instance)
1. Open Studio One, create a new Song
2. Insert XOceanus on an instrument track
3. Select **OceanDeep** in Slot 1 and **Orbital** in Slot 2 via the engine tiles
4. Set macro1 (CHARACTER) to ~0.75, macro3 (COUPLING) to ~0.3
5. In OceanDeep, turn `deep_macroPressure` to max; in Orbital, turn `orb_brightness` down
6. Play a few notes and confirm both engines sound
7. **Save the Song** (Ctrl+S)
8. **Close the Song** (File → Close Song)
9. **Reopen the Song** (Recent Files or File → Open)
10. **Verify:**
    - [ ] Slot 1 shows OceanDeep, Slot 2 shows Orbital
    - [ ] macro1 is at ~0.75, macro3 is at ~0.3
    - [ ] deep_macroPressure is at max
    - [ ] orb_brightness is at the set value
    - [ ] Playing a note produces the same sound as before save

### Test 2 — Coupling routes round-trip
1. With the session from Test 1 open, add a coupling route: Slot 1 → Slot 2, type AmpToFilter, amount ~0.6
2. Verify the coupling route is active and audibly affects the sound
3. Save the Song
4. Close and reopen
5. **Verify:**
    - [ ] The coupling route from Slot 1 → Slot 2 (AmpToFilter, 0.6) is present
    - [ ] The coupling is audibly active (not just visually present)

### Test 3 — Performance overlay round-trip
1. In the Coupling Performance overlay, enable cp_r1: source Slot 1, target Slot 2, type LFOToPitch, amount 0.4
2. Save the Song, close, reopen
3. **Verify:**
    - [ ] cp_r1 is active
    - [ ] Source = Slot 1, Target = Slot 2, Type = LFOToPitch, Amount = 0.4

### Test 4 — Engine swap then save
1. Load the session. Change Slot 1 from OceanDeep to **Oware**
2. Adjust a few Oware params (owr_material, owr_malletHardness)
3. Save the Song, close, reopen
4. **Verify:**
    - [ ] Slot 1 shows Oware (not OceanDeep)
    - [ ] The Oware param values match what was set

### Test 5 — Multiple instances on different tracks
1. Add a second XOceanus instance on a second instrument track
2. Load Obrix in Slot 1 of the second instance; set distinct params
3. Save the Song, close, reopen
4. **Verify:**
    - [ ] Instance 1: original engine layout and params
    - [ ] Instance 2: Obrix in Slot 1 with its own params (not contaminated by instance 1)
    - [ ] The two instances do not share state

### Test 6 — Legacy alias resolution
1. (If testing after an engine rename has occurred) Open a session saved before the rename
2. Reopen in current build
3. **Verify:**
    - [ ] The old engine name (e.g. "Snap") resolves correctly to "OddfeliX" via `resolveEngineAlias()`
    - [ ] No crash on unrecognized engine ID (should load empty slot with warning in debug build)

### Test 7 — MPC scenario (Akai MPC as controller / plugin host)
1. If using XOceanus as a plugin on MPC hardware, use the MPC's session save function
2. Save a session with a two-engine patch and coupling route
3. Power cycle the MPC (or reload the session)
4. **Verify same checklist items as Test 1–3 above**

---

## Fix Priority Order

| Priority | Bug | Estimated Impact |
|----------|-----|-----------------|
| P0 | BUG 1 — Engine slots not saved/restored | Complete session loss — plugin sounds nothing on reload |
| P0 | BUG 2 — Coupling matrix not saved/restored | All coupling work lost on reload |
| P1 | BUG 3 — ChordMachine step data not saved | Custom sequences lost |
| P2 | FINDING 4 — No schema version | Safe migration when format changes |
| P3 | FINDING 5 — No default engine on fresh insert | UX: blank tiles on first use |
| P3 | FINDING 6 — Preset name not shown after reload | Cosmetic UI gap |

All P0 fixes are in a single function pair (`getStateInformation` / `setStateInformation`) in `Source/XOceanusProcessor.cpp`. They can be landed together in one commit.
