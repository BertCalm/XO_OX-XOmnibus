#pragma once
//==============================================================================
//
//  OuieEngine.h — XOuie | "The Hammerhead"
//  XO_OX Designs | XOlokun Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      The Hammerhead shark (Sphyrna) patrols the thermocline — that
//      invisible boundary where warm, bright water meets cold, dark
//      depths. Its cephalofoil (the hammer-shaped head) houses the
//      ampullae of Lorenzini: electromagnetic sensors so sensitive
//      they detect the heartbeat of prey buried in sand.
//
//      In the XO_OX mythology, XOuie sits at perfect 50/50 feliX-Oscar
//      polarity. Neither bright nor dark, neither warm nor cold. Two
//      eyes on opposite ends of the cephalofoil — each seeing a
//      different algorithm, a different waveform, a different world.
//      Between them, the HAMMER: STRIFE or LOVE. Cross-FM and ring
//      modulation tear the voices apart. Spectral blend and harmonic
//      lock draw them together. The thermocline is the battleground.
//
//  ENGINE CONCEPT:
//      Duophonic synthesis — exactly 2 voices, each with a selectable
//      algorithm from a palette of 8. The voices interact through the
//      Interaction Stage (the HAMMER), which sweeps from destructive
//      interference (STRIFE) to constructive coupling (LOVE).
//
//      Voice 1 ("Left Eye") and Voice 2 ("Right Eye") each select from:
//
//      SMOOTH algorithms (feliX side):
//        0. VA         — Virtual analog saw/square/tri with PWM
//        1. Wavetable  — 16 built-in metallic/organic tables
//        2. FM         — 2-operator FM (carrier + modulator)
//        3. Additive   — 8 partials with individual amplitude control
//
//      ROUGH algorithms (Oscar side):
//        4. Phase Dist — CZ-style phase distortion synthesis
//        5. Wavefolder — Triangle source through multi-stage folding
//        6. KS         — Karplus-Strong plucked string excitation
//        7. Noise      — Filtered noise with pitch tracking
//
//      Signal flow per voice:
//        Algorithm -> [Interaction Stage] -> SVF Filter -> Amp Env -> Pan
//
//      Voice modes:
//        - Split:  Voice 1 plays below split point, Voice 2 above
//        - Layer:  Both voices sound on every note
//        - Duo:    Voice 1 = most recent note, Voice 2 = previous note
//
//  4 MACROS:
//      HAMMER    — STRIFE<->LOVE interaction axis (bipolar -1 to +1)
//      AMPULLAE  — Sensitivity (velocity/aftertouch/expression depth)
//      CARTILAGE — Flexibility (filter resonance + envelope speed)
//      CURRENT   — Environment (chorus depth + delay + reverb)
//
//  COUPLING:
//      Output:  L/R audio, envelope level
//      Input:   AmpToFilter, LFOToPitch, AudioToFM, AudioToRing
//
//  ACCENT COLOR: Hammerhead Steel #708090
//  PARAMETER PREFIX: ouie_
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <vector>

namespace xolokun {

//==============================================================================
//
//  I. PRIMITIVES
//
//==============================================================================

// OuieADSR replaced by shared StandardADSR (Source/DSP/StandardADSR.h)

// OuieLFO replaced by shared StandardLFO (Source/DSP/StandardLFO.h)

// OuieBreathingLFO replaced by shared BreathingLFO (Source/DSP/StandardLFO.h)

//==============================================================================
// Noise generator — xorshift32 PRNG.
//==============================================================================
struct OuieNoiseGen
{
    uint32_t state = 1;

    void seed (uint32_t s) noexcept { state = s ? s : 1; }

    float process() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float> (static_cast<int32_t> (state)) / 2147483648.0f;
    }
};

//==============================================================================
//
//  II. OSCILLATOR ALGORITHMS — The 8 selectable synthesis methods.
//
//  Each algorithm is a minimal struct with process(freq, sr) -> sample.
//  The engine selects one per voice; both run simultaneously in the
//  Interaction Stage where STRIFE and LOVE merge or clash.
//
//==============================================================================

//==============================================================================
// Algorithm 0: VA — Virtual Analog (saw/square/tri with PWM)
// Band-limited polyBLEP for anti-aliased waveforms.
//==============================================================================
struct OuieVA
{
    float phase = 0.0f;
    float prevSample = 0.0f;

    // PolyBLEP residual for band-limited discontinuities
    static float polyBLEP (float t, float dt) noexcept
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    // waveform: 0=saw, 1=square, 2=triangle
    // pw: pulse width [0.1 .. 0.9] for square
    float process (float freq, float sr, int waveform, float pw) noexcept
    {
        float dt = freq / sr;
        if (dt > 0.49f) dt = 0.49f;

        float out = 0.0f;

        switch (waveform)
        {
            case 0: // Saw
            {
                out = 2.0f * phase - 1.0f;
                out -= polyBLEP (phase, dt);
                break;
            }
            case 1: // Square (with PWM)
            {
                float pwClamped = clamp (pw, 0.1f, 0.9f);
                out = (phase < pwClamped) ? 1.0f : -1.0f;
                out += polyBLEP (phase, dt);
                // Second edge at PW boundary
                float phase2 = phase - pwClamped;
                if (phase2 < 0.0f) phase2 += 1.0f;
                out -= polyBLEP (phase2, dt);
                break;
            }
            case 2: // Triangle (integrated square, leaky)
            {
                float sq = (phase < 0.5f) ? 1.0f : -1.0f;
                sq += polyBLEP (phase, dt);
                float phase2 = phase - 0.5f;
                if (phase2 < 0.0f) phase2 += 1.0f;
                sq -= polyBLEP (phase2, dt);
                // Leaky integrator to form triangle from square
                prevSample = prevSample * 0.999f + sq * dt * 4.0f;
                out = prevSample;
                break;
            }
        }

        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;

        return out;
    }

    void reset() noexcept { phase = 0.0f; prevSample = 0.0f; }
};

//==============================================================================
// Algorithm 1: Wavetable — 16 built-in tables with morphing.
// Tables are procedurally generated on prepare().
//==============================================================================
struct OuieWavetable
{
    static constexpr int kNumTables = 16;
    static constexpr int kTableSize = 256;
    float tables[kNumTables][kTableSize] {};
    float phase = 0.0f;

    void build() noexcept
    {
        constexpr float twoPi = 6.28318530718f;
        for (int t = 0; t < kNumTables; ++t)
        {
            float morph = static_cast<float> (t) / static_cast<float> (kNumTables - 1);
            for (int s = 0; s < kTableSize; ++s)
            {
                float p = static_cast<float> (s) / static_cast<float> (kTableSize);
                float sample = 0.0f;
                int numPartials = 1 + static_cast<int> (morph * 12.0f);
                float totalAmp = 0.0f;

                for (int h = 1; h <= numPartials; ++h)
                {
                    float n = static_cast<float> (h);
                    // Metallic stretching for higher tables
                    float ratio = n * (1.0f + morph * 0.02f * n);
                    float amp = 1.0f / (n * 0.7f + 0.3f);
                    // Odd-harmonic emphasis in mid tables
                    if (t >= 4 && t <= 10 && h % 2 == 1)
                        amp *= 1.5f;
                    sample += amp * std::sin (twoPi * ratio * p);
                    totalAmp += amp;
                }
                if (totalAmp > 0.0f)
                    sample /= totalAmp;

                tables[t][s] = sample;
            }
        }
    }

    // position: 0..1 morphs across the 16 tables
    float process (float freq, float sr, float position) noexcept
    {
        float dt = freq / sr;
        if (dt > 0.49f) dt = 0.49f;

        // Determine which two tables to crossfade
        float tablePos = clamp (position, 0.0f, 1.0f) * static_cast<float> (kNumTables - 1);
        int t0 = static_cast<int> (tablePos);
        int t1 = std::min (t0 + 1, kNumTables - 1);
        float tFrac = tablePos - static_cast<float> (t0);

        // Linear interpolation within each table
        float readPos = phase * static_cast<float> (kTableSize);
        int i0 = static_cast<int> (readPos) % kTableSize;
        int i1 = (i0 + 1) % kTableSize;
        float iFrac = readPos - std::floor (readPos);

        float s0 = tables[t0][i0] + iFrac * (tables[t0][i1] - tables[t0][i0]);
        float s1 = tables[t1][i0] + iFrac * (tables[t1][i1] - tables[t1][i0]);

        float out = s0 + tFrac * (s1 - s0);

        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;

        return out;
    }

