// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OnkoloEngine.h — XOnkolo | "The Diaspora Clavinet"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOnkolo is the Clavinet that remembers the thumb piano. Named for
//      nkolo, a Central African ancestor of the kalimba — because the
//      lineage is the instrument. The rubber pad strikes the string, the
//      magnetic pickup captures the percussive slap, and the whole thing
//      is so funky it makes you move involuntarily. Every note carries
//      the West African diaspora: the kora's thumb articulation, the
//      balafon's wooden bite, centuries of percussive sophistication
//      crossing the Atlantic and landing in Stevie Wonder's hands.
//
//  ENGINE CONCEPT:
//      A physical model of the Hohner Clavinet D6. A rubber pad strikes
//      a tensioned string, which vibrates between two magnetic pickups.
//      The pickup position determines timbre: bridge pickup is bright
//      and cutting, neck pickup is warm and full. The Clavinet's defining
//      characteristic is its aggressive percussive attack and controlled
//      sustain. Key-off produces a mechanical clunk from the damper pad.
//      Auto-wah (envelope-following filter) is the iconic Clavinet effect.
//
//  SOURCE TRADITION TEST:
//      Can it funk? Play "Superstition" on it. Does the thumb-string
//      relationship feel right? The Clavinet's defining property is
//      percussive attack — the string is struck AND damped.
//
//  CULTURAL ROUTE: West African Diaspora
//      Thumb piano -> diaspora -> Bernie Worrell -> funk.
//
//  Accent: Kente Gold #FFB300
//  Parameter prefix: onko_
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

namespace xoceanus {

#ifndef XOLOKUN_SPECTRAL_FINGERPRINT_DEFINED
#define XOLOKUN_SPECTRAL_FINGERPRINT_DEFINED
struct SpectralFingerprint
{
    float modalFrequencies[8] = {};
    float modalAmplitudes[8]  = {};
    float impedanceEstimate   = 0.5f;
    float temperature         = 0.5f;
    float spectralCentroid    = 1000.0f;
    float activeVoiceCount    = 0.0f;
    float fundamentalFreq     = 440.0f;
    float rmsLevel            = 0.0f;
    float harmonicDensity     = 0.5f;
    float attackTransience    = 0.0f;
    float padding[2]          = {};
};
#endif

//==============================================================================
// ClaviStringModel — Struck string with rubber pad excitation.
//
// The Clavinet's string is struck by a small rubber-tipped tangent (like
// a piano hammer but smaller and sharper). The tangent stays in contact
// with the string while the key is held, acting as a damper when released.
// This creates the Clavinet's unique character: bright attack, controlled
// sustain, and a mechanical "clunk" on key release.
//
// The string produces a rich harmonic series with strong odd harmonics
// (similar to a harpsichord). The pickup position then filters which
// harmonics reach the output.
//==============================================================================
struct ClaviStringModel
{
    static constexpr int kNumHarmonics = 8;

    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        for (int i = 0; i < kNumHarmonics; ++i)
        {
            phases[i] = 0.0f;
            harmonicLevels[i] = 0.0f;
            cachedDecayRates[i] = 0.0f;
        }
        // Precompute fast-damp coeff immediately so it's valid before any noteOff.
        cachedFastDampCoeff = 1.0f - std::exp (-1.0f / (sr * 0.05f));
        clunkPhase = 0.0f;
        clunkLevel = 0.0f;
    }

    void trigger (float velocity) noexcept
    {
        vel = velocity;
        // String excitation — percussive, all harmonics excited
        for (int i = 0; i < kNumHarmonics; ++i)
        {
            // Odd harmonics stronger (string struck at center-ish point)
            float oddBoost = ((i % 2) == 0) ? 1.0f : 0.6f;
            float harmDecay = 1.0f / (1.0f + static_cast<float>(i) * 0.5f);
            harmonicLevels[i] = velocity * oddBoost * harmDecay;

            // FIX 3: Precompute per-harmonic decay coefficients (avoids 8 std::exp calls/sample)
            cachedDecayRates[i] = 1.0f - std::exp (-1.0f / (sr * (0.8f - static_cast<float>(i) * 0.08f)));
        }
        keyDown = true;
        clunkLevel = 0.0f;
    }

