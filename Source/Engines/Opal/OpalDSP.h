// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <array>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "../../DSP/FastMath.h"

namespace xoceanus {
namespace XOpal {

//==============================================================================
// Constants
//==============================================================================
static constexpr int MAX_CLOUD_COUNT  = 12;
static constexpr int MAX_GRAIN_COUNT  = 32;
static constexpr int GRAIN_BUFFER_SEC = 4;
static constexpr int MAX_BUFFER_SIZE  = 192000 * GRAIN_BUFFER_SEC; // 768000 samples @ 192kHz (handles all common pro sample rates)

//==============================================================================
// Enums
//==============================================================================
enum class OscSource { Sine = 0, Saw, Pulse, Noise, TwoOsc, Coupling };
enum class FilterMode { LowPass = 0, BandPass, HighPass };

//==============================================================================
// Parameter IDs — opal_ namespace
//==============================================================================
namespace ParamID {
    static constexpr const char* SOURCE        = "opal_source";
    static constexpr const char* GRAIN_SIZE    = "opal_grainSize";
    static constexpr const char* DENSITY       = "opal_density";
    static constexpr const char* POSITION      = "opal_position";
    static constexpr const char* POS_SCATTER   = "opal_posScatter";
    static constexpr const char* PITCH_SCATTER = "opal_pitchScatter";
    static constexpr const char* PAN_SCATTER   = "opal_panScatter";
    static constexpr const char* WINDOW        = "opal_window";
    static constexpr const char* FREEZE        = "opal_freeze";
    static constexpr const char* COUPLING_LEVEL = "opal_couplingLevel";
    static constexpr const char* PULSE_WIDTH   = "opal_pulseWidth";
    static constexpr const char* DETUNE_CENTS  = "opal_detuneCents";
    static constexpr const char* FILTER_CUTOFF = "opal_filterCutoff";
    static constexpr const char* FILTER_RESO   = "opal_filterReso";
    static constexpr const char* FILTER_MODE   = "opal_filterMode";
    static constexpr const char* SHIMMER       = "opal_shimmer";
    static constexpr const char* FROST         = "opal_frost";
    static constexpr const char* SMEAR         = "opal_smear";
    static constexpr const char* REVERB_MIX    = "opal_reverbMix";
    static constexpr const char* AMP_ATTACK    = "opal_ampAttack";
    static constexpr const char* AMP_DECAY     = "opal_ampDecay";
    static constexpr const char* AMP_SUSTAIN   = "opal_ampSustain";
    static constexpr const char* AMP_RELEASE   = "opal_ampRelease";
    static constexpr const char* MACRO_SCATTER = "opal_macroScatter";
    static constexpr const char* MACRO_DRIFT   = "opal_macroDrift";
    static constexpr const char* MACRO_COUPLING = "opal_macroCoupling";
    static constexpr const char* MACRO_SPACE   = "opal_macroSpace";
    static constexpr const char* MASTER_VOLUME = "opal_masterVolume";
} // namespace ParamID

//==============================================================================
// GrainParams — shared per-block grain configuration
//==============================================================================
struct GrainParams
{
    float position      = 0.5f;
    float posScatter    = 0.0f;
    float grainSizeMs   = 100.0f;
    float density       = 20.0f;
    float pitchScatter  = 0.0f;
    float panScatter    = 0.0f;
    int   windowShape   = 0;
    float peakAmplitude = 1.0f;
};

//==============================================================================
// WindowFunctions — grain envelope shapes
//==============================================================================
class WindowFunctions
{
public:
    // Returns the window amplitude at phase [0..1] for the given shape
    float evaluate(int shape, float phase) const noexcept
    {
        phase = std::max(0.0f, std::min(1.0f, phase));
        switch (shape)
        {
            case 0: return hann(phase);
            case 1: return gaussian(phase);
            case 2: return tukey(phase);
            case 3: return 1.0f; // Rectangular
            default: return hann(phase);
        }
    }

private:
    static float hann(float p) noexcept
    {
        return 0.5f * (1.0f - fastCos(6.283185307f * p));
    }

