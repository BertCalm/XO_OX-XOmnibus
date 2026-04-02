// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "OwlfishFastMath.h"
#include <cmath>
#include <cstdint>

namespace xowlfish {

//==============================================================================
// MicroGranular -- Ultra-short predatory grain engine (DIET).
//
// 2-10 ms micro-grains simulate the owlfish's predatory feeding strategy:
// lightning-fast micro-captures of sonic material processed through
// a stochastic grain cloud with pitch scatter and density control.
//
// Circular buffer captures input. Grains read from it with Hann windows,
// random pitch scatter, and configurable density (10-200 grains/sec).
// Feed rate controls write-vs-read speed for repeated or fast-forward material.
//
// Max 16 simultaneous grains. All buffers fixed-size. No allocations.
// Mix is external -- caller blends wet/dry.
//==============================================================================

class MicroGranular
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    void reset()
    {
        for (int i = 0; i < kBufferSize; ++i)
            buffer[i] = 0.0f;
        writePos = 0;
        writePhase = 0.0f;
        spawnCounter = 0.0f;

        for (int i = 0; i < kMaxGrains; ++i)
            grains[i].active = false;

        rngState = 12345;
    }

    /// Set parameters from normalized 0-1 values (call once per block).
    /// size01:     0 -> 2 ms, 1 -> 10 ms grain length
    /// density01:  0 -> 10 grains/sec, 1 -> 200 grains/sec
    /// pitch01:    0 -> 0 semitones scatter, 1 -> 12 semitones scatter
    /// feedRate01: 0 -> 0.1x write speed, 1 -> 2.0x write speed
    void setParams (float size01, float density01, float pitch01, float feedRate01)
    {
        // Grain size: 2ms to 10ms in samples
        float grainMs = 2.0f + size01 * 8.0f;
        grainSizeSamples = static_cast<int> (grainMs * 0.001f * static_cast<float> (sr));
        if (grainSizeSamples < 4) grainSizeSamples = 4;

        // Grain density: 10 to 200 grains/sec -> spawn interval in samples
        float density = 10.0f + density01 * 190.0f;
        spawnInterval = static_cast<float> (sr) / density;

        // Pitch scatter: 0 to 12 semitones
        pitchScatterSemitones = pitch01 * 12.0f;

        // Feed rate: 0.1 to 2.0
        feedRateMultiplier = 0.1f + feedRate01 * 1.9f;
    }

    /// Feed one sample into the capture buffer.
    void writeSample (float input)
    {
        // Fractional write position for feed rate control
        writePhase += feedRateMultiplier;

        while (writePhase >= 1.0f)
        {
            buffer[writePos] = input;
            writePos = (writePos + 1) & (kBufferSize - 1);
            writePhase -= 1.0f;
        }
    }

    /// Get one sample of granulated output.
    float readSample()
    {
        // -- Grain scheduling --
        spawnCounter += 1.0f;
        if (spawnCounter >= spawnInterval)
        {
            // Add jitter: +/- 50% of interval
            float jitter = (nextRandom() - 0.5f) * spawnInterval;
            spawnCounter = jitter;

            spawnGrain();
        }

        // -- Sum active grains --
        float output = 0.0f;
        for (int i = 0; i < kMaxGrains; ++i)
        {
            if (!grains[i].active) continue;

            Grain& g = grains[i];

            // Read from buffer with linear interpolation
            int posInt = static_cast<int> (g.position);
            float frac = g.position - static_cast<float> (posInt);
            int idx0 = posInt & (kBufferSize - 1);
            int idx1 = (posInt + 1) & (kBufferSize - 1);
            float sample = buffer[idx0] + frac * (buffer[idx1] - buffer[idx0]);

            // Hann window: 0.5 * (1 - cos(2*pi*t)) where t = windowPhase (0->1)
            constexpr float twoPi = 6.28318530717958647692f;
            float window = 0.5f * (1.0f - fastCos (g.windowPhase * twoPi));

            output += sample * window * g.amplitude;

            // Advance grain position and window
            g.position += g.phaseInc;
            g.windowPhase += g.windowInc;

            // Deactivate when window completes
            if (g.windowPhase >= 1.0f)
                g.active = false;
        }

        return flushDenormal (output);
    }

private:
    static constexpr int kBufferSize = 4096;
    static constexpr int kMaxGrains  = 16;

    float buffer[kBufferSize] = {};
    int   writePos = 0;
    float writePhase = 0.0f;

    struct Grain
    {
        float position    = 0.0f;    // read position in buffer (fractional)
        float phaseInc    = 1.0f;    // playback rate
        float windowPhase = 0.0f;    // 0->1 over grain lifetime
        float windowInc   = 0.0f;    // per-sample window advance
        float amplitude   = 1.0f;
        bool  active      = false;
    };
    Grain grains[kMaxGrains];

    double   sr = 44100.0;
    float    spawnCounter = 0.0f;
    float    spawnInterval = 4410.0f;   // samples between spawns
    int      grainSizeSamples = 200;
    float    pitchScatterSemitones = 0.0f;
    float    feedRateMultiplier = 1.0f;

    // Simple LCG random (audio-thread safe, no std::random)
    uint32_t rngState = 12345;

    float nextRandom()
    {
        rngState = rngState * 1664525u + 1013904223u;
        return static_cast<float> (rngState) / static_cast<float> (0xFFFFFFFFu);
    }

    void spawnGrain()
    {
        // Find an inactive grain slot
        int slot = -1;
        for (int i = 0; i < kMaxGrains; ++i)
        {
            if (!grains[i].active)
            {
                slot = i;
                break;
            }
        }
        if (slot < 0) return;  // all slots full

        Grain& g = grains[slot];

        // Random start position within the recent buffer content
        // Read from somewhere behind the write head
        float offset = nextRandom() * static_cast<float> (kBufferSize - grainSizeSamples);
        g.position = static_cast<float> ((writePos - static_cast<int> (offset)
                      + kBufferSize) & (kBufferSize - 1));

        // Per-grain pitch scatter: random offset in semitones
        float randomOffset = nextRandom() * pitchScatterSemitones;
        g.phaseInc = std::pow (2.0f, randomOffset / 12.0f);

        // Window advances from 0 to 1 over grainSizeSamples
        g.windowPhase = 0.0f;
        g.windowInc = 1.0f / static_cast<float> (grainSizeSamples);

        g.amplitude = 1.0f;
        g.active = true;
    }
};

} // namespace xowlfish
