#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

namespace xomnibus {

//==============================================================================
// OpticNoiseGen — xorshift32 PRNG for stochastic modulation.
//==============================================================================
class OpticNoiseGen
{
public:
    void seed (uint32_t s) noexcept { state = s ? s : 1; }

    float process() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float> (static_cast<int32_t> (state)) / 2147483648.0f;
    }

private:
    uint32_t state = 1;
};

//==============================================================================
// OpticBandAnalyzer — 8-band energy tracker using cascaded SVF pairs.
// Runs on the audio thread. Extracts spectral shape without FFT overhead.
//
// Bands (approximate):
//   0: Sub       20-80 Hz       4: Hi-Mid     1-4 kHz
//   1: Bass      80-200 Hz      5: Presence   4-8 kHz
//   2: Lo-Mid    200-500 Hz     6: Brilliance 8-16 kHz
//   3: Mid       500-1000 Hz    7: Air        16-20 kHz
//==============================================================================
class OpticBandAnalyzer
{
public:
    static constexpr int kNumBands = 8;

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;

        // Crossover frequencies between bands
        static constexpr float crossovers[kNumBands - 1] = {
            80.0f, 200.0f, 500.0f, 1000.0f, 4000.0f, 8000.0f, 16000.0f
        };

        // Set up bandpass filters for each band
        for (int i = 0; i < kNumBands; ++i)
        {
            float lo = (i == 0) ? 20.0f : crossovers[i - 1];
            float hi = (i == kNumBands - 1) ? 20000.0f : crossovers[i];
            float center = std::sqrt (lo * hi);
            float denom = hi - lo;
            if (denom < 1.0f) denom = 1.0f;
            float q = center / denom;

            bandFilters[i].setMode (CytomicSVF::Mode::BandPass);
            bandFilters[i].setCoefficients (center, clamp (q * 0.5f, 0.1f, 0.9f),
                                             static_cast<float> (sr));

            // Envelope follower LP filter (~30 Hz cutoff for smooth tracking)
            envFilters[i].setMode (CytomicSVF::Mode::LowPass);
            envFilters[i].setCoefficients (30.0f, 0.0f, static_cast<float> (sr));
        }
    }

    void reset() noexcept
    {
        for (int i = 0; i < kNumBands; ++i)
        {
            bandFilters[i].reset();
            envFilters[i].reset();
            bandEnergy[i] = 0.0f;
        }
        spectralCentroid = 0.0f;
        spectralFlux = 0.0f;
        totalEnergy = 0.0f;
    }

    // Process a single sample — call per-sample in renderBlock
    void processSample (float input) noexcept
    {
        float prevTotal = totalEnergy;
        totalEnergy = 0.0f;

        for (int i = 0; i < kNumBands; ++i)
        {
            float bp = bandFilters[i].processSample (input);
            float rectified = std::fabs (bp);
            float smoothed = envFilters[i].processSample (rectified);
            bandEnergy[i] = flushDenormal (smoothed);
            totalEnergy += bandEnergy[i];
        }

        // Spectral centroid: energy-weighted average band index [0, 1]
        if (totalEnergy > 0.0001f)
        {
            float weightedSum = 0.0f;
            for (int i = 0; i < kNumBands; ++i)
                weightedSum += bandEnergy[i] * static_cast<float> (i);
            spectralCentroid = flushDenormal (weightedSum / (totalEnergy * (kNumBands - 1)));
        }
        else
        {
            spectralCentroid = 0.0f;
        }

        // Spectral flux: rate of change of total energy
        spectralFlux = flushDenormal (totalEnergy - prevTotal);
    }

    float getBandEnergy (int band) const noexcept { return bandEnergy[band]; }
    float getCentroid()            const noexcept { return spectralCentroid; }
    float getFlux()                const noexcept { return spectralFlux; }
    float getTotalEnergy()         const noexcept { return totalEnergy; }

private:
    double sr = 44100.0;
    CytomicSVF bandFilters[kNumBands];
    CytomicSVF envFilters[kNumBands];
    float bandEnergy[kNumBands] = {};
    float spectralCentroid = 0.0f;
    float spectralFlux = 0.0f;
    float totalEnergy = 0.0f;
};

