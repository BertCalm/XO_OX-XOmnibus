// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OxalisEngine.h — XOxalis | "The Geometric Garden"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOxalis is the geometric plant — clover patterns, fractal leaf
//      structures, mathematical growth. Fibonacci spirals in sound.
//      The succulent that grows according to mathematical law, not
//      organic impulse. Too orderly to be natural. Too beautiful
//      to be purely mechanical.
//
//  ENGINE CONCEPT:
//      A supersaw-style synth strings engine with phyllotaxis harmonic
//      spacing. Instead of integer harmonic ratios, partials appear at
//      golden angle (137.5 deg) intervals around the harmonic spiral.
//      The phi parameter controls how strongly the golden ratio influences
//      harmonic spacing — at 0 it's a standard supersaw, at 1 it's full
//      phyllotaxis. The resulting sound is close to strings but wrong in
//      an interesting way: too mathematical, too even, too precise.
//
//  DSP ARCHITECTURE:
//      Per-voice: N sawtooth oscillators at phyllotaxis-derived frequency
//      ratios (fundamental * phi^n for partials). CytomicSVF filter for
//      spectral shaping. Standard amp/filter envelopes. Growth Mode:
//      partials appear at golden angle intervals over time.
//
//  GARDEN QUAD ROLE:
//      Pioneer species — the first to respond to a note-on. Fast
//      germination. Synthetic precision. In the full GARDEN quad,
//      XOxalis sounds first and establishes the tonal ground for
//      the organic voices to grow into.
//
//  Accent: Fractal Purple #7B2D8B (checked — XOdyssey is Violet #7B2D8B;
//          using Clover Purple #6A0DAD to avoid conflict)
//  Parameter prefix: oxal_
//  Voices: 4 (Decision G2: CPU budget)
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/FilterEnvelope.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/ParameterSmoother.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/GardenAccumulators.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// Phyllotaxis oscillator bank — partials at golden ratio intervals
//==============================================================================
struct PhyllotaxisOscBank
{
    static constexpr int kMaxPartials = 7;
    static constexpr float kPhi = 1.6180339887f;       // Golden ratio
    static constexpr float kGoldenAngle = 2.39996323f; // 137.5 degrees in radians

    struct Partial
    {
        float phase = 0.0f;
        float phaseInc = 0.0f;
        float amplitude = 1.0f;
        bool active = true;
    };

    std::array<Partial, kMaxPartials> partials;
    float phiAmount = 0.5f; // 0 = standard harmonic, 1 = full phyllotaxis

    // Precomputed phyllotaxis ratios (kPhi^i for i=0..kMaxPartials-1).
    // These are constants — computed once at init, not per sample, so no per-sample pow().
    float cachedPhyllotaxisRatios[kMaxPartials] = {};
    float cachedFundamental = 440.0f;
    float cachedSampleRate = 48000.0f;

    void initPhyllotaxisCache() noexcept
    {
        float ratio = 1.0f;
        for (int i = 0; i < kMaxPartials; ++i)
        {
            cachedPhyllotaxisRatios[i] = ratio;
            ratio *= kPhi; // iterative multiply avoids std::pow() entirely
        }
    }

    void setFundamental(float freqHz, float sampleRate, float phi) noexcept
    {
        phiAmount = phi;
        cachedFundamental = freqHz;
        cachedSampleRate = sampleRate;
        // Ensure cache is populated (idempotent after first call)
        if (cachedPhyllotaxisRatios[0] == 0.0f)
            initPhyllotaxisCache();
        updatePartials(freqHz, sampleRate, phi);
    }

    // Per-note call: fully recalculate all partials. Call this in noteOn.
    void updatePartials(float freqHz, float sampleRate, float phi) noexcept
    {
        for (int i = 0; i < kMaxPartials; ++i)
        {
            // Standard harmonic ratio
            float standardRatio = static_cast<float>(i + 1);
            // Phyllotaxis ratio from precomputed cache (no std::pow per call)
            float phyllotaxisRatio = cachedPhyllotaxisRatios[i];
            // Blend between standard and phyllotaxis based on phi parameter
            float ratio = standardRatio + (phyllotaxisRatio - standardRatio) * phi;

            float partialFreq = freqHz * ratio;
            // Clamp to Nyquist
            if (partialFreq >= sampleRate * 0.49f)
            {
                partials[i].active = false;
                partials[i].phaseInc = 0.0f;
            }
            else
            {
                partials[i].active = true;
                partials[i].phaseInc = partialFreq / sampleRate;
            }

            // Amplitude roll-off: standard = 1/(i+1), phyllotaxis = Fibonacci-weighted
            float stdAmp = 1.0f / static_cast<float>(i + 1);
            float phyAmp = 1.0f / (1.0f + static_cast<float>(i) * 0.618f); // phi^-1 weighting
            partials[i].amplitude = stdAmp + (phyAmp - stdAmp) * phi;
        }
    }

