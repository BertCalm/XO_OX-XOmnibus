#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/ShoreSystem/ShoreSystem.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xomnibus {

//==============================================================================
// OspreyEngine — Turbulence-Modulated Resonator Synthesis.
//
// Transforms the open ocean into an instrument. Each voice drives a bank of
// 16 modal resonators (3 instrument groups x 4 formants + 4 sympathetic)
// excited by a fluid energy model that ranges from calm sinusoidal swell to
// layered turbulent noise. Three creature voice formant generators add organic
// life — bird calls, whale songs, ambient coastal textures — modulated by
// sea state and shore position.
//
// The ShoreSystem provides 5 coastal regions (Atlantic, Nordic, Mediterranean,
// Pacific, Southern) with continuous morphing. Each shore defines resonator
// spectral profiles, creature voice targets, and fluid character.
//
// Features:
//   - 16 modal resonators per voice (tuned 2-pole resonant filters)
//   - 3 creature voice formant generators (sweeping 3-band formant filters)
//   - Fluid energy model (Perlin-inspired layered noise + swell)
//   - Shore-morphing resonator profiles from ShoreSystem
//   - Sympathetic resonator coupling (cross-resonator energy transfer)
//   - Coherence control (partial phase correlation)
//   - Foam (HF saturation), Brine (subtle bitcrushing), Hull (body resonance)
//   - Harbor verb (lightweight 4-allpass delay network)
//   - Fog (HF rolloff), Tilt filter (LP/HP spectral balance)
//   - Amp ADSR + 4 macros (Sea State, Movement, Coupling, Space)
//   - DC blocker + soft limiter on output
//   - 8-voice polyphony with LRU stealing + 5ms crossfade
//
// Coupling:
//   - Output: post-limiter stereo audio via getSampleForCoupling
//   - Input: AudioToFM (resonator excitation), AmpToFilter (sea state mod),
//            EnvToMorph (swell period mod), LFOToPitch (resonator tuning),
//            AudioToWavetable (replaces excitation source)
//
//==============================================================================

//==============================================================================
// ADSR envelope generator — lightweight, inline, no allocation.
//==============================================================================
struct OspreyADSR
{
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    Stage stage = Stage::Idle;
    float level = 0.0f;
    float attackRate  = 0.0f;
    float decayRate   = 0.0f;
    float sustainLevel = 1.0f;
    float releaseRate = 0.0f;

    void setParams (float attackSec, float decaySec, float sustain, float releaseSec,
                    float sampleRate) noexcept
    {
        float sr = std::max (1.0f, sampleRate);
        attackRate  = (attackSec  > 0.001f) ? (1.0f / (attackSec  * sr)) : 1.0f;
        decayRate   = (decaySec   > 0.001f) ? (1.0f / (decaySec   * sr)) : 1.0f;
        sustainLevel = sustain;
        releaseRate = (releaseSec > 0.001f) ? (1.0f / (releaseSec * sr)) : 1.0f;
    }

    void noteOn() noexcept
    {
        stage = Stage::Attack;
    }

    void noteOff() noexcept
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Idle:
                return 0.0f;

            case Stage::Attack:
                level += attackRate;
                if (level >= 1.0f)
                {
                    level = 1.0f;
                    stage = Stage::Decay;
                }
                return level;

            case Stage::Decay:
                level -= decayRate * (level - sustainLevel + 0.0001f);
                if (level <= sustainLevel + 0.0001f)
                {
                    level = sustainLevel;
                    stage = Stage::Sustain;
                }
                return level;

            case Stage::Sustain:
                return level;

            case Stage::Release:
                level -= releaseRate * (level + 0.0001f);
                if (level <= 0.0001f)
                {
                    level = 0.0f;
                    stage = Stage::Idle;
                }
                return level;
        }
        return 0.0f;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }

    void reset() noexcept
    {
        stage = Stage::Idle;
        level = 0.0f;
    }
};

//==============================================================================
// LFO with multiple shapes.
//==============================================================================
struct OspreyLFO
{
    enum class Shape { Sine, Triangle, Saw, Square, SandH };

    float phase = 0.0f;
    float phaseInc = 0.0f;
    Shape shape = Shape::Sine;
    float holdValue = 0.0f;
    uint32_t sampleCounter = 0;

    void setRate (float hz, float sampleRate) noexcept
    {
        phaseInc = hz / std::max (1.0f, sampleRate);
    }

    void setShape (int idx) noexcept
    {
        shape = static_cast<Shape> (std::min (4, std::max (0, idx)));
    }

    float process() noexcept
    {
        float out = 0.0f;

        switch (shape)
        {
            case Shape::Sine:
                out = fastSin (phase * 6.28318530718f);
                break;
            case Shape::Triangle:
                out = 4.0f * std::fabs (phase - 0.5f) - 1.0f;
                break;
            case Shape::Saw:
                out = 2.0f * phase - 1.0f;
                break;
            case Shape::Square:
                out = (phase < 0.5f) ? 1.0f : -1.0f;
                break;
            case Shape::SandH:
            {
                float prevPhase = phase - phaseInc;
                if (prevPhase < 0.0f || phase < prevPhase)
                {
                    sampleCounter = sampleCounter * 1664525u + 1013904223u;
                    holdValue = static_cast<float> (sampleCounter & 0xFFFF) / 32768.0f - 1.0f;
                }
                out = holdValue;
                break;
            }
        }

        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;

        return out;
    }

    void reset() noexcept
    {
        phase = 0.0f;
        holdValue = 0.0f;
        sampleCounter = 12345u;
    }
};

//==============================================================================
// ModalResonator — a single tuned resonator modeled as a 2-pole resonant filter.
//==============================================================================
struct ModalResonator
{
    float freq = 440.0f;       // center frequency
    float bandwidth = 100.0f;  // bandwidth Hz
    float gain = 1.0f;         // amplitude
    float s1 = 0.0f, s2 = 0.0f; // state
    float a1 = 0.0f, a2 = 0.0f, b0 = 0.0f; // coefficients

