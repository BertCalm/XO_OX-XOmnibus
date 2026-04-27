// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/ShoreSystem/ShoreSystem.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
//
//  XOsprey — OSPREY Engine
//  Turbulence-Modulated Resonator Synthesis
//
//  Gallery code:  OSPREY
//  Accent color:  Azulejo Blue #1B4F8A
//  Param prefix:  osprey_
//
//  -------------------------------------------------------------------------
//  CREATURE IDENTITY (XO_OX Aquatic Mythology)
//  -------------------------------------------------------------------------
//
//  The osprey hunts at the surface — the only raptor that plunges into water
//  to catch fish. It lives at the boundary between air and ocean, between
//  feliX's sky and Oscar's deep. XOsprey transforms that boundary into an
//  instrument: the turbulent interface where wind meets wave, where resonance
//  is born from chaos.
//
//  In the water column, XOsprey inhabits the Surface zone — where energy
//  from above (wind, light, transients) excites the resonant structures
//  below (modal bodies, creature voices, sympathetic strings). It is the
//  meeting point between the two worlds.
//
//  -------------------------------------------------------------------------
//  SYNTH LINEAGE & TECHNIQUE
//  -------------------------------------------------------------------------
//
//  XOsprey channels three synthesis traditions:
//
//  1. MODAL SYNTHESIS (Modalys / IRCAM, 1990s)
//     Physical models of resonant bodies — struck, bowed, plucked — reduced
//     to banks of tuned 2-pole resonant filters. Each resonator models one
//     partial of an acoustic instrument (guitar, kora, koto, etc.), excited
//     by fluid energy rather than a physical gesture.
//
//  2. SPECTRAL PROCESSING (SPEAR / AudioSculpt)
//     The ShoreSystem morphs between 5 coastal resonator profiles, each
//     representing a family of acoustic instruments from that region. This
//     is spectral morphing — the timbral DNA of a kora smoothly becoming
//     the DNA of a koto as the Shore knob sweeps from Atlantic to Pacific.
//
//  3. GRANULAR / STOCHASTIC EXCITATION (Xenakis, 1960s)
//     The FluidEnergyModel replaces traditional oscillators with a
//     physics-inspired noise generator. At low sea states it produces
//     smooth sinusoidal swells; at high sea states it layers 4 octaves
//     of Perlin-style noise, modeling the energy cascade from large
//     ocean waves down to surface chop — stochastic synthesis shaped
//     by fluid dynamics.
//
//  -------------------------------------------------------------------------
//  ARCHITECTURE OVERVIEW
//  -------------------------------------------------------------------------
//
//  Signal flow:
//
//    FluidEnergyModel (global, 1 instance)
//         |
//         v
//    Per Voice (x8 polyphonic, LRU stealing, 5ms crossfade):
//      Excitation -> 16 Modal Resonators
//                    [3 instrument groups x 4 formants + 4 sympathetic]
//                 -> Creature Formant Voices (3 per voice)
//                 -> DC Blocker -> Soft Limiter -> Amp Env -> Pan
//         |
//         v
//    Stereo Mix -> Tilt Filter -> Foam -> Brine -> Hull -> Fog
//              -> Harbor Verb (4-allpass chain)
//              -> Output + Coupling Cache
//
//  The ShoreSystem provides 5 coastal regions (Atlantic, Nordic,
//  Mediterranean, Pacific, Southern) with continuous morphing. Each shore
//  defines resonator spectral profiles, creature voice targets, and fluid
//  character.
//
//  Coupling:
//    Output:  post-limiter stereo audio via getSampleForCoupling
//    Input:   AudioToFM (resonator excitation), AmpToFilter (sea state),
//             EnvToMorph (swell period), LFOToPitch (resonator tuning),
//             AudioToWavetable (replaces excitation source)
//
//  Parameters: 25 (11 core + 6 character + 4 envelope + 4 macros)
//  CPU budget: <10% single-engine @ 44100Hz, 512 block on M1
//
//==============================================================================

//==============================================================================
//  SECTION 3: Modal Resonator
//  -------------------------------------------------------------------------
//  A single tuned resonator modeled as a 2-pole resonant filter — the
//  fundamental building block of modal synthesis (IRCAM Modalys, 1990s).
//
//  Each resonator models one partial of an acoustic body. The filter's
//  transfer function is:
//
//      y[n] = b0 * x[n] - a1 * y[n-1] - a2 * y[n-2]
//
//  Where:
//      w0 = 2pi * freq / sampleRate     (normalized angular frequency)
//      r  = exp(-pi * bw / sampleRate)   (pole radius = decay rate)
//      a1 = -2r * cos(w0)                (resonant frequency placement)
//      a2 = r^2                          (decay envelope)
//      b0 = (1 - r^2) * gain             (input scaling for unity peak)
//
//  16 resonators per voice: 3 instrument groups x 4 formants + 4 sympathetic.
//==============================================================================
struct OspreyResonator
{
    // --- Configuration ---
    float centerFrequency = 440.0f; // Hz — resonant peak frequency
    float bandwidth = 100.0f;       // Hz — 3dB bandwidth (controls decay time)
    float gain = 1.0f;              // amplitude scaling

    // --- Filter state (2 delay elements) ---
    float state1 = 0.0f; // y[n-1]
    float state2 = 0.0f; // y[n-2]

    // --- Filter coefficients ---
    float coeffA1 = 0.0f; // feedback coeff (frequency)
    float coeffA2 = 0.0f; // feedback coeff (decay)
    float coeffB0 = 0.0f; // input gain

    void setCoefficients(float freq, float bw, float g, float sampleRate) noexcept
    {
        centerFrequency = freq;
        bandwidth = bw;
        gain = g;

        // Normalized angular frequency
        float w0 = 2.0f * kPi * freq / sampleRate;

        // Pole radius: controls how long the resonator rings.
        // r -> 1.0 = infinite ring (narrow bandwidth),
        // r -> 0.0 = instant decay (wide bandwidth).
        float poleRadius = fastExp(-kPi * bw / sampleRate);

        coeffA1 = -2.0f * poleRadius * fastCos(w0);
        coeffA2 = poleRadius * poleRadius;

        // Input gain normalized so peak response = gain parameter
        coeffB0 = (1.0f - poleRadius * poleRadius) * g;
    }

    float process(float excitation) noexcept
    {
        float out = coeffB0 * excitation - coeffA1 * state1 - coeffA2 * state2;
        state2 = state1;
        // Flush denormals on state to prevent CPU spikes from subnormal
        // float arithmetic in feedback paths. Without this, a decaying
        // resonator can produce denormal values that are 10-100x slower
        // to process on x86/ARM hardware.
        state1 = flushDenormal(out);
        return out;
    }

    void reset() noexcept
    {
        state1 = 0.0f;
        state2 = 0.0f;
    }

private:
    static constexpr float kPi = 3.14159265358979323846f;
};

//==============================================================================
//  SECTION 4: Creature Formant Generator
//  -------------------------------------------------------------------------
//  Generates organic creature voice sounds — bird calls, whale songs,
//  coastal ambient textures — using sweeping formant filters. Each creature
//  voice has 3 bandpass formant bands that sweep from start to end
//  frequencies over a configurable duration, then pause during a gap
//  period before probabilistically re-triggering.
//
//  This models the vocalization patterns of marine and coastal fauna:
//  a storm petrel's call sweeps rapidly through its formants (short
//  sweepMs), while a humpback song glides slowly (long sweepMs) with
//  extended pauses (long gapMs). The ShoreSystem provides regionally
//  appropriate creature voice targets for each coastline.
//==============================================================================
struct CreatureFormant
{
    // --- Sweep state ---
    float sweepPhase = 0.0f;         // current position in formant sweep [0,1)
    float sweepDurationMs = 1000.0f; // how long one vocalization lasts
    float gapDurationMs = 5000.0f;   // silence between vocalizations
    bool active = false;
    float amplitude = 0.5f;

    // --- Formant frequency targets (3 bands) ---
    float startFrequencies[3] = {};  // Hz — formant positions at sweep start
    float endFrequencies[3] = {};    // Hz — formant positions at sweep end
    float formantBandwidths[3] = {}; // Hz — formant widths

    // --- Filter bank (3 CytomicSVF bandpass filters) ---
    CytomicSVF formantFilters[3];

    // --- Timing ---
    float sweepPhaseIncrement = 0.0f; // per-sample phase advance during sweep
    float gapCounter = 0.0f;          // current position in gap [0,1)
    float gapPhaseIncrement = 0.0f;   // per-sample phase advance during gap
    bool inGap = true;

    // --- PRNG (LCG, Knuth TAOCP constants) ---
    // FIX P36: pointer-hash default so each CreatureFormant instance (3 per voice × N voices)
    // produces unique probabilistic retrigger timing. Without this all creature formants
    // across simultaneous voices share the same gap-trigger probability sequence.
    uint32_t randomState = 0xC2B2AE3Du ^ static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this) >> 4);

    float nextRandom() noexcept
    {
        randomState = randomState * 1664525u + 1013904223u;
        return static_cast<float>(randomState & 0xFFFF) / 65536.0f;
    }

    void trigger(float sampleRate) noexcept
    {
        active = true;
        inGap = false;
        sweepPhase = 0.0f;

        // Convert sweep duration (ms) to per-sample phase increment:
        // phaseInc = 1000 / (durationMs * sampleRate) so phase reaches 1.0
        // after exactly durationMs milliseconds.
        sweepPhaseIncrement = 1000.0f / (sweepDurationMs * sampleRate);
        if (sweepPhaseIncrement <= 0.0f)
            sweepPhaseIncrement = 0.001f;

        for (int i = 0; i < 3; ++i)
        {
            formantFilters[i].setMode(CytomicSVF::Mode::BandPass);
            formantFilters[i].setCoefficients(startFrequencies[i], 0.7f, sampleRate);
        }
    }

    void configure(const CreatureVoice& creatureVoice, float sampleRate) noexcept
    {
        sweepDurationMs = creatureVoice.sweepMs;
        gapDurationMs = creatureVoice.gapMs;
        amplitude = creatureVoice.amplitude;

        for (int i = 0; i < 3; ++i)
        {
            startFrequencies[i] = creatureVoice.startFreqs[i];
            endFrequencies[i] = creatureVoice.endFreqs[i];
            formantBandwidths[i] = creatureVoice.bandwidths[i];
        }

        gapPhaseIncrement = 1000.0f / (gapDurationMs * sampleRate);
        if (gapPhaseIncrement <= 0.0f)
            gapPhaseIncrement = 0.0001f;
    }

    float process(float sampleRate, float excitation) noexcept
    {
        if (!active)
            return 0.0f;

        // --- Gap phase: silence between vocalizations ---
        if (inGap)
        {
            gapCounter += gapPhaseIncrement;
            if (gapCounter >= 1.0f)
            {
                gapCounter = 0.0f;
                // 70% chance to re-trigger — models the stochastic nature
                // of animal vocalizations (not perfectly periodic)
                static constexpr float kRetriggerProbability = 0.7f;
                if (nextRandom() < kRetriggerProbability)
                    trigger(sampleRate);
            }
            return 0.0f;
        }

        // --- Sweep phase: interpolate formant frequencies from start to end ---

        // Smoothstep: t^2 * (3 - 2t) — eases in/out for natural vocal quality
        float t = sweepPhase;
        float smoothT = t * t * (3.0f - 2.0f * t);

        float out = 0.0f;
        for (int i = 0; i < 3; ++i)
        {
            float currentFreq = startFrequencies[i] + smoothT * (endFrequencies[i] - startFrequencies[i]);
            currentFreq = clamp(currentFreq, 40.0f, sampleRate * 0.45f);

            // Q derived from center-frequency / bandwidth ratio
            float q = currentFreq / std::max(formantBandwidths[i], 10.0f);
            q = clamp(q, 0.2f, 0.95f);

            formantFilters[i].setCoefficients(currentFreq, q, sampleRate);
            out += formantFilters[i].processSample(excitation);
        }

        sweepPhase += sweepPhaseIncrement;
        if (sweepPhase >= 1.0f)
        {
            // Sweep complete — enter gap (silence between calls)
            inGap = true;
            gapCounter = 0.0f;
            sweepPhase = 0.0f;
        }

        // Normalize by 3 formant bands to prevent amplitude growth
        static constexpr float kFormantNormalization = 1.0f / 3.0f;
        return out * amplitude * kFormantNormalization;
    }

    void reset() noexcept
    {
        sweepPhase = 0.0f;
        active = false;
        inGap = true;
        gapCounter = 0.0f;
        for (int i = 0; i < 3; ++i)
            formantFilters[i].reset();
    }
};

