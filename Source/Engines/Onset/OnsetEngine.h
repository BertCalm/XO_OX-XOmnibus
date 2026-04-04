// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OnsetEngine.h — XOnset | "The Surface Splash"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOnset lives at the water's surface — the boundary where air meets
//      water, where feliX's electric transients ripple outward. Every trigger
//      is a splash, a stone skipping across the meniscus. Pure percussion.
//      Pure feliX. Second generation: feliX's transients, isolated and
//      amplified into a dedicated species of rhythmic life.
//
//  ENGINE CONCEPT:
//      The drum machine that morphs between an 808 and a physics simulation
//      on every hit. Each of 8 dedicated voices holds two synthesis paradigms:
//
//          Layer X (Circuit)    — Analog warmth: TR-808/909 topology modeling.
//                                 Bridged-T resonators, noise burst generators,
//                                 6-oscillator metallic networks.
//
//          Layer O (Algorithm)  — Digital complexity: FM synthesis (Yamaha DX),
//                                 modal resonators (physical modeling), Karplus-
//                                 Strong plucked strings, Casio CZ-style phase
//                                 distortion.
//
//      A continuous Blend axis crossfades between these worlds using equal-
//      power (cos/sin) curves. Cross-Voice Coupling (XVC) makes the kit
//      interact like neurons in a rhythm brain — kick opens the snare filter,
//      snare tightens the hat decay.
//
//  HISTORICAL LINEAGE:
//      Layer X circuits channel the Roland TR-808 (1980) bridged-T oscillator
//      topology and the TR-909's hybrid noise/oscillator snare. The metallic
//      oscillator bank recreates the 808's 6-square-wave hi-hat network with
//      non-harmonic frequency ratios.
//
//      Layer O algorithms draw from the Yamaha DX7 (1983) FM synthesis, the
//      Casio CZ-101 (1984) phase distortion, and Stanford CCRMA's physical
//      modeling research (Karplus-Strong, 1983; modal synthesis).
//
//  SIGNAL FLOW:
//      MIDI Trigger -> Transient Designer (snap: pitch spike + noise burst)
//                   -> [Layer X (Circuit) | Layer O (Algorithm)] -> BLEND
//                   -> Per-Voice SVF Filter -> Amp Envelope -> Voice Output
//                   -> Voice Mixer (level, pan) -> Cross-Voice Coupling Matrix
//                   -> Character Stage (Grit/Warmth)
//                   -> FX Rack [Delay -> Reverb -> LoFi]
//                   -> Master Output
//
//  ACCENT COLOR: Electric Blue #0066FF
//  PARAMETER PREFIX: onset_ (internal: perc_)
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/ModMatrix.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/PolyBLEP.h"
#include <array>
#include <cmath>
#include <cstring>

namespace xoceanus
{

//==============================================================================
//
//  I. PRIMITIVES — Low-level building blocks
//
//==============================================================================

//==============================================================================
// OnsetNoiseGen — xorshift32 PRNG for percussion noise.
//
// George Marsaglia's xorshift (2003): the shift constants 13, 17, 5 form a
// maximal-period generator with 2^32-1 states. Faster than LCG, better spectral
// distribution than rand(). The signed reinterpret maps [0, 2^32) to [-1, +1)
// as a cheap float conversion without division by UINT_MAX.
//==============================================================================
class OnsetNoiseGen
{
public:
    void seed(uint32_t s) noexcept { state = s ? s : 1; }

    float process() noexcept
    {
        // Marsaglia xorshift32 — shift triple (13, 17, 5) for full-period cycle
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        // Reinterpret as signed int32, divide by 2^31 to map to [-1.0, +1.0)
        return static_cast<float>(static_cast<int32_t>(state)) / 2147483648.0f;
    }

private:
    uint32_t state = 1;
};

//==============================================================================
// OnsetEnvelope — AD/AHD/ADSR percussion envelope with coefficient caching.
//
// Percussion envelopes are the heartbeat of drum synthesis. Three shapes serve
// different rhythmic needs:
//   AD   — Attack-Decay: the default for most hits. No sustain, no release.
//   AHD  — Attack-Hold-Decay: adds a plateau for body (toms, claps).
//   ADSR — Attack-Decay-Sustain-Release: for sustained/gated sounds.
//
// The decay coefficient is cached (lastDecay) so that repeated triggers at the
// same decay time skip the expensive exp() recomputation.
//==============================================================================
class OnsetEnvelope
{
public:
    enum class Stage
    {
        Idle,
        Attack,
        Hold,
        Decay,
        Sustain,
        Release
    };
    enum class Shape
    {
        AD = 0,
        AHD = 1,
        ADSR = 2
    };

    void prepare(double sampleRate) noexcept { sr = static_cast<float>(sampleRate); }

    void trigger(float attackSec, float decaySec, Shape shape = Shape::AD, float holdSec = 0.0f,
                 float sustainLvl = 0.0f) noexcept
    {
        currentShape = shape;
        sustainLevel = sustainLvl;
        stage = Stage::Attack;
        level = 0.0f;

        // Minimum 0.1ms attack to avoid division-by-zero / DC click
        float aSec = std::max(attackSec, 0.0001f);
        attackRate = 1.0f / (sr * aSec);

        holdSamplesLeft = (shape != Shape::AD) ? std::max(0, static_cast<int>(sr * holdSec)) : 0;

        // Minimum 1ms decay to prevent coefficient underflow
        float dSec = std::max(decaySec, 0.001f);
        if (dSec != lastDecay)
        {
            lastDecay = dSec;
            // -4.6 = ln(0.01): the envelope reaches 1% of its initial value
            // after exactly dSec seconds, giving a natural exponential decay.
            decayCoeff = 1.0f - std::exp(-4.6f / (sr * dSec));
        }
    }

    float process() noexcept
    {
        switch (stage)
        {
        case Stage::Idle:
            return 0.0f;
        case Stage::Attack:
            level += attackRate;
            if (level >= 1.0f)
            {
                level = 1.0f;
                stage = (holdSamplesLeft > 0) ? Stage::Hold : Stage::Decay;
            }
            return level;
        case Stage::Hold:
            if (--holdSamplesLeft <= 0)
                stage = Stage::Decay;
            return level;
        case Stage::Decay:
            if (currentShape == Shape::ADSR && sustainLevel > 0.0f)
            {
                level -= (level - sustainLevel) * decayCoeff;
                // Denormal protection: exponential decay toward sustain creates
                // vanishingly small deltas that trigger x87 FPU slow-path (~100x penalty).
                level = flushDenormal(level);
                // 0.001 threshold: close enough to sustain to snap without audible jump
                if (level <= sustainLevel + 0.001f)
                {
                    level = sustainLevel;
                    stage = Stage::Sustain;
                }
            }
            else
            {
                level -= level * decayCoeff;
                // Denormal protection: same concern — decay toward zero is the
                // classic denormal trap in audio DSP.
                level = flushDenormal(level);
                // 1e-6 (~-120dB): well below audible threshold, safe to cut to zero
                if (level < 1e-6f)
                {
                    level = 0.0f;
                    stage = Stage::Idle;
                }
            }
            return level;
        case Stage::Sustain:
            return level;
        case Stage::Release:
            level -= level * decayCoeff;
            // Denormal protection: release tail decays exponentially toward zero
            level = flushDenormal(level);
            if (level < 1e-6f)
            {
                level = 0.0f;
                stage = Stage::Idle;
            }
            return level;
        }
        return 0.0f;
    }

    void release() noexcept
    {
        if (stage == Stage::Sustain || stage == Stage::Hold || stage == Stage::Decay)
            stage = Stage::Release;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }
    float getLevel() const noexcept { return level; }
    void reset() noexcept
    {
        stage = Stage::Idle;
        level = 0.0f;
        lastDecay = -1.0f;
        holdSamplesLeft = 0;
    }

private:
    float sr = 44100.0f;
    Stage stage = Stage::Idle;
    Shape currentShape = Shape::AD;
    float level = 0.0f;
    float attackRate = 0.01f;
    float decayCoeff = 0.001f;
    float lastDecay = -1.0f;
    float sustainLevel = 0.0f;
    int holdSamplesLeft = 0;
};

//==============================================================================
// OnsetTransient — Pre-blend click/snap: pitch spike + noise burst.
//
// The "splash" of XOnset's surface identity. A brief, bright transient that
// precedes the main voice body. Combines a pitched sine spike (attack click)
// with a noise burst (snap), both scaled by the snap parameter and velocity.
// This models the initial contact energy of a drumstick hitting a membrane —
// the sharp click before the resonance takes over.
//==============================================================================
class OnsetTransient
{
public:
    void prepare(double sampleRate) noexcept { sr = static_cast<float>(sampleRate); }

    void trigger(float snapAmount, float baseFreqHz) noexcept
    {
        if (snapAmount < 0.01f)
        {
            active = false;
            return;
        }
        active = true;
        snapLevel = snapAmount;
        phase = 0.0f;

        // Pitch spike: 4x-16x the base frequency (higher snap = higher pitch click).
        // This emulates the initial membrane impact burst heard in real drum recordings.
        spikeFrequency = baseFreqHz * (4.0f + snapAmount * 12.0f);

        // Fix 3: Extend spike window from 1-6ms to 2-14ms so the transient sits above
        // the psychoacoustic integration threshold (~2ms) for all snap values.
        // Higher snap = shorter, tighter click (same direction, wider range).
        spikeSamplesRemaining = static_cast<int>(sr * (0.002f + (1.0f - snapAmount) * 0.012f));

        // Fix 3: Extend noise window from 1-3ms to 2-8ms for the same reason.
        // Higher snap = longer noise burst (more "snap" character, same direction).
        noiseSamplesRemaining = static_cast<int>(sr * (0.002f + snapAmount * 0.006f));
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;
        float out = 0.0f;

        if (spikeSamplesRemaining > 0)
        {
            phase += spikeFrequency / sr;
            while (phase >= 1.0f)
                phase -= 1.0f;
            // 6.28318... = 2*pi; 0.4 = spike level relative to main voice body
            out += fastSin(phase * 6.28318530718f) * snapLevel * 0.4f;
            --spikeSamplesRemaining;
        }

        if (noiseSamplesRemaining > 0)
        {
            // 0.25 = noise level: quieter than spike to sit behind the tonal click
            out += noise.process() * snapLevel * 0.25f;
            --noiseSamplesRemaining;
        }

        if (spikeSamplesRemaining <= 0 && noiseSamplesRemaining <= 0)
            active = false;
        return out;
    }

    void reset() noexcept
    {
        active = false;
        spikeSamplesRemaining = 0;
        noiseSamplesRemaining = 0;
    }

private:
    float sr = 44100.0f;
    bool active = false;
    float snapLevel = 0.0f;
    float phase = 0.0f;
    float spikeFrequency = 1000.0f;
    int spikeSamplesRemaining = 0;
    int noiseSamplesRemaining = 0;
    OnsetNoiseGen noise;
};

//==============================================================================
//
//  II. LAYER X — CIRCUIT MODELS
//
//  The analog heritage. These modules model the discrete component topologies
//  of classic Roland drum machines. In the XO_OX mythology, Layer X is feliX's
//  contribution — surface energy, electric transients, the familiar warmth of
//  analog circuits.
//
//==============================================================================

//==============================================================================
// BridgedTOsc — TR-808 kick/tom: decaying sine + pitch envelope + sub.
//
// Heritage: The Roland TR-808 (1980) kick circuit uses a bridged-T oscillator
// network — a passive notch filter driven into self-oscillation by a trigger
// pulse. The pitch sweeps down from the initial impulse (the characteristic
// 808 "boom"). We model this as a sine oscillator with an exponential pitch
// envelope that sweeps from spikeFreq down to baseFreq.
//
// The sub-oscillator (triangle wave, one octave below) adds low-end weight,
// mimicking the sub-harmonic reinforcement common in modern kick synthesis.
//==============================================================================
class BridgedTOsc
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void trigger(float freqHz, float snapAmt, float bodyAmt, float charAmt) noexcept
    {
        baseFreq = static_cast<double>(freqHz);
        subOscLevel = bodyAmt;
        saturationAmount = charAmt;

        // 48 semitones = 4 octaves: maximum pitch sweep range from snap.
        // At full snap, the pitch spike starts 4 octaves above the fundamental,
        // producing the classic 808 "chirp" attack.
        pitchEnvelopeSemitones = snapAmt * 48.0f;

        // Pitch envelope decay: 5-50ms. Higher snap = faster decay (tighter chirp).
        double pitchDecayMs = 5.0 + (1.0 - static_cast<double>(snapAmt)) * 45.0;
        pitchDecayCoeff = 1.0f - fastExp(static_cast<float>(-1.0 / (sr * pitchDecayMs * 0.001)));

        phase = 0.0;
        subPhase = 0.0;
        active = true;
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;

        // Exponential pitch envelope decay
        pitchEnvelopeSemitones *= (1.0f - pitchDecayCoeff);
        // Denormal protection: pitch envelope decays toward zero — classic denormal trap
        pitchEnvelopeSemitones = flushDenormal(pitchEnvelopeSemitones);

        // Convert semitones to frequency multiplier.
        // 0.693147 = ln(2): semitones-to-ratio via exp(st * ln(2)/12).
        // Skip multiplication when envelope is essentially settled (< 0.01 semitones).
        float freqMultiplier =
            (pitchEnvelopeSemitones > 0.01f) ? fastExp(pitchEnvelopeSemitones * (0.693147f / 12.0f)) : 1.0f;

        double freq = baseFreq * static_cast<double>(freqMultiplier);
        double phaseIncrement = freq / sr;

        // Main oscillator (sine wave — the 808 bridged-T fundamental)
        phase += phaseIncrement;
        while (phase >= 1.0)
            phase -= 1.0;
        // 6.28318... = 2*pi
        float out = fastSin(static_cast<float>(phase) * 6.28318530718f);

        // Sub oscillator (triangle wave, one octave below via 0.5x increment)
        subPhase += phaseIncrement * 0.5;
        while (subPhase >= 1.0)
            subPhase -= 1.0;
        float triangle = 2.0f * std::abs(2.0f * static_cast<float>(subPhase) - 1.0f) - 1.0f;
        out += triangle * subOscLevel;

        // Diode-starvation saturation: tanh soft-clip with drive scaled by character.
        // 4.0 max drive multiplier chosen to match 808 output stage clipping behavior.
        if (saturationAmount > 0.01f)
            out = fastTanh(out * (1.0f + saturationAmount * 4.0f));

        return out;
    }

