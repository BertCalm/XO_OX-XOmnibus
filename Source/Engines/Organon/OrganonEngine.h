#pragma once

//==============================================================================
//
//  ORGANON ENGINE — Informational Dissipative Synthesis
//  XO_OX Designs | XOmnibus Multi-Engine Synthesizer
//
//  Creature: The Deep-Sea Chemotroph
//  Depth:    Oscar-leaning — The Deep (slow, chemical, alien metabolism)
//  Accent:   Bioluminescent Cyan #00CED1
//  Prefix:   organon_
//
//  Historical Lineage:
//    Organon channels the spirit of biological DSP — where Iannis Xenakis
//    formalized stochastic processes as music (GENDYN, 1991), and where
//    Karl Sims evolved virtual creatures through energy-driven physics
//    simulations. The Port-Hamiltonian modal array descends from Julius O.
//    Smith III's modal synthesis work (Physical Audio Signal Processing,
//    Stanford CCRMA), while the Active Inference / Variational Free Energy
//    framework draws from Karl Friston's neuroscience (The Free-Energy
//    Principle, 2010). Organon fuses these lineages: a living organism
//    that metabolizes audio into harmonic structure.
//
//  Aquatic Mythology:
//    XOrganon is the chemotroph of the deep — a Fourth Generation species
//    born from feliX and Oscar's coupling. It lives below the thermocline
//    where sunlight never reaches. Like deep-sea tube worms that feed on
//    hydrothermal vent chemistry, Organon consumes audio signals and
//    converts their information content (entropy) into living harmonic
//    structures. No two performances sound the same because the organism
//    accumulates metabolic state from its history of coupling inputs.
//    When starved, it dims. When fed rich signal, it blooms.
//
//  Synthesis Technique:
//    1. INGESTION  — Audio enters through enzyme-selective bandpass filter
//    2. CATABOLISM — Shannon entropy analysis extracts information content
//    3. ECONOMY    — Free energy pool with Active Inference (VFE minimization)
//    4. ANABOLISM  — 32-mode Port-Hamiltonian oscillator array driven by energy
//
//  Coupling Symbiosis:
//    Organon is the ultimate coupling receiver — it literally eats the output
//    of other engines. Feed it ODDFELIX transients for rhythmic metabolism.
//    Feed it ODDOSCAR pads for slow, evolving harmonic growth. Feed it
//    OUROBOROS feedback for self-consuming recursive textures.
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../Core/SharedTransport.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/EngineProfiler.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <cstring>
#include <atomic>

namespace xomnibus {

//==============================================================================
//  SECTION 1: NOISE SUBSTRATE
//==============================================================================
//
// OrganonNoiseGen — xorshift32 PRNG for internal noise substrate.
//
// When no coupling partner is feeding audio to the organism, it generates
// its own internal noise as a "self-feeding" substrate. This is the
// chemotroph's equivalent of metabolizing minerals from the ocean floor
// when no prey drifts down from above.
//
// The xorshift32 algorithm (Marsaglia, 2003) provides a fast, statistically
// adequate noise source with a period of 2^32 - 1.
//==============================================================================
class OrganonNoiseGen
{
public:
    void seed (uint32_t seedValue) noexcept { state = seedValue ? seedValue : 1; }

    float process() noexcept
    {
        // Marsaglia xorshift32: three shift-XOR operations produce
        // the full 2^32-1 period. The shift constants (13, 17, 5) are
        // from Marsaglia's original 2003 paper "Xorshift RNGs."
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;

        // Convert uint32 to float in [-1, 1] range.
        // 2147483648.0f = 2^31, the signed integer half-range.
        return static_cast<float> (static_cast<int32_t> (state)) / 2147483648.0f;
    }

private:
    uint32_t state = 1;
};

//==============================================================================
//  SECTION 2: CATABOLISM — ENTROPY ANALYSIS
//==============================================================================
//
// EntropyAnalyzer — Shannon entropy from short-time amplitude histogram.
//
// The catabolism stage of the organism. Just as a biological cell breaks
// down food to extract usable energy, this module breaks down audio signal
// to extract its information content (entropy).
//
// High entropy (noisy/complex input) = rich "nutrition" for the organism.
// Low entropy (simple/tonal input) = sparse nutrition, slower growth.
//
// Algorithm:
//   1. Quantize samples into 32 amplitude bins
//   2. Build histogram over a 256-sample analysis window
//   3. Compute Shannon entropy: H = -sum(p_i * log2(p_i))
//   4. Normalize to [0, 1] via H / log2(32)
//
// The spectral centroid provides a secondary analysis dimension: where in
// the amplitude distribution the energy concentrates. This feeds the
// modal array's weight distribution, biasing harmonic reconstruction
// toward the spectral character of the ingested signal.
//==============================================================================
class EntropyAnalyzer
{
public:
    static constexpr int kNumBins = 32;           // Amplitude histogram resolution
    static constexpr int kDefaultWindowSize = 256; // ~5.8ms at 44.1kHz — balances responsiveness with stability
    static constexpr int kSmallWindowSize = 64;    // ~1.5ms — for high-frequency enzyme selectivity (faster tracking)
    static constexpr float kMaxEntropy = 5.0f;     // log2(32) = 5.0 — theoretical maximum for 32-bin distribution

    void prepare (double sampleRate) noexcept
    {
        cachedSampleRate = sampleRate;
        // Control rate ~2kHz: entropy computation is expensive, so we
        // downsample the analysis to every N audio samples.
        controlRateDivisor = std::max (1, static_cast<int> (sampleRate / 2000.0));
        reset();
    }

    void reset() noexcept
    {
        writePosition = 0;
        controlCounter = 0;
        entropyValue = 0.0f;
        spectralCentroid = 0.5f;
        std::memset (ringBuffer, 0, sizeof (ringBuffer));
        std::memset (histogram, 0, sizeof (histogram));
    }

    // Feed a sample into the analysis buffer. Call every audio sample.
    void pushSample (float sample) noexcept
    {
        ringBuffer[writePosition] = sample;
        writePosition = (writePosition + 1) & (kMaxWindowSize - 1); // Power-of-2 wrap

        if (++controlCounter >= controlRateDivisor)
        {
            controlCounter = 0;
            computeEntropy (currentWindowSize);
        }
    }

    // Set analysis window size based on enzyme selectivity frequency.
    // High-frequency diet (>10kHz) uses the smaller 64-sample window
    // for faster transient response; lower frequencies use the full
    // 256-sample window for more stable entropy readings.
    void setWindowSize (float enzymeFrequency) noexcept
    {
        currentWindowSize = (enzymeFrequency > 10000.0f) ? kSmallWindowSize : kDefaultWindowSize;
    }

    float getEntropy() const noexcept { return entropyValue; }
    float getSpectralCentroid() const noexcept { return spectralCentroid; }

private:
    static constexpr int kMaxWindowSize = 256; // Must be power of 2 for bitwise wrap

    void computeEntropy (int windowSize) noexcept
    {
        std::memset (histogram, 0, sizeof (histogram));

        int startPosition = (writePosition - windowSize + kMaxWindowSize) & (kMaxWindowSize - 1);
        float inverseWindowSize = 1.0f / static_cast<float> (windowSize);

        // Build amplitude histogram: map each sample to one of 32 bins
        for (int i = 0; i < windowSize; ++i)
        {
            int bufferIndex = (startPosition + i) & (kMaxWindowSize - 1);
            float sampleValue = ringBuffer[bufferIndex];

            // Map sample from [-1, 1] to bin index [0, kNumBins-1]
            int bin = static_cast<int> ((sampleValue * 0.5f + 0.5f) * static_cast<float> (kNumBins - 1));
            if (bin < 0) bin = 0;
            if (bin >= kNumBins) bin = kNumBins - 1;
            histogram[bin]++;
        }

        // Compute Shannon entropy H = -sum(p_i * log2(p_i))
        // and amplitude-weighted spectral centroid simultaneously
        float entropy = 0.0f;
        float centroidNumerator = 0.0f;
        float centroidDenominator = 0.0f;

        for (int i = 0; i < kNumBins; ++i)
        {
            if (histogram[i] > 0)
            {
                float probability = static_cast<float> (histogram[i]) * inverseWindowSize;
                entropy -= probability * fastLog2 (probability);
                float normalizedBinPosition = static_cast<float> (i) / static_cast<float> (kNumBins - 1);
                centroidNumerator += normalizedBinPosition * probability;
                centroidDenominator += probability;
            }
        }

        // Normalize entropy to [0, 1] by dividing by theoretical maximum
        entropyValue = clamp (entropy / kMaxEntropy, 0.0f, 1.0f);
        spectralCentroid = (centroidDenominator > 0.0001f) ? (centroidNumerator / centroidDenominator) : 0.5f;
    }

