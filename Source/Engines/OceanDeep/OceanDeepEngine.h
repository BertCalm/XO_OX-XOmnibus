#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/Effects/LushReverb.h"
#include "../../DSP/Effects/Compressor.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace xomnibus {

//==============================================================================
// OceanDeepEngine — Abyssal Bass Synthesis.
//
// Pure Oscar polarity. The deepest, darkest engine in the XO_OX water column.
// Maps the physics and creatures of the hadal zone (6000-11000m) into a bass
// synthesis architecture optimized for crushing sub-frequency weight.
//
// SUBSYSTEMS:
//
//   1. SUB OSCILLATOR STACK — Three-oscillator sub engine (sine + triangle +
//      square, selectable per-osc) with a sub-harmonic generator that adds
//      octave-below content. Phase-locked for maximum bottom-end coherence.
//      The fundamental building block of abyssal pressure.
//
//   2. HYDROSTATIC COMPRESSOR — Pressure-dependent dynamics processor.
//      As PRESSURE macro increases, compression ratio and makeup gain
//      increase logarithmically, simulating the crushing weight of water
//      at depth. At full depth: brickwall limiting with extreme makeup.
//      The sound gets heavier, denser, more compressed the deeper you go.
//
//   3. WAVEGUIDE BODY RESONANCE — Three selectable body types (Shipwreck,
//      Cave, Trench) each modeled as a delay line + allpass network with
//      characteristic reflection patterns. Shipwreck: metallic, short.
//      Cave: round, medium. Trench: massive, dark, infinite-feeling.
//      WRECK macro controls resonance intensity and body selection blend.
//
//   4. BIOLUMINESCENT EXCITER — Random light-like transients: subtle
//      noise bursts with a pitch envelope that sweeps downward, simulating
//      the flash patterns of deep-sea creatures. Adds organic transient
//      interest to sustained bass tones. Probability-based triggering.
//
//   5. CREATURE MODULATION — Slow, organic LFO system with random drift.
//      Two independent creature LFOs with rates below 0.01 Hz (D005).
//      Random walk modulation simulates bioluminescent creatures drifting
//      through the darkness. CREATURE macro controls depth and randomness.
//
//   6. DARKNESS FILTER — Multi-mode SVF that gets progressively darker
//      with increasing PRESSURE. At zero depth: open, bright. At full
//      depth: heavily filtered, only the deepest frequencies survive.
//      The ocean eats light; this filter eats harmonics.
//
// 4 MACROS:
//   PRESSURE — depth/weight (drives compressor, filter darkness, sub level)
//   CREATURE — alien life modulation (LFO depth, exciter probability)
//   WRECK    — environment resonance (body type blend, resonance intensity)
//   ABYSS    — vastness/reverb (reverb size, pre-delay, darkness)
//
// Coupling:
//   - Output: Post-process stereo audio (ch 0,1), envelope level (ch 2)
//   - Input: AmpToFilter (modulate darkness cutoff), LFOToPitch (creature
//            pitch warping), AudioToFM (FM input to sub osc), EnvToDecay
//            (modulate body resonance decay)
//   - Key coupling: OCEANDEEP x OPENSKY = "The Full Column"
//
// Accent Color: Trench Violet #2D0A4E
//
//==============================================================================

//==============================================================================
// ADSR envelope generator.
//==============================================================================
struct DeepADSR
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
            case Stage::Idle:    return 0.0f;
            case Stage::Attack:
                level += attackRate;
                if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
                return level;
            case Stage::Decay:
                level -= decayRate * (level - sustainLevel + 0.0001f);
                if (level <= sustainLevel + 0.0001f) { level = sustainLevel; stage = Stage::Sustain; }
                return level;
            case Stage::Sustain:
                return level;
            case Stage::Release:
                level -= releaseRate * (level + 0.0001f);
                if (level <= 0.0001f) { level = 0.0f; stage = Stage::Idle; }
                return level;
        }
        return 0.0f;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }

    void reset() noexcept { stage = Stage::Idle; level = 0.0f; }
};

//==============================================================================
// LFO with multiple shapes — used by creature modulation and general mod.
//==============================================================================
struct DeepLFO
{
    enum class Shape { Sine, Triangle, Saw, Square, SandH };

    float phase = 0.0f;
    float phaseInc = 0.0f;
    Shape shape = Shape::Sine;
    float holdValue = 0.0f;
    uint32_t rngState = 12345u;

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
            case Shape::Sine:     out = fastSin (phase * 6.28318530718f); break;
            case Shape::Triangle: out = 4.0f * std::fabs (phase - 0.5f) - 1.0f; break;
            case Shape::Saw:      out = 2.0f * phase - 1.0f; break;
            case Shape::Square:   out = (phase < 0.5f) ? 1.0f : -1.0f; break;
            case Shape::SandH:
            {
                float prevPhase = phase - phaseInc;
                if (prevPhase < 0.0f || phase < prevPhase)
                {
                    rngState = rngState * 1664525u + 1013904223u;
                    holdValue = static_cast<float> (rngState & 0xFFFF) / 32768.0f - 1.0f;
                }
                out = holdValue;
                break;
            }
        }
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;
        return out;
    }

    void reset() noexcept { phase = 0.0f; holdValue = 0.0f; rngState = 12345u; }
};

//==============================================================================
// Waveguide body resonance — delay + allpass + reflection network.
// Three body types: Shipwreck (metallic), Cave (round), Trench (massive).
//==============================================================================
struct DeepWaveguideBody
{
    static constexpr int kMaxDelay = 8192;
    static constexpr int kNumAllpass = 3;

    // Delay line
    float delayBuffer[kMaxDelay] {};
    int writePos = 0;
    float delaySamples = 200.0f;
    float feedback = 0.6f;
    float damping = 0.4f;
    float prevSample = 0.0f;

    // Allpass chain for diffusion
    float allpassBuffers[kNumAllpass][1024] {};
    int allpassPos[kNumAllpass] {};
    float allpassDelays[kNumAllpass] = { 113.0f, 199.0f, 347.0f };

    void setBodyType (float bodyBlend, float sampleRate) noexcept
    {
        // bodyBlend: 0=Shipwreck, 0.5=Cave, 1.0=Trench
        // Shipwreck: short delay, high damping, metallic resonance
        // Cave: medium delay, moderate damping, round resonance
        // Trench: long delay, low damping, massive dark resonance

        float srScale = sampleRate / 44100.0f;

        if (bodyBlend < 0.5f)
        {
            // Shipwreck -> Cave blend
            float t = bodyBlend * 2.0f;
            delaySamples = clamp ((80.0f + t * 200.0f) * srScale, 1.0f, static_cast<float> (kMaxDelay - 1));
            damping = 0.7f - t * 0.3f;
            allpassDelays[0] = (37.0f + t * 76.0f) * srScale;
            allpassDelays[1] = (67.0f + t * 132.0f) * srScale;
            allpassDelays[2] = (97.0f + t * 250.0f) * srScale;
        }
        else
        {
            // Cave -> Trench blend
            float t = (bodyBlend - 0.5f) * 2.0f;
            delaySamples = clamp ((280.0f + t * 500.0f) * srScale, 1.0f, static_cast<float> (kMaxDelay - 1));
            damping = 0.4f - t * 0.25f;
            allpassDelays[0] = (113.0f + t * 150.0f) * srScale;
            allpassDelays[1] = (199.0f + t * 250.0f) * srScale;
            allpassDelays[2] = (347.0f + t * 400.0f) * srScale;
        }

        // Clamp allpass delays
        for (int i = 0; i < kNumAllpass; ++i)
            allpassDelays[i] = clamp (allpassDelays[i], 1.0f, 1023.0f);
    }

    void setFeedback (float fb) noexcept
    {
        feedback = clamp (fb, 0.0f, 0.97f);
    }

