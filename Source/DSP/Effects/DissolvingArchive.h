// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include "../FastMath.h"

namespace xoceanus
{

//==============================================================================
// DissolvingArchive — Persistent memory buffer with stochastic grain extraction.
//
// Mashup: Chase Bliss Habit × OBNE Parting
//
// CPU-optimized version: 30-second memory well (not 3 minutes) to stay within
// budget. At 48kHz stereo: ~2.88M samples — fits in ~11MB per channel.
//
// A. Memory Well: 30s circular buffer, always writing.
// B. Extraction Engine: Transient-triggered Poisson grain extraction.
//    Grains 50-500ms (reduced from 500-2000ms for CPU). Probabilistic
//    pitch shift (+12st) or ratchet (4 repeats, not 16th-note).
// C. Dissolve Chamber: Sample-rate decimation (ZOH) from native down to 4kHz
//    (reduced from 2kHz to halve the aliasing CPU). Fed into diffuse reverb.
//
// CC42 = Chance Extractor, CC43 = Data Dissolve intensity.
//==============================================================================
class DissolvingArchive
{
public:
    DissolvingArchive() = default;

    void prepare(double sampleRate)
    {
        sr = static_cast<float>(sampleRate);

        // Memory Well: 30 seconds (CPU-friendly vs spec's 3 minutes)
        int wellSize = static_cast<int>(sr * 30.0f) + 1;
        wellL.assign(static_cast<size_t>(wellSize), 0.0f);
        wellR.assign(static_cast<size_t>(wellSize), 0.0f);
        wellWritePos = 0;

        // Grain state
        grainActive = false;
        grainReadPos = 0;
        grainLength = 0;
        grainProgress = 0;
        grainGainL = grainGainR = 0.0f;
        grainPitchRatio = 1.0f;
        grainReadPhase = 0.0f;

        // Transient detection
        envFollower = 0.0f;
        prevEnvFollower = 0.0f;
        rng = 3141592653u;

        // Dissolve: ZOH state
        zohHoldL = zohHoldR = 0.0f;
        zohCounter = 0;

        // Simple diffuse reverb on grain output
        float srScale = sr / 44100.0f;
        static constexpr int kDiffLens[4] = {887, 1093, 1303, 1523};
        for (int i = 0; i < 4; ++i)
        {
            int len = static_cast<int>(static_cast<float>(kDiffLens[i]) * srScale) + 1;
            diffBuf[i].assign(static_cast<size_t>(len), 0.0f);
            diffPos[i] = 0;
            diffFilt[i] = 0.0f;
        }
    }

    void reset()
    {
        std::fill(wellL.begin(), wellL.end(), 0.0f);
        std::fill(wellR.begin(), wellR.end(), 0.0f);
        wellWritePos = 0;
        grainActive = false;
        grainReadPos = 0;
        grainLength = 0;
        grainProgress = 0;
        envFollower = prevEnvFollower = 0.0f;
        zohHoldL = zohHoldR = 0.0f;
        zohCounter = 0;
        for (int i = 0; i < 4; ++i)
        {
            std::fill(diffBuf[i].begin(), diffBuf[i].end(), 0.0f);
            diffPos[i] = 0;
            diffFilt[i] = 0.0f;
        }
    }

