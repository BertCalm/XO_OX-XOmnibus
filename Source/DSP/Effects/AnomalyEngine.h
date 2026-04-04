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
// AnomalyEngine — Multi-textural generator + harmonic tremolo-verb + time-slip.
//
// Mashup: Chase Bliss Lost+Found × OBNE Minim
//
// A. Curiosity Engine: Karplus-Strong resonator bank driven by input envelope.
// B. Atmosphere Engine: FDN reverb with Linkwitz-Riley crossover → harmonic
//    tremolo (LFO on High/Low bands, 180° out of phase).
// C. Time-Slip: 4-second reverse buffer with 1× and 2× speed modes.
//
// CC40 = Time-Slip engage, CC41 = Texture Blend (Curiosity↔Atmosphere).
//==============================================================================
class AnomalyEngine
{
public:
    AnomalyEngine() = default;

    void prepare(double sampleRate)
    {
        sr = static_cast<float>(sampleRate);

        // Karplus-Strong resonator (tuned delay line + LP filter)
        int ksLen = static_cast<int>(sr / 60.0f) + 1; // lowest pitch ~60 Hz
        ksBufL.assign(static_cast<size_t>(ksLen), 0.0f);
        ksBufR.assign(static_cast<size_t>(ksLen), 0.0f);
        ksWritePos = 0;
        ksFiltL = ksFiltR = 0.0f;
        envFollower = 0.0f;

        // Atmosphere FDN (4-tap reverb)
        float srScale = sr / 44100.0f;
        static constexpr int kAtmLens[4] = {1187, 1433, 1709, 2003};
        for (int i = 0; i < 4; ++i)
        {
            int len = static_cast<int>(static_cast<float>(kAtmLens[i]) * srScale) + 1;
            atmBuf[i].assign(static_cast<size_t>(len), 0.0f);
            atmPos[i] = 0;
            atmFilt[i] = 0.0f;
        }

        // Crossover state
        crossLP_L = crossLP_R = 0.0f;
        tremoloPhase = 0.0f;

        // Time-slip buffer (4 seconds)
        int slipLen = static_cast<int>(sr * 4.0f) + 1;
        slipBufL.assign(static_cast<size_t>(slipLen), 0.0f);
        slipBufR.assign(static_cast<size_t>(slipLen), 0.0f);
        slipWritePos = 0;
        slipReadPhase = 0.0f;
        slipActive = false;
        slipFadeGain = 0.0f;
    }

