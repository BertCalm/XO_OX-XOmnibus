// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OakenEngine.h — XOaken | "The Cured Wood"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOaken is the acoustic bass resting in the cellar — the upright bass,
//      cured wood that has been patient, that has dried and hardened and
//      concentrated its resonance. Smoke-cured, salt-preserved, time-
//      transformed. The instrument that connects the player's fingers
//      to the room through centuries of wood memory.
//
//  ENGINE CONCEPT:
//      Karplus-Strong string synthesis with a 3-mode body resonator (BPF for
//      wood body), switchable exciter (pluck/bow/slap), and a curing model
//      that dries high-frequency content from the sustain over time.
//      The acoustic bass: warm wood resonance, responsive to touch.
//
//  CELLAR QUAD ROLE: Acoustic/Upright Bass / Cured Wood
//      Where XOgre is sub and XOlate is analog, XOaken is organic. The sound
//      of wood vibrating, of gut strings resonating, of the room responding
//      to an upright bass at 2 AM in a jazz club.
//
//  DSP ARCHITECTURE:
//      1. Karplus-Strong: delay line with filtered feedback (string synthesis)
//      2. Exciter: switchable pluck/bow/slap (noise burst / sustained / click+noise)
//      3. Body resonator: 3-mode BPF bank for wood body (200, 580, 1100 Hz)
//      4. Curing model: slow HF removal during sustain (wood drying metaphor)
//      5. String tension: gut vs steel vs synthetic (harmonic series weighting)
//      6. MIDI-domain gravitational coupling
//
//  HISTORICAL LINEAGE:
//      Karplus & Strong (1983), Smith (1992 digital waveguide), Fletcher &
//      Rossing (1998) string instrument acoustics.
//
//  Accent: Dark Walnut #3D2412 (from Visionary doc)
//  Parameter prefix: oaken_
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
// OakenExciter — Three playing styles for the acoustic bass:
//   0 = Pluck (pizzicato): short noise burst, classic KS excitation
//   1 = Bow (arco): sustained filtered noise, continual energy injection
//   2 = Slap: sharp click + wider noise burst for percussive attack
//==============================================================================
struct OakenExciter
{
    void trigger(float velocity, int exciterType, float baseFreq, float sampleRate) noexcept
    {
        active = true;
        type = exciterType;
        sampleCounter = 0;
        vel = velocity;

        noiseState = static_cast<uint32_t>(velocity * 65535.0f) + 54321u;

        if (type == 0) // Pluck
        {
            float periodSamples = sampleRate / std::max(baseFreq, 20.0f);
            burstLength = static_cast<int>(periodSamples * 2.0f);
            burstLength = std::max(burstLength, 16);
        }
        else if (type == 1) // Bow — sustained, active as long as note is held
        {
            burstLength = 1 << 30; // effectively infinite
            bowFilterState = 0.0f;
            // Bow LP cutoff depends on bow pressure (mapped from velocity)
            float fc = 400.0f + velocity * 1200.0f;
            bowLPCoeff = 1.0f - std::exp(-2.0f * 3.14159265f * fc / sampleRate);
        }
        else // Slap
        {
            burstLength = static_cast<int>(sampleRate * 0.008f); // 8ms burst
            burstLength = std::max(burstLength, 16);
        }
    }

    float process(float bowPressure = 0.5f) noexcept
    {
        if (!active)
            return 0.0f;

        float out = 0.0f;

        if (type == 0) // Pluck: decaying noise burst
        {
            if (sampleCounter < burstLength)
            {
                float env = 1.0f - static_cast<float>(sampleCounter) / static_cast<float>(burstLength);
                noiseState = noiseState * 1664525u + 1013904223u;
                float noise = (static_cast<float>(noiseState & 0xFFFF) / 32768.0f - 1.0f);
                out = noise * vel * env;
            }
            else
            {
                active = false;
            }
        }
        else if (type == 1) // Bow: sustained LP-filtered noise
        {
            noiseState = noiseState * 1664525u + 1013904223u;
            float noise = (static_cast<float>(noiseState & 0xFFFF) / 32768.0f - 1.0f);
            // Bow pressure controls energy injection
            float energy = noise * vel * bowPressure * 0.3f;
            bowFilterState += bowLPCoeff * (energy - bowFilterState);
            out = flushDenormal(bowFilterState);
        }
        else // Slap: click + burst
        {
            if (sampleCounter < burstLength)
            {
                float clickPhase = static_cast<float>(sampleCounter) / static_cast<float>(burstLength);
                // Initial click — sharp transient
                float click = (sampleCounter < 8) ? (1.0f - clickPhase * 8.0f) : 0.0f;
                click = std::max(click, 0.0f);
                // Wider noise burst
                noiseState = noiseState * 1664525u + 1013904223u;
                float noise = (static_cast<float>(noiseState & 0xFFFF) / 32768.0f - 1.0f);
                float env = 1.0f - clickPhase;
                out = (click * 2.0f + noise * env) * vel;
            }
            else
            {
                active = false;
            }
        }

        ++sampleCounter;
        return out;
    }

