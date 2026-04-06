// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OrchardEngine.h — XOrchard | "The Cultivated Grove"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOrchard is the managed orchard — rows of trees planted in deliberate
//      arrangement, tended through seasons, pruned and cultivated for decades.
//      Under the canopy, root systems interweave. Above, branches reach toward
//      seasonal light. The sound of a full orchestral string section, managed
//      by a concertmaster, warmed by a hall over the course of a performance.
//
//  ENGINE CONCEPT:
//      A full orchestral string synthesizer built on detuned sawtooth ensemble
//      oscillators (4 per voice) shaped through formant-resonant filters. The
//      Concertmaster mechanism: the highest voice leads, others follow. Slow
//      attack, lush sustain. Seasonal tonal character shifts from bright
//      (spring) through full (summer) to warm (fall) to sparse (winter).
//      Evolutionary accumulators (W/A/D) accumulate over session time.
//
//  DSP ARCHITECTURE:
//      Per-voice: 4 detuned PolyBLEP sawtooth oscillators → formant-shaped
//      CytomicSVF filter → amp envelope. Ensemble spread via inter-oscillator
//      detuning (±cents). Formant shape adds orchestral body resonance.
//      Growth Mode: note-on = seed, harmonics germinate over 5-30 seconds.
//
//  GARDEN QUAD ROLE:
//      Climax species — orchestral strings that require established conditions
//      to bloom fully. Slow to arrive, most resource-demanding, most stable
//      once established. In full-quad coupling, XOrchard is the last to
//      achieve peak expression. The concertmaster mechanism.
//
//  Accent: Orchard Blossom #FFB7C5
//  Parameter prefix: orch_
//  Voices: 4 (Decision G2: CPU budget)
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
#include "../../DSP/GardenAccumulators.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// PolyBLEP sawtooth oscillator — anti-aliased via polynomial bandlimited step
//==============================================================================
struct OrchardSaw
{
    void setFrequency(float freqHz, float sampleRate) noexcept
    {
        phaseInc = freqHz / sampleRate;
        if (phaseInc > 0.49f)
            phaseInc = 0.49f;
    }

    float process() noexcept
    {
        float naive = 2.0f * phase - 1.0f;

        // PolyBLEP correction at discontinuity
        float t = phase;
        float blep = 0.0f;
        if (t < phaseInc)
        {
            t /= phaseInc;
            blep = t + t - t * t - 1.0f;
        }
        else if (t > 1.0f - phaseInc)
        {
            t = (t - 1.0f) / phaseInc;
            blep = t * t + t + t + 1.0f;
        }

        phase += phaseInc;
        if (phase >= 1.0f)
            phase -= 1.0f;

        return naive - blep;
    }

    void reset() noexcept { phase = 0.0f; }
    void reset(float startPhase) noexcept { phase = startPhase; }

    float phase = 0.0f;
    float phaseInc = 0.01f;
};

//==============================================================================
// OrchardVoice — 4 detuned saws + formant filter + amp/filter envelopes
//==============================================================================
struct OrchardVoice
{
    static constexpr int kNumOscs = 4;

    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;

    GlideProcessor glide;
    std::array<OrchardSaw, kNumOscs> oscs;
    CytomicSVF filter;
    CytomicSVF formantFilter; // body resonance shaping
    FilterEnvelope ampEnv;
    FilterEnvelope filterEnv;
    StandardLFO vibratoLFO;
    StandardLFO lfo1, lfo2;

    // Growth Mode state
    bool growthMode = false;
    float growthPhase = 0.0f;     // 0-1 through germination phases
    float growthTimer = 0.0f;     // seconds since seed planted
    float growthDuration = 20.0f; // configurable germination time

    // Detuning offsets in cents for the 4 oscillators
    std::array<float, kNumOscs> detuneOffsets = {-7.0f, -3.0f, 3.0f, 7.0f};

    // Dormancy pitch offset (settled at note-on from D accumulator)
    float dormancyPitchCents = 0.0f;

    // Pan position
    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        growthMode = false;
        growthPhase = 0.0f;
        growthTimer = 0.0f;
        dormancyPitchCents = 0.0f;
        glide.reset();
        for (auto& o : oscs)
            o.reset();
        filter.reset();
        formantFilter.reset();
        ampEnv.kill();
        filterEnv.kill();
        vibratoLFO.reset();
    }
};

