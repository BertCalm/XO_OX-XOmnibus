// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/FilterEnvelope.h"
#include "../../DSP/ParameterSmoother.h"
#include "../../DSP/FastMath.h"
#include <cmath>
#include <array>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// OsmosisEngine — the membrane between XOceanus and the outside world.
//
// Receives external audio via setExternalInput(). Analyzes envelope, pitch,
// and spectral content. Produces coupling signals that let other engines
// respond to the outside world.
//
// Does not generate its own sound — like Optic (B005), its primary output
// is coupling data, not audio. Wet/dry blend passes external audio through
// the membrane with subtle coloring (one-pole filter at membrane frequency,
// LFO-modulated).
//
// 4-band split via CytomicSVF crossovers at 200 Hz, 1 kHz, 5 kHz:
//   Band 0 — sub  (<200 Hz)     — LP200
//   Band 1 — lo-mid (200-1k Hz) — HP200 → LP1k
//   Band 2 — mid  (1k-5k Hz)    — HP1k  → LP5k
//   Band 3 — high (>5 kHz)      — HP5k
//
// Pitch detection via autocorrelation on a 2048-sample circular buffer
// (50–2000 Hz range, reliable on polyphonic content).
//
// LFO modulates the membrane filter cutoff:
//   effectiveCutoff = baseCutoff * (1.0f + lfoValue * lfoDepth)
//
// Macros: PERMEABILITY (wet/dry), SELECTIVITY (filter Q/cutoff), REACTIVITY (env speed), MEMORY (decay)
// Param prefix: osmo_
// Accent: Surface Tension Silver #C0C0C0
//
class OsmosisEngine : public SynthEngine
{
public:
    OsmosisEngine() = default;