    float processSample (float input) noexcept
    {
        // Read from delay line with linear interpolation
        float readPos = static_cast<float> (writePos) - delaySamples;
        if (readPos < 0.0f) readPos += static_cast<float> (kMaxDelay);

        int idx0 = static_cast<int> (readPos);
        int idx1 = (idx0 + 1) % kMaxDelay;
        float frac = readPos - static_cast<float> (idx0);
        idx0 = idx0 % kMaxDelay;

        float delayed = delayBuffer[idx0] + frac * (delayBuffer[idx1] - delayBuffer[idx0]);

        // 1-pole damping in feedback path
        float damped = delayed + damping * (prevSample - delayed);
        prevSample = flushDenormal (damped);

        // Write: input + feedback
        delayBuffer[writePos] = flushDenormal (input + damped * feedback);
        writePos = (writePos + 1) % kMaxDelay;

        // Process through allpass chain for diffusion
        float diffused = delayed;
        for (int i = 0; i < kNumAllpass; ++i)
        {
            int apDelay = static_cast<int> (allpassDelays[i]);
            apDelay = std::min (apDelay, 1023);
            int apReadPos = (allpassPos[i] - apDelay + 1024) % 1024;

            float apDelayed = allpassBuffers[i][apReadPos];
            float apInput = diffused + apDelayed * 0.5f;
            allpassBuffers[i][allpassPos[i]] = flushDenormal (apInput);
            allpassPos[i] = (allpassPos[i] + 1) % 1024;

            diffused = apDelayed - diffused * 0.5f;
        }

        return flushDenormal (diffused);
    }

    void reset() noexcept
    {
        std::memset (delayBuffer, 0, sizeof (delayBuffer));
        std::memset (allpassBuffers, 0, sizeof (allpassBuffers));
        writePos = 0;
        prevSample = 0.0f;
        for (int i = 0; i < kNumAllpass; ++i)
            allpassPos[i] = 0;
    }
};

//==============================================================================
// Bioluminescent exciter — random transient noise bursts with pitch envelope.
//==============================================================================
struct DeepBioExciter
{
    float phase = 0.0f;
    float freq = 2000.0f;
    float freqDecay = 0.999f;
    float envLevel = 0.0f;
    float envDecay = 0.0f;
    bool active = false;
    uint32_t rng = 67890u;

    void trigger (float startFreq, float decayTime, float sampleRate) noexcept
    {
        freq = startFreq;
        freqDecay = 1.0f - (1.0f / (decayTime * std::max (1.0f, sampleRate) + 1.0f));
        envLevel = 1.0f;
        envDecay = 1.0f - (1.0f / (decayTime * 0.5f * std::max (1.0f, sampleRate) + 1.0f));
        phase = 0.0f;
        active = true;
    }

    float process (float sampleRate) noexcept
    {
        if (!active) return 0.0f;

        // Noise burst modulated by decaying pitch envelope
        rng = rng * 1664525u + 1013904223u;
        float noise = static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f;

        // Tonal component: swept sine
        float tonal = fastSin (phase * 6.28318530718f);
        phase += freq / std::max (1.0f, sampleRate);
        if (phase >= 1.0f) phase -= 1.0f;

        // Mix noise and tonal — more tonal at start, more noise at end
        float mix = tonal * envLevel + noise * (1.0f - envLevel) * 0.5f;

        // Apply amplitude envelope
        float output = mix * envLevel;

        // Decay pitch downward (bioluminescent flash fades down)
        freq *= freqDecay;
        freq = std::max (20.0f, freq);

        // Decay amplitude
        envLevel *= envDecay;
        envLevel = flushDenormal (envLevel);

        if (envLevel < 0.0001f)
        {
            active = false;
            envLevel = 0.0f;
        }

        return output;
    }

    void reset() noexcept
    {
        phase = 0.0f;
        envLevel = 0.0f;
        active = false;
    }
};

//==============================================================================
// Creature modulation — slow organic LFO with random drift.
// D005 compliant: rate floor <= 0.01 Hz.
//==============================================================================
struct DeepCreatureLFO
{
    float phase = 0.0f;
    float phaseInc = 0.0f;
    float driftPhase = 0.0f;
    float driftRate = 0.003f;
    float driftAmount = 0.0f;
    uint32_t rng = 11111u;
    float smoothedOutput = 0.0f;
    float smoothCoeff = 0.999f;

    void setRate (float hz, float sampleRate) noexcept
    {
        // D005: floor at 0.005 Hz — ultra-slow breathing
        float clampedHz = std::max (0.005f, hz);
        phaseInc = clampedHz / std::max (1.0f, sampleRate);
        smoothCoeff = 1.0f - (1.0f / (0.05f * std::max (1.0f, sampleRate)));
    }

    void setDrift (float amount) noexcept
    {
        driftAmount = clamp (amount, 0.0f, 1.0f);
    }

    float process() noexcept
    {
        // Base sine LFO
        float base = fastSin (phase * 6.28318530718f);

        // Random drift: slow random walk modulation
        driftPhase += driftRate * phaseInc * 3.7f;
        if (driftPhase >= 1.0f)
        {
            driftPhase -= 1.0f;
            rng = rng * 1664525u + 1013904223u;
        }
        float drift = fastSin (driftPhase * 6.28318530718f);

        // Secondary random perturbation
        rng = rng * 1664525u + 1013904223u;
        float jitter = static_cast<float> (rng & 0xFFFF) / 65536.0f - 0.5f;

        // Combine: base + organic drift
        float raw = base + driftAmount * (drift * 0.5f + jitter * 0.1f);

        // Smooth to prevent clicks
        smoothedOutput += (raw - smoothedOutput) * (1.0f - smoothCoeff);
        smoothedOutput = flushDenormal (smoothedOutput);

        // Advance phase
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;

        return clamp (smoothedOutput, -1.0f, 1.0f);
    }

    void reset() noexcept
    {
        phase = 0.0f;
        driftPhase = 0.0f;
        smoothedOutput = 0.0f;
    }
};

//==============================================================================
// DeepVoice — per-voice state.
//==============================================================================
struct DeepVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // MPE per-voice expression state
    MPEVoiceExpression mpeExpression;

    // Glide
    float currentFreq = 440.0f;
    float targetFreq = 440.0f;
    float glideCoeff = 1.0f;

    // Sub oscillator phases (3 oscillators + 1 sub-harmonic)
    float oscPhase[3] = { 0.0f, 0.0f, 0.0f };
    float subPhase = 0.0f;  // sub-harmonic generator (octave below)

    // Envelopes
    DeepADSR ampEnv;
    DeepADSR modEnv;
    DeepADSR filterEnv;

    // LFOs
    DeepLFO lfo1;
    DeepLFO lfo2;

    // Creature modulation (per-voice for organic independence)
    DeepCreatureLFO creature1;
    DeepCreatureLFO creature2;

    // Bioluminescent exciter
    DeepBioExciter exciter;
    uint32_t exciterRng = 0u;

    // Waveguide body resonance (stereo pair)
    DeepWaveguideBody bodyL;
    DeepWaveguideBody bodyR;

    // Darkness filter (multi-mode SVF)
    CytomicSVF darknessFilter;

    // Filter envelope filter (velocity-scaled, D001)
    CytomicSVF filterEnvSVF;

    // Voice stealing crossfade
    float fadeGain = 1.0f;
    bool fadingOut = false;

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        currentFreq = 440.0f;
        targetFreq = 440.0f;
        fadeGain = 1.0f;
        fadingOut = false;
        for (int i = 0; i < 3; ++i) oscPhase[i] = 0.0f;
        subPhase = 0.0f;
        ampEnv.reset();
        modEnv.reset();
        filterEnv.reset();
        lfo1.reset();
        lfo2.reset();
        creature1.reset();
        creature2.reset();
        exciter.reset();
        bodyL.reset();
        bodyR.reset();
        darknessFilter.reset();
        filterEnvSVF.reset();
    }
};

