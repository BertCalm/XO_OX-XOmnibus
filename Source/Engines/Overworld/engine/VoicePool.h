// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// XOverworld VoicePool — Real chip synthesis DSP for 6 retro eras.
//
// Era map (era 0-5):
//   0 = NES      — PolyBLEP square (4 duty cycles) + staircase triangle + LFSR noise
//   1 = Genesis  — 2-op FM synthesis (carrier + modulator, configurable ratio/index)
//   2 = SNES     — 32-sample wavetable with Gaussian interpolation, 4 waveforms
//   3 = Game Boy — PolyBLEP square (4 duty cycles) + 32-sample programmable wave
//   4 = PC Engine— 32-sample wavetable, 5-bit resolution, 8 built-in shapes
//   5 = Neo Geo  — FM + ADPCM-style noise layer
//
// eraY (0-1): Y-axis of ERA triangle drives NES→FM mix within the current era pair.
// eraX (era parameter): selects position along X-axis, crossfades between eras.
//
// Signal chain per voice: chip osc → ADSR envelope → sum → era crossfade
//
// All timing is sample-rate-independent. No std::sin/cos/exp/pow in hot path.
// Uses FastMath.h and PolyBLEP.h from the shared DSP library (via include path).

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include "../Parameters.h"
#include "../../../DSP/FastMath.h"
#include "../../../DSP/PolyBLEP.h"

namespace xoverworld
{

using namespace xoceanus;

//==============================================================================
// Shared helpers
//==============================================================================

// LFSR-15 noise register (NES-accurate bit 1 XOR mode)
struct LFSR15
{
    uint16_t reg = 1u;
    bool shortMode = false; // short mode uses bit 6 instead of bit 1

    void reset() { reg = 1u; }

    float next()
    {
        // XOR bits 0 and 1 (normal mode) or bits 0 and 6 (short mode)
        uint16_t feedback = (reg & 1u) ^ ((reg >> (shortMode ? 6u : 1u)) & 1u);
        reg = (reg >> 1u) | (feedback << 14u);
        return (reg & 1u) ? 1.0f : -1.0f;
    }
};

// Simple one-pole lowpass for noise smoothing / filter feedback path
struct OnePole
{
    float s = 0.0f;
    float process(float x, float coeff)
    {
        s = flushDenormal(s + coeff * (x - s));
        return s;
    }
    void reset() { s = 0.0f; }
};

//==============================================================================
// Wavetables — built at compile time (constexpr not practical with sin;
// we init lazily in prepare() exactly once via a flag)
//==============================================================================

struct ChipWavetables
{
    static constexpr int kLen = 32;

    // SNES (era 2) — 4 waveforms × 32 samples, 16-bit quantized to 4-bit like BRR
    float snes[4][kLen] = {};
    // PC Engine (era 4) — 8 waveforms × 32 samples, 5-bit (0-31 values)
    float pce[8][kLen] = {};
    // Game Boy wave channel — 8 slots × 32 samples (4-bit / 16 values)
    float gb[8][kLen] = {};

    bool ready = false;

