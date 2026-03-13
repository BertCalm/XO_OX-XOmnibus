#pragma once
#include <cmath>

namespace xocelot {

// WaveFolder — tanh-based soft wave folder.
// Available to Floor (post-model saturation) and Emergent (formant saturation).
// Canopy keeps its own reflection-style folder (Buchla character).

struct WaveFolder
{
    // drive: 1.0 = clean, higher = more folds (useful range 1–5)
    static float process(float x, float drive)
    {
        return std::tanh(x * drive);
    }
};

} // namespace xocelot