    void setCoefficients (float centerFreq, float bw, float g, float sampleRate) noexcept
    {
        freq = centerFreq;
        bandwidth = bw;
        gain = g;
        float w0 = 2.0f * 3.14159265f * centerFreq / sampleRate;
        float r = fastExp (-3.14159265f * bw / sampleRate); // decay rate
        a1 = -2.0f * r * fastCos (w0);
        a2 = r * r;
        b0 = (1.0f - r * r) * gain;
    }

    float process (float excitation) noexcept
    {
        float out = b0 * excitation - a1 * s1 - a2 * s2;
        s2 = s1;
        s1 = flushDenormal (out);
        return out;
    }

    void reset() noexcept { s1 = 0.0f; s2 = 0.0f; }
};

//==============================================================================
// CreatureFormant — generates creature voice sounds using sweeping formant
// filters. Three formant bands sweep from start to end frequencies.
//==============================================================================
struct CreatureFormant
{
    float phase = 0.0f;        // sweep phase [0,1)
    float sweepMs = 1000.0f;
    float gapMs = 5000.0f;
    bool active = false;
    float startFreqs[3] = {}, endFreqs[3] = {}, bandwidths[3] = {};
    float amplitude = 0.5f;
    CytomicSVF filters[3];    // 3 formant bands
    uint32_t rng = 54321u;

    // Timing state
    float sweepPhaseInc = 0.0f;
    float gapCounter = 0.0f;
    float gapPhaseInc = 0.0f;
    bool inGap = true;

    float nextRandom() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return static_cast<float> (rng & 0xFFFF) / 65536.0f;
    }

    void trigger (float sampleRate) noexcept
    {
        active = true;
        inGap = false;
        phase = 0.0f;
        sweepPhaseInc = 1000.0f / (sweepMs * sampleRate); // per-sample phase increment
        if (sweepPhaseInc <= 0.0f) sweepPhaseInc = 0.001f;

        for (int i = 0; i < 3; ++i)
        {
            filters[i].setMode (CytomicSVF::Mode::BandPass);
            filters[i].setCoefficients (startFreqs[i], 0.7f, sampleRate);
        }
    }

    void configure (const CreatureVoice& cv, float sampleRate) noexcept
    {
        sweepMs = cv.sweepMs;
        gapMs = cv.gapMs;
        amplitude = cv.amplitude;
        for (int i = 0; i < 3; ++i)
        {
            startFreqs[i] = cv.startFreqs[i];
            endFreqs[i] = cv.endFreqs[i];
            bandwidths[i] = cv.bandwidths[i];
        }
        gapPhaseInc = 1000.0f / (gapMs * sampleRate);
        if (gapPhaseInc <= 0.0f) gapPhaseInc = 0.0001f;
    }

    float process (float sampleRate, float excitation) noexcept
    {
        if (!active) return 0.0f;

        if (inGap)
        {
            gapCounter += gapPhaseInc;
            if (gapCounter >= 1.0f)
            {
                gapCounter = 0.0f;
                // Random chance to re-trigger
                if (nextRandom() < 0.7f)
                {
                    trigger (sampleRate);
                }
            }
            return 0.0f;
        }

        // Sweeping: interpolate formant frequencies from start to end
        float t = phase;
        float smoothT = t * t * (3.0f - 2.0f * t); // smoothstep
        float out = 0.0f;

        for (int i = 0; i < 3; ++i)
        {
            float currentFreq = startFreqs[i] + smoothT * (endFreqs[i] - startFreqs[i]);
            currentFreq = clamp (currentFreq, 40.0f, sampleRate * 0.45f);
            float q = currentFreq / std::max (bandwidths[i], 10.0f);
            q = clamp (q, 0.2f, 0.95f);
            filters[i].setCoefficients (currentFreq, q, sampleRate);
            out += filters[i].processSample (excitation);
        }

        phase += sweepPhaseInc;
        if (phase >= 1.0f)
        {
            // Sweep complete — enter gap
            inGap = true;
            gapCounter = 0.0f;
            phase = 0.0f;
        }

        return out * amplitude * (1.0f / 3.0f);
    }

    void reset() noexcept
    {
        phase = 0.0f;
        active = false;
        inGap = true;
        gapCounter = 0.0f;
        for (int i = 0; i < 3; ++i)
            filters[i].reset();
    }
};

//==============================================================================
// FluidEnergyModel — Perlin-inspired noise generator that models turbulent
// fluid energy. At low seaState produces smooth sinusoidal swell; at high
// seaState produces layered multi-octave noise with surface chop.
//==============================================================================
struct FluidEnergyModel
{
    float phase = 0.0f;           // swell phase
    float chopPhase = 0.0f;       // surface chop phase
    float noiseState[4] = {};     // layered noise state (4 octaves)
    uint32_t rng = 98765u;

    float nextRandom() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f;
    }

    float nextRandomUni() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return static_cast<float> (rng & 0xFFFF) / 65536.0f;
    }

    float process (float seaState, float swellPeriod, const FluidCharacter& fluid,
                   float sampleRate) noexcept
    {
        // Effective swell period blended with shore character
        float effectivePeriod = swellPeriod * (0.5f + 0.5f * fluid.swellPeriodBase / 12.0f);
        effectivePeriod = clamp (effectivePeriod, 0.5f, 60.0f);

        // Swell phase advance
        float swellInc = 1.0f / (effectivePeriod * sampleRate);
        phase += swellInc;
        if (phase >= 1.0f) phase -= 1.0f;

        // Base swell: smooth sinusoidal
        float swell = fastSin (phase * 6.28318530718f) * fluid.swellDepth;

        // Surface chop (higher frequency modulation)
        float chopFreq = fluid.chopFreqBase * (1.0f + seaState * 3.0f);
        float chopInc = chopFreq / sampleRate;
        chopPhase += chopInc;
        if (chopPhase >= 1.0f) chopPhase -= 1.0f;
        float chop = fastSin (chopPhase * 6.28318530718f) * fluid.chopAmount * seaState;

        // Turbulence threshold — noise kicks in at higher sea states
        float turbulence = 0.0f;
        float turbulenceAmount = clamp ((seaState - fluid.turbulenceOnset) /
                                         (1.0f - fluid.turbulenceOnset + 0.001f), 0.0f, 1.0f);

        if (turbulenceAmount > 0.001f)
        {
            // Multi-octave layered noise
            float freqs[4] = { 0.3f, 0.7f, 1.5f, 3.2f };
            float amps[4]  = { 1.0f, 0.5f, 0.25f, 0.125f };

            for (int oct = 0; oct < 4; ++oct)
            {
                // Smooth random walk per octave
                float target = nextRandom();
                float rate = freqs[oct] / sampleRate;
                noiseState[oct] += (target - noiseState[oct]) * rate;
                noiseState[oct] = flushDenormal (noiseState[oct]);
                turbulence += noiseState[oct] * amps[oct];
            }

            turbulence *= turbulenceAmount * 0.5f;
        }

        // Depth bias: blend surface vs subsurface energy
        float depthFactor = 1.0f - fluid.depthBias * 0.5f;

        // Combine all energy sources
        float energy = (swell + chop + turbulence) * depthFactor;

        // Scale by sea state (calm = gentle, stormy = intense)
        energy *= (0.2f + seaState * 0.8f);

        return energy;
    }

    void reset() noexcept
    {
        phase = 0.0f;
        chopPhase = 0.0f;
        for (int i = 0; i < 4; ++i)
            noiseState[i] = 0.0f;
    }
};

