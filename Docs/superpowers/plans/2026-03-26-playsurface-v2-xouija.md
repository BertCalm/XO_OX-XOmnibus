# PlaySurface V2 — XOuija Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the V1 OrbitPathZone + PerformancePads with the XOuija harmonic navigation system (B041/B042/B043), add KEYS mode, and wire CC 85-90 output — transforming PlaySurface from a pad controller into a harmonic performance instrument.

**Architecture:** Four new headers (HarmonicField, GestureTrailBuffer, XOuijaPanel, KeysMode) integrate into the existing monolithic PlaySurface.h. Pure-logic modules (HarmonicField, GestureTrailBuffer) are JUCE-free and fully testable. XOuijaPanel contains the Planchette, GestureButtonBar, and GoodbyeButton as nested classes. The processor gains a lock-free CC output queue. Layout restructures from 4-zone grid to XOuija-left / NoteInput-right / Strip-bottom.

**Tech Stack:** JUCE 8 (Component, Timer, Graphics, Path, AffineTransform), C++17, custom test framework (reportTest pattern), CMake/Ninja

**Spec:** `Docs/specs/2026-03-26-playsurface-v2-xouija-design.md` (canonical source of truth)

---

## Dependency Graph

```
Wave 1 (parallel — no deps):
  Task 1: HarmonicField ─────────┐
  Task 2: GestureTrailBuffer ────┤
  Task 3: CC Output Queue ───────┤
                                  │
Wave 2 (sequential within XOuija):│
  Task 4: XOuijaPanel Surface ←──┘ (depends on Task 1)
  Task 5: Planchette ← Task 4
  Task 6: Trail Rendering + CC ← Tasks 2, 3, 5
  Task 7: GestureButtonBar + GOODBYE ← Task 4

Wave 3 (parallel — depend on Task 1):
  Task 8: KeysMode ← Task 1
  Task 9: PAD XOuija Coloring ← Task 1

Wave 4 (integration — depends on all above):
  Task 10: PerformanceStrip V2
  Task 11: PlaySurface V2 Layout ← Tasks 4-10
  Task 12: Full CC Wiring ← Tasks 3, 11
  Task 13: Build Verification ← Task 12
```

---

## File Structure

| Action | Path | Responsibility |
|--------|------|---------------|
| CREATE | `Source/UI/PlaySurface/HarmonicField.h` | Circle-of-fifths math, tension coloring, scale membership. No JUCE dependency. |
| CREATE | `Source/UI/PlaySurface/GestureTrailBuffer.h` | 256-tuple ring buffer, freeze/replay, two-trail interference. No JUCE dependency. |
| CREATE | `Source/UI/PlaySurface/XOuijaPanel.h` | XOuija surface + Planchette + GestureButtonBar + GoodbyeButton. All JUCE components. |
| CREATE | `Source/UI/PlaySurface/KeysMode.h` | Seaboard-style 2-octave keyboard with scroll and expression. |
| MODIFY | `Source/UI/PlaySurface/PlaySurface.h` | Remove OrbitPathZone + PerformancePads, restructure layout, integrate XOuija + KeysMode. |
| MODIFY | `Source/XOceanusProcessor.h` | Add CC output queue struct + push/drain methods, flip `producesMidi()` to true. |
| MODIFY | `Source/XOceanusProcessor.cpp` | Drain CC queue at end of `processBlock()`. |
| CREATE | `Tests/PlaySurfaceTests/HarmonicFieldTests.h` | Test declarations for HarmonicField. |
| CREATE | `Tests/PlaySurfaceTests/HarmonicFieldTests.cpp` | Full TDD tests for circle-of-fifths logic. |
| CREATE | `Tests/PlaySurfaceTests/GestureTrailTests.h` | Test declarations for GestureTrailBuffer. |
| CREATE | `Tests/PlaySurfaceTests/GestureTrailTests.cpp` | Full TDD tests for trail buffer + freeze + interference. |
| MODIFY | `Tests/run_tests.cpp` | Add `playsurface_tests::runAll()` to test runner. |
| MODIFY | `CMakeLists.txt` | Add new test source files to XOceanusTests target. |

---

## Wave 1: Data Foundation (Pure Logic, TDD)

### Task 1: HarmonicField

**Files:**
- Create: `Source/UI/PlaySurface/HarmonicField.h`
- Create: `Tests/PlaySurfaceTests/HarmonicFieldTests.h`
- Create: `Tests/PlaySurfaceTests/HarmonicFieldTests.cpp`
- Modify: `Tests/run_tests.cpp`
- Modify: `CMakeLists.txt`

This module encodes all circle-of-fifths math, tension coloring, and scale membership. It has zero JUCE dependencies — pure C++ with `<cmath>`, `<array>`, `<algorithm>`. Colors are stored as RGB float triples (converted to `juce::Colour` at the call site).

- [ ] **Step 1: Write the test file header**

Create `Tests/PlaySurfaceTests/HarmonicFieldTests.h`:

```cpp
#pragma once

namespace playsurface_tests {
    int runAll();
}
```

- [ ] **Step 2: Write the failing tests**

Create `Tests/PlaySurfaceTests/HarmonicFieldTests.cpp`:

```cpp
#include "HarmonicFieldTests.h"
#include "../../Source/UI/PlaySurface/HarmonicField.h"
#include <iostream>
#include <cmath>

namespace playsurface_tests {

static int g_passed = 0;
static int g_failed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed) { std::cout << "  [PASS] " << name << "\n"; g_passed++; }
    else        { std::cout << "  [FAIL] " << name << "\n"; g_failed++; }
}

static void testCircleOfFifths()
{
    std::cout << "\n--- HarmonicField: Circle of Fifths ---\n";
    using namespace xoceanus;

    // C is at center (position 0.5)
    reportTest("C at center X=0.5",
        HarmonicField::positionToKey(0.5f) == 0); // 0 = C

    // G is one fifth sharp (right of center)
    int keyAtRight = HarmonicField::positionToKey(0.5f + 1.0f/12.0f);
    reportTest("G is one fifth right of C",
        keyAtRight == 7); // 7 = G in semitones

    // F is one fifth flat (left of center)
    int keyAtLeft = HarmonicField::positionToKey(0.5f - 1.0f/12.0f);
    reportTest("F is one fifth left of C",
        keyAtLeft == 5); // 5 = F

    // Extremes: Gb/F# at edges
    reportTest("F#/Gb at rightmost",
        HarmonicField::positionToKey(1.0f) == 6); // 6 = F#
    reportTest("Gb/F# at leftmost",
        HarmonicField::positionToKey(0.0f) == 6); // 6 = Gb (enharmonic)

    // Round-trip: key to position to key
    for (int fifths = -6; fifths <= 6; ++fifths)
    {
        float pos = HarmonicField::fifthsOffsetToPosition(fifths);
        int recoveredFifths = HarmonicField::positionToFifthsOffset(pos);
        char buf[64];
        snprintf(buf, sizeof(buf), "round-trip fifths offset %d", fifths);
        reportTest(buf, recoveredFifths == fifths);
    }
}

static void testFifthsDistance()
{
    std::cout << "\n--- HarmonicField: Fifths Distance ---\n";
    using namespace xoceanus;

    reportTest("C to C = 0", HarmonicField::fifthsDistance(0, 0) == 0);
    reportTest("C to G = 1", HarmonicField::fifthsDistance(0, 7) == 1);
    reportTest("C to F = 1", HarmonicField::fifthsDistance(0, 5) == 1);
    reportTest("C to D = 2", HarmonicField::fifthsDistance(0, 2) == 2);
    reportTest("C to F# = 6", HarmonicField::fifthsDistance(0, 6) == 6);
    reportTest("C to Gb = 6", HarmonicField::fifthsDistance(0, 6) == 6);
    // Symmetric
    reportTest("G to C = 1", HarmonicField::fifthsDistance(7, 0) == 1);
}

static void testTensionColor()
{
    std::cout << "\n--- HarmonicField: Tension Coloring ---\n";
    using namespace xoceanus;

    auto [r0, g0, b0] = HarmonicField::tensionColor(0); // home = teal
    reportTest("home key is teal (r < 0.2)",  r0 < 0.25f);
    reportTest("home key is teal (g > 0.5)",  g0 > 0.5f);
    reportTest("home key is teal (b > 0.4)",  b0 > 0.4f);

    auto [r6, g6, b6] = HarmonicField::tensionColor(6); // max = warm red
    reportTest("max tension is warm red (r > 0.7)", r6 > 0.7f);
    reportTest("max tension is warm red (b < 0.5)", b6 < 0.5f);

    auto [r3, g3, b3] = HarmonicField::tensionColor(3); // mid = XO Gold
    reportTest("mid tension is gold (r > 0.8)", r3 > 0.8f);
    reportTest("mid tension is gold (g > 0.6)", g3 > 0.6f);
}

static void testScaleMembership()
{
    std::cout << "\n--- HarmonicField: Scale Membership ---\n";
    using namespace xoceanus;

    // C major: C D E F G A B
    reportTest("C in C major",  HarmonicField::isInKey(60, 0)); // C4
    reportTest("D in C major",  HarmonicField::isInKey(62, 0)); // D4
    reportTest("E in C major",  HarmonicField::isInKey(64, 0)); // E4
    reportTest("F in C major",  HarmonicField::isInKey(65, 0)); // F4
    reportTest("C# not in C major", !HarmonicField::isInKey(61, 0)); // C#4
    reportTest("F# not in C major", !HarmonicField::isInKey(66, 0)); // F#4

    // G major: G A B C D E F#
    reportTest("F# in G major", HarmonicField::isInKey(66, 7)); // F#4
    reportTest("F not in G major", !HarmonicField::isInKey(65, 7)); // F4
}

static void testQuantizeToNearest()
{
    std::cout << "\n--- HarmonicField: Quantize to Nearest ---\n";
    using namespace xoceanus;

    // C# in C major should quantize to C or D
    int q1 = HarmonicField::quantizeToNearest(61, 0); // C#4 in C major
    reportTest("C#4 quantizes to C4 or D4 in C major",
        q1 == 60 || q1 == 62);

    // F# in C major should quantize to F or G
    int q2 = HarmonicField::quantizeToNearest(66, 0); // F#4 in C major
    reportTest("F#4 quantizes to F4 or G4 in C major",
        q2 == 65 || q2 == 67);

    // Already in key: no change
    reportTest("C4 stays C4 in C major",
        HarmonicField::quantizeToNearest(60, 0) == 60);
}

static void testMarkerProperties()
{
    std::cout << "\n--- HarmonicField: Marker Properties ---\n";
    using namespace xoceanus;

    // Home key marker: 100% size, 100% opacity
    auto [size0, opacity0] = HarmonicField::markerProperties(0);
    reportTest("home marker size = 1.0", std::abs(size0 - 1.0f) < 0.01f);
    reportTest("home marker opacity = 1.0", std::abs(opacity0 - 1.0f) < 0.01f);

    // 1-2 fifths: 85% size, 75% opacity
    auto [size1, opacity1] = HarmonicField::markerProperties(1);
    reportTest("1-fifth marker size ~0.85", std::abs(size1 - 0.85f) < 0.01f);
    reportTest("1-fifth marker opacity ~0.75", std::abs(opacity1 - 0.75f) < 0.01f);

    // 5-6 fifths: 55% size, 35% opacity
    auto [size6, opacity6] = HarmonicField::markerProperties(6);
    reportTest("6-fifth marker size ~0.55", std::abs(size6 - 0.55f) < 0.01f);
    reportTest("6-fifth marker opacity ~0.35", std::abs(opacity6 - 0.35f) < 0.01f);
}

int runAll()
{
    std::cout << "\n========== PlaySurface Tests ==========\n";
    g_passed = 0;
    g_failed = 0;

    testCircleOfFifths();
    testFifthsDistance();
    testTensionColor();
    testScaleMembership();
    testQuantizeToNearest();
    testMarkerProperties();

    std::cout << "\nPlaySurface: " << g_passed << " passed, "
              << g_failed << " failed\n";
    return g_failed;
}

} // namespace playsurface_tests
```

