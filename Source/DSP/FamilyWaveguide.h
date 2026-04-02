#pragma once
#include "FastMath.h"
#include <vector>
#include <cmath>
#include <cstring>
#include <array>
#include <algorithm>

namespace xoceanus {

//==============================================================================
// FamilyWaveguide — Shared physical-modeling DSP for the XOrphica Family Constellation.
//
// Five waveguide primitives + seven exciters used by:
//   XOrphica (ORPHICA) — microsound harp
//   XOhm (OHM)         — folk physical modeling + FM
//   XObbligato (OBBLIGATO) — dual wind brothers
//   XOttoni (OTTONI)   — triple brass/sax cousins
//   XOlé (OLE)         — Afro-Latin strummed strings
//
// Design rules:
//   - Header-only, no memory allocation in process path after prepare()
//   - Denormal-safe (uses flushDenormal() from FastMath.h)
//   - No JUCE dependency — pure C++17 DSP
//   - Real-time safe: no allocations, no blocking I/O, no exceptions
//
// Usage pattern:
//   1. Call prepare(sampleRate, maxDelaySamples) once before audio starts
//   2. Set delayLength to pitch (sampleRate / frequency)
//   3. Feed exciter output into write(), then read() each sample
//   4. Route read() output through FamilyDampingFilter in feedback loop
//==============================================================================

//==============================================================================
// FamilyDelayLine
//
// Circular buffer with 4-point Lagrange fractional read.
// The core of waveguide synthesis: delay length controls pitch.
//
//   pitch (Hz) = sampleRate / delayLength
//
// Lagrange 4-point interpolation gives smooth fractional reads without
// the HF rolloff that plagues linear interpolation. Essential for accurate
// pitch over the full keyboard range.
//==============================================================================
class FamilyDelayLine {
public:
    FamilyDelayLine() = default;

    // Call once before audio starts.
    // maxDelaySamples: sampleRate / lowestPitch (e.g. SR/20 for 20 Hz floor).
    void prepare (int maxDelaySamples)
    {
        buffer.assign (maxDelaySamples + 8, 0.0f); // +8: Lagrange guard + round-up
        bufSize = static_cast<int> (buffer.size());
        writePos = 0;
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    // Write one sample into the delay line.
    void write (float x)
    {
        buffer[writePos] = x;
        writePos = (writePos + 1) % bufSize; // bounded — no integer overflow
    }

    // Read with 4-point Lagrange interpolation.
    // delayLength: samples of delay. Must be < maxDelaySamples.
    float read (float delayLength) const
    {
        int d = static_cast<int> (delayLength);
        float frac = delayLength - static_cast<float> (d);

        // Sample taps centered on the integer delay point.
        // After write(), most recent sample is at writePos-1.
        // "d samples ago" = writePos - d.  frac in [0,1) interpolates between
        // writePos-d (y1) and writePos-d+1 (y2), i.e. fractionally < d samples ago.
        auto tap = [&] (int offset) -> float {
            int pos = writePos - d + offset;
            pos = ((pos % bufSize) + bufSize) % bufSize; // safe unsigned wrap
            return buffer[pos];
        };

        // Grid positions (Lagrange nodes at -1, 0, +1, +2):
        //   y0 @ node -1 → d-1 samples back (one newer than center)
        //   y1 @ node  0 → d   samples back (center; frac=0 returns exactly y1)
        //   y2 @ node +1 → d+1 samples back (one older; frac→1 approaches y2)
        //   y3 @ node +2 → d+2 samples back (two older, for curvature)
        // Taps go in the OLDER direction as Lagrange node index increases.
        float y0 = tap ( 1);  // d-1 samples back (newer)
        float y1 = tap ( 0);  // d   samples back (center)
        float y2 = tap (-1);  // d+1 samples back (older)
        float y3 = tap (-2);  // d+2 samples back (oldest)

        // Standard 4-point Lagrange basis polynomials (nodes at -1, 0, 1, 2):
        //   L0(t) =  t(t-1)(t-2) / -6
        //   L1(t) = (t+1)(t-1)(t-2) / 2
        //   L2(t) = (t+1) t (t-2) / -2
        //   L3(t) = (t+1) t (t-1) / 6
        float t  = frac;
        float t1 = t - 1.0f;
        float t2 = t - 2.0f;
        float tp = t + 1.0f;

        float w0 =  t  * t1 * t2 * (-1.0f / 6.0f);
        float w1 =  tp * t1 * t2 * ( 1.0f / 2.0f);
        float w2 =  tp * t  * t2 * (-1.0f / 2.0f);
        float w3 =  tp * t  * t1 * ( 1.0f / 6.0f);

        return w0*y0 + w1*y1 + w2*y2 + w3*y3;
    }

private:
    std::vector<float> buffer;
    int bufSize = 0;
    int writePos = 0;
};

//==============================================================================
// FamilyDampingFilter
//
// One-pole low-pass filter for the waveguide feedback path.
//
//   y[n] = (1-a)*x[n] + a*y[n-1]
//
// Models energy absorption: high frequencies decay faster than low frequencies,
// mimicking the physics of any string or tube (material friction, air viscosity).
//
// damping: 0.0 = no filtering (instant decay, harsh) → 0.9999 = maximum sustain
// Do NOT set to 1.0 (infinite sustain = infinite feedback).
//==============================================================================
class FamilyDampingFilter {
public:
    void prepare() { state = 0.0f; }
    void reset()   { state = 0.0f; }