//==============================================================================
// AllpassDelay — simple allpass delay for harbor verb.
// y[n] = -g*x[n] + x[n-d] + g*y[n-d]
// Fixed max buffer size (4096 samples = ~93ms at 44.1kHz).
//==============================================================================
struct AllpassDelay
{
    static constexpr int kMaxDelay = 4096;
    float buffer[kMaxDelay] = {};
    int writePos = 0;
    int delaySamples = 1000;
    float feedback = 0.5f;

    void setParams (int delay, float fb) noexcept
    {
        delaySamples = clamp (delay, 1, kMaxDelay - 1);
        feedback = fb;
    }

    float process (float input) noexcept
    {
        int readPos = writePos - delaySamples;
        if (readPos < 0) readPos += kMaxDelay;

        float delayed = buffer[readPos];
        float out = -feedback * input + delayed + feedback * delayed;
        // Correct allpass: out = delayed - g * input, buffer stores: input + g * delayed
        // More standard form:
        float bufVal = input + feedback * delayed;
        out = delayed - feedback * bufVal;
        buffer[writePos] = flushDenormal (bufVal);

        writePos++;
        if (writePos >= kMaxDelay) writePos = 0;

        return flushDenormal (out);
    }

    void reset() noexcept
    {
        for (int i = 0; i < kMaxDelay; ++i)
            buffer[i] = 0.0f;
        writePos = 0;
    }
};

//==============================================================================
// OspreyVoice — per-voice state containing resonator bank + creature voices.
//==============================================================================
struct OspreyVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;
    float targetFreq = 440.0f;
    float currentTargetFreq = 440.0f;
    float glideCoeff = 1.0f;

    // 16 modal resonators (3 instrument groups x 4 formants + 4 sympathetic)
    std::array<ModalResonator, 16> resonators;

    // 3 creature voice formant generators
    std::array<CreatureFormant, 3> creatures;

    // ADSR
    OspreyADSR ampEnv;

    // Voice stealing crossfade
    float fadeGain = 1.0f;
    bool fadingOut = false;

    // Control rate decimation
    int controlCounter = 0;

    // Per-voice PRNG
    uint32_t rng = 12345u;

    // DC blocker state
    float dcPrevInL = 0.0f, dcPrevOutL = 0.0f;
    float dcPrevInR = 0.0f, dcPrevOutR = 0.0f;

    // Per-voice coherence phase offset tracking
    float coherencePhases[16] = {};

    float nextRandom() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f; // [-1, 1]
    }

    float nextRandomUni() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return static_cast<float> (rng & 0xFFFF) / 65536.0f; // [0, 1)
    }

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        targetFreq = 440.0f;
        currentTargetFreq = 440.0f;
        glideCoeff = 1.0f;
        fadeGain = 1.0f;
        fadingOut = false;
        controlCounter = 0;
        rng = 12345u;
        dcPrevInL = 0.0f;
        dcPrevOutL = 0.0f;
        dcPrevInR = 0.0f;
        dcPrevOutR = 0.0f;
        ampEnv.reset();

        for (auto& r : resonators)
            r.reset();
        for (auto& c : creatures)
            c.reset();
        for (int i = 0; i < 16; ++i)
            coherencePhases[i] = 0.0f;
    }
};