    static float gaussian(float p) noexcept
    {
        float x = (p - 0.5f) * 2.8f; // sigma ~ 0.35
        return fastExp(-0.5f * x * x);
    }

    static float tukey(float p) noexcept
    {
        constexpr float alpha = 0.5f;
        if (p < alpha * 0.5f)
            return 0.5f * (1.0f - fastCos(6.283185307f * p / alpha));
        if (p > 1.0f - alpha * 0.5f)
            return 0.5f * (1.0f - fastCos(6.283185307f * (1.0f - p) / alpha));
        return 1.0f;
    }
};

//==============================================================================
// GrainBuffer — stereo ring buffer (4 seconds, pre-allocated)
//==============================================================================
class GrainBuffer
{
public:
    GrainBuffer()
    {
        clear();
    }

    void clear() noexcept
    {
        std::memset(bufL, 0, sizeof(bufL));
        std::memset(bufR, 0, sizeof(bufR));
        writePos = 0;
        length   = MAX_BUFFER_SIZE;
        frozen   = false;
    }

    void setLength(int len) noexcept
    {
        length = std::max(1, std::min(MAX_BUFFER_SIZE, len));
    }

    void setFrozen(bool f) noexcept { frozen = f; }

    void write(float l, float r) noexcept
    {
        if (frozen) return;
        bufL[writePos] = l;
        bufR[writePos] = r;
        writePos = (writePos + 1) % length;
    }

    // Read from the buffer at a fractional position [0..1]
    void read(float position, float& outL, float& outR) const noexcept
    {
        float fIdx = position * static_cast<float>(length - 1);
        int idx0 = static_cast<int>(fIdx) % length;
        int idx1 = (idx0 + 1) % length;
        float frac = fIdx - std::floor(fIdx);

        outL = bufL[idx0] + frac * (bufL[idx1] - bufL[idx0]);
        outR = bufR[idx0] + frac * (bufR[idx1] - bufR[idx0]);
    }

    // Read at a sample index relative to current write position
    void readRelative(int offsetSamples, float& outL, float& outR) const noexcept
    {
        int idx = ((writePos - offsetSamples) % length + length) % length;
        outL = bufL[idx];
        outR = bufR[idx];
    }

    int getLength() const noexcept { return length; }

private:
    float bufL[MAX_BUFFER_SIZE] {};
    float bufR[MAX_BUFFER_SIZE] {};
    int   writePos = 0;
    int   length   = MAX_BUFFER_SIZE;
    bool  frozen   = false;
};

//==============================================================================
// GrainVoice — a single active grain with its own phase, pitch, pan
//==============================================================================
struct GrainVoice
{
    bool  active       = false;
    float position     = 0.0f;     // start position in buffer [0..1]
    float phase        = 0.0f;     // progress through grain [0..1]
    float phaseInc     = 0.001f;   // 1 / grainSizeSamples
    float pitchRatio   = 1.0f;     // playback speed multiplier
    float panL         = 0.707f;
    float panR         = 0.707f;
    int   windowShape  = 0;
    float amplitude    = 1.0f;

    void start(float pos, float sizeSamples, float pitch,
               float pL, float pR, int window, float amp) noexcept
    {
        active      = true;
        position    = pos;
        phase       = 0.0f;
        phaseInc    = (sizeSamples > 0.0f) ? (1.0f / sizeSamples) : 0.01f;
        pitchRatio  = pitch;
        panL        = pL;
        panR        = pR;
        windowShape = window;
        amplitude   = amp;
    }
};

//==============================================================================
// GrainPool — fixed-size pool of active grains
//==============================================================================
template <int MaxGrains>
class GrainPool
{
public:
    GrainPool() = default;