//==============================================================================
// OpticAutoPulse — Self-oscillating rhythm generator for "autoplay" trance mode.
//
// Generates a thumping, evolving rhythmic pulse without MIDI input.
// The pulse is shaped like a kick drum: exponential pitch sweep + amplitude decay.
// Visual feedback modulates the pulse character over time, creating emergent
// trance patterns that evolve organically.
//
// Output is a modulation signal (0-1), not audio — it drives other engines
// through the coupling matrix.
//==============================================================================
class OpticAutoPulse
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        phase = 0.0;
        pulseLevel = 0.0f;
        noiseGen.seed (42);
    }

    void reset() noexcept
    {
        phase = 0.0;
        pulseLevel = 0.0f;
        swingAccum = 0.0f;
        driftPhase = 0.0;
        subdivAccum = 0.0;
    }

    struct Params {
        float rate;         // Base pulse rate in Hz (0.5-16, typically 1-4 for trance)
        float shape;        // Pulse shape: 0 = sharp kick, 1 = soft swell
        float swing;        // Swing amount 0-1
        float evolve;       // How much visual feedback morphs the pulse 0-1
        float subdivision;  // Rhythmic subdivision: 0=whole, 0.25=quarter, 0.5=8th, 1=16th
        float accent;       // Accent strength on downbeats 0-1

        // These come FROM the visual analyzer (lock-free handoff)
        float visualCentroid;   // 0-1 spectral centroid
        float visualEnergy;     // 0-1 total energy
        float visualFlux;       // spectral flux
    };

    float process (const Params& p) noexcept
    {
        // Base rate with visual-driven drift
        float effectiveRate = p.rate;
        if (p.evolve > 0.0f)
        {
            // Centroid shifts rate subtly (±20%)
            float centroidDrift = (p.visualCentroid - 0.5f) * 0.4f * p.evolve;
            // Energy surges speed up slightly
            float energyPush = p.visualEnergy * 0.15f * p.evolve;
            effectiveRate *= (1.0f + centroidDrift + energyPush);
        }

        // Soft drift LFO for organic rate variation
        driftPhase += 0.07 / sr;
        if (driftPhase > 1.0) driftPhase -= 1.0;
        float driftMod = fastSin (static_cast<float> (driftPhase * 6.28318530718)) * 0.03f * p.evolve;
        effectiveRate *= (1.0f + driftMod);

        effectiveRate = clamp (effectiveRate, 0.1f, 30.0f);

        // Advance phase
        double phaseInc = effectiveRate / sr;
        phase += phaseInc;

        // Subdivision counter
        float subdivRate = 1.0f;
        if (p.subdivision > 0.6f) subdivRate = 4.0f;       // 16th
        else if (p.subdivision > 0.35f) subdivRate = 2.0f;  // 8th
        else if (p.subdivision > 0.15f) subdivRate = 1.0f;  // quarter

        subdivAccum += phaseInc * subdivRate;

        // Detect pulse trigger (phase wrap)
        bool mainTrigger = (phase >= 1.0);
        bool subdivTrigger = (subdivAccum >= 1.0);

        if (mainTrigger)
        {
            phase -= 1.0;
            subdivAccum = 0.0;
            triggerPulse (1.0f, p);  // Full accent on downbeat
        }
        else if (subdivTrigger && subdivRate > 1.0f)
        {
            subdivAccum -= 1.0;
            // Subdivisions get reduced accent
            float subAccent = 0.4f + (1.0f - p.accent) * 0.3f;
            // Add swing to off-beats
            if (swingCounter & 1)
                subAccent *= (1.0f - p.swing * 0.3f);
            swingCounter = (swingCounter + 1) & 0x7FFFFFFF;
            triggerPulse (subAccent, p);
        }

        // Decay the pulse
        float decayRate = getDecayRate (p);
        pulseLevel *= decayRate;
        pulseLevel = flushDenormal (pulseLevel);

        return pulseLevel;
    }

    bool isActive() const noexcept { return pulseLevel > 0.001f; }

