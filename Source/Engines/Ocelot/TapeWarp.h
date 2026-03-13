#pragma once
#include <cmath>
#include <algorithm>
#include <array>

namespace xocelot {

// TapeWarp — LFO-modulated delay line for Mellotron-style flutter/wobble.
// tapeWobble controls wobble depth (0–8ms), tapeAge controls LFO rate + noise.

struct TapeWarp
{
    static constexpr int kDelaySize = 2048; // ~46ms at 44.1k

    void prepare(float sampleRate)
    {
        sr = sampleRate;
        delay.fill(0.0f);
        writePos = 0;
        lfoPhase = 0.0f;
    }

    // wobble: 0–1 (depth of flutter in ms)
    // age: 0–1 (rate of LFO + noise on the flutter)
    float process(float x, float wobble, float age)
    {
        constexpr float kPi = 3.14159265358979323846f;

        delay[static_cast<size_t>(writePos)] = x;

        // LFO for flutter (rate increases with age)
        float lfoRate = 0.5f + age * 5.0f;  // 0.5–5.5 Hz
        lfoPhase += lfoRate / sr;
        if (lfoPhase > 1.0f) lfoPhase -= 1.0f;
        float lfo = std::sin(lfoPhase * 2.0f * kPi);

        // Modulated delay in samples (0–8ms flutter range)
        float delayMs = wobble * 8.0f * (0.5f + 0.5f * lfo);
        float delaySamples = std::clamp(delayMs * sr / 1000.0f, 0.0f,
                                         static_cast<float>(kDelaySize - 2));

        // Linear interpolation read
        float readPosF = static_cast<float>(writePos) - delaySamples;
        if (readPosF < 0.0f) readPosF += static_cast<float>(kDelaySize);
        int ri = static_cast<int>(readPosF);
        float frac = readPosF - static_cast<float>(ri);
        float s0 = delay[static_cast<size_t>(ri % kDelaySize)];
        float s1 = delay[static_cast<size_t>((ri + 1) % kDelaySize)];
        float out = s0 + frac * (s1 - s0);

        writePos = (writePos + 1) % kDelaySize;

        return out;
    }

    void reset()
    {
        delay.fill(0.0f);
        writePos = 0;
        lfoPhase = 0.0f;
    }

private:
    float sr = 44100.0f;
    std::array<float, kDelaySize> delay{};
    int writePos = 0;
    float lfoPhase = 0.0f;
};

} // namespace xocelot
