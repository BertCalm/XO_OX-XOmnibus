#pragma once

//==============================================================================
// FastMath.h — xoverlap namespace forwarding header
//
// Re-exports the shared XOceanus FastMath functions under the xoverlap
// namespace so that XOverlapAdapter.h can call xoverlap::flushDenormal,
// xoverlap::fastSin, etc. without modification.
//==============================================================================

#include "../../../DSP/FastMath.h"

namespace xoverlap {

using xoceanus::flushDenormal;
using xoceanus::fastExp;
using xoceanus::fastTanh;
using xoceanus::fastSin;
using xoceanus::fastCos;
using xoceanus::fastTan;
using xoceanus::fastLog2;
using xoceanus::fastPow2;
using xoceanus::midiToFreq;
using xoceanus::midiToFreqTune;
using xoceanus::dbToGain;
using xoceanus::gainToDb;
using xoceanus::clamp;
using xoceanus::lerp;
using xoceanus::smoothstep;
using xoceanus::smoothCoeffFromTime;
using xoceanus::softClip;

} // namespace xoverlap