    void stopBow() noexcept
    {
        if (type == 1)
            active = false;
    }

    void reset() noexcept
    {
        active = false;
        sampleCounter = 0;
        bowFilterState = 0.0f;
    }

    bool active = false;
    int type = 0; // 0=pluck, 1=bow, 2=slap
    int sampleCounter = 0;
    int burstLength = 512;
    float vel = 1.0f;
    uint32_t noiseState = 54321u;
    float bowFilterState = 0.0f;
    float bowLPCoeff = 0.1f;
};

//==============================================================================
// OakenKarplusString — Delay line with filtered feedback for string synthesis.
// The core of the acoustic bass model.
// Named with Oaken prefix to avoid collision with OvergrowEngine's KarplusStrongString.
//==============================================================================
struct OakenKarplusString
{
    static constexpr int kMaxDelay = 4096;

    void setFrequency(float freqHz, float sampleRate) noexcept
    {
        sr = sampleRate;
        float periodSamples = sampleRate / std::max(freqHz, 20.0f);
        delaySamples = std::min(periodSamples, static_cast<float>(kMaxDelay - 1));
        // LP coefficient for string damping: higher = brighter, lower = duller
        // Relates to string tension: gut=dull, steel=bright
    }

    void setDamping(float dampCoeff) noexcept { feedbackGain = dampCoeff; }

    void setStringType(float stringTension) noexcept
    {
        // String tension affects the feedback LP filter:
        // Gut (0) = low cutoff (warm, fast decay), Steel (0.5) = mid, Synthetic (1.0) = bright
        float fc = 1000.0f + stringTension * 6000.0f;
        // Matched-Z transform: exp(-2π*fc/sr) — correct for string synthesis LP.
        // Previously used Euler approx 1/(1+1/(fc*0.00005)) which gave near-zero coefficients
        // for gut string tension (fc=1000 Hz), causing unrealistically fast decay.
        float safeSr = (sr > 0.0f) ? sr : 48000.0f;
        stringLPCoeff = std::exp(-2.0f * 3.14159265f * fc / safeSr);
        stringLPCoeff = std::clamp(stringLPCoeff, 0.01f, 0.99f);
    }

    float process(float excitation) noexcept
    {
        // Write excitation + feedback into delay line
        delayLine[writePos] = excitation + readFromDelay() * feedbackGain;

        // Advance write position
        writePos = (writePos + 1) % kMaxDelay;

        // Output is the delayed + filtered signal
        return readFromDelay();
    }

    void reset() noexcept
    {
        std::fill(delayLine.begin(), delayLine.end(), 0.0f);
        writePos = 0;
        lpState = 0.0f;
    }

private:
    float readFromDelay() noexcept
    {
        // Linear interpolation for fractional delay
        float readPos = static_cast<float>(writePos) - delaySamples;
        if (readPos < 0.0f)
            readPos += kMaxDelay;
        int r0 = static_cast<int>(readPos);
        float frac = readPos - static_cast<float>(r0);
        int r1 = (r0 + 1) % kMaxDelay;
        float raw = delayLine[r0] * (1.0f - frac) + delayLine[r1] * frac;

        // One-pole LP for string damping (averaging filter in classic KS)
        lpState += stringLPCoeff * (raw - lpState);
        lpState = flushDenormal(lpState);
        return lpState;
    }

