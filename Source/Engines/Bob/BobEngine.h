// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/ModMatrix.h"
#include "../../DSP/VoiceAllocator.h"
#include <array>
#include <cmath>
#include <vector>
#include <cstdint>

namespace xoceanus
{

//==============================================================================
// BobNoiseGen — xorshift32 PRNG (replaces juce::Random for RT-safe noise).
//==============================================================================
class BobNoiseGen
{
public:
    void seed(uint32_t s) noexcept { state = s ? s : 1; }

    float process() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float>(static_cast<int32_t>(state)) / 2147483648.0f;
    }

    float nextFloat() noexcept { return (process() + 1.0f) * 0.5f; }

private:
    uint32_t state = 1;
};

//==============================================================================
// BobSineTable — 2048-sample sine lookup table for Oblong Sine and Sub.
// Linear interpolation. Phase input [0, 1). Thread-safe static init.
//==============================================================================
class BobSineTable
{
public:
    static constexpr int kSize = 2048;

    static float lookup(double phase) noexcept
    {
        const auto& t = getTable();
        double idx = phase * static_cast<double>(kSize);
        int iLo = static_cast<int>(idx);
        float frac = static_cast<float>(idx - static_cast<double>(iLo));
        if (iLo < 0)
            iLo = 0;
        if (iLo >= kSize)
            iLo = kSize - 1;
        return t[static_cast<size_t>(iLo)] + frac * (t[static_cast<size_t>(iLo + 1)] - t[static_cast<size_t>(iLo)]);
    }

private:
    static const std::array<float, kSize + 1>& getTable() noexcept
    {
        static const auto table = []()
        {
            constexpr double twoPi = 6.283185307179586;
            std::array<float, kSize + 1> t{};
            for (int i = 0; i <= kSize; ++i)
                t[static_cast<size_t>(i)] =
                    static_cast<float>(std::sin(static_cast<double>(i) / static_cast<double>(kSize) * twoPi));
            return t;
        }();
        return table;
    }
};

//==============================================================================
// BobOscA — Oblong Core oscillator with 4 waveforms:
//   0: Oblong Sine (sine + 2nd harmonic warmth)
//   1: Soft Triangle (leaky integrator rounding)
//   2: Velvet Saw (polyBLEP with gentle rolloff)
//   3: Cushion Pulse (polyBLEP variable width via shape)
// Per-voice drift for analog warmth.
//==============================================================================
class BobOscA
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        phase = 0.0;
        triState = 0.0f;
        driftPhase = 0.0;
        driftSmooth = 0.0f;
        driftCurrent = 0.0f;
        driftTarget = 0.0f;
        phaseWrapped = false;
    }

    void setFrequency(float hz) noexcept
    {
        baseFreq = static_cast<double>(hz);
        updatePhaseInc();
    }

    void setTune(float semitones) noexcept
    {
        if (semitones != lastTune)
        {
            lastTune = semitones;
            tuneMul = std::pow(2.0, static_cast<double>(semitones) / 12.0);
        }
    }

    void setWave(int w) noexcept { wave = w; }
    void setShape(float s) noexcept { shape = s; }
    void setDrift(float d) noexcept { driftAmount = d; }

    bool didPhaseWrap() const noexcept { return phaseWrapped; }

    float processSample() noexcept
    {
        tickDrift();

        float out = 0.0f;
        double t = phase;

        switch (wave)
        {
        case 0:
            out = sampleOblongSine(t);
            break;
        case 1:
            out = sampleSoftTriangle(t);
            break;
        case 2:
            out = sampleVelvetSaw(t);
            break;
        case 3:
            out = sampleCushionPulse(t);
            break;
        default:
            out = sampleOblongSine(t);
            break;
        }

        phase += phaseInc;
        if (phase >= 1.0)
        {
            phase -= 1.0;
            phaseWrapped = true;
        }
        else
        {
            phaseWrapped = false;
        }

        return clamp(out, -1.0f, 1.0f);
    }

private:
    double sr = 44100.0;
    double phase = 0.0;
    double phaseInc = 0.0;
    double baseFreq = 440.0;
    double tuneMul = 1.0;
    float lastTune = -9999.0f;
    int wave = 0;
    float shape = 0.5f;
    float driftAmount = 0.1f;

    double driftPhase = 0.0;
    float driftSmooth = 0.0f;
    float driftCurrent = 0.0f;
    float driftTarget = 0.0f;
    BobNoiseGen driftRng;

    float triState = 0.0f;
    bool phaseWrapped = false;

    void updatePhaseInc() noexcept
    {
        float driftMul = (std::abs(driftSmooth) > 0.0001f) ? fastExp(driftSmooth * (0.693147f / 12.0f)) : 1.0f;
        double freq = clamp(static_cast<float>(baseFreq * tuneMul * static_cast<double>(driftMul)), 20.0f, 20000.0f);
        phaseInc = freq / sr;
    }

    void tickDrift() noexcept
    {
        if (driftAmount < 0.001f)
            return;

        driftPhase += phaseInc * 0.001;
        if (driftPhase >= 1.0)
        {
            driftPhase -= 1.0;
            driftTarget = (driftRng.process()) * driftAmount * 0.5f;
        }

        float delta = (driftTarget - driftCurrent) * 0.0001f;
        if (std::abs(delta) < 1e-9f)
            return;

        driftCurrent += delta;
        driftCurrent = flushDenormal(driftCurrent);
        driftSmooth = driftCurrent;
        updatePhaseInc();
    }

    float sampleOblongSine(double t) noexcept
    {
        float sine = BobSineTable::lookup(t);
        double t2 = t * 2.0;
        if (t2 >= 1.0)
            t2 -= 1.0;
        float harm2 = BobSineTable::lookup(t2) * 0.15f;
        return sine + harm2 * shape * 0.5f;
    }

    float sampleSoftTriangle(double t) noexcept
    {
        float tri = (t < 0.5) ? static_cast<float>(4.0 * t - 1.0) : static_cast<float>(3.0 - 4.0 * t);
        float coeff = 0.05f + shape * 0.3f;
        triState += (tri - triState) * coeff;
        triState = flushDenormal(triState);
        return triState * 0.9f;
    }

    float sampleVelvetSaw(double t) noexcept
    {
        float saw = static_cast<float>(2.0 * t - 1.0);
        saw -= polyBlepF(t, phaseInc);
        float soft = 1.0f - shape * 0.4f;
        return saw * soft;
    }

    float sampleCushionPulse(double t) noexcept
    {
        float width = 0.1f + shape * 0.8f;
        float pulse = (t < static_cast<double>(width)) ? 1.0f : -1.0f;
        pulse += polyBlepF(t, phaseInc);
        pulse -= polyBlepF(std::fmod(t + 1.0 - static_cast<double>(width), 1.0), phaseInc);
        return pulse * 0.8f;
    }

    static float polyBlepF(double t, double dt) noexcept
    {
        if (dt <= 0.0)
            return 0.0f;
        if (t < dt)
        {
            double x = t / dt;
            return static_cast<float>(2.0 * x - x * x - 1.0);
        }
        if (t > 1.0 - dt)
        {
            double x = (t - 1.0) / dt;
            return static_cast<float>(x * x + 2.0 * x + 1.0);
        }
        return 0.0f;
    }
};

