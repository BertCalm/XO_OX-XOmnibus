#pragma once
#include "OwlfishFastMath.h"
#include <cmath>
#include <cstdint>

namespace xowlfish {

//==============================================================================
// ArmorBuffer -- Velocity-triggered sacrificial signal capture (SACRIFICIAL ARMOR).
//
// The barreleye owlfish sacrifices its transparent cranial shield on impact,
// releasing a burst of captured sonic material. On velocity trigger:
//   1. Last 2048 samples of input history are frozen into a capture buffer
//   2. 8 grains spawn at random positions with pitch scatter
//   3. Grains feed through a feedback delay line
//   4. The main signal is ducked to make room for the armor burst
//   5. Exponential decay fades the armor cloud to silence
//
// All buffers fixed-size. No allocations. Denormal-safe.
//==============================================================================

class ArmorBuffer
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    void reset()
    {
        for (int i = 0; i < kHistorySize; ++i)
        {
            inputHistory[i] = 0.0f;
            captureBuffer[i] = 0.0f;
        }
        historyPos = 0;

        for (int i = 0; i < kMaxDelay; ++i)
            delayLine[i] = 0.0f;
        delayPos = 0;

        for (int i = 0; i < kNumGrains; ++i)
            armorGrains[i].active = false;

        armed = false;
        decayEnv = 0.0f;
        rngState = 54321;
    }

    /// Set parameters from normalized 0-1 values (call once per block).
    /// decay01:   0 -> 50 ms, 1 -> 2000 ms time constant
    /// scatter01: 0 -> 0 semitones, 1 -> 24 semitones pitch scatter
    /// delay01:   0 -> 50 ms, 1 -> 500 ms delay time
    void setParams (float decay01, float scatter01, float delay01)
    {
        // Decay time constant: 50ms to 2000ms
        float decayMs = 50.0f + decay01 * 1950.0f;
        float decaySec = decayMs * 0.001f;
        // Exponential decay coefficient per sample
        decayCoeff = 1.0f - smoothCoeffFromTime (decaySec, static_cast<float> (sr));

        // Pitch scatter range: 0 to 24 semitones
        scatterRange = scatter01 * 24.0f;

        // Delay time: 50ms to 500ms in samples
        float delayMs = 50.0f + delay01 * 450.0f;
        delaySamples = static_cast<int> (delayMs * 0.001f * static_cast<float> (sr));
        if (delaySamples >= kMaxDelay) delaySamples = kMaxDelay - 1;
        if (delaySamples < 1) delaySamples = 1;
    }

    /// Feed one sample into the continuous input history ring buffer.
    void feedInput (float sample)
    {
        inputHistory[historyPos] = sample;
        historyPos = (historyPos + 1) & (kHistorySize - 1);
    }

    /// Trigger the armor sacrifice. velocity and threshold are both 0-1.
    /// Only triggers if not already active.
    void trigger (float velocity, float threshold)
    {
        if (armed) return;               // already active
        if (velocity <= threshold) return; // below threshold

        // -- Copy input history into capture buffer --
        for (int i = 0; i < kHistorySize; ++i)
        {
            int srcIdx = (historyPos + i) & (kHistorySize - 1);
            captureBuffer[i] = inputHistory[srcIdx];
        }

        // -- Reset decay envelope --
        decayEnv = 1.0f;

        // -- Spawn 8 grains at random positions --
        for (int i = 0; i < kNumGrains; ++i)
        {
            ArmorGrain& g = armorGrains[i];
            g.position = nextRandom() * static_cast<float> (kHistorySize);
            // Pitch scatter: random offset in semitones
            float semitones = nextRandom() * scatterRange;
            g.phaseInc = std::pow (2.0f, semitones / 12.0f);
            g.active = true;
        }

        // -- Clear delay line for fresh burst --
        for (int i = 0; i < kMaxDelay; ++i)
            delayLine[i] = 0.0f;
        delayPos = 0;

        armed = true;
    }

    /// Process one sample of armor grain output.
    float processSample()
    {
        if (!armed) return 0.0f;

        // -- Sum grain outputs --
        float grainSum = 0.0f;
        for (int i = 0; i < kNumGrains; ++i)
        {
            if (!armorGrains[i].active) continue;

            ArmorGrain& g = armorGrains[i];

            // Read from capture buffer with linear interpolation
            int posInt = static_cast<int> (g.position);
            float frac = g.position - static_cast<float> (posInt);
            int idx0 = posInt & (kHistorySize - 1);
            int idx1 = (posInt + 1) & (kHistorySize - 1);
            float sample = captureBuffer[idx0] + frac * (captureBuffer[idx1] - captureBuffer[idx0]);

            grainSum += sample;

            // Advance grain position (wrapping in capture buffer)
            g.position += g.phaseInc;
            if (g.position >= static_cast<float> (kHistorySize))
                g.position -= static_cast<float> (kHistorySize);
        }

        grainSum *= (1.0f / static_cast<float> (kNumGrains));

        // -- Delay line with gentle feedback --
        int readPos = (delayPos - delaySamples + kMaxDelay) % kMaxDelay;
        float delayed = delayLine[readPos];
        delayed = flushDenormal (delayed);

        float delayInput = grainSum + delayed * 0.3f;  // feedback = 0.3
        delayLine[delayPos] = delayInput;
        delayPos = (delayPos + 1) % kMaxDelay;

        float output = (grainSum + delayed) * 0.5f;

        // -- Apply decay envelope --
        output *= decayEnv;
        decayEnv *= decayCoeff;
        decayEnv = flushDenormal (decayEnv);

        // -- Deactivate when envelope is negligible --
        if (decayEnv < 0.001f)
        {
            armed = false;
            decayEnv = 0.0f;
            for (int i = 0; i < kNumGrains; ++i)
                armorGrains[i].active = false;
        }

        return flushDenormal (output);
    }

    /// Get the current duck amount (0 = no ducking, peaks at 1).
    /// Caller uses: main *= (1.0 - duckAmount * armorDuck)
    float getDuckAmount() const
    {
        return armed ? decayEnv : 0.0f;
    }

    /// Is the armor burst currently active?
    bool isActive() const
    {
        return armed;
    }

private:
    static constexpr int kHistorySize = 2048;
    static constexpr int kMaxDelay    = 22050;
    static constexpr int kNumGrains   = 8;

    float inputHistory[kHistorySize] = {};
    int   historyPos = 0;
    float captureBuffer[kHistorySize] = {};
    float delayLine[kMaxDelay] = {};
    int   delayPos = 0;
    int   delaySamples = 2205;

    struct ArmorGrain
    {
        float position = 0.0f;
        float phaseInc = 1.0f;
        bool  active   = false;
    };
    ArmorGrain armorGrains[kNumGrains];

    bool  armed = false;       // true when armor has been triggered
    float decayEnv = 0.0f;     // exponential decay envelope
    float decayCoeff = 0.999f; // per-sample decay multiplier
    float scatterRange = 0.0f; // pitch scatter in semitones
    double sr = 44100.0;

    // Simple LCG random (audio-thread safe)
    uint32_t rngState = 54321;

    float nextRandom()
    {
        rngState = rngState * 1664525u + 1013904223u;
        return static_cast<float> (rngState) / static_cast<float> (0xFFFFFFFFu);
    }
};

} // namespace xowlfish
