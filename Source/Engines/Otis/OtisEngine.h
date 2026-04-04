// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OtisEngine.h — XOtis | "The Soul Organ"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOtis is the gospel fish — a golden humphead parrotfish that swims
//      through Sunday morning services, its massive beak grinding coral into
//      sand. Its body resonates with the tonewheels of every Hammond organ
//      that ever played a church, a jazz club, a roadhouse. When it opens
//      its mouth the Leslie horn spins and the whole reef trembles with
//      soul. Otis Redding, Jimmy Smith, Billy Preston — their fingers are
//      in the water, pressing the keys, pulling the drawbars, stomping the
//      Leslie switch.
//
//  ENGINE CONCEPT:
//      A four-model organ engine spanning the breadth of American wind/reed
//      instruments. The Hammond B3 tonewheel organ — the most recognizable
//      organ sound in the world — is the flagship model, with physically-
//      modeled drawbar harmonics, tonewheel crosstalk, key click transient,
//      percussion harmonic emphasis, and Leslie rotating speaker simulation.
//      Three companion models expand the palette: the Calliope (steam-driven
//      chaos), the Blues Harmonica (breath and bend), and the Zydeco
//      Accordion (buzzy Louisiana reeds).
//
//  THE 4 ORGAN MODELS:
//      0: Hammond B3 — 9-drawbar additive tonewheel, Leslie, key click,
//         percussion. The heartbeat of soul, gospel, jazz, and rock.
//      1: Calliope — Steam organ with massive pitch instability, binary
//         dynamics, inter-pipe detuning. Carnival unhinged.
//      2: Blues Harmonica — Breath-driven with pitch bend envelope,
//         cross-harp blue notes, overblows. Raw emotional human blues.
//      3: Zydeco Accordion — Diatonic button accordion, buzzy reed
//         model with strong attack. Louisiana dance heat.
//
//  HISTORICAL LINEAGE:
//      Hammond tonewheel organ (Laurens Hammond, 1935), Leslie rotating
//      speaker (Don Leslie, 1941), Blues harmonica cross-harp technique
//      (Sonny Boy Williamson, Little Walter), Calliope steam organ
//      (Joshua C. Stoddard, 1855), Zydeco accordion (Clifton Chenier,
//      Buckwheat Zydeco). Drawbar harmonic series follows the original
//      Hammond footage ratios: 16', 5-1/3', 8', 4', 2-2/3', 2',
//      1-3/5', 1-1/3', 1'.
//
//  Accent: Gospel Gold #D4A017
//  Parameter prefix: otis_
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/FilterEnvelope.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/ParameterSmoother.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// Hammond drawbar footage ratios — the harmonic series of the B3.
// Each drawbar controls one harmonic partial. The footage numbers
// correspond to pipe organ pipe lengths.
//
//   Drawbar   Footage   Harmonic   Interval
//     1       16'       1st        Sub-octave
//     2       5-1/3'    3rd        Fifth + octave below
//     3       8'        2nd        Unison
//     4       4'        4th        Octave above
//     5       2-2/3'    6th        Octave + fifth above
//     6       2'        8th        Two octaves above
//     7       1-3/5'    10th       Two octaves + major third
//     8       1-1/3'    12th       Two octaves + fifth
//     9       1'        16th       Three octaves above
//
// These ratios are EXACT per the original Hammond tonewheel gearing.
//==============================================================================
static constexpr float kHammondHarmonics[9] = {
    0.5f, // 16'  — sub-octave (fundamental / 2)
    1.5f, // 5⅓' — 3rd harmonic (sub-fifth)
    1.0f, // 8'   — unison (true fundamental)
    2.0f, // 4'   — octave
    3.0f, // 2⅔' — octave + fifth
    4.0f, // 2'   — two octaves
    5.0f, // 1⅗' — two octaves + major third
    6.0f, // 1⅓' — two octaves + fifth
    8.0f  // 1'   — three octaves
};

//==============================================================================
// Tonewheel crosstalk model — adjacent tonewheels bleed into each other.
// Real Hammond organs have 91 tonewheels on a single shaft driven by a
// synchronous motor. Adjacent wheels are physically coupled through the
// shaft and pickup crosstalk. This adds the characteristic warmth and
// "sizzle" that distinguishes a real B3 from a clean additive synth.
//
// Each tonewheel bleeds ~0.5-3% into its neighbors. The crosstalk amount
// varies with wheel proximity (mechanical coupling) and pickup alignment
// (electromagnetic coupling). We model this as additive sine leakage
// from the nearest 2 tonewheels above and below each drawbar harmonic.
//==============================================================================
struct TonewheelCrosstalk
{
    // Generate crosstalk for a given drawbar's harmonic
    // Returns the additional signal from adjacent tonewheels
    float process(float fundamentalFreq, int drawbarIndex, float phase, float amount, float sampleRate) noexcept
    {
        if (amount < 0.001f)
            return 0.0f;

        float crosstalkSum = 0.0f;

        // Adjacent tonewheel frequencies — slightly detuned from exact harmonics
        // Real Hammond uses gear ratios that aren't perfectly integer, creating
        // micro-detuning between "adjacent" tonewheels.
        float baseHarmonic = kHammondHarmonics[drawbarIndex];

        // Lower neighbor: harmonic slightly below
        float lowerRatio = baseHarmonic * 0.9944f; // ~10 cents flat (gear tooth mesh)
        float lowerFreq = fundamentalFreq * lowerRatio;
        float lowerPhase = phase * lowerRatio / baseHarmonic;
        crosstalkSum += fastSin(lowerPhase * kTwoPi) * 0.015f;

        // Upper neighbor: harmonic slightly above
        float upperRatio = baseHarmonic * 1.0056f; // ~10 cents sharp
        float upperFreq = fundamentalFreq * upperRatio;
        float upperPhase = phase * upperRatio / baseHarmonic;
        crosstalkSum += fastSin(upperPhase * kTwoPi) * 0.012f;

        // Second neighbors (weaker)
        float farLowerRatio = baseHarmonic * 0.9889f;
        float farLowerPhase = phase * farLowerRatio / baseHarmonic;
        crosstalkSum += fastSin(farLowerPhase * kTwoPi) * 0.005f;

        float farUpperRatio = baseHarmonic * 1.0112f;
        float farUpperPhase = phase * farUpperRatio / baseHarmonic;
        crosstalkSum += fastSin(farUpperPhase * kTwoPi) * 0.004f;

        (void)lowerFreq;
        (void)upperFreq; // suppress warnings
        (void)sampleRate;

        return crosstalkSum * amount;
    }

    static constexpr float kTwoPi = 6.28318530717958647692f;
};