//==============================================================================
// BobOscB — Companion oscillator with sync, FM, and detune.
//   0: Soft Saw, 1: Rounded Pulse, 2: Triangle, 3: Sub Harmonic (sine -1 oct)
//==============================================================================
class BobOscB
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        phase = 0.0;
        triState = 0.0f;
    }

    void setFrequency(float hz) noexcept
    {
        baseFreq = static_cast<double>(hz);
        updatePhaseInc();
    }

    void setDetune(float cents) noexcept
    {
        if (cents != lastDetune)
        {
            lastDetune = cents;
            detuneMul = std::pow(2.0, static_cast<double>(cents) / 1200.0);
            updatePhaseInc();
        }
    }

    void setWave(int w) noexcept { wave = w; }
    void setSync(bool s) noexcept { syncEnabled = s; }
    void setFM(float amt) noexcept { fmAmount = amt; }

    void syncToOscA() noexcept
    {
        if (syncEnabled)
            phase = 0.0;
    }

    float processSample(float oscAout) noexcept
    {
        float out = 0.0f;
        double t = phase;

        switch (wave)
        {
        case 0: // Soft Saw
        {
            out = static_cast<float>(2.0 * t - 1.0);
            out -= polyBlepF(t, phaseInc);
            break;
        }
        case 1: // Rounded Pulse
        {
            out = (t < 0.5) ? 1.0f : -1.0f;
            out += polyBlepF(t, phaseInc);
            out -= polyBlepF(std::fmod(t + 0.5, 1.0), phaseInc);
            break;
        }
        case 2: // Triangle
        {
            float tri = (t < 0.5) ? static_cast<float>(4.0 * t - 1.0) : static_cast<float>(3.0 - 4.0 * t);
            float coeff = 0.1f;
            triState += (tri - triState) * coeff;
            triState = flushDenormal(triState);
            out = triState;
            break;
        }
        case 3: // Sub Harmonic (sine one octave below)
            out = BobSineTable::lookup(t);
            break;
        default:
            out = static_cast<float>(2.0 * t - 1.0);
            break;
        }

        // FM from OscA
        double fmMod = static_cast<double>(oscAout * fmAmount * 0.02f);
        phase += phaseInc + fmMod;
        while (phase >= 1.0)
            phase -= 1.0;
        while (phase < 0.0)
            phase += 1.0;

        return clamp(out, -1.0f, 1.0f);
    }

private:
    double sr = 44100.0;
    double phase = 0.0;
    double phaseInc = 0.0;
    double baseFreq = 440.0;
    double detuneMul = 1.0;
    float lastDetune = -9999.0f;
    int wave = 0;
    bool syncEnabled = false;
    float fmAmount = 0.0f;
    float triState = 0.0f;

    void updatePhaseInc() noexcept
    {
        double freq = baseFreq * detuneMul;
        freq = clamp(static_cast<float>(freq), 20.0f, 20000.0f);
        phaseInc = freq / sr;
    }

    static float polyBlepF(double t, double dt) noexcept
    {
        if (dt <= 0.0)
            return 0.0f;
        if (t < dt)
        {
            double x = t / dt;
            return static_cast<float>(2.0 * x - x * x - 1.0);
        }
        if (t > 1.0 - dt)
        {
            double x = (t - 1.0) / dt;
            return static_cast<float>(x * x + 2.0 * x + 1.0);
        }
        return 0.0f;
    }
};

//==============================================================================
// BobTextureOsc — Stereo noise/texture layer with 4 modes:
//   0: Dust (sparse impulses), 1: Blanket (filtered warm noise),
//   2: Static (bright noise), 3: Breath (pitch-tracked band-pass noise)
//==============================================================================
class BobTextureOsc
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        lpStateL = lpStateR = hpStateL = hpStateR = 0.0f;
        dustTimer = 0.0f;
    }

    void setMode(int m) noexcept { mode = m; }
    void setTone(float t) noexcept
    {
        tone = clamp(t, 0.0f, 1.0f);
        updateFilters();
    }
    void setWidth(float w) noexcept { width = clamp(w, 0.0f, 1.0f); }
    void setFrequency(float hz) noexcept { noteFreq = hz; }

    void processSample(float level, float& outL, float& outR) noexcept
    {
        if (level < 0.001f)
        {
            outL = outR = 0.0f;
            return;
        }

        float rawL = rndL.process();
        float rawR = rndR.process();

        // Stereo decorrelation via width
        float mid = (rawL + rawR) * 0.5f;
        float side = (rawL - rawR) * 0.5f;
        rawL = mid + side * width;
        rawR = mid - side * width;

        float sL = 0.0f, sR = 0.0f;

        switch (mode)
        {
        case 0: // Dust — sparse impulses
        {
            dustTimer += 1.0f / static_cast<float>(sr);
            float rate = 0.002f + tone * 0.05f;
            if (dustTimer > rate)
            {
                dustTimer = 0.0f;
                sL = rawL * 3.0f;
                sR = rawR * 3.0f;
            }
            break;
        }
        case 1: // Blanket — warm filtered noise
        {
            lpStateL += lpCoeff * (rawL - lpStateL);
            lpStateR += lpCoeff * (rawR - lpStateR);
            lpStateL = flushDenormal(lpStateL);
            lpStateR = flushDenormal(lpStateR);
            sL = lpStateL;
            sR = lpStateR;
            break;
        }
        case 2: // Static — bright noise
        {
            sL = rawL;
            sR = rawR;
            break;
        }
        case 3: // Breath — pitch-tracked BP noise
        {
            float bpCoeff = clamp(noteFreq / static_cast<float>(sr) * 6.28f, 0.001f, 0.5f);
            lpStateL += bpCoeff * (rawL - lpStateL);
            lpStateR += bpCoeff * (rawR - lpStateR);
            hpStateL += 0.001f * (lpStateL - hpStateL);
            hpStateR += 0.001f * (lpStateR - hpStateR);
            lpStateL = flushDenormal(lpStateL);
            lpStateR = flushDenormal(lpStateR);
            hpStateL = flushDenormal(hpStateL);
            hpStateR = flushDenormal(hpStateR);
            sL = lpStateL - hpStateL;
            sR = lpStateR - hpStateR;
            break;
        }
        }

        outL = sL * level;
        outR = sR * level;
    }

private:
    double sr = 44100.0;
    int mode = 1;
    float tone = 0.5f;
    float width = 0.5f;
    float noteFreq = 440.0f;

    float lpStateL = 0.0f, lpStateR = 0.0f;
    float hpStateL = 0.0f, hpStateR = 0.0f;
    float lpCoeff = 0.3f;
    float dustTimer = 0.0f;

    BobNoiseGen rndL, rndR;

    void updateFilters() noexcept { lpCoeff = clamp(0.01f + tone * 0.5f, 0.01f, 0.5f); }
};

