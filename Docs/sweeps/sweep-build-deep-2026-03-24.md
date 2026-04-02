# Deep Build Verification Report — 2026-03-24

**Date:** 2026-03-24
**Branch:** main
**Plugin:** XOceanus (`aumu / Xolk / XoOx`)

---

## Step 1: Clean Rebuild

Clean `build/` removed; CMake configure ran in 13.6 seconds.

```
cmake -B build -G Ninja   →   PASS (Rive not found — Oscar animation disabled; expected)
cmake --build build       →   PASS
  Errors:   0
  Warnings: 552
```

Build completed all 133 targets including `XOceanus_AU`, `XOceanus_Standalone`, and `XOceanusTests`.

---

## Step 2: Binary Identity

### AU Component (`~/Library/Audio/Plug-Ins/Components/XOceanus.component`)

| Field | Value |
|-------|-------|
| `CFBundleName` | `XOceanus` |
| `CFBundleDisplayName` | `XOceanus` |
| `CFBundleIdentifier` | `com.xo-ox.xoceanus` |
| `CFBundleVersion` | `1.0.0` |
| `type` | `aumu` |
| `subtype` | `Xolk` |
| `manufacturer` | `XoOx` |
| `name` | `XO_OX Designs: XOceanus` |
| Architecture | `Mach-O 64-bit bundle arm64` |
| Binary size | **30 MB** (30,303,616 bytes) |
| Timestamp | `Mar 24 09:37` — matches this build session |

### Standalone App (`build/XOceanus_artefacts/Standalone/XOceanus.app`)

| Field | Value |
|-------|-------|
| `CFBundleDisplayName` | `XOceanus` |
| `CFBundleIdentifier` | `com.xo-ox.xoceanus` |
| `CFBundlePackageType` | `APPL` |
| Architecture | `Mach-O 64-bit executable arm64` |
| Binary size | **33 MB** |
| Timestamp | `Mar 24 09:37` |

Note: Standalone lives at `build/XOceanus_artefacts/Standalone/XOceanus.app` (not under a `Release/` or `Debug/` subdirectory as expected — this is a Ninja/CMake artefact path difference from Xcode builds, not an issue).

### Old XOceanus Component

`~/Library/Audio/Plug-Ins/Components/XOceanus.component` **WAS PRESENT** — removed during this sweep.

---

## Step 3: auval Validation

```
auval -v aumu Xolk XoOx
```

| Test | Result |
|------|--------|
| Open time (cold) | 76.6 ms |
| Open time (warm) | 35.8 ms |
| Init time | 1.5 ms |
| Default scope formats | PASS |
| Required properties | PASS |
| Recommended properties | PASS |
| Render (137 frames @ 96000 Hz) | PASS |
| Render (4096 frames @ 48000 Hz) | PASS |
| Render (4096 frames @ 192000 Hz) | PASS |
| Render (4096 frames @ 11025 Hz) | PASS |
| Render (512 frames @ 44100 Hz) | PASS |
| Connection semantics | PASS |
| Bad max frames (render should fail) | PASS |
| Parameter setting | PASS |
| Ramped parameter scheduling | PASS |
| MIDI | PASS |
| **Overall** | **AU VALIDATION SUCCEEDED** |

---

## Step 4: Engine Registration Count

```
grep -c "registered_\|registerEngine" Source/XOceanusProcessor.cpp
→ 73
```

Matches CLAUDE.md claim of **73 engines**. All 73 use `static bool registered_*` pattern. PASS.

---

## Step 5: Parameter Count

```
grep -c "addParameter\|addParameters" Source/XOceanusProcessor.cpp
→ 69
```

This count reflects call-sites, not total parameter count (engines register their own params internally via APVTS). Normal.

---

## Step 6: Binary Sizes

| Artefact | Size |
|---------|------|
| AU component (`~/Library/.../XOceanus.component/Contents/MacOS/XOceanus`) | 30 MB |
| Standalone app (`build/.../XOceanus.app/Contents/MacOS/XOceanus`) | 33 MB |

Size differential (AU vs Standalone) is expected — Standalone includes the JUCE GUI host shell.

---

## Step 7: Old Component Cleanup

`XOceanus.component` was still installed. **Removed** during this sweep.

```
rm -rf ~/Library/Audio/Plug-Ins/Components/XOceanus.component  → DONE
```

No `XOceanus.component` remains. Only `XOceanus.component` is installed.

---

## Warning Analysis