    void releaseKey (float clunkScale = 1.0f) noexcept
    {
        keyDown = false;
        // Key-off clunk — the damper pad hitting the string
        // produces a brief, bright noise burst.
        // FIX 2: clunkScale (0-1) is pClunk, applied at noteOff time.
        clunkPhase = 0.0f;
        clunkLevel = vel * 0.4f * clunkScale;
        clunkNoiseState = static_cast<uint32_t>(vel * 65535.0f) + 54321u;

        // CPU fix (ONKOLO): precompute the fast-damp coefficient here at noteOff
        // (once per key release) instead of calling std::exp x8 per sample x64 harmonics
        // while the key is held up. sr is constant — result is identical to the
        // previous inline computation in process().
        cachedFastDampCoeff = 1.0f - std::exp (-1.0f / (sr * 0.05f));
    }

    float process (float fundamentalHz, float pickupPos) noexcept
    {
        float out = 0.0f;

        for (int i = 0; i < kNumHarmonics; ++i)
        {
            float harmonicNum = static_cast<float>(i + 1);
            float freq = fundamentalHz * harmonicNum;
            if (freq >= sr * 0.49f) continue;

            // Pickup position comb filtering:
            // At bridge (pos=1.0), all harmonics pass.
            // At neck (pos=0.0), every other harmonic is attenuated.
            float pickupGain = 1.0f;
            if (pickupPos < 0.5f)
            {
                // Neck: attenuate harmonics that have a node near the pickup
                float nodeProximity = std::fabs (fastSin (pickupPos * harmonicNum * 3.14159265f));
                pickupGain = 0.3f + 0.7f * nodeProximity;
            }

            float phaseInc = freq / sr;
            phases[i] += phaseInc;
            if (phases[i] >= 1.0f) phases[i] -= 1.0f;

            out += fastSin (phases[i] * 6.28318530718f) * harmonicLevels[i] * pickupGain;

            // String decay — faster when key is released (damper pad).
            // CPU fix: cachedFastDampCoeff is precomputed in releaseKey() — no std::exp
            // per sample per harmonic. cachedDecayRates[i] is precomputed in trigger().
            float decayRate;
            if (keyDown)
                decayRate = cachedDecayRates[i];
            else
                decayRate = cachedFastDampCoeff;  // precomputed at noteOff (CPU fix)

            harmonicLevels[i] -= harmonicLevels[i] * decayRate;
            harmonicLevels[i] = flushDenormal (harmonicLevels[i]);
        }

        // Key-off clunk (mechanical noise, ~5ms)
        if (clunkLevel > 0.001f)
        {
            clunkNoiseState = clunkNoiseState * 1664525u + 1013904223u;
            float noise = (static_cast<float>(clunkNoiseState & 0xFFFF) / 32768.0f - 1.0f);
            out += noise * clunkLevel;
            clunkLevel *= 0.995f;
            clunkLevel = flushDenormal (clunkLevel);
        }

        return out;
    }

    bool isDead() const noexcept
    {
        float total = clunkLevel;
        for (int i = 0; i < kNumHarmonics; ++i) total += harmonicLevels[i];
        return total < 1e-6f;
    }

    void reset() noexcept
    {
        for (int i = 0; i < kNumHarmonics; ++i)
        {
            phases[i] = 0.0f;
            harmonicLevels[i] = 0.0f;
            cachedDecayRates[i] = 0.0f;
        }
        // Keep cachedFastDampCoeff valid (computed from sr in prepare()).
        keyDown = false;
        clunkPhase = 0.0f;
        clunkLevel = 0.0f;
    }

    float sr = 48000.0f;
    float vel = 0.0f;
    float phases[kNumHarmonics] = {};
    float cachedDecayRates[kNumHarmonics] = {};  // FIX 3: precomputed per trigger()
    float cachedFastDampCoeff = 0.0f;            // CPU fix: precomputed at releaseKey()
    float harmonicLevels[kNumHarmonics] = {};
    bool keyDown = false;
    float clunkPhase = 0.0f;
    float clunkLevel = 0.0f;
    uint32_t clunkNoiseState = 54321u;
};

