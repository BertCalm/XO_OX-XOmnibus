/*
    HarmonicField Tests
    ====================
    Tests for circle-of-fifths math, tension coloring, scale membership,
    and marker properties defined in HarmonicField.h.
    Migrated to Catch2 v3: issue #81
*/

#include "HarmonicFieldTests.h"

#include <catch2/catch_test_macros.hpp>

#include "UI/PlaySurface/HarmonicField.h"

#include <cmath>

using namespace xoceanus;

//==============================================================================
// positionToKey — Circle of Fifths mapping
//==============================================================================

TEST_CASE("HarmonicField - positionToKey maps positions to correct semitones", "[playsurface][harmonic]")
{
    CHECK(HarmonicField::positionToKey(0.5f) == 0);         // C at center
    CHECK(HarmonicField::positionToKey(7.0f / 12.0f) == 7); // G
    CHECK(HarmonicField::positionToKey(5.0f / 12.0f) == 5); // F
    CHECK(HarmonicField::positionToKey(0.0f) == 6);         // Gb left edge
    CHECK(HarmonicField::positionToKey(1.0f) == 6);         // F# right edge (enharmonic)
    CHECK(HarmonicField::positionToKey(8.0f / 12.0f) == 2); // D
}

//==============================================================================
// positionToFifthsOffset / fifthsOffsetToPosition — round-trip
//==============================================================================

TEST_CASE("HarmonicField - fifthsOffset round-trips through position", "[playsurface][harmonic]")
{
    // Center: offset 0
    {
        float pos = HarmonicField::fifthsOffsetToPosition(0);
        int off = HarmonicField::positionToFifthsOffset(pos);
        CHECK(std::abs(pos - 0.5f) < 1e-4f);
        CHECK(off == 0);
    }
    // Left edge: offset -6
    {
        float pos = HarmonicField::fifthsOffsetToPosition(-6);
        int off = HarmonicField::positionToFifthsOffset(pos);
        CHECK(std::abs(pos - 0.0f) < 1e-4f);
        CHECK(off == -6);
    }
    // Right edge: offset +6
    {
        float pos = HarmonicField::fifthsOffsetToPosition(6);
        int off = HarmonicField::positionToFifthsOffset(pos);
        CHECK(std::abs(pos - 1.0f) < 1e-4f);
        CHECK(off == 6);
    }
    // Positive offsets → positions > 0.5
    CHECK(HarmonicField::fifthsOffsetToPosition(1) > 0.5f);
    CHECK(HarmonicField::fifthsOffsetToPosition(3) > 0.5f);
    // Negative offsets → positions < 0.5
    CHECK(HarmonicField::fifthsOffsetToPosition(-1) < 0.5f);
    CHECK(HarmonicField::fifthsOffsetToPosition(-3) < 0.5f);
}

//==============================================================================
// fifthsDistance — symmetry and known values
//==============================================================================

TEST_CASE("HarmonicField - fifthsDistance known values and properties", "[playsurface][harmonic]")
{
    CHECK(HarmonicField::fifthsDistance(0, 7) == 1); // C↔G
    CHECK(HarmonicField::fifthsDistance(7, 0) == 1); // G↔C (symmetric)
    CHECK(HarmonicField::fifthsDistance(0, 6) == 6); // C↔F# (tritone = max)
    CHECK(HarmonicField::fifthsDistance(6, 0) == 6); // F#↔C (symmetric)
    CHECK(HarmonicField::fifthsDistance(0, 0) == 0); // C↔C (unison)
    CHECK(HarmonicField::fifthsDistance(0, 5) == 1); // C↔F
    CHECK(HarmonicField::fifthsDistance(0, 2) == 2); // C↔D

    // Always in [0, 6]
    bool allInRange = true;
    for (int a = 0; a < 12; ++a)
        for (int b = 0; b < 12; ++b)
        {
            int d = HarmonicField::fifthsDistance(a, b);
            if (d < 0 || d > 6)
            {
                allInRange = false;
            }
        }
    CHECK(allInRange);

    // Symmetric for all pairs
    bool allSymmetric = true;
    for (int a = 0; a < 12; ++a)
        for (int b = 0; b < 12; ++b)
            if (HarmonicField::fifthsDistance(a, b) != HarmonicField::fifthsDistance(b, a))
                allSymmetric = false;
    CHECK(allSymmetric);
}