//==============================================================================
// BobSnoutFilter — Character filter with 4 modes:
//   0: Snout LP (warm), 1: Snout BP (nasal), 2: Snout Form (dual formant),
//   3: Snout Soft (gentle 1-pole)
// TPT SVF with bilinear pre-warp approximation, character saturation, drive.
//==============================================================================
class BobSnoutFilter
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        invSR = 1.0f / static_cast<float>(sr);
        reset();
    }

    void reset() noexcept
    {
        s1 = s2 = s1b = s2b = lpState = 0.0f;
        coeffDirty = true;
    }

    void setMode(int m) noexcept
    {
        if (m != mode)
        {
            mode = m;
            reset();
        }
    }

    void setCutoff(float freqHz) noexcept
    {
        float clamped = clamp(freqHz, 20.0f, static_cast<float>(sr) * 0.45f);
        // Semitone-relative threshold (~0.2% ≈ 3.5 cents): flat Hz guard is defeated by
        // envelopes sweeping large frequency ranges, so we compare as a ratio instead.
        if (std::abs(clamped / cutoffHz - 1.0f) < 0.002f)
            return;
        cutoffHz = clamped;
        coeffDirty = true;
        updateCoefficients();
    }

    void setResonance(float q) noexcept
    {
        float clamped = clamp(q, 0.0f, 0.98f);
        if (clamped == resonance)
            return;
        resonance = clamped;
        coeffDirty = true;
        updateCoefficients();
    }

    void setCharacter(float v) noexcept { character = v; }
    void setDrive(float v) noexcept { drive = v; }

    float processSample(float x) noexcept
    {
        // Pre-filter drive
        x = softClip(x, drive);

        float out = 0.0f;

        switch (mode)
        {
        case 0: // Snout LP
        {
            float res = clamp(resonance, 0.0f, 0.98f);
            float r2 = 2.0f * (1.0f - res);
            float hp = (x - r2 * s1 - s2) / (1.0f + r2 * g + g * g);
            float bp = g * hp + s1;
            float lp = g * bp + s2;
            s1 = g * hp + bp;
            s2 = g * bp + lp;
            if (!std::isfinite(lp))
            {
                lp = hp = bp = 0.0f;
                s1 = s2 = 0.0f;
            }
            float resTail = softClip(bp * res * 2.0f, character * 0.6f);
            out = lp + resTail * character * 0.3f;
            break;
        }
        case 1: // Snout BP
        {
            float res = clamp(resonance, 0.0f, 0.98f);
            float r2 = 2.0f * (1.0f - res);
            float hp = (x - r2 * s1 - s2) / (1.0f + r2 * g + g * g);
            float bp = g * hp + s1;
            float lp = g * bp + s2;
            s1 = g * hp + bp;
            s2 = g * bp + lp;
            if (!std::isfinite(lp))
            {
                s1 = s2 = 0.0f;
            }
            out = bp * (1.0f + res);
            out = softClip(out, character * 0.4f);
            break;
        }
        case 2: // Snout Form (dual BP formant morph)
        {
            float r2 = R2;
            float hp1 = (x - r2 * s1 - s2) / (1.0f + r2 * g + g * g);
            float bp1 = g * hp1 + s1;
            float lp1 = g * bp1 + s2;
            s1 = g * hp1 + bp1;
            s2 = g * bp1 + lp1;
            if (!std::isfinite(lp1))
            {
                s1 = s2 = 0.0f;
                bp1 = 0.0f;
            }

            float hp2 = (x - r2 * s1b - s2b) / (1.0f + r2 * gb + gb * gb);
            float bp2 = gb * hp2 + s1b;
            float lp2 = gb * bp2 + s2b;
            s1b = gb * hp2 + bp2;
            s2b = gb * bp2 + lp2;
            if (!std::isfinite(lp2))
            {
                s1b = s2b = 0.0f;
                bp2 = 0.0f;
            }

            out = bp1 * (1.0f - character) + bp2 * character;
            out *= 0.7f;
            break;
        }
        case 3: // Snout Soft (1-pole LP)
        {
            float coeff = g / (1.0f + g);
            lpState += coeff * (x - lpState);
            lpState = flushDenormal(lpState);
            out = lpState;
            break;
        }
        default:
            out = x;
            break;
        }

        if (!std::isfinite(out))
            out = 0.0f;
        return clamp(out, -4.0f, 4.0f);
    }

private:
    double sr = 44100.0;
    float invSR = 1.0f / static_cast<float>(sr); // overwritten by prepare()
    int mode = 0;
    float cutoffHz = 8000.0f;
    float resonance = 0.3f;
    float character = 0.5f;
    float drive = 0.0f;
    bool coeffDirty = true;

    float s1 = 0.0f, s2 = 0.0f;
    float g = 0.0f, R2 = 0.0f;
    float s1b = 0.0f, s2b = 0.0f, gb = 0.0f;
    float lpState = 0.0f;

    void updateCoefficients() noexcept
    {
        if (!coeffDirty)
            return;
        coeffDirty = false;
        constexpr float pi = 3.14159265f;
        float clamped = clamp(cutoffHz, 20.0f, static_cast<float>(sr) * 0.45f);
        g = std::tan(pi * clamped * invSR);
        R2 = 2.0f * (1.0f - resonance);
        float cutoff2 = clamp(cutoffHz * 2.0f, 20.0f, static_cast<float>(sr) * 0.45f);
        gb = std::tan(pi * cutoff2 * invSR);
    }

    static float softClip(float x, float amount) noexcept
    {
        if (amount < 0.001f)
            return x;
        float gain = 1.0f + amount * 4.0f;
        x *= gain;
        float ax = std::abs(x);
        x = x / (1.0f + ax);
        return x / gain;
    }
};

//==============================================================================
// BobAdsrEnvelope — alias to the shared StandardADSR implementation.
// Drop-in replacement: same API (prepare/noteOn/noteOff/process/isActive/reset).
//==============================================================================
using BobAdsrEnvelope = StandardADSR;

//==============================================================================
// BobCuriosityLFO — Dual LFO + 5 curiosity behavior modes.
// Bhaskara-I sine approximation for RT-safe LFO.
// Modes: 0=Sniff, 1=Wander, 2=Investigate, 3=Twitch, 4=Nap
//==============================================================================
class BobCuriosityLFO
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        invSR = 1.0f / static_cast<float>(sr);
        sniffSmooth = 0.001f * 44100.0f * invSR;
        twitchSmooth = 0.001133f * 44100.0f * invSR;
        napSmooth = 0.00005f * 44100.0f * invSR;
    }

    void reset() noexcept
    {
        // Reset StandardLFO with voice-staggered start phase
        lfo1.reset(voiceOffset);
        // LFO2 micro-motion phase
        lfo2Phase = voiceOffset * 0.7f;
        curSmooth = curTarget = curTimer = twitchCool = 0.0f;
        curThreshold = 0.5f;
        snh1 = smooth1 = 0.0f;
    }

    void setVoiceOffset(float offset) noexcept { voiceOffset = offset; }
    void setLFO1Rate(float hz) noexcept
    {
        lfo1Rate = clamp(hz, 0.01f, 20.0f);
        lfo1.setRate(lfo1Rate, 1.0f / invSR);
    }
    void setLFO1Depth(float d) noexcept { lfo1Depth = clamp(d, 0.0f, 1.0f); }
    void setLFO1Shape(int s) noexcept
    {
        lfo1Shape = s;
        // Shapes 0 (Sine) and 1 (Triangle) map directly to StandardLFO.
        // Shapes 2 (S&H) and 3 (SmoothRand) are handled by custom curiosity code below.
        if (s == 0)
            lfo1.setShape(StandardLFO::Sine);
        else if (s == 1)
            lfo1.setShape(StandardLFO::Triangle);
    }
    void setCurMode(int m) noexcept { curMode = m; }
    void setCurAmount(float a) noexcept { curAmount = clamp(a, 0.0f, 1.0f); }

    struct Output
    {
        float lfo1;
        float curiosity;
    };

    Output process() noexcept
    {
        // LFO1: delegate Sine/Triangle to StandardLFO; keep S&H and SmoothRand custom
        // (they share state with the curiosity PRNG and must remain correlated).
        float l1 = 0.0f;
        if (lfo1Shape == 0 || lfo1Shape == 1)
        {
            // StandardLFO handles phase accumulation and waveform generation.
            // Rate was set via setLFO1Rate; shape was set via setLFO1Shape.
            l1 = lfo1.process() * lfo1Depth;
        }
        else
        {
            // S&H (shape 2) and SmoothRand (shape 3): manual phase accumulation
            // using the same BobNoiseGen rng to keep curiosity correlations intact.
            float prevPhase1 = lfo1ManualPhase;
            lfo1ManualPhase += lfo1Rate * invSR;
            if (lfo1ManualPhase >= 1.0f)
                lfo1ManualPhase -= 1.0f;

            if (lfo1Shape == 2) // S&H
            {
                if (prevPhase1 > lfo1ManualPhase)
                    snh1 = rng.process();
                l1 = snh1 * lfo1Depth;
            }
            else // SmoothRand (shape 3)
            {
                if (prevPhase1 > lfo1ManualPhase)
                    snh1 = rng.process();
                smooth1 += (snh1 - smooth1) * 0.0005f;
                smooth1 = flushDenormal(smooth1);
                l1 = smooth1 * lfo1Depth;
            }
        }

        // LFO2 (micro motion, always smooth random, very slow)
        float prevPhase2 = lfo2Phase;
        lfo2Phase += 0.05f * invSR;
        if (lfo2Phase >= 1.0f)
            lfo2Phase -= 1.0f;
        float l2 = 0.0f;
        if (prevPhase2 > lfo2Phase)
            snh2 = rng.process();
        smooth2 += (snh2 - smooth2) * 0.0005f;
        smooth2 = flushDenormal(smooth2);
        l2 = smooth2 * 0.3f;

        // Curiosity
        float c = tickCuriosity(l1, l2);

        return {l1, c};
    }