//==============================================================================
// OrchardEngine — "The Cultivated Grove"
//==============================================================================
class OrchardEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 4;

    juce::String getEngineId() const override { return "Orchard"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFFFB7C5); }
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
            voices[i].vibratoLFO.reseed(static_cast<uint32_t>(i * 7919 + 101));
            // Stagger oscillator phases for ensemble width
            for (int o = 0; o < OrchardVoice::kNumOscs; ++o)
                voices[i].oscs[o].reset(static_cast<float>(o) * 0.25f);
        }

        smoothCutoff.prepare(srf);
        smoothDetune.prepare(srf);
        smoothFormant.prepare(srf);
        smoothAttack.prepare(srf, 0.02f); // slower smoothing for attack

        accumulators.reset();
        network.configure(0.5f, 4.0f);

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
            couplingFormantMod += val;
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

        const float pCutoff = loadP(paramCutoff, 6000.0f);
        const float pResonance = loadP(paramResonance, 0.2f);
        const float pDetune = loadP(paramDetune, 7.0f);
        const float pFormant = loadP(paramFormant, 0.5f);
        const float pVibratoRate = loadP(paramVibratoRate, 5.0f);
        const float pVibratoDepth = loadP(paramVibratoDepth, 0.2f);
        const float pSeason = loadP(paramSeason, -1.0f); // -1 = auto
        const float pBendRange = loadP(paramBendRange, 2.0f);
        const float pEnsembleWid = loadP(paramEnsembleWidth, 0.7f);
        const float pFilterEnvAmt = loadP(paramFilterEnvAmt, 0.4f);
        const float pBrightness = loadP(paramBrightness, 0.5f);
        const float pWarmth = loadP(paramWarmthCtrl, 0.5f);

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

        //-- Accumulator update (block rate) --
        float avgVel = 0.0f;
        int activeCount = 0;
        for (const auto& v : voices)
        {
            if (v.active)
            {
                avgVel += v.velocity;
                activeCount++;
            }
        }
        if (activeCount > 0)
            avgVel /= static_cast<float>(activeCount);

        float blockSizeSec = static_cast<float>(numSamples) / srf;
        accumulators.update(blockSizeSec, activeCount, avgVel, noteOnCount);

        // Override season if user has manual control
        if (pSeason >= 0.0f)
            accumulators.season = static_cast<Season>(std::clamp(static_cast<int>(pSeason), 0, 3));

        //-- Accumulator-derived modifiers --
        float warmthRolloff = accumulators.getWarmthRolloff() * pWarmth;
        float aggrHarsh = accumulators.getAggressionHarshness();
        float seasonBrightness = accumulators.getSeasonBrightness();

        //-- Effective parameters with macros + accumulators --
        float effectiveCutoff = std::clamp(pCutoff + macroChar * 4000.0f - warmthRolloff * 4000.0f +
                                               seasonBrightness * 2000.0f + pBrightness * 6000.0f + couplingFilterMod,
                                           200.0f, 20000.0f);
        float effectiveDetune = pDetune + macroMove * 10.0f + aggrHarsh * 5.0f;
        float effectiveFormant = std::clamp(pFormant + macroChar * 0.3f + couplingFormantMod + macroCoupl * 0.25f, 0.0f,
                                            1.0f);                     // M3 COUPLING: deeper formant resonance
        const float effectiveWidth = pEnsembleWid + macroSpace * 0.6f; // M4 SPACE: ensemble width + macro expansion

        smoothCutoff.set(effectiveCutoff);
        smoothDetune.set(effectiveDetune);
        smoothFormant.set(effectiveFormant);

        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingFormantMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;

        // D006: mod wheel → vibrato depth (expression during sustain)
        float effectiveVibratoDepth =
            pVibratoDepth + modWheelAmount * 0.4f + accumulators.getAggressionVibrato() * 0.3f;

        //-- Concertmaster mechanism (block-rate): highest active voice leads.
        // Voice 0 is the seat of the concertmaster — it sets the reference vibrato phase
        // and exerts a subtle pitch pull on the other voices (≤ ±5 cents) to keep the
        // section in tune. Lower voices follow the highest note's tuning.
        {
            // Find the highest active note (concertmaster candidate)
            int concertmasterIdx = -1;
            int highestNote = -1;
            for (int vi = 0; vi < kMaxVoices; ++vi)
            {
                if (voices[vi].active && voices[vi].currentNote > highestNote)
                {
                    highestNote = voices[vi].currentNote;
                    concertmasterIdx = vi;
                }
            }

            if (concertmasterIdx >= 0)
            {
                float concertmasterFreq = voices[concertmasterIdx].glide.getFreq();
                // Other voices receive a small intonation pull toward the just-intonation
                // ratio nearest to their interval from the concertmaster.
                // This is a simplified version: pull of ±2 cents per active voice.
                for (int vi = 0; vi < kMaxVoices; ++vi)
                {
                    if (vi == concertmasterIdx || !voices[vi].active)
                        continue;
                    // Compute the interval in semitones from concertmaster to this voice
                    float intervalSemis = static_cast<float>(voices[vi].currentNote - highestNote);
                    // A perfect fifth (7 semitones) is 2 cents flat in equal temperament vs JI.
                    // Apply a subtle ±3-cent pull toward the section center of gravity.
                    // Uses the voice index distance to avoid clustering.
                    float pullCents =
                        (concertmasterFreq > 0.0f) ? std::clamp(intervalSemis * -0.15f, -3.0f, 3.0f) : 0.0f;
                    // Blend with existing dormancy offset rather than overwrite
                    voices[vi].dormancyPitchCents = voices[vi].dormancyPitchCents * 0.97f + pullCents * 0.03f;
                }
            }
        }

        //-- Per-voice LFO config (once per block) --
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
            float detNow = smoothDetune.process();
            float formNow = smoothFormant.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                float baseFreq = voice.glide.process();
                float vibrato = voice.vibratoLFO.process() * effectiveVibratoDepth;
                float freq =
                    baseFreq * PitchBendUtil::semitonesToFreqRatio(bendSemitones + couplingPitchMod + vibrato * 0.1f +
                                                                   voice.dormancyPitchCents / 100.0f);

                float l1 = voice.lfo1.process() * lfo1Depth;
                float l2 = voice.lfo2.process() * lfo2Depth;

                // Growth Mode: scale harmonic content by growth phase
                float growthGain = 1.0f;
                if (voice.growthMode)
                {
                    voice.growthTimer += 1.0f / srf;
                    voice.growthPhase = std::min(voice.growthTimer / voice.growthDuration, 1.0f);
                    // Phase 1 (0-0.1): near silence. Phase 2 (0.1-0.4): fundamental emerges.
                    // Phase 3 (0.4-0.7): harmonics fill. Phase 4 (0.7-1.0): full bloom.
                    growthGain = voice.growthPhase * voice.growthPhase; // quadratic bloom
                }

                // 4 detuned sawtooth oscillators — the orchestral section
                float oscMix = 0.0f;
                for (int o = 0; o < OrchardVoice::kNumOscs; ++o)
                {
                    float detuneHz = freq * (voice.detuneOffsets[o] + detNow * (o < 2 ? -1.0f : 1.0f)) /
                                     1200.0f; // cents to ratio approximation
                    float oscFreq = freq + detuneHz;
                    voice.oscs[o].setFrequency(oscFreq, srf);

                    // In growth mode, fade in oscillators sequentially
                    float oscGain = 1.0f;
                    if (voice.growthMode)
                    {
                        float oscOnset = static_cast<float>(o) * 0.2f;
                        oscGain = std::clamp((voice.growthPhase - oscOnset) * 3.0f, 0.0f, 1.0f);
                    }

                    oscMix += voice.oscs[o].process() * oscGain;
                }
                oscMix *= 0.25f; // normalize 4 oscillators

                // Dormancy attack noise (cold strings, stiff rosin)
                float dormNoise = 0.0f;
                if (voice.dormancyPitchCents > 0.5f && voice.growthTimer < 0.5f)
                {
                    uint32_t ns = static_cast<uint32_t>(voice.startTime * 17 + s * 31);
                    ns = ns * 1664525u + 1013904223u;
                    dormNoise = (static_cast<float>(ns & 0xFFFF) / 32768.0f - 1.0f) *
                                accumulators.getDormancyAttackNoise() * 0.1f;
                }

                // Formant-shaped filter (orchestral body resonance)
                float formFreq = 300.0f + formNow * 2500.0f; // formant center
                voice.formantFilter.setMode(CytomicSVF::Mode::BandPass);
                voice.formantFilter.setCoefficients(formFreq, 0.3f + formNow * 0.4f, srf);
                float formantSig = voice.formantFilter.processSample(oscMix);
                float blended = oscMix * (1.0f - formNow * 0.5f) + formantSig * formNow * 0.5f;

                // Main filter
                float envLevel = voice.filterEnv.process();
                float fCut =
                    std::clamp(cutNow + envLevel * pFilterEnvAmt * 6000.0f + l1 * 3000.0f + aftertouchAmount * 3000.0f,
                               200.0f, 20000.0f);
                voice.filter.setMode(CytomicSVF::Mode::LowPass);
                voice.filter.setCoefficients(fCut, std::clamp(pResonance + l2 * 0.15f, 0.0f, 1.0f),
                                             srf); // l2 → resonance shimmer
                float filtered = voice.filter.processSample(blended + dormNoise);

                // Amplitude envelope
                float ampLevel = voice.ampEnv.process() * growthGain;
                if (ampLevel < 1e-6f && voice.ampEnv.getStage() == FilterEnvelope::Stage::Idle)
                {
                    voice.active = false;
                    continue;
                }

                // D001: velocity shapes timbre (not just amplitude)
                float output = filtered * ampLevel * (0.5f + voice.velocity * 0.5f);

                // Mycorrhizal stress propagation — send voice stress to network
                if (voice.velocity > accumulators.aThreshold)
                    network.sendStress(&voice - voices.data(), voice.velocity * accumulators.A * 0.1f,
                                       accumulators.sessionTime);

                // Stereo placement with ensemble width
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
        float growthTime = paramGrowthTime ? paramGrowthTime->load() : 20.0f;

        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.setTargetOrSnap(freq);

        // D001: velocity → attack time (harder = faster attack)
        float attackTime = paramAttack ? paramAttack->load() : 0.15f;
        attackTime *= (1.2f - vel * 0.4f); // faster attack at high velocity
        float decayTime = paramDecay ? paramDecay->load() : 0.5f;
        float sustainLvl = paramSustain ? paramSustain->load() : 0.8f;
        float releaseTime = paramRelease ? paramRelease->load() : 1.0f;

        v.ampEnv.prepare(srf);
        v.ampEnv.setADSR(attackTime, decayTime, sustainLvl, releaseTime);
        v.ampEnv.triggerHard();

        v.filterEnv.prepare(srf);
        v.filterEnv.setADSR(attackTime * 0.5f, decayTime * 1.5f, 0.2f, releaseTime * 0.8f);
        v.filterEnv.triggerHard();

        // Vibrato starts slow, develops — Concertmaster mechanism
        v.vibratoLFO.reset();

        // Growth Mode setup
        v.growthMode = isGrowthMode;
        v.growthTimer = 0.0f;
        v.growthPhase = 0.0f;
        v.growthDuration = growthTime;

        // Dormancy pitch offset — cold strings drift slightly out of tune
        float dPitch = accumulators.getDormancyPitchVariance();
        uint32_t seed = static_cast<uint32_t>(note * 7919 + idx * 31);
        seed = seed * 1664525u + 1013904223u;
        v.dormancyPitchCents = (static_cast<float>(seed & 0xFFFF) / 32768.0f - 1.0f) * dPitch;

        // Stereo spread based on ensemble width and voice index
        float ensWidth = paramEnsembleWidth ? paramEnsembleWidth->load() : 0.7f;
        float panPos = (static_cast<float>(idx) / static_cast<float>(kMaxVoices - 1)) * 2.0f - 1.0f;
        panPos *= ensWidth;
        float angle = (panPos + 1.0f) * 0.25f * 3.14159265f;
        v.panL = std::cos(angle);
        v.panR = std::sin(angle);

        // Mycorrhizal: receive stress from network
        float receivedStress = network.receiveStress(idx, accumulators.sessionTime);
        if (receivedStress > 0.01f)
            v.dormancyPitchCents += receivedStress * 2.0f; // stress → slight detuning
    }

    void noteOff(int note) noexcept
    {
        for (auto& v : voices)
            if (v.active && v.currentNote == note)
                v.ampEnv.release();
    }

    //==========================================================================
    // Parameters — 28 total
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
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_attack", 1}, "Orchard Attack",
                                              juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.3f), 0.15f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_decay", 1}, "Orchard Decay",
                                              juce::NormalisableRange<float>(0.01f, 5.0f, 0.0f, 0.4f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_sustain", 1}, "Orchard Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_release", 1}, "Orchard Release",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.4f), 1.0f));

        // Filter
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_cutoff", 1}, "Orchard Cutoff",
                                              juce::NormalisableRange<float>(200.0f, 20000.0f, 0.0f, 0.3f), 6000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_resonance", 1}, "Orchard Resonance",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_filterEnvAmt", 1}, "Orchard Filter Env Amount",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));

        // Ensemble
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_detune", 1}, "Orchard Ensemble Detune",
                                              juce::NormalisableRange<float>(0.0f, 30.0f, 0.1f), 7.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_ensembleWidth", 1}, "Orchard Ensemble Width",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_formant", 1}, "Orchard Formant Body",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Vibrato
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_vibratoRate", 1}, "Orchard Vibrato Rate",
                                              juce::NormalisableRange<float>(0.5f, 12.0f, 0.1f), 5.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_vibratoDepth", 1}, "Orchard Vibrato Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));

        // GARDEN-specific
        params.push_back(std::make_unique<PI>(juce::ParameterID{"orch_season", 1}, "Orchard Season", -1, 3, -1));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_brightness", 1}, "Orchard Brightness",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_warmth", 1}, "Orchard Warmth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Growth Mode
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_growthMode", 1}, "Orchard Growth Mode",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_growthTime", 1}, "Orchard Growth Time",
                                              juce::NormalisableRange<float>(5.0f, 60.0f, 0.1f), 20.0f));

        // Pitch
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_bendRange", 1}, "Orchard Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));

        // Macros
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_macroCharacter", 1}, "Orchard Macro CHARACTER",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_macroMovement", 1}, "Orchard Macro MOVEMENT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_macroCoupling", 1}, "Orchard Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_macroSpace", 1}, "Orchard Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // LFOs (D002 + D005)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_lfo1Rate", 1}, "Orchard LFO1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_lfo1Depth", 1}, "Orchard LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"orch_lfo1Shape", 1}, "Orchard LFO1 Shape", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_lfo2Rate", 1}, "Orchard LFO2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"orch_lfo2Depth", 1}, "Orchard LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"orch_lfo2Shape", 1}, "Orchard LFO2 Shape", 0, 4, 0));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramAttack = apvts.getRawParameterValue("orch_attack");
        paramDecay = apvts.getRawParameterValue("orch_decay");
        paramSustain = apvts.getRawParameterValue("orch_sustain");
        paramRelease = apvts.getRawParameterValue("orch_release");
        paramCutoff = apvts.getRawParameterValue("orch_cutoff");
        paramResonance = apvts.getRawParameterValue("orch_resonance");
        paramFilterEnvAmt = apvts.getRawParameterValue("orch_filterEnvAmt");
        paramDetune = apvts.getRawParameterValue("orch_detune");
        paramEnsembleWidth = apvts.getRawParameterValue("orch_ensembleWidth");
        paramFormant = apvts.getRawParameterValue("orch_formant");
        paramVibratoRate = apvts.getRawParameterValue("orch_vibratoRate");
        paramVibratoDepth = apvts.getRawParameterValue("orch_vibratoDepth");
        paramSeason = apvts.getRawParameterValue("orch_season");
        paramBrightness = apvts.getRawParameterValue("orch_brightness");
        paramWarmthCtrl = apvts.getRawParameterValue("orch_warmth");
        paramGrowthMode = apvts.getRawParameterValue("orch_growthMode");
        paramGrowthTime = apvts.getRawParameterValue("orch_growthTime");
        paramBendRange = apvts.getRawParameterValue("orch_bendRange");
        paramMacroCharacter = apvts.getRawParameterValue("orch_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("orch_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("orch_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("orch_macroSpace");
        paramLfo1Rate = apvts.getRawParameterValue("orch_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("orch_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("orch_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("orch_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("orch_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("orch_lfo2Shape");
    }

private:
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float srf = 0.0f;  // Sentinel: must be set by prepare() before use

    std::array<OrchardVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount{0};

    GardenAccumulators accumulators;
    GardenMycorrhizalNetwork network;

    ParameterSmoother smoothCutoff, smoothDetune, smoothFormant, smoothAttack;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f, couplingFormantMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramSustain = nullptr;
    std::atomic<float>* paramRelease = nullptr;
    std::atomic<float>* paramCutoff = nullptr;
    std::atomic<float>* paramResonance = nullptr;
    std::atomic<float>* paramFilterEnvAmt = nullptr;
    std::atomic<float>* paramDetune = nullptr;
    std::atomic<float>* paramEnsembleWidth = nullptr;
    std::atomic<float>* paramFormant = nullptr;
    std::atomic<float>* paramVibratoRate = nullptr;
    std::atomic<float>* paramVibratoDepth = nullptr;
    std::atomic<float>* paramSeason = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramWarmthCtrl = nullptr;
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
