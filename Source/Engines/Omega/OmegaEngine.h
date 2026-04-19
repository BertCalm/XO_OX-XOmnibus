// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OmegaEngine.h — XOmega | "The Distillation"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOmega is the still in the cellar — copper and glass, heat and
//      patience, the slow separation of what matters from what doesn't.
//      Distillation as synthesis: every complex FM bass sound is a
//      distillation away from simplicity. The DX bass, the Reese,
//      the crystalline digital fundamental. The opposite of XOgre's
//      earthy mass — XOmega is pure, concentrated, refined.
//
//  ENGINE CONCEPT:
//      2-operator FM synthesis with ratio control, feedback, and a
//      distillation model where FM complexity reduces toward pure carrier
//      over note sustain. The omega_purity parameter controls the balance
//      between mathematical precision (1.0) and noisy instability (0.0).
//      Digital bass at its most essential.
//
//  CELLAR QUAD ROLE: FM/Digital Bass / Distillation
//      The crystalline endpoint of the CELLAR spectrum. While XOgre
//      provides seismic sub, XOlate analog warmth, and XOaken organic
//      wood, XOmega provides precision: the bass note as mathematical
//      object, as pure frequency ratio, as distilled essence of pitch.
//
//  DSP ARCHITECTURE:
//      1. 2-operator FM: carrier (sine) + modulator (sine) with ratio + feedback
//      2. Distillation model: mod index decreases over note sustain
//         (complex -> pure). Every note distills from complexity to purity.
//      3. Purity parameter: at 1.0, FM is mathematically pure. At 0.0,
//         noise and phase instability creep in (the "impure distillate")
//      4. Algorithm selector: ratio presets for classic FM bass tones
//      5. Digital precision: no analog warmth, no body resonance — pure math
//      6. MIDI-domain gravitational coupling
//
//  HISTORICAL LINEAGE:
//      Chowning (1973 FM synthesis), Yamaha DX7 (1983), Bristow-Johnson (2001),
//      Native Instruments FM8, Dexed. The DX bass is one of the most important
//      bass sounds in electronic music history.
//
//  Accent: Copper Still #B04010 (from Visionary doc)
//  Parameter prefix: omega_
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

//==============================================================================
// OmegaFMOperator — Single sine-wave FM operator with phase accumulation and feedback.
//==============================================================================
struct OmegaFMOperator
{
    void setFrequency(float freqHz, float sampleRate) noexcept { phaseInc = freqHz / sampleRate; }

    float process(float modInput = 0.0f) noexcept
    {
        float effectivePhase = phase + modInput;
        float out = fastSin(effectivePhase * kTwoPi);

        // Self-feedback: store raw out so the call site (which multiplies by feedback)
        // produces a single feedback gain, not feedback².
        lastOutput = out;

        phase += phaseInc;
        if (phase >= 1.0f)
            phase -= 1.0f;
        if (phase < 0.0f)
            phase += 1.0f;

        return out;
    }

    void setFeedback(float fb) noexcept { feedback = fb; }

    void reset() noexcept
    {
        phase = 0.0f;
        lastOutput = 0.0f;
    }

    float phase = 0.0f;
    float phaseInc = 0.0f;
    float feedback = 0.0f;
    float lastOutput = 0.0f;

    static constexpr float kTwoPi = 6.28318530717958647692f;
};

//==============================================================================
// DistillationModel — FM modulation index decreases over note sustain.
// Complex spectrum simplifies toward pure carrier. Irreversible within note.
// This is the opposite of additive attack: distilled bass starts complex
// and finishes as a near-pure sine.
//==============================================================================
struct DistillationModel
{
    void trigger(float initialIndex) noexcept
    {
        currentIndex = initialIndex;
        noteAge = 0.0f;
    }

    // CPU fix: precompute decayCoeff once per block (distillRate and dtSec are
    // block-rate constants). Call computeDecayCoeff() before the sample loop,
    // then call processWithCoeff() inside it to avoid std::exp per sample per voice.
    static float computeDecayCoeff(float distillRate, float dtSec) noexcept
    {
        float halfLife = 2.0f + (1.0f - distillRate) * 18.0f; // 2s to 20s half-life
        return std::exp(-0.693f * dtSec / halfLife);          // ln(2)
    }

    float processWithCoeff(float decayCoeff, float dtSec) noexcept
    {
        noteAge += dtSec;
        currentIndex *= decayCoeff;
        currentIndex = flushDenormal(currentIndex);
        return std::max(currentIndex, 0.001f); // never fully zero — keep a trace
    }

