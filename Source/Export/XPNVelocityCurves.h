// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <vector>

namespace xoceanus
{

//==============================================================================
// XPN Velocity Curves — shared by XOriginate and XOutshine.
//
// Defines musical velocity split ranges and volume levels for MPC programs.
// Both exporters must use these curves so drum and keygroup programs are
// consistent across the entire XPN ecosystem.
//
// Ranges are [start, end] inclusive, matching MPC VelStart/VelEnd semantics.
// Volume is a 0–1 linear scale applied as a per-layer gain offset.
//
//==============================================================================

struct VelocitySplit
{
    int start;     // MIDI velocity range start (inclusive)
    int end;       // MIDI velocity range end (inclusive)
    float volume;  // Linear gain for this layer (0.0–1.0)
    float normVel; // Normalised velocity to use when rendering this layer (0.0–1.0)
};

enum class XPNVelocityCurve
{
    Musical,  // Expressive default — non-linear, wide soft range
    BoomBap,  // Punchy 90s hip-hop — heavy bottom, explosive top
    NeoSoul,  // Smooth — compressed range, gentle top
    TrapHard, // Thin ghost layers, huge hard hits
    Linear,   // Uniform splits — diagnostic / utility use
};

inline std::vector<VelocitySplit> getVelocitySplits(XPNVelocityCurve curve, int numLayers = 4)
{
    // Full 4-layer tables — trimmed to numLayers if fewer layers are requested.
    // When trimming, the last layer always extends to vel 127.
    std::vector<VelocitySplit> full;

    switch (curve)
    {
    case XPNVelocityCurve::BoomBap:
        full = {{1, 15, 0.25f, 0.12f}, {16, 45, 0.50f, 0.35f}, {46, 85, 0.78f, 0.65f}, {86, 127, 1.00f, 1.00f}};
        break;
    case XPNVelocityCurve::NeoSoul:
        // normVel for Hard layer: 109/127 ≈ 0.86 — top of zone 4 midpoint (matches Musical curve convention)
        full = {{1, 30, 0.35f, 0.20f}, {31, 65, 0.60f, 0.47f}, {66, 95, 0.80f, 0.72f}, {96, 127, 0.95f, 0.86f}};
        break;
    case XPNVelocityCurve::TrapHard:
        full = {{1, 10, 0.20f, 0.08f}, {11, 35, 0.55f, 0.27f}, {36, 70, 0.80f, 0.60f}, {71, 127, 1.00f, 1.00f}};
        break;
    case XPNVelocityCurve::Linear:
        full = {{1, 31, 0.25f, 0.25f}, {32, 63, 0.50f, 0.50f}, {64, 95, 0.75f, 0.75f}, {96, 127, 1.00f, 1.00f}};
        break;
    case XPNVelocityCurve::Musical:
    default:
        // Ghost Council Modified zones (adopted 2026-04-04).
        // Old zone 2 end was 50, old zone 3 start was 51 (replaced).
        // normVel = zone midpoint / 127: Ghost=10/127≈0.08, Light=38/127≈0.30,
        //           Medium=73/127≈0.57, Hard=109/127≈0.86.
        full = {{1, 20, 0.30f, 0.08f}, {21, 55, 0.55f, 0.30f}, {56, 90, 0.75f, 0.57f}, {91, 127, 0.95f, 0.86f}};
        break;
    }

    if (numLayers <= 0 || numLayers >= (int)full.size())
        return full;

    // Trim: keep only the top-N layers (loudest end) and extend first retained
    // layer's start to 1 so no MIDI velocity is unmapped.
    std::vector<VelocitySplit> trimmed(full.end() - numLayers, full.end());
    trimmed.front().start = 1;
    return trimmed;
}

// Convenience: normalised render velocity for a given layer index
inline float renderVelocityForLayer(int layerIndex, int totalLayers, XPNVelocityCurve curve = XPNVelocityCurve::Musical)
{
    auto splits = getVelocitySplits(curve, totalLayers);
    if (layerIndex < 0 || layerIndex >= (int)splits.size())
        return 0.8f;
    return splits[(size_t)layerIndex].normVel;
}

} // namespace xoceanus