    std::array<float, kMaxDelay> delayLine{};
    int writePos = 0;
    float delaySamples = 100.0f;
    float feedbackGain = 0.99f;
    float stringLPCoeff = 0.5f;
    float lpState = 0.0f;
    float sr = 0.0f;  // Sentinel: must be set by prepare() before use // cached from setFrequency for use in setStringType
};

//==============================================================================
// OakenBodyResonator — 3-mode BPF bank for wood body resonance.
// Simulates the resonant cavity of an upright bass body.
//==============================================================================
struct OakenBodyResonator
{
    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        mode1.setMode(CytomicSVF::Mode::BandPass);
        mode2.setMode(CytomicSVF::Mode::BandPass);
        mode3.setMode(CytomicSVF::Mode::BandPass);
        updateModes(0.5f);
    }

    void updateModes(float woodAge) noexcept
    {
        // Wood age: 0=fresh bright wood, 1=old dark wood
        // Fresh: higher body resonances, wider bandwidth
        // Old: lower, narrower, more colored
        float m1f = 180.0f + woodAge * 40.0f;   // ~180-220 Hz
        float m2f = 520.0f + woodAge * 80.0f;   // ~520-600 Hz
        float m3f = 1000.0f + woodAge * 200.0f; // ~1000-1200 Hz

        float q = 0.4f + woodAge * 0.3f; // old wood = more resonant

        mode1.setCoefficients(m1f, q, sr);
        mode2.setCoefficients(m2f, q + 0.1f, sr);
        mode3.setCoefficients(m3f, q - 0.1f, sr);
    }

    float process(float input, float bodyDepth) noexcept
    {
        if (bodyDepth < 0.001f)
            return input;

        float body =
            mode1.processSample(input) * 0.5f + mode2.processSample(input) * 0.3f + mode3.processSample(input) * 0.2f;

        return input * (1.0f - bodyDepth) + body * bodyDepth;
    }

    void reset() noexcept
    {
        mode1.reset();
        mode2.reset();
        mode3.reset();
    }

    float sr = 0.0f;  // Sentinel: must be set by prepare() before use
    CytomicSVF mode1, mode2, mode3;
};

//==============================================================================
// CuringModel — Slow HF removal during sustain (wood drying metaphor).
// The cured version of a note is darker, drier, more fundamental-focused.
//==============================================================================
struct CuringModel
{
    void prepare(float sampleRate) noexcept
    {
        curingLP.setMode(CytomicSVF::Mode::LowPass);
        sr = sampleRate;
    }

    void trigger() noexcept { curingAge = 0.0f; }

    float process(float input, float curingRate, float dtSec) noexcept
    {
        curingAge += dtSec;

        // Cutoff drops from 12kHz toward 1kHz as the note "cures" over time
        float curingProgress = std::min(1.0f, curingAge * curingRate * 0.2f);
        float cutoff = 12000.0f - curingProgress * 11000.0f;
        cutoff = std::max(cutoff, 500.0f);

        curingLP.setCoefficients(cutoff, 0.3f, sr);
        return curingLP.processSample(input);
    }

    void reset() noexcept
    {
        curingLP.reset();
        curingAge = 0.0f;
    }

    float sr = 0.0f;  // Sentinel: must be set by prepare() before use
    float curingAge = 0.0f;
    CytomicSVF curingLP;
};

//==============================================================================
// OakenVoice
//==============================================================================
struct OakenVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 36;
    float velocity = 0.0f;

    GlideProcessor glide;
    OakenExciter exciter;
    OakenKarplusString string;
    OakenBodyResonator body;
    CuringModel curing;
    FilterEnvelope ampEnv;
    FilterEnvelope filterEnv;
    CytomicSVF outputFilter;
    StandardLFO lfo1, lfo2;

    // Gravitational mass
    float gravityMass = 0.0f;
    float noteHeldTime = 0.0f;

    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        gravityMass = 0.0f;
        noteHeldTime = 0.0f;
        glide.reset();
        exciter.reset();
        string.reset();
        body.reset();
        curing.reset();
        ampEnv.kill();
        filterEnv.kill();
        outputFilter.reset();
        lfo1.reset();
        lfo2.reset();
    }
};

