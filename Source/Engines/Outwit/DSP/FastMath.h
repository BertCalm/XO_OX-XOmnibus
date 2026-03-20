#pragma once

//==============================================================================
// FastMath.h (XOutwit local) — forwards to the shared XOmnibus FastMath and
// re-exports all symbols into the xoutwit namespace so adapter code can call
// xoutwit::fastTanh, xoutwit::midiToFreq, etc.
//
// The adapter calls:  xoutwit::fastTanh(x)
//==============================================================================

#include "DSP/FastMath.h"

namespace xoutwit {

using xomnibus::flushDenormal;
using xomnibus::fastExp;
using xomnibus::fastTanh;
using xomnibus::fastSin;
using xomnibus::fastCos;
using xomnibus::fastTan;
using xomnibus::fastLog2;
using xomnibus::fastPow2;
using xomnibus::midiToFreq;
using xomnibus::midiToFreqTune;
using xomnibus::dbToGain;
using xomnibus::gainToDb;
using xomnibus::clamp;
using xomnibus::lerp;
using xomnibus::smoothstep;
using xomnibus::smoothCoeffFromTime;
using xomnibus::softClip;

} // namespace xoutwit
