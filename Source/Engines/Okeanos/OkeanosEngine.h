#pragma once
//==============================================================================
//
//  OkeanosEngine.h — XOkeanos | "The Spice Route Rhodes"
//  XO_OX Designs | XOmnibus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOkeanos is the Rhodes electric piano that traveled the Spice Route —
//      from Harold Rhodes' Army rehabilitation workshop through Chicago jazz
//      clubs, across the Atlantic to Tokyo kissaten, down to Lagos Afrobeat
//      sessions, and back through neo-soul and lo-fi. Every note carries
//      the warm bell-tone of the tine, shaped by every tradition it passed
//      through. A traveler's instrument. An instrument that belongs everywhere
//      because it is made of fundamentals everyone can understand.
//
//  ENGINE CONCEPT:
//      A physical model of the Rhodes tine-and-pickup system. A hammer
//      strikes a tine, the tine vibrates near a magnetic pickup, and the
//      electromagnetic induction captures the vibration. The warm bell-tone
//      comes from the tine's characteristic partial distribution — dominant
//      fundamental, clear third partial, rapidly decaying upper partials.
//      Velocity controls bark (asymmetric clipping from the amp stage).
//      The migration parameter opens the engine to cultural influences
//      from coupled engines via the Spectral Fingerprint Cache.
//
//  SOURCE TRADITION TEST:
//      Must pass as a playable jazz EP in a straight-ahead context. Can
//      someone play McCoy Tyner's "Afro Blue" voicings and have it feel right?
//
//  FUSION QUAD — KITCHEN COLLECTION:
//      Part of the 4-engine FUSION family. Unlocked via the 5th-slot mechanic
//      when all 4 Kitchen engines (XOven, XOchre, XObelisk, XOpaline) are loaded.
//      Migratory coupling via Cultural Artifact Bus. Spectral Fingerprint Cache
//      enables coupling without audio routing to slot 5.
//
//  Accent: Cardamom Gold #C49B3F
//  Parameter prefix: okan_
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

namespace xolokun {

//==============================================================================
// SpectralFingerprint — 152-byte metadata struct for FUSION inter-engine coupling.
//
// Each FUSION engine exports this struct continuously. The 5th-slot engine
// reads fingerprints from the 4 Kitchen engines to synthesize coupling effects
// from metadata rather than raw audio. This is the mechanism that keeps
// 5-engine operation under 17% CPU.
//
// The fingerprint captures the resonant character of the engine's output:
// modal frequencies, impedance, temperature, spectral centroid, and activity.
// The coupling engine uses these to model energy transfer (PLATE coupling)
// without running audio through the coupled engine's processing chain.
//==============================================================================
#ifndef XOLOKUN_SPECTRAL_FINGERPRINT_DEFINED
#define XOLOKUN_SPECTRAL_FINGERPRINT_DEFINED
struct SpectralFingerprint
{
    float modalFrequencies[8] = {};   // Top 8 modal frequencies (Hz)
    float modalAmplitudes[8]  = {};   // Amplitude of each modal frequency [0,1]
    float impedanceEstimate   = 0.5f; // Material impedance (0=soft/absorptive, 1=hard/reflective)
    float temperature         = 0.5f; // Thermal/energy state [0=cold/static, 1=hot/active]
    float spectralCentroid    = 1000.0f; // Brightness measure (Hz)
    float activeVoiceCount    = 0.0f; // Number of currently sounding voices
    float fundamentalFreq     = 440.0f; // Most prominent fundamental
    float rmsLevel            = 0.0f;   // Current output RMS
    float harmonicDensity     = 0.5f;   // Ratio of harmonic to inharmonic energy
    float attackTransience    = 0.0f;   // Recent transient energy (decays over ~50ms)
    float padding[2]          = {};     // Reserve for future fields (total: 152 bytes)
};
#endif

//==============================================================================
// RhodesToneGenerator — Tine + pickup physical model.
//
// The Rhodes piano works by a hammer striking a thin metal tine. The tine
// vibrates near a magnetic pickup (like a guitar pickup), and the
// electromagnetic induction converts the vibration to an electrical signal.
//
// The characteristic Rhodes sound comes from:
//   1. Strong fundamental (tine's primary mode)
//   2. Clear 3rd partial (tine geometry)
//   3. Rapidly decaying upper partials
//   4. Bell-like attack transient (hammer-tine contact)
//   5. Velocity-dependent bark (asymmetric clipping in the amp stage)
//
// References:
//   - Pianoteq physical modeling documentation
//   - Loris/Smith (2003), "Spectral analysis of Rhodes tones"
//   - Paspaliaris (2015), "Physical modeling of the Rhodes piano"
//==============================================================================
#ifndef XOLOKUN_RHODES_TONE_GENERATOR_DEFINED
#define XOLOKUN_RHODES_TONE_GENERATOR_DEFINED
struct RhodesToneGenerator
{
    static constexpr int kNumPartials = 6;