- [ ] **Step 3: Wire tests into the test runner**

Modify `Tests/run_tests.cpp` — add include and call:

```cpp
// Add at top with other includes:
#include "PlaySurfaceTests/HarmonicFieldTests.h"

// Add inside main(), after existing runAll() calls:
totalFailed += playsurface_tests::runAll();
```

Modify `CMakeLists.txt` — add the new test source files to the `XOceanusTests` target's source list:

```cmake
# Add to XOceanusTests target_sources:
Tests/PlaySurfaceTests/HarmonicFieldTests.cpp
```

- [ ] **Step 4: Build and verify tests fail**

```bash
cd ~/Documents/GitHub/XO_OX-XOceanus
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build 2>&1 | tail -20
```

Expected: Compilation fails because `HarmonicField.h` does not exist yet.

- [ ] **Step 5: Implement HarmonicField.h**

Create `Source/UI/PlaySurface/HarmonicField.h`:

```cpp
#pragma once

#include <array>
#include <cmath>
#include <algorithm>
#include <tuple>

namespace xoceanus {

// Pure-logic harmonic field utilities for XOuija.
// No JUCE dependency — colors returned as RGB float triples.
// Spec: Docs/specs/2026-03-26-playsurface-v2-xouija-design.md Sections 3.2–3.5

struct HarmonicField
{
    // ── Circle of Fifths Mapping ──
    // The 13 positions on the X-axis map to fifths offsets -6..+6 from C.
    // Position 0.0 = Gb (offset -6), 0.5 = C (offset 0), 1.0 = F# (offset +6).
    // Semitone values: C=0, C#=1, D=2, ... B=11.

    // Fifths order in semitones: F#(6), Db(1), Ab(8), Eb(3), Bb(10), F(5),
    //                            C(0), G(7), D(2), A(9), E(4), B(11), F#(6)
    static constexpr std::array<int, 13> kFifthsSemitones = {
        6, 1, 8, 3, 10, 5, 0, 7, 2, 9, 4, 11, 6
    };
    // Corresponding fifths offsets: -6, -5, -4, -3, -2, -1, 0, +1, +2, +3, +4, +5, +6
    static constexpr std::array<int, 13> kFifthsOffsets = {
        -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6
    };

    // Note names for display (sharps preferred on right, flats on left)
    static constexpr const char* kNoteNames[13] = {
        "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#"
    };

    // X position (0.0–1.0) → nearest fifths offset (-6 to +6)
    static int positionToFifthsOffset(float x)
    {
        float idx = x * 12.0f; // 0..12 mapping to 13 positions (indices 0..12)
        int i = static_cast<int>(std::round(std::clamp(idx, 0.0f, 12.0f)));
        return kFifthsOffsets[static_cast<size_t>(i)];
    }

    // X position (0.0–1.0) → semitone key (0=C, 1=C#, ... 11=B)
    static int positionToKey(float x)
    {
        float idx = x * 12.0f;
        int i = static_cast<int>(std::round(std::clamp(idx, 0.0f, 12.0f)));
        return kFifthsSemitones[static_cast<size_t>(i)];
    }

    // Fifths offset (-6 to +6) → X position (0.0–1.0)
    static float fifthsOffsetToPosition(int offset)
    {
        int clamped = std::clamp(offset, -6, 6);
        return static_cast<float>(clamped + 6) / 12.0f;
    }

    // Semitone key → fifths offset from C (shortest path, range 0–6)
    static int semitoneToFifthsFromC(int semitone)
    {
        // Fifths from C: C=0, G=1, D=2, A=3, E=4, B=5, F#=6, Db=5, Ab=4, Eb=3, Bb=2, F=1
        constexpr int kTable[12] = { 0, 5, 2, 3, 4, 1, 6, 1, 4, 3, 2, 5 };
        return kTable[((semitone % 12) + 12) % 12];
    }

    // Distance in fifths between two keys (0–6, symmetric)
    static int fifthsDistance(int keyA, int keyB)
    {
        // Build a lookup: for each semitone, its signed fifths offset from C
        // Then compute shortest circular distance
        constexpr int kSigned[12] = { 0, -5, 2, -3, 4, -1, 6, 1, -4, 3, -2, 5 };
        int a = kSigned[((keyA % 12) + 12) % 12];
        int b = kSigned[((keyB % 12) + 12) % 12];
        int diff = std::abs(a - b);
        return std::min(diff, 12 - diff);
    }

    // ── Tension Coloring ──
    // Teal #2A9D8F → XO Gold #E9C46A → Warm Red #E07A5F
    // Distance 0-2: teal→gold, 3-4: gold, 5-6: gold→red

    static constexpr float kTeal[3]    = { 0.165f, 0.616f, 0.561f }; // #2A9D8F
    static constexpr float kGold[3]    = { 0.914f, 0.788f, 0.416f }; // #E9C46A
    static constexpr float kWarmRed[3] = { 0.878f, 0.478f, 0.373f }; // #E07A5F

    static std::tuple<float, float, float> tensionColor(int fifthsDist)
    {
        float d = static_cast<float>(std::clamp(fifthsDist, 0, 6));
        float r, g, b;
        if (d <= 3.0f)
        {
            float t = d / 3.0f; // 0..1 across teal→gold
            r = kTeal[0] + t * (kGold[0] - kTeal[0]);
            g = kTeal[1] + t * (kGold[1] - kTeal[1]);
            b = kTeal[2] + t * (kGold[2] - kTeal[2]);
        }
        else
        {
            float t = (d - 3.0f) / 3.0f; // 0..1 across gold→red
            r = kGold[0] + t * (kWarmRed[0] - kGold[0]);
            g = kGold[1] + t * (kWarmRed[1] - kGold[1]);
            b = kGold[2] + t * (kWarmRed[2] - kGold[2]);
        }
        return { r, g, b };
    }

    // ── Marker Properties ──
    // Spec Section 3.4: size/opacity by distance from home key

    static std::pair<float, float> markerProperties(int fifthsDist)
    {
        int d = std::clamp(fifthsDist, 0, 6);
        // 0 → (1.0, 1.0), 1-2 → (0.85, 0.75), 3-4 → (0.70, 0.50), 5-6 → (0.55, 0.35)
        constexpr float kSize[4]    = { 1.00f, 0.85f, 0.70f, 0.55f };
        constexpr float kOpacity[4] = { 1.00f, 0.75f, 0.50f, 0.35f };
        int bracket = (d == 0) ? 0 : std::min((d - 1) / 2 + 1, 3);
        return { kSize[bracket], kOpacity[bracket] };
    }

    // Marker vertical arc offset: center notes sit lower, edges sit higher.
    // Amplitude ±8px. Index 0 and 12 are at +8, index 6 (C) is at -8.
    static float markerArcOffset(int fifthsIdx, float amplitude = 8.0f)
    {
        // fifthsIdx: 0..12 (left to right)
        float center = 6.0f;
        float normalized = (static_cast<float>(fifthsIdx) - center) / center; // -1..+1
        return normalized * normalized * amplitude - amplitude; // parabola: edges high, center low
        // At edges (±1): 0. At center (0): -amplitude. Wait, spec says edges HIGH, center LOW.
        // Let me recalculate: edges = +amp, center = -amp
        // y = amp * (norm^2 * 2 - 1) ... no, simpler:
        // y = amplitude * (normalized * normalized) - amplitude * 0  ...
        // Actually: at idx=0: norm=-1, norm^2=1 → 1*amp - amp = 0. That's wrong.
        // Correct formula: y = amplitude * (abs(norm) - 0.5) * 2
        // No. Let me just use: center at -amp, edges at +amp.
        // y = amplitude * (2 * norm^2 - 1)
        // At norm=0: -amplitude. At norm=±1: +amplitude.
    }

    // Corrected arc offset
    static float markerArcY(int fifthsIdx, float amplitude = 8.0f)
    {
        float center = 6.0f;
        float normalized = (static_cast<float>(std::clamp(fifthsIdx, 0, 12)) - center) / center;
        return amplitude * (2.0f * normalized * normalized - 1.0f);
        // idx=0 or 12: norm=±1 → 2*1-1 = +amplitude (high)
        // idx=6 (C):   norm=0  → 2*0-1 = -amplitude (low)
    }

    // ── Scale Membership ──
    // Major scale intervals from root: W W H W W W H (0,2,4,5,7,9,11)

    static constexpr int kMajorScale[7] = { 0, 2, 4, 5, 7, 9, 11 };

    // Is this MIDI note in the major scale of the given root key?
    static bool isInKey(int midiNote, int rootKey)
    {
        int pc = ((midiNote - rootKey) % 12 + 12) % 12;
        for (int i = 0; i < 7; ++i)
            if (kMajorScale[i] == pc) return true;
        return false;
    }

    // Is this pitch class the root of the given key?
    static bool isRoot(int midiNote, int rootKey)
    {
        return ((midiNote % 12) + 12) % 12 == ((rootKey % 12) + 12) % 12;
    }

    // Quantize a MIDI note to the nearest in-key note
    static int quantizeToNearest(int midiNote, int rootKey)
    {
        if (isInKey(midiNote, rootKey)) return midiNote;
        // Search outward ±1, ±2, ... until we find an in-key note
        for (int offset = 1; offset <= 6; ++offset)
        {
            if (isInKey(midiNote - offset, rootKey)) return midiNote - offset;
            if (isInKey(midiNote + offset, rootKey)) return midiNote + offset;
        }
        return midiNote; // fallback (should never reach here for 7-note scale)
    }
};

} // namespace xoceanus
```

**Note:** The `markerArcOffset` method has a bug (dead code). Delete it — `markerArcY` is the correct implementation. The subagent should only include `markerArcY`.

- [ ] **Step 6: Build and run tests — verify all pass**

```bash
cd ~/Documents/GitHub/XO_OX-XOceanus
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/XOceanusTests
```

Expected: All HarmonicField tests pass (`[PASS]` for every test).

- [ ] **Step 7: Commit**

```bash
git add Source/UI/PlaySurface/HarmonicField.h \
        Tests/PlaySurfaceTests/HarmonicFieldTests.h \
        Tests/PlaySurfaceTests/HarmonicFieldTests.cpp \
        Tests/run_tests.cpp CMakeLists.txt
git commit -m "feat(PlaySurface): add HarmonicField — circle-of-fifths math, tension coloring, scale membership"
```

---

### Task 2: GestureTrailBuffer

**Files:**
- Create: `Source/UI/PlaySurface/GestureTrailBuffer.h`
- Create: `Tests/PlaySurfaceTests/GestureTrailTests.h`
- Create: `Tests/PlaySurfaceTests/GestureTrailTests.cpp`
- Modify: `Tests/PlaySurfaceTests/HarmonicFieldTests.h` (rename namespace to shared)
- Modify: `Tests/run_tests.cpp`
- Modify: `CMakeLists.txt`

This module implements the B043 gesture trail data structure. No JUCE dependency.

- [ ] **Step 1: Write the test header**

Create `Tests/PlaySurfaceTests/GestureTrailTests.h`:

```cpp
#pragma once

namespace gesture_trail_tests {
    int runAll();
}
```

- [ ] **Step 2: Write failing tests**

Create `Tests/PlaySurfaceTests/GestureTrailTests.cpp`:

```cpp
#include "GestureTrailTests.h"
#include "../../Source/UI/PlaySurface/GestureTrailBuffer.h"
#include <iostream>
#include <cmath>

namespace gesture_trail_tests {

static int g_passed = 0;
static int g_failed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed) { std::cout << "  [PASS] " << name << "\n"; g_passed++; }
    else        { std::cout << "  [FAIL] " << name << "\n"; g_failed++; }
}

static void testPushAndCount()
{
    std::cout << "\n--- GestureTrailBuffer: Push & Count ---\n";
    using namespace xoceanus;

    GestureTrailBuffer buf;
    reportTest("empty buffer count = 0", buf.count() == 0);

    buf.push(0.5f, 0.5f, 0.3f, 1.0);
    reportTest("after 1 push, count = 1", buf.count() == 1);

    for (int i = 0; i < 300; ++i)
        buf.push(float(i) / 300.0f, 0.5f, 0.1f, double(i));

    reportTest("after 301 pushes, count capped at 256", buf.count() == 256);
}

static void testRingBufferWrap()
{
    std::cout << "\n--- GestureTrailBuffer: Ring Wrap ---\n";
    using namespace xoceanus;

    GestureTrailBuffer buf;
    // Push 256 points
    for (int i = 0; i < 256; ++i)
        buf.push(float(i) / 255.0f, 0.0f, 0.0f, double(i));

    // The oldest point should be index 0 → x = 0.0
    auto oldest = buf.oldest();
    reportTest("oldest after 256 pushes: x ~0.0", std::abs(oldest.x) < 0.01f);

    // Push one more — oldest should now be index 1 → x ~ 1/255
    buf.push(1.0f, 1.0f, 1.0f, 256.0);
    auto newOldest = buf.oldest();
    reportTest("after wrap, oldest x ~ 1/255",
        std::abs(newOldest.x - 1.0f / 255.0f) < 0.01f);
}

static void testFreeze()
{
    std::cout << "\n--- GestureTrailBuffer: Freeze ---\n";
    using namespace xoceanus;

    GestureTrailBuffer buf;
    buf.push(0.1f, 0.2f, 0.3f, 1.0);
    buf.push(0.4f, 0.5f, 0.6f, 2.0);
    buf.push(0.7f, 0.8f, 0.9f, 3.0);

    reportTest("not frozen initially", !buf.isFrozen());

    buf.freeze();
    reportTest("frozen after freeze()", buf.isFrozen());
    reportTest("frozen count = 3", buf.frozenCount() == 3);

    // Frozen replay: at t=0, should return first frozen point
    auto p = buf.frozenPointAt(0);
    reportTest("frozen point 0: x=0.1", std::abs(p.x - 0.1f) < 0.01f);

    // Unfreeze
    buf.unfreeze();
    reportTest("not frozen after unfreeze()", !buf.isFrozen());
}

static void testReplay()
{
    std::cout << "\n--- GestureTrailBuffer: Replay ---\n";
    using namespace xoceanus;

    GestureTrailBuffer buf;
    // 3 points with timestamps 0.0, 0.5, 1.0
    buf.push(0.0f, 0.0f, 0.0f, 0.0);
    buf.push(0.5f, 0.5f, 0.5f, 0.5);
    buf.push(1.0f, 1.0f, 1.0f, 1.0);

    buf.freeze();

    // Replay at normalized time 0.0 → first point
    auto r0 = buf.replayAt(0.0f);
    reportTest("replay at t=0.0: x=0.0", std::abs(r0.x) < 0.01f);

    // Replay at normalized time 0.5 → second point
    auto r1 = buf.replayAt(0.5f);
    reportTest("replay at t=0.5: x=0.5", std::abs(r1.x - 0.5f) < 0.05f);

    // Replay at normalized time 1.0 → third point (or wrap to first)
    auto r2 = buf.replayAt(1.0f);
    reportTest("replay at t=1.0: wraps to x=0.0", std::abs(r2.x) < 0.05f);
}

static void testTwoTrailInterference()
{
    std::cout << "\n--- GestureTrailBuffer: Two-Trail Interference ---\n";
    using namespace xoceanus;

    GestureTrailBuffer a, b;
    a.push(0.3f, 0.8f, 0.0f, 0.0);
    b.push(0.6f, 0.5f, 0.0f, 0.0);

    a.freeze();
    b.freeze();

    auto pa = a.frozenPointAt(0);
    auto pb = b.frozenPointAt(0);

    // Spec: X summed and clamped: x_out = clamp(x_A + x_B - 0.5, 0, 1)
    float xOut = std::clamp(pa.x + pb.x - 0.5f, 0.0f, 1.0f);
    reportTest("interference X: clamp(0.3+0.6-0.5) = 0.4",
        std::abs(xOut - 0.4f) < 0.01f);

    // Spec: Y multiplied: y_out = y_A * y_B
    float yOut = pa.y * pb.y;
    reportTest("interference Y: 0.8 * 0.5 = 0.4",
        std::abs(yOut - 0.4f) < 0.01f);

    // Test the static helper
    auto [ix, iy] = GestureTrailBuffer::interference(pa, pb);
    reportTest("interference helper X = 0.4", std::abs(ix - 0.4f) < 0.01f);
    reportTest("interference helper Y = 0.4", std::abs(iy - 0.4f) < 0.01f);
}

static void testClear()
{
    std::cout << "\n--- GestureTrailBuffer: Clear ---\n";
    using namespace xoceanus;

    GestureTrailBuffer buf;
    buf.push(0.5f, 0.5f, 0.5f, 1.0);
    buf.freeze();
    buf.clear();
    reportTest("clear resets count to 0", buf.count() == 0);
    reportTest("clear unfreezes", !buf.isFrozen());
}

int runAll()
{
    std::cout << "\n========== GestureTrail Tests ==========\n";
    g_passed = 0;
    g_failed = 0;

    testPushAndCount();
    testRingBufferWrap();
    testFreeze();
    testReplay();
    testTwoTrailInterference();
    testClear();

    std::cout << "\nGestureTrail: " << g_passed << " passed, "
              << g_failed << " failed\n";
    return g_failed;
}

} // namespace gesture_trail_tests
```

- [ ] **Step 3: Wire into test runner and CMakeLists**

Add to `Tests/run_tests.cpp`:

```cpp
#include "PlaySurfaceTests/GestureTrailTests.h"
// In main():
totalFailed += gesture_trail_tests::runAll();
```

Add to `CMakeLists.txt` XOceanusTests sources:

```cmake
Tests/PlaySurfaceTests/GestureTrailTests.cpp
```

- [ ] **Step 4: Build — verify tests fail**

```bash
cmake --build build 2>&1 | tail -10
```

Expected: Fails because `GestureTrailBuffer.h` doesn't exist.

- [ ] **Step 5: Implement GestureTrailBuffer.h**

Create `Source/UI/PlaySurface/GestureTrailBuffer.h`:

```cpp
#pragma once

#include <array>
#include <cmath>
#include <algorithm>
#include <tuple>
#include <cstring>

namespace xoceanus {

// B043: Gesture Trail as First-Class Modulation Source
// 256-tuple ring buffer with freeze/replay and two-trail interference.
// No JUCE dependency.
// Spec: Docs/specs/2026-03-26-playsurface-v2-xouija-design.md Section 5

struct TrailPoint
{
    float x         = 0.0f;  // normalized 0.0–1.0 within XOuija surface
    float y         = 0.0f;  // normalized 0.0–1.0
    float velocity  = 0.0f;  // derived from distance/time, clamped 0.0–1.0
    double timestamp = 0.0;  // seconds since plugin load (monotonic clock)
};

class GestureTrailBuffer
{
public:
    static constexpr int kBufferSize = 256;
    static constexpr int kMask = kBufferSize - 1; // power-of-two masking

    GestureTrailBuffer() = default;

    void push(float x, float y, float velocity, double timestamp)
    {
        buffer_[head_] = { x, y, std::clamp(velocity, 0.0f, 1.0f), timestamp };
        head_ = (head_ + 1) & kMask;
        if (count_ < kBufferSize) ++count_;
    }

    int count() const { return count_; }

    // Oldest point in the buffer
    TrailPoint oldest() const
    {
        if (count_ == 0) return {};
        int idx = (head_ - count_ + kBufferSize) & kMask;
        return buffer_[idx];
    }

    // Newest point in the buffer
    TrailPoint newest() const
    {
        if (count_ == 0) return {};
        return buffer_[(head_ - 1 + kBufferSize) & kMask];
    }

    // Access point by age (0 = newest, count-1 = oldest)
    TrailPoint pointByAge(int age) const
    {
        if (age < 0 || age >= count_) return {};
        return buffer_[(head_ - 1 - age + kBufferSize) & kMask];
    }

    // ── Freeze / Replay (B043 Section 5.2) ──

    void freeze()
    {
        std::memcpy(frozen_, buffer_, sizeof(buffer_));
        frozenHead_ = head_;
        frozenCount_ = count_;
        frozen_ = true;  // Oops, name collision. Let me fix.
    }

    // Actually, let me use a separate flag:
    void clear()
    {
        head_ = 0;
        count_ = 0;
        isFrozen_ = false;
        frozenCount_ = 0;
    }

    bool isFrozen() const { return isFrozen_; }
    int frozenCount() const { return frozenCount_; }

    TrailPoint frozenPointAt(int index) const
    {
        if (!isFrozen_ || index < 0 || index >= frozenCount_) return {};
        int start = (frozenHead_ - frozenCount_ + kBufferSize) & kMask;
        return frozenBuffer_[(start + index) & kMask];
    }

    // Replay at normalized time (0.0–1.0 loops through frozen buffer)
    TrailPoint replayAt(float normalizedTime) const
    {
        if (!isFrozen_ || frozenCount_ == 0) return {};
        float t = normalizedTime - std::floor(normalizedTime); // wrap to 0..1
        int idx = static_cast<int>(t * frozenCount_) % frozenCount_;
        return frozenPointAt(idx);
    }

    void unfreeze()
    {
        isFrozen_ = false;
        frozenCount_ = 0;
    }

    // ── Two-Trail Interference (B043 Section 5.3) ──
    // X summed and clamped: x_out = clamp(x_A + x_B - 0.5, 0, 1)
    // Y multiplied: y_out = y_A * y_B

    static std::pair<float, float> interference(const TrailPoint& a, const TrailPoint& b)
    {
        float xOut = std::clamp(a.x + b.x - 0.5f, 0.0f, 1.0f);
        float yOut = a.y * b.y;
        return { xOut, yOut };
    }

private:
    // Live buffer
    std::array<TrailPoint, kBufferSize> buffer_ {};
    int head_ = 0;
    int count_ = 0;

    // Frozen snapshot
    std::array<TrailPoint, kBufferSize> frozenBuffer_ {};
    int frozenHead_ = 0;
    int frozenCount_ = 0;
    bool isFrozen_ = false;
};

} // namespace xoceanus
```

**IMPORTANT — Bug in the code above:** The `freeze()` method has a name collision (uses `frozen_` as both array and bool). The subagent must fix this: use `frozenBuffer_` for the array and `isFrozen_` for the bool. The corrected `freeze()`:

```cpp
void freeze()
{
    frozenBuffer_ = buffer_; // std::array supports copy assignment
    frozenHead_ = head_;
    frozenCount_ = count_;
    isFrozen_ = true;
}
```

- [ ] **Step 6: Build and run tests — verify all pass**

```bash
cmake --build build && ./build/XOceanusTests
```

Expected: All GestureTrail tests pass.

- [ ] **Step 7: Commit**

```bash
git add Source/UI/PlaySurface/GestureTrailBuffer.h \
        Tests/PlaySurfaceTests/GestureTrailTests.h \
        Tests/PlaySurfaceTests/GestureTrailTests.cpp \
        Tests/run_tests.cpp CMakeLists.txt
git commit -m "feat(PlaySurface): add GestureTrailBuffer — B043 ring buffer, freeze/replay, interference"
```

---

### Task 3: CC Output Infrastructure

**Files:**
- Modify: `Source/XOceanusProcessor.h`
- Modify: `Source/XOceanusProcessor.cpp`

The processor currently returns `false` from `producesMidi()` and has no CC output path. This task adds a lock-free SPSC (single-producer, single-consumer) ring queue for CC events and drains it at the end of `processBlock()`.

- [ ] **Step 1: Read the current processor files**

Read `Source/XOceanusProcessor.h` and `Source/XOceanusProcessor.cpp` to understand the current `processBlock` structure and where to add the drain call. Look for:
- The `producesMidi()` method (should be line ~106)
- The end of `processBlock()` in the .cpp file
- Existing `playSurfaceMidiCollector` usage pattern

- [ ] **Step 2: Add CCOutputQueue struct to XOceanusProcessor.h**

Add this inside the `XOceanusProcessor` class (private section), alongside the existing `NoteMapEvent` ring buffer:

```cpp
// ── CC Output Queue (XOuija → MIDI out) ──
// Lock-free SPSC: UI thread pushes, audio thread drains in processBlock.
struct CCOutputEvent
{
    uint8_t channel    = 0;   // 0-15
    uint8_t controller = 0;   // CC number (85-90 for XOuija)
    uint8_t value      = 0;   // 0-127
};

static constexpr size_t kCCQueueSize = 256;
std::array<CCOutputEvent, kCCQueueSize> ccOutputQueue_ {};
std::atomic<size_t> ccOutputHead_ { 0 };  // written by UI thread
std::atomic<size_t> ccOutputTail_ { 0 };  // read by audio thread

public:
// Called from UI thread (PlaySurface/XOuija components)
void pushCCOutput(uint8_t channel, uint8_t cc, uint8_t value) noexcept
{
    size_t head = ccOutputHead_.load(std::memory_order_relaxed);
    size_t next = (head + 1) % kCCQueueSize;
    if (next == ccOutputTail_.load(std::memory_order_acquire))
        return; // queue full — drop (acceptable for CC)
    ccOutputQueue_[head] = { channel, cc, value };
    ccOutputHead_.store(next, std::memory_order_release);
}

private:
// Called from audio thread at end of processBlock
void drainCCOutput(juce::MidiBuffer& midiOut, int numSamples) noexcept
{
    size_t tail = ccOutputTail_.load(std::memory_order_relaxed);
    size_t head = ccOutputHead_.load(std::memory_order_acquire);

    while (tail != head)
    {
        const auto& evt = ccOutputQueue_[tail];
        midiOut.addEvent(
            juce::MidiMessage::controllerEvent(evt.channel + 1, evt.controller, evt.value),
            numSamples - 1); // place at end of block
        tail = (tail + 1) % kCCQueueSize;
    }
    ccOutputTail_.store(tail, std::memory_order_release);
}
```

- [ ] **Step 3: Flip producesMidi() to true**

Change:
```cpp
bool producesMidi() const override { return false; }
```
To:
```cpp
bool producesMidi() const override { return true; }
```

- [ ] **Step 4: Drain CC queue at end of processBlock**

In `Source/XOceanusProcessor.cpp`, find the end of `processBlock()` (before the final `}`) and add:

```cpp
// Drain XOuija CC output into MIDI output buffer
drainCCOutput(midiMessages, buffer.getNumSamples());
```

- [ ] **Step 5: Build and verify**

```bash
cmake --build build
auval -v aumu Xolk XoOx 2>&1 | tail -5
```

Expected: Build succeeds. Auval passes (the CC queue is empty by default, so no behavioral change).

- [ ] **Step 6: Commit**

```bash
git add Source/XOceanusProcessor.h Source/XOceanusProcessor.cpp
git commit -m "feat(Processor): add CC output queue for XOuija — SPSC ring, producesMidi=true"
```

---

## Wave 2: XOuija Panel (JUCE Components)

### Task 4: XOuijaPanel Surface

**Files:**
- Create: `Source/UI/PlaySurface/XOuijaPanel.h`

This is the main XOuija container component. This task creates the panel surface with circle-of-fifths markers, tension coloring, noise texture, and mouse input handling. The Planchette and buttons are added in subsequent tasks.

- [ ] **Step 1: Create XOuijaPanel.h with class skeleton**

Create `Source/UI/PlaySurface/XOuijaPanel.h` with the following structure. The panel renders the background, the 12+1 circle-of-fifths markers with Georgia italic typography and arc layout, YES/NO axis labels, and handles mouse events to update circle position (X) and influence depth (Y).

Key specs to implement (from Sections 3.2–3.6):
- X-axis: left=Gb(-6), center=C(0), right=F#(+6). CC 85 range 0.0–1.0.
- Y-axis: bottom=NO(0.0), top=YES(1.0). CC 86 range 0.0–1.0.
- Markers: Georgia italic, size/opacity by `HarmonicField::markerProperties()`, color by `tensionColor()`, vertical arc by `markerArcY()`.
- YES/NO labels: Georgia italic 9px, opacity 0.20.
- Surface noise: static pre-generated image, opacity 0.05.
- Background: slightly lighter than PlaySurface bg to create visual division without separator lines.

```cpp
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "HarmonicField.h"
#include "GestureTrailBuffer.h"
#include <functional>

namespace xoceanus {

// Forward declarations for nested classes added in later tasks
class Planchette;
class GestureButtonBar;

class XOuijaPanel : public juce::Component
{
public:
    // ── Constants (Spec Section 2.2) ──
    static constexpr int kMinWidth  = 155;
    static constexpr int kMaxWidth  = 185;
    static constexpr int kPreferredWidth = 165;
    static constexpr float kAspectRatio = 9.0f / 19.5f; // width / height

    // ── Callbacks ──
    std::function<void(float circleX, float influenceY)> onPositionChanged;
    std::function<void()> onGoodbye; // All Notes Off

    XOuijaPanel()
    {
        generateNoiseTexture();
    }

    // ── Accessors ──
    float getCirclePosition() const { return circleX_; }
    float getInfluenceDepth() const { return influenceY_; }
    int   getCurrentKey() const { return HarmonicField::positionToKey(circleX_); }

    void setAccentColour(juce::Colour c) { accentColour_ = c; repaint(); }
    juce::Colour getAccentColour() const { return accentColour_; }

    void setCirclePosition(float x)
    {
        circleX_ = std::clamp(x, 0.0f, 1.0f);
        repaint();
    }

    void setInfluenceDepth(float y)
    {
        influenceY_ = std::clamp(y, 0.0f, 1.0f);
        repaint();
    }

    // ── Paint ──
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background — slightly lighter than parent surface
        g.setColour(juce::Colour(0xff1e1e22));
        g.fillRoundedRectangle(bounds, 2.0f);

        // Noise texture overlay
        if (noiseTexture_.isValid())
        {
            g.setOpacity(0.05f);
            g.drawImage(noiseTexture_, bounds);
            g.setOpacity(1.0f);
        }

        // Draw circle-of-fifths markers
        paintMarkers(g, bounds);

        // YES / NO labels (Spec Section 3.3)
        auto serifFont = juce::Font("Georgia", 9.0f, juce::Font::italic);
        g.setFont(serifFont);
        g.setColour(juce::Colours::white.withAlpha(0.20f));
        g.drawText("YES", bounds.removeFromTop(16.0f), juce::Justification::centred);
        g.drawText("NO",  bounds.removeFromBottom(80.0f).removeFromTop(16.0f),
                   juce::Justification::centred);
    }

    void paintMarkers(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Markers sit in a horizontal band roughly at 40% from top
        float markerY = bounds.getY() + bounds.getHeight() * 0.40f;
        float markerXStart = bounds.getX() + 8.0f;
        float markerXEnd   = bounds.getRight() - 8.0f;
        float markerWidth  = markerXEnd - markerXStart;

        int currentKey = HarmonicField::positionToKey(circleX_);

        auto serifFont = juce::Font("Georgia", 14.0f, juce::Font::italic);

        for (int i = 0; i <= 12; ++i)
        {
            float xFrac = static_cast<float>(i) / 12.0f;
            float px = markerXStart + xFrac * markerWidth;

            int semitone = HarmonicField::kFifthsSemitones[i];
            int dist = HarmonicField::fifthsDistance(currentKey, semitone);

            auto [sizeFactor, opacity] = HarmonicField::markerProperties(dist);
            auto [r, gc, b] = HarmonicField::tensionColor(dist);

            float arcY = HarmonicField::markerArcY(i, 8.0f);

            g.setFont(serifFont.withHeight(14.0f * sizeFactor));
            g.setColour(juce::Colour::fromFloatRGBA(r, gc, b, opacity));
            g.drawText(HarmonicField::kNoteNames[i],
                       juce::Rectangle<float>(px - 15.0f, markerY + arcY - 8.0f, 30.0f, 16.0f),
                       juce::Justification::centred);
        }
    }

    // ── Mouse Handling ──
    void mouseDown(const juce::MouseEvent& e) override { updateFromMouse(e); touching_ = true; }
    void mouseDrag(const juce::MouseEvent& e) override { updateFromMouse(e); }
    void mouseUp(const juce::MouseEvent&) override     { touching_ = false; }

    bool isTouching() const { return touching_; }

private:
    float circleX_    = 0.5f;  // center = C
    float influenceY_ = 0.0f;  // bottom = NO
    bool  touching_   = false;

    juce::Colour accentColour_ { 0xffE9C46A }; // XO Gold default
    juce::Image noiseTexture_;

    void updateFromMouse(const juce::MouseEvent& e)
    {
        auto bounds = getLocalBounds().toFloat();
        circleX_    = std::clamp((e.position.x - bounds.getX()) / bounds.getWidth(), 0.0f, 1.0f);
        influenceY_ = std::clamp(1.0f - (e.position.y - bounds.getY()) / bounds.getHeight(), 0.0f, 1.0f);

        if (onPositionChanged)
            onPositionChanged(circleX_, influenceY_);

        repaint();
    }

    void generateNoiseTexture()
    {
        // Pre-generate static noise at 128x128 and tile
        noiseTexture_ = juce::Image(juce::Image::ARGB, 128, 128, true);
        juce::Image::BitmapData data(noiseTexture_, juce::Image::BitmapData::writeOnly);
        juce::Random rng(42); // deterministic seed
        for (int y = 0; y < 128; ++y)
            for (int x = 0; x < 128; ++x)
            {
                uint8_t v = static_cast<uint8_t>(rng.nextInt(256));
                data.setPixelColour(x, y, juce::Colour(v, v, v, static_cast<uint8_t>(255)));
            }
    }
};

} // namespace xoceanus
```

- [ ] **Step 2: Include XOuijaPanel in PlaySurface.h to verify it compiles**

In `Source/UI/PlaySurface/PlaySurface.h`, add near the top includes:

```cpp
#include "XOuijaPanel.h"
```

- [ ] **Step 3: Build**

```bash
cmake --build build 2>&1 | tail -10
```

Expected: Build succeeds. No behavioral change yet — XOuijaPanel is included but not wired into the layout.

- [ ] **Step 4: Commit**

```bash
git add Source/UI/PlaySurface/XOuijaPanel.h Source/UI/PlaySurface/PlaySurface.h
git commit -m "feat(PlaySurface): add XOuijaPanel surface — markers, tension coloring, noise texture"
```

---

### Task 5: Planchette

**Files:**
- Modify: `Source/UI/PlaySurface/XOuijaPanel.h`

Add the Planchette as a child component of XOuijaPanel. The Planchette (B042) has: translucent oval lens (68×46), Lissajous idle drift, spring lock-on (150ms), warm memory hold (400ms), bioluminescent trail, and interior text showing key + influence%.

- [ ] **Step 1: Add Planchette class inside XOuijaPanel.h**

Add this class definition before the `XOuijaPanel` class:

```cpp
// B042: The Planchette as Autonomous Entity
// Spec: Section 4 — Lissajous drift, spring lock-on, warm hold, trail
class Planchette : public juce::Component, private juce::Timer
{
public:
    static constexpr float kWidth  = 68.0f;
    static constexpr float kHeight = 46.0f;
    static constexpr float kPipSize = 6.0f;

    // ── Lissajous Drift (Section 4.2) ──
    static constexpr float kDriftFreqX = 0.3f;  // Hz
    static constexpr float kDriftFreqY = 0.2f;  // Hz
    static constexpr float kDriftAmplitude = 0.15f; // fraction of parent size
    static constexpr float kDriftPhaseOffset = juce::MathConstants<float>::pi / 4.0f;

    // ── Timing ──
    static constexpr float kSpringDurationMs = 150.0f;
    static constexpr float kWarmHoldMs = 400.0f;

    Planchette()
    {
        setSize(static_cast<int>(kWidth), static_cast<int>(kHeight));
        setInterceptsMouseClicks(false, false); // parent handles mouse
        startTimerHz(60); // 60fps animation
    }

    void setAccentColour(juce::Colour c) { accent_ = c; repaint(); }

    void setDisplayText(const juce::String& text) { displayText_ = text; repaint(); }

    // Called by parent when touch begins — spring to target
    void springTo(float normX, float normY)
    {
        targetX_ = normX;
        targetY_ = normY;
        springStartX_ = displayX_;
        springStartY_ = displayY_;
        springElapsed_ = 0.0f;
        springing_ = true;
        touching_ = true;
        drifting_ = false;
        warmHoldElapsed_ = 0.0f;
    }

    // Called by parent when touch moves
    void moveTo(float normX, float normY)
    {
        targetX_ = normX;
        targetY_ = normY;
        // Snap immediately during active touch (spring only on initial press)
        if (!springing_)
        {
            displayX_ = normX;
            displayY_ = normY;
        }
        touching_ = true;
    }

    // Called by parent when touch ends — start warm hold
    void release()
    {
        touching_ = false;
        warmHoldElapsed_ = 0.0f;
        driftCenterX_ = displayX_;
        driftCenterY_ = displayY_;
    }

    // Toggle autonomous drift on/off (DRIFT button)
    void setDriftEnabled(bool enabled) { driftEnabled_ = enabled; }
    bool isDriftEnabled() const { return driftEnabled_; }

    // Snap to home position (HOME button)
    void snapHome()
    {
        springTo(0.5f, 0.5f);
    }

    float getDisplayX() const { return displayX_; }
    float getDisplayY() const { return displayY_; }
    bool  isTouching() const { return touching_; }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        // Translucent oval lens
        g.setColour(accent_.withAlpha(0.15f));
        g.fillEllipse(b);

        // Inner glow
        g.setColour(accent_.withAlpha(0.40f));
        juce::Path glowPath;
        glowPath.addEllipse(b.reduced(4.0f));
        g.strokePath(glowPath, juce::PathStrokeType(1.5f));

        // Border
        g.setColour(accent_);
        g.drawEllipse(b.reduced(0.75f), 1.5f);

        // Center pip
        float cx = b.getCentreX(), cy = b.getCentreY();
        g.setColour(accent_);
        g.fillEllipse(cx - kPipSize / 2, cy - kPipSize / 2, kPipSize, kPipSize);

        // Interior text
        auto serifFont = juce::Font("Georgia", 10.0f, juce::Font::italic);
        g.setFont(serifFont);
        g.setColour(accent_.withAlpha(0.85f));
        g.drawText(displayText_, b.translated(0, 8.0f), juce::Justification::centred);
    }

private:
    juce::Colour accent_ { 0xffE9C46A };
    juce::String displayText_ { "C \xC2\xB7 0%" }; // "C · 0%"

    // Position (normalized 0–1 in parent coords)
    float displayX_ = 0.5f, displayY_ = 0.0f;
    float targetX_ = 0.5f, targetY_ = 0.0f;

    // Spring animation
    bool springing_ = false;
    float springStartX_ = 0.5f, springStartY_ = 0.0f;
    float springElapsed_ = 0.0f;

    // State
    bool touching_ = false;
    bool drifting_ = false;
    bool driftEnabled_ = true;
    float warmHoldElapsed_ = 0.0f;
    float driftCenterX_ = 0.5f, driftCenterY_ = 0.0f;
    double driftTime_ = 0.0;

    // Cubic-bezier ease-out approximation: t^(1/2) gives reasonable ease-out feel
    static float easeOut(float t) { return 1.0f - std::pow(1.0f - std::clamp(t, 0.0f, 1.0f), 2.0f); }

    void timerCallback() override
    {
        constexpr float dt = 1.0f / 60.0f; // 60fps
        constexpr float dtMs = dt * 1000.0f;

        if (springing_)
        {
            springElapsed_ += dtMs;
            float t = easeOut(springElapsed_ / kSpringDurationMs);
            displayX_ = springStartX_ + t * (targetX_ - springStartX_);
            displayY_ = springStartY_ + t * (targetY_ - springStartY_);
            if (springElapsed_ >= kSpringDurationMs)
            {
                displayX_ = targetX_;
                displayY_ = targetY_;
                springing_ = false;
            }
        }
        else if (touching_)
        {
            displayX_ = targetX_;
            displayY_ = targetY_;
        }
        else if (warmHoldElapsed_ < kWarmHoldMs)
        {
            // Warm memory hold — stay put
            warmHoldElapsed_ += dtMs;
        }
        else if (driftEnabled_)
        {
            // Lissajous idle drift (Section 4.2)
            driftTime_ += dt;
            float parentW = getParentWidth() > 0 ? static_cast<float>(getParentWidth()) : 165.0f;
            float parentH = getParentHeight() > 0 ? static_cast<float>(getParentHeight()) : 420.0f;
            float ampX = kDriftAmplitude * (kWidth / parentW);  // normalize to parent
            float ampY = kDriftAmplitude * (kHeight / parentH);

            displayX_ = driftCenterX_ + ampX * std::sin(2.0f * juce::MathConstants<float>::pi * kDriftFreqX * static_cast<float>(driftTime_));
            displayY_ = driftCenterY_ + ampY * std::sin(2.0f * juce::MathConstants<float>::pi * kDriftFreqY * static_cast<float>(driftTime_) + kDriftPhaseOffset);

            displayX_ = std::clamp(displayX_, 0.0f, 1.0f);
            displayY_ = std::clamp(displayY_, 0.0f, 1.0f);
        }

        // Update position within parent
        if (auto* parent = getParentComponent())
        {
            int px = static_cast<int>(displayX_ * parent->getWidth() - kWidth / 2);
            int py = static_cast<int>((1.0f - displayY_) * parent->getHeight() - kHeight / 2);
            setBounds(px, py, static_cast<int>(kWidth), static_cast<int>(kHeight));
        }

        repaint();
    }
};
```

- [ ] **Step 2: Add Planchette as child of XOuijaPanel**

In the `XOuijaPanel` class, add:

```cpp
// In the public section, after constructor:
Planchette planchette_;

// In constructor body, add:
addAndMakeVisible(planchette_);

// Override mouseDown/mouseDrag/mouseUp to drive planchette:
void mouseDown(const juce::MouseEvent& e) override
{
    touching_ = true;
    auto [normX, normY] = mouseToNormalized(e);
    circleX_ = normX;
    influenceY_ = normY;
    planchette_.springTo(normX, normY);
    updateDisplayText();
    if (onPositionChanged) onPositionChanged(circleX_, influenceY_);
    repaint();
}

void mouseDrag(const juce::MouseEvent& e) override
{
    auto [normX, normY] = mouseToNormalized(e);
    circleX_ = normX;
    influenceY_ = normY;
    planchette_.moveTo(normX, normY);
    updateDisplayText();
    if (onPositionChanged) onPositionChanged(circleX_, influenceY_);
    repaint();
}

void mouseUp(const juce::MouseEvent&) override
{
    touching_ = false;
    planchette_.release();
}

// Helper:
std::pair<float, float> mouseToNormalized(const juce::MouseEvent& e) const
{
    auto bounds = getLocalBounds().toFloat();
    float nx = std::clamp((e.position.x - bounds.getX()) / bounds.getWidth(), 0.0f, 1.0f);
    float ny = std::clamp(1.0f - (e.position.y - bounds.getY()) / bounds.getHeight(), 0.0f, 1.0f);
    return { nx, ny };
}

void updateDisplayText()
{
    int key = HarmonicField::positionToKey(circleX_);
    // Find the note name from the semitone
    constexpr const char* kNotes[12] = { "C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B" };
    int pct = static_cast<int>(influenceY_ * 100.0f);
    planchette_.setDisplayText(juce::String(kNotes[key]) + " \xC2\xB7 " + juce::String(pct) + "%");
}
```

- [ ] **Step 3: Propagate accent colour**

In `XOuijaPanel::setAccentColour()`, also propagate:

```cpp
void setAccentColour(juce::Colour c)
{
    accentColour_ = c;
    planchette_.setAccentColour(c);
    repaint();
}
```

- [ ] **Step 4: Build**

```bash
cmake --build build 2>&1 | tail -10
```

Expected: Compiles. The planchette is not visible yet (not wired into layout), but the code is valid.

- [ ] **Step 5: Commit**

```bash
git add Source/UI/PlaySurface/XOuijaPanel.h
git commit -m "feat(PlaySurface): add Planchette — B042 autonomous entity with Lissajous drift, spring, warm hold"
```

---

### Task 6: Trail Rendering + CC Emission

**Files:**
- Modify: `Source/UI/PlaySurface/XOuijaPanel.h`

Wire the GestureTrailBuffer into XOuijaPanel: record trail points during touch, render bioluminescent trail, emit CC 85/86 on position change, and emit CC 88 on freeze.

- [ ] **Step 1: Add trail buffer and CC emission to XOuijaPanel**

In `XOuijaPanel`, add members:

```cpp
GestureTrailBuffer trailBuffer_;
std::function<void(uint8_t cc, uint8_t value)> onCCOutput; // caller wires to processor.pushCCOutput
```

- [ ] **Step 2: Record trail points during mouse drag**

In `mouseDrag()`, after updating circleX_/influenceY_, add:

```cpp
// Record trail point (distance-spaced, ~4px apart)
float dx = e.position.x - lastTrailPixelX_;
float dy = e.position.y - lastTrailPixelY_;
if (dx * dx + dy * dy >= 16.0f) // 4px distance squared
{
    double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    float vel = std::clamp(std::sqrt(dx * dx + dy * dy) / 50.0f, 0.0f, 1.0f);
    trailBuffer_.push(circleX_, influenceY_, vel, now);
    lastTrailPixelX_ = e.position.x;
    lastTrailPixelY_ = e.position.y;
}
```

Add `float lastTrailPixelX_ = 0, lastTrailPixelY_ = 0;` to private members.

- [ ] **Step 3: Emit CC 85/86 on position change**

In both `mouseDown` and `mouseDrag`, after updating position:

```cpp
if (onCCOutput)
{
    onCCOutput(85, static_cast<uint8_t>(circleX_ * 127.0f));
    onCCOutput(86, static_cast<uint8_t>(influenceY_ * 127.0f));
}
```

- [ ] **Step 4: Paint bioluminescent trail**

Add a `paintTrail()` method called from `paint()` after background, before planchette:

```cpp
void paintTrail(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    int n = trailBuffer_.count();
    for (int i = 0; i < n && i < 12; ++i)
    {
        auto pt = trailBuffer_.pointByAge(i);
        float age = static_cast<float>(now - pt.timestamp);
        if (age > 1.5f) continue; // expired

        // Exponential decay
        float alpha = std::exp(-age * 2.0f); // ~1.5s to near-zero
        float radius = 5.0f * alpha + 1.0f;  // 5px → 1px

        float px = bounds.getX() + pt.x * bounds.getWidth();
        float py = bounds.getY() + (1.0f - pt.y) * bounds.getHeight();

        // Glow
        g.setColour(accentColour_.withAlpha(alpha * 0.5f));
        g.fillEllipse(px - radius * 1.5f, py - radius * 1.5f, radius * 3.0f, radius * 3.0f);

        // Core
        g.setColour(accentColour_.withAlpha(alpha));
        g.fillEllipse(px - radius, py - radius, radius * 2.0f, radius * 2.0f);
    }

    // Frozen trail replay at 50% opacity
    if (trailBuffer_.isFrozen())
    {
        // Animate frozen replay... (simplified: draw all frozen points at 50% alpha)
        for (int i = 0; i < trailBuffer_.frozenCount() && i < 256; ++i)
        {
            auto pt = trailBuffer_.frozenPointAt(i);
            float px = bounds.getX() + pt.x * bounds.getWidth();
            float py = bounds.getY() + (1.0f - pt.y) * bounds.getHeight();
            g.setColour(accentColour_.withAlpha(0.25f));
            g.fillEllipse(px - 2, py - 2, 4, 4);
        }
    }
}
```

- [ ] **Step 5: Build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 6: Commit**

```bash
git add Source/UI/PlaySurface/XOuijaPanel.h
git commit -m "feat(PlaySurface): add bioluminescent trail rendering + CC 85/86 emission"
```

