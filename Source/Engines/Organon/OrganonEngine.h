#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>
#include <cstring>
#include <atomic>

namespace xomnibus {

//==============================================================================
// OrganonNoiseGen — xorshift32 PRNG for internal noise substrate.
// Generates the "self-feeding" signal when no coupling input is active.
//==============================================================================
class OrganonNoiseGen
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
// EntropyAnalyzer — Shannon entropy from short-time amplitude histogram.
//
// Measures the information content of the ingested signal. High entropy
// (noisy/complex input) means rich "nutrition"; low entropy (simple/tonal)
// means less energy available for reconstruction.
//
// Algorithm:
//   1. Quantize samples into 32 amplitude bins
//   2. Build histogram over a 256-sample window
//   3. Compute Shannon entropy: H = -sum(p_i * log2(p_i))
//   4. Normalize to [0, 1] via H / log2(32)
//==============================================================================
class EntropyAnalyzer
{
public:
    static constexpr int kNumBins = 32;
    static constexpr int kDefaultWindowSize = 256;
    static constexpr int kSmallWindowSize = 64;
    static constexpr float kMaxEntropy = 5.0f; // log2(32) = 5.0

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        controlRateDiv = std::max (1, static_cast<int> (sampleRate / 2000.0));
        reset();
    }

    void reset() noexcept
    {
        writePos = 0;
        controlCounter = 0;
        entropyValue = 0.0f;
        spectralCentroid = 0.5f;
        std::memset (ringBuffer, 0, sizeof (ringBuffer));
        std::memset (histogram, 0, sizeof (histogram));
    }

    // Feed a sample into the analysis buffer. Call every audio sample.
    void pushSample (float sample) noexcept
    {
        ringBuffer[writePos] = sample;
        writePos = (writePos + 1) & (kMaxWindowSize - 1);

        if (++controlCounter >= controlRateDiv)
        {
            controlCounter = 0;
            computeEntropy (currentWindowSize);
        }
    }

    // Set analysis window size based on enzyme selectivity.
    // High-frequency diet → smaller window for faster response.
    void setWindowSize (float enzymeFreq) noexcept
    {
        currentWindowSize = (enzymeFreq > 10000.0f) ? kSmallWindowSize : kDefaultWindowSize;
    }

    float getEntropy() const noexcept { return entropyValue; }
    float getSpectralCentroid() const noexcept { return spectralCentroid; }

private:
    static constexpr int kMaxWindowSize = 256; // must be power of 2

    void computeEntropy (int windowSize) noexcept
    {
        std::memset (histogram, 0, sizeof (histogram));

        int startPos = (writePos - windowSize + kMaxWindowSize) & (kMaxWindowSize - 1);
        float invWindowSize = 1.0f / static_cast<float> (windowSize);

        // Build histogram
        for (int i = 0; i < windowSize; ++i)
        {
            int idx = (startPos + i) & (kMaxWindowSize - 1);
            float s = ringBuffer[idx];
            // Map [-1, 1] → [0, kNumBins-1]
            int bin = static_cast<int> ((s * 0.5f + 0.5f) * static_cast<float> (kNumBins - 1));
            if (bin < 0) bin = 0;
            if (bin >= kNumBins) bin = kNumBins - 1;
            histogram[bin]++;
        }

        // Compute Shannon entropy and spectral centroid
        float entropy = 0.0f;
        float centroidNum = 0.0f;
        float centroidDen = 0.0f;

        for (int i = 0; i < kNumBins; ++i)
        {
            if (histogram[i] > 0)
            {
                float p = static_cast<float> (histogram[i]) * invWindowSize;
                entropy -= p * fastLog2 (p);
                float binPos = static_cast<float> (i) / static_cast<float> (kNumBins - 1);
                centroidNum += binPos * p;
                centroidDen += p;
            }
        }

        entropyValue = clamp (entropy / kMaxEntropy, 0.0f, 1.0f);
        spectralCentroid = (centroidDen > 0.0001f) ? (centroidNum / centroidDen) : 0.5f;
    }

