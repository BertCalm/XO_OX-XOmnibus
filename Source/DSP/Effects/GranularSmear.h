// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include <cstring>
#include "../FastMath.h"
#include "../SRO/LookupTable.h"

namespace xoceanus
{

//==============================================================================
// GranularSmear — Micro-granular buffer that smears transients into texture.
//
// Captures audio into a small circular buffer (50-200ms), then reads back
// 4 overlapping grains with randomized positions and Hann windowing.
// At low smear amounts, transients soften. At high amounts, sound dissolves
// into ambient texture.
//
// Controls:
//   smear:    0..1 — grain scatter amount (0 = clean, 1 = full dissolve)
//   grainSize: 10..200ms — individual grain length
//   density:  0..1 — grain overlap (0 = sparse, 1 = dense)
//   mix:      0..1 — wet/dry blend
//
// CPU: 4 grains × (read + window) = trivial.
//==============================================================================
class GranularSmear
{
public:
    GranularSmear() = default;

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        // Buffer: 250ms max
        bufferSize = static_cast<int>(sr * 0.25);
        bufferL.resize(static_cast<size_t>(bufferSize), 0.0f);
        bufferR.resize(static_cast<size_t>(bufferSize), 0.0f);
        reset();
    }

    void setSmear(float s) { smear = std::clamp(s, 0.0f, 1.0f); }
    void setGrainSize(float ms) { grainMs = std::clamp(ms, 10.0f, 200.0f); }
    void setDensity(float d) { density = std::clamp(d, 0.0f, 1.0f); }
    void setMix(float m) { mix = std::clamp(m, 0.0f, 1.0f); }

    void processBlock(float* left, float* right, int numSamples)
    {
        if (bufferSize <= 0)
            return;
        if (mix < 0.001f || smear < 0.001f)
            return;

        const int grainSizeSamples = std::max(1, static_cast<int>(grainMs * 0.001f * static_cast<float>(sr)));

        for (int i = 0; i < numSamples; ++i)
        {
            // Write input to circular buffer
            bufferL[static_cast<size_t>(writePos)] = left[i];
            bufferR[static_cast<size_t>(writePos)] = right[i];

            float dryL = left[i];
            float dryR = right[i];
            float wetL = 0.0f;
            float wetR = 0.0f;

            // Read from 4 grains
            for (int g = 0; g < kNumGrains; ++g)
            {
                auto& grain = grains[g];

                if (grain.samplesRemaining <= 0)
                {
                    // Retrigger grain
                    grain.samplesRemaining = grainSizeSamples;
                    grain.grainLength = grainSizeSamples;

                    // Scatter: how far back in the buffer to read
                    float maxScatter = smear * static_cast<float>(bufferSize - grainSizeSamples - 1);
                    float scatter = maxScatter * pseudoRandom(grain.seed);
                    grain.readPos = writePos - static_cast<int>(scatter) - grainSizeSamples;
                    if (grain.readPos < 0)
                        grain.readPos += bufferSize;

                    // Stagger grain start based on density
                    float stagger = (1.0f - density) * static_cast<float>(grainSizeSamples);
                    grain.samplesRemaining += static_cast<int>(stagger * (static_cast<float>(g) / kNumGrains));
                }

                if (grain.samplesRemaining > 0 && grain.samplesRemaining <= grain.grainLength)
                {
                    // Hann window position
                    float phase = static_cast<float>(grain.grainLength - grain.samplesRemaining) /
                                  static_cast<float>(grain.grainLength);
                    // SRO: Hann LUT replaces trig (per-sample × 4 grains, zero trig)
                    float window = SROTables::hann().lookupNormalized(phase);

                    int rp = grain.readPos % bufferSize;
                    if (rp < 0)
                        rp += bufferSize;

                    wetL += bufferL[static_cast<size_t>(rp)] * window;
                    wetR += bufferR[static_cast<size_t>(rp)] * window;

                    grain.readPos = (grain.readPos + 1) % bufferSize;
                }

                grain.samplesRemaining--;
            }

            // Normalize by grain count
            wetL *= (1.0f / kNumGrains);
            wetR *= (1.0f / kNumGrains);

            // Mix
            left[i] = dryL + mix * (wetL - dryL);
            right[i] = dryR + mix * (wetR - dryR);

            writePos = (writePos + 1) % bufferSize;
        }
    }

    void reset()
    {
        std::fill(bufferL.begin(), bufferL.end(), 0.0f);
        std::fill(bufferR.begin(), bufferR.end(), 0.0f);
        writePos = 0;

        for (int g = 0; g < kNumGrains; ++g)
        {
            grains[g].readPos = 0;
            grains[g].samplesRemaining = 0;
            grains[g].grainLength = 256;
            grains[g].seed = static_cast<uint32_t>(g * 1337 + 42);
        }
    }

private:
    static constexpr int kNumGrains = 4;

    struct Grain
    {
        int readPos = 0;
        int samplesRemaining = 0;
        int grainLength = 256;
        uint32_t seed = 0;
    };

    std::array<Grain, kNumGrains> grains;
    std::vector<float> bufferL, bufferR;
    int bufferSize = 0;
    int writePos = 0;

    double sr = 44100.0;
    float smear = 0.0f;
    float grainMs = 60.0f;
    float density = 0.5f;
    float mix = 1.0f;

    // Simple deterministic pseudo-random (no allocation, no stdlib)
    static float pseudoRandom(uint32_t& seed)
    {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        return static_cast<float>(seed & 0xFFFF) / 65535.0f;
    }
};

} // namespace xoceanus
