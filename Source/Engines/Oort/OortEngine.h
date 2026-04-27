// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/ModMatrix.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <vector>

namespace xoceanus
{

//==============================================================================
//
//  O O R T   E N G I N E
//  Agent-Based Flocking Synthesis (Reynolds Boids)
//
//  XO_OX Aquatic Identity: The Oort Cloud — a vast halo of primordial bodies
//  orbiting the sun at the edge of the solar system. N autonomous agents drifting
//  on a 1D amplitude axis. When they flock together -> tonal. When they scatter ->
//  noise. Emergent timbre from collective behaviour — not from an oscillator.
//
//  "Each agent = a voice in the crowd."
//  SOLIDARITY controls flocking strength.
//  INTENT: Cage (0, agents ignore each other, pure random) <-> Xenakis (1, full
//  behavioural dynamics, emergent order).
//
//  References (D003):
//    Craig W. Reynolds, "Flocks, Herds, and Schools: A Distributed Behavioral
//    Model" (SIGGRAPH 1987)
//    Vicsek et al., "Novel Type of Phase Transition in a System of Self-Driven
//    Particles" (1995)
//    Cucker & Smale, "Emergent Behavior in Flocks" (2007)
//
//  Architecture: Boids polyphonic waveform synthesis.
//    - Waveform = N agents linearly interpolated (period = pitch cycle)
//    - Per-cycle Boids update: separation, alignment, cohesion forces
//    - Random perturbation scaled by (1 - intent) for Cage component
//    - Poisson event overlay (gusts of wind on the flock)
//    - CytomicSVF filter -> VCA -> output
//
//  Signal Flow:
//    N Agents (Boids simulation, per-cycle update)
//      -> Linear interpolation between agent positions = waveform
//      + Poisson Event Layer (when eventDensity > 0)
//      -> DC Block (when dcBlock=1)
//      -> Wavefold (foldAmt)
//      -> CytomicSVF Filter
//      -> VCA (amp envelope * velocity)
//      -> Output
//
//  Coupling:
//    Output: stereo (ch0=L, ch1=R), flock coherence 0-1 (ch2)
//    Input:  AudioToFM      -> perturbs agent positions (wind gust on flock)
//            AmpToFilter    -> filter cutoff modulation
//            EnvToMorph     -> solidarity (tighter/looser flocking)
//            RhythmToBlend  -> triggers Poisson events
//
//  Gallery code: OORT | Accent: Oort Cloud Violet #9B7FD4 | Prefix: oort_
//
//==============================================================================

static constexpr int   kOortMaxVoices  = 8;
static constexpr int   kOortMaxAgents  = 32;   // maximum agents per voice
static constexpr float kOortTwoPi      = 6.28318530717958647692f;
static constexpr float kOortPi         = 3.14159265358979323846f;

//==============================================================================
// FlockAgent — one autonomous agent on the amplitude axis [-1, +1]
//==============================================================================
struct FlockAgent
{
    float position = 0.0f;  // amplitude, -1 to +1
    float velocity = 0.0f;  // change per cycle step
};

//==============================================================================
// OortPoissonGrain — short transient event triggered by Poisson process
//==============================================================================
struct OortPoissonGrain
{
    bool  active     = false;
    float amplitude  = 0.0f;
    float level      = 0.0f;    // current envelope level
    float decayCoeff = 0.0f;    // per-sample decay coefficient
};

//==============================================================================
// OortVoice — one Boids synthesis voice
//==============================================================================
struct OortVoice
{
    bool  active    = false;
    bool  releasing = false;
    bool  gliding   = false;  // true while sliding from a previous note (Legato mode)
    int   note      = -1;
    float velocity  = 0.0f;  // normalised 0..1
    float keyTrack  = 0.0f;  // (note-60)/60, bipolar

    // ---- Boids flock state ----
    std::vector<FlockAgent> agents;  // sized kOortMaxAgents in prepare()
    int   agentCount  = 16;          // active agents (8..32 step 4)
    float cyclePhase  = 0.0f;        // progress through the period [0, 1)
    float cyclePeriod = 100.0f;      // samples per cycle (determined by pitch)
    int   readIdx     = 0;           // which agent pair we're interpolating
    float agentPhase  = 0.0f;        // progress between readIdx and readIdx+1

    // ---- Glide state ----
    float glideBaseFreq = 440.0f;    // smoothed base frequency

    // ---- Poisson event layer ----
    float poissonTimer = 0.0f;
    OortPoissonGrain grain;

    // ---- DC blocker state ----
    float dcBlockX = 0.0f;
    float dcBlockY = 0.0f;

    // ---- Filter ----
    CytomicSVF filterL;
    CytomicSVF filterR;
    int   filterCoeffCounter = 0;
    int   prevFltType = -1; // sentinel: -1 forces IC reset on first mode change

    // ---- Envelopes ----
    StandardADSR ampEnv;
    StandardADSR filterEnv;

    // ---- Per-voice LFOs ----
    StandardLFO lfo1;
    StandardLFO lfo2;

    // ---- Last LFO output (for mod matrix sources) ----
    float lastLfo1Val = 0.0f;
    float lastLfo2Val = 0.0f;

    // ---- PRNG state (xorshift32, per-voice) ----
    uint32_t rng = 12345u;

    // ---- Coupling accumulators ----
    float audioToFMAccum = 0.0f;

    //--------------------------------------------------------------------------
    // xorshift32 — fast per-voice PRNG
    //--------------------------------------------------------------------------
    float nextRandom() noexcept
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return static_cast<float>(rng & 0xFFFFFF) / 8388608.0f - 1.0f; // [-1, +1]
    }

