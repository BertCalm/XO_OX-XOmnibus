#pragma once
#include "OperaConstants.h"
//==============================================================================
// OperaPartialBank.h — Additive Partial Bank with Formant Shaping & Spatial Panning
//
// XOpera Engine #45 — XO_OX Designs
// Kuramoto-coupled additive synthesis: 4-48 sine oscillators per voice,
// formant-weighted, spectrally tilted, spatially panned by phase coherence.
//
// This module is purely rendering: given partial phases/frequencies/amplitudes
// (set externally by KuramotoField), it renders them to a stereo buffer with
// formant weighting and spatial positioning.
//
// Does NOT handle: voice allocation, envelopes, or Kuramoto update.
// Those are separate modules (OperaVoice, KuramotoField).
//
// References:
//   - Peterson & Barney (1952) — formant frequencies
//   - Fant (1960) — F4-F5 extension
//   - Kuramoto (1975) — synchronicity field
//   - Acebron et al. (2005) — mean-field decomposition
//   - Ciani — spatial Kuramoto panning concept
//
// All code inline. No allocation on audio thread. #pragma once.
//==============================================================================

#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace opera {

//==============================================================================
// FastMath — 1024-entry sine/cosine lookup table + utilities
//
// Table-based lookup for inner-loop partial rendering. The polynomial fastSin
// from XOmnibus is great for general use, but with up to 48 partials per voice
// times 8 voices, a table lookup saves meaningful CPU.
//==============================================================================

namespace FastMath {

//------------------------------------------------------------------------------
// Sine lookup table — initialized once at static init time.
// 1024 entries covering [0, 2*pi). Index = phase / (2*pi) * 1024.
//------------------------------------------------------------------------------

struct SinTable {
    float table[kSinTableSize];

    SinTable() noexcept
    {
        for (int i = 0; i < kSinTableSize; ++i)
            table[i] = std::sin(static_cast<float>(i) * kTwoPi / static_cast<float>(kSinTableSize));
    }
};

// Meyers singleton — constructed once, no allocation after startup.
inline const SinTable& getSinTable() noexcept
{
    static const SinTable instance;
    return instance;
}

//------------------------------------------------------------------------------
/// Fast sine via 1024-entry table with linear interpolation.
/// Input: radians (any range). Output: [-1, 1].
/// Accuracy: ~16-bit (better than -90 dB THD).
//------------------------------------------------------------------------------
inline float fastSin(float radians) noexcept
{
    const auto& tbl = getSinTable();

    // Normalize to [0, 1) period
    float phase = radians * (1.0f / kTwoPi);
    phase -= std::floor(phase);  // wrap to [0, 1)

    float indexF = phase * static_cast<float>(kSinTableSize);
    int idx0 = static_cast<int>(indexF);
    float frac = indexF - static_cast<float>(idx0);

    idx0 &= kSinTableMask;
    int idx1 = (idx0 + 1) & kSinTableMask;

    return tbl.table[idx0] + frac * (tbl.table[idx1] - tbl.table[idx0]);
}

//------------------------------------------------------------------------------
/// Fast cosine via phase-shifted table lookup. Same accuracy as fastSin.
//------------------------------------------------------------------------------
inline float fastCos(float radians) noexcept
{
    return fastSin(radians + kHalfPi);
}

//------------------------------------------------------------------------------
/// Flush denormals to zero. Critical for feedback paths.
//------------------------------------------------------------------------------
inline float flushDenormal(float x) noexcept
{
    uint32_t bits;
    std::memcpy(&bits, &x, sizeof(bits));
    if ((bits & 0x7F800000) == 0 && (bits & 0x007FFFFF) != 0)
        return 0.0f;
    return x;
}

//------------------------------------------------------------------------------
/// Clamp a float to [lo, hi].
//------------------------------------------------------------------------------
inline float clamp(float x, float lo, float hi) noexcept
{
    return (x < lo) ? lo : ((x > hi) ? hi : x);
}

//------------------------------------------------------------------------------
/// Linear interpolation.
//------------------------------------------------------------------------------
inline float lerp(float a, float b, float t) noexcept
{
    return a + t * (b - a);
}

//------------------------------------------------------------------------------
/// Fast exp using Schraudolph's method. Accurate to ~4% across [-10, 10].
//------------------------------------------------------------------------------
inline float fastExp(float x) noexcept
{
    if (x < -87.0f) return 0.0f;
    if (x >  88.0f) return 3.4028235e+38f;
    int32_t bits = static_cast<int32_t>(12102203.0f * x + 1065353216.0f);
    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

} // namespace FastMath

//==============================================================================
// PartialState — Per-partial data. Phases/frequencies are set by KuramotoField;
// amplitudes and panning are computed by OperaPartialBank.
//==============================================================================

struct PartialState {
    float theta     = 0.0f;   // phase in radians (set by KuramotoField)
    float omega     = 0.0f;   // natural angular frequency in rad/s
    float amplitude = 0.0f;   // formant-weighted amplitude (computed here)
    float pan       = 0.0f;   // stereo position [-1, 1] (computed here)
};

//==============================================================================
// FormantProfile — F1-F5 center frequencies and bandwidths for a vowel.
//==============================================================================

struct FormantProfile {
    float freq[5] = {};    // formant center frequencies (Hz)
    float bw[5]   = {};    // formant bandwidths (Hz)
};

//==============================================================================
// FormantTable — 6 vowel profiles: A, E, I, O, U, Alien
// Peterson & Barney (1952), Fant (1960) — from architecture_spec.md Section 4.1
//==============================================================================

struct FormantTable {
    static constexpr int kNumVowels = 6;