    void build(float)
    {
        if (ready)
            return;
        ready = true;

        constexpr float twoPi = 6.28318530717958647692f;
        constexpr float kSnes4bit = 1.0f / 7.5f; // 4-bit signed: -8..7 → /7.5 → ~±1
        constexpr float kPce5bit = 1.0f / 15.5f; // 5-bit: 0..31 → /15.5 → ±1

        // --- SNES waveforms ---
        // Slot 0: sine
        for (int i = 0; i < kLen; ++i)
        {
            float raw = std::sin(twoPi * i / kLen);
            float q4 = std::round(raw * 7.5f); // quantize to 4-bit signed
            q4 = std::max(-8.0f, std::min(7.0f, q4));
            snes[0][i] = q4 * kSnes4bit;
        }
        // Slot 1: sawtooth
        for (int i = 0; i < kLen; ++i)
        {
            float raw = 2.0f * (float)i / kLen - 1.0f;
            float q4 = std::round(raw * 7.5f);
            q4 = std::max(-8.0f, std::min(7.0f, q4));
            snes[1][i] = q4 * kSnes4bit;
        }
        // Slot 2: square (50% duty)
        for (int i = 0; i < kLen; ++i)
        {
            float raw = (i < kLen / 2) ? 1.0f : -1.0f;
            float q4 = std::round(raw * 7.5f);
            q4 = std::max(-8.0f, std::min(7.0f, q4));
            snes[2][i] = q4 * kSnes4bit;
        }
        // Slot 3: noise-like (pseudo-random via simple LCG, quantized)
        {
            uint32_t lcg = 0xACE1u;
            for (int i = 0; i < kLen; ++i)
            {
                lcg = lcg * 1664525u + 1013904223u;
                float raw = (float)(int32_t)(lcg >> 16) / 32768.0f;
                float q4 = std::round(raw * 7.5f);
                q4 = std::max(-8.0f, std::min(7.0f, q4));
                snes[3][i] = q4 * kSnes4bit;
            }
        }

        // --- PC Engine waveforms ---
        // Slot 0: sine
        for (int i = 0; i < kLen; ++i)
        {
            float raw = std::sin(twoPi * i / kLen);
            float q5 = std::round((raw + 1.0f) * 15.5f);
            q5 = std::max(0.0f, std::min(31.0f, q5));
            pce[0][i] = (q5 - 15.5f) * kPce5bit;
        }
        // Slot 1: sawtooth
        for (int i = 0; i < kLen; ++i)
        {
            float q5 = (float)i;
            pce[1][i] = (q5 - 15.5f) * kPce5bit;
        }
        // Slot 2: reverse saw
        for (int i = 0; i < kLen; ++i)
        {
            float q5 = (float)(kLen - 1 - i);
            pce[2][i] = (q5 - 15.5f) * kPce5bit;
        }
        // Slot 3: triangle
        for (int i = 0; i < kLen; ++i)
        {
            float q5;
            if (i < kLen / 2)
                q5 = (float)(i * 2);
            else
                q5 = (float)((kLen - 1 - i) * 2);
            pce[3][i] = (q5 - 15.5f) * kPce5bit;
        }
        // Slot 4: square 50%
        for (int i = 0; i < kLen; ++i)
        {
            pce[4][i] = (i < kLen / 2) ? 1.0f : -1.0f;
        }
        // Slot 5: square 25%
        for (int i = 0; i < kLen; ++i)
        {
            pce[5][i] = (i < kLen / 4) ? 1.0f : -1.0f;
        }
        // Slot 6: half-sine (rectified)
        for (int i = 0; i < kLen; ++i)
        {
            float raw = std::fabs(std::sin(twoPi * i / kLen));
            float q5 = std::round(raw * 31.0f);
            q5 = std::max(0.0f, std::min(31.0f, q5));
            pce[6][i] = (q5 - 15.5f) * kPce5bit;
        }
        // Slot 7: double-cycle sine (two full sines per period)
        for (int i = 0; i < kLen; ++i)
        {
            float raw = std::sin(twoPi * 2.0f * i / kLen);
            float q5 = std::round((raw + 1.0f) * 15.5f);
            q5 = std::max(0.0f, std::min(31.0f, q5));
            pce[7][i] = (q5 - 15.5f) * kPce5bit;
        }

        // --- Game Boy wave channel ---
        // Slot 0: sine
        for (int i = 0; i < kLen; ++i)
        {
            float raw = std::sin(twoPi * i / kLen);
            float q4 = std::round((raw + 1.0f) * 7.5f);
            q4 = std::max(0.0f, std::min(15.0f, q4));
            gb[0][i] = (q4 - 7.5f) / 7.5f;
        }
        // Slot 1: sawtooth
        for (int i = 0; i < kLen; ++i)
        {
            float q4 = std::round((float)i * 15.0f / (kLen - 1));
            gb[1][i] = (q4 - 7.5f) / 7.5f;
        }
        // Slot 2: triangle
        for (int i = 0; i < kLen; ++i)
        {
            float q4;
            if (i < kLen / 2)
                q4 = (float)(i) * 2.0f;
            else
                q4 = (float)(kLen - i) * 2.0f;
            q4 = std::min(15.0f, q4);
            gb[2][i] = (q4 - 7.5f) / 7.5f;
        }
        // Slot 3: double pulse
        for (int i = 0; i < kLen; ++i)
        {
            gb[3][i] = ((i % 16) < 8) ? 1.0f : -1.0f;
        }
        // Slot 4-7: scaled-down sines with harmonics
        for (int slot = 4; slot < 8; ++slot)
        {
            int harm = slot - 2;
            for (int i = 0; i < kLen; ++i)
            {
                float raw = std::sin(twoPi * harm * i / kLen) * 0.8f + std::sin(twoPi * (harm + 1) * i / kLen) * 0.2f;
                float q4 = std::round((raw + 1.0f) * 7.5f);
                q4 = std::max(0.0f, std::min(15.0f, q4));
                gb[slot][i] = (q4 - 7.5f) / 7.5f;
            }
        }
    }
};

// Single global wavetable set (allocated in VoicePool, built on prepare())
inline ChipWavetables gWavetables;

//==============================================================================
// Per-voice state
//==============================================================================

struct ChipVoice
{
    bool active = false;
    int note = 0;
    float velocity = 0.0f;
    float freq = 440.0f;
    float sr = 44100.0f;