    double cachedSampleRate = 44100.0;
    int controlRateDivisor = 22;  // ~2kHz control rate at 44.1kHz sample rate
    int controlCounter = 0;
    int writePosition = 0;
    int currentWindowSize = kDefaultWindowSize;
    float ringBuffer[kMaxWindowSize] {};
    int histogram[kNumBins] {};
    float entropyValue = 0.0f;
    float spectralCentroid = 0.5f; // 0.5 = centered (no bias toward low or high bins)
};

//==============================================================================
//  SECTION 3: METABOLIC ECONOMY — FREE ENERGY WITH ACTIVE INFERENCE
//==============================================================================
//
// MetabolicEconomy — Free Energy Pool tracking with Active Inference (VFE).
//
// The organism's energy system. This is the heart of the chemotroph
// metaphor: the organism maintains an internal pool of "free energy"
// that rises when fed high-entropy signal and depletes when starved.
// The free energy level directly controls how loudly and richly the
// modal array reconstructs harmonic content.
//
// v2 upgrade: Active Inference via Variational Free Energy minimization.
// The organism now maintains a generative model of its input — it
// predicts incoming entropy and adjusts its internal state to minimize
// prediction error. This creates genuinely adaptive behavior:
// the organism "learns" its diet and anticipates it.
//
// VFE = D_KL[q(s) || p(s|o)] + complexity
//     ~ (predicted_entropy - observed_entropy)^2 + lambda * |belief_change|
//
// When VFE is high: the organism is surprised -> it adapts faster.
// When VFE is low: the organism has learned its environment -> stable output.
//
// This is Karl Friston's Free-Energy Principle (2010) applied to DSP:
// the organism minimizes surprise by updating its internal model to
// match the statistical structure of its audio environment.
//==============================================================================
class MetabolicEconomy
{
public:
    void prepare (double sampleRate) noexcept
    {
        cachedSampleRate = sampleRate;
        // Control rate ~2kHz: metabolic updates don't need audio-rate precision
        controlRateDivisor = std::max (1, static_cast<int> (sampleRate / 2000.0));
        reset();
    }

    void reset() noexcept
    {
        freeEnergy = 0.0f;
        controlCounter = 0;

        // VFE state — initial beliefs
        predictedEntropy = 0.5f;  // Start with moderate expectation
        entropyVariance = 0.1f;   // Moderate initial uncertainty
        beliefChangeRate = 0.5f;  // Moderate initial belief momentum
        surprise = 0.0f;
        variationalFreeEnergy = 0.0f;
        adaptationGain = 1.0f;    // Start fully settled
    }

    // Update once per control tick.
    // metabolicRate: Hz (0.1-10), signalFlux: 0-1, entropyValue: 0-1
    void update (float metabolicRate, float signalFlux, float entropyValue,
                 float externalRhythmModulation, float externalDecayModulation,
                 bool noteReleased) noexcept
    {
        if (++controlCounter < controlRateDivisor)
            return;
        controlCounter = 0;

        float deltaTime = static_cast<float> (controlRateDivisor) / static_cast<float> (cachedSampleRate);

        // =====================================================================
        // ACTIVE INFERENCE: Variational Free Energy minimization
        // (Friston's Free-Energy Principle applied to metabolic DSP)
        // =====================================================================

        // Step 1: Compute prediction error (surprise)
        // How much does the observed entropy differ from what we predicted?
        float predictionError = entropyValue - predictedEntropy;
        surprise = predictionError * predictionError;

        // Step 2: Precision-weighted prediction error
        // Precision = 1/variance — how confident the organism is in its model.
        // High confidence + large error = very surprising.
        // 0.01f floor prevents division by zero.
        float precision = 1.0f / (entropyVariance + 0.01f);
        float weightedError = surprise * precision;

        // Step 3: VFE = weighted prediction error + complexity penalty
        // Complexity penalty discourages rapid belief changes (Occam's razor
        // for the organism's internal model — prefer simple, stable beliefs).
        // 0.1f scales the penalty to a useful range relative to prediction error.
        float complexityPenalty = beliefChangeRate * beliefChangeRate * 0.1f;
        variationalFreeEnergy = weightedError + complexityPenalty;

        // Step 4: Bayesian belief update via gradient descent on VFE
        // The organism adjusts its prediction toward observed entropy.
        // Learning rate scales with metabolic rate: faster metabolism = faster learning.
        // 0.05f scaling keeps learning rate in stable range [0.001, 0.2].
        float learningRate = clamp (metabolicRate * 0.05f, 0.001f, 0.2f);
        float beliefDelta = learningRate * predictionError;
        predictedEntropy += beliefDelta;
        predictedEntropy = clamp (predictedEntropy, 0.0f, 1.0f);

        // Track belief change rate for the complexity penalty.
        // 0.95/0.05 EMA: slow-moving average of absolute belief changes.
        // flushDenormal prevents tiny values from accumulating as denormals
        // in the feedback path — without this, the multiplied-by-0.95 decay
        // can produce subnormal floats that trigger CPU performance penalties.
        beliefChangeRate = flushDenormal (beliefChangeRate * 0.95f + std::fabs (beliefDelta) * 0.05f);

        // Step 5: Update variance estimate (organism's uncertainty about its model).
        // 0.98/0.02 EMA of squared prediction errors — slower than belief updates
        // because uncertainty should change more gradually than beliefs.
        // Clamped to [0.001, 1.0] to prevent pathological confidence or chaos.
        entropyVariance = flushDenormal (entropyVariance * 0.98f + surprise * 0.02f);
        entropyVariance = clamp (entropyVariance, 0.001f, 1.0f);

        // Step 6: Adaptation gain — how settled the organism is.
        // High VFE (surprised) -> low gain (organism redirects energy to adaptation).
        // Low VFE (settled) -> high gain (organism uses energy for rich harmonics).
        // 2.0f scaling maps VFE to a useful reduction range.
        // 0.97/0.03 EMA: smooth transition prevents abrupt timbral jumps.
        float targetAdaptation = clamp (1.0f - variationalFreeEnergy * 2.0f, 0.2f, 1.0f);
        adaptationGain = flushDenormal (adaptationGain * 0.97f + targetAdaptation * 0.03f);

        // =====================================================================
        // METABOLIC ECONOMY (now modulated by Active Inference state)
        // =====================================================================

        // Effective metabolic rate, modulated by:
        //   - External coupling (decay modulation from partner engines)
        //   - VFE surprise (fight-or-flight: surprise accelerates metabolism)
        // 2.0f surprise multiplier: a fully surprised organism metabolizes 3x faster.
        float vfeModulation = 1.0f + surprise * 2.0f;
        float effectiveRate = metabolicRate * (1.0f + externalDecayModulation) * vfeModulation;
        if (effectiveRate < 0.01f) effectiveRate = 0.01f;

        // Energy balance: intake from signal vs metabolic cost
        // Adaptation gain modulates intake — settled organisms extract more energy
        // from the same signal (they've learned to digest it efficiently).
        float intake = entropyValue * signalFlux * adaptationGain * (1.0f + externalRhythmModulation);
        float cost = effectiveRate * 0.1f; // 0.1f scales rate to energy-per-tick

        // Starvation on note release — 4x metabolic cost accelerates decay.
        // The organism burns through its reserves when the note ends,
        // creating a natural fade-out tied to internal state rather than
        // a fixed envelope time.
        if (noteReleased)
            cost *= 4.0f;

        freeEnergy += (intake - cost) * deltaTime;
        freeEnergy = clamp (freeEnergy, 0.0f, 1.0f);
    }

    float getFreeEnergy() const noexcept { return freeEnergy; }
    void setFreeEnergy (float energy) noexcept { freeEnergy = clamp (energy, 0.0f, 1.0f); }