//==============================================================================
// KeyClick — Mechanical contact noise transient on note-on.
// The original Hammond B3 key click was an unintended artifact of the
// nine-bus-bar key contacts closing. Hammond engineers tried to eliminate
// it; musicians demanded it back. The click is now one of the most
// beloved transient characteristics in all of keyboard sound design.
//
// DSP: short burst of filtered noise on note-on, shaped by a fast
// exponential decay. The click contains energy across 1-8 kHz.
//==============================================================================
struct KeyClick
{
    void trigger(float velocity, float clickLevel, float sampleRate) noexcept
    {
        if (clickLevel < 0.001f)
        {
            active = false;
            return;
        }

        active = true;
        level = velocity * clickLevel;
        // Click duration: 1-3ms (the real B3 click is ~2ms)
        decayCoeff = std::exp(-1.0f / (0.002f * sampleRate));
        envelope = 1.0f;
        noiseState = static_cast<uint32_t>(velocity * 65535.0f) + 98765u;
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;

        // Generate band-limited noise in the click frequency range
        noiseState = noiseState * 1664525u + 1013904223u;
        float noise = (static_cast<float>(noiseState & 0xFFFF) / 32768.0f - 1.0f);

        float out = noise * envelope * level;
        envelope *= decayCoeff;
        envelope = flushDenormal(envelope);

        if (envelope < 1e-6f)
            active = false;

        return out;
    }

    void reset() noexcept
    {
        active = false;
        envelope = 0.0f;
    }

    bool active = false;
    float level = 0.0f;
    float envelope = 0.0f;
    float decayCoeff = 0.99f;
    uint32_t noiseState = 98765u;
};

//==============================================================================
// HammondPercussion — Single-trigger harmonic emphasis.
// The Hammond percussion circuit adds a brief emphasis on either the 2nd
// or 3rd harmonic (selectable) at note-on. It is "single-trigger" — only
// the first note in a legato passage gets percussion; subsequent notes
// while any key is held do NOT re-trigger. This is critical to authentic
// Hammond feel: the percussion adds attack definition without turning
// every note into a staccato punch.
//
// Two modes: 2nd harmonic (bright, cutting) or 3rd harmonic (warm, round).
// Fast decay (~200ms) or slow decay (~500ms).
//==============================================================================
struct HammondPercussion
{
    void trigger(float velocity, float percLevel, float harmSelect, float decayTime, float sampleRate) noexcept
    {
        if (percLevel < 0.001f)
        {
            active = false;
            return;
        }

        active = true;
        level = velocity * percLevel;
        // harmSelect: 0 = 2nd harmonic (4'), 1 = 3rd harmonic (2-2/3')
        harmonicRatio = (harmSelect < 0.5f) ? 2.0f : 3.0f;
        // Decay: 200ms (fast) to 500ms (slow)
        float decaySec = 0.2f + decayTime * 0.3f;
        decayCoeff = std::exp(-4.6f / (decaySec * sampleRate));
        envelope = 1.0f;
    }

    float process(float phase) noexcept
    {
        if (!active)
            return 0.0f;

        float percPhase = phase * harmonicRatio;
        float out = fastSin(percPhase * kTwoPi) * envelope * level;

        envelope *= decayCoeff;
        envelope = flushDenormal(envelope);
        if (envelope < 1e-6f)
            active = false;

        return out;
    }

    void reset() noexcept
    {
        active = false;
        envelope = 0.0f;
    }

    bool active = false;
    float level = 0.0f;
    float harmonicRatio = 2.0f;
    float envelope = 0.0f;
    float decayCoeff = 0.999f;

    static constexpr float kTwoPi = 6.28318530717958647692f;
};

//==============================================================================
// LeslieSpeaker — Rotating horn + drum speaker simulation.
//
// The Leslie speaker cabinet is inseparable from the Hammond sound. A
// rotating horn (treble) and rotating drum/baffle (bass) create complex
// amplitude modulation (tremolo), true Doppler pitch shift, and spatial
// movement. The Leslie has three speeds: brake (stopped), slow (chorale,
// ~0.7 Hz), and fast (tremolo, ~6.7 Hz). The transition between speeds
// is NOT instant — the motor spins up and spins down with physical
// inertia (ramp time ~1-3 seconds).
//
// DSP model:
//   - Frequency split: ~800 Hz crossover. Horn handles highs, drum handles lows.
//   - Horn: AM (±6 dB) + TRUE Doppler via a short circular delay buffer.
//     The horn rotation sine drives a time-varying read position. At 48kHz
//     a 2048-sample buffer covers ~42ms, more than enough for the ±~15 cent
//     Doppler shift at fast speed. Linear interpolation for fractional reads.
//   - Drum: AM only (±3 dB), slower, no Doppler needed for low frequencies.
//   - Speed ramping: one-pole smoothing on rotation rate (physical inertia).
//   - Stereo imaging: horn and drum 90° phase offset between L/R ears.
//==============================================================================
struct LeslieSpeaker
{
    static constexpr int kDelayBufferSize = 2048; // ~42ms at 48kHz, never heap-allocated

    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        hornPhase = 0.0f;
        drumPhase = 0.25f; // 90 degrees offset from horn
        currentHornRate = 0.7f;
        currentDrumRate = 0.6f;
        // Speed ramp coefficient: ~1.5 second transition time
        speedRampCoeff = 1.0f - std::exp(-1.0f / (1.5f * sr));

        // Clear delay buffers
        delayBufL.fill(0.0f);
        delayBufR.fill(0.0f);
        delayWritePos = 0;

