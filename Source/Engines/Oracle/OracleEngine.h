#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xomnibus {

//==============================================================================
// OracleEngine — Stochastic GENDY + Maqam Microtonal Synthesis.
//
// Fuses Xenakis's GENDY stochastic waveform generation with Middle-Eastern
// maqam microtonal tuning systems. Breakpoints define a single waveform cycle
// and undergo random walks each cycle, producing constantly evolving timbres
// that range from smooth undulations to chaotic noise sculptures.
//
// Features:
//   - N breakpoints (8-32) per waveform cycle with stochastic random walk
//   - Two morphable distributions: Cauchy (heavy-tailed) and Logistic (smooth)
//   - Mirror barrier reflection on breakpoint boundaries
//   - Cubic Hermite interpolation between breakpoints (per-sample)
//   - 8 maqamat with quarter-tone microtonal tuning
//   - Gravity parameter blends 12-TET and maqam tuning
//   - DC blocker (5Hz first-order HPF) + soft limiter post-processing
//   - Mono/Legato/Poly4/Poly8 voice modes with LRU stealing + 5ms crossfade
//   - 2 LFOs (Sine/Tri/Saw/Square/S&H)
//   - Full XOmnibus coupling support
//
// Coupling:
//   - Output: post-processing stereo audio via getSampleForCoupling
//   - Input: AudioToFM (perturb breakpoint amplitudes),
//            AmpToFilter (modulate barrier positions),
//            EnvToMorph (drive distribution morph)
//
// Macro Labels: PROPHECY, EVOLUTION, GRAVITY, DRIFT
//
//==============================================================================

//==============================================================================
// Maqam tuning tables — cent offsets from 12-TET for each scale degree.
// Each maqam has 7 intervals (8 notes including octave).
// Cents are relative to the scale root (degree 0 = 0 cents).
// Quarter-tone = 50 cents. Standard semitone = 100 cents.
//==============================================================================
struct MaqamTable
{
    // Cent offsets for degrees 0-7 (octave at index 7 = 1200).
    float cents[8];
};

static constexpr int kNumMaqamat = 8;

// The 8 maqamat: Rast, Bayati, Saba, Hijaz, Sikah, Nahawand, Kurd, Ajam
static const MaqamTable kMaqamat[kNumMaqamat] = {
    // 0: Rast — C D Ed F G A Bd C (Ed = E half-flat, Bd = B half-flat)
    {{ 0.0f, 200.0f, 350.0f, 500.0f, 700.0f, 900.0f, 1050.0f, 1200.0f }},
    // 1: Bayati — D Ed F G A Bb C D
    {{ 0.0f, 150.0f, 300.0f, 500.0f, 700.0f, 800.0f, 1000.0f, 1200.0f }},
    // 2: Saba — D Ed F Gb A Bb C D
    {{ 0.0f, 150.0f, 300.0f, 400.0f, 700.0f, 800.0f, 1000.0f, 1200.0f }},
    // 3: Hijaz — D Eb F# G A Bb C D (augmented second)
    {{ 0.0f, 100.0f, 400.0f, 500.0f, 700.0f, 800.0f, 1000.0f, 1200.0f }},
    // 4: Sikah — Ed F G Ab Bd C D Ed
    {{ 0.0f, 150.0f, 350.0f, 450.0f, 650.0f, 850.0f, 1050.0f, 1200.0f }},
    // 5: Nahawand — C D Eb F G Ab Bb C (harmonic minor variant)
    {{ 0.0f, 200.0f, 300.0f, 500.0f, 700.0f, 800.0f, 1000.0f, 1200.0f }},
    // 6: Kurd — D Eb F G A Bb C D (Phrygian-like)
    {{ 0.0f, 100.0f, 300.0f, 500.0f, 700.0f, 800.0f, 1000.0f, 1200.0f }},
    // 7: Ajam — C D E F G A B C (major / Ionian)
    {{ 0.0f, 200.0f, 400.0f, 500.0f, 700.0f, 900.0f, 1100.0f, 1200.0f }}
};

//==============================================================================
// xorshift64 PRNG — fast, per-voice, deterministic from seed.
//==============================================================================
struct Xorshift64
{
    uint64_t state = 1;

    void seed (uint64_t s) noexcept
    {
        state = (s == 0) ? 1 : s;
    }

    uint64_t next() noexcept
    {
        uint64_t x = state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        state = x;
        return x;
    }