    // VFE readouts for coupling and UI
    float getSurprise() const noexcept { return surprise; }
    float getVFE() const noexcept { return variationalFreeEnergy; }
    float getAdaptationGain() const noexcept { return adaptationGain; }
    float getPredictedEntropy() const noexcept { return predictedEntropy; }

private:
    double cachedSampleRate = 44100.0;
    int controlRateDivisor = 22;  // ~2kHz control rate at 44.1kHz
    int controlCounter = 0;
    float freeEnergy = 0.0f;

    // Active Inference / VFE state
    float predictedEntropy = 0.5f;        // q(s): organism's belief about expected entropy
    float entropyVariance = 0.1f;         // Uncertainty in prediction (Bayesian variance)
    float beliefChangeRate = 0.5f;        // Rate of belief change (for complexity penalty)
    float surprise = 0.0f;               // Squared prediction error
    float variationalFreeEnergy = 0.0f;   // Total VFE (prediction error + complexity)
    float adaptationGain = 1.0f;          // Organism settledness (1.0 = fully stable)
};

//==============================================================================
//  SECTION 4: ANABOLISM — PORT-HAMILTONIAN MODAL ARRAY
//==============================================================================
//
// ModalArray — 32-mode Port-Hamiltonian harmonic oscillator array.
//
// The anabolism stage: the organism reconstructs harmonic content from
// the free energy it has accumulated. Just as a biological cell uses
// metabolic energy (ATP) to build proteins and structures, this module
// uses the free energy pool to drive 32 damped harmonic oscillators
// that collectively produce the engine's output timbre.
//
// Each mode is a damped harmonic oscillator with the ODE:
//   dx_n/dt = v_n
//   dv_n/dt = -omega_n^2 * x_n - gamma * v_n + F_n(t)
//
// where:
//   x_n     = displacement (output signal contribution)
//   v_n     = velocity (rate of change)
//   omega_n = angular frequency of mode n
//   gamma   = damping coefficient (shared across all modes)
//   F_n(t)  = driving force from metabolic energy * spectral weight
//
// Solved with 4th-order Runge-Kutta (RK4) per sample for numerical
// accuracy. All 32 modes are stored in contiguous arrays for cache
// efficiency and future SIMD optimization.
//
// The Port-Hamiltonian formulation (van der Schaft & Jeltsema, 2014)
// guarantees energy conservation: the system can only gain energy from
// the driving force F_n and lose it through damping gamma. This prevents
// the runaway instability that can plague naive modal synthesis.
//==============================================================================
class ModalArray
{
public:
    static constexpr int kNumModes = 32;

    void prepare (double sampleRate) noexcept
    {
        cachedSampleRate = sampleRate;
        inverseSampleRate = 1.0 / sampleRate;
        reset();
    }

    void reset() noexcept
    {
        std::memset (displacement, 0, sizeof (displacement));
        std::memset (velocity, 0, sizeof (velocity));
        std::memset (angularFrequency, 0, sizeof (angularFrequency));
        std::memset (drivingWeight, 0, sizeof (drivingWeight));
    }

    // Set fundamental frequency from MIDI note.
    // isotopeBalance shifts the mode distribution:
    //   0.0 = compress harmonics downward (subharmonic weighting — deep, dark)
    //   0.5 = natural harmonic series (acoustic, familiar)
    //   1.0 = spread harmonics upward (upper partial emphasis — bright, alien)
    void setFundamental (float frequencyHz, float isotopeBalance) noexcept
    {
        constexpr float kTwoPi = 6.28318530717958647692f;
        for (int modeIndex = 0; modeIndex < kNumModes; ++modeIndex)
        {
            float harmonicNumber = static_cast<float> (modeIndex + 1);

            // Isotope balance controls the exponent applied to harmonic number:
            //   spread range [0.5, 2.0] maps isotopeBalance [0, 1]
            //   At spread=1.0 (balance=0.33): standard harmonic series (f, 2f, 3f...)
            //   At spread=0.5 (balance=0.0): compressed (sqrt spacing, subharmonic)
            //   At spread=2.0 (balance=1.0): squared spacing (f, 4f, 9f... — metallic)
            float spread = 0.5f + isotopeBalance * 1.5f;
            float modeFrequency = frequencyHz * std::pow (harmonicNumber, spread);

            // Clamp to 49% of Nyquist to prevent aliasing artifacts.
            // Using 0.49 instead of 0.5 provides a safety margin.
            float nyquistLimit = static_cast<float> (cachedSampleRate) * 0.49f;
            if (modeFrequency > nyquistLimit) modeFrequency = nyquistLimit;

            angularFrequency[modeIndex] = kTwoPi * modeFrequency;
        }
    }

    // Update mode weights from entropy analysis (called at control rate).
    // Each mode's driving force is a Gaussian-weighted function of its
    // position relative to the spectral centroid of the ingested signal.
    // Modes near the centroid receive more energy; distant modes receive less.
    void updateWeights (float spectralCentroid, float freeEnergy,
                        float catalystDrive, float entropyValue,
                        float isotopeBalance) noexcept
    {
        // Gaussian bandwidth: wider at high isotope balance (more modes excited)
        // Range [0.15, 0.50] maps isotopeBalance [0, 1]
        float bandwidth = 0.15f + 0.35f * isotopeBalance;
        float inverseBandwidthSquared = 1.0f / (bandwidth * bandwidth + 0.0001f); // 0.0001f prevents division by zero

        for (int modeIndex = 0; modeIndex < kNumModes; ++modeIndex)
        {
            // Normalize mode position to [0, 1] range
            float normalizedModePosition = static_cast<float> (modeIndex) / static_cast<float> (kNumModes - 1);

            // Gaussian weight: modes near the spectral centroid get more energy
            float distanceFromCentroid = normalizedModePosition - spectralCentroid;
            float gaussianWeight = fastExp (-0.5f * distanceFromCentroid * distanceFromCentroid * inverseBandwidthSquared);

            // Final driving force: catalyst * energy * entropy * spectral weight
            // All four factors must be nonzero for any mode to sound:
            //   catalyst = user control over excitation strength
            //   freeEnergy = metabolic reserves (organism must be "alive")
            //   entropy = information content of ingested signal
            //   gaussianWeight = spectral selectivity
            drivingWeight[modeIndex] = catalystDrive * freeEnergy * entropyValue * gaussianWeight;
        }
    }

    // Process one sample — 4th-order Runge-Kutta integration of all 32 modes.
    // Returns the summed displacement of all modes (the synthesized output).
    float processSample (float dampingCoefficient) noexcept
    {
        float deltaTime = static_cast<float> (inverseSampleRate);
        float halfDeltaTime = deltaTime * 0.5f;
        float output = 0.0f;

        for (int modeIndex = 0; modeIndex < kNumModes; ++modeIndex)
        {
            float omegaSquared = angularFrequency[modeIndex] * angularFrequency[modeIndex];
            float drivingForce = drivingWeight[modeIndex];
            float damping = dampingCoefficient;

            // Current state
            float currentDisplacement = displacement[modeIndex];
            float currentVelocity = velocity[modeIndex];

            // ---- RK4 Integration (4th-order Runge-Kutta) ----
            // Solves the coupled ODE: dx/dt = v, dv/dt = -w^2*x - g*v + F

            // k1: slope at current state
            float dx1 = currentVelocity;
            float dv1 = -omegaSquared * currentDisplacement - damping * currentVelocity + drivingForce;

            // k2: slope at midpoint using k1
            float midDisplacement1 = currentDisplacement + halfDeltaTime * dx1;
            float midVelocity1 = currentVelocity + halfDeltaTime * dv1;
            float dx2 = midVelocity1;
            float dv2 = -omegaSquared * midDisplacement1 - damping * midVelocity1 + drivingForce;

            // k3: slope at midpoint using k2
            float midDisplacement2 = currentDisplacement + halfDeltaTime * dx2;
            float midVelocity2 = currentVelocity + halfDeltaTime * dv2;
            float dx3 = midVelocity2;
            float dv3 = -omegaSquared * midDisplacement2 - damping * midVelocity2 + drivingForce;

            // k4: slope at endpoint using k3
            float endDisplacement = currentDisplacement + deltaTime * dx3;
            float endVelocity = currentVelocity + deltaTime * dv3;
            float dx4 = endVelocity;
            float dv4 = -omegaSquared * endDisplacement - damping * endVelocity + drivingForce;

            // Combine: weighted average of slopes (RK4 formula: 1/6 * (k1 + 2*k2 + 2*k3 + k4))
            float sixthDeltaTime = deltaTime / 6.0f;

            // flushDenormal is critical in this feedback path: the displacement
            // and velocity values decay exponentially when damped. Without
            // denormal flushing, values below ~1e-38 become subnormal floats
            // that can cause 10-100x CPU performance penalties on x86 processors
            // due to microcode-assisted handling of IEEE 754 denormals.
            displacement[modeIndex] = flushDenormal (currentDisplacement + sixthDeltaTime * (dx1 + 2.0f * dx2 + 2.0f * dx3 + dx4));
            velocity[modeIndex] = flushDenormal (currentVelocity + sixthDeltaTime * (dv1 + 2.0f * dv2 + 2.0f * dv3 + dv4));

            output += displacement[modeIndex];
        }

        return output;
    }

private:
    double cachedSampleRate = 44100.0;
    double inverseSampleRate = 1.0 / 44100.0;