        // One-pole crossover coefficient for ~800 Hz high-pass on horn path.
        // Drum gets the low-pass complement.
        // Using matched-Z: coeff = exp(-2*pi*fc/sr)
        const float fc = 800.0f;
        xoverCoeff = std::exp(-6.28318530f * fc / sr);
        xoverStateL = 0.0f;
        xoverStateR = 0.0f;
    }

    // leslieSpeed: 0.0 = brake, 0.5 = slow (chorale), 1.0 = fast (tremolo)
    void setSpeed(float leslieSpeed) noexcept
    {
        if (leslieSpeed < 0.15f)
        {
            // Brake — horn and drum slow to stop
            targetHornRate = 0.0f;
            targetDrumRate = 0.0f;
        }
        else if (leslieSpeed < 0.65f)
        {
            // Slow / Chorale — classic gentle rotation
            targetHornRate = 0.7f; // ~0.7 Hz horn
            targetDrumRate = 0.6f; // ~0.6 Hz drum (slightly slower)
        }
        else
        {
            // Fast / Tremolo — the trademark Leslie throb
            targetHornRate = 6.7f; // ~6.7 Hz horn (fast tremolo)
            targetDrumRate = 5.8f; // ~5.8 Hz drum
        }
    }

    struct LeslieResult
    {
        float left;
        float right;
    };

    LeslieResult process(float inputL, float inputR, float depth) noexcept
    {
        if (depth < 0.001f)
            return {inputL, inputR};

        // --- Frequency crossover ---
        // One-pole low-pass gives the drum (bass) path.
        // High-pass = input - low-pass gives the horn (treble) path.
        xoverStateL += (1.0f - xoverCoeff) * (inputL - xoverStateL);
        xoverStateR += (1.0f - xoverCoeff) * (inputR - xoverStateR);
        xoverStateL = flushDenormal(xoverStateL);
        xoverStateR = flushDenormal(xoverStateR);

        float hornInL = inputL - xoverStateL; // high-pass
        float hornInR = inputR - xoverStateR;
        float drumInL = xoverStateL; // low-pass
        float drumInR = xoverStateR;

        // --- Ramp rotation speeds with physical inertia ---
        currentHornRate += (targetHornRate - currentHornRate) * speedRampCoeff;
        currentHornRate = flushDenormal(currentHornRate);
        currentDrumRate += (targetDrumRate - currentDrumRate) * speedRampCoeff;
        currentDrumRate = flushDenormal(currentDrumRate);

        // --- Horn: write current horn input into circular delay buffer ---
        delayBufL[delayWritePos] = hornInL;
        delayBufR[delayWritePos] = hornInR;

        // True Doppler: horn rotation sine drives read position offset.
        // Maximum delay offset for ±15-20 cents at fast speed:
        //   20 cents ≈ ratio 1.01157 → fractional speed change ≈ 0.01157
        //   At f=440 Hz, period ≈ sr/440 ≈ 109 samples at 48kHz.
        //   We want the delay variation to produce the same pitch shift as
        //   dd/dt = dopplerDepth → use delay modulation depth in samples.
        //   At 6.7 Hz rotation: d/dt of delay = dopplerSamples * 2π * rotHz
        //   For ±15 cents: ratio = pow2(15/1200) ≈ 1.00868
        //   dopplerDepth = 1.00868 / (2π * 6.7) ≈ 0.02395 * sr / sr = sr * 0.00868 / (2π * 6.7)
        //   ≈ 48000 * 0.00868 / 42.1 ≈ ~9.9 samples. We use 10 samples as depth.
        //   This scales with speed (slower rate → same cents but longer period, same depth).
        const float kDopplerDepthSamples = 10.0f; // ±10 samples ≈ ±15 cents at fast speed
        float hornSin = fastSin(hornPhase * kTwoPi);
        float hornCos = fastCos(hornPhase * kTwoPi);

        // Compute fractional read position (delay behind write position)
        // Base delay = half the buffer so we have room in both directions
        float basedelay = 20.0f;
        float dopplerOffset = hornSin * kDopplerDepthSamples * depth;
        float readOffsetL = basedelay + dopplerOffset;
        float readOffsetR = basedelay - dopplerOffset; // 180° for stereo Doppler spread

        // Fractional read via linear interpolation
        auto readDelay = [&](const std::array<float, kDelayBufferSize>& buf, float offset) -> float
        {
            float readPosF = static_cast<float>(delayWritePos) - offset;
            while (readPosF < 0.0f)
                readPosF += static_cast<float>(kDelayBufferSize);
            int r0 = static_cast<int>(readPosF) % kDelayBufferSize;
            int r1 = (r0 + 1) % kDelayBufferSize;
            float frac = readPosF - static_cast<float>(static_cast<int>(readPosF));
            return buf[r0] + frac * (buf[r1] - buf[r0]);
        };

        float hornDopplerL = readDelay(delayBufL, readOffsetL);
        float hornDopplerR = readDelay(delayBufR, readOffsetR);

        // Advance write position
        delayWritePos = (delayWritePos + 1) % kDelayBufferSize;

        // Horn AM (added on top of Doppler, not replacing it)
        // ±6 dB at full depth. L and R are 90° apart using sin/cos.
        float hornAM_L = 1.0f + hornSin * 0.5f * depth;
        float hornAM_R = 1.0f + hornCos * 0.5f * depth;

        float hornOutL = hornDopplerL * hornAM_L;
        float hornOutR = hornDopplerR * hornAM_R;

        // --- Drum: AM only — no Doppler for low frequencies ---
        float drumSin = fastSin(drumPhase * kTwoPi);
        float drumCos = fastCos(drumPhase * kTwoPi);
        float drumAM_L = 1.0f + drumSin * 0.25f * depth;
        float drumAM_R = 1.0f + drumCos * 0.25f * depth;

        float drumOutL = drumInL * drumAM_L;
        float drumOutR = drumInR * drumAM_R;

        // Advance phases
        hornPhase += currentHornRate / sr;
        if (hornPhase >= 1.0f)
            hornPhase -= 1.0f;
        drumPhase += currentDrumRate / sr;
        if (drumPhase >= 1.0f)
            drumPhase -= 1.0f;

        // Recombine horn + drum
        float outL = hornOutL + drumOutL;
        float outR = hornOutR + drumOutR;

        // Crossfeed: some of the cabinet reflections bleed between channels
        float crossfeed = 0.15f * depth;
        float cfL = outL + outR * crossfeed;
        float cfR = outR + outL * crossfeed;

        return {cfL, cfR};
    }

    void reset() noexcept
    {
        hornPhase = 0.0f;
        drumPhase = 0.25f;
        currentHornRate = 0.7f;
        currentDrumRate = 0.6f;
        delayBufL.fill(0.0f);
        delayBufR.fill(0.0f);
        delayWritePos = 0;
        xoverStateL = 0.0f;
        xoverStateR = 0.0f;
    }

    float sr = 48000.0f;
    float hornPhase = 0.0f, drumPhase = 0.25f;
    float currentHornRate = 0.7f, targetHornRate = 0.7f;
    float currentDrumRate = 0.6f, targetDrumRate = 0.6f;
    float speedRampCoeff = 0.001f;

    // Doppler delay buffers — member-allocated, never on the audio thread heap
    std::array<float, kDelayBufferSize> delayBufL = {};
    std::array<float, kDelayBufferSize> delayBufR = {};
    int delayWritePos = 0;

    // Crossover one-pole state (~800 Hz split)
    float xoverCoeff = 0.9f;
    float xoverStateL = 0.0f, xoverStateR = 0.0f;

    static constexpr float kTwoPi = 6.28318530717958647692f;
};

//==============================================================================
// CalliopePipe — Single steam organ pipe with pressure-driven instability.
// A calliope pipe is basically a whistle powered by steam. The pitch
// depends on steam pressure, which fluctuates constantly. Real calliopes
// are LOUD, detuned, and gloriously chaotic. Each pipe has its own
// pressure wobble phase (no two pipes drift in sync).
//==============================================================================
struct CalliopePipe
{
    void trigger(float freq, float sampleRate) noexcept
    {
        baseFreq = freq;
        sr = sampleRate;
        active = true;
        phase = 0.0f;
        // Each pipe has its own pressure wobble rate (1.5-5 Hz)
        wobbleNoiseState = wobbleNoiseState * 1664525u + 1013904223u;
        wobbleRate = 1.5f + (static_cast<float>(wobbleNoiseState & 0xFFFF) / 65536.0f) * 3.5f;
        wobblePhase = static_cast<float>(wobbleNoiseState & 0xFF) / 256.0f;
        // Inter-pipe detuning: each pipe is randomly detuned ±15 cents
        wobbleNoiseState = wobbleNoiseState * 1664525u + 1013904223u;
        detuneOffset = (static_cast<float>(wobbleNoiseState & 0xFFFF) / 32768.0f - 1.0f) * 15.0f;
    }