//==============================================================================
// AutoWahEnvelope — Envelope-following filter for funky playing.
//
// The auto-wah detects the attack amplitude and sweeps a bandpass filter
// upward — the harder you play, the more "wah" you get. This is the
// quintessential Clavinet effect, used by Stevie Wonder on every funk track.
//==============================================================================
struct AutoWahEnvelope
{
    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        envFollower = 0.0f;
        wahBPF.reset();
        // FIX 4: cache envelope coefficients — avoid two std::exp calls per sample
        attackCoeff  = 1.0f - std::exp (-1.0f / (sr * 0.002f));
        releaseCoeff = 1.0f - std::exp (-1.0f / (sr * 0.1f));
    }

    float process (float input, float wahDepth, float velocity) noexcept
    {
        if (wahDepth < 0.01f) return input;

        // Envelope follower — fast attack, slow release
        float absInput = std::fabs (input);
        float coeff = (absInput > envFollower) ? attackCoeff : releaseCoeff;
        envFollower += coeff * (absInput - envFollower);
        envFollower = flushDenormal (envFollower);

        // Wah filter sweep: velocity and envelope control center frequency
        float wahFreq = 400.0f + envFollower * wahDepth * 3000.0f + velocity * wahDepth * 2000.0f;
        wahFreq = std::clamp (wahFreq, 200.0f, 6000.0f);

        wahBPF.setMode (CytomicSVF::Mode::BandPass);
        wahBPF.setCoefficients (wahFreq, 0.6f, sr);
        float wahOut = wahBPF.processSample (input);

        return input * (1.0f - wahDepth * 0.6f) + wahOut * wahDepth * 1.5f;
    }

    void reset() noexcept
    {
        envFollower = 0.0f;
        wahBPF.reset();
    }

    float sr = 48000.0f;
    float envFollower = 0.0f;
    float attackCoeff  = 0.0f;   // FIX 4: cached, set in prepare()
    float releaseCoeff = 0.0f;   // FIX 4: cached, set in prepare()
    CytomicSVF wahBPF;
};

//==============================================================================
// OnkoloVoice
//==============================================================================
struct OnkoloVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;

    GlideProcessor glide;
    ClaviStringModel string;
    AutoWahEnvelope wah;
    FilterEnvelope filterEnv;
    CytomicSVF svf;
    StandardLFO lfo1, lfo2;

    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        glide.reset();
        string.reset();
        wah.reset();
        filterEnv.kill();
        svf.reset();
        lfo1.reset();
        lfo2.reset();
    }
};