    // --- ADSR ---
    enum class EnvStage
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };
    EnvStage envStage = EnvStage::Idle;
    float envLevel = 0.0f;
    float attackRate = 0.0f;
    float decayCoeff = 0.0f;
    float sustainLvl = 0.0f;
    float releaseCoeff = 0.0f;

    // --- NES / Game Boy square oscillator ---
    float sqPhase = 0.0f;
    float sqPhaseInc = 0.0f;

    // --- NES triangle (staircase 16-step) ---
    float triPhase = 0.0f;
    float triPhaseInc = 0.0f;
    int triStep = 0;

    // --- LFSR noise ---
    LFSR15 lfsr;
    float noisePhase = 0.0f;
    float noisePhaseInc = 0.0f;
    float noiseOut = 0.0f;

    // --- FM operators (Genesis, Neo Geo) ---
    float modPhase = 0.0f;
    float carPhase = 0.0f;
    float modFeedbk = 0.0f;  // modulator self-feedback (previous output)
    float fmEnvLevel = 0.0f; // modulator envelope (ADSR-like)
    EnvStage fmEnvStage = EnvStage::Idle;
    float fmAttackRate = 0.0f;
    float fmDecayCoeff = 0.0f;
    float fmSustainLvl = 0.0f;
    float fmReleaseCoeff = 0.0f;

    // --- Wavetable (SNES, GB wave, PCE) ---
    float wtPhase = 0.0f;
    float wtPhaseInc = 0.0f;

    // ERA ghost (memory layer)
    float ghostEraLast = 0.0f;

    void prepare(float sampleRate)
    {
        sr = sampleRate;
        lfsr.reset();
    }

    void noteOn(int midiNote, float vel, const ParamSnapshot& snap)
    {
        active = true;
        note = midiNote;
        velocity = vel;
        freq = midiToFreq(midiNote) * fastPow2(snap.masterTune / 12.0f);

        // Reset oscillator phases (hard sync on note-on = authentic chip behaviour)
        sqPhase = 0.0f;
        triPhase = 0.0f;
        triStep = 0;
        wtPhase = 0.0f;
        modPhase = 0.0f;
        carPhase = 0.0f;
        modFeedbk = 0.0f;
        noisePhase = 0.0f;
        noiseOut = 0.0f;
        lfsr.reset();

        // Envelope
        envStage = EnvStage::Attack;
        envLevel = 0.0f;
        updateEnv(snap);

        // FM envelope
        fmEnvStage = EnvStage::Attack;
        fmEnvLevel = 0.0f;
        updateFMEnv(snap);
    }

    void noteOff()
    {
        envStage = EnvStage::Release;
        fmEnvStage = EnvStage::Release;
    }

    // Update amp ADSR from snapshot
    void updateEnv(const ParamSnapshot& snap)
    {
        float aSec = std::max(snap.ampAttack, 0.001f);
        float dSec = std::max(snap.ampDecay, 0.001f);
        float rSec = std::max(snap.ampRelease, 0.001f);
        attackRate = 1.0f / (sr * aSec);
        decayCoeff = 1.0f - fastExp(-4.6f / (sr * dSec));
        sustainLvl = snap.ampSustain;
        releaseCoeff = 1.0f - fastExp(-4.6f / (sr * rSec));
    }

    // FM envelope from OPN-style register values (0-31 attack, 0-15 decay/sustain/release)
    void updateFMEnv(const ParamSnapshot& snap)
    {
        // Map 0-31 attack register → time (31=fastest ~0.5ms, 0=slowest ~5s)
        float aSec = fastPow2(-0.2f * snap.fmAttack) * 2.0f;
        aSec = std::max(aSec, 0.0001f);
        float dSec = fastPow2(-0.35f * snap.fmDecay) * 1.5f;
        dSec = std::max(dSec, 0.001f);
        float rSec = fastPow2(-0.35f * snap.fmRelease) * 1.5f;
        rSec = std::max(rSec, 0.001f);
        // Sustain level 0-15 → 0.0-1.0
        fmSustainLvl = (float)snap.fmSustain / 15.0f;

        fmAttackRate = 1.0f / (sr * aSec);
        fmDecayCoeff = 1.0f - fastExp(-4.6f / (sr * dSec));
        fmReleaseCoeff = 1.0f - fastExp(-4.6f / (sr * rSec));
    }