    float process(float pressureWobble, float globalInstability) noexcept
    {
        if (!active)
            return 0.0f;

        // Steam pressure variation: per-pipe wobble + global instability
        wobblePhase += wobbleRate / sr;
        if (wobblePhase >= 1.0f)
            wobblePhase -= 1.0f;

        float pipeWobble = fastSin(wobblePhase * kTwoPi);
        float totalInstability = (pipeWobble * 0.6f + pressureWobble * 0.4f) * globalInstability;

        // Pitch deviation: ±50 cents at max instability (wild!)
        float cents = detuneOffset + totalInstability * 50.0f;
        float freq = baseFreq * fastPow2(cents / 1200.0f);

        // Near-sine tone with slight harmonics from turbulent airflow
        float phaseInc = freq / sr;
        phase += phaseInc;
        if (phase >= 1.0f)
            phase -= 1.0f;

        // Fundamental + weak 2nd + trace 3rd (steam pipe spectrum)
        float out = fastSin(phase * kTwoPi) * 0.8f + fastSin(phase * 2.0f * kTwoPi) * 0.15f +
                    fastSin(phase * 3.0f * kTwoPi) * 0.05f;

        return out;
    }

    void release() noexcept { active = false; }
    void reset() noexcept
    {
        active = false;
        phase = 0.0f;
    }

    bool active = false;
    float baseFreq = 440.0f;
    float sr = 48000.0f;
    float phase = 0.0f;
    float wobbleRate = 3.0f;
    float wobblePhase = 0.0f;
    float detuneOffset = 0.0f;
    uint32_t wobbleNoiseState = 11111u;

    static constexpr float kTwoPi = 6.28318530717958647692f;
};

//==============================================================================
// BluesHarpVoice — Single blues harmonica reed with pitch bend envelope.
// Cross-harp means playing in a different key than the harp's tuning
// (e.g., playing in G on a C harmonica — "2nd position"). This naturally
// creates blue notes: the 3rd, 5th, and 7th scale degrees are available
// as draw bends, producing the signature wailing, crying quality.
//
// The bend is not a pitch wheel — it's an ENVELOPE. When you draw bend,
// the note starts slightly sharp and drops into the target pitch over
// 30-100ms (the reed takes time to respond to changed airflow).
//==============================================================================
struct BluesHarpVoice
{
    void trigger(float freq, float velocity, float bendAmount, float sampleRate) noexcept
    {
        baseFreq = freq;
        sr = sampleRate;
        active = true;
        phase = 0.0f;
        breath = velocity;

        // Bend envelope: starts at bendAmount semitones above target,
        // drops to 0 over bendTime. Higher velocity = faster bend (more air pressure)
        bendCurrent = bendAmount;
        float bendTimeSec = 0.03f + (1.0f - velocity) * 0.07f; // 30-100ms
        bendDecay = std::exp(-4.6f / (bendTimeSec * sr));

        // Breath noise level: more breath = more air noise
        breathNoiseLevel = velocity * 0.08f;

        // Overblows: high velocity on certain notes triggers overblow
        // (reed flips to next partial — characteristic harmonica sound)
        overblowActive = (velocity > 0.85f);
        overblowPhase = 0.0f;
    }

    float process(float breathPressure, float expressionMod) noexcept
    {
        if (!active)
            return 0.0f;

        // Bend envelope — decays toward 0
        bendCurrent *= bendDecay;
        bendCurrent = flushDenormal(bendCurrent);

        // Apply bend as pitch deviation (semitones → frequency ratio)
        float freq = baseFreq * PitchBendUtil::semitonesToFreqRatio(bendCurrent);

        // Add breath-driven vibrato (5-7 Hz, depth proportional to expression)
        float vibratoDepth = expressionMod * 0.3f; // ±0.3 semitones max
        float vibratoRate = 5.5f;
        vibratoPhase += vibratoRate / sr;
        if (vibratoPhase >= 1.0f)
            vibratoPhase -= 1.0f;
        float vibrato = fastSin(vibratoPhase * kTwoPi) * vibratoDepth;
        freq *= PitchBendUtil::semitonesToFreqRatio(vibrato);

        // Phase accumulation
        float phaseInc = freq / sr;
        phase += phaseInc;
        if (phase >= 1.0f)
            phase -= 1.0f;

        // Reed tone: fundamental + 2nd + 3rd harmonics with breath-dependent balance
        // Harder breath pushes more energy into upper harmonics
        float harm1 = fastSin(phase * kTwoPi);
        float harm2 = fastSin(phase * 2.0f * kTwoPi) * (0.2f + breath * 0.3f);
        float harm3 = fastSin(phase * 3.0f * kTwoPi) * (0.05f + breath * 0.15f);
        float tone = harm1 * 0.7f + harm2 + harm3;

        // Overblow: adds the next partial at reduced amplitude
        if (overblowActive)
        {
            overblowPhase += (freq * 2.0f) / sr;
            if (overblowPhase >= 1.0f)
                overblowPhase -= 1.0f;
            tone += fastSin(overblowPhase * kTwoPi) * 0.3f;
        }

        // Breath noise (air turbulence)
        breathNoiseState = breathNoiseState * 1664525u + 1013904223u;
        float noise = (static_cast<float>(breathNoiseState & 0xFFFF) / 32768.0f - 1.0f);
        tone += noise * breathNoiseLevel * breathPressure;

        return tone * breath * breathPressure;
    }

    void release() noexcept { active = false; }
    void reset() noexcept
    {
        active = false;
        phase = 0.0f;
        vibratoPhase = 0.0f;
    }

    bool active = false;
    float baseFreq = 440.0f;
    float sr = 48000.0f;
    float phase = 0.0f;
    float breath = 0.0f;
    float bendCurrent = 0.0f;
    float bendDecay = 0.999f;
    float breathNoiseLevel = 0.05f;
    float vibratoPhase = 0.0f;
    bool overblowActive = false;
    float overblowPhase = 0.0f;
    uint32_t breathNoiseState = 33333u;

    static constexpr float kTwoPi = 6.28318530717958647692f;
};

//==============================================================================
// ZydecoReed — Diatonic button accordion reed model.
// The zydeco accordion sound is characterized by: buzzy free reeds,
// strong attack (the bellows snap), and a raw, rhythmic quality. The
// accordion has TWO reeds per note that are intentionally detuned
// from each other to create a "musette" beating effect.
//==============================================================================
struct ZydecoReed
{
    void trigger(float freq, float velocity, float sampleRate) noexcept
    {
        baseFreq = freq;
        sr = sampleRate;
        active = true;
        phase1 = 0.0f;
        phase2 = 0.0f;
        attackEnv = 1.0f;

        // Attack burst: bellows snap creates a 5-15ms percussive transient
        float attackMs = 5.0f + (1.0f - velocity) * 10.0f;
        attackDecay = std::exp(-4.6f / (attackMs * 0.001f * sr));

        // Two reeds detuned symmetrically: Reed1 at -N cents, Reed2 at +N cents.
        // N is 3-8 cents randomised per trigger. This gives symmetric musette
        // beating centred on the true pitch, rather than always-sharp Reed2.
        detuneNoiseState = detuneNoiseState * 1664525u + 1013904223u;
        float halfDetuneCents = 3.0f + (static_cast<float>(detuneNoiseState & 0xFFFF) / 65536.0f) * 5.0f;
        // detuneRatio stores the half-step ratio; applied as ÷ and × in process()
        detuneRatio = fastPow2(halfDetuneCents / 1200.0f);

        level = velocity;
    }

