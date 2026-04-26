// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/VoiceAllocator.h"
#include <array>
#include <cmath>
#include <vector>
#include <cstdint>

namespace xoceanus
{

//==============================================================================
// BiteNoiseGen -- xorshift32 PRNG for RT-safe noise generation.
//==============================================================================
class BiteNoiseGen
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
// BiteSineTable -- 2048-sample sine lookup for sub oscillator and FM carrier.
//==============================================================================
class BiteSineTable
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
// BiteOscA -- "Belly" oscillator: warm, low-end focused waveforms.
//   0: Sine (pure sub weight)
//   1: Triangle (soft harmonic body)
//   2: Saw (rich but warm via gentle rolloff)
//   3: Cushion Pulse (variable-width plush tone)
// Shape parameter modifies per-waveform character.
// Drift adds slow analog-style pitch instability.
//==============================================================================
class BiteOscA
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        // Precompute drift LFO phase increment — avoids a per-sample division
        driftPhaseInc = 0.37 / sr;
        reset();
    }

    void reset() noexcept
    {
        phase = 0.0;
        triState = 0.0f;
        driftPhase = 0.0;
        driftLfoState = 0.0f;
    }

    void setFrequency(float hz) noexcept
    {
        double freq = clamp(static_cast<float>(hz), 20.0f, 20000.0f);
        basePhaseInc = freq / sr;
    }

    void setWaveform(int w) noexcept { wave = w; }
    void setShape(float s) noexcept { shape = clamp(s, 0.0f, 1.0f); }
    void setDrift(float d) noexcept { driftAmount = clamp(d, 0.0f, 1.0f); }

    // Returns true when phase wraps (for hard sync detection)
    bool didPhaseWrap() const noexcept { return wrapped; }

    float processSample() noexcept
    {
        // Apply analog drift: slow random pitch variation
        float driftMod = 1.0f;
        if (driftAmount > 0.001f)
        {
            driftPhase += driftPhaseInc; // ~0.37 Hz drift LFO (precomputed in prepare)
            if (driftPhase >= 1.0)
                driftPhase -= 1.0;
            float driftRaw = BiteSineTable::lookup(driftPhase);
            driftLfoState += (driftRaw - driftLfoState) * 0.002f;
            driftLfoState = flushDenormal(driftLfoState);
            driftMod = 1.0f + driftLfoState * driftAmount * 0.003f;
        }
        double phaseInc = basePhaseInc * static_cast<double>(driftMod);

        float out = 0.0f;
        double t = phase;

        switch (wave)
        {
        case 0: // Sine -- shape controls harmonic fold-in
        {
            float raw = BiteSineTable::lookup(t);
            // Shape adds subtle odd harmonics via cubic fold-in (preserves anti-symmetry,
            // no DC offset). raw*raw*raw keeps the waveform bipolar unlike raw*raw.
            if (shape > 0.01f)
                raw = raw + shape * 0.3f * raw * raw * raw;
            out = raw;
            break;
        }

        case 1: // Triangle -- shape morphs toward saw
        {
            float tri = (t < 0.5) ? static_cast<float>(4.0 * t - 1.0) : static_cast<float>(3.0 - 4.0 * t);
            float saw = static_cast<float>(2.0 * t - 1.0);
            float morphed = lerp(tri, saw, shape);
            triState += (morphed - triState) * 0.15f;
            triState = flushDenormal(triState);
            out = triState * 0.95f;
            break;
        }

        case 2: // Saw -- shape controls warmth rolloff
        {
            out = static_cast<float>(2.0 * t - 1.0);
            out -= polyBlepD(t, phaseInc);
            float rolloff = 0.6f + (1.0f - shape) * 0.35f; // shape=0 warm, shape=1 bright
            out *= rolloff;
            break;
        }

        case 3: // Cushion Pulse -- shape controls pulse width
        {
            float pw = 0.15f + shape * 0.7f; // shape 0→0.15 PW, shape 1→0.85 PW
            float pulse = (t < static_cast<double>(pw)) ? 1.0f : -1.0f;
            pulse += polyBlepD(t, phaseInc);
            pulse -= polyBlepD(std::fmod(t + 1.0 - static_cast<double>(pw), 1.0), phaseInc);
            out = pulse * 0.75f;
            break;
        }

        default:
            out = BiteSineTable::lookup(t);
            break;
        }

        phase += phaseInc;
        wrapped = false;
        if (phase >= 1.0)
        {
            phase -= 1.0;
            wrapped = true;
        }

        return clamp(out, -1.0f, 1.0f);
    }

    double getPhase() const noexcept { return phase; }
    double getPhaseInc() const noexcept { return basePhaseInc; }

private:
    double sr = 0.0;
    double phase = 0.0;
    double basePhaseInc = 0.0;
    int wave = 0;
    float shape = 0.5f;
    float triState = 0.0f;
    bool wrapped = false;

    // Drift state
    float driftAmount = 0.0f;
    double driftPhase = 0.0;
    double driftPhaseInc = 0.0; // precomputed in prepare(): 0.37 / sr
    float driftLfoState = 0.0f;

    static float polyBlepD(double t, double dt) noexcept
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
// BiteOscB -- "Bite" oscillator: aggressive upper harmonics.
//   0: Hard Sync Saw (sync'd to OscA phase)
//   1: FM (2-op frequency modulation)
//   2: Ring Mod (ring modulation with OscA)
//   3: Noise (filtered noise)
//   4: Grit (bitcrushed saw)
// Shape modifies per-waveform character (sync ratio, FM index, etc.)
// Instability adds pitch/phase jitter for organic movement.
//==============================================================================
class BiteOscB
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        // Precompute instability LFO phase increment — avoids a per-sample division
        instabilityPhaseInc = 2.7 / sr;
        reset();
    }

    void reset() noexcept
    {
        phase = 0.0;
        fmModPhase = 0.0;
        instabilityPhase = 0.0;
        syncBlepActive = false;
        syncBlepHeight = 0.0f;
        syncBlepDt = 0.0f;
    }

    void setFrequency(float hz) noexcept
    {
        baseFreq = static_cast<double>(clamp(hz, 20.0f, 20000.0f));
        phaseInc = baseFreq / sr;
    }

    void setWaveform(int w) noexcept { wave = w; }
    void setShape(float s) noexcept { shape = clamp(s, 0.0f, 1.0f); }
    void setInstability(float i) noexcept { instability = clamp(i, 0.0f, 1.0f); }

    // Call when OscA wraps phase (for hard sync).
    // Records the step height at the discontinuity for polyBLEP antialiasing.
    // The slave oscillator resets to phase=0 (saw resets to -1), so the step
    // size is (currentValue - resetValue) = (2*phase - 1) - (-1) = 2*phase.
    // We also record the fractional phase offset within the current master period
    // so we can correctly position the BLEP in both the pre- and post-reset samples.
    void syncReset(double masterPhaseAtSync, double masterPhaseInc) noexcept
    {
        // Step height: value just before reset minus value just after reset (-1).
        syncBlepHeight = static_cast<float>(2.0 * phase - 1.0) - (-1.0f);
        // Store master inc for post-reset BLEP window calculation
        syncBlepDt = static_cast<float>(masterPhaseInc);
        syncBlepActive = true;
        phase = 0.0;
    }

    float processSample(float oscAout, float externalFM) noexcept
    {
        // Instability: random pitch jitter via slow noise
        float instMod = 0.0f;
        if (instability > 0.001f)
        {
            instabilityPhase += instabilityPhaseInc; // ~2.7 Hz (precomputed in prepare)
            if (instabilityPhase >= 1.0)
                instabilityPhase -= 1.0;
            instMod = BiteSineTable::lookup(instabilityPhase) * instability * 0.006f;
        }
        double effPhaseInc = phaseInc * (1.0 + static_cast<double>(instMod));

        float out = 0.0f;

        switch (wave)
        {
        case 0: // Hard Sync Saw -- shape controls slave frequency ratio
        {
            // Shape 0→1x ratio, shape 1→4x ratio (more harmonics)
            double syncRatio = 1.0 + static_cast<double>(shape) * 3.0;
            double syncPhase = std::fmod(phase * syncRatio, 1.0);
            out = static_cast<float>(2.0 * syncPhase - 1.0);
            // Normal per-cycle polyBLEP for the slave saw's own wrap discontinuity
            out -= polyBlepD(syncPhase, effPhaseInc * syncRatio);

            // Hard-sync discontinuity BLEP (persistent correction).
            // The polyBLEP post-reset tail spans t=[0, dt] — i.e. roughly 1/effPhaseInc
            // samples after the reset.  We apply the correction each sample while
            // the slave phase is still within the correction window (t < dt_slave).
            // syncBlepActive is cleared once we leave the window.
            if (syncBlepActive && syncBlepDt > 0.0f)
            {
                float t_post = static_cast<float>(phase);
                float dt_slave = static_cast<float>(effPhaseInc);
                if (dt_slave > 0.0f && t_post < dt_slave)
                {
                    out -= syncBlepHeight * polyBlepD(static_cast<double>(t_post), static_cast<double>(dt_slave));
                    // Keep active until we exit the BLEP window
                }
                else
                {
                    syncBlepActive = false; // correction window complete
                }
            }
            break;
        }

        case 1: // FM -- shape controls modulation index
        {
            fmModPhase += effPhaseInc * 2.0;
            // Use while-loop in case effPhaseInc*2.0 > 1.0 at very high frequencies
            while (fmModPhase >= 1.0)
                fmModPhase -= 1.0;
            float mod = BiteSineTable::lookup(fmModPhase);
            float fmDepth = shape * 2.0f + externalFM * 2.0f;
            double modPhase = phase + static_cast<double>(mod * fmDepth * 0.3f);
            modPhase -= std::floor(modPhase);
            out = BiteSineTable::lookup(modPhase);
            break;
        }

        case 2: // Ring Mod -- shape controls carrier waveform blend (sine→saw)
        {
            float sine = BiteSineTable::lookup(phase);
            float saw = static_cast<float>(2.0 * phase - 1.0);
            float carrier = lerp(sine, saw, shape);
            out = carrier * oscAout;
            break;
        }

        case 3: // Noise -- shape controls colour (white→brown)
        {
            float raw = noiseGen.process();
            // Shape: 0 = white (raw), 1 = brown (integrated/smoothed)
            noiseFilterState += (raw - noiseFilterState) * (1.0f - shape * 0.95f);
            noiseFilterState = flushDenormal(noiseFilterState);
            out = noiseFilterState;
            break;
        }

        case 4: // Grit -- shape controls bit depth (coarser = more grit)
        {
            out = static_cast<float>(2.0 * phase - 1.0);
            float steps = 64.0f - shape * 56.0f; // shape 0→64 steps, shape 1→8 steps
            steps = std::max(steps, 4.0f);
            out = std::floor(out * steps) / steps;
            break;
        }

        default:
            out = static_cast<float>(2.0 * phase - 1.0);
            break;
        }

        // Apply external FM modulation to phase
        double fmOffset = static_cast<double>(externalFM * 0.01f);
        phase += effPhaseInc + fmOffset;
        while (phase >= 1.0)
            phase -= 1.0;
        while (phase < 0.0)
            phase += 1.0;

        return clamp(out, -1.0f, 1.0f);
    }

private:
    double sr = 0.0;
    double phase = 0.0;
    double phaseInc = 0.0;
    double baseFreq = 440.0;
    double fmModPhase = 0.0;
    double instabilityPhase = 0.0;
    double instabilityPhaseInc = 0.0; // precomputed in prepare(): 2.7 / sr
    int wave = 0;
    float shape = 0.5f;
    float instability = 0.0f;
    float noiseFilterState = 0.0f;
    bool syncBlepActive = false;
    float syncBlepHeight = 0.0f;
    float syncBlepDt = 0.0f; // master phaseInc at sync time (for BLEP window)
    BiteNoiseGen noiseGen;

    static float polyBlepD(double t, double dt) noexcept
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
// BiteSubOsc -- Sub oscillator at -1 or -2 octaves (sine or triangle).
//==============================================================================
class BiteSubOsc
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept { phase = 0.0; }

    void setFrequency(float hz, int octave) noexcept
    {
        // octave: 0 = -1 oct, 1 = -2 oct
        float divisor = (octave == 1) ? 4.0f : 2.0f;
        double freq = static_cast<double>(clamp(hz / divisor, 10.0f, 5000.0f));
        phaseInc = freq / sr;
    }

    float processSample() noexcept
    {
        float out = BiteSineTable::lookup(phase);
        phase += phaseInc;
        if (phase >= 1.0)
            phase -= 1.0;
        return out;
    }

private:
    double sr = 0.0;
    double phase = 0.0;
    double phaseInc = 0.0;
};

//==============================================================================
// BiteWeightEngine -- Low-frequency reinforcement oscillator.
// Adds sub-harmonic weight below the main oscillators. 5 shapes, 3 octaves.
//==============================================================================
class BiteWeightEngine
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }
    void reset() noexcept { phase = 0.0; }

    void setFrequency(float hz, int octave, float tuneCents) noexcept
    {
        // octave: 0=-1, 1=-2, 2=-3
        float divisor = (octave == 2) ? 8.0f : (octave == 1) ? 4.0f : 2.0f;
        // Recompute tuneRatio only when tuneCents changes (block-constant in practice).
        // Use epsilon guard rather than exact == to catch tiny floating-point deltas.
        if (std::abs(tuneCents - lastTuneCents) > 1e-4f)
        {
            lastTuneCents = tuneCents;
            cachedTuneRatio = std::pow(2.0f, tuneCents / 1200.0f);
        }
        double freq = static_cast<double>(clamp(hz / divisor * cachedTuneRatio, 5.0f, 2000.0f));
        phaseInc = freq / sr;
    }

    void setShape(int s) noexcept { shape = s; }

    float processSample() noexcept
    {
        float out = 0.0f;
        double t = phase;

        switch (shape)
        {
        case 0: // Sine -- clean sub weight
            out = BiteSineTable::lookup(t);
            break;
        case 1: // Triangle -- slightly brighter sub
            out = (t < 0.5) ? static_cast<float>(4.0 * t - 1.0) : static_cast<float>(3.0 - 4.0 * t);
            break;
        case 2: // Saw -- full harmonic sub
            out = static_cast<float>(2.0 * t - 1.0);
            break;
        case 3: // Square -- hollow sub
            out = (t < 0.5) ? 1.0f : -1.0f;
            break;
        case 4: // Pulse -- narrow pulse sub for click
            out = (t < 0.2) ? 1.0f : -0.25f;
            break;
        default:
            out = BiteSineTable::lookup(t);
            break;
        }

        phase += phaseInc;
        if (phase >= 1.0)
            phase -= 1.0;
        return out;
    }

private:
    double sr = 0.0;
    double phase = 0.0;
    double phaseInc = 0.0;
    int shape = 0;
    float lastTuneCents = -99999.0f; // sentinel: force recompute on first use
    float cachedTuneRatio = 1.0f;    // cached pow(2, tuneCents/1200)
};