//==============================================================================
// OceanDeepEngine — the main engine class.
//==============================================================================
class OceanDeepEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;
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

        // Initialize voices
        for (int v = 0; v < kMaxVoices; ++v)
        {
            auto& voice = voices[v];
            voice.reset();
            voice.exciterRng = static_cast<uint32_t> (v * 999 + 24601);

            voice.darknessFilter.reset();
            voice.darknessFilter.setMode (CytomicSVF::Mode::LowPass);

            voice.filterEnvSVF.reset();
            voice.filterEnvSVF.setMode (CytomicSVF::Mode::LowPass);
        }

        // Initialize hydrostatic compressor
        hydroComp.prepare (sr);

        // Initialize abyss reverb
        abyssReverb.prepare (sr);
        abyssReverb.setRoomSize (0.9f);
        abyssReverb.setDamping (0.6f);
        abyssReverb.setWidth (1.0f);
        abyssReverb.setMix (0.0f);
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
        couplingDecayMod = 0.0f;

        smoothedDarkness = 8000.0f;
        smoothedPressure = 0.0f;
        smoothedCreature = 0.0f;
        smoothedWreck = 0.0f;
        smoothedAbyss = 0.0f;

        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //==========================================================================
    // Audio
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // --- ParamSnapshot: read all parameters once per block ---

        // Sub oscillator stack
        const int   pOsc1Wave     = static_cast<int> (loadParam (paramOsc1Wave, 0.0f));
        const int   pOsc2Wave     = static_cast<int> (loadParam (paramOsc2Wave, 1.0f));
        const int   pOsc3Wave     = static_cast<int> (loadParam (paramOsc3Wave, 2.0f));
        const float pOsc1Level    = loadParam (paramOsc1Level, 1.0f);
        const float pOsc2Level    = loadParam (paramOsc2Level, 0.0f);
        const float pOsc3Level    = loadParam (paramOsc3Level, 0.0f);
        const float pOsc2Detune   = loadParam (paramOsc2Detune, 0.0f);
        const float pOsc3Detune   = loadParam (paramOsc3Detune, 0.0f);
        const float pSubLevel     = loadParam (paramSubLevel, 0.5f);
        const float pSubOctave    = loadParam (paramSubOctave, -1.0f);
        const float pOscFM        = loadParam (paramOscFM, 0.0f);
        const float pGlide        = loadParam (paramGlide, 0.1f);

        // Hydrostatic compressor
        const float pCompThresh   = loadParam (paramCompThresh, -12.0f);
        const float pCompRatio    = loadParam (paramCompRatio, 4.0f);
        const float pCompAttack   = loadParam (paramCompAttackMs, 5.0f);
        const float pCompRelease  = loadParam (paramCompReleaseMs, 80.0f);

        // Body resonance
        const float pBodyType     = loadParam (paramBodyType, 0.5f);
        const float pBodyFeedback = loadParam (paramBodyFeedback, 0.5f);
        const float pBodyMix      = loadParam (paramBodyMix, 0.0f);

        // Bioluminescent exciter
        const float pExciterProb  = loadParam (paramExciterProb, 0.1f);
        const float pExciterFreq  = loadParam (paramExciterFreq, 3000.0f);
        const float pExciterDecay = loadParam (paramExciterDecay, 0.1f);
        const float pExciterMix   = loadParam (paramExciterMix, 0.0f);

        // Creature modulation
        const float pCreature1Rate  = loadParam (paramCreature1Rate, 0.05f);
        const float pCreature1Drift = loadParam (paramCreature1Drift, 0.3f);
        const float pCreature2Rate  = loadParam (paramCreature2Rate, 0.02f);
        const float pCreature2Drift = loadParam (paramCreature2Drift, 0.5f);
        const float pCreatureToPitch = loadParam (paramCreatureToPitch, 0.0f);
        const float pCreatureToFilter = loadParam (paramCreatureToFilter, 0.0f);

        // Darkness filter
        const float pDarkCutoff   = loadParam (paramDarkCutoff, 8000.0f);
        const float pDarkReso     = loadParam (paramDarkReso, 0.0f);
        const int   pDarkMode     = static_cast<int> (loadParam (paramDarkMode, 0.0f));

        // Filter envelope
        const float pFiltEnvAmt   = loadParam (paramFilterEnvAmt, 0.0f);
        const float pFiltEnvA     = loadParam (paramFilterEnvAttack, 0.01f);
        const float pFiltEnvD     = loadParam (paramFilterEnvDecay, 0.3f);
        const float pFiltEnvS     = loadParam (paramFilterEnvSustain, 0.0f);
        const float pFiltEnvR     = loadParam (paramFilterEnvRelease, 0.3f);

        // Amp envelope
        const float pAmpA         = loadParam (paramAmpAttack, 0.01f);
        const float pAmpD         = loadParam (paramAmpDecay, 0.3f);
        const float pAmpS         = loadParam (paramAmpSustain, 0.9f);
        const float pAmpR         = loadParam (paramAmpRelease, 0.5f);

        // Mod envelope
        const float pModA         = loadParam (paramModAttack, 0.01f);
        const float pModD         = loadParam (paramModDecay, 0.5f);
        const float pModS         = loadParam (paramModSustain, 0.5f);
        const float pModR         = loadParam (paramModRelease, 0.5f);

        // LFOs
        const float pLfo1Rate     = loadParam (paramLfo1Rate, 0.5f);
        const float pLfo1Depth    = loadParam (paramLfo1Depth, 0.0f);
        const int   pLfo1Shape    = static_cast<int> (loadParam (paramLfo1Shape, 0.0f));
        const float pLfo2Rate     = loadParam (paramLfo2Rate, 0.1f);
        const float pLfo2Depth    = loadParam (paramLfo2Depth, 0.0f);
        const int   pLfo2Shape    = static_cast<int> (loadParam (paramLfo2Shape, 0.0f));

        // Abyss reverb
        const float pAbyssSize    = loadParam (paramAbyssSize, 0.8f);
        const float pAbyssDamp    = loadParam (paramAbyssDamp, 0.5f);
        const float pAbyssMix     = loadParam (paramAbyssMix, 0.0f);
        const float pAbyssPreDelay = loadParam (paramAbyssPreDelay, 0.0f);

        // Voice mode
        const int   voiceModeIdx  = static_cast<int> (loadParam (paramVoiceMode, 2.0f));

        // Level
        const float pLevel        = loadParam (paramLevel, 0.8f);

        // Macros — the 4 abyssal controls
        const float macroPressure = loadParam (paramMacroPressure, 0.0f);
        const float macroCreature = loadParam (paramMacroCreature, 0.0f);
        const float macroWreck    = loadParam (paramMacroWreck, 0.0f);
        const float macroAbyss    = loadParam (paramMacroAbyss, 0.0f);

        // Voice mode resolution
        int maxPoly = kMaxVoices;
        bool monoMode = false;
        bool legatoMode = false;
        switch (voiceModeIdx)
        {
            case 0: maxPoly = 1; monoMode = true; break;
            case 1: maxPoly = 1; monoMode = true; legatoMode = true; break;
            case 2: maxPoly = 4; break;
            case 3: maxPoly = 8; break;
            default: maxPoly = 4; break;
        }

        // Glide coefficient
        float glideCoeff = 1.0f;
        if (pGlide > 0.001f)
            glideCoeff = 1.0f - std::exp (-1.0f / (pGlide * srf));

        // === MACRO MODULATION ===
        //
        // PRESSURE: depth/weight — drives compressor harder, darkens filter,
        //           boosts sub-harmonic level, increases body resonance density
        // CREATURE: alien life — increases creature LFO depths, exciter probability
        // WRECK:    environment — body type blend toward Trench, body mix up
        // ABYSS:    vastness — reverb size/mix, pre-delay, filter darkens

        float effectivePressure = clamp (macroPressure + aftertouch_ * 0.3f, 0.0f, 1.0f);

        // Hydrostatic compressor: pressure drives ratio and makeup
        float hydroRatio  = clamp (pCompRatio + effectivePressure * 12.0f, 1.0f, 20.0f);
        float hydroThresh = clamp (pCompThresh - effectivePressure * 12.0f, -60.0f, 0.0f);

        // Darkness filter: pressure closes the filter
        float darkCutoff = clamp (pDarkCutoff - effectivePressure * 6000.0f
                                  + couplingFilterMod * 4000.0f, 20.0f, 20000.0f);
        float darkReso = clamp (pDarkReso + effectivePressure * 0.15f, 0.0f, 1.0f);

        // Sub-harmonic boost from pressure
        float effectiveSubLevel = clamp (pSubLevel + effectivePressure * 0.4f, 0.0f, 1.0f);

        // Creature macro: LFO depth and exciter probability
        float creatureScale = clamp (macroCreature + modWheelAmount_ * 0.4f, 0.0f, 1.0f);
        float effectiveCreaturePitch = clamp (pCreatureToPitch + creatureScale * 0.5f, 0.0f, 1.0f);
        float effectiveCreatureFilter = clamp (pCreatureToFilter + creatureScale * 0.5f, 0.0f, 1.0f);
        float effectiveExciterProb = clamp (pExciterProb + creatureScale * 0.3f, 0.0f, 1.0f);

        // Wreck macro: body type toward Trench, body mix up
        float effectiveBodyType = clamp (pBodyType + macroWreck * 0.5f, 0.0f, 1.0f);
        float effectiveBodyMix  = clamp (pBodyMix + macroWreck * 0.5f, 0.0f, 1.0f);
        float effectiveBodyFB   = clamp (pBodyFeedback + macroWreck * 0.2f + couplingDecayMod * 0.2f, 0.0f, 0.97f);

        // Abyss macro: reverb size and mix, also darkens filter further
        float effectiveAbyssSize = clamp (pAbyssSize + macroAbyss * 0.2f, 0.0f, 1.0f);
        float effectiveAbyssMix  = clamp (pAbyssMix + macroAbyss * 0.5f, 0.0f, 1.0f);
        float effectiveAbyssPreDelay = clamp (pAbyssPreDelay + macroAbyss * 0.05f, 0.0f, 0.1f);
        darkCutoff = clamp (darkCutoff - macroAbyss * 2000.0f, 20.0f, 20000.0f);

        // FM amount (from oscillator + coupling)
        float effectiveFM = clamp (pOscFM + couplingFMMod * 2.0f, 0.0f, 1.0f);

        // Filter envelope amount — velocity scaling (D001)
        float effectiveFiltEnvAmt = pFiltEnvAmt;

        // Set darkness filter mode
        CytomicSVF::Mode darkFilterMode = CytomicSVF::Mode::LowPass;
        switch (pDarkMode)
        {
            case 0: darkFilterMode = CytomicSVF::Mode::LowPass;  break;
            case 1: darkFilterMode = CytomicSVF::Mode::BandPass; break;
            case 2: darkFilterMode = CytomicSVF::Mode::HighPass; break;
            case 3: darkFilterMode = CytomicSVF::Mode::Notch;    break;
            default: break;
        }

        // Update abyss reverb parameters
        abyssReverb.setRoomSize (effectiveAbyssSize);
        abyssReverb.setDamping (clamp (pAbyssDamp + effectivePressure * 0.3f, 0.0f, 1.0f));
        abyssReverb.setMix (effectiveAbyssMix);
        abyssReverb.setPreDelay (effectiveAbyssPreDelay * 1000.0f);  // param is seconds, API expects ms

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), msg.getChannel(),
                        maxPoly, monoMode, legatoMode, glideCoeff,
                        pAmpA, pAmpD, pAmpS, pAmpR,
                        pModA, pModD, pModS, pModR,
                        pFiltEnvA, pFiltEnvD, pFiltEnvS, pFiltEnvR,
                        pLfo1Rate, pLfo1Shape, pLfo2Rate, pLfo2Shape,
                        pCreature1Rate, pCreature1Drift, pCreature2Rate, pCreature2Drift,
                        darkCutoff, darkReso, darkFilterMode);
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber(), msg.getChannel());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
            else if (msg.isController())
            {
                if (msg.getControllerNumber() == 1)
                    modWheelAmount_ = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isAftertouch() || msg.isChannelPressure())
            {
                aftertouch_ = msg.isChannelPressure()
                    ? msg.getChannelPressureValue() / 127.0f
                    : msg.getAfterTouchValue() / 127.0f;
            }
        }

        // --- Update per-voice MPE expression from MPEManager ---
        if (mpeManager != nullptr)
        {
            for (auto& voice : voices)
            {
                if (!voice.active) continue;
                mpeManager->updateVoiceExpression (voice.mpeExpression);
            }
        }

        // --- Update per-voice parameters once per block ---
        for (auto& voice : voices)
        {
            if (!voice.active) continue;

            // Darkness filter coefficients
            voice.darknessFilter.setMode (darkFilterMode);
            voice.darknessFilter.setCoefficients (darkCutoff, darkReso, srf);

            // Body resonance settings
            voice.bodyL.setBodyType (effectiveBodyType, srf);
            voice.bodyR.setBodyType (effectiveBodyType, srf);
            voice.bodyL.setFeedback (effectiveBodyFB);
            voice.bodyR.setFeedback (effectiveBodyFB);
            // Slight stereo offset for body R
            voice.bodyR.delaySamples = voice.bodyL.delaySamples * 1.007f;

            // Creature LFO rates
            voice.creature1.setRate (pCreature1Rate, srf);
            voice.creature1.setDrift (pCreature1Drift + creatureScale * 0.3f);
            voice.creature2.setRate (pCreature2Rate, srf);
            voice.creature2.setDrift (pCreature2Drift + creatureScale * 0.3f);
        }

        // Hydrostatic compressor setup (per-block, no allocations)
        hydroComp.setThreshold (hydroThresh);
        hydroComp.setRatio (hydroRatio);
        hydroComp.setAttack (pCompAttack);
        hydroComp.setRelease (pCompRelease);

        float peakEnv = 0.0f;

        // --- Render sample loop ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Smooth macro values
            smoothedPressure += (effectivePressure - smoothedPressure) * smoothCoeff;
            smoothedCreature += (creatureScale - smoothedCreature) * smoothCoeff;
            smoothedWreck    += (effectiveBodyMix - smoothedWreck) * smoothCoeff;
            smoothedAbyss    += (effectiveAbyssMix - smoothedAbyss) * smoothCoeff;
            smoothedDarkness += (darkCutoff - smoothedDarkness) * smoothCoeff;

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // --- Voice-stealing crossfade ---
                if (voice.fadingOut)
                {
                    voice.fadeGain -= crossfadeRate;
                    voice.fadeGain = flushDenormal (voice.fadeGain);
                    if (voice.fadeGain <= 0.0f)
                    {
                        voice.fadeGain = 0.0f;
                        voice.active = false;
                        continue;
                    }
                }

                // --- Glide ---
                voice.currentFreq += (voice.targetFreq - voice.currentFreq) * voice.glideCoeff;
                voice.currentFreq = flushDenormal (voice.currentFreq);

                // --- Envelopes ---
                float ampLevel = voice.ampEnv.process();
                float modLevel = voice.modEnv.process();
                float filtEnvLevel = voice.filterEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // --- LFO modulation ---
                float lfo1Val = voice.lfo1.process() * pLfo1Depth;
                float lfo2Val = voice.lfo2.process() * pLfo2Depth;

                // --- Creature modulation ---
                float creature1Val = voice.creature1.process();
                float creature2Val = voice.creature2.process();

                // Creature pitch modulation
                float creaturePitchMod = creature1Val * effectiveCreaturePitch * 0.5f;

                // Creature filter modulation
                float creatureFilterMod = creature2Val * effectiveCreatureFilter;

                // =====================================================
                // 1. SUB OSCILLATOR STACK
                // =====================================================

                // MPE pitch bend
                float mpeFreq = voice.currentFreq
                    * std::pow (2.0f, voice.mpeExpression.pitchBendSemitones / 12.0f);

                // Apply creature pitch modulation and coupling pitch mod
                float modFreq = mpeFreq * std::pow (2.0f,
                    (creaturePitchMod + couplingPitchMod * 0.5f + lfo1Val * 0.1f) / 12.0f);

                // Oscillator 1 (primary)
                float osc1 = generateOsc (voice.oscPhase[0], modFreq, pOsc1Wave, srf);

                // Oscillator 2 (detuned)
                float osc2Freq = modFreq * std::pow (2.0f, pOsc2Detune / 1200.0f);
                float osc2 = generateOsc (voice.oscPhase[1], osc2Freq, pOsc2Wave, srf);

                // Oscillator 3 (detuned opposite)
                float osc3Freq = modFreq * std::pow (2.0f, pOsc3Detune / 1200.0f);
                float osc3 = generateOsc (voice.oscPhase[2], osc3Freq, pOsc3Wave, srf);

                // FM: osc2 modulates osc1 phase
                if (effectiveFM > 0.001f)
                {
                    float fmAmount = effectiveFM * osc2 * 0.5f;
                    osc1 = generateOscWithFM (voice.oscPhase[0], modFreq, pOsc1Wave, srf, fmAmount);
                }

                // Sub-harmonic generator (octave below, always sine for purity)
                float subOctMul = std::pow (2.0f, std::round (pSubOctave));
                float subFreq = modFreq * subOctMul;
                voice.subPhase += subFreq / srf;
                if (voice.subPhase >= 1.0f) voice.subPhase -= 1.0f;
                float subOsc = fastSin (voice.subPhase * kTwoPi);

                // Mix oscillators
                float oscMix = osc1 * pOsc1Level
                             + osc2 * pOsc2Level
                             + osc3 * pOsc3Level
                             + subOsc * effectiveSubLevel;

                // Normalize if multiple oscillators are active
                float oscGainComp = 1.0f / std::max (1.0f,
                    pOsc1Level + pOsc2Level + pOsc3Level + effectiveSubLevel);
                oscMix *= oscGainComp;

                // =====================================================
                // 4. BIOLUMINESCENT EXCITER
                // =====================================================

                // Probability-based triggering (per sample)
                voice.exciterRng = voice.exciterRng * 1664525u + 1013904223u;
                float randVal = static_cast<float> (voice.exciterRng & 0xFFFF) / 65536.0f;

                // Trigger probability per sample (scaled to make sense at audio rate)
                float triggerProb = effectiveExciterProb * 0.0001f;
                if (randVal < triggerProb && !voice.exciter.active)
                {
                    voice.exciter.trigger (
                        pExciterFreq * (0.5f + randVal * 2.0f),
                        pExciterDecay, srf);
                }

                float exciterSig = voice.exciter.process (srf);

                // Mix exciter with oscillator
                float voiceSignal = oscMix + exciterSig * pExciterMix;

                // =====================================================
                // 6. DARKNESS FILTER — the ocean eats light
                // =====================================================

                // D001: velocity scales filter brightness
                float velFilterBoost = voice.velocity * 0.4f;
                float envFilterMod = filtEnvLevel * effectiveFiltEnvAmt * voice.velocity;

                // Per-sample darkness filter with creature and velocity modulation
                float dynCutoff = clamp (smoothedDarkness
                    + creatureFilterMod * 3000.0f
                    + envFilterMod * 8000.0f
                    + velFilterBoost * 2000.0f
                    + lfo2Val * 1000.0f, 20.0f, 20000.0f);

                voice.darknessFilter.setCoefficients_fast (dynCutoff, darkReso, srf);
                voiceSignal = voice.darknessFilter.processSample (voiceSignal);

                // =====================================================
                // 3. WAVEGUIDE BODY RESONANCE
                // =====================================================

                if (smoothedWreck > 0.001f)
                {
                    float bodyL = voice.bodyL.processSample (voiceSignal);
                    float bodyR = voice.bodyR.processSample (voiceSignal);

                    voiceSignal = voiceSignal * (1.0f - smoothedWreck)
                                + bodyL * smoothedWreck;

                    // Body adds stereo information
                    float stereoBody = bodyR - bodyL;
                    float finalL = voiceSignal;
                    float finalR = voiceSignal + stereoBody * smoothedWreck * 0.5f;

                    // Apply amplitude envelope, velocity, crossfade
                    float gain = ampLevel * voice.velocity * voice.fadeGain;
                    mixL += finalL * gain;
                    mixR += finalR * gain;
                }
                else
                {
                    // No body resonance: mono signal
                    float gain = ampLevel * voice.velocity * voice.fadeGain;
                    mixL += voiceSignal * gain;
                    mixR += voiceSignal * gain;
                }

                peakEnv = std::max (peakEnv, ampLevel);
            }

            // Denormal protection on mix bus
            mixL = flushDenormal (mixL);
            mixR = flushDenormal (mixR);

            // Apply master level
            float finalL = mixL * pLevel;
            float finalR = mixR * pLevel;

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

            // Cache for coupling
            if (sample < static_cast<int> (outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t> (sample)] = finalL;
                outputCacheR[static_cast<size_t> (sample)] = finalR;
            }
        }

        // =====================================================
        // 2. HYDROSTATIC COMPRESSOR — pressure-dependent compression
        //    Applied post-voice-mix for maximum weight
        // =====================================================
        if (effectivePressure > 0.001f && buffer.getNumChannels() >= 2)
        {
            float* left  = buffer.getWritePointer (0);
            float* right = buffer.getWritePointer (1);
            hydroComp.processBlock (left, right, numSamples);

            // Update output cache with compressed signal
            for (int s = 0; s < numSamples && s < static_cast<int> (outputCacheL.size()); ++s)
            {
                outputCacheL[static_cast<size_t> (s)] = left[s];
                outputCacheR[static_cast<size_t> (s)] = right[s];
            }
        }

        // =====================================================
        // ABYSS REVERB — vastness of the deep
        //    Applied after compression for maximum depth
        // =====================================================
        if (effectiveAbyssMix > 0.001f && buffer.getNumChannels() >= 2)
        {
            float* left  = buffer.getWritePointer (0);
            float* right = buffer.getWritePointer (1);

            // Process reverb in-place
            float reverbL[2048];
            float reverbR[2048];
            int remaining = numSamples;
            int offset = 0;

            while (remaining > 0)
            {
                int blockSize = std::min (remaining, 2048);
                std::memcpy (reverbL, left + offset, static_cast<size_t> (blockSize) * sizeof (float));
                std::memcpy (reverbR, right + offset, static_cast<size_t> (blockSize) * sizeof (float));

                abyssReverb.processBlock (reverbL, reverbR, reverbL, reverbR, blockSize);

                std::memcpy (left + offset, reverbL, static_cast<size_t> (blockSize) * sizeof (float));
                std::memcpy (right + offset, reverbR, static_cast<size_t> (blockSize) * sizeof (float));

                offset += blockSize;
                remaining -= blockSize;
            }

            // Update coupling cache post-reverb
            for (int s = 0; s < numSamples && s < static_cast<int> (outputCacheL.size()); ++s)
            {
                outputCacheL[static_cast<size_t> (s)] = left[s];
                outputCacheR[static_cast<size_t> (s)] = right[s];
            }
        }

        envelopeOutput = peakEnv;

        // Reset coupling accumulators (after render, Sisters S-014 pattern)
        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingFMMod = 0.0f;
        couplingDecayMod = 0.0f;

        int count = 0;
        for (const auto& v : voices)
            if (v.active) ++count;
        activeVoices = count;
    }

    //==========================================================================
    // Coupling
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
            case CouplingType::AmpToFilter:
                couplingFilterMod += amount;
                break;
            case CouplingType::LFOToPitch:
                couplingPitchMod += amount * 0.5f;
                break;
            case CouplingType::AudioToFM:
                couplingFMMod += amount * 0.3f;
                break;
            case CouplingType::EnvToDecay:
                couplingDecayMod += amount * 0.4f;
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
        // --- Sub Oscillator Stack ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "deep_osc1Wave", 1 }, "Deep Osc1 Waveform",
            juce::StringArray { "Sine", "Triangle", "Square" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "deep_osc2Wave", 1 }, "Deep Osc2 Waveform",
            juce::StringArray { "Sine", "Triangle", "Square" }, 1));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "deep_osc3Wave", 1 }, "Deep Osc3 Waveform",
            juce::StringArray { "Sine", "Triangle", "Square" }, 2));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_osc1Level", 1 }, "Deep Osc1 Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_osc2Level", 1 }, "Deep Osc2 Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_osc3Level", 1 }, "Deep Osc3 Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_osc2Detune", 1 }, "Deep Osc2 Detune",
            juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_osc3Detune", 1 }, "Deep Osc3 Detune",
            juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_subLevel", 1 }, "Deep Sub-Harmonic Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_subOctave", 1 }, "Deep Sub Octave",
            juce::NormalisableRange<float> (-2.0f, -1.0f, 1.0f), -1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_oscFM", 1 }, "Deep FM Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_glide", 1 }, "Deep Glide",
            juce::NormalisableRange<float> (0.0f, 5.0f, 0.001f, 0.5f), 0.1f));

        // --- Hydrostatic Compressor ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_compThresh", 1 }, "Deep Comp Threshold",
            juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -12.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_compRatio", 1 }, "Deep Comp Ratio",
            juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f), 4.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_compAttack", 1 }, "Deep Comp Attack",
            juce::NormalisableRange<float> (0.1f, 100.0f, 0.1f), 5.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_compRelease", 1 }, "Deep Comp Release",
            juce::NormalisableRange<float> (10.0f, 1000.0f, 1.0f), 80.0f));

        // --- Waveguide Body Resonance ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_bodyType", 1 }, "Deep Body Type",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_bodyFeedback", 1 }, "Deep Body Feedback",
            juce::NormalisableRange<float> (0.0f, 0.97f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_bodyMix", 1 }, "Deep Body Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Bioluminescent Exciter ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_exciterProb", 1 }, "Deep Exciter Probability",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.1f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_exciterFreq", 1 }, "Deep Exciter Frequency",
            juce::NormalisableRange<float> (500.0f, 8000.0f, 1.0f, 0.3f), 3000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_exciterDecay", 1 }, "Deep Exciter Decay",
            juce::NormalisableRange<float> (0.01f, 0.5f, 0.001f), 0.1f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_exciterMix", 1 }, "Deep Exciter Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Creature Modulation ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_creature1Rate", 1 }, "Deep Creature1 Rate",
            juce::NormalisableRange<float> (0.005f, 5.0f, 0.001f, 0.3f), 0.05f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_creature1Drift", 1 }, "Deep Creature1 Drift",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_creature2Rate", 1 }, "Deep Creature2 Rate",
            juce::NormalisableRange<float> (0.005f, 5.0f, 0.001f, 0.3f), 0.02f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_creature2Drift", 1 }, "Deep Creature2 Drift",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_creatureToPitch", 1 }, "Deep Creature to Pitch",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_creatureToFilter", 1 }, "Deep Creature to Filter",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Darkness Filter ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_darkCutoff", 1 }, "Deep Darkness Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_darkReso", 1 }, "Deep Darkness Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "deep_darkMode", 1 }, "Deep Darkness Mode",
            juce::StringArray { "LowPass", "BandPass", "HighPass", "Notch" }, 0));

        // --- Filter Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_filtEnvAmt", 1 }, "Deep Filter Env Amount",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_filtEnvAttack", 1 }, "Deep Filter Env Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_filtEnvDecay", 1 }, "Deep Filter Env Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_filtEnvSustain", 1 }, "Deep Filter Env Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_filtEnvRelease", 1 }, "Deep Filter Env Release",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.3f));

        // --- Amp Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_ampAttack", 1 }, "Deep Amp Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_ampDecay", 1 }, "Deep Amp Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_ampSustain", 1 }, "Deep Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.9f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_ampRelease", 1 }, "Deep Amp Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.5f));

        // --- Mod Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_modAttack", 1 }, "Deep Mod Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_modDecay", 1 }, "Deep Mod Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_modSustain", 1 }, "Deep Mod Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_modRelease", 1 }, "Deep Mod Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.5f));

        // --- LFO 1 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_lfo1Rate", 1 }, "Deep LFO1 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_lfo1Depth", 1 }, "Deep LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "deep_lfo1Shape", 1 }, "Deep LFO1 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- LFO 2 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_lfo2Rate", 1 }, "Deep LFO2 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 0.1f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_lfo2Depth", 1 }, "Deep LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "deep_lfo2Shape", 1 }, "Deep LFO2 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- Abyss Reverb ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_abyssSize", 1 }, "Deep Abyss Size",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_abyssDamp", 1 }, "Deep Abyss Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_abyssMix", 1 }, "Deep Abyss Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_abyssPreDelay", 1 }, "Deep Abyss Pre-Delay",
            juce::NormalisableRange<float> (0.0f, 0.1f, 0.001f), 0.0f));

        // --- Voice Mode ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "deep_polyphony", 1 }, "Deep Voice Mode",
            juce::StringArray { "Mono", "Legato", "Poly4", "Poly8" }, 2));

        // --- Level ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_level", 1 }, "Deep Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // --- Macros ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_macroPressure", 1 }, "Deep Macro PRESSURE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_macroCreature", 1 }, "Deep Macro CREATURE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_macroWreck", 1 }, "Deep Macro WRECK",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "deep_macroAbyss", 1 }, "Deep Macro ABYSS",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        // Sub oscillator stack
        paramOsc1Wave     = apvts.getRawParameterValue ("deep_osc1Wave");
        paramOsc2Wave     = apvts.getRawParameterValue ("deep_osc2Wave");
        paramOsc3Wave     = apvts.getRawParameterValue ("deep_osc3Wave");
        paramOsc1Level    = apvts.getRawParameterValue ("deep_osc1Level");
        paramOsc2Level    = apvts.getRawParameterValue ("deep_osc2Level");
        paramOsc3Level    = apvts.getRawParameterValue ("deep_osc3Level");
        paramOsc2Detune   = apvts.getRawParameterValue ("deep_osc2Detune");
        paramOsc3Detune   = apvts.getRawParameterValue ("deep_osc3Detune");
        paramSubLevel     = apvts.getRawParameterValue ("deep_subLevel");
        paramSubOctave    = apvts.getRawParameterValue ("deep_subOctave");
        paramOscFM        = apvts.getRawParameterValue ("deep_oscFM");
        paramGlide        = apvts.getRawParameterValue ("deep_glide");

        // Hydrostatic compressor
        paramCompThresh    = apvts.getRawParameterValue ("deep_compThresh");
        paramCompRatio     = apvts.getRawParameterValue ("deep_compRatio");
        paramCompAttackMs  = apvts.getRawParameterValue ("deep_compAttack");
        paramCompReleaseMs = apvts.getRawParameterValue ("deep_compRelease");

        // Body resonance
        paramBodyType     = apvts.getRawParameterValue ("deep_bodyType");
        paramBodyFeedback = apvts.getRawParameterValue ("deep_bodyFeedback");
        paramBodyMix      = apvts.getRawParameterValue ("deep_bodyMix");

        // Exciter
        paramExciterProb  = apvts.getRawParameterValue ("deep_exciterProb");
        paramExciterFreq  = apvts.getRawParameterValue ("deep_exciterFreq");
        paramExciterDecay = apvts.getRawParameterValue ("deep_exciterDecay");
        paramExciterMix   = apvts.getRawParameterValue ("deep_exciterMix");

        // Creature modulation
        paramCreature1Rate  = apvts.getRawParameterValue ("deep_creature1Rate");
        paramCreature1Drift = apvts.getRawParameterValue ("deep_creature1Drift");
        paramCreature2Rate  = apvts.getRawParameterValue ("deep_creature2Rate");
        paramCreature2Drift = apvts.getRawParameterValue ("deep_creature2Drift");
        paramCreatureToPitch  = apvts.getRawParameterValue ("deep_creatureToPitch");
        paramCreatureToFilter = apvts.getRawParameterValue ("deep_creatureToFilter");

        // Darkness filter
        paramDarkCutoff   = apvts.getRawParameterValue ("deep_darkCutoff");
        paramDarkReso     = apvts.getRawParameterValue ("deep_darkReso");
        paramDarkMode     = apvts.getRawParameterValue ("deep_darkMode");

        // Filter envelope
        paramFilterEnvAmt     = apvts.getRawParameterValue ("deep_filtEnvAmt");
        paramFilterEnvAttack  = apvts.getRawParameterValue ("deep_filtEnvAttack");
        paramFilterEnvDecay   = apvts.getRawParameterValue ("deep_filtEnvDecay");
        paramFilterEnvSustain = apvts.getRawParameterValue ("deep_filtEnvSustain");
        paramFilterEnvRelease = apvts.getRawParameterValue ("deep_filtEnvRelease");

        // Amp envelope
        paramAmpAttack    = apvts.getRawParameterValue ("deep_ampAttack");
        paramAmpDecay     = apvts.getRawParameterValue ("deep_ampDecay");
        paramAmpSustain   = apvts.getRawParameterValue ("deep_ampSustain");
        paramAmpRelease   = apvts.getRawParameterValue ("deep_ampRelease");

        // Mod envelope
        paramModAttack    = apvts.getRawParameterValue ("deep_modAttack");
        paramModDecay     = apvts.getRawParameterValue ("deep_modDecay");
        paramModSustain   = apvts.getRawParameterValue ("deep_modSustain");
        paramModRelease   = apvts.getRawParameterValue ("deep_modRelease");

        // LFOs
        paramLfo1Rate     = apvts.getRawParameterValue ("deep_lfo1Rate");
        paramLfo1Depth    = apvts.getRawParameterValue ("deep_lfo1Depth");
        paramLfo1Shape    = apvts.getRawParameterValue ("deep_lfo1Shape");
        paramLfo2Rate     = apvts.getRawParameterValue ("deep_lfo2Rate");
        paramLfo2Depth    = apvts.getRawParameterValue ("deep_lfo2Depth");
        paramLfo2Shape    = apvts.getRawParameterValue ("deep_lfo2Shape");

        // Abyss reverb
        paramAbyssSize     = apvts.getRawParameterValue ("deep_abyssSize");
        paramAbyssDamp     = apvts.getRawParameterValue ("deep_abyssDamp");
        paramAbyssMix      = apvts.getRawParameterValue ("deep_abyssMix");
        paramAbyssPreDelay = apvts.getRawParameterValue ("deep_abyssPreDelay");

        // Voice mode
        paramVoiceMode    = apvts.getRawParameterValue ("deep_polyphony");

        // Level
        paramLevel        = apvts.getRawParameterValue ("deep_level");

        // Macros
        paramMacroPressure = apvts.getRawParameterValue ("deep_macroPressure");
        paramMacroCreature = apvts.getRawParameterValue ("deep_macroCreature");
        paramMacroWreck    = apvts.getRawParameterValue ("deep_macroWreck");
        paramMacroAbyss    = apvts.getRawParameterValue ("deep_macroAbyss");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String getEngineId() const override { return "OceanDeep"; }

    // Trench Violet — the deepest darkness of the hadal zone
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF2D0A4E); }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override { return activeVoices; }

