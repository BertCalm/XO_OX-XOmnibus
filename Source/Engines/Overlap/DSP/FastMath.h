// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// FastMath.h — xoverlap namespace forwarding header
//
// Re-exports the shared XOceanus FastMath functions under the xoverlap
// namespace so that XOverlapAdapter.h can call xoverlap::flushDenormal,
// xoverlap::fastSin, etc. without modification.
//==============================================================================

#include "../../../DSP/FastMath.h"

namespace xoverlap
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

} // namespace xoverlap
