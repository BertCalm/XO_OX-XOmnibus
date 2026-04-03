// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include <cmath>
#include <algorithm>

namespace xocelot
{

// EcosystemMatrix — dispatches the 12 typed cross-feed routes.
//
// Route types (compile-time, per route):
//   Continuous  — linear bipolar scalar. amount: depth. negative: inverts polarity.
//   Threshold   — sigmoid (k=8) on source amplitude. amount: sensitivity (closer to 0 = fires more easily).
//                 negative: inverse trigger (silence fires, not amplitude peak).
//   Rhythmic    — stepped, quantized. amount: intensity of division shift.
//                 negative: rhythmic opposition (dense source = sparse destination).
//
// The EcosystemMatrix reads from StrataSignals (output signals from each stratum
// this block) and writes to StrataModulation (modulation inputs for next block).
// All per-block arithmetic — no per-sample branches.

struct StrataSignals
{
    float floorAmplitude = 0.0f;    // RMS amplitude of Floor output this block
    float floorTimbre = 0.0f;       // brightness proxy (spectral centroid, 0-1)
    float understoryEnergy = 0.0f;  // RMS amplitude of Understory output
    float understoryPitch = 0.0f;   // dominant pitch of chop buffer (0-1, normalized)
    float canopyAmplitude = 0.0f;   // RMS amplitude of Canopy output
    float canopySpectral = 0.0f;    // spectral centroid of Canopy (0-1)
    float emergentAmplitude = 0.0f; // RMS of Emergent output
    float emergentPattern = 0.0f;   // rhythm density of Emergent calls (0-1)
};

struct StrataModulation
{
    // Modulations APPLIED TO each stratum next block
    float floorSwingMod = 0.0f;   // additive swing offset
    float floorDampingMod = 0.0f; // additive damping offset
    float floorAccentMod = 0.0f;  // multiplicative accent level (1.0 = no change)

    float understoryChopRateMod = 0.0f; // chop rate division shift
    float understoryGrainPosMod = 0.0f; // grain read position offset (0-1)
    float understoryScatterMod = 0.0f;  // scatter timing shift

    float canopyFilterMod = 0.0f;  // additive spectral filter position
    float canopyMorphMod = 0.0f;   // additive wavefold depth
    float canopyShimmerMod = 0.0f; // additive shimmer depth

    float emergentTriggerMod = 0.0f; // trigger gate (0-1 — combined threshold output)
    float emergentPitchMod = 0.0f;   // additive pitch offset
    float emergentFormantMod = 0.0f; // additive formant offset
};

class EcosystemMatrix
{
public:
    // Process all 12 routes. ecosystemDepth scales the overall cross-feed intensity.
    StrataModulation process(const StrataSignals& sig, const struct OcelotParamSnapshot& snap,
                             float ecosystemDepth) const
    {
        StrataModulation mod;

        // ── Continuous routes (linear bipolar) ────────────────────────
        // Floor → Canopy: filter
        mod.canopyFilterMod += continuous(sig.floorAmplitude, snap.xfFloorCanopy);

        // Understory → Floor: swing
        mod.floorSwingMod += continuous(sig.understoryEnergy, snap.xfUnderFloor);

        // Understory → Canopy: morph (wavefold)
        mod.canopyMorphMod += continuous(sig.understoryEnergy, snap.xfUnderCanopy);

        // Understory → Emergent: pitch
        mod.emergentPitchMod += continuous(sig.understoryPitch, snap.xfUnderEmerg);

        // Canopy → Floor: damp
        mod.floorDampingMod += continuous(sig.canopySpectral, snap.xfCanopyFloor);

        // Canopy → Understory: grain pos
        mod.understoryGrainPosMod += continuous(sig.canopyAmplitude, snap.xfCanopyUnder);

        // Canopy → Emergent: formant
        mod.emergentFormantMod += continuous(sig.canopySpectral, snap.xfCanopyEmerg);

        // Emergent → Canopy: shimmer
        mod.canopyShimmerMod += continuous(sig.emergentAmplitude, snap.xfEmergCanopy);

        // ── Threshold routes (sigmoid, negative = inverse) ─────────────
        // Floor → Emergent: trigger
        mod.emergentTriggerMod += threshold(sig.floorAmplitude, snap.xfFloorEmerg);

        // Emergent → Floor: accent
        mod.floorAccentMod += threshold(sig.emergentAmplitude, snap.xfEmergFloor);

        // ── Rhythmic routes (stepped/quantized) ────────────────────────
        // Floor → Understory: chop rate
        mod.understoryChopRateMod += rhythmic(sig.floorAmplitude, snap.xfFloorUnder);

        // Emergent → Understory: scatter
        mod.understoryScatterMod += rhythmic(sig.emergentPattern, snap.xfEmergUnder);

        // Scale all modulation by master ecosystemDepth
        applyDepth(mod, ecosystemDepth);

        return mod;
    }

private:
    // Continuous: linear bipolar. source in [0,1], amount in [-1,1].
    static float continuous(float source, float amount) { return source * amount; }

    // Threshold: sigmoid, amount controls sensitivity threshold.
    // Positive amount: large amplitude fires.
    // Negative amount: silence fires (inverse — like animals going quiet).
    static float threshold(float source, float amount)
    {
        if (std::abs(amount) < 0.001f)
            return 0.0f;

        // Invert source for negative amounts (inverse trigger)
        float s = (amount < 0.0f) ? (1.0f - source) : source;
        float sensitivity = std::abs(amount);

        // Map amount [0,1] to threshold position: amount=1 -> threshold=0.1 (fires easily)
        float thresh = 1.0f - sensitivity * 0.9f;

        // Soft-knee sigmoid with k=8
        float x = (s - thresh) * 8.0f;
        return 1.0f / (1.0f + std::exp(-x));
    }

    // Rhythmic: stepped. amount controls intensity of shift.
    // Positive: fast source = fast destination.
    // Negative: fast source = slow destination (opposition).
    static float rhythmic(float source, float amount)
    {
        if (std::abs(amount) < 0.001f)
            return 0.0f;
        float direction = (amount >= 0.0f) ? 1.0f : -1.0f;
        return source * std::abs(amount) * direction;
    }

    // Scale all modulation fields by ecosystem depth
    static void applyDepth(StrataModulation& mod, float depth)
    {
        mod.floorSwingMod *= depth;
        mod.floorDampingMod *= depth;
        mod.floorAccentMod *= depth;
        mod.understoryChopRateMod *= depth;
        mod.understoryGrainPosMod *= depth;
        mod.understoryScatterMod *= depth;
        mod.canopyFilterMod *= depth;
        mod.canopyMorphMod *= depth;
        mod.canopyShimmerMod *= depth;
        mod.emergentTriggerMod *= depth;
        mod.emergentPitchMod *= depth;
        mod.emergentFormantMod *= depth;
    }
};

} // namespace xocelot
