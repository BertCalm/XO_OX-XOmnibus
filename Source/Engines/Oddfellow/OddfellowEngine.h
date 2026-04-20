// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OddfellowEngine.h — XOddfellow | "The Night Market Wurlitzer"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOddfellow is the Wurlitzer electric piano that lives in the night
//      market — open after the restaurants close, serving sounds that are
//      urgent and imperfect and exactly right. The reed vibrates with the
//      grit of a busker economy: slightly warbled, slightly driven, always
//      characterful. Oddfellow knows every market on every continent where
//      amplified instruments have competed with vendors and traffic.
//
//  ENGINE CONCEPT:
//      A physical model of the Wurlitzer reed-and-pickup system. A hammer
//      strikes a vibrating steel reed near a magnetic pickup. The reed's
//      vibration is softer and warmer than a tine — more fundamental, less
//      bell. The built-in preamp was always slightly driven, adding the
//      signature warmth/grit that made the Wurlitzer the quintessential
//      "character" electric piano. The built-in tremolo circuit is the
//      Wurlitzer's most recognizable feature after the reed tone itself.
//
//  SOURCE TRADITION TEST:
//      Must sound broken enough. The Wurlitzer's reedy character comes
//      partly from its imperfection — reed warble, drive from the amplifier,
//      the slight distortion that gives it warmth. A clean Wurlitzer isn't
//      a Wurlitzer.
//
//  CULTURAL ROUTE: Night Market (street food fusion)
//      Busker economy, night markets, street music. The Wurlitzer's grit
//      belongs to the cramped stage, the slightly-too-loud PA.
//
//  Accent: Neon Night #FF6B35
//  Parameter prefix: oddf_
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
// WurliReedModel — Vibrating reed physical model.
//
// The Wurlitzer uses a flat steel reed (like a tuning fork) struck by a
// felt hammer. The reed produces a tone dominated by fundamental and low
// harmonics, with a characteristic slight warble from imperfect clamping.
// Unlike the Rhodes tine (which is bright and bell-like), the Wurli reed
// is warm and slightly buzzy — more like a clarinet than a bell.
//
// The reed's vibration characteristics:
//   1. Strong fundamental
//   2. Warm 2nd harmonic (the "body")
//   3. Slight inharmonic upper partials (reed clamping imperfection)
//   4. Natural warble from manufacturing variance
//==============================================================================
struct WurliReedModel
{
    static constexpr int kNumPartials = 5;

    // Reed partial ratios — slightly inharmonic (reed clamping effect)
    static constexpr float kReedRatios[kNumPartials] = {1.0f, 2.01f, 3.03f, 4.08f, 5.15f};