    // Process one sample. damping: 0.0–0.9999.
    float process (float x, float damping)
    {
        float a = std::min (damping, 0.9999f);
        state = (1.0f - a) * x + a * state;
        state = flushDenormal (state);
        return state;
    }

private:
    float state = 0.0f;
};

//==============================================================================
// FamilyBodyResonance
//
// 2-pole biquad bandpass resonator simulating instrument body modes.
//
// Real instrument bodies have resonant modes determined by their material and
// geometry. A guitar body emphasizes ~200 Hz; a sitar gourd has a distinctive
// mid-range bloom; brass bells reinforce upper partials.
//
// Use 1–3 instances tuned to harmonic ratios of the fundamental.
// gain (at call site, not internal): typically 0.1–0.4 for blend.
//==============================================================================
class FamilyBodyResonance {
public:
    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);
        reset();
    }

    void reset()
    {
        x1 = x2 = y1 = y2 = 0.0f;
    }

    // Recompute biquad coefficients. Call when pitch or material changes.
    // frequency: resonance center Hz  — e.g., 200 Hz (guitar body)
    // resonance: Q factor 1.0–20.0   — higher = narrower, more ringy
    void setParams (float frequency, float resonance)
    {
        // SRO: fastSin/fastCos replace std:: trig (called per-sample from exciters)
        float w0    = 2.0f * 3.14159265f * frequency / sr;
        float cosw0 = fastCos (w0);
        float sinw0 = fastSin (w0);
        float alpha = sinw0 / (2.0f * std::max (resonance, 0.1f));

        float a0 =  1.0f + alpha;
        nb0 =  alpha  / a0;
        nb1 =  0.0f;
        nb2 = -alpha  / a0;
        na1 = -2.0f * cosw0 / a0;
        na2 =  (1.0f - alpha) / a0;
    }

    float process (float x)
    {
        float y = nb0 * x + nb1 * x1 + nb2 * x2 - na1 * y1 - na2 * y2;
        y = flushDenormal (y);
        x2 = x1;  x1 = x;
        y2 = y1;  y1 = y;
        return y;
    }

private:
    float sr  = 44100.0f;
    float nb0 = 0.0f, nb1 = 0.0f, nb2 = 0.0f;
    float na1 = 0.0f, na2 = 0.0f;
    float x1  = 0.0f, x2  = 0.0f;
    float y1  = 0.0f, y2  = 0.0f;
};