    float nextRandomUnipolar() noexcept
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return static_cast<float>(rng & 0xFFFFFF) / 16777216.0f; // [0, 1)
    }

    //--------------------------------------------------------------------------
    // sampleDistribution — draw a random step from the selected distribution
    //--------------------------------------------------------------------------
    float sampleDistribution(int distType, float scatter) noexcept
    {
        // Gaussian: Box-Muller (single sample via uniform approximation)
        // Cauchy: gamma * tan(pi * (u - 0.5))
        // Logistic: log(u / (1-u))
        // Uniform: raw
        float u = nextRandomUnipolar();
        // Guard u from edges
        u = std::max(0.001f, std::min(0.999f, u));

        float step = 0.0f;
        switch (distType)
        {
        case 0: // Gaussian — sum of 3 uniforms approximation (central limit)
        {
            float u2 = nextRandomUnipolar();
            float u3 = nextRandomUnipolar();
            step = ((u + u2 + u3) / 3.0f - 0.5f) * 2.0f; // roughly [-1, +1]
            break;
        }
        case 1: // Cauchy — gamma * tan(pi * (u - 0.5)), clamped at ±4σ
        {
            // fastTan valid for |x| < π/4; (u-0.5)∈(-0.5,0.5) so arg ∈ (-π/2, π/2)
            // Clamp argument to ±π/4 to stay in fastTan's accurate range; beyond that
            // we clamp the output anyway (t > 4 gets clamped to 4).
            const float arg = std::max(-kOortPi * 0.25f, std::min(kOortPi * 0.25f, kOortPi * (u - 0.5f)));
            float t = fastTan(arg);
            t = std::max(-4.0f, std::min(4.0f, t));
            step = t * 0.25f; // scale to approx [-1, +1]
            break;
        }
        case 2: // Logistic — log(u/(1-u)) = log2(u/(1-u)) * ln(2)
        {
            // fastLog2 ~0.002% error; multiply by ln(2) to recover natural log
            float lv = fastLog2(u / (1.0f - u)) * 0.6931472f;
            lv = std::max(-4.0f, std::min(4.0f, lv));
            step = lv * 0.25f;
            break;
        }
        default: // Uniform
        {
            step = u * 2.0f - 1.0f;
            break;
        }
        }
        return step * scatter;
    }

    //--------------------------------------------------------------------------
    // initAgents — distribute agents across [-1, +1] on note-on
    //   scatter: amount of initial randomisation
    //   velocity: higher velocity = more initial scatter + excitation energy (D001)
    //--------------------------------------------------------------------------
    void initAgents(int count, float scatter, float vel, int distType) noexcept
    {
        const int n = std::min(count, static_cast<int>(agents.size()));
        agentCount = n;
        if (n <= 0) return;

        // Velocity -> initial scatter + excitation energy (D001)
        const float velScatter = scatter + vel * scatter;

        for (int i = 0; i < n; ++i)
        {
            const float even = (n > 1)
                ? (static_cast<float>(i) / static_cast<float>(n - 1)) * 2.0f - 1.0f
                : 0.0f;
            const float jitter = sampleDistribution(distType, velScatter * 0.2f);
            agents[i].position = std::max(-1.0f, std::min(1.0f, even + jitter));
            // Small random initial velocities scaled by velocity (D001 excitation energy)
            agents[i].velocity = sampleDistribution(distType, 0.02f * (1.0f + vel));
            agents[i].velocity = flushDenormal(agents[i].velocity);
        }

        readIdx    = 0;
        agentPhase = 0.0f;
        cyclePhase = 0.0f;
    }

    //--------------------------------------------------------------------------
    // updateFlock — Reynolds Boids per-cycle update
    //   solidarity: overall flocking strength
    //   intent:     0=Cage (chaos only), 1=Xenakis (full flocking)
    //   scatter:    random perturbation amplitude
    //   sep:        separation force weight
    //   coh:        cohesion force weight
    //   damping:    velocity damping
    //   distType:   random distribution
    //   audioFM:    external perturbation from AudioToFM coupling
    //--------------------------------------------------------------------------
    void updateFlock(float solidarity, float intent, float scatter, float sep,
                     float coh, float damping, int distType, float audioFM) noexcept
    {
        const int n = agentCount;
        if (n <= 0) return;

        // Compute group center for cohesion
        float center = 0.0f;
        for (int i = 0; i < n; ++i)
            center += agents[i].position;
        center /= static_cast<float>(n);

        // Separation radius scales with agent count to prevent overcrowding
        const float sepRadius = 0.2f * (8.0f / std::max(1.0f, static_cast<float>(n)));

        for (int i = 0; i < n; ++i)
        {
            float sepForce = 0.0f;
            int   sepCount = 0;
            float aliSum   = 0.0f;
            int   aliCount = 0;

            const float posI = agents[i].position;
            const float aliRadius = sepRadius * 2.0f;

            for (int j = 0; j < n; ++j)
            {
                if (j == i) continue;
                const float dist  = posI - agents[j].position;
                const float absDist = std::fabs(dist);

                // Separation: inverse distance repulsion when too close
                if (absDist < sepRadius)
                {
                    const float sign = (dist >= 0.0f) ? 1.0f : -1.0f;
                    sepForce += sign / std::max(absDist, 0.001f);
                    ++sepCount;
                }

                // Alignment: match neighbours' velocity
                if (absDist < aliRadius)
                {
                    aliSum += agents[j].velocity;
                    ++aliCount;
                }
            }

            // Separation force — normalise by neighbour count to keep magnitude
            // agent-count-independent (was unbounded: 32 agents all within radius
            // could accumulate 32× more force than 8 agents).
            const float fSep = (sepCount > 0)
                ? (sepForce / static_cast<float>(sepCount)) * sep * 0.01f
                : 0.0f;

            // Alignment force
            const float fAli = (aliCount > 0)
                ? (aliSum / static_cast<float>(aliCount) - agents[i].velocity) * 0.5f
                : 0.0f;

            // Cohesion: move toward group center
            const float fCoh = (center - posI) * coh;

            // Flocking force (scaled by solidarity and intent)
            const float flockForce = (fSep + fAli + fCoh) * solidarity * intent;

            // Random perturbation (Cage component — scales with 1-intent)
            const float chaos = sampleDistribution(distType, scatter) * (1.0f - intent);

            // AudioToFM coupling: external wind gust on the flock
            const float wind = audioFM * 0.1f;

            // Update agent
            agents[i].velocity += flockForce + chaos + wind;
            agents[i].velocity *= damping;
            agents[i].velocity  = flushDenormal(agents[i].velocity);

            agents[i].position += agents[i].velocity;
            agents[i].position  = flushDenormal(agents[i].position);

            // Reflect at boundaries (bounce, don't hard-clamp)
            if (agents[i].position > 1.0f)
            {
                agents[i].position = 2.0f - agents[i].position;
                agents[i].velocity *= -1.0f;
            }
            if (agents[i].position < -1.0f)
            {
                agents[i].position = -2.0f - agents[i].position;
                agents[i].velocity *= -1.0f;
            }
            // Safety clamp (after reflection)
            agents[i].position = std::max(-1.0f, std::min(1.0f, agents[i].position));
        }

        // Decay the AudioToFM accumulator
        audioToFMAccum *= 0.999f;
        audioToFMAccum  = flushDenormal(audioToFMAccum);
    }

    //--------------------------------------------------------------------------
    // computeCoherence — flock coherence metric for coupling output ch2
    //   = 1 - min(1, stddev(positions) * 4)
    //--------------------------------------------------------------------------
    float computeCoherence() const noexcept
    {
        const int n = agentCount;
        if (n <= 1) return 1.0f;

        float mean = 0.0f;
        for (int i = 0; i < n; ++i) mean += agents[i].position;
        mean /= static_cast<float>(n);

        float variance = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float d = agents[i].position - mean;
            variance += d * d;
        }
        variance /= static_cast<float>(n);
        // Skip std::sqrt: work in variance domain. stddev*4 >= 1 ↔ variance >= 0.0625 (1/16)
        // coherence = 1 - min(1, stddev*4) = 1 - min(1, sqrt(variance)*4)
        // Equivalent: use sqrt(variance) approximation via fastPow2(fastLog2(variance)*0.5f)
        // Simpler: since we only need the final value 0..1, compute without sqrt:
        const float stddev = (variance > 0.0f) ? fastPow2(fastLog2(variance) * 0.5f) : 0.0f;

        return std::max(0.0f, 1.0f - std::min(1.0f, stddev * 4.0f));
    }

    //--------------------------------------------------------------------------
    // reset
    //--------------------------------------------------------------------------
    void reset(float sampleRate) noexcept
    {
        active    = false;
        releasing = false;
        gliding   = false;
        note      = -1;
        velocity  = 0.0f;
        keyTrack  = 0.0f;

        agentCount  = 16;
        cyclePhase  = 0.0f;
        cyclePeriod = (sampleRate > 0.0f) ? sampleRate / 440.0f : 100.0f;
        readIdx     = 0;
        agentPhase  = 0.0f;
        glideBaseFreq = 440.0f;

        poissonTimer = 0.0f;
        grain.active = false;
        grain.level  = 0.0f;

        dcBlockX = 0.0f;
        dcBlockY = 0.0f;

        filterL.reset();
        filterR.reset();
        filterCoeffCounter = 15; // Fire on first sample of note (was 0: stale for first 15 samples)

        ampEnv.reset();
        filterEnv.reset();
        lfo1.reset();
        lfo2.reset();
        lastLfo1Val = 0.0f;
        lastLfo2Val = 0.0f;
        audioToFMAccum = 0.0f;

        // Initialise agents to evenly spaced sine-like distribution
        const int n = static_cast<int>(agents.size());
        if (n <= 0) return;
        for (int i = 0; i < n; ++i)
        {
            const float phase = static_cast<float>(i) / static_cast<float>(n);
            agents[i].position = fastSin(phase * kOortTwoPi);
            agents[i].velocity = 0.0f;
        }
    }
};

//==============================================================================
// OortEngine — Agent-Based Flocking Synthesis
//==============================================================================
class OortEngine : public SynthEngine
{
public:
    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRateFloat = (sampleRate > 0.0) ? static_cast<float>(sampleRate) : 0.0f;
        maxBlock = maxBlockSize;

        for (int i = 0; i < kOortMaxVoices; ++i)
        {
            voices[i].agents.resize(kOortMaxAgents);
            voices[i].rng = 12345u + static_cast<uint32_t>(i) * 31337u;
            voices[i].reset(sampleRateFloat);
        }

        // Coupling accumulators
        couplingFMVal      = 0.0f;
        couplingAmpFilter  = 0.0f;
        couplingEnvSolid   = 0.0f;
        couplingRhythm     = 0.0f;

        modWheelValue   = 0.0f;
        aftertouchValue = 0.0f;

        lastSampleL    = 0.0f;
        lastSampleR    = 0.0f;
        lastCoherence  = 0.0f;

        activeVoiceCount_.store(0, std::memory_order_relaxed);