    // Relative amplitudes — fundamental-heavy, warm, fewer upper partials
    static constexpr float kReedAmps[kNumPartials] = {1.0f, 0.5f, 0.2f, 0.08f, 0.03f};

    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        for (int i = 0; i < kNumPartials; ++i)
        {
            phases[i] = 0.0f;
            partialLevels[i] = 0.0f;
            // CPU FIX: cache decay rates — they depend only on sr and partial index.
            // std::exp() was being called 5x per voice per sample inside process().
            cachedDecayRate[i] = 1.0f - std::exp(-1.0f / (sampleRate * (3.0f - static_cast<float>(i) * 0.4f)));
        }
    }

    void trigger(float velocity, float reedStiffness) noexcept
    {
        vel = velocity;
        for (int i = 0; i < kNumPartials; ++i)
        {
            // Stiffer reed = more upper partials, softer reed = fundamental only
            float stiffScale = 1.0f - (1.0f - reedStiffness) * static_cast<float>(i) * 0.3f;
            if (stiffScale < 0.0f)
                stiffScale = 0.0f;
            partialLevels[i] = kReedAmps[i] * velocity * stiffScale;
        }

        // Reed warble: slight frequency modulation from imperfect clamping
        warblePhase = 0.0f;
        warbleDepth = (1.0f - reedStiffness) * 0.003f; // softer = more warble
    }

    float process(float fundamentalHz, float reedParam) noexcept
    {
        float out = 0.0f;

        // Reed warble modulation (slow pitch drift, ~3-6 Hz)
        warblePhase += 4.5f / sr;
        if (warblePhase >= 1.0f)
            warblePhase -= 1.0f;
        float warble = 1.0f + fastSin(warblePhase * 6.28318530718f) * warbleDepth;

        for (int i = 0; i < kNumPartials; ++i)
        {
            // Slightly inharmonic ratios — reed stiffness makes them more harmonic
            float ratio = 1.0f + (kReedRatios[i] - 1.0f) * (1.0f - reedParam * 0.3f + 0.7f);
            float freq = fundamentalHz * ratio * warble;
            if (freq >= sr * 0.49f)
                continue;

            float phaseInc = freq / sr;
            phases[i] += phaseInc;
            if (phases[i] >= 1.0f)
                phases[i] -= 1.0f;

            out += fastSin(phases[i] * 6.28318530718f) * partialLevels[i];

            // Reed decay: use cached rate (computed once in prepare() — CPU FIX)
            partialLevels[i] -= partialLevels[i] * cachedDecayRate[i] * 0.05f;
            partialLevels[i] = flushDenormal(partialLevels[i]);
        }

        return out;
    }

    void reset() noexcept
    {
        for (int i = 0; i < kNumPartials; ++i)
        {
            phases[i] = 0.0f;
            partialLevels[i] = 0.0f;
        }
        warblePhase = 0.0f;
    }

    float sr = 0.0f;  // Sentinel: must be set by prepare() before use
    float vel = 0.0f;
    float phases[kNumPartials] = {};
    float partialLevels[kNumPartials] = {};
    float warblePhase = 0.0f;
    float warbleDepth = 0.002f;
    // CPU FIX: cached decay rates — constant for a given sr + partial index
    float cachedDecayRate[kNumPartials] = {};
};

//==============================================================================
// WurliPreamp — Always-on tube preamp with inherent drive.
//
// The Wurlitzer 200A's preamp was always slightly saturated — even at
// "clean" settings there was warmth from the tube circuit. At higher
// levels, the asymmetric tube clipping produced the signature bark.
// Unlike the Rhodes amp stage (which was relatively clean), the Wurli
// preamp IS the sound.
//==============================================================================
struct WurliPreamp
{
    float process(float input, float drive) noexcept
    {
        // The Wurli preamp is never truly clean — minimum drive of 1.5
        float effectiveDrive = 1.5f + drive * 4.0f;

        // Tube-style asymmetric saturation
        float driven = input * effectiveDrive;

        // Odd-harmonic emphasis (reed character)
        float saturated = fastTanh(driven * 0.8f) * 0.7f + fastTanh(driven * 1.5f) * 0.3f;

        // DC removal
        dcState += 0.0002f * (saturated - dcState);
        saturated -= dcState;

        return saturated;
    }

    void reset() noexcept { dcState = 0.0f; }

    float dcState = 0.0f;
};

//==============================================================================
// OddfellowVoice
//==============================================================================
struct OddfellowVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;

    GlideProcessor glide;
    WurliReedModel reed;
    WurliPreamp preamp;
    FilterEnvelope ampEnv;
    FilterEnvelope filterEnv;
    CytomicSVF svf;
    StandardLFO lfo1, lfo2;
    StandardLFO tremoloLFO;

    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        glide.reset();
        reed.reset();
        preamp.reset();
        ampEnv.kill();
        filterEnv.kill();
        svf.reset();
        lfo1.reset();
        lfo2.reset();
        tremoloLFO.reset();
    }
};

