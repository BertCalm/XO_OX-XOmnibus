// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OasisEngine.h — XOasis | "The Spice Route Rhodes"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
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
//  Accent: Desert Spring Teal #00827F
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

namespace xoceanus
{

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
    float modalAmplitudes[8] = {};    // Amplitude of each modal frequency [0,1]
    float impedanceEstimate = 0.5f;   // Material impedance (0=soft/absorptive, 1=hard/reflective)
    float temperature = 0.5f;         // Thermal/energy state [0=cold/static, 1=hot/active]
    float spectralCentroid = 1000.0f; // Brightness measure (Hz)
    float activeVoiceCount = 0.0f;    // Number of currently sounding voices
    float fundamentalFreq = 440.0f;   // Most prominent fundamental
    float rmsLevel = 0.0f;            // Current output RMS
    float harmonicDensity = 0.5f;     // Ratio of harmonic to inharmonic energy
    float attackTransience = 0.0f;    // Recent transient energy (decays over ~50ms)
    float padding[2] = {};            // Reserve for future fields (total: 152 bytes)
};
#endif

//==============================================================================
// OasisRhodesToneGenerator — Tine physical model (OASIS-specific).
//
// OASIS uses STRETCHED partial ratios (Paspaliaris 2015 Table 2) and a
// two-argument process() that applies per-partial pickup gains in-line.
// Named with Oasis prefix to avoid collision with OkeanosEngine's
// RhodesToneGenerator (which uses integer harmonic ratios and a simpler
// 1-argument process). Both live in namespace xoceanus; same guard name
// would cause the Okeanos version to win.
//
// References:
//   - Pianoteq physical modeling documentation
//   - Loris/Smith (2003), "Spectral analysis of Rhodes tones"
//   - Paspaliaris (2015), "Physical modeling of the Rhodes piano"
//==============================================================================
#ifndef XOLOKUN_OASIS_RHODES_DSP_DEFINED
#define XOLOKUN_OASIS_RHODES_DSP_DEFINED
struct OasisRhodesToneGenerator
{
    static constexpr int kNumPartials = 6;

    // Stretched harmonic ratios — tine stiffness stretches upper partials sharp.
    // These ratios match Paspaliaris (2015) Table 2 for middle-register tines.
    static constexpr float kPartialRatios[kNumPartials] = {1.0f, 1.981f, 3.006f, 4.024f, 5.052f, 6.098f};

    // Relative amplitudes — fundamental dominant, 3rd partial strong ("bell" quality).
    // Upper partials decay rapidly. These are raw "naked tine" values before
    // pickup position filtering (which further shapes per-partial levels).
    static constexpr float kPartialAmps[kNumPartials] = {1.0f, 0.35f, 0.55f, 0.15f, 0.08f, 0.04f};

    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        for (int i = 0; i < kNumPartials; ++i)
        {
            phases[i] = 0.0f;
            partialLevels[i] = 0.0f;
            // Cache matched-Z decay coefficients — depend only on sr and partial index.
            // Decay time T(i) = 2.0 - i*0.25 seconds. Computed once here; never again
            // per-sample. Formula: 1 - exp(-1 / (sr * T(i)))
            float T = 2.0f - static_cast<float>(i) * 0.25f;
            cachedDecayRates[i] = 1.0f - std::exp(-1.0f / (sampleRate * T));
        }
        tineEnvLevel = 0.0f;
        bellEnvLevel = 0.0f;
    }

    void trigger(float velocity, float bellAmount) noexcept
    {
        // The hammer-tine contact creates a bright burst that decays quickly.
        // Higher velocity = more upper partials excited = more "bell."
        tineEnvLevel = velocity;
        bellEnvLevel = velocity * velocity * bellAmount; // quadratic: bell is nonlinear
        tineVelocity = velocity;
        bellAmountLive = bellAmount;

        // Per-partial initial amplitude from velocity (bell boost for index 2 is NOT baked in
        // here — it is computed live in process() using bellAmountLive so the knob responds
        // on held notes without requiring a new note-on).
        for (int i = 0; i < kNumPartials; ++i)
        {
            float partialVelScale = 1.0f - (1.0f - velocity) * static_cast<float>(i) * 0.15f;
            if (partialVelScale < 0.0f)
                partialVelScale = 0.0f;
            partialLevels[i] = kPartialAmps[i] * partialVelScale;
        }
        // Store base level for index 2 so process() can derive the proportional bell
        // contribution as the partial decays.
        baseLevelIndex2 = partialLevels[2];
    }

    // process() with optional per-partial pickup gains.
    // pickupGains[i] comes from OasisRhodesPickupModel::partialGains[i] — call
    // pickup.setPickupPosition() at note-on, then pass pickup.partialGains here.
    // Set bellAmountLive each block (from the block-rate parameter snapshot) to make
    // the Bell Amount knob respond in real time on held notes.
    float process(float fundamentalHz, const float* pickupGains = nullptr) noexcept
    {
        float out = 0.0f;

        for (int i = 0; i < kNumPartials; ++i)
        {
            float freq = fundamentalHz * kPartialRatios[i];
            if (freq >= sr * 0.49f)
                continue;

            float phaseInc = freq / sr;
            phases[i] += phaseInc;
            if (phases[i] >= 1.0f)
                phases[i] -= 1.0f;

            // For the bell partial (index 2), recompute the total level from the
            // decaying base envelope plus the live bell contribution so that the
            // Bell Amount knob affects held notes in real time.
            float effectiveLevel = partialLevels[i];
            if (i == 2)
            {
                // partialLevels[2] carries the decaying base amplitude.
                // bellAmountLive × tineVelocity × 0.3 recreates the bell boost
                // at whatever the knob is currently set to, scaled by the same
                // per-partial decay that the base level has already undergone.
                float bellDecayRatio = (baseLevelIndex2 > 0.0001f) ? partialLevels[2] / baseLevelIndex2 : 0.0f;
                effectiveLevel = partialLevels[2] + bellAmountLive * tineVelocity * 0.3f * bellDecayRatio;
            }

            float partialGain = pickupGains ? pickupGains[i] : 1.0f;
            out += fastSin(phases[i] * 6.28318530718f) * effectiveLevel * partialGain;

            // Per-partial decay: higher partials decay faster (tine physics).
            // cachedDecayRates[i] was precomputed in prepare() — zero std::exp per sample.
            partialLevels[i] -= partialLevels[i] * cachedDecayRates[i] * 0.1f;
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
        tineEnvLevel = 0.0f;
        bellEnvLevel = 0.0f;
    }

    float sr = 48000.0f;
    float phases[kNumPartials] = {};
    float partialLevels[kNumPartials] = {};
    // Precomputed matched-Z decay poles — set in prepare(), never recalculated per sample.
    float cachedDecayRates[kNumPartials] = {};
    float tineEnvLevel = 0.0f;
    float bellEnvLevel = 0.0f;
    float tineVelocity = 0.0f;
    // Live bell amount — set each block from the block-rate parameter snapshot
    // so that the Bell Amount knob responds in real time on held notes.
    float bellAmountLive = 0.5f;
    // Base level of partial index 2 at trigger time (before bell boost), used in
    // process() to compute the proportional bell contribution during decay.
    float baseLevelIndex2 = 0.0f;
};

