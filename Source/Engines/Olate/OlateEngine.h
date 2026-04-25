// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// DEEP-REVIEW: 2026-04-20 — 20 fixes applied (perf, sound, stability, params, voices)
//==============================================================================
//
//  OlateEngine.h — XOlate | "The Aged Wine Analog"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOlate is the wine cellar — oak barrels in the dark, esters forming,
//      tannins mellowing, volatile acids finding equilibrium. The warm analog
//      bass that improves with age. Named from the oblate wine glass shape
//      and the yeast bed (fermentation oblate) at the bottom of a cask.
//
//  ENGINE CONCEPT:
//      Classic subtractive synthesis — 2 oscillators (saw + pulse) through a
//      ladder filter with drive, envelope-driven filter sweep, and a session-
//      scale aging model. The olate_vintage parameter moves through analog
//      circuit eras: early transistor → Moog → TB-303 → modern → unknown.
//      Session-scale tonal drift accumulates subtly, developing character.
//
//  CELLAR QUAD ROLE: Analog Bass / Aged Wine
//      Where XOgre has sub weight, XOlate has warmth and complexity. The analog
//      circuit character that develops over a playing session. Fermentation as
//      synthesis: notes start simple and grow harmonically rich over sustain.
//
//  DSP ARCHITECTURE:
//      1. Dual PolyBLEP oscillators (saw + pulse with variable width)
//      2. Vintage-scaled filter: LP with drive that changes character by era
//      3. Fermentation integrator: harmonic complexity grows over note sustain
//      4. Session-age accumulator: subtle tonal shift over playing time
//      5. Terroir: regional circuit flavor (West Coast, East Coast, UK, Japanese)
//      6. MIDI-domain gravitational coupling
//
//  Accent: Brown #5C3317
//  Parameter prefix: olate_
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PolyBLEP.h"
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
// FermentationIntegrator — harmonic complexity grows over note sustain.
// Operates like a leaky integrator that accumulates nonlinear waveshaping
// at very low rates. Early in note time: mostly fundamental. Late in note
// time: harmonics have grown via cross-modulation and saturation buildup.
// This is the opposite of most envelopes (complex → simple). Fermented
// bass starts simple and grows rich.
//==============================================================================
struct FermentationIntegrator
{
    // FIX (stability/perf): leaky-integrator coeff is now SR-derived so the
    // blend rate is consistent at 44.1, 48, and 96 kHz. Target smoothing TC
    // is 50 ms. Formula: 1 - exp(-2pi * f / sr) with f = 1/(2pi*0.05) ≈ 3.18 Hz.
    void prepare(float sampleRate) noexcept
    {
        lerpCoeff = 1.0f - std::exp(-1.0f / (0.05f * sampleRate));
    }

    void trigger() noexcept
    {
        fermentLevel = 0.0f;
        noteAge = 0.0f;
    }

    float process(float input, float ageRate, float dtSec) noexcept
    {
        noteAge += dtSec;

        // Leaky integration: ferment level grows toward 1.0 at ageRate
        float target = std::min(1.0f, noteAge * ageRate * 0.1f);
        fermentLevel += (target - fermentLevel) * lerpCoeff;
        fermentLevel = flushDenormal(fermentLevel);

        // Apply progressive saturation: more harmonics as fermentation develops
        float driveScale = 1.0f + fermentLevel * 4.0f;
        float fermented = fastTanh(input * driveScale);

        // Mix: fresh → fermented over time
        return input * (1.0f - fermentLevel * 0.7f) + fermented * fermentLevel * 0.7f;
    }

    void reset() noexcept
    {
        fermentLevel = 0.0f;
        noteAge = 0.0f;
    }

    float lerpCoeff  = 0.001f; // overwritten by prepare(); fallback keeps old behaviour
    float fermentLevel = 0.0f;
    float noteAge = 0.0f; // in seconds
};