    void reset() noexcept
    {
        phase = 0.0;
        subPhase = 0.0;
        pitchEnvelopeSemitones = 0.0f;
        active = false;
    }
    bool isActive() const noexcept { return active; }

private:
    double sr = 44100.0;
    double baseFreq = 55.0; // A1 — typical 808 kick fundamental
    double phase = 0.0;
    double subPhase = 0.0;
    float pitchEnvelopeSemitones = 0.0f; // Current pitch offset in semitones (decaying)
    float pitchDecayCoeff = 0.01f;       // One-pole decay coefficient for pitch envelope
    float subOscLevel = 0.5f;            // Sub-oscillator mix level (0=off, 1=full)
    float saturationAmount = 0.0f;       // Diode-starvation drive amount
    bool active = false;
};

//==============================================================================
// NoiseBurstCircuit — Snare/clap: dual sine body + filtered noise.
//
// Heritage: The 808 snare uses two bridged-T oscillators (tuned ~180Hz and
// ~330Hz for body resonance) mixed with high-pass filtered noise (the snare
// wire rattle). The 909 added a second noise source with a sharper filter
// for more "crack." We combine both approaches with a tone balance knob.
//
// Clap mode re-triggers the noise burst 3 times at 10ms intervals, recreating
// the 808's distinctive multi-burst handclap circuit (3 retriggered VCAs
// feeding a shared reverb in the original).
//==============================================================================
class NoiseBurstCircuit
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        noiseHPF.setMode(CytomicSVF::Mode::HighPass);
        reset();
    }

    void trigger(float freqHz, float snapAmt, float toneAmt, float charAmt, bool isClap) noexcept
    {
        // Dual body oscillators: 180Hz and 330Hz are the 808 snare resonant frequencies.
        // The pitch parameter offsets both, weighted 30%/70% to maintain the interval ratio.
        bodyOsc1Freq = 180.0 + static_cast<double>(freqHz - 180.0f) * 0.3;
        bodyOsc2Freq = 330.0 + static_cast<double>(freqHz - 180.0f) * 0.7;
        toneBalance = toneAmt;

        // Noise envelope: 10-50ms burst. Higher snap = longer noise for more "crack."
        noiseEnvelopeLevel = 1.0f;
        float noiseDurationSec = 0.01f + snapAmt * 0.04f;
        // NOTE: actual T63 time constant = noiseDurationSec * 0.368 (not noiseDurationSec).
        // The factor 0.368 (≈ 1/e) converts the labeled duration to a one-pole coefficient
        // so the envelope decays to 1/e² in noiseDurationSec, producing a faster perceived snap.
        noiseDecayRate = 1.0f - fastExp(static_cast<float>(-1.0 / (sr * noiseDurationSec * 0.368)));

        // Body pitch sweep: up to 12 semitones (1 octave) downward chirp
        bodyPitchSemitones = snapAmt * 12.0f;
        float pitchDecayMs = 5.0f + (1.0f - snapAmt) * 20.0f;
        bodyPitchCoeff = 1.0f - fastExp(static_cast<float>(-1.0 / (sr * pitchDecayMs * 0.001)));

        // Noise HPF: 2-8kHz. Higher character = more "wire" and less "body" in the noise.
        float noiseHighpassCutoff = 2000.0f + charAmt * 6000.0f;
        noiseHPF.setCoefficients(noiseHighpassCutoff, 0.0f, static_cast<float>(sr));

        bodyPhase1 = 0.0;
        bodyPhase2 = 0.0;
        active = true;
        clapMode = isClap;
        // 808 clap: 3 re-triggers at 10ms intervals (original used 3 retriggered VCAs)
        clapBurstsRemaining = isClap ? 3 : 0;
        clapSampleCounter = 0;
        clapBurstIntervalSamples = static_cast<int>(sr * 0.01);
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;

        // Body pitch envelope decay
        bodyPitchSemitones *= (1.0f - bodyPitchCoeff);
        bodyPitchSemitones = flushDenormal(bodyPitchSemitones);

        // 0.693147 = ln(2): semitone-to-ratio conversion
        float pitchMultiplier = (bodyPitchSemitones > 0.01f) ? fastExp(bodyPitchSemitones * (0.693147f / 12.0f)) : 1.0f;

        // Dual body oscillators (sine waves at ~180Hz and ~330Hz)
        bodyPhase1 += (bodyOsc1Freq * static_cast<double>(pitchMultiplier)) / sr;
        while (bodyPhase1 >= 1.0)
            bodyPhase1 -= 1.0;
        bodyPhase2 += (bodyOsc2Freq * static_cast<double>(pitchMultiplier)) / sr;
        while (bodyPhase2 >= 1.0)
            bodyPhase2 -= 1.0;

        float body = (fastSin(static_cast<float>(bodyPhase1) * 6.28318530718f) +
                      fastSin(static_cast<float>(bodyPhase2) * 6.28318530718f)) *
                     0.5f;

        // Noise burst with exponential decay
        noiseEnvelopeLevel -= noiseEnvelopeLevel * noiseDecayRate;
        // Denormal protection: noise envelope decays toward zero
        noiseEnvelopeLevel = flushDenormal(noiseEnvelopeLevel);
        float noiseOutput = noiseHPF.processSample(noise.process()) * noiseEnvelopeLevel;

        // Clap multi-burst: re-trigger noise at 10ms intervals
        if (clapMode && clapBurstsRemaining > 0)
        {
            if (++clapSampleCounter >= clapBurstIntervalSamples)
            {
                clapSampleCounter = 0;
                --clapBurstsRemaining;
                // 0.8: each re-trigger is slightly quieter than the initial burst
                noiseEnvelopeLevel = 0.8f;
            }
        }

        // Mix body oscillators with noise based on tone balance
        return body * (1.0f - toneBalance) + noiseOutput * toneBalance;
    }

    void reset() noexcept
    {
        bodyPhase1 = 0.0;
        bodyPhase2 = 0.0;
        noiseEnvelopeLevel = 0.0f;
        bodyPitchSemitones = 0.0f;
        active = false;
        clapBurstsRemaining = 0;
        noiseHPF.reset();
    }
    bool isActive() const noexcept { return active; }

private:
    double sr = 44100.0;
    double bodyOsc1Freq = 180.0; // 808 snare body resonance 1 (~180Hz)
    double bodyOsc2Freq = 330.0; // 808 snare body resonance 2 (~330Hz)
    double bodyPhase1 = 0.0;
    double bodyPhase2 = 0.0;
    float toneBalance = 0.5f;        // Body vs. noise mix (0=all body, 1=all noise)
    float noiseEnvelopeLevel = 0.0f; // Current noise burst amplitude
    float noiseDecayRate = 0.01f;    // One-pole decay coefficient for noise envelope
    float bodyPitchSemitones = 0.0f; // Current body pitch offset (decaying)
    float bodyPitchCoeff = 0.01f;    // Pitch envelope decay coefficient
    bool active = false;
    bool clapMode = false;
    int clapBurstsRemaining = 0;        // Remaining re-trigger bursts (808 clap)
    int clapSampleCounter = 0;          // Counter between clap bursts
    int clapBurstIntervalSamples = 441; // ~10ms at 44.1kHz between clap bursts
    OnsetNoiseGen noise;
    CytomicSVF noiseHPF;
};

//==============================================================================
// MetallicOsc — 808-style 6-oscillator metallic hat/cymbal.
//
// Heritage: The TR-808 hi-hat circuit uses 6 square-wave oscillators at non-
// harmonic frequencies (derived from the original's resistor network values),
// summed and filtered through two bandpass filters and a highpass. The non-
// harmonic ratios create the characteristic metallic, inharmonic shimmer that
// distinguishes the 808 hat from pitched instruments.
//
// The 6 base frequencies (205.3, 304.4, 369.6, 522.7, 800.0, 1048.0 Hz) are
// measured from TR-808 schematics and match the ratios produced by the original
// resistor divider network driving the hex inverter oscillators.
//==============================================================================
class MetallicOsc
{
public:
    static constexpr int kNumMetallicOscillators = 6;

    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        lowBandpass.setMode(CytomicSVF::Mode::BandPass);
        highBandpass.setMode(CytomicSVF::Mode::BandPass);
        outputHighpass.setMode(CytomicSVF::Mode::HighPass);
        reset();
    }

    void trigger(float pitchRatio, float toneAmt, float charAmt) noexcept
    {
        // TR-808 hex inverter oscillator frequencies (Hz), measured from schematics.
        // These non-harmonic ratios produce the metallic, bell-like timbre.
        static constexpr float kBaseFrequencies[kNumMetallicOscillators] = {205.3f, 304.4f, 369.6f,
                                                                            522.7f, 800.0f, 1048.0f};

        for (int i = 0; i < kNumMetallicOscillators; ++i)
        {
            oscillatorFrequencies[i] = kBaseFrequencies[i] * pitchRatio;
            oscillatorPhases[i] = 0.0;
        }

        // Pulse width: 0.5 (square) to 0.2 (narrow pulse). Narrower = brighter, more harmonics.
        pulseWidth = 0.5f - charAmt * 0.3f;
        toneBalance = toneAmt;

        float sampleRateFloat = static_cast<float>(sr);
        // 3440Hz: the 808's "low metallic" bandpass center frequency
        lowBandpass.setCoefficients(3440.0f * pitchRatio, 0.7f, sampleRateFloat);
        // 7100Hz: the 808's "high metallic" bandpass center frequency
        // Clamped to 0.45 * sr to prevent filter instability near Nyquist
        highBandpass.setCoefficients(std::min(7100.0f * pitchRatio, sampleRateFloat * 0.45f), 0.7f, sampleRateFloat);
        // 6000Hz HPF: removes low-frequency rumble, keeps only metallic shimmer
        outputHighpass.setCoefficients(std::min(6000.0f * pitchRatio, sampleRateFloat * 0.45f), 0.0f, sampleRateFloat);

        active = true;
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;

        // Sum 6 band-limited pulse waves at non-harmonic frequencies.
        // PolyBLEP correction is applied at both the rising edge (phase wraps
        // through 0) and the falling edge (phase crosses pulseWidth) to suppress
        // aliasing in the 4–20 kHz metallic shimmer band.
        float sum = 0.0f;
        for (int i = 0; i < kNumMetallicOscillators; ++i)
        {
            oscillatorPhases[i] += static_cast<double>(oscillatorFrequencies[i]) / sr;
            while (oscillatorPhases[i] >= 1.0)
                oscillatorPhases[i] -= 1.0;

            const float phase = static_cast<float>(oscillatorPhases[i]);
            const float dt = static_cast<float>(oscillatorFrequencies[i]) / static_cast<float>(sr);

            // Naive pulse: +1 for phase < pulseWidth, -1 otherwise
            float osc = (phase < pulseWidth) ? 1.0f : -1.0f;

            // PolyBLEP at the rising edge (discontinuity at phase == 0)
            osc += PolyBLEP::polyBLEP(phase, dt);

            // PolyBLEP at the falling edge (discontinuity at phase == pulseWidth).
            // Shift phase so the falling discontinuity maps to the 0-crossing of
            // the BLEP kernel: t = fmod(phase - pulseWidth + 1.0f, 1.0f)
            {
                float t = phase - pulseWidth;
                if (t < 0.0f)
                    t += 1.0f;
                osc -= PolyBLEP::polyBLEP(t, dt);
            }

            sum += osc;
        }
        // Normalize by oscillator count to prevent clipping
        sum *= (1.0f / static_cast<float>(kNumMetallicOscillators));

        // Dual bandpass filtering: low band (body) and high band (shimmer)
        float lowBand = lowBandpass.processSample(sum);
        float highBand = highBandpass.processSample(sum);
        float mixed = lowBand * (1.0f - toneBalance) + highBand * toneBalance;

        // Final HPF removes any remaining low-frequency content
        return outputHighpass.processSample(mixed);
    }

    void reset() noexcept
    {
        for (auto& p : oscillatorPhases)
            p = 0.0;
        active = false;
        lowBandpass.reset();
        highBandpass.reset();
        outputHighpass.reset();
    }
    bool isActive() const noexcept { return active; }

