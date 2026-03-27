/*
    HarmonicField Tests
    ====================
    Tests for circle-of-fifths math, tension coloring, scale membership,
    and marker properties defined in HarmonicField.h.

    No external test framework — assert-style with descriptive console output.
*/

#include "HarmonicFieldTests.h"
#include "UI/PlaySurface/HarmonicField.h"

#include <iostream>
#include <cmath>
#include <string>
#include <tuple>

using namespace xolokun;

//==============================================================================
// Test infrastructure
//==============================================================================

static int g_passed = 0;
static int g_failed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        ++g_passed;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        ++g_failed;
    }
}

//==============================================================================
// positionToKey — Circle of Fifths mapping
//==============================================================================

static void testPositionToKey()
{
    std::cout << "\n--- positionToKey ---\n";

    // C is at the center (x=0.5) → semitone 0
    reportTest("x=0.5 → C (semitone 0)",
               HarmonicField::positionToKey(0.5f) == 0);

    // G is one step right of C on the circle (idx 7 out of 0-12, x=7/12≈0.583)
    // G = semitone 7
    reportTest("x≈0.583 (G) → semitone 7",
               HarmonicField::positionToKey(7.0f / 12.0f) == 7);

    // F is one step left of C (idx 5, x=5/12≈0.417)
    // F = semitone 5
    reportTest("x≈0.417 (F) → semitone 5",
               HarmonicField::positionToKey(5.0f / 12.0f) == 5);

    // Left edge (x=0.0) → Gb = semitone 6
    reportTest("x=0.0 → Gb (semitone 6)",
               HarmonicField::positionToKey(0.0f) == 6);

    // Right edge (x=1.0) → F# = semitone 6 (enharmonic with Gb)
    reportTest("x=1.0 → F# (semitone 6)",
               HarmonicField::positionToKey(1.0f) == 6);

    // D is two steps right of C (idx 8, x=8/12≈0.667) → semitone 2
    reportTest("x≈0.667 (D) → semitone 2",
               HarmonicField::positionToKey(8.0f / 12.0f) == 2);
}

//==============================================================================
// positionToFifthsOffset / fifthsOffsetToPosition — round-trip
//==============================================================================

static void testFifthsOffsetRoundTrip()
{
    std::cout << "\n--- fifthsOffset round-trip ---\n";

    // Center: offset 0 → position 0.5 → offset 0
    {
        float pos = HarmonicField::fifthsOffsetToPosition(0);
        int   off = HarmonicField::positionToFifthsOffset(pos);
        reportTest("offset 0 round-trips through position",
                   std::abs(pos - 0.5f) < 1e-4f && off == 0);
    }

    // Left edge: offset -6 → position 0.0 → offset -6
    {
        float pos = HarmonicField::fifthsOffsetToPosition(-6);
        int   off = HarmonicField::positionToFifthsOffset(pos);
        reportTest("offset -6 round-trips through position",
                   std::abs(pos - 0.0f) < 1e-4f && off == -6);
    }

    // Right edge: offset +6 → position 1.0 → offset +6
    {
        float pos = HarmonicField::fifthsOffsetToPosition(6);
        int   off = HarmonicField::positionToFifthsOffset(pos);
        reportTest("offset +6 round-trips through position",
                   std::abs(pos - 1.0f) < 1e-4f && off == 6);
    }

    // Positive offsets produce positions > 0.5
    {
        float posPlus1 = HarmonicField::fifthsOffsetToPosition(1);
        float posPlus3 = HarmonicField::fifthsOffsetToPosition(3);
        reportTest("positive offsets produce positions > 0.5",
                   posPlus1 > 0.5f && posPlus3 > 0.5f);
    }

    // Negative offsets produce positions < 0.5
    {
        float posMinus1 = HarmonicField::fifthsOffsetToPosition(-1);
        float posMinus3 = HarmonicField::fifthsOffsetToPosition(-3);
        reportTest("negative offsets produce positions < 0.5",
                   posMinus1 < 0.5f && posMinus3 < 0.5f);
    }
}

//==============================================================================
// fifthsDistance — symmetry and known values
//==============================================================================

