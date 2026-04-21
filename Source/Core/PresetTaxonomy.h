#pragma once

#include <juce_core/juce_core.h>
#include <array>

namespace xoceanus {

// 10-term instrument category taxonomy (required in schema v2).
// Functional categorization: what role does this preset play in a track.
inline constexpr std::array<const char*, 10> kPresetCategories = {
    "keys", "pads", "leads", "bass", "drums",
    "perc", "textures", "fx", "sequence", "vocal"
};

// 8-term timbre taxonomy (optional/nullable in schema v2).
// Sonic family: what acoustic instrument does this preset evoke.
// Most XOceanus presets are electronic → timbre remains null.
inline constexpr std::array<const char*, 8> kPresetTimbres = {
    "strings", "brass", "wind", "choir",
    "organ", "plucked", "metallic", "world"
};

// Returns true if `category` exactly matches one of kPresetCategories.
// Case-sensitive. Empty / null strings return false.
inline bool isValidPresetCategory (const juce::String& category) noexcept
{
    for (auto* c : kPresetCategories)
        if (category == c) return true;
    return false;
}

// Returns true if `timbre` exactly matches one of kPresetTimbres.
// Empty string is handled upstream (timbre is optional via std::optional);
// this function only validates non-empty values supplied by the caller.
inline bool isValidPresetTimbre (const juce::String& timbre) noexcept
{
    for (auto* t : kPresetTimbres)
        if (timbre == t) return true;
    return false;
}

} // namespace xoceanus
