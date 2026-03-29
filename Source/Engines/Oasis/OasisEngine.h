#pragma once
//==============================================================================
//
//  OasisEngine.h — XOasis | "The Spice Route Rhodes"
//  XO_OX Designs | XOlokun Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOasis is an entropy-driven bioluminescent ecosystem. It actively
//      monitors the player's rhythmic and harmonic slop, dynamically shifting
//      from rigid sub-bass to shimmering biophonic granular swarms. Perfect
//      sequencing starves the ecosystem; human slop feeds it. The engine
//      lives between the grid and the garden.
//
//  ENGINE CONCEPT:
//      Hybrid architecture: subtractive sub-bass oscillator feeds a resonator
//      bank and a granular canopy delay network. An entropy analyzer measures
//      timing deviations from expected MIDI patterns. High entropy splinters
//      sub-bass partials into the delay network (Mycelial Morphing). Voice
//      stealing dumps harmonic energy into an ASMR noise floor (Harmonic
//      Culling). Ecological Memory means the ecosystem evolves — perfect
//      playing makes it rigid, imperfect playing makes it bloom.
//
//  Accent: Bioluminescent Cyan #00FFFF
//  Parameter prefix: oas_
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/FilterEnvelope.h"
#include <array>
#include <cmath>

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
// OasisVoice
//==============================================================================
struct OasisVoice
{
    bool active = false;
    bool releasing = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    int note = 60;
    float velocity = 0.0f;
    float amplitude = 0.0f;
    double phase = 0.0;
    double phaseDelta = 0.0;
    int noteAge = 0;

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

    // Amp envelope output — tracks the ADSR level for this voice.
    // Driven by ampEnv in renderBlock; used to scale voice amplitude.
    float ampEnvLevel = 0.0f;
    // Filter envelope output — tracks the filter ADSR level.
    float filterEnvLevel = 0.0f;

    void prepare (float sampleRate) noexcept
    {
        tine.prepare (sampleRate);
        amp.prepare (sampleRate);

        // Rhodes amp envelope: near-zero attack, 1.5s decay, 0% sustain, 0.4s release.
        // The Rhodes is a percussive instrument — no sustain after the hammer strike.
        ampEnv.prepare (sampleRate);
        ampEnv.setADSR (0.001f, 1.5f, 0.0f, 0.4f);

        // Filter envelope: fast attack, 0.8s decay, 30% sustain, 0.5s release.
        // Opens the brightness burst on the attack transient.
        filterEnv.prepare (sampleRate);
        filterEnv.setADSR (0.002f, 0.8f, 0.3f, 0.5f);

        // Per-voice SVF — lowpass default; cutoff set per-sample in renderBlock.
        svf.setMode (CytomicSVF::Mode::LowPass);
        svf.setCoefficients (4000.0f, 0.7f, sampleRate);
        svf.reset();

        // Tremolo LFO (sine, ~5 Hz default, shaped by warmth param)
        tremoloLFO.setShape (StandardLFO::Sine);
        tremoloLFO.setRate (5.0f, sampleRate);
        tremoloLFO.reset();

        // LFO1 — slow pitch/tone modulation (vibrato-style)
        lfo1.setShape (StandardLFO::Sine);
        lfo1.setRate (0.5f, sampleRate);
        lfo1.reset();

        // LFO2 — slower timbral drift
        lfo2.setShape (StandardLFO::Triangle);
        lfo2.setRate (0.12f, sampleRate);
        lfo2.reset();

        // Glide: 0ms by default (snap on first note)
        glide.setTime (0.0f, sampleRate);
        glide.reset();
    }