//==============================================================================
// OasisRhodesAmpStage — Tube amp warmth + velocity-dependent bark (OASIS).
//
// The Rhodes preamp was typically a tube circuit that added warmth at
// low levels and asymmetric clipping ("bark") at high levels. The bark
// is the hallmark of aggressive Rhodes playing — Herbie Hancock,
// Chick Corea, the sound of the tine being driven hard into the pickup.
// Named with Oasis prefix to avoid collision with OkeanosEngine's version.
//==============================================================================
struct OasisRhodesAmpStage
{
    void prepare(float sampleRate) noexcept
    {
        // DC blocker coefficient derived from sample rate (target cutoff ~5 Hz).
        // At 44100 Hz: 2*pi*5/44100 ≈ 0.000713 — vs hardcoded 0.0001 (was too slow at 96kHz).
        // Using a leaky integrator: coeff = 2*pi*fc/sr, approximating a 1-pole HP.
        dcCoeff = 2.0f * 3.14159265f * 5.0f / std::max(sampleRate, 1.0f);
        dcCoeff = std::clamp(dcCoeff, 0.00001f, 0.01f);
    }

    float process(float input, float warmth, float velocity) noexcept
    {
        // Drive amount scales with both warmth param and velocity
        float drive = 1.0f + warmth * 3.0f + velocity * velocity * 2.0f;

        // Asymmetric clipping — positive excursions clip harder than negative.
        // This is the characteristic Rhodes bark: it happens on the "push"
        // side of the waveform, creating even harmonics.
        float driven = input * drive;
        float asymmetric;
        if (driven >= 0.0f)
            asymmetric = fastTanh(driven * (1.0f + warmth * 0.5f));
        else
            asymmetric = fastTanh(driven * (1.0f + warmth * 0.2f));

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
    float dcCoeff = 0.000713f; // default for 44100 Hz (2*pi*5/44100); updated in prepare()
};
#endif // XOLOKUN_OASIS_RHODES_DSP_DEFINED

//==============================================================================
// OasisRhodesPickupModel — Magnetic pickup simulation (OASIS-specific).
//
// Extended version of the Rhodes pickup that precomputes per-partial gains
// based on pickup position. This is OASIS-specific and lives outside the
// shared XOLOKUN_RHODES_TONE_GENERATOR_DEFINED guard so it is always defined,
// even when the guard is already set by OkeanosEngine.h (which has a simpler
// pickup model without setPickupPosition / partialGains).
//
// The pickup position relative to the tine determines which partials are
// captured. Gain for partial n at normalised position x ≈ |sin(n·π·x)|,
// derived from the fixed-free beam velocity mode shape.
//   - pickupPosition=0.0 = near base (warm, fundamental-heavy)
//   - pickupPosition=1.0 = near tip (bright, even partials boosted)
//
// References: Smith & Perttunen (2002) "Acoustic Pickup Position and Timbre"
//==============================================================================
#ifndef XOLOKUN_OASIS_RHODES_PICKUP_MODEL_DEFINED
#define XOLOKUN_OASIS_RHODES_PICKUP_MODEL_DEFINED
struct OasisRhodesPickupModel
{
    static constexpr int kNumPartials = 6;
    float partialGains[kNumPartials] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

    // Call once per note-on (not per sample) — precomputes per-partial gains.
    void setPickupPosition(float pickupPosition) noexcept
    {
        // pickupPosition [0,1]: 0=base (warm), 1=tip (bright).
        // Gain for partial n at normalised position x ≈ |sin(n * pi * x)|
        // We use n=1..6 and clamp to avoid silence at exactly 0.
        float x = 0.05f + pickupPosition * 0.95f; // avoid pure-node singularity
        for (int i = 0; i < kNumPartials; ++i)
        {
            float n = static_cast<float>(i + 1);
            float modeGain = std::fabs(fastSin(n * 3.14159265f * x));
            // Fundamental always gets a floor so the note never disappears
            if (i == 0)
                modeGain = std::max(modeGain, 0.4f);
            partialGains[i] = modeGain;
        }
        // Normalise so the fundamental is always unity
        float fundGain = partialGains[0];
        if (fundGain > 0.001f)
            for (int i = 0; i < kNumPartials; ++i)
                partialGains[i] /= fundGain;
    }

    // Per-sample process: one-pole LP that models pickup proximity effect.
    // Main timbral shaping is done via partialGains[] in OasisRhodesToneGenerator::process().
    float process(float tineSignal, float pickupPosition) noexcept
    {
        float brightness = 0.3f + pickupPosition * 0.7f;
        float cutoffCoeff = 0.05f + brightness * 0.9f;
        pickupState += cutoffCoeff * (tineSignal - pickupState);
        pickupState = flushDenormal(pickupState);
        return pickupState;
    }

    void reset() noexcept
    {
        pickupState = 0.0f;
        for (int i = 0; i < kNumPartials; ++i)
            partialGains[i] = 1.0f;
    }

    float pickupState = 0.0f;
};
#endif // XOLOKUN_OASIS_RHODES_PICKUP_MODEL_DEFINED

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
    OasisRhodesToneGenerator tine;
    OasisRhodesPickupModel pickup;
    OasisRhodesAmpStage amp;
    FilterEnvelope ampEnv;
    FilterEnvelope filterEnv;
    CytomicSVF svf;
    // lfo1 and lfo2 removed — they were initialised but never ticked in renderBlock.
    // tremoloLFO is the active per-voice LFO (wired to tremolo depth/rate params).
    StandardLFO tremoloLFO;

    float panL = 0.707f, panR = 0.707f;

