// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include "../FastMath.h"

namespace xoceanus
{

//==============================================================================
// VoronoiShatter — Spatial-granular processor using geometric partitioning.
//
// Math Mixture: Voronoi Diagrams + Fibonacci Spiral + Tensegrity.
//
// Seeds are placed along a Fibonacci Spiral (golden-angle distribution).
// A Voronoi-like algorithm partitions the audio buffer into grains based
// on distance to the nearest seed. Tensegrity equations move these cells
// in the stereo field: higher tension pulls grains toward center, lower
// tension scatters them to extreme edges.
//
// Parameters:
//   mfx_vsCrystallize (CC 31) — morphs grains from organic curves to jagged polygons (0–1)
//   mfx_vsTension             — tensegrity: center pull vs scatter (0–1)
//   mfx_vsGrainSize           — base grain size in ms (5–100)
//   mfx_vsMix                 — dry/wet (0–1)
//
// The "Crystallize" parameter controls how sharp the grain boundaries are:
// low = smooth crossfades between grains (organic Fibonacci), high = hard
// cuts at Voronoi cell edges (jagged polygonal).
//==============================================================================
class VoronoiShatter
{
public:
    VoronoiShatter() = default;

    void prepare(double sampleRate)
    {
        sr = static_cast<float>(sampleRate);

        // Circular grain buffer (~200ms)
        int bufSize = static_cast<int>(sr * 0.2f) + 1;
        grainBufL.assign(static_cast<size_t>(bufSize), 0.0f);
        grainBufR.assign(static_cast<size_t>(bufSize), 0.0f);
        writePos = 0;

        // Generate Fibonacci spiral seed positions (up to 16 seeds)
        // Golden angle in radians: π(3 - √5)
        constexpr float goldenAngle = 2.39996323f;
        for (int i = 0; i < kMaxSeeds; ++i)
        {
            float angle = static_cast<float>(i) * goldenAngle;
            float radius = std::sqrt(static_cast<float>(i + 1)) / std::sqrt(static_cast<float>(kMaxSeeds));
            seeds[i].x = radius * std::cos(angle); // -1 to +1
            seeds[i].y = radius * std::sin(angle); // -1 to +1
            seeds[i].grainPhase = 0.0f;
        }

        outputPhase = 0.0f;
    }

    void reset()
    {
        std::fill(grainBufL.begin(), grainBufL.end(), 0.0f);
        std::fill(grainBufR.begin(), grainBufR.end(), 0.0f);
        writePos = 0;
        outputPhase = 0.0f;
    }