    double sr = 44100.0;
    int controlRateDiv = 22;
    int controlCounter = 0;
    int writePos = 0;
    int currentWindowSize = kDefaultWindowSize;
    float ringBuffer[kMaxWindowSize] {};
    int histogram[kNumBins] {};
    float entropyValue = 0.0f;
    float spectralCentroid = 0.5f;
};

//==============================================================================
// MetabolicEconomy — Free Energy Pool tracking.
//
// Tracks the organism's internal energy state. Accumulates when entropy is
// high (rich input). Depletes at the metabolic rate. Controls how many
// harmonic modes are active and how dense the output is.
//==============================================================================
class MetabolicEconomy
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        controlRateDiv = std::max (1, static_cast<int> (sampleRate / 2000.0));
        reset();
    }

    void reset() noexcept
    {
        freeEnergy = 0.0f;
        controlCounter = 0;
    }

    // Update once per control tick
    // metabolicRate: Hz (0.1-10), signalFlux: 0-1, entropyValue: 0-1
    void update (float metabolicRate, float signalFlux, float entropyValue,
                 float externalRhythmMod, float externalDecayMod,
                 bool noteReleased) noexcept
    {
        if (++controlCounter < controlRateDiv)
            return;
        controlCounter = 0;

        float dt = static_cast<float> (controlRateDiv) / static_cast<float> (sr);

        // Effective metabolic rate (modulated by external coupling)
        float effectiveRate = metabolicRate * (1.0f + externalDecayMod);
        if (effectiveRate < 0.01f) effectiveRate = 0.01f;

        // Accumulation vs depletion
        float intake = entropyValue * signalFlux * (1.0f + externalRhythmMod);
        float cost = effectiveRate * 0.1f; // base metabolic cost

        // Starvation on note release — 4x metabolic rate
        if (noteReleased)
            cost *= 4.0f;

        freeEnergy += (intake - cost) * dt;
        freeEnergy = clamp (freeEnergy, 0.0f, 1.0f);
    }

    float getFreeEnergy() const noexcept { return freeEnergy; }
    void setFreeEnergy (float e) noexcept { freeEnergy = clamp (e, 0.0f, 1.0f); }

private:
    double sr = 44100.0;
    int controlRateDiv = 22;
    int controlCounter = 0;
    float freeEnergy = 0.0f;
};