//==============================================================================
// FamilySympatheticBank
//
// 8 tuned comb filters simulating sympathetic strings or resonant tubes.
//
// On a sitar, 13 sympathetic strings resonate when the main strings are plucked.
// On a Norwegian hardanger fiddle, 8 steel strings resonate below the bow strings.
// FamilySympatheticBank approximates this with 8 comb filters tuned to harmonic
// ratios (1x, 2x, 3x, 4x, 5x, 6x, 8x, 12x) of the base frequency.
//
// The result: shimmer, bloom, and sympathetic sustain that makes physical modeling
// feel like a real instrument room, not a simulation.
//==============================================================================
class FamilySympatheticBank {
public:
    static constexpr int   kNumCombs    = 8;
    static constexpr float kHarmonics[] = { 1.0f, 2.0f, 3.0f, 4.0f,
                                             5.0f, 6.0f, 8.0f, 12.0f };

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = static_cast<float> (sampleRate);
        int maxLen = static_cast<int> (sr / 20.0f) + 8; // min 20 Hz
        for (int i = 0; i < kNumCombs; ++i)
        {
            combs[i].assign (maxLen, 0.0f);
            positions[i] = 0;
            lengths[i]   = 100.0f;
        }
        std::fill (states, states + kNumCombs, 0.0f);
    }

    void reset()
    {
        for (int i = 0; i < kNumCombs; ++i)
        {
            std::fill (combs[i].begin(), combs[i].end(), 0.0f);
            positions[i] = 0;
        }
        std::fill (states, states + kNumCombs, 0.0f);
    }

    // Retune all 8 combs to harmonics of baseFrequency.
    void tune (float baseFrequency)
    {
        for (int i = 0; i < kNumCombs; ++i)
        {
            float f = std::max (1.0f, baseFrequency * kHarmonics[i]);
            lengths[i] = std::max (sr / f, 2.0f);
        }
    }

    // Process one sample.
    // amount: 0.0 (sympathetic off) → 1.0 (full sympathetic blend)
    float process (float input, float amount)
    {
        float out = 0.0f;
        for (int i = 0; i < kNumCombs; ++i)
        {
            int sz      = static_cast<int> (combs[i].size());
            int len     = static_cast<int> (lengths[i]);
            int readPos = ((positions[i] - len) % sz + sz) % sz;

            float delayed = combs[i][readPos];
            float fed     = flushDenormal (input + 0.82f * delayed); // 0.82 = stable decay
            combs[i][positions[i]] = fed;
            positions[i] = (positions[i] + 1) % sz;

            // Smooth state for musical sustain
            states[i] = flushDenormal (states[i] * 0.92f + delayed * 0.08f);
            out += states[i];
        }
        return (out / kNumCombs) * amount;
    }

private:
    float sr = 44100.0f;
    std::vector<float> combs[kNumCombs];
    int   positions[kNumCombs] = {};
    float lengths[kNumCombs]   = {};
    float states[kNumCombs]    = {};
};

//==============================================================================
// FamilyOrganicDrift
//
// Slow pitch/timing wander for humanized waveguide voices.
//
// Real instruments drift: intonation wanders with breath temperature, bow
// pressure fatigue, finger micro-variations. FamilyOrganicDrift uses two
// LFOs at a golden-ratio frequency ratio to avoid periodicity and produce
// naturalistic, non-repeating pitch wander.
//
// Output: pitch deviation as a fraction of a semitone (±0.05 = ±5 cents).
// Apply: newDelay = baseDelay * std::pow(2.0f, drift / 12.0f)
//   or for small drifts (<±50 cents): newDelay = baseDelay * (1.0f + drift * 0.05776f)
//   (0.05776 ≈ (2^(1/12) - 1), the per-semitone linear approximation)
//==============================================================================
class FamilyOrganicDrift {
public:
    void prepare (double sampleRate)
    {
        sr     = static_cast<float> (sampleRate);
        phase1 = phase2 = 0.0f;
    }

    void reset() { phase1 = phase2 = 0.0f; }

    // Returns pitch deviation as fraction of a semitone (e.g. ±0.05 = ±5 cents).
    // rateHz:     drift speed, 0.05–0.5 Hz typical
    // depthCents: peak pitch deviation in cents, 0–20 typical
    float tick (float rateHz, float depthCents)
    {
        float inc1 = rateHz / sr;
        float inc2 = rateHz * 1.6180339f / sr; // golden ratio avoids harmonic beating

        phase1 = std::fmod (phase1 + inc1, 1.0f);
        phase2 = std::fmod (phase2 + inc2, 1.0f);

        // SRO: fastSin replaces std::sin (per-sample × 2 LFOs)
        float lfo = 0.6f * fastSin (phase1 * 6.2831853f)
                  + 0.4f * fastSin (phase2 * 6.2831853f);

        return lfo * (depthCents / 100.0f); // cents → semitone fraction
    }

private:
    float sr     = 44100.0f;
    float phase1 = 0.0f;
    float phase2 = 0.0f;
};