    // Uniform float in [0, 1)
    float uniform() noexcept
    {
        return static_cast<float> (next() & 0xFFFFFFFF) / 4294967296.0f;
    }

    // Uniform float in [-1, 1)
    float uniformBipolar() noexcept
    {
        return uniform() * 2.0f - 1.0f;
    }
};

//==============================================================================
// GENDY Breakpoint
//==============================================================================
static constexpr int kMaxBreakpoints = 32;

struct GENDYBreakpoint
{
    float timeOffset = 0.0f;   // [0, 1]
    float amplitude  = 0.0f;   // [-1, 1]
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
// DC Blocker — first-order HPF at ~5Hz.
// y[n] = x[n] - x[n-1] + R * y[n-1], where R = 1 - (2*pi*fc/sr)
//==============================================================================
struct DCBlocker
{
    float x1 = 0.0f;
    float y1 = 0.0f;
    float R  = 0.9999f;

    void prepare (float sampleRate) noexcept
    {
        constexpr float fc = 5.0f;
        constexpr float twoPi = 6.28318530718f;
        R = 1.0f - (twoPi * fc / std::max (1.0f, sampleRate));
        if (R < 0.9f) R = 0.9f;
        if (R > 0.9999f) R = 0.9999f;
    }

    float process (float x) noexcept
    {
        float y = x - x1 + R * y1;
        x1 = x;
        y1 = flushDenormal (y);
        return y1;
    }

    void reset() noexcept
    {
        x1 = 0.0f;
        y1 = 0.0f;
    }
};

//==============================================================================
// OracleVoice — per-voice state.
//==============================================================================
struct OracleVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // GENDY breakpoints (current cycle waveform)
    GENDYBreakpoint breakpoints[kMaxBreakpoints];
    int numBreakpoints = 16;

    // Waveform phase: tracks position within current cycle [0, 1)
    float wavePhase = 0.0f;
    float wavePhaseInc = 0.0f;  // freq / sampleRate

    // Frequency
    float currentFreq = 440.0f;
    float targetFreq  = 440.0f;
    float glideCoeff  = 1.0f;

    // Per-voice PRNG
    Xorshift64 rng;

    // Envelopes
    OracleADSR ampEnv;
    OracleADSR stochEnv;  // modulates stochastic evolution depth

    // LFOs
    OracleLFO lfo1;
    OracleLFO lfo2;

    // DC blocker (stereo)
    DCBlocker dcBlockerL;
    DCBlocker dcBlockerR;

    // Voice stealing crossfade
    float fadeGain = 1.0f;
    bool fadingOut = false;

    // Cached last output for stereo spread
    float lastOutputL = 0.0f;
    float lastOutputR = 0.0f;

    // Previous cycle output for crossfade smoothing at cycle boundary
    float prevCycleLastSample = 0.0f;
    int cycleBlendCounter = 0;
    static constexpr int kCycleBlendSamples = 64;

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        wavePhase = 0.0f;
        wavePhaseInc = 0.0f;
        currentFreq = 440.0f;
        targetFreq = 440.0f;
        fadeGain = 1.0f;
        fadingOut = false;
        lastOutputL = 0.0f;
        lastOutputR = 0.0f;
        prevCycleLastSample = 0.0f;
        cycleBlendCounter = 0;
        ampEnv.reset();
        stochEnv.reset();
        lfo1.reset();
        lfo2.reset();
        dcBlockerL.reset();
        dcBlockerR.reset();
        rng.seed (1);