    void stopAll() noexcept
    {
        for (auto& g : grains)
            g.active = false;
    }

    GrainVoice* allocate() noexcept
    {
        for (auto& g : grains)
            if (!g.active)
                return &g;

        // Steal oldest (lowest phase remaining = most progressed)
        float maxPhase = -1.0f;
        GrainVoice* oldest = &grains[0];
        for (auto& g : grains)
        {
            if (g.phase > maxPhase)
            {
                maxPhase = g.phase;
                oldest = &g;
            }
        }
        return oldest;
    }

    // Process all active grains, reading from buffer and writing to output
    void processAll(const GrainBuffer& buffer, const WindowFunctions& windows,
                    float* outL, float* outR, int numSamples) noexcept
    {
        for (auto& g : grains)
        {
            if (!g.active) continue;

            for (int n = 0; n < numSamples; ++n)
            {
                if (g.phase >= 1.0f)
                {
                    g.active = false;
                    break;
                }

                float windowAmp = windows.evaluate(g.windowShape, g.phase);

                // Read from buffer at grain's position + phase offset
                float readPos = g.position + g.phase * g.pitchRatio * 0.1f;
                readPos -= std::floor(readPos); // wrap [0..1]

                float sL, sR;
                buffer.read(readPos, sL, sR);

                float amp = windowAmp * g.amplitude;
                outL[n] += sL * amp * g.panL;
                outR[n] += sR * amp * g.panR;

                g.phase += g.phaseInc;
            }
        }
    }

    int getActiveCount() const noexcept
    {
        int count = 0;
        for (auto& g : grains)
            if (g.active) ++count;
        return count;
    }

private:
    std::array<GrainVoice, MaxGrains> grains {};
};

//==============================================================================
// CloudVoice — MIDI-triggered grain cloud with ADSR amplitude envelope
//==============================================================================
class CloudVoice
{
public:
    bool  active     = false;
    int   noteNumber = -1;
    float velocity   = 0.0f;
    float pitchRatio = 1.0f;  // derived from MIDI note

    // ADSR envelope state
    enum class EnvStage { Attack, Decay, Sustain, Release, Off };
    EnvStage envStage  = EnvStage::Off;
    float    envLevel  = 0.0f;
    float    envAttack = 0.01f, envDecay = 0.1f;
    float    envSustain = 0.7f, envRelease = 0.3f;

    // Grain scheduling
    float grainTimer   = 0.0f;   // countdown to next grain trigger

    void noteOn(int note, float vel,
                float attack, float decay, float sustain, float release) noexcept
    {
        active      = true;
        noteNumber  = note;
        velocity    = vel;
        pitchRatio  = fastPow2((static_cast<float>(note) - 60.0f) / 12.0f);
        envStage    = EnvStage::Attack;
        envLevel    = 0.0f;
        envAttack   = std::max(0.001f, attack);
        envDecay    = std::max(0.001f, decay);
        envSustain  = sustain;
        envRelease  = std::max(0.001f, release);
        grainTimer  = 0.0f;
    }

    void noteOff() noexcept
    {
        if (envStage != EnvStage::Off)
            envStage = EnvStage::Release;
    }

    // Advance envelope by one sample at given sample rate
    void tickEnvelope(double sampleRate) noexcept
    {
        float sr = static_cast<float>(sampleRate);
        switch (envStage)
        {
            case EnvStage::Attack:
                envLevel += 1.0f / (envAttack * sr);
                if (envLevel >= 1.0f)
                {
                    envLevel = 1.0f;
                    envStage = EnvStage::Decay;
                }
                break;

            case EnvStage::Decay:
                envLevel -= (1.0f - envSustain) / (envDecay * sr);
                if (envLevel <= envSustain)
                {
                    envLevel = envSustain;
                    envStage = EnvStage::Sustain;
                }
                break;

            case EnvStage::Sustain:
                envLevel = envSustain;
                break;

            case EnvStage::Release:
                envLevel -= envLevel / (envRelease * sr + 1.0f);
                if (envLevel < 0.001f)
                {
                    envLevel = 0.0f;
                    envStage = EnvStage::Off;
                    active   = false;
                }
                break;

            case EnvStage::Off:
                break;
        }
    }
};

//==============================================================================
// CloudPool — manages polyphonic cloud voices
//==============================================================================
class CloudPool
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        stopAll();
    }