private:
    double sr = 44100.0;
    float oscillatorFrequencies[kNumMetallicOscillators] = {};
    double oscillatorPhases[kNumMetallicOscillators] = {};
    float toneBalance = 0.5f; // Low-band vs. high-band metallic mix
    float pulseWidth = 0.5f;  // Square wave pulse width (0.2-0.5)
    bool active = false;
    CytomicSVF lowBandpass;    // ~3440Hz: 808 "low metallic" resonance
    CytomicSVF highBandpass;   // ~7100Hz: 808 "high metallic" shimmer
    CytomicSVF outputHighpass; // ~6000Hz: removes sub/bass from metallic output
};

//==============================================================================
//
//  III. LAYER O — ALGORITHMIC SYNTHESIS
//
//  The digital frontier. These modules implement mathematical synthesis
//  techniques that have no analog circuit equivalent. In the XO_OX mythology,
//  Layer O is Oscar's contribution — depth, complexity, the alien territory
//  beyond the familiar shore. When blended with Layer X, the 808 meets the
//  physics engine.
//
//==============================================================================

//==============================================================================
// FMPercussion — 2-operator FM with self-feedback.
//
// Heritage: Yamaha DX7 (1983). Two-operator FM with a carrier and modulator
// at a fixed 1.4:1 ratio (chosen for its metallic, bell-like quality — not
// a simple harmonic ratio, producing inharmonic sidebands ideal for percussion).
// Self-feedback on the carrier output adds grit and complexity, channeling
// the DX7's operator 6 feedback loop.
//==============================================================================
class FMPercussion
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void trigger(float freqHz, float snapAmt, float charAmt, float toneAmt) noexcept
    {
        carrierFreq = static_cast<double>(freqHz);

        // 1.4:1 carrier-to-modulator ratio: an inharmonic ratio that produces
        // metallic, bell-like sidebands. Common in DX7 percussion patches.
        modulatorRatio = 1.4;

        // Modulation index 0-8: controls FM brightness.
        // 8.0 max chosen to stay within single-reflection sideband territory
        // (higher indices create aliasing without oversampling).
        // Nyquist guard: reduce FM index as carrier approaches Nyquist to prevent
        // aliasing on high-pitched hat/cymbal voices at standard sample rates.
        {
            float nyquistGuard =
                std::min(1.0f, static_cast<float>(sr * 0.4) / (static_cast<float>(carrierFreq) + 1.0f));
            modulationIndex = charAmt * 8.0f * nyquistGuard;
        }
        modulatorEnvelopeLevel = 1.0f;

        // Modulator envelope decay: 5-105ms. Higher snap = faster decay (sharper attack).
        // NOTE: actual T63 time constant = modulatorDecaySec * 0.368 (not modulatorDecaySec).
        // The factor 0.368 (≈ 1/e) converts the labeled duration to a one-pole coefficient
        // so the envelope decays to 1/e² in modulatorDecaySec, producing a faster perceived decay.
        float modulatorDecaySec = 0.005f + (1.0f - snapAmt) * 0.1f;
        modulatorEnvelopeDecayRate =
            static_cast<float>(1.0 - std::exp(-1.0 / (sr * static_cast<double>(modulatorDecaySec) * 0.368)));

        // Self-feedback: 0-30% of carrier output fed back into its own phase.
        // 0.3 max prevents runaway but adds enough grit for metallic character.
        feedbackAmount = toneAmt * 0.3f;

        carrierPhase = 0.0;
        modulatorPhase = 0.0;
        lastCarrierOutput = 0.0f;
        active = true;
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;

        // Modulator envelope decay (brightness fades over time — the FM "pluck")
        modulatorEnvelopeLevel -= modulatorEnvelopeLevel * modulatorEnvelopeDecayRate;
        // Denormal protection: modulator envelope decays toward zero
        modulatorEnvelopeLevel = flushDenormal(modulatorEnvelopeLevel);

        // Modulator oscillator
        modulatorPhase += (carrierFreq * modulatorRatio) / sr;
        while (modulatorPhase >= 1.0)
            modulatorPhase -= 1.0;
        float modulatorSignal =
            fastSin(static_cast<float>(modulatorPhase) * 6.28318530718f) * modulationIndex * modulatorEnvelopeLevel;

        // Carrier oscillator with phase modulation from modulator + self-feedback
        carrierPhase += carrierFreq / sr;
        while (carrierPhase >= 1.0)
            carrierPhase -= 1.0;
        float out = fastSin(static_cast<float>(carrierPhase) * 6.28318530718f + modulatorSignal +
                            lastCarrierOutput * feedbackAmount);

        // Denormal protection on feedback path: without this, the feedback loop
        // accumulates subnormal values that cause 100x CPU penalty on x87 FPU.
        lastCarrierOutput = flushDenormal(out);
        return out;
    }

    void reset() noexcept
    {
        carrierPhase = 0.0;
        modulatorPhase = 0.0;
        modulatorEnvelopeLevel = 0.0f;
        lastCarrierOutput = 0.0f;
        active = false;
    }
    bool isActive() const noexcept { return active; }

private:
    double sr = 44100.0;
    double carrierFreq = 200.0;
    double carrierPhase = 0.0;
    double modulatorPhase = 0.0;
    double modulatorRatio = 1.4;              // Carrier:modulator frequency ratio
    float modulationIndex = 1.0f;             // FM depth (0-8)
    float modulatorEnvelopeLevel = 0.0f;      // Decaying modulator amplitude
    float modulatorEnvelopeDecayRate = 0.01f; // One-pole decay coefficient
    float feedbackAmount = 0.0f;              // Self-feedback gain (0-0.3)
    float lastCarrierOutput = 0.0f;           // Previous carrier output for feedback
    bool active = false;
};

//==============================================================================
// ModalResonator — 8-mode parallel bandpass resonator bank.
//
// Heritage: Physical modeling via modal synthesis (Adrien, 1991). A struck
// membrane or bar is modeled as a bank of parallel bandpass filters, each
// tuned to a vibrational mode. The membrane ratios below are solutions to the
// Bessel function zeros for a circular drumhead — the physics that gives toms,
// congas, and timpani their characteristic non-harmonic spectra.
//
// The "character" knob controls inharmonicity: at 0 the modes follow exact
// Bessel ratios (natural membrane), at 1 the higher modes are stretched
// (metallic bar/plate behavior, approaching xylophone/marimba territory).
//==============================================================================
class ModalResonator
{
public:
    static constexpr int kNumModes = 8;

    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        for (auto& resonator : resonators)
            resonator.setMode(CytomicSVF::Mode::BandPass);
        reset();
    }

    void trigger(float baseFreqHz, float charAmt, float toneAmt, float snapAmt) noexcept
    {
        // Circular membrane Bessel function zero ratios (modes 0,1 through 0,8).
        // These are the resonant frequency ratios of an ideal circular drumhead.
        // Source: Kinsler & Frey, "Fundamentals of Acoustics" (Table 9.3).
        static constexpr float kMembraneRatios[kNumModes] = {1.0f, 1.59f, 2.14f, 2.30f, 2.65f, 2.92f, 3.16f, 3.50f};

        // Inharmonicity: stretches higher modes to simulate stiffer materials.
        // 1.0 = ideal membrane, 1.5 = stiff bar/plate.
        float inharmonicityFactor = 1.0f + charAmt * 0.5f;
        // QA C3: compute one log for all 8 modes (replaces 8 separate pow() calls)
        float logInharmonicity = std::log(inharmonicityFactor);

        for (int i = 0; i < kNumModes; ++i)
        {
            // Each higher mode is progressively stretched by the inharmonicity factor
            float modeRatio = kMembraneRatios[i] * fastExp(logInharmonicity * (static_cast<float>(i) * 0.1f));
            float modeFreq = clamp(baseFreqHz * modeRatio, 20.0f, sr * 0.45f);

            // Q decreases for higher modes: fundamental rings longest, overtones decay faster
            // 0.99 base Q: very resonant (near self-oscillation). -0.02 per mode.
            float modeQ = 0.99f - static_cast<float>(i) * 0.02f;
            resonators[i].setCoefficients(modeFreq, modeQ, sr);

            // Mode amplitudes: tone controls fundamental-vs-overtone balance
            float modePosition = static_cast<float>(i) / static_cast<float>(kNumModes - 1);
            modeAmplitudes[i] = lerp(1.0f - modePosition * 0.5f, modePosition + 0.3f, toneAmt);
        }

        // Excitation: noise burst fed into resonator bank (models the strike impulse)
        excitationLevel = 1.0f;
        float excitationDurationSec = 0.001f + (1.0f - snapAmt) * 0.01f;
        // NOTE: actual T63 time constant = excitationDurationSec * 0.368 (not excitationDurationSec).
        // The factor 0.368 (≈ 1/e) converts the labeled duration to a one-pole coefficient
        // so the excitation decays to 1/e² in excitationDurationSec, producing a faster transient.
        excitationDecay = 1.0f - fastExp(-1.0f / (sr * excitationDurationSec * 0.368f));
        active = true;
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;

        // Excitation envelope decay
        excitationLevel -= excitationLevel * excitationDecay;
        // Denormal protection: excitation decays toward zero
        excitationLevel = flushDenormal(excitationLevel);

        // Feed noise impulse through all resonant modes in parallel
        float excitationSample = noise.process() * excitationLevel;
        float sum = 0.0f;
        for (int i = 0; i < kNumModes; ++i)
            sum += resonators[i].processSample(excitationSample * modeAmplitudes[i]);

        // Normalize by mode count to prevent clipping
        return sum * (1.0f / static_cast<float>(kNumModes));
    }

    void reset() noexcept
    {
        for (auto& r : resonators)
            r.reset();
        excitationLevel = 0.0f;
        active = false;
    }
    bool isActive() const noexcept { return active; }

private:
    float sr = 44100.0f;
    std::array<CytomicSVF, kNumModes> resonators;
    float modeAmplitudes[kNumModes] = {}; // Per-mode gain weights
    float excitationLevel = 0.0f;         // Current excitation impulse amplitude
    float excitationDecay = 0.01f;        // One-pole decay for excitation envelope
    bool active = false;
    OnsetNoiseGen noise; // Strike impulse noise source
};

