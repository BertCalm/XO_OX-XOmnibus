#pragma once
#include "MegaCouplingMatrix.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <cmath>

namespace xomnibus {

//==============================================================================
// FamilyPentagonCoupling
//
// Named relationship routes + cross-engine macro bleed for the XOrphica
// Family Pentagon (Ohm, Orphica, Obbligato, Ottoni, Ole).
//
// Two systems:
//   1. Family Coupling Routes — pre-wired normalled routes based on family
//      relationships (marriage, parent-child, siblings, cousins, in-laws).
//   2. Macro Bleed — when 2+ pentagon engines are loaded, their signature
//      macros subtly influence each other (±10-15% nudge).
//
// Install: call installFamilyRoutes() after engines are loaded into slots.
// Process: call processBleed() once per block before engine rendering.
//==============================================================================
class FamilyPentagonCoupling {
public:
    // Pentagon engine IDs
    static constexpr const char* kOhm       = "Ohm";
    static constexpr const char* kOrphica   = "Orphica";
    static constexpr const char* kObbligato = "Obbligato";
    static constexpr const char* kOttoni    = "Ottoni";
    static constexpr const char* kOle       = "Ole";

    //--------------------------------------------------------------------------
    // Family Coupling Route Definitions
    //
    // Each pentagon edge maps to a specific CouplingType with a descriptive name.
    // These are installed as normalled routes in the MegaCouplingMatrix.
    //--------------------------------------------------------------------------
    struct FamilyRoute {
        const char* name;            // Human-readable relationship name
        const char* sourceEngine;
        const char* destEngine;
        CouplingType type;
        float defaultAmount;         // 0.0-1.0, typically 0.15-0.35
    };

    // The 6 pentagon edges + 5 diagonals = 11 family routes (bidirectional = 11 more)
    static inline const FamilyRoute kFamilyRoutes[] = {
        // Pentagon Edges
        {"Resonance Sympathy",  kOrphica, kOhm,       CouplingType::PitchToPitch,    0.20f},
        {"Resonance Sympathy",  kOhm,     kOrphica,   CouplingType::PitchToPitch,    0.20f},
        {"Harmonic Argument",   kOrphica, kOle,        CouplingType::AmpToFilter,     0.25f},
        {"Groove Inheritance",  kOhm,     kObbligato,  CouplingType::RhythmToBlend,   0.18f},
        {"Temperature Exchange",kObbligato,kOttoni,    CouplingType::FilterToFilter,  0.22f},
        {"Temperature Exchange",kOttoni,  kObbligato,  CouplingType::FilterToFilter,  0.22f},
        {"Age Teaches Fire",    kOttoni,  kOle,        CouplingType::AmpToFilter,     0.15f},
        {"Drama Bleeds",        kOle,     kOhm,        CouplingType::EnvToMorph,      0.20f},

        // Pentagon Diagonals
        {"Surface Current",     kOrphica, kOttoni,     CouplingType::AmpToPitch,      0.10f},
        {"Divine Inheritance",  kOrphica, kObbligato,  CouplingType::EnvToDecay,      0.12f},
        {"Warmth Thaw",         kOhm,     kOttoni,     CouplingType::AmpToFilter,     0.12f},
        {"Campfire Circle",     kOhm,     kOle,        CouplingType::AudioToRing,     0.08f},
        {"Wind Over Strings",   kObbligato,kOle,       CouplingType::AudioToFM,       0.10f},
    };

    static constexpr int kNumFamilyRoutes = sizeof(kFamilyRoutes) / sizeof(kFamilyRoutes[0]);

    //--------------------------------------------------------------------------
    // Install family routes into the MegaCouplingMatrix.
    // Call after loading pentagon engines into slots.
    // engineSlots: map of engine ID → slot index (0-3)
    //--------------------------------------------------------------------------
    static void installFamilyRoutes (
        MegaCouplingMatrix& matrix,
        const std::unordered_map<std::string, int>& engineSlots)
    {
        for (int i = 0; i < kNumFamilyRoutes; ++i)
        {
            const auto& fr = kFamilyRoutes[i];
            auto srcIt  = engineSlots.find (fr.sourceEngine);
            auto destIt = engineSlots.find (fr.destEngine);

            // Only install route if both engines are loaded
            if (srcIt == engineSlots.end() || destIt == engineSlots.end())
                continue;

            MegaCouplingMatrix::CouplingRoute route;
            route.sourceSlot  = srcIt->second;
            route.destSlot    = destIt->second;
            route.type        = fr.type;
            route.amount      = fr.defaultAmount;
            route.isNormalled = true;
            route.active      = true;
            matrix.addRoute (route);
        }
    }