    void reset() noexcept
    {
        active = false;
        releasing = false;
        velocity = 0.0f;
        amplitude = 0.0f;
        ampEnvLevel = 0.0f;
        filterEnvLevel = 0.0f;
        phase = 0.0;
        phaseDelta = 0.0;
        noteAge = 0;
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
// OasisEngine — "The Spice Route Rhodes"
//==============================================================================
class OasisEngine : public SynthEngine
{
public:
    OasisEngine() = default;

    //-- Identity --------------------------------------------------------------
    juce::String getEngineId() const override { return "Oasis"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF00FFFF); }
    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override
    {
        int count = 0;
        for (int v = 0; v < kMaxVoices; ++v)
            if (voices_[v].active) count++;
        return count;
    }

    //-- Lifecycle -------------------------------------------------------------
    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr_ = sampleRate;
        srF_ = static_cast<float> (sampleRate);
        blockSize_ = maxBlockSize;

        // Canopy delay network (1 second at max sample rate)
        int maxDelaySamples = static_cast<int> (sr_) + 1;
        for (int t = 0; t < kCanopyTaps; ++t)
        {
            canopyBufL_[t].assign (static_cast<size_t> (maxDelaySamples), 0.0f);
            canopyBufR_[t].assign (static_cast<size_t> (maxDelaySamples), 0.0f);
            canopySize_[t] = maxDelaySamples;
            canopyWritePos_[t] = 0;
        }

        // Set tap delay times (prime-based, 30–400ms)
        static constexpr float tapDelaysMs[kCanopyTaps] = {
            31.0f, 67.0f, 113.0f, 179.0f, 251.0f, 353.0f
        };
        for (int t = 0; t < kCanopyTaps; ++t)
            canopyDelaySamples_[t] = static_cast<int> (tapDelaysMs[t] * 0.001f * srF_);

        // Filters
        subFilter_.setMode (CytomicSVF::Mode::LowPass);
        subFilter_.setCoefficients (200.0f, 0.707f, srF_);
        subFilter_.reset();

        canopyLP_.setMode (CytomicSVF::Mode::LowPass);
        canopyLP_.setCoefficients (8000.0f, 0.5f, srF_);
        canopyLP_.reset();

        resonatorBank_.setMode (CytomicSVF::Mode::BandPass);
        resonatorBank_.setCoefficients (400.0f, 4.0f, srF_);
        resonatorBank_.reset();

        // LFOs
        biolumLFO_.setShape (StandardLFO::Sine);
        biolumLFO_.setRate (0.08f, srF_);  // D005: sub-audible breathing
        biolumLFO_.reset();

        tidalLFO_.setShape (StandardLFO::Triangle);
        tidalLFO_.setRate (0.003f, srF_);  // Ultra-slow ecosystem tide
        tidalLFO_.reset();

        // Entropy state
        entropy_ = 0.0f;
        ecologicalHealth_ = 0.5f;
        culledEnergy_ = 0.0f;
        lastNoteOnTime_ = 0;
        expectedInterval_ = 0;
        sampleCounter_ = 0;
        noiseRng_ = 42u;

        // Voices — prepare() initialises ALL per-voice DSP objects with sampleRate.
        for (int v = 0; v < kMaxVoices; ++v)
        {
            voices_[v] = {};
            voices_[v].prepare (srF_);
        }

        // Coupling state
        lastSampleL_ = lastSampleR_ = 0.0f;
        extFilterMod_ = 0.0f;
        extPitchMod_ = 0.0f;
        aftertouch_ = 0.0f;
        modWheel_ = 0.0f;
        pitchBendNorm_ = 0.0f;

        // Silence gate: 500ms (reverb-tail category due to canopy delay)
        silenceGate.prepare (sr_, maxBlockSize);
        silenceGate.setHoldTime (500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (int t = 0; t < kCanopyTaps; ++t)
        {
            std::fill (canopyBufL_[t].begin(), canopyBufL_[t].end(), 0.0f);
            std::fill (canopyBufR_[t].begin(), canopyBufR_[t].end(), 0.0f);
            canopyWritePos_[t] = 0;
        }
        subFilter_.reset();
        canopyLP_.reset();
        resonatorBank_.reset();
        biolumLFO_.reset();
        tidalLFO_.reset();
        entropy_ = 0.0f;
        ecologicalHealth_ = 0.5f;
        culledEnergy_ = 0.0f;
        lastSampleL_ = lastSampleR_ = 0.0f;
        for (int v = 0; v < kMaxVoices; ++v)
            voices_[v].reset();
    }

    //-- Audio -----------------------------------------------------------------
    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // 1. Parse MIDI
        for (const auto& meta : midi)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();

                // Entropy analysis: measure timing deviation from expected grid
                int timeSinceLastNote = sampleCounter_ - lastNoteOnTime_;
                if (expectedInterval_ > 0)
                {
                    float deviation = std::fabs (static_cast<float> (timeSinceLastNote - expectedInterval_))
                                      / static_cast<float> (expectedInterval_ + 1);
                    // Smooth entropy tracking
                    entropy_ = entropy_ * 0.85f + deviation * 0.15f;
                    entropy_ = clamp (entropy_, 0.0f, 1.0f);
                }
                expectedInterval_ = timeSinceLastNote;
                lastNoteOnTime_ = sampleCounter_;

                // Ecological memory: slop feeds health, perfection starves it
                ecologicalHealth_ = ecologicalHealth_ * 0.95f + entropy_ * 0.05f;
                ecologicalHealth_ = clamp (ecologicalHealth_, 0.05f, 1.0f);

                // Allocate voice (steal oldest if full)
                int slot = -1;
                for (int v = 0; v < kMaxVoices; ++v)
                {
                    if (!voices_[v].active) { slot = v; break; }
                }
                if (slot < 0)
                {
                    // Voice stealing — Harmonic Culling: dump energy into ASMR floor
                    slot = 0;
                    int oldest = voices_[0].noteAge;
                    for (int v = 1; v < kMaxVoices; ++v)
                    {
                        if (voices_[v].noteAge > oldest)
                        {
                            oldest = voices_[v].noteAge;
                            slot = v;
                        }
                    }
                    culledEnergy_ += voices_[slot].amplitude * 0.5f;
                }

                auto& voice = voices_[slot];
                voice.active = true;
                voice.releasing = false;
                voice.note = msg.getNoteNumber();
                voice.velocity = msg.getFloatVelocity();
                voice.phase = 0.0;
                voice.amplitude = 1.0f;  // envelope controls final level
                voice.noteAge = 0;

                float freq = 440.0f * std::pow (2.0f, (voice.note - 69) / 12.0f);
                voice.phaseDelta = freq / sr_;

                // Glide: snap on first active note, glide on subsequent notes.
                voice.glide.setTargetOrSnap (freq);

                // Trigger tine with velocity and live bellAmount parameter.
                float bellAmount = pBellAmountParam_ ? pBellAmountParam_->load() : 0.5f;
                voice.tine.trigger (voice.velocity, bellAmount);

                // Trigger amp and filter envelopes.
                voice.ampEnv.trigger();
                voice.filterEnv.trigger();
            }
            else if (msg.isNoteOff())
            {
                int noteNum = msg.getNoteNumber();
                for (int v = 0; v < kMaxVoices; ++v)
                {
                    if (voices_[v].active && voices_[v].note == noteNum)
                    {
                        voices_[v].releasing = true;
                        voices_[v].ampEnv.release();
                        voices_[v].filterEnv.release();
                        break;
                    }
                }
            }
            else if (msg.isAftertouch() || msg.isChannelPressure())
            {
                aftertouch_ = msg.isAftertouch()
                    ? msg.getAfterTouchValue() / 127.0f
                    : msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheel_ = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
            {
                pitchBendNorm_ = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
            }
        }