    void reset()
    {
        std::fill(ksBufL.begin(), ksBufL.end(), 0.0f);
        std::fill(ksBufR.begin(), ksBufR.end(), 0.0f);
        ksWritePos = 0;
        ksFiltL = ksFiltR = 0.0f;
        envFollower = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            std::fill(atmBuf[i].begin(), atmBuf[i].end(), 0.0f);
            atmPos[i] = 0;
            atmFilt[i] = 0.0f;
        }
        crossLP_L = crossLP_R = 0.0f;
        tremoloPhase = 0.0f;
        std::fill(slipBufL.begin(), slipBufL.end(), 0.0f);
        std::fill(slipBufR.begin(), slipBufR.end(), 0.0f);
        slipWritePos = 0;
        slipReadPhase = 0.0f;
        slipActive = false;
        slipFadeGain = 0.0f;
    }

    void processBlock(float* L, float* R, int numSamples, float textureBlend, float reverbSize, float tremoloRate,
                      bool timeSlip, float slipSpeed, float mix)
    {
        if (mix < 0.001f)
            return;

        float ksDecay = 0.996f;
        float ksCutoff = 1.0f - std::exp(-6.28318f * 4000.0f / sr);
        float envCoeff = 1.0f - std::exp(-6.28318f * 30.0f / sr);
        float crossCoeff = 1.0f - std::exp(-6.28318f * 800.0f / sr); // 800 Hz crossover
        float feedback = 0.3f + reverbSize * 0.6f;
        float tremoloInc = tremoloRate / sr;

        // Time-slip speed: 0=1×, 1=2× (octave up)
        float slipReadRate = (slipSpeed > 0.5f) ? -2.0f : -1.0f;

        for (int s = 0; s < numSamples; ++s)
        {
            float dryL = L[s], dryR = R[s];
            float mono = (dryL + dryR) * 0.5f;

            // === A. Curiosity Engine: Karplus-Strong resonator ===
            float ksOut = 0.0f;
            if (textureBlend < 0.99f)
            {
                int ksBufSize = static_cast<int>(ksBufL.size());
                if (ksBufSize > 1)
                {
                    envFollower += (std::fabs(mono) - envFollower) * envCoeff;

                    // Envelope-driven excitation: inject input when envelope rises
                    float excitation = mono * envFollower * 4.0f;

                    // Read from delay + LP filter (string damping)
                    int ksDelay = std::max(1, static_cast<int>(sr / (80.0f + envFollower * 400.0f)));
                    ksDelay = std::min(ksDelay, ksBufSize - 1);
                    int ksReadPos = (ksWritePos - ksDelay + ksBufSize) % ksBufSize;

                    float ksRead = ksBufL[static_cast<size_t>(ksReadPos)];
                    ksFiltL += (ksRead - ksFiltL) * ksCutoff;
                    ksFiltL = flushDenormal(ksFiltL);

                    float ksWrite = excitation + ksFiltL * ksDecay;
                    ksBufL[static_cast<size_t>(ksWritePos)] = flushDenormal(ksWrite);
                    ksWritePos = (ksWritePos + 1) % ksBufSize;

                    ksOut = ksFiltL;
                }
            }

            // === B. Atmosphere Engine: FDN + Harmonic Tremolo ===
            float atmOut = 0.0f;
            if (textureBlend > 0.01f)
            {
                float tap[4];
                for (int i = 0; i < 4; ++i)
                {
                    int len = static_cast<int>(atmBuf[i].size());
                    if (len == 0)
                    {
                        tap[i] = 0.0f;
                        continue;
                    }
                    int rp = (atmPos[i] - len + 1 + len) % len;
                    tap[i] = atmBuf[i][static_cast<size_t>(rp)];
                    atmFilt[i] = flushDenormal(atmFilt[i] + (tap[i] - atmFilt[i]) * 0.4f);
                    tap[i] = atmFilt[i];
                }
                float tapSum = tap[0] + tap[1] + tap[2] + tap[3];
                for (int i = 0; i < 4; ++i)
                {
                    float fb = fastTanh((tap[i] - 0.5f * tapSum) * feedback + mono);
                    int len = static_cast<int>(atmBuf[i].size());
                    if (len > 0)
                        atmBuf[i][static_cast<size_t>(atmPos[i])] = flushDenormal(fb);
                    atmPos[i] = (atmPos[i] + 1) % std::max(1, len);
                }

                float rawWet = (tap[0] + tap[1] + tap[2] + tap[3]) * 0.25f;

                // Linkwitz-Riley crossover → harmonic tremolo (180° out of phase)
                crossLP_L += (rawWet - crossLP_L) * crossCoeff;
                crossLP_L = flushDenormal(crossLP_L);
                float lowBand = crossLP_L;
                float highBand = rawWet - lowBand;

                float tremLFO = fastSin(tremoloPhase * 6.28318f);
                tremoloPhase += tremoloInc;
                if (tremoloPhase >= 1.0f)
                    tremoloPhase -= 1.0f;

                // 180° out of phase: low gets +LFO, high gets -LFO
                float tremLow = lowBand * (0.5f + tremLFO * 0.5f);
                float tremHigh = highBand * (0.5f - tremLFO * 0.5f);
                atmOut = tremLow + tremHigh;
            }

            // Blend Curiosity ↔ Atmosphere
            float textureOut = ksOut * (1.0f - textureBlend) + atmOut * textureBlend;

            // === C. Time-Slip: reverse buffer ===
            int slipBufSize = static_cast<int>(slipBufL.size());
            if (slipBufSize > 1)
            {
                slipBufL[static_cast<size_t>(slipWritePos)] = textureOut;
                slipBufR[static_cast<size_t>(slipWritePos)] = textureOut;
                slipWritePos = (slipWritePos + 1) % slipBufSize;
            }

            float slipOut = 0.0f;
            if (timeSlip)
            {
                if (!slipActive)
                {
                    slipActive = true;
                    slipReadPhase = static_cast<float>(slipWritePos);
                    slipFadeGain = 0.0f;
                }
                slipFadeGain = std::min(1.0f, slipFadeGain + 0.001f);

                slipReadPhase += slipReadRate;
                if (slipReadPhase < 0.0f)
                    slipReadPhase += static_cast<float>(slipBufSize);
                if (slipReadPhase >= static_cast<float>(slipBufSize))
                    slipReadPhase -= static_cast<float>(slipBufSize);

                int idx = static_cast<int>(slipReadPhase);
                float frac = slipReadPhase - static_cast<float>(idx);
                int next = (idx + 1) % slipBufSize;
                slipOut =
                    (slipBufL[static_cast<size_t>(idx)] * (1.0f - frac) + slipBufL[static_cast<size_t>(next)] * frac) *
                    slipFadeGain;
            }
            else
            {
                if (slipActive)
                {
                    slipFadeGain -= 0.002f;
                    if (slipFadeGain <= 0.0f)
                    {
                        slipActive = false;
                        slipFadeGain = 0.0f;
                    }
                    else
                    {
                        int idx = static_cast<int>(slipReadPhase);
                        slipOut = slipBufL[static_cast<size_t>(idx % slipBufSize)] * slipFadeGain;
                        slipReadPhase += slipReadRate;
                        if (slipReadPhase < 0.0f)
                            slipReadPhase += static_cast<float>(slipBufSize);
                    }
                }
            }

            float finalOut = textureOut + slipOut;

            L[s] = dryL * (1.0f - mix) + finalOut * mix;
            R[s] = dryR * (1.0f - mix) + finalOut * mix * 0.8f + slipOut * 0.2f;
        }
    }

private:
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)

    // Karplus-Strong
    std::vector<float> ksBufL, ksBufR;
    int ksWritePos = 0;
    float ksFiltL = 0.0f, ksFiltR = 0.0f;
    float envFollower = 0.0f;

    // Atmosphere FDN
    std::vector<float> atmBuf[4];
    int atmPos[4]{};
    float atmFilt[4]{};
    float crossLP_L = 0.0f, crossLP_R = 0.0f;
    float tremoloPhase = 0.0f;

    // Time-slip
    std::vector<float> slipBufL, slipBufR;
    int slipWritePos = 0;
    float slipReadPhase = 0.0f;
    bool slipActive = false;
    float slipFadeGain = 0.0f;
};

} // namespace xoceanus