    // Contiguous arrays for cache efficiency — all 32 modes' data sits in
    // adjacent memory so the RK4 loop benefits from hardware prefetching.
    // alignas(16) enables future SSE/NEON SIMD optimization.
    alignas(16) float displacement[kNumModes] {};      // x_n: modal displacements (output signal)
    alignas(16) float velocity[kNumModes] {};           // v_n: modal velocities (rate of change)
    alignas(16) float angularFrequency[kNumModes] {};   // omega_n: angular frequencies (rad/s)
    alignas(16) float drivingWeight[kNumModes] {};      // F_n: driving force per mode (control rate)
};

//==============================================================================
//  SECTION 5: VOICE — A SINGLE METABOLIC ORGANISM
//==============================================================================
//
// OrganonVoice — A single metabolic organism.
//
// Each voice is an independent deep-sea chemotroph with its own:
//   - Ingestion stage (ring buffer for coupling input or internal noise)
//   - Catabolism stage (entropy analyzer — breaks down signal)
//   - Economy stage (metabolic free energy pool with Active Inference)
//   - Anabolism stage (32-mode Port-Hamiltonian modal array — builds sound)
//
// Up to 4 organisms can coexist (kMaxVoices), each at a different pitch,
// each metabolizing signal independently. The Phason Shift parameter
// creates temporal offsets between their metabolic cycles, producing
// polyrhythmic pulsing unique to this engine.
//==============================================================================
struct OrganonVoice
{
    bool active = false;
    bool released = false;
    int noteNumber = -1;
    float velocityLevel = 0.0f;
    uint64_t startTime = 0;         // Timestamp for LRU voice stealing
    float stealFadeGain = 1.0f;     // Crossfade gain during voice steal (5ms ramp)
    float stealFadeStep = 0.0f;     // Per-sample decrement during steal crossfade

    // ---- DSP stages (the organism's biology) ----
    OrganonNoiseGen noiseGen;        // Internal noise substrate (self-feeding)
    CytomicSVF ingestionFilter;      // Enzyme selectivity bandpass on noise
    EntropyAnalyzer entropyAnalyzer; // Catabolism: information content measurement
    MetabolicEconomy economy;        // Free energy pool with Active Inference
    ModalArray modalArray;           // Anabolism: 32-mode harmonic reconstruction

    // ---- Coupling ingestion buffer ----
    // Pre-allocated ring buffer for audio from coupling partners.
    // 2048 samples = ~46ms at 44.1kHz — enough latency headroom for
    // block-misaligned coupling reads without allocation.
    static constexpr int kIngestionBufferSize = 2048;
    float ingestionBuffer[kIngestionBufferSize] {};
    int ingestionWritePosition = 0;
    bool hasCouplingInput = false;

    void prepare (double sampleRate, int /*maxBlockSize*/) noexcept
    {
        entropyAnalyzer.prepare (sampleRate);
        economy.prepare (sampleRate);
        modalArray.prepare (sampleRate);
        ingestionFilter.setMode (CytomicSVF::Mode::BandPass);
        resetVoice();
    }

    void resetVoice() noexcept
    {
        active = false;
        released = false;
        noteNumber = -1;
        velocityLevel = 0.0f;
        stealFadeGain = 1.0f;
        stealFadeStep = 0.0f;
        entropyAnalyzer.reset();
        economy.reset();
        modalArray.reset();
        ingestionFilter.reset();
        ingestionWritePosition = 0;
        hasCouplingInput = false;
        std::memset (ingestionBuffer, 0, sizeof (ingestionBuffer));
    }

    // Phason shift: per-voice phase offset for metabolic control-rate stagger.
    // Each organism pulses at a different phase of the metabolic cycle,
    // creating polyrhythmic timbral movement.
    int phasonOffset = 0;

    void noteOn (int note, float vel, uint64_t time, double sampleRate) noexcept
    {
        active = true;
        released = false;
        noteNumber = note;
        velocityLevel = vel;
        startTime = time;
        stealFadeGain = 1.0f;
        stealFadeStep = 0.0f;

        entropyAnalyzer.reset();
        economy.reset();
        modalArray.reset();
        ingestionFilter.reset();

        // Velocity -> initial catalyst boost: higher velocity = faster bloom.
        // 0.15f scaling means max velocity provides 15% initial energy —
        // enough to hear immediate response without bypassing the growth curve.
        economy.setFreeEnergy (vel * 0.15f);

        // Seed noise uniquely per voice/note combination.
        // 7919 is a large prime that decorrelates seeds across adjacent notes.
        noiseGen.seed (static_cast<uint32_t> (note * 7919 + time));
    }

    void noteOff() noexcept
    {
        released = true;
    }

    // Begin 5ms crossfade when this voice is being stolen for a new note.
    // 5ms = ~220 samples at 44.1kHz, fast enough to prevent audible overlap
    // but slow enough to avoid clicks.
    void beginStealFade (double sampleRate) noexcept
    {
        int fadeSamples = static_cast<int> (sampleRate * 0.005); // 5ms
        if (fadeSamples < 1) fadeSamples = 1;
        stealFadeStep = stealFadeGain / static_cast<float> (fadeSamples);
    }

    // Legato: migrate the organism to a new note while preserving its
    // metabolic state (free energy). The organism slides to the new pitch
    // without dying and being reborn — it *migrates*, like a deep-sea
    // creature drifting to a new thermal vent.
    void legatoRetrigger (int note, float vel, uint64_t time) noexcept
    {
        noteNumber = note;
        velocityLevel = vel;
        startTime = time;
        released = false;
        // Keep economy.freeEnergy — the organism migrates with its reserves
        // Reset modes to retune to new fundamental frequency
        modalArray.reset();
    }
};

//==============================================================================
//  SECTION 6: ENGINE — THE COLONY
//==============================================================================
//
// OrganonEngine — Informational Dissipative Synthesis.
//
// "XOrganon is a metabolic synth that consumes audio signals to grow
//  living harmonic structures."
//
// The colony houses up to 4 independent organisms (voices), each
// performing the full metabolic cycle: Ingest -> Analyze -> Metabolize
// -> Reconstruct. The engine as a whole manages voice allocation,
// parameter distribution, coupling I/O, and the Phason Shift system
// that creates temporal offsets between organisms' metabolic cycles.
//
// Sound is produced by:
//   1. Ingesting audio (from coupling partners or internal noise substrate)
//   2. Analyzing its entropy (Shannon information content)
//   3. Converting entropy to free energy (metabolic economy with VFE)
//   4. Using free energy to drive a 32-mode harmonic oscillator array
//
// The result: sound that evolves based on what the engine has been fed.
// No two performances are identical because each organism accumulates
// internal state from its history of coupling inputs. This is a synth
// that remembers what it ate.
//
// Parameters (10 total, all prefixed organon_):
//   metabolicRate   — Speed of energy turnover (Hz, 0.1-10)
//   enzymeSelect    — Bandpass center for ingestion filter (Hz, 20-20k)
//   catalystDrive   — Gain on modal driving force (0-2)
//   dampingCoeff    — Modal oscillator damping (0.01-0.99)
//   signalFlux      — Input gain for ingested signal (0-1)
//   phasonShift     — Temporal offset between voice metabolic cycles (0-1)
//   isotopeBalance  — Spectral weighting: sub <-> upper partials (0-1)
//   lockIn          — Sync metabolic rate to SharedTransport tempo (0-1)
//   membrane        — Reverb send level / membrane porosity (0-1)
//   noiseColor      — Spectral tilt of internal noise: dark <-> bright (0-1)
//==============================================================================
class OrganonEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 4;