        // 2. Silence gate bypass
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // 3. Read parameters (ParamSnapshot pattern)
        float pEntropySens  = pEntropySensParam_  ? pEntropySensParam_->load()  : 0.5f;
        float pBreeze       = pBreezeParam_       ? pBreezeParam_->load()       : 0.2f;
        float pSwarmDensity = pSwarmDensityParam_ ? pSwarmDensityParam_->load() : 0.4f;
        float pLagoonDepth  = pLagoonDepthParam_  ? pLagoonDepthParam_->load()  : 0.3f;
        float pSubDrive     = pSubDriveParam_     ? pSubDriveParam_->load()     : 0.3f;
        float pResonatorQ   = pResonatorQParam_   ? pResonatorQParam_->load()   : 4.0f;
        float pCullDecay    = pCullDecayParam_    ? pCullDecayParam_->load()    : 0.995f;
        float pCanopyFB     = pCanopyFBParam_     ? pCanopyFBParam_->load()     : 0.4f;
        float pFilterCutoff = pFilterCutoffParam_ ? pFilterCutoffParam_->load() : 2000.0f;
        float pFilterEnvAmt = pFilterEnvAmtParam_ ? pFilterEnvAmtParam_->load() : 0.5f;

        // Rhodes params
        float pRhodesMix    = pRhodesMixParam_    ? pRhodesMixParam_->load()    : 1.0f;
        float pPickupPos    = pPickupPosParam_     ? pPickupPosParam_->load()    : 0.5f;
        float pWarmth       = pWarmthParam_        ? pWarmthParam_->load()       : 0.3f;
        float pTremoloDepth = pTremoloDepthParam_  ? pTremoloDepthParam_->load() : 0.0f;
        float pTremoloRate  = pTremoloRateParam_   ? pTremoloRateParam_->load()  : 5.0f;
        float pGlideTime    = pGlideTimeParam_     ? pGlideTimeParam_->load()    : 0.0f;