552 warnings across 57 source files (all from project sources, not JUCE internals except 2 pragma-message notices).

### Warning Categories

| Category | Count | Severity |
|----------|-------|----------|
| `-Wunused-variable` | 311 | Low — dead locals, cosmetic |
| `-Wunused-parameter` | 189 | Low — interface params unused in some engines |
| `-Wunused-but-set-variable` | 25 | Low — computed but discarded |
| `-Wdeprecated-declarations` | 17 | Medium — JUCE Font API, see below |
| `-Wunused-private-field` | 4 | Low |
| `-Wdangling-else` | 3 | **Medium — see below** |
| `-W#pragma-messages` | 2 | Informational (JUCE splash screen flag ignored) |
| `-Wliteral-conversion` | 1 | **Medium — see below** |

### Actionable Warnings

#### 1. `-Wliteral-conversion` — REAL BUG CANDIDATE
**File:** `Source/Core/MasterFXSequencer.h:205`

```cpp
clamp(1.0f - fastExp(...), 0.001f, 1.0f)
```

The warning fires because `clamp` is resolving to an overload with `int` range params and `0.001f` is being silently truncated to `0`. This means the minimum smooth coefficient is `0` instead of `0.001`, potentially causing a divide-by-zero or instant-snap artifact at the boundary. Needs investigation — confirm `clamp` template resolution, add explicit `0.001f` cast or use `std::clamp<float>`.

#### 2. `-Wdangling-else` — ObrixEngine.h:591
**File:** `Source/Engines/Obrix/ObrixEngine.h:591`

```cpp
else if (msg.isChannelPressure())
    for (auto& v : voices) if (v.active) v.aftertouch = ...;
else if (msg.isPitchWheel())        // ← dangling: belongs to the inner if, not outer else-if
    pitchBend_ = ...;
```

The `else if (msg.isPitchWheel())` is syntactically dangling from the inner `if (v.active)` rather than the outer `else if` chain. This is a logic bug — pitch wheel will only be processed if the last voice is active. Add braces to the aftertouch arm. Low reproduction probability (requires aftertouch + pitchbend in same block), but is a genuine correctness issue.

#### 3. `-Wdeprecated-declarations` — JUCE Font API
**Files:** `Source/Export/XPNCoverArt.h` (5 sites), `Source/UI/CouplingStrip/CouplingStripEditor.h` (4+ sites)

All deprecated `juce::Font` constructor usages (JUCE 8 renamed to `FontOptions`-based constructor) and `getStringWidthFloat` (replaced by `GlyphArrangement`/`TextLayout`). Not a correctness issue today, but will break on JUCE 9. Track for `v1.1` cleanup.

### Top Warning-Dense Engines

| Engine | Warnings |
|--------|---------|
| Ocelot | 68 |
| Osier | 30 |
| Orchard | 30 |
| Oxalis | 27 |
| Overgrow | 27 |
| Osteria | 24 |
| Opcode | 21 |

Ocelot is the clear outlier at 68 warnings — primarily unused variables and parameters. Worth a focused cleanup pass.

---

## Summary

| Check | Status | Notes |
|-------|--------|-------|
| Clean build | PASS | 0 errors |
| AU identity | PASS | Name, type, subtype, manufacturer all correct |
| Standalone identity | PASS | arm64, correct bundle ID |
| AU installed timestamp | PASS | Matches this build session |
| auval | **PASS** | All 16 subtests pass; renders at 11025/44100/48000/96000/192000 Hz |
| Engine count | PASS | 73 registered, matches spec |
| Old XOceanus.component | REMOVED | Was still installed; now gone |
| Warnings | 552 — see findings | No errors; 3 actionable items |

### P-Level Action Items from This Sweep

| ID | Priority | Description | File |
|----|----------|-------------|------|
| SW-01 | P1 | `clamp` literal-conversion: `0.001f` may truncate to `0` → investigate smooth coefficient floor | `Source/Core/MasterFXSequencer.h:205` |
| SW-02 | P1 | Dangling-else in OBRIX: pitch wheel only processed if last voice is active | `Source/Engines/Obrix/ObrixEngine.h:591` |
| SW-03 | P2 | JUCE Font deprecations (17 sites across 2 files) — will break on JUCE 9 | `XPNCoverArt.h`, `CouplingStripEditor.h` |
| SW-04 | P3 | Ocelot warning cleanup (68 warnings — worst engine by count) | `Source/Engines/Ocelot/OcelotEngine.h` |