private:
    double sr = 44100.0;
    float invSR = 1.0f / static_cast<float>(sr); // overwritten by prepare()
    float voiceOffset = 0.0f;

    // StandardLFO handles phase accumulation and waveform generation for
    // Sine (shape 0) and Triangle (shape 1). Rate is always kept in sync.
    StandardLFO lfo1;
    float lfo1Rate = 1.0f, lfo1Depth = 0.5f;
    int lfo1Shape = 0;

    // Manual phase accumulator retained for S&H (shape 2) and SmoothRand (shape 3),
    // which share state with the BobNoiseGen curiosity PRNG.
    float lfo1ManualPhase = 0.0f;

    float lfo2Phase = 0.0f;
    float snh2 = 0.0f, smooth2 = 0.0f;

    int curMode = 0;
    float curAmount = 0.5f;
    float curSmooth = 0.0f, curTarget = 0.0f;
    float curTimer = 0.0f, curThreshold = 0.5f;
    float twitchCool = 0.0f;

    float sniffSmooth = 0.001f;
    float twitchSmooth = 0.001133f;
    float napSmooth = 0.00005f;

    BobNoiseGen rng;
    float snh1 = 0.0f, smooth1 = 0.0f;

    float tickCuriosity(float lfo1out, float lfo2out) noexcept
    {
        float out = 0.0f;
        switch (curMode)
        {
        case 0: // Sniff
            curTimer += invSR;
            if (curTimer > curThreshold)
            {
                curTimer = 0.0f;
                curThreshold = 0.5f + rng.nextFloat();
                curTarget = rng.process() * curAmount;
            }
            curSmooth += (curTarget - curSmooth) * sniffSmooth;
            curSmooth = flushDenormal(curSmooth);
            out = curSmooth;
            break;
        case 1: // Wander
            out = lfo2out * curAmount;
            break;
        case 2: // Investigate
            out = (lfo1out * 0.6f + lfo2out * 0.4f) * curAmount;
            break;
        case 3: // Twitch
            twitchCool -= invSR;
            if (twitchCool <= 0.0f)
            {
                curTarget = rng.process() * curAmount;
                twitchCool = 0.3f + rng.nextFloat() * 2.0f;
            }
            curSmooth += (curTarget - curSmooth) * twitchSmooth;
            curSmooth = flushDenormal(curSmooth);
            out = curSmooth;
            break;
        case 4: // Nap
            curTimer += invSR;
            if (curTimer > curThreshold)
            {
                curTimer = 0.0f;
                curThreshold = 2.0f + rng.nextFloat() * 4.0f;
                curTarget = rng.process() * curAmount * 0.3f;
            }
            curSmooth += (curTarget - curSmooth) * napSmooth;
            curSmooth = flushDenormal(curSmooth);
            out = curSmooth;
            break;
        }
        return clamp(out, -1.0f, 1.0f);
    }
};

//==============================================================================
// BobMode — Master macro. Single knob [0..1] fans out to 5 targets
// via S-curve and acceleration curves.
//==============================================================================
struct BobModeTargets
{
    float oscDrift = 0.0f;
    float filterChar = 0.0f;
    float texLevel = 0.0f;
    float modDepth = 0.0f;
    float fxDepth = 0.0f;
};

inline BobModeTargets computeBobMode(float value) noexcept
{
    float v = clamp(value, 0.0f, 1.0f);
    float vs = v * v * (3.0f - 2.0f * v); // smoothstep S-curve
    float v2 = vs * vs;

    BobModeTargets t;
    t.oscDrift = v2 * 0.8f;
    t.filterChar = vs * 0.9f;
    float texStart = std::max(0.0f, (v - 0.5f) / 0.5f);
    t.texLevel = texStart * texStart * (3.0f - 2.0f * texStart) * 0.25f;
    t.modDepth = vs * 0.9f;
    float fxStart = std::max(0.0f, (v - 0.2f) / 0.8f);
    t.fxDepth = fxStart * fxStart * (3.0f - 2.0f * fxStart) * 0.85f;
    return t;
}

//==============================================================================
// BobDustTape — Simplified tape color: saturation + HF rolloff.
//==============================================================================
class BobDustTape
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        lpState = 0.0f;
    }

    void reset() noexcept { lpState = 0.0f; }

    void setTone(float tone) noexcept
    {
        if (tone != lastTone)
        {
            lastTone = tone;
            float cutoff = 2000.0f + tone * 16000.0f;
            cachedCoeff = clamp(cutoff / static_cast<float>(sr) * 6.28f, 0.01f, 0.99f);
        }
    }

    float process(float input, float amount) noexcept
    {
        if (amount < 0.001f)
            return input;

        // Soft saturation (even harmonics — tape character)
        float driven = input * (1.0f + amount * 3.0f);
        float sat = driven / (1.0f + std::abs(driven));

        // HF rolloff (tape head response)
        lpState += cachedCoeff * (sat - lpState);
        lpState = flushDenormal(lpState);

        return lerp(input, lpState, amount);
    }

private:
    double sr = 44100.0;
    float lpState = 0.0f;
    float lastTone = -1.0f;
    float cachedCoeff = 0.5f;
};

//==============================================================================
// BobVoice — Per-voice state for the OBLONG engine.
//==============================================================================
struct BobVoice
{
    bool active = false;
    int noteNumber = 60;
    float velocity = 0.0f;
    uint64_t age = 0;
    uint64_t startTime = 0; // wall-clock sample count at note-on; used by VoiceAllocator

    BobOscA oscA;
    BobOscB oscB;
    BobTextureOsc texture;
    BobSnoutFilter filter;
    BobAdsrEnvelope ampEnv;
    BobAdsrEnvelope motionEnv;
    BobCuriosityLFO curiosity;
    BobDustTape dustTape;

    // Glide
    bool glideActive = false;
    float glideSourceFreq = 0.0f;

    // Cached per-block (avoids std::pow in inner loop)
    float cachedBaseFreq = 261.63f;

    // MPE per-voice expression state
    MPEVoiceExpression mpeExpression;
};