private:
    double sr = 44100.0;
    double phase = 0.0;
    double driftPhase = 0.0;
    double subdivAccum = 0.0;
    float pulseLevel = 0.0f;
    float swingAccum = 0.0f;
    int swingCounter = 0;
    OpticNoiseGen noiseGen;

    void triggerPulse (float accent, const Params& p) noexcept
    {
        // Shape-dependent trigger: sharp = instant peak, soft = ramp up
        float attack = lerp (1.0f, 0.5f + pulseLevel * 0.5f, p.shape);
        pulseLevel = clamp (attack * accent, 0.0f, 1.0f);

        // Visual flux adds stochastic accent variation
        if (p.evolve > 0.0f)
        {
            float fluxBoost = clamp (p.visualFlux * 2.0f, -0.3f, 0.3f) * p.evolve;
            pulseLevel = clamp (pulseLevel + fluxBoost, 0.0f, 1.0f);
        }
    }

    float getDecayRate (const Params& p) const noexcept
    {
        // Shape controls decay: 0 = fast (kick), 1 = slow (pad swell)
        float decayMs = lerp (30.0f, 500.0f, p.shape);

        // Visual energy makes pulses ring longer when there's more activity
        if (p.evolve > 0.0f)
            decayMs *= (1.0f + p.visualEnergy * 0.5f * p.evolve);

        float decaySamples = decayMs * 0.001f * static_cast<float> (sr);
        if (decaySamples < 1.0f) return 0.0f;
        return flushDenormal (std::pow (0.001f, 1.0f / decaySamples));  // -60dB decay time
    }
};

//==============================================================================
// OpticModOutputs — Lock-free modulation signal bus.
// Written on the audio thread, read on the UI thread for visualization,
// and also read back on the audio thread for coupling output.
//
// 8 modulation channels:
//   0: AutoPulse level         4: Spectral centroid
//   1: Bass energy (sub+bass)  5: Spectral flux
//   2: Mid energy              6: Total energy
//   3: High energy             7: Transient detect (flux > threshold)
//==============================================================================
struct OpticModOutputs
{
    static constexpr int kNumMods = 8;
    std::atomic<float> values[kNumMods] = {};

    void store (int idx, float val) noexcept { values[idx].store (val, std::memory_order_relaxed); }
    float load (int idx) const noexcept { return values[idx].load (std::memory_order_relaxed); }

    // Named accessors for clarity
    void setPulse     (float v) noexcept { store (0, v); }
    void setBass      (float v) noexcept { store (1, v); }
    void setMid       (float v) noexcept { store (2, v); }
    void setHigh      (float v) noexcept { store (3, v); }
    void setCentroid  (float v) noexcept { store (4, v); }
    void setFlux      (float v) noexcept { store (5, v); }
    void setEnergy    (float v) noexcept { store (6, v); }
    void setTransient (float v) noexcept { store (7, v); }

    float getPulse()     const noexcept { return load (0); }
    float getBass()      const noexcept { return load (1); }
    float getMid()       const noexcept { return load (2); }
    float getHigh()      const noexcept { return load (3); }
    float getCentroid()  const noexcept { return load (4); }
    float getFlux()      const noexcept { return load (5); }
    float getEnergy()    const noexcept { return load (6); }
    float getTransient() const noexcept { return load (7); }
};

//==============================================================================
// OpticEngine — Visual Modulation Engine ("XOptic")
//
// A Winamp-inspired audio-reactive visualizer engine that also modulates sound.
// OPTIC doesn't generate audio — it analyzes incoming audio (via coupling),
// generates evolving visual modulation signals, and feeds them back into
// the coupling matrix to shape other engines' parameters.
//
// The AutoPulse mode generates self-evolving thumping trance rhythms:
// the visualizer's spectral state slowly morphs the pulse character,
// creating patterns that breathe and shift without player input.
//
// Accent: Phosphor Green #00FF41 (CRT phosphor glow)
// Param prefix: optic_
//==============================================================================
class OpticEngine : public SynthEngine
{
public:
    //--------------------------------------------------------------------------
    // Lifecycle
    //--------------------------------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;

        analyzer.prepare (sampleRate);
        autoPulse.prepare (sampleRate);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        // Smoothing filters for mod outputs (prevent zipper noise)
        for (int i = 0; i < OpticModOutputs::kNumMods; ++i)
        {
            modSmoothing[i].setMode (CytomicSVF::Mode::LowPass);
            modSmoothing[i].setCoefficients (15.0f, 0.0f, static_cast<float> (sr));
        }