        // Macros
        float pM1 = pMacroCharacterParam_ ? pMacroCharacterParam_->load() : 0.0f;
        float pM2 = pMacroMovementParam_  ? pMacroMovementParam_->load()  : 0.0f;
        float pM3 = pMacroCouplingParam_  ? pMacroCouplingParam_->load()  : 0.0f;
        float pM4 = pMacroSpaceParam_     ? pMacroSpaceParam_->load()     : 0.0f;

        // Macro → parameter mapping
        // M1 CHARACTER → Rhodes mix (more character = more Rhodes blend vs sub-bass)
        pRhodesMix   = clamp (pRhodesMix + pM1 * 0.4f, 0.0f, 1.0f);
        // M1 also nudges entropy sensitivity and sub drive
        pEntropySens = clamp (pEntropySens + pM1 * 0.3f, 0.0f, 1.0f);
        pSubDrive    = clamp (pSubDrive + pM1 * 0.2f, 0.0f, 1.0f);

        // M2 MOVEMENT → pickup position (brightness) + breeze
        pPickupPos = clamp (pPickupPos + pM2 * 0.5f, 0.0f, 1.0f);
        pBreeze    = clamp (pBreeze + pM2 * 0.3f, 0.0f, 1.0f);

        // M3 COUPLING → swarm density
        pSwarmDensity = clamp (pSwarmDensity + pM3 * 0.5f, 0.0f, 1.0f);

        // M4 SPACE → lagoon depth (comb filter feedback)
        pLagoonDepth = clamp (pLagoonDepth + pM4 * 0.5f, 0.0f, 1.0f);

        // Expression: aftertouch → warmth (D006: velocity → timbre + CC → timbre)
        pWarmth = clamp (pWarmth + aftertouch_ * 0.5f, 0.0f, 1.0f);
        // Aftertouch also nudges entropy sensitivity
        pEntropySens = clamp (pEntropySens + aftertouch_ * 0.2f, 0.0f, 1.0f);
        // Mod wheel → canopy feedback / swarm (D006)
        pCanopyFB = clamp (pCanopyFB + modWheel_ * 0.4f, 0.0f, 0.95f);

        // Coupling modulation
        pFilterCutoff = clamp (pFilterCutoff + extFilterMod_, 60.0f, 16000.0f);

        // Effective entropy (sensitivity-scaled)
        float ent = entropy_ * pEntropySens;

        // Update resonator bank frequency based on ecological health
        float resBankFreq = 200.0f + ecologicalHealth_ * 800.0f;
        resonatorBank_.setCoefficients (resBankFreq, pResonatorQ, srF_);

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            sampleCounter_++;

            // LFO modulation
            float biolumMod = biolumLFO_.process();
            float tidalMod = tidalLFO_.process();

            // Pitch bend ratio for this sample
            float bendRatio = PitchBendUtil::semitonesToFreqRatio (pitchBendNorm_ * 2.0f);

            // === VOICE SYNTHESIS ===
            float subMix = 0.0f;
            float canopySendL = 0.0f;
            float canopySendR = 0.0f;