//==============================================================================
// KarplusStrongPerc — Plucked/struck string delay line + averaging filter.
//
// Heritage: Karplus & Strong (1983, Stanford CCRMA). The simplest physical
// model — fill a delay line with noise (the "pluck"), then feed it back
// through an averaging lowpass filter. Each trip around the loop attenuates
// high frequencies, producing the characteristic decay from bright attack to
// warm sustain heard in plucked strings.
//
// The blend factor adds a probabilistic sign flip in the feedback path,
// transitioning from clean string (blend=1) to snare-like buzz (blend=0),
// mimicking the snare wire rattle of Kevin Karplus's original extension.
//==============================================================================
class KarplusStrongPerc
{
public:
    // 4096 samples: supports fundamentals down to ~10.7Hz at 44.1kHz (sub-bass).
    // Must be power of 2 for bitmask wraparound (writePos & (kMaxDelay - 1)).
    static constexpr int kMaxDelaySamples = 4096;

    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void trigger(float freqHz, float snapAmt, float toneAmt, float charAmt, float decayAmt) noexcept
    {
        // Delay length = period of the fundamental (sr / freq)
        float delaySamplesFloat = static_cast<float>(sr) / std::max(freqHz, 20.0f);
        delayLength = std::min(std::max(static_cast<int>(delaySamplesFloat), 1), kMaxDelaySamples - 1);

        // Initial excitation: fill first N samples with noise (the "pluck" impulse).
        // Burst length: 1-11ms. Higher snap = longer noise burst (brighter attack).
        int burstLength = std::min(delayLength, std::max(1, static_cast<int>(sr * (0.001f + snapAmt * 0.01f))));
        for (int i = 0; i < kMaxDelaySamples; ++i)
            delayBuffer[i] = (i < burstLength) ? noise.process() : 0.0f;
        writePosition = 0;

        // Averaging filter coefficient: controls brightness decay rate.
        // Higher tone = more high-frequency content preserved per loop trip.
        averagingFilterCoeff = 0.3f + toneAmt * 0.5f;

        // Blend factor: string (1.0) to snare-drum buzz (0.0).
        // At low values, random sign flips add snare-wire character.
        stringToSnareBlend = charAmt;

        // Feedback gain: 0.9-0.999. Controls sustain length.
        // 0.999 max: longer than this risks infinite sustain / DC buildup.
        feedbackGain = clamp(0.95f + decayAmt * 0.049f, 0.9f, 0.999f);

        lastFilterOutput = 0.0f;
        active = true;
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;

        int readPosition = writePosition - delayLength;
        if (readPosition < 0)
            readPosition += kMaxDelaySamples;

        float currentSample = delayBuffer[readPosition];

        // One-pole averaging filter: y[n] = a*x[n] + (1-a)*y[n-1]
        // This is the Karplus-Strong lowpass that gives each loop trip its
        // characteristic high-frequency rolloff.
        float filtered = averagingFilterCoeff * currentSample + (1.0f - averagingFilterCoeff) * lastFilterOutput;
        // Denormal protection: feedback loop is the classic denormal accumulation site
        filtered = flushDenormal(filtered);

        // Probabilistic sign flip: random polarity inversion in feedback path.
        // At low blend values, this adds snare-wire buzz character.
        if (stringToSnareBlend < 0.5f && noise.process() > stringToSnareBlend * 2.0f)
            filtered = -filtered;

        delayBuffer[writePosition] = filtered * feedbackGain;
        lastFilterOutput = filtered;
        // Bitmask wraparound (requires kMaxDelaySamples to be power of 2)
        writePosition = (writePosition + 1) & (kMaxDelaySamples - 1);

        // 1e-7 (~-140dB): auto-deactivate when signal is effectively silent
        if (std::abs(filtered) < 1e-7f && std::abs(currentSample) < 1e-7f)
        {
            active = false;
            return 0.0f;
        }

        return filtered;
    }

    void reset() noexcept
    {
        std::memset(delayBuffer, 0, sizeof(delayBuffer));
        writePosition = 0;
        lastFilterOutput = 0.0f;
        active = false;
    }
    bool isActive() const noexcept { return active; }

private:
    double sr = 44100.0;
    float delayBuffer[kMaxDelaySamples] = {};
    int delayLength = 100; // Period of fundamental in samples
    int writePosition = 0;
    float averagingFilterCoeff = 0.5f; // One-pole LP coefficient (brightness)
    float feedbackGain = 0.99f;        // Loop gain (sustain length)
    float stringToSnareBlend = 0.5f;   // 1.0=clean string, 0.0=snare buzz
    float lastFilterOutput = 0.0f;     // Previous filter output for one-pole LP
    bool active = false;
    OnsetNoiseGen noise;
};

//==============================================================================
// PhaseDistPerc — Casio CZ-inspired phase distortion percussion.
//
// Heritage: Casio CZ-101 (1984). Phase distortion synthesis warps the phase
// accumulator of a sine oscillator, reshaping the waveform without changing
// the fundamental frequency. The DCW (Digitally Controlled Waveform) parameter
// controls the amount of phase warping — at 0 the output is a pure sine, at
// maximum it approaches a sawtooth-like waveform with rich harmonics.
//
// Three waveshape zones:
//   tone < 0.33 : resonant sine (classic CZ "resonance" character)
//   tone 0.33-0.67: rectified sine (full-wave rectified, octave-up effect)
//   tone > 0.67 : hard-clipped sine (square-ish, aggressive)
//==============================================================================
class PhaseDistPerc
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void trigger(float freqHz, float snapAmt, float charAmt, float toneAmt) noexcept
    {
        frequency = static_cast<double>(freqHz);
        phase = 0.0;

        // DCW (Digitally Controlled Waveform): the CZ's signature parameter.
        // Controls phase distortion depth — how much the sine is "bent."
        dcwAmount = charAmt;
        dcwEnvelopeLevel = snapAmt;

        // DCW envelope decay: 10-110ms. Higher snap = faster decay (sharper transient).
        // NOTE: actual T63 time constant = dcwDecaySec * 0.368 (not dcwDecaySec).
        // The factor 0.368 (≈ 1/e) converts the labeled duration to a one-pole coefficient
        // so the envelope decays to 1/e² in dcwDecaySec, producing a faster perceived snap.
        float dcwDecaySec = 0.01f + (1.0f - snapAmt) * 0.1f;
        dcwEnvelopeDecayRate =
            static_cast<float>(1.0 - std::exp(-1.0 / (sr * static_cast<double>(dcwDecaySec) * 0.368)));

        // Wave shape selector (continuous): sine / rectified / clipped zones
        waveShapePosition = toneAmt;
        active = true;
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;

        // DCW envelope decay
        dcwEnvelopeLevel -= dcwEnvelopeLevel * dcwEnvelopeDecayRate;
        // Denormal protection: DCW envelope decays toward zero
        dcwEnvelopeLevel = flushDenormal(dcwEnvelopeLevel);

        // Early-out when DCW envelope is silent and no base DCW —
        // avoids unnecessary phase distortion math when output would be plain sine
        if (dcwEnvelopeLevel < 1e-6f && dcwAmount < 0.01f)
            return 0.0f;

        float totalDCW = clamp(dcwAmount + dcwEnvelopeLevel, 0.0f, 2.0f);

        phase += frequency / sr;
        while (phase >= 1.0)
            phase -= 1.0;
        float linearPhase = static_cast<float>(phase);

        // Phase distortion: warp the linear phase ramp into a non-linear curve.
        // The first half-cycle is stretched (faster traversal = higher harmonics),
        // the second half is compressed (slower = fundamental reinforced).
        // This is the core CZ algorithm from Casio's 1984 patent.
        float distortedPhase;
        if (totalDCW > 0.01f)
        {
            float distortionDepth = totalDCW / (1.0f + totalDCW);
            distortedPhase = (linearPhase < 0.5f)
                                 ? linearPhase * (0.5f + distortionDepth) / 0.5f
                                 : (0.5f + distortionDepth) + (linearPhase - 0.5f) * (0.5f - distortionDepth) / 0.5f;
            distortedPhase = clamp(distortedPhase, 0.0f, 1.0f);
        }
        else
        {
            distortedPhase = linearPhase;
        }

        // Apply sine waveshaping to distorted phase
        float out = fastSin(distortedPhase * 6.28318530718f);

        // Waveshape zones: three timbral characters
        if (waveShapePosition > 0.33f && waveShapePosition < 0.67f)
            out = 2.0f * std::abs(out) - 1.0f; // Full-wave rectified (octave-up)
        else if (waveShapePosition >= 0.67f)
            out = clamp(out * 2.0f, -1.0f, 1.0f); // Hard-clipped (square-ish)

        return out;
    }

    void reset() noexcept
    {
        phase = 0.0;
        dcwEnvelopeLevel = 0.0f;
        active = false;
    }
    bool isActive() const noexcept { return active; }

private:
    double sr = 44100.0;
    double frequency = 200.0;
    double phase = 0.0;
    float dcwAmount = 0.0f;             // Base DCW depth (Casio's "resonance" amount)
    float dcwEnvelopeLevel = 0.0f;      // Decaying DCW modulation (transient brightness)
    float dcwEnvelopeDecayRate = 0.01f; // One-pole decay coefficient for DCW envelope
    float waveShapePosition = 0.0f;     // 0-0.33: sine, 0.33-0.67: rectified, 0.67+: clipped
    bool active = false;
};

//==============================================================================
//
//  IV. POST-PROCESSING — Character, FX, and output shaping
//
//==============================================================================

//==============================================================================
// OnsetCharacterStage — Post-mixer grit (tanh saturation) + warmth (LP filter).
//
// Grit adds harmonic density via tanh soft-clipping (the same waveshaping used
// in analog console summing). Warmth rolls off highs via a Cytomic SVF lowpass
// (18kHz at 0 warmth, 4kHz at full warmth), softening the transient edge.
//==============================================================================
class OnsetCharacterStage
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        warmthLPL.setMode(CytomicSVF::Mode::LowPass);
        warmthLPR.setMode(CytomicSVF::Mode::LowPass);
    }

    // Call once per block with the current warmth value to update filter coefficients.
    // Keeps setCoefficients() (which computes 2*sin(pi*f/sr)) out of the per-sample path.
    void setWarmth(float warmth) noexcept
    {
        if (warmth > 0.01f)
        {
            // Fix 4: Warmth sweeps LP cutoff from 18kHz (open/bright) down to 8kHz (warm/dark).
            // Floor raised from 4kHz to 8kHz to prevent double-LP stacking with the per-voice
            // filter. At full warmth the character stage no longer steals presence from the mix.
            // 10000 = 18000 - 8000: the sweep range in Hz.
            float cutoff = 18000.0f - warmth * 10000.0f;
            warmthLPL.setCoefficients(cutoff, 0.1f, sr);
            warmthLPR.setCoefficients(cutoff, 0.1f, sr);
        }
    }

    void process(float& left, float& right, float grit, float warmth) noexcept
    {
        if (grit > 0.01f)
        {
            // Drive 1x-7x into tanh soft-clip, then volume-compensate by dividing
            // by drive and boosting by (1 + grit*2) to maintain perceived loudness.
            float drive = 1.0f + grit * 6.0f;
            left = fastTanh(left * drive) / drive * (1.0f + grit * 2.0f);
            right = fastTanh(right * drive) / drive * (1.0f + grit * 2.0f);
        }
        if (warmth > 0.01f)
        {
            // Coefficients are block-constant — caller should call setWarmth()
            // once per block before process() to avoid per-sample coefficient recompute.
            left = warmthLPL.processSample(left);
            right = warmthLPR.processSample(right);
        }
    }

    void reset() noexcept
    {
        warmthLPL.reset();
        warmthLPR.reset();
    }

private:
    float sr = 44100.0f;
    CytomicSVF warmthLPL, warmthLPR;
};

//==============================================================================
// OnsetDelay — Dark tape-style delay with LP in feedback path.
//
// A lowpass filter (4kHz cutoff) sits inside the feedback loop, so each
// repetition loses high-frequency content — mimicking tape degradation.
// This is the classic "dub delay" topology: each echo is darker and softer
// than the last, creating depth without harshness.
//==============================================================================
class OnsetDelay
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        // Size for 2 seconds at any sample rate — fixes 96kHz OOB and 704KB stack use.
        const size_t size = static_cast<size_t>(sampleRate * 2.0) + 1;
        bufL.assign(size, 0.0f);
        bufR.assign(size, 0.0f);
        writePos = 0;
        fbFilterL.setMode(CytomicSVF::Mode::LowPass);
        fbFilterR.setMode(CytomicSVF::Mode::LowPass);
        fbFilterL.setCoefficients(4000.0f, 0.0f, sr);
        fbFilterR.setCoefficients(4000.0f, 0.0f, sr);
    }

    void process(float& left, float& right, float timeSec, float feedback, float mix) noexcept
    {
        if (mix < 0.001f || bufL.empty())
            return;

        const int maxDelaySamples = static_cast<int>(bufL.size());
        int delaySamples = std::max(1, std::min(static_cast<int>(timeSec * sr), maxDelaySamples - 1));
        int readPosition = writePos - delaySamples;
        if (readPosition < 0)
            readPosition += maxDelaySamples;

        float delayedLeft = bufL[static_cast<size_t>(readPosition)];
        float delayedRight = bufR[static_cast<size_t>(readPosition)];

        // Feedback through LP filter: each loop trip darkens the echo (tape degradation)
        float feedbackLeft = fbFilterL.processSample(delayedLeft) * feedback;
        float feedbackRight = fbFilterR.processSample(delayedRight) * feedback;
        // Denormal protection: feedback loop with LP filter is a prime denormal site —
        // the filter's integrator states decay toward zero indefinitely.
        feedbackLeft = flushDenormal(feedbackLeft);
        feedbackRight = flushDenormal(feedbackRight);

        bufL[static_cast<size_t>(writePos)] = left + feedbackLeft;
        bufR[static_cast<size_t>(writePos)] = right + feedbackRight;
        writePos = (writePos + 1) % maxDelaySamples;

        left += delayedLeft * mix;
        right += delayedRight * mix;
    }

    void reset() noexcept
    {
        std::fill(bufL.begin(), bufL.end(), 0.0f);
        std::fill(bufR.begin(), bufR.end(), 0.0f);
        writePos = 0;
        fbFilterL.reset();
        fbFilterR.reset();
    }