    // Legacy single-call path (kept for reference; not used in render loop).
    float process(float distillRate, float dtSec) noexcept
    {
        return processWithCoeff(computeDecayCoeff(distillRate, dtSec), dtSec);
    }

    void reset() noexcept
    {
        currentIndex = 1.0f;
        noteAge = 0.0f;
    }

    float currentIndex = 1.0f;
    float noteAge = 0.0f;
};

//==============================================================================
// OmegaVoice — FM bass voice with 2-op FM + distillation
//==============================================================================
struct OmegaVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 36;
    float velocity = 0.0f;

    GlideProcessor glide;
    OmegaFMOperator carrier;
    OmegaFMOperator modulator;
    DistillationModel distill;
    FilterEnvelope ampEnv;
    FilterEnvelope filterEnv;
    CytomicSVF outputFilter;
    StandardLFO lfo1, lfo2;

    // Gravitational mass
    float gravityMass = 0.0f;
    float noteHeldTime = 0.0f;

    // Noise/impurity state
    uint32_t noiseState = 12345u;

    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        gravityMass = 0.0f;
        noteHeldTime = 0.0f;
        glide.reset();
        carrier.reset();
        modulator.reset();
        distill.reset();
        ampEnv.kill();
        filterEnv.kill();
        outputFilter.reset();
        lfo1.reset();
        lfo2.reset();
        noiseState = 12345u;
    }
};