    FormantProfile profiles[kNumVowels];

    FormantTable() noexcept
    {
        // A (ah) — Peterson & Barney averaged male/female
        profiles[0] = {{ 730.0f, 1090.0f, 2440.0f, 3400.0f, 4500.0f },
                        {  80.0f,   90.0f,  120.0f,  150.0f,  200.0f }};

        // E (eh)
        profiles[1] = {{ 530.0f, 1840.0f, 2480.0f, 3400.0f, 4500.0f },
                        {  60.0f,   80.0f,  100.0f,  150.0f,  200.0f }};

        // I (ee)
        profiles[2] = {{ 270.0f, 2290.0f, 3010.0f, 3400.0f, 4500.0f },
                        {  40.0f,   70.0f,  110.0f,  150.0f,  200.0f }};

        // O (oh)
        profiles[3] = {{ 570.0f,  840.0f, 2410.0f, 3400.0f, 4500.0f },
                        {  70.0f,   80.0f,  120.0f,  150.0f,  200.0f }};

        // U (oo)
        profiles[4] = {{ 300.0f,  870.0f, 2240.0f, 3400.0f, 4500.0f },
                        {  50.0f,   60.0f,  100.0f,  150.0f,  200.0f }};

        // Alien — impossible for human vocal tract
        profiles[5] = {{ 180.0f, 1560.0f, 3850.0f, 6200.0f, 8400.0f },
                        { 120.0f,  200.0f,  300.0f,  400.0f,  500.0f }};
    }

    inline const FormantProfile& operator[](int index) const noexcept
    {
        return profiles[FastMath::clamp(static_cast<float>(index), 0.0f,
                        static_cast<float>(kNumVowels - 1)) == static_cast<float>(index)
                        ? index : 0];
    }

    inline const FormantProfile& get(int index) const noexcept
    {
        if (index < 0) index = 0;
        if (index >= kNumVowels) index = kNumVowels - 1;
        return profiles[index];
    }
};

// Meyers singleton — constructed once at static init.
inline const FormantTable& getFormantTable() noexcept
{
    static const FormantTable instance;
    return instance;
}

//==============================================================================
// OperaPartialBank — The core rendering module.
//
// Responsibilities:
//   1. Partial generation: render N sine oscillators from PartialState array
//   2. Formant shaping: weight amplitudes by vowel profile (Gaussian peaks)
//   3. Vowel morphing: log-frequency interpolation between two vowels
//   4. Spectral tilt: per-partial amplitude scaling (dark <-> bright)
//   5. Spatial panning: per-partial stereo from Kuramoto phase coherence
//   6. Non-uniform partial spacing: 75% harmonic + 25% formant-targeted
//   7. Lorentzian detuning: deterministic pseudo-random spread from harmonics
//   8. Anti-aliasing: fade partials approaching Nyquist
//
// Usage pattern (called from OperaVoice per block):
//   1. Call prepare() once with sample rate
//   2. Call computePartialRatios() when note-on or formant params change
//   3. Call computeLorentzianDetune() when note-on or detune param changes
//   4. Call initPartialFrequencies() on note-on
//   5. Per block: call computeFormantWeights() then renderBlock()
//==============================================================================

struct OperaPartialBank {