//==============================================================================
// ModalArray — 32-mode Port-Hamiltonian harmonic oscillator array.
//
// Each mode is a damped harmonic oscillator driven by energy from the
// metabolic economy. The modes collectively reconstruct harmonic content
// from the entropy analysis of the ingested signal.
//
// ODE per mode n:
//   dx_n/dt = v_n
//   dv_n/dt = -omega_n^2 * x_n - gamma * v_n + F_n(t)
//
// Solved with RK4 per sample. All 32 modes stored contiguously for cache
// efficiency and SIMD optimization.
//==============================================================================
class ModalArray
{
public:
    static constexpr int kNumModes = 32;

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        invSr = 1.0 / sampleRate;
        reset();
    }

    void reset() noexcept
    {
        std::memset (x, 0, sizeof (x));
        std::memset (v, 0, sizeof (v));
        std::memset (omega, 0, sizeof (omega));
        std::memset (weight, 0, sizeof (weight));
    }

    // Set fundamental frequency from MIDI note.
    // isotopeBalance shifts the mode distribution (0 = sub, 1 = upper partials).
    void setFundamental (float freqHz, float isotopeBalance) noexcept
    {
        constexpr float twoPi = 6.28318530717958647692f;
        for (int n = 0; n < kNumModes; ++n)
        {
            // Harmonic spacing with isotope weighting
            float harmonic = static_cast<float> (n + 1);

            // Isotope balance shifts energy distribution:
            //   At 0.0: compress harmonics downward (subharmonic weighting)
            //   At 0.5: natural harmonic series
            //   At 1.0: spread harmonics upward (upper partial emphasis)
            float spread = 0.5f + isotopeBalance * 1.5f; // range [0.5, 2.0]
            float modeFreq = freqHz * std::pow (harmonic, spread);

            // Clamp to Nyquist
            float nyquist = static_cast<float> (sr) * 0.49f;
            if (modeFreq > nyquist) modeFreq = nyquist;

            omega[n] = twoPi * modeFreq;
        }
    }

    // Update mode weights from entropy analysis (control rate).
    // spectralCentroid: 0-1, freeEnergy: 0-1, catalystDrive: 0-2
    // entropyValue: 0-1
    void updateWeights (float spectralCentroid, float freeEnergy,
                        float catalystDrive, float entropyValue,
                        float isotopeBalance) noexcept
    {
        float bandwidth = 0.15f + 0.35f * isotopeBalance;
        float invBw2 = 1.0f / (bandwidth * bandwidth + 0.0001f);

        for (int n = 0; n < kNumModes; ++n)
        {
            float modePos = static_cast<float> (n) / static_cast<float> (kNumModes - 1);
            float dist = modePos - spectralCentroid;
            float gaussian = fastExp (-0.5f * dist * dist * invBw2);

            // Driving force strength: catalyst * energy * entropy * gaussian weight
            weight[n] = catalystDrive * freeEnergy * entropyValue * gaussian;
        }
    }

    // Process one sample — RK4 integration of all 32 modes.
    // Returns the summed displacement of all modes.
    float processSample (float damping) noexcept
    {
        float dt = static_cast<float> (invSr);
        float halfDt = dt * 0.5f;
        float output = 0.0f;

        for (int n = 0; n < kNumModes; ++n)
        {
            float w2 = omega[n] * omega[n];
            float F = weight[n];
            float g = damping;

            // Current state
            float x0 = x[n];
            float v0 = v[n];

            // RK4 integration
            // k1
            float dx1 = v0;
            float dv1 = -w2 * x0 - g * v0 + F;

            // k2
            float x1 = x0 + halfDt * dx1;
            float v1 = v0 + halfDt * dv1;
            float dx2 = v1;
            float dv2 = -w2 * x1 - g * v1 + F;

            // k3
            float x2 = x0 + halfDt * dx2;
            float v2 = v0 + halfDt * dv2;
            float dx3 = v2;
            float dv3 = -w2 * x2 - g * v2 + F;

            // k4
            float x3 = x0 + dt * dx3;
            float v3 = v0 + dt * dv3;
            float dx4 = v3;
            float dv4 = -w2 * x3 - g * v3 + F;

            // Combine
            x[n] = flushDenormal (x0 + (dt / 6.0f) * (dx1 + 2.0f * dx2 + 2.0f * dx3 + dx4));
            v[n] = flushDenormal (v0 + (dt / 6.0f) * (dv1 + 2.0f * dv2 + 2.0f * dv3 + dv4));

            output += x[n];
        }

        return output;
    }

private:
    double sr = 44100.0;
    double invSr = 1.0 / 44100.0;

    // Contiguous arrays for cache efficiency
    alignas(16) float x[kNumModes] {};       // displacements
    alignas(16) float v[kNumModes] {};       // velocities
    alignas(16) float omega[kNumModes] {};   // angular frequencies
    alignas(16) float weight[kNumModes] {};  // driving force weights (updated at control rate)
};

//==============================================================================
// OrganonVoice — A single metabolic organism.
//
// Each voice is an independent organism with its own:
//   - Ingestion stage (ring buffer for coupling input or internal noise)
//   - Catabolism stage (entropy analyzer)
//   - Economy stage (metabolic free energy pool)
//   - Anabolism stage (32-mode Port-Hamiltonian modal array)
//==============================================================================
struct OrganonVoice
{
    bool active = false;
    bool released = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;       // for LRU voice stealing
    float stealFadeGain = 1.0f;   // crossfade on voice steal (5ms)
    float stealFadeStep = 0.0f;

    // DSP stages
    OrganonNoiseGen noiseGen;
    CytomicSVF ingestionFilter;   // enzyme selectivity bandpass on noise
    EntropyAnalyzer entropy;
    MetabolicEconomy economy;
    ModalArray modes;

