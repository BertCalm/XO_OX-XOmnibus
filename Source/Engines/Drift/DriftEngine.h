#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>
#include <vector>

namespace xomnibus {

//==============================================================================
// DriftNoiseGen — xorshift32 PRNG for noise oscillator.
//==============================================================================
class DriftNoiseGen
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
// DriftVoyagerDrift — Smooth random walk per voice for organic pitch/filter drift.
// Returns bipolar value [-1, 1] scaled by depth.
// Ported from XOdyssey's VoyagerDrift — the signature "alive pad" trait.
//==============================================================================
class DriftVoyagerDrift
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        phase = 0.0;
        currentValue = 0.0f;
        targetValue = 0.0f;
    }

    void seed (uint32_t s) noexcept { rngState = s | 1; }

    float process (float rate, float depth) noexcept
    {
        if (depth < 0.0001f) return 0.0f;

        double inc = static_cast<double> (rate) / sr;
        phase += inc;

        if (phase >= 1.0)
        {
            phase -= 1.0;
            targetValue = nextRandom();
        }

        // Cache coefficient — only recompute when rate changes
        if (rate != lastRate)
        {
            lastRate = rate;
            smoothCoeff = 1.0f - std::exp (-2.0f * 3.14159265f * rate / static_cast<float> (sr));
        }

        currentValue += smoothCoeff * (targetValue - currentValue);
        currentValue = flushDenormal (currentValue);

        return currentValue * depth;
    }

private:
    double sr = 44100.0;
    double phase = 0.0;
    float currentValue = 0.0f;
    float targetValue = 0.0f;
    float lastRate = -1.0f;
    float smoothCoeff = 0.0f;
    uint32_t rngState = 48271;

    float nextRandom() noexcept
    {
        rngState ^= rngState << 13;
        rngState ^= rngState >> 17;
        rngState ^= rngState << 5;
        return static_cast<float> (rngState) / static_cast<float> (UINT32_MAX) * 2.0f - 1.0f;
    }
};

//==============================================================================
// DriftSupersawOsc — 7-voice detuned sawtooth with inline PolyBLEP.
// Ported from XOdyssey's SupersawOsc. Staggered initial phases for immediate
// stereo width. Detune parameter spreads voices symmetrically around center.
//==============================================================================
class DriftSupersawOsc
{
public:
    static constexpr int kNumVoices = 7;

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        for (int v = 0; v < kNumVoices; ++v)
            phases[v] = static_cast<double> (v) / static_cast<double> (kNumVoices);
    }

    void setFrequency (float hz, float detuneAmount) noexcept
    {
        float maxCents = detuneAmount * 50.0f;
        for (int v = 0; v < kNumVoices; ++v)
        {
            float offset = 0.0f;
            if (v > 0)
            {
                int pair = (v + 1) / 2;
                float sign = (v % 2 == 1) ? 1.0f : -1.0f;
                offset = sign * maxCents * static_cast<float> (pair) / 3.0f;
            }
            float voiceHz = hz * fastExp (offset * (0.693147f / 1200.0f));
            phaseIncs[v] = static_cast<double> (voiceHz) / sr;
        }
    }

    float processSample() noexcept
    {
        constexpr float gain = 1.0f / static_cast<float> (kNumVoices);
        double mix = 0.0;

        for (int v = 0; v < kNumVoices; ++v)
        {
            double raw = 2.0 * phases[v] - 1.0;
            raw -= polyBlep (phases[v], phaseIncs[v]);
            mix += raw;

            phases[v] += phaseIncs[v];
            if (phases[v] >= 1.0) phases[v] -= 1.0;
        }

        return static_cast<float> (mix) * gain;
    }

private:
    double sr = 44100.0;
    std::array<double, kNumVoices> phases {};
    std::array<double, kNumVoices> phaseIncs {};

    static double polyBlep (double t, double dt) noexcept
    {
        if (dt <= 0.0) return 0.0;
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0;
        }
        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        }
        return 0.0;
    }
};

//==============================================================================
// DriftFMOsc — 2-operator FM oscillator (modulator -> carrier).
// Ported from XOdyssey's FMOsc. Depth parameter scales modulation index.
//==============================================================================
class DriftFMOsc
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        carrierPhase = 0.0;
        modPhase = 0.0;
    }

    void setFrequency (float hz) noexcept
    {
        carrierInc = static_cast<double> (hz) / sr;
    }

    // ratio: modulator freq multiplier (e.g. 2.0 = harmonic)
    // depth: mod index (0 = pure sine, 1 = rich harmonics)
    float processSample (float ratio, float depth) noexcept
    {
        constexpr double twoPi = 6.28318530717958647692;
        double modInc = carrierInc * static_cast<double> (ratio);
        double modIndex = static_cast<double> (depth) * 5.0;

        double modOut = static_cast<double> (fastSin (static_cast<float> (modPhase * twoPi))) * modIndex;
        double carrierOut = static_cast<double> (fastSin (static_cast<float> ((carrierPhase + modOut) * twoPi)));

        carrierPhase += carrierInc;
        if (carrierPhase >= 1.0) carrierPhase -= 1.0;

        modPhase += modInc;
        if (modPhase >= 1.0) modPhase -= 1.0;

        return static_cast<float> (carrierOut);
    }

private:
    double sr = 44100.0;
    double carrierPhase = 0.0;
    double modPhase = 0.0;
    double carrierInc = 0.0;
};

//==============================================================================
// DriftFormantFilter — 3 parallel Cytomic SVF bandpasses for vowel formants.
// Ported from XOdyssey's FilterB. Morph sweeps through 5 vowel shapes:
//   ah (0.0) → eh (0.25) → ee (0.5) → oh (0.75) → oo (1.0)
//==============================================================================
class DriftFormantFilter
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        for (auto& bp : bands)
        {
            bp.setMode (CytomicSVF::Mode::BandPass);
            bp.reset();
        }
    }

    void reset() noexcept
    {
        for (auto& bp : bands)
            bp.reset();
    }

    void updateCoefficients (float morph, float resonance, float sampleRate) noexcept
    {
        float morphScaled = clamp (morph, 0.0f, 1.0f) * 4.0f;
        int vowelIdx = std::min (static_cast<int> (morphScaled), 3);
        float frac = morphScaled - static_cast<float> (vowelIdx);

        for (int b = 0; b < 3; ++b)
        {
            float f1 = vowelFreqs[vowelIdx][b];
            float f2 = vowelFreqs[vowelIdx + 1][b];
            float center = f1 + frac * (f2 - f1);
            center = clamp (center, 20.0f, sampleRate * 0.49f);

            bands[b].setMode (CytomicSVF::Mode::BandPass);
            bands[b].setCoefficients (center, resonance, sampleRate);
        }
    }

    float process (float input, float mix) noexcept
    {
        if (mix < 0.0001f) return input;

        float formant = 0.0f;
        for (auto& bp : bands)
            formant += bp.processSample (input);

        formant *= 0.4f;  // normalize 3 parallel BPs
        return input * (1.0f - mix) + formant * mix;
    }

private:
    double sr = 44100.0;
    std::array<CytomicSVF, 3> bands;

    static constexpr float vowelFreqs[5][3] = {
        { 730.0f, 1090.0f, 2440.0f },   // ah
        { 530.0f, 1840.0f, 2480.0f },   // eh
        { 270.0f, 2290.0f, 3010.0f },   // ee
        { 570.0f,  840.0f, 2410.0f },   // oh
        { 300.0f,  870.0f, 2240.0f },   // oo
    };
};

//==============================================================================
// DriftHazeSaturation — Pre-filter soft tanh saturation with makeup gain.
// Ported from XOdyssey's HazeSaturation.
//==============================================================================
class DriftHazeSaturation
{
public:
    float process (float input, float amount) noexcept
    {
        if (amount < 0.0001f) return input;

        float driveGain = 1.0f + amount * 4.0f;
        float driven = input * driveGain;
        float saturated = fastTanh (driven);

        float compensation = 1.0f / (1.0f + amount * 0.5f);
        saturated *= compensation;

        return input * (1.0f - amount) + saturated * amount;
    }
};

//==============================================================================
// DriftPrismShimmer — Post-filter harmonic shimmer via full-wave rectification
// and one-pole LP tone control. Ported from XOdyssey's PrismShimmer.
//==============================================================================
class DriftPrismShimmer
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        lpState = 0.0f;
        cachedTone = -1.0f;
    }

    void reset() noexcept { lpState = 0.0f; }

    void updateTone (float tone) noexcept
    {
        if (tone == cachedTone) return;
        cachedTone = tone;
        float cutoff = 500.0f + tone * 15000.0f;
        cachedCoeff = 1.0f - fastExp (-6.28318530f * cutoff / static_cast<float> (sr));
    }

    float process (float input, float amount, float tone) noexcept
    {
        if (amount < 0.0001f) return input;

        updateTone (tone);

        float rectified = std::abs (input);
        float harmonics = rectified - input * 0.5f;

        lpState += cachedCoeff * (harmonics - lpState);
        lpState = flushDenormal (lpState);

        return input + lpState * amount * 0.5f;
    }