//==============================================================================
//  SECTION 5: Fluid Energy Model
//  -------------------------------------------------------------------------
//  The core excitation generator — replaces traditional oscillators with a
//  physics-inspired energy source modeled on ocean fluid dynamics.
//
//  Inspired by Xenakis's stochastic synthesis (1960s) and Perlin noise
//  (1983), the model layers three energy components:
//
//  1. SWELL: Smooth sinusoidal wave — the dominant low-frequency energy
//     of the open ocean. Period controlled by shore character and user
//     parameter (0.5s–60s).
//
//  2. CHOP: Higher-frequency surface modulation — wind-driven capillary
//     waves riding on top of the swell. Frequency scales with sea state
//     (more wind = faster chop).
//
//  3. TURBULENCE: 4-octave layered noise, modeling the Kolmogorov energy
//     cascade — in real fluid dynamics, energy transfers from large
//     eddies to progressively smaller scales. The 4 octaves at
//     frequencies {0.3, 0.7, 1.5, 3.2} Hz with amplitudes
//     {1.0, 0.5, 0.25, 0.125} approximate the -5/3 spectral slope of
//     fully-developed turbulence. Only active above the turbulenceOnset
//     threshold defined per shore region.
//
//  The fluid model runs once per sample globally (not per-voice). All
//  voices read the same energy stream but process it through differently-
//  tuned resonator banks, creating coherent-yet-varied timbral response.
//==============================================================================
struct FluidEnergyModel
{
    // --- Phase accumulators ---
    float swellPhase = 0.0f; // main swell oscillator [0,1)
    float chopPhase = 0.0f;  // surface chop oscillator [0,1)

    // --- Turbulence state (4 octaves of smoothed random walk) ---
    static constexpr int kTurbulenceOctaves = 4;
    float turbulenceState[kTurbulenceOctaves] = {};

    // --- PRNG (LCG, Knuth TAOCP constants) ---
    // FIX P36: pointer-hash default so each FluidEnergyModel instance (one per engine)
    // starts with a unique turbulence seed, preventing correlated fluid-energy noise
    // when multiple Osprey instances play simultaneously.
    uint32_t randomState = 0xC2B2AE3Du ^ static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this) >> 4);

    float nextRandomBipolar() noexcept
    {
        randomState = randomState * 1664525u + 1013904223u;
        return static_cast<float>(randomState & 0xFFFF) / 32768.0f - 1.0f; // [-1, +1]
    }
    // nextRandomUnipolar removed: FluidEnergyModel only uses bipolar noise.
    // Dead code risked confusion about which PRNG calls advance randomState.

    float process(float seaState, float swellPeriod, const FluidCharacter& fluid, float sampleRate) noexcept
    {
        static constexpr float kTwoPi = 6.28318530717958647692f;

        // --- Swell: smooth sinusoidal ocean energy ---

        // Effective swell period blends user control with shore character.
        // Shore swellPeriodBase ranges ~4-15s; normalizing by 12.0 centers
        // the blend so Atlantic (~8s) and Pacific (~12s) both feel natural.
        float effectivePeriod = swellPeriod * (0.5f + 0.5f * fluid.swellPeriodBase / 12.0f);
        effectivePeriod = clamp(effectivePeriod, 0.5f, 60.0f);

        float swellIncrement = 1.0f / (effectivePeriod * sampleRate);
        swellPhase += swellIncrement;
        if (swellPhase >= 1.0f)
            swellPhase -= 1.0f;

        float swell = fastSin(swellPhase * kTwoPi) * fluid.swellDepth;

        // --- Chop: wind-driven surface capillary waves ---

        // Chop frequency rises with sea state (more wind = faster ripples).
        // The 3.0 multiplier gives ~4x frequency range from calm to storm.
        float chopFrequency = fluid.chopFreqBase * (1.0f + seaState * 3.0f);
        float chopIncrement = chopFrequency / sampleRate;
        chopPhase += chopIncrement;
        if (chopPhase >= 1.0f)
            chopPhase -= 1.0f;

        float chop = fastSin(chopPhase * kTwoPi) * fluid.chopAmount * seaState;

        // --- Turbulence: multi-octave noise (Kolmogorov cascade) ---

        float turbulence = 0.0f;
        float turbulenceAmount = clamp((seaState - fluid.turbulenceOnset) /
                                           (1.0f - fluid.turbulenceOnset + 0.001f), // +0.001 prevents div-by-zero
                                       0.0f, 1.0f);

        if (turbulenceAmount > 0.001f)
        {
            // Octave frequencies (Hz) — ascending scale sizes in the cascade
            static constexpr float kOctaveFrequencies[kTurbulenceOctaves] = {0.3f, 0.7f, 1.5f, 3.2f};
            // Octave amplitudes — approximate -5/3 spectral slope of turbulence
            static constexpr float kOctaveAmplitudes[kTurbulenceOctaves] = {1.0f, 0.5f, 0.25f, 0.125f};

            for (int octave = 0; octave < kTurbulenceOctaves; ++octave)
            {
                // Smooth random walk: one-pole lowpass on white noise.
                // Matched-Z coefficient: 1 - exp(-2π·fc/sr). The previous
                // Euler approximation (fc/sr) severely under-estimated the
                // response at these sub-Hz frequencies, making the cascade
                // nearly inaudible. Matched-Z gives correct time-constants.
                float target = nextRandomBipolar();
                float smoothingRate = 1.0f - fastExp(-6.28318f * kOctaveFrequencies[octave] / sampleRate);
                turbulenceState[octave] += (target - turbulenceState[octave]) * smoothingRate;
                // Flush denormals in the random walk state to prevent CPU
                // spikes from subnormal arithmetic in this feedback path.
                turbulenceState[octave] = flushDenormal(turbulenceState[octave]);
                turbulence += turbulenceState[octave] * kOctaveAmplitudes[octave];
            }

            // 0.5 scaling keeps turbulence in similar range to swell+chop
            turbulence *= turbulenceAmount * 0.5f;
        }

        // --- Combine energy sources ---

        // Depth bias: subsurface energy is attenuated (water absorbs HF).
        // At depthBias=1.0, total energy is halved.
        float depthFactor = 1.0f - fluid.depthBias * 0.5f;

        float energy = (swell + chop + turbulence) * depthFactor;

        // Sea state master scaling: at seaState=0 (dead calm), only 20%
        // of the energy passes through. At seaState=1 (full storm), 100%.
        energy *= (0.2f + seaState * 0.8f);

        return energy;
    }

    void reset() noexcept
    {
        swellPhase = 0.0f;
        chopPhase = 0.0f;
        for (int i = 0; i < kTurbulenceOctaves; ++i)
            turbulenceState[i] = 0.0f;
    }
};

//==============================================================================
//  SECTION 6: Allpass Delay — Harbor Verb Building Block
//  -------------------------------------------------------------------------
//  A first-order allpass delay used to build the Harbor Verb reverb.
//  4 of these in series create a simple diffusion network reminiscent
//  of Schroeder/Moorer reverb topology (1962/1979).
//
//  Transfer function (standard allpass form):
//      buffer[n] = input + g * delayed
//      output    = delayed - g * buffer[n]
//
//  This structure passes all frequencies at unity gain but scrambles
//  their phase relationships, creating the "smeared" quality of reverb.
//
//  Max buffer: 8192 samples — sized for 192kHz operation.
//  At 192kHz the largest scaled delay (1777 * 192000/44100 ≈ 7734) must fit.
//  8192 provides headroom up to ~218kHz.
//==============================================================================
struct AllpassDelay
{
    static constexpr int kMaxBufferSize = 8192;

    float buffer[kMaxBufferSize] = {};
    int writePosition = 0;
    int delaySamples = 1000;
    float feedbackCoeff = 0.5f;

    void setParams(int delay, float feedback) noexcept
    {
        delaySamples = clamp(delay, 1, kMaxBufferSize - 1);
        feedbackCoeff = feedback;
    }

    float process(float input) noexcept
    {
        int readPosition = writePosition - delaySamples;
        if (readPosition < 0)
            readPosition += kMaxBufferSize;

        float delayed = buffer[readPosition];

        // Standard allpass form (Schroeder, 1962):
        // Store input mixed with fed-back delayed signal
        float bufferValue = input + feedbackCoeff * delayed;
        // Output is delayed signal minus fed-forward input
        float out = delayed - feedbackCoeff * bufferValue;

        // Flush denormals in the delay buffer to prevent CPU spikes.
        // Reverb feedback paths are especially vulnerable — a single
        // denormal value recirculates and multiplies through all 4
        // allpass stages, compounding the performance hit.
        buffer[writePosition] = flushDenormal(bufferValue);

        writePosition++;
        if (writePosition >= kMaxBufferSize)
            writePosition = 0;

        return flushDenormal(out);
    }

    void reset() noexcept
    {
        for (int i = 0; i < kMaxBufferSize; ++i)
            buffer[i] = 0.0f;
        writePosition = 0;
    }
};

//==============================================================================
//  SECTION 7: Voice State
//  -------------------------------------------------------------------------
//  Per-MIDI-note state. Each voice contains:
//    - 16 modal resonators (the acoustic "body" of this note)
//    - 3 creature formant generators (organic life layered on top)
//    - Amplitude envelope (ADSR)
//    - Glide/portamento tracking
//    - DC blocker (prevents offset buildup from asymmetric excitation)
//    - Voice-stealing crossfade state (5ms fade for click-free stealing)
//    - Control-rate decimation counter (coefficient updates at ~2kHz)
//    - Coherence tracking (per-resonator phase alignment state)
//==============================================================================
struct OspreyVoice
{
    // --- Voice lifecycle ---
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0; // monotonic counter for LRU stealing