    // Amp envelope output — tracks the ADSR level for this voice.
    // Driven by ampEnv in renderBlock; used to scale voice amplitude.
    float ampEnvLevel = 0.0f;
    // Filter envelope output — tracks the filter ADSR level.
    float filterEnvLevel = 0.0f;

    // Voice-steal crossfade: 5ms linear fade-out ramp applied to the
    // outgoing voice before it is re-triggered. crossfadeRamp counts down
    // from crossfadeSamples to 0. While > 0 the voice outputs with a fade gain.
    int crossfadeSamples = 0; // total ramp length in samples (set in prepare())
    int crossfadeCounter = 0; // remaining ramp samples (0 = crossfade done)
    bool crossfading = false; // true while fading out the old note
    // Frequency for the incoming stolen note — computed once at steal time to
    // avoid std::pow per sample during the crossfade countdown.
    float cachedRetriggerFreq = 440.0f;

    void prepare(float sampleRate) noexcept
    {
        tine.prepare(sampleRate);
        amp.prepare(sampleRate);

        // Rhodes amp envelope: near-zero attack, 1.5s decay, 0% sustain, 0.4s release.
        // The Rhodes is a percussive instrument — no sustain after the hammer strike.
        ampEnv.prepare(sampleRate);
        ampEnv.setADSR(0.001f, 1.5f, 0.0f, 0.4f);

        // Filter envelope: fast attack, 0.8s decay, 30% sustain, 0.5s release.
        // Opens the brightness burst on the attack transient.
        filterEnv.prepare(sampleRate);
        filterEnv.setADSR(0.002f, 0.8f, 0.3f, 0.5f);

        // Per-voice SVF — lowpass default; cutoff set per-sample in renderBlock.
        svf.setMode(CytomicSVF::Mode::LowPass);
        svf.setCoefficients(4000.0f, 0.7f, sampleRate);
        svf.reset();

        // Tremolo LFO (sine, ~5 Hz default, shaped by warmth param)
        tremoloLFO.setShape(StandardLFO::Sine);
        tremoloLFO.setRate(5.0f, sampleRate);
        tremoloLFO.reset();

        // Glide: 0ms by default (snap on first note)
        glide.setTime(0.0f, sampleRate);
        glide.reset();

        // 5ms crossfade ramp (fleet pattern: prevent click on voice steal)
        crossfadeSamples = static_cast<int>(0.005f * sampleRate);
        crossfadeCounter = 0;
        crossfading = false;
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
        crossfadeCounter = 0;
        crossfading = false;
        glide.reset();
        tine.reset();
        pickup.reset();
        amp.reset();
        ampEnv.kill();
        filterEnv.kill();
        svf.reset();
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
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF00827F); }
    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override
    {
        int count = 0;
        for (int v = 0; v < kMaxVoices; ++v)
            if (voices_[v].active)
                count++;
        return count;
    }

    //-- Lifecycle -------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr_ = sampleRate;
        srF_ = static_cast<float>(sampleRate);
        blockSize_ = maxBlockSize;

        // Canopy delay network (1 second at max sample rate)
        int maxDelaySamples = static_cast<int>(sr_) + 1;
        for (int t = 0; t < kCanopyTaps; ++t)
        {
            canopyBufL_[t].assign(static_cast<size_t>(maxDelaySamples), 0.0f);
            canopyBufR_[t].assign(static_cast<size_t>(maxDelaySamples), 0.0f);
            canopySize_[t] = maxDelaySamples;
            canopyWritePos_[t] = 0;
        }

        // Set tap delay times (prime-based, 30–400ms)
        static constexpr float tapDelaysMs[kCanopyTaps] = {31.0f, 67.0f, 113.0f, 179.0f, 251.0f, 353.0f};
        for (int t = 0; t < kCanopyTaps; ++t)
            canopyDelaySamples_[t] = static_cast<int>(tapDelaysMs[t] * 0.001f * srF_);

        // Filters
        subFilter_.setMode(CytomicSVF::Mode::LowPass);
        subFilter_.setCoefficients(200.0f, 0.707f, srF_);
        subFilter_.reset();

        canopyLP_.setMode(CytomicSVF::Mode::LowPass);
        canopyLP_.setCoefficients(8000.0f, 0.5f, srF_);
        canopyLP_.reset();

        canopyLPR_.setMode(CytomicSVF::Mode::LowPass);
        canopyLPR_.setCoefficients(8000.0f, 0.5f, srF_);
        canopyLPR_.reset();

        // Harmonic Culling noise LP: one-pole at 2 kHz — gives the stolen-voice
        // energy the characteristic soft "whoosh" of a dying resonance rather
        // than harsh white noise.
        breezeLP_.setMode(CytomicSVF::Mode::LowPass);
        breezeLP_.setCoefficients(2000.0f, 0.7f, srF_);
        breezeLP_.reset();

        resonatorBank_.setMode(CytomicSVF::Mode::BandPass);
        resonatorBank_.setCoefficients(400.0f, 4.0f, srF_);
        resonatorBank_.reset();

        // LFOs
        biolumLFO_.setShape(StandardLFO::Sine);
        biolumLFO_.setRate(0.08f, srF_); // D005: sub-audible breathing
        biolumLFO_.reset();

        tidalLFO_.setShape(StandardLFO::Triangle);
        tidalLFO_.setRate(0.003f, srF_); // Ultra-slow ecosystem tide
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
            voices_[v].prepare(srF_);
        }

        // Coupling state
        lastSampleL_ = lastSampleR_ = 0.0f;
        extFilterMod_ = 0.0f;
        extPitchMod_ = 0.0f;
        aftertouch_ = 0.0f;
        modWheel_ = 0.0f;
        pitchBendNorm_ = 0.0f;

        // Silence gate: 500ms (reverb-tail category due to canopy delay)
        prepareSilenceGate(sr_, maxBlockSize, 500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (int t = 0; t < kCanopyTaps; ++t)
        {
            std::fill(canopyBufL_[t].begin(), canopyBufL_[t].end(), 0.0f);
            std::fill(canopyBufR_[t].begin(), canopyBufR_[t].end(), 0.0f);
            canopyWritePos_[t] = 0;
        }
        subFilter_.reset();
        canopyLP_.reset();
        canopyLPR_.reset();
        breezeLP_.reset();
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
    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // 1. Parse MIDI
        for (const auto& meta : midi)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                wakeSilenceGate();

                // Entropy analysis: measure timing deviation from expected grid
                int64_t timeSinceLastNote = sampleCounter_ - lastNoteOnTime_;
                if (expectedInterval_ > 0)
                {
                    float deviation = std::fabs(static_cast<float>(timeSinceLastNote - expectedInterval_)) /
                                      static_cast<float>(expectedInterval_ + 1);
                    // Smooth entropy tracking
                    entropy_ = entropy_ * 0.85f + deviation * 0.15f;
                    entropy_ = clamp(entropy_, 0.0f, 1.0f);
                }
                expectedInterval_ = timeSinceLastNote;
                lastNoteOnTime_ = sampleCounter_;

                // Ecological memory: slop feeds health, perfection starves it
                ecologicalHealth_ = ecologicalHealth_ * 0.95f + entropy_ * 0.05f;
                ecologicalHealth_ = clamp(ecologicalHealth_, 0.05f, 1.0f);

                // Allocate voice (steal oldest if full)
                int slot = -1;
                for (int v = 0; v < kMaxVoices; ++v)
                {
                    if (!voices_[v].active)
                    {
                        slot = v;
                        break;
                    }
                }
                bool isSteal = (slot < 0);
                if (isSteal)
                {
                    // Voice stealing — Harmonic Culling:
                    // 1. Dump residual partial energy into the ASMR noise floor.
                    // 2. Mark the stolen voice for a 5ms crossfade so the outgoing
                    //    note fades out cleanly (no click) before the new note starts.
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
                    // Collect residual energy from all active partials
                    float stolenEnergy = 0.0f;
                    for (int p = 0; p < OasisRhodesToneGenerator::kNumPartials; ++p)
                        stolenEnergy += voices_[slot].tine.partialLevels[p];
                    stolenEnergy *= voices_[slot].ampEnvLevel;
                    culledEnergy_ += clamp(stolenEnergy * 0.8f, 0.0f, 1.0f);
                    culledEnergy_ = clamp(culledEnergy_, 0.0f, 3.0f);

                    // Crossfade: keep the voice sounding for 5ms while we set up
                    // the new note underneath — actual re-trigger happens after.
                    voices_[slot].crossfading = true;
                    voices_[slot].crossfadeCounter = voices_[slot].crossfadeSamples;
                }

                auto& voice = voices_[slot];

                if (!isSteal)
                {
                    // Clean allocation: reset all voice state
                    voice.active = true;
                    voice.releasing = false;
                    voice.crossfading = false;
                    voice.crossfadeCounter = 0;
                    voice.note = msg.getNoteNumber();
                    voice.velocity = msg.getFloatVelocity();
                    voice.phase = 0.0;
                    voice.amplitude = 1.0f;
                    voice.noteAge = 0;

                    float freq = 440.0f * std::pow(2.0f, (voice.note - 69) / 12.0f);
                    voice.phaseDelta = freq / sr_;
                    voice.glide.setTargetOrSnap(freq);

                    float bellAmount = pBellAmountParam_ ? pBellAmountParam_->load() : 0.5f;
                    float pickupPos = pPickupPosParam_ ? pPickupPosParam_->load() : 0.5f;
                    voice.pickup.setPickupPosition(pickupPos);
                    voice.tine.trigger(voice.velocity, bellAmount);
                    voice.ampEnv.trigger();
                    voice.filterEnv.trigger();
                }
                else
                {
                    // Steal: store the incoming note params; the crossfade loop in
                    // renderBlock will re-trigger the voice once the ramp completes.
                    // We use a small staging area on the voice itself.
                    // The crossfadeCounter will count down to 0, at which point
                    // the voice is re-triggered with these saved params.
                    voice.active = true;
                    voice.releasing = false;
                    // Save incoming note data into currentNote/startTime (re-purposed
                    // for this purpose — the new note will read them after fade).
                    voice.currentNote = msg.getNoteNumber();
                    // Re-use startTime as packed velocity (float → uint64 via cast)
                    float inVel = msg.getFloatVelocity();
                    uint32_t velBits;
                    std::memcpy(&velBits, &inVel, sizeof(velBits));
                    voice.startTime = static_cast<uint64_t>(velBits);
                    // Cache the frequency now (once) so the crossfade branch never
                    // calls std::pow per sample during the fade countdown.
                    voice.cachedRetriggerFreq = 440.0f * std::pow(2.0f, (voice.currentNote - 69) / 12.0f);
                }
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
                aftertouch_ =
                    msg.isAftertouch() ? msg.getAfterTouchValue() / 127.0f : msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheel_ = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
            {
                pitchBendNorm_ = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
            }
        }

        // 2. Silence gate bypass
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // 3. Read parameters (ParamSnapshot pattern)
        float pEntropySens = pEntropySensParam_ ? pEntropySensParam_->load() : 0.5f;
        float pBreeze = pBreezeParam_ ? pBreezeParam_->load() : 0.2f;
        float pSwarmDensity = pSwarmDensityParam_ ? pSwarmDensityParam_->load() : 0.4f;
        float pLagoonDepth = pLagoonDepthParam_ ? pLagoonDepthParam_->load() : 0.3f;
        float pSubDrive = pSubDriveParam_ ? pSubDriveParam_->load() : 0.3f;
        float pResonatorQ = pResonatorQParam_ ? pResonatorQParam_->load() : 4.0f;
        float pCullDecay = pCullDecayParam_ ? pCullDecayParam_->load() : 0.995f;
        float pCanopyFB = pCanopyFBParam_ ? pCanopyFBParam_->load() : 0.4f;
        float pFilterCutoff = pFilterCutoffParam_ ? pFilterCutoffParam_->load() : 2000.0f;
        float pFilterEnvAmt = pFilterEnvAmtParam_ ? pFilterEnvAmtParam_->load() : 0.5f;

        // Rhodes params
        float pRhodesMix = pRhodesMixParam_ ? pRhodesMixParam_->load() : 1.0f;
        float pPickupPos = pPickupPosParam_ ? pPickupPosParam_->load() : 0.5f;
        float pWarmth = pWarmthParam_ ? pWarmthParam_->load() : 0.3f;
        float pTremoloDepth = pTremoloDepthParam_ ? pTremoloDepthParam_->load() : 0.0f;
        float pTremoloRate = pTremoloRateParam_ ? pTremoloRateParam_->load() : 5.0f;
        float pGlideTime = pGlideTimeParam_ ? pGlideTimeParam_->load() : 0.0f;
        float pBellAmount = pBellAmountParam_ ? pBellAmountParam_->load() : 0.5f;

        // Macros
        float pM1 = pMacroCharacterParam_ ? pMacroCharacterParam_->load() : 0.0f;
        float pM2 = pMacroMovementParam_ ? pMacroMovementParam_->load() : 0.0f;
        float pM3 = pMacroCouplingParam_ ? pMacroCouplingParam_->load() : 0.0f;
        float pM4 = pMacroSpaceParam_ ? pMacroSpaceParam_->load() : 0.0f;

        // Macro → parameter mapping
        // M1 CHARACTER → Rhodes mix (more character = more Rhodes blend vs sub-bass)
        pRhodesMix = clamp(pRhodesMix + pM1 * 0.4f, 0.0f, 1.0f);
        // M1 also nudges entropy sensitivity and sub drive
        pEntropySens = clamp(pEntropySens + pM1 * 0.3f, 0.0f, 1.0f);
        pSubDrive = clamp(pSubDrive + pM1 * 0.2f, 0.0f, 1.0f);

        // M2 MOVEMENT → pickup position (brightness) + breeze
        pPickupPos = clamp(pPickupPos + pM2 * 0.5f, 0.0f, 1.0f);
        pBreeze = clamp(pBreeze + pM2 * 0.3f, 0.0f, 1.0f);

        // M3 COUPLING → swarm density
        pSwarmDensity = clamp(pSwarmDensity + pM3 * 0.5f, 0.0f, 1.0f);

        // M4 SPACE → lagoon depth (comb filter feedback)
        pLagoonDepth = clamp(pLagoonDepth + pM4 * 0.5f, 0.0f, 1.0f);

        // Expression: aftertouch → warmth (D006: velocity → timbre + CC → timbre)
        pWarmth = clamp(pWarmth + aftertouch_ * 0.5f, 0.0f, 1.0f);
        // Aftertouch also nudges entropy sensitivity
        pEntropySens = clamp(pEntropySens + aftertouch_ * 0.2f, 0.0f, 1.0f);
        // Mod wheel → canopy feedback / swarm (D006)
        pCanopyFB = clamp(pCanopyFB + modWheel_ * 0.4f, 0.0f, 0.95f);

        // Coupling modulation
        pFilterCutoff = clamp(pFilterCutoff + extFilterMod_, 60.0f, 16000.0f);
        // extPitchMod_ (AmpToPitch coupling): treat ±1 as ±2 semitones, same range
        // as pitchBendNorm_. Compute the combined bend ratio once per block.
        float couplingPitchRatio = PitchBendUtil::semitonesToFreqRatio(extPitchMod_ * 2.0f);

        // Effective entropy (sensitivity-scaled)
        float ent = entropy_ * pEntropySens;

        // Update resonator bank frequency based on ecological health
        float resBankFreq = 200.0f + ecologicalHealth_ * 800.0f;
        resonatorBank_.setCoefficients(resBankFreq, pResonatorQ, srF_);

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            sampleCounter_++;

            // LFO modulation
            float biolumMod = biolumLFO_.process();
            float tidalMod = tidalLFO_.process();

            // Pitch bend ratio for this sample
            float bendRatio = PitchBendUtil::semitonesToFreqRatio(pitchBendNorm_ * 2.0f);

            // === VOICE SYNTHESIS ===
            float subMix = 0.0f;
            float canopySendL = 0.0f;
            float canopySendR = 0.0f;

            for (int v = 0; v < kMaxVoices; ++v)
            {
                auto& voice = voices_[v];
                if (!voice.active)
                    continue;

                // === CROSSFADE: stolen voice counting down ===
                // While crossfading, the outgoing note plays with a linear fade-out
                // gain. When the counter expires, we re-trigger the new note data
                // that was stored in voice.currentNote / voice.startTime.
                if (voice.crossfading)
                {
                    float xfGain = static_cast<float>(voice.crossfadeCounter) /
                                   static_cast<float>(std::max(voice.crossfadeSamples, 1));

                    voice.noteAge++;
                    voice.ampEnvLevel = voice.ampEnv.process();
                    float voiceAmp = voice.ampEnvLevel * voice.velocity * xfGain;
                    voiceAmp = flushDenormal(voiceAmp);

                    voice.glide.setTime(pGlideTime, srF_);
                    float glidedFreq = voice.glide.process() * bendRatio * couplingPitchRatio;
                    glidedFreq = clamp(glidedFreq, 20.0f, srF_ * 0.49f);
                    voice.phaseDelta = static_cast<double>(glidedFreq) / sr_;
                    voice.phase += voice.phaseDelta;
                    if (voice.phase >= 1.0)
                        voice.phase -= 1.0;
                    float phaseF = static_cast<float>(voice.phase) * 6.28318530718f;

                    voice.tine.bellAmountLive = pBellAmount;
                    float rhodesOut = voice.tine.process(glidedFreq, voice.pickup.partialGains);
                    rhodesOut = voice.pickup.process(rhodesOut, pPickupPos);
                    rhodesOut = voice.amp.process(rhodesOut, pWarmth, voice.velocity);

                    float rawSub = fastSin(phaseF) + 0.5f * fastSin(phaseF * 0.5f);
                    float sagAmount = (1.0f - ent) * voice.velocity;
                    float granularOut = fastTanh(rawSub * (1.0f + sagAmount * pSubDrive * 4.0f));
                    float blended = rhodesOut * pRhodesMix + granularOut * (1.0f - pRhodesMix);

                    voice.tremoloLFO.setRate(pTremoloRate, srF_);
                    blended *= (1.0f + voice.tremoloLFO.process() * pTremoloDepth * 0.5f);

                    voice.filterEnvLevel = voice.filterEnv.process();
                    float velBright = 0.5f + voice.velocity * 0.5f;
                    float envCutoff = clamp(pFilterCutoff * velBright + pFilterEnvAmt * voice.filterEnvLevel * 6000.0f,
                                            60.0f, srF_ * 0.49f);
                    voice.svf.setCoefficients_fast(envCutoff, 0.7f, srF_);
                    blended = voice.svf.processSample(blended);

                    float mycelialSend = blended * ent * pEntropySens;
                    float directOut = blended * (1.0f - ent * 0.6f) * voiceAmp;
                    subMix += directOut;
                    float pan = static_cast<float>(v) / static_cast<float>(kMaxVoices - 1);
                    canopySendL += mycelialSend * (1.0f - pan) * voiceAmp;
                    canopySendR += mycelialSend * pan * voiceAmp;

                    voice.crossfadeCounter--;
                    if (voice.crossfadeCounter <= 0)
                    {
                        // Crossfade complete — re-trigger with saved note data
                        voice.crossfading = false;
                        voice.crossfadeCounter = 0;
                        voice.note = voice.currentNote;
                        uint32_t velBits = static_cast<uint32_t>(voice.startTime & 0xFFFFFFFF);
                        std::memcpy(&voice.velocity, &velBits, sizeof(float));
                        voice.phase = 0.0;
                        voice.noteAge = 0;

                        // Use frequency cached at steal time — avoids std::pow per sample.
                        float freq = voice.cachedRetriggerFreq;
                        voice.phaseDelta = freq / sr_;
                        voice.glide.setTargetOrSnap(freq);

                        voice.pickup.setPickupPosition(pPickupPos);
                        voice.tine.trigger(voice.velocity, pBellAmount);
                        voice.ampEnv.trigger();
                        voice.filterEnv.trigger();
                    }
                    continue;
                }

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
                voiceAmp = flushDenormal(voiceAmp);

                // === GLIDE — derive current frequency from glide processor ===
                // couplingPitchRatio applies AmpToPitch coupling (extPitchMod_) as a
                // frequency multiplier — computed once per block before the voice loop.
                voice.glide.setTime(pGlideTime, srF_);
                float glidedFreq = voice.glide.process() * bendRatio * couplingPitchRatio;
                glidedFreq = clamp(glidedFreq, 20.0f, srF_ * 0.49f);

                // Phase accumulation (kept for sub-bass oscillator compatibility)
                voice.phaseDelta = static_cast<double>(glidedFreq) / sr_;
                voice.phase += voice.phaseDelta;
                if (voice.phase >= 1.0)
                    voice.phase -= 1.0;

                float phaseF = static_cast<float>(voice.phase) * 6.28318530718f;

                // === RHODES SIGNAL CHAIN ===
                // 1. Tine generator: stretched-harmonic physical model with per-partial
                //    pickup gains (set at note-on via pickup.setPickupPosition()).
                // Update bellAmountLive each block so held notes respond to knob changes.
                voice.tine.bellAmountLive = pBellAmount;
                float rhodesOut = voice.tine.process(glidedFreq, voice.pickup.partialGains);

                // 2. Pickup model: presence filter (tip=bright, base=warm).
                //    Per-partial shaping already applied inside tine.process().
                rhodesOut = voice.pickup.process(rhodesOut, pPickupPos);

                // 3. Amp stage: tube preamp warmth + velocity-dependent asymmetric bark.
                rhodesOut = voice.amp.process(rhodesOut, pWarmth, voice.velocity);

                // === SUB-BASS / GRANULAR OSCILLATOR ===
                // Power supply sag: low entropy → more sag → warmer distortion
                float rawSub = fastSin(phaseF) + 0.5f * fastSin(phaseF * 0.5f);
                float sagAmount = (1.0f - ent) * voice.velocity;
                float granularOut = fastTanh(rawSub * (1.0f + sagAmount * pSubDrive * 4.0f));

                // === BLEND ===
                // pRhodesMix = 1.0 → full Rhodes | pRhodesMix = 0.0 → all sub-bass.
                // Default is 1.0 so the engine produces its Rhodes character out of the box.
                float blendedSub = rhodesOut * pRhodesMix + granularOut * (1.0f - pRhodesMix);

                // === TREMOLO (per-voice LFO, D002/D005) ===
                // Update tremolo LFO rate per block (pTremoloRate changes are block-rate).
                voice.tremoloLFO.setRate(pTremoloRate, srF_);
                float tremoloMod = voice.tremoloLFO.process();
                // Bipolar modulation: pTremoloDepth != 0 activates tremolo (handles negative too).
                float tremoloScale = 1.0f + tremoloMod * pTremoloDepth * 0.5f;
                blendedSub *= tremoloScale;

                // === FILTER ENVELOPE + PER-VOICE SVF (D001 velocity → timbre) ===
                voice.filterEnvLevel = voice.filterEnv.process();
                // Velocity → filter brightness (D001): high velocity opens the filter.
                float velBright = 0.5f + voice.velocity * 0.5f;
                float envCutoff = pFilterCutoff * velBright + pFilterEnvAmt * voice.filterEnvLevel * 6000.0f;
                envCutoff = clamp(envCutoff, 60.0f, srF_ * 0.49f);
                voice.svf.setCoefficients_fast(envCutoff, 0.7f, srF_);
                blendedSub = voice.svf.processSample(blendedSub);

                // === ECOLOGICAL MEMORY — entropy wires into per-voice timbre ===
                // High entropy (sloppy playing) → more harmonic richness splinters into
                // the canopy. The biolumMod modulates the direct level so imperfect
                // playing literally makes each voice shimmer more (bioluminescence).
                float biolumScale = 1.0f + ecologicalHealth_ * biolumMod * 0.25f;
                blendedSub *= biolumScale;

                // Mycelial Morphing: entropy splinters signal into canopy delay
                float mycelialSend = blendedSub * ent * pEntropySens;
                float directOut = blendedSub * (1.0f - ent * 0.6f) * voiceAmp;

                subMix += directOut;

                // Stereo canopy send with voice spread
                float pan = static_cast<float>(v) / static_cast<float>(kMaxVoices - 1);
                canopySendL += mycelialSend * (1.0f - pan) * voiceAmp;
                canopySendR += mycelialSend * pan * voiceAmp;
            }

            // Sub-bass filter (with velocity-driven cutoff modulation)
            float subCutoff = clamp(pFilterCutoff + pFilterEnvAmt * std::fabs(subMix) * 2000.0f + biolumMod * 400.0f,
                                    60.0f, 16000.0f);
            subFilter_.setCoefficients_fast(subCutoff, 0.707f, srF_);
            float filteredSub = subFilter_.processSample(subMix);

            // Resonator bank (entropy-modulated)
            float resSample = resonatorBank_.processSample(filteredSub) * ent * 0.5f;

            // === CANOPY DELAY NETWORK (Granular Swarm) ===
            float canopyOutL = 0.0f;
            float canopyOutR = 0.0f;
            int activeTaps =
                std::max(1, 1 + static_cast<int>(pSwarmDensity *
                                                 (kCanopyTaps - 1))); // #606: guard NaN/negative from atomic param

            for (int t = 0; t < activeTaps; ++t)
            {
                // Read from delay
                int readPos = (canopyWritePos_[t] - canopyDelaySamples_[t] + canopySize_[t]) % canopySize_[t];
                float tapL = canopyBufL_[t][static_cast<size_t>(readPos)];
                float tapR = canopyBufR_[t][static_cast<size_t>(readPos)];

                // Lagoon: comb filter with feedback (watery resonance).
                // Both lagoonDepth and canopyFB shape the delay feedback.
                // Entropy also drives feedback: sloppy playing thickens the swarm.
                // Per CLAUDE.md: feedback path clamped ≤ 0.95 to prevent instability.
                float lagoonFB =
                    clamp(pLagoonDepth * 0.7f + pCanopyFB * 0.1f + ent * pEntropySens * 0.15f, 0.0f, 0.95f);
                float feedL = canopySendL + tapL * lagoonFB;
                float feedR = canopySendR + tapR * lagoonFB;

                // Write to delay
                canopyBufL_[t][static_cast<size_t>(canopyWritePos_[t])] = flushDenormal(feedL);
                canopyBufR_[t][static_cast<size_t>(canopyWritePos_[t])] = flushDenormal(feedR);
                canopyWritePos_[t] = (canopyWritePos_[t] + 1) % canopySize_[t];

                // Tidal drift modulates tap contribution
                float tapGain = 1.0f / static_cast<float>(activeTaps);
                tapGain *= (1.0f + tidalMod * 0.2f);
                canopyOutL += tapL * tapGain;
                canopyOutR += tapR * tapGain;
            }

            // Canopy lowpass — independent L and R to produce genuine stereo depth.
            // The ecology drives the cutoff: more health = brighter canopy (blooming).
            // Entropy modulates the cutoff amount so imperfect playing literally
            // opens the canopy filter (bioluminescent growth in the delay network).
            float canopyCutoff =
                3000.0f + ecologicalHealth_ * 9000.0f + ent * pEntropySens * 4000.0f + biolumMod * 1500.0f;
            canopyCutoff = clamp(canopyCutoff, 200.0f, srF_ * 0.49f);
            canopyLP_.setCoefficients_fast(canopyCutoff, 0.5f, srF_);
            canopyOutL = canopyLP_.processSample(canopyOutL);

            // Right channel gets a slightly different cutoff for spatial width
            float canopyCutoffR = clamp(canopyCutoff * (1.0f - tidalMod * 0.08f), 200.0f, srF_ * 0.49f);
            canopyLPR_.setCoefficients_fast(canopyCutoffR, 0.5f, srF_);
            canopyOutR = canopyLPR_.processSample(canopyOutR);

            // === ASMR BREEZE (Harmonic Culling noise floor) ===
            // White noise scaled by culledEnergy_ + breeze param, then low-pass
            // filtered at ~2kHz (one-pole). This turns the stolen-voice energy
            // into a soft, organic "whoosh" — the dying resonance of the culled
            // tine, not harsh white noise.
            noiseRng_ ^= noiseRng_ << 13;
            noiseRng_ ^= noiseRng_ >> 17;
            noiseRng_ ^= noiseRng_ << 5;
            float noise = (static_cast<float>(noiseRng_ & 0xFFFF) / 32768.0f - 1.0f);
            float rawBreezeL = noise * (pBreeze * 0.1f + culledEnergy_ * 0.35f) * (1.0f + biolumMod * 0.3f);

            noiseRng_ ^= noiseRng_ << 13;
            noiseRng_ ^= noiseRng_ >> 17;
            noiseRng_ ^= noiseRng_ << 5;
            float noiseR = (static_cast<float>(noiseRng_ & 0xFFFF) / 32768.0f - 1.0f);
            float rawBreezeR = noiseR * (pBreeze * 0.1f + culledEnergy_ * 0.35f) * (1.0f - biolumMod * 0.3f);

            // Apply the culling LP filter (2kHz — gives the stolen-voice warmth)
            // The breeze LP is stereo via processSample(L) then processSample(R)
            // using the same single filter (mono LP tracks the signal envelope).
            float breezeFiltered = breezeLP_.processSample((rawBreezeL + rawBreezeR) * 0.5f);
            float breezeL = rawBreezeL * 0.3f + breezeFiltered * 0.7f;
            float breezeR = rawBreezeR * 0.3f + breezeFiltered * 0.7f;

            // Decay culled energy
            culledEnergy_ *= pCullDecay;
            culledEnergy_ = flushDenormal(culledEnergy_);

            // === FINAL MIX ===
            float sampleL = filteredSub + resSample + canopyOutL * pSwarmDensity + breezeL;
            float sampleR = filteredSub + resSample + canopyOutR * pSwarmDensity + breezeR;

            // Cache for coupling
            lastSampleL_ = sampleL;
            lastSampleR_ = sampleR;

            // Accumulate into buffer
            outL[i] += sampleL;
            if (outR)
                outR[i] += sampleR;
        }

        // Reset coupling mods
        extFilterMod_ = 0.0f;
        extPitchMod_ = 0.0f;

        // Silence gate analysis
        analyzeForSilenceGate(buffer, numSamples);
    }

    //-- Coupling ---------------------------------------------------------------
    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? lastSampleL_ : lastSampleR_;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int /*numSamples*/) override
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
            if (sourceBuffer)
                extFilterMod_ += sourceBuffer[0] * amount * 1000.0f;
            break;
        default:
            break;
        }
    }

    //-- Parameters -------------------------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;

        // Core ecosystem params
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_entropySens", 1}, "Entropy Sensitivity",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_breeze", 1}, "Breeze",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_swarmDensity", 1}, "Swarm Density",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_lagoonDepth", 1}, "Lagoon Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Sub-bass shaping
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_subDrive", 1}, "Sub Drive",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_resonatorQ", 1}, "Resonator Q",
                                              juce::NormalisableRange<float>(0.5f, 20.0f, 0.0f, 0.4f), 4.0f));

        // Canopy network
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_cullDecay", 1}, "Cull Decay",
                                              juce::NormalisableRange<float>(0.9f, 0.9999f), 0.995f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_canopyFB", 1}, "Canopy Feedback",
                                              juce::NormalisableRange<float>(0.0f, 0.95f), 0.4f));

        // Filter
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_filterCutoff", 1}, "Filter Cutoff",
                                              juce::NormalisableRange<float>(60.0f, 16000.0f, 0.0f, 0.3f), 2000.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_filterEnvAmt", 1}, "Filter Env Amount",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Rhodes voice parameters
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_rhodesMix", 1}, "Rhodes Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f)); // default: all Rhodes

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_pickupPos", 1}, "Pickup Position",
                                              juce::NormalisableRange<float>(0.0f, 1.0f),
                                              0.5f)); // centre = balanced tone

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_bellAmount", 1}, "Bell Amount",
                                              juce::NormalisableRange<float>(0.0f, 1.0f),
                                              0.5f)); // 3rd-partial bell burst

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_warmth", 1}, "Warmth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f),
                                              0.3f)); // amp-stage warmth/bark

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_tremoloDepth", 1}, "Tremolo Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f)); // off by default

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_tremoloRate", 1}, "Tremolo Rate",
                                              juce::NormalisableRange<float>(0.1f, 10.0f, 0.0f, 0.5f), 5.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_glideTime", 1}, "Glide Time",
                                              juce::NormalisableRange<float>(0.0f, 2.0f, 0.0f, 0.3f), 0.0f));

        // Macros (CHARACTER / MOVEMENT / COUPLING / SPACE)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_macroCharacter", 1}, "Character",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_macroMovement", 1}, "Movement",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_macroCoupling", 1}, "Coupling",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oas_macroSpace", 1}, "Space",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pEntropySensParam_ = apvts.getRawParameterValue("oas_entropySens");
        pBreezeParam_ = apvts.getRawParameterValue("oas_breeze");
        pSwarmDensityParam_ = apvts.getRawParameterValue("oas_swarmDensity");
        pLagoonDepthParam_ = apvts.getRawParameterValue("oas_lagoonDepth");
        pSubDriveParam_ = apvts.getRawParameterValue("oas_subDrive");
        pResonatorQParam_ = apvts.getRawParameterValue("oas_resonatorQ");
        pCullDecayParam_ = apvts.getRawParameterValue("oas_cullDecay");
        pCanopyFBParam_ = apvts.getRawParameterValue("oas_canopyFB");
        pFilterCutoffParam_ = apvts.getRawParameterValue("oas_filterCutoff");
        pFilterEnvAmtParam_ = apvts.getRawParameterValue("oas_filterEnvAmt");
        pMacroCharacterParam_ = apvts.getRawParameterValue("oas_macroCharacter");
        pMacroMovementParam_ = apvts.getRawParameterValue("oas_macroMovement");
        pMacroCouplingParam_ = apvts.getRawParameterValue("oas_macroCoupling");
        pMacroSpaceParam_ = apvts.getRawParameterValue("oas_macroSpace");

        // Rhodes voice params
        pRhodesMixParam_ = apvts.getRawParameterValue("oas_rhodesMix");
        pPickupPosParam_ = apvts.getRawParameterValue("oas_pickupPos");
        pBellAmountParam_ = apvts.getRawParameterValue("oas_bellAmount");
        pWarmthParam_ = apvts.getRawParameterValue("oas_warmth");
        pTremoloDepthParam_ = apvts.getRawParameterValue("oas_tremoloDepth");
        pTremoloRateParam_ = apvts.getRawParameterValue("oas_tremoloRate");
        pGlideTimeParam_ = apvts.getRawParameterValue("oas_glideTime");
    }

