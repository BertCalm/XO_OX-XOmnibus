// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "SynthEngine.h"
#include <array>

namespace xoceanus {

//==============================================================================
// BrothCoordinator — Multi-timescale diffusion coupling for the BROTH quad.
//
// The BROTH quad (OVERWASH, OVERWORN, OVERFLOW, OVERCAST) is designed to work
// as a coordinated ensemble. Each engine operates on a different temporal scale:
//
//   OVERWASH  — Distance          (3–30 s diffusion arcs)
//   OVERWORN  — Irreversibility   (30+ min session arc)
//   OVERFLOW  — Potential Energy  (phrase-scale pressure accumulation)
//   OVERCAST  — Negation          (instantaneous capture, no duration)
//
// The coordinator detects when all four engines are present and wires the
// cross-engine state sharing described in each engine's header:
//
//   OVERWORN → OVERWASH  : sessionAge      → viscosity increase
//   OVERWORN → OVERFLOW  : concentrateDark → lower pressure threshold
//   OVERWORN → OVERCAST  : totalSpectralMass → crystal seed darkness
//
// Implementation lives in XOceanusProcessor::processBrothCoupling() which
// has full access to the concrete engine types. This header provides:
//   - Engine ID constants for the four BROTH engines
//   - isBrothQuadActive() utility (engine-type-agnostic)
//
// Thread-safety:
//   processBrothCoupling() is called exclusively on the audio thread, once
//   per block, after coupling matrix processing and before mix-down.
//   All BROTH setter/getter methods are audio-thread-only by convention.
//
//==============================================================================
class BrothCoordinator
{
public:
    // Engine ID constants matching each engine's getEngineId() return value.
    static constexpr const char* kOverwash = "Overwash";
    static constexpr const char* kOverworn = "Overworn";
    static constexpr const char* kOverflow = "Overflow";
    static constexpr const char* kOvercast = "Overcast";

    //--------------------------------------------------------------------------
    // isBrothQuadActive — returns true when all four BROTH engines are present
    // in the active slots. Useful for the UI (e.g., a BROTH coupling indicator).
    //--------------------------------------------------------------------------
    template <int MaxSlots>
    static bool isBrothQuadActive (const std::array<SynthEngine*, MaxSlots>& enginePtrs)
    {
        bool hasWash = false, hasWorn = false, hasFlow = false, hasCast = false;
        for (const auto* eng : enginePtrs)
        {
            if (!eng) continue;
            const juce::String id = eng->getEngineId();
            if      (id == kOverwash) hasWash = true;
            else if (id == kOverworn) hasWorn = true;
            else if (id == kOverflow) hasFlow = true;
            else if (id == kOvercast) hasCast = true;
        }
        return hasWash && hasWorn && hasFlow && hasCast;
    }
};

} // namespace xoceanus