//==============================================================================
// BobEngine — Warm, character-driven synthesizer adapted from XOblong.
//
// Signal chain per voice:
//   OscA + OscB (w/ sync + FM) + Texture → SnoutFilter (4 modes w/ character)
//   → Amp Envelope → DustTape → Output
//
// Modulation: CuriosityEngine (LFO + 5 behavioral modes), MotionEnvelope,
//   BobMode master macro, per-voice drift.
//
// Coupling:
//   Output: envelope level (ch2), audio (ch0/ch1)
//   Input: AmpToFilter, LFOToPitch, AmpToPitch
//==============================================================================
class BobEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    //-- SynthEngine interface -------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);
        silenceGate.prepare(sampleRate, maxBlockSize);

        outputCacheL.resize(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR.resize(static_cast<size_t>(maxBlockSize), 0.0f);

        aftertouch.prepare(sampleRate);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            auto& v = voices[static_cast<size_t>(i)];
            v.active = false;
            v.oscA.prepare(sr);
            v.oscB.prepare(sr);
            v.texture.prepare(sr);
            v.texture.setMode(1);
            v.filter.prepare(sr);
            v.ampEnv.prepare(sr);
            v.motionEnv.prepare(sr);
            v.curiosity.prepare(sr);
            v.curiosity.setVoiceOffset(static_cast<float>(i) / static_cast<float>(kMaxVoices));
            v.dustTape.prepare(sr);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.oscA.reset();
            v.oscB.reset();
            v.texture.reset();
            v.filter.reset();
            v.ampEnv.reset();
            v.motionEnv.reset();
            v.curiosity.reset();
            v.dustTape.reset();
            v.glideActive = false;
        }
        envelopeOutput = 0.0f;
        externalPitchMod = 0.0f;
        externalFilterMod = 0.0f;
        std::fill(outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill(outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0)
            return;

        // --- ParamSnapshot ---
        const int oscA_wave = (pOscA_wave != nullptr) ? static_cast<int>(pOscA_wave->load()) : 0;
        const float oscA_shape = (pOscA_shape != nullptr) ? pOscA_shape->load() : 0.5f;
        const float oscA_tune = (pOscA_tune != nullptr) ? pOscA_tune->load() : 0.0f;
        const float oscA_drift = (pOscA_drift != nullptr) ? pOscA_drift->load() : 0.1f;

        const int oscB_wave = (pOscB_wave != nullptr) ? static_cast<int>(pOscB_wave->load()) : 0;
        const float oscB_detune = (pOscB_detune != nullptr) ? pOscB_detune->load() : 0.0f;
        const float oscB_blend = (pOscB_blend != nullptr) ? pOscB_blend->load() : 0.5f;
        const int oscB_sync = (pOscB_sync != nullptr) ? static_cast<int>(pOscB_sync->load()) : 0;
        const float oscB_fm = (pOscB_fm != nullptr) ? pOscB_fm->load() : 0.0f;

        const int texMode = (pTexMode != nullptr) ? static_cast<int>(pTexMode->load()) : 1;
        const float texLevel = (pTexLevel != nullptr) ? pTexLevel->load() : 0.0f;
        const float texTone = (pTexTone != nullptr) ? pTexTone->load() : 0.5f;
        const float texWidth = (pTexWidth != nullptr) ? pTexWidth->load() : 0.5f;

        const int fltMode = (pFltMode != nullptr) ? static_cast<int>(pFltMode->load()) : 0;
        const float fltCutoff = (pFltCutoff != nullptr) ? pFltCutoff->load() : 8000.0f;
        const float fltReso = (pFltReso != nullptr) ? pFltReso->load() : 0.3f;
        const float fltChar = (pFltChar != nullptr) ? pFltChar->load() : 0.5f;
        const float fltDrive = (pFltDrive != nullptr) ? pFltDrive->load() : 0.0f;
        const float fltEnvAmt = (pFltEnvAmt != nullptr) ? pFltEnvAmt->load() : 0.0f;

        const float ampA = (pAmpAttack != nullptr) ? pAmpAttack->load() : 0.01f;
        const float ampD = (pAmpDecay != nullptr) ? pAmpDecay->load() : 0.3f;
        const float ampS = (pAmpSustain != nullptr) ? pAmpSustain->load() : 0.7f;
        const float ampR = (pAmpRelease != nullptr) ? pAmpRelease->load() : 0.5f;

        const float motA = (pMotAttack != nullptr) ? pMotAttack->load() : 0.05f;
        const float motD = (pMotDecay != nullptr) ? pMotDecay->load() : 0.5f;
        const float motS = (pMotSustain != nullptr) ? pMotSustain->load() : 0.5f;
        const float motR = (pMotRelease != nullptr) ? pMotRelease->load() : 0.8f;
        const float motDepth = (pMotDepth != nullptr) ? pMotDepth->load() : 0.5f;

        const float lfo1Rate = (pLfo1Rate != nullptr) ? pLfo1Rate->load() : 1.0f;
        const float lfo1Depth = (pLfo1Depth != nullptr) ? pLfo1Depth->load() : 0.3f;
        const int lfo1Shape = (pLfo1Shape != nullptr) ? static_cast<int>(pLfo1Shape->load()) : 0;
        const int lfo1Target = (pLfo1Target != nullptr) ? static_cast<int>(pLfo1Target->load()) : 0;

        const int curMode = (pCurMode != nullptr) ? static_cast<int>(pCurMode->load()) : 0;
        const float curAmount = (pCurAmount != nullptr) ? pCurAmount->load() : 0.5f;

        const float bobModeVal = (pBobMode != nullptr) ? pBobMode->load() : 0.0f;

        const float dustAmount = (pDustAmount != nullptr) ? pDustAmount->load() : 0.0f;
        const float dustTone = (pDustTone != nullptr) ? pDustTone->load() : 0.5f;

        const float level = (pLevel != nullptr) ? pLevel->load() : 0.8f;

        const int voiceMode = (pVoiceMode != nullptr) ? static_cast<int>(pVoiceMode->load()) : 0;
        const float glideAmt = (pGlide != nullptr) ? pGlide->load() : 0.0f;
        const int maxPoly = (pPolyphony != nullptr) ? (1 << std::min(3, static_cast<int>(pPolyphony->load()))) : 8;

        // M1–M4 macro reads (0–1 range)
        const float macroCharacter = (pMacroCharacter != nullptr) ? pMacroCharacter->load() : 0.0f;
        const float macroMovement = (pMacroMovement != nullptr) ? pMacroMovement->load() : 0.0f;
        const float macroCoupling = (pMacroCoupling != nullptr) ? pMacroCoupling->load() : 0.0f;
        const float macroSpace = (pMacroSpace != nullptr) ? pMacroSpace->load() : 0.0f;

        // D006: smooth aftertouch pressure and compute modulation value
        aftertouch.updateBlock(numSamples);
        const float atPressure = aftertouch.getSmoothedPressure(0);

        // D002 mod matrix — apply per-block.
        // Destinations: 0=Off, 1=FilterCutoff, 2=LFO1Rate, 3=Pitch, 4=AmpLevel
        {
            ModMatrix<4>::Sources mSrc;
            mSrc.lfo1       = 0.0f;
            mSrc.lfo2       = 0.0f;
            mSrc.env        = 0.0f;
            mSrc.velocity   = 0.0f;
            mSrc.keyTrack   = 0.0f;
            mSrc.modWheel   = modWheelAmount;
            mSrc.aftertouch = atPressure;
            float mDst[5]   = {};
            modMatrix.apply(mSrc, mDst);
            bobModCutoffOffset  = mDst[1] * 5000.0f;
            bobModLfo1RateOffset = mDst[2] * 8.0f;
            bobModPitchOffset   = mDst[3] * 12.0f;
            bobModLevelOffset   = mDst[4] * 0.5f;
        }

        // Compute BobMode macro targets
        BobModeTargets bob = computeBobMode(bobModeVal);

        // Effective values (user params + bob macro additive + M1–M4 macro additive)
        // CHARACTER sweeps filter cutoff up to +6000 Hz on top of the user value; D002 mod matrix adds further offset
        const float effFltCutoff = clamp(fltCutoff + macroCharacter * 6000.0f + bobModCutoffOffset, 20.0f, 20000.0f);
        // MOVEMENT boosts LFO1 rate (×1 at 0, ×4 at full); D002 mod matrix adds further offset
        const float effLfo1Rate = clamp(lfo1Rate * (1.0f + macroMovement * 3.0f) + bobModLfo1RateOffset, 0.01f, 20.0f);
        // COUPLING offsets the outgoing send level (+0 to +0.5) — used downstream by coupling bus
        const float effCouplingLevel = clamp(macroCoupling * 0.5f, 0.0f, 1.0f);
        // SPACE adds texture level for room/space character
        const float effMacroSpaceTex = macroSpace * 0.5f;

        float effDrift = clamp(oscA_drift + bob.oscDrift, 0.0f, 1.0f);
        // D006: aftertouch adds up to +0.3 filter character/warmth (sensitivity 0.3)
        float effChar = clamp(fltChar + bob.filterChar + atPressure * 0.3f, 0.0f, 1.0f);
        float effTexLevel = clamp(texLevel + bob.texLevel + effMacroSpaceTex, 0.0f, 1.0f);
        float effLfoDepth = clamp(lfo1Depth + bob.modDepth * 0.3f, 0.0f, 1.0f);
        float effDustAmt = clamp(dustAmount + bob.fxDepth * 0.3f, 0.0f, 1.0f);

        // Suppress unused-variable warning when coupling bus routing is not yet wired
        (void)effCouplingLevel;

        // --- Process MIDI ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), msg.getChannel(), glideAmt, voiceMode, maxPoly);
            }
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber(), msg.getChannel());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                allVoicesOff();
            // D006: channel pressure → aftertouch (applied to filter character below)
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure(msg.getChannelPressureValue() / 127.0f);
            // D006: CC#1 mod wheel → filter cutoff boost (+0–4000 Hz, opens filter progressively)
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }

        // SilenceGate: skip all DSP if engine has been silent long enough
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // --- Update per-voice MPE expression from MPEManager ---
        if (mpeManager != nullptr)
        {
            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;
                mpeManager->updateVoiceExpression(voice.mpeExpression);
            }
        }

        // Consume coupling accumulators
        float pitchMod = externalPitchMod;
        externalPitchMod = 0.0f;
        float filterMod = externalFilterMod;
        externalFilterMod = 0.0f;

        float peakEnv = 0.0f;

        // --- Pre-compute block-constant values per voice (C-1/C-2/C-3/C-4 fix) ---
        float glideCoeff = 0.0f;
        if (glideAmt > 0.0f)
        {
            float glideTimeSec = 0.005f + glideAmt * 0.495f;
            glideCoeff = 1.0f - std::exp(-1.0f / (srf * glideTimeSec));
        }

        for (auto& voice : voices)
        {
            if (!voice.active)
                continue;
            voice.ampEnv.setADSR(ampA, ampD, ampS, ampR);
            voice.motionEnv.setADSR(motA, motD, motS, motR);
            voice.cachedBaseFreq = midiToFreq(voice.noteNumber);
        }

        // --- Render voices ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;
                ++voice.age;

                // Base frequency with glide
                float baseFreq = voice.cachedBaseFreq;
                if (voice.glideActive)
                {
                    voice.glideSourceFreq += glideCoeff * (baseFreq - voice.glideSourceFreq);
                    if (std::abs(voice.glideSourceFreq - baseFreq) < 0.1f)
                    {
                        voice.glideSourceFreq = baseFreq;
                        voice.glideActive = false;
                    }
                    baseFreq = voice.glideSourceFreq;
                }

                // CuriosityEngine tick — effLfo1Rate incorporates the M2 MOVEMENT macro boost
                voice.curiosity.setLFO1Rate(effLfo1Rate);
                voice.curiosity.setLFO1Depth(effLfoDepth);
                voice.curiosity.setLFO1Shape(lfo1Shape);
                voice.curiosity.setCurMode(curMode);
                voice.curiosity.setCurAmount(curAmount);
                auto curOut = voice.curiosity.process();

                // LFO1 routing
                float lfoPitchMod = 0.0f;
                float lfoCutoffMod = 0.0f;
                float lfoShapeMod = 0.0f;
                switch (lfo1Target)
                {
                case 0:
                    lfoCutoffMod = curOut.lfo1;
                    break;
                case 1:
                    lfoPitchMod = curOut.lfo1;
                    break;
                case 2:
                    lfoShapeMod = curOut.lfo1;
                    break;
                case 3:
                    break; // FX depth — applied globally
                }

                // Curiosity modulates filter cutoff
                lfoCutoffMod += curOut.curiosity * 0.5f;

                // Apply pitch modulation (fastExp avoids std::pow per sample) + MPE + pitch bend + mod matrix
                float totalPitch =
                    lfoPitchMod * 2.0f + pitchMod + voice.mpeExpression.pitchBendSemitones + pitchBendNorm * 2.0f + bobModPitchOffset;
                float freq = baseFreq * fastExp(totalPitch * (0.693147f / 12.0f));

                // OscA
                voice.oscA.setWave(oscA_wave);
                voice.oscA.setShape(clamp(oscA_shape + lfoShapeMod * 0.5f, 0.0f, 1.0f));
                voice.oscA.setTune(oscA_tune);
                voice.oscA.setDrift(effDrift);
                voice.oscA.setFrequency(freq);
                float oscAout = voice.oscA.processSample();

                // OscB with sync/FM
                voice.oscB.setWave(oscB_wave);
                voice.oscB.setDetune(oscB_detune);
                voice.oscB.setSync(oscB_sync != 0);
                voice.oscB.setFM(oscB_fm);

                // Sub harmonic: set OscB freq to half
                if (oscB_wave == 3)
                    voice.oscB.setFrequency(freq * 0.5f);
                else
                    voice.oscB.setFrequency(freq);

                if (voice.oscA.didPhaseWrap())
                    voice.oscB.syncToOscA();

                float oscBout = voice.oscB.processSample(oscAout) * oscB_blend;

                float raw = (oscAout + oscBout) * 0.5f;

                // Texture
                float texL = 0.0f, texR = 0.0f;
                voice.texture.setMode(texMode);
                voice.texture.setTone(texTone);
                voice.texture.setWidth(texWidth);
                voice.texture.setFrequency(freq);
                voice.texture.processSample(effTexLevel, texL, texR);

                // Motion envelope
                float motEnvVal = voice.motionEnv.process();
                float motMod = (motEnvVal * 2.0f - 1.0f) * motDepth;

                // Filter — effFltCutoff incorporates the M1 CHARACTER macro offset
                float cutoffMod = effFltCutoff;
                // Motion envelope to filter — D001: velocity scales filter envelope depth
                if (std::abs(fltEnvAmt) > 0.001f)
                    cutoffMod += fltEnvAmt * motMod * voice.velocity * 5000.0f;
                // LFO to filter
                cutoffMod += lfoCutoffMod * 1000.0f;
                // External coupling
                cutoffMod += filterMod * 2000.0f;
                // D006: mod wheel opens filter — CC#1 adds up to +4000 Hz (sensitivity 4000)
                cutoffMod += modWheelAmount * 4000.0f;
                // Velocity
                cutoffMod = clamp(cutoffMod, 20.0f, 18000.0f);

                voice.filter.setMode(fltMode);
                voice.filter.setCutoff(cutoffMod);
                voice.filter.setResonance(fltReso);
                voice.filter.setCharacter(effChar);
                voice.filter.setDrive(fltDrive);
                float filtered = voice.filter.processSample(raw);

                // Amp envelope
                float envVal = voice.ampEnv.process();

                float out = filtered * envVal * voice.velocity;

                // Dust tape
                voice.dustTape.setTone(dustTone);
                out = voice.dustTape.process(out, effDustAmt);

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    voice.age = 0;
                }

                // Stereo: texture adds stereo width
                mixL += out + texL * envVal * voice.velocity;
                mixR += out + texR * envVal * voice.velocity;

                peakEnv = std::max(peakEnv, envVal);
            }

            // Apply level + soft limit (D002: mod matrix level offset)
            const float effectiveLevel = clamp(level + bobModLevelOffset, 0.05f, 1.5f);
            float outL = fastTanh(mixL * effectiveLevel);
            float outR = fastTanh(mixR * effectiveLevel);

            outputCacheL[static_cast<size_t>(sample)] = outL;
            outputCacheR[static_cast<size_t>(sample)] = outR;

            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample(0, sample, outL);
                buffer.addSample(1, sample, outR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample(0, sample, (outL + outR) * 0.5f);
            }
        }

        envelopeOutput = peakEnv;

        // SilenceGate: analyze output level for next-block bypass decision
        silenceGate.analyzeBlock(buffer.getReadPointer(0),
                                 buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr, numSamples);
    }

    //-- Coupling --------------------------------------------------------------

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0)
            return 0.0f;
        auto si = static_cast<size_t>(sampleIndex);
        if (channel == 0 && si < outputCacheL.size())
            return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size())
            return outputCacheR[si];
        if (channel == 2)
            return envelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
        case CouplingType::AmpToFilter:
            externalFilterMod += amount;
            break;
        case CouplingType::AmpToPitch:
        case CouplingType::LFOToPitch:
        case CouplingType::PitchToPitch:
            externalPitchMod += amount * 0.5f;
            break;
        default:
            break;
        }
    }

    //-- Parameters ------------------------------------------------------------

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl(params);
        return {params.begin(), params.end()};
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // --- Osc A ---
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"bob_oscA_wave", 1}, "Bob Osc A Wave",
            juce::StringArray{"Oblong Sine", "Soft Tri", "Velvet Saw", "Cushion Pulse"}, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_oscA_shape", 1}, "Bob Osc A Shape",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_oscA_tune", 1}, "Bob Osc A Tune",
                                                        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_oscA_drift", 1}, "Bob Osc A Drift",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.1f));

        // --- Osc B ---
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"bob_oscB_wave", 1}, "Bob Osc B Wave",
            juce::StringArray{"Soft Saw", "Rounded Pulse", "Triangle", "Sub Harmonic"}, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_oscB_detune", 1}, "Bob Osc B Detune",
                                                        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_oscB_blend", 1}, "Bob Osc B Blend",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"bob_oscB_sync", 1}, "Bob Osc B Sync", juce::StringArray{"Off", "On"}, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_oscB_fm", 1}, "Bob Osc B FM",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        // --- Texture ---
        params.push_back(
            std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"bob_texMode", 1}, "Bob Texture Mode",
                                                         juce::StringArray{"Dust", "Blanket", "Static", "Breath"}, 1));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_texLevel", 1}, "Bob Texture Level",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_texTone", 1}, "Bob Texture Tone",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_texWidth", 1}, "Bob Texture Width",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        // --- Filter ---
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"bob_fltMode", 1}, "Bob Filter Mode",
            juce::StringArray{"Snout LP", "Snout BP", "Snout Form", "Snout Soft"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"bob_fltCutoff", 1}, "Bob Filter Cutoff",
            juce::NormalisableRange<float>(20.0f, 18000.0f, 0.1f, 0.25f), 8000.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_fltReso", 1}, "Bob Filter Resonance",
                                                        juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.3f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_fltChar", 1}, "Bob Filter Character",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_fltDrive", 1}, "Bob Filter Drive",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_fltEnvAmt", 1}, "Bob Filter Env Amt",
                                                        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

        // --- Amp Envelope ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"bob_ampAttack", 1}, "Bob Amp Attack",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.01f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"bob_ampDecay", 1}, "Bob Amp Decay",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.3f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_ampSustain", 1}, "Bob Amp Sustain",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"bob_ampRelease", 1}, "Bob Amp Release",
            juce::NormalisableRange<float>(0.001f, 20.0f, 0.001f, 0.3f), 0.5f));

        // --- Motion Envelope ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"bob_motAttack", 1}, "Bob Motion Attack",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.05f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"bob_motDecay", 1}, "Bob Motion Decay",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.5f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_motSustain", 1}, "Bob Motion Sustain",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"bob_motRelease", 1}, "Bob Motion Release",
            juce::NormalisableRange<float>(0.001f, 20.0f, 0.001f, 0.3f), 0.8f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_motDepth", 1}, "Bob Motion Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        // --- LFO1 ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"bob_lfo1Rate", 1}, "Bob LFO1 Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f), 1.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_lfo1Depth", 1}, "Bob LFO1 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"bob_lfo1Shape", 1}, "Bob LFO1 Shape",
            juce::StringArray{"Sine", "Triangle", "S&H", "SmoothRand"}, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"bob_lfo1Target", 1}, "Bob LFO1 Target",
                                                         juce::StringArray{"Filter", "Pitch", "Shape", "FX"}, 0));

        // --- Curiosity ---
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"bob_curMode", 1}, "Bob Curiosity Mode",
            juce::StringArray{"Sniff", "Wander", "Investigate", "Twitch", "Nap"}, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_curAmount", 1}, "Bob Curiosity Amount",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        // --- Bob Mode ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"bob_bobMode", 1}, "Bob Mode", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        // --- Dust Tape ---
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_dustAmount", 1}, "Bob Dust Amount",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_dustTone", 1}, "Bob Dust Tone",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        // --- Level ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"bob_level", 1}, "Bob Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        // --- Voice ---
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"bob_voiceMode", 1}, "Bob Voice Mode", juce::StringArray{"Poly", "Mono", "Legato"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"bob_glide", 1}, "Bob Glide", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"bob_polyphony", 1}, "Bob Polyphony", juce::StringArray{"1", "2", "4", "8"}, 3));

        // --- Macros (M1–M4: CHARACTER / MOVEMENT / COUPLING / SPACE) ---
        // CHARACTER → filter cutoff sweep (+0 to +6000 Hz over the user value)
        // MOVEMENT  → LFO1 rate multiplier (×1 to ×4 boost)
        // COUPLING  → coupling send amount offset (+0 to +0.5)
        // SPACE     → texture level offset (+0 to +0.5 adds room character)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_macroCharacter", 1},
                                                                     "Bob Macro CHARACTER",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_macroMovement", 1},
                                                                     "Bob Macro MOVEMENT",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_macroCoupling", 1},
                                                                     "Bob Macro COUPLING",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bob_macroSpace", 1},
                                                                     "Bob Macro SPACE",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // D002 mod matrix — 4 user-configurable source→destination slots
        static const juce::StringArray kBobModDests {"Off", "Filter Cutoff", "LFO1 Rate", "Pitch", "Amp Level"};
        ModMatrix<4>::addParameters(params, "bob_", "Bob", kBobModDests);
    }