    //--------------------------------------------------------------------------
    // State
    //--------------------------------------------------------------------------

    PartialState partials[kMaxPartials];    // per-partial phase/freq/amp/pan
    float partialRatios[kMaxPartials];      // frequency ratios (may be non-harmonic)
    float detuneOffsets[kMaxPartials];      // Lorentzian detune per partial
    float formantWeights[kMaxPartials];     // cached formant amplitude weights
    float nyquistGains[kMaxPartials];       // anti-aliasing fade per partial

    int   numPartials = 32;                 // active partial count [4, 48]
    float sampleRate  = 48000.0f;
    float invSampleRate = 1.0f / 48000.0f;

    //--------------------------------------------------------------------------
    /// Call once at startup or when sample rate changes.
    //--------------------------------------------------------------------------
    inline void prepare(float sr) noexcept
    {
        sampleRate = sr;
        invSampleRate = 1.0f / sr;

        // Zero all state
        for (int i = 0; i < kMaxPartials; ++i) {
            partials[i] = {};
            partialRatios[i] = static_cast<float>(i + 1);
            detuneOffsets[i] = 0.0f;
            formantWeights[i] = 0.0f;
            nyquistGains[i] = 1.0f;
        }
    }

    //==========================================================================
    // 1. NON-UNIFORM PARTIAL SPACING (Section 4.4)
    //
    // 75% of partials at the harmonic series (1f, 2f, 3f, ...)
    // 25% placed near formant center frequencies for better coverage.
    // Only engages when numPartials > 16.
    //==========================================================================

    inline void computePartialRatios(const FormantProfile& formants, float f0) noexcept
    {
        if (f0 < 1.0f) f0 = 1.0f;  // safety

        int harmonicCount = (numPartials * 3) / 4;  // 75% harmonic

        // When numPartials <= 16, all go to harmonic series
        if (numPartials <= 16)
            harmonicCount = numPartials;

        // Place harmonic partials: 1, 2, 3, ...
        for (int i = 0; i < harmonicCount; ++i)
            partialRatios[i] = static_cast<float>(i + 1);

        // Place formant-targeted partials near formant centers
        int placed = harmonicCount;
        for (int f = 0; f < 5 && placed < numPartials; ++f) {
            float targetRatio = formants.freq[f] / f0;

            // Nearest half-integer ratio (allows non-harmonic placement)
            float nearestRatio = std::round(targetRatio * 2.0f) * 0.5f;
            if (nearestRatio < 1.0f) nearestRatio = 1.0f;

            // Check for duplicates (within 0.25 of existing)
            bool duplicate = false;
            for (int i = 0; i < placed; ++i) {
                if (std::fabs(partialRatios[i] - nearestRatio) < 0.25f) {
                    duplicate = true;
                    break;
                }
            }

            if (!duplicate && placed < numPartials) {
                partialRatios[placed++] = nearestRatio;
            }
        }

        // Fill any remaining slots with next harmonic positions
        for (int i = placed; i < numPartials; ++i)
            partialRatios[i] = static_cast<float>(harmonicCount + (i - placed) + 1);

        // Sort ratios for orderly processing
        std::sort(partialRatios, partialRatios + numPartials);
    }

    //==========================================================================
    // 2. LORENTZIAN DETUNING (Section 3.6)
    //
    // CRITICAL: Without detuning, the Kuramoto field is inaudible.
    // Partials are offset from exact harmonics by a deterministic pseudo-random
    // amount with Lorentzian (heavy-tailed) distribution.
    //
    // The detune parameter scales the spread:
    //   detuneOffset_i = lorentzian(hash(i)) * detuneAmount * 0.03
    //   omega_i = 2*pi * f0 * ratio_i * (1 + detuneOffset_i)
    //
    // gamma = detuneAmount * f0 * 0.03  =>  Kc = 2 * gamma
    //==========================================================================