    //-- Identity --------------------------------------------------------------
    juce::String getEngineId() const override { return "Osmosis"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFC0C0C0); }
    int getMaxVoices() const override { return 1; }         // mono analysis engine
    bool isAnalysisEngine() const override { return true; } // enables external audio routing without RTTI

    //-- Lifecycle -------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr_ = sampleRate;
        blockSize_ = maxBlockSize;

        // Envelope follower — one-pole with variable attack/release
        envFollowerL_ = 0.0f;
        envFollowerR_ = 0.0f;

        // Pitch detection — autocorrelation
        acBufPos_ = 0;
        acBuffer_.fill(0.0f);
        detectedPitch_ = 0.0f;
        pitchUpdateCounter_ = 0;

        // Spectral bands
        for (int i = 0; i < 4; ++i)
            bandRMS_[i] = 0.0f;

        // Band-split filters
        setupBandFilters();

        // Coupling output cache
        couplingSampleL_ = 0.0f;
        couplingSampleR_ = 0.0f;

        // Smoothers for macro parameters
        permeabilitySmoother_.prepare(static_cast<float>(sr_), 5.0f);
        selectivitySmoother_.prepare(static_cast<float>(sr_), 5.0f);
        reactivitySmoother_.prepare(static_cast<float>(sr_), 5.0f);
        memorySmoother_.prepare(static_cast<float>(sr_), 5.0f);

        // Membrane filter (one-pole LP for coloring pass-through audio)
        membraneLPCoeff_ = 0.0f;
        membraneLPStateL_ = 0.0f;
        membraneLPStateR_ = 0.0f;

        // LFO for membrane cutoff modulation
        lfo_.setShape(StandardLFO::Shape::Sine);
        lfo_.setRate(0.5f, static_cast<float>(sr_));
        lfo_.reset();

        // Invalidate cached envelope-follower coefficients so they are
        // recomputed on the first block after a sample-rate change.
        cachedReactivity_ = -1.0f;
        cachedMemory_     = -1.0f;

        prepareSilenceGate(sr_, maxBlockSize, 500.0f);

        externalBufferL_ = nullptr;
        externalBufferR_ = nullptr;
        externalNumSamples_ = 0;
    }

    void releaseResources() override {}

    void reset() override
    {
        envFollowerL_ = 0.0f;
        envFollowerR_ = 0.0f;
        acBufPos_ = 0;
        acBuffer_.fill(0.0f);
        detectedPitch_ = 0.0f;
        pitchUpdateCounter_ = 0;
        for (int i = 0; i < 4; ++i)
            bandRMS_[i] = 0.0f;
        membraneLPStateL_ = 0.0f;
        membraneLPStateR_ = 0.0f;
        couplingPermeabilityOffset_ = 0.0f; // fix #303
        lfo_.reset();
        for (auto& f : bandLP_)
            f.reset();
        for (auto& f : bandHP_)
            f.reset();
        for (auto& f : bandLP2_)
            f.reset();
        for (auto& f : bandHP2_)
            f.reset();
    }

    //-- External audio injection (called by XOceanusProcessor) ----------------
    void setExternalInput(const float* left, const float* right, int numSamples)
    {
        externalBufferL_ = left;
        externalBufferR_ = right;
        externalNumSamples_ = numSamples;
    }

    //-- Analysis getters (for coupling and UI) --------------------------------
    float getEnvelopeLevel() const { return (envFollowerL_ + envFollowerR_) * 0.5f; }
    float getDetectedPitch() const { return detectedPitch_; }
    const float* getBandRMS() const { return bandRMS_; }

    //-- Audio -----------------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // SilenceGate: early-out when no external audio is present
        if (isSilenceGateBypassed())
        {
            buffer.clear();
            return;
        }

        // Read macro parameters
        permeabilitySmoother_.set(pPermeability_ ? pPermeability_->load() : 0.5f);
        selectivitySmoother_.set(pSelectivity_ ? pSelectivity_->load() : 0.5f);
        reactivitySmoother_.set(pReactivity_ ? pReactivity_->load() : 0.5f);
        memorySmoother_.set(pMemory_ ? pMemory_->load() : 0.3f);

        // Fix #303: add coupling-driven offset to base permeability, clamped to [0, 1]
        const float permeability =
            juce::jlimit(0.0f, 1.0f, permeabilitySmoother_.process() + couplingPermeabilityOffset_);
        const float selectivity = selectivitySmoother_.process();
        const float reactivity = reactivitySmoother_.process();
        const float memory = memorySmoother_.process();

        // Envelope follower coefficients from reactivity — cached to avoid
        // std::exp on the audio thread every block.  Recompute only when the
        // smoothed reactivity or memory value has drifted by more than 0.002
        // (≈ one 7-bit MIDI step), which is inaudible for these slow time-constants.
        const float attackMs = 1.0f + (1.0f - reactivity) * 49.0f; // 1–50 ms
        const float releaseMs = 10.0f + memory * 990.0f;           // 10–1000 ms
        if (std::abs(reactivity - cachedReactivity_) > 0.002f ||
            std::abs(memory    - cachedMemory_)     > 0.002f)
        {
            cachedAttackCoeff_  = std::exp(-1.0f / (static_cast<float>(sr_) * attackMs  * 0.001f));
            cachedReleaseCoeff_ = std::exp(-1.0f / (static_cast<float>(sr_) * releaseMs * 0.001f));
            cachedReactivity_   = reactivity;
            cachedMemory_       = memory;
        }
        const float attackCoeff  = cachedAttackCoeff_;
        const float releaseCoeff = cachedReleaseCoeff_;

        // Membrane filter base frequency from selectivity (200 Hz – 18.2 kHz)
        const float membraneBaseFreq = 200.0f + selectivity * 18000.0f;

        // LFO depth is fixed at 0.3 (±30% cutoff excursion) — tasteful membrane breathing
        constexpr float kLfoDepth = 0.3f;

        auto* outL = buffer.getWritePointer(0);
        auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

        // Band RMS accumulators for this block
        float bandSumSq[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        int bandCount = 0;

        // ---- Compute membrane LP coefficient once at block-rate ----
        // LFO rate is low (≤~5 Hz); re-computing the one-pole coefficient
        // every sample via std::exp is the #1 CPU hot-spot in this engine.
        // Using fastExp with the LFO's current phase value gives imperceptible
        // difference while removing a transcendental from the per-sample path.
        {
            const float effPhase = lfo_.phase + lfo_.phaseOffset;
            const float lfoValBlock = fastSin((effPhase >= 1.0f ? effPhase - 1.0f : effPhase) * 6.28318530718f);
            const float effectiveCutoffBlock = membraneBaseFreq * (1.0f + lfoValBlock * kLfoDepth);
            membraneLPCoeff_ = fastExp(-2.0f * 3.14159265f * effectiveCutoffBlock / static_cast<float>(sr_));
        }

        for (int i = 0; i < numSamples; ++i)
        {
            // Read external input (or silence if none)
            const float inL = (externalBufferL_ && i < externalNumSamples_) ? externalBufferL_[i] : 0.0f;
            const float inR = (externalBufferR_ && i < externalNumSamples_) ? externalBufferR_[i] : 0.0f;
            const float mono = (inL + inR) * 0.5f;

            // ---- Envelope follower ----
            const float absL = std::fabs(inL);
            const float absR = std::fabs(inR);
            envFollowerL_ = (absL > envFollowerL_) ? attackCoeff * envFollowerL_ + (1.0f - attackCoeff) * absL
                                                   : releaseCoeff * envFollowerL_ + (1.0f - releaseCoeff) * absL;
            envFollowerR_ = (absR > envFollowerR_) ? attackCoeff * envFollowerR_ + (1.0f - attackCoeff) * absR
                                                   : releaseCoeff * envFollowerR_ + (1.0f - releaseCoeff) * absR;
            envFollowerL_ = (std::abs(envFollowerL_) < 1e-18f) ? 0.0f : envFollowerL_;
            envFollowerR_ = (std::abs(envFollowerR_) < 1e-18f) ? 0.0f : envFollowerR_;

            // ---- Autocorrelation buffer — push mono sample ----
            acBuffer_[acBufPos_] = mono;
            acBufPos_ = (acBufPos_ + 1) & (kAcBufSize - 1); // power-of-2 mask

            // ---- 4-band split (left channel drives band RMS; stereo pass uses both) ----
            // Band 0: LP200 (sub)
            const float b0 = bandLP_[0].processSample(bandLP_[1].processSample(inL));

            // Band 1: HP200 → LP1k (lo-mid)
            const float hp200 = bandHP_[0].processSample(bandHP_[1].processSample(inL));
            const float b1 = bandLP2_[0].processSample(bandLP2_[1].processSample(hp200));

            // Band 2: HP1k → LP5k (mid)
            const float hp1k = bandHP2_[0].processSample(bandHP2_[1].processSample(inL));
            const float b2 = bandLP3_[0].processSample(bandLP3_[1].processSample(hp1k));

            // Band 3: HP5k (high)
            const float b3 = bandHP3_[0].processSample(bandHP3_[1].processSample(inL));

            bandSumSq[0] += b0 * b0;
            bandSumSq[1] += b1 * b1;
            bandSumSq[2] += b2 * b2;
            bandSumSq[3] += b3 * b3;
            ++bandCount;

            // ---- LFO → advance phase per-sample (coefficient computed at block-rate above) ----
            lfo_.process(); // advance LFO state; membraneLPCoeff_ is refreshed at block-rate

            // ---- Membrane filter (one-pole LP for pass-through coloring) ----
            // Also run right channel through same coefficient
            const float inR_forLP = (externalBufferR_ && i < externalNumSamples_) ? externalBufferR_[i] : 0.0f;
            membraneLPStateL_ = membraneLPCoeff_ * membraneLPStateL_ + (1.0f - membraneLPCoeff_) * inL;
            membraneLPStateR_ = membraneLPCoeff_ * membraneLPStateR_ + (1.0f - membraneLPCoeff_) * inR_forLP;
            membraneLPStateL_ = (std::abs(membraneLPStateL_) < 1e-18f) ? 0.0f : membraneLPStateL_;
            membraneLPStateR_ = (std::abs(membraneLPStateR_) < 1e-18f) ? 0.0f : membraneLPStateR_;

            // ---- Output: blend dry external with filtered (permeability = wet/dry) ----
            outL[i] = inL * (1.0f - permeability) + membraneLPStateL_ * permeability;
            outR[i] = inR_forLP * (1.0f - permeability) + membraneLPStateR_ * permeability;

            // Cache for coupling
            couplingSampleL_ = outL[i];
            couplingSampleR_ = outR[i];
        }

        // ---- Update band RMS (exponential smoothing into stored values) ----
        if (bandCount > 0)
        {
            const float n = static_cast<float>(bandCount);
            for (int b = 0; b < 4; ++b)
            {
                const float rms = std::sqrt(bandSumSq[b] / n);
                bandRMS_[b] = bandRMS_[b] * 0.8f + rms * 0.2f;
            }
        }

        // ---- Pitch detection: autocorrelation — runs every ~512 samples ----
        // This spreads the O(N²) work over multiple blocks; 2048-sample buffer
        // gives reliable detection for 50–2000 Hz even on polyphonic material.
        pitchUpdateCounter_ += numSamples;
        if (pitchUpdateCounter_ >= kPitchUpdateInterval)
        {
            pitchUpdateCounter_ = 0;
            updatePitchViaAutocorrelation();
        }

        // Wake silence gate if external audio present
        if (envFollowerL_ > 0.001f || envFollowerR_ > 0.001f)
            wakeSilenceGate();

        analyzeForSilenceGate(buffer, numSamples);
    }

    //-- Coupling --------------------------------------------------------------
    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? couplingSampleL_ : couplingSampleR_;
    }

    // Fix #303: wire coupling input to membrane permeability modulation.
    // The coupling signal's block RMS is computed and blended into a
    // couplingPermeabilityOffset_ accumulator, which renderBlock() adds to the
    // base permeability value before clamping to [0, 1].
    //
    // Effect: an engine coupled into Osmosis raises the membrane permeability —
    // more external signal passes through (wet blend increases) as the coupled
    // engine becomes louder. This creates a natural call-and-response dynamic:
    // the membrane opens when something is pushing against it.
    //
    // Reactivity (osmo_reactivity) controls how quickly the offset tracks the
    // incoming coupling RMS by reusing the same attack coefficient used for the
    // envelope follower. amount [0, 1] scales the maximum modulation depth
    // so the coupling route strength drives the permeability sensitivity.
    void applyCouplingInput(CouplingType /*type*/, float amount, const float* sourceBuffer, int numSamples) override
    {
        if (!sourceBuffer || numSamples <= 0 || amount <= 0.0f)
        {
            // Decay toward zero when no active coupling arrives
            couplingPermeabilityOffset_ *= 0.999f;
            return;
        }

        // Compute block RMS of incoming coupling signal
        float sumSq = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            sumSq += sourceBuffer[i] * sourceBuffer[i];
        const float rms = std::sqrt(sumSq / static_cast<float>(numSamples));

        // Smooth toward the new target using a 50ms one-pole attack
        const float coeff = std::exp(-1.0f / (static_cast<float>(sr_) * 0.050f));
        const float target = rms * amount; // amount ∈ [0,1] scales depth
        couplingPermeabilityOffset_ = coeff * couplingPermeabilityOffset_ + (1.0f - coeff) * target;
    }

    //-- Parameters ------------------------------------------------------------
    // addParameters — called by XOceanusProcessor::createParameterLayout()
    // to register all osmo_ params in the shared APVTS.
    // Note: macro1-4 duplicate registrations removed — each param registered once.
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osmo_permeability", 1},
                                                                     "PERMEABILITY",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_selectivity", 1}, "SELECTIVITY", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_reactivity", 1}, "REACTIVITY", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osmo_memory", 1}, "MEMORY",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osmo_permeability", 1},
                                                               "PERMEABILITY",
                                                               juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osmo_selectivity", 1}, "SELECTIVITY",
                                                               juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osmo_reactivity", 1}, "REACTIVITY",
                                                               juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osmo_memory", 1}, "MEMORY",
                                                               juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        return layout;
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pPermeability_ = apvts.getRawParameterValue("osmo_permeability");
        pSelectivity_ = apvts.getRawParameterValue("osmo_selectivity");
        pReactivity_ = apvts.getRawParameterValue("osmo_reactivity");
        pMemory_ = apvts.getRawParameterValue("osmo_memory");
    }