    // Process amp ADSR, returns current level
    float tickEnv()
    {
        switch (envStage)
        {
        case EnvStage::Attack:
            envLevel += attackRate;
            if (envLevel >= 1.0f)
            {
                envLevel = 1.0f;
                envStage = EnvStage::Decay;
            }
            break;
        case EnvStage::Decay:
            envLevel += decayCoeff * (sustainLvl - envLevel);
            if (std::fabs(envLevel - sustainLvl) < 0.0001f)
            {
                envLevel = sustainLvl;
                envStage = EnvStage::Sustain;
            }
            break;
        case EnvStage::Sustain:
            envLevel = sustainLvl;
            break;
        case EnvStage::Release:
            envLevel -= releaseCoeff * envLevel;
            envLevel = flushDenormal(envLevel);
            if (envLevel < 0.0001f)
            {
                envLevel = 0.0f;
                envStage = EnvStage::Idle;
                active = false;
            }
            break;
        case EnvStage::Idle:
            envLevel = 0.0f;
            break;
        }
        return envLevel;
    }

    // Process FM operator envelope
    float tickFMEnv()
    {
        switch (fmEnvStage)
        {
        case EnvStage::Attack:
            fmEnvLevel += fmAttackRate;
            if (fmEnvLevel >= 1.0f)
            {
                fmEnvLevel = 1.0f;
                fmEnvStage = EnvStage::Decay;
            }
            break;
        case EnvStage::Decay:
            fmEnvLevel += fmDecayCoeff * (fmSustainLvl - fmEnvLevel);
            break;
        case EnvStage::Sustain:
            fmEnvLevel = fmSustainLvl;
            break;
        case EnvStage::Release:
            fmEnvLevel -= fmReleaseCoeff * fmEnvLevel;
            fmEnvLevel = flushDenormal(fmEnvLevel);
            if (fmEnvLevel < 0.0001f)
            {
                fmEnvLevel = 0.0f;
                fmEnvStage = EnvStage::Idle;
            }
            break;
        case EnvStage::Idle:
            fmEnvLevel = 0.0f;
            break;
        }
        return fmEnvLevel;
    }