    inline void computeLorentzianDetune(float detuneAmount) noexcept
    {
        for (int i = 0; i < kMaxPartials; ++i) {
            // Deterministic hash of partial index (Knuth multiplicative hash)
            uint32_t hash = (static_cast<uint32_t>(i) * 2654435761u) >> 16;
            float normalizedHash = static_cast<float>(hash & 0xFFFF) / 32768.0f - 1.0f;  // [-1, 1]

            // Lorentzian spread: tan(pi/2 * x) gives heavy tails
            // Use 0.48 * pi to avoid infinity at +/-1
            float lorentzian = std::tan(normalizedHash * 0.48f * kPi);

            // Clamp extreme tails to +/-3 standard deviations
            lorentzian = FastMath::clamp(lorentzian, -3.0f, 3.0f);

            // 3% max detune at detuneAmount = 1.0
            detuneOffsets[i] = lorentzian * detuneAmount * 0.03f;
        }
    }

    //==========================================================================
    // 3. PARTIAL FREQUENCY INITIALIZATION
    //
    // Sets omega (angular frequency) for each partial based on:
    //   omega_i = 2*pi * f0 * ratio_i * (1 + detuneOffset_i)
    //
    // Call on note-on and when fundamental changes.
    //==========================================================================

    inline void initPartialFrequencies(float f0) noexcept
    {
        for (int i = 0; i < numPartials; ++i) {
            float freqHz = f0 * partialRatios[i] * (1.0f + detuneOffsets[i]);
            partials[i].omega = kTwoPi * freqHz;
        }
    }

    //==========================================================================
    // 4. FORMANT SHAPING (Section 4.2)
    //
    // Per-partial amplitude weighting from 5 Gaussian formant peaks.
    // Each formant has decreasing relative amplitude:
    //   F1=1.0, F2=0.8, F3=0.5, F4=0.3, F5=0.2
    //
    // Weight = sum over f of: formantGain[f] * exp(-0.5 * ((freq - fc) / (bw/2))^2)
    //==========================================================================

    static constexpr float kFormantGain[5] = { 1.0f, 0.8f, 0.5f, 0.3f, 0.2f };

    /// Compute the formant weight for a single frequency.
    static inline float computeFormantWeight(float partialFreqHz,
                                             const FormantProfile& profile) noexcept
    {
        float weight = 0.0f;
        for (int f = 0; f < 5; ++f) {
            float fc = profile.freq[f];
            float bw = profile.bw[f];

            // Gaussian formant peak model
            float deviation = (partialFreqHz - fc) / (bw * 0.5f);
            float contribution = FastMath::fastExp(-0.5f * deviation * deviation);

            weight += contribution * kFormantGain[f];
        }
        return FastMath::clamp(weight, 0.0f, 1.0f);
    }

    //==========================================================================
    // 5. VOWEL MORPHING (Section 4.3)
    //
    // Logarithmic interpolation for formant frequencies (perceptually linear).
    // Linear interpolation for bandwidths.
    //==========================================================================

    static inline FormantProfile interpolateFormants(const FormantProfile& vowelA,
                                                     const FormantProfile& vowelB,
                                                     float morphPosition) noexcept
    {
        FormantProfile result;
        morphPosition = FastMath::clamp(morphPosition, 0.0f, 1.0f);

        for (int f = 0; f < 5; ++f) {
            // Log interpolation for frequencies — perceptually even
            float logA = std::log(vowelA.freq[f]);
            float logB = std::log(vowelB.freq[f]);
            result.freq[f] = std::exp(FastMath::lerp(logA, logB, morphPosition));

            // Linear interpolation for bandwidths
            result.bw[f] = FastMath::lerp(vowelA.bw[f], vowelB.bw[f], morphPosition);
        }
        return result;
    }