    // --- Pitch tracking ---
    float targetFrequency = 440.0f;       // destination frequency (Hz)
    float currentGlideFrequency = 440.0f; // smoothed frequency for portamento
    float glideCoefficient = 1.0f;        // 1.0 = instant, <1.0 = glide

    // --- Resonator bank ---
    // 16 total: indices 0-3 = instrument group A (4 formants)
    //           indices 4-7 = instrument group B (4 formants)
    //           indices 8-11 = instrument group C (4 formants)
    //           indices 12-15 = sympathetic resonators (harmonics 2,3,4,5)
    static constexpr int kResonatorsPerVoice = 16;
    std::array<OspreyResonator, kResonatorsPerVoice> resonators;

    // --- Creature voice generators ---
    // 3 per voice: typically bird, deep-sea creature, ambient texture
    static constexpr int kCreaturesPerVoice = 3;
    std::array<CreatureFormant, kCreaturesPerVoice> creatures;

    // --- Amplitude envelope ---
    StandardADSR amplitudeEnvelope;

    // --- Voice stealing crossfade ---
    float fadeGain = 1.0f;  // current crossfade level (1.0 = full, 0.0 = silent)
    bool fadingOut = false; // true when this voice is being stolen

    // --- Control-rate decimation ---
    int controlCounter = 0; // counts samples between coefficient updates

    // --- Per-voice PRNG (LCG, Knuth TAOCP constants) ---
    uint32_t randomState = 12345u;

    // --- Stereo pan gains (CPU fix: precomputed at noteOn, constant per voice lifetime) ---
    // Pan position derives from noteNumber % 12 — never changes after voice start.
    float panGainL = 0.7071f;
    float panGainR = 0.7071f;

    // --- DC blocker state (1-pole highpass) ---
    float dcPreviousInputL = 0.0f;
    float dcPreviousOutputL = 0.0f;
    float dcPreviousInputR = 0.0f;
    float dcPreviousOutputR = 0.0f;

    // --- Coherence tracking ---
    // Per-resonator smoothed output for phase alignment control.
    // At high coherence, resonator outputs are gently lowpassed to
    // align their phase relationships (more tonal, less chaotic).
    float coherenceStates[kResonatorsPerVoice] = {};

    // --- PRNG utilities ---

    float nextRandomBipolar() noexcept
    {
        randomState = randomState * 1664525u + 1013904223u;
        return static_cast<float>(randomState & 0xFFFF) / 32768.0f - 1.0f; // [-1, +1]
    }

    float nextRandomUnipolar() noexcept
    {
        randomState = randomState * 1664525u + 1013904223u;
        return static_cast<float>(randomState & 0xFFFF) / 65536.0f; // [0, 1)
    }

    // --- Reset ---

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        targetFrequency = 440.0f;
        currentGlideFrequency = 440.0f;
        glideCoefficient = 1.0f;
        fadeGain = 1.0f;
        fadingOut = false;
        controlCounter = 0;
        randomState = 12345u;

        panGainL = 0.7071f;
        panGainR = 0.7071f;

        dcPreviousInputL = 0.0f;
        dcPreviousOutputL = 0.0f;
        dcPreviousInputR = 0.0f;
        dcPreviousOutputR = 0.0f;

        amplitudeEnvelope.reset();

        for (auto& resonator : resonators)
            resonator.reset();
        for (auto& creature : creatures)
            creature.reset();
        for (int i = 0; i < kResonatorsPerVoice; ++i)
            coherenceStates[i] = 0.0f;
    }
};