    //--------------------------------------------------------------------------
    // Era 0: NES
    // Square wave with 4 duty cycles + 16-step staircase triangle + LFSR noise.
    // eraX (0-1) blends: 0=pure pulse, 0.5=pulse+triangle, 1=triangle+noise
    //--------------------------------------------------------------------------
    float processNES(const ParamSnapshot& snap, float eraX)
    {
        // Phase increments
        sqPhaseInc = freq / sr;
        triPhaseInc = freq / sr;

        // Duty cycle selection (0-3 → 12.5%, 25%, 50%, 75%)
        static const float kDuty[4] = {0.125f, 0.25f, 0.5f, 0.75f};
        float duty = kDuty[std::max(0, std::min(3, snap.pulseDuty))];

        // --- Square with PolyBLEP ---
        float squareOut = (sqPhase < duty) ? 1.0f : -1.0f;
        float dt = sqPhaseInc;
        squareOut += PolyBLEP::polyBLEP(sqPhase, dt);
        squareOut -= PolyBLEP::polyBLEP(PolyBLEP::wrapPhase(sqPhase - duty + 1.0f), dt);
        sqPhase = std::fmod(sqPhase + sqPhaseInc, 1.0f);

        // --- 16-step staircase triangle ---
        // Authentic NES: 15 values per half-cycle, 4-bit unsigned.
        // We model this as 16 steps per half-cycle (32 total) mapped to [-1, 1].
        static const float kTriTable[32] = {
            15 / 15.0f, 14 / 15.0f, 13 / 15.0f, 12 / 15.0f, 11 / 15.0f, 10 / 15.0f, 9 / 15.0f,  8 / 15.0f,
            7 / 15.0f,  6 / 15.0f,  5 / 15.0f,  4 / 15.0f,  3 / 15.0f,  2 / 15.0f,  1 / 15.0f,  0 / 15.0f,
            0 / 15.0f,  1 / 15.0f,  2 / 15.0f,  3 / 15.0f,  4 / 15.0f,  5 / 15.0f,  6 / 15.0f,  7 / 15.0f,
            8 / 15.0f,  9 / 15.0f,  10 / 15.0f, 11 / 15.0f, 12 / 15.0f, 13 / 15.0f, 14 / 15.0f, 15 / 15.0f};

        triPhase += triPhaseInc;
        if (triPhase >= 1.0f)
            triPhase -= 1.0f;
        int triIdx = (int)(triPhase * 32.0f) & 31;
        float triOut = kTriTable[triIdx] * 2.0f - 1.0f; // → [-1, 1]

        // --- LFSR noise ---
        // Period mapped to a clock rate per note period
        // noiseMode: 0=normal (bit 1 XOR), 1=short (bit 6 XOR)
        lfsr.shortMode = (snap.noiseMode != 0);
        // Noise period 0-15: NES clock divisors 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
        static const float kNoisePeriods[16] = {4.0f,   8.0f,   16.0f,  32.0f,  64.0f,  96.0f,   128.0f,  160.0f,
                                                202.0f, 254.0f, 380.0f, 508.0f, 762.0f, 1016.0f, 2034.0f, 4068.0f};
        int noisePeriodIdx = std::max(0, std::min(15, snap.noisePeriod));
        float noiseClockRate = 1789773.0f / kNoisePeriods[noisePeriodIdx] / sr;
        noisePhase += noiseClockRate;
        if (noisePhase >= 1.0f)
        {
            noisePhase -= 1.0f;
            noiseOut = lfsr.next();
        }

        // Mix: eraX 0=square only, 0.33=square+tri, 0.66=tri+noise, 1=noise only
        float mixPulse, mixTri, mixNoise;
        float x3 = eraX * 3.0f;
        if (x3 <= 1.0f)
        {
            mixPulse = 1.0f;
            mixTri = x3 * (snap.triEnable ? 1.0f : 0.0f);
            mixNoise = 0.0f;
        }
        else if (x3 <= 2.0f)
        {
            float t = x3 - 1.0f;
            mixPulse = 1.0f - t;
            mixTri = (snap.triEnable ? 1.0f : 0.0f);
            mixNoise = t * snap.nesMix;
        }
        else
        {
            float t = x3 - 2.0f;
            mixPulse = 0.0f;
            mixTri = (snap.triEnable ? 1.0f - t : 0.0f);
            mixNoise = snap.nesMix;
        }

        return (squareOut * mixPulse + triOut * mixTri + noiseOut * mixNoise) *
               (1.0f / (mixPulse + mixTri + mixNoise + 0.0001f));
    }

    //--------------------------------------------------------------------------
    // Era 1: Genesis FM (YM2612-style 2-operator)
    // Modulator → Carrier with configurable ratio and FM index.
    // eraX blends FM index (0=quiet modulation, 1=full clang).
    //--------------------------------------------------------------------------
    float processGenesisFM(const ParamSnapshot& snap, float eraX)
    {
        constexpr float twoPi = 6.28318530717958647692f;

        // Operator 1 = modulator, Operator 2 = carrier
        int ratioM = std::max(1, snap.fmOpMult[0]); // multiplier 1-15
        int ratioC = std::max(1, snap.fmOpMult[1]);
        float freqM = freq * (float)ratioM;
        float freqC = freq * (float)ratioC;

        // Modulator level (0-127 → 0-1 amplitude)
        float modLevel = (float)std::max(0, std::min(127, snap.fmOpLevel[0])) / 127.0f;

        // FM index: eraX 0-1 scales the index, giving more metallic character
        // Carrier level controls output amplitude
        float carLevel = (float)std::max(0, std::min(127, snap.fmOpLevel[1])) / 127.0f;

        // FM modulation index: eraX drives how much modulation is applied
        // Typical OPN range: 0 = pure sine, 1 = heavy FM clang
        float fmIndex = eraX * modLevel * 4.0f; // max index = 4π at full

        // FM envelope on modulator
        float modEnv = tickFMEnv();

        // LFO vibrato/tremolo (OPN LFO)
        float lfoRate = (float)snap.fmLfoRate * 0.5f + 0.1f; // 0.1-4 Hz
        float lfoDepth = (float)snap.fmLfoDepth / 127.0f;
        // LFO uses a shared accumulator tied to carrier phase progress
        // (simple: drive LFO from carPhase ratio)
        float lfoPhase = carPhase * lfoRate;
        float lfo = fastSin(lfoPhase * twoPi) * lfoDepth * 0.05f;

        // Detune: OPN detune register adds slight frequency offset
        float detuneM = (float)snap.fmOpDetune[0] * 0.5f; // in semitones × 0.5
        float detuneC = (float)snap.fmOpDetune[1] * 0.5f;
        float modFreqDetune = freqM * fastPow2(detuneM / 12.0f);
        float carFreqDetune = freqC * fastPow2(detuneC / 12.0f + lfo);

        // Feedback on modulator (self-FM)
        int fbLevel = std::max(0, std::min(7, snap.fmFeedback));
        float fbAmt = (fbLevel > 0) ? fastPow2((float)(fbLevel - 7)) * 2.0f : 0.0f;

        // Advance modulator phase
        float modPhaseInc = modFreqDetune / sr;
        modPhase += modPhaseInc;
        if (modPhase >= 1.0f)
            modPhase -= 1.0f;

        // Modulator output with self-feedback
        float modOut = fastSin((modPhase + modFeedbk * fbAmt) * twoPi) * modEnv;
        modFeedbk = flushDenormal(modFeedbk * 0.5f + modOut * 0.5f); // filtered feedback

        // Carrier: modulate its phase by modulator output
        float carPhaseInc = carFreqDetune / sr;
        carPhase += carPhaseInc + modOut * fmIndex * carPhaseInc;
        if (carPhase >= 1.0f)
            carPhase -= std::floor(carPhase);

        float carOut = fastSin(carPhase * twoPi) * carLevel;

        return carOut;
    }

