#pragma once
#include <cmath>
#include <algorithm>
#include "../FastMath.h"

namespace xomnibus {

//==============================================================================
// Compressor — Dynamics compressor with sidechain input.
//
// Features:
//   - Threshold, ratio, attack, release, makeup gain
//   - Soft knee for smooth gain transitions
//   - Peak detection with attack/release envelope follower
//   - External sidechain input for cross-engine ducking (key XOmnibus feature)
//   - Gain reduction metering for UI display
//   - Denormal protection on envelope state
//
// Usage:
//   Compressor comp;
//   comp.prepare(44100.0);
//   comp.setThreshold(-18.0f);
//   comp.setRatio(4.0f);
//   comp.setAttack(10.0f);
//   comp.setRelease(100.0f);
//   comp.processBlock(left, right, numSamples);
//   float gr = comp.getGainReduction(); // for metering
//
//   // With external sidechain (cross-engine ducking):
//   comp.processBlockWithSidechain(left, right, scLeft, scRight, numSamples);
//==============================================================================
class Compressor
{
public:
    Compressor() = default;

    //--------------------------------------------------------------------------
    /// Prepare the compressor for playback.
    /// @param sampleRate  Current sample rate in Hz.
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        envelope = 0.0f;
        gainReductionDb = 0.0f;
        updateCoefficients();
    }

    //--------------------------------------------------------------------------
    /// Set the threshold in dB. Clamped to [-60, 0] dB.
    void setThreshold (float dbThreshold)
    {
        threshold = clamp (dbThreshold, -60.0f, 0.0f);
    }

    /// Set the compression ratio. Clamped to [1.0, 20.0].
    /// 1.0 = no compression, 20.0 = near-limiting.
    void setRatio (float r)
    {
        ratio = clamp (r, 1.0f, 20.0f);
    }

    /// Set the attack time in milliseconds. Clamped to [0.1, 100]ms.
    void setAttack (float ms)
    {
        attackMs = clamp (ms, 0.1f, 100.0f);
        updateCoefficients();
    }

    /// Set the release time in milliseconds. Clamped to [10, 1000]ms.
    void setRelease (float ms)
    {
        releaseMs = clamp (ms, 10.0f, 1000.0f);
        updateCoefficients();
    }

    /// Set makeup gain in dB. Clamped to [0, 30] dB.
    void setMakeupGain (float db)
    {
        makeupGainDb = clamp (db, 0.0f, 30.0f);
        makeupGainLinear = dbToGain (makeupGainDb);
    }

    /// Set soft knee width in dB. 0 = hard knee, up to 12 dB.
    void setKnee (float db)
    {
        kneeDb = clamp (db, 0.0f, 12.0f);
    }

    //--------------------------------------------------------------------------
    /// Process a stereo block using the input signal for detection.
    /// @param left   Left channel buffer (modified in-place).
    /// @param right  Right channel buffer (modified in-place).
    /// @param numSamples  Number of samples to process.
    void processBlock (float* left, float* right, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            // Peak detection from input
            float detect = std::max (std::abs (left[i]), std::abs (right[i]));

            // Compute and apply gain
            float gain = computeGain (detect);
            left[i]  *= gain;
            right[i] *= gain;
        }
    }

    //--------------------------------------------------------------------------
    /// Process a stereo block using an external sidechain signal for detection.
    /// This enables cross-engine ducking — a key XOmnibus coupling feature.
    /// @param left    Left channel buffer (modified in-place).
    /// @param right   Right channel buffer (modified in-place).
    /// @param scLeft  Sidechain left channel buffer (read-only).
    /// @param scRight Sidechain right channel buffer (read-only).
    /// @param numSamples  Number of samples to process.
    void processBlockWithSidechain (float* left, float* right,
                                    const float* scLeft, const float* scRight,
                                    int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            // Peak detection from sidechain input
            float detect = std::max (std::abs (scLeft[i]), std::abs (scRight[i]));

            // Compute and apply gain to main signal
            float gain = computeGain (detect);
            left[i]  *= gain;
            right[i] *= gain;
        }
    }

    //--------------------------------------------------------------------------
    /// Get the current gain reduction in dB (positive value = reduction).
    /// Use this for UI metering display.
    float getGainReduction() const
    {
        return gainReductionDb;
    }

    //--------------------------------------------------------------------------
    /// Reset all compressor state without reallocation.
    void reset()
    {
        envelope = 0.0f;
        gainReductionDb = 0.0f;
    }