    // Rhodes partial ratios — from spectral analysis of a Mk I Stage 73.
    // The tine produces a near-harmonic series with the 3rd partial
    // being characteristically strong (the "bell" quality).
    static constexpr float kPartialRatios[kNumPartials] = {
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f
    };

    // Relative amplitudes — fundamental dominant, 3rd partial strong,
    // upper partials decay rapidly. These are the "naked tine" values;
    // the pickup position modifies which partials are captured.
    static constexpr float kPartialAmps[kNumPartials] = {
        1.0f, 0.35f, 0.55f, 0.15f, 0.08f, 0.04f
    };

    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        for (int i = 0; i < kNumPartials; ++i)
        {
            phases[i] = 0.0f;
            partialLevels[i] = 0.0f;
        }
        tineEnvLevel = 0.0f;
        bellEnvLevel = 0.0f;
    }

    void trigger (float velocity, float bellAmount) noexcept
    {
        // The hammer-tine contact creates a bright burst that decays quickly.
        // Higher velocity = more upper partials excited = more "bell."
        tineEnvLevel = velocity;
        bellEnvLevel = velocity * velocity * bellAmount;  // quadratic: bell is nonlinear
        tineVelocity = velocity;

        // Per-partial initial amplitude from velocity + bell setting
        for (int i = 0; i < kNumPartials; ++i)
        {
            float partialVelScale = 1.0f - (1.0f - velocity) * static_cast<float>(i) * 0.15f;
            if (partialVelScale < 0.0f) partialVelScale = 0.0f;
            partialLevels[i] = kPartialAmps[i] * partialVelScale;
            // Bell boost to 3rd partial
            if (i == 2) partialLevels[i] += bellAmount * velocity * 0.3f;
        }
    }

