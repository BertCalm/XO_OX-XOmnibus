#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xomnibus {

//==============================================================================
// OracleEngine — Stochastic GENDY + Maqam Microtonal Synthesis.
//
// Combines Xenakis-inspired GENDY stochastic waveform generation with an
// authentic maqam microtonal tuning system. N breakpoints define one waveform
// cycle; each cycle, a random walk perturbs both time offsets and amplitudes
// using a blendable Cauchy/Logistic distribution. Mirror barriers with
// configurable elasticity keep values bounded. Cubic Hermite spline
// interpolation produces smooth audio-rate readout.
//
// The maqam system provides 8 Arabic maqamat with quarter-tone intervals,
// blendable between 12-TET and maqam-correct tuning via the gravity parameter.
//
// Features:
//   - GENDY breakpoint engine (8-32 breakpoints per cycle)
//   - Cauchy (heavy-tailed) and Logistic (smooth) distribution blend
//   - Mirror barrier reflection with configurable elasticity
//   - Cubic Hermite spline interpolation between breakpoints
//   - 8 maqamat + 12-TET mode with gravity blend
//   - Stochastic envelope (ADSR modulating random walk step sizes)
//   - DC blocker (5Hz HPF) + soft limiter post-processing
//   - Mono/Legato/Poly4/Poly8 voice modes with LRU stealing + 5ms crossfade
//   - 2 LFOs (Sine/Tri/Saw/Square/S&H)
//   - Full XOmnibus coupling support
//
// Coupling:
//   - Output: post-limiter stereo audio via getSampleForCoupling
//   - Input: AudioToFM (perturb breakpoint amplitudes), AmpToFilter (modulate
//            barrier positions), EnvToMorph (drive distribution morph)
//
//==============================================================================

//==============================================================================
// Constants
//==============================================================================
static constexpr int kOracleMaxBreakpoints = 32;
static constexpr float kOraclePI = 3.14159265358979323846f;
static constexpr float kOracleTwoPi = 6.28318530717958647692f;

//==============================================================================
// Maqam tuning tables — cent offsets from 12-TET per scale degree.
// Each row: 12 entries mapping chromatic degrees 0-11 to cent offsets.
// Mode 0 = 12-TET (all zeros). Modes 1-8 = maqamat.
//==============================================================================
static constexpr int kNumMaqamat = 9; // 0=12-TET + 8 maqamat
static constexpr int kMaqamDegrees = 12;