---

### Task 7: GestureButtonBar + GOODBYE

**Files:**
- Modify: `Source/UI/PlaySurface/XOuijaPanel.h`

Add the configurable gesture button bar (FREEZE/HOME/DRIFT + 3 other banks) and the GOODBYE button at the bottom of XOuijaPanel.

- [ ] **Step 1: Add GestureButtonBar class**

Add before `XOuijaPanel`:

```cpp
// Performance Gesture Buttons (Spec Section 6)
// 3 configurable slots that auto-follow PerformanceStrip mode
class GestureButtonBar : public juce::Component
{
public:
    enum class Bank { XOuija, Dub, Coupling, Performance };

    struct ButtonDef
    {
        juce::String label;
        char shortcutKey;
        std::function<void()> action;
    };

    GestureButtonBar()
    {
        setBank(Bank::XOuija);
    }

    void setBank(Bank bank)
    {
        if (bankLocked_ && currentBank_ != Bank::XOuija) return;
        currentBank_ = bank;
        repaint();
    }

    Bank getBank() const { return currentBank_; }
    void setBankLocked(bool locked) { bankLocked_ = locked; }
    bool isBankLocked() const { return bankLocked_; }

    // Owner sets these based on current bank
    std::array<ButtonDef, 3> buttons_;
    juce::Colour accent_ { 0xffE9C46A };

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        float btnW = (bounds.getWidth() - 8.0f) / 3.0f; // 4px gutters
        float btnH = 28.0f;
        float y = (bounds.getHeight() - btnH) / 2.0f;

        for (int i = 0; i < 3; ++i)
        {
            float x = 2.0f + i * (btnW + 2.0f);
            auto rect = juce::Rectangle<float>(x, y, btnW, btnH);

            bool active = (pressedIndex_ == i);

            // Background
            g.setColour(active ? accent_.withAlpha(0.30f)
                               : juce::Colour(255, 255, 255).withAlpha(0.06f));
            g.fillRoundedRectangle(rect, 3.0f);

            // Border
            g.setColour(juce::Colour(255, 255, 255).withAlpha(0.12f));
            g.drawRoundedRectangle(rect, 3.0f, 1.0f);

            // Label
            g.setFont(juce::Font(9.0f));
            g.setColour(juce::Colours::white.withAlpha(active ? 1.0f : 0.65f));
            g.drawText(buttons_[i].label, rect, juce::Justification::centred);
        }

        // Lock icon (top-right)
        float lockX = bounds.getRight() - 16.0f;
        g.setColour(juce::Colours::white.withAlpha(bankLocked_ ? 0.60f : 0.25f));
        g.setFont(juce::Font(11.0f));
        g.drawText(bankLocked_ ? "\xF0\x9F\x94\x92" : "\xF0\x9F\x94\x93",
                   juce::Rectangle<float>(lockX, 1.0f, 14.0f, 14.0f),
                   juce::Justification::centred);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int idx = hitTestButton(e.position);
        if (idx >= 0 && idx < 3)
        {
            pressedIndex_ = idx;
            if (buttons_[idx].action) buttons_[idx].action();
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent&) override { pressedIndex_ = -1; repaint(); }

    bool handleKey(char key)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (buttons_[i].shortcutKey == key)
            {
                pressedIndex_ = i;
                if (buttons_[i].action) buttons_[i].action();
                repaint();
                return true;
            }
        }
        return false;
    }

private:
    Bank currentBank_ = Bank::XOuija;
    bool bankLocked_ = false;
    int pressedIndex_ = -1;

    int hitTestButton(juce::Point<float> pos) const
    {
        float btnW = (static_cast<float>(getWidth()) - 8.0f) / 3.0f;
        for (int i = 0; i < 3; ++i)
        {
            float x = 2.0f + i * (btnW + 2.0f);
            if (pos.x >= x && pos.x < x + btnW) return i;
        }
        return -1;
    }
};

// GOODBYE button (Spec Section 7)
class GoodbyeButton : public juce::Component
{
public:
    static constexpr int kHeight = 32;
    std::function<void()> onGoodbye;

    void setAccentColour(juce::Colour) { /* GOODBYE is always terracotta */ }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        bool pressed = isMouseButtonDown();

        // Terracotta background (Spec: #E07A5F at 80% normal, 100% on press)
        g.setColour(juce::Colour(0xffE07A5F).withAlpha(pressed ? 1.0f : 0.80f));
        g.fillRoundedRectangle(b, 4.0f);

        // Georgia italic "GOODBYE" (Spec: 11px, white 90%)
        g.setFont(juce::Font("Georgia", 11.0f, juce::Font::italic));
        g.setColour(juce::Colours::white.withAlpha(0.90f));
        g.drawText("GOODBYE", b, juce::Justification::centred);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        if (onGoodbye) onGoodbye();
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override { repaint(); }
};
```

- [ ] **Step 2: Add button bar and GOODBYE as children of XOuijaPanel**

In XOuijaPanel constructor, add:

```cpp
addAndMakeVisible(gestureButtons_);
addAndMakeVisible(goodbyeButton_);

// Wire GOODBYE action (Spec Section 7.3)
goodbyeButton_.onGoodbye = [this]()
{
    // 1. Reset position
    circleX_ = 0.5f;
    influenceY_ = 0.0f;
    // 2. Animate planchette to bottom-center (GOODBYE position)
    planchette_.springTo(0.5f, 0.0f);
    // 3. Clear trail
    trailBuffer_.clear();
    // 4. Emit CC reset
    if (onCCOutput)
    {
        onCCOutput(85, 64);  // C = 0.5 → 64
        onCCOutput(86, 0);   // NO = 0.0 → 0
        onCCOutput(88, 0);   // unfreeze
    }
    // 5. Fire callback for All Notes Off
    if (onGoodbye) onGoodbye();
    updateDisplayText();
    repaint();
};

// Wire default XOuija button bank
setupDefaultButtonBank();
```

Add `GestureButtonBar gestureButtons_;` and `GoodbyeButton goodbyeButton_;` as members.

- [ ] **Step 3: Add setupDefaultButtonBank()**

```cpp
void setupDefaultButtonBank()
{
    gestureButtons_.buttons_[0] = { "FREEZE", 'Z', [this]() {
        if (trailBuffer_.isFrozen()) {
            trailBuffer_.unfreeze();
            if (onCCOutput) onCCOutput(88, 0);
        } else {
            trailBuffer_.freeze();
            if (onCCOutput) onCCOutput(88, 127);
        }
        repaint();
    }};
    gestureButtons_.buttons_[1] = { "HOME", 'X', [this]() {
        planchette_.snapHome();
        circleX_ = 0.5f;
        influenceY_ = 0.5f;
        if (onPositionChanged) onPositionChanged(circleX_, influenceY_);
        if (onCCOutput) { onCCOutput(85, 64); onCCOutput(86, 64); onCCOutput(89, 127); }
        updateDisplayText();
        repaint();
    }};
    gestureButtons_.buttons_[2] = { "DRIFT", 'C', [this]() {
        planchette_.setDriftEnabled(!planchette_.isDriftEnabled());
        if (onCCOutput) onCCOutput(90, planchette_.isDriftEnabled() ? 127 : 0);
        repaint();
    }};
}
```

- [ ] **Step 4: Add resized() for button/goodbye layout**

```cpp
void resized() override
{
    auto bounds = getLocalBounds();
    // GOODBYE at bottom (32px)
    goodbyeButton_.setBounds(bounds.removeFromBottom(GoodbyeButton::kHeight));
    // Button bar above GOODBYE (34px = 28px buttons + 6px padding)
    gestureButtons_.setBounds(bounds.removeFromBottom(34));
    // Remaining area is the harmonic surface (planchette moves within this)
}
```

- [ ] **Step 5: Handle keyboard shortcuts**

In `XOuijaPanel`, add:

```cpp
bool keyPressed(const juce::KeyPress& key) override
{
    char c = static_cast<char>(std::toupper(key.getTextCharacter()));
    if (c == 'V') { goodbyeButton_.onGoodbye(); return true; }
    return gestureButtons_.handleKey(c);
}
```

- [ ] **Step 6: Build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 7: Commit**

```bash
git add Source/UI/PlaySurface/XOuijaPanel.h
git commit -m "feat(PlaySurface): add GestureButtonBar + GOODBYE — configurable banks, keyboard shortcuts"
```

---

## Wave 3: Note Input Enhancements

### Task 8: KeysMode

**Files:**
- Create: `Source/UI/PlaySurface/KeysMode.h`
- Modify: `Source/UI/PlaySurface/PlaySurface.h` (add include, add to NoteInputZone::Mode enum)

New Seaboard-style 2-octave keyboard with scroll, Y-velocity, X-pitch-glide, and XOuija-reactive coloring (Spec Section 8.4).

- [ ] **Step 1: Create KeysMode.h**

Create `Source/UI/PlaySurface/KeysMode.h`:

```cpp
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "HarmonicField.h"
#include <functional>

namespace xoceanus {

// KEYS Mode — Seaboard-style 2-octave scrollable keyboard
// Spec: Section 8.4

class KeysMode : public juce::Component
{
public:
    // ── Constants ──
    static constexpr int kVisibleOctaves = 2;
    static constexpr int kMinOctave = 1;  // C1
    static constexpr int kMaxOctave = 7;  // C7
    static constexpr int kKeysPerOctave = 12;
    static constexpr int kVisibleKeys = kVisibleOctaves * kKeysPerOctave;

    // Colors (Spec Section 8.4)
    static constexpr uint32_t kNaturalColor = 0xffF0EDE8;  // Warm white
    static constexpr uint32_t kSharpColor   = 0xff2A2A2A;  // Charcoal

    // ── Callbacks ──
    std::function<void(int midiNote, float velocity)> onNoteOn;
    std::function<void(int midiNote)> onNoteOff;
    std::function<void(float pitchBend)> onPitchBend; // -1..+1
    juce::MidiMessageCollector* midiCollector = nullptr;

    KeysMode() {}

    void setRootKey(int key) { rootKey_ = key % 12; repaint(); }
    void setAccentColour(juce::Colour c) { accent_ = c; repaint(); }

    // Octave navigation
    int getBaseOctave() const { return baseOctave_; }
    void setBaseOctave(int oct)
    {
        baseOctave_ = std::clamp(oct, kMinOctave, kMaxOctave - kVisibleOctaves);
        repaint();
    }
    void octaveUp()   { setBaseOctave(baseOctave_ + 1); }
    void octaveDown() { setBaseOctave(baseOctave_ - 1); }

    juce::String getVisibleRange() const
    {
        constexpr const char* kNames[12] = {"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
        int lo = baseOctave_;
        int hi = baseOctave_ + kVisibleOctaves;
        return juce::String(kNames[0]) + juce::String(lo) + "-"
             + juce::String(kNames[11]) + juce::String(hi - 1);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Calculate key dimensions
        // 2 octaves = 14 natural keys (7 per octave)
        int numNaturals = 7 * kVisibleOctaves;
        float naturalW = bounds.getWidth() / static_cast<float>(numNaturals);
        float naturalH = bounds.getHeight();
        float sharpW = naturalW * 0.6f;
        float sharpH = naturalH * 0.6f;

        int baseMidi = (baseOctave_ + 1) * 12; // C1 = MIDI 24, C2 = 36, etc.

        // Draw natural keys first
        int naturalIdx = 0;
        for (int i = 0; i < kVisibleKeys; ++i)
        {
            int midi = baseMidi + i;
            int pc = midi % 12;
            if (isSharp(pc)) continue;

            float x = naturalIdx * naturalW;
            auto keyRect = juce::Rectangle<float>(x, 0, naturalW - 1.0f, naturalH);

            bool inKey = HarmonicField::isInKey(midi, rootKey_);
            bool isRoot = HarmonicField::isRoot(midi, rootKey_);
            bool active = (midi == activeNote_);

            // Key fill
            float opacity = inKey ? 1.0f : 0.20f;
            g.setColour(juce::Colour(kNaturalColor).withAlpha(opacity));
            g.fillRoundedRectangle(keyRect.withTrimmedBottom(-2), 2.0f);

            // In-key glow
            if (inKey)
            {
                g.setColour(accent_.withAlpha(active ? 0.30f : 0.12f));
                g.fillRoundedRectangle(keyRect, 2.0f);
            }

            // Root gold border (Spec: 5px on natural keys)
            if (isRoot)
            {
                g.setColour(juce::Colour(0xffE9C46A));
                g.fillRect(keyRect.removeFromBottom(5.0f));
            }

            naturalIdx++;
        }

        // Draw sharp keys on top
        naturalIdx = 0;
        for (int i = 0; i < kVisibleKeys; ++i)
        {
            int midi = baseMidi + i;
            int pc = midi % 12;

            if (!isSharp(pc))
            {
                naturalIdx++;
                continue;
            }

            // Sharp key sits between the previous and next natural key
            float x = (naturalIdx - 1) * naturalW + naturalW * 0.65f;
            auto keyRect = juce::Rectangle<float>(x, 0, sharpW, sharpH);

            bool inKey = HarmonicField::isInKey(midi, rootKey_);
            bool isRoot = HarmonicField::isRoot(midi, rootKey_);
            bool active = (midi == activeNote_);

            float opacity = inKey ? 1.0f : 0.20f;
            g.setColour(juce::Colour(kSharpColor).withAlpha(opacity));
            g.fillRoundedRectangle(keyRect, 2.0f);

            if (inKey)
            {
                g.setColour(accent_.withAlpha(active ? 0.30f : 0.10f));
                g.fillRoundedRectangle(keyRect, 2.0f);
            }

            if (isRoot)
            {
                g.setColour(juce::Colour(0xffE9C46A));
                g.fillRect(keyRect.removeFromBottom(3.0f));
            }
        }
    }

    // ── Mouse → Note + Expression ──
    void mouseDown(const juce::MouseEvent& e) override
    {
        int midi = pixelToMidi(e.position);
        if (midi < 0) return;

        // Y-velocity: top = 127, bottom = 1
        float vy = 1.0f - (e.position.y / static_cast<float>(getHeight()));
        int vel = std::clamp(static_cast<int>(vy * 126.0f + 1.0f), 1, 127);

        activeNote_ = midi;
        pressOriginX_ = e.position.x;

        if (midiCollector)
            midiCollector->addMessageToQueue(juce::MidiMessage::noteOn(1, midi, static_cast<juce::uint8>(vel)),
                                             juce::Time::getMillisecondCounterHiRes() / 1000.0);
        if (onNoteOn) onNoteOn(midi, vel / 127.0f);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (activeNote_ < 0) return;

        // X-drag = pitch glide (Spec: smooth pitch bend proportional to X distance)
        float dx = e.position.x - pressOriginX_;
        float bendRange = static_cast<float>(getWidth()) / 4.0f; // full bend = 1/4 width
        float bend = std::clamp(dx / bendRange, -1.0f, 1.0f);

        if (midiCollector)
        {
            int bendVal = static_cast<int>((bend + 1.0f) * 8191.5f); // 0–16383, center=8192
            midiCollector->addMessageToQueue(juce::MidiMessage::pitchWheel(1, bendVal),
                                             juce::Time::getMillisecondCounterHiRes() / 1000.0);
        }
        if (onPitchBend) onPitchBend(bend);
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (activeNote_ >= 0)
        {
            if (midiCollector)
            {
                midiCollector->addMessageToQueue(juce::MidiMessage::noteOff(1, activeNote_),
                                                 juce::Time::getMillisecondCounterHiRes() / 1000.0);
                // Reset pitch bend
                midiCollector->addMessageToQueue(juce::MidiMessage::pitchWheel(1, 8192),
                                                 juce::Time::getMillisecondCounterHiRes() / 1000.0);
            }
            if (onNoteOff) onNoteOff(activeNote_);
        }
        activeNote_ = -1;
        repaint();
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        if (std::abs(wheel.deltaY) > 0.01f)
        {
            if (wheel.deltaY > 0) octaveUp();
            else octaveDown();
        }
    }

private:
    int rootKey_ = 0;       // C
    int baseOctave_ = 3;    // C3–B4 default visible range
    int activeNote_ = -1;
    float pressOriginX_ = 0;
    juce::Colour accent_ { 0xffE9C46A };

    static bool isSharp(int pitchClass)
    {
        return pitchClass == 1 || pitchClass == 3 || pitchClass == 6
            || pitchClass == 8 || pitchClass == 10;
    }

    int pixelToMidi(juce::Point<float> pos) const
    {
        int baseMidi = (baseOctave_ + 1) * 12;
        int numNaturals = 7 * kVisibleOctaves;
        float naturalW = static_cast<float>(getWidth()) / static_cast<float>(numNaturals);
        float sharpW = naturalW * 0.6f;
        float sharpH = static_cast<float>(getHeight()) * 0.6f;

        // Check sharps first (they're on top)
        if (pos.y < sharpH)
        {
            int naturalIdx = 0;
            for (int i = 0; i < kVisibleKeys; ++i)
            {
                int pc = (baseMidi + i) % 12;
                if (!isSharp(pc)) { naturalIdx++; continue; }

                float x = (naturalIdx - 1) * naturalW + naturalW * 0.65f;
                if (pos.x >= x && pos.x < x + sharpW)
                    return baseMidi + i;
            }
        }

        // Check naturals
        int naturalIdx = static_cast<int>(pos.x / naturalW);
        int count = 0;
        for (int i = 0; i < kVisibleKeys; ++i)
        {
            int pc = (baseMidi + i) % 12;
            if (isSharp(pc)) continue;
            if (count == naturalIdx) return baseMidi + i;
            count++;
        }

        return -1;
    }
};

} // namespace xoceanus
```

- [ ] **Step 2: Add Keys to NoteInputZone::Mode enum**

In `PlaySurface.h`, find the `NoteInputZone::Mode` enum and add `Keys`:

```cpp
enum class Mode { Pad, Fretless, Drum, Keys };
```

**Note:** The spec replaces Fretless with KEYS. Keep Fretless for backward compatibility but the mode tab should show PAD | DRUM | KEYS. The Fretless mode can be kept but hidden from the tab UI.

- [ ] **Step 3: Include KeysMode.h in PlaySurface.h**

```cpp
#include "KeysMode.h"
```

- [ ] **Step 4: Build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 5: Commit**

```bash
git add Source/UI/PlaySurface/KeysMode.h Source/UI/PlaySurface/PlaySurface.h
git commit -m "feat(PlaySurface): add KeysMode — Seaboard-style keyboard with pitch glide + XOuija coloring"
```

---

### Task 9: PAD Mode XOuija-Reactive Coloring

**Files:**
- Modify: `Source/UI/PlaySurface/PlaySurface.h`

Wire HarmonicField into the existing NoteInputZone pad painting to add XOuija-reactive coloring (Spec Section 8.2).

- [ ] **Step 1: Add harmonic field state to NoteInputZone**

Add members:

```cpp
int harmonicRootKey_ = 0;    // current key from XOuija circle position
int harmonicTension_ = 0;    // fifths distance from home (for color temperature)
```

Add setter:

```cpp
void setHarmonicField(int rootKey, int tension)
{
    harmonicRootKey_ = rootKey % 12;
    harmonicTension_ = std::clamp(tension, 0, 6);
    repaint();
}
```

- [ ] **Step 2: Modify pad painting to use harmonic coloring**

In the `paint()` method of `NoteInputZone`, find the pad rendering loop (where each of the 16 pads is drawn). After calculating the MIDI note for each pad, add this coloring logic:

```cpp
// XOuija-reactive coloring (Spec Section 8.2)
bool inKey = HarmonicField::isInKey(midiNote, harmonicRootKey_);
bool isRoot = HarmonicField::isRoot(midiNote, harmonicRootKey_);
auto [tr, tg, tb] = HarmonicField::tensionColor(harmonicTension_);
juce::Colour tensionColour = juce::Colour::fromFloatRGBA(tr, tg, tb, 1.0f);

if (mode_ == Mode::Pad)
{
    if (isRoot)
    {
        // XO Gold bottom border (4px)
        g.setColour(juce::Colour(0xffE9C46A));
        g.fillRect(padRect.removeFromBottom(4.0f));
    }

    float padAlpha = inKey ? 1.0f : 0.20f;
    // Blend accent with tension color for in-key pads
    if (inKey)
    {
        g.setColour(tensionColour.withAlpha(0.15f));
        g.fillRoundedRectangle(padRect, 4.0f);
    }
}
```

**Important:** This should be additive to existing pad painting, not replacing it. The subagent needs to read the current pad painting code and insert the harmonic coloring at the right point.

- [ ] **Step 3: Add HarmonicField include at top of PlaySurface.h**

If not already added in Task 4:

```cpp
#include "HarmonicField.h"
```

- [ ] **Step 4: Build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 5: Commit**

```bash
git add Source/UI/PlaySurface/PlaySurface.h
git commit -m "feat(PlaySurface): add XOuija-reactive pad coloring — in-key glow, root gold border, tension colors"
```

---

## Wave 4: Integration

### Task 10: PerformanceStrip V2

**Files:**
- Modify: `Source/UI/PlaySurface/PlaySurface.h`

Update the PerformanceStrip class to match spec: 64px height (was 80px), mode tabs inside the strip (was in header), crosshair gridlines, upgraded cursor with radial gradient + floor glow, and 250ms cubic-bezier spring-back (Spec Section 9).

- [ ] **Step 1: Fix strip height constant**

Find `kZone3H` in the PS namespace constants and change:

```cpp
// Old:
static constexpr int kZone3H = 80;
// New:
static constexpr int kZone3H = 64;
```

- [ ] **Step 2: Add crosshair gridlines to PerformanceStrip::paint()**

After painting the background tint, add:

```cpp
// Crosshair gridlines (Spec Section 9.3)
g.setColour(juce::Colours::white.withAlpha(0.10f));
float midY = bounds.getCentreY();
float midX = bounds.getCentreX();
g.drawHorizontalLine(static_cast<int>(midY), bounds.getX(), bounds.getRight());
g.drawVerticalLine(static_cast<int>(midX), bounds.getY(), bounds.getBottom());
```

- [ ] **Step 3: Upgrade cursor rendering**

Replace the current cursor drawing with:

```cpp
// Cursor: 10px dot with radial gradient + vertical bar + floor glow (Spec Section 9.3)
float cx = bounds.getX() + posX_ * bounds.getWidth();
float cy = bounds.getY() + (1.0f - posY_) * bounds.getHeight();

// Vertical bar from cursor to floor
g.setColour(accent_.withAlpha(0.30f));
g.drawVerticalLine(static_cast<int>(cx), cy, bounds.getBottom());

// Floor glow
g.setColour(accent_.withAlpha(0.30f));
g.fillEllipse(cx - 8.0f, bounds.getBottom() - 4.0f, 16.0f, 4.0f);

// Cursor dot with glow halo
g.setColour(accent_.withAlpha(0.60f));
g.fillEllipse(cx - 8.0f, cy - 8.0f, 16.0f, 16.0f); // 8px radius glow

g.setColour(accent_);
g.fillEllipse(cx - 5.0f, cy - 5.0f, 10.0f, 10.0f); // 5px radius core
```

- [ ] **Step 4: Add mode tabs inside the strip**

Add a tab rendering section at the left edge of the strip:

```cpp
// Mode tabs inside strip (Spec Section 9.2)
constexpr const char* kModeNames[] = { "DubSpace", "FilterSweep", "Coupling", "DubSiren" };
float tabW = 72.0f;
float tabH = 22.0f;
float tabY = (bounds.getHeight() - tabH) / 2.0f;

for (int i = 0; i < 4; ++i)
{
    auto tabRect = juce::Rectangle<float>(i * tabW + 4.0f, tabY, tabW - 4.0f, tabH);
    bool active = (i == static_cast<int>(mode_));

    if (active)
    {
        g.setColour(accent_.withAlpha(0.25f));
        g.fillRoundedRectangle(tabRect, 3.0f);
    }

    g.setFont(juce::Font(8.0f));
    g.setColour(juce::Colours::white.withAlpha(active ? 1.0f : 0.45f));
    g.drawText(kModeNames[i], tabRect, juce::Justification::centred);
}
```

