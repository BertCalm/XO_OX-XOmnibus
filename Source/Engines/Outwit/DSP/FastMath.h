#pragma once

//==============================================================================
// FastMath.h (XOutwit local) — forwards to the shared XOlokun FastMath and
// re-exports all symbols into the xoutwit namespace so adapter code can call
// xoutwit::fastTanh, xoutwit::midiToFreq, etc.
//
// The adapter calls:  xoutwit::fastTanh(x)
//==============================================================================

#include "../../../DSP/FastMath.h"

namespace xoutwit {

using xolokun::flushDenormal;
using xolokun::fastExp;
using xolokun::fastTanh;
using xolokun::fastSin;
using xolokun::fastCos;
using xolokun::fastTan;
using xolokun::fastLog2;
using xolokun::fastPow2;
using xolokun::midiToFreq;
using xolokun::midiToFreqTune;
using xolokun::dbToGain;
using xolokun::gainToDb;
using xolokun::clamp;
using xolokun::lerp;
using xolokun::smoothstep;
using xolokun::smoothCoeffFromTime;
using xolokun::softClip;

} // namespace xoutwit