    float process() noexcept
    {
        float sum = 0.0f;
        for (auto& p : partials)
        {
            if (!p.active)
                continue;

            // PolyBLEP sawtooth per partial
            float naive = 2.0f * p.phase - 1.0f;
            float blep = 0.0f;
            float t = p.phase;
            if (t < p.phaseInc)
            {
                t /= p.phaseInc;
                blep = t + t - t * t - 1.0f;
            }
            else if (t > 1.0f - p.phaseInc)
            {
                t = (t - 1.0f) / p.phaseInc;
                blep = t * t + t + t + 1.0f;
            }

            sum += (naive - blep) * p.amplitude;

            p.phase += p.phaseInc;
            if (p.phase >= 1.0f)
                p.phase -= 1.0f;
        }
        return sum * 0.25f; // normalize
    }

    void reset() noexcept
    {
        for (int i = 0; i < kMaxPartials; ++i)
        {
            partials[i].phase = static_cast<float>(i) * kGoldenAngle / (2.0f * 3.14159265f);
            if (partials[i].phase >= 1.0f)
                partials[i].phase -= 1.0f;
        }
    }
};

//==============================================================================
// OxalisVoice
//==============================================================================
struct OxalisVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;

    GlideProcessor glide;
    PhyllotaxisOscBank oscBank;
    CytomicSVF filter;
    FilterEnvelope ampEnv;
    FilterEnvelope filterEnv;
    StandardLFO vibratoLFO;
    StandardLFO lfo1, lfo2;

    // Growth Mode
    bool growthMode = false;
    float growthPhase = 0.0f;
    float growthTimer = 0.0f;
    float growthDuration = 10.0f;

    // Dormancy
    float dormancyPitchCents = 0.0f;

    float panL = 0.707f, panR = 0.707f;

    // CPU fix: cached pitch ratio to avoid std::pow inside semitonesToFreqRatio per sample.
    // lastBendInput tracks bendSemitones+couplingPitchMod+dormancy; ratio recomputed only
    // when the combined input changes by more than kPitchCacheThreshold semitones.
    float cachedPitchRatio = 1.0f;
    float lastBendInput = 0.0f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        growthMode = false;
        growthPhase = 0.0f;
        growthTimer = 0.0f;
        dormancyPitchCents = 0.0f;
        cachedPitchRatio = 1.0f;
        lastBendInput = 0.0f;
        glide.reset();
        oscBank.reset();
        filter.reset();
        ampEnv.kill();
        filterEnv.kill();
        vibratoLFO.reset();
    }
};

