// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/ParameterSmoother.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// OceanicEngine — Swarm Particle Synthesis via Boid Flocking Rules.
//
// Each voice spawns 128 simple oscillators ("particles") that self-organize
// using Craig Reynolds' boid rules (separation, alignment, cohesion) plus an
// attractor anchored to the MIDI note frequency. Particles live in a 3D
// perceptual space: log-frequency, amplitude, and pan. The emergent flocking
// behavior produces evolving, organic timbres impossible with static additive
// synthesis.
//
// Features:
//   - 128 particles per voice, each with independent freq/amp/pan + velocities
//   - Boid rules evaluated at control rate (~2kHz) for efficiency
//   - Sub-flocks (1-4) with independent frequency ratios (1x, 2x, 1.5x, 3x)
//   - Waveform per particle: Sine/Saw/Pulse/Noise
//   - PolyBLEP antialiasing for saw/pulse waveforms
//   - Note-on scatter: velocity-proportional random perturbation
//   - Murmuration: cascade reorganization triggered by coupling
//   - Amp ADSR + Swarm envelope (controls boid rule strength over time)
//   - 2 LFOs (Sine/Tri/Saw/Square/S&H)
//   - DC blocker + soft limiter on output
//   - Mono/Poly2/Poly4 voice modes with LRU stealing + 5ms crossfade
//
// Coupling:
//   - Output: post-limiter stereo audio via getSampleForCoupling
//   - Input: AudioToFM (velocity perturbation), AmpToFilter (modulates cohesion),
//            RhythmToBlend (triggers murmuration)
//
//==============================================================================

//==============================================================================
// Constants
//==============================================================================
static constexpr int kParticlesPerVoice = 128;
static constexpr float kMinFreqHz = 20.0f;
static constexpr float kMaxFreqHz = 20000.0f;
static constexpr float kLogMinFreq = 4.32193f;   // log2(20)
static constexpr float kLogMaxFreq = 14.28771f;  // log2(20000)
static constexpr float kLogFreqRange = 9.96578f; // kLogMaxFreq - kLogMinFreq

//==============================================================================
// Sub-flock frequency ratios relative to MIDI note.
//==============================================================================
static constexpr float kSubFlockRatios[4] = {1.0f, 2.0f, 1.5f, 3.0f};

//==============================================================================
// OceanicADSR — alias to the shared fleet envelope.
// StandardADSR provides exponential decay/release (more natural), linear attack
// (snappy), and the identical setParams/noteOn/noteOff/isActive/reset API.
//==============================================================================
using OceanicADSR = StandardADSR;

//==============================================================================
// OceanicLFO — alias to the shared fleet LFO.
// StandardLFO provides identical Sine/Triangle/Saw/Square/S&H shapes and the
// same setRate/setShape/process/reset API. S&H uses the same Knuth TAOCP LCG
// with default seed 12345u — deterministic output is preserved.
//==============================================================================
using OceanicLFO = StandardLFO;

//==============================================================================
// Particle — a single oscillating element in the swarm.
//==============================================================================
struct Particle
{
    float freq = 440.0f; // Hz (20-20000)
    float amp = 0.5f;    // 0-1
    float pan = 0.0f;    // -1 to 1
    float vFreq = 0.0f;  // velocity in log-freq space (octaves/sec)
    float vAmp = 0.0f;   // velocity in amp space
    float vPan = 0.0f;   // velocity in pan space
    int subFlock = 0;    // 0-3
    float phase = 0.0f;  // oscillator phase [0, 1)

    // FIX 3: cached constant-power pan coefficients.
    // Recomputed at control rate when p.pan changes; read every sample.
    float panL = 0.7071068f; // cos(pi/4) — default center
    float panR = 0.7071068f; // sin(pi/4) — default center

    void reset(float f, float a, float p, int flock) noexcept
    {
        freq = f;
        amp = a;
        pan = p;
        vFreq = 0.0f;
        vAmp = 0.0f;
        vPan = 0.0f;
        subFlock = flock;
        phase = 0.0f;
        // Recompute cached pan for new position: map [-1,1] -> [0, pi/2]
        float angle = (p + 1.0f) * 0.25f * 3.14159265358979323846f;
        panL = fastCos(angle);
        panR = fastSin(angle);
    }
};

//==============================================================================
// OceanicVoice — per-voice state including the particle swarm.
//==============================================================================
struct OceanicVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // Target frequency from MIDI note
    float targetFreq = 440.0f;

    // Glide
    float currentTargetFreq = 440.0f;
    float glideCoeff = 1.0f;

    // Particle swarm
    std::array<Particle, kParticlesPerVoice> particles;

    // Envelopes
    OceanicADSR ampEnv;
    OceanicADSR swarmEnv;

    // LFOs (per-voice for free-running independence)
    OceanicLFO lfo1;
    OceanicLFO lfo2;

    // Voice stealing crossfade
    float fadeGain = 1.0f;
    bool fadingOut = false;

    // Control-rate decimation counter
    int controlCounter = 0;

    // Per-voice PRNG state for scatter/noise
    uint32_t rng = 12345u;

    // DC blocker state (per-voice)
    float dcPrevInL = 0.0f;
    float dcPrevOutL = 0.0f;
    float dcPrevInR = 0.0f;
    float dcPrevOutR = 0.0f;

    // Murmuration trigger
    bool murmurate = false;

    // FIX 4: cached inverse-norm factor (1/sqrt(activeParticles)).
    // Recomputed at control rate; used every sample to avoid per-sample sqrt.
    float cachedInvNorm = 1.0f;

    float nextRandom() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return static_cast<float>(rng & 0xFFFF) / 32768.0f - 1.0f; // [-1, 1]
    }

    float nextRandomUni() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return static_cast<float>(rng & 0xFFFF) / 65536.0f; // [0, 1)
    }

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        targetFreq = 440.0f;
        currentTargetFreq = 440.0f;
        fadeGain = 1.0f;
        fadingOut = false;
        controlCounter = 0;
        rng = 12345u;
        dcPrevInL = 0.0f;
        dcPrevOutL = 0.0f;
        dcPrevInR = 0.0f;
        dcPrevOutR = 0.0f;
        murmurate = false;
        cachedInvNorm = 1.0f;
        ampEnv.reset();
        swarmEnv.reset();
        lfo1.reset();
        lfo2.reset();

        for (auto& p : particles)
            p.reset(440.0f, 0.0f, 0.0f, 0);
    }
};