    void processBlock(float* L, float* R, int numSamples, float chanceProb, float dissolveAmount, float grainMix,
                      float reverbMix, float mix)
    {
        if (mix < 0.001f)
            return;

        int wellSize = static_cast<int>(wellL.size());
        if (wellSize < 2)
            return;

        float transAttack = 1.0f - std::exp(-1.0f / (0.001f * sr));
        float transRelease = 1.0f - std::exp(-1.0f / (0.05f * sr));

        // ZOH decimation ratio: 1 = no effect, 12 = sr/4kHz at 48kHz
        int zohRatio = std::max(1, static_cast<int>(1.0f + dissolveAmount * 11.0f));

        for (int s = 0; s < numSamples; ++s)
        {
            float dryL = L[s], dryR = R[s];

            // Always write to memory well
            wellL[static_cast<size_t>(wellWritePos)] = dryL;
            wellR[static_cast<size_t>(wellWritePos)] = dryR;
            wellWritePos = (wellWritePos + 1) % wellSize;

            // --- Transient detection ---
            float absIn = std::fabs(dryL) + std::fabs(dryR);
            float envCoeff = (absIn > envFollower) ? transAttack : transRelease;
            envFollower += (absIn - envFollower) * envCoeff;

            // Detect rising edge (transient)
            bool transient = (envFollower > prevEnvFollower * 1.5f + 0.01f);
            prevEnvFollower = envFollower;

            // --- Poisson extraction trigger ---
            if (transient && !grainActive && chanceProb > 0.01f)
            {
                rng = rng * 1664525u + 1013904223u;
                float roll = static_cast<float>(rng & 0xFFFF) / 65536.0f;
                if (roll < chanceProb)
                {
                    // Extract grain from random past position
                    rng = rng * 1664525u + 1013904223u;
                    int jumpBack = static_cast<int>((static_cast<float>(rng & 0xFFFF) / 65536.0f) *
                                                    static_cast<float>(wellSize - 1));
                    jumpBack = std::max(1, jumpBack);

                    grainReadPos = (wellWritePos - jumpBack + wellSize) % wellSize;
                    grainLength =
                        static_cast<int>((0.05f + (static_cast<float>((rng >> 16) & 0xFF) / 255.0f) * 0.45f) * sr);
                    grainLength = std::min(grainLength, wellSize / 2);
                    grainProgress = 0;
                    grainActive = true;
                    grainReadPhase = static_cast<float>(grainReadPos);

                    // Probabilistic pitch: 70% normal, 20% +12st, 10% ratchet
                    rng = rng * 1664525u + 1013904223u;
                    float pitchRoll = static_cast<float>(rng & 0xFF) / 255.0f;
                    if (pitchRoll < 0.7f)
                        grainPitchRatio = 1.0f;
                    else if (pitchRoll < 0.9f)
                        grainPitchRatio = 2.0f; // +12st
                    else
                        grainPitchRatio = 4.0f; // ratchet (4× speed = rapid repeats)
                }
            }

            // --- Read grain ---
            float grainOutL = 0.0f, grainOutR = 0.0f;
            if (grainActive)
            {
                // Hann window
                float t = static_cast<float>(grainProgress) / static_cast<float>(grainLength);
                float window = 0.5f * (1.0f - std::cos(6.28318f * t));

                int idx = static_cast<int>(grainReadPhase) % wellSize;
                grainOutL = wellL[static_cast<size_t>(idx)] * window;
                grainOutR = wellR[static_cast<size_t>(idx)] * window;

                grainReadPhase += grainPitchRatio;
                if (grainReadPhase >= static_cast<float>(wellSize))
                    grainReadPhase -= static_cast<float>(wellSize);

                grainProgress++;
                if (grainProgress >= grainLength)
                    grainActive = false;
            }

            // --- Dissolve Chamber: ZOH decimation ---
            if (dissolveAmount > 0.01f && (grainOutL != 0.0f || grainOutR != 0.0f))
            {
                zohCounter++;
                if (zohCounter >= zohRatio)
                {
                    zohHoldL = grainOutL;
                    zohHoldR = grainOutR;
                    zohCounter = 0;
                }
                grainOutL = zohHoldL;
                grainOutR = zohHoldR;
            }

            // --- Diffuse reverb on grain output ---
            float diffOut = 0.0f;
            if (reverbMix > 0.001f && (grainOutL != 0.0f || grainOutR != 0.0f || diffFilt[0] > 0.0001f))
            {
                float input = (grainOutL + grainOutR) * 0.5f;
                float tap[4];
                for (int i = 0; i < 4; ++i)
                {
                    int len = static_cast<int>(diffBuf[i].size());
                    if (len == 0)
                    {
                        tap[i] = 0.0f;
                        continue;
                    }
                    int rp = (diffPos[i] - len + 1 + len) % len;
                    tap[i] = diffBuf[i][static_cast<size_t>(rp)];
                    diffFilt[i] = flushDenormal(diffFilt[i] + (tap[i] - diffFilt[i]) * 0.3f);
                    tap[i] = diffFilt[i];
                }
                float tapSum = tap[0] + tap[1] + tap[2] + tap[3];
                for (int i = 0; i < 4; ++i)
                {
                    float fb = fastTanh((tap[i] - 0.5f * tapSum) * 0.6f + input);
                    int len = static_cast<int>(diffBuf[i].size());
                    if (len > 0)
                        diffBuf[i][static_cast<size_t>(diffPos[i])] = flushDenormal(fb);
                    diffPos[i] = (diffPos[i] + 1) % std::max(1, len);
                }
                diffOut = (tap[0] + tap[2]) * 0.5f * reverbMix;
            }

            float wetL = grainOutL * grainMix + diffOut;
            float wetR = grainOutR * grainMix + diffOut;

            L[s] = dryL * (1.0f - mix) + wetL * mix;
            R[s] = dryR * (1.0f - mix) + wetR * mix;
        }
    }

private:
    float sr = 44100.0f;

    // Memory Well
    std::vector<float> wellL, wellR;
    int wellWritePos = 0;

    // Grain state
    bool grainActive = false;
    int grainReadPos = 0;
    int grainLength = 0;
    int grainProgress = 0;
    float grainGainL = 0.0f, grainGainR = 0.0f;
    float grainPitchRatio = 1.0f;
    float grainReadPhase = 0.0f;

    // Transient detection
    float envFollower = 0.0f;
    float prevEnvFollower = 0.0f;
    uint32_t rng = 3141592653u;

    // ZOH decimation
    float zohHoldL = 0.0f, zohHoldR = 0.0f;
    int zohCounter = 0;

    // Diffuse reverb
    std::vector<float> diffBuf[4];
    int diffPos[4]{};
    float diffFilt[4]{};
};

} // namespace xoceanus