public:
    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pOscA_wave = apvts.getRawParameterValue("bob_oscA_wave");
        pOscA_shape = apvts.getRawParameterValue("bob_oscA_shape");
        pOscA_tune = apvts.getRawParameterValue("bob_oscA_tune");
        pOscA_drift = apvts.getRawParameterValue("bob_oscA_drift");
        pOscB_wave = apvts.getRawParameterValue("bob_oscB_wave");
        pOscB_detune = apvts.getRawParameterValue("bob_oscB_detune");
        pOscB_blend = apvts.getRawParameterValue("bob_oscB_blend");
        pOscB_sync = apvts.getRawParameterValue("bob_oscB_sync");
        pOscB_fm = apvts.getRawParameterValue("bob_oscB_fm");
        pTexMode = apvts.getRawParameterValue("bob_texMode");
        pTexLevel = apvts.getRawParameterValue("bob_texLevel");
        pTexTone = apvts.getRawParameterValue("bob_texTone");
        pTexWidth = apvts.getRawParameterValue("bob_texWidth");
        pFltMode = apvts.getRawParameterValue("bob_fltMode");
        pFltCutoff = apvts.getRawParameterValue("bob_fltCutoff");
        pFltReso = apvts.getRawParameterValue("bob_fltReso");
        pFltChar = apvts.getRawParameterValue("bob_fltChar");
        pFltDrive = apvts.getRawParameterValue("bob_fltDrive");
        pFltEnvAmt = apvts.getRawParameterValue("bob_fltEnvAmt");
        pAmpAttack = apvts.getRawParameterValue("bob_ampAttack");
        pAmpDecay = apvts.getRawParameterValue("bob_ampDecay");
        pAmpSustain = apvts.getRawParameterValue("bob_ampSustain");
        pAmpRelease = apvts.getRawParameterValue("bob_ampRelease");
        pMotAttack = apvts.getRawParameterValue("bob_motAttack");
        pMotDecay = apvts.getRawParameterValue("bob_motDecay");
        pMotSustain = apvts.getRawParameterValue("bob_motSustain");
        pMotRelease = apvts.getRawParameterValue("bob_motRelease");
        pMotDepth = apvts.getRawParameterValue("bob_motDepth");
        pLfo1Rate = apvts.getRawParameterValue("bob_lfo1Rate");
        pLfo1Depth = apvts.getRawParameterValue("bob_lfo1Depth");
        pLfo1Shape = apvts.getRawParameterValue("bob_lfo1Shape");
        pLfo1Target = apvts.getRawParameterValue("bob_lfo1Target");
        pCurMode = apvts.getRawParameterValue("bob_curMode");
        pCurAmount = apvts.getRawParameterValue("bob_curAmount");
        pBobMode = apvts.getRawParameterValue("bob_bobMode");
        pDustAmount = apvts.getRawParameterValue("bob_dustAmount");
        pDustTone = apvts.getRawParameterValue("bob_dustTone");
        pLevel = apvts.getRawParameterValue("bob_level");
        pVoiceMode = apvts.getRawParameterValue("bob_voiceMode");
        pGlide = apvts.getRawParameterValue("bob_glide");
        pPolyphony = apvts.getRawParameterValue("bob_polyphony");
        pMacroCharacter = apvts.getRawParameterValue("bob_macroCharacter");
        pMacroMovement = apvts.getRawParameterValue("bob_macroMovement");
        pMacroCoupling = apvts.getRawParameterValue("bob_macroCoupling");
        pMacroSpace = apvts.getRawParameterValue("bob_macroSpace");
        modMatrix.attachParameters(apvts, "bob_");
    }

    //-- Identity --------------------------------------------------------------

    juce::String getEngineId() const override { return "Oblong"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFE9A84A); }
    int getMaxVoices() const override { return kMaxVoices; }