    /// Compute and cache formant weights for all active partials.
    /// Call once per block (or when vowel/formant params change).
    inline void computeFormantWeights(const FormantProfile& profile, float f0) noexcept
    {
        for (int i = 0; i < numPartials; ++i) {
            float freqHz = f0 * partialRatios[i] * (1.0f + detuneOffsets[i]);
            formantWeights[i] = computeFormantWeight(freqHz, profile);
        }

        // Zero weights for inactive partials
        for (int i = numPartials; i < kMaxPartials; ++i)
            formantWeights[i] = 0.0f;
    }

    //==========================================================================
    // 6. SPECTRAL TILT (Section 9, param table row 9)
    //
    // Per-partial amplitude scaling:
    //   a_i *= pow(i/N, tilt * 3.0)
    //
    // tilt < 0: dark (higher partials attenuated)
    // tilt = 0: neutral
    // tilt > 0: bright (higher partials boosted)
    //
    // Bipolar: use != 0, not > 0 (negative tilt sweeps downward).
    //==========================================================================

    static inline float computeSpectralTilt(int partialIndex, int totalPartials,
                                            float tilt) noexcept
    {
        if (totalPartials <= 1) return 1.0f;

        // Ratio in (0, 1] — partial 0 is always 1.0 (fundamental unaffected)
        float ratio = static_cast<float>(partialIndex + 1) / static_cast<float>(totalPartials);

        // tilt * 3.0 gives useful range: at tilt=+1 highest partial is ratio^3
        float exponent = tilt * 3.0f;

        // pow(ratio, exponent) — use exp(log) for fractional exponents
        // Guard: ratio is always > 0 since partialIndex >= 0 and totalPartials >= 1
        if (exponent == 0.0f) return 1.0f;
        return std::exp(exponent * std::log(ratio));
    }

    //==========================================================================
    // 7. SPATIAL PANNING (Section 7 — Ciani)
    //
    // Per-partial stereo position derived from phase coherence with mean field:
    //   - High coherence (synced) -> center
    //   - Low coherence (desynced) -> edges
    //   - Odd/even partial index determines which edge
    //   - Alternating quadrants (partialIdx & 2) for richer distribution
    //
    // Uses the Kuramoto order parameter (r) and mean phase (psi) passed in
    // from the KuramotoField module.
    //==========================================================================

    static inline float computePartialPan(int partialIdx, float theta_i,
                                          float psi, float /*r*/,
                                          float widthParam) noexcept
    {
        // Note: r (order parameter) is accepted for API compatibility with the
        // spec signature but not used directly — coherence is derived from the
        // phase difference cos(theta_i - psi), which is the per-partial analog.

        // Phase difference from mean field
        float phaseDiff = theta_i - psi;

        // Normalize to [-pi, pi]
        // Branchless wrap: faster than while loops for audio thread
        phaseDiff = phaseDiff - kTwoPi * std::floor((phaseDiff + kPi) / kTwoPi);

        // Coherence: how close this partial is to the mean phase
        // 1.0 = perfectly aligned, 0.0 = maximally desynced
        float coherence = (1.0f + FastMath::fastCos(phaseDiff)) * 0.5f;

        // Edge sign: odd partials go left, even go right
        float edgeSign = (partialIdx & 1) ? -1.0f : 1.0f;

        // Alternating quadrants for richer spatial distribution
        if (partialIdx & 2) edgeSign *= -0.7f;  // inner positions

        float pan = edgeSign * (1.0f - coherence) * widthParam;
        return FastMath::clamp(pan, -1.0f, 1.0f);
    }

    //==========================================================================
    // 8. ANTI-ALIASING (Section 9.6)
    //
    // Fade out partials whose frequency approaches Nyquist:
    //   - Below 0.9 * Nyquist: gain = 1.0
    //   - At Nyquist: gain = 0.0
    //   - Linear fade between
    //==========================================================================

    static inline float nyquistFade(float partialFreqHz, float sr) noexcept
    {
        float nyquist  = sr * 0.5f;
        float fadeStart = nyquist * 0.9f;
        if (partialFreqHz < fadeStart) return 1.0f;
        if (partialFreqHz >= nyquist)  return 0.0f;
        return (nyquist - partialFreqHz) / (nyquist - fadeStart);
    }

