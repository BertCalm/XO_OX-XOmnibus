// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// FastMath.h (XOutwit local) — forwards to the shared XOceanus FastMath and
// re-exports all symbols into the xoutwit namespace so adapter code can call
// xoutwit::fastTanh, xoutwit::midiToFreq, etc.
//
// The adapter calls:  xoutwit::fastTanh(x)
//==============================================================================

#include "../../../DSP/FastMath.h"

namespace xoutwit
{

using xoceanus::clamp;
using xoceanus::dbToGain;
using xoceanus::fastCos;
using xoceanus::fastExp;
using xoceanus::fastLog2;
using xoceanus::fastPow2;
using xoceanus::fastSin;
using xoceanus::fastTan;
using xoceanus::fastTanh;
using xoceanus::flushDenormal;
using xoceanus::gainToDb;
using xoceanus::lerp;
using xoceanus::midiToFreq;
using xoceanus::midiToFreqTune;
using xoceanus::smoothCoeffFromTime;
using xoceanus::smoothstep;
using xoceanus::softClip;

} // namespace xoutwit
