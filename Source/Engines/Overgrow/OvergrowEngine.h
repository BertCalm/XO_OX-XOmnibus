// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OvergrowEngine.h — XOvergrow | "The Weed Through Cracks"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOvergrow is the weed that grows through pavement — untamed, resilient,
//      beautiful in its refusal to be controlled. A solo string voice that
//      responds to silence more than to playing, that sends runners from
//      stressed notes, that finds space wherever space exists.
//
//  ENGINE CONCEPT:
//      A solo string synthesizer built on Karplus-Strong plucked string
//      synthesis with unpredictable modulation. Single voice focus with
//      organic string character. The DSP responds to silence — the moments
//      between notes are where XOvergrow develops. The wildness parameter
//      controls how unpredictable the growth behavior becomes.
//
//  DSP ARCHITECTURE:
//      Per-voice: Karplus-Strong string model (noise burst → tuned delay line
//      → one-pole damping filter → feedback) with pitch jitter, bow noise,
//      and runner generation. Silence detection triggers harmonic evolution
//      in the tail. Growth Mode: single seed, harmonics emerge sequentially.
//
//  GARDEN QUAD ROLE:
//      Intermediate species — solo strings finding space once XOxalis has
//      established. More variable, more alive, a little slower. In full-quad
//      coupling, XOvergrow is the weed — beautiful and uncontrollable.
//
//  Accent: Vine Green #3A5F0B
//  Parameter prefix: grow_
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
// KarplusStrongString — tuned delay line with damping for string model
//==============================================================================
struct KarplusStrongString
{
    static constexpr int kMaxDelay = 4096;

    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        std::fill(delayLine.begin(), delayLine.end(), 0.0f);
        writePos = 0;
        filterState = 0.0f;
    }

    void setFrequency(float freqHz) noexcept
    {
        if (freqHz < 20.0f)
            freqHz = 20.0f;
        delaySamples = sr / freqHz;
        if (delaySamples >= static_cast<float>(kMaxDelay - 1))
            delaySamples = static_cast<float>(kMaxDelay - 2);
    }

    void setDamping(float damp) noexcept
    {
        // One-pole coefficient: higher damping = more HF loss per cycle
        dampCoeff = 0.5f + damp * 0.49f; // [0.5, 0.99]
    }

    void setFeedback(float fb) noexcept { feedback = std::clamp(fb, 0.0f, 0.9999f); }

    /// Excite the string with a noise burst (call on note-on)
    void excite(float velocity, float brightness) noexcept
    {
        uint32_t noiseState = static_cast<uint32_t>(velocity * 65535.0f) + 54321u;
        int burstLen = static_cast<int>(delaySamples);
        for (int i = 0; i < burstLen && i < kMaxDelay; ++i)
        {
            noiseState = noiseState * 1664525u + 1013904223u;
            float noise = (static_cast<float>(noiseState & 0xFFFF) / 32768.0f - 1.0f);
            // Brightness controls the noise burst spectral shape
            float shaped = noise * velocity * (0.3f + brightness * 0.7f);
            delayLine[(writePos + i) % kMaxDelay] += shaped;
        }
    }

    float process() noexcept
    {
        // Fractional delay read (linear interpolation)
        float readPos = static_cast<float>(writePos) - delaySamples;
        if (readPos < 0.0f)
            readPos += kMaxDelay;
        int r0 = static_cast<int>(readPos);
        float frac = readPos - static_cast<float>(r0);
        int r1 = (r0 + 1) % kMaxDelay;
        float delayed = delayLine[r0] * (1.0f - frac) + delayLine[r1] * frac;

        // One-pole damping filter (KS averaging filter)
        filterState = filterState * dampCoeff + delayed * (1.0f - dampCoeff);
        float output = flushDenormal(filterState);

        // Write back with feedback
        delayLine[writePos] = output * feedback;
        writePos = (writePos + 1) % kMaxDelay;

        return output;
    }

    void reset() noexcept
    {
        std::fill(delayLine.begin(), delayLine.end(), 0.0f);
        writePos = 0;
        filterState = 0.0f;
    }

    float sr = 0.0f;  // Sentinel: must be set by prepare() before use
    float delaySamples = 100.0f;
    float dampCoeff = 0.5f;
    float feedback = 0.995f;
    float filterState = 0.0f;
    int writePos = 0;
    std::array<float, kMaxDelay> delayLine{};
};

