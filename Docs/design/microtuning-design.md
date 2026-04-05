# Microtuning / Scala Scale Support — Design Document
**Issue:** #722
**Status:** DESIGN STUB — not yet implemented in UI or per-engine wiring
**Date:** 2026-04-05

---

## 1. Existing Implementation

A fully functional `TuningSystem` class already exists at:

```
Source/Future/Sampler/TuningSystem.h
```

It is **production-ready DSP** but is currently only used by the Sampler subsystem
in `Source/Future/`. It has NOT yet been wired into the main engine fleet.

### What TuningSystem already provides

| Feature | Status |
|---------|--------|
| 12-TET equal temperament (default) | DONE |
| 5-limit Just Intonation | DONE |
| 3-limit Pythagorean intonation | DONE |
| Quarter-comma Meantone | DONE |
| 24-TET Arabic Maqam (quarter tones) | DONE |
| Scala `.scl` file parser (arbitrary n-TET, rational intervals, cents) | DONE |
| Scala `.kbm` keyboard mapping parser | DONE |
| 128-entry precomputed frequency table (zero transcendental math on audio thread) | DONE |
| Thread-safety contract (write on prepare/non-audio thread, read on audio thread) | DONE |
| Denormal protection (clamped to 8 Hz–25,600 Hz) | DONE |

### Key API (from TuningSystem.h)

```cpp
TuningSystem tuning;

// Non-audio thread (prepare or preset load):
tuning.setReferenceFrequency(440.0f);  // default A4
tuning.setEqual12();                   // reset to 12-TET
tuning.setJustIntonation();            // 5-limit JI
tuning.setPythagorean();               // 3-limit
tuning.setMeantone();                  // quarter-comma meantone
tuning.set24TET();                     // Arabic maqam
tuning.loadScalaFile(data, len);       // parse .scl text
tuning.loadKeyboardMapping(data, len); // parse .kbm text

// Audio thread (table lookup — safe):
float freq = tuning.noteToFrequency(midiNote);
// With pitch bend (PitchBendUtil converts norm→semitones):
float freq = tuning.noteToFrequency((float)midiNote + bendSemitones);
```

References:
- Scala .scl format: https://www.huygens-fokker.org/scala/scl_format.html
- Scala .kbm format: https://www.huygens-fokker.org/scala/help.htm#mappings

---

## 2. Integration Plan — Per-Engine Wiring

### 2.1 SynthEngine interface change

Add a `setTuning(const TuningSystem&)` method to the `SynthEngine` base interface
(`Source/Core/SynthEngine.h`). Each engine stores a local `TuningSystem` instance.

```cpp
// In SynthEngine.h:
virtual void setTuning(const TuningSystem& t) { tuning_ = t; }

// Default implementation in base class:
TuningSystem tuning_;  // default-constructed = 12-TET
```

Engines that do their own pitch calculation (e.g. Obiont, Opera, Overtone) must
override `setTuning()` and update their internal pitch tables.

### 2.2 XOceanusProcessor routing

`XOceanusProcessor` holds a global `TuningSystem` per slot (or one shared instance
depending on coupling requirements). When tuning changes:

1. Call `tuning_.loadScalaFile(...)` or `tuning_.setJustIntonation()` etc. on the
   message thread (prepare-time or user interaction).
2. Broadcast via `setTuning()` to all 4 engine slots.
3. Engines update their per-note frequencies before the next block.

### 2.3 Preset storage (.xometa)

Add an optional `"tuning"` block to the `.xometa` schema:

```json
{
  "name": "Acid Foundation",
  ...
  "tuning": {
    "mode": "scala",
    "sclContent": "! 19tet.scl\n19 equal divisions of the octave\n19\n...",
    "kbmContent": null,
    "referenceNote": 69,
    "referenceFreqHz": 440.0
  }
}
```

`"mode"` values: `"12tet"` (default, omit field), `"ji5"`, `"pythagorean"`,
`"meantone"`, `"24tet"`, `"scala"`.

When `"mode"` is absent, the engine uses 12-TET (fully backward-compatible).

---

## 3. UI — Scale Selection Panel

### 3.1 Location

Add a "Tuning" tab to `SettingsPanel` (Column C, Tab 7) or as a sub-panel in
each `EngineDetailPanel`. The SettingsPanel approach is simpler and affects all
engines uniformly; the EngineDetailPanel approach enables per-engine tuning.

**Recommendation:** Start with a global tuning panel in SettingsPanel (simpler,
ships faster). Per-engine tuning is a V1.1 feature.

### 3.2 SettingsPanel additions

```
── TUNING ───────────────────────────────────────
  Scale:      [ 12-TET ▼ ]   (dropdown)
              Options: 12-TET, Just Intonation (5-limit),
                       Pythagorean, Quarter-comma Meantone,
                       24-TET (Maqam), Custom (.scl file)
  Reference:  A4 = [ 440.0 ] Hz
  [Load .scl file...]   [Load .kbm file...]
  [Reset to 12-TET]
────────────────────────────────────────────────
```

### 3.3 Component hierarchy

```
SettingsPanel
  └── TuningSection (new component or inline in SettingsPanel)
        ├── juce::ComboBox  scaleBox
        ├── juce::Slider    referenceFreqSlider   (432–446 Hz)
        ├── juce::TextButton loadSclBtn
        ├── juce::TextButton loadKbmBtn
        ├── juce::Label     activeScaleLabel
        └── juce::TextButton resetBtn
```

