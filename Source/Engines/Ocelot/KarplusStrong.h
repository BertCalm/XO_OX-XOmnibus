#pragma once
#include <cmath>
#include <algorithm>
#include <array>

namespace xocelot {

// KarplusStrong — plucked string model via delay line + 1-pole lowpass feedback.
// Used by Berimbau model in OcelotFloor.
// Supports fractional-delay interpolation for accurate tuning.

struct KarplusStrong
{
    static constexpr int kMaxDelay = 4096;

    void prepare(float sampleRate)
    {
        sr = sampleRate;
        delay.fill(0.0f);
        writePos = 0;
        prevFiltered = 0.0f;
    }

    void setFreq(float freq)
    {
        freq = std::clamp(freq, sr / static_cast<float>(kMaxDelay - 1), sr / 2.0f);
        periodF = sr / freq;
        periodI = static_cast<int>(periodF);
        fracDelay = periodF - static_cast<float>(periodI);
    }

    // Inject noise burst excitation (call once per noteOn)
    void excite(float velocity, uint32_t& seed)
    {
        for (int i = 0; i < periodI && i < kMaxDelay; ++i)
        {
            seed = seed * 1664525u + 1013904223u;
            float noise = static_cast<float>(static_cast<int32_t>(seed)) * 4.656612e-10f;
            delay[static_cast<size_t>((writePos + i) % kMaxDelay)] = noise * velocity;
        }
    }

    // Process one sample. damping: 0 = no damping (infinite sustain), 1 = maximum damping.
    float process(float damping)
    {
        int readPos  = (writePos - periodI + kMaxDelay) % kMaxDelay;
        int readPos2 = (readPos - 1 + kMaxDelay) % kMaxDelay;

        // Linear interpolation for fractional delay
        float d0 = delay[static_cast<size_t>(readPos)];
        float d1 = delay[static_cast<size_t>(readPos2)];
        float interpolated = d0 + fracDelay * (d1 - d0);

        // 1-pole lowpass: mix between current and previous (damping controls cutoff)
        float lpCoeff = 0.5f + damping * 0.48f;  // 0.5 (bright) → 0.98 (dark)
        float filtered = interpolated * (1.0f - lpCoeff) + prevFiltered * lpCoeff;
        prevFiltered = filtered;

        delay[static_cast<size_t>(writePos)] = filtered;
        writePos = (writePos + 1) % kMaxDelay;

        // Denormal flush
        if (std::abs(prevFiltered) < 1.0e-15f) prevFiltered = 0.0f;

        return interpolated;  // return pre-filter for brighter output; filtered is feedback
    }

    void reset()
    {
        delay.fill(0.0f);
        writePos = 0;
        prevFiltered = 0.0f;
    }

    float getAmplitude() const
    {
        // Approximate from last few samples
        int rp = (writePos - 1 + kMaxDelay) % kMaxDelay;
        return std::abs(delay[static_cast<size_t>(rp)]);
    }

private:
    float sr = 0.0f;
    std::array<float, kMaxDelay> delay{};
    int writePos = 0;
    float periodF = 100.0f;
    int periodI = 100;
    float fracDelay = 0.0f;
    float prevFiltered = 0.0f;
};

} // namespace xocelot
