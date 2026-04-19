// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OpcodeEngine.h — XOpcode | "Algorithm, 1983"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOpcode is the DX7 electric piano that defined a decade. The
//      crystalline tones of FM synthesis — mathematically precise,
//      impossibly clean — traveled from John Chowning's lab at Stanford
//      to Yamaha's factories in Hamamatsu to the hands of every City Pop
//      producer in Tokyo. Algorithm as recipe. Mathematics as cuisine.
//      Preset 11, "E. Piano 1," became the most-heard keyboard sound
//      of the 1980s. XOpcode knows that sound by heart, and knows all
//      the roads it traveled after.
//
//  ENGINE CONCEPT:
//      A 2-operator FM synthesis engine with velocity-sensitive modulation
//      index. The classic DX EP patch architecture: one carrier (the pitch
//      you hear) modulated by one modulator (the harmonic character). The
//      modulator:carrier frequency ratio determines the tonal character.
//      Low ratios (1:1, 2:1) = warm, piano-like. High ratios (3:1, 7:1)
//      = bell-like, glassy. The modulation index (depth of FM) controls
//      brightness: low index = sine-like purity, high index = complex
//      harmonics.
//
//      Three algorithm modes:
//        0 = Series (mod → carrier) — classic DX EP
//        1 = Parallel (mod + carrier) — organ-like
//        2 = Feedback (mod feeds back into itself) — harsh, metallic
//
//  SOURCE TRADITION TEST:
//      Must nail the DX7 EP bell. The crystalline attack, the smooth
//      decay, the mathematical purity that somehow feels emotional.
//      Play E. Piano 1. Does it feel like 1983?
//
//  CULTURAL ROUTE: Silicon Valley → Tokyo
//      Stanford FM algorithm → Yamaha DX7 → City Pop → global.
//
//  Accent: Digital Cyan #00CED1
//  Parameter prefix: opco_
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/FilterEnvelope.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/ParameterSmoother.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xoceanus
{

// Note: SpectralFingerprint struct and getSpectralFingerprint() removed.
// The FUSION inter-engine fingerprint coupling was designed but never wired.
// (#686 — dead code removal 2026-04-03)

//==============================================================================
// FMOperator — Single FM operator with phase accumulator and envelope.
//
// An FM operator is simply a sine oscillator with controllable frequency.
// When one operator's output modulates another's phase, you get FM synthesis.
// The DX7 had 6 operators; we use 2 for the classic EP patches because
// the iconic DX EP sound is fundamentally a 2-op configuration.
//==============================================================================
struct FMOperator
{
    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        phase = 0.0f;
        cachedPhaseInc = 0.0f;
    }

    // Call this in noteOn and whenever pitch changes. Caches freqHz / sr so
    // the per-sample process() loop avoids an unnecessary division each sample.
    void setFrequency(float freqHz) noexcept { cachedPhaseInc = (sr > 0.0f) ? (freqHz / sr) : 0.0f; }

    // process() uses the cached phase increment. Pass freqHz=0 if the frequency
    // was already set via setFrequency(); the argument is ignored when nonzero
    // only if you explicitly want to override the cache per-call.
    float process(float phaseModulation = 0.0f) noexcept
    {
        phase += cachedPhaseInc;
        if (phase >= 1.0f)
            phase -= 1.0f;
        if (phase < 0.0f)
            phase += 1.0f;

        // FM: add phase modulation to the oscillator phase
        float effPhase = phase + phaseModulation;
        effPhase -= static_cast<float>(static_cast<int>(effPhase));
        if (effPhase < 0.0f)
            effPhase += 1.0f;

        return fastSin(effPhase * 6.28318530718f);
    }

    void reset() noexcept
    {
        phase = 0.0f;
        cachedPhaseInc = 0.0f;
    }

    float sr = 0.0f;  // Sentinel: must be set by prepare() before use
    float phase = 0.0f;
    float cachedPhaseInc = 0.0f;
};