//==============================================================================
//
//  E X C I T E R S
//
//  Each exciter seeds a FamilyDelayLine with an initial burst of energy.
//  The delay line + FamilyDampingFilter sustains that energy as a pitched tone.
//
//  One-shot exciters (Pluck, Pick): trigger() on note-on, tick() returns
//    non-zero for burstMs then goes silent. The waveguide sustains.
//
//  Continuous exciters (AirJet, Reed, LipBuzz, Bow): tick() every sample,
//    returns signal as long as the physical control parameter is non-zero.
//    These models keep the waveguide excited (like a sustained bow stroke).
//
//==============================================================================

//==============================================================================
// PluckExciter
//
// Short filtered noise burst for harp and plucked string tones.
// The finger's pluck releases stored energy as broadband noise; the delay line
// then filters it into a sustained pitched tone.
//
// trigger():   call on note-on
// tick():      call each sample; returns non-zero for burstMs, then 0
// brightness:  0.0 (dark gut/nylon) → 1.0 (crystal/glass)
//==============================================================================
class PluckExciter {
public:
    void prepare (double sampleRate)
    {
        sr          = static_cast<float> (sampleRate);
        filterState = 0.0f;
        remaining   = 0;
        seed        = 12345u;
    }

    void reset()
    {
        remaining   = 0;
        filterState = 0.0f;
    }

    // Fire exciter. burstMs: length of noise burst (1–5 ms typical).
    void trigger (float burstMs = 2.0f)
    {
        remaining = static_cast<int> (sr * burstMs * 0.001f);
    }

    // brightness: 0.0 = warm/dark, 1.0 = bright/crystal
    float tick (float brightness)
    {
        if (remaining <= 0) return 0.0f;
        --remaining;

        // Real-time safe LCG noise — never use std::rand on the audio thread
        seed        = seed * 1664525u + 1013904223u;
        float noise = static_cast<float> (static_cast<int32_t> (seed)) * 4.656612e-10f;

        // One-pole LP/HP blend: high brightness → more HF retained
        float coeff = 0.1f + brightness * 0.85f;
        filterState = coeff * noise + (1.0f - coeff) * filterState;
        filterState = flushDenormal (filterState);

        // Exponential decay envelope over burst window
        float env = static_cast<float> (remaining) /
                    static_cast<float> (remaining + 1);
        return filterState * env;
    }

private:
    float    sr          = 44100.0f;
    float    filterState = 0.0f;
    int      remaining   = 0;
    uint32_t seed        = 12345u;
};

//==============================================================================
// StrumExciter
//
// Staggered multi-string plucks for strummed chord textures.
// Fires up to 6 PluckExciters with inter-string delay to simulate strum.
//
// trigger():      call on note-on
// numStrings:     1–6 strings to strum
// strumRateMs:    time between successive string triggers (1–30 ms)
// direction:      +1.0 = down strum, -1.0 = up strum (reverses string order)
//==============================================================================
class StrumExciter {
public:
    static constexpr int kMaxStrings = 6;

    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);
        for (auto& p : plucks)
            p.prepare (sampleRate);
        reset();
    }

    void reset()
    {
        for (auto& p : plucks) p.reset();
        std::fill (countdown, countdown + kMaxStrings, 0);
    }

    void trigger (int numStrings, float strumRateMs, float direction)
    {
        numStrings = std::min (numStrings, kMaxStrings);
        int interval = static_cast<int> (sr * strumRateMs * 0.001f);
        for (int i = 0; i < kMaxStrings; ++i) countdown[i] = -1; // inactive

        for (int i = 0; i < numStrings; ++i)
        {
            int idx = (direction >= 0.0f) ? i : (numStrings - 1 - i);
            countdown[idx] = i * interval + 1; // +1: count down to trigger
        }
    }

    float tick (float brightness)
    {
        float out = 0.0f;
        for (int i = 0; i < kMaxStrings; ++i)
        {
            if (countdown[i] > 0)
            {
                --countdown[i];
                if (countdown[i] == 0) plucks[i].trigger (2.0f);
            }
            out += plucks[i].tick (brightness);
        }
        return out;
    }

private:
    float        sr         = 44100.0f;
    PluckExciter plucks[kMaxStrings];
    int          countdown[kMaxStrings] = {};
};