private:
    float sr = 44100.0f;
    std::vector<float> bufL, bufR;
    int writePos = 0;
    CytomicSVF fbFilterL, fbFilterR;
};

//==============================================================================
// OnsetReverb — Tight room reverb using 4 comb filters + 2 allpass diffusers.
//
// Architecture: Schroeder reverb topology (1962). Four parallel comb filters
// feed into two series allpass diffusers. The comb lengths (1116, 1188, 1277,
// 1356 samples at 44.1kHz) are co-prime to avoid flutter echoes — these are
// the classic Schroeder/Moorer delay lengths used in Freeverb and many
// hardware reverbs. The allpass lengths (556, 441) add diffusion without
// coloring the frequency response.
//
// A one-pole lowpass in each comb's feedback path provides frequency-dependent
// decay (high frequencies decay faster), simulating air absorption in a real
// acoustic space.
//==============================================================================
class OnsetReverb
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        // Scale delay lengths from 44.1kHz reference to actual sample rate.
        // The base lengths are Schroeder/Moorer co-prime values (originally from
        // Freeverb by Jezar at Dreampoint), chosen to minimize flutter echoes.
        float sampleRateScale = sr / 44100.0f;
        combLen[0] = static_cast<int>(1116 * sampleRateScale); // ~25.3ms at 44.1kHz
        combLen[1] = static_cast<int>(1188 * sampleRateScale); // ~26.9ms
        combLen[2] = static_cast<int>(1277 * sampleRateScale); // ~29.0ms
        combLen[3] = static_cast<int>(1356 * sampleRateScale); // ~30.7ms
        apLen[0] = static_cast<int>(556 * sampleRateScale);    // ~12.6ms
        apLen[1] = static_cast<int>(441 * sampleRateScale);    // ~10.0ms

        // Fix 5: R-channel banks with +3.2% delay spread for stereo decorrelation.
        // This breaks the mono summing so the two channels arrive at slightly different
        // times, producing natural stereo width without pitch-shifting artifacts.
        // 1.032 chosen as the smallest spread that gives audible decorrelation above
        // ~200Hz without introducing metallic comb-filtering at the stereo center.
        for (int c = 0; c < 4; ++c)
            combLenR[c] = static_cast<int>(combLen[c] * 1.032f);
        for (int a = 0; a < 2; ++a)
            apLenR[a] = static_cast<int>(apLen[a] * 1.032f);

        // Allocate comb and allpass buffers to exactly the needed length.
        // Static arrays sized for 44.1 kHz would overflow at higher sample rates.
        for (int c = 0; c < 4; ++c)
        {
            combBufLen[c] = combLen[c] + 1;
            combBuf[c].assign(static_cast<size_t>(combBufLen[c]), 0.0f);
            combBufLenR[c] = combLenR[c] + 1;
            combBufR[c].assign(static_cast<size_t>(combBufLenR[c]), 0.0f);
        }
        for (int a = 0; a < 2; ++a)
        {
            apBufLen[a] = apLen[a] + 1;
            apBuf[a].assign(static_cast<size_t>(apBufLen[a]), 0.0f);
            apBufLenR[a] = apLenR[a] + 1;
            apBufR[a].assign(static_cast<size_t>(apBufLenR[a]), 0.0f);
        }

        reset();
    }

    void process(float& left, float& right, float roomSize, float decay, float mix) noexcept
    {
        if (mix < 0.001f)
            return;

        // Fix 5: Feed L and R channels separately into their own comb banks so the
        // two paths are decorrelated. Sum the stereo input to mono first so both
        // banks receive the same musical content but diverge over the delay network.
        float inputL = left;
        float inputR = right;
        // Comb feedback: 0.5-0.95. Higher decay = longer reverb tail.
        // Capped at 0.95 to prevent infinite buildup.
        float fb = 0.5f + decay * 0.45f;
        // Damping: 0.3-0.8. Smaller room = more high-frequency absorption.
        float damp = 0.3f + (1.0f - roomSize) * 0.5f;

        // L channel comb bank
        float sumL = 0.0f;
        for (int c = 0; c < 4; ++c)
        {
            int rp = combPos[c] - combLen[c];
            if (rp < 0)
                rp += combBufLen[c];
            float del = combBuf[c][static_cast<size_t>(rp)];
            // One-pole lowpass in feedback path: simulates air absorption
            combLP[c] = combLP[c] + damp * (del - combLP[c]);
            // Denormal protection: comb filter LP states decay toward zero when
            // input goes silent — denormal values persist indefinitely in the loop.
            combLP[c] = flushDenormal(combLP[c]);
            combBuf[c][static_cast<size_t>(combPos[c])] = inputL + combLP[c] * fb;
            combPos[c] = (combPos[c] + 1) % combBufLen[c];
            sumL += del;
        }
        sumL *= 0.25f;

        // Fix 5: R channel comb bank (3.2% longer delays = decorrelated path)
        float sumR = 0.0f;
        for (int c = 0; c < 4; ++c)
        {
            int rp = combPosR[c] - combLenR[c];
            if (rp < 0)
                rp += combBufLenR[c];
            float del = combBufR[c][static_cast<size_t>(rp)];
            combLPR[c] = combLPR[c] + damp * (del - combLPR[c]);
            combLPR[c] = flushDenormal(combLPR[c]);
            combBufR[c][static_cast<size_t>(combPosR[c])] = inputR + combLPR[c] * fb;
            combPosR[c] = (combPosR[c] + 1) % combBufLenR[c];
            sumR += del;
        }
        sumR *= 0.25f;

        // L allpass diffusers
        for (int a = 0; a < 2; ++a)
        {
            int rp = apPos[a] - apLen[a];
            if (rp < 0)
                rp += apBufLen[a];
            float del = apBuf[a][static_cast<size_t>(rp)];
            float apIn = sumL + del * 0.5f;
            apBuf[a][static_cast<size_t>(apPos[a])] = apIn;
            sumL = del - apIn * 0.5f;
            apPos[a] = (apPos[a] + 1) % apBufLen[a];
        }

        // Fix 5: R allpass diffusers (separate state, 3.2% longer)
        for (int a = 0; a < 2; ++a)
        {
            int rp = apPosR[a] - apLenR[a];
            if (rp < 0)
                rp += apBufLenR[a];
            float del = apBufR[a][static_cast<size_t>(rp)];
            float apIn = sumR + del * 0.5f;
            apBufR[a][static_cast<size_t>(apPosR[a])] = apIn;
            sumR = del - apIn * 0.5f;
            apPosR[a] = (apPosR[a] + 1) % apBufLenR[a];
        }

        left += sumL * mix;
        right += sumR * mix;
    }

    void reset() noexcept
    {
        for (int c = 0; c < 4; ++c)
        {
            std::fill(combBuf[c].begin(), combBuf[c].end(), 0.0f);
            combPos[c] = 0;
            combLP[c] = 0.0f;
            // Fix 5: also reset R-channel comb state
            std::fill(combBufR[c].begin(), combBufR[c].end(), 0.0f);
            combPosR[c] = 0;
            combLPR[c] = 0.0f;
        }
        for (int a = 0; a < 2; ++a)
        {
            std::fill(apBuf[a].begin(), apBuf[a].end(), 0.0f);
            apPos[a] = 0;
            // Fix 5: also reset R-channel allpass state
            std::fill(apBufR[a].begin(), apBufR[a].end(), 0.0f);
            apPosR[a] = 0;
        }
    }

private:
    float sr = 44100.0f;
    // L-channel comb + allpass banks
    std::vector<float> combBuf[4];
    float combLP[4] = {};
    int combLen[4] = {1116, 1188, 1277, 1356};
    int combBufLen[4] = {1117, 1189, 1278, 1357}; // len + 1, sized in prepare()
    int combPos[4] = {};
    std::vector<float> apBuf[2];
    int apLen[2] = {556, 441};
    int apBufLen[2] = {557, 442}; // len + 1, sized in prepare()
    int apPos[2] = {};
    // Fix 5: R-channel comb + allpass banks (3.2% longer delays for stereo decorrelation)
    std::vector<float> combBufR[4];
    float combLPR[4] = {};
    int combLenR[4] = {1152, 1226, 1318, 1400};    // ~1.032x L lengths (sized in prepare())
    int combBufLenR[4] = {1153, 1227, 1319, 1401}; // len + 1, sized in prepare()
    int combPosR[4] = {};
    std::vector<float> apBufR[2];
    int apLenR[2] = {574, 455};    // ~1.032x L lengths (sized in prepare())
    int apBufLenR[2] = {575, 456}; // len + 1, sized in prepare()
    int apPosR[2] = {};
};

//==============================================================================
// OnsetLoFi — Bitcrush quantization.
//
// Reduces bit depth by quantizing samples to discrete steps. At 16 bits,
// the effect is transparent; at 4 bits, the signal becomes a harsh staircase
// waveform. The steps and invSteps are precomputed per block (caller
// computes pow(2, bits) once) to avoid per-sample transcendental math.
//==============================================================================
class OnsetLoFi
{
public:
    // Process with precomputed steps/invSteps to avoid per-sample std::pow.
    // steps = std::pow(2.0f, bits), invSteps = 1.0f / steps (block-constant).
    void process(float& left, float& right, float steps, float invSteps, float mix) noexcept
    {
        if (mix < 0.001f)
            return;

        float crushL = std::round(left * steps) * invSteps;
        float crushR = std::round(right * steps) * invSteps;

        left = left + (crushL - left) * mix;
        right = right + (crushR - right) * mix;
    }
};

//==============================================================================
//
//  V. VOICE — The splash: one drum voice, two synthesis worlds
//
//==============================================================================

//==============================================================================
// OnsetVoice — Single drum voice: Layer X + Layer O + blend + transient + env.
//
// Each voice is a complete drum synthesizer containing both a Circuit model
// (Layer X) and an Algorithmic model (Layer O), crossfaded by the Blend
// parameter using equal-power (cos/sin) curves. The transient designer adds
// a pre-blend attack spike, and the voice filter shapes the final timbre.
//
// Voice assignment is fixed (not polyphonic allocation): each of the 8 voices
// is permanently bound to a specific drum role and MIDI note.
//==============================================================================
struct OnsetVoice
{
    // Circuit type: 0=BridgedT, 1=NoiseBurst, 2=Metallic
    int circuitType = 0;
    bool isClap = false;
    float baseFreq = 220.0f;
    float sr = 44100.0f;

    // Fix 2: Voice index (0-7) drives per-voice filter Q selection.
    // Set by OnsetEngine::prepare() immediately after constructing the voice array.
    int voiceIndex = 0;

    // Fix 2: Per-voice resonance table — each drum role has a characteristic Q that
    // gives it a distinctive spectral peak on the voice filter. Using voice index so
    // the Q is data-driven and patch-preserving (no new parameters, no ID changes).
    // Values: kick is low-Q (no resonance), hats are highest-Q (most metallic sparkle).
    static constexpr float kVoiceFilterQ[8] = {0.35f, 0.45f, 0.55f, 0.40f, 0.60f, 0.50f, 0.30f, 0.65f};

    // Layer X
    BridgedTOsc bridgedT;
    NoiseBurstCircuit noiseBurst;
    MetallicOsc metallic;

    // Layer O
    FMPercussion fm;
    ModalResonator modal;
    KarplusStrongPerc karplusStrong;
    PhaseDistPerc phaseDist;

    // Shared
    OnsetEnvelope ampEnv;
    OnsetTransient transient;
    CytomicSVF voiceFilter;

    // D005: Breathing LFO — continuous autonomous filter modulation
    // Replaced inline struct with shared BreathingLFO (Source/DSP/StandardLFO.h).
    // Rate is set from perc_breathRate parameter; updated per block.
    BreathingLFO breathingLFO;
    float baseCutoff = 1000.0f; // stored at trigger for LFO modulation

    // Block-rate breathing: counter drives filter coefficient update every 32 samples.
    // At 0.08 Hz the cutoff changes < 0.003 Hz per sample — perceptually identical
    // at block-rate but saves 8 std::sin calls per sample across 8 voices.
    int breathBlockCounter = 0;
    float lastBreathCutoff = 1000.0f; // cached modulated cutoff, updated every 32 samples

    // State
    bool active = false;
    float lastOutput = 0.0f;
    float velocity = 1.0f;
    bool triggeredThisBlock = false;

    void prepare(double sampleRate)
    {
        sr = static_cast<float>(sampleRate);
        bridgedT.prepare(sampleRate);
        noiseBurst.prepare(sampleRate);
        metallic.prepare(sampleRate);
        fm.prepare(sampleRate);
        modal.prepare(sampleRate);
        karplusStrong.prepare(sampleRate);
        phaseDist.prepare(sampleRate);
        ampEnv.prepare(sampleRate);
        transient.prepare(sampleRate);
        breathingLFO.setRate(0.08f,
                             static_cast<float>(sampleRate)); // default; overridden per block from perc_breathRate
        voiceFilter.setMode(CytomicSVF::Mode::LowPass);
    }