private:
    double sr = 44100.0;
    float lpState = 0.0f;
    float cachedTone = -1.0f;
    float cachedCoeff = 0.1f;
};

//==============================================================================
// DriftTidalPulse — sin² breathing LFO for the BREATHE macro.
// Ported from XOdyssey's TidalPulse.
//
// Unlike a standard sine LFO (bipolar, equal positive/negative excursion),
// TidalPulse produces a unipolar [0,1] output with soft peaks and gentle
// valleys — closer to the rhythm of actual breathing. Applied as amplitude
// modulation so the sound swells and recedes rather than oscillates.
//
// Rate range: 0.05–2.0 Hz (one breath every 0.5s to 20s).
// Depth range: 0.0–1.0 (depth=0 disables entirely).
//==============================================================================
class DriftTidalPulse
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        phase = 0.0;
    }

    void reset() noexcept { phase = 0.0; }

    // Returns unipolar output [0, 1] — the breath cycle.
    // Returns 0.0 when depth is negligible (early-out).
    float process (float rate, float depth) noexcept
    {
        if (depth < 0.0001f) return 0.0f;

        double inc = static_cast<double> (rate) / sr;
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;

        // sin² shaping: always positive [0,1], soft peaks, no sharp zero-crossings.
        // Mathematically equivalent to (1 - cos(2π·phase)) / 2, but computed as
        // sine squared for clarity and consistency with XOdyssey's implementation.
        constexpr double twoPi = 6.28318530717958647692;
        float s = static_cast<float> (std::sin (phase * twoPi));
        float breathCycle = s * s;   // unipolar [0, 1]

        return breathCycle * depth;
    }

private:
    double sr = 44100.0;
    double phase = 0.0;
};

//==============================================================================
// DriftFracture — circular-buffer stutter glitch.
// Ported from XOdyssey's Fracture (named Signature Trait in the XOdyssey spec).
//
// Captures audio into a 4096-sample fixed circular buffer (~93ms at 44.1kHz)
// and probabilistically replays fragments, creating stutter / granular-like
// effects. Each voice has its own independent Fracture instance.
//
// Buffer is a std::array (pre-allocated, zero heap on the audio thread).
// The FRACTURE macro directly maps to drift_fractureIntensity.
//==============================================================================
class DriftFracture
{
public:
    static constexpr int kBufferSize = 4096;  // ~93ms at 44100 Hz

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        buffer.fill (0.0f);
        writePos = 0;
        readPos = 0;
        stutterCounter = 0;
        stuttering = false;
        currentStutterLen = 512;
        stutterPlayPos = 0;
    }

    // enable:    on/off gate
    // intensity: 0.0–1.0 — probability and wet/dry mix of the glitch
    // rate:      0.0–1.0 — stutter speed (higher = shorter, faster repeats)
    float process (float input, bool enable, float intensity, float rate) noexcept
    {
        if (!enable || intensity < 0.0001f) return input;

        // Write to circular buffer
        buffer[static_cast<size_t> (writePos)] = input;
        writePos = (writePos + 1) % kBufferSize;

        // Stutter length determined by rate: high rate → short grains
        int stutterLen = static_cast<int> ((1.0f - rate * 0.8f) * static_cast<float> (kBufferSize) * 0.5f);
        stutterLen = std::max (stutterLen, 64);

        if (!stuttering)
        {
            ++stutterCounter;
            // Trigger interval: shorter at higher intensity
            int triggerInterval = static_cast<int> (static_cast<float> (sr)
                * (0.05f + (1.0f - intensity) * 0.4f));
            if (stutterCounter >= triggerInterval)
            {
                stuttering = true;
                stutterCounter = 0;
                readPos = (writePos - stutterLen + kBufferSize) % kBufferSize;
                currentStutterLen = stutterLen;
                stutterPlayPos = 0;
            }
            else
            {
                return input;
            }
        }

        float stuttered = buffer[static_cast<size_t> (readPos)];
        readPos = (readPos + 1) % kBufferSize;
        ++stutterPlayPos;

        if (stutterPlayPos >= currentStutterLen)
        {
            stuttering = false;
            stutterPlayPos = 0;
        }

        return input * (1.0f - intensity) + stuttered * intensity;
    }

private:
    double sr = 44100.0;
    std::array<float, kBufferSize> buffer {};
    int writePos = 0;
    int readPos = 0;
    int stutterCounter = 0;
    bool stuttering = false;
    int currentStutterLen = 512;
    int stutterPlayPos = 0;
};

//==============================================================================
// DriftReverb — Schroeder reverb for per-engine spatial depth.
// Ported from XOdyssey's ReverbFX.
//
// 4 parallel comb filters (mutually-prime delay times: 29.7, 37.1, 41.1,
// 43.7 ms) + 2 series allpass filters. One-pole lowpass damping in the
// feedback path simulates high-frequency absorption of real spaces.
//
// Buffers are pre-allocated in prepare() via std::vector::assign() —
// NO heap allocation in processStereo(). This is safe for the audio thread.
// The DriftEngine allocates one DriftReverb instance (not per-voice).
//==============================================================================
class DriftReverb
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;

        // Comb filter delay times (ms) — mutually prime to avoid flutter echo
        static constexpr float combTimesMs[] = { 29.7f, 37.1f, 41.1f, 43.7f };
        for (size_t c = 0; c < 4; ++c)
        {
            size_t len = static_cast<size_t> (sr * combTimesMs[c] * 0.001);
            combBuffers[c].assign (len, 0.0f);
            combSizes[c] = len;
            combPos[c] = 0;
        }

        // Allpass delay times
        static constexpr float apTimesMs[] = { 5.0f, 1.7f };
        for (size_t a = 0; a < 2; ++a)
        {
            size_t len = static_cast<size_t> (sr * apTimesMs[a] * 0.001);
            apBuffers[a].assign (len, 0.0f);
            apSizes[a] = len;
            apPos[a] = 0;
        }

        combFilterState.fill (0.0f);
    }

    void reset()
    {
        for (auto& buf : combBuffers) std::fill (buf.begin(), buf.end(), 0.0f);
        for (auto& buf : apBuffers)   std::fill (buf.begin(), buf.end(), 0.0f);
        for (auto& p : combPos) p = 0;
        for (auto& p : apPos)   p = 0;
        combFilterState.fill (0.0f);
    }

    // Stereo in-place processing. mix=0 → bypass. size controls feedback (room scale).
    // damping is hardwired to 0.5 (natural sounding default for pads).
    void processStereo (float* left, float* right, int numSamples,
                        float size, float mix) noexcept
    {
        if (mix < 0.0001f || combSizes[0] == 0) return;

        const float feedback = 0.5f + size * 0.45f;   // [0.50, 0.95]
        constexpr float damping = 0.5f;                // fixed — natural pad damping

        for (int i = 0; i < numSamples; ++i)
        {
            float input = (left[i] + right[i]) * 0.5f;

            // 4 parallel comb filters with lowpass damping in feedback
            float combSum = 0.0f;
            for (size_t c = 0; c < 4; ++c)
            {
                float delayed = combBuffers[c][combPos[c]];
                combFilterState[c] = delayed * (1.0f - damping) + combFilterState[c] * damping;
                // Denormal flush — prevents CPU load from subnormal feedback values
                if (std::abs (combFilterState[c]) < 1.0e-15f) combFilterState[c] = 0.0f;
                combBuffers[c][combPos[c]] = input + combFilterState[c] * feedback;
                combPos[c] = (combPos[c] + 1) % combSizes[c];
                combSum += delayed;
            }
            combSum *= 0.25f;

            // 2 series allpass filters
            float ap = combSum;
            for (size_t a = 0; a < 2; ++a)
            {
                float delayed = apBuffers[a][apPos[a]];
                constexpr float apCoeff = 0.5f;
                float out = -ap * apCoeff + delayed;
                apBuffers[a][apPos[a]] = ap + delayed * apCoeff;
                apPos[a] = (apPos[a] + 1) % apSizes[a];
                ap = out;
            }

            float wet = ap;
            left[i]  = left[i]  * (1.0f - mix) + wet * mix;
            right[i] = right[i] * (1.0f - mix) + wet * mix * 0.95f;  // slight R attenuation for width
        }
    }

private:
    double sr = 44100.0;
    std::array<std::vector<float>, 4> combBuffers;
    std::array<size_t, 4> combSizes {};
    std::array<size_t, 4> combPos {};
    std::array<float, 4> combFilterState {};
    std::array<std::vector<float>, 2> apBuffers;
    std::array<size_t, 2> apSizes {};
    std::array<size_t, 2> apPos {};
};