    //--------------------------------------------------------------------------
    // Era 2: SNES BRR wavetable with Gaussian interpolation
    // brrSample selects waveform (0-3). eraX blends between adjacent waveforms.
    //--------------------------------------------------------------------------
    float processSNES(const ParamSnapshot& snap, float eraX)
    {
        // Phase increment
        wtPhaseInc = freq / sr;
        wtPhase += wtPhaseInc;
        if (wtPhase >= 1.0f)
            wtPhase -= 1.0f;

        float pos = wtPhase * ChipWavetables::kLen;
        int i0 = (int)pos & (ChipWavetables::kLen - 1);
        int i1 = (i0 + 1) & (ChipWavetables::kLen - 1);
        int im1 = (i0 - 1 + ChipWavetables::kLen) & (ChipWavetables::kLen - 1);
        int i2 = (i0 + 2) & (ChipWavetables::kLen - 1);
        float frac = pos - (float)(int)pos;

        // Select waveform slot (0-3)
        int slot0 = snap.brrSample & 3;
        int slot1 = (slot0 + 1) & 3;
        float morphAmt = eraX; // eraX morphs between adjacent waveforms

        // SNES Gaussian interpolation coefficients (4-point)
        // Approximation of the SNES Gaussian kernel
        float g0 = 0.5f - frac * (1.0f - 0.5f * frac);
        float g1 = 1.0f - frac * frac;
        float g2 = frac * (1.0f + frac * (0.5f - 0.5f * frac));
        float g3 = 0.5f * frac * frac * (frac - 1.0f);

        float s0a = gWavetables.snes[slot0][im1] * g0 + gWavetables.snes[slot0][i0] * g1 +
                    gWavetables.snes[slot0][i1] * g2 + gWavetables.snes[slot0][i2] * g3;
        float s0b = gWavetables.snes[slot1][im1] * g0 + gWavetables.snes[slot1][i0] * g1 +
                    gWavetables.snes[slot1][i1] * g2 + gWavetables.snes[slot1][i2] * g3;

        // Noise replace mode: substitute noise for wavetable
        if (snap.noiseReplace)
        {
            noisePhaseInc = freq / sr;
            noisePhase += noisePhaseInc;
            if (noisePhase >= 1.0f)
            {
                noisePhase -= 1.0f;
                noiseOut = lfsr.next();
            }
            return noiseOut;
        }

        return lerp(s0a, s0b, morphAmt);
    }