    void reset() noexcept { phase = 0.0f; }
};

//==============================================================================
// Algorithm 2: FM — 2-operator FM synthesis (carrier + modulator).
//==============================================================================
struct OuieFM
{
    float carrierPhase = 0.0f;
    float modPhase = 0.0f;

    // ratio: modulator/carrier frequency ratio
    // index: FM modulation index
    float process (float freq, float sr, float ratio, float index) noexcept
    {
        float dt = freq / sr;
        if (dt > 0.49f) dt = 0.49f;

        // Modulator
        float modFreq = dt * ratio;
        float modOut = fastSin (modPhase * 6.28318530718f) * index;

        // Carrier with FM
        float out = fastSin ((carrierPhase + modOut) * 6.28318530718f);

        carrierPhase += dt;
        if (carrierPhase >= 1.0f) carrierPhase -= 1.0f;
        modPhase += modFreq;
        if (modPhase >= 1.0f) modPhase -= 1.0f;

        return out;
    }

    void reset() noexcept { carrierPhase = 0.0f; modPhase = 0.0f; }
};

//==============================================================================
// Algorithm 3: Additive — 8 harmonics with individual amplitude.
//==============================================================================
struct OuieAdditive
{
    float phases[8] {};

    // brightness: 0..1 controls harmonic rolloff
    float process (float freq, float sr, float brightness) noexcept
    {
        float dt = freq / sr;
        float out = 0.0f;
        float totalAmp = 0.0f;

        for (int h = 0; h < 8; ++h)
        {
            float n = static_cast<float> (h + 1);
            float hDt = dt * n;
            if (hDt > 0.49f) break; // anti-alias: skip harmonics above Nyquist

            // Brightness controls rolloff: high brightness = more harmonics
            float amp = 1.0f / n;
            amp *= (1.0f - (1.0f - brightness) * (n - 1.0f) * 0.12f);
            if (amp < 0.0f) amp = 0.0f;

            out += amp * fastSin (phases[h] * 6.28318530718f);
            totalAmp += amp;

            phases[h] += hDt;
            if (phases[h] >= 1.0f) phases[h] -= 1.0f;
        }

        if (totalAmp > 0.0f)
            out /= totalAmp;

        return out;
    }

    void reset() noexcept { std::memset (phases, 0, sizeof (phases)); }
};

//==============================================================================
// Algorithm 4: Phase Distortion — CZ-style (Casio CZ resonance model).
//==============================================================================
struct OuiePhaseDist
{
    float phase = 0.0f;

    // depth: distortion amount 0..1
    float process (float freq, float sr, float depth) noexcept
    {
        float dt = freq / sr;
        if (dt > 0.49f) dt = 0.49f;

        // CZ-style phase distortion: warp the phase with a piecewise function
        // The distortion creates resonance-like formants
        float d = clamp (depth, 0.0f, 0.99f);
        float warpedPhase;

        if (phase < 0.5f)
        {
            // First half: compress or expand
            float halfPhase = phase * 2.0f;
            warpedPhase = halfPhase * (1.0f + d * 2.0f);
            if (warpedPhase > 1.0f) warpedPhase = 1.0f;
            warpedPhase *= 0.5f;
        }
        else
        {
            // Second half: inverse warp
            float halfPhase = (phase - 0.5f) * 2.0f;
            float warp = halfPhase * (1.0f / (1.0f + d * 2.0f));
            warpedPhase = 0.5f + warp * 0.5f;
        }

        float out = fastSin (warpedPhase * 6.28318530718f);

        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;

        return out;
    }

    void reset() noexcept { phase = 0.0f; }
};

//==============================================================================
// Algorithm 5: Wavefolder — Triangle through multi-stage waveshaping.
//==============================================================================
struct OuieWavefolder
{
    float phase = 0.0f;

    // folds: number of folding stages 1..8
    float process (float freq, float sr, float folds) noexcept
    {
        float dt = freq / sr;
        if (dt > 0.49f) dt = 0.49f;

        // Generate triangle wave
        float tri = 4.0f * std::fabs (phase - 0.5f) - 1.0f;

        // Multi-stage wavefolding (Buchla/Serge style)
        float foldAmt = clamp (folds, 1.0f, 8.0f);
        float signal = tri * foldAmt;

        // Iterative folding: reflect signal at +/-1 boundaries
        // Each fold adds harmonic content
        for (int i = 0; i < 4; ++i)
        {
            if (signal > 1.0f)
                signal = 2.0f - signal;
            else if (signal < -1.0f)
                signal = -2.0f - signal;
        }

        // Soft saturation on output
        signal = fastTanh (signal);

        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;

        return signal;
    }

    void reset() noexcept { phase = 0.0f; }
};

//==============================================================================
// Algorithm 6: Karplus-Strong — Plucked string physical model.
//==============================================================================
struct OuieKS
{
    // 8192 samples covers down to ~12Hz at 96kHz (was 4096 → ~23Hz min at 96kHz)
    static constexpr int kMaxDelay = 8192;
    float buffer[kMaxDelay] {};
    int writePos = 0;
    float prevSample = 0.0f;
    OuieNoiseGen exciteNoise;
    bool needsExcite = false;
    int exciteCounter = 0;

    void excite() noexcept
    {
        needsExcite = true;
        exciteCounter = 0;
    }

    // brightness: 0..1 controls the LP filter in the feedback loop
    float process (float freq, float sr, float brightness) noexcept
    {
        float delaySamples = sr / std::max (20.0f, freq);
        if (delaySamples >= static_cast<float> (kMaxDelay))
            delaySamples = static_cast<float> (kMaxDelay - 1);

        // Excitation: burst of filtered noise
        if (needsExcite && exciteCounter < static_cast<int> (delaySamples))
        {
            float noise = exciteNoise.process();
            buffer[writePos] = flushDenormal (buffer[writePos] + noise * 0.8f);
            ++exciteCounter;
            if (exciteCounter >= static_cast<int> (delaySamples))
                needsExcite = false;
        }

        // Read with linear interpolation
        float readPos = static_cast<float> (writePos) - delaySamples;
        if (readPos < 0.0f) readPos += static_cast<float> (kMaxDelay);

        int idx0 = static_cast<int> (readPos) % kMaxDelay;
        int idx1 = (idx0 + 1) % kMaxDelay;
        float frac = readPos - std::floor (readPos);

        float delayed = buffer[idx0] + frac * (buffer[idx1] - buffer[idx0]);

        // 1-pole lowpass in feedback — brightness controls the damping
        float dampCoeff = 0.1f + (1.0f - brightness) * 0.85f;
        float filtered = delayed + dampCoeff * (prevSample - delayed);
        prevSample = flushDenormal (filtered);

        // Write back with slight loss (string decay)
        buffer[writePos] = flushDenormal (filtered * 0.998f);
        writePos = (writePos + 1) % kMaxDelay;

        return delayed;
    }

    void reset() noexcept
    {
        std::memset (buffer, 0, sizeof (buffer));
        writePos = 0;
        prevSample = 0.0f;
        needsExcite = false;
        exciteCounter = 0;
    }
};

//==============================================================================
// Algorithm 7: Noise — Filtered noise with pitch tracking.
//==============================================================================
struct OuieFilteredNoise
{
    OuieNoiseGen gen;
    CytomicSVF trackFilter;

    // color: 0..1 (0=deep bass rumble, 1=bright hiss)
    float process (float freq, float sr, float color) noexcept
    {
        float noise = gen.process();

        // Track pitch: filter center follows the note frequency
        // Color shifts the center: 0 = octave below, 1 = octave above
        float filterFreq = freq * (0.5f + color * 3.5f);
        filterFreq = clamp (filterFreq, 20.0f, sr * 0.49f);

        // Resonance rises with color for more tonal character
        float reso = 0.2f + color * 0.5f;

        trackFilter.setMode (CytomicSVF::Mode::BandPass);
        trackFilter.setCoefficients_fast (filterFreq, reso, sr);

        return trackFilter.processSample (noise);
    }