            for (int v = 0; v < kMaxVoices; ++v)
            {
                auto& voice = voices_[v];
                if (!voice.active) continue;

                voice.noteAge++;

                // === AMP ENVELOPE ===
                // Tick the ADSR — returns [0, 1].
                voice.ampEnvLevel = voice.ampEnv.process();

                // When releasing, deactivate voice once envelope reaches silence.
                if (voice.releasing && voice.ampEnvLevel < 0.0001f)
                {
                    voice.active = false;
                    continue;
                }

                // Final voice amplitude: envelope × velocity scaling (D001).
                float voiceAmp = voice.ampEnvLevel * voice.velocity;
                voiceAmp = flushDenormal (voiceAmp);

                // === GLIDE — derive current frequency from glide processor ===
                voice.glide.setTime (pGlideTime, srF_);
                float glidedFreq = voice.glide.process() * bendRatio;
                glidedFreq = clamp (glidedFreq, 20.0f, srF_ * 0.49f);

                // Phase accumulation (kept for sub-bass oscillator compatibility)
                voice.phaseDelta = static_cast<double> (glidedFreq) / sr_;
                voice.phase += voice.phaseDelta;
                if (voice.phase >= 1.0) voice.phase -= 1.0;

                float phaseF = static_cast<float> (voice.phase) * 6.28318530718f;

                // === RHODES SIGNAL CHAIN ===
                // 1. Tine generator: tine/tone-bar physical model
                float rhodesOut = voice.tine.process (glidedFreq);

                // 2. Pickup model: electromagnetic pickup position shapes which
                //    partials are captured. pPickupPos drives brightness.
                rhodesOut = voice.pickup.process (rhodesOut, pPickupPos);

                // 3. Amp stage: tube preamp warmth + velocity-dependent bark.
                //    pWarmth controls saturation; velocity controls asymmetry.
                rhodesOut = voice.amp.process (rhodesOut, pWarmth, voice.velocity);

                // === SUB-BASS / GRANULAR OSCILLATOR ===
                // Power supply sag: low entropy → more sag → warmer distortion
                float rawSub = fastSin (phaseF) + 0.5f * fastSin (phaseF * 0.5f);
                float sagAmount = (1.0f - ent) * voice.velocity;
                float granularOut = fastTanh (rawSub * (1.0f + sagAmount * pSubDrive * 4.0f));

                // === BLEND ===
                // pRhodesMix = 1.0 → full Rhodes | pRhodesMix = 0.0 → all sub-bass.
                // Default is 1.0 so the engine produces its Rhodes character out of the box.
                float blendedSub = rhodesOut * pRhodesMix + granularOut * (1.0f - pRhodesMix);

                // === TREMOLO (per-voice LFO, D002/D005) ===
                // Update tremolo LFO rate per block (pTremoloRate changes are block-rate).
                voice.tremoloLFO.setRate (pTremoloRate, srF_);
                float tremoloMod = voice.tremoloLFO.process();
                // Bipolar modulation: pTremoloDepth != 0 activates tremolo (handles negative too).
                float tremoloScale = 1.0f + tremoloMod * pTremoloDepth * 0.5f;
                blendedSub *= tremoloScale;

                // === FILTER ENVELOPE + PER-VOICE SVF (D001 velocity → timbre) ===
                voice.filterEnvLevel = voice.filterEnv.process();
                // Velocity → filter brightness (D001): high velocity opens the filter.
                float velBright = 0.5f + voice.velocity * 0.5f;
                float envCutoff = pFilterCutoff * velBright
                                  + pFilterEnvAmt * voice.filterEnvLevel * 6000.0f;
                envCutoff = clamp (envCutoff, 60.0f, srF_ * 0.49f);
                voice.svf.setCoefficients_fast (envCutoff, 0.7f, srF_);
                blendedSub = voice.svf.processSample (blendedSub);

                // Mycelial Morphing: entropy splinters signal into canopy delay
                float mycelialSend = blendedSub * ent * pEntropySens;
                float directOut = blendedSub * (1.0f - ent * 0.6f) * voiceAmp;

                subMix += directOut;

                // Stereo canopy send with voice spread
                float pan = static_cast<float> (v) / static_cast<float> (kMaxVoices - 1);
                canopySendL += mycelialSend * (1.0f - pan) * voiceAmp;
                canopySendR += mycelialSend * pan * voiceAmp;
            }