static void testFifthsDistance()
{
    std::cout << "\n--- fifthsDistance ---\n";

    // C (0) to G (7) = 1 fifth apart
    reportTest("C↔G = 1 fifth",
               HarmonicField::fifthsDistance(0, 7) == 1);

    // G (7) to C (0) = 1 (symmetric)
    reportTest("G↔C = 1 fifth (symmetric)",
               HarmonicField::fifthsDistance(7, 0) == 1);

    // C (0) to F# (6) = 6 fifths (tritone = maximum distance)
    reportTest("C↔F# = 6 fifths (max distance)",
               HarmonicField::fifthsDistance(0, 6) == 6);

    // F# (6) to C (0) = 6 (symmetric)
    reportTest("F#↔C = 6 fifths (symmetric)",
               HarmonicField::fifthsDistance(6, 0) == 6);

    // C to C = 0
    reportTest("C↔C = 0 fifths (unison)",
               HarmonicField::fifthsDistance(0, 0) == 0);

    // C (0) to F (5) = 1 fifth (F is one fifth below/left of C)
    reportTest("C↔F = 1 fifth",
               HarmonicField::fifthsDistance(0, 5) == 1);

    // C (0) to D (2) = 2 fifths
    reportTest("C↔D = 2 fifths",
               HarmonicField::fifthsDistance(0, 2) == 2);

    // Maximum distance is always <= 6
    bool allWithinRange = true;
    for (int a = 0; a < 12; ++a)
        for (int b = 0; b < 12; ++b)
        {
            int d = HarmonicField::fifthsDistance(a, b);
            if (d < 0 || d > 6) { allWithinRange = false; }
        }
    reportTest("fifthsDistance always in [0, 6]", allWithinRange);

    // Distance is symmetric for all pairs
    bool allSymmetric = true;
    for (int a = 0; a < 12; ++a)
        for (int b = 0; b < 12; ++b)
            if (HarmonicField::fifthsDistance(a, b) != HarmonicField::fifthsDistance(b, a))
                allSymmetric = false;
    reportTest("fifthsDistance is symmetric for all pairs", allSymmetric);
}

//==============================================================================
// tensionColor — correct color ranges at anchors
//==============================================================================

static void testTensionColor()
{
    std::cout << "\n--- tensionColor ---\n";

    auto [r0, g0, b0] = HarmonicField::tensionColor(0);
    auto [r3, g3, b3] = HarmonicField::tensionColor(3);
    auto [r6, g6, b6] = HarmonicField::tensionColor(6);

    // dist=0 → Teal #2A9D8F (r low, g high, b moderate)
    reportTest("dist=0: teal — r < 0.20",   r0 < 0.20f);
    reportTest("dist=0: teal — g > 0.55",   g0 > 0.55f);
    reportTest("dist=0: teal — b > 0.50",   b0 > 0.50f);

    // dist=3 → Gold #E9C46A (r high, g moderate, b low)
    reportTest("dist=3: gold — r > 0.85",   r3 > 0.85f);
    reportTest("dist=3: gold — g > 0.65",   g3 > 0.65f);
    reportTest("dist=3: gold — b < 0.45",   b3 < 0.45f);

    // dist=6 → Red #E07A5F (r high, g low, b low)
    reportTest("dist=6: red — r > 0.80",    r6 > 0.80f);
    reportTest("dist=6: red — g < 0.50",    g6 < 0.50f);
    reportTest("dist=6: red — b < 0.40",    b6 < 0.40f);

    // All channels must be in [0, 1]
    bool allValid = true;
    for (int d = 0; d <= 6; ++d)
    {
        auto [r, g, b] = HarmonicField::tensionColor(d);
        if (r < 0.0f || r > 1.0f || g < 0.0f || g > 1.0f || b < 0.0f || b > 1.0f)
            allValid = false;
    }
    reportTest("tensionColor: all RGB values in [0, 1]", allValid);

    // Colors at dist=1 and dist=2 are strictly between teal and gold
    auto [r1, g1, b1] = HarmonicField::tensionColor(1);
    reportTest("dist=1: r is between teal and gold", r1 > r0 && r1 < r3);
}

//==============================================================================
// markerProperties — correct size/opacity at each bracket
//==============================================================================