private:
    //==========================================================================
    // Safe parameter load
    //==========================================================================

    static float loadParam (std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    //==========================================================================
    // Oscillator generation — sine, triangle, square with anti-aliased edges
    //==========================================================================

    static float generateOsc (float& phase, float freq, int waveform, float sampleRate) noexcept
    {
        float phaseInc = freq / sampleRate;
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;
        if (phase < 0.0f)  phase += 1.0f;

        switch (waveform)
        {
            case 0: // Sine — pure, massive fundamental
                return fastSin (phase * 6.28318530718f);

            case 1: // Triangle — slightly brighter, still round
            {
                float t = 4.0f * std::fabs (phase - 0.5f) - 1.0f;
                return t;
            }

            case 2: // Square — hollow, powerful sub
            {
                // PolyBLEP square for reduced aliasing
                float naive = (phase < 0.5f) ? 1.0f : -1.0f;

                // PolyBLEP correction at transitions
                float t;
                // Transition at phase = 0
                t = phase / phaseInc;
                if (t < 1.0f)
                    naive += (2.0f * t - t * t - 1.0f);
                else
                {
                    t = (phase - 1.0f) / phaseInc;
                    if (t > -1.0f)
                        naive += (t * t + 2.0f * t + 1.0f);
                }
                // Transition at phase = 0.5
                t = (phase - 0.5f) / phaseInc;
                if (t >= 0.0f && t < 1.0f)
                    naive -= (2.0f * t - t * t - 1.0f);
                else
                {
                    t = (phase - 0.5f - 1.0f) / phaseInc;
                    if (t > -1.0f && t < 0.0f)
                        naive -= (t * t + 2.0f * t + 1.0f);
                }
                return naive;
            }

            default:
                return fastSin (phase * 6.28318530718f);
        }
    }

    static float generateOscWithFM (float& phase, float freq, int waveform,
                                     float sampleRate, float fmAmount) noexcept
    {
        float phaseInc = freq / sampleRate;
        float modPhase = phase + fmAmount;
        // Wrap modulated phase
        modPhase = modPhase - static_cast<float> (static_cast<int> (modPhase));
        if (modPhase < 0.0f) modPhase += 1.0f;

        // Advance the base phase normally
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;

        switch (waveform)
        {
            case 0:  return fastSin (modPhase * 6.28318530718f);
            case 1:  return 4.0f * std::fabs (modPhase - 0.5f) - 1.0f;
            case 2:  return (modPhase < 0.5f) ? 1.0f : -1.0f;
            default: return fastSin (modPhase * 6.28318530718f);
        }
    }

    //==========================================================================
    // MIDI note handling
    //==========================================================================

    void noteOn (int noteNumber, float velocity, int midiChannel,
                 int maxPoly, bool monoMode, bool legatoMode, float glideCoeff,
                 float ampA, float ampD, float ampS, float ampR,
                 float modA, float modD, float modS, float modR,
                 float filtA, float filtD, float filtS, float filtR,
                 float lfo1Rate, int lfo1Shape, float lfo2Rate, int lfo2Shape,
                 float c1Rate, float c1Drift, float c2Rate, float c2Drift,
                 float cutoff, float reso, CytomicSVF::Mode filterMode)
    {
        float freq = 440.0f * fastPow2 ((static_cast<float> (noteNumber) - 69.0f) / 12.0f);

        if (monoMode)
        {
            auto& voice = voices[0];
            bool wasActive = voice.active;

            voice.targetFreq = freq;
            voice.glideCoeff = glideCoeff;

            if (!wasActive || !legatoMode)
            {
                if (!wasActive)
                    voice.currentFreq = freq;

                voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
                voice.ampEnv.noteOn();
                voice.modEnv.setParams (modA, modD, modS, modR, srf);
                voice.modEnv.noteOn();
                voice.filterEnv.setParams (filtA, filtD, filtS, filtR, srf);
                voice.filterEnv.noteOn();
            }

            voice.active = true;
            voice.noteNumber = noteNumber;
            voice.velocity = velocity;
            voice.fadingOut = false;
            voice.fadeGain = 1.0f;
            voice.startTime = ++voiceCounter;

            voice.mpeExpression.reset();
            voice.mpeExpression.midiChannel = midiChannel;
            if (mpeManager != nullptr)
                mpeManager->updateVoiceExpression (voice.mpeExpression);

            voice.lfo1.setRate (lfo1Rate, srf);
            voice.lfo1.setShape (lfo1Shape);
            voice.lfo2.setRate (lfo2Rate, srf);
            voice.lfo2.setShape (lfo2Shape);
            voice.creature1.setRate (c1Rate, srf);
            voice.creature1.setDrift (c1Drift);
            voice.creature2.setRate (c2Rate, srf);
            voice.creature2.setDrift (c2Drift);

            voice.darknessFilter.setMode (filterMode);
            voice.darknessFilter.setCoefficients (cutoff, reso, srf);

            return;
        }

        // Polyphonic — find free voice or steal oldest
        int freeSlot = -1;
        uint64_t oldest = UINT64_MAX;
        int oldestSlot = 0;

        for (int i = 0; i < maxPoly && i < kMaxVoices; ++i)
        {
            if (!voices[i].active)
            {
                freeSlot = i;
                break;
            }
            if (voices[i].startTime < oldest)
            {
                oldest = voices[i].startTime;
                oldestSlot = i;
            }
        }

        int slot = (freeSlot >= 0) ? freeSlot : oldestSlot;

        if (freeSlot < 0)
            voices[slot].fadingOut = true;

        auto& voice = voices[slot];
        voice.reset();
        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.currentFreq = freq;
        voice.targetFreq = freq;
        voice.glideCoeff = glideCoeff;
        voice.startTime = ++voiceCounter;
        voice.exciterRng = static_cast<uint32_t> (slot * 999 + voiceCounter);

        voice.mpeExpression.reset();
        voice.mpeExpression.midiChannel = midiChannel;
        if (mpeManager != nullptr)
            mpeManager->updateVoiceExpression (voice.mpeExpression);

        voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
        voice.ampEnv.noteOn();
        voice.modEnv.setParams (modA, modD, modS, modR, srf);
        voice.modEnv.noteOn();
        voice.filterEnv.setParams (filtA, filtD, filtS, filtR, srf);
        voice.filterEnv.noteOn();

        voice.lfo1.setRate (lfo1Rate, srf);
        voice.lfo1.setShape (lfo1Shape);
        voice.lfo2.setRate (lfo2Rate, srf);
        voice.lfo2.setShape (lfo2Shape);
        voice.creature1.setRate (c1Rate, srf);
        voice.creature1.setDrift (c1Drift);
        voice.creature2.setRate (c2Rate, srf);
        voice.creature2.setDrift (c2Drift);

        voice.darknessFilter.setMode (filterMode);
        voice.darknessFilter.setCoefficients (cutoff, reso, srf);
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
                voice.filterEnv.noteOff();
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
    float modWheelAmount_ = 0.0f;   // CC#1 — creature modulation intensity (D006)
    float aftertouch_ = 0.0f;       // aftertouch — PRESSURE macro boost (D006)

    // Voices
    std::array<DeepVoice, kMaxVoices> voices {};
    int activeVoices = 0;

    // Hydrostatic compressor
    Compressor hydroComp;

    // Abyss reverb
    LushReverb abyssReverb;

    // Output cache for coupling
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;
    float envelopeOutput = 0.0f;

    // Coupling modulation accumulators
    float couplingFilterMod = 0.0f;
    float couplingPitchMod = 0.0f;
    float couplingFMMod = 0.0f;
    float couplingDecayMod = 0.0f;

    // Smoothed control values
    float smoothedDarkness = 8000.0f;
    float smoothedPressure = 0.0f;
    float smoothedCreature = 0.0f;
    float smoothedWreck = 0.0f;
    float smoothedAbyss = 0.0f;

    // Parameter pointers
    // Sub oscillator stack
    std::atomic<float>* paramOsc1Wave = nullptr;
    std::atomic<float>* paramOsc2Wave = nullptr;
    std::atomic<float>* paramOsc3Wave = nullptr;
    std::atomic<float>* paramOsc1Level = nullptr;
    std::atomic<float>* paramOsc2Level = nullptr;
    std::atomic<float>* paramOsc3Level = nullptr;
    std::atomic<float>* paramOsc2Detune = nullptr;
    std::atomic<float>* paramOsc3Detune = nullptr;
    std::atomic<float>* paramSubLevel = nullptr;
    std::atomic<float>* paramSubOctave = nullptr;
    std::atomic<float>* paramOscFM = nullptr;
    std::atomic<float>* paramGlide = nullptr;

    // Hydrostatic compressor
    std::atomic<float>* paramCompThresh = nullptr;
    std::atomic<float>* paramCompRatio = nullptr;
    std::atomic<float>* paramCompAttackMs = nullptr;
    std::atomic<float>* paramCompReleaseMs = nullptr;

    // Body resonance
    std::atomic<float>* paramBodyType = nullptr;
    std::atomic<float>* paramBodyFeedback = nullptr;
    std::atomic<float>* paramBodyMix = nullptr;

    // Exciter
    std::atomic<float>* paramExciterProb = nullptr;
    std::atomic<float>* paramExciterFreq = nullptr;
    std::atomic<float>* paramExciterDecay = nullptr;
    std::atomic<float>* paramExciterMix = nullptr;

    // Creature modulation
    std::atomic<float>* paramCreature1Rate = nullptr;
    std::atomic<float>* paramCreature1Drift = nullptr;
    std::atomic<float>* paramCreature2Rate = nullptr;
    std::atomic<float>* paramCreature2Drift = nullptr;
    std::atomic<float>* paramCreatureToPitch = nullptr;
    std::atomic<float>* paramCreatureToFilter = nullptr;

    // Darkness filter
    std::atomic<float>* paramDarkCutoff = nullptr;
    std::atomic<float>* paramDarkReso = nullptr;
    std::atomic<float>* paramDarkMode = nullptr;

    // Filter envelope
    std::atomic<float>* paramFilterEnvAmt = nullptr;
    std::atomic<float>* paramFilterEnvAttack = nullptr;
    std::atomic<float>* paramFilterEnvDecay = nullptr;
    std::atomic<float>* paramFilterEnvSustain = nullptr;
    std::atomic<float>* paramFilterEnvRelease = nullptr;

    // Amp envelope
    std::atomic<float>* paramAmpAttack = nullptr;
    std::atomic<float>* paramAmpDecay = nullptr;
    std::atomic<float>* paramAmpSustain = nullptr;
    std::atomic<float>* paramAmpRelease = nullptr;

    // Mod envelope
    std::atomic<float>* paramModAttack = nullptr;
    std::atomic<float>* paramModDecay = nullptr;
    std::atomic<float>* paramModSustain = nullptr;
    std::atomic<float>* paramModRelease = nullptr;

    // LFOs
    std::atomic<float>* paramLfo1Rate = nullptr;
    std::atomic<float>* paramLfo1Depth = nullptr;
    std::atomic<float>* paramLfo1Shape = nullptr;
    std::atomic<float>* paramLfo2Rate = nullptr;
    std::atomic<float>* paramLfo2Depth = nullptr;
    std::atomic<float>* paramLfo2Shape = nullptr;

    // Abyss reverb
    std::atomic<float>* paramAbyssSize = nullptr;
    std::atomic<float>* paramAbyssDamp = nullptr;
    std::atomic<float>* paramAbyssMix = nullptr;
    std::atomic<float>* paramAbyssPreDelay = nullptr;

    // Voice mode
    std::atomic<float>* paramVoiceMode = nullptr;

    // Level
    std::atomic<float>* paramLevel = nullptr;

    // Macros
    std::atomic<float>* paramMacroPressure = nullptr;
    std::atomic<float>* paramMacroCreature = nullptr;
    std::atomic<float>* paramMacroWreck = nullptr;
    std::atomic<float>* paramMacroAbyss = nullptr;
};

} // namespace xomnibus