    void reset() noexcept
    {
        gen.seed (1);
        trackFilter.reset();
    }
};

//==============================================================================
//
//  III. INTERACTION STAGE — The HAMMER
//
//  The soul of XOuie. Bipolar axis from STRIFE (-1) to LOVE (+1).
//  At center (0), voices pass through unmodified.
//
//  STRIFE (negative values):
//    - Cross-FM: each voice frequency-modulates the other
//    - Ring modulation: multiplicative interference
//    - Hard sync: Voice 2 resets Voice 1's phase at its zero crossings
//
//  LOVE (positive values):
//    - Spectral blend: weighted average of both voices
//    - Harmonic lock: quantize Voice 2 pitch to Voice 1 harmonics
//    - Soft unison: voices converge in pitch with chorus-like detune
//
//==============================================================================
struct OuieInteraction
{
    // Process the interaction between two voice samples.
    // hammerAmount: -1 (full STRIFE) to +1 (full LOVE)
    // Returns two modified samples: one per voice.
    static void process (float& v1, float& v2, float hammerAmount,
                         float v1Phase, float v2Phase) noexcept
    {
        // v1Phase / v2Phase are reserved for a future true hard-sync implementation
        // where Voice 1's phase is reset at Voice 2's zero crossings. For now the
        // sync effect is approximated at sample level below.
        (void) v1Phase;
        (void) v2Phase;

        if (hammerAmount < -0.01f)
        {
            // === STRIFE ===
            float strife = -hammerAmount; // 0..1

            // Cross-FM: each voice modulates the other's output
            float crossFM = strife * 0.6f;
            float v1mod = v1 + v2 * crossFM * 2.0f;
            float v2mod = v2 + v1 * crossFM * 2.0f;

            // Ring modulation blended in at deeper STRIFE
            float ringMod = v1 * v2;
            float ringAmount = clamp ((strife - 0.3f) * 2.0f, 0.0f, 1.0f);

            v1 = v1mod * (1.0f - ringAmount) + ringMod * ringAmount;
            v2 = v2mod * (1.0f - ringAmount) + ringMod * ringAmount;

            // Hard sync effect at extreme STRIFE: phase reset artifacts
            if (strife > 0.7f)
            {
                float syncAmount = (strife - 0.7f) * 3.333f; // 0..1
                // Simulate sync discontinuity with sample distortion
                float syncV1 = fastTanh (v1 * (1.0f + syncAmount * 3.0f));
                v1 = lerp (v1, syncV1, syncAmount * 0.5f);
            }

            // Soft-clip to prevent blowup
            v1 = fastTanh (v1);
            v2 = fastTanh (v2);
        }
        else if (hammerAmount > 0.01f)
        {
            // === LOVE ===
            float love = hammerAmount; // 0..1

            // Spectral blend: weighted average
            float blendAmount = love * 0.7f;
            float blended1 = v1 * (1.0f - blendAmount * 0.5f) + v2 * blendAmount * 0.5f;
            float blended2 = v2 * (1.0f - blendAmount * 0.5f) + v1 * blendAmount * 0.5f;

            // At deep LOVE, voices merge toward unison
            if (love > 0.5f)
            {
                float mergeAmount = (love - 0.5f) * 2.0f; // 0..1
                float avg = (blended1 + blended2) * 0.5f;
                blended1 = lerp (blended1, avg, mergeAmount * 0.6f);
                blended2 = lerp (blended2, avg, mergeAmount * 0.6f);
            }

            v1 = blended1;
            v2 = blended2;
        }
        // At hammerAmount ~= 0, voices pass through unmodified
    }
};

//==============================================================================
//
//  IV. VOICE — Per-voice state (2 voices max, duophonic).
//
//==============================================================================
struct OuieVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // MPE expression
    MPEVoiceExpression mpeExpression;

    // Pitch state
    float currentFreq = 440.0f;
    float targetFreq = 440.0f;
    float glideCoeff = 1.0f;

    // Algorithm instances (all 8 — only the selected one runs per voice)
    OuieVA va;
    OuieWavetable wt;
    OuieFM fm;
    OuieAdditive additive;
    OuiePhaseDist phaseDist;
    OuieWavefolder wavefolder;
    OuieKS ks;
    OuieFilteredNoise filteredNoise;

    // Unison: up to 4 oscillators per voice
    float unisonPhases[4] {};
    float unisonDetune[4] = { 0.0f, 0.003f, -0.003f, 0.006f };

    // Per-voice filter
    CytomicSVF filter;

    // Per-voice envelopes
    StandardADSR ampEnv;
    StandardADSR modEnv;

    // Per-voice LFO
    StandardLFO lfo;

    // Voice-stealing crossfade
    float fadeGain = 1.0f;
    bool fadingOut = false;

    // Current algorithm selection (cached from param)
    int algorithm = 0;

    // Pan position (-1 to 1)
    float pan = 0.0f;

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        currentFreq = 440.0f;
        targetFreq = 440.0f;
        fadeGain = 1.0f;
        fadingOut = false;
        algorithm = 0;
        pan = 0.0f;

        va.reset();
        wt.reset();
        fm.reset();
        additive.reset();
        phaseDist.reset();
        wavefolder.reset();
        ks.reset();
        filteredNoise.reset();

        std::memset (unisonPhases, 0, sizeof (unisonPhases));

        filter.reset();
        ampEnv.reset();
        modEnv.reset();
        lfo.reset();
        mpeExpression.reset();
    }
};