    // Coupling ingestion buffer (pre-allocated ring)
    static constexpr int kIngestionBufferSize = 2048;
    float ingestionBuffer[kIngestionBufferSize] {};
    int ingestionWritePos = 0;
    bool hasCouplingInput = false;

    void prepare (double sampleRate, int /*maxBlockSize*/) noexcept
    {
        entropy.prepare (sampleRate);
        economy.prepare (sampleRate);
        modes.prepare (sampleRate);
        ingestionFilter.setMode (CytomicSVF::Mode::BandPass);
        resetVoice();
    }

    void resetVoice() noexcept
    {
        active = false;
        released = false;
        noteNumber = -1;
        velocity = 0.0f;
        stealFadeGain = 1.0f;
        stealFadeStep = 0.0f;
        entropy.reset();
        economy.reset();
        modes.reset();
        ingestionFilter.reset();
        ingestionWritePos = 0;
        hasCouplingInput = false;
        std::memset (ingestionBuffer, 0, sizeof (ingestionBuffer));
    }

    void noteOn (int note, float vel, uint64_t time, double sampleRate) noexcept
    {
        active = true;
        released = false;
        noteNumber = note;
        velocity = vel;
        startTime = time;
        stealFadeGain = 1.0f;
        stealFadeStep = 0.0f;

        entropy.reset();
        economy.reset();
        modes.reset();
        ingestionFilter.reset();

        // Velocity → initial catalyst boost (higher velocity = faster bloom)
        economy.setFreeEnergy (vel * 0.15f);

        // Seed noise gen uniquely per voice/note
        noiseGen.seed (static_cast<uint32_t> (note * 7919 + time));
    }

    void noteOff() noexcept
    {
        released = true;
    }

    // Called when stealing this voice for a new note (5ms crossfade)
    void beginStealFade (double sampleRate) noexcept
    {
        int fadeSamples = static_cast<int> (sampleRate * 0.005); // 5ms
        if (fadeSamples < 1) fadeSamples = 1;
        stealFadeStep = stealFadeGain / static_cast<float> (fadeSamples);
    }

    // Legato: migrate organism to new note (preserve free energy)
    void legatoRetrigger (int note, float vel, uint64_t time) noexcept
    {
        noteNumber = note;
        velocity = vel;
        startTime = time;
        released = false;
        // Keep economy.freeEnergy — the organism migrates
        // Reset modes to retune to new fundamental
        modes.reset();
    }
};

