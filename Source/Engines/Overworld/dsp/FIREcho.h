#pragma once
// XOverworld FIREcho — Delay/echo with FIR-filtered feedback path.
//
// Signal path per call to process():
//   1. Read from delay line at current write head - delayLen.
//   2. Apply 8-tap FIR filter to the feedback sample (tone shaping in echo tail).
//   3. Write (dry input + feedback) into the delay line.
//   4. Return mix(dry, delayed).
//
// The FIR filter in the feedback path acts as a tone-shaping EQ on each echo
// repeat. The default coefficients (echoFir[0]=0.7, rest=0) pass a slightly
// attenuated signal, giving clean echo. Setting coefficients with a LP shape
// softens the high end on each repeat (analogous to SNES echo FIR).
//
// Parameters:
//   delay     : delay index 0-15, mapped to powers of 2 samples (8ms–~700ms @ 44.1kHz)
//   feedback  : 0-1 feedback amount (capped at 0.95 to prevent infinite buildup)
//   mix       : 0-1 wet/dry blend
//   echoFir[8]: 8 FIR coefficients for feedback tone shaping
//
// Denormal protection: one DC-block pole in the feedback path + flushDenormal.
// Ping-pong: this is a mono process() — stereo ping-pong is achieved by the
// OverworldEngine's Haas micro-delay on the right channel.
//
// Buffer: 1 second at 96kHz = 96000 samples. Statically allocated, no heap.

#include "../../../DSP/FastMath.h"
#include <cstring>

namespace xoverworld {

using namespace xoceanus;

class FIREcho {
public:
    static constexpr int kBufLen = 96000; // 1s @ 96kHz

    FIREcho() {
        std::memset(delayBuf, 0, sizeof(delayBuf));
        std::memset(firBuf,   0, sizeof(firBuf));
        // Default FIR: [0.7, 0, 0, 0, 0, 0, 0, 0] — clean attenuated echo
        firCoeffs[0] = 0.7f;
        for (int i = 1; i < 8; ++i) firCoeffs[i] = 0.0f;
    }

    void prepare(float sampleRate) {
        sr = sampleRate;
        writeHead = 0;
        firHead   = 0;
        dcState   = 0.0f;
        std::memset(delayBuf, 0, sizeof(delayBuf));
        std::memset(firBuf,   0, sizeof(firBuf));
        updateDelayLen();
    }

    // delay: 0-15 → maps to tap lengths (roughly doubling each step)
    // At 44.1kHz: 0=~8ms, 4=~50ms, 8=~200ms, 12=~700ms
    void setDelay(int d) {
        delayIndex = d < 0 ? 0 : d > 15 ? 15 : d;
        updateDelayLen();
    }

    // feedback: capped to 0.95 to prevent infinite oscillation
    void setFeedback(float f) {
        feedback = f < 0.0f ? 0.0f : f > 0.95f ? 0.95f : f;
    }

    // mix: 0=dry, 1=fully wet
    void setMix(float m) {
        mix = m < 0.0f ? 0.0f : m > 1.0f ? 1.0f : m;
    }

    // Update FIR coefficients for feedback tone shaping
    void setFIRCoefficients(const float coeffs[8]) {
        for (int i = 0; i < 8; ++i)
            firCoeffs[i] = coeffs[i];
    }

    float process(float x) {
        // 1. Read from delay line
        int readHead = (writeHead - delayLen + kBufLen) % kBufLen;
        float delayed = delayBuf[readHead];

        // 2. Apply 8-tap FIR to delayed feedback signal (SNES-style echo filter)
        //    FIR runs on the last 8 output samples of the delay read
        firBuf[firHead & 7] = delayed;
        float firOut = 0.0f;
        for (int i = 0; i < 8; ++i)
            firOut += firCoeffs[i] * firBuf[(firHead - i + 8) & 7];
        firHead = (firHead + 1) & 7;

        // DC block on feedback path (matched-Z 1-pole highpass at ~10 Hz)
        // coeff = exp(-2π * fc / sr) where fc ≈ 10 Hz
        // At 44100: coeff ≈ 0.999857; we precompute once per prepare().
        float dcBlocked = flushDenormal(firOut - dcState);
        dcState = flushDenormal(dcState + dcBlockCoeff * dcBlocked);

        // 3. Write input + filtered feedback into delay line
        float toDelay = x + dcBlocked * feedback;
        // Soft-clip to prevent runaway buildup with near-unity feedback
        toDelay = softClip(toDelay);
        delayBuf[writeHead] = toDelay;
        writeHead = (writeHead + 1) % kBufLen;

        // 4. Wet/dry blend
        return lerp(x, delayed, mix);
    }

private:

    void updateDelayLen() {
        // Map index 0-15 to delay lengths in samples.
        // Each step approximately doubles the time (like the SNES echo buffer).
        // Base: 350 samples (~8ms @ 44.1kHz), then ×2 per step.
        // Actual delays (44.1kHz): 350, 700, 1400, 2800, 5600, 11200, 22400, ...
        // Capped to kBufLen - 1.
        int baseSamples = (int)(sr * 0.008f); // 8ms at current SR
        if (baseSamples < 64) baseSamples = 64;

        delayLen = baseSamples;
        for (int i = 0; i < delayIndex; ++i) {
            delayLen *= 2;
            if (delayLen >= kBufLen) { delayLen = kBufLen - 1; break; }
        }

        // Recompute DC block coefficient for current SR
        // fc ≈ 10 Hz, matched-Z: coeff = exp(-2π*10/sr)
        constexpr float twoPi = 6.28318530717958647692f;
        dcBlockCoeff = fastExp(-twoPi * 10.0f / sr);
    }

    float sr          = 44100.0f;
    int   delayIndex  = 3;
    int   delayLen    = 0;
    float feedback    = 0.5f;
    float mix         = 0.3f;
    float firCoeffs[8];
    float dcBlockCoeff = 0.9986f; // updated in updateDelayLen

    float delayBuf[kBufLen];
    float firBuf[8];
    int   writeHead = 0;
    int   firHead   = 0;
    float dcState   = 0.0f; // DC block state variable
};

} // namespace xoverworld