// Cent offsets from 12-TET for each scale degree.
// For degrees not in the scale, use nearest 12-TET (0 offset).
static constexpr float kMaqamCents[kNumMaqamat][kMaqamDegrees] = {
    // 0: 12-TET (no offsets)
    { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    // 1: Rast — [0, 200, 350, 500, 700, 900, 1050, 1200]
    //    degree 3 (E) = 350 cents vs 400 = -50; degree 10 (Bb) = 1050 vs 1000 = +50
    { 0.0f, 0.0f, 0.0f, -50.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 50.0f, 0.0f },
    // 2: Bayati — [0, 150, 300, 500, 700, 850, 1000, 1200]
    //    degree 1 (Db) = 150 vs 100 = +50; degree 8 (Ab) = 850 vs 800 = +50
    { 0.0f, 50.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 50.0f, 0.0f, 0.0f, 0.0f },
    // 3: Saba — [0, 150, 300, 400, 500, 700, 850, 1000]
    //    degree 1 = +50, degree 3 (E) = 400 vs 400 = 0, degree 8 (Ab) = 850 vs 800 = +50
    { 0.0f, 50.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 50.0f, 0.0f, 0.0f, 0.0f },
    // 4: Hijaz — [0, 100, 400, 500, 700, 900, 1000, 1200]
    //    All degrees at standard 12-TET positions for this layout
    { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    // 5: Sikah — [0, 150, 350, 500, 650, 850, 1000, 1200]
    //    degree 1 = +50, degree 3 (E) = 350 vs 400 = -50, degree 5 (F#) = 650 vs 600 = +50, degree 8 = +50
    { 0.0f, 50.0f, 0.0f, -50.0f, 0.0f, 50.0f, 0.0f, 0.0f, 50.0f, 0.0f, 0.0f, 0.0f },
    // 6: Nahawand — [0, 200, 300, 500, 700, 800, 1100, 1200]
    //    Standard minor-like. All at 12-TET positions.
    { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    // 7: Kurd — [0, 100, 300, 500, 700, 800, 1000, 1200]
    //    Phrygian-like. All at 12-TET positions.
    { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    // 8: Ajam — [0, 200, 400, 500, 700, 900, 1100, 1200]
    //    Major scale. All at 12-TET positions.
    { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
};

//==============================================================================
// ADSR envelope generator — lightweight, inline, no allocation.
//==============================================================================
struct OracleADSR
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

    void noteOn() noexcept { stage = Stage::Attack; }

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
struct OracleLFO
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
                out = fastSin (phase * kOracleTwoPi);
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
// GENDY Breakpoint — (timeOffset, amplitude) pair for one waveform cycle.
//==============================================================================
struct GendyBreakpoint
{
    float time = 0.0f;   // [0, 1] — position within one cycle
    float amp  = 0.0f;   // [-1, 1] — waveform amplitude at this point
};

//==============================================================================
// OracleVoice — per-voice state including GENDY breakpoints and DC blocker.
//==============================================================================
struct OracleVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // GENDY breakpoints — current cycle and next cycle (for crossfade)
    std::array<GendyBreakpoint, kOracleMaxBreakpoints> breakpoints {};
    int numBreakpoints = 16;

    // Phase accumulator — progress through current waveform cycle [0, 1)
    float phase = 0.0f;
    float phaseInc = 0.0f;
    bool cycleComplete = false;

    // Frequency / glide
    float currentFreq = 440.0f;
    float targetFreq = 440.0f;
    float glideCoeff = 1.0f;

    // Per-voice xorshift64 PRNG
    uint64_t prngState = 0xDEADBEEFCAFEBABEULL;

    // Envelopes
    OracleADSR ampEnv;
    OracleADSR stochEnv;  // Modulates stochastic walk step sizes

    // LFOs
    OracleLFO lfo1;
    OracleLFO lfo2;

    // DC blocker state: y[n] = x[n] - x[n-1] + 0.995 * y[n-1]
    float dcX1 = 0.0f;  // previous input
    float dcY1 = 0.0f;  // previous output

    // Voice stealing crossfade
    float fadeGain = 1.0f;
    bool fadingOut = false;

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        phase = 0.0f;
        phaseInc = 0.0f;
        cycleComplete = false;
        currentFreq = 440.0f;
        targetFreq = 440.0f;
        fadeGain = 1.0f;
        fadingOut = false;
        ampEnv.reset();
        stochEnv.reset();
        lfo1.reset();
        lfo2.reset();
        dcX1 = 0.0f;
        dcY1 = 0.0f;
        prngState = 0xDEADBEEFCAFEBABEULL;

        // Initialize breakpoints to a sine-like default
        for (int i = 0; i < kOracleMaxBreakpoints; ++i)
        {
            float t = static_cast<float> (i) / static_cast<float> (kOracleMaxBreakpoints);
            breakpoints[static_cast<size_t> (i)].time = t;
            breakpoints[static_cast<size_t> (i)].amp = std::sin (kOracleTwoPi * t);
        }
    }
};

//==============================================================================
// OracleEngine — the main engine class.
//==============================================================================
class OracleEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    //==========================================================================
    // SynthEngine interface — Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        // Pre-compute smoothing coefficient (5ms time constant)
        smoothCoeff = 1.0f - std::exp (-kOracleTwoPi * (1.0f / 0.005f) / srf);

        // Pre-compute crossfade rate (5ms)
        crossfadeRate = 1.0f / (0.005f * srf);

        // Allocate output cache for coupling reads
        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

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
        couplingBreakpointMod = 0.0f;
        couplingBarrierMod = 0.0f;
        couplingDistribMod = 0.0f;

        smoothedTimeStep = 0.3f;
        smoothedAmpStep = 0.3f;
        smoothedDistribution = 0.5f;

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
        const int   pBreakpointsVal   = static_cast<int> (loadParam (pBreakpoints, 16.0f));
        const float pTimeStepVal      = loadParam (pTimeStep, 0.3f);
        const float pAmpStepVal       = loadParam (pAmpStep, 0.3f);
        const float pDistributionVal  = loadParam (pDistribution, 0.5f);
        const float pBarrierVal       = loadParam (pBarrierElasticity, 0.5f);
        const int   pMaqamVal         = static_cast<int> (loadParam (pMaqam, 0.0f));
        const float pGravityVal       = loadParam (pGravity, 0.0f);
        const float pDriftVal         = loadParam (pDrift, 0.0f);
        const float pLevelVal         = loadParam (pLevel, 0.8f);

        const float pAmpA             = loadParam (pAmpAttack, 0.01f);
        const float pAmpD             = loadParam (pAmpDecay, 0.1f);
        const float pAmpS             = loadParam (pAmpSustain, 0.8f);
        const float pAmpR             = loadParam (pAmpRelease, 0.3f);
        const float pStochA           = loadParam (pStochEnvAttack, 0.05f);
        const float pStochD           = loadParam (pStochEnvDecay, 0.2f);
        const float pStochS           = loadParam (pStochEnvSustain, 0.6f);
        const float pStochR           = loadParam (pStochEnvRelease, 0.5f);

        const float pLfo1R            = loadParam (pLfo1Rate, 1.0f);
        const float pLfo1D            = loadParam (pLfo1Depth, 0.0f);
        const int   pLfo1S            = static_cast<int> (loadParam (pLfo1Shape, 0.0f));
        const float pLfo2R            = loadParam (pLfo2Rate, 1.0f);
        const float pLfo2D            = loadParam (pLfo2Depth, 0.0f);
        const int   pLfo2S            = static_cast<int> (loadParam (pLfo2Shape, 0.0f));

        const int   voiceModeIdx      = static_cast<int> (loadParam (pVoiceMode, 2.0f));
        const float glideTime         = loadParam (pGlide, 0.0f);

        const float macroChar         = loadParam (pMacroCharacter, 0.0f);
        const float macroMove         = loadParam (pMacroMovement, 0.0f);
        const float macroCoup         = loadParam (pMacroCoupling, 0.0f);
        const float macroSpace        = loadParam (pMacroSpace, 0.0f);

        // Determine max polyphony from voice mode
        int maxPoly = kMaxVoices;
        bool monoMode = false;
        bool legatoMode = false;
        switch (voiceModeIdx)
        {
            case 0: maxPoly = 1; monoMode = true; break;          // Mono
            case 1: maxPoly = 1; monoMode = true; legatoMode = true; break; // Legato
            case 2: maxPoly = 4; break;                            // Poly4
            case 3: maxPoly = 8; break;                            // Poly8
            default: maxPoly = 4; break;
        }

        // Glide coefficient
        float glideCoeffVal = 1.0f;
        if (glideTime > 0.001f)
            glideCoeffVal = 1.0f - std::exp (-1.0f / (glideTime * srf));

        // Apply macro offsets to effective parameters
        // M1 CHARACTER: breakpoints + distribution
        int effectiveBreakpoints = clampI (pBreakpointsVal + static_cast<int> (macroChar * 12.0f), 8, 32);
        float effectiveDistribution = clamp (pDistributionVal + macroChar * 0.4f + couplingDistribMod, 0.0f, 1.0f);

        // M2 MOVEMENT: ampStep + timeStep
        float effectiveAmpStep  = clamp (pAmpStepVal + macroMove * 0.4f, 0.0f, 1.0f);
        float effectiveTimeStep = clamp (pTimeStepVal + macroMove * 0.3f, 0.0f, 1.0f);

        // M3 COUPLING: gravity + coupling input gain
        float effectiveGravity = clamp (pGravityVal + macroCoup * 0.5f, 0.0f, 1.0f);
        int effectiveMaqam = clampI (pMaqamVal, 0, kNumMaqamat - 1);

        // M4 SPACE: drift + barrierElasticity
        float effectiveDrift   = clamp (pDriftVal + macroSpace * 0.4f, 0.0f, 1.0f);
        float effectiveBarrier = clamp (pBarrierVal + macroSpace * 0.3f + couplingBarrierMod, 0.0f, 1.0f);

        // Reset coupling accumulators
        couplingBreakpointMod = 0.0f;
        couplingBarrierMod = 0.0f;
        couplingDistribMod = 0.0f;

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), maxPoly, monoMode, legatoMode,
                        glideCoeffVal, effectiveBreakpoints, effectiveMaqam, effectiveGravity,
                        pAmpA, pAmpD, pAmpS, pAmpR,
                        pStochA, pStochD, pStochS, pStochR,
                        pLfo1R, pLfo1D, pLfo1S, pLfo2R, pLfo2D, pLfo2S);
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
        }

        float peakEnv = 0.0f;

        // --- Render sample loop ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Smooth control-rate parameters (5ms)
            smoothedTimeStep     += (effectiveTimeStep - smoothedTimeStep) * smoothCoeff;
            smoothedAmpStep      += (effectiveAmpStep - smoothedAmpStep) * smoothCoeff;
            smoothedDistribution += (effectiveDistribution - smoothedDistribution) * smoothCoeff;

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
                voice.currentFreq += (voice.targetFreq - voice.currentFreq) * voice.glideCoeff;

                // --- Envelopes ---
                float ampLevel   = voice.ampEnv.process();
                float stochLevel = voice.stochEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // --- LFO modulation ---
                float lfo1Val = voice.lfo1.process() * pLfo1D;
                float lfo2Val = voice.lfo2.process() * pLfo2D;

                // LFO1 -> time step modulation, LFO2 -> amp step modulation
                float modTimeStep = clamp (smoothedTimeStep + lfo1Val * 0.3f, 0.0f, 1.0f);
                float modAmpStep  = clamp (smoothedAmpStep + lfo2Val * 0.3f, 0.0f, 1.0f);

                // Scale stochastic steps by stochastic envelope
                float scaledTimeStep = modTimeStep * stochLevel;
                float scaledAmpStep  = modAmpStep * stochLevel;

                // --- Phase increment (with maqam tuning) ---
                float freq = voice.currentFreq;
                // Apply drift: slow random pitch wander
                if (effectiveDrift > 0.001f)
                {
                    float driftAmount = (uniformRandom (voice.prngState) - 0.5f) * 2.0f;
                    driftAmount *= effectiveDrift * 0.005f; // max ~0.5% pitch drift
                    freq *= (1.0f + driftAmount);
                }

                voice.phaseInc = freq / srf;

                // Advance phase
                float prevPhase = voice.phase;
                voice.phase += voice.phaseInc;

                // Check for cycle completion
                if (voice.phase >= 1.0f)
                {
                    voice.phase -= 1.0f;
                    // --- GENDY: update breakpoints once per cycle ---
                    updateBreakpoints (voice, effectiveBreakpoints, scaledTimeStep,
                                       scaledAmpStep, smoothedDistribution,
                                       effectiveBarrier, couplingBreakpointMod);
                }

                // --- Waveform readout: cubic Hermite interpolation ---
                float out = readWaveform (voice, voice.phase);

                // --- DC blocker: y[n] = x[n] - x[n-1] + 0.995 * y[n-1] ---
                float dcOut = out - voice.dcX1 + 0.995f * voice.dcY1;
                dcOut = flushDenormal (dcOut);
                voice.dcX1 = out;
                voice.dcY1 = dcOut;
                out = dcOut;

                // --- Soft limiter: tanh(x * 0.7) / 0.7 ---
                out = fastTanh (out * 0.7f) / 0.7f;

                // --- Apply amplitude envelope, velocity, and crossfade ---
                float gain = ampLevel * voice.velocity * voice.fadeGain;
                float outL = out * gain;
                float outR = out * gain;

                // Slight stereo spread from drift (L/R phase offset)
                if (effectiveDrift > 0.01f)
                {
                    float phaseR = voice.phase + effectiveDrift * 0.02f;
                    if (phaseR >= 1.0f) phaseR -= 1.0f;
                    float outRaw = readWaveform (voice, phaseR);
                    // DC block the R channel variant
                    outRaw = fastTanh (outRaw * 0.7f) / 0.7f;
                    outR = outRaw * gain;
                }

                // Denormal protection
                outL = flushDenormal (outL);
                outR = flushDenormal (outR);

                mixL += outL;
                mixR += outR;

                peakEnv = std::max (peakEnv, ampLevel);
            }

            // Apply master level
            float finalL = mixL * pLevelVal;
            float finalR = mixR * pLevelVal;

            // Write to output buffer
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sample, finalL);
                buffer.addSample (1, sample, finalR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sample, (finalL + finalR) * 0.5f);
            }

            // Cache for coupling reads
            if (sample < static_cast<int> (outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t> (sample)] = finalL;
                outputCacheR[static_cast<size_t> (sample)] = finalR;
            }
        }

        envelopeOutput = peakEnv;

        // Update active voice count
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
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AudioToFM:
                // External audio perturbs breakpoint amplitudes
                couplingBreakpointMod += amount * 0.5f;
                break;

            case CouplingType::AmpToFilter:
                // External amplitude modulates barrier positions
                couplingBarrierMod += amount;
                break;

            case CouplingType::EnvToMorph:
                // External envelope drives distribution morph
                couplingDistribMod += amount * 0.4f;
                break;

            case CouplingType::RhythmToBlend:
                // Rhythm pattern modulates stochastic step sizes via breakpoint mod
                couplingBreakpointMod += amount * 0.3f;
                break;

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
        // --- Core GENDY Parameters ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_breakpoints", 1 }, "Oracle Breakpoints",
            juce::NormalisableRange<float> (8.0f, 32.0f, 1.0f), 16.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_timeStep", 1 }, "Oracle Time Step",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_ampStep", 1 }, "Oracle Amp Step",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_distribution", 1 }, "Oracle Distribution",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_barrierElasticity", 1 }, "Oracle Barrier Elasticity",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        // --- Maqam Tuning ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_maqam", 1 }, "Oracle Maqam",
            juce::NormalisableRange<float> (0.0f, 8.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_gravity", 1 }, "Oracle Gravity",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // --- Drift ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_drift", 1 }, "Oracle Drift",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // --- Level ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_level", 1 }, "Oracle Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // --- Amp Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_ampAttack", 1 }, "Oracle Amp Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_ampDecay", 1 }, "Oracle Amp Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.1f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_ampSustain", 1 }, "Oracle Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_ampRelease", 1 }, "Oracle Amp Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // --- Stochastic Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_stochEnvAttack", 1 }, "Oracle Stoch Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.05f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_stochEnvDecay", 1 }, "Oracle Stoch Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.2f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_stochEnvSustain", 1 }, "Oracle Stoch Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_stochEnvRelease", 1 }, "Oracle Stoch Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.5f));

        // --- LFO 1 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_lfo1Rate", 1 }, "Oracle LFO1 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_lfo1Depth", 1 }, "Oracle LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "oracle_lfo1Shape", 1 }, "Oracle LFO1 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- LFO 2 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_lfo2Rate", 1 }, "Oracle LFO2 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_lfo2Depth", 1 }, "Oracle LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "oracle_lfo2Shape", 1 }, "Oracle LFO2 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- Voice Parameters ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "oracle_voiceMode", 1 }, "Oracle Voice Mode",
            juce::StringArray { "Mono", "Legato", "Poly4", "Poly8" }, 2));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_glide", 1 }, "Oracle Glide",
            juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

        // --- Macros ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_macroCharacter", 1 }, "Oracle Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_macroMovement", 1 }, "Oracle Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_macroCoupling", 1 }, "Oracle Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_macroSpace", 1 }, "Oracle Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pBreakpoints        = apvts.getRawParameterValue ("oracle_breakpoints");
        pTimeStep           = apvts.getRawParameterValue ("oracle_timeStep");
        pAmpStep            = apvts.getRawParameterValue ("oracle_ampStep");
        pDistribution       = apvts.getRawParameterValue ("oracle_distribution");
        pBarrierElasticity  = apvts.getRawParameterValue ("oracle_barrierElasticity");
        pMaqam              = apvts.getRawParameterValue ("oracle_maqam");
        pGravity            = apvts.getRawParameterValue ("oracle_gravity");
        pDrift              = apvts.getRawParameterValue ("oracle_drift");
        pLevel              = apvts.getRawParameterValue ("oracle_level");

        pAmpAttack          = apvts.getRawParameterValue ("oracle_ampAttack");
        pAmpDecay           = apvts.getRawParameterValue ("oracle_ampDecay");
        pAmpSustain         = apvts.getRawParameterValue ("oracle_ampSustain");
        pAmpRelease         = apvts.getRawParameterValue ("oracle_ampRelease");

        pStochEnvAttack     = apvts.getRawParameterValue ("oracle_stochEnvAttack");
        pStochEnvDecay      = apvts.getRawParameterValue ("oracle_stochEnvDecay");
        pStochEnvSustain    = apvts.getRawParameterValue ("oracle_stochEnvSustain");
        pStochEnvRelease    = apvts.getRawParameterValue ("oracle_stochEnvRelease");

        pLfo1Rate           = apvts.getRawParameterValue ("oracle_lfo1Rate");
        pLfo1Depth          = apvts.getRawParameterValue ("oracle_lfo1Depth");
        pLfo1Shape          = apvts.getRawParameterValue ("oracle_lfo1Shape");
        pLfo2Rate           = apvts.getRawParameterValue ("oracle_lfo2Rate");
        pLfo2Depth          = apvts.getRawParameterValue ("oracle_lfo2Depth");
        pLfo2Shape          = apvts.getRawParameterValue ("oracle_lfo2Shape");

        pVoiceMode          = apvts.getRawParameterValue ("oracle_voiceMode");
        pGlide              = apvts.getRawParameterValue ("oracle_glide");

        pMacroCharacter     = apvts.getRawParameterValue ("oracle_macroCharacter");
        pMacroMovement      = apvts.getRawParameterValue ("oracle_macroMovement");
        pMacroCoupling      = apvts.getRawParameterValue ("oracle_macroCoupling");
        pMacroSpace         = apvts.getRawParameterValue ("oracle_macroSpace");
    }

    //==========================================================================
    // SynthEngine interface — Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Oracle"; }

    juce::Colour getAccentColour() const override { return juce::Colour (0xFF4B0082); }

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

    static int clampI (int x, int lo, int hi) noexcept
    {
        return (x < lo) ? lo : ((x > hi) ? hi : x);
    }

    //==========================================================================
    // xorshift64 PRNG — fast, per-voice, deterministic.
    //==========================================================================

    static uint64_t xorshift64 (uint64_t& state) noexcept
    {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return state;
    }

    // Uniform random in (0, 1)
    static float uniformRandom (uint64_t& state) noexcept
    {
        uint64_t x = xorshift64 (state);
        // Use upper 32 bits for better distribution
        return (static_cast<float> (x >> 32) + 1.0f) / 4294967298.0f; // (0, 1) exclusive
    }

    // Cauchy distribution sample — heavy-tailed, dramatic jumps
    static float cauchySample (uint64_t& state) noexcept
    {
        float u = uniformRandom (state);
        // Clamp away from 0 and 1 to avoid infinities
        u = clamp (u, 0.01f, 0.99f);
        return std::tan (kOraclePI * (u - 0.5f));
    }

    // Logistic distribution sample — smooth, gradual changes
    static float logisticSample (uint64_t& state) noexcept
    {
        float u = uniformRandom (state);
        // Clamp away from 0 and 1 to avoid infinities
        u = clamp (u, 0.01f, 0.99f);
        return std::log (u / (1.0f - u));
    }

    // Blended stochastic sample: distribution [0,1] blends Cauchy <-> Logistic
    static float stochasticSample (uint64_t& state, float distribution) noexcept
    {
        float c = cauchySample (state);
        float l = logisticSample (state);
        // distribution 0 = pure Cauchy, 1 = pure Logistic
        return c * (1.0f - distribution) + l * distribution;
    }

    //==========================================================================
    // Mirror barrier reflection — elastic boundaries.
    //==========================================================================

    static float mirrorBarrier (float val, float lo, float hi, float elasticity) noexcept
    {
        // Elasticity controls how much energy is preserved on reflection.
        // 1.0 = perfect elastic, 0.0 = hard clamp (no bounce).
        int maxIter = 8; // Prevent infinite loop
        while ((val < lo || val > hi) && maxIter-- > 0)
        {
            if (val < lo)
            {
                float overshoot = lo - val;
                val = lo + overshoot * elasticity;
            }
            if (val > hi)
            {
                float overshoot = val - hi;
                val = hi - overshoot * elasticity;
            }
        }
        // Final hard clamp as safety
        return clamp (val, lo, hi);
    }

    //==========================================================================
    // GENDY: Update breakpoints — called once per waveform cycle.
    //==========================================================================

    void updateBreakpoints (OracleVoice& voice, int numBP, float timeStepSize,
                            float ampStepSize, float distribution, float elasticity,
                            float couplingAmpMod) noexcept
    {
        int n = clampI (numBP, 8, kOracleMaxBreakpoints);
        voice.numBreakpoints = n;

        // Scale step sizes: map [0,1] to useful stochastic ranges
        // timeStepSize controls maximum time perturbation per cycle
        float timeScale = timeStepSize * 0.15f;  // max 15% of cycle span
        // ampStepSize controls maximum amplitude perturbation per cycle
        float ampScale  = ampStepSize * 0.4f;    // max 40% of amplitude range

        for (int i = 0; i < n; ++i)
        {
            auto& bp = voice.breakpoints[static_cast<size_t> (i)];

            // Random walk on time offset
            float timeDelta = stochasticSample (voice.prngState, distribution) * timeScale;
            bp.time += timeDelta;

            // Random walk on amplitude
            float ampDelta = stochasticSample (voice.prngState, distribution) * ampScale;
            // Apply coupling modulation to amplitude perturbation
            ampDelta += couplingAmpMod * 0.1f;
            bp.amp += ampDelta;

            // Mirror barrier reflection
            bp.time = mirrorBarrier (bp.time, 0.0f, 1.0f, elasticity);
            bp.amp  = mirrorBarrier (bp.amp, -1.0f, 1.0f, elasticity);
        }

        // Sort breakpoints by time to maintain valid waveform ordering
        std::sort (voice.breakpoints.begin(),
                   voice.breakpoints.begin() + n,
                   [] (const GendyBreakpoint& a, const GendyBreakpoint& b) {
                       return a.time < b.time;
                   });

        // Normalize time span: first breakpoint at 0, last at 1
        if (n >= 2)
        {
            float tMin = voice.breakpoints[0].time;
            float tMax = voice.breakpoints[static_cast<size_t> (n - 1)].time;
            float span = tMax - tMin;
            if (span < 0.001f) span = 0.001f;

            for (int i = 0; i < n; ++i)
            {
                voice.breakpoints[static_cast<size_t> (i)].time =
                    (voice.breakpoints[static_cast<size_t> (i)].time - tMin) / span;
            }
        }

        // Ensure endpoints
        voice.breakpoints[0].time = 0.0f;
        voice.breakpoints[static_cast<size_t> (n - 1)].time = 1.0f;
    }

    //==========================================================================
    // Cubic Hermite spline interpolation between breakpoints.
    //==========================================================================

    float readWaveform (const OracleVoice& voice, float phase) const noexcept
    {
        int n = voice.numBreakpoints;
        if (n < 2) return 0.0f;

        // Wrap phase to [0, 1)
        phase = phase - static_cast<float> (static_cast<int> (phase));
        if (phase < 0.0f) phase += 1.0f;

        // Find the segment: breakpoints[i].time <= phase < breakpoints[i+1].time
        int seg = 0;
        for (int i = 0; i < n - 1; ++i)
        {
            if (phase >= voice.breakpoints[static_cast<size_t> (i)].time)
                seg = i;
        }

        int i0 = seg;
        int i1 = std::min (seg + 1, n - 1);

        float t0 = voice.breakpoints[static_cast<size_t> (i0)].time;
        float t1 = voice.breakpoints[static_cast<size_t> (i1)].time;
        float a0 = voice.breakpoints[static_cast<size_t> (i0)].amp;
        float a1 = voice.breakpoints[static_cast<size_t> (i1)].amp;

        float dt = t1 - t0;
        if (dt < 0.0001f) return a0;

        float t = (phase - t0) / dt; // Local interpolation parameter [0, 1]

        // Get neighboring points for tangent calculation
        int iPrev = (i0 > 0) ? i0 - 1 : n - 1;
        int iNext = (i1 < n - 1) ? i1 + 1 : 0;

        float aPrev = voice.breakpoints[static_cast<size_t> (iPrev)].amp;
        float aNext = voice.breakpoints[static_cast<size_t> (iNext)].amp;

        // Catmull-Rom tangents (finite difference)
        float m0 = (a1 - aPrev) * 0.5f;
        float m1 = (aNext - a0) * 0.5f;

        // Cubic Hermite basis functions
        float t2 = t * t;
        float t3 = t2 * t;
        float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
        float h10 = t3 - 2.0f * t2 + t;
        float h01 = -2.0f * t3 + 3.0f * t2;
        float h11 = t3 - t2;

        float result = h00 * a0 + h10 * m0 + h01 * a1 + h11 * m1;

        // Clamp output to [-1, 1] for safety
        return clamp (result, -1.2f, 1.2f);
    }

    //==========================================================================
    // Maqam tuning: convert MIDI note to frequency with maqam offsets.
    //==========================================================================

    float maqamMidiToHz (int noteNumber, int maqamIdx, float gravity) const noexcept
    {
        // Base 12-TET frequency
        float baseFreq = 440.0f * std::pow (2.0f, (static_cast<float> (noteNumber) - 69.0f) / 12.0f);

        if (maqamIdx <= 0 || maqamIdx >= kNumMaqamat || gravity < 0.001f)
            return baseFreq;

        // Get the chromatic degree (0-11) within the octave
        int degree = noteNumber % 12;
        if (degree < 0) degree += 12;

        // Look up cent offset for this maqam and degree
        float centOffset = kMaqamCents[maqamIdx][degree];

        // Apply gravity: 0 = 12-TET, 1 = full maqam tuning
        float effectiveCents = centOffset * gravity;

        // Convert cent offset to frequency ratio: ratio = 2^(cents/1200)
        float ratio = std::pow (2.0f, effectiveCents / 1200.0f);

        return baseFreq * ratio;
    }

    //==========================================================================
    // MIDI note handling.
    //==========================================================================

    void noteOn (int noteNumber, float velocity, int maxPoly,
                 bool monoMode, bool legatoMode, float glideCoeffVal,
                 int numBreakpoints, int maqamIdx, float gravity,
                 float ampA, float ampD, float ampS, float ampR,
                 float stochA, float stochD, float stochS, float stochR,
                 float lfo1Rate, float lfo1Depth, int lfo1Shape,
                 float lfo2Rate, float lfo2Depth, int lfo2Shape)
    {
        float freq = maqamMidiToHz (noteNumber, maqamIdx, gravity);

        if (monoMode)
        {
            auto& voice = voices[0];
            bool wasActive = voice.active;

            voice.targetFreq = freq;

            if (legatoMode && wasActive)
            {
                // Legato: don't retrigger envelopes, just glide
                voice.glideCoeff = glideCoeffVal;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
            }
            else
            {
                voice.active = true;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
                voice.startTime = voiceCounter++;
                voice.currentFreq = freq;
                voice.glideCoeff = glideCoeffVal;
                voice.phase = 0.0f;
                voice.fadingOut = false;
                voice.fadeGain = 1.0f;
                voice.dcX1 = 0.0f;
                voice.dcY1 = 0.0f;

                // Seed PRNG from note number + velocity for deterministic character
                voice.prngState = static_cast<uint64_t> (noteNumber) * 2654435761ULL
                                + static_cast<uint64_t> (velocity * 1000.0f) * 40503ULL
                                + 0xDEADBEEFCAFEBABEULL;

                // Initialize breakpoints to a sine-like starting shape
                initBreakpoints (voice, numBreakpoints);

                voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
                voice.ampEnv.noteOn();
                voice.stochEnv.setParams (stochA, stochD, stochS, stochR, srf);
                voice.stochEnv.noteOn();

                voice.lfo1.setRate (lfo1Rate, srf);
                voice.lfo1.setShape (lfo1Shape);
                voice.lfo2.setRate (lfo2Rate, srf);
                voice.lfo2.setShape (lfo2Shape);
            }
            return;
        }

        // Polyphonic mode
        int idx = findFreeVoice (maxPoly);
        auto& voice = voices[static_cast<size_t> (idx)];

        // If stealing, initiate crossfade
        if (voice.active)
        {
            voice.fadingOut = true;
            voice.fadeGain = std::min (voice.fadeGain, 0.5f);
        }

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.currentFreq = freq;
        voice.targetFreq = freq;
        voice.glideCoeff = 1.0f;
        voice.phase = 0.0f;
        voice.fadingOut = false;
        voice.fadeGain = 1.0f;
        voice.dcX1 = 0.0f;
        voice.dcY1 = 0.0f;

        // Seed PRNG
        voice.prngState = static_cast<uint64_t> (noteNumber) * 2654435761ULL
                        + static_cast<uint64_t> (velocity * 1000.0f) * 40503ULL
                        + voiceCounter * 6364136223846793005ULL
                        + 0xDEADBEEFCAFEBABEULL;

        initBreakpoints (voice, numBreakpoints);

        voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
        voice.ampEnv.noteOn();
        voice.stochEnv.setParams (stochA, stochD, stochS, stochR, srf);
        voice.stochEnv.noteOn();

        voice.lfo1.setRate (lfo1Rate, srf);
        voice.lfo1.setShape (lfo1Shape);
        voice.lfo1.reset();
        voice.lfo2.setRate (lfo2Rate, srf);
        voice.lfo2.setShape (lfo2Shape);
        voice.lfo2.reset();
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.fadingOut)
            {
                voice.ampEnv.noteOff();
                voice.stochEnv.noteOff();
            }
        }
    }

    //==========================================================================
    // Initialize breakpoints to a sine-like shape.
    //==========================================================================

    void initBreakpoints (OracleVoice& voice, int numBP) noexcept
    {
        int n = clampI (numBP, 8, kOracleMaxBreakpoints);
        voice.numBreakpoints = n;

        for (int i = 0; i < n; ++i)
        {
            float t = static_cast<float> (i) / static_cast<float> (n);
            voice.breakpoints[static_cast<size_t> (i)].time = t;
            voice.breakpoints[static_cast<size_t> (i)].amp = std::sin (kOracleTwoPi * t);
        }

        // Add slight per-voice randomization so simultaneous notes don't sound identical
        for (int i = 0; i < n; ++i)
        {
            float r = (uniformRandom (voice.prngState) - 0.5f) * 0.1f;
            voice.breakpoints[static_cast<size_t> (i)].amp += r;
            voice.breakpoints[static_cast<size_t> (i)].amp =
                clamp (voice.breakpoints[static_cast<size_t> (i)].amp, -1.0f, 1.0f);
        }

        // Ensure endpoints
        voice.breakpoints[0].time = 0.0f;
        voice.breakpoints[static_cast<size_t> (n - 1)].time = 1.0f;
    }

    //==========================================================================
    // LRU voice finding.
    //==========================================================================

    int findFreeVoice (int maxPoly) const
    {
        int poly = std::min (maxPoly, kMaxVoices);

        // Find inactive voice within polyphony limit
        for (int i = 0; i < poly; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        // LRU voice stealing — find oldest voice
        int oldest = 0;
        uint64_t oldestTime = UINT64_MAX;
        for (int i = 0; i < poly; ++i)
        {
            if (voices[static_cast<size_t> (i)].startTime < oldestTime)
            {
                oldestTime = voices[static_cast<size_t> (i)].startTime;
                oldest = i;
            }
        }
        return oldest;
    }

    //==========================================================================
    // Member data
    //==========================================================================

    double sr = 44100.0;
    float srf = 44100.0f;
    float smoothCoeff = 0.1f;
    float crossfadeRate = 0.01f;

    std::array<OracleVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    int activeVoices = 0;

    // Smoothed control parameters
    float smoothedTimeStep = 0.3f;
    float smoothedAmpStep = 0.3f;
    float smoothedDistribution = 0.5f;

    // Coupling accumulators
    float envelopeOutput = 0.0f;
    float couplingBreakpointMod = 0.0f;  // AudioToFM -> breakpoint amplitude perturbation
    float couplingBarrierMod = 0.0f;     // AmpToFilter -> barrier position modulation
    float couplingDistribMod = 0.0f;     // EnvToMorph -> distribution blend modulation

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS parameter pointers
    std::atomic<float>* pBreakpoints = nullptr;
    std::atomic<float>* pTimeStep = nullptr;
    std::atomic<float>* pAmpStep = nullptr;
    std::atomic<float>* pDistribution = nullptr;
    std::atomic<float>* pBarrierElasticity = nullptr;
    std::atomic<float>* pMaqam = nullptr;
    std::atomic<float>* pGravity = nullptr;
    std::atomic<float>* pDrift = nullptr;
    std::atomic<float>* pLevel = nullptr;

    std::atomic<float>* pAmpAttack = nullptr;
    std::atomic<float>* pAmpDecay = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;

    std::atomic<float>* pStochEnvAttack = nullptr;
    std::atomic<float>* pStochEnvDecay = nullptr;
    std::atomic<float>* pStochEnvSustain = nullptr;
    std::atomic<float>* pStochEnvRelease = nullptr;

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

} // namespace xomnibus
