// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "OperaConstants.h"

//==============================================================================
// KuramotoField.h — opera::KuramotoField
//
// Mean-field Kuramoto synchronicity engine for XOpera. This is the core DSP
// module that drives the operatic climax: N detuned partials coupled through
// a global order parameter, with phase-transition hysteresis (Schulze),
// acausal resonance cluster detection, emotional memory (Vangelis), and
// logarithmic response speed control.
//
// Mathematical foundation: Kuramoto (1975), Strogatz (2000).
// Mean-field decomposition: Acebron et al. (2005) — exact O(2N) reduction.
//
// The field updates every kKuraBlock = 8 samples (6 kHz at 48 kHz).
// Between updates, partials free-run at their natural frequencies.
//
// All code is inline in this header, allocation-free, and real-time safe.
//==============================================================================

#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace opera
{

//==============================================================================
// Cluster structure for acausal resonance detection
//==============================================================================

struct Cluster
{
    int partials[kClusterMaxSize] = {};
    int size = 0;
    float coherence = 0.0f;
};

//==============================================================================
// Emotional memory snapshot — stores Kuramoto state across note boundaries
//==============================================================================

struct EmotionalMemory
{
    float storedPhases[kMaxPartials] = {};
    bool storedLocked[kMaxPartials] = {};
    float storedR = 0.0f;
    uint64_t releaseTimeSamples = 0;
    int numPartials = 0;
    bool valid = false;
};

//==============================================================================
// One-pole smoother for K response speed (ParameterSmoother pattern)
//==============================================================================

struct KSmoother
{
    void prepare(float sampleRate, float timeSec) noexcept
    {
        if (sampleRate <= 0.0f || timeSec <= 0.0f)
        {
            coeff = 1.0f;
            return;
        }
        coeff = 1.0f - std::exp(-kTwoPi * (1.0f / timeSec) / sampleRate);
    }

    void snapTo(float value) noexcept
    {
        currentValue = value;
        targetValue = value;
    }

    void set(float target) noexcept { targetValue = target; }

    float process() noexcept
    {
        currentValue += (targetValue - currentValue) * coeff;
        currentValue = flushDenormal(currentValue);
        return currentValue;
    }

    float get() const noexcept { return currentValue; }

    float currentValue = 0.0f;
    float targetValue = 0.0f;
    float coeff = 1.0f;
};

//==============================================================================
// KuramotoField — the core synchronicity engine
//==============================================================================

class KuramotoField
{
public:
    //==========================================================================
    // Lifecycle
    //==========================================================================

    /// Call once from prepareToPlay with the host sample rate and the maximum
    /// number of partials this voice will ever use.
    void prepare(double sampleRate, int maxPartials) noexcept
    {
        sampleRate_ = static_cast<float>(sampleRate);
        maxPartials_ = std::min(maxPartials, kMaxPartials);
        dt_ = static_cast<float>(kKuraBlock) / sampleRate_;

        // Default smoother at 100ms (responseSpeed=0.5 maps here)
        kSmoother_.prepare(sampleRate_, 0.1f);
        kSmoother_.snapTo(0.0f);

        reset();
    }

    /// Reset all Kuramoto state to initial conditions. Called on voice steal
    /// or hard reset. Does NOT clear emotional memory (that persists across
    /// note boundaries).
    void reset() noexcept
    {
        orderParameter_ = 0.0f;
        meanPhase_ = 0.0f;
        blockCounter_ = 0;
        numClusters_ = 0;
        sampleCounter_ = 0;

        for (int i = 0; i < kMaxPartials; ++i)
        {
            partialLocked_[i] = false;
            clusterBoost_[i] = 1.0f;
        }
    }

    //==========================================================================
    // Core update — called every kKuraBlock samples
    //==========================================================================

    /// Main Kuramoto field update. Called by the voice every kKuraBlock samples.
    ///
    /// theta        — mutable phase array (radians, [0, 2pi)), updated in-place
    /// omega        — natural angular frequencies (rad/s), read-only
    /// numPartials  — active partial count (4..48)
    /// drama        — normalised drama parameter [0, 1], mapped to K = drama * Kmax
    /// resSens      — acausal resonance sensitivity [0, 1]
    /// responseSpeed — response speed parameter [0, 1] (logarithmic mapping)
    void updateField(float* theta, const float* omega, int numPartials, float drama, float resSens,
                     float responseSpeed) noexcept
    {
        numPartials = std::min(numPartials, maxPartials_);
        if (numPartials < 2)
            return;

        //----------------------------------------------------------------------
        // Response speed: logarithmic mapping from 5ms (1.0) to 60s (0.0)
        //   t = 60.0 * 10^(-speed * 4.08)
        //   speed=0.0 -> 60s, speed=0.5 -> ~0.55s, speed=1.0 -> ~5ms
        //----------------------------------------------------------------------
        float smootherTime = 60.0f * std::pow(10.0f, -responseSpeed * 4.08f);

        // FIX P0: only call prepare() when smootherTime changes; recalculating
        // the coefficient every 8 samples was resetting the ramp mid-glide.
        if (std::abs(smootherTime - cachedSmootherTime_) > 0.0001f)
        {
            kSmoother_.prepare(sampleRate_, smootherTime);
            cachedSmootherTime_ = smootherTime;
        }

        //----------------------------------------------------------------------
        // Target K from drama parameter
        //----------------------------------------------------------------------
        float targetK = drama * kKmax;
        kSmoother_.set(targetK);

        // Advance the smoother by kKuraBlock samples to get effective K
        float Keff = 0.0f;
        for (int s = 0; s < kKuraBlock; ++s)
            Keff = kSmoother_.process();

        //----------------------------------------------------------------------
        // Step 1: Compute order parameter r(t) and mean phase psi(t)
        //         O(N) — single pass over all active partials
        //----------------------------------------------------------------------
        float sumCos = 0.0f;
        float sumSin = 0.0f;

        for (int i = 0; i < numPartials; ++i)
        {
            sumCos += fastCos(theta[i]);
            sumSin += fastSin(theta[i]);
        }

        float invN = 1.0f / static_cast<float>(numPartials);
        float rCos = sumCos * invN;
        float rSin = sumSin * invN;
        float r = std::sqrt(rCos * rCos + rSin * rSin);
        float psi = std::atan2(rSin, rCos);

        // Store for external queries (spatial panning, reactive stage)
        orderParameter_ = r;
        meanPhase_ = psi;

        //----------------------------------------------------------------------
        // Step 2: Critical K threshold (Lorentzian distribution)
        //         Kc = 2 * gamma, where gamma = detuneAmount * f0 * 0.03
        //
        //         We estimate gamma from the actual frequency spread:
        //         gamma = stddev(omega) / (2*pi) * something...
        //
        //         Per the spec, we compute Kc from the omega distribution
        //         at runtime. Use the spread between min and max omega as
        //         a proxy for 2*gamma in the Lorentzian sense.
        //----------------------------------------------------------------------
        float omegaMin = omega[0];
        float omegaMax = omega[0];
        for (int i = 1; i < numPartials; ++i)
        {
            if (omega[i] < omegaMin)
                omegaMin = omega[i];
            if (omega[i] > omegaMax)
                omegaMax = omega[i];
        }

        // gamma = half-width of the frequency distribution
        // For a Lorentzian, the interquartile range ~ 2*gamma
        // We use the full range / 4 as a robust estimator for gamma
        float gamma = (omegaMax - omegaMin) / (4.0f * kTwoPi);
        gamma = std::max(gamma, 0.01f); // floor to prevent division weirdness

        float Kc = 2.0f * gamma;
        float KcUnlock = Kc * kHysteresisRatio;

        //----------------------------------------------------------------------
        // Step 3: Per-partial phase update with hysteresis (Schulze)
        //         O(N) — single pass
        //----------------------------------------------------------------------
        for (int i = 0; i < numPartials; ++i)
        {
            //------------------------------------------------------------------
            // Phase transition hysteresis: track per-partial lock state
            //------------------------------------------------------------------
            float phaseDiff = std::fabs(psi - theta[i]);
            // Wrap to [0, pi]
            if (phaseDiff > kPi)
                phaseDiff = kTwoPi - phaseDiff;

            // Lock: partial snaps when phase difference < pi/6 AND K >= Kc
            if (!partialLocked_[i] && phaseDiff < kLockPhaseThreshold && Keff >= Kc)
            {
                partialLocked_[i] = true;
            }

            // Unlock: only release when K drops below Kc * 0.7
            if (partialLocked_[i] && Keff < KcUnlock)
            {
                partialLocked_[i] = false;
            }

            //------------------------------------------------------------------
            // Effective coupling: locked partials get stronger pull (1.3x)
            //------------------------------------------------------------------
            float effectiveK = partialLocked_[i] ? (Keff * kLockedCouplingBoost) : Keff;

            //------------------------------------------------------------------
            // Kuramoto phase update:
            //   dtheta_i/dt = omega_i + K_eff * r * sin(psi - theta_i)
            //   theta_i += dtheta * dt
            //------------------------------------------------------------------
            float dtheta = omega[i] + effectiveK * r * fastSin(psi - theta[i]);
            theta[i] += dtheta * dt_;

            //------------------------------------------------------------------
            // Wrap phase to [0, 2*pi]
            // FIX F2: replaced while-loops with floor-based modulo — high-frequency
            // partials (e.g. partial 48 at 48kHz) accumulate up to ~3.5 × 2pi per
            // Kuramoto block (8 samples), so while-loops iterate 3-4 times each.
            // std::floor is a single instruction on modern FPUs.
            //------------------------------------------------------------------
            if (theta[i] >= kTwoPi || theta[i] < 0.0f)
                theta[i] -= kTwoPi * std::floor(theta[i] / kTwoPi);
        }

        //----------------------------------------------------------------------
        // Step 4: Acausal resonance cluster detection and boost
        //----------------------------------------------------------------------
        detectAcausalClusters(theta, numPartials, resSens);
        applyClusterBoost(numPartials, resSens);

        //----------------------------------------------------------------------
        // Advance global sample counter
        //----------------------------------------------------------------------
        sampleCounter_ += static_cast<uint64_t>(kKuraBlock);
    }

    //==========================================================================
    // Queries — read by spatial panning, reactive stage, coupling tap
    //==========================================================================

    /// Order parameter r(t) in [0, 1]. 0 = incoherent, 1 = fully locked.
    float getOrderParameter() const noexcept { return orderParameter_; }

    /// Mean phase psi(t) in [-pi, pi].
    float getMeanPhase() const noexcept { return meanPhase_; }

    /// Whether partial i is in the locked (hysteresis) state.
    bool isPartialLocked(int i) const noexcept
    {
        if (i < 0 || i >= kMaxPartials)
            return false;
        return partialLocked_[i];
    }

    /// Per-partial amplitude boost from acausal resonance clusters.
    /// Returns a linear gain multiplier (1.0 = no boost, up to ~2.0 = +6 dB).
    float getClusterBoost(int i) const noexcept
    {
        if (i < 0 || i >= kMaxPartials)
            return 1.0f;
        return clusterBoost_[i];
    }

    /// Number of detected acausal resonance clusters.
    int getNumClusters() const noexcept { return numClusters_; }

    /// Access detected cluster data (for visualization / coupling tap).
    const Cluster& getCluster(int c) const noexcept
    {
        static const Cluster empty{};
        if (c < 0 || c >= numClusters_)
            return empty;
        return detectedClusters_[c];
    }

    /// Current smoothed K value (for debug / visualization).
    float getSmoothedK() const noexcept { return kSmoother_.get(); }

    //==========================================================================
    // Block counter management — the voice calls shouldUpdate() each sample
    //==========================================================================

    /// Returns true every kKuraBlock samples. The caller should then invoke
    /// updateField(). Between updates, partials free-run at omega[i].
    bool shouldUpdate() noexcept
    {
        if (++blockCounter_ >= kKuraBlock)
        {
            blockCounter_ = 0;
            return true;
        }
        return false;
    }

    //==========================================================================
    // Emotional Memory (Vangelis)
    //
    // When a note is released and retriggered within 500ms, the Kuramoto
    // field retains partial memory of its synchronization state.
    // Quadratic decay: blend = max(0, 1 - (t/500ms)^2)
    //==========================================================================

    /// Call on noteOff to store the current Kuramoto state into emotional memory.
    void onNoteOff(const float* theta, const bool* locked, int numPartials) noexcept
    {
        numPartials = std::min(numPartials, kMaxPartials);

        for (int i = 0; i < numPartials; ++i)
        {
            memory_.storedPhases[i] = theta[i];
            memory_.storedLocked[i] = locked != nullptr ? locked[i] : partialLocked_[i];
        }
        // Zero out any remaining slots
        for (int i = numPartials; i < kMaxPartials; ++i)
        {
            memory_.storedPhases[i] = 0.0f;
            memory_.storedLocked[i] = false;
        }

        memory_.storedR = orderParameter_;
        memory_.releaseTimeSamples = sampleCounter_;
        memory_.numPartials = numPartials;
        memory_.valid = true;
    }

    /// Convenience overload that reads lock state from internal array.
    void onNoteOff() noexcept
    {
        // Caller must have valid theta somewhere — use stored phases if needed
        // This version stores whatever we have as the last known state.
        memory_.storedR = orderParameter_;
        memory_.releaseTimeSamples = sampleCounter_;
        memory_.valid = true;

        for (int i = 0; i < kMaxPartials; ++i)
            memory_.storedLocked[i] = partialLocked_[i];
    }

    /// Call on noteOn to blend stored emotional memory into the fresh phase array.
    /// theta is the freshly-initialised phase array that will be modified in-place.
    void onNoteOn(float* theta, int numPartials) noexcept
    {
        if (!memory_.valid)
            return;

        numPartials = std::min(numPartials, kMaxPartials);

        //----------------------------------------------------------------------
        // Compute elapsed time since release
        //----------------------------------------------------------------------
        float elapsedMs = static_cast<float>(sampleCounter_ - memory_.releaseTimeSamples) / sampleRate_ * 1000.0f;

        if (elapsedMs > kEmotionalMemoryWindowMs)
        {
            memory_.valid = false;
            return;
        }

        //----------------------------------------------------------------------
        // Quadratic decay blend:
        //   blend = max(0, 1 - (t / 500ms)^2)
        //   At t=0:   blend = 1.0 (full recall)
        //   At t=250: blend = 0.75
        //   At t=354: blend = 0.5
        //   At t=500: blend = 0.0 (no recall)
        //----------------------------------------------------------------------
        float normTime = elapsedMs / kEmotionalMemoryWindowMs;
        float blend = std::max(0.0f, 1.0f - normTime * normTime);

        //----------------------------------------------------------------------
        // Blend stored phases toward the freshly-initialised phases.
        // For partials that were locked, restore lock state if blend is strong.
        //----------------------------------------------------------------------
        int overlapCount = std::min(numPartials, memory_.numPartials);

        for (int i = 0; i < overlapCount; ++i)
        {
            // FIX F10: circular (shortest-arc) interpolation instead of linear lerp.
            // Linear lerp across the 0/2pi boundary takes the long path (e.g. lerp between
            // 0.1 and 6.2 goes backward through pi rather than forward through ~0.15),
            // producing a phase discontinuity click on re-attack.
            // Circular lerp: wrap the difference to [-pi, pi], then interpolate.
            float diff = memory_.storedPhases[i] - theta[i];
            // Wrap diff to [-pi, pi]
            diff = diff - kTwoPi * std::floor((diff + kPi) / kTwoPi);
            theta[i] = theta[i] + diff * blend;

            // Wrap after interpolation
            if (theta[i] >= kTwoPi || theta[i] < 0.0f)
                theta[i] -= kTwoPi * std::floor(theta[i] / kTwoPi);

            // Restore lock state if recall is strong enough
            if (blend > 0.5f)
                partialLocked_[i] = memory_.storedLocked[i];
        }
    }

    /// Advance the internal sample counter by numSamples. Call this from the
    /// voice's per-sample loop (or pass the total samples processed).
    void advanceSampleCounter(uint64_t numSamples) noexcept { sampleCounter_ += numSamples; }

    /// Set the sample counter directly (for syncing with external timing).
    void setSampleCounter(uint64_t count) noexcept { sampleCounter_ = count; }

    /// Get the current sample counter (for emotional memory timing).
    uint64_t getSampleCounter() const noexcept { return sampleCounter_; }

private:
    //==========================================================================
    // Acausal Resonance Detection
    //
    // Sliding-window local order parameter for partial clusters.
    // "Adjacent" = consecutive partial index (harmonically related).
    //
    // Detection criteria:
    //   - local_r > threshold  (sensitivity-controlled)
    //   - global_r < 0.7       (not already globally synchronised)
    //   - cluster size 3..5
    //==========================================================================

    void detectAcausalClusters(const float* theta, int numPartials, float resSens) noexcept
    {
        numClusters_ = 0;

        if (numPartials < kClusterMinSize)
            return;

        //----------------------------------------------------------------------
        // Sensitivity threshold mapping:
        //   resSens=0 -> threshold=0.9 (almost impossible to detect)
        //   resSens=1 -> threshold=0.1 (everything detects)
        //----------------------------------------------------------------------
        float sensitivityThreshold = (1.0f - resSens) * 0.8f + 0.1f;
        float detectionBar = 1.0f - sensitivityThreshold; // local_r must exceed this

        for (int start = 0; start <= numPartials - kClusterMinSize; ++start)
        {
            for (int size = kClusterMinSize; size <= kClusterMaxSize && start + size <= numPartials; ++size)
            {
                //--------------------------------------------------------------
                // Compute local order parameter for partials [start, start+size)
                //--------------------------------------------------------------
                float localCos = 0.0f;
                float localSin = 0.0f;

                for (int k = 0; k < size; ++k)
                {
                    localCos += fastCos(theta[start + k]);
                    localSin += fastSin(theta[start + k]);
                }

                float invSize = 1.0f / static_cast<float>(size);
                float lc = localCos * invSize;
                float ls = localSin * invSize;
                float localR = std::sqrt(lc * lc + ls * ls);

                //--------------------------------------------------------------
                // Cluster detected if:
                //   1. Local coherence exceeds sensitivity threshold
                //   2. Global coherence is below full synchrony (< 0.7)
                //      (if globally synced, the boost would be redundant)
                //--------------------------------------------------------------
                if (localR > detectionBar && orderParameter_ < 0.7f)
                {
                    if (numClusters_ < kMaxClusters)
                    {
                        Cluster& c = detectedClusters_[numClusters_++];
                        c.size = size;
                        c.coherence = localR;

                        for (int k = 0; k < size; ++k)
                            c.partials[k] = start + k;

                        // Zero out unused slots
                        for (int k = size; k < kClusterMaxSize; ++k)
                            c.partials[k] = 0;
                    }

                    // Skip past this cluster — don't double-count partials
                    start += size - 1; // -1 because the outer loop will ++start
                    break;             // take the first valid window size at this position
                }
            }
        }
    }

    //==========================================================================
    // Cluster Boost
    //
    // Boost locked cluster amplitudes by +2-3 dB (linear gain 1.26..1.41)
    // based on cluster coherence and resSens.
    //
    // boost = 1.0 + coherence * resSens
    //   At coherence=1.0, resSens=1.0: boost = 2.0 (+6 dB, upper bound)
    //   At coherence=0.85, resSens=0.5: boost = 1.425 (+3.1 dB)
    //   At coherence=0.9, resSens=0.3: boost = 1.27 (+2.1 dB)
    //==========================================================================

    void applyClusterBoost(int numPartials, float resSens) noexcept
    {
        // Reset all boost factors
        for (int i = 0; i < numPartials; ++i)
            clusterBoost_[i] = 1.0f;

        for (int c = 0; c < numClusters_; ++c)
        {
            const Cluster& cluster = detectedClusters_[c];
            float boost = 1.0f + cluster.coherence * resSens;

            for (int k = 0; k < cluster.size; ++k)
            {
                int idx = cluster.partials[k];
                if (idx >= 0 && idx < numPartials)
                    clusterBoost_[idx] = boost;
            }
        }
    }

    //==========================================================================
    // State
    //==========================================================================

    // Sample rate and derived constants
    float sampleRate_ = 0.0f;  // Sentinel: must be set by prepare() before use
    int maxPartials_ = kMaxPartials;
    float dt_ = 0.0f;  // Sentinel: computed from sampleRate_ in prepare()

    // Order parameter (updated every kKuraBlock samples)
    float orderParameter_ = 0.0f; // r(t) in [0, 1]
    float meanPhase_ = 0.0f;      // psi(t) in [-pi, pi]

    // Block update counter
    int blockCounter_ = 0;

    // Global sample counter (for emotional memory timing)
    uint64_t sampleCounter_ = 0;

    // Per-partial hysteresis lock state
    bool partialLocked_[kMaxPartials] = {};

    // Per-partial cluster boost (linear gain, 1.0 = no boost)
    float clusterBoost_[kMaxPartials] = {};

    // K smoother (response speed)
    KSmoother kSmoother_;

    // Cached smoother time — used to skip redundant prepare() calls inside
    // updateField(). Initialised to -1 so the first call always runs prepare().
    float cachedSmootherTime_ = -1.0f;

    // Acausal resonance clusters
    Cluster detectedClusters_[kMaxClusters] = {};
    int numClusters_ = 0;

    // Emotional memory
    EmotionalMemory memory_{};
};

} // namespace opera
