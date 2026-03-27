/*
    GestureTrailTests.cpp
    ======================
    Tests for GestureTrailBuffer: ring wrap, freeze/replay, interference.

    No external test framework — assert-style with descriptive console output.
    Mirrors the HarmonicFieldTests pattern.
*/

#include "GestureTrailTests.h"
#include "UI/PlaySurface/GestureTrailBuffer.h"

#include <iostream>
#include <cmath>
#include <string>

using namespace xolokun;

//==============================================================================
// Test infrastructure (local to this TU)
//==============================================================================

static int g_gt_passed = 0;
static int g_gt_failed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        ++g_gt_passed;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        ++g_gt_failed;
    }
}

//==============================================================================
// Push and count
//==============================================================================

static void testPushAndCount()
{
    std::cout << "\n--- push and count ---\n";

    GestureTrailBuffer buf;
    reportTest("empty buffer: count == 0", buf.count() == 0);

    buf.push(0.5f, 0.5f, 0.8f, 1.0);
    reportTest("after 1 push: count == 1", buf.count() == 1);

    // Push 300 more; buffer is capped at 256
    for (int i = 0; i < 300; ++i)
        buf.push(static_cast<float>(i) / 300.0f, 0.5f, 0.5f,
                 static_cast<double>(i) * 0.01);

    reportTest("after 301 total pushes: count capped at 256",
               buf.count() == GestureTrailBuffer::kBufferSize);
}

//==============================================================================
// Ring wrap — oldest advances when buffer overflows
//==============================================================================

static void testRingWrap()
{
    std::cout << "\n--- ring wrap ---\n";

    GestureTrailBuffer buf;

    // Push exactly 256 points; point 0 has x=0.0, point 255 has x=255/255
    for (int i = 0; i < 256; ++i)
        buf.push(static_cast<float>(i) / 255.0f, 0.0f, 0.5f,
                 static_cast<double>(i));

    // After 256 pushes the oldest point is the first one pushed (x ≈ 0.0)
    reportTest("after 256 pushes: oldest.x ≈ 0.0",
               std::abs(buf.oldest().x) < 1e-4f);

    // Push one more point; the first pushed point is overwritten,
    // so oldest is now point 1 (x = 1/255 ≈ 0.00392)
    buf.push(1.0f, 0.0f, 0.5f, 256.0);

    float expected = 1.0f / 255.0f;
    reportTest("after 257th push: oldest.x ≈ 1/255",
               std::abs(buf.oldest().x - expected) < 1e-3f);

    // Newest is always the last pushed
    reportTest("after 257th push: newest.x == 1.0",
               std::abs(buf.newest().x - 1.0f) < 1e-4f);
}

//==============================================================================
// Velocity clamping
//==============================================================================

static void testVelocityClamping()
{
    std::cout << "\n--- velocity clamping ---\n";

    GestureTrailBuffer buf;

    buf.push(0.5f, 0.5f, 1.5f, 0.0);   // over-range → clamped to 1.0
    reportTest("velocity 1.5 clamped to 1.0",
               std::abs(buf.newest().velocity - 1.0f) < 1e-4f);

    buf.push(0.5f, 0.5f, -0.3f, 1.0);  // under-range → clamped to 0.0
    reportTest("velocity -0.3 clamped to 0.0",
               std::abs(buf.newest().velocity - 0.0f) < 1e-4f);

    buf.push(0.5f, 0.5f, 0.7f, 2.0);   // normal value preserved
    reportTest("velocity 0.7 preserved",
               std::abs(buf.newest().velocity - 0.7f) < 1e-4f);
}

//==============================================================================
// Freeze
//==============================================================================

static void testFreeze()
{
    std::cout << "\n--- freeze ---\n";

    GestureTrailBuffer buf;
    reportTest("not frozen initially", !buf.isFrozen());

    for (int i = 0; i < 10; ++i)
        buf.push(static_cast<float>(i) / 9.0f, 0.5f, 0.5f,
                 static_cast<double>(i));

    buf.freeze();
    reportTest("frozen after freeze()", buf.isFrozen());
    reportTest("frozenCount matches live count at freeze time",
               buf.frozenCount() == 10);

    // frozenPointAt(0) returns oldest point in snapshot (x ≈ 0.0)
    TrailPoint p0 = buf.frozenPointAt(0);
    reportTest("frozenPointAt(0).x ≈ 0.0 (oldest)",
               std::abs(p0.x) < 1e-4f);

    // Pushing more points after freeze does not affect frozenCount
    buf.push(0.9f, 0.9f, 0.9f, 100.0);
    reportTest("frozenCount unchanged after post-freeze push",
               buf.frozenCount() == 10);
}

//==============================================================================
// Unfreeze
//==============================================================================

static void testUnfreeze()
{
    std::cout << "\n--- unfreeze ---\n";

    GestureTrailBuffer buf;
    buf.push(0.5f, 0.5f, 0.5f, 0.0);
    buf.freeze();
    reportTest("frozen after freeze()", buf.isFrozen());

    buf.unfreeze();
    reportTest("not frozen after unfreeze()", !buf.isFrozen());
}