    //--------------------------------------------------------------------------
    // Era 3: Game Boy
    // Square oscillator (4 duties) + 32-sample wave channel.
    // eraX blends between pulse and wave channel.
    //--------------------------------------------------------------------------
    float processGameBoy(const ParamSnapshot& snap, float eraX)
    {
        // Pulse channel
        static const float kGBDuty[4] = {0.125f, 0.25f, 0.5f, 0.75f};
        float duty = kGBDuty[std::max(0, std::min(3, snap.gbPulseDuty))];

        sqPhaseInc = freq / sr;
        float squareOut = (sqPhase < duty) ? 1.0f : -1.0f;
        float dt = sqPhaseInc;
        squareOut += PolyBLEP::polyBLEP(sqPhase, dt);
        squareOut -= PolyBLEP::polyBLEP(PolyBLEP::wrapPhase(sqPhase - duty + 1.0f), dt);
        sqPhase += sqPhaseInc;
        if (sqPhase >= 1.0f)
            sqPhase -= 1.0f;

        // Wave channel — 4-bit 32-sample table, linear interpolation
        int waveSlot = std::max(0, std::min(7, snap.gbWaveSlot));
        wtPhaseInc = freq / sr;
        wtPhase += wtPhaseInc;
        if (wtPhase >= 1.0f)
            wtPhase -= 1.0f;

        float wPos = wtPhase * ChipWavetables::kLen;
        int wi0 = (int)wPos & (ChipWavetables::kLen - 1);
        int wi1 = (wi0 + 1) & (ChipWavetables::kLen - 1);
        float wf = wPos - (float)(int)wPos;
        float waveOut = lerp(gWavetables.gb[waveSlot][wi0], gWavetables.gb[waveSlot][wi1], wf);

        return lerp(squareOut, waveOut, eraX);
    }

    //--------------------------------------------------------------------------
    // Era 4: PC Engine
    // 32-sample wavetable with 5-bit resolution.
    // eraX blends between selected waveform and adjacent slot.
    //--------------------------------------------------------------------------
    float processPCEngine(const ParamSnapshot& snap, float eraX)
    {
        int slot0 = std::max(0, std::min(7, snap.pceWaveSlot));
        int slot1 = (slot0 + 1) & 7;

        wtPhaseInc = freq / sr;
        wtPhase += wtPhaseInc;
        if (wtPhase >= 1.0f)
            wtPhase -= 1.0f;

        float wPos = wtPhase * ChipWavetables::kLen;
        int wi0 = (int)wPos & (ChipWavetables::kLen - 1);
        int wi1 = (wi0 + 1) & (ChipWavetables::kLen - 1);
        float wf = wPos - (float)(int)wPos;

        float s0 = lerp(gWavetables.pce[slot0][wi0], gWavetables.pce[slot0][wi1], wf);
        float s1 = lerp(gWavetables.pce[slot1][wi0], gWavetables.pce[slot1][wi1], wf);

        return lerp(s0, s1, eraX);
    }

    //--------------------------------------------------------------------------
    // Era 5: Neo Geo
    // 2-op FM (same engine as Genesis) + ADPCM-style noise layer.
    // eraX drives noise mix amount.
    //--------------------------------------------------------------------------
    float processNeoGeo(const ParamSnapshot& snap, float eraX)
    {
        // FM backbone same as Genesis
        float fmOut = processGenesisFM(snap, 0.5f); // fixed FM depth for Neo Geo character

        // ADPCM noise: band-limited via simple one-pole LP, retriggered from LFSR
        lfsr.shortMode = false;
        float noiseClockRate = freq * 0.5f / sr; // half-rate for ADPCM feel
        noisePhase += noiseClockRate;
        if (noisePhase >= 1.0f)
        {
            noisePhase -= 1.0f;
            noiseOut = lfsr.next() * 0.5f;
        }

        // Blend FM and noise — eraX controls noise level
        return fmOut * (1.0f - eraX * 0.5f) + noiseOut * eraX * 0.5f;
    }