    /// Pre-compute anti-aliasing gains for all active partials.
    /// Call once per block or when frequencies change.
    inline void computeNyquistGains(float f0) noexcept
    {
        for (int i = 0; i < numPartials; ++i) {
            float freqHz = f0 * partialRatios[i] * (1.0f + detuneOffsets[i]);
            nyquistGains[i] = nyquistFade(freqHz, sampleRate);
        }
        for (int i = numPartials; i < kMaxPartials; ++i)
            nyquistGains[i] = 0.0f;
    }

    //==========================================================================
    // RENDER BLOCK — The main audio rendering function.
    //
    // Renders all active partials to a stereo buffer for one block.
    //
    // Parameters:
    //   outputL, outputR   — output buffers (ADDED TO, not overwritten)
    //   numSamples         — block size
    //   f0                 — fundamental frequency in Hz
    //   tilt               — spectral tilt [-1, 1]
    //   widthParam         — stereo width [0, 1]
    //   orderParameter     — Kuramoto r(t) [0, 1] from KuramotoField
    //   meanPhase          — Kuramoto psi(t) from KuramotoField
    //   baseAmplitude      — overall amplitude scaling (e.g. from envelope)
    //
    // The caller is responsible for:
    //   - Clearing output buffers before calling (or accumulating voices)
    //   - Running the Kuramoto update at kKuraBlock intervals
    //   - Setting partial phases (theta) from the Kuramoto field
    //   - Applying envelope and voice mixing after this call
    //
    // Audio thread safe: no allocation, no blocking, no I/O.
    //==========================================================================

    inline void renderBlock(float* outputL,
                            float* outputR,
                            int    numSamples,
                            float  f0,
                            float  tilt,
                            float  widthParam,
                            float  orderParameter,
                            float  meanPhase,
                            float  baseAmplitude) noexcept
    {
        if (numPartials <= 0 || f0 < 1.0f) return;

        // Pre-compute per-partial constants (once per block, not per sample)
        // These are: formant weight * spectral tilt * nyquist fade * base amplitude
        float partialAmp[kMaxPartials];
        float partialPanVal[kMaxPartials];

        for (int i = 0; i < numPartials; ++i) {
            float tiltGain = computeSpectralTilt(i, numPartials, tilt);
            partialAmp[i] = formantWeights[i] * tiltGain * nyquistGains[i] * baseAmplitude;

            // Compute spatial pan position from Kuramoto coherence
            partialPanVal[i] = computePartialPan(i, partials[i].theta, meanPhase,
                                                  orderParameter, widthParam);

            // Cache into PartialState for external inspection
            partials[i].amplitude = partialAmp[i];
            partials[i].pan = partialPanVal[i];
        }

        // Pre-compute constant-power panning coefficients per partial.
        // pan in [-1, 1] -> angle in [0, pi/2]
        // panL = cos(angle), panR = sin(angle)
        float panL[kMaxPartials];
        float panR[kMaxPartials];

        for (int i = 0; i < numPartials; ++i) {
            float angle = (partialPanVal[i] + 1.0f) * 0.25f * kPi;  // [0, pi/2]
            panL[i] = FastMath::fastCos(angle);
            panR[i] = FastMath::fastSin(angle);
        }

        // Per-partial phase increment per sample: omega / sampleRate
        // (between Kuramoto updates, partials free-run at their natural frequency)
        float phaseInc[kMaxPartials];
        for (int i = 0; i < numPartials; ++i)
            phaseInc[i] = partials[i].omega * invSampleRate;

        //----------------------------------------------------------------------
        // Inner loop: per-sample, sum all partials
        //----------------------------------------------------------------------
        for (int s = 0; s < numSamples; ++s) {
            float sumL = 0.0f;
            float sumR = 0.0f;

            for (int i = 0; i < numPartials; ++i) {
                // Skip silent partials
                if (partialAmp[i] < 1e-8f) {
                    // Still advance phase to keep partial in sync
                    partials[i].theta += phaseInc[i];
                    continue;
                }

                // Generate sine sample from current phase
                float sample = partialAmp[i] * FastMath::fastSin(partials[i].theta);

                // Stereo placement via constant-power pan
                sumL += sample * panL[i];
                sumR += sample * panR[i];

                // Advance phase (free-running between Kuramoto updates)
                partials[i].theta += phaseInc[i];
            }

            // Accumulate into output buffers
            outputL[s] += FastMath::flushDenormal(sumL);
            outputR[s] += FastMath::flushDenormal(sumR);
        }

        // Phase wrapping: keep theta in [0, 2*pi) to prevent float precision loss
        // Done once per block (not per sample) for efficiency
        for (int i = 0; i < numPartials; ++i) {
            float& theta = partials[i].theta;
            if (theta >= kTwoPi) theta -= kTwoPi * std::floor(theta / kTwoPi);
            if (theta < 0.0f)    theta += kTwoPi;
        }
    }