private:
    //--------------------------------------------------------------------------
    static constexpr int kMaxVoices = 8;
    static_assert(kMaxVoices > 1, "Oasis pan spread divides by kMaxVoices-1");
    static constexpr int kCanopyTaps = 6;

    double sr_ = 44100.0;
    float srF_ = 44100.0f;
    int blockSize_ = 512;

    // Voices
    OasisVoice voices_[kMaxVoices];

    // Canopy delay network
    std::vector<float> canopyBufL_[kCanopyTaps];
    std::vector<float> canopyBufR_[kCanopyTaps];
    int canopySize_[kCanopyTaps]{};
    int canopyWritePos_[kCanopyTaps]{};
    int canopyDelaySamples_[kCanopyTaps]{};

    // Filters
    CytomicSVF subFilter_;
    CytomicSVF canopyLP_;  // Canopy smoothing LP (left channel)
    CytomicSVF canopyLPR_; // Canopy smoothing LP (right channel — independent state)
    CytomicSVF resonatorBank_;
    CytomicSVF breezeLP_; // Harmonic Culling: LP at ~2kHz for ASMR noise floor

    // LFOs
    StandardLFO biolumLFO_;
    StandardLFO tidalLFO_;

    // Entropy / Ecological state
    float entropy_ = 0.0f;
    float ecologicalHealth_ = 0.5f;
    float culledEnergy_ = 0.0f;
    int64_t lastNoteOnTime_ = 0;
    int expectedInterval_ = 0;
    int64_t sampleCounter_ = 0;
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
    std::atomic<float>* pEntropySensParam_ = nullptr;
    std::atomic<float>* pBreezeParam_ = nullptr;
    std::atomic<float>* pSwarmDensityParam_ = nullptr;
    std::atomic<float>* pLagoonDepthParam_ = nullptr;
    std::atomic<float>* pSubDriveParam_ = nullptr;
    std::atomic<float>* pResonatorQParam_ = nullptr;
    std::atomic<float>* pCullDecayParam_ = nullptr;
    std::atomic<float>* pCanopyFBParam_ = nullptr;
    std::atomic<float>* pFilterCutoffParam_ = nullptr;
    std::atomic<float>* pFilterEnvAmtParam_ = nullptr;
    std::atomic<float>* pMacroCharacterParam_ = nullptr;
    std::atomic<float>* pMacroMovementParam_ = nullptr;
    std::atomic<float>* pMacroCouplingParam_ = nullptr;
    std::atomic<float>* pMacroSpaceParam_ = nullptr;

    // Rhodes voice params
    std::atomic<float>* pRhodesMixParam_ = nullptr;
    std::atomic<float>* pPickupPosParam_ = nullptr;
    std::atomic<float>* pBellAmountParam_ = nullptr;
    std::atomic<float>* pWarmthParam_ = nullptr;
    std::atomic<float>* pTremoloDepthParam_ = nullptr;
    std::atomic<float>* pTremoloRateParam_ = nullptr;
    std::atomic<float>* pGlideTimeParam_ = nullptr;

    //--------------------------------------------------------------------------
    static float clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
};

} // namespace xoceanus
