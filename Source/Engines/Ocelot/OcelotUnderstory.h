// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include <cstdint>
#include "OcelotParamSnapshot.h"
#include "BiomeMorph.h"
#include "BitCrusher.h"
#include "TapeWarp.h"

namespace xocelot {

// OcelotUnderstory -- Sample Mangler + Grain Chopper
//
// Pipeline: internal osc / coupling bus -> [Chop Gate] -> [Bit-Crush] -> [Tape Warp] -> [Dust] -> out
//
// Biome transforms: Underwater = slow warp + blur (slow motion through water)
//                   Winter = cold artifact (colder bit crush, dropout character)
//
// All randomness uses LCG: seed = seed * 1664525u + 1013904223u
// No std::rand(). No memory allocation on audio thread.

class OcelotUnderstory
{
public:
    static constexpr int kBufferSize = 65536; // ~1.5 sec at 44.1k

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        chopBuffer.fill(0.0f);
        writeHead      = 0;
        readHead       = 0.0f;
        oscPhase       = 0.0f;
        lastEnergy     = 0.0f;
        lastPitch      = 0.0f;

        // Chop gate state
        chopSamplePos  = 0;
        currentSegment = 0;
        segmentMuted   = false;

        // DSP primitives
        bitCrusher.reset();
        tapeWarp.prepare(static_cast<float>(sampleRate));

        // LCG seed -- deterministic start
        noiseSeed = 42u;

        // Grain scatter state
        scatterActive  = false;
        scatterRate    = 1.0f;
    }

    // Feed external audio into chop buffer (for coupling)
    void writeCouplingInput(const float* monoSrc, int numSamples, float level)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            chopBuffer[static_cast<size_t>(writeHead)] =
                chopBuffer[static_cast<size_t>(writeHead)] * (1.0f - level)
                + monoSrc[i] * level;
            writeHead = (writeHead + 1) % kBufferSize;
        }
    }

    // renderBlock: fill outL/outR. Returns block RMS energy.
    float renderBlock(float* outL, float* outR, int numSamples,
                      const OcelotParamSnapshot& snap,
                      const BiomeProfile& biome,
                      const struct StrataModulation& mod)
    {
        // -- Effective parameters with biome offsets --

        float effectiveBitDepth = std::clamp(snap.bitDepth + biome.understoryBitShift,
                                              4.0f, 16.0f);
        float effectiveSampleRateRed = std::clamp(snap.sampleRateRed, 4000.0f, 192000.0f);
        float effectiveWobble = std::clamp(snap.tapeWobble + biome.understoryWobbleBase,
                                            0.0f, 1.0f);

        // Chop rate with rhythmic matrix mod
        int chopDiv = std::clamp(snap.chopRate + static_cast<int>(mod.understoryChopRateMod * 4.0f),
                                  1, 32);

        // Slice length in samples. At sr=44100 and chopRate=8, one slice ~5512 samples.
        // chopSwing tilts even/odd slice timing.
        float baseSliceLen = static_cast<float>(sr) / static_cast<float>(chopDiv);

        // Grain scatter: when EcosystemMatrix scatter mod exceeds threshold,
        // read the ring buffer at altered speed (2x or 0.5x)
        if (mod.understoryScatterMod > 0.3f)
        {
            if (!scatterActive)
            {
                scatterActive = true;
                // Deterministic choice: use LCG to pick 2x or 0.5x
                noiseSeed = noiseSeed * 1664525u + 1013904223u;
                scatterRate = (noiseSeed & 1u) ? 2.0f : 0.5f;
            }
        }
        else
        {
            scatterActive = false;
            scatterRate   = 1.0f;
        }

        // Read position offset from canopy grain mod
        float grainPosMod = mod.understoryGrainPosMod;

        // Internal oscillator frequency: sine/saw blend at chopDiv-derived pitch
        float oscFreq = 110.0f * std::pow(2.0f, static_cast<float>(chopDiv) * 0.1f);

        float sumSq = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // ===== 1. SOURCE: Internal Osc or Coupling Bus =====

            if (snap.understorySrc < 0.5f)
            {
                // Internal oscillator: sine/saw blend
                oscPhase += oscFreq / static_cast<float>(sr);
                if (oscPhase >= 1.0f) oscPhase -= 1.0f;

                float saw = oscPhase * 2.0f - 1.0f;
                float sine = std::sin(oscPhase * 2.0f * kPi);
                // Blend: 70% saw, 30% sine for a rich but simple tone
                float oscSample = saw * 0.7f + sine * 0.3f;

                chopBuffer[static_cast<size_t>(writeHead)] = oscSample;
                writeHead = (writeHead + 1) % kBufferSize;
            }

            // ===== 2. CHOP GATE: segment mute/unmute =====

            // Compute slice length for current segment (swing applied)
            bool isEvenSegment = (currentSegment % 2 == 0);
            float swingFactor = isEvenSegment
                ? (1.0f - snap.chopSwing * 0.5f)   // even slices shortened
                : (1.0f + snap.chopSwing * 0.5f);  // odd slices lengthened
            float sliceLenSamples = baseSliceLen * swingFactor;
            sliceLenSamples = std::max(sliceLenSamples, 1.0f); // safety floor

            chopSamplePos++;

            if (static_cast<float>(chopSamplePos) >= sliceLenSamples)
            {
                // Advance to next segment
                chopSamplePos = 0;
                currentSegment++;

                // Deterministic mute decision: hash of (segment index + floorPattern)
                uint32_t hash = deterministicHash(
                    static_cast<uint32_t>(currentSegment),
                    static_cast<uint32_t>(snap.floorPattern));
                // Mute ~30% of segments for rhythmic interest
                segmentMuted = (hash % 100u) < 30u;
            }

            // Read from chop buffer with grain position offset
            float readOffset = static_cast<float>(kBufferSize) * grainPosMod * 0.1f;
            float rHead = readHead + readOffset;
            // Wrap into valid range
            while (rHead < 0.0f) rHead += static_cast<float>(kBufferSize);
            int ri = static_cast<int>(rHead) % kBufferSize;
            float sample = chopBuffer[static_cast<size_t>(ri)];

            // Apply chop gate: mute if this segment is gated
            if (segmentMuted)
                sample = 0.0f;

            // ===== 3. BIT-CRUSH (with sample rate decimation) =====

            sample = bitCrusher.process(sample, effectiveBitDepth,
                                         effectiveSampleRateRed,
                                         static_cast<float>(sr));

            // ===== 4. TAPE WARP =====

            sample = tapeWarp.process(sample, effectiveWobble, snap.tapeAge);

            // ===== 5. DUST (vinyl crackle -- rare LCG impulse noise) =====

            float dustProb = snap.dustLevel * 0.001f;
            noiseSeed = noiseSeed * 1664525u + 1013904223u;
            float dustRand = static_cast<float>(noiseSeed >> 8) / 16777216.0f; // 0..1
            if (dustRand < dustProb)
            {
                noiseSeed = noiseSeed * 1664525u + 1013904223u;
                float crackle = static_cast<float>(static_cast<int32_t>(noiseSeed)) / 2147483648.0f;
                sample += crackle * 0.3f;
            }

            // ===== 6. OUTPUT LEVEL =====

            sample *= snap.understoryLevel;

            // Denormal protection
            sample = flushDenormal(sample);

            outL[i] = sample;
            outR[i] = sample;
            sumSq  += sample * sample;

            // Advance read head with scatter rate
            readHead += scatterRate;
            if (readHead >= static_cast<float>(kBufferSize))
                readHead -= static_cast<float>(kBufferSize);
        }

        lastEnergy = std::sqrt(sumSq / static_cast<float>(std::max(numSamples, 1)));
        lastPitch  = oscFreq;
        return lastEnergy;
    }

    float getLastEnergy() const { return lastEnergy; }
    float getLastPitch() const  { return lastPitch; }