//==============================================================================
// BiteNoiseSource -- 5 noise types with amplitude decay envelope.
//   0: White   1: Pink   2: Brown   3: Crackle   4: Hiss
//==============================================================================
class BiteNoiseSource
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        invSR = 1.0f / static_cast<float>(sr);
        // Pink noise IIR: 3 one-pole leaky integrators approximating -3dB/octave spectrum.
        // Cutoff frequencies (Hz) from Paul Kellet's latticed filter design; matched-Z
        // transform: coeff = 1 - exp(-2π·fc/sr) ensures correct spectral shape at any sr.
        // (Previously used linear scaling 44100/sr — Euler approximation, SR-dependent error.)
        const float twoPiOverSr = 2.0f * 3.14159265f / static_cast<float>(sr);
        // fc values preserve the original relative cutoff positions scaled to Hz
        const float fc0 = 0.0555f * 44100.0f; // ~2449 Hz
        const float fc1 = 0.0158f * 44100.0f; // ~697 Hz
        const float fc2 = 0.0046f * 44100.0f; // ~203 Hz
        pinkCoeff[0] = 1.0f - std::exp(-twoPiOverSr * fc0);
        pinkCoeff[1] = 1.0f - std::exp(-twoPiOverSr * fc1);
        pinkCoeff[2] = 1.0f - std::exp(-twoPiOverSr * fc2);
        // Clamp so coefficients stay in (0,1] — prevents instability at very low sr
        for (auto& c : pinkCoeff)
            c = clamp(c, 0.0001f, 1.0f);
        reset();
    }

    void reset() noexcept
    {
        pinkState[0] = pinkState[1] = pinkState[2] = 0.0f;
        brownState = 0.0f;
        decayLevel = 1.0f;
        lastDecayTime = -1.0f; // force recompute on first process() call
        cachedDecayCoeff = 0.0f;
    }

    void noteOn() noexcept { decayLevel = 1.0f; }

    float process(int type, float decayTime) noexcept
    {
        float raw = rng.process(); // white noise base

        float out = 0.0f;
        switch (type)
        {
        case 0: // White
            out = raw;
            break;

        case 1: // Pink (approximation via 3 leaky integrators — SR-scaled coefficients)
            pinkState[0] += (raw - pinkState[0]) * pinkCoeff[0];
            pinkState[1] += (raw - pinkState[1]) * pinkCoeff[1];
            pinkState[2] += (raw - pinkState[2]) * pinkCoeff[2];
            out = (pinkState[0] + pinkState[1] + pinkState[2]) * 0.66f + raw * 0.1f;
            for (auto& s : pinkState)
                s = flushDenormal(s);
            break;

        case 2: // Brown (integrated white)
            brownState += raw * 0.02f;
            brownState *= 0.998f; // leak to prevent DC drift
            brownState = flushDenormal(brownState);
            out = brownState * 3.5f;
            break;

        case 3: // Crackle (sparse impulses)
        {
            float threshold = 0.97f;
            out = (std::abs(raw) > threshold) ? raw * 4.0f : 0.0f;
            break;
        }

        case 4: // Hiss (high-passed white)
        {
            float prev = hissState;
            hissState = raw;
            out = (raw - prev) * 1.5f;
            break;
        }

        default:
            out = raw;
            break;
        }

        // Apply decay envelope — recompute coefficient only when decayTime changes.
        // Use epsilon guard rather than exact == to catch tiny floating-point deltas.
        float safeDecay = std::max(decayTime, 0.001f);
        if (std::abs(safeDecay - lastDecayTime) > 1e-6f)
        {
            lastDecayTime = safeDecay;
            cachedDecayCoeff = std::exp(-invSR / safeDecay);
        }
        decayLevel *= cachedDecayCoeff;
        decayLevel = flushDenormal(decayLevel);

        return clamp(out * decayLevel, -2.0f, 2.0f);
    }

private:
    double sr = 0.0;
    float invSR = 0.0f; // set by prepare()
    float pinkState[3] = {};
    float pinkCoeff[3] = {0.0555f, 0.0158f, 0.0046f}; // SR-scaled in prepare()
    float brownState = 0.0f;
    float hissState = 0.0f;
    float decayLevel = 1.0f;
    float lastDecayTime = -1.0f;   // sentinel: force recompute on first use
    float cachedDecayCoeff = 0.0f; // cached exp(-invSR / decayTime)
    BiteNoiseGen rng;
};

//==============================================================================
// BiteFur -- Pre-filter soft saturation (tanh warming stage).
// "Fur" adds plush weight before the filter by gently saturating the signal.
//==============================================================================
class BiteFur
{
public:
    float process(float input, float amount) noexcept
    {
        if (amount < 0.001f)
            return input;
        float drive = 1.0f + amount * 6.0f;
        float driven = input * drive;
        // Soft tanh saturation -- even harmonics for warmth
        float sat = fastTanh(driven);
        // Blend dry/wet
        return lerp(input, sat, amount);
    }
};

//==============================================================================
// BiteChew -- Post-filter contour: gentle compression/shaping.
// Compresses dynamics to add sustain and body to the bass.
//==============================================================================
class BiteChew
{
public:
    float process(float input, float amount) noexcept
    {
        if (amount < 0.001f)
            return input;
        // Soft knee compressor approximation
        float absIn = std::abs(input);
        float threshold = 1.0f - amount * 0.7f;
        if (absIn > threshold)
        {
            float excess = absIn - threshold;
            float compressed = threshold + excess / (1.0f + excess * amount * 4.0f);
            input = (input > 0.0f) ? compressed : -compressed;
        }
        return input;
    }
};

//==============================================================================
// BiteGnash -- Post-filter asymmetric waveshaper.
// Adds aggressive "bite" character via asymmetric distortion.
//==============================================================================
class BiteGnash
{
public:
    float process(float input, float amount) noexcept
    {
        if (amount < 0.001f)
            return input;
        float drive = 1.0f + amount * 8.0f;
        float x = input * drive;
        // Asymmetric: positive half gets harder clipping than negative
        float shaped;
        if (x >= 0.0f)
        {
            // Hard curve for positive (the "bite")
            shaped = x / (1.0f + x * x * amount);
        }
        else
        {
            // Softer curve for negative (the "belly")
            shaped = fastTanh(x * 0.7f);
        }
        return lerp(input, shaped, amount);
    }
};

//==============================================================================
// BiteTrash -- Dirt modes: rust (bitcrush), splatter (fold), crushed (clip).
//   Mode 0: Off
//   Mode 1: Rust (bitcrush + sample rate reduction)
//   Mode 2: Splatter (wavefolding)
//   Mode 3: Crushed (hard clip)
//==============================================================================
class BiteTrash
{
public:
    void reset() noexcept
    {
        holdSample = 0.0f;
        holdCounter = 0.0f;
    }

    float process(float input, int mode, float amount) noexcept
    {
        if (mode == 0 || amount < 0.001f)
            return input;

        float out = input;

        switch (mode)
        {
        case 1: // Rust -- bitcrush
        {
            // Reduce bit depth
            float bits = 16.0f - amount * 12.0f; // 16-bit down to 4-bit
            float steps = std::pow(2.0f, bits);
            out = std::floor(out * steps) / steps;
            // Sample rate reduction
            float srReduce = 1.0f + amount * 15.0f;
            holdCounter += 1.0f;
            if (holdCounter >= srReduce)
            {
                holdCounter = 0.0f;
                holdSample = out;
            }
            out = holdSample;
            break;
        }

        case 2: // Splatter -- wavefolding
        {
            float gain = 1.0f + amount * 6.0f;
            out *= gain;
            // Tri-fold waveshaping — capped at 16 iterations to bound worst-case CPU
            // at high gain. Maximum gain=7x drives out to ±7 requiring at most 6 folds.
            for (int fold = 0; fold < 16 && (out > 1.0f || out < -1.0f); ++fold)
            {
                if (out > 1.0f)
                    out = 2.0f - out;
                if (out < -1.0f)
                    out = -2.0f - out;
            }
            break;
        }

        case 3: // Crushed -- hard clip with edge
        {
            float gain = 1.0f + amount * 4.0f;
            out *= gain;
            float clipLevel = 1.0f - amount * 0.5f;
            if (out > clipLevel)
                out = clipLevel;
            if (out < -clipLevel)
                out = -clipLevel;
            break;
        }

        default:
            break;
        }

        return clamp(out, -2.0f, 2.0f);
    }

private:
    float holdSample = 0.0f;
    float holdCounter = 0.0f;
};

//==============================================================================
// BiteAdsrEnvelope -- alias for StandardADSR (shared fleet implementation).
// Replaced from a local copy to eliminate duplication across the engine fleet.
//==============================================================================
using BiteAdsrEnvelope = StandardADSR;

//==============================================================================
// BiteMotionFX -- Stereo chorus / doubler / flange modulated delay line.
// Max delay 40ms. LFO-modulated read pointer with quadrature L/R spread.
//==============================================================================
class BiteMotionFX
{
public:
    static constexpr int kMaxDelaySamples = 4096; // ~43ms @48kHz, ~43ms @96kHz (was 2048: chorus/doubler overflowed at 96kHz)

    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        bufL.assign(static_cast<size_t>(kMaxDelaySamples), 0.0f);
        bufR.assign(static_cast<size_t>(kMaxDelaySamples), 0.0f);
        writePos = 0;
        lfoPhase = 0.0f;
    }

    void reset() noexcept
    {
        if (!bufL.empty())
            std::fill(bufL.begin(), bufL.end(), 0.0f);
        if (!bufR.empty())
            std::fill(bufR.begin(), bufR.end(), 0.0f);
        writePos = 0;
        lfoPhase = 0.0f;
    }

    // type: 0=Plush Chorus, 1=Uneasy Doubler, 2=Oil Flange
    void process(float& outL, float& outR, int type, float rate, float depth, float mix) noexcept
    {
        if (bufL.empty())
            return;

        // Advance LFO always (keeps buffer valid even when mix=0)
        lfoPhase += rate / sr;
        if (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;
        float lfoL = BiteSineTable::lookup(static_cast<double>(lfoPhase));
        double rPhase = static_cast<double>(lfoPhase) + 0.25;
        if (rPhase >= 1.0)
            rPhase -= 1.0;
        float lfoR = BiteSineTable::lookup(rPhase);

        bufL[static_cast<size_t>(writePos)] = outL;
        bufR[static_cast<size_t>(writePos)] = outR;

        if (mix > 0.001f)
        {
            float invSR_ms = 1000.0f / sr;
            float centreMs = 0.0f, widthMs = 0.0f, widthMsR = 0.0f;
            switch (type)
            {
            case 0:
                centreMs = 15.0f;
                widthMs = 7.0f * depth;
                widthMsR = 7.0f * depth * 1.3f;
                break;
            case 1:
                centreMs = 25.0f;
                widthMs = 3.0f * depth;
                widthMsR = 3.0f * depth * 0.7f;
                break;
            case 2:
                centreMs = 3.0f;
                widthMs = 2.5f * depth;
                widthMsR = 2.5f * depth * 0.9f;
                break;
            default:
                centreMs = 15.0f;
                widthMs = 7.0f * depth;
                widthMsR = 7.0f * depth;
                break;
            }
            float centreSmp = centreMs / invSR_ms;
            float wL = widthMs / invSR_ms;
            float wR = widthMsR / invSR_ms;
            float delayL = clamp(centreSmp + lfoL * wL, 1.0f, static_cast<float>(kMaxDelaySamples - 2));
            float delayR = clamp(centreSmp + lfoR * wR, 1.0f, static_cast<float>(kMaxDelaySamples - 2));

            auto readInterp = [this](const std::vector<float>& buf, float delay) -> float
            {
                int n = kMaxDelaySamples;
                float fIdx = static_cast<float>(writePos) - delay;
                // Use fmod instead of while-loop — single-step, handles large delays safely
                if (fIdx < 0.0f)
                    fIdx += static_cast<float>(n);
                int iLo = static_cast<int>(fIdx) % n;
                int iHi = (iLo + 1) % n;
                float frac = fIdx - std::floor(fIdx);
                return buf[static_cast<size_t>(iLo)] +
                       frac * (buf[static_cast<size_t>(iHi)] - buf[static_cast<size_t>(iLo)]);
            };

            float wetL = readInterp(bufL, delayL);
            float wetR = readInterp(bufR, delayR);
            outL = lerp(outL, wetL, mix);
            outR = lerp(outR, wetR, mix);
        }

        writePos = (writePos + 1) % kMaxDelaySamples;
    }

private:
    float sr = 0.0f;
    std::vector<float> bufL, bufR;
    int writePos = 0;
    float lfoPhase = 0.0f;
};

//==============================================================================
// BiteEchoFX -- Stereo delay with character types and feedback.
// Max delay 2s. Ping mode swaps L/R on feedback path.
//==============================================================================
class BiteEchoFX
{
public:
    static constexpr int kMaxDelaySamples = 192001; // >=2s @96kHz (was 96001, too small at 96kHz)

    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        bufL.assign(static_cast<size_t>(kMaxDelaySamples), 0.0f);
        bufR.assign(static_cast<size_t>(kMaxDelaySamples), 0.0f);
        writePos = 0;
        filterStateL = filterStateR = 0.0f;
        // Cache session-constant 1-pole LP coefficients
        darkTapeCoeff = std::exp(-2.0f * 3.14159265f * 3000.0f / sr);
    }

    void reset() noexcept
    {
        if (!bufL.empty())
            std::fill(bufL.begin(), bufL.end(), 0.0f);
        if (!bufR.empty())
            std::fill(bufR.begin(), bufR.end(), 0.0f);
        writePos = 0;
        filterStateL = filterStateR = 0.0f;
    }

    // type: 0=Dark Tape, 1=Murky Digital, 2=Short Slap, 3=Ping
    void process(float& outL, float& outR, int type, float timeSeconds, float feedback, float mix) noexcept
    {
        if (bufL.empty())
            return;

        float effTime = (type == 2) ? std::min(timeSeconds, 0.06f) : timeSeconds;
        float delaySmp = clamp(effTime * sr, 1.0f, static_cast<float>(kMaxDelaySamples - 2));
        int n = kMaxDelaySamples;

        auto readInterp = [n, this](const std::vector<float>& buf, float delay) -> float
        {
            float fIdx = static_cast<float>(writePos) - delay;
            // Single-step offset instead of while-loop — delay is clamped ≤ n-2 so one add suffices
            if (fIdx < 0.0f)
                fIdx += static_cast<float>(n);
            int iLo = static_cast<int>(fIdx) % n;
            int iHi = (iLo + 1) % n;
            float frac = fIdx - std::floor(fIdx);
            return buf[static_cast<size_t>(iLo)] +
                   frac * (buf[static_cast<size_t>(iHi)] - buf[static_cast<size_t>(iLo)]);
        };

        float delayedL = readInterp(bufL, delaySmp);
        float delayedR = readInterp(bufR, delaySmp);

        if (type == 0) // Dark Tape: 1-pole LP on feedback (~3kHz)
        {
            // darkTapeCoeff is precomputed in prepare() — session-constant
            filterStateL = filterStateL * darkTapeCoeff + delayedL * (1.0f - darkTapeCoeff);
            filterStateR = filterStateR * darkTapeCoeff + delayedR * (1.0f - darkTapeCoeff);
            filterStateL = flushDenormal(filterStateL);
            filterStateR = flushDenormal(filterStateR);
            delayedL = filterStateL;
            delayedR = filterStateR;
        }
        else if (type == 1) // Murky Digital: light sat on feedback
        {
            delayedL = fastTanh(delayedL * 1.15f);
            delayedR = fastTanh(delayedR * 1.15f);
        }

        float fbL = (type == 3) ? delayedR * feedback : delayedL * feedback;
        float fbR = (type == 3) ? delayedL * feedback : delayedR * feedback;

        bufL[static_cast<size_t>(writePos)] = flushDenormal(outL + fbL);
        bufR[static_cast<size_t>(writePos)] = flushDenormal(outR + fbR);
        writePos = (writePos + 1) % n;

        if (mix > 0.001f)
        {
            outL = lerp(outL, delayedL, mix);
            outR = lerp(outR, delayedR, mix);
        }
    }

private:
    float sr = 0.0f;
    std::vector<float> bufL, bufR;
    int writePos = 0;
    float filterStateL = 0.0f, filterStateR = 0.0f;
    float darkTapeCoeff = 0.0f; // cached exp(-2π*3000/sr) — computed in prepare()
};