//==============================================================================
// DriftChorus — BDD stereo chorus. Two modulated delay lines (L/R offset
// phases) produce natural width without a dedicated stereo-to-mono collapse.
// Pre-allocated delay buffers; no heap allocation in processBlock().
//==============================================================================
class DriftChorus
{
public:
    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);
        const int maxDelaySamples = static_cast<int> (sr * kMaxDelayMs * 0.001f) + 2;
        for (auto& buf : delayBuf)
            buf.assign (static_cast<size_t> (maxDelaySamples), 0.0f);
        phase = 0.0f;
    }

    void reset()
    {
        for (auto& buf : delayBuf) std::fill (buf.begin(), buf.end(), 0.0f);
        phase = 0.0f;
        writePos = 0;
    }

    // In-place stereo process. rate in Hz, depth 0–1, mix 0–1.
    void processBlock (float* L, float* R, int n, float rate, float depth, float mix) noexcept
    {
        if (mix < 0.0001f) return;

        const float phaseInc = rate / sr;
        // L uses phase; R is offset by π/2 for natural stereo width.
        const float baseDelay = kBaseDelayMs * 0.001f * sr;
        const float modRange  = kMaxModMs  * 0.001f * sr * depth;
        const int bufLen = static_cast<int> (delayBuf[0].size());

        for (int i = 0; i < n; ++i)
        {
            const float modL = fastSin (phase * 6.28318530f) * modRange;
            const float modR = fastSin ((phase + 0.25f) * 6.28318530f) * modRange; // π/2 offset

            auto readLerp = [&] (int ch, float delaySamples) -> float {
                const float rd = static_cast<float> (writePos) - delaySamples;
                int   i0 = static_cast<int> (rd);
                float frac = rd - static_cast<float> (i0);
                i0 = ((i0 % bufLen) + bufLen) % bufLen;
                int   i1 = (i0 + 1) % bufLen;
                return delayBuf[ch][static_cast<size_t> (i0)] * (1.0f - frac)
                     + delayBuf[ch][static_cast<size_t> (i1)] * frac;
            };

            delayBuf[0][static_cast<size_t> (writePos)] = L[i];
            delayBuf[1][static_cast<size_t> (writePos)] = R[i];
            writePos = (writePos + 1) % bufLen;

            const float wetL = readLerp (0, baseDelay + modL);
            const float wetR = readLerp (1, baseDelay + modR);
            L[i] += (wetL - L[i]) * mix;
            R[i] += (wetR - R[i]) * mix;

            phase += phaseInc;
            if (phase >= 1.0f) phase -= 1.0f;
        }
    }

private:
    static constexpr float kBaseDelayMs = 7.0f;  // centre delay (ms)
    static constexpr float kMaxModMs    = 5.0f;  // max modulation swing (ms)
    static constexpr float kMaxDelayMs  = kBaseDelayMs + kMaxModMs + 1.0f;

    float sr = 44100.0f;
    float phase = 0.0f;
    int   writePos = 0;
    std::array<std::vector<float>, 2> delayBuf;
};

//==============================================================================
// DriftPhaser — 4-stage all-pass phaser. LFO modulates all-pass pole frequency
// to sweep the notch pattern through the spectrum. Feedback adds resonance.
// Algorithm: textbook Regalia–Mitra APF chain; coefficients in [−1, +1].
// No per-sample heap allocation; state arrays are value-initialised.
//==============================================================================
class DriftPhaser
{
public:
    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);
        for (auto& s : apState1) s = 0.0f;
        for (auto& s : apState2) s = 0.0f;
        for (auto& s : fbState)  s = 0.0f;
        phase = 0.0f;
    }

    void reset()
    {
        for (auto& s : apState1) s = 0.0f;
        for (auto& s : apState2) s = 0.0f;
        for (auto& s : fbState)  s = 0.0f;
        phase = 0.0f;
    }

    // In-place stereo process. rate in Hz, depth 0–1, feedback −1–+1, mix 0–1.
    void processBlock (float* L, float* R, int n,
                       float rate, float depth, float feedback, float mix) noexcept
    {
        if (mix < 0.0001f) return;

        const float phaseInc  = rate / sr;
        const float minFreq   = 200.0f;
        const float freqRange = 8000.0f;

        for (int i = 0; i < n; ++i)
        {
            const float lfoVal = fastSin (phase * 6.28318530f);
            // LFO → normalised frequency for APF coefficient
            const float freqHz = minFreq + (1.0f + lfoVal) * 0.5f * freqRange * depth;
            const float w = std::tan (juce::MathConstants<float>::pi * freqHz / sr);
            const float a = (w - 1.0f) / (w + 1.0f); // APF coefficient

            auto processChain = [&] (float in, std::array<float, 4>& s1,
                                     std::array<float, 4>& s2, float& fb) -> float {
                const float input = in + fb * feedback;
                float x = input;
                for (int stage = 0; stage < 4; ++stage)
                {
                    float y = a * x + s1[static_cast<size_t>(stage)];
                    s1[static_cast<size_t>(stage)] = x - a * y;
                    x = y;
                }
                fb = x;
                return (in + x) * 0.5f; // wet = dry+wet mix (classic phaser output)
            };

            const float wetL = processChain (L[i], apState1, apState2, fbState[0]);
            const float wetR = processChain (R[i], apState2, apState1, fbState[1]);
            L[i] += (wetL - L[i]) * mix;
            R[i] += (wetR - R[i]) * mix;

            phase += phaseInc;
            if (phase >= 1.0f) phase -= 1.0f;
        }
    }

private:
    float sr = 44100.0f;
    float phase = 0.0f;
    std::array<float, 4> apState1 {};   // APF state for L chain
    std::array<float, 4> apState2 {};   // APF state for R chain (swapped for stereo)
    std::array<float, 2> fbState  {};   // feedback memory per channel
};

//==============================================================================
// DriftDelay — stereo tape delay with LP feedback path. One-pole filter in
// the feedback loop simulates high-frequency tape roll-off. Ping-pong mode
// available via width parameter (0=mono, 1=full ping-pong).
// Pre-allocated ring buffers; no per-sample allocation.
//==============================================================================
class DriftDelay
{
public:
    static constexpr float kMaxDelaySeconds = 2.0f;

    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);
        const int cap = static_cast<int> (sr * kMaxDelaySeconds) + 2;
        for (auto& buf : delayBuf)
            buf.assign (static_cast<size_t> (cap), 0.0f);
        writePos = 0;
        for (auto& s : lpState) s = 0.0f;
    }

    void reset()
    {
        for (auto& buf : delayBuf) std::fill (buf.begin(), buf.end(), 0.0f);
        writePos = 0;
        for (auto& s : lpState) s = 0.0f;
    }

    // In-place stereo process. time 0–1 (maps to 0–2s), feedback 0–0.95, mix 0–1.
    void processBlock (float* L, float* R, int n,
                       float time, float feedbackAmt, float mix) noexcept
    {
        if (mix < 0.0001f) return;

        const int bufLen     = static_cast<int> (delayBuf[0].size());
        const int delaySamples = std::clamp (
            static_cast<int> (time * sr * kMaxDelaySeconds),
            1, bufLen - 1);
        // LP coefficient: ~4 kHz cutoff simulates tape bandwidth
        const float lpCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 4000.0f / sr);

        for (int i = 0; i < n; ++i)
        {
            const int readPos = ((writePos - delaySamples) % bufLen + bufLen) % bufLen;
            float wetL = delayBuf[0][static_cast<size_t>(readPos)];
            float wetR = delayBuf[1][static_cast<size_t>(readPos)];

            // One-pole LP in feedback path
            lpState[0] += lpCoeff * (wetL - lpState[0]);
            lpState[1] += lpCoeff * (wetR - lpState[1]);

            delayBuf[0][static_cast<size_t>(writePos)] = L[i] + lpState[0] * feedbackAmt;
            delayBuf[1][static_cast<size_t>(writePos)] = R[i] + lpState[1] * feedbackAmt;
            writePos = (writePos + 1) % bufLen;

            L[i] += (wetL - L[i]) * mix;
            R[i] += (wetR - R[i]) * mix;
        }
    }

private:
    float sr = 44100.0f;
    int   writePos = 0;
    std::array<float, 2> lpState {};
    std::array<std::vector<float>, 2> delayBuf;
};

//==============================================================================
// DriftAdsrEnvelope — ADSR with exponential decay/release.
// Same pattern as DubAdsrEnvelope for consistency across XOmnibus engines.
//==============================================================================
class DriftAdsrEnvelope
{
public:
    enum class Stage { Off, Attack, Decay, Sustain, Release };

    void prepare (double sampleRate) noexcept { sr = sampleRate; }
    void reset() noexcept { stage = Stage::Off; level = 0.0f; }

    void setParams (float a, float d, float s, float r) noexcept
    {
        attack = std::max (a, 0.001f);
        attackRate = 1.0f / (static_cast<float> (sr) * attack);
        decay = std::max (d, 0.001f);
        // Compute decayRate here so it stays current when the knob changes mid-note.
        // Previously computed only at Attack→Decay transition, causing stale values.
        decayRate = static_cast<float> (
            1.0 - std::exp (-1.0 / (static_cast<double> (decay) * sr)));
        sustain = clamp (s, 0.0f, 1.0f);
        release = std::max (r, 0.001f);
    }

    void noteOn() noexcept
    {
        stage = Stage::Attack;
    }