        inputSmoother.setMode (CytomicSVF::Mode::LowPass);
        inputSmoother.setCoefficients (10.0f, 0.0f, static_cast<float> (sr));
    }

    void releaseResources() override
    {
        outputCacheL.clear();
        outputCacheR.clear();
    }

    void reset() override
    {
        analyzer.reset();
        autoPulse.reset();
        for (int i = 0; i < OpticModOutputs::kNumMods; ++i)
            modSmoothing[i].reset();
        inputSmoother.reset();

        couplingInputLevel = 0.0f;
        envelopeOutput = 0.0f;
        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //--------------------------------------------------------------------------
    // Audio rendering
    //--------------------------------------------------------------------------

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/,
                      int numSamples) override
    {
        // ---- ParamSnapshot: cache all parameters once per block ----
        const float reactivity  = (pReactivity  != nullptr) ? pReactivity->load()  : 0.7f;
        const float modDepth    = (pModDepth    != nullptr) ? pModDepth->load()    : 0.5f;
        const float pulseRate   = (pPulseRate   != nullptr) ? pPulseRate->load()   : 2.0f;
        const float pulseShape  = (pPulseShape  != nullptr) ? pPulseShape->load()  : 0.2f;
        const float pulseSwing  = (pPulseSwing  != nullptr) ? pPulseSwing->load()  : 0.0f;
        const float pulseEvolve = (pPulseEvolve != nullptr) ? pPulseEvolve->load() : 0.5f;
        const float pulseSub    = (pPulseSubdiv != nullptr) ? pPulseSubdiv->load() : 0.0f;
        const float pulseAccent = (pPulseAccent != nullptr) ? pPulseAccent->load() : 0.7f;
        const float autoPulseOn = (pAutoPulse   != nullptr) ? pAutoPulse->load()   : 0.0f;
        const float inputGain   = (pInputGain   != nullptr) ? pInputGain->load()   : 1.0f;
        const float modMixPulse = (pModMixPulse != nullptr) ? pModMixPulse->load() : 0.5f;
        const float modMixSpec  = (pModMixSpec  != nullptr) ? pModMixSpec->load()  : 0.5f;

        const bool doPulse = (autoPulseOn >= 0.5f);

        // ---- Per-sample processing ----
        for (int i = 0; i < numSamples; ++i)
        {
            // Get input: either from coupling input or from the audio buffer
            float inputSample = couplingInputLevel;

            // Also read any audio already in the buffer (sidechain from host)
            if (buffer.getNumChannels() > 0)
            {
                float bufL = buffer.getSample (0, i);
                float bufR = buffer.getNumChannels() > 1 ? buffer.getSample (1, i) : bufL;
                inputSample += (bufL + bufR) * 0.5f;
            }

            inputSample *= inputGain * reactivity;
            inputSample = inputSmoother.processSample (inputSample);

            // Spectrum analysis
            analyzer.processSample (inputSample);

            // AutoPulse
            float pulseOut = 0.0f;
            if (doPulse)
            {
                OpticAutoPulse::Params pp;
                pp.rate = pulseRate;
                pp.shape = pulseShape;
                pp.swing = pulseSwing;
                pp.evolve = pulseEvolve;
                pp.subdivision = pulseSub;
                pp.accent = pulseAccent;
                pp.visualCentroid = analyzer.getCentroid();
                pp.visualEnergy = clamp (analyzer.getTotalEnergy() * 5.0f, 0.0f, 1.0f);
                pp.visualFlux = analyzer.getFlux();

                pulseOut = autoPulse.process (pp);
            }

            // Compute mod outputs
            float bass = (analyzer.getBandEnergy (0) + analyzer.getBandEnergy (1)) * 4.0f;
            float mid = (analyzer.getBandEnergy (2) + analyzer.getBandEnergy (3)
                       + analyzer.getBandEnergy (4)) * 3.0f;
            float high = (analyzer.getBandEnergy (5) + analyzer.getBandEnergy (6)
                        + analyzer.getBandEnergy (7)) * 5.0f;
            float centroid = analyzer.getCentroid();
            float flux = analyzer.getFlux();
            float energy = clamp (analyzer.getTotalEnergy() * 5.0f, 0.0f, 1.0f);
            float transient = (flux > 0.05f) ? 1.0f : 0.0f;

            // Apply smoothing
            bass      = modSmoothing[1].processSample (bass);
            mid       = modSmoothing[2].processSample (mid);
            high      = modSmoothing[3].processSample (high);
            centroid  = modSmoothing[4].processSample (centroid);
            energy    = modSmoothing[6].processSample (energy);

            // Clamp to [0, 1]
            bass     = clamp (bass, 0.0f, 1.0f);
            mid      = clamp (mid, 0.0f, 1.0f);
            high     = clamp (high, 0.0f, 1.0f);
            centroid = clamp (centroid, 0.0f, 1.0f);
            energy   = clamp (energy, 0.0f, 1.0f);

            // Store to atomic bus (UI thread can read these for visualization)
            modOutputs.setPulse (pulseOut);
            modOutputs.setBass (bass);
            modOutputs.setMid (mid);
            modOutputs.setHigh (high);
            modOutputs.setCentroid (centroid);
            modOutputs.setFlux (flux);
            modOutputs.setEnergy (energy);
            modOutputs.setTransient (transient);

            // Composite modulation output for coupling:
            // Blend pulse-driven and spectrum-driven modulation
            float couplingOut = 0.0f;
            if (doPulse)
                couplingOut += pulseOut * modMixPulse;
            couplingOut += energy * modMixSpec;
            couplingOut *= modDepth;
            couplingOut = clamp (couplingOut, 0.0f, 1.0f);

            // Cache for coupling reads
            auto si = static_cast<size_t> (i);
            if (si < outputCacheL.size())
            {
                outputCacheL[si] = couplingOut;
                outputCacheR[si] = couplingOut;
            }
            envelopeOutput = couplingOut;
        }

        // OPTIC outputs modulation, not audio — clear the buffer
        buffer.clear();

        // Reset coupling accumulator for next block
        couplingInputLevel = 0.0f;
    }

    //--------------------------------------------------------------------------
    // Coupling
    //--------------------------------------------------------------------------

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto si = static_cast<size_t> (sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];

        // Channel 2 = composite envelope (for AmpToFilter, AmpToPitch on other engines)
        if (channel == 2) return envelopeOutput;

        // Channels 3-10 = individual mod outputs (extended coupling)
        if (channel >= 3 && channel < 3 + OpticModOutputs::kNumMods)
            return modOutputs.load (channel - 3);

        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int numSamples) override
    {
        // OPTIC receives audio from other engines for analysis
        switch (type)
        {
            case CouplingType::AudioToFM:
            case CouplingType::AudioToRing:
            case CouplingType::AudioToWavetable:
            case CouplingType::FilterToFilter:
            {
                // Sum the incoming audio for spectrum analysis
                if (numSamples > 0 && sourceBuffer != nullptr)
                {
                    float sum = 0.0f;
                    for (int i = 0; i < numSamples; ++i)
                        sum += std::fabs (sourceBuffer[i]);
                    couplingInputLevel += (sum / static_cast<float> (numSamples)) * amount;
                }
                break;
            }

            case CouplingType::AmpToFilter:
            case CouplingType::AmpToPitch:
                // Amplitude coupling: modulate reactivity
                couplingInputLevel += amount * 0.5f;
                break;

            case CouplingType::RhythmToBlend:
                // Rhythm input can sync AutoPulse (future: tempo sync)
                couplingInputLevel += amount * 0.3f;
                break;

            default:
                break;
        }
    }

    //--------------------------------------------------------------------------
    // Parameters — prefix: optic_
    //--------------------------------------------------------------------------

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
        // --- Analysis ---

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_reactivity", 1 }, "Optic Reactivity",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_inputGain", 1 }, "Optic Input Gain",
            juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 1.0f));

        // --- AutoPulse ---

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "optic_autoPulse", 1 }, "Optic AutoPulse",
            juce::StringArray { "Off", "On" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_pulseRate", 1 }, "Optic Pulse Rate",
            juce::NormalisableRange<float> (0.5f, 16.0f, 0.01f, 0.35f), 2.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_pulseShape", 1 }, "Optic Pulse Shape",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.2f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_pulseSwing", 1 }, "Optic Pulse Swing",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_pulseEvolve", 1 }, "Optic Pulse Evolve",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_pulseSubdiv", 1 }, "Optic Pulse Subdivision",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_pulseAccent", 1 }, "Optic Pulse Accent",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

        // --- Mod Output ---

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_modDepth", 1 }, "Optic Mod Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_modMixPulse", 1 }, "Optic Mod Mix Pulse",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_modMixSpec", 1 }, "Optic Mod Mix Spectrum",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        // --- Visualizer style (for UI rendering) ---

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "optic_vizMode", 1 }, "Optic Viz Mode",
            juce::StringArray { "Scope", "Spectrum", "Milkdrop", "Particles" }, 2));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_vizFeedback", 1 }, "Optic Viz Feedback",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_vizSpeed", 1 }, "Optic Viz Speed",
            juce::NormalisableRange<float> (0.1f, 4.0f, 0.01f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_vizIntensity", 1 }, "Optic Viz Intensity",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
    }