Persistence: store the active mode + serialized .scl content in the PropertiesFile
under keys `"tuningMode"` and `"tuningScalaData"`.

---

## 4. Scala .scl File Format (reference)

```
! filename.scl
! Comment lines start with !
Description of the scale
 7       <- number of notes (not counting 0/1 = unison)
!
 9/8     <- ratio (fraction)
 193.157 <- cents
 5/4
 498.045
 3/2
 889.358
 2/1     <- octave (MUST be last; ratio ≥ 1 or exactly 2/1)
```

Parser in `TuningSystem::loadScalaFile()` handles:
- Cents notation (e.g. `100.0`)
- Ratio notation (e.g. `3/2`)
- Integer notation (treated as ratio N/1)
- Comment lines (`!`)
- Whitespace-only lines

### Known Scala scale collections

- Scale Archive: https://www.huygens-fokker.org/scala/downloads.html (~5,000 scales)
- Sevish Scales: https://sevish.com/scaleworkshop/
- Rationale by Marc Sabat: JI lattice-based tunings

---

## 5. Keyboard Mapping (.kbm) Format (reference)

```
! keyboard mapping
! size of map (usually 12 for standard keyboard)
12
! first MIDI note to be mapped
0
! last MIDI note to be mapped
127
! MIDI note for 1/1 (middle C = 60)
60
! reference frequency (A4 = 69 in standard mapping)
69
! reference frequency in Hz
440.0
! octave period (scale degree that represents the octave)
12
! the mapping (degree for each MIDI note, -1 = unmapped)
0
1
2
3
4
5
6
7
8
9
10
11
```

The `.kbm` parser in `TuningSystem::loadKeyboardMapping()` implements the full
Huygens-Fokker specification including unmapped notes (-1) and non-12 period sizes.

---

## 6. Per-Engine Considerations

### Engines with pitch generated from MIDI noteNumber (most engines)
These engines call a frequency lookup or `mtof()` equivalent. Integration:
- Replace `mtof(noteNumber)` with `tuning_.noteToFrequency(noteNumber)`
- No other changes required.

### Engines with internal pitch generation (not from MIDI noteNumber)
These engines generate their own pitch sequences independent of MIDI:
- **Organism** (cellular automata pitch grid): tuning maps scale degrees → freq
- **Opera** (Kuramoto partials): partial frequencies may need tuning remapping
- **Overtone** (continued fraction spectral): inherently non-12-TET; tuning as offset
- **Oracle** (GENDY): stochastic frequencies; global pitch offset only recommended
- **Ostinato** (world rhythms): rhythm-based, pitch is secondary; tuning applicable

For the above engines, implement tuning as a global pitch-offset table rather than
full scale remapping. Full integration is V1.1 scope.

### Coupling and microtuning
When two engines are coupled (MegaCouplingMatrix), both should use the same tuning
to avoid beating artifacts unless deliberately de-tuned. The `XOceanusProcessor`
should share one TuningSystem per slot pair, not per-engine, for coupled engines.

---

## 7. Implementation Phases

### Phase 0 — Foundation (already done)
- `TuningSystem.h` — complete, production-ready, in `Source/Future/Sampler/`

### Phase 1 — Engine Integration (V1.0 target)
- Move or reference `TuningSystem.h` from `Source/Future/Sampler/` into `Source/DSP/`
- Add `tuning_` member and `setTuning()` to `SynthEngine` base class
- Wire `noteToFrequency()` into the 50 simplest engines (those with direct MIDI→freq)
- Add global tuning UI to SettingsPanel (5 built-in modes + .scl file loader)
- Persist active tuning mode in PropertiesFile

### Phase 2 — Preset-Level Tuning (V1.1)
- Add `"tuning"` block to .xometa schema
- Serialize/deserialize .scl content into presets
- Per-engine tuning in EngineDetailPanel

### Phase 3 — Per-Engine Tuning (V2.0)
- Independent tuning per engine slot
- Coupling-aware tuning sync
- In-app scale editor (visual keyboard + cent deviations)
- Bundled Scala scale library (20–50 curated scales)

---

## 8. Non-Goals (V1.0)

- In-app Scala file editor
- Microtuning via MIDI pitch bend per-note (MPE) — this is separate from global tuning
- Adaptive Just Intonation (dynamic retuning based on chord context)
- Xenharmonic interval recognition in UI labels

---

## 9. Files to Change (Phase 1)

| File | Change |
|------|--------|
| `Source/Future/Sampler/TuningSystem.h` | Move to `Source/DSP/TuningSystem.h` |
| `Source/Core/SynthEngine.h` | Add `TuningSystem tuning_` + `setTuning()` |
| `Source/XOceanusProcessor.h/.cpp` | Hold per-slot TuningSystem, broadcast `setTuning()` |
| `Source/UI/Gallery/SettingsPanel.h` | Add TuningSection UI |
| `Source/Core/PresetManager.h` | Parse optional `"tuning"` block in .xometa |
| `CMakeLists.txt` | Include new DSP path if TuningSystem is moved |

---

## 10. Testing

- Build with `cmake --build build` and run `auval -v aumu Xocn XoOx`
- Test: load a JI tuning and play root-position C major triad — major third should
  be beatless (pure 5/4).
- Test: load a 24-TET .scl file, play chromatic scale, verify quarter-tone intervals.
- Test: load a preset with embedded .scl, reload it — tuning must restore correctly.
- Test: disable tuning (reset to 12-TET), verify all engines return to standard pitch.