    void stopAll() noexcept
    {
        for (auto& c : clouds)
        {
            c.active = false;
            c.envStage = CloudVoice::EnvStage::Off;
            c.envLevel = 0.0f;
        }
    }

    CloudVoice* allocate() noexcept
    {
        // Find inactive cloud
        for (auto& c : clouds)
            if (!c.active)
                return &c;

        // Steal quietest
        float minEnv = 999.0f;
        CloudVoice* quietest = &clouds[0];
        for (auto& c : clouds)
        {
            if (c.envLevel < minEnv)
            {
                minEnv = c.envLevel;
                quietest = &c;
            }
        }
        return quietest;
    }

    CloudVoice* findNote(int noteNumber) noexcept
    {
        for (auto& c : clouds)
            if (c.active && c.noteNumber == noteNumber)
                return &c;
        return nullptr;
    }

    void setSharedParams(const GrainParams& p) noexcept
    {
        sharedParams = p;
    }

    // Tick all clouds by one sample — schedule grains as needed
    template <int MaxGrains>
    void tickOneSample(const GrainBuffer& buffer,
                       const WindowFunctions& windows,
                       GrainPool<MaxGrains>& grainPool) noexcept
    {
        for (auto& c : clouds)
        {
            if (!c.active) continue;

            c.tickEnvelope(sr);
            if (!c.active) continue; // envelope finished

            // Grain scheduling: timer counts down, triggers new grain at 0
            c.grainTimer -= 1.0f;
            if (c.grainTimer <= 0.0f)
            {
                float density = std::max(1.0f, sharedParams.density);
                c.grainTimer = static_cast<float>(sr) / density;

                // Compute grain parameters with scatter
                float pos = sharedParams.position + randomBipolar() * sharedParams.posScatter;
                pos -= std::floor(pos);
                if (pos < 0.0f) pos += 1.0f;

                float sizeSamples = sharedParams.grainSizeMs * 0.001f * static_cast<float>(sr);
                sizeSamples = std::max(10.0f, sizeSamples);

                float pitch = c.pitchRatio;
                if (sharedParams.pitchScatter > 0.0f)
                    pitch *= fastPow2(randomBipolar() * sharedParams.pitchScatter / 12.0f);

                float pan = 0.5f + randomBipolar() * sharedParams.panScatter * 0.5f;
                pan = std::max(0.0f, std::min(1.0f, pan));
                float panL = fastCos(pan * 1.5707963f);
                float panR = fastSin(pan * 1.5707963f);

                auto* grain = grainPool.allocate();
                if (grain)
                {
                    grain->start(pos, sizeSamples, pitch,
                                 panL, panR, sharedParams.windowShape,
                                 c.envLevel * c.velocity);
                }
            }
        }
    }

    int getActiveCount() const noexcept
    {
        int count = 0;
        for (auto& c : clouds)
            if (c.active) ++count;
        return count;
    }

private:
    std::array<CloudVoice, MAX_CLOUD_COUNT> clouds {};
    GrainParams sharedParams;
    double sr = 44100.0;

    // Simple deterministic pseudo-random (no allocation, audio-safe)
    uint32_t rngState = 12345u;
    float randomBipolar() noexcept
    {
        rngState = rngState * 1103515245u + 12345u;
        return (static_cast<float>(rngState >> 16) / 32768.0f) - 1.0f;
    }
};