//==============================================================================
//  SECTION 8: OspreyEngine — Main Engine Class
//  -------------------------------------------------------------------------
//  The osprey dives. Sound rises from the collision of wind and water.
//
//  Implements the SynthEngine interface for XOceanus integration.
//  All DSP is inline in this header per XO_OX architecture rules.
//==============================================================================
class OspreyEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;
    static constexpr float kPi = 3.14159265358979323846f;
    static constexpr float kTwoPi = 6.28318530717958647692f;
    static constexpr int kNumResonators = 16; // per voice: 3 groups x 4 formants + 4 sympathetic

    //==========================================================================
    // SynthEngine interface — Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRateDouble = sampleRate;
        sampleRateFloat = static_cast<float>(sampleRateDouble);

        // Control-rate decimation: update filter coefficients at ~2kHz
        // instead of every sample. Saves ~80% of coefficient computation
        // with negligible audible difference for slowly-changing parameters.
        controlRateDivisor = std::max(1, static_cast<int>(sampleRateFloat / 2000.0f));
        controlRateDeltaTime = static_cast<float>(controlRateDivisor) / sampleRateFloat;

        // Voice-stealing crossfade: 5ms ramp prevents clicks when
        // stealing a voice. Rate = samples needed to fade from 1.0 to 0.0.
        static constexpr float kCrossfadeTimeSeconds = 0.005f;
        crossfadeRate = 1.0f / std::max(kCrossfadeTimeSeconds * sampleRateFloat, 1e-6f);

        // Output cache for coupling reads (other engines read our output)
        outputCacheL.resize(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR.resize(static_cast<size_t>(maxBlockSize), 0.0f);

        // Initialize all voices
        for (auto& voice : voices)
            voice.reset();

        // Initialize global fluid energy model
        fluidModel.reset();

        // D005/D004 fix: initialize the sea state LFO (sine shape, 0.1 Hz default).
        // This LFO modulates sea state amplitude so the sound breathes organically.
        seaStateLFO.reset();
        seaStateLFO.setShape(0); // Sine shape for smooth wave-like modulation
        seaStateLFO.setRate(0.1f, sampleRateFloat);

        // P22: initialize the brightness LFO so its internal state is valid before
        // the first renderBlock() call. Without this, the LFO may start with random
        // phase or stale coefficients, producing incorrect timbral shimmer on
        // the very first note.
        brightnessLFO.reset();
        brightnessLFO.setShape(1); // Triangle shape default (matches osprey_lfo2Shape default=1)
        brightnessLFO.setRate(0.2f, sampleRateFloat); // matches osprey_lfo2Rate default

        // Harbor verb: allpass delays with prime-number lengths.
        // Primes {1087, 1283, 1511, 1777} chosen to avoid harmonic
        // relationships between delay taps — this prevents comb filtering
        // artifacts and produces smoother diffusion (Moorer, 1979).
        // Lengths are scaled proportionally for sample rates other than 44.1kHz.
        float sampleRateScale = sampleRateFloat / 44100.0f;
        static constexpr int kBaseDelayLengths[4] = {1087, 1283, 1511, 1777};
        for (int i = 0; i < 4; ++i)
        {
            harborVerbAllpass[i].reset();
            int scaledLength = static_cast<int>(static_cast<float>(kBaseDelayLengths[i]) * sampleRateScale);
            harborVerbAllpass[i].setParams(scaledLength, 0.5f);
        }

        // Initialize global post-processing filters
        tiltFilterL.reset();
        tiltFilterR.reset();
        fogFilterL.reset();
        fogFilterR.reset();
        hullFilterL.reset();
        hullFilterR.reset();

        aftertouch.prepare(sampleRate);

        silenceGate.prepare(sampleRate, maxBlockSize);
        silenceGate.setHoldTime(500.0f); // Osprey has harbor reverb tails
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& voice : voices)
            voice.reset();

        fluidModel.reset();

        // Reset coupling accumulators
        peakEnvelopeOutput = 0.0f;
        couplingExcitationMod = 0.0f;
        couplingSeaStateMod = 0.0f;
        couplingSwellPeriodMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingAudioReplaceLevel = 0.0f;
        couplingAudioReplaceActive = false;

        // P14: reset MIDI expression state — stale modWheel/pitchBend survive
        // across DAW transport-reset otherwise, causing incorrect initial sea state
        // and resonator tuning on the next note.
        modWheelAmount = 0.0f;
        pitchBendNorm = 0.0f;
        filterEnvBoost = 0.0f;

        // D005/D004 fix: reset the sea state LFO to prevent stale phase state.
        seaStateLFO.reset();
        seaStateLFO.setShape(0);
        seaStateLFO.setRate(0.1f, sampleRateFloat);

        // P22: reset brightnessLFO — it was absent from reset(), causing the
        // LFO2 phase to persist across transport resets and preset changes.
        brightnessLFO.reset();

        // Reset harbor verb
        for (int i = 0; i < 4; ++i)
            harborVerbAllpass[i].reset();

        // Reset post-processing filters
        tiltFilterL.reset();
        tiltFilterR.reset();
        fogFilterL.reset();
        fogFilterR.reset();
        hullFilterL.reset();
        hullFilterR.reset();

        // Clear coupling output cache
        std::fill(outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill(outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //==========================================================================
    // SynthEngine interface — Audio Rendering
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0)
            return;

        // =================================================================
        //  ParamSnapshot: read all APVTS parameters once per block.
        //  This is the standard XO_OX pattern — cache atomic loads at
        //  block boundaries to avoid per-sample atomic overhead.
        // =================================================================

        // --- Core ocean parameters ---
        const float pShore = loadParam(paramShore, 0.0f);
        const float pSeaState = loadParam(paramSeaState, 0.2f);
        const float pSwellPeriod = loadParam(paramSwellPeriod, 8.0f);
        const float pWindDirection = loadParam(paramWindDir, 0.5f);
        const float pDepth = loadParam(paramDepth, 0.3f);

        // --- Resonator parameters ---
        // Note: effectiveResonatorBright (non-const) — LFO2 modulation applied below.
        float effectiveResonatorBright = loadParam(paramResonatorBright, 0.5f);
        const float pResonatorDecay = loadParam(paramResonatorDecay, 1.0f);
        const float pSympathyAmount = loadParam(paramSympathyAmount, 0.3f);

        // --- Creature parameters ---
        const float pCreatureRate = loadParam(paramCreatureRate, 0.2f);
        const float pCreatureDepth = loadParam(paramCreatureDepth, 0.3f);

        // --- Coherence & character ---
        const float pCoherence = loadParam(paramCoherence, 0.7f);
        const float pFoam = loadParam(paramFoam, 0.0f);
        const float pBrine = loadParam(paramBrine, 0.0f);
        const float pHull = loadParam(paramHull, 0.2f);

        // --- Filter & space ---
        const float pFilterTilt = loadParam(paramFilterTilt, 0.5f);
        const float pHarborVerb = loadParam(paramHarborVerb, 0.2f);
        const float pFog = loadParam(paramFog, 0.1f);

        // --- Amplitude envelope ---
        const float pAmpAttack = loadParam(paramAmpAttack, 0.5f);
        const float pAmpDecay = loadParam(paramAmpDecay, 1.0f);
        const float pAmpSustain = loadParam(paramAmpSustain, 0.7f);
        const float pAmpRelease = loadParam(paramAmpRelease, 2.0f);

        // --- Voice mode and glide (Round 10F) ---
        // 0=Poly (all notes independent), 1=Mono (retrigger), 2=Legato (slide, no retrigger).
        const int pVoiceMode = static_cast<int>(loadParam(paramVoiceMode, 0.0f));
        const float pGlideTime = loadParam(paramGlideTime, 0.0f);

        // --- Macros (M1-M4) ---
        const float macroCharacter = loadParam(paramMacroCharacter, 0.0f);
        const float macroMovement = loadParam(paramMacroMovement, 0.0f);
        const float macroCoupling = loadParam(paramMacroCoupling, 0.0f);
        const float macroSpace = loadParam(paramMacroSpace, 0.0f);

        // =================================================================
        //  Compute effective (macro-modulated) parameter values
        // =================================================================

        // M1 (SEA STATE/CHARACTER): increases sea state and turbulence
        float effectiveSeaState = clamp(pSeaState + macroCharacter * 0.5f + couplingSeaStateMod, 0.0f, 1.0f);

        // M2 (MOVEMENT): shortens swell period (faster wave rhythm)
        float effectiveSwellPeriod =
            clamp(pSwellPeriod + couplingSwellPeriodMod * 5.0f - macroMovement * 4.0f, 0.5f, 30.0f);

        // D005/D004 fix: advance seaStateLFO — rate scaled by M2 MOVEMENT (0.05-1.0 Hz).
        // The LFO creates organic amplitude breathing: the sea state gently rises and falls
        // like a slowly breathing organism at rest, or choppy waves when MOVEMENT is high.
        // Depth is fixed at 0.15 (audible but not overwhelming — a gentle swell, not a storm).
        // D002: LFO1 shape is now user-controllable (was sine-only).
        float lfoRateHz = 0.05f + macroMovement * 0.95f; // 0.05 Hz resting to 1.0 Hz active
        seaStateLFO.setRate(lfoRateHz, sampleRateFloat);
        seaStateLFO.setShape(static_cast<int>(loadParam(paramLfo1Shape, 0.0f)));
        float lfoOutput = seaStateLFO.process(); // [-1, +1]
        static constexpr float kLfoSeaStateDepth = 0.15f;
        effectiveSeaState = clamp(effectiveSeaState + lfoOutput * kLfoSeaStateDepth, 0.0f, 1.0f);
        // D006: mod wheel raises sea state / turbulence intensity — CC#1 adds up to +0.4 storm energy
        effectiveSeaState = clamp(effectiveSeaState + modWheelAmount * 0.4f, 0.0f, 1.0f);

        // D002: LFO2 — resonator brightness modulation (timbral shimmer).
        // Rate and shape are user-controllable. Depth scales the modulation amount.
        // At depth 0 = no effect. At depth 1 = full +/-0.4 brightness modulation.
        const float lfo2Rate = loadParam(paramLfo2Rate, 0.2f);
        const float lfo2Depth = loadParam(paramLfo2Depth, 0.0f);
        const int lfo2Shape = static_cast<int>(loadParam(paramLfo2Shape, 1.0f));
        brightnessLFO.setRate(lfo2Rate, sampleRateFloat);
        brightnessLFO.setShape(lfo2Shape);
        float lfo2Output = brightnessLFO.process(); // [-1, +1]
        float brightnessLfoMod = lfo2Output * lfo2Depth * 0.4f;

        // M4 (SPACE): increases harbor verb wet mix
        float effectiveVerbAmount = clamp(pHarborVerb + macroSpace * 0.4f, 0.0f, 1.0f);

        // M3 (COUPLING): increases creature voice depth
        float effectiveCreatureDepth = clamp(pCreatureDepth + macroCoupling * 0.3f, 0.0f, 1.0f);

        // D002: Apply LFO2 brightness modulation to resonator brightness parameter.
        // This creates timbral shimmer — the resonant body's brightness slowly morphs
        // as if light is moving across the instrument's surface.
        effectiveResonatorBright = clamp(effectiveResonatorBright + brightnessLfoMod, 0.0f, 1.0f);

        // =================================================================
        //  Shore morphing: decompose continuous shore position into
        //  interpolated resonator profiles, creature voices, and
        //  fluid character data from the ShoreSystem.
        // =================================================================

        ShoreMorphState shoreState = decomposeShore(pShore);

        ResonatorProfile morphedResonators[3];
        for (int i = 0; i < 3; ++i)
            morphedResonators[i] = morphResonator(shoreState, i);

        CreatureVoice morphedCreatures[3];
        for (int i = 0; i < 3; ++i)
            morphedCreatures[i] = morphCreature(shoreState, i);

        FluidCharacter morphedFluid = morphFluid(shoreState);

        // =================================================================
        //  Consume coupling accumulators (written by applyCouplingInput)
        //  and reset for next block.
        // =================================================================

        // Clamp accumulators before consuming: multiple AudioToFM / LFOToPitch
        // sources can accumulate additively per block. Clamping here prevents
        // runaway excitation that would drive the resonators into instability.
        float excitationMod = clamp(couplingExcitationMod, -2.0f, 2.0f);
        float pitchMod = clamp(couplingPitchMod, -1.0f, 1.0f);
        bool audioReplace = couplingAudioReplaceActive;
        float audioReplaceVal = couplingAudioReplaceLevel;

        couplingExcitationMod = 0.0f;
        couplingSeaStateMod = 0.0f;
        couplingSwellPeriodMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingAudioReplaceActive = false;
        couplingAudioReplaceLevel = 0.0f;

        // =================================================================
        //  Configure post-processing filters for this block
        // =================================================================

        // D001: filter envelope — velocity × envelope level boosts LP cutoff brightness.
        // Only applied in LP mode (pFilterTilt < 0.5); HP mode opens up as tilt decreases,
        // so boosting cutoff in HP would darken rather than brighten — wrong direction.
        // kMaxHz=5500 gives an audible sweep across most of the 200-8200 LP range.
        {
            static constexpr float kOspreyFilterEnvMaxHz = 5500.0f;
            const float filterEnvDepth = loadParam(paramFilterEnvDepth, 0.25f);
            float peakVelEnv = 0.0f;
            for (const auto& v : voices)
            {
                if (v.active)
                    peakVelEnv = std::max(peakVelEnv, v.velocity * v.amplitudeEnvelope.level);
            }
            filterEnvBoost = filterEnvDepth * peakVelEnv * kOspreyFilterEnvMaxHz;
        }

        // Tilt filter: spectral balance control.
        // At 0.0 (dark): lowpass sweeps 200-8200 Hz
        // At 0.5 (neutral): wide open
        // At 1.0 (bright): highpass sweeps 8200-400 Hz
        if (pFilterTilt < 0.5f)
        {
            float cutoff = 200.0f + (pFilterTilt * 2.0f) * 8000.0f + filterEnvBoost;
            // P17: clamp to sample-rate-relative Nyquist, not hardcoded 20 kHz.
            // At 96kHz/192kHz, CytomicSVF can handle frequencies above 20 kHz;
            // using sampleRateFloat*0.49f keeps the ceiling correct at all rates.
            cutoff = juce::jlimit(200.0f, sampleRateFloat * 0.49f, cutoff);
            tiltFilterL.setMode(CytomicSVF::Mode::LowPass);
            tiltFilterR.setMode(CytomicSVF::Mode::LowPass);
            tiltFilterL.setCoefficients(cutoff, 0.3f, sampleRateFloat);
            tiltFilterR.setCoefficients(cutoff, 0.3f, sampleRateFloat);
        }
        else
        {
            float cutoff = 8200.0f - ((pFilterTilt - 0.5f) * 2.0f) * 7800.0f;
            tiltFilterL.setMode(CytomicSVF::Mode::HighPass);
            tiltFilterR.setMode(CytomicSVF::Mode::HighPass);
            tiltFilterL.setCoefficients(cutoff, 0.3f, sampleRateFloat);
            tiltFilterR.setCoefficients(cutoff, 0.3f, sampleRateFloat);
        }

        // Fog filter: gentle lowpass HF rolloff (simulates sound in fog/mist).
        // At pFog=0: near-Nyquist (transparent). At pFog=1: 4kHz (muffled).
        // P17: top of range is sampleRateFloat*0.45f so the filter is truly
        // transparent at high sample rates (96/192kHz) rather than capping at 20 kHz.
        const float kFogMaxCutoff = sampleRateFloat * 0.45f;
        float fogCutoff = kFogMaxCutoff - pFog * (kFogMaxCutoff - 4000.0f);
        fogFilterL.setMode(CytomicSVF::Mode::LowPass);
        fogFilterR.setMode(CytomicSVF::Mode::LowPass);
        fogFilterL.setCoefficients(fogCutoff, 0.1f, sampleRateFloat);
        fogFilterR.setCoefficients(fogCutoff, 0.1f, sampleRateFloat);

        // Hull filter: resonant lowpass for body resonance (the boat itself).
        // Cutoff 150-750 Hz with increasing resonance — models the wooden
        // body of a vessel amplifying low-mid frequencies.
        float hullCutoff = 150.0f + pHull * 600.0f;
        float hullResonance = 0.3f + pHull * 0.5f;
        hullFilterL.setMode(CytomicSVF::Mode::LowPass);
        hullFilterR.setMode(CytomicSVF::Mode::LowPass);
        hullFilterL.setCoefficients(hullCutoff, hullResonance, sampleRateFloat);
        hullFilterR.setCoefficients(hullCutoff, hullResonance, sampleRateFloat);

        // Harbor verb feedback: higher verb amount = longer diffusion tail.
        // Range 0.3-0.65 keeps the verb stable (no runaway feedback).
        float verbFeedback = 0.3f + effectiveVerbAmount * 0.35f;
        for (int i = 0; i < 4; ++i)
            harborVerbAllpass[i].feedbackCoeff = verbFeedback;

        // =================================================================
        //  Process MIDI events
        // =================================================================

        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();

            if (msg.isNoteOn())
            {
                silenceGate.wake();
                handleNoteOn(msg.getNoteNumber(), msg.getFloatVelocity(), pAmpAttack, pAmpDecay, pAmpSustain,
                             pAmpRelease, morphedResonators, morphedCreatures, morphedFluid, effectiveSeaState,
                             effectiveResonatorBright, pResonatorDecay, pCreatureRate, pVoiceMode, pGlideTime);
            }
            else if (msg.isNoteOff())
            {
                handleNoteOff(msg.getNoteNumber());
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                reset();
            }
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure(msg.getChannelPressureValue() / 127.0f);
            // D006: CC#1 mod wheel → sea state / turbulence intensity boost (+0–0.4)
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }

        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        aftertouch.updateBlock(numSamples);
        const float atPressure = aftertouch.getSmoothedPressure(0);

        // D006: aftertouch shifts shore blend position — pressing harder moves toward
        // the next coastline (sensitivity 0.25). Shore range is 0–4 (5 coastlines),
        // so +0.25 nudges toward the next coastal character (~1/4 of a full region step).
        // Re-decompose shore with aftertouch offset so resonator profiles update.
        float effectiveShore = clamp(pShore + atPressure * 0.25f * 4.0f, 0.0f, 4.0f);
        if (atPressure > 0.001f)
        {
            ShoreMorphState atShoreState = decomposeShore(effectiveShore);
            for (int i = 0; i < 3; ++i)
            {
                morphedResonators[i] = morphResonator(atShoreState, i);
                morphedCreatures[i] = morphCreature(atShoreState, i);
            }
            morphedFluid = morphFluid(atShoreState);
        }

        float blockPeakEnvelope = 0.0f;

        // =================================================================
        //  Per-sample render loop
        // =================================================================

        // Block-constant pitch-bend ratio used inside the control-rate resonator
        // refresh path. pitchBendNorm is block-rate from MIDI.
        const float blockPitchBendRatio = PitchBendUtil::semitonesToFreqRatio(pitchBendNorm * 2.0f);

        // Perf: DC blocker coefficient is sample-rate-derived and constant
        // for the entire block. Hoisted from the per-voice inner loop to avoid
        // one float division per voice per sample (sampleRateFloat never changes
        // mid-block; declaring const inside the loop defeats the intent).
        const float kDcBlockerCoefficient = 1.0f - (6.28318f * 5.0f / sampleRateFloat);

        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            // --- Global fluid energy (one instance shared by all voices) ---
            float fluidEnergy =
                fluidModel.process(effectiveSeaState, effectiveSwellPeriod, morphedFluid, sampleRateFloat);

            // Add coupling excitation modulation (from external engines)
            float excitation = fluidEnergy + excitationMod * 0.3f;

            // AudioToWavetable coupling: external audio replaces the
            // fluid model entirely as the excitation source
            if (audioReplace)
                excitation = audioReplaceVal;

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                // --- Voice-stealing crossfade (5ms linear ramp) ---
                // Fade-out: existing stolen voice decrements to silence then dies.
                // Fade-in: new note on a stolen slot starts at 0, increments to 1.
                if (voice.fadingOut)
                {
                    voice.fadeGain -= crossfadeRate;
                    if (voice.fadeGain <= 0.0f)
                    {
                        voice.fadeGain = 0.0f;
                        voice.active = false;
                        continue;
                    }
                }
                else if (voice.fadeGain < 1.0f)
                {
                    voice.fadeGain = std::min(voice.fadeGain + crossfadeRate, 1.0f);
                }

                // --- Glide (portamento) ---
                voice.currentGlideFrequency +=
                    (voice.targetFrequency - voice.currentGlideFrequency) * voice.glideCoefficient;

                // --- Amplitude envelope tick ---
                float envelopeLevel = voice.amplitudeEnvelope.process();

                if (!voice.amplitudeEnvelope.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // --- Control-rate coefficient update (~2kHz) ---
                // Resonator coefficients only need updating when pitch or
                // shore parameters change. At audio rate this would waste
                // ~80% of CPU on redundant trig computations.
                voice.controlCounter++;
                if (voice.controlCounter >= controlRateDivisor)
                {
                    voice.controlCounter = 0;

                    // blockPitchBendRatio hoisted to per-block scope below —
                    // pitchBendNorm is block-rate from MIDI pitch wheel.
                    float baseFrequency = voice.currentGlideFrequency * blockPitchBendRatio;

                    // Apply pitch coupling modulation (+/- half octave range)
                    if (std::fabs(pitchMod) > 0.001f)
                        baseFrequency *= fastPow2(pitchMod * 0.5f);

                    // Update resonator coefficients: 3 instrument groups x 4 formants
                    for (int instrumentGroup = 0; instrumentGroup < 3; ++instrumentGroup)
                    {
                        const auto& profile = morphedResonators[instrumentGroup];
                        for (int formantIndex = 0; formantIndex < 4; ++formantIndex)
                        {
                            int resonatorIndex = instrumentGroup * 4 + formantIndex;

                            // Scale formant frequency relative to MIDI note.
                            // The profile stores absolute frequencies based on
                            // A440 reference — divide by 440 to get the ratio,
                            // then multiply by the note's actual frequency.
                            float frequencyRatio = profile.formantFreqs[formantIndex] / 440.0f;
                            float resonatorFreq = baseFrequency * frequencyRatio;
                            resonatorFreq = clamp(resonatorFreq, 20.0f, sampleRateFloat * 0.45f);

                            // Brightness: boost upper formants (indices 2,3),
                            // slightly attenuate lower formants (indices 0,1)
                            float formantGain = profile.formantGains[formantIndex];
                            if (formantIndex >= 2)
                                formantGain *= (0.5f + effectiveResonatorBright * 1.0f);
                            else
                                formantGain *= (1.0f - effectiveResonatorBright * 0.3f);

                            // Wind direction shifts spectral energy distribution.
                            // windShift [-1,+1]: negative = energy toward low formants,
                            // positive = energy toward high formants.
                            float windShift = (pWindDirection - 0.5f) * 2.0f;
                            if (formantIndex < 2)
                                formantGain *= (1.0f - windShift * 0.3f);
                            else
                                formantGain *= (1.0f + windShift * 0.3f);

                            formantGain = clamp(formantGain, 0.0f, 1.5f);

                            // Resonator decay controls bandwidth: longer decay =
                            // narrower bandwidth = more tonal, ringing character.
                            float resonatorBandwidth =
                                profile.formantBandwidths[formantIndex] / (0.5f + pResonatorDecay);
                            resonatorBandwidth = clamp(resonatorBandwidth, 5.0f, 2000.0f);

                            // Depth: at high depth, attenuate surface-oriented
                            // high formants (water absorbs high frequencies)
                            if (formantIndex >= 2)
                                formantGain *= (1.0f - pDepth * 0.5f);

                            voice.resonators[static_cast<size_t>(resonatorIndex)].setCoefficients(
                                resonatorFreq, resonatorBandwidth, formantGain, sampleRateFloat);
                        }
                    }

                    // Sympathetic resonators (indices 12-15): tuned to
                    // harmonics 2, 3, 4, 5 of the fundamental. These model
                    // the sympathetic vibration of unstruck strings on
                    // instruments like sitar, harp, and piano.
                    static constexpr float kSympathyHarmonicRatios[4] = {2.0f, 3.0f, 4.0f, 5.0f};
                    for (int sympatheticIndex = 0; sympatheticIndex < 4; ++sympatheticIndex)
                    {
                        int resonatorIndex = 12 + sympatheticIndex;
                        float sympatheticFreq = baseFrequency * kSympathyHarmonicRatios[sympatheticIndex];
                        sympatheticFreq = clamp(sympatheticFreq, 20.0f, sampleRateFloat * 0.45f);

                        // Higher harmonics get progressively quieter (0.5, 0.4, 0.3, 0.2)
                        float sympatheticGain = pSympathyAmount * (0.5f - static_cast<float>(sympatheticIndex) * 0.1f);
                        sympatheticGain = clamp(sympatheticGain, 0.0f, 1.0f);

                        float sympatheticBandwidth = 50.0f + (1.0f - pResonatorDecay * 0.1f) * 100.0f;
                        sympatheticBandwidth = clamp(sympatheticBandwidth, 10.0f, 500.0f);

                        voice.resonators[static_cast<size_t>(resonatorIndex)].setCoefficients(
                            sympatheticFreq, sympatheticBandwidth, sympatheticGain, sampleRateFloat);
                    }

                    // Update creature formant targets and trigger new vocalizations
                    for (int creatureIndex = 0; creatureIndex < 3; ++creatureIndex)
                    {
                        auto& creature = voice.creatures[static_cast<size_t>(creatureIndex)];
                        creature.configure(morphedCreatures[creatureIndex], sampleRateFloat);

                        // Stochastic creature triggering: probability increases
                        // with both creature rate and sea state (stormier seas
                        // provoke more vocalizations — birds call in wind, etc.)
                        if (!creature.active)
                        {
                            float triggerProbability = pCreatureRate * effectiveSeaState * controlRateDeltaTime * 0.5f;
                            if (voice.nextRandomUnipolar() < triggerProbability)
                            {
                                creature.active = true;
                                creature.trigger(sampleRateFloat);
                            }
                        }
                    }
                }

                // =============================================================
                //  Audio-rate processing
                // =============================================================

                float voiceOutput = 0.0f;

                // Per-voice noise injection for decoherence: at low coherence,
                // each voice receives slightly different excitation, creating
                // the scattered quality of wind-driven waves.
                float voiceExcitation = excitation + voice.nextRandomBipolar() * (1.0f - pCoherence) * 0.1f;

                // --- Process all 16 resonators ---
                float resonatorSum = 0.0f;
                for (int resonatorIndex = 0; resonatorIndex < kNumResonators; ++resonatorIndex)
                {
                    float resonatorOutput =
                        voice.resonators[static_cast<size_t>(resonatorIndex)].process(voiceExcitation);

                    // Coherence control: at high coherence (>0.5), gently
                    // lowpass each resonator's output to align their phases.
                    // This makes the sound more tonal and less chaotic —
                    // like calm water vs. choppy surf.
                    if (pCoherence > 0.5f)
                    {
                        float coherenceFactor = (pCoherence - 0.5f) * 2.0f; // remap 0.5-1.0 -> 0.0-1.0
                        float smoothingRate = 0.1f + coherenceFactor * 0.4f;

                        voice.coherenceStates[resonatorIndex] +=
                            (resonatorOutput - voice.coherenceStates[resonatorIndex]) * smoothingRate;
                        // Flush denormals in coherence feedback state
                        voice.coherenceStates[resonatorIndex] = flushDenormal(voice.coherenceStates[resonatorIndex]);

                        resonatorOutput =
                            lerp(resonatorOutput, voice.coherenceStates[resonatorIndex], coherenceFactor * 0.5f);
                    }

                    resonatorSum += resonatorOutput;
                }

                // Sympathetic coupling: feed the resonator sum back into the
                // sympathetic resonators (indices 12-15). This models how
                // acoustic instruments have strings that vibrate in sympathy
                // with the played note — a piano's undamped strings ring
                // when a chord is struck nearby.
                if (pSympathyAmount > 0.01f)
                {
                    float sympathyFeedback = resonatorSum * pSympathyAmount * 0.05f;
                    for (int sympatheticIndex = 12; sympatheticIndex < 16; ++sympatheticIndex)
                    {
                        voice.resonators[static_cast<size_t>(sympatheticIndex)].state1 +=
                            flushDenormal(sympathyFeedback * 0.1f);
                    }
                }

                // Normalize resonator output. 1/sqrt(16) = 0.25 is the
                // theoretical RMS normalization for summing 16 uncorrelated
                // signals to maintain consistent output level. 1/6 ≈ 0.167
                // is empirically tuned (lower to avoid overdrive into the
                // soft limiter, since the resonators are correlated in practice).
                static constexpr float kResonatorNormalization = 1.0f / 6.0f;
                voiceOutput = resonatorSum * kResonatorNormalization;

                // --- Mix in creature voice formant output ---
                float creatureOutput = 0.0f;
                float creatureExcitation = voice.nextRandomBipolar() * 0.5f + fluidEnergy * 0.5f;
                for (int creatureIndex = 0; creatureIndex < 3; ++creatureIndex)
                {
                    creatureOutput += voice.creatures[static_cast<size_t>(creatureIndex)].process(sampleRateFloat,
                                                                                                  creatureExcitation);
                }
                voiceOutput += creatureOutput * effectiveCreatureDepth;

                // --- DC Blocker (1-pole highpass at ~5Hz) ---
                // Removes DC offset that can accumulate from asymmetric
                // excitation patterns and creature formant filtering.
                // Coefficient R = 1 - 2π·fc/sr (first-order Euler approximation).
                // At 5 Hz this is negligibly different from matched-Z
                // (exp(-2π·fc/sr)) — the two converge for fc << sr.
                // Using the sample-rate-derived coefficient ensures the 5 Hz
                // cutoff is correct at 44.1 / 48 / 96 / 192 kHz.
                // kDcBlockerCoefficient is hoisted to pre-sample-loop scope above.
                float dcInput = voiceOutput;
                float dcOutput = dcInput - voice.dcPreviousInputL + kDcBlockerCoefficient * voice.dcPreviousOutputL;
                voice.dcPreviousInputL = dcInput;
                voice.dcPreviousOutputL = flushDenormal(dcOutput);
                voiceOutput = dcOutput;

                // --- Soft limiter (tanh saturation) ---
                // 1.5x overdrive into tanh provides gentle compression.
                // Prevents harsh digital clipping while adding warmth.
                voiceOutput = fastTanh(voiceOutput * 1.5f);

                // --- Apply amplitude envelope, velocity, and crossfade ---
                float voiceGain = envelopeLevel * voice.velocity * voice.fadeGain;
                voiceOutput *= voiceGain;

                // --- Stereo placement ---
                // CPU fix: panGainL/panGainR are precomputed at noteOn (startVoice).
                // noteNumber % 12 is constant for the voice lifetime — std::cos/std::sin
                // do not need to run per sample per voice.
                float voiceL = voiceOutput * voice.panGainL;
                float voiceR = voiceOutput * voice.panGainR;

                // Final per-voice denormal protection before accumulation
                voiceL = flushDenormal(voiceL);
                voiceR = flushDenormal(voiceR);

                mixL += voiceL;
                mixR += voiceR;

                blockPeakEnvelope = std::max(blockPeakEnvelope, envelopeLevel);
            }

            // =============================================================
            //  Post-processing chain (applied to stereo voice sum)
            //
            //  Signal flow:
            //    Tilt -> Foam -> Brine -> Hull -> Fog -> Harbor Verb
            // =============================================================

            // --- Tilt filter: spectral balance ---
            mixL = tiltFilterL.processSample(mixL);
            mixR = tiltFilterR.processSample(mixR);

            // --- Foam: HF saturation (whitecap spray) ---
            // Soft-clipping with variable drive models the fizzy,
            // airy quality of breaking wave crests.
            if (pFoam > 0.01f)
            {
                float foamDriveAmount = 1.0f + pFoam * 8.0f; // 1-9x drive
                // Drive into softClip, then normalize + slight makeup gain
                mixL = softClip(mixL * foamDriveAmount) / foamDriveAmount * (1.0f + pFoam * 0.5f);
                mixR = softClip(mixR * foamDriveAmount) / foamDriveAmount * (1.0f + pFoam * 0.5f);
            }

            // --- Brine: subtle bit reduction (salt crystal) ---
            // Quantizes amplitude to simulate the granular quality
            // of sound through salt-laden air.
            if (pBrine > 0.01f)
            {
                float effectiveBitDepth = 16.0f - pBrine * 12.0f; // 16-bit -> 4-bit
                float quantizationLevels = fastPow2(effectiveBitDepth);
                float inverseQuantization = 1.0f / std::max(quantizationLevels, 1e-6f);
                mixL = std::floor(mixL * quantizationLevels) * inverseQuantization;
                mixR = std::floor(mixR * quantizationLevels) * inverseQuantization;
            }

            // --- Hull: body resonance (the boat) ---
            // Parallel mix of resonant lowpass adds wooden body character,
            // like sound resonating inside a vessel's hull.
            if (pHull > 0.01f)
            {
                float hullResonanceL = hullFilterL.processSample(mixL);
                float hullResonanceR = hullFilterR.processSample(mixR);
                mixL += hullResonanceL * pHull * 0.5f;
                mixR += hullResonanceR * pHull * 0.5f;
            }

            // --- Fog: gentle LP rolloff ---
            mixL = fogFilterL.processSample(mixL);
            mixR = fogFilterR.processSample(mixR);

            // --- Harbor verb: 4-stage allpass diffusion network ---
            if (effectiveVerbAmount > 0.01f)
            {
                float verbInputL = mixL;
                float verbInputR = mixR;

                // L channel through allpass stages 0,1; R through 2,3
                float verbOutputL = harborVerbAllpass[0].process(verbInputL);
                verbOutputL = harborVerbAllpass[1].process(verbOutputL);
                float verbOutputR = harborVerbAllpass[2].process(verbInputR);
                verbOutputR = harborVerbAllpass[3].process(verbOutputR);

                // Cross-feed between L/R for stereo width in the verb tail.
                // 15% creates subtle width without comb-filtering artifacts.
                static constexpr float kVerbCrossfeedAmount = 0.15f;
                float diffusedL = verbOutputL + verbOutputR * kVerbCrossfeedAmount;
                float diffusedR = verbOutputR + verbOutputL * kVerbCrossfeedAmount;

                // Wet/dry mix
                mixL = mixL * (1.0f - effectiveVerbAmount) + diffusedL * effectiveVerbAmount;
                mixR = mixR * (1.0f - effectiveVerbAmount) + diffusedR * effectiveVerbAmount;
            }

            // Final denormal protection on master output.
            // This is the last line of defense — catches any denormals
            // that leaked through the post-processing chain.
            mixL = flushDenormal(mixL);
            mixR = flushDenormal(mixR);

            // --- Write to output buffer ---
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample(0, sampleIndex, mixL);
                buffer.addSample(1, sampleIndex, mixR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample(0, sampleIndex, (mixL + mixR) * 0.5f);
            }

            // --- Cache for coupling reads (other engines read our output) ---
            if (sampleIndex < static_cast<int>(outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t>(sampleIndex)] = mixL;
                outputCacheR[static_cast<size_t>(sampleIndex)] = mixR;
            }
        }

        peakEnvelopeOutput = blockPeakEnvelope;

        // Update active voice count for UI display
        int count = 0;
        for (const auto& voice : voices)
            if (voice.active)
                ++count;
        activeVoiceCount_.store(count, std::memory_order_relaxed);

        silenceGate.analyzeBlock(buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples);
    }

    //==========================================================================
    // SynthEngine interface — Coupling
    //
    // Output channels:
    //   0 = Left audio (post-processing)
    //   1 = Right audio (post-processing)
    //   2 = Peak envelope level (for amplitude-driven coupling)
    //
    // Input types:
    //   AudioToFM       -> adds to resonator excitation energy
    //   AmpToFilter     -> modulates effective sea state
    //   EnvToMorph      -> modulates swell period
    //   LFOToPitch      -> modulates resonator tuning (+/- semitones)
    //   AudioToWavetable -> replaces fluid excitation with external audio
    //
    // Rejected: AmpToChoke (the ocean doesn't stop),
    //           PitchToPitch (creates mud with resonator tuning)
    //==========================================================================

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0)
            return 0.0f;
        auto index = static_cast<size_t>(sampleIndex);
        if (channel == 0 && index < outputCacheL.size())
            return outputCacheL[index];
        if (channel == 1 && index < outputCacheR.size())
            return outputCacheR[index];
        if (channel == 2)
            return peakEnvelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        switch (type)
        {
        case CouplingType::AudioToFM:
            // External audio adds to resonator excitation energy.
            // 0.5 scaling prevents overwhelming the fluid model.
            couplingExcitationMod += amount * 0.5f;
            break;

        case CouplingType::AmpToFilter:
            // External amplitude modulates sea state (louder = stormier).
            // 0.3 scaling keeps modulation musical.
            couplingSeaStateMod += amount * 0.3f;
            break;

        case CouplingType::EnvToMorph:
            // External envelope modulates swell period (faster/slower waves).
            couplingSwellPeriodMod += amount * 0.4f;
            break;

        case CouplingType::LFOToPitch:
            // External LFO modulates resonator tuning offset.
            // 0.2 scaling = max +/- ~2.4 semitones at full amount.
            couplingPitchMod += amount * 0.2f;
            break;

        case CouplingType::AudioToWavetable:
        {
            // External audio replaces the fluid energy model entirely
            // as the resonator excitation source. Uses RMS of the source
            // buffer to derive a smooth energy level.
            couplingAudioReplaceActive = true;
            if (sourceBuffer != nullptr && numSamples > 0)
            {
                float sumOfSquares = 0.0f;
                int samplesToAnalyze = std::min(numSamples, 64); // cap analysis to 64 samples
                for (int i = 0; i < samplesToAnalyze; ++i)
                    sumOfSquares += sourceBuffer[i] * sourceBuffer[i];
                couplingAudioReplaceLevel = std::sqrt(sumOfSquares / static_cast<float>(samplesToAnalyze)) * amount;
            }
            else
            {
                couplingAudioReplaceLevel = amount * 0.5f;
            }
            break;
        }

        default:
            break;
        }
    }

    //==========================================================================
    // SynthEngine interface — Parameters
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }
    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // --- Core Ocean Parameters ---
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_shore", 1}, "Osprey Shore",
                                                        juce::NormalisableRange<float>(0.0f, 4.0f, 0.001f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_seaState", 1}, "Osprey Sea State",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_swellPeriod", 1}, "Osprey Swell Period",
            juce::NormalisableRange<float>(0.5f, 30.0f, 0.01f, 0.4f), 8.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_windDir", 1}, "Osprey Wind Direction",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_depth", 1}, "Osprey Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        // --- Resonator Parameters ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_resonatorBright", 1}, "Osprey Resonator Brightness",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_resonatorDecay", 1}, "Osprey Resonator Decay",
            juce::NormalisableRange<float>(0.01f, 8.0f, 0.001f, 0.3f), 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_sympathyAmount", 1}, "Osprey Sympathy Amount",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        // --- Creature Parameters ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_creatureRate", 1}, "Osprey Creature Rate",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_creatureDepth", 1}, "Osprey Creature Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        // --- Coherence & Texture Parameters ---
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_coherence", 1}, "Osprey Coherence",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.7f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_foam", 1}, "Osprey Foam",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
                                                                     0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_brine", 1}, "Osprey Brine",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_hull", 1}, "Osprey Hull",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
                                                                     0.2f));

        // --- Filter & Space Parameters ---
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_filterTilt", 1}, "Osprey Filter Tilt",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        // D001: velocity × envelope level sweeps the tilt LP cutoff upward on hard attacks.
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_filterEnvDepth", 1}, "Osprey Filter Env Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.25f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_harborVerb", 1}, "Osprey Harbor Verb",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_fog", 1}, "Osprey Fog",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
                                                                     0.1f));

        // --- Amp Envelope ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_ampAttack", 1}, "Osprey Amp Attack",
            juce::NormalisableRange<float>(0.0f, 8.0f, 0.001f, 0.3f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_ampDecay", 1}, "Osprey Amp Decay",
            juce::NormalisableRange<float>(0.05f, 8.0f, 0.001f, 0.3f), 1.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_ampSustain", 1}, "Osprey Amp Sustain",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_ampRelease", 1}, "Osprey Amp Release",
            juce::NormalisableRange<float>(0.05f, 12.0f, 0.001f, 0.3f), 2.0f));

        // --- Voice mode and glide (Round 10F) ---
        // osprey_voiceMode:
        //   0=Poly  — every note spawns its own voice (original behaviour, default)
        //   1=Mono  — retriggered single voice (useful for leads with portamento)
        //   2=Legato — new note while gate open slides pitch, skips envelope retrigger
        // osprey_glide: portamento time 0–2 s.  When > 0, glideCoefficient is computed
        //   as 1 - exp(-1/(glideTime * sampleRate)) and applied per-sample in the
        //   render loop (IIR exponential approach).
        //
        // Sample-rate invariance: the formula coeff = 1 - exp(-1/(T*sr)) is SR-invariant.
        // At higher SR, coeff is smaller (smaller per-sample step), but there are
        // proportionally more samples per second, so wall-clock convergence time is
        // always T seconds (reaches 63.2% of target at T seconds, 99% at ~4.6*T seconds)
        // regardless of whether sr=44100 or sr=96000.
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"osprey_voiceMode", 1},
                                                                      "Osprey Voice Mode",
                                                                      juce::StringArray{"Poly", "Mono", "Legato"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_glide", 1}, "Osprey Glide",
            juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.3f), 0.0f));

        // --- Macros ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_macroCharacter", 1}, "Osprey Macro CHARACTER",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_macroMovement", 1}, "Osprey Macro MOVEMENT",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_macroCoupling", 1}, "Osprey Macro COUPLING",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_macroSpace", 1}, "Osprey Macro SPACE",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        // --- D002: LFO Shape + Second LFO ---
        // LFO1 shape: selects waveform for the existing sea state LFO (was sine-only).
        // AudioParameterChoice (not Float) so DAWs render it as a discrete selector.
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"osprey_lfo1Shape", 1}, "Osprey LFO1 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        // LFO2: secondary modulator targeting resonator brightness.
        // Rate: 0.01-8 Hz (slow shimmer to fast tremolo). Depth: 0-1.
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osprey_lfo2Rate", 1}, "Osprey LFO2 Rate",
            juce::NormalisableRange<float>(0.01f, 8.0f, 0.01f, 0.35f), 0.2f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osprey_lfo2Depth", 1}, "Osprey LFO2 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        // AudioParameterChoice (not Float) so DAWs render it as a discrete selector.
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"osprey_lfo2Shape", 1}, "Osprey LFO2 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 1));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramShore = apvts.getRawParameterValue("osprey_shore");
        paramSeaState = apvts.getRawParameterValue("osprey_seaState");
        paramSwellPeriod = apvts.getRawParameterValue("osprey_swellPeriod");
        paramWindDir = apvts.getRawParameterValue("osprey_windDir");
        paramDepth = apvts.getRawParameterValue("osprey_depth");
        paramResonatorBright = apvts.getRawParameterValue("osprey_resonatorBright");
        paramResonatorDecay = apvts.getRawParameterValue("osprey_resonatorDecay");
        paramSympathyAmount = apvts.getRawParameterValue("osprey_sympathyAmount");
        paramCreatureRate = apvts.getRawParameterValue("osprey_creatureRate");
        paramCreatureDepth = apvts.getRawParameterValue("osprey_creatureDepth");
        paramCoherence = apvts.getRawParameterValue("osprey_coherence");
        paramFoam = apvts.getRawParameterValue("osprey_foam");
        paramBrine = apvts.getRawParameterValue("osprey_brine");
        paramHull = apvts.getRawParameterValue("osprey_hull");
        paramFilterTilt = apvts.getRawParameterValue("osprey_filterTilt");
        paramFilterEnvDepth = apvts.getRawParameterValue("osprey_filterEnvDepth");
        paramHarborVerb = apvts.getRawParameterValue("osprey_harborVerb");
        paramFog = apvts.getRawParameterValue("osprey_fog");
        paramAmpAttack = apvts.getRawParameterValue("osprey_ampAttack");
        paramAmpDecay = apvts.getRawParameterValue("osprey_ampDecay");
        paramAmpSustain = apvts.getRawParameterValue("osprey_ampSustain");
        paramAmpRelease = apvts.getRawParameterValue("osprey_ampRelease");
        paramMacroCharacter = apvts.getRawParameterValue("osprey_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("osprey_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("osprey_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("osprey_macroSpace");
        // Round 10F: voice mode and glide
        paramVoiceMode = apvts.getRawParameterValue("osprey_voiceMode");
        paramGlideTime = apvts.getRawParameterValue("osprey_glide");
        paramLfo1Shape = apvts.getRawParameterValue("osprey_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("osprey_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("osprey_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("osprey_lfo2Shape");
    }

    //==========================================================================
    // SynthEngine interface — Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Osprey"; }

    // Azulejo Blue #1B4F8A — the blue of Portuguese coastal tiles,
    // where ocean meets architecture, nature meets craft.
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF1B4F8A); }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override { return activeVoiceCount_.load(std::memory_order_relaxed); }

private:
    SilenceGate silenceGate;

    //==========================================================================
    // Helper: safe atomic parameter load
    //==========================================================================

    static float loadParam(std::atomic<float>* param, float fallback) noexcept
    {
        return (param != nullptr) ? param->load() : fallback;
    }

    //==========================================================================
    // MIDI note handling
    //==========================================================================

    void handleNoteOn(int noteNumber, float velocity, float ampAttack, float ampDecay, float ampSustain,
                      float ampRelease, const ResonatorProfile* morphedResonators,
                      const CreatureVoice* morphedCreatures, const FluidCharacter& morphedFluid, float seaState,
                      float resonatorBrightness, float resonatorDecay, float creatureRate, int voiceMode,
                      float glideTimeSec)
    {
        float frequency = midiNoteToHz(static_cast<float>(noteNumber));

        // -----------------------------------------------------------------------
        // Legato detection (voiceMode == 2)
        //
        // If a voice is currently gate-open (active and NOT fading/releasing),
        // slide its pitch to the new frequency without retriggering the envelope.
        // The glide coefficient controls the speed of the pitch slide:
        //   glideCoeff = exp(-1/(glideTime * sampleRate))
        //   glideCoeff = 1.0 means instant (no portamento effect within the glide)
        // The coefficient is applied per-sample in the render loop:
        //   currentFrequency += (target - current) * (1 - glideCoeff)
        // which is an exponential approach converging to target in ~glideTime seconds.
        //
        // This is the wire-up described in Round 9D: the glideCoefficient field in
        // OspreyVoice has always been present and used in the render loop, but
        // initializeVoice() always set it to 1.0 (instant), and there was no
        // voice-mode parameter to control legato behaviour.
        // -----------------------------------------------------------------------
        if (voiceMode == 2)
        {
            for (int i = 0; i < kMaxVoices; ++i)
            {
                auto& v = voices[static_cast<size_t>(i)];
                // Gate open: active voice that has NOT started fading out and whose
                // amplitude envelope is NOT in Release or Idle
                if (v.active && !v.fadingOut && v.amplitudeEnvelope.stage != StandardADSR::Stage::Release &&
                    v.amplitudeEnvelope.stage != StandardADSR::Stage::Idle)
                {
                    v.noteNumber = noteNumber;
                    // Do not update velocity in legato mode — changing it mid-note
                    // causes an abrupt volume jump since voiceGain = env * velocity.
                    // True legato slides pitch only; the playing voice's amplitude
                    // context (velocity, envelope level) continues uninterrupted.
                    // v.velocity = velocity;  // intentionally omitted for legato
                    v.targetFrequency = frequency;

                    // Compute glide coefficient from user glide time.
                    // If glideTime==0 (or very small), snap immediately to
                    // prevent a detached-pitch artefact between notes.
                    static constexpr float kMinGlideSec = 0.001f;
                    if (glideTimeSec > kMinGlideSec)
                        // SR-invariant IIR coefficient: coeff = 1 - exp(-1/(T*sr)).
                        // Wall-clock convergence is always T seconds at any sample rate —
                        // higher SR produces a smaller coeff but more samples/sec, cancelling out.
                        v.glideCoefficient = 1.0f - fastExp(-1.0f / (glideTimeSec * sampleRateFloat));
                    else
                        v.currentGlideFrequency = frequency; // snap

                    return; // legato: no retrigger, no new voice allocation
                }
            }
        }

        // -----------------------------------------------------------------------
        // Mono voice mode (voiceMode == 1): release all other active voices
        // before allocating the new note. This ensures only one voice plays at
        // a time, with the amplitude envelope retriggerring on each new note.
        // (Previously Mono fell through to the same path as Poly — it was silent
        // missing feature: up to 8 voices played simultaneously in "Mono" mode.)
        // -----------------------------------------------------------------------
        if (voiceMode == 1)
        {
            for (int i = 0; i < kMaxVoices; ++i)
            {
                auto& v = voices[static_cast<size_t>(i)];
                if (v.active && !v.fadingOut)
                    v.amplitudeEnvelope.noteOff();
            }
        }

        // -----------------------------------------------------------------------
        // Normal (Poly / Mono) voice allocation
        // -----------------------------------------------------------------------

        // Find a free voice or steal the oldest (LRU)
        int voiceIndex = findFreeVoice();
        auto& voice = voices[static_cast<size_t>(voiceIndex)];

        // If stealing an active voice, record whether a steal is happening so
        // initializeVoice can start the new note at fadeGain=0 (fade-in).
        // Previously, fadingOut=true was set here but immediately overwritten to
        // false by initializeVoice(), so the crossfade never ran — stealing a busy
        // voice caused an abrupt cut. Starting the new note at 0 and ramping to 1
        // over 5ms prevents the click on the incoming note instead.
        bool stealing = voice.active;

        initializeVoice(voice, noteNumber, velocity, frequency, ampAttack, ampDecay, ampSustain, ampRelease,
                        morphedResonators, morphedCreatures, morphedFluid, seaState, resonatorBrightness,
                        resonatorDecay, creatureRate, glideTimeSec);

        // Apply fade-in after initializeVoice sets fadeGain=1.0 — ramp from 0
        // to prevent click on the stolen voice slot.
        if (stealing)
            voice.fadeGain = 0.0f;
    }

    void initializeVoice(OspreyVoice& voice, int noteNumber, float velocity, float frequency, float ampAttack,
                         float ampDecay, float ampSustain, float ampRelease, const ResonatorProfile* morphedResonators,
                         const CreatureVoice* morphedCreatures, const FluidCharacter& /*morphedFluid*/,
                         float /*seaState*/, float resonatorBrightness, float resonatorDecay, float creatureRate,
                         float glideTimeSec = 0.0f)
    {
        // --- Voice lifecycle ---
        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.targetFrequency = frequency;

        // Round 10F: Wire glideCoefficient from osprey_glide parameter.
        // Previously hardcoded to 1.0 (instant). Now:
        //   glideTime == 0  -> coefficient = 1.0 (snap, currentGlideFrequency = target)
        //   glideTime >  0  -> coefficient = 1 - exp(-1/(glideTime * sampleRate))
        // The coefficient drives the per-sample IIR in the render loop:
        //   currentFrequency += (target - current) * coefficient
        //
        // SR-invariance: coeff = 1 - exp(-1/(T*sr)) produces identical wall-clock
        // glide duration at any sample rate. At 96kHz coeff is ~2x smaller than at
        // 44.1kHz, but twice as many samples elapse per second — these cancel exactly.
        static constexpr float kMinGlideSec = 0.001f;
        if (glideTimeSec > kMinGlideSec && voice.currentGlideFrequency > 10.0f)
        {
            // Glide from previous position to new target
            voice.glideCoefficient = 1.0f - fastExp(-1.0f / (glideTimeSec * sampleRateFloat));
        }
        else
        {
            voice.currentGlideFrequency = frequency;
            voice.glideCoefficient = 1.0f; // instant: reaches target next sample
        }
        voice.fadingOut = false;
        voice.fadeGain = 1.0f;
        voice.controlCounter = 0;

        // --- Clear DC blocker state ---
        voice.dcPreviousInputL = 0.0f;
        voice.dcPreviousOutputL = 0.0f;
        voice.dcPreviousInputR = 0.0f;
        voice.dcPreviousOutputR = 0.0f;

        // Initialize PRNG with note-dependent seed.
        // 7919 and 104729 are primes — multiplying by primes creates
        // good dispersion across the LCG state space, ensuring each
        // note+voice combination produces unique noise character.
        voice.randomState = static_cast<uint32_t>(noteNumber * 7919 + voiceCounter * 104729);

        // --- Stereo pan (CPU fix: precompute cos/sin once at noteOn) ---
        // noteNumber % 12 is constant for the voice lifetime — no need for per-sample trig.
        {
            float voicePanPosition = static_cast<float>(noteNumber % 12) / 12.0f - 0.5f;
            static constexpr float kMaxStereoSpread = 0.6f;
            voicePanPosition *= kMaxStereoSpread;
            float panAngle = (voicePanPosition + 1.0f) * 0.25f * kPi;
            voice.panGainL = std::cos(panAngle);
            voice.panGainR = std::sin(panAngle);
        }

        // --- Amplitude envelope ---
        voice.amplitudeEnvelope.setParams(ampAttack, ampDecay, ampSustain, ampRelease, sampleRateFloat);
        voice.amplitudeEnvelope.noteOn();

        // --- Initialize resonators with morphed shore profiles ---
        for (int instrumentGroup = 0; instrumentGroup < 3; ++instrumentGroup)
        {
            const auto& profile = morphedResonators[instrumentGroup];
            for (int formantIndex = 0; formantIndex < 4; ++formantIndex)
            {
                int resonatorIndex = instrumentGroup * 4 + formantIndex;
                auto& resonator = voice.resonators[static_cast<size_t>(resonatorIndex)];
                resonator.reset();

                float frequencyRatio = profile.formantFreqs[formantIndex] / 440.0f;
                float resonatorFreq = frequency * frequencyRatio;
                resonatorFreq = clamp(resonatorFreq, 20.0f, sampleRateFloat * 0.45f);

                float formantGain = profile.formantGains[formantIndex];
                if (formantIndex >= 2)
                    formantGain *= (0.5f + resonatorBrightness * 1.0f);

                float resonatorBandwidth = profile.formantBandwidths[formantIndex] / (0.5f + resonatorDecay);
                resonatorBandwidth = clamp(resonatorBandwidth, 5.0f, 2000.0f);

                resonator.setCoefficients(resonatorFreq, resonatorBandwidth, formantGain, sampleRateFloat);
            }
        }

        // --- Initialize sympathetic resonators (harmonics 2,3,4,5) ---
        static constexpr float kSympathyHarmonicRatios[4] = {2.0f, 3.0f, 4.0f, 5.0f};
        for (int sympatheticIndex = 0; sympatheticIndex < 4; ++sympatheticIndex)
        {
            int resonatorIndex = 12 + sympatheticIndex;
            auto& resonator = voice.resonators[static_cast<size_t>(resonatorIndex)];
            resonator.reset();
            float sympatheticFreq = frequency * kSympathyHarmonicRatios[sympatheticIndex];
            sympatheticFreq = clamp(sympatheticFreq, 20.0f, sampleRateFloat * 0.45f);
            resonator.setCoefficients(sympatheticFreq, 80.0f, 0.2f, sampleRateFloat);
        }

        // --- Initialize creature formant generators ---
        for (int creatureIndex = 0; creatureIndex < 3; ++creatureIndex)
        {
            auto& creature = voice.creatures[static_cast<size_t>(creatureIndex)];
            creature.reset();
            creature.configure(morphedCreatures[creatureIndex], sampleRateFloat);

            // Stagger initial creature triggering: not all creatures
            // vocalize immediately on note-on. This creates a more
            // natural onset where voices emerge gradually.
            if (voice.nextRandomUnipolar() < creatureRate * 0.5f)
            {
                creature.active = true;
                creature.trigger(sampleRateFloat);
            }
        }

        // --- Clear coherence tracking states ---
        for (int i = 0; i < OspreyVoice::kResonatorsPerVoice; ++i)
            voice.coherenceStates[i] = 0.0f;
    }

    void handleNoteOff(int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.fadingOut)
                voice.amplitudeEnvelope.noteOff();
        }
    }

    int findFreeVoice() { return VoiceAllocator::findFreeVoice(voices, kMaxVoices); }

    static float midiNoteToHz(float midiNote) noexcept { return 440.0f * fastPow2((midiNote - 69.0f) / 12.0f); }

    //==========================================================================
    //  Member data
    //==========================================================================

    // --- Sample rate ---
    double sampleRateDouble = 0.0;  // Sentinel: must be set by prepare() before use
    float sampleRateFloat = 0.0f;  // Sentinel: must be set by prepare() before use

    // --- Voice-stealing crossfade ---
    float crossfadeRate = 0.01f; // per-sample fade decrement

    // --- Control-rate decimation ---
    int controlRateDivisor = 22;          // ~2kHz at 44.1kHz
    float controlRateDeltaTime = 0.0005f; // 1/2000 seconds per control tick

    // --- Voice pool ---
    std::array<OspreyVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0; // monotonic counter for LRU stealing
    // activeVoiceCount_ promoted to base class std::atomic<int> — for UI display

    // --- Global fluid energy model (shared by all voices) ---
    FluidEnergyModel fluidModel;

    // D006: aftertouch handler — CS-80-style channel pressure → shore blend shift
    PolyAftertouch aftertouch;

    // ---- D006 Mod wheel — CC#1 boosts sea state / turbulence intensity (+0–0.4) ----
    float modWheelAmount = 0.0f;
    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // D005/D004 fix: StandardLFO instance to modulate sea state (amplitude breathing).
    // Route: LFO output -> effectiveSeaState modulation (low-frequency wave energy swell).
    // Rate is controlled by M2 MOVEMENT macro (higher MOVEMENT = faster LFO = choppier seas).
    StandardLFO seaStateLFO;

    // D002: Second LFO — targets resonator brightness for timbral shimmer.
    StandardLFO brightnessLFO;

    // --- Coupling accumulators ---
    // Written by applyCouplingInput(), consumed and cleared each renderBlock()
    float peakEnvelopeOutput = 0.0f;         // output: peak env for coupling reads
    float couplingExcitationMod = 0.0f;      // AudioToFM: adds to excitation
    float couplingSeaStateMod = 0.0f;        // AmpToFilter: modulates sea state
    float couplingSwellPeriodMod = 0.0f;     // EnvToMorph: modulates swell period
    float couplingPitchMod = 0.0f;           // LFOToPitch: modulates tuning
    float couplingAudioReplaceLevel = 0.0f;  // AudioToWavetable: replacement level
    bool couplingAudioReplaceActive = false; // AudioToWavetable: active flag

    // --- Output cache for coupling reads ---
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // --- Harbor verb: 4-stage allpass diffusion network ---
    AllpassDelay harborVerbAllpass[4];

    // --- Global post-processing filters ---
    CytomicSVF tiltFilterL, tiltFilterR; // spectral balance (LP <-> HP)
    CytomicSVF fogFilterL, fogFilterR;   // HF rolloff (distance/atmosphere)
    CytomicSVF hullFilterL, hullFilterR; // body resonance (the vessel)

    // --- Cached APVTS parameter pointers ---
    // Set once in attachParameters(), read atomically each renderBlock().

    // Core ocean
    std::atomic<float>* paramShore = nullptr;
    std::atomic<float>* paramSeaState = nullptr;
    std::atomic<float>* paramSwellPeriod = nullptr;
    std::atomic<float>* paramWindDir = nullptr;
    std::atomic<float>* paramDepth = nullptr;

    // Resonator
    std::atomic<float>* paramResonatorBright = nullptr;
    std::atomic<float>* paramResonatorDecay = nullptr;
    std::atomic<float>* paramSympathyAmount = nullptr;

    // Creature
    std::atomic<float>* paramCreatureRate = nullptr;
    std::atomic<float>* paramCreatureDepth = nullptr;

    // Coherence & character
    std::atomic<float>* paramCoherence = nullptr;
    std::atomic<float>* paramFoam = nullptr;
    std::atomic<float>* paramBrine = nullptr;
    std::atomic<float>* paramHull = nullptr;

    // Filter & space
    std::atomic<float>* paramFilterTilt = nullptr;
    std::atomic<float>* paramFilterEnvDepth = nullptr; // D001: velocity → tilt LP cutoff
    float filterEnvBoost = 0.0f;                       // computed block-rate, LP branch only
    std::atomic<float>* paramHarborVerb = nullptr;
    std::atomic<float>* paramFog = nullptr;

    // Amplitude envelope
    std::atomic<float>* paramAmpAttack = nullptr;
    std::atomic<float>* paramAmpDecay = nullptr;
    std::atomic<float>* paramAmpSustain = nullptr;
    std::atomic<float>* paramAmpRelease = nullptr;

    // Macros (M1-M4)
    std::atomic<float>* paramMacroCharacter = nullptr;
    std::atomic<float>* paramMacroMovement = nullptr;
    std::atomic<float>* paramMacroCoupling = nullptr;
    std::atomic<float>* paramMacroSpace = nullptr;

    // Round 10F: voice mode and glide (osprey_voiceMode, osprey_glide)
    std::atomic<float>* paramVoiceMode = nullptr; // 0=Poly, 1=Mono, 2=Legato
    std::atomic<float>* paramGlideTime = nullptr; // portamento time 0–2s

    // D002: LFO shape + second LFO
    std::atomic<float>* paramLfo1Shape = nullptr; // 0=Sine, 1=Tri, 2=Saw, 3=Sq, 4=S&H
    std::atomic<float>* paramLfo2Rate = nullptr;  // 0.01-8 Hz
    std::atomic<float>* paramLfo2Depth = nullptr; // 0-1
    std::atomic<float>* paramLfo2Shape = nullptr; // 0=Sine, 1=Tri, 2=Saw, 3=Sq, 4=S&H
};

} // namespace xoceanus