    void noteOff() noexcept
    {
        if (stage != Stage::Off)
        {
            stage = Stage::Release;
            releaseRate = static_cast<float> (
                1.0 - std::exp (-1.0 / (static_cast<double> (release) * sr)));
        }
    }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Off:
                return 0.0f;

            case Stage::Attack:
                level += attackRate;
                if (level >= 1.0f)
                {
                    level = 1.0f;
                    stage = Stage::Decay;
                    // decayRate is kept current by setParams(); no recompute needed.
                }
                break;

            case Stage::Decay:
                level -= (level - sustain) * decayRate;
                level = flushDenormal (level);
                if (level <= sustain + 0.0001f)
                {
                    level = sustain;
                    stage = Stage::Sustain;
                    if (sustain < 0.0001f)
                    {
                        level = 0.0f;
                        stage = Stage::Off;
                    }
                }
                break;

            case Stage::Sustain:
                level = sustain;
                if (sustain < 0.0001f)
                {
                    level = 0.0f;
                    stage = Stage::Off;
                }
                break;

            case Stage::Release:
                level -= level * releaseRate;
                level = flushDenormal (level);
                if (level < 0.0001f)
                {
                    level = 0.0f;
                    stage = Stage::Off;
                }
                break;
        }
        return level;
    }

    bool isActive() const noexcept { return stage != Stage::Off; }

private:
    double sr = 44100.0;
    Stage stage = Stage::Off;
    float level = 0.0f;
    float attack = 0.1f, decay = 0.5f, sustain = 0.7f, release = 0.5f;
    float attackRate = 0.0f, decayRate = 0.0f, releaseRate = 0.0f;
};

//==============================================================================
// DriftLFO — Sine LFO. Same pattern as DubLFO for consistency.
//==============================================================================
class DriftLFO
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; }

    void reset() noexcept { phase = 0.0; }

    void setRate (float hz) noexcept { rate = hz; }

    float process() noexcept
    {
        constexpr double twoPi = 6.28318530717958647692;
        float out = fastSin (static_cast<float> (phase * twoPi));
        phase += static_cast<double> (rate) / sr;
        if (phase >= 1.0) phase -= 1.0;
        return out;
    }

private:
    double sr = 44100.0;
    double phase = 0.0;
    float rate = 1.5f;
};

//==============================================================================
// DriftVoice — Per-voice state for the ODYSSEY engine.
// Each voice has dual multi-mode oscillators (Classic/Supersaw/FM),
// sub, noise, Haze saturation, dual filters (LP + Formant),
// Prism Shimmer, Voyager Drift, ADSR, and glide.
//==============================================================================
struct DriftVoice
{
    bool active = false;
    int noteNumber = 60;
    float velocity = 0.0f;
    uint64_t age = 0;

    // Osc A: one of three modes active at a time
    PolyBLEP oscA_classic;
    DriftSupersawOsc oscA_supersaw;
    DriftFMOsc oscA_fm;

    // Osc B: independent mode from A
    PolyBLEP oscB_classic;
    DriftSupersawOsc oscB_supersaw;
    DriftFMOsc oscB_fm;

    // Sub oscillator (always sine, one octave below A)
    PolyBLEP subOsc;

    // Noise
    DriftNoiseGen noise;

    // Character stages
    DriftHazeSaturation haze;
    DriftPrismShimmer shimmer;
    DriftTidalPulse tidal;   // BREATHE macro engine (Option B: drift_tidalDepth/Rate)
    DriftFracture fracture;  // FRACTURE macro engine (Option B: drift_fracture*)

    // Filters
    CytomicSVF filterA1;         // Primary LP filter
    CytomicSVF filterA2;         // Cascade for 24dB slope
    DriftFormantFilter filterB;   // Formant filter

    // Modulation
    DriftAdsrEnvelope ampEnv;
    DriftVoyagerDrift drift;

    // Glide
    bool glideActive = false;
    float glideSourceFreq = 0.0f;

    // Block-start cached base frequencies (avoids std::pow in per-sample loop)
    float cachedBaseFreqA = 440.0f;
    float cachedBaseFreqB = 440.0f;
};

