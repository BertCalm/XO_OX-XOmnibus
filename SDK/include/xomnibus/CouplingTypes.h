#pragma once
// xomnibus-engine-sdk — CouplingTypes.h
// Standalone header — no JUCE dependency.
// Defines the coupling modulation types available for cross-engine interaction.

namespace xomnibus {

/// Cross-engine modulation types.
/// Each type describes a source→destination relationship between two engines.
enum class CouplingType {
    AmpToFilter,       ///< Engine A amplitude → Engine B filter cutoff
    AmpToPitch,        ///< Engine A amplitude → Engine B pitch
    LFOToPitch,        ///< Engine A LFO → Engine B pitch
    EnvToMorph,        ///< Engine A envelope → Engine B wavetable/morph position
    AudioToFM,         ///< Engine A audio → Engine B FM input
    AudioToRing,       ///< Engine A audio × Engine B audio (ring modulation)
    FilterToFilter,    ///< Engine A filter output → Engine B filter input
    AmpToChoke,        ///< Engine A amplitude chokes Engine B (ducking)
    RhythmToBlend,     ///< Engine A rhythm pattern → Engine B blend parameter
    EnvToDecay,        ///< Engine A envelope → Engine B decay time
    PitchToPitch,      ///< Engine A pitch → Engine B pitch (harmony)
    AudioToWavetable,  ///< Engine A audio → Engine B wavetable source
    AudioToBuffer,     ///< Engine A audio → Engine B ring buffer (continuous stereo stream)
    KnotTopology       ///< Bidirectional topological coupling — both engines mutually modulate
                       ///< each other's pitch/filter/morph via a shared knot state variable.
                       ///< Post-V1 feature; included here for forward compatibility.
};

/// Number of coupling types — useful for iteration.
constexpr int kNumCouplingTypes = 14;

/// Get a human-readable name for a CouplingType.
inline const char* couplingTypeName (CouplingType t)
{
    switch (t)
    {
        case CouplingType::AmpToFilter:      return "Amp→Filter";
        case CouplingType::AmpToPitch:       return "Amp→Pitch";
        case CouplingType::LFOToPitch:       return "LFO→Pitch";
        case CouplingType::EnvToMorph:       return "Env→Morph";
        case CouplingType::AudioToFM:        return "Audio→FM";
        case CouplingType::AudioToRing:      return "Audio×Ring";
        case CouplingType::FilterToFilter:   return "Filter→Filter";
        case CouplingType::AmpToChoke:       return "Amp→Choke";
        case CouplingType::RhythmToBlend:    return "Rhythm→Blend";
        case CouplingType::EnvToDecay:       return "Env→Decay";
        case CouplingType::PitchToPitch:     return "Pitch→Pitch";
        case CouplingType::AudioToWavetable: return "Audio→Wavetable";
        case CouplingType::AudioToBuffer:    return "Audio→Buffer";
        case CouplingType::KnotTopology:     return "Knot↔Topology";
    }
    return "Unknown";
}

} // namespace xomnibus
