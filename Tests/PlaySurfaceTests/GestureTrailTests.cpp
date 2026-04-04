/*
    GestureTrailTests.cpp
    ======================
    Tests for GestureTrailBuffer: ring wrap, freeze/replay, interference.
    Migrated to Catch2 v3: issue #81
*/

#include "GestureTrailTests.h"

#include <catch2/catch_test_macros.hpp>

#include "UI/PlaySurface/GestureTrailBuffer.h"

#include <cmath>

using namespace xoceanus;

TEST_CASE("GestureTrail - empty buffer count is 0", "[gesture][trail]")
{
    GestureTrailBuffer buf;
    CHECK(buf.count() == 0);
}

TEST_CASE("GestureTrail - push and count, capped at kBufferSize", "[gesture][trail]")
{
    GestureTrailBuffer buf;
    buf.push(0.5f, 0.5f, 0.8f, 1.0);
    CHECK(buf.count() == 1);

    for (int i = 0; i < 300; ++i)
        buf.push(static_cast<float>(i) / 300.0f, 0.5f, 0.5f, static_cast<double>(i) * 0.01);

    CHECK(buf.count() == GestureTrailBuffer::kBufferSize);
}

TEST_CASE("GestureTrail - ring wrap overwrites oldest entry", "[gesture][trail]")
{
    GestureTrailBuffer buf;

    for (int i = 0; i < 256; ++i)
        buf.push(static_cast<float>(i) / 255.0f, 0.0f, 0.5f, static_cast<double>(i));

    CHECK(std::abs(buf.oldest().x) < 1e-4f); // oldest = first pushed (x ≈ 0.0)

    buf.push(1.0f, 0.0f, 0.5f, 256.0);

    float expected = 1.0f / 255.0f;
    CHECK(std::abs(buf.oldest().x - expected) < 1e-3f); // first overwritten, oldest = idx 1
    CHECK(std::abs(buf.newest().x - 1.0f) < 1e-4f);     // newest = last pushed
}

TEST_CASE("GestureTrail - velocity clamping to [0, 1]", "[gesture][trail]")
{
    GestureTrailBuffer buf;

    buf.push(0.5f, 0.5f, 1.5f, 0.0);
    CHECK(std::abs(buf.newest().velocity - 1.0f) < 1e-4f);

    buf.push(0.5f, 0.5f, -0.3f, 1.0);
    CHECK(std::abs(buf.newest().velocity - 0.0f) < 1e-4f);

    buf.push(0.5f, 0.5f, 0.7f, 2.0);
    CHECK(std::abs(buf.newest().velocity - 0.7f) < 1e-4f);
}

TEST_CASE("GestureTrail - freeze captures snapshot, post-freeze pushes do not change frozenCount", "[gesture][trail]")
{
    GestureTrailBuffer buf;
    CHECK(!buf.isFrozen());

    for (int i = 0; i < 10; ++i)
        buf.push(static_cast<float>(i) / 9.0f, 0.5f, 0.5f, static_cast<double>(i));

    buf.freeze();
    CHECK(buf.isFrozen());
    CHECK(buf.frozenCount() == 10);

    TrailPoint p0 = buf.frozenPointAt(0);
    CHECK(std::abs(p0.x) < 1e-4f); // oldest in snapshot

    buf.push(0.9f, 0.9f, 0.9f, 100.0);
    CHECK(buf.frozenCount() == 10); // snapshot unchanged
}

TEST_CASE("GestureTrail - unfreeze clears frozen state", "[gesture][trail]")
{
    GestureTrailBuffer buf;
    buf.push(0.5f, 0.5f, 0.5f, 0.0);
    buf.freeze();
    CHECK(buf.isFrozen());
    buf.unfreeze();
    CHECK(!buf.isFrozen());
}

TEST_CASE("GestureTrail - replayAt samples frozen snapshot at normalized positions", "[gesture][trail]")
{
    GestureTrailBuffer buf;
    const int N = 10;
    for (int i = 0; i < N; ++i)
        buf.push(static_cast<float>(i) / static_cast<float>(N - 1), 0.0f, 0.5f, static_cast<double>(i));

    buf.freeze();

    TrailPoint t0 = buf.replayAt(0.0f);
    CHECK(std::abs(t0.x) < 1e-3f); // oldest (x ≈ 0.0)

    TrailPoint t5 = buf.replayAt(0.5f);
    CHECK(std::abs(t5.x - (5.0f / 9.0f)) < 0.02f); // idx 5

    TrailPoint t1 = buf.replayAt(1.0f);
    CHECK(std::abs(t1.x) < 1e-3f); // wraps back to oldest

    TrailPoint t15 = buf.replayAt(1.5f);
    CHECK(std::abs(t15.x - t5.x) < 1e-4f); // t=1.5 == t=0.5
}

TEST_CASE("GestureTrail - interference combines two trail points", "[gesture][trail]")
{
    // X = clamp(0.3 + 0.6 - 0.5, 0, 1) = 0.4; Y = 0.8 * 0.5 = 0.4
    TrailPoint a;
    a.x = 0.3f;
    a.y = 0.8f;
    a.velocity = 0.5f;
    TrailPoint b;
    b.x = 0.6f;
    b.y = 0.5f;
    b.velocity = 0.5f;
    auto [x, y] = GestureTrailBuffer::interference(a, b);
    CHECK(std::abs(x - 0.4f) < 1e-4f);
    CHECK(std::abs(y - 0.4f) < 1e-4f);

    // High clamping: 0.9+0.9-0.5=1.3 → 1.0
    TrailPoint c;
    c.x = 0.9f;
    c.y = 1.0f;
    TrailPoint d;
    d.x = 0.9f;
    d.y = 1.0f;
    auto [xHigh, yHigh] = GestureTrailBuffer::interference(c, d);
    CHECK(std::abs(xHigh - 1.0f) < 1e-4f);
    CHECK(std::abs(yHigh - 1.0f) < 1e-4f);

    // Low clamping: 0.1+0.1-0.5=-0.3 → 0.0
    TrailPoint e;
    e.x = 0.1f;
    e.y = 0.0f;
    TrailPoint f;
    f.x = 0.1f;
    f.y = 1.0f;
    auto [xLow, yLow] = GestureTrailBuffer::interference(e, f);
    CHECK(std::abs(xLow - 0.0f) < 1e-4f);
    CHECK(std::abs(yLow - 0.0f) < 1e-4f);
}

TEST_CASE("GestureTrail - clear resets count and frozen state", "[gesture][trail]")
{
    GestureTrailBuffer buf;
    for (int i = 0; i < 50; ++i)
        buf.push(0.5f, 0.5f, 0.5f, static_cast<double>(i));
    buf.freeze();

    buf.clear();
    CHECK(buf.count() == 0);
    CHECK(!buf.isFrozen());
}