public:
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pReactivity  = apvts.getRawParameterValue ("optic_reactivity");
        pInputGain   = apvts.getRawParameterValue ("optic_inputGain");
        pAutoPulse   = apvts.getRawParameterValue ("optic_autoPulse");
        pPulseRate   = apvts.getRawParameterValue ("optic_pulseRate");
        pPulseShape  = apvts.getRawParameterValue ("optic_pulseShape");
        pPulseSwing  = apvts.getRawParameterValue ("optic_pulseSwing");
        pPulseEvolve = apvts.getRawParameterValue ("optic_pulseEvolve");
        pPulseSubdiv = apvts.getRawParameterValue ("optic_pulseSubdiv");
        pPulseAccent = apvts.getRawParameterValue ("optic_pulseAccent");
        pModDepth    = apvts.getRawParameterValue ("optic_modDepth");
        pModMixPulse = apvts.getRawParameterValue ("optic_modMixPulse");
        pModMixSpec  = apvts.getRawParameterValue ("optic_modMixSpec");
        pVizMode     = apvts.getRawParameterValue ("optic_vizMode");
        pVizFeedback = apvts.getRawParameterValue ("optic_vizFeedback");
        pVizSpeed    = apvts.getRawParameterValue ("optic_vizSpeed");
        pVizIntensity = apvts.getRawParameterValue ("optic_vizIntensity");
    }

    //--------------------------------------------------------------------------
    // Identity
    //--------------------------------------------------------------------------

    juce::String getEngineId() const override { return "Optic"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF00FF41); }
    int getMaxVoices() const override { return 0; }  // Modulation engine, no voices

    //--------------------------------------------------------------------------
    // Public access to mod bus (for UI visualizer component)
    //--------------------------------------------------------------------------

    const OpticModOutputs& getModOutputs() const noexcept { return modOutputs; }
    const OpticBandAnalyzer& getAnalyzer() const noexcept { return analyzer; }

