#pragma once

namespace xoceanus {

//------------------------------------------------------------------------------
// Sample classification — shared between XOutshine and RebirthProfiles.
// Extracted to break circular include dependency.
//------------------------------------------------------------------------------

enum class SampleCategory
{
    // Drum / percussive
    Kick, Snare, HiHatClosed, HiHatOpen, Clap, Tom, Percussion, FX,
    // Structural
    Loop,
    // Melodic
    Bass, Pad, Lead, Keys, Pluck, String, Woodwind, Brass, Vocal,
    // Fallback
    Unknown
};

inline bool isDrumCategory (SampleCategory c)
{
    return c == SampleCategory::Kick || c == SampleCategory::Snare
        || c == SampleCategory::HiHatClosed || c == SampleCategory::HiHatOpen
        || c == SampleCategory::Clap || c == SampleCategory::Tom
        || c == SampleCategory::Percussion || c == SampleCategory::FX;
}

inline int gmNoteForCategory (SampleCategory c)
{
    switch (c) {
        case SampleCategory::Kick:         return 36;
        case SampleCategory::Snare:        return 38;
        case SampleCategory::HiHatClosed:  return 42;
        case SampleCategory::HiHatOpen:    return 46;
        case SampleCategory::Clap:         return 39;
        case SampleCategory::Tom:          return 41;
        case SampleCategory::Percussion:   return 43;
        case SampleCategory::FX:           return 49;
        case SampleCategory::Loop:         return 60;
        default:                           return 60;
    }
}

inline const char* categoryName (SampleCategory c)
{
    switch (c) {
        case SampleCategory::Kick:         return "Kick";
        case SampleCategory::Snare:        return "Snare";
        case SampleCategory::HiHatClosed:  return "HiHat Closed";
        case SampleCategory::HiHatOpen:    return "HiHat Open";
        case SampleCategory::Clap:         return "Clap";
        case SampleCategory::Tom:          return "Tom";
        case SampleCategory::Percussion:   return "Percussion";
        case SampleCategory::FX:           return "FX";
        case SampleCategory::Bass:         return "Bass";
        case SampleCategory::Pad:          return "Pad";
        case SampleCategory::Lead:         return "Lead";
        case SampleCategory::Keys:         return "Keys";
        case SampleCategory::Pluck:        return "Pluck";
        case SampleCategory::String:       return "String";
        case SampleCategory::Loop:         return "Loop";
        case SampleCategory::Woodwind:     return "Woodwind";
        case SampleCategory::Brass:        return "Brass";
        case SampleCategory::Vocal:        return "Vocal";
        default:                           return "Unknown";
    }
}

} // namespace xoceanus