//==============================================================================
// OddfellowEngine — "The Night Market Wurlitzer"
//==============================================================================
class OddfellowEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    juce::String getEngineId() const override { return "Oddfellow"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFB87333); }
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
            voices[i].reed.prepare(srf);
            voices[i].ampEnv.prepare(srf);
            voices[i].filterEnv.prepare(srf);
            voices[i].tremoloLFO.setShape(StandardLFO::Sine);
            voices[i].tremoloLFO.reset(static_cast<float>(i) / static_cast<float>(kMaxVoices));
            // Per-voice warble phase offset: spread across 0..1 so voices don't warble in unison.
            // This is the fix for monophonic warble — each voice starts at a different phase.
            voices[i].reed.warblePhase = static_cast<float>(i) / static_cast<float>(kMaxVoices);
        }

        smoothReed.prepare(srf);
        smoothDrive.prepare(srf);
        smoothBrightness.prepare(srf);
        smoothTremRate.prepare(srf);
        smoothTremDepth.prepare(srf);
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
            couplingReedMod += val;
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
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        auto loadP = [](std::atomic<float>* p, float def) { return p ? p->load(std::memory_order_relaxed) : def; };

        const float pReed = loadP(paramReed, 0.5f);
        const float pDrive = loadP(paramDrive, 0.3f);
        const float pBrightness = loadP(paramBrightness, 4000.0f);
        const float pTremRate = loadP(paramTremRate, 5.5f);
        const float pTremDepth = loadP(paramTremDepth, 0.4f);
        const float pFilterEnvAmt = loadP(paramFilterEnvAmt, 0.5f);
        const float pBendRange = loadP(paramBendRange, 2.0f);
        const float pMigration = loadP(paramMigration, 0.0f);

        const float macroChar = loadP(paramMacroCharacter, 0.0f);
        const float macroMove = loadP(paramMacroMovement, 0.0f);
        const float macroCoup = loadP(paramMacroCoupling, 0.0f);
        const float macroSpace = loadP(paramMacroSpace, 0.0f);

        // D006: mod wheel -> drive, aftertouch -> reed stiffness
        float effectiveReed =
            std::clamp(pReed + macroChar * 0.4f + aftertouchAmount * 0.3f + couplingReedMod, 0.0f, 1.0f);
        float effectiveDrive = std::clamp(pDrive + macroChar * 0.3f + modWheelAmount * 0.5f, 0.0f, 1.0f);
        float effectiveBright = std::clamp(
            pBrightness + macroMove * 3000.0f + aftertouchAmount * 2000.0f + couplingFilterMod, 200.0f, 16000.0f);
        float effectiveTremDepth = std::clamp(pTremDepth + macroMove * 0.3f, 0.0f, 1.0f);

        smoothReed.set(effectiveReed);
        smoothDrive.set(effectiveDrive);
        smoothBrightness.set(effectiveBright);
        smoothTremRate.set(pTremRate + macroSpace * 4.0f);
        smoothTremDepth.set(effectiveTremDepth);
        smoothMigration.set(std::clamp(pMigration + macroCoup * 0.5f, 0.0f, 1.0f));

        couplingFilterMod = 0.0f;
        const float pitchCouplingVal = couplingPitchMod;
        couplingPitchMod = 0.0f;
        couplingReedMod = 0.0f;

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
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        // Hoisted per-block (was incorrectly per-sample — fix 2026-04-19): atomic loads and setADSR fire once per block, not per sample.
        const float envA = paramAttack  ? paramAttack->load()  : 0.005f;
        const float envD = paramDecay   ? paramDecay->load()   : 0.6f;
        const float envS = paramSustain ? paramSustain->load() : 0.5f;
        const float envR = paramRelease ? paramRelease->load() : 0.4f;
        for (auto& voice : voices)
            if (voice.active)
                voice.ampEnv.setADSR(envA, envD, envS, envR);

        for (int s = 0; s < numSamples; ++s)
        {
            float reedNow = smoothReed.process();
            float driveNow = smoothDrive.process();
            float brightNow = smoothBrightness.process();
            float tremRateN = smoothTremRate.process();
            float tremDepthN = smoothTremDepth.process();
            float migrationN = smoothMigration.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;


                float freq = voice.glide.process();
                freq *= PitchBendUtil::semitonesToFreqRatio(bendSemitones + pitchCouplingVal);

                // LFO1 -> pitch vibrato
                float lfo1Val = voice.lfo1.process() * lfo1Depth;
                freq *= PitchBendUtil::semitonesToFreqRatio(lfo1Val * 0.5f);

                // LFO2 -> drive modulation
                float lfo2Val = voice.lfo2.process() * lfo2Depth;
                float voiceDrive = std::clamp(driveNow + lfo2Val * 0.3f, 0.0f, 1.0f);

                // Reed synthesis
                float reedOut = voice.reed.process(freq, reedNow);

                // Preamp with drive
                float preampOut = voice.preamp.process(reedOut, voiceDrive);

                // Wurlitzer signature tremolo
                voice.tremoloLFO.setRate(tremRateN, srf);
                float tremVal = voice.tremoloLFO.process();
                float tremGain = 1.0f - tremDepthN * 0.5f * (1.0f + tremVal);

                // Migration — adds subtle spectral content from coupled engines
                if (migrationN > 0.01f)
                {
                    float migColor = fastTanh(preampOut * (1.0f + migrationN * 2.0f)) * migrationN * 0.2f;
                    preampOut += migColor;
                }

                // Amplitude envelope
                float ampLevel = voice.ampEnv.process();
                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // Filter — Wurli has a warmer, lower cutoff ceiling than Rhodes
                float fEnvMod = voice.filterEnv.process() * pFilterEnvAmt * 3000.0f;
                float velBright = voice.velocity * 2500.0f;
                float cutoff = std::clamp(brightNow + fEnvMod + velBright, 200.0f, 16000.0f);
                voice.svf.setMode(CytomicSVF::Mode::LowPass);
                voice.svf.setCoefficients(cutoff, 0.2f, srf);
                float filtered = voice.svf.processSample(preampOut);

                float output = filtered * ampLevel * tremGain;

                // Stereo tremolo pan (Wurli 200A stereo vibrato effect)
                float stereoPan = tremVal * 0.3f;
                mixL += output * (voice.panL + stereoPan);
                mixR += output * (voice.panR - stereoPan);
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
        float reedStiffness = paramReed ? paramReed->load() : 0.5f;

        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.snapTo(freq);

        v.reed.prepare(srf);
        v.reed.trigger(vel, reedStiffness);
        // Per-note warble phase randomization — prevents all voices warbling in unison.
        // Each note-on gets a unique phase offset derived from note+voice index.
        {
            uint32_t seed = static_cast<uint32_t>(note * 5003u + idx * 7919u + voiceCounter * 1013u);
            seed ^= seed >> 13;
            seed ^= seed << 17;
            seed ^= seed >> 5;
            v.reed.warblePhase = static_cast<float>(seed & 0xFFFF) / 65536.0f;
        }
        v.preamp.reset();

        // Amp envelope
        float attack = paramAttack ? paramAttack->load() : 0.005f;
        float decay = paramDecay ? paramDecay->load() : 0.6f;
        float sustain = paramSustain ? paramSustain->load() : 0.5f;
        float release = paramRelease ? paramRelease->load() : 0.4f;
        v.ampEnv.prepare(srf);
        v.ampEnv.setADSR(attack, decay, sustain, release);
        v.ampEnv.triggerHard();

        // Filter envelope — Wurli has a faster, more percussive filter sweep
        v.filterEnv.prepare(srf);
        v.filterEnv.setADSR(0.001f, 0.2f + (1.0f - vel) * 0.3f, 0.0f, 0.2f);
        v.filterEnv.triggerHard();

        v.svf.reset();

        // Stereo placement
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
                v.filterEnv.release();
            }
        }
    }

    //==========================================================================
    // Parameters — 26 total
    //==========================================================================
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // Core tone
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_reed", 1}, "Oddfellow Reed Stiffness",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_drive", 1}, "Oddfellow Drive",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_brightness", 1}, "Oddfellow Brightness",
                                              juce::NormalisableRange<float>(200.0f, 16000.0f, 0.0f, 0.3f), 4000.0f));

        // Tremolo (signature Wurlitzer feature)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_tremRate", 1}, "Oddfellow Tremolo Rate",
                                              juce::NormalisableRange<float>(0.5f, 12.0f, 0.01f), 5.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_tremDepth", 1}, "Oddfellow Tremolo Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));

        // Amp envelope
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_attack", 1}, "Oddfellow Attack",
                                              juce::NormalisableRange<float>(0.001f, 0.5f, 0.0f, 0.3f), 0.005f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_decay", 1}, "Oddfellow Decay",
                                              juce::NormalisableRange<float>(0.05f, 5.0f, 0.0f, 0.4f), 0.6f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_sustain", 1}, "Oddfellow Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_release", 1}, "Oddfellow Release",
                                              juce::NormalisableRange<float>(0.01f, 5.0f, 0.0f, 0.4f), 0.4f));

        // Filter
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_filterEnvAmt", 1}, "Oddfellow Filter Env Amount",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // FUSION
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_migration", 1}, "Oddfellow Migration",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // Pitch
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_bendRange", 1}, "Oddfellow Pitch Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));

        // Macros
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_macroCharacter", 1}, "Oddfellow Macro CHARACTER",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_macroMovement", 1}, "Oddfellow Macro MOVEMENT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_macroCoupling", 1}, "Oddfellow Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_macroSpace", 1}, "Oddfellow Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // LFOs (D002/D005)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_lfo1Rate", 1}, "Oddfellow LFO1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_lfo1Depth", 1}, "Oddfellow LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oddf_lfo1Shape", 1}, "Oddfellow LFO1 Shape", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_lfo2Rate", 1}, "Oddfellow LFO2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oddf_lfo2Depth", 1}, "Oddfellow LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oddf_lfo2Shape", 1}, "Oddfellow LFO2 Shape", 0, 4, 0));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramReed = apvts.getRawParameterValue("oddf_reed");
        paramDrive = apvts.getRawParameterValue("oddf_drive");
        paramBrightness = apvts.getRawParameterValue("oddf_brightness");
        paramTremRate = apvts.getRawParameterValue("oddf_tremRate");
        paramTremDepth = apvts.getRawParameterValue("oddf_tremDepth");
        paramAttack = apvts.getRawParameterValue("oddf_attack");
        paramDecay = apvts.getRawParameterValue("oddf_decay");
        paramSustain = apvts.getRawParameterValue("oddf_sustain");
        paramRelease = apvts.getRawParameterValue("oddf_release");
        paramFilterEnvAmt = apvts.getRawParameterValue("oddf_filterEnvAmt");
        paramMigration = apvts.getRawParameterValue("oddf_migration");
        paramBendRange = apvts.getRawParameterValue("oddf_bendRange");
        paramMacroCharacter = apvts.getRawParameterValue("oddf_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("oddf_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("oddf_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("oddf_macroSpace");
        paramLfo1Rate = apvts.getRawParameterValue("oddf_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("oddf_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("oddf_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("oddf_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("oddf_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("oddf_lfo2Shape");
    }

private:
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float srf = 0.0f;  // Sentinel: must be set by prepare() before use

    std::array<OddfellowVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount{0};

    ParameterSmoother smoothReed, smoothDrive, smoothBrightness;
    ParameterSmoother smoothTremRate, smoothTremDepth, smoothMigration;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f, couplingReedMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    std::atomic<float>* paramReed = nullptr;
    std::atomic<float>* paramDrive = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramTremRate = nullptr;
    std::atomic<float>* paramTremDepth = nullptr;
    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramSustain = nullptr;
    std::atomic<float>* paramRelease = nullptr;
    std::atomic<float>* paramFilterEnvAmt = nullptr;
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