//==============================================================================
// OmegaEngine — "The Distillation" | FM/Digital Bass
//==============================================================================
class OmegaEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    // FM ratio presets for the algorithm parameter
    // Classic bass ratios from DX7 literature
    struct RatioPreset
    {
        float modRatio;
        float modIndexDefault;
        const char* name;
    };

    static constexpr int kNumAlgorithms = 8;

    juce::String getEngineId() const override { return "Omega"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF003366); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);
        inverseSr_ = 1.0f / srf;

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].ampEnv.prepare(srf);
            voices[i].filterEnv.prepare(srf);
            voices[i].noiseState = static_cast<uint32_t>(i * 7919 + 42);
        }

        smoothModIndex.prepare(srf);
        smoothRatio.prepare(srf);
        smoothFeedback.prepare(srf);
        smoothPurity.prepare(srf);
        smoothBrightness.prepare(srf);
        smoothDistillRate.prepare(srf);

        prepareSilenceGate(sr, maxBlockSize, 300.0f);
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
            couplingFilterMod += val * 2000.0f;
            break;
        case CouplingType::LFOToPitch:
            couplingPitchMod += val * 2.0f;
            break;
        case CouplingType::AmpToPitch:
            couplingPitchMod += val;
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

        const float pModIndex = loadP(paramModIndex, 3.0f);
        const float pRatio = loadP(paramRatio, 1.0f);
        const float pFeedback = loadP(paramFeedback, 0.0f);
        const float pPurity = loadP(paramPurity, 0.8f);
        const float pDistill = loadP(paramDistillRate, 0.3f);
        const int pAlgorithm = static_cast<int>(loadP(paramAlgorithm, 0.0f));
        const float pBrightness = loadP(paramBrightness, 8000.0f);
        const float pGlide = loadP(paramGlide, 0.0f);
        const float pAttack = loadP(paramAttack, 0.001f);
        const float pDecay = loadP(paramDecay, 0.3f);
        const float pSustain = loadP(paramSustain, 0.7f);
        const float pRelease = loadP(paramRelease, 0.2f);
        const float pFiltEnvAmt = loadP(paramFilterEnvAmount, 0.4f);
        const float pBendRange = loadP(paramBendRange, 2.0f);
        const float pGravity = loadP(paramGravity, 0.5f);

        const float macroChar = loadP(paramMacroCharacter, 0.0f);
        const float macroMove = loadP(paramMacroMovement, 0.0f);
        const float macroCoup = loadP(paramMacroCoupling, 0.0f);
        const float macroSpace = loadP(paramMacroSpace, 0.0f);

        // Algorithm -> ratio mapping (classic DX bass ratios)
        // These override pRatio when an algorithm is selected
        static constexpr float algoRatios[kNumAlgorithms] = {
            1.0f,   // 0: Unison (carrier = modulator)
            2.0f,   // 1: Octave up
            3.0f,   // 2: Fifth up (classic DX bass)
            0.5f,   // 3: Octave down
            1.414f, // 4: Minor seventh (Reese bass territory)
            3.5f,   // 5: Inharmonic metallic
            7.0f,   // 6: Bell-like
            0.0f    // 7: Custom (uses pRatio directly)
        };

        float effectiveRatio = (pAlgorithm < kNumAlgorithms - 1) ? algoRatios[pAlgorithm] : pRatio;

        // D006: mod wheel → mod index, aftertouch → feedback
        float effectiveModIndex = std::clamp(pModIndex + macroChar * 3.0f + modWheelAmount * 4.0f, 0.0f, 20.0f);
        float effectiveFeedback = std::clamp(pFeedback + aftertouchAmount * 0.3f + macroMove * 0.2f, 0.0f, 1.0f);
        float effectiveBright = std::clamp(pBrightness + macroChar * 4000.0f + couplingFilterMod, 200.0f, 20000.0f);

        smoothModIndex.set(effectiveModIndex);
        smoothRatio.set(effectiveRatio);
        smoothFeedback.set(effectiveFeedback);
        smoothPurity.set(pPurity);
        smoothBrightness.set(effectiveBright);
        smoothDistillRate.set(pDistill);

        // Snapshot pitch coupling before reset (#1118).
        const float blockCouplingPitchMod = couplingPitchMod;
        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;

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
            voice.glide.setTime(pGlide, srf);
            voice.ampEnv.setADSR(pAttack, pDecay, pSustain, pRelease);
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        const float dtSec = inverseSr_;

        // CPU fix 1 (OMEGA): precompute distillation decayCoeff once per block.
        // pDistill and dtSec are both block-rate constants; std::exp is expensive.
        // smoothDistillRate has just been set from pDistill — use pDistill directly
        // for the block-rate coefficient.
        const float blockDecayCoeff = DistillationModel::computeDecayCoeff(pDistill, dtSec);

        // CPU fix 2 (OMEGA): precompute per-voice pan gains once per block.
        // macroSpace and voice index are both block-constant — no need to recompute
        // std::cos/std::sin on every sample for every voice.
        for (int vi = 0; vi < kMaxVoices; ++vi)
        {
            float voiceSpread = (static_cast<float>(vi) - static_cast<float>(kMaxVoices - 1) * 0.5f) /
                                static_cast<float>(kMaxVoices); // normalized -0.5..+0.5
            float pan = 0.5f + macroSpace * voiceSpread;
            pan = std::clamp(pan, 0.01f, 0.99f);
            voices[vi].panL = std::cos(pan * 1.5707963f);
            voices[vi].panR = std::sin(pan * 1.5707963f);
        }

        // Hoist pitch-bend ratio; uses the pre-reset snapshot (#1118).
        const float blockBendRatio =
            PitchBendUtil::semitonesToFreqRatio(bendSemitones + blockCouplingPitchMod);

        for (int s = 0; s < numSamples; ++s)
        {
            const bool updateFilter = ((s & 15) == 0);
            float modIdxNow = smoothModIndex.process();
            float ratioNow = smoothRatio.process();
            float fbNow = smoothFeedback.process();
            float purityNow = smoothPurity.process();
            float brightNow = smoothBrightness.process();
            float distRNow = smoothDistillRate.process();
            (void)distRNow; // block-rate decayCoeff used instead (CPU fix 1)

            float mixL = 0.0f, mixR = 0.0f;

            for (int vi = 0; vi < kMaxVoices; ++vi)
            {
                auto& voice = voices[vi];
                if (!voice.active)
                    continue;

                // panL/panR precomputed before sample loop (CPU fix 2 — block-constant).

                float freq = voice.glide.process();
                freq *= blockBendRatio; // hoisted above — was per-sample per-voice fastPow2

                float lfo1Val = voice.lfo1.process() * lfo1Depth;
                float lfo2Val = voice.lfo2.process() * lfo2Depth;

                // Distillation: mod index decays over note sustain.
                // CPU fix 1: use block-rate precomputed decayCoeff instead of std::exp per sample.
                float distilledIndex = voice.distill.processWithCoeff(blockDecayCoeff, dtSec);
                float activeModIndex = modIdxNow * distilledIndex;

                // LFO1 -> mod index modulation
                activeModIndex = std::max(0.0f, activeModIndex + lfo1Val * 2.0f);

                // omega_gravity: gravityMass steers the modulator ratio toward the nearest
                // integer ratio (harmonic lock). High gravity = mathematical precision,
                // low gravity = free inharmonic drift. This gives the "distillation"
                // metaphor a second dimension: not just time-based but gravity-based.
                float nearestInt = std::round(ratioNow);
                nearestInt = std::max(nearestInt, 1.0f); // clamp to 1 minimum
                float gravitatedRatio = ratioNow + (nearestInt - ratioNow) * voice.gravityMass;

                // omega_macroCoupling: coupling adds ratio detune, pulling the modulator
                // slightly sharp/flat for inter-engine entanglement coloring.
                gravitatedRatio += macroCoup * 0.1f;

                // Set operator frequencies using gravity-adjusted ratio
                float modFreq = freq * gravitatedRatio;
                voice.modulator.setFrequency(modFreq, srf);
                voice.modulator.setFeedback(fbNow * 0.5f);
                voice.carrier.setFrequency(freq, srf);

                // FM synthesis: modulator -> carrier
                // Modulator output scaled by mod index (in radians of phase deviation)
                float modOut = voice.modulator.process(voice.modulator.lastOutput * fbNow * 0.5f);
                float modSignal = modOut * activeModIndex / (2.0f * 3.14159265f);

                // Purity: at 1.0, pure FM. At 0.0, noise and instability creep in
                if (purityNow < 0.99f)
                {
                    float impurity = 1.0f - purityNow;
                    voice.noiseState = voice.noiseState * 1664525u + 1013904223u;
                    float noise = (static_cast<float>(voice.noiseState & 0xFFFF) / 32768.0f - 1.0f);
                    modSignal += noise * impurity * 0.02f; // subtle phase noise
                    // Phase instability: slight random frequency drift
                    float drift = noise * impurity * 0.001f;
                    voice.carrier.phase += drift;
                }

                float carrierOut = voice.carrier.process(modSignal);

                // D001: velocity shapes FM brightness (more velocity = brighter attack)
                // Tick env per sample; decimate SVF coeff refresh to every 16.
                float envMod = voice.filterEnv.process() * pFiltEnvAmt * 5000.0f;
                if (updateFilter)
                {
                    float velBright = brightNow + voice.velocity * 4000.0f;
                    float cutoff = std::clamp(velBright + envMod + lfo2Val * 2000.0f, 200.0f, 20000.0f);
                    voice.outputFilter.setMode(CytomicSVF::Mode::LowPass);
                    voice.outputFilter.setCoefficients(cutoff, 0.2f, srf);
                }
                float filtered = voice.outputFilter.processSample(carrierOut);

                // Amplitude envelope
                float ampLevel = voice.ampEnv.process();
                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // Gravity mass
                voice.noteHeldTime += dtSec;
                voice.gravityMass = std::min(1.0f, voice.gravityMass + dtSec * 0.1f * pGravity);

                float output = filtered * ampLevel * voice.velocity;

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

        v.reset();
        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.snapTo(freq);
        v.gravityMass = 0.0f;
        v.noteHeldTime = 0.0f;

        float initModIndex = paramModIndex ? paramModIndex->load() : 3.0f;
        v.distill.trigger(initModIndex);

        v.ampEnv.prepare(srf);
        v.filterEnv.prepare(srf);

        float attack = paramAttack ? paramAttack->load() : 0.001f;
        float decay = paramDecay ? paramDecay->load() : 0.3f;
        float sustain = paramSustain ? paramSustain->load() : 0.7f;
        float release = paramRelease ? paramRelease->load() : 0.2f;
        v.ampEnv.setADSR(attack, decay, sustain, release);
        v.ampEnv.triggerHard();

        float fAttack = paramFilterAttack ? paramFilterAttack->load() : 0.001f;
        float fDecay = paramFilterDecay ? paramFilterDecay->load() : 0.3f;
        v.filterEnv.setADSR(fAttack, fDecay, 0.0f, 0.2f);
        v.filterEnv.triggerHard();

        // Subtle stereo spread
        float pan = 0.5f + (static_cast<float>(idx) - 3.5f) * 0.03f;
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

        // FM Synthesis
        params.push_back(std::make_unique<PI>(juce::ParameterID{"omega_algorithm", 1}, "Omega Algorithm", 0, 7, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_ratio", 1}, "Omega Ratio",
                                              juce::NormalisableRange<float>(0.25f, 16.0f, 0.0f, 0.3f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_modIndex", 1}, "Omega Mod Index",
                                              juce::NormalisableRange<float>(0.0f, 20.0f, 0.0f, 0.3f), 3.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_feedback", 1}, "Omega Feedback",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // Distillation
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_purity", 1}, "Omega Purity",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_distillRate", 1}, "Omega Distill Rate",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Tone
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_brightness", 1}, "Omega Brightness",
                                              juce::NormalisableRange<float>(200.0f, 20000.0f, 0.0f, 0.3f), 8000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_filterEnvAmount", 1}, "Omega Filter Env Amount",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_filterAttack", 1}, "Omega Filter Attack",
                                              juce::NormalisableRange<float>(0.001f, 2.0f, 0.0f, 0.3f), 0.001f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_filterDecay", 1}, "Omega Filter Decay",
                                              juce::NormalisableRange<float>(0.01f, 5.0f, 0.0f, 0.3f), 0.3f));

        // Amp Envelope
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_attack", 1}, "Omega Attack",
                                              juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.3f), 0.001f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_decay", 1}, "Omega Decay",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.3f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_sustain", 1}, "Omega Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_release", 1}, "Omega Release",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.3f), 0.2f));

        // Performance
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_glide", 1}, "Omega Glide",
                                              juce::NormalisableRange<float>(0.0f, 5.0f, 0.0f, 0.3f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_bendRange", 1}, "Omega Pitch Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));

        // Gravitational coupling
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_gravity", 1}, "Omega Gravity",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Macros
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_macroCharacter", 1}, "Omega Macro CHARACTER",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_macroMovement", 1}, "Omega Macro MOVEMENT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_macroCoupling", 1}, "Omega Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_macroSpace", 1}, "Omega Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // LFOs
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_lfo1Rate", 1}, "Omega LFO1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_lfo1Depth", 1}, "Omega LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"omega_lfo1Shape", 1}, "Omega LFO1 Shape", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_lfo2Rate", 1}, "Omega LFO2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"omega_lfo2Depth", 1}, "Omega LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"omega_lfo2Shape", 1}, "Omega LFO2 Shape", 0, 4, 0));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramAlgorithm = apvts.getRawParameterValue("omega_algorithm");
        paramRatio = apvts.getRawParameterValue("omega_ratio");
        paramModIndex = apvts.getRawParameterValue("omega_modIndex");
        paramFeedback = apvts.getRawParameterValue("omega_feedback");
        paramPurity = apvts.getRawParameterValue("omega_purity");
        paramDistillRate = apvts.getRawParameterValue("omega_distillRate");
        paramBrightness = apvts.getRawParameterValue("omega_brightness");
        paramFilterEnvAmount = apvts.getRawParameterValue("omega_filterEnvAmount");
        paramFilterAttack = apvts.getRawParameterValue("omega_filterAttack");
        paramFilterDecay = apvts.getRawParameterValue("omega_filterDecay");
        paramAttack = apvts.getRawParameterValue("omega_attack");
        paramDecay = apvts.getRawParameterValue("omega_decay");
        paramSustain = apvts.getRawParameterValue("omega_sustain");
        paramRelease = apvts.getRawParameterValue("omega_release");
        paramGlide = apvts.getRawParameterValue("omega_glide");
        paramBendRange = apvts.getRawParameterValue("omega_bendRange");
        paramGravity = apvts.getRawParameterValue("omega_gravity");
        paramMacroCharacter = apvts.getRawParameterValue("omega_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("omega_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("omega_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("omega_macroSpace");
        paramLfo1Rate = apvts.getRawParameterValue("omega_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("omega_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("omega_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("omega_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("omega_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("omega_lfo2Shape");
    }

private:
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float srf = 0.0f;  // Sentinel: must be set by prepare() before use
    float inverseSr_ = 1.0f / 48000.0f;

    std::array<OmegaVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount{0};

    ParameterSmoother smoothModIndex, smoothRatio, smoothFeedback;
    ParameterSmoother smoothPurity, smoothBrightness, smoothDistillRate;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    std::atomic<float>* paramAlgorithm = nullptr;
    std::atomic<float>* paramRatio = nullptr;
    std::atomic<float>* paramModIndex = nullptr;
    std::atomic<float>* paramFeedback = nullptr;
    std::atomic<float>* paramPurity = nullptr;
    std::atomic<float>* paramDistillRate = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramFilterEnvAmount = nullptr;
    std::atomic<float>* paramFilterAttack = nullptr;
    std::atomic<float>* paramFilterDecay = nullptr;
    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramSustain = nullptr;
    std::atomic<float>* paramRelease = nullptr;
    std::atomic<float>* paramGlide = nullptr;
    std::atomic<float>* paramBendRange = nullptr;
    std::atomic<float>* paramGravity = nullptr;
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