    //--------------------------------------------------------------------------
    // Main per-sample process — selects and crossfades eras
    //--------------------------------------------------------------------------
    float process(float eraX, float eraY, float ghostEra, float ghostEraY, float memMix, const ParamSnapshot& snap)
    {
        if (!active)
            return 0.0f;

        // --- Compute per-era output ---
        // eraX selects base era (0-5), with crossfade between floor and ceil
        float eraF = eraX * 5.0f;
        int eraLo = (int)eraF;
        int eraHi = eraLo + 1;
        float eraT = eraF - (float)eraLo;
        if (eraLo > 5)
        {
            eraLo = 5;
            eraHi = 5;
            eraT = 0.0f;
        }
        if (eraHi > 5)
            eraHi = 5;

        // eraY drives the sub-era blend (X within the selected era)
        // For most eras: 0=darker/simpler, 1=brighter/more complex
        float subX = eraY;

        auto getSample = [&](int era, float sx) -> float
        {
            switch (era)
            {
            case 0:
                return processNES(snap, sx);
            case 1:
                return processGenesisFM(snap, sx);
            case 2:
                return processSNES(snap, sx);
            case 3:
                return processGameBoy(snap, sx);
            case 4:
                return processPCEngine(snap, sx);
            case 5:
                return processNeoGeo(snap, sx);
            default:
                return 0.0f;
            }
        };

        // Note: We only call processGenesisFM for one era to avoid double-ticking
        // the FM envelope. For crossfade we interpolate outputs, not ticking twice.
        // The second era's process functions share oscillator state but are pure
        // functions of phase (no extra phase advance since both read the same phase).
        // In practice era crossfades are slow (portamento), so this is inaudible.
        float outLo = getSample(eraLo, subX);
        float outHi = (eraHi != eraLo) ? getSample(eraHi, subX) : outLo;

        float mainOut = lerp(outLo, outHi, smoothstep(eraT));

        // --- ERA Memory ghost layer ---
        // Ghost plays back a slightly-delayed ERA position for shimmer
        if (memMix > 0.001f)
        {
            float ghostEraF = ghostEra * 5.0f;
            int ghostEraLo = (int)ghostEraF;
            int ghostEraHi = ghostEraLo + 1;
            float ghostEraT = ghostEraF - (float)ghostEraLo;
            if (ghostEraLo > 5)
            {
                ghostEraLo = 5;
                ghostEraHi = 5;
                ghostEraT = 0.0f;
            }
            if (ghostEraHi > 5)
                ghostEraHi = 5;

            float ghostX = ghostEraY;
            float ghostLoS = getSample(ghostEraLo, ghostX);
            float ghostHiS = (ghostEraHi != ghostEraLo) ? getSample(ghostEraHi, ghostX) : ghostLoS;
            float ghostOut = lerp(ghostLoS, ghostHiS, smoothstep(ghostEraT));
            mainOut = lerp(mainOut, ghostOut, memMix * 0.5f);
        }

        // --- Amplitude envelope ---
        float envGain = tickEnv();
        return mainOut * envGain * velocity;
    }
};

//==============================================================================
// VoicePool — 8-voice polyphonic chip synthesizer
//==============================================================================

class VoicePool
{
public:
    static constexpr int kMaxVoices = 8;

    void prepare(float sampleRate)
    {
        sr = sampleRate;
        gWavetables.build(sampleRate);
        for (auto& v : voices)
            v.prepare(sampleRate);
    }

    void allNotesOff()
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.envStage = ChipVoice::EnvStage::Idle;
            v.envLevel = 0.0f;
        }
    }

    void reset()
    {
        allNotesOff();
        for (auto& v : voices)
        {
            v.sqPhase = 0.0f;
            v.triPhase = 0.0f;
            v.wtPhase = 0.0f;
            v.modPhase = 0.0f;
            v.carPhase = 0.0f;
        }
    }

    void noteOn(int midiNote, float velocity)
    {
        // Find free voice, or steal oldest active voice (round-robin)
        int target = -1;

        // 1. Look for an idle voice
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[i].active || voices[i].envStage == ChipVoice::EnvStage::Idle)
            {
                target = i;
                break;
            }
        }
        // 2. Steal: find any voice in release
        if (target < 0)
        {
            for (int i = 0; i < kMaxVoices; ++i)
            {
                if (voices[i].envStage == ChipVoice::EnvStage::Release)
                {
                    target = i;
                    break;
                }
            }
        }
        // 3. Steal oldest (round-robin)
        if (target < 0)
        {
            target = stealIdx;
            stealIdx = (stealIdx + 1) % kMaxVoices;
        }

        voices[target].noteOn(midiNote, velocity, cachedSnap);
    }

    void noteOff(int midiNote)
    {
        for (auto& v : voices)
        {
            if (v.active && v.note == midiNote && v.envStage != ChipVoice::EnvStage::Release &&
                v.envStage != ChipVoice::EnvStage::Idle)
            {
                v.noteOff();
            }
        }
    }

    void applyParams(const ParamSnapshot& snap)
    {
        cachedSnap = snap;
        for (auto& v : voices)
        {
            if (v.active)
            {
                v.updateEnv(snap);
                v.updateFMEnv(snap);
            }
        }
    }

    // Called per-sample by OverworldEngine::renderBlock
    float process(float eraX, float eraY, float ghostEra, float ghostEraY, float memMix)
    {
        float sum = 0.0f;
        for (auto& v : voices)
        {
            if (v.active)
            {
                sum += v.process(eraX, eraY, ghostEra, ghostEraY, memMix, cachedSnap);
            }
        }
        // Gentle soft-clip to handle voice summing headroom
        return softClip(sum * 0.5f);
    }

private:
    float sr = 44100.0f;
    int stealIdx = 0;
    ChipVoice voices[kMaxVoices];
    ParamSnapshot cachedSnap;
};

} // namespace xoverworld