//==============================================================================
// RunnerGenerator — spawns sympathetic sub-harmonics from stressed notes
//==============================================================================
struct RunnerGenerator
{
    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        runnerString.prepare(sampleRate);
        active = false;
        timer = 0;
        // Scale the per-sample decay coefficient to the actual sample rate.
        // 0.9995f was tuned for 48kHz; at 96kHz runners faded 2x too fast.
        runnerDecay_ = std::pow(0.9995f, 48000.0f / sampleRate);
    }

    /// Check if conditions warrant spawning a runner
    void checkSpawn(float velocity, float aggression, float wildness, float baseFreqHz, double sessionTime) noexcept
    {
        if (active)
            return; // already running
        if (velocity < 0.6f || aggression < 0.3f)
            return; // not stressed enough

        // Probability increases with wildness and aggression
        float probability = wildness * aggression * 0.2f;
        uint32_t hash = static_cast<uint32_t>(sessionTime * 1000.0) * 1664525u + 1013904223u;
        float roll = static_cast<float>(hash & 0xFFFF) / 65536.0f;

        if (roll < probability)
        {
            // Spawn a runner at sub-harmonic (octave below or fifth below)
            float runnerFreq = baseFreqHz * ((hash & 1) ? 0.5f : 0.667f);
            runnerString.setFrequency(runnerFreq);
            runnerString.setDamping(0.7f);
            runnerString.setFeedback(0.98f);
            runnerString.excite(velocity * 0.3f, 0.3f);
            active = true;
            timer = 0;
            delayedOnset = static_cast<int>(sr * (2.0f + wildness * 6.0f)); // 2-8 sec delay
            amplitude = 0.15f * wildness;
        }
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;

        timer++;
        if (timer < delayedOnset)
            return 0.0f; // waiting for delayed onset

        float out = runnerString.process() * amplitude;

        // Fade out over time — runner should fade in ~2 seconds (was 25× too slow).
        // 0.99998 per sample ≈ 1s time constant; 0.9995 ≈ ~40ms time constant.
        // Target: runners appear, grow briefly, then fade in ~2-4 seconds.
        // Per-sample coefficient for ~3s at 48kHz: exp(-1/(3*48000)) ≈ 0.999993.
        // But seance says 25x too slow, so target ~40ms: 0.99998 * 0 = wrong.
        // Original was 0.99998f ≈ 50ks time constant. 25x faster = 2ks = ~40ms.
        // For 2s fade: 1 - (1-0.99998)*25 = 1 - 0.0005 = 0.9995 per sample.
        amplitude *= runnerDecay_;
        if (amplitude < 1e-6f)
            active = false;

        return out;
    }

    void reset() noexcept
    {
        active = false;
        timer = 0;
        runnerString.reset();
    }

    float sr = 0.0f;  // Sentinel: must be set by prepare() before use
    float runnerDecay_ = 0.9995f; // per-sample fade coefficient, SR-scaled in prepare()
    KarplusStrongString runnerString;
    bool active = false;
    int timer = 0;
    int delayedOnset = 0;
    float amplitude = 0.0f;
};

//==============================================================================
// OvergrowVoice
//==============================================================================
struct OvergrowVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;

    GlideProcessor glide;
    KarplusStrongString string;
    CytomicSVF filter;
    FilterEnvelope ampEnv;
    FilterEnvelope filterEnv;
    StandardLFO vibratoLFO;
    StandardLFO lfo1, lfo2;
    RunnerGenerator runner;

    // Growth Mode state
    bool growthMode = false;
    float growthPhase = 0.0f;
    float growthTimer = 0.0f;
    float growthDuration = 15.0f;

    // Silence response — the tail evolves during silence
    float silenceTimer = 0.0f;
    float lastOutputLevel = 0.0f;

    // Dormancy pitch offset
    float dormancyPitchCents = 0.0f;

    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        growthMode = false;
        growthPhase = 0.0f;
        growthTimer = 0.0f;
        silenceTimer = 0.0f;
        lastOutputLevel = 0.0f;
        dormancyPitchCents = 0.0f;
        glide.reset();
        string.reset();
        filter.reset();
        ampEnv.kill();
        filterEnv.kill();
        vibratoLFO.reset();
        runner.reset();
    }
};