//==============================================================================
// BiteSpaceFX -- Schroeder-style reverb (4 comb + 2 allpass) with 3 room types.
//==============================================================================
class BiteSpaceFX
{
public:
    static constexpr int kNumCombs = 4;
    static constexpr int kNumAllpass = 2;
    static constexpr int kMaxLen = 4096; // must cover longest comb at 96kHz (was 2048, collapsed all combs to same length at 96kHz)

    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        float srScale = sr / 44100.0f;
        const int kCombLens[kNumCombs] = {1557, 1617, 1491, 1422};
        const int kAPLens[kNumAllpass] = {225, 341};
        for (int i = 0; i < kNumCombs; ++i)
        {
            int len = std::min(static_cast<int>(kCombLens[i] * srScale) | 1, kMaxLen - 1);
            combBuf[i].assign(static_cast<size_t>(len), 0.0f);
            combLen[i] = len;
            combPos[i] = 0;
            combState[i] = 0.0f;
        }
        for (int i = 0; i < kNumAllpass; ++i)
        {
            int len = std::min(static_cast<int>(kAPLens[i] * srScale) | 1, kMaxLen - 1);
            apBuf[i].assign(static_cast<size_t>(len), 0.0f);
            apLen[i] = len;
            apPos[i] = 0;
        }
    }

    void reset() noexcept
    {
        for (int i = 0; i < kNumCombs; ++i)
        {
            if (!combBuf[i].empty())
                std::fill(combBuf[i].begin(), combBuf[i].end(), 0.0f);
            combPos[i] = 0;
            combState[i] = 0.0f;
        }
        for (int i = 0; i < kNumAllpass; ++i)
        {
            if (!apBuf[i].empty())
                std::fill(apBuf[i].begin(), apBuf[i].end(), 0.0f);
            apPos[i] = 0;
        }
    }

    // type: 0=Burrow Room, 1=Fog Chamber, 2=Drain Hall
    void process(float& outL, float& outR, int type, float size, float decaySec, float damping, float mix) noexcept
    {
        if (mix < 0.001f || combBuf[0].empty())
            return;

        // RT60 feedback from first comb length
        float baseFB = clamp(std::pow(10.0f, -3.0f * static_cast<float>(combLen[0]) / (decaySec * sr)), 0.0f, 0.97f);
        float effDamping = clamp(damping - size * 0.15f, 0.0f, 1.0f);
        switch (type)
        {
        case 0:
            effDamping = clamp(effDamping + 0.15f, 0.0f, 1.0f);
            baseFB *= 0.85f;
            break;
        case 1:
            effDamping = clamp(effDamping + 0.25f, 0.0f, 1.0f);
            break;
        case 2:
            effDamping = clamp(effDamping - 0.1f, 0.0f, 1.0f);
            baseFB = clamp(baseFB * 1.1f, 0.0f, 0.97f);
            break;
        default:
            break;
        }
        // Cache dampCoeff: only recompute when effDamping changes (typically block-constant).
        // Use epsilon guard rather than exact == to catch tiny floating-point deltas.
        if (std::abs(effDamping - lastEffDamping) > 1e-6f)
        {
            lastEffDamping = effDamping;
            cachedDampCoeff = std::exp(-2.0f * 3.14159265f * (300.0f + (1.0f - effDamping) * 6000.0f) / sr);
        }
        float dampCoeff = cachedDampCoeff;

        float mono = (outL + outR) * 0.5f;
        float reverbOut = 0.0f;
        for (int i = 0; i < kNumCombs; ++i)
        {
            int pos = combPos[i];
            float delayed = combBuf[i][static_cast<size_t>(pos)];
            combState[i] = combState[i] * dampCoeff + delayed * (1.0f - dampCoeff);
            combState[i] = flushDenormal(combState[i]);
            combBuf[i][static_cast<size_t>(pos)] = flushDenormal(mono + combState[i] * baseFB);
            combPos[i] = (pos + 1) % combLen[i];
            reverbOut += delayed;
        }
        reverbOut *= 0.25f;
        // Two allpass diffusers in series (Schroeder).
        // R channel reads from a half-buffer-offset tap for stereo decorrelation
        // without requiring a second set of allpass buffers.
        float reverbL = reverbOut;
        float reverbR = reverbOut;
        for (int i = 0; i < kNumAllpass; ++i)
        {
            int pos = apPos[i];
            // L: canonical write pointer
            float delayedL = apBuf[i][static_cast<size_t>(pos)];
            float inputL = reverbL + delayedL * 0.5f;
            apBuf[i][static_cast<size_t>(pos)] = flushDenormal(inputL);
            apPos[i] = (pos + 1) % apLen[i];
            reverbL = flushDenormal(delayedL - 0.5f * inputL);

            // R: read from a half-period-offset position (read-only, no state write)
            int posR = (pos + apLen[i] / 2) % apLen[i];
            float delayedR = apBuf[i][static_cast<size_t>(posR)];
            reverbR = flushDenormal(delayedR - 0.5f * (reverbR + delayedR * 0.5f));
        }
        outL = lerp(outL, reverbL, mix);
        outR = lerp(outR, reverbR, mix);
    }

private:
    float sr = 0.0f;
    std::vector<float> combBuf[kNumCombs];
    int combLen[kNumCombs] = {1557, 1617, 1491, 1422};
    int combPos[kNumCombs] = {};
    float combState[kNumCombs] = {};
    std::vector<float> apBuf[kNumAllpass];
    int apLen[kNumAllpass] = {225, 341};
    int apPos[kNumAllpass] = {};
    // Cached damping coefficient to avoid per-block exp() when effDamping is constant
    float lastEffDamping = -1.0f; // sentinel: force recompute on first process() call
    float cachedDampCoeff = 0.0f;
};

//==============================================================================
// BiteLFO -- LFO with 7 shapes: Sine, Triangle, Saw, Square, S&H, Random, Stepped.
// Supports retrigger (reset on note-on) and start phase offset.
//==============================================================================
class BiteLFO
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        invSR = 1.0f / static_cast<float>(sr);
    }

    void reset() noexcept
    {
        phase = 0.0f;
        snhValue = 0.0f;
        randomTarget = 0.0f;
        randomCurrent = 0.0f;
    }

    void setRate(float hz) noexcept { rate = clamp(hz, 0.01f, 50.0f); }
    void setShape(int s) noexcept { shape = s; }
    void setStartPhase(float p) noexcept { startPhase = clamp(p, 0.0f, 1.0f); }

    void retrigger() noexcept { phase = startPhase; }

    float process() noexcept
    {
        phase += rate * invSR;
        bool wrapped = false;
        if (phase >= 1.0f)
        {
            phase -= 1.0f;
            wrapped = true;
        }

        float out = 0.0f;

        switch (shape)
        {
        case 0: // Sine
        {
            constexpr float twoPi = 6.28318530f;
            out = fastSin(phase * twoPi);
            break;
        }
        case 1: // Triangle
            out = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
            break;
        case 2: // Saw
            out = 2.0f * phase - 1.0f;
            break;
        case 3: // Square
            out = (phase < 0.5f) ? 1.0f : -1.0f;
            break;
        case 4: // S&H -- sample-and-hold
            if (wrapped)
                snhValue = rng.process();
            out = snhValue;
            break;
        case 5: // Random -- smoothly interpolated random
            if (wrapped)
            {
                randomCurrent = randomTarget;
                randomTarget = rng.process();
            }
            out = lerp(randomCurrent, randomTarget, phase);
            break;
        case 6: // Stepped -- 8-step staircase
        {
            int step = static_cast<int>(phase * 8.0f);
            out = (static_cast<float>(step) / 3.5f) - 1.0f;
            break;
        }
        default:
            out = fastSin(phase * 6.28318530f);
            break;
        }

        return out;
    }

private:
    double sr = 0.0;
    float invSR = 0.0f; // set by prepare()
    float phase = 0.0f;
    float rate = 1.0f;
    int shape = 0;
    float startPhase = 0.0f;
    float snhValue = 0.0f;
    float randomTarget = 0.0f;
    float randomCurrent = 0.0f;
    BiteNoiseGen rng;
};

//==============================================================================
// BiteVoice -- Per-voice state for the BITE engine.
//==============================================================================
struct BiteVoice
{
    bool active = false;
    bool releasing = false; // True during release stage; voice can be stolen preferentially
    int noteNumber = 60;
    float velocity = 0.0f;
    uint64_t startTime = 0; // Monotonic counter for LRU voice stealing (VoiceAllocator)

    BiteOscA oscA;
    BiteOscB oscB;
    BiteSubOsc sub;
    BiteWeightEngine weight;
    BiteNoiseSource noise;
    CytomicSVF filter;
    BiteFur fur;
    BiteChew chew;
    BiteGnash gnash;
    BiteTrash trash;
    CytomicSVF chewFilter;
    BiteAdsrEnvelope ampEnv;
    BiteAdsrEnvelope filterEnv;
    BiteAdsrEnvelope modEnv;
    BiteLFO lfo1;
    BiteLFO lfo2;
    BiteLFO lfo3;

    // Unison sub-voice index (0 = center, 1..N-1 = detuned copies)
    int unisonIndex = 0;
    int unisonTotal = 1;

    // Track OscA phase wrap for hard sync
    float prevOscAPhase = 0.0f;
    bool oscAWrapped = false;

    // Glide state
    GlideProcessor glide; // shared utility: frequency-domain portamento
    bool hasPlayedBefore = false;

    // Cached per-block
    float cachedUnisonDetuneRatio = 1.0f; // hoist std::pow out of per-sample/per-voice loop

    // MPE per-voice expression state
    MPEVoiceExpression mpeExpression;
};