private:
    SilenceGate silenceGate;

    //--------------------------------------------------------------------------
    void noteOn(int noteNumber, float velocity, int midiChannel, float glideAmt, int voiceMode, int maxPoly)
    {
        if (voiceMode == 1 || voiceMode == 2)
        {
            for (int i = 1; i < kMaxVoices; ++i)
                voices[static_cast<size_t>(i)].active = false;

            auto& v = voices[0];
            bool legatoRetrigger = !(voiceMode == 2 && v.active);

            if (v.active && glideAmt > 0.001f)
            {
                v.glideSourceFreq = midiToFreq(v.noteNumber);
                v.glideActive = true;
            }
            else
            {
                v.glideActive = false;
            }

            v.active = true;
            v.noteNumber = noteNumber;
            v.velocity = velocity;
            v.age = 0;
            v.startTime = ++noteOnCounter;

            // Initialize MPE expression for this voice's channel
            v.mpeExpression.reset();
            v.mpeExpression.midiChannel = midiChannel;
            if (mpeManager != nullptr)
                mpeManager->updateVoiceExpression(v.mpeExpression);

            if (legatoRetrigger)
            {
                v.ampEnv.noteOn();
                v.motionEnv.noteOn();
                v.curiosity.reset();
                v.oscA.reset();
                v.oscB.reset();
            }
            return;
        }

        int idx = findFreeVoice(maxPoly);
        auto& v = voices[static_cast<size_t>(idx)];

        if (v.active && glideAmt > 0.001f)
        {
            v.glideSourceFreq = midiToFreq(v.noteNumber);
            v.glideActive = true;
        }
        else
        {
            v.glideActive = false;
        }

        v.active = true;
        v.noteNumber = noteNumber;
        v.velocity = velocity;
        v.age = 0;
        v.startTime = ++noteOnCounter;

        // Initialize MPE expression for this voice's channel
        v.mpeExpression.reset();
        v.mpeExpression.midiChannel = midiChannel;
        if (mpeManager != nullptr)
            mpeManager->updateVoiceExpression(v.mpeExpression);

        v.ampEnv.noteOn();
        v.motionEnv.noteOn();
        v.curiosity.reset();
        v.oscA.reset();
        v.oscB.reset();
        v.filter.reset();
        v.texture.reset();
        v.dustTape.reset();
    }

    void noteOff(int noteNumber, int midiChannel = 0)
    {
        for (auto& v : voices)
        {
            if (v.active && v.noteNumber == noteNumber)
            {
                // In MPE mode, match by channel too
                if (midiChannel > 0 && v.mpeExpression.midiChannel > 0 && v.mpeExpression.midiChannel != midiChannel)
                    continue;

                v.ampEnv.noteOff();
                v.motionEnv.noteOff();
            }
        }
    }

    void allVoicesOff()
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.ampEnv.reset();
            v.motionEnv.reset();
            v.glideActive = false;
        }
        envelopeOutput = 0.0f;
        externalPitchMod = 0.0f;
        externalFilterMod = 0.0f;
    }

    int findFreeVoice(int maxPoly)
    {
        int poly = std::min(maxPoly, kMaxVoices);
        int idx = VoiceAllocator::findFreeVoice(voices, poly);
        if (voices[static_cast<size_t>(idx)].active)
            voices[static_cast<size_t>(idx)].ampEnv.kill();
        return idx;
    }

    static float midiToFreq(int note) noexcept
    {
        return 440.0f * fastPow2((static_cast<float>(note) - 69.0f) * (1.0f / 12.0f));
    }

    //--------------------------------------------------------------------------
    double sr = 44100.0;
    float srf = 44100.0f;
    uint64_t noteOnCounter = 0; // monotonic counter for VoiceAllocator LRU ordering
    std::array<BobVoice, kMaxVoices> voices;

    float envelopeOutput = 0.0f;
    float externalPitchMod = 0.0f;
    float externalFilterMod = 0.0f;

    // ---- D006 Aftertouch — pressure adds warmth/character on Snout filter ----
    PolyAftertouch aftertouch;

    // ---- D006 Mod wheel — CC#1 opens bob_fltCutoff progressively (+0–4000 Hz) ----
    float modWheelAmount = 0.0f;
    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range

    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS parameter pointers (41 params)
    std::atomic<float>* pOscA_wave = nullptr;
    std::atomic<float>* pOscA_shape = nullptr;
    std::atomic<float>* pOscA_tune = nullptr;
    std::atomic<float>* pOscA_drift = nullptr;
    std::atomic<float>* pOscB_wave = nullptr;
    std::atomic<float>* pOscB_detune = nullptr;
    std::atomic<float>* pOscB_blend = nullptr;
    std::atomic<float>* pOscB_sync = nullptr;
    std::atomic<float>* pOscB_fm = nullptr;
    std::atomic<float>* pTexMode = nullptr;
    std::atomic<float>* pTexLevel = nullptr;
    std::atomic<float>* pTexTone = nullptr;
    std::atomic<float>* pTexWidth = nullptr;
    std::atomic<float>* pFltMode = nullptr;
    std::atomic<float>* pFltCutoff = nullptr;
    std::atomic<float>* pFltReso = nullptr;
    std::atomic<float>* pFltChar = nullptr;
    std::atomic<float>* pFltDrive = nullptr;
    std::atomic<float>* pFltEnvAmt = nullptr;
    std::atomic<float>* pAmpAttack = nullptr;
    std::atomic<float>* pAmpDecay = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;
    std::atomic<float>* pMotAttack = nullptr;
    std::atomic<float>* pMotDecay = nullptr;
    std::atomic<float>* pMotSustain = nullptr;
    std::atomic<float>* pMotRelease = nullptr;
    std::atomic<float>* pMotDepth = nullptr;
    std::atomic<float>* pLfo1Rate = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo1Shape = nullptr;
    std::atomic<float>* pLfo1Target = nullptr;
    std::atomic<float>* pCurMode = nullptr;
    std::atomic<float>* pCurAmount = nullptr;
    std::atomic<float>* pBobMode = nullptr;
    std::atomic<float>* pDustAmount = nullptr;
    std::atomic<float>* pDustTone = nullptr;
    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide = nullptr;
    std::atomic<float>* pPolyphony = nullptr;
    std::atomic<float>* pMacroCharacter = nullptr; // bob_macroCharacter — M1: filter cutoff sweep
    std::atomic<float>* pMacroMovement = nullptr;  // bob_macroMovement  — M2: LFO1 rate boost
    std::atomic<float>* pMacroCoupling = nullptr;  // bob_macroCoupling  — M3: coupling send offset
    std::atomic<float>* pMacroSpace = nullptr;     // bob_macroSpace     — M4: texture level offset

    // D002 mod matrix — 4-slot configurable modulation routing
    ModMatrix<4> modMatrix;
    float bobModCutoffOffset   = 0.0f;
    float bobModLfo1RateOffset = 0.0f;
    float bobModPitchOffset    = 0.0f;
    float bobModLevelOffset    = 0.0f;
};

} // namespace xoceanus