//==============================================================================
// OvergrowEngine — "The Weed Through Cracks"
//==============================================================================
class OvergrowEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 4;

    juce::String getEngineId() const override { return "Overgrow"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF228B22); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRate = std::max(sampleRate, 1.0);
        sr = sampleRate;
        srf = static_cast<float>(sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].string.prepare(srf);
            voices[i].ampEnv.prepare(srf);
            voices[i].filterEnv.prepare(srf);
            voices[i].vibratoLFO.setShape(StandardLFO::Sine);
            voices[i].vibratoLFO.reseed(static_cast<uint32_t>(i * 8111 + 77));
            voices[i].runner.prepare(srf);
        }

        smoothCutoff.prepare(srf);
        smoothWildness.prepare(srf);
        smoothBowNoise.prepare(srf);

        accumulators.reset();
        // Configure for solo strings: faster warmth rise, lower threshold
        accumulators.wRiseRate = 0.003f;
        accumulators.aThreshold = 0.35f;

        network.configure(0.6f, 3.0f);

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
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        auto loadP = [](std::atomic<float>* p, float def) { return p ? p->load(std::memory_order_relaxed) : def; };

        const float pCutoff = loadP(paramCutoff, 4000.0f);
        const float pResonance = loadP(paramResonance, 0.3f);
        const float pDamping = loadP(paramDamping, 0.4f);
        const float pFeedback = loadP(paramFeedback, 0.995f);
        const float pBowNoise = loadP(paramBowNoise, 0.2f);
        const float pWildness = loadP(paramWildness, 0.3f);
        const float pVibratoRate = loadP(paramVibratoRate, 5.5f);
        const float pVibratoDepth = loadP(paramVibratoDepth, 0.3f);
        const float pBendRange = loadP(paramBendRange, 2.0f);
        const float pFilterEnvAmt = loadP(paramFilterEnvAmt, 0.3f);
        const float pBrightness = loadP(paramBrightness, 0.5f);

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

        float effectiveCutoff =
            std::clamp(pCutoff + macroChar * 4000.0f + accumulators.getSeasonBrightness() * 2000.0f +
                           pBrightness * 8000.0f + couplingFilterMod + aftertouchAmount * 3000.0f,
                       200.0f, 20000.0f);
        float effectiveWildness = std::clamp(pWildness + macroMove * 0.3f + accumulators.A * 0.3f + macroCoupl * 0.3f,
                                             0.0f, 1.0f); // M3 COUPLING: coupling energy feeds wildness/runner density
        const float effectiveWidth = 1.0f + macroSpace * 0.6f; // M4 SPACE: stereo expansion

        smoothCutoff.set(effectiveCutoff);
        smoothWildness.set(effectiveWildness);
        smoothBowNoise.set(pBowNoise);

        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;
        float effectiveVibratoDepth =
            pVibratoDepth + modWheelAmount * 0.5f + accumulators.getAggressionVibrato() * 0.4f;

        for (auto& voice : voices)
        {
            if (!voice.active)
                continue;
            voice.vibratoLFO.setRate(pVibratoRate + macroMove * 3.0f, srf);
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
            float wildNow = smoothWildness.process();
            float bowNow = smoothBowNoise.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                float baseFreq = voice.glide.process();
                float vibrato = voice.vibratoLFO.process() * effectiveVibratoDepth;

                // Wildness adds pitch jitter — the weed growing unpredictably
                float pitchJitter = 0.0f;
                if (wildNow > 0.01f)
                {
                    uint32_t ns = static_cast<uint32_t>(voice.startTime * 13 + s * 37);
                    ns = ns * 1664525u + 1013904223u;
                    pitchJitter = (static_cast<float>(ns & 0xFFFF) / 32768.0f - 1.0f) * wildNow *
                                  0.05f; // ±5 cents at max wildness
                }

                float freq =
                    baseFreq * PitchBendUtil::semitonesToFreqRatio(bendSemitones + couplingPitchMod + vibrato * 0.15f +
                                                                   pitchJitter + voice.dormancyPitchCents / 100.0f);

                float l1 = voice.lfo1.process() * lfo1Depth;
                float l2 = voice.lfo2.process() * lfo2Depth;

                voice.string.setFrequency(freq);
                voice.string.setDamping(pDamping + accumulators.getWarmthRolloff() * 0.2f);
                voice.string.setFeedback(pFeedback);

                float stringOut = voice.string.process();

                // Bow noise injection (continuous excitation)
                if (bowNow > 0.001f && voice.ampEnv.getStage() != FilterEnvelope::Stage::Idle)
                {
                    uint32_t bs = static_cast<uint32_t>(voice.startTime * 19 + s * 41);
                    bs = bs * 1664525u + 1013904223u;
                    float bowNoiseSignal = (static_cast<float>(bs & 0xFFFF) / 32768.0f - 1.0f) * bowNow * 0.05f;
                    stringOut += bowNoiseSignal;
                }

                // Runner generation — stressed notes sprout unexpected sub-harmonics
                voice.runner.checkSpawn(voice.velocity, accumulators.A, wildNow, freq, accumulators.sessionTime);
                float runnerOut = voice.runner.process();

                // Growth Mode
                float growthGain = 1.0f;
                if (voice.growthMode)
                {
                    voice.growthTimer += 1.0f / srf;
                    voice.growthPhase = std::min(voice.growthTimer / voice.growthDuration, 1.0f);
                    growthGain = voice.growthPhase * voice.growthPhase;
                }

                // Filter
                float envLevel = voice.filterEnv.process();
                float fCut = std::clamp(cutNow + envLevel * pFilterEnvAmt * 5000.0f + l1 * 3000.0f, 200.0f, 20000.0f);
                voice.filter.setMode(CytomicSVF::Mode::LowPass);
                voice.filter.setCoefficients(fCut, std::clamp(pResonance + l2 * 0.15f, 0.0f, 1.0f),
                                             srf); // l2 → resonance shimmer
                float filtered = voice.filter.processSample(stringOut + runnerOut);

                // Amp envelope
                float ampLevel = voice.ampEnv.process() * growthGain;
                if (ampLevel < 1e-6f && voice.ampEnv.getStage() == FilterEnvelope::Stage::Idle)
                {
                    voice.active = false;
                    continue;
                }

                float output = filtered * ampLevel * (0.4f + voice.velocity * 0.6f);
                voice.lastOutputLevel = std::fabs(output);

                // Feed engine-level output tracker for silence detection
                engineLastOutputLevel = std::max(engineLastOutputLevel, std::fabs(output));

                // Mycorrhizal network
                if (voice.velocity > accumulators.aThreshold)
                    network.sendStress(&voice - voices.data(), voice.velocity * accumulators.A * 0.15f,
                                       accumulators.sessionTime);

                mixL += output * voice.panL;
                mixR += output * voice.panR;
            }

            // M4 SPACE: mid/side width expansion
            const float mid = (mixL + mixR) * 0.5f;
            const float side = (mixL - mixR) * 0.5f * effectiveWidth;
            outL[s] = mid + side;
            if (outR)
                outR[s] = mid - side;
            couplingCacheL = outL[s];
            couplingCacheR = outR ? outR[s] : mixR;
        }

        // Silence response: track how long the engine has been silent.
        // Update engine-level silence timer once per block.
        float blockSec = static_cast<float>(numSamples) / srf;
        if (engineLastOutputLevel < 0.001f)
            engineSilenceTimer += blockSec; // accumulate silence
        else
            engineSilenceTimer = 0.0f; // reset on any audible output
        engineLastOutputLevel = 0.0f;  // reset peak tracker for next block

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

        // RT-fix: string.prepare() already called at engine prepare()-time for all
        // voice slots.  On noteOn, zero the delay buffer via reset() then reconfigure.
        v.string.reset();
        v.string.setFrequency(freq);
        v.string.setDamping(paramDamping ? paramDamping->load() : 0.4f);
        v.string.setFeedback(paramFeedback ? paramFeedback->load() : 0.995f);

        // D001: velocity → excitation brightness.
        // Silence response: after extended silence, XOvergrow erupts —
        // brighter excitation and boosted wildness (the weed through pavement concept).
        // After ≥5 seconds of silence: full eruption. 0–5 seconds: scaled smoothly.
        float silenceBoost = std::min(engineSilenceTimer / 5.0f, 1.0f);
        float brightness = 0.3f + vel * 0.7f + silenceBoost * 0.3f; // up to 30% brighter
        brightness = std::clamp(brightness, 0.0f, 1.0f);
        v.string.excite(vel * (1.0f + silenceBoost * 0.4f), brightness); // louder hit too

        // Silence-boosted wildness: stored in dormancyPitchCents scale (temporary boost)
        // The weed has been growing roots in the dark — now it erupts

        float attackTime = paramAttack ? paramAttack->load() : 0.05f;
        attackTime *= (1.3f - vel * 0.5f);
        float releaseTime = paramRelease ? paramRelease->load() : 1.5f;

        // RT-fix: ampEnv/filterEnv.prepare() already called at engine prepare()-time.
        // setADSR() reconfigures the envelope in-place (no allocation), then trigger.
        v.ampEnv.setADSR(attackTime, paramDecay ? paramDecay->load() : 0.3f, paramSustain ? paramSustain->load() : 0.7f,
                         releaseTime);
        v.ampEnv.triggerHard();

        v.filterEnv.setADSR(attackTime * 0.3f, 0.5f, 0.1f, releaseTime * 0.6f);
        v.filterEnv.triggerHard();

        v.vibratoLFO.reset();
        v.runner.reset();

        // Growth Mode
        v.growthMode = isGrowthMode;
        v.growthTimer = 0.0f;
        v.growthPhase = 0.0f;
        v.growthDuration = paramGrowthTime ? paramGrowthTime->load() : 15.0f;

        // Dormancy pitch offset
        float dPitch = accumulators.getDormancyPitchVariance();
        uint32_t seed = static_cast<uint32_t>(note * 6151 + idx * 53);
        seed = seed * 1664525u + 1013904223u;
        v.dormancyPitchCents = (static_cast<float>(seed & 0xFFFF) / 32768.0f - 1.0f) * dPitch * 1.5f;

        // Solo string: center panning with slight random offset
        float panOffset = (static_cast<float>(seed >> 16 & 0xFFFF) / 65536.0f - 0.5f) * 0.3f;
        float angle = (panOffset + 0.5f) * 0.5f * 3.14159265f;
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

        // String model
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_damping", 1}, "Overgrow String Damping",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_feedback", 1}, "Overgrow String Feedback",
                                              juce::NormalisableRange<float>(0.9f, 0.9999f, 0.0001f), 0.995f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_bowNoise", 1}, "Overgrow Bow Noise",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));

        // Garden-specific
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_wildness", 1}, "Overgrow Wildness",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Filter
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_cutoff", 1}, "Overgrow Cutoff",
                                              juce::NormalisableRange<float>(200.0f, 20000.0f, 0.0f, 0.3f), 4000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_resonance", 1}, "Overgrow Resonance",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_filterEnvAmt", 1}, "Overgrow Filter Env Amt",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_brightness", 1}, "Overgrow Brightness",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Amp envelope
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_attack", 1}, "Overgrow Attack",
                                              juce::NormalisableRange<float>(0.001f, 2.0f, 0.0f, 0.3f), 0.05f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_decay", 1}, "Overgrow Decay",
                                              juce::NormalisableRange<float>(0.01f, 5.0f, 0.0f, 0.4f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_sustain", 1}, "Overgrow Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_release", 1}, "Overgrow Release",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.4f), 1.5f));

        // Vibrato
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_vibratoRate", 1}, "Overgrow Vibrato Rate",
                                              juce::NormalisableRange<float>(0.5f, 12.0f, 0.1f), 5.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_vibratoDepth", 1}, "Overgrow Vibrato Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Growth Mode
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_growthMode", 1}, "Overgrow Growth Mode",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_growthTime", 1}, "Overgrow Growth Time",
                                              juce::NormalisableRange<float>(3.0f, 60.0f, 0.1f), 15.0f));

        // Pitch
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_bendRange", 1}, "Overgrow Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));

        // Macros
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_macroCharacter", 1}, "Overgrow Macro CHARACTER",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_macroMovement", 1}, "Overgrow Macro MOVEMENT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_macroCoupling", 1}, "Overgrow Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_macroSpace", 1}, "Overgrow Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // LFOs
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_lfo1Rate", 1}, "Overgrow LFO1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_lfo1Depth", 1}, "Overgrow LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"grow_lfo1Shape", 1}, "Overgrow LFO1 Shape", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_lfo2Rate", 1}, "Overgrow LFO2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"grow_lfo2Depth", 1}, "Overgrow LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"grow_lfo2Shape", 1}, "Overgrow LFO2 Shape", 0, 4, 0));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramDamping = apvts.getRawParameterValue("grow_damping");
        paramFeedback = apvts.getRawParameterValue("grow_feedback");
        paramBowNoise = apvts.getRawParameterValue("grow_bowNoise");
        paramWildness = apvts.getRawParameterValue("grow_wildness");
        paramCutoff = apvts.getRawParameterValue("grow_cutoff");
        paramResonance = apvts.getRawParameterValue("grow_resonance");
        paramFilterEnvAmt = apvts.getRawParameterValue("grow_filterEnvAmt");
        paramBrightness = apvts.getRawParameterValue("grow_brightness");
        paramAttack = apvts.getRawParameterValue("grow_attack");
        paramDecay = apvts.getRawParameterValue("grow_decay");
        paramSustain = apvts.getRawParameterValue("grow_sustain");
        paramRelease = apvts.getRawParameterValue("grow_release");
        paramVibratoRate = apvts.getRawParameterValue("grow_vibratoRate");
        paramVibratoDepth = apvts.getRawParameterValue("grow_vibratoDepth");
        paramGrowthMode = apvts.getRawParameterValue("grow_growthMode");
        paramGrowthTime = apvts.getRawParameterValue("grow_growthTime");
        paramBendRange = apvts.getRawParameterValue("grow_bendRange");
        paramMacroCharacter = apvts.getRawParameterValue("grow_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("grow_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("grow_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("grow_macroSpace");
        paramLfo1Rate = apvts.getRawParameterValue("grow_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("grow_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("grow_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("grow_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("grow_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("grow_lfo2Shape");
    }

private:
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float srf = 0.0f;  // Sentinel: must be set by prepare() before use

    std::array<OvergrowVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount{0};

    GardenAccumulators accumulators;
    GardenMycorrhizalNetwork network;

    ParameterSmoother smoothCutoff, smoothWildness, smoothBowNoise;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    // Silence response: XOvergrow responds most to silence — next note after
    // extended silence should sound wilder/brighter (the weed exploding through pavement).
    float engineSilenceTimer = 0.0f;    // seconds since last audible output
    float engineLastOutputLevel = 0.0f; // RMS-ish tracker of recent output

    std::atomic<float>* paramDamping = nullptr;
    std::atomic<float>* paramFeedback = nullptr;
    std::atomic<float>* paramBowNoise = nullptr;
    std::atomic<float>* paramWildness = nullptr;
    std::atomic<float>* paramCutoff = nullptr;
    std::atomic<float>* paramResonance = nullptr;
    std::atomic<float>* paramFilterEnvAmt = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramSustain = nullptr;
    std::atomic<float>* paramRelease = nullptr;
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