        // Initialize breakpoints to a sine-like shape
        for (int i = 0; i < kMaxBreakpoints; ++i)
        {
            float t = static_cast<float> (i) / static_cast<float> (kMaxBreakpoints);
            breakpoints[i].timeOffset = t;
            breakpoints[i].amplitude = std::sin (t * 6.28318530718f);
        }
    }

    //--------------------------------------------------------------------------
    // Initialize breakpoints for a given count — distribute evenly with
    // a sine-like amplitude shape for a clean starting waveform.
    //--------------------------------------------------------------------------
    void initBreakpoints (int count) noexcept
    {
        numBreakpoints = std::max (8, std::min (kMaxBreakpoints, count));
        for (int i = 0; i < numBreakpoints; ++i)
        {
            float t = static_cast<float> (i) / static_cast<float> (numBreakpoints);
            breakpoints[i].timeOffset = t;
            breakpoints[i].amplitude = std::sin (t * 6.28318530718f);
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
    static constexpr float kPI = 3.14159265358979323846f;
    static constexpr float kTwoPi = 6.28318530717958647692f;

    //==========================================================================
    // SynthEngine interface — Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        // Smoothing coefficient (5ms time constant)
        smoothCoeff = 1.0f - std::exp (-kTwoPi * (1.0f / 0.005f) / srf);

        // Crossfade rate (5ms)
        crossfadeRate = 1.0f / (0.005f * srf);

        // Allocate output cache for coupling reads
        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        // Initialize voices
        for (auto& v : voices)
        {
            v.reset();
            v.dcBlockerL.prepare (srf);
            v.dcBlockerR.prepare (srf);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        envelopeOutput = 0.0f;
        couplingBreakpointMod = 0.0f;
        couplingBarrierMod = 0.0f;
        couplingDistributionMod = 0.0f;

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
        const int   pBpCount      = static_cast<int> (loadParam (pBreakpoints, 16.0f));
        const float pTimeStepVal  = loadParam (pTimeStep, 0.3f);
        const float pAmpStepVal   = loadParam (pAmpStep, 0.3f);
        const float pDistVal      = loadParam (pDistribution, 0.5f);
        const float pElasticVal   = loadParam (pBarrierElasticity, 0.5f);
        const int   pMaqamIdx     = static_cast<int> (loadParam (pMaqam, 0.0f));
        const float pGravityVal   = loadParam (pGravity, 0.0f);
        const float pDriftVal     = loadParam (pDrift, 0.3f);
        const float pLevelVal     = loadParam (pLevel, 0.8f);

        const float pAmpA         = loadParam (pAmpAttack, 0.01f);
        const float pAmpD         = loadParam (pAmpDecay, 0.1f);
        const float pAmpS         = loadParam (pAmpSustain, 0.8f);
        const float pAmpR         = loadParam (pAmpRelease, 0.3f);
        const float pStochA       = loadParam (pStochEnvAttack, 0.05f);
        const float pStochD       = loadParam (pStochEnvDecay, 0.3f);
        const float pStochS       = loadParam (pStochEnvSustain, 0.7f);
        const float pStochR       = loadParam (pStochEnvRelease, 0.5f);

        const float pLfo1R        = loadParam (pLfo1Rate, 1.0f);
        const float pLfo1D        = loadParam (pLfo1Depth, 0.0f);
        const int   pLfo1S        = static_cast<int> (loadParam (pLfo1Shape, 0.0f));
        const float pLfo2R        = loadParam (pLfo2Rate, 1.0f);
        const float pLfo2D        = loadParam (pLfo2Depth, 0.0f);
        const int   pLfo2S        = static_cast<int> (loadParam (pLfo2Shape, 0.0f));

        const int   voiceModeIdx  = static_cast<int> (loadParam (pVoiceMode, 2.0f));
        const float glideTime     = loadParam (pGlide, 0.0f);

        const float macroProphecy  = loadParam (pMacroProphecy, 0.0f);
        const float macroEvolution = loadParam (pMacroEvolution, 0.0f);
        const float macroGravity   = loadParam (pMacroGravity, 0.0f);
        const float macroDrift     = loadParam (pMacroDrift, 0.0f);

        // Determine max polyphony from voice mode
        int maxPoly = kMaxVoices;
        bool monoMode = false;
        bool legatoMode = false;
        switch (voiceModeIdx)
        {
            case 0: maxPoly = 1; monoMode = true; break;           // Mono
            case 1: maxPoly = 1; monoMode = true; legatoMode = true; break; // Legato
            case 2: maxPoly = 4; break;                              // Poly4
            case 3: maxPoly = 8; break;                              // Poly8
            default: maxPoly = 4; break;
        }

        // Glide coefficient
        float glideCoeffCalc = 1.0f;
        if (glideTime > 0.001f)
            glideCoeffCalc = 1.0f - std::exp (-1.0f / (glideTime * srf));

        // Apply macro and coupling offsets to effective parameters
        float effectiveTimeStep = clamp (pTimeStepVal + macroEvolution * 0.4f
                                         + macroDrift * 0.2f, 0.0f, 1.0f);
        float effectiveAmpStep  = clamp (pAmpStepVal + macroEvolution * 0.3f
                                         + macroProphecy * 0.2f, 0.0f, 1.0f);
        float effectiveDist     = clamp (pDistVal + couplingDistributionMod
                                         + macroProphecy * 0.3f, 0.0f, 1.0f);
        float effectiveElastic  = clamp (pElasticVal + couplingBarrierMod, 0.0f, 1.0f);
        float effectiveGravity  = clamp (pGravityVal + macroGravity * 0.5f, 0.0f, 1.0f);
        float effectiveDrift    = clamp (pDriftVal + macroDrift * 0.3f, 0.0f, 1.0f);
        int   effectiveMaqam    = std::max (0, std::min (kNumMaqamat, pMaqamIdx));

        // Reset coupling accumulators for next block
        couplingBreakpointMod = 0.0f;
        couplingBarrierMod = 0.0f;
        couplingDistributionMod = 0.0f;

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(),
                        maxPoly, monoMode, legatoMode, glideCoeffCalc,
                        pBpCount, effectiveMaqam, effectiveGravity,
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
            // Smooth stochastic parameters toward their targets
            smoothedTimeStep     += (effectiveTimeStep - smoothedTimeStep) * smoothCoeff;
            smoothedAmpStep      += (effectiveAmpStep - smoothedAmpStep) * smoothCoeff;
            smoothedDistribution += (effectiveDist - smoothedDistribution) * smoothCoeff;

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
                voice.currentFreq += (voice.targetFreq - voice.currentFreq)
                                   * voice.glideCoeff;

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
                float modTimeStep = clamp (smoothedTimeStep + lfo1Val * 0.3f,
                                           0.0f, 1.0f);
                float modAmpStep  = clamp (smoothedAmpStep + lfo2Val * 0.3f,
                                           0.0f, 1.0f);

                // Scale stochastic depth by stoch envelope and drift
                float stochDepth = stochLevel * effectiveDrift;

                // --- Phase increment ---
                float freq = voice.currentFreq;
                voice.wavePhaseInc = freq / srf;

                // --- Advance waveform phase ---
                voice.wavePhase += voice.wavePhaseInc;

                // --- Check for cycle completion: evolve breakpoints ---
                if (voice.wavePhase >= 1.0f)
                {
                    voice.wavePhase -= 1.0f;

                    // Save last sample of previous cycle for crossfade
                    voice.prevCycleLastSample = voice.lastOutputL;
                    voice.cycleBlendCounter = OracleVoice::kCycleBlendSamples;

                    // Evolve breakpoints via stochastic random walk
                    evolveBreakpoints (voice,
                                       modTimeStep * stochDepth,
                                       modAmpStep * stochDepth,
                                       smoothedDistribution,
                                       effectiveElastic,
                                       couplingBreakpointMod);
                }

                // --- Cubic Hermite interpolation across breakpoints ---
                float rawSample = interpolateBreakpoints (voice, voice.wavePhase);

                // --- Cycle boundary crossfade (prevent clicks) ---
                if (voice.cycleBlendCounter > 0)
                {
                    float blendT = static_cast<float> (voice.cycleBlendCounter)
                                 / static_cast<float> (OracleVoice::kCycleBlendSamples);
                    rawSample = rawSample * (1.0f - blendT)
                              + voice.prevCycleLastSample * blendT;
                    --voice.cycleBlendCounter;
                }

                // --- DC blocker (L channel) ---
                float outL = voice.dcBlockerL.process (rawSample);

                // --- Stereo decorrelation: R channel uses slight phase offset ---
                float phaseR = voice.wavePhase + 0.01f;
                if (phaseR >= 1.0f) phaseR -= 1.0f;
                float rawSampleR = interpolateBreakpoints (voice, phaseR);
                if (voice.cycleBlendCounter > 0)
                {
                    float blendT = static_cast<float> (voice.cycleBlendCounter + 1)
                                 / static_cast<float> (OracleVoice::kCycleBlendSamples);
                    rawSampleR = rawSampleR * (1.0f - blendT)
                               + voice.prevCycleLastSample * blendT;
                }
                float outR = voice.dcBlockerR.process (rawSampleR);

                // --- Soft limiter (tanh saturation) ---
                outL = fastTanh (outL);
                outR = fastTanh (outR);

                // --- Apply amplitude envelope, velocity, and crossfade ---
                float gain = ampLevel * voice.velocity * voice.fadeGain;
                outL *= gain;
                outR *= gain;

                // Denormal protection on outputs
                outL = flushDenormal (outL);
                outR = flushDenormal (outR);

                voice.lastOutputL = outL;
                voice.lastOutputR = outR;

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

        // Update active voice count (safe for UI thread reads)
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
                couplingDistributionMod += amount * 0.4f;
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

        // --- Maqam Parameters ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "oracle_maqam", 1 }, "Oracle Maqam",
            juce::StringArray { "12-TET", "Rast", "Bayati", "Saba", "Hijaz",
                                "Sikah", "Nahawand", "Kurd", "Ajam" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_gravity", 1 }, "Oracle Gravity",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // --- Evolution ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_drift", 1 }, "Oracle Drift",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

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
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_stochEnvSustain", 1 }, "Oracle Stoch Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

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
            juce::ParameterID { "oracle_macroProphecy", 1 }, "Oracle Macro PROPHECY",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_macroEvolution", 1 }, "Oracle Macro EVOLUTION",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_macroGravity", 1 }, "Oracle Macro GRAVITY",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oracle_macroDrift", 1 }, "Oracle Macro DRIFT",
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

        pMacroProphecy      = apvts.getRawParameterValue ("oracle_macroProphecy");
        pMacroEvolution     = apvts.getRawParameterValue ("oracle_macroEvolution");
        pMacroGravity       = apvts.getRawParameterValue ("oracle_macroGravity");
        pMacroDrift         = apvts.getRawParameterValue ("oracle_macroDrift");
    }

    //==========================================================================
    // SynthEngine interface — Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Oracle"; }

    juce::Colour getAccentColour() const override
    {
        return juce::Colour (0xFF4B0082);  // Prophecy Indigo
    }

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
    // Maqam tuning — compute frequency for a MIDI note given maqam + gravity.
    //
    // When maqam is 0 (12-TET) or gravity is 0, returns standard 12-TET.
    // Otherwise, maps each chromatic note to its maqam-adjusted frequency
    // and blends between 12-TET and maqam based on gravity [0,1].
    //==========================================================================

    static float computeMaqamFreq (int midiNote, int maqamIdx, float gravity) noexcept
    {
        // 12-TET frequency
        float tetFreq = 440.0f * std::pow (2.0f,
            (static_cast<float> (midiNote) - 69.0f) / 12.0f);

        // If maqam is 0 (12-TET) or gravity is zero, use standard tuning
        if (maqamIdx <= 0 || maqamIdx > kNumMaqamat || gravity <= 0.0001f)
            return tetFreq;

        const auto& maq = kMaqamat[maqamIdx - 1];

        // Determine note position within octave
        // Use C as the maqam root (MIDI 60 = C4)
        int noteInOctave = ((midiNote % 12) + 12) % 12;  // 0-11
        int octave = (midiNote / 12) - 1;

        // Convert chromatic position to cents from root
        float chromaticCents = static_cast<float> (noteInOctave) * 100.0f;

        // Find the maqam cent offset by interpolating between degrees
        float maqamCentOffset = chromaticCents;  // default: same as 12-TET
        for (int d = 0; d < 7; ++d)
        {
            if (chromaticCents >= maq.cents[d] - 0.01f
                && chromaticCents <= maq.cents[d + 1] + 0.01f)
            {
                float range = maq.cents[d + 1] - maq.cents[d];
                float pos   = chromaticCents - maq.cents[d];
                if (range > 0.001f)
                {
                    float t = pos / range;
                    maqamCentOffset = maq.cents[d]
                                    + t * (maq.cents[d + 1] - maq.cents[d]);
                }
                else
                {
                    maqamCentOffset = maq.cents[d];
                }
                break;
            }
        }

        // Compute maqam frequency from the C root of the same octave
        int rootMidi = (octave + 1) * 12;  // C of this octave
        float rootFreq = 440.0f * std::pow (2.0f,
            (static_cast<float> (rootMidi) - 69.0f) / 12.0f);
        float maqamFreq = rootFreq * std::pow (2.0f, maqamCentOffset / 1200.0f);

        // Blend between 12-TET and maqam tuning via gravity
        return tetFreq + gravity * (maqamFreq - tetFreq);
    }

    //==========================================================================
    // Distribution sampling — morphable Cauchy/Logistic blend.
    //
    // distribution = 0.0 -> pure Cauchy (heavy-tailed, erratic)
    // distribution = 1.0 -> pure Logistic (smooth, predictable)
    //==========================================================================

    static float sampleDistribution (Xorshift64& rng, float distribution) noexcept
    {
        float u = rng.uniform();

        // Clamp to avoid singularities at 0 and 1
        u = std::max (0.001f, std::min (0.999f, u));

        // Cauchy: tan(pi * (u - 0.5)) — heavy-tailed distribution
        float cauchy = std::tan (kPI * (u - 0.5f));
        // Clamp and normalize to roughly [-1, 1]
        cauchy = std::max (-10.0f, std::min (10.0f, cauchy));
        cauchy *= 0.1f;

        // Logistic: log(u / (1 - u)) — smooth sigmoid-like tails
        float logistic = std::log (u / (1.0f - u));
        // Normalize to roughly [-1, 1]
        logistic *= 0.15f;

        // Morph between the two distributions
        return cauchy * (1.0f - distribution) + logistic * distribution;
    }

    //==========================================================================
    // Mirror barrier — elastic reflection at boundaries.
    //
    // When a value exceeds [lo, hi], it reflects back like a billiard ball.
    // Elasticity controls how much of the overshoot energy is preserved:
    //   0 = hard clamp (no reflection)
    //   1 = full elastic reflection
    //==========================================================================

    static float mirrorBarrier (float value, float lo, float hi,
                                float elasticity) noexcept
    {
        float range = hi - lo;
        if (range <= 0.0001f) return (lo + hi) * 0.5f;

        int iterations = 0;
        while ((value < lo || value > hi) && iterations < 8)
        {
            if (value < lo)
            {
                float overshoot = lo - value;
                value = lo + overshoot * elasticity;
            }
            else if (value > hi)
            {
                float overshoot = value - hi;
                value = hi - overshoot * elasticity;
            }
            ++iterations;
        }

        // Final hard clamp as safety net
        return std::max (lo, std::min (hi, value));
    }

    //==========================================================================
    // Evolve breakpoints — stochastic random walk, once per waveform cycle.
    //
    // Each breakpoint's time and amplitude are perturbed by samples from
    // the morphable Cauchy/Logistic distribution, then reflected via mirror
    // barriers. After perturbation, breakpoints are sorted by time and the
    // time span is normalized to [0, 1].
    //==========================================================================

    void evolveBreakpoints (OracleVoice& voice, float timeStep, float ampStep,
                            float distribution, float elasticity,
                            float bpMod) noexcept
    {
        int n = voice.numBreakpoints;

        // Quadratic scaling for gradual onset of chaos
        float tStep = timeStep * timeStep * 0.15f;
        float aStep = ampStep * ampStep * 0.3f;

        for (int i = 0; i < n; ++i)
        {
            auto& bp = voice.breakpoints[i];

            // Random walk on time offset
            float timeDelta = sampleDistribution (voice.rng, distribution) * tStep;
            bp.timeOffset += timeDelta;

            // Random walk on amplitude
            float ampDelta = sampleDistribution (voice.rng, distribution) * aStep;
            bp.amplitude += ampDelta;

            // Apply coupling modulation to amplitude
            bp.amplitude += bpMod * 0.05f;

            // Mirror barrier for time [0, 1]
            bp.timeOffset = mirrorBarrier (bp.timeOffset, 0.0f, 1.0f, elasticity);

            // Mirror barrier for amplitude [-1, 1]
            bp.amplitude = mirrorBarrier (bp.amplitude, -1.0f, 1.0f, elasticity);
        }

        // Sort breakpoints by time offset to maintain waveform ordering
        std::sort (voice.breakpoints, voice.breakpoints + n,
                   [] (const GENDYBreakpoint& a, const GENDYBreakpoint& b) {
                       return a.timeOffset < b.timeOffset;
                   });

        // Normalize time span: first breakpoint at 0, last at ~1
        if (n >= 2)
        {
            float tMin = voice.breakpoints[0].timeOffset;
            float tMax = voice.breakpoints[n - 1].timeOffset;
            float span = tMax - tMin;

            if (span > 0.001f)
            {
                for (int i = 0; i < n; ++i)
                {
                    voice.breakpoints[i].timeOffset =
                        (voice.breakpoints[i].timeOffset - tMin) / span;
                }
            }
            else
            {
                // Degenerate case: redistribute evenly
                for (int i = 0; i < n; ++i)
                {
                    voice.breakpoints[i].timeOffset =
                        static_cast<float> (i) / static_cast<float> (n - 1);
                }
            }
        }

        // Smoothing for high breakpoint counts (> 20): apply a small
        // low-pass average to neighboring amplitudes to reduce aliasing
        if (n > 20)
        {
            float prevAmp = voice.breakpoints[0].amplitude;
            for (int i = 1; i < n - 1; ++i)
            {
                float cur  = voice.breakpoints[i].amplitude;
                float next = voice.breakpoints[i + 1].amplitude;
                float smoothed = prevAmp * 0.25f + cur * 0.5f + next * 0.25f;
                prevAmp = cur;
                voice.breakpoints[i].amplitude = smoothed;
            }
        }
    }

    //==========================================================================
    // Cubic Hermite interpolation between breakpoints.
    //
    // Uses Catmull-Rom tangent estimation for smooth interpolation.
    // Runs per-sample — this is the inner-loop hot path.
    //==========================================================================

    float interpolateBreakpoints (const OracleVoice& voice,
                                  float phase) const noexcept
    {
        int n = voice.numBreakpoints;
        if (n < 2) return 0.0f;

        // Clamp phase to [0, 1)
        phase = std::max (0.0f, std::min (0.99999f, phase));

        // Find the segment: breakpoint[i] <= phase < breakpoint[i+1]
        int segIdx = n - 2;  // default to last segment
        for (int i = 0; i < n - 1; ++i)
        {
            if (phase < voice.breakpoints[i + 1].timeOffset)
            {
                segIdx = i;
                break;
            }
        }

        // Get 4 control points for cubic Hermite (Catmull-Rom)
        int i0 = std::max (0, segIdx - 1);
        int i1 = segIdx;
        int i2 = std::min (n - 1, segIdx + 1);
        int i3 = std::min (n - 1, segIdx + 2);

        float t1 = voice.breakpoints[i1].timeOffset;
        float t2 = voice.breakpoints[i2].timeOffset;

        float a0 = voice.breakpoints[i0].amplitude;
        float a1 = voice.breakpoints[i1].amplitude;
        float a2 = voice.breakpoints[i2].amplitude;
        float a3 = voice.breakpoints[i3].amplitude;

        // Local parameter t within segment [t1, t2]
        float segLen = t2 - t1;
        float t = (segLen > 0.00001f) ? (phase - t1) / segLen : 0.0f;
        t = std::max (0.0f, std::min (1.0f, t));

        // Catmull-Rom tangent estimation
        float t0v = voice.breakpoints[i0].timeOffset;
        float t3v = voice.breakpoints[i3].timeOffset;

        float m1 = 0.0f;
        float m2 = 0.0f;

        // Tangent at p1
        {
            float dt = t2 - t0v;
            if (dt > 0.00001f)
                m1 = (a2 - a0) / dt * segLen;
        }

        // Tangent at p2
        {
            float dt = t3v - t1;
            if (dt > 0.00001f)
                m2 = (a3 - a1) / dt * segLen;
        }

        // Hermite basis functions
        float t2_ = t * t;
        float t3_ = t2_ * t;

        float h00 =  2.0f * t3_ - 3.0f * t2_ + 1.0f;
        float h10 =         t3_ - 2.0f * t2_ + t;
        float h01 = -2.0f * t3_ + 3.0f * t2_;
        float h11 =         t3_ -        t2_;

        float result = h00 * a1 + h10 * m1 + h01 * a2 + h11 * m2;

        // Clamp output to prevent wild overshoots from interpolation
        return std::max (-1.5f, std::min (1.5f, result));
    }

    //==========================================================================
    // MIDI note handling.
    //==========================================================================

    void noteOn (int noteNumber, float velocity, int maxPoly,
                 bool monoMode, bool legatoMode, float glideCoeffVal,
                 int bpCount, int maqamIdx, float gravity,
                 float ampA, float ampD, float ampS, float ampR,
                 float stochA, float stochD, float stochS, float stochR,
                 float lfo1Rate, float lfo1Depth, int lfo1Shape,
                 float lfo2Rate, float lfo2Depth, int lfo2Shape)
    {
        float freq = computeMaqamFreq (noteNumber, maqamIdx, gravity);

        if (monoMode)
        {
            auto& voice = voices[0];
            bool wasActive = voice.active;

            voice.targetFreq = freq;

            if (legatoMode && wasActive)
            {
                // Legato: glide to new note without retriggering envelopes
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
                voice.wavePhase = 0.0f;
                voice.fadingOut = false;
                voice.fadeGain = 1.0f;
                voice.cycleBlendCounter = 0;

                // Seed PRNG from note number for deterministic variation
                voice.rng.seed (static_cast<uint64_t> (noteNumber)
                              * 2654435761ULL + 1);

                voice.initBreakpoints (bpCount);

                voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
                voice.ampEnv.noteOn();
                voice.stochEnv.setParams (stochA, stochD, stochS, stochR, srf);
                voice.stochEnv.noteOn();

                voice.lfo1.setRate (lfo1Rate, srf);
                voice.lfo1.setShape (lfo1Shape);
                voice.lfo2.setRate (lfo2Rate, srf);
                voice.lfo2.setShape (lfo2Shape);

                voice.dcBlockerL.prepare (srf);
                voice.dcBlockerL.reset();
                voice.dcBlockerR.prepare (srf);
                voice.dcBlockerR.reset();
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
        voice.glideCoeff = 1.0f;  // No glide in poly mode
        voice.wavePhase = 0.0f;
        voice.fadingOut = false;
        voice.fadeGain = 1.0f;
        voice.cycleBlendCounter = 0;

        // Seed PRNG from note number + voice counter for unique variation
        voice.rng.seed (static_cast<uint64_t> (noteNumber)
                      * 2654435761ULL + voiceCounter);

        voice.initBreakpoints (bpCount);

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

        voice.dcBlockerL.prepare (srf);
        voice.dcBlockerL.reset();
        voice.dcBlockerR.prepare (srf);
        voice.dcBlockerR.reset();
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber
                && !voice.fadingOut)
            {
                voice.ampEnv.noteOff();
                voice.stochEnv.noteOff();
            }
        }
    }

    int findFreeVoice (int maxPoly) const
    {
        int poly = std::min (maxPoly, kMaxVoices);

        // Find inactive voice within polyphony limit
        for (int i = 0; i < poly; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        // LRU voice stealing — find oldest active voice
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
    float couplingBreakpointMod = 0.0f;   // AudioToFM -> breakpoint perturbation
    float couplingBarrierMod = 0.0f;       // AmpToFilter -> barrier modulation
    float couplingDistributionMod = 0.0f;  // EnvToMorph -> distribution morph

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS parameter pointers (ParamSnapshot pattern)
    std::atomic<float>* pBreakpoints       = nullptr;
    std::atomic<float>* pTimeStep          = nullptr;
    std::atomic<float>* pAmpStep           = nullptr;
    std::atomic<float>* pDistribution      = nullptr;
    std::atomic<float>* pBarrierElasticity = nullptr;
    std::atomic<float>* pMaqam             = nullptr;
    std::atomic<float>* pGravity           = nullptr;
    std::atomic<float>* pDrift             = nullptr;
    std::atomic<float>* pLevel             = nullptr;

    std::atomic<float>* pAmpAttack         = nullptr;
    std::atomic<float>* pAmpDecay          = nullptr;
    std::atomic<float>* pAmpSustain        = nullptr;
    std::atomic<float>* pAmpRelease        = nullptr;

    std::atomic<float>* pStochEnvAttack    = nullptr;
    std::atomic<float>* pStochEnvDecay     = nullptr;
    std::atomic<float>* pStochEnvSustain   = nullptr;
    std::atomic<float>* pStochEnvRelease   = nullptr;

    std::atomic<float>* pLfo1Rate          = nullptr;
    std::atomic<float>* pLfo1Depth         = nullptr;
    std::atomic<float>* pLfo1Shape         = nullptr;
    std::atomic<float>* pLfo2Rate          = nullptr;
    std::atomic<float>* pLfo2Depth         = nullptr;
    std::atomic<float>* pLfo2Shape         = nullptr;

    std::atomic<float>* pVoiceMode         = nullptr;
    std::atomic<float>* pGlide             = nullptr;

    std::atomic<float>* pMacroProphecy     = nullptr;
    std::atomic<float>* pMacroEvolution    = nullptr;
    std::atomic<float>* pMacroGravity      = nullptr;
    std::atomic<float>* pMacroDrift        = nullptr;
};

} // namespace xomnibus
