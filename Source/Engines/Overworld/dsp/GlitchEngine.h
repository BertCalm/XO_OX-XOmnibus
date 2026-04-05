// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// XOverworld GlitchEngine — Buffer freeze / stutter / reverse glitch effects.
//
// Three operational modes (selected by type parameter):
//   0  = Freeze/stutter: captures a buffer of N samples, then loops them.
//        The freeze window length is driven by glitchDepth (longer = more context).
//   1  = Reverse: fills the freeze buffer, then plays it back in reverse.
//   2  = Random retrigger: stutter with randomly-timed re-captures driven by
//        glitchRate (higher rate = more frequent retriggers).
//   3-12 = Sub-modes: harmonic subdivisions of the stutter period (1/2, 1/3, ...1/10)
//          mapped across types 3-12 to give rhythmic stutter effects.
//
// glitchAmount (0-1): controls whether glitch is active. Below 0.1, the engine
//   is bypassed entirely (no processing overhead). Above 0.1, glitch engages.
// glitchRate (0-1): retrigger rate for mode 2, scaled 0.5Hz–20Hz.
// glitchDepth (0-1): freeze buffer length, scaled 512 samples – kBufLen.
// glitchMix (0-1): wet/dry blend.
//
// Buffer size: 2 seconds at 48kHz = 96000 samples (statically allocated).
// No heap allocation on the audio thread.

#include "../../../DSP/FastMath.h"
#include <cstring>

namespace xoverworld
{

using namespace xoceanus;

class GlitchEngine
{
public:
    static constexpr int kBufLen = 96000; // 2s @ 48kHz; ample for any SR

    GlitchEngine() { std::memset(buf, 0, sizeof(buf)); }

    void prepare(float sampleRate)
    {
        sr = sampleRate;
        writePos = 0;
        readPos = 0;
        frozen = false;
        freezeLen = 4096;
        retrigPhase = 0.0f;
        std::memset(buf, 0, sizeof(buf));
    }

    void setAmount(float a) { amount = a < 0.0f ? 0.0f : a > 1.0f ? 1.0f : a; }
    void setType(int t) { type = t < 0 ? 0 : t > 12 ? 12 : t; }
    void setRate(float r) { rate = r < 0.0f ? 0.0f : r > 1.0f ? 1.0f : r; }
    void setDepth(float d) { depth = d < 0.0f ? 0.0f : d > 1.0f ? 1.0f : d; }
    void setMix(float m) { mix = m < 0.0f ? 0.0f : m > 1.0f ? 1.0f : m; }