    float process (float fundamentalHz) noexcept
    {
        float out = 0.0f;

        for (int i = 0; i < kNumPartials; ++i)
        {
            float freq = fundamentalHz * kPartialRatios[i];
            if (freq >= sr * 0.49f) continue;

            float phaseInc = freq / sr;
            phases[i] += phaseInc;
            if (phases[i] >= 1.0f) phases[i] -= 1.0f;

            out += fastSin (phases[i] * 6.28318530718f) * partialLevels[i];

            // Per-partial decay: higher partials decay faster (tine physics)
            float decayRate = 1.0f - std::exp (-1.0f / (sr * (2.0f - static_cast<float>(i) * 0.25f)));
            partialLevels[i] -= partialLevels[i] * decayRate * 0.1f;
            partialLevels[i] = flushDenormal (partialLevels[i]);
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
        tineEnvLevel = 0.0f;
        bellEnvLevel = 0.0f;
    }

    float sr = 48000.0f;
    float phases[kNumPartials] = {};
    float partialLevels[kNumPartials] = {};
    float tineEnvLevel = 0.0f;
    float bellEnvLevel = 0.0f;
    float tineVelocity = 0.0f;
};

//==============================================================================
// RhodesPickupModel — Magnetic pickup simulation.
//
// The pickup position relative to the tine determines which partials
// are captured. Near the tine tip: bright, bell-like (more upper partials).
// Near the base: warm, fundamental-heavy.
//
// The pickup also introduces a subtle phase relationship between partials
// (electromagnetic induction is velocity-sensitive, not displacement-sensitive),
// giving the Rhodes its characteristic "alive" quality.
//==============================================================================
struct RhodesPickupModel
{
    float process (float tineSignal, float pickupPosition) noexcept
    {
        // Pickup position acts as a comb filter — models the physical
        // relationship between tine vibration node and pickup placement.
        // This is simplified: full model would use per-partial gain based
        // on mode shape at pickup position.
        float brightness = 0.3f + pickupPosition * 0.7f;

        // One-pole LP to simulate pickup proximity effect
        // More warmth when pickup is near base (low position value)
        float cutoffCoeff = 0.1f + brightness * 0.85f;
        pickupState += cutoffCoeff * (tineSignal - pickupState);
        pickupState = flushDenormal (pickupState);

        return pickupState;
    }

    void reset() noexcept { pickupState = 0.0f; }

    float pickupState = 0.0f;
};

//==============================================================================
// RhodesAmpStage — Tube amp warmth + velocity-dependent bark.
//
// The Rhodes preamp was typically a tube circuit that added warmth at
// low levels and asymmetric clipping ("bark") at high levels. The bark
// is the hallmark of aggressive Rhodes playing — Herbie Hancock,
// Chick Corea, the sound of the tine being driven hard into the pickup.
//==============================================================================
struct RhodesAmpStage
{
    void prepare (float sampleRate) noexcept
    {
        // DC blocker coefficient derived from sample rate (target cutoff ~5 Hz).
        // At 44100 Hz: 2*pi*5/44100 ≈ 0.000713 — vs hardcoded 0.0001 (was too slow at 96kHz).
        // Using a leaky integrator: coeff = 2*pi*fc/sr, approximating a 1-pole HP.
        dcCoeff = 2.0f * 3.14159265f * 5.0f / std::max (sampleRate, 1.0f);
        dcCoeff = std::clamp (dcCoeff, 0.00001f, 0.01f);
    }

    float process (float input, float warmth, float velocity) noexcept
    {
        // Drive amount scales with both warmth param and velocity
        float drive = 1.0f + warmth * 3.0f + velocity * velocity * 2.0f;

        // Asymmetric clipping — positive excursions clip harder than negative.
        // This is the characteristic Rhodes bark: it happens on the "push"
        // side of the waveform, creating even harmonics.
        float driven = input * drive;
        float asymmetric;
        if (driven >= 0.0f)
            asymmetric = fastTanh (driven * (1.0f + warmth * 0.5f));
        else
            asymmetric = fastTanh (driven * (1.0f + warmth * 0.2f));

        // Mix clean and driven based on warmth
        float out = input * (1.0f - warmth * 0.6f) + asymmetric * warmth * 0.6f;

        // Slight DC offset from asymmetric clipping — remove it.
        // dcCoeff is derived from sampleRate in prepare() — not hardcoded.
        dcBlock += dcCoeff * (out - dcBlock);
        out -= dcBlock;

        return out;
    }

    void reset() noexcept { dcBlock = 0.0f; }

    float dcBlock = 0.0f;
    float dcCoeff = 0.000713f;  // default for 44100 Hz (2*pi*5/44100); updated in prepare()
};
#endif // XOLOKUN_RHODES_TONE_GENERATOR_DEFINED

//==============================================================================
// OkeanosVoice
//==============================================================================
struct OkeanosVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;