//==============================================================================
// OceanicEngine — the main engine class.
//==============================================================================
class OceanicEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 4;
    static constexpr float kPI = 3.14159265358979323846f;
    static constexpr float kTwoPi = 6.28318530717958647692f;

    //==========================================================================
    // SynthEngine interface — Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);

        // Control rate decimation: ~2kHz
        controlRateDiv = std::max(1, static_cast<int>(srf / 2000.0f));
        controlDt = static_cast<float>(controlRateDiv) / srf;

        // Prepare parameter smoother (5ms time constant)
        paramSmoother.prepare(srf, 0.005f);

        // Pre-compute crossfade rate (5ms)
        crossfadeRate = 1.0f / (0.005f * srf);

        // Allocate output cache for coupling reads
        outputCacheL.resize(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR.resize(static_cast<size_t>(maxBlockSize), 0.0f);

        aftertouch.prepare(sampleRate); // D006: 5ms attack / 20ms release smoothing

        silenceGate.prepare(sampleRate, maxBlockSize);
        silenceGate.setHoldTime(500.0f); // Oceanic has boid reverb tails

        // Initialize voices
        for (auto& v : voices)
            v.reset();
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        envelopeOutput = 0.0f;
        couplingVelocityMod = 0.0f;
        couplingCohesionMod = 0.0f;
        couplingMurmurationTrig = false;

        std::fill(outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill(outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //==========================================================================
    // SynthEngine interface — Audio
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0)
            return;

        // --- ParamSnapshot: read all parameters once per block ---
        const float pSep = loadParam(pSeparation, 0.5f);
        const float pAlign = loadParam(pAlignment, 0.5f);
        const float pCoh = loadParam(pCohesion, 0.5f);
        const float pTeth = loadParam(pTether, 0.5f);
        const float pScat = loadParam(pScatter, 0.3f);
        const int pFlocks = static_cast<int>(loadParam(pSubflocks, 1.0f));
        const float pDamp = loadParam(pDamping, 0.5f);
        const int pWave = static_cast<int>(loadParam(pWaveform, 0.0f));

        const float pAmpA = loadParam(pAmpAttack, 0.01f);
        const float pAmpD = loadParam(pAmpDecay, 0.1f);
        const float pAmpS = loadParam(pAmpSustain, 0.8f);
        const float pAmpR = loadParam(pAmpRelease, 0.3f);
        const float pSwmA = loadParam(pSwarmAttack, 0.05f);
        const float pSwmD = loadParam(pSwarmDecay, 0.3f);
        const float pSwmS = loadParam(pSwarmSustain, 0.6f);
        const float pSwmR = loadParam(pSwarmRelease, 0.5f);

        const float pLfo1R = loadParam(pLfo1Rate, 1.0f);
        const float pLfo1D = loadParam(pLfo1Depth, 0.0f);
        const int pLfo1S = static_cast<int>(loadParam(pLfo1Shape, 0.0f));
        const float pLfo2R = loadParam(pLfo2Rate, 1.0f);
        const float pLfo2D = loadParam(pLfo2Depth, 0.0f);
        const int pLfo2S = static_cast<int>(loadParam(pLfo2Shape, 0.0f));

        const int voiceModeIdx = static_cast<int>(loadParam(pVoiceMode, 2.0f));
        const float glideTime = loadParam(pGlide, 0.0f);

        const float macroChar = loadParam(pMacroCharacter, 0.0f);
        const float macroMove = loadParam(pMacroMovement, 0.0f);
        const float macroCoup = loadParam(pMacroCoupling, 0.0f);
        const float macroSpace = loadParam(pMacroSpace, 0.0f);

        // Determine max polyphony from voice mode
        int maxPoly = kMaxVoices;
        bool monoMode = false;
        switch (voiceModeIdx)
        {
        case 0:
            maxPoly = 1;
            monoMode = true;
            break; // Mono
        case 1:
            maxPoly = 2;
            break; // Poly2
        case 2:
            maxPoly = 4;
            break; // Poly4
        default:
            maxPoly = 4;
            break;
        }

        // Glide coefficient
        float glideCoeff = 1.0f;
        if (glideTime > 0.001f)
            glideCoeff = 1.0f - std::exp(-1.0f / (glideTime * srf));

        // Apply macro offsets to boid parameters
        // D006: aftertouch boosts scatter (sensitivity 0.25) — pressing harder accelerates
        // particle scatter, modelling the chromatophore rate speeding up under pressure.
        // atPressure is computed after the MIDI loop so we use a forward-read pattern:
        // scatter is a block-constant, so we compute it after updateBlock() below.
        // DSP FIX: Autonomous breathing LFO on tether strength — even with LFO depths
        // at 0, the boid school gently contracts and expands. 0.07 Hz (~14s cycle)
        // triangle wave modulates tether ±10%. This addresses "nothing dynamic" seance finding.
        autoBreathPhase += 0.07 / sr;
        if (autoBreathPhase >= 1.0)
            autoBreathPhase -= 1.0;
        float breathMod = (4.0f * std::fabs(static_cast<float>(autoBreathPhase) - 0.5f) - 1.0f) * 0.1f;

        float effectiveSep = clamp(pSep + macroChar * 0.3f, 0.0f, 1.0f);
        float effectiveAlign = clamp(pAlign + macroMove * 0.3f, 0.0f, 1.0f);
        // D006: mod wheel boosts cohesion — CC#1 tightens boid school (sensitivity 0.4)
        float effectiveCoh = clamp(pCoh + couplingCohesionMod + macroCoup * 0.3f + modWheelAmount * 0.4f, 0.0f, 1.0f);
        // DSP FIX: tether breathes autonomously — school contracts/expands gently
        float effectiveTeth = clamp(pTeth + breathMod, 0.0f, 1.0f);
        float effectiveDamp = clamp(pDamp + macroSpace * 0.2f, 0.0f, 1.0f);
        int effectiveFlocks = std::max(1, std::min(4, pFlocks));

        // Reset coupling accumulators
        couplingCohesionMod = 0.0f;
        bool trigMurmuration = couplingMurmurationTrig;
        couplingMurmurationTrig = false;
        float velPerturbation = couplingVelocityMod;
        couplingVelocityMod = 0.0f;

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), maxPoly, monoMode, glideCoeff, effectiveFlocks,
                       pAmpA, pAmpD, pAmpS, pAmpR, pSwmA, pSwmD, pSwmS, pSwmR, pLfo1R, pLfo1D, pLfo1S, pLfo2R, pLfo2D,
                       pLfo2S, pScat);
            }
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
            // D006: channel pressure → aftertouch (applied to particle scatter rate below)
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure(msg.getChannelPressureValue() / 127.0f);
            // D006: CC#1 mod wheel → boid cohesion boost (+0–0.4, boids school together more)
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

        // D006: smooth aftertouch pressure — models the chromatophore rate response.
        // Pressing harder causes faster particle scatter (color shift), exactly like
        // an octopus accelerating its chromatophore cycles under stress or excitement.
        aftertouch.updateBlock(numSamples);
        const float atPressure = aftertouch.getSmoothedPressure(0); // channel-mode: voice 0 holds global value

        // D006: aftertouch boosts separation (scatter) — sensitivity 0.25.
        // Higher pressure = particles scatter further from their neighbors = faster color shift.
        effectiveSep = clamp(effectiveSep + atPressure * 0.25f, 0.0f, 1.0f);

        // Apply murmuration trigger to all active voices
        if (trigMurmuration)
        {
            for (auto& voice : voices)
                if (voice.active)
                    voice.murmurate = true;
        }

        // Apply coupling velocity perturbation to all active voices
        if (std::fabs(velPerturbation) > 0.001f)
        {
            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;
                for (auto& p : voice.particles)
                {
                    p.vFreq += velPerturbation * 0.5f * (voice.nextRandom());
                    p.vAmp += velPerturbation * 0.1f * (voice.nextRandom());
                }
            }
        }

        float peakEnv = 0.0f;

        // --- Render sample loop ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                // --- Voice-stealing crossfade (5ms) ---
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

                // --- Glide (portamento) ---
                voice.currentTargetFreq += (voice.targetFreq - voice.currentTargetFreq) * voice.glideCoeff;

                // --- Envelopes ---
                float ampLevel = voice.ampEnv.process();
                float swarmLevel = voice.swarmEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // --- LFO modulation ---
                float lfo1Val = voice.lfo1.process() * pLfo1D;
                float lfo2Val = voice.lfo2.process() * pLfo2D;

                // LFO1 -> separation modulation, LFO2 -> cohesion modulation
                float modSep = clamp(effectiveSep + lfo1Val * 0.3f, 0.0f, 1.0f);
                float modCoh = clamp(effectiveCoh + lfo2Val * 0.3f, 0.0f, 1.0f);

                // --- Control-rate boid update ---
                voice.controlCounter++;
                if (voice.controlCounter >= controlRateDiv)
                {
                    voice.controlCounter = 0;

                    // Scale boid rules by swarm envelope
                    float sepStr = modSep * swarmLevel * 2.0f;
                    float alignStr = effectiveAlign * swarmLevel * 1.5f;
                    float cohStr = modCoh * swarmLevel * 1.5f;
                    float tethStr = effectiveTeth * 4.0f;
                    float dampCoeff = 1.0f - effectiveDamp * controlDt * 8.0f;
                    dampCoeff = clamp(dampCoeff, 0.8f, 1.0f);

                    // Murmuration: cascade perturbation starting from particle 0
                    if (voice.murmurate)
                    {
                        voice.murmurate = false;
                        float cascade = 1.0f;
                        for (int i = 0; i < kParticlesPerVoice; ++i)
                        {
                            auto& p = voice.particles[static_cast<size_t>(i)];
                            p.vFreq += cascade * voice.nextRandom() * 2.0f;
                            p.vAmp += cascade * voice.nextRandom() * 0.3f;
                            p.vPan += cascade * voice.nextRandom() * 0.5f;
                            cascade *= 0.97f; // Decay cascade influence
                        }
                    }

                    // Compute sub-flock centroids (in log-freq space)
                    float flockCentroidFreq[4] = {};
                    float flockCentroidAmp[4] = {};
                    float flockCentroidPan[4] = {};
                    float flockAvgVFreq[4] = {};
                    float flockAvgVAmp[4] = {};
                    float flockAvgVPan[4] = {};
                    int flockCount[4] = {};

                    for (int i = 0; i < kParticlesPerVoice; ++i)
                    {
                        const auto& p = voice.particles[static_cast<size_t>(i)];
                        int f = p.subFlock;
                        if (f < 0 || f >= effectiveFlocks)
                            continue;
                        float logF = fastLog2(std::max(kMinFreqHz, p.freq));
                        flockCentroidFreq[f] += logF;
                        flockCentroidAmp[f] += p.amp;
                        flockCentroidPan[f] += p.pan;
                        flockAvgVFreq[f] += p.vFreq;
                        flockAvgVAmp[f] += p.vAmp;
                        flockAvgVPan[f] += p.vPan;
                        flockCount[f]++;
                    }

                    for (int f = 0; f < 4; ++f)
                    {
                        if (flockCount[f] > 0)
                        {
                            float inv = 1.0f / static_cast<float>(flockCount[f]);
                            flockCentroidFreq[f] *= inv;
                            flockCentroidAmp[f] *= inv;
                            flockCentroidPan[f] *= inv;
                            flockAvgVFreq[f] *= inv;
                            flockAvgVAmp[f] *= inv;
                            flockAvgVPan[f] *= inv;
                        }
                    }

                    // Attractor targets: MIDI note * sub-flock ratio * pitch bend
                    const float pitchBendRatio = PitchBendUtil::semitonesToFreqRatio(pitchBendNorm * 2.0f);
                    float attractorLogFreq[4];
                    for (int f = 0; f < 4; ++f)
                    {
                        float ratio = kSubFlockRatios[f];
                        attractorLogFreq[f] =
                            fastLog2(std::max(kMinFreqHz, voice.currentTargetFreq * ratio * pitchBendRatio));
                    }

                    // Boid rule radii (in log-freq octaves)
                    const float rSep = 0.3f;   // separation radius: 0.3 octaves
                    const float rAlign = 1.5f; // alignment radius: 1.5 octaves
                    const float rCoh = 3.0f;   // cohesion radius: 3 octaves

                    // Update each particle
                    for (int i = 0; i < kParticlesPerVoice; ++i)
                    {
                        auto& p = voice.particles[static_cast<size_t>(i)];
                        int flock = p.subFlock;
                        if (flock < 0 || flock >= effectiveFlocks)
                        {
                            // Reassign orphan particles to valid flocks
                            p.subFlock = i % effectiveFlocks;
                            flock = p.subFlock;
                        }

                        float logFreq = fastLog2(std::max(kMinFreqHz, p.freq));

                        // --- SEPARATION: repel from nearby particles in same sub-flock ---
                        float sepForceFreq = 0.0f;
                        float sepForceAmp = 0.0f;
                        float sepForcePan = 0.0f;
                        int sepNeighbors = 0;

                        // Sample a subset of particles for separation (every 4th)
                        for (int j = (i + 1) & 3; j < kParticlesPerVoice; j += 4)
                        {
                            if (j == i)
                                continue;
                            const auto& other = voice.particles[static_cast<size_t>(j)];
                            if (other.subFlock != flock)
                                continue;

                            float dFreq = logFreq - fastLog2(std::max(kMinFreqHz, other.freq));
                            float dAmp = p.amp - other.amp;
                            float dPan = p.pan - other.pan;
                            float dist2 = dFreq * dFreq + dAmp * dAmp * 4.0f + dPan * dPan;

                            if (dist2 < rSep * rSep && dist2 > 0.0001f)
                            {
                                // FIX 1: avoid std::sqrt in O(N²) loop.
                                // Quake-style fast inverse-sqrt (rsqrt) — ~0.2% error,
                                // imperceptible for a boid force direction vector.
                                float y = dist2;
                                int32_t ybits;
                                std::memcpy(&ybits, &y, sizeof(ybits));
                                ybits = 0x5F375A86 - (ybits >> 1);
                                float invSqrt;
                                std::memcpy(&invSqrt, &ybits, sizeof(invSqrt));
                                invSqrt *= (1.5f - 0.5f * dist2 * invSqrt * invSqrt); // one Newton-Raphson step
                                // Convert rsqrt(dist2) to 1/(sqrt(dist2)+0.01):
                                // sqrt(dist2) = dist2 * invSqrt; then add 0.01 and invert.
                                float invDist = 1.0f / (dist2 * invSqrt + 0.01f);
                                sepForceFreq += dFreq * invDist;
                                sepForceAmp += dAmp * invDist;
                                sepForcePan += dPan * invDist;
                                sepNeighbors++;
                            }
                        }

                        if (sepNeighbors > 0)
                        {
                            float inv = 1.0f / static_cast<float>(sepNeighbors);
                            sepForceFreq *= inv;
                            sepForceAmp *= inv;
                            sepForcePan *= inv;
                        }

                        // --- ALIGNMENT: match velocity of flock ---
                        float alignForceFreq = flockAvgVFreq[flock] - p.vFreq;
                        float alignForceAmp = flockAvgVAmp[flock] - p.vAmp;
                        float alignForcePan = flockAvgVPan[flock] - p.vPan;

                        // Scale by distance to centroid (only within alignment radius)
                        float distToCentroid = std::fabs(logFreq - flockCentroidFreq[flock]);
                        float alignScale = (distToCentroid < rAlign) ? 1.0f : 0.0f;
                        alignForceFreq *= alignScale;
                        alignForceAmp *= alignScale;
                        alignForcePan *= alignScale;

                        // --- COHESION: pull toward flock centroid ---
                        float cohForceFreq = flockCentroidFreq[flock] - logFreq;
                        float cohForceAmp = flockCentroidAmp[flock] - p.amp;
                        float cohForcePan = flockCentroidPan[flock] - p.pan;

                        // Scale by distance (only within cohesion radius)
                        float cohScale = (distToCentroid < rCoh) ? 1.0f : 0.5f;
                        cohForceFreq *= cohScale;
                        cohForceAmp *= cohScale;
                        cohForcePan *= cohScale;

                        // --- ATTRACTOR: pull toward MIDI target frequency ---
                        float attrForceFreq = attractorLogFreq[flock] - logFreq;
                        float attrForceAmp = 0.5f - p.amp; // Attract amplitude to 0.5
                        float attrForcePan = 0.0f - p.pan; // Attract pan to center

                        // --- Accumulate forces into velocity ---
                        p.vFreq += (sepForceFreq * sepStr + alignForceFreq * alignStr + cohForceFreq * cohStr +
                                    attrForceFreq * tethStr) *
                                   controlDt;

                        p.vAmp += (sepForceAmp * sepStr * 0.5f + alignForceAmp * alignStr * 0.3f +
                                   cohForceAmp * cohStr * 0.5f + attrForceAmp * tethStr * 0.3f) *
                                  controlDt;

                        p.vPan += (sepForcePan * sepStr * 0.5f + alignForcePan * alignStr * 0.3f +
                                   cohForcePan * cohStr * 0.5f + attrForcePan * tethStr * 0.2f) *
                                  controlDt;

                        // --- Apply damping ---
                        p.vFreq *= dampCoeff;
                        p.vAmp *= dampCoeff;
                        p.vPan *= dampCoeff;

                        // --- Clamp velocities to prevent instability ---
                        p.vFreq = clamp(p.vFreq, -5.0f, 5.0f);
                        p.vAmp = clamp(p.vAmp, -2.0f, 2.0f);
                        p.vPan = clamp(p.vPan, -2.0f, 2.0f);

                        // --- Integrate position ---
                        float newLogFreq = logFreq + p.vFreq * controlDt;

                        // Soft bounce at frequency boundaries
                        if (newLogFreq < kLogMinFreq + 0.1f)
                        {
                            newLogFreq = kLogMinFreq + 0.1f;
                            p.vFreq = std::fabs(p.vFreq) * 0.5f;
                        }
                        else if (newLogFreq > kLogMaxFreq - 0.1f)
                        {
                            newLogFreq = kLogMaxFreq - 0.1f;
                            p.vFreq = -std::fabs(p.vFreq) * 0.5f;
                        }

                        // FIX 2: replace std::pow(2.0f, x) with fastPow2(x) (~0.1% error,
                        // indistinguishable for a boid frequency integration step).
                        p.freq = fastPow2(newLogFreq);
                        p.freq = clamp(p.freq, kMinFreqHz, kMaxFreqHz);

                        // Soft clamp amplitude
                        p.amp += p.vAmp * controlDt;
                        if (p.amp < 0.01f)
                        {
                            p.amp = 0.01f;
                            p.vAmp = std::fabs(p.vAmp) * 0.5f;
                        }
                        else if (p.amp > 1.0f)
                        {
                            p.amp = 1.0f;
                            p.vAmp = -std::fabs(p.vAmp) * 0.5f;
                        }

                        // Soft clamp pan
                        p.pan += p.vPan * controlDt;
                        if (p.pan < -1.0f)
                        {
                            p.pan = -1.0f;
                            p.vPan = std::fabs(p.vPan) * 0.5f;
                        }
                        else if (p.pan > 1.0f)
                        {
                            p.pan = 1.0f;
                            p.vPan = -std::fabs(p.vPan) * 0.5f;
                        }

                        // Flush denormals on velocities
                        p.vFreq = flushDenormal(p.vFreq);
                        p.vAmp = flushDenormal(p.vAmp);
                        p.vPan = flushDenormal(p.vPan);

                        // FIX 3: update cached pan coefficients at control rate.
                        // panL/panR are read every sample; computing them here (not per-sample)
                        // eliminates ~512 std::cos/sin calls/sample at Poly4.
                        float panAngleCR = (p.pan + 1.0f) * 0.25f * kPI;
                        p.panL = fastCos(panAngleCR);
                        p.panR = fastSin(panAngleCR);
                    }

                    // FIX 4: cache inverse-norm factor (1/sqrt(activeParticles)).
                    // activeParticles only changes when particles cross the amp threshold,
                    // which happens at control rate — computing sqrt here avoids one
                    // std::sqrt per voice per sample.
                    {
                        int countActive = 0;
                        for (int pi = 0; pi < kParticlesPerVoice; ++pi)
                            if (voice.particles[static_cast<size_t>(pi)].amp >= 0.001f)
                                ++countActive;
                        voice.cachedInvNorm =
                            (countActive > 0) ? (1.0f / std::sqrt(static_cast<float>(countActive))) : 0.0f;
                    }
                } // end control-rate update

                // --- Audio-rate: render particle oscillator bank ---
                float voiceL = 0.0f, voiceR = 0.0f;

                for (int i = 0; i < kParticlesPerVoice; ++i)
                {
                    auto& p = voice.particles[static_cast<size_t>(i)];
                    if (p.amp < 0.001f)
                        continue;

                    // Phase increment for this particle
                    float phaseInc = p.freq / srf;

                    // Generate waveform sample
                    float osc = 0.0f;
                    switch (pWave)
                    {
                    case 0: // Sine
                        osc = fastSin(p.phase * kTwoPi);
                        break;

                    case 1: // Saw with PolyBLEP
                    {
                        osc = 2.0f * p.phase - 1.0f;
                        // PolyBLEP correction at discontinuity
                        float t = p.phase;
                        if (t < phaseInc)
                        {
                            t /= phaseInc;
                            osc -= t + t - t * t - 1.0f;
                        }
                        else if (t > 1.0f - phaseInc)
                        {
                            t = (t - 1.0f) / phaseInc;
                            osc -= t * t + t + t + 1.0f;
                        }
                        break;
                    }

                    case 2: // Pulse (50% duty) with PolyBLEP
                    {
                        osc = (p.phase < 0.5f) ? 1.0f : -1.0f;
                        // PolyBLEP at phase 0
                        float t = p.phase;
                        if (t < phaseInc)
                        {
                            t /= phaseInc;
                            osc += 2.0f * (t + t - t * t - 1.0f);
                        }
                        else if (t > 1.0f - phaseInc)
                        {
                            t = (t - 1.0f) / phaseInc;
                            osc += 2.0f * (t * t + t + t + 1.0f);
                        }
                        // PolyBLEP at phase 0.5
                        t = p.phase - 0.5f;
                        if (t < 0.0f)
                            t += 1.0f;
                        if (t < phaseInc)
                        {
                            t /= phaseInc;
                            osc -= 2.0f * (t + t - t * t - 1.0f);
                        }
                        else if (t > 1.0f - phaseInc)
                        {
                            t = (t - 1.0f) / phaseInc;
                            osc -= 2.0f * (t * t + t + t + 1.0f);
                        }
                        break;
                    }

                    case 3: // Noise (per-particle PRNG)
                    {
                        // Simple noise: hash phase with particle index
                        uint32_t hash = static_cast<uint32_t>(i) * 2654435761u;
                        hash ^= static_cast<uint32_t>(p.phase * 65536.0f);
                        hash = hash * 1664525u + 1013904223u;
                        osc = static_cast<float>(hash & 0xFFFF) / 32768.0f - 1.0f;
                        break;
                    }

                    default:
                        osc = fastSin(p.phase * kTwoPi);
                        break;
                    }

                    // Advance phase
                    p.phase += phaseInc;
                    if (p.phase >= 1.0f)
                        p.phase -= 1.0f;

                    // Apply particle amplitude
                    osc *= p.amp;

                    // FIX 3: use cached pan coefficients (updated at control rate).
                    // Eliminates std::cos + std::sin per particle per sample.
                    voiceL += osc * p.panL;
                    voiceR += osc * p.panR;
                }

                // FIX 4: use cached inverse-norm (updated at control rate).
                // Eliminates std::sqrt per voice per sample; cachedInvNorm is updated
                // in the control-rate block above whenever activeParticles changes.
                voiceL *= voice.cachedInvNorm;
                voiceR *= voice.cachedInvNorm;

                // --- DC Blocker (1-pole highpass at ~5Hz) ---
                constexpr float dcCoeff = 0.9975f;
                float dcOutL = voiceL - voice.dcPrevInL + dcCoeff * voice.dcPrevOutL;
                float dcOutR = voiceR - voice.dcPrevInR + dcCoeff * voice.dcPrevOutR;
                voice.dcPrevInL = voiceL;
                voice.dcPrevOutL = flushDenormal(dcOutL);
                voice.dcPrevInR = voiceR;
                voice.dcPrevOutR = flushDenormal(dcOutR);
                voiceL = dcOutL;
                voiceR = dcOutR;

                // --- Soft limiter (tanh) ---
                voiceL = fastTanh(voiceL * 1.5f);
                voiceR = fastTanh(voiceR * 1.5f);

                // --- Apply amplitude envelope, velocity, and crossfade ---
                float gain = ampLevel * voice.velocity * voice.fadeGain;
                voiceL *= gain;
                voiceR *= gain;

                // Denormal protection on outputs
                voiceL = flushDenormal(voiceL);
                voiceR = flushDenormal(voiceR);

                mixL += voiceL;
                mixR += voiceR;

                peakEnv = std::max(peakEnv, ampLevel);
            }

            // Write to output buffer
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample(0, sample, mixL);
                buffer.addSample(1, sample, mixR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample(0, sample, (mixL + mixR) * 0.5f);
            }

            // Cache for coupling reads
            if (sample < static_cast<int>(outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t>(sample)] = mixL;
                outputCacheR[static_cast<size_t>(sample)] = mixR;
            }
        }

        envelopeOutput = peakEnv;

        // Update active voice count (atomic for thread-safe read from UI)
        int count = 0;
        for (const auto& v : voices)
            if (v.active)
                ++count;
        activeVoices.store(count, std::memory_order_relaxed);

        silenceGate.analyzeBlock(buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples);
    }

    //==========================================================================
    // SynthEngine interface — Coupling
    //==========================================================================

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0)
            return 0.0f;
        auto si = static_cast<size_t>(sampleIndex);
        if (channel == 0 && si < outputCacheL.size())
            return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size())
            return outputCacheR[si];
        if (channel == 2)
            return envelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
        case CouplingType::AudioToFM:
            // External audio -> velocity perturbation on particles
            couplingVelocityMod += amount * 0.5f;
            break;

        case CouplingType::AmpToFilter:
            // External amplitude modulates cohesion strength
            couplingCohesionMod += amount * 0.3f;
            break;

        case CouplingType::RhythmToBlend:
            // Rhythm triggers murmuration cascade
            if (amount > 0.1f)
                couplingMurmurationTrig = true;
            break;

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

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl(params);
        return {params.begin(), params.end()};
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // --- Core Boid Parameters ---
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ocean_separation", 1}, "Oceanic Separation",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ocean_alignment", 1}, "Oceanic Alignment",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ocean_cohesion", 1}, "Oceanic Cohesion",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ocean_tether", 1}, "Oceanic Tether",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ocean_scatter", 1}, "Oceanic Scatter",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ocean_subflocks", 1}, "Oceanic Sub-Flocks", juce::StringArray{"1", "2", "3", "4"}, 0));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ocean_damping", 1}, "Oceanic Damping",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"ocean_waveform", 1}, "Oceanic Waveform",
                                                         juce::StringArray{"Sine", "Saw", "Pulse", "Noise"}, 0));

        // --- Amp Envelope ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ocean_ampAttack", 1}, "Oceanic Amp Attack",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ocean_ampDecay", 1}, "Oceanic Amp Decay",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.1f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ocean_ampSustain", 1}, "Oceanic Amp Sustain",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ocean_ampRelease", 1}, "Oceanic Amp Release",
            juce::NormalisableRange<float>(0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // --- Swarm Envelope ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ocean_swarmEnvAttack", 1}, "Oceanic Swarm Attack",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.05f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ocean_swarmEnvDecay", 1}, "Oceanic Swarm Decay",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ocean_swarmEnvSustain", 1}, "Oceanic Swarm Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.6f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ocean_swarmEnvRelease", 1}, "Oceanic Swarm Release",
            juce::NormalisableRange<float>(0.0f, 20.0f, 0.001f, 0.3f), 0.5f));

        // --- LFO 1 ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ocean_lfo1Rate", 1}, "Oceanic LFO1 Rate",
            juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ocean_lfo1Depth", 1}, "Oceanic LFO1 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ocean_lfo1Shape", 1}, "Oceanic LFO1 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        // --- LFO 2 ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ocean_lfo2Rate", 1}, "Oceanic LFO2 Rate",
            juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ocean_lfo2Depth", 1}, "Oceanic LFO2 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ocean_lfo2Shape", 1}, "Oceanic LFO2 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        // --- Voice Parameters ---
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"ocean_voiceMode", 1},
                                                                      "Oceanic Voice Mode",
                                                                      juce::StringArray{"Mono", "Poly2", "Poly4"}, 2));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ocean_glide", 1}, "Oceanic Glide",
            juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

        // --- Macros ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ocean_macroCharacter", 1}, "Oceanic Macro CHARACTER",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ocean_macroMovement", 1}, "Oceanic Macro MOVEMENT",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ocean_macroCoupling", 1}, "Oceanic Macro COUPLING",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ocean_macroSpace", 1}, "Oceanic Macro SPACE",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pSeparation = apvts.getRawParameterValue("ocean_separation");
        pAlignment = apvts.getRawParameterValue("ocean_alignment");
        pCohesion = apvts.getRawParameterValue("ocean_cohesion");
        pTether = apvts.getRawParameterValue("ocean_tether");
        pScatter = apvts.getRawParameterValue("ocean_scatter");
        pSubflocks = apvts.getRawParameterValue("ocean_subflocks");
        pDamping = apvts.getRawParameterValue("ocean_damping");
        pWaveform = apvts.getRawParameterValue("ocean_waveform");

        pAmpAttack = apvts.getRawParameterValue("ocean_ampAttack");
        pAmpDecay = apvts.getRawParameterValue("ocean_ampDecay");
        pAmpSustain = apvts.getRawParameterValue("ocean_ampSustain");
        pAmpRelease = apvts.getRawParameterValue("ocean_ampRelease");

        pSwarmAttack = apvts.getRawParameterValue("ocean_swarmEnvAttack");
        pSwarmDecay = apvts.getRawParameterValue("ocean_swarmEnvDecay");
        pSwarmSustain = apvts.getRawParameterValue("ocean_swarmEnvSustain");
        pSwarmRelease = apvts.getRawParameterValue("ocean_swarmEnvRelease");

        pLfo1Rate = apvts.getRawParameterValue("ocean_lfo1Rate");
        pLfo1Depth = apvts.getRawParameterValue("ocean_lfo1Depth");
        pLfo1Shape = apvts.getRawParameterValue("ocean_lfo1Shape");
        pLfo2Rate = apvts.getRawParameterValue("ocean_lfo2Rate");
        pLfo2Depth = apvts.getRawParameterValue("ocean_lfo2Depth");
        pLfo2Shape = apvts.getRawParameterValue("ocean_lfo2Shape");

        pVoiceMode = apvts.getRawParameterValue("ocean_voiceMode");
        pGlide = apvts.getRawParameterValue("ocean_glide");

        pMacroCharacter = apvts.getRawParameterValue("ocean_macroCharacter");
        pMacroMovement = apvts.getRawParameterValue("ocean_macroMovement");
        pMacroCoupling = apvts.getRawParameterValue("ocean_macroCoupling");
        pMacroSpace = apvts.getRawParameterValue("ocean_macroSpace");
    }

    //==========================================================================
    // SynthEngine interface — Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Oceanic"; }

    juce::Colour getAccentColour() const override { return juce::Colour(0xFF00B4A0); }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override { return activeVoices.load(std::memory_order_relaxed); }