//==============================================================================
// tensionColor — correct color ranges at anchors
//==============================================================================

TEST_CASE("HarmonicField - tensionColor correct at anchor distances", "[playsurface][harmonic]")
{
    auto [r0, g0, b0] = HarmonicField::tensionColor(0);
    auto [r3, g3, b3] = HarmonicField::tensionColor(3);
    auto [r6, g6, b6] = HarmonicField::tensionColor(6);

    // dist=0 → Teal #2A9D8F
    CHECK(r0 < 0.20f);
    CHECK(g0 > 0.55f);
    CHECK(b0 > 0.50f);

    // dist=3 → Gold #E9C46A
    CHECK(r3 > 0.85f);
    CHECK(g3 > 0.65f);
    CHECK(b3 < 0.45f);

    // dist=6 → Red #E07A5F
    CHECK(r6 > 0.80f);
    CHECK(g6 < 0.50f);
    CHECK(b6 < 0.40f);

    // All channels in [0, 1]
    bool allValid = true;
    for (int d = 0; d <= 6; ++d)
    {
        auto [r, g, b] = HarmonicField::tensionColor(d);
        if (r < 0.0f || r > 1.0f || g < 0.0f || g > 1.0f || b < 0.0f || b > 1.0f)
            allValid = false;
    }
    CHECK(allValid);

    // dist=1 is strictly between teal and gold
    auto [r1, g1, b1] = HarmonicField::tensionColor(1);
    CHECK(r1 > r0);
    CHECK(r1 < r3);
}

//==============================================================================
// markerProperties — correct size/opacity at each bracket
//==============================================================================

TEST_CASE("HarmonicField - markerProperties correct at all distances", "[playsurface][harmonic]")
{
    auto [s0, o0] = HarmonicField::markerProperties(0);
    CHECK(std::abs(s0 - 1.00f) < 1e-4f);
    CHECK(std::abs(o0 - 1.00f) < 1e-4f);

    auto [s1, o1] = HarmonicField::markerProperties(1);
    CHECK(std::abs(s1 - 0.85f) < 1e-4f);
    CHECK(std::abs(o1 - 0.75f) < 1e-4f);

    auto [s2, o2] = HarmonicField::markerProperties(2);
    CHECK(std::abs(s2 - 0.85f) < 1e-4f);
    CHECK(std::abs(o2 - 0.75f) < 1e-4f);

    auto [s3, o3] = HarmonicField::markerProperties(3);
    CHECK(std::abs(s3 - 0.70f) < 1e-4f);
    CHECK(std::abs(o3 - 0.50f) < 1e-4f);

    auto [s4, o4] = HarmonicField::markerProperties(4);
    CHECK(std::abs(s4 - 0.70f) < 1e-4f);
    CHECK(std::abs(o4 - 0.50f) < 1e-4f);

    auto [s5, o5] = HarmonicField::markerProperties(5);
    CHECK(std::abs(s5 - 0.55f) < 1e-4f);
    CHECK(std::abs(o5 - 0.35f) < 1e-4f);

    auto [s6, o6] = HarmonicField::markerProperties(6);
    CHECK(std::abs(s6 - 0.55f) < 1e-4f);
    CHECK(std::abs(o6 - 0.35f) < 1e-4f);
}

//==============================================================================
// markerArcY — parabolic arc correctness
//==============================================================================