//==============================================================================
// DriftEngine — Psychedelic pad synthesizer adapted from XOdyssey.
//
// Signal chain per voice (Option B extended — 45 params, up from 38):
//   OscA + OscB + Sub + Noise → Mix → Haze Saturation → FilterA (LP 12/24)
//   → FilterB (Formant) → Fracture (stutter glitch) → Prism Shimmer
//   → Amp Envelope × TidalPulse (breath) → Output
//   [Post-mix engine-level: Reverb]
//
// Signature traits ported in Option B pass (Round 11B):
//   - TidalPulse: sin² breathing LFO → BREATHE macro now has DSP backing
//   - Fracture: circular-buffer stutter glitch → FRACTURE macro now has DSP
//   - Reverb: Schroeder 4-comb+2-allpass spatial tail (engine-level, stereo)
//
// All previously-ported signature traits:
//   - Voyager Drift: per-voice smooth random walk on pitch/filter
//   - Haze Saturation: pre-filter soft tanh warmth
//   - Prism Shimmer: post-filter harmonic overtones
//   - Formant Filter: 3 parallel SVF BPs with vowel morphing
//   - 3 oscillator modes: Classic (PolyBLEP), Supersaw (7-voice), FM (2-op)
//
// Coupling:
//   Output: envelope level (ch2) for AmpToFilter/AmpToPitch
//   Input:  AmpToFilter, LFOToPitch, AmpToPitch, EnvToMorph
//
// Parameter count: 45 (was 38 — +7 from Option B)
//==============================================================================
class DriftEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    //-- SynthEngine interface -------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        aftertouch.prepare (sampleRate);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            auto& v = voices[static_cast<size_t> (i)];
            v.active = false;
            v.oscA_classic.reset();
            v.oscA_supersaw.prepare (sr);
            v.oscA_fm.prepare (sr);
            v.oscB_classic.reset();
            v.oscB_supersaw.prepare (sr);
            v.oscB_fm.prepare (sr);
            v.subOsc.reset();
            v.noise.seed (static_cast<uint32_t> (i * 7331 + 5));
            v.haze = DriftHazeSaturation {};
            v.shimmer.prepare (sr);
            v.tidal.prepare (sr);
            v.fracture.prepare (sr);
            v.filterA1.reset();
            v.filterA1.setMode (CytomicSVF::Mode::LowPass);
            v.filterA2.reset();
            v.filterA2.setMode (CytomicSVF::Mode::LowPass);
            v.filterB.prepare (sr);
            v.ampEnv.prepare (sr);
            v.ampEnv.reset();
            v.drift.prepare (sr);
            v.drift.seed (static_cast<uint32_t> (i * 8191 + 3));
        }

        lfo.prepare (sr);
        chorus.prepare (sampleRate);
        phaser.prepare (sampleRate);
        delay.prepare  (sampleRate);
        reverb.prepare (sr);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.oscA_classic.reset();
            v.oscA_supersaw.reset();
            v.oscA_fm.reset();
            v.oscB_classic.reset();
            v.oscB_supersaw.reset();
            v.oscB_fm.reset();
            v.subOsc.reset();
            v.shimmer.reset();
            v.tidal.reset();
            v.fracture.reset();
            v.filterA1.reset();
            v.filterA2.reset();
            v.filterB.reset();
            v.ampEnv.reset();
            v.drift.reset();
            v.glideActive = false;
        }
        lfo.reset();
        chorus.reset();
        phaser.reset();
        delay.reset();
        reverb.reset();
        envelopeOutput = 0.0f;
        externalPitchMod = 0.0f;
        externalFilterMod = 0.0f;
        externalMorphMod = 0.0f;
        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // --- ParamSnapshot: read all parameters once per block ---

        // Osc A
        const int oscA_modeIdx     = (pOscA_mode != nullptr) ? static_cast<int> (pOscA_mode->load()) : 0;
        const int oscA_shapeIdx    = (pOscA_shape != nullptr) ? static_cast<int> (pOscA_shape->load()) : 1;
        const float oscA_tune      = (pOscA_tune != nullptr) ? pOscA_tune->load() : 0.0f;
        const float oscA_level     = (pOscA_level != nullptr) ? pOscA_level->load() : 1.0f;
        const float oscA_detune    = (pOscA_detune != nullptr) ? pOscA_detune->load() : 0.3f;
        const float oscA_pw        = (pOscA_pw != nullptr) ? pOscA_pw->load() : 0.5f;
        const float oscA_fmDepth   = (pOscA_fmDepth != nullptr) ? pOscA_fmDepth->load() : 0.3f;

        // Osc B
        const int oscB_modeIdx     = (pOscB_mode != nullptr) ? static_cast<int> (pOscB_mode->load()) : 0;
        const int oscB_shapeIdx    = (pOscB_shape != nullptr) ? static_cast<int> (pOscB_shape->load()) : 1;
        const float oscB_tune      = (pOscB_tune != nullptr) ? pOscB_tune->load() : 0.0f;
        const float oscB_level     = (pOscB_level != nullptr) ? pOscB_level->load() : 0.0f;
        const float oscB_detune    = (pOscB_detune != nullptr) ? pOscB_detune->load() : 0.3f;
        const float oscB_pw        = (pOscB_pw != nullptr) ? pOscB_pw->load() : 0.5f;
        const float oscB_fmDepth   = (pOscB_fmDepth != nullptr) ? pOscB_fmDepth->load() : 0.3f;

        // Sub + Noise
        const float subLevel       = (pSubLevel != nullptr) ? pSubLevel->load() : 0.0f;
        const float noiseLevel     = (pNoiseLevel != nullptr) ? pNoiseLevel->load() : 0.0f;

        // Haze
        const float hazeAmt        = (pHazeAmount != nullptr) ? pHazeAmount->load() : 0.0f;

        // Filter A
        const float filterCut      = (pFilterCutoff != nullptr) ? pFilterCutoff->load() : 8000.0f;
        const float filterRes      = (pFilterReso != nullptr) ? pFilterReso->load() : 0.0f;
        const int filterSlope      = (pFilterSlope != nullptr) ? static_cast<int> (pFilterSlope->load()) : 0;
        const float filterEnv      = (pFilterEnvAmt != nullptr) ? pFilterEnvAmt->load() : 0.0f;

        // Filter B (Formant)
        const float formantMorph   = (pFormantMorph != nullptr) ? pFormantMorph->load() : 0.5f;
        const float formantMix     = (pFormantMix != nullptr) ? pFormantMix->load() : 0.0f;

        // Shimmer (non-const: aftertouch boosts Prism Shimmer depth below)
        float shimmerAmt           = (pShimmerAmount != nullptr) ? pShimmerAmount->load() : 0.0f;
        const float shimmerTone    = (pShimmerTone != nullptr) ? pShimmerTone->load() : 0.5f;

        // Amp Envelope
        const float attack         = (pAttack != nullptr) ? pAttack->load() : 0.1f;
        const float decay          = (pDecay != nullptr) ? pDecay->load() : 0.5f;
        const float sustain        = (pSustain != nullptr) ? pSustain->load() : 0.7f;
        const float release        = (pRelease != nullptr) ? pRelease->load() : 0.5f;

        // LFO
        const float lfoRate        = (pLfoRate != nullptr) ? pLfoRate->load() : 1.5f;
        const float lfoDepth       = (pLfoDepth != nullptr) ? pLfoDepth->load() : 0.0f;
        const int lfoDest          = (pLfoDest != nullptr) ? static_cast<int> (pLfoDest->load()) : 0;

        // Drift
        const float driftDepth     = (pDriftDepth != nullptr) ? pDriftDepth->load() : 0.3f;
        const float driftRate      = (pDriftRate != nullptr) ? pDriftRate->load() : 0.2f;

        // Tidal Pulse (BREATHE macro engine — Option B)
        const float tidalDepth     = (pTidalDepth != nullptr) ? pTidalDepth->load() : 0.0f;
        const float tidalRate      = (pTidalRate != nullptr) ? pTidalRate->load() : 0.15f;

        // Fracture glitch (FRACTURE macro engine — Option B)
        const bool fractureEnable      = (pFractureEnable != nullptr) ? (pFractureEnable->load() >= 0.5f) : false;
        const float fractureIntensity  = (pFractureIntensity != nullptr) ? pFractureIntensity->load() : 0.0f;
        const float fractureRate       = (pFractureRate != nullptr) ? pFractureRate->load() : 0.5f;

        // Reverb (engine-level spatial tail — Option B)
        const float reverbMix      = (pReverbMix != nullptr) ? pReverbMix->load() : 0.0f;
        const float reverbSize     = (pReverbSize != nullptr) ? pReverbSize->load() : 0.5f;

        // Level
        const float level          = (pLevel != nullptr) ? pLevel->load() : 0.8f;

        // Voice
        const int voiceMode        = (pVoiceMode != nullptr) ? static_cast<int> (pVoiceMode->load()) : 0;
        const float glideAmt       = (pGlide != nullptr) ? pGlide->load() : 0.0f;
        const int maxPoly          = (pPolyphony != nullptr)
            ? (1 << std::min (3, static_cast<int> (pPolyphony->load()))) : 8;

        // Precompute glide coefficient (block-constant)
        float glideCoeff = 0.0f;
        if (glideAmt > 0.0f)
            glideCoeff = 1.0f - fastExp (-1.0f / (srf * (0.005f + glideAmt * 0.495f)));

        // Map classic waveforms
        PolyBLEP::Waveform waveA = mapWaveform (oscA_shapeIdx);
        PolyBLEP::Waveform waveB = mapWaveform (oscB_shapeIdx);

        // FM ratio: shape index maps to harmonic ratio (1.0, 2.0, 3.0, 4.0)
        float fmRatioA = static_cast<float> (oscA_shapeIdx + 1);
        float fmRatioB = static_cast<float> (oscB_shapeIdx + 1);

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(),
                        oscA_tune, oscB_tune, glideAmt, voiceMode, maxPoly);
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                allVoicesOff();
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = static_cast<float> (msg.getControllerValue()) / 127.0f;
            // D006: channel pressure → aftertouch (applied to Prism Shimmer below)
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure (msg.getChannelPressureValue() / 127.0f);
        }

        // D006: smooth aftertouch pressure and compute modulation value
        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);

        // Consume coupling accumulators
        float pitchMod = externalPitchMod;
        externalPitchMod = 0.0f;
        float filterMod = externalFilterMod;
        externalFilterMod = 0.0f;
        float morphMod = externalMorphMod;
        externalMorphMod = 0.0f;

        // D006: aftertouch pushes Prism Shimmer deeper — sensitivity 0.35
        // Full pressure adds up to +0.35 shimmer (the JOURNEY macro analog: more shimmer = more Alien)
        shimmerAmt = clamp (shimmerAmt + atPressure * 0.35f, 0.0f, 1.0f);

        // Setup LFO
        lfo.setRate (lfoRate);
        bool hasLfo = lfoDepth > 0.001f;

        // Cache per-voice block-start base frequencies — midiToFreqTune calls std::pow,
        // but note number + tune offsets are block-constant, so compute once.
        for (auto& voice : voices)
        {
            if (!voice.active) continue;
            voice.cachedBaseFreqA = midiToFreqTune (voice.noteNumber, oscA_tune);
            voice.cachedBaseFreqB = midiToFreqTune (voice.noteNumber, oscB_tune);
        }

        float peakEnv = 0.0f;

        // --- Render voices sample by sample ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Per-sample LFO
            float lfoVal = 0.0f;
            float lfoPitchMod = 0.0f;
            float lfoCutoffMod = 0.0f;
            float lfoAmpMod = 0.0f;

            if (hasLfo)
            {
                lfoVal = lfo.process() * lfoDepth;
                switch (lfoDest)
                {
                    case 0: lfoPitchMod = lfoVal; break;
                    case 1: lfoCutoffMod = lfoVal; break;
                    case 2: lfoAmpMod = lfoVal; break;
                }
            }

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;
                ++voice.age;

                // Update envelope params (cached per-block, not per-sample)
                if (sample == 0)
                    voice.ampEnv.setParams (attack, decay, sustain, release);

                // --- Glide ---
                float baseFreqA = voice.cachedBaseFreqA;
                float baseFreqB = voice.cachedBaseFreqB;

                if (voice.glideActive)
                {
                    voice.glideSourceFreq += glideCoeff * (baseFreqA - voice.glideSourceFreq);

                    if (std::abs (voice.glideSourceFreq - baseFreqA) < 0.1f)
                    {
                        voice.glideSourceFreq = baseFreqA;
                        voice.glideActive = false;
                    }
                    float glideRatio = voice.glideSourceFreq / std::max (baseFreqA, 0.01f);
                    baseFreqA = voice.glideSourceFreq;
                    baseFreqB *= glideRatio;
                }

                // --- Voyager Drift ---
                float driftVal = voice.drift.process (driftRate, driftDepth);
                float driftSemitones = driftVal * 0.5f;  // ±0.5 semitones max

                // Total pitch modulation: drift + LFO + coupling
                float totalPitchSemi = driftSemitones + lfoPitchMod * 2.0f + pitchMod;
                float pitchMul = fastExp (totalPitchSemi * (0.693147f / 12.0f));

                float freqA = baseFreqA * pitchMul;
                float freqB = baseFreqB * pitchMul;

                // --- Generate Osc A ---
                float oscAOut = 0.0f;
                switch (oscA_modeIdx)
                {
                    case 0:  // Classic
                        voice.oscA_classic.setFrequency (freqA, srf);
                        voice.oscA_classic.setWaveform (waveA);
                        voice.oscA_classic.setPulseWidth (oscA_pw);
                        oscAOut = voice.oscA_classic.processSample();
                        break;
                    case 1:  // Supersaw
                        voice.oscA_supersaw.setFrequency (freqA, oscA_detune);
                        oscAOut = voice.oscA_supersaw.processSample();
                        break;
                    case 2:  // FM
                        voice.oscA_fm.setFrequency (freqA);
                        oscAOut = voice.oscA_fm.processSample (fmRatioA, oscA_fmDepth);
                        break;
                }
                oscAOut *= oscA_level;

                // --- Generate Osc B ---
                float oscBOut = 0.0f;
                if (oscB_level > 0.001f)
                {
                    switch (oscB_modeIdx)
                    {
                        case 0:  // Classic
                            voice.oscB_classic.setFrequency (freqB, srf);
                            voice.oscB_classic.setWaveform (waveB);
                            voice.oscB_classic.setPulseWidth (oscB_pw);
                            oscBOut = voice.oscB_classic.processSample();
                            break;
                        case 1:  // Supersaw
                            voice.oscB_supersaw.setFrequency (freqB, oscB_detune);
                            oscBOut = voice.oscB_supersaw.processSample();
                            break;
                        case 2:  // FM
                            voice.oscB_fm.setFrequency (freqB);
                            oscBOut = voice.oscB_fm.processSample (fmRatioB, oscB_fmDepth);
                            break;
                    }
                    oscBOut *= oscB_level;
                }

                // --- Sub + Noise ---
                voice.subOsc.setFrequency (freqA * 0.5f, srf);
                voice.subOsc.setWaveform (PolyBLEP::Waveform::Sine);
                float subOut = voice.subOsc.processSample() * subLevel;

                float noiseOut = voice.noise.process() * noiseLevel;

                // --- Mix sources ---
                float raw = oscAOut + oscBOut + subOut + noiseOut;

                // --- Haze Saturation (pre-filter) ---
                raw = voice.haze.process (raw, hazeAmt);

                // --- Filter A (LP 12/24 dB) ---
                float envVal = voice.ampEnv.process();

                float cutoffMod = filterCut;

                // Envelope to filter
                if (std::abs (filterEnv) > 0.001f)
                {
                    float envOffset = filterEnv * envVal * voice.velocity * 10000.0f;
                    cutoffMod = clamp (cutoffMod + envOffset, 20.0f, 20000.0f);
                }
                // LFO to filter
                if (std::abs (lfoCutoffMod) > 0.001f)
                {
                    cutoffMod *= fastExp (lfoCutoffMod * 2.0f * 0.693147f);
                    cutoffMod = clamp (cutoffMod, 20.0f, 20000.0f);
                }
                // Voyager Drift to filter (subtle)
                cutoffMod *= fastExp (driftVal * 0.1f * 0.693147f);
                cutoffMod = clamp (cutoffMod, 20.0f, 20000.0f);
                // External coupling filter modulation
                cutoffMod += filterMod * 2000.0f;
                // CC1 modwheel → open filter (0→no change, 1→+4000Hz)
                cutoffMod += modWheelAmount * 4000.0f;
                cutoffMod = clamp (cutoffMod, 20.0f, 20000.0f);

                voice.filterA1.setMode (CytomicSVF::Mode::LowPass);
                voice.filterA1.setCoefficients_fast (cutoffMod, filterRes, srf);
                float filtered = voice.filterA1.processSample (raw);

                // 24dB cascade: run through second SVF
                if (filterSlope == 1)
                {
                    voice.filterA2.setMode (CytomicSVF::Mode::LowPass);
                    voice.filterA2.setCoefficients_fast (cutoffMod, filterRes, srf);
                    filtered = voice.filterA2.processSample (filtered);
                }

                // --- Filter B (Formant) ---
                float effectiveMorph = clamp (formantMorph + morphMod, 0.0f, 1.0f);
                voice.filterB.updateCoefficients (effectiveMorph, 0.5f, srf);
                filtered = voice.filterB.process (filtered, formantMix);

                // --- Fracture glitch (post-filter, pre-envelope — FRACTURE macro) ---
                // Applied in mono before envelope so the stutter texture is shaped by
                // the amp envelope's natural decay — glitch grains feel musically timed.
                if (fractureEnable && fractureIntensity > 0.0001f)
                    filtered = voice.fracture.process (filtered, true, fractureIntensity, fractureRate);

                // --- Prism Shimmer (post-filter, post-Fracture) ---
                filtered = voice.shimmer.process (filtered, shimmerAmt, shimmerTone);

                // --- Apply envelope and velocity ---
                float out = filtered * envVal * voice.velocity;

                // LFO amp modulation
                if (std::abs (lfoAmpMod) > 0.001f)
                {
                    float ampMod = 1.0f + lfoAmpMod * 0.5f;
                    out *= std::max (0.0f, ampMod);
                }

                // --- TidalPulse amplitude modulation (BREATHE macro) ---
                // sin² breathing shape modulates output amplitude by up to (1 - tidalDepth).
                // At tidalDepth=0.0 → no modulation. At tidalDepth=1.0 → output reaches 0
                // at the trough of the breath cycle (full swell/recede). Applied post-envelope
                // so the breathing sits on top of the note's natural ADSR shape.
                if (tidalDepth > 0.0001f)
                {
                    float breathVal = voice.tidal.process (tidalRate, tidalDepth);
                    // breathVal in [0, tidalDepth]. 1.0 - breathVal scales amplitude: swell at 0,
                    // recede at peak. Inverted: sound is loudest during trough → unipolar "inhale".
                    // Use 1.0 - breathVal so amplitude = 1.0 at breathVal=0 (silence = exhale).
                    out *= (1.0f - breathVal);
                }

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    voice.age = 0;
                }

                // Mono voice sum → stereo (slight stereo spread from drift)
                float panOffset = driftVal * 0.15f;
                float panL = 0.5f - panOffset;
                float panR = 0.5f + panOffset;
                mixL += out * panL * 2.0f;
                mixR += out * panR * 2.0f;

                peakEnv = std::max (peakEnv, envVal);
            }

            // Apply engine level
            float outL = mixL * level;
            float outR = mixR * level;

            // Soft limiter
            outL = fastTanh (outL);
            outR = fastTanh (outR);

            // Store into output cache — coupling reads dry (pre-reverb) signal.
            // Reverb is applied in a block-level pass below before writing to buffer.
            outputCacheL[static_cast<size_t> (sample)] = outL;
            outputCacheR[static_cast<size_t> (sample)] = outR;
        }

        // --- FX chain (block-level, in-place on outputCache) ---
        // Order: Chorus → Phaser → Delay → Reverb (each bypassed if mix ≈ 0).
        // All FX operate on outputCacheL/R which were pre-allocated in prepare().
        const float chorusMix      = (pChorusMix      != nullptr) ? pChorusMix->load()      : 0.0f;
        const float chorusRate     = (pChorusRate     != nullptr) ? pChorusRate->load()     : 0.5f;
        const float chorusDepth    = (pChorusDepth    != nullptr) ? pChorusDepth->load()    : 0.5f;
        const float phaserMix      = (pPhaserMix      != nullptr) ? pPhaserMix->load()      : 0.0f;
        const float phaserRate     = (pPhaserRate     != nullptr) ? pPhaserRate->load()     : 0.3f;
        const float phaserDepth    = (pPhaserDepth    != nullptr) ? pPhaserDepth->load()    : 0.7f;
        const float phaserFeedback = (pPhaserFeedback != nullptr) ? pPhaserFeedback->load() : 0.5f;
        const float delayMix       = (pDelayMix       != nullptr) ? pDelayMix->load()       : 0.0f;
        const float delayTime      = (pDelayTime      != nullptr) ? pDelayTime->load()      : 0.375f;
        const float delayFeedback  = (pDelayFeedback  != nullptr) ? pDelayFeedback->load()  : 0.4f;

        if (chorusMix > 0.0001f)
            chorus.processBlock (outputCacheL.data(), outputCacheR.data(),
                                 numSamples, chorusRate, chorusDepth, chorusMix);

        if (phaserMix > 0.0001f)
            phaser.processBlock (outputCacheL.data(), outputCacheR.data(),
                                 numSamples, phaserRate, phaserDepth, phaserFeedback, phaserMix);

        if (delayMix > 0.0001f)
            delay.processBlock (outputCacheL.data(), outputCacheR.data(),
                                numSamples, delayTime, delayFeedback, delayMix);

        // --- Reverb (engine-level, block-pass — BREATHE / spatial Option B) ---
        // Applied to the full block after the per-sample voice loop. The reverb
        // processStereo() call is in-place on the outputCache arrays, which were
        // pre-allocated in prepare(). No heap allocation occurs here.
        if (reverbMix > 0.0001f)
            reverb.processStereo (outputCacheL.data(), outputCacheR.data(),
                                  numSamples, reverbSize, reverbMix);

        // Write output cache to shared mix buffer
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float outL = outputCacheL[static_cast<size_t> (sample)];
            float outR = outputCacheR[static_cast<size_t> (sample)];

            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sample, outL);
                buffer.addSample (1, sample, outR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sample, (outL + outR) * 0.5f);
            }
        }

        envelopeOutput = peakEnv;
    }

    //-- Coupling --------------------------------------------------------------

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto si = static_cast<size_t> (sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];
        if (channel == 2) return envelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
                externalFilterMod += amount;
                break;
            case CouplingType::AmpToPitch:
            case CouplingType::LFOToPitch:
            case CouplingType::PitchToPitch:
                externalPitchMod += amount * 0.5f;
                break;
            case CouplingType::EnvToMorph:
                externalMorphMod += amount;
                break;
            default:
                break;
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

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // --- Oscillator A ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "drift_oscA_mode", 1 }, "Drift Osc A Mode",
            juce::StringArray { "Classic", "Supersaw", "FM" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "drift_oscA_shape", 1 }, "Drift Osc A Shape",
            juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 1));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_oscA_tune", 1 }, "Drift Osc A Tune",
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_oscA_level", 1 }, "Drift Osc A Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_oscA_detune", 1 }, "Drift Osc A Detune",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_oscA_pw", 1 }, "Drift Osc A PW",
            juce::NormalisableRange<float> (0.01f, 0.99f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_oscA_fmDepth", 1 }, "Drift Osc A FM Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        // --- Oscillator B ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "drift_oscB_mode", 1 }, "Drift Osc B Mode",
            juce::StringArray { "Classic", "Supersaw", "FM" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "drift_oscB_shape", 1 }, "Drift Osc B Shape",
            juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 1));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_oscB_tune", 1 }, "Drift Osc B Tune",
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_oscB_level", 1 }, "Drift Osc B Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_oscB_detune", 1 }, "Drift Osc B Detune",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_oscB_pw", 1 }, "Drift Osc B PW",
            juce::NormalisableRange<float> (0.01f, 0.99f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_oscB_fmDepth", 1 }, "Drift Osc B FM Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        // --- Sub + Noise ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_subLevel", 1 }, "Drift Sub Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_noiseLevel", 1 }, "Drift Noise Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Haze Saturation ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_hazeAmount", 1 }, "Drift Haze Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Filter A ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_filterCutoff", 1 }, "Drift Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f), 8000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_filterReso", 1 }, "Drift Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "drift_filterSlope", 1 }, "Drift Filter Slope",
            juce::StringArray { "12 dB", "24 dB" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_filterEnvAmt", 1 }, "Drift Filter Env Amt",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));

        // --- Filter B (Formant) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_formantMorph", 1 }, "Drift Formant Morph",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_formantMix", 1 }, "Drift Formant Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Prism Shimmer ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_shimmerAmount", 1 }, "Drift Shimmer Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_shimmerTone", 1 }, "Drift Shimmer Tone",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        // --- Amp Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_attack", 1 }, "Drift Attack",
            juce::NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.3f), 0.1f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_decay", 1 }, "Drift Decay",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_sustain", 1 }, "Drift Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_release", 1 }, "Drift Release",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.5f));

        // --- LFO ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_lfoRate", 1 }, "Drift LFO Rate",
            juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f, 0.3f), 1.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_lfoDepth", 1 }, "Drift LFO Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "drift_lfoDest", 1 }, "Drift LFO Dest",
            juce::StringArray { "Pitch", "Filter", "Amp" }, 0));

        // --- Voyager Drift ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_driftDepth", 1 }, "Drift Drift Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_driftRate", 1 }, "Drift Drift Rate",
            juce::NormalisableRange<float> (0.05f, 2.0f, 0.01f), 0.2f));

        // --- Tidal Pulse — BREATHE macro engine (Option B, Round 11B) ---
        // sin² breathing LFO applied as amplitude modulation. Default off (depth=0).
        // Rate range matches one full breath per 20s (0.05 Hz) to 2 breaths/second (2.0 Hz).
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_tidalDepth", 1 }, "Drift Tidal Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_tidalRate", 1 }, "Drift Tidal Rate",
            juce::NormalisableRange<float> (0.05f, 2.0f, 0.01f, 0.5f), 0.15f));

        // --- Fracture — FRACTURE macro engine (Option B, Round 11B) ---
        // Circular-buffer stutter glitch. Default off (enable=false, intensity=0).
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "drift_fractureEnable", 1 }, "Drift Fracture Enable",
            false));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_fractureIntensity", 1 }, "Drift Fracture Intensity",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_fractureRate", 1 }, "Drift Fracture Rate",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        // --- Chorus — BDD stereo chorus. Rate, Depth, Mix. Default off. ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_chorusMix", 1 }, "Drift Chorus Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_chorusRate", 1 }, "Drift Chorus Rate",
            juce::NormalisableRange<float> (0.1f, 5.0f, 0.01f, 0.4f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_chorusDepth", 1 }, "Drift Chorus Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        // --- Phaser — 4-stage APF phaser. Rate, Depth, Feedback, Mix. Default off. ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_phaserMix", 1 }, "Drift Phaser Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_phaserRate", 1 }, "Drift Phaser Rate",
            juce::NormalisableRange<float> (0.01f, 4.0f, 0.01f, 0.4f), 0.3f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_phaserDepth", 1 }, "Drift Phaser Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_phaserFeedback", 1 }, "Drift Phaser Feedback",
            juce::NormalisableRange<float> (-0.9f, 0.9f, 0.01f), 0.5f));

        // --- Delay — tape delay with LP feedback. Time, Feedback, Mix. Default off. ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_delayMix", 1 }, "Drift Delay Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_delayTime", 1 }, "Drift Delay Time",
            juce::NormalisableRange<float> (0.01f, 1.0f, 0.001f, 0.4f), 0.375f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_delayFeedback", 1 }, "Drift Delay Feedback",
            juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.4f));

        // --- Reverb — engine-level spatial tail (Option B, Round 11B) ---
        // Schroeder 4-comb + 2-allpass reverb. Default off (mix=0).
        // Size controls room scale (feedback 0.50–0.95). Mix is wet/dry.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_reverbMix", 1 }, "Drift Reverb Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_reverbSize", 1 }, "Drift Reverb Size",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        // --- Level ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_level", 1 }, "Drift Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // --- Voice ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "drift_voiceMode", 1 }, "Drift Voice Mode",
            juce::StringArray { "Poly", "Mono", "Legato" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "drift_glide", 1 }, "Drift Glide",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "drift_polyphony", 1 }, "Drift Polyphony",
            juce::StringArray { "1", "2", "4", "8" }, 3));
    }

