#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/SharedTransport.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/EngineProfiler.h"
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
// MetabolicEconomy — Free Energy Pool tracking with Active Inference (VFE).
//
// v2 upgrade: The organism now maintains a generative model of its input.
// It predicts incoming entropy and adjusts its internal state to minimize
// Variational Free Energy (prediction error). This creates genuinely
// adaptive behavior — the organism "learns" its diet and anticipates it.
//
// VFE = D_KL[q(s) || p(s|o)] + complexity
//     ≈ (predicted_entropy - observed_entropy)^2 + lambda * |belief_change|
//
// When VFE is high: the organism is surprised → it adapts faster.
// When VFE is low: the organism has learned its environment → stable output.
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

        // VFE state
        predictedEntropy = 0.5f;
        entropyVariance = 0.1f;
        beliefRate = 0.5f;
        surprise = 0.0f;
        vfe = 0.0f;
        adaptationGain = 1.0f;
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

        // === ACTIVE INFERENCE: Variational Free Energy minimization ===

        // 1. Compute prediction error (surprise)
        float predictionError = entropyValue - predictedEntropy;
        surprise = predictionError * predictionError;

        // 2. Compute precision-weighted prediction error
        //    Precision = 1/variance — how confident the organism is in its prediction
        float precision = 1.0f / (entropyVariance + 0.01f);
        float weightedError = surprise * precision;

        // 3. Compute VFE = weighted prediction error + complexity penalty
        //    Complexity penalty penalizes rapid belief changes (keeps the organism stable)
        float complexityPenalty = beliefRate * beliefRate * 0.1f;
        vfe = weightedError + complexityPenalty;

        // 4. Update beliefs (Bayesian belief update via gradient descent on VFE)
        //    The organism adjusts its prediction toward the observed entropy
        //    Learning rate scales with metabolic rate — faster metabolism = faster learning
        float learningRate = clamp (metabolicRate * 0.05f, 0.001f, 0.2f);
        float beliefDelta = learningRate * predictionError;
        predictedEntropy += beliefDelta;
        predictedEntropy = clamp (predictedEntropy, 0.0f, 1.0f);

        // Track belief change rate (for complexity penalty)
        beliefRate = flushDenormal (beliefRate * 0.95f + std::fabs (beliefDelta) * 0.05f);

        // 5. Update variance estimate (how uncertain the organism is)
        //    Exponential moving average of squared prediction errors
        entropyVariance = flushDenormal (entropyVariance * 0.98f + surprise * 0.02f);
        entropyVariance = clamp (entropyVariance, 0.001f, 1.0f);

        // 6. Adaptation gain: high VFE → organism adapts faster (more responsive)
        //    low VFE → organism is settled (more stable, richer harmonics)
        float targetAdaptation = clamp (1.0f - vfe * 2.0f, 0.2f, 1.0f);
        adaptationGain = flushDenormal (adaptationGain * 0.97f + targetAdaptation * 0.03f);

        // === METABOLIC ECONOMY (now modulated by VFE) ===

        // Effective metabolic rate (modulated by external coupling + VFE)
        // High surprise → metabolic rate increases (fight-or-flight response)
        float vfeMod = 1.0f + surprise * 2.0f;
        float effectiveRate = metabolicRate * (1.0f + externalDecayMod) * vfeMod;
        if (effectiveRate < 0.01f) effectiveRate = 0.01f;

        // Accumulation vs depletion
        // Adaptation gain modulates intake — settled organisms extract more energy
        float intake = entropyValue * signalFlux * adaptationGain * (1.0f + externalRhythmMod);
        float cost = effectiveRate * 0.1f;

        // Starvation on note release — 4x metabolic rate
        if (noteReleased)
            cost *= 4.0f;

        freeEnergy += (intake - cost) * dt;
        freeEnergy = clamp (freeEnergy, 0.0f, 1.0f);
    }

    float getFreeEnergy() const noexcept { return freeEnergy; }
    void setFreeEnergy (float e) noexcept { freeEnergy = clamp (e, 0.0f, 1.0f); }

    // VFE readouts for coupling and UI
    float getSurprise() const noexcept { return surprise; }
    float getVFE() const noexcept { return vfe; }
    float getAdaptationGain() const noexcept { return adaptationGain; }
    float getPredictedEntropy() const noexcept { return predictedEntropy; }

