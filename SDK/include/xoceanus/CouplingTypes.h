#pragma once
// xoceanus-engine-sdk — CouplingTypes.h
// Standalone header — no JUCE dependency.
//
// Coupling is the signature feature of XOceanus: two engines running in parallel,
// each modulating the other in real time. A CouplingType describes the nature of
// that relationship — what signal flows from the source engine and how the
// destination engine interprets it.
//
// The MegaCouplingMatrix evaluates active routes once per block and dispatches
// to each engine's applyCouplingInput(). Engine developers declare which types
// they respond to; all others are silently ignored.

namespace xoceanus {

/// The channel through which one engine modulates another.
///
/// Each value describes the signal flow: source produces it, destination
/// consumes it. Engines that don't handle a given type ignore it gracefully.
enum class CouplingType {
    // --- Amplitude-driven --------------------------------------------------
    AmpToFilter,       ///< Source amplitude opens/closes destination filter cutoff
    AmpToPitch,        ///< Source amplitude bends destination pitch
    AmpToChoke,        ///< Source amplitude ducks/gates destination (sidechain ducking)

    // --- LFO / Envelope driven --------------------------------------------
    LFOToPitch,        ///< Source LFO modulates destination pitch (vibrato injection)
    EnvToMorph,        ///< Source envelope sweeps destination wavetable/morph position
    EnvToDecay,        ///< Source envelope stretches/shrinks destination decay time

    // --- Audio-rate ---------------------------------------------------------
    AudioToFM,         ///< Source audio becomes an FM carrier for destination oscillator
    AudioToRing,       ///< Source audio × destination audio (ring modulation)
    AudioToWavetable,  ///< Source audio replaces destination wavetable source material
    AudioToBuffer,     ///< Source audio streams into destination ring buffer (continuous)

    // --- Filter / Pitch topology --------------------------------------------
    FilterToFilter,    ///< Source filter output feeds into destination filter input
    PitchToPitch,      ///< Source pitch transposes destination (parallel harmony)

    // --- Pattern -----------------------------------------------------------
    RhythmToBlend,     ///< Source rhythm pattern modulates destination blend parameter

    // --- Bidirectional -----------------------------------------------------
    KnotTopology,      ///< Both engines mutually modulate each other's pitch/filter/morph
                       ///< via a shared knot state variable. Active in the host —
                       ///< MegaCouplingMatrix routes knot traffic through
                       ///< processKnotRoute() and intentionally excludes
                       ///< KnotTopology from acyclic-graph cycle detection
                       ///< because the coupling is bidirectional by design.

    // --- Love triangle ---------------------------------------------------
    TriangularCoupling ///< Source engine's intimacy/passion/connection bleed into
                       ///< destination engine's love components (Oxytocin signature coupling)
};

/// Total number of coupling types — useful for building iteration tables.
constexpr int kNumCouplingTypes = 15;

/// Human-readable display name for each CouplingType.
/// Used in UI labels, preset debugging, and validation output.
inline const char* couplingTypeName (CouplingType t)
{
    switch (t)
    {
        case CouplingType::AmpToFilter:      return "Amp→Filter";
        case CouplingType::AmpToPitch:       return "Amp→Pitch";
        case CouplingType::AmpToChoke:       return "Amp→Choke";
        case CouplingType::LFOToPitch:       return "LFO→Pitch";
        case CouplingType::EnvToMorph:       return "Env→Morph";
        case CouplingType::EnvToDecay:       return "Env→Decay";
        case CouplingType::AudioToFM:        return "Audio→FM";
        case CouplingType::AudioToRing:      return "Audio×Ring";
        case CouplingType::AudioToWavetable: return "Audio→Wavetable";
        case CouplingType::AudioToBuffer:    return "Audio→Buffer";
        case CouplingType::FilterToFilter:   return "Filter→Filter";
        case CouplingType::PitchToPitch:     return "Pitch→Pitch";
        case CouplingType::RhythmToBlend:    return "Rhythm→Blend";
        case CouplingType::KnotTopology:     return "Knot↔Topology";
        case CouplingType::TriangularCoupling: return "Love△Coupling";
    }
    return "Unknown";
}

} // namespace xoceanus