    //==========================================================================
    // FULL SETUP — Convenience function to initialize everything for a note-on.
    //
    // Sets up partial ratios, Lorentzian detune, frequencies, formant weights,
    // and Nyquist gains in one call. Use this on note-on events.
    //==========================================================================

    inline void setupForNote(float f0,
                             int   activePartials,
                             float detuneAmount,
                             int   vowelAIndex,
                             int   vowelBIndex,
                             float voiceMorph) noexcept
    {
        numPartials = FastMath::clamp(static_cast<float>(activePartials), 4.0f, 48.0f);
        numPartials = static_cast<int>(numPartials);  // ensure integer

        const auto& table = getFormantTable();
        const FormantProfile& vowelA = table.get(vowelAIndex);
        const FormantProfile& vowelB = table.get(vowelBIndex);

        // Interpolate formant profile
        FormantProfile morphed = interpolateFormants(vowelA, vowelB, voiceMorph);

        // Compute partial layout
        computePartialRatios(morphed, f0);

        // Compute Lorentzian detune offsets
        computeLorentzianDetune(detuneAmount);

        // Set initial frequencies
        initPartialFrequencies(f0);

        // Compute formant amplitude weights
        computeFormantWeights(morphed, f0);

        // Compute anti-aliasing
        computeNyquistGains(f0);
    }

    //==========================================================================
    // UPDATE FORMANTS — Call per-block when voice morph changes smoothly.
    //
    // Lighter than setupForNote: only recomputes formant weights and Nyquist
    // gains. Does NOT recompute partial ratios or Lorentzian detune (those are
    // stable within a note).
    //==========================================================================

    inline void updateFormants(float f0,
                               int   vowelAIndex,
                               int   vowelBIndex,
                               float voiceMorph) noexcept
    {
        const auto& table = getFormantTable();
        const FormantProfile& vowelA = table.get(vowelAIndex);
        const FormantProfile& vowelB = table.get(vowelBIndex);

        FormantProfile morphed = interpolateFormants(vowelA, vowelB, voiceMorph);

        computeFormantWeights(morphed, f0);
        computeNyquistGains(f0);
    }

    //==========================================================================
    // RESET — Clear all partial state (silence). Call on note-off or voice steal.
    //==========================================================================

    inline void reset() noexcept
    {
        for (int i = 0; i < kMaxPartials; ++i) {
            partials[i].theta     = 0.0f;
            partials[i].omega     = 0.0f;
            partials[i].amplitude = 0.0f;
            partials[i].pan       = 0.0f;
            formantWeights[i]     = 0.0f;
            nyquistGains[i]       = 0.0f;
        }
    }

    //==========================================================================
    // DIAGNOSTIC — Compute the current fundamental frequency from partial 0.
    //==========================================================================

    inline float getCurrentF0() const noexcept
    {
        if (numPartials <= 0) return 0.0f;
        return partials[0].omega / kTwoPi;
    }

    //==========================================================================
    // GET MONO SAMPLE — Quick mono sum for coupling tap.
    // Returns the sum of all active partials without spatial panning.
    //==========================================================================

    inline float getMonoSample() const noexcept
    {
        float sum = 0.0f;
        for (int i = 0; i < numPartials; ++i) {
            if (partials[i].amplitude < 1e-8f) continue;
            sum += partials[i].amplitude * FastMath::fastSin(partials[i].theta);
        }
        return sum;
    }
};

// Static member definition (C++14 compatible)
constexpr float OperaPartialBank::kFormantGain[5];

} // namespace opera