//==============================================================================
// OspreyEngine — the main engine class.
//==============================================================================
class OspreyEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;
    static constexpr float kPI = 3.14159265358979323846f;
    static constexpr float kTwoPi = 6.28318530717958647692f;
    static constexpr int kNumResonators = 16;

    //==========================================================================
    // SynthEngine interface — Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        // Control rate decimation: ~2kHz
        controlRateDiv = std::max (1, static_cast<int> (srf / 2000.0f));
        controlDt = static_cast<float> (controlRateDiv) / srf;

        // Pre-compute crossfade rate (5ms)
        crossfadeRate = 1.0f / (0.005f * srf);

        // Allocate output cache for coupling reads
        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        // Initialize voices
        for (auto& v : voices)
            v.reset();

        // Initialize global fluid model
        fluidModel.reset();

        // Initialize harbor verb allpass delays with prime-number lengths
        // Scaled for ~44.1kHz; will be rescaled in prepare based on actual SR
        float srScale = srf / 44100.0f;
        int delayLens[4] = {
            static_cast<int> (1087.0f * srScale),
            static_cast<int> (1283.0f * srScale),
            static_cast<int> (1511.0f * srScale),
            static_cast<int> (1777.0f * srScale)
        };
        for (int i = 0; i < 4; ++i)
        {
            verbAllpass[i].reset();
            verbAllpass[i].setParams (delayLens[i], 0.5f);
        }

        // Initialize global filters
        tiltFilterL.reset();
        tiltFilterR.reset();
        fogFilterL.reset();
        fogFilterR.reset();
        hullFilterL.reset();
        hullFilterR.reset();
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        fluidModel.reset();
        envelopeOutput = 0.0f;
        couplingExcitationMod = 0.0f;
        couplingSeaStateMod = 0.0f;
        couplingSwellMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingAudioReplace = 0.0f;
        couplingAudioReplaceActive = false;

        for (int i = 0; i < 4; ++i)
            verbAllpass[i].reset();

        tiltFilterL.reset();
        tiltFilterR.reset();
        fogFilterL.reset();
        fogFilterR.reset();
        hullFilterL.reset();
        hullFilterR.reset();

        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //==========================================================================
    // SynthEngine interface — Audio
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // --- ParamSnapshot: read all parameters once per block ---
        const float pShore             = loadParam (paramShore, 0.0f);
        const float pSeaState          = loadParam (paramSeaState, 0.2f);
        const float pSwellPeriod       = loadParam (paramSwellPeriod, 8.0f);
        const float pWindDir           = loadParam (paramWindDir, 0.5f);
        const float pDepth             = loadParam (paramDepth, 0.3f);
        const float pResonatorBright   = loadParam (paramResonatorBright, 0.5f);
        const float pResonatorDecay    = loadParam (paramResonatorDecay, 1.0f);
        const float pSympathyAmount    = loadParam (paramSympathyAmount, 0.3f);
        const float pCreatureRate      = loadParam (paramCreatureRate, 0.2f);
        const float pCreatureDepth     = loadParam (paramCreatureDepth, 0.3f);
        const float pCoherence         = loadParam (paramCoherence, 0.7f);
        const float pFoam              = loadParam (paramFoam, 0.0f);
        const float pBrine             = loadParam (paramBrine, 0.0f);
        const float pHull              = loadParam (paramHull, 0.2f);
        const float pFilterTilt        = loadParam (paramFilterTilt, 0.5f);
        const float pHarborVerb        = loadParam (paramHarborVerb, 0.2f);
        const float pFog               = loadParam (paramFog, 0.1f);
        const float pAmpA              = loadParam (paramAmpAttack, 0.5f);
        const float pAmpD              = loadParam (paramAmpDecay, 1.0f);
        const float pAmpS              = loadParam (paramAmpSustain, 0.7f);
        const float pAmpR              = loadParam (paramAmpRelease, 2.0f);
        const float macroChar          = loadParam (paramMacroCharacter, 0.0f);
        const float macroMove          = loadParam (paramMacroMovement, 0.0f);
        const float macroCoup          = loadParam (paramMacroCoupling, 0.0f);
        const float macroSpace         = loadParam (paramMacroSpace, 0.0f);

        // Compute effective sea state (base + M1 macro)
        float effectiveSeaState = clamp (pSeaState + macroChar * 0.5f + couplingSeaStateMod, 0.0f, 1.0f);

        // Effective swell period (base + coupling modulation + M2 macro)
        float effectiveSwellPeriod = clamp (pSwellPeriod + couplingSwellMod * 5.0f
                                            - macroMove * 4.0f, 0.5f, 30.0f);

        // Effective verb amount (base + M4 macro)
        float effectiveVerb = clamp (pHarborVerb + macroSpace * 0.4f, 0.0f, 1.0f);

        // Effective creature depth (base + coupling macro)
        float effectiveCreatureDepth = clamp (pCreatureDepth + macroCoup * 0.3f, 0.0f, 1.0f);

        // Decompose shore for morphing
        ShoreMorphState shoreState = decomposeShore (pShore);

        // Morph resonator profiles, creature voices, fluid character
        ResonatorProfile morphedResonators[3];
        for (int i = 0; i < 3; ++i)
            morphedResonators[i] = morphResonator (shoreState, i);

        CreatureVoice morphedCreatures[3];
        for (int i = 0; i < 3; ++i)
            morphedCreatures[i] = morphCreature (shoreState, i);

        FluidCharacter morphedFluid = morphFluid (shoreState);

        // Reset coupling accumulators
        float excitationMod = couplingExcitationMod;
        couplingExcitationMod = 0.0f;
        couplingSeaStateMod = 0.0f;
        couplingSwellMod = 0.0f;
        float pitchMod = couplingPitchMod;
        couplingPitchMod = 0.0f;
        bool audioReplace = couplingAudioReplaceActive;
        float audioReplaceVal = couplingAudioReplace;
        couplingAudioReplaceActive = false;
        couplingAudioReplace = 0.0f;

        // Update tilt filter
        // tilt 0 = dark (low-pass at 800 Hz), tilt 1 = bright (high-pass at 800 Hz)
        if (pFilterTilt < 0.5f)
        {
            float cutoff = 200.0f + (pFilterTilt * 2.0f) * 8000.0f; // 200-8200 Hz LP
            tiltFilterL.setMode (CytomicSVF::Mode::LowPass);
            tiltFilterR.setMode (CytomicSVF::Mode::LowPass);
            tiltFilterL.setCoefficients (cutoff, 0.3f, srf);
            tiltFilterR.setCoefficients (cutoff, 0.3f, srf);
        }
        else
        {
            float cutoff = 8200.0f - ((pFilterTilt - 0.5f) * 2.0f) * 7800.0f; // 8200-400 Hz HP
            tiltFilterL.setMode (CytomicSVF::Mode::HighPass);
            tiltFilterR.setMode (CytomicSVF::Mode::HighPass);
            tiltFilterL.setCoefficients (cutoff, 0.3f, srf);
            tiltFilterR.setCoefficients (cutoff, 0.3f, srf);
        }

        // Update fog filter (gentle LP)
        float fogCutoff = 20000.0f - pFog * 16000.0f; // 20kHz -> 4kHz
        fogFilterL.setMode (CytomicSVF::Mode::LowPass);
        fogFilterR.setMode (CytomicSVF::Mode::LowPass);
        fogFilterL.setCoefficients (fogCutoff, 0.1f, srf);
        fogFilterR.setCoefficients (fogCutoff, 0.1f, srf);

        // Update hull filter (resonant LP for body)
        float hullCutoff = 150.0f + pHull * 600.0f; // 150-750 Hz
        float hullRes = 0.3f + pHull * 0.5f;
        hullFilterL.setMode (CytomicSVF::Mode::LowPass);
        hullFilterR.setMode (CytomicSVF::Mode::LowPass);
        hullFilterL.setCoefficients (hullCutoff, hullRes, srf);
        hullFilterR.setCoefficients (hullCutoff, hullRes, srf);

        // Verb feedback based on verb amount
        float verbFb = 0.3f + effectiveVerb * 0.35f; // 0.3 to 0.65
        for (int i = 0; i < 4; ++i)
            verbAllpass[i].feedback = verbFb;

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(),
                        pAmpA, pAmpD, pAmpS, pAmpR,
                        morphedResonators, morphedCreatures, morphedFluid,
                        effectiveSeaState, pResonatorBright, pResonatorDecay,
                        pCreatureRate);
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
        }

        float peakEnv = 0.0f;

        // --- Render sample loop ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Global fluid energy (one instance, not per-voice)
            float fluidEnergy = fluidModel.process (effectiveSeaState, effectiveSwellPeriod,
                                                     morphedFluid, srf);

            // Apply coupling excitation modulation
            float excitation = fluidEnergy + excitationMod * 0.3f;

            // If AudioToWavetable is active, replace excitation source
            if (audioReplace)
                excitation = audioReplaceVal;

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

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
                voice.currentTargetFreq += (voice.targetFreq - voice.currentTargetFreq)
                                           * voice.glideCoeff;

                // --- Envelope ---
                float ampLevel = voice.ampEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // --- Control-rate update (~2kHz) ---
                voice.controlCounter++;
                if (voice.controlCounter >= controlRateDiv)
                {
                    voice.controlCounter = 0;

                    float baseFreq = voice.currentTargetFreq;

                    // Apply pitch coupling modulation
                    if (std::fabs (pitchMod) > 0.001f)
                        baseFreq *= fastPow2 (pitchMod * 0.5f); // +/- half octave range

                    // Update resonator coefficients based on shore + note pitch
                    // 3 instrument groups x 4 formants = 12 resonators
                    for (int inst = 0; inst < 3; ++inst)
                    {
                        const auto& profile = morphedResonators[inst];
                        for (int f = 0; f < 4; ++f)
                        {
                            int idx = inst * 4 + f;
                            // Scale formant frequency relative to note
                            float freqRatio = profile.formantFreqs[f] / 440.0f;
                            float resFreq = baseFreq * freqRatio;
                            resFreq = clamp (resFreq, 20.0f, srf * 0.45f);

                            // Apply brightness: boost high formants
                            float brightGain = profile.formantGains[f];
                            if (f >= 2)
                                brightGain *= (0.5f + pResonatorBright * 1.0f);
                            else
                                brightGain *= (1.0f - pResonatorBright * 0.3f);

                            // Wind direction shifts energy distribution
                            float windShift = (pWindDir - 0.5f) * 2.0f; // [-1, 1]
                            if (f < 2)
                                brightGain *= (1.0f - windShift * 0.3f);
                            else
                                brightGain *= (1.0f + windShift * 0.3f);

                            brightGain = clamp (brightGain, 0.0f, 1.5f);

                            // Decay -> bandwidth (longer decay = narrower bandwidth)
                            float bw = profile.formantBandwidths[f] / (0.5f + pResonatorDecay);
                            bw = clamp (bw, 5.0f, 2000.0f);

                            // Depth: at high depth, attenuate surface-oriented high formants
                            if (f >= 2)
                                brightGain *= (1.0f - pDepth * 0.5f);

                            voice.resonators[static_cast<size_t>(idx)].setCoefficients (
                                resFreq, bw, brightGain, srf);
                        }
                    }

                    // 4 sympathetic resonators (indices 12-15)
                    // Tuned to harmonics of the fundamental
                    float sympathyRatios[4] = { 2.0f, 3.0f, 4.0f, 5.0f };
                    for (int s = 0; s < 4; ++s)
                    {
                        int idx = 12 + s;
                        float sFreq = baseFreq * sympathyRatios[s];
                        sFreq = clamp (sFreq, 20.0f, srf * 0.45f);
                        float sGain = pSympathyAmount * (0.5f - static_cast<float>(s) * 0.1f);
                        sGain = clamp (sGain, 0.0f, 1.0f);
                        float sBw = 50.0f + (1.0f - pResonatorDecay * 0.1f) * 100.0f;
                        sBw = clamp (sBw, 10.0f, 500.0f);
                        voice.resonators[static_cast<size_t>(idx)].setCoefficients (
                            sFreq, sBw, sGain, srf);
                    }

                    // Update creature formant targets
                    for (int c = 0; c < 3; ++c)
                    {
                        auto& creature = voice.creatures[static_cast<size_t>(c)];
                        creature.configure (morphedCreatures[c], srf);

                        // Trigger creatures based on rate and sea state
                        if (!creature.active)
                        {
                            float triggerChance = pCreatureRate * effectiveSeaState *
                                                  controlDt * 0.5f;
                            if (voice.nextRandomUni() < triggerChance)
                            {
                                creature.active = true;
                                creature.trigger (srf);
                            }
                        }
                    }
                }

                // --- Audio-rate processing ---
                float voiceOut = 0.0f;

                // Excite resonators with fluid energy
                // Add per-voice noise variation for decoherence
                float voiceExcitation = excitation + voice.nextRandom() *
                                        (1.0f - pCoherence) * 0.1f;

                // Process all 16 resonators
                float resonatorSum = 0.0f;
                for (int r = 0; r < kNumResonators; ++r)
                {
                    float resOut = voice.resonators[static_cast<size_t>(r)].process (voiceExcitation);

                    // Coherence: at high coherence, apply phase alignment (gentle LP on output)
                    if (pCoherence > 0.5f)
                    {
                        float coherenceFactor = (pCoherence - 0.5f) * 2.0f; // 0-1
                        voice.coherencePhases[r] += (resOut - voice.coherencePhases[r])
                                                    * (0.1f + coherenceFactor * 0.4f);
                        voice.coherencePhases[r] = flushDenormal (voice.coherencePhases[r]);
                        resOut = lerp (resOut, voice.coherencePhases[r], coherenceFactor * 0.5f);
                    }

                    resonatorSum += resOut;
                }

                // Sympathetic coupling: feed sum back into sympathetic resonators
                if (pSympathyAmount > 0.01f)
                {
                    float sympathyFeedback = resonatorSum * pSympathyAmount * 0.05f;
                    for (int s = 12; s < 16; ++s)
                    {
                        voice.resonators[static_cast<size_t>(s)].s1 +=
                            flushDenormal (sympathyFeedback * 0.1f);
                    }
                }

                // Normalize resonator output
                voiceOut = resonatorSum * (1.0f / 6.0f); // ~sqrt(16) normalization

                // Mix in creature voice formant output
                float creatureOut = 0.0f;
                float creatureExcitation = voice.nextRandom() * 0.5f + fluidEnergy * 0.5f;
                for (int c = 0; c < 3; ++c)
                {
                    creatureOut += voice.creatures[static_cast<size_t>(c)].process (
                        srf, creatureExcitation);
                }
                voiceOut += creatureOut * effectiveCreatureDepth;

                // --- DC Blocker (1-pole highpass at ~5Hz) ---
                constexpr float dcCoeff = 0.9975f;
                float monoIn = voiceOut;
                float dcOutL = monoIn - voice.dcPrevInL + dcCoeff * voice.dcPrevOutL;
                voice.dcPrevInL = monoIn;
                voice.dcPrevOutL = flushDenormal (dcOutL);
                voiceOut = dcOutL;

                // --- Soft limiter (tanh) ---
                voiceOut = fastTanh (voiceOut * 1.5f);

                // --- Apply amplitude envelope, velocity, and crossfade ---
                float gain = ampLevel * voice.velocity * voice.fadeGain;
                voiceOut *= gain;

                // Create stereo spread from per-voice detuning/panning
                // Use voice index and note for consistent stereo image
                float voicePan = static_cast<float> (voice.noteNumber % 12) / 12.0f - 0.5f;
                voicePan *= 0.6f; // limit spread
                float panAngle = (voicePan + 1.0f) * 0.25f * kPI;
                float panL = std::cos (panAngle);
                float panR = std::sin (panAngle);

                float voiceL = voiceOut * panL;
                float voiceR = voiceOut * panR;

                // Denormal protection on outputs
                voiceL = flushDenormal (voiceL);
                voiceR = flushDenormal (voiceR);

                mixL += voiceL;
                mixR += voiceR;

                peakEnv = std::max (peakEnv, ampLevel);
            }

            // --- Post-processing (applied to stereo sum) ---

            // Apply tilt filter
            mixL = tiltFilterL.processSample (mixL);
            mixR = tiltFilterR.processSample (mixR);

            // Apply foam (softClip with drive)
            if (pFoam > 0.01f)
            {
                float foamDrive = 1.0f + pFoam * 8.0f; // 1-9x drive
                mixL = softClip (mixL * foamDrive) / foamDrive * (1.0f + pFoam * 0.5f);
                mixR = softClip (mixR * foamDrive) / foamDrive * (1.0f + pFoam * 0.5f);
            }

            // Apply brine (subtle bit reduction)
            if (pBrine > 0.01f)
            {
                float bitDepth = 16.0f - pBrine * 12.0f; // 16 -> 4 bit
                float levels = fastPow2 (bitDepth);
                float invLevels = 1.0f / levels;
                mixL = std::floor (mixL * levels) * invLevels;
                mixR = std::floor (mixR * levels) * invLevels;
            }

            // Apply hull (body resonance — mix in filtered version)
            if (pHull > 0.01f)
            {
                float hullL = hullFilterL.processSample (mixL);
                float hullR = hullFilterR.processSample (mixR);
                mixL += hullL * pHull * 0.5f;
                mixR += hullR * pHull * 0.5f;
            }

            // Apply fog (gentle LP)
            mixL = fogFilterL.processSample (mixL);
            mixR = fogFilterR.processSample (mixR);

            // Apply harbor verb (simple allpass chain)
            if (effectiveVerb > 0.01f)
            {
                float verbInL = mixL;
                float verbInR = mixR;

                // Process through 4 allpass delays (L channel uses 0,1; R uses 2,3)
                float verbL = verbAllpass[0].process (verbInL);
                verbL = verbAllpass[1].process (verbL);
                float verbR = verbAllpass[2].process (verbInR);
                verbR = verbAllpass[3].process (verbR);

                // Cross-feed for stereo width
                float crossFeed = 0.15f;
                float finalVerbL = verbL + verbR * crossFeed;
                float finalVerbR = verbR + verbL * crossFeed;

                mixL = mixL * (1.0f - effectiveVerb) + finalVerbL * effectiveVerb;
                mixR = mixR * (1.0f - effectiveVerb) + finalVerbR * effectiveVerb;
            }

            // Final denormal protection
            mixL = flushDenormal (mixL);
            mixR = flushDenormal (mixR);

            // Write to output buffer
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sample, mixL);
                buffer.addSample (1, sample, mixR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sample, (mixL + mixR) * 0.5f);
            }

            // Cache for coupling reads
            if (sample < static_cast<int> (outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t> (sample)] = mixL;
                outputCacheR[static_cast<size_t> (sample)] = mixR;
            }
        }

        envelopeOutput = peakEnv;

        // Update active voice count (atomic for thread-safe read from UI)
        int count = 0;
        for (const auto& v : voices)
            if (v.active) ++count;
        activeVoices = count;
    }

    //==========================================================================
    // SynthEngine interface — Coupling
    //==========================================================================

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto si = static_cast<size_t> (sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];
        if (channel == 2) return envelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int numSamples) override
    {
        switch (type)
        {
            case CouplingType::AudioToFM:
                // External audio -> add to resonator excitation energy
                couplingExcitationMod += amount * 0.5f;
                break;

            case CouplingType::AmpToFilter:
                // External amplitude modulates sea state
                couplingSeaStateMod += amount * 0.3f;
                break;

            case CouplingType::EnvToMorph:
                // External envelope modulates swell period
                couplingSwellMod += amount * 0.4f;
                break;

            case CouplingType::LFOToPitch:
                // External LFO modulates resonator tuning
                couplingPitchMod += amount * 0.2f;
                break;

            case CouplingType::AudioToWavetable:
            {
                // External audio replaces resonator excitation source
                couplingAudioReplaceActive = true;
                if (sourceBuffer != nullptr && numSamples > 0)
                {
                    // Use RMS of source buffer as excitation level
                    float sum = 0.0f;
                    int n = std::min (numSamples, 64);
                    for (int i = 0; i < n; ++i)
                        sum += sourceBuffer[i] * sourceBuffer[i];
                    couplingAudioReplace = std::sqrt (sum / static_cast<float> (n)) * amount;
                }
                else
                {
                    couplingAudioReplace = amount * 0.5f;
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
        // --- Core Ocean Parameters ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_shore", 1 }, "Osprey Shore",
            juce::NormalisableRange<float> (0.0f, 4.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_seaState", 1 }, "Osprey Sea State",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_swellPeriod", 1 }, "Osprey Swell Period",
            juce::NormalisableRange<float> (0.5f, 30.0f, 0.01f, 0.4f), 8.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_windDir", 1 }, "Osprey Wind Direction",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_depth", 1 }, "Osprey Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

        // --- Resonator Parameters ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_resonatorBright", 1 }, "Osprey Resonator Brightness",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_resonatorDecay", 1 }, "Osprey Resonator Decay",
            juce::NormalisableRange<float> (0.01f, 8.0f, 0.001f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_sympathyAmount", 1 }, "Osprey Sympathy Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

        // --- Creature Parameters ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_creatureRate", 1 }, "Osprey Creature Rate",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_creatureDepth", 1 }, "Osprey Creature Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

        // --- Coherence & Texture Parameters ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_coherence", 1 }, "Osprey Coherence",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.7f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_foam", 1 }, "Osprey Foam",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_brine", 1 }, "Osprey Brine",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_hull", 1 }, "Osprey Hull",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f));

        // --- Filter & Space Parameters ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_filterTilt", 1 }, "Osprey Filter Tilt",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_harborVerb", 1 }, "Osprey Harbor Verb",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_fog", 1 }, "Osprey Fog",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.1f));

        // --- Amp Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_ampAttack", 1 }, "Osprey Amp Attack",
            juce::NormalisableRange<float> (0.0f, 8.0f, 0.001f, 0.3f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_ampDecay", 1 }, "Osprey Amp Decay",
            juce::NormalisableRange<float> (0.05f, 8.0f, 0.001f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_ampSustain", 1 }, "Osprey Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_ampRelease", 1 }, "Osprey Amp Release",
            juce::NormalisableRange<float> (0.05f, 12.0f, 0.001f, 0.3f), 2.0f));

        // --- Macros ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_macroCharacter", 1 }, "Osprey Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_macroMovement", 1 }, "Osprey Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_macroCoupling", 1 }, "Osprey Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osprey_macroSpace", 1 }, "Osprey Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramShore             = apvts.getRawParameterValue ("osprey_shore");
        paramSeaState          = apvts.getRawParameterValue ("osprey_seaState");
        paramSwellPeriod       = apvts.getRawParameterValue ("osprey_swellPeriod");
        paramWindDir           = apvts.getRawParameterValue ("osprey_windDir");
        paramDepth             = apvts.getRawParameterValue ("osprey_depth");
        paramResonatorBright   = apvts.getRawParameterValue ("osprey_resonatorBright");
        paramResonatorDecay    = apvts.getRawParameterValue ("osprey_resonatorDecay");
        paramSympathyAmount    = apvts.getRawParameterValue ("osprey_sympathyAmount");
        paramCreatureRate      = apvts.getRawParameterValue ("osprey_creatureRate");
        paramCreatureDepth     = apvts.getRawParameterValue ("osprey_creatureDepth");
        paramCoherence         = apvts.getRawParameterValue ("osprey_coherence");
        paramFoam              = apvts.getRawParameterValue ("osprey_foam");
        paramBrine             = apvts.getRawParameterValue ("osprey_brine");
        paramHull              = apvts.getRawParameterValue ("osprey_hull");
        paramFilterTilt        = apvts.getRawParameterValue ("osprey_filterTilt");
        paramHarborVerb        = apvts.getRawParameterValue ("osprey_harborVerb");
        paramFog               = apvts.getRawParameterValue ("osprey_fog");
        paramAmpAttack         = apvts.getRawParameterValue ("osprey_ampAttack");
        paramAmpDecay          = apvts.getRawParameterValue ("osprey_ampDecay");
        paramAmpSustain        = apvts.getRawParameterValue ("osprey_ampSustain");
        paramAmpRelease        = apvts.getRawParameterValue ("osprey_ampRelease");
        paramMacroCharacter    = apvts.getRawParameterValue ("osprey_macroCharacter");
        paramMacroMovement     = apvts.getRawParameterValue ("osprey_macroMovement");
        paramMacroCoupling     = apvts.getRawParameterValue ("osprey_macroCoupling");
        paramMacroSpace        = apvts.getRawParameterValue ("osprey_macroSpace");
    }

    //==========================================================================
    // SynthEngine interface — Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Osprey"; }

    juce::Colour getAccentColour() const override { return juce::Colour (0xFF1B4F8A); }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override { return activeVoices; }

private:
    //==========================================================================
    // Helper: safe parameter load
    //==========================================================================

    static float loadParam (std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    //==========================================================================
    // MIDI note handling
    //==========================================================================

    void noteOn (int noteNumber, float velocity,
                 float ampA, float ampD, float ampS, float ampR,
                 const ResonatorProfile* morphedResonators,
                 const CreatureVoice* morphedCreatures,
                 const FluidCharacter& morphedFluid,
                 float seaState, float resBright, float resDecay,
                 float creatureRate)
    {
        float freq = midiToHz (static_cast<float> (noteNumber));

        // Find a free voice or steal the oldest
        int idx = findFreeVoice();
        auto& voice = voices[static_cast<size_t> (idx)];

        // If stealing, initiate crossfade
        if (voice.active)
        {
            voice.fadingOut = true;
            voice.fadeGain = std::min (voice.fadeGain, 0.5f);
        }

        initVoice (voice, noteNumber, velocity, freq,
                   ampA, ampD, ampS, ampR,
                   morphedResonators, morphedCreatures, morphedFluid,
                   seaState, resBright, resDecay, creatureRate);
    }

    void initVoice (OspreyVoice& voice, int noteNumber, float velocity, float freq,
                    float ampA, float ampD, float ampS, float ampR,
                    const ResonatorProfile* morphedResonators,
                    const CreatureVoice* morphedCreatures,
                    const FluidCharacter& /*morphedFluid*/,
                    float /*seaState*/, float resBright, float resDecay,
                    float creatureRate)
    {
        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.targetFreq = freq;
        voice.currentTargetFreq = freq;
        voice.glideCoeff = 1.0f;
        voice.fadingOut = false;
        voice.fadeGain = 1.0f;
        voice.controlCounter = 0;

        // DC blocker state
        voice.dcPrevInL = 0.0f;
        voice.dcPrevOutL = 0.0f;
        voice.dcPrevInR = 0.0f;
        voice.dcPrevOutR = 0.0f;

        // Initialize PRNG with note-dependent seed
        voice.rng = static_cast<uint32_t> (noteNumber * 7919 + voiceCounter * 104729);

        // Set up envelope
        voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
        voice.ampEnv.noteOn();

        // Initialize resonators with morphed profiles
        for (int inst = 0; inst < 3; ++inst)
        {
            const auto& profile = morphedResonators[inst];
            for (int f = 0; f < 4; ++f)
            {
                int idx = inst * 4 + f;
                auto& res = voice.resonators[static_cast<size_t>(idx)];
                res.reset();

                float freqRatio = profile.formantFreqs[f] / 440.0f;
                float resFreq = freq * freqRatio;
                resFreq = clamp (resFreq, 20.0f, srf * 0.45f);

                float brightGain = profile.formantGains[f];
                if (f >= 2) brightGain *= (0.5f + resBright * 1.0f);

                float bw = profile.formantBandwidths[f] / (0.5f + resDecay);
                bw = clamp (bw, 5.0f, 2000.0f);

                res.setCoefficients (resFreq, bw, brightGain, srf);
            }
        }

        // Initialize sympathetic resonators
        float sympathyRatios[4] = { 2.0f, 3.0f, 4.0f, 5.0f };
        for (int s = 0; s < 4; ++s)
        {
            int idx = 12 + s;
            auto& res = voice.resonators[static_cast<size_t>(idx)];
            res.reset();
            float sFreq = freq * sympathyRatios[s];
            sFreq = clamp (sFreq, 20.0f, srf * 0.45f);
            res.setCoefficients (sFreq, 80.0f, 0.2f, srf);
        }

        // Initialize creature formants
        for (int c = 0; c < 3; ++c)
        {
            auto& creature = voice.creatures[static_cast<size_t>(c)];
            creature.reset();
            creature.configure (morphedCreatures[c], srf);

            // Stagger initial creature triggering
            if (voice.nextRandomUni() < creatureRate * 0.5f)
            {
                creature.active = true;
                creature.trigger (srf);
            }
        }

        // Initialize coherence phases
        for (int i = 0; i < 16; ++i)
            voice.coherencePhases[i] = 0.0f;
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.fadingOut)
            {
                voice.ampEnv.noteOff();
            }
        }
    }

    int findFreeVoice() const
    {
        // Find inactive voice
        for (int i = 0; i < kMaxVoices; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        // LRU voice stealing — find oldest voice
        int oldest = 0;
        uint64_t oldestTime = UINT64_MAX;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (voices[static_cast<size_t> (i)].startTime < oldestTime)
            {
                oldestTime = voices[static_cast<size_t> (i)].startTime;
                oldest = i;
            }
        }
        return oldest;
    }

    static float midiToHz (float midiNote) noexcept
    {
        return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
    }

    //==========================================================================
    // Member data
    //==========================================================================

    double sr = 44100.0;
    float srf = 44100.0f;
    float crossfadeRate = 0.01f;

    // Control rate decimation
    int controlRateDiv = 22;   // ~2kHz at 44.1kHz
    float controlDt = 0.0005f; // 1/2000

    std::array<OspreyVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    int activeVoices = 0;

    // Global fluid energy model
    FluidEnergyModel fluidModel;

    // Coupling accumulators
    float envelopeOutput = 0.0f;
    float couplingExcitationMod = 0.0f;
    float couplingSeaStateMod = 0.0f;
    float couplingSwellMod = 0.0f;
    float couplingPitchMod = 0.0f;
    float couplingAudioReplace = 0.0f;
    bool couplingAudioReplaceActive = false;

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Harbor verb (4 allpass delays)
    AllpassDelay verbAllpass[4];

    // Global filters
    CytomicSVF tiltFilterL, tiltFilterR;
    CytomicSVF fogFilterL, fogFilterR;
    CytomicSVF hullFilterL, hullFilterR;

    // Cached APVTS parameter pointers
    std::atomic<float>* paramShore = nullptr;
    std::atomic<float>* paramSeaState = nullptr;
    std::atomic<float>* paramSwellPeriod = nullptr;
    std::atomic<float>* paramWindDir = nullptr;
    std::atomic<float>* paramDepth = nullptr;
    std::atomic<float>* paramResonatorBright = nullptr;
    std::atomic<float>* paramResonatorDecay = nullptr;
    std::atomic<float>* paramSympathyAmount = nullptr;
    std::atomic<float>* paramCreatureRate = nullptr;
    std::atomic<float>* paramCreatureDepth = nullptr;
    std::atomic<float>* paramCoherence = nullptr;
    std::atomic<float>* paramFoam = nullptr;
    std::atomic<float>* paramBrine = nullptr;
    std::atomic<float>* paramHull = nullptr;
    std::atomic<float>* paramFilterTilt = nullptr;
    std::atomic<float>* paramHarborVerb = nullptr;
    std::atomic<float>* paramFog = nullptr;
    std::atomic<float>* paramAmpAttack = nullptr;
    std::atomic<float>* paramAmpDecay = nullptr;
    std::atomic<float>* paramAmpSustain = nullptr;
    std::atomic<float>* paramAmpRelease = nullptr;
    std::atomic<float>* paramMacroCharacter = nullptr;
    std::atomic<float>* paramMacroMovement = nullptr;
    std::atomic<float>* paramMacroCoupling = nullptr;
    std::atomic<float>* paramMacroSpace = nullptr;
};

} // namespace xomnibus