//==============================================================================
// OnkoloEngine — "The Diaspora Clavinet"
//==============================================================================
class OnkoloEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    juce::String getEngineId() const override { return "Onkolo"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFFFB300); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    SpectralFingerprint getSpectralFingerprint() const noexcept
    {
        SpectralFingerprint fp;
        int voiceCount = 0;
        float centroidNum = 0.0f, centroidDen = 0.0f, rmsSum = 0.0f;

        for (int i = 0; i < kMaxVoices; ++i)
        {
            const auto& v = voices[i];
            if (!v.active) continue;
            voiceCount++;
            float freq = v.glide.getFreq();
            float amp = v.filterEnv.getLevel();
            rmsSum += amp * amp;
            if (voiceCount <= 8) {
                fp.modalFrequencies[voiceCount - 1] = freq;
                fp.modalAmplitudes[voiceCount - 1] = amp;
            }
            centroidNum += freq * amp;
            centroidDen += amp;
        }

        fp.activeVoiceCount = static_cast<float>(voiceCount);
        fp.rmsLevel = std::sqrt (rmsSum / std::max (voiceCount, 1));
        fp.spectralCentroid = (centroidDen > 0.001f) ? centroidNum / centroidDen : 1200.0f;
        fp.impedanceEstimate = 0.7f;  // Clav: bright, percussive = higher impedance
        fp.temperature = fp.rmsLevel;
        fp.harmonicDensity = 0.9f;    // Highly harmonic (tensioned string)
        fp.fundamentalFreq = (voiceCount > 0) ? fp.modalFrequencies[0] : 440.0f;
        fp.attackTransience = attackTracker;
        return fp;
    }

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].string.prepare (srf);
            voices[i].wah.prepare (srf);
            voices[i].filterEnv.prepare (srf);
        }

        smoothFunk.prepare (srf);
        smoothPickup.prepare (srf);
        smoothBrightness.prepare (srf);
        smoothMigration.prepare (srf);

        prepareSilenceGate (sr, maxBlockSize, 200.0f);  // Clav: percussive, shorter tail
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices) v.reset();
        pitchBendNorm = 0.0f;
        modWheelAmount = 0.0f;
        aftertouchAmount = 0.0f;
        attackTracker = 0.0f;
    }

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        (void) sampleIndex;
        return (channel == 0) ? couplingCacheL : couplingCacheR;
    }

    void applyCouplingInput (CouplingType type, float amount,
                            const float* buf, int numSamples) override
    {
        if (!buf || numSamples <= 0) return;
        float val = buf[numSamples - 1] * amount;
        switch (type) {
            case CouplingType::AmpToFilter:  couplingFilterMod += val * 2000.0f; break;
            case CouplingType::LFOToPitch:   couplingPitchMod += val * 2.0f; break;
            case CouplingType::AmpToPitch:   couplingPitchMod += val; break;
            case CouplingType::EnvToMorph:   couplingFunkMod += val; break;
            default: break;
        }
    }

    //==========================================================================
    // Render
    //==========================================================================
    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())          { noteOn (msg.getNoteNumber(), msg.getFloatVelocity()); wakeSilenceGate(); }
            else if (msg.isNoteOff())    noteOff (msg.getNoteNumber());
            else if (msg.isPitchWheel()) pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
            else if (msg.isChannelPressure()) aftertouchAmount = msg.getChannelPressureValue() / 127.0f;
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
        }

        if (isSilenceGateBypassed()) {
            buffer.clear (0, numSamples);
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        auto loadP = [] (std::atomic<float>* p, float def) {
            return p ? p->load (std::memory_order_relaxed) : def;
        };

        const float pFunk        = loadP (paramFunk, 0.5f);
        const float pPickup      = loadP (paramPickup, 0.7f);
        const float pBrightness  = loadP (paramBrightness, 8000.0f);
        const float pFilterEnvAmt = loadP (paramFilterEnvAmt, 0.6f);
        const float pBendRange   = loadP (paramBendRange, 2.0f);
        const float pMigration   = loadP (paramMigration, 0.0f);

        const float macroChar    = loadP (paramMacroCharacter, 0.0f);
        const float macroMove    = loadP (paramMacroMovement, 0.0f);
        const float macroCoup    = loadP (paramMacroCoupling, 0.0f);
        const float macroSpace   = loadP (paramMacroSpace, 0.0f);

        // D006: mod wheel -> funk (wah depth), aftertouch -> pickup brightness
        float effectiveFunk = std::clamp (pFunk + macroChar * 0.4f
                                + modWheelAmount * 0.5f + couplingFunkMod, 0.0f, 1.0f);
        float effectivePickup = std::clamp (pPickup + macroSpace * 0.3f
                                + aftertouchAmount * 0.3f, 0.0f, 1.0f);
        float effectiveBright = std::clamp (pBrightness + macroMove * 4000.0f
                                + aftertouchAmount * 3000.0f + couplingFilterMod, 200.0f, 20000.0f);

        smoothFunk.set (effectiveFunk);
        smoothPickup.set (effectivePickup);
        smoothBrightness.set (effectiveBright);
        smoothMigration.set (std::clamp (pMigration + macroCoup * 0.5f, 0.0f, 1.0f));

        couplingFilterMod = 0.0f;
        const float pitchCouplingVal = couplingPitchMod;
        couplingPitchMod = 0.0f;
        couplingFunkMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;

        // LFO params
        const float lfo1Rate  = loadP (paramLfo1Rate, 0.5f);
        const float lfo1Depth = loadP (paramLfo1Depth, 0.0f);
        const int   lfo1Shape = static_cast<int>(loadP (paramLfo1Shape, 0.0f));
        const float lfo2Rate  = loadP (paramLfo2Rate, 1.0f);
        const float lfo2Depth = loadP (paramLfo2Depth, 0.0f);
        const int   lfo2Shape = static_cast<int>(loadP (paramLfo2Shape, 0.0f));

        for (auto& voice : voices)
        {
            if (!voice.active) continue;
            voice.lfo1.setRate (lfo1Rate, srf);
            voice.lfo1.setShape (lfo1Shape);
            voice.lfo2.setRate (lfo2Rate, srf);
            voice.lfo2.setShape (lfo2Shape);
        }

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        float transientDecay = std::exp (-1.0f / (srf * 0.03f));

        for (int s = 0; s < numSamples; ++s)
        {
            float funkNow    = smoothFunk.process();
            float pickupNow  = smoothPickup.process();
            float brightNow  = smoothBrightness.process();
            float migrationN = smoothMigration.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                float freq = voice.glide.process();
                freq *= PitchBendUtil::semitonesToFreqRatio (bendSemitones + pitchCouplingVal);

                // LFO1 -> pitch
                float lfo1Val = voice.lfo1.process() * lfo1Depth;
                freq *= PitchBendUtil::semitonesToFreqRatio (lfo1Val * 0.5f);

                // LFO2 -> funk depth modulation
                float lfo2Val = voice.lfo2.process() * lfo2Depth;
                float voiceFunk = std::clamp (funkNow + lfo2Val * 0.3f, 0.0f, 1.0f);

                // String model with pickup position
                float stringOut = voice.string.process (freq, pickupNow);

                // Auto-wah envelope filter (the funk!)
                float wahOut = voice.wah.process (stringOut, voiceFunk, voice.velocity);

                // Migration — adds resonance from coupled engines
                if (migrationN > 0.01f)
                {
                    float migOdd = fastSin (freq * 3.0f / srf * 6.28318530718f
                                            * voice.string.phases[0]) * migrationN * 0.1f;
                    wahOut += migOdd;
                }

                // Check if voice has fully decayed
                if (voice.string.isDead() && !voice.string.keyDown)
                {
                    voice.active = false;
                    continue;
                }

                // Filter with velocity-driven brightness (D001)
                float fEnvMod = voice.filterEnv.process() * pFilterEnvAmt * 5000.0f;
                float velBright = voice.velocity * voice.velocity * 5000.0f;  // quadratic for aggressive response
                float cutoff = std::clamp (brightNow + fEnvMod + velBright, 200.0f, 20000.0f);
                voice.svf.setMode (CytomicSVF::Mode::LowPass);
                voice.svf.setCoefficients (cutoff, 0.15f, srf);
                float filtered = voice.svf.processSample (wahOut);

                float output = filtered;

                mixL += output * voice.panL;
                mixR += output * voice.panR;
            }

            outL[s] = mixL;
            if (outR) outR[s] = mixR;
            couplingCacheL = mixL;
            couplingCacheR = mixR;

            attackTracker *= transientDecay;
        }

        int count = 0;
        for (const auto& v : voices) if (v.active) ++count;
        activeVoiceCount.store (count);
        analyzeForSilenceGate (buffer, numSamples);
    }

    //==========================================================================
    // Note management
    //==========================================================================
    void noteOn (int note, float vel) noexcept
    {
        int idx = VoiceAllocator::findFreeVoice (voices, kMaxVoices);
        auto& v = voices[idx];

        float freq = 440.0f * std::pow (2.0f, (static_cast<float>(note) - 69.0f) / 12.0f);

        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.snapTo (freq);

        v.string.prepare (srf);
        v.string.trigger (vel);
        v.wah.prepare (srf);

        // Filter envelope — use user ADSR parameters (FIX 1: was hardcoded)
        auto loadP = [] (std::atomic<float>* p, float def) {
            return p ? p->load (std::memory_order_relaxed) : def;
        };
        const float fAttack  = loadP (paramAttack,  0.001f);
        const float fDecay   = loadP (paramDecay,   0.3f);
        const float fSustain = loadP (paramSustain,  0.4f);
        const float fRelease = loadP (paramRelease,  0.15f);

        v.filterEnv.prepare (srf);
        v.filterEnv.setADSR (fAttack, fDecay, fSustain, fRelease);
        v.filterEnv.triggerHard();

        v.svf.reset();

        // Stereo placement
        float pan = static_cast<float>(note - 36) / 60.0f;
        pan = std::clamp (pan, 0.0f, 1.0f);
        v.panL = std::cos (pan * 1.5707963f);
        v.panR = std::sin (pan * 1.5707963f);

        attackTracker = vel;
    }

    void noteOff (int note) noexcept
    {
        // FIX 2: read pClunk live so releaseKey() receives the actual parameter value
        const float clunkScale = paramClunk ? paramClunk->load (std::memory_order_relaxed) : 0.5f;
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note)
            {
                v.string.releaseKey (clunkScale);
                v.filterEnv.release();
            }
        }
    }

    //==========================================================================
    // Parameters — 27 total
    //==========================================================================
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl (params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl (params);
        return { params.begin(), params.end() };
    }

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // Core tone
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_funk", 1 }, "Onkolo Funk (Wah Depth)",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_pickup", 1 }, "Onkolo Pickup Position",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.7f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_brightness", 1 }, "Onkolo Brightness",
            juce::NormalisableRange<float> (200.0f, 20000.0f, 0.0f, 0.3f), 8000.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_clunk", 1 }, "Onkolo Key-Off Clunk",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // Amp envelope
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_attack", 1 }, "Onkolo Attack",
            juce::NormalisableRange<float> (0.001f, 0.2f, 0.0f, 0.3f), 0.001f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_decay", 1 }, "Onkolo Decay",
            juce::NormalisableRange<float> (0.05f, 3.0f, 0.0f, 0.4f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_sustain", 1 }, "Onkolo Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_release", 1 }, "Onkolo Release",
            juce::NormalisableRange<float> (0.01f, 2.0f, 0.0f, 0.4f), 0.15f));

        // Filter
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_filterEnvAmt", 1 }, "Onkolo Filter Env Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.6f));

        // FUSION
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_migration", 1 }, "Onkolo Migration",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // Pitch
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_bendRange", 1 }, "Onkolo Pitch Bend Range",
            juce::NormalisableRange<float> (1.0f, 24.0f, 1.0f), 2.0f));

        // Macros
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_macroCharacter", 1 }, "Onkolo Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_macroMovement", 1 }, "Onkolo Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_macroCoupling", 1 }, "Onkolo Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_macroSpace", 1 }, "Onkolo Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // LFOs (D002/D005)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_lfo1Rate", 1 }, "Onkolo LFO1 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_lfo1Depth", 1 }, "Onkolo LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "onko_lfo1Shape", 1 }, "Onkolo LFO1 Shape", 0, 4, 0));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_lfo2Rate", 1 }, "Onkolo LFO2 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "onko_lfo2Depth", 1 }, "Onkolo LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "onko_lfo2Shape", 1 }, "Onkolo LFO2 Shape", 0, 4, 0));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramFunk         = apvts.getRawParameterValue ("onko_funk");
        paramPickup       = apvts.getRawParameterValue ("onko_pickup");
        paramBrightness   = apvts.getRawParameterValue ("onko_brightness");
        paramClunk        = apvts.getRawParameterValue ("onko_clunk");
        paramAttack       = apvts.getRawParameterValue ("onko_attack");
        paramDecay        = apvts.getRawParameterValue ("onko_decay");
        paramSustain      = apvts.getRawParameterValue ("onko_sustain");
        paramRelease      = apvts.getRawParameterValue ("onko_release");
        paramFilterEnvAmt = apvts.getRawParameterValue ("onko_filterEnvAmt");
        paramMigration    = apvts.getRawParameterValue ("onko_migration");
        paramBendRange    = apvts.getRawParameterValue ("onko_bendRange");
        paramMacroCharacter = apvts.getRawParameterValue ("onko_macroCharacter");
        paramMacroMovement  = apvts.getRawParameterValue ("onko_macroMovement");
        paramMacroCoupling  = apvts.getRawParameterValue ("onko_macroCoupling");
        paramMacroSpace     = apvts.getRawParameterValue ("onko_macroSpace");
        paramLfo1Rate     = apvts.getRawParameterValue ("onko_lfo1Rate");
        paramLfo1Depth    = apvts.getRawParameterValue ("onko_lfo1Depth");
        paramLfo1Shape    = apvts.getRawParameterValue ("onko_lfo1Shape");
        paramLfo2Rate     = apvts.getRawParameterValue ("onko_lfo2Rate");
        paramLfo2Depth    = apvts.getRawParameterValue ("onko_lfo2Depth");
        paramLfo2Shape    = apvts.getRawParameterValue ("onko_lfo2Shape");
    }

private:
    double sr = 48000.0;
    float srf = 48000.0f;

    std::array<OnkoloVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount { 0 };

    ParameterSmoother smoothFunk, smoothPickup, smoothBrightness, smoothMigration;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;
    float attackTracker = 0.0f;

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f, couplingFunkMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    std::atomic<float>* paramFunk = nullptr;
    std::atomic<float>* paramPickup = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramClunk = nullptr;
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