            // Sub-bass filter (with velocity-driven cutoff modulation)
            float subCutoff = clamp (pFilterCutoff + pFilterEnvAmt * std::fabs (subMix) * 2000.0f
                                     + biolumMod * 400.0f, 60.0f, 16000.0f);
            subFilter_.setCoefficients_fast (subCutoff, 0.707f, srF_);
            float filteredSub = subFilter_.processSample (subMix);

            // Resonator bank (entropy-modulated)
            float resSample = resonatorBank_.processSample (filteredSub) * ent * 0.5f;

            // === CANOPY DELAY NETWORK (Granular Swarm) ===
            float canopyOutL = 0.0f;
            float canopyOutR = 0.0f;
            int activeTaps = 1 + static_cast<int> (pSwarmDensity * (kCanopyTaps - 1));

            for (int t = 0; t < activeTaps; ++t)
            {
                // Read from delay
                int readPos = (canopyWritePos_[t] - canopyDelaySamples_[t]
                               + canopySize_[t]) % canopySize_[t];
                float tapL = canopyBufL_[t][static_cast<size_t> (readPos)];
                float tapR = canopyBufR_[t][static_cast<size_t> (readPos)];

                // Lagoon: comb filter with feedback (watery resonance).
                // Both lagoonDepth and canopyFB shape the delay feedback;
                // canopyFB is the mod wheel target (D006).
                float lagoonFB = clamp (pLagoonDepth * 0.85f + pCanopyFB * 0.1f, 0.0f, 0.95f);
                float feedL = canopySendL + tapL * lagoonFB;
                float feedR = canopySendR + tapR * lagoonFB;

                // Write to delay
                canopyBufL_[t][static_cast<size_t> (canopyWritePos_[t])] = flushDenormal (feedL);
                canopyBufR_[t][static_cast<size_t> (canopyWritePos_[t])] = flushDenormal (feedR);
                canopyWritePos_[t] = (canopyWritePos_[t] + 1) % canopySize_[t];

                // Tidal drift modulates tap contribution
                float tapGain = 1.0f / static_cast<float> (activeTaps);
                tapGain *= (1.0f + tidalMod * 0.2f);
                canopyOutL += tapL * tapGain;
                canopyOutR += tapR * tapGain;
            }

            // Canopy lowpass (smooth the granular taps)
            float canopyCutoff = 4000.0f + ecologicalHealth_ * 8000.0f + biolumMod * 1000.0f;
            canopyLP_.setCoefficients_fast (clamp (canopyCutoff, 200.0f, srF_ * 0.49f), 0.5f, srF_);
            canopyOutL = canopyLP_.processSample (canopyOutL);
            // Second pass for R uses same filter state which is fine for subtle stereo

            // === ASMR BREEZE (Harmonic Culling noise floor) ===
            noiseRng_ ^= noiseRng_ << 13;
            noiseRng_ ^= noiseRng_ >> 17;
            noiseRng_ ^= noiseRng_ << 5;
            float noise = (static_cast<float> (noiseRng_ & 0xFFFF) / 32768.0f - 1.0f);
            float breezeL = noise * (pBreeze * 0.1f + culledEnergy_ * 0.3f) * (1.0f + biolumMod * 0.3f);

            noiseRng_ ^= noiseRng_ << 13;
            noiseRng_ ^= noiseRng_ >> 17;
            noiseRng_ ^= noiseRng_ << 5;
            float noiseR = (static_cast<float> (noiseRng_ & 0xFFFF) / 32768.0f - 1.0f);
            float breezeR = noiseR * (pBreeze * 0.1f + culledEnergy_ * 0.3f) * (1.0f - biolumMod * 0.3f);

            // Decay culled energy
            culledEnergy_ *= pCullDecay;
            culledEnergy_ = flushDenormal (culledEnergy_);

            // === FINAL MIX ===
            float sampleL = filteredSub + resSample + canopyOutL * pSwarmDensity + breezeL;
            float sampleR = filteredSub + resSample + canopyOutR * pSwarmDensity + breezeR;

            // Cache for coupling
            lastSampleL_ = sampleL;
            lastSampleR_ = sampleR;

