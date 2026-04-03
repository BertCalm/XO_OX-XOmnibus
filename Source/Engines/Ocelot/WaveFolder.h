// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include "../../DSP/FastMath.h"

namespace xocelot
{

// WaveFolder — tanh-based soft wave folder.
// Available to Floor (post-model saturation) and Emergent (formant saturation).
// Canopy keeps its own reflection-style folder (Buchla character).

struct WaveFolder
{
    // drive: 1.0 = clean, higher = more folds (useful range 1–5)
    static float process(float x, float drive) { return xoceanus::fastTanh(x * drive); }
};

} // namespace xocelot