//==============================================================================
//
//  V. ENGINE — OuieEngine, the main class.
//
//==============================================================================
class OuieEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 2;  // Duophonic
    static constexpr float kPI = 3.14159265358979323846f;
    static constexpr float kTwoPi = 6.28318530717958647692f;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        smoothCoeff = 1.0f - std::exp (-kTwoPi * (1.0f / 0.005f) / srf);
        crossfadeRate = 1.0f / (0.005f * srf);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        // Build wavetables for both voices
        for (auto& v : voices)
        {
            v.reset();
            v.wt.build();
            v.filter.reset();
            v.filter.setMode (CytomicSVF::Mode::LowPass);
            v.ks.exciteNoise.seed (54321);
        }

        breathingLFO.reset();

        // SRO SilenceGate: duophonic synth with Karplus-Strong decay — 500ms hold
        prepareSilenceGate (sampleRate, maxBlockSize, 500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        envelopeOutput = 0.0f;
        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingFMMod = 0.0f;
        couplingRingMod = 0.0f;
        modWheelAmount_ = 0.0f;
        aftertouch_ = 0.0f;

        smoothedHammer = 0.0f;
        smoothedCutoff1 = 8000.0f;
        smoothedCutoff2 = 8000.0f;

        breathingLFO.reset();

        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //==========================================================================
    // Audio — the main render loop
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0) return;

        // SRO SilenceGate: wake on note-on, bypass when silent
        for (const auto& md : midi)
            if (md.getMessage().isNoteOn()) { wakeSilenceGate(); break; }
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // --- ParamSnapshot: read all parameters once per block ---
        // Voice algorithms
        const int pAlgo1       = static_cast<int> (loadParam (paramAlgo1, 0.0f));
        const int pAlgo2       = static_cast<int> (loadParam (paramAlgo2, 4.0f));
        const float pWaveform1 = loadParam (paramWaveform1, 0.0f);
        const float pWaveform2 = loadParam (paramWaveform2, 0.0f);
        const float pParam1    = loadParam (paramAlgoParam1, 0.5f);
        const float pParam2    = loadParam (paramAlgoParam2, 0.5f);
        const float pWTPos1    = loadParam (paramWTPos1, 0.0f);
        const float pWTPos2    = loadParam (paramWTPos2, 0.5f);
        const float pFMRatio1  = loadParam (paramFMRatio1, 2.0f);
        const float pFMRatio2  = loadParam (paramFMRatio2, 3.0f);
        const float pFMIndex1  = loadParam (paramFMIndex1, 1.0f);
        const float pFMIndex2  = loadParam (paramFMIndex2, 1.5f);
        const float pPW1       = loadParam (paramPW1, 0.5f);
        const float pPW2       = loadParam (paramPW2, 0.5f);

        // Interaction
        const float pHammer    = loadParam (paramMacroHammer, 0.0f);

        // Filters
        const float pCutoff1   = loadParam (paramCutoff1, 8000.0f);
        const float pReso1     = loadParam (paramReso1, 0.0f);
        const float pCutoff2   = loadParam (paramCutoff2, 8000.0f);
        const float pReso2     = loadParam (paramReso2, 0.0f);
        const int pFilterMode1 = static_cast<int> (loadParam (paramFilterMode1, 0.0f));
        const int pFilterMode2 = static_cast<int> (loadParam (paramFilterMode2, 0.0f));
        const int pFilterLink  = static_cast<int> (loadParam (paramFilterLink, 0.0f));

        // Amp envelopes
        const float pAmpA1     = loadParam (paramAmpA1, 0.01f);
        const float pAmpD1     = loadParam (paramAmpD1, 0.1f);
        const float pAmpS1     = loadParam (paramAmpS1, 0.8f);
        const float pAmpR1     = loadParam (paramAmpR1, 0.3f);
        const float pAmpA2     = loadParam (paramAmpA2, 0.01f);
        const float pAmpD2     = loadParam (paramAmpD2, 0.1f);
        const float pAmpS2     = loadParam (paramAmpS2, 0.8f);
        const float pAmpR2     = loadParam (paramAmpR2, 0.3f);

        // Mod envelopes
        const float pModA      = loadParam (paramModA, 0.01f);
        const float pModD      = loadParam (paramModD, 0.3f);
        const float pModS      = loadParam (paramModS, 0.5f);
        const float pModR      = loadParam (paramModR, 0.5f);
        const float pModDepth  = loadParam (paramModDepth, 0.3f);

        // LFOs
        const float pLfo1Rate  = loadParam (paramLfo1Rate, 0.5f);
        const float pLfo1Depth = loadParam (paramLfo1Depth, 0.3f);
        const int pLfo1Shape   = static_cast<int> (loadParam (paramLfo1Shape, 0.0f));
        const float pLfo2Rate  = loadParam (paramLfo2Rate, 2.0f);
        const float pLfo2Depth = loadParam (paramLfo2Depth, 0.0f);
        const int pLfo2Shape   = static_cast<int> (loadParam (paramLfo2Shape, 0.0f));

        // Unison
        const int pUnisonCount = static_cast<int> (loadParam (paramUnisonCount, 1.0f));
        const float pUnisonDetune = loadParam (paramUnisonDetune, 0.1f);

        // Voice mode & misc
        const int pVoiceMode   = static_cast<int> (loadParam (paramVoiceMode, 1.0f));
        const int pSplitNote   = static_cast<int> (loadParam (paramSplitNote, 60.0f));
        const float pGlide     = loadParam (paramGlide, 0.0f);
        const float pLevel     = loadParam (paramLevel, 0.8f);
        const float pVoiceMix  = loadParam (paramVoiceMix, 0.5f);

        // Macros
        const float pAmpullae  = loadParam (paramMacroAmpullae, 0.5f);
        const float pCartilage = loadParam (paramMacroCartilage, 0.5f);
        const float pCurrent   = loadParam (paramMacroCurrent, 0.0f);

        // Breathing LFO (D005)
        const float pBreathRate = loadParam (paramBreathRate, 0.05f);
        const float pBreathDepth = loadParam (paramBreathDepth, 0.1f);
        breathingLFO.setRate (pBreathRate, srf);

        // === Apply macros ===

        // AMPULLAE: sensitivity — scales velocity and aftertouch response
        float velScale = 0.3f + pAmpullae * 0.7f;

        // CARTILAGE: flexibility — increases resonance, decreases envelope times
        float resoBoost = pCartilage * 0.4f;
        float envSpeedMul = 1.0f + (1.0f - pCartilage) * 2.0f; // faster at low cartilage

        // Mod wheel (D006): morph both voices' algo parameter
        float modWheelMod = modWheelAmount_ * 0.5f;

        // Aftertouch (D006): drive the HAMMER interaction
        float effectiveHammer = clamp (pHammer + aftertouch_ * 0.3f, -1.0f, 1.0f);

        // Glide coefficient
        float glideCoeff = 1.0f;
        if (pGlide > 0.001f)
            glideCoeff = 1.0f - std::exp (-1.0f / (pGlide * srf));

        // Filter link: if enabled, Voice 2 mirrors Voice 1's filter
        float effectiveCutoff1 = clamp (pCutoff1 + couplingFilterMod * 4000.0f, 20.0f, 20000.0f);
        float effectiveReso1 = clamp (pReso1 + resoBoost, 0.0f, 1.0f);
        float effectiveCutoff2 = pFilterLink ? effectiveCutoff1 : clamp (pCutoff2 + couplingFilterMod * 4000.0f, 20.0f, 20000.0f);
        float effectiveReso2 = pFilterLink ? effectiveReso1 : clamp (pReso2 + resoBoost, 0.0f, 1.0f);

        // Set filter modes
        auto toFilterMode = [] (int idx) -> CytomicSVF::Mode {
            switch (idx)
            {
                case 0: return CytomicSVF::Mode::LowPass;
                case 1: return CytomicSVF::Mode::HighPass;
                case 2: return CytomicSVF::Mode::BandPass;
                case 3: return CytomicSVF::Mode::Notch;
                default: return CytomicSVF::Mode::LowPass;
            }
        };

        // Reset coupling accumulators
        float localCouplingFM = couplingFMMod;
        float localCouplingRing = couplingRingMod;
        float localCouplingPitch = couplingPitchMod;
        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingFMMod = 0.0f;
        couplingRingMod = 0.0f;

        // Unison detune spread
        float uniDetuneSpread = pUnisonDetune * 0.02f; // semitones -> ratio

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), msg.getChannel(),
                        pVoiceMode, pSplitNote, glideCoeff,
                        pAmpA1 / envSpeedMul, pAmpD1 / envSpeedMul, pAmpS1, pAmpR1 / envSpeedMul,
                        pAmpA2 / envSpeedMul, pAmpD2 / envSpeedMul, pAmpS2, pAmpR2 / envSpeedMul,
                        pModA / envSpeedMul, pModD / envSpeedMul, pModS, pModR / envSpeedMul,
                        pAlgo1, pAlgo2,
                        pLfo1Rate, pLfo1Shape, pLfo2Rate, pLfo2Shape);
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber(), msg.getChannel());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount_ = msg.getControllerValue() / 127.0f;
            else if (msg.isChannelPressure())
                aftertouch_ = msg.getChannelPressureValue() / 127.0f;
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }

        // --- Update per-voice MPE expression ---
        if (mpeManager != nullptr)
        {
            for (auto& voice : voices)
            {
                if (!voice.active) continue;
                mpeManager->updateVoiceExpression (voice.mpeExpression);
            }
        }

        // --- Update voice filter coefficients once per block ---
        if (voices[0].active)
        {
            voices[0].filter.setMode (toFilterMode (pFilterMode1));
            voices[0].filter.setCoefficients (effectiveCutoff1, effectiveReso1, srf);
        }
        if (voices[1].active)
        {
            voices[1].filter.setMode (toFilterMode (pFilterLink ? pFilterMode1 : pFilterMode2));
            voices[1].filter.setCoefficients (effectiveCutoff2, effectiveReso2, srf);
        }

        float peakEnv = 0.0f;

        // --- Render sample loop ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Smooth HAMMER parameter
            smoothedHammer += (effectiveHammer - smoothedHammer) * smoothCoeff;
            smoothedHammer = flushDenormal (smoothedHammer);

            // Breathing LFO (D005)
            float breathMod = breathingLFO.process() * pBreathDepth;

            float mixL = 0.0f, mixR = 0.0f;

            // We must generate both voice outputs before the interaction stage
            float voiceSamples[2] = { 0.0f, 0.0f };
            float voiceGains[2] = { 0.0f, 0.0f };
            bool voiceActive[2] = { voices[0].active, voices[1].active };

            // --- Generate raw oscillator output per voice ---
            for (int vi = 0; vi < 2; ++vi)
            {
                auto& voice = voices[vi];
                if (!voice.active) continue;

                // Voice-stealing crossfade
                if (voice.fadingOut)
                {
                    voice.fadeGain -= crossfadeRate;
                    voice.fadeGain = flushDenormal (voice.fadeGain);
                    if (voice.fadeGain <= 0.0f)
                    {
                        voice.fadeGain = 0.0f;
                        voice.active = false;
                        voiceActive[vi] = false;
                        continue;
                    }
                }

                // Glide
                voice.currentFreq += (voice.targetFreq - voice.currentFreq) * voice.glideCoeff;
                voice.currentFreq = flushDenormal (voice.currentFreq);

                // MPE pitch bend + global channel pitch bend
                // CPU fix: fastPow2 replaces std::pow — same 2^x formula, no stdlib call per sample.
                float freq = voice.currentFreq * fastPow2 (voice.mpeExpression.pitchBendSemitones / 12.0f)
                             * PitchBendUtil::semitonesToFreqRatio(pitchBendNorm * 2.0f);

                // Coupling pitch mod
                freq *= (1.0f + localCouplingPitch * 0.1f);

                // Breathing LFO pitch mod
                freq *= (1.0f + breathMod * 0.005f);

                // Envelopes
                float ampLevel = voice.ampEnv.process();
                float modLevel = voice.modEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    voiceActive[vi] = false;
                    continue;
                }

                // LFO
                float lfoVal = voice.lfo.process() * (vi == 0 ? pLfo1Depth : pLfo2Depth);

                // Select algorithm parameters based on voice index
                int algo = voice.algorithm;
                float algoParam = (vi == 0 ? pParam1 : pParam2) + modWheelMod;
                algoParam = clamp (algoParam, 0.0f, 1.0f);
                float wtPos = vi == 0 ? pWTPos1 : pWTPos2;
                float fmRatio = vi == 0 ? pFMRatio1 : pFMRatio2;
                float fmIndex = vi == 0 ? pFMIndex1 : pFMIndex2;
                float pw = vi == 0 ? pPW1 : pPW2;
                int waveform = static_cast<int> (vi == 0 ? pWaveform1 : pWaveform2);

                // Mod envelope modulates the algorithm parameter
                float effectiveParam = clamp (algoParam + modLevel * pModDepth + lfoVal * 0.3f, 0.0f, 1.0f);

                // Coupling FM modulation on FM index
                float effectiveFMIndex = fmIndex + localCouplingFM * 2.0f;

                // === Generate unison oscillator output ===
                float oscOut = 0.0f;
                int uniCount = clamp (pUnisonCount, 1, 4);

                for (int u = 0; u < uniCount; ++u)
                {
                    float detuneRatio = 1.0f + voice.unisonDetune[u] * uniDetuneSpread * static_cast<float>(u);
                    float uniFreq = freq * detuneRatio;

                    float uniSample = 0.0f;

                    switch (algo)
                    {
                        case 0: // VA
                            // For unison, offset the VA phase
                            if (u == 0)
                                uniSample = voice.va.process (uniFreq, srf, waveform, pw);
                            else
                            {
                                // Simple additional saw for unison voices
                                float dt = uniFreq / srf;
                                voice.unisonPhases[u] += dt;
                                if (voice.unisonPhases[u] >= 1.0f) voice.unisonPhases[u] -= 1.0f;
                                uniSample = 2.0f * voice.unisonPhases[u] - 1.0f;
                                uniSample -= OuieVA::polyBLEP (voice.unisonPhases[u], dt);
                            }
                            break;
                        case 1: // Wavetable
                            if (u == 0)
                                uniSample = voice.wt.process (uniFreq, srf, wtPos + lfoVal * 0.2f);
                            else
                            {
                                float dt = uniFreq / srf;
                                voice.unisonPhases[u] += dt;
                                if (voice.unisonPhases[u] >= 1.0f) voice.unisonPhases[u] -= 1.0f;
                                // Read from wavetable at same position
                                float tPos = clamp (wtPos + lfoVal * 0.2f, 0.0f, 1.0f);
                                int ti = static_cast<int> (tPos * 15.0f);
                                ti = std::min (ti, 15);
                                int si = static_cast<int> (voice.unisonPhases[u] * 255.0f) % 256;
                                uniSample = voice.wt.tables[ti][si];
                            }
                            break;
                        case 2: // FM
                            if (u == 0)
                                uniSample = voice.fm.process (uniFreq, srf, fmRatio, effectiveFMIndex);
                            else
                            {
                                float dt = uniFreq / srf;
                                voice.unisonPhases[u] += dt;
                                if (voice.unisonPhases[u] >= 1.0f) voice.unisonPhases[u] -= 1.0f;
                                uniSample = fastSin (voice.unisonPhases[u] * 6.28318530718f);
                            }
                            break;
                        case 3: // Additive
                            if (u == 0)
                                uniSample = voice.additive.process (uniFreq, srf, effectiveParam);
                            else
                            {
                                float dt = uniFreq / srf;
                                voice.unisonPhases[u] += dt;
                                if (voice.unisonPhases[u] >= 1.0f) voice.unisonPhases[u] -= 1.0f;
                                uniSample = fastSin (voice.unisonPhases[u] * 6.28318530718f);
                            }
                            break;
                        case 4: // Phase Distortion
                            if (u == 0)
                                uniSample = voice.phaseDist.process (uniFreq, srf, effectiveParam);
                            else
                            {
                                float dt = uniFreq / srf;
                                voice.unisonPhases[u] += dt;
                                if (voice.unisonPhases[u] >= 1.0f) voice.unisonPhases[u] -= 1.0f;
                                uniSample = fastSin (voice.unisonPhases[u] * 6.28318530718f);
                            }
                            break;
                        case 5: // Wavefolder
                            if (u == 0)
                                uniSample = voice.wavefolder.process (uniFreq, srf, 1.0f + effectiveParam * 7.0f);
                            else
                            {
                                float dt = uniFreq / srf;
                                voice.unisonPhases[u] += dt;
                                if (voice.unisonPhases[u] >= 1.0f) voice.unisonPhases[u] -= 1.0f;
                                float tri = 4.0f * std::fabs (voice.unisonPhases[u] - 0.5f) - 1.0f;
                                uniSample = fastTanh (tri * (1.0f + effectiveParam * 3.0f));
                            }
                            break;
                        case 6: // Karplus-Strong
                            if (u == 0)
                                uniSample = voice.ks.process (uniFreq, srf, effectiveParam);
                            else
                                uniSample = 0.0f; // KS is monophonic per voice
                            break;
                        case 7: // Filtered Noise
                            if (u == 0)
                                uniSample = voice.filteredNoise.process (uniFreq, srf, effectiveParam);
                            else
                                uniSample = 0.0f; // Noise is already dense
                            break;
                        default:
                            break;
                    }

                    oscOut += uniSample;
                }

                // Normalize unison
                if (uniCount > 1)
                    oscOut /= std::sqrt (static_cast<float> (uniCount));

                voiceSamples[vi] = oscOut;

                // D001: velocity shapes filter cutoff
                float velFilterBoost = voice.velocity * velScale * 4000.0f;
                float voiceCutoff = (vi == 0 ? effectiveCutoff1 : effectiveCutoff2) + velFilterBoost + breathMod * 500.0f;
                voiceCutoff = clamp (voiceCutoff, 20.0f, 20000.0f);

                // Update filter cutoff per sample for modulation (use fast path)
                voice.filter.setCoefficients_fast (voiceCutoff, vi == 0 ? effectiveReso1 : effectiveReso2, srf);

                voiceGains[vi] = ampLevel * voice.velocity * voice.fadeGain;
            }

            // === INTERACTION STAGE (the HAMMER) ===
            if (voiceActive[0] && voiceActive[1])
            {
                // Coupling: ring mod from external engine
                if (std::fabs (localCouplingRing) > 0.001f)
                {
                    voiceSamples[0] *= (1.0f + localCouplingRing * voiceSamples[1]);
                }

                OuieInteraction::process (voiceSamples[0], voiceSamples[1],
                                          smoothedHammer,
                                          voices[0].va.phase, voices[1].va.phase);
            }

            // === Apply per-voice filter + gain + pan ===
            for (int vi = 0; vi < 2; ++vi)
            {
                if (!voiceActive[vi]) continue;

                float filtered = voices[vi].filter.processSample (voiceSamples[vi]);
                filtered = flushDenormal (filtered);

                float gain = voiceGains[vi];

                // Voice mix balance
                float vmix = (vi == 0) ? (1.0f - pVoiceMix) * 2.0f : pVoiceMix * 2.0f;
                vmix = std::min (vmix, 1.0f);
                gain *= vmix;

                // CURRENT macro: simple stereo spread by voice index
                float panL, panR;
                if (vi == 0)
                {
                    panL = 0.5f + pCurrent * 0.3f; // Left eye -> left
                    panR = 0.5f - pCurrent * 0.3f;
                }
                else
                {
                    panL = 0.5f - pCurrent * 0.3f; // Right eye -> right
                    panR = 0.5f + pCurrent * 0.3f;
                }

                mixL += filtered * gain * panL;
                mixR += filtered * gain * panR;

                peakEnv = std::max (peakEnv, voiceGains[vi]);
            }

            // === CURRENT macro: environment FX (simple chorus + delay) ===
            if (pCurrent > 0.01f)
            {
                // Simple chorus: modulated delay
                float chorusPhaseInc = 0.8f / srf;
                chorusPhase += chorusPhaseInc;
                if (chorusPhase >= 1.0f) chorusPhase -= 1.0f;

                float chorusMod = fastSin (chorusPhase * kTwoPi) * pCurrent * 0.003f * srf;
                float chorusDelay = 5.0f + chorusMod; // ~5ms base delay
                int chorusIdx = static_cast<int> (chorusDelay);
                chorusIdx = std::min (chorusIdx, kChorusBufferSize - 2);
                if (chorusIdx < 0) chorusIdx = 0;

                int readPos = chorusWritePos - chorusIdx;
                if (readPos < 0) readPos += kChorusBufferSize;

                float chorusL = chorusBufferL[readPos] * pCurrent * 0.3f;
                float chorusR = chorusBufferR[readPos] * pCurrent * 0.3f;

                chorusBufferL[chorusWritePos] = mixL;
                chorusBufferR[chorusWritePos] = mixR;
                chorusWritePos = (chorusWritePos + 1) % kChorusBufferSize;

                mixL += chorusL;
                mixR += chorusR;
            }

            // Apply master level
            float finalL = mixL * pLevel;
            float finalR = mixR * pLevel;

            // Denormal protection on output
            finalL = flushDenormal (finalL);
            finalR = flushDenormal (finalR);

            // Write to buffer
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sample, finalL);
                buffer.addSample (1, sample, finalR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sample, (finalL + finalR) * 0.5f);
            }

            // Cache for coupling
            if (sample < static_cast<int> (outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t> (sample)] = finalL;
                outputCacheR[static_cast<size_t> (sample)] = finalR;
            }
        }

        envelopeOutput = peakEnv;

        int count = 0;
        for (const auto& v : voices)
            if (v.active) ++count;
        activeVoices.store(count, std::memory_order_relaxed);

        // SRO SilenceGate: feed output to the gate for silence detection
        analyzeForSilenceGate (buffer, numSamples);
    }

    //==========================================================================
    // Coupling
    //==========================================================================

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0) return 0.0f;
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
                couplingFilterMod += amount;
                break;
            case CouplingType::LFOToPitch:
                couplingPitchMod += amount * 0.1f;
                break;
            case CouplingType::AudioToFM:
                couplingFMMod += amount * 0.5f;
                break;
            case CouplingType::AudioToRing:
                couplingRingMod += amount;
                break;
            default:
                break;
        }
    }

    //==========================================================================
    // Parameters
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

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // --- Voice 1 Algorithm ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_algo1", 1 }, "Ouie V1 Algorithm",
            juce::StringArray { "VA", "Wavetable", "FM", "Additive", "Phase Dist", "Wavefolder", "KS", "Noise" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_waveform1", 1 }, "Ouie V1 Waveform",
            juce::StringArray { "Saw", "Square", "Triangle" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_algoParam1", 1 }, "Ouie V1 Param",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_wtPos1", 1 }, "Ouie V1 WT Position",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_fmRatio1", 1 }, "Ouie V1 FM Ratio",
            juce::NormalisableRange<float> (0.5f, 16.0f, 0.01f, 0.4f), 2.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_fmIndex1", 1 }, "Ouie V1 FM Index",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f, 0.4f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_pw1", 1 }, "Ouie V1 Pulse Width",
            juce::NormalisableRange<float> (0.1f, 0.9f, 0.001f), 0.5f));

        // --- Voice 2 Algorithm ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_algo2", 1 }, "Ouie V2 Algorithm",
            juce::StringArray { "VA", "Wavetable", "FM", "Additive", "Phase Dist", "Wavefolder", "KS", "Noise" }, 4));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_waveform2", 1 }, "Ouie V2 Waveform",
            juce::StringArray { "Saw", "Square", "Triangle" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_algoParam2", 1 }, "Ouie V2 Param",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_wtPos2", 1 }, "Ouie V2 WT Position",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_fmRatio2", 1 }, "Ouie V2 FM Ratio",
            juce::NormalisableRange<float> (0.5f, 16.0f, 0.01f, 0.4f), 3.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_fmIndex2", 1 }, "Ouie V2 FM Index",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f, 0.4f), 1.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_pw2", 1 }, "Ouie V2 Pulse Width",
            juce::NormalisableRange<float> (0.1f, 0.9f, 0.001f), 0.5f));

        // --- Interaction (the HAMMER) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_macroHammer", 1 }, "Ouie HAMMER",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));

        // --- Voice Mix ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_voiceMix", 1 }, "Ouie Voice Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        // --- Filter 1 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_cutoff1", 1 }, "Ouie V1 Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_reso1", 1 }, "Ouie V1 Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_filterMode1", 1 }, "Ouie V1 Filter Mode",
            juce::StringArray { "LowPass", "HighPass", "BandPass", "Notch" }, 0));

        // --- Filter 2 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_cutoff2", 1 }, "Ouie V2 Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_reso2", 1 }, "Ouie V2 Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_filterMode2", 1 }, "Ouie V2 Filter Mode",
            juce::StringArray { "LowPass", "HighPass", "BandPass", "Notch" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_filterLink", 1 }, "Ouie Filter Link",
            juce::StringArray { "Independent", "Linked" }, 0));

        // --- Amp Envelope 1 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_ampA1", 1 }, "Ouie V1 Amp Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_ampD1", 1 }, "Ouie V1 Amp Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.1f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_ampS1", 1 }, "Ouie V1 Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_ampR1", 1 }, "Ouie V1 Amp Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // --- Amp Envelope 2 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_ampA2", 1 }, "Ouie V2 Amp Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_ampD2", 1 }, "Ouie V2 Amp Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.1f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_ampS2", 1 }, "Ouie V2 Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_ampR2", 1 }, "Ouie V2 Amp Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // --- Mod Envelope (shared) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_modA", 1 }, "Ouie Mod Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_modD", 1 }, "Ouie Mod Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_modS", 1 }, "Ouie Mod Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_modR", 1 }, "Ouie Mod Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_modDepth", 1 }, "Ouie Mod Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        // --- LFO 1 (Voice 1) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_lfo1Rate", 1 }, "Ouie LFO1 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_lfo1Depth", 1 }, "Ouie LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_lfo1Shape", 1 }, "Ouie LFO1 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- LFO 2 (Voice 2) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_lfo2Rate", 1 }, "Ouie LFO2 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 2.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_lfo2Depth", 1 }, "Ouie LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_lfo2Shape", 1 }, "Ouie LFO2 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- Breathing LFO (D005) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_breathRate", 1 }, "Ouie Breath Rate",
            juce::NormalisableRange<float> (0.005f, 2.0f, 0.001f, 0.3f), 0.05f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_breathDepth", 1 }, "Ouie Breath Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.1f));

        // --- Unison ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_unisonCount", 1 }, "Ouie Unison Voices",
            juce::StringArray { "1", "2", "3", "4" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_unisonDetune", 1 }, "Ouie Unison Detune",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.1f));

        // --- Voice Mode ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_voiceMode", 1 }, "Ouie Voice Mode",
            juce::StringArray { "Split", "Layer", "Duo" }, 1));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_splitNote", 1 }, "Ouie Split Note",
            juce::NormalisableRange<float> (24.0f, 96.0f, 1.0f), 60.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_glide", 1 }, "Ouie Glide",
            juce::NormalisableRange<float> (0.0f, 5.0f, 0.001f, 0.5f), 0.0f));

        // --- Level ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_level", 1 }, "Ouie Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // --- Macros ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_macroAmpullae", 1 }, "Ouie AMPULLAE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_macroCartilage", 1 }, "Ouie CARTILAGE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_macroCurrent", 1 }, "Ouie CURRENT",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramAlgo1       = apvts.getRawParameterValue ("ouie_algo1");
        paramWaveform1   = apvts.getRawParameterValue ("ouie_waveform1");
        paramAlgoParam1  = apvts.getRawParameterValue ("ouie_algoParam1");
        paramWTPos1      = apvts.getRawParameterValue ("ouie_wtPos1");
        paramFMRatio1    = apvts.getRawParameterValue ("ouie_fmRatio1");
        paramFMIndex1    = apvts.getRawParameterValue ("ouie_fmIndex1");
        paramPW1         = apvts.getRawParameterValue ("ouie_pw1");

        paramAlgo2       = apvts.getRawParameterValue ("ouie_algo2");
        paramWaveform2   = apvts.getRawParameterValue ("ouie_waveform2");
        paramAlgoParam2  = apvts.getRawParameterValue ("ouie_algoParam2");
        paramWTPos2      = apvts.getRawParameterValue ("ouie_wtPos2");
        paramFMRatio2    = apvts.getRawParameterValue ("ouie_fmRatio2");
        paramFMIndex2    = apvts.getRawParameterValue ("ouie_fmIndex2");
        paramPW2         = apvts.getRawParameterValue ("ouie_pw2");

        paramMacroHammer = apvts.getRawParameterValue ("ouie_macroHammer");
        paramVoiceMix    = apvts.getRawParameterValue ("ouie_voiceMix");

        paramCutoff1     = apvts.getRawParameterValue ("ouie_cutoff1");
        paramReso1       = apvts.getRawParameterValue ("ouie_reso1");
        paramFilterMode1 = apvts.getRawParameterValue ("ouie_filterMode1");
        paramCutoff2     = apvts.getRawParameterValue ("ouie_cutoff2");
        paramReso2       = apvts.getRawParameterValue ("ouie_reso2");
        paramFilterMode2 = apvts.getRawParameterValue ("ouie_filterMode2");
        paramFilterLink  = apvts.getRawParameterValue ("ouie_filterLink");

        paramAmpA1       = apvts.getRawParameterValue ("ouie_ampA1");
        paramAmpD1       = apvts.getRawParameterValue ("ouie_ampD1");
        paramAmpS1       = apvts.getRawParameterValue ("ouie_ampS1");
        paramAmpR1       = apvts.getRawParameterValue ("ouie_ampR1");
        paramAmpA2       = apvts.getRawParameterValue ("ouie_ampA2");
        paramAmpD2       = apvts.getRawParameterValue ("ouie_ampD2");
        paramAmpS2       = apvts.getRawParameterValue ("ouie_ampS2");
        paramAmpR2       = apvts.getRawParameterValue ("ouie_ampR2");

        paramModA        = apvts.getRawParameterValue ("ouie_modA");
        paramModD        = apvts.getRawParameterValue ("ouie_modD");
        paramModS        = apvts.getRawParameterValue ("ouie_modS");
        paramModR        = apvts.getRawParameterValue ("ouie_modR");
        paramModDepth    = apvts.getRawParameterValue ("ouie_modDepth");

        paramLfo1Rate    = apvts.getRawParameterValue ("ouie_lfo1Rate");
        paramLfo1Depth   = apvts.getRawParameterValue ("ouie_lfo1Depth");
        paramLfo1Shape   = apvts.getRawParameterValue ("ouie_lfo1Shape");
        paramLfo2Rate    = apvts.getRawParameterValue ("ouie_lfo2Rate");
        paramLfo2Depth   = apvts.getRawParameterValue ("ouie_lfo2Depth");
        paramLfo2Shape   = apvts.getRawParameterValue ("ouie_lfo2Shape");

        paramBreathRate  = apvts.getRawParameterValue ("ouie_breathRate");
        paramBreathDepth = apvts.getRawParameterValue ("ouie_breathDepth");

        paramUnisonCount  = apvts.getRawParameterValue ("ouie_unisonCount");
        paramUnisonDetune = apvts.getRawParameterValue ("ouie_unisonDetune");

        paramVoiceMode   = apvts.getRawParameterValue ("ouie_voiceMode");
        paramSplitNote   = apvts.getRawParameterValue ("ouie_splitNote");
        paramGlide       = apvts.getRawParameterValue ("ouie_glide");
        paramLevel       = apvts.getRawParameterValue ("ouie_level");

        paramMacroAmpullae  = apvts.getRawParameterValue ("ouie_macroAmpullae");
        paramMacroCartilage = apvts.getRawParameterValue ("ouie_macroCartilage");
        paramMacroCurrent   = apvts.getRawParameterValue ("ouie_macroCurrent");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Ouie"; }

    // Hammerhead Steel
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF708090); }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override { return activeVoices.load(std::memory_order_relaxed); }