- [ ] **Step 5: Update spring-back to 250ms ease-out**

Find the spring-back logic and update timing/easing:

```cpp
static constexpr float kSpringBackMs = 250.0f;

// In the timer callback, replace linear spring with ease-out:
float t = std::clamp(springElapsed_ / kSpringBackMs, 0.0f, 1.0f);
float eased = 1.0f - std::pow(1.0f - t, 2.0f); // quadratic ease-out
posX_ = springStartX_ + eased * (springTargetX_ - springStartX_);
posY_ = springStartY_ + eased * (springTargetY_ - springStartY_);
```

- [ ] **Step 6: Build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 7: Commit**

```bash
git add Source/UI/PlaySurface/PlaySurface.h
git commit -m "feat(PlaySurface): PerformanceStrip V2 — 64px, tabs inside, crosshair, cursor upgrade, 250ms spring"
```

---

### Task 11: PlaySurface V2 Layout

**Files:**
- Modify: `Source/UI/PlaySurface/PlaySurface.h`

This is the major restructure. Remove OrbitPathZone and PerformancePads from the layout. Wire XOuijaPanel on the left, NoteInputZone on the right, PerformanceStrip at the bottom. Add KeysMode to NoteInputZone. Update PlaySurfaceWindow dimensions.

- [ ] **Step 1: Read the current PlaySurface class layout carefully**

Read the `PlaySurface` class's `resized()` method and the constructor to understand how zones are currently laid out. Take note of:
- Current zone sizes (kZone1W, kZone2W, kZone3H, kZone4W, kMainZoneH)
- How the header bar allocates mode/bank/octave/scale controls
- The 4-zone grid: NoteInput(Z1) | PerfPads(Z2) bottom, OrbitPath(Z4) | ... top, Strip(Z3) full width bottom

- [ ] **Step 2: Update layout constants**

Replace the PS namespace layout constants:

```cpp
// Old V1 layout:
// static constexpr int kDesktopW = 1060;
// static constexpr int kDesktopH = 344;
// static constexpr int kZone1W  = 480;
// static constexpr int kZone2W  = 200;
// static constexpr int kZone4W  = 100;
// static constexpr int kMainZoneH = 240;

// New V2 layout (Spec Section 2.2):
static constexpr int kDesktopW = 700;       // Narrower — XOuija + NoteInput + gaps
static constexpr int kDesktopH = 484;       // 420 (main) + 64 (strip)
static constexpr int kXOuijaW = 165;        // XOuija panel width
static constexpr int kMainZoneH = 420;      // XOuija + NoteInput height
static constexpr int kStripH = 64;          // Performance strip
static constexpr int kHeaderH = 28;         // Mode tab bar (in NoteInput zone)
```

- [ ] **Step 3: Add XOuijaPanel and KeysMode as owned members**

In the `PlaySurface` class, add:

```cpp
XOuijaPanel xouijaPanel_;
KeysMode keysMode_;
```

In the constructor, add:

```cpp
addAndMakeVisible(xouijaPanel_);
addAndMakeVisible(keysMode_);

// Wire XOuija position changes to pad coloring
xouijaPanel_.onPositionChanged = [this](float circleX, float influenceY)
{
    int key = HarmonicField::positionToKey(circleX);
    int tension = HarmonicField::semitoneToFifthsFromC(key); // distance from C
    noteInputZone_.setHarmonicField(key, tension);
    keysMode_.setRootKey(key);
};

// Wire GOODBYE to All Notes Off
xouijaPanel_.onGoodbye = [this]()
{
    // Send All Notes Off on ch 1-16
    if (noteInputZone_.midiCollector)
    {
        double ts = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        for (int ch = 1; ch <= 16; ++ch)
            noteInputZone_.midiCollector->addMessageToQueue(
                juce::MidiMessage::controllerEvent(ch, 123, 0), ts);
    }
};

// Wire KeysMode MIDI collector
keysMode_.midiCollector = &playSurfaceMidiCollector_;
```

- [ ] **Step 4: Rewrite resized()**

Replace the `PlaySurface::resized()` method with the V2 layout:

```cpp
void resized() override
{
    auto bounds = getLocalBounds();

    // Performance Strip — full width, bottom
    performanceStrip_.setBounds(bounds.removeFromBottom(PS::kStripH));

    // XOuija Panel — left column, portrait proportions
    int ouijaW = std::clamp(PS::kXOuijaW, XOuijaPanel::kMinWidth, XOuijaPanel::kMaxWidth);
    xouijaPanel_.setBounds(bounds.removeFromLeft(ouijaW));

    // Note Input Zone — remaining space
    auto noteArea = bounds;

    // Mode tab bar at top of note area (PAD | DRUM | KEYS)
    // headerBar_.setBounds(noteArea.removeFromTop(PS::kHeaderH));

    // NoteInputZone gets the rest
    if (noteInputZone_.getMode() == NoteInputZone::Mode::Keys)
        keysMode_.setBounds(noteArea);
    else
        noteInputZone_.setBounds(noteArea);

    // Toggle visibility
    keysMode_.setVisible(noteInputZone_.getMode() == NoteInputZone::Mode::Keys);
    noteInputZone_.setVisible(noteInputZone_.getMode() != NoteInputZone::Mode::Keys);
}
```

- [ ] **Step 5: Remove OrbitPathZone and PerformancePads from the layout**

Comment out or remove:
- `orbitPathZone_` member and its `addAndMakeVisible()` call
- `performancePads_` member and its `addAndMakeVisible()` call
- Any references to these in `resized()`, `timerCallback()`, and `paint()`

**Do NOT delete the class definitions yet** — they may be referenced elsewhere. Just remove them from the PlaySurface composite. Mark with `// V1 DEPRECATED — remove after V2 stabilizes`.

- [ ] **Step 6: Propagate accent colour to new components**

In the accent colour setter (or wherever `setAccentColour` is called):

```cpp
xouijaPanel_.setAccentColour(colour);
keysMode_.setAccentColour(colour);
```

- [ ] **Step 7: Update PlaySurfaceWindow default size**

Find the `PlaySurfaceWindow` constructor and update:

```cpp
// Old: setSize(520, 520);
setSize(PS::kDesktopW, PS::kDesktopH);
// Old: setResizeLimits(320, 320, 1200, 1200);
setResizeLimits(500, 400, 1200, 900);
```

- [ ] **Step 8: Build**

```bash
cmake --build build 2>&1 | tail -10
```

Expected: Build passes. This is the biggest integration step — expect some issues to fix.

- [ ] **Step 9: Commit**

```bash
git add Source/UI/PlaySurface/PlaySurface.h
git commit -m "feat(PlaySurface): V2 layout — XOuija left, NoteInput right, Strip bottom, remove OrbitPath/PerfPads"
```

---

### Task 12: Full CC Wiring

**Files:**
- Modify: `Source/UI/PlaySurface/PlaySurface.h`
- Modify: `Source/UI/PlaySurface/XOuijaPanel.h`

Wire XOuija CC output (85-90) through to the processor's CC queue, and wire CC input (86/89/90) to move the planchette remotely.

- [ ] **Step 1: Wire CC output from XOuija to processor**

In `PlaySurface`, store a pointer to the processor and wire the callback:

```cpp
// Add member:
XOceanusProcessor* processor_ = nullptr;

// Add setter:
void setProcessor(XOceanusProcessor* p) { processor_ = p; }

// In constructor, after creating xouijaPanel_:
xouijaPanel_.onCCOutput = [this](uint8_t cc, uint8_t value)
{
    if (processor_)
        processor_->pushCCOutput(0, cc, value); // channel 0 = MIDI ch 1
};
```

- [ ] **Step 2: Add CC input handling**

Add a method to PlaySurface that the processor calls when incoming CC 86/89/90 is received:

```cpp
void handleIncomingCC(int cc, int value)
{
    if (cc == 86) // influence depth
    {
        xouijaPanel_.setInfluenceDepth(value / 127.0f);
    }
    else if (cc == 89 && value == 127) // home snap trigger
    {
        xouijaPanel_.planchette_.snapHome();
        xouijaPanel_.setCirclePosition(0.5f);
        xouijaPanel_.setInfluenceDepth(0.5f);
    }
    else if (cc == 90) // drift toggle
    {
        xouijaPanel_.planchette_.setDriftEnabled(value > 63);
    }
}
```

- [ ] **Step 3: Wire mod matrix sources**

In `XOuijaPanel`, expose the trail buffer's current values for mod matrix consumption:

```cpp
// Mod matrix source values (Spec Section 5.4)
float getModCircle() const    { return circleX_; }
float getModInfluence() const { return influenceY_; }
float getModVelocity() const
{
    if (trailBuffer_.count() == 0) return 0.0f;
    return trailBuffer_.newest().velocity;
}
std::pair<float, float> getModTrailMix() const
{
    // Only meaningful when two trails are frozen — implementation deferred to
    // when dual-freeze UI is added. For now, returns current position.
    return { circleX_, influenceY_ };
}
```

- [ ] **Step 4: Build**

```bash
cmake --build build 2>&1 | tail -10
```

- [ ] **Step 5: Commit**

```bash
git add Source/UI/PlaySurface/PlaySurface.h Source/UI/PlaySurface/XOuijaPanel.h
git commit -m "feat(PlaySurface): wire CC 85-90 output + CC input for remote planchette control"
```

---

### Task 13: Build Verification

**Files:** None (verification only)

- [ ] **Step 1: Clean build**

```bash
cd ~/Documents/GitHub/XO_OX-XOceanus
rm -rf build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build 2>&1 | tail -20
```

Expected: 0 errors. Warnings are acceptable.

- [ ] **Step 2: Run tests**

```bash
./build/XOceanusTests
```

Expected: All HarmonicField and GestureTrail tests pass. All existing tests still pass.

- [ ] **Step 3: Run auval**

```bash
auval -v aumu Xolk XoOx 2>&1 | tail -10
```

Expected: `AU VALIDATION SUCCEEDED`.

- [ ] **Step 4: Visual smoke test**

Open XOceanus in a DAW or standalone, open PlaySurface, and verify:
- XOuija panel appears on the left with circle-of-fifths markers
- Planchette drifts when idle, springs to touch, holds briefly after release
- Trail renders during drag
- FREEZE/HOME/DRIFT buttons work (Z/X/C keys)
- GOODBYE resets everything (V key)
- PAD mode shows XOuija-reactive coloring
- KEYS mode shows a scrollable keyboard
- Performance strip has tabs inside, crosshair, upgraded cursor
- CC output visible in MIDI monitor

- [ ] **Step 5: Final commit (if any fixes needed)**

```bash
git add -A
git commit -m "fix(PlaySurface): V2 integration polish from smoke test"
```

---

## Spec Coverage Check

| Spec Section | Plan Task |
|---|---|
| 2. Layout Architecture | Task 11 |
| 3. XOuija Panel | Task 4 |
| 4. Planchette (B042) | Task 5 |
| 5. Gesture Trail (B043) | Tasks 2, 6 |
| 6. Performance Gesture Buttons | Task 7 |
| 7. GOODBYE | Task 7 |
| 8.1 Mode Tab Bar | Task 11 (header restructure) |
| 8.2 PAD Mode + XOuija coloring | Task 9 |
| 8.3 DRUM Mode | No change needed (V1 retained) |
| 8.4 KEYS Mode | Task 8 |
| 9. Performance Strip | Task 10 |
| 10. Visual Language | Tasks 1, 4, 5, 8 (spread across components) |
| 11. CC/OSC Mapping | Tasks 3, 6, 12 |
| 12.1 Phase 1 Checklist | All tasks above |
| 13. Eliminated V1 Elements | Task 11 |

**Phase 2 (iOS) and Phase 3 (Network) are out of scope for this plan.**