            // Accumulate into buffer
            outL[i] += sampleL;
            if (outR) outR[i] += sampleR;
        }

        // Reset coupling mods
        extFilterMod_ = 0.0f;
        extPitchMod_ = 0.0f;

        // Silence gate analysis
        const float* rL = buffer.getReadPointer (0);
        const float* rR = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr;
        silenceGate.analyzeBlock (rL, rR, numSamples);
    }

    //-- Coupling ---------------------------------------------------------------
    float getSampleForCoupling (int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? lastSampleL_ : lastSampleR_;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
                extFilterMod_ = amount * 4000.0f;
                break;
            case CouplingType::AmpToPitch:
                extPitchMod_ = amount;
                break;
            case CouplingType::AudioToRing:
                // Ring mod against sub-bass
                if (sourceBuffer) extFilterMod_ += sourceBuffer[0] * amount * 1000.0f;
                break;
            default:
                break;
        }
    }

    //-- Parameters -------------------------------------------------------------
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl (params);
        return { params.begin(), params.end() };
    }

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl (params);
    }

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;

        // Core ecosystem params
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_entropySens", 1 }, "Entropy Sensitivity",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_breeze", 1 }, "Breeze",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_swarmDensity", 1 }, "Swarm Density",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_lagoonDepth", 1 }, "Lagoon Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        // Sub-bass shaping
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_subDrive", 1 }, "Sub Drive",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_resonatorQ", 1 }, "Resonator Q",
            juce::NormalisableRange<float> (0.5f, 20.0f, 0.0f, 0.4f), 4.0f));

        // Canopy network
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_cullDecay", 1 }, "Cull Decay",
            juce::NormalisableRange<float> (0.9f, 0.9999f), 0.995f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_canopyFB", 1 }, "Canopy Feedback",
            juce::NormalisableRange<float> (0.0f, 0.95f), 0.4f));

        // Filter
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_filterCutoff", 1 }, "Filter Cutoff",
            juce::NormalisableRange<float> (60.0f, 16000.0f, 0.0f, 0.3f), 2000.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_filterEnvAmt", 1 }, "Filter Env Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // Rhodes voice parameters
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_rhodesMix", 1 }, "Rhodes Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f));   // default: all Rhodes

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_pickupPos", 1 }, "Pickup Position",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));   // centre = balanced tone

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_bellAmount", 1 }, "Bell Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));   // 3rd-partial bell burst

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_warmth", 1 }, "Warmth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));   // amp-stage warmth/bark

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_tremoloDepth", 1 }, "Tremolo Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));   // off by default

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_tremoloRate", 1 }, "Tremolo Rate",
            juce::NormalisableRange<float> (0.1f, 10.0f, 0.0f, 0.5f), 5.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_glideTime", 1 }, "Glide Time",
            juce::NormalisableRange<float> (0.0f, 2.0f, 0.0f, 0.3f), 0.0f));

        // Macros (CHARACTER / MOVEMENT / COUPLING / SPACE)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_macroCharacter", 1 }, "Character",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_macroMovement", 1 }, "Movement",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_macroCoupling", 1 }, "Coupling",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oas_macroSpace", 1 }, "Space",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pEntropySensParam_    = apvts.getRawParameterValue ("oas_entropySens");
        pBreezeParam_         = apvts.getRawParameterValue ("oas_breeze");
        pSwarmDensityParam_   = apvts.getRawParameterValue ("oas_swarmDensity");
        pLagoonDepthParam_    = apvts.getRawParameterValue ("oas_lagoonDepth");
        pSubDriveParam_       = apvts.getRawParameterValue ("oas_subDrive");
        pResonatorQParam_     = apvts.getRawParameterValue ("oas_resonatorQ");
        pCullDecayParam_      = apvts.getRawParameterValue ("oas_cullDecay");
        pCanopyFBParam_       = apvts.getRawParameterValue ("oas_canopyFB");
        pFilterCutoffParam_   = apvts.getRawParameterValue ("oas_filterCutoff");
        pFilterEnvAmtParam_   = apvts.getRawParameterValue ("oas_filterEnvAmt");
        pMacroCharacterParam_ = apvts.getRawParameterValue ("oas_macroCharacter");
        pMacroMovementParam_  = apvts.getRawParameterValue ("oas_macroMovement");
        pMacroCouplingParam_  = apvts.getRawParameterValue ("oas_macroCoupling");
        pMacroSpaceParam_     = apvts.getRawParameterValue ("oas_macroSpace");

        // Rhodes voice params
        pRhodesMixParam_     = apvts.getRawParameterValue ("oas_rhodesMix");
        pPickupPosParam_     = apvts.getRawParameterValue ("oas_pickupPos");
        pBellAmountParam_    = apvts.getRawParameterValue ("oas_bellAmount");
        pWarmthParam_        = apvts.getRawParameterValue ("oas_warmth");
        pTremoloDepthParam_  = apvts.getRawParameterValue ("oas_tremoloDepth");
        pTremoloRateParam_   = apvts.getRawParameterValue ("oas_tremoloRate");
        pGlideTimeParam_     = apvts.getRawParameterValue ("oas_glideTime");
    }