public:
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pOscA_mode     = apvts.getRawParameterValue ("drift_oscA_mode");
        pOscA_shape    = apvts.getRawParameterValue ("drift_oscA_shape");
        pOscA_tune     = apvts.getRawParameterValue ("drift_oscA_tune");
        pOscA_level    = apvts.getRawParameterValue ("drift_oscA_level");
        pOscA_detune   = apvts.getRawParameterValue ("drift_oscA_detune");
        pOscA_pw       = apvts.getRawParameterValue ("drift_oscA_pw");
        pOscA_fmDepth  = apvts.getRawParameterValue ("drift_oscA_fmDepth");
        pOscB_mode     = apvts.getRawParameterValue ("drift_oscB_mode");
        pOscB_shape    = apvts.getRawParameterValue ("drift_oscB_shape");
        pOscB_tune     = apvts.getRawParameterValue ("drift_oscB_tune");
        pOscB_level    = apvts.getRawParameterValue ("drift_oscB_level");
        pOscB_detune   = apvts.getRawParameterValue ("drift_oscB_detune");
        pOscB_pw       = apvts.getRawParameterValue ("drift_oscB_pw");
        pOscB_fmDepth  = apvts.getRawParameterValue ("drift_oscB_fmDepth");
        pSubLevel      = apvts.getRawParameterValue ("drift_subLevel");
        pNoiseLevel    = apvts.getRawParameterValue ("drift_noiseLevel");
        pHazeAmount    = apvts.getRawParameterValue ("drift_hazeAmount");
        pFilterCutoff  = apvts.getRawParameterValue ("drift_filterCutoff");
        pFilterReso    = apvts.getRawParameterValue ("drift_filterReso");
        pFilterSlope   = apvts.getRawParameterValue ("drift_filterSlope");
        pFilterEnvAmt  = apvts.getRawParameterValue ("drift_filterEnvAmt");
        pFormantMorph  = apvts.getRawParameterValue ("drift_formantMorph");
        pFormantMix    = apvts.getRawParameterValue ("drift_formantMix");
        pShimmerAmount = apvts.getRawParameterValue ("drift_shimmerAmount");
        pShimmerTone   = apvts.getRawParameterValue ("drift_shimmerTone");
        pAttack        = apvts.getRawParameterValue ("drift_attack");
        pDecay         = apvts.getRawParameterValue ("drift_decay");
        pSustain       = apvts.getRawParameterValue ("drift_sustain");
        pRelease       = apvts.getRawParameterValue ("drift_release");
        pLfoRate       = apvts.getRawParameterValue ("drift_lfoRate");
        pLfoDepth      = apvts.getRawParameterValue ("drift_lfoDepth");
        pLfoDest       = apvts.getRawParameterValue ("drift_lfoDest");
        pDriftDepth        = apvts.getRawParameterValue ("drift_driftDepth");
        pDriftRate         = apvts.getRawParameterValue ("drift_driftRate");
        pTidalDepth        = apvts.getRawParameterValue ("drift_tidalDepth");
        pTidalRate         = apvts.getRawParameterValue ("drift_tidalRate");
        pFractureEnable    = apvts.getRawParameterValue ("drift_fractureEnable");
        pFractureIntensity = apvts.getRawParameterValue ("drift_fractureIntensity");
        pFractureRate      = apvts.getRawParameterValue ("drift_fractureRate");
        pChorusMix      = apvts.getRawParameterValue ("drift_chorusMix");
        pChorusRate     = apvts.getRawParameterValue ("drift_chorusRate");
        pChorusDepth    = apvts.getRawParameterValue ("drift_chorusDepth");
        pPhaserMix      = apvts.getRawParameterValue ("drift_phaserMix");
        pPhaserRate     = apvts.getRawParameterValue ("drift_phaserRate");
        pPhaserDepth    = apvts.getRawParameterValue ("drift_phaserDepth");
        pPhaserFeedback = apvts.getRawParameterValue ("drift_phaserFeedback");
        pDelayMix       = apvts.getRawParameterValue ("drift_delayMix");
        pDelayTime      = apvts.getRawParameterValue ("drift_delayTime");
        pDelayFeedback  = apvts.getRawParameterValue ("drift_delayFeedback");
        pReverbMix         = apvts.getRawParameterValue ("drift_reverbMix");
        pReverbSize        = apvts.getRawParameterValue ("drift_reverbSize");
        pLevel             = apvts.getRawParameterValue ("drift_level");
        pVoiceMode     = apvts.getRawParameterValue ("drift_voiceMode");
        pGlide         = apvts.getRawParameterValue ("drift_glide");
        pPolyphony     = apvts.getRawParameterValue ("drift_polyphony");
    }

    //-- Identity --------------------------------------------------------------

    juce::String getEngineId() const override { return "Odyssey"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF7B2D8B); }
    int getMaxVoices() const override { return kMaxVoices; }

