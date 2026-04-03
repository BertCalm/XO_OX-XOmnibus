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
// ArtifactCathedral — Dual-engine cinematic reverb with data-rot decay.
//
// Mashup: Chase Bliss Lossy × OBNE Dark Light
//
// Two parallel FDN reverb cores:
//   Core 1 (Dark Side): Input pitch-shifted -12st before entering tank.
//   Core 2 (Sun Side): Envelope followers modulate FDN delay times (wow/flutter).
//
// Instead of traditional LP dampening in the feedback loop, uses Data Rot:
//   1. Packet Loss: Random gating zeroes 256-sample chunks in feedback path.
//   2. Bandwidth Squeeze: Bit-depth reduction (32-bit → 8-bit) in feedback.
//
// Spectral Freeze: Captures 1024-sample window, Hamming-windowed, loops
// continuously as a "dial-up modem" sustaining drone.
//
// CC44 = Packet Loss, CC45 = Spectral Freeze trigger.
//==============================================================================
class ArtifactCathedral
{
public:
    ArtifactCathedral() = default;

    void prepare(double sampleRate)
    {
        sr = static_cast<float>(sampleRate);
        float srScale = sr / 44100.0f;

        // Dark Side FDN (4-tap, pitch-shifted input)
        static constexpr int kDarkLens[4] = {1523, 1831, 2141, 2473};
        for (int i = 0; i < 4; ++i)
        {
            int len = static_cast<int>(static_cast<float>(kDarkLens[i]) * srScale) + 1;
            darkBuf[i].assign(static_cast<size_t>(len), 0.0f);
            darkPos[i] = 0;
        }

        // Sun Side FDN (4-tap, modulated delay times)
        static constexpr int kSunLens[4] = {1367, 1693, 1997, 2311};
        for (int i = 0; i < 4; ++i)
        {
            int len = static_cast<int>(static_cast<float>(kSunLens[i]) * srScale) + 1;
            sunBuf[i].assign(static_cast<size_t>(len), 0.0f);
            sunPos[i] = 0;
            sunEnvFollow[i] = 0.0f;
        }

        // Pitch shifter state (Dark Side: -12st = half speed read)
        pitchBufL.assign(static_cast<size_t>(static_cast<int>(sr * 0.05f) + 1), 0.0f);
        pitchBufR.assign(static_cast<size_t>(static_cast<int>(sr * 0.05f) + 1), 0.0f);
        pitchWritePos = 0;
        pitchReadPhase = 0.0f;

        // Data rot state
        packetLossCounter = 0;
        packetLossGate = true;
        rng = 2654435761u;

        // Spectral freeze
        freezeBuf.assign(kFreezeSize, 0.0f);
        freezeActive = false;
        freezeReadPos = 0;
        freezeGain = 0.0f;

        // Crossover state
        crossLPStateL = crossLPStateR = 0.0f;
    }

    void reset()
    {
        for (int i = 0; i < 4; ++i)
        {
            std::fill(darkBuf[i].begin(), darkBuf[i].end(), 0.0f);
            std::fill(sunBuf[i].begin(), sunBuf[i].end(), 0.0f);
            darkPos[i] = sunPos[i] = 0;
            sunEnvFollow[i] = 0.0f;
        }
        std::fill(pitchBufL.begin(), pitchBufL.end(), 0.0f);
        std::fill(pitchBufR.begin(), pitchBufR.end(), 0.0f);
        pitchWritePos = 0;
        pitchReadPhase = 0.0f;
        packetLossCounter = 0;
        packetLossGate = true;
        freezeActive = false;
        freezeReadPos = 0;
        freezeGain = 0.0f;
        crossLPStateL = crossLPStateR = 0.0f;
    }