    void triggerVoice(float vel, float blend, int algoMode, float pitch, float decay, float tone, float snap,
                      float body, float character, int envShape = 0)
    {
        velocity = vel;
        active = true;
        triggeredThisBlock = true;

        // Convert pitch offset (semitones) to frequency.
        // 0.693147 = ln(2): exp(pitch * ln(2)/12) converts semitones to ratio.
        float freq = baseFreq * fastExp(pitch * (0.693147f / 12.0f));

        // Trigger Layer X
        switch (circuitType)
        {
        case 0:
            bridgedT.trigger(freq, snap, body, character);
            break;
        case 1:
            noiseBurst.trigger(freq, snap, tone, character, isClap);
            break;
        case 2:
            metallic.trigger(fastExp(pitch * (0.693147f / 12.0f)), tone, character);
            break;
        }

        // Trigger Layer O
        switch (algoMode)
        {
        case 0:
            fm.trigger(freq, snap, character, tone);
            break;
        case 1:
            modal.trigger(freq, character, tone, snap);
            break;
        case 2:
            karplusStrong.trigger(freq, snap, tone, character, decay);
            break;
        case 3:
            phaseDist.trigger(freq, snap, character, tone);
            break;
        }

        // Envelope: shape determines AD / AHD / ADSR behavior
        auto shape = static_cast<OnsetEnvelope::Shape>(std::min(envShape, 2));
        float holdTime = (shape != OnsetEnvelope::Shape::AD) ? body * 0.05f : 0.0f;
        float sustainLvl = (shape == OnsetEnvelope::Shape::ADSR) ? 0.3f : 0.0f;
        ampEnv.trigger(0.001f, decay, shape, holdTime, sustainLvl);

        // Transient: snap scaled by velocity
        transient.trigger(snap * vel, freq);

        // QA I3: Reset filter state on retrigger to prevent artifacts
        voiceFilter.reset();
        // D001: velocity opens filter — harder hits are brighter
        baseCutoff = 200.0f + tone * vel * 18000.0f;
        // Force immediate coefficient update on trigger so the first sample is correct.
        // Reset the counter so the block-rate guard fires on the very next processSample call.
        breathBlockCounter = 0;
        lastBreathCutoff = baseCutoff;
        // Fix 2: use per-voice Q from table instead of hardcoded 0.1
        const float voiceQ = kVoiceFilterQ[voiceIndex & 7];
        voiceFilter.setCoefficients(baseCutoff, voiceQ, sr);
    }

    // QA C1: Blend gains precomputed per block, passed in to avoid per-sample trig
    float processSample(float blendGainX, float blendGainO, int algoMode) noexcept
    {
        if (!active)
            return 0.0f;

        float envLevel = ampEnv.process();
        if (!ampEnv.isActive())
        {
            active = false;
            lastOutput = 0.0f;
            return 0.0f;
        }

        // Render active Layer X
        float layerX = 0.0f;
        switch (circuitType)
        {
        case 0:
            layerX = bridgedT.process();
            break;
        case 1:
            layerX = noiseBurst.process();
            break;
        case 2:
            layerX = metallic.process();
            break;
        }

        // Render active Layer O
        float layerO = 0.0f;
        switch (algoMode)
        {
        case 0:
            layerO = fm.process();
            break;
        case 1:
            layerO = modal.process();
            break;
        case 2:
            layerO = karplusStrong.process();
            break;
        case 3:
            layerO = phaseDist.process();
            break;
        }

        // Equal-power blend crossfade (gains precomputed per block)
        float blended = layerX * blendGainX + layerO * blendGainO;

        // D005: breathing LFO modulates filter cutoff at block-rate (every 32 samples).
        // At 0.08 Hz the LFO moves < 0.003 Hz per sample; updating every 32 samples
        // is perceptually identical and saves 8 std::sin calls per sample across 8 voices.
        if ((breathBlockCounter & 31) == 0)
        {
            float breathMod = breathingLFO.process();
            lastBreathCutoff = clamp(baseCutoff * (1.0f + breathMod * 0.15f), 20.0f, 18000.0f);
            // Fix 2: use per-voice Q from table (consistent with triggerVoice)
            voiceFilter.setCoefficients(lastBreathCutoff, kVoiceFilterQ[voiceIndex & 7], sr);
        }
        ++breathBlockCounter;

        // Voice filter (Fix 1: filter applied before transient addition below)
        blended = voiceFilter.processSample(blended);

        // Fix 1: Add transient AFTER the voice filter so the click/snap cuts through
        // the mix without being attenuated by the per-voice lowpass. Previously the
        // transient was added pre-filter and got rolled off with the body.
        blended += transient.process();

        // Apply envelope and velocity
        float out = blended * envLevel * velocity;
        lastOutput = out;
        return out;
    }

    void releaseVoice() noexcept { ampEnv.release(); }

    void choke() noexcept
    {
        active = false;
        ampEnv.reset();
        bridgedT.reset();
        noiseBurst.reset();
        metallic.reset();
        fm.reset();
        modal.reset();
        karplusStrong.reset();
        phaseDist.reset();
        transient.reset();
        voiceFilter.reset();
        lastOutput = 0.0f;
    }

    void reset() { choke(); }
};

//==============================================================================
//
//  VI. ENGINE — The complete XOnset: 8-voice percussive synthesis engine
//
//==============================================================================

//==============================================================================
// OnsetEngine — Percussive synthesis engine: 8 fixed voices, dual layers, blend.
// Implements SynthEngine for the XOceanus mega-tool.
//
// The drum machine that morphs between an 808 and a physics simulation on
// every hit. Lives at the water's surface in the XO_OX aquatic mythology —
// pure feliX energy, pure transient, every trigger a splash.
//
// Architecture:
//   8 dedicated voices (Kick, Snare, HH-C, HH-O, Clap, Tom, Perc A, Perc B)
//   Each voice: Layer X (Circuit) <--Blend--> Layer O (Algorithm)
//   Cross-Voice Coupling (XVC): voices modulate each other like a rhythm brain
//   4 Macros: MACHINE (blend bias), PUNCH (snap+body), SPACE (FX), MUTATE (drift)
//   FX Chain: Character Stage -> Delay -> Reverb -> LoFi
//
// Coupling interface (XOceanus integration):
//   getSampleForCoupling() returns post-FX master output
//   applyCouplingInput() accepts filter, decay, blend, and choke modulation
//==============================================================================
class OnsetEngine : public SynthEngine
{
public:
    static constexpr int kNumVoices = 8;

    //-- Voice configuration table -----------------------------------------------
    // Each voice is permanently mapped to a circuit topology, default algorithm,
    // GM-standard MIDI note, base frequency, and initial blend/decay values.
    //
    // Circuit types: 0=BridgedT (kick/tom), 1=NoiseBurst (snare/clap), 2=Metallic (hat/cymbal)
    // Algorithm modes: 0=FM, 1=Modal, 2=Karplus-Strong, 3=PhaseDistortion
    // MIDI notes follow General MIDI drum map (channel 10)
    struct VoiceCfg
    {
        int circuit;
        int defaultAlgorithm;
        int midiNote;
        float baseFreq;
        float defaultBlend;
        float defaultDecay;
        bool isClap;
    };

    static constexpr VoiceCfg kVoiceCfg[kNumVoices] = {
        //   Circuit  Algo  MIDI   Freq      Blend  Decay  Clap
        {0, 1, 36, 55.0f, 0.2f, 0.5f, false},    // V1 Kick    — BridgedT + Modal, A1, mostly circuit
        {1, 0, 38, 180.0f, 0.5f, 0.3f, false},   // V2 Snare   — NoiseBurst + FM, ~F#3, balanced blend
        {2, 0, 42, 8000.0f, 0.7f, 0.05f, false}, // V3 HH-C    — Metallic + FM, 8kHz base, very short
        {2, 0, 46, 8000.0f, 0.7f, 0.4f, false},  // V4 HH-O    — Metallic + FM, 8kHz base, longer decay
        {1, 3, 39, 1200.0f, 0.4f, 0.25f, true},  // V5 Clap    — NoiseBurst + PhaseDist, multi-burst mode
        {0, 1, 45, 110.0f, 0.3f, 0.4f, false},   // V6 Tom     — BridgedT + Modal, A2
        {0, 2, 37, 220.0f, 0.6f, 0.3f, false},   // V7 Perc A  — BridgedT + K-S, A3
        {2, 1, 44, 440.0f, 0.8f, 0.35f, false},  // V8 Perc B  — Metallic + Modal, A4, mostly algorithm
    };

    //-- Static parameter registration -------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    //-- SynthEngine lifecycle ---------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        blockSize = maxBlockSize;
        couplingBuffer.setSize(2, maxBlockSize);
        silenceGate.prepare(sampleRate, maxBlockSize);
        silenceGate.setHoldTime(100.0f); // Percussive — short hold
        couplingBuffer.clear();

        masterFilter.setMode(CytomicSVF::Mode::LowPass);
        masterFilterR.setMode(CytomicSVF::Mode::LowPass);
        characterStage.prepare(sampleRate);
        fxDelay.prepare(sampleRate);
        fxReverb.prepare(sampleRate);
        aftertouch.prepare(sampleRate);