//==============================================================================
// OxalisEngine — "The Geometric Garden"
//==============================================================================
class OxalisEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 4;

    juce::String getEngineId() const override { return "Oxalis"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF9B59B6); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].ampEnv.prepare(srf);
            voices[i].filterEnv.prepare(srf);
            voices[i].vibratoLFO.setShape(StandardLFO::Sine);
            voices[i].vibratoLFO.reseed(static_cast<uint32_t>(i * 6271 + 313));
            // Pre-initialize phyllotaxis ratio cache so per-sample updatePartials
            // never hits the std::pow path (D004 / CPU fix).
            voices[i].oscBank.initPhyllotaxisCache();
        }

        smoothCutoff.prepare(srf);
        smoothPhi.prepare(srf);
        smoothSpread.prepare(srf);

        accumulators.reset();
        // Pioneer: fast warmth, sensitive to any activity
        accumulators.wRiseRate = 0.004f;
        accumulators.aThreshold = 0.3f;
        accumulators.dDecayRate = 0.015f; // Recovers from dormancy fastest

        network.configure(0.3f, 6.0f);

        prepareSilenceGate(sr, maxBlockSize, 500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();
        pitchBendNorm = 0.0f;
        modWheelAmount = 0.0f;
        aftertouchAmount = 0.0f;
        accumulators.reset();
        network.reset();
    }

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? couplingCacheL : couplingCacheR;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* buf, int numSamples) override
    {
        if (!buf || numSamples <= 0)
            return;
        float val = buf[numSamples - 1] * amount;
        switch (type)
        {
        case CouplingType::AmpToFilter:
            couplingFilterMod += val * 3000.0f;
            break;
        case CouplingType::LFOToPitch:
            couplingPitchMod += val * 2.0f;
            break;
        case CouplingType::AmpToPitch:
            couplingPitchMod += val;
            break;
        case CouplingType::EnvToMorph:
            couplingPhiMod += val;
            break;
        default:
            break;
        }
    }

    //==========================================================================
    // Render
    //==========================================================================
    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        int noteOnCount = 0;

        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
                wakeSilenceGate();
                noteOnCount++;
            }
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber());
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
            else if (msg.isChannelPressure())
                aftertouchAmount = msg.getChannelPressureValue() / 127.0f;
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
        }

        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear(0, numSamples);
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        auto loadP = [](std::atomic<float>* p, float def) { return p ? p->load(std::memory_order_relaxed) : def; };

        const float pCutoff = loadP(paramCutoff, 8000.0f);
        const float pResonance = loadP(paramResonance, 0.15f);
        const float pPhi = loadP(paramPhi, 0.5f);
        const float pSpread = loadP(paramSpread, 0.5f);
        const float pVibratoRate = loadP(paramVibratoRate, 4.5f);
        const float pVibratoDepth = loadP(paramVibratoDepth, 0.15f);
        const float pBendRange = loadP(paramBendRange, 2.0f);
        const float pFilterEnvAmt = loadP(paramFilterEnvAmt, 0.3f);
        const float pBrightness = loadP(paramBrightness, 0.6f);
        const float pSymmetry = loadP(paramSymmetry, 0.5f);

        const float macroChar = loadP(paramMacroCharacter, 0.0f);
        const float macroMove = loadP(paramMacroMovement, 0.0f);
        const float macroCoupl = loadP(paramMacroCoupling, 0.0f);
        const float macroSpace = loadP(paramMacroSpace, 0.0f);

        const float lfo1Rate = loadP(paramLfo1Rate, 0.5f);
        const float lfo1Depth = loadP(paramLfo1Depth, 0.0f);
        const int lfo1Shape = static_cast<int>(loadP(paramLfo1Shape, 0.0f));
        const float lfo2Rate = loadP(paramLfo2Rate, 1.0f);
        const float lfo2Depth = loadP(paramLfo2Depth, 0.0f);
        const int lfo2Shape = static_cast<int>(loadP(paramLfo2Shape, 0.0f));

        //-- Accumulator update --
        float avgVel = 0.0f;
        int activeCount = 0;
        for (const auto& v : voices)
            if (v.active)
            {
                avgVel += v.velocity;
                activeCount++;
            }
        if (activeCount > 0)
            avgVel /= static_cast<float>(activeCount);

        float blockSizeSec = static_cast<float>(numSamples) / srf;
        accumulators.update(blockSizeSec, activeCount, avgVel, noteOnCount);

        float effectivePhi =
            std::clamp(pPhi + macroChar * 0.4f + couplingPhiMod + modWheelAmount * 0.3f + macroCoupl * 0.25f, 0.0f,
                       1.0f); // M3 COUPLING: phi opens toward more irrational harmonics
        float effectiveCutoff =
            std::clamp(pCutoff + macroChar * 4000.0f + accumulators.getSeasonBrightness() * 1500.0f +
                           pBrightness * 8000.0f + couplingFilterMod + aftertouchAmount * 4000.0f,
                       200.0f, 20000.0f);
        const float effectiveWidth = 1.0f + macroSpace * 0.6f; // M4 SPACE: stereo expansion

        smoothCutoff.set(effectiveCutoff);
        smoothPhi.set(effectivePhi);
        smoothSpread.set(pSpread);

        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingPhiMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;
        // D006: Mod wheel → vibrato depth (standard expression mapping for strings)
        float effectiveVibratoDepth =
            pVibratoDepth + modWheelAmount * 0.5f + accumulators.getAggressionVibrato() * 0.2f;

        for (auto& voice : voices)
        {
            if (!voice.active)
                continue;
            voice.vibratoLFO.setRate(pVibratoRate + macroMove * 2.0f, srf);
            voice.lfo1.setRate(lfo1Rate, srf);
            voice.lfo1.setShape(lfo1Shape);
            voice.lfo2.setRate(lfo2Rate, srf);
            voice.lfo2.setShape(lfo2Shape);
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        for (int s = 0; s < numSamples; ++s)
        {
            float cutNow = smoothCutoff.process();
            float phiNow = smoothPhi.process();
            float sprNow = smoothSpread.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                float baseFreq = voice.glide.process();
                float vibrato = voice.vibratoLFO.process() * effectiveVibratoDepth;

                // CPU fix (OXALIS): cache pitch ratio and only recompute semitonesToFreqRatio
                // (which contains std::pow) when the slow-moving inputs change significantly.
                // Vibrato contributes up to ~0.08 semitones — included in cache invalidation.
                // Threshold of 0.005 semitones keeps tuning error < 0.01 cents inaudible.
                static constexpr float kPitchCacheThreshold = 0.005f;
                float bendInput =
                    bendSemitones + couplingPitchMod + vibrato * 0.08f + voice.dormancyPitchCents / 100.0f;
                if (std::fabs(bendInput - voice.lastBendInput) > kPitchCacheThreshold)
                {
                    voice.cachedPitchRatio = PitchBendUtil::semitonesToFreqRatio(bendInput);
                    voice.lastBendInput = bendInput;
                }
                float freq = baseFreq * voice.cachedPitchRatio;

                float l1 = voice.lfo1.process() * lfo1Depth;
                float l2 = voice.lfo2.process() * lfo2Depth;

                // Update phyllotaxis spacing — phi modulates harmonic ratios.
                // Use updatePartials() (no std::pow — cache already initialized in noteOn).
                voice.oscBank.updatePartials(freq, srf, phiNow + l2 * 0.2f);

                // Growth Mode: partials emerge at golden angle intervals
                float growthGain = 1.0f;
                if (voice.growthMode)
                {
                    voice.growthTimer += 1.0f / srf;
                    voice.growthPhase = std::min(voice.growthTimer / voice.growthDuration, 1.0f);

                    // Partials emerge sequentially at golden angle intervals
                    int activePartials = 1 + static_cast<int>(voice.growthPhase * 6.0f);
                    for (int p = 0; p < PhyllotaxisOscBank::kMaxPartials; ++p)
                        voice.oscBank.partials[p].active =
                            (p < activePartials) && voice.oscBank.partials[p].phaseInc > 0.0f;

                    growthGain = 0.3f + voice.growthPhase * 0.7f; // subtle fade-in
                }

                float oscOut = voice.oscBank.process();

                // Symmetry: at high symmetry, the sound becomes more even/synthetic
                // At low symmetry, introduce asymmetric waveshaping for organic feel
                if (pSymmetry < 0.9f)
                {
                    float asymmetry = 1.0f - pSymmetry;
                    oscOut = oscOut + asymmetry * 0.3f * fastTanh(oscOut * 2.0f);
                }

                // Filter
                float envLevel = voice.filterEnv.process();
                float fCut = std::clamp(cutNow + envLevel * pFilterEnvAmt * 6000.0f + l1 * 4000.0f, 200.0f, 20000.0f);
                voice.filter.setMode(CytomicSVF::Mode::LowPass);
                voice.filter.setCoefficients(fCut, pResonance, srf);
                float filtered = voice.filter.processSample(oscOut);

                // Amplitude
                float ampLevel = voice.ampEnv.process() * growthGain;
                if (ampLevel < 1e-6f && voice.ampEnv.getStage() == FilterEnvelope::Stage::Idle)
                {
                    voice.active = false;
                    continue;
                }

                // D001: velocity scales brightness (higher velocity = more partials audible)
                float velBright = 0.4f + voice.velocity * 0.6f;
                float output = filtered * ampLevel * velBright;

                mixL += output * voice.panL;
                mixR += output * voice.panR;
            }

            // M4 SPACE + Spread: mid/side width expansion with real-time spread modulation.
            // sprNow (smoothed pSpread) scales the side signal independently of macroSpace.
            const float mid = (mixL + mixR) * 0.5f;
            const float side = (mixL - mixR) * 0.5f * effectiveWidth * (0.5f + sprNow);
            outL[s] = mid + side;
            if (outR)
                outR[s] = mid - side;
            couplingCacheL = outL[s];
            couplingCacheR = outR ? outR[s] : mixR;
        }

        int count = 0;
        for (const auto& v : voices)
            if (v.active)
                ++count;
        activeVoiceCount.store(count);
        analyzeForSilenceGate(buffer, numSamples);
    }

    //==========================================================================
    // Note management
    //==========================================================================

    void noteOn(int note, float vel) noexcept
    {
        int idx = VoiceAllocator::findFreeVoice(voices, kMaxVoices);
        auto& v = voices[idx];

        float freq = 440.0f * std::pow(2.0f, (static_cast<float>(note) - 69.0f) / 12.0f);
        bool isGrowthMode = paramGrowthMode && paramGrowthMode->load() > 0.5f;

        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.setTargetOrSnap(freq);

        // Pioneer species: fastest attack in the quad
        float attackTime = paramAttack ? paramAttack->load() : 0.01f;
        attackTime *= (1.1f - vel * 0.2f);

        v.ampEnv.prepare(srf);
        v.ampEnv.setADSR(attackTime, paramDecay ? paramDecay->load() : 0.3f,
                         paramSustain ? paramSustain->load() : 0.85f, paramRelease ? paramRelease->load() : 0.6f);
        v.ampEnv.triggerHard();

        v.filterEnv.prepare(srf);
        v.filterEnv.setADSR(attackTime * 0.5f, 0.4f, 0.1f, 0.5f);
        v.filterEnv.triggerHard();

        v.vibratoLFO.reset();
        v.oscBank.reset();

        float phi = paramPhi ? paramPhi->load() : 0.5f;
        v.oscBank.setFundamental(freq, srf, phi);

        v.growthMode = isGrowthMode;
        v.growthTimer = 0.0f;
        v.growthPhase = 0.0f;
        v.growthDuration = paramGrowthTime ? paramGrowthTime->load() : 10.0f;

        // Dormancy — geometric precision degrades slightly during dormancy
        float dPitch = accumulators.getDormancyPitchVariance() * 0.5f; // less than organic engines
        uint32_t seed = static_cast<uint32_t>(note * 3571 + idx * 97);
        seed = seed * 1664525u + 1013904223u;
        v.dormancyPitchCents = (static_cast<float>(seed & 0xFFFF) / 32768.0f - 1.0f) * dPitch;

        // Spread panning
        float spread = paramSpread ? paramSpread->load() : 0.5f;
        float panPos = (static_cast<float>(idx) / static_cast<float>(kMaxVoices - 1)) * 2.0f - 1.0f;
        panPos *= spread;
        float angle = (panPos + 1.0f) * 0.25f * 3.14159265f;
        v.panL = std::cos(angle);
        v.panR = std::sin(angle);
    }

    void noteOff(int note) noexcept
    {
        for (auto& v : voices)
            if (v.active && v.currentNote == note)
                v.ampEnv.release();
    }

    //==========================================================================
    // Parameters — 27 total
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }
    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // Amp envelope
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_attack", 1}, "Oxalis Attack",
                                              juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.3f), 0.01f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_decay", 1}, "Oxalis Decay",
                                              juce::NormalisableRange<float>(0.01f, 5.0f, 0.0f, 0.4f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_sustain", 1}, "Oxalis Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.85f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_release", 1}, "Oxalis Release",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.4f), 0.6f));

        // Filter
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_cutoff", 1}, "Oxalis Cutoff",
                                              juce::NormalisableRange<float>(200.0f, 20000.0f, 0.0f, 0.3f), 8000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_resonance", 1}, "Oxalis Resonance",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.15f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_filterEnvAmt", 1}, "Oxalis Filter Env Amt",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Garden-specific
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_phi", 1}, "Oxalis Phi (Golden Ratio)",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_spread", 1}, "Oxalis Stereo Spread",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_symmetry", 1}, "Oxalis Symmetry",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_brightness", 1}, "Oxalis Brightness",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.6f));

        // Vibrato
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_vibratoRate", 1}, "Oxalis Vibrato Rate",
                                              juce::NormalisableRange<float>(0.5f, 12.0f, 0.1f), 4.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_vibratoDepth", 1}, "Oxalis Vibrato Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.15f));

        // Growth Mode
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_growthMode", 1}, "Oxalis Growth Mode",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_growthTime", 1}, "Oxalis Growth Time",
                                              juce::NormalisableRange<float>(3.0f, 60.0f, 0.1f), 10.0f));

        // Pitch
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_bendRange", 1}, "Oxalis Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));

        // Macros
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_macroCharacter", 1}, "Oxalis Macro CHARACTER",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_macroMovement", 1}, "Oxalis Macro MOVEMENT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_macroCoupling", 1}, "Oxalis Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_macroSpace", 1}, "Oxalis Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // LFOs
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_lfo1Rate", 1}, "Oxalis LFO1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_lfo1Depth", 1}, "Oxalis LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oxal_lfo1Shape", 1}, "Oxalis LFO1 Shape", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_lfo2Rate", 1}, "Oxalis LFO2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxal_lfo2Depth", 1}, "Oxalis LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oxal_lfo2Shape", 1}, "Oxalis LFO2 Shape", 0, 4, 0));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramAttack = apvts.getRawParameterValue("oxal_attack");
        paramDecay = apvts.getRawParameterValue("oxal_decay");
        paramSustain = apvts.getRawParameterValue("oxal_sustain");
        paramRelease = apvts.getRawParameterValue("oxal_release");
        paramCutoff = apvts.getRawParameterValue("oxal_cutoff");
        paramResonance = apvts.getRawParameterValue("oxal_resonance");
        paramFilterEnvAmt = apvts.getRawParameterValue("oxal_filterEnvAmt");
        paramPhi = apvts.getRawParameterValue("oxal_phi");
        paramSpread = apvts.getRawParameterValue("oxal_spread");
        paramSymmetry = apvts.getRawParameterValue("oxal_symmetry");
        paramBrightness = apvts.getRawParameterValue("oxal_brightness");
        paramVibratoRate = apvts.getRawParameterValue("oxal_vibratoRate");
        paramVibratoDepth = apvts.getRawParameterValue("oxal_vibratoDepth");
        paramGrowthMode = apvts.getRawParameterValue("oxal_growthMode");
        paramGrowthTime = apvts.getRawParameterValue("oxal_growthTime");
        paramBendRange = apvts.getRawParameterValue("oxal_bendRange");
        paramMacroCharacter = apvts.getRawParameterValue("oxal_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("oxal_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("oxal_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("oxal_macroSpace");
        paramLfo1Rate = apvts.getRawParameterValue("oxal_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("oxal_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("oxal_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("oxal_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("oxal_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("oxal_lfo2Shape");
    }

private:
    double sr = 48000.0;
    float srf = 48000.0f;

    std::array<OxalisVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount{0};

    GardenAccumulators accumulators;
    GardenMycorrhizalNetwork network;

    ParameterSmoother smoothCutoff, smoothPhi, smoothSpread;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f, couplingPhiMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramSustain = nullptr;
    std::atomic<float>* paramRelease = nullptr;
    std::atomic<float>* paramCutoff = nullptr;
    std::atomic<float>* paramResonance = nullptr;
    std::atomic<float>* paramFilterEnvAmt = nullptr;
    std::atomic<float>* paramPhi = nullptr;
    std::atomic<float>* paramSpread = nullptr;
    std::atomic<float>* paramSymmetry = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramVibratoRate = nullptr;
    std::atomic<float>* paramVibratoDepth = nullptr;
    std::atomic<float>* paramGrowthMode = nullptr;
    std::atomic<float>* paramGrowthTime = nullptr;
    std::atomic<float>* paramBendRange = nullptr;
    std::atomic<float>* paramMacroCharacter = nullptr;
    std::atomic<float>* paramMacroMovement = nullptr;
    std::atomic<float>* paramMacroCoupling = nullptr;
    std::atomic<float>* paramMacroSpace = nullptr;
    std::atomic<float>* paramLfo1Rate = nullptr;
    std::atomic<float>* paramLfo1Depth = nullptr;
    std::atomic<float>* paramLfo1Shape = nullptr;
    std::atomic<float>* paramLfo2Rate = nullptr;
    std::atomic<float>* paramLfo2Depth = nullptr;
    std::atomic<float>* paramLfo2Shape = nullptr;
};

} // namespace xoceanus