private:
    SilenceGate silenceGate;

    //==========================================================================
    // Helper: safe parameter load
    //==========================================================================

    static float loadParam(std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    //==========================================================================
    // MIDI note handling.
    //==========================================================================

    void noteOn(int noteNumber, float velocity, int maxPoly, bool monoMode, float glideCoeff, int numSubflocks,
                float ampA, float ampD, float ampS, float ampR, float swmA, float swmD, float swmS, float swmR,
                float lfo1Rate, float lfo1Depth, int lfo1Shape, float lfo2Rate, float lfo2Depth, int lfo2Shape,
                float scatterAmount)
    {
        float freq = midiToHz(static_cast<float>(noteNumber));

        if (monoMode)
        {
            auto& voice = voices[0];
            bool wasActive = voice.active;

            voice.targetFreq = freq;

            if (wasActive)
            {
                // Mono retrigger with glide
                voice.glideCoeff = glideCoeff;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;

                // Retrigger envelopes
                voice.ampEnv.setParams(ampA, ampD, ampS, ampR, srf);
                voice.ampEnv.noteOn();
                voice.swarmEnv.setParams(swmA, swmD, swmS, swmR, srf);
                voice.swarmEnv.noteOn();

                // Apply scatter to existing particles
                applyScatter(voice, velocity, scatterAmount, numSubflocks);
            }
            else
            {
                initVoice(voice, noteNumber, velocity, freq, glideCoeff, numSubflocks, ampA, ampD, ampS, ampR, swmA,
                          swmD, swmS, swmR, lfo1Rate, lfo1Depth, lfo1Shape, lfo2Rate, lfo2Depth, lfo2Shape,
                          scatterAmount);
            }
            return;
        }

        // Polyphonic mode
        int idx = findFreeVoice(maxPoly);
        auto& voice = voices[static_cast<size_t>(idx)];

        // If stealing, initiate crossfade
        if (voice.active)
        {
            voice.fadingOut = true;
            voice.fadeGain = std::min(voice.fadeGain, 0.5f);
        }

        initVoice(voice, noteNumber, velocity, freq, 1.0f, numSubflocks, ampA, ampD, ampS, ampR, swmA, swmD, swmS, swmR,
                  lfo1Rate, lfo1Depth, lfo1Shape, lfo2Rate, lfo2Depth, lfo2Shape, scatterAmount);
    }

    void initVoice(OceanicVoice& voice, int noteNumber, float velocity, float freq, float glideCoeff, int numSubflocks,
                   float ampA, float ampD, float ampS, float ampR, float swmA, float swmD, float swmS, float swmR,
                   float lfo1Rate, float lfo1Depth, int lfo1Shape, float lfo2Rate, float lfo2Depth, int lfo2Shape,
                   float scatterAmount)
    {
        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.targetFreq = freq;
        voice.currentTargetFreq = freq;
        voice.glideCoeff = glideCoeff;
        voice.fadingOut = false;
        voice.fadeGain = 1.0f;
        voice.controlCounter = 0;
        voice.murmurate = false;

        // DC blocker state
        voice.dcPrevInL = 0.0f;
        voice.dcPrevOutL = 0.0f;
        voice.dcPrevInR = 0.0f;
        voice.dcPrevOutR = 0.0f;

        // Initialize PRNG with note-dependent seed
        voice.rng = static_cast<uint32_t>(noteNumber * 7919 + voiceCounter * 104729);

        // Set up envelopes
        voice.ampEnv.setParams(ampA, ampD, ampS, ampR, srf);
        voice.ampEnv.noteOn();
        voice.swarmEnv.setParams(swmA, swmD, swmS, swmR, srf);
        voice.swarmEnv.noteOn();

        // Set up LFOs
        voice.lfo1.setRate(lfo1Rate, srf);
        voice.lfo1.setShape(lfo1Shape);
        voice.lfo1.reset();
        voice.lfo2.setRate(lfo2Rate, srf);
        voice.lfo2.setShape(lfo2Shape);
        voice.lfo2.reset();

        // Initialize particles: distribute across sub-flocks
        int effectiveFlocks = std::max(1, std::min(4, numSubflocks));

        for (int i = 0; i < kParticlesPerVoice; ++i)
        {
            auto& p = voice.particles[static_cast<size_t>(i)];
            int flock = i % effectiveFlocks;
            float ratio = kSubFlockRatios[flock];

            // Start particles clustered near the target frequency with slight spread
            float baseFreq = freq * ratio;
            float spread = voice.nextRandom() * 0.1f; // +/- 10% spread in log-freq
            float particleFreq = baseFreq * fastPow2(spread);
            particleFreq = clamp(particleFreq, kMinFreqHz, kMaxFreqHz);

            float particleAmp = 0.3f + voice.nextRandomUni() * 0.4f; // [0.3, 0.7]
            float particlePan = voice.nextRandom() * 0.5f;           // [-0.5, 0.5]

            p.reset(particleFreq, particleAmp, particlePan, flock);

            // Random initial phase to avoid constructive interference spike
            p.phase = voice.nextRandomUni();
        }

        // Apply note-on scatter
        applyScatter(voice, velocity, scatterAmount, effectiveFlocks);
    }

    void applyScatter(OceanicVoice& voice, float velocity, float scatterAmount, int /*numSubflocks*/) noexcept
    {
        // Note-on scatter: random velocity impulse proportional to MIDI velocity
        float impulseStrength = scatterAmount * velocity * 3.0f;

        for (int i = 0; i < kParticlesPerVoice; ++i)
        {
            auto& p = voice.particles[static_cast<size_t>(i)];
            p.vFreq += voice.nextRandom() * impulseStrength;
            p.vAmp += voice.nextRandom() * impulseStrength * 0.2f;
            p.vPan += voice.nextRandom() * impulseStrength * 0.3f;
        }
    }

    void noteOff(int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.fadingOut)
            {
                voice.ampEnv.noteOff();
                voice.swarmEnv.noteOff();
            }
        }
    }

    int findFreeVoice(int maxPoly) const
    {
        int poly = std::min(maxPoly, kMaxVoices);
        // Delegate to shared LRU allocator (inactive first, then steal oldest).
        return VoiceAllocator::findFreeVoice(const_cast<std::array<OceanicVoice, kMaxVoices>&>(voices), poly);
    }

    static float midiToHz(float midiNote) noexcept { return 440.0f * fastPow2((midiNote - 69.0f) / 12.0f); }

    //==========================================================================
    // Member data
    //==========================================================================

    double sr = 44100.0;
    float srf = 44100.0f;
    ParameterSmoother paramSmoother; // 5ms one-pole smoother (replaces inline smoothCoeff)
    float crossfadeRate = 0.01f;

    // Control rate decimation
    int controlRateDiv = 22;   // ~2kHz at 44.1kHz
    float controlDt = 0.0005f; // 1/2000

    std::array<OceanicVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoices{0};

    // Coupling accumulators
    float envelopeOutput = 0.0f;
    float couplingVelocityMod = 0.0f;
    float couplingCohesionMod = 0.0f;
    bool couplingMurmurationTrig = false;

    // D006: CS-80-inspired poly aftertouch (channel pressure → particle scatter rate)
    PolyAftertouch aftertouch;

    // DSP FIX: autonomous breathing LFO for tether modulation (0.07 Hz)
    double autoBreathPhase = 0.0;

    // ---- D006 Mod wheel — CC#1 boosts boid cohesion (+0–0.4, boids school together more) ----
    float modWheelAmount = 0.0f;
    float pitchBendNorm = 0.0f;

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS parameter pointers
    std::atomic<float>* pSeparation = nullptr;
    std::atomic<float>* pAlignment = nullptr;
    std::atomic<float>* pCohesion = nullptr;
    std::atomic<float>* pTether = nullptr;
    std::atomic<float>* pScatter = nullptr;
    std::atomic<float>* pSubflocks = nullptr;
    std::atomic<float>* pDamping = nullptr;
    std::atomic<float>* pWaveform = nullptr;

    std::atomic<float>* pAmpAttack = nullptr;
    std::atomic<float>* pAmpDecay = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;

    std::atomic<float>* pSwarmAttack = nullptr;
    std::atomic<float>* pSwarmDecay = nullptr;
    std::atomic<float>* pSwarmSustain = nullptr;
    std::atomic<float>* pSwarmRelease = nullptr;

    std::atomic<float>* pLfo1Rate = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo1Shape = nullptr;
    std::atomic<float>* pLfo2Rate = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;
    std::atomic<float>* pLfo2Shape = nullptr;

    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide = nullptr;

    std::atomic<float>* pMacroCharacter = nullptr;
    std::atomic<float>* pMacroMovement = nullptr;
    std::atomic<float>* pMacroCoupling = nullptr;
    std::atomic<float>* pMacroSpace = nullptr;
};

} // namespace xoceanus