        for (int v = 0; v < kNumVoices; ++v)
        {
            voices[v].circuitType = kVoiceCfg[v].circuit;
            voices[v].isClap = kVoiceCfg[v].isClap;
            voices[v].baseFreq = kVoiceCfg[v].baseFreq;
            voices[v].voiceIndex = v; // Fix 2: store index so per-voice Q table lookup works
            voices[v].prepare(sampleRate);
        }
    }

    void releaseResources() override
    {
        for (auto& v : voices)
            v.reset();
    }

    void reset() override
    {
        for (auto& v : voices)
            v.reset();
        couplingBuffer.clear();
        couplingFilterMod = 0.0f;
        couplingDecayMod = 0.0f;
        couplingBlendMod = 0.0f;
        std::memset(voicePeaks, 0, sizeof(voicePeaks));
        characterStage.reset();
        fxDelay.reset();
        fxReverb.reset();
    }

    //-- Render ------------------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0)
            return;

        // ---- 1. Snapshot parameters (ParamSnapshot pattern) ----
        // Cache all parameter values once per block to avoid per-sample
        // atomic loads. This is the XOceanus-wide ParamSnapshot convention.
        float vBlend[kNumVoices], vPitch[kNumVoices], vDecay[kNumVoices];
        float vTone[kNumVoices], vSnap[kNumVoices], vBody[kNumVoices];
        float vChar[kNumVoices], vLevel[kNumVoices], vPan[kNumVoices];
        int vAlgo[kNumVoices];

        int vEnvShape[kNumVoices];

        for (int v = 0; v < kNumVoices; ++v)
        {
            auto& vp = voiceParams[v];
            vBlend[v] = clamp(vp.blend ? vp.blend->load() + couplingBlendMod : kVoiceCfg[v].defaultBlend, 0.0f, 1.0f);
            vAlgo[v] = vp.algoMode ? static_cast<int>(vp.algoMode->load()) : kVoiceCfg[v].defaultAlgorithm;
            vPitch[v] = vp.pitch ? vp.pitch->load() : 0.0f;
            vDecay[v] =
                clamp((vp.decay ? vp.decay->load() : kVoiceCfg[v].defaultDecay) + couplingDecayMod, 0.001f, 8.0f);
            vTone[v] = clamp((vp.tone ? vp.tone->load() : 0.5f) + couplingFilterMod, 0.0f, 1.0f);
            vSnap[v] = vp.snap ? vp.snap->load() : 0.3f;
            vBody[v] = vp.body ? vp.body->load() : 0.5f;
            vChar[v] = vp.character ? vp.character->load() : 0.0f;
            vLevel[v] = vp.level ? vp.level->load() : 0.7f;
            vPan[v] = vp.pan ? vp.pan->load() : 0.0f;
            vEnvShape[v] = envShapeParams[v] ? static_cast<int>(envShapeParams[v]->load()) : 0;
        }

        // --- Macro snapshots ---
        float mMachine = macroMachine ? macroMachine->load() : 0.5f;
        float mPunch = macroPunch ? macroPunch->load() : 0.5f;
        float mMutate = macroMutate ? macroMutate->load() : 0.0f;
        // M3 SPACE: wired in Phase 4 (FX rack)

        // M1 MACHINE: bias all blends toward circuit (0) or algorithm (1)
        float machineBias = (mMachine - 0.5f) * 1.0f;
        for (int v = 0; v < kNumVoices; ++v)
            vBlend[v] = clamp(vBlend[v] + machineBias, 0.0f, 1.0f);

        // D006: smooth aftertouch pressure and compute modulation value
        aftertouch.updateBlock(numSamples);
        const float atPressure = aftertouch.getSmoothedPressure(0);

        // D002 mod matrix — apply per-block.
        // Destinations: 0=Off, 1=MasterLevel, 2=PunchMacro, 3=MutateMacro, 4=SpaceMacro
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
            percModLevelOffset  = mDst[1] * 0.5f;
            percModPunchOffset  = mDst[2] * 0.4f;
            percModMutateOffset = mDst[3] * 0.4f;
            percModSpaceOffset  = mDst[4] * 0.4f;
        }

        // M2 PUNCH: bias snap and body (0=soft, 1=aggressive)
        // D006: aftertouch adds up to +0.3 to PUNCH macro (sensitivity 0.3)
        mPunch = clamp(mPunch + atPressure * 0.3f + percModPunchOffset, 0.0f, 1.0f);
        float punchBias = (mPunch - 0.5f) * 0.6f;
        for (int v = 0; v < kNumVoices; ++v)
        {
            vSnap[v] = clamp(vSnap[v] + punchBias, 0.0f, 1.0f);
            vBody[v] = clamp(vBody[v] + punchBias, 0.0f, 1.0f);
        }

        // M4 MUTATE: per-block random drift on blend + character
        // D006: mod wheel scales MUTATE depth — wheel at max doubles drift range.
        // D002: percModMutateOffset adds mod matrix contribution
        mMutate = clamp(mMutate * (1.0f + modWheelAmount) + percModMutateOffset, 0.0f, 1.0f);
        if (mMutate > 0.01f)
        {
            for (int v = 0; v < kNumVoices; ++v)
            {
                vBlend[v] = clamp(vBlend[v] + mutateRng.process() * mMutate * 0.2f, 0.0f, 1.0f);
                vChar[v] = clamp(vChar[v] + mutateRng.process() * mMutate * 0.2f, 0.0f, 1.0f);
            }
        }

        // --- Cross-Voice Coupling (XVC): previous-block peaks modulate targets ---
        // XVC is the "rhythm brain" — voices interact like neurons. The peak
        // amplitude of each voice from the previous block modulates parameters
        // of other voices, creating emergent rhythmic relationships.
        float xvcGlobalLevel = xvcGlobalAmount ? xvcGlobalAmount->load() : 0.5f;
        if (xvcGlobalLevel > 0.01f)
        {
            float kickPeakScaled = voicePeaks[0] * xvcGlobalLevel;
            float snarePeakScaled = voicePeaks[1] * xvcGlobalLevel;

            // Kick -> Snare filter: kick hit opens snare's tone filter (rhythmic brightness)
            float kickToSnareFilterAmount = xvcKickSnareFilter ? xvcKickSnareFilter->load() : 0.15f;
            vTone[1] = clamp(vTone[1] + kickPeakScaled * kickToSnareFilterAmount, 0.0f, 1.0f);

            // Snare -> Hat decay: snare hit tightens hi-hat (0.5 scaling prevents over-dampening)
            float snareToHatDecayAmount = xvcSnareHatDecay ? xvcSnareHatDecay->load() : 0.10f;
            vDecay[2] = clamp(vDecay[2] - snarePeakScaled * snareToHatDecayAmount * 0.5f, 0.001f, 8.0f);

            // Kick -> Tom pitch: kick ducks tom pitch (6 semitones max range)
            float kickToTomPitchAmount = xvcKickTomPitch ? xvcKickTomPitch->load() : 0.0f;
            vPitch[5] = vPitch[5] - kickPeakScaled * kickToTomPitchAmount * 6.0f;

            // Snare -> Perc A blend: snare shifts percussion toward algorithmic layer
            float snareToPercBlendAmount = xvcSnarePercBlend ? xvcSnarePercBlend->load() : 0.0f;
            vBlend[6] = clamp(vBlend[6] + snarePeakScaled * snareToPercBlendAmount * 0.3f, 0.0f, 1.0f);
        }

        float masterLevel = percLevel ? percLevel->load() : 0.8f;
        float masterDrive = percDrive ? percDrive->load() : 0.0f;
        float masterTone = percTone ? percTone->load() : 0.5f;

        // D005: update breathing LFO rate on all voices once per block.
        // Using perc_breathRate gives user + coupling system control over filter drift speed.
        const float breathRateHz = percBreathRate ? percBreathRate->load() : 0.08f;
        for (int v = 0; v < kNumVoices; ++v)
            voices[v].breathingLFO.setRate(breathRateHz, static_cast<float>(sr));

        // FX + Character stage snapshots
        float pCharGrit = charGrit ? charGrit->load() : 0.0f;
        float pCharWarmth = charWarmth ? charWarmth->load() : 0.5f;
        float pDelTime = fxDelayTime ? fxDelayTime->load() : 0.3f;
        float pDelFb = fxDelayFeedback ? fxDelayFeedback->load() : 0.3f;
        float pDelMix = fxDelayMix ? fxDelayMix->load() : 0.0f;
        float pRevSize = fxReverbSize ? fxReverbSize->load() : 0.4f;
        float pRevDecay = fxReverbDecay ? fxReverbDecay->load() : 0.3f;
        float pRevMix = fxReverbMix ? fxReverbMix->load() : 0.0f;
        float pLofiBits = fxLofiBits ? fxLofiBits->load() : 16.0f;
        float pLofiMix = fxLofiMix ? fxLofiMix->load() : 0.0f;
        // Precompute block-constant LoFi quantisation steps (avoids per-sample std::pow).
        const float lofiSteps = std::pow(2.0f, pLofiBits);
        const float lofiInvSteps = 1.0f / lofiSteps;

        // M3 SPACE macro: drives reverb mix + delay feedback (percModSpaceOffset adds D002 contribution)
        float mSpace = clamp((macroSpace ? macroSpace->load() : 0.0f) + percModSpaceOffset, 0.0f, 1.0f);
        pRevMix = clamp(pRevMix + mSpace * 0.6f, 0.0f, 1.0f);
        pDelFb = clamp(pDelFb + mSpace * 0.3f, 0.0f, 0.95f);

        // QA I1: Apply master tone as LP filter on output
        float masterCutoff = 200.0f + masterTone * 18000.0f;
        masterFilter.setCoefficients(masterCutoff, 0.1f, static_cast<float>(sr));
        masterFilterR.setCoefficients(masterCutoff, 0.1f, static_cast<float>(sr));

        // Update character stage warmth filter coefficients once per block
        // (was previously recomputed inside every per-sample process() call).
        characterStage.setWarmth(pCharWarmth);

        // QA C6: Clear coupling buffer to prevent stale tail reads
        couplingBuffer.clear();

        // Hat choke mode (XVC parameter — can be disabled)
        bool hatChokeEnabled = xvcHatChoke ? xvcHatChoke->load() >= 0.5f : true;

        // 2. Process MIDI — trigger voices + note-off for ADSR
        for (const auto metadata : midi)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                int voiceIdx = noteToVoice(msg.getNoteNumber());
                if (voiceIdx < 0)
                    continue;

                // Hat choke: closed hat (V3) chokes open hat (V4) — XVC controlled
                if (voiceIdx == 2 && hatChokeEnabled)
                    voices[3].choke();

                float vel = msg.getFloatVelocity();
                voices[voiceIdx].triggerVoice(
                    vel, vBlend[voiceIdx], vAlgo[voiceIdx], vPitch[voiceIdx] + pitchBendNorm * 2.0f, vDecay[voiceIdx],
                    vTone[voiceIdx], vSnap[voiceIdx], vBody[voiceIdx], vChar[voiceIdx], vEnvShape[voiceIdx]);
            }
            else if (msg.isNoteOff())
            {
                int voiceIdx = noteToVoice(msg.getNoteNumber());
                if (voiceIdx >= 0)
                    voices[voiceIdx].releaseVoice();
            }
            // D006: channel pressure → aftertouch (applied to PUNCH macro below)
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure(msg.getChannelPressureValue() / 127.0f);
            // D006: mod wheel (CC#1) → MUTATE macro depth scale
            // Wheel up = more per-hit random drift in blend + character. Full wheel
            // doubles the effective MUTATE depth — the kit personality becomes
            // increasingly unpredictable with each trigger.
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

        // ---- 3. Precompute block-constant voice gains ----
        // Equal-power crossfade: cos(blend * pi/2) for Layer X, sin(blend * pi/2) for Layer O.
        // Pan law: sqrt(0.5*(1-pan)) for left, sqrt(0.5*(1+pan)) for right.
        // Both are hoisted out of the per-sample loop (QA C1 + C2).
        constexpr float halfPi = 1.5707963267948966f; // pi/2
        float blendGainX[kNumVoices], blendGainO[kNumVoices];
        float panGainL[kNumVoices], panGainR[kNumVoices];
        for (int v = 0; v < kNumVoices; ++v)
        {
            blendGainX[v] = std::cos(vBlend[v] * halfPi);
            blendGainO[v] = std::sin(vBlend[v] * halfPi);
            panGainL[v] = std::sqrt(0.5f * (1.0f - vPan[v]));
            panGainR[v] = std::sqrt(0.5f * (1.0f + vPan[v]));
        }

        // ---- 4. Render all voices (with peak tracking for XVC) ----
        auto* outL = buffer.getWritePointer(0);
        auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
        auto* cplL = couplingBuffer.getWritePointer(0);
        auto* cplR = couplingBuffer.getWritePointer(1);
        float blockPeaks[kNumVoices] = {};

        for (int s = 0; s < numSamples; ++s)
        {
            float sumL = 0.0f, sumR = 0.0f;

            for (int v = 0; v < kNumVoices; ++v)
            {
                if (!voices[v].active)
                    continue;

                float sample = voices[v].processSample(blendGainX[v], blendGainO[v], vAlgo[v]);
                float absSamp = std::abs(sample * vLevel[v]);
                if (absSamp > blockPeaks[v])
                    blockPeaks[v] = absSamp;

                sumL += sample * vLevel[v] * panGainL[v];
                sumR += sample * vLevel[v] * panGainR[v];
            }

            // Master drive (soft clip)
            if (masterDrive > 0.01f)
            {
                float dg = 1.0f + masterDrive * 4.0f;
                sumL = fastTanh(sumL * dg);
                sumR = fastTanh(sumR * dg);
            }

            // QA I1: master tone LP filter
            sumL = masterFilter.processSample(sumL);
            sumR = masterFilterR.processSample(sumR);

            // Character stage: grit (saturation) + warmth (LP)
            characterStage.process(sumL, sumR, pCharGrit, pCharWarmth);

            // FX chain: Delay → Reverb → LoFi
            fxDelay.process(sumL, sumR, pDelTime, pDelFb, pDelMix);
            fxReverb.process(sumL, sumR, pRevSize, pRevDecay, pRevMix);
            fxLoFi.process(sumL, sumR, lofiSteps, lofiInvSteps, pLofiMix);

            const float effectiveMasterLevel = clamp(masterLevel + percModLevelOffset, 0.05f, 1.5f);
            sumL *= effectiveMasterLevel;
            sumR *= effectiveMasterLevel;

            outL[s] += sumL;
            if (outR)
                outR[s] += sumR;
            cplL[s] = sumL;
            cplR[s] = sumR;
        }

        // 4. Store voice peaks for next block's XVC
        for (int v = 0; v < kNumVoices; ++v)
            voicePeaks[v] = blockPeaks[v];

        // 5. Clear per-block state
        for (auto& v : voices)
            v.triggeredThisBlock = false;
        couplingFilterMod = 0.0f;
        couplingDecayMod = 0.0f;
        couplingBlendMod = 0.0f;

        // SilenceGate: analyze output level for next-block bypass decision
        silenceGate.analyzeBlock(buffer.getReadPointer(0),
                                 buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr, numSamples);
    }

    //-- Coupling ----------------------------------------------------------------
    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        if (channel < couplingBuffer.getNumChannels() && sampleIndex < couplingBuffer.getNumSamples())
            return couplingBuffer.getSample(channel, sampleIndex);
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        float sum = 0.0f;
        if (sourceBuffer != nullptr)
            for (int i = 0; i < numSamples; ++i)
                sum += sourceBuffer[i];
        float avgMod = (numSamples > 0 && sourceBuffer != nullptr) ? (sum / static_cast<float>(numSamples)) * amount
                                                                   : amount; // fall back to scalar amount if no buffer

        switch (type)
        {
        case CouplingType::AmpToFilter:
            couplingFilterMod += avgMod;
            break;
        case CouplingType::EnvToDecay:
            couplingDecayMod += avgMod;
            break;
        case CouplingType::RhythmToBlend:
            couplingBlendMod += avgMod;
            break;
        case CouplingType::AmpToChoke:
            if (avgMod > 0.5f)
                for (auto& v : voices)
                    v.choke();
            break;
        default:
            break;
        }
    }

    //-- Parameters --------------------------------------------------------------
    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        for (int v = 0; v < kNumVoices; ++v)
        {
            juce::String pre = "perc_v" + juce::String(v + 1) + "_";
            auto& vp = voiceParams[v];
            vp.blend = apvts.getRawParameterValue(pre + "blend");
            vp.algoMode = apvts.getRawParameterValue(pre + "algoMode");
            vp.pitch = apvts.getRawParameterValue(pre + "pitch");
            vp.decay = apvts.getRawParameterValue(pre + "decay");
            vp.tone = apvts.getRawParameterValue(pre + "tone");
            vp.snap = apvts.getRawParameterValue(pre + "snap");
            vp.body = apvts.getRawParameterValue(pre + "body");
            vp.character = apvts.getRawParameterValue(pre + "character");
            vp.level = apvts.getRawParameterValue(pre + "level");
            vp.pan = apvts.getRawParameterValue(pre + "pan");
            envShapeParams[v] = apvts.getRawParameterValue(pre + "envShape");
        }
        percLevel = apvts.getRawParameterValue("perc_level");
        percDrive = apvts.getRawParameterValue("perc_drive");
        percTone = apvts.getRawParameterValue("perc_masterTone");

        macroMachine = apvts.getRawParameterValue("perc_macro_machine");
        macroPunch = apvts.getRawParameterValue("perc_macro_punch");
        macroSpace = apvts.getRawParameterValue("perc_macro_space");
        macroMutate = apvts.getRawParameterValue("perc_macro_mutate");

        xvcKickSnareFilter = apvts.getRawParameterValue("perc_xvc_kick_to_snare_filter");
        xvcSnareHatDecay = apvts.getRawParameterValue("perc_xvc_snare_to_hat_decay");
        xvcKickTomPitch = apvts.getRawParameterValue("perc_xvc_kick_to_tom_pitch");
        xvcSnarePercBlend = apvts.getRawParameterValue("perc_xvc_snare_to_perc_blend");
        xvcHatChoke = apvts.getRawParameterValue("perc_xvc_hat_choke");
        xvcGlobalAmount = apvts.getRawParameterValue("perc_xvc_global_amount");

        charGrit = apvts.getRawParameterValue("perc_char_grit");
        charWarmth = apvts.getRawParameterValue("perc_char_warmth");

        percBreathRate = apvts.getRawParameterValue("perc_breathRate");

        fxDelayTime = apvts.getRawParameterValue("perc_fx_delay_time");
        fxDelayFeedback = apvts.getRawParameterValue("perc_fx_delay_feedback");
        fxDelayMix = apvts.getRawParameterValue("perc_fx_delay_mix");
        fxReverbSize = apvts.getRawParameterValue("perc_fx_reverb_size");
        fxReverbDecay = apvts.getRawParameterValue("perc_fx_reverb_decay");
        fxReverbMix = apvts.getRawParameterValue("perc_fx_reverb_mix");
        fxLofiBits = apvts.getRawParameterValue("perc_fx_lofi_bits");
        fxLofiMix = apvts.getRawParameterValue("perc_fx_lofi_mix");

        // D002 mod matrix
        modMatrix.attachParameters(apvts, "perc_");
    }

    //-- Identity ----------------------------------------------------------------
    juce::String getEngineId() const override { return "Onset"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF0066FF); }
    int getMaxVoices() const override { return kNumVoices; }