//==============================================================================
// OrganonEngine — Informational Dissipative Synthesis.
//
// "XO-Organon is a metabolic synth that consumes audio signals to grow
//  living harmonic structures."
//
// Sound is produced by:
//   1. Ingesting audio (from coupling partners or internal noise)
//   2. Analyzing its entropy (information content)
//   3. Converting entropy to free energy (metabolic economy)
//   4. Using free energy to drive a 32-mode harmonic oscillator array
//
// The result: sound that evolves based on what the engine has been fed.
// No two performances are identical because the engine accumulates internal
// state from its history of coupling inputs.
//==============================================================================
class OrganonEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 4;

    //-- Identity --------------------------------------------------------------

    juce::String getEngineId() const override { return "Organon"; }

    juce::Colour getAccentColour() const override
    {
        return juce::Colour (0xFF00CED1); // Bioluminescent Cyan
    }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override
    {
        return activeVoiceCount.load (std::memory_order_relaxed);
    }

    //-- Lifecycle -------------------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        blockSize = maxBlockSize;
        noteCounter = 0;

        for (auto& voice : voices)
            voice.prepare (sampleRate, maxBlockSize);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);
    }

    void releaseResources() override
    {
        outputCacheL.clear();
        outputCacheR.clear();
    }

    void reset() override
    {
        for (auto& voice : voices)
            voice.resetVoice();

        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);

        externalPitchMod = 0.0f;
        externalFilterMod = 0.0f;
        externalMorphMod = 0.0f;
        externalRhythmMod = 0.0f;
        externalDecayMod = 0.0f;
        couplingAudioActive = false;
    }

    //-- Audio -----------------------------------------------------------------

    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        // ParamSnapshot: read all parameters once per block
        const float metabolicRate  = paramMetabolicRate  ? paramMetabolicRate->load()  : 1.0f;
        const float enzymeSelect   = paramEnzymeSelect   ? paramEnzymeSelect->load()   : 1000.0f;
        const float catalystDrive  = paramCatalystDrive  ? paramCatalystDrive->load()  : 0.5f;
        const float dampingCoeff   = paramDampingCoeff   ? paramDampingCoeff->load()   : 0.3f;
        const float signalFlux     = paramSignalFlux     ? paramSignalFlux->load()     : 0.5f;
        const float phasonShift    = paramPhasonShift    ? paramPhasonShift->load()    : 0.0f;
        const float isotopeBalance = paramIsotopeBalance ? paramIsotopeBalance->load() : 0.5f;
        const float lockIn         = paramLockIn         ? paramLockIn->load()         : 0.0f;
        const float membrane       = paramMembrane       ? paramMembrane->load()       : 0.2f;
        const float noiseColor     = paramNoiseColor     ? paramNoiseColor->load()     : 0.5f;

        // Handle MIDI
        for (const auto metadata : midi)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                handleNoteOn (msg.getNoteNumber(), msg.getFloatVelocity());
            else if (msg.isNoteOff())
                handleNoteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                for (auto& voice : voices)
                    voice.resetVoice();
            }
        }

        // Damping mapped from parameter (0.01-0.99) → actual gamma value
        // Higher param = more damping = shorter tail
        float gamma = dampingCoeff * 20.0f; // scale to useful range for ODE

        // Process each sample
        for (int s = 0; s < numSamples; ++s)
        {
            float mixL = 0.0f;
            float mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                // Set entropy window size based on enzyme selectivity
                voice.entropy.setWindowSize (enzymeSelect);

                // --- INGESTION ---
                float ingested = 0.0f;
                if (couplingAudioActive && voice.hasCouplingInput)
                {
                    // Read from coupling ingestion buffer
                    int readPos = (voice.ingestionWritePos - 1 + OrganonVoice::kIngestionBufferSize)
                                  & (OrganonVoice::kIngestionBufferSize - 1);
                    ingested = voice.ingestionBuffer[readPos] * signalFlux;
                }
                else
                {
                    // Self-feeding: internal noise substrate
                    float noise = voice.noiseGen.process();

                    // Noise color: tilt the spectrum via simple one-pole
                    // noiseColor 0 = dark (LP), 0.5 = white, 1.0 = bright (HP)
                    voice.ingestionFilter.setCoefficients (
                        enzymeSelect + externalFilterMod * 2000.0f,
                        0.3f + noiseColor * 0.4f,
                        static_cast<float> (sr));
                    ingested = voice.ingestionFilter.processSample (noise) * signalFlux;
                }

                // --- CATABOLISM ---
                voice.entropy.pushSample (ingested);

                // --- ECONOMY ---
                voice.economy.update (metabolicRate, signalFlux,
                                      voice.entropy.getEntropy(),
                                      externalRhythmMod, externalDecayMod,
                                      voice.released);

                // Check if organism has fully decayed
                if (voice.released && voice.economy.getFreeEnergy() < 0.001f)
                {
                    voice.active = false;
                    continue;
                }

                // --- ANABOLISM ---
                float fundamental = midiToFreq (voice.noteNumber) +
                                    externalPitchMod * 20.0f; // ±10 semitones at mod=±0.5
                if (fundamental < 20.0f) fundamental = 20.0f;

                float effectiveIsotope = clamp (isotopeBalance + externalMorphMod * 0.3f, 0.0f, 1.0f);

                voice.modes.setFundamental (fundamental, effectiveIsotope);
                voice.modes.updateWeights (voice.entropy.getSpectralCentroid(),
                                           voice.economy.getFreeEnergy(),
                                           catalystDrive,
                                           voice.entropy.getEntropy(),
                                           effectiveIsotope);

                float sample = voice.modes.processSample (gamma);

                // Soft clip
                sample = fastTanh (sample);

                // Voice steal crossfade
                if (voice.stealFadeStep > 0.0f)
                {
                    voice.stealFadeGain -= voice.stealFadeStep;
                    if (voice.stealFadeGain <= 0.0f)
                    {
                        voice.active = false;
                        continue;
                    }
                    sample *= voice.stealFadeGain;
                }

                // Velocity scaling
                sample *= voice.velocity;

                mixL += sample;
                mixR += sample;
            }

            // Cache output for coupling
            outputCacheL[static_cast<size_t> (s)] = mixL;
            outputCacheR[static_cast<size_t> (s)] = mixR;

            // Write to output buffer
            if (buffer.getNumChannels() > 0)
                buffer.getWritePointer (0)[s] += mixL;
            if (buffer.getNumChannels() > 1)
                buffer.getWritePointer (1)[s] += mixR;
        }

        // Reset coupling accumulators after consumption
        externalPitchMod = 0.0f;
        externalFilterMod = 0.0f;
        externalMorphMod = 0.0f;
        externalRhythmMod = 0.0f;
        externalDecayMod = 0.0f;
        couplingAudioActive = false;

        // Update active voice count (safe to read from message thread)
        int count = 0;
        for (const auto& voice : voices)
            if (voice.active) ++count;
        activeVoiceCount.store (count, std::memory_order_relaxed);
    }

    //-- Coupling --------------------------------------------------------------

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0 || sampleIndex >= static_cast<int> (outputCacheL.size()))
            return 0.0f;

        return (channel == 0) ? outputCacheL[static_cast<size_t> (sampleIndex)]
                              : outputCacheR[static_cast<size_t> (sampleIndex)];
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int numSamples) override
    {
        switch (type)
        {
            case CouplingType::AudioToFM:
            case CouplingType::AudioToWavetable:
            {
                // Write source audio into all active voices' ingestion buffers
                if (sourceBuffer != nullptr && numSamples > 0)
                {
                    for (auto& voice : voices)
                    {
                        if (!voice.active) continue;
                        for (int s = 0; s < numSamples; ++s)
                        {
                            voice.ingestionBuffer[voice.ingestionWritePos] = sourceBuffer[s] * amount;
                            voice.ingestionWritePos = (voice.ingestionWritePos + 1)
                                                      & (OrganonVoice::kIngestionBufferSize - 1);
                        }
                        voice.hasCouplingInput = true;
                    }
                    couplingAudioActive = true;
                }
                break;
            }

            case CouplingType::RhythmToBlend:
                externalRhythmMod += amount;
                break;

            case CouplingType::EnvToDecay:
                externalDecayMod += amount;
                break;

            case CouplingType::AmpToFilter:
                externalFilterMod += amount;
                break;

            case CouplingType::EnvToMorph:
                externalMorphMod += amount;
                break;

            case CouplingType::LFOToPitch:
            case CouplingType::PitchToPitch:
                externalPitchMod += amount * 0.5f;
                break;

            default:
                break; // AmpToPitch, AudioToRing, FilterToFilter, AmpToChoke unsupported
        }
    }

    //-- Parameters ------------------------------------------------------------

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

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramMetabolicRate  = apvts.getRawParameterValue ("organon_metabolicRate");
        paramEnzymeSelect   = apvts.getRawParameterValue ("organon_enzymeSelect");
        paramCatalystDrive  = apvts.getRawParameterValue ("organon_catalystDrive");
        paramDampingCoeff   = apvts.getRawParameterValue ("organon_dampingCoeff");
        paramSignalFlux     = apvts.getRawParameterValue ("organon_signalFlux");
        paramPhasonShift    = apvts.getRawParameterValue ("organon_phasonShift");
        paramIsotopeBalance = apvts.getRawParameterValue ("organon_isotopeBalance");
        paramLockIn         = apvts.getRawParameterValue ("organon_lockIn");
        paramMembrane       = apvts.getRawParameterValue ("organon_membrane");
        paramNoiseColor     = apvts.getRawParameterValue ("organon_noiseColor");
    }

