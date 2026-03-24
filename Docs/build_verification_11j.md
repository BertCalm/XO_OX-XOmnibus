# Build Verification Report — Round 11J

**Date:** 2026-03-14
**Round:** 11J (Prism Sweep — post-Round 10 + Round 11 source changes)
**Verified by:** Claude (claude-sonnet-4-6)

---

## Summary

| Check | Result |
|-------|--------|
| CMake configuration | PASS |
| XOlokun AU build | PASS |
| Warnings (AU target) | 7 (all pre-existing) |
| auval validation | PASS |
| Binary size | 8.9 MB |
| Build time (full) | ~38 seconds |

---

## Build Result: PASS

### CMake Configuration

Clean configuration. No errors.

```
-- Configuring done (0.8s)
-- Generating done (0.0s)
-- Build files have been written to: .../XO_OX-XOlokun/build
```

---

## Errors Found and Fixed

All errors were instances of the same root cause: `atPressure` (and in one engine also `modWheelAmount`) was declared **after** the point in `renderBlock()` where it was first used. These were introduced in Round 10's D006 aftertouch batch 4, which added aftertouch to Bob, Bite, Onset, Opal, and Ouroboros.

### Root Cause

The D006 aftertouch implementation followed a pattern where:
1. `atPressure` was used early in the function (in the parameter snapshot section)
2. The MIDI processing loop (which updates aftertouch state) appeared later
3. `aftertouch.updateBlock()` + `const float atPressure = ...` were placed after the MIDI loop

This is valid sequencing for the smoothed value (MIDI loop sets new pressure, then `updateBlock` smooths it), but the variable was referenced before it was declared in the same function scope.

### Fix Applied to Each Engine

**Pattern:** Move `aftertouch.updateBlock(numSamples)` + `const float atPressure = aftertouch.getSmoothedPressure(0)` to just before the first use of `atPressure`. Remove the duplicate declaration from its original (later) location. The MIDI loop that calls `aftertouch.setChannelPressure()` remains in place — aftertouch values take effect in the subsequent block (1-block latency, standard practice).

| Engine | File | First Use (line) | Original Declaration (line) | Fix |
|--------|------|-------------------|------------------------------|-----|
| Bob / Oblong | `BobEngine.h` | 1143 | 1162 (after MIDI loop) | Moved declaration to line 1134 (before BobMode computation) |
| Bite / Overbite | `BiteEngine.h` | 1085 | 1125 (after MIDI loop) | Moved declaration to line 1083 (before M2 BITE macro) |
| Onset | `OnsetEngine.h` | 1621 | 1731 (after MIDI loop) | Moved declaration to line 1616 (before M2 PUNCH macro) |
| Opal | `OpalEngine.h` | 1671 | 1822 (after MIDI loop) | Moved declaration to line 1661 (before param snapshot section) |
| Ouroboros | `OuroborosEngine.h` | 891 | 904 (after MIDI loop) | Moved entire MIDI loop + declaration to before "Apply coupling" section (line 855) |

**Total errors fixed:** 7 (Bob×1, Bite×1, Onset×1, Opal×1, Ouroboros×2 — two `atPressure` usages in one section)

---

## Warnings

7 unique warnings in the AU target build, all **pre-existing** before Round 11J:

| Warning | File | Status |
|---------|------|--------|
| `JUCE_DISPLAY_SPLASH_SCREEN is ignored` | JUCE internal | Pre-existing |
| `implicit conversion from 'float' to 'int'` | `MasterFXSequencer.h:203` | Pre-existing |
| `juce::Font(name, size, style)` deprecated (×5) | `XOlokunEditor.h:129-133` | Pre-existing (JUCE 8 FontOptions migration) |

No new warnings introduced by Round 11 changes.

---

## XPNExporter Test Target

The `XOlokunTests` target (XPNExportTests.cpp) has 2 pre-existing errors:

| Error | Location | Cause |
|-------|----------|-------|
| `no type named 'WavAudioFormat' in namespace 'juce'` | `XPNExporter.h:633` | `juce_audio_formats` module not linked in test target |
| `no member named 'AudioFormatWriter' in namespace 'juce'` | `XPNExporter.h:634` | Same missing module |

These errors existed prior to Round 11J (documented in `build_verification_8h.md`). They do not affect the AU plugin build. Deferred to a dedicated test infrastructure fix round.

---

## auval Result: PASS

```
auval -v aumu Xomn XoOx
```

Note: four-char codes in Info.plist are case-sensitive: `Xomn` (subtype) + `XoOx` (manufacturer). The codes documented elsewhere as `XOmn`/`XOox` do not match the binary — use `Xomn`/`XoOx` for auval.

```
* * PASS
--------------------------------------------------
AU VALIDATION SUCCEEDED.
--------------------------------------------------
```

All auval tests passed:
- Render tests at 22050, 44100, 48000, 96000, 192000, 11025 Hz
- 1-channel test
- Bad max frames rejection
- Parameter setting (AudioUnitSetParameter + AudioUnitScheduleParameter)
- Ramped parameter scheduling
- MIDI test

---

## Binary

| Artifact | Size |
|----------|------|
| `XOlokun.component/Contents/MacOS/XOlokun` (AU binary) | 8.9 MB |
| `XOlokun.component` (full bundle) | 8.9 MB |

**Installed to:** `/Users/joshuacramblet/Library/Audio/Plug-Ins/Components/XOlokun.component`

**Build time:** ~38 seconds (full rebuild of AU target from modified source files)

---

## Files Modified This Round

| File | Change |
|------|--------|
| `Source/Engines/Bob/BobEngine.h` | Moved `atPressure` declaration before first use |
| `Source/Engines/Bite/BiteEngine.h` | Moved `atPressure` declaration before first use |
| `Source/Engines/Onset/OnsetEngine.h` | Moved `atPressure` declaration before first use |
| `Source/Engines/Opal/OpalEngine.h` | Moved `atPressure` declaration before first use |
| `Source/Engines/Ouroboros/OuroborosEngine.h` | Moved MIDI loop + `atPressure` declaration before effectiveChaos/effectiveLeash computation |