private:
    static constexpr float kPi = 3.14159265358979323846f;

    double sr = 0.0;
    std::array<float, kBufferSize> chopBuffer{};
    int   writeHead      = 0;
    float readHead       = 0.0f;
    float oscPhase       = 0.0f;
    float lastEnergy     = 0.0f;
    float lastPitch      = 0.0f;

    // Chop gate state
    int  chopSamplePos   = 0;
    int  currentSegment  = 0;
    bool segmentMuted    = false;

    // DSP primitives (pre-allocated, no audio-thread allocation)
    BitCrusher bitCrusher;
    TapeWarp   tapeWarp;

    // LCG noise state (replaces std::rand)
    uint32_t noiseSeed   = 42u;

    // Grain scatter state
    bool  scatterActive  = false;
    float scatterRate    = 1.0f;

    // Deterministic hash for segment mute decisions
    // Uses floorPattern as seed so mute pattern is repeatable per-preset
    static uint32_t deterministicHash(uint32_t segmentIndex, uint32_t pattern)
    {
        uint32_t h = segmentIndex * 2654435761u + pattern * 340573321u;
        h ^= h >> 16;
        h *= 0x45d9f3bu;
        h ^= h >> 16;
        return h;
    }

    // Denormal flush
    static float flushDenormal(float x)
    {
        union { float f; uint32_t u; } val;
        val.f = x;
        if ((val.u & 0x7F800000u) == 0u && (val.u & 0x007FFFFFu) != 0u)
            return 0.0f;
        return x;
    }
};

} // namespace xocelot