//==============================================================================
// Replay
//==============================================================================

static void testReplay()
{
    std::cout << "\n--- replay ---\n";

    GestureTrailBuffer buf;

    // Push 10 points; x values = 0/9, 1/9, …, 9/9
    const int N = 10;
    for (int i = 0; i < N; ++i)
        buf.push(static_cast<float>(i) / static_cast<float>(N - 1),
                 0.0f, 0.5f, static_cast<double>(i));

    buf.freeze();

    // t=0.0 → index 0 → oldest point (x ≈ 0.0)
    TrailPoint t0 = buf.replayAt(0.0f);
    reportTest("replayAt(0.0) → oldest point (x ≈ 0.0)",
               std::abs(t0.x) < 1e-3f);

    // t=0.5 → index floor(0.5 * 10) = 5 → point 5 (x = 5/9 ≈ 0.556)
    TrailPoint t5 = buf.replayAt(0.5f);
    float expected5 = 5.0f / 9.0f;
    reportTest("replayAt(0.5) → middle point (x ≈ 5/9)",
               std::abs(t5.x - expected5) < 0.02f);

    // t=1.0 wraps back to t=0 (oldest point again)
    TrailPoint t1 = buf.replayAt(1.0f);
    reportTest("replayAt(1.0) wraps → oldest point (x ≈ 0.0)",
               std::abs(t1.x) < 1e-3f);

    // t=1.5 also wraps → same as t=0.5
    TrailPoint t15 = buf.replayAt(1.5f);
    reportTest("replayAt(1.5) wraps → same as replayAt(0.5)",
               std::abs(t15.x - t5.x) < 1e-4f);
}

//==============================================================================
// Two-trail interference
//==============================================================================

static void testInterference()
{
    std::cout << "\n--- two-trail interference ---\n";

    // X = clamp(0.3 + 0.6 - 0.5, 0, 1) = clamp(0.4, 0, 1) = 0.4
    // Y = 0.8 * 0.5 = 0.4
    TrailPoint a; a.x = 0.3f; a.y = 0.8f; a.velocity = 0.5f;
    TrailPoint b; b.x = 0.6f; b.y = 0.5f; b.velocity = 0.5f;

    auto [x, y] = GestureTrailBuffer::interference(a, b);
    reportTest("interference X = 0.4", std::abs(x - 0.4f) < 1e-4f);
    reportTest("interference Y = 0.4", std::abs(y - 0.4f) < 1e-4f);

    // Clamping: a.x=0.9, b.x=0.9 → 0.9+0.9-0.5=1.3 → clamped to 1.0
    TrailPoint c; c.x = 0.9f; c.y = 1.0f;
    TrailPoint d; d.x = 0.9f; d.y = 1.0f;
    auto [xHigh, yHigh] = GestureTrailBuffer::interference(c, d);
    reportTest("interference X clamped to 1.0 when sum > 1.5",
               std::abs(xHigh - 1.0f) < 1e-4f);
    reportTest("interference Y = 1.0 * 1.0 = 1.0",
               std::abs(yHigh - 1.0f) < 1e-4f);

    // Clamping low: a.x=0.1, b.x=0.1 → 0.1+0.1-0.5=-0.3 → clamped to 0.0
    TrailPoint e; e.x = 0.1f; e.y = 0.0f;
    TrailPoint f; f.x = 0.1f; f.y = 1.0f;
    auto [xLow, yLow] = GestureTrailBuffer::interference(e, f);
    reportTest("interference X clamped to 0.0 when sum < 0.5",
               std::abs(xLow - 0.0f) < 1e-4f);
    reportTest("interference Y = 0.0 * 1.0 = 0.0",
               std::abs(yLow - 0.0f) < 1e-4f);
}

//==============================================================================
// Clear
//==============================================================================

static void testClear()
{
    std::cout << "\n--- clear ---\n";

    GestureTrailBuffer buf;

    for (int i = 0; i < 50; ++i)
        buf.push(0.5f, 0.5f, 0.5f, static_cast<double>(i));

    buf.freeze();
    reportTest("frozen before clear", buf.isFrozen());
    reportTest("count == 50 before clear", buf.count() == 50);

    buf.clear();
    reportTest("count == 0 after clear", buf.count() == 0);
    reportTest("not frozen after clear", !buf.isFrozen());
}

//==============================================================================
// Public entry point
//==============================================================================

namespace gesture_trail_tests {

int runAll()
{
    g_gt_passed = 0;
    g_gt_failed = 0;

    std::cout << "\n========================================\n";
    std::cout << "  GestureTrail Tests (B043)\n";
    std::cout << "========================================\n";

    testPushAndCount();
    testRingWrap();
    testVelocityClamping();
    testFreeze();
    testUnfreeze();
    testReplay();
    testInterference();
    testClear();

    std::cout << "\n  GestureTrail Tests: " << g_gt_passed << " passed, "
              << g_gt_failed << " failed\n";

    return g_gt_failed;
}

} // namespace gesture_trail_tests