static void testMarkerProperties()
{
    std::cout << "\n--- markerProperties ---\n";

    auto [s0, o0] = HarmonicField::markerProperties(0);
    reportTest("dist=0: size=1.0",    std::abs(s0 - 1.00f) < 1e-4f);
    reportTest("dist=0: opacity=1.0", std::abs(o0 - 1.00f) < 1e-4f);

    auto [s1, o1] = HarmonicField::markerProperties(1);
    reportTest("dist=1: size=0.85",    std::abs(s1 - 0.85f) < 1e-4f);
    reportTest("dist=1: opacity=0.75", std::abs(o1 - 0.75f) < 1e-4f);

    auto [s2, o2] = HarmonicField::markerProperties(2);
    reportTest("dist=2: size=0.85",    std::abs(s2 - 0.85f) < 1e-4f);
    reportTest("dist=2: opacity=0.75", std::abs(o2 - 0.75f) < 1e-4f);

    auto [s3, o3] = HarmonicField::markerProperties(3);
    reportTest("dist=3: size=0.70",    std::abs(s3 - 0.70f) < 1e-4f);
    reportTest("dist=3: opacity=0.50", std::abs(o3 - 0.50f) < 1e-4f);

    auto [s4, o4] = HarmonicField::markerProperties(4);
    reportTest("dist=4: size=0.70",    std::abs(s4 - 0.70f) < 1e-4f);
    reportTest("dist=4: opacity=0.50", std::abs(o4 - 0.50f) < 1e-4f);

    auto [s5, o5] = HarmonicField::markerProperties(5);
    reportTest("dist=5: size=0.55",    std::abs(s5 - 0.55f) < 1e-4f);
    reportTest("dist=5: opacity=0.35", std::abs(o5 - 0.35f) < 1e-4f);

    auto [s6, o6] = HarmonicField::markerProperties(6);
    reportTest("dist=6: size=0.55",    std::abs(s6 - 0.55f) < 1e-4f);
    reportTest("dist=6: opacity=0.35", std::abs(o6 - 0.35f) < 1e-4f);
}

//==============================================================================
// markerArcY — parabolic arc correctness
//==============================================================================

static void testMarkerArcY()
{
    std::cout << "\n--- markerArcY ---\n";

    constexpr float amp = 8.0f;

    // Center (idx=6): norm=0 → arcY = amp*(2*0-1) = -amp
    float center = HarmonicField::markerArcY(6, amp);
    reportTest("idx=6 (center) → -amplitude",
               std::abs(center - (-amp)) < 1e-4f);

    // Left edge (idx=0): norm=-1 → arcY = amp*(2*1-1) = +amp
    float leftEdge = HarmonicField::markerArcY(0, amp);
    reportTest("idx=0 (left edge) → +amplitude",
               std::abs(leftEdge - amp) < 1e-4f);

    // Right edge (idx=12): norm=+1 → arcY = amp*(2*1-1) = +amp
    float rightEdge = HarmonicField::markerArcY(12, amp);
    reportTest("idx=12 (right edge) → +amplitude",
               std::abs(rightEdge - amp) < 1e-4f);

    // Default amplitude (8.0)
    float defaultAmp = HarmonicField::markerArcY(6);
    reportTest("idx=6 default amplitude → -8.0",
               std::abs(defaultAmp - (-8.0f)) < 1e-4f);

    // Symmetric: idx=3 and idx=9 should give the same arcY
    float left3  = HarmonicField::markerArcY(3, amp);
    float right9 = HarmonicField::markerArcY(9, amp);
    reportTest("idx=3 and idx=9 are symmetric",
               std::abs(left3 - right9) < 1e-4f);

    // Values increase monotonically from center to edges (parabola)
    bool monotonic = true;
    float prev = HarmonicField::markerArcY(6, amp);
    for (int i = 7; i <= 12; ++i)
    {
        float cur = HarmonicField::markerArcY(i, amp);
        if (cur < prev) monotonic = false;
        prev = cur;
    }
    reportTest("markerArcY increases from center to right edge", monotonic);
}

//==============================================================================
// isInKey — C major and G major membership
//==============================================================================