//==============================================================================
// PickExciter
//
// Fingerstyle or clawhammer attack for banjo/guitar/mandolin.
// Similar to PluckExciter but with bandpass-shaped attack simulating
// the nail sliding across the string before release ("twang").
//
// hardness:  0.0 = thumb/warm, 1.0 = fingernail/bright
//==============================================================================
class PickExciter {
public:
    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);
        body.prepare (sampleRate);
        remaining = 0;
        seed      = 98765u;
    }

    void reset()
    {
        remaining = 0;
        body.reset();
    }

    void trigger (float attackMs = 1.5f)
    {
        remaining = static_cast<int> (sr * attackMs * 0.001f);
    }

    float tick (float hardness)
    {
        if (remaining <= 0) return 0.0f;
        --remaining;

        seed        = seed * 1664525u + 1013904223u;
        float noise = static_cast<float> (static_cast<int32_t> (seed)) * 4.656612e-10f;

        // Bandpass tuned to nail attack frequency (2–4 kHz range)
        float mid = 200.0f + hardness * 3800.0f;
        body.setParams (mid, 2.5f + hardness * 5.0f);
        float shaped = noise * (1.0f - hardness * 0.5f) + body.process (noise) * hardness;

        float env = static_cast<float> (remaining) /
                    static_cast<float> (remaining + 4);
        return flushDenormal (shaped * env * 0.8f);
    }

private:
    float              sr        = 44100.0f;
    FamilyBodyResonance body;
    int                remaining = 0;
    uint32_t           seed      = 98765u;
};

//==============================================================================
// AirJetExciter
//
// Breath-driven air jet for flute-family instruments (XObbligato Brother A).
//
// Models the air jet striking the embouchure hole: noise filtered by a
// bandpass tuned near the resonant frequency, modulated by breath pressure.
// Includes a natural breath-noise floor for realism at low pressure.
//
// breathPressure: 0.0 (no sound) → 1.0 (full tone)
// frequency:      fundamental pitch Hz — tunes the jet resonance window
//==============================================================================
class AirJetExciter {
public:
    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);
        jetr.prepare (sampleRate);
        seed     = 54321u;
        hpState  = 0.0f;
    }

    void reset()
    {
        hpState     = 0.0f;
        hpStatePrev = 0.0f;
        jetr.reset();
    }

    float tick (float breathPressure, float frequency)
    {
        seed        = seed * 1664525u + 1013904223u;
        float noise = static_cast<float> (static_cast<int32_t> (seed)) * 4.656612e-10f;

        // Bandpass near resonance — models air jet coupling to tube
        jetr.setParams (frequency * 0.5f, 1.8f);
        float jet = jetr.process (noise) * breathPressure;

        // DC-blocking one-pole HP
        hpState = flushDenormal (0.9995f * (hpState + jet - hpStatePrev));
        hpStatePrev = jet;

        // Natural breath noise floor at low (but nonzero) pressure — no noise at silence
        float breathNoise = (breathPressure > 0.01f)
                          ? noise * std::max (0.0f, 0.06f - breathPressure * 0.06f)
                          : 0.0f;

        return flushDenormal (hpState + breathNoise);
    }

private:
    float               sr          = 44100.0f;
    FamilyBodyResonance jetr;
    float               hpState     = 0.0f;
    float               hpStatePrev = 0.0f;
    uint32_t            seed        = 54321u;
};

//==============================================================================
// ReedExciter
//
// Nonlinear reed model for clarinet/oboe/bassoon family (XObbligato Brother B).
//
// The reed is a pressure-controlled valve: at low pressure it opens freely,
// at high pressure it clamps shut. This nonlinear behavior creates the
// characteristic odd-harmonic saturation of reed instruments.
//
// Uses tanh saturation of the pressure signal, then LP-filtered for
// the reed's mechanical mass response.
//
// reedStiffness: 0.0 (soft/clarinet) → 1.0 (hard/bassoon brightness)
//==============================================================================
class ReedExciter {
public:
    void prepare (double sampleRate)
    {
        sr     = static_cast<float> (sampleRate);
        filter.prepare();
        seed   = 77777u;
    }

    void reset() { filter.reset(); }