private:
    double sr = 44100.0;

    // DSP
    OpticBandAnalyzer analyzer;
    OpticAutoPulse autoPulse;
    OpticModOutputs modOutputs;
    CytomicSVF modSmoothing[OpticModOutputs::kNumMods];
    CytomicSVF inputSmoother;

    // Coupling state
    float couplingInputLevel = 0.0f;
    float envelopeOutput = 0.0f;

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Parameter pointers (cached for zero-cost audio-thread reads)
    std::atomic<float>* pReactivity  = nullptr;
    std::atomic<float>* pInputGain   = nullptr;
    std::atomic<float>* pAutoPulse   = nullptr;
    std::atomic<float>* pPulseRate   = nullptr;
    std::atomic<float>* pPulseShape  = nullptr;
    std::atomic<float>* pPulseSwing  = nullptr;
    std::atomic<float>* pPulseEvolve = nullptr;
    std::atomic<float>* pPulseSubdiv = nullptr;
    std::atomic<float>* pPulseAccent = nullptr;
    std::atomic<float>* pModDepth    = nullptr;
    std::atomic<float>* pModMixPulse = nullptr;
    std::atomic<float>* pModMixSpec  = nullptr;
    std::atomic<float>* pVizMode     = nullptr;
    std::atomic<float>* pVizFeedback = nullptr;
    std::atomic<float>* pVizSpeed    = nullptr;
    std::atomic<float>* pVizIntensity = nullptr;
};

} // namespace xomnibus