//==============================================================================
// OscillatorBank — simple grain source oscillators (sine, saw, pulse, noise, 2-osc)
//==============================================================================
class OscillatorBank
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        phase1 = 0.0;
        phase2 = 0.0;
    }

    void generateSample(OscSource mode, float freqHz,
                         float pulseWidth, float detuneCents,
                         float& outL, float& outR) noexcept
    {
        double inc1 = static_cast<double>(freqHz) / sr;
        double inc2 = static_cast<double>(freqHz) * static_cast<double>(fastPow2(static_cast<float>(detuneCents) / 1200.0f)) / sr;

        switch (mode)
        {
            case OscSource::Sine:
            {
                // SRO: fastSin replaces std::sin (per-sample oscillator)
                float s = fastSin(static_cast<float>(phase1 * 6.283185307f));
                outL = outR = s;
                phase1 += inc1;
                break;
            }

            case OscSource::Saw:
            {
                float s = static_cast<float>(2.0 * phase1 - 1.0);
                outL = outR = s;
                phase1 += inc1;
                break;
            }

            case OscSource::Pulse:
            {
                float s = (phase1 < static_cast<double>(pulseWidth)) ? 1.0f : -1.0f;
                outL = outR = s;
                phase1 += inc1;
                break;
            }

            case OscSource::Noise:
            {
                noiseState = noiseState * 1103515245u + 12345u;
                float s = (static_cast<float>(noiseState >> 16) / 32768.0f) - 1.0f;
                outL = outR = s;
                break;
            }

            case OscSource::TwoOsc:
            {
                // SRO: fastSin replaces std::sin (per-sample oscillator)
                float s1 = fastSin(static_cast<float>(phase1 * 6.283185307f));
                float s2 = static_cast<float>(2.0 * phase2 - 1.0); // detuned saw
                outL = s1;
                outR = s2;
                phase1 += inc1;
                phase2 += inc2;
                break;
            }

            default:
                outL = outR = 0.0f;
                break;
        }

        // Wrap phases
        if (phase1 >= 1.0) phase1 -= 1.0;
        if (phase2 >= 1.0) phase2 -= 1.0;
    }

private:
    double sr = 44100.0;
    double phase1 = 0.0;
    double phase2 = 0.0;
    uint32_t noiseState = 54321u;
};

//==============================================================================
// SVFilterStereo — stereo Cytomic SVF (LP/BP/HP)
//==============================================================================
class SVFilterStereo
{
public:
    void reset() noexcept
    {
        ic1eqL = 0.0f; ic2eqL = 0.0f;
        ic1eqR = 0.0f; ic2eqR = 0.0f;
    }

    void setParams(double sampleRate, float cutoffHz, float resonance, FilterMode mode) noexcept
    {
        fMode = mode;
        float nyquist = static_cast<float>(sampleRate) * 0.49f;
        float fc = std::max(20.0f, std::min(cutoffHz, nyquist));
        float res = std::max(0.0f, std::min(1.0f, resonance));

        constexpr float pi = 3.14159265358979323846f;
        g = std::tan(pi * fc / static_cast<float>(sampleRate));
        k = 2.0f - 2.0f * res;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    void process(float& l, float& r) noexcept
    {
        l = tick(l, ic1eqL, ic2eqL);
        r = tick(r, ic1eqR, ic2eqR);
    }

private:
    float tick(float input, float& s1, float& s2) const noexcept
    {
        float v3 = input - s2;
        float v1 = a1 * s1 + a2 * v3;
        float v2 = s2 + a2 * s1 + a3 * v3;

        s1 = flushDenormal(2.0f * v1 - s1);
        s2 = flushDenormal(2.0f * v2 - s2);

        switch (fMode)
        {
            case FilterMode::LowPass:  return v2;
            case FilterMode::BandPass: return v1;
            case FilterMode::HighPass: return input - k * v1 - v2;
        }
        return v2;
    }

    FilterMode fMode = FilterMode::LowPass;
    float g = 0.0f, k = 2.0f;
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float ic1eqL = 0.0f, ic2eqL = 0.0f;
    float ic1eqR = 0.0f, ic2eqR = 0.0f;
};

//==============================================================================
// Parameter registration — adds all opal_ parameters to a shared vector
//==============================================================================
inline void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    using juce::AudioParameterFloat;
    using juce::AudioParameterChoice;
    using juce::ParameterID;
    using juce::NormalisableRange;