    //==========================================================================
    //  IDENTITY
    //==========================================================================

    juce::String getEngineId() const override { return "Organon"; }

    juce::Colour getAccentColour() const override
    {
        // Bioluminescent Cyan — the cold glow of deep-sea chemotrophs
        return juce::Colour (0xFF00CED1);
    }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override
    {
        return activeVoiceCount.load (std::memory_order_relaxed);
    }

    //==========================================================================
    //  TRANSPORT & SHARED STATE
    //==========================================================================

    // Called by the processor to give Organon access to SharedTransport.
    // Must be called before the first renderBlock().
    void setSharedTransport (const SharedTransport* transport) noexcept
    {
        sharedTransport = transport;
    }

    // Reverb send level — read by the processor to route to shared reverb bus.
    // Updated once per block in renderBlock(), driven by the membrane parameter
    // and modulated by average organism surprise (VFE).
    float getReverbSendLevel() const noexcept
    {
        return reverbSendLevel.load (std::memory_order_relaxed);
    }

    //==========================================================================
    //  LIFECYCLE
    //==========================================================================

    // Return the profiler for UI/diagnostic reads.
    const EngineProfiler& getProfiler() const noexcept { return profiler; }

    void prepare (double sampleRate, int maxBlockSize) override
    {
        cachedSampleRate = sampleRate;
        cachedBlockSize = maxBlockSize;
        noteCounter = 0;

        for (auto& voice : voices)
            voice.prepare (sampleRate, maxBlockSize);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        aftertouch.prepare (sampleRate);

        profiler.prepare (sampleRate, maxBlockSize);
        profiler.setCpuBudgetFraction (0.22f); // Organon's 22% CPU budget allocation

        silenceGate.prepare (sampleRate, maxBlockSize);
        silenceGate.setHoldTime (1000.0f);  // Organon has infinite-sustain voices
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

        externalPitchModulation = 0.0f;
        externalFilterModulation = 0.0f;
        externalMorphModulation = 0.0f;
        externalRhythmModulation = 0.0f;
        externalDecayModulation = 0.0f;
        couplingAudioActive = false;
        phasonClock = 0.0f;
        reverbSendLevel.store (0.0f, std::memory_order_relaxed);
    }

    //==========================================================================
    //  AUDIO RENDERING
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        EngineProfiler::ScopedMeasurement measurement (profiler);

        // ---- ParamSnapshot: read all parameters once per block ----
        // This pattern (cache atomic loads at block boundaries) eliminates
        // per-sample atomic reads and ensures parameter coherence within a block.
        // (non-const: aftertouch will boost metabolicRate via lockedMetabolicRate below,
        //  and signalFlux via D006 entropy acceleration below)
        float metabolicRate          = paramMetabolicRate    ? paramMetabolicRate->load()    : 1.0f;
        const float enzymeSelectivity = paramEnzymeSelect   ? paramEnzymeSelect->load()     : 1000.0f;
        const float catalystDrive    = paramCatalystDrive    ? paramCatalystDrive->load()    : 0.5f;
        const float dampingParameter = paramDampingCoeff     ? paramDampingCoeff->load()     : 0.3f;
        float signalFlux             = paramSignalFlux       ? paramSignalFlux->load()       : 0.5f;
        const float phasonShift      = paramPhasonShift      ? paramPhasonShift->load()      : 0.0f;
        const float isotopeBalance   = paramIsotopeBalance   ? paramIsotopeBalance->load()   : 0.5f;
        const float lockIn           = paramLockIn           ? paramLockIn->load()           : 0.0f;
        const float membrane         = paramMembrane         ? paramMembrane->load()         : 0.2f;
        const float noiseColor       = paramNoiseColor       ? paramNoiseColor->load()       : 0.5f;

        // ---- MIDI handling ----
        for (const auto metadata : midi)
        {
            auto message = metadata.getMessage();
            if (message.isNoteOn())
            {
                silenceGate.wake();
                handleNoteOn (message.getNoteNumber(), message.getFloatVelocity());
            }
            else if (message.isNoteOff())
                handleNoteOff (message.getNoteNumber());
            else if (message.isAllNotesOff() || message.isAllSoundOff())
            {
                for (auto& voice : voices)
                    voice.resetVoice();
            }
            // D006: channel pressure → aftertouch (applied to metabolic rate below)
            else if (message.isChannelPressure())
                aftertouch.setChannelPressure (message.getChannelPressureValue() / 127.0f);
            // D006: mod wheel (CC#1) → entropy rate acceleration
            // Wheel up = faster metabolic feeding cycle — the chemotroph ingests
            // signal more rapidly, belief updates accelerate, and the harmonic
            // spectrum evolves at a higher rate. Full wheel adds +3.0 Hz.
            else if (message.isController() && message.getControllerNumber() == 1)
                modWheelAmount = message.getControllerValue() / 127.0f;
        }

        if (silenceGate.isBypassed() && midi.isEmpty()) { buffer.clear(); return; }