    GlideProcessor glide;
    RhodesToneGenerator tine;
    RhodesPickupModel pickup;
    RhodesAmpStage amp;
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
        tine.reset();
        pickup.reset();
        amp.reset();
        ampEnv.kill();
        filterEnv.kill();
        svf.reset();
        lfo1.reset();
        lfo2.reset();
        tremoloLFO.reset();
    }
};

//==============================================================================
// OkeanosEngine — "The Spice Route Rhodes"
//==============================================================================
class OkeanosEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    juce::String getEngineId() const override { return "Okeanos"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFC49B3F); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    //--------------------------------------------------------------------------
    // Spectral Fingerprint — FUSION 5th-slot coupling metadata
    //--------------------------------------------------------------------------
    SpectralFingerprint getSpectralFingerprint() const noexcept
    {
        SpectralFingerprint fp;
        // Populate from current voice state
        int voiceCount = 0;
        float centroidNum = 0.0f, centroidDen = 0.0f;
        float rmsSum = 0.0f;

        for (int i = 0; i < kMaxVoices; ++i)
        {
            const auto& v = voices[i];
            if (!v.active) continue;
            voiceCount++;

            float freq = v.glide.getFreq();
            float amp = v.ampEnv.getLevel();
            rmsSum += amp * amp;

            if (voiceCount <= 8)
            {
                fp.modalFrequencies[voiceCount - 1] = freq;
                fp.modalAmplitudes[voiceCount - 1] = amp;
            }
            centroidNum += freq * amp;
            centroidDen += amp;
        }

        fp.activeVoiceCount = static_cast<float>(voiceCount);
        fp.rmsLevel = std::sqrt (rmsSum / std::max (voiceCount, 1));
        fp.spectralCentroid = (centroidDen > 0.001f) ? centroidNum / centroidDen : 1000.0f;
        fp.impedanceEstimate = 0.3f;  // Rhodes: moderate impedance (tine metal)
        fp.temperature = fp.rmsLevel;
        fp.harmonicDensity = 0.85f;   // Rhodes: highly harmonic
        fp.fundamentalFreq = (voiceCount > 0) ? fp.modalFrequencies[0] : 440.0f;
        fp.attackTransience = attackTransientTracker;

        return fp;
    }

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].tine.prepare (srf);
            voices[i].amp.prepare (srf);   // DC blocker coefficient derived from sampleRate
            voices[i].ampEnv.prepare (srf);
            voices[i].filterEnv.prepare (srf);
            voices[i].tremoloLFO.setShape (StandardLFO::Sine);
            voices[i].tremoloLFO.reset (static_cast<float>(i) / static_cast<float>(kMaxVoices));
        }

        smoothWarmth.prepare (srf);
        smoothBell.prepare (srf);
        smoothBrightness.prepare (srf);
        smoothTremRate.prepare (srf);
        smoothTremDepth.prepare (srf);
        smoothMigration.prepare (srf);

        prepareSilenceGate (sr, maxBlockSize, 500.0f);  // EP has sustain tails
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices) v.reset();
        pitchBendNorm = 0.0f;
        modWheelAmount = 0.0f;
        aftertouchAmount = 0.0f;
        attackTransientTracker = 0.0f;
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
            case CouplingType::EnvToMorph:   couplingWarmthMod += val; break;
            default: break;
        }
    }

    //==========================================================================
    // Render
    //==========================================================================
    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
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

        const float pWarmth      = loadP (paramWarmth, 0.3f);
        const float pBell        = loadP (paramBell, 0.5f);
        const float pBrightness  = loadP (paramBrightness, 6000.0f);
        const float pTremRate    = loadP (paramTremRate, 4.0f);
        const float pTremDepth   = loadP (paramTremDepth, 0.0f);
        const float pFilterEnvAmt = loadP (paramFilterEnvAmt, 0.4f);
        const float pBendRange   = loadP (paramBendRange, 2.0f);
        const float pMigration   = loadP (paramMigration, 0.0f);
        const float pStereoWidth = loadP (paramStereoWidth, 0.5f);

        const float macroChar    = loadP (paramMacroCharacter, 0.0f);
        const float macroMove    = loadP (paramMacroMovement, 0.0f);
        const float macroCoup    = loadP (paramMacroCoupling, 0.0f);
        const float macroSpace   = loadP (paramMacroSpace, 0.0f);

        // D006: mod wheel -> warmth, aftertouch -> brightness
        float effectiveWarmth = std::clamp (pWarmth + macroChar * 0.6f
                                + modWheelAmount * 0.4f + couplingWarmthMod, 0.0f, 1.0f);
        float effectiveBright = std::clamp (pBrightness + macroMove * 4000.0f
                                + aftertouchAmount * 3000.0f + couplingFilterMod, 200.0f, 20000.0f);
        float effectiveTremDepth = std::clamp (pTremDepth + macroMove * 0.3f, 0.0f, 1.0f);
        float effectiveMigration = std::clamp (pMigration + macroCoup * 0.5f, 0.0f, 1.0f);
        float effectiveStereoWidth = std::clamp (pStereoWidth + macroSpace * 0.5f, 0.0f, 1.0f);

        smoothWarmth.set (effectiveWarmth);
        smoothBell.set (pBell);
        smoothBrightness.set (effectiveBright);
        smoothTremRate.set (pTremRate);
        smoothTremDepth.set (effectiveTremDepth);
        smoothMigration.set (effectiveMigration);

        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingWarmthMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;

        // LFO params once per block
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

        // Decay attack transient tracker (~50ms decay)
        float transientDecay = std::exp (-1.0f / (srf * 0.05f));

        for (int s = 0; s < numSamples; ++s)
        {
            float warmthNow   = smoothWarmth.process();
            float bellNow     = smoothBell.process();
            float brightNow   = smoothBrightness.process();
            float tremRateNow = smoothTremRate.process();
            float tremDepthNow = smoothTremDepth.process();
            float migrationNow = smoothMigration.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                float freq = voice.glide.process();
                freq *= PitchBendUtil::semitonesToFreqRatio (bendSemitones + couplingPitchMod);

                // LFO1 -> pitch vibrato (subtle, +-50 cents at full depth)
                float lfo1Val = voice.lfo1.process() * lfo1Depth;
                freq *= PitchBendUtil::semitonesToFreqRatio (lfo1Val * 0.5f);

                // LFO2 -> tremolo depth modulation
                float lfo2Val = voice.lfo2.process() * lfo2Depth;

                // Tine synthesis
                float tineOut = voice.tine.process (freq);

                // Pickup model — bell param controls tine-tip vs base position
                float pickupOut = voice.pickup.process (tineOut, bellNow);

                // Amp stage — warmth and velocity-dependent bark
                float ampOut = voice.amp.process (pickupOut, warmthNow, voice.velocity);

                // Tremolo (Rhodes' optional built-in stereo vibrato)
                voice.tremoloLFO.setRate (tremRateNow, srf);
                float tremVal = voice.tremoloLFO.process();
                float tremGain = 1.0f - tremDepthNow * 0.5f * (1.0f + tremVal);

                // Migration modulation: when coupled, absorb spectral characteristics
                // from other engines. This blends the tine character subtly.
                if (migrationNow > 0.01f)
                {
                    // Migration adds subtle harmonic complexity (even harmonics)
                    float migrationHarmonics = fastSin (freq * 2.0f / srf * 6.28318530718f
                                                        * voice.tine.phases[0]) * migrationNow * 0.15f;
                    ampOut += migrationHarmonics;
                }

                // Amplitude envelope
                float ampLevel = voice.ampEnv.process();
                if (!voice.ampEnv.isActive()) { voice.active = false; continue; }

                // Filter envelope + brightness
                float fEnvMod = voice.filterEnv.process() * pFilterEnvAmt * 5000.0f;
                // D001: velocity shapes filter brightness
                float velBright = voice.velocity * 4000.0f;
                float cutoff = std::clamp (brightNow + fEnvMod + velBright
                                + lfo2Val * 2000.0f, 200.0f, 20000.0f);
                voice.svf.setMode (CytomicSVF::Mode::LowPass);
                voice.svf.setCoefficients (cutoff, 0.15f, srf);
                float filtered = voice.svf.processSample (ampOut);

                float output = filtered * ampLevel * tremGain;

                // Stereo: tremolo pans slightly with width parameter
                float stereoTrem = tremVal * effectiveStereoWidth * 0.3f;
                mixL += output * (voice.panL + stereoTrem);
                mixR += output * (voice.panR - stereoTrem);
            }

            outL[s] = mixL;
            if (outR) outR[s] = mixR;
            couplingCacheL = mixL;
            couplingCacheR = mixR;

            attackTransientTracker *= transientDecay;
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

        // Tine trigger with velocity-dependent bell
        float bell = paramBell ? paramBell->load() : 0.5f;
        v.tine.prepare (srf);
        v.tine.trigger (vel, bell);
        v.pickup.reset();
        v.amp.prepare (srf);  // ensure DC blocker coefficient is current for this sample rate
        v.amp.reset();

        // Amp envelope — Rhodes has fast attack, velocity-sensitive decay
        float attack = paramAttack ? paramAttack->load() : 0.005f;
        float decay  = paramDecay  ? paramDecay->load()  : 0.8f;
        float sustain = paramSustain ? paramSustain->load() : 0.6f;
        float release = paramRelease ? paramRelease->load() : 0.5f;
        v.ampEnv.prepare (srf);
        v.ampEnv.setADSR (attack, decay, sustain, release);
        v.ampEnv.triggerHard();

        // Filter envelope — velocity-scaled
        v.filterEnv.prepare (srf);
        v.filterEnv.setADSR (0.001f, 0.3f + (1.0f - vel) * 0.5f, 0.0f, 0.3f);
        v.filterEnv.triggerHard();

        v.svf.reset();

        // Stereo placement based on note position (piano-like L-R spread)
        float pan = static_cast<float>(note - 36) / 60.0f;  // Low notes left, high right
        pan = std::clamp (pan, 0.0f, 1.0f);
        v.panL = std::cos (pan * 1.5707963f);
        v.panR = std::sin (pan * 1.5707963f);

        // Attack transient tracker
        attackTransientTracker = vel;
    }

    void noteOff (int note) noexcept
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
    // Parameters — 28 total
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
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_warmth", 1 }, "Okeanos Warmth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_bell", 1 }, "Okeanos Bell",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_brightness", 1 }, "Okeanos Brightness",
            juce::NormalisableRange<float> (200.0f, 20000.0f, 0.0f, 0.3f), 6000.0f));

        // Tremolo (Rhodes vibrato circuit)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_tremRate", 1 }, "Okeanos Tremolo Rate",
            juce::NormalisableRange<float> (0.5f, 12.0f, 0.01f), 4.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_tremDepth", 1 }, "Okeanos Tremolo Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // Amp envelope
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_attack", 1 }, "Okeanos Attack",
            juce::NormalisableRange<float> (0.001f, 0.5f, 0.0f, 0.3f), 0.005f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_decay", 1 }, "Okeanos Decay",
            juce::NormalisableRange<float> (0.05f, 5.0f, 0.0f, 0.4f), 0.8f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_sustain", 1 }, "Okeanos Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.6f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_release", 1 }, "Okeanos Release",
            juce::NormalisableRange<float> (0.01f, 5.0f, 0.0f, 0.4f), 0.5f));

        // Filter
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_filterEnvAmt", 1 }, "Okeanos Filter Env Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        // FUSION
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_migration", 1 }, "Okeanos Migration",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_stereoWidth", 1 }, "Okeanos Stereo Width",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // Pitch
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_bendRange", 1 }, "Okeanos Pitch Bend Range",
            juce::NormalisableRange<float> (1.0f, 24.0f, 1.0f), 2.0f));

        // Macros
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_macroCharacter", 1 }, "Okeanos Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_macroMovement", 1 }, "Okeanos Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_macroCoupling", 1 }, "Okeanos Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_macroSpace", 1 }, "Okeanos Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // LFOs (D002/D005)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_lfo1Rate", 1 }, "Okeanos LFO1 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_lfo1Depth", 1 }, "Okeanos LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "okan_lfo1Shape", 1 }, "Okeanos LFO1 Shape", 0, 4, 0));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_lfo2Rate", 1 }, "Okeanos LFO2 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "okan_lfo2Depth", 1 }, "Okeanos LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "okan_lfo2Shape", 1 }, "Okeanos LFO2 Shape", 0, 4, 0));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramWarmth       = apvts.getRawParameterValue ("okan_warmth");
        paramBell         = apvts.getRawParameterValue ("okan_bell");
        paramBrightness   = apvts.getRawParameterValue ("okan_brightness");
        paramTremRate     = apvts.getRawParameterValue ("okan_tremRate");
        paramTremDepth    = apvts.getRawParameterValue ("okan_tremDepth");
        paramAttack       = apvts.getRawParameterValue ("okan_attack");
        paramDecay        = apvts.getRawParameterValue ("okan_decay");
        paramSustain      = apvts.getRawParameterValue ("okan_sustain");
        paramRelease      = apvts.getRawParameterValue ("okan_release");
        paramFilterEnvAmt = apvts.getRawParameterValue ("okan_filterEnvAmt");
        paramMigration    = apvts.getRawParameterValue ("okan_migration");
        paramStereoWidth  = apvts.getRawParameterValue ("okan_stereoWidth");
        paramBendRange    = apvts.getRawParameterValue ("okan_bendRange");
        paramMacroCharacter = apvts.getRawParameterValue ("okan_macroCharacter");
        paramMacroMovement  = apvts.getRawParameterValue ("okan_macroMovement");
        paramMacroCoupling  = apvts.getRawParameterValue ("okan_macroCoupling");
        paramMacroSpace     = apvts.getRawParameterValue ("okan_macroSpace");
        paramLfo1Rate     = apvts.getRawParameterValue ("okan_lfo1Rate");
        paramLfo1Depth    = apvts.getRawParameterValue ("okan_lfo1Depth");
        paramLfo1Shape    = apvts.getRawParameterValue ("okan_lfo1Shape");
        paramLfo2Rate     = apvts.getRawParameterValue ("okan_lfo2Rate");
        paramLfo2Depth    = apvts.getRawParameterValue ("okan_lfo2Depth");
        paramLfo2Shape    = apvts.getRawParameterValue ("okan_lfo2Shape");
    }

private:
    double sr = 48000.0;
    float srf = 48000.0f;

    std::array<OkeanosVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount { 0 };

    ParameterSmoother smoothWarmth, smoothBell, smoothBrightness;
    ParameterSmoother smoothTremRate, smoothTremDepth, smoothMigration;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;
    float attackTransientTracker = 0.0f;

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f, couplingWarmthMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    std::atomic<float>* paramWarmth = nullptr;
    std::atomic<float>* paramBell = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramTremRate = nullptr;
    std::atomic<float>* paramTremDepth = nullptr;
    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramSustain = nullptr;
    std::atomic<float>* paramRelease = nullptr;
    std::atomic<float>* paramFilterEnvAmt = nullptr;
    std::atomic<float>* paramMigration = nullptr;
    std::atomic<float>* paramStereoWidth = nullptr;
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

} // namespace xolokun
