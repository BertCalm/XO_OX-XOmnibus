#pragma once

//==============================================================================
// FastMath.h — xoverlap namespace forwarding header
//
// Re-exports the shared XOlokun FastMath functions under the xoverlap
// namespace so that XOverlapAdapter.h can call xoverlap::flushDenormal,
// xoverlap::fastSin, etc. without modification.
//==============================================================================

#include "../../../DSP/FastMath.h"

namespace xoverlap {

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

} // namespace xoverlap