        // D006: smooth aftertouch pressure and compute modulation value
        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);

        // ---- LOCK-IN: Sync metabolic rate to SharedTransport tempo ----
        // lockIn 0.0 = free-running metabolic rate (the organism breathes at its own pace).
        // lockIn 1.0 = fully quantized to nearest beat subdivision (the organism
        //              pulses in sync with the DAW tempo — useful for rhythmic textures).
        // We find the tempo subdivision closest to the current metabolic rate and
        // crossfade between the free rate and the quantized rate.
        // D006: aftertouch accelerates metabolic rate — sensitivity 0.25 × range 9.9
        // Full pressure adds up to +2.5 Hz metabolic rate (from 1.0 Hz default toward faster feeding)
        metabolicRate = std::clamp (metabolicRate + atPressure * 0.25f * 9.9f, 0.1f, 10.0f);
        // D006: mod wheel accelerates entropy rate — sensitivity 3.0 Hz
        // Full wheel adds +3.0 Hz to metabolicRate: organism feeds on signal faster,
        // belief updates quicken, and harmonic spectrum evolves more rapidly.
        metabolicRate = std::clamp (metabolicRate + modWheelAmount * 3.0f, 0.1f, 10.0f);

        // D006: aftertouch increases entropic free energy — sensitivity 0.2
        // Channel pressure increases the signal flux feeding the EntropyAnalyzer: the organism
        // ingests more signal per cycle, its observed entropy rises above what it predicted,
        // prediction error grows, and the VFE metabolism accelerates its belief updates.
        // Semantically: the organism feels more uncertain, its predictions fail more often,
        // and it generates more surprise. The metabolism accelerates.
        // At default signalFlux 0.5, full pressure reaches 0.7 — well within the [0,1] range.
        signalFlux = std::clamp (signalFlux + atPressure * 0.2f, 0.0f, 1.0f);

        // ---- Macros (M1-M4) ----
        const float macroMetabolism = paramMacroMetabolism ? paramMacroMetabolism->load() : 0.0f;
        const float macroSpectrum   = paramMacroSpectrum   ? paramMacroSpectrum->load()   : 0.0f;
        const float macroCoupling   = paramMacroCoupling   ? paramMacroCoupling->load()   : 0.0f;
        const float macroSpace      = paramMacroSpace      ? paramMacroSpace->load()      : 0.0f;

        // M1 METABOLISM: boosts metabolic rate (+3 Hz) and catalyst drive (+0.5)
        metabolicRate = std::clamp (metabolicRate + macroMetabolism * 3.0f, 0.1f, 10.0f);
        float effectiveCatalyst = std::clamp (catalystDrive + macroMetabolism * 0.5f, 0.0f, 2.0f);

        // M2 SPECTRUM: shifts isotope balance toward bright (+0.4) and noise color (+0.3)
        float effectiveIsotope = std::clamp (isotopeBalance + macroSpectrum * 0.4f, 0.0f, 1.0f);
        float effectiveNoiseColor = std::clamp (noiseColor + macroSpectrum * 0.3f, 0.0f, 1.0f);

        // M3 COUPLING: boosts signal flux (+0.3) and membrane porosity (+0.3)
        float effectiveSignalFlux = std::clamp (signalFlux + macroCoupling * 0.3f, 0.0f, 1.0f);
        float effectiveMembrane = std::clamp (membrane + macroCoupling * 0.3f, 0.0f, 1.0f);

        // M4 SPACE: boosts membrane (+0.4) and damping (+0.3, more percussive reverberant tails)
        effectiveMembrane = std::clamp (effectiveMembrane + macroSpace * 0.4f, 0.0f, 1.0f);
        float effectiveDamping = std::clamp (dampingParameter + macroSpace * 0.3f, 0.01f, 0.99f);

        // ---- D005: Breathing LFO ----
        // Slow autonomous modulation of metabolic rate (+/- 0.5 Hz) and isotope balance
        // (+/- 0.1). Rate is 0.02 Hz (50-second cycle) — slow enough to feel organic,
        // fast enough to be perceivable within a performance.
        breathingLfoIncrement = 0.02f / static_cast<float> (cachedSampleRate);
        breathingLfoPhase += breathingLfoIncrement * static_cast<float> (numSamples);
        if (breathingLfoPhase >= 1.0f) breathingLfoPhase -= 1.0f;
        float breathLfo = std::sin (breathingLfoPhase * 6.28318530717958647692f);
        metabolicRate = std::clamp (metabolicRate + breathLfo * 0.5f, 0.1f, 10.0f);
        effectiveIsotope = std::clamp (effectiveIsotope + breathLfo * 0.1f, 0.0f, 1.0f);

        float lockedMetabolicRate = metabolicRate;
        float lockInTransportPhase = 0.0f;
        if (lockIn > 0.001f && sharedTransport != nullptr)
        {
            double beatsPerMinute = sharedTransport->getBPM();
            if (beatsPerMinute > 0.0)
            {
                // Convert metabolic rate (Hz) to the closest beat subdivision.
                // beatFrequency = BPM/60 gives quarter-note frequency in Hz.
                float beatFrequency = static_cast<float> (beatsPerMinute / 60.0);

                // Available subdivision rates (in Hz), from fastest to slowest
                float subdivisionRates[5] = {
                    beatFrequency * 4.0f,    // 16th notes
                    beatFrequency * 2.0f,    // 8th notes
                    beatFrequency,           // Quarter notes
                    beatFrequency * 0.5f,    // Half notes
                    beatFrequency * 0.25f    // Whole notes
                };

                // Subdivision durations in beats (for phase calculation)
                float subdivisionBeats[5] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };

                // Find closest subdivision to current metabolic rate
                float closestRate = subdivisionRates[0];
                float closestDivisionBeats = subdivisionBeats[0];
                float bestDistance = std::fabs (metabolicRate - subdivisionRates[0]);
                for (int i = 1; i < 5; ++i)
                {
                    float distance = std::fabs (metabolicRate - subdivisionRates[i]);
                    if (distance < bestDistance)
                    {
                        bestDistance = distance;
                        closestRate = subdivisionRates[i];
                        closestDivisionBeats = subdivisionBeats[i];
                    }
                }

                // Crossfade: free rate -> quantized rate based on lockIn amount
                lockedMetabolicRate = metabolicRate + lockIn * (closestRate - metabolicRate);

                // Get transport phase for the locked division (used by phason clock)
                lockInTransportPhase = static_cast<float> (
                    sharedTransport->getPhaseForDivision (closestDivisionBeats));
            }
        }

        // ---- Damping mapping ----
        // Map parameter range (0.01-0.99) to gamma value for the modal ODE.
        // 20.0f scaling factor produces gamma range [0.2, 19.8] which covers:
        //   Low damping (~0.2): very long resonant tails, shimmering
        //   Mid damping (~6.0): natural acoustic decay
        //   High damping (~19.8): heavily damped, percussive, muted
        float gamma = effectiveDamping * 20.0f;

        // ---- Phason Shift: per-voice metabolic rate modulation ----
        // Each voice gets an evenly-spaced phase offset around a cycle;
        // phasonShift controls the modulation depth.
        //   At 0.0: all voices share the same metabolic rate (no pulse).
        //   At 1.0: each voice pulses fully out of phase with the others,
        //           creating a polyrhythmic metabolic pattern.
        // When lock-in is active, the phason clock syncs to transport phase
        // for tempo-locked polyrhythmic pulsing.
        float phasonModulations[kMaxVoices];
        if (phasonShift > 0.001f)
        {
            if (lockIn > 0.5f && lockInTransportPhase >= 0.0f)
            {
                // Lock phason clock to transport phase for tempo-synced pulsing
                float targetPhase = lockInTransportPhase;
                // Smooth transition to avoid phase-jump clicks.
                // 0.3f rate: fast enough to track tempo changes, slow enough
                // to prevent discontinuities.
                float phaseDifference = targetPhase - phasonClock;
                if (phaseDifference > 0.5f) phaseDifference -= 1.0f;
                if (phaseDifference < -0.5f) phaseDifference += 1.0f;
                phasonClock += phaseDifference * lockIn * 0.3f;
                if (phasonClock < 0.0f) phasonClock += 1.0f;
                if (phasonClock > 1.0f) phasonClock -= 1.0f;
            }
            else
            {
                // Free-running: advance phason clock at locked metabolic rate
                float phasonCycleSamples = static_cast<float> (cachedSampleRate) / std::max (lockedMetabolicRate, 0.1f);
                float phasonIncrement = static_cast<float> (numSamples) / phasonCycleSamples;
                phasonClock += phasonIncrement;
                if (phasonClock > 1.0f) phasonClock -= std::floor (phasonClock);
            }

            constexpr float kTwoPi = 6.28318530717958647692f;
            for (int voiceIndex = 0; voiceIndex < kMaxVoices; ++voiceIndex)
            {
                // Evenly space voices across the metabolic cycle
                float voicePhase = phasonClock + static_cast<float> (voiceIndex) / static_cast<float> (kMaxVoices);
                if (voicePhase > 1.0f) voicePhase -= 1.0f;
                // Sinusoidal modulation: +/- phasonShift of metabolic rate
                phasonModulations[voiceIndex] = std::sin (kTwoPi * voicePhase) * phasonShift;
            }
        }
        else
        {
            for (int voiceIndex = 0; voiceIndex < kMaxVoices; ++voiceIndex)
                phasonModulations[voiceIndex] = 0.0f;
        }

        // ---- Per-sample rendering loop ----
        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            float mixL = 0.0f;
            float mixR = 0.0f;

            int voiceIndex = 0;
            for (auto& voice : voices)
            {
                if (!voice.active)
                {
                    ++voiceIndex;
                    continue;
                }

                // Set entropy window size based on enzyme selectivity
                voice.entropyAnalyzer.setWindowSize (enzymeSelectivity);

                // ---- INGESTION: the organism feeds ----
                float ingestedSample = 0.0f;
                if (couplingAudioActive && voice.hasCouplingInput)
                {
                    // Read from coupling ingestion buffer (fed by partner engines)
                    int readPosition = (voice.ingestionWritePosition - 1 + OrganonVoice::kIngestionBufferSize)
                                       & (OrganonVoice::kIngestionBufferSize - 1);
                    ingestedSample = voice.ingestionBuffer[readPosition] * effectiveSignalFlux;
                }
                else
                {
                    // Self-feeding: internal noise substrate filtered through
                    // the enzyme selectivity bandpass. The organism generates
                    // and consumes its own noise when no coupling partner feeds it.
                    float noise = voice.noiseGen.process();

                    // Noise color tilts the spectrum via the enzyme filter Q.
                    // noiseColor 0.0 = dark (low Q, gentle LP character)
                    // noiseColor 0.5 = white (moderate Q)
                    // noiseColor 1.0 = bright (high Q, resonant HP character)
                    // Range [0.3, 0.7] maps noiseColor [0, 1]
                    voice.ingestionFilter.setCoefficients (
                        enzymeSelectivity + externalFilterModulation * 2000.0f,
                        0.3f + effectiveNoiseColor * 0.4f,
                        static_cast<float> (cachedSampleRate));
                    ingestedSample = voice.ingestionFilter.processSample (noise) * effectiveSignalFlux;
                }

                // ---- CATABOLISM: break down the signal ----
                voice.entropyAnalyzer.pushSample (ingestedSample);

                // ---- ECONOMY: metabolize and update beliefs ----
                // Locked + phason-shifted metabolic rate: each organism pulses
                // at a different phase of the metabolic cycle.
                float voiceMetabolicRate = lockedMetabolicRate * (1.0f + phasonModulations[voiceIndex]);
                if (voiceMetabolicRate < 0.05f) voiceMetabolicRate = 0.05f; // Floor prevents stalling

                voice.economy.update (voiceMetabolicRate, effectiveSignalFlux,
                                      voice.entropyAnalyzer.getEntropy(),
                                      externalRhythmModulation, externalDecayModulation,
                                      voice.released);

                // Check if organism has fully decayed (free energy exhausted)
                if (voice.released && voice.economy.getFreeEnergy() < 0.001f)
                {
                    voice.active = false;
                    continue;
                }

                // ---- ANABOLISM: reconstruct harmonic content ----
                float fundamental = midiToFreq (voice.noteNumber) +
                                    externalPitchModulation * 20.0f; // +/-10 semitones at mod +/-0.5
                if (fundamental < 20.0f) fundamental = 20.0f; // Below 20Hz is inaudible

                // VFE modulates isotope balance: surprise shifts spectral character.
                // 0.15f scaling: full surprise shifts balance by 15% — enough to
                // hear timbral change without destabilizing the harmonic structure.
                float surpriseShift = voice.economy.getSurprise() * 0.15f;
                float voiceIsotope = clamp (effectiveIsotope + externalMorphModulation * 0.3f + surpriseShift, 0.0f, 1.0f);

                voice.modalArray.setFundamental (fundamental, voiceIsotope);

                // VFE adaptation gain modulates catalyst effectiveness:
                // Settled organisms (low VFE) get full catalyst drive -> rich harmonics.
                // Surprised organisms redirect energy to adaptation -> thinner sound.
                float voiceCatalyst = effectiveCatalyst * voice.economy.getAdaptationGain();

                voice.modalArray.updateWeights (voice.entropyAnalyzer.getSpectralCentroid(),
                                                voice.economy.getFreeEnergy(),
                                                voiceCatalyst,
                                                voice.entropyAnalyzer.getEntropy(),
                                                voiceIsotope);

                float sample = voice.modalArray.processSample (gamma);

                // Soft clip via tanh to prevent output spikes from resonant modes
                sample = fastTanh (sample);

                // Voice steal crossfade (5ms ramp-down)
                if (voice.stealFadeStep > 0.0f)
                {
                    voice.stealFadeGain -= voice.stealFadeStep;
                    // flushDenormal prevents the fade gain from lingering as a
                    // subnormal float near zero, which would waste CPU cycles
                    // on subsequent multiplications.
                    voice.stealFadeGain = flushDenormal (voice.stealFadeGain);
                    if (voice.stealFadeGain <= 0.0f)
                    {
                        voice.active = false;
                        continue;
                    }
                    sample *= voice.stealFadeGain;
                }

                // Velocity scaling
                sample *= voice.velocityLevel;

                // VFE-driven stereo spread: surprised organisms widen the stereo
                // image slightly, as if their metabolic turbulence scatters energy
                // across the spatial field. 0.15f maximum spread.
                float stereoSpread = voice.economy.getSurprise() * 0.15f;
                mixL += sample * (1.0f - stereoSpread);
                mixR += sample * (1.0f + stereoSpread);

                ++voiceIndex;
            }

            // Cache output for coupling reads by partner engines
            outputCacheL[static_cast<size_t> (sampleIndex)] = mixL;
            outputCacheR[static_cast<size_t> (sampleIndex)] = mixR;

            // Write to output buffer (additive — multiple engines may write)
            if (buffer.getNumChannels() > 0)
                buffer.getWritePointer (0)[sampleIndex] += mixL;
            if (buffer.getNumChannels() > 1)
                buffer.getWritePointer (1)[sampleIndex] += mixR;
        }

        // ---- Reset coupling accumulators after consumption ----
        // Each block's coupling inputs are consumed and cleared so stale
        // values don't persist into the next block.
        externalPitchModulation = 0.0f;
        externalFilterModulation = 0.0f;
        externalMorphModulation = 0.0f;
        externalRhythmModulation = 0.0f;
        externalDecayModulation = 0.0f;
        couplingAudioActive = false;

        // ---- Update active voice count and membrane reverb send ----
        int count = 0;
        float averageSurprise = 0.0f;
        for (const auto& voice : voices)
        {
            if (voice.active)
            {
                ++count;
                averageSurprise += voice.economy.getSurprise();
            }
        }
        activeVoiceCount.store (count, std::memory_order_relaxed);

        // Membrane porosity -> reverb send level.
        // Base send = membrane parameter value.
        // Modulated by average surprise across voices: surprised organisms
        // "sweat" metabolic energy into the reverb (up to +30% extra send).
        // 0.3f scaling: prevents reverb from overwhelming the dry signal.
        if (count > 0)
            averageSurprise /= static_cast<float> (count);
        float sendLevel = effectiveMembrane + averageSurprise * 0.3f;
        if (sendLevel > 1.0f) sendLevel = 1.0f;
        reverbSendLevel.store (sendLevel, std::memory_order_relaxed);

        silenceGate.analyzeBlock (buffer.getReadPointer (0), buffer.getReadPointer (1), numSamples);
    }

    //==========================================================================
    //  COUPLING — SYMBIOTIC AUDIO I/O
    //==========================================================================
    //
    // Organon is the ultimate coupling *receiver* in the XOmnibus ecosystem.
    // While most engines couple by modulating each other's parameters,
    // Organon literally ingests the audio output of partner engines and
    // metabolizes it into new harmonic content. This is parasitic synthesis:
    // the chemotroph feeds on whatever drifts down from above in the
    // water column.
    //==========================================================================

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
                // Write source audio into all active voices' ingestion buffers.
                // This is the primary feeding mechanism: partner engine audio
                // becomes the organism's food supply.
                if (sourceBuffer != nullptr && numSamples > 0)
                {
                    for (auto& voice : voices)
                    {
                        if (!voice.active) continue;
                        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
                        {
                            voice.ingestionBuffer[voice.ingestionWritePosition] = sourceBuffer[sampleIndex] * amount;
                            voice.ingestionWritePosition = (voice.ingestionWritePosition + 1)
                                                           & (OrganonVoice::kIngestionBufferSize - 1);
                        }
                        voice.hasCouplingInput = true;
                    }
                    couplingAudioActive = true;
                }
                break;
            }

            case CouplingType::RhythmToBlend:
                externalRhythmModulation += amount;
                break;

            case CouplingType::EnvToDecay:
                externalDecayModulation += amount;
                break;

            case CouplingType::AmpToFilter:
                externalFilterModulation += amount;
                break;

            case CouplingType::EnvToMorph:
                externalMorphModulation += amount;
                break;

            case CouplingType::LFOToPitch:
            case CouplingType::PitchToPitch:
                // 0.5f scaling: external pitch modulation at half strength
                // to prevent excessive detuning from aggressive coupling.
                externalPitchModulation += amount * 0.5f;
                break;

            default:
                break; // AmpToPitch, AudioToRing, FilterToFilter, AmpToChoke — not applicable to metabolic synthesis
        }
    }

    //==========================================================================
    //  PARAMETERS
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

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramMetabolicRate    = apvts.getRawParameterValue ("organon_metabolicRate");
        paramEnzymeSelect     = apvts.getRawParameterValue ("organon_enzymeSelect");
        paramCatalystDrive    = apvts.getRawParameterValue ("organon_catalystDrive");
        paramDampingCoeff     = apvts.getRawParameterValue ("organon_dampingCoeff");
        paramSignalFlux       = apvts.getRawParameterValue ("organon_signalFlux");
        paramPhasonShift      = apvts.getRawParameterValue ("organon_phasonShift");
        paramIsotopeBalance   = apvts.getRawParameterValue ("organon_isotopeBalance");
        paramLockIn           = apvts.getRawParameterValue ("organon_lockIn");
        paramMembrane         = apvts.getRawParameterValue ("organon_membrane");
        paramNoiseColor       = apvts.getRawParameterValue ("organon_noiseColor");
        paramMacroMetabolism  = apvts.getRawParameterValue ("organon_macroMetabolism");
        paramMacroSpectrum    = apvts.getRawParameterValue ("organon_macroSpectrum");
        paramMacroCoupling    = apvts.getRawParameterValue ("organon_macroCoupling");
        paramMacroSpace       = apvts.getRawParameterValue ("organon_macroSpace");
    }