    float tick (float breathPressure, float reedStiffness)
    {
        seed        = seed * 1664525u + 1013904223u;
        float noise = static_cast<float> (static_cast<int32_t> (seed)) * 4.656612e-10f * 0.08f;

        float pressure = breathPressure + noise;

        // Nonlinear reed valve: cubic saturation produces odd harmonics
        float drive   = 1.0f + reedStiffness * 4.0f;
        float clipped = fastTanh (pressure * drive);

        // LP filter models mechanical mass of reed (darker = heavier reed)
        float damping = 0.65f + reedStiffness * 0.3f;
        return filter.process (clipped, damping);
    }

private:
    float              sr = 44100.0f;
    FamilyDampingFilter filter;
    uint32_t           seed = 77777u;
};

//==============================================================================
// LipBuzzExciter
//
// Lip buzz oscillation for brass and natural horn family (XOttoni).
//
// Models lip vibration as a bandlimited sawtooth (the simplest model of
// Helmholtz motion through lips) filtered by the lip mass-spring response.
//
// embouchureTension: 0.0 (loose/tuba/conch) → 1.0 (tight/trumpet)
// ageScale:          0.0 (toddler/simple)   → 1.0 (teen/full virtuosity)
//   XOttoni's age model: toddler = loose intonation, teen = stable
//==============================================================================
class LipBuzzExciter {
public:
    void prepare (double sampleRate)
    {
        sr        = static_cast<float> (sampleRate);
        phase     = 0.0f;
        lipFilter.prepare();
    }

    void reset()
    {
        phase = 0.0f;
        lipFilter.reset();
    }

    float tick (float frequency, float embouchureTension, float ageScale)
    {
        // Age-scaled intonation instability: toddlers wobble, teens are stable
        float jitterAmt = (1.0f - ageScale) * 0.015f;
        // SRO: fastSin replaces std::sin (per-sample)
        float jitter    = jitterAmt * fastSin (phase * 17.3f); // irregular flutter
        float adjFreq   = frequency * (1.0f + jitter);

        phase += adjFreq / sr;
        if (phase >= 1.0f) phase -= 1.0f;

        // Sawtooth = fundamental lip buzz
        float saw = 2.0f * phase - 1.0f;

        // Lip mass filter: loose lips = more LP (bigger/darker sound)
        float damping = 0.45f + embouchureTension * 0.5f;
        float buzzed  = lipFilter.process (saw, damping);

        // Age scales output amplitude (toddlers can't play loud)
        float amplitude = 0.3f + ageScale * 0.7f;
        return flushDenormal (buzzed * amplitude);
    }

private:
    float               sr  = 44100.0f;
    float               phase = 0.0f;
    FamilyDampingFilter lipFilter;
};

//==============================================================================
// BowExciter
//
// Continuous friction model for bowed string instruments (XOhm fiddle).
//
// Models Helmholtz motion: the bow alternately sticks and slips on the string.
// Stick phase: friction drags the string with the bow velocity.
// Slip phase:  string springs back (Helmholtz flyback).
//
// Approximated with velocity-dependent tanh friction: fast bow = slippy/bright,
// slow/heavy bow = sticky/warm. Feed the waveguide output back as stringVelocity
// to close the Helmholtz loop properly.
//
// bowPressure:    0.0 (lifted) → 1.0 (full weight)
// bowSpeed:       0.0 (slow)  → 1.0 (fast)
// stringVelocity: feed waveguide output sample back here each tick
//==============================================================================
class BowExciter {
public:
    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);
        frictionLP.prepare();
    }

    void reset() { frictionLP.reset(); }

    float tick (float bowPressure, float bowSpeed, float stringVelocity)
    {
        if (bowPressure < 0.001f) return 0.0f;

        // Relative velocity: bow surface velocity minus string velocity
        float relVel = bowSpeed - stringVelocity;

        // Friction curve: tanh models the nonlinear stick/slip transition
        // Higher bow pressure = tighter stick phase = stronger driving force
        float friction = bowPressure * fastTanh (relVel * (5.0f + bowPressure * 12.0f));

        // Smooth friction impulses (bow hair has distributed contact)
        float damping = 0.55f + bowPressure * 0.35f;
        return flushDenormal (frictionLP.process (friction, damping));
    }

private:
    float               sr = 44100.0f;
    FamilyDampingFilter frictionLP;
};

} // namespace xoceanus