//==============================================================================
// DXModulationEnvelope — The DX7's distinctive 8-rate envelope.
//
// Simplified to a 4-stage ADSR-like envelope that captures the DX EP's
// characteristic: bright attack that decays to a warm sustain. The modulation
// envelope controls the FM index over time — this is what gives the DX EP
// its "bell attack into warm sustain" character.
//==============================================================================
struct DXModulationEnvelope
{
    enum Stage
    {
        Idle,
        Attack,
        Decay1,
        Sustain,
        Release
    };

    void prepare(float sampleRate) noexcept { sr = sampleRate; }

    void setRates(float atkTime, float dk1Time, float susLevel, float relTime) noexcept
    {
        if (sr <= 0.0f)
            return;
        attackRate = 1.0f / (sr * std::max(atkTime, 0.0001f));
        decay1Coeff = 1.0f - std::exp(-4.6f / (sr * std::max(dk1Time, 0.001f)));
        sustainLevel = std::clamp(susLevel, 0.0f, 1.0f);
        releaseCoeff = 1.0f - std::exp(-4.6f / (sr * std::max(relTime, 0.001f)));
    }

    void trigger() noexcept
    {
        level = 0.0f;
        stage = Attack;
    }

    void release() noexcept
    {
        if (stage != Idle)
            stage = Release;
    }

    void kill() noexcept
    {
        level = 0.0f;
        stage = Idle;
    }

    float process() noexcept
    {
        switch (stage)
        {
        case Idle:
            return 0.0f;
        case Attack:
            // Exponential approach to 1.0 — matches standard DX envelope character.
            // Target slightly above 1.0 (1.02) ensures the threshold is crossed
            // cleanly. Was linear (level += attackRate) which sounded abrupt.
            level += (1.02f - level) * attackRate;
            if (level >= 1.0f)
            {
                level = 1.0f;
                stage = Decay1;
            }
            return level;
        case Decay1:
            level -= (level - sustainLevel) * decay1Coeff;
            level = flushDenormal(level);
            if (std::abs(level - sustainLevel) < 0.002f)
                stage = Sustain;
            return level;
        case Sustain:
            return level;
        case Release:
            level -= level * releaseCoeff;
            level = flushDenormal(level);
            if (level < 1e-6f)
            {
                level = 0.0f;
                stage = Idle;
            }
            return level;
        }
        return 0.0f;
    }

    bool isActive() const noexcept { return stage != Idle; }
    float getLevel() const noexcept { return level; }

    float sr = 0.0f;  // Sentinel: must be set by prepare() before use
    Stage stage = Idle;
    float level = 0.0f;
    float attackRate = 0.0f;
    float decay1Coeff = 0.0f;
    float sustainLevel = 0.0f;
    float releaseCoeff = 0.0f;
};

//==============================================================================
// OpcodeVoice
//==============================================================================
struct OpcodeVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;

    GlideProcessor glide;
    FMOperator carrier;
    FMOperator modulator;
    DXModulationEnvelope modEnv; // Controls FM index over time
    FilterEnvelope ampEnv;
    FilterEnvelope filterEnv;
    CytomicSVF svf;
    StandardLFO lfo1, lfo2;

    float feedbackState = 0.0f; // For feedback algorithm mode
    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        feedbackState = 0.0f;
        glide.reset();
        carrier.reset();
        modulator.reset();
        modEnv.kill();
        ampEnv.kill();
        filterEnv.kill();
        svf.reset();
        lfo1.reset();
        lfo2.reset();
    }
};