    //--------------------------------------------------------------------------
    void processBlock(float* L, float* R, int numSamples, float packetLoss, float bitCrush, float darkMix, float sunMix,
                      float modDepth, float decayTime, bool freezeTrigger, float mix)
    {
        if (mix < 0.001f && !freezeActive)
            return;

        float feedback = 0.3f + decayTime * 0.6f; // 0.3 to 0.9

        // Packet loss parameters
        float lossProb = packetLoss * 0.3f; // 0 to 0.3 probability per chunk

        // Bit crush: reduce from 32-bit to target
        float crushBits = 32.0f - bitCrush * 24.0f; // 32 down to 8
        float crushLevels = std::pow(2.0f, crushBits);

        // Handle freeze trigger
        if (freezeTrigger && !freezeActive)
        {
            // Capture current reverb tail into freeze buffer
            for (int i = 0; i < kFreezeSize; ++i)
            {
                int darkLen = static_cast<int>(darkBuf[0].size());
                int readIdx = (darkPos[0] - kFreezeSize + i + darkLen) % darkLen;
                float sample = (darkLen > 0) ? darkBuf[0][static_cast<size_t>(readIdx)] : 0.0f;

                // Hamming window
                float w =
                    0.54f - 0.46f * std::cos(6.28318f * static_cast<float>(i) / static_cast<float>(kFreezeSize - 1));
                freezeBuf[static_cast<size_t>(i)] = sample * w;
            }
            freezeActive = true;
            freezeReadPos = 0;
            freezeGain = 1.0f;
        }
        else if (!freezeTrigger && freezeActive)
        {
            freezeGain *= 0.999f; // slow fade out
            if (freezeGain < 0.001f)
                freezeActive = false;
        }

        for (int s = 0; s < numSamples; ++s)
        {
            float dryL = L[s];
            float dryR = R[s];
            float mono = (dryL + dryR) * 0.5f;

            // ================================================================
            // Dark Side: pitch-shift input down 12st (half-speed read)
            // ================================================================
            float darkInput = 0.0f;
            if (darkMix > 0.001f)
            {
                int pbSize = static_cast<int>(pitchBufL.size());
                if (pbSize > 1)
                {
                    pitchBufL[static_cast<size_t>(pitchWritePos)] = mono;
                    pitchWritePos = (pitchWritePos + 1) % pbSize;

                    // Read at half speed = -12 semitones
                    pitchReadPhase += 0.5f;
                    if (pitchReadPhase >= static_cast<float>(pbSize))
                        pitchReadPhase -= static_cast<float>(pbSize);

                    int idx = static_cast<int>(pitchReadPhase);
                    float frac = pitchReadPhase - static_cast<float>(idx);
                    int next = (idx + 1) % pbSize;
                    darkInput = pitchBufL[static_cast<size_t>(idx)] * (1.0f - frac) +
                                pitchBufL[static_cast<size_t>(next)] * frac;
                }
            }

            // ================================================================
            // Dark Side FDN
            // ================================================================
            float darkOut = 0.0f;
            if (darkMix > 0.001f)
            {
                float tap[4];
                for (int i = 0; i < 4; ++i)
                {
                    int len = static_cast<int>(darkBuf[i].size());
                    if (len == 0)
                    {
                        tap[i] = 0.0f;
                        continue;
                    }
                    int rp = (darkPos[i] - len + 1 + len) % len;
                    tap[i] = darkBuf[i][static_cast<size_t>(rp)];
                }

                float tapSum = tap[0] + tap[1] + tap[2] + tap[3];
                for (int i = 0; i < 4; ++i)
                {
                    float fb = (tap[i] - 0.5f * tapSum) * feedback + darkInput;
                    fb = applyDataRot(fb, crushLevels, lossProb);
                    fb = flushDenormal(fastTanh(fb));
                    int len = static_cast<int>(darkBuf[i].size());
                    if (len > 0)
                        darkBuf[i][static_cast<size_t>(darkPos[i])] = fb;
                    darkPos[i] = (darkPos[i] + 1) % std::max(1, len);
                }
                darkOut = (tap[0] + tap[2]) * 0.5f * darkMix;
            }

            // ================================================================
            // Sun Side FDN (modulated delay times via envelope follower)
            // ================================================================
            float sunOut = 0.0f;
            if (sunMix > 0.001f)
            {
                // Envelope follower on input
                float envIn = std::fabs(mono);

                float tap[4];
                for (int i = 0; i < 4; ++i)
                {
                    int len = static_cast<int>(sunBuf[i].size());
                    if (len < 2)
                    {
                        tap[i] = 0.0f;
                        continue;
                    }

                    // Modulate read position based on envelope
                    float envCoeff = 1.0f - std::exp(-6.28318f * 10.0f / sr);
                    sunEnvFollow[i] += (envIn - sunEnvFollow[i]) * envCoeff;
                    sunEnvFollow[i] = flushDenormal(sunEnvFollow[i]);

                    int modOffset = static_cast<int>(sunEnvFollow[i] * modDepth * static_cast<float>(len) * 0.05f);
                    int readOff = len - 1 - modOffset;
                    readOff = std::max(1, std::min(readOff, len - 1));
                    int rp = (sunPos[i] - readOff + len) % len;
                    tap[i] = sunBuf[i][static_cast<size_t>(rp)];
                }

                float tapSum = tap[0] + tap[1] + tap[2] + tap[3];
                for (int i = 0; i < 4; ++i)
                {
                    float fb = (tap[i] - 0.5f * tapSum) * feedback + mono;
                    fb = applyDataRot(fb, crushLevels, lossProb);
                    fb = flushDenormal(fastTanh(fb));
                    int len = static_cast<int>(sunBuf[i].size());
                    if (len > 0)
                        sunBuf[i][static_cast<size_t>(sunPos[i])] = fb;
                    sunPos[i] = (sunPos[i] + 1) % std::max(1, len);
                }
                sunOut = (tap[1] + tap[3]) * 0.5f * sunMix;
            }

            // ================================================================
            // Spectral Freeze (Hamming-windowed loop)
            // ================================================================
            float freezeOut = 0.0f;
            if (freezeActive)
            {
                freezeOut = freezeBuf[static_cast<size_t>(freezeReadPos)] * freezeGain;
                freezeReadPos = (freezeReadPos + 1) % kFreezeSize;
            }

            // ================================================================
            // Mix
            // ================================================================
            float wetL = darkOut + sunOut + freezeOut;
            float wetR = -darkOut * 0.7f + sunOut * 0.7f + freezeOut; // stereo spread

            L[s] = dryL * (1.0f - mix) + wetL * mix;
            R[s] = dryR * (1.0f - mix) + wetR * mix;
        }
    }

private:
    float sr = 44100.0f;