private:
    //--------------------------------------------------------------------------
    void noteOn (int noteNumber, float velocity,
                 float oscA_tune, float oscB_tune,
                 float glideAmt, int voiceMode, int maxPoly)
    {
        // Mono/legato modes
        if (voiceMode == 1 || voiceMode == 2)
        {
            for (int i = 1; i < kMaxVoices; ++i)
                voices[static_cast<size_t> (i)].active = false;

            auto& v = voices[0];
            bool legatoRetrigger = !(voiceMode == 2 && v.active);

            if (v.active && glideAmt > 0.001f)
            {
                v.glideSourceFreq = midiToFreqTune (v.noteNumber, oscA_tune);
                v.glideActive = true;
            }
            else
            {
                v.glideActive = false;
            }

            v.active = true;
            v.noteNumber = noteNumber;
            v.velocity = velocity;
            v.age = 0;

            if (legatoRetrigger)
            {
                v.ampEnv.noteOn();
                resetOscillators (v);
            }
            return;
        }

        // Poly mode
        int idx = findFreeVoice (maxPoly);
        auto& v = voices[static_cast<size_t> (idx)];

        if (v.active && glideAmt > 0.001f)
        {
            v.glideSourceFreq = midiToFreqTune (v.noteNumber, oscA_tune);
            v.glideActive = true;
        }
        else
        {
            v.glideActive = false;
        }

        v.active = true;
        v.noteNumber = noteNumber;
        v.velocity = velocity;
        v.age = 0;
        v.ampEnv.noteOn();
        resetOscillators (v);
        v.filterA1.reset();
        v.filterA2.reset();
        v.filterB.reset();
        v.shimmer.reset();
        v.fracture.reset();  // clear any stutter buffer from previous note
        v.tidal.reset();     // restart breath phase on new note
    }

    void noteOff (int noteNumber)
    {
        for (auto& v : voices)
            if (v.active && v.noteNumber == noteNumber)
                v.ampEnv.noteOff();
    }

    void allVoicesOff()
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.ampEnv.reset();
            v.glideActive = false;
        }
        envelopeOutput = 0.0f;
        externalPitchMod = 0.0f;
        externalFilterMod = 0.0f;
        externalMorphMod = 0.0f;
    }

    void resetOscillators (DriftVoice& v)
    {
        v.oscA_classic.reset();
        v.oscA_supersaw.reset();
        v.oscA_fm.reset();
        v.oscB_classic.reset();
        v.oscB_supersaw.reset();
        v.oscB_fm.reset();
        v.subOsc.reset();
    }

    int findFreeVoice (int maxPoly)
    {
        int poly = std::min (maxPoly, kMaxVoices);

        for (int i = 0; i < poly; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        // Oldest voice stealing — initialize from voice 0
        int oldest = 0;
        uint64_t oldestAge = voices[0].age;
        for (int i = 1; i < poly; ++i)
        {
            if (voices[static_cast<size_t> (i)].age > oldestAge)
            {
                oldestAge = voices[static_cast<size_t> (i)].age;
                oldest = i;
            }
        }
        voices[static_cast<size_t> (oldest)].ampEnv.reset();
        return oldest;
    }

    static float midiToFreqTune (int midiNote, float tuneSemitones) noexcept
    {
        float n = static_cast<float> (midiNote) + tuneSemitones;
        return 440.0f * fastPow2 ((n - 69.0f) * (1.0f / 12.0f));
    }

    static PolyBLEP::Waveform mapWaveform (int shapeIdx) noexcept
    {
        switch (shapeIdx)
        {
            case 0: return PolyBLEP::Waveform::Sine;
            case 1: return PolyBLEP::Waveform::Saw;
            case 2: return PolyBLEP::Waveform::Square;
            case 3: return PolyBLEP::Waveform::Triangle;
            default: return PolyBLEP::Waveform::Saw;
        }
    }

    //--------------------------------------------------------------------------
    double sr = 44100.0;
    float srf = 44100.0f;
    std::array<DriftVoice, kMaxVoices> voices;

    // LFO (global, not per-voice — XOdyssey LFO1 is global)
    DriftLFO lfo;

    // FX chain — engine-level, block-pass. Chorus → Phaser → Delay → Reverb.
    // All buffers pre-allocated in prepare(); safe on audio thread.
    DriftChorus chorus;
    DriftPhaser phaser;
    DriftDelay  delay;
    DriftReverb reverb;

    // Coupling state
    float envelopeOutput = 0.0f;
    float externalPitchMod = 0.0f;
    float externalFilterMod = 0.0f;
    float externalMorphMod = 0.0f;
    float modWheelAmount = 0.0f; // CC1 [0,1]

    // ---- D006 Aftertouch — pressure deepens Prism Shimmer (JOURNEY analog) ----
    PolyAftertouch aftertouch;

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS parameter pointers (45 params — 38 original + 7 Option B)
    std::atomic<float>* pOscA_mode = nullptr;
    std::atomic<float>* pOscA_shape = nullptr;
    std::atomic<float>* pOscA_tune = nullptr;
    std::atomic<float>* pOscA_level = nullptr;
    std::atomic<float>* pOscA_detune = nullptr;
    std::atomic<float>* pOscA_pw = nullptr;
    std::atomic<float>* pOscA_fmDepth = nullptr;
    std::atomic<float>* pOscB_mode = nullptr;
    std::atomic<float>* pOscB_shape = nullptr;
    std::atomic<float>* pOscB_tune = nullptr;
    std::atomic<float>* pOscB_level = nullptr;
    std::atomic<float>* pOscB_detune = nullptr;
    std::atomic<float>* pOscB_pw = nullptr;
    std::atomic<float>* pOscB_fmDepth = nullptr;
    std::atomic<float>* pSubLevel = nullptr;
    std::atomic<float>* pNoiseLevel = nullptr;
    std::atomic<float>* pHazeAmount = nullptr;
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterReso = nullptr;
    std::atomic<float>* pFilterSlope = nullptr;
    std::atomic<float>* pFilterEnvAmt = nullptr;
    std::atomic<float>* pFormantMorph = nullptr;
    std::atomic<float>* pFormantMix = nullptr;
    std::atomic<float>* pShimmerAmount = nullptr;
    std::atomic<float>* pShimmerTone = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pLfoRate = nullptr;
    std::atomic<float>* pLfoDepth = nullptr;
    std::atomic<float>* pLfoDest = nullptr;
    std::atomic<float>* pDriftDepth = nullptr;
    std::atomic<float>* pDriftRate = nullptr;
    // Option B — Round 11B (7 new params)
    std::atomic<float>* pTidalDepth = nullptr;
    std::atomic<float>* pTidalRate = nullptr;
    std::atomic<float>* pFractureEnable = nullptr;
    std::atomic<float>* pFractureIntensity = nullptr;
    std::atomic<float>* pFractureRate = nullptr;
    std::atomic<float>* pReverbMix = nullptr;
    std::atomic<float>* pReverbSize = nullptr;
    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide = nullptr;
    std::atomic<float>* pPolyphony = nullptr;
};

} // namespace xomnibus