//==============================================================================
// BiteEngine -- Bass-forward character synth adapted from XOverbite.
//
// Signal chain per voice:
//   OscA (belly, shape+drift) + OscB (bite, shape+instability) + Sub
//   + Weight Engine + Noise -> Fur (pre-filter saturation)
//   -> FilterDrive -> CytomicSVF (4 modes, key tracking)
//   -> Chew (contour) -> Gnash (asymmetric) -> Trash (dirt modes)
//   -> Amp Envelope (velocity sensitive) -> Pan -> Output
//
// Modulation: 3 ADSR envelopes (amp, filter, mod), 3 LFOs (7 shapes),
//   8-slot mod matrix (Phase 4), osc interaction (4 modes).
// 5 macros: Belly, Bite, Scurry, Trash, Play Dead.
// Voice: glide (legato/always), unison (Phase 4), polyphony 1-16.
// FX: Motion, Echo, Space, Finish (Phase 5).
// 122 parameters total (frozen IDs, poss_ prefix).
//
// Coupling:
//   Output: audio (ch0/ch1), envelope level (ch2)
//   Input: AmpToFilter (drum hits pump filter),
//          AudioToFM (other engine audio FM-modulates oscillators)
//==============================================================================
class BiteEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;

    //-- SynthEngine interface -------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);
        // Cache session-constant FxFinish LowMono 1-pole LP coefficient
        lowMonoCoeff = std::exp(-2.0f * 3.14159265f * 200.0f / srf);
        silenceGate.prepare(sampleRate, maxBlockSize);
        silenceGate.setHoldTime(100.0f); // Percussive — short hold

        outputCacheL.resize(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR.resize(static_cast<size_t>(maxBlockSize), 0.0f);

        motionFX.prepare(sampleRate);
        echoFX.prepare(sampleRate);
        spaceFX.prepare(sampleRate);

        aftertouch.prepare(sampleRate);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            auto& v = voices[static_cast<size_t>(i)];
            v.active = false;
            v.oscA.prepare(sr);
            v.oscB.prepare(sr);
            v.sub.prepare(sr);
            v.weight.prepare(sr);
            v.noise.prepare(sr);
            v.filter.reset();
            v.filter.setMode(CytomicSVF::Mode::LowPass);
            v.chewFilter.reset();
            v.chewFilter.setMode(CytomicSVF::Mode::LowPass);
            v.trash.reset();
            v.ampEnv.prepare(static_cast<float>(sr));
            v.filterEnv.prepare(static_cast<float>(sr));
            v.modEnv.prepare(static_cast<float>(sr));
            v.lfo1.prepare(sr);
            v.lfo2.prepare(sr);
            v.lfo3.prepare(sr);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.releasing = false;
            v.oscA.reset();
            v.oscB.reset();
            v.sub.reset();
            v.weight.reset();
            v.noise.reset();
            v.filter.reset();
            v.chewFilter.reset();
            v.trash.reset();
            v.ampEnv.reset();
            v.filterEnv.reset();
            v.modEnv.reset();
            v.lfo1.reset();
            v.lfo2.reset();
            v.lfo3.reset();
            v.glide.reset();
            v.hasPlayedBefore = false;
        }
        envelopeOutput = 0.0f;
        externalFilterMod = 0.0f;
        externalFMBuffer = nullptr;
        externalFMSamples = 0;
        motionFX.reset();
        echoFX.reset();
        spaceFX.reset();
        lowMonoStateL = 0.0f;
        lowMonoStateR = 0.0f;
        std::fill(outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill(outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0)
            return;
        jassert(sr > 0.0);
        if (sr <= 0.0) { buffer.clear(); return; }

        // =====================================================================
        // ParamSnapshot: read ALL parameters once per block
        // =====================================================================
        // Oscillators
        const int oscAWave = safeLoad(pOscAWaveform, 0);
        const float oscAShape = safeLoadF(pOscAShape, 0.5f);
        const float oscADriftAmt = safeLoadF(pOscADrift, 0.05f);
        const int oscBWave = safeLoad(pOscBWaveform, 0);
        const float oscBShape = safeLoadF(pOscBShape, 0.5f);
        const float oscBInstab = safeLoadF(pOscBInstability, 0.0f);
        const float oscMix = safeLoadF(pOscMix, 0.3f);
        const int oscInteractMode = safeLoad(pOscInteractMode, 0);
        const float oscInteractAmt = safeLoadF(pOscInteractAmount, 0.0f);
        // Sub
        const float subLevel = safeLoadF(pSubLevel, 0.3f);
        const int subOctave = safeLoad(pSubOctave, 0);
        // Weight
        const int weightShape = safeLoad(pWeightShape, 0);
        const int weightOctave = safeLoad(pWeightOctave, 1);
        const float weightLevel = safeLoadF(pWeightLevel, 0.0f);
        const float weightTune = safeLoadF(pWeightTune, 0.0f);
        // Noise
        const int noiseType = safeLoad(pNoiseType, 0);
        const int noiseRouting = safeLoad(pNoiseRouting, 0);
        const float noiseLevel = safeLoadF(pNoiseLevel, 0.0f);
        const float noiseDecay = safeLoadF(pNoiseDecay, 0.1f);
        // Filter
        const float filterCutoff = safeLoadF(pFilterCutoff, 2000.0f);
        const float filterReso = safeLoadF(pFilterReso, 0.3f);
        const int filterMode = safeLoad(pFilterMode, 0);
        const float filterKeyTrack = safeLoadF(pFilterKeyTrack, 0.0f);
        const float filterDrive = safeLoadF(pFilterDrive, 0.0f);
        // Character
        const float furAmount = safeLoadF(pFurAmount, 0.0f);
        const float chewAmount = safeLoadF(pChewAmount, 0.0f);
        const float chewFreq = safeLoadF(pChewFreq, 1000.0f);
        const float chewMix = safeLoadF(pChewMix, 0.5f);
        const float driveAmount = safeLoadF(pDriveAmount, 0.0f);
        const int driveType = safeLoad(pDriveType, 0);
        const float gnashAmount = safeLoadF(pGnashAmount, 0.0f);
        const int trashMode = safeLoad(pTrashMode, 0);
        const float trashAmount = safeLoadF(pTrashAmount, 0.0f);
        // Amp Envelope
        const float ampA = safeLoadF(pAmpAttack, 0.005f);
        const float ampD = safeLoadF(pAmpDecay, 0.3f);
        const float ampS = safeLoadF(pAmpSustain, 0.8f);
        const float ampR = safeLoadF(pAmpRelease, 0.3f);
        const float ampVelSens = safeLoadF(pAmpVelSens, 0.7f);
        // Filter Envelope
        const float filtEnvAmt = safeLoadF(pFilterEnvAmount, 0.3f);
        const float filtA = safeLoadF(pFilterAttack, 0.005f);
        const float filtD = safeLoadF(pFilterDecay, 0.3f);
        const float filtS = safeLoadF(pFilterSustain, 0.0f);
        const float filtR = safeLoadF(pFilterRelease, 0.3f);
        // Mod Envelope
        const float modEnvAmt = safeLoadF(pModEnvAmount, 0.0f);
        const float modA = safeLoadF(pModAttack, 0.01f);
        const float modD = safeLoadF(pModDecay, 0.5f);
        const float modS = safeLoadF(pModSustain, 0.0f);
        const float modR = safeLoadF(pModRelease, 0.5f);
        const int modEnvDest = safeLoad(pModEnvDest, 2); // default: Filter Cutoff
        // LFO 1
        const int lfo1Shape = safeLoad(pLfo1Shape, 0);
        const float lfo1RateRaw = safeLoadF(pLfo1Rate, 1.0f);
        const float lfo1Depth = safeLoadF(pLfo1Depth, 0.0f);
        const bool lfo1Sync = safeLoad(pLfo1Sync, 0) > 0;
        const bool lfo1Retrigger = safeLoad(pLfo1Retrigger, 0) > 0;
        const float lfo1Phase = safeLoadF(pLfo1Phase, 0.0f);
        // LFO 2
        const int lfo2Shape = safeLoad(pLfo2Shape, 0);
        const float lfo2RateRaw = safeLoadF(pLfo2Rate, 2.0f);
        const float lfo2Depth = safeLoadF(pLfo2Depth, 0.0f);
        const bool lfo2Sync = safeLoad(pLfo2Sync, 0) > 0;
        const bool lfo2Retrigger = safeLoad(pLfo2Retrigger, 0) > 0;
        const float lfo2Phase = safeLoadF(pLfo2Phase, 0.0f);
        // LFO 3
        const int lfo3Shape = safeLoad(pLfo3Shape, 0);
        const float lfo3RateRaw = safeLoadF(pLfo3Rate, 0.5f);
        const float lfo3Depth = safeLoadF(pLfo3Depth, 0.0f);
        const bool lfo3Sync = safeLoad(pLfo3Sync, 0) > 0;
        const bool lfo3Retrigger = safeLoad(pLfo3Retrigger, 0) > 0;
        const float lfo3Phase = safeLoadF(pLfo3Phase, 0.0f);

        // LFO tempo sync: when Sync=On, snap rate to nearest common subdivision
        // at nominal 120 BPM (2 beats/sec).  The rate knob value [0.01..50] maps to
        // subdivision choices: 1/32(0.25Hz), 1/16(0.5Hz), 1/8(1Hz), 1/4(2Hz),
        // 1/2(4Hz), 1 bar(0.5Hz), 2 bars(0.25Hz).  We pick the subdivision whose
        // Hz value is closest to the raw rate, giving meaningful audible differences
        // as the rate knob is turned while sync is engaged.
        auto syncedRate = [](float rawHz) -> float
        {
            // Common subdivisions at 120 BPM in Hz (beat = 2 Hz)
            constexpr float kSubdivisions[] = {0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f};
            constexpr int kNSub = 7;
            float best = kSubdivisions[0];
            float bestDist = std::abs(rawHz - kSubdivisions[0]);
            for (int i = 1; i < kNSub; ++i)
            {
                float d = std::abs(rawHz - kSubdivisions[i]);
                if (d < bestDist)
                {
                    bestDist = d;
                    best = kSubdivisions[i];
                }
            }
            return best;
        };

        const float lfo1Rate = lfo1Sync ? syncedRate(lfo1RateRaw) : lfo1RateRaw;
        const float lfo2Rate = lfo2Sync ? syncedRate(lfo2RateRaw) : lfo2RateRaw;
        const float lfo3Rate = lfo3Sync ? syncedRate(lfo3RateRaw) : lfo3RateRaw;
        // Output / Voice
        const float level = safeLoadF(pLevel, 0.7f);
        const float pan = safeLoadF(pPan, 0.0f);
        const int polyChoice = safeLoad(pPolyphony, 3);
        const float glideTime = safeLoadF(pGlideTime, 0.0f);
        const int glideMode = safeLoad(pGlideMode, 0);
        // Unison: choice 0-6 maps to 1-7 voices
        const int unisonVoices = safeLoad(pUnisonVoices, 0) + 1;
        const float unisonDetune = safeLoadF(pUnisonDetune, 0.2f);
        const float unisonSpread = safeLoadF(pUnisonSpread, 0.5f);
        // Macros
        const float macroBelly = safeLoadF(pMacroBelly, 0.0f);
        const float macroBite = safeLoadF(pMacroBite, 0.0f);
        const float macroScurry = safeLoadF(pMacroScurry, 0.0f);
        const float macroTrash = safeLoadF(pMacroTrash, 0.0f);
        const float macroPlayDead = safeLoadF(pMacroPlayDead, 0.0f);
        // Mod matrix (8 slots × 3 = 24 params)
        struct ModSlot
        {
            int src;
            int dst;
            float amt;
        };
        const ModSlot modSlots[8] = {
            {safeLoad(pModSlot1Src, 0), safeLoad(pModSlot1Dst, 0), safeLoadF(pModSlot1Amt, 0.0f)},
            {safeLoad(pModSlot2Src, 0), safeLoad(pModSlot2Dst, 0), safeLoadF(pModSlot2Amt, 0.0f)},
            {safeLoad(pModSlot3Src, 0), safeLoad(pModSlot3Dst, 0), safeLoadF(pModSlot3Amt, 0.0f)},
            {safeLoad(pModSlot4Src, 0), safeLoad(pModSlot4Dst, 0), safeLoadF(pModSlot4Amt, 0.0f)},
            {safeLoad(pModSlot5Src, 0), safeLoad(pModSlot5Dst, 0), safeLoadF(pModSlot5Amt, 0.0f)},
            {safeLoad(pModSlot6Src, 0), safeLoad(pModSlot6Dst, 0), safeLoadF(pModSlot6Amt, 0.0f)},
            {safeLoad(pModSlot7Src, 0), safeLoad(pModSlot7Dst, 0), safeLoadF(pModSlot7Amt, 0.0f)},
            {safeLoad(pModSlot8Src, 0), safeLoad(pModSlot8Dst, 0), safeLoadF(pModSlot8Amt, 0.0f)},
        };
        // FX: Motion (4 params)
        const int fxMotionType = safeLoad(pFxMotionType, 0);
        const float fxMotionRate = safeLoadF(pFxMotionRate, 0.5f);
        const float fxMotionDepth = safeLoadF(pFxMotionDepth, 0.0f);
        const float fxMotionMix = safeLoadF(pFxMotionMix, 0.0f);
        // FX: Echo (5 params)
        const int fxEchoType = safeLoad(pFxEchoType, 0);
        const float fxEchoTime = safeLoadF(pFxEchoTime, 0.3f);
        const float fxEchoFeedback = safeLoadF(pFxEchoFeedback, 0.3f);
        const float fxEchoMix = safeLoadF(pFxEchoMix, 0.0f);
        const bool fxEchoSync = safeLoad(pFxEchoSync, 0) > 0;
        // FX: Space (5 params)
        const int fxSpaceType = safeLoad(pFxSpaceType, 0);
        const float fxSpaceSize = safeLoadF(pFxSpaceSize, 0.3f);
        const float fxSpaceDecay = safeLoadF(pFxSpaceDecay, 1.5f);
        const float fxSpaceDamping = safeLoadF(pFxSpaceDamping, 0.5f);
        const float fxSpaceMix = safeLoadF(pFxSpaceMix, 0.0f);
        // FX: Finish (4 params)
        const float fxFinishGlue = safeLoadF(pFxFinishGlue, 0.0f);
        const float fxFinishClip = safeLoadF(pFxFinishClip, 0.0f);
        const float fxFinishWidth = safeLoadF(pFxFinishWidth, 1.0f);
        const float fxFinishLowMono = safeLoadF(pFxFinishLowMono, 0.0f);

        // =====================================================================
        // Decode polyphony: 0->1, 1->2, 2->4, 3->8, 4->16
        // =====================================================================
        static constexpr int polyTable[] = {1, 2, 4, 8, 16};
        const int maxPoly = polyTable[std::min(4, polyChoice)];

        // =====================================================================
        // Apply macros
        // =====================================================================
        // M1 BELLY: plush weight
        const float effOscMix = clamp(oscMix - macroBelly * 0.4f, 0.0f, 1.0f);
        const float effSubLevel = clamp(subLevel + macroBelly * 0.5f, 0.0f, 1.0f);
        const float effFurAmount = clamp(furAmount + macroBelly * 0.4f, 0.0f, 1.0f);
        const float effWeightLvl = clamp(weightLevel + macroBelly * 0.3f, 0.0f, 1.0f);
        const float bellyCutoffMod = -macroBelly * 3000.0f;

        // D006: smooth aftertouch pressure and compute modulation value
        aftertouch.updateBlock(numSamples);
        const float atPressure = aftertouch.getSmoothedPressure(0);

        // M2 BITE: feral aggression
        // D006: aftertouch adds up to +0.3 to BITE macro intensity (sensitivity 0.3)
        // D006: mod wheel adds up to +0.4 to BITE macro depth (CC#1 increases feral bite edge)
        const float effectiveBite = clamp(macroBite + atPressure * 0.3f + modWheelAmount * 0.4f, 0.0f, 1.0f);
        const float biteOscMix = clamp(effOscMix + effectiveBite * 0.4f, 0.0f, 1.0f);
        const float effGnashAmount = clamp(gnashAmount + effectiveBite * 0.6f, 0.0f, 1.0f);
        const float biteResoMod = effectiveBite * 0.3f;

        // M3 SCURRY: nervous energy
        const float scurryLfoMul = 1.0f + macroScurry * 4.0f;
        const float scurryEnvMul = 1.0f - macroScurry * 0.7f;

        // M4 TRASH: dirt and destruction
        const float effTrashAmount = clamp(trashAmount + macroTrash * 0.8f, 0.0f, 1.0f);
        const float trashResoMod = macroTrash * 0.2f;

        // M5 PLAY DEAD: decay to silence
        const float playDeadRelMul = 1.0f + macroPlayDead * 4.0f; // extends release
        const float playDeadLevel = 1.0f - macroPlayDead * 0.8f;  // ducks level
        const float playDeadCutoff = -macroPlayDead * 4000.0f;    // closes filter

        // Combined effective resonance
        const float effFilterReso = clamp(filterReso + biteResoMod + trashResoMod, 0.0f, 0.95f);

        // Cache LFO retrigger flags so noteOn() can act on them
        lfo1RetriggerFlag = lfo1Retrigger;
        lfo2RetriggerFlag = lfo2Retrigger;
        lfo3RetriggerFlag = lfo3Retrigger;
        unisonVoicesFlag = unisonVoices;

        // Glide: update time on all active voices (once per block)
        for (auto& voice : voices)
            if (voice.active)
                voice.glide.setTime(glideTime, srf);

        // =====================================================================
        // Process MIDI events
        // =====================================================================
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), msg.getChannel(), maxPoly);
            }
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber(), msg.getChannel());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                allVoicesOff();
            // D006: channel pressure → aftertouch (applied to BITE macro below)
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure(msg.getChannelPressureValue() / 127.0f);
            // D006: CC#1 mod wheel → BITE macro depth boost (+0–0.4, increases feral bite edge)
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
        float filterMod = externalFilterMod;
        externalFilterMod = 0.0f;

        float peakEnv = 0.0f;

        // Filter mode (once per block)
        CytomicSVF::Mode svfMode = CytomicSVF::Mode::LowPass;
        switch (filterMode)
        {
        case 0:
            svfMode = CytomicSVF::Mode::LowPass;
            break;
        case 1:
            svfMode = CytomicSVF::Mode::BandPass;
            break;
        case 2:
            svfMode = CytomicSVF::Mode::HighPass;
            break;
        case 3:
            svfMode = CytomicSVF::Mode::Notch;
            break;
        }

        // Pan gains (equal-power)
        const float panR = clamp((pan + 1.0f) * 0.5f, 0.0f, 1.0f);
        const float panL = 1.0f - panR;
        const float panGainL = std::sqrt(panL);
        const float panGainR = std::sqrt(panR);

        // =====================================================================
        // Pre-compute per-voice block constants
        // =====================================================================
        for (auto& voice : voices)
        {
            if (!voice.active)
                continue;
            voice.ampEnv.setADSR(ampA, ampD, ampS, ampR * playDeadRelMul);
            voice.filterEnv.setADSR(filtA * scurryEnvMul, filtD * scurryEnvMul, filtS,
                                    filtR * scurryEnvMul * playDeadRelMul);
            voice.modEnv.setADSR(modA, modD, modS, modR);

            // LFO config: shape/rate/startPhase are all block-constant per voice.
            // Hoisted here from the per-sample loop — was setRate × 3 LFOs ×
            // N samples × V voices per block.
            voice.lfo1.setShape(lfo1Shape);
            voice.lfo1.setRate(lfo1Rate * scurryLfoMul);
            voice.lfo1.setStartPhase(lfo1Phase);
            voice.lfo2.setShape(lfo2Shape);
            voice.lfo2.setRate(lfo2Rate * scurryLfoMul);
            voice.lfo2.setStartPhase(lfo2Phase);
            voice.lfo3.setShape(lfo3Shape);
            voice.lfo3.setRate(lfo3Rate * scurryLfoMul);
            voice.lfo3.setStartPhase(lfo3Phase);

            float targetFreq = midiToFreq(voice.noteNumber);
            // MPE pitch bend (fastPow2: ~0.1% error, per-block per-voice)
            targetFreq *= fastPow2(voice.mpeExpression.pitchBendSemitones * (1.0f / 12.0f));
            // Glide
            if (glideMode > 0 && glideTime > 0.001f && voice.hasPlayedBefore)
                voice.glide.setTarget(targetFreq);
            else
                voice.glide.snapTo(targetFreq);
            voice.filter.setMode(svfMode);

            // Pre-compute unison detune ratio per voice — block-constant so we hoist the
            // std::pow out of the per-sample render loop.
            if (voice.unisonTotal > 1)
            {
                const float detuneRange = unisonDetune * 50.0f; // cents
                const float unisonPos = (static_cast<float>(voice.unisonIndex) -
                                          static_cast<float>(voice.unisonTotal - 1) * 0.5f) /
                                         static_cast<float>(voice.unisonTotal - 1);
                const float detuneSemitones = unisonPos * detuneRange / 100.0f;
                voice.cachedUnisonDetuneRatio = fastPow2(detuneSemitones * (1.0f / 12.0f));
            }
            else
            {
                voice.cachedUnisonDetuneRatio = 1.0f;
            }
        }

        // Block-constant pitch-bend ratio (channel pitch wheel) — hoist the
        // fastPow2 call out of the per-sample loop; LFO1 pitch vibrato stays
        // per-sample as a separate linear scale.
        const float blockChannelBendRatio = PitchBendUtil::semitonesToFreqRatio(pitchBendNorm * 2.0f);

        // =====================================================================
        // Render voices (per-sample loop)
        // =====================================================================
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const bool updateFilter = ((sample & 15) == 0);
            float mixL = 0.0f, mixR = 0.0f;

            float externalFM = 0.0f;
            if (externalFMBuffer != nullptr && sample < externalFMSamples)
                externalFM = externalFMBuffer[sample] * externalFMAmount;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                // --- Glide ---
                float freq = voice.glide.process();

                // --- Unison detune ---
                // (cached in per-block voice setup loop — block-constant)
                if (voice.unisonTotal > 1)
                    freq *= voice.cachedUnisonDetuneRatio;

                // --- Velocity sensitivity ---
                float velGain = 1.0f - ampVelSens + ampVelSens * voice.velocity;

            // --- LFOs (setShape/setRate/setStartPhase hoisted to per-block voice loop) ---
                // lfoXraw: pre-depth [-1,1] value used as mod-matrix source so the matrix
                // amount knob is the sole depth control (prevents double-scaling with lfoXDepth).
                // lfoXval: post-depth value used for dedicated hardwired destinations (pitch, filter).
                float lfo1raw = voice.lfo1.process();
                float lfo2raw = voice.lfo2.process();
                float lfo3raw = voice.lfo3.process();
                float lfo1val = lfo1raw * lfo1Depth;
                float lfo2val = lfo2raw * lfo2Depth;
                float lfo3val = lfo3raw * lfo3Depth;

                // --- Mod Envelope (with destination routing) ---
                float modEnvVal = voice.modEnv.process() * modEnvAmt;
                // modEnvDest: 0=OscA Shape, 1=OscB Shape, 2=Filter Cutoff,
                //             3=Fur, 4=Gnash, 5=Trash, 6=Osc Mix, 7=Weight Level
                float modEnvOscAShape = (modEnvDest == 0) ? modEnvVal : 0.0f;
                float modEnvOscBShape = (modEnvDest == 1) ? modEnvVal : 0.0f;
                float modEnvCutoff = (modEnvDest == 2) ? modEnvVal * 3000.0f : 0.0f;
                float modEnvFur = (modEnvDest == 3) ? modEnvVal : 0.0f;
                float modEnvGnash = (modEnvDest == 4) ? modEnvVal : 0.0f;
                float modEnvTrash = (modEnvDest == 5) ? modEnvVal : 0.0f;
                float modEnvOscMix = (modEnvDest == 6) ? modEnvVal : 0.0f;
                float modEnvWeightLvl = (modEnvDest == 7) ? modEnvVal : 0.0f;

                // ---- Mod Matrix ----
                // Sources: 0=Off, 1=LFO1, 2=LFO2, 3=LFO3, 4=AmpEnv, 5=FilterEnv, 6=ModEnv,
                //          7=Velocity, 8=Note, 9=Aftertouch, 10=ModWheel, 11-15=Macros
                // Destinations: 0=Off, 1=OscAShape, 2=OscADrift, 3=OscBShape, 4=OscBInstab,
                //   5=OscMix, 6=OscInteract, 7=SubLevel, 8=WeightLevel, 9=NoiseLevel,
                //   10=FilterCutoff, 11=FilterReso, 12=FilterDrive, 13=Fur, 14=Chew,
                //   15=Drive, 16=Gnash, 17=Trash, 18=AmpLevel, 19=Pan,
                //   20=FxMotionRate, 21=FxMotionDepth, 22=FxEchoTime, 23=FxEchoFeedback,
                //   24=FxSpaceSize, 25=FxSpaceDecay
                const float mmSrc[16] = {0.0f,
                                         lfo1raw, // pre-depth; slot.amt is the sole scaling factor
                                         lfo2raw,
                                         lfo3raw,
                                         voice.ampEnv.getLevel(),
                                         voice.filterEnv.getLevel(),
                                         modEnvVal,
                                         voice.velocity,
                                         (static_cast<float>(voice.noteNumber) - 60.0f) / 64.0f,
                                         atPressure,
                                         modWheelAmount,
                                         macroBelly,
                                         macroBite,
                                         macroScurry,
                                         macroTrash,
                                         macroPlayDead};
                float mmDst[26] = {};
                for (int ms = 0; ms < 8; ++ms)
                {
                    const auto& slot = modSlots[ms];
                    if (slot.src == 0 || slot.dst == 0 || std::abs(slot.amt) < 1e-4f)
                        continue;
                    mmDst[std::min(slot.dst, 25)] += mmSrc[std::min(slot.src, 15)] * slot.amt;
                }
                // Fold matrix contributions into existing mod accumulators
                modEnvOscAShape += mmDst[1];
                modEnvOscBShape += mmDst[3];
                modEnvOscMix += mmDst[5];
                modEnvWeightLvl += mmDst[8];
                modEnvCutoff += mmDst[10] * 3000.0f;
                modEnvFur += mmDst[13];
                modEnvGnash += mmDst[16];
                modEnvTrash += mmDst[17];

                // LFO1 -> pitch (subtle vibrato), scaled by Scurry
                float pitchMod = lfo1val * 0.005f * (1.0f + macroScurry);
                freq *= (1.0f + pitchMod);
                freq *= blockChannelBendRatio; // hoisted above — block-const pitch bend

                // --- OscA (Belly) ---
                voice.oscA.setFrequency(freq);
                voice.oscA.setWaveform(oscAWave);
                // modEnv can morph OscA shape when dest=0
                voice.oscA.setShape(clamp(oscAShape + modEnvOscAShape, 0.0f, 1.0f));
                voice.oscA.setDrift(clamp(oscADriftAmt + mmDst[2], 0.0f, 1.0f)); // mmDst[2]=OscA Drift
                float oscAout = voice.oscA.processSample();

                // --- Hard sync: reset OscB on OscA phase wrap ---
                // Pass master phase (post-wrap value) and master phaseInc so the
                // BLEP residual can be correctly positioned at the discontinuity.
                if (oscBWave == 0 && voice.oscA.didPhaseWrap())
                    voice.oscB.syncReset(voice.oscA.getPhase(), voice.oscA.getPhaseInc());

                // --- OscB (Bite) ---
                voice.oscB.setFrequency(freq);
                voice.oscB.setWaveform(oscBWave);
                // modEnv can morph OscB shape when dest=1
                voice.oscB.setShape(clamp(oscBShape + modEnvOscBShape, 0.0f, 1.0f));
                voice.oscB.setInstability(clamp(oscBInstab + mmDst[4], 0.0f, 1.0f)); // mmDst[4]=OscB Instability
                float oscBout = voice.oscB.processSample(oscAout, externalFM);

                // --- Osc Interaction ---
                float effInteractAmt = clamp(oscInteractAmt + mmDst[6], 0.0f, 1.0f); // mmDst[6]=OscInteract
                if (oscInteractMode > 0 && effInteractAmt > 0.001f)
                {
                    float interactSig = 0.0f;
                    switch (oscInteractMode)
                    {
                    case 1: // Soft Sync — blend in sync'd waveform
                        interactSig = oscAout * oscBout;
                        break;
                    case 2: // Low FM — OscA ring-modulates output at reduced depth (gently FM-flavoured)
                        // True FM requires phase modulation before generation; this is a post-gen
                        // ring-mod approximation that gives an FM-adjacent tonal shift.
                        interactSig = oscAout * oscBout * 0.6f;
                        break;
                    case 3: // Phase Push — AM with phase-adjacent blend (subtle, organic)
                        // True phase-push requires feeding oscAout into the phase accumulator;
                        // use amplitude-coupled blend as a safe approximation.
                        interactSig = (oscAout + oscBout) * 0.5f * 0.3f;
                        break;
                    case 4: // Grit Multiply — multiply and quantize
                    {
                        float mult = oscAout * oscBout;
                        float steps = 16.0f;
                        interactSig = std::floor(mult * steps) / steps;
                        break;
                    }
                    }
                    oscBout = lerp(oscBout, oscBout + interactSig, effInteractAmt);
                }

                // --- Sub ---
                voice.sub.setFrequency(freq, subOctave);
                float subOut =
                    voice.sub.processSample() * clamp(effSubLevel + mmDst[7], 0.0f, 1.0f); // mmDst[7]=SubLevel

                // --- Weight Engine ---
                // modEnv can boost weight level when dest=7
                float effWeightLvlMod = clamp(effWeightLvl + modEnvWeightLvl, 0.0f, 1.0f);
                float weightOut = 0.0f;
                if (effWeightLvlMod > 0.001f)
                {
                    voice.weight.setShape(weightShape);
                    voice.weight.setFrequency(freq, weightOctave, weightTune);
                    weightOut = voice.weight.processSample() * effWeightLvlMod;
                }

                // --- Noise (pre-filter routing) ---
                float noisePre = 0.0f, noisePost = 0.0f;
                float effNoiseLvl = clamp(noiseLevel + mmDst[9], 0.0f, 1.0f); // mmDst[9]=NoiseLevel
                if (effNoiseLvl > 0.001f)
                {
                    float noiseSig = voice.noise.process(noiseType, noiseDecay) * effNoiseLvl;
                    switch (noiseRouting)
                    {
                    case 0:
                        noisePre = noiseSig;
                        break; // Pre-Filter
                    case 1:
                        noisePost = noiseSig;
                        break; // Post-Filter
                    case 2:
                        noisePre = noiseSig * 0.5f;
                        noisePost = noiseSig * 0.5f;
                        break; // Parallel
                    case 3:
                        break; // Sidechain (reserved for coupling)
                    }
                }

                // --- Osc Mix ---
                // modEnv can shift A/B balance when dest=6; bipolar: positive pushes toward B
                float effBiteOscMix = clamp(biteOscMix + modEnvOscMix * 0.5f, 0.0f, 1.0f);
                float oscOut =
                    oscAout * (1.0f - effBiteOscMix) + oscBout * effBiteOscMix + subOut + weightOut + noisePre;

                // --- Fur (pre-filter saturation) ---
                // modEnv can add fur warmth when dest=3
                float effFurMod = clamp(effFurAmount + modEnvFur, 0.0f, 1.0f);
                oscOut = voice.fur.process(oscOut, effFurMod);

                // --- Filter Drive (pre-filter) ---
                float effFilterDriveMod = clamp(filterDrive + mmDst[12], 0.0f, 1.0f); // mmDst[12]=FilterDrive
                if (effFilterDriveMod > 0.001f)
                    oscOut = fastTanh(oscOut * (1.0f + effFilterDriveMod * 4.0f));

                // --- Filter ---
                float filtEnvVal = voice.filterEnv.process();
                // Key tracking: offset cutoff based on note distance from middle C
                float keyTrackOffset = filterKeyTrack * (static_cast<float>(voice.noteNumber) - 60.0f) * 50.0f;
                // D001: velocity scales filter envelope depth for timbral expression
                // LFO2 → filter cutoff unconditionally (Scurry scales rate, not presence).
                // LFO3 → filter cutoff unconditionally (wider sweep range).
                // modEnvCutoff is non-zero only when modEnvDest==2 (Filter Cutoff).
                // Scale 4000 Hz gives a musically useful ±4 kHz sweep across the filter
                // envelope range. The previous 10000 Hz drove the cutoff to the 18 kHz
                // ceiling at low base-cutoff values, killing velocity-timbral variation.
                // Decimate SVF coefficient refresh to every 16 samples — env + LFOs
                // still advance per sample (already ticked above), only the expensive
                // filter coefficient update is throttled. ~0.36ms refresh @ 44.1k is
                // well below audible cutoff-tracking lag.
                if (updateFilter)
                {
                    float modCutoff = filterCutoff + filtEnvAmt * filtEnvVal * voice.velocity * 4000.0f + bellyCutoffMod +
                                      playDeadCutoff + filterMod * 2000.0f + lfo2val * 2000.0f + lfo3val * 2000.0f +
                                      modEnvCutoff + keyTrackOffset;
                    modCutoff = clamp(modCutoff, 20.0f, 18000.0f);

                    float voiceFilterReso = clamp(effFilterReso + mmDst[11], 0.0f, 0.95f); // mmDst[11]=FilterReso
                    voice.filter.setCoefficients_fast(modCutoff, voiceFilterReso, srf);
                }
                float filtered = voice.filter.processSample(oscOut);

                // Add post-filter noise
                filtered += noisePost;

                // --- Chew (post-filter contour) ---
                // chewFreq isolates the low-mid band for compression; chewMix blends wet/dry
                float effChewAmtMod = clamp(chewAmount + mmDst[14], 0.0f, 1.0f); // mmDst[14]=Chew
                if (effChewAmtMod > 0.001f)
                {
                    // chewFreq is block-constant (loaded from pChewFreq at block start);
                    // refresh coefficients only when the filter tick fires.
                    if (updateFilter)
                    {
                        voice.chewFilter.setMode(CytomicSVF::Mode::LowPass);
                        voice.chewFilter.setCoefficients_fast(chewFreq, 0.5f, srf);
                    }
                    float chewBand = voice.chewFilter.processSample(filtered);
                    float chewOut = voice.chew.process(chewBand, effChewAmtMod) + (filtered - chewBand);
                    filtered = lerp(filtered, chewOut, chewMix);
                }

                // --- Gnash (asymmetric bite) ---
                // modEnv can intensify gnash when dest=4; use != 0 for bipolar safety
                float effGnashMod = clamp(effGnashAmount + modEnvGnash, 0.0f, 1.0f);
                filtered = voice.gnash.process(filtered, effGnashMod);

                // --- Drive stage ---
                float effDriveAmtMod = clamp(driveAmount + mmDst[15], 0.0f, 1.0f); // mmDst[15]=Drive
                if (effDriveAmtMod > 0.001f)
                {
                    float driveGain = 1.0f + effDriveAmtMod * 6.0f;
                    float driven = filtered * driveGain;
                    float shaped = 0.0f;
                    switch (driveType)
                    {
                    case 0: // Warm — soft tanh
                        shaped = fastTanh(driven);
                        break;
                    case 1: // Grit — soft clip with even harmonics
                        shaped = driven / (1.0f + std::abs(driven));
                        break;
                    case 2: // Clip — hard clip
                        shaped = clamp(driven, -1.0f, 1.0f);
                        break;
                    case 3: // Tube — asymmetric soft clip (positive half clips harder)
                        shaped = (driven >= 0.0f) ? driven / (1.0f + driven * driven * effDriveAmtMod)
                                                  : fastTanh(driven * 0.8f);
                        break;
                    default:
                        shaped = fastTanh(driven);
                        break;
                    }
                    filtered = lerp(filtered, shaped, effDriveAmtMod);
                }

                // --- Trash (dirt modes) ---
                int effTrashMode = trashMode;
                if (macroTrash > 0.9f && trashMode == 0)
                    effTrashMode = 1;
                // modEnv can push trash amount when dest=5; use != 0 for bipolar safety
                float effTrashAmtMod = clamp(effTrashAmount + modEnvTrash, 0.0f, 1.0f);
                filtered = voice.trash.process(filtered, effTrashMode, effTrashAmtMod);

                // --- Amp Envelope ---
                float envVal = voice.ampEnv.process();

                // mmDst[18]=AmpLevel: scales output amplitude (bipolar: 1+mmDst[18])
                float out = filtered * envVal * velGain * playDeadLevel * clamp(1.0f + mmDst[18], 0.0f, 2.0f);

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    voice.releasing = false;
                }

                // Pan (equal-power) — unison voices spread across stereo field
                // mmDst[19]=Pan modulation: bipolar offset added to base pan
                float voicePanBase = clamp(pan + mmDst[19], -1.0f, 1.0f);
                float voicePanL = panGainL;
                float voicePanR = panGainR;
                if (voice.unisonTotal > 1 || std::abs(mmDst[19]) > 0.001f)
                {
                    float unisonPos = (voice.unisonTotal > 1) ? ((static_cast<float>(voice.unisonIndex) -
                                                                  static_cast<float>(voice.unisonTotal - 1) * 0.5f) /
                                                                 static_cast<float>(voice.unisonTotal - 1))
                                                              : 0.0f;
                    float spreadPan = voicePanBase + unisonPos * unisonSpread;
                    spreadPan = clamp(spreadPan, -1.0f, 1.0f);
                    float spreadR = clamp((spreadPan + 1.0f) * 0.5f, 0.0f, 1.0f);
                    float spreadL = 1.0f - spreadR;
                    voicePanL = std::sqrt(spreadL);
                    voicePanR = std::sqrt(spreadR);
                }
                mixL += out * voicePanL;
                mixR += out * voicePanR;

                peakEnv = std::max(peakEnv, envVal);
            }

            // Apply level + soft limiter
            float outL = fastTanh(mixL * level);
            float outR = fastTanh(mixR * level);

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

        // =====================================================================
        // FX Chain: Motion → Echo → Space → Finish
        // Applied block-level to the accumulated stereo mix via outputCache.
        // Mod matrix FX dests (20-25) use first-active-voice sources for block-level control.
        // =====================================================================
        {
            // Resolve FX mod matrix accumulations (block-level sources only)
            float fxMmMotionRate = 0.0f, fxMmMotionDepth = 0.0f;
            float fxMmEchoTime = 0.0f, fxMmEchoFB = 0.0f;
            float fxMmSpaceSize = 0.0f, fxMmSpaceDecay = 0.0f;
            float srcVel = 0.0f, srcNote = 0.0f;
            for (const auto& v : voices)
            {
                if (!v.active)
                    continue;
                srcVel = v.velocity;
                srcNote = (static_cast<float>(v.noteNumber) - 60.0f) / 64.0f;
                break;
            }
            const float fxSrc[16] = {0,
                                     0,
                                     0,
                                     0,
                                     0,
                                     0,
                                     0,
                                     srcVel,
                                     srcNote,
                                     atPressure,
                                     modWheelAmount,
                                     macroBelly,
                                     macroBite,
                                     macroScurry,
                                     macroTrash,
                                     macroPlayDead};
            for (int ms = 0; ms < 8; ++ms)
            {
                const auto& slot = modSlots[ms];
                if (slot.src == 0 || slot.dst < 20 || slot.dst > 25 || std::abs(slot.amt) < 1e-4f)
                    continue;
                float sv = fxSrc[std::min(slot.src, 15)];
                switch (slot.dst)
                {
                case 20:
                    fxMmMotionRate += sv * slot.amt;
                    break;
                case 21:
                    fxMmMotionDepth += sv * slot.amt;
                    break;
                case 22:
                    fxMmEchoTime += sv * slot.amt;
                    break;
                case 23:
                    fxMmEchoFB += sv * slot.amt;
                    break;
                case 24:
                    fxMmSpaceSize += sv * slot.amt;
                    break;
                case 25:
                    fxMmSpaceDecay += sv * slot.amt;
                    break;
                default:
                    break;
                }
            }

            const float effMotionRate = clamp(fxMotionRate + fxMmMotionRate, 0.01f, 10.0f);
            const float effMotionDepth = clamp(fxMotionDepth + fxMmMotionDepth, 0.0f, 1.0f);
            // Echo Sync: when On, snap delay time to the nearest 120 BPM subdivision.
            // Uses the same subdivision table as LFO sync (1/16=0.125s through 1bar=2.0s)
            // so a delay set at 0.5s in free mode lands cleanly on 1/4 note when synced.
            const float rawEchoTime = clamp(fxEchoTime + fxMmEchoTime, 0.01f, 2.0f);
            const float effEchoTime = [&]() -> float
            {
                if (!fxEchoSync)
                    return rawEchoTime;
                // Subdivisions at 120 BPM: 1/16=0.125s, 1/8=0.25s, 1/4=0.5s, 1/2=1.0s, 1bar=2.0s
                constexpr float kEchoSubs[] = {0.125f, 0.25f, 0.5f, 1.0f, 2.0f};
                constexpr int kNSubs = 5;
                float best = kEchoSubs[0];
                float bestDist = std::abs(rawEchoTime - kEchoSubs[0]);
                for (int i = 1; i < kNSubs; ++i)
                {
                    float d = std::abs(rawEchoTime - kEchoSubs[i]);
                    if (d < bestDist)
                    {
                        bestDist = d;
                        best = kEchoSubs[i];
                    }
                }
                return best;
            }();
            const float effEchoFB = clamp(fxEchoFeedback + fxMmEchoFB, 0.0f, 0.95f);
            const float effSpaceSize = clamp(fxSpaceSize + fxMmSpaceSize, 0.0f, 1.0f);
            const float effSpaceDecay = clamp(fxSpaceDecay + fxMmSpaceDecay, 0.1f, 20.0f);

            const bool fxActive = fxMotionMix > 0.001f || fxEchoMix > 0.001f || fxSpaceMix > 0.001f ||
                                  fxFinishGlue > 0.001f || fxFinishClip > 0.001f ||
                                  std::abs(fxFinishWidth - 1.0f) > 0.01f || fxFinishLowMono > 0.001f;

            int nc = buffer.getNumChannels();
            for (int s = 0; s < numSamples; ++s)
            {
                float sL = outputCacheL[static_cast<size_t>(s)];
                float sR = outputCacheR[static_cast<size_t>(s)];

                // Motion (always tick delay line to prevent pops on enable)
                motionFX.process(sL, sR, fxMotionType, effMotionRate, effMotionDepth, fxMotionMix);
                // Echo (always tick to prevent pops on enable)
                echoFX.process(sL, sR, fxEchoType, effEchoTime, effEchoFB, fxEchoMix);
                // Space
                spaceFX.process(sL, sR, fxSpaceType, effSpaceSize, effSpaceDecay, fxSpaceDamping, fxSpaceMix);

                // Finish — only process if any Finish param is non-trivial
                if (fxActive)
                {
                    // Glue: soft-knee bus compression
                    if (fxFinishGlue > 0.001f)
                    {
                        float glueThresh = 1.0f - fxFinishGlue * 0.6f;
                        float ratio = 4.0f + fxFinishGlue * 4.0f; // 4:1 → 8:1
                        auto applyGlue = [&](float x) -> float
                        {
                            float a = std::abs(x);
                            if (a > glueThresh)
                            {
                                float excess = a - glueThresh;
                                float compressed = glueThresh + excess / ratio;
                                return (x >= 0.0f) ? compressed : -compressed;
                            }
                            return x;
                        };
                        sL = applyGlue(sL);
                        sR = applyGlue(sR);
                    }

                    // Clip: output ceiling
                    if (fxFinishClip > 0.001f)
                    {
                        float ceiling = 1.0f - fxFinishClip * 0.3f; // 1.0 → 0.7
                        sL = clamp(sL, -ceiling, ceiling);
                        sR = clamp(sR, -ceiling, ceiling);
                    }

                    // Width: mid/side stereo widening (width=1.0 → unity)
                    if (std::abs(fxFinishWidth - 1.0f) > 0.01f)
                    {
                        float mid = (sL + sR) * 0.5f;
                        float side = (sL - sR) * 0.5f * fxFinishWidth;
                        sL = mid + side;
                        sR = mid - side;
                    }

                    // LowMono: mono-ize the sub-200Hz band for clean bass translation
                    // lowMonoCoeff is precomputed in prepare() — session-constant
                    if (fxFinishLowMono > 0.001f)
                    {
                        lowMonoStateL = flushDenormal(lowMonoStateL * lowMonoCoeff + sL * (1.0f - lowMonoCoeff));
                        lowMonoStateR = flushDenormal(lowMonoStateR * lowMonoCoeff + sR * (1.0f - lowMonoCoeff));
                        float loMono = (lowMonoStateL + lowMonoStateR) * 0.5f;
                        sL = lerp(sL, sL - lowMonoStateL + loMono, fxFinishLowMono);
                        sR = lerp(sR, sR - lowMonoStateR + loMono, fxFinishLowMono);
                    }
                }

                // Write FX-processed samples back, replacing previous un-FX'd values
                if (nc >= 2)
                {
                    buffer.getWritePointer(0)[s] -= outputCacheL[static_cast<size_t>(s)];
                    buffer.getWritePointer(1)[s] -= outputCacheR[static_cast<size_t>(s)];
                    buffer.getWritePointer(0)[s] += sL;
                    buffer.getWritePointer(1)[s] += sR;
                }
                else if (nc == 1)
                {
                    buffer.getWritePointer(0)[s] -=
                        (outputCacheL[static_cast<size_t>(s)] + outputCacheR[static_cast<size_t>(s)]) * 0.5f;
                    buffer.getWritePointer(0)[s] += (sL + sR) * 0.5f;
                }

                // Update cache with post-FX values for coupling readback
                outputCacheL[static_cast<size_t>(s)] = sL;
                outputCacheR[static_cast<size_t>(s)] = sR;
            }
        }

        // Clear coupling buffer pointer after use (consumed)
        externalFMBuffer = nullptr;
        externalFMSamples = 0;

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

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        switch (type)
        {
        case CouplingType::AmpToFilter:
            // Drum hits pump filter cutoff
            externalFilterMod += amount;
            break;

        case CouplingType::AudioToFM:
            // Other engine audio FM-modulates our oscillators
            if (sourceBuffer != nullptr && numSamples > 0)
            {
                externalFMBuffer = sourceBuffer;
                externalFMSamples = numSamples;
                externalFMAmount = amount;
            }
            break;

        default:
            break; // Other coupling types not supported by BITE
        }
    }

    //-- Parameters ------------------------------------------------------------

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // Mod matrix choice arrays (shared across 8 slots)
        static const juce::StringArray modSources{"Off",        "LFO 1",        "LFO 2",       "LFO 3",
                                                  "Amp Env",    "Filter Env",   "Mod Env",     "Velocity",
                                                  "Note",       "Aftertouch",   "Mod Wheel",   "Macro Belly",
                                                  "Macro Bite", "Macro Scurry", "Macro Trash", "Macro Play Dead"};
        static const juce::StringArray modDests{"Off",
                                                "OscA Shape",
                                                "OscA Drift",
                                                "OscB Shape",
                                                "OscB Instability",
                                                "Osc Mix",
                                                "Osc Interact",
                                                "Sub Level",
                                                "Weight Level",
                                                "Noise Level",
                                                "Filter Cutoff",
                                                "Filter Reso",
                                                "Filter Drive",
                                                "Fur",
                                                "Chew",
                                                "Drive",
                                                "Gnash",
                                                "Trash",
                                                "Amp Level",
                                                "Pan",
                                                "FX Motion Rate",
                                                "FX Motion Depth",
                                                "FX Echo Time",
                                                "FX Echo Feedback",
                                                "FX Space Size",
                                                "FX Space Decay"};

        //======================================================================
        // 1. OSCILLATOR A — Belly
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_oscAWaveform", 1}, "Bite Osc A Waveform",
            juce::StringArray{"Sine", "Triangle", "Saw", "Cushion Pulse"}, 0));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_oscAShape", 1}, "Bite Osc A Shape",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_oscADrift", 1}, "Bite Osc A Drift",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.05f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_oscMix", 1}, "Bite Osc Mix",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        //======================================================================
        // 2. OSCILLATOR B — Bite
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_oscBWaveform", 1}, "Bite Osc B Waveform",
            juce::StringArray{"Hard Sync Saw", "FM", "Ring Mod", "Noise", "Grit"}, 0));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_oscBShape", 1}, "Bite Osc B Shape",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_oscBInstability", 1}, "Bite Osc B Instability",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        //======================================================================
        // 3. OSCILLATOR INTERACTION
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_oscInteractMode", 1}, "Bite Osc Interact Mode",
            juce::StringArray{"Off", "Soft Sync", "Low FM", "Phase Push", "Grit Multiply"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_oscInteractAmount", 1}, "Bite Osc Interact Amount",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        //======================================================================
        // 4. SUB OSCILLATOR
        //======================================================================
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_subLevel", 1}, "Bite Sub Level",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_subOctave", 1}, "Bite Sub Octave", juce::StringArray{"-1 Oct", "-2 Oct"}, 0));

        //======================================================================
        // 5. WEIGHT ENGINE
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_weightShape", 1}, "Bite Weight Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "Pulse"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_weightOctave", 1}, "Bite Weight Octave",
            juce::StringArray{"-1 Oct", "-2 Oct", "-3 Oct"}, 1));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_weightLevel", 1}, "Bite Weight Level",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_weightTune", 1}, "Bite Weight Tune",
                                                        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));

        //======================================================================
        // 6. NOISE SOURCE
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_noiseType", 1}, "Bite Noise Type",
            juce::StringArray{"White", "Pink", "Brown", "Crackle", "Hiss"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_noiseRouting", 1}, "Bite Noise Routing",
            juce::StringArray{"Pre-Filter", "Post-Filter", "Parallel", "Sidechain"}, 0));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_noiseLevel", 1}, "Bite Noise Level",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_noiseDecay", 1}, "Bite Noise Decay",
            juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.4f), 0.1f));

        //======================================================================
        // 7. FILTER BLOCK
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_filterCutoff", 1}, "Bite Filter Cutoff",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 2000.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_filterReso", 1}, "Bite Filter Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_filterMode", 1}, "Bite Filter Mode",
            juce::StringArray{"Burrow LP", "Snarl BP", "Wire HP", "Hollow Notch"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_filterKeyTrack", 1}, "Bite Filter Key Track",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_filterDrive", 1}, "Bite Filter Drive",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        //======================================================================
        // 8. CHARACTER STAGES
        //======================================================================
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_furAmount", 1}, "Bite Fur Amount",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_chewAmount", 1}, "Bite Chew Amount",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_chewFreq", 1}, "Bite Chew Freq",
            juce::NormalisableRange<float>(100.0f, 8000.0f, 0.1f, 0.3f), 1000.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_chewMix", 1}, "Bite Chew Mix",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_driveAmount", 1}, "Bite Drive Amount",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_driveType", 1}, "Bite Drive Type",
                                                         juce::StringArray{"Warm", "Grit", "Clip", "Tube"}, 0));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_gnashAmount", 1}, "Bite Gnash Amount",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_trashMode", 1}, "Bite Trash Mode",
                                                         juce::StringArray{"Off", "Rust", "Splatter", "Crushed"}, 0));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_trashAmount", 1}, "Bite Trash Amount",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        //======================================================================
        // 9. AMP ENVELOPE
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_ampAttack", 1}, "Bite Amp Attack",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.005f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_ampDecay", 1}, "Bite Amp Decay",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.3f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_ampSustain", 1}, "Bite Amp Sustain",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_ampRelease", 1}, "Bite Amp Release",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.4f), 0.3f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_ampVelSens", 1}, "Bite Amp Vel Sens",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));

        //======================================================================
        // 10. FILTER ENVELOPE
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_filterEnvAmount", 1}, "Bite Filter Env Amount",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_filterAttack", 1}, "Bite Filter Attack",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.005f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_filterDecay", 1}, "Bite Filter Decay",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_filterSustain", 1}, "Bite Filter Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_filterRelease", 1}, "Bite Filter Release",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.4f), 0.3f));

        //======================================================================
        // 11. MOD ENVELOPE
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_modEnvAmount", 1}, "Bite Mod Env Amount",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_modAttack", 1}, "Bite Mod Attack",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.01f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_modDecay", 1}, "Bite Mod Decay",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_modSustain", 1}, "Bite Mod Sustain",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_modRelease", 1}, "Bite Mod Release",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.4f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_modEnvDest", 1}, "Bite Mod Env Dest",
            juce::StringArray{"OscA Shape", "OscB Shape", "Filter Cutoff", "Fur", "Gnash", "Trash", "Osc Mix",
                              "Weight Level"},
            2));

        //======================================================================
        // 12. LFO 1
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_lfo1Shape", 1}, "Bite LFO 1 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H", "Random", "Stepped"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_lfo1Rate", 1}, "Bite LFO 1 Rate",
            juce::NormalisableRange<float>(0.01f, 50.0f, 0.01f, 0.3f), 1.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_lfo1Depth", 1}, "Bite LFO 1 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_lfo1Sync", 1}, "Bite LFO 1 Sync", juce::StringArray{"Off", "On"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_lfo1Retrigger", 1}, "Bite LFO 1 Retrigger", juce::StringArray{"Off", "On"}, 0));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_lfo1Phase", 1}, "Bite LFO 1 Phase",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        //======================================================================
        // 13. LFO 2
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_lfo2Shape", 1}, "Bite LFO 2 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H", "Random", "Stepped"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_lfo2Rate", 1}, "Bite LFO 2 Rate",
            juce::NormalisableRange<float>(0.01f, 50.0f, 0.01f, 0.3f), 2.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_lfo2Depth", 1}, "Bite LFO 2 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_lfo2Sync", 1}, "Bite LFO 2 Sync", juce::StringArray{"Off", "On"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_lfo2Retrigger", 1}, "Bite LFO 2 Retrigger", juce::StringArray{"Off", "On"}, 0));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_lfo2Phase", 1}, "Bite LFO 2 Phase",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        //======================================================================
        // 14. LFO 3
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_lfo3Shape", 1}, "Bite LFO 3 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H", "Random", "Stepped"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_lfo3Rate", 1}, "Bite LFO 3 Rate",
            juce::NormalisableRange<float>(0.01f, 50.0f, 0.01f, 0.3f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_lfo3Depth", 1}, "Bite LFO 3 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_lfo3Sync", 1}, "Bite LFO 3 Sync", juce::StringArray{"Off", "On"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_lfo3Retrigger", 1}, "Bite LFO 3 Retrigger", juce::StringArray{"Off", "On"}, 0));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_lfo3Phase", 1}, "Bite LFO 3 Phase",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        //======================================================================
        // 15. MOD MATRIX — 8 slots
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot1Src", 1},
                                                                      "Bite Mod 1 Src", modSources, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot1Dst", 1},
                                                                      "Bite Mod 1 Dst", modDests, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_modSlot1Amt", 1}, "Bite Mod 1 Amt",
                                                        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot2Src", 1},
                                                                      "Bite Mod 2 Src", modSources, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot2Dst", 1},
                                                                      "Bite Mod 2 Dst", modDests, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_modSlot2Amt", 1}, "Bite Mod 2 Amt",
                                                        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot3Src", 1},
                                                                      "Bite Mod 3 Src", modSources, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot3Dst", 1},
                                                                      "Bite Mod 3 Dst", modDests, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_modSlot3Amt", 1}, "Bite Mod 3 Amt",
                                                        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot4Src", 1},
                                                                      "Bite Mod 4 Src", modSources, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot4Dst", 1},
                                                                      "Bite Mod 4 Dst", modDests, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_modSlot4Amt", 1}, "Bite Mod 4 Amt",
                                                        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot5Src", 1},
                                                                      "Bite Mod 5 Src", modSources, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot5Dst", 1},
                                                                      "Bite Mod 5 Dst", modDests, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_modSlot5Amt", 1}, "Bite Mod 5 Amt",
                                                        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot6Src", 1},
                                                                      "Bite Mod 6 Src", modSources, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot6Dst", 1},
                                                                      "Bite Mod 6 Dst", modDests, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_modSlot6Amt", 1}, "Bite Mod 6 Amt",
                                                        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot7Src", 1},
                                                                      "Bite Mod 7 Src", modSources, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot7Dst", 1},
                                                                      "Bite Mod 7 Dst", modDests, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_modSlot7Amt", 1}, "Bite Mod 7 Amt",
                                                        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot8Src", 1},
                                                                      "Bite Mod 8 Src", modSources, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_modSlot8Dst", 1},
                                                                      "Bite Mod 8 Dst", modDests, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_modSlot8Amt", 1}, "Bite Mod 8 Amt",
                                                        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

        //======================================================================
        // 16. MACROS — 5
        //======================================================================
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_macroBelly", 1}, "Bite Macro Belly",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_macroBite", 1}, "Bite Macro Bite",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_macroScurry", 1}, "Bite Macro Scurry",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_macroTrash", 1}, "Bite Macro Trash",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_macroPlayDead", 1}, "Bite Macro Play Dead",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        //======================================================================
        // 17. FX: MOTION
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_fxMotionType", 1}, "Bite FX Motion Type",
            juce::StringArray{"Plush Chorus", "Uneasy Doubler", "Oil Flange"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_fxMotionRate", 1}, "Bite FX Motion Rate",
            juce::NormalisableRange<float>(0.01f, 10.0f, 0.01f, 0.3f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_fxMotionDepth", 1}, "Bite FX Motion Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_fxMotionMix", 1}, "Bite FX Motion Mix",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        //======================================================================
        // 18. FX: ECHO
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_fxEchoType", 1}, "Bite FX Echo Type",
            juce::StringArray{"Dark Tape", "Murky Digital", "Short Slap", "Ping"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_fxEchoTime", 1}, "Bite FX Echo Time",
            juce::NormalisableRange<float>(0.01f, 2.0f, 0.001f, 0.5f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_fxEchoFeedback", 1}, "Bite FX Echo Feedback",
            juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.3f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_fxEchoMix", 1}, "Bite FX Echo Mix",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_fxEchoSync", 1}, "Bite FX Echo Sync", juce::StringArray{"Off", "On"}, 0));

        //======================================================================
        // 19. FX: SPACE
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_fxSpaceType", 1}, "Bite FX Space Type",
            juce::StringArray{"Burrow Room", "Fog Chamber", "Drain Hall"}, 0));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_fxSpaceSize", 1}, "Bite FX Space Size",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_fxSpaceDecay", 1}, "Bite FX Space Decay",
            juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.3f), 1.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_fxSpaceDamping", 1}, "Bite FX Space Damping",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_fxSpaceMix", 1}, "Bite FX Space Mix",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        //======================================================================
        // 20. FX: FINISH
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_fxFinishGlue", 1}, "Bite FX Finish Glue",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_fxFinishClip", 1}, "Bite FX Finish Clip",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_fxFinishWidth", 1}, "Bite FX Finish Width",
            juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_fxFinishLowMono", 1}, "Bite FX Finish Low Mono",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        //======================================================================
        // 21. VOICE CONTROL
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_polyphony", 1}, "Bite Polyphony", juce::StringArray{"1", "2", "4", "8", "16"}, 3));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_glideTime", 1}, "Bite Glide Time",
            juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"poss_glideMode", 1},
                                                                      "Bite Glide Mode",
                                                                      juce::StringArray{"Off", "Legato", "Always"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"poss_unisonVoices", 1}, "Bite Unison Voices",
            juce::StringArray{"1", "2", "3", "4", "5", "6", "7"}, 0));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_unisonDetune", 1}, "Bite Unison Detune",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"poss_unisonSpread", 1}, "Bite Unison Spread",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        //======================================================================
        // 22. OUTPUT
        //======================================================================
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_level", 1}, "Bite Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"poss_pan", 1}, "Bite Pan", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));
    }