private:
    //-- Parameter definitions -------------------------------------------------

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // 1. Metabolic Rate — speed of energy turnover (Hz)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_metabolicRate", 1), "Metabolic Rate",
            juce::NormalisableRange<float> (0.1f, 10.0f, 0.0f, 0.5f), 1.0f));

        // 2. Enzyme Selectivity — bandpass center on ingestion (Hz)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_enzymeSelect", 1), "Enzyme Selectivity",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.3f), 1000.0f));

        // 3. Catalyst Drive — gain on modal driving force
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_catalystDrive", 1), "Catalyst Drive",
            juce::NormalisableRange<float> (0.0f, 2.0f), 0.5f));

        // 4. Damping Coefficient — damping in the modal ODE
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_dampingCoeff", 1), "Damping",
            juce::NormalisableRange<float> (0.01f, 0.99f), 0.3f));

        // 5. Signal Flux — gain of ingestion input
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_signalFlux", 1), "Signal Flux",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // 6. Phason Shift — temporal offset between metabolic cycles
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_phasonShift", 1), "Phason Shift",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // 7. Isotope Balance — spectral weighting (sub ↔ upper)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_isotopeBalance", 1), "Isotope Balance",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // 8. Lock-in Strength — sync to SharedTransport tempo
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_lockIn", 1), "Lock-in Strength",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // 9. Membrane Porosity — diffusion/reverb send
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_membrane", 1), "Membrane Porosity",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));

        // 10. Noise Color — spectral tilt of internal noise (dark ↔ bright)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_noiseColor", 1), "Noise Color",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    }

    //-- Voice management ------------------------------------------------------

    void handleNoteOn (int note, float velocity) noexcept
    {
        ++noteCounter;

        // Find a free voice
        int freeSlot = -1;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[i].active)
            {
                freeSlot = i;
                break;
            }
        }

        // If no free voice, steal the oldest (LRU)
        if (freeSlot < 0)
        {
            uint64_t oldest = UINT64_MAX;
            freeSlot = 0;
            for (int i = 0; i < kMaxVoices; ++i)
            {
                if (voices[i].startTime < oldest)
                {
                    oldest = voices[i].startTime;
                    freeSlot = i;
                }
            }
            // Begin 5ms crossfade on stolen voice
            voices[freeSlot].beginStealFade (sr);
        }

        voices[freeSlot].noteOn (note, velocity, noteCounter, sr);
    }

    void handleNoteOff (int note) noexcept
    {
        for (auto& voice : voices)
        {
            if (voice.active && !voice.released && voice.noteNumber == note)
            {
                voice.noteOff();
                break; // release only one voice per note-off
            }
        }
    }

    //-- State -----------------------------------------------------------------

    double sr = 44100.0;
    int blockSize = 512;
    uint64_t noteCounter = 0;

    std::array<OrganonVoice, kMaxVoices> voices;
    std::atomic<int> activeVoiceCount { 0 };

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Coupling accumulators (reset after each block)
    float externalPitchMod = 0.0f;
    float externalFilterMod = 0.0f;
    float externalMorphMod = 0.0f;
    float externalRhythmMod = 0.0f;
    float externalDecayMod = 0.0f;
    bool couplingAudioActive = false;

    // Cached parameter pointers (set by attachParameters)
    std::atomic<float>* paramMetabolicRate  = nullptr;
    std::atomic<float>* paramEnzymeSelect   = nullptr;
    std::atomic<float>* paramCatalystDrive  = nullptr;
    std::atomic<float>* paramDampingCoeff   = nullptr;
    std::atomic<float>* paramSignalFlux     = nullptr;
    std::atomic<float>* paramPhasonShift    = nullptr;
    std::atomic<float>* paramIsotopeBalance = nullptr;
    std::atomic<float>* paramLockIn         = nullptr;
    std::atomic<float>* paramMembrane       = nullptr;
    std::atomic<float>* paramNoiseColor     = nullptr;
};

} // namespace xomnibus