private:
    SilenceGate silenceGate;

    std::array<OnsetVoice, kNumVoices> voices;
    juce::AudioBuffer<float> couplingBuffer;
    CytomicSVF masterFilter;  // QA I1: master tone LP filter (L channel)
    CytomicSVF masterFilterR; // R channel
    OnsetCharacterStage characterStage;
    OnsetDelay fxDelay;
    OnsetReverb fxReverb;
    OnsetLoFi fxLoFi;
    double sr = 44100.0;
    int blockSize = 512;

    // Coupling modulation accumulators
    float couplingFilterMod = 0.0f;
    float couplingDecayMod = 0.0f;
    float couplingBlendMod = 0.0f;

    // Cached parameter pointers (ParamSnapshot pattern: resolved once in
    // attachParameters(), then accessed via atomic load per block — zero overhead
    // per sample, no string lookups on the audio thread).
    struct VoiceParameterPointers
    {
        std::atomic<float>* blend = nullptr;
        std::atomic<float>* algoMode = nullptr;
        std::atomic<float>* pitch = nullptr;
        std::atomic<float>* decay = nullptr;
        std::atomic<float>* tone = nullptr;
        std::atomic<float>* snap = nullptr;
        std::atomic<float>* body = nullptr;
        std::atomic<float>* character = nullptr;
        std::atomic<float>* level = nullptr;
        std::atomic<float>* pan = nullptr;
    };
    std::array<VoiceParameterPointers, kNumVoices> voiceParams;
    std::atomic<float>* percLevel = nullptr;
    std::atomic<float>* percDrive = nullptr;
    std::atomic<float>* percTone = nullptr;

    // Per-voice envelope shape pointers
    std::array<std::atomic<float>*, kNumVoices> envShapeParams{};

    // Macro parameter pointers
    std::atomic<float>* macroMachine = nullptr;
    std::atomic<float>* macroPunch = nullptr;
    std::atomic<float>* macroSpace = nullptr;
    std::atomic<float>* macroMutate = nullptr;

    // ---- D006 Aftertouch — pressure boosts PUNCH macro (snap+body aggression) ----
    PolyAftertouch aftertouch;

    // Cross-Voice Coupling (XVC) parameter pointers
    std::atomic<float>* xvcKickSnareFilter = nullptr;
    std::atomic<float>* xvcSnareHatDecay = nullptr;
    std::atomic<float>* xvcKickTomPitch = nullptr;
    std::atomic<float>* xvcSnarePercBlend = nullptr;
    std::atomic<float>* xvcHatChoke = nullptr;
    std::atomic<float>* xvcGlobalAmount = nullptr;

    // FX parameter pointers
    std::atomic<float>* fxDelayTime = nullptr;
    std::atomic<float>* fxDelayFeedback = nullptr;
    std::atomic<float>* fxDelayMix = nullptr;
    std::atomic<float>* fxReverbSize = nullptr;
    std::atomic<float>* fxReverbDecay = nullptr;
    std::atomic<float>* fxReverbMix = nullptr;
    std::atomic<float>* fxLofiBits = nullptr;
    std::atomic<float>* fxLofiMix = nullptr;

    // Character stage parameter pointers
    std::atomic<float>* charGrit = nullptr;
    std::atomic<float>* charWarmth = nullptr;

    // D005: breathing LFO rate parameter pointer
    std::atomic<float>* percBreathRate = nullptr;

    // XVC state: previous-block voice peak amplitudes
    float voicePeaks[kNumVoices] = {};

    // MUTATE macro RNG
    OnsetNoiseGen mutateRng;

    // D006: mod wheel (CC#1) — scales MUTATE macro depth
    float modWheelAmount = 0.0f;
    float pitchBendNorm = 0.0f;

    // D002 mod matrix — 4-slot configurable modulation routing
    ModMatrix<4> modMatrix;
    float percModLevelOffset  = 0.0f;
    float percModPunchOffset  = 0.0f;
    float percModMutateOffset = 0.0f;
    float percModSpaceOffset  = 0.0f;

    //-- MIDI note to voice mapping ----------------------------------------------
    // Maps incoming MIDI notes to voice indices using the GM drum map.
    // Returns -1 for unmapped notes (they are silently ignored).
    int noteToVoice(int midiNoteNumber) const noexcept
    {
        for (int i = 0; i < kNumVoices; ++i)
            if (kVoiceCfg[i].midiNote == midiNoteNumber)
                return i;
        // GM note 35 (Acoustic Bass Drum) is an alternate trigger for the kick (V1)
        if (midiNoteNumber == 35)
            return 0;
        return -1;
    }

    //-- Parameter registration --------------------------------------------------
    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using Float = juce::AudioParameterFloat;
        using Choice = juce::AudioParameterChoice;

        static const char* voiceNames[kNumVoices] = {"Kick", "Snare", "HH-C", "HH-O", "Clap", "Tom", "PercA", "PercB"};
        static const float defBlend[kNumVoices] = {0.2f, 0.5f, 0.7f, 0.7f, 0.4f, 0.3f, 0.6f, 0.8f};
        static const int defAlgo[kNumVoices] = {1, 0, 0, 0, 3, 1, 2, 1};
        static const float defDecay[kNumVoices] = {0.5f, 0.3f, 0.05f, 0.4f, 0.25f, 0.4f, 0.3f, 0.35f};

        for (int v = 0; v < kNumVoices; ++v)
        {
            juce::String pre = "perc_v" + juce::String(v + 1) + "_";
            juce::String name = juce::String(voiceNames[v]);

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "blend", 1), name + " Blend",
                                                     juce::NormalisableRange<float>(0.0f, 1.0f), defBlend[v]));

            params.push_back(std::make_unique<Choice>(juce::ParameterID(pre + "algoMode", 1), name + " Algo",
                                                      juce::StringArray{"FM", "Modal", "K-S", "PhaseDist"},
                                                      defAlgo[v]));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "pitch", 1), name + " Pitch",
                                                     juce::NormalisableRange<float>(-24.0f, 24.0f), 0.0f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "decay", 1), name + " Decay",
                                                     juce::NormalisableRange<float>(0.01f, 8.0f, 0.0f, 0.3f),
                                                     defDecay[v]));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "tone", 1), name + " Tone",
                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "snap", 1), name + " Snap",
                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "body", 1), name + " Body",
                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "character", 1), name + " Character",
                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "level", 1), name + " Level",
                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "pan", 1), name + " Pan",
                                                     juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));

            params.push_back(std::make_unique<Choice>(juce::ParameterID(pre + "envShape", 1), name + " Env",
                                                      juce::StringArray{"AD", "AHD", "ADSR"}, 0));
        }

        // Global parameters
        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_level", 1), "Perc Level",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_drive", 1), "Perc Drive",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_masterTone", 1), "Perc Tone",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Macros
        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_macro_machine", 1), "Machine",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_macro_punch", 1), "Punch",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_macro_space", 1), "Space",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_macro_mutate", 1), "Mutate",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // Cross-Voice Coupling (XVC)
        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_xvc_kick_to_snare_filter", 1),
                                                 "XVC Kick>Snare Flt", juce::NormalisableRange<float>(0.0f, 1.0f),
                                                 0.15f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_xvc_snare_to_hat_decay", 1),
                                                 "XVC Snare>Hat Dcy", juce::NormalisableRange<float>(0.0f, 1.0f),
                                                 0.10f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_xvc_kick_to_tom_pitch", 1), "XVC Kick>Tom Pch",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_xvc_snare_to_perc_blend", 1),
                                                 "XVC Snare>Perc Bld", juce::NormalisableRange<float>(0.0f, 1.0f),
                                                 0.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_xvc_hat_choke", 1), "XVC Hat Choke",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_xvc_global_amount", 1), "XVC Amount",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // D005: Breathing LFO rate — user-controllable autonomous filter modulation speed.
        // Range 0.001–8.0 Hz covers ultra-slow organic drift to fast pulse; default 0.08 Hz.
        // Skew 0.3 gives finer resolution in the sub-Hz range where the effect is most musical.
        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_breathRate", 1), "Breath Rate",
                                                 juce::NormalisableRange<float>(0.001f, 8.0f, 0.0f, 0.3f), 0.08f));

        // Character stage
        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_char_grit", 1), "Perc Grit",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_char_warmth", 1), "Perc Warmth",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // FX rack
        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_fx_delay_time", 1), "Perc Delay Time",
                                                 juce::NormalisableRange<float>(0.01f, 1.0f, 0.0f, 0.5f), 0.3f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_fx_delay_feedback", 1), "Perc Delay FB",
                                                 juce::NormalisableRange<float>(0.0f, 0.95f), 0.3f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_fx_delay_mix", 1), "Perc Delay Mix",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_fx_reverb_size", 1), "Perc Reverb Size",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_fx_reverb_decay", 1), "Perc Reverb Decay",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_fx_reverb_mix", 1), "Perc Reverb Mix",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_fx_lofi_bits", 1), "Perc LoFi Bits",
                                                 juce::NormalisableRange<float>(4.0f, 16.0f), 16.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("perc_fx_lofi_mix", 1), "Perc LoFi Mix",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // D002 mod matrix — 4-slot source→destination routing
        static const juce::StringArray kPercModDests {"Off", "Master Level", "Punch Macro", "Mutate Macro", "Space Macro"};
        ModMatrix<4>::addParameters(params, "perc_", "Onset", kPercModDests);
    }
};

//==============================================================================
// End of OnsetEngine — The Surface Splash
// feliX's transients, isolated and amplified. Every trigger a ripple.
//==============================================================================

} // namespace xoceanus