    float process(float x)
    {
        juce::ScopedNoDenormals noDenormals;

        // Below threshold: bypass all processing
        if (amount < 0.1f || mix < 0.001f)
        {
            // Continue writing into ring buffer (so freeze has fresh audio)
            buf[writePos] = x;
            writePos = (writePos + 1) % kBufLen;
            return x;
        }

        // Compute freeze window length from depth
        // depth 0 → 512 samples (~11ms @ 44.1kHz), depth 1 → ~sr*0.25 (250ms)
        int maxFreezeLen = std::min((int)(sr * 0.25f), kBufLen - 1);
        freezeLen = 512 + (int)(depth * (float)(maxFreezeLen - 512));
        if (freezeLen < 1)
            freezeLen = 1;

        // Retrigger rate: 0.5 Hz – 20 Hz
        float retrigHz = 0.5f + rate * 19.5f;
        retrigPhase += retrigHz / sr;

        float glitchOut = x;

        if (type == 0)
        {
            // --- Mode 0: Freeze / stutter ---
            if (!frozen)
            {
                // Fill buffer
                buf[writePos] = x;
                writePos = (writePos + 1) % kBufLen;
                // Freeze when amount crosses threshold based on amount knob timing
                // Higher amount = more likely to freeze this sample
                float trigThresh = 1.0f - amount;
                if (retrigPhase >= trigThresh)
                {
                    retrigPhase -= trigThresh;
                    frozen = true;
                    readPos = (writePos - freezeLen + kBufLen) % kBufLen;
                }
                glitchOut = x;
            }
            else
            {
                // Loop the frozen section
                glitchOut = buf[readPos];
                readPos = (readPos + 1) % kBufLen;
                if (readPos == writePos % kBufLen)
                {
                    // End of frozen window — decide to unfreeze
                    if (retrigPhase >= 1.0f)
                    {
                        retrigPhase -= 1.0f;
                        frozen = false;
                    }
                    else
                    {
                        // Re-loop same window
                        readPos = (writePos - freezeLen + kBufLen) % kBufLen;
                    }
                }
                // Still write to keep buffer fresh while frozen
                buf[writePos] = x;
                writePos = (writePos + 1) % kBufLen;
            }
        }
        else if (type == 1)
        {
            // --- Mode 1: Reverse ---
            buf[writePos] = x;
            writePos = (writePos + 1) % kBufLen;

            if (!frozen)
            {
                if (retrigPhase >= 1.0f)
                {
                    retrigPhase -= 1.0f;
                    frozen = true;
                    readPos = writePos; // start reading from current write head (newest)
                }
                glitchOut = x;
            }
            else
            {
                // Read backwards
                readPos = (readPos - 1 + kBufLen) % kBufLen;
                glitchOut = buf[readPos];

                // Count how far we've gone back
                int dist = (writePos - readPos + kBufLen) % kBufLen;
                if (dist >= freezeLen)
                {
                    frozen = false;
                }
            }
        }
        else if (type == 2)
        {
            // --- Mode 2: Random retrigger ---
            buf[writePos] = x;
            writePos = (writePos + 1) % kBufLen;

            if (retrigPhase >= 1.0f)
            {
                retrigPhase -= 1.0f;
                frozen = true;
                readPos = (writePos - freezeLen + kBufLen) % kBufLen;
            }

            if (frozen)
            {
                glitchOut = buf[readPos];
                readPos = (readPos + 1) % kBufLen;
                int dist = (readPos - (writePos - freezeLen + kBufLen) % kBufLen + kBufLen) % kBufLen;
                if (dist >= freezeLen)
                {
                    frozen = false;
                }
            }
            else
            {
                glitchOut = x;
            }
        }
        else
        {
            // --- Types 3-12: Harmonic subdivisions of stutter ---
            // Type 3 = 1/2 period, type 4 = 1/3 period, ..., type 12 = 1/10 period
            int divisor = type - 1; // 2, 3, ..., 10
            buf[writePos] = x;
            writePos = (writePos + 1) % kBufLen;

            int stutterLen = freezeLen / divisor;
            if (stutterLen < 64)
                stutterLen = 64;

            if (!frozen || retrigPhase >= 1.0f)
            {
                if (retrigPhase >= 1.0f)
                    retrigPhase -= 1.0f;
                frozen = true;
                readPos = (writePos - stutterLen + kBufLen) % kBufLen;
            }

            if (frozen)
            {
                glitchOut = buf[readPos];
                readPos = (readPos + 1) % kBufLen;
                int dist = (readPos - (writePos - stutterLen + kBufLen) % kBufLen + kBufLen) % kBufLen;
                if (dist >= stutterLen)
                {
                    // Re-loop the sub-period stutter window
                    readPos = (writePos - stutterLen + kBufLen) % kBufLen;
                }
            }
            else
            {
                glitchOut = x;
            }
        }

        // Amount scales the intensity of the effect (also acts as a gate)
        float scaledGlitch = lerp(x, glitchOut, amount);

        // Wet/dry mix
        return lerp(x, scaledGlitch, mix);
    }

private:
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    float amount = 0.0f;
    int type = 0;
    float rate = 0.3f;
    float depth = 0.5f;
    float mix = 0.0f;

    float buf[kBufLen];
    int writePos = 0;
    int readPos = 0;
    bool frozen = false;
    int freezeLen = 4096;
    float retrigPhase = 0.0f;
};

} // namespace xoverworld