private:
    //--------------------------------------------------------------------------
    /// Recompute attack/release envelope coefficients from time constants.
    void updateCoefficients()
    {
        if (sr <= 0.0) return;

        // Envelope follower coefficients: coeff = exp(-1 / (time_seconds * sampleRate))
        // Using 1 - exp(-1/(t*sr)) as the "approach" coefficient
        float srF = static_cast<float> (sr);
        attackCoeff  = 1.0f - std::exp (-1.0f / (attackMs * 0.001f * srF));
        releaseCoeff = 1.0f - std::exp (-1.0f / (releaseMs * 0.001f * srF));
    }

    //--------------------------------------------------------------------------
    /// Compute gain reduction for a single sample and return the linear gain.
    /// @param detectLevel  Absolute peak level of the detection signal.
    /// @return Linear gain to apply to the audio signal.
    float computeGain (float detectLevel)
    {
        // Convert detection level to dB
        float detectDb = (detectLevel > 1e-10f)
            ? 20.0f * std::log10 (detectLevel)
            : -200.0f;

        // Compute gain reduction with soft knee
        float reductionDb = computeReduction (detectDb);

        // Smooth the gain reduction with attack/release envelope
        // Attack when gain reduction is increasing, release when decreasing
        if (reductionDb > envelope)
            envelope += attackCoeff * (reductionDb - envelope);
        else
            envelope += releaseCoeff * (reductionDb - envelope);

        envelope = flushDenormal (envelope);

        // Store for metering (positive value = amount of reduction)
        gainReductionDb = envelope;

        // Convert to linear gain: negative dB reduction + positive makeup
        float totalGainDb = -envelope + makeupGainDb;
        return dbToGain (totalGainDb);
    }

    //--------------------------------------------------------------------------
    /// Compute the target gain reduction in dB for a given input level.
    /// Implements soft knee around the threshold.
    /// @param inputDb  Input level in dB.
    /// @return Target gain reduction in dB (0 = no reduction).
    float computeReduction (float inputDb) const
    {
        float overDb = inputDb - threshold;

        if (kneeDb <= 0.001f)
        {
            // Hard knee
            if (overDb <= 0.0f)
                return 0.0f;
            return overDb * (1.0f - 1.0f / ratio);
        }

        // Soft knee: quadratic interpolation in the knee region
        float halfKnee = kneeDb * 0.5f;

        if (overDb <= -halfKnee)
        {
            // Below knee — no compression
            return 0.0f;
        }
        else if (overDb >= halfKnee)
        {
            // Above knee — full compression
            return overDb * (1.0f - 1.0f / ratio);
        }
        else
        {
            // In the knee region — smooth quadratic transition
            // The gain curve smoothly transitions from 1:1 to ratio:1
            float kneeInput = overDb + halfKnee;
            float kneeRatio = kneeInput / kneeDb;
            float slope = (1.0f - 1.0f / ratio) * kneeRatio;
            return slope * kneeInput * 0.5f;
        }
    }

    //--------------------------------------------------------------------------
    double sr = 44100.0;

    // Parameters
    float threshold = -18.0f;    // dB
    float ratio = 4.0f;
    float attackMs = 10.0f;      // ms
    float releaseMs = 100.0f;    // ms
    float makeupGainDb = 0.0f;   // dB
    float makeupGainLinear = 1.0f;
    float kneeDb = 6.0f;         // dB (soft knee width)

    // Envelope follower state
    float envelope = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // Metering
    float gainReductionDb = 0.0f;
};

} // namespace xomnibus