private:
    //==========================================================================
    // Safe parameter load
    //==========================================================================

    static float loadParam (std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    //==========================================================================
    // MIDI note handling — duophonic voice allocation
    //==========================================================================

    void noteOn (int noteNumber, float velocity, int midiChannel,
                 int voiceMode, int splitNote, float glideCoeff,
                 float ampA1, float ampD1, float ampS1, float ampR1,
                 float ampA2, float ampD2, float ampS2, float ampR2,
                 float modA, float modD, float modS, float modR,
                 int algo1, int algo2,
                 float lfo1Rate, int lfo1Shape,
                 float lfo2Rate, int lfo2Shape)
    {
        float freq = 440.0f * fastPow2 ((static_cast<float> (noteNumber) - 69.0f) / 12.0f);

        switch (voiceMode)
        {
            case 0: // Split — voice 1 below split, voice 2 above
            {
                int vi = (noteNumber < splitNote) ? 0 : 1;
                auto& voice = voices[vi];

                voice.targetFreq = freq;
                voice.glideCoeff = glideCoeff;

                if (!voice.active)
                    voice.currentFreq = freq;

                voice.active = true;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
                voice.fadingOut = false;
                voice.fadeGain = 1.0f;
                voice.startTime = ++voiceCounter;
                voice.algorithm = (vi == 0) ? algo1 : algo2;

                voice.mpeExpression.reset();
                voice.mpeExpression.midiChannel = midiChannel;
                if (mpeManager != nullptr)
                    mpeManager->updateVoiceExpression (voice.mpeExpression);

                voice.ampEnv.setParams (vi == 0 ? ampA1 : ampA2, vi == 0 ? ampD1 : ampD2,
                                        vi == 0 ? ampS1 : ampS2, vi == 0 ? ampR1 : ampR2, srf);
                voice.ampEnv.noteOn();
                voice.modEnv.setParams (modA, modD, modS, modR, srf);
                voice.modEnv.noteOn();

                voice.lfo.setRate (vi == 0 ? lfo1Rate : lfo2Rate, srf);
                voice.lfo.setShape (vi == 0 ? lfo1Shape : lfo2Shape);

                // KS excitation on note-on
                if (voice.algorithm == 6)
                    voice.ks.excite();

                break;
            }
            case 1: // Layer — both voices play every note
            {
                for (int vi = 0; vi < 2; ++vi)
                {
                    auto& voice = voices[vi];

                    voice.targetFreq = freq;
                    voice.glideCoeff = glideCoeff;

                    if (!voice.active)
                        voice.currentFreq = freq;

                    voice.active = true;
                    voice.noteNumber = noteNumber;
                    voice.velocity = velocity;
                    voice.fadingOut = false;
                    voice.fadeGain = 1.0f;
                    voice.startTime = ++voiceCounter;
                    voice.algorithm = (vi == 0) ? algo1 : algo2;

                    voice.mpeExpression.reset();
                    voice.mpeExpression.midiChannel = midiChannel;
                    if (mpeManager != nullptr)
                        mpeManager->updateVoiceExpression (voice.mpeExpression);

                    voice.ampEnv.setParams (vi == 0 ? ampA1 : ampA2, vi == 0 ? ampD1 : ampD2,
                                            vi == 0 ? ampS1 : ampS2, vi == 0 ? ampR1 : ampR2, srf);
                    voice.ampEnv.noteOn();
                    voice.modEnv.setParams (modA, modD, modS, modR, srf);
                    voice.modEnv.noteOn();

                    voice.lfo.setRate (vi == 0 ? lfo1Rate : lfo2Rate, srf);
                    voice.lfo.setShape (vi == 0 ? lfo1Shape : lfo2Shape);

                    if (voice.algorithm == 6)
                        voice.ks.excite();
                }
                break;
            }
            case 2: // Duo — voice 1 = newest note, voice 2 = previous
            {
                // Shift voice 1 -> voice 2 (the previous note)
                if (voices[0].active)
                {
                    voices[1] = voices[0];
                    voices[1].algorithm = algo2;
                    voices[1].lfo.setRate (lfo2Rate, srf);
                    voices[1].lfo.setShape (lfo2Shape);
                    // Retrigger voice 2 envelope for the carried-over note
                    voices[1].ampEnv.setParams (ampA2, ampD2, ampS2, ampR2, srf);
                    voices[1].modEnv.setParams (modA, modD, modS, modR, srf);
                }

                // Voice 1 gets the new note
                auto& voice = voices[0];
                voice.targetFreq = freq;
                voice.glideCoeff = glideCoeff;

                if (!voice.active)
                    voice.currentFreq = freq;

                voice.active = true;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
                voice.fadingOut = false;
                voice.fadeGain = 1.0f;
                voice.startTime = ++voiceCounter;
                voice.algorithm = algo1;

                voice.mpeExpression.reset();
                voice.mpeExpression.midiChannel = midiChannel;
                if (mpeManager != nullptr)
                    mpeManager->updateVoiceExpression (voice.mpeExpression);

                voice.ampEnv.setParams (ampA1, ampD1, ampS1, ampR1, srf);
                voice.ampEnv.noteOn();
                voice.modEnv.setParams (modA, modD, modS, modR, srf);
                voice.modEnv.noteOn();

                voice.lfo.setRate (lfo1Rate, srf);
                voice.lfo.setShape (lfo1Shape);

                if (voice.algorithm == 6)
                    voice.ks.excite();

                break;
            }
        }
    }

    void noteOff (int noteNumber, int midiChannel = 0)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.fadingOut)
            {
                if (midiChannel > 0 && voice.mpeExpression.midiChannel > 0
                    && voice.mpeExpression.midiChannel != midiChannel)
                    continue;

                voice.ampEnv.noteOff();
                voice.modEnv.noteOff();
            }
        }
    }

    //==========================================================================
    // State
    //==========================================================================

    double sr = 44100.0;
    float srf = 44100.0f;
    float smoothCoeff = 0.0f;
    float crossfadeRate = 0.0f;
    uint64_t voiceCounter = 0;

    // MIDI expression
    float modWheelAmount_ = 0.0f;   // CC#1 — morphs algo params (D006)
    float pitchBendNorm   = 0.0f;
    float aftertouch_ = 0.0f;       // Channel pressure -> HAMMER (D006)

    // Voices (2 — duophonic)
    std::array<OuieVoice, kMaxVoices> voices {};
    std::atomic<int> activeVoices{0};

    // Output cache for coupling
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;
    float envelopeOutput = 0.0f;

    // Coupling modulation accumulators
    float couplingFilterMod = 0.0f;
    float couplingPitchMod = 0.0f;
    float couplingFMMod = 0.0f;
    float couplingRingMod = 0.0f;

    // Smoothed control values
    float smoothedHammer = 0.0f;
    float smoothedCutoff1 = 8000.0f;
    float smoothedCutoff2 = 8000.0f;

    // Breathing LFO (D005)
    BreathingLFO breathingLFO;

    // CURRENT macro: chorus delay buffers
    static constexpr int kChorusBufferSize = 4096;
    float chorusBufferL[kChorusBufferSize] {};
    float chorusBufferR[kChorusBufferSize] {};
    int chorusWritePos = 0;
    float chorusPhase = 0.0f;

    // Parameter pointers
    std::atomic<float>* paramAlgo1 = nullptr;
    std::atomic<float>* paramWaveform1 = nullptr;
    std::atomic<float>* paramAlgoParam1 = nullptr;
    std::atomic<float>* paramWTPos1 = nullptr;
    std::atomic<float>* paramFMRatio1 = nullptr;
    std::atomic<float>* paramFMIndex1 = nullptr;
    std::atomic<float>* paramPW1 = nullptr;

    std::atomic<float>* paramAlgo2 = nullptr;
    std::atomic<float>* paramWaveform2 = nullptr;
    std::atomic<float>* paramAlgoParam2 = nullptr;
    std::atomic<float>* paramWTPos2 = nullptr;
    std::atomic<float>* paramFMRatio2 = nullptr;
    std::atomic<float>* paramFMIndex2 = nullptr;
    std::atomic<float>* paramPW2 = nullptr;

    std::atomic<float>* paramMacroHammer = nullptr;
    std::atomic<float>* paramVoiceMix = nullptr;

    std::atomic<float>* paramCutoff1 = nullptr;
    std::atomic<float>* paramReso1 = nullptr;
    std::atomic<float>* paramFilterMode1 = nullptr;
    std::atomic<float>* paramCutoff2 = nullptr;
    std::atomic<float>* paramReso2 = nullptr;
    std::atomic<float>* paramFilterMode2 = nullptr;
    std::atomic<float>* paramFilterLink = nullptr;

    std::atomic<float>* paramAmpA1 = nullptr;
    std::atomic<float>* paramAmpD1 = nullptr;
    std::atomic<float>* paramAmpS1 = nullptr;
    std::atomic<float>* paramAmpR1 = nullptr;
    std::atomic<float>* paramAmpA2 = nullptr;
    std::atomic<float>* paramAmpD2 = nullptr;
    std::atomic<float>* paramAmpS2 = nullptr;
    std::atomic<float>* paramAmpR2 = nullptr;

    std::atomic<float>* paramModA = nullptr;
    std::atomic<float>* paramModD = nullptr;
    std::atomic<float>* paramModS = nullptr;
    std::atomic<float>* paramModR = nullptr;
    std::atomic<float>* paramModDepth = nullptr;

    std::atomic<float>* paramLfo1Rate = nullptr;
    std::atomic<float>* paramLfo1Depth = nullptr;
    std::atomic<float>* paramLfo1Shape = nullptr;
    std::atomic<float>* paramLfo2Rate = nullptr;
    std::atomic<float>* paramLfo2Depth = nullptr;
    std::atomic<float>* paramLfo2Shape = nullptr;

    std::atomic<float>* paramBreathRate = nullptr;
    std::atomic<float>* paramBreathDepth = nullptr;

    std::atomic<float>* paramUnisonCount = nullptr;
    std::atomic<float>* paramUnisonDetune = nullptr;

    std::atomic<float>* paramVoiceMode = nullptr;
    std::atomic<float>* paramSplitNote = nullptr;
    std::atomic<float>* paramGlide = nullptr;
    std::atomic<float>* paramLevel = nullptr;

    std::atomic<float>* paramMacroAmpullae = nullptr;
    std::atomic<float>* paramMacroCartilage = nullptr;
    std::atomic<float>* paramMacroCurrent = nullptr;
};

} // namespace xolokun