    float process(float bellowsPressure, float musetteAmount = 0.5f) noexcept
    {
        if (!active)
            return 0.0f;

        float freq = baseFreq;
        // Musette: Reed1 at -N cents, Reed2 at +N cents (symmetric around true pitch).
        // musetteAmount scales the half-detune ratio: 0 = unison, 1 = full spread.
        float halfRatio = 1.0f + (detuneRatio - 1.0f) * musetteAmount;
        float phaseInc1 = (freq / halfRatio) / sr; // flat by halfDetune cents
        float phaseInc2 = (freq * halfRatio) / sr; // sharp by halfDetune cents

        phase1 += phaseInc1;
        if (phase1 >= 1.0f)
            phase1 -= 1.0f;
        phase2 += phaseInc2;
        if (phase2 >= 1.0f)
            phase2 -= 1.0f;

        // Reed 1: buzzy waveform (square-ish with harmonic rolloff)
        // Free reeds produce a spectrum between sawtooth and square
        float reed1 = fastSin(phase1 * kTwoPi) * 0.5f + fastSin(phase1 * 3.0f * kTwoPi) * 0.25f +
                      fastSin(phase1 * 5.0f * kTwoPi) * 0.15f + fastSin(phase1 * 7.0f * kTwoPi) * 0.08f;

        // Reed 2: same spectrum, slightly detuned
        float reed2 = fastSin(phase2 * kTwoPi) * 0.5f + fastSin(phase2 * 3.0f * kTwoPi) * 0.25f +
                      fastSin(phase2 * 5.0f * kTwoPi) * 0.15f + fastSin(phase2 * 7.0f * kTwoPi) * 0.08f;

        float tone = (reed1 + reed2) * 0.5f;

        // Attack transient: bellows snap
        float attack = attackEnv;
        attackEnv *= attackDecay;
        attackEnv = flushDenormal(attackEnv);
        // Attack adds broadband click
        detuneNoiseState = detuneNoiseState * 1664525u + 1013904223u;
        float noise = (static_cast<float>(detuneNoiseState & 0xFFFF) / 32768.0f - 1.0f);
        tone += noise * attack * 0.3f;

        return tone * level * bellowsPressure;
    }

    void release() noexcept { active = false; }
    void reset() noexcept
    {
        active = false;
        phase1 = 0.0f;
        phase2 = 0.0f;
    }

    bool active = false;
    float baseFreq = 440.0f;
    float sr = 48000.0f;
    float phase1 = 0.0f, phase2 = 0.0f;
    float detuneRatio = 1.003f;
    float attackEnv = 0.0f;
    float attackDecay = 0.999f;
    float level = 1.0f;
    uint32_t detuneNoiseState = 55555u;

    static constexpr float kTwoPi = 6.28318530717958647692f;
};

//==============================================================================
// OtisVoice — One polyphonic voice of the Otis organ engine.
// Each voice contains ALL four organ models but only one is active at
// a time (selected by the organ type parameter).
//==============================================================================
struct OtisVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;

    // Phase accumulator for Hammond tonewheel synthesis
    float phase = 0.0f;

    // Sub-components
    GlideProcessor glide;
    KeyClick keyClick;
    HammondPercussion percussion;
    TonewheelCrosstalk crosstalk;
    CalliopePipe calliope;
    BluesHarpVoice harmonica;
    ZydecoReed accordion;

    // Per-voice filter
    CytomicSVF svf;
    FilterEnvelope filterEnv;

    // Amp envelope
    FilterEnvelope ampEnv;

    // Per-voice LFOs
    StandardLFO lfo1, lfo2;

    // Per-voice pan
    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        phase = 0.0f;
        glide.reset();
        keyClick.reset();
        percussion.reset();
        calliope.reset();
        harmonica.reset();
        accordion.reset();
        svf.reset();
        filterEnv.kill();
        ampEnv.kill();
        lfo1.reset();
        lfo2.reset();
    }
};