    //--------------------------------------------------------------------------
    // Macro Bleed System
    //
    // When 2+ pentagon engines are active, their signature macros influence
    // each other with a ±10-15% nudge. The COMMUNE macro is special —
    // it dampens ALL other family stress.
    //
    // Call once per block, passing current macro values and receiving
    // the bleed-adjusted values back.
    //--------------------------------------------------------------------------
    struct PentagonMacroState {
        float jam       = 0.5f;  // Ohm M1
        float meddling  = 0.0f;  // Ohm M2
        float commune   = 0.0f;  // Ohm M3
        float meadow    = 0.3f;  // Ohm M4
        float pluck     = 0.5f;  // Orphica M1
        float fracture  = 0.0f;  // Orphica M2
        float surface   = 0.5f;  // Orphica M3
        float divine    = 0.2f;  // Orphica M4
        float breath    = 0.5f;  // Obbligato M1
        float bond      = 0.0f;  // Obbligato M2
        float mischief  = 0.0f;  // Obbligato M3
        float wind      = 0.3f;  // Obbligato M4
        float embouchure= 0.5f;  // Ottoni M1
        float grow      = 0.0f;  // Ottoni M2
        float foreign   = 0.0f;  // Ottoni M3
        float lake      = 0.3f;  // Ottoni M4
        float fuego     = 0.5f;  // Ole M1
        float drama     = 0.0f;  // Ole M2
        float sides     = 0.0f;  // Ole M3
        float isla      = 0.3f;  // Ole M4
    };

    // Apply macro bleed. Modifies state in-place.
    // bleedAmount: 0.0 (no family interaction) to 1.0 (full 15% bleed)
    static void processBleed (PentagonMacroState& s, float bleedAmount = 1.0f)
    {
        float k = bleedAmount * 0.15f; // max ±15% nudge

        // DRAMA ↑ → MEDDLING ↑ (aunts fighting energizes in-laws)
        s.meddling += s.drama * k;

        // DRAMA ↑ → BOND destabilized (aunts' fighting stresses the brothers)
        s.bond += s.drama * k * 0.5f * std::sin (s.bond * 6.28f); // oscillation

        // MEDDLING ↑ → SURFACE shifts (in-law interference makes wife react)
        s.surface += (s.meddling - 0.5f) * k;

        // GROW ↑ (teen zone) → SIDES rotation (teen energy excites alliance)
        if (s.grow > 0.6f)
            s.sides += (s.grow - 0.6f) * k * 2.5f;

        // GROW ↑ (toddler zone) → DRAMA ↓ (baby softens the aunts)
        if (s.grow < 0.3f)
            s.drama -= (0.3f - s.grow) * k * 2.0f;

        // BOND destabilized → DRAMA ↑ (sons fighting stresses aunts)
        s.drama += std::abs (s.bond - 0.5f) * k;

        // SURFACE extreme → GROW shifts (mother's emotion affects cousins)
        s.grow += (std::abs (s.surface - 0.5f)) * k * 0.5f;

        // --- THE COMMUNE DAMPER ---
        // COMMUNE dampens ALL family stress. Dad's peace heals the pentagon.
        float communeDamp = s.commune * k * 2.0f;
        s.drama    -= communeDamp;
        s.meddling -= communeDamp;
        s.bond      = s.bond * (1.0f - s.commune * k) + 0.5f * s.commune * k; // stabilize toward 0.5
        s.surface   = s.surface * (1.0f - s.commune * k * 0.5f) + 0.5f * s.commune * k * 0.5f;

        // Clamp all values to [0, 1]
        auto clamp01 = [](float& v) { v = std::max (0.0f, std::min (1.0f, v)); };
        clamp01 (s.meddling); clamp01 (s.commune);  clamp01 (s.surface);
        clamp01 (s.drama);    clamp01 (s.bond);     clamp01 (s.grow);
        clamp01 (s.sides);    clamp01 (s.fracture);
    }

    //--------------------------------------------------------------------------
    // Utility: check if an engine ID belongs to the Pentagon family.
    //--------------------------------------------------------------------------
    static bool isPentagonEngine (const std::string& engineId)
    {
        return engineId == kOhm || engineId == kOrphica
            || engineId == kObbligato || engineId == kOttoni
            || engineId == kOle;
    }

    //--------------------------------------------------------------------------
    // Count how many pentagon engines are in the active slots.
    //--------------------------------------------------------------------------
    static int countPentagonEngines (
        const std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots>& engines)
    {
        int count = 0;
        for (auto* e : engines)
            if (e && isPentagonEngine (e->getEngineId().toStdString()))
                ++count;
        return count;
    }
};

} // namespace xomnibus