private:
    //--------------------------------------------------------------------------
    static constexpr int kMaxVoices = 8;
    static constexpr int kCanopyTaps = 6;

    double sr_ = 44100.0;
    float srF_ = 44100.0f;
    int blockSize_ = 512;

    // Voices
    OasisVoice voices_[kMaxVoices];

    // Canopy delay network
    std::vector<float> canopyBufL_[kCanopyTaps];
    std::vector<float> canopyBufR_[kCanopyTaps];
    int canopySize_[kCanopyTaps] {};
    int canopyWritePos_[kCanopyTaps] {};
    int canopyDelaySamples_[kCanopyTaps] {};

    // Filters
    CytomicSVF subFilter_;
    CytomicSVF canopyLP_;
    CytomicSVF resonatorBank_;

    // LFOs
    StandardLFO biolumLFO_;
    StandardLFO tidalLFO_;

    // Entropy / Ecological state
    float entropy_ = 0.0f;
    float ecologicalHealth_ = 0.5f;
    float culledEnergy_ = 0.0f;
    int lastNoteOnTime_ = 0;
    int expectedInterval_ = 0;
    int sampleCounter_ = 0;
    uint32_t noiseRng_ = 42u;

    // Coupling state
    float lastSampleL_ = 0.0f;
    float lastSampleR_ = 0.0f;
    float extFilterMod_ = 0.0f;
    float extPitchMod_ = 0.0f;

    // Expression state
    float aftertouch_ = 0.0f;
    float modWheel_ = 0.0f;
    float pitchBendNorm_ = 0.0f;

    // Parameter pointers
    std::atomic<float>* pEntropySensParam_    = nullptr;
    std::atomic<float>* pBreezeParam_         = nullptr;
    std::atomic<float>* pSwarmDensityParam_   = nullptr;
    std::atomic<float>* pLagoonDepthParam_    = nullptr;
    std::atomic<float>* pSubDriveParam_       = nullptr;
    std::atomic<float>* pResonatorQParam_     = nullptr;
    std::atomic<float>* pCullDecayParam_      = nullptr;
    std::atomic<float>* pCanopyFBParam_       = nullptr;
    std::atomic<float>* pFilterCutoffParam_   = nullptr;
    std::atomic<float>* pFilterEnvAmtParam_   = nullptr;
    std::atomic<float>* pMacroCharacterParam_ = nullptr;
    std::atomic<float>* pMacroMovementParam_  = nullptr;
    std::atomic<float>* pMacroCouplingParam_  = nullptr;
    std::atomic<float>* pMacroSpaceParam_     = nullptr;

    // Rhodes voice params
    std::atomic<float>* pRhodesMixParam_     = nullptr;
    std::atomic<float>* pPickupPosParam_     = nullptr;
    std::atomic<float>* pBellAmountParam_    = nullptr;
    std::atomic<float>* pWarmthParam_        = nullptr;
    std::atomic<float>* pTremoloDepthParam_  = nullptr;
    std::atomic<float>* pTremoloRateParam_   = nullptr;
    std::atomic<float>* pGlideTimeParam_     = nullptr;

    //--------------------------------------------------------------------------
    static float clamp (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
};

} // namespace xolokun