private:
    double sr = 44100.0;
    int controlRateDiv = 22;
    int controlCounter = 0;
    float freeEnergy = 0.0f;

    // VFE / Active Inference state
    float predictedEntropy = 0.5f;   // q(s): organism's belief about expected entropy
    float entropyVariance = 0.1f;    // uncertainty in prediction
    float beliefRate = 0.5f;         // rate of belief change (for complexity penalty)
    float surprise = 0.0f;           // squared prediction error
    float vfe = 0.0f;                // variational free energy (total)
    float adaptationGain = 1.0f;     // how settled the organism is (1.0 = stable)
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

    // Phason shift: per-voice phase offset for metabolic control-rate stagger
    int phasonOffset = 0;

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

    //-- Transport -------------------------------------------------------------

    // Called by the processor to give Organon access to SharedTransport.
    // Must be called before the first renderBlock().
    void setSharedTransport (const SharedTransport* transport) noexcept
    {
        sharedTransport = transport;
    }

    // Reverb send level — read by the processor to route to shared reverb bus.
    // Updated once per block in renderBlock().
    float getReverbSendLevel() const noexcept
    {
        return reverbSendLevel.load (std::memory_order_relaxed);
    }

    //-- Lifecycle -------------------------------------------------------------

    // Return the profiler for UI/diagnostic reads.
    const EngineProfiler& getProfiler() const noexcept { return profiler; }

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        blockSize = maxBlockSize;
        noteCounter = 0;

        for (auto& voice : voices)
            voice.prepare (sampleRate, maxBlockSize);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        profiler.prepare (sampleRate, maxBlockSize);
        profiler.setCpuBudgetFraction (0.22f); // Organon's 22% CPU budget
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
        phasonClock = 0.0f;
        reverbSendLevel.store (0.0f, std::memory_order_relaxed);
    }

    //-- Audio -----------------------------------------------------------------

    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        EngineProfiler::ScopedMeasurement measurement (profiler);

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

        // --- LOCK-IN: Sync metabolic rate to SharedTransport tempo ---
        // lockIn 0.0 = free-running, 1.0 = fully quantized to nearest beat subdivision.
        // We find the tempo subdivision closest to the current metabolic rate and
        // crossfade between the free rate and the quantized rate.
        float lockedMetabolicRate = metabolicRate;
        float lockInTransportPhase = 0.0f;
        if (lockIn > 0.001f && sharedTransport != nullptr)
        {
            double bpm = sharedTransport->getBPM();
            if (bpm > 0.0)
            {
                // Convert metabolic rate (Hz) to the closest beat subdivision.
                // beatFreq = BPM/60. Subdivisions: 1/4, 1/2, 1, 2, 4 beats.
                float beatFreq = static_cast<float> (bpm / 60.0);
                // Available subdivision rates (in Hz)
                float subdivisions[5] = {
                    beatFreq * 4.0f,   // 16th notes
                    beatFreq * 2.0f,   // 8th notes
                    beatFreq,          // quarter notes
                    beatFreq * 0.5f,   // half notes
                    beatFreq * 0.25f   // whole notes
                };

                // Find closest subdivision to current metabolic rate
                float closestRate = subdivisions[0];
                float closestDivision = 0.25f; // in beats
                float bestDist = std::fabs (metabolicRate - subdivisions[0]);
                float divisionBeats[5] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
                for (int i = 1; i < 5; ++i)
                {
                    float dist = std::fabs (metabolicRate - subdivisions[i]);
                    if (dist < bestDist)
                    {
                        bestDist = dist;
                        closestRate = subdivisions[i];
                        closestDivision = divisionBeats[i];
                    }
                }

                // Crossfade: free rate → quantized rate based on lockIn amount
                lockedMetabolicRate = metabolicRate + lockIn * (closestRate - metabolicRate);

                // Get transport phase for the locked division (used by phason clock)
                lockInTransportPhase = static_cast<float> (
                    sharedTransport->getPhaseForDivision (closestDivision));
            }
        }

        // Damping mapped from parameter (0.01-0.99) → actual gamma value
        // Higher param = more damping = shorter tail
        float gamma = dampingCoeff * 20.0f; // scale to useful range for ODE

        // Phason Shift: compute per-voice metabolic rate modulation.
        // Each voice gets an evenly-spaced phase offset; phasonShift controls
        // the modulation depth. At 0.0 all voices share the same rate (no pulse).
        // At 1.0 each voice pulses fully out of phase with the others.
        // When lock-in is active, the phason clock syncs to transport phase.
        float phasonMods[kMaxVoices];
        if (phasonShift > 0.001f)
        {
            if (lockIn > 0.5f && lockInTransportPhase >= 0.0f)
            {
                // Lock phason clock to transport phase for tempo-synced pulsing
                float targetPhase = lockInTransportPhase;
                // Smooth transition to avoid clicks
                float phaseDiff = targetPhase - phasonClock;
                if (phaseDiff > 0.5f) phaseDiff -= 1.0f;
                if (phaseDiff < -0.5f) phaseDiff += 1.0f;
                phasonClock += phaseDiff * lockIn * 0.3f;
                if (phasonClock < 0.0f) phasonClock += 1.0f;
                if (phasonClock > 1.0f) phasonClock -= 1.0f;
            }
            else
            {
                // Free-running: advance at locked metabolic rate
                float phasonCycleSamples = static_cast<float> (sr) / std::max (lockedMetabolicRate, 0.1f);
                float phasonInc = static_cast<float> (numSamples) / phasonCycleSamples;
                phasonClock += phasonInc;
                if (phasonClock > 1.0f) phasonClock -= std::floor (phasonClock);
            }

            constexpr float twoPi = 6.28318530717958647692f;
            for (int i = 0; i < kMaxVoices; ++i)
            {
                // Evenly space voices across the cycle
                float voicePhase = phasonClock + static_cast<float> (i) / static_cast<float> (kMaxVoices);
                if (voicePhase > 1.0f) voicePhase -= 1.0f;
                // Sinusoidal modulation: ±phasonShift of metabolic rate
                phasonMods[i] = std::sin (twoPi * voicePhase) * phasonShift;
            }
        }
        else
        {
            for (int i = 0; i < kMaxVoices; ++i)
                phasonMods[i] = 0.0f;
        }

        // Process each sample
        for (int s = 0; s < numSamples; ++s)
        {
            float mixL = 0.0f;
            float mixR = 0.0f;

            int voiceIdx = 0;
            for (auto& voice : voices)
            {
                if (!voice.active)
                {
                    ++voiceIdx;
                    continue;
                }

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
                // Locked + phason-shifted metabolic rate: each voice pulses at a different phase
                float voiceMetabolicRate = lockedMetabolicRate * (1.0f + phasonMods[voiceIdx]);
                if (voiceMetabolicRate < 0.05f) voiceMetabolicRate = 0.05f;

                voice.economy.update (voiceMetabolicRate, signalFlux,
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

                // VFE modulates isotope balance: surprise shifts spectral character
                float surpriseShift = voice.economy.getSurprise() * 0.15f;
                float effectiveIsotope = clamp (isotopeBalance + externalMorphMod * 0.3f + surpriseShift, 0.0f, 1.0f);

                voice.modes.setFundamental (fundamental, effectiveIsotope);

                // VFE adaptation gain modulates catalyst effectiveness:
                // settled organisms (low VFE) get full catalyst drive
                // surprised organisms redirect energy to adaptation
                float effectiveCatalyst = catalystDrive * voice.economy.getAdaptationGain();

                voice.modes.updateWeights (voice.entropy.getSpectralCentroid(),
                                           voice.economy.getFreeEnergy(),
                                           effectiveCatalyst,
                                           voice.entropy.getEntropy(),
                                           effectiveIsotope);

                float sample = voice.modes.processSample (gamma);

                // Soft clip
                sample = fastTanh (sample);

                // Voice steal crossfade
                if (voice.stealFadeStep > 0.0f)
                {
                    voice.stealFadeGain -= voice.stealFadeStep;
                    voice.stealFadeGain = flushDenormal (voice.stealFadeGain);
                    if (voice.stealFadeGain <= 0.0f)
                    {
                        voice.active = false;
                        continue;
                    }
                    sample *= voice.stealFadeGain;
                }

                // Velocity scaling
                sample *= voice.velocity;

                // VFE-driven stereo spread: surprise widens the image slightly
                float spread = voice.economy.getSurprise() * 0.15f;
                mixL += sample * (1.0f - spread);
                mixR += sample * (1.0f + spread);

                ++voiceIdx;
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

        // Update active voice count and membrane reverb send level
        int count = 0;
        float avgSurprise = 0.0f;
        for (const auto& voice : voices)
        {
            if (voice.active)
            {
                ++count;
                avgSurprise += voice.economy.getSurprise();
            }
        }
        activeVoiceCount.store (count, std::memory_order_relaxed);

        // Membrane porosity → reverb send level
        // Base send = membrane param, modulated by average surprise across voices
        // Surprised organisms "sweat" more into the reverb (up to +30% extra send)
        if (count > 0)
            avgSurprise /= static_cast<float> (count);
        float sendLevel = membrane + avgSurprise * 0.3f;
        if (sendLevel > 1.0f) sendLevel = 1.0f;
        reverbSendLevel.store (sendLevel, std::memory_order_relaxed);
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

    // Phason Shift clock (0.0-1.0, advances at metabolic rate)
    float phasonClock = 0.0f;

    // Lock-in: pointer to shared transport (set externally by processor)
    const SharedTransport* sharedTransport = nullptr;

    // Membrane: reverb send level computed per block (read by processor)
    std::atomic<float> reverbSendLevel { 0.0f };

    // Coupling accumulators (reset after each block)
    float externalPitchMod = 0.0f;
    float externalFilterMod = 0.0f;
    float externalMorphMod = 0.0f;
    float externalRhythmMod = 0.0f;
    float externalDecayMod = 0.0f;
    bool couplingAudioActive = false;

    // Performance profiler
    EngineProfiler profiler;

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