    // Source select
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID{ParamID::SOURCE, 1}, "Opal Source",
        juce::StringArray{"Sine", "Saw", "Pulse", "Noise", "Two-Osc", "Coupling"}, 0));

    // Grain controls
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::GRAIN_SIZE, 1}, "Opal Grain Size",
        NormalisableRange<float>(10.0f, 800.0f, 0.1f, 0.4f), 100.0f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::DENSITY, 1}, "Opal Density",
        NormalisableRange<float>(1.0f, 120.0f, 0.1f, 0.5f), 20.0f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::POSITION, 1}, "Opal Position",
        NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    // Scatter
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::POS_SCATTER, 1}, "Opal Pos Scatter",
        NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.1f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::PITCH_SCATTER, 1}, "Opal Pitch Scatter",
        NormalisableRange<float>(0.0f, 24.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::PAN_SCATTER, 1}, "Opal Pan Scatter",
        NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

    // Window shape
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID{ParamID::WINDOW, 1}, "Opal Window",
        juce::StringArray{"Hann", "Gaussian", "Tukey", "Rect"}, 0));

    // Freeze
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::FREEZE, 1}, "Opal Freeze",
        NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    // Coupling level
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::COUPLING_LEVEL, 1}, "Opal Coupling Level",
        NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    // Oscillator controls
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::PULSE_WIDTH, 1}, "Opal Pulse Width",
        NormalisableRange<float>(0.05f, 0.95f, 0.01f), 0.5f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::DETUNE_CENTS, 1}, "Opal Detune",
        NormalisableRange<float>(0.0f, 50.0f, 0.1f), 7.0f));

    // Filter
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::FILTER_CUTOFF, 1}, "Opal Filter Cutoff",
        NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::FILTER_RESO, 1}, "Opal Filter Resonance",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.15f));

    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID{ParamID::FILTER_MODE, 1}, "Opal Filter Mode",
        juce::StringArray{"LP", "BP", "HP"}, 0));

    // Character stages
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::SHIMMER, 1}, "Opal Shimmer",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::FROST, 1}, "Opal Frost",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::SMEAR, 1}, "Opal Smear",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    // Reverb
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::REVERB_MIX, 1}, "Opal Reverb Mix",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

    // Amp envelope
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::AMP_ATTACK, 1}, "Opal Amp Attack",
        NormalisableRange<float>(0.001f, 8.0f, 0.001f, 0.3f), 0.3f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::AMP_DECAY, 1}, "Opal Amp Decay",
        NormalisableRange<float>(0.05f, 4.0f, 0.01f, 0.4f), 0.5f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::AMP_SUSTAIN, 1}, "Opal Amp Sustain",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::AMP_RELEASE, 1}, "Opal Amp Release",
        NormalisableRange<float>(0.05f, 8.0f, 0.01f, 0.3f), 1.5f));

    // Macros (M1-M4)
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::MACRO_SCATTER, 1}, "Opal M1 Scatter",
        NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::MACRO_DRIFT, 1}, "Opal M2 Drift",
        NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::MACRO_COUPLING, 1}, "Opal M3 Coupling",
        NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::MACRO_SPACE, 1}, "Opal M4 Space",
        NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    // Master volume
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ParamID::MASTER_VOLUME, 1}, "Opal Volume",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    addParameters(params);
    return { params.begin(), params.end() };
}

} // namespace XOpal
} // namespace xoceanus
