#pragma once

//==============================================================================
// FastMath.h — xoverlap namespace forwarding header
//
// Re-exports the shared XOmnibus FastMath functions under the xoverlap
// namespace so that XOverlapAdapter.h can call xoverlap::flushDenormal,
// xoverlap::fastSin, etc. without modification.
//==============================================================================

#include "../../../DSP/FastMath.h"

namespace xoverlap {

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

} // namespace xoverlap