public:
    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        // Oscillator A
        pOscAWaveform = apvts.getRawParameterValue("poss_oscAWaveform");
        pOscAShape = apvts.getRawParameterValue("poss_oscAShape");
        pOscADrift = apvts.getRawParameterValue("poss_oscADrift");
        pOscMix = apvts.getRawParameterValue("poss_oscMix");
        // Oscillator B
        pOscBWaveform = apvts.getRawParameterValue("poss_oscBWaveform");
        pOscBShape = apvts.getRawParameterValue("poss_oscBShape");
        pOscBInstability = apvts.getRawParameterValue("poss_oscBInstability");
        // Osc Interaction
        pOscInteractMode = apvts.getRawParameterValue("poss_oscInteractMode");
        pOscInteractAmount = apvts.getRawParameterValue("poss_oscInteractAmount");
        // Sub
        pSubLevel = apvts.getRawParameterValue("poss_subLevel");
        pSubOctave = apvts.getRawParameterValue("poss_subOctave");
        // Weight Engine
        pWeightShape = apvts.getRawParameterValue("poss_weightShape");
        pWeightOctave = apvts.getRawParameterValue("poss_weightOctave");
        pWeightLevel = apvts.getRawParameterValue("poss_weightLevel");
        pWeightTune = apvts.getRawParameterValue("poss_weightTune");
        // Noise
        pNoiseType = apvts.getRawParameterValue("poss_noiseType");
        pNoiseRouting = apvts.getRawParameterValue("poss_noiseRouting");
        pNoiseLevel = apvts.getRawParameterValue("poss_noiseLevel");
        pNoiseDecay = apvts.getRawParameterValue("poss_noiseDecay");
        // Filter
        pFilterCutoff = apvts.getRawParameterValue("poss_filterCutoff");
        pFilterReso = apvts.getRawParameterValue("poss_filterReso");
        pFilterMode = apvts.getRawParameterValue("poss_filterMode");
        pFilterKeyTrack = apvts.getRawParameterValue("poss_filterKeyTrack");
        pFilterDrive = apvts.getRawParameterValue("poss_filterDrive");
        // Character stages
        pFurAmount = apvts.getRawParameterValue("poss_furAmount");
        pChewAmount = apvts.getRawParameterValue("poss_chewAmount");
        pChewFreq = apvts.getRawParameterValue("poss_chewFreq");
        pChewMix = apvts.getRawParameterValue("poss_chewMix");
        pDriveAmount = apvts.getRawParameterValue("poss_driveAmount");
        pDriveType = apvts.getRawParameterValue("poss_driveType");
        pGnashAmount = apvts.getRawParameterValue("poss_gnashAmount");
        pTrashMode = apvts.getRawParameterValue("poss_trashMode");
        pTrashAmount = apvts.getRawParameterValue("poss_trashAmount");
        // Amp Envelope
        pAmpAttack = apvts.getRawParameterValue("poss_ampAttack");
        pAmpDecay = apvts.getRawParameterValue("poss_ampDecay");
        pAmpSustain = apvts.getRawParameterValue("poss_ampSustain");
        pAmpRelease = apvts.getRawParameterValue("poss_ampRelease");
        pAmpVelSens = apvts.getRawParameterValue("poss_ampVelSens");
        // Filter Envelope
        pFilterEnvAmount = apvts.getRawParameterValue("poss_filterEnvAmount");
        pFilterAttack = apvts.getRawParameterValue("poss_filterAttack");
        pFilterDecay = apvts.getRawParameterValue("poss_filterDecay");
        pFilterSustain = apvts.getRawParameterValue("poss_filterSustain");
        pFilterRelease = apvts.getRawParameterValue("poss_filterRelease");
        // Mod Envelope
        pModEnvAmount = apvts.getRawParameterValue("poss_modEnvAmount");
        pModAttack = apvts.getRawParameterValue("poss_modAttack");
        pModDecay = apvts.getRawParameterValue("poss_modDecay");
        pModSustain = apvts.getRawParameterValue("poss_modSustain");
        pModRelease = apvts.getRawParameterValue("poss_modRelease");
        pModEnvDest = apvts.getRawParameterValue("poss_modEnvDest");
        // LFO 1
        pLfo1Shape = apvts.getRawParameterValue("poss_lfo1Shape");
        pLfo1Rate = apvts.getRawParameterValue("poss_lfo1Rate");
        pLfo1Depth = apvts.getRawParameterValue("poss_lfo1Depth");
        pLfo1Sync = apvts.getRawParameterValue("poss_lfo1Sync");
        pLfo1Retrigger = apvts.getRawParameterValue("poss_lfo1Retrigger");
        pLfo1Phase = apvts.getRawParameterValue("poss_lfo1Phase");
        // LFO 2
        pLfo2Shape = apvts.getRawParameterValue("poss_lfo2Shape");
        pLfo2Rate = apvts.getRawParameterValue("poss_lfo2Rate");
        pLfo2Depth = apvts.getRawParameterValue("poss_lfo2Depth");
        pLfo2Sync = apvts.getRawParameterValue("poss_lfo2Sync");
        pLfo2Retrigger = apvts.getRawParameterValue("poss_lfo2Retrigger");
        pLfo2Phase = apvts.getRawParameterValue("poss_lfo2Phase");
        // LFO 3
        pLfo3Shape = apvts.getRawParameterValue("poss_lfo3Shape");
        pLfo3Rate = apvts.getRawParameterValue("poss_lfo3Rate");
        pLfo3Depth = apvts.getRawParameterValue("poss_lfo3Depth");
        pLfo3Sync = apvts.getRawParameterValue("poss_lfo3Sync");
        pLfo3Retrigger = apvts.getRawParameterValue("poss_lfo3Retrigger");
        pLfo3Phase = apvts.getRawParameterValue("poss_lfo3Phase");
        // Mod Matrix (8 slots × 3)
        pModSlot1Src = apvts.getRawParameterValue("poss_modSlot1Src");
        pModSlot1Dst = apvts.getRawParameterValue("poss_modSlot1Dst");
        pModSlot1Amt = apvts.getRawParameterValue("poss_modSlot1Amt");
        pModSlot2Src = apvts.getRawParameterValue("poss_modSlot2Src");
        pModSlot2Dst = apvts.getRawParameterValue("poss_modSlot2Dst");
        pModSlot2Amt = apvts.getRawParameterValue("poss_modSlot2Amt");
        pModSlot3Src = apvts.getRawParameterValue("poss_modSlot3Src");
        pModSlot3Dst = apvts.getRawParameterValue("poss_modSlot3Dst");
        pModSlot3Amt = apvts.getRawParameterValue("poss_modSlot3Amt");
        pModSlot4Src = apvts.getRawParameterValue("poss_modSlot4Src");
        pModSlot4Dst = apvts.getRawParameterValue("poss_modSlot4Dst");
        pModSlot4Amt = apvts.getRawParameterValue("poss_modSlot4Amt");
        pModSlot5Src = apvts.getRawParameterValue("poss_modSlot5Src");
        pModSlot5Dst = apvts.getRawParameterValue("poss_modSlot5Dst");
        pModSlot5Amt = apvts.getRawParameterValue("poss_modSlot5Amt");
        pModSlot6Src = apvts.getRawParameterValue("poss_modSlot6Src");
        pModSlot6Dst = apvts.getRawParameterValue("poss_modSlot6Dst");
        pModSlot6Amt = apvts.getRawParameterValue("poss_modSlot6Amt");
        pModSlot7Src = apvts.getRawParameterValue("poss_modSlot7Src");
        pModSlot7Dst = apvts.getRawParameterValue("poss_modSlot7Dst");
        pModSlot7Amt = apvts.getRawParameterValue("poss_modSlot7Amt");
        pModSlot8Src = apvts.getRawParameterValue("poss_modSlot8Src");
        pModSlot8Dst = apvts.getRawParameterValue("poss_modSlot8Dst");
        pModSlot8Amt = apvts.getRawParameterValue("poss_modSlot8Amt");
        // Macros
        pMacroBelly = apvts.getRawParameterValue("poss_macroBelly");
        pMacroBite = apvts.getRawParameterValue("poss_macroBite");
        pMacroScurry = apvts.getRawParameterValue("poss_macroScurry");
        pMacroTrash = apvts.getRawParameterValue("poss_macroTrash");
        pMacroPlayDead = apvts.getRawParameterValue("poss_macroPlayDead");
        // FX: Motion
        pFxMotionType = apvts.getRawParameterValue("poss_fxMotionType");
        pFxMotionRate = apvts.getRawParameterValue("poss_fxMotionRate");
        pFxMotionDepth = apvts.getRawParameterValue("poss_fxMotionDepth");
        pFxMotionMix = apvts.getRawParameterValue("poss_fxMotionMix");
        // FX: Echo
        pFxEchoType = apvts.getRawParameterValue("poss_fxEchoType");
        pFxEchoTime = apvts.getRawParameterValue("poss_fxEchoTime");
        pFxEchoFeedback = apvts.getRawParameterValue("poss_fxEchoFeedback");
        pFxEchoMix = apvts.getRawParameterValue("poss_fxEchoMix");
        pFxEchoSync = apvts.getRawParameterValue("poss_fxEchoSync");
        // FX: Space
        pFxSpaceType = apvts.getRawParameterValue("poss_fxSpaceType");
        pFxSpaceSize = apvts.getRawParameterValue("poss_fxSpaceSize");
        pFxSpaceDecay = apvts.getRawParameterValue("poss_fxSpaceDecay");
        pFxSpaceDamping = apvts.getRawParameterValue("poss_fxSpaceDamping");
        pFxSpaceMix = apvts.getRawParameterValue("poss_fxSpaceMix");
        // FX: Finish
        pFxFinishGlue = apvts.getRawParameterValue("poss_fxFinishGlue");
        pFxFinishClip = apvts.getRawParameterValue("poss_fxFinishClip");
        pFxFinishWidth = apvts.getRawParameterValue("poss_fxFinishWidth");
        pFxFinishLowMono = apvts.getRawParameterValue("poss_fxFinishLowMono");
        // Voice
        pPolyphony = apvts.getRawParameterValue("poss_polyphony");
        pGlideTime = apvts.getRawParameterValue("poss_glideTime");
        pGlideMode = apvts.getRawParameterValue("poss_glideMode");
        pUnisonVoices = apvts.getRawParameterValue("poss_unisonVoices");
        pUnisonDetune = apvts.getRawParameterValue("poss_unisonDetune");
        pUnisonSpread = apvts.getRawParameterValue("poss_unisonSpread");
        // Output
        pLevel = apvts.getRawParameterValue("poss_level");
        pPan = apvts.getRawParameterValue("poss_pan");
    }

    //-- Identity --------------------------------------------------------------

    juce::String getEngineId() const override { return "Overbite"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFF0EDE8); }
    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override
    {
        int count = 0;
        for (const auto& v : voices)
            if (v.active)
                ++count;
        return count;
    }