private:

    SilenceGate silenceGate;

    //==========================================================================
    //  PARAMETER DEFINITIONS
    //==========================================================================

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // 1. Metabolic Rate — speed of energy turnover (Hz).
        //    0.1Hz = glacial deep-sea metabolism, 10Hz = frantic surface feeding.
        //    Skew 0.5: logarithmic feel for frequency-domain parameter.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_metabolicRate", 1), "Metabolic Rate",
            juce::NormalisableRange<float> (0.1f, 10.0f, 0.0f, 0.5f), 1.0f));

        // 2. Enzyme Selectivity — bandpass center frequency on ingestion (Hz).
        //    Controls which frequency band of the input signal the organism
        //    can "digest." Narrow = specialist feeder, wide = generalist.
        //    Skew 0.3: logarithmic feel for audio frequency range.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_enzymeSelect", 1), "Enzyme Selectivity",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.3f), 1000.0f));

        // 3. Catalyst Drive — gain on modal driving force (0-2).
        //    How aggressively the metabolic energy excites the harmonic modes.
        //    At 0: silent (no catalyst). At 2: intense, potentially self-exciting.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_catalystDrive", 1), "Catalyst Drive",
            juce::NormalisableRange<float> (0.0f, 2.0f), 0.5f));

        // 4. Damping Coefficient — damping in the modal ODE (0.01-0.99).
        //    Low = long resonant tails (shimmering, crystalline).
        //    High = short, muted, percussive response.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_dampingCoeff", 1), "Damping",
            juce::NormalisableRange<float> (0.01f, 0.99f), 0.3f));

        // 5. Signal Flux — gain of ingestion input (0-1).
        //    How much of the available signal (coupling or noise) enters
        //    the organism. At 0: starved. At 1: fully fed.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_signalFlux", 1), "Signal Flux",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // 6. Phason Shift — temporal offset between metabolic cycles (0-1).
        //    At 0: all voices pulse in unison.
        //    At 1: voices pulse fully out of phase (polyrhythmic metabolism).
        //    Named after quasicrystal phason modes — structural shifts that
        //    don't change energy but change configuration.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_phasonShift", 1), "Phason Shift",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // 7. Isotope Balance — spectral weighting (0-1).
        //    0.0 = subharmonic emphasis (deep, dark, compressed modes).
        //    0.5 = natural harmonic series (acoustic, familiar).
        //    1.0 = upper partial emphasis (bright, metallic, alien).
        //    Named after isotope ratios that shift atomic mass distribution.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_isotopeBalance", 1), "Isotope Balance",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // 8. Lock-in Strength — sync metabolic rate to transport tempo (0-1).
        //    At 0: free-running metabolism (organic, drifting).
        //    At 1: quantized to nearest beat subdivision (rhythmic, pulsing).
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_lockIn", 1), "Lock-in Strength",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // 9. Membrane Porosity — diffusion / reverb send level (0-1).
        //    How porous the organism's cell membrane is: high porosity lets
        //    more metabolic energy leak into the shared reverb environment.
        //    Also modulated by VFE surprise (stressed organisms sweat more).
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_membrane", 1), "Membrane Porosity",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));

        // 10. Noise Color — spectral tilt of internal noise substrate (0-1).
        //     0.0 = dark (rumbling, low-frequency substrate).
        //     0.5 = white (flat spectrum).
        //     1.0 = bright (hissing, high-frequency substrate).
        //     Only affects the self-feeding noise; coupling input bypasses this.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_noiseColor", 1), "Noise Color",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // ---- Macros (M1-M4) ----
        // D002: 4 standard macros for one-knob performance control.
        // M1 METABOLISM: boosts metabolicRate + catalystDrive (faster, hotter feeding)
        // M2 SPECTRUM: shifts isotopeBalance + noiseColor (dark <-> bright)
        // M3 COUPLING: increases signalFlux + membrane porosity (more connected)
        // M4 SPACE: increases membrane + damping (more diffuse, reverberant)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_macroMetabolism", 1), "Organon Macro METABOLISM",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_macroSpectrum", 1), "Organon Macro SPECTRUM",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_macroCoupling", 1), "Organon Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("organon_macroSpace", 1), "Organon Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    }

    //==========================================================================
    //  VOICE MANAGEMENT
    //==========================================================================

    void handleNoteOn (int note, float noteVelocity) noexcept
    {
        ++noteCounter;

        // Find a free voice slot
        int freeSlot = -1;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[i].active)
            {
                freeSlot = i;
                break;
            }
        }

        // If no free voice, steal the oldest (LRU — Least Recently Used)
        if (freeSlot < 0)
        {
            uint64_t oldestTime = UINT64_MAX;
            freeSlot = 0;
            for (int i = 0; i < kMaxVoices; ++i)
            {
                if (voices[i].startTime < oldestTime)
                {
                    oldestTime = voices[i].startTime;
                    freeSlot = i;
                }
            }
            // Begin 5ms crossfade on stolen voice to prevent click
            voices[freeSlot].beginStealFade (cachedSampleRate);
        }

        voices[freeSlot].noteOn (note, noteVelocity, noteCounter, cachedSampleRate);
    }

    void handleNoteOff (int note) noexcept
    {
        for (auto& voice : voices)
        {
            if (voice.active && !voice.released && voice.noteNumber == note)
            {
                voice.noteOff();
                break; // Release only one voice per note-off (oldest match)
            }
        }
    }

    //==========================================================================
    //  ENGINE STATE
    //==========================================================================

    double cachedSampleRate = 44100.0;
    int cachedBlockSize = 512;
    uint64_t noteCounter = 0;           // Monotonic counter for LRU voice stealing

    std::array<OrganonVoice, kMaxVoices> voices;
    std::atomic<int> activeVoiceCount { 0 };

    // Output cache for coupling reads (partner engines read our output)
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Phason Shift clock (0.0-1.0, advances at metabolic rate)
    float phasonClock = 0.0f;

    // Lock-in: pointer to shared transport (set externally by processor)
    const SharedTransport* sharedTransport = nullptr;

    // Membrane: reverb send level computed per block (read by processor)
    std::atomic<float> reverbSendLevel { 0.0f };

    // Coupling accumulators (consumed and reset after each render block)
    float externalPitchModulation = 0.0f;
    float externalFilterModulation = 0.0f;
    float externalMorphModulation = 0.0f;
    float externalRhythmModulation = 0.0f;
    float externalDecayModulation = 0.0f;
    bool couplingAudioActive = false;

    // ---- Macro parameter pointers (M1-M4) ----
    std::atomic<float>* paramMacroMetabolism = nullptr;
    std::atomic<float>* paramMacroSpectrum   = nullptr;
    std::atomic<float>* paramMacroCoupling   = nullptr;
    std::atomic<float>* paramMacroSpace      = nullptr;

    // ---- D005: Breathing LFO ----
    // A slow autonomous LFO that modulates metabolic rate and isotope balance,
    // creating organic rhythmic variation in the organism's metabolic cycle.
    // Rate floor 0.005 Hz (200-second breathing cycle) satisfies D005.
    float breathingLfoPhase = 0.0f;
    float breathingLfoIncrement = 0.0f;

    // Performance profiler
    EngineProfiler profiler;

    // ---- D006 Aftertouch — dual application: metabolic rate + entropy (signal flux) ----
    // 1. metabolicRate: sensitivity 0.25 × 9.9 Hz — organism feeds faster under pressure
    // 2. signalFlux: sensitivity 0.2 — more signal enters catabolism, entropy rises,
    //    prediction error increases, VFE surprise accelerates belief updates
    PolyAftertouch aftertouch;

    // D006: mod wheel (CC#1) — accelerates entropy rate (metabolicRate +3.0 Hz at full)
    float modWheelAmount = 0.0f;

    // Cached parameter pointers (set by attachParameters, read per block)
    std::atomic<float>* paramMetabolicRate    = nullptr;
    std::atomic<float>* paramEnzymeSelect     = nullptr;
    std::atomic<float>* paramCatalystDrive    = nullptr;
    std::atomic<float>* paramDampingCoeff     = nullptr;
    std::atomic<float>* paramSignalFlux       = nullptr;
    std::atomic<float>* paramPhasonShift      = nullptr;
    std::atomic<float>* paramIsotopeBalance   = nullptr;
    std::atomic<float>* paramLockIn           = nullptr;
    std::atomic<float>* paramMembrane         = nullptr;
    std::atomic<float>* paramNoiseColor       = nullptr;
};

} // namespace xomnibus