private:
    double sr_ = 44100.0;
    int blockSize_ = 512;

    // External audio pointers (set per block by processor, NOT owned)
    const float* externalBufferL_ = nullptr;
    const float* externalBufferR_ = nullptr;
    int externalNumSamples_ = 0;

    // Envelope follower state
    float envFollowerL_ = 0.0f;
    float envFollowerR_ = 0.0f;

    // ---- Autocorrelation pitch detection ----
    // 2048-sample circular buffer (power of 2 — mask-based wrap is branchless)
    static constexpr int kAcBufSize = 2048;
    std::array<float, kAcBufSize> acBuffer_{};
    int acBufPos_ = 0;

    // Pitch estimation is expensive — recompute every ~512 samples
    static constexpr int kPitchUpdateInterval = 512;
    int pitchUpdateCounter_ = 0;
    float detectedPitch_ = 0.0f;

    // ---- Band RMS ----
    float bandRMS_[4] = {};

    // ---- CytomicSVF band-split filters ----
    // Each crossover uses two cascaded SVFs (12 dB/oct × 2 = 24 dB/oct Linkwitz-Riley).
    // L and R channel share the same coefficient — only L drives band RMS analysis.
    // (analysing mono suffices for RMS; pass-through uses the separate one-pole membrane filter.)

    // Crossover 1: 200 Hz LP (Band 0 output)
    CytomicSVF bandLP_[2]; // 2 cascaded LP @ 200 Hz (Linkwitz-Riley)
    // Crossover 1: 200 Hz HP (feeds Bands 1+2+3)
    CytomicSVF bandHP_[2]; // 2 cascaded HP @ 200 Hz
    // Crossover 2: 1 kHz LP (Band 1 output, applied after HP200)
    CytomicSVF bandLP2_[2]; // 2 cascaded LP @ 1 kHz
    // Crossover 2: 1 kHz HP (feeds Bands 2+3)
    CytomicSVF bandHP2_[2]; // 2 cascaded HP @ 1 kHz
    // Crossover 3: 5 kHz LP (Band 2 output, applied after HP1k)
    CytomicSVF bandLP3_[2]; // 2 cascaded LP @ 5 kHz
    // Crossover 3: 5 kHz HP (Band 3 output)
    CytomicSVF bandHP3_[2]; // 2 cascaded HP @ 5 kHz

    // Membrane filter
    float membraneLPCoeff_ = 0.0f;
    float membraneLPStateL_ = 0.0f;
    float membraneLPStateR_ = 0.0f;

    // Coupling output cache
    float couplingSampleL_ = 0.0f;
    float couplingSampleR_ = 0.0f;

    // Fix #303: coupling input offset — adds to base permeability so incoming
    // coupling signals open the membrane proportionally to their amplitude.
    float couplingPermeabilityOffset_ = 0.0f;

    // Parameter pointers
    std::atomic<float>* pPermeability_ = nullptr;
    std::atomic<float>* pSelectivity_ = nullptr;
    std::atomic<float>* pReactivity_ = nullptr;
    std::atomic<float>* pMemory_ = nullptr;

    // Smoothers
    ParameterSmoother permeabilitySmoother_;
    ParameterSmoother selectivitySmoother_;
    ParameterSmoother reactivitySmoother_;
    ParameterSmoother memorySmoother_;

    // Cached envelope-follower coefficients (#620 block-rate caching)
    float cachedAttackCoeff_  = 0.99f; // default ~44 ms @ 48 kHz
    float cachedReleaseCoeff_ = 0.99f;
    float cachedReactivity_   = -1.0f; // sentinel — forces compute on first block
    float cachedMemory_       = -1.0f;

    // LFO (modulates membrane cutoff)
    StandardLFO lfo_;

    //--------------------------------------------------------------------------
    void setupBandFilters()
    {
        const float sr = static_cast<float>(sr_);
        constexpr float kQ = 0.7071f; // Butterworth Q (Linkwitz-Riley cascade)

        // Crossover 1 @ 200 Hz
        for (int i = 0; i < 2; ++i)
        {
            bandLP_[i].setMode(CytomicSVF::Mode::LowPass);
            bandLP_[i].setCoefficients(200.0f, kQ, sr);
            bandLP_[i].reset();

            bandHP_[i].setMode(CytomicSVF::Mode::HighPass);
            bandHP_[i].setCoefficients(200.0f, kQ, sr);
            bandHP_[i].reset();
        }

        // Crossover 2 @ 1 kHz
        for (int i = 0; i < 2; ++i)
        {
            bandLP2_[i].setMode(CytomicSVF::Mode::LowPass);
            bandLP2_[i].setCoefficients(1000.0f, kQ, sr);
            bandLP2_[i].reset();

            bandHP2_[i].setMode(CytomicSVF::Mode::HighPass);
            bandHP2_[i].setCoefficients(1000.0f, kQ, sr);
            bandHP2_[i].reset();
        }

        // Crossover 3 @ 5 kHz
        for (int i = 0; i < 2; ++i)
        {
            bandLP3_[i].setMode(CytomicSVF::Mode::LowPass);
            bandLP3_[i].setCoefficients(5000.0f, kQ, sr);
            bandLP3_[i].reset();

            bandHP3_[i].setMode(CytomicSVF::Mode::HighPass);
            bandHP3_[i].setCoefficients(5000.0f, kQ, sr);
            bandHP3_[i].reset();
        }
    }

    //--------------------------------------------------------------------------
    // Autocorrelation-based pitch detection.
    // Searches lags corresponding to 50–2000 Hz, finds the peak correlation,
    // converts to Hz. Runs at ~kPitchUpdateInterval cadence (not per-sample).
    //
    // O(N·K) where N = kAcBufSize (2048), K = lag range (~800 lags).
    // At 44100 Hz with kPitchUpdateInterval=512 this is ~3M mults per second —
    // well within budget for an analysis engine (no synthesis voices to compete).
    void updatePitchViaAutocorrelation()
    {
        const float sr = static_cast<float>(sr_);

        // Lag bounds for 50–2000 Hz
        const int lagMin = static_cast<int>(sr / 2000.0f); // ~22 samples @ 44.1k
        const int lagMax = static_cast<int>(sr / 50.0f);   // ~882 samples @ 44.1k

        // Clamp to buffer capacity (leave half the buffer for the correlation window)
        const int maxUsableLag = kAcBufSize / 2 - 1;
        const int clampedLagMin = std::max(lagMin, 1);
        const int clampedLagMax = std::min(lagMax, maxUsableLag);

        if (clampedLagMin >= clampedLagMax)
            return; // degenerate — keep last estimate

        // Window length: use kAcBufSize/2 samples so we always have lag+window in buffer
        const int windowLen = kAcBufSize / 2;

        float bestCorr = -1.0f;
        int bestLag = clampedLagMin;

        // Normalise r(0) first to avoid magnitude-dependent bias
        float r0 = 0.0f;
        for (int n = 0; n < windowLen; ++n)
        {
            const int idx = (acBufPos_ - windowLen + n + kAcBufSize) & (kAcBufSize - 1);
            const float s = acBuffer_[idx];
            r0 += s * s;
        }
        if (r0 < 1e-10f)
            return; // silence — keep last estimate

        // Scan lags
        for (int lag = clampedLagMin; lag <= clampedLagMax; ++lag)
        {
            float corr = 0.0f;
            for (int n = 0; n < windowLen; ++n)
            {
                const int idx0 = (acBufPos_ - windowLen + n + kAcBufSize) & (kAcBufSize - 1);
                const int idx1 = (acBufPos_ - windowLen + n + lag + kAcBufSize) & (kAcBufSize - 1);
                corr += acBuffer_[idx0] * acBuffer_[idx1];
            }
            // Normalised correlation in [-1,+1]
            const float normCorr = corr / r0;
            if (normCorr > bestCorr)
            {
                bestCorr = normCorr;
                bestLag = lag;
            }
        }

        // Only update if correlation is strong enough (avoids noisy/unpitched signals)
        if (bestCorr > 0.3f)
        {
            const float newPitch = sr / static_cast<float>(bestLag);
            // Smooth the pitch estimate to prevent coupling consumers seeing jitter
            detectedPitch_ = detectedPitch_ * 0.7f + newPitch * 0.3f;
        }
    }
};

} // namespace xoceanus