        // Flocking synths can have long evolving tails
        prepareSilenceGate(sampleRate, maxBlockSize, 400.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset(sampleRateFloat);

        couplingFMVal      = 0.0f;
        couplingAmpFilter  = 0.0f;
        couplingEnvSolid   = 0.0f;
        couplingRhythm     = 0.0f;
        modWheelValue      = 0.0f;
        aftertouchValue    = 0.0f;
        lastSampleL  = 0.0f;
        lastSampleR  = 0.0f;
        lastCoherence = 0.0f;
        stealIdx     = 0;
        activeVoiceCount_.store(0, std::memory_order_relaxed);
    }

    //==========================================================================
    // Coupling
    //==========================================================================

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override
    {
        if (sourceBuffer == nullptr || numSamples <= 0)
            return;

        switch (type)
        {
        case CouplingType::AudioToFM:
        {
            // Perturbs agent positions (wind gust on the flock)
            const float last = sourceBuffer[numSamples - 1] * amount;
            couplingFMVal = last;
            for (auto& v : voices)
                if (v.active)
                    v.audioToFMAccum = last;
            break;
        }
        case CouplingType::AmpToFilter:
        {
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += std::fabs(sourceBuffer[i]);
            rms /= static_cast<float>(numSamples);
            couplingAmpFilter = rms * amount;
            break;
        }
        case CouplingType::EnvToMorph:
        {
            // Coupling envelope modulates solidarity (tighter/looser flocking)
            couplingEnvSolid = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        case CouplingType::RhythmToBlend:
        {
            // Coupling rhythm triggers Poisson events
            couplingRhythm = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        default:
        {
            // Generic fallback: treat as AmpToFilter. Assign (not +=) so repeated
            // unknown coupling types cannot accumulate across blocks.
            float mav = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                mav += std::fabs(sourceBuffer[i]);
            mav /= static_cast<float>(numSamples);
            couplingAmpFilter = mav * amount;
            break;
        }
        }
    }

    //==========================================================================
    // getSampleForCoupling — O(1) cached output
    //==========================================================================

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        if (channel == 0) return lastSampleL;
        if (channel == 1) return lastSampleR;
        if (channel == 2) return lastCoherence; // flock coherence 0..1
        return 0.0f;
    }

    //==========================================================================
    // Parameters
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using NRange = juce::NormalisableRange<float>;
        using AP  = juce::AudioParameterFloat;
        using APC = juce::AudioParameterChoice;
        using PID = juce::ParameterID;

        // ---- Group A: Flock Core (8 params) ----
        static const juce::StringArray kAgentChoices {"8","12","16","20","24","28","32"};
        params.push_back(std::make_unique<APC>(PID{"oort_agentCount", 1}, "oort_agentCount",
            kAgentChoices, 2)); // default index 2 = "16" agents

        params.push_back(std::make_unique<AP>(PID{"oort_solidarity", 1}, "oort_solidarity",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"oort_intent", 1}, "oort_intent",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"oort_scatter", 1}, "oort_scatter",
            NRange{0.0f, 1.0f, 0.001f}, 0.3f));

        static const juce::StringArray kDistTypes {"Gaussian","Cauchy","Logistic","Uniform"};
        params.push_back(std::make_unique<APC>(PID{"oort_distType", 1}, "oort_distType",
            kDistTypes, 0));

        params.push_back(std::make_unique<AP>(PID{"oort_damping", 1}, "oort_damping",
            NRange{0.8f, 1.0f, 0.0001f}, 0.95f));

        params.push_back(std::make_unique<AP>(PID{"oort_separation", 1}, "oort_separation",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"oort_cohesion", 1}, "oort_cohesion",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        // ---- Group B: Waveform Shaping (5 params) ----
        params.push_back(std::make_unique<AP>(PID{"oort_symmetry", 1}, "oort_symmetry",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"oort_foldAmt", 1}, "oort_foldAmt",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));

        static const juce::StringArray kDCOnOff {"Off","On"};
        params.push_back(std::make_unique<APC>(PID{"oort_dcBlock", 1}, "oort_dcBlock",
            kDCOnOff, 1));

        params.push_back(std::make_unique<AP>(PID{"oort_pitchTrack", 1}, "oort_pitchTrack",
            NRange{0.0f, 1.0f, 0.001f}, 1.0f));

        {
            NRange r{20.0f, 800.0f, 0.1f};
            r.setSkewForCentre(110.0f); // Fix: 0.3 was below minimum (20); 110 Hz centre gives musical feel
            params.push_back(std::make_unique<AP>(PID{"oort_delayBase", 1}, "oort_delayBase", r, 110.0f));
        }

        // ---- Group C: Poisson Events (4 params) ----
        params.push_back(std::make_unique<AP>(PID{"oort_eventDensity", 1}, "oort_eventDensity",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"oort_eventAmp", 1}, "oort_eventAmp",
            NRange{0.0f, 1.0f, 0.001f}, 0.8f));

        {
            NRange r{0.001f, 0.5f, 0.0001f};
            r.setSkewForCentre(0.02f);
            params.push_back(std::make_unique<AP>(PID{"oort_eventDecay", 1}, "oort_eventDecay", r, 0.02f));
        }

        params.push_back(std::make_unique<AP>(PID{"oort_densityJitter", 1}, "oort_densityJitter",
            NRange{0.0f, 1.0f, 0.001f}, 0.3f));

        // ---- Group D: Filter + Filter Envelope (9 params) ----
        {
            NRange r{20.0f, 20000.0f, 0.1f};
            r.setSkewForCentre(1000.0f); // Fix: 0.3 was below minimum (20); 1 kHz centre is perceptually centred
            params.push_back(std::make_unique<AP>(PID{"oort_fltCutoff", 1}, "oort_fltCutoff", r, 8000.0f));
        }

        params.push_back(std::make_unique<AP>(PID{"oort_fltReso", 1}, "oort_fltReso",
            NRange{0.0f, 1.0f, 0.001f}, 0.1f));

        static const juce::StringArray kFltTypes {"LP","HP","BP","Notch"};
        params.push_back(std::make_unique<APC>(PID{"oort_fltType", 1}, "oort_fltType",
            kFltTypes, 0));

        params.push_back(std::make_unique<AP>(PID{"oort_fltEnvAmt", 1}, "oort_fltEnvAmt",
            NRange{-1.0f, 1.0f, 0.001f}, 0.3f));

        params.push_back(std::make_unique<AP>(PID{"oort_fltKeyTrack", 1}, "oort_fltKeyTrack",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_fltAtk", 1}, "oort_fltAtk", r, 0.01f));
        }
        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_fltDec", 1}, "oort_fltDec", r, 0.3f));
        }

        params.push_back(std::make_unique<AP>(PID{"oort_fltSus", 1}, "oort_fltSus",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));

        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_fltRel", 1}, "oort_fltRel", r, 0.4f));
        }

        // ---- Group E: Amp Envelope (5 params) ----
        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_ampAtk", 1}, "oort_ampAtk", r, 0.005f));
        }
        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_ampDec", 1}, "oort_ampDec", r, 0.4f));
        }

        params.push_back(std::make_unique<AP>(PID{"oort_ampSus", 1}, "oort_ampSus",
            NRange{0.0f, 1.0f, 0.001f}, 0.7f));

        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_ampRel", 1}, "oort_ampRel", r, 0.6f));
        }

        params.push_back(std::make_unique<AP>(PID{"oort_velTimbre", 1}, "oort_velTimbre",
            NRange{0.0f, 1.0f, 0.001f}, 0.6f));

        // ---- Group F: LFOs (8 params) ----
        static const juce::StringArray kLFOShapes  {"Sine","Tri","Saw","Square","S&H"};
        static const juce::StringArray kLFOTargets {"Solidarity","Intent","Filter Cutoff","Event Density"};

        {
            NRange r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_lfo1Rate", 1}, "oort_lfo1Rate", r, 0.5f));
        }
        params.push_back(std::make_unique<AP>(PID{"oort_lfo1Depth", 1}, "oort_lfo1Depth",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"oort_lfo1Shape", 1}, "oort_lfo1Shape",
            kLFOShapes, 0));
        params.push_back(std::make_unique<APC>(PID{"oort_lfo1Target", 1}, "oort_lfo1Target",
            kLFOTargets, 0)); // Solidarity

        {
            NRange r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_lfo2Rate", 1}, "oort_lfo2Rate", r, 0.12f));
        }
        params.push_back(std::make_unique<AP>(PID{"oort_lfo2Depth", 1}, "oort_lfo2Depth",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"oort_lfo2Shape", 1}, "oort_lfo2Shape",
            kLFOShapes, 2)); // Saw
        params.push_back(std::make_unique<APC>(PID{"oort_lfo2Target", 1}, "oort_lfo2Target",
            kLFOTargets, 1)); // Intent

        // ---- Group G: Mod Matrix (4 slots x 3 params = 12 params) ----
        static const juce::StringArray kOortModDests {
            "Off", "Filter Cutoff", "Solidarity", "Intent",
            "Event Density", "Scatter", "Amp Level"
        };
        ModMatrix<4>::addParameters(params, "oort_", "Oort", kOortModDests);

        // ---- Group H: Macros + Voice (7 params) ----
        // M1=SOLIDARITY: overall flocking strength
        params.push_back(std::make_unique<AP>(PID{"oort_macro1", 1}, "oort_macro1",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));
        // M2=INTENT: Cage<->Xenakis axis
        params.push_back(std::make_unique<AP>(PID{"oort_macro2", 1}, "oort_macro2",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));
        // M3=DRIFT: scatter + damping inverse
        params.push_back(std::make_unique<AP>(PID{"oort_macro3", 1}, "oort_macro3",
            NRange{0.0f, 1.0f, 0.001f}, 0.3f));
        // M4=SPACE: filter cutoff
        params.push_back(std::make_unique<AP>(PID{"oort_macro4", 1}, "oort_macro4",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        static const juce::StringArray kVoiceModes {"Mono","Legato","Poly4","Poly8"};
        params.push_back(std::make_unique<APC>(PID{"oort_voiceMode", 1}, "oort_voiceMode",
            kVoiceModes, 2));

        {
            NRange r{0.0f, 2.0f, 0.001f};
            r.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<AP>(PID{"oort_glide", 1}, "oort_glide", r, 0.0f));
        }

        static const juce::StringArray kGlideModes {"Legato","Always"};
        params.push_back(std::make_unique<APC>(PID{"oort_glideMode", 1}, "oort_glideMode",
            kGlideModes, 0));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters(params);
        return {params.begin(), params.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        // Group A
        pAgentCount    = apvts.getRawParameterValue("oort_agentCount");
        pSolidarity    = apvts.getRawParameterValue("oort_solidarity");
        pIntent        = apvts.getRawParameterValue("oort_intent");
        pScatter       = apvts.getRawParameterValue("oort_scatter");
        pDistType      = apvts.getRawParameterValue("oort_distType");
        pDamping       = apvts.getRawParameterValue("oort_damping");
        pSeparation    = apvts.getRawParameterValue("oort_separation");
        pCohesion      = apvts.getRawParameterValue("oort_cohesion");

        // Group B
        pSymmetry      = apvts.getRawParameterValue("oort_symmetry");
        pFoldAmt       = apvts.getRawParameterValue("oort_foldAmt");
        pDCBlock       = apvts.getRawParameterValue("oort_dcBlock");
        pPitchTrack    = apvts.getRawParameterValue("oort_pitchTrack");
        pDelayBase     = apvts.getRawParameterValue("oort_delayBase");

        // Group C
        pEventDensity  = apvts.getRawParameterValue("oort_eventDensity");
        pEventAmp      = apvts.getRawParameterValue("oort_eventAmp");
        pEventDecay    = apvts.getRawParameterValue("oort_eventDecay");
        pDensityJitter = apvts.getRawParameterValue("oort_densityJitter");

        // Group D
        pFltCutoff     = apvts.getRawParameterValue("oort_fltCutoff");
        pFltReso       = apvts.getRawParameterValue("oort_fltReso");
        pFltType       = apvts.getRawParameterValue("oort_fltType");
        pFltEnvAmt     = apvts.getRawParameterValue("oort_fltEnvAmt");
        pFltKeyTrack   = apvts.getRawParameterValue("oort_fltKeyTrack");
        pFltAtk        = apvts.getRawParameterValue("oort_fltAtk");
        pFltDec        = apvts.getRawParameterValue("oort_fltDec");
        pFltSus        = apvts.getRawParameterValue("oort_fltSus");
        pFltRel        = apvts.getRawParameterValue("oort_fltRel");

        // Group E
        pAmpAtk        = apvts.getRawParameterValue("oort_ampAtk");
        pAmpDec        = apvts.getRawParameterValue("oort_ampDec");
        pAmpSus        = apvts.getRawParameterValue("oort_ampSus");
        pAmpRel        = apvts.getRawParameterValue("oort_ampRel");
        pVelTimbre     = apvts.getRawParameterValue("oort_velTimbre");

        // Group F
        pLfo1Rate      = apvts.getRawParameterValue("oort_lfo1Rate");
        pLfo1Depth     = apvts.getRawParameterValue("oort_lfo1Depth");
        pLfo1Shape     = apvts.getRawParameterValue("oort_lfo1Shape");
        pLfo1Target    = apvts.getRawParameterValue("oort_lfo1Target");
        pLfo2Rate      = apvts.getRawParameterValue("oort_lfo2Rate");
        pLfo2Depth     = apvts.getRawParameterValue("oort_lfo2Depth");
        pLfo2Shape     = apvts.getRawParameterValue("oort_lfo2Shape");
        pLfo2Target    = apvts.getRawParameterValue("oort_lfo2Target");

        // Group G
        modMatrix.attachParameters(apvts, "oort_");

        // Group H
        pMacro1    = apvts.getRawParameterValue("oort_macro1");
        pMacro2    = apvts.getRawParameterValue("oort_macro2");
        pMacro3    = apvts.getRawParameterValue("oort_macro3");
        pMacro4    = apvts.getRawParameterValue("oort_macro4");
        pVoiceMode = apvts.getRawParameterValue("oort_voiceMode");
        pGlide     = apvts.getRawParameterValue("oort_glide");
        pGlideMode = apvts.getRawParameterValue("oort_glideMode");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String  getEngineId()     const override { return "Oort"; }
    juce::Colour  getAccentColour() const override { return juce::Colour(0xFF9B7FD4); } // Oort Cloud Violet
    int           getMaxVoices()    const override { return kOortMaxVoices; }

    //==========================================================================
    // renderBlock
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (sampleRateFloat <= 0.0f) return;
        if (numSamples <= 0) return;

        // ---- SilenceGate: wake on note-on, bail if silent ----
        for (const auto& md : midi)
        {
            if (md.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        }
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            return;
        }

        // ---- Snapshot parameters (block-rate) ----
        const float macro1 = pMacro1 ? pMacro1->load() : 0.5f;
        const float macro2 = pMacro2 ? pMacro2->load() : 0.5f;
        const float macro3 = pMacro3 ? pMacro3->load() : 0.3f;
        const float macro4 = pMacro4 ? pMacro4->load() : 0.5f;

        // M1=SOLIDARITY: overall flocking strength multiplier
        // M2=INTENT: Cage<->Xenakis multiplier
        // M3=DRIFT: scatter + inverse damping
        // M4=SPACE: filter cutoff multiplier

        // Group A: Flock core
        const int agentCountIdx = pAgentCount ? static_cast<int>(pAgentCount->load()) : 2;
        // Indices 0..6 -> 8,12,16,20,24,28,32
        const int agentCountBase  = 8 + agentCountIdx * 4;
        const int agentCount      = std::clamp(agentCountBase, 8, kOortMaxAgents);

        float solidarity = pSolidarity ? pSolidarity->load() : 0.5f;
        // M1=SOLIDARITY: bipolar — macro at 0.5 is neutral (±0.5 range = ±50% modulation)
        solidarity = std::clamp(solidarity * (1.0f + (macro1 - 0.5f) * 2.0f), 0.0f, 2.0f);
        // EnvToMorph coupling modulates solidarity (tighter/looser flocking)
        solidarity = std::clamp(solidarity + couplingEnvSolid, 0.0f, 2.0f);

        float intent = pIntent ? pIntent->load() : 0.5f;
        // M2=INTENT: bipolar — macro at 0.5 is neutral; ±0.5 range gives ±0.5 intent offset
        intent = std::clamp(intent + (macro2 - 0.5f) * 0.5f, 0.0f, 1.0f);

        // M3=DRIFT: scatter increase + damping decrease
        float scatter = pScatter ? pScatter->load() : 0.3f;
        // M3=DRIFT: bipolar — macro at 0.5 is neutral; CW adds up to +scatter, CCW subtracts
        scatter = std::clamp(scatter * (1.0f + (macro3 - 0.5f) * 2.0f), 0.0f, 2.0f);

        const int   distType   = pDistType   ? static_cast<int>(pDistType->load())   : 0;
        float       damping    = pDamping    ? pDamping->load()    : 0.95f;
        // M3=DRIFT: bipolar — macro at 0.5 is neutral; damping decreases above 0.5
        damping = std::clamp(damping - (macro3 - 0.5f) * 0.15f, 0.5f, 1.0f);

        const float separation = pSeparation ? pSeparation->load() : 0.5f;
        const float cohesion   = pCohesion   ? pCohesion->load()   : 0.5f;

        // Group B
        const float symmetry   = pSymmetry   ? pSymmetry->load()   : 0.5f;
        const float foldAmt    = pFoldAmt    ? pFoldAmt->load()    : 0.0f;
        const int   dcBlock    = pDCBlock    ? static_cast<int>(pDCBlock->load())  : 1;
        const float pitchTrack = pPitchTrack ? pPitchTrack->load() : 1.0f;
        const float delayBase  = pDelayBase  ? pDelayBase->load()  : 110.0f;

        // Group C
        float eventDensity = pEventDensity ? pEventDensity->load() : 0.0f;
        // RhythmToBlend coupling drives Poisson event density.
        // Use fabs: density is always positive, but coupling can arrive bipolar;
        // both positive and negative pulses should raise event rate.
        // (std::fabs is correct here — unlike vowelY in Ondine, density has no
        //  directional meaning, so magnitude-only application is intentional.)
        eventDensity = std::clamp(eventDensity + std::fabs(couplingRhythm), 0.0f, 1.0f);
        const float eventAmp      = pEventAmp      ? pEventAmp->load()      : 0.8f;
        const float eventDecaySec = pEventDecay    ? pEventDecay->load()    : 0.02f;
        const float densityJitter = pDensityJitter ? pDensityJitter->load() : 0.3f;

        // Group D: filter
        float baseCutoff = (pFltCutoff ? pFltCutoff->load() : 8000.0f);
        // M4=SPACE: filter cutoff
        baseCutoff *= (0.5f + macro4);
        // AmpToFilter coupling raises cutoff
        baseCutoff += couplingAmpFilter * 8000.0f;
        baseCutoff = std::clamp(baseCutoff, 20.0f, 20000.0f);

        const float fltReso     = pFltReso    ? pFltReso->load()    : 0.1f;
        const int   fltType     = pFltType    ? static_cast<int>(pFltType->load())  : 0;
        const float fltEnvAmt   = pFltEnvAmt  ? pFltEnvAmt->load()  : 0.3f;
        const float fltKeyTrack = pFltKeyTrack? pFltKeyTrack->load(): 0.5f;
        const float fltAtk      = pFltAtk     ? pFltAtk->load()     : 0.01f;
        const float fltDec      = pFltDec     ? pFltDec->load()     : 0.3f;
        const float fltSus      = pFltSus     ? pFltSus->load()     : 0.0f;
        const float fltRel      = pFltRel     ? pFltRel->load()     : 0.4f;

        // Group E: amp envelope
        const float ampAtk   = pAmpAtk  ? pAmpAtk->load()  : 0.005f;
        const float ampDec   = pAmpDec  ? pAmpDec->load()  : 0.4f;
        const float ampSus   = pAmpSus  ? pAmpSus->load()  : 0.7f;
        const float ampRel   = pAmpRel  ? pAmpRel->load()  : 0.6f;
        const float velTimbre= pVelTimbre? pVelTimbre->load(): 0.6f;

        // Group F: LFOs (floor 0.01 Hz, D005)
        const float lfo1Rate  = std::max(0.01f, pLfo1Rate  ? pLfo1Rate->load()  : 0.5f);
        const float lfo1Depth = pLfo1Depth ? pLfo1Depth->load() : 0.0f;
        const int   lfo1Shape = pLfo1Shape ? static_cast<int>(pLfo1Shape->load()) : 0;
        const int   lfo1Tgt   = pLfo1Target? static_cast<int>(pLfo1Target->load()): 0;

        const float lfo2Rate  = std::max(0.01f, pLfo2Rate  ? pLfo2Rate->load()  : 0.12f);
        const float lfo2Depth = pLfo2Depth ? pLfo2Depth->load() : 0.0f;
        const int   lfo2Shape = pLfo2Shape ? static_cast<int>(pLfo2Shape->load()) : 2;
        const int   lfo2Tgt   = pLfo2Target? static_cast<int>(pLfo2Target->load()): 1;

        // Voice / glide
        const int   voiceMode = pVoiceMode ? static_cast<int>(pVoiceMode->load()) : 2;
        const float glideTime = pGlide     ? pGlide->load()     : 0.0f;
        const int   glideMode = pGlideMode ? static_cast<int>(pGlideMode->load()) : 0;

        // Glide coefficient — use it (Lesson 8)
        const float glideCoeff = (glideTime > 0.0001f)
            ? fastExp(-1.0f / (glideTime * sampleRateFloat))
            : 0.0f;

        // Poisson lambda (events/sample)
        const float lambdaPerSample = (eventDensity > 0.0f)
            ? (eventDensity * 100.0f) / sampleRateFloat
            : 0.0f;

        // Per-sample event decay coefficient
        const float grainDecayCoeff = (eventDecaySec > 0.001f)
            ? 1.0f - fastExp(-1.0f / (eventDecaySec * sampleRateFloat))
            : 0.5f;

        // ---- Gather mod matrix sources from active voices ----
        {
            float lfo1Sum = 0.0f, lfo2Sum = 0.0f, envSum = 0.0f;
            float velSum  = 0.0f, ktSum   = 0.0f;
            int count = 0;
            for (auto& v : voices)
            {
                if (v.active)
                {
                    lfo1Sum += v.lastLfo1Val;
                    lfo2Sum += v.lastLfo2Val;
                    envSum  += v.ampEnv.getLevel();
                    velSum  += v.velocity;
                    ktSum   += v.keyTrack;
                    ++count;
                }
            }
            if (count > 0)
            {
                const float inv = 1.0f / static_cast<float>(count);
                blockModSrc.lfo1     = lfo1Sum * inv;
                blockModSrc.lfo2     = lfo2Sum * inv;
                blockModSrc.env      = envSum  * inv;
                blockModSrc.velocity = velSum  * inv;
                blockModSrc.keyTrack = ktSum   * inv;
            }
            blockModSrc.modWheel   = modWheelValue;
            blockModSrc.aftertouch = aftertouchValue;
        }

        // ---- Apply mod matrix ----
        // Destinations: 0=Off, 1=Filter Cutoff, 2=Solidarity, 3=Intent,
        //               4=Event Density, 5=Scatter, 6=Amp Level
        float modDestOffsets[7] = {};
        modMatrix.apply(blockModSrc, modDestOffsets);

        baseCutoff   = std::clamp(baseCutoff   + modDestOffsets[1] * 8000.0f, 20.0f, 20000.0f);
        solidarity   = std::clamp(solidarity   + modDestOffsets[2],            0.0f,   2.0f);
        intent       = std::clamp(intent       + modDestOffsets[3],            0.0f,   1.0f);
        eventDensity = std::clamp(eventDensity + modDestOffsets[4],            0.0f,   1.0f);
        scatter      = std::clamp(scatter      + modDestOffsets[5] * 0.5f,     0.0f,   2.0f);
        const float modAmpLevel = modDestOffsets[6];

        // CC1 mod wheel -> scatter (D006: expression input)
        scatter = std::clamp(scatter + modWheelValue * 0.3f, 0.0f, 2.0f);
        // Aftertouch -> intent (timbral change, D001/D006)
        intent  = std::clamp(intent  + aftertouchValue * 0.3f, 0.0f, 1.0f);

        // ---- Output buffers ----
        auto* writeL = buffer.getWritePointer(0);
        auto* writeR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : writeL;
        // ADDITIVE: do not clear — engine adds to existing buffer (slot chain convention)

        // ---- MIDI processing interleaved with rendering ----
        int midiSamplePos = 0;

        for (const auto& midiEvent : midi)
        {
            const auto& msg    = midiEvent.getMessage();
            const int   msgPos = std::min(midiEvent.samplePosition, numSamples - 1);

            renderVoicesRange(writeL, writeR, midiSamplePos, msgPos,
                              agentCount, solidarity, intent, scatter,
                              distType, damping, separation, cohesion,
                              symmetry, foldAmt, dcBlock, pitchTrack, delayBase,
                              lambdaPerSample, eventAmp, grainDecayCoeff, densityJitter,
                              baseCutoff, fltReso, fltType, fltEnvAmt, fltKeyTrack,
                              fltAtk, fltDec, fltSus, fltRel,
                              ampAtk, ampDec, ampSus, ampRel, velTimbre,
                              lfo1Rate, lfo1Depth, lfo1Shape, lfo1Tgt,
                              lfo2Rate, lfo2Depth, lfo2Shape, lfo2Tgt,
                              glideCoeff, glideMode, modAmpLevel);
            midiSamplePos = msgPos;

            if (msg.isNoteOn())
            {
                handleNoteOn(msg.getNoteNumber(), msg.getVelocity(),
                             ampAtk, ampDec, ampSus, ampRel,
                             fltAtk, fltDec, fltSus, fltRel,
                             agentCount, scatter, distType,
                             voiceMode, glideCoeff, glideMode, velTimbre);
            }
            else if (msg.isNoteOff())
            {
                handleNoteOff(msg.getNoteNumber());
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheelValue = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isChannelPressure())
            {
                aftertouchValue = msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isAftertouch())
            {
                aftertouchValue = msg.getAfterTouchValue() / 127.0f;
            }
        }

        // Render remaining samples after last MIDI event
        renderVoicesRange(writeL, writeR, midiSamplePos, numSamples,
                          agentCount, solidarity, intent, scatter,
                          distType, damping, separation, cohesion,
                          symmetry, foldAmt, dcBlock, pitchTrack, delayBase,
                          lambdaPerSample, eventAmp, grainDecayCoeff, densityJitter,
                          baseCutoff, fltReso, fltType, fltEnvAmt, fltKeyTrack,
                          fltAtk, fltDec, fltSus, fltRel,
                          ampAtk, ampDec, ampSus, ampRel, velTimbre,
                          lfo1Rate, lfo1Depth, lfo1Shape, lfo1Tgt,
                          lfo2Rate, lfo2Depth, lfo2Shape, lfo2Tgt,
                          glideCoeff, glideMode, modAmpLevel);

        // ---- Update cached coupling outputs ----
        lastSampleL = writeL[numSamples - 1];
        lastSampleR = writeR[numSamples - 1];

        // Compute flock coherence across active voices
        float cohSum = 0.0f;
        int cohCount = 0;
        for (auto& v : voices)
        {
            if (v.active)
            {
                cohSum += v.computeCoherence();
                ++cohCount;
            }
        }
        lastCoherence = (cohCount > 0) ? cohSum / static_cast<float>(cohCount) : 0.0f;

        // ---- Update silence gate and active voice count ----
        int activeCount = 0;
        for (auto& v : voices)
            if (v.active) ++activeCount;
        activeVoiceCount_.store(activeCount, std::memory_order_relaxed);

        analyzeForSilenceGate(buffer, numSamples);

        // ---- Decay coupling accumulators (Lesson 10) ----
        // fastExp gives sample-rate-correct per-block decay (~50 ms time constant).
        // Was 0.999f^1 per block which is sample-rate dependent: at 48 kHz with 128
        // samples/block the effective decay is faster than at 44.1 kHz.
        const float couplingDecay = fastExp(-static_cast<float>(numSamples) / (0.05f * sampleRateFloat));
        couplingFMVal     *= couplingDecay;
        couplingAmpFilter *= couplingDecay;
        couplingEnvSolid  *= couplingDecay;
        couplingRhythm    *= couplingDecay;
        // Block-size-invariant: the old `*= 0.999f` per block gave radically
        // different time constants across DAW buffer sizes (~11.6s @ 512/44.1k
        // vs ~0.33s @ 32/96k). Use fastExp on a ~1s time constant so the decay
        // feel is identical at every buffer size.
        const float couplingDecayCoeff = fastExp(-static_cast<float>(numSamples) / sampleRateFloat);
        couplingFMVal     *= couplingDecayCoeff;
        couplingAmpFilter *= couplingDecayCoeff;
        couplingEnvSolid  *= couplingDecayCoeff;
        couplingRhythm    *= couplingDecayCoeff;
        couplingFMVal     = flushDenormal(couplingFMVal);
        couplingAmpFilter = flushDenormal(couplingAmpFilter);
        couplingEnvSolid  = flushDenormal(couplingEnvSolid);
        couplingRhythm    = flushDenormal(couplingRhythm);
    }

private:
    //==========================================================================
    // renderVoicesRange — render samples [startSample, endSample)
    //==========================================================================

    void renderVoicesRange(float* writeL, float* writeR,
                           int startSample, int endSample,
                           int agentCount, float solidarity, float intent, float scatter,
                           int distType, float damping, float separation, float cohesion,
                           float symmetry, float foldAmt, int dcBlock,
                           float pitchTrack, float delayBase,
                           float lambdaPerSample, float eventAmp, float grainDecayCoeff,
                           float densityJitter,
                           float baseCutoff, float fltReso, int fltType,
                           float fltEnvAmt, float fltKeyTrack,
                           float fltAtk, float fltDec, float fltSus, float fltRel,
                           float ampAtk, float ampDec, float ampSus, float ampRel,
                           float velTimbre,
                           float lfo1Rate, float lfo1Depth, int lfo1Shape, int lfo1Tgt,
                           float lfo2Rate, float lfo2Depth, int lfo2Shape, int lfo2Tgt,
                           float glideCoeff, int glideMode, float modAmpLevel) noexcept
    {
        if (startSample >= endSample) return;

        for (auto& v : voices)
        {
            if (!v.active) continue;

            // ---- agentCount: update live if changed (D004 — every param affects audio) ----
            // If the agent count knob has changed, reinitialise to match. We only change the
            // count when it differs to avoid resetting the flock on every block.
            if (v.agentCount != agentCount && agentCount >= 1 && agentCount <= kOortMaxAgents)
            {
                // Preserve existing positions for the agents that remain; new ones interpolate
                const int oldN = v.agentCount;
                const int newN = agentCount;
                if (newN > oldN)
                {
                    for (int i = oldN; i < newN; ++i)
                    {
                        // Insert new agents by interpolating between their neighbours
                        const float t = static_cast<float>(i) / static_cast<float>(newN - 1);
                        const float srcPos = (oldN > 1)
                            ? lerp(v.agents[0].position, v.agents[oldN - 1].position, t)
                            : 0.0f;
                        v.agents[i].position = std::clamp(srcPos, -1.0f, 1.0f);
                        v.agents[i].velocity = v.agents[oldN > 0 ? oldN - 1 : 0].velocity * 0.5f;
                    }
                }
                v.agentCount = newN;
                v.readIdx    = std::min(v.readIdx, std::max(0, newN - 1));
            }

            // ---- Configure LFOs (per block) ----
            v.lfo1.setRate(lfo1Rate, sampleRateFloat);
            v.lfo1.setShape(lfo1Shape);
            v.lfo2.setRate(lfo2Rate, sampleRateFloat);
            v.lfo2.setShape(lfo2Shape);

            // ---- Configure envelopes ----
            v.ampEnv.setParams(ampAtk, ampDec, ampSus, ampRel, sampleRateFloat);
            v.filterEnv.setParams(fltAtk, fltDec, fltSus, fltRel, sampleRateFloat);

            // Filter mode is block-constant from paramFltType — compute once per voice
            // instead of switching on every 16-sample coefficient refresh below.
            const CytomicSVF::Mode blockFltMode =
                (fltType == 1) ? CytomicSVF::Mode::HighPass :
                (fltType == 2) ? CytomicSVF::Mode::BandPass :
                (fltType == 3) ? CytomicSVF::Mode::Notch    :
                                 CytomicSVF::Mode::LowPass;

            for (int s = startSample; s < endSample; ++s)
            {
                // ---- LFO tick (block-rate LFOs on engine, Lesson 2) ----
                const float lfo1Out = v.lfo1.process();
                const float lfo2Out = v.lfo2.process();
                v.lastLfo1Val = lfo1Out;
                v.lastLfo2Val = lfo2Out;

                // ---- Apply LFO modulation to targets ----
                float effSolidarity  = solidarity;
                float effIntent      = intent;
                float effCutoff      = baseCutoff;
                float effEventDensity= lambdaPerSample;

                // LFO1 target
                const float lfo1Mod = lfo1Out * lfo1Depth;
                switch (lfo1Tgt)
                {
                case 0: effSolidarity   = std::clamp(solidarity   + lfo1Mod * 0.5f, 0.0f, 2.0f); break;
                case 1: effIntent       = std::clamp(intent       + lfo1Mod * 0.3f, 0.0f, 1.0f); break;
                case 2: effCutoff       = std::clamp(baseCutoff   + lfo1Mod * 8000.0f, 20.0f, 20000.0f); break;
                case 3: effEventDensity = std::clamp(lambdaPerSample + lfo1Mod * 0.001f, 0.0f, 0.01f); break;
                default: break;
                }

                // LFO2 target
                const float lfo2Mod = lfo2Out * lfo2Depth;
                switch (lfo2Tgt)
                {
                case 0: effSolidarity   = std::clamp(effSolidarity  + lfo2Mod * 0.5f, 0.0f, 2.0f); break;
                case 1: effIntent       = std::clamp(effIntent       + lfo2Mod * 0.3f, 0.0f, 1.0f); break;
                case 2: effCutoff       = std::clamp(effCutoff       + lfo2Mod * 8000.0f, 20.0f, 20000.0f); break;
                case 3: effEventDensity = std::clamp(effEventDensity + lfo2Mod * 0.001f, 0.0f, 0.01f); break;
                default: break;
                }

                // ---- Glide: smooth base frequency (Lesson 8) ----
                // glideMode=0 (Legato): glide only when this voice is actively gliding (set
                //   in handleNoteOn when the voice was already playing).
                // glideMode=1 (Always): always apply portamento from the previous note.
                const float targetFreq = midiToFreq(v.note);
                // Effective glide coeff: Always mode uses full coeff; Legato mode uses coeff
                // only when the voice is marked as gliding (flag set in handleNoteOn).
                const float effGlideCoeff = (glideMode == 1) ? glideCoeff
                                          : (v.gliding ? glideCoeff : 0.0f);
                if (effGlideCoeff > 0.0f)
                    v.glideBaseFreq = v.glideBaseFreq * effGlideCoeff + targetFreq * (1.0f - effGlideCoeff);
                else
                    v.glideBaseFreq = targetFreq;
                v.glideBaseFreq = flushDenormal(v.glideBaseFreq);

                // ---- Compute cycle period from pitch ----
                const float baseFreq = (pitchTrack > 0.0f)
                    ? lerp(delayBase, v.glideBaseFreq, pitchTrack)
                    : delayBase;
                v.cyclePeriod = std::max(2.0f, sampleRateFloat / std::max(0.1f, baseFreq));

                // ---- Read waveform from current agent positions ----
                // Linear interpolation between agents in index order
                const int   n             = v.agentCount;
                // One division; derive invStepLen (= n / cyclePeriod) from same reciprocal
                const float invCyclePeriod = 1.0f / v.cyclePeriod;
                const float invStepLen     = static_cast<float>(n) * invCyclePeriod;

                // How many samples into the current agent segment?
                float raw = 0.0f;
                if (n > 1)
                {
                    // Linear interp between agent[readIdx] and agent[(readIdx+1) % n]
                    const int   nextIdx = (v.readIdx + 1) % n;
                    // Symmetry bias: shift all agent positions toward positive (symmetry=0.5 = neutral)
                    const float symBias = (symmetry - 0.5f) * 0.5f;
                    const float pA = std::clamp(v.agents[v.readIdx].position + symBias, -1.0f, 1.0f);
                    const float pB = std::clamp(v.agents[nextIdx].position   + symBias, -1.0f, 1.0f);
                    raw = lerp(pA, pB, v.agentPhase);
                }
                else if (n == 1)
                {
                    raw = v.agents[0].position;
                }

                // Advance position within current agent segment
                v.agentPhase += invStepLen; // was 1.0f / stepLen — now one division shared with invCyclePeriod
                while (v.agentPhase >= 1.0f) // while-loop: handles overshoot at high pitch / fast glide
                {
                    v.agentPhase -= 1.0f;
                    v.readIdx = (v.readIdx + 1) % std::max(1, n);
                }

                // Advance full cycle counter
                v.cyclePhase += invCyclePeriod; // was 1.0f / v.cyclePeriod — reuse reciprocal
                if (v.cyclePhase >= 1.0f)
                {
                    v.cyclePhase -= 1.0f;
                    // ---- Per-cycle Boids update ----
                    // velTimbre (D001): velocity scales scatter ongoing — higher velocity
                    // = more perturbation per cycle = brighter, more agitated timbre
                    const float velScatterMod = scatter * (1.0f + v.velocity * velTimbre);
                    v.updateFlock(effSolidarity, effIntent, velScatterMod,
                                  separation, cohesion, damping,
                                  distType, v.audioToFMAccum);
                }

                // ---- Poisson event layer ----
                float poissonOut = 0.0f;
                if (effEventDensity > 0.0f)
                {
                    if (!v.grain.active)
                    {
                        v.poissonTimer -= 1.0f;
                        if (v.poissonTimer <= 0.0f)
                        {
                            v.grain.active     = true;
                            v.grain.amplitude  = eventAmp;
                            v.grain.level      = 1.0f;
                            v.grain.decayCoeff = grainDecayCoeff;

                            // Next event time: Poisson (exponential inter-arrival)
                            const float u = std::max(0.001f, v.nextRandomUnipolar());
                            const float jitter = 1.0f + densityJitter * (v.nextRandom() * 0.5f);
                            const float lambda = effEventDensity;
                            // -ln(u) = -log2(u) * ln(2); fastLog2 ~0.002% error, fine for stochastic timing
                            v.poissonTimer = (-fastLog2(u) * 0.6931472f / std::max(lambda, 1e-9f)) * jitter;
                        }
                    }

                    if (v.grain.active)
                    {
                        poissonOut = v.grain.level * v.grain.amplitude;
                        v.grain.level -= v.grain.level * v.grain.decayCoeff;
                        v.grain.level  = flushDenormal(v.grain.level);
                        if (v.grain.level < 1e-5f)
                        {
                            v.grain.active = false;
                            v.grain.level  = 0.0f;
                        }
                    }
                }

                // Mix flock waveform + Poisson events
                // softClip before DC block / wavefold: Poisson grain + fold can exceed unity
                float sig = softClip(raw + poissonOut);

                // ---- DC Block ----
                if (dcBlock)
                {
                    // First-order highpass at ~5 Hz — coefficient derived from SR so it
                    // tracks correctly at 48 kHz (0.99935) and 96 kHz (0.99967).
                    // Was constexpr 0.9997f which gave ~6.7 Hz at 44.1 kHz but ~13.4 Hz
                    // at 96 kHz due to sample count difference.
                    const float dcCoeff = 1.0f - fastExp(-kOortTwoPi * 5.0f / sampleRateFloat);
                    const float dcOut = sig - v.dcBlockX + dcCoeff * v.dcBlockY;
                    v.dcBlockX = sig;
                    v.dcBlockY = flushDenormal(dcOut);
                    sig = dcOut;
                }

                // ---- Wavefold ----
                if (foldAmt > 0.001f)
                {
                    const float foldGain = 1.0f + foldAmt * 4.0f;
                    sig = sig * foldGain;
                    // Fold by reflecting through ±1 boundaries
                    while (sig > 1.0f)  sig = 2.0f - sig;
                    while (sig < -1.0f) sig = -2.0f - sig;
                    sig = std::clamp(sig, -1.0f, 1.0f);
                }

                // ---- Envelope ticks ----
                const float ampLevel = v.ampEnv.process();
                const float fltLevel = v.filterEnv.process();

                if (!v.ampEnv.isActive())
                {
                    v.active    = false;
                    v.releasing = false;
                    break;
                }

                // ---- Filter ----
                // Update filter coefficients every 16 samples (Lesson 4)
                ++v.filterCoeffCounter;
                if ((v.filterCoeffCounter & 15) == 0)
                {
                    // Exponential keytracking: cutoff · 2^((note-60)·keyTrack/12).
                    // Was linear in normalised-note space (keyTrack01 × 4800 Hz) which
                    // gave a mis-scaled response at the note extremes.
                    const float keyTrackMul = fastPow2(
                        static_cast<float>(v.note - 60) * fltKeyTrack * (1.0f / 12.0f));
                    // fltEnvAmt is bipolar — |x| > 1e-6 (was exact !=).
                    const float envOffset = (std::fabs(fltEnvAmt) > 1e-6f)
                        ? fltLevel * fltEnvAmt * 8000.0f
                        : 0.0f;
                    const float fCutoff = std::clamp(effCutoff * keyTrackMul + envOffset, 20.0f, 20000.0f);

                    if (fltType != v.prevFltType)
                    {
                        v.filterL.reset();
                        v.filterR.reset();
                        v.prevFltType = fltType;
                    }
                    v.filterL.setMode(blockFltMode);
                    v.filterR.setMode(blockFltMode);
                    v.filterL.setCoefficients_fast(fCutoff, fltReso, sampleRateFloat);
                    v.filterR.setCoefficients_fast(fCutoff, fltReso, sampleRateFloat);
                }

                float sigL = v.filterL.processSample(sig);
                float sigR = v.filterR.processSample(sig);

                // ---- VCA ----
                // Clamp (1+modAmpLevel) ≥ 0 to prevent polarity inversion when mod drives below -1
                const float gain = ampLevel * v.velocity * std::max(0.0f, 1.0f + modAmpLevel);
                sigL *= gain;
                sigR *= gain;

                writeL[s] += sigL;
                writeR[s] += sigR;
            }
        }
    }

    //==========================================================================
    // handleNoteOn
    //==========================================================================

    void handleNoteOn(int note, int midiVel,
                      float ampAtk, float ampDec, float ampSus, float ampRel,
                      float fltAtk, float fltDec, float fltSus, float fltRel,
                      int agentCount, float scatter, int distType,
                      int voiceMode, float glideCoeff, int glideMode,
                      float velTimbre) noexcept
    {
        const float vel = midiVel / 127.0f;

        // Determine max polyphony
        const int maxPoly = (voiceMode == 0 || voiceMode == 1) ? 1
                          : (voiceMode == 2)                   ? 4
                                                               : 8;

        // Find a free voice or steal the oldest
        int idx = -1;

        if (voiceMode == 0 || voiceMode == 1)
        {
            // Mono/Legato: always use voice 0
            idx = 0;
        }
        else
        {
            // Poly: find free voice first
            for (int i = 0; i < maxPoly; ++i)
            {
                if (!voices[i].active)
                {
                    idx = i;
                    break;
                }
            }
            // If none free, steal via round-robin across the poly range.
            // Round-robin spreads interruptions across notes rather than always
            // cutting the same voice (voice-0 cutoff), reducing click clustering.
            if (idx == -1)
            {
                stealIdx = (stealIdx + 1) % maxPoly;
                idx = stealIdx;
            }
        }

        auto& v = voices[idx];
        const float prevFreq = v.glideBaseFreq;

        // Apply glide: flag voice as gliding if legato mode+was active, or glideMode=Always
        const bool doGlide = (glideCoeff > 0.0f) &&
                             ((glideMode == 1) || (voiceMode == 1 && v.active));
        v.gliding = doGlide;
        if (!doGlide)
            v.glideBaseFreq = midiToFreq(note);
        else
            v.glideBaseFreq = prevFreq; // let the per-sample smoother handle it

        v.note     = note;
        v.velocity = vel;
        v.keyTrack = (static_cast<float>(note) - 60.0f) / 60.0f;
        v.active   = true;
        v.releasing= false;

        // Legato retrigger vs fresh start
        if (voiceMode == 1 && v.ampEnv.isActive())
        {
            v.ampEnv.retriggerFrom(v.ampEnv.getLevel(), ampAtk, ampDec, ampSus, ampRel);
            v.filterEnv.retriggerFrom(v.filterEnv.getLevel(), fltAtk, fltDec, fltSus, fltRel);
        }
        else
        {
            v.ampEnv.setParams(ampAtk, ampDec, ampSus, ampRel, sampleRateFloat);
            v.filterEnv.setParams(fltAtk, fltDec, fltSus, fltRel, sampleRateFloat);
            v.ampEnv.noteOn();
            v.filterEnv.noteOn();
        }

        // Reset filter coefficient counter so first sample gets correct coefficients
        // (stolen voice may have stale coefficients from previous note)
        v.filterCoeffCounter = 15;

        // Velocity -> initial agent scatter + excitation energy (D001)
        // Use passed velTimbre param (block-rate snapshot) rather than a raw ptr read
        const float velScatter = scatter * (1.0f + vel * velTimbre);
        v.initAgents(agentCount, velScatter, vel, distType);
        v.cyclePeriod = sampleRateFloat / midiToFreq(note);

        wakeSilenceGate();
    }

    //==========================================================================
    // handleNoteOff
    //==========================================================================

    void handleNoteOff(int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && !v.releasing && v.note == note)
            {
                v.releasing = true;
                v.ampEnv.noteOff();
                v.filterEnv.noteOff();
                break;
            }
        }
    }

    //==========================================================================
    // State
    //==========================================================================

    // Do not default-init — must be set by prepare() on the live sample rate.
    // Sentinel 0.0 makes misuse before prepare() a crash instead of silent wrong-rate DSP.
    float sampleRateFloat = 0.0f;
    int   maxBlock        = 512;

    std::array<OortVoice, kOortMaxVoices> voices;
    int   stealIdx = 0; // round-robin steal pointer

    // Coupling accumulators
    float couplingFMVal      = 0.0f;
    float couplingAmpFilter  = 0.0f;
    float couplingEnvSolid   = 0.0f;
    float couplingRhythm     = 0.0f;

    // Expression inputs
    float modWheelValue   = 0.0f;
    float aftertouchValue = 0.0f;

    // Cached coupling outputs (O(1))
    float lastSampleL   = 0.0f;
    float lastSampleR   = 0.0f;
    float lastCoherence = 0.0f;

    // Mod matrix
    ModMatrix<4>          modMatrix;
    ModMatrix<4>::Sources blockModSrc;

    //==========================================================================
    // Parameter pointers (cached from APVTS)
    //==========================================================================

    // Group A: Flock Core
    std::atomic<float>* pAgentCount  = nullptr;
    std::atomic<float>* pSolidarity  = nullptr;
    std::atomic<float>* pIntent      = nullptr;
    std::atomic<float>* pScatter     = nullptr;
    std::atomic<float>* pDistType    = nullptr;
    std::atomic<float>* pDamping     = nullptr;
    std::atomic<float>* pSeparation  = nullptr;
    std::atomic<float>* pCohesion    = nullptr;

    // Group B: Waveform Shaping
    std::atomic<float>* pSymmetry    = nullptr;
    std::atomic<float>* pFoldAmt     = nullptr;
    std::atomic<float>* pDCBlock     = nullptr;
    std::atomic<float>* pPitchTrack  = nullptr;
    std::atomic<float>* pDelayBase   = nullptr;

    // Group C: Poisson Events
    std::atomic<float>* pEventDensity  = nullptr;
    std::atomic<float>* pEventAmp      = nullptr;
    std::atomic<float>* pEventDecay    = nullptr;
    std::atomic<float>* pDensityJitter = nullptr;

    // Group D: Filter
    std::atomic<float>* pFltCutoff    = nullptr;
    std::atomic<float>* pFltReso      = nullptr;
    std::atomic<float>* pFltType      = nullptr;
    std::atomic<float>* pFltEnvAmt    = nullptr;
    std::atomic<float>* pFltKeyTrack  = nullptr;
    std::atomic<float>* pFltAtk       = nullptr;
    std::atomic<float>* pFltDec       = nullptr;
    std::atomic<float>* pFltSus       = nullptr;
    std::atomic<float>* pFltRel       = nullptr;

    // Group E: Amp Envelope
    std::atomic<float>* pAmpAtk       = nullptr;
    std::atomic<float>* pAmpDec       = nullptr;
    std::atomic<float>* pAmpSus       = nullptr;
    std::atomic<float>* pAmpRel       = nullptr;
    std::atomic<float>* pVelTimbre    = nullptr;

    // Group F: LFOs
    std::atomic<float>* pLfo1Rate     = nullptr;
    std::atomic<float>* pLfo1Depth    = nullptr;
    std::atomic<float>* pLfo1Shape    = nullptr;
    std::atomic<float>* pLfo1Target   = nullptr;
    std::atomic<float>* pLfo2Rate     = nullptr;
    std::atomic<float>* pLfo2Depth    = nullptr;
    std::atomic<float>* pLfo2Shape    = nullptr;
    std::atomic<float>* pLfo2Target   = nullptr;

    // Group H: Macros + Voice
    std::atomic<float>* pMacro1    = nullptr;
    std::atomic<float>* pMacro2    = nullptr;
    std::atomic<float>* pMacro3    = nullptr;
    std::atomic<float>* pMacro4    = nullptr;
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide     = nullptr;
    std::atomic<float>* pGlideMode = nullptr;
};

} // namespace xoceanus