//==============================================================================
// OakenEngine — "The Cured Wood" | Acoustic/Upright Bass
//==============================================================================
class OakenEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    juce::String getEngineId() const override { return "Oaken"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF9C6B30); }
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
            voices[i].body.prepare(srf);
            voices[i].curing.prepare(srf);
        }

        smoothBowPressure.prepare(srf);
        smoothStringTension.prepare(srf);
        smoothBodyDepth.prepare(srf);
        smoothBrightness.prepare(srf);
        smoothWoodAge.prepare(srf);
        smoothCuringRate.prepare(srf);

        prepareSilenceGate(sr, maxBlockSize, 500.0f); // long acoustic tail
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

        const float pBowPressure = loadP(paramBowPressure, 0.5f);
        const float pStringTension = loadP(paramStringTension, 0.5f);
        const float pBodyDepth = loadP(paramBodyDepth, 0.5f);
        const float pBrightness = loadP(paramBrightness, 6000.0f);
        const float pDamping = loadP(paramDamping, 0.3f);
        const float pWoodAge = loadP(paramWoodAge, 0.5f);
        const float pCuringRate = loadP(paramCuringRate, 0.3f);
        const float pRoom = loadP(paramRoom, 0.3f);
        const float pGlide = loadP(paramGlide, 0.0f);
        const float pAttack = loadP(paramAttack, 0.005f);
        const float pDecay = loadP(paramDecay, 1.0f);
        const float pSustain = loadP(paramSustain, 0.6f);
        const float pRelease = loadP(paramRelease, 0.5f);
        const float pFiltEnvAmt = loadP(paramFilterEnvAmount, 0.3f);
        const float pBendRange = loadP(paramBendRange, 2.0f);
        const float pGravity = loadP(paramGravity, 0.5f);

        const float macroChar = loadP(paramMacroCharacter, 0.0f);
        const float macroMove = loadP(paramMacroMovement, 0.0f);
        const float macroCoup = loadP(paramMacroCoupling, 0.0f);
        const float macroSpace = loadP(paramMacroSpace, 0.0f);

        // D006: mod wheel -> bow pressure, aftertouch -> string damping
        float effectiveBowPressure = std::clamp(pBowPressure + modWheelAmount * 0.5f + macroChar * 0.3f, 0.0f, 1.0f);
        float effectiveDamping = 0.95f + (1.0f - pDamping) * 0.049f; // 0.95-0.999 feedback
        effectiveDamping -= aftertouchAmount * 0.03f;                // aftertouch slightly damps
        effectiveDamping = std::clamp(effectiveDamping, 0.9f, 0.999f);

        float effectiveBright = std::clamp(pBrightness + macroChar * 3000.0f + couplingFilterMod, 200.0f, 20000.0f);

        smoothBowPressure.set(effectiveBowPressure);
        smoothStringTension.set(pStringTension);
        smoothBodyDepth.set(std::clamp(pBodyDepth + macroSpace * 0.3f, 0.0f, 1.0f));
        smoothBrightness.set(effectiveBright);
        smoothWoodAge.set(pWoodAge);
        smoothCuringRate.set(pCuringRate);

        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;

        // LFO params — macroMove speeds both LFOs for more organic bowing movement
        const float lfo1Rate = loadP(paramLfo1Rate, 0.5f) * (1.0f + macroMove * 2.0f);
        const float lfo1Depth = loadP(paramLfo1Depth, 0.0f);
        const int lfo1Shape = static_cast<int>(loadP(paramLfo1Shape, 0.0f));
        const float lfo2Rate = loadP(paramLfo2Rate, 1.0f) * (1.0f + macroMove * 2.0f);
        const float lfo2Depth = loadP(paramLfo2Depth, 0.0f);
        const int lfo2Shape = static_cast<int>(loadP(paramLfo2Shape, 0.0f));

        const float roomSpread = pRoom * 0.15f;
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

            // Pre-compute pan gains per voice — depends on currentNote (note-constant) and
            // roomSpread (block-constant). Hoists per-sample std::cos + std::sin out of
            // inner render loop (was up to 64×N pairs of trig calls per block).
            const float panOffset = (static_cast<float>((voice.currentNote * 7) % 13) / 13.0f - 0.5f) * roomSpread;
            const float panPos    = 0.5f + panOffset;
            voice.panL = fastCos(panPos * 1.5707963f);
            voice.panR = fastSin(panPos * 1.5707963f);
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        const float dtSec = inverseSr_;

        for (int s = 0; s < numSamples; ++s)
        {
            const bool updateFilter = ((s & 15) == 0);
            float bowPNow = smoothBowPressure.process();
            float strTNow = smoothStringTension.process();
            float bodyDNow = smoothBodyDepth.process();
            float brightNow = smoothBrightness.process();
            float woodANow = smoothWoodAge.process();
            float curingRNow = smoothCuringRate.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                float freq = voice.glide.process();
                freq *= PitchBendUtil::semitonesToFreqRatio(bendSemitones + couplingPitchMod);

                float lfo1Val = voice.lfo1.process() * lfo1Depth;
                float lfo2Val = voice.lfo2.process() * lfo2Depth;

                // Set string frequency and damping
                voice.string.setFrequency(freq, srf);
                voice.string.setDamping(effectiveDamping);
                voice.string.setStringType(strTNow);

                // Get excitation signal
                float excitation = voice.exciter.process(bowPNow);

                // Karplus-Strong string synthesis
                float stringSample = voice.string.process(excitation);

                // Wood body resonator
                voice.body.updateModes(woodANow);
                float bodied = voice.body.process(stringSample, bodyDNow);

                // Curing model: HF removal over sustain time
                float cured = voice.curing.process(bodied, curingRNow, dtSec);

                // Output brightness filter (env ticked per-sample, SVF decimated)
                float envMod = voice.filterEnv.process() * pFiltEnvAmt * 4000.0f;
                if (updateFilter)
                {
                    float cutoff =
                        std::clamp(brightNow + envMod + lfo1Val * 2000.0f + lfo2Val * 2000.0f + voice.velocity * 2000.0f,
                                   200.0f, 20000.0f);
                    voice.outputFilter.setMode(CytomicSVF::Mode::LowPass);
                    voice.outputFilter.setCoefficients(cutoff, 0.3f, srf);
                }
                float filtered = voice.outputFilter.processSample(cured);

                // Room: simple one-pole reverb-like diffusion
                // (scaled by pRoom — just adds a slight tail)

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

                // Room bleed: pan gains cached per voice in per-block setup loop above.
                mixL += output * voice.panL;
                mixR += output * voice.panR;
            }

            outL[s] = mixL;
            if (outR)
                outR[s] = mixR;
            // macroCoup scales coupling output: higher coupling macro = stronger cross-engine signal
            const float coupGain = 1.0f + macroCoup * 1.5f;
            couplingCacheL = mixL * coupGain;
            couplingCacheR = mixR * coupGain;
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

        int exciterType = paramExciter ? static_cast<int>(paramExciter->load()) : 0;

        v.reset();
        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.snapTo(freq);
        v.gravityMass = 0.0f;
        v.noteHeldTime = 0.0f;

        v.string.reset();
        v.exciter.trigger(vel, exciterType, freq, srf);
        v.body.prepare(srf);
        v.curing.prepare(srf);
        v.curing.trigger();

        v.ampEnv.prepare(srf);
        v.filterEnv.prepare(srf);

        float attack = paramAttack ? paramAttack->load() : 0.005f;
        float decay = paramDecay ? paramDecay->load() : 1.0f;
        float sustain = paramSustain ? paramSustain->load() : 0.6f;
        float release = paramRelease ? paramRelease->load() : 0.5f;
        v.ampEnv.setADSR(attack, decay, sustain, release);
        v.ampEnv.triggerHard();
        v.filterEnv.setADSR(0.001f, 0.3f, 0.0f, 0.4f);
        v.filterEnv.triggerHard();
    }

    void noteOff(int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note)
            {
                v.ampEnv.release();
                v.filterEnv.release();
                v.exciter.stopBow();
            }
        }
    }

    //==========================================================================
    // Parameters — 29 total
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // Playing style
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oaken_exciter", 1}, "Oaken Exciter", 0, 2, 0));
        // 0=pluck, 1=bow, 2=slap
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_bowPressure", 1}, "Oaken Bow Pressure",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_stringTension", 1}, "Oaken String Tension",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f)); // gut→steel→synthetic

        // Body
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_bodyDepth", 1}, "Oaken Body Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_woodAge", 1}, "Oaken Wood Age",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f)); // fresh→old
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_room", 1}, "Oaken Room",
                                              juce::NormalisableRange<float>(0.0f, 1.0f),
                                              0.3f)); // studio→jazz club→concert hall

        // Tone
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_brightness", 1}, "Oaken Brightness",
                                              juce::NormalisableRange<float>(200.0f, 20000.0f, 0.0f, 0.3f), 6000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_damping", 1}, "Oaken Damping",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_curingRate", 1}, "Oaken Curing Rate",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_filterEnvAmount", 1}, "Oaken Filter Env Amount",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Amp Envelope
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_attack", 1}, "Oaken Attack",
                                              juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.3f), 0.005f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_decay", 1}, "Oaken Decay",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.3f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_sustain", 1}, "Oaken Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.6f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_release", 1}, "Oaken Release",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.3f), 0.5f));

        // Performance
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_glide", 1}, "Oaken Glide",
                                              juce::NormalisableRange<float>(0.0f, 5.0f, 0.0f, 0.3f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_bendRange", 1}, "Oaken Pitch Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));

        // Gravitational coupling
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_gravity", 1}, "Oaken Gravity",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Macros
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_macroCharacter", 1}, "Oaken Macro CHARACTER",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_macroMovement", 1}, "Oaken Macro MOVEMENT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_macroCoupling", 1}, "Oaken Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_macroSpace", 1}, "Oaken Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // LFOs
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_lfo1Rate", 1}, "Oaken LFO1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_lfo1Depth", 1}, "Oaken LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oaken_lfo1Shape", 1}, "Oaken LFO1 Shape", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_lfo2Rate", 1}, "Oaken LFO2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oaken_lfo2Depth", 1}, "Oaken LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oaken_lfo2Shape", 1}, "Oaken LFO2 Shape", 0, 4, 0));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramExciter = apvts.getRawParameterValue("oaken_exciter");
        paramBowPressure = apvts.getRawParameterValue("oaken_bowPressure");
        paramStringTension = apvts.getRawParameterValue("oaken_stringTension");
        paramBodyDepth = apvts.getRawParameterValue("oaken_bodyDepth");
        paramWoodAge = apvts.getRawParameterValue("oaken_woodAge");
        paramRoom = apvts.getRawParameterValue("oaken_room");
        paramBrightness = apvts.getRawParameterValue("oaken_brightness");
        paramDamping = apvts.getRawParameterValue("oaken_damping");
        paramCuringRate = apvts.getRawParameterValue("oaken_curingRate");
        paramFilterEnvAmount = apvts.getRawParameterValue("oaken_filterEnvAmount");
        paramAttack = apvts.getRawParameterValue("oaken_attack");
        paramDecay = apvts.getRawParameterValue("oaken_decay");
        paramSustain = apvts.getRawParameterValue("oaken_sustain");
        paramRelease = apvts.getRawParameterValue("oaken_release");
        paramGlide = apvts.getRawParameterValue("oaken_glide");
        paramBendRange = apvts.getRawParameterValue("oaken_bendRange");
        paramGravity = apvts.getRawParameterValue("oaken_gravity");
        paramMacroCharacter = apvts.getRawParameterValue("oaken_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("oaken_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("oaken_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("oaken_macroSpace");
        paramLfo1Rate = apvts.getRawParameterValue("oaken_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("oaken_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("oaken_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("oaken_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("oaken_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("oaken_lfo2Shape");
    }

private:
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float srf = 0.0f;  // Sentinel: must be set by prepare() before use
    float inverseSr_ = 1.0f / 48000.0f;

    std::array<OakenVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount{0};

    ParameterSmoother smoothBowPressure, smoothStringTension;
    ParameterSmoother smoothBodyDepth, smoothBrightness;
    ParameterSmoother smoothWoodAge, smoothCuringRate;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    std::atomic<float>* paramExciter = nullptr;
    std::atomic<float>* paramBowPressure = nullptr;
    std::atomic<float>* paramStringTension = nullptr;
    std::atomic<float>* paramBodyDepth = nullptr;
    std::atomic<float>* paramWoodAge = nullptr;
    std::atomic<float>* paramRoom = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramDamping = nullptr;
    std::atomic<float>* paramCuringRate = nullptr;
    std::atomic<float>* paramFilterEnvAmount = nullptr;
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