static void testIsInKey()
{
    std::cout << "\n--- isInKey ---\n";

    // C major (root=0): scale degrees 0,2,4,5,7,9,11 (C D E F G A B)
    const int cMajorRoot = 0;
    reportTest("C in C major",  HarmonicField::isInKey(60, cMajorRoot));  // C4
    reportTest("D in C major",  HarmonicField::isInKey(62, cMajorRoot));  // D4
    reportTest("E in C major",  HarmonicField::isInKey(64, cMajorRoot));  // E4
    reportTest("F in C major",  HarmonicField::isInKey(65, cMajorRoot));  // F4
    reportTest("G in C major",  HarmonicField::isInKey(67, cMajorRoot));  // G4
    reportTest("A in C major",  HarmonicField::isInKey(69, cMajorRoot));  // A4
    reportTest("B in C major",  HarmonicField::isInKey(71, cMajorRoot));  // B4

    // Chromatic notes NOT in C major
    reportTest("C# NOT in C major",  !HarmonicField::isInKey(61, cMajorRoot));
    reportTest("F# NOT in C major",  !HarmonicField::isInKey(66, cMajorRoot));
    reportTest("Bb NOT in C major",  !HarmonicField::isInKey(70, cMajorRoot));

    // G major (root=7): scale degrees 0,2,4,5,7,9,11 → G A B C D E F#
    const int gMajorRoot = 7;
    reportTest("F# in G major",  HarmonicField::isInKey(66, gMajorRoot));  // F#4
    reportTest("G in G major",   HarmonicField::isInKey(67, gMajorRoot));  // G4
    reportTest("C in G major",   HarmonicField::isInKey(60, gMajorRoot));  // C4 (4th)
    reportTest("F NOT in G major",  !HarmonicField::isInKey(65, gMajorRoot));  // F4 is NOT in G major

    // Octave independence: C5 should also be in C major
    reportTest("C5 in C major (octave independent)",
               HarmonicField::isInKey(72, cMajorRoot));

    // isRoot checks
    reportTest("C4 is root of C major",
               HarmonicField::isRoot(60, cMajorRoot));
    reportTest("G4 is NOT root of C major",
               !HarmonicField::isRoot(67, cMajorRoot));
    reportTest("G4 is root of G major",
               HarmonicField::isRoot(67, gMajorRoot));
}

//==============================================================================
// quantizeToNearest — chromatic → diatonic snapping
//==============================================================================

static void testQuantizeToNearest()
{
    std::cout << "\n--- quantizeToNearest ---\n";

    const int cRoot = 0;

    // C# (61) in C major should snap to C (60) or D (62) — whichever is nearest.
    // +1 → D (62) is checked first; -1 → C (60) is checked second.
    // Both are equidistant (1 semitone away). Per the implementation, +delta
    // is tested before -delta, so C# → D (62).
    int csResult = HarmonicField::quantizeToNearest(61, cRoot);
    reportTest("C# in C major → D (62) or C (60)",
               csResult == 62 || csResult == 60);

    // A note already in key should be returned unchanged
    reportTest("C (in C major) unchanged",
               HarmonicField::quantizeToNearest(60, cRoot) == 60);
    reportTest("G (in C major) unchanged",
               HarmonicField::quantizeToNearest(67, cRoot) == 67);
    reportTest("B (in C major) unchanged",
               HarmonicField::quantizeToNearest(71, cRoot) == 71);

    // Bb (70) in C major: nearest are A (69) at -1 or B (71) at +1.
    // +1 → B (71) found first.
    int bbResult = HarmonicField::quantizeToNearest(70, cRoot);
    reportTest("Bb in C major → A (69) or B (71)",
               bbResult == 69 || bbResult == 71);

    // Result of quantize should always be in key
    bool quantizeStaysInKey = true;
    for (int note = 48; note < 84; ++note)
    {
        int quantized = HarmonicField::quantizeToNearest(note, cRoot);
        if (!HarmonicField::isInKey(quantized, cRoot))
            quantizeStaysInKey = false;
    }
    reportTest("quantizeToNearest result is always in C major (range 48-83)",
               quantizeStaysInKey);

    // G major test: F (65) → F# (66) or E (64)
    int fResult = HarmonicField::quantizeToNearest(65, 7);  // G major root
    reportTest("F in G major → F# (66) or E (64)",
               fResult == 66 || fResult == 64);
}

//==============================================================================
// Public entry point
//==============================================================================

namespace playsurface_tests {

int runAll()
{
    g_passed = 0;
    g_failed = 0;

    std::cout << "\n========================================\n";
    std::cout << "  PlaySurface Tests (HarmonicField)\n";
    std::cout << "========================================\n";

    testPositionToKey();
    testFifthsOffsetRoundTrip();
    testFifthsDistance();
    testTensionColor();
    testMarkerProperties();
    testMarkerArcY();
    testIsInKey();
    testQuantizeToNearest();

    std::cout << "\n  HarmonicField Tests: " << g_passed << " passed, "
              << g_failed << " failed\n";

    return g_failed;
}

} // namespace playsurface_tests