//==============================================================================
// OpcodeEngine — "Algorithm, 1983"
//==============================================================================
class OpcodeEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    juce::String getEngineId() const override { return "Opcode"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF5F9EA0); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    // getSpectralFingerprint() removed — FUSION inter-engine coupling was never wired. (#686)

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].carrier.prepare(srf);
            voices[i].modulator.prepare(srf);
            voices[i].modEnv.prepare(srf);
            voices[i].ampEnv.prepare(srf);
            voices[i].filterEnv.prepare(srf);
        }

        smoothRatio.prepare(srf);
        smoothIndex.prepare(srf);
        smoothBrightness.prepare(srf);
        smoothMigration.prepare(srf);

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
    }

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        (void)sampleIndex;
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
            couplingFilterMod += val * 2000.0f;
            break;
        case CouplingType::LFOToPitch:
            couplingPitchMod += val * 2.0f;
            break;
        case CouplingType::AmpToPitch:
            couplingPitchMod += val;
            break;
        case CouplingType::EnvToMorph:
            couplingIndexMod += val * 2.0f;
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
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
                wakeSilenceGate();
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

        if (isSilenceGateBypassed())
        {
            buffer.clear(0, numSamples);
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        auto loadP = [](std::atomic<float>* p, float def) { return p ? p->load(std::memory_order_relaxed) : def; };

        const int pAlgorithm = static_cast<int>(loadP(paramAlgorithm, 0.0f));
        const float pRatio = loadP(paramRatio, 2.0f);
        const float pIndex = loadP(paramIndex, 0.7f);
        const float pBrightness = loadP(paramBrightness, 10000.0f);
        const float pFeedback = loadP(paramFeedback, 0.0f);
        const float pFilterEnvAmt = loadP(paramFilterEnvAmt, 0.3f);
        const float pBendRange = loadP(paramBendRange, 2.0f);
        const float pMigration = loadP(paramMigration, 0.0f);
        const float pVelToIndex = loadP(paramVelToIndex, 0.6f);

        const float macroChar = loadP(paramMacroCharacter, 0.0f);
        const float macroMove = loadP(paramMacroMovement, 0.0f);
        const float macroCoup = loadP(paramMacroCoupling, 0.0f);
        const float macroSpace = loadP(paramMacroSpace, 0.0f);

        // D006: mod wheel -> index, aftertouch -> brightness
        float effectiveIndex =
            std::clamp(pIndex + macroChar * 0.5f + modWheelAmount * 0.8f + couplingIndexMod, 0.0f, 5.0f);
        float effectiveBright = std::clamp(
            pBrightness + macroMove * 5000.0f + aftertouchAmount * 4000.0f + couplingFilterMod, 200.0f, 20000.0f);
        float effectiveRatio = std::clamp(pRatio + macroSpace * 2.0f, 0.5f, 16.0f);

        smoothRatio.set(effectiveRatio);
        smoothIndex.set(effectiveIndex);
        smoothBrightness.set(effectiveBright);
        smoothMigration.set(std::clamp(pMigration + macroCoup * 0.5f, 0.0f, 1.0f));

        couplingFilterMod = 0.0f;
        const float pitchCouplingVal = couplingPitchMod;
        couplingPitchMod = 0.0f;
        couplingIndexMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;
        const float blockBendRatio = PitchBendUtil::semitonesToFreqRatio(bendSemitones + pitchCouplingVal);

        // LFO params
        const float lfo1Rate = loadP(paramLfo1Rate, 0.5f);
        const float lfo1Depth = loadP(paramLfo1Depth, 0.0f);
        const int lfo1Shape = static_cast<int>(loadP(paramLfo1Shape, 0.0f));
        const float lfo2Rate = loadP(paramLfo2Rate, 1.0f);
        const float lfo2Depth = loadP(paramLfo2Depth, 0.0f);
        const int lfo2Shape = static_cast<int>(loadP(paramLfo2Shape, 0.0f));

        for (auto& voice : voices)
        {
            if (!voice.active)
                continue;
            voice.lfo1.setRate(lfo1Rate, srf);
            voice.lfo1.setShape(lfo1Shape);
            voice.lfo2.setRate(lfo2Rate, srf);
            voice.lfo2.setShape(lfo2Shape);
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        for (int s = 0; s < numSamples; ++s)
        {
            const bool updateFilter = ((s & 15) == 0);
            float ratioNow = smoothRatio.process();
            float indexNow = smoothIndex.process();
            float brightNow = smoothBrightness.process();
            float migrationN = smoothMigration.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                // Update amp envelope ADSR per block so knob changes take effect on held notes
                voice.ampEnv.setADSR(
                    paramAttack  ? paramAttack->load()  : 0.005f,
                    paramDecay   ? paramDecay->load()   : 1.0f,
                    paramSustain ? paramSustain->load() : 0.5f,
                    paramRelease ? paramRelease->load() : 0.6f);

                float freq = voice.glide.process();
                freq *= blockBendRatio; // hoisted above — block-const bend + pitch coupling snapshot

                // LFO1 -> pitch vibrato (classic DX vibrato)
                float lfo1Val = voice.lfo1.process() * lfo1Depth;
                freq *= PitchBendUtil::semitonesToFreqRatio(lfo1Val * 0.5f);

                // LFO2 -> FM index modulation
                float lfo2Val = voice.lfo2.process() * lfo2Depth;

                // Modulation envelope — controls FM index over time
                // This is what creates the "bell attack into warm sustain"
                float modEnvLevel = voice.modEnv.process();

                // Velocity-sensitive FM index: harder = more harmonics
                float velIndex = indexNow + voice.velocity * pVelToIndex * 2.0f;
                velIndex = std::clamp(velIndex + lfo2Val * 0.5f, 0.0f, 8.0f);

                // Effective FM index = base index * modulation envelope
                float fmIndex = velIndex * modEnvLevel;

                // Update cached phase increments — freq changes each sample due to glide/LFO/bend.
                // This keeps the cache valid so process() stays O(1) with no per-sample division.
                voice.carrier.setFrequency(freq);
                voice.modulator.setFrequency(freq * ratioNow);

                float fmOutput = 0.0f;

                switch (pAlgorithm)
                {
                case 0: // Series: modulator → carrier (classic DX EP)
                {
                    float modOut = voice.modulator.process();
                    float phaseMod = modOut * fmIndex;
                    fmOutput = voice.carrier.process(phaseMod);
                    break;
                }
                case 1: // Parallel: modulator + carrier (organ-like)
                {
                    float modOut = voice.modulator.process();
                    float carrOut = voice.carrier.process();
                    fmOutput = carrOut + modOut * fmIndex * 0.5f;
                    break;
                }
                case 2: // Feedback: modulator feeds back into itself
                {
                    float fbAmount = pFeedback * voice.feedbackState;
                    float modOut = voice.modulator.process(fbAmount * 0.3f);
                    // Saturation clamp: prevent feedback runaway at high pFeedback values.
                    // fastTanh soft-clips feedbackState to roughly ±1.0, preserving
                    // FM character while bounding the signal from growing unbounded.
                    voice.feedbackState = fastTanh(modOut);
                    float phaseMod = modOut * fmIndex;
                    fmOutput = voice.carrier.process(phaseMod);
                    break;
                }
                }

                // Migration — subtle spectral enrichment from coupled engines
                if (migrationN > 0.01f)
                {
                    // Add very subtle inharmonicity (what happens when FM travels
                    // through acoustic traditions)
                    float migDetune = fastSin(voice.carrier.phase * 6.28318530718f * 1.003f) * migrationN * 0.08f;
                    fmOutput += migDetune;
                }

                // Amplitude envelope
                float ampLevel = voice.ampEnv.process();
                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // Filter — FM EP benefits from gentle LP to tame aliasing at high index.
                // Tick filter env per sample; refresh SVF coeffs every 16 samples
                // (~0.36ms @ 44.1k — below audible lag).
                float fEnvMod = voice.filterEnv.process() * pFilterEnvAmt * 4000.0f;
                if (updateFilter)
                {
                    float velBright = voice.velocity * 3000.0f;
                    float cutoff = std::clamp(brightNow + fEnvMod + velBright, 200.0f, 20000.0f);
                    voice.svf.setMode(CytomicSVF::Mode::LowPass);
                    voice.svf.setCoefficients(cutoff, 0.1f, srf);
                }
                float filtered = voice.svf.processSample(fmOutput);

                float output = filtered * ampLevel;

                mixL += output * voice.panL;
                mixR += output * voice.panR;
            }

            outL[s] = mixL;
            if (outR)
                outR[s] = mixR;
            couplingCacheL = mixL;
            couplingCacheR = mixR;
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

        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.snapTo(freq);
        v.feedbackState = 0.0f;

        v.carrier.prepare(srf);
        v.carrier.reset();
        v.modulator.prepare(srf);
        v.modulator.reset();

        // Cache phase increments at note-on. renderBlock recomputes these when
        // pitch or ratio changes (glide, bend, LFO), so they are also updated
        // per-sample in the render loop when glide is active.
        float ratio = paramRatio ? paramRatio->load() : 2.0f;
        v.carrier.setFrequency(freq);
        v.modulator.setFrequency(freq * ratio);

        // Amp envelope params — read before mod env so release can be shared
        float attack = paramAttack ? paramAttack->load() : 0.005f;
        float decay = paramDecay ? paramDecay->load() : 1.0f;
        float sustain = paramSustain ? paramSustain->load() : 0.5f;
        float release = paramRelease ? paramRelease->load() : 0.6f;

        // Modulation envelope — fast attack, moderate decay to sustain.
        // Release is coupled to amp envelope release so mod index tails off
        // in sync with the amplitude (was hardcoded 0.3f — BUG FIX).
        float modAtk = paramModAttack ? paramModAttack->load() : 0.001f;
        float modDk = paramModDecay ? paramModDecay->load() : 0.3f;
        float modSus = paramModSustain ? paramModSustain->load() : 0.2f;
        v.modEnv.prepare(srf);
        v.modEnv.setRates(modAtk, modDk, modSus, release);
        v.modEnv.trigger();

        // Amp envelope
        v.ampEnv.prepare(srf);
        v.ampEnv.setADSR(attack, decay, sustain, release);
        v.ampEnv.triggerHard();

        // Filter envelope — sustain is user-controlled via opco_filterSustain.
        // Default 0.0 preserves the classic DX EP behavior (filter closes fully after decay).
        // Non-zero sustain lets the filter hold open, giving smoother pad/string-like tones.
        float fltSus = paramFilterSustain ? paramFilterSustain->load() : 0.0f;
        v.filterEnv.prepare(srf);
        v.filterEnv.setADSR(0.001f, 0.4f + (1.0f - vel) * 0.4f, fltSus, 0.4f);
        v.filterEnv.triggerHard();

        v.svf.reset();

        // Stereo placement — keyboard position
        float pan = static_cast<float>(note - 36) / 60.0f;
        pan = std::clamp(pan, 0.0f, 1.0f);
        v.panL = std::cos(pan * 1.5707963f);
        v.panR = std::sin(pan * 1.5707963f);
    }

    void noteOff(int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note)
            {
                v.ampEnv.release();
                v.modEnv.release();
                v.filterEnv.release();
            }
        }
    }

    //==========================================================================
    // Parameters — 30 total
    //==========================================================================
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }
    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // FM core
        params.push_back(std::make_unique<PI>(juce::ParameterID{"opco_algorithm", 1}, "Opcode Algorithm", 0, 2, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_ratio", 1}, "Opcode Mod:Carrier Ratio",
                                              juce::NormalisableRange<float>(0.5f, 16.0f, 0.01f), 2.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_index", 1}, "Opcode FM Index",
                                              juce::NormalisableRange<float>(0.0f, 5.0f, 0.01f), 0.7f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_feedback", 1}, "Opcode Feedback",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_velToIndex", 1}, "Opcode Velocity to Index",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.6f));

        // Brightness
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_brightness", 1}, "Opcode Brightness",
                                              juce::NormalisableRange<float>(200.0f, 20000.0f, 0.0f, 0.3f), 10000.0f));

        // Amp envelope
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_attack", 1}, "Opcode Attack",
                                              juce::NormalisableRange<float>(0.001f, 0.5f, 0.0f, 0.3f), 0.005f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_decay", 1}, "Opcode Decay",
                                              juce::NormalisableRange<float>(0.05f, 8.0f, 0.0f, 0.4f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_sustain", 1}, "Opcode Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_release", 1}, "Opcode Release",
                                              juce::NormalisableRange<float>(0.01f, 5.0f, 0.0f, 0.4f), 0.6f));

        // Modulation envelope
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_modAttack", 1}, "Opcode Mod Env Attack",
                                              juce::NormalisableRange<float>(0.001f, 0.2f, 0.0f, 0.3f), 0.001f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_modDecay", 1}, "Opcode Mod Env Decay",
                                              juce::NormalisableRange<float>(0.01f, 3.0f, 0.0f, 0.4f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_modSustain", 1}, "Opcode Mod Env Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));

        // Filter
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_filterEnvAmt", 1}, "Opcode Filter Env Amount",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        // opco_filterSustain: how much the filter envelope holds open after decay.
        // At 0.0 (default) the filter closes completely — the classic DX EP character.
        // At 1.0 the filter stays fully open after the decay sweep, enabling pad-like tones.
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_filterSustain", 1}, "Opcode Filter Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // FUSION
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_migration", 1}, "Opcode Migration",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // Pitch
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_bendRange", 1}, "Opcode Pitch Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));

        // Macros
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_macroCharacter", 1}, "Opcode Macro CHARACTER",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_macroMovement", 1}, "Opcode Macro MOVEMENT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_macroCoupling", 1}, "Opcode Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_macroSpace", 1}, "Opcode Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // LFOs (D002/D005)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_lfo1Rate", 1}, "Opcode LFO1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_lfo1Depth", 1}, "Opcode LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"opco_lfo1Shape", 1}, "Opcode LFO1 Shape", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_lfo2Rate", 1}, "Opcode LFO2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opco_lfo2Depth", 1}, "Opcode LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"opco_lfo2Shape", 1}, "Opcode LFO2 Shape", 0, 4, 0));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramAlgorithm = apvts.getRawParameterValue("opco_algorithm");
        paramRatio = apvts.getRawParameterValue("opco_ratio");
        paramIndex = apvts.getRawParameterValue("opco_index");
        paramFeedback = apvts.getRawParameterValue("opco_feedback");
        paramVelToIndex = apvts.getRawParameterValue("opco_velToIndex");
        paramBrightness = apvts.getRawParameterValue("opco_brightness");
        paramAttack = apvts.getRawParameterValue("opco_attack");
        paramDecay = apvts.getRawParameterValue("opco_decay");
        paramSustain = apvts.getRawParameterValue("opco_sustain");
        paramRelease = apvts.getRawParameterValue("opco_release");
        paramModAttack = apvts.getRawParameterValue("opco_modAttack");
        paramModDecay = apvts.getRawParameterValue("opco_modDecay");
        paramModSustain = apvts.getRawParameterValue("opco_modSustain");
        paramFilterEnvAmt = apvts.getRawParameterValue("opco_filterEnvAmt");
        paramFilterSustain = apvts.getRawParameterValue("opco_filterSustain");
        paramMigration = apvts.getRawParameterValue("opco_migration");
        paramBendRange = apvts.getRawParameterValue("opco_bendRange");
        paramMacroCharacter = apvts.getRawParameterValue("opco_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("opco_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("opco_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("opco_macroSpace");
        paramLfo1Rate = apvts.getRawParameterValue("opco_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("opco_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("opco_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("opco_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("opco_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("opco_lfo2Shape");
    }

private:
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float srf = 0.0f;  // Sentinel: must be set by prepare() before use

    std::array<OpcodeVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount{0};

    ParameterSmoother smoothRatio, smoothIndex, smoothBrightness, smoothMigration;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f, couplingIndexMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    std::atomic<float>* paramAlgorithm = nullptr;
    std::atomic<float>* paramRatio = nullptr;
    std::atomic<float>* paramIndex = nullptr;
    std::atomic<float>* paramFeedback = nullptr;
    std::atomic<float>* paramVelToIndex = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramSustain = nullptr;
    std::atomic<float>* paramRelease = nullptr;
    std::atomic<float>* paramModAttack = nullptr;
    std::atomic<float>* paramModDecay = nullptr;
    std::atomic<float>* paramModSustain = nullptr;
    std::atomic<float>* paramFilterEnvAmt = nullptr;
    std::atomic<float>* paramFilterSustain = nullptr;
    std::atomic<float>* paramMigration = nullptr;
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