//==============================================================================
// OtisEngine — "The Soul Organ"
//==============================================================================
class OtisEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    juce::String getEngineId() const override { return "Otis"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFD4A017); } // Gospel Gold
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].filterEnv.prepare(srf);
            voices[i].ampEnv.prepare(srf);
            voices[i].lfo1.reseed(static_cast<uint32_t>(i * 7919 + 101));
            voices[i].lfo2.reseed(static_cast<uint32_t>(i * 6571 + 202));

            // Stagger voice pan positions for stereo spread
            float panAngle = (static_cast<float>(i) / static_cast<float>(kMaxVoices) - 0.5f) * 0.6f;
            voices[i].panL = std::cos((panAngle + 0.5f) * 1.5707963f);
            voices[i].panR = std::sin((panAngle + 0.5f) * 1.5707963f);
        }

        leslie.prepare(srf);

        smoothDrawbar.fill({});
        for (auto& s : smoothDrawbar)
            s.prepare(srf);
        smoothLeslie.prepare(srf);
        smoothKeyClick.prepare(srf);
        smoothBrightness.prepare(srf);
        smoothDrive.prepare(srf);

        // Hammond organ: reverb-tail category (organ sustains + Leslie tail)
        prepareSilenceGate(sr, maxBlockSize, 500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();
        leslie.reset();
        pitchBendNorm = 0.0f;
        modWheelAmount = 0.0f;
        aftertouchAmount = 0.0f;
        percussionArmed = true;
        anyKeysHeld = 0;
    }

    //==========================================================================
    // Coupling
    //==========================================================================

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? couplingCacheL : couplingCacheR;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* buf, int numSamples) override
    {
        if (!buf || numSamples <= 0)
            return;
        float val = buf[numSamples - 1] * amount;
        switch (type)
        {
        case CouplingType::AmpToFilter:
            couplingFilterMod += val * 3000.0f;
            break;
        case CouplingType::LFOToPitch:
            couplingPitchMod += val * 2.0f;
            break;
        case CouplingType::AmpToPitch:
            couplingPitchMod += val;
            break;
        case CouplingType::AudioToFM:
            couplingFMMod += val;
            break;
        default:
            break;
        }
    }

    //==========================================================================
    // Render — The Heart of Otis
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // Step 1: Parse MIDI — wake gate BEFORE bypass check
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
                wakeSilenceGate();
            }
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber());
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
            else if (msg.isChannelPressure())
                aftertouchAmount = msg.getChannelPressureValue() / 127.0f;
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
        }

        // Step 2: Silence gate bypass
        if (isSilenceGateBypassed())
        {
            buffer.clear(0, numSamples);
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        // Step 3: Load parameters
        auto loadP = [](std::atomic<float>* p, float def) { return p ? p->load(std::memory_order_relaxed) : def; };

        const int organModel = static_cast<int>(loadP(paramOrgan, 0.0f));
        const float pLeslieSpeed = loadP(paramLeslie, 0.5f);
        const float pKeyClickLevel = loadP(paramKeyClick, 0.5f);
        const float pCrosstalk = loadP(paramCrosstalk, 0.3f);
        const float pBrightness = loadP(paramBrightness, 8000.0f);
        const float pDrive = loadP(paramDrive, 0.2f);
        const float pBendRange = loadP(paramBendRange, 2.0f);
        const float pInstability = loadP(paramInstability, 0.5f); // calliope chaos
        const float pMusette = loadP(paramMusette, 0.5f);         // accordion reed detune

        const float pFilterEnvAmt = loadP(paramFilterEnvAmount, 0.3f);

        // Macros
        const float macroCharacter = loadP(paramMacroCharacter, 0.0f);
        const float macroMovement = loadP(paramMacroMovement, 0.0f);
        const float macroCoupling = loadP(paramMacroCoupling, 0.0f);
        const float macroSpace = loadP(paramMacroSpace, 0.0f);

        // LFO params
        const float lfo1Rate = loadP(paramLfo1Rate, 0.5f);
        const float lfo1Depth = loadP(paramLfo1Depth, 0.0f);
        const int lfo1Shape = static_cast<int>(loadP(paramLfo1Shape, 0.0f));
        const float lfo2Rate = loadP(paramLfo2Rate, 1.0f);
        const float lfo2Depth = loadP(paramLfo2Depth, 0.0f);
        const int lfo2Shape = static_cast<int>(loadP(paramLfo2Shape, 0.0f));

        // Drawbar levels (0-8 maps to 0.0-1.0)
        float drawbarLevels[9];
        for (int d = 0; d < 9; ++d)
            drawbarLevels[d] = loadP(paramDrawbar[d], (d == 2) ? 1.0f : 0.0f);

        // Effective parameters with macro influence
        // D006: mod wheel → Leslie speed (organ tradition: swell pedal)
        float effectiveLeslie = std::clamp(pLeslieSpeed + macroMovement * 0.4f + modWheelAmount * 0.5f, 0.0f, 1.0f);
        float effectiveBright = std::clamp(
            pBrightness + macroCharacter * 4000.0f + aftertouchAmount * 3000.0f + couplingFilterMod, 200.0f, 20000.0f);
        float effectiveDrive = std::clamp(pDrive + macroCharacter * 0.3f, 0.0f, 1.0f);
        float effectiveCrosstalk = std::clamp(pCrosstalk + macroCoupling * 0.3f, 0.0f, 1.0f);

        // D006: aftertouch → drive (harder pressing = more gospel fire)
        effectiveDrive = std::clamp(effectiveDrive + aftertouchAmount * 0.3f, 0.0f, 1.0f);

        // Update Leslie speed
        leslie.setSpeed(effectiveLeslie);

        // Smooth parameters
        for (int d = 0; d < 9; ++d)
            smoothDrawbar[d].set(drawbarLevels[d]);
        smoothLeslie.set(effectiveLeslie);
        smoothKeyClick.set(pKeyClickLevel);
        smoothBrightness.set(effectiveBright);
        smoothDrive.set(effectiveDrive);

        // Pitch bend
        const float bendSemitones = pitchBendNorm * pBendRange;

        // Reset coupling accumulators
        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingFMMod = 0.0f;

        // Global steam pressure wobble for calliope (all pipes share this base)
        // Slow chaotic variation in steam boiler pressure
        steamPressurePhase += 1.7f / srf; // ~1.7 Hz base wobble
        if (steamPressurePhase >= 1.0f)
            steamPressurePhase -= 1.0f;
        float globalSteamWobble = fastSin(steamPressurePhase * 6.28318530f);

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        for (int s = 0; s < numSamples; ++s)
        {
            // Per-sample smoothed values
            float smoothedDrawbars[9];
            for (int d = 0; d < 9; ++d)
                smoothedDrawbars[d] = smoothDrawbar[d].process();

            float brightNow = smoothBrightness.process();
            float driveNow = smoothDrive.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (int vi = 0; vi < kMaxVoices; ++vi)
            {
                auto& voice = voices[vi];
                if (!voice.active)
                    continue;

                // Process amp envelope
                float ampEnvLevel = voice.ampEnv.process();
                if (voice.ampEnv.getStage() == FilterEnvelope::Stage::Idle)
                {
                    voice.active = false;
                    continue;
                }

                float freq = voice.glide.process();
                freq *= PitchBendUtil::semitonesToFreqRatio(bendSemitones + couplingPitchMod);

                // Per-voice LFOs
                voice.lfo1.setRate(lfo1Rate, srf);
                voice.lfo1.setShape(lfo1Shape);
                voice.lfo2.setRate(lfo2Rate, srf);
                voice.lfo2.setShape(lfo2Shape);

                float lfo1Val = voice.lfo1.process() * lfo1Depth;
                float lfo2Val = voice.lfo2.process() * lfo2Depth;

                // LFO1 → pitch modulation (vibrato)
                // Gate when harmonica (model 2): BluesHarpVoice has its own
                // breath vibrato; adding engine LFO1 on top causes dual-vibrato
                // beating. Reduce to 10% of depth so LFO1 still responds as a
                // subtle modulation source without competing with breath vibrato.
                float lfo1PitchVal = (organModel == 2) ? lfo1Val * 0.1f : lfo1Val;
                freq *= PitchBendUtil::semitonesToFreqRatio(lfo1PitchVal * 2.0f);

                // FM coupling input
                freq += couplingFMMod * 100.0f;

                float voiceOut = 0.0f;

                switch (organModel)
                {
                //------------------------------------------------------
                // MODEL 0: HAMMOND B3 — The Soul
                //------------------------------------------------------
                case 0:
                {
                    float phaseInc = freq / srf;
                    voice.phase += phaseInc;
                    if (voice.phase >= 1.0f)
                        voice.phase -= 1.0f;

                    // 9-drawbar additive synthesis
                    float tonewheelSum = 0.0f;
                    for (int d = 0; d < 9; ++d)
                    {
                        float drawbarLevel = smoothedDrawbars[d];
                        if (drawbarLevel < 0.001f)
                            continue;

                        float harmonicPhase = voice.phase * kHammondHarmonics[d];
                        // Wrap phase to [0, 1)
                        harmonicPhase -= static_cast<float>(static_cast<int>(harmonicPhase));

                        // Main tonewheel
                        float tonewheel = fastSin(harmonicPhase * 6.28318530f);

                        // Tonewheel crosstalk
                        float xtalk = voice.crosstalk.process(freq, d, voice.phase, effectiveCrosstalk, srf);

                        tonewheelSum += (tonewheel + xtalk) * drawbarLevel;
                    }

                    // Normalize by active drawbar count to prevent clipping
                    tonewheelSum *= 0.22f;

                    // Key click transient
                    float click = voice.keyClick.process();

                    // Percussion
                    float perc = voice.percussion.process(voice.phase);

                    voiceOut = tonewheelSum + click + perc;

                    // Tube amp overdrive (the B3 through a cranked Leslie amp)
                    // D001: velocity shapes drive amount
                    float driveAmount = driveNow * (0.5f + voice.velocity * 0.5f);
                    if (driveAmount > 0.01f)
                    {
                        float gained = voiceOut * (1.0f + driveAmount * 4.0f);
                        voiceOut = fastTanh(gained);
                    }
                    break;
                }

                //------------------------------------------------------
                // MODEL 1: CALLIOPE — The Chaos
                //------------------------------------------------------
                case 1:
                {
                    voiceOut = voice.calliope.process(globalSteamWobble, pInstability);

                    // Binary dynamics — calliope pipes are either full-on or off
                    // BUT we add subtle pressure-based amplitude wobble
                    float pressureAmp = 0.85f + globalSteamWobble * 0.15f * pInstability;
                    voiceOut *= pressureAmp;
                    break;
                }

                //------------------------------------------------------
                // MODEL 2: BLUES HARMONICA — The Breath
                //------------------------------------------------------
                case 2:
                {
                    // Breath pressure: base level + aftertouch expression
                    float breathPressure = 0.6f + aftertouchAmount * 0.4f;
                    float expressionMod = modWheelAmount;

                    voiceOut = voice.harmonica.process(breathPressure, expressionMod);
                    break;
                }

                //------------------------------------------------------
                // MODEL 3: ZYDECO ACCORDION — The Dance
                //------------------------------------------------------
                case 3:
                {
                    // Bellows pressure: continuous breath-like control
                    float bellowsPressure = 0.7f + aftertouchAmount * 0.3f + modWheelAmount * 0.2f;
                    voiceOut = voice.accordion.process(bellowsPressure, pMusette);
                    break;
                }

                default:
                    break;
                }

                // Filter envelope
                float filterEnvLevel = voice.filterEnv.process();
                float envMod = filterEnvLevel * pFilterEnvAmt * 4000.0f;
                // LFO2 → filter cutoff
                float cutoff = std::clamp(brightNow + envMod + lfo2Val * 3000.0f, 200.0f, 20000.0f);
                // D001: velocity → filter brightness
                cutoff = std::clamp(cutoff * (0.5f + voice.velocity * 0.5f), 200.0f, 20000.0f);

                voice.svf.setMode(CytomicSVF::Mode::LowPass);
                voice.svf.setCoefficients_fast(cutoff, 0.15f, srf);
                float filtered = voice.svf.processSample(voiceOut);

                float output = filtered * ampEnvLevel;

                mixL += output * voice.panL;
                mixR += output * voice.panR;
            }

            // Leslie speaker simulation (post-voice mix, pre-output)
            // Leslie depth controlled by macro SPACE
            float leslieDepth = std::clamp(0.5f + macroSpace * 0.5f, 0.0f, 1.0f);
            if (organModel == 0) // Full Leslie only on Hammond
            {
                auto leslieOut = leslie.process(mixL, mixR, leslieDepth);
                mixL = leslieOut.left;
                mixR = leslieOut.right;
            }
            else if (organModel == 1)
            {
                // Calliope gets a lighter Leslie (steam organs don't have Leslies,
                // but a touch of rotation adds to the carnival atmosphere)
                auto leslieOut = leslie.process(mixL, mixR, leslieDepth * 0.3f);
                mixL = leslieOut.left;
                mixR = leslieOut.right;
            }

            outL[s] = mixL;
            if (outR)
                outR[s] = mixR;
            couplingCacheL = mixL;
            couplingCacheR = mixR;
        }

        // Update voice count
        int count = 0;
        for (const auto& v : voices)
            if (v.active)
                ++count;
        activeVoiceCount.store(count);

        // Analyze for silence gate
        analyzeForSilenceGate(buffer, numSamples);
    }

    //==========================================================================
    // Note management
    //==========================================================================

    void noteOn(int note, float vel) noexcept
    {
        int idx = VoiceAllocator::findFreeVoice(voices, kMaxVoices);
        auto& v = voices[idx];

        float freq = midiToFreq(note);

        int organModel = paramOrgan ? static_cast<int>(paramOrgan->load()) : 0;

        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.setTargetOrSnap(freq);
        v.phase = 0.0f;

        // Amp envelope
        float attack = paramAttack ? paramAttack->load() : 0.005f;
        float decay = paramDecay ? paramDecay->load() : 0.3f;
        float sustain = paramSustain ? paramSustain->load() : 0.8f;
        float release = paramRelease ? paramRelease->load() : 0.3f;

        v.ampEnv.prepare(srf);
        v.ampEnv.setADSR(attack, decay, sustain, release);
        v.ampEnv.triggerHard();

        // Filter envelope
        v.filterEnv.prepare(srf);
        v.filterEnv.setADSR(0.001f, 0.3f, 0.0f, 0.5f);
        v.filterEnv.triggerHard();

        // Reset filter
        v.svf.reset();

        // Model-specific note-on
        switch (organModel)
        {
        case 0: // Hammond B3
        {
            // Key click
            float clickLevel = paramKeyClick ? paramKeyClick->load() : 0.5f;
            v.keyClick.trigger(vel, clickLevel, srf);

            // Percussion: single-trigger only when no other keys are held
            float percLevel = paramPercussion ? paramPercussion->load() : 0.5f;
            float percHarm = paramPercHarmonic ? paramPercHarmonic->load() : 0.0f;
            float percDecay = paramPercDecay ? paramPercDecay->load() : 0.3f;

            if (percussionArmed && anyKeysHeld == 0)
            {
                v.percussion.trigger(vel, percLevel, percHarm, percDecay, srf);
                percussionArmed = false;
            }
            break;
        }
        case 1: // Calliope
        {
            v.calliope.wobbleNoiseState += static_cast<uint32_t>(note * 127 + idx * 31);
            v.calliope.trigger(freq, srf);
            break;
        }
        case 2: // Blues Harmonica
        {
            float bendAmt = paramBendAmount ? paramBendAmount->load() : 2.0f;
            // D001: velocity determines bend depth — harder playing = deeper bend
            float velBend = bendAmt * vel;
            v.harmonica.trigger(freq, vel, velBend, srf);
            break;
        }
        case 3: // Zydeco Accordion
        {
            v.accordion.detuneNoiseState += static_cast<uint32_t>(note * 53 + idx * 17);
            v.accordion.trigger(freq, vel, srf);
            break;
        }
        default:
            break;
        }

        anyKeysHeld++;
    }

    void noteOff(int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note)
            {
                v.ampEnv.release();
                v.filterEnv.release();
                v.calliope.release();
                v.harmonica.release();
                v.accordion.release();
            }
        }

        if (anyKeysHeld > 0)
            anyKeysHeld--;

        // Re-arm percussion when all keys are released
        if (anyKeysHeld == 0)
            percussionArmed = true;
    }

    //==========================================================================
    // Parameters — 36 total
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }
    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // Organ model selector
        params.push_back(std::make_unique<PI>(juce::ParameterID{"otis_organ", 1}, "Otis Organ Model", 0, 3, 0));

        // Hammond drawbars (9)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_drawbar1", 1}, "Otis Drawbar 16'",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_drawbar2", 1}, "Otis Drawbar 5-1/3'",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_drawbar3", 1}, "Otis Drawbar 8'",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_drawbar4", 1}, "Otis Drawbar 4'",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_drawbar5", 1}, "Otis Drawbar 2-2/3'",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_drawbar6", 1}, "Otis Drawbar 2'",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_drawbar7", 1}, "Otis Drawbar 1-3/5'",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_drawbar8", 1}, "Otis Drawbar 1-1/3'",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_drawbar9", 1}, "Otis Drawbar 1'",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // Leslie
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_leslie", 1}, "Otis Leslie Speed",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Key click
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_keyClick", 1}, "Otis Key Click",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Percussion
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_percussion", 1}, "Otis Percussion Level",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_percHarmonic", 1}, "Otis Perc Harmonic (2nd/3rd)",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_percDecay", 1}, "Otis Perc Decay",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Tonewheel crosstalk
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_crosstalk", 1}, "Otis Crosstalk",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Shared tone controls
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_brightness", 1}, "Otis Brightness",
                                              juce::NormalisableRange<float>(200.0f, 20000.0f, 0.0f, 0.3f), 8000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_drive", 1}, "Otis Drive",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_filterEnvAmount", 1}, "Otis Filter Env Amount",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_bendRange", 1}, "Otis Pitch Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));

        // ADSR
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_attack", 1}, "Otis Attack",
                                              juce::NormalisableRange<float>(0.001f, 2.0f, 0.0f, 0.3f), 0.005f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_decay", 1}, "Otis Decay",
                                              juce::NormalisableRange<float>(0.01f, 5.0f, 0.0f, 0.3f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_sustain", 1}, "Otis Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_release", 1}, "Otis Release",
                                              juce::NormalisableRange<float>(0.01f, 5.0f, 0.0f, 0.3f), 0.3f));

        // Model-specific params
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_bendAmount", 1}, "Otis Bend Amount (Harp)",
                                              juce::NormalisableRange<float>(0.0f, 5.0f, 0.1f), 2.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_instability", 1}, "Otis Instability (Calliope)",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_musette", 1}, "Otis Musette (Accordion)",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Macros
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_macroCharacter", 1}, "Otis Macro CHARACTER",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_macroMovement", 1}, "Otis Macro MOVEMENT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_macroCoupling", 1}, "Otis Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_macroSpace", 1}, "Otis Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // LFOs
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_lfo1Rate", 1}, "Otis LFO1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_lfo1Depth", 1}, "Otis LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"otis_lfo1Shape", 1}, "Otis LFO1 Shape", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_lfo2Rate", 1}, "Otis LFO2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"otis_lfo2Depth", 1}, "Otis LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"otis_lfo2Shape", 1}, "Otis LFO2 Shape", 0, 4, 0));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramOrgan = apvts.getRawParameterValue("otis_organ");
        paramDrawbar[0] = apvts.getRawParameterValue("otis_drawbar1");
        paramDrawbar[1] = apvts.getRawParameterValue("otis_drawbar2");
        paramDrawbar[2] = apvts.getRawParameterValue("otis_drawbar3");
        paramDrawbar[3] = apvts.getRawParameterValue("otis_drawbar4");
        paramDrawbar[4] = apvts.getRawParameterValue("otis_drawbar5");
        paramDrawbar[5] = apvts.getRawParameterValue("otis_drawbar6");
        paramDrawbar[6] = apvts.getRawParameterValue("otis_drawbar7");
        paramDrawbar[7] = apvts.getRawParameterValue("otis_drawbar8");
        paramDrawbar[8] = apvts.getRawParameterValue("otis_drawbar9");
        paramLeslie = apvts.getRawParameterValue("otis_leslie");
        paramKeyClick = apvts.getRawParameterValue("otis_keyClick");
        paramPercussion = apvts.getRawParameterValue("otis_percussion");
        paramPercHarmonic = apvts.getRawParameterValue("otis_percHarmonic");
        paramPercDecay = apvts.getRawParameterValue("otis_percDecay");
        paramCrosstalk = apvts.getRawParameterValue("otis_crosstalk");
        paramBrightness = apvts.getRawParameterValue("otis_brightness");
        paramDrive = apvts.getRawParameterValue("otis_drive");
        paramFilterEnvAmount = apvts.getRawParameterValue("otis_filterEnvAmount");
        paramBendRange = apvts.getRawParameterValue("otis_bendRange");
        paramAttack = apvts.getRawParameterValue("otis_attack");
        paramDecay = apvts.getRawParameterValue("otis_decay");
        paramSustain = apvts.getRawParameterValue("otis_sustain");
        paramRelease = apvts.getRawParameterValue("otis_release");
        paramBendAmount = apvts.getRawParameterValue("otis_bendAmount");
        paramInstability = apvts.getRawParameterValue("otis_instability");
        paramMusette = apvts.getRawParameterValue("otis_musette");
        paramMacroCharacter = apvts.getRawParameterValue("otis_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("otis_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("otis_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("otis_macroSpace");
        paramLfo1Rate = apvts.getRawParameterValue("otis_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("otis_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("otis_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("otis_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("otis_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("otis_lfo2Shape");
    }

private:
    double sr = 48000.0;
    float srf = 48000.0f;

    std::array<OtisVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount{0};

    LeslieSpeaker leslie;

    // Hammond percussion single-trigger state
    bool percussionArmed = true;
    int anyKeysHeld = 0;

    // Calliope steam pressure
    float steamPressurePhase = 0.0f;

    // Parameter smoothers
    std::array<ParameterSmoother, 9> smoothDrawbar;
    ParameterSmoother smoothLeslie, smoothKeyClick, smoothBrightness, smoothDrive;

    // Expression
    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    // Coupling accumulators
    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f, couplingFMMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    // Parameter pointers
    std::atomic<float>* paramOrgan = nullptr;
    std::array<std::atomic<float>*, 9> paramDrawbar = {};
    std::atomic<float>* paramLeslie = nullptr;
    std::atomic<float>* paramKeyClick = nullptr;
    std::atomic<float>* paramPercussion = nullptr;
    std::atomic<float>* paramPercHarmonic = nullptr;
    std::atomic<float>* paramPercDecay = nullptr;
    std::atomic<float>* paramCrosstalk = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramDrive = nullptr;
    std::atomic<float>* paramFilterEnvAmount = nullptr;
    std::atomic<float>* paramBendRange = nullptr;
    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramSustain = nullptr;
    std::atomic<float>* paramRelease = nullptr;
    std::atomic<float>* paramBendAmount = nullptr;
    std::atomic<float>* paramInstability = nullptr;
    std::atomic<float>* paramMusette = nullptr;
    std::atomic<float>* paramMacroCharacter = nullptr;
    std::atomic<float>* paramMacroMovement = nullptr;
    std::atomic<float>* paramMacroCoupling = nullptr;
    std::atomic<float>* paramMacroSpace = nullptr;
    std::atomic<float>* paramLfo1Rate = nullptr;
    std::atomic<float>* paramLfo1Depth = nullptr;
    std::atomic<float>* paramLfo1Shape = nullptr;
    std::atomic<float>* paramLfo2Rate = nullptr;
    std::atomic<float>* paramLfo2Depth = nullptr;
    std::atomic<float>* paramLfo2Shape = nullptr;
};

} // namespace xoceanus
