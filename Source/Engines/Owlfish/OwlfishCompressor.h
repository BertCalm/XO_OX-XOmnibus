// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "OwlfishFastMath.h"
#include <cmath>

namespace xowlfish {

//==============================================================================
// OwlfishCompressor -- Extreme feed-forward compressor (OWL OPTICS).
//
// The owlfish's enormous telescopic eyes amplify the faintest photons.
// This compressor provides extreme dynamics control with RMS envelope
// detection, configurable ballistics, and automatic makeup gain.
//
// Ratio range: 1:1 to infinity:1 (limiter).
// Threshold range: -60 dB to 0 dB.
//
// All DSP inline. No allocations. Denormal-safe.
//==============================================================================

class OwlfishCompressor
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    void reset()
    {
        envState = 0.0f;
    }

    /// Set parameters from normalized 0-1 values (call once per block).
    /// ratio01:     0 -> 1:1 (no compression), 1 -> inf:1 (limiter)
    /// threshold01: 0 -> -60 dB, 1 -> 0 dB
    /// attack01:    0 -> 0.1 ms, 1 -> 100 ms
    /// release01:   0 -> 10 ms, 1 -> 1000 ms
    void setParams (float ratio01, float threshold01, float attack01, float release01)
    {
        // Map ratio: 0-1 -> 1.0 to 100.0
        // At param value 1.0, treat as limiter (infinite ratio)
        isLimiter = (ratio01 >= 0.999f);
        ratioActual = 1.0f + ratio01 * 99.0f;

        // Map threshold: 0-1 -> -60 dB to 0 dB
        thresholdDb = -60.0f + threshold01 * 60.0f;

        // Map attack: 0-1 -> 0.1 ms to 100 ms -> coefficient
        float attackMs = 0.1f + attack01 * 99.9f;
        float attackSec = attackMs * 0.001f;
        attackCoeff = smoothCoeffFromTime (attackSec, static_cast<float> (sr));

        // Map release: 0-1 -> 10 ms to 1000 ms -> coefficient
        float releaseMs = 10.0f + release01 * 990.0f;
        float releaseSec = releaseMs * 0.001f;
        releaseCoeff = smoothCoeffFromTime (releaseSec, static_cast<float> (sr));

        // Auto makeup gain: compensate for average gain reduction
        // makeupDB = thresholdDB * (1 - 1/ratio) * 0.5
        float invRatio = isLimiter ? 0.0f : (1.0f / ratioActual);
        float makeupDb = thresholdDb * (1.0f - invRatio) * 0.5f;
        makeupGain = dbToGain (-makeupDb);  // negative because threshold is negative
    }

    /// Process one sample through the compressor.
    float processSample (float input)
    {
        // -- RMS envelope detection --
        float inputSquared = input * input;
        float coeff = (inputSquared > envState) ? attackCoeff : releaseCoeff;
        envState += coeff * (inputSquared - envState);
        envState = flushDenormal (envState);

        // RMS level in dB
        float rms = std::sqrt (envState);
        float inputDb = gainToDb (rms);

        // -- Gain computation (feed-forward) --
        float gainReductionDb = 0.0f;
        if (inputDb > thresholdDb)
        {
            float overDb = inputDb - thresholdDb;
            if (isLimiter)
                gainReductionDb = overDb;  // inf:1 ratio
            else
                gainReductionDb = overDb * (1.0f - 1.0f / ratioActual);
        }

        // Apply gain reduction + makeup
        float gainDb = -gainReductionDb;
        float gainLinear = dbToGain (gainDb) * makeupGain;

        return input * gainLinear;
    }

private:
    double sr = 44100.0;
    float envState = 0.0f;       // envelope follower state (squared amplitude)
    float ratioActual = 1.0f;
    float thresholdDb = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float makeupGain = 1.0f;     // linear makeup gain
    bool  isLimiter = false;
};

} // namespace xowlfish