private:
    SilenceGate silenceGate;

    //--------------------------------------------------------------------------
    // Safe parameter loading helpers (avoid nullptr dereference)
    static int safeLoad(std::atomic<float>* p, int fallback) noexcept
    {
        return (p != nullptr) ? static_cast<int>(p->load()) : fallback;
    }

    static float safeLoadF(std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    static float midiToFreq(int note) noexcept
    {
        return 440.0f * fastPow2((static_cast<float>(note) - 69.0f) * (1.0f / 12.0f));
    }

    //--------------------------------------------------------------------------
    void initVoiceSlot(BiteVoice& v, int noteNumber, float velocity, int midiChannel, int unisonIdx, int unisonTotal)
    {
        float prevFreq = v.hasPlayedBefore ? v.glide.getFreq() : midiToFreq(noteNumber);

        v.active = true;
        v.releasing = false; // Clear releasing flag — new noteOn takes this slot
        v.noteNumber = noteNumber;
        v.velocity = velocity;
        v.startTime = ++voiceCounter;
        v.unisonIndex = unisonIdx;
        v.unisonTotal = unisonTotal;

        v.mpeExpression.reset();
        v.mpeExpression.midiChannel = midiChannel;
        if (mpeManager != nullptr)
            mpeManager->updateVoiceExpression(v.mpeExpression);

        float freq = midiToFreq(noteNumber);
        v.oscA.reset();
        v.oscA.setFrequency(freq);
        v.oscB.reset();
        v.oscB.setFrequency(freq);
        v.sub.reset();
        v.weight.reset();
        v.noise.reset();
        v.noise.noteOn();
        v.filter.reset();
        v.chewFilter.reset();
        v.trash.reset();
        if (lfo1RetriggerFlag)
        {
            v.lfo1.setStartPhase(safeLoadF(pLfo1Phase, 0.0f));
            v.lfo1.retrigger();
        }
        else
            v.lfo1.reset();
        if (lfo2RetriggerFlag)
        {
            v.lfo2.setStartPhase(safeLoadF(pLfo2Phase, 0.0f));
            v.lfo2.retrigger();
        }
        else
            v.lfo2.reset();
        if (lfo3RetriggerFlag)
        {
            v.lfo3.setStartPhase(safeLoadF(pLfo3Phase, 0.0f));
            v.lfo3.retrigger();
        }
        else
            v.lfo3.reset();

        v.glide.setTarget(freq);
        if (v.hasPlayedBefore)
            v.glide.currentFreq = prevFreq;
        else
            v.glide.snapTo(freq);
        v.hasPlayedBefore = true;

        v.ampEnv.noteOn();
        v.filterEnv.noteOn();
        v.modEnv.noteOn();
    }

    void noteOn(int noteNumber, float velocity, int midiChannel, int maxPoly)
    {
        int nUnison = std::min(unisonVoicesFlag, maxPoly);
        nUnison = std::max(nUnison, 1);
        int poly = std::min(maxPoly, kMaxVoices);

        for (int ui = 0; ui < nUnison; ++ui)
        {
            // Prefer stealing releasing voices over actively-sustaining ones.
            // This prevents polyphony exhaustion during long release tails — a
            // 2s release on an 8-voice poly patch no longer blocks new notes.
            int idx = VoiceAllocator::findFreeVoicePreferRelease(voices, poly,
                                                                 [](const BiteVoice& v) { return v.releasing; });
            initVoiceSlot(voices[static_cast<size_t>(idx)], noteNumber, velocity, midiChannel, ui, nUnison);
        }
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
                v.filterEnv.noteOff();
                v.modEnv.noteOff();
                // Mark as releasing so VoiceAllocator can steal this slot first
                // when all poly slots are full. Releasing voices are always preferred
                // over actively-sustaining voices for stealing (see noteOn).
                v.releasing = true;
            }
        }
    }

    void allVoicesOff()
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.releasing = false;
            v.ampEnv.reset();
            v.filterEnv.reset();
            v.modEnv.reset();
        }
    }

    // Voice allocation delegated to VoiceAllocator::findFreeVoice() — called in noteOn().

    //--------------------------------------------------------------------------
    double sr = 0.0;
    float srf = 0.0f;
    std::array<BiteVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0; // Monotonic counter for VoiceAllocator LRU stealing

    // Coupling state
    float envelopeOutput = 0.0f;
    float externalFilterMod = 0.0f;
    const float* externalFMBuffer = nullptr;
    int externalFMSamples = 0;
    float externalFMAmount = 0.0f;

    // LFO retrigger flags — cached from params each block, read by noteOn
    bool lfo1RetriggerFlag = false;
    bool lfo2RetriggerFlag = false;
    bool lfo3RetriggerFlag = false;
    // Unison voice count — cached from params each block, read by noteOn
    int unisonVoicesFlag = 1;

    // ---- D006 Aftertouch — pressure intensifies BITE macro (feral aggression) ----
    PolyAftertouch aftertouch;

    // ---- D006 Mod wheel — CC#1 adds to BITE macro depth (+0–0.4, feral bite edge) ----
    float modWheelAmount = 0.0f;
    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // FX DSP objects (Motion/Echo/Space/Finish chain)
    BiteMotionFX motionFX;
    BiteEchoFX echoFX;
    BiteSpaceFX spaceFX;

    // FX Finish: LowMono 1-pole LP filter state (persistent between blocks)
    float lowMonoStateL = 0.0f;
    float lowMonoStateR = 0.0f;
    float lowMonoCoeff = 0.0f; // cached exp(-2π*200/sr) — computed in prepare()

    // Cached APVTS parameter pointers — 122 total, frozen IDs
    // Oscillator A
    std::atomic<float>* pOscAWaveform = nullptr;
    std::atomic<float>* pOscAShape = nullptr;
    std::atomic<float>* pOscADrift = nullptr;
    std::atomic<float>* pOscMix = nullptr;
    // Oscillator B
    std::atomic<float>* pOscBWaveform = nullptr;
    std::atomic<float>* pOscBShape = nullptr;
    std::atomic<float>* pOscBInstability = nullptr;
    // Osc Interaction
    std::atomic<float>* pOscInteractMode = nullptr;
    std::atomic<float>* pOscInteractAmount = nullptr;
    // Sub
    std::atomic<float>* pSubLevel = nullptr;
    std::atomic<float>* pSubOctave = nullptr;
    // Weight Engine
    std::atomic<float>* pWeightShape = nullptr;
    std::atomic<float>* pWeightOctave = nullptr;
    std::atomic<float>* pWeightLevel = nullptr;
    std::atomic<float>* pWeightTune = nullptr;
    // Noise
    std::atomic<float>* pNoiseType = nullptr;
    std::atomic<float>* pNoiseRouting = nullptr;
    std::atomic<float>* pNoiseLevel = nullptr;
    std::atomic<float>* pNoiseDecay = nullptr;
    // Filter
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterReso = nullptr;
    std::atomic<float>* pFilterMode = nullptr;
    std::atomic<float>* pFilterKeyTrack = nullptr;
    std::atomic<float>* pFilterDrive = nullptr;
    // Character stages
    std::atomic<float>* pFurAmount = nullptr;
    std::atomic<float>* pChewAmount = nullptr;
    std::atomic<float>* pChewFreq = nullptr;
    std::atomic<float>* pChewMix = nullptr;
    std::atomic<float>* pDriveAmount = nullptr;
    std::atomic<float>* pDriveType = nullptr;
    std::atomic<float>* pGnashAmount = nullptr;
    std::atomic<float>* pTrashMode = nullptr;
    std::atomic<float>* pTrashAmount = nullptr;
    // Amp Envelope
    std::atomic<float>* pAmpAttack = nullptr;
    std::atomic<float>* pAmpDecay = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;
    std::atomic<float>* pAmpVelSens = nullptr;
    // Filter Envelope
    std::atomic<float>* pFilterEnvAmount = nullptr;
    std::atomic<float>* pFilterAttack = nullptr;
    std::atomic<float>* pFilterDecay = nullptr;
    std::atomic<float>* pFilterSustain = nullptr;
    std::atomic<float>* pFilterRelease = nullptr;
    // Mod Envelope
    std::atomic<float>* pModEnvAmount = nullptr;
    std::atomic<float>* pModAttack = nullptr;
    std::atomic<float>* pModDecay = nullptr;
    std::atomic<float>* pModSustain = nullptr;
    std::atomic<float>* pModRelease = nullptr;
    std::atomic<float>* pModEnvDest = nullptr;
    // LFO 1
    std::atomic<float>* pLfo1Shape = nullptr;
    std::atomic<float>* pLfo1Rate = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo1Sync = nullptr;
    std::atomic<float>* pLfo1Retrigger = nullptr;
    std::atomic<float>* pLfo1Phase = nullptr;
    // LFO 2
    std::atomic<float>* pLfo2Shape = nullptr;
    std::atomic<float>* pLfo2Rate = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;
    std::atomic<float>* pLfo2Sync = nullptr;
    std::atomic<float>* pLfo2Retrigger = nullptr;
    std::atomic<float>* pLfo2Phase = nullptr;
    // LFO 3
    std::atomic<float>* pLfo3Shape = nullptr;
    std::atomic<float>* pLfo3Rate = nullptr;
    std::atomic<float>* pLfo3Depth = nullptr;
    std::atomic<float>* pLfo3Sync = nullptr;
    std::atomic<float>* pLfo3Retrigger = nullptr;
    std::atomic<float>* pLfo3Phase = nullptr;
    // Mod Matrix (8 slots)
    std::atomic<float>* pModSlot1Src = nullptr;
    std::atomic<float>* pModSlot1Dst = nullptr;
    std::atomic<float>* pModSlot1Amt = nullptr;
    std::atomic<float>* pModSlot2Src = nullptr;
    std::atomic<float>* pModSlot2Dst = nullptr;
    std::atomic<float>* pModSlot2Amt = nullptr;
    std::atomic<float>* pModSlot3Src = nullptr;
    std::atomic<float>* pModSlot3Dst = nullptr;
    std::atomic<float>* pModSlot3Amt = nullptr;
    std::atomic<float>* pModSlot4Src = nullptr;
    std::atomic<float>* pModSlot4Dst = nullptr;
    std::atomic<float>* pModSlot4Amt = nullptr;
    std::atomic<float>* pModSlot5Src = nullptr;
    std::atomic<float>* pModSlot5Dst = nullptr;
    std::atomic<float>* pModSlot5Amt = nullptr;
    std::atomic<float>* pModSlot6Src = nullptr;
    std::atomic<float>* pModSlot6Dst = nullptr;
    std::atomic<float>* pModSlot6Amt = nullptr;
    std::atomic<float>* pModSlot7Src = nullptr;
    std::atomic<float>* pModSlot7Dst = nullptr;
    std::atomic<float>* pModSlot7Amt = nullptr;
    std::atomic<float>* pModSlot8Src = nullptr;
    std::atomic<float>* pModSlot8Dst = nullptr;
    std::atomic<float>* pModSlot8Amt = nullptr;
    // Macros
    std::atomic<float>* pMacroBelly = nullptr;
    std::atomic<float>* pMacroBite = nullptr;
    std::atomic<float>* pMacroScurry = nullptr;
    std::atomic<float>* pMacroTrash = nullptr;
    std::atomic<float>* pMacroPlayDead = nullptr;
    // FX: Motion
    std::atomic<float>* pFxMotionType = nullptr;
    std::atomic<float>* pFxMotionRate = nullptr;
    std::atomic<float>* pFxMotionDepth = nullptr;
    std::atomic<float>* pFxMotionMix = nullptr;
    // FX: Echo
    std::atomic<float>* pFxEchoType = nullptr;
    std::atomic<float>* pFxEchoTime = nullptr;
    std::atomic<float>* pFxEchoFeedback = nullptr;
    std::atomic<float>* pFxEchoMix = nullptr;
    std::atomic<float>* pFxEchoSync = nullptr;
    // FX: Space
    std::atomic<float>* pFxSpaceType = nullptr;
    std::atomic<float>* pFxSpaceSize = nullptr;
    std::atomic<float>* pFxSpaceDecay = nullptr;
    std::atomic<float>* pFxSpaceDamping = nullptr;
    std::atomic<float>* pFxSpaceMix = nullptr;
    // FX: Finish
    std::atomic<float>* pFxFinishGlue = nullptr;
    std::atomic<float>* pFxFinishClip = nullptr;
    std::atomic<float>* pFxFinishWidth = nullptr;
    std::atomic<float>* pFxFinishLowMono = nullptr;
    // Voice
    std::atomic<float>* pPolyphony = nullptr;
    std::atomic<float>* pGlideTime = nullptr;
    std::atomic<float>* pGlideMode = nullptr;
    std::atomic<float>* pUnisonVoices = nullptr;
    std::atomic<float>* pUnisonDetune = nullptr;
    std::atomic<float>* pUnisonSpread = nullptr;
    // Output
    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>* pPan = nullptr;
};

} // namespace xoceanus