    // --- Data Rot: packet loss + bit crush in feedback path ---
    float applyDataRot(float sample, float crushLevels, float lossProb) noexcept
    {
        // Packet loss: zero 256-sample chunks stochastically
        if (packetLossCounter <= 0)
        {
            rng = rng * 1664525u + 1013904223u;
            float roll = static_cast<float>(rng & 0xFFFF) / 65536.0f;
            packetLossGate = (roll >= lossProb);
            packetLossCounter = 256;
        }
        packetLossCounter--;

        if (!packetLossGate)
            sample = 0.0f;

        // Bandwidth squeeze: reduce bit depth
        if (crushLevels < 65000.0f) // avoid no-op at full bit depth
        {
            sample = std::round(sample * crushLevels) / crushLevels;
        }

        return sample;
    }

    // Dark Side FDN
    std::vector<float> darkBuf[4];
    int darkPos[4]{};

    // Sun Side FDN
    std::vector<float> sunBuf[4];
    int sunPos[4]{};
    float sunEnvFollow[4]{};

    // Pitch shifter (Dark Side -12st)
    std::vector<float> pitchBufL, pitchBufR;
    int pitchWritePos = 0;
    float pitchReadPhase = 0.0f;

    // Data rot state
    int packetLossCounter = 0;
    bool packetLossGate = true;
    uint32_t rng = 2654435761u;

    // Spectral freeze
    static constexpr int kFreezeSize = 1024;
    std::vector<float> freezeBuf;
    bool freezeActive = false;
    int freezeReadPos = 0;
    float freezeGain = 0.0f;

    // Crossover
    float crossLPStateL = 0.0f, crossLPStateR = 0.0f;
};

} // namespace xoceanus