    //--------------------------------------------------------------------------
    void processBlock(float* L, float* R, int numSamples, float crystallize, float tension, float grainSizeMs,
                      float mix)
    {
        if (mix < 0.001f)
            return;

        int bufSize = static_cast<int>(grainBufL.size());
        if (bufSize < 2)
            return;

        // Grain size in samples (clamped to buffer)
        int grainSamples = std::max(32, std::min(bufSize - 1, static_cast<int>(grainSizeMs * 0.001f * sr)));

        // Number of active seeds scales with grain size (more seeds = smaller grains)
        int numSeeds = std::max(2, std::min(kMaxSeeds, static_cast<int>(4.0f + crystallize * 12.0f)));

        for (int s = 0; s < numSamples; ++s)
        {
            // Write dry signal into circular buffer
            grainBufL[static_cast<size_t>(writePos)] = L[s];
            grainBufR[static_cast<size_t>(writePos)] = R[s];

            // --- Voronoi grain selection ---
            // Current position in the grain cycle
            outputPhase += 1.0f / static_cast<float>(grainSamples);
            if (outputPhase >= 1.0f)
                outputPhase -= 1.0f;

            // Find 2 nearest seeds to current phase position (for crossfade)
            float minDist1 = 999.0f, minDist2 = 999.0f;
            int nearestSeed1 = 0, nearestSeed2 = 1;

            for (int i = 0; i < numSeeds; ++i)
            {
                // Distance in 1D phase space (wrapped)
                float seedPhase = static_cast<float>(i) / static_cast<float>(numSeeds);
                float dist = std::fabs(outputPhase - seedPhase);
                dist = std::min(dist, 1.0f - dist); // wrap

                if (dist < minDist1)
                {
                    minDist2 = minDist1;
                    nearestSeed2 = nearestSeed1;
                    minDist1 = dist;
                    nearestSeed1 = i;
                }
                else if (dist < minDist2)
                {
                    minDist2 = dist;
                    nearestSeed2 = i;
                }
            }

            // Grain read offset: each seed reads from a different buffer position
            // Fibonacci spiral position determines the temporal offset
            int offset1 = static_cast<int>((seeds[nearestSeed1].x + 1.0f) * 0.5f * static_cast<float>(grainSamples));
            int offset2 = static_cast<int>((seeds[nearestSeed2].x + 1.0f) * 0.5f * static_cast<float>(grainSamples));

            int readPos1 = (writePos - offset1 + bufSize) % bufSize;
            int readPos2 = (writePos - offset2 + bufSize) % bufSize;

            float grain1L = grainBufL[static_cast<size_t>(readPos1)];
            float grain1R = grainBufR[static_cast<size_t>(readPos1)];
            float grain2L = grainBufL[static_cast<size_t>(readPos2)];
            float grain2R = grainBufR[static_cast<size_t>(readPos2)];

            // --- Crossfade between grains (crystallize controls sharpness) ---
            // Low crystallize = smooth Hann-window crossfade (organic)
            // High crystallize = hard step (jagged Voronoi edges)
            float cellDist = minDist1 / std::max(0.001f, minDist1 + minDist2);
            float crossfade;
            if (crystallize < 0.5f)
            {
                // Smooth: raised cosine crossfade
                float smoothness = 1.0f - crystallize * 2.0f;
                float angle = cellDist * 3.14159f;
                crossfade = 0.5f * (1.0f + std::cos(angle)) * smoothness + cellDist * (1.0f - smoothness);
            }
            else
            {
                // Jagged: hard threshold with slight smoothing
                float sharpness = (crystallize - 0.5f) * 2.0f;
                float threshold = 0.5f - sharpness * 0.45f; // approaches 0.05
                crossfade = (cellDist < threshold) ? 1.0f : 0.0f;
                // Tiny smooth to prevent clicks
                crossfade = crossfade * 0.95f + (1.0f - cellDist) * 0.05f;
            }

            float grainL = grain1L * crossfade + grain2L * (1.0f - crossfade);
            float grainR = grain1R * crossfade + grain2R * (1.0f - crossfade);

            // --- Tensegrity stereo placement ---
            // tension = 0: grains scatter to extreme L/R based on seed position
            // tension = 1: grains pull toward center
            float seedPan = seeds[nearestSeed1].y;        // -1 to +1
            float panAmount = seedPan * (1.0f - tension); // tension damps panning

            float panL = std::max(0.0f, std::min(1.0f, 0.5f - panAmount * 0.5f));
            float panR = std::max(0.0f, std::min(1.0f, 0.5f + panAmount * 0.5f));

            float wetL = grainL * panL * 2.0f; // compensate for pan law
            float wetR = grainR * panR * 2.0f;

            wetL = flushDenormal(wetL);
            wetR = flushDenormal(wetR);

            L[s] = L[s] * (1.0f - mix) + wetL * mix;
            R[s] = R[s] * (1.0f - mix) + wetR * mix;

            writePos = (writePos + 1) % bufSize;
        }
    }

private:
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)

    static constexpr int kMaxSeeds = 16;

    struct Seed
    {
        float x = 0.0f; // Fibonacci spiral x (-1 to +1)
        float y = 0.0f; // Fibonacci spiral y (-1 to +1)
        float grainPhase = 0.0f;
    };
    std::array<Seed, kMaxSeeds> seeds{};

    // Circular grain buffer
    std::vector<float> grainBufL, grainBufR;
    int writePos = 0;

    float outputPhase = 0.0f;
};

} // namespace xoceanus