//==============================================================================
// OlateVoice — Analog bass voice with dual osc + ladder filter + fermentation
//==============================================================================
struct OlateVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 36;
    float velocity = 0.0f;

    GlideProcessor glide;
    PolyBLEP oscSaw;
    PolyBLEP oscPulse;
    FermentationIntegrator ferment;
    FilterEnvelope ampEnv;
    FilterEnvelope filterEnv;
    CytomicSVF ladderFilter; // main LP filter (simulated ladder)
    CytomicSVF warmthFilter; // warmth shaping (low shelf or LP)
    StandardLFO lfo1, lfo2;

    // Gravitational mass accumulator
    float gravityMass = 0.0f;
    float noteHeldTime = 0.0f;

    // Cached per-voice values
    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        gravityMass = 0.0f;
        noteHeldTime = 0.0f;
        glide.reset();
        oscSaw.reset();
        oscPulse.reset();
        ferment.reset();
        ampEnv.kill();
        filterEnv.kill();
        ladderFilter.reset();
        warmthFilter.reset();
        lfo1.reset();
        lfo2.reset();
    }
};

//==============================================================================
// OlateEngine — "The Aged Wine Analog" | Subtractive Bass with Session Aging
//==============================================================================
class OlateEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    juce::String getEngineId() const override { return "Olate"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF5C3317); }
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
            voices[i].ferment.prepare(srf); // FIX: SR-aware leaky coeff
        }

        smoothCutoff.prepare(srf);
        smoothResonance.prepare(srf);
        smoothDrive.prepare(srf);
        smoothOscMix.prepare(srf);
        smoothPulseWidth.prepare(srf);
        smoothWarmth.prepare(srf);

        sessionAge = 0.0f;
        sessionSampleCount = 0;

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
        sessionAge = 0.0f;
        sessionSampleCount = 0.0;
        couplingCacheL = couplingCacheR = 0.0f;
        couplingFilterMod = couplingPitchMod = 0.0f;
        // FIX (P14): snap all smoothers to zero on reset to avoid transients on re-enable
        smoothCutoff.snapTo(4000.0f);
        smoothResonance.snapTo(0.3f);
        smoothDrive.snapTo(0.3f);
        smoothOscMix.snapTo(0.5f);
        smoothPulseWidth.snapTo(0.5f);
        smoothWarmth.snapTo(0.5f);
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
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        auto loadP = [](std::atomic<float>* p, float def) { return p ? p->load(std::memory_order_relaxed) : def; };

        const float pCutoff = loadP(paramCutoff, 4000.0f);
        const float pResonance = loadP(paramResonance, 0.3f);
        const float pDrive = loadP(paramDrive, 0.3f);
        const float pOscMix = loadP(paramOscMix, 0.5f); // 0=saw, 1=pulse
        const float pPulseWidth = loadP(paramPulseWidth, 0.5f);
        const float pVintage = loadP(paramVintage, 0.3f);
        const float pWarmth = loadP(paramWarmth, 0.5f);
        const float pTerroir = loadP(paramTerroir, 0.0f);
        const float pAgeRate = loadP(paramAgeRate, 0.5f);
        const float pGlide = loadP(paramGlide, 0.0f);
        const float pAttack = loadP(paramAttack, 0.005f);
        const float pDecay = loadP(paramDecay, 0.3f);
        const float pSustain = loadP(paramSustain, 0.7f);
        const float pRelease = loadP(paramRelease, 0.3f);
        const float pFiltEnvAmt = loadP(paramFilterEnvAmount, 0.5f);
        const float pFilterAttack = loadP(paramFilterAttack, 0.001f); // FIX: read per-block for live updates
        const float pFilterDecay  = loadP(paramFilterDecay,  0.4f);   // FIX: read per-block for live updates
        const float pBendRange = loadP(paramBendRange, 2.0f);
        const float pGravity = loadP(paramGravity, 0.5f);

        const float macroChar = loadP(paramMacroCharacter, 0.0f);
        const float macroMove = loadP(paramMacroMovement, 0.0f);
        const float macroCoup = loadP(paramMacroCoupling, 0.0f);
        const float macroSpace = loadP(paramMacroSpace, 0.0f);

        // Vintage affects filter character: early transistor (vintage<0.25) = grainy,
        // Moog (0.25-0.5) = fat, TB-303 (0.5-0.75) = acidic, modern (>0.75) = clean
        float vintageResoBoost = 0.0f;
        float vintageDriveBoost = 0.0f;
        if (pVintage < 0.25f)
        {
            vintageDriveBoost = 0.3f; // grainy early transistor
            vintageResoBoost = -0.1f; // less resonance
        }
        else if (pVintage < 0.5f)
        {
            vintageDriveBoost = 0.15f; // fat Moog
            vintageResoBoost = 0.1f;   // more resonance
        }
        else if (pVintage < 0.75f)
        {
            vintageDriveBoost = 0.1f; // 303 acid
            vintageResoBoost = 0.3f;  // screaming resonance
        }
        else
        {
            vintageDriveBoost = 0.0f; // clean modern
            vintageResoBoost = 0.0f;
        }

        // D006: mod wheel → cutoff, aftertouch → resonance
        // FIX (P7): also guard block-level cutoff against Nyquist
        const float nyquistCap = srf * 0.5f - 200.0f;
        float effectiveCutoff =
            std::clamp(pCutoff + macroChar * 4000.0f + modWheelAmount * 3000.0f + couplingFilterMod, 100.0f, nyquistCap);
        float effectiveReso =
            std::clamp(pResonance + vintageResoBoost + aftertouchAmount * 0.3f + macroSpace * 0.2f, 0.0f, 0.95f);
        float effectiveDrive = std::clamp(pDrive + vintageDriveBoost + macroChar * 0.3f, 0.0f, 1.0f);

        smoothCutoff.set(effectiveCutoff);
        smoothResonance.set(effectiveReso);
        smoothDrive.set(effectiveDrive);
        smoothOscMix.set(pOscMix);
        smoothPulseWidth.set(pPulseWidth);
        smoothWarmth.set(pWarmth);

        // Snapshot pitch coupling before reset (#1118).
        const float blockCouplingPitchMod = couplingPitchMod;
        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;

        // Session age: accumulates slowly over the entire playing session
        sessionSampleCount += static_cast<double>(numSamples);
        sessionAge = static_cast<float>(sessionSampleCount / static_cast<double>(sr)); // in seconds

        // Session warmth: gradual tonal drift (caps at ~20 minutes = 1200 seconds)
        float sessionWarmthFactor = std::min(1.0f, sessionAge / 1200.0f);

        // LFO params — macroMove speeds both LFOs for more expressive fretless vibrato
        const float lfo1Rate = loadP(paramLfo1Rate, 0.5f) * (1.0f + macroMove * 2.0f);
        const float lfo1Depth = loadP(paramLfo1Depth, 0.0f);
        const int lfo1Shape = static_cast<int>(loadP(paramLfo1Shape, 0.0f));
        const float lfo2Rate = loadP(paramLfo2Rate, 1.0f) * (1.0f + macroMove * 2.0f);
        const float lfo2Depth = loadP(paramLfo2Depth, 0.0f);
        const int lfo2Shape = static_cast<int>(loadP(paramLfo2Shape, 0.0f));

        // FIX (P15/perf): apply all per-block voice param updates once per voice, not
        // gated by active — inactive voices need fresh settings on next noteOn.
        // Also fixes ADSR setParams-per-sample anti-pattern (hoisted from inner sample loop).
        for (auto& voice : voices)
        {
            voice.lfo1.setRate(lfo1Rate, srf);
            voice.lfo1.setShape(lfo1Shape);
            voice.lfo2.setRate(lfo2Rate, srf);
            voice.lfo2.setShape(lfo2Shape);
            voice.glide.setTime(pGlide, srf);
            voice.ampEnv.setADSR(pAttack, pDecay, pSustain, pRelease);
            // FIX: filterEnv ADSR updated per-block so param tweaks take effect mid-note
            voice.filterEnv.setADSR(pFilterAttack, pFilterDecay, 0.0f, 0.3f);
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        const float dtSec = inverseSr_;

        // Hoist pitch-bend ratio; uses the pre-reset snapshot (#1118).
        const float blockBendRatio =
            PitchBendUtil::semitonesToFreqRatio(bendSemitones + blockCouplingPitchMod);

        for (int s = 0; s < numSamples; ++s)
        {
            float cutNow = smoothCutoff.process();
            float resNow = smoothResonance.process();
            float drvNow = smoothDrive.process();
            float mixNow = smoothOscMix.process();
            float pwNow = smoothPulseWidth.process();
            float warmNow = smoothWarmth.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                float freq = voice.glide.process();
                freq *= blockBendRatio; // hoisted above — was per-sample per-voice fastPow2

                // LFO modulations — FIX: advance lfo2 unconditionally (phase must tick)
                // but only scale when depth>0 or UK terroir might use it
                float lfo1Val = voice.lfo1.process() * lfo1Depth;
                float lfo2Raw = voice.lfo2.process();
                float lfo2Val = lfo2Raw * lfo2Depth;

                // Set oscillator frequencies — FIX (perf): setWaveform is constant,
                // move to noteOn; only setFrequency and setPulseWidth need per-sample updates
                voice.oscSaw.setFrequency(freq, srf);
                voice.oscPulse.setFrequency(freq, srf);
                voice.oscPulse.setPulseWidth(pwNow);

                // Dual oscillator mix
                float sawSample = voice.oscSaw.processSample();
                float pulseSample = voice.oscPulse.processSample();
                float oscOut = sawSample * (1.0f - mixNow) + pulseSample * mixNow;

                // Fermentation: harmonic complexity grows over note sustain
                float fermented = voice.ferment.process(oscOut, pAgeRate, dtSec);

                // Drive / saturation — vintage character
                float driveAmount = 1.0f + drvNow * 6.0f;
                float driven = fastTanh(fermented * driveAmount) / fastTanh(driveAmount);

                // Ladder filter — vintage-era-aware
                float filtEnv = voice.filterEnv.process() * pFiltEnvAmt;
                // D001: velocity scales filter envelope amount
                float velCutMod = voice.velocity * 3000.0f;
                // Session warmth: LP cutoff gently decreases as session ages (tube amp warmup)
                float sessionCutShift = sessionWarmthFactor * -800.0f;
                float voiceCutoff = std::clamp(
                    cutNow + filtEnv * 6000.0f + velCutMod + lfo1Val * 2000.0f + sessionCutShift,
                    100.0f, nyquistCap); // FIX (P7): use block-level nyquistCap (SR-derived)

                // FIX (perf): setMode is constant LowPass — moved to noteOn; only update coefficients
                voice.ladderFilter.setCoefficients(voiceCutoff, resNow, srf);
                float filtered = voice.ladderFilter.processSample(driven);

                // Warmth filter: low shelf boost — tube vs transistor character
                // FIX (P19): mode is constant so only update coefficients when warmth changes;
                // guard both the setMode and processSample to avoid unnecessary CPU when warmth=0.
                if (warmNow > 0.01f)
                {
                    // FIX (P19): mode set at noteOn; update coefficients once per block only
                    if (s == 0)
                        voice.warmthFilter.setCoefficients(200.0f, 0.5f, srf, warmNow * 6.0f);
                    filtered = voice.warmthFilter.processSample(filtered);
                }

                // Terroir: regional circuit flavor as tonal bias (D004 fix — all 4 regions active).
                // 0.0=West Coast cool, 0.4=UK mid-forward, 0.7=East Coast grit, 0.98=Japanese transparent.
                if (pTerroir > 0.01f && pTerroir < 0.4f)
                {
                    // West Coast (Capitol, Western Recorders): darker, cooler top end
                    filtered *= (1.0f - pTerroir * 0.15f);
                }
                else if (pTerroir >= 0.4f && pTerroir < 0.7f)
                {
                    // UK (Abbey Road, Olympic): mid-forward presence via LFO2 path
                    filtered += filtered * lfo2Val * 0.1f;
                }
                else if (pTerroir >= 0.7f && pTerroir < 0.98f)
                {
                    // East Coast (Bell Sound, RCA): harmonic grit — soft saturation + presence boost
                    float eastGrit = (pTerroir - 0.7f) / 0.28f; // 0..1 within region
                    float driveAmt = 1.0f + eastGrit * 2.0f;
                    filtered = fastTanh(filtered * driveAmt) / driveAmt;
                    filtered *= (1.0f + eastGrit * 0.12f); // subtle presence lift
                }
                else if (pTerroir >= 0.98f)
                {
                    // Japanese (Nippon Columbia, Victor): ultra-clean, minimal coloration
                    // Soft gain reduction for pristine headroom — no saturation, no presence
                    filtered *= 0.88f;
                }

                // Amplitude envelope
                float ampLevel = voice.ampEnv.process();
                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // Gravitational mass
                voice.noteHeldTime += dtSec;
                voice.gravityMass = std::min(1.0f, voice.gravityMass + dtSec * 0.1f * pGravity);

                float output = filtered * ampLevel * voice.velocity;

                mixL += output * voice.panL;
                mixR += output * voice.panR;
            }

            // FIX (P8): softClip summed bus before write — 8 voices of driven bass can clip
            outL[s] = softClip(mixL);
            if (outR)
                outR[s] = softClip(mixR);
        }

        // FIX: capture coupling cache once after block (last sample), not per-sample (wasteful)
        if (numSamples > 0)
        {
            const float coupGain = 1.0f + macroCoup * 1.5f;
            couplingCacheL = outL[numSamples - 1] * coupGain;
            couplingCacheR = (outR ? outR[numSamples - 1] : outL[numSamples - 1]) * coupGain;
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

        float freq = midiToFreq(note); // FIX (P4): use fastPow2-based helper, not std::pow

        v.reset();
        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.snapTo(freq);
        v.gravityMass = 0.0f;
        v.noteHeldTime = 0.0f;
        v.ferment.trigger();
        // FIX (perf): set waveforms and filter mode once at noteOn — constant, not per-sample
        v.oscSaw.setWaveform(PolyBLEP::Waveform::Saw);
        v.oscPulse.setWaveform(PolyBLEP::Waveform::Pulse);
        v.ladderFilter.setMode(CytomicSVF::Mode::LowPass);
        v.warmthFilter.setMode(CytomicSVF::Mode::LowShelf);
        // FIX (P18): prepare() must NOT be called per-noteOn — it was already called in
        // prepare() and resets internal filter state, causing clicks on note steal.

        // FIX (P20): read params directly — setADSR is also called per-block in the
        // voice-prep loop so values stay live; noteOn just needs a trigger.
        float attack = paramAttack ? paramAttack->load() : 0.005f;
        float decay = paramDecay ? paramDecay->load() : 0.3f;
        float sustain = paramSustain ? paramSustain->load() : 0.7f;
        float release = paramRelease ? paramRelease->load() : 0.3f;
        v.ampEnv.setADSR(attack, decay, sustain, release);
        v.ampEnv.triggerHard();

        float fAttack = paramFilterAttack ? paramFilterAttack->load() : 0.001f;
        float fDecay  = paramFilterDecay  ? paramFilterDecay->load()  : 0.4f;
        v.filterEnv.setADSR(fAttack, fDecay, 0.0f, 0.3f);
        v.filterEnv.triggerHard();

        // Subtle stereo spread — FIX: use named constant instead of inline pi/2 literal
        static constexpr float kHalfPi = 1.5707963267948966f;
        float pan = 0.5f + (static_cast<float>(idx) - 3.5f) * 0.04f;
        v.panL = std::cos(pan * kHalfPi);
        v.panR = std::sin(pan * kHalfPi);
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

        // Oscillators
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_oscMix", 1}, "Olate Osc Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f)); // 0=saw, 1=pulse
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_pulseWidth", 1}, "Olate Pulse Width",
                                              juce::NormalisableRange<float>(0.01f, 0.99f), 0.5f));

        // Filter
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_cutoff", 1}, "Olate Cutoff",
                                              juce::NormalisableRange<float>(100.0f, 20000.0f, 0.0f, 0.3f), 4000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_resonance", 1}, "Olate Resonance",
                                              juce::NormalisableRange<float>(0.0f, 0.95f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_drive", 1}, "Olate Drive",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_filterEnvAmount", 1}, "Olate Filter Env Amount",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_filterAttack", 1}, "Olate Filter Attack",
                                              juce::NormalisableRange<float>(0.001f, 2.0f, 0.0f, 0.3f), 0.001f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_filterDecay", 1}, "Olate Filter Decay",
                                              juce::NormalisableRange<float>(0.01f, 5.0f, 0.0f, 0.3f), 0.4f));

        // Character
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_vintage", 1}, "Olate Vintage",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_warmth", 1}, "Olate Warmth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_terroir", 1}, "Olate Terroir",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_ageRate", 1}, "Olate Age Rate",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Amp Envelope
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_attack", 1}, "Olate Attack",
                                              juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.3f), 0.005f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_decay", 1}, "Olate Decay",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.3f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_sustain", 1}, "Olate Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_release", 1}, "Olate Release",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.3f), 0.3f));

        // Performance
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_glide", 1}, "Olate Glide",
                                              juce::NormalisableRange<float>(0.0f, 5.0f, 0.0f, 0.3f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_bendRange", 1}, "Olate Pitch Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));

        // Gravitational coupling
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_gravity", 1}, "Olate Gravity",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Macros
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_macroCharacter", 1}, "Olate Macro CHARACTER",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_macroMovement", 1}, "Olate Macro MOVEMENT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_macroCoupling", 1}, "Olate Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_macroSpace", 1}, "Olate Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // LFOs
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_lfo1Rate", 1}, "Olate LFO1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_lfo1Depth", 1}, "Olate LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"olate_lfo1Shape", 1}, "Olate LFO1 Shape", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_lfo2Rate", 1}, "Olate LFO2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"olate_lfo2Depth", 1}, "Olate LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"olate_lfo2Shape", 1}, "Olate LFO2 Shape", 0, 4, 0));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramOscMix = apvts.getRawParameterValue("olate_oscMix");
        paramPulseWidth = apvts.getRawParameterValue("olate_pulseWidth");
        paramCutoff = apvts.getRawParameterValue("olate_cutoff");
        paramResonance = apvts.getRawParameterValue("olate_resonance");
        paramDrive = apvts.getRawParameterValue("olate_drive");
        paramFilterEnvAmount = apvts.getRawParameterValue("olate_filterEnvAmount");
        paramFilterAttack = apvts.getRawParameterValue("olate_filterAttack");
        paramFilterDecay = apvts.getRawParameterValue("olate_filterDecay");
        paramVintage = apvts.getRawParameterValue("olate_vintage");
        paramWarmth = apvts.getRawParameterValue("olate_warmth");
        paramTerroir = apvts.getRawParameterValue("olate_terroir");
        paramAgeRate = apvts.getRawParameterValue("olate_ageRate");
        paramAttack = apvts.getRawParameterValue("olate_attack");
        paramDecay = apvts.getRawParameterValue("olate_decay");
        paramSustain = apvts.getRawParameterValue("olate_sustain");
        paramRelease = apvts.getRawParameterValue("olate_release");
        paramGlide = apvts.getRawParameterValue("olate_glide");
        paramBendRange = apvts.getRawParameterValue("olate_bendRange");
        paramGravity = apvts.getRawParameterValue("olate_gravity");
        paramMacroCharacter = apvts.getRawParameterValue("olate_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("olate_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("olate_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("olate_macroSpace");
        paramLfo1Rate = apvts.getRawParameterValue("olate_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("olate_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("olate_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("olate_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("olate_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("olate_lfo2Shape");
    }

private:
    double sr = 0.0;   // Sentinel: must be set by prepare() before use
    float srf = 0.0f;  // Sentinel: must be set by prepare() before use
    // FIX: sentinel 0.0 instead of 1/48000 — wrong SR assumption if called before prepare()
    float inverseSr_ = 0.0f;

    std::array<OlateVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount{0};

    ParameterSmoother smoothCutoff, smoothResonance, smoothDrive;
    ParameterSmoother smoothOscMix, smoothPulseWidth, smoothWarmth;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    // Session-scale aging state
    // FIX: use double for sample accumulator — float loses sub-second precision after ~349s
    float sessionAge = 0.0f;           // seconds since session start
    double sessionSampleCount = 0.0;   // total samples processed (double for precision)

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    std::atomic<float>* paramOscMix = nullptr;
    std::atomic<float>* paramPulseWidth = nullptr;
    std::atomic<float>* paramCutoff = nullptr;
    std::atomic<float>* paramResonance = nullptr;
    std::atomic<float>* paramDrive = nullptr;
    std::atomic<float>* paramFilterEnvAmount = nullptr;
    std::atomic<float>* paramFilterAttack = nullptr;
    std::atomic<float>* paramFilterDecay = nullptr;
    std::atomic<float>* paramVintage = nullptr;
    std::atomic<float>* paramWarmth = nullptr;
    std::atomic<float>* paramTerroir = nullptr;
    std::atomic<float>* paramAgeRate = nullptr;
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