TEST_CASE("HarmonicField - markerArcY parabolic arc properties", "[playsurface][harmonic]")
{
    constexpr float amp = 8.0f;

    CHECK(std::abs(HarmonicField::markerArcY(6, amp) - (-amp)) < 1e-4f); // center → -amp
    CHECK(std::abs(HarmonicField::markerArcY(0, amp) - amp) < 1e-4f);    // left edge → +amp
    CHECK(std::abs(HarmonicField::markerArcY(12, amp) - amp) < 1e-4f);   // right edge → +amp
    CHECK(std::abs(HarmonicField::markerArcY(6) - (-8.0f)) < 1e-4f);     // default amp

    // Symmetric: idx=3 and idx=9 give same arcY
    CHECK(std::abs(HarmonicField::markerArcY(3, amp) - HarmonicField::markerArcY(9, amp)) < 1e-4f);

    // Monotonically increasing from center to right edge
    bool monotonic = true;
    float prev = HarmonicField::markerArcY(6, amp);
    for (int i = 7; i <= 12; ++i)
    {
        float cur = HarmonicField::markerArcY(i, amp);
        if (cur < prev)
            monotonic = false;
        prev = cur;
    }
    CHECK(monotonic);
}

//==============================================================================
// isInKey / isRoot — scale membership
//==============================================================================

TEST_CASE("HarmonicField - isInKey C major membership", "[playsurface][harmonic]")
{
    const int cRoot = 0;
    CHECK(HarmonicField::isInKey(60, cRoot));  // C4
    CHECK(HarmonicField::isInKey(62, cRoot));  // D4
    CHECK(HarmonicField::isInKey(64, cRoot));  // E4
    CHECK(HarmonicField::isInKey(65, cRoot));  // F4
    CHECK(HarmonicField::isInKey(67, cRoot));  // G4
    CHECK(HarmonicField::isInKey(69, cRoot));  // A4
    CHECK(HarmonicField::isInKey(71, cRoot));  // B4
    CHECK(!HarmonicField::isInKey(61, cRoot)); // C#
    CHECK(!HarmonicField::isInKey(66, cRoot)); // F#
    CHECK(!HarmonicField::isInKey(70, cRoot)); // Bb
    CHECK(HarmonicField::isInKey(72, cRoot));  // C5 (octave independent)
}

TEST_CASE("HarmonicField - isInKey G major and isRoot checks", "[playsurface][harmonic]")
{
    const int gRoot = 7;
    CHECK(HarmonicField::isInKey(66, gRoot));  // F#
    CHECK(HarmonicField::isInKey(67, gRoot));  // G
    CHECK(HarmonicField::isInKey(60, gRoot));  // C (4th)
    CHECK(!HarmonicField::isInKey(65, gRoot)); // F (not in G major)

    CHECK(HarmonicField::isRoot(60, 0));  // C is root of C major
    CHECK(!HarmonicField::isRoot(67, 0)); // G is not root of C major
    CHECK(HarmonicField::isRoot(67, 7));  // G is root of G major
}

//==============================================================================
// quantizeToNearest — chromatic → diatonic snapping
//==============================================================================

TEST_CASE("HarmonicField - quantizeToNearest snaps to nearest diatonic note", "[playsurface][harmonic]")
{
    const int cRoot = 0;

    int csResult = HarmonicField::quantizeToNearest(61, cRoot);
    CHECK((csResult == 62 || csResult == 60)); // C# → D or C

    CHECK(HarmonicField::quantizeToNearest(60, cRoot) == 60); // C unchanged
    CHECK(HarmonicField::quantizeToNearest(67, cRoot) == 67); // G unchanged
    CHECK(HarmonicField::quantizeToNearest(71, cRoot) == 71); // B unchanged

    int bbResult = HarmonicField::quantizeToNearest(70, cRoot);
    CHECK((bbResult == 69 || bbResult == 71)); // Bb → A or B

    // Result always in key
    bool quantizeStaysInKey = true;
    for (int note = 48; note < 84; ++note)
    {
        int q = HarmonicField::quantizeToNearest(note, cRoot);
        if (!HarmonicField::isInKey(q, cRoot))
            quantizeStaysInKey = false;
    }
    CHECK(quantizeStaysInKey);

    int fResult = HarmonicField::quantizeToNearest(65, 7); // F in G major
    CHECK((fResult == 66 || fResult == 64));
}

// Backward-compat shim
namespace playsurface_tests
{
int runAll()
{
    return 0;
}
} // namespace playsurface_tests
